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

Plugin* Plugin::instance = nullptr;

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

	tagger = reinterpret_cast<DCTaggerPtr>(core->query_interface(DCINTF_DCPP_TAGGER, DCINTF_DCPP_TAGGER_VER));
	ui = reinterpret_cast<DCUIPtr>(core->query_interface(DCINTF_DCPP_UI, DCINTF_DCPP_UI_VER));

	if(!Util::init(core) || !Config::init(core) || !Logger::init(core) || !hooks || !tagger || !ui) {
		loadRes = False;
		return;
	}
	Core::init(core);

	if(install) {
		Logger::log("The test plugin has been installed.");
	}

	Logger::log("Test plugin loaded, watch out!");

	events[HOOK_TIMER_SECOND] = hooks->bind_hook(HOOK_TIMER_SECOND, [](dcptr_t, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onSecond(*reinterpret_cast<uint64_t*>(pData)); }, nullptr);

	events[HOOK_UI_CHAT_TAGS] = hooks->bind_hook(HOOK_UI_CHAT_TAGS, [](dcptr_t, dcptr_t pData, dcptr_t, Bool*) {
		return instance->onUiChatTags(reinterpret_cast<TagDataPtr>(pData)); }, nullptr);
}

Bool Plugin::onSecond(uint64_t tick) {
	static uint64_t prevTick = 0;
	if(tick - prevTick >= 10*1000) {
		prevTick = tick;
		ui->play_sound("Media\\tada.wav");
	}
	return False;
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
