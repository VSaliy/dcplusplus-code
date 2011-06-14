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

#include "stdafx.h"
#include "CrashLogger.h"

#include <dcpp/Util.h>
#include <dcpp/version.h>
#include "WinUtil.h"

namespace {

FILE* f;

#if defined(__MINGW32__) && defined(HAVE_BFD_H) && !defined(_WIN64)

/* This backtrace writer for MinGW comes from the backtrace-mingw project by Cloud Wu:
<http://code.google.com/p/backtrace-mingw/> */

/*
	Copyright (c) 2010 ,
		Cloud Wu . All rights reserved.

		http://www.codingnow.com

	Use, modification and distribution are subject to the "New BSD License"
	as listed at <url: http://www.opensource.org/licenses/bsd-license.php >.
*/

#include <bfd.h>
#include <imagehlp.h>

struct bfd_ctx {
	bfd * handle;
	asymbol ** symbol;
};

struct bfd_set {
	char * name;
	bfd_ctx * bc;
	bfd_set *next;
};

struct find_info {
	asymbol **symbol;
	bfd_vma counter;
	const char *file;
	const char *func;
	unsigned line;
};

void lookup_section(bfd *abfd, asection *sec, void *opaque_data)
{
	find_info *data = reinterpret_cast<find_info*>(opaque_data);

	if (data->func)
		return;

	if (!(bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
		return;

	bfd_vma vma = bfd_get_section_vma(abfd, sec);
	if (data->counter < vma || vma + bfd_get_section_size(sec) <= data->counter)
		return;

	bfd_find_nearest_line(abfd, sec, data->symbol, data->counter - vma, &(data->file), &(data->func), &(data->line));
}

void find(bfd_ctx * b, DWORD offset, const char **file, const char **func, unsigned *line)
{
	find_info data = { b->symbol, offset };

	bfd_map_over_sections(b->handle, &lookup_section, &data);

	*file = data.file;
	*func = data.func;
	*line = data.line;
}

bool init_bfd_ctx(bfd_ctx *bc, const char * procname)
{
	bc->handle = 0;
	bc->symbol = 0;

	bfd *b = bfd_openr(procname, 0);
	if (!b) {
		fprintf(f, "Failed to open bfd from %s\n", procname);
		return false;
	}

	int r1 = bfd_check_format(b, bfd_object);
	int r2 = bfd_check_format_matches(b, bfd_object, 0);
	int r3 = bfd_get_file_flags(b) & HAS_SYMS;

	if (!(r1 && r2 && r3)) {
		bfd_close(b);
		fprintf(f, "Failed to init bfd from %s\n", procname);
		return false;
	}

	void *symbol_table;

	unsigned dummy = 0;
	if (bfd_read_minisymbols(b, FALSE, &symbol_table, &dummy) == 0) {
		if (bfd_read_minisymbols(b, TRUE, &symbol_table, &dummy) < 0) {
			free(symbol_table);
			bfd_close(b);
			fprintf(f, "Failed to read symbols from %s\n", procname);
			return false;
		}
	}

	bc->handle = b;
	bc->symbol = reinterpret_cast<asymbol**>(symbol_table);

	return true;
}

void close_bfd_ctx(bfd_ctx *bc)
{
	if (bc) {
		if (bc->symbol) {
			free(bc->symbol);
		}
		if (bc->handle) {
			bfd_close(bc->handle);
		}
	}
}

bfd_ctx * get_bc(bfd_set *set, const char *procname)
{
	while(set->name) {
		if (strcmp(set->name, procname) == 0) {
			return set->bc;
		}
		set = set->next;
	}
	bfd_ctx bc;
	if(!init_bfd_ctx(&bc, procname)) {
		return 0;
	}
	set->next = reinterpret_cast<bfd_set*>(calloc(1, sizeof(*set)));
	set->bc = reinterpret_cast<bfd_ctx*>(malloc(sizeof(bfd_ctx)));
	memcpy(set->bc, &bc, sizeof(bc));
	set->name = strdup(procname);

	return set->bc;
}

void release_set(bfd_set *set)
{
	while(set) {
		bfd_set * temp = set->next;
		free(set->name);
		close_bfd_ctx(set->bc);
		free(set);
		set = temp;
	}
}

// add some x64 defs that MinGW is missing.
#define DWORD64 DWORD
#define STACKFRAME64 STACKFRAME
#define IMAGEHLP_SYMBOL64 IMAGEHLP_SYMBOL
#define StackWalk64 StackWalk
#define SymFunctionTableAccess64 SymFunctionTableAccess
#define SymGetModuleBase64 SymGetModuleBase
#define SymGetSymFromAddr64 SymGetSymFromAddr

#elif defined(_MSC_VER)

#include <dbghelp.h>

// Nothing special for MSVC besides that include file.

#else

#define NO_BACKTRACE

#endif

inline void writeAppInfo() {
	fputs(Util::formatTime(APPNAME " has crashed on %Y-%m-%d at %H:%M:%S.\n", time(0)).c_str(), f);
	fputs("Please report this data to the " APPNAME " team for further investigation.\n\n", f);

	fprintf(f, APPNAME " version: %s\n", fullVersionString.c_str());
	fprintf(f, "TTH: %S\n", WinUtil::tth.c_str());

#ifdef __MINGW32__
	fputs("Compiled with GCC " __VERSION__, f);
#else
	fprintf(f, "Compiled with MS Visual Studio %d", _MSC_VER);
#endif
#ifdef _DEBUG
	fputs(" (debug)", f);
#endif
#ifdef _WIN64
	fputs(" (x64)", f);
#endif
	fputs("\n", f);
}

inline void writePlatformInfo() {
	OSVERSIONINFOEX ver = { sizeof(OSVERSIONINFOEX) };
	if(!::GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&ver)))
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if(::GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&ver))) {
		fprintf(f, "Windows version: major = %d, minor = %d, build = %d, SP = %d, type = %d\n",
			ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber, ver.wServicePackMajor, ver.wProductType);

	} else {
		fputs("Windows version: unknown\n", f);
	}

	SYSTEM_INFO info;
	::GetNativeSystemInfo(&info);
	fprintf(f, "Processors: %d * %s\n", info.dwNumberOfProcessors,
		(info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" :
		(info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) ? "x86" :
		(info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) ? "ia64" :
		"[unknown architecture]");
}

