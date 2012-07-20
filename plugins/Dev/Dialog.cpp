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
#include "resource.h"
#include "Util.h"

#include <unordered_set>
#include <vector>

#include <CriticalSection.h>

#include <commctrl.h>
#include <windowsx.h>

using std::move;
using std::unordered_set;
using std::vector;

using dcpp::CriticalSection;
using dcpp::Lock;

HINSTANCE Dialog::instance;

namespace {

// store the messages to be displayed here; process them with a timer.
struct Message { bool hubOrUser; bool sending; string ip; string peer; string message; };
vector<Message> messages;

CriticalSection mutex;
uint16_t counter = 0;
bool scroll = true;
bool hubMessages = true;
bool userMessages = true;
unordered_set<tstring> filter;
tstring filterSel;

struct Item {
	tstring index;
	tstring dir;
	tstring ip;
	tstring peer;
	tstring message;
};

void initFilter(HWND hwnd) {
#define noFilter _T("0 - No filtering")
	auto control = GetDlgItem(hwnd, IDC_FILTER);
	ComboBox_SetCurSel(control, ComboBox_AddString(control, noFilter));
}

BOOL init(HWND hwnd) {
	auto control = GetDlgItem(hwnd, IDC_MESSAGES);

	ListView_SetExtendedListViewStyle(control, LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	RECT rect;
	GetClientRect(control, &rect);

	LVCOLUMN col = { LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, 50, _T("N°") };
	ListView_InsertColumn(control, 0, &col);

	col.pszText = _T("Dir");
	ListView_InsertColumn(control, 1, &col);

	col.cx = 100;
	col.pszText = _T("IP");
	ListView_InsertColumn(control, 2, &col);

	col.cx = 200;
	col.pszText = _T("Peer info");
	ListView_InsertColumn(control, 3, &col);

	col.cx = rect.right - rect.left - 50 - 50 - 100 - 200 - 30;
	col.pszText = _T("Message");
	ListView_InsertColumn(control, 4, &col);

	SendMessage(GetDlgItem(hwnd, IDC_SCROLL), BM_SETCHECK, BST_CHECKED, 0);
	SendMessage(GetDlgItem(hwnd, IDC_HUB_MESSAGES), BM_SETCHECK, BST_CHECKED, 0);
	SendMessage(GetDlgItem(hwnd, IDC_USER_MESSAGES), BM_SETCHECK, BST_CHECKED, 0);

	initFilter(hwnd);

	SetTimer(hwnd, 1, 500, 0);

	return TRUE;
}

void timer(HWND hwnd) {
	decltype(messages) messages_;
	{
		Lock l(mutex);
		messages_.swap(messages);
	}
	if(messages_.empty()) {
		return;
	}

	auto control = GetDlgItem(hwnd, IDC_MESSAGES);

	LVITEM lvi = { 0 };

	for(auto& message: messages_) {
		if(!(message.hubOrUser ? hubMessages : userMessages)) {
			continue;
		}

		auto ip = Util::toT(message.ip);
		if(!filterSel.empty() && ip != filterSel) {
			continue;
		}

		auto item = new Item;
		item->index = Util::toT(Util::toString(counter));
		item->dir = message.sending ? _T("Out") : _T("In");
		item->ip = move(ip);
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
		lvi.pszText = const_cast<LPTSTR>(item->peer.c_str());
		ListView_SetItem(control, &lvi);

		lvi.iSubItem = 4;
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

void copy(HWND hwnd) {
	tstring str;

	auto control = GetDlgItem(hwnd, IDC_MESSAGES);
	LVITEM lvi = { LVIF_PARAM };
	int i = -1;
	while((i = ListView_GetNextItem(control, i, LVNI_SELECTED)) != -1) {
		lvi.iItem = i;
		if(ListView_GetItem(control, &lvi)) {
			auto& item = *reinterpret_cast<Item*>(lvi.lParam);
			if(!str.empty()) { str += _T("\r\n"); }
			str += item.index + _T(" ") + item.ip + _T(" (") + item.peer + _T("): ") + item.message;
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

void filterSelChanged(HWND hwnd) {
	auto control = GetDlgItem(hwnd, IDC_FILTER);

	auto sel = ComboBox_GetCurSel(hwnd);

	tstring str(ComboBox_GetLBTextLen(control, sel), '\0');
	ComboBox_GetLBText(control, sel, &str[0]);

	if(str == noFilter) {
		filterSel.clear();
	} else {
		filterSel = move(str);
	}
}

void clear(HWND hwnd) {
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
	initFilter(hwnd);
}

void command(HWND hwnd, WPARAM wParam) {
	switch(LOWORD(wParam)) {
	case IDOK:
	case IDCANCEL:
		{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}

	case IDC_COPY:
		{
			copy(hwnd);
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
				filterSelChanged(hwnd);
			}
			break;
		}

	case IDC_CLEAR:
		{
			clear(hwnd);
			break;
		}
	}
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_INITDIALOG:
		{
			return init(hwnd);
		}
	case WM_TIMER:
		{
			timer(hwnd);
			break;
		}
	case WM_COMMAND:
		{
			command(hwnd, wParam);
			break;
		}
	case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			break;
		}
	case WM_DESTROY:
		{
			clear(hwnd);
			break;
		}
	}
	return FALSE;
}

} // unnamed namespace

Dialog::Dialog() : hwnd(nullptr)
{
}

Dialog::~Dialog() {
	if(hwnd) {
		DestroyWindow(hwnd);
	}
}

void Dialog::create(HWND parent) {
	hwnd = CreateDialog(instance, MAKEINTRESOURCE(IDD_PLUGINDLG), hwnd, DialogProc);

#ifdef _DEBUG
	if(!hwnd) {
		TCHAR buf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, buf, 256, 0);
		MessageBox(parent, buf, _T("Error creating the dev plugin's dialog"), MB_OK);
	}
#endif
}

void Dialog::write(bool hubOrUser, bool sending, string ip, string peer, string message) {
	Message msg = { hubOrUser, sending, move(ip), move(peer), move(message) };
	Lock l(mutex);
	messages.push_back(move(msg));
}
