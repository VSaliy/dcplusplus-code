/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2013, Jacek Sieka

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

#ifndef DWT_VSSTYLE_H
#define DWT_VSSTYLE_H

#ifdef __GNUC__
// MinGW lacks vsstyle.h, so define what we need here

#define VSCLASS_LISTVIEW L"LISTVIEW"
#define LVP_GROUPHEADER 6
#define LVGH_OPEN 1

#define VSCLASS_MENU L"MENU"
#define MENU_BARBACKGROUND 7
#define MENU_BARITEM 8
#define MENU_POPUPBACKGROUND 9
#define MENU_POPUPCHECK 11
#define MENU_POPUPCHECKBACKGROUND 12
#define MENU_POPUPITEM 14
#define MENU_POPUPSEPARATOR 15
#define MENU_POPUPSUBMENU 16
#define MB_ACTIVE 1
#define MBI_NORMAL 1
#define MBI_HOT 2
#define MBI_PUSHED 3
#define MC_CHECKMARKNORMAL 1
#define MCB_NORMAL 2
#define MCB_BITMAP 3
#define MPI_NORMAL 1
#define MPI_HOT 2
#define MPI_DISABLED 3
#define MPI_DISABLEDHOT 4
#define MSM_NORMAL 1
#define MSM_DISABLED 2

#define VSCLASS_TAB L"TAB"
#define TABP_TABITEM 1
#define TIS_NORMAL 1
#define TIS_HOT 2
#define TIS_SELECTED 3

#define VSCLASS_TREEVIEW L"TREEVIEW"
#define TVP_GLYPH 2
#define GLPS_CLOSED 1
#define GLPS_OPENED 2

#define VSCLASS_WINDOW L"WINDOW"
#define WP_CAPTION 1
#define CS_ACTIVE 1

#else
#include <vsstyle.h>
#endif

#endif
