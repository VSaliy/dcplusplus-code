/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2009, Jacek Sieka

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

#include <algorithm>

namespace dwt {

const TCHAR TabView::windowClass[] = WC_TABCONTROL;

TabView::Seed::Seed(unsigned maxLength_, bool toggleActive_) :
	BaseType::Seed(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE |
		 TCS_HOTTRACK | TCS_MULTILINE | TCS_RAGGEDRIGHT | TCS_TOOLTIPS | TCS_FOCUSNEVER),
	font(new Font(DefaultGuiFont)),
	maxLength(maxLength_),
	toggleActive(toggleActive_)
{
}

TabView::TabView(Widget* w) :
	BaseType(w, ChainingDispatcher::superClass<TabView>()),
	tip(0),
	toggleActive(false),
	inTab(false),
	active(-1),
	dragging(0)
	{ }

void TabView::create(const Seed & cs) {
	BaseType::create(cs);
	maxLength = cs.maxLength;
	if(maxLength <= 3)
		maxLength = 0;
	toggleActive = cs.toggleActive;

	if(cs.font)
		setFont( cs.font );

	imageList = new ImageList(16, 16, ILC_COLOR32 | ILC_MASK);

	TabCtrl_SetImageList(handle(), imageList->handle());

	onSelectionChanged(std::tr1::bind(&TabView::handleTabSelected, this));
	onLeftMouseDown(std::tr1::bind(&TabView::handleLeftMouseDown, this, _1));
	onLeftMouseUp(std::tr1::bind(&TabView::handleLeftMouseUp, this, _1));
	onContextMenu(std::tr1::bind(&TabView::handleContextMenu, this, _1));
	onMiddleMouseDown(std::tr1::bind(&TabView::handleMiddleMouseDown, this, _1));
	onXMouseUp(std::tr1::bind(&TabView::handleXMouseUp, this, _1));

	if(cs.style & TCS_TOOLTIPS) {
		tip = WidgetCreator<ToolTip>::attach(this, TabCtrl_GetToolTips(handle())); // created and managed by the tab control thanks to the TCS_TOOLTIPS style
		tip->addRemoveStyle(TTS_NOPREFIX, true);
		tip->onRaw(std::tr1::bind(&TabView::handleToolTip, this, _2), Message(WM_NOTIFY, TTN_GETDISPINFO));
	}
}

void TabView::add(ContainerPtr w, const IconPtr& icon) {
	int image = addIcon(icon);
	size_t tabs = size();
	TabInfo* ti = new TabInfo(w);
	tstring title = formatTitle(w->getText());

	TCITEM item = { 0 };
	item.mask = TCIF_TEXT | TCIF_PARAM;
	item.pszText = const_cast < TCHAR * >( title.c_str() );
	item.lParam = reinterpret_cast<LPARAM>(ti);

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

	w->onTextChanging(std::tr1::bind(&TabView::handleTextChanging, this, w, _1));
}

ContainerPtr TabView::getActive() {
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

IconPtr TabView::getTabIcon(ContainerPtr w) {
	int i = findTab(w);
	if(i != -1) {
		TCITEM item = { TCIF_IMAGE };
		TabCtrl_GetItem(handle(), i, &item);
		if(item.iImage >= 0 && item.iImage < imageList->size())
			return imageList->getIcon(item.iImage);
	}
	return IconPtr();
}

void TabView::setTabIcon(ContainerPtr w, const IconPtr& icon) {
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

tstring TabView::getTabText(ContainerPtr w) {
	int i = findTab(w);
	if(i != -1)
		return getText(i);
	return tstring();
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
	newW->setBounds(clientSize, false);
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

	TabCtrl_HighlightItem(handle(), i, FALSE);

	if(titleChangedFunction)
		titleChangedFunction(ti->w->getText());
}

void TabView::mark(ContainerPtr w) {
	int i = findTab(w);
	if(i != -1 && i != getSelected()) {
		TabCtrl_HighlightItem(handle(), i, TRUE);
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
		setText(i, formatTitle(newText));
		layout();

		if((i == active) && titleChangedFunction)
			titleChangedFunction(newText);
	}
}

tstring TabView::formatTitle(tstring title) {
	if(maxLength && title.length() > maxLength)
		title = title.substr(0, maxLength - 3) + _T("...");
	return util::escapeMenu(title);
}

void TabView::handleSized(const SizedEvent& sz) {
	layout();
}

void TabView::layout() {
	Rectangle tmp = getUsableArea(true);
	if(!(tmp == clientSize)) {
		int i = getSelected();
		if(i != -1) {
			getTabInfo(i)->w->setBounds(tmp);
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
		tipText = ti->w->getText();
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
		}
	}
	return true;
}

bool TabView::handleLeftMouseUp(const MouseEvent& mouseEvent) {
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
				if(toggleActive)
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

void TabView::helpImpl(unsigned& id) {
	// we have the help id of the whole tab control; convert to the one of the specific tab the user just clicked on
	TabInfo* ti = getTabInfo(hitTest(ScreenCoordinate(Point::fromLParam(::GetMessagePos()))));
	if(ti)
		id = ti->w->getHelpId();
}

bool TabView::filter(const MSG& msg) {
	if(tip)
		tip->relayEvent(msg);

	if(msg.message == WM_KEYUP && msg.wParam == VK_CONTROL) {
		inTab = false;

		TabInfo* ti = getTabInfo(getSelected());
		if(ti) {
			setTop(ti->w);
		}
	} else if(msg.message == WM_KEYDOWN && msg.wParam == VK_TAB && ::GetKeyState(VK_CONTROL) < 0) {
		inTab = true;
		next(::GetKeyState(VK_SHIFT) < 0);
		return true;
	}
	return false;
}

void TabView::setText( unsigned index, const tstring& text )
{
	TCITEM item = { TCIF_TEXT };
	item.pszText = const_cast < TCHAR * >( text.c_str() );
	TabCtrl_SetItem(this->handle(), index, &item);
}

tstring TabView::getText(unsigned idx) const
{
	TCITEM item = { TCIF_TEXT };
	TCHAR buffer[1024];
	item.cchTextMax = (sizeof(buffer) / sizeof(TCHAR)) - 1 ;
	item.pszText = buffer;
	if ( !TabCtrl_GetItem( this->handle(), idx, & item ) )
	{
		throw Win32Exception("Couldn't retrieve text in TabView::getText.");
	}
	return buffer;
}

dwt::Rectangle TabView::getUsableArea(bool cutBorders) const
{
	RECT rc;
	::GetClientRect(handle(), &rc);
	TabCtrl_AdjustRect( this->handle(), false, &rc );
	Rectangle rect( rc );
	if(cutBorders) {
		Rectangle rctabs(getClientAreaSize());
		// Get rid of ugly border...assume y border is the same as x border
		const long border = (rctabs.width() - rect.width()) / 2;
		const long upTranslation = 2;
		rect.pos.x = rctabs.x();
		rect.pos.y -= upTranslation;
		rect.size.x = rctabs.width();
		rect.size.y += border + upTranslation;
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

bool TabView::tryFire( const MSG & msg, LRESULT & retVal ) {
	bool handled = BaseType::tryFire(msg, retVal);

	if(msg.message == WM_SIZE) {
		// We need to let the tab control window proc handle this first, otherwise getUsableArea will not return
		// correct values on mulitrow tabs (since the number of rows might change with the size)
		retVal = getDispatcher().chain(msg);

		handleSized(SizedEvent(msg));

		return true;
	}

	if(!handled && msg.message == WM_COMMAND && getActive()) {
		// Forward commands to the active tab
		handled = getActive()->tryFire(msg, retVal);
	}

	return handled;
}

}
