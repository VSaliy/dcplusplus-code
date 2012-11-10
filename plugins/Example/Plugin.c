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

#include "Dialog.h"

#ifndef __cplusplus
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#else
# include <cstdio>
# include <cstdlib>
# include <cstring>
#endif

#include <pluginsdk/Config.h>

/* Variables */
DCCorePtr dcpp;

DCHooksPtr hooks;
DCLogPtr logging;

DCHubPtr hub;
DCTaggerPtr tagger;
DCUIPtr ui = NULL;

/* Hook subscription store */
#define HOOKS_SUBSCRIBED 4

const char* hookGuids[HOOKS_SUBSCRIBED] = {
	HOOK_UI_PROCESS_CHAT_CMD,
	HOOK_HUB_ONLINE,
	HOOK_TIMER_SECOND,
	HOOK_UI_CHAT_TAGS
};

DCHOOK hookFuncs[HOOKS_SUBSCRIBED] = {
	&onHubEnter,
	&onHubOnline,
	&onSecond,
	&onChatTags
};

subsHandle subs[HOOKS_SUBSCRIBED];

Bool onLoad(uint32_t eventId, DCCorePtr core) {
	uint32_t i = 0;
	dcpp = core;

	hooks = (DCHooksPtr)core->query_interface(DCINTF_HOOKS, DCINTF_HOOKS_VER);
	logging = (DCLogPtr)core->query_interface(DCINTF_LOGGING, DCINTF_LOGGING_VER);

	hub = (DCHubPtr)core->query_interface(DCINTF_DCPP_HUBS, DCINTF_DCPP_HUBS_VER);
	tagger = (DCTaggerPtr)core->query_interface(DCINTF_DCPP_TAGGER, DCINTF_DCPP_TAGGER_VER);
	ui = (DCUIPtr)core->query_interface(DCINTF_DCPP_UI, DCINTF_DCPP_UI_VER); 

	if(!hooks || !init_cfg(core) || !logging || !hub || !tagger)
		return False;

	if(eventId == ON_INSTALL) {
		/* Default settings */
		set_cfg("SendSuffix", "<Plugins Test - " PLUGIN_NAME ">");
	}

	logging->log(PLUGIN_NAME " loaded...");

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
			"\t\t\t Help: " PLUGIN_NAME " \n"
			"\t /pluginhelp    \t Prints info about the purpose of this plugin\n"
			"\t /plugininfo    \t Prints info about this plugin\n"
			"\t /send <text>   \t Chat message test\n";

		hub->local_message(hHub, help, MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "pluginhelp") == 0) {
		const char* pluginhelp =
			"\t\t\t Plugin help: " PLUGIN_NAME " \n"
			"\t This plugin project is intended to both demonstrate the use and test the implementation of the core API\n"
			"\t as such the plugin itself does nothing useful but it can be used to verify whether an implementation works\n"
			"\t with the API or not, however, it is by no means intended to be a comprehensive testing tool for the API.\n";

		hub->local_message(hHub, pluginhelp, MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "plugininfo") == 0) {
		const char* info =
			"\t\t\t Plugin info: " PLUGIN_NAME " \n"
			"\t Name:           \t" PLUGIN_NAME "\n"
			"\t Author:         \t" PLUGIN_AUTHOR "\n"
			"\t Version:        \t" STRINGIZE(PLUGIN_VERSION) "\n"
			"\t Description:    \t" PLUGIN_DESC "\n"
			"\t GUID/UUID:      \t" PLUGIN_GUID "\n"
			"\t Core API Ver:   \t" STRINGIZE(DCAPI_CORE_VER) "\n";

		hub->local_message(hHub, info, MSG_SYSTEM);
		return True;
	} else if(stricmp(cmd->command, "send") == 0) {
		size_t len = strlen(cmd->params);
		if(len > 0) {
			char* suffix = get_cfg("SendSuffix");
			size_t msgLen = len + strlen(suffix) + 2;
			char* text = (char*)memset(malloc(msgLen), 0, msgLen);

			strncat(text, cmd->params, len);
			text[len] = ' ';
			strncat(text, suffix, msgLen - (len + 2));

			hub->send_message(hHub, text, (strnicmp(text, "/me ", 4) == 0) ? True : False);

			free(text);
			free(suffix);
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

Bool DCAPI onSecond(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak) {
	static uint64_t prevTick = 0;
	uint64_t tick = *(uint64_t*)pData;

	if(tick - prevTick >= 10*1000) {
		prevTick = tick;
		if(!ui) {
			char* buf = (char*)memset(malloc(128), 0, 128);
			snprintf(buf, 128, "*** Plugin timer tick (%lld)", tick);

			logging->log(buf);
			free(buf);
		} else ui->play_sound("Media\\tada.wav");
	}
	return False;
}

Bool DCAPI onChatTags(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak) {
	static const char* pattern = "ABC DEF";
	TagDataPtr tags = (TagDataPtr)pData;
	char* text = strstr(tags->text, pattern);

	while(text != NULL) {	
		tagger->add_tag(tags, text - tags->text, (text - tags->text) + strlen(pattern), "b", "");
		text = strstr(text + strlen(pattern), pattern);
	}

	text = strstr(tags->text, "***");
	if (text != NULL && (text - tags->text) == 0)
		tagger->add_tag(tags, 0, strlen(tags->text), "b", "");

	return False;
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
			/* Note: core may be NULL for this call */
			return dialog_create(pData, dcpp);
		default: return False;
	}
}
