/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#include "HubFrame.h"

#include "MainWindow.h"
#include "PrivateFrame.h"
#include "ParamDlg.h"
#include "HoldRedraw.h"

#include <dcpp/ChatMessage.h>
#include <dcpp/ClientManager.h>
#include <dcpp/Client.h>
#include <dcpp/LogManager.h>
#include <dcpp/User.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/ConnectionManager.h>
#include <dcpp/SearchManager.h>
#include <dcpp/version.h>
#include <dcpp/WindowInfo.h>

const string HubFrame::id = WindowManager::hub();
const string& HubFrame::getId() const { return id; }

static const ColumnInfo usersColumns[] = {
	{ N_("Nick"), 100, false },
	{ N_("Shared"), 80, true },
	{ N_("Description"), 75, false },
	{ N_("Tag"), 100, false },
	{ N_("Connection"), 75, false },
	{ N_("IP"), 100, false },
	{ N_("E-Mail"), 100, false },
	{ N_("CID"), 300, false}
};

HubFrame::FrameList HubFrame::frames;

void HubFrame::openWindow(dwt::TabView* mdiParent, const string& url) {
	for(FrameIter i = frames.begin(); i!= frames.end(); ++i) {
		HubFrame* frame = *i;
		if(frame->url == url) {
			frame->activate();
			return;
		}
	}

	new HubFrame(mdiParent, url);
}

void HubFrame::closeDisconnected() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i) {
		HubFrame* frame = *i;
		if(!(frame->client->isConnected())) {
			frame->close(true);
		}
	}
}

void HubFrame::closeFavGroup(const string& group, bool reversed) {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i) {
		HubFrame* frame = *i;
		FavoriteHubEntry* fav = FavoriteManager::getInstance()->getFavoriteHubEntry(frame->url);
		if((fav && fav->getGroup() == group) ^ reversed) {
			frame->close(true);
		}
	}
}

void HubFrame::resortUsers() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		(*i)->resortForFavsFirst(true);
}

const StringMap HubFrame::getWindowParams() const {
	StringMap ret;
	ret[WindowInfo::title] = Text::fromT(getText());
	ret[WindowInfo::address] = url;
	return ret;
}

void HubFrame::parseWindowParams(dwt::TabView* parent, const StringMap& params) {
	StringMap::const_iterator i = params.find(WindowInfo::address);
	if(i != params.end())
		openWindow(parent, i->second);
}

bool HubFrame::isFavorite(const StringMap& params) {
	StringMap::const_iterator i = params.find(WindowInfo::address);
	if(i != params.end())
		return FavoriteManager::getInstance()->isFavoriteHub(i->second);
	return false;
}

HubFrame::HubFrame(dwt::TabView* mdiParent, const string& url_) :
BaseType(mdiParent, Text::toT(url_), IDH_HUB, IDR_HUB_OFF, false),
paned(0),
userGrid(0),
users(0),
filter(0),
filterType(0),
showUsers(0),
client(0),
url(url_),
updateUsers(false),
waitingForPW(false),
resort(false),
showJoins(BOOLSETTING(SHOW_JOINS)),
favShowJoins(BOOLSETTING(FAV_SHOW_JOINS)),
currentUser(0),
inTabMenu(false),
inTabComplete(false)
{
	paned = addChild(WidgetVPaned::Seed(0.7));

	chat->setHelpId(IDH_HUB_CHAT);
	addWidget(chat);
	paned->setFirst(chat);
	chat->onContextMenu(std::tr1::bind(&HubFrame::handleChatContextMenu, this, _1));

	message->setHelpId(IDH_HUB_MESSAGE);
	addWidget(message, true, false);
	message->onKeyDown(std::tr1::bind(&HubFrame::handleMessageKeyDown, this, _1));
	message->onSysKeyDown(std::tr1::bind(&HubFrame::handleMessageKeyDown, this, _1));
	message->onChar(std::tr1::bind(&HubFrame::handleMessageChar, this, _1));

	{
		userGrid = addChild(Grid::Seed(2, 2));
		userGrid->column(0).mode = GridInfo::FILL;
		userGrid->row(0).mode = GridInfo::FILL;
		userGrid->row(0).align = GridInfo::STRETCH;
		paned->setSecond(userGrid);

		users = userGrid->addChild(WidgetUsers::Seed());
		userGrid->setWidget(users, 0, 0, 1, 2);
		addWidget(users);

		users->setSmallImageList(WinUtil::userImages);
		WinUtil::makeColumns(users, usersColumns, COLUMN_LAST, SETTING(HUBFRAME_ORDER), SETTING(HUBFRAME_WIDTHS));
		users->setSort(COLUMN_NICK);

		users->onSelectionChanged(std::tr1::bind(&HubFrame::callAsync, this,
			dwt::Application::Callback(std::tr1::bind(&HubFrame::updateStatus, this))));
		users->onDblClicked(std::tr1::bind(&HubFrame::handleDoubleClickUsers, this));
		users->onKeyDown(std::tr1::bind(&HubFrame::handleUsersKeyDown, this, _1));
		users->onContextMenu(std::tr1::bind(&HubFrame::handleUsersContextMenu, this, _1));

		TextBox::Seed cs = WinUtil::Seeds::textBox;
		cs.style |= ES_AUTOHSCROLL;
		filter = userGrid->addChild(cs);
		filter->setHelpId(IDH_HUB_FILTER);
		addWidget(filter);
		filter->onUpdated(std::tr1::bind(&HubFrame::handleFilterUpdated, this));

		filterType = userGrid->addChild(WinUtil::Seeds::comboBoxStatic);
		filterType->setHelpId(IDH_HUB_FILTER);
		addWidget(filterType);

		for(int j=0; j<COLUMN_LAST; j++) {
			filterType->addValue(T_(usersColumns[j].name));
		}
		filterType->addValue(T_("Any"));
		filterType->setSelected(COLUMN_LAST);
		filterType->onSelectionChanged(std::tr1::bind(&HubFrame::updateUserList, this, (UserInfo*)0));
	}

	showUsers = addChild(WinUtil::Seeds::splitCheckBox);
	showUsers->setHelpId(IDH_HUB_SHOW_USERS);
	showUsers->setChecked(BOOLSETTING(GET_USER_INFO));

	initStatus();
	status->setSize(STATUS_SHOW_USERS, showUsers->getPreferedSize().x);
	status->onDblClicked(STATUS_STATUS, std::tr1::bind(&HubFrame::openLog, this, false));

	status->setHelpId(STATUS_STATUS, IDH_HUB_STATUS);
	status->setHelpId(STATUS_USERS, IDH_HUB_USERS_COUNT);
	status->setHelpId(STATUS_SHARED, IDH_HUB_SHARED);
	status->setHelpId(STATUS_AVERAGE_SHARED, IDH_HUB_AVERAGE_SHARED);

	addAccel(FALT, 'G', std::tr1::bind(&HubFrame::handleGetList, this));
	addAccel(FCONTROL, 'R', std::tr1::bind(&HubFrame::handleReconnect, this));
	addAccel(FCONTROL, 'T', std::tr1::bind(&HubFrame::handleFollow, this));
	addAccel(FALT, 'P', std::tr1::bind(&HubFrame::handlePrivateMessage, this, getParent()));
	addAccel(FALT, 'U', std::tr1::bind(&dwt::Control::setFocus, users));
	initAccels();

	layout();
	activate();

	initSecond();

	client = ClientManager::getInstance()->getClient(url);
	client->addListener(this);
	client->connect();

	readLog(getLogPath(), SETTING(HUB_LAST_LOG_LINES));

	frames.push_back(this);

	showUsers->onClicked(std::tr1::bind(&HubFrame::handleShowUsersClicked, this));

	FavoriteManager::getInstance()->addListener(this);

	addRecent();
}

