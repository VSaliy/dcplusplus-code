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

#include "stdafx.h"
#include "Plugin.h"
#include "Util.h"

Plugin* Plugin::instance = nullptr;

Plugin::Plugin() {
}

Plugin::~Plugin() {
	for(auto& i: events)
		hooks->release_hook(i.second);
	events.clear();
}

Bool DCAPI Plugin::main(PluginState state, DCCorePtr core, dcptr_t) {
	switch(state) {
	case ON_INSTALL:
	case ON_LOAD:
		{
			Bool res = True;
			newInstance();
			getInstance()->onLoad(core, (state == ON_INSTALL), res);
			return res;
		}

	case ON_UNINSTALL:
	case ON_UNLOAD:
		{
			deleteInstance();
			return True;
		}

	default:
		{
			return False;
		}
	}
}

void Plugin::onLoad(DCCorePtr core, bool install, Bool& loadRes) {
	dcpp = core;
	hooks = reinterpret_cast<DCHooksPtr>(core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER));

	auto utils = reinterpret_cast<DCUtilsPtr>(core->query_interface(DCINTF_DCPP_UTILS, DCINTF_DCPP_UTILS_VER));
	auto config = reinterpret_cast<DCConfigPtr>(core->query_interface(DCINTF_CONFIG, DCINTF_CONFIG_VER));
	auto logger = reinterpret_cast<DCLogPtr>(core->query_interface(DCINTF_LOGGING, DCINTF_LOGGING_VER));

	if(!utils || !config || !logger) {
		loadRes = False;
		return;
	}

	Util::initialize(utils, config, logger);

	/*if(install) {
		// Default settings

		Util::logMessage("Dev plugin installed, please restart the client to begin using the plugin.");
		return;
	}*/

	events[HOOK_HUB_OFFLINE]			= hooks->bind_hook(HOOK_HUB_OFFLINE, &hubOfflineEvent, NULL);
	events[HOOK_HUB_ONLINE]				= hooks->bind_hook(HOOK_HUB_ONLINE, &hubOnlineEvent, NULL);

	events[HOOK_NETWORK_HUB_IN]			= hooks->bind_hook(HOOK_NETWORK_HUB_IN, &netHubInEvent, NULL);
	events[HOOK_NETWORK_HUB_OUT]		= hooks->bind_hook(HOOK_NETWORK_HUB_OUT, &netHubOutEvent, NULL);
	events[HOOK_NETWORK_CONN_IN]		= hooks->bind_hook(HOOK_NETWORK_CONN_IN, &netConnInEvent, NULL);
	events[HOOK_NETWORK_CONN_OUT]		= hooks->bind_hook(HOOK_NETWORK_CONN_OUT, &netConnOutEvent, NULL);

	events[HOOK_UI_CREATED]				= hooks->bind_hook(HOOK_UI_CREATED, &uiCreatedEvent, NULL);
}

Bool Plugin::onHubDisconnected(HubDataPtr hHub) {
	return False;
}

Bool Plugin::onHubConnected(HubDataPtr hHub) {
	return False;
}

Bool Plugin::onHubDataIn(HubDataPtr hHub, const char* message) {
	dialog.write(true, false, hHub->ip, "Hub " + string(hHub->url), message);
	return False;
}

Bool Plugin::onHubDataOut(HubDataPtr hHub, const char* message, Bool* bBreak) {
	dialog.write(true, true, hHub->ip, "Hub " + string(hHub->url), message);
	return False;
}

Bool Plugin::onConnectionDataIn(ConnectionDataPtr hConn, const char* message) {
	dialog.write(false, false, hConn->ip, "User " + string(reinterpret_cast<UserDataPtr>(hConn->object)->nick), message);
	return False;
}

Bool Plugin::onConnectionDataOut(ConnectionDataPtr hConn, const char* message) {
	dialog.write(false, true, hConn->ip, "User " + string(reinterpret_cast<UserDataPtr>(hConn->object)->nick), message);
	return False;
}

Bool Plugin::onUiCreated(HWND hwnd) {
	dialog.create(hwnd);
	return False;
}
