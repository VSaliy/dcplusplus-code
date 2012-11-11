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
#include <pluginsdk/Tagger.h>
#include <pluginsdk/UI.h>
#include <pluginsdk/Util.h>

using dcapi::Config;
using dcapi::Core;
using dcapi::Hooks;
using dcapi::Logger;
using dcapi::Tagger;
using dcapi::UI;
using dcapi::Util;

Plugin::Plugin() {
}

Plugin::~Plugin() {
	Hooks::clear();
}

Bool DCAPI Plugin::main(PluginState state, DCCorePtr core, dcptr_t) {
	static Plugin* instance;

	switch(state) {
	case ON_INSTALL:
	case ON_LOAD:
		{
			instance = new Plugin();
			return instance->onLoad(core, state == ON_INSTALL) ? True : False;
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

bool Plugin::onLoad(DCCorePtr core, bool install) {
	Core::init(core);
	if(!Config::init() || !Hooks::init() || !Logger::init() || !Tagger::init() || !UI::init() || !Util::init()) {
		return false;
	}

	if(install) {
		Logger::log("The test plugin has been installed.");
	}

	Logger::log("Test plugin loaded, watch out!");

	Hooks::Timer::onSecond([this](uint64_t tick, bool&) { return onSecond(tick); });
	Hooks::UI::onChatTags([this](UserDataPtr, TagDataPtr tags, bool&) { return onUiChatTags(tags); });

	return true;
}

bool Plugin::onSecond(uint64_t tick) {
	static uint64_t prevTick = 0;
	if(tick - prevTick >= 10*1000) {
		prevTick = tick;
		UI::handle()->play_sound("Media\\tada.wav");
	}
	return false;
}

bool Plugin::onUiChatTags(TagDataPtr tags) {
	// look for the pattern and make it bold.
	const string pattern = "ABC DEF";

	string text(tags->text);
	size_t start, end = 0;
	while((start = text.find(pattern, end)) != string::npos) {
		end = start + pattern.size();
		Tagger::handle()->add_tag(tags, start, end, "b", "");
	}
	return false;
}
