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

#include "MainFrm.h"
#include "AboutDlg.h"
#include "HubFrame.h"
#include "SearchFrm.h"
#include "PublicHubsFrm.h"
#include "SettingsDlg.h"
#include "SimpleXML.h"

#include "ConnectionManager.h"
#include "DownloadManager.h"
#include "UploadManager.h"
#include "CryptoManager.h"
#include "DirectoryListing.h"
#include "DirectoryListingFrm.h"
#include "ShareManager.h"
#include "SearchManager.h"
#include "FavoritesFrm.h"

MainFrame::~MainFrame() {
	arrows.Destroy();
}

DWORD WINAPI MainFrame::stopper(void* p) {
	MainFrame* mf = (MainFrame*)p;
	HWND wnd, wnd2 = NULL;
	while( (wnd=::GetWindow(mf->m_hWndClient, GW_CHILD)) != NULL) {
		if(wnd == wnd2) 
			Sleep(1);
		else { 
			::SendMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}
	TimerManager::getInstance()->removeListeners();
	
	SettingsManager::getInstance()->save();
	SettingsManager::deleteInstance();

	ShareManager::deleteInstance();
	CryptoManager::deleteInstance();
	DownloadManager::deleteInstance();
	UploadManager::deleteInstance();
	SearchManager::deleteInstance();
	ConnectionManager::deleteInstance();
	ClientManager::deleteInstance();
	HubManager::deleteInstance();
	TimerManager::deleteInstance();
	mf->PostMessage(WM_CLOSE);	
	return 0;
}

LRESULT MainFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == UPLOAD_COMPLETE || wParam == UPLOAD_FAILED || wParam == DOWNLOAD_REMOVED) {
		ctrlTransfers.DeleteItem(ctrlTransfers.find(lParam));
	} else if(wParam == STATS) {
		StringList* str = (StringList*)lParam;
		if(ctrlStatus.IsWindow()) {
			ctrlStatus.SetText(1, (*str)[0].c_str());
			ctrlStatus.SetText(2, (*str)[1].c_str());
			ctrlStatus.SetText(3, (*str)[2].c_str());
			ctrlStatus.SetText(4, (*str)[3].c_str());
		}
		delete str;
	} else if(wParam == UPLOAD_STARTING) {
		StringListInfo* i = (StringListInfo*)lParam;
		ctrlTransfers.insert(i->l, IMAGE_UPLOAD, i->lParam);
		delete i;
	} else if(wParam == DOWNLOAD_ADDED) {
		StringListInfo* i = (StringListInfo*)lParam;
		ctrlTransfers.insert(i->l, IMAGE_DOWNLOAD, i->lParam);
		delete i;
	} else if(wParam == DOWNLOAD_FAILED) {
		StringListInfo* i = (StringListInfo*)lParam;
		int j = ctrlTransfers.find(i->lParam);
		ctrlTransfers.SetItemText(j, 1, i->l[0].c_str());
		ctrlTransfers.SetItemText(j, 3, i->l[1].c_str());
		delete i;
	} else if(wParam == UPLOAD_TICK || wParam == DOWNLOAD_TICK) {
		StringInfo* i = (StringInfo*)lParam;
		ctrlTransfers.SetItemText(ctrlTransfers.find(i->lParam), 1, i->str.c_str());
		delete i;
	} else if(wParam == DOWNLOAD_CONNECTING) {
		ctrlTransfers.SetItemText(ctrlTransfers.find(lParam), 1, "Connecting...");
	} else if(wParam == DOWNLOAD_STARTING) {
		StringListInfo* i = (StringListInfo*)lParam;
		int pos = ctrlTransfers.find(i->lParam);

		ctrlTransfers.SetItemText(pos, 2, i->l[0].c_str());
		ctrlTransfers.SetItemText(pos, 3, i->l[1].c_str());
	} else if(wParam == DOWNLOAD_SOURCEADDED) {
		StringInfo* i = (StringInfo*)lParam;
		ctrlTransfers.SetItemText(ctrlTransfers.find(i->lParam), 3, i->str.c_str());
		delete i;
	} else if(wParam == DOWNLOAD_LISTING) {
		StringListInfo* i = (StringListInfo*)lParam;
		ctrlTransfers.SetItemText(ctrlTransfers.find(i->lParam), 1, "Preparing file list...");
		
		HANDLE h = CreateFile(i->l[0].c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if(h==INVALID_HANDLE_VALUE) {
			return 0;
		}
		DirectoryListing* dl = new DirectoryListing();
		DWORD size = GetFileSize(h, NULL);
		BYTE* buf = new BYTE[size];
		ReadFile(h, buf, size, &size, NULL);
		CloseHandle(h);
		string tmp;
		CryptoManager::getInstance()->decodeHuffman(buf, tmp);
		delete buf;
		dl->load(tmp);
		
		DirectoryListingFrame* pChild = new DirectoryListingFrame(dl, i->l[1]);
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
		pChild->SetWindowText((i->l[1] + i->l[2]).c_str());
		delete i;
	} else if(wParam == AUTO_CONNECT) {
		HubManager::getInstance()->addListener(this);
		HubManager::getInstance()->getFavoriteHubs();
	}

	return 0;
}

