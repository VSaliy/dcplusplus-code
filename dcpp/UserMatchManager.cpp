/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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
#include "UserMatchManager.h"

#include "Client.h"
#include "ClientManager.h"
#include "SimpleXML.h"

namespace dcpp {

UserMatchManager::UserMatchManager() {
	SettingsManager::getInstance()->addListener(this);
}

UserMatchManager::~UserMatchManager() {
	SettingsManager::getInstance()->removeListener(this);
}

UserMatchManager::UserMatches UserMatchManager::getList() const {
	return list;
}

void UserMatchManager::setList(UserMatches&& newList) {
	if(list.empty() && newList.empty())
		return;

	// operate within the ClientManager lock.
	auto cm = ClientManager::getInstance();
	auto lock = cm->lock();

	// swap the new list.
#ifdef __GNUC__ /// @todo GCC doesn't seem to support vector swapping to an rvalue ref...
	auto& lvalueList = newList;
	const_cast<UserMatches&>(list).swap(lvalueList);
#else
	const_cast<UserMatches&>(list).swap(std::forward<UserMatches>(newList));
#endif

	// refresh user matches.
	auto& users = cm->getOnlineUsers();
	for(auto i = users.begin(), iend = users.end(); i != iend; ++i) {
		i->second->getClient().updated(*i->second);
	}
}

void UserMatchManager::match(Identity& identity) const {
	for(auto i = list.cbegin(), iend = list.cend(); i != iend; ++i) {
		if(i->match(identity)) {
			return;
		}
	}
	identity.setMatch(nullptr);
}

void UserMatchManager::on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
	UserMatches newList;

	xml.resetCurrentChild();
	if(xml.findChild("UserMatches")) {

		xml.stepIn();

		while(xml.findChild("UserMatch")) {
			UserMatch match;

			match.name = xml.getChildAttrib("Name");

			if(xml.getBoolChildAttrib("Favs")) { match.setFlag(UserMatch::FAVS); }
			if(xml.getBoolChildAttrib("Ops")) { match.setFlag(UserMatch::OPS); }
			if(xml.getBoolChildAttrib("Bots")) { match.setFlag(UserMatch::BOTS); }

			match.props = new UserMatchProps();
			match.props->noChat = xml.getBoolChildAttrib("NoChat");
			match.props->font = xml.getChildAttrib("Font");
			match.props->textColor = xml.getIntChildAttrib("TextColor");
			match.props->bgColor = xml.getIntChildAttrib("BgColor");

			xml.stepIn();

			while(xml.findChild("Rule")) {
				UserMatch::Rule rule;

				if(xml.getBoolChildAttrib("Regex")) { rule.setRegEx(true); }

				rule.field = static_cast<decltype(rule.field)>(xml.getIntChildAttrib("Field"));
				rule.pattern = xml.getChildData();

				match.addRule(std::move(rule));
			}

			xml.stepOut();

			newList.push_back(std::move(match));
		}

		xml.stepOut();
	}

	setList(std::move(newList));
}

void UserMatchManager::on(SettingsManagerListener::Save, SimpleXML& xml) noexcept {
	xml.addTag("UserMatches");
	xml.stepIn();
	for(auto i = list.cbegin(), iend = list.cend(); i != iend; ++i) {
		xml.addTag("UserMatch");

		xml.addChildAttrib("Name", i->name);

		if(i->isSet(UserMatch::FAVS)) { xml.addChildAttrib("Favs", true); }
		if(i->isSet(UserMatch::OPS)) { xml.addChildAttrib("Ops", true); }
		if(i->isSet(UserMatch::BOTS)) { xml.addChildAttrib("Bots", true); }

		if(i->props->noChat) { xml.addChildAttrib("NoChat", true); }
		xml.addChildAttrib("Font", i->props->font);
		xml.addChildAttrib("TextColor", i->props->textColor);
		xml.addChildAttrib("BgColor", i->props->bgColor);

		xml.stepIn();
		for(auto rule = i->rules.cbegin(), rule_end = i->rules.cend(); rule != rule_end; ++rule) {
			xml.addTag("Rule", rule->pattern);

			if(rule->isRegEx()) { xml.addChildAttrib("Regex", true); }

			xml.addChildAttrib("Field", rule->field);
		}
		xml.stepOut();
	}
	xml.stepOut();
}

} // namespace dcpp
