/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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
#include "format.h"
#include "SimpleXML.h"
#include "version.h"

namespace dcpp {

UserMatchManager::UserMatchManager() {
	SettingsManager::getInstance()->addListener(this);
}

UserMatchManager::~UserMatchManager() {
	SettingsManager::getInstance()->removeListener(this);
}

const UserMatchManager::UserMatches& UserMatchManager::getList() const {
	return list;
}

void UserMatchManager::setList(UserMatches&& newList) {
	if(list.empty() && newList.empty())
		return;

	// operate within the ClientManager lock.
	auto cm = ClientManager::getInstance();
	auto lock = cm->lock();

	// assign the new list.
	const_cast<UserMatches&>(list) = move(newList);

	// refresh user matches.
	for(auto i: cm->getClients()) {
		i->updateUsers();
	}
}

void UserMatchManager::match(OnlineUser& user) const {
	auto& identity = user.getIdentity();

	bool chatSet = false;
	Style style;

	for(auto& i: list) {
		if(i.match(user)) {

			if(!chatSet && (i.isSet(UserMatch::FORCE_CHAT) || i.isSet(UserMatch::IGNORE_CHAT))) {
				identity.setNoChat(i.isSet(UserMatch::IGNORE_CHAT));
				chatSet = true;
			}

			if(style.font.empty() && !i.style.font.empty()) {
				style.font = i.style.font;
			}

			if(style.textColor < 0 && i.style.textColor >= 0) {
				style.textColor = i.style.textColor;
			}

			if(style.bgColor < 0 && i.style.bgColor >= 0) {
				style.bgColor = i.style.bgColor;
			}
		}
	}

	if(!chatSet) { identity.setNoChat(false); }
	identity.setStyle(std::move(style));
}

void UserMatchManager::ignoreChat(const HintedUser& user, bool ignore) {
	setList(ignoreChatImpl(list, user, ignore));
}

void UserMatchManager::ignoreChat(const HintedUserList& users, bool ignore) {
	auto newList = list;
	for(auto& user: users) {
		newList = ignoreChatImpl(newList, user, ignore);
	}
	setList(move(newList));
}

UserMatchManager::UserMatches UserMatchManager::ignoreChatImpl(const UserMatches& list, const HintedUser& user, bool ignore) {
	auto nick = ClientManager::getInstance()->getNicks(user)[0];

	UserMatch matcher;
	matcher.setFlag(UserMatch::GENERATED);
	matcher.setFlag(ignore ? UserMatch::IGNORE_CHAT : UserMatch::FORCE_CHAT);

	matcher.name = str(F_("%1% %2% (added by %3%)") % (ignore ? _("Ignore") : _("Un-ignore")) % nick % APPNAME);

	if(!user.user->isNMDC()) {
		// for ADC, just match the CID.
		UserMatch::Rule rule;
		rule.field = UserMatch::Rule::CID;
		rule.pattern = user.user->getCID().toBase32();
		rule.setMethod(UserMatch::Rule::EXACT);
		matcher.addRule(std::move(rule));

	} else {
		// for NMDC, match both the nick and the hub name.
		UserMatch::Rule rule;
		rule.field = UserMatch::Rule::NICK;
		rule.pattern = nick;
		rule.setMethod(UserMatch::Rule::EXACT);
		matcher.addRule(std::move(rule));

		rule = UserMatch::Rule();
		rule.field = UserMatch::Rule::HUB_ADDRESS;
		rule.pattern = user.hint;
		rule.setMethod(UserMatch::Rule::EXACT);
		matcher.addRule(std::move(rule));
	}

	auto newList = list;

	// see if an automatic matcher with these rules already exists.
	for(auto i = newList.begin(), iend = newList.end(); i != iend; ++i) {
		if(i->isSet(UserMatch::GENERATED) && i->rules == matcher.rules) {
			matcher.style = i->style;
			newList.erase(i);
			break;
		}
	}

	newList.insert(newList.begin(), std::move(matcher));
	return newList;
}

void UserMatchManager::on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
	UserMatches newList;

	xml.resetCurrentChild();
	if(xml.findChild("UserMatches")) {

		xml.stepIn();

		while(xml.findChild("UserMatch")) {
			UserMatch match;

			match.name = xml.getChildAttrib("Name");

			if(xml.getBoolChildAttrib("Predefined")) { match.setFlag(UserMatch::PREDEFINED); }
			if(xml.getBoolChildAttrib("Generated")) { match.setFlag(UserMatch::GENERATED); }
			if(xml.getBoolChildAttrib("Favs")) { match.setFlag(UserMatch::FAVS); }
			if(xml.getBoolChildAttrib("Ops")) { match.setFlag(UserMatch::OPS); }
			if(xml.getBoolChildAttrib("Bots")) { match.setFlag(UserMatch::BOTS); }
			if(xml.getBoolChildAttrib("ForceChat")) { match.setFlag(UserMatch::FORCE_CHAT); }
			if(xml.getBoolChildAttrib("IgnoreChat")) { match.setFlag(UserMatch::IGNORE_CHAT); }

			match.style.font = xml.getChildAttrib("Font");
			match.style.textColor = xml.getIntChildAttrib("TextColor");
			match.style.bgColor = xml.getIntChildAttrib("BgColor");

			xml.stepIn();

			while(xml.findChild("Rule")) {
				UserMatch::Rule rule;

				rule.field = static_cast<decltype(rule.field)>(xml.getIntChildAttrib("Field"));
				rule.pattern = xml.getChildData();
				rule.setMethod(static_cast<UserMatch::Rule::Method>(xml.getIntChildAttrib("Method")));

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
	for(auto& i: list) {
		xml.addTag("UserMatch");

		xml.addChildAttrib("Name", i.name);

		if(i.isSet(UserMatch::PREDEFINED)) { xml.addChildAttrib("Predefined", true); }
		if(i.isSet(UserMatch::GENERATED)) { xml.addChildAttrib("Generated", true); }
		if(i.isSet(UserMatch::FAVS)) { xml.addChildAttrib("Favs", true); }
		if(i.isSet(UserMatch::OPS)) { xml.addChildAttrib("Ops", true); }
		if(i.isSet(UserMatch::BOTS)) { xml.addChildAttrib("Bots", true); }
		if(i.isSet(UserMatch::FORCE_CHAT)) { xml.addChildAttrib("ForceChat", true); }
		if(i.isSet(UserMatch::IGNORE_CHAT)) { xml.addChildAttrib("IgnoreChat", true); }

		xml.addChildAttrib("Font", i.style.font);
		xml.addChildAttrib("TextColor", i.style.textColor);
		xml.addChildAttrib("BgColor", i.style.bgColor);

		xml.stepIn();
		for(auto& rule: i.rules) {
			xml.addTag("Rule", rule.pattern);
			xml.addChildAttrib("Field", rule.field);
			xml.addChildAttrib("Method", rule.getMethod());
		}
		xml.stepOut();
	}
	xml.stepOut();
}

} // namespace dcpp
