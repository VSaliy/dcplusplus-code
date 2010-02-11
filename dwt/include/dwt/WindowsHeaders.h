/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2010, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** @file
 * This file contains all common DWT includes and also configures compiler-specific
 * quirks etc.
 */
#ifndef DWT_WindowsHeaders_h
#define DWT_WindowsHeaders_h

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif
#ifndef _WIN32_IE
	#define _WIN32_IE 0x0501
#endif
#ifndef WINVER
	#define WINVER 0x501
#endif

// Try to remove min/max macros - we prefer the c++ equivalents
#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#ifndef STRICT
#define STRICT
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Windows API files...
#include <errno.h>
#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <assert.h>

// Standard headers that most classes need
#include <vector>
#include <list>
#include <boost/noncopyable.hpp>

// GCC specific
#ifdef __GNUC__
#include "GCCHeaders.h"
#endif //! __GNUC__

// MSVC specific
#ifdef _MSC_VER
#include "VCDesktopHeaders.h"
#endif

#endif // !WindowsHeaders_h
