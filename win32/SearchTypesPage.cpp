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

#include "SearchTypesPage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/SearchManager.h>
#include <dcpp/version.h>
#include "ParamDlg.h"
#include "WinUtil.h"
#include "SearchTypeDlg.h" 

static const ColumnInfo columns[] = {
	{ N_("Search type"), 110, false },
	{ N_("Predefined"), 70, false },
	{ N_("Extensions"), 100, false }
};

SearchTypesPage::SearchTypesPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
types(0),
rename(0),
remove(0),
modify(0)
{
	setHelpId(IDH_SEARCHTYPESPAGE);

	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Search types")));
		group->setHelpId(IDH_SETTINGS_SEARCHTYPES_LIST);

		GridPtr cur = group->addChild(Grid::Seed(4, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(6);

		{
			Table::Seed seed = WinUtil::Seeds::Dialog::table;
			seed.style |= LVS_SINGLESEL;
			types = cur->addChild(seed);
		}

		{
			GridPtr row = cur->addChild(Grid::Seed(1, 5));
			row->column(0).mode = GridInfo::FILL;
			row->column(1).mode = GridInfo::FILL;
			row->column(2).mode = GridInfo::FILL;
			row->column(3).mode = GridInfo::FILL;
			row->column(4).mode = GridInfo::FILL;

			ButtonPtr button = row->addChild(Button::Seed(T_("&Add")));
			button->setHelpId(IDH_SETTINGS_SEARCHTYPES_ADD);
			button->onClicked(std::bind(&SearchTypesPage::handleAddClicked, this));

			modify = row->addChild(Button::Seed(T_("M&odify")));
			modify->setHelpId(IDH_SETTINGS_SEARCHTYPES_MODIFY);
			modify->onClicked(std::bind(&SearchTypesPage::handleModClicked, this));

			rename = row->addChild(Button::Seed(T_("Re&name")));
			rename->setHelpId(IDH_SETTINGS_SEARCHTYPES_RENAME);
			rename->onClicked(std::bind(&SearchTypesPage::handleRenameClicked, this));

			remove = row->addChild(Button::Seed(T_("&Remove")));
			remove->setHelpId(IDH_SETTINGS_SEARCHTYPES_REMOVE);
			remove->onClicked(std::bind(&SearchTypesPage::handleRemoveClicked, this));

			button = row->addChild(Button::Seed(T_("&Defaults")));
			button->setHelpId(IDH_SETTINGS_SEARCHTYPES_DEFAULTS);
			button->onClicked(std::bind(&SearchTypesPage::handleDefaultsClicked, this));
		}

		cur->addChild(Label::Seed(T_("Note; Custom search types will only be applied to ADC hubs!")));
	}

	WinUtil::makeColumns(types, columns, 3);
	types->setSort(0, dwt::Table::SORT_STRING_NOCASE);

	fillList();

	handleSelectionChanged();

	types->onDblClicked(std::bind(&SearchTypesPage::handleDoubleClick, this));
	types->onKeyDown(std::bind(&SearchTypesPage::handleKeyDown, this, _1));
	types->onSelectionChanged(std::bind(&SearchTypesPage::handleSelectionChanged, this));
}

SearchTypesPage::~SearchTypesPage() {
}

void SearchTypesPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));

	types->setColumnWidth(2, types->getWindowSize().x - 190);
}

void SearchTypesPage::handleDoubleClick() {
	if(types->hasSelected()) {
		handleModClicked();
	} else {
		handleAddClicked();
	}
}

