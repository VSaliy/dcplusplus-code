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
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

#include <dwt/widgets/RichTextBox.h>
#include <dwt/Point.h>
#include <dwt/util/HoldRedraw.h>

#include <dwt/LibraryLoader.h>

namespace dwt {

RichTextBox::Seed::Seed() :
	BaseType::Seed(RICHEDIT_CLASS, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | WS_BORDER | ES_NOHIDESEL),
	font(new Font(DefaultGuiFont)),
	foregroundColor(RGB( 0, 0, 0 )),
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

	CHARFORMAT textFormat;
	textFormat.cbSize = sizeof(textFormat);
	textFormat.dwMask = CFM_COLOR;
	textFormat.dwEffects = 0;
	textFormat.crTextColor = cs.foregroundColor;
	setDefaultCharFormat(textFormat);

	setScrollBarHorizontally( cs.scrollBarHorizontallyFlag );
	setScrollBarVertically( cs.scrollBarVerticallyFlag );

	sendMessage( EM_AUTOURLDETECT, FALSE, 0 );
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

inline Point RichTextBox::posFromChar(int charOffset)
{
	POINTL pt;
	sendMessage(EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)charOffset);
	return Point(pt.x, pt.y);
}

inline int RichTextBox::lineFromPos(const ScreenCoordinate& pt) {
	ClientCoordinate cc(pt, this);
	return ::SendMessage(this->handle(), EM_EXLINEFROMCHAR, 0, charFromPos(pt));
}

tstring RichTextBox::getSelection() const { 
	std::pair<int, int> range = getCaretPosRange();
	tstring tmp = getText();

	// This is uglier than it has to be because of the
	// \r\n vs \r handling - note that WINE, for example,
	// uses consistent line endings between the internal
	// and external buffer representations, but Windows does	
	// not - so it cannot even assume getText() consistently
	// is broken.
	int realS = fixupLineEndings(tmp.begin(), tmp.end(), range.first),
	    realE = fixupLineEndings(tmp.begin() + realS, tmp.end(), range.second - range.first);
	return tmp.substr(realS, realE);
}

Point RichTextBox::getScrollPos() const {
	POINT scrollPos;
	sendMessage(EM_GETSCROLLPOS, 0, reinterpret_cast< LPARAM >(&scrollPos));
	return scrollPos;
}

void RichTextBox::setScrollPos(Point& scrollPos) {
	sendMessage(EM_SETSCROLLPOS, 0, reinterpret_cast< LPARAM >(&scrollPos));
}

tstring RichTextBox::textUnderCursor(const ScreenCoordinate& p) {
	tstring tmp = getText();

	tstring::size_type start = tmp.find_last_of(_T(" <\t\r\n"), fixupLineEndings(tmp.begin(), tmp.end(), charFromPos(p)));
	if(start == tstring::npos)
		start = 0;
	else
		start++;

	tstring::size_type end = tmp.find_first_of(_T(" >\t\r\n"), start + 1);
	if(end == tstring::npos)
		end = tmp.size();

	return tmp.substr(start, end - start);
}

int RichTextBox::fixupLineEndings(tstring::const_iterator begin, tstring::const_iterator end, tstring::difference_type ibo) const {
	// http://rubyforge.org/pipermail/wxruby-users/2006-August/002116.html
	// ("TE_RICH2 RichEdit control"). Otherwise charFromPos will be increasingly
	// off from getText with each new line by one character. 
	int cur = 0;
	return std::find_if(begin, end, (cur += (boost::lambda::_1 != static_cast< TCHAR >('\r')),
		boost::lambda::var(cur) > ibo)) - begin;
}

void RichTextBox::addText(const std::string & txt) {
	SETTEXTEX config = {ST_SELECTION, CP_ACP};
	setSelection(-1, -1);
	sendMessage(EM_SETTEXTEX, reinterpret_cast< WPARAM >(&config), reinterpret_cast< LPARAM >(txt.c_str()));
}

