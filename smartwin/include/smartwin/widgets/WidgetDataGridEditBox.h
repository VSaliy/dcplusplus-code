// $Revision: 1.8 $
/*
  Copyright (c) 2005, Thomas Hansen
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

	  * Redistributions of source code must retain the above copyright notice,
		this list of conditions and the following disclaimer.
	  * Redistributions in binary form must reproduce the above copyright notice, 
		this list of conditions and the following disclaimer in the documentation 
		and/or other materials provided with the distribution.
	  * Neither the name of the SmartWin++ nor the names of its contributors 
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
#ifndef WidgetDataGridViewEditBox_h
#define WidgetDataGridViewEditBox_h

#include "WidgetTextBox.h"
#include "WidgetWindowBase.h"

namespace SmartWin
{
// begin namespace SmartWin

namespace private_
{
// begin namespace private_

// Class is only to make subclassing of Edit Control in List View possible
// TODO: Make window NOT hide the leftmost cell of row when entering "edit modus"..
template< class EventHandlerClass >
class ListViewEditBox :
	public WidgetTextBox< EventHandlerClass >
{
public:
	// Class type
	typedef ListViewEditBox< EventHandlerClass > ThisType;

	// Object type
	typedef ListViewEditBox< EventHandlerClass > * ObjectType;

	void createSubclass( HWND hWnd );

	explicit ListViewEditBox( SmartWin::Widget * parent );

	virtual ~ListViewEditBox()
	{}

	Rectangle itsRect;

private:
	virtual LRESULT sendWidgetMessage( HWND hWnd, UINT msg, WPARAM & wPar, LPARAM & lPar );
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template< class EventHandlerClass >
ListViewEditBox< EventHandlerClass >::ListViewEditBox( SmartWin::Widget * parent )
	: Widget( parent ), WidgetTextBox< EventHandlerClass >( parent )
{
	// Can't have a text box without a parent...
	xAssert( parent, _T( "Cant have a TextBox without a parent..." ) );
}

template< class EventHandlerClass >
void ListViewEditBox< EventHandlerClass >::createSubclass( HWND hWnd )
{
	this->Widget::itsHandle = hWnd;
	::SetWindowLongPtr( this->Widget::itsHandle, GWL_ID, reinterpret_cast< LONG >( this->Widget::itsCtrlId ) );
	this->Widget::itsCtrlId = reinterpret_cast< HMENU >( GetWindowLongPtr( this->Widget::itsHandle, GWL_ID ) );

	// Have to cast to Widget, it looks weird but otherwise we can't extract it
	// since we're using reinterpret to get it back again...
	::SetProp( this->Widget::itsHandle, _T( "_mainWndProc" ), reinterpret_cast< HANDLE >( static_cast< Widget * >( this ) ) );
	MessageMapControl< EventHandlerClass, WidgetTextBox< EventHandlerClass > >::itsDefaultWindowProc = reinterpret_cast< WNDPROC >( ::SetWindowLongPtr( this->Widget::itsHandle, GWL_WNDPROC, reinterpret_cast< LONG_PTR >( WidgetWindowBase< EventHandlerClass, MessageMapPolicyModalDialogWidget >::wndProc ) ) );
	Widget::registerWidget();
}

template< class EventHandlerClass >
LRESULT ListViewEditBox< EventHandlerClass >::sendWidgetMessage( HWND hWnd, UINT msg, WPARAM & wPar, LPARAM & lPar )
{
#ifdef _MSC_VER
#pragma warning( disable : 4060 )
#endif
	switch ( msg )
	{
			// Windows CE doesn't support WM_WINDOWPOSCHANGING message
			// therefore we can't reposition the EditControl in the listview
			// edit modus...!
#ifndef WINCE
		case WM_WINDOWPOSCHANGING :
		{
			// Ensuring position is "locked" in the rectangle initially set in
			// WidgetDataGrid's create func And then letting message "pass
			// through" the rest of the hierarchy...
			WINDOWPOS * pos = ( WINDOWPOS * ) lPar;
			pos->x = itsRect.pos.x;
			pos->y = itsRect.pos.y;
			pos->cx = itsRect.size.x - itsRect.pos.x;
			pos->cy = itsRect.size.y - itsRect.pos.y;
		} break;
#endif
	}
	return WidgetTextBox< EventHandlerClass >::sendWidgetMessage( hWnd, msg, wPar, lPar );
}

// end namespace private_
}

// end namespace SmartWin
}

#endif
