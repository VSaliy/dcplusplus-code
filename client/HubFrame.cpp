/* 
 * Copyright (C) 2001 Jacek Sieka, jacek@creatio.se
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
#include "DCPlusPlus.h"

#include "HubFrame.h"
#include "DownloadManager.h"

CImageList* HubFrame::images = NULL;

LRESULT HubFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE);

	ctrlClient.FmtLines(TRUE);
	ctrlClient.LimitText(0);
	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	
	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_USERS);

	ctrlClient.SetFont(ctrlUsers.GetFont());
	ctrlMessage.SetFont(ctrlUsers.GetFont());

	SetSplitterPanes(ctrlClient.m_hWnd, ctrlUsers.m_hWnd, false);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	m_nProportionalPos = 7500;

	ctrlUsers.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100, 0);
	ctrlUsers.InsertColumn(1, _T("Shared"), LVCFMT_LEFT, 75, 1);
	ctrlUsers.InsertColumn(2, _T("Description"), LVCFMT_LEFT, 100, 2);
	ctrlUsers.InsertColumn(3, _T("Connection"), LVCFMT_LEFT, 75, 3);
	ctrlUsers.InsertColumn(4, _T("E-Mail"), LVCFMT_LEFT, 100, 4);

	userMenu.CreatePopupMenu();
	opMenu.CreatePopupMenu();
	
	if(!images) {
		images = new CImageList();
		images->CreateFromImage(IDB_USERS, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_SHARED);
	}
	ctrlUsers.SetImageList(*images, LVSIL_SMALL);

	CMenuItemInfo mi;
	mi.fMask = MIIM_ID | MIIM_STRING;
	mi.dwTypeData = "Get File List";
	mi.wID = IDC_GETLIST;
	userMenu.InsertMenuItem(0, TRUE, &mi);
	opMenu.InsertMenuItem(0, TRUE, &mi);
	
	mi.fMask = MIIM_ID | MIIM_STRING;
	mi.dwTypeData = "Private Message";
	mi.wID = IDC_PRIVATEMESSAGE;
	userMenu.InsertMenuItem(1, TRUE, &mi);
	opMenu.InsertMenuItem(1, TRUE, &mi);
	
	mi.fMask = MIIM_TYPE;
	mi.fType = MFT_SEPARATOR;
	userMenu.InsertMenuItem(2, TRUE, &mi);
	opMenu.InsertMenuItem(2, TRUE, &mi);

	mi.fMask = MIIM_ID | MIIM_STRING;
	mi.dwTypeData = "Refresh User List";
	mi.wID = IDC_REFRESH;
	userMenu.InsertMenuItem(3, TRUE, &mi);
	opMenu.InsertMenuItem(3, TRUE, &mi);

	mi.fMask = MIIM_TYPE;
	mi.fType = MFT_SEPARATOR;
	opMenu.InsertMenuItem(4, TRUE, &mi);

	mi.fMask = MIIM_ID | MIIM_STRING;
	mi.dwTypeData = "Kick User";
	mi.wID = IDC_KICK;
	opMenu.InsertMenuItem(5, TRUE, &mi);

	mi.fMask = MIIM_ID | MIIM_STRING;
	mi.dwTypeData = "Redirect";
	mi.wID = IDC_REDIRECT;
	opMenu.InsertMenuItem(6, TRUE, &mi);
	
	bHandled = FALSE;
	client->connect(server);
	return 1;
}

LRESULT HubFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i=-1;
	char buf[256];
	if(client->isConnected()) {
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			ctrlUsers.GetItemText(i, 0, buf, 256);
			string user = buf;
			User::Ptr& u = client->getUser(user);
			try {
				if(u)
					DownloadManager::getInstance()->downloadList(u);
				else 
					DownloadManager::getInstance()->downloadList(user);
			} catch(...) {
				// ...
			}
		}
	}
	return 0;
}

LRESULT HubFrame::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i=-1;
	char buf[256];
	if(client->isConnected()) {
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			ctrlUsers.GetItemText(i, 0, buf, 256);
			string user = buf;
			User::Ptr& u = client->getUser(user);
			if(u) {
				PrivateFrame* frm = PrivateFrame::getFrame(u, m_hWndMDIClient);
				frm->setTab(getTab());
				frm->CreateEx(m_hWndMDIClient);
			}
		}
	}
	return 0;
}

LRESULT HubFrame::onDoubleClickUsers(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	string user;
	char buf[256];
	
	if(client->isConnected() && item->iItem != -1) {
		ctrlUsers.GetItemText(item->iItem, 0, buf, 256);
		user = buf;
		User::Ptr& u = client->getUser(user);
		try {
			if(u)
				DownloadManager::getInstance()->downloadList(u);
			else 
				DownloadManager::getInstance()->downloadList(user);
		} catch(...) {
			// ...
		}
	}
	return 0;
}

LRESULT HubFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	cs.enter();
	// First some specials to handle those messages that have to initialize variables...
	if(wParam == CLIENT_MESSAGE) {
		addLine(*(string*)lParam);
		delete (string*)lParam;
	} else if(wParam == CLIENT_MYINFO) {
		User::Ptr& u = *(User::Ptr*)lParam;
		LV_FINDINFO fi;
		fi.flags = LVFI_STRING;
		fi.psz = u->getNick().c_str();
		int j = ctrlUsers.FindItem(&fi, -1);
		if(j == -1) {
			UserInfo* ui = new UserInfo;
			ui->size = u->getBytesShared();
			StringList l;
			l.push_back(u->getNick());
			l.push_back(Util::formatBytes(u->getBytesSharedString()));
			l.push_back(u->getDescription());
			l.push_back(u->getConnection());
			l.push_back(u->getEmail());
			ctrlUsers.insert(l, u->isSet(User::OP) ? IMAGE_OP : IMAGE_USER, (LPARAM)ui);
		} else {
			ctrlUsers.SetItem(j, 0, LVIF_IMAGE, NULL, u->isSet(User::OP) ? IMAGE_OP : IMAGE_USER, 0, 0, NULL);
			ctrlUsers.SetItemText(j, 1, Util::formatBytes(u->getBytesShared()).c_str());
			ctrlUsers.SetItemText(j, 2, u->getDescription().c_str());
			ctrlUsers.SetItemText(j, 3, u->getConnection().c_str());
			ctrlUsers.SetItemText(j, 4, u->getEmail().c_str());
			((UserInfo*)ctrlUsers.GetItemData(j))->size = u->getBytesShared();
		}
		
		updateStatusBar();
		delete (User::Ptr*)lParam;
	} else if(wParam == CLIENT_QUIT) {
		User::Ptr& u = *(User::Ptr*)lParam;
		
		int item = ctrlUsers.find(u->getNick());
		if(item != -1) {
			delete (UserInfo*)ctrlUsers.GetItemData(item);
			ctrlUsers.DeleteItem(item);
		}
		updateStatusBar();		
		delete (User::Ptr*)lParam;
	} else if(wParam == CLIENT_GETPASSWORD) {
		LineDlg dlg;
		dlg.title = "Hub Password";
		dlg.description = "Please enter your password";
		dlg.password = true;
		
		if(dlg.DoModal() == IDOK) {
			client->password(dlg.line);
		} else {
			client->disconnect();
		}
	} else if(wParam == CLIENT_CONNECTING) {
		addClientLine("Connecting to " + client->getServer() + "...");
		SetWindowText(client->getServer().c_str());
	} else if(wParam == CLIENT_ERROR) {
		addClientLine(*(string*)lParam);
		delete (string*)lParam;
	} else if(wParam == CLIENT_HUBNAME) {
		SetWindowText(client->getName().c_str());
		addClientLine("Connected");
	} else if(wParam == CLIENT_VALIDATEDENIED) {
		addClientLine("Your nick was already taken, please change to something else!");
		client->disconnect();
	} else if(wParam == CLIENT_PRIVATEMESSAGE) {
		PMInfo* i = (PMInfo*)lParam;
		i->frm->Create(m_hWndMDIClient);
		i->frm->addLine(i->msg);
		delete i;
	}
	cs.leave();
	return 0;
};

/**
 * @file HubFrame.cpp
 * $Id: HubFrame.cpp,v 1.16 2002/01/06 21:55:20 arnetheduck Exp $
 * @if LOG
 * $Log: HubFrame.cpp,v $
 * Revision 1.16  2002/01/06 21:55:20  arnetheduck
 * Some minor bugs fixed, but there remains one strange thing, the reconnect
 * button doesn't work...
 *
 * Revision 1.15  2002/01/05 19:06:09  arnetheduck
 * Added user list images, fixed bugs and made things more effective
 *
 * Revision 1.13  2002/01/05 10:13:39  arnetheduck
 * Automatic version detection and some other updates
 *
 * Revision 1.12  2001/12/21 23:52:30  arnetheduck
 * Last commit for five days
 *
 * Revision 1.11  2001/12/16 19:47:48  arnetheduck
 * Reworked downloading and user handling some, and changed some small UI things
 *
 * Revision 1.10  2001/12/15 17:01:06  arnetheduck
 * Passive mode searching as well as some searching code added
 *
 * Revision 1.9  2001/12/13 19:21:57  arnetheduck
 * A lot of work done almost everywhere, mainly towards a friendlier UI
 * and less bugs...time to release 0.06...
 *
 * Revision 1.8  2001/12/08 20:59:26  arnetheduck
 * Fixing bugs...
 *
 * Revision 1.7  2001/12/08 14:25:49  arnetheduck
 * More bugs removed...did my first search as well...
 *
 * Revision 1.6  2001/12/07 20:03:07  arnetheduck
 * More work done towards application stability
 *
 * Revision 1.5  2001/12/05 19:40:13  arnetheduck
 * More bugfixes.
 *
 * Revision 1.4  2001/12/02 23:47:35  arnetheduck
 * Added the framework for uploading and file sharing...although there's something strange about
 * the file lists...my client takes them, but not the original...
 *
 * Revision 1.3  2001/11/26 23:40:36  arnetheduck
 * Downloads!! Now downloads are possible, although the implementation is
 * likely to change in the future...more UI work (splitters...) and some bug
 * fixes. Only user file listings are downloadable, but at least it's something...
 *
 * Revision 1.2  2001/11/24 10:37:09  arnetheduck
 * onQuit is now handled
 * User list sorting
 * File sizes correcly cut down to B, kB, MB, GB and TB
 *
 * Revision 1.1.1.1  2001/11/21 17:33:20  arnetheduck
 * Inital release
 *
 * @endif
 */

