/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2008, Jacek Sieka

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

#include <dwt/widgets/ModelessDialog.h>
#include <dwt/widgets/ModalDialog.h>
#include <dwt/DWTException.h>

namespace dwt {

static struct DLGTEMPLATEEX {
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
    WORD menu;
    WORD windowClass;
    WORD title;
    WORD pointsize;
    WORD weight;
    BYTE italic;
    BYTE charset;
    WCHAR typeface[13];
} defaultTemplate = {
	1,
	0xffff,
	0,
	0,
	DS_SHELLFONT | DS_CONTROL | WS_CHILD | WS_CAPTION,
	0,
	0,
	0,
	100,
	100,
	0,
	0,
	0,
	8,
	0,
	0,
	0,
	L"MS Shell Dlg"
};

void ModelessDialog::createDialog( unsigned resourceId )
{
	HWND wnd = ::CreateDialogParam( ::GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId),
		getParentHandle(), (DLGPROC)&ThisType::wndProc, toLParam());

	if ( !wnd ) {
		throw Win32Exception("CreateDialogParam failed");
	}
}

void ModelessDialog::createDialog() {
	// Default template followed by a bunch of 0's for menu, class, title and font
	DLGTEMPLATEEX t = defaultTemplate;

	HWND dlg = ::CreateDialogIndirectParam(::GetModuleHandle(NULL), (DLGTEMPLATE*)&t,
		getParentHandle(), (DLGPROC)&ThisType::wndProc, toLParam());

	if(dlg == NULL) {
		throw Win32Exception("CreateDialogIndirectParam failed");
	}
}

}
