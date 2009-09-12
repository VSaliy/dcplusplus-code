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

#include <dwt/Dispatcher.h>
#include <dwt/util/check.h>
#include <dwt/Widget.h>
#include <dwt/DWTException.h>

namespace dwt {

template<typename Policy>
class WindowProc {
public:
	typedef WindowProc<Policy> PolicyType;

	static LRESULT CALLBACK initProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		MSG msg = { hwnd, uMsg, wParam, lParam };

		Widget* w = Policy::getInitWidget(msg);
		if(w) {
			w->setHandle(hwnd);

			// Set the normal dispatcher
			::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndProc);

			return wndProc(hwnd, uMsg, wParam, lParam);
		}

		return Policy::returnUnknown(msg);
	}

	static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		// We dispatch certain messages back to the child widget, so that their
		// potential callbacks will die along with the child when the time comes
		HWND handler = getHandler(hwnd, uMsg, wParam, lParam);

		MSG msg = { hwnd, uMsg, wParam, lParam };

		// Try to get the this pointer
		Widget* w = hwnd_cast<Widget*>(handler);

		TCHAR buf[400];
		::GetClassName(hwnd, buf, 400);

		if(w) {
			if(uMsg == WM_NCDESTROY) {
				w->kill();
				return Policy::returnDestroyed(msg);
			}

			LRESULT res = 0;
			if(w->tryFire(msg, res)) {
				return Policy::returnHandled(res, msg);
			}
		}

		if(handler != hwnd) {
			w = hwnd_cast<Widget*>(hwnd);
		}

		// If this fails there's something wrong
		dwtassert(w, "Expected to get a pointer to a widget - something's wrong");

		if(!w) {
			return Policy::returnUnknown(msg);
		}

	    return Policy::returnUnhandled(w, msg);
	}

private:
	static HWND getHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		HWND handler;
		// Check who should handle the message - parent or child
		switch(uMsg) {
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC: {
			handler = reinterpret_cast<HWND>(lParam);
		} break;
		case WM_NOTIFY : {
			NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
			handler = nmhdr->hwndFrom;
		} break;
		case WM_COMMAND: {
			if(lParam != 0) {
				handler = reinterpret_cast<HWND>(lParam);
			} else {
				handler = hwnd;
			}
		} break;
		default: {
			// By default, widgets handle their own messages
			handler = hwnd;
		}
		}
		return handler;
	}
};

namespace Policies {
/// Policy for modeless dialogs
class Dialog
{
public:
	Dialog() { }

	static LRESULT returnDestroyed(HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar) {
		return FALSE;
	}

	static LRESULT returnHandled(LRESULT, HWND, UINT, WPARAM, LPARAM) {
		// A dialog Widget should return TRUE to the windows internal dialog
		// message handler procedure to tell windows that it have handled the
		// message
		/// @todo We should SetWindowLong with the actual result in some cases, see DialogProc docs
		return TRUE;
	}

	static LRESULT returnUnknown(HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar) {
		return returnUnhandled(hWnd, msg, wPar, lPar);
	}

	static LRESULT returnUnhandled( HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar ) {
		// As opposed to a "normal" Widget the dialog Widget should NOT return
		// ::DefaultMessageProc or something similar since this is done internally
		// INSIDE windows but rather return FALSE to indicate it DID NOT handle the
		// message, exception is WM_INITDIALOG which must return TRUE if unhandled
		// to set focus to default "focusable" control!
		if ( WM_INITDIALOG == msg )
			return TRUE;
		return FALSE;
	}

	static void initPolicy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		if(uMsg == WM_INITDIALOG) {
/*			// extracting the this pointer and stuffing it into the Window with SetProp
			Widget* This = Widget::fromLParam(lParam);
			This->setHandle( hwnd );*/
		}
	}
};

/// Aspect classes for a normal Container Widget
/** Used as the third template argument to WidgetFactory if you're creating a normal
  * Container Widget Note that this one is default so if you don't supply  policy
  * SmartWin will assume this is the one you're after!
  */
struct Normal
{
	static LRESULT returnDestroyed(const MSG& msg) {
		return ::DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}

	static LRESULT returnHandled(LRESULT hres, const MSG& msg) {
		return hres;
	}

	static LRESULT returnUnknown(MSG& msg) {
		return ::DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}

	static LRESULT returnUnhandled(Widget* w, const MSG& msg) {
		return ::DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}

	static Widget* getInitWidget(MSG& msg) {
		if (msg.message == WM_NCCREATE) {
			// extracting the this pointer and stuffing it into the Window with SetProp
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>( msg.lParam );
			return reinterpret_cast<Widget*>( cs->lpCreateParams );
		}

		return 0;
	}
};

