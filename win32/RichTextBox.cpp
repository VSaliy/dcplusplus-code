/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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
}

bool RichTextBox::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(msg.message == WM_KEYDOWN) {
		switch(static_cast<int>(msg.wParam)) {
		case 'E': case 'J': case 'L': case 'R':
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			// these don't play well with DC++ since it is sometimes impossible to revert the change
			if(isControlPressed())
				return true;
		}
	}

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
		scrollToBottom();
		clearCurrentNeedle();
		return true;
	}
	return false;
}

#define ID_EDIT_CLEAR 0xE120
#define ID_EDIT_CLEAR_ALL 0xE121
#define ID_EDIT_COPY 0xE122
#define ID_EDIT_CUT 0xE123
#define ID_EDIT_FIND 0xE124
#define ID_EDIT_PASTE 0xE125
#define ID_EDIT_PASTE_LINK 0xE126
#define ID_EDIT_PASTE_SPECIAL 0xE127
#define ID_EDIT_REPEAT 0xE128
#define ID_EDIT_REPLACE 0xE129
#define ID_EDIT_SELECT_ALL 0xE12A
#define ID_EDIT_UNDO 0xE12B
#define ID_EDIT_REDO 0xE12C

bool RichTextBox::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 || pt.y() == -1) {
		pt = getContextMenuPos();
	}

	const bool writable = !hasStyle(ES_READONLY);

	MenuPtr menu(dwt::WidgetCreator<Menu>::create(getParent(), WinUtil::Seeds::menu));
	if(writable) {
		menu->appendItem(T_("&Undo\tCtrl+Z"),
			std::tr1::bind(&RichTextBox::sendMessage, this, WM_COMMAND, MAKEWPARAM(ID_EDIT_UNDO, 0), 0),
			dwt::IconPtr(), sendMessage(EM_CANUNDO));
		menu->appendSeparator();
	}
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
