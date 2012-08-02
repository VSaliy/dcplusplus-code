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

string Util::appName; 

DCUtilsPtr Util::utils = nullptr;
DCConfigPtr Util::config = nullptr;
DCLogPtr Util::logger = nullptr;

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

string Util::fromT(const tstring& str) {
	string res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->wcs_to_utf8(&res[0], &str[0], res.size()));
	return res;
}

tstring Util::toT(const string& str) {
	tstring res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->utf8_to_wcs(&res[0], &str[0], str.size()));
	return res;
}
