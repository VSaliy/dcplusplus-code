/*
* Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

// Based on <http://www.codeproject.com/shell/shellcontextmenu.asp> by R. Engels.

#include "stdafx.h"

#include "ShellMenu.h"

#include "resource.h"
#include "WinUtil.h"

ShellMenu::Seed::Seed() :
BaseType::Seed()
{
	/// @todo find a better way to directly use styles set in WinUtil (see TypedTable/TypedTree)...
	ownerDrawn = WinUtil::Seeds::menu.ownerDrawn;
	font = WinUtil::Seeds::menu.font;
}

ShellMenu::ShellMenu( dwt::Widget * parent ) :
BaseType(parent),
handler(0),
sel_id(0)
{
}

void ShellMenu::appendShellMenu(const wstring& path) {
#define check(x) if(!(x)) { return; }

	// get IShellFolder interface of Desktop (root of Shell namespace)
	IShellFolder* desktop = 0;
	HRESULT hr = ::SHGetDesktopFolder(&desktop);
	check(hr == S_OK && desktop);

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = 0;
	hr = desktop->ParseDisplayName(0, 0, const_cast<LPWSTR>(path.c_str()), 0, &pidl, 0);
	desktop->Release();
	check(hr == S_OK && pidl);

	// get the parent IShellFolder interface of pidl and the relative PIDL
	IShellFolder* folder = 0;
	LPCITEMIDLIST pidlItem = 0;
	hr = ::SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<LPVOID*>(&folder), &pidlItem);

	// get interface to IMalloc used to free pidl
	LPMALLOC lpMalloc = 0;
	::SHGetMalloc(&lpMalloc);
	lpMalloc->Free(pidl);
	lpMalloc->Release();

	check(hr == S_OK && folder && pidlItem);

	// first we retrieve the normal IContextMenu interface (every object should have it)
	LPCONTEXTMENU handler1 = 0;
	hr = folder->GetUIObjectOf(0, 1, &pidlItem, IID_IContextMenu, 0, reinterpret_cast<LPVOID*>(&handler1));
	folder->Release();
	check(hr == S_OK && handler1);

	// then try to get the version 3 interface
	hr = handler1->QueryInterface(IID_IContextMenu3, reinterpret_cast<LPVOID*>(&handler));
	handler1->Release();
	check(hr == S_OK && handler);

	getParent()->setCallback(dwt::Message(WM_DRAWITEM), Dispatcher(std::tr1::bind(&ShellMenu::handleDrawItem, this, _1), handler));
	getParent()->setCallback(dwt::Message(WM_MEASUREITEM), Dispatcher(std::tr1::bind(&ShellMenu::handleMeasureItem, this, _1), handler));
	getParent()->setCallback(dwt::Message(WM_MENUCHAR), Dispatcher(std::tr1::bind(&ShellMenu::handleMenuChar, this), handler));
	getParent()->setCallback(dwt::Message(WM_INITMENUPOPUP), Dispatcher(std::tr1::bind(&ShellMenu::handleInitMenuPopup, this, _1), handler));
	getParent()->setCallback(dwt::Message(WM_UNINITMENUPOPUP), Dispatcher(std::tr1::bind(&ShellMenu::handleUnInitMenuPopup, this, _1), handler));
	getParent()->setCallback(dwt::Message(WM_MENUSELECT), Dispatcher(std::tr1::bind(&ShellMenu::handleMenuSelect, this, _1), handler));

	appendSeparator();
	menu = appendPopup(dwt::Menu::Seed(false), T_("Shell menu"));

#undef check
}

ShellMenu::~ShellMenu() {
	if(handler)
		handler->Release();
}

void ShellMenu::open(const dwt::ScreenCoordinate& pt, unsigned flags) {
	BaseType::open(pt, flags);

	if(sel_id >= ID_SHELLCONTEXTMENU_MIN && sel_id <= ID_SHELLCONTEXTMENU_MAX) {
		CMINVOKECOMMANDINFO cmi = { sizeof(CMINVOKECOMMANDINFO) };
		cmi.lpVerb = (LPSTR)MAKEINTRESOURCE(sel_id - ID_SHELLCONTEXTMENU_MIN);
		cmi.nShow = SW_SHOWNORMAL;
		handler->InvokeCommand(&cmi);
	}
}

bool ShellMenu::handleMenuChar() {
	return true;
}

bool ShellMenu::handleDrawItem(const MSG& msg) {
	if(msg.wParam == 0) {
		LPDRAWITEMSTRUCT t = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
		if(t) {
			if(t->CtlType != ODT_MENU)
				return false;
			unsigned id = t->itemID;
			if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX) {
				return true;
			}
			BaseType::handleDrawItem(t);
		}
	}
	return false;
}

bool ShellMenu::handleMeasureItem(const MSG& msg) {
	if(msg.wParam == 0) {
		LPMEASUREITEMSTRUCT t = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
		if(t) {
			if(t->CtlType != ODT_MENU)
				return false;
			unsigned id = t->itemID;
			if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX) {
				return true;
			}
			BaseType::handleMeasureItem(t);
		}
	}
	return false;
}

bool ShellMenu::handleInitMenuPopup(const MSG& msg) {
	if(reinterpret_cast<HMENU>(msg.wParam) == menu->handle()) {
		handler->QueryContextMenu(menu->handle(), 0, ID_SHELLCONTEXTMENU_MIN, ID_SHELLCONTEXTMENU_MAX, CMF_NORMAL | CMF_EXPLORE);
	}
	return true;
}

bool ShellMenu::handleUnInitMenuPopup(const MSG& msg) {
	if(reinterpret_cast<HMENU>(msg.wParam) == menu->handle()) {
		menu->removeAllItems();
	}
	return true;
}

bool ShellMenu::handleMenuSelect(const MSG& msg) {
	// make sure this isn't a "menu closed" signal
	if((HIWORD(msg.wParam) == 0xFFFF) && (msg.lParam == 0))
		return false;

	// save the currently selected id in case we need to dispatch it later on
	sel_id = LOWORD(msg.wParam);
	return false;
}
