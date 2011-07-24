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

#include "PrivateFrame.h"

#include "HoldRedraw.h"
#include "MainWindow.h"
#include "resource.h"

#include <dcpp/ClientManager.h>
#include <dcpp/Client.h>
#include <dcpp/LogManager.h>
#include <dcpp/User.h>
#include <dcpp/WindowInfo.h>

using dwt::Grid;
using dwt::GridInfo;

const string PrivateFrame::id = "PM";
const string& PrivateFrame::getId() const { return id; }

PrivateFrame::FrameMap PrivateFrame::frames;

void PrivateFrame::openWindow(TabViewPtr parent, const HintedUser& replyTo_, const tstring& msg,
	const string& logPath, bool activate)
{
	PrivateFrame* pf;
	FrameIter i = frames.find(replyTo_);
	if(i == frames.end()) {
		pf = new PrivateFrame(parent, replyTo_, logPath);
	} else {
		pf = i->second;
	}
	if(activate)
		pf->activate();
	if(!msg.empty())
		pf->sendMessage(msg);

}

void PrivateFrame::gotMessage(TabViewPtr parent, const UserPtr& from, const UserPtr& to, const UserPtr& replyTo,
							  const tstring& aMessage, const string& hubHint)
{
	const UserPtr& user = (replyTo == ClientManager::getInstance()->getMe()) ? to : replyTo;
	FrameIter i = frames.find(user);
	if(i == frames.end()) {
		auto p = new PrivateFrame(parent, HintedUser(user, hubHint));
		if(!BOOLSETTING(POPUNDER_PM))
			p->activate();
		p->addChat(aMessage);
		if(Util::getAway()) {
			if(!(BOOLSETTING(NO_AWAYMSG_TO_BOTS) && user->isSet(User::BOT)))
				p->sendMessage(Text::toT(Util::getAwayMessage()));
		}
		WinUtil::notify(WinUtil::NOTIFICATION_PM_WINDOW, aMessage, [user] { activateWindow(user); });
	} else {
		i->second->addChat(aMessage);
	}
	WinUtil::notify(WinUtil::NOTIFICATION_PM, aMessage, [user] { activateWindow(user); });
}

void PrivateFrame::activateWindow(const UserPtr& u) {
	auto i = frames.find(u);
	if(i != frames.end())
		i->second->activate();
}

void PrivateFrame::closeAll(){
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		i->second->close(true);
}

void PrivateFrame::closeAllOffline() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i) {
		if(!i->second->online)
			i->second->close(true);
	}
}

WindowParams PrivateFrame::getWindowParams() const {
	WindowParams ret;
	addRecentParams(ret);
	ret["CID"] = WindowParam(replyTo.getUser().user->getCID().toBase32(), WindowParam::FLAG_IDENTIFIES | WindowParam::FLAG_CID);
	ret["Hub"] = WindowParam(replyTo.getUser().hint);
	ret["LogPath"] = WindowParam(getLogPath());
	return ret;
}

void PrivateFrame::parseWindowParams(TabViewPtr parent, const WindowParams& params) {
	auto cid = params.find("CID");
	auto hub = params.find("Hub");
	if(cid != params.end() && hub != params.end()) {
		auto logPath = params.find("LogPath");
		openWindow(parent, HintedUser(ClientManager::getInstance()->getUser(CID(cid->second)), hub->second), Util::emptyStringT,
			logPath != params.end() ? logPath->second.content : Util::emptyString, parseActivateParam(params));
	}
}

bool PrivateFrame::isFavorite(const WindowParams& params) {
	auto cid = params.find("CID");
	if(cid != params.end()) {
		UserPtr u = ClientManager::getInstance()->getUser(CID(cid->second));
		if(u)
			return FavoriteManager::getInstance()->isFavoriteUser(u);
	}
	return false;
}

