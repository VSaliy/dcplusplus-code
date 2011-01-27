/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2011, Jacek Sieka

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

/* the shared_ptr implementation provided by MinGW / GCC 4.5's libstdc++ consumes too many
semaphores, so we prefer boost's one. see <https://bugs.launchpad.net/dcplusplus/+bug/654040>. */
#define _SHARED_PTR_H 1 // skip libstdc++'s bits/shared_ptr.h
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#define SHARED_PTR_NS boost

#ifndef BCM_FIRST
#define BCM_FIRST 0x1600
#endif
#ifndef BCM_GETIDEALSIZE
#define BCM_GETIDEALSIZE (BCM_FIRST + 0x0001)
#endif
#ifndef BCM_SETTEXTMARGIN
#define BCM_SETTEXTMARGIN (BCM_FIRST + 0x0004)
#endif

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

// MinGW doesn't have all the fields in the following structure, so re-define it better.
typedef struct tagNMLVCUSTOMDRAW_
{
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
    COLORREF clrTextBk;
#if (_WIN32_IE >= 0x0400)
    int iSubItem;
#endif
#if (_WIN32_WINNT >= 0x0501)
    DWORD dwItemType;

    // Item custom draw
    COLORREF clrFace;
    int iIconEffect;
    int iIconPhase;
    int iPartId;
    int iStateId;

    // Group Custom Draw
    RECT rcText;
    UINT uAlign;      // Alignment. Use LVGA_HEADER_CENTER, LVGA_HEADER_RIGHT, LVGA_HEADER_LEFT
#endif
} NMLVCUSTOMDRAW_, *LPNMLVCUSTOMDRAW_;

#define tagNMLVCUSTOMDRAW tagNMLVCUSTOMDRAW_
#define NMLVCUSTOMDRAW NMLVCUSTOMDRAW_
#define LPNMLVCUSTOMDRAW LPNMLVCUSTOMDRAW_

// dwItemType
#define LVCDI_ITEM      0x00000000
#define LVCDI_GROUP     0x00000001
#define LVCDI_ITEMSLIST 0x00000002

#ifndef LVHT_EX_GROUP
#define LVHT_EX_GROUP_HEADER       0x10000000
#define LVHT_EX_GROUP_FOOTER       0x20000000
#define LVHT_EX_GROUP_COLLAPSE     0x40000000
#define LVHT_EX_GROUP_BACKGROUND   0x80000000
#define LVHT_EX_GROUP_STATEICON    0x01000000
#define LVHT_EX_GROUP_SUBSETLINK   0x02000000
#define LVHT_EX_GROUP              (LVHT_EX_GROUP_BACKGROUND | LVHT_EX_GROUP_COLLAPSE | LVHT_EX_GROUP_FOOTER | LVHT_EX_GROUP_HEADER | LVHT_EX_GROUP_STATEICON | LVHT_EX_GROUP_SUBSETLINK)
#endif

#ifndef DTT_VALIDBITS
// Callback function used by DrawThemeTextEx, instead of DrawText
typedef int (WINAPI *DTT_CALLBACK_PROC)(HDC hdc, LPWSTR pszText, int cchText, LPRECT prc, UINT dwFlags, LPARAM lParam);

//---- bits used in dwFlags of DTTOPTS ----
#define DTT_TEXTCOLOR       (1UL << 0)      // crText has been specified
#define DTT_BORDERCOLOR     (1UL << 1)      // crBorder has been specified
#define DTT_SHADOWCOLOR     (1UL << 2)      // crShadow has been specified
#define DTT_SHADOWTYPE      (1UL << 3)      // iTextShadowType has been specified
#define DTT_SHADOWOFFSET    (1UL << 4)      // ptShadowOffset has been specified
#define DTT_BORDERSIZE      (1UL << 5)      // iBorderSize has been specified
#define DTT_FONTPROP        (1UL << 6)      // iFontPropId has been specified
#define DTT_COLORPROP       (1UL << 7)      // iColorPropId has been specified
#define DTT_STATEID         (1UL << 8)      // IStateId has been specified
#define DTT_CALCRECT        (1UL << 9)      // Use pRect as and in/out parameter
#define DTT_APPLYOVERLAY    (1UL << 10)     // fApplyOverlay has been specified
#define DTT_GLOWSIZE        (1UL << 11)     // iGlowSize has been specified
#define DTT_CALLBACK        (1UL << 12)     // pfnDrawTextCallback has been specified
#define DTT_COMPOSITED      (1UL << 13)     // Draws text with antialiased alpha (needs a DIB section)
#define DTT_VALIDBITS       (DTT_TEXTCOLOR | DTT_BORDERCOLOR | DTT_SHADOWCOLOR | DTT_SHADOWTYPE | DTT_SHADOWOFFSET | DTT_BORDERSIZE | \
                             DTT_FONTPROP | DTT_COLORPROP | DTT_STATEID | DTT_CALCRECT | DTT_APPLYOVERLAY | DTT_GLOWSIZE | DTT_COMPOSITED)

typedef struct _DTTOPTS
{
    DWORD             dwSize;              // size of the struct
    DWORD             dwFlags;             // which options have been specified
    COLORREF          crText;              // color to use for text fill
    COLORREF          crBorder;            // color to use for text outline
    COLORREF          crShadow;            // color to use for text shadow
    int               iTextShadowType;     // TST_SINGLE or TST_CONTINUOUS
    POINT             ptShadowOffset;      // where shadow is drawn (relative to text)
    int               iBorderSize;         // Border radius around text
    int               iFontPropId;         // Font property to use for the text instead of TMT_FONT
    int               iColorPropId;        // Color property to use for the text instead of TMT_TEXTCOLOR
    int               iStateId;            // Alternate state id
    BOOL              fApplyOverlay;       // Overlay text on top of any text effect?
    int               iGlowSize;           // Glow radious around text
    DTT_CALLBACK_PROC pfnDrawTextCallback; // Callback for DrawText
    LPARAM            lParam;              // Parameter for callback
} DTTOPTS, *PDTTOPTS; 
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#endif
