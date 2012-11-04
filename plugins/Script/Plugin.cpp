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

#include <boost/filesystem.hpp>

Plugin* Plugin::instance = nullptr;

Plugin::Plugin() { L = nullptr; }

Plugin::~Plugin() {
	if(L) {
		if(!hubs.empty()) {
			for(auto& i: hubs) {
				hub->remove_hub(i);
				hub->release(i);
			}

			Logger::log("Script plugin warning: scripts do not correctly remove hubs they add!");
			hubs.clear();
		}

		for(auto& i: events)
			hooks->release_hook(i.second);
		events.clear();

		chatCache.clear();

		lua_close(L);
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

void Plugin::onLoad(DCCorePtr core, bool install, Bool& loadRes) {
	hooks = reinterpret_cast<DCHooksPtr>(core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER));

	hub = reinterpret_cast<DCHubPtr>(core->query_interface(DCINTF_DCPP_HUBS, DCINTF_DCPP_HUBS_VER));
	connection = reinterpret_cast<DCConnectionPtr>(core->query_interface(DCINTF_DCPP_CONNECTIONS, DCINTF_DCPP_CONNECTIONS_VER));

	if(!Util::init(core) || !Config::init(core) || !Logger::init(core) || !hooks || !hub || !connection) {
		loadRes = False;
		return;
	}
	Core::init(core);

	// Default settings
	if(Config::getConfig("ScriptPath").empty()) {
		Config::setConfig("ScriptPath", Config::getPath(PATH_RESOURCES) + "scripts" +
			Util::fromT(boost::filesystem::path::string_type(boost::filesystem::path::preferred_separator)));
	}

	if(Config::getConfig("LuaDebug").empty())
		Config::setConfig("LuaDebug", false);

	if(Config::getConfig("FormatChat").empty())
		Config::setConfig("FormatChat", true);

	if(install) {
		Logger::log("Script plugin installed, please restart " + Core::appName + " to begin using the plugin.");
		return;
	}

	L = luaL_newstate();
	luaL_openlibs(L);

#ifdef WITH_LUAJIT
	// This just turns the whole JIT compiler on with default behaviour (could probably be improved)
	luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON);
#endif

	Lunar<LuaManager>::Register(L);
	lua_pop(L, lua_gettop(L));

	events[HOOK_CHAT_OUT] = hooks->bind_hook(HOOK_CHAT_OUT, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onOwnChatOut(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);

	events[HOOK_HUB_ONLINE] = hooks->bind_hook(HOOK_HUB_ONLINE, [](dcptr_t pObject, dcptr_t, dcptr_t, Bool*) {
		return instance->onHubConnected(reinterpret_cast<HubDataPtr>(pObject)); }, nullptr);
	events[HOOK_HUB_OFFLINE] = hooks->bind_hook(HOOK_HUB_OFFLINE, [](dcptr_t pObject, dcptr_t, dcptr_t, Bool*) {
		return instance->onHubDisconnected(reinterpret_cast<HubDataPtr>(pObject)); }, nullptr);

	events[HOOK_NETWORK_HUB_IN] = hooks->bind_hook(HOOK_NETWORK_HUB_IN, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onHubDataIn(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);
	events[HOOK_NETWORK_HUB_OUT] = hooks->bind_hook(HOOK_NETWORK_HUB_OUT, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool* bBreak) {
		return instance->onHubDataOut(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); }, nullptr);
	events[HOOK_NETWORK_CONN_IN] = hooks->bind_hook(HOOK_NETWORK_CONN_IN, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onConnectionDataIn(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);
	events[HOOK_NETWORK_CONN_OUT] = hooks->bind_hook(HOOK_NETWORK_CONN_OUT, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onConnectionDataOut(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData)); }, nullptr);

	events[HOOK_UI_PROCESS_CHAT_CMD] = hooks->bind_hook(HOOK_UI_PROCESS_CHAT_CMD, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onHubEnter(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<CommandDataPtr>(pData)); }, nullptr);

	/// @todo let the user configure which files to auto-load
	auto file = Util::fromUtf8(Config::getConfig("ScriptPath")) + "startup.lua";
	if(boost::filesystem::exists(file))
		ScriptInstance::EvaluateFile(file);

	// This ensures that FormatChatText is only called when present...
	if(CheckFunction("dcpp", "FormatChatHTML")) {
		events[HOOK_UI_CHAT_DISPLAY] = hooks->bind_hook(HOOK_UI_CHAT_DISPLAY, [](dcptr_t pObject, dcptr_t pData, dcptr_t, Bool* bBreak) {
			return instance->onFormatChat(reinterpret_cast<UserDataPtr>(pObject), reinterpret_cast<StringDataPtr>(pData), bBreak); }, nullptr);
	}
}

void Plugin::setTimer(bool bState) {
	auto i = events.find(HOOK_TIMER_SECOND);
	if(bState && i == events.end()) {
		events[HOOK_TIMER_SECOND] = hooks->bind_hook(HOOK_TIMER_SECOND, [](dcptr_t, dcptr_t, dcptr_t, Bool*) {
			return instance->onTimer(); }, nullptr);
	} else if(i != events.end()) {
		hooks->release_hook(i->second);
		i->second = nullptr;
	}
}

namespace { string formatBytes(double val) {
	int i = 0;
	while(val >= 1024 && i < 5) { val = val / 1024; i += 1; }
	const char units[5][4] = { "KiB", "MiB", "GiB", "TiB", "PiB" };

	char buf[64];
	snprintf(buf, sizeof(buf), "%.02f %s", val, units[i]);
	return buf;
} }

