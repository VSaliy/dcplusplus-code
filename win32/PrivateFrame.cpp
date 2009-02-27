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
#include "resource.h"

#include <dcpp/ClientManager.h>
#include <dcpp/Client.h>
#include <dcpp/LogManager.h>
#include <dcpp/User.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/UploadManager.h>
#include <dcpp/QueueItem.h>
#include <dcpp/QueueManager.h>

PrivateFrame::FrameMap PrivateFrame::frames;

void PrivateFrame::openWindow(dwt::TabView* mdiParent, const UserPtr& replyTo_, const tstring& msg, const string& hubHint) {
	PrivateFrame* pf = 0;
	FrameIter i = frames.find(replyTo_);
	if(i == frames.end()) {
		pf = new PrivateFrame(mdiParent, replyTo_, true, hubHint);
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
		::PostMessage(i->second->handle(), WM_CLOSE, 0, 0);
}

void PrivateFrame::closeAllOffline() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i) {
		if(!i->first->isOnline())
			::PostMessage(i->second->handle(), WM_CLOSE, 0, 0);
	}
}

PrivateFrame::PrivateFrame(dwt::TabView* mdiParent, const UserPtr& replyTo_, bool activate, const string& hubHint_) :
	BaseType(mdiParent, _T(""), IDH_PM, IDR_PRIVATE, activate),
	replyTo(replyTo_),
	hubHint(hubHint_)
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

	updateTitle();
	layout();

	readLog();

	ClientManager::getInstance()->addListener(this);

	callAsync(std::tr1::bind(&PrivateFrame::updateTitle, this));

	frames.insert(std::make_pair(replyTo, this));
}

PrivateFrame::~PrivateFrame() {
	frames.erase(replyTo);
}

void PrivateFrame::addChat(const tstring& aLine, bool log) {
	ChatType::addChat(aLine);

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

void PrivateFrame::openLog() {
	StringMap params;
	fillLogParams(params);
	WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::PM, params))));
}

void PrivateFrame::readLog() {
	if(SETTING(SHOW_LAST_LINES_LOG) == 0)
		return;

	StringMap params;
	fillLogParams(params);
	string path = Util::validateFileName(LogManager::getInstance()->getPath(LogManager::PM, params));

	StringList lines;

	try {
		File f(path, File::READ, File::OPEN);
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

void PrivateFrame::fillLogParams(StringMap& params) {
	params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(replyTo->getCID()));
	params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(replyTo->getCID()));
	params["userCID"] = replyTo->getCID().toBase32();
	params["userNI"] = ClientManager::getInstance()->getNicks(replyTo->getCID())[0];
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

void PrivateFrame::updateTitle() {
	pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo);
	setText((WinUtil::getNicks(replyTo) + _T(" - ") + hubs.first));
	setIcon(hubs.second ? IDR_PRIVATE : IDR_PRIVATE_OFF);
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
			UploadManager::getInstance()->reserveSlot(replyTo, hubHint);
			addStatus(T_("Slot granted"));
		} else if(Util::stricmp(cmd.c_str(), _T("close")) == 0) {
			postMessage(WM_CLOSE);
		} else if((Util::stricmp(cmd.c_str(), _T("favorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("fav")) == 0)) {
			FavoriteManager::getInstance()->addFavoriteUser(replyTo);
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
		if(replyTo->isOnline()) {
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
	ClientManager::getInstance()->privateMessage(replyTo, Text::fromT(msg), thirdPerson, hubHint);
}

void PrivateFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) throw() {
	if(aUser.getUser() == replyTo)
		callAsync(std::tr1::bind(&PrivateFrame::updateTitle, this));
}
void PrivateFrame::on(ClientManagerListener::UserConnected, const UserPtr& aUser) throw() {
	if(aUser == replyTo)
		callAsync(std::tr1::bind(&PrivateFrame::updateTitle, this));
}
void PrivateFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
	if(aUser == replyTo)
		callAsync(std::tr1::bind(&PrivateFrame::updateTitle, this));
}

void PrivateFrame::tabMenuImpl(dwt::MenuPtr& menu) {
	menu->appendItem(T_("&Get file list"), std::tr1::bind(&PrivateFrame::handleGetList, this));
	menu->appendItem(T_("&Match queue"), std::tr1::bind(&PrivateFrame::handleMatchQueue, this));
	menu->appendItem(T_("Grant &extra slot"), std::tr1::bind(&UploadManager::reserveSlot, UploadManager::getInstance(), replyTo, hubHint));
	if(!FavoriteManager::getInstance()->isFavoriteUser(replyTo))
		menu->appendItem(T_("Add To &Favorites"), std::tr1::bind(&FavoriteManager::addFavoriteUser, FavoriteManager::getInstance(), replyTo), dwt::IconPtr(new dwt::Icon(IDR_FAVORITE_USERS)));

	prepareMenu(menu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(replyTo->getCID()));

	menu->appendSeparator();
}

void PrivateFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	ClientManager::getInstance()->userCommand(replyTo, uc, ucParams, true);
}

void PrivateFrame::handleGetList() {
	try {
		QueueManager::getInstance()->addList(replyTo, hubHint, QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception& e) {
		addStatus(Text::toT(e.getError()));
	}
}

void PrivateFrame::handleMatchQueue() {
	try {
		QueueManager::getInstance()->addList(replyTo, hubHint, QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception& e) {
		addStatus(Text::toT(e.getError()));
	}
}
