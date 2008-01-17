/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#include "SettingsDialog.h"
#include "resource.h"

#include "GeneralPage.h"
#include "NetworkPage.h"
#include "DownloadPage.h"
#include "FavoriteDirsPage.h"
#include "QueuePage.h"
#include "UploadPage.h"
#include "AppearancePage.h"
#include "Appearance2Page.h"
#include "TabsPage.h"
#include "WindowsPage.h"
#include "AdvancedPage.h"
#include "LogPage.h"
#include "Advanced3Page.h"
#include "UCPage.h"
#include "CertificatesPage.h"

static const TCHAR SEPARATOR = _T('\\');
static const size_t MAX_NAME_LENGTH = 256;

SettingsDialog::SettingsDialog(SmartWin::Widget* parent) : WidgetFactory<SmartWin::WidgetModalDialog>(parent), currentPage(0) {
	onInitDialog(std::tr1::bind(&SettingsDialog::initDialog, this));
}

bool SettingsDialog::initDialog() {
	attachButton(IDOK)->onClicked(std::tr1::bind(&SettingsDialog::handleOKClicked, this));

	attachButton(IDCANCEL)->onClicked(std::tr1::bind(&SettingsDialog::endDialog, this, IDCANCEL));

	pageTree = attachTreeView(IDC_SETTINGS_PAGES);
	pageTree->onSelectionChanged(std::tr1::bind(&SettingsDialog::selectionChanged, this));
	
	setText(T_("Settings"));
	
	addPage(T_("Personal information"), new GeneralPage(this));
	addPage(T_("Connection settings"), new NetworkPage(this));
	addPage(T_("Downloads"), new DownloadPage(this));
	addPage(T_("Downloads\\Favorites"), new FavoriteDirsPage(this));
	addPage(T_("Downloads\\Queue"), new QueuePage(this));
	addPage(T_("Sharing"), new UploadPage(this));
	addPage(T_("Appearance"), new AppearancePage(this));
	addPage(T_("Appearance\\Colors and sounds"), new Appearance2Page(this));
	addPage(T_("Appearance\\Tabs"), new TabsPage(this));
	addPage(T_("Appearance\\Windows"), new WindowsPage(this));
	addPage(T_("Advanced"), new AdvancedPage(this));
	addPage(T_("Advanced\\Logs"), new LogPage(this));
	addPage(T_("Advanced\\Experts only"), new Advanced3Page(this));
	addPage(T_("Advanced\\User Commands"), new UCPage(this));
	addPage(T_("Advanced\\Security Certificates"), new CertificatesPage(this));
	
	return false;
}

SettingsDialog::~SettingsDialog() {
}

int SettingsDialog::run() {
	return createDialog(IDD_SETTINGS);
}

void SettingsDialog::addPage(const tstring& title, PropPage* page) {
	pages.push_back(page);
	createTree(title, TVI_ROOT, page);
}

void SettingsDialog::handleOKClicked() {
	write();
	endDialog(IDOK);
}

void SettingsDialog::selectionChanged() {
	HTREEITEM item = TreeView_GetSelection(pageTree->handle());
	if(item == NULL) {
		showPage(0);
	} else {
		TVITEM tvitem = { TVIF_PARAM | TVIF_HANDLE };
		tvitem.hItem = item;
		if(!TreeView_GetItem(pageTree->handle(), &tvitem)) {
			showPage(0);
		} else {
			showPage(reinterpret_cast<PropPage*>(tvitem.lParam));
		}
	}
}

void SettingsDialog::showPage(PropPage* page) {
	if(currentPage) {
		::ShowWindow(dynamic_cast<SmartWin::Widget*>(currentPage)->handle(), SW_HIDE);
		currentPage = 0;
	}
	if(page) {
		::ShowWindow(dynamic_cast<SmartWin::Widget*>(page)->handle(), SW_SHOW);
		currentPage = page;
	}
}

HTREEITEM SettingsDialog::createTree(const tstring& str, HTREEITEM parent, PropPage* page) {
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter = TVI_LAST;
	tvi.hParent = parent;

	HTREEITEM first = (parent == TVI_ROOT) ? TreeView_GetRoot(pageTree->handle()) : TreeView_GetChild(pageTree->handle(), parent);

	string::size_type i = str.find(SEPARATOR);
	if(i == string::npos) {
		// Last dir, the actual page
		HTREEITEM item = find(str, first);
		if(item == NULL) {
			// Doesn't exist, add
			tvi.item.mask = TVIF_PARAM | TVIF_TEXT;
			tvi.item.pszText = const_cast<LPTSTR>(str.c_str());
			tvi.item.lParam = reinterpret_cast<LPARAM>(page);
			item = TreeView_InsertItem(pageTree->handle(), &tvi);
			TreeView_Expand(pageTree->handle(), parent, TVE_EXPAND);
			return item;
		} else {
			// Update page
			if(pageTree->getData(item) == 0)
				pageTree->setData(item, reinterpret_cast<LPARAM>(page));
			return item;
		}
	} else {
		tstring name = str.substr(0, i);
		HTREEITEM item = find(name, first);
		if(item == NULL) {
			// Doesn't exist, add...
			tvi.item.mask = TVIF_PARAM | TVIF_TEXT;
			tvi.item.lParam = 0;
			tvi.item.pszText = const_cast<LPTSTR>(name.c_str());
			item = TreeView_InsertItem(pageTree->handle(), &tvi);
		}
		TreeView_Expand(pageTree->handle(), parent, TVE_EXPAND);
		// Recurse...
		return createTree(str.substr(i+1), item, page);
	}
}

HTREEITEM SettingsDialog::find(const tstring& str, HTREEITEM start) {
	while(start != NULL) {
		if(pageTree->getText(start) == str) {
			return start;
		}
		start = TreeView_GetNextSibling(pageTree->handle(), start);
	}
	return start;
}

void SettingsDialog::write() {
	for(PageList::iterator i = pages.begin(); i != pages.end(); ++i) {
		(*i)->write();
	}
}