void MainFrame::onUploadStarting(Upload* aUpload) {
	StringListInfo* i = new StringListInfo((LPARAM)aUpload);
	i->l.push_back(aUpload->getFileName());
	i->l.push_back("Connecting...");
	i->l.push_back(Util::formatBytes(aUpload->getSize()));
	
	i->l.push_back(aUpload->getUser()->getNick() + " (" + aUpload->getUser()->getClientName() + ")");

	PostMessage(WM_SPEAKER, UPLOAD_STARTING, (LPARAM)i);
}

void MainFrame::onUploadTick(Upload* aUpload) {
	char buf[128];
	LONGLONG dif = (LONGLONG)(TimerManager::getTick() - aUpload->getStart());
	int seconds = 0;
	LONGLONG avg = 0;
	if(dif > 0) {
		avg = aUpload->getTotal() * (LONGLONG)1000 / dif;
		if(avg > 0) {
			seconds = (aUpload->getSize() - aUpload->getPos()) / avg;
		}
	}

	sprintf(buf, "Uploaded %s (%.01f%%), %s/s, %s left", Util::formatBytes(aUpload->getPos()).c_str(), 
		(double)aUpload->getPos()*100.0/(double)aUpload->getSize(), Util::formatBytes(avg).c_str(), Util::formatSeconds(seconds).c_str());

	StringInfo* i = new StringInfo((LPARAM)aUpload, buf);
	PostMessage(WM_SPEAKER, UPLOAD_TICK, (LPARAM)i);
}

void MainFrame::onAction(DownloadManagerListener::Types type, Download* aDownload) {
	switch(type) {
	case DownloadManagerListener::ADDED:
		onDownloadAdded(aDownload); 
		break;
	case DownloadManagerListener::COMPLETE:
		onDownloadComplete(aDownload); 
		break;
	case DownloadManagerListener::CONNECTING:
		PostMessage(WM_SPEAKER, DOWNLOAD_CONNECTING, (LPARAM) aDownload); 
		break;
	case DownloadManagerListener::REMOVED:
		PostMessage(WM_SPEAKER, DOWNLOAD_REMOVED, (LPARAM)aDownload); 
		break;
	case DownloadManagerListener::STARTING:
		onDownloadStarting(aDownload); 
		break;
	case DownloadManagerListener::TICK:
		onDownloadTick(aDownload);
		break;
	default:
		dcassert(0);
	}
}


void MainFrame::onDownloadAdded(Download* p) {
	StringListInfo* i = new StringListInfo((LPARAM)p);	
	i->l.push_back(p->getTarget().substr(p->getTarget().rfind('\\') + 1));
	i->l.push_back("Waiting to connect...");
	i->l.push_back((p->getSize() != -1) ? Util::formatBytes(p->getSize()).c_str() : "Unknown");
	i->l.push_back("");

	PostMessage(WM_SPEAKER, DOWNLOAD_ADDED, (LPARAM)i);
}


void MainFrame::onDownloadComplete(Download* p) {
	if(p->isSet(Download::USER_LIST)) {
		// We have a new DC listing, show it...
		StringListInfo* i = new StringListInfo((LPARAM)p);
		i->l.push_back(p->getTarget());
		i->l.push_back(p->getCurrentSource()->getUser()->getNick());
		i->l.push_back(" (" + p->getCurrentSource()->getUser()->getClientName() + ")");
		
		PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
	}
}

