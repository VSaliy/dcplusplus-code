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

#include "Hooks.h"

namespace dcapi {

DCHooksPtr Hooks::hooks;
map<string, pair<subsHandle, Hooks::Callback>> Hooks::events;

bool Hooks::init(DCCorePtr core) {
	init(reinterpret_cast<DCHooksPtr>(core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER)));
	return hooks;
}
void Hooks::init(DCHooksPtr coreHooks) { hooks = coreHooks; }
DCHooksPtr Hooks::handle() { return hooks; }

void Hooks::Chat::onIncomingChat(function<bool (HubDataPtr, char*, bool&)> f) {
	addEvent(HOOK_CHAT_IN, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}
void Hooks::Chat::onOutgoingChat(function<bool (HubDataPtr, char*, bool&)> f) {
	addEvent(HOOK_CHAT_OUT, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}
void Hooks::Chat::onIncomingPM(function<bool (UserDataPtr, char*, bool&)> f) {
	addEvent(HOOK_CHAT_PM_IN, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<UserDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}
void Hooks::Chat::onOutgoingPM(function<bool (UserDataPtr, char*, bool&)> f) {
	addEvent(HOOK_CHAT_PM_OUT, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<UserDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}

void Hooks::Timer::onSecond(function<bool (uint64_t, bool&)> f) {
	addEvent(HOOK_TIMER_SECOND, [f](dcptr_t, dcptr_t pData, bool& bBreak) {
		return f(*reinterpret_cast<uint64_t*>(pData), bBreak); });
}
void Hooks::Timer::onMinute(function<bool (uint64_t, bool&)> f) {
	addEvent(HOOK_TIMER_MINUTE, [f](dcptr_t, dcptr_t pData, bool& bBreak) {
		return f(*reinterpret_cast<uint64_t*>(pData), bBreak); });
}

void Hooks::Hubs::onOnline(function<bool (HubDataPtr, bool&)> f) {
	addEvent(HOOK_HUB_ONLINE, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), bBreak); });
}
void Hooks::Hubs::onOffline(function<bool (HubDataPtr, bool&)> f) {
	addEvent(HOOK_HUB_OFFLINE, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), bBreak); });
}

void Hooks::Users::onOnline(function<bool (UserDataPtr, bool&)> f) {
	addEvent(HOOK_USER_ONLINE, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<UserDataPtr>(pObject), bBreak); });
}
void Hooks::Users::onOffline(function<bool (UserDataPtr, bool&)> f) {
	addEvent(HOOK_USER_OFFLINE, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<UserDataPtr>(pObject), bBreak); });
}

void Hooks::Network::onHubDataIn(function<bool (HubDataPtr, char*, bool&)> f) {
	addEvent(HOOK_NETWORK_HUB_IN, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}
void Hooks::Network::onHubDataOut(function<bool (HubDataPtr, char*, bool&)> f) {
	addEvent(HOOK_NETWORK_HUB_OUT, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}
void Hooks::Network::onClientDataIn(function<bool (ConnectionDataPtr, char*, bool&)> f) {
	addEvent(HOOK_NETWORK_CONN_IN, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}
void Hooks::Network::onClientDataOut(function<bool (ConnectionDataPtr, char*, bool&)> f) {
	addEvent(HOOK_NETWORK_CONN_OUT, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<ConnectionDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
}

void Hooks::Queue::onAdded(function<bool (QueueDataPtr, bool&)> f) {
	addEvent(HOOK_QUEUE_ADDED, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<QueueDataPtr>(pObject), bBreak); });
}
void Hooks::Queue::onMoved(function<bool (QueueDataPtr, bool&)> f) {
	addEvent(HOOK_QUEUE_MOVED, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<QueueDataPtr>(pObject), bBreak); });
}
void Hooks::Queue::onRemoved(function<bool (QueueDataPtr, bool&)> f) {
	addEvent(HOOK_QUEUE_REMOVED, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<QueueDataPtr>(pObject), bBreak); });
}
void Hooks::Queue::onFinished(function<bool (QueueDataPtr, bool&)> f) {
	addEvent(HOOK_QUEUE_FINISHED, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(reinterpret_cast<QueueDataPtr>(pObject), bBreak); });
}

void Hooks::UI::onCreated(function<bool (dcptr_t, bool&)> f) {
	addEvent(HOOK_UI_CREATED, [f](dcptr_t pObject, dcptr_t, bool& bBreak) {
		return f(pObject, bBreak); });
}
void Hooks::UI::onChatTags(function<bool (UserDataPtr, TagDataPtr, bool&)> f) {
	addEvent(HOOK_UI_CHAT_TAGS, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<UserDataPtr>(pObject), reinterpret_cast<TagDataPtr>(pData), bBreak); });
}
void Hooks::UI::onChatDisplay(function<bool (UserDataPtr, StringDataPtr, bool&)> f) {
	addEvent(HOOK_UI_CHAT_DISPLAY, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<UserDataPtr>(pObject), reinterpret_cast<StringDataPtr>(pData), bBreak); });
}
void Hooks::UI::onChatCommand(function<bool (HubDataPtr, CommandDataPtr, bool&)> f) {
	addEvent(HOOK_UI_CHAT_COMMAND, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<CommandDataPtr>(pData), bBreak); });
}
void Hooks::UI::onChatCommandPM(function<bool (UserDataPtr, CommandDataPtr, bool&)> f) {
	addEvent(HOOK_UI_CHAT_COMMAND_PM, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<UserDataPtr>(pObject), reinterpret_cast<CommandDataPtr>(pData), bBreak); });
}

bool Hooks::empty() {
	return events.empty();
}

void Hooks::clear() {
	for(auto& i: events)
		hooks->release_hook(i.second.first);
	events.clear();
}

void Hooks::addEvent(const char* id, Callback f) {
	// insert first to construct map keys etc; then create the hook, using the map key as pCommon.
	auto it = events.insert(make_pair(id, make_pair(nullptr, f))).first;
	it->second.first = hooks->bind_hook(id, eventCallback, reinterpret_cast<dcptr_t>(const_cast<char*>(it->first.c_str())));
}

Bool DCAPI Hooks::eventCallback(dcptr_t pObject, dcptr_t pData, dcptr_t pCommon, Bool* bBreak) {
	bool bbBreak = *bBreak;
	auto ret = events[reinterpret_cast<char*>(pCommon)].second(pObject, pData, bbBreak) ? True : False;
	*bBreak = bbBreak ? True : False;
	return ret;
}

} // namespace dcapi
