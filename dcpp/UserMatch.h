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

#ifndef DCPLUSPLUS_DCPP_USER_MATCH_H
#define DCPLUSPLUS_DCPP_USER_MATCH_H

#include "forward.h"
#include "Flags.h"
#include "Pointer.h"
#include "StringSearch.h"

#include <string>
#include <vector>

#include <boost/regex.hpp>
#include <boost/variant.hpp>

namespace dcpp {

using std::string;
using std::vector;

/** Defines rules to match users. */
struct UserMatch : public Flags {
	enum {
		FAVS = 1 << 1,
		OPS = 1 << 2,
		BOTS = 1 << 3
	};

	string name;

	struct Rule {
		enum {
			NICK,
			CID,
			IP,

			FIELD_LAST
		} field;

		string pattern;

		bool isRegEx() const;
		void setRegEx(bool b);

	private:
		friend struct UserMatch;
		bool prepare();
		bool match(const string& str) const;
		boost::variant<StringSearch, boost::regex> search;
	};

	vector<Rule> rules;

	UserMatchPropsPtr props;

	void addRule(Rule&& rule);
	bool empty() const;

	bool match(Identity& identity) const;
};

/** Stores properties to be applied to matched users. */
struct UserMatchProps : public intrusive_ptr_base<UserMatchProps> {
	bool noChat;

	string font;
	int textColor;
	int bgColor;
};

} // namespace dcpp

#endif
