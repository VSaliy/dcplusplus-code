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

#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lambda/lambda.hpp>

#include <CriticalSection.h>

#include <richedit.h>

using std::vector;

using dcpp::CriticalSection;
using dcpp::Lock;

HINSTANCE Dialog::instance;

namespace {

// store the messages to be displayed here; process them with a timer.
CriticalSection mutex;
vector<string> messages;
uint16_t counter = 0;

// RTF formatting taken from dwt's RichTextBox

typedef boost::iterator_range<boost::range_const_iterator<string>::type> string_range;

string unicodeEscapeFormatter(const string_range& match) {
	if(match.empty())
		return string();
	return (boost::format("\\ud\\u%dh")%(int(*match.begin()))).str();
}

string escapeUnicode(const string& str) {
	string ret;
	boost::find_format_all_copy(std::back_inserter(ret), str,
		boost::first_finder(L"\x7f", std::greater<char>()), unicodeEscapeFormatter);
	return ret;
}

string rtfEscapeFormatter(const string_range& match) {
	if(match.empty())
		return string();
	string s(1, *match.begin());
	if (s == "\r") return "";
	if (s == "\n") return "\\line\n";
	return "\\" + s;
}

string rtfEscape(const string& str) {
	using boost::lambda::_1;
	string escaped;
	boost::find_format_all_copy(std::back_inserter(escaped), str,
		boost::first_finder("\x7f", _1 == '{' || _1 == '}' || _1 == '\\' || _1 == '\n' || _1 == '\r'), rtfEscapeFormatter);
	return escaped;
}

string formatText(const string& text) {
	return "{\\urtf " + escapeUnicode(rtfEscape(Util::toString(++counter) + " " + text)) + "}";
}

BOOL init(HWND hwnd) {
	return TRUE;
}

void command(HWND hwnd, UINT id) {
	switch(id) {
	case IDOK:
	case IDCANCEL:
		{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
	}
}

void timer(HWND hwnd) {
	auto control = GetDlgItem(hwnd, IDC_MESSAGES);

	SendMessage(control, WM_SETREDRAW, FALSE, 0);

	{
		SETTEXTEX config = { ST_SELECTION, CP_ACP };

		Lock l(mutex);
		for(auto& text: messages) {
			SendMessage(control, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
			SendMessage(control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&config), reinterpret_cast<LPARAM>(formatText(text).c_str()));
		}
		messages.clear();
	}

	SendMessage(control, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(control, 0, 0, RDW_ERASE | RDW_INVALIDATE);
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_INITDIALOG:
		{
			return init(hwnd);
		}
	case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			break;
		}
	case WM_COMMAND:
		{
			command(hwnd, LOWORD(wParam));
			break;
		}
	case WM_TIMER:
		{
			timer(hwnd);
			break;
		}
	}
	return FALSE;
}

} // unnamed namespace

Dialog::Dialog() : hwnd(nullptr), richEditLib(nullptr)
{
}

Dialog::~Dialog() {
	if(hwnd) {
		DestroyWindow(hwnd);
	}
	if(richEditLib) {
		FreeLibrary(richEditLib);
	}
}

void Dialog::create(HWND parent) {
	richEditLib = LoadLibrary(_T("msftedit.dll"));

	hwnd = CreateDialog(instance, MAKEINTRESOURCE(IDD_PLUGINDLG), hwnd, DialogProc);

#ifdef _DEBUG
	if(!hwnd) {
		TCHAR buf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, buf, 256, 0);
		MessageBox(parent, buf, _T("Error creating the dev plugin's dialog"), MB_OK);
	}
#endif

	/// @todo what about when the limit is hit?
	SendMessage(GetDlgItem(hwnd, IDC_MESSAGES), EM_LIMITTEXT, 10 * 64 * 1024, 0);

	SetTimer(hwnd, 1, 500, 0);
}

void Dialog::write(string&& text) {
	Lock l(mutex);
	messages.push_back(std::move(text));
}
