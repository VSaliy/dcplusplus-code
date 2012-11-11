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

#ifndef PLUGINSDK_UTIL_H
#define PLUGINSDK_UTIL_H

#include <cstdint>
#include <string>

#include <pluginsdk/PluginDefs.h>

namespace dcapi {

using std::string;
#ifdef _UNICODE
using std::wstring;
#endif

class Util
{
public:
	static bool init(DCCorePtr core);
	static void init(DCUtilsPtr coreUtils);
	static DCUtilsPtr handle();

#ifdef _UNICODE
	static string fromT(const wstring& str);
	static wstring toT(const string& str);

	// identities (for convenience)
	static string fromT(const string& str);
	static wstring toT(const wstring& str);
#endif // _UNICODE

	static string fromUtf8(const string& str);
	static string toUtf8(const string& str);

private:
	static DCUtilsPtr utils;
};

} // namespace dcapi

#endif
