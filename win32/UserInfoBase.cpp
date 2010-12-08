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

#include "stdafx.h"

#include "UserInfoBase.h"

#include <dcpp/ClientManager.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/UploadManager.h>
#include <dcpp/User.h>

#include <dwt/util/StringUtils.h>

#include "PrivateFrame.h"
#include "HubFrame.h"

void UserInfoBase::matchQueue() {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());
	}
}
void UserInfoBase::getList() {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());
	}
}
void UserInfoBase::browseList() {
	if(user.user->getCID().isZero())
		return;
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());
	}
}

void UserInfoBase::addFav() {
	FavoriteManager::getInstance()->addFavoriteUser(user);
}

void UserInfoBase::pm(dwt::TabView* mdiParent) {
	PrivateFrame::openWindow(mdiParent, user, Util::emptyStringT);
}

void UserInfoBase::grant() {
	UploadManager::getInstance()->reserveSlot(user);
}

void UserInfoBase::removeFromQueue() {
	QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
}

void UserInfoBase::connectFav(dwt::TabView* mdiParent) {
	std::string url = user.hint;
	if(url.empty())
		url = FavoriteManager::getInstance()->getUserURL(user);
	if(!url.empty()) {
		HubFrame::openWindow(mdiParent, url);
	}
}

const size_t maxChars = 100; // max chars per tooltip line

tstring UserInfoBase::getTooltip(bool priv) const {
	bool hubSet = priv;
	if(!priv)
		priv = FavoriteManager::getInstance()->isPrivate(user.hint);

	tstring ret(WinUtil::getNicks(user, priv));
	dwt::util::cutStr(ret, maxChars);

	auto addLine = [&ret](tstring line) {
		dwt::util::cutStr(line, maxChars);
		ret += _T("\r\n") + line;
	};

	if(!hubSet)
		addLine(str(TF_("Hubs: %1%") % WinUtil::getHubNames(user, priv).first));

	OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(user, priv);
	if(!ou)
		return ret;
	const Identity& id = ou->getIdentity();

	auto getField = [&id](const char* code) -> tstring {
		string field = id.get(code);
		return field.empty() ? _T("?") : Text::toT(field);
	};

	if(id.isHidden())
		addLine(T_("Hidden user"));
	if(id.isBot())
		addLine(T_("Bot"));
	if(id.isOp())
		addLine(T_("Hub operator"));
	if(id.isAway())
		addLine(T_("In away mode"));

	addLine(str(TF_("Shared: %1%") % Text::toT(Util::formatBytes(id.getBytesShared()))));
	addLine(str(TF_("Description: %1%") % Text::toT(id.getDescription())));
	addLine(str(TF_("Tag: %1%") % Text::toT(id.getTag())));
	addLine(str(TF_("Connection: %1%") % Text::toT(id.getConnection())));
	addLine(str(TF_("IP: %1%") % getField("I4")));
	const string country = id.getCountry();
	if(!country.empty())
		addLine(str(TF_("Country: %1%") % Text::toT(country)));
	addLine(str(TF_("E-mail: %1%") % Text::toT(id.getEmail())));
	addLine(str(TF_("Slots: %1%/%2%") % getField("FS") % getField("SL")));

	return ret;
}

void UserInfoBase::UserTraits::parse(UserInfoBase* ui) {
	if(ui->getUser().user->isSet(User::NMDC))
		adcOnly = false;
	bool fav = FavoriteManager::getInstance()->isFavoriteUser(ui->getUser());
	if(fav)
		nonFavOnly = false;
	else
		favOnly = false;
}