HubFrame::~HubFrame() {
	ClientManager::getInstance()->putClient(client);
	frames.erase(std::remove(frames.begin(), frames.end(), this), frames.end());
	clearTaskList();
}

bool HubFrame::preClosing() {
	if(BOOLSETTING(CONFIRM_HUB_CLOSING) && !WinUtil::mainWindow->closing() &&
		dwt::MessageBox(this).show(getText() + _T("\n\n") + T_("Really close?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDNO)
		return false;

	FavoriteManager::getInstance()->removeListener(this);
	client->removeListener(this);
	client->disconnect(true);
	return true;
}

void HubFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, showUsers->getChecked());

	clearUserList();
	clearTaskList();

	SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_ORDER, WinUtil::toString(users->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_WIDTHS, WinUtil::toString(users->getColumnWidths()));
}

void HubFrame::layout() {
	bool scroll = chat->scrollIsAtEnd();

	const int border = 2;

	dwt::Rectangle r(getClientSize());

	status->layout(r);
	status->mapWidget(STATUS_SHOW_USERS, showUsers);

	int ymessage = message->getTextSize(_T("A")).y * messageLines + 10;
	dwt::Rectangle rm(0, r.size.y - ymessage, r.width(), ymessage);
	message->layout(rm);

	r.size.y -= rm.size.y + border;

	bool checked = showUsers->getChecked();
	if(checked && !paned->getSecond()) {
		paned->setSecond(userGrid);
	} else if(!checked && paned->getSecond()) {
		paned->setSecond(0);
	}
	paned->setRect(r);

	if(scroll)
		chat->sendMessage(WM_VSCROLL, SB_BOTTOM);
}

void HubFrame::updateStatus() {
	status->setText(STATUS_USERS, getStatusUsers());
	status->setText(STATUS_SHARED, getStatusShared());
	status->setText(STATUS_AVERAGE_SHARED, getStatusAverageShared());
}

void HubFrame::initSecond() {
	createTimer(std::tr1::bind(&HubFrame::eachSecond, this), 1000);
}

bool HubFrame::eachSecond() {
	if(updateUsers) {
		updateUsers = false;
		callAsync(std::tr1::bind(&HubFrame::execTasks, this));
	}

	updateStatus();
	return true;
}

void HubFrame::enterImpl(const tstring& s) {
	// Special command
	if(s[0] == _T('/')) {
		tstring cmd = s;
		tstring param;
		tstring msg;
		tstring status;
		bool thirdPerson = false;
		if(WinUtil::checkCommand(cmd, param, msg, status, thirdPerson)) {
			if(!msg.empty()) {
				client->hubMessage(Text::fromT(msg), thirdPerson);
			}
			if(!status.empty()) {
				addStatus(status);
			}
		} else if(ChatType::checkCommand(cmd, param, status)) {
			if(!status.empty()) {
				addStatus(status);
			}
		} else if(Util::stricmp(cmd.c_str(), _T("join"))==0) {
			if(!param.empty()) {
				redirect = Text::fromT(param);
				if(BOOLSETTING(JOIN_OPEN_NEW_WINDOW)) {
					HubFrame::openWindow(getParent(), Text::fromT(param));
				} else {
					handleFollow();
				}
			} else {
				addStatus(T_("Specify a server to connect to"));
			}
		} else if( (Util::stricmp(cmd.c_str(), _T("password")) == 0) && waitingForPW ) {
			client->setPassword(Text::fromT(param));
			client->password(Text::fromT(param));
			waitingForPW = false;
		} else if( Util::stricmp(cmd.c_str(), _T("showjoins")) == 0 ) {
			showJoins = !showJoins;
			if(showJoins) {
				addStatus(T_("Join/part showing on"));
			} else {
				addStatus(T_("Join/part showing off"));
			}
		} else if( Util::stricmp(cmd.c_str(), _T("favshowjoins")) == 0 ) {
			favShowJoins = !favShowJoins;
			if(favShowJoins) {
				addStatus(T_("Join/part of favorite users showing on"));
			} else {
				addStatus(T_("Join/part of favorite users showing off"));
			}
		} else if(Util::stricmp(cmd.c_str(), _T("close")) == 0) {
			this->close(true);
		} else if(Util::stricmp(cmd.c_str(), _T("userlist")) == 0) {
			showUsers->setChecked(!showUsers->getChecked());
		} else if(Util::stricmp(cmd.c_str(), _T("connection")) == 0) {
			addStatus(str(TF_("IP: %1%, Port: %2%/%3%/%4%") % Text::toT(client->getLocalIp())
				% ConnectionManager::getInstance()->getPort()
				% SearchManager::getInstance()->getPort()
				% ConnectionManager::getInstance()->getSecurePort()
				));
		} else if((Util::stricmp(cmd.c_str(), _T("favorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("fav")) == 0)) {
			addAsFavorite();
		} else if((Util::stricmp(cmd.c_str(), _T("removefavorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("removefav")) == 0)) {
			removeFavoriteHub();
		} else if(Util::stricmp(cmd.c_str(), _T("getlist")) == 0){
			if( !param.empty() ){
				UserInfo* ui = findUser(param);
				if(ui) {
					ui->getList();
				}
			}
		} else if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
			if(param.empty())
				openLog();
			else if(Util::stricmp(param.c_str(), _T("status")) == 0)
				openLog(true);
		} else if(Util::stricmp(cmd.c_str(), _T("f")) == 0) {
			if(param.empty())
				param = chat->findTextPopup();

			chat->findText(param);
		} else if(Util::stricmp(cmd.c_str(), _T("help")) == 0) {
			addChat(_T("*** ") + WinUtil::commands + _T(", /join <hub-ip>, /showjoins, /favshowjoins, /close, /userlist, /connection, /favorite, /pm <user> [message], /getlist <user>, /log <status, system, downloads, uploads>, /removefavorite, /f <text-to-find>"));
		} else if(Util::stricmp(cmd.c_str(), _T("pm")) == 0) {
			string::size_type j = param.find(_T(' '));
			if(j != string::npos) {
				tstring nick = param.substr(0, j);
				UserInfo* ui = findUser(nick);

				if(ui) {
					if(param.size() > j + 1)
						PrivateFrame::openWindow(getParent(), HintedUser(ui->getUser(), url), param.substr(j+1));
					else
						PrivateFrame::openWindow(getParent(), HintedUser(ui->getUser(), url), Util::emptyStringT);
				}
			} else if(!param.empty()) {
				UserInfo* ui = findUser(param);
				if(ui) {
					PrivateFrame::openWindow(getParent(), HintedUser(ui->getUser(), url), Util::emptyStringT);
				}
			}
		} else {
			if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
				client->hubMessage(Text::fromT(s));
			} else {
				addStatus(str(TF_("Unknown command: %1%") % cmd));
			}
		}
		message->setText(_T(""));
	} else if(waitingForPW) {
		addStatus(T_("Don't remove /password before your password"));
		message->setText(_T("/password "));
		message->setFocus();
		message->setSelection(10, 10);
	} else {
		client->hubMessage(Text::fromT(s));
		message->setText(_T(""));
	}
}

void HubFrame::clearUserList() {
	users->clear();
	for(UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
		delete i->second;
	}
	userMap.clear();
}

void HubFrame::clearTaskList() {
	tasks.clear();
}

void HubFrame::addChat(const tstring& aLine) {
	ChatType::addChat(client, aLine);

	WinUtil::playSound(SettingsManager::SOUND_MAIN_CHAT);
	setDirty(SettingsManager::BOLD_HUB);

	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(aLine);
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}
}

void HubFrame::addStatus(const tstring& aLine, bool legitimate /* = true */) {
	tstring line = Text::toT("[" + Util::getShortTimeString() + "] ") + aLine;

	status->setText(STATUS_STATUS, line);

	if(legitimate) {
		if(BOOLSETTING(STATUS_IN_CHAT))
			addChat(_T("*** ") + aLine);
		else
			setDirty(SettingsManager::BOLD_HUB);
	}

	if(BOOLSETTING(LOG_STATUS_MESSAGES)) {
		StringMap params;
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		params["message"] = Text::fromT(aLine);
		LOG(LogManager::STATUS, params);
	}
}

void HubFrame::addTask(Tasks s, const OnlineUser& u) {
	tasks.add(s, new UserTask(u));
	updateUsers = true;
}

void HubFrame::execTasks() {
	updateUsers = false;
	TaskQueue::List t;
	tasks.get(t);

	HoldRedraw hold(users);

	for(TaskQueue::Iter i = t.begin(); i != t.end(); ++i) {
		if(i->first == UPDATE_USER) {
			updateUser(*static_cast<UserTask*>(i->second));
		} else if(i->first == UPDATE_USER_JOIN) {
			UserTask& u = *static_cast<UserTask*>(i->second);
			if(updateUser(u)) {
				if (showJoins || (favShowJoins && FavoriteManager::getInstance()->isFavoriteUser(u.user))) {
					addStatus(str(TF_("*** Joins: %1%") % Text::toT(u.identity.getNick())));
				}
			}
		} else if(i->first == REMOVE_USER) {
			UserTask& u = *static_cast<UserTask*>(i->second);
			removeUser(u.user);
			if (showJoins || (favShowJoins && FavoriteManager::getInstance()->isFavoriteUser(u.user))) {
				addStatus(str(TF_("*** Parts: %1%") % Text::toT(u.identity.getNick())));
			}
		}
		delete i->second;
	}
	if(resort && showUsers->getChecked()) {
		users->resort();
		resort = false;
	}
}

void HubFrame::onConnected() {
	addStatus(T_("Connected"));
	setIcon(IDR_HUB);
	status->setText(STATUS_CIPHER, Text::toT(client->getCipherName()));
}

void HubFrame::onDisconnected() {
	clearUserList();
	clearTaskList();
	setIcon(IDR_HUB_OFF);
}

void HubFrame::onGetPassword() {
	if(client->getPassword().size() > 0) {
		client->password(client->getPassword());
		addStatus(T_("Stored password sent..."));
	} else {
		if(!BOOLSETTING(PROMPT_PASSWORD)) {
			message->setText(_T("/password "));
			message->setFocus();
			message->setSelection(10, 10);
			waitingForPW = true;
		} else {
			ParamDlg linePwd(this, T_("Please enter a password"), T_("Please enter a password"), Util::emptyStringT, true);
			if(linePwd.run() == IDOK) {
				client->setPassword(Text::fromT(linePwd.getValue()));
				client->password(Text::fromT(linePwd.getValue()));
				waitingForPW = false;
			} else {
				client->disconnect(true);
			}
		}
	}
}

void HubFrame::onPrivateMessage(const UserPtr& from, const UserPtr& to, const UserPtr& replyTo, bool hub, bool bot, const tstring& m) {
	bool ignore = false, window = false;
	if(hub) {
		if(BOOLSETTING(IGNORE_HUB_PMS)) {
			ignore = true;
		} else if(BOOLSETTING(POPUP_HUB_PMS) || PrivateFrame::isOpen(replyTo)) {
			window = true;
		}
	} else if(bot) {
		if(BOOLSETTING(IGNORE_BOT_PMS)) {
			ignore = true;
		} else if(BOOLSETTING(POPUP_BOT_PMS) || PrivateFrame::isOpen(replyTo)) {
			window = true;
		}
	} else if(BOOLSETTING(POPUP_PMS) || PrivateFrame::isOpen(replyTo) || from == client->getMyIdentity().getUser()) {
		window = true;
	}

	if(ignore) {
		addStatus(str(TF_("Ignored message: %1%") % m), false);
	} else {
		if(window) {
			PrivateFrame::gotMessage(getParent(), from, to, replyTo, m, url);
		} else {
			addChat(str(TF_("Private message from %1%: %2%") % getNick(from) % m));
		}
		WinUtil::mainWindow->TrayPM();
	}
}

HubFrame::UserInfo* HubFrame::findUser(const tstring& nick) {
	for(UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
		if(i->second->columns[COLUMN_NICK] == nick)
			return i->second;
	}
	return 0;
}

const tstring& HubFrame::getNick(const UserPtr& aUser) {
	UserMapIter i = userMap.find(aUser);
	if(i == userMap.end())
		return Util::emptyStringT;

	UserInfo* ui = i->second;
	return ui->columns[COLUMN_NICK];
}

bool HubFrame::updateUser(const UserTask& u) {
	UserMapIter i = userMap.find(u.user);
	if(i == userMap.end()) {
		UserInfo* ui = new UserInfo(u);
		userMap.insert(make_pair(u.user, ui));
		if(!ui->isHidden() && showUsers->getChecked())
			users->insert(ui);

		if(!filterString.empty())
			updateUserList(ui);
		return true;
	} else {
		UserInfo* ui = i->second;
		if(!ui->isHidden() && u.identity.isHidden() && showUsers->getChecked()) {
			users->erase(ui);
		}

		resort = ui->update(u.identity, users->getSortColumn()) || resort;
		if(showUsers->getChecked()) {
			int pos = users->find(ui);
			if(pos != -1) {
				users->update(pos);
			}
			updateUserList(ui);
		}

		return false;
	}
}

bool HubFrame::UserInfo::update(const Identity& identity, int sortCol) {
	bool needsSort = (getIdentity().isOp() != identity.isOp());
	tstring old;
	if(sortCol != -1)
		old = columns[sortCol];

	columns[COLUMN_NICK] = Text::toT(identity.getNick());
	columns[COLUMN_SHARED] = Text::toT(Util::formatBytes(identity.getBytesShared()));
	columns[COLUMN_DESCRIPTION] = Text::toT(identity.getDescription());
	columns[COLUMN_TAG] = Text::toT(identity.getTag());
	columns[COLUMN_CONNECTION] = Text::toT(identity.getConnection());
	string ip = identity.getIp();
	string country = ip.empty()?Util::emptyString:Util::getIpCountry(ip);
	if (!country.empty())
		ip = country + " (" + ip + ")";
	columns[COLUMN_IP] = Text::toT(ip);
	columns[COLUMN_EMAIL] = Text::toT(identity.getEmail());
	columns[COLUMN_CID] = Text::toT(identity.getUser()->getCID().toBase32());

	if(sortCol != -1) {
		needsSort = needsSort || (old != columns[sortCol]);
	}

	setIdentity(identity);
	return needsSort;
}

void HubFrame::removeUser(const UserPtr& aUser) {
	UserMapIter i = userMap.find(aUser);
	if(i == userMap.end()) {
		// Should never happen?
		dcassert(i != userMap.end());
		return;
	}

	UserInfo* ui = i->second;
	if(!ui->isHidden() && showUsers->getChecked())
		users->erase(ui);

	userMap.erase(i);
	delete ui;
}

bool HubFrame::handleUsersKeyDown(int c) {
	if(c == VK_RETURN && users->hasSelected()) {
		handleGetList();
		return true;
	}
	return false;
}

bool HubFrame::handleMessageChar(int c) {
	switch(c) {
	case VK_TAB: return true; break;
	}

	return ChatType::handleMessageChar(c);
}

bool HubFrame::handleMessageKeyDown(int c) {
	if(!complete.empty() && c != VK_TAB)
		complete.clear(), inTabComplete = false;

	switch(c) {
	case VK_TAB:
		if(tab())
			return true;
		break;
	}

	return ChatType::handleMessageKeyDown(c);
}

int HubFrame::UserInfo::getImage() const {
	int image = identity.isOp() ? IMAGE_OP : IMAGE_USER;

	if(identity.getUser()->isSet(User::DCPLUSPLUS))
		image+=2;
	if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_PASSIVE && !identity.isTcpActive()) {
		// Users we can't connect to...
		image+=4;
	}
	return image;
}

HubFrame::UserTask::UserTask(const OnlineUser& ou) :
user(ou.getUser(), ou.getClient().getHubUrl()),
identity(ou.getIdentity())
{
}

int HubFrame::UserInfo::compareItems(const HubFrame::UserInfo* a, const HubFrame::UserInfo* b, int col) {
	if(col == COLUMN_NICK) {
		bool a_isOp = a->getIdentity().isOp(),
			b_isOp = b->getIdentity().isOp();
		if(a_isOp && !b_isOp)
			return -1;
		if(!a_isOp && b_isOp)
			return 1;
		if(BOOLSETTING(SORT_FAVUSERS_FIRST)) {
			bool a_isFav = FavoriteManager::getInstance()->isFavoriteUser(a->getIdentity().getUser()),
				b_isFav = FavoriteManager::getInstance()->isFavoriteUser(b->getIdentity().getUser());
			if(a_isFav && !b_isFav)
				return -1;
			if(!a_isFav && b_isFav)
				return 1;
		}
	}
	if(col == COLUMN_SHARED) {
		return compare(a->identity.getBytesShared(), b->identity.getBytesShared());;
	}
	return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
}

void HubFrame::on(Connecting, Client*) throw() {
	callAsync(std::tr1::bind(&HubFrame::addStatus, this, str(TF_("Connecting to %1%...") % Text::toT(client->getHubUrl())), true));
	callAsync(std::tr1::bind(&RecentType::setText, this, Text::toT(client->getHubUrl())));
}
void HubFrame::on(Connected, Client*) throw() {
	callAsync(std::tr1::bind(&HubFrame::onConnected, this));
}
void HubFrame::on(UserUpdated, Client*, const OnlineUser& user) throw() {
	addTask(UPDATE_USER_JOIN, user);
}
void HubFrame::on(UsersUpdated, Client*, const OnlineUserList& aList) throw() {
	for(OnlineUserList::const_iterator i = aList.begin(); i != aList.end(); ++i) {
		tasks.add(UPDATE_USER, new UserTask(*(*i)));
	}
	updateUsers = true;
}

void HubFrame::on(ClientListener::UserRemoved, Client*, const OnlineUser& user) throw() {
	addTask(REMOVE_USER, user);
}

void HubFrame::on(Redirect, Client*, const string& line) throw() {
	if(ClientManager::getInstance()->isConnected(line)) {
		callAsync(std::tr1::bind(&HubFrame::addStatus, this, T_("Redirect request received to a hub that's already connected"), true));
		return;
	}
	redirect = line;
	if(BOOLSETTING(AUTO_FOLLOW)) {
		callAsync(std::tr1::bind(&HubFrame::handleFollow, this));
	} else {
		callAsync(std::tr1::bind(&HubFrame::addStatus, this, str(TF_("Press the follow redirect button to connect to %1%") % Text::toT(line)), true));
	}
}

void HubFrame::on(Failed, Client*, const string& line) throw() {
	callAsync(std::tr1::bind(&HubFrame::addStatus, this, Text::toT(line), true));
	callAsync(std::tr1::bind(&HubFrame::onDisconnected, this));
}

void HubFrame::on(GetPassword, Client*) throw() {
	callAsync(std::tr1::bind(&HubFrame::onGetPassword, this));
}

void HubFrame::on(HubUpdated, Client*) throw() {
	string hubName;
	if(client->isTrusted()) {
		hubName = "[S] ";
	} else if(client->isSecure()) {
		hubName = "[U] ";
	}

	hubName += client->getHubName();

	if(!client->getHubDescription().empty()) {
		hubName += " - " + client->getHubDescription();
	}
	hubName += " (" + client->getHubUrl() + ")";
#ifdef _DEBUG
	string version = client->getHubIdentity().get("VE");
	if(!version.empty()) {
		hubName += " - " + version;
	}
#endif
	callAsync(std::tr1::bind(&RecentType::setText, this, Text::toT(hubName)));
}

void HubFrame::on(Message, Client*, const ChatMessage& message) throw() {
	if(message.to && message.replyTo) {
		callAsync(std::tr1::bind(&HubFrame::onPrivateMessage, this,
			message.from->getUser(), message.to->getUser(), message.replyTo->getUser(),
			message.replyTo->getIdentity().isHub(), message.replyTo->getIdentity().isBot(),
			Text::toT(message.format())));
	} else {
		callAsync(std::tr1::bind(&HubFrame::addChat, this, Text::toT(message.format())));
	}
}

void HubFrame::on(StatusMessage, Client*, const string& line, int statusFlags) throw() {
	onStatusMessage(line, statusFlags);
}

void HubFrame::on(NickTaken, Client*) throw() {
	callAsync(std::tr1::bind(&HubFrame::addStatus, this, T_("Your nick was already taken, please change to something else!"), true));
}

void HubFrame::on(SearchFlood, Client*, const string& line) throw() {
	onStatusMessage(str(F_("Search spam detected from %1%") % line), ClientListener::FLAG_IS_SPAM);
}

void HubFrame::onStatusMessage(const string& line, int flags) {
	callAsync(std::tr1::bind(&HubFrame::addStatus, this, Text::toT(line),
		!(flags & ClientListener::FLAG_IS_SPAM) || !BOOLSETTING(FILTER_MESSAGES)));
}

tstring HubFrame::getStatusShared() const {
	int64_t available;
	if (users->countSelected() > 1) {
		available = users->forEachSelectedT(CountAvailable()).available;
	} else {
		available = std::for_each(userMap.begin(), userMap.end(), CountAvailable()).available;
	}
	return Text::toT(Util::formatBytes(available));
}

tstring HubFrame::getStatusUsers() const {
	size_t userCount = 0;
	for(UserMap::const_iterator i = userMap.begin(); i != userMap.end(); ++i){
		UserInfo* ui = i->second;
		if(!ui->isHidden())
			userCount++;
	}

	tstring textForUsers;
	if (users->countSelected() > 1)
		textForUsers += Text::toT(Util::toString(users->countSelected()) + "/");
	if (showUsers->getChecked() && users->size() < userCount)
		textForUsers += Text::toT(Util::toString(users->size()) + "/");
	return textForUsers + str(TFN_("%1% user", "%1% users", userCount) % userCount);
}

tstring HubFrame::getStatusAverageShared() const {
	int64_t available;
	size_t userCount = 0;
	if (users->countSelected() > 1) {
		available = users->forEachSelectedT(CountAvailable()).available;
		userCount = users->countSelected();
	} else {
		available = std::for_each(userMap.begin(), userMap.end(), CountAvailable()).available;
		for(UserMap::const_iterator i = userMap.begin(); i != userMap.end(); ++i){
			UserInfo* ui = i->second;
			if(!ui->isHidden())
				userCount++;
		}
	}

	return str(TF_("Average: %1%") % Text::toT(Util::formatBytes(userCount > 0 ? available / userCount : 0)));
}

void HubFrame::on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) throw() {
	resortForFavsFirst();
}
void HubFrame::on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) throw() {
	resortForFavsFirst();
}

void HubFrame::resortForFavsFirst(bool justDoIt /* = false */) {
	if(justDoIt || BOOLSETTING(SORT_FAVUSERS_FIRST)) {
		resort = true;
		callAsync(std::tr1::bind(&HubFrame::execTasks, this));
	}
}

void HubFrame::addAsFavorite() {
	FavoriteHubEntry* existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
	if(!existingHub) {
		FavoriteHubEntry aEntry;
		aEntry.setServer(url);
		aEntry.setName(client->getHubName());
		aEntry.setDescription(client->getHubDescription());
		aEntry.setNick(client->getMyNick());
		FavoriteManager::getInstance()->addFavorite(aEntry);
		addStatus(T_("Favorite hub added"));
	} else {
		addStatus(T_("Hub already exists as a favorite"));
	}
}

void HubFrame::removeFavoriteHub() {
	FavoriteHubEntry* removeHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
	if(removeHub) {
		FavoriteManager::getInstance()->removeFavorite(removeHub);
		addStatus(T_("Favorite hub removed"));
	} else {
		addStatus(T_("This hub is not a favorite hub"));
	}
}

bool HubFrame::parseFilter(FilterModes& mode, int64_t& size) {
	tstring::size_type start = static_cast<tstring::size_type>(tstring::npos);
	tstring::size_type end = static_cast<tstring::size_type>(tstring::npos);
	int64_t multiplier = 1;

	if(filterString.empty()) {
		return false;
	}
	if(filterString.compare(0, 2, _T(">=")) == 0) {
		mode = GREATER_EQUAL;
		start = 2;
	} else if(filterString.compare(0, 2, _T("<=")) == 0) {
		mode = LESS_EQUAL;
		start = 2;
	} else if(filterString.compare(0, 2, _T("==")) == 0) {
		mode = EQUAL;
		start = 2;
	} else if(filterString.compare(0, 2, _T("!=")) == 0) {
		mode = NOT_EQUAL;
		start = 2;
	} else if(filterString[0] == _T('<')) {
		mode = LESS;
		start = 1;
	} else if(filterString[0] == _T('>')) {
		mode = GREATER;
		start = 1;
	} else if(filterString[0] == _T('=')) {
		mode = EQUAL;
		start = 1;
	}

	if(start == tstring::npos)
		return false;
	if(filterString.length() <= start)
		return false;

	if((end = Util::findSubString(filterString, _T("TiB"))) != tstring::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(filterString, _T("GiB"))) != tstring::npos) {
		multiplier = 1024*1024*1024;
	} else if((end = Util::findSubString(filterString, _T("MiB"))) != tstring::npos) {
		multiplier = 1024*1024;
	} else if((end = Util::findSubString(filterString, _T("KiB"))) != tstring::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(filterString, _T("TB"))) != tstring::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(filterString, _T("GB"))) != tstring::npos) {
		multiplier = 1000*1000*1000;
	} else if((end = Util::findSubString(filterString, _T("MB"))) != tstring::npos) {
		multiplier = 1000*1000;
	} else if((end = Util::findSubString(filterString, _T("kB"))) != tstring::npos) {
		multiplier = 1000;
	} else if((end = Util::findSubString(filterString, _T("B"))) != tstring::npos) {
		multiplier = 1;
	}

	if(end == tstring::npos) {
		end = filterString.length();
	}

	tstring tmpSize = filterString.substr(start, end-start);
	size = static_cast<int64_t>(Util::toDouble(Text::fromT(tmpSize)) * multiplier);

	return true;
}

