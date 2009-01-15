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

#ifndef DCPLUSPLUS_WIN32_PROP_PAGE_H
#define DCPLUSPLUS_WIN32_PROP_PAGE_H

#define SETTINGS_BUF_LEN 1024

#include <dwt/aspects/AspectChild.h>

#include "WidgetFactory.h"

class PropPage : public WidgetFactory<dwt::ModelessDialog>
{
public:
	PropPage(dwt::Widget* parent);
	virtual ~PropPage();

	virtual void write() { }

	enum Type {
		T_STR,
		T_INT,
		T_INT_WITH_SPIN, // fill even when the current value is the same as the default value (for controls with a spin buddy)
		T_BOOL,
		T_END
	};

	struct Item {
		Item() : widget(0), setting(0), type(T_STR) { }
		Item(Widget* w, int s, Type t) : widget(w), setting(s), type(t) { }
		Widget* widget;
		int setting;
		Type type;
	};

	typedef std::vector<Item> ItemList;

	struct ListItem {
		int setting;
		const char* desc;
		unsigned helpId;
	};

	struct TextItem {
		WORD itemID;
		const char* stringToTranslate;
	};

protected:
	void read(const ItemList& items);
	void read(const ListItem* listItems, TablePtr list);
	void initList(TablePtr list);
	void write(const ItemList& items);
	void write(const ListItem* listItems, TablePtr list);
	void translate(HWND page, TextItem* items);

	void handleBrowseDir(const Item& i);

private:
	void handleListHelp(HWND hWnd, unsigned id, const ListItem* listItems, TablePtr list);
};

#endif // !defined(PROP_PAGE_H)
