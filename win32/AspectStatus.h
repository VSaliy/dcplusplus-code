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

#include <dwt/widgets/StatusBar.h>
#include "WinUtil.h"

template<class WidgetType>
class AspectStatus {
	typedef AspectStatus<WidgetType> ThisType;

	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

protected:
	AspectStatus() : status(0), tip(0) {
		filterIter = dwt::Application::instance().addFilter(std::tr1::bind(&ThisType::filter, this, _1));
	}

	~AspectStatus() {
		dwt::Application::instance().removeFilter(filterIter);
	}

	void initStatus(bool sizeGrip = false) {
		dwt::StatusBar::Seed cs(WidgetType::STATUS_LAST, WidgetType::STATUS_STATUS, sizeGrip);
		cs.font = WinUtil::font;
		status = W().addChild(cs);
		status->onHelp(std::tr1::bind(&WinUtil::help, _1, _2));

		tip = W().addChild(dwt::ToolTip::Seed());
		tip->setTool(status, std::tr1::bind(&ThisType::handleToolTip, this, _1));
	}

	void setStatus(unsigned part, const tstring& text, bool alwaysResize = false) {
		/// @todo move this to dwt
		if(part == WidgetType::STATUS_STATUS) {
			lastLines.push_back(text);
			while(lastLines.size() > MAX_LINES) {
				lastLines.erase(lastLines.begin());
			}
		}

		status->setText(part, text, alwaysResize);
	}

	dwt::StatusBarPtr status;

private:
	dwt::Application::FilterIter filterIter;
	dwt::ToolTipPtr tip;
	TStringList lastLines;

	enum { MAX_LINES = 10 };

	bool filter(const MSG& msg) {
		tip->relayEvent(msg);
		return false;
	}

	void handleToolTip(tstring& text) {
		tip->setMaxTipWidth(status->getSize(WidgetType::STATUS_STATUS));
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
