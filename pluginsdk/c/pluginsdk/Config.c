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

#include <string.h>

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

extern ConfigStrPtr DCAPI get_cfg(const char* name);
extern ConfigIntPtr DCAPI get_cfg_int(const char* name);
extern ConfigInt64Ptr DCAPI get_cfg_int64(const char* name);

extern ConfigValuePtr DCAPI get_core_cfg(const char* name);
extern void DCAPI free_cfg(ConfigValuePtr val);