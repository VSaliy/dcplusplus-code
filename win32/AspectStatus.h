/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_ASPECTSTATUS_H_
#define DCPLUSPLUS_WIN32_ASPECTSTATUS_H_

#include <boost/lambda/lambda.hpp>
#include <dwt/widgets/StatusBar.h>
#include "WinUtil.h"

template<class WidgetType>
class AspectStatus {
	typedef AspectStatus<WidgetType> ThisType;

	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

protected:
	AspectStatus() : status(0), statusNeedsLayout(false), tip(0) {
		statusSizes.resize(WidgetType::STATUS_LAST);
		filterIter = dwt::Application::instance().addFilter(std::tr1::bind(&ThisType::filter, this, _1));
	}

	~AspectStatus() {
		dwt::Application::instance().removeFilter(filterIter);
	}

	void initStatus(bool sizeGrip = false) {
		dwt::StatusBar::Seed cs(sizeGrip);
		cs.font = WinUtil::font;
		status = W().addChild(cs);
		status->onHelp(std::tr1::bind(&ThisType::handleHelp, this, _1, _2));

		tip = W().addChild(dwt::ToolTip::Seed());
		tip->setTool(status, std::tr1::bind(&ThisType::handleToolTip, this, _1));
	}

	/**
	* change the text of a part of the status bar
	* @param s index of the part (section) to update.
	* @param text new text to put into "s".
	* @param layout if the part is resized, then the status bar needs a layout. when "layout" is
	* true, the setStatus function takes care of it; otherwise, "statusNeedsLayout" will be set to
	* true and it is the caller's responsibility to update the status bar's layout and set
	* "statusNeedsLayout" back to false.
	* @param alwaysResize if false, the part will be resized only if the new text is too big for
	* the current; if true, the size of the part will always be adjusted to the text it contains.
	* note: setting "alwaysResize" to true for often changing parts might result in flickering.
	*/
	void setStatus(int s, const tstring& text, bool layout = true, bool alwaysResize = false) {
		if(s != WidgetType::STATUS_STATUS) {
			int oldW = statusSizes[s];
			int newW = status->getTextSize(text).x + 12;
			if(newW > oldW || (alwaysResize && newW != oldW)) {
				statusSizes[s] = newW;
				if(layout)
					layoutSections(status->getSize());
				else
					statusNeedsLayout = true;
			}
		} else {
			lastLines.push_back(text);
			while(lastLines.size() > MAX_LINES) {
				lastLines.erase(lastLines.begin());
			}
		}
		status->setText(text, s);
	}

	void setStatusHelpId(int s, unsigned id) {
		helpIds[s] = id;
	}

	void layoutStatus(dwt::Rectangle& r) {
		status->refresh();

		dwt::Point sz(status->getSize());
		r.size.y -= sz.y;
		layoutSections(sz);
	}

	void layoutSections(const dwt::Point& sz) {
		statusSizes[WidgetType::STATUS_STATUS] = 0;
		statusSizes[WidgetType::STATUS_STATUS] = sz.x - std::accumulate(statusSizes.begin(), statusSizes.end(), 0);

		status->setSections(statusSizes);
	}

	template<typename A>
	void mapWidget(int s, dwt::AspectSizable<A>* widget, const dwt::Rectangle& padding = dwt::Rectangle(0, 0, 0, 0)) {
		POINT p[2];
		status->sendMessage(SB_GETRECT, s, reinterpret_cast<LPARAM>(p));
		::MapWindowPoints(status->handle(), H(), (POINT*)p, 2);
		widget->setBounds(
			p[0].x + padding.left(),
			p[0].y + padding.top(),
			p[1].x - p[0].x - padding.right(),
			p[1].y - p[0].y - padding.bottom()
			);
	}

	dwt::StatusBarPtr status;

	std::vector<unsigned> statusSizes;
	bool statusNeedsLayout;

private:
	dwt::Application::FilterIter filterIter;
	dwt::ToolTipPtr tip;
	TStringList lastLines;

	enum { MAX_LINES = 10 };

	typedef std::tr1::unordered_map<int, unsigned> HelpIdsMap;
	HelpIdsMap helpIds;

	bool filter(const MSG& msg) {
		tip->relayEvent(msg);
		return false;
	}

	void handleHelp(HWND hWnd, unsigned id) {
		if(!dwt::AspectKeyboardBase::isKeyPressed(VK_F1)) {
			// we have the help id of the whole status bar; convert to the one of the specific part the user just clicked on
			dwt::Point pt = dwt::Point::fromLParam(::GetMessagePos());
			RECT rect = status->getBounds(false);
			if(::PtInRect(&rect, pt)) {
				unsigned x = dwt::ClientCoordinate(dwt::ScreenCoordinate(pt), status).x();
				unsigned total = 0;
				boost::lambda::var_type<unsigned>::type v(boost::lambda::var(total));
				HelpIdsMap::const_iterator i = helpIds.find(find_if(statusSizes.begin(), statusSizes.end(), (v += boost::lambda::_1, v > x)) - statusSizes.begin());
				if(i != helpIds.end())
					id = i->second;
			}
		}
		WinUtil::help(hWnd, id);
	}

	void handleToolTip(tstring& text) {
		tip->setMaxTipWidth(statusSizes[WidgetType::STATUS_STATUS]);
		text.clear();
		for(size_t i = 0; i < lastLines.size(); ++i) {
			if(i > 0) {
				text += _T("\r\n");
			}
			text += lastLines[i];
		}
	}
};

#endif /*ASPECTSTATUS_H_*/
