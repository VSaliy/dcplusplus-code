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

#include <dwt/Application.h>

#include <dwt/tstring.h>
#include <dwt/DWTException.h>
#include <dwt/util/check.h>
#include <dwt/widgets/Control.h>
#include <assert.h>

extern int SmartWinMain(dwt::Application & app);

namespace dwt {

// link to Common Controls to relieve user of explicitly doing so
#ifdef _MSC_VER
#ifdef WINCE
#pragma comment( lib, "commctrl.lib" )
#else
#pragma comment( lib, "comctl32.lib" )
#endif //! WINCE
#endif //! _MSC_VER
// Friend functions to Application

Application* Application::itsInstance = 0;
HANDLE Application::itsMutex = 0;

// Application implementation

/** Initializes the runtime for SmartWin++
 Typically only called by WinMain or DllMain.
 */
void Application::init(int nCmdShow) {
	itsInstance = new Application(nCmdShow);

#ifndef WINCE
	BOOL enable;
	::SystemParametersInfo(SPI_GETUIEFFECTS, 0, &enable, 0);
	if (!enable) {
		enable = TRUE;
		::SystemParametersInfo(SPI_SETUIEFFECTS, 0, &enable, 0);
	}
#endif

	// Initializing Common Controls...
	INITCOMMONCONTROLSEX init = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_ANIMATE_CLASS | ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES | ICC_HOTKEY_CLASS |
		ICC_INTERNET_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES | ICC_NATIVEFNTCTL_CLASS | ICC_PAGESCROLLER_CLASS |
		ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES
	};

	::InitCommonControlsEx(&init);
}

Application::Application(int nCmdShow) :
	itsCmdShow(nCmdShow), taskMutex(::CreateMutex(NULL, FALSE, NULL)), quit(false),
	threadId(::GetCurrentThreadId())
{
}

Application::~Application() {
	::CloseHandle(taskMutex);
}

const CommandLine& Application::getCommandLine() const {
	return itsCmdLine;
}

bool Application::addWaitEvent(HANDLE hWaitEvent, const Application::Callback& pSignal) {
	// in case the maximum number of objects is already achieved return false
	if (itsVHEvents.size() >= MAXIMUM_WAIT_OBJECTS - 1)
		return false;

	if (hWaitEvent != INVALID_HANDLE_VALUE) {
		itsVSignals.push_back(pSignal);
		itsVHEvents.push_back(hWaitEvent);
	}
	return true;
}

void Application::removeWaitEvent(HANDLE hWaitEvent) {
	if (hWaitEvent != INVALID_HANDLE_VALUE) {
		std::vector<Callback>::iterator pSig;
		std::vector<HANDLE>::iterator pH;
		for (pSig = itsVSignals.begin(), pH = itsVHEvents.begin(); pSig != itsVSignals.end(); pSig++, pH++) {
			if (*pH == hWaitEvent) {
				itsVSignals.erase(pSig);
				itsVHEvents.erase(pH);
				break;
			}
		}
	}
}

void Application::uninit() {
	delete itsInstance;
	itsInstance = 0;
	if (itsMutex) {
		::CloseHandle(itsMutex);
		itsMutex = 0;
	}
}

Application& Application::instance() {
	assert(itsInstance);
	return *itsInstance;
}

tstring Application::getModulePath() const {
	TCHAR retVal[2049];
	GetModuleFileName(0, retVal, 2048);
	tstring retStr = retVal;
	retStr = retStr.substr(0, retStr.find_last_of('\\') + 1);
	return retStr;
}

tstring Application::getModuleFileName() const {
	TCHAR retVal[2049];
	return tstring(retVal, GetModuleFileName(0, retVal, 2048));
}

void Application::run() {
	while(!quit) {
		if(!dispatch()) {
			sleep();
		}
	}
}

bool Application::sleep() {
	if (quit) {
		return false;
	}

	while(true) {
		size_t n = itsVHEvents.size();

		DWORD ret = ::MsgWaitForMultipleObjectsEx(static_cast<DWORD> (n), itsVHEvents.empty() ? 0 : &itsVHEvents[0],
		    INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

		if (ret == WAIT_OBJECT_0 + n) {
			return true;
		} else if (ret < WAIT_OBJECT_0 + itsVHEvents.size()) {
			// the wait event was signaled by Windows
			// signal its handlers
			itsVSignals[ret - WAIT_OBJECT_0]();
		} else {
			throw Win32Exception("Unexpected return value from MsgWaitForMultipleObjectsEx");
		}
	}
}

#include <stdio.h>
bool Application::dispatch() {
	MSG msg = { 0 };
	if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == 0) {
		return dispatchAsync();
	}

	if (msg.message == WM_QUIT) {
		// Make sure outer message loops see it...
		::PostQuitMessage(msg.wParam);
		quit = true;
		return false;
	}

	// heavily inspired by SWT (Display.java - filterMessage & findControl)
	if(msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) {
		HWND hwnd = msg.hwnd;
		HWND owner = 0;
		while(hwnd && hwnd != owner) {
			{
				// make sure the window is ours or hwnd_cast might crash.
				TCHAR className[128];
				if(!::GetClassName(hwnd, className, 128) || !Dispatcher::isRegistered(className))
					break;
			}
			Control* control = hwnd_cast<Control*>(hwnd);
			if(control && control->filter(msg)) {
				return true;
			}
			owner = ::GetWindow(hwnd, GW_OWNER);
			hwnd = ::GetParent(hwnd);
		}
	}

	for(FilterIter i = filters.begin(); i != filters.end(); ++i) {
		if ((*i)(msg)) {
			return true;
		}
	}

	::TranslateMessage(&msg);
	::DispatchMessage(&msg);

	return true;
}

void Application::wake() {
	::PostThreadMessage(threadId, WM_NULL, 0, 0);
}

int Application::getCmdShow() const {
	return itsCmdShow;
}

struct MutexLock {
	MutexLock(HANDLE h_) : h(h_) {
		::WaitForSingleObject(h, INFINITE);
	}
	~MutexLock() {
		::ReleaseMutex(h);
	}
	HANDLE h;
};

bool Application::dispatchAsync() {
	Callback callback;
	{
		MutexLock m(taskMutex);
		if(tasks.empty()) {
			return false;
		}
		callback = tasks.front();
		tasks.pop_front();
	}

	callback();
	return true;
}

void Application::callAsync(const Callback& callback) {
	MutexLock m(taskMutex);
	tasks.push_back(callback);
	wake();
}

Application::FilterIter Application::addFilter(const FilterFunction& f) {
	return filters.insert(filters.end(), f);
}

void Application::removeFilter(const FilterIter& i) {
	filters.erase(i);
}

} // namespace dwt

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	unsigned int retVal = 0;

	dwt::Application::init(nCmdShow);

	retVal = SmartWinMain(dwt::Application::instance()); // Call library user's startup function.

	dwt::Application::uninit();

	return retVal;
}