PrivateFrame::PrivateFrame(TabViewPtr parent, const HintedUser& replyTo_, const string& logPath) :
BaseType(parent, _T(""), IDH_PM, IDI_PRIVATE, false),
replyTo(replyTo_),
priv(FavoriteManager::getInstance()->isPrivate(replyTo.getUser().hint)),
online(replyTo.getUser().user->isOnline())
{
	createChat(this);
	chat->setHelpId(IDH_PM_CHAT);
	addWidget(chat);
	chat->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleChatContextMenu(sc); });

	message->setHelpId(IDH_PM_MESSAGE);
	addWidget(message, true);
	message->onKeyDown([this](int c) { return GCC_WTF->handleMessageKeyDown(c); });
	message->onSysKeyDown([this](int c) { return GCC_WTF->handleMessageKeyDown(c); });
	message->onChar([this](int c) { return GCC_WTF->handleMessageChar(c); });

	initStatus();
	status->onDblClicked(STATUS_STATUS, [this] { openLog(); });

	updateOnlineStatus();

	initAccels();

	layout();

	readLog(logPath, SETTING(PM_LAST_LOG_LINES));

	ClientManager::getInstance()->addListener(this);

	callAsync([this] { updateOnlineStatus(); });

	frames.insert(std::make_pair(replyTo.getUser(), this));

	addRecent();
}

PrivateFrame::~PrivateFrame() {
	frames.erase(replyTo.getUser());
}

void PrivateFrame::addChat(const tstring& aLine, bool log) {
	/// @todo null clients are allowed (eg to display log history on opening), fix later
	Client* pClient = 0;
	/// @todo getting the client is disabled for now (no calling findOnlineUser outside the ClientManager lock)
	/*
	OnlineUser *ou = ClientManager::getInstance()->findOnlineUser(*replyTo.getUser(), Util::emptyString);
	if (ou) pClient = &(ou->getClient()); // getClient actually retuns a ref.
	*/

	ChatType::addChat(pClient, aLine);

	setDirty(SettingsManager::BOLD_PM);

	if(log && BOOLSETTING(LOG_PRIVATE_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(aLine);
		fillLogParams(params);
		LOG(LogManager::PM, params);
	}
}

void PrivateFrame::addStatus(const tstring& aLine, bool log) {
	tstring line = Text::toT("[" + Util::getShortTimeString() + "] ") + aLine;

	status->setText(STATUS_STATUS, line);

	if(BOOLSETTING(STATUS_IN_CHAT))
		addChat(_T("*** ") + aLine, log);
}

bool PrivateFrame::preClosing() {
	ClientManager::getInstance()->removeListener(this);
	return true;
}

string PrivateFrame::getLogPath() const {
	StringMap params;
	fillLogParams(params);
	return Util::validateFileName(LogManager::getInstance()->getPath(LogManager::PM, params));
}

void PrivateFrame::openLog() {
	WinUtil::openFile(Text::toT(getLogPath()));
}

void PrivateFrame::fillLogParams(StringMap& params) const {
	const CID& cid = replyTo.getUser().user->getCID();
	const string& hint = replyTo.getUser().hint;
	params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(cid, hint, priv));
	params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(cid, hint, priv));
	params["userCID"] = cid.toBase32();
	params["userNI"] = ClientManager::getInstance()->getNicks(cid, hint, priv)[0];
	params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
}

void PrivateFrame::layout() {
	const int border = 2;

	dwt::Rectangle r(getClientSize());

	r.size.y -= status->refresh();

	int ymessage = message->getTextSize(_T("A")).y * messageLines + 10;
	dwt::Rectangle rm(0, r.size.y - ymessage, r.width(), ymessage);
	message->resize(rm);

	r.size.y -= rm.size.y + border;
	chat->resize(r);
}