void HubFrame::updateUserList(UserInfo* ui) {
	int64_t size = -1;
	FilterModes mode = NONE;

	int sel = filterType->getSelected();

	bool doSizeCompare = parseFilter(mode, size) && sel == COLUMN_SHARED;

	//single update?
	//avoid refreshing the whole list and just update the current item
	//instead
	if(ui != NULL) {
		if(ui->isHidden()) {
			return;
		}
		if(filterString.empty()) {
			if(users->find(ui) == -1) {
				users->insert(ui);
			}
		} else {
			if(matchFilter(*ui, sel, doSizeCompare, mode, size)) {
				if(users->find(ui) == -1) {
					users->insert(ui);
				}
			} else {
				//erase checks to see that the item exists in the list
				//unnecessary to do it twice.
				users->erase(ui);
			}
		}
	} else {
		HoldRedraw hold(users);
		users->clear();

		if(filterString.empty()) {
			for(UserMapIter i = userMap.begin(); i != userMap.end(); ++i){
				UserInfo* ui = i->second;
				if(!ui->isHidden())
					users->insert(i->second);
			}
		} else {
			for(UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
				UserInfo* ui = i->second;
				if(!ui->isHidden() && matchFilter(*ui, sel, doSizeCompare, mode, size)) {
					users->insert(ui);
				}
			}
		}
	}
}

