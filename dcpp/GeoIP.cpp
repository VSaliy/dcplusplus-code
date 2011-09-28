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
#include "GeoIP.h"

#include "File.h"
#include "format.h"
#include "Util.h"
#include "ZUtils.h"

#include <GeoIP.h>

namespace dcpp {

GeoIP::GeoIP() : geo(0) {
}

GeoIP::~GeoIP() {
	GeoIP_delete(geo);
}

void GeoIP::init(const string& path) {
	if(File::getSize(path) <= 0) {
		if(File::getSize(path + ".gz") <= 0) {
			return;
		}

		// there is a .gz file but no .dat; decompress the .gz.
		try { GZ::decompress(path + ".gz", path); }
		catch(const Exception&) { return; }
	}

	geo = GeoIP_open(path.c_str(), GEOIP_STANDARD);
	if(geo) {
		GeoIP_set_charset(geo, GEOIP_CHARSET_UTF8);
	}
}

string GeoIP::getCountry(const string& ip) const {
	if(geo) {

		auto id = (v6() ? GeoIP_id_by_addr_v6 : GeoIP_id_by_addr)(geo, ip.c_str());
		if(id > 0) {

			auto code = GeoIP_code_by_id(id);
			auto name = GeoIP_country_name_by_id(geo, id);

			if(code && name)
				return str(F_("%1% - %2%") % code % name);

			if(code && !name)
				return code;

			if(name && !code)
				return name;
		}
	}

	return Util::emptyString;
}

bool GeoIP::v6() const {
	return geo->databaseType == GEOIP_COUNTRY_EDITION_V6 || geo->databaseType == GEOIP_LARGE_COUNTRY_EDITION_V6;
}

} // namespace dcpp
