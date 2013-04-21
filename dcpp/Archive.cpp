/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "Archive.h"

#include "debug.h"
#include "File.h"
#include "Util.h"

#ifndef LIBARCHIVE_STATIC
#define LIBARCHIVE_STATIC
#endif
#include <archive.h>
#include <archive_entry.h>

namespace dcpp {

Archive::Archive(const string& path) : a(nullptr), f(nullptr) {
	a = archive_read_new();
	if(!a) {
		throw Exception(Util::translateError(ERROR_OUTOFMEMORY));
	}

	f = dcpp_fopen(path.c_str(), "rb");
	if(!f) {
		throw FileException(Util::translateError(ERROR_FILE_NOT_FOUND));
	}

	check(archive_read_support_format_tar(a));
	check(archive_read_support_filter_bzip2(a));
	check(archive_read_support_filter_gzip(a));

	check(archive_read_open_FILE(a, f));
}

Archive::~Archive() {
	if(a) {
		archive_read_free(a);
	}

	if(f) {
		fclose(f);
	}
}

void Archive::extract(const string& path) {
	dcassert(!path.empty() && (*(path.end() - 1) == '/' || *(path.end() - 1) == '\\'));

	::archive_entry* entry;
	while(true) {
		if(check(archive_read_next_header(a, &entry)) == ARCHIVE_EOF) {
			break;
		}

		auto path_out = path + archive_entry_pathname(entry);
		File::ensureDirectory(path_out);
		File f_out(path_out, File::WRITE, File::CREATE | File::TRUNCATE);

		const void* buf;
		size_t size;
		__LA_INT64_T offset;
		while(true) {
			if(check(archive_read_data_block(a, &buf, &size, &offset)) == ARCHIVE_EOF) {
				break;
			}
			f_out.write(buf, size);
		}
	}
}

int Archive::check(int ret) {
	if(ret != ARCHIVE_OK && ret != ARCHIVE_EOF) {
		throw Exception(archive_error_string(a));
	}
	return ret;
}

} // namespace dcpp
