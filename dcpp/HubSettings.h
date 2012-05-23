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

#ifndef DCPLUSPLUS_DCPP_HUBSETTINGS_H
#define DCPLUSPLUS_DCPP_HUBSETTINGS_H

#include <string>

#include "GetSet.h"
#include "SimpleXML.h"
#include "tribool.h"

namespace dcpp {

using std::string;

/** Stores settings to be applied to a hub. There are 3 HubSettings levels in DC++: global; per
favorite hub group; per favorite hub entry. */
struct HubSettings
{
	HubSettings () : showJoins(indeterminate), favShowJoins(indeterminate) { }

	/** Apply a set of sub-settings that may override current ones. Strings are overridden when not
	null. Tribools are overridden when not in an indeterminate state. */
	void merge(const HubSettings& sub);

	void load(SimpleXML& xml);
	void save(SimpleXML& xml) const;

	GETSET(string, nick, Nick);
	GETSET(string, description, Description);
	GETSET(string, email, Email);
	GETSET(string, userIp, UserIp);

	/* don't forget to init new tribools to indeterminate in the constructor! they default to false
	otherwise. */
	tribool showJoins;
	tribool favShowJoins;

	friend class Client;
};

} // namespace dcpp

#endif
