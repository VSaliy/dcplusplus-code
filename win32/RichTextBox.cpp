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

#include "stdafx.h"

#include "RichTextBox.h"

#include "WinUtil.h"
#include "ParamDlg.h"

RichTextBox::Seed::Seed() : 
BaseType::Seed()
{
}

RichTextBox::RichTextBox(dwt::Widget* parent) : BaseType(parent) {
	onLeftMouseDblClick(std::tr1::bind(&RichTextBox::handleLeftDblClick, this, _1));
}

bool RichTextBox::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(BaseType::handleMessage(msg, retVal))
		return true;

	switch(msg.message)
	{
		// we process these messages here to give the host a chance to handle them differently.

	case WM_KEYDOWN:
		{
			// imitate AspectKeyboard
			return handleKeyDown(static_cast<int>(msg.wParam));
		}

	case WM_CONTEXTMENU:
		{
			// imitate AspectContextMenu
			bool shown = handleContextMenu(dwt::ScreenCoordinate(dwt::Point::fromLParam(msg.lParam)));
			retVal = shown;
			return shown;
		}
	}

	return false;
}

bool RichTextBox::handleKeyDown(int c) {
	switch(c) {
	case VK_F3:
		findTextNext();
		return true;
	case VK_ESCAPE:
		setSelection(-1, -1);
		sendMessage(WM_VSCROLL, SB_BOTTOM);
		clearCurrentNeedle();
		return true;
	}
	return false;
}

bool RichTextBox::handleContextMenu(const dwt::ScreenCoordinate& pt) {
	// This context menu is specialized for non-user-modifiable controls.
	/// @todo add other commands depending on whether the style has ES_READONLY
	MenuPtr menu(dwt::WidgetCreator<Menu>::create(this, WinUtil::Seeds::menu));
	menu->appendItem(T_("&Copy\tCtrl+C"), std::tr1::bind(&RichTextBox::handleCopy, this));
	menu->appendSeparator();
	menu->appendItem(T_("&Find...\tF3"), std::tr1::bind(&RichTextBox::handleFind, this));

	menu->open(pt, TPM_LEFTALIGN | TPM_RIGHTBUTTON);
	return true;
}

void RichTextBox::handleCopy()
{
	WinUtil::setClipboard(getSelection());
}

void RichTextBox::handleFind()
{
	findText(findTextPopup());
}

tstring RichTextBox::findTextPopup() {
	tstring param = Util::emptyStringT;
	ParamDlg lineFind(this, T_("Search"), T_("Specify search string"), Util::emptyStringT, false);
	if(lineFind.run() == IDOK) {
		param = lineFind.getValue();
	}
	return param;
}

void RichTextBox::findTextNext() {
	findText(currentNeedle.empty() ? findTextPopup() : currentNeedle);
}

bool RichTextBox::handleLeftDblClick(const dwt::MouseEvent& ev) {
	return WinUtil::parseDBLClick(textUnderCursor(ev.pos));
}
