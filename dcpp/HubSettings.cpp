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

#include "stdinc.h"
#include "HubSettings.h"

namespace dcpp {

void HubSettings::merge(const HubSettings& sub) {
	if(!sub.nick.empty()) { nick = sub.nick; }
	if(!sub.description.empty()) { description = sub.description; }
	if(!sub.email.empty()) { email = sub.email; }
	if(!sub.userIp.empty()) { userIp = sub.userIp; }
	if(!indeterminate(sub.showJoins)) { showJoins = sub.showJoins; }
	if(!indeterminate(sub.favShowJoins)) { favShowJoins = sub.favShowJoins; }
}

void HubSettings::load(SimpleXML& xml) {
	nick = xml.getChildAttrib("Nick");
	description = xml.getChildAttrib("UserDescription"); // not "Description" for compat with prev fav hub lists
	email = xml.getChildAttrib("Email");
	userIp = xml.getChildAttrib("UserIp");
	showJoins = to3bool(xml.getIntChildAttrib("ShowJoins"));
	favShowJoins = to3bool(xml.getIntChildAttrib("FavShowJoins"));
}

void HubSettings::save(SimpleXML& xml) const {
	if(!nick.empty()) { xml.addChildAttrib("Nick", nick); }
	if(!description.empty()) { xml.addChildAttrib("UserDescription", description); }
	if(!email.empty()) { xml.addChildAttrib("Email", email); }
	if(!userIp.empty()) { xml.addChildAttrib("UserIp", userIp); }
	if(!indeterminate(showJoins)) { xml.addChildAttrib("ShowJoins", toInt(showJoins)); }
	if(!indeterminate(favShowJoins)) { xml.addChildAttrib("FavShowJoins", toInt(favShowJoins)); }
}

} // namespace dcpp
