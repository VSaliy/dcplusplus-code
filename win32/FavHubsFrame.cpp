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

#include "FavHubsFrame.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>
#include "HoldRedraw.h"
#include "HubFrame.h"
#include "FavHubProperties.h"

static const ColumnInfo hubsColumns[] = {
	{ N_("Auto connect / Name"), 200, false },
	{ N_("Description"), 290, false },
	{ N_("Nick"), 125, false },
	{ N_("Password"), 100, false },
	{ N_("Server"), 100, false },
	{ N_("User Description"), 125, false }
};

FavHubsFrame::FavHubsFrame() :
	BaseType(T_("Favorite Hubs"), IDH_FAVORITE_HUBS, IDR_FAVORITES),
	hubs(0),
	connect(0),
	add(0),
	properties(0),
	up(0),
	down(0),
	remove(0),
	nosave(false)
{
	{
		Table::Seed cs = WinUtil::Seeds::Table;
		cs.style |= LVS_NOSORTHEADER;
		cs.lvStyle |= LVS_EX_CHECKBOXES;
		hubs = addChild(cs);
		addWidget(hubs);

		WinUtil::makeColumns(hubs, hubsColumns, COLUMN_LAST, SETTING(FAVHUBSFRAME_ORDER), SETTING(FAVHUBSFRAME_WIDTHS));

		hubs->onDblClicked(std::tr1::bind(&FavHubsFrame::handleDoubleClick, this));
		hubs->onKeyDown(std::tr1::bind(&FavHubsFrame::handleKeyDown, this, _1));
		hubs->onRaw(std::tr1::bind(&FavHubsFrame::handleItemChanged, this, _1, _2), dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));
		hubs->onContextMenu(std::tr1::bind(&FavHubsFrame::handleContextMenu, this, _1));
	}

	{
		Button::Seed cs = WinUtil::Seeds::button;

		cs.caption = T_("&Connect");
		connect = addChild(cs);
		connect->setHelpId(IDH_FAVORITE_HUBS_CONNECT);
		connect->onClicked(std::tr1::bind(&FavHubsFrame::openSelected, this));
		addWidget(connect);

		cs.caption = T_("&New...");
		add = addChild(cs);
		add->setHelpId(IDH_FAVORITE_HUBS_NEW);
		add->onClicked(std::tr1::bind(&FavHubsFrame::handleAdd, this));
		addWidget(add);

		cs.caption = T_("&Properties");
		properties = addChild(cs);
		properties->setHelpId(IDH_FAVORITE_HUBS_PROPERTIES);
		properties->onClicked(std::tr1::bind(&FavHubsFrame::handleProperties, this));
		addWidget(properties);

		cs.caption = T_("Move &Up");
		up = addChild(cs);
		up->setHelpId(IDH_FAVORITE_HUBS_MOVE_UP);
		up->onClicked(std::tr1::bind(&FavHubsFrame::handleUp, this));
		addWidget(up);

		cs.caption = T_("Move &Down");
		down = addChild(cs);
		down->setHelpId(IDH_FAVORITE_HUBS_MOVE_DOWN);
		down->onClicked(std::tr1::bind(&FavHubsFrame::handleDown, this));
		addWidget(down);

		cs.caption = T_("&Remove");
		remove = addChild(cs);
		remove->setHelpId(IDH_FAVORITE_HUBS_REMOVE);
		remove->onClicked(std::tr1::bind(&FavHubsFrame::handleRemove, this));
		addWidget(remove);
	}

	initStatus();

	layout();

	const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();
	for(FavoriteHubEntryList::const_iterator i = fl.begin(); i != fl.end(); ++i)
		addEntry(*i, /*itemCount*/ -1, /*scroll*/ false);

	FavoriteManager::getInstance()->addListener(this);
}

FavHubsFrame::~FavHubsFrame() {

}

void FavHubsFrame::layout() {
	dwt::Rectangle r(getClientAreaSize());

	layoutStatus(r);

	/// @todo dynamic width
	const int ybutton = add->getTextSize(_T("A")).y + 10;
	const int xbutton = 90;
	const int xborder = 10;

	dwt::Rectangle rb(r.getBottom(ybutton));
	r.size.y -= ybutton;
	hubs->setBounds(r);

	rb.size.x = xbutton;
	connect->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	add->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	properties->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	up->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	down->setBounds(rb);

	rb.pos.x += xbutton + xborder;
	remove->setBounds(rb);
}

bool FavHubsFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);
	return true;
}

void FavHubsFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::FAVHUBSFRAME_ORDER, WinUtil::toString(hubs->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::FAVHUBSFRAME_WIDTHS, WinUtil::toString(hubs->getColumnWidths()));
}

