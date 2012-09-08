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
#include "Util.h"

Plugin::Plugin() {
}

Plugin::~Plugin() {
	for(auto& i: events)
		hooks->release_hook(i.second);
	events.clear();
}

Bool DCAPI Plugin::main(PluginState state, DCCorePtr core, dcptr_t) {
	switch(state) {
	case ON_INSTALL:
	case ON_LOAD:
		{
			Bool res = True;
			newInstance();
			getInstance()->onLoad(core, (state == ON_INSTALL), res);
			return res;
		}

	case ON_UNINSTALL:
	case ON_UNLOAD:
		{
			deleteInstance();
			return True;
		}

	default:
		{
			return False;
		}
	}
}

void Plugin::onLoad(DCCorePtr core, bool install, Bool& loadRes) {
	dcpp = core;
	hooks = reinterpret_cast<DCHooksPtr>(core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER));

	auto utils = reinterpret_cast<DCUtilsPtr>(core->query_interface(DCINTF_DCPP_UTILS, DCINTF_DCPP_UTILS_VER));
	auto config = reinterpret_cast<DCConfigPtr>(core->query_interface(DCINTF_CONFIG, DCINTF_CONFIG_VER));
	auto logger = reinterpret_cast<DCLogPtr>(core->query_interface(DCINTF_LOGGING, DCINTF_LOGGING_VER));

	tagger = reinterpret_cast<DCTaggerPtr>(core->query_interface(DCINTF_DCPP_TAGGER, DCINTF_DCPP_TAGGER_VER));

	if(!utils || !config || !logger || !tagger) {
		loadRes = False;
		return;
	}

	Util::initialize(core->host_name(), utils, config, logger);

	Util::logMessage("Test plugin loaded, watch out!");

	/*if(install) {
		// Default settings

		Util::logMessage("Test plugin installed, please restart " + Util::appName + " to begin using the plugin.");
		return;
	}*/

	events[HOOK_UI_CHAT_TAGS] = hooks->bind_hook(HOOK_UI_CHAT_TAGS, &uiChatTags, 0);
}

Bool Plugin::onUiChatTags(TagDataPtr tags) {
	// look for the pattern and make it bold.
	const string pattern = "ABC DEF";

	string text(tags->text);
	size_t start, end = 0;
	while((start = text.find(pattern, end)) != string::npos) {
		end = start + pattern.size();
		tagger->add_tag(tags, start, end, "b", "");
	}
	return False;
}
