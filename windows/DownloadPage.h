/*
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(DOWNLOAD_PAGE_H)
#define DOWNLOAD_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"

class DownloadPage : public CPropertyPage<IDD_DOWNLOADPAGE>, public PropPage
{
public:
	DownloadPage(SettingsManager *s) : PropPage(s) {
		SetTitle(CTSTRING(SETTINGS_DOWNLOADS));
		m_psp.dwFlags |= PSP_HASHELP | PSP_RTLREADING;
	};
	virtual ~DownloadPage() { };

	BEGIN_MSG_MAP(DownloadPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_HELP, onHelp)
		COMMAND_ID_HANDLER(IDC_BROWSEDIR, onClickedBrowseDir)
		COMMAND_ID_HANDLER(IDC_BROWSETEMPDIR, onClickedBrowseTempDir)
		COMMAND_ID_HANDLER(IDC_SETTINGS_LIST_CONFIG, onClickedListConfigure)
		NOTIFY_CODE_HANDLER_EX(PSN_HELP, onHelpInfo)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedBrowseTempDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedListConfigure(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onHelpInfo(LPNMHDR /*pnmh*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();
	
protected:
	static Item items[];
	static TextItem texts[];
};

#endif //  !defined(DOWNLOAD_PAGE_H)

/**
 * @file
 * $Id: DownloadPage.h,v 1.15 2006/01/05 00:11:31 arnetheduck Exp $
 */