bool HubFrame::matchFilter(const UserInfo& ui, int sel, bool doSizeCompare, FilterModes mode, int64_t size) {
	if(filterString.empty())
		return true;

	bool insert = false;
	if(doSizeCompare) {
		switch(mode) {
			case EQUAL: insert = (size == ui.getIdentity().getBytesShared()); break;
			case GREATER_EQUAL: insert = (size <= ui.getIdentity().getBytesShared()); break;
			case LESS_EQUAL: insert = (size >= ui.getIdentity().getBytesShared()); break;
			case GREATER: insert = (size < ui.getIdentity().getBytesShared()); break;
			case LESS: insert = (size > ui.getIdentity().getBytesShared()); break;
			case NOT_EQUAL: insert = (size != ui.getIdentity().getBytesShared()); break;
			case NONE: ; break;
		}
	} else {
		if(sel >= COLUMN_LAST) {
			for(int i = COLUMN_FIRST; i < COLUMN_LAST; ++i) {
				if(Util::findSubString(ui.getText(i), filterString) != string::npos) {
					insert = true;
					break;
				}
			}
		} else {
			if(Util::findSubString(ui.getText(sel), filterString) != string::npos)
				insert = true;
		}
	}

	return insert;
}

bool HubFrame::handleChatContextMenu(dwt::ScreenCoordinate pt) {
	tstring txt = chat->textUnderCursor(pt);
	if(txt.empty())
		return false;

	// Possible nickname click, let's see if we can find one like it in the name list...
	/// @todo make this work with shift+F10/etc
	if(showUsers->getChecked()) {
		int pos = users->find(txt);
		if(pos == -1)
			return false;
		users->clearSelection();
		users->setSelected(pos);
		users->ensureVisible(pos);
	} else if(!(currentUser = findUser(txt))) {
		return false;
	}

	if(pt.x() == -1 || pt.y() == -1) {
		pt = chat->getContextMenuPos();
	}
	return handleUsersContextMenu(pt);
}

