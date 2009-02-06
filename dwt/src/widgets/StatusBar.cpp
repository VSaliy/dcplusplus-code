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

#include <dwt/widgets/StatusBar.h>

#include <dwt/WidgetCreator.h>
#include <dwt/widgets/ToolTip.h>

namespace dwt {

StatusBar::Seed::Seed(unsigned parts_, unsigned fill_, bool sizeGrip, bool tooltip_) :
BaseType::Seed(STATUSCLASSNAME, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS),
parts(parts_),
fill(fill_),
tooltip(tooltip_)
{
	assert(fill < parts);

	if(sizeGrip) {
		style |= SBARS_SIZEGRIP;
	}
}

StatusBar::StatusBar(Widget* parent) :
BaseType(parent),
tip(0)
{
}

void StatusBar::create(const Seed& cs) {
	parts.resize(cs.parts);
	parts[cs.fill].fill = true;

	BaseType::create(cs);
	if(cs.font)
		setFont(cs.font);

	if(cs.tooltip) {
		tip = WidgetCreator<ToolTip>::create(this, ToolTip::Seed());
		tip->setTool(this, std::tr1::bind(&StatusBar::handleToolTip, this, _1));
	}

	ClickType::onClicked(std::tr1::bind(&StatusBar::handleClicked, this));
	DblClickType::onDblClicked(std::tr1::bind(&StatusBar::handleDblClicked, this));
}

void StatusBar::setSize(unsigned part, unsigned size) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].size = size;
}

void StatusBar::setText(unsigned part, const tstring& text, bool alwaysResize) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	Part& info = parts[part];
	if(!info.fill) {
		unsigned oldW = info.size;
		unsigned newW = getTextSize(text).x + 12;
		if(newW > oldW || (alwaysResize && newW != oldW)) {
			info.size = newW;
			layoutSections(BaseType::getSize());
		}
	} else if(tip) {
		lastLines.push_back(text);
		while(lastLines.size() > MAX_LINES) {
			lastLines.erase(lastLines.begin());
		}
	}
	sendMessage(SB_SETTEXT, static_cast<WPARAM>(part), reinterpret_cast<LPARAM>(text.c_str()));
}

void StatusBar::setHelpId(unsigned part, unsigned id) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].helpId = id;
}

void StatusBar::mapWidget(unsigned part, Widget* widget, const Rectangle& padding) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	POINT p[2];
	sendMessage(SB_GETRECT, part, reinterpret_cast<LPARAM>(p));
	::MapWindowPoints(handle(), getParent()->handle(), (POINT*)p, 2);
	::MoveWindow(widget->handle(),
		p[0].x + padding.left(),
		p[0].y + padding.top(),
		p[1].x - p[0].x - padding.right(),
		p[1].y - p[0].y - padding.bottom(), TRUE);
}

void StatusBar::onClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].clickF = f;
}

void StatusBar::onDblClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].dblClickF = f;
}

void StatusBar::layout(Rectangle& r) {
	setBounds(0, 0, 0, 0);

	Point sz(BaseType::getSize());
	r.size.y -= sz.y;
	layoutSections(sz);
}

bool StatusBar::tryFire(const MSG& msg, LRESULT& retVal) {
	if(tip)
		tip->relayEvent(msg);

	return BaseType::tryFire(msg, retVal);
}

void StatusBar::layoutSections(const Point& sz) {
	std::vector<unsigned> sizes;
	unsigned fillSize = sz.x;
	unsigned fillIndex = 0;
	size_t count = 0;
	for(Parts::const_iterator i = parts.begin(); i != parts.end(); ++i, ++count) {
		if(i->fill)
			fillIndex = count;
		else
			fillSize -= i->size;
		sizes.push_back(i->size);
	}
	parts[fillIndex].size = sizes[fillIndex] = fillSize;

	std::vector< unsigned > newVec( sizes );
	std::vector< unsigned >::const_iterator origIdx = sizes.begin();
	unsigned offset = 0;
	for ( std::vector< unsigned >::iterator idx = newVec.begin(); idx != newVec.end(); ++idx, ++origIdx ) {
		* idx = ( * origIdx ) + offset;
		offset += * origIdx;
	}
	const unsigned * intArr = & newVec[0];
	const size_t size = newVec.size();
	sendMessage(SB_SETPARTS, static_cast< WPARAM >( size ), reinterpret_cast< LPARAM >( intArr ) );
}

StatusBar::Part* StatusBar::getClickedPart() {
	unsigned x = ClientCoordinate(ScreenCoordinate(Point::fromLParam(::GetMessagePos())), this).x();
	unsigned total = 0;
	for(Parts::iterator i = parts.begin(); i != parts.end(); ++i) {
		total += i->size;
		if(total > x)
			return &*i;
	}

	return 0;
}

void StatusBar::handleToolTip(tstring& text) {
	for(Parts::const_iterator i = parts.begin(); i != parts.end(); ++i) {
		if(i->fill) {
			tip->setMaxTipWidth(i->size);
			break;
		}
	}

	text.clear();
	for(size_t i = 0; i < lastLines.size(); ++i) {
		if(i > 0) {
			text += _T("\r\n");
		}
		text += lastLines[i];
	}
}

void StatusBar::handleClicked() {
	Part* part = getClickedPart();
	if(part && part->clickF)
		part->clickF();
}

void StatusBar::handleDblClicked() {
	Part* part = getClickedPart();
	if(part && part->dblClickF)
		part->dblClickF();
}

void StatusBar::helpImpl(unsigned& id) {
	// we have the help id of the whole status bar; convert to the one of the specific part the user just clicked on
	Part* part = getClickedPart();
	if(part && part->helpId)
		id = part->helpId;
}

}
