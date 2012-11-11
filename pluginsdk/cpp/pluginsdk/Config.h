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

#ifndef PLUGINSDK_CONFIG_H
#define PLUGINSDK_CONFIG_H

#include <cstdint>
#include <string>

#include <pluginsdk/PluginDefs.h>

namespace dcapi {

using std::string;

class Config
{
public:
	static bool init();
	static void init(DCConfigPtr coreConfig);
	static DCConfigPtr handle();

	static void setConfig(const char* name, const char* value);
	static void setConfig(const char* name, const string& value);
	static void setConfig(const char* name, bool value);
	static void setConfig(const char* name, int32_t value);
	static void setConfig(const char* name, int64_t value);

	static string getConfig(const char *name);
	static bool getBoolConfig(const char* name);
	static int32_t getIntConfig(const char* name);
	static int64_t getInt64Config(const char* name);

	static ConfigValuePtr getCoreConfig(const char* name);
	static void freeCoreConfig(ConfigValuePtr value);

	static string getPath(PathType type);

private:
	template<typename ConfigT, typename ValueT> static void setConfig(const char* name, ConfigType type, ValueT value);
	template<typename ConfigT, typename RetT> static RetT getConfig(const char* name, ConfigType type);

	static DCConfigPtr config;
};

template<typename ConfigT, typename ValueT> void Config::setConfig(const char* name, ConfigType type, ValueT value) {
	ConfigT val = { type, value };
	config->set_cfg(PLUGIN_GUID, name, reinterpret_cast<ConfigValuePtr>(&val));
}

template<typename ConfigT, typename RetT> RetT Config::getConfig(const char* name, ConfigType type) {
	auto cfg = config->get_cfg(PLUGIN_GUID, name, type);
	RetT ret(reinterpret_cast<ConfigT>(cfg)->value);
	config->release(cfg);
	return ret;
}

} // namespace dcapi

#endif
