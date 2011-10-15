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

#include <dcpp/FavoriteManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/UploadManager.h>
#include <dcpp/ScopedFunctor.h>

#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/Splitter.h>
#include <dwt/widgets/ScrolledContainer.h>
#include <dwt/widgets/SplitterContainer.h>

#include "ParamDlg.h"
#include "HoldRedraw.h"
#include "TypedTable.h"

using std::for_each;
using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::SplitterContainer;

const string UsersFrame::id = "Users";
const string& UsersFrame::getId() const { return id; }

dwt::ImageListPtr UsersFrame::userIcons;

static const ColumnInfo usersColumns[] = {
	{ N_("Favorite"), 25, false },
	{ N_("Auto grant slot"), 25, false },
	{ N_("Nick"), 125, false },
	{ N_("Hub(s)"), 300, false },
	{ N_("Status / Time last seen"), 150, false },
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

static tstring formatSpeed(const string& val) {
	return Text::toT(str(F_("%1%/s") % Util::formatBytes(val)));
}

static const FieldName fields[] =
{
	{ "NI", T_("Nick"), &Text::toT },
	{ "AW", T_("Away"), &Text::toT },
	{ "DE", T_("Description"), &Text::toT },
	{ "EM", T_("E-Mail"), &Text::toT },
	{ "SS", T_("Shared bytes"), &formatBytes },
	{ "SF", T_("Shared files"), &Text::toT },
	{ "US", T_("Upload speed"), &formatSpeed },
	{ "DS", T_("Download speed"), &formatSpeed },
	{ "SL", T_("Total slots"), &Text::toT },
	{ "FS", T_("Free slots"), &Text::toT },
	{ "HN", T_("Hubs (normal)"), &Text::toT },
	{ "HR", T_("Hubs (registered)"), &Text::toT },
	{ "HO", T_("Hubs (op)"), &Text::toT },
	{ "I4", T_("IP (v4)"), &Text::toT },
	{ "I6", T_("IP (v6)"), &Text::toT },
	{ "U4", T_("Search port (v4)"), &Text::toT },
	{ "U6", T_("Search port (v6)"), &Text::toT },
	{ "SU", T_("Features"), &Text::toT },
	{ "VE", T_("Application version"), &Text::toT },
	{ "AP", T_("Application"), &Text::toT },
	{ "ID", T_("CID"), &Text::toT },
	{ "KP", T_("TLS Keyprint"), &Text::toT },
	{ "CO", T_("Connection"), &Text::toT },
	{ "CT", T_("Client type"), &Text::toT },
	{ "TA", T_("Tag"), &Text::toT },

	{ "", _T(""), 0 }
};

UsersFrame::UsersFrame(TabViewPtr parent) :
	BaseType(parent, T_("Users"), IDH_USERS, IDI_USERS),
	users(0),
	scroll(0),
	startup(true)
{
	filterGrid = addChild(Grid::Seed(1, 6));

	auto updated = [this] { handleFilterUpdated(); };
	filterGrid->addChild(Label::Seed(T_("Nick filter:")));
	filter = filterGrid->addChild(WinUtil::Seeds::textBox);
	filter->setHelpId(IDH_USERS_FILTER_NICK);
	filter->onUpdated(updated);
	filterGrid->column(1).mode = GridInfo::FILL;

	showOnline = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showOnline->setHelpId(IDH_USERS_FILTER_ONLINE);
	showOnline->setText(_T("Online"));
	showOnline->setChecked(BOOLSETTING(USERS_FILTER_ONLINE));
	showOnline->onClicked(updated);

	showFavs = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showFavs->setHelpId(IDH_USERS_FILTER_FAVORITE);
	showFavs->setText(_T("Favorite"));
	showFavs->setChecked(BOOLSETTING(USERS_FILTER_FAVORITE));
	showFavs->onClicked(updated);

	showQueue = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showQueue->setHelpId(IDH_USERS_FILTER_QUEUE);
	showQueue->setText(_T("Pending download"));
	showQueue->setChecked(BOOLSETTING(USERS_FILTER_QUEUE));
	showQueue->onClicked(updated);

	showWaiting = filterGrid->addChild(WinUtil::Seeds::checkBox);
	showWaiting->setHelpId(IDH_USERS_FILTER_WAITING);
	showWaiting->setText(_T("Pending upload"));
	showWaiting->setChecked(BOOLSETTING(USERS_FILTER_WAITING));
	showWaiting->onClicked(updated);

	splitter = addChild(SplitterContainer::Seed(0.7));

	if(!userIcons) {
		const dwt::Point size(16, 16);
		userIcons = dwt::ImageListPtr(new dwt::ImageList(size));
		userIcons->add(dwt::Icon(IDI_FAVORITE_USER_OFF, size));
		userIcons->add(dwt::Icon(IDI_FAVORITE_USER_ON, size));
		userIcons->add(dwt::Icon(IDI_GRANT_SLOT_OFF, size));
		userIcons->add(dwt::Icon(IDI_GRANT_SLOT_ON, size));
	}

	{
		WidgetUsers::Seed cs;
		cs.lvStyle |= LVS_EX_SUBITEMIMAGES;
		users = splitter->addChild(cs);
		addWidget(users);

		WinUtil::makeColumns(users, usersColumns, COLUMN_LAST, SETTING(USERSFRAME_ORDER), SETTING(USERSFRAME_WIDTHS));
		users->setSort(COLUMN_NICK);

		// TODO check default (browse vs get)
		users->onDblClicked([this] { handleGetList(); });
		users->onKeyDown([this](int c) { return handleKeyDown(c); });
		users->onContextMenu([this](dwt::ScreenCoordinate pt) { return handleContextMenu(pt); });
		users->onSelectionChanged([this] { handleSelectionChanged(); });
		users->setSmallImageList(userIcons);
		users->onLeftMouseDown([this](const dwt::MouseEvent &me) { return handleClick(me); });
		prepareUserList(users);
	}

	{
		scroll = splitter->addChild(dwt::ScrolledContainer::Seed());
		userInfo = scroll->addChild(Grid::Seed(0, 1));
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

	addAccel(FALT, 'I', [this] { filter->setFocus(); });
	initAccels();

	layout();

	startup = false;

	handleFilterUpdated();
}

UsersFrame::~UsersFrame() {
}

void UsersFrame::layout() {
	dwt::Rectangle r(dwt::Point(0, 0), getClientSize());

	r.size.y -= status->refresh();

	auto r2 = r;
	auto r2y = filter->getPreferredSize().y;
	r2.pos.y = r2.pos.y + r2.size.y - r2y;
	r2.size.y = r2y;

	filterGrid->resize(r2);

	r.size.y -= filter->getWindowSize().y + 3;

	splitter->resize(r);
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

	SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_ONLINE, showOnline->getChecked());
	SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_FAVORITE, showFavs->getChecked());
	SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_QUEUE, showQueue->getChecked());
	SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_WAITING, showWaiting->getChecked());
}

