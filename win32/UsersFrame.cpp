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
#include <dcpp/QueueManager.h>
#include <dcpp/UploadManager.h>

#include <dcpp/version.h>

#include <dwt/widgets/Splitter.h>

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
	tstring (*convert)(const string &val);
};

static tstring formatBytes(const string& val) {
	return Text::toT(Util::formatBytes(val));
}

static const FieldName fields[] =
{
	{ "NI", T_("Nick"), &Text::toT },
	{ "AW", T_("Away"), &Text::toT },
	{ "DE", T_("Description"), &Text::toT },
	{ "EM", T_("E-Mail"), &Text::toT },
	{ "SS", T_("Shared bytes"), &formatBytes },
	{ "SF", T_("Shared files"), &Text::toT },
	{ "US", T_("Upload speed"), &Text::toT },
	{ "DS", T_("Download speed"), &Text::toT },
	{ "SL", T_("Slots"), &Text::toT },
	{ "HN", T_("Hubs (normal)"), &Text::toT },
	{ "HR", T_("Hubs (registered)"), &Text::toT },
	{ "HO", T_("Hubs (op)"), &Text::toT },
	{ "I4", T_("IP (v4)"), &Text::toT },
	{ "I6", T_("IP (v6)"), &Text::toT },
	{ "U4", T_("Search port (v4)"), &Text::toT },
	{ "U6", T_("Search port (v4)"), &Text::toT },
	{ "SU", T_("Features"), &Text::toT },
	{ "VE", T_("Client"), &Text::toT },
	{ "ID", T_("CID"), &Text::toT },

	{ "", _T(""), 0 }
};

UsersFrame::UsersFrame(TabViewPtr parent) :
	BaseType(parent, T_("Users"), IDH_FAVUSERS, IDI_FAVORITE_USERS),
	users(0),
	startup(true)
{
	filterGrid = addChild(Grid::Seed(1, 5));

	filter = filterGrid->addChild(WinUtil::Seeds::textBox);
	filter->onUpdated(std::bind(&UsersFrame::handleFilterUpdated, this));
	filterGrid->column(0).mode = GridInfo::FILL;

	showOnline = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showOnline->setText(_T("Online"));
	showOnline->setChecked(); // TODO save / restore last state
	showOnline->onClicked(std::bind(&UsersFrame::handleFilterUpdated, this));

	showFavs = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showFavs->setText(_T("Favorite"));
	showFavs->setChecked();	// TODO save / restore last state
	showFavs->onClicked(std::bind(&UsersFrame::handleFilterUpdated, this));

	showQueue = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showQueue->setText(_T("Pending download"));
	showQueue->onClicked(std::bind(&UsersFrame::handleFilterUpdated, this));

	showWaiting = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showWaiting->setText(_T("Pending upload"));
	showWaiting->onClicked(std::bind(&UsersFrame::handleFilterUpdated, this));

	splitter = addChild(VSplitter::Seed(0.7));

	if(!userIcons) {
		userIcons = dwt::ImageListPtr(new dwt::ImageList(dwt::Point(16, 16)));
		userIcons->add(dwt::Icon(IDI_FAVORITE_USER_OFF));
		userIcons->add(dwt::Icon(IDI_FAVORITE_USER_ON));
		userIcons->add(dwt::Icon(IDI_GRANT_SLOT_OFF));
		userIcons->add(dwt::Icon(IDI_GRANT_SLOT_ON));
	}

	{
		WidgetUsers::Seed cs;
		cs.lvStyle |= LVS_EX_SUBITEMIMAGES;
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

		for(auto i = ou.begin(); i != ou.end(); ++i) {
			addUser(i->second);
		}
	}

	FavoriteManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);

	initStatus();

	layout();

	startup = false;

	handleFilterUpdated();
}

UsersFrame::~UsersFrame() {
}

void UsersFrame::layout() {
	dwt::Rectangle r(dwt::Point(0, 0), getClientSize());

	status->layout(r);

	auto r2 = r;
	auto r2y = filter->getPreferredSize().y;
	r2.pos.y = r2.pos.y + r2.size.y - r2y;
	r2.size.y = r2y;

	filterGrid->layout(r2);

	r.size.y -= filter->getWindowSize().y + 3;

	splitter->layout(r);
}

bool UsersFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);

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
	if(!show(aUser, true)) {
		return;
	}

	auto ui = userInfos.find(aUser);
	if(ui == userInfos.end()) {
		auto x = make_shared<UserInfo>(aUser);
		userInfos.insert(std::make_pair(aUser, x));

		if(matches(*x)) {
			users->insert(x.get());
		}
	} else {
		updateUser(aUser);
	}
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	auto ui = userInfos.find(aUser);
	if(ui != userInfos.end()) {
		if(!show(aUser, true)) {
			users->erase(ui->second.get());
			userInfos.erase(ui);
			return;
		}

		ui->second->update(aUser);

		if(matches(*ui->second)) {
			auto i = users->find(ui->second.get());
			if(i != -1) {
				users->update(i);
			} else {
				users->insert(ui->second.get());
			}
		}
	}
}

