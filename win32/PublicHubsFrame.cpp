/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

#include "PublicHubsFrame.h"

#include "resource.h"
#include "HubFrame.h"
#include "HoldRedraw.h"
#include "HubListsDlg.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>

static const ColumnInfo hubsColumns[] = {
	{ N_("Name"), 200, false },
	{ N_("Description"), 290, false },
	{ N_("Users"), 50, true },
	{ N_("Address"), 100, false },
	{ N_("Country"), 100, false },
	{ N_("Shared"), 100, true },
	{ N_("Min share"), 100, true },
	{ N_("Min slots"), 100, true },
	{ N_("Max hubs"), 100, true },
	{ N_("Max users"), 100, true },
	{ N_("Reliability"), 100, false },
	{ N_("Rating"), 100, true }
};

PublicHubsFrame::HubInfo::HubInfo(const HubEntry* entry_) : entry(entry_) {
	columns[COLUMN_NAME] = Text::toT(entry->getName());
	columns[COLUMN_DESCRIPTION] = Text::toT(entry->getDescription());
	columns[COLUMN_USERS] = Text::toT(Util::toString(entry->getUsers()));
	columns[COLUMN_SERVER] = Text::toT(entry->getServer());
	columns[COLUMN_COUNTRY] = Text::toT(entry->getCountry());
	columns[COLUMN_SHARED] = Text::toT(Util::formatBytes(entry->getShared()));
	columns[COLUMN_MINSHARE] = Text::toT(Util::formatBytes(entry->getMinShare()));
	columns[COLUMN_MINSLOTS] = Text::toT(Util::toString(entry->getMinSlots()));
	columns[COLUMN_MAXHUBS] = Text::toT(Util::toString(entry->getMaxHubs()));
	columns[COLUMN_MAXUSERS] = Text::toT(Util::toString(entry->getMaxUsers()));
	columns[COLUMN_RELIABILITY] = Text::toT(Util::toString(entry->getReliability()));
	columns[COLUMN_RATING] = Text::toT(entry->getRating());
}

int PublicHubsFrame::HubInfo::compareItems(const HubInfo* a, const HubInfo* b, int col) {
	switch(col) {
	case COLUMN_USERS: return compare(a->entry->getUsers(), b->entry->getUsers());
	case COLUMN_MINSLOTS: return compare(a->entry->getMinSlots(), b->entry->getMinSlots());
	case COLUMN_MAXHUBS: return compare(a->entry->getMaxHubs(), b->entry->getMaxHubs());
	case COLUMN_MAXUSERS: return compare(a->entry->getMaxUsers(), b->entry->getMaxUsers());
	case COLUMN_RELIABILITY: return compare(a->entry->getReliability(), b->entry->getReliability());
	case COLUMN_SHARED: return compare(a->entry->getShared(), b->entry->getShared());
	case COLUMN_MINSHARE: return compare(a->entry->getMinShare(), b->entry->getMinShare());
	case COLUMN_RATING: return compare(a->entry->getRating(), b->entry->getRating());
	default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
	}
}

