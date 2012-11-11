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

/* Helpers around the DCConfig interface. */

#include "Config.h"

#include "Core.h"

namespace dcapi {

DCConfigPtr Config::config;

bool Config::init() {
	if(!Core::handle()) { return false; }
	init(reinterpret_cast<DCConfigPtr>(Core::handle()->query_interface(DCINTF_CONFIG, DCINTF_CONFIG_VER)));
	return config;
}
void Config::init(DCConfigPtr coreConfig) { config = coreConfig; }
DCConfigPtr Config::handle() { return config; }

void Config::setConfig(const char* name, const char* value) { setConfig<ConfigStr>(name, CFG_TYPE_STRING, value); }
void Config::setConfig(const char* name, const string& value) { setConfig(name, value.c_str()); }
void Config::setConfig(const char* name, bool value) { setConfig<ConfigBool>(name, CFG_TYPE_BOOL, value ? True : False); };
void Config::setConfig(const char* name, int32_t value) { setConfig<ConfigInt>(name, CFG_TYPE_INT, value); }
void Config::setConfig(const char* name, int64_t value) { setConfig<ConfigInt64>(name, CFG_TYPE_INT64, value); }

string Config::getConfig(const char* name) { return getConfig<ConfigStrPtr, string>(name, CFG_TYPE_STRING); }
bool Config::getBoolConfig(const char* name) { return getConfig<ConfigBoolPtr, bool>(name, CFG_TYPE_BOOL); }
int32_t Config::getIntConfig(const char* name) { return getConfig<ConfigIntPtr, int32_t>(name, CFG_TYPE_INT); }
int64_t Config::getInt64Config(const char* name) { return getConfig<ConfigInt64Ptr, int64_t>(name, CFG_TYPE_INT64); }

ConfigValuePtr Config::getCoreConfig(const char* name) { return config->get_cfg("CoreSetup", name, CFG_TYPE_UNKNOWN); }
void Config::freeCoreConfig(ConfigValuePtr value) { config->release(value); }

string Config::getPath(PathType type) { return config->get_path(type); }

} // namespace dcapi
