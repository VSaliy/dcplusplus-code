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

/*
* Based on a class by R. Engels
* http://www.codeproject.com/shell/shellcontextmenu.asp
*/

#include "stdafx.h"

#include "ShellContextMenu.h"
#include "resource.h"

IContextMenu3* CShellContextMenu::g_IContext3 = 0;
CShellContextMenu* CShellContextMenu::pShellMenu = 0;
WNDPROC CShellContextMenu::OldWndProc = 0;

CShellContextMenu::CShellContextMenu(dwt::MenuPtr& parent_, const wstring& path) :
parent(parent_),
m_psfFolder(NULL),
m_pidlArray(NULL),
sel_id(0),
sel_handle(0)
{
	parent->appendSeparator();
	menu = parent->appendPopup(dwt::Menu::Seed(false), _T("Shell menu"));

	// get IShellFolder interface of Desktop(root of shell namespace)
	IShellFolder* psfDesktop = NULL;
	SHGetDesktopFolder(&psfDesktop);

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;
	psfDesktop->ParseDisplayName(NULL, NULL, (LPOLESTR)const_cast<WCHAR*>(path.c_str()), NULL, &pidl, NULL);

	// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
	SHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&m_psfFolder, NULL);

	// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc(&lpMalloc);
	lpMalloc->Free(pidl);

	// now we need the relative pidl
	IShellFolder* psfFolder = NULL;
	psfDesktop->ParseDisplayName (NULL, NULL, (LPOLESTR)const_cast<WCHAR*>(path.c_str()), NULL, &pidl, NULL);
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

	LPCONTEXTMENU pContextMenu = 0;
	LPCONTEXTMENU icm1 = 0;
	// first we retrieve the normal IContextMenu interface (every object should have it)
	m_psfFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *) m_pidlArray, IID_IContextMenu, NULL, (LPVOID*) &icm1);
	if(icm1 && (icm1->QueryInterface(IID_IContextMenu3, (LPVOID*)&pContextMenu) == NOERROR) && pContextMenu) {
		icm1->Release(); // we can now release version 1 interface, cause we got a higher one

		g_IContext3 = (LPCONTEXTMENU3)pContextMenu;
		pShellMenu = this;
		OldWndProc = (WNDPROC)::SetWindowLongPtr(menu->getParent()->handle(), GWLP_WNDPROC, (LONG_PTR)&HookWndProc);
	}
}

CShellContextMenu::~CShellContextMenu() {
	if(OldWndProc)
		::SetWindowLongPtr(menu->getParent()->handle(), GWLP_WNDPROC, (LONG_PTR)OldWndProc);

	if(g_IContext3)
		g_IContext3->Release();

	// free all allocated datas
	if(m_psfFolder)
		m_psfFolder->Release();
	m_psfFolder = NULL;
	FreePIDLArray(m_pidlArray);
	m_pidlArray = NULL;
}

void CShellContextMenu::open(const dwt::ScreenCoordinate& pt, unsigned flags) {
	{
		// remove the MNS_NOTIFYBYPOS style
		MENUINFO mi = { sizeof(MENUINFO), MIM_STYLE };
		if(!::GetMenuInfo(parent->handle(), &mi))
			return;
		if(mi.dwStyle & MNS_NOTIFYBYPOS) {
			mi.dwStyle &= ~MNS_NOTIFYBYPOS;
			if(!::SetMenuInfo(parent->handle(), &mi))
				return;
		}
	}

	unsigned id = parent->open(pt, flags | TPM_RETURNCMD);
	if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX)
		InvokeCommand(g_IContext3, id - ID_SHELLCONTEXTMENU_MIN);
	else
		parent->getParent()->postMessage(WM_MENUCOMMAND, sel_id, sel_handle);

	delete this;
}

void CShellContextMenu::FreePIDLArray(LPITEMIDLIST* pidlArray)
{
	if(!pidlArray)
		return;

	int iSize = _msize (pidlArray) / sizeof (LPITEMIDLIST);

	for(int i = 0; i < iSize; i++)
		free(pidlArray[i]);
	free(pidlArray);
}

void CShellContextMenu::InvokeCommand(LPCONTEXTMENU pContextMenu, unsigned id) {
	CMINVOKECOMMANDINFO cmi = { sizeof(CMINVOKECOMMANDINFO) };
	cmi.lpVerb = (LPSTR)MAKEINTRESOURCE(id);
	cmi.nShow = SW_SHOWNORMAL;
	pContextMenu->InvokeCommand(&cmi);
}

LRESULT CALLBACK CShellContextMenu::HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
	case WM_MENUCHAR:
		{
			LRESULT ret = 0;
			g_IContext3->HandleMenuMsg2(message, wParam, lParam, &ret);
			return ret;
		}

	case WM_MEASUREITEM:
		{
			if(wParam == 0) {
				unsigned id = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam)->itemID;
				if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX) {
					LRESULT ret = 0;
					g_IContext3->HandleMenuMsg2(message, wParam, lParam, &ret);
					return ret;
				}
			}
			break;
		}

	case WM_DRAWITEM:
		{
			if(wParam == 0) {
				unsigned id = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam)->itemID;
				if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX) {
					LRESULT ret = 0;
					g_IContext3->HandleMenuMsg2(message, wParam, lParam, &ret);
					return ret;
				}
			}
			break;
		}

	case WM_INITMENUPOPUP:
		{
			pShellMenu->handleInitMenuPopup(reinterpret_cast<HMENU>(wParam));

			LRESULT ret = 0;
			g_IContext3->HandleMenuMsg2(message, wParam, lParam, &ret);
			return ret;
		}

	case WM_UNINITMENUPOPUP:
		{
			pShellMenu->handleUnInitMenuPopup(reinterpret_cast<HMENU>(wParam));

			LRESULT ret = 0;
			g_IContext3->HandleMenuMsg2(message, wParam, lParam, &ret);
			return ret;
		}

	case WM_MENUSELECT:
		{
			// make sure this isn't a "menu closed" signal
			if((HIWORD(wParam) == 0xFFFF) && (lParam == 0))
				break;

			unsigned id = LOWORD(wParam);
			if(id < ID_SHELLCONTEXTMENU_MIN || id > ID_SHELLCONTEXTMENU_MAX) {
				/*
				* this is not an item added by the Shell menu; save the id (which is an index,
				* actually) and the handle in case we have to dispatch them with WM_MENUCOMMAND.
				*/
				pShellMenu->sel_id = id;
				pShellMenu->sel_handle = lParam;
			}
			break;
		}
	}

	return ::CallWindowProc(OldWndProc, hWnd, message, wParam, lParam);
}

void CShellContextMenu::handleInitMenuPopup(HMENU hMenu) {
	if(hMenu == menu->handle()) {
		g_IContext3->QueryContextMenu(menu->handle(), 0, ID_SHELLCONTEXTMENU_MIN, ID_SHELLCONTEXTMENU_MAX, CMF_NORMAL | CMF_EXPLORE);
	}
}

void CShellContextMenu::handleUnInitMenuPopup(HMENU hMenu) {
	if(hMenu == menu->handle()) {
		menu->removeAllItems();
	}
}