void MainFrame::onDownloadFailed(Download::Ptr aDownload, const string& aReason) {
	StringListInfo* i = new StringListInfo((LPARAM)aDownload);
	i->l.push_back(aReason);
	i->l.push_back("");
	for(Download::Source::Iter j = aDownload->getSources().begin(); j != aDownload->getSources().end(); ++j) {
		if(i->l[1].size() > 0)
			i->l[1] += ", ";
		
		Download::Source::Ptr sr = *j;
		if(sr->getUser()) {
			i->l[1] += sr->getUser()->getNick() + " (" + sr->getUser()->getClientName() + ")";
		} else {
			i->l[1] += sr->getNick() + " (Offline)";
		}
	}
	PostMessage(WM_SPEAKER, DOWNLOAD_FAILED, (LPARAM)i);
}

void MainFrame::onDownloadSourceAdded(Download::Ptr aDownload, Download::Source* aSource) {
	if(!aDownload->isSet(Download::RUNNING)) {
		StringInfo* i = new StringInfo((LPARAM)aDownload);
		for(Download::Source::Iter j = aDownload->getSources().begin(); j != aDownload->getSources().end(); ++j) {
			if(i->str.size() > 0)
				i->str += ", ";

			Download::Source::Ptr sr = *j;
			if(sr->getUser()) {
				i->str += sr->getUser()->getNick() + " (" + sr->getUser()->getClientName() + ")";
			} else {
				i->str += sr->getNick() + " (Offline)";
			}
		}

		PostMessage(WM_SPEAKER, DOWNLOAD_SOURCEADDED, (LPARAM)i);
	}
}

void MainFrame::onDownloadStarting(Download* aDownload) {
	StringListInfo* i = new StringListInfo((LPARAM)aDownload);
	i->l.push_back(Util::formatBytes(aDownload->getSize()));
	dcassert(aDownload->getCurrentSource());
	i->l.push_back(aDownload->getCurrentSource()->getUser()->getNick() + " (" + aDownload->getCurrentSource()->getUser()->getClientName() + ")");
	
	PostMessage(WM_SPEAKER, DOWNLOAD_STARTING, (LPARAM)i);
}

void MainFrame::onDownloadTick(Download* aDownload) {
	char buf[256];
	LONGLONG dif = (LONGLONG)(TimerManager::getTick() - aDownload->getStart());
	int seconds = 0;
	LONGLONG avg = 0;
	if(dif > 0) {
		avg = aDownload->getTotal() * (LONGLONG)1000 / dif;
		if(avg > 0) {
			seconds = (aDownload->getSize() - aDownload->getPos()) / avg;
		}
	}
	
	sprintf(buf, "Downloaded %s (%.01f%%), %s/s, %s left", Util::formatBytes(aDownload->getPos()).c_str(), 
		(double)aDownload->getPos()*100.0/(double)aDownload->getSize(), Util::formatBytes(avg).c_str(), Util::formatSeconds(seconds).c_str());
	
	StringInfo* i = new StringInfo((LPARAM)aDownload, buf);
	PostMessage(WM_SPEAKER, DOWNLOAD_TICK, (LPARAM)i);

}

HWND MainFrame::createToolbar() {
	
	CToolBarCtrl ctrl;
	ctrl.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT, 0, ATL_IDW_TOOLBAR);
	ctrl.SetImageList(images);
	
	TBBUTTON tb[5];
	memset(tb, 0, sizeof(tb));
	tb[0].iBitmap = 0;
	tb[0].idCommand = ID_FILE_CONNECT;
	tb[0].fsState = TBSTATE_ENABLED;
	tb[0].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	tb[1].iBitmap = 1;
	tb[1].idCommand = ID_FILE_RECONNECT;
	tb[1].fsState = TBSTATE_ENABLED;
	tb[1].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	
	tb[2].iBitmap = 2;
	tb[2].idCommand = IDC_FAVORITES;
	tb[2].fsState = TBSTATE_ENABLED;
	tb[2].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	tb[3].iBitmap = 3;
	tb[3].idCommand = ID_FILE_SEARCH;
	tb[3].fsState = TBSTATE_ENABLED;
	tb[3].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	
	tb[4].iBitmap = 4;
	tb[4].idCommand = ID_FILE_SETTINGS;
	tb[4].fsState = TBSTATE_ENABLED;
	tb[4].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	ctrl.SetButtonStructSize();
	ctrl.AddButtons(5, tb);
	
	ctrl.AutoSize();
	return ctrl.m_hWnd;
	
}

LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		
	// Set window name
	SetWindowText(APPNAME " " VERSIONSTRING);

	// Load images
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	images.CreateFromImage(IDB_TOOLBAR, 16, 5, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	m_CmdBar.m_hImageList = images;
	m_CmdBar.m_arrCommand.Add(ID_FILE_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_FILE_RECONNECT);
	m_CmdBar.m_arrCommand.Add(IDC_FAVORITES);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SEARCH);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SETTINGS);
	
	// remove old menu
	SetMenu(NULL);
	
	HWND hWndToolBar = createToolbar();
	
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetSimple(FALSE);
	
	CreateMDIClient();
	m_CmdBar.SetMDIClient(m_hWndMDIClient);
	
	arrows.CreateFromImage(IDB_ARROWS, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	
	ctrlTransfers.InsertColumn(0, "File", LVCFMT_LEFT, 400, 0);
	ctrlTransfers.InsertColumn(1, "Status", LVCFMT_LEFT, 300, 1);
	ctrlTransfers.InsertColumn(2, "Size", LVCFMT_RIGHT, 100, 2);
	ctrlTransfers.InsertColumn(3, "User", LVCFMT_LEFT, 200, 3);
	
	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	
	ctrlTab.Create(m_hWnd, rcDefault);
	
	SetSplitterPanes(m_hWndClient, ctrlTransfers.m_hWnd);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	m_nProportionalPos = 8000;
	
	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	
	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	SettingsManager::newInstance();
	ShareManager::newInstance();
	TimerManager::newInstance();
	CryptoManager::newInstance();
	SearchManager::newInstance();
	ClientManager::newInstance();
	ConnectionManager::newInstance();
	DownloadManager::newInstance();
	UploadManager::newInstance();
	HubManager::newInstance();

	TimerManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);

	SettingsManager::getInstance()->load();	

	SettingsManager::getInstance()->setDefault(SettingsManager::SERVER, Util::getLocalIp());
	SettingsManager::getInstance()->setDefault(SettingsManager::PORT, 412);
	SettingsManager::getInstance()->setDefault(SettingsManager::ROLLBACK, 1024);

	ShareManager::getInstance()->refresh();
	HubManager::getInstance()->refresh();

	if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
		try {
			ConnectionManager::getInstance()->setPort(SETTING(PORT));
			SearchManager::getInstance()->setPort(SETTING(PORT));
		} catch(Exception e) {
			dcdebug("MainFrame::OnCreate caught %s\n", e.getError().c_str());
			MessageBox(("Port " + Util::toString(SETTING(PORT)) + " is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart DC++").c_str());
		}
	}

	transferMenu.CreatePopupMenu();

	c.addListener(this);
	c.downloadFile("http://dcplusplus.sourceforge.net/version.xml");

	PostMessage(WM_COMMAND, ID_FILE_CONNECT);
	PostMessage(WM_SPEAKER, AUTO_CONNECT);
	
	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	RECT rc;                    // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	
	// Get the bounding rectangle of the client area. 
	ctrlTransfers.GetClientRect(&rc);
	ctrlTransfers.ScreenToClient(&pt); 
	if (PtInRect(&rc, pt)) 
	{ 
		// Remove all old items
		while(transferMenu.GetMenuItemCount() > 0) {
			transferMenu.DeleteMenu(0, MF_BYPOSITION);
		}
		
		int n = 0;
		CMenuItemInfo mi;
		
		mi.fMask = MIIM_ID | MIIM_TYPE;
		mi.fType = MFT_STRING;
		mi.cch = 15;
		mi.dwTypeData = "Remove Transfer";
		mi.wID = IDC_REMOVE;
		transferMenu.InsertMenuItem(n++, TRUE, &mi);
		
		if(ctrlTransfers.GetSelectedCount() == 1) {
			mi.fMask = MIIM_TYPE;
			mi.fType = MFT_SEPARATOR;
			transferMenu.InsertMenuItem(n++, TRUE, &mi);
	
			LVITEM lvi;
			lvi.iItem = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
			lvi.iSubItem = 0;
			lvi.mask = LVIF_IMAGE | LVIF_PARAM;
			
			ctrlTransfers.GetItem(&lvi);
			menuItems = 0;
			
			if(lvi.iImage == IMAGE_DOWNLOAD) {
				Download* d = (Download*)lvi.lParam;
				for(Download::Source::Iter i = d->getSources().begin(); i != d->getSources().end(); ++i) {
					string str = "Browse " + (*i)->getNick() + "'s files";
					mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
					mi.fType = MFT_STRING;
					mi.cch = str.size();
					mi.dwTypeData = (LPSTR)str.c_str();
					mi.dwItemData = (DWORD)*i;
					mi.wID = IDC_TRANSFERITEM + menuItems++;
					transferMenu.InsertMenuItem(n++, TRUE, &mi);
					
					str = "Remove " + (*i)->getNick() + " from this transfer";
					mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
					mi.fType = MFT_STRING;
					mi.cch = str.size();
					mi.dwTypeData = (LPSTR)str.c_str();
					mi.dwItemData = (DWORD)*i;
					mi.wID = IDC_TRANSFERITEM + menuItems++;
					transferMenu.InsertMenuItem(n++, TRUE, &mi);
					
					str = "Send Message To " + (*i)->getNick();
					mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
					mi.fType = MFT_STRING;
					mi.cch = str.size();
					mi.dwTypeData = (LPSTR)str.c_str();
					mi.dwItemData = (DWORD)*i;
					mi.wID = IDC_TRANSFERITEM + menuItems++;
					transferMenu.InsertMenuItem(n++, TRUE, &mi);

					mi.fMask = MIIM_TYPE;
					mi.fType = MFT_SEPARATOR;
					transferMenu.InsertMenuItem(n++, TRUE, &mi);
					
				}
			} else {
				Upload* u = (Upload*) lvi.lParam;
				string str = "Browse " + u->getUser()->getNick() + "'s files";
				mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
				mi.fType = MFT_STRING;
				mi.cch = str.size();
				mi.dwTypeData = (LPSTR)str.c_str();
				mi.dwItemData = NULL;
				mi.wID = IDC_TRANSFERITEM + menuItems++;
				transferMenu.InsertMenuItem(n++, TRUE, &mi);
				
				str = "Send Message To " + u->getUser()->getNick();
				mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
				mi.fType = MFT_STRING;
				mi.cch = str.size();
				mi.dwTypeData = (LPSTR)str.c_str();
				mi.dwItemData = NULL;
				mi.wID = IDC_TRANSFERITEM + menuItems++;
				transferMenu.InsertMenuItem(n++, TRUE, &mi);
				
			}
		}
		
		ctrlTransfers.ClientToScreen(&pt);
		
		transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		return TRUE; 
	}
	return FALSE; 
}

