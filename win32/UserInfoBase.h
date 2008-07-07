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

#ifndef USERINFOBASE_H_
#define USERINFOBASE_H_

#include <dcpp/forward.h>
#include <dcpp/Text.h>
#include "resource.h"

class UserInfoBase {
public:
	UserInfoBase(const UserPtr& u) : user(u) { }

	void getList();
	void browseList();
	void matchQueue();
	void pm(dwt::TabViewPtr);
	void grant();
	void addFav();
	void removeFromQueue();
	void connectFav(dwt::TabViewPtr);

	UserPtr& getUser() { return user; }
	UserPtr user;

	struct UserTraits {
		UserTraits() : adcOnly(true), favOnly(true), nonFavOnly(true) { }
		void operator()(UserInfoBase* ui);

		bool adcOnly;
		bool favOnly;
		bool nonFavOnly;
	};

};

template<class T>
class AspectUserInfo {
public:
	typedef AspectUserInfo<T> ThisType;

	void handleMatchQueue() {
		static_cast<T*>(this)->getUserList()->forEachSelected(&UserInfoBase::matchQueue);
	}
	void handleGetList() {
		static_cast<T*>(this)->getUserList()->forEachSelected(&UserInfoBase::getList);
	}
	void handleBrowseList() {
		static_cast<T*>(this)->getUserList()->forEachSelected(&UserInfoBase::browseList);
	}
	void handleAddFavorite() {
		static_cast<T*>(this)->getUserList()->forEachSelected(&UserInfoBase::addFav);
	}
	// std::tr1::bind(&UserInfoBase::connectFav, _1, parent) doesn't seem to work with g++ svn 2007-07-30...
	// wonder if it's me or the implementation as boost::bind/function swallows it...
	struct Caller {
		Caller(dwt::TabViewPtr parent_, void (UserInfoBase::*f_)(dwt::TabViewPtr)) : parent(parent_), f(f_) { }
		void operator()(UserInfoBase* uib) { (uib->*f)(parent); }
		dwt::TabViewPtr parent;
		void (UserInfoBase::*f)(dwt::TabViewPtr);
	};

	void handlePrivateMessage(dwt::TabViewPtr parent) {
		static_cast<T*>(this)->getUserList()->forEachSelectedT(Caller(parent, &UserInfoBase::pm));
	}
	void handleGrantSlot() {
		static_cast<T*>(this)->getUserList()->forEachSelected(&UserInfoBase::grant);
	}
	void handleRemoveFromQueue() {
		static_cast<T*>(this)->getUserList()->forEachSelected(&UserInfoBase::removeFromQueue);
	}
	void handleConnectFav(dwt::TabViewPtr parent) {
		static_cast<T*>(this)->getUserList()->forEachSelectedT(Caller(parent, &UserInfoBase::connectFav));
	}

	void appendUserItems(dwt::TabViewPtr parent, dwt::MenuPtr menu, bool defaultIsGetList = true) {
		T* This = static_cast<T*>(this);
		UserInfoBase::UserTraits traits = This->getUserList()->forEachSelectedT(UserInfoBase::UserTraits());
		menu->appendItem(T_("&Get file list"), std::tr1::bind(&T::handleGetList, This), dwt::BitmapPtr(), true, defaultIsGetList);
		if(traits.adcOnly)
			menu->appendItem(T_("&Browse file list"), std::tr1::bind(&T::handleBrowseList, This));
		menu->appendItem(T_("&Match queue"), std::tr1::bind(&T::handleMatchQueue, This));
		menu->appendItem(T_("&Send private message"), std::tr1::bind(&T::handlePrivateMessage, This, parent), dwt::BitmapPtr(), true, !defaultIsGetList);
		if(!traits.favOnly)
			menu->appendItem(T_("Add To &Favorites"), std::tr1::bind(&T::handleAddFavorite, This), dwt::BitmapPtr(new dwt::Bitmap(IDB_FAVORITE_USERS)));
		menu->appendItem(T_("Grant &extra slot"), std::tr1::bind(&T::handleGrantSlot, This));
		if(!traits.nonFavOnly)
			menu->appendItem(T_("Connect to hub"), std::tr1::bind(&T::handleConnectFav, This, parent), dwt::BitmapPtr(new dwt::Bitmap(IDB_HUB)));
		menu->appendSeparator();
		menu->appendItem(T_("Remove user from queue"), std::tr1::bind(&T::handleRemoveFromQueue, This));
	}
};

#endif /*USERINFOBASE_H_*/
