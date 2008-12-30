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

#include <dcpp/Text.h>
#include "resource.h"

ShellMenu::ShellMenu(dwt::MenuPtr& parent_, const wstring& path) :
parent(parent_),
handler(0),
m_psfFolder(NULL),
m_pidlArray(NULL),
sel_id(0)
{
	parent->appendSeparator();
	menu = parent->appendPopup(dwt::Menu::Seed(false), T_("Shell menu"));

	// get IShellFolder interface of Desktop(root of shell namespace)
	IShellFolder* psfDesktop = NULL;
	SHGetDesktopFolder(&psfDesktop);

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;
	psfDesktop->ParseDisplayName(NULL, NULL, const_cast<LPWSTR>(path.c_str()), NULL, &pidl, NULL);

	// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
	SHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&m_psfFolder, NULL);

	// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc(&lpMalloc);
	lpMalloc->Free(pidl);

	// now we need the relative pidl
	IShellFolder* psfFolder = NULL;
	psfDesktop->ParseDisplayName (NULL, NULL, const_cast<LPWSTR>(path.c_str()), NULL, &pidl, NULL);
	LPITEMIDLIST pidlItem = NULL;
	SHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&psfFolder, (LPCITEMIDLIST*)&pidlItem);

	// copy pidlItem to m_pidlArray
	m_pidlArray = (LPITEMIDLIST *) realloc(m_pidlArray, sizeof (LPITEMIDLIST));
	int nSize = 0;
	LPITEMIDLIST pidlTemp = pidlItem;
	while(pidlTemp->mkid.cb)
	{
		nSize += pidlTemp->mkid.cb;
		pidlTemp = (LPITEMIDLIST) (((LPBYTE) pidlTemp) + pidlTemp->mkid.cb);
	}
	LPITEMIDLIST pidlRet = (LPITEMIDLIST) calloc(nSize + sizeof (USHORT), sizeof (BYTE));
	CopyMemory(pidlRet, pidlItem, nSize);
	m_pidlArray[0] = pidlRet;

	lpMalloc->Free(pidl);

	lpMalloc->Release();
	psfFolder->Release();
	psfDesktop->Release();

	LPCONTEXTMENU handlerBase = 0;
	LPCONTEXTMENU handler1 = 0;
	// first we retrieve the normal IContextMenu interface (every object should have it)
	m_psfFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *) m_pidlArray, IID_IContextMenu, NULL, reinterpret_cast<LPVOID*>(&handler1));
	if(handler1 && (handler1->QueryInterface(IID_IContextMenu3, reinterpret_cast<LPVOID*>(&handlerBase)) == NOERROR) && handlerBase) {
		handler1->Release(); // we can now release version 1 interface, cause we got a higher one

		handler = reinterpret_cast<LPCONTEXTMENU3>(handlerBase);

		parent->getParent()->addCallback(dwt::Message(WM_DRAWITEM), Dispatcher(std::tr1::bind(&ShellMenu::handleDrawItem, this, _1), handler));
		parent->getParent()->addCallback(dwt::Message(WM_MEASUREITEM), Dispatcher(std::tr1::bind(&ShellMenu::handleMeasureItem, this, _1), handler));
		parent->getParent()->setCallback(dwt::Message(WM_MENUCHAR), Dispatcher(std::tr1::bind(&ShellMenu::handleMenuChar, this), handler));
		parent->getParent()->setCallback(dwt::Message(WM_INITMENUPOPUP), Dispatcher(std::tr1::bind(&ShellMenu::handleInitMenuPopup, this, _1), handler));
		parent->getParent()->setCallback(dwt::Message(WM_UNINITMENUPOPUP), Dispatcher(std::tr1::bind(&ShellMenu::handleUnInitMenuPopup, this, _1), handler));
		parent->getParent()->setCallback(dwt::Message(WM_MENUSELECT), Dispatcher(std::tr1::bind(&ShellMenu::handleMenuSelect, this, _1), handler));
	}
}

ShellMenu::~ShellMenu() {
	if(handler)
		handler->Release();

	// free all allocated datas
	if(m_psfFolder)
		m_psfFolder->Release();
	m_psfFolder = NULL;
	FreePIDLArray(m_pidlArray);
	m_pidlArray = NULL;
}

void ShellMenu::open(const dwt::ScreenCoordinate& pt, unsigned flags) {
	parent->open(pt, flags);
	if(sel_id >= ID_SHELLCONTEXTMENU_MIN && sel_id <= ID_SHELLCONTEXTMENU_MAX) {
		CMINVOKECOMMANDINFO cmi = { sizeof(CMINVOKECOMMANDINFO) };
		cmi.lpVerb = (LPSTR)MAKEINTRESOURCE(sel_id - ID_SHELLCONTEXTMENU_MIN);
		cmi.nShow = SW_SHOWNORMAL;
		handler->InvokeCommand(&cmi);
	}

	delete this;
}

void ShellMenu::FreePIDLArray(LPITEMIDLIST* pidlArray)
{
	if(!pidlArray)
		return;

	int iSize = _msize (pidlArray) / sizeof (LPITEMIDLIST);

	for(int i = 0; i < iSize; i++)
		free(pidlArray[i]);
	free(pidlArray);
}

bool ShellMenu::handleMenuChar() {
	return true;
}

bool ShellMenu::handleDrawItem(const MSG& msg) {
	if(msg.wParam == 0) {
		LPDRAWITEMSTRUCT t = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
		if(t) {
			unsigned id = t->itemID;
			if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX) {
				return true;
			}
		}
	}
	return false;
}

bool ShellMenu::handleMeasureItem(const MSG& msg) {
	if(msg.wParam == 0) {
		LPMEASUREITEMSTRUCT t = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
		if(t) {
			unsigned id = t->itemID;
			if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX) {
				return true;
			}
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
