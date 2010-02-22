/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_WIN32_MDI_CHILD_FRAME_H_
#define DCPLUSPLUS_WIN32_MDI_CHILD_FRAME_H_

#include "WinUtil.h"

#include "AspectStatus.h"
#include <dwt/widgets/Menu.h>
#include <dcpp/SettingsManager.h>
#include "resource.h"

template<typename T>
class MDIChildFrame :
	public dwt::Container,
	public AspectStatus<T>
{
	typedef dwt::Container BaseType;
	typedef MDIChildFrame<T> ThisType;

	const T& t() const { return *static_cast<const T*>(this); }
	T& t() { return *static_cast<T*>(this); }

protected:
	MDIChildFrame(dwt::TabView* tabView, const tstring& title, unsigned helpId = 0, unsigned iconId = 0, bool manageAccels = true) :
		BaseType(tabView),
		lastFocus(NULL),
		alwaysSameFocus(false),
		reallyClose(false)
	{
		typename ThisType::Seed cs;
		cs.style &= ~WS_VISIBLE;
		cs.caption = title;
		cs.location = tabView->getClientSize();
		this->create(cs);

		if(helpId)
			setHelpId(helpId);

		tabView->add(this, iconId ? WinUtil::tabIcon(iconId) : dwt::IconPtr());

		this->onTabContextMenu(std::tr1::bind(&ThisType::handleContextMenu, this, _1));

		onClosing(std::tr1::bind(&ThisType::handleClosing, this));
		onFocus(std::tr1::bind(&ThisType::handleFocus, this));
		onSized(std::tr1::bind(&ThisType::handleSized, this, _1));
		onActivate(std::tr1::bind(&ThisType::handleActivate, this, _1));
		addDlgCodeMessage(this);

		addAccel(FCONTROL, 'W', std::tr1::bind(&ThisType::close, this, true));
		addAccel(FCONTROL, VK_F4, std::tr1::bind(&ThisType::close, this, true));
		if(manageAccels)
			initAccels();
	}

	virtual ~MDIChildFrame() {
		getParent()->remove(this);
	}

	void handleFocus() {
		if(lastFocus != NULL) {
			::SetFocus(lastFocus);
		}
	}

	/**
	 * The first of two close phases, used to disconnect from other threads that might affect this window.
	 * This is where all stuff that might be affected by other threads goes - it should make sure
	 * that no more messages can arrive from other threads
	 * @return True if close should be allowed, false otherwise
	 */
	bool preClosing() { return true; }
	/** Second close phase, perform any cleanup that depends only on the UI thread */
	void postClosing() { }

	template<typename W>
	void addWidget(W* widget, bool alwaysFocus = false, bool autoTab = true, bool customColor = true) {
		addDlgCodeMessage(widget, autoTab);

		if(customColor)
			addColor(widget);

		if(alwaysFocus || (lastFocus == NULL)) {
			lastFocus = widget->handle();
			if(this->getVisible())
				::SetFocus(lastFocus);
		}
		if(alwaysFocus)
			alwaysSameFocus = true;
	}

	void setDirty(SettingsManager::IntSetting setting) {
		if(SettingsManager::getInstance()->getBool(setting)) {
			getParent()->mark(this);
		}
	}

	void onTabContextMenu(const std::tr1::function<bool (const dwt::ScreenCoordinate&)>& f) {
		getParent()->onTabContextMenu(this, f);
	}

	void activate() {
		getParent()->setActive(this);
	}

	dwt::TabView* getParent() {
		return static_cast<dwt::TabView*>(BaseType::getParent());
	}

	void setIcon(unsigned iconId) {
		getParent()->setIcon(this, WinUtil::tabIcon(iconId));
	}

public:
	virtual const string& getId() const {
		return Util::emptyString;
	}

	virtual const StringMap getWindowParams() const {
		return StringMap();
	}

	static void parseWindowParams(dwt::TabView* parent, const StringMap& /*params*/) {
		T::openWindow(parent);
	}

private:
	HWND lastFocus; // last focused widget
	bool alwaysSameFocus; // always focus the same widget

	bool reallyClose;

	void addDlgCodeMessage(ComboBox* widget, bool autoTab = true) {
		widget->onRaw(std::tr1::bind(&ThisType::handleGetDlgCode, this, _1, autoTab), dwt::Message(WM_GETDLGCODE));
		TextBox* text = widget->getTextBox();
		if(text)
			text->onRaw(std::tr1::bind(&ThisType::handleGetDlgCode, this, _1, autoTab), dwt::Message(WM_GETDLGCODE));
	}

	template<typename W>
	void addDlgCodeMessage(W* widget, bool autoTab = true) {
		widget->onRaw(std::tr1::bind(&ThisType::handleGetDlgCode, this, _1, autoTab), dwt::Message(WM_GETDLGCODE));
	}

	void addColor(ComboBox* widget) {
		// do not apply our custom colors to the combo itself, but apply it to its drop-down and edit controls

		ListBoxPtr listBox = widget->getListBox();
		if(listBox)
			addColor(listBox);

		TextBoxPtr text = widget->getTextBox();
		if(text)
			addColor(text);
	}

	// do not apply our custom colors to Buttons and Button-derived controls
	void addColor(dwt::Button* widget) {
		// empty on purpose
	}

	template<typename A>
	void addColor(dwt::AspectColor<A>* widget) {
		widget->setColor(WinUtil::textColor, WinUtil::bgColor);
	}

	// Catch-rest for the above
	void addColor(void* w) {

	}

	void handleSized(const dwt::SizedEvent& sz) {
		t().layout();
	}

	void handleActivate(bool active) {
		if(active) {
			if(lastFocus) {
				::SetFocus(lastFocus);
			}
		} else if(!alwaysSameFocus) {
			HWND focus = ::GetFocus();
			if(focus != NULL && ::IsChild(t().handle(), focus))
				lastFocus = focus;
		}
	}

	LRESULT handleGetDlgCode(WPARAM wParam, bool autoTab) {
		if(autoTab && wParam == VK_TAB)
			return 0;
		return DLGC_WANTMESSAGE;
	}

	bool handleContextMenu(const dwt::ScreenCoordinate& pt) {
		dwt::Menu::ObjectType menu = addChild(WinUtil::Seeds::menu);

		menu->setTitle(getParent()->getText(this), getParent()->getIcon(this));

		tabMenuImpl(menu);
		menu->appendItem(T_("&Close"), std::tr1::bind(&ThisType::close, this, true), WinUtil::menuIcon(IDI_EXIT));

		menu->open(pt);
		return true;
	}

	bool handleClosing() {
		if(reallyClose) {
			t().postClosing();
			if(getParent()->getActive() == this) {
				// Prevent flicker by selecting the next tab - WM_DESTROY would already be too late
				getParent()->next();
			}
			return true;
		} else if(t().preClosing()) {
			reallyClose = true;
			this->close(true);
			return false;
		}
		return false;
	}

	virtual void tabMenuImpl(dwt::MenuPtr& menu) {
		// empty on purpose; implement this in the derived class to modify the tab menu.
	}
};

#endif
