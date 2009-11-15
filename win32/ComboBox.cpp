/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#include "ComboBox.h"

const TCHAR ListBox::windowClass[] = WC_LISTBOX;

ComboBox::Seed::Seed() :
BaseType::Seed()
{
}

ComboBox::ComboBox(dwt::Widget* parent) :
BaseType(parent),
listBox(0),
textBox(0)
{
}

ListBoxPtr ComboBox::getListBox() {
	if(!listBox) {
		COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
		if(::GetComboBoxInfo(handle(), &info) && info.hwndList && info.hwndList != handle())
			listBox = dwt::WidgetCreator<ListBox>::attach(this, info.hwndList);
	}
	return listBox;
}

TextBoxPtr ComboBox::getTextBox() {
	if(!textBox) {
		COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
		if(::GetComboBoxInfo(handle(), &info) && info.hwndItem && info.hwndItem != handle())
			textBox = dwt::WidgetCreator<TextBox>::attach(this, info.hwndItem);
	}
	return textBox;
}
