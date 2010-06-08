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

#include "resource.h"

#include "StringListDlg.h"

#include "HoldRedraw.h"
#include "ParamDlg.h"
#include "WinUtil.h"

StringListDlg::StringListDlg(dwt::Widget* parent, const TStringList& initialValues) :
dwt::ModalDialog(parent),
grid(0),
editBox(0),
list(0)
{
	onInitDialog(std::tr1::bind(&StringListDlg::handleInitDialog, this, initialValues));
	onHelp(std::tr1::bind(&WinUtil::help, _1, _2));
}

StringListDlg::~StringListDlg() {
}

int StringListDlg::run() {
	create(Seed(dwt::Point(400, 316), DS_CONTEXTHELP));
	return show();
}

void StringListDlg::insert(const tstring& line, int index) {
	int itemCount = list->insert(TStringList(1, line), 0, index);
	if(index == -1)
		index = itemCount;
	list->ensureVisible(index);
}

tstring StringListDlg::getTitle() const {
	return T_("List configuration");
}

tstring StringListDlg::getEditTitle() const {
	return T_("Item edition");
}

tstring StringListDlg::getEditDescription() const {
	return T_("Value");
}

unsigned StringListDlg::getHelpId(HelpFields field) const {
	switch(field) {
		case HELP_DIALOG: return IDH_STRING_LIST;
		case HELP_EDIT_BOX: return IDH_STRING_LIST_EDIT_BOX;
		case HELP_LIST: return IDH_STRING_LIST_LIST;
		case HELP_ADD: return IDH_STRING_LIST_ADD;
		case HELP_MOVE_UP: return IDH_STRING_LIST_MOVE_UP;
		case HELP_MOVE_DOWN: return IDH_STRING_LIST_MOVE_DOWN;
		case HELP_EDIT: return IDH_STRING_LIST_EDIT;
		case HELP_REMOVE: return IDH_STRING_LIST_REMOVE;
	}
}

void StringListDlg::add(const tstring& s) {
	insert(s);
}

bool StringListDlg::handleInitDialog(const TStringList& initialValues) {
	setHelpId(getHelpId(HELP_DIALOG));

	grid = addChild(Grid::Seed(2, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	editBox = grid->addChild(WinUtil::Seeds::Dialog::textBox);
	editBox->setHelpId(getHelpId(HELP_EDIT_BOX));

	{
		ButtonPtr add = grid->addChild(Button::Seed(T_("&Add")));
		add->setHelpId(getHelpId(HELP_ADD));
		add->onClicked(std::tr1::bind(&StringListDlg::handleAddClicked, this));
	}

	{
		Table::Seed seed = WinUtil::Seeds::Dialog::table;
		seed.style |= LVS_NOCOLUMNHEADER;
		list = grid->addChild(seed);
		list->setHelpId(getHelpId(HELP_LIST));
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(6, 1));
		cur->row(4).mode = GridInfo::FILL;
		cur->row(4).align = GridInfo::BOTTOM_RIGHT;

		ButtonPtr button;
		Button::Seed seed;

		seed.caption = T_("Move &Up");
		button = cur->addChild(seed);
		button->setHelpId(getHelpId(HELP_MOVE_UP));
		button->onClicked(std::tr1::bind(&StringListDlg::handleMoveUpClicked, this));

		seed.caption = T_("Move &Down");
		button = cur->addChild(seed);
		button->setHelpId(getHelpId(HELP_MOVE_DOWN));
		button->onClicked(std::tr1::bind(&StringListDlg::handleMoveDownClicked, this));

		seed.caption = T_("&Edit");
		button = cur->addChild(seed);
		button->setHelpId(getHelpId(HELP_EDIT));
		button->onClicked(std::tr1::bind(&StringListDlg::handleEditClicked, this));

		seed.caption = T_("&Remove");
		button = cur->addChild(seed);
		button->setHelpId(getHelpId(HELP_REMOVE));
		button->onClicked(std::tr1::bind(&StringListDlg::handleRemoveClicked, this));

		WinUtil::addDlgButtons(cur,
			std::tr1::bind(&StringListDlg::handleOKClicked, this),
			std::tr1::bind(&StringListDlg::endDialog, this, IDCANCEL));
	}

	list->createColumns(TStringList(1));

	for(TStringIterC i = initialValues.begin(), iend = initialValues.end(); i != iend; ++i)
		insert(*i);

	list->onDblClicked(std::tr1::bind(&StringListDlg::handleDoubleClick, this));
	list->onKeyDown(std::tr1::bind(&StringListDlg::handleKeyDown, this, _1));

	setText(getTitle());

	layout();
	centerWindow();

	return false;
}

void StringListDlg::handleDoubleClick() {
	if(list->hasSelected()) {
		handleEditClicked();
	}
}

bool StringListDlg::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddClicked();
		return true;
	case VK_DELETE:
		handleRemoveClicked();
		return true;
	}
	return false;
}

void StringListDlg::handleAddClicked() {
	add(editBox->getText());
}

void StringListDlg::handleMoveUpClicked() {
	HoldRedraw hold(list);
	std::vector<unsigned> selected = list->getSelection();
	for(std::vector<unsigned>::const_iterator i = selected.begin(); i != selected.end(); ++i) {
		if(*i > 0) {
			tstring selText = list->getText(*i, 0);
			list->erase(*i);
			insert(selText, *i - 1);
			list->select(*i - 1);
		}
	}
}

void StringListDlg::handleMoveDownClicked() {
	HoldRedraw hold(list);
	std::vector<unsigned> selected = list->getSelection();
	for(std::vector<unsigned>::reverse_iterator i = selected.rbegin(); i != selected.rend(); ++i) {
		if(*i < list->size() - 1) {
			tstring selText = list->getText(*i, 0);
			list->erase(*i);
			insert(selText, *i + 1);
			list->select(*i + 1);
		}
	}
}

void StringListDlg::handleEditClicked() {
	int i = -1;
	while((i = list->getNext(i, LVNI_SELECTED)) != -1) {
		ParamDlg dlg(this, getEditTitle(), getEditDescription(), list->getText(i, 0));
		if(dlg.run() == IDOK)
			list->setText(i, 0, dlg.getValue());
	}
}

void StringListDlg::handleRemoveClicked() {
	int i;
	while((i = list->getNext(-1, LVNI_SELECTED)) != -1)
		list->erase(i);
}

void StringListDlg::handleOKClicked() {
	for(int i = 0, iend = list->size(); i < iend; ++i)
		values.push_back(list->getText(i, 0));

	endDialog(IDOK);
}

void StringListDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));

	list->setColumnWidth(0, list->getWindowSize().x - 20);
}
