/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#include "resource.h"

#include "WaitingUsersFrame.h"

#include <dcpp/UploadManager.h>
#include "UserInfoBase.h"
#include "WinUtil.h"

const string WaitingUsersFrame::id = "WaitingUsers";
const string& WaitingUsersFrame::getId() const { return id; }

// Constructor
WaitingUsersFrame::WaitingUsersFrame(dwt::TabView* mdiParent) :
	BaseType(mdiParent, T_("Waiting Users"), IDH_WAITING_USERS, IDR_WAITING_USERS)
{
	UploadManager::getInstance()->addListener(this);

	// Create tree control
	{
		queued = addChild(WinUtil::Seeds::treeView);
		addWidget(queued);
		queued->onContextMenu(std::tr1::bind(&WaitingUsersFrame::handleContextMenu, this, _1));
		queued->onChar(std::tr1::bind(&WaitingUsersFrame::handleChar, this, _1));
	}

	initStatus();

	layout();

	// Load all waiting users & files.
	loadAll();
}

// Recalculate frame control layout
void WaitingUsersFrame::layout() {
	dwt::Rectangle r(this->getClientAreaSize());

	status->layout(r);

	queued->setBounds(r);
}

bool WaitingUsersFrame::preClosing() {
	UploadManager::getInstance()->removeListener(this);
	return true;
}

void WaitingUsersFrame::postClosing() {
	for(HTREEITEM userNode = queued->getRoot(); userNode; userNode = queued->getNextSibling(userNode)) {
		delete reinterpret_cast<UserInfoBase*>(queued->getData(userNode));
	}
}

bool WaitingUsersFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = queued->getContextMenuPos();
	} else {
		queued->select(pt);
	}

	MenuPtr menu = addChild(WinUtil::Seeds::menu);
	appendUserItems(getParent(), menu, Util::emptyString);
	menu->appendSeparator();
	menu->appendItem(T_("&Copy filename"), std::tr1::bind(&WaitingUsersFrame::onCopyFilename, this));
	menu->appendItem(T_("&Remove"), std::tr1::bind(&WaitingUsersFrame::onRemove, this));

	menu->open(pt);
	return true;
}

// Load all searches from manager
void WaitingUsersFrame::loadAll()
{
	// Load queue
	UserList users = UploadManager::getInstance()->getWaitingUsers();
	for (UserList::iterator uit = users.begin(); uit != users.end(); ++uit) {
		HTREEITEM lastInserted = queued->insert(
			(WinUtil::getNicks(*uit) + _T(" - ") + WinUtil::getHubNames(*uit).first),
			NULL, (LPARAM)(new UserInfoBase(*uit)));
		UploadManager::FileSet files = UploadManager::getInstance()->getWaitingUserFiles(*uit);
		for (UploadManager::FileSet::const_iterator fit = files.begin(); fit != files.end(); ++fit) {
			queued->insert(Text::toT(*fit), lastInserted);
		}
	}
}

HTREEITEM WaitingUsersFrame::getParentItem() const {
	HTREEITEM sel = queued->getSelected();
	if(!sel) {
		return NULL;
	}
	HTREEITEM parent = queued->getParent(sel);
	return parent ? parent : sel;
}

UserInfoBase* WaitingUsersFrame::getSelectedUser() const {
	HTREEITEM selectedItem = getParentItem();
	return selectedItem ? reinterpret_cast<UserInfoBase*>(queued->getData(selectedItem)) : 0;
}

void WaitingUsersFrame::onCopyFilename() {
	HTREEITEM selectedItem = queued->getSelected(), parentItem = getParentItem();

	if (!selectedItem || !parentItem || selectedItem == parentItem)
		return;

	tstring txt = queued->getText(selectedItem);
	tstring::size_type i = txt.find(_T('('));
	if(i != tstring::npos && i > 0) {
		txt.erase(i - 1);
	}
	if(!txt.empty()) {
		WinUtil::setClipboard(txt);
	}
}

// Remove queued item
void WaitingUsersFrame::onRemove()
{
	UserInfoBase* user = getSelectedUser();
	if (user) {
		UploadManager::getInstance()->clearUserFiles(user->getUser());
	}
}

WaitingUsersFrame::UserInfoList WaitingUsersFrame::selectedUsersImpl() const {
	UserInfoList users;
	UserInfoBase* user = getSelectedUser();
	if(user)
		users.push_back(user);
	return users;
}

// UploadManagerListener
void WaitingUsersFrame::on(UploadManagerListener::WaitingAddFile, const UserPtr& aUser, const string& aFilename) throw() {
	callAsync(std::tr1::bind(&WaitingUsersFrame::onAddFile, this, aUser, aFilename));
}
void WaitingUsersFrame::on(UploadManagerListener::WaitingRemoveUser, const UserPtr& aUser) throw() {
	callAsync(std::tr1::bind(&WaitingUsersFrame::onRemoveUser, this, aUser));
}

// Keyboard shortcuts
bool WaitingUsersFrame::handleChar(int c) {
	if(c == VK_DELETE) {
		onRemove();
		return true;
	}
	return false;
}

void WaitingUsersFrame::onRemoveUser(const UserPtr& aUser) {
	HTREEITEM userNode = queued->getRoot();

	while (userNode) {
		UserInfoBase* u = reinterpret_cast<UserInfoBase*>(queued->getData(userNode));
		if (aUser == u->getUser()) {
			delete u;
			queued->erase(userNode);
			return;
		}
		userNode = queued->getNextSibling(userNode);
	}

	setDirty(SettingsManager::BOLD_WAITING_USERS);
}

void WaitingUsersFrame::onAddFile(const UserPtr& aUser, const string& aFile) {
	HTREEITEM userNode = queued->getRoot();

	string fname = aFile.substr(0, aFile.find(_T('(')));

	while (userNode) {
		if (aUser == reinterpret_cast<UserInfoBase*>(queued->getData(userNode))->getUser()) {
			HTREEITEM childNode = queued->getChild(userNode);
			while (childNode) {
				tstring buf = queued->getText(childNode);

				if (fname == Text::fromT(buf.substr(0, buf.find(_T('('))))) {
					delete reinterpret_cast<UserPtr *>(queued->getData(childNode));
					queued->erase(childNode);
					break;
				}
				childNode = queued->getNextSibling(childNode);
			}

			//file isn't already listed, add it
			queued->insert(Text::toT(aFile), userNode, (LPARAM)new UserInfoBase(aUser));

			return;
		}

		userNode = queued->getNextSibling(userNode);
	}

	userNode = queued->insert(WinUtil::getNicks(aUser) + _T(" - ") + WinUtil::getHubNames(aUser).first,
		NULL, (LPARAM)new UserInfoBase(aUser));
	queued->insert(Text::toT(aFile), userNode);
	queued->expand(userNode);

	setDirty(SettingsManager::BOLD_WAITING_USERS);
}
