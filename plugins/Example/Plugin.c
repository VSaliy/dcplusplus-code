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

#ifndef __cplusplus
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#else
# include <cstdio>
# include <cstdlib>
# include <cstring>
#endif

#ifdef _WIN32
# include "resource.h"
# ifdef _MSC_VER
#  define snprintf _snprintf
#  define snwprintf _snwprintf
# endif
#elif __GNUC__
# define stricmp strcasecmp
# define strnicmp strncasecmp
#else
# error No supported compiler found
#endif

/* Variables */
DCCorePtr dcpp;

DCHooksPtr hooks;
DCConfigPtr config;
DCLogPtr logging;

DCHubPtr hub;
DCUtilsPtr utils = NULL;

/* Hook subscription store */
#define HOOKS_SUBSCRIBED 2

const char* hookGuids[HOOKS_SUBSCRIBED] = {
	HOOK_UI_PROCESS_CHAT_CMD,
	HOOK_HUB_ONLINE
};

DCHOOK hookFuncs[HOOKS_SUBSCRIBED] = {
	&onHubEnter,
	&onHubOnline
};

subsHandle subs[HOOKS_SUBSCRIBED];

Bool onLoad(uint32_t eventId, DCCorePtr core) {
	uint32_t i = 0;
	dcpp = core;

	hooks = (DCHooksPtr)core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER);
	config = (DCConfigPtr)core->query_interface(DCINTF_CONFIG, DCINTF_CONFIG_VER);
	logging = (DCLogPtr)core->query_interface(DCINTF_LOGGING, DCINTF_LOGGING_VER);

	hub = (DCHubPtr)core->query_interface(DCINTF_DCPP_HUBS, DCINTF_DCPP_HUBS_VER);
#ifdef _WIN32
	utils = (DCUtilsPtr)core->query_interface(DCINTF_DCPP_UTILS, DCINTF_DCPP_UTILS_VER);
#endif

	if(eventId == ON_INSTALL) {
		/* Default settings */
		set_cfg("SendSuffix", "<DC++ Plugins Test>");
	}

	while(i < HOOKS_SUBSCRIBED) {
		subs[i] = hooks->bind_hook(hookGuids[i], hookFuncs[i], NULL);
		++i;
	}

	return True;
}

Bool onUnload() {
	uint32_t i = 0;
	while(i < HOOKS_SUBSCRIBED) {
		if(subs[i]) hooks->release_hook(subs[i]);
		++i;
	}
	return True;
}

/* Event handlers */
Bool DCAPI onHubEnter(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak) {
	HubDataPtr hHub = (HubDataPtr)pObject;
	CommandDataPtr cmd = (CommandDataPtr)pData;

	if(cmd->isPrivate)
		return False;

	if(stricmp(cmd->command, "help") == 0 && stricmp(cmd->params, "plugins") == 0) {
		const char* help =
			"\t\t\t Help: Example plugin \t\t\t\n"
			"\t /pluginhelp \t\t\t Prints info about the purpose of this plugin\n"
			"\t /plugininfo \t\t\t Prints info about the example plugin\n"
			"\t /unhook <index> \t\t Hooks test\n"
			"\t /rehook <index> \t\t Hooks test\n"
			"\t /send <text> \t\t\t Chat message test\n";

		hub->local_message(hHub, help, MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "pluginhelp") == 0) {
		const char* pluginhelp =
			"\t\t\t Plugin help: Example plugin \t\t\t\n"
			"\t The example plugin project is intended to both demonstrate the use and test the implementation of the API\n"
			"\t as such the plugin itself does nothing useful but it can be used to verify whether an implementation works\n"
			"\t with the API or not, however, it is by no means intended to be a comprehensive testing tool for the API.\n";

		hub->local_message(hHub, pluginhelp, MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "plugininfo") == 0) {
		const char* info =
			"\t\t\t Plugin info: Example plugin \t\t\t\n"
			"\t Name: \t\t\t\t" PLUGIN_NAME "\n"
			"\t Author: \t\t\t" PLUGIN_AUTHOR "\n"
			"\t Version: \t\t\t" STRINGIZE(PLUGIN_VERSION) "\n"
			"\t Description: \t\t\t" PLUGIN_DESC "\n"
			"\t GUID/UUID: \t\t\t" PLUGIN_GUID "\n";

		hub->local_message(hHub, info, MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "unhook") == 0) {
		/* Unhook test */
		if(strlen(cmd->params) == 0) {
			hub->local_message(hHub, "You must supply a parameter!", MSG_SYSTEM);
		} else {
			uint32_t subIdx = atoi(cmd->params);
			if((subIdx < HOOKS_SUBSCRIBED) && subs[subIdx]) {
				hooks->release_hook(subs[subIdx]);
				subs[subIdx] = 0;
				hub->local_message(hHub, "Hooking changed...", MSG_SYSTEM);
			}
		}
		return True;
	} else if(stricmp(cmd->command, "rehook") == 0) {
		/* Rehook test */
		if(strlen(cmd->params) == 0) {
			hub->local_message(hHub, "You must supply a parameter!", MSG_SYSTEM);
		} else {
			uint32_t subIdx = atoi(cmd->params);
			if((subIdx < HOOKS_SUBSCRIBED) && !subs[subIdx]) {
				subs[subIdx] = hooks->bind_hook(hookGuids[subIdx], hookFuncs[subIdx], NULL);
				hub->local_message(hHub, "Hooking changed...", MSG_SYSTEM);
			}
		}
		return True;
	} else if(stricmp(cmd->command, "send") == 0) {
		size_t len = strlen(cmd->params);
		if(len > 0) {
			ConfigStrPtr suffix = get_cfg("SendSuffix");
			size_t msgLen = len + strlen(suffix->value) + 2;
			char* text = (char*)memset(malloc(msgLen), 0, msgLen);

			strcat(text, cmd->params);
			text[len] = ' ';
			strcat(text, suffix->value);

			hub->send_message(hHub, text, (strnicmp(text, "/me ", 4) == 0) ? True : False);

			free(text);
			config->release((ConfigValuePtr)suffix);
		} else {
			hub->local_message(hHub, "You must supply a parameter!", MSG_SYSTEM);
		}
		return True;
	}
	return False;
}

