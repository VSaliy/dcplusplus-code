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
#include "Util.h"

#include "version.h"

DCUtilsPtr Util::utils = nullptr;
DCConfigPtr Util::config = nullptr;
DCLogPtr Util::logger = nullptr;

string Util::formatBytes(double val) {
	int i = 0;
	while(val >= 1024 && i < 5) { val = val / 1024; i += 1; }
	const char units[5][4] = { "KiB", "MiB", "GiB", "TiB", "PiB" };

	char buf[64];
	snprintf(buf, sizeof(buf), "%.02f %s", val, units[i]);
	return buf;
}

// Make BCDC's Lua user commands work... hopefully
string Util::ucLuaConvert(const string& str, bool isAdc) {
	string::size_type i, j, k;
	i = j = k = 0;
	string tmp = str;
	while((i = tmp.find("%[lua:", i)) != string::npos) {
		i += 6;
		j = tmp.find(']', i);
		if(j == string::npos) break;
		string chunk = tmp.substr(i, j-i);
		k = 0;
		while((k = chunk.find("!%")) != string::npos) {
			chunk.erase(k, 2);
			chunk.insert(k, "%");
		}
		k = 0;
		while((k = chunk.find("!{")) != string::npos) {
			chunk.erase(k, 2);
			chunk.insert(k, "[");
		}
		k = 0;
		while((k = chunk.find("!}")) != string::npos) {
			chunk.erase(k, 2);
			chunk.insert(k, "]");
		}
		i -= 6;
		tmp.erase(i, j-i+1);
		tmp.insert(i, (isAdc ? "LuaExec " : "&#36;LuaExec ") + chunk);
		i += (isAdc ? 8 : 13) + chunk.length();
	}

	return tmp;
}

void Util::setConfig(const char* name, const char* value) {
	ConfigStr val;
	memset(&val, 0, sizeof(ConfigStr));

	val.type = CFG_TYPE_STRING;
	val.value = value;
	config->set_cfg(PLUGIN_GUID, name, (ConfigValuePtr)&val);
}

string Util::getConfig(const char *name) {
	auto value = (ConfigStrPtr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_STRING);
	string str(value->value);
	config->release((ConfigValuePtr)value);

	return str;
}

bool Util::getBoolConfig(const char *name) {
	auto value = (ConfigIntPtr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_INT);
	int32_t num = value->value;
	config->release((ConfigValuePtr)value);

	return (num != 0);
}

ConfigValuePtr Util::getCoreConfig(const char* name) {
	return config->get_cfg("CoreSetup", name, CFG_TYPE_UNKNOWN);
}

string Util::fromUtf8(const string& str) {
	string res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->from_utf8(&res[0], &str[0], res.size()));
	return res;
}

string Util::toUtf8(const string& str) {
	string res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->to_utf8(&res[0], &str[0], str.size()));
	return res;
}
