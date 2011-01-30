/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"

#include "UsersFrame.h"
#include "ParamDlg.h"
#include "HoldRedraw.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/version.h>

const string UsersFrame::id = "Users";
const string& UsersFrame::getId() const { return id; }

dwt::ImageListPtr UsersFrame::userIcons;

static const ColumnInfo usersColumns[] = {
	{ N_("Favorite"), 25, false },
	{ N_("Grant slot"), 25, false },
	{ N_("Nick"), 125, false },
	{ N_("Hub (last seen in, if offline)"), 300, false },
	{ N_("Time last seen"), 150, false },
	{ N_("Description"), 200, false },
	{ N_("CID"), 300, false }
};

struct FieldName {
	string field;
	tstring name;
};

static const FieldName fields[] =
{
	{ "NI", T_("Nick") },
	{ "DE", T_("Description") },
};

UsersFrame::UsersFrame(TabViewPtr parent) :
	BaseType(parent, T_("Users"), IDH_FAVUSERS, IDI_FAVORITE_USERS),
	users(0),
	startup(true)
{
	splitter = addChild(VSplitter::Seed(0.3));

	if(!userIcons) {
		userIcons = dwt::ImageListPtr(new dwt::ImageList(dwt::Point(16, 16)));
		userIcons->add(dwt::Icon(IDI_FAVORITE_USER_OFF));
		userIcons->add(dwt::Icon(IDI_FAVORITE_USER_ON));
		userIcons->add(dwt::Icon(IDI_GRANT_SLOT_OFF));
		userIcons->add(dwt::Icon(IDI_GRANT_SLOT_ON));
	}

	{
		WidgetUsers::Seed cs;
		users = addChild(cs);
		addWidget(users);

		WinUtil::makeColumns(users, usersColumns, COLUMN_LAST, SETTING(USERSFRAME_ORDER), SETTING(USERSFRAME_WIDTHS));
		users->setSort(COLUMN_NICK);

		// TODO check default (browse vs get)
		users->onDblClicked(std::bind(&UsersFrame::handleGetList, this));
		users->onKeyDown(std::bind(&UsersFrame::handleKeyDown, this, _1));
		users->onContextMenu(std::bind(&UsersFrame::handleContextMenu, this, _1));
		users->onSelectionChanged(std::bind(&UsersFrame::handleSelectionChanged, this));
		users->setSmallImageList(userIcons);
		users->onLeftMouseDown(std::bind(&UsersFrame::handleClick, this, _1));
		prepareUserList(users);

		splitter->setFirst(users);
	}

	{
		Grid::Seed cs(0, 2);
		cs.style |= WS_VSCROLL;
		userInfo = addChild(cs);
		splitter->setSecond(userInfo);
	}

	{
		auto lock = ClientManager::getInstance()->lock();
		auto &ou = ClientManager::getInstance()->getUsers();

		HoldRedraw hold(users);
		for(auto i = ou.begin(); i != ou.end(); ++i) {
			if(i->second->isOnline()) {
				addUser(i->second);
			}
		}
	}

	{
		auto fu = FavoriteManager::getInstance()->getFavoriteUsers();

		HoldRedraw hold(users);
		for(auto i = fu.begin(); i != fu.end(); ++i) {
			addUser(i->second.getUser());
		}
	}

	FavoriteManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	initStatus();

	layout();

	startup = false;
}

UsersFrame::~UsersFrame() {
}

void UsersFrame::layout() {
	dwt::Rectangle r(dwt::Point(0, 0), getClientSize());

	status->layout(r);

	splitter->layout(r);
}

bool UsersFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	return true;
}

void UsersFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::USERSFRAME_ORDER, WinUtil::toString(users->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::USERSFRAME_WIDTHS, WinUtil::toString(users->getColumnWidths()));
}

UsersFrame::UserInfo::UserInfo(const FavoriteUser& u) :
UserInfoBase(HintedUser(u.getUser(), Util::emptyString)) /// @todo fav users aren't aware of hub hints for now
{
	update(u);
}

UsersFrame::UserInfo::UserInfo(const UserPtr& u) :
UserInfoBase(HintedUser(u, Util::emptyString)) /// @todo fav users aren't aware of hub hints for now
{
	update(u);
}

void UsersFrame::UserInfo::remove() {
	FavoriteManager::getInstance()->removeFavoriteUser(user);
}

void UsersFrame::UserInfo::update(const UserPtr& u) {
	columns[COLUMN_NICK] = WinUtil::getNicks(u, Util::emptyString);
	columns[COLUMN_HUB] = u->isOnline() ? WinUtil::getHubNames(u, Util::emptyString).first : Util::emptyStringT;
	columns[COLUMN_SEEN] = u->isOnline() ? T_("Online") : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", FavoriteManager::getInstance()->getLastSeen(u)));
	columns[COLUMN_CID] = Text::toT(u->getCID().toBase32());

	isFavorite = FavoriteManager::getInstance()->isFavoriteUser(u);
	grantSlot = FavoriteManager::getInstance()->hasSlot(u);
}

