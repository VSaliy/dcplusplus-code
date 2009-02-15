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
	for(WindowInfoList::const_iterator i = list.begin(); i != list.end(); ++i) {
		if(i->getAddress().empty())
			fire(WindowManagerListener::Window(), i->getId());
		else if(!skipHubs)
			fire(WindowManagerListener::Hub(), i->getName(), i->getAddress());
	}
}

void WindowManager::lock() {
	cs.enter();
}

void WindowManager::unlock() {
	cs.leave();
}

void WindowManager::add(const string& id) {
	list.push_back(WindowInfo(id));
}

void WindowManager::add(const string& name, const string& address) {
	list.push_back(WindowInfo(name, address));
}

void WindowManager::clear() {
	list.clear();
}

void WindowManager::on(SettingsManagerListener::Load, SimpleXML& xml) throw() {
	Lock l(cs);
	clear();

	xml.resetCurrentChild();
	if(xml.findChild("Windows")) {
		xml.stepIn();

		while(xml.findChild("Window")) {
			const string& address = xml.getChildAttrib("Address");
			if(address.empty()) {
				const string& id = xml.getChildAttrib("Id");
				if(!id.empty())
					add(id);
			} else
				add(xml.getChildAttrib("Name"), address);
		}

		xml.stepOut();
	}
}

void WindowManager::on(SettingsManagerListener::Save, SimpleXML& xml) throw() {
	Lock l(cs);

	xml.addTag("Windows");
	xml.stepIn();

	for(WindowInfoList::const_iterator i = list.begin(); i != list.end(); ++i) {
		xml.addTag("Window");

		if(i->getAddress().empty()) {
			xml.addChildAttrib("Id", i->getId());
		} else {
			// hub window
			xml.addChildAttrib("Name", i->getName());
			xml.addChildAttrib("Address", i->getAddress());
		}
	}

	xml.stepOut();
}

} // namespace dcpp
