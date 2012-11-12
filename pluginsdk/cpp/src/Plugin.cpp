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

/* Include plugin SDK helpers. There are more interfaces available that can be included in the same
fashion (check the pluginsdk directory). */
#include <pluginsdk/Config.h>
#include <pluginsdk/Core.h>
#include <pluginsdk/Hooks.h>
#include <pluginsdk/Logger.h>
#include <pluginsdk/Util.h>

/* Plugin SDK helpers are in the "dcapi" namespace; ease their calling. */
using dcapi::Config;
using dcapi::Core;
using dcapi::Hooks;
using dcapi::Logger;
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
			instance = 0;
			return True;
		}

	default:
		{
			return False;
		}
	}
}

bool Plugin::onLoad(DCCorePtr core, bool install) {
	/* Initialization phase. Initiate additional interfaces that you may have included from the
	plugin SDK. */
	Core::init(core);
	if(!Config::init(PLUGIN_GUID) || !Hooks::init() || !Logger::init() || !Util::init()) {
		return false;
	}

	if(install) {
		// This only executes when the plugin has been installed for the first time.
	}

	// Start the plugin logic here; add hooks with functions from the Hooks interface.

	return true;
}
