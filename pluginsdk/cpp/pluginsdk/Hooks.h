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

#include <cstdint>
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
	static bool init(DCCorePtr core);
	static void init(DCHooksPtr coreHooks);

	static void onHubDataIn(function<bool (HubDataPtr, char*, bool&)> f);

	static bool empty();
	static void clear();

private:
	typedef function<bool (dcptr_t, dcptr_t, bool&)> Callback;
	static void addEvent(const char* id, Callback f);
	static Bool DCAPI eventCallback(dcptr_t pObject, dcptr_t pData, dcptr_t pCommon, Bool* bBreak);

	static DCHooksPtr hooks;

	static map<string, pair<subsHandle, Callback>> events;
};

} // namespace dcapi

#endif
