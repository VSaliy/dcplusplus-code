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
#include "ScriptInstance.h"

#include "Util.h"
#include "LuaExec.h"
#include "version.h"

lua_State* ScriptInstance::L = NULL;
CriticalSection ScriptInstance::cs;

bool ScriptInstance::MakeCallRaw(const string& table, const string& method, int args, int ret) throw() {
	auto stack_pre = lua_gettop(L) - args;
	lua_getglobal(L, table.c_str());		// args + 1
	if (lua_istable(L, -1)) {
		lua_pushstring(L, method.c_str());	// args + 2
		lua_gettable(L, -2);				// args + 2
		lua_remove(L, -2);					// args + 1
		lua_insert(L, stack_pre + 1);		// args + 1
		if(lua_pcall(L, args, ret, 0) == 0) {
			dcassert(lua_gettop(L) - stack_pre == ret);
			return true;
		}
		const char *msg = lua_tostring(L, -1);
		Util::logMessage((msg != NULL) ? string("Lua Error calling ") + table + "." + method + ": " + msg : string("Lua Error: (unknown)"));
		lua_pop(L, 1);
	} else {
		// Abandon to garbage collector arguments pushed to function in table
		// that doesn't exist, up to point in stack where this instance of Lua
		// (in a possibly re-entrant manner) started building.
		lua_settop(L, stack_pre);
	}
	dcassert(lua_gettop(L) == stack_pre);
	return false;
}

// Check if lua function is defined
bool ScriptInstance::CheckFunction(const string& table, const string& method) {
	Lock l(cs);

	lua_getglobal(L, table.c_str());
	lua_pushstring(L, method.c_str());
	if(lua_istable(L, -2)) {
		lua_gettable(L, -2);
		lua_remove(L, -2);
		if(lua_isfunction(L, -1)) {
			lua_pop(L, 1);
			return true;
		}
	}

	lua_settop(L, 0);
	return false;
}

// Get value from top of stack, check if should cancel message.
Bool ScriptInstance::GetLuaBool() {
	bool ret = false;
	if (lua_gettop(L) > 0) {
		ret = !lua_isnil(L, -1);
		lua_pop(L, 1);
	}

	return ret ? True : False;
}

void ScriptInstance::EvaluateChunk(const string& chunk) {
	Lock l(cs);
	lua_dostring(L, chunk.c_str());
}

void ScriptInstance::EvaluateFile(const string& fn) {
	Lock l(cs);
	lua_dofile(L, (fn[1] == ':' || fn[0] == '/') ? fn.c_str() : Util::fromUtf8(SETTING(ScriptPath) + fn).c_str());
}
