/*
 * Copyright (C) 2010 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef PLUGIN_STDAFX_H
#define PLUGIN_STDAFX_H

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define STRICT
#define WIN32_LEAN_AND_MEAN

#ifdef _MSC_VER

/* disable the deprecated warnings for the CRT functions. */
#define _CRT_SECURE_NO_DEPRECATE 1
#define _ATL_SECURE_NO_DEPRECATE 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1

#if _MSC_VER == 1400 || _MSC_VER == 1500
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
/* disable the deprecated warnings for the crt functions. */
#pragma warning(disable: 4996)
#endif

#endif

#include <windows.h>
#include <tchar.h>

#else
#include <unistd.h>
#endif

/* This can make a #define value to string (from boost) */
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

#include <stdint.h>
#include <PluginDefs.h>

#endif /* PLUGIN_STDAFX_H */