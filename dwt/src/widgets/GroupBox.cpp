/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2008, Jacek Sieka

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

#include <dwt/widgets/GroupBox.h>
#include <dwt/Point.h>

namespace dwt {

GroupBox::Seed::Seed(const tstring& caption) :
BaseType::Seed(WC_BUTTON, BS_GROUPBOX | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT, caption),
font(new Font(DefaultGuiFont)),
padding(6, 6)
{
}

void GroupBox::create( const GroupBox::Seed & cs ) {
	BaseType::create(cs);
	if(cs.font)
		setFont( cs.font );

	padding.x = ::GetSystemMetrics(SM_CXEDGE) * 2 + cs.padding.x * 2;
	padding.y = ::GetSystemMetrics(SM_CYEDGE) + cs.padding.y * 2; // ignore the top border
}

Point GroupBox::getPreferedSize() {
	Point ret(0, 0);
	Widget* w = getChild();

	if(w) {
		ret = w->getPreferedSize();
	}

	return expand(ret);
}

void GroupBox::layout(const Rectangle& rect) {
	Widget* child = getChild();
	if(child) {
		child->layout(shrink(Rectangle(0, 0, rect.width(), rect.height())));
	}

	BaseType::layout(rect);
}

Rectangle GroupBox::shrink(const Rectangle& client) {
	// Taken from http://support.microsoft.com/kb/124315
	UpdateCanvas c(this);

	c.selectFont(getFont());
	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);
	const int h = tmNew.tmHeight;

	return Rectangle(
		client.width() > padding.x / 2 ? client.x() + padding.x / 2 : client.width(),
		client.height() > padding.y / 2 + h ? client.y() + padding.y / 2 + h : client.height(),
		client.width() > padding.x ? client.width() - padding.x : 0,
		client.height() > padding.y + h ? client.height() - padding.y - h : 0
	);
}

Point GroupBox::expand(const Point& child) {
	UpdateCanvas c(this);
	c.selectFont(getFont());

	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);
	const int h = tmNew.tmHeight;

	Point txt = c.getTextExtent(getText());

	return Point(std::max(child.x, txt.x) + padding.x, child.y + padding.y + h);
}

}
