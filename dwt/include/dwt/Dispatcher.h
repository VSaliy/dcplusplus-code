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

#ifndef DWT_DISPATCHER_H
#define DWT_DISPATCHER_H

#include "WindowsHeaders.h"
#include "tstring.h"
#include <typeinfo>
#include <sstream>
#include <memory>

namespace dwt {

class Dispatcher {
public:
	virtual LRESULT chain(const MSG& msg) = 0;

	template<typename T>
	static tstring className() {
		// Convert to wide
		std::basic_stringstream<TCHAR> stream;
		stream << typeid(T).name();
		return stream.str();
	}

	LPCTSTR getClassName() { return reinterpret_cast<LPCTSTR>(atom); }
protected:
	friend class std::auto_ptr<Dispatcher>;

	Dispatcher(LPCTSTR className, WNDPROC initProc);
	Dispatcher(WNDCLASSEX& cls);

	virtual ~Dispatcher();

	ATOM atom;
};

class NormalDispatcher : public Dispatcher {
public:
	static Dispatcher& getDefault();

	NormalDispatcher(LPCTSTR className_);

	template<typename T>
	static Dispatcher& newClass() {
		static std::auto_ptr<Dispatcher> dispatcher(new NormalDispatcher(className<T>().c_str()));
		return *dispatcher;
	}

	virtual LRESULT chain(const MSG& msg);
};

class ChainingDispatcher : public Dispatcher {
public:
	ChainingDispatcher(LPCTSTR className_, WNDPROC wndProc_);

	template<typename T>
	static Dispatcher& superClass() {
		static std::auto_ptr<Dispatcher> dispatcher = superClass(T::windowClass, className<T>().c_str());
		return *dispatcher;
	}

	static std::auto_ptr<Dispatcher> superClass(LPCTSTR original, LPCTSTR newName);

	virtual LRESULT chain(const MSG& msg);
private:
	ChainingDispatcher(WNDCLASSEX& cls, WNDPROC wndProc_);

	WNDPROC wndProc;
};

}

#endif