bool HubFrame::handleUsersContextMenu(dwt::ScreenCoordinate pt) {
	if(showUsers->getChecked() ? users->hasSelected() : (currentUser != 0)) {
		if(pt.x() == -1 || pt.y() == -1) {
			pt = users->getContextMenuPos();
		}

		MenuPtr menu = addChild(WinUtil::Seeds::menu);
		appendUserItems(getParent(), menu);

		menu->appendSeparator();
		MenuPtr copyMenu = menu->appendPopup(T_("&Copy"));
		for(int j=0; j<COLUMN_LAST; j++) {
			copyMenu->appendItem(T_(usersColumns[j].name), std::tr1::bind(&HubFrame::handleMultiCopy, this, j));
		}
		prepareMenu(menu, UserCommand::CONTEXT_CHAT, client->getHubUrl());

		inTabMenu = false;

		menu->open(pt);
		return true;
	}
	return false;
}

void HubFrame::tabMenuImpl(dwt::MenuPtr& menu) {
	if(!FavoriteManager::getInstance()->isFavoriteHub(url)) {
		menu->appendItem(T_("Add To &Favorites"), std::tr1::bind(&HubFrame::addAsFavorite, this), dwt::IconPtr(new dwt::Icon(IDR_FAVORITE_HUBS)));
	}

	menu->appendItem(T_("&Reconnect\tCtrl+R"), std::tr1::bind(&HubFrame::handleReconnect, this), dwt::IconPtr(new dwt::Icon(IDR_RECONNECT)));
	menu->appendItem(T_("Copy &address to clipboard"), std::tr1::bind(&HubFrame::handleCopyHub, this));

	prepareMenu(menu, UserCommand::CONTEXT_HUB, url);

	menu->appendSeparator();

	inTabMenu = true;
}