#ifndef NO_BACKTRACE

inline void writeBacktrace(LPCONTEXT context)
{
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

#ifdef __MINGW32__
	SymSetOptions(SYMOPT_DEFERRED_LOADS);
#else
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
#endif

	if(!SymInitialize(process, 0, TRUE)) {
		fputs("Failed to init symbol context\n", f);
		return;
	}

#ifdef __MINGW32__
	bfd_init();
	bfd_set *set = reinterpret_cast<bfd_set*>(calloc(1, sizeof(*set)));

	bfd_ctx *bc = 0;
#endif

	STACKFRAME64 frame;
	memset(&frame, 0, sizeof(frame));

	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;

#ifdef _WIN64
	frame.AddrPC.Offset = context->Rip;
	frame.AddrFrame.Offset = context->Rbp;
	frame.AddrStack.Offset = context->Rsp;

#define WALK_ARCH IMAGE_FILE_MACHINE_AMD64

#else
	frame.AddrPC.Offset = context->Eip;
	frame.AddrFrame.Offset = context->Ebp;
	frame.AddrStack.Offset = context->Esp;

#define WALK_ARCH IMAGE_FILE_MACHINE_I386

#endif

	char symbol_buffer[sizeof(IMAGEHLP_SYMBOL64) + 255];
	char module_name_raw[MAX_PATH];

	// get the current module address.
	HMODULE module = ::GetModuleHandle(0);

	for(uint8_t depth = 0; depth <= 128; ++depth) {

		if(!StackWalk64(WALK_ARCH, process, thread, &frame, context,
			0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) { break; }

		HMODULE module_base = reinterpret_cast<HMODULE>(SymGetModuleBase64(process, frame.AddrPC.Offset));

		const char * module_name = 0;
		if(module_base) {
			if(module_base == module) {
				module_name = "DCPlusPlus.pdb";
			} else if(GetModuleFileNameA(module_base, module_name_raw, MAX_PATH)) {
				module_name = module_name_raw;
			}
		}
		if(module_name) {
#ifdef __MINGW32__
			bc = get_bc(set, module_name);
#endif
		} else {
			module_name = "[unknown module]";
		}

		const char* file = 0;
		const char* func = 0;
		unsigned line = 0;

#ifdef __MINGW32__
		if(bc) {
			find(bc, frame.AddrPC.Offset, &file, &func, &line);
		}
#endif

		if(file == 0) {
			DWORD64 dummy = 0;
			IMAGEHLP_SYMBOL64* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(symbol_buffer);
			symbol->SizeOfStruct = (sizeof *symbol) + 255;
			symbol->MaxNameLength = 254;
			if(SymGetSymFromAddr64(process, frame.AddrPC.Offset, &dummy, symbol)) {
				file = symbol->Name;
			} else {
				file = "[unknown file]";
			}
		}

#ifdef _MSC_VER
		IMAGEHLP_LINE64 info = { sizeof(IMAGEHLP_LINE64) };
		DWORD dummy = 0;
		if(SymGetLineFromAddr64(process, frame.AddrPC.Offset, &dummy, &info)) {
			func = file;
			file = info.FileName;
			line = info.LineNumber;
		}
#endif

		if(func == 0) {
			fprintf(f, "%s - %s\n", module_name, file);
		} else {
			fprintf(f, "%s - %s (%d) - in function %s\n", module_name, file, line, func);
		}
	}

#ifdef __MINGW32__
	release_set(set);
#endif

	SymCleanup(process);
}

#endif // NO_BACKTRACE

LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info)
{
	f = fopen((Util::getPath(Util::PATH_USER_LOCAL) + "CrashLog.txt").c_str(), "w");
	if(f) {
		writeAppInfo();

		fprintf(f, "Exception code: %x\n", info->ExceptionRecord->ExceptionCode);

		writePlatformInfo();

#ifdef NO_BACKTRACE
		fputs("\nStack trace unavailable: this program hasn't been compiled with backtrace support\n", f);
#else
		fputs("\nWriting the stack trace...\n\n", f);
		writeBacktrace(info->ContextRecord);
#endif

		fputs("\nInformation about the crash has been written.\n", f);
		fclose(f);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

LPTOP_LEVEL_EXCEPTION_FILTER prevFilter;

} // anonymous namespace

CrashLogger::CrashLogger() {
	prevFilter = SetUnhandledExceptionFilter(exception_filter);
}

CrashLogger::~CrashLogger() {
	SetUnhandledExceptionFilter(prevFilter);
}
