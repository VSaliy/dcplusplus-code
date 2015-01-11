/*
 * Copyright (C) 2001-2015 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_CONNECTION_MANAGER_H
#define DCPLUSPLUS_DCPP_CONNECTION_MANAGER_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "BufferedSocket.h"
#include "ConnectionManagerListener.h"
#include "ConnectionType.h"
#include "CriticalSection.h"
#include "HintedUser.h"
#include "Singleton.h"
#include "TimerManager.h"
#include "UserConnectionListener.h"

namespace dcpp {

using std::unique_ptr;
using std::unordered_map;
using std::vector;

class SocketException;

class ConnectionQueueItem {
public:
	enum State {
		CONNECTING,					// Recently sent request to connect
		WAITING,					// Waiting to send request to connect
		NO_DOWNLOAD_SLOTS,			// Not needed right now
		ACTIVE						// In one up/downmanager
	};

	ConnectionQueueItem(const HintedUser& user, ConnectionType type);

	GETSET(string, token, Token);
	GETSET(uint64_t, lastAttempt, LastAttempt);
	GETSET(int, errors, Errors); // Number of connection errors, or -1 after a protocol error
	GETSET(State, state, State);
	GETSET(ConnectionType, type, Type);

	const HintedUser& getUser() const { return user; }

	bool operator==(const ConnectionQueueItem& rhs) const;
	bool operator==(const UserPtr& user) const;

private:
	HintedUser user;
};

class ExpectedMap {
public:
	void add(const string& aNick, const string& aMyNick, const string& aHubUrl) {
		Lock l(cs);
		expectedConnections.emplace(aNick, make_pair(aMyNick, aHubUrl));
	}

	StringPair remove(const string& aNick) {
		Lock l(cs);
		auto i = expectedConnections.find(aNick);

		if(i == expectedConnections.end())
			return make_pair(Util::emptyString, Util::emptyString);

		StringPair tmp = i->second;
		expectedConnections.erase(i);

		return tmp;
	}

private:
	/** Nick -> myNick, hubUrl for expected NMDC incoming connections */
	typedef unordered_map<string, StringPair> ExpectMap;
	ExpectMap expectedConnections;

	CriticalSection cs;
};

class ConnectionManager :
	public Singleton<ConnectionManager>,
	public Speaker<ConnectionManagerListener>,
	private TimerManagerListener,
	private UserConnectionListener
{
public:
	string makeToken() const;
	void addToken(const string& token, const OnlineUser& user, ConnectionType type);

	void nmdcExpect(const string& aNick, const string& aMyNick, const string& aHubUrl) {
		expectedConnections.add(aNick, aMyNick, aHubUrl);
	}

	void nmdcConnect(const string& aServer, const string& aPort, const string& aMyNick, const string& hubUrl, const string& encoding);
	void adcConnect(const OnlineUser& aUser, const string& aPort, const string& aToken, bool secure);
	void adcConnect(const OnlineUser& aUser, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure);

	void getDownloadConnection(const HintedUser& aUser);
	void force(const UserPtr& aUser);

	void disconnect(const UserPtr& user); // disconnect all transfers for the user
	void disconnect(const UserPtr& user, ConnectionType type);

	void shutdown();

	/** Find a suitable port to listen on, and start doing it */
	void listen();
	void disconnect() noexcept;

	const string& getPort() const;
	const string& getSecurePort() const;

private:

	class Server : public Thread {
	public:
		Server(bool secure, const string& port_, const string& ip);
		virtual ~Server() { die = true; join(); }

		const string& getPort() const { return port; }

	private:
		virtual int run() noexcept;

		Socket sock;
		string port;
		bool secure;
		bool die;
	};

	friend class Server;

	mutable CriticalSection cs;

	/** All ConnectionQueueItems */
	vector<ConnectionQueueItem> cqis[CONNECTION_TYPE_LAST],
		&downloads; // shortcut

	/** All active connections */
	UserConnectionList userConnections;

	StringList features;
	StringList adcFeatures;

	unordered_map<string, pair<CID, ConnectionType>> tokens;

	ExpectedMap expectedConnections;

	uint32_t floodCounter;

	unique_ptr<Server> server;
	unique_ptr<Server> secureServer;

	bool shuttingDown;

	friend class Singleton<ConnectionManager>;
	ConnectionManager();

	virtual ~ConnectionManager() { shutdown(); }

	UserConnection* getConnection(bool aNmdc, bool secure) noexcept;
	void putConnection(UserConnection* aConn);

	void addDownloadConnection(UserConnection* uc);
	void addNewConnection(UserConnection* uc, ConnectionType type);

	ConnectionQueueItem& getCQI(const HintedUser& user, ConnectionType type);
	void putCQI(ConnectionQueueItem& cqi);

	void accept(const Socket& sock, bool secure) noexcept;

	bool checkKeyprint(UserConnection* aSource);
	pair<bool, ConnectionType> checkToken(const UserConnection* uc) const;
	bool checkDownload(const UserConnection* uc) const;

	void failed(UserConnection* aSource, const string& aError, bool protocolError);

	// UserConnectionListener
	virtual void on(Connected, UserConnection*) noexcept;
	virtual void on(Failed, UserConnection*, const string&) noexcept;
	virtual void on(ProtocolError, UserConnection*, const string&) noexcept;
	virtual void on(CLock, UserConnection*, const string&, const string&) noexcept;
	virtual void on(Key, UserConnection*, const string&) noexcept;
	virtual void on(Direction, UserConnection*, const string&, const string&) noexcept;
	virtual void on(MyNick, UserConnection*, const string&) noexcept;
	virtual void on(Supports, UserConnection*, const StringList&) noexcept;

	virtual void on(AdcCommand::SUP, UserConnection*, const AdcCommand&) noexcept;
	virtual void on(AdcCommand::INF, UserConnection*, const AdcCommand&) noexcept;
	virtual void on(AdcCommand::STA, UserConnection*, const AdcCommand&) noexcept;

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
	virtual void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;

};

} // namespace dcpp

#endif // !defined(CONNECTION_MANAGER_H)