struct Chain : public Normal {
	static LRESULT returnUnhandled(Widget* w, const MSG& msg) {
		return w->getDispatcher().chain(msg);
	}
};

/// Aspect classes for a MDI Child Container Widget
struct MDIChild
{
	MDIChild(Widget* parent) { }

	static LRESULT returnDestroyed(HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar) {
		return ::DefMDIChildProc( hWnd, msg, wPar, lPar );
	}

	static LRESULT returnHandled(HRESULT hres, HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar )
	{
		switch ( msg )
		{
			case WM_SIZE :
			{
				return ::DefMDIChildProc( hWnd, msg, wPar, lPar );
			}
			default:
				return hres;
		}
	}

	static LRESULT returnUnknown( HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar ) {
		return returnUnhandled(hWnd, msg, wPar, lPar);
	}

	static LRESULT returnUnhandled( HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar )
	{
		return ::DefMDIChildProc( hWnd, msg, wPar, lPar );
	}

	// STATIC EXTRACTER/Windows Message Procedure, extracts the this pointer and
	// dispatches the message to the this Widget
	static void initPolicy( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		if ( uMsg == WM_NCCREATE )
		{
			/*
			// extracting the this pointer and stuffing it into the Window with SetProp
			CREATESTRUCT * cs = reinterpret_cast< CREATESTRUCT * >( lParam );
			MDICREATESTRUCT * mcs = reinterpret_cast< MDICREATESTRUCT*>(cs->lpCreateParams);

			MDIChild* This = reinterpret_cast<MDIChild*>(mcs->lParam);
			This->setHandle(hWnd);*/
		}
	}
};

template<typename WidgetType>
struct MDIFrame : public Normal {
	MDIFrame() { }

	LRESULT returnUnhandled( HWND hWnd, UINT msg, WPARAM wPar, LPARAM lPar ) {
		WidgetType* This = static_cast<WidgetType*>(this);
		if(This->getMDIParent()) {
			return ::DefFrameProc( hWnd, This->getMDIParent()->handle(), msg, wPar, lPar );
		}
		return Normal::returnUnhandled(hWnd, msg, wPar, lPar);
	}

};

}

Dispatcher::Dispatcher(LPCTSTR className, WNDPROC initProc) : atom(0) {
	WNDCLASSEX cls = { sizeof(WNDCLASSEX) };

	cls.lpfnWndProc = initProc;
	cls.hInstance = ::GetModuleHandle(NULL);
	cls.lpszClassName = className;

	atom = ::RegisterClassEx(&cls);
	if(!atom) {
		throw Win32Exception("Unable to register class");
	}
}

Dispatcher::Dispatcher(WNDCLASSEX& cls) : atom(0) {
	atom = ::RegisterClassEx(&cls);
	if(!atom) {
		throw Win32Exception("Unable to register class");
	}
}

Dispatcher::~Dispatcher() {
	if(getClassName()) {
		::UnregisterClass(getClassName(), ::GetModuleHandle(NULL));
	}
}

NormalDispatcher::NormalDispatcher(LPCTSTR className_) :
	Dispatcher(className_, WindowProc<Policies::Normal>::initProc)
{ }

Dispatcher& NormalDispatcher::getDefault() {
	static NormalDispatcher dispatcher(_T("dwt::NormalDispatcher"));
	return dispatcher;
}

LRESULT NormalDispatcher::chain(const MSG& msg) {
	return ::DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
}

ChainingDispatcher::ChainingDispatcher(LPCTSTR className_, WNDPROC wndProc_) :
	Dispatcher(className_, WindowProc<Policies::Chain>::initProc),
	wndProc(wndProc_)
{ }

ChainingDispatcher::ChainingDispatcher(WNDCLASSEX& cls, WNDPROC wndProc_) :
	Dispatcher(cls),
	wndProc(wndProc_)
{ }

LRESULT ChainingDispatcher::chain(const MSG& msg) {
	return ::CallWindowProc(wndProc, msg.hwnd, msg.message, msg.wParam, msg.lParam);
}

std::auto_ptr<Dispatcher> ChainingDispatcher::superClass(LPCTSTR original, LPCTSTR newName) {
	WNDCLASSEX orgClass = { sizeof(WNDCLASSEX) };

	if(!::GetClassInfoEx(GetModuleHandle(NULL), original, &orgClass)) {
		throw Win32Exception("Unable to find information for class");
	}

	WNDPROC proc = orgClass.lpfnWndProc;

	orgClass.lpfnWndProc = WindowProc<Policies::Chain>::initProc;
	orgClass.lpszClassName = newName;
	orgClass.hInstance = ::GetModuleHandle(NULL);
	orgClass.lpszMenuName = 0;
	orgClass.style = CS_DBLCLKS;

	return std::auto_ptr<Dispatcher>(new ChainingDispatcher(orgClass, proc));
}

}
