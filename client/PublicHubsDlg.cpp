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

#include "PublicHubsDlg.h"

void PublicHubsDlg::onHub(const string& aName, const string& aServer, const string& aDescription, const string& aUsers) {
	StringList l;
	l.push_back(aName);
	l.push_back(aDescription);
	l.push_back(aUsers);
	l.push_back(aServer);
	ctrlHubs.insertItem(l);
}

LRESULT PublicHubsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ctrlHubs.Attach(GetDlgItem(IDC_HUBLIST));
	ctrlHubs.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 200, 0);
	ctrlHubs.InsertColumn(1, _T("Description"), LVCFMT_LEFT, 290, 1);
	ctrlHubs.InsertColumn(2, _T("Users"), LVCFMT_RIGHT, 50, 2);
	ctrlHubs.InsertColumn(3, _T("Server"), LVCFMT_LEFT, 100, 3);
	
	ctrlHubs.setSort(2, ExListViewCtrl::SORT_INT, false);
	SetDlgItemText(IDC_SERVER, server.c_str());

	HubManager::getInstance()->addListener(this);
	HubManager::getInstance()->getPublicHubs();
	
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT PublicHubsDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		char buf[1024];
		GetDlgItemText(IDC_SERVER, buf, 1024);
		server = buf;
	}
	wId = wID;
	DWORD tId;
	CreateThread(NULL, 0, &stopper, this, NULL, &tId);
	return 0;
}

LRESULT PublicHubsDlg::OnItemchangedHublist(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	
	if(lv->uNewState & LVIS_FOCUSED) {
		char buf[1024];
		ctrlHubs.GetItemText(lv->iItem, 3, buf, 1024);
		SetDlgItemText(IDC_SERVER, buf);
	}
	return 0;
}

LRESULT PublicHubsDlg::OnClickedRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	ctrlHubs.DeleteAllItems();
	listing = true;
	HubManager::getInstance()->getPublicHubs(true);

	return 0;
}

DWORD WINAPI PublicHubsDlg::stopper(void* p) {
	PublicHubsDlg* phd = (PublicHubsDlg*) p;

	HubManager::getInstance()->removeListener(phd);
	if(phd->wId != -1)
		phd->EndDialog(phd->wId);
	return 0;
}
/**
 * @file PublicHubsDlg.cpp
 * $Id: PublicHubsDlg.cpp,v 1.3 2001/11/25 22:06:25 arnetheduck Exp $
 * @if LOG
 * $Log: PublicHubsDlg.cpp,v $
 * Revision 1.3  2001/11/25 22:06:25  arnetheduck
 * Finally downloading is working! There are now a few quirks and bugs to be fixed
 * but what the heck....!
 *
 * Revision 1.2  2001/11/24 10:31:45  arnetheduck
 * Added hub list sorting and fixed deadlock bugs (hopefully...).
 *
 * Revision 1.1.1.1  2001/11/21 17:33:20  arnetheduck
 * Inital release
 *
 * @endif
 */

