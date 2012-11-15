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
#include "Plugin.h"
#include "resource.h"

#include <pluginsdk/Core.h>
#include <pluginsdk/Util.h>

#include <boost/lexical_cast.hpp>

#include <commctrl.h>
#include <windowsx.h>

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif

#define noFilter _T("0 - No filtering")

using dcapi::Core;
using dcapi::Util;

HINSTANCE Dialog::instance;
Dialog* dlg = nullptr;

Dialog::Dialog() :
	hwnd(nullptr),
	messages(1024),
	counter(0),
	scroll(true),
	hubMessages(true),
	userMessages(true)
{
	dlg = this;
}

Dialog::~Dialog() {
	close();

	dlg = nullptr;
}

void Dialog::create() {
	if(hwnd) {
		MessageBox(0, Util::toT("The dev plugin hasn't been properly shut down; you better restart " + Core::appName).c_str(),
			_T("Error creating the dev plugin's dialog"), MB_OK);
		return;
	}

	CreateDialog(instance, MAKEINTRESOURCE(IDD_PLUGINDLG), 0, DialogProc);

	if(!hwnd) {
		TCHAR buf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, buf, 256, 0);
		MessageBox(0, buf, _T("Error creating the dev plugin's dialog"), MB_OK);
	}
}

void Dialog::write(bool hubOrUser, bool sending, string ip, decltype(ConnectionData().port) port, string peer, string message) {
	Message msg = { hubOrUser, sending, move(ip), port, move(peer), move(message) };
	messages.push(msg);
}

void Dialog::close() {
	if(hwnd) {
		DestroyWindow(hwnd);
		hwnd = nullptr;
	}
}

INT_PTR CALLBACK Dialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM) {
	switch(uMsg) {
	case WM_INITDIALOG:
		{
			dlg->hwnd = hwnd;
			return dlg->init();
		}
	case WM_TIMER:
		{
			dlg->timer();
			break;
		}
	case WM_COMMAND:
		{
			dlg->command(wParam);
			break;
		}
	case WM_CLOSE:
		{
			Plugin::dlgClosed();
			break;
		}
	case WM_DESTROY:
		{
			dlg->clear();
			break;
		}
	}
	return FALSE;
}

