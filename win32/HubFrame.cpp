/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include <dcpp/AdcHub.h>
#include <dcpp/ChatMessage.h>
#include <dcpp/ClientManager.h>
#include <dcpp/ConnectionManager.h>
#include <dcpp/ConnectivityManager.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/SearchManager.h>
#include <dcpp/User.h>
#include <dcpp/UserMatch.h>
#include <dcpp/version.h>
#include <dcpp/WindowInfo.h>

#include <dwt/util/HoldResize.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/SplitterContainer.h>

#include "MainWindow.h"
#include "PrivateFrame.h"
#include "ParamDlg.h"
#include "HoldRedraw.h"
#include "TypedTable.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::SplitterContainer;

const string HubFrame::id = WindowManager::hub();
const string& HubFrame::getId() const { return id; }

static const ColumnInfo usersColumns[] = {
	{ N_("Nick"), 100, false },
	{ N_("Shared"), 80, true },
	{ N_("Description"), 75, false },
	{ N_("Tag"), 100, false },
	{ N_("Connection"), 75, false },
	{ N_("IP"), 100, false },
	{ N_("Country"), 100, false },
	{ N_("E-Mail"), 100, false },
	{ N_("CID"), 300, false}
};

HubFrame::FrameList HubFrame::frames;

void HubFrame::openWindow(TabViewPtr parent, const string& url, bool activate, bool connect) {
	auto i = find_if(frames.begin(), frames.end(), [&url](HubFrame* frame) { return frame->url == url; });

	if(i == frames.end()) {
		auto frame = new HubFrame(parent, url, connect);
		if(activate)
			frame->activate();

	} else {
		auto frame = *i;
		if(activate)
			frame->activate();
		else
			frame->setDirty(SettingsManager::BOLD_HUB);
	}
}

void HubFrame::activateWindow(const string& url) {
	auto i = find_if(frames.begin(), frames.end(), [&url](HubFrame* frame) { return frame->url == url; });
	if(i != frames.end()) {
		(*i)->activate();
	}
}

