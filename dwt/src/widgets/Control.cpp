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

#include <dwt/widgets/Control.h>

#include <dwt/DWTException.h>
#include <dwt/util/check.h>

namespace dwt {

Control::Seed::Seed(DWORD style, DWORD exStyle, const tstring& caption) :
BaseType::Seed(style | WS_VISIBLE, exStyle, caption)
{
}

Control::Control(Widget* parent, Dispatcher& dispatcher) :
BaseType(parent, dispatcher),
accel(0)
{
}

void Control::addAccel(BYTE fVirt, WORD key, const CommandDispatcher::F& f) {
	const size_t id = id_offset + accels.size();
	ACCEL a = { fVirt | FVIRTKEY, key, id };
	accels.push_back(a);
	onCommand(f, id);
}

void Control::initAccels() {
	accel = ::CreateAcceleratorTable(&accels[0], accels.size());
	if(!accel) {
		throw Win32Exception("Control::create: CreateAcceleratorTable failed");
	}
}

void Control::create(const Seed& seed) {
	BaseType::create(seed);

	if(!accels.empty()) {
		initAccels();
	}
}

Control::~Control() {
	if(accel) {
		::DestroyAcceleratorTable(accel);
	}
}

bool Control::filter(MSG& msg) {
	return accel && ::TranslateAccelerator(handle(), accel, &msg);
}

}