void UsersFrame::handleSelectionChanged() {
	// Clear old items
	auto children = userInfo->getChildren<Control>();
	auto v = std::vector<Control*>(children.first, children.second);

	for_each(v.begin(), v.end(), [](Control *w) { w->close(); });

	userInfo->clearRows();

	if(users->countSelected() != 1) {
		return;
	}

	auto sel = users->getSelectedData();
	if(!sel) {
		return;
	}

	auto user = sel->getUser();
	auto idents = ClientManager::getInstance()->getIdentities(user);
	if(idents.empty()) {
		return;
	}

	auto info = idents[0].getInfo();
	for(size_t i = 1; i < idents.size(); ++i) {
		for(auto j = idents[i].getInfo().begin(); j != idents[i].getInfo().end(); ++j) {
			info[j->first] = j->second;
		}
	}

	userInfo->addRow(GridInfo());
	auto general = userInfo->addChild(Label::Seed(T_("General information")));
	userInfo->setWidget(general, 0, 0, 1, 2);

	for(auto f = fields; !f->field.empty(); ++f) {
		auto i = info.find(f->field);
		if(i != info.end()) {
			userInfo->addRow(GridInfo());
			userInfo->addChild(Label::Seed(f->name));
			userInfo->addChild(Label::Seed(f->convert(i->second)));
			info.erase(i);
		}
	}

	for(auto i = info.begin(); i != info.end(); ++i) {
		userInfo->addRow(GridInfo());
		userInfo->addChild(Label::Seed(Text::toT(i->first)));
		userInfo->addChild(Label::Seed(Text::toT(i->second)));
	}

	auto queued = QueueManager::getInstance()->getQueued(user);

	if(queued.first) {
		userInfo->addRow(GridInfo());
		auto general = userInfo->addChild(Label::Seed(T_("Pending downloads information")));
		userInfo->setWidget(general, userInfo->rowCount()-1, 0, 1, 2);

		userInfo->addRow(GridInfo());
		userInfo->addChild(Label::Seed(T_("Queued files")));
		userInfo->addChild(Label::Seed(Text::toT(Util::toString(queued.first))));

		userInfo->addRow(GridInfo());
		userInfo->addChild(Label::Seed(T_("Queued bytes")));
		userInfo->addChild(Label::Seed(Text::toT(Util::formatBytes(queued.second))));
	}

	auto files = UploadManager::getInstance()->getWaitingUserFiles(user);
	if(!files.empty()) {
		userInfo->addRow(GridInfo());
		auto general = userInfo->addChild(Label::Seed(T_("Pending uploads information")));
		userInfo->setWidget(general, userInfo->rowCount()-1, 0, 1, 2);

		for(auto i = files.begin(); i != files.end(); ++i) {
			userInfo->addRow(GridInfo());
			userInfo->addChild(Label::Seed(T_("Filename")));
			userInfo->addChild(Label::Seed(Text::toT(*i)));
		}
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

void UsersFrame::handleFilterUpdated() {
	HoldRedraw h(users);
	users->clear();
	for(auto i = userInfos.begin(); i != userInfos.end(); ++i) {
		if(matches(*i->second)) {
			users->insert(i->second.get());
		}
	}
}

bool UsersFrame::matches(const UserInfo &ui) {
	auto txt = filter->getText();
	if(!txt.empty() && Util::findSubString(ui.columns[COLUMN_NICK], txt) == string::npos) {
		return false;
	}

	return show(ui.getUser(), false);
}

bool UsersFrame::show(const UserPtr &u, bool any) const {
	if((any || showOnline->getChecked()) && u->isOnline()) {
		return true;
	}

	if((any || showFavs->getChecked()) && FavoriteManager::getInstance()->isFavoriteUser(u)) {
		return true;
	}

	if((any || showWaiting->getChecked()) && UploadManager::getInstance()->isWaiting(u)) {
		return true;
	}

	if((any || showQueue->getChecked()) && QueueManager::getInstance()->getQueued(u).first > 0) {
		return true;
	}

	return false;
}

UsersFrame::UserInfoList UsersFrame::selectedUsersImpl() const {
	return usersFromTable(users);
}

void UsersFrame::on(UserAdded, const FavoriteUser& aUser) throw() {
	callAsync(std::bind(&UsersFrame::addUser, this, aUser.getUser()));
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

void UsersFrame::on(UserUpdated, const UserPtr& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser));
}

void UsersFrame::on(UserDisconnected, const UserPtr& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser));
}

void UsersFrame::on(WaitingAddFile, const HintedUser& aUser, const string&) throw() {
	callAsync(std::bind(&UsersFrame::addUser, this, aUser.user));
}

void UsersFrame::on(WaitingRemoveUser, const HintedUser& aUser) throw() {
	callAsync(std::bind(&UsersFrame::updateUser, this, aUser.user));
}

void UsersFrame::on(Added, QueueItem* qi) throw() {
	for(auto i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		callAsync(std::bind(&UsersFrame::addUser, this, i->getUser().user));
	}
}

void UsersFrame::on(SourcesUpdated, QueueItem* qi) throw() {
	for(auto i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		callAsync(std::bind(&UsersFrame::updateUser, this, i->getUser().user));
	}
}

void UsersFrame::on(Removed, QueueItem* qi) throw() {
	for(auto i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		callAsync(std::bind(&UsersFrame::updateUser, this, i->getUser().user));
	}
}
