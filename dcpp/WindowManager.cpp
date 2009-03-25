/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "WindowManager.h"

#include "WindowInfo.h"
#include "SimpleXML.h"

namespace dcpp {

WindowManager::WindowManager() {
	SettingsManager::getInstance()->addListener(this);
}

WindowManager::~WindowManager() throw() {
	SettingsManager::getInstance()->removeListener(this);
}

void WindowManager::autoOpen(bool skipHubs) {
	Lock l(cs);
	for(WindowInfoList::const_iterator i = list.begin(); i != list.end(); ++i) {
		fire(WindowManagerListener::Window(), i->getId(), i->getParams(), skipHubs);
	}
}

void WindowManager::lock() {
	cs.enter();
}

void WindowManager::unlock() {
	cs.leave();
}

void WindowManager::add(const string& id, const StringMap& params) {
	list.push_back(WindowInfo(id, params));
}

void WindowManager::clear() {
	list.clear();
}

void WindowManager::addRecent(const string& id, const StringMap& params) {
	Lock l(cs);
	addRecent_(id, params);
}

void WindowManager::addRecent_(const string& id, const StringMap& params) {
	WindowInfo info(id, params);

	if(recent.find(id) == recent.end()) {
		recent[id] = WindowInfoList(1, info);
		return;
	}

	WindowInfoList& infoList = recent[id];
	WindowInfoList::iterator i = std::find(infoList.begin(), infoList.end(), info);
	if(i == infoList.end()) {
		infoList.insert(infoList.begin(), info);
		if(infoList.size() > 10) /// @todo configurable?
			infoList.erase(infoList.end() - 1);
	} else {
		infoList.erase(i);
		infoList.insert(infoList.begin(), info);
	}
}

void WindowManager::updateRecent(const string& id, const StringMap& params) {
	Lock l(cs);
	RecentList::iterator ri = recent.find(id);
	if(ri != recent.end()) {
		WindowInfo info(id, params);
		WindowInfoList::iterator i = std::find(ri->second.begin(), ri->second.end(), info);
		if(i != ri->second.end())
			i->setParams(params);
	}
}

void WindowManager::parseTags(SimpleXML& xml, handler_type handler) {
	xml.stepIn();

	while(xml.findChild("Window")) {
		const string& id = xml.getChildAttrib("Id");
		if(id.empty())
			continue;

		StringMap params;
		xml.stepIn();
		while(xml.findChild("Param")) {
			const string& id_ = xml.getChildAttrib("Id");
			if(id_.empty())
				continue;
			params[id_] = xml.getChildData();
		}
		xml.stepOut();

		(this->*handler)(id, params);
	}

	xml.stepOut();
}

void WindowManager::addTag(SimpleXML& xml, const WindowInfo& info) const {
	xml.addTag("Window");
	xml.addChildAttrib("Id", info.getId());

	if(!info.getParams().empty()) {
		xml.stepIn();
		for(StringMap::const_iterator i = info.getParams().begin(); i != info.getParams().end(); ++i) {
			xml.addTag("Param", i->second);
			xml.addChildAttrib("Id", i->first);
		}
		xml.stepOut();
	}
}

void WindowManager::on(SettingsManagerListener::Load, SimpleXML& xml) throw() {
	Lock l(cs);
	clear();

	xml.resetCurrentChild();
	if(xml.findChild("Windows"))
		parseTags(xml, &WindowManager::add);

	if(xml.findChild("Recent"))
		parseTags(xml, &WindowManager::addRecent_);
}

void WindowManager::on(SettingsManagerListener::Save, SimpleXML& xml) throw() {
	Lock l(cs);

	xml.addTag("Windows");
	xml.stepIn();
	for(WindowInfoList::const_iterator i = list.begin(); i != list.end(); ++i)
		addTag(xml, *i);
	xml.stepOut();

	xml.addTag("Recent");
	xml.stepIn();
	for(RecentList::const_iterator ri = recent.begin(); ri != recent.end(); ++ri) {
		const WindowInfoList& infoList = ri->second;
		for(WindowInfoList::const_iterator i = infoList.begin(); i != infoList.end(); ++i)
			addTag(xml, *i);
	}
	xml.stepOut();
}

} // namespace dcpp
