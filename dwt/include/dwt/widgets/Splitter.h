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

#ifndef DWT_SPLITTER_H
#define DWT_SPLITTER_H

#include "Container.h"
#include "ToolTip.h"
#include <dwt/Texts.h>
#include <dwt/dwt_vsstyle.h>

namespace dwt {

template<bool horizontal>
class Splitter :
	public Container
{
	typedef Container BaseType;

	friend class WidgetCreator<Splitter>;

public:
	/// Class type
	typedef Splitter<horizontal> ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		double pos;

		explicit Seed(double pos_ = 0.5);
	};

	double getRelativePos() const {
		return pos;
	}

	void setRelativePos(double pos_) {
		pos = pos_;
		layout();
	}

	Widget* getFirst() {
		return first;
	}
	void setFirst(Widget* first_) {
		first = first_;
		dwtassert(!first || first->getParent() == getParent(), _T("A splitter and its siblings must have the same parent"));
	}

	Widget* getSecond() {
		return second;
	}
	void setSecond(Widget* second_) {
		second = second_;
		dwtassert(!second || second->getParent() == getParent(), _T("A splitter and its siblings must have the same parent"));
	}

	void create(const Seed& cs = Seed());

	void layout(const Rectangle& r) {
		rect = r;
		layout();
	}

protected:
	explicit Splitter(Widget* parent);

	virtual ~Splitter() { }

private:
	Widget* first;
	Widget* second;

	Theme theme;

	double pos;

	bool hovering;
	bool moving;

	Rectangle rect;

	Rectangle getSplitterRect();
	void layout();

	void handlePainting(PaintCanvas& canvas) {
		if(theme) {
			int part, state;
			if(hovering) {
				part = horizontal ? PP_FILL : PP_FILLVERT;
				state = horizontal ? PBFS_NORMAL : PBFVS_NORMAL;
			} else {
				part = horizontal ? PP_BAR : PP_BARVERT;
				state = 0;
			}
			theme.drawBackground(canvas, part, state, canvas.getPaintRect());

		} else if(hovering) {
			// safe to assume that the text color is different enough from the default background.
			canvas.fill(canvas.getPaintRect(), Brush(Brush::WindowText));
		}
	}

	bool handleLButtonDown() {
		::SetCapture(handle());
		moving = true;
		return true;
	}

	bool handleMouseMove(const MouseEvent& mouseEvent) {
		if(!hovering) {
			hovering = true;
			redraw();
			onMouseLeave([this] {
				GCC_WTF->hovering = false;
				GCC_WTF->redraw();
			});
		}

		if(moving && mouseEvent.ButtonPressed == MouseEvent::LEFT) {
			ClientCoordinate cc(mouseEvent.pos, getParent());
			double distance = horizontal ? cc.y() : cc.x();
			double size = horizontal ? rect.size.y : rect.size.x;
			double offset = horizontal ? rect.pos.y : rect.pos.x;
			pos = (distance - offset) / size;
			layout();
		}
		return true;
	}

	bool handleLButtonUp() {
		::ReleaseCapture();
		moving = false;
		return true;
	}
};

template<bool horizontal>
Splitter<horizontal>::Seed::Seed(double pos_) :
BaseType::Seed(),
pos(pos_)
{
}

template<bool horizontal>
Splitter<horizontal>::Splitter(Widget* parent) :
BaseType(parent, NormalDispatcher::newClass<ThisType>(0, 0, ::LoadCursor(0, horizontal ? IDC_SIZENS : IDC_SIZEWE))),
first(0),
second(0),
pos(0.5),
hovering(false),
moving(false)
{
}

template<bool horizontal>
void Splitter<horizontal>::create(const Seed& cs) {
	pos = cs.pos;
	BaseType::create(cs);

	theme.load(VSCLASS_PROGRESS, this);
	onPainting([this](PaintCanvas& canvas) { GCC_WTF->handlePainting(canvas); });

	onLeftMouseDown([this](const MouseEvent&) { return GCC_WTF->handleLButtonDown(); });
	onMouseMove([this](const MouseEvent& mouseEvent) { return GCC_WTF->handleMouseMove(mouseEvent); });
	onLeftMouseUp([this](const MouseEvent&) { return GCC_WTF->handleLButtonUp(); });

	addChild(ToolTip::Seed())->setText(Texts::get(Texts::resize));
}

template<bool horizontal>
Rectangle Splitter<horizontal>::getSplitterRect() {
	// Sanity check
	if(pos < 0.) {
		pos = 0.0;
	} else if(pos > 1.0) {
		pos = 1.0;
	}

	Rectangle rc;
	if(!first || !second) {
		return rc;
	}

	rc = rect;
	double splitterSize = ::GetSystemMetrics(horizontal ? SM_CYEDGE : SM_CXEDGE) + 2;
	if(horizontal) {
		rc.size.y = splitterSize;
		rc.pos.y += pos * rect.height() - splitterSize / 2.;
	} else {
		rc.size.x = splitterSize;
		rc.pos.x += pos * rect.width() - splitterSize / 2.;
	}

	return rc;
}

template<bool horizontal>
void Splitter<horizontal>::layout() {
	if(!first) {
		if(second) {
			second->layout(rect);
		}
		BaseType::layout(getSplitterRect());
		return;
	}
	if(!second) {
		first->layout(rect);
		BaseType::layout(getSplitterRect());
		return;
	}

	Rectangle left = rect, right = rect;
	Rectangle rcSplit = getSplitterRect();

	if(horizontal) {
		left.size.y = rcSplit.y() - left.y();
		right.pos.y = rcSplit.y() + rcSplit.height();
		right.size.y = rect.height() - rcSplit.height() - left.height();
	} else {
		left.size.x = rcSplit.x() - left.x();
		right.pos.x = rcSplit.x() + rcSplit.width();
		right.size.x = rect.width() - rcSplit.width() - left.width();
	}

	first->layout(left);
	second->layout(right);

	BaseType::layout(rcSplit);
}

}

#endif
