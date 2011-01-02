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

#ifndef DCPLUSPLUS_WIN32_WAITING_USERS_FRAME_H
#define DCPLUSPLUS_WIN32_WAITING_USERS_FRAME_H

#include "StaticFrame.h"
#include "UserInfoBase.h"

#include <dcpp/forward.h>
#include <dcpp/UploadManagerListener.h>
#include "resource.h"

/// @todo use typed controls & auto-manage UserInfoBase pointers
class WaitingUsersFrame :
	public StaticFrame<WaitingUsersFrame>,
	public AspectUserInfo<WaitingUsersFrame>,
	public UploadManagerListener
{
	typedef StaticFrame<WaitingUsersFrame> BaseType;

	friend class StaticFrame<WaitingUsersFrame>;
	friend class MDIChildFrame<WaitingUsersFrame>;
	friend class AspectUserInfo<WaitingUsersFrame>;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	// Constructor
	WaitingUsersFrame(dwt::TabView* mdiParent);
	virtual ~WaitingUsersFrame() { }

private:
	// Contained controls
	TreePtr queued;

	void layout();
	bool preClosing();
	void postClosing();

	// Message handlers
	void onGetList();
	void onCopyFilename();
	void onRemove();
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	bool handleChar(int c);

	HTREEITEM getParentItem() const;
	UserInfoBase* getSelectedUser() const;

	// Communication with manager
	void loadAll();
	void updateSearch(int index, BOOL doDelete = TRUE);

	// AspectUserInfo
	UserInfoList selectedUsersImpl() const;

	// UploadManagerListener
	virtual void on(UploadManagerListener::WaitingAddFile, const HintedUser&, const string&) throw();
	virtual void on(UploadManagerListener::WaitingRemoveUser, const HintedUser&) throw();
	void onRemoveUser(const UserPtr&);
	void onAddFile(const HintedUser&, const string&);
};

#endif	/* WAITING_QUEUE_FRAME_H */
