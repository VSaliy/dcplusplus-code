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

#include "ParamDlg.h"

#include "WinUtil.h"

ParamDlg::ParamDlg(dwt::Widget* parent, const tstring& title) :
dwt::ModalDialog(parent),
grid(0),
left(0)
{
	onInitDialog(std::tr1::bind(&ParamDlg::initDialog, this, title));
}

ParamDlg::ParamDlg(dwt::Widget* parent, const tstring& title, const tstring& name, const tstring& value, bool password) :
dwt::ModalDialog(parent),
grid(0),
left(0)
{
	onInitDialog(std::tr1::bind(&ParamDlg::initDialog, this, title));
	addTextBox(name, value, password);
}

ParamDlg::ParamDlg(dwt::Widget* parent, const tstring& title, const tstring& name, const TStringList& choices, size_t sel) :
dwt::ModalDialog(parent),
grid(0),
left(0)
{
	onInitDialog(std::tr1::bind(&ParamDlg::initDialog, this, title));
	addComboBox(name, choices, sel);
}

int ParamDlg::run() {
	create(dwt::Point(365, 0)); // the height will be set later on
	return show();
}

void ParamDlg::addTextBox(const tstring& name, const tstring& value, bool password) {
	initFs.push_back(std::tr1::bind(&ParamDlg::initTextBox, this, name, value, password));
}

void ParamDlg::addComboBox(const tstring& name, const TStringList& choices, size_t sel) {
	initFs.push_back(std::tr1::bind(&ParamDlg::initComboBox, this, name, choices, sel));
}

void ParamDlg::initTextBox(const tstring& name, const tstring& value, bool password) {
	TextBoxPtr box = left->addChild(GroupBox::Seed(name))->addChild(WinUtil::Seeds::Dialog::textBox);
	if(password) {
		box->setPassword();
	}
	box->setText(value);
	box->setSelection();
	valueFs.push_back(std::tr1::bind((tstring (TextBox::*)() const)(&TextBox::getText), box));
}

void ParamDlg::initComboBox(const tstring& name, const TStringList& choices, size_t sel) {
	ComboBoxPtr box = left->addChild(GroupBox::Seed(name))->addChild(WinUtil::Seeds::Dialog::comboBox);
	for(TStringList::const_iterator i = choices.begin(), iend = choices.end(); i != iend; ++i)
		box->addValue(*i);
	box->setSelected(sel);
	valueFs.push_back(std::tr1::bind((tstring (ComboBox::*)() const)(&ComboBox::getText), box));
}

bool ParamDlg::initDialog(const tstring& title) {
	grid = addChild(Grid::Seed(1, 2));
	grid->column(0).mode = GridInfo::FILL;

	const size_t n = initFs.size();
	left = grid->addChild(Grid::Seed(n, 1));
	left->column(0).mode = GridInfo::FILL;

	for(size_t i = 0; i < n; ++i)
		initFs[i]();

	WinUtil::addDlgButtons(grid->addChild(Grid::Seed(2, 1)),
		std::tr1::bind(&ParamDlg::okClicked, this),
		std::tr1::bind(&ParamDlg::endDialog, this, IDCANCEL));

	setText(title);

	layout();
	centerWindow();

	return false;
}

void ParamDlg::okClicked() {
	const size_t n = valueFs.size();
	values.resize(n);
	for(size_t i = 0; i < n; ++i)
		values[i] = valueFs[i]();
	endDialog(IDOK);
}

void ParamDlg::layout() {
	dwt::Point sz = getClientSize();
	sz.y = grid->getPreferedSize().y;
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y));

	// now resize the dialog itself
	sz.x = getWindowSize().x; // don't change the horizontal size
	sz.y += 6 + ::GetSystemMetrics(SM_CYSIZE) + 2 * ::GetSystemMetrics(SM_CYEDGE) + 2 * ::GetSystemMetrics(SM_CXFIXEDFRAME);
	dwt::ModalDialog::layout(dwt::Rectangle(sz));
}
