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

#ifndef DWT_CONTROL_H_
#define DWT_CONTROL_H_

#include "../Widget.h"

#include "../aspects/AspectBorder.h"
#include "../aspects/AspectCloseable.h"
#include "../aspects/AspectContextMenu.h"
#include "../aspects/AspectEnabled.h"
#include "../aspects/AspectHelp.h"
#include "../aspects/AspectKeyboard.h"
#include "../aspects/AspectMouse.h"
#include "../aspects/AspectRaw.h"
#include "../aspects/AspectSizable.h"
#include "../aspects/AspectTimer.h"
#include "../aspects/AspectVisible.h"

namespace dwt {

/** Base class for all windows */
class Control:
	public Widget,

	public AspectBorder<Control>,
	public AspectCloseable<Control>,
	public AspectContextMenu<Control>,
	public AspectEnabled<Control>,
	public AspectHelp<Control>,
	public AspectKeyboard<Control>,
	public AspectMouse<Control>,
	public AspectRaw<Control>,
	public AspectSizable<Control>,
	public AspectTimer<Control>,
	public AspectVisible<Control>
{
	struct CreateDispatcher
	{
		typedef std::tr1::function<void (const CREATESTRUCT&)> F;

		CreateDispatcher(const F& f_) : f(f_) { }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			const CREATESTRUCT* cs = reinterpret_cast<const CREATESTRUCT*>( msg.lParam );
			f(*cs);
			return false;
		}

		F f;
	};

	typedef Widget BaseType;
public:

	/// Setting the event handler for the "create" event
	/** The event handler must have the signature "void foo( CREATESTRUCT * )" where
	  * the WidgetType is the type of Widget that realizes the Aspect. <br>
	  * If you supply an event handler for this event your handler will be called
	  * when Widget is initially created. <br>
	  */
	void onCreate(const CreateDispatcher::F& f) {
		addCallback(
			Message( WM_CREATE ), CreateDispatcher(f)
		);
	}

protected:
	struct Seed : public BaseType::Seed {
		Seed(DWORD style, DWORD exStyle = 0, const tstring& caption = tstring());
	};

	typedef Control ControlType;

	Control(Widget* parent, Dispatcher& dispatcher);
};

typedef Control CommonControl;

inline Control::Control(Widget* parent, Dispatcher& dispatcher) : BaseType(parent, dispatcher) {

}

inline Control::Seed::Seed(DWORD style, DWORD exStyle, const tstring& caption) :
	BaseType::Seed(style | WS_VISIBLE, exStyle, caption)
{

}

}

#endif /*CONTROL_H_*/
