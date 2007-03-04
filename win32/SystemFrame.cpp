/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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
#include <client/DCPlusPlus.h>

#include "SystemFrame.h"

#include "StupidWin.h"

SystemFrame::SystemFrame(WidgetMDIParentPtr parent) : 
	StaticFrame<SystemFrame>(parent), 
	log(0) 
{
	{
		WidgetTextBox::Seed cs;
		cs.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY;
		cs.exStyle = WS_EX_CLIENTEDGE;
		log = createTextBox(cs);
	}
	
	deque<pair<time_t, string> > oldMessages = LogManager::getInstance()->getLastLogs();
	// Technically, we might miss a message or two here, but who cares...
	LogManager::getInstance()->addListener(this);

	for(deque<pair<time_t, string> >::iterator i = oldMessages.begin(); i != oldMessages.end(); ++i) {
		addLine(i->first, Text::toT(i->second));
	}
	
}

void SystemFrame::addLine(time_t t, const tstring& msg) {
	///@todo add GetWindowTextLength to smartwin & send patch
	int limit = log->getTextLimit();
	if(StupidWin::getWindowTextLength(log) + msg.size() > limit) {
#ifdef PORT_ME
		ctrlPad.SetRedraw(FALSE);
#endif
		///@todo add EM_LINEFROMCHAR to smartwin & send patch
		log->setSelection(0, StupidWin::lineIndex(log, StupidWin::lineFromChar(log, limit / 10)));
		
		log->replaceSelection(_T(""));
#ifdef PORT_ME
		ctrlPad.SetRedraw(TRUE);
#endif
	}
	log->addTextLines(Text::toT("\r\n[" + Util::getShortTimeString(t) + "] ") + msg);
}

#ifdef PORT_ME

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "SystemFrame.h"
#include "WinUtil.h"
#include "../client/File.h"

LRESULT SystemFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.SetFont(WinUtil::font);

	ctrlClientContainer.SubclassWindow(ctrlPad.m_hWnd);

	bHandled = FALSE;
	return 1;
}

LRESULT SystemFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	LogManager::getInstance()->removeListener(this);
	bHandled = FALSE;
	return 0;

}

void SystemFrame::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */)
{
	CRect rc;

	GetClientRect(rc);

	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;
	ctrlPad.MoveWindow(rc);
}

LRESULT SystemFrame::onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlPad.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		tstring x;
		tstring::size_type start = (tstring::size_type)WinUtil::textUnderCursor(pt, ctrlPad, x);
		tstring::size_type end = x.find(_T(" "), start);

		if(end == tstring::npos)
			end = x.length();

		bHandled = WinUtil::parseDBLClick(x, start, end);
	}
	return 0;
}

LRESULT SystemFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	auto_ptr<pair<time_t, tstring> > msg((pair<time_t, tstring>*)wParam);

	addLine(msg->first, msg->second);
	if(BOOLSETTING(BOLD_SYSTEM_LOG))
		setDirty();
	return 0;
}

#endif
