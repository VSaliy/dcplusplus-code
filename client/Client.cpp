/* 
 * Copyright (C) 2001 Jacek Sieka, jacek@creatio.se
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
#include "DCPlusPlus.h"

#include "Socket.h"
#include "Client.h"
#include "CriticalSection.h"

Client::List Client::clientList;

ClientListener::List Client::staticListeners;
CriticalSection Client::staticListenersCS;

void Client::connect(const string& aServer, short aPort) {
	if(socket.isConnected()) {
		disconnect();
	}
	server = aServer;
	port = aPort;
	fireConnecting();
	socket.addListener(this);
	socket.connect(aServer, aPort);
}

/**
 * Reader thread. Read data from socket and calls getLine whenever there's data to be read.
 * Finishes whenever p->stopEvent is signaled
 * @param p Pointer to the Clientent that started the thread
 */

void Client::onLine(const string& aLine) {
	lastActivity = TimerManager::getTick();

	string cmd;
	string param;
	int x;

	if(aLine.length() == 0)
		return;
	
	if( (x = aLine.find(' ')) == string::npos) {
		cmd = aLine;
	} else {
		cmd = aLine.substr(0, x);
		param = aLine.substr(x+1);
	}
	
	if(cmd == "$Search") {
		string seeker = param.substr(0, param.find(' '));
		param = param.substr(param.find(' ') + 1);
		int a;
		if(param[0] == 'F') {
			a = SearchManager::SIZE_DONTCARE;
		} else if(param[2] == 'F') {
			a = SearchManager::SIZE_ATLEAST;
		} else {
			a = SearchManager::SIZE_ATMOST;
		}
		param=param.substr(4);
		string size = param.substr(0, param.find('?'));
		param = param.substr(param.find('?')+1);
		int type = atoi(param.substr(0, param.find('?')).c_str());
		param = param.substr(param.find('?')+1);
		fireSearch(seeker, a, size, type, param);
	} else if(cmd == "$ConnectToMe") {
		param = param.substr(param.find(' ') + 1);
		string server = param.substr(0, param.find(':'));
		fireConnectToMe(server, param.substr(param.find(':')+1));
	} else if(cmd == "$RevConnectToMe") {
		if(Settings::getConnectionType() == Settings::CONNECTION_ACTIVE) {
			User::NickIter i = users.find(param.substr(0, param.find(' ')));
			if(i != users.end()) {
				fireRevConnectToMe(i->second);
			}
		}
	} else if(cmd == "$HubName") {
		name = param;
		fireHubName();
	} else if(cmd == "$Lock") {
	
		string lock = param.substr(0, param.find(' '));
		string pk = param.substr(param.find(' ') + 4);
		fireLock(lock, pk);	
	} else if(cmd == "$Hello") {
		User* u;
		User::NickIter i = users.find(param);
		if(i == users.end()) {
			u = new User(param);
			u->setClient(this);
			users[param] = u;
		} else {
			u = i->second;
		}
		fireHello(u);
	} else if(cmd == "$ForceMove") {
		fireForceMove(param);
	} else if(cmd == "$HubIsFull") {
		fireHubFull();
	} else if(cmd == "$MyINFO") {
		string nick;
		param = param.substr(5);
		nick = param.substr(0, param.find(' '));
		param = param.substr(param.find(' ')+1);
		User* u;
		if(users.find(nick) == users.end()) {
			u = new User(nick);
			u->setClient(this);
			users[nick] = u;
		} else {
			u = users[nick];
		}

		u->setDescription(param.substr(0, param.find('$')));
		param = param.substr(param.find('$')+3);
		u->setConnection(param.substr(0, param.find('$')-1));
		param = param.substr(param.find('$')+1);
		u->setEmail(param.substr(0, param.find('$')));
		param = param.substr(param.find('$')+1);
		u->setBytesShared(param.substr(0, param.find('$')));
		
		fireMyInfo(u);
		
	} else if(cmd == "$Quit") {
		if(users.find(param) != users.end()) {
			User* u = users[param];
			users.erase(param);
			
			fireQuit(u);
			delete u;
		}
		
	} else if(cmd == "$ValidateDenide") {
		fireValidateDenied();
	} else if(cmd == "$NickList") {
		StringList v;
		int j;
		while( (j=param.find("$$")) != string::npos) {
			string nick = param.substr(0, j);
			
			if(users.find(nick) == users.end()) {
				User* u = new User(nick);
				u->setClient(this);
				users[nick] = u;
			}

			v.push_back(nick);
			param = param.substr(j+2);
		}
		
		fireNickList(v);
		
	} else if(cmd == "$OpList") {
		StringList v;
		int j;
		while( (j=param.find("$$")) != string::npos) {
			string nick = param.substr(0, j);
			if(users.find(nick) == users.end()) {
				User* u = new User(nick, User::FLAG_OP);
				u->setClient(this);
				users[nick] = u;
			}
			users[nick]->setFlag(User::FLAG_OP);
			v.push_back(nick);
			param = param.substr(j+2);
		}
		fireOpList(v);
	} else if(cmd == "$To:") {
		string tmp = param.substr(param.find("From:") + 6);
		string nick = tmp.substr(0, tmp.find("$") - 1);
		tmp = tmp.substr(tmp.find("$") + 1);
		firePrivateMessage(nick, tmp);
	} else if(cmd == "$") {
		fireUnknown(aLine);
	} else {
		fireMessage(aLine);
	}
}


