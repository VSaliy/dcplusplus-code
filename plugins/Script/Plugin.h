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

#ifndef SCRIPT_PLUGIN_H
#define SCRIPT_PLUGIN_H

#include "ScriptInstance.h"

#include <map>
#include <set>

using std::string;
using std::map;
using std::set;

class Plugin : public ScriptInstance
{
public:
	static Bool DCAPI main(PluginState state, DCCorePtr core, dcptr_t);

	void setTimer(bool bState);

	HubDataPtr createHub(const char* url, const char* nick, const char* password) {
		auto hHub = hub->add_hub(url, nick, password);
		if(!hHub) hubs.insert(hHub);
		return hHub;
	}

	void destroyHub(HubDataPtr hHub) {
		auto i = hubs.find(hHub);
		if(i != hubs.end()) {
			hub->remove_hub(*i);
			hub->release(*i);
			hubs.erase(i);
		}
	}

	void sendHubCommand(HubDataPtr hHub, const string& cmd) {
		if(hHub)
			hub->send_protocol_cmd(hHub, cmd.c_str());
	}

	void injectHubCommand(HubDataPtr hHub, const string& cmd) {
		if(hHub)
			hub->emulate_protocol_cmd(hHub, cmd.c_str());
	}

	// Accessors (c<->c connections)
	void sendUdpPacket(const char* ip, uint32_t port, const string& data) {
		connection->send_udp_data(ip, port, (dcptr_t)data.c_str(), data.size());
	}

	void sendClientCommand(ConnectionDataPtr hConn, const char* cmd) {
		if(hConn)
			connection->send_protocol_cmd(hConn, cmd);
	}

	void dropClientConnection(ConnectionDataPtr hConn, bool graceless) {
		if(hConn)
			connection->terminate_conn(hConn, graceless ? True : False);
	}

	static Plugin* getInstance() { return instance; }

private:
	Plugin();
	~Plugin();

	void onLoad(DCCorePtr core, bool install, Bool& loadRes);
	Bool onHubEnter(HubDataPtr hHub, CommandDataPtr cmd);
	Bool onOwnChatOut(HubDataPtr hHub, const char* message);
	Bool onHubConnected(HubDataPtr hHub);
	Bool onHubDisconnected(HubDataPtr hHub);
	Bool onHubDataIn(HubDataPtr hHub, const char* message);
	Bool onHubDataOut(HubDataPtr hHub, const char* message, Bool* bBreak);
	Bool onConnectionDataIn(ConnectionDataPtr hConn, const char* message);
	Bool onConnectionDataOut(ConnectionDataPtr hConn, const char* message);
	Bool onFormatChat(UserDataPtr hUser, StringDataPtr line, Bool* bBreak);
	Bool onTimer();

	void removeChatCache(const HubDataPtr hub) {
		auto j = chatCache.find(hub->url);
		if(j != chatCache.end())
			chatCache.erase(j);
	}

	map<string, subsHandle> events;
	map<string, string> chatCache;
	set<HubDataPtr> hubs;

	DCCorePtr dcpp;
	DCHooksPtr hooks;

	DCHubPtr hub;
	DCConnectionPtr connection;

	/** @todo switch to dcpp::Singleton when <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=51494>
	is fixed */
	static Plugin* instance;
};

#endif
