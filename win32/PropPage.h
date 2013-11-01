/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#include <unordered_map>
#include <dcpp/typedefs.h>
#include <dwt/widgets/Container.h>

#include "forward.h"

using std::unordered_map;

class PropPage : public dwt::Container
{
public:
	PropPage(dwt::Widget* parent, int rows, int cols);
	virtual ~PropPage();

	virtual void layout();
	virtual void write() { }

	enum Type {
		T_STR,
		T_INT,
		T_INT_WITH_SPIN, // fill even when the current value is the same as the default value (for controls with a spin buddy)
		T_INT64,
		T_INT64_WITH_SPIN, // fill even when the current value is the same as the default value (for controls with a spin buddy)
		T_BOOL
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

protected:
	void read(const ItemList& items);
	void read(const ListItem* listItems, TablePtr list);

	void write(const ItemList& items);
	void write(TablePtr list);

	void handleBrowseDir(TextBoxPtr box, int setting);
	void handleBrowseFile(TextBoxPtr box, int setting);

	GridPtr grid;

private:
	virtual dwt::Point getPreferredSize();

	void handleListHelpId(TablePtr list, unsigned& id);

	unordered_map<TablePtr, const ListItem*> lists;
};

#endif // !defined(PROP_PAGE_H)
