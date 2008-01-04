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

// Amount of space between child and border
// TODO make configurable or something
static const int PADDING = 3;

GroupBox::Seed::Seed(const tstring& caption) :
	BaseType::Seed(WC_BUTTON, BS_GROUPBOX | WS_CHILD, 0, caption),
	font(new Font(DefaultGuiFont))
{
}

void GroupBox::create( const GroupBox::Seed & cs ) {
	BaseType::create(cs);
	if(cs.font)
		setFont( cs.font );
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
	BaseType::layout(rect);

	Widget* child = getChild();
	if(child) {
		child->layout(shrink(Rectangle(0, 0, rect.width(), rect.height())));
	}
}

Rectangle GroupBox::shrink(const Rectangle& client) {
	// Taken from http://support.microsoft.com/kb/124315
	UpdateCanvas c(this);

	c.selectFont(getFont());
	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);
	const int h = tmNew.tmHeight;

	return Rectangle(
		client.width() > PADDING ? client.x() + PADDING : client.width(),
		client.height() > PADDING + h ? client.y() + PADDING + h : client.height(),
		client.width() > PADDING * 2 ? client.width() - PADDING * 2 : 0,
		client.height() > PADDING * 2 + h ? client.height() - PADDING * 2 - h : 0
	);
}

Point GroupBox::expand(const Point& child) {
	UpdateCanvas c(this);

	c.selectFont(getFont());
	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);
	const int h = tmNew.tmHeight;
	Point txt = c.getTextExtent(getText());
	return Point(std::max(child.x, txt.x) + PADDING * 2, child.y + PADDING * 2 + h);
}

}
