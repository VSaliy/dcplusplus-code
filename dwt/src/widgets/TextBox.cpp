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

#include <dwt/widgets/TextBox.h>

#include <dwt/CanvasClasses.h>
#include <dwt/Texts.h>
#include <dwt/WidgetCreator.h>
#include <dwt/util/check.h>

namespace dwt {

TextBoxBase::TextBoxBase(Widget *parent, Dispatcher& dispatcher) :
BaseType(parent, dispatcher),
lines(1)
{
	// Can't have a text box without a parent...
	dwtassert( parent, _T( "Cant have a TextBox without a parent..." ) );
}

TextBox::TextBox(Widget* parent) :
TextBoxBase(parent, ChainingDispatcher::superClass<TextBox>())
{
}
	
const TCHAR TextBox::windowClass[] = WC_EDIT;

TextBoxBase::Seed::Seed(DWORD style, DWORD exStyle, const tstring& caption) :
BaseType::Seed(style, exStyle, caption),
lines(1)
{
}

void TextBoxBase::create(const Seed& cs) {
	lines = cs.lines;
	menuSeed = cs.menuSeed;
	BaseType::create(cs);
}

TextBox::Seed::Seed(const tstring& caption) :
BaseType::Seed(WS_CHILD | WS_TABSTOP, WS_EX_CLIENTEDGE, caption),
font(new Font(DefaultGuiFont))
{
}

void TextBox::create(const Seed& cs) {
	BaseType::create(cs);
	if(cs.font)
		setFont(cs.font);

	// multiline text-boxes don't handle ctrl + A so we have do it ourselves...
	if((cs.style & ES_MULTILINE) == ES_MULTILINE)
		onKeyDown([this](int c) { return handleKeyDown(c); });
}

void TextBox::setText(const tstring& txt) {
	BaseType::setText(txt);

	// multiline text-boxes don't fire EN_CHANGE / EN_UPDATE when they receive WM_SETTTEXT
	if(hasStyle(ES_MULTILINE)) {
		sendMessage(WM_COMMAND, MAKEWPARAM(0, EN_UPDATE), reinterpret_cast<LPARAM>(handle()));
		sendMessage(WM_COMMAND, MAKEWPARAM(0, EN_CHANGE), reinterpret_cast<LPARAM>(handle()));
	}
}

tstring TextBox::getLine(int line) {
	tstring tmp;
	tmp.resize(std::max(2, lineLength(lineIndex(line))));

	*reinterpret_cast<WORD*>(&tmp[0]) = static_cast<WORD>(tmp.size());
	tmp.resize(sendMessage(EM_GETLINE, static_cast<WPARAM>(line), reinterpret_cast<LPARAM>(&tmp[0])));
	return tmp;
}

tstring TextBox::textUnderCursor(const ScreenCoordinate& p, bool includeSpaces) {
	int i = charFromPos(p);
	int line = lineFromPos(p);
	int c = (i - lineIndex(line)) & 0xFFFF;

	tstring tmp = getLine(line);

	tstring::size_type start = tmp.find_last_of(includeSpaces ? _T("<\t\r\n") : _T(" <\t\r\n"), c);
	if(start == tstring::npos)
		start = 0;
	else
		start++;

	tstring::size_type end = tmp.find_first_of(includeSpaces ? _T(">\t\r\n") : _T(" >\t\r\n"), start + 1);
	if(end == tstring::npos)
		end = tmp.size();

	return tmp.substr(start, end-start);
}

tstring TextBox::getSelection() const
{
	DWORD start, end;
	this->sendMessage( EM_GETSEL, reinterpret_cast< WPARAM >( & start ), reinterpret_cast< LPARAM >( & end ) );
	tstring retVal = this->getText().substr( start, end - start );
	return retVal;
}

ClientCoordinate TextBoxBase::ptFromPos(int pos) {
	LRESULT res = sendMessage(EM_POSFROMCHAR, pos);
	Point pt;
	if(res == -1) {
		Point sz = getClientSize();
		pt.x = sz.x / 2;
		pt.y = sz.y / 2;
	} else {
		pt.x = LOWORD(res);
		pt.y = HIWORD(res);
	}
	return ClientCoordinate(pt, this);
}

void TextBoxBase::scrollToBottom() {
	// this function takes care of various corner cases (not fully scrolled, scrolled too far...)

	const std::pair<int, int> sel = getCaretPosRange();

	setSelection(length());
	showCaret();

	// restore the previous selection
	setSelection(sel.first, sel.second);

	sendMessage(WM_VSCROLL, SB_BOTTOM);
}

ScreenCoordinate TextBoxBase::getContextMenuPos() {
	return ptFromPos(getCaretPos());
}

Point TextBoxBase::getPreferredSize() {
	// Taken from http://support.microsoft.com/kb/124315
	UpdateCanvas c(this);

	c.selectFont(FontPtr(new Font(SystemFont)));
	TEXTMETRIC tmSys = { 0 };
	c.getTextMetrics(tmSys);

	c.selectFont(getFont());
	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);

