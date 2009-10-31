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

#include <dwt/widgets/Label.h>

#include <boost/lexical_cast.hpp>

namespace dwt {

const TCHAR Label::windowClass[] = WC_STATIC;

Label::Seed::Seed(const tstring& caption) :
BaseType::Seed(WS_CHILD | SS_NOTIFY | SS_NOPREFIX, 0, caption),
font(new Font(DefaultGuiFont))
{
}

Label::Seed::Seed(unsigned iconId) :
BaseType::Seed(WS_CHILD | SS_NOTIFY | SS_ICON, 0, _T('#') + boost::lexical_cast<tstring>(iconId))
{
}

void Label::create( const Seed & cs ) {
	BaseType::create(cs);
	if(cs.font)
		setFont( cs.font );
}

void Label::layout(const Rectangle& r_) {
	Rectangle r = r_;

	if(hasStyle(SS_ICON)) {
		// icon control; we don't want the size to change.
		r.size = getWindowSize();
	}

	BaseType::layout(r);
}

Point Label::getPreferedSize() {
	if(hasStyle(SS_ICON)) {
		// icon control; the control should have already resized itself to its preferred size.
		return getWindowSize();
	}

	return getTextSize(getText());
}

}
