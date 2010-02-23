/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2010, Jacek Sieka

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

//
//  TABSTYLE class parts and states 
//
#define VSCLASS_TABSTYLE	L"TABSTYLE"
#define VSCLASS_TAB	L"TAB"

enum TABPARTS {
	TABP_TABITEM = 1,
	TABP_TABITEMLEFTEDGE = 2,
	TABP_TABITEMRIGHTEDGE = 3,
	TABP_TABITEMBOTHEDGE = 4,
	TABP_TOPTABITEM = 5,
	TABP_TOPTABITEMLEFTEDGE = 6,
	TABP_TOPTABITEMRIGHTEDGE = 7,
	TABP_TOPTABITEMBOTHEDGE = 8,
	TABP_PANE = 9,
	TABP_BODY = 10,
	TABP_AEROWIZARDBODY = 11,
};

#define TABSTYLEPARTS TABPARTS;

enum TABITEMSTATES {
	TIS_NORMAL = 1,
	TIS_HOT = 2,
	TIS_SELECTED = 3,
	TIS_DISABLED = 4,
	TIS_FOCUSED = 5,
};

enum TABITEMLEFTEDGESTATES {
	TILES_NORMAL = 1,
	TILES_HOT = 2,
	TILES_SELECTED = 3,
	TILES_DISABLED = 4,
	TILES_FOCUSED = 5,
};

enum TABITEMRIGHTEDGESTATES {
	TIRES_NORMAL = 1,
	TIRES_HOT = 2,
	TIRES_SELECTED = 3,
	TIRES_DISABLED = 4,
	TIRES_FOCUSED = 5,
};

enum TABITEMBOTHEDGESTATES {
	TIBES_NORMAL = 1,
	TIBES_HOT = 2,
	TIBES_SELECTED = 3,
	TIBES_DISABLED = 4,
	TIBES_FOCUSED = 5,
};

enum TOPTABITEMSTATES {
	TTIS_NORMAL = 1,
	TTIS_HOT = 2,
	TTIS_SELECTED = 3,
	TTIS_DISABLED = 4,
	TTIS_FOCUSED = 5,
};

enum TOPTABITEMLEFTEDGESTATES {
	TTILES_NORMAL = 1,
	TTILES_HOT = 2,
	TTILES_SELECTED = 3,
	TTILES_DISABLED = 4,
	TTILES_FOCUSED = 5,
};

enum TOPTABITEMRIGHTEDGESTATES {
	TTIRES_NORMAL = 1,
	TTIRES_HOT = 2,
	TTIRES_SELECTED = 3,
	TTIRES_DISABLED = 4,
	TTIRES_FOCUSED = 5,
};

enum TOPTABITEMBOTHEDGESTATES {
	TTIBES_NORMAL = 1,
	TTIBES_HOT = 2,
	TTIBES_SELECTED = 3,
	TTIBES_DISABLED = 4,
	TTIBES_FOCUSED = 5,
};

#else
#include <vsstyle.h>
#endif

#endif
