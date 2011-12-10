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
#include "format.h"
#include "ScopedFunctor.h"
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
	const_cast<UserMatches&>(list) = std::forward<UserMatches>(newList);

	// refresh user matches.
	auto& users = cm->getOnlineUsers();
	for(auto i = users.begin(), iend = users.end(); i != iend; ++i) {
		i->second->getClient().updated(*i->second);
	}
}

void UserMatchManager::match(OnlineUser& user) const {
	for(auto i = list.cbegin(), iend = list.cend(); i != iend; ++i) {
		if(i->match(user)) {
			user.getIdentity().setMatch(i->props);
			return;
		}
	}
	user.getIdentity().setMatch(nullptr);
}

void UserMatchManager::ignoreChat(const HintedUser& user, bool ignore) {
	auto nick = ClientManager::getInstance()->getNicks(user)[0];

	UserMatch matcher;
	matcher.setFlag(UserMatch::GENERATED);

	matcher.name = str(F_("Match %1% (added by %2%)") % nick % APPNAME);

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

	ScopedFunctor(([this, &newList, &matcher] {
		newList.insert(newList.begin(), std::move(matcher));
		setList(std::move(newList));
	}));

	// first, see if an automatic matcher with these rules already exists.
	for(auto i = newList.begin(), iend = newList.end(); i != iend; ++i) {
		if(i->isSet(UserMatch::GENERATED) && i->rules == matcher.rules) {
			matcher.props = i->props;
			matcher.props->noChat = ignore;
			newList.erase(i);
			return;
		}
	}

	matcher.props = new UserMatchProps();
	matcher.props->noChat = ignore;
	matcher.props->textColor = -1;
	matcher.props->bgColor = -1;
}

void UserMatchManager::on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
	UserMatches newList;

	xml.resetCurrentChild();
	if(xml.findChild("UserMatches")) {

		xml.stepIn();

		while(xml.findChild("UserMatch")) {
			UserMatch match;

			match.name = xml.getChildAttrib("Name");

			if(xml.getBoolChildAttrib("Generated")) { match.setFlag(UserMatch::GENERATED); }
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
	for(auto i = list.cbegin(), iend = list.cend(); i != iend; ++i) {
		xml.addTag("UserMatch");

		xml.addChildAttrib("Name", i->name);

		if(i->isSet(UserMatch::GENERATED)) { xml.addChildAttrib("Generated", true); }
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
			xml.addChildAttrib("Field", rule->field);
			xml.addChildAttrib("Method", rule->getMethod());
		}
		xml.stepOut();
	}
	xml.stepOut();
}

} // namespace dcpp
