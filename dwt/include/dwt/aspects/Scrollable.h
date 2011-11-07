/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2011, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

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

#ifndef DWT_aspects_Scrollable_h
#define DWT_aspects_Scrollable_h

#include "../Dispatchers.h"
#include "../DWTException.h"
#include "../Message.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that have the possibility of scrolling
/** \ingroup aspects::Classes
  * E.g. the Slider have a scroll Aspect to it therefore Slider realize
  * the aspects::Scrollable through inheritance.
  */
template< class WidgetType >
class Scrollable
{
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	typedef Dispatchers::VoidVoid<> ScrollableDispatcher;

public:
	/// @ refer to the ::GetScrollInfo doc for information on the params.
	SCROLLINFO getScrollInfo(int fnBar, int mask = SIF_ALL) const;
	bool scrollIsAtEnd() const;

	/// \ingroup EventHandlersaspects::Scrollable
	/// Setting the event handler for the "scrolling horizontally" event
	/** A scrolling event occurs when for instance a Sliders value is being
	  * manipulated through user interaction. Such an action would raise this event.
	  * <br>
	  * No parameters are passed.
	  */
	void onScrollHorz(const ScrollableDispatcher::F& f) {
		W().addCallback(Message( WM_HSCROLL ), ScrollableDispatcher(f));
	}

	/// \ingroup EventHandlersaspects::Scrollable
	/// Setting the event handler for the "scrolling vertically" event
	/** A scrolling event occurs when for instance a Sliders value is being
	  * manipulated through user interaction. Such an action would raise this event.
	  * <br>
	  * No parameters are passed.
	  */
	void onScrollVert(const ScrollableDispatcher::F& f) {
		W().addCallback(Message( WM_VSCROLL ), ScrollableDispatcher(f));
	}

private:
	/// override if the derived widget needs to be adjusted when determining the scroll pos.
	virtual int scrollOffsetImpl() const {
		return 0;
	}
};

template<class WidgetType>
SCROLLINFO Scrollable<WidgetType>::getScrollInfo(int fnBar, int mask) const {
	SCROLLINFO info = { sizeof(SCROLLINFO), mask };
	if(!::GetScrollInfo(H(), fnBar, &info)) {
		throw Win32Exception("aspects::Scrollable: Can't get scroll info");
	}
	return info;
}

template<class WidgetType>
bool Scrollable<WidgetType>::scrollIsAtEnd() const {
	SCROLLINFO scrollInfo = getScrollInfo(SB_VERT, SIF_RANGE | SIF_PAGE | SIF_POS);
	return !scrollInfo.nPage || scrollInfo.nPos >= static_cast<int>(scrollInfo.nMax - scrollInfo.nPage) + scrollOffsetImpl();
}

} }

#endif