Bool Plugin::onHubEnter(HubDataPtr hHub, CommandDataPtr cmd) {
	if(cmd->isPrivate)
		return False;

	if(stricmp(cmd->command, "help") == 0) {
		if(stricmp(cmd->params, "plugins") == 0) {
			const char* help =
				"\t\t\t Help: Script plugin \t\t\t\n"
				"\t /lua <code> \t\t\t Evaluate parameter as Lua code\n"
				"\t /luafile <file> \t\t\t Execute specified script file\n"
				"\t /luamem \t\t\t Return Lua memory usage and version\n"
				"\t /luadebug \t\t\t Toggle non-fatal error messages (default: off)\n"
				"\t /formatchat \t\t\t Toggle Lua chat formatting hook (default: on, if available)\n";

			hub->local_message(hHub, help, MSG_SYSTEM);
			return True;
		}
	} else if(stricmp(cmd->command, "lua") == 0) {
		if(strlen(cmd->params) != 0) {
			ScriptInstance::EvaluateChunk(cmd->params);
		} else {
			hub->local_message(hHub, "You must supply a parameter!", MSG_SYSTEM);
		}
		return True;
	} else if(stricmp(cmd->command, "luafile") == 0) {
		if(strlen(cmd->params) > 4) {
			ScriptInstance::EvaluateFile(cmd->params);
		} else {
			hub->local_message(hHub, "You must supply a valid parameter!", MSG_SYSTEM);
		}
		return True;
	} else if(stricmp(cmd->command, "meminfo") == 0 || stricmp(cmd->command, "luamem") == 0) {
		// Run GC and get LUA memory usage
		lua_gc(L, LUA_GCCOLLECT, 0);
		double mem = lua_gc(L, LUA_GCCOUNT, 0);
		auto stats = "Scripts (" LUA_DIST ") are using " + formatBytes(mem) + " of system memory";

		hub->local_message(hHub, stats.c_str(), MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "luadebug") == 0) {
		bool state = Config::getBoolConfig("LuaDebug");
		const char* status = (state ? "Additional error messages disabled" : "Additional error messages enabled");

		Config::setConfig("LuaDebug", state);
		hub->local_message(hHub, status, MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "formatchat") == 0) {
		bool state = Config::getBoolConfig("FormatChat");
		const char* status = (state ? "Lua chat formatting disabled" : "Lua chat formatting enabled");

		Config::setConfig("FormatChat", state);
		hub->local_message(hHub, status, MSG_SYSTEM);
		return True;
	}

	// This stupidity is because of the API
	return onOwnChatOut(hHub, string("/" + string(cmd->command) + " " + string(cmd->params)).c_str());
}

Bool Plugin::onOwnChatOut(HubDataPtr hHub, const char* message) {
	Lock l(cs);
	return MakeCall("dcpp", "OnCommandEnter", 1, hHub, string(message)) ? GetLuaBool() : False;
}

Bool Plugin::onHubConnected(HubDataPtr hHub) {
	MakeCall(GetHubType(hHub), "OnHubAdded", 0, hHub);

	return False;
}

Bool Plugin::onHubDisconnected(HubDataPtr hHub) {
	// fixme: DC++ may trigger this incorrectly (for hubs where OnHubAdded was never invoked), if socket creation fails...
	MakeCall(GetHubType(hHub), "OnHubRemoved", 0, hHub);
	removeChatCache(hHub);

	return False;
}

Bool Plugin::onHubDataIn(HubDataPtr hHub, const char* message) {
	Lock l(cs);
	return MakeCall(GetHubType(hHub), "DataArrival", 1, hHub, string(message)) ? GetLuaBool() : False;
}

Bool Plugin::onHubDataOut(HubDataPtr hHub, const char* message, Bool* bBreak) {
	string sText = message;
	if(sText.find("LuaExec") != string::npos) {
		string delim = (hHub->protocol == PROTOCOL_ADC) ? "\n" : "|";
		string::size_type i = 0, j = 0;
		string out;
		while((i = sText.find(delim, i)) < sText.length()) {
			string cmd = sText.substr(j, i-j);
			if(cmd.find("LuaExec") != string::npos) {
				ScriptInstance::EvaluateChunk(cmd.substr(cmd.find(" ") + 1));
			} else if(cmd.length() > 1) {
				out += cmd + delim;
			}
			j = ++i;
		}
		if(out.length() > 1)
			hub->send_protocol_cmd(hHub, out.c_str());

		// We don't need to send this to other plugins
		*bBreak = True;
		return True;
	}

	return False;
}

Bool Plugin::onConnectionDataIn(ConnectionDataPtr hConn, const char* message) {
	Lock l(cs);
	return MakeCall("dcpp", "UserDataIn", 1, hConn, string(message)) ? GetLuaBool() : False;
}

Bool Plugin::onConnectionDataOut(ConnectionDataPtr hConn, const char* message) {
	Lock l(cs);
	return MakeCall("dcpp", "UserDataOut", 1, hConn, string(message)) ? GetLuaBool() : False;
}

Bool Plugin::onFormatChat(UserDataPtr hUser, StringDataPtr line, Bool* bBreak) {
	Lock l(cs);
	if(!hUser || !Config::getBoolConfig("FormatChat") || !MakeCall("dcpp", "FormatChatHTML", 1, hUser, string(line->in)))
		return False;

	if(lua_isstring(L, -1)) {
		string text = lua_tostring(L, -1);
		if(strcmp(line->in, text.c_str()) != 0) {
			string& out = chatCache[hUser->hubHint] = text;
			line->out = &out[0];
		}
	}

	lua_settop(L, 0);

	if(line->out != NULL) {
		// We don't send this to other plugins (users can control which formatting applies via plugin order)
		*bBreak = True;
		return True;
	} else return False;
}

Bool Plugin::onTimer() {
	MakeCall("dcpp", "OnTimer", 0, 0);
	return False;
}
