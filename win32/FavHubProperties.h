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

#ifndef DCPLUSPLUS_WIN32_FAV_HUB_PROPERTIES_H
#define DCPLUSPLUS_WIN32_FAV_HUB_PROPERTIES_H

#include <dcpp/forward.h>

#include "GridDialog.h"

class FavHubProperties : public GridDialog
{
public:
	FavHubProperties(dwt::Widget* parent, FavoriteHubEntry *_entry);
	virtual ~FavHubProperties();

private:
	TextBoxPtr name;
	TextBoxPtr address;
	TextBoxPtr hubDescription;
	TextBoxPtr nick;
	TextBoxPtr password;
	TextBoxPtr description;
	TextBoxPtr email;
	TextBoxPtr userIp;
	ComboBoxPtr showJoins;
	ComboBoxPtr favShowJoins;
	ComboBoxPtr logMainChat;
	ComboBoxPtr groups;

	FavoriteHubEntry *entry;

	bool handleInitDialog();
	void handleGroups();
	void handleOKClicked();

	void fillGroups();
};

#endif // !defined(DCPLUSPLUS_WIN32_FAV_HUB_PROPERTIES_H)
