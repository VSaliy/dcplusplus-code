/*
 * Copyright (C) 2012 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef PLUGINS_DEV_PLUGIN_H
#define PLUGINS_DEV_PLUGIN_H

#include <Singleton.h>

#include <map>

#include "Dialog.h"

using std::map;
using std::string;

class Plugin : public dcpp::Singleton<Plugin>
{
public:
	static Bool DCAPI main(PluginState state, DCCorePtr core, dcptr_t);

private:
	friend class dcpp::Singleton<Plugin>;

	Plugin();
	~Plugin();

	void onLoad(DCCorePtr core, bool install, Bool& loadRes);
	Bool onHubDisconnected(HubDataPtr hHub);
	Bool onHubConnected(HubDataPtr hHub);
	Bool onHubDataIn(HubDataPtr hHub, const char* message);
	Bool onHubDataOut(HubDataPtr hHub, const char* message, Bool* bBreak);
	Bool onConnectionDataIn(ConnectionDataPtr hConn, const char* message);
	Bool onConnectionDataOut(ConnectionDataPtr hConn, const char* message);
	Bool onUiCreated(HWND hwnd);

	// Event wrappers
	static Bool DCAPI hubOfflineEvent(dcptr_t pObject, dcptr_t /*pData*/, dcptr_t, Bool* /*bBreak*/) { return getInstance()->onHubDisconnected(reinterpret_cast<HubDataPtr>(pObject)); }
	static Bool DCAPI hubOnlineEvent(dcptr_t pObject, dcptr_t /*pData*/, dcptr_t, Bool* /*bBreak*/) { return getInstance()->onHubConnected(reinterpret_cast<HubDataPtr>(pObject)); }
	static Bool DCAPI netHubInEvent(dcptr_t pObject, dcptr_t pData, dcptr_t, Bool* /*bBreak*/) { return getInstance()->onHubDataIn(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData)); }
	static Bool DCAPI netHubOutEvent(dcptr_t pObject, dcptr_t pData, dcptr_t, Bool* bBreak) { return getInstance()->onHubDataOut(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); }
	static Bool DCAPI netConnInEvent(dcptr_t pObject, dcptr_t pData, dcptr_t, Bool* /*bBreak*/) { return getInstance()->onConnectionDataIn(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData)); }
	static Bool DCAPI netConnOutEvent(dcptr_t pObject, dcptr_t pData, dcptr_t, Bool* /*bBreak*/) { return getInstance()->onConnectionDataOut(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData)); }
	static Bool DCAPI uiCreatedEvent(dcptr_t pObject, dcptr_t /*pData*/, dcptr_t, Bool* /*bBreak*/) { return getInstance()->onUiCreated(reinterpret_cast<HWND>(pObject)); }

	map<string, subsHandle> events;

	DCCorePtr dcpp;
	DCHooksPtr hooks;

	Dialog dialog;

	static Plugin* instance;
};

#endif
