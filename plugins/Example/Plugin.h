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

#ifndef PLUGIN_H
#define PLUGIN_H

#ifdef _WIN32
extern HINSTANCE hInst;
#endif

/* Event handlers */
Bool DCAPI onHubEnter(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak);
Bool DCAPI onHubOnline(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak);
Bool DCAPI onSecond(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak);
Bool DCAPI onChatTags(dcptr_t pObject, dcptr_t pData, dcptr_t opaque, Bool* bBreak);

/* Plugin main function */
Bool DCAPI pluginMain(PluginState state, DCCorePtr core, dcptr_t pData);

#endif /* PLUGIN_H */
