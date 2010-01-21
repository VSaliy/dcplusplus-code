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

#include "FavHubsFrame.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>

#include "HoldRedraw.h"
#include "HubFrame.h"
#include "FavHubProperties.h"
#include "FavHubGroupsDlg.h"

const string FavHubsFrame::id = "FavHubs";
const string& FavHubsFrame::getId() const { return id; }

static const ColumnInfo hubsColumns[] = {
	{ N_("Name"), 200, false },
	{ N_("Description"), 290, false },
	{ N_("Nick"), 125, false },
	{ N_("Password"), 100, false },
	{ N_("Server"), 100, false },
	{ N_("User Description"), 125, false },
	{ N_("Group"), 100, false }
};

FavHubsFrame::FavHubsFrame(dwt::TabView* mdiParent) :
BaseType(mdiParent, T_("Favorite Hubs"), IDH_FAVORITE_HUBS, IDR_FAVORITE_HUBS),
grid(0),
hubs(0)
{
	grid = addChild(Grid::Seed(2, 7));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;
	grid->column(3).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;
	grid->column(5).mode = GridInfo::FILL;
	grid->column(6).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		Table::Seed cs = WinUtil::Seeds::table;
		cs.style |= LVS_NOSORTHEADER;
		hubs = grid->addChild(cs);
		grid->setWidget(hubs, 0, 0, 1, 7);
		addWidget(hubs, false, true, false); /// @todo group headers never change colors so for now, we keep default Win colors

		WinUtil::makeColumns(hubs, hubsColumns, COLUMN_LAST, SETTING(FAVHUBSFRAME_ORDER), SETTING(FAVHUBSFRAME_WIDTHS));

		hubs->onDblClicked(std::tr1::bind(&FavHubsFrame::handleDoubleClick, this));
		hubs->onKeyDown(std::tr1::bind(&FavHubsFrame::handleKeyDown, this, _1));
		hubs->onContextMenu(std::tr1::bind(&FavHubsFrame::handleContextMenu, this, _1));
	}

	{
		ButtonPtr button;
		Button::Seed cs = WinUtil::Seeds::button;

		cs.caption = T_("&Connect");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_CONNECT);
		button->onClicked(std::tr1::bind(&FavHubsFrame::openSelected, this));
		addWidget(button);

		cs.caption = T_("&New...");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_NEW);
		button->onClicked(std::tr1::bind(&FavHubsFrame::handleAdd, this));
		addWidget(button);

		cs.caption = T_("&Properties");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_PROPERTIES);
		button->onClicked(std::tr1::bind(&FavHubsFrame::handleProperties, this));
		addWidget(button);

		cs.caption = T_("Move &Up");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_MOVE_UP);
		button->onClicked(std::tr1::bind(&FavHubsFrame::handleMove, this, true));
		addWidget(button);

		cs.caption = T_("Move &Down");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_MOVE_DOWN);
		button->onClicked(std::tr1::bind(&FavHubsFrame::handleMove, this, false));
		addWidget(button);

		cs.caption = T_("&Remove");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_REMOVE);
		button->onClicked(std::tr1::bind(&FavHubsFrame::handleRemove, this));
		addWidget(button);

		cs.caption = T_("Manage &groups");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_MANAGE_GROUPS);
		button->onClicked(std::tr1::bind(&FavHubsFrame::handleGroups, this));
		addWidget(button);
	}

	initStatus();

	layout();
	activate();

	fillList();

	FavoriteManager::getInstance()->addListener(this);
}

FavHubsFrame::~FavHubsFrame() {

}

void FavHubsFrame::layout() {
	dwt::Rectangle r(getClientSize());

	status->layout(r);

	grid->layout(r);
}

bool FavHubsFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);
	return true;
}

void FavHubsFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::FAVHUBSFRAME_ORDER, WinUtil::toString(hubs->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::FAVHUBSFRAME_WIDTHS, WinUtil::toString(hubs->getColumnWidths()));
}

FavHubsFrame::SelectionKeeper::SelectionKeeper(TablePtr hubs_) : hubs(hubs_) {
	// in grouped mode, the indexes of each item are completely random, so use entry pointers instead
	std::vector<unsigned> selection = hubs->getSelection();
	for(std::vector<unsigned>::const_iterator i = selection.begin(), iend = selection.end(); i != iend; ++i)
		selected.push_back(reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(*i)));
}

FavHubsFrame::SelectionKeeper::~SelectionKeeper() {
	for(FavoriteHubEntryList::const_iterator i = selected.begin(), iend = selected.end(); i != iend; ++i) {
		hubs->select(hubs->findData(reinterpret_cast<LPARAM>(*i)));
	}
}

const FavoriteHubEntryList& FavHubsFrame::SelectionKeeper::getSelection() const {
	return selected;
}

void FavHubsFrame::handleAdd() {
	FavoriteHubEntry e;

	while(true) {
		FavHubProperties dlg(this, &e);
		if(dlg.run() == IDOK) {
			if(FavoriteManager::getInstance()->isFavoriteHub(e.getServer())) {
				dwt::MessageBox(this).show(T_("Hub already exists as a favorite"),
					_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
			} else {
				FavoriteManager::getInstance()->addFavorite(e);
				break;
			}
		} else
			break;
	}
}

void FavHubsFrame::handleProperties() {
	if(hubs->countSelected() == 1) {
		FavHubProperties dlg(this, reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(hubs->getSelected())));
		if(dlg.run() == IDOK) {
			HoldRedraw hold(hubs);
			SelectionKeeper keeper(hubs);
			refresh();
		}
	}
}

