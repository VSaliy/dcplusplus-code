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
#include "FavHubGroupsDlg.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/format.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"
#include "HoldRedraw.h"
#include "TypedTable.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

static const ColumnInfo columns[] = {
	{ N_("Group name"), 100, false },
	{ N_("Private"), 80, false },
	{ N_("Connect"), 80, false }
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
	onInitDialog([this] { return handleInitDialog(); });
	onHelp(&WinUtil::help);
}

FavHubGroupsDlg::~FavHubGroupsDlg() {
}

int FavHubGroupsDlg::run() {
	create(Seed(dwt::Point(400, 450), DS_CONTEXTHELP));
	return show();
}

FavHubGroupsDlg::GroupInfo::GroupInfo(const FavHubGroup& group_) : group(group_) {
	columns[COLUMN_NAME] = Text::toT(group.first);
	columns[COLUMN_PRIVATE] = group.second.priv ? T_("Yes") : T_("No");
	columns[COLUMN_CONNECT] = group.second.connect ? T_("Yes") : T_("No");
}

const tstring& FavHubGroupsDlg::GroupInfo::getText(int col) const {
	return columns[col];
}

int FavHubGroupsDlg::GroupInfo::getImage(int) const {
	return -1;
}

int FavHubGroupsDlg::GroupInfo::compareItems(const GroupInfo* a, const GroupInfo* b, int col) {
	return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
}

bool FavHubGroupsDlg::handleInitDialog() {
	setHelpId(IDH_FAV_HUB_GROUPS);

	grid = addChild(Grid::Seed(4, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	groups = grid->addChild(Groups::Seed(WinUtil::Seeds::Dialog::table));
	groups->setHelpId(IDH_FAV_HUB_GROUPS_LIST);

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		Button::Seed seed(T_("&Update"));
		update = cur->addChild(seed);
		update->setHelpId(IDH_FAV_HUB_GROUPS_UPDATE);
		update->onClicked([this] { handleUpdate(); });

		seed.caption = T_("&Remove");
		remove = cur->addChild(seed);
		remove->setHelpId(IDH_FAV_HUB_GROUPS_REMOVE);
		remove->onClicked([this] { handleRemove(); });
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
			GridPtr cur2 = cur->addChild(Grid::Seed(3, 1));
			cur2->column(0).align = GridInfo::TOP_LEFT;

			priv_box = cur2->addChild(CheckBox::Seed(T_("Mark hubs in this group as private hubs")));
			priv_box->setHelpId(IDH_FAV_HUB_GROUPS_PRIVATE);

			connect_box = cur2->addChild(CheckBox::Seed(T_("Connect to all hubs in this group when the program starts")));
			connect_box->setHelpId(IDH_FAV_HUB_GROUPS_CONNECT);

			ButtonPtr add = cur2->addChild(Button::Seed(T_("&Add to the list")));
			add->setHelpId(IDH_FAV_HUB_GROUPS_ADD);
			add->onClicked([this] { handleAdd(); });
		}
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;

		pair<ButtonPtr, ButtonPtr> buttons = WinUtil::addDlgButtons(cur,
			[this] { handleClose(); },
			[this] { handleClose(); });
		buttons.first->setText(T_("&Close"));
		buttons.second->setVisible(false);
	}

	WinUtil::makeColumns(groups, columns, COLUMN_LAST);

	{
		HoldRedraw hold(groups);

		const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
		for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i)
			add(*i, false);

		groups->setSort(COLUMN_NAME);
	}

	groups->onKeyDown([this](int c) { return handleKeyDown(c); });
	groups->onSelectionChanged([this] { handleSelectionChanged(); });

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
	bool connect;
	if(selected == 1) {
		const FavHubGroup& group = groups->getSelectedData()->group;
		text = Text::toT(group.first);
		priv = group.second.priv;
		connect = group.second.connect;
	} else {
		priv = false;
		connect = false;
	}
	edit->setText(text);
	priv_box->setChecked(priv);
	connect_box->setChecked(connect);
}

void FavHubGroupsDlg::handleUpdate() {
	if(groups->countSelected() != 1)
		return;

	const tstring name = edit->getText();
	const size_t selected = groups->getSelected();
	if(!addable(name, selected))
		return;

	string oldName = groups->getData(selected)->group.first;
	if(name != Text::toT(oldName)) {
		// name of the group changed; propagate the change to hubs that are in this group
		FavoriteHubEntryList hubs = FavoriteManager::getInstance()->getFavoriteHubs(oldName);
		for(FavoriteHubEntryList::iterator i = hubs.begin(); i != hubs.end(); ++i)
			(*i)->setGroup(Text::fromT(name));
	}

	HoldRedraw hold(groups);
	const bool priv = priv_box->getChecked();
	const bool connect = connect_box->getChecked();
	groups->erase(selected);
	add(name, priv, connect);
}

void FavHubGroupsDlg::handleRemove() {
	groups->forEachSelectedT([this](const GroupInfo *group) { removeGroup(group); }, true);
}

void FavHubGroupsDlg::handleAdd() {
	tstring name = edit->getText();
	if(addable(name)) {
		HoldRedraw hold(groups);
		add(name, priv_box->getChecked(), connect_box->getChecked());
	}
}

void FavHubGroupsDlg::handleClose() {
	FavoriteManager::getInstance()->setFavHubGroups(groups->forEachT(GroupCollector()).groups);
	FavoriteManager::getInstance()->save();

	endDialog(IDOK);
}

void FavHubGroupsDlg::add(const FavHubGroup& group, bool ensureVisible) {
	int pos = groups->insert(new GroupInfo(group));
	if(ensureVisible)
		groups->ensureVisible(pos);
}

void FavHubGroupsDlg::add(const tstring& name, bool priv, bool connect) {
	FavHubGroupProperties props = { priv, connect };
	add(FavHubGroup(Text::fromT(name), props));
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
		if(Text::toT(groups->getData(i)->group.first) == name) {
			dwt::MessageBox(this).show(str(TF_("Another group with the name \"%1%\" already exists") % name),
				_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
			return false;
		}
	}

	return true;
}

void FavHubGroupsDlg::removeGroup(const GroupInfo* group) {
	FavoriteHubEntryList hubs = FavoriteManager::getInstance()->getFavoriteHubs(group->group.first);
	size_t count = hubs.size();
	if(count > 0) {
		bool removeChildren = dwt::MessageBox(this).show(str(TF_(
			"The group \"%1%\" contains %2% hubs; remove all of the hubs as well?\n\n"
			"If you select 'Yes', all of these hubs are going to be deleted!\n\n"
			"If you select 'No', these hubs will simply be moved to the main default group.")
			% Text::toT(group->group.first) % count), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == dwt::MessageBox::RETBOX_YES;
		for(FavoriteHubEntryList::iterator j = hubs.begin(); j != hubs.end(); ++j) {
			if(removeChildren && *j != parentEntry)
				FavoriteManager::getInstance()->removeFavorite(*j);
			else
				(*j)->setGroup("");
		}
	}
	groups->erase(group);
}

void FavHubGroupsDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->resize(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));

	groups->setColumnWidth(0, groups->getWindowSize().x - 180);
}
