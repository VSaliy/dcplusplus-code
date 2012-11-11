/*
 * Copyright (C) 2012 Jacek Sieka, arnetheduck on gmail point com
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

/* Helpers around the DCUtils interface. */

#include "Util.h"

#include "Core.h"

namespace dcapi {

DCUtilsPtr Util::utils;

bool Util::init() {
	if(!Core::handle()) { return false; }
	init(reinterpret_cast<DCUtilsPtr>(Core::handle()->query_interface(DCINTF_DCPP_UTILS, DCINTF_DCPP_UTILS_VER)));
	return utils;
}
void Util::init(DCUtilsPtr coreUtils) { utils = coreUtils; }
DCUtilsPtr Util::handle() { return utils; }

#ifdef _UNICODE
string Util::fromT(const wstring& str) {
	string res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->wcs_to_utf8(&res[0], &str[0], res.size()));
	return res;
}

wstring Util::toT(const string& str) {
	wstring res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->utf8_to_wcs(&res[0], &str[0], str.size()));
	return res;
}

string Util::fromT(const string& str) { return str; }
wstring Util::toT(const wstring& str) { return str; }
#endif // _UNICODE

string Util::fromUtf8(const string& str) {
	string res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->from_utf8(&res[0], &str[0], res.size()));
	return res;
}

string Util::toUtf8(const string& str) {
	string res;
	if(str.empty())
		return res;
	res.resize(str.size() + 1);
	res.resize(utils->to_utf8(&res[0], &str[0], str.size()));
	return res;
}

} // namespace dcapi
