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

#ifndef DCPLUSPLUS_WIN32_FAVHUBGROUPSDLG_H
#define DCPLUSPLUS_WIN32_FAVHUBGROUPSDLG_H

#include <dcpp/FavHubGroup.h>

#include "TypedTable.h"

class FavHubGroupsDlg : public dwt::ModalDialog
{
public:
	/// @param parentEntry_ entry currently being edited, to make sure we don't delete it
	FavHubGroupsDlg(dwt::Widget* parent, FavoriteHubEntry* parentEntry_ = 0);
	virtual ~FavHubGroupsDlg();

	int run();

private:
	enum {
		COLUMN_NAME,
		COLUMN_PRIVATE,
		COLUMN_CONNECT,
		COLUMN_LAST
	};

	class GroupInfo : public FastAlloc<GroupInfo> {
	public:
		GroupInfo(const FavHubGroup& group_);

		const tstring& getText(int col) const;
		int getImage() const;

		static int compareItems(const GroupInfo* a, const GroupInfo* b, int col);

		const FavHubGroup group;

	private:
		tstring columns[COLUMN_LAST];
	};

	struct GroupCollector {
		void operator()(const GroupInfo* group) {
			groups.insert(group->group);
		}
		FavHubGroups groups;
	};

	GridPtr grid;

	typedef TypedTable<const GroupInfo> Groups;
	Groups* groups;

	ButtonPtr update;
	ButtonPtr remove;
	GroupBoxPtr properties;
	TextBoxPtr edit;
	CheckBoxPtr priv_box;
	CheckBoxPtr connect_box;

	FavoriteHubEntry* parentEntry;

	bool handleInitDialog();
	bool handleKeyDown(int c);
	void handleSelectionChanged();
	void handleUpdate();
	void handleRemove();
	void handleAdd();
	void handleClose();

	void add(const FavHubGroup& group, bool ensureVisible = true);
	void add(const tstring& name, bool priv, bool connect);
	bool addable(const tstring& name, int ignore = -1);
	void removeGroup(const GroupInfo* group);

	void layout();
};

#endif
