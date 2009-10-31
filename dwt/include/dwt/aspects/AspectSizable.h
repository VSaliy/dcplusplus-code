/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2008, Jacek Sieka

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

#ifndef DWT_AspectSizable_h
#define DWT_AspectSizable_h

#include "../Widget.h"
#include "../Dispatchers.h"
#include "../Events.h"

namespace dwt {

/// \ingroup AspectClasses
/// \ingroup WidgetLayout
/// Aspect class used by Widgets that have the possibility of setting and getting a
/// "size" property of their objects.
/** E.g. the TextBox have a "size" Aspect therefore it realizes the
  * AspectSizable through inheritance. <br>
  * Note! <br>
  * All coordinates have zenith top-left corner of either the desktop display or the
  * client area of the parent Widget. <br>
  * Note! <br>
  * There are two different ways to calculate the position of a Widget, one is in
  * screen coordinates which starts top left of the desktop window, the other way is
  * relative to its parent Widget which starts at the top left of the parent Widgets
  * client area which is the total area of the Widget after the border, menu, toolbar
  * etc have been taken away. <br>
  * In addition all bounding Rectangles dealt with through this class are giving
  * their down right coordinates in SIZES and not in POSITIONS!
  */
template< class WidgetType >
class AspectSizable
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	typedef Dispatchers::ConvertBase<SizedEvent, &Dispatchers::convert<SizedEvent>, 0, false>  SizeDispatcher;
	typedef Dispatchers::ConvertBase<Point, &Point::fromMSG, 0, false> MoveDispatcher;

public:
	/// Returns the screen size.
	/** This is the screen size, and useful for making applications that must adapt
	 * to different screen sizes.
	 */
	static Point getDesktopSize();

	/// Brings the widget to the front
	/** Makes the widget become the front most widget meaning it will not be obscured
	  * by other widgets which are contained in the same container widget. <br>
	  * For instance if you have two widgets which partially hides eachother and you
	  * call bringToFront on one of them it will make sure that the widget you call
	  * bringToFront on will be the one which will be all visible and the other one
	  * will be partially hidden by the parts which are obscured by the this widget.
	  */
	void bringToFront();

	void centerWindow(Widget* target = 0);

	/// Brings the widget to the bottom
	/** Makes the widget become the bottom most widget meaning it will be obscured by
	  * all other widgets which are contained in the same container widget. <br>
	  * For instance if you have two widgets which partially hides eachother and you
	  * call bringToBottom on one of them it will make sure that the widget you call
	  * bringToBottom on will be the one which will be invisible and the other one
	  * will be all visible by the parts which are obscured by the this widget.
	  */
	void bringToBottom();

	bool isIconic();
	bool isZoomed();

	/// \ingroup EventHandlersAspectSizable
	// Setting the event handler for the "sized" event
	/** The size submitted to the event handler is the new client area size. The
	  * parameter passed is WidgetSizedEventResult which contains the new size
	  * information.
	  */
	void onSized(const typename SizeDispatcher::F& f) {
		W().addCallback(Message( WM_SIZE ), SizeDispatcher(f));
	}

	/// \ingroup EventHandlersAspectSizable
	// Setting the event handler for the "moved" event
	/** This event will be raised when the Widget is being moved. The parameter
	  * passed is Point which is the new position of the Widget
	  */
	void onMoved(const typename MoveDispatcher::F& f) {
		W().addCallback(Message( WM_MOVE ), MoveDispatcher(f));
	}

protected:
	virtual ~AspectSizable() { }
};

template< class WidgetType >
Point AspectSizable< WidgetType >::getDesktopSize()
{
	RECT rc;
	::GetWindowRect( ::GetDesktopWindow(), & rc );
	return Point( rc.right - rc.left, rc.bottom - rc.top );
}

template< class WidgetType >
void AspectSizable< WidgetType >::centerWindow( Widget* target ) {
	Point size = W().getWindowSize();
	RECT rc;
	if(!target) {
		target = static_cast<WidgetType*>(this)->getParent();
	}
	::GetWindowRect(target->handle(), &rc);
	W().layout(Rectangle(rc.left + (rc.right - rc.left)/2 - size.x/2, rc.top + (rc.bottom - rc.top)/2 - size.y/2, size.x, size.y));
}

template< class WidgetType >
void AspectSizable< WidgetType >::bringToFront()
{
	::SetWindowPos(H(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
}

template< class WidgetType >
void AspectSizable< WidgetType >::bringToBottom()
{
	::SetWindowPos(H(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
}

template< class WidgetType >
bool AspectSizable< WidgetType >::isIconic()
{
	return ::IsIconic(H()) > 0;
}

template< class WidgetType >
bool AspectSizable< WidgetType >::isZoomed()
{
	return ::IsZoomed(H()) > 0;
}

}

#endif
