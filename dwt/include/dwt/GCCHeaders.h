/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2009, Jacek Sieka

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

#ifndef DWT_GCCHeaders_h
#define DWT_GCCHeaders_h

#ifndef __GNUC__
#error This file is only for GCC
#endif

// Need to forward declare these since GCC does syntax checking of
// non-instantiated templates!
BOOL CommandBar_InsertMenubarEx( HWND hwndCB, HINSTANCE hInst, LPTSTR pszMenu, WORD iButton );
BOOL CommandBar_AddAdornments( HWND hwndCB, DWORD dwFlags, DWORD dwReserved );
void CommandBar_Destroy( HWND hwndCB );

#ifndef BUTTON_IMAGELIST_ALIGN_CENTER

typedef struct
{
	HIMAGELIST himl; // Index: Normal, hot pushed, disabled. If count is less than 4, we use index 1
	RECT margin; // Margin around icon.
	UINT uAlign;
} BUTTON_IMAGELIST, * PBUTTON_IMAGELIST;

#define BUTTON_IMAGELIST_ALIGN_CENTER   4       // Doesn't draw text

#endif

#define COLOR_MENUHILIGHT       29
#define COLOR_MENUBAR           30
#define ODS_HOTLIGHT        0x0040
#define ODS_INACTIVE        0x0080
#ifndef MN_GETHMENU
#define MN_GETHMENU                     0x01E1
#endif
#if(_WIN32_WINNT >= 0x0500)
#ifndef ODS_NOACCEL
#define ODS_NOACCEL         0x0100
#endif
#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX               0x00100000
#endif
#ifndef GET_KEYSTATE_WPARAM
#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))
#endif
#endif

// Additional (gcc, normally) stuff

#ifndef SPI_GETUIEFFECTS
	#define SPI_GETUIEFFECTS 0x103E
#endif //! SPI_GETUIEFFECTS

#ifndef SPI_SETUIEFFECTS
	#define SPI_SETUIEFFECTS 0x103F
#endif //! SPI_SETUIEFFECTS


#ifndef GCLP_HCURSOR
	#define GCLP_HCURSOR (-12)
#endif //! GCLP_HCURSOR

#ifndef SetClassLong
	DWORD WINAPI SetClassLongA( HWND, INT, LONG );
	DWORD WINAPI SetClassLongW( HWND, INT, LONG );
	#ifdef UNICODE
		#define SetClassLong  SetClassLongW
	#else //! UNICODE
		#define SetClassLong  SetClassLongA
	#endif // !UNICODE
#endif // !SetClassLong

#ifndef SetClassLongPtr
	#ifdef _WIN64
		ULONG_PTR WINAPI SetClassLongPtrA( HWND, INT, LONG_PTR );
		ULONG_PTR WINAPI SetClassLongPtrW( HWND, INT, LONG_PTR );
		#ifdef UNICODE
			#define SetClassLongPtr  SetClassLongPtrW
		#else //! UNICODE
			#define SetClassLongPtr  SetClassLongPtrA
		#endif // !UNICODE
	#else // !_WIN64
		#define SetClassLongPtr  SetClassLong
	#endif // !_WIN64
#endif // !SetClassLongPtr

#ifndef GetWindowLongPtr
	#ifdef _WIN64
		ULONG_PTR WINAPI GetWindowLongPtrA( HWND, LONG_PTR );
		ULONG_PTR WINAPI GetWindowLongPtrW( HWND, LONG_PTR );
		#ifdef UNICODE
			#define GetWindowLongPtr  GetWindowLongPtrW
		#else //! UNICODE
			#define GetWindowLongPtr  GetWindowLongPtrA
		#endif // !UNICODE
	#else //! _WIN64
		#define GetWindowLongPtr  GetWindowLong
	#endif // !_WIN64
#endif // !GetWindowLongPtr

// these should be defined in CommCtrl.h, but the one in MinGW doesn't define them... (2007-11-06)
#if (_WIN32_WINNT >= 0x0501)
#ifndef HDF_SORTUP
#define HDF_SORTUP              0x0400
#endif
#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN            0x0200
#endif
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif
#endif
#ifndef TBN_GETINFOTIP
#ifdef UNICODE
#define TBN_GETINFOTIP          TBN_GETINFOTIPW
#else
#define TBN_GETINFOTIP          TBN_GETINFOTIPA
#endif
#endif

// MinGW's CommCtrl.h has got some of the following defs wrong
#undef LVIF_TEXT
#undef LVIF_IMAGE
#undef LVIF_PARAM
#undef LVIF_STATE
#define LVIF_TEXT               0x00000001
#define LVIF_IMAGE              0x00000002
#define LVIF_PARAM              0x00000004
#define LVIF_STATE              0x00000008
#if (_WIN32_IE >= 0x0300)
#undef LVIF_INDENT
#undef LVIF_NORECOMPUTE
#define LVIF_INDENT             0x00000010
#define LVIF_NORECOMPUTE        0x00000800
#endif
#if (_WIN32_WINNT >= 0x0501)
#undef LVIF_GROUPID
#undef LVIF_COLUMNS
#define LVIF_GROUPID            0x00000100
#define LVIF_COLUMNS            0x00000200
#endif

// List-View grouping stuff that should be in CommCtrl.h (Vista parts have been left out)

#ifndef LVGF_NONE

typedef struct tagLVGROUP
{
    UINT    cbSize;
    UINT    mask;
    LPWSTR  pszHeader;
    int     cchHeader;

    LPWSTR  pszFooter;
    int     cchFooter;

    int     iGroupId;

    UINT    stateMask;
    UINT    state;
    UINT    uAlign;
} LVGROUP, *PLVGROUP;

#define LVGF_NONE           0x00000000
#define LVGF_HEADER         0x00000001
#define LVGF_FOOTER         0x00000002
#define LVGF_STATE          0x00000004
#define LVGF_ALIGN          0x00000008
#define LVGF_GROUPID        0x00000010
#endif

#ifndef LVGS_NORMAL
#define LVGS_NORMAL             0x00000000
#define LVGS_COLLAPSED          0x00000001
#define LVGS_HIDDEN             0x00000002
#define LVGS_NOHEADER           0x00000004
#define LVGS_COLLAPSIBLE        0x00000008
#define LVGS_FOCUSED            0x00000010
#define LVGS_SELECTED           0x00000020
#define LVGS_SUBSETED           0x00000040
#define LVGS_SUBSETLINKFOCUSED  0x00000080
#endif

#ifndef LVGA_HEADER_LEFT
#define LVGA_HEADER_LEFT    0x00000001
#define LVGA_HEADER_CENTER  0x00000002
#define LVGA_HEADER_RIGHT   0x00000004  // Don't forget to validate exclusivity
#define LVGA_FOOTER_LEFT    0x00000008
#define LVGA_FOOTER_CENTER  0x00000010
#define LVGA_FOOTER_RIGHT   0x00000020  // Don't forget to validate exclusivity
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#endif