	Point ret = c.getTextExtent(getText());
	ret.x += GetSystemMetrics(SM_CXEDGE) * 2;
	ret.y = lines * tmNew.tmHeight + std::min(tmNew.tmHeight, tmSys.tmHeight) / 2 + GetSystemMetrics(SM_CYEDGE) * 2;
	return ret;
}

MenuPtr TextBoxBase::getMenu() {
	const bool writable = !hasStyle(ES_READONLY);
	const bool text = !getText().empty();
	const bool selection = !getSelection().empty();

	MenuPtr menu(WidgetCreator<Menu>::create(getParent(), menuSeed));

	if(writable) {
		menu->appendItem(Texts::get(Texts::undo), [this] { this->sendMessage(WM_UNDO); },
			IconPtr(), sendMessage(EM_CANUNDO));
		menu->appendSeparator();
		menu->appendItem(Texts::get(Texts::cut), [this] { this->sendMessage(WM_CUT); },
			IconPtr(), selection);
	}
	menu->appendItem(Texts::get(Texts::copy), [this] { this->sendMessage(WM_COPY); },
		IconPtr(), selection);
	if(writable) {
		menu->appendItem(Texts::get(Texts::paste), [this] { this->sendMessage(WM_PASTE); });
		menu->appendItem(Texts::get(Texts::del), [this] { this->sendMessage(WM_CLEAR); },
			IconPtr(), selection);
	}
	menu->appendSeparator();
	menu->appendItem(Texts::get(Texts::selAll), [this] { this->setSelection(0, -1);},
		IconPtr(), text);

	return menu;
}

bool TextBoxBase::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(msg.message == WM_RBUTTONDOWN && !hasFocus())
		setFocus();

	bool handled = BaseType::handleMessage(msg, retVal);

	// keep the scroll position at the end if it already was at the end
	if((msg.message == WM_SIZE || msg.message == WM_MOVE) && hasStyle(WS_VSCROLL) && scrollIsAtEnd()) {
		retVal = getDispatcher().chain(msg);
		scrollToBottom();
		return true;
	}

	if(!handled && msg.message == WM_CONTEXTMENU) {
		// process this here to give the host a chance to handle it differently.

		// imitate AspectContextMenu
		ScreenCoordinate pt(Point::fromLParam(msg.lParam));
		if(pt.x() == -1 || pt.y() == -1) {
			pt = getContextMenuPos();
		}

		getMenu()->open(pt);
		return true;
	}

	return handled;
}

bool TextBox::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(BaseType::handleMessage(msg, retVal))
		return true;

	if(msg.message == WM_GETDLGCODE) {
		/*
		* multiline edit controls add the DLGC_WANTALLKEYS flag but go haywire when they actually
		* try to handle keys like tab/enter/escape; especially when hosted in a modeless dialog or
		* when one of their parents has the WS_EX_CONTROLPARENT style.
		*/
		retVal = getDispatcher().chain(msg);
		if(retVal & DLGC_WANTALLKEYS) {
			retVal &= ~DLGC_WANTALLKEYS;

			if(msg.wParam == VK_RETURN)
				retVal |= DLGC_WANTMESSAGE;
		}

		return true;
	}

	return false;
}

bool TextBox::handleKeyDown(int c) {
	if(c == 'A' && isControlPressed() && !isAltPressed()) {
		setSelection();
		return true;
	}
	return false;
}

}
