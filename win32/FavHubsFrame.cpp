/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include <set>

#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/MessageBox.h>

#include "HubFrame.h"
#include "FavHubProperties.h"
#include "FavHubGroupsDlg.h"

using std::for_each;
using std::set;
using std::swap;

using dwt::Grid;
using dwt::GridInfo;

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

FavHubsFrame::FavHubsFrame(TabViewPtr parent) :
BaseType(parent, T_("Favorite Hubs"), IDH_FAVORITE_HUBS, IDI_FAVORITE_HUBS),
grid(0),
hubs(0)
{
	grid = addChild(Grid::Seed(2, 8));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;
	grid->column(3).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;
	grid->column(5).mode = GridInfo::FILL;
	grid->column(6).mode = GridInfo::FILL;
	grid->column(7).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		Table::Seed cs = WinUtil::Seeds::table;
		cs.style |= LVS_NOSORTHEADER;
		hubs = grid->addChild(cs);
		grid->setWidget(hubs, 0, 0, 1, 8);
		addWidget(hubs);

		WinUtil::makeColumns(hubs, hubsColumns, COLUMN_LAST, SETTING(FAVHUBSFRAME_ORDER), SETTING(FAVHUBSFRAME_WIDTHS));

		hubs->onDblClicked([this] { handleDoubleClick(); });
		hubs->onKeyDown([this](int c) { return handleKeyDown(c); });
		hubs->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleContextMenu(sc); });
	}

	{
		ButtonPtr button;
		Button::Seed cs = WinUtil::Seeds::button;
		cs.style |= WS_DISABLED;

		cs.caption = T_("&Connect");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_CONNECT);
		button->onClicked([this] { openSelected(); });
		addWidget(button);
		hubs->onSelectionChanged([this, button] { button->setEnabled(hubs->hasSelected()); });

		cs.caption = T_("&New...");
		cs.style &= ~WS_DISABLED;
		button = grid->addChild(cs);
		cs.style |= WS_DISABLED;
		button->setHelpId(IDH_FAVORITE_HUBS_NEW);
		button->onClicked([this] { handleAdd(); });
		addWidget(button);

		cs.caption = T_("&Properties");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_PROPERTIES);
		button->onClicked([this] { handleProperties(); });
		addWidget(button);
		hubs->onSelectionChanged([this, button] { button->setEnabled(hubs->countSelected() == 1); });

		cs.caption = T_("Move &Up");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_MOVE_UP);
		button->onClicked([this] { handleMove(true); });
		addWidget(button);
		hubs->onSelectionChanged([this, button] { button->setEnabled(hubs->hasSelected()); });

		cs.caption = T_("Move &Down");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_MOVE_DOWN);
		button->onClicked([this] { handleMove(false); });
		addWidget(button);
		hubs->onSelectionChanged([this, button] { button->setEnabled(hubs->hasSelected()); });

		cs.caption = T_("&Remove");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_REMOVE);
		button->onClicked([this] { handleRemove(); });
		addWidget(button);
		hubs->onSelectionChanged([this, button] { button->setEnabled(hubs->hasSelected()); });

		cs.caption = T_("&Move to group");
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_GROUP);
		button->onClicked([this] { handleGroup(); });
		addWidget(button);
		hubs->onSelectionChanged([this, button] { button->setEnabled(hubs->hasSelected()); });

		cs.caption = T_("Manage &groups");
		cs.style &= ~WS_DISABLED;
		button = grid->addChild(cs);
		button->setHelpId(IDH_FAVORITE_HUBS_MANAGE_GROUPS);
		button->onClicked([this] { handleGroups(); });
		addWidget(button);
	}

	initStatus();

	layout();

	fillList();

	FavoriteManager::getInstance()->addListener(this);
}

FavHubsFrame::~FavHubsFrame() {

}

void FavHubsFrame::layout() {
	dwt::Rectangle r(getClientSize());

	// TODO status->layout(r);

	grid->resize(r);
}

bool FavHubsFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);
	return true;
}

void FavHubsFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::FAVHUBSFRAME_ORDER, WinUtil::toString(hubs->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::FAVHUBSFRAME_WIDTHS, WinUtil::toString(hubs->getColumnWidths()));
}

FavHubsFrame::StateKeeper::StateKeeper(TablePtr hubs_, bool ensureVisible_) :
hubs(hubs_),
ensureVisible(ensureVisible_)
{
	hubs->sendMessage(WM_SETREDRAW, FALSE);

	// in grouped mode, the indexes of each item are completely random, so use entry pointers instead
	std::vector<unsigned> selection = hubs->getSelection();
	for(auto i = selection.begin(), iend = selection.end(); i != iend; ++i)
		selected.push_back(reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(*i)));

	scroll = hubs->getScrollInfo(SB_VERT, SIF_POS).nPos;
}

FavHubsFrame::StateKeeper::~StateKeeper() {
	// restore visual updating now, otherwise it doesn't always scroll
	hubs->sendMessage(WM_SETREDRAW, TRUE);

	hubs->scroll(0, scroll);

	for(auto i = selected.begin(), iend = selected.end(); i != iend; ++i) {
		int pos = hubs->findData(reinterpret_cast<LPARAM>(*i));
		hubs->select(pos);
		if(ensureVisible)
			hubs->ensureVisible(pos);
	}
}

const FavoriteHubEntryList& FavHubsFrame::StateKeeper::getSelection() const {
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
			StateKeeper keeper(hubs);
			refresh();
		}
	}
}