void PrivateFrame::updateOnlineStatus() {
	const CID& cid = replyTo.getUser().user->getCID();
	const string& hint = replyTo.getUser().hint;

	pair<tstring, bool> hubs = WinUtil::getHubNames(cid, hint, priv);

	setText(WinUtil::getNicks(cid, hint, priv) + _T(" - ") + hubs.first);

	online = hubs.second;
	setIcon(online ? IDI_PRIVATE : IDI_PRIVATE_OFF);
}

void PrivateFrame::enterImpl(const tstring& s) {
	bool resetText = true;
	bool send = false;
	// Process special commands
	if(s[0] == '/') {
		tstring cmd = s;
		tstring param;
		tstring message;
		tstring status;
		bool thirdPerson = false;
		if(WinUtil::checkCommand(cmd, param, message, status, thirdPerson)) {
			if(!message.empty()) {
				sendMessage(message, thirdPerson);
			}
			if(!status.empty()) {
				addStatus(status);
			}
		} else if(ChatType::checkCommand(cmd, param, status)) {
			if(!status.empty()) {
				addStatus(status);
			}
		} else if(Util::stricmp(cmd.c_str(), _T("grant")) == 0) {
			handleGrantSlot();
			addStatus(T_("Slot granted"));
		} else if(Util::stricmp(cmd.c_str(), _T("close")) == 0) {
			postMessage(WM_CLOSE);
		} else if((Util::stricmp(cmd.c_str(), _T("favorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("fav")) == 0)) {
			handleAddFavorite();
			addStatus(T_("Favorite user added"));
		} else if(Util::stricmp(cmd.c_str(), _T("getlist")) == 0) {
			handleGetList();
		} else if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
			openLog();
		} else if(Util::stricmp(cmd.c_str(), _T("help")) == 0) {
			addStatus(_T("*** ") + WinUtil::commands + _T(", /getlist, /grant, /close, /favorite, /log <system, downloads, uploads>"));
		} else {
			send = true;
		}
	} else {
		send = true;
	}

	if(send) {
		if(online) {
			sendMessage(s);
		} else {
			addStatus(T_("User went offline"));
			resetText = false;
		}
	}
	if(resetText) {
		message->setText(Util::emptyStringT);
	}
}

void PrivateFrame::sendMessage(const tstring& msg, bool thirdPerson) {
	ClientManager::getInstance()->privateMessage(replyTo.getUser(), Text::fromT(msg), thirdPerson);
}

PrivateFrame::UserInfoList PrivateFrame::selectedUsersImpl() {
	return UserInfoList(1, &replyTo);
}

void PrivateFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept {
	if(replyTo.getUser() == aUser.getUser())
		callAsync([this] { updateOnlineStatus(); });
}
void PrivateFrame::on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept {
	if(replyTo.getUser() == aUser)
		callAsync([this] { updateOnlineStatus(); });
}
void PrivateFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
	if(replyTo.getUser() == aUser)
		callAsync([this] { updateOnlineStatus(); });
}

void PrivateFrame::tabMenuImpl(dwt::MenuPtr& menu) {
	appendUserItems(getParent(), menu, false, false);
	prepareMenu(menu, UserCommand::CONTEXT_USER, ClientManager::getInstance()->getHubs(replyTo.getUser().user->getCID(),
		replyTo.getUser().hint, priv));
	menu->appendSeparator();
}

bool PrivateFrame::handleChatContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 || pt.y() == -1) {
		pt = chat->getContextMenuPos();
	}

	MenuPtr menu = chat->getMenu();

	menu->setTitle(escapeMenu(getText()), getParent()->getIcon(this));
	
	menu->appendItem(T_("Clear chat"), [this] {
	chat->setSelection();
	chat->replaceSelection(_T("")); 
	});

	prepareMenu(menu, UserCommand::CONTEXT_USER, ClientManager::getInstance()->getHubs(replyTo.getUser().user->getCID(),
		replyTo.getUser().hint, priv));

	menu->open(pt);
	return true;
}

void PrivateFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	ClientManager::getInstance()->userCommand(replyTo.getUser(), uc, ucParams, true);
}
