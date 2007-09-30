/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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
#include <dcpp/ResourceManager.h>
#include <dcpp/version.h>

int PublicHubsFrame::columnIndexes[] = {
	COLUMN_NAME,
	COLUMN_DESCRIPTION,
	COLUMN_USERS,
	COLUMN_SERVER,
	COLUMN_COUNTRY,
	COLUMN_SHARED,
	COLUMN_MINSHARE,
	COLUMN_MINSLOTS,
	COLUMN_MAXHUBS,
	COLUMN_MAXUSERS,
	COLUMN_RELIABILITY,
	COLUMN_RATING
 };

int PublicHubsFrame::columnSizes[] = { 200, 290, 50, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

static ResourceManager::Strings columnNames[] = {
	ResourceManager::HUB_NAME,
	ResourceManager::DESCRIPTION,
	ResourceManager::USERS,
	ResourceManager::HUB_ADDRESS,
	ResourceManager::COUNTRY,
	ResourceManager::SHARED,
	ResourceManager::MIN_SHARE,
	ResourceManager::MIN_SLOTS,
	ResourceManager::MAX_HUBS,
	ResourceManager::MAX_USERS,
	ResourceManager::RELIABILITY,
	ResourceManager::RATING,

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

PublicHubsFrame::PublicHubsFrame(SmartWin::WidgetMDIParent* mdiParent) :
	BaseType(mdiParent),
	hubs(0),
	configure(0),
	refresh(0),
	lists(0),
	filterDesc(0),
	filter(0),
	pubLists(0),
	filterSel(0),
	visibleHubs(0),
	users(0)
{
	{
		WidgetDataGrid::Seed cs;
		cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
		cs.exStyle = WS_EX_CLIENTEDGE;
		hubs = SmartWin::WidgetCreator<WidgetHubs>::create(this, cs);
		hubs->setListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);		
		hubs->setFont(WinUtil::font);
		addWidget(hubs);
		
		hubs->createColumns(ResourceManager::getInstance()->getStrings(columnNames));
		hubs->setColumnOrder(WinUtil::splitTokens(SETTING(FAVHUBSFRAME_ORDER), columnIndexes));
		hubs->setColumnWidths(WinUtil::splitTokens(SETTING(FAVHUBSFRAME_WIDTHS), columnSizes));
		hubs->setColor(WinUtil::textColor, WinUtil::bgColor);
		hubs->setSortColumn(COLUMN_USERS);
		
		hubs->onDblClicked(std::tr1::bind(&PublicHubsFrame::openSelected, this));
		hubs->onKeyDown(std::tr1::bind(&PublicHubsFrame::handleKeyDown, this, _1));
	}
	
	{
		WidgetButton::Seed cs;
		cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_PUSHBUTTON;
		
		cs.caption = TSTRING(CONFIGURE);
		configure = createButton(cs);
		configure->onClicked(std::tr1::bind(&PublicHubsFrame::handleConfigure, this));
		configure->setFont(WinUtil::font);
		addWidget(configure);
		
		cs.caption = TSTRING(REFRESH);
		refresh = createButton(cs);
		refresh->onClicked(std::tr1::bind(&PublicHubsFrame::handleRefresh, this));
		refresh->setFont(WinUtil::font);
		addWidget(refresh);

		cs.style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_GROUPBOX;
		cs.exStyle = WS_EX_TRANSPARENT;
		cs.caption = TSTRING(CONFIGURED_HUB_LISTS);
		lists = createButton(cs);
		lists->setFont(WinUtil::font);

		cs.caption = TSTRING(FILTER);
		filterDesc = createButton(cs);
		filterDesc->setFont(WinUtil::font);

	}

	{
		WidgetComboBox::Seed cs;
		cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST;
		cs.caption.clear();
		pubLists = createComboBox(cs);
		pubLists->setFont(WinUtil::font);
		pubLists->onSelectionChanged(std::tr1::bind(&PublicHubsFrame::handleListSelChanged, this));
		
		filterSel = createComboBox(cs);
		filterSel->setFont(WinUtil::font);

		//populate the filter list with the column names
		for(int j=0; j<COLUMN_LAST; j++) {
			filterSel->addValue(CTSTRING_I(columnNames[j]));
		}
		filterSel->addValue(CTSTRING(ANY));
		filterSel->setSelectedIndex(COLUMN_LAST);
	}
	{
		WidgetTextBox::Seed cs;
		cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
		filter = createTextBox(cs);
		filter->setFont(WinUtil::font);
		filter->onChar(std::tr1::bind(&PublicHubsFrame::handleFilterChar, this, _1));
	}
	
	initStatus();

	statusSizes[STATUS_DUMMY] = 16;

	FavoriteManager::getInstance()->addListener(this);
	
	// populate with values from the settings
	updateDropDown();
	updateList();

	layout();
	
	onSpeaker(std::tr1::bind(&PublicHubsFrame::handleSpeaker, this, _1, _2));
	onRaw(std::tr1::bind(&PublicHubsFrame::handleContextMenu, this, _1, _2), SmartWin::Message(WM_CONTEXTMENU));
	
	entries	 = FavoriteManager::getInstance()->getPublicHubs();
	if(FavoriteManager::getInstance()->isDownloading()) {
		setStatus(STATUS_STATUS, TSTRING(DOWNLOADING_HUB_LIST));
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
	const int border = 2;

	SmartWin::Rectangle r(getClientAreaSize()); 
	
	layoutStatus(r);

	int const comboH = 140;

	// listview
	int ymessage = filter->getTextSize(_T("A")).y + 10;

	r.size.y -= ymessage*2 + 8 + border * 2;
	hubs->setBounds(r);

	r.pos.y += r.size.y + border;
	r.size.y = ymessage * 2 + 8;
	
	// filter box
	r.size.x = (r.size.x - 100 - border * 2) / 2 ;
	filterDesc->setBounds(r);

	SmartWin::Rectangle rc = r;
	// filter edit
	rc.pos.y += ymessage - 4;
	rc.size.y = ymessage;
	rc.pos.x += 16;
	rc.size.x = rc.size.x * 2 / 3 - 24 - border;
	filter->setBounds(rc);
	
	//filter sel
	rc.size.y += comboH;
	
	rc.pos.x += rc.size.x + border;
	rc.size.x = (rc.size.x + 24 + border) / 2 - 8;
	filterSel->setBounds(rc);

	// lists box
	r.pos.x = r.size.x + border;
	lists->setBounds(r);

	rc = r;
	// lists dropdown
	rc.pos.y += ymessage - 4;
	rc.size.y = ymessage;
	
	rc.size.y += comboH;
	rc.pos.x += 16;
	rc.size.x -= 100 + 24 + border;
	pubLists->setBounds(rc);

	// configure button
	rc.size.y -= comboH;
	rc.pos.x += rc.size.x + border;
	rc.size.x = 100;
	configure->setBounds(rc);

	// refresh button
	rc.pos.x += rc.size.x + 8;
	refresh->setBounds(rc);
}

void PublicHubsFrame::updateStatus() {
	setStatus(STATUS_HUBS, Text::toT(STRING(HUBS) + ": " + Util::toString(visibleHubs)));
	setStatus(STATUS_USERS, Text::toT(STRING(USERS) + ": " + Util::toString(users)));
}

void PublicHubsFrame::updateDropDown() {
	pubLists->removeAllItems();
	StringList lists(FavoriteManager::getInstance()->getHubLists());
	for(StringList::iterator idx = lists.begin(); idx != lists.end(); ++idx) {
		pubLists->addValue(Text::toT(*idx).c_str());
	}
	pubLists->setSelectedIndex(FavoriteManager::getInstance()->getSelectedHubList());
}

void PublicHubsFrame::updateList() {
	hubs->forEachSelectedT(DeleteFunction());
	hubs->clear();
	users = 0;
	visibleHubs = 0;

	HoldRedraw hold(hubs);
	
	double size = -1;
	FilterModes mode = NONE;

	int sel = filterSel->getSelectedIndex();

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

HRESULT PublicHubsFrame::handleSpeaker(WPARAM wParam, LPARAM lParam) {
	if((wParam == FINISHED) || (wParam == LOADED_FROM_CACHE)) {
		std::auto_ptr<tstring> x(reinterpret_cast<tstring*>(lParam));
		entries = FavoriteManager::getInstance()->getPublicHubs();
		updateList();
		setStatus(STATUS_STATUS, ((wParam == LOADED_FROM_CACHE) ? TSTRING(HUB_LIST_LOADED_FROM_CACHE) : TSTRING(HUB_LIST_DOWNLOADED)) + _T(" (") + (*x) + _T(")"));
	} else if(wParam == STARTING) {
		std::auto_ptr<tstring> x(reinterpret_cast<tstring*>(lParam));
		setStatus(STATUS_STATUS, TSTRING(DOWNLOADING_HUB_LIST) + _T(" (") + (*x) + _T(")"));
	} else if(wParam == FAILED) {
		std::auto_ptr<tstring> x(reinterpret_cast<tstring*>(lParam));
		setStatus(STATUS_STATUS, TSTRING(DOWNLOAD_FAILED) + (*x));
	}
	return 0;
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

HRESULT PublicHubsFrame::handleContextMenu(WPARAM wParam, LPARAM lParam) {
	if(reinterpret_cast<HWND>(wParam) == hubs->handle() && hubs->hasSelection()) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			pt = hubs->getContextMenuPos();
		}

		WidgetMenuPtr menu = createMenu(true);
		menu->appendItem(IDC_CONNECT, TSTRING(CONNECT), std::tr1::bind(&PublicHubsFrame::handleConnect, this));
		menu->appendItem(IDC_ADD, TSTRING(ADD_TO_FAVORITES), std::tr1::bind(&PublicHubsFrame::handleAdd, this));
		menu->appendItem(IDC_COPY_HUB, TSTRING(COPY_HUB), std::tr1::bind(&PublicHubsFrame::handleCopyHub, this));
		menu->setDefaultItem(IDC_CONNECT);
		menu->trackPopupMenu(this, pt.x, pt.y, TPM_LEFTALIGN | TPM_RIGHTBUTTON);
		return TRUE;
	}
	return FALSE;
}

void PublicHubsFrame::handleRefresh() {
	setStatus(STATUS_STATUS, CTSTRING(DOWNLOADING_HUB_LIST));
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

	if(hubs->hasSelection() == 1) {
		HubFrame::openWindow(getParent(), hubs->getSelectedData()->entry->getServer());
	}
}

void PublicHubsFrame::handleAdd() {
	if(!checkNick())
		return;

	if(hubs->hasSelection()) {
		FavoriteManager::getInstance()->addFavorite(*hubs->getSelectedData()->entry);
	}	
}

void PublicHubsFrame::handleCopyHub() {
	if(hubs->hasSelection()) {
		WinUtil::setClipboard(Text::toT(hubs->getSelectedData()->entry->getServer()));
	}
}

bool PublicHubsFrame::checkNick() {
	if(SETTING(NICK).empty()) {
		createMessageBox().show(TSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
		return false;
	}
	return true;
}


void PublicHubsFrame::openSelected() {
	if(!checkNick())
		return;
	
	if(hubs->hasSelection()) {
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
	FavoriteManager::getInstance()->setHubList(pubLists->getSelectedIndex());
	entries = FavoriteManager::getInstance()->getPublicHubs();
	updateList();
}

bool PublicHubsFrame::handleFilterChar(int c) {
	if(c == VK_RETURN) {
		filterString = Text::fromT(filter->getText());
		updateList();
		return true;
	} 
	return false;
}
