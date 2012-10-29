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

#include <map>

using std::map;
using std::string;

class Plugin
{
public:
	static Bool DCAPI main(PluginState state, DCCorePtr core, dcptr_t);

private:
	Plugin();
	~Plugin();

	void onLoad(DCCorePtr core, bool install, Bool& loadRes);
	Bool onSecond(uint64_t tick);
	Bool onUiChatTags(TagDataPtr tags);

	map<string, subsHandle> events;

	DCCorePtr dcpp;
	DCHooksPtr hooks;

	DCTaggerPtr tagger;
	DCUIPtr ui;

	/** @todo switch to dcpp::Singleton when <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=51494>
	is fixed */
	static Plugin* instance;
};

#endif
