/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "TextFrame.h"
#include "WinUtil.h"
#include "../client/File.h"

TextFrame* TextFrame::frame = NULL;

#define MAX_TEXT_LEN 32768
LRESULT TextFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE);
	
	ctrlPad.LimitText(0);
	ctrlPad.SetFont(WinUtil::monoFont);
	string tmp;
	try {
		tmp = File(file, File::READ, File::OPEN).read(MAX_TEXT_LEN);
		string::size_type i = 0;
		while((i = tmp.find('\n', i)) != string::npos) {
			if(i == 0 || tmp[i-1] != '\r') {
				tmp.insert(i, 1, '\r');
				i++;
			}
			i++;
		}
		ctrlPad.SetWindowText(tmp.c_str());
		ctrlPad.EmptyUndoBuffer();
		SetWindowText(Util::getFileName(file).c_str());
		File::deleteFile(file);
	} catch(const FileException& e) {
		SetWindowText((Util::getFileName(file) + ": " + e.getError()).c_str());
	}
	
	m_hMenu = WinUtil::mainMenu;

	bHandled = FALSE;
	return 1;
}

void TextFrame::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */)
{
	CRect rc;

	GetClientRect(rc);
	
	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;
	ctrlPad.MoveWindow(rc);
}

/**
 * @file
 * $Id: TextFrame.cpp,v 1.3 2003/10/07 15:46:27 arnetheduck Exp $
 */


