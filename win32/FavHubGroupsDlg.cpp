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

#include "FavHubGroupsDlg.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>

#include "HoldRedraw.h"
#include "WinUtil.h"

static const ColumnInfo columns[] = {
	{ N_("Group name"), 100, false },
	{ N_("Private"), 50, false }
};

FavHubGroupsDlg::FavHubGroupsDlg(dwt::Widget* parent, FavoriteHubEntry* parentEntry_) :
dwt::ModalDialog(parent),
grid(0),
groups(0),
update(0),
remove(0),
properties(0),
edit(0),
priv_box(0),
parentEntry(parentEntry_)
{
	onInitDialog(std::tr1::bind(&FavHubGroupsDlg::handleInitDialog, this));
	onHelp(std::tr1::bind(&WinUtil::help, _1, _2));
}

FavHubGroupsDlg::~FavHubGroupsDlg() {
}

int FavHubGroupsDlg::run() {
	create(Seed(dwt::Point(300, 450), DS_CONTEXTHELP));
	return show();
}

bool FavHubGroupsDlg::handleInitDialog() {
	setHelpId(IDH_FAV_HUB_GROUPS);

	grid = addChild(Grid::Seed(4, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		Table::Seed seed = WinUtil::Seeds::Dialog::table;
		seed.lvStyle |= LVS_EX_HEADERDRAGDROP;
		groups = grid->addChild(seed);
		groups->setHelpId(IDH_FAV_HUB_GROUPS_LIST);
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		Button::Seed seed(T_("&Update"));
		update = cur->addChild(seed);
		update->setHelpId(IDH_FAV_HUB_GROUPS_UPDATE);
		update->onClicked(std::tr1::bind(&FavHubGroupsDlg::handleUpdate, this));

		seed.caption = T_("&Remove");
		remove = cur->addChild(seed);
		remove->setHelpId(IDH_FAV_HUB_GROUPS_REMOVE);
		remove->onClicked(std::tr1::bind(&FavHubGroupsDlg::handleRemove, this));
	}

	{
		properties = grid->addChild(GroupBox::Seed(T_("Group properties")));
		GridPtr cur = properties->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		{
			GridPtr cur2 = cur->addChild(Grid::Seed(1, 2));
			cur2->column(1).mode = GridInfo::FILL;
			cur2->setHelpId(IDH_FAV_HUB_GROUPS_NAME);

			cur2->addChild(Label::Seed(T_("Name")));

			edit = cur2->addChild(WinUtil::Seeds::Dialog::textBox);
		}

		{
			GridPtr cur2 = cur->addChild(Grid::Seed(2, 1));
			cur2->column(0).align = GridInfo::TOP_LEFT;

			priv_box = cur2->addChild(CheckBox::Seed(T_("Mark hubs in this group as private hubs")));
			priv_box->setHelpId(IDH_FAV_HUB_GROUPS_PRIVATE);

			ButtonPtr add = cur2->addChild(Button::Seed(T_("&Add to the list")));
			add->setHelpId(IDH_FAV_HUB_GROUPS_ADD);
			add->onClicked(std::tr1::bind(&FavHubGroupsDlg::handleAdd, this));
		}
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;

		pair<ButtonPtr, ButtonPtr> buttons = WinUtil::addDlgButtons(cur,
			std::tr1::bind(&FavHubGroupsDlg::handleClose, this),
			std::tr1::bind(&FavHubGroupsDlg::handleClose, this));
		buttons.first->setText(T_("&Close"));
		buttons.second->setVisible(false);
	}

	WinUtil::makeColumns(groups, columns, 2);

	{
		HoldRedraw hold(groups);

		const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
		for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i)
			add(Text::toT(i->first), i->second.priv);

		groups->setSort(0, Table::SORT_STRING_NOCASE);
	}

	groups->onKeyDown(std::tr1::bind(&FavHubGroupsDlg::handleKeyDown, this, _1));
	groups->onSelectionChanged(std::tr1::bind(&FavHubGroupsDlg::handleSelectionChanged, this));

	handleSelectionChanged();

	setText(T_("Favorite hub groups"));

	layout();
	centerWindow();

	return false;
}