LRESULT MainFrame::OnFileSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	SearchFrame* pChild = new SearchFrame();
	pChild->setTab(&ctrlTab);
	pChild->CreateEx(m_hWndClient);
	return 0;
}	

LRESULT MainFrame::onFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(FavoriteHubsFrame::frame == NULL) {
		FavoriteHubsFrame* pChild = new FavoriteHubsFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(FavoriteHubsFrame::frame->m_hWnd);
	}
	return 0;
}

LRESULT MainFrame::OnFileConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(PublicHubsFrame::frame == NULL) {
		PublicHubsFrame* pChild = new PublicHubsFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(PublicHubsFrame::frame->m_hWnd);
	}

/*	HANDLE h = CreateFile("c:\\temp\\test.dcl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	BYTE* buf = new BYTE[GetFileSize(h, NULL)];
	DWORD x;

	ReadFile(h, buf, GetFileSize(h, NULL), &x, NULL);
	string tmp1;
	CloseHandle(h);

	CryptoManager::getInstance()->decodeHuffman(buf, tmp1);
	delete buf;
*/	
	return 0;
}

LRESULT MainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SettingsDlg dlg;
	dlg.nick = SettingsManager::getInstance()->get(SettingsManager::NICK, false);
	dlg.email = SettingsManager::getInstance()->get(SettingsManager::EMAIL, false);
	dlg.description = SettingsManager::getInstance()->get(SettingsManager::DESCRIPTION, false);
	dlg.connection = SettingsManager::getInstance()->get(SettingsManager::CONNECTION, false);
	dlg.server = SettingsManager::getInstance()->get(SettingsManager::SERVER, false);
	dlg.directory = SettingsManager::getInstance()->get(SettingsManager::DOWNLOAD_DIRECTORY, false);
	dlg.port = SettingsManager::getInstance()->get(SettingsManager::PORT, false) == 0 ? "" : Util::toString(SettingsManager::getInstance()->get(SettingsManager::PORT, false));
	dlg.connectionType = SettingsManager::getInstance()->get(SettingsManager::CONNECTION_TYPE, false);
	dlg.slots = SettingsManager::getInstance()->get(SettingsManager::SLOTS, false);

	if(dlg.DoModal(m_hWnd) == IDOK) {

		if(dlg.directory.size() > 0 && dlg.directory[dlg.directory.size() - 1] != '\\') {
			dlg.directory += '\\';
		}
		
		SettingsManager::getInstance()->set(SettingsManager::NICK, Util::validateString(dlg.nick));
		SettingsManager::getInstance()->set(SettingsManager::DESCRIPTION, Util::validateString(dlg.description));
		SettingsManager::getInstance()->set(SettingsManager::EMAIL, Util::validateString(dlg.email));
		SettingsManager::getInstance()->set(SettingsManager::CONNECTION, dlg.connection);
		SettingsManager::getInstance()->set(SettingsManager::SERVER, dlg.server);
		SettingsManager::getInstance()->set(SettingsManager::PORT, dlg.port);
		SettingsManager::getInstance()->set(SettingsManager::CONNECTION_TYPE, dlg.connectionType);
		SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_DIRECTORY, dlg.directory);
		SettingsManager::getInstance()->set(SettingsManager::SLOTS, dlg.slots);
		SettingsManager::getInstance()->save();

		ShareManager::getInstance()->refresh();

		if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
			try {
				ConnectionManager::getInstance()->setPort(SETTING(PORT));
				SearchManager::getInstance()->setPort(SETTING(PORT));
			} catch(Exception e) {
				dcdebug("MainFrame::OnCreate caught %s\n", e.getError().c_str());
				MessageBox(("Port " + Util::toString(SETTING(PORT)) + " is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart DC++").c_str());
			}
		}
	}
	return 0;
}

