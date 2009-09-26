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
#include "../util/check.h"
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
	/// Sets the new size and position of the window
	/** The input parameter Rectangle defines the new size (and position) of the
	  * window. <br>
	  * The pos member of the Rectangle is the position and the size member is the
	  * size. <br>
	  * So a call to this function will (probably) also MOVE your Widget too.
	  *
	  * For a top-level window, the position and dimensions are relative to the
	  * upper-left corner of the screen. For a child window, they are relative
	  * to the upper-left corner of the parent window's client area.
	  */
	void setBounds( const Rectangle & rect, bool updateWindow = true );

	/// Sets the new size and position of the window
	/** The input parameter newPos of type Point defines the new position of the
	  * window. <br>
	  * The newSize member of type Point is the new size of the window. <br>
	  * A call to this function will (probably) also MOVE your Widget too.
	  */
	void setBounds( const Point & newPos, const Point & newSize, bool updateWindow = true );

	/// Sets the new size and position of the window
	/** x is the new horizontal position of your window. <br>
	  * y is the new vertical position of your window. <br>
	  * width is the new width and height is the new height of your window. <br>
	  * Zenith is as in all other bounds function top/left. <br>
	  * A call to this function will (probably) also MOVE your Widget too.
	  */
	void setBounds( int x, int y, int width, int height, bool updateWindow = true );

	/// Returns the screen size.
	/** This is the screen size, and useful for making applications that must adapt
	  * to different screen sizes.
	  */
	static Point getDesktopSize();

	/// Returns the position and size of the window.
	/** Note that this is in screen coordinates meaning the position returned is
	  * relative to the upper left corner  of the desktop screen, the function also
	  * returns in the size member of the Rectangle the size of the window and not
	  * the position of the lower right point. Values includes borders, frames and
	  * toolbar etc of the window.
	  * Note that if you don't override the default parameter adjustForParent it will
	  * adjust for the parent meaning it will return in client coordinates instead of screen coordinates.
	  */
	Rectangle getBounds( bool adjustForParent = true ) const;

	/// Returns the size of the window.
	/** Includes the border, frame and toolbar etc of the window.
	  */
	Point getSize() const;

	/// Returns the position of the window relative to the parent window.
	/** Note that this is in client coordinates.
	*/
	Point getPosition() const;

	/// Returns the screen position of the window.
	Point getScreenPosition() const;

	/// Returns the size of the client area of the window.
	/** This differs from getSize because it disregards the border and headers, this
	  * function only returns the client area of the Widget meaning the area which it
	  * is possible to draw on.
	  */
	Point getClientAreaSize() const;

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
void AspectSizable< WidgetType >::setBounds( const Rectangle & rect, bool updateWindow) {
	setBounds(rect.x(), rect.y(), rect.width(), rect.height(), updateWindow);
}

template< class WidgetType >
void AspectSizable< WidgetType >::setBounds( const Point & newPos, const Point & newSize, bool updateWindow ) {
	setBounds(newPos.x, newPos.y, newSize.x, newSize.y, updateWindow);
}

template< class WidgetType >
void AspectSizable< WidgetType >::setBounds( int x, int y, int width, int height, bool updateWindow ) {
	if ( ::MoveWindow( H(), x, y, width, height, updateWindow ? TRUE : FALSE ) == 0 ) {
		dwtDebugFail("Couldn't reposition windows");
	}
}

template< class WidgetType >
void AspectSizable< WidgetType >::centerWindow( Widget* target ) {
	Point size = this->getSize();
	RECT rc;
	if(!target) {
		target = static_cast<WidgetType*>(this)->getParent();
	}
	::GetWindowRect(target->handle(), &rc);
	this->setBounds(rc.left + (rc.right - rc.left)/2 - size.x/2, rc.top + (rc.bottom - rc.top)/2 - size.y/2, size.x, size.y);
}

template< class WidgetType >
Point AspectSizable< WidgetType >::getDesktopSize()
{
	RECT rc;
	::GetWindowRect( ::GetDesktopWindow(), & rc );
	return Point( rc.right - rc.left, rc.bottom - rc.top );
}

template< class WidgetType >
Rectangle AspectSizable< WidgetType >::getBounds( bool adjustForParent ) const
{
	int width, height;
	RECT rc;
	POINT pt;
	HWND hwnd = H();
	::GetWindowRect( hwnd, & rc );
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;
	pt.x = rc.left;
	pt.y = rc.top;
	if( adjustForParent )
	{
		Widget* parent = W().getParent();
		if( parent )
		{
			//if it's a child, adjust coordinates relative to parent
			::ScreenToClient( parent->handle(), &pt );
		}
	}
	return Rectangle( pt.x, pt.y, width, height );
}

template< class WidgetType >
Point AspectSizable< WidgetType >::getSize() const
{
	return this->getBounds().size;
}

template< class WidgetType >
Point AspectSizable< WidgetType >::getPosition() const
{
	return this->getBounds().pos;
}

template< class WidgetType >
Point AspectSizable< WidgetType >::getScreenPosition() const
{
	RECT rc;
	::GetWindowRect( H(), & rc );
	return Point( rc.left, rc.top );
}

template< class WidgetType >
Point AspectSizable< WidgetType >::getClientAreaSize() const
{
	RECT rc;
	::GetClientRect( H(), & rc );
	return Point( rc.right, rc.bottom );
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
