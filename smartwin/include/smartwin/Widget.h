// $Revision: 1.12 $
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
#ifndef Widget_h
#define Widget_h

#include "WindowsHeaders.h"
#include "../SmartUtil.h"
#include "CallbackFuncPrototypes.h"
#include "Message.h"
#include "Threads.h"
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>

namespace SmartWin
{
// begin namespace SmartWin

class Application;

namespace private_
{
	// Helper function to be able to set itsHandle to Widget in WM_NCCREATE message
	// Since we don't want to have it public and cannot have it a friend (templates
	// can't be friends)
	void setHandle( Widget * widget, HWND handle );
}

/// Abstract Base class for all Widgets
/** Basically (almost) all Widgets derive from this class, this is the root class for
  * (almost) every single Widget type. <br>
  * This class contains the functionality that all Widgets must or should have
  * support for. <br>
  * E.g. the handle to the specific Widgets are contained here, plus all the
  * commonalities between Widgets <br>
  * The Widget class inherits from boost::noncopyable to indicate it's not to be
  * copied
  */
class Widget
	: public boost::noncopyable
{
	friend void private_::setHandle( Widget * widget, HWND handle );
public:
	/// Returns the HWND to the Widget
	/** Returns the HWND to the inner window of the Widget. <br>
	  * If you need to do directly manipulation of the window use this function to
	  * retrieve the HWND of the Widget.
	  */
	HWND handle() const
	{ return itsHandle;
	}

	/// Returns a CriticalSection associated with the current Widget object
	/** If you need serialized thread safe access to the Widget call this function
	  * and either stuff the returned object into a Utilities::ThreadLock or call
	  * Utilities::CriticalSection::lock (then you manually have to ensure
	  * CriticalSection::unlock is called on it)
	  */
	Utilities::CriticalSection & getCriticalSection();

	/// Returns the control id of the Widget
	/** This one only makes sense for control items, e.g. WidgetButton,
	  * WidgetComboBox etc. <br>
	  * Every control in a Widget has got its own control ID, mark that for a
	  * WidgetWindow this will always be ZERO
	  */
	HMENU getCtrlId() const
	{ return itsCtrlId;
	}

	// TODO These need to be moved to an appropriate location so that they're only
	// available when the handle is a HWND...
	/// Send a message to the Widget
	/** If you need to be able to send a message to a Widget then use this function
	  * as it will unroll into <br>
	  * a ::SendMessage from the Windows API
	  */
	LRESULT sendMessage( UINT msg, WPARAM wParam = 0, LPARAM lParam = 0 ) {
		return ::SendMessage(handle(), msg, wParam, lParam);
	}
	
	bool postMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) {
		return ::PostMessage(handle(), msg, wParam, lParam);
	}

	// TODO: Change all references to typedefed WidgetPtr...??
	/// Returns the parent Widget of the Widget
	/** Most Widgets have got a parent, this function will retrieve a pointer to the
	  * Widgets parent, if the Widget doesn't have a parent it will return a null
	  * pointer.
	  */
	Widget * getParent() const
	{ return itsParent;
	}

	/// Repaints the whole window
	/** Invalidate the window and repaints it.
	  */
	void updateWidget();

	/// Add this widget to the update area.
	/** Same as updateWidget except that this does not force an immediate redraw.
	  */
	void invalidateWidget();

	/// Subclasses the dialog item with the given dialog item id
	/** Subclasses a dialog item, the id is the dialog item id from the resource
	  * editor. <br>
	  * Should normally not be called directly but rather called from e.g. one of the
	  * creational functions found in the WidgetFactory class.
	  */
	virtual void subclass( unsigned id );

	/// Use this function to add or remove windows styles.
	/** The first parameter is the type of style you wish to add/remove. <br>
	  * The second argument is a boolean indicating if you wish to add or remove the
	  * style (if true add style, else remove)
	  */
	void addRemoveStyle( DWORD addStyle, bool add );

	/// Use this function to add or remove windows exStyles.
	/** The first parameter is the type of style you wish to add/remove. <br>
	  * The second argument is a boolean indicating if you wish to add or remove the
	  * style (if true add style, else remove)
	  */
	void addRemoveExStyle( DWORD addStyle, bool add );

	bool clientToScreen(POINT& pt) { return ::ClientToScreen(handle(), &pt); }
	bool screenToClient(POINT& pt) { return ::ScreenToClient(handle(), &pt); }
protected:
	// TODO: Can we remove these two?!?!?
	bool isChild;

	// TODO: Can these become PRIVATE?!?!?
	static int itsInstanceNo;
	HWND itsHandle;
	Widget * itsParent;
	HMENU itsCtrlId;
	std::vector < Widget * > itsChildren; // Derived widgets might need access to the children

	// Widget will almost ALLWAYS be deleted from a pointer to a Widget
	virtual ~Widget();

	// While the two last parameters here hanve defaults,
	// the first one does not to ensure that anyone constructing 
	// Widgets doesn't forget to initialize this virtually inherited
	// class correctly...
	// See: http://geneura.ugr.es/~jmerelo/c++-faq/multiple-inheritance.html#faq-25.12
	Widget( Widget * parent, HWND hWnd = NULL, bool doReg = true );

	// Creates the Widget, should NOT be called directly but overridden in the
	// derived class (with no parameters)
	virtual void create( const SmartWin::Seed & );

	virtual void registerWidget();

	// Kills the "this" Widget
	void killMe();

	// Kills all children to the this Widget
	void killChildren();

	// Erases the "this" widget from its parent's list of children.
	void eraseMeFromParentsChildren();

private:
	friend class Application;

	std::auto_ptr< Utilities::CriticalSection > itsCriticalSection;

	// Kills the Widget, wrapper around "delete this" plus some other logic
	void kill();
};

// end namespace SmartWin
}

#endif
