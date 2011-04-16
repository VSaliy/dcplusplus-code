/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2011, Jacek Sieka

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

#include <numeric>

namespace dwt {

const TCHAR StatusBar::windowClass[] = STATUSCLASSNAME;

StatusBar::Seed::Seed(unsigned parts_, unsigned fill_, bool sizeGrip) :
BaseType::Seed(WS_CHILD),
parts(parts_),
fill(fill_)
{
	assert(fill < parts);

	if(sizeGrip) {
		style |= SBARS_SIZEGRIP;
	}
}

StatusBar::StatusBar(Widget* parent) :
BaseType(parent, ChainingDispatcher::superClass<StatusBar>()),
fill(0),
tip(0),
tipPart(0)
{
}

void StatusBar::create(const Seed& cs) {
	parts.resize(cs.parts);
	fill = cs.fill;

	BaseType::create(cs);
	if(cs.font)
		setFont(cs.font);

	tip = WidgetCreator<ToolTip>::create(this, ToolTip::Seed());
	tip->setTool(this, [this](tstring& text) { handleToolTip(text); });
}

void StatusBar::setSize(unsigned part, unsigned size) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].size = size;
}

void StatusBar::setText(unsigned part, const tstring& text, bool alwaysResize) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	Part& info = parts[part];
	info.text = text;
	if(part != fill) {
		info.updateSize(this, alwaysResize);
	} else if(!text.empty()) {
		lastLines.push_back(text);
		while(lastLines.size() > MAX_LINES) {
			lastLines.erase(lastLines.begin());
		}
		tstring& tip = info.tip;
		tip.clear();
		for(size_t i = 0; i < lastLines.size(); ++i) {
			if(i > 0) {
				tip += _T("\r\n");
			}
			tip += lastLines[i];
		}
	}
	sendMessage(SB_SETTEXT, static_cast<WPARAM>(part), reinterpret_cast<LPARAM>(text.c_str()));
}

void StatusBar::setIcon(unsigned part, const IconPtr& icon, bool alwaysResize) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	Part& info = parts[part];
	info.icon = icon;
	if(part != fill)
		info.updateSize(this, alwaysResize);
	sendMessage(SB_SETICON, part, icon ? reinterpret_cast<LPARAM>(icon->handle()) : 0);
}

void StatusBar::setToolTip(unsigned part, const tstring& text) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].tip = text;
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
	::SetWindowPos(widget->handle(), handle(),
		p[0].x + padding.left(),
		p[0].y + padding.top(),
		p[1].x - p[0].x - padding.right(),
		p[1].y - p[0].y - padding.bottom(), SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void StatusBar::onClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].clickF = f;

	// imitate the default onClicked but with a setCallback.
	setCallback(Message(WM_NOTIFY, NM_CLICK), Dispatchers::VoidVoid<>([this] { handleClicked(); }));
}

void StatusBar::onRightClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].rightClickF = f;

	// imitate the default onRightClicked but with a setCallback.
	setCallback(Message(WM_NOTIFY, NM_RCLICK), Dispatchers::VoidVoid<>([this] { handleRightClicked(); }));
}

void StatusBar::onDblClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), _T("Invalid part number."));
	parts[part].dblClickF = f;

	// imitate the default onDblClicked but with a setCallback.
	setCallback(Message(WM_NOTIFY, NM_DBLCLK), Dispatchers::VoidVoid<>([this] { handleDblClicked(); }));
}

int StatusBar::refresh() {
	// The status bar will auto-resize itself - all we need to do is to layout the sections
	sendMessage(WM_SIZE);

	auto sz = BaseType::getWindowSize();
	layoutSections(sz);
	return sz.y;
}

bool StatusBar::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(tip) {
		if(msg.message == WM_MOUSEMOVE) {
			Part* part = getClickedPart();
			if(part && part != tipPart) {
				tip->refresh();
				tipPart = part;
			}
		}
		tip->relayEvent(msg);
	}

	return BaseType::handleMessage(msg, retVal);
}

void StatusBar::Part::updateSize(StatusBar* bar, bool alwaysResize) {
	unsigned newSize = 0;
	if(icon)
		newSize += icon->getSize().x;
	if(!text.empty()) {
		if(icon)
			newSize += 4; // spacing between icon & text
		newSize += bar->getTextSize(text).x;
	}
	if(newSize > 0)
		newSize += 10; // add margins
	if(newSize > size || (alwaysResize && newSize != size)) {
		size = newSize;
		bar->layoutSections();
	}
}

void StatusBar::layoutSections() {
	layoutSections(getWindowSize());
}

void StatusBar::layoutSections(const Point& sz) {
	std::vector<unsigned> sizes;
	for(Parts::const_iterator i = parts.begin(); i != parts.end(); ++i)
		sizes.push_back(i->size);

	sizes[fill] = 0;
	parts[fill].size = sizes[fill] = sz.x - std::accumulate(sizes.begin(), sizes.end(), 0);

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
	Part* part = getClickedPart();
	if(part) {
		text = part->tip;
		tip->setMaxTipWidth((text.find('\n') == tstring::npos) ? -1 : part->size);
	} else {
		text.clear();
	}
}

void StatusBar::handleClicked() {
	Part* part = getClickedPart();
	if(part && part->clickF)
		part->clickF();
}

void StatusBar::handleRightClicked() {
	Part* part = getClickedPart();
	if(part && part->rightClickF)
		part->rightClickF();
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
