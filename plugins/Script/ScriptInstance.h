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

#ifndef SCRIPT_INSTANCE_H
#define SCRIPT_INSTANCE_H

#include <CriticalSection.h>
#include <debug.h>

#include "LuaManager.h"

using dcpp::CriticalSection;
using dcpp::Lock;

using std::string;

class ScriptInstance {
	bool MakeCallRaw(const string& table, const string& method, int args, int ret) throw();

	void LuaPush(int i) { lua_pushnumber(L, i); }
	void LuaPush(const string& s) { lua_pushlstring(L, s.data(), s.size()); }
	void LuaPush(void* p) { lua_pushlightuserdata(L, p); }
protected:
	ScriptInstance() { }
	virtual ~ScriptInstance() { }

	template <typename T>
	bool MakeCall(const string& table, const string& method, int ret, const T& t) throw() {
		Lock l(cs);
		LuaPush(t);
		return MakeCallRaw(table, method, 1, ret);
	}

	template <typename T, typename T2>
	bool MakeCall(const string& table, const string& method, int ret, const T& t, const T2& t2) throw() {
		Lock l(cs);
		LuaPush(t);
		LuaPush(t2);
		return MakeCallRaw(table, method, 2, ret);
	}

	bool CheckFunction(const string& table, const string& method);
	string GetHubType(HubDataPtr aHub) { return (aHub->protocol == PROTOCOL_ADC ? "adch" : "nmdch"); }
	Bool GetLuaBool();

	static lua_State* L;
	static CriticalSection cs;
public:
	static void EvaluateChunk(const string& chunk);
	static void EvaluateFile(const string& fn);
};

#endif // SCRIPT_INSTANCE_H
