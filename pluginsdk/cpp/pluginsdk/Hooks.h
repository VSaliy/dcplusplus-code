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

/* Helpers around the DCHooks interface. */

#ifndef PLUGINSDK_HOOKS_H
#define PLUGINSDK_HOOKS_H

#include <functional>
#include <map>
#include <string>
#include <utility>

#include <pluginsdk/PluginDefs.h>

namespace dcapi {

using std::function;
using std::map;
using std::string;
using std::pair;

class Hooks
{
public:
	static bool init();
	static void init(DCHooksPtr coreHooks);
	static DCHooksPtr handle();

	/* The following functions register events. See the Hooks section of PluginDefs.h to see a
	description of each.
	Callbacks return a bool to indicate whether they want to prevent the plugin host from doing any
	further processing related to the event.
	Callbacks are also given a "bool& bBreak" argument to indicate whether they want to prevent
	other plugins from catching the event.
	Remember to remove hooks the plugin has added by calling Hooks::clear() before destroying the
	plugin. */

	struct Chat {
		static void onIncomingChat(function<bool (HubDataPtr, char*, bool&)> f);
		static void onOutgoingChat(function<bool (HubDataPtr, char*, bool&)> f);
		static void onIncomingPM(function<bool (UserDataPtr, char*, bool&)> f);
		static void onOutgoingPM(function<bool (UserDataPtr, char*, bool&)> f);
	};

	struct Timer {
		static void onSecond(function<bool (uint64_t, bool&)> f);
		static void onMinute(function<bool (uint64_t, bool&)> f);
	};

	struct Hubs {
		static void onOnline(function<bool (HubDataPtr, bool&)> f);
		static void onOffline(function<bool (HubDataPtr, bool&)> f);
	};

	struct Users {
		static void onOnline(function<bool (UserDataPtr, bool&)> f);
		static void onOffline(function<bool (UserDataPtr, bool&)> f);
	};

	struct Network {
		static void onHubDataIn(function<bool (HubDataPtr, char*, bool&)> f);
		static void onHubDataOut(function<bool (HubDataPtr, char*, bool&)> f);
		static void onClientDataIn(function<bool (ConnectionDataPtr, char*, bool&)> f);
		static void onClientDataOut(function<bool (ConnectionDataPtr, char*, bool&)> f);
	};

	struct Queue {
		static void onAdded(function<bool (QueueDataPtr, bool&)> f);
		static void onMoved(function<bool (QueueDataPtr, bool&)> f);
		static void onRemoved(function<bool (QueueDataPtr, bool&)> f);
		static void onFinished(function<bool (QueueDataPtr, bool&)> f);
	};

	struct UI {
		static void onCreated(function<bool (dcptr_t, bool&)> f);
		static void onChatTags(function<bool (UserDataPtr, TagDataPtr, bool&)> f);
		static void onChatDisplay(function<bool (UserDataPtr, StringDataPtr, bool&)> f);
		static void onChatCommand(function<bool (HubDataPtr, CommandDataPtr, bool&)> f);
		static void onChatCommandPM(function<bool (UserDataPtr, CommandDataPtr, bool&)> f);
	};

	static bool empty();
	static void clear();
	static void remove(const char* id);

private:
	typedef function<bool (dcptr_t, dcptr_t, bool&)> Callback;
	static void addEvent(const char* id, Callback f);
	static Bool DCAPI eventCallback(dcptr_t pObject, dcptr_t pData, dcptr_t pCommon, Bool* bBreak);

	static DCHooksPtr hooks;

	static map<string, pair<subsHandle, Callback>> events;
};

} // namespace dcapi

#endif