Bool DCAPI onHubOnline(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak) {
	HubDataPtr hHub = (HubDataPtr)pObject;

	char* buf = (char*)memset(malloc(256), 0, 256);
	snprintf(buf, 256, "*** %s connected! (%s)", hHub->url, (hHub->protocol == PROTOCOL_ADC ? "adc" : "nmdc"));

	logging->log(buf);
	free(buf);

	return False;
}

#ifdef _WIN32
/* Config dialog stuff */
BOOL onConfigInit(HWND hWnd) {
	ConfigStrPtr value = get_cfg("SendSuffix");
	size_t len = strlen(value->value) + 1;
	TCHAR* buf = (TCHAR*)memset(malloc(len * sizeof(TCHAR)), 0, len * sizeof(TCHAR));

	utils->utf8_to_wcs(buf, value->value, len);
	config->release((ConfigValuePtr)value);
	value = NULL;

	SetDlgItemText(hWnd, IDC_SUFFIX, buf);
	SetWindowText(hWnd, _T(PLUGIN_NAME) _T(" Settings"));

	free(buf);
	return TRUE;
}

BOOL onConfigClose(HWND hWnd, UINT wID) {
	if(wID == IDOK) {
		int len = GetWindowTextLength(GetDlgItem(hWnd, IDC_SUFFIX)) + 1;
		TCHAR* wbuf = (TCHAR*)memset(malloc(len * sizeof(TCHAR)), 0, len * sizeof(TCHAR));
		char* value = (char*)memset(malloc(len), 0, len);

		GetWindowText(GetDlgItem(hWnd, IDC_SUFFIX), wbuf, len);
		utils->wcs_to_utf8(value, wbuf, len);
		set_cfg("SendSuffix", value);

		free(value);
		free(wbuf);
	}

	EndDialog(hWnd, wID);
	return FALSE;
}

INT_PTR CALLBACK configProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch(uMsg) {
		case WM_INITDIALOG:
			return onConfigInit(hWnd);
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
				case IDCLOSE:
					return onConfigClose(hWnd, LOWORD(wParam));
			}
		}
	}
	return FALSE;
}
#endif

Bool onConfig(dcptr_t hWnd) {
#ifdef _WIN32
	DialogBox(hInst, MAKEINTRESOURCE(IDD_PLUGINDLG), (HWND)hWnd, configProc);
	return True;
#else
	return False;
#endif
}

/* Settings helpers */
ConfigStrPtr DCAPI get_cfg(const char* name) {
	return (ConfigStrPtr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_STRING);
}

ConfigIntPtr DCAPI get_cfg_int(const char* name) {
	return (ConfigIntPtr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_INT);
}

ConfigInt64Ptr DCAPI get_cfg_int64(const char* name) {
	return (ConfigInt64Ptr)config->get_cfg(PLUGIN_GUID, name, CFG_TYPE_INT64);
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

/* Plugin main function */
Bool DCAPI pluginMain(PluginState state, DCCorePtr core, dcptr_t pData) {
	switch(state) {
		case ON_INSTALL:
		case ON_LOAD:
			return onLoad(state, core);
		case ON_UNINSTALL:
		case ON_UNLOAD:
			return onUnload();
		case ON_CONFIGURE:
			return onConfig(pData);
		default: return False;
	}
}
