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

#include <stdlib.h>
#include <string.h>

DCConfigPtr config = NULL;

Bool DCAPI init_cfg(DCCorePtr core) {
	config = (DCConfigPtr)core->query_interface(DCINTF_CONFIG, DCINTF_CONFIG_VER);
	return config ? True : False;
}

char* DCAPI get_cfg(const char* name) {
	ConfigStrPtr val = (ConfigStrPtr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_STRING);
	int len = strlen(val->value) + 1;
	char* value = (char*)memset(malloc(len), 0, len);

	strncpy(value, val->value, len);
	config->release((ConfigValuePtr)val);

	return value;
}

int32_t DCAPI get_cfg_int(const char* name) {
	ConfigIntPtr val = (ConfigIntPtr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_INT);
	int32_t value = val->value;

	config->release((ConfigValuePtr)val);

	return value;
}

int64_t DCAPI get_cfg_int64(const char* name) {
	ConfigInt64Ptr val = (ConfigInt64Ptr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_INT64);
	int64_t value = val->value;

	config->release((ConfigValuePtr)val);

	return value;
}

void DCAPI set_cfg(const char* name, const char* value) {
	ConfigStr val;
	memset(&val, 0, sizeof(ConfigStr));

	val.type = CFG_TYPE_STRING;
	val.value = value;
	config->set_cfg(PLUGIN_GUID, name, (ConfigValuePtr)&val);
}

void DCAPI set_cfg_int(const char* name, int32_t value) {
	ConfigInt val;
	memset(&val, 0, sizeof(ConfigInt64));

	val.type = CFG_TYPE_INT;
	val.value = value;
	config->set_cfg(PLUGIN_GUID, name, (ConfigValuePtr)&val);
}

void DCAPI set_cfg_int64(const char* name, int64_t value) {
	ConfigInt64 val;
	memset(&val, 0, sizeof(ConfigInt64));

	val.type = CFG_TYPE_INT64;
	val.value = value;
	config->set_cfg(PLUGIN_GUID, name, (ConfigValuePtr)&val);
}

ConfigValuePtr DCAPI get_core_cfg(const char* name) { return config->get_cfg("CoreSetup", name, CFG_TYPE_UNKNOWN); }
void DCAPI free_core_cfg(ConfigValuePtr val) { config->release(val); }

const char* DCAPI get_cfg_path(PathType path) { return config->get_path(path); }
