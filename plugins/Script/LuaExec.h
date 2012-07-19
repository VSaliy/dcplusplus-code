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

#ifndef LUA_EXEC_H
#define LUA_EXEC_H

static void callalert (lua_State *L, int status) {
	if (status != 0) {
		lua_getglobal(L, "_ALERT");
		if (lua_isfunction(L, -1)) {
			lua_insert(L, -2);
			lua_call(L, 1, 0);
		}
		else {  /* no _ALERT function; print it on stderr */
			Util::logMessage(Util::toUtf8(string("Lua ERROR: ") + lua_tostring(L, -2)));
			lua_pop(L, 2);  /* remove error message and _ALERT */
		}
	}
}

static int aux_do (lua_State *L, int status) {
  if (status == 0) {  /* parse OK? */
    status = lua_pcall(L, 0, LUA_MULTRET, 0);  /* call main */
  }
  callalert(L, status);
  return status;
}

LUALIB_API int lua_dofile (lua_State *L, const char *filename) {
  return aux_do(L, luaL_loadfile(L, filename));
}

LUALIB_API int lua_dobuffer (lua_State *L, const char *buff, size_t size,
                          const char *name) {
  return aux_do(L, luaL_loadbuffer(L, buff, size, name));
}

LUALIB_API int lua_dostring (lua_State *L, const char *str) {
  return lua_dobuffer(L, str, strlen(str), str);
}

#endif // LUA_EXEC_H
