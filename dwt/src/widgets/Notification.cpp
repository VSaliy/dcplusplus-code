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

#include <dwt/widgets/Notification.h>
#include <dwt/Application.h>

namespace dwt {

const UINT Notification::message = ::RegisterWindowMessage(_T("dwt::Notification"));

static const UINT taskbar = ::RegisterWindowMessage(_T("TaskbarCreated"));

Notification::~Notification() {
	setVisible(false);
}

void Notification::create(const Notification::Seed& seed) {
	icon = seed.icon;
	tip = seed.tip;

	// TODO Allow more than one icon per window
	parent->setCallback(Message(message), std::tr1::bind(&Notification::trayHandler, this, _1, _2));
	parent->setCallback(Message(taskbar), std::tr1::bind(&Notification::redisplay, this, _1, _2));
}

void Notification::setVisible(bool visible_) {
	if(visible == visible_)
		return;

	visible = visible_;

	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };

	nid.hWnd = parent->handle();

	if(visible) {
		nid.uFlags = NIF_MESSAGE;
		nid.uCallbackMessage = message;

		if(!tip.empty()) {
			nid.uFlags |= NIF_TIP;
			tip.copy(nid.szTip, (sizeof(nid.szTip) / sizeof(nid.szTip[0])) - 1);
		}

		if(icon) {
			nid.uFlags |= NIF_ICON;
			nid.hIcon = icon->handle();
		}

		::Shell_NotifyIcon(NIM_ADD, &nid);
	} else {
		::Shell_NotifyIcon(NIM_DELETE, &nid);
	}
}

void Notification::setTooltip(const tstring& tip_) {
	tip = tip_;
	lastTick = ::GetTickCount();

	if(visible) {
		NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
		nid.hWnd = parent->handle();
		nid.uFlags = NIF_TIP;
		tip.copy(nid.szTip, (sizeof(nid.szTip) / sizeof(nid.szTip[0])) - 1);
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
	}
}

bool Notification::redisplay(const MSG& msg, LPARAM& result) {
	if(visible) {
		setVisible(false);
		setVisible(true);
	}
	return false;
}

// Mingw headers miss this
#ifndef NIN_BALLOONUSERCLICK
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
#endif

bool Notification::trayHandler(const MSG& msg, LPARAM& result) {
	switch(msg.lParam) {
	case WM_LBUTTONUP: {
		if(iconClicked) {
			iconClicked();
		}
	} break;
	case WM_RBUTTONUP: {
		if(contextMenu) {
			// Work-around for windows bug (KB135788)
			::SetForegroundWindow(parent->handle());
			contextMenu();
			parent->postMessage(WM_NULL);
		}
	} break;
	case WM_MOUSEMOVE: {
		if(updateTip) {
			DWORD now = ::GetTickCount();
			if(now - 1000 > lastTick) {
				updateTip();
				lastTick = now;
			}
		}
	}

	case NIN_BALLOONUSERCLICK: {
		if(notificationClicked) {
			notificationClicked();
		}
	} break;
	}

	return true;
}

}