void UsersFrame::UserInfo::update(const FavoriteUser& u) {
	columns[COLUMN_NICK] = Text::toT(u.getNick());
	columns[COLUMN_HUB] = u.getUser()->isOnline() ? WinUtil::getHubNames(u.getUser(), Util::emptyString).first : Text::toT(u.getUrl());
	columns[COLUMN_SEEN] = u.getUser()->isOnline() ? T_("Online") : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", u.getLastSeen()));
	columns[COLUMN_DESCRIPTION] = Text::toT(u.getDescription());
	columns[COLUMN_CID] = Text::toT(u.getUser()->getCID().toBase32());
}

void UsersFrame::addUser(const UserPtr& aUser) {
	auto ui = userInfos.find(aUser);
	if(ui == userInfos.end()) {
		auto x = new UserInfo(aUser);
		userInfos.insert(std::make_pair(aUser, x));
		users->insert(x);
	} else {
		// TODO Update
	}
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	auto ui = userInfos.find(aUser);
	if(ui != userInfos.end()) {
		auto i = users->find(ui->second);
		if(i != -1) {
			ui->second->update(aUser);
			users->update(i);
		}
	}
}

void UsersFrame::removeUser(const UserPtr& aUser) {
	auto ui = userInfos.find(aUser);
	if(ui != userInfos.end()) {
		users->erase(ui->second);
		userInfos.erase(ui);
	}
}

void UsersFrame::handleSelectionChanged() {
	// Clear old items
	auto children = userInfo->getChildren<Control>();
	auto v = std::vector<Control*>(children.first, children.second);

	for_each(v.begin(), v.end(), [](Control *w) { w->close(); });

	userInfo->clearRows();

	auto sel = users->getSelectedData();
	if(!sel) {
		return;
	}

	auto lock = ClientManager::getInstance()->lock();
	auto ui = ClientManager::getInstance()->findOnlineUser(sel->getUser(), false);
	if(!ui) {
		return;
	}

	auto info = ui->getIdentity().getInfo();
	tstring text;
	for(auto i = info.begin(); i != info.end(); ++i) {
		userInfo->addRow(GridInfo());
		userInfo->addChild(Label::Seed(Text::toT(i->first)));
		userInfo->addChild(Label::Seed(Text::toT(i->second)));
	}

	layout();
}

void UsersFrame::handleDescription() {
	if(users->countSelected() == 1) {
		int i = users->getSelected();
		UserInfo* ui = users->getData(i);
		ParamDlg dlg(this, ui->columns[COLUMN_NICK], T_("Description"), ui->columns[COLUMN_DESCRIPTION]);

		if(dlg.run() == IDOK) {
			FavoriteManager::getInstance()->setUserDescription(ui->getUser(), Text::fromT(dlg.getValue()));
			ui->columns[COLUMN_DESCRIPTION] = dlg.getValue();
			users->update(i);
		}
	}
}

void UsersFrame::handleRemove() {
	if(!BOOLSETTING(CONFIRM_USER_REMOVAL) || dwt::MessageBox(this).show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == dwt::MessageBox::RETBOX_YES)
		users->forEachSelected(&UsersFrame::UserInfo::remove);
}

bool UsersFrame::handleKeyDown(int c) {
	switch(c) {
	case VK_DELETE:
		handleRemove();
		return true;
	case VK_RETURN:
		handleDescription();
		return true;
	}
	return false;
}

bool UsersFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	size_t sel = users->countSelected();
	if(sel > 0) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = users->getContextMenuPos();
		}

		MenuPtr menu = addChild(WinUtil::Seeds::menu);
		menu->setTitle((sel == 1) ? escapeMenu(users->getSelectedData()->getText(COLUMN_NICK)) : str(TF_("%1% users") % sel));
		appendUserItems(getParent(), menu);
		menu->appendSeparator();
		menu->appendItem(T_("&Description"), std::bind(&UsersFrame::handleDescription, this));
		menu->appendItem(T_("&Remove"), std::bind(&UsersFrame::handleRemove, this));

		menu->open(pt);

		return true;
	}

	return false;
}

bool UsersFrame::handleClick(const dwt::MouseEvent &me) {
	auto item = users->hitTest(me.pos);
	if(item.first == -1 || item.second == -1) {
		return false;
	}

	auto ui = users->getData(item.first);
	switch(item.second) {
	case COLUMN_FAVORITE:
		if(ui->isFavorite) {
			FavoriteManager::getInstance()->removeFavoriteUser(ui->getUser());
		} else {
			FavoriteManager::getInstance()->addFavoriteUser(ui->getUser());
		}

		ui->isFavorite = !ui->isFavorite;
		users->update(item.first);
		break;

	case COLUMN_SLOT:
		FavoriteManager::getInstance()->setAutoGrant(ui->getUser(), !ui->grantSlot);
		ui->grantSlot = !ui->grantSlot;
		users->update(item.first);
		break;

	default:
		return false;
	}

	return true;
}

UsersFrame::UserInfoList UsersFrame::selectedUsersImpl() const {
	return usersFromTable(users);
}

void UsersFrame::on(UserAdded, const FavoriteUser& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser.getUser()));
}

void UsersFrame::on(UserRemoved, const FavoriteUser& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser.getUser()));
}

void UsersFrame::on(StatusChanged, const UserPtr& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser));
}

void UsersFrame::on(UserConnected, const UserPtr& aUser) throw() {
	callAsync(std::bind(&UsersFrame::addUser, this, aUser));
}

void UsersFrame::on(UserDisconnected, const UserPtr& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser));
}
