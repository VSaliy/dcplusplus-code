/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#if !defined(AFX_PRIVATEFRAME_H__8F6D05EC_ADCF_4987_8881_6DF3C0E355FA__INCLUDED_)
#define AFX_PRIVATEFRAME_H__8F6D05EC_ADCF_4987_8881_6DF3C0E355FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/User.h"
#include "../client/CriticalSection.h"
#include "../client/ClientManagerListener.h"

#include "FlatTabCtrl.h"
#include "WinUtil.h"

#define PM_MESSAGE_MAP 8		// This could be any number, really...

class PrivateFrame : public MDITabChildWindowImpl<PrivateFrame, RGB(0, 255, 255)>, private ClientManagerListener
{
public:
	static void gotMessage(const User::Ptr& aUser, const string& aMessage);
	static void openWindow(const User::Ptr& aUser);
	static bool isOpen(const User::Ptr u) { return frames.find(u) != frames.end(); };

	enum {
		USER_UPDATED
	};

	DECLARE_FRAME_WND_CLASS_EX("PrivateFrame", IDR_PRIVATE, 0, COLOR_3DFACE);

	virtual void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}

	typedef MDITabChildWindowImpl<PrivateFrame, RGB(0, 255, 255)> baseClass;

	BEGIN_MSG_MAP(PrivateFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_SEND_MESSAGE, onSendMessage)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(PM_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	
	void addLine(const string& aLine);
	void onEnter();
	void UpdateLayout(BOOL bResizeBars = TRUE);	
	
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	
	LRESULT onSendMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onEnter();
		return 0;
	}

	LRESULT PrivateFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		updateTitle();
		return 0;
	}
	
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		if(hWnd == ctrlClient.m_hWnd || hWnd == ctrlMessage.m_hWnd) {
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
		}
		bHandled = FALSE;
		return FALSE;
	};

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlMessage.SetFocus();
		return 0;
	}
	
	void addClientLine(const string& aLine) {
		if(!created) {
			CreateEx(WinUtil::mdiClient);
		}
		ctrlStatus.SetText(0, ("[" + Util::getShortTimeString() + "] " + aLine).c_str());
		setDirty();
	}
	
	void setUser(const User::Ptr& aUser) { user = aUser; };
	void sendMessage(const string& msg) {
		if(user && user->isOnline()) {
			string s = "<" + user->getClientNick() + "> " + msg;
			user->privateMessage(s);
			addLine(s);
		}
	}
	
	User::Ptr& getUser() { return user; };
private:
	PrivateFrame(const User::Ptr& aUser) : user(aUser), 
		created(false), closed(false), 
		ctrlMessageContainer("edit", this, PM_MESSAGE_MAP) {
	}
	
	~PrivateFrame() {
	}
	
	bool created;
	typedef HASH_MAP<User::Ptr, PrivateFrame*, User::HashFunction> FrameMap;
	typedef FrameMap::iterator FrameIter;
	static FrameMap frames;
	CEdit ctrlClient;
	CEdit ctrlMessage;
	CStatusBarCtrl ctrlStatus;
	static CriticalSection cs;

	User::Ptr user;
	CContainedWindow ctrlMessageContainer;

	bool closed;

	void updateTitle() {
		if(user->isOnline()) {
			SetWindowText(user->getFullNick().c_str());
			setTabColor(RGB(0, 255, 255));
		} else {
			if(user->getClientName() == STRING(OFFLINE)) {
				SetWindowText(user->getFullNick().c_str());
			} else {
				SetWindowText((user->getFullNick() + " [" + STRING(OFFLINE) + "]").c_str());
			}
			setTabColor(RGB(255, 0, 0));
		}
	}
	
	// ClientManagerListener
	virtual void onAction(ClientManagerListener::Types type, const User::Ptr& aUser) throw();
};

#endif // !defined(AFX_PRIVATEFRAME_H__8F6D05EC_ADCF_4987_8881_6DF3C0E355FA__INCLUDED_)

/**
 * @file
 * $Id: PrivateFrame.h,v 1.12 2003/10/08 21:55:11 arnetheduck Exp $
 */

