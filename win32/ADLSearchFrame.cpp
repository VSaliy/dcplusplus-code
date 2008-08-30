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

#include "resource.h"

#include "ADLSearchFrame.h"

#include <dcpp/Client.h>
#include <dcpp/version.h>
#include "HoldRedraw.h"
#include "ADLSProperties.h"

static const ColumnInfo itemsColumns[] = {
	{ N_("Enabled / Search String"), 120, false },
	{ N_("Source Type"), 90, false },
	{ N_("Destination Directory"), 90, false },
	{ N_("Min Size"), 100, true },
	{ N_("Max Size"), 100, true }
};

ADLSearchFrame::ADLSearchFrame(dwt::TabView* mdiParent) :
	BaseType(mdiParent, T_("Automatic Directory Listing Search"), IDH_ADL_SEARCH, IDR_ADLSEARCH),
	add(0),
	properties(0),
	up(0),
	down(0),
	remove(0),
	help(0)
{
	{
		Table::Seed cs = WinUtil::Seeds::Table;
		cs.lvStyle |= LVS_EX_CHECKBOXES;
		items = addChild(cs);
		addWidget(items);

		WinUtil::makeColumns(items, itemsColumns, COLUMN_LAST, SETTING(ADLSEARCHFRAME_ORDER), SETTING(ADLSEARCHFRAME_WIDTHS));

		items->onDblClicked(std::tr1::bind(&ADLSearchFrame::handleDoubleClick, this));
		items->onKeyDown(std::tr1::bind(&ADLSearchFrame::handleKeyDown, this, _1));
		items->onRaw(std::tr1::bind(&ADLSearchFrame::handleItemChanged, this, _2), dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));
		items->onContextMenu(std::tr1::bind(&ADLSearchFrame::handleContextMenu, this, _1));
	}

	{
		Button::Seed cs = WinUtil::Seeds::button;

		cs.caption = T_("&New...");
		add = addChild(cs);
		add->setHelpId(IDH_ADLS_NEW);
		add->onClicked(std::tr1::bind(&ADLSearchFrame::handleAdd, this));
		addWidget(add);

		cs.caption = T_("&Properties");
		properties = addChild(cs);
		properties->setHelpId(IDH_ADLS_PROPERTIES);
		properties->onClicked(std::tr1::bind(&ADLSearchFrame::handleProperties, this));
		addWidget(properties);

		cs.caption = T_("Move &Up");
		up = addChild(cs);
		up->setHelpId(IDH_ADLS_MOVE_UP);
		up->onClicked(std::tr1::bind(&ADLSearchFrame::handleUp, this));
		addWidget(up);

		cs.caption = T_("Move &Down");
		down = addChild(cs);
		down->setHelpId(IDH_ADLS_MOVE_DOWN);
		down->onClicked(std::tr1::bind(&ADLSearchFrame::handleDown, this));
		addWidget(down);

		cs.caption = T_("&Remove");
		remove = addChild(cs);
		remove->setHelpId(IDH_ADLS_REMOVE);
		remove->onClicked(std::tr1::bind(&ADLSearchFrame::handleRemove, this));
		addWidget(remove);

		cs.caption = T_("&Help");
		help = addChild(cs);
		help->setHelpId(IDH_DCPP_HELP);
		help->onClicked(std::tr1::bind(&WinUtil::help, handle(), IDH_ADL_SEARCH));
		addWidget(help);
	}

	initStatus();

	layout();

	// Load all searches
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	for(ADLSearchManager::SearchCollection::iterator i = collection.begin(); i != collection.end(); ++i)
		addEntry(*i, /*itemCount*/ -1, /*scroll*/ false);
}

ADLSearchFrame::~ADLSearchFrame() {
}

void ADLSearchFrame::layout() {
	dwt::Rectangle r(dwt::Point(0, 0), getClientAreaSize());

	layoutStatus(r);

	/// @todo dynamic width
	const int ybutton = add->getTextSize(_T("A")).y + 10;
	const int xbutton = 90;
	const int xborder = 10;

	dwt::Rectangle rb(r.getBottom(ybutton));
	r.size.y -= ybutton;
	items->setBounds(r);

	rb.size.x = xbutton;
	add->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	properties->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	up->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	down->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	remove->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	help->setBounds(rb);
}

bool ADLSearchFrame::preClosing() {
	ADLSearchManager::getInstance()->Save();
	return true;
}

void ADLSearchFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::ADLSEARCHFRAME_ORDER, WinUtil::toString(items->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::ADLSEARCHFRAME_WIDTHS, WinUtil::toString(items->getColumnWidths()));
}

