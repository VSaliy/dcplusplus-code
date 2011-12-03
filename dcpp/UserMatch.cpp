/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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
#include "UserMatch.h"

#include "FavoriteManager.h"
#include "format.h"
#include "LogManager.h"
#include "OnlineUser.h"

namespace dcpp {

bool UserMatch::Rule::isRegEx() const {
	return boost::get<boost::regex>(&search);
}

void UserMatch::Rule::setRegEx(bool b) {
	if(b)
		search = boost::regex();
	else
		search = StringSearch();
}
	
struct Prepare : boost::static_visitor<bool> {
	Prepare(const string& pattern) : pattern(pattern) { }

	bool operator()(StringSearch& s) const {
		s = pattern;
		return true;
	}

	bool operator()(boost::regex& r) const {
		try {
			r.assign(pattern);
			return true;
		} catch(const std::runtime_error&) {
			LogManager::getInstance()->message(str(F_("Invalid user matching regular expression: %1%") % pattern));
			return false;
		}
	}

private:
	const string& pattern;
};

bool UserMatch::Rule::prepare() {
	return !pattern.empty() && boost::apply_visitor(Prepare(pattern), search);
}

struct Match : boost::static_visitor<bool> {
	Match(const string& str) : str(str) { }

	bool operator()(const StringSearch& s) const {
		return s.match(str);
	}

	bool operator()(const boost::regex& r) const {
		try {
			return !r.empty() && boost::regex_search(str, r);
		} catch(const std::runtime_error&) {
			// most likely a stack overflow, ignore...
			return false;
		}
	}

private:
	const string& str;
};

bool UserMatch::Rule::match(const string& str) const {
	return boost::apply_visitor(Match(str), search);
}

void UserMatch::addRule(Rule&& rule) {
	if(rule.prepare()) {
		rules.push_back(std::forward<Rule>(rule));
	}
}

bool UserMatch::empty() const {
	return !isSet(FAVS) && !isSet(OPS) && !isSet(BOTS) && rules.empty();
}

bool UserMatch::match(Identity& identity) const {
	if(isSet(FAVS) && !FavoriteManager::getInstance()->isFavoriteUser(identity.getUser())) {
		return false;
	}

	if(isSet(OPS) && !identity.isOp()) {
		return false;
	}

	if(isSet(BOTS) && !identity.isBot()) {
		return false;
	}

	for(auto i = rules.cbegin(), iend = rules.cend(); i != iend; ++i) {
		string str;
		switch(i->field) {
		case UserMatch::Rule::NICK: str = identity.getNick(); break;
		case UserMatch::Rule::CID: str = identity.getUser()->getCID().toBase32(); break;
		case UserMatch::Rule::IP: str = identity.getIp(); break;
		}
		if(!i->match(str)) {
			return false;
		}
	}

	identity.match = props;
	return true;
}

} // namespace dcpp