UsersFrame::UserInfo::UserInfo(const UserPtr& u, bool visible) :
UserInfoBase(HintedUser(u, Util::emptyString))
{
	update(u, visible);
}

void UsersFrame::UserInfo::remove() {
	FavoriteManager::getInstance()->removeFavoriteUser(user);
}

void UsersFrame::UserInfo::update(const UserPtr& u, bool visible) {
	auto fu = FavoriteManager::getInstance()->getFavoriteUser(u);
	if(fu) {
		isFavorite = true;
		grantSlot = fu->isSet(FavoriteUser::FLAG_GRANTSLOT);
		columns[COLUMN_NICK] = Text::toT(fu->getNick());
		if(!visible) {
			return;
		}
		columns[COLUMN_DESCRIPTION] = Text::toT(fu->getDescription());

		if(u->isOnline()) {
			columns[COLUMN_SEEN] = T_("Online");
			columns[COLUMN_HUB] = WinUtil::getHubNames(u, Util::emptyString).first;
		} else {
			columns[COLUMN_SEEN] = fu->getLastSeen() > 0 ? Text::toT(Util::formatTime("%Y-%m-%d %H:%M", fu->getLastSeen())) : T_("Offline");
			columns[COLUMN_HUB] = Text::toT(fu->getUrl());
		}
	} else {
		isFavorite = false;
		grantSlot = false;
		columns[COLUMN_NICK] = WinUtil::getNicks(u, Util::emptyString);

		if(!visible) {
			return;
		}
		if(u->isOnline()) {
			columns[COLUMN_SEEN] = T_("Online");
			columns[COLUMN_HUB] = WinUtil::getHubNames(u, Util::emptyString).first;
		} else {
			columns[COLUMN_SEEN] = T_("Offline");
		}
	}

	columns[COLUMN_CID] = Text::toT(u->getCID().toBase32());
}