void HubFrame::handleShowUsersClicked() {
	bool checked = showUsers->getChecked();

	userGrid->setVisible(checked);
	paned->setVisible(checked);

	if(checked) {
		updateUserList();
		currentUser = 0;
	} else {
		users->clear();
	}

	SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, checked);

	layout();
}

void HubFrame::handleMultiCopy(unsigned index) {
	if(index > COLUMN_LAST) {
		return;
	}

	UserInfoList sel = selectedUsersImpl();
	tstring tmpstr;
	for(UserInfoList::const_iterator i = sel.begin(), iend = sel.end(); i != iend; ++i) {
		tmpstr += static_cast<UserInfo*>(*i)->getText(index);
		tmpstr += _T(" / ");
	}
	if(!tmpstr.empty()) {
		// remove last space
		tmpstr.erase(tmpstr.length() - 3);
		WinUtil::setClipboard(tmpstr);
	}
}

void HubFrame::handleCopyHub() {
	WinUtil::setClipboard(Text::toT(url));
}

void HubFrame::handleDoubleClickUsers() {
	if(users->hasSelected()) {
		users->getSelectedData()->getList();
	}
}

void HubFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	// imitate ClientManager::userCommand, except some params are cached for multiple selections

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

	if(inTabMenu) {
		client->escapeParams(ucParams);
		client->sendUserCmd(Util::formatParams(uc.getCommand(), ucParams, false));
	} else {
		UserInfoList sel = selectedUsersImpl();
		for(UserInfoList::const_iterator i = sel.begin(), iend = sel.end(); i != iend; ++i) {
			StringMap tmp = ucParams;
			static_cast<UserInfo*>(*i)->getIdentity().getParams(tmp, "user", true);
			client->escapeParams(tmp);
			client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp, false));
		}
	}
}

