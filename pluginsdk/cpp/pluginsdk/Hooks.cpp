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

/* Helpers around the DCCore interface. */

#include "Hooks.h"

namespace dcapi {

DCHooksPtr Hooks::hooks;
map<string, pair<subsHandle, Hooks::Callback>> Hooks::events;

bool Hooks::init(DCCorePtr core) {
	init(reinterpret_cast<DCHooksPtr>(core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER)));
	return hooks;
}
void Hooks::init(DCHooksPtr coreHooks) { hooks = coreHooks; }

void Hooks::onHubDataIn(function<bool (HubDataPtr, char*, bool&)> f) {
	addEvent(HOOK_NETWORK_HUB_IN, [f](dcptr_t pObject, dcptr_t pData, bool& bBreak) {
		return f(reinterpret_cast<HubDataPtr>(pObject), reinterpret_cast<char*>(pData), bBreak); });
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