bool SearchTypesPage::handleKeyDown(int c) {
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

void SearchTypesPage::handleSelectionChanged() {
	bool sel = types->hasSelected();
	bool changeable = sel && types->getText(types->getSelected(), 1).empty();
	rename->setEnabled(changeable);
	remove->setEnabled(changeable);
	modify->setEnabled(sel);
}

void SearchTypesPage::handleAddClicked() {
	ParamDlg dlg(this, T_("New search type"), T_("Name of the new search type"));
	if(dlg.run() == IDOK) {
		const tstring& name = dlg.getValue();
		try { SettingsManager::getInstance()->validateSearchTypeName(Text::fromT(name)); }
		catch(const SearchTypeException& e) { showError(e.getError()); return; }

		SearchTypeDlg dlg(this, name, TStringList());
		if(dlg.run() == IDOK) {
			const TStringList& values = dlg.getValues();
			if(!values.empty()) {
				try {
					StringList extList;
					Text::fromT(values, extList);
					SettingsManager::getInstance()->addSearchType(Text::fromT(name), extList, true);
					addRow(name, false, values);
					types->resort();
				} catch(const SearchTypeException& e) {
					showError(e.getError());
				}
			}
		}
	}
}

void SearchTypesPage::handleModClicked() {
	if(!types->hasSelected())
		return;

	int cur = types->getSelected();
	tstring caption = types->getText(cur, 0);
	string name = Text::fromT(caption);
	if(!types->getText(cur, 1).empty()) {
		findRealName(name);
	}

	TStringList extListT;
	try {
		Text::toT(SettingsManager::getInstance()->getExtensions(name), extListT);
	} catch(const SearchTypeException& e) {
		showError(e.getError());
		return;
	}

	SearchTypeDlg dlg(this, caption, extListT);
	if(dlg.run() == IDOK) {
		StringList extList;
		Text::fromT(dlg.getValues(), extList);
		try {
			SettingsManager::getInstance()->modSearchType(name, extList);
			types->setText(cur, 2, Text::toT(Util::toString(";", extList)));
		} catch(const SearchTypeException& e) {
			showError(e.getError());
		}
	}
}

void SearchTypesPage::handleDefaultsClicked() {
	if(dwt::MessageBox(this).show(T_("This will delete all defined search types and restore the default ones. Do you want to continue?"),
		_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONEXCLAMATION) == IDYES)
	{
		SettingsManager::getInstance()->setSearchTypeDefaults();
		types->clear();
		fillList();
	}
}

void SearchTypesPage::handleRenameClicked() {
	if(!types->hasSelected())
		return;

	int cur = types->getSelected();
	ParamDlg dlg(this, T_("Rename a search type"), T_("New name"), types->getText(cur, 0));
	if(dlg.run() == IDOK) {
		const tstring& newName = dlg.getValue();
		try {
			SettingsManager::getInstance()->renameSearchType(Text::fromT(types->getText(cur, 0)), Text::fromT(newName));
			types->setText(cur, 0, newName);
			types->resort();
		} catch(const SearchTypeException& e) {
			showError(e.getError());
		}
	}
}

void SearchTypesPage::handleRemoveClicked() {
	if(!types->hasSelected())
		return;

	if(dwt::MessageBox(this).show(T_("Do you really want to delete this search type?"),
		_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONEXCLAMATION) == IDYES)
	{
		int cur = types->getSelected();
		try {
			SettingsManager::getInstance()->delSearchType(Text::fromT(types->getText(cur, 0)));
			types->erase(cur);
		} catch(const SearchTypeException& e) {
			showError(e.getError());
		}
	}
}

void SearchTypesPage::addRow(const tstring& name, bool predefined, const TStringList& extensions) {
	TStringList row;
	row.push_back(name);
	row.push_back(predefined ? T_("Predefined") : Util::emptyStringT);
	row.push_back(Util::toString(_T(";"), extensions));
	types->insert(row);
}

void SearchTypesPage::findRealName(string& name) const {
	for(int i = 1; i <= 6; ++i) {
		if(SearchManager::getTypeStr(i) == name) {
			name = '0' + i;
			break;
		}
	}
}

void SearchTypesPage::fillList() {
	const SettingsManager::SearchTypes& searchTypes = SettingsManager::getInstance()->getSearchTypes();
	for(SettingsManager::SearchTypesIterC i = searchTypes.begin(), iend = searchTypes.end(); i != iend; ++i) {
		string name = i->first;
		bool predefined = false;
		if(name.size() == 1 && name[0] >= '1' && name[0] <= '6') {
			name = SearchManager::getTypeStr(name[0] - '0');
			predefined = true;
		}
		TStringList exts;
		Text::toT(i->second, exts);
		addRow(Text::toT(name), predefined, exts);
	}
	types->resort();
}

void SearchTypesPage::showError(const string& e) {
	dwt::MessageBox(this).show(Text::toT(e), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
		dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
}
