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

#include <dwt/widgets/Spinner.h>

namespace dwt {

const TCHAR Spinner::windowClass[] = UPDOWN_CLASS;

Spinner::Seed::Seed(int minValue_, int maxValue_, Widget* buddy_) :
BaseType::Seed(WS_CHILD | UDS_ARROWKEYS | UDS_NOTHOUSANDS),
minValue(minValue_),
maxValue(maxValue_),
buddy(buddy_)
{
	if(buddy || (style & UDS_AUTOBUDDY))
		style |= UDS_ALIGNRIGHT | UDS_SETBUDDYINT;
}

void Spinner::create(const Seed& cs) {
	BaseType::create(cs);
	setRange(cs.minValue, cs.maxValue);
	if(cs.buddy)
		assignBuddy(cs.buddy);
}

void Spinner::layout(const dwt::Rectangle&) {
	Widget* w = getBuddy();
	if(w)
		assignBuddy(w);
}

}
