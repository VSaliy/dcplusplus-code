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
      * Neither the name of the DWT nor the names of its contributors
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

#ifndef DWT_THEMED_H
#define DWT_THEMED_H

#include "Widget.h"
#include "CanvasClasses.h"

#include <uxtheme.h>

namespace dwt {

/** helper class a widget can derive from. this class:
- manages calls to visual styles APIs without having to link to uxtheme.lib.
- handles opening and closing the theme on destruction and on WM_THEMECHANGED.
*/
class Themed {
protected:
	explicit Themed(Widget* w_);
	virtual ~Themed();

	void loadTheme(LPCWSTR classes);

	/**
	* @param drawParent if false, you have to call isThemeBackgroundPartiallyTransparent and handle
	* drawing the transparent bits yourself.
	*/
	void drawThemeBackground(Canvas& canvas, int part, int state, const Rectangle& rect, bool drawParent = true);
	void drawThemeText(Canvas& canvas, int part, int state, const tstring& text, DWORD flags, const Rectangle& rect);
	bool isThemeBackgroundPartiallyTransparent(int part, int state);

	HTHEME theme;

private:
	void openTheme(LPCWSTR classes);
	void closeTheme();
	void themeChanged(LPCWSTR classes);

	Widget* w;
};

}

#endif
