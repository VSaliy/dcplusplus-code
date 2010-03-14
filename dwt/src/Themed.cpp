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

#include <dwt/Themed.h>

#include <dwt/Dispatchers.h>
#include <dwt/LibraryLoader.h>
#include <dwt/util/check.h>

namespace dwt {

typedef HRESULT (WINAPI *t_CloseThemeData)(HTHEME);
static t_CloseThemeData CloseThemeData;

typedef HRESULT (WINAPI *t_DrawThemeBackground)(HTHEME, HDC, int, int, const RECT*, const RECT*);
static t_DrawThemeBackground DrawThemeBackground;

typedef HRESULT (WINAPI *t_DrawThemeParentBackground)(HWND, HDC, const RECT*);
static t_DrawThemeParentBackground DrawThemeParentBackground;

typedef HRESULT (WINAPI *t_DrawThemeText)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, DWORD, LPCRECT);
static t_DrawThemeText DrawThemeText;

typedef HRESULT (WINAPI *t_GetThemePartSize)(HTHEME, HDC, int, int, LPCRECT, THEMESIZE, SIZE*);
static t_GetThemePartSize GetThemePartSize;

typedef BOOL (WINAPI *t_IsAppThemed)();
static t_IsAppThemed IsAppThemed;

typedef BOOL (WINAPI *t_IsThemeBackgroundPartiallyTransparent)(HTHEME, int, int);
static t_IsThemeBackgroundPartiallyTransparent IsThemeBackgroundPartiallyTransparent;

typedef HWND (WINAPI *t_OpenThemeData)(HTHEME, LPCWSTR);
static t_OpenThemeData OpenThemeData;

Themed::Themed(Widget* w_) : theme(0), w(w_)
{
	dwtassert(w, _T("Themed: no widget set"));
}

Themed::~Themed() {
	closeTheme();
}

void Themed::loadTheme(LPCWSTR classes, bool handleThemeChanges) {
	static LibraryLoader lib(_T("uxtheme"), true);
	if(lib.loaded()) {

#define get(name) if(!name) { if(!(name = reinterpret_cast<t_##name>(lib.getProcAddress(_T(#name))))) { return; } }
		get(CloseThemeData);
		get(DrawThemeBackground);
		get(DrawThemeParentBackground);
		get(DrawThemeText);
		get(GetThemePartSize);
		get(IsAppThemed);
		get(IsThemeBackgroundPartiallyTransparent);
		get(OpenThemeData);
#undef get

		openTheme(classes);

		if(handleThemeChanges) {
			w->addCallback(Message(WM_THEMECHANGED),
				Dispatchers::VoidVoid<0, false>(std::tr1::bind(&Themed::themeChanged, this, classes)));
		}
	}
}

void Themed::drawThemeBackground(Canvas& canvas, int part, int state, const Rectangle& rect, bool drawParent) {
	::RECT rc = rect;
	if(drawParent && isThemeBackgroundPartiallyTransparent(part, state)) {
		DrawThemeParentBackground(w->handle(), canvas.handle(), &rc);
	}
	DrawThemeBackground(theme, canvas.handle(), part, state, &rc, 0);
}

void Themed::drawThemeText(Canvas& canvas, int part, int state, const tstring& text, DWORD flags, const Rectangle& rect) {
	::RECT rc = rect;
	DrawThemeText(theme, canvas.handle(), part, state, text.c_str(), text.size(), flags, 0, &rc);
}

bool Themed::getThemePartSize(Canvas& canvas, int part, int state, Point& ret) {
	SIZE size;
	if(GetThemePartSize(theme, canvas.handle(), part, state, 0, TS_TRUE, &size) == S_OK) {
		ret.x = size.cx;
		ret.y = size.cy;
		return true;
	}
	return false;
}

bool Themed::isThemeBackgroundPartiallyTransparent(int part, int state) {
	return IsThemeBackgroundPartiallyTransparent(theme, part, state);
}

void Themed::openTheme(LPCWSTR classes) {
	if(IsAppThemed()) {
		theme = OpenThemeData(w->handle(), classes);
	}
}

void Themed::closeTheme() {
	if(theme) {
		CloseThemeData(theme);
		theme = 0;
	}
}

void Themed::themeChanged(LPCWSTR classes) {
	closeTheme();
	openTheme(classes);
}

}
