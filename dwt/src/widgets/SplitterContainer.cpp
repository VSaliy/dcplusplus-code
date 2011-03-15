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

#include <dwt/widgets/SplitterContainer.h>
#include <dwt/widgets/Splitter.h>

#include <boost/next_prior.hpp>
#include <boost/range/algorithm/for_each.hpp>

namespace dwt {

namespace {
	bool isSplitter(Widget *w) { return dynamic_cast<Splitter*>(w); }
	bool isNotSplitter(Widget *w) { return !isSplitter(w); }
}

using boost::for_each;
using boost::next;

SplitterContainer::Seed::Seed(double startPos, bool horizontal) :
BaseType::Seed(0, WS_EX_CONTROLPARENT),
startPos(startPos),
horizontal(horizontal)
{
}

void SplitterContainer::create(const Seed& cs) {
	horizontal = cs.horizontal;
	startPos = cs.startPos;
	BaseType::create(cs);
}

void SplitterContainer::setSplitter(size_t n, double pos) {
	auto ns = ensureSplitters();
	if(n >= ns) {
		return;
	}

	auto splitter = *next(getChildren<Splitter>().first, n);
	splitter->setRelativePos(pos);
}

double SplitterContainer::getSplitterPos(size_t n) {
	auto ns = ensureSplitters();
	if(n >= ns) {
		return startPos;
	}

	auto splitter = *next(getChildren<Splitter>().first, n);
	return splitter->getRelativePos();
}

void SplitterContainer::layout() {
	BaseType::layout();
	ensureSplitters();

	auto children = getChildren<Widget>();
	auto splitters = getChildren<Splitter>();

	auto size = getClientSize();

	auto avail = horizontal ? size.y : size.x;

	for_each(splitters, [&](Splitter *w) {
		avail -= horizontal ? w->getPreferredSize().y : w->getPreferredSize().x;
	});

	auto rc = Rectangle(getClientSize());
	auto &sz = horizontal ? rc.size.y : rc.size.x;
	auto &p = horizontal ? rc.pos.y : rc.pos.x;

	sz = 0;

	auto splitter_iter = splitters.first;

	for_each(children, [&](Widget *w) {
		if(isSplitter(w)) {
			return;
		}

		if(splitter_iter == splitters.second) {
			// Last splitter, the next control gets any remaining space
			sz = std::max(avail - p, 0l);
			::MoveWindow(w->handle(), rc.left(), rc.top(), rc.width(), rc.height(), TRUE);
		} else {
			auto splitter = *splitter_iter;
			auto ss = horizontal ? splitter->getPreferredSize().y : splitter->getPreferredSize().x;
			sz = std::max(static_cast<int>(avail * splitter->getRelativePos() - ss / 2. - p), 0);
			::MoveWindow(w->handle(), rc.left(), rc.top(), rc.width(), rc.height(), TRUE);

			p += sz;

			sz = ss;
			::MoveWindow(splitter->handle(), rc.left(), rc.top(), rc.width(), rc.height(), TRUE);
			p += sz;
			splitter_iter++;
		}
	});
}

size_t SplitterContainer::ensureSplitters() {
	auto children = getChildren<Widget>();
	auto nc = 0, ns = 0;
	for_each(children, [&](Widget *w) { isSplitter(w) ? ns++ : nc++; });

	while(ns < nc - 1) {
		auto splitter = addChild(Splitter::Seed(startPos, horizontal));
		splitter->onMove([=] (double pos) { onMove(splitter, pos); });
		ns++;
	}

	return ns;
}

void SplitterContainer::onMove(SplitterPtr splitter, double pos)
{
	// TODO Check that one splitter does not move past another
	splitter->setRelativePos(pos);
	RECT rc;
	::GetWindowRect(handle(), &rc);
	::MapWindowPoints(NULL, getParent()->handle(), (LPPOINT) &rc, 2);
	layout();
	redraw(true);
}

}