PublicHubsFrame::PublicHubsFrame(dwt::TabView* mdiParent) :
BaseType(mdiParent, T_("Public Hubs"), IDH_PUBLIC_HUBS, IDR_PUBLICHUBS),
grid(0),
hubs(0),
filter(0),
filterSel(0),
lists(0),
visibleHubs(0),
users(0)
{
	grid = addChild(Grid::Seed(2, 3));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		WidgetHubs::Seed cs;
		cs.style |= LVS_SINGLESEL;
		hubs = grid->addChild(cs);
		grid->setWidget(hubs, 0, 0, 1, 3);
		addWidget(hubs);

		WinUtil::makeColumns(hubs, hubsColumns, COLUMN_LAST, SETTING(PUBLICHUBSFRAME_ORDER), SETTING(PUBLICHUBSFRAME_WIDTHS));
		hubs->setSort(COLUMN_USERS, false);

		hubs->onDblClicked(std::tr1::bind(&PublicHubsFrame::openSelected, this));
		hubs->onKeyDown(std::tr1::bind(&PublicHubsFrame::handleKeyDown, this, _1));
		hubs->onContextMenu(std::tr1::bind(&PublicHubsFrame::handleContextMenu, this, _1));
	}

	{
		GroupBox::Seed gs = WinUtil::Seeds::group;

		gs.caption = T_("F&ilter");
		GroupBoxPtr group = grid->addChild(gs);
		group->setHelpId(IDH_PUBLIC_HUBS_FILTER);

		GridPtr cur = group->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;

		TextBox::Seed ts = WinUtil::Seeds::textBox;
		ts.style |= ES_AUTOHSCROLL;
		filter = cur->addChild(ts);
		addWidget(filter);
		filter->onKeyDown(std::tr1::bind(&PublicHubsFrame::handleFilterKeyDown, this, _1));

		filterSel = cur->addChild(WinUtil::Seeds::comboBoxStatic);
		addWidget(filterSel);

		gs.caption = T_("Configured Public Hub Lists");
		group = grid->addChild(gs);
		group->setHelpId(IDH_PUBLIC_HUBS_LISTS);

		cur = group->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;

		lists = cur->addChild(WinUtil::Seeds::comboBoxStatic);
		addWidget(lists);
		lists->onSelectionChanged(std::tr1::bind(&PublicHubsFrame::handleListSelChanged, this));

		Button::Seed bs = WinUtil::Seeds::button;
		bs.caption = T_("&Configure");
		ButtonPtr button = cur->addChild(bs);
		addWidget(button);
		button->onClicked(std::tr1::bind(&PublicHubsFrame::handleConfigure, this));
	}

	{
		Button::Seed cs = WinUtil::Seeds::button;
		cs.caption = T_("&Refresh");
		ButtonPtr button = grid->addChild(cs);
		button->setHelpId(IDH_PUBLIC_HUBS_REFRESH);
		addWidget(button);
		button->onClicked(std::tr1::bind(&PublicHubsFrame::handleRefresh, this));
	}

	initStatus();

	status->setHelpId(STATUS_STATUS, IDH_PUBLIC_HUBS_STATUS);
	status->setHelpId(STATUS_HUBS, IDH_PUBLIC_HUBS_HUBS);
	status->setHelpId(STATUS_USERS, IDH_PUBLIC_HUBS_USERS);

	//populate the filter list with the column names
	for(int j=0; j<COLUMN_LAST; j++) {
		filterSel->addValue(T_(hubsColumns[j].name));
	}
	filterSel->addValue(T_("Any"));
	filterSel->setSelected(COLUMN_LAST);
	filterSel->onSelectionChanged(std::tr1::bind(&PublicHubsFrame::updateList, this));

	FavoriteManager::getInstance()->addListener(this);

	entries	 = FavoriteManager::getInstance()->getPublicHubs();

	// populate with values from the settings
	updateDropDown();
	updateList();

	layout();

	if(FavoriteManager::getInstance()->isDownloading()) {
		status->setText(STATUS_STATUS, T_("Downloading public hub list..."));
	} else if(entries.empty()) {
		FavoriteManager::getInstance()->refresh();
	}
}

PublicHubsFrame::~PublicHubsFrame() {
}

bool PublicHubsFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);

	return true;
}

void PublicHubsFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::PUBLICHUBSFRAME_ORDER, WinUtil::toString(hubs->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::PUBLICHUBSFRAME_WIDTHS, WinUtil::toString(hubs->getColumnWidths()));
}

void PublicHubsFrame::layout() {
	dwt::Rectangle r(getClientAreaSize());

	status->layout(r);

	grid->layout(r);
}

void PublicHubsFrame::updateStatus() {
	if (entries.size() > visibleHubs)
		status->setText(STATUS_HUBS, str(TF_("Hubs: %1% / %2%") % visibleHubs % entries.size()));
	else
		status->setText(STATUS_HUBS, str(TF_("Hubs: %1%") % visibleHubs));
	status->setText(STATUS_USERS, str(TF_("Users: %1%") % users));
}

void PublicHubsFrame::updateDropDown() {
	lists->clear();
	StringList l(FavoriteManager::getInstance()->getHubLists());
	for(StringIterC idx = l.begin(); idx != l.end(); ++idx) {
		lists->addValue(Text::toT(*idx).c_str());
	}
	lists->setSelected((FavoriteManager::getInstance()->getSelectedHubList()) % l.size());
}

void PublicHubsFrame::updateList() {
	dcdebug("PublicHubsFrame::updateList\n");

	hubs->clear();
	users = 0;
	visibleHubs = 0;

	HoldRedraw hold(hubs);

	double size = -1;
	FilterModes mode = NONE;

	int sel = filterSel->getSelected();

	bool doSizeCompare = parseFilter(mode, size);

	for(HubEntryList::const_iterator i = entries.begin(); i != entries.end(); ++i) {
		if(matchFilter(*i, sel, doSizeCompare, mode, size)) {
			hubs->insert(hubs->size(), new HubInfo(&(*i)));
			visibleHubs++;
			users += i->getUsers();
		}
	}

	hubs->resort();

	updateStatus();
}

