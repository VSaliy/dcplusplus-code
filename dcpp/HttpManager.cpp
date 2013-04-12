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

void HttpManager::shutdown() {
	Lock l(cs);
	for(auto& conn: conns) { delete conn.c; }
	conns.clear();
}

HttpConnection* HttpManager::makeConn(string&& url) {
	Lock l(cs);
	Conn conn { new HttpConnection(), string(), 0 };
	conns.push_back(move(conn));
	conn.c->addListener(this);
	conn.c->setUrl(move(url));
	fire(HttpManagerListener::Added(), conn.c);
	return conn.c;
}

HttpManager::Conn* HttpManager::findConn(HttpConnection* c) {
	for(auto& conn: conns) {
		if(conn.c == c) {
			return &conn;
		}
	}
	return nullptr;
}

void HttpManager::remove(Conn& conn) {
	conn.remove = GET_TICK() + 60 * 1000;
}

void HttpManager::on(HttpConnectionListener::Data, HttpConnection* c, const uint8_t* data, size_t len) noexcept {
	Lock l(cs);
	auto& buf = findConn(c)->buf;
	if(buf.empty() && c->getSize() != -1) {
		buf.reserve(c->getSize());
	}
	buf.append(reinterpret_cast<const char*>(data), len);
	fire(HttpManagerListener::Updated(), c);
}

void HttpManager::on(HttpConnectionListener::Failed, HttpConnection* c, const string& str) noexcept {
	auto& conn = *findConn(c);
	conn.buf.clear();
	fire(HttpManagerListener::Failed(), c, str);
	remove(conn);
}

void HttpManager::on(HttpConnectionListener::Complete, HttpConnection* c) noexcept {
	auto& conn = *findConn(c);
	fire(HttpManagerListener::Complete(), c, move(conn.buf));
	remove(conn);
}

void HttpManager::on(HttpConnectionListener::Redirected, HttpConnection* c) noexcept {
	fire(HttpManagerListener::Removed(), c);
	fire(HttpManagerListener::Added(), c);
}

void HttpManager::on(HttpConnectionListener::Retried, HttpConnection* c, bool connected) noexcept {
	if(connected) {
		findConn(c)->buf.clear();
	}
}

void HttpManager::on(TimerManagerListener::Minute, uint64_t tick) noexcept {
	Lock l(cs);
	conns.erase(std::remove_if(conns.begin(), conns.end(), [tick, this](const Conn& conn) -> bool {
		if(conn.remove && tick > conn.remove) {
			fire(HttpManagerListener::Removed(), conn.c);
			delete conn.c;
			return true;
		}
		return false;
	}), conns.end());
}

} // namespace dcpp