void HubFrame::closeAll(ClosePred f) {
	if(!WinUtil::mainWindow->getEnabled())
		return;

	auto toClose = frames;
	if(f) {
		toClose.erase(std::remove_if(toClose.begin(), toClose.end(), f), toClose.end());
	}

	if(!toClose.empty() && (!BOOLSETTING(CONFIRM_HUB_CLOSING) || dwt::MessageBox(WinUtil::mainWindow).show(
		str(TF_("Really close %1% hub windows?") % toClose.size()), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
		dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES))
	{
		for(auto frame: toClose) {
			frame->confirmClose = false;
			frame->close(true);
		}
	}
}

void HubFrame::closeAll(bool disconnected) {
	closeAll(disconnected ? [](HubFrame* frame) { return frame->client->isConnected(); } : ClosePred());
}

void HubFrame::closeFavGroup(const string& group, bool reversed) {
	closeAll([&](HubFrame* frame) -> bool {
		FavoriteHubEntry* fav = FavoriteManager::getInstance()->getFavoriteHubEntry(frame->url);
		return (fav && fav->getGroup() == group) ^ !reversed;
	});
}

void HubFrame::resortUsers() {
	for(auto i: frames)
		i->resortForFavsFirst(true);
}

WindowParams HubFrame::getWindowParams() const {
	WindowParams ret;
	addRecentParams(ret);
	ret[WindowInfo::address] = WindowParam(url, WindowParam::FLAG_IDENTIFIES);
	return ret;
}

void HubFrame::parseWindowParams(TabViewPtr parent, const WindowParams& params) {
	auto address = params.find(WindowInfo::address);
	if(address != params.end())
		openWindow(parent, address->second, parseActivateParam(params), params.find("NoConnect") == params.end());
}

bool HubFrame::isFavorite(const WindowParams& params) {
	auto i = params.find(WindowInfo::address);
	if(i != params.end())
		return FavoriteManager::getInstance()->isFavoriteHub(i->second);
	return false;
}

HubFrame::HubFrame(TabViewPtr parent, const string& url_, bool connect) :
BaseType(parent, Text::toT(url_), IDH_HUB, IDI_HUB_OFF, false),
paned(0),
userGrid(0),
users(0),
filter(usersColumns, COLUMN_LAST, [this] { updateUserList(); }),
filterOpts(0),
showUsers(0),
client(0),
url(url_),
updateUsers(false),
waitingForPW(false),
resort(false),
showJoins(BOOLSETTING(SHOW_JOINS)),
favShowJoins(BOOLSETTING(FAV_SHOW_JOINS)),
confirmClose(true),
currentUser(0),
hubMenu(false),
inTabComplete(false)
{
	paned = addChild(SplitterContainer::Seed(0.7));

	createChat(paned);
	chat->setHelpId(IDH_HUB_CHAT);
	addWidget(chat);
	chat->onLink([this](const tstring& link) { return handleChatLink(link); });
	chat->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleChatContextMenu(sc); });

	message->setHelpId(IDH_HUB_MESSAGE);
	addWidget(message, ALWAYS_FOCUS, false);
	message->onKeyDown([this](int c) { return handleMessageKeyDown(c); });
	message->onSysKeyDown([this](int c) { return handleMessageKeyDown(c); });
	message->onChar([this] (int c) { return handleMessageChar(c); });

	{
		userGrid = paned->addChild(Grid::Seed(2, 1));
		userGrid->column(0).mode = GridInfo::FILL;
		userGrid->row(0).mode = GridInfo::FILL;
		userGrid->row(0).align = GridInfo::STRETCH;

		users = userGrid->addChild(WidgetUsers::Seed(WinUtil::Seeds::table));
		addWidget(users);

		users->setSmallImageList(WinUtil::userImages);
		WinUtil::makeColumns(users, usersColumns, COLUMN_LAST, SETTING(HUBFRAME_ORDER), SETTING(HUBFRAME_WIDTHS));
		users->setSort(COLUMN_NICK);

		users->onSelectionChanged([this] { callAsync([&] { updateStatus(); }); });
		users->onDblClicked([this] { handleDoubleClickUsers(); });
		users->onKeyDown([this](int c) { return handleUsersKeyDown(c); });
		users->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleUsersContextMenu(sc); });

		filter.createTextBox(userGrid);
		filter.text->setHelpId(IDH_HUB_FILTER);
		filter.text->setCue(T_("Filter users"));
		addWidget(filter.text);
	}

	{
		auto seed = Grid::Seed(1, 2);
		seed.exStyle |= WS_EX_TRANSPARENT;
		filterOpts = addChild(seed);
		filterOpts->setHelpId(IDH_HUB_FILTER);

		filter.createColumnBox(filterOpts);
		addWidget(filter.column, AUTO_FOCUS, false);

		filter.createMethodBox(filterOpts);
		addWidget(filter.method, AUTO_FOCUS, false);

		hideFilterOpts(nullptr);

		filter.text->onFocus([this] { showFilterOpts(); });
		filter.text->onKillFocus([this](dwt::Widget* w) { hideFilterOpts(w); });
		filter.column->onKillFocus([this](dwt::Widget* w) { hideFilterOpts(w); });
		filter.method->onKillFocus([this](dwt::Widget* w) { hideFilterOpts(w); });
	}

	initStatus();

	status->onDblClicked(STATUS_STATUS, [this] { openLog(false); });

	status->setIcon(STATUS_USERS, WinUtil::statusIcon(IDI_USERS));

	showUsers = addChild(WinUtil::Seeds::splitCheckBox);
	showUsers->setHelpId(IDH_HUB_SHOW_USERS);
	showUsers->setChecked(BOOLSETTING(GET_USER_INFO));
	status->setWidget(STATUS_SHOW_USERS, showUsers);

	status->setHelpId(STATUS_STATUS, IDH_HUB_STATUS);
	status->setHelpId(STATUS_SECURE, IDH_HUB_SECURE_STATUS);
	status->setHelpId(STATUS_USERS, IDH_HUB_USERS_COUNT);
	status->setHelpId(STATUS_SHARED, IDH_HUB_SHARED);
	status->setHelpId(STATUS_AVERAGE_SHARED, IDH_HUB_AVERAGE_SHARED);

	addAccel(FALT, 'G', [this] { handleGetList(); });
	addAccel(FCONTROL, 'R', [this] { handleReconnect(); });
	addAccel(FALT, 'P', [this] { handlePrivateMessage(getParent()); });
	addAccel(FALT, 'U', [this] { users->setFocus(); });
	initAccels();

	layout();

	initSecond();

	client = ClientManager::getInstance()->getClient(url);
	client->addListener(this);
	if(connect)
		client->connect();

	readLog(getLogPath(), SETTING(HUB_LAST_LOG_LINES));

	frames.push_back(this);

	showUsers->onClicked([this] { handleShowUsersClicked(); });

	FavoriteManager::getInstance()->addListener(this);

	addRecent();
}

