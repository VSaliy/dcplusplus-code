/*
 * Copyright (C) 2001-2015 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_CONNECTIVITY_MANUAL_PAGE_H
#define DCPLUSPLUS_WIN32_CONNECTIVITY_MANUAL_PAGE_H

#include <dcpp/ConnectivityManager.h>

#include "PropPage.h"

class ConnectivityManualPage : public PropPage, private ConnectivityManagerListener
{
public:
	ConnectivityManualPage(dwt::Widget* parent);
	virtual ~ConnectivityManualPage();

	void write();

private:
	ItemList items;

	GroupBoxPtr autoGroup;
	CheckBoxPtr autoDetect;

	RadioButtonPtr active;
	RadioButtonPtr upnp;
	RadioButtonPtr passive;

	ComboBoxPtr mapper;

	TextBoxPtr transferBox;
	TextBoxPtr tlstransferBox;

	void handleAutoClicked();

	void read();
	void updateAuto();

	void onTransferPortUpdated();
	void onTLSTransferPortUpdated();

	void validatePort(TextBoxPtr sourceBox, TextBoxPtr otherBox, const tstring& source, const tstring& other);

	// ConnectivityManagerListener
	void on(SettingChanged) noexcept;
};

#endif // !defined(DCPLUSPLUS_WIN32_CONNECTIVITY_MANUAL_PAGE_H)
