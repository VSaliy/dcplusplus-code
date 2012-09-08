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

#include <Singleton.h>

#include <map>

using std::map;
using std::string;

class Plugin : public dcpp::Singleton<Plugin>
{
public:
	static Bool DCAPI main(PluginState state, DCCorePtr core, dcptr_t);

private:
	friend class dcpp::Singleton<Plugin>;

	Plugin();
	~Plugin();

	void onLoad(DCCorePtr core, bool install, Bool& loadRes);
	Bool onUiChatTags(TagDataPtr tags);

	// Event wrappers
	static Bool DCAPI uiChatTags(dcptr_t /*pObject*/, dcptr_t pData, dcptr_t, Bool* /*bBreak*/) {
		return getInstance()->onUiChatTags(reinterpret_cast<TagDataPtr>(pData));
	}

	map<string, subsHandle> events;

	DCCorePtr dcpp;
	DCHooksPtr hooks;

	DCTaggerPtr tagger;
};

#endif