bool PublicHubsFrame::parseFilter(FilterModes& mode, double& size) {
	string::size_type start = (string::size_type)string::npos;
	string::size_type end = (string::size_type)string::npos;
	int64_t multiplier = 1;

	if(filterString.compare(0, 2, ">=") == 0) {
		mode = GREATER_EQUAL;
		start = 2;
	} else if(filterString.compare(0, 2, "<=") == 0) {
		mode = LESS_EQUAL;
		start = 2;
	} else if(filterString.compare(0, 2, "==") == 0) {
		mode = EQUAL;
		start = 2;
	} else if(filterString.compare(0, 2, "!=") == 0) {
		mode = NOT_EQUAL;
		start = 2;
	} else if(filterString[0] == _T('<')) {
		mode = LESS;
		start = 1;
	} else if(filterString[0] == _T('>')) {
		mode = GREATER;
		start = 1;
	} else if(filterString[0] == _T('=')) {
		mode = EQUAL;
		start = 1;
	}

	if(start == string::npos)
		return false;
	if(filterString.length() <= start)
		return false;

	if((end = Util::findSubString(filterString, "TiB")) != tstring::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(filterString, "GiB")) != tstring::npos) {
		multiplier = 1024*1024*1024;
	} else if((end = Util::findSubString(filterString, "MiB")) != tstring::npos) {
		multiplier = 1024*1024;
	} else if((end = Util::findSubString(filterString, "KiB")) != tstring::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(filterString, "TB")) != tstring::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(filterString, "GB")) != tstring::npos) {
		multiplier = 1000*1000*1000;
	} else if((end = Util::findSubString(filterString, "MB")) != tstring::npos) {
		multiplier = 1000*1000;
	} else if((end = Util::findSubString(filterString, "kB")) != tstring::npos) {
		multiplier = 1000;
	} else if((end = Util::findSubString(filterString, "B")) != tstring::npos) {
		multiplier = 1;
	}

	if(end == string::npos) {
		end = filterString.length();
	}

	string tmpSize = filterString.substr(start, end-start);
	size = Util::toDouble(tmpSize) * multiplier;

	return true;
}

bool PublicHubsFrame::matchFilter(const HubEntry& entry, const int& sel, bool doSizeCompare, const FilterModes& mode, const double& size) {
	if(filterString.empty())
		return true;

	double entrySize = 0;
	string entryString = "";

	switch(sel) {
		case COLUMN_NAME: entryString = entry.getName(); doSizeCompare = false; break;
		case COLUMN_DESCRIPTION: entryString = entry.getDescription(); doSizeCompare = false; break;
		case COLUMN_USERS: entrySize = entry.getUsers(); break;
		case COLUMN_SERVER: entryString = entry.getServer(); doSizeCompare = false; break;
		case COLUMN_COUNTRY: entryString = entry.getCountry(); doSizeCompare = false; break;
		case COLUMN_SHARED: entrySize = (double)entry.getShared(); break;
		case COLUMN_MINSHARE: entrySize = (double)entry.getMinShare(); break;
		case COLUMN_MINSLOTS: entrySize = entry.getMinSlots(); break;
		case COLUMN_MAXHUBS: entrySize = entry.getMaxHubs(); break;
		case COLUMN_MAXUSERS: entrySize = entry.getMaxUsers(); break;
		case COLUMN_RELIABILITY: entrySize = entry.getReliability(); break;
		case COLUMN_RATING: entryString = entry.getRating(); doSizeCompare = false; break;
		default: break;
	}

	bool insert = false;
	if(doSizeCompare) {
		switch(mode) {
			case EQUAL: insert = (size == entrySize); break;
			case GREATER_EQUAL: insert = (size <= entrySize); break;
			case LESS_EQUAL: insert = (size >= entrySize); break;
			case GREATER: insert = (size < entrySize); break;
			case LESS: insert = (size > entrySize); break;
			case NOT_EQUAL: insert = (size != entrySize); break;
			case NONE: ; break;
		}
	} else {
		if(sel >= COLUMN_LAST) {
			if( Util::findSubString(entry.getName(), filterString) != string::npos ||
				Util::findSubString(entry.getDescription(), filterString) != string::npos ||
				Util::findSubString(entry.getServer(), filterString) != string::npos ||
				Util::findSubString(entry.getCountry(), filterString) != string::npos ||
				Util::findSubString(entry.getRating(), filterString) != string::npos ) {
					insert = true;
				}
		}
		if(Util::findSubString(entryString, filterString) != string::npos)
			insert = true;
	}

	return insert;
}