void ADLSearchFrame::handleAdd() {
	ADLSearch search;
	ADLSProperties dlg(this, &search);
	if(dlg.run() == IDOK) {
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

		int index;

		// Add new search to the end or if selected, just before
		if(items->countSelected() == 1) {
			index = items->getSelected();
			collection.insert(collection.begin() + index, search);
		} else {
			index = -1;
			collection.push_back(search);
		}

		addEntry(search, index);
	}
}

void ADLSearchFrame::handleProperties() {
	// Get selection info
	std::vector<unsigned> selected = items->getSelection();
	for(std::vector<unsigned>::const_iterator i = selected.begin(); i != selected.end(); ++i) {
		// Edit existing
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		ADLSearch search = collection[*i];

		// Invoke dialog with selected search
		ADLSProperties dlg(this, &search);
		if(dlg.run() == IDOK) {
			// Update search collection
			collection[*i] = search;

			// Update list control
			HoldRedraw hold(items);
			items->erase(*i);
			addEntry(search, *i);
			items->select(*i);
		}
	}
}

void ADLSearchFrame::handleUp() {
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	HoldRedraw hold(items);
	std::vector<unsigned> selected = items->getSelection();
	for(std::vector<unsigned>::const_iterator i = selected.begin(); i != selected.end(); ++i) {
		if(*i > 0) {
			ADLSearch search = collection[*i];
			swap(collection[*i], collection[*i - 1]);
			items->erase(*i);
			addEntry(search, *i - 1);
			items->select(*i - 1);
		}
	}
}

void ADLSearchFrame::handleDown() {
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	HoldRedraw hold(items);
	std::vector<unsigned> selected = items->getSelection();
	for(std::vector<unsigned>::reverse_iterator i = selected.rbegin(); i != selected.rend(); ++i) {
		if(*i < items->size() - 1) {
			ADLSearch search = collection[*i];
			swap(collection[*i], collection[*i + 1]);
			items->erase(*i);
			addEntry(search, *i + 1);
			items->select(*i + 1);
		}
	}
}

void ADLSearchFrame::handleRemove() {
	if(!BOOLSETTING(CONFIRM_ADLS_REMOVAL) || createMessageBox().show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_YESNO, MessageBox::BOX_ICONQUESTION) == IDYES) {
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		int i;
		while((i = items->getNext(-1, LVNI_SELECTED)) != -1) {
			collection.erase(collection.begin() + i);
			items->erase(i);
		}
	}
}

void ADLSearchFrame::handleDoubleClick() {
	if(items->hasSelected()) {
		handleProperties();
	} else {
		handleAdd();
	}
}

bool ADLSearchFrame::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAdd();
		return true;
	case VK_DELETE:
		handleRemove();
		return true;
	case VK_RETURN:
		handleProperties();
		return true;
	}
	return false;
}

LRESULT ADLSearchFrame::handleItemChanged(LPARAM lParam) {
	LPNMITEMACTIVATE item = reinterpret_cast<LPNMITEMACTIVATE>(lParam);

	if((item->uChanged & LVIF_STATE) == 0)
		return 0;
	if((item->uOldState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;
	if((item->uNewState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;

	if(item->iItem >= 0)
	{
		// Set new active status check box
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		ADLSearch& search = collection[item->iItem];
		search.isActive = items->isChecked(item->iItem);
	}
	return 0;
}

bool ADLSearchFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = items->getContextMenuPos();
	}

	bool hasSelected = items->hasSelected();

	MenuPtr contextMenu = addChild(WinUtil::Seeds::menu);
	contextMenu->appendItem(T_("&New..."), std::tr1::bind(&ADLSearchFrame::handleAdd, this));
	contextMenu->appendItem(T_("&Properties"), std::tr1::bind(&ADLSearchFrame::handleProperties, this), dwt::IconPtr(), hasSelected);
	contextMenu->appendItem(T_("&Remove"), std::tr1::bind(&ADLSearchFrame::handleRemove, this), dwt::IconPtr(), hasSelected);

	contextMenu->open(pt);
	return true;
}

void ADLSearchFrame::addEntry(ADLSearch& search, int index, bool scroll) {
	TStringList l;
	l.push_back(Text::toT(search.searchString));
	l.push_back(Text::toT(search.SourceTypeToString(search.sourceType)));
	l.push_back(Text::toT(search.destDir));
	l.push_back((search.minFileSize >= 0) ? Text::toT(Util::toString(search.minFileSize)) + _T(" ") + Text::toT(search.SizeTypeToString(search.typeFileSize)) : Util::emptyStringT);
	l.push_back((search.maxFileSize >= 0) ? Text::toT(Util::toString(search.maxFileSize)) + _T(" ") + Text::toT(search.SizeTypeToString(search.typeFileSize)) : Util::emptyStringT);
	int itemCount = items->insert(l, 0, index);
	if(index == -1)
		index = itemCount;
	items->setChecked(index, search.isActive);
	if (scroll)
		items->ensureVisible(index);
}
