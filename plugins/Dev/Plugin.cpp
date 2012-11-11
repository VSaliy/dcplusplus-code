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
#include <pluginsdk/Hooks.h>
#include <pluginsdk/Logger.h>
#include <pluginsdk/UI.h>
#include <pluginsdk/Util.h>

using dcapi::Config;
using dcapi::Core;
using dcapi::Hooks;
using dcapi::Logger;
using dcapi::UI;
using dcapi::Util;

Plugin* Plugin::instance = nullptr;

Plugin::Plugin() {
}

Plugin::~Plugin() {
	clearHooks();

	if(UI::handle()) {
		UI::removeCommand(commandName);
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
	instance->close();
}

void Plugin::addHooks() {
	Hooks::Network::onHubDataIn([this](HubDataPtr hHub, char* message, bool&) { return onHubDataIn(hHub, message); });
	Hooks::Network::onHubDataOut([this](HubDataPtr hHub, char* message, bool&) { return onHubDataOut(hHub, message); });
	Hooks::Network::onClientDataIn([this](ConnectionDataPtr hConn, char* message, bool&) { return onClientDataIn(hConn, message); });
	Hooks::Network::onClientDataOut([this](ConnectionDataPtr hConn, char* message, bool&) { return onClientDataOut(hConn, message); });
	Hooks::UI::onChatCommand([this](HubDataPtr hHub, CommandDataPtr cmd, bool&) { return onChatCommand(hHub, cmd); });
}

void Plugin::clearHooks() {
	Hooks::clear();
}

void Plugin::start() {
	dialog.create();
	addHooks();
	Config::setConfig("Enabled", true);
	refreshSwitchCommand();
}

void Plugin::close() {
	Config::setConfig("Enabled", false);
	clearHooks();
	dialog.close();
	refreshSwitchCommand();
}

void Plugin::refreshSwitchCommand() {
	UI::removeCommand(commandName);
	commandName = Hooks::empty() ? PLUGIN_NAME ": enable" : PLUGIN_NAME ": disable";
	UI::addCommand(commandName, [this] { onSwitched(); });
}

void Plugin::onLoad(DCCorePtr core, bool install, Bool& loadRes) {
	hubs = reinterpret_cast<DCHubPtr>(core->query_interface(DCINTF_DCPP_HUBS, DCINTF_DCPP_HUBS_VER));

	Core::init(core);
	if(!Config::init() || !Hooks::init() || !Logger::init() || !UI::init() || !Util::init() || !hubs) {
		loadRes = False;
		return;
	}

	if(install) {
		Config::setConfig("Enabled", true);

		Logger::log("The dev plugin has been installed; check the plugins menu and the /raw chat command.");
	}

	if(Config::getBoolConfig("Enabled")) {
		start();
	} else {
		refreshSwitchCommand();
	}
}

void Plugin::onSwitched() {
	if(Hooks::empty()) {
		start();
	} else {
		close();
	}
}

bool Plugin::onHubDataIn(HubDataPtr hHub, char* message) {
	dialog.write(true, false, hHub->ip, hHub->port, "Hub " + string(hHub->url), message);
	return false;
}

bool Plugin::onHubDataOut(HubDataPtr hHub, char* message) {
	dialog.write(true, true, hHub->ip, hHub->port, "Hub " + string(hHub->url), message);
	return false;
}

bool Plugin::onClientDataIn(ConnectionDataPtr hConn, char* message) {
	dialog.write(false, false, hConn->ip, hConn->port, "User" /** @todo get user's nick */, message);
	return false;
}

bool Plugin::onClientDataOut(ConnectionDataPtr hConn, char* message) {
	dialog.write(false, true, hConn->ip, hConn->port, "User" /** @todo get user's nick */, message);
	return false;
}

bool Plugin::onChatCommand(HubDataPtr hub, CommandDataPtr cmd) {
	if(stricmp(cmd->command, "help") == 0) {
		hubs->local_message(hub, "/raw <message>", MSG_SYSTEM);

	} else if(stricmp(cmd->command, "raw") == 0) {
		if(strlen(cmd->params) == 0) {
			hubs->local_message(hub, "Specify a message to send", MSG_SYSTEM);
		} else {
			hubs->send_protocol_cmd(hub, cmd->params);
		}
	}

	return false;
}