LRESULT MainFrame::onTransferItem(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlTransfers.GetSelectedCount() == 1) {
		
		LVITEM lvi;
		lvi.iItem = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
		lvi.iSubItem = 0;
		lvi.mask = LVIF_IMAGE | LVIF_PARAM;
		
		ctrlTransfers.GetItem(&lvi);
		
		if(lvi.iImage == IMAGE_DOWNLOAD) {
			Download* d = (Download*)lvi.lParam;
			CMenuItemInfo mi;
			mi.fMask = MIIM_DATA;
			int cmd = (wID - IDC_TRANSFERITEM) % 3;
			
			transferMenu.GetMenuItemInfo(wID, FALSE, &mi);
			Download::Source* s = (Download::Source*)mi.dwItemData;
			switch(cmd) {
			case 0:
				try {
					DownloadManager::getInstance()->downloadList(s->getUser());
				} catch(...) {
					// ...
				}
				break;
			case 1:
				DownloadManager::getInstance()->removeSource(d, s);
				break;
			case 2:
				if(s->getUser() && s->getUser()->isOnline()) {
					PrivateFrame* frm = PrivateFrame::getFrame(s->getUser(), m_hWndClient);
					if(frm->m_hWnd == NULL) {
						frm->setTab(&ctrlTab);
						frm->CreateEx(m_hWndClient);
					} else {
						frm->MDIActivate(frm->m_hWnd);
					}
				}
				break;
			}
		} else {
			Upload* u = (Upload*)lvi.lParam;
			int cmd = (wID - IDC_TRANSFERITEM) % 2;
			switch(cmd) {
			case 0:
				try {
					DownloadManager::getInstance()->downloadList(u->getUser());
				} catch(...) {
					// ...
				}
				break;
			case 1:
				if(u->getUser()->isOnline()) {
					PrivateFrame* frm = PrivateFrame::getFrame(u->getUser(), m_hWndClient);
					if(frm->m_hWnd == NULL) {
						frm->setTab(&ctrlTab);
						frm->CreateEx(m_hWndClient);
					} else {
						frm->MDIActivate(frm->m_hWnd);
					}
				}
				
			}
		}
	}
	
	return 0;
}

