/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_ARCHIVE_H
#define DCPLUSPLUS_DCPP_ARCHIVE_H

#include <string>

typedef void* unzFile;

namespace dcpp {

using std::string;

/** Extract a ZIP archive. */
class Archive {
public:
	Archive(const string& path);
	~Archive();

	/** Extract all the files in the archive to the specified directory. Throws on errors. */
	void extract(const string& path);

private:
	inline int check(int ret);

	::unzFile file;
};

} // namespace dcpp

#endif
