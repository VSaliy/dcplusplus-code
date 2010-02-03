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

#ifndef DCPLUSPLUS_WIN32_HUB_FRAME_H
#define DCPLUSPLUS_WIN32_HUB_FRAME_H

#include "MDIChildFrame.h"
#include "IRecent.h"
#include "AspectChat.h"
#include "TypedTable.h"
#include "AspectUserCommand.h"
#include "UserInfoBase.h"

#include <dcpp/forward.h>
#include <dcpp/ClientListener.h>
#include <dcpp/TaskQueue.h>
#include <dcpp/User.h>
#include <dcpp/FavoriteManagerListener.h>

class HubFrame :
	public MDIChildFrame<HubFrame>,
	public IRecent<HubFrame>,
	private ClientListener,
	private FavoriteManagerListener,
	public AspectChat<HubFrame>,
	public AspectUserInfo<HubFrame>,
	public AspectUserCommand<HubFrame>
{
	typedef MDIChildFrame<HubFrame> BaseType;
	typedef AspectChat<HubFrame> ChatType;

	friend class MDIChildFrame<HubFrame>;
	friend class AspectChat<HubFrame>;
	friend class AspectUserInfo<HubFrame>;
	friend class AspectUserCommand<HubFrame>;

	typedef IRecent<HubFrame> RecentType;
	using RecentType::setText;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_CIPHER,
		STATUS_USERS,
		STATUS_SHARED,
		STATUS_AVERAGE_SHARED,
		STATUS_SHOW_USERS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

	static void openWindow(dwt::TabView* mdiParent, const string& url);
	static void closeDisconnected();
	static void closeFavGroup(const string& group, bool reversed);
	static void resortUsers();

	const StringMap getWindowParams() const;
	static void parseWindowParams(dwt::TabView* parent, const StringMap& params);
	static bool isFavorite(const StringMap& params);

private:
	friend class MainWindow;

	enum FilterModes{
		NONE,
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL
	};

	enum {
		IMAGE_USER = 0, IMAGE_OP
	};

	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST,
		COLUMN_SHARED,
		COLUMN_DESCRIPTION,
		COLUMN_TAG,
		COLUMN_CONNECTION,
		COLUMN_IP,
		COLUMN_EMAIL,
		COLUMN_CID,
		COLUMN_LAST
	};

	enum Tasks {
		UPDATE_USER_JOIN, UPDATE_USER, REMOVE_USER
	};

	struct UserTask : public Task {
		UserTask(const OnlineUser& ou);

		HintedUser user;
		Identity identity;
	};

	class UserInfo : public UserInfoBase, public FastAlloc<UserInfo> {
	public:
		UserInfo(const UserTask& u) : UserInfoBase(u.user) {
			update(u.identity, -1);
		}

		const tstring& getText(int col) const {
			return columns[col];
		}
		int getImage() const;

		static int compareItems(const UserInfo* a, const UserInfo* b, int col);
		bool update(const Identity& identity, int sortCol);

		string getNick() const { return identity.getNick(); }
		bool isHidden() const { return identity.isHidden(); }

		tstring columns[COLUMN_LAST];
		GETSET(Identity, identity, Identity);
	};

	typedef unordered_map<UserPtr, UserInfo*, User::Hash> UserMap;
	typedef UserMap::iterator UserMapIter;

	struct CountAvailable {
		CountAvailable() : available(0) { }
		int64_t available;
		void operator()(UserInfo *ui) {
			available += ui->getIdentity().getBytesShared();
		}
		void operator()(UserMap::const_reference ui) {
			available += ui.second->getIdentity().getBytesShared();
		}
	};

	WidgetVPanedPtr paned;

	GridPtr userGrid;

	typedef TypedTable<UserInfo, false> WidgetUsers;
	typedef WidgetUsers* WidgetUsersPtr;
	WidgetUsersPtr users;

	TextBoxPtr filter;
	ComboBoxPtr filterType;
	CheckBoxPtr showUsers;

	UserMap userMap;

	Client* client;
	string url;
	string redirect;
	bool updateUsers;
	bool waitingForPW;
	bool resort;
	bool showJoins;
	bool favShowJoins;

	TaskQueue tasks; // todo get rid of TaskQueue

	tstring filterString;

	StringMap ucLineParams;
	bool inTabMenu;

	tstring complete;
	StringList tabCompleteNicks;
	bool inTabComplete;

	typedef std::vector<HubFrame*> FrameList;
	typedef FrameList::iterator FrameIter;
	static FrameList frames;

	HubFrame(dwt::TabView* mdiParent, const string& url);
	virtual ~HubFrame();

	void layout();
	bool preClosing();
	void postClosing();

	bool tab();

	void addChat(const tstring& aLine);
	void addStatus(const tstring& aLine, bool legitimate = true);

	tstring getStatusUsers() const;
	tstring getStatusShared() const;
	tstring getStatusAverageShared() const;
	void updateStatus();

	void initSecond();
	bool eachSecond();

	UserInfo* findUser(const tstring& nick);
	bool updateUser(const UserTask& u);
	void removeUser(const UserPtr& aUser);
	const tstring& getNick(const UserPtr& u);

	void updateUserList(UserInfo* ui = NULL);

	void clearUserList();
	void clearTaskList();

	void addAsFavorite();
	void removeFavoriteHub();

	void runUserCommand(const UserCommand& uc);

	bool handleMessageChar(int c);
	bool handleMessageKeyDown(int c);
	bool handleUsersKeyDown(int c);
	bool handleChatContextMenu(dwt::ScreenCoordinate pt);
	bool handleUsersContextMenu(dwt::ScreenCoordinate pt);
	void handleShowUsersClicked();
	void handleMultiCopy(unsigned index);
	void handleDoubleClickUsers();
	void handleCopyHub();
	void handleAddAsFavorite();
	void handleReconnect();
	void handleFollow();

	void handleFilterUpdated();
	bool parseFilter(FilterModes& mode, int64_t& size);
	bool matchFilter(const UserInfo& ui, int sel, bool doSizeCompare = false, FilterModes mode = NONE, int64_t size = 0);

	string getLogPath(bool status = false) const;
	void openLog(bool status = false);

	string stripNick(const string& nick) const;
	tstring scanNickPrefix(const tstring& prefix);

	// MDIChildFrame
	void tabMenuImpl(dwt::MenuPtr& menu);

	// AspectChat
	void enterImpl(const tstring& s);

	// AspectUserInfo
	UserInfoList selectedUsersImpl() const;

	void addTask(Tasks s, const OnlineUser& u);
	void execTasks();

	void onConnected();
	void onDisconnected();
	void onGetPassword();
	void onPrivateMessage(const UserPtr& from, const UserPtr& to, const UserPtr& replyTo, bool hub, bool bot, const tstring& m);

	// FavoriteManagerListener
	virtual void on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) throw();
	virtual void on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) throw();
	void resortForFavsFirst(bool justDoIt = false);

	// ClientListener
	virtual void on(Connecting, Client*) throw();
	virtual void on(Connected, Client*) throw();
	virtual void on(UserUpdated, Client*, const OnlineUser&) throw();
	virtual void on(UsersUpdated, Client*, const OnlineUserList&) throw();
	virtual void on(ClientListener::UserRemoved, Client*, const OnlineUser&) throw();
	virtual void on(Redirect, Client*, const string&) throw();
	virtual void on(Failed, Client*, const string&) throw();
	virtual void on(GetPassword, Client*) throw();
	virtual void on(HubUpdated, Client*) throw();
	virtual void on(Message, Client*, const ChatMessage&) throw();
	virtual void on(StatusMessage, Client*, const string&, int = ClientListener::FLAG_NORMAL) throw();
	virtual void on(NickTaken, Client*) throw();
	virtual void on(SearchFlood, Client*, const string&) throw();
	void onStatusMessage(const string& line, int flags);
};

#endif // !defined(HUB_FRAME_H)
