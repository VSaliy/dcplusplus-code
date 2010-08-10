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

#include <dwt/widgets/TabView.h>

#include <dwt/widgets/Container.h>
#include <dwt/widgets/ToolTip.h>
#include <dwt/WidgetCreator.h>
#include <dwt/util/StringUtils.h>
#include <dwt/DWTException.h>
#include <dwt/resources/Brush.h>
#include <dwt/dwt_vsstyle.h>
#include <dwt/Texts.h>

#include <algorithm>

namespace dwt {

const TCHAR TabView::windowClass[] = WC_TABCONTROL;

TabView::Seed::Seed(unsigned widthConfig_, bool toggleActive_, bool ctrlTab_) :
BaseType::Seed(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |
	TCS_FOCUSNEVER | TCS_HOTTRACK | TCS_MULTILINE | TCS_OWNERDRAWFIXED | TCS_RAGGEDRIGHT | TCS_TOOLTIPS),
font(new Font(DefaultGuiFont)),
widthConfig(widthConfig_),
toggleActive(toggleActive_),
ctrlTab(ctrlTab_)
{
}

TabView::TabView(Widget* w) :
BaseType(w, ChainingDispatcher::superClass<TabView>()),
Themed(this),
tip(0),
toggleActive(false),
font(0),
boldFont(0),
inTab(false),
highlighted(-1),
highlightClose(false),
closeAuthorized(false),
active(-1),
dragging(0)
{
}

void TabView::create(const Seed & cs) {
	if(cs.ctrlTab) {
		addAccel(FCONTROL, VK_TAB, std::bind(&TabView::handleCtrlTab, this, false));
		addAccel(FCONTROL | FSHIFT, VK_TAB, std::bind(&TabView::handleCtrlTab, this, true));
	}

	BaseType::create(cs);

	widthConfig = cs.widthConfig;
	toggleActive = cs.toggleActive;

	if(cs.font)
		font = cs.font;
	else
		font = new Font(DefaultGuiFont);

	if(cs.style & TCS_OWNERDRAWFIXED) {
		dwtassert(dynamic_cast<Control*>(getParent()), _T("Owner-drawn tabs must have a parent derived from dwt::Control"));

		if(widthConfig < 100)
			widthConfig = 100;
		TabCtrl_SetMinTabWidth(handle(), widthConfig);

		LOGFONT lf;
		::GetObject(font->handle(), sizeof(lf), &lf);
		lf.lfWeight = FW_BOLD;
		boldFont = FontPtr(new Font(::CreateFontIndirect(&lf), true));

		loadTheme(VSCLASS_TAB);

		if(!(cs.style & TCS_BUTTONS)) {
			// we don't want pre-drawn borders to get in the way here, so we fully take over painting.
			onPainting(std::bind((void (TabView::*)(PaintCanvas&))(&TabView::handlePainting), this, _1));
		}

		// TCS_HOTTRACK seems to have no effect in owner-drawn tabs, so do the tracking ourselves.
		onMouseMove(std::bind(&TabView::handleMouseMove, this, _1));

	} else {
		if(widthConfig <= 3)
			widthConfig = 0;

		setFont(font);
	}

	imageList = new ImageList(Point(16, 16));

	TabCtrl_SetImageList(handle(), imageList->handle());

	onSelectionChanged(std::bind(&TabView::handleTabSelected, this));
	onLeftMouseDown(std::bind(&TabView::handleLeftMouseDown, this, _1));
	onLeftMouseUp(std::bind(&TabView::handleLeftMouseUp, this, _1));
	onContextMenu(std::bind(&TabView::handleContextMenu, this, _1));
	onMiddleMouseDown(std::bind(&TabView::handleMiddleMouseDown, this, _1));
	onXMouseUp(std::bind(&TabView::handleXMouseUp, this, _1));

	if(cs.style & TCS_TOOLTIPS) {
		tip = WidgetCreator<ToolTip>::attach(this, TabCtrl_GetToolTips(handle())); // created and managed by the tab control thanks to the TCS_TOOLTIPS style
		tip->addRemoveStyle(TTS_NOPREFIX, true);
		tip->onRaw(std::bind(&TabView::handleToolTip, this, _2), Message(WM_NOTIFY, TTN_GETDISPINFO));
	}
}

void TabView::add(ContainerPtr w, const IconPtr& icon) {
	int image = addIcon(icon);

	size_t tabs = size();

	TabInfo* ti = new TabInfo(this, w);
	TCITEM item = {  TCIF_PARAM };
	item.lParam = reinterpret_cast<LPARAM>(ti);

	if(!hasStyle(TCS_OWNERDRAWFIXED)) {
		ti->text = formatTitle(w->getText());
		item.mask |= TCIF_TEXT;
		item.pszText = const_cast<LPTSTR>(ti->text.c_str());
	}

	if(image != -1) {
		item.mask |= TCIF_IMAGE;
		item.iImage = image;
	}

	int newIdx = TabCtrl_InsertItem( handle(), tabs, &item );
	if ( newIdx == - 1 ) {
		throw Win32Exception("Error while trying to add page into Tab Sheet");
	}

	viewOrder.push_front(w);

	if(viewOrder.size() == 1 || w->hasStyle(WS_VISIBLE)) {
		if(viewOrder.size() > 1) {
			swapWidgets(viewOrder.back(), w);
		} else {
			swapWidgets(0, w);
		}
		setActive(tabs);
	}

	layout();

	w->onTextChanging(std::bind(&TabView::handleTextChanging, this, w, _1));
}

ContainerPtr TabView::getActive() const {
	TabInfo* ti = getTabInfo(getSelected());
	return ti ? ti->w : 0;
}

void TabView::remove(ContainerPtr w) {
	int i = findTab(w);
	if(i == -1) {
		return;
	}

	int cur = getSelected();

	if(viewOrder.size() > 1 && i == cur) {
		next();
	}

	viewOrder.remove(w);

	if(w == dragging)
		dragging = 0;

	delete getTabInfo(i);
	erase(i);

	if(size() == 0) {
		active = -1;
		if(titleChangedFunction)
			titleChangedFunction(tstring());
	} else {
		if(i < cur) {
			active--;
		}
	}

	layout();
}

IconPtr TabView::getIcon(unsigned index) const {
	TCITEM item = { TCIF_IMAGE };
	TabCtrl_GetItem(handle(), index, &item);
	if(item.iImage >= 0 && item.iImage < imageList->size())
		return imageList->getIcon(item.iImage);
	return IconPtr();
}

void TabView::setIcon(ContainerPtr w, const IconPtr& icon) {
	int i = findTab(w);
	if(i != -1) {
		int image = addIcon(icon);
		if(image != -1) {
			TCITEM item = { TCIF_IMAGE };
			item.iImage = image;
			TabCtrl_SetItem(this->handle(), i, &item);
		}
	}
}

IconPtr TabView::getIcon(ContainerPtr w) const {
	int i = findTab(w);
	if(i != -1)
		return getIcon(i);
	return IconPtr();
}

void TabView::onTabContextMenu(ContainerPtr w, const ContextMenuFunction& f) {
	TabInfo* ti = getTabInfo(w);
	if(ti) {
		ti->handleContextMenu = f;
	}
}

const TabView::ChildList TabView::getChildren() const {
	ChildList ret;
	for(size_t i = 0; i < size(); ++i) {
		ret.push_back(getTabInfo(i)->w);
	}
	return ret;
}

void TabView::setActive(int i) {
	if(i == -1)
		return;

	setSelected(i);
	handleTabSelected();
}

void TabView::swapWidgets(ContainerPtr oldW, ContainerPtr newW) {
	newW->layout(clientSize);
	newW->setVisible(true);
	if(oldW) {
		oldW->sendMessage(WM_ACTIVATE, WA_INACTIVE, reinterpret_cast<LPARAM>(newW->handle()));
		oldW->setVisible(false);
	}
	newW->sendMessage(WM_ACTIVATE, WA_ACTIVE, oldW ? reinterpret_cast<LPARAM>(oldW->handle()) : 0);
}

void TabView::handleTabSelected() {
	int i = getSelected();
	if(i == active) {
		return;
	}

	TabInfo* old = getTabInfo(active);

	TabInfo* ti = getTabInfo(i);

	if(ti == old)
		return;

	swapWidgets(old ? old->w : 0, ti->w);

	if(!inTab)
		setTop(ti->w);
	active = i;

	if(ti->marked) {
		ti->marked = false;
		if(hasStyle(TCS_OWNERDRAWFIXED)) {
			redraw(i);
		} else {
			TabCtrl_HighlightItem(handle(), i, 0);
		}
	}

	if(titleChangedFunction)
		titleChangedFunction(ti->w->getText());
}

void TabView::mark(ContainerPtr w) {
	int i = findTab(w);
	if(i != -1 && i != getSelected()) {
		bool& marked = getTabInfo(w)->marked;
		if(!marked) {
			marked = true;
			if(hasStyle(TCS_OWNERDRAWFIXED)) {
				redraw(i);
			} else {
				TabCtrl_HighlightItem(handle(), i, 1);
			}
		}
	}
}

int TabView::findTab(ContainerPtr w) const {
	for(size_t i = 0; i < size(); ++i) {
		if(getTabInfo(i)->w == w) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

TabView::TabInfo* TabView::getTabInfo(ContainerPtr w) const {
	return getTabInfo(findTab(w));
}

TabView::TabInfo* TabView::getTabInfo(int i) const {
	if(i != -1) {
		TCITEM item = { TCIF_PARAM };
		TabCtrl_GetItem(handle(), i, &item);
		return reinterpret_cast<TabInfo*>(item.lParam);
	}
	return 0;
}

void TabView::handleTextChanging(ContainerPtr w, const tstring& newText) {
	int i = findTab(w);
	if(i != -1) {
		if(hasStyle(TCS_OWNERDRAWFIXED)) {
			redraw(i);
		} else {
			setText(i, formatTitle(newText));
			layout();
		}

		if((i == active) && titleChangedFunction)
			titleChangedFunction(newText);
	}
}

tstring TabView::formatTitle(tstring title) {
	if(widthConfig && title.length() > widthConfig)
		title = title.substr(0, widthConfig - 3) + _T("...");
	return util::escapeMenu(title);
}

void TabView::layout() {
	BaseType::redraw(Rectangle(Widget::getClientSize()));

	Rectangle tmp = getUsableArea(true);
	if(!(tmp == clientSize)) {
		int i = getSelected();
		if(i != -1) {
			getTabInfo(i)->w->layout(tmp);
		}
		clientSize = tmp;
	}
}

void TabView::next(bool reverse) {
	if(viewOrder.size() < 2) {
		return;
	}
	ContainerPtr wnd = getActive();
	if(!wnd) {
		return;
	}

	WindowIter i;
	if(inTab) {
		i = std::find(viewOrder.begin(), viewOrder.end(), wnd);
		if(i == viewOrder.end()) {
			return;
		}

		if(!reverse) {
			if(i == viewOrder.begin()) {
				i = viewOrder.end();
			}
			--i;
		} else {
			if(++i == viewOrder.end()) {
				i = viewOrder.begin();
			}
		}
	} else {
		if(!reverse) {
			i = --(--viewOrder.end());
		} else {
			i = ++viewOrder.begin();
		}
	}

	setActive(*i);
	return;
}

void TabView::setTop(ContainerPtr wnd) {
	WindowIter i = std::find(viewOrder.begin(), viewOrder.end(), wnd);
	if(i != viewOrder.end() && i != --viewOrder.end()) {
		viewOrder.erase(i);
		viewOrder.push_back(wnd);
	}
}

int TabView::addIcon(const IconPtr& icon) {
	int image = -1;
	if(icon) {
		for(size_t i = 0; i < icons.size(); ++i) {
			if(icon == icons[i]) {
				image = i;
				break;
			}
		}
		if(image == -1) {
			image = icons.size();
			icons.push_back(icon);
			getImageList()->add(*icon);
		}
	}
	return image;
}

LRESULT TabView::handleToolTip(LPARAM lParam) {
	LPNMTTDISPINFO ttdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);
	TabInfo* ti = getTabInfo(ttdi->hdr.idFrom); // here idFrom corresponds to the index of the tab
	if(ti) {
		tipText = highlightClose ? Texts::get(Texts::close) : ti->w->getText();
		ttdi->lpszText = const_cast<LPTSTR>(tipText.c_str());
	}
	return 0;
}

bool TabView::handleLeftMouseDown(const MouseEvent& mouseEvent) {
	TabInfo* ti = getTabInfo(hitTest(mouseEvent.pos));
	if(ti) {
		if(mouseEvent.isShiftPressed)
			ti->w->close();
		else {
			dragging = ti->w;
			::SetCapture(handle());
			if(hasStyle(TCS_OWNERDRAWFIXED)) {
				int index = findTab(dragging);
				if(index == active) {
					closeAuthorized = inCloseRect(mouseEvent.pos);
					redraw(index);
				}
			}
		}
	}
	return true;
}

bool TabView::handleLeftMouseUp(const MouseEvent& mouseEvent) {
	bool closeAuth = closeAuthorized;
	closeAuthorized = false;
	::ReleaseCapture();

	if(dragging) {
		int dragPos = findTab(dragging);
		dragging = 0;

		if(dragPos == -1)
			return true;

		int dropPos = hitTest(mouseEvent.pos);

		if(dropPos == -1) {
			// not in the tab control; move the tab to the end
			dropPos = size() - 1;
		}

		if(dropPos == dragPos) {
			// the tab hasn't moved; handle the click
			if(dropPos == active) {
				if(closeAuth && inCloseRect(mouseEvent.pos)) {
					TabInfo* ti = getTabInfo(active);
					if(ti)
						ti->w->close();
				} else if(toggleActive)
					next();
			} else
				setActive(dropPos);
			return true;
		}

		// save some information about the tab before we erase it
		TCITEM item = { TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE };
		TCHAR buf[1024] = { 0 };
		item.pszText = buf;
		item.cchTextMax = (sizeof(buf) / sizeof(TCHAR)) - 1;

		TabCtrl_GetItem( this->handle(), dragPos, & item );

		erase(dragPos);

		TabCtrl_InsertItem(handle(), dropPos, &item);

		active = getSelected();

		layout();
	}

	return true;
}

bool TabView::handleContextMenu(ScreenCoordinate pt) {
	TabInfo* ti = 0;
	if(pt.x() == -1 && pt.y() == -1) {
		int i = getSelected();

		RECT rc;
		if(i == -1 || !TabCtrl_GetItemRect(handle(), i, &rc)) {
			return false;
		}
		pt = ScreenCoordinate(Point(rc.left, rc.top));
		ti = getTabInfo(i);
	} else {
		int i = hitTest(pt);
		if(i == -1) {
			return false;
		}
		ti = getTabInfo(i);
	}

	if(ti->handleContextMenu && ti->handleContextMenu(pt)) {
		return true;
	}

	return false;
}

bool TabView::handleMiddleMouseDown(const MouseEvent& mouseEvent) {
	TabInfo* ti = getTabInfo(hitTest(mouseEvent.pos));
	if(ti)
		ti->w->close();
	return true;
}

bool TabView::handleXMouseUp(const MouseEvent& mouseEvent) {
	switch(mouseEvent.ButtonPressed) {
	case MouseEvent::X1: next(true); break;
	case MouseEvent::X2: next(); break;
	}
	return true;
}

bool TabView::handleMouseMove(const MouseEvent& mouseEvent) {
	int i = hitTest(mouseEvent.pos);
	if(highlighted != -1 && i != highlighted) {
		redraw(highlighted);
		highlighted = -1;
		highlightClose = false;
	}
	if(i != -1 && i != highlighted) {
		redraw(i);
		highlighted = i;
		onMouseLeave(std::bind(&TabView::handleMouseLeave, this));
	}
	if(i != -1 && i == active) {
		if(highlightClose ^ inCloseRect(mouseEvent.pos)) {
			highlightClose = !highlightClose;
			if(tip)
				tip->refresh();
			redraw(i);
		}
	}
	return false;
}

void TabView::handleMouseLeave() {
	if(highlighted != -1) {
		redraw(highlighted);
		highlighted = -1;
	}
}

bool TabView::handlePainting(LPDRAWITEMSTRUCT info, TabInfo* ti) {
	FreeCanvas canvas(info->hDC);
	bool oldMode = canvas.setBkMode(true);

	Rectangle rect(info->rcItem);
	if(theme) {
		// remove some borders
		rect.pos.x -= 1;
		rect.pos.y -= 1;
		rect.size.x += 2;
		rect.size.y += 1;
	}

	draw(canvas, info->itemID, std::move(rect), (info->itemState & ODS_SELECTED) == ODS_SELECTED);

	canvas.setBkMode(oldMode);
	return true;
}

void TabView::handlePainting(PaintCanvas& canvas) {
	Rectangle rect(canvas.getPaintRect());
	if(rect.width() == 0 || rect.height() == 0)
		return;

	bool oldMode = canvas.setBkMode(true);

	int sel = getSelected();
	Rectangle selRect;

	for(size_t i = 0; i < size(); ++i) {
		RECT rc;
		if(TabCtrl_GetItemRect(handle(), i, &rc) &&
			(rc.right >= rect.left() || rc.left <= rect.right()) &&
			(rc.bottom >= rect.top() || rc.top <= rect.bottom()))
		{
			if(static_cast<int>(i) == sel) {
				rc.top -= 2;
				rc.bottom += 1;
				rc.left -= 1;
				rc.right += 1;
				selRect = Rectangle(rc);
			} else {
				draw(canvas, i, Rectangle(rc), false);
			}
		}
	}

	// draw the selected tab last because it might need to step on others
	if(selRect.height() > 0)
		draw(canvas, sel, std::move(selRect), true);

	canvas.setBkMode(oldMode);
}

void TabView::draw(Canvas& canvas, unsigned index, Rectangle&& rect, bool isSelected) {
	TabInfo* ti = getTabInfo(index);
	if(!ti)
		return;

	bool isHighlighted = static_cast<int>(index) == highlighted || ti->marked;

	int part, state;
	if(theme) {
		part = TABP_TABITEM;
		state = isSelected ? TIS_SELECTED : isHighlighted ? TIS_HOT : TIS_NORMAL;

		drawThemeBackground(canvas, part, state, rect);

	} else {
		canvas.fill(rect, Brush(isSelected ? Brush::Window : isHighlighted ? Brush::HighLight : Brush::BtnFace));
	}

	if(isSelected && theme && !hasStyle(TCS_BUTTONS)) {
		rect.pos.y += 1;
		rect.size.y -= 1;
	}

	const Point margin(4, 1);
	rect.pos += margin;
	rect.size -= margin + margin;

	IconPtr icon = getIcon(index);
	if(icon) {
		Point size = icon->getSize();
		Point pos = rect.pos;
		if(size.y < rect.size.y)
			pos.y += (rect.size.y - size.y) / 2; // center the icon vertically
		canvas.drawIcon(icon, Rectangle(pos, size));

		size.x += margin.x;
		rect.pos.x += size.x;
		rect.size.x -= size.x;
	}

	if(isSelected)
		rect.size.x -= margin.x + 16; // keep some space for the 'X' button

	const tstring text = ti->w->getText();
	const unsigned dtFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_WORD_ELLIPSIS;
	Canvas::Selector select(canvas, *((isSelected || ti->marked) ? boldFont : font));
	if(theme) {
		drawThemeText(canvas, part, state, text, dtFormat, rect);
	} else {
		canvas.setTextColor(::GetSysColor(isSelected ? COLOR_WINDOWTEXT : isHighlighted ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
		canvas.drawText(text, rect, dtFormat);
	}

	if(isSelected) {
		rect.pos.x = rect.right() + margin.x;
		rect.size.x = 16;
		if(16 < rect.size.y)
			rect.pos.y += (rect.size.y - 16) / 2; // center the icon vertically
		rect.size.y = 16;

		UINT format = DFCS_CAPTIONCLOSE | DFCS_FLAT;
		if(isHighlighted && highlightClose) {
			format |= DFCS_HOT;
			if(closeAuthorized)
				format |= DFCS_PUSHED;
		}
		::RECT rc(rect);
		::DrawFrameControl(canvas.handle(), &rc, DFC_CAPTION, format);

		closeRect = rect;
		closeRect.pos = ScreenCoordinate(ClientCoordinate(closeRect.pos, this)).getPoint();
	}
}

bool TabView::inCloseRect(const ScreenCoordinate& pos) const {
	if(closeRect.width() > 0 && closeRect.height() > 0) {
		::RECT rc(closeRect);
		return ::PtInRect(&rc, pos.getPoint());
	}
	return false;
}

void TabView::helpImpl(unsigned& id) {
	// we have the help id of the whole tab control; convert to the one of the specific tab the user just clicked on
	TabInfo* ti = getTabInfo(hitTest(ScreenCoordinate(Point::fromLParam(::GetMessagePos()))));
	if(ti)
		id = ti->w->getHelpId();
}

void TabView::handleCtrlTab(bool shift) {
	inTab = true;
	next(shift);
}

bool TabView::filter(const MSG& msg) {
	if(tip)
		tip->relayEvent(msg);

	/* handle Alt+Left and Alt+Right here instead of setting up global accelerators in order to be
	able to allow further dispatching if we can't move more to the left or to the right. */
	if(msg.message == WM_SYSKEYDOWN && active != -1) {
		switch(static_cast<int>(msg.wParam)) {
		case VK_LEFT:
			if(isAltPressed() && active > 0) {
				setActive(active - 1);
				return true;
			}
			break;
		case VK_RIGHT:
			if(isAltPressed() && active < static_cast<int>(size()) - 1) {
				setActive(active + 1);
				return true;
			}
			break;
		}
	}

	if(inTab && msg.message == WM_KEYUP && msg.wParam == VK_CONTROL) {
		inTab = false;

		TabInfo* ti = getTabInfo(getSelected());
		if(ti) {
			setTop(ti->w);
		}
	}

	return false;
}

void TabView::setText(unsigned index, const tstring& text) {
	TabInfo* ti = getTabInfo(index);
	if(ti) {
		ti->text = text;
		TCITEM item = { TCIF_TEXT };
		item.pszText = const_cast<LPTSTR>(ti->text.c_str());
		TabCtrl_SetItem(handle(), index, &item);
	}
}

void TabView::redraw(unsigned index) {
	RECT rect;
	if(TabCtrl_GetItemRect(handle(), index, &rect)) {
		BaseType::redraw(Rectangle(rect));
	}
}

dwt::Rectangle TabView::getUsableArea(bool cutBorders) const
{
	RECT rc;
	::GetClientRect(handle(), &rc);
	TabCtrl_AdjustRect( this->handle(), false, &rc );
	Rectangle rect( rc );
	if(cutBorders) {
		Rectangle rctabs(Widget::getClientSize());
		// Get rid of ugly border...assume y border is the same as x border
		const long border = (rctabs.width() - rect.width()) / 2;
		const long upStretching = hasStyle(TCS_BUTTONS) ? 4 : 2;
		rect.pos.x = rctabs.x();
		rect.pos.y -= upStretching;
		rect.size.x = rctabs.width();
		rect.size.y += border + upStretching;
	}
	return rect;
}

const ImageListPtr& TabView::getImageList() const {
	return imageList;
}

int TabView::hitTest(const ScreenCoordinate& pt) {
	TCHITTESTINFO tci = { ClientCoordinate(pt, this).getPoint() };
	return TabCtrl_HitTest(handle(), &tci);
}

bool TabView::handleMessage( const MSG & msg, LRESULT & retVal ) {
	if(msg.message == WM_PAINT && !theme) {
		// let the tab control draw the borders of unthemed tabs (and revert to classic owner-draw callbacks).
		return false;
	}

	bool handled = BaseType::handleMessage(msg, retVal);

	if(msg.message == WM_SIZE) {
		// We need to let the tab control window proc handle this first, otherwise getUsableArea will not return
		// correct values on mulitrow tabs (since the number of rows might change with the size)
		retVal = getDispatcher().chain(msg);

		layout();

		return true;
	}

	if(!handled && msg.message == WM_COMMAND && getActive()) {
		// Forward commands to the active tab
		handled = getActive()->handleMessage(msg, retVal);
	}

	return handled;
}

}