void RichTextBox::addTextSteady( const tstring & txtRaw, std::size_t len ) {
	Point scrollPos = getScrollPos();
	bool scroll = scrollIsAtEnd();

	{
		util::HoldRedraw hold(this, !scroll);
		std::pair<int, int> cr = getCaretPosRange();
		std::string txt = escapeUnicode(txtRaw);
	
		unsigned charsRemoved = 0;
		int multipler = 1;
	
		size_t limit = getTextLimit();
		if(length() + len > limit) {
			util::HoldRedraw hold2(this, scroll);
			if(len >= limit) {
				charsRemoved = length();
			} else {
				while (charsRemoved < len)
					charsRemoved = lineIndex(lineFromChar(multipler++ * limit / 10));
			}
	
			scrollPos.y -= posFromChar(charsRemoved).y;
			setSelection(0, charsRemoved);
			replaceSelection(_T(""));
		}
	
		addText(txt);
		setSelection(cr.first-charsRemoved, cr.second-charsRemoved);
	
		if(scroll)
			sendMessage(WM_VSCROLL, SB_BOTTOM);
		else
			setScrollPos(scrollPos);
	}
	redraw();
}

void RichTextBox::findText(tstring const& needle) throw() {
	// The code here is slightly longer than a pure getText/STL approach
	// might allow, but it also ducks entirely the line-endings debacle.
	int max = length();

	// a new search? reset cursor to bottom
	if(needle != currentNeedle || currentNeedlePos == -1) {
		currentNeedle = needle;
		currentNeedlePos = max;
	}

	// set current selection
	FINDTEXT ft = { {currentNeedlePos, 0}, NULL };	// reversed
	tstring::size_type len = needle.size();
	boost::scoped_array< TCHAR > needleCTstr( new TCHAR[len+1] );
	needleCTstr[len] = 0;
	copy(needle.begin(), needle.begin()+len, needleCTstr.get());
	ft.lpstrText = needleCTstr.get();

	// empty search? stop
	if(needle.empty())
		return;

	// find upwards
	currentNeedlePos = sendMessage(EM_FINDTEXTW, 0, reinterpret_cast< LPARAM >(&ft));

	// not found? try again on full range
	if(currentNeedlePos == -1 && ft.chrg.cpMin != max) { // no need to search full range twice
		currentNeedlePos = max;
		ft.chrg.cpMin = currentNeedlePos;
		currentNeedlePos = sendMessage(EM_FINDTEXTW, 0, reinterpret_cast< LPARAM >(&ft));
	}

	// found? set selection
	if(currentNeedlePos != -1) {
		ft.chrg.cpMin = currentNeedlePos;
		ft.chrg.cpMax = currentNeedlePos + needle.length();
		setFocus();
		sendMessage(EM_EXSETSEL, 0, reinterpret_cast< LPARAM >(&ft.chrg));
	} else {
#ifdef PORT_ME
		addStatus(T_("String not found: ") + needle);
#endif
		clearCurrentNeedle();
	}
}

void RichTextBox::clearCurrentNeedle()
{
	currentNeedle.clear();
}

LRESULT RichTextBox::onUnsuppRtf(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
#ifdef PORT_ME
	ENLOWFIRTF *pLow = (ENLOWFIRTF *)pnmh;
	LogManager::getInstance()->message("Unsupported RTF code: " + string(pLow->szControl));
	bHandled = FALSE;
#endif
	return 0;
}

std::string RichTextBox::unicodeEscapeFormatter(const tstring_range& match) {
	return (boost::format("\\uc1\\u%dh ")%(int(*match.begin()))).str();
}

std::string RichTextBox::escapeUnicode(const tstring& str) {
	std::string ret;
	boost::find_format_all_copy(std::back_inserter(ret), str,
		boost::first_finder(L"\x7f", std::greater<TCHAR>()), unicodeEscapeFormatter);
	return ret;
}

}
