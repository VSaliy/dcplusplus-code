/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2009, Jacek Sieka

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

#include <dwt/widgets/ModalDialog.h>
#include <dwt/DWTException.h>
#include <dwt/Application.h>
#include <dwt/util/win32/DlgTemplateEx.h>

namespace dwt {

ModalDialog::Seed::Seed(const Point& size_, DWORD styles_) :
size(size_),
styles(styles_)
{
}

ModalDialog::ModalDialog(Widget* parent) :
BaseType(parent),
quit(false),
ret(0)
{
	onClosing(std::tr1::bind(&ThisType::defaultClosing, this));

	filterIter = dwt::Application::instance().addFilter(std::tr1::bind(&ThisType::filter, this, _1));
}

ModalDialog::~ModalDialog() {
	dwt::Application::instance().removeFilter(filterIter);
}

void ModalDialog::create(unsigned resourceId) {
	HWND dlg = ::CreateDialogParam(::GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId),
		getParentHandle(), (DLGPROC)&ThisType::wndProc, toLParam());

	if(dlg == NULL) {
		throw Win32Exception("Couldn't create modal dialog");
	}
}

void ModalDialog::create(const Seed& cs) {
	util::win32::DLGTEMPLATEEX t = util::win32::defaultTemplate;
	t.style |= DS_MODALFRAME | WS_POPUP | WS_SYSMENU | cs.styles;
	t.cx = cs.size.x;
	t.cy = cs.size.y;

	HWND dlg = ::CreateDialogIndirectParam(::GetModuleHandle(NULL), (DLGTEMPLATE*)&t,
		getParentHandle(), (DLGPROC)&ThisType::wndProc, toLParam());

	if(dlg == NULL) {
		throw Win32Exception("Couldn't create modal dialog");
	}
}

int ModalDialog::show() {
	bool enabled = false;

	if(getParent()) {
		enabled = !::EnableWindow(getParentHandle(), FALSE);
	}

	setVisible(true);

	while(!quit) {
		if(!Application::instance().dispatch()) {
			quit |= !Application::instance().sleep();
		}
	}

	if(enabled) {
		::EnableWindow(getParentHandle(), TRUE);
	}

	::DestroyWindow(handle());

	return ret;
}

bool ModalDialog::filter(MSG& msg) {
	if(::IsDialogMessage(handle(), &msg)) {
		return true;
	}
	return false;
}

}