void FavHubsFrame::handleMove(bool up) {
	FavoriteHubEntryList& fh = FavoriteManager::getInstance()->getFavoriteHubs();
	if(fh.size() <= 1)
		return;

	StateKeeper keeper(hubs);
	const FavoriteHubEntryList& selected = keeper.getSelection();

	FavoriteHubEntryList fh_copy = fh;
	if(!up)
		reverse(fh_copy.begin(), fh_copy.end());
	FavoriteHubEntryList moved;
	for(auto i = fh_copy.begin(); i != fh_copy.end(); ++i) {
		if(find(selected.begin(), selected.end(), *i) == selected.end())
			continue;
		if(find(moved.begin(), moved.end(), *i) != moved.end())
			continue;
		const string& group = (*i)->getGroup();
		for(auto j = i; ;) {
			if(j == fh_copy.begin()) {
				// couldn't move within the same group; change group.
				TStringList groups(getSortedGroups());
				if(!up)
					reverse(groups.begin(), groups.end());
				auto ig = find(groups.begin(), groups.end(), Text::toT(group));
				if(ig != groups.begin()) {
					FavoriteHubEntryPtr f = *i;
					f->setGroup(Text::fromT(*(ig - 1)));
					i = fh_copy.erase(i);
					fh_copy.push_back(f);
					moved.push_back(f);
				}
				break;
			}
			if((*--j)->getGroup() == group) {
				swap(*i, *j);
				break;
			}
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

void FavHubsFrame::handleGroup() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);
	fillGroupMenu(menu);
	menu->open(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos())));
}

void FavHubsFrame::handleGroup(const string& group) {
	StateKeeper keeper(hubs);
	const FavoriteHubEntryList& selected = keeper.getSelection();
	for_each(selected.cbegin(), selected.cend(), [group](FavoriteHubEntryPtr entry) { entry->setGroup(group); });
	FavoriteManager::getInstance()->save();
	refresh();
}

void FavHubsFrame::handleGroups() {
	FavHubGroupsDlg(this).run();

	StateKeeper keeper(hubs);
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

	size_t sel = hubs->countSelected();

	MenuPtr menu = addChild(WinUtil::Seeds::menu);
	menu->setTitle((sel == 0) ? getText() : (sel == 1) ? escapeMenu(hubs->getText(hubs->getSelected(), COLUMN_NAME)) :
		str(TF_("%1% hubs") % sel), getParent()->getIcon(this));
	menu->appendItem(T_("&Connect"), [this] { openSelected(); }, dwt::IconPtr(), sel, true);
	menu->appendSeparator();
	menu->appendItem(T_("&New..."), [this] { handleAdd(); });
	menu->appendItem(T_("&Properties"), [this] { handleProperties(); }, dwt::IconPtr(), sel == 1);
	menu->appendItem(T_("Move &Up"), [this] { handleMove(true); }, dwt::IconPtr(), sel);
	menu->appendItem(T_("Move &Down"), [this] { handleMove(false); }, dwt::IconPtr(), sel);
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), [this] { handleRemove(); }, dwt::IconPtr(), sel);
	menu->appendSeparator();
	if(sel) {
		fillGroupMenu(menu->appendPopup(T_("&Move to group")));
	} else {
		menu->appendItem(T_("&Move to group"), 0, dwt::IconPtr(), false);
	}
	menu->appendItem(T_("Manage &groups"), [this] { handleGroups(); });

	menu->open(pt);
	return true;
}

TStringList FavHubsFrame::getSortedGroups() const {
	set<tstring, noCaseStringLess> sorted_groups;
	const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
	for(auto i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i)
		sorted_groups.insert(Text::toT(i->first));

	TStringList groups(sorted_groups.begin(), sorted_groups.end());
	groups.insert(groups.begin(), Util::emptyStringT); // default group (otherwise, hubs without group don't show up)
	return groups;
}

void FavHubsFrame::fillGroupMenu(MenuPtr menu) {
	TStringList groups(getSortedGroups());
	for(auto i = groups.cbegin(), iend = groups.cend(); i != iend; ++i) {
		const tstring& group = i->empty() ? T_("Default group") : *i;
		menu->appendItem(group, [this, group] { handleGroup(Text::fromT(group)); });
	}
}

void FavHubsFrame::fillList() {
	TStringList groups(getSortedGroups());
	hubs->setGroups(groups);
	bool grouped = hubs->isGrouped();

	const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();
	for(auto i = fl.begin(), iend = fl.end(); i != iend; ++i) {
		const FavoriteHubEntryPtr& entry = *i;
		const string& group = entry->getGroup();

		int index;
		if(grouped) {
			index = 0;
			if(!group.empty()) {
				auto groupI = find(groups.begin() + 1, groups.end(), Text::toT(group));
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
	for(auto i = items.begin(), iend = items.end(); i != iend; ++i) {
		HubFrame::openWindow(getParent(), reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(*i))->getServer());
	}
}

void FavHubsFrame::on(FavoriteAdded, const FavoriteHubEntryPtr e) noexcept {
	{
		StateKeeper keeper(hubs, false);
		refresh();
	}
	hubs->ensureVisible(hubs->findData(reinterpret_cast<LPARAM>(e)));
}

void FavHubsFrame::on(FavoriteRemoved, const FavoriteHubEntryPtr e) noexcept {
	hubs->erase(hubs->findData(reinterpret_cast<LPARAM>(e)));
}
