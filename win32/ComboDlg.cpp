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

#include "resource.h"

#include "ComboDlg.h"

#include "WinUtil.h"

ComboDlg::ComboDlg(dwt::Widget* parent, const tstring& title, const tstring& desc, const TStringList& values, size_t sel) :
dwt::ModalDialog(parent),
grid(0),
combo(0)
{
	onInitDialog(std::tr1::bind(&ComboDlg::initDialog, this, title, desc, values, sel));
}

int ComboDlg::run() {
	create(dwt::Point(365, 83));
	return show();
}

bool ComboDlg::initDialog(const tstring& title, const tstring& desc, const TStringList& values, size_t sel) {
	grid = addChild(Grid::Seed(1, 2));
	grid->column(0).mode = GridInfo::FILL;

	combo = grid->addChild(GroupBox::Seed(desc))->addChild(WinUtil::Seeds::Dialog::comboBox);
	for(TStringList::const_iterator i = values.begin(), iend = values.end(); i != iend; ++i)
		combo->addValue(*i);
	combo->setSelected(sel);

	WinUtil::addDlgButtons(grid->addChild(Grid::Seed(2, 1)),
		std::tr1::bind(&ComboDlg::okClicked, this),
		std::tr1::bind(&ComboDlg::endDialog, this, IDCANCEL));

	setText(title);

	layout();
	centerWindow();

	return false;
}

void ComboDlg::okClicked() {
	value = combo->getText();
	endDialog(IDOK);
}

void ComboDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
