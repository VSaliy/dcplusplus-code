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
#include <dcpp/version.h>

const string UsersFrame::id = "FavUsers";
const string& UsersFrame::getId() const { return id; }

static const ColumnInfo usersColumns[] = {
	{ N_("Auto grant slot / Nick"), 200, false },
	{ N_("Hub (last seen in, if offline)"), 300, false },
	{ N_("Time last seen"), 150, false },
	{ N_("Description"), 200, false },
	{ N_("CID"), 300, false }
};

UsersFrame::UsersFrame(TabViewPtr parent) :
	BaseType(parent, T_("Favorite Users"), IDH_FAVUSERS, IDI_FAVORITE_USERS),
	users(0),
	startup(true)
{
	{
		WidgetUsers::Seed cs;
		cs.lvStyle |= LVS_EX_CHECKBOXES;
		users = addChild(cs);
		addWidget(users);

		WinUtil::makeColumns(users, usersColumns, COLUMN_LAST, SETTING(USERSFRAME_ORDER), SETTING(USERSFRAME_WIDTHS));
		users->setSort(COLUMN_NICK);

		// TODO check default (browse vs get)
		users->onDblClicked(std::bind(&UsersFrame::handleGetList, this));
		users->onKeyDown(std::bind(&UsersFrame::handleKeyDown, this, _1));
		users->onRaw(std::bind(&UsersFrame::handleItemChanged, this, _2), dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));
		users->onContextMenu(std::bind(&UsersFrame::handleContextMenu, this, _1));

		prepareUserList(users);
	}

	{
		HoldRedraw hold(users);
		FavoriteManager::FavoriteMap ul = FavoriteManager::getInstance()->getFavoriteUsers();
		for(FavoriteManager::FavoriteMap::iterator i = ul.begin(); i != ul.end(); ++i) {
			addUser(i->second);
		}
	}

	FavoriteManager::getInstance()->addListener(this);

	initStatus();

	layout();

	startup = false;
}

UsersFrame::~UsersFrame() {
}

void UsersFrame::layout() {
	dwt::Rectangle r(dwt::Point(0, 0), getClientSize());

	status->layout(r);

	users->layout(r);
}

bool UsersFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);
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

void UsersFrame::UserInfo::remove() {
	FavoriteManager::getInstance()->removeFavoriteUser(user);
}

void UsersFrame::UserInfo::update(const FavoriteUser& u) {
	columns[COLUMN_NICK] = Text::toT(u.getNick());
	columns[COLUMN_HUB] = u.getUser()->isOnline() ? WinUtil::getHubNames(u.getUser(), Util::emptyString).first : Text::toT(u.getUrl());
	columns[COLUMN_SEEN] = u.getUser()->isOnline() ? T_("Online") : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", u.getLastSeen()));
	columns[COLUMN_DESCRIPTION] = Text::toT(u.getDescription());
	columns[COLUMN_CID] = Text::toT(u.getUser()->getCID().toBase32());
}

void UsersFrame::addUser(const FavoriteUser& aUser) {
	int i = users->insert(new UserInfo(aUser));
	bool b = aUser.isSet(FavoriteUser::FLAG_GRANTSLOT);
	users->setChecked(i, b);
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	for(size_t i = 0; i < users->size(); ++i) {
		UserInfo *ui = users->getData(i);
		if(ui->getUser() == aUser) {
			ui->columns[COLUMN_SEEN] = aUser->isOnline() ? T_("Online") : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", FavoriteManager::getInstance()->getLastSeen(aUser)));
			users->update(i);
		}
	}
}

void UsersFrame::removeUser(const UserPtr& aUser) {
	for(size_t i = 0; i < users->size(); ++i) {
		UserInfo *ui = users->getData(i);
		if(ui->getUser() == aUser) {
			users->erase(i);
			return;
		}
	}
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

LRESULT UsersFrame::handleItemChanged(LPARAM lParam) {
	LPNMITEMACTIVATE l = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
	if(!startup && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		FavoriteManager::getInstance()->setAutoGrant(users->getData(l->iItem)->getUser(), users->isChecked(l->iItem));
	}
	return 0;
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

UsersFrame::UserInfoList UsersFrame::selectedUsersImpl() const {
	return usersFromTable(users);
}

void UsersFrame::on(UserAdded, const FavoriteUser& aUser) throw() {
	addUser(aUser);
}

void UsersFrame::on(UserRemoved, const FavoriteUser& aUser) throw() {
	callAsync(std::bind(&UsersFrame::removeUser, this, aUser.getUser()));
}

void UsersFrame::on(StatusChanged, const UserPtr& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser));
}
