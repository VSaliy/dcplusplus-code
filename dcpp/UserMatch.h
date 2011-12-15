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
#include "StringMatch.h"

#include <string>
#include <vector>

#include <boost/regex.hpp>
#include <boost/variant.hpp>

namespace dcpp {

using std::string;
using std::vector;

struct Style {
	string font;
	int textColor;
	int bgColor;

	Style() : textColor(-1), bgColor(-1) { }
};

/** Defines rules to match users. */
struct UserMatch : public Flags {
	enum {
		GENERATED_BIT,
		FAVS_BIT,
		OPS_BIT,
		BOTS_BIT,
		FORCE_CHAT_BIT,
		IGNORE_CHAT_BIT
	};

	enum {
		GENERATED = 1 << GENERATED_BIT, /** Generated by DC++. Matchers that the user touches
										become precious and cannot be modified anymore by DC++ when
										automating actions. */

		FAVS = 1 << FAVS_BIT,
		OPS = 1 << OPS_BIT,
		BOTS = 1 << BOTS_BIT,

		FORCE_CHAT = 1 << FORCE_CHAT_BIT,
		IGNORE_CHAT = 1 << IGNORE_CHAT_BIT
	};

	string name;

	struct Rule : StringMatch {
		enum {
			NICK,
			CID,
			IP,
			HUB_ADDRESS,

			FIELD_LAST
		} field;

		bool operator==(const Rule& rhs) const;
	};

	vector<Rule> rules;

	Style style;

	void addRule(Rule&& rule);
	bool empty() const;

	bool match(OnlineUser& user) const;
};

} // namespace dcpp

#endif
