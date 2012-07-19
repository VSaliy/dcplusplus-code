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

#ifndef VERSION_H
#define VERSION_H

#ifdef WITH_LUAJIT
# define REVISION_GUID "{139e5bad-9998-44ae-8564-479c7886edff}"
# define LUAJIT " (LuaJIT)"
# define LUAJIT_LONG " Using " LUAJIT_VERSION " from " LUAJIT_URL "."
# define LUA_DIST LUAJIT_VERSION
#else
# define LUAJIT
# define LUAJIT_LONG
# define REVISION_GUID "{6a8a51d8-50a9-4155-aa4c-8569c7aaf85b}"
# define LUA_DIST LUA_RELEASE
#endif

/* UUID/GUID for this plugin project */
#define PLUGIN_GUID REVISION_GUID

/* Name of the plugin */
#define PLUGIN_NAME "Scripting plugin" LUAJIT

/* Author of the plugin */
#define PLUGIN_AUTHOR "DC++"

/* Short description of the plugin */
#define PLUGIN_DESC "Adds scripting support (supported languages: Lua)."

/* Version of the plugin (note: not API version) */
#define PLUGIN_VERSION 1.0

/* Plugin website, set to "N/A" if none */
#define PLUGIN_WEB "http://dcplusplus.sourceforge.net/"

#endif // VERSION_H
