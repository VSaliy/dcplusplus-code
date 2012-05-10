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

#ifndef DCPLUSPLUS_WIN32_DEBUG_FRAME_H
#define DCPLUSPLUS_WIN32_DEBUG_FRAME_H

#include <dcpp/Client.h>
#include <dcpp/ClientManagerListener.h>
#include <dcpp/DebugManager.h>
#include <dcpp/Semaphore.h>

#include <deque>

#include "StaticFrame.h"

using std::deque;

class DebugFrame : public StaticFrame<DebugFrame>, public Thread,
	private DebugManagerListener,
	private ClientManagerListener
{
	typedef StaticFrame<DebugFrame> BaseType;
public:
	enum Status {
		STATUS_STATUS,
		STATUS_CLIENT,		
		STATUS_HUB,
		STATUS_HUB_ADDRESS,
		STATUS_LAST
	};
	
	static const string id;
	const string& getId() const;
	
	void addLine(const tstring& msg);

private:
	bool stop;
	CriticalSection cs;
	Semaphore s;
	deque<string> cmdList;

	int run() {
		setThreadPriority(Thread::LOW);
		string x = Util::emptyString;
		stop = false;

		while(true) {
			s.wait();
			if(stop)
				break;

			{
				Lock l(cs);
				if(cmdList.empty()) continue;

				x = cmdList.front();
				cmdList.pop_front();
			}
			addLine(Text::toT(x));
		}
		
		stop = false;
		return 0;
	}

	void addDbgLine(const string& cmd) {
		{
			Lock l(cs);
			cmdList.push_back(cmd);
		}
		s.signal();
	}

	struct HubInfo : public FastAlloc<HubInfo> {
		HubInfo(const tstring& aIp) : ip(aIp) { }
		HubInfo(Client* client) : ip(Text::toT(client->getIpPort())) { }

		tstring ip;
	};

	friend class StaticFrame<DebugFrame>;
	friend class MDIChildFrame<DebugFrame>;

	GridPtr grid;
	
	TextBoxPtr debug;
	
	CheckBoxPtr clientMsg, hubMsg, filterHub;
	bool showClientMsg, showHubMsg, filterByHub;

	ComboBoxPtr hubs;
	
	DebugFrame(TabViewPtr parent);
	virtual ~DebugFrame();

	void layout();
	void updateStatus();
	bool preClosing();
	
	void clearMessages();
	void handleClientMsg();
	void handleHubMsg();
	void handleFilterHub();

	bool handleKeyDown(int c);
	
	void on(DebugManagerListener::DebugCommand, const string& aLine, int cmdType, const string& ip) throw() {
			auto url = hubs->getText();
			switch(cmdType) {
				case DebugManager::HUB_IN:
						if(!showHubMsg)
							return;
						if(!filterByHub || Text::toT(ip) == url) {
							addDbgLine("From Hub:\t\t<" + ip + ">\t \t" + aLine);
						}
					break;
				case DebugManager::HUB_OUT:
						if(!showHubMsg)
							return;
						if(!filterByHub || Text::toT(ip) == url) {
							addDbgLine("To Hub:\t\t<" + ip + ">\t \t" + aLine);
						}
					break;
				case DebugManager::CLIENT_IN:
						if(!showClientMsg)
							return;
						if(!filterByHub || Text::toT(ip) == url) {
							addDbgLine("From Client:\t\t<" + ip + ">\t \t" + aLine);
						}
					break;
				case DebugManager::CLIENT_OUT:
						if(!showClientMsg)
							return;
						if(!filterByHub || Text::toT(ip) == url) {
							addDbgLine("To Client:\t\t<" + ip + ">\t \t" + aLine);
						}
					break;
				default: dcassert(0);
			}
	}

	// ClientManagerListener
	virtual void on(ClientConnected, Client* c) noexcept;
	virtual void on(ClientDisconnected, Client* c) noexcept;

	void onHubAdded(HubInfo* info);
	void onHubRemoved(HubInfo* info);

	// DebugManagerListener
	virtual void on(DebugCommand, const string& message) noexcept;

};

#endif
