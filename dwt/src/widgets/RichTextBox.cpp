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

#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <dwt/widgets/RichTextBox.h>

#include <dwt/LibraryLoader.h>

namespace dwt {

RichTextBox::Seed::Seed() :
	BaseType::Seed(RICHEDIT_CLASS, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_MULTILINE | WS_BORDER | ES_WANTRETURN),
	font(new Font(DefaultGuiFont)),
	backgroundColor(RGB( 255, 255, 255 )),
	scrollBarHorizontallyFlag(false),
	scrollBarVerticallyFlag(false)
{
}

void RichTextBox::create( const Seed & cs )
{
	// Need to load up RichEdit library!
	static LibraryLoader richEditLibrary( _T( "riched20.dll" ) );

	dwtassert((cs.style & WS_CHILD) == WS_CHILD, _T("Widget must have WS_CHILD style"));
	PolicyType::create( cs );
	if(cs.font)
		setFont( cs.font );
	setBackgroundColor( cs.backgroundColor );
	setScrollBarHorizontally( cs.scrollBarHorizontallyFlag );
	setScrollBarVertically( cs.scrollBarVerticallyFlag );
}

inline int RichTextBox::charFromPos(const ScreenCoordinate& pt) {
	ClientCoordinate cc(pt, this);
	// Unlike edit control: "The return value specifies the zero-based character index of the character
	// nearest the specified point. The return value indicates the last character in the edit control if the
	// specified point is beyond the last character in the control."
	POINTL lp;
	lp.x = cc.x();
	lp.y = cc.y();
	return ::SendMessage(this->handle(), EM_CHARFROMPOS, 0, (LPARAM)&lp);
}

inline int RichTextBox::lineFromPos(const ScreenCoordinate& pt) {
	ClientCoordinate cc(pt, this);
	return ::SendMessage(this->handle(), EM_EXLINEFROMCHAR, 0, charFromPos(pt));
}

tstring RichTextBox::textUnderCursor(const ScreenCoordinate& p) {
	int i = charFromPos(p), cur = 0;
	tstring tmp = getText();

	// http://rubyforge.org/pipermail/wxruby-users/2006-August/002116.html
	// Otherwise charFromPos will be increasingly off from getText with each new
	// line by one character.
	i = std::find_if(tmp.begin(), tmp.end(),
		(cur += (boost::lambda::_1 != _T('\r')), boost::lambda::var(cur) >= i)) - tmp.begin();

	tstring::size_type start = tmp.find_last_of(_T(" <\t\r\n"), i);
	if(start == tstring::npos)
		start = 0;
	else
		start++;

	tstring::size_type end = tmp.find_first_of(_T(" >\t\r\n"), start + 1);
	if(end == tstring::npos)
		end = tmp.size();

	return tmp.substr(start, end - start);
}

LONG RichTextBox::streamIn(UINT uFormat, EDITSTREAM& es) {
	return static_cast<LONG>(sendMessage(EM_STREAMIN, uFormat, (LPARAM)&es));
}

}
