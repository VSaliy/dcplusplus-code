/*
 * Copyright (C) 2012 Jacek Sieka, arnetheduck on gmail point com
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
#include "Dialog.h"

#ifndef __cplusplus
# include <stdlib.h>
# include <string.h>
#else
# include <cstdlib>
# include <cstring>
#endif

#include <pluginsdk/Config.h>

DCUtilsPtr utils = NULL;

#ifdef _WIN32

#include "resource.h"
extern HINSTANCE hInst;

BOOL onConfigInit(HWND hWnd) {
	char* value = get_cfg("SendSuffix");
	size_t len = strlen(value) + 1;
	TCHAR* buf = (TCHAR*)memset(malloc(len * sizeof(TCHAR)), 0, len * sizeof(TCHAR));

	utils->utf8_to_wcs(buf, value, len);
	free(value);
	value = NULL;

	SetDlgItemText(hWnd, IDC_SUFFIX, buf);
	SetWindowText(hWnd, _T(PLUGIN_NAME) _T(" Settings"));

	free(buf);
	return TRUE;
}

BOOL onConfigClose(HWND hWnd, UINT wID) {
	if(wID == IDOK) {
		int len = GetWindowTextLength(GetDlgItem(hWnd, IDC_SUFFIX)) + 1;
		TCHAR* wbuf = (TCHAR*)memset(malloc(len * sizeof(TCHAR)), 0, len * sizeof(TCHAR));
		char* value = (char*)memset(malloc(len), 0, len);

		GetWindowText(GetDlgItem(hWnd, IDC_SUFFIX), wbuf, len);
		utils->wcs_to_utf8(value, wbuf, len);
		set_cfg("SendSuffix", value);

		free(value);
		free(wbuf);
	}

	EndDialog(hWnd, wID);
	return FALSE;
}

INT_PTR CALLBACK configProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch(uMsg) {
		case WM_INITDIALOG:
			return onConfigInit(hWnd);
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
				case IDCLOSE:
					return onConfigClose(hWnd, LOWORD(wParam));
			}
		}
	}
	return FALSE;
}

Bool dialog_create(dcptr_t hWnd, DCCorePtr core) {
	utils = (DCUtilsPtr)core->query_interface(DCINTF_DCPP_UTILS, DCINTF_DCPP_UTILS_VER);
	if(utils) {
		DialogBox(hInst, MAKEINTRESOURCE(IDD_PLUGINDLG), (HWND)hWnd, configProc);
		return True;
	}
	return False;
}

#else

Bool dialog_create(dcptr_t hWnd, DCCorePtr core) { return False; }

#endif
