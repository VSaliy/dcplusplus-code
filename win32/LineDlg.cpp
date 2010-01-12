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

#include "LineDlg.h"

#include "WinUtil.h"

LineDlg::LineDlg(dwt::Widget* parent, const tstring& title, const tstring& desc, const tstring& text_, bool password) :
dwt::ModalDialog(parent),
grid(0),
line(0),
text(text_)
{
	onInitDialog(std::tr1::bind(&LineDlg::initDialog, this, title, desc, password));
}

int LineDlg::run() {
	create(dwt::Point(365, 90));
	return show();
}

bool LineDlg::initDialog(const tstring& title, const tstring& desc, bool password) {
	grid = addChild(Grid::Seed(1, 2));
	grid->column(0).mode = GridInfo::FILL;

	line = grid->addChild(GroupBox::Seed(desc))->addChild(WinUtil::Seeds::Dialog::textBox);
	if(password) {
		line->setPassword();
	}
	line->setText(text);
	line->setSelection();

	WinUtil::addDlgButtons(grid->addChild(Grid::Seed(2, 1)),
		std::tr1::bind(&LineDlg::okClicked, this),
		std::tr1::bind(&LineDlg::endDialog, this, IDCANCEL));

	setText(title);

	layout();
	centerWindow();

	return false;
}

void LineDlg::okClicked() {
	text = line->getText();
	endDialog(IDOK);
}

void LineDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
