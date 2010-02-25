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

static LibraryLoader& lib() {
	static LibraryLoader l(_T("uxtheme"), true);
	return l;
}

Themed::Themed(Widget* w_) : theme(0), w(w_)
{
	dwtassert(w, _T("Themed: no widget set"));
}

Themed::~Themed() {
	if(theme) {
		closeTheme();
	}
}

void Themed::loadTheme(LPCWSTR classes) {
	if(lib().loaded() && reinterpret_cast<BOOL (*)()>(lib().getProcAddress(_T("IsAppThemed")))()) {
		openTheme(classes);
		if(theme) {
			w->addCallback(Message(WM_THEMECHANGED), Dispatchers::VoidVoid<>(std::tr1::bind(&Themed::themeChanged, this, classes)));
		}
	}
}

void Themed::drawThemeBackground(Canvas& canvas, int part, int state, const Rectangle& rect, bool drawParent) {
	::RECT rc = rect;
	if(drawParent && isThemeBackgroundPartiallyTransparent(part, state)) {
		reinterpret_cast<HRESULT (*)(HWND, HDC, const RECT*)>(
			lib().getProcAddress(_T("DrawThemeParentBackground")))(w->handle(), canvas.handle(), &rc);
	}
	reinterpret_cast<HRESULT (*)(HTHEME, HDC, int, int, const RECT*, const RECT*)>(
		lib().getProcAddress(_T("DrawThemeBackground")))(theme, canvas.handle(), part, state, &rc, 0);
}

void Themed::drawThemeText(Canvas& canvas, int part, int state, const tstring& text, DWORD flags, const Rectangle& rect) {
	::RECT rc = rect;
	reinterpret_cast<HRESULT (*)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, DWORD, LPCRECT)>(
			lib().getProcAddress(_T("DrawThemeText")))(theme, canvas.handle(), part, state, text.c_str(), text.size(), flags, 0, &rc);
}

bool Themed::isThemeBackgroundPartiallyTransparent(int part, int state) {
	return reinterpret_cast<BOOL (*)(HTHEME, int, int)>(
		lib().getProcAddress(_T("IsThemeBackgroundPartiallyTransparent")))(theme, part, state);
}

void Themed::openTheme(LPCWSTR classes) {
	theme = reinterpret_cast<HWND (*)(HTHEME, LPCWSTR)>(lib().getProcAddress(_T("OpenThemeData")))(w->handle(), classes);
}

void Themed::closeTheme() {
	reinterpret_cast<HRESULT (*)(HTHEME)>(lib().getProcAddress(_T("CloseThemeData")))(theme);
}

void Themed::themeChanged(LPCWSTR classes) {
	if(theme) {
		closeTheme();
		openTheme(classes);
	}
}

}
