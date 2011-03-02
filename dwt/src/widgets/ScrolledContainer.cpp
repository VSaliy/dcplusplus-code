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

#include <dwt/widgets/ScrolledContainer.h>

namespace dwt {

Point ScrolledContainer::getPreferredSize() {
	auto child = getChild();
	return child ? child->getPreferredSize() : Point(0, 0);
}

void ScrolledContainer::layout(const Rectangle &rect) {
	BaseType::layout(rect);
	auto child = getChild();
	if(!child) {
		return;
	}

	auto clientSize = getClientSize();
	auto childSize = child->getPreferredSize();

	::ShowScrollBar(handle(), SB_HORZ, childSize.x > clientSize.x);
	::ShowScrollBar(handle(), SB_VERT, childSize.y > clientSize.y);

	clientSize = getClientSize();

	if(childSize.x > clientSize.x) {
		setScrollInfo(SB_HORZ, clientSize.x, childSize.x);
	}

	if(childSize.y > childSize.y) {
		setScrollInfo(SB_VERT, clientSize.y, childSize.y);
	}

	child->layout(Rectangle(childSize));
}

void ScrolledContainer::setScrollInfo(int type, int page, int max, int pos) {
	SCROLLINFO si = { sizeof(SCROLLINFO) };

	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = max - 1;
	si.nPos = pos;
	si.nPage = type == SB_HORZ ? getClientSize().x : getClientSize().y;
	::SetScrollInfo(handle(), type, &si, TRUE);
}

bool ScrolledContainer::handleMessage(const MSG &msg, LRESULT &retVal) {
	if(!(msg.message == WM_HSCROLL || msg.message == WM_VSCROLL)) {
		return false;
	}

	auto child = getChild();

	if(!child) {
		return false;
	}

	auto childSize = child->getWindowSize();

	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };

	auto type = msg.message == WM_HSCROLL ? SB_HORZ : SB_VERT;
    ::GetScrollInfo(handle(), type, &si);

    auto orig = si.nPos;

    switch (LOWORD (msg.wParam)) {
    case SB_TOP:
    	si.nPos = 0;
    	break;
    case SB_BOTTOM:
    	si.nPos = si.nMax;
    	break;
    case SB_LINELEFT:
        si.nPos -= 1;
        break;
    case SB_LINERIGHT:
        si.nPos += 1;
        break;
    case SB_PAGELEFT:
        si.nPos -= si.nPage;
        break;
    case SB_PAGERIGHT:
        si.nPos += si.nPage;
        break;
    case SB_THUMBTRACK:
        si.nPos = si.nTrackPos;
        break;
    }

    ::SetScrollInfo(handle(), type, &si, FALSE);

	::GetScrollInfo(handle(), type, &si);
	auto hDiff = type == SB_HORZ ? orig - si.nPos : 0;
	auto vDiff = type == SB_VERT ? orig - si.nPos : 0;

	::ScrollWindowEx(handle(), hDiff, vDiff, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);

	return true;
}

}
