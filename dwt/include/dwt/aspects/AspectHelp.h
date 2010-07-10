/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2010, Jacek Sieka

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
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

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

	typedef std::function<void (unsigned&)> id_function_type;

public:
	unsigned getHelpId() const {
		unsigned ret = ::GetWindowContextHelpId(H());
		if(!ret) {
			WidgetType* parent = dynamic_cast<WidgetType*>(W().getParent());
			if(parent)
				ret = parent->getHelpId();
		}
		if(id_function)
			id_function(ret);
		return ret;
	}

	void setHelpId(unsigned id) {
		::SetWindowContextHelpId(H(), id);
	}

	/**
	* set a callback function that can modify the id returned by getHelpId. note that the
	* dispatcher used by onHelp doesn't call getHelpId, so this won't affect messages dispatched
	* from WM_HELP. in order to modify help ids dispatched via the function defined in onHelp, use
	* helpImpl.
	*/
	void setHelpId(const id_function_type& f) {
		id_function = f;
	}

	void onHelp(const typename HelpDispatcher::F& f) {
		W().addCallback(Message(WM_HELP), HelpDispatcher(f, &W()));
	}

private:
	id_function_type id_function;

	/** implement this in the derived widget in order to change the help id before it is
	* dispatched. if you are not using onHelp to define callbacks but simply calling getHelpId when
	* necessary, then it is the setHelpId overload which takes a function as input that you are
	* looking for.
	*/
	virtual void helpImpl(unsigned& id) {
		// empty on purpose.
	}
};

}

#endif /*ASPECTHELP_H_*/