string HubFrame::getLogPath(bool status) const {
	StringMap params;
	params["hubNI"] = client->getHubName();
	params["hubURL"] = client->getHubUrl();
	params["myNI"] = client->getMyNick();
	return Util::validateFileName(LogManager::getInstance()->getPath(status ? LogManager::STATUS : LogManager::CHAT, params));
}

void HubFrame::openLog(bool status) {
	WinUtil::openFile(Text::toT(getLogPath(status)));
}

string HubFrame::stripNick(const string& nick) const {
	if (nick.substr(0, 1) != "[") return nick;
	string::size_type x = nick.find(']');
	string ret;
	// Avoid full deleting of [IMCOOL][CUSIHAVENOTHINGELSETHANBRACKETS]-type nicks
	if ((x != string::npos) && (nick.substr(x+1).length() > 0)) {
		ret = nick.substr(x+1);
	} else {
		ret = nick;
	}
	return ret;
}

static bool compareCharsNoCase(string::value_type a, string::value_type b) {
	return Text::toLower(a) == Text::toLower(b);
}

//Has fun side-effects. Otherwise needs reference arguments or multiple-return-values.
tstring HubFrame::scanNickPrefix(const tstring& prefixT) {
	string prefix = Text::fromT(prefixT), maxPrefix;
	tabCompleteNicks.clear();
	for (UserMap::const_iterator i = userMap.begin(); i != userMap.end(); ++i) {
		string prevNick, nick = i->second->getIdentity().getNick(), wholeNick = nick;

		do {
			string::size_type lp = prefix.size(), ln = nick.size();
			if ((ln >= lp) && (!Util::strnicmp(nick, prefix, lp))) {
				if (maxPrefix == Util::emptyString) maxPrefix = nick;	//ugly hack
				tabCompleteNicks.push_back(nick);
				tabCompleteNicks.push_back(wholeNick);
				maxPrefix = maxPrefix.substr(0, mismatch(maxPrefix.begin(),
					maxPrefix.begin()+min(maxPrefix.size(), nick.size()),
					nick.begin(), compareCharsNoCase).first - maxPrefix.begin());
			}

			prevNick = nick;
			nick = stripNick(nick);
		} while (prevNick != nick);
	}

	return Text::toT(maxPrefix);
}

