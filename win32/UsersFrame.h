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

#ifndef DCPLUSPLUS_WIN32_USERS_FRAME_H
#define DCPLUSPLUS_WIN32_USERS_FRAME_H

#include <dcpp/FavoriteManagerListener.h>
#include <dcpp/ClientManagerListener.h>

#include "StaticFrame.h"
#include "WinUtil.h"
#include "TypedTable.h"

#include "UserInfoBase.h"
#include "resource.h"

class UsersFrame :
	public StaticFrame<UsersFrame>,
	private FavoriteManagerListener,
	private ClientManagerListener,
	public AspectUserInfo<UsersFrame>
{
	typedef StaticFrame<UsersFrame> BaseType;

	friend class StaticFrame<UsersFrame>;
	friend class MDIChildFrame<UsersFrame>;
	friend class AspectUserInfo<UsersFrame>;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	UsersFrame(TabViewPtr parent);
	virtual ~UsersFrame();

	void layout();

	bool preClosing();
	void postClosing();

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FAVORITE = COLUMN_FIRST,
		COLUMN_SLOT,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_SEEN,
		COLUMN_DESCRIPTION,
		COLUMN_CID,
		COLUMN_LAST
	};

	enum {
		FAVORITE_OFF_ICON,
		FAVORITE_ON_ICON,
		GRANT_OFF_ICON,
		GRANT_ON_ICON
	};

	class UserInfo : public UserInfoBase {
	public:
		UserInfo(const UserPtr& u);
		UserInfo(const FavoriteUser& u);

		const tstring& getText(int col) const {
			return columns[col];
		}

		int getImage(int col) const {
			switch(col) {
			case COLUMN_FAVORITE: return isFavorite ? FAVORITE_ON_ICON : FAVORITE_OFF_ICON;
			case COLUMN_SLOT: return grantSlot ? GRANT_ON_ICON : GRANT_OFF_ICON;
			default: return -1;
			}
		}

		static int compareItems(UserInfo* a, UserInfo* b, int col) {
			switch(col) {
			case COLUMN_FAVORITE: return compare(a->isFavorite, b->isFavorite);
			case COLUMN_SLOT: return compare(a->grantSlot, b->grantSlot);
			default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
			}
		}

		void remove();

		void update(const UserPtr& u);
		void update(const FavoriteUser& u);

		tstring columns[COLUMN_LAST];

		bool isFavorite;
		bool grantSlot;
	};

	typedef TypedTable<UserInfo> WidgetUsers;
	typedef WidgetUsers* WidgetUsersPtr;
	WidgetUsersPtr users;
	GridPtr userInfo;
	VSplitterPtr splitter;
	static dwt::ImageListPtr userIcons;

	std::unordered_map<UserPtr, UserInfo*, User::Hash> userInfos;

	bool startup;

	void addUser(const UserPtr& aUser);
	void updateUser(const UserPtr& aUser);
	void removeUser(const UserPtr& aUser);

	void handleDescription();
	void handleRemove();
	bool handleKeyDown(int c);
	LRESULT handleItemChanged(LPARAM lParam);
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	void handleSelectionChanged();
	bool handleClick(const dwt::MouseEvent &me);

	// AspectUserInfo
	UserInfoList selectedUsersImpl() const;

	// FavoriteManagerListener
	virtual void on(UserAdded, const FavoriteUser& aUser) throw();
	virtual void on(UserRemoved, const FavoriteUser& aUser) throw();
	virtual void on(StatusChanged, const UserPtr& aUser) throw();

	// ClientManagerListner
	virtual void on(UserConnected, const UserPtr& aUser) throw();
	virtual void on(UserDisconnected, const UserPtr& aUser) throw();
};

#endif // !defined(USERS_FRAME_H)
