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

#include <pluginsdk/Config.h>
#include <pluginsdk/Core.h>
#include <pluginsdk/Logger.h>
#include <pluginsdk/Util.h>

using dcapi::Config;
using dcapi::Core;
using dcapi::Logger;
using dcapi::Util;

Plugin* Plugin::instance = nullptr;

Plugin::Plugin() : commandName(PLUGIN_NAME ": enable") {
}

Plugin::~Plugin() {
	clearHooks();

	if(ui) {
		ui->remove_command(commandName.c_str());
	}
}

Bool DCAPI Plugin::main(PluginState state, DCCorePtr core, dcptr_t) {
	switch(state) {
	case ON_INSTALL:
	case ON_LOAD:
		{
			Bool res = True;
			instance = new Plugin();
			instance->onLoad(core, (state == ON_INSTALL), res);
			return res;
		}

	case ON_UNINSTALL:
	case ON_UNLOAD:
		{
			delete instance;
			instance = nullptr;
			return True;
		}

	default:
		{
			return False;
		}
	}
}

void Plugin::dlgClosed() {
	auto oldCommand = instance->commandName;
	instance->close();

	instance->ui->remove_command(oldCommand.c_str());
	instance->ui->add_command(instance->commandName.c_str(), [] { instance->onSwitched(); });
}

void Plugin::addHooks() {
	events[HOOK_NETWORK_HUB_IN] = hooks->bind_hook(HOOK_NETWORK_HUB_IN, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onHubDataIn(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);
	events[HOOK_NETWORK_HUB_OUT] = hooks->bind_hook(HOOK_NETWORK_HUB_OUT, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onHubDataOut(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);
	events[HOOK_NETWORK_CONN_IN] = hooks->bind_hook(HOOK_NETWORK_CONN_IN, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onConnectionDataIn(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);
	events[HOOK_NETWORK_CONN_OUT] = hooks->bind_hook(HOOK_NETWORK_CONN_OUT, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onConnectionDataOut(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);
	events[HOOK_UI_PROCESS_CHAT_CMD] = hooks->bind_hook(HOOK_UI_PROCESS_CHAT_CMD, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		auto cmd = reinterpret_cast<CommandDataPtr>(pData);
		if(cmd->isPrivate) { return False; }
		return instance->onChatCommand(reinterpret_cast<HubDataPtr>(pObject), cmd); }, nullptr);
}

void Plugin::clearHooks() {
	for(auto& i: events)
		hooks->release_hook(i.second);
	events.clear();
}

void Plugin::start() {
	dialog.create();
	addHooks();
	Config::setConfig("Enabled", true);
	commandName = PLUGIN_NAME ": disable";
}

void Plugin::close() {
	Config::setConfig("Enabled", false);
	clearHooks();
	dialog.close();
	commandName = PLUGIN_NAME ": enable";
}

void Plugin::onLoad(DCCorePtr core, bool install, Bool& loadRes) {
	hooks = reinterpret_cast<DCHooksPtr>(core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER));

	hubs = reinterpret_cast<DCHubPtr>(core->query_interface(DCINTF_DCPP_HUBS, DCINTF_DCPP_HUBS_VER));
	ui = reinterpret_cast<DCUIPtr>(core->query_interface(DCINTF_DCPP_UI, DCINTF_DCPP_UI_VER));

	if(!Util::init(core) || !Config::init(core) || !Logger::init(core) || !hooks || !hubs || !ui) {
		loadRes = False;
		return;
	}
	Core::init(core);

	if(install) {
		Config::setConfig("Enabled", true);

		Logger::log("The dev plugin has been installed; check the plugins menu and the /raw chat command.");
	}

	if(Config::getBoolConfig("Enabled")) {
		start();
	}

	ui->add_command(commandName.c_str(), [] { instance->onSwitched(); });
}

void Plugin::onSwitched() {
	auto oldCommand = commandName;
	if(events.empty()) {
		start();
	} else {
		close();
	}

	ui->remove_command(oldCommand.c_str());
	ui->add_command(commandName.c_str(), [] { instance->onSwitched(); });
}

Bool Plugin::onHubDataIn(HubDataPtr hHub, const char* message) {
	dialog.write(true, false, hHub->ip, hHub->port, "Hub " + string(hHub->url), message);
	return False;
}

Bool Plugin::onHubDataOut(HubDataPtr hHub, const char* message) {
	dialog.write(true, true, hHub->ip, hHub->port, "Hub " + string(hHub->url), message);
	return False;
}

Bool Plugin::onConnectionDataIn(ConnectionDataPtr hConn, const char* message) {
	dialog.write(false, false, hConn->ip, hConn->port, "User" /** @todo get user's nick */, message);
	return False;
}

Bool Plugin::onConnectionDataOut(ConnectionDataPtr hConn, const char* message) {
	dialog.write(false, true, hConn->ip, hConn->port, "User" /** @todo get user's nick */, message);
	return False;
}

Bool Plugin::onChatCommand(HubDataPtr hub, CommandDataPtr cmd) {
	if(stricmp(cmd->command, "help") == 0) {
		hubs->local_message(hub, "/raw <message>", MSG_SYSTEM);

	} else if(stricmp(cmd->command, "raw") == 0) {
		if(strlen(cmd->params) == 0) {
			hubs->local_message(hub, "Specify a message to send", MSG_SYSTEM);
		} else {
			hubs->send_protocol_cmd(hub, cmd->params);
		}
	}

	return False;
}
