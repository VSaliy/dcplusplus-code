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

#ifndef DCPLUSPLUS_WIN32_PRIVATE_FRAME_H
#define DCPLUSPLUS_WIN32_PRIVATE_FRAME_H

#include <dcpp/ClientManagerListener.h>
#include <dcpp/User.h>

#include "MDIChildFrame.h"
#include "IRecent.h"
#include "AspectChat.h"
#include "UserInfoBase.h"
#include "AspectUserCommand.h"

class PrivateFrame :
	public MDIChildFrame<PrivateFrame>,
	public IRecent<PrivateFrame>,
	private ClientManagerListener,
	public AspectChat<PrivateFrame>,
	public AspectUserInfo<PrivateFrame>,
	public AspectUserCommand<PrivateFrame>
{
	typedef MDIChildFrame<PrivateFrame> BaseType;
	typedef AspectChat<PrivateFrame> ChatType;

	friend class MDIChildFrame<PrivateFrame>;
	friend class AspectChat<PrivateFrame>;
	friend class AspectUserInfo<PrivateFrame>;
	friend class AspectUserCommand<PrivateFrame>;

	using IRecent<PrivateFrame>::setText;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

	static void gotMessage(TabViewPtr parent, const UserPtr& from, const UserPtr& to, const UserPtr& replyTo,
		const tstring& aMessage, const string& hubHint);
	static void openWindow(TabViewPtr parent, const HintedUser& replyTo, const tstring& msg = Util::emptyStringT,
		const string& logPath = Util::emptyString, bool activate = true);
	static void activateWindow(const UserPtr& u);
	static bool isOpen(const UserPtr& u) { return frames.find(u) != frames.end(); }
	static void closeAll(bool offline);

	WindowParams getWindowParams() const;
	static void parseWindowParams(TabViewPtr parent, const WindowParams& params);
	static bool isFavorite(const WindowParams& params);

	void sendMessage(const tstring& msg, bool thirdPerson = false);

private:
	GridPtr grid;
	GridPtr hubGrid;
	ComboBoxPtr hubBox;

	StringPairList hubs;
	ParamMap ucLineParams;

	UserInfoBase replyTo;
	const bool priv;
	bool online;

	typedef unordered_map<UserPtr, PrivateFrame*, User::Hash> FrameMap;
	static FrameMap frames;

	PrivateFrame(TabViewPtr parent, const HintedUser& replyTo_, const string& logPath = Util::emptyString);
	virtual ~PrivateFrame();

	void layout();
	bool preClosing();

	string getLogPath() const;
	void openLog();
	void fillLogParams(ParamMap& params) const;
	void addChat(const tstring& aLine, bool log = true);
	void addStatus(const tstring& aLine, bool log = true);
	void updateOnlineStatus();

	bool handleChatContextMenu(dwt::ScreenCoordinate pt);

	void runUserCommand(const UserCommand& uc);

	// MDIChildFrame
	void tabMenuImpl(dwt::Menu* menu);

	// AspectChat
	void enterImpl(const tstring& s);

	// AspectUserInfo
	UserInfoList selectedUsersImpl();

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept;
	virtual void on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept;
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;
};

#endif // !defined(PRIVATE_FRAME_H)