BOOL Dialog::init() {
	auto control = GetDlgItem(hwnd, IDC_MESSAGES);

	ListView_SetExtendedListViewStyle(control, LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	RECT rect;
	GetClientRect(control, &rect);

	LVCOLUMN col = { LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, 50, (LPWSTR)_T("#") };
	ListView_InsertColumn(control, 0, &col);

	col.pszText = (LPWSTR)_T("Dir");
	ListView_InsertColumn(control, 1, &col);

	col.cx = 100;
	col.pszText = (LPWSTR)_T("IP");
	ListView_InsertColumn(control, 2, &col);

	col.cx = 50;
	col.pszText = (LPWSTR) _T("Port");
	ListView_InsertColumn(control, 3, &col);

	col.cx = 200;
	col.pszText = (LPWSTR)_T("Peer info");
	ListView_InsertColumn(control, 4, &col);

	col.cx = rect.right - rect.left - 50 - 50 - 100 - 50 - 200 - 30;
	col.pszText = (LPWSTR)_T("Message");
	ListView_InsertColumn(control, 5, &col);

	SendMessage(GetDlgItem(hwnd, IDC_SCROLL), BM_SETCHECK, BST_CHECKED, 0);
	SendMessage(GetDlgItem(hwnd, IDC_HUB_MESSAGES), BM_SETCHECK, BST_CHECKED, 0);
	SendMessage(GetDlgItem(hwnd, IDC_USER_MESSAGES), BM_SETCHECK, BST_CHECKED, 0);

	initFilter();

	SetTimer(hwnd, 1, 500, 0);

	return TRUE;
}

void Dialog::timer() {
	auto control = GetDlgItem(hwnd, IDC_MESSAGES);

	LVITEM lvi = { 0 };

	Message message;
	while(messages.pop(message)) {

		if(!(message.hubOrUser ? hubMessages : userMessages)) {
			continue;
		}

		auto ip = Util::toT(message.ip);
		if(!filterSel.empty() && ip != filterSel) {
			continue;
		}

		if(!regex.empty()) {
			try {
				if(!boost::regex_search(message.message, regex)) {
					continue;
				}
			} catch(const std::runtime_error&) {
				// most likely a stack overflow, ignore...
				continue;
			}
		}

		auto item = new Item;
		item->index = Util::toT(boost::lexical_cast<string>(counter));
		item->dir = message.sending ? _T("Out") : _T("In");
		item->ip = move(ip);
		item->port = Util::toT(boost::lexical_cast<string>(message.port));
		item->peer = Util::toT(message.peer);
		item->message = Util::toT(message.message);

		// first column; include the lParam.
		lvi.mask = LVIF_PARAM | LVIF_TEXT;
		lvi.iItem = counter;
		lvi.iSubItem = 0;
		lvi.pszText = const_cast<LPTSTR>(item->index.c_str());
		lvi.lParam = reinterpret_cast<LPARAM>(item);
		ListView_InsertItem(control, &lvi);

		// no lParam for other columns (ie sub-items).
		lvi.mask = LVIF_TEXT;

		lvi.iSubItem = 1;
		lvi.pszText = const_cast<LPTSTR>(item->dir.c_str());
		ListView_SetItem(control, &lvi);

		lvi.iSubItem = 2;
		lvi.pszText = const_cast<LPTSTR>(item->ip.c_str());
		ListView_SetItem(control, &lvi);

		lvi.iSubItem = 3;
		lvi.pszText = const_cast<LPTSTR>(item->port.c_str());
		ListView_SetItem(control, &lvi);

		lvi.iSubItem = 4;
		lvi.pszText = const_cast<LPTSTR>(item->peer.c_str());
		ListView_SetItem(control, &lvi);

		lvi.iSubItem = 5;
		lvi.pszText = const_cast<LPTSTR>(item->message.c_str());
		ListView_SetItem(control, &lvi);

		++counter;

		if(filter.find(item->ip) == filter.end()) {
			ComboBox_AddString(GetDlgItem(hwnd, IDC_FILTER), filter.insert(item->ip).first->c_str());
		}
	}

	if(scroll && lvi.lParam) { // make sure something was added
		ListView_EnsureVisible(control, lvi.iItem, FALSE);
	}
}

void Dialog::command(WPARAM wParam) {
	switch(LOWORD(wParam)) {
	case IDOK:
	case IDCANCEL:
		{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}

	case IDC_SCROLL:
		{
			scroll = SendMessage(GetDlgItem(hwnd, IDC_SCROLL), BM_GETCHECK, 0, 0) == BST_CHECKED;
			break;
		}

	case IDC_HUB_MESSAGES:
		{
			hubMessages = SendMessage(GetDlgItem(hwnd, IDC_HUB_MESSAGES), BM_GETCHECK, 0, 0) == BST_CHECKED;
			break;
		}

	case IDC_USER_MESSAGES:
		{
			userMessages = SendMessage(GetDlgItem(hwnd, IDC_USER_MESSAGES), BM_GETCHECK, 0, 0) == BST_CHECKED;
			break;
		}

	case IDC_FILTER:
		{
			if(HIWORD(wParam) == CBN_SELENDOK) {
				filterSelChanged();
			}
			break;
		}

	case IDC_REGEX_APPLY:
		{
			applyRegex();
			break;
		}

	case IDC_COPY:
		{
			copy();
			break;
		}

	case IDC_CLEAR:
		{
			clear();
			break;
		}
	}
}

void Dialog::initFilter() {
	auto control = GetDlgItem(hwnd, IDC_FILTER);
	ComboBox_SetCurSel(control, ComboBox_AddString(control, noFilter));
}

void Dialog::filterSelChanged() {
	auto control = GetDlgItem(hwnd, IDC_FILTER);

	auto sel = ComboBox_GetCurSel(control);

	tstring str(ComboBox_GetLBTextLen(control, sel), '\0');
	ComboBox_GetLBText(control, sel, &str[0]);

	if(str == noFilter) {
		filterSel.clear();
	} else {
		filterSel = move(str);
	}
}

void Dialog::applyRegex() {
	regex = "";

	auto control = GetDlgItem(hwnd, IDC_REGEX);

	auto n = SendMessage(control, WM_GETTEXTLENGTH, 0, 0);
	if(!n) { return; }
	tstring str(n + 1, 0);
	str.resize(SendMessage(control, WM_GETTEXT, static_cast<WPARAM>(n + 1), reinterpret_cast<LPARAM>(&str[0])));

	try {
		regex.assign(Util::fromT(str));
	} catch(const std::runtime_error&) {
		MessageBox(hwnd, _T("Invalid regular expression"), _T("Dev plugin"), MB_OK);
		return;
	}
}

void Dialog::copy() {
	tstring str;

	auto control = GetDlgItem(hwnd, IDC_MESSAGES);
	LVITEM lvi = { LVIF_PARAM };
	int i = -1;
	while((i = ListView_GetNextItem(control, i, LVNI_SELECTED)) != -1) {
		lvi.iItem = i;
		if(ListView_GetItem(control, &lvi)) {
			auto& item = *reinterpret_cast<Item*>(lvi.lParam);
			if(!str.empty()) { str += _T("\r\n"); }
			str += item.index + _T(" [") + item.dir + _T("] ") + item.ip + _T(":") + item.port + _T(" (") + item.peer + _T("): ") + item.message;
		}
	}

	// the following has been taken from WinUtil::setClipboard

	if(!::OpenClipboard(hwnd)) {
		return;
	}

	EmptyClipboard();

	// Allocate a global memory object for the text.
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(TCHAR));
	if(hglbCopy == NULL) {
		CloseClipboard();
		return;
	}

	// Lock the handle and copy the text to the buffer.
	TCHAR* lptstrCopy = (TCHAR*) GlobalLock(hglbCopy);
	_tcscpy(lptstrCopy, str.c_str());
	GlobalUnlock(hglbCopy);

	// Place the handle on the clipboard.
	SetClipboardData(CF_UNICODETEXT, hglbCopy);

	CloseClipboard();
}

void Dialog::clear() {
	auto control = GetDlgItem(hwnd, IDC_MESSAGES);

	LVITEM lvi = { LVIF_PARAM };
	for(decltype(counter) i = 0; i < counter; ++i) {
		lvi.iItem = i;
		if(ListView_GetItem(control, &lvi)) {
			delete reinterpret_cast<Item*>(lvi.lParam);
		}
	}

	ListView_DeleteAllItems(control);
	counter = 0;

	ComboBox_ResetContent(GetDlgItem(hwnd, IDC_FILTER));
	filter.clear();
	filterSel.clear();
	initFilter();
}
