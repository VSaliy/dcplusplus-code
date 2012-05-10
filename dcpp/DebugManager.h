/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined DCPLUSPLUS_DCPP_DEBUGMANAGER_H
#define DCPLUSPLUS_DCPP_DEBUGMANAGER_H

#pragma once

#include "DCPlusPlus.h"
#include "Singleton.h"
#include "TimerManager.h"

namespace dcpp {

class DebugManagerListener {
public:
template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> DebugCommand;

	virtual void on(DebugCommand, const string&, int, const string&) noexcept { }

};

class DebugManager : public Singleton<DebugManager>, public Speaker<DebugManagerListener>
{
	friend class Singleton<DebugManager>;
	DebugManager() { };
public:
	void SendCommandMessage(const string& mess, int cmdType, const string& ip) {
		fire(DebugManagerListener::DebugCommand(), mess, cmdType, ip);
	}
	~DebugManager() { };
	enum {
		HUB_IN, HUB_OUT, CLIENT_IN, CLIENT_OUT
	};
};
#define COMMAND_DEBUG(a,b,c) DebugManager::getInstance()->SendCommandMessage(a,b,c);

} // namespace dcpp

#endif // !defined(DEBUG_MANAGER_H)