/**
 * @file Client.cpp
 * $Id: Client.cpp,v 1.8 2001/12/13 19:21:57 arnetheduck Exp $
 * @if LOG
 * $Log: Client.cpp,v $
 * Revision 1.8  2001/12/13 19:21:57  arnetheduck
 * A lot of work done almost everywhere, mainly towards a friendlier UI
 * and less bugs...time to release 0.06...
 *
 * Revision 1.7  2001/12/12 00:06:04  arnetheduck
 * Updated the public hub listings, fixed some minor transfer bugs, reworked the
 * sockets to use only one thread (instead of an extra thread for sending files),
 * and fixed a major bug in the client command decoding (still have to fix this
 * one for the userconnections...)
 *
 * Revision 1.6  2001/12/08 14:25:49  arnetheduck
 * More bugs removed...did my first search as well...
 *
 * Revision 1.5  2001/12/07 20:03:02  arnetheduck
 * More work done towards application stability
 *
 * Revision 1.4  2001/12/04 21:50:34  arnetheduck
 * Work done towards application stability...still a lot to do though...
 * a bit more and it's time for a new release.
 *
 * Revision 1.3  2001/12/01 17:15:03  arnetheduck
 * Added a crappy version of huffman encoding, and some other minor changes...
 *
 * Revision 1.2  2001/11/29 19:10:54  arnetheduck
 * Refactored down/uploading and some other things completely.
 * Also added download indicators and download resuming, along
 * with some other stuff.
 *
 * Revision 1.1  2001/11/27 22:10:08  arnetheduck
 * Renamed DCClient* to Client*
 *
 * Revision 1.5  2001/11/26 23:40:36  arnetheduck
 * Downloads!! Now downloads are possible, although the implementation is
 * likely to change in the future...more UI work (splitters...) and some bug
 * fixes. Only user file listings are downloadable, but at least it's something...
 *
 * Revision 1.4  2001/11/25 22:06:25  arnetheduck
 * Finally downloading is working! There are now a few quirks and bugs to be fixed
 * but what the heck....!
 *
 * Revision 1.3  2001/11/24 10:34:02  arnetheduck
 * Updated to use BufferedSocket instead of handling threads by itself.
 *
 * Revision 1.2  2001/11/22 19:47:42  arnetheduck
 * A simple XML parser. Doesn't have all the features, but works good enough for
 * the configuration file.
 *
 * Revision 1.1.1.1  2001/11/21 17:33:20  arnetheduck
 * Inital release
 *
 * @endif
 */

