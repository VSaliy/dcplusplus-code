/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "HttpManager.h"

#include "format.h"
#include "HttpConnection.h"

namespace dcpp {

HttpManager::HttpManager() {
	TimerManager::getInstance()->addListener(this);
}

HttpManager::~HttpManager() {
	TimerManager::getInstance()->removeListener(this);
}

HttpConnection* HttpManager::download(string url) {
	auto conn = makeConn(move(url));
	conn->download();
	return conn;
}

HttpConnection* HttpManager::download(string url, const StringMap& postData) {
	auto conn = makeConn(move(url));
	conn->download(postData);
	return conn;
}

void HttpManager::disconnect(const string& url) {
	HttpConnection* c = nullptr;

	{
		Lock l(cs);
		conns.erase(std::remove_if(conns.begin(), conns.end(), [&](const Conn& conn) -> bool {
			if(conn.c->getUrl() == url) {
				c = conn.c;
				return true;
			}
			return false;
		}), conns.end());
	}

	if(c) {
		fire(HttpManagerListener::Failed(), c, _("Disconnected"));
		fire(HttpManagerListener::Removed(), c);
		delete c;
	}
}

void HttpManager::shutdown() {
	Lock l(cs);
	for(auto& conn: conns) { delete conn.c; }
	conns.clear();
}

HttpConnection* HttpManager::makeConn(string&& url) {
	auto c = new HttpConnection();
	{
		Lock l(cs);
		Conn conn { c, string(), 0 };
		conns.push_back(move(conn));
	}
	c->addListener(this);
	c->setUrl(move(url));
	fire(HttpManagerListener::Added(), c);
	return c;
}

HttpManager::Conn* HttpManager::findConn(HttpConnection* c) {
	for(auto& conn: conns) {
		if(conn.c == c) {
			return &conn;
		}
	}
	return nullptr;
}

void HttpManager::removeLater(HttpConnection* c) {
	Lock l(cs);
	findConn(c)->remove = GET_TICK() + 60 * 1000;
}

void HttpManager::on(HttpConnectionListener::Data, HttpConnection* c, const uint8_t* data, size_t len) noexcept {
	{
		Lock l(cs);
		auto& buf = findConn(c)->buf;
		if(buf.empty() && c->getSize() != -1) {
			buf.reserve(c->getSize());
		}
		buf.append(reinterpret_cast<const char*>(data), len);
	}
	fire(HttpManagerListener::Updated(), c);
	printf("size: %d\n", c->getSize());
}

void HttpManager::on(HttpConnectionListener::Failed, HttpConnection* c, const string& str) noexcept {
	{
		Lock l(cs);
		findConn(c)->buf.clear();
	}
	fire(HttpManagerListener::Failed(), c, str);
	removeLater(c);
}

void HttpManager::on(HttpConnectionListener::Complete, HttpConnection* c) noexcept {
	string buf;
	{
		Lock l(cs);
		buf = move(findConn(c)->buf);
	}
	fire(HttpManagerListener::Complete(), c, move(buf));
	removeLater(c);
}

void HttpManager::on(HttpConnectionListener::Redirected, HttpConnection* c) noexcept {
	fire(HttpManagerListener::Removed(), c);
	fire(HttpManagerListener::Added(), c);
}

void HttpManager::on(HttpConnectionListener::Retried, HttpConnection* c, bool connected) noexcept {
	if(connected) {
		Lock l(cs);
		findConn(c)->buf.clear();
	}
}

void HttpManager::on(TimerManagerListener::Minute, uint64_t tick) noexcept {
	vector<HttpConnection*> removed;

	{
		Lock l(cs);
		conns.erase(std::remove_if(conns.begin(), conns.end(), [tick, &removed](const Conn& conn) -> bool {
			if(conn.remove && tick > conn.remove) {
				removed.push_back(conn.c);
				return true;
			}
			return false;
		}), conns.end());
	}

	for(auto c: removed) {
		fire(HttpManagerListener::Removed(), c);
		delete c;
	}
}

} // namespace dcpp