bool HubFrame::tab() {
	if(message->length() == 0) {
		::SetFocus(::GetNextDlgTabItem(handle(), message->handle(), isShiftPressed()));
		return true;
	}

	HWND focus = GetFocus();
	if( (focus == message->handle()) && !isShiftPressed() )
	{
		tstring text = message->getText();
		string::size_type textStart = text.find_last_of(_T(" \n\t"));

		if(complete.empty()) {
			if(textStart != string::npos) {
				complete = text.substr(textStart + 1);
			} else {
				complete = text;
			}
			if(complete.empty()) {
				// Still empty, no text entered...
				return false;
			}
			users->clearSelection();
		}

		if(textStart == string::npos)
			textStart = 0;
		else
			textStart++;

		if (inTabComplete) {
			// Already pressed tab once. Output nick candidate list.
			tstring nicks;
			for (StringList::const_iterator i = tabCompleteNicks.begin(); i < tabCompleteNicks.end(); i+=2)
				nicks.append(Text::toT(*i + " "));
			addChat(nicks);
			inTabComplete = false;
		} else {
			// First tab. Maximally extend proposed nick.
			tstring nick = scanNickPrefix(complete);
			if (tabCompleteNicks.empty()) return true;

			// Maybe it found a unique match. If userlist showing, highlight.
			if(showUsers->getChecked() && tabCompleteNicks.size() == 2) {
				int i = users->find(Text::toT(tabCompleteNicks[1]));
				users->setSelected(i);
				users->ensureVisible(i);
			}

			message->setSelection(textStart, -1);

			// no shift, use partial nick when appropriate
			if(isShiftPressed()) {
				message->replaceSelection(nick);
			} else {
				message->replaceSelection(Text::toT(stripNick(Text::fromT(nick))));
			}

			inTabComplete = true;
			return true;
		}
	}
	return false;
}

void HubFrame::handleReconnect() {
	client->reconnect();
}

void HubFrame::handleFollow() {
	if(!redirect.empty()) {
		if(ClientManager::getInstance()->isConnected(redirect)) {
			addStatus(T_("Redirect request received to a hub that's already connected"));
			return;
		}

		url = redirect;

		// the client is dead, long live the client!
		client->removeListener(this);
		ClientManager::getInstance()->putClient(client);
		clearUserList();
		clearTaskList();
		client = ClientManager::getInstance()->getClient(url);
		client->addListener(this);
		client->connect();
	}
}

void HubFrame::handleFilterUpdated() {
	tstring newText = filter->getText();
	if(newText != filterString) {
		filterString = newText;
		updateUserList();
	}
}

HubFrame::UserInfoList HubFrame::selectedUsersImpl() const {
	return showUsers->getChecked() ? usersFromTable(users) : (currentUser ? UserInfoList(1, currentUser) : UserInfoList());
}