HubFrame::~HubFrame() {
	ClientManager::getInstance()->putClient(client);
	clearTaskList();
}

bool HubFrame::preClosing() {
	if(BOOLSETTING(CONFIRM_HUB_CLOSING) && confirmClose && !WinUtil::mainWindow->closing() &&
		dwt::MessageBox(this).show(getText() + _T("\n\n") + T_("Really close?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
		dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) != IDYES)
	{
		return false;
	}

	FavoriteManager::getInstance()->removeListener(this);
	client->removeListener(this);
	client->disconnect(true);

	frames.erase(std::remove(frames.begin(), frames.end(), this), frames.end());
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
	const int border = 2;

	dwt::Rectangle r(getClientSize());

	r.size.y -= status->refresh();

	dwt::util::HoldResize hr(this, 2);
	int ymessage = message->getTextSize(_T("A")).y * messageLines + 10;
	dwt::Rectangle rm(0, r.size.y - ymessage, r.width(), ymessage);
	hr.resize(message, rm);

	r.size.y -= rm.size.y + border;
	hr.resize(paned, r);

	if(showUsers->getChecked()) {
		userGrid->setVisible(true);
		paned->maximize(0);

		if(filterOpts->hasStyle(WS_VISIBLE)) {
			filterOpts->setZOrder(HWND_TOP);
			filter.column->setZOrder(HWND_TOP);
			filter.method->setZOrder(HWND_TOP);

			auto r = filter.text->getWindowRect();
			r.pos = dwt::ClientCoordinate(dwt::ScreenCoordinate(r.pos), filterOpts->getParent()).getPoint();
			r.pos.y += r.height();
			r.size = filterOpts->getPreferredSize();
			hr.resize(filterOpts, r);
		}

	} else {
		paned->maximize(chat);
		userGrid->setVisible(false);
	}
}

void HubFrame::updateStatus() {
	auto users = getStatusUsers();
	status->setText(STATUS_USERS, users.second + Text::toT(Util::toString(users.first)));
	status->setToolTip(STATUS_USERS, users.second + str(TFN_("%1% user", "%1% users", users.first) % users.first));

	status->setText(STATUS_SHARED, getStatusShared());
	status->setText(STATUS_AVERAGE_SHARED, getStatusAverageShared());
}

void HubFrame::updateSecureStatus() {
	dwt::IconPtr icon;
	tstring text;
	if(client) {
		if(client->isTrusted()) {
			icon = WinUtil::statusIcon(IDI_TRUSTED);
			text = _T("[T] ");
		} else if(client->isSecure()) {
			icon = WinUtil::statusIcon(IDI_SECURE);
			text = _T("[U] ");
		}
		text += Text::toT(client->getCipherName());
	}
	status->setIcon(STATUS_SECURE, icon);
	status->setToolTip(STATUS_SECURE, text);
}

void HubFrame::initSecond() {
	setTimer([this] { return eachSecond(); }, 1000);
}

bool HubFrame::eachSecond() {
	if(updateUsers) {
		updateUsers = false;
		callAsync([this] { execTasks(); });
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
				if(BOOLSETTING(JOIN_OPEN_NEW_WINDOW)) {
					HubFrame::openWindow(getParent(), Text::fromT(param));
				} else {
					redirect(Text::fromT(param));
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
			close(true);
		} else if(Util::stricmp(cmd.c_str(), _T("userlist")) == 0) {
			showUsers->setChecked(!showUsers->getChecked());
			handleShowUsersClicked();
		} else if(Util::stricmp(cmd.c_str(), _T("conn")) == 0 || Util::stricmp(cmd.c_str(), _T("connection")) == 0) {
			addChat(_T("*** ") + Text::toT(ConnectivityManager::getInstance()->getInformation()));
		} else if((Util::stricmp(cmd.c_str(), _T("favorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("fav")) == 0)) {
			addAsFavorite();
		} else if((Util::stricmp(cmd.c_str(), _T("removefavorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("removefav")) == 0)) {
			removeFavoriteHub();
		} else if(Util::stricmp(cmd.c_str(), _T("getlist")) == 0) {
			if(!param.empty()) {
				auto ui = findUser(param);
				if(ui) {
					ui->getList();
				}
			}
		} else if(Util::stricmp(cmd.c_str(), _T("ignore")) == 0) {
			if(!param.empty()) {
				auto ui = findUser(param);
				if(ui) {
					ui->ignoreChat(true);
				}
			}
		} else if(Util::stricmp(cmd.c_str(), _T("unignore")) == 0) {
			if(!param.empty()) {
				auto ui = findUser(param);
				if(ui) {
					ui->ignoreChat(false);
				}
			}
		} else if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
			if(param.empty())
				openLog();
			else if(Util::stricmp(param.c_str(), _T("status")) == 0)
				openLog(true);
		} else if(Util::stricmp(cmd.c_str(), _T("help")) == 0) {
			addChat(_T("*** ") + WinUtil::commands +
				_T(", /join <hub-ip>, /showjoins, /favshowjoins, /close, /userlist, ")
				_T("/conn[ection], /fav[orite], /removefav[orite], ")
				_T("/pm <user> [message], /getlist <user>, /ignore <user>, /unignore <user>, ")
				_T("/log <status, system, downloads, uploads>"));
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
	for(auto& i: userMap) {
		delete i.second;
	}
	currentUser = 0;
	userMap.clear();
}

void HubFrame::clearTaskList() {
	tasks.clear();
}

void HubFrame::addedChat(const tstring& message) {
	{
		auto u = url;
		WinUtil::notify(WinUtil::NOTIFICATION_MAIN_CHAT, message, [this, u] { activateWindow(u); });
	}
	setDirty(SettingsManager::BOLD_HUB);

	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		ParamMap params;
		params["message"] = [&message] { return Text::toDOS(Text::fromT(message)); };
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = [this] { return client->getHubUrl(); };
		client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}
}

void HubFrame::addStatus(const tstring& text, bool legitimate /* = true */) {
	status->setText(STATUS_STATUS, Text::toT("[" + Util::getShortTimeString() + "] ") + text);

	if(legitimate) {
		if(BOOLSETTING(STATUS_IN_CHAT)) {
			addChat(_T("*** ") + text);
		} else {
			setDirty(SettingsManager::BOLD_HUB);
		}
	}

	if(BOOLSETTING(LOG_STATUS_MESSAGES)) {
		ParamMap params;
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = [this] { return client->getHubUrl(); };
		client->getMyIdentity().getParams(params, "my", true);
		params["message"] = [&text] { return Text::fromT(text); };
		LOG(LogManager::STATUS, params);
	}
}

void HubFrame::addTask(Tasks s, const OnlineUser& u) {
	tasks.add(s, unique_ptr<Task>(new UserTask(u)));
	updateUsers = true;
}

void HubFrame::execTasks() {
	updateUsers = false;
	TaskQueue::List t;
	tasks.get(t);

	HoldRedraw hold(users);

	for(auto& i: t) {
		if(i.first == UPDATE_USER) {
			updateUser(static_cast<UserTask&>(*i.second));
		} else if(i.first == UPDATE_USER_JOIN) {
			UserTask& u = static_cast<UserTask&>(*i.second);
			if(updateUser(u)) {
				if (showJoins || (favShowJoins && FavoriteManager::getInstance()->isFavoriteUser(u.user))) {
					addStatus(str(TF_("Joins: %1%") % Text::toT(u.identity.getNick())));
				}
			}
		} else if(i.first == REMOVE_USER) {
			UserTask& u = static_cast<UserTask&>(*i.second);
			removeUser(u.user);
			if (showJoins || (favShowJoins && FavoriteManager::getInstance()->isFavoriteUser(u.user))) {
				addStatus(str(TF_("Parts: %1%") % Text::toT(u.identity.getNick())));
			}
		}
	}
	if(resort && showUsers->getChecked()) {
		users->resort();
		resort = false;
	}
}

void HubFrame::onConnected() {
	addStatus(T_("Connected"));
	setIcon(IDI_HUB);
	updateSecureStatus();
}

void HubFrame::onDisconnected() {
	clearUserList();
	clearTaskList();
	setIcon(IDI_HUB_OFF);
	updateSecureStatus();
}

void HubFrame::onGetPassword() {
	if(!client->getPassword().empty()) {
		client->password(client->getPassword());
		addStatus(T_("Stored password sent..."));
	} else if(!waitingForPW) {
		waitingForPW = true;
		if(BOOLSETTING(PROMPT_PASSWORD)) {
			ParamDlg linePwd(this, getText(), T_("Please enter a password"), Util::emptyStringT, true);
			auto res = linePwd.run();
			waitingForPW = false;
			if(res == IDOK) {
				client->setPassword(Text::fromT(linePwd.getValue()));
				client->password(Text::fromT(linePwd.getValue()));
			} else {
				client->disconnect(true);
			}
		} else {
			message->setText(_T("/password "));
			message->setFocus();
			message->setSelection(10, 10);
		}
	}
}

void HubFrame::onPrivateMessage(const ChatMessage& message) {
	bool fromHub = false, fromBot = false;
	{
		auto lock = ClientManager::getInstance()->lock();
		auto ou = ClientManager::getInstance()->findOnlineUser(message.replyTo->getCID(), url, true);
		if(ou && ou->getIdentity().isHub())
			fromHub = true;
		if(ou && ou->getIdentity().isBot())
			fromBot = true;
	}

	bool ignore = false, window = false;
	if(fromHub) {
		if(BOOLSETTING(IGNORE_HUB_PMS)) {
			ignore = true;
		} else if(BOOLSETTING(POPUP_HUB_PMS) || PrivateFrame::isOpen(message.replyTo)) {
			window = true;
		}
	} else if(fromBot) {
		if(BOOLSETTING(IGNORE_BOT_PMS)) {
			ignore = true;
		} else if(BOOLSETTING(POPUP_BOT_PMS) || PrivateFrame::isOpen(message.replyTo)) {
			window = true;
		}
	} else if(BOOLSETTING(POPUP_PMS) || PrivateFrame::isOpen(message.replyTo) || message.from == client->getMyIdentity().getUser()) {
		window = true;
	}

	if(ignore) {
		addStatus(str(TF_("Ignored message: %1%") % Text::toT(message.message)), false);

	} else {
		if(window && !PrivateFrame::gotMessage(getParent(), message.from, message.to, message.replyTo, message, url, fromBot)) {
			window = false;
			addStatus(T_("Failed to create a new PM window; check the \"Max PM windows\" value in Settings > Experts only"));
		}
		if(!window) {
			/// @todo add formatting here (for PMs in main chat)
			addChat(str(TF_("Private message from %1%: %2%") % getNick(message.from) % Text::toT(message.message)));
		}
		WinUtil::mainWindow->TrayPM();
	}
}

HubFrame::UserInfo* HubFrame::findUser(const tstring& nick) {
	for(auto& i: userMap) {
		if(i.second->getText(COLUMN_NICK) == nick)
			return i.second;
	}
	return nullptr;
}

const tstring& HubFrame::getNick(const UserPtr& aUser) {
	auto i = userMap.find(aUser);
	if(i == userMap.end())
		return Util::emptyStringT;

	UserInfo* ui = i->second;
	return ui->getText(COLUMN_NICK);
}

bool HubFrame::updateUser(const UserTask& u) {
	auto i = userMap.find(u.user);
	if(i == userMap.end()) {
		UserInfo* ui = new UserInfo(u);
		userMap.insert(make_pair(u.user, ui));
		if(!ui->isHidden() && showUsers->getChecked())
			users->insert(ui);

		if(!filter.empty())
			updateUserList(ui);
		return true;
	} else {
		UserInfo* ui = i->second;
		if(!ui->isHidden() && u.identity.isHidden() && showUsers->getChecked()) {
			users->erase(ui);
		}

		resort = ui->update(u.identity, users->getSortColumn()) || resort;
		if(showUsers->getChecked()) {
			users->update(ui);
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
	columns[COLUMN_IP] = Text::toT(identity.getIp());
	columns[COLUMN_COUNTRY] = Text::toT(identity.getCountry());
	columns[COLUMN_EMAIL] = Text::toT(identity.getEmail());
	columns[COLUMN_CID] = Text::toT(identity.getUser()->getCID().toBase32());

	if(sortCol != -1) {
		needsSort = needsSort || (old != columns[sortCol]);
	}

	setIdentity(identity);
	return needsSort;
}

void HubFrame::removeUser(const UserPtr& aUser) {
	auto i = userMap.find(aUser);
	if(i == userMap.end()) {
		// Should never happen?
		dcassert(i != userMap.end());
		return;
	}

	UserInfo* ui = i->second;
	if(!ui->isHidden() && showUsers->getChecked())
		users->erase(ui);

	userMap.erase(i);
	if(ui == currentUser)
		currentUser = 0;
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

int HubFrame::UserInfo::getImage(int col) const {
	if(col != 0) {
		return -1;
	}

	int image = identity.isBot() ? WinUtil::USER_ICON_BOT : identity.isAway() ? WinUtil::USER_ICON_AWAY : WinUtil::USER_ICON;
	image *= WinUtil::USER_ICON_MOD_START * WinUtil::USER_ICON_MOD_START;

	if(CONNSETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_PASSIVE &&
		!identity.isBot() && !identity.isTcpActive() && !identity.supports(AdcHub::NAT0_FEATURE))
	{
		// Users we can't connect to
		image += 1 << (WinUtil::USER_ICON_NOCON - WinUtil::USER_ICON_MOD_START);
	}

	string freeSlots = identity.get("FS");
	if(!freeSlots.empty() && Util::toUInt(freeSlots) == 0) {
		image += 1 << (WinUtil::USER_ICON_NOSLOT - WinUtil::USER_ICON_MOD_START);
	}

	if(identity.isOp()) {
		image += 1 << (WinUtil::USER_ICON_OP - WinUtil::USER_ICON_MOD_START);
	}

	return image;
}

int HubFrame::UserInfo::getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const {
	auto style = identity.getStyle();

	if(!style.font.empty()) {
		auto cached = WinUtil::getUserMatchFont(style.font);
		if(cached.get()) {
			font = cached->handle();
		}
	}

	if(style.textColor != -1) {
		textColor = style.textColor;
	}

	if(style.bgColor != -1) {
		bgColor = style.bgColor;
	}

	return CDRF_NEWFONT;
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

void HubFrame::on(Connecting, Client*) noexcept {
	auto hubUrl = client->getHubUrl();
	callAsync([this, hubUrl] {
		addStatus(str(TF_("Connecting to %1%...") % Text::toT(Util::addBrackets(hubUrl))));
		setText(Text::toT(hubUrl));
	});
}
void HubFrame::on(Connected, Client*) noexcept {
	callAsync([this] { onConnected(); });
}
void HubFrame::on(ClientListener::UserUpdated, Client*, const OnlineUser& user) noexcept {
	addTask(UPDATE_USER_JOIN, user);
}
void HubFrame::on(UsersUpdated, Client*, const OnlineUserList& aList) noexcept {
	for(auto& i: aList) {
		tasks.add(UPDATE_USER, unique_ptr<Task>(new UserTask(*i)));
	}
	updateUsers = true;
}

void HubFrame::on(ClientListener::UserRemoved, Client*, const OnlineUser& user) noexcept {
	addTask(REMOVE_USER, user);
}

void HubFrame::on(Redirect, Client*, const string& line) noexcept {
	if(ClientManager::getInstance()->isConnected(line)) {
		callAsync([this] { addStatus(T_("Redirect request received to a hub that's already connected"), true); });
		return;
	}
	callAsync([this, line] {
		if(BOOLSETTING(AUTO_FOLLOW)) {
			auto copy = line; /// @todo shouldn't the lambda have already created a copy?
			redirect(std::move(copy));
		} else {
			string msg = str(F_("Received a redirect request to %1%, click here to follow it") % Util::addBrackets(line));
			tstring msgT = Text::toT(msg);
			string tmp;
			addStatus(msgT, false);
			/// @todo change to "javascript: external.redirect" when switching to an HTML control
			addChatHTML("<span>*** </span><a href=\"redirect: " + SimpleXML::escape(line, tmp, true) + "\">" +
				SimpleXML::escape(msg, tmp, false) + "</a>");
			addedChat(_T("*** ") + msgT);
		}
	});
}

void HubFrame::on(Failed, Client*, const string& line) noexcept {
	callAsync([this, line] {
		addStatus(Text::toT(line));
		onDisconnected();
	});
}

void HubFrame::on(GetPassword, Client*) noexcept {
	callAsync([this] { onGetPassword(); });
}

void HubFrame::on(HubUpdated, Client*) noexcept {
	string hubName = client->getHubName();
	if(!client->getHubDescription().empty()) {
		hubName += " - " + client->getHubDescription();
	}
	hubName += " (" + client->getHubUrl() + ")";
#ifdef _DEBUG
	auto application = client->getHubIdentity().getApplication();
	if(!application.empty()) {
		hubName += " - " + application;
	}
#endif
	tstring hubNameT = Text::toT(hubName);
	callAsync([this, hubNameT] { setText(hubNameT); });
}

void HubFrame::on(Message, Client*, const ChatMessage& message) noexcept {
	callAsync([this, message] {
		if(message.to.get() && message.replyTo.get()) {
			onPrivateMessage(message);
		} else {
			addChat(message);
		}
	});
}

void HubFrame::on(StatusMessage, Client*, const string& line, int statusFlags) noexcept {
	onStatusMessage(line, statusFlags);
}

void HubFrame::on(NickTaken, Client*) noexcept {
	callAsync([this] { addStatus(T_("Your nick was already taken, please change to something else!"), true); });
}

void HubFrame::on(SearchFlood, Client*, const string& line) noexcept {
	callAsync([=] { onStatusMessage(str(F_("Search spam detected from %1%") % line), ClientListener::FLAG_IS_SPAM); });
}

void HubFrame::onStatusMessage(const string& line, int flags) {
	callAsync([=] { addStatus(Text::toT(line),
		!(flags & ClientListener::FLAG_IS_SPAM) || !BOOLSETTING(FILTER_MESSAGES)); });
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

pair<size_t, tstring> HubFrame::getStatusUsers() const {
	size_t userCount = 0;
	for(auto& i: userMap) {
		UserInfo* ui = i.second;
		if(!ui->isHidden())
			userCount++;
	}

	tstring textForUsers;
	if (users->countSelected() > 1)
		textForUsers += Text::toT(Util::toString(users->countSelected()) + "/");
	if (showUsers->getChecked() && users->size() < userCount)
		textForUsers += Text::toT(Util::toString(users->size()) + "/");
	return make_pair(userCount, textForUsers);
}

tstring HubFrame::getStatusAverageShared() const {
	int64_t available;
	size_t userCount = 0;
	if (users->countSelected() > 1) {
		available = users->forEachSelectedT(CountAvailable()).available;
		userCount = users->countSelected();
	} else {
		available = std::for_each(userMap.begin(), userMap.end(), CountAvailable()).available;
		for(auto& i: userMap) {
			UserInfo* ui = i.second;
			if(!ui->isHidden())
				userCount++;
		}
	}

	return str(TF_("Average: %1%") % Text::toT(Util::formatBytes(userCount > 0 ? available / userCount : 0)));
}

void HubFrame::on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) noexcept {
	resortForFavsFirst();
}
void HubFrame::on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) noexcept {
	resortForFavsFirst();
}

void HubFrame::resortForFavsFirst(bool justDoIt /* = false */) {
	if(justDoIt || BOOLSETTING(SORT_FAVUSERS_FIRST)) {
		resort = true;
		callAsync([this] { execTasks(); });
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
		if(!client->getPassword().empty())  {
			aEntry.setPassword(client->getPassword());
		}
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

void HubFrame::updateUserList(UserInfo* ui) {
	auto filterPrep = filter.prepare();
	auto filterInfoF = [this, &ui](int column) { return Text::fromT(ui->getText(column)); };

	//single update?
	//avoid refreshing the whole list and just update the current item
	//instead
	if(ui) {
		if(ui->isHidden()) {
			return;
		}
		if(filter.empty()) {
			if(users->find(ui) == -1) {
				users->insert(ui);
			}
		} else {
			if(filter.match(filterPrep, filterInfoF)) {
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

		if(filter.empty()) {
			for(auto& i: userMap) {
				ui = i.second;
				if(!ui->isHidden())
					users->insert(i.second);
			}
		} else {
			for(auto& i: userMap) {
				ui = i.second;
				if(!ui->isHidden() && filter.match(filterPrep, filterInfoF)) {
					users->insert(ui);
				}
			}
		}
	}
}

bool HubFrame::userClick(const dwt::ScreenCoordinate& pt) {
	tstring txt = chat->textUnderCursor(pt);
	if(txt.empty())
		return false;

	// Possible nickname click, let's see if we can find one like it in the name list...
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

	return true;
}

bool HubFrame::handleChatLink(const tstring& link) {
	if(link.size() > 10 && !link.compare(0, 10, _T("redirect: "))) {
		redirect(Text::fromT(link.substr(10)));
		return true;
	}

	return false;
}

bool HubFrame::handleChatContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 || pt.y() == -1) {
		pt = chat->getContextMenuPos();
	}

	if(userClick(pt) && handleUsersContextMenu(pt))
		return true;

	auto menu = chat->getMenu();

	menu->setTitle(escapeMenu(getText()), getParent()->getIcon(this));

	prepareMenu(menu.get(), UserCommand::CONTEXT_HUB, url);

	hubMenu = true;
	menu->open(pt);
	return true;
}

bool HubFrame::handleUsersContextMenu(dwt::ScreenCoordinate pt) {
	auto sel = selectedUsersImpl();
	if(!sel.empty()) {
		if(pt.x() == -1 || pt.y() == -1) {
			pt = users->getContextMenuPos();
		}

		auto menu = addChild(WinUtil::Seeds::menu);

		menu->setTitle((sel.size() == 1) ? escapeMenu(getNick(sel[0]->getUser())) : str(TF_("%1% users") % sel.size()),
			WinUtil::userImages->getIcon(0));

		appendUserItems(getParent(), menu.get());

		menu->appendSeparator();
		auto copyMenu = menu->appendPopup(T_("&Copy"));
		for(int j=0; j<COLUMN_LAST; j++) {
			copyMenu->appendItem(T_(usersColumns[j].name), [this, j] { handleMultiCopy(j); });
		}

		prepareMenu(menu.get(), UserCommand::CONTEXT_USER, url);

		hubMenu = false;
		menu->open(pt);
		return true;
	}
	return false;
}

void HubFrame::tabMenuImpl(dwt::Menu* menu) {
	if(!FavoriteManager::getInstance()->isFavoriteHub(url)) {
		menu->appendItem(T_("Add To &Favorites"), [this] { addAsFavorite(); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
	}

	menu->appendItem(T_("&Reconnect\tCtrl+R"), [this] { handleReconnect(); }, WinUtil::menuIcon(IDI_RECONNECT));
	menu->appendItem(T_("Copy &address to clipboard"), [this] { handleCopyHub(); });

	prepareMenu(menu, UserCommand::CONTEXT_HUB, url);

	menu->appendSeparator();

	hubMenu = true;
}

void HubFrame::handleShowUsersClicked() {
	bool checked = showUsers->getChecked();

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

	tstring tmpstr;
	for(auto& i: selectedUsersImpl()) {
		tmpstr += static_cast<UserInfo*>(i)->getText(index);
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

	auto ucParams = ucLineParams;

	// imitate ClientManager::userCommand, except some params are cached for multiple selections

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

	if(hubMenu) {
		client->sendUserCmd(uc, ucParams);
	} else {
		auto sel = selectedUsersImpl();
		for(auto& i: sel) {
			auto tmp = ucParams;
			static_cast<UserInfo*>(i)->getIdentity().getParams(tmp, "user", true);
			client->sendUserCmd(uc, tmp);
		}
	}
}

string HubFrame::getLogPath(bool status) const {
	ParamMap params;
	params["hubNI"] = [this] { return client->getHubName(); };
	params["hubURL"] = [this] { return client->getHubUrl(); };
	params["myNI"] = [this] { return client->getMyNick(); };
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
	for(auto& i: userMap) {
		string prevNick, nick = i.second->getIdentity().getNick(), wholeNick = nick;

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
			for (auto i = tabCompleteNicks.begin(); i < tabCompleteNicks.end(); i+=2)
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

void HubFrame::redirect(string&& target) {
	if(ClientManager::getInstance()->isConnected(target)) {
		addStatus(T_("Redirect request received to a hub that's already connected"));
		return;
	}

	url = std::forward<string>(target);

	// the client is dead, long live the client!
	client->removeListener(this);
	ClientManager::getInstance()->putClient(client);
	client = 0;
	onDisconnected();
	client = ClientManager::getInstance()->getClient(url);
	client->addListener(this);
	client->connect();
}

void HubFrame::showFilterOpts() {
	filterOpts->setEnabled(true);
	filterOpts->setVisible(true);

	layout();
}

void HubFrame::hideFilterOpts(dwt::Widget* w) {
	if(w != filter.text && w != filter.column && w != filter.method) {
		filterOpts->setEnabled(false);
		filterOpts->setVisible(false);
	}
}

HubFrame::UserInfoList HubFrame::selectedUsersImpl() const {
	return showUsers->getChecked() ? usersFromTable(users) : (currentUser ? UserInfoList(1, currentUser) : UserInfoList());
}