void UsersFrame::addUser(const UserPtr& aUser) {
	if(!show(aUser, true)) {
		return;
	}

	auto ui = userInfos.find(aUser);
	if(ui == userInfos.end()) {
		auto x = userInfos.insert(std::make_pair(aUser, UserInfo(aUser, false))).first;

		if(matches(x->second)) {
			x->second.update(aUser, true);
			users->insert(&x->second);
		}
	} else {
		updateUser(aUser);
	}
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	auto ui = userInfos.find(aUser);
	if(ui != userInfos.end()) {
		ui->second.update(aUser, false);

		if(!show(aUser, false)) {
			users->erase(&ui->second);
			if(!show(aUser, true)) {
				userInfos.erase(aUser);
			}
			return;
		}

		if(matches(ui->second)) {
			ui->second.update(aUser, true);
			auto i = users->find(&ui->second);
			if(i != -1) {
				if(users->getSelected() == i) {
					handleSelectionChanged();
				} else {
					users->update(i);
				}
			} else {
				users->insert(&ui->second);
			}
		}
	}
}

void UsersFrame::handleSelectionChanged() {
	ScopedFunctor([&] { scroll->layout(); userInfo->layout(); userInfo->redraw(); });

	HoldRedraw hold(userInfo);

	// Clear old items
	auto children = userInfo->getChildren<Control>();
	auto v = std::vector<Control*>(children.first, children.second);

	for_each(v.begin(), v.end(), [](Control *w) { w->close(); });

	userInfo->clearRows();

	if(users->countSelected() != 1) {
		userInfo->redraw();
		return;
	}

	auto sel = users->getSelectedData();
	if(!sel) {
		userInfo->redraw();
		return;
	}

	auto user = sel->getUser();
	auto idents = ClientManager::getInstance()->getIdentities(user);
	if(idents.empty()) {
		userInfo->redraw();
		return;
	}

	auto info = idents[0].getInfo();
	for(size_t i = 1; i < idents.size(); ++i) {
		auto info2 = idents[i].getInfo();
		for(auto j = info2.begin(); j != info2.end(); ++j) {
			info[j->first] = j->second;
		}
	}

	userInfo->addRow(GridInfo());
	auto generalGroup = userInfo->addChild(GroupBox::Seed(T_("General information")));
	auto generalGrid = generalGroup->addChild(Grid::Seed(0, 2));

	for(auto f = fields; !f->field.empty(); ++f) {
		auto i = info.find(f->field);
		if(i != info.end()) {
			generalGrid->addRow(GridInfo());
			generalGrid->addChild(Label::Seed(f->name));
			generalGrid->addChild(Label::Seed(f->convert(i->second)));
			info.erase(i);
		}
	}

	for(auto i = info.begin(); i != info.end(); ++i) {
		generalGrid->addRow(GridInfo());
		generalGrid->addChild(Label::Seed(Text::toT(i->first)));
		generalGrid->addChild(Label::Seed(Text::toT(i->second)));
	}

	auto queued = QueueManager::getInstance()->getQueued(user);

	if(queued.first) {
		userInfo->addRow(GridInfo());
		auto queuedGroup = userInfo->addChild(GroupBox::Seed(T_("Pending downloads information")));
		auto queuedGrid = queuedGroup->addChild(Grid::Seed(0, 2));

		queuedGrid->addRow(GridInfo());
		queuedGrid->addChild(Label::Seed(T_("Queued files")));
		queuedGrid->addChild(Label::Seed(Text::toT(Util::toString(queued.first))));

		queuedGrid->addRow(GridInfo());
		queuedGrid->addChild(Label::Seed(T_("Queued bytes")));
		queuedGrid->addChild(Label::Seed(Text::toT(Util::formatBytes(queued.second))));
	}

	auto files = UploadManager::getInstance()->getWaitingUserFiles(user);
	if(!files.empty()) {
		userInfo->addRow(GridInfo());
		auto uploadsGroup = userInfo->addChild(GroupBox::Seed(T_("Pending uploads information")));
		auto uploadsGrid = uploadsGroup->addChild(Grid::Seed(0, 2));

		for(auto i = files.begin(); i != files.end(); ++i) {
			uploadsGrid->addRow(GridInfo());
			uploadsGrid->addChild(Label::Seed(T_("Filename")));
			uploadsGrid->addChild(Label::Seed(Text::toT(*i)));
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
		menu->appendItem(T_("&Description"), [this] { handleDescription(); });
		menu->appendItem(T_("&Remove"), [this] { handleRemove(); });

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
		break;

	case COLUMN_SLOT:
		FavoriteManager::getInstance()->setAutoGrant(ui->getUser(), !ui->grantSlot);
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
		if(matches(i->second)) {
			i->second.update(i->first, true);
			users->insert(&i->second);
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

static bool isFav(const UserPtr &u) { return FavoriteManager::getInstance()->isFavoriteUser(u); }
static bool isWaiting(const UserPtr &u) { return UploadManager::getInstance()->isWaiting(u); }
static bool hasDownload(const UserPtr &u) { return QueueManager::getInstance()->getQueued(u).first > 0; }

bool UsersFrame::show(const UserPtr &u, bool any) const {
	if(any && (u->isOnline() || isFav(u) || isWaiting(u) || hasDownload(u))) {
		return true;
	}

	if(showOnline->getChecked() && !u->isOnline()) {
		return false;
	}

	if(showFavs->getChecked() && !isFav(u)) {
		return false;
	}

	if(showWaiting->getChecked() && !isWaiting(u)) {
		return false;
	}

	if(showQueue->getChecked() && !hasDownload(u)) {
		return false;
	}

	return true;
}

UsersFrame::UserInfoList UsersFrame::selectedUsersImpl() const {
	return usersFromTable(users);
}

void UsersFrame::on(UserAdded, const FavoriteUser& aUser) noexcept {
	auto u = aUser.getUser();
	callAsync([=] { addUser(u); });
}

void UsersFrame::on(FavoriteManagerListener::UserUpdated, const FavoriteUser& aUser) noexcept {
	auto u = aUser.getUser();
	callAsync([=] { updateUser(u); });
}
void UsersFrame::on(UserRemoved, const FavoriteUser& aUser) noexcept {
	auto u = aUser.getUser();
	callAsync([=] { updateUser(u); });
}

void UsersFrame::on(StatusChanged, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}

void UsersFrame::on(UserConnected, const UserPtr& aUser) noexcept {
	callAsync([=] { addUser(aUser); });
}

void UsersFrame::on(ClientManagerListener::UserUpdated, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}

void UsersFrame::on(UserDisconnected, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}

void UsersFrame::on(WaitingAddFile, const HintedUser& aUser, const string&) noexcept {
	callAsync([=] { addUser(aUser); });
}

void UsersFrame::on(WaitingRemoveUser, const HintedUser& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}

void UsersFrame::on(Added, QueueItem* qi) noexcept {
	for(auto i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		auto u = i->getUser().user;
		callAsync([=] { addUser(u); });
	}
}

void UsersFrame::on(SourcesUpdated, QueueItem* qi) noexcept {
	for(auto i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		auto u = i->getUser().user;
		callAsync([=] { updateUser(u); });
	}
}

void UsersFrame::on(Removed, QueueItem* qi) noexcept {
	for(auto i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		auto u = i->getUser().user;
		callAsync([=] { updateUser(u); });
	}
}
