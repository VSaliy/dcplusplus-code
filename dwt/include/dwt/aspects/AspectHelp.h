/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2009, Jacek Sieka

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

#ifndef DWT_ASPECTHELP_H_
#define DWT_ASPECTHELP_H_

#include "../Message.h"
#include <functional>

namespace dwt {

template<typename WidgetType>
class AspectHelp {
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	struct HelpDispatcher : Dispatchers::Base<void (WidgetType*, unsigned)> {
		typedef Dispatchers::Base<void (WidgetType*, unsigned)> BaseType;
		HelpDispatcher(const typename BaseType::F& f, const WidgetType* const widget_) :
		BaseType(f),
		widget(widget_)
		{ }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(msg.lParam);
			if(lphi->iContextType != HELPINFO_WINDOW)
				return false;

			HWND hWnd = reinterpret_cast<HWND>(lphi->hItemHandle);
			// make sure this handle is ours
			if(hWnd != widget->handle() && !::IsChild(widget->handle(), hWnd))
				return false;

			WidgetType* w = hwnd_cast<WidgetType*>(hWnd);
			if(!w)
				return false;

			unsigned id = lphi->dwContextId;
			w->helpImpl(id);

			f(w, id);

			ret = TRUE;
			return true;
		}

	private:
		/// the widget that catches WM_HELP messages (not necessarily the one that sent them)
		const WidgetType* const widget;
	};

public:
	unsigned getHelpId() {
		return ::GetWindowContextHelpId(H());
	}

	void setHelpId(unsigned id) {
		::SetWindowContextHelpId(H(), id);
	}

	void onHelp(const typename HelpDispatcher::F& f) {
		W().addCallback(Message(WM_HELP), HelpDispatcher(f, &W()));
	}

private:
	virtual void helpImpl(unsigned& id) {
		// empty on purpose.
		// implement this in your widget if you wish to change the help id before it is dispatched
	}
};

}

#endif /*ASPECTHELP_H_*/
