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

#ifndef LUA_MANAGER_H
#define LUA_MANAGER_H

#ifdef WITH_LUAJIT
# include <lua.hpp>
# define lualib_h
#endif

#include "lunar.h"

class LuaManager 
{
public:
	static const char className[];
	static Lunar<LuaManager>::RegType methods[];

	LuaManager(lua_State* /* L */) { }
	int SendClientMessage(lua_State* L);
	int SendHubMessage(lua_State* L);
	int GenerateDebugMessage(lua_State* L);
	int GetHubIpPort(lua_State* L);
	int GetHubUrl(lua_State* L);
	int GetClientIp(lua_State* L);
	int SendUDPPacket(lua_State* L);
	int InjectHubMessage(lua_State* L);
	int DropUserConnection(lua_State* L);

	int CreateClient(lua_State* L);
	int DeleteClient(lua_State* L);

	int RunTimer(lua_State* L);

	int GetSetting(lua_State* L);
	int GetAppPath(lua_State* L);
	int GetConfigPath(lua_State* L);
	int GetScriptPath(lua_State* L);

	int ToUtf8(lua_State* L);
	int FromUtf8(lua_State* L);

	int NotImpl(lua_State* L) { LuaError(L, "Call to unimplemented function", false); return 0; };

private:
	void LuaError(lua_State* L, const char* fmt, bool fatal = true, ...);
};

#endif // LUA_MANAGER_H
