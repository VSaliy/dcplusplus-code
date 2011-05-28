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

#include <dwt/widgets/Splitter.h>

#include <dwt/dwt_vsstyle.h>
#include <dwt/Texts.h>
#include <dwt/WidgetCreator.h>
#include <dwt/resources/Brush.h>
#include <dwt/widgets/SplitterContainer.h>
#include <dwt/widgets/ToolTip.h>

namespace dwt {

void Splitter::create(const Seed& cs) {
	pos = cs.pos;
	horizontal = cs.horizontal;
	BaseType::create(cs);

	theme.load(VSCLASS_WINDOW, this);
	onPainting([this](PaintCanvas& canvas) { GCC_WTF->handlePainting(canvas); });

	onLeftMouseDown([this](const MouseEvent&) { return GCC_WTF->handleLButtonDown(); });
	onMouseMove([this](const MouseEvent& mouseEvent) { return GCC_WTF->handleMouseMove(mouseEvent); });
	onLeftMouseUp([this](const MouseEvent&) { return GCC_WTF->handleLButtonUp(); });

	WidgetCreator<ToolTip>::create(this, ToolTip::Seed())->setText(Texts::get(Texts::resize));
}

SplitterContainerPtr Splitter::getParent() const {
	return static_cast<SplitterContainerPtr>(BaseType::getParent());
}

void Splitter::handlePainting(PaintCanvas& canvas) {
	if(theme) {
		Rectangle rect(getClientSize());
		// don't draw edges.
		(horizontal ? rect.pos.x : rect.pos.y) -= 2;
		(horizontal ? rect.size.x : rect.size.y) += 4;

		theme.drawBackground(canvas, WP_CAPTION, CS_ACTIVE, rect, true, canvas.getPaintRect());

	} else {
		canvas.fill(canvas.getPaintRect(), Brush(Brush::ActiveCaption));
	}

	if(hovering) {
		canvas.invert(Region(canvas.getPaintRect()));
	}
}

bool Splitter::handleMouseMove(const MouseEvent& mouseEvent) {
	if(!hovering) {
		hovering = true;
		redraw();
		::SetCursor(::LoadCursor(0, horizontal ? IDC_SIZENS : IDC_SIZEWE));
		onMouseLeave([this] {
			GCC_WTF->hovering = false;
			GCC_WTF->redraw();
			::SetCursor(NULL);

		});
	}

	if(moving && mouseEvent.ButtonPressed == MouseEvent::LEFT) {
		ClientCoordinate cc(mouseEvent.pos, getParent());
		pos = (horizontal ? cc.y() : cc.x()) / getParent()->getMaxSize(this);
		if(pos < 0.) pos = 0.;
		if(pos > 1.) pos = 1.;
		getParent()->onMove();
	}

	return true;
}

};
