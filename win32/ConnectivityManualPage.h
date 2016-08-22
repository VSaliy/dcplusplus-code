/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

class ConnectivityManualPage;

class PageContent
{

public:

	PageContent(ConnectivityManualPage* parent, dwt::GridPtr grid);

	void read();

	void write();
	
	void onSettingsChange();

protected:

	void Initialize();
	
protected:

	SettingsManager::IntSetting settingIncomingConnections;
	SettingsManager::StrSetting settingExternalIP;
	SettingsManager::BoolSetting settingNoIPOverride;
	SettingsManager::IntSetting settingTCPPort;
	SettingsManager::IntSetting settingTLSPort;
	SettingsManager::IntSetting settingUDPPort;
	SettingsManager::StrSetting settingBindAddress;
	SettingsManager::StrSetting settingMapper;

	bool isV6;
	
private:

	void InitializeUI();

	void onTransferPortUpdated();
	void onTLSTransferPortUpdated();
	void validatePort(TextBoxPtr sourceBox, TextBoxPtr otherBox, const tstring& source, const tstring& other);

private:

	RadioButtonPtr active;
	RadioButtonPtr upnp;
	RadioButtonPtr passive;
	RadioButtonPtr inactive;

	ComboBoxPtr mapper;
	ComboBoxPtr bind;

	TextBoxPtr transferBox;
	TextBoxPtr tlstransferBox;

	PropPage::ItemList items;

	ConnectivityManualPage* parent;
	dwt::GridPtr grid;
	
};

class PageContentV4 : public PageContent
{
public:
	PageContentV4(ConnectivityManualPage* parent, dwt::GridPtr grid);
};

class PageContentV6 : public PageContent
{
public:
	PageContentV6(ConnectivityManualPage* parent, dwt::GridPtr grid);
};

class ConnectivityManualPage : public PropPage, private ConnectivityManagerListener
{
public:

	ConnectivityManualPage(dwt::Widget* parent);
	virtual ~ConnectivityManualPage();

public:

	void read(const PropPage::ItemList& items);
	void write(const PropPage::ItemList& items);

	void read();
	void write();

private:

	GroupBoxPtr autoGroup;
	CheckBoxPtr autoDetect;
	void handleAutoClicked();
	void updateAuto();

	// ConnectivityManagerListener
	void on(SettingChanged) noexcept;

	PageContent* v4Content;
	dwt::GridPtr v4Grid;

	PageContent* v6Content;
	dwt::GridPtr v6Grid;
};

#endif // !defined(DCPLUSPLUS_WIN32_CONNECTIVITY_MANUAL_PAGE_H)