bool FavHubGroupsDlg::handleKeyDown(int c) {
	switch(c) {
	case VK_DELETE:
		handleRemove();
		return true;
	}
	return false;
}

void FavHubGroupsDlg::handleSelectionChanged() {
	size_t selected = groups->countSelected();
	update->setEnabled(selected == 1);
	remove->setEnabled(selected > 0);

	tstring text;
	bool priv;
	if(selected == 1) {
		size_t sel = groups->getSelected();
		text = groups->getText(sel, 0);
		priv = isPrivate(sel);
	} else {
		priv = false;
	}
	edit->setText(text);
	priv_box->setChecked(priv);
}

void FavHubGroupsDlg::handleUpdate() {
	if(groups->countSelected() != 1)
		return;

	const tstring name = edit->getText();
	const size_t selected = groups->getSelected();
	if(!addable(name, selected))
		return;
	const bool priv = priv_box->getChecked();

	tstring oldName = groups->getText(selected, 0);
	if(name != oldName) {
		// name of the group changed; propagate the change to hubs that are in this group
		FavoriteHubEntryList hubs = FavoriteManager::getInstance()->getFavoriteHubs(Text::fromT(oldName));
		for(FavoriteHubEntryList::iterator i = hubs.begin(); i != hubs.end(); ++i)
			(*i)->setGroup(Text::fromT(name));
	}

	HoldRedraw hold(groups);
	groups->erase(selected);
	add(name, priv);
	groups->resort();
}

void FavHubGroupsDlg::handleRemove() {
	int i;
	while((i = groups->getNext(-1, LVNI_SELECTED)) != -1) {
		tstring name = groups->getText(i, 0);
		FavoriteHubEntryList hubs = FavoriteManager::getInstance()->getFavoriteHubs(Text::fromT(name));
		size_t count = hubs.size();
		if(count > 0) {
			bool removeChildren = dwt::MessageBox(this).show(str(TF_(
				"The group \"%1%\" contains %2% hubs; remove all of the hubs as well?\n\n"
				"If you select 'Yes', all of these hubs are going to be deleted!\n\n"
				"If you select 'No', these hubs will simply be moved to the main default group.") % name % count),
				_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION)
				== dwt::MessageBox::RETBOX_YES;
			for(FavoriteHubEntryList::iterator j = hubs.begin(); j != hubs.end(); ++j) {
				if(removeChildren && *j != parentEntry)
					FavoriteManager::getInstance()->removeFavorite(*j);
				else
					(*j)->setGroup("");
			}
		}
		groups->erase(i);
	}
}

void FavHubGroupsDlg::handleAdd() {
	tstring text = edit->getText();
	if(addable(text)) {
		HoldRedraw hold(groups);
		add(text, priv_box->getChecked());
		groups->resort();
	}
}

void FavHubGroupsDlg::handleClose() {
	FavHubGroups favHubGroups;
	for(size_t i = 0, size = groups->size(); i < size; ++i) {
		FavHubGroupProperties props = { isPrivate(i) };
		favHubGroups[Text::fromT(groups->getText(i, 0))] = props;
	}
	FavoriteManager::getInstance()->setFavHubGroups(favHubGroups);
	FavoriteManager::getInstance()->save();

	endDialog(IDOK);
}

void FavHubGroupsDlg::add(const tstring& name, bool priv) {
	TStringList row;
	row.push_back(name);
	row.push_back(priv ? T_("Yes") : T_("No"));
	groups->insert(row);
}

bool FavHubGroupsDlg::isPrivate(size_t row) const {
	return groups->getText(row, 1) == T_("Yes");
}

bool FavHubGroupsDlg::addable(const tstring& name, int ignore) {
	if(name.empty()) {
		dwt::MessageBox(this).show(T_("You must enter a name"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return false;
	}

	for(size_t i = 0, size = groups->size(); i < size; ++i) {
		if(ignore >= 0 && i == static_cast<size_t>(ignore))
			continue;
		if(groups->getText(i, 0) == name) {
			dwt::MessageBox(this).show(str(TF_("Another group with the name \"%1%\" already exists") % name),
				_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
			return false;
		}
	}

	return true;
}

void FavHubGroupsDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));

	groups->setColumnWidth(0, groups->getSize().x - 80);
}