void FavHubsFrame::handleAdd() {
	FavoriteHubEntry e;

	while(true) {
		FavHubProperties dlg(this, &e);
		if(dlg.run() == IDOK) {
			if(FavoriteManager::getInstance()->isFavoriteHub(e.getServer())) {
				createMessageBox().show(T_("Hub already exists as a favorite"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONEXCLAMATION);
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
		int i = hubs->getSelected();
		FavoriteHubEntryPtr e = reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(i));
		dcassert(e != NULL);
		FavHubProperties dlg(this, e);
		if(dlg.run() == IDOK) {
			hubs->setText(i, COLUMN_NAME, Text::toT(e->getName()));
			hubs->setText(i, COLUMN_DESCRIPTION, Text::toT(e->getDescription()));
			hubs->setText(i, COLUMN_SERVER, Text::toT(e->getServer()));
			hubs->setText(i, COLUMN_NICK, Text::toT(e->getNick(false)));
			hubs->setText(i, COLUMN_PASSWORD, tstring(e->getPassword().size(), '*'));
			hubs->setText(i, COLUMN_USERDESCRIPTION, Text::toT(e->getUserDescription()));
		}
	}
}

void FavHubsFrame::handleUp() {
	nosave = true;
	FavoriteHubEntryList& fh = FavoriteManager::getInstance()->getFavoriteHubs();
	HoldRedraw hold(hubs);
	std::vector<unsigned> selected = hubs->getSelection();
	for(std::vector<unsigned>::const_iterator i = selected.begin(); i != selected.end(); ++i) {
		if(*i > 0) {
			FavoriteHubEntryPtr e = fh[*i];
			swap(fh[*i], fh[*i - 1]);
			hubs->erase(*i);
			addEntry(e, *i - 1);
			hubs->select(*i - 1);
		}
	}
	FavoriteManager::getInstance()->save();
	nosave = false;
}

void FavHubsFrame::handleDown() {
	nosave = true;
	FavoriteHubEntryList& fh = FavoriteManager::getInstance()->getFavoriteHubs();
	HoldRedraw hold(hubs);
	std::vector<unsigned> selected = hubs->getSelection();
	for(std::vector<unsigned>::reverse_iterator i = selected.rbegin(); i != selected.rend(); ++i) {
		if(*i < hubs->size() - 1) {
			FavoriteHubEntryPtr e = fh[*i];
			swap(fh[*i], fh[*i + 1]);
			hubs->erase(*i);
			addEntry(e, *i + 1);
			hubs->select(*i + 1);
		}
	}
	FavoriteManager::getInstance()->save();
	nosave = false;
}

void FavHubsFrame::handleRemove() {
	if(hubs->hasSelected() && (!BOOLSETTING(CONFIRM_HUB_REMOVAL) || createMessageBox().show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_YESNO, MessageBox::BOX_ICONQUESTION) == MessageBox::RETBOX_YES)) {
		int i;
		while((i = hubs->getNext(-1, LVNI_SELECTED)) != -1)
			FavoriteManager::getInstance()->removeFavorite(reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(i)));
	}
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

LRESULT FavHubsFrame::handleItemChanged(WPARAM /*wParam*/, LPARAM lParam) {
	LPNMITEMACTIVATE l = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
	if(!nosave && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		FavoriteHubEntryPtr f = reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(l->iItem));
		f->setConnect(hubs->isChecked(l->iItem));
		FavoriteManager::getInstance()->save();
	}
	return 0;
}

bool FavHubsFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = hubs->getContextMenuPos();
	}

	bool hasSelected = hubs->hasSelected();

	MenuPtr menu = addChild(WinUtil::Seeds::menu);
	menu->appendItem(T_("&Connect"), std::tr1::bind(&FavHubsFrame::openSelected, this), dwt::BitmapPtr(), hasSelected, true);
	menu->appendSeparator();
	menu->appendItem(T_("&New..."), std::tr1::bind(&FavHubsFrame::handleAdd, this));
	menu->appendItem(T_("&Properties"), std::tr1::bind(&FavHubsFrame::handleProperties, this), dwt::BitmapPtr(), hasSelected);
	menu->appendItem(T_("Move &Up"), std::tr1::bind(&FavHubsFrame::handleUp, this), dwt::BitmapPtr(), hasSelected);
	menu->appendItem(T_("Move &Down"), std::tr1::bind(&FavHubsFrame::handleDown, this), dwt::BitmapPtr(), hasSelected);
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), std::tr1::bind(&FavHubsFrame::handleRemove, this), dwt::BitmapPtr(), hasSelected);

	menu->open(pt);
	return true;
}

void FavHubsFrame::addEntry(const FavoriteHubEntryPtr entry, int index, bool scroll) {
	TStringList l;
	l.push_back(Text::toT(entry->getName()));
	l.push_back(Text::toT(entry->getDescription()));
	l.push_back(Text::toT(entry->getNick(false)));
	l.push_back(tstring(entry->getPassword().size(), '*'));
	l.push_back(Text::toT(entry->getServer()));
	l.push_back(Text::toT(entry->getUserDescription()));
	bool b = entry->getConnect();
	int itemCount = hubs->insert(l, reinterpret_cast<LPARAM>(entry), index);
	if(index == -1)
		index = itemCount;
	hubs->setChecked(index, b);
	if (scroll)
		hubs->ensureVisible(index);
}

void FavHubsFrame::openSelected() {
	if(!hubs->hasSelected())
		return;

	if(SETTING(NICK).empty()) {
		createMessageBox().show(T_("Please enter a nickname in the settings dialog!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONSTOP);
		return;
	}

	std::vector<unsigned> items = hubs->getSelection();
	for(std::vector<unsigned>::iterator i = items.begin(); i != items.end(); ++i) {
		FavoriteHubEntryPtr entry = reinterpret_cast<FavoriteHubEntryPtr>(hubs->getData(*i));
		HubFrame::openWindow(entry->getServer());
	}
}

void FavHubsFrame::on(FavoriteAdded, const FavoriteHubEntryPtr e) throw() {
	addEntry(e);
}

void FavHubsFrame::on(FavoriteRemoved, const FavoriteHubEntryPtr e) throw() {
	hubs->erase(hubs->findData(reinterpret_cast<LPARAM>(e)));
}