bool PublicHubsFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(hubs->hasSelected()) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = hubs->getContextMenuPos();
		}

		MenuPtr menu = addChild(WinUtil::Seeds::menu);
		menu->appendItem(T_("&Connect"), std::tr1::bind(&PublicHubsFrame::handleConnect, this), dwt::IconPtr(), true, true);
		menu->appendItem(T_("Add To &Favorites"), std::tr1::bind(&PublicHubsFrame::handleAdd, this), dwt::IconPtr(new dwt::Icon(IDR_FAVORITE_HUBS)));
		menu->appendItem(T_("Copy &address to clipboard"), std::tr1::bind(&PublicHubsFrame::handleCopyHub, this));

		menu->open(pt);
		return true;
	}
	return false;
}

void PublicHubsFrame::handleRefresh() {
	status->setText(STATUS_STATUS, T_("Downloading public hub list..."));
	FavoriteManager::getInstance()->refresh(true);
	updateDropDown();
}

void PublicHubsFrame::handleConfigure() {
	HubListsDlg dlg(this);
	if(dlg.run() == IDOK) {
		updateDropDown();
	}
}

void PublicHubsFrame::handleConnect() {
	if(!checkNick())
		return;

	if(hubs->hasSelected() == 1) {
		HubFrame::openWindow(getParent(), hubs->getSelectedData()->entry->getServer());
	}
}

void PublicHubsFrame::handleAdd() {
	if(!checkNick())
		return;

	if(hubs->hasSelected()) {
		FavoriteManager::getInstance()->addFavorite(*hubs->getSelectedData()->entry);
	}
}

void PublicHubsFrame::handleCopyHub() {
	if(hubs->hasSelected()) {
		WinUtil::setClipboard(Text::toT(hubs->getSelectedData()->entry->getServer()));
	}
}

bool PublicHubsFrame::checkNick() {
	if(SETTING(NICK).empty()) {
		createMessageBox().show(T_("Please enter a nickname in the settings dialog!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
		return false;
	}
	return true;
}


void PublicHubsFrame::openSelected() {
	if(!checkNick())
		return;

	if(hubs->hasSelected()) {
		HubFrame::openWindow(getParent(), hubs->getSelectedData()->entry->getServer());
	}
}

bool PublicHubsFrame::handleKeyDown(int c) {
	if(c == VK_RETURN) {
		openSelected();
	}

	return false;
}

void PublicHubsFrame::handleListSelChanged() {
	FavoriteManager::getInstance()->setHubList(lists->getSelected());
	entries = FavoriteManager::getInstance()->getPublicHubs();
	updateList();
}

bool PublicHubsFrame::handleFilterKeyDown(int c) {
	if(c == VK_RETURN) {
		filterString = Text::fromT(filter->getText());
		updateList();
		return true;
	}
	return false;
}

void PublicHubsFrame::onFinished(const tstring& s) {
	entries = FavoriteManager::getInstance()->getPublicHubs();
	updateList();
	status->setText(STATUS_STATUS, s);
}

void PublicHubsFrame::on(DownloadStarting, const string& l) throw() {
	callAsync(std::tr1::bind(&dwt::StatusBar::setText, status, STATUS_STATUS, str(TF_("Downloading public hub list... (%1%)") % Text::toT(l)), false));
}

void PublicHubsFrame::on(DownloadFailed, const string& l) throw() {
	callAsync(std::tr1::bind(&dwt::StatusBar::setText, status, STATUS_STATUS, str(TF_("Download failed: %1%") % Text::toT(l)), false));
}

void PublicHubsFrame::on(DownloadFinished, const string& l) throw() {
	callAsync(std::tr1::bind(&PublicHubsFrame::onFinished, this, str(TF_("Hub list downloaded... (%1%)") % Text::toT(l))));
}

void PublicHubsFrame::on(LoadedFromCache, const string& l) throw() {
	callAsync(std::tr1::bind(&PublicHubsFrame::onFinished, this, T_("Hub list loaded from cache...")));
}
