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

#include <map>

#include "Dialog.h"

using std::map;
using std::string;

class Plugin
{
public:
	static Bool DCAPI main(PluginState state, DCCorePtr core, dcptr_t);

	static void dlgClosed();

private:
	Plugin();
	~Plugin();

	void addHooks();
	void clearHooks();

	void start();
	void close();

	void onLoad(DCCorePtr core, bool install, Bool& loadRes);
	void onSwitched();
	Bool onHubDataIn(HubDataPtr hHub, const char* message);
	Bool onHubDataOut(HubDataPtr hHub, const char* message);
	Bool onConnectionDataIn(ConnectionDataPtr hConn, const char* message);
	Bool onConnectionDataOut(ConnectionDataPtr hConn, const char* message);
	Bool onChatCommand(HubDataPtr hub, CommandDataPtr cmd);

	map<string, subsHandle> events;

	DCHooksPtr hooks;
	DCHubPtr hubs;
	DCUIPtr ui;

	Dialog dialog;

	/** @todo switch to dcpp::Singleton when <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=51494>
	is fixed */
	static Plugin* instance;
};

#endif