void MainFrame::onHttpData(HttpConnection* aConn, const BYTE* aBuf, int aLen) {
	versionInfo += string((const char*)aBuf, aLen);
}

void MainFrame::onHttpComplete(HttpConnection* aConn)  {
	try {
		SimpleXML xml;
		xml.fromXML(versionInfo);
		xml.stepIn();
		if(xml.findChild("Version")) {
			if(atof(xml.getChildData().c_str()) > VERSIONFLOAT) {
				xml.resetCurrentChild();
				if(xml.findChild("Message")) {
					const string& msg = xml.getChildData();
					xml.resetCurrentChild();
					if(xml.findChild("Title")) {
						MessageBox(msg.c_str(), xml.getChildData().c_str());
					}
				}
			}
		}
	} catch (Exception e) {
		// ...
	}
}

void MainFrame::onAction(HubManagerListener::Types type, const FavoriteHubEntry::List& fl) {
	switch(type) {
	case HubManagerListener::GET_FAVORITE_HUBS: 
		for(FavoriteHubEntry::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
			FavoriteHubEntry* entry = *i;
			if(entry->getConnect()) {
				HubFrame* frm = new HubFrame(entry->getServer(), entry->getNick(), entry->getPassword());
				frm->setTab(&ctrlTab);
				frm->CreateEx(m_hWndMDIClient);
			}
		}
		HubManager::getInstance()->removeListener(this);
		break;
	}
}

