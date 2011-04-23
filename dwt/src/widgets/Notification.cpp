/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2011, Jacek Sieka

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

static const UINT taskbarMsg = ::RegisterWindowMessage(_T("TaskbarCreated"));

Notification::Notification(WindowPtr parent_) :
parent(parent_),
visible(false),
balloons(0),
lastTick(0)
{
}

Notification::~Notification() {
	setVisible(false);
}

void Notification::create(const Notification::Seed& seed) {
	icon = seed.icon;
	tip = seed.tip;

	// TODO Allow more than one icon per window
	parent->setCallback(Message(message), [this](const MSG& msg, LRESULT&) { return trayHandler(msg); });
	parent->setCallback(Message(taskbarMsg), [this](const MSG&, LRESULT&) { return redisplay(); });
}

void Notification::setIcon(const IconPtr& icon_) {
	icon = icon_;
	redisplay();
}

void Notification::setVisible(bool visible_) {
	if(visible == visible_) {
		if(visible && balloons)
			balloons = 0;
		return;
	}

	visible = visible_;

	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA), parent->handle() };

	if(visible) {
		nid.uFlags = NIF_MESSAGE;
		nid.uCallbackMessage = message;

		if(!tip.empty()) {
			nid.uFlags |= NIF_TIP;
			tip.copy(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
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
		NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA), parent->handle() };
		nid.uFlags = NIF_TIP;
		tip.copy(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
	}
}

void Notification::addMessage(const tstring& title, const tstring& message) {
	if(!visible || balloons)
		++balloons;
	if(!visible)
		setVisible(true);

	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA), parent->handle() };
	nid.uFlags = NIF_INFO;
	message.copy(nid.szInfo, sizeof(nid.szInfo) / sizeof(nid.szInfo[0]) - 1);
	title.copy(nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]) - 1);
	nid.dwInfoFlags = NIIF_INFO;
	::Shell_NotifyIcon(NIM_MODIFY, &nid);
}

bool Notification::redisplay() {
	if(visible) {
		setVisible(false);
		setVisible(true);
	}
	return false;
}

// Mingw headers miss these
#ifndef NIN_BALLOONUSERCLICK
#define NIN_BALLOONHIDE (WM_USER + 3)
#define NIN_BALLOONTIMEOUT (WM_USER + 4)
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
#endif

bool Notification::trayHandler(const MSG& msg) {
	switch(msg.lParam) {
	case WM_LBUTTONUP:
		{
			if(iconClicked) {
				iconClicked();
			}
			break;
		}

	case WM_RBUTTONUP:
		{
			if(contextMenu) {
				// Work-around for windows bug (KB135788)
				::SetForegroundWindow(parent->handle());
				contextMenu();
				parent->postMessage(WM_NULL);
			}
			break;
		}

	case WM_MOUSEMOVE:
		{
			if(updateTip) {
				DWORD now = ::GetTickCount();
				if(now - 1000 > lastTick) {
					updateTip();
					lastTick = now;
				}
			}
			break;
		}

	case NIN_BALLOONUSERCLICK:
		{
			if(balloonClicked) {
				balloonClicked();
			}
		} // fall through
	case NIN_BALLOONHIDE: // fall through
	case NIN_BALLOONTIMEOUT:
		{
			if(balloons && !--balloons) {
				setVisible(false);
			}
			break;
		}
	}

	return true;
}

}
