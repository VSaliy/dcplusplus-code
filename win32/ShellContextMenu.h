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

#ifndef DCPLUSPLUS_WIN32_SHELL_CONTEXT_MENU_H
#define DCPLUSPLUS_WIN32_SHELL_CONTEXT_MENU_H

class CShellContextMenu
{
	static IContextMenu3* g_IContext3;
	static CShellContextMenu* pShellMenu;
	static WNDPROC OldWndProc;

public:
	CShellContextMenu(dwt::MenuPtr& parent_, const wstring& path);
	~CShellContextMenu();

	void open(const dwt::ScreenCoordinate& pt, unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON);

private:
	dwt::MenuPtr parent;
	dwt::MenuPtr menu;

	IShellFolder* m_psfFolder;
	LPITEMIDLIST* m_pidlArray;

	unsigned sel_id;

	void FreePIDLArray(LPITEMIDLIST* pidlArray);

	static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void handleInitMenuPopup(HMENU hMenu);
	void handleUnInitMenuPopup(HMENU hMenu);
};

#endif // !defined(DCPLUSPLUS_WIN32_SHELL_CONTEXT_MENU_H)
