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

#ifndef DCPLUSPLUS_WIN32_FAVHUBGROUPSDLG_H
#define DCPLUSPLUS_WIN32_FAVHUBGROUPSDLG_H

#include <dcpp/forward.h>

class FavHubGroupsDlg : public dwt::ModalDialog
{
public:
	/// @param parentEntry_ entry currently being edited, to make sure we don't delete it
	FavHubGroupsDlg(dwt::Widget* parent, FavoriteHubEntry* parentEntry_ = 0);
	virtual ~FavHubGroupsDlg();

	int run();

private:
	GridPtr grid;
	TablePtr groups;
	ButtonPtr update;
	ButtonPtr remove;
	GroupBoxPtr properties;
	TextBoxPtr edit;
	CheckBoxPtr priv_box;

	FavoriteHubEntry* parentEntry;

	bool handleInitDialog();
	bool handleKeyDown(int c);
	void handleSelectionChanged();
	void handleUpdate();
	void handleRemove();
	void handleAdd();
	void handleClose();

	void add(const tstring& name, bool priv);
	bool isPrivate(size_t row) const;
	bool addable(const tstring& name, int ignore = -1);

	void layout();
};

#endif
