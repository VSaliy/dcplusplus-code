/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_PROXY_PAGE_H
#define DCPLUSPLUS_WIN32_PROXY_PAGE_H

#include <dcpp/ConnectivityManager.h>

#include "PropPage.h"

class ProxyPage : public PropPage, private ConnectivityManagerListener
{
public:
	ProxyPage(dwt::Widget* parent);
	virtual ~ProxyPage();

	virtual void write();

private:
	ItemList items;

	GroupBoxPtr autoGroup;
	CheckBoxPtr autoDetect;

	RadioButtonPtr directOut;
	RadioButtonPtr socks5;

	GroupBoxPtr socksSettings;
	CheckBoxPtr socksResolve;

	void handleAutoClicked();

	void read();
	void updateAuto();
	void fixControlsOut();

	// ConnectivityManagerListener
	void on(SettingChanged) noexcept;
};

#endif // !defined(DCPLUSPLUS_WIN32_PROXY_PAGE_H)
