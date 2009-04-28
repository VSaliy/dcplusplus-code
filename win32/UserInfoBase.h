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

#ifndef USERINFOBASE_H_
#define USERINFOBASE_H_

#include <dcpp/forward.h>
#include <dcpp/Text.h>
#include "resource.h"

class UserInfoBase {
public:
	UserInfoBase(const UserPtr& u) : user(u) { }

	void getList(const string& hubHint);
	void browseList(const string& hubHint);
	void matchQueue(const string& hubHint);
	void pm(dwt::TabViewPtr, const string& hubHint);
	void grant(const string& hubHint);
	void addFav();
	void removeFromQueue();
	void connectFav(dwt::TabViewPtr);

	const UserPtr& getUser() const { return user; }

	struct UserTraits {
		UserTraits() : adcOnly(true), favOnly(true), nonFavOnly(true) { }

		void parse(UserInfoBase* ui);

		bool adcOnly;
		bool favOnly;
		bool nonFavOnly;
	};

protected:
	UserPtr user;
};

template<typename T>
class AspectUserInfo {
	typedef AspectUserInfo<T> ThisType;

	T& t() { return *static_cast<T*>(this); }
	const T& t() const { return *static_cast<const T*>(this); }

protected:
	typedef std::vector<UserInfoBase*> UserInfoList;

private:
	struct UserCollector {
		void operator()(UserInfoBase* data) {
			users.push_back(data);
		}
		UserInfoList users;
	};

protected:
	template<typename TableType>
	UserInfoList usersFromTable(TableType* table) const {
		return table->forEachSelectedT(UserCollector()).users;
	}

private:
	template<typename FunctionType>
	void handleUserFunction(const FunctionType& userFunction) {
		UserInfoList users = t().selectedUsersImpl();
		for_each(users.begin(), users.end(), userFunction);
	}

protected:
	void handleMatchQueue(const string& hubHint) {
		handleUserFunction(std::tr1::bind(&UserInfoBase::matchQueue, _1, hubHint));
	}
	void handleGetList(const string& hubHint) {
		handleUserFunction(std::tr1::bind(&UserInfoBase::getList, _1, hubHint));
	}
	void handleBrowseList(const string& hubHint) {
		handleUserFunction(std::tr1::bind(&UserInfoBase::browseList, _1, hubHint));
	}
	void handleAddFavorite() {
		handleUserFunction(std::tr1::bind(&UserInfoBase::addFav, _1));
	}
	void handlePrivateMessage(dwt::TabViewPtr parent, const string& hubHint) {
		handleUserFunction(std::tr1::bind(&UserInfoBase::pm, _1, parent, hubHint));
	}
	void handleGrantSlot(const string& hubHint) {
		handleUserFunction(std::tr1::bind(&UserInfoBase::grant, _1, hubHint));
	}
	void handleRemoveFromQueue() {
		handleUserFunction(std::tr1::bind(&UserInfoBase::removeFromQueue, _1));
	}
	void handleConnectFav(dwt::TabViewPtr parent) {
		handleUserFunction(std::tr1::bind(&UserInfoBase::connectFav, _1, parent));
	}

	void appendUserItems(dwt::TabViewPtr parent, dwt::MenuPtr menu, const string& hubHint,
		bool defaultIsGetList = true, bool includeSendPM = true, bool includeGetList = true)
	{
		UserInfoList users = t().selectedUsersImpl();
		if(users.empty())
			return;

		UserInfoBase::UserTraits traits;
		for_each(users.begin(), users.end(), std::tr1::bind(&UserInfoBase::UserTraits::parse, &traits, _1));

		if(includeGetList) {
			menu->appendItem(T_("&Get file list"), std::tr1::bind(&ThisType::handleGetList, this, hubHint), dwt::IconPtr(), true, defaultIsGetList);
			if(traits.adcOnly)
				menu->appendItem(T_("&Browse file list"), std::tr1::bind(&ThisType::handleBrowseList, this, hubHint));
			menu->appendItem(T_("&Match queue"), std::tr1::bind(&ThisType::handleMatchQueue, this, hubHint));
		}
		if(includeSendPM)
			menu->appendItem(T_("&Send private message"), std::tr1::bind(&ThisType::handlePrivateMessage, this, parent, hubHint), dwt::IconPtr(), true, !defaultIsGetList);
		if(!traits.favOnly)
			menu->appendItem(T_("Add To &Favorites"), std::tr1::bind(&ThisType::handleAddFavorite, this), dwt::IconPtr(new dwt::Icon(IDR_FAVORITE_USERS)));
		menu->appendItem(T_("Grant &extra slot"), std::tr1::bind(&ThisType::handleGrantSlot, this, hubHint));
		if(!traits.nonFavOnly)
			menu->appendItem(T_("Connect to hub"), std::tr1::bind(&ThisType::handleConnectFav, this, parent), dwt::IconPtr(new dwt::Icon(IDR_HUB)));
		menu->appendSeparator();
		menu->appendItem(T_("Remove user from queue"), std::tr1::bind(&ThisType::handleRemoveFromQueue, this));
	}
};

#endif /*USERINFOBASE_H_*/
