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

#include "PrivateFrame.h"

#include "HoldRedraw.h"
#include "MainWindow.h"
#include "resource.h"

#include <dcpp/ClientManager.h>
#include <dcpp/Client.h>
#include <dcpp/File.h>
#include <dcpp/LogManager.h>
#include <dcpp/User.h>
#include <dcpp/WindowInfo.h>

const string PrivateFrame::id = "PM";
const string& PrivateFrame::getId() const { return id; }

PrivateFrame::FrameMap PrivateFrame::frames;

void PrivateFrame::openWindow(dwt::TabView* mdiParent, const UserPtr& replyTo_, const tstring& msg, const string& hubHint,
							  const string& logPath)
{
	PrivateFrame* pf = 0;
	FrameIter i = frames.find(replyTo_);
	if(i == frames.end()) {
		pf = new PrivateFrame(mdiParent, replyTo_, true, hubHint, logPath);
	} else {
		pf = i->second;
		pf->activate();
	}
	if(!msg.empty())
		pf->sendMessage(msg);

}

void PrivateFrame::gotMessage(dwt::TabView* mdiParent, const UserPtr& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, const string& hubHint) {
	PrivateFrame* p = 0;
	const UserPtr& user = (replyTo == ClientManager::getInstance()->getMe()) ? to : replyTo;

	FrameIter i = frames.find(user);
	if(i == frames.end()) {
		p = new PrivateFrame(mdiParent, user, !BOOLSETTING(POPUNDER_PM), hubHint);
		p->addChat(aMessage);
		if(Util::getAway()) {
			if(!(BOOLSETTING(NO_AWAYMSG_TO_BOTS) && user->isSet(User::BOT)))
				p->sendMessage(Text::toT(Util::getAwayMessage()));
		}
		WinUtil::playSound(SettingsManager::SOUND_PM_WINDOW);
	} else {
		i->second->addChat(aMessage);
		WinUtil::playSound(SettingsManager::SOUND_PM);
	}
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

const StringMap PrivateFrame::getWindowParams() const {
	StringMap ret;
	ret[WindowInfo::title] = Text::fromT(getText());
	ret[WindowInfo::cid] = replyTo.getUser()->getCID().toBase32();
	ret["Hub"] = hubHint;
	ret["LogPath"] = getLogPath();
	return ret;
}

void PrivateFrame::parseWindowParams(dwt::TabView* parent, const StringMap& params) {
	StringMap::const_iterator cid = params.find(WindowInfo::cid);
	StringMap::const_iterator hub = params.find("Hub");
	if(cid != params.end() && hub != params.end()) {
		StringMap::const_iterator logPath = params.find("LogPath");
		openWindow(parent, ClientManager::getInstance()->getUser(CID(cid->second)), Util::emptyStringT, hub->second,
			logPath != params.end() ? logPath->second : Util::emptyString);
	}
}

bool PrivateFrame::isFavorite(const StringMap& params) {
	StringMap::const_iterator cid = params.find(WindowInfo::cid);
	if(cid != params.end()) {
		UserPtr u = ClientManager::getInstance()->getUser(CID(cid->second));
		if(u)
			return FavoriteManager::getInstance()->isFavoriteUser(u);
	}
	return false;
}

PrivateFrame::PrivateFrame(dwt::TabView* mdiParent, const UserPtr& replyTo_, bool activate, const string& hubHint_,
						   const string& logPath) :
	BaseType(mdiParent, _T(""), IDH_PM, IDR_PRIVATE, activate),
	replyTo(replyTo_),
	hubHint(hubHint_),
	priv(FavoriteManager::getInstance()->isPrivate(hubHint)),
	online(replyTo.getUser()->isOnline())
{
	chat->setHelpId(IDH_PM_CHAT);
	addWidget(chat);

	message->setHelpId(IDH_PM_MESSAGE);
	addWidget(message, true);
	message->onKeyDown(std::tr1::bind(&PrivateFrame::handleMessageKeyDown, this, _1));
	message->onSysKeyDown(std::tr1::bind(&PrivateFrame::handleMessageKeyDown, this, _1));
	message->onChar(std::tr1::bind(&PrivateFrame::handleMessageChar, this, _1));

	initStatus();
	status->onDblClicked(STATUS_STATUS, std::tr1::bind(&PrivateFrame::openLog, this));

	updateOnlineStatus();
	layout();

	readLog(logPath);

	ClientManager::getInstance()->addListener(this);

	callAsync(std::tr1::bind(&PrivateFrame::updateOnlineStatus, this));

	frames.insert(std::make_pair(replyTo.getUser(), this));

	addRecent();
}

PrivateFrame::~PrivateFrame() {
	frames.erase(replyTo.getUser());
}

void PrivateFrame::addChat(const tstring& aLine, bool log) {
	/// @todo null clients are allowed (eg to display log history on opening), fix later
	Client* pClient = 0;
	OnlineUser *ou = ClientManager::getInstance()->findOnlineUser(*replyTo.getUser(), Util::emptyString);
	if (ou) pClient = &(ou->getClient()); // getClient actually retuns a ref.

	ChatType::addChat(pClient, aLine);

	if(log && BOOLSETTING(LOG_PRIVATE_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(aLine);
		fillLogParams(params);
		LOG(LogManager::PM, params);
	}

	setDirty(SettingsManager::BOLD_PM);
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

void PrivateFrame::readLog(const string& logPath) {
	if(SETTING(SHOW_LAST_LINES_LOG) == 0)
		return;

	StringList lines;

	try {
		File f(logPath.empty() ? getLogPath() : logPath, File::READ, File::OPEN);
		if(f.getSize() > 32*1024) {
			f.setEndPos(- 32*1024 + 1);
		}

		lines = StringTokenizer<string>(f.read(32*1024), "\r\n").getTokens();

		f.close();
	} catch(const FileException&) { }

	if(lines.empty())
		return;

	// the last line in the log file is an empty line; remove it
	lines.pop_back();

	size_t linesCount = lines.size();
	for(size_t i = std::max(static_cast<int>(linesCount) - SETTING(SHOW_LAST_LINES_LOG), 0); i < linesCount; ++i) {
		addStatus(_T("- ") + Text::toT(lines[i]), false);
	}
}

void PrivateFrame::fillLogParams(StringMap& params) const {
	params["hubNI"] = Util::toString(priv ? ClientManager::getInstance()->getHubNames(replyTo.getUser()->getCID(), hubHint) :
		ClientManager::getInstance()->getHubNames(replyTo.getUser()->getCID()));
	params["hubURL"] = priv ? hubHint : Util::toString(ClientManager::getInstance()->getHubs(replyTo.getUser()->getCID()));
	params["userCID"] = replyTo.getUser()->getCID().toBase32();
	params["userNI"] = (priv ? ClientManager::getInstance()->getNicks(replyTo.getUser()->getCID(), hubHint) :
		ClientManager::getInstance()->getNicks(replyTo.getUser()->getCID()))[0];
	params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
}

void PrivateFrame::layout() {
	bool scroll = chat->scrollIsAtEnd();

	const int border = 2;

	dwt::Rectangle r(getClientAreaSize());

	status->layout(r);

	int ymessage = message->getTextSize(_T("A")).y + 10;
	dwt::Rectangle rm(0, r.size.y - ymessage, r.width() , ymessage);
	message->setBounds(rm);

	r.size.y -= rm.size.y + border;
	chat->setBounds(r);

	if(scroll)
		chat->sendMessage(WM_VSCROLL, SB_BOTTOM);
}

void PrivateFrame::updateOnlineStatus() {
	pair<tstring, bool> hubs = priv ? WinUtil::getHubNames(replyTo.getUser(), hubHint) : WinUtil::getHubNames(replyTo.getUser());

	setText((priv ? WinUtil::getNicks(replyTo.getUser(), hubHint) : WinUtil::getNicks(replyTo.getUser())) + _T(" - ") + hubs.first);

	online = hubs.second;
	setIcon(online ? IDR_PRIVATE : IDR_PRIVATE_OFF);
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
			handleGrantSlot(hubHint);
			addStatus(T_("Slot granted"));
		} else if(Util::stricmp(cmd.c_str(), _T("close")) == 0) {
			postMessage(WM_CLOSE);
		} else if((Util::stricmp(cmd.c_str(), _T("favorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("fav")) == 0)) {
			handleAddFavorite();
			addStatus(T_("Favorite user added"));
		} else if(Util::stricmp(cmd.c_str(), _T("getlist")) == 0) {
			handleGetList(hubHint);
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
	ClientManager::getInstance()->privateMessage(replyTo.getUser(), Text::fromT(msg), thirdPerson, hubHint);
}

PrivateFrame::UserInfoList PrivateFrame::selectedUsersImpl() {
	return UserInfoList(1, &replyTo);
}

void PrivateFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) throw() {
	if(aUser.getUser() == replyTo.getUser())
		callAsync(std::tr1::bind(&PrivateFrame::updateOnlineStatus, this));
}
void PrivateFrame::on(ClientManagerListener::UserConnected, const UserPtr& aUser) throw() {
	if(aUser == replyTo.getUser())
		callAsync(std::tr1::bind(&PrivateFrame::updateOnlineStatus, this));
}
void PrivateFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
	if(aUser == replyTo.getUser())
		callAsync(std::tr1::bind(&PrivateFrame::updateOnlineStatus, this));
}

void PrivateFrame::tabMenuImpl(dwt::MenuPtr& menu) {
	appendUserItems(getParent(), menu, hubHint, false, false);
	prepareMenu(menu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(replyTo.getUser()->getCID()));
	menu->appendSeparator();
}

void PrivateFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	ClientManager::getInstance()->userCommand(replyTo.getUser(), uc, ucParams, true);
}
