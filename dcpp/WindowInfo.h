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

#ifndef DCPLUSPLUS_DCPP_WINDOW_INFO_H
#define DCPLUSPLUS_DCPP_WINDOW_INFO_H

#include "forward.h"
#include "Util.h"

namespace dcpp {

struct WindowParam {
	WindowParam() : identifies(false) { }
	WindowParam(const string& content, bool identifies) : content(content), identifies(identifies) { }

	string content;
	bool identifies; /// whether this param determines the uniqueness of the WindowInfo holding it.

	operator const string&() const { return content; }
	template<typename T> bool operator==(const T& str) const { return content == str; }
	bool empty() const { return content.empty(); }
};

typedef unordered_map<string, WindowParam> WindowParams;

class WindowInfo {
public:
	explicit WindowInfo(const string& id_, const WindowParams& params_);

	GETSET(string, id, Id);
	GETSET(WindowParams, params, Params);

	bool operator==(const WindowInfo& rhs) const;

	/// special param for hub addresses.
	static const string address;

	/// special param that indicates a CID for an user whose information shall be saved on exit.
	static const string cid;

	/// special param for paths to file lists that must not be deleted on exit.
	static const string fileList;
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_WINDOW_INFO_H)
