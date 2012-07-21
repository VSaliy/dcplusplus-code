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
#include "Dialog.h"
#include "Plugin.h"
#include "version.h"

extern "C" {

// Plugin loader
DCEXP DCMAIN DCAPI pluginInit(MetaDataPtr info) {
	info->name = PLUGIN_NAME;
	info->author = PLUGIN_AUTHOR;
	info->description = PLUGIN_DESC;
	info->web = PLUGIN_WEB;
	info->version = PLUGIN_VERSION;
	info->apiVersion = DCAPI_CORE_VER;
	info->guid = PLUGIN_GUID;

	return &Plugin::main;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD /*fdwReason*/, LPVOID /*lpvReserved*/) {
	Dialog::instance = hinstDLL;
	return TRUE;
}
#endif

}
