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
	buffer.resize(getBlockSize(0));

	auto buf = &buffer[0];
	File f(file, File::READ, File::OPEN | File::SHARED);

	size_t total = 0;
	size_t n = buffer.size();
	while(f.read(buf, n) > 0) {
		callback(buf, n);
		total += n;
		n = buffer.size();
	}

	return total;
}

size_t FileReader::getBlockSize(size_t alignment) {
	auto block = blockSize < DEFAULT_BLOCK_SIZE ? DEFAULT_BLOCK_SIZE : blockSize;
	if(alignment > 0) {
		block = ((block + alignment - 1) / alignment) * alignment;
	}

	return block;
}

void* FileReader::align(void *buf, size_t alignment) {
	return alignment == 0 ? buf
		: reinterpret_cast<void*>(((reinterpret_cast<size_t>(buf) + alignment - 1) / alignment) * alignment);
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
		return READ_FAILED;
	}

	Handle h(tmp);

	DWORD bufSize = static_cast<DWORD>(getBlockSize(sector));
	buffer.resize(bufSize * 2 + sector);

	auto buf = align(&buffer[0], sector);

	DWORD hn = 0;
	DWORD rn = 0;
	uint8_t* hbuf = static_cast<uint8_t*>(buf) + bufSize;
	uint8_t* rbuf = static_cast<uint8_t*>(buf);
	OVERLAPPED over = { 0 };

	// Read the first block
	auto res = ::ReadFile(h, hbuf, bufSize, NULL, &over);
	auto err = ::GetLastError();

	if(!res && err != ERROR_IO_PENDING) {
		if(err != ERROR_HANDLE_EOF) {
			dcdebug("First overlapped read failed: %s\n", Util::translateError(::GetLastError()).c_str());
			return READ_FAILED;
		}
	}

	// Finish the read and see how it went
	if(!GetOverlappedResult(h, &over, &hn, TRUE)) {
		err = ::GetLastError();
		if(err != ERROR_HANDLE_EOF) {
			dcdebug("First overlapped read failed: %s\n", Util::translateError(::GetLastError()).c_str());
			return READ_FAILED;
		}
	}
	over.Offset = hn;

	for (; hn == bufSize;) {
		// Start a new overlapped read
		res = ::ReadFile(h, rbuf, bufSize, NULL, &over);
		auto err = ::GetLastError();

		// Process the previously read data
		callback(hbuf, hn);

		if (!res && err != ERROR_IO_PENDING) {
			if(err != ERROR_HANDLE_EOF) {
				throw FileException(Util::translateError(err));
			}

			rn = 0;
		} else {
			// Finish the new read
			if (!GetOverlappedResult(h, &over, &rn, TRUE)) {
				err = ::GetLastError();
				if(err != ERROR_HANDLE_EOF) {
					throw FileException(Util::translateError(err));
				}

				rn = 0;
			}
		}

		*((uint64_t*)&over.Offset) += rn;

		swap(rbuf, hbuf);
		swap(rn, hn);
	}

	if(hn != 0) {
		// Process leftovers
		callback(hbuf, hn);
	}

	return *((uint64_t*)&over.Offset);
}

size_t FileReader::readMapped(const string& file, const DataCallback& callback) {
	auto tfile = Text::toT(file);

	auto tmp = ::CreateFile(tfile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (tmp == INVALID_HANDLE_VALUE) {
		dcdebug("Failed to open unbuffered file: %s\n", Util::translateError(::GetLastError()).c_str());
		return READ_FAILED;
	}

	Handle h(tmp);

	LARGE_INTEGER size = { 0 };
	if(!::GetFileSizeEx(h, &size)) {
		dcdebug("Couldn't get file size: %s\n", Util::translateError(::GetLastError()).c_str());
		return READ_FAILED;
	}

	if(!(tmp = ::CreateFileMapping(h, NULL, PAGE_READONLY, 0, 0, NULL))) {
		dcdebug("Couldn't create file mapping: %s\n", Util::translateError(::GetLastError()).c_str());
		return READ_FAILED;
	}

	Handle hmap(tmp);

	SYSTEM_INFO si = { 0 };
	::GetSystemInfo(&si);

	auto blockSize = getBlockSize(si.dwPageSize);

	LARGE_INTEGER total = { 0 };
	while(size.QuadPart > 0) {
		auto n = min(size.QuadPart, (int64_t)blockSize);
		auto p = ::MapViewOfFile(hmap, FILE_MAP_READ, total.HighPart, total.LowPart, static_cast<DWORD>(n));
		if(!p) {
			throw FileException(Util::translateError(::GetLastError()));
		}

		callback(p, n);

		if(!::UnmapViewOfFile(p)) {
			throw FileException(Util::translateError(::GetLastError()));
		}

		size.QuadPart -= n;
		total.QuadPart += n;
	}

	return total.QuadPart;
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
