/* 
 * Copyright (C) 2001 Jacek Sieka, j_s@telia.com
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

#if !defined(AFX_FILE_H__CB551CD7_189C_4175_922E_8B00B4C8D6F1__INCLUDED_)
#define AFX_FILE_H__CB551CD7_189C_4175_922E_8B00B4C8D6F1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Exception.h"

#ifndef WIN32
#include <sys/stat.h>
#include <fcntl.h>
#endif

STANDARD_EXCEPTION(FileException);

class File  
{
public:
	enum {
		READ = 0x01,
		WRITE = 0x02
	};
	
	enum {
		OPEN = 0x01,
		CREATE = 0x02,
		TRUNCATE = 0x04
	};

#ifdef WIN32
	File(const string& aFileName, int access = WRITE, int mode = OPEN) throw(FileException) {
		dcassert(access == WRITE || access == READ || access == (READ | WRITE));

		int m = 0;
		if(mode & OPEN) {
			if(mode & CREATE) {
				m = (mode & TRUNCATE) ? CREATE_ALWAYS : OPEN_ALWAYS;
			} else {
				m = (mode & TRUNCATE) ? TRUNCATE_EXISTING : OPEN_EXISTING;
			}
		} else {
			if(mode & CREATE) {
				m = (mode & TRUNCATE) ? CREATE_ALWAYS : CREATE_NEW;
			} else {
				dcassert(0);
			}
		}
		int a = 0;
		if(access & READ)
			a |= GENERIC_READ;
		if(access & WRITE)
			a |= GENERIC_WRITE;

		h = ::CreateFile(aFileName.c_str(), a, FILE_SHARE_READ, NULL, m, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		
		if(h == INVALID_HANDLE_VALUE) {
			throw FileException(Util::translateError(GetLastError()));
		}

	}

	virtual void close() {
		if(h != INVALID_HANDLE_VALUE) {
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}
	
	virtual int64_t getSize() {
		DWORD x;
		DWORD l = ::GetFileSize(h, &x);
		
		if( (l == INVALID_FILE_SIZE) && (GetLastError() != NO_ERROR))
			return -1;
		
		return (int64_t)l | ((int64_t)x)<<32;
	}

	virtual int64_t getPos() {
		LONG x = 0;
		DWORD l = ::SetFilePointer(h, 0, &x, FILE_CURRENT);
		
		return (int64_t)l | ((int64_t)x)<<32;
	}		

	virtual void setPos(int64_t pos) {
		LONG x = (LONG) (pos>>32);
		::SetFilePointer(h, (DWORD)(pos & 0xffffffff), &x, FILE_BEGIN);
	}		
	virtual void setEndPos(int64_t pos) {
		LONG x = (LONG) (pos>>32);
		::SetFilePointer(h, (DWORD)(pos & 0xffffffff), &x, FILE_END);
	}		

	virtual void movePos(int64_t pos) {
		LONG x = (LONG) (pos>>32);
		::SetFilePointer(h, (DWORD)(pos & 0xffffffff), &x, FILE_CURRENT);
	}
	
	virtual u_int32_t read(void* buf, u_int32_t len) throw(FileException) {
		DWORD x;
		if(!::ReadFile(h, buf, len, &x, NULL)) {
			throw(FileException(Util::translateError(GetLastError())));
		}
		return x;
	}

	virtual void write(const void* buf, u_int32_t len) throw(FileException) {
		DWORD x;
		if(!::WriteFile(h, buf, len, &x, NULL)) {
			throw FileException(Util::translateError(GetLastError()));
		}
		if(x < len) {
			throw FileException(STRING(DISC_FULL));
		}
	}

	static void deleteFile(const string& aFileName) { ::DeleteFile(aFileName.c_str()); };
	static void renameFile(const string& source, const string& target) { ::MoveFile(source.c_str(), target.c_str()); };

	static int64_t getSize(const string& aFileName) {
		WIN32_FIND_DATA fd;
		HANDLE hFind;
		
		hFind = FindFirstFile(aFileName.c_str(), &fd);
		
		if (hFind == INVALID_HANDLE_VALUE) {
			return -1;
		} else {
			FindClose(hFind);
			return ((int64_t)fd.nFileSizeHigh << 32 | (int64_t)fd.nFileSizeLow);
		}
	}

#else // WIN32
	
	File(const string& aFileName, int access = WRITE, int mode = OPEN) throw(FileException) {
		dcassert(access == WRITE || access == READ || access == (READ | WRITE));
		
		int m = 0;
		if(access == READ)
			m |= O_RDONLY;
		else if(access == WRITE)
			m |= O_WRONLY;
		else
			m |= O_RDWR;
		
		if(mode & CREATE) {
			m |= O_CREAT;
		}
		if(mode & TRUNCATE) {
			m |= O_TRUNC;
		}
		h = open(aFileName.c_str(), m, S_IRUSR | S_IWUSR);
		if(h == -1)
			throw FileException("Could not open file");
	}		

	virtual void close() {
		if(h != -1) {
			::close(h);
			h = -1;
		}
	}

	virtual int64_t getSize() {
		struct stat s;
		if(fstat(h, &s) == -1)
			return -1;
		
		return (int64_t)s.st_size;
	}

	virtual int64_t getPos() {
		return (int64_t) lseek(h, 0, SEEK_CUR);
	}

	virtual void setPos(int64_t pos) { lseek(h, (off_t)pos, SEEK_SET); };
	virtual void setEndPos(int64_t pos) { lseek(h, (off_t)pos, SEEK_END); };
	virtual void movePos(int64_t pos) { lseek(h, (off_t)pos, SEEK_CUR); };

	virtual u_int32_t read(void* buf, u_int32_t len) throw(FileException) {
		ssize_t x = ::read(h, buf, (size_t)len);
		if(x == -1)
			throw("Read error");
		return (u_int32_t)x;
	}
	
	virtual void write(const void* buf, u_int32_t len) throw(FileException) {
		ssize_t x;
		x = ::write(h, buf, len);
		if(x == -1)
			throw FileException("Write error");
		if(x < (ssize_t)len)
			throw FileException("Disk full(?)");
	}

	static void deleteFile(const string& aFileName) { ::unlink(aFileName.c_str()); };
	static void renameFile(const string& source, const string& target) { ::rename(source.c_str(), target.c_str()); };

	static int64_t getSize(const string& aFileName) {
		struct stat s;
		if(stat(aFileName.c_str(), &s) == -1)
			return -1;
		
		return s.st_size;
	}
	
#endif // WIN32

	virtual ~File() {
		close();
	}

	virtual string read(u_int32_t len) throw(FileException) {
		char* buf = new char[len];
		u_int32_t x;
		try {
			x = read(buf, len);
		} catch(...) {
			delete[] buf;
			throw;
		}
		string tmp(buf, x);
		delete[] buf;
		return tmp;
	}

	virtual string read() throw(FileException) {
		setPos(0);
		return read((u_int32_t)getSize());
	}

	virtual void write(const string& aString) throw(FileException) {
		write((void*)aString.data(), aString.size());
	}
		
private:
#ifdef WIN32
	HANDLE h;
#else
	int h;
#endif

};

class BufferedFile : public File {
public:
	BufferedFile(const string& aFileName, int access = WRITE, int mode = OPEN, int bufSize = SETTING(BUFFER_SIZE)) throw(FileException) : 
		File(aFileName, access, mode), pos(0), size(bufSize*1024) {
		
		buf = new u_int8_t[size];
	}
	
	virtual ~BufferedFile() {
		flush();
		delete[] buf;
	}

	void flush() throw(FileException) {
		dcassert(pos >= 0);
		if(pos > 0) {
			try {
				File::write(buf, (u_int32_t)pos);
			} catch( ... ) {
				pos = 0;
				throw;
			}
			pos = 0;
		}
	}

	virtual void write(const void* aBuf, u_int32_t len) throw(FileException) {
		dcassert(pos >= 0);

		if( (size == 0) || ((pos == 0) && (len > (u_int32_t)size)) ) {
			File::write(aBuf, len);
			return;
		}

		u_int32_t pos2 = 0;
		while(pos2 < len) {
			size_t i = min((size_t)(size-pos), (size_t)(len - pos2));
			
			memcpy(buf+pos, ((char*)aBuf)+pos2, i);
			pos += (int)i;
			pos2 += (u_int32_t)i;
			dcassert(pos <= size);
			dcassert(pos2 <= len);

			if(pos == size)
				flush();
		}
	}

	virtual void write(const string& aStr) throw(FileException) { write(aStr.c_str(), aStr.size()); };

	virtual void close() { flush(); File::close(); };
	virtual int64_t getSize() { flush(); return File::getSize(); };
	virtual int64_t getPos() { flush(); return File::getPos(); };
	virtual void setPos(int64_t pos) { flush(); File::setPos(pos); };	
	virtual void movePos(int64_t pos) { flush(); File::movePos(pos); };
	virtual u_int32_t read(void* aBuf, u_int32_t len) throw(FileException) { flush(); return File::read(aBuf, len); };
	virtual string read(int32_t len) throw(FileException) { flush(); return File::read(len); };	
	virtual string read() throw(FileException) {  flush(); return File::read(); };
	
private:
	u_int8_t* buf;
	int pos;
	int size;
};

#endif // !defined(AFX_FILE_H__CB551CD7_189C_4175_922E_8B00B4C8D6F1__INCLUDED_)

/**
 * @file File.h
 * $Id: File.h,v 1.12 2002/04/22 13:58:14 arnetheduck Exp $
 */

