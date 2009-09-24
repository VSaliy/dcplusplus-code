/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2009, Jacek Sieka

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

#ifndef DWT_Spinner_h
#define DWT_Spinner_h

#include "../aspects/AspectPainting.h"
#include "../aspects/AspectScrollable.h"
#include "Control.h"
#include "../Dispatchers.h"

namespace dwt {

/** sideeffect=\par Side Effects:
  */
/// Spinner Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html spinner.PNG
  * Class for creating a Spinner Control Widget. <br>
  * A Spinner is a Widget used to manipulate an integer value. A good example is the
  * volume control of a music  player which has two buttons, one for louder and the
  * other for softer.
  */
class Spinner :
	public CommonControl,
	// Aspects
	public AspectPainting< Spinner >,
	public AspectScrollable< Spinner >
{
	typedef CommonControl BaseType;
	friend class WidgetCreator< Spinner >;

	struct Dispatcher : Dispatchers::Base<bool (int, int)> {
		typedef Dispatchers::Base<bool (int, int)> BaseType;
		Dispatcher(const F& f_) : BaseType(f_) { }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			LPNMUPDOWN lpnmud = reinterpret_cast<LPNMUPDOWN>(msg.lParam);
			ret = !f(lpnmud->iPos, lpnmud->iDelta);
			return true;
		}
	};

public:
	/// Class type
	typedef Spinner ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		int minValue;
		int maxValue;

		Widget* buddy;

		/// Fills with default parameters
		Seed(int minValue_ = UD_MINVAL, int maxValue_ = UD_MAXVAL, Widget* buddy_ = 0);
	};

	/// Sets the range of the Spinner
	/** The range is the unique values of the control, use this function to set the
	  * range of the control. Maximum values are 65000 and minimum are -65000
	  */
	void setRange( int minimum, int maximum );

	/// Assigns a buddy control
	/** A "buddy control" can theoretically be any type of control, but the most
	  * usually type of control to assign is probably either a TextBox or a
	  * Label. <br>
	  * A buddy control is a control which (can) receive automatically the text value
	  * (setText) automatically when the value of the spinner changes. <br>
	  * And if you change the value of the buddy control the Spinner Control will
	  * automatically also change its value.
	  */
	void assignBuddy(Widget* buddy);

	Widget* getBuddy() const;

	/// Returns the value of the control
	/** The value can be any value between the minimum and maximum range defined in
	  * the setRange function
	  */
	int getValue();

	/// Sets the value of the control
	/** The value can be any value between the minimum and maximum range defined in
	  * the setRange function
	  */
	int setValue( int v );

	void onUpdate(const Dispatcher::F& f);

	/// Actually creates the Spinner Control
	/** You should call WidgetFactory::createSpinner if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create(const Seed &cs = Seed());

	virtual void layout(const Rectangle&);

protected:
	// Constructor Taking pointer to parent
	explicit Spinner( Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~Spinner() { }
private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void Spinner::setRange( int minimum, int maximum )
{
	sendMessage(UDM_SETRANGE32, static_cast< WPARAM >( minimum ), static_cast< LPARAM >( maximum ));
}

inline void Spinner::assignBuddy(Widget* buddy) {
	assert(buddy && buddy->handle() && buddy->getParent() == getParent());
	sendMessage(UDM_SETBUDDY, reinterpret_cast< WPARAM >( buddy->handle() ));
}

inline Widget* Spinner::getBuddy() const {
	return hwnd_cast<Widget*>(reinterpret_cast<HWND>(sendMessage(UDM_GETBUDDY)));
}

inline int Spinner::getValue()
{
#ifdef WINCE
	LRESULT retVal = sendMessage(UDM_GETPOS);
	if ( HIWORD( retVal ) != 0 )
	{
		dwtWin32DebugFail(" Something went wrong while trying to retrieve value if Spinner");
	}
	return LOWORD( retVal );
#else
	return sendMessage(UDM_GETPOS32);
#endif //! WINCE
}

inline int Spinner::setValue( int v )
{
#ifdef WINCE
	return sendMessage(UDM_SETPOS, 0, v);
#else
	return sendMessage(UDM_SETPOS32, 0, v);
#endif
}

inline void Spinner::onUpdate(const Dispatcher::F& f) {
	setCallback(Message(WM_NOTIFY, UDN_DELTAPOS), Dispatcher(f));
}

inline Spinner::Spinner(dwt::Widget * parent )
	: BaseType(parent, ChainingDispatcher::superClass<Spinner>())
{
}

}

#endif
