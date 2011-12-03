/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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
#include "UserMatchPage.h"

#include <dcpp/UserMatchManager.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"

#include "HoldRedraw.h"
#include "SettingsDialog.h"
#include "StylesPage.h"
#include "UserMatchDlg.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;

static const ColumnInfo columns[] = {
	{ "", 100, false }
};

/// @todo help
#define IDH_SETTINGS_USER_MATCH_LIST 0
#define IDH_SETTINGS_USER_MATCH_ADD 0
#define IDH_SETTINGS_USER_MATCH_EDIT 0
#define IDH_SETTINGS_USER_MATCH_MOVE_UP 0
#define IDH_SETTINGS_USER_MATCH_MOVE_DOWN 0
#define IDH_SETTINGS_USER_MATCH_REMOVE 0
#define IDH_SETTINGS_USER_MATCH_STYLES 0

UserMatchPage::UserMatchPage(dwt::Widget* parent) :
PropPage(parent, 2, 1),
table(0),
edit(0),
up(0),
down(0),
remove(0),
dirty(false)
{
	setHelpId(IDH_USERMATCHPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto cur = grid->addChild(GroupBox::Seed(T_("User matching definitions")))->addChild(Grid::Seed(2, 5));
		cur->setHelpId(IDH_SETTINGS_USER_MATCH_LIST);
		cur->column(0).mode = GridInfo::FILL;
		cur->column(1).mode = GridInfo::FILL;
		cur->column(2).mode = GridInfo::FILL;
		cur->column(3).mode = GridInfo::FILL;
		cur->column(4).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(grid->getSpacing());

		Table::Seed seed = WinUtil::Seeds::Dialog::table;
		seed.style |= LVS_NOCOLUMNHEADER;
		table = cur->addChild(seed);
		cur->setWidget(table, 0, 0, 1, 5);

		auto button = cur->addChild(Button::Seed(T_("&Add")));
		button->setHelpId(IDH_SETTINGS_USER_MATCH_ADD);
		button->onClicked([this] { handleAddClicked(); });

		edit = cur->addChild(Button::Seed(T_("&Edit")));
		edit->setHelpId(IDH_SETTINGS_USER_MATCH_EDIT);
		edit->onClicked([this] { handleEditClicked(); });

		up = cur->addChild(Button::Seed(T_("Move &Up")));
		up->setHelpId(IDH_SETTINGS_USER_MATCH_MOVE_UP);
		up->onClicked([this] { handleMoveUpClicked(); });

		down = cur->addChild(Button::Seed(T_("Move &Down")));
		down->setHelpId(IDH_SETTINGS_USER_MATCH_MOVE_DOWN);
		down->onClicked([this] { handleMoveDownClicked(); });

		remove = cur->addChild(Button::Seed(T_("&Remove")));
		remove->setHelpId(IDH_SETTINGS_USER_MATCH_REMOVE);
		remove->onClicked([this] { handleRemoveClicked(); });
	}

	{
		auto button = grid->addChild(Grid::Seed(1, 1))->addChild(Button::Seed(T_("Configure styles (fonts / colors) for these user matching definitions")));
		button->setHelpId(IDH_SETTINGS_USER_MATCH_STYLES);
		button->onClicked([this] { static_cast<SettingsDialog*>(getRoot())->activatePage<StylesPage>(); });
	}

	WinUtil::makeColumns(table, columns, 1);

	list = UserMatchManager::getInstance()->getList();
	for(auto i = list.cbegin(), iend = list.cend(); i != iend; ++i) {
		addEntry(*i);
	}

	handleSelectionChanged();

	table->onDblClicked([this] { handleDoubleClick(); });
	table->onKeyDown([this](int c) { return handleKeyDown(c); });
	table->onSelectionChanged([this] { handleSelectionChanged(); });

	// async to avoid problems if one page gets init'd before the other.
	callAsync([this] { updateStyles(); });
}

UserMatchPage::~UserMatchPage() {
}

void UserMatchPage::layout() {
	PropPage::layout();

	table->setColumnWidth(0, table->getWindowSize().x - 20);
}

void UserMatchPage::write() {
	if(dirty) {
		UserMatchManager::getInstance()->setList(std::move(list));
		WinUtil::updateUserMatchFonts();
	}
}

void UserMatchPage::setDirty() {
	dirty = true;
}

void UserMatchPage::handleDoubleClick() {
	if(table->hasSelected()) {
		handleEditClicked();
	} else {
		handleAddClicked();
	}
}

bool UserMatchPage::handleKeyDown(int c) {
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

void UserMatchPage::handleSelectionChanged() {
	auto sel = table->countSelected();
	edit->setEnabled(sel == 1);
	up->setEnabled(sel > 0);
	down->setEnabled(sel > 0);
	remove->setEnabled(sel > 0);
}

void UserMatchPage::handleAddClicked() {
	UserMatchDlg dlg(this);
	if(dlg.run() == IDOK) {
		list.push_back(dlg.getResult());

		addEntry(dlg.getResult());

		setDirty();
		updateStyles();
	}
}

void UserMatchPage::handleEditClicked() {
	auto sel = table->getSelected();
	if(sel == -1)
		return;

	UserMatchDlg dlg(this, &list[sel]);
	if(dlg.run() == IDOK) {
		list[sel] = dlg.getResult();

		table->erase(sel);
		addEntry(dlg.getResult(), sel);

		setDirty();
		updateStyles();
	}
}

void UserMatchPage::handleMoveUpClicked() {
	HoldRedraw hold(table);
	auto sel = table->getSelection();
	for(auto i = sel.cbegin(), iend = sel.cend(); i != iend; ++i) {
		if(*i > 0) {
			std::swap(list[*i], list[*i - 1]);

			table->erase(*i);
			addEntry(list[*i - 1], *i - 1);
			table->select(*i - 1);
		}
	}

	setDirty();
	updateStyles();
}

void UserMatchPage::handleMoveDownClicked() {
	HoldRedraw hold(table);
	auto sel = table->getSelection();
	for(auto i = sel.crbegin(), iend = sel.crend(); i != iend; ++i) {
		if(*i < table->size() - 1) {
			std::swap(list[*i], list[*i + 1]);

			table->erase(*i);
			addEntry(list[*i + 1], *i + 1);
			table->select(*i + 1);
		}
	}

	setDirty();
	updateStyles();
}

void UserMatchPage::handleRemoveClicked() {
	if(!table->hasSelected())
		return;

	if(dwt::MessageBox(this).show(T_("Do you really want to delete selected user matching definitions?"),
		_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES)
	{
		int i;
		while((i = table->getNext(-1, LVNI_SELECTED)) != -1) {
			list.erase(list.begin() + i);

			table->erase(i);
		}
	}

	setDirty();
	updateStyles();
}

void UserMatchPage::addEntry(const UserMatch& matcher, int index) {
	table->insert(TStringList(1, Text::toT(matcher.name)), 0, index);
}

void UserMatchPage::updateStyles() {
	static_cast<SettingsDialog*>(getRoot())->getPage<StylesPage>()->updateUserMatches(list);
}
