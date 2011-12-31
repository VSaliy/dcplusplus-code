#include "stdinc.h"

#include "FileReader.h"

#include "File.h"
#include "Text.h"
#include "Util.h"

namespace dcpp {

using std::make_pair;
using std::swap;

namespace {
static const size_t READ_FAILED = static_cast<size_t>(-1);
}

size_t FileReader::read(const string& file, const DataCallback& callback) {
	size_t ret = READ_FAILED;

	if(direct) {
		ret = readDirect(file, callback);
	}

	if(ret == READ_FAILED) {
		ret = readMapped(file, callback);

		if(ret == READ_FAILED) {
			ret = readCached(file, callback);
		}
	}

	return ret;
}


/** Read entire file, never returns READ_FAILED */
size_t FileReader::readCached(const string& file, const DataCallback& callback) {
	auto buf = getBuffer(512);

	File f(file, File::READ, File::OPEN | File::SHARED);

	size_t total = 0;
	size_t n = buf.second;
	while(f.read(buf.first, n) > 0) {
		callback(buf.first, n);
		total += n;
		n = buf.second;
	}

	return total;
}

pair<void*, size_t> FileReader::getBuffer(size_t alignment) {
	auto block = (((blockSize == 0 ? DEFAULT_BLOCK_SIZE : blockSize) + alignment - 1) / alignment) * alignment;

	buffer.resize(block * 2 + alignment); // Prepare for worst case alignment

	auto start = reinterpret_cast<void*>(((reinterpret_cast<size_t>(&buffer[0]) + alignment - 1) / alignment) * alignment);
	return make_pair(start, block);
}

#ifdef _WIN32

struct Handle : boost::noncopyable {
	Handle(HANDLE h) : h(h) { }
	~Handle() { ::CloseHandle(h); }

	operator HANDLE() { return h; }

	HANDLE h;
};

size_t FileReader::readDirect(const string& file, const DataCallback& callback) {
	DWORD sector = 0, y;

	auto tfile = Text::toT(file);

	if (!::GetDiskFreeSpace(Util::getFilePath(tfile).c_str(), &y, &sector, &y, &y)) {
		dcdebug("Failed to get sector size: %s\n", Util::translateError(::GetLastError()).c_str());
		return READ_FAILED;
	}

	auto tmp = ::CreateFile(tfile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);

	if (tmp == INVALID_HANDLE_VALUE) {
		dcdebug("Failed to open unbuffered file: %s\n", Util::translateError(::GetLastError()).c_str());
		return false;
	}

	Handle h(tmp);

	auto buf = getBuffer(sector);
	DWORD bufSize = static_cast<DWORD>(buf.second);

	DWORD hn = 0;
	DWORD rn = 0;
	uint8_t* hbuf = static_cast<uint8_t*>(buf.first) + bufSize;
	uint8_t* rbuf = static_cast<uint8_t*>(buf.first);
	OVERLAPPED over = { 0 };

	// Read the first block
	auto res = ::ReadFile(h, hbuf, buf.second, &hn, &over);

	if(!res) {
		auto err = ::GetLastError();
		if (err == ERROR_HANDLE_EOF) {
			hn = 0;
		} else if(err == ERROR_IO_PENDING) {
			// Finish the read and see how it went
			if(!GetOverlappedResult(h, &over, &hn, TRUE)) {
				if (::GetLastError() == ERROR_HANDLE_EOF) {
					hn = 0;
				} else {
					dcdebug("First overlapped read failed: %s\n", Util::translateError(::GetLastError()).c_str());
					return READ_FAILED;
				}
			}
		}
	}

	over.Offset = hn;

	for (; hn > 0;) {
		// Last read returned some bytes, start a new overlapped read
		res = ::ReadFile(h, rbuf, buf.second, &rn, &over);

		callback(hbuf, hn);

		if (!res) {
			auto err = ::GetLastError();
			if(err == ERROR_HANDLE_EOF) {
				rn = 0;
			} else if(err == ERROR_IO_PENDING) {
				// Finish the read
				if (!GetOverlappedResult(h, &over, &rn, TRUE)) {
					err = ::GetLastError();
					if(err != ERROR_HANDLE_EOF) {
						throw FileException(Util::translateError(err));
					}

					rn = 0;
				}
			} else {
				throw FileException(Util::translateError(::GetLastError()));
			}
		}

		*((uint64_t*)&over.Offset) += rn;

		swap(rbuf, hbuf);
		swap(rn, hn);
	}

	return *((uint64_t*)&over.Offset);
}

size_t FileReader::readMapped(const string& file, const DataCallback& callback) {
	return READ_FAILED;
}

#else

size_t FileReader::readDirect(const string& file, const DataCallback& callback) {
	return READ_FAILED;
}

size_t FileReader::readMapped(const string& file, const DataCallback& callback) {
	return READ_FAILED;
}

#endif
}