/**
 * @file MainFrm.cpp
 * $Id: MainFrm.cpp,v 1.40 2002/01/17 23:35:59 arnetheduck Exp $
 * @if LOG
 * $Log: MainFrm.cpp,v $
 * Revision 1.40  2002/01/17 23:35:59  arnetheduck
 * Reworked threading once more, now it actually seems stable. Also made
 * sure that noone tries to access client objects that have been deleted
 * as well as some other minor updates
 *
 * Revision 1.39  2002/01/16 20:56:27  arnetheduck
 * Bug fixes, file listing sort and some other small changes
 *
 * Revision 1.38  2002/01/14 22:19:43  arnetheduck
 * Commiting minor bugfixes
 *
 * Revision 1.37  2002/01/13 22:50:48  arnetheduck
 * Time for 0.12, added favorites, a bunch of new icons and lot's of other stuff
 *
 * Revision 1.36  2002/01/11 14:52:57  arnetheduck
 * Huge changes in the listener code, replaced most of it with templates,
 * also moved the getinstance stuff for the managers to a template
 *
 * Revision 1.35  2002/01/10 12:33:14  arnetheduck
 * Various fixes
 *
 * Revision 1.34  2002/01/08 00:24:10  arnetheduck
 * Last bugs fixed before 0.11
 *
 * Revision 1.33  2002/01/07 20:17:59  arnetheduck
 * Finally fixed the reconnect bug that's been annoying me for a whole day...
 * Hopefully the app works better in w95 now too...
 *
 * Revision 1.32  2002/01/05 19:06:09  arnetheduck
 * Added user list images, fixed bugs and made things more effective
 *
 * Revision 1.30  2002/01/05 10:13:39  arnetheduck
 * Automatic version detection and some other updates
 *
 * Revision 1.29  2002/01/02 16:55:56  arnetheduck
 * Time for 0.09
 *
 * Revision 1.28  2002/01/02 16:12:32  arnetheduck
 * Added code for multiple download sources
 *
 * Revision 1.27  2001/12/30 17:41:16  arnetheduck
 * Fixed some XML parsing bugs
 *
 * Revision 1.26  2001/12/30 15:03:45  arnetheduck
 * Added framework to handle incoming searches
 *
 * Revision 1.25  2001/12/29 13:47:14  arnetheduck
 * Fixing bugs and UI work
 *
 * Revision 1.24  2001/12/27 18:14:36  arnetheduck
 * Version 0.08, here we go...
 *
 * Revision 1.23  2001/12/27 12:05:00  arnetheduck
 * Added flat tabs, fixed sorting and a StringTokenizer bug
 *
 * Revision 1.22  2001/12/21 23:52:30  arnetheduck
 * Last commit for five days
 *
 * Revision 1.21  2001/12/21 20:21:17  arnetheduck
 * Private messaging added, and a lot of other updates as well...
 *
 * Revision 1.20  2001/12/19 23:07:59  arnetheduck
 * Added directory downloading from the directory tree (although it hasn't been
 * tested at all) and password support.
 *
 * Revision 1.19  2001/12/16 19:47:48  arnetheduck
 * Reworked downloading and user handling some, and changed some small UI things
 *
 * Revision 1.18  2001/12/13 19:21:57  arnetheduck
 * A lot of work done almost everywhere, mainly towards a friendlier UI
 * and less bugs...time to release 0.06...
 *
 * Revision 1.17  2001/12/12 00:06:04  arnetheduck
 * Updated the public hub listings, fixed some minor transfer bugs, reworked the
 * sockets to use only one thread (instead of an extra thread for sending files),
 * and fixed a major bug in the client command decoding (still have to fix this
 * one for the userconnections...)
 *
 * Revision 1.16  2001/12/11 01:10:29  arnetheduck
 * More bugfixes...I really have to change the bufferedsocket so that it only
 * uses one thread...or maybe even multiple sockets/thread...
 *
 * Revision 1.15  2001/12/08 20:59:26  arnetheduck
 * Fixing bugs...
 *
 * Revision 1.14  2001/12/08 14:25:49  arnetheduck
 * More bugs removed...did my first search as well...
 *
 * Revision 1.13  2001/12/07 20:03:13  arnetheduck
 * More work done towards application stability
 *
 * Revision 1.12  2001/12/04 21:50:34  arnetheduck
 * Work done towards application stability...still a lot to do though...
 * a bit more and it's time for a new release.
 *
 * Revision 1.11  2001/12/03 20:52:19  arnetheduck
 * Blah! Finally, the listings are working...one line of code missing (of course),
 * but more than 2 hours of search...hate that kind of bugs...=(...some other
 * things spiffed up as well...
 *
 * Revision 1.10  2001/12/02 23:47:35  arnetheduck
 * Added the framework for uploading and file sharing...although there's something strange about
 * the file lists...my client takes them, but not the original...
 *
 * Revision 1.9  2001/12/02 14:05:36  arnetheduck
 * More sorting work, the hub list is now fully usable...
 *
 * Revision 1.8  2001/12/02 11:16:46  arnetheduck
 * Optimised hub listing, removed a few bugs and leaks, and added a few small
 * things...downloads are now working, time to start writing the sharing
 * code...
 *
 * Revision 1.7  2001/12/01 17:15:03  arnetheduck
 * Added a crappy version of huffman encoding, and some other minor changes...
 *
 * Revision 1.6  2001/11/29 19:10:55  arnetheduck
 * Refactored down/uploading and some other things completely.
 * Also added download indicators and download resuming, along
 * with some other stuff.
 *
 * Revision 1.5  2001/11/26 23:40:36  arnetheduck
 * Downloads!! Now downloads are possible, although the implementation is
 * likely to change in the future...more UI work (splitters...) and some bug
 * fixes. Only user file listings are downloadable, but at least it's something...
 *
 * Revision 1.4  2001/11/25 22:06:25  arnetheduck
 * Finally downloading is working! There are now a few quirks and bugs to be fixed
 * but what the heck....!
 *
 * Revision 1.3  2001/11/22 20:42:18  arnetheduck
 * Fixed Settings dialog (Speed setting actually works now!)
 *
 * Revision 1.2  2001/11/22 19:47:42  arnetheduck
 * A simple XML parser. Doesn't have all the features, but works good enough for
 * the configuration file.
 *
 * Revision 1.1.1.1  2001/11/21 17:33:20  arnetheduck
 * Inital release
 *
 * @endif
 */