void FavHubsFrame::handleMove(bool up) {
	FavoriteHubEntryList& fh = FavoriteManager::getInstance()->getFavoriteHubs();

	HoldRedraw hold(hubs);
	SelectionKeeper keeper(hubs);
	const FavoriteHubEntryList& selected = keeper.getSelection();

	FavoriteHubEntryList fh_copy = fh;
	if(!up)
		reverse(fh_copy.begin(), fh_copy.end());
	for(FavoriteHubEntryList::iterator i = fh_copy.begin() + 1; i != fh_copy.end(); ++i) {
		if(find(selected.begin(), selected.end(), *i) == selected.end())
			continue;
		const string& group = (*i)->getGroup();
		for(FavoriteHubEntryList::iterator j = i - 1; ; --j) {
			if((*j)->getGroup() == group) {
				swap(*i, *j);
				break;
			}
			if(j == fh_copy.begin())
				break;
		}
	}
	if(!up)
		reverse(fh_copy.begin(), fh_copy.end());
	fh = fh_copy;
	FavoriteManager::getInstance()->save();

	refresh();
}

void FavHubsFrame::handleRemove() {
	if(hubs->hasSelected() && (!BOOLSETTING(CONFIRM_HUB_REMOVAL) || dwt::MessageBox(this).show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == dwt::MessageBox::RETBOX_YES)) {
		int i;
		while((i = hubs->getNext(-1, LVNI_SELECTED)) != -1)
			FavoriteManager::getInstance()->removeFavorite(reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(i)));
	}
}

void FavHubsFrame::handleGroups() {
	FavHubGroupsDlg(this).run();

	HoldRedraw hold(hubs);
	SelectionKeeper keeper(hubs);
	refresh();
}

void FavHubsFrame::handleDoubleClick() {
	if(hubs->hasSelected()) {
		openSelected();
	} else {
		handleAdd();
	}
}

bool FavHubsFrame::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAdd();
		return true;
	case VK_DELETE:
		handleRemove();
		return true;
	case VK_RETURN:
		openSelected();
		return true;
	}
	return false;
}

bool FavHubsFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = hubs->getContextMenuPos();
	}

	const size_t selected = hubs->countSelected();

	MenuPtr menu = addChild(WinUtil::Seeds::menu);
	menu->appendItem(T_("&Connect"), std::tr1::bind(&FavHubsFrame::openSelected, this), dwt::IconPtr(), selected, true);
	menu->appendSeparator();
	menu->appendItem(T_("&New..."), std::tr1::bind(&FavHubsFrame::handleAdd, this));
	menu->appendItem(T_("&Properties"), std::tr1::bind(&FavHubsFrame::handleProperties, this), dwt::IconPtr(), selected == 1);
	menu->appendItem(T_("Move &Up"), std::tr1::bind(&FavHubsFrame::handleMove, this, true), dwt::IconPtr(), selected);
	menu->appendItem(T_("Move &Down"), std::tr1::bind(&FavHubsFrame::handleMove, this, false), dwt::IconPtr(), selected);
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), std::tr1::bind(&FavHubsFrame::handleRemove, this), dwt::IconPtr(), selected);
	menu->appendSeparator();
	menu->appendItem(T_("Manage &groups"), std::tr1::bind(&FavHubsFrame::handleGroups, this));

	menu->open(pt);
	return true;
}

void FavHubsFrame::fillList() {
	// sort groups
	set<tstring, noCaseStringLess> sorted_groups;
	const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
	for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i)
		sorted_groups.insert(Text::toT(i->first));

	TStringList groups(sorted_groups.begin(), sorted_groups.end());
	groups.insert(groups.begin(), Util::emptyStringT); // default group (otherwise, hubs without group don't show up)
	hubs->setGroups(groups);
	bool grouped = hubs->isGrouped();

	const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();
	for(FavoriteHubEntryList::const_iterator i = fl.begin(), iend = fl.end(); i != iend; ++i) {
		const FavoriteHubEntryPtr& entry = *i;
		const string& group = entry->getGroup();

		int index;
		if(grouped) {
			index = 0;
			if(!group.empty()) {
				TStringIterC groupI = find(groups.begin() + 1, groups.end(), Text::toT(group));
				if(groupI != groups.end())
					index = groupI - groups.begin();
			}
		} else
			index = -1;

		TStringList l;
		l.push_back(Text::toT(entry->getName()));
		l.push_back(Text::toT(entry->getDescription()));
		l.push_back(Text::toT(entry->getNick(false)));
		l.push_back(tstring(entry->getPassword().size(), '*'));
		l.push_back(Text::toT(entry->getServer()));
		l.push_back(Text::toT(entry->getUserDescription()));
		l.push_back(Text::toT(group));

		hubs->insert(l, reinterpret_cast<LPARAM>(entry), index);
	}
}

void FavHubsFrame::refresh() {
	hubs->clear();
	fillList();
}

void FavHubsFrame::openSelected() {
	if(!hubs->hasSelected())
		return;

	if(!WinUtil::checkNick())
		return;

	std::vector<unsigned> items = hubs->getSelection();
	for(std::vector<unsigned>::const_iterator i = items.begin(), iend = items.end(); i != iend; ++i) {
		HubFrame::openWindow(getParent(), reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(*i))->getServer());
	}
}

void FavHubsFrame::on(FavoriteAdded, const FavoriteHubEntryPtr e) throw() {
	{
		HoldRedraw hold(hubs);
		SelectionKeeper keeper(hubs);
		refresh();
	}
	hubs->ensureVisible(hubs->findData(reinterpret_cast<LPARAM>(e)));
}

void FavHubsFrame::on(FavoriteRemoved, const FavoriteHubEntryPtr e) throw() {
	hubs->erase(hubs->findData(reinterpret_cast<LPARAM>(e)));
}
