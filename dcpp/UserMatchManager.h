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

#ifndef DCPLUSPLUS_DCPP_USER_MATCH_MANAGER_H
#define DCPLUSPLUS_DCPP_USER_MATCH_MANAGER_H

#include "forward.h"
#include "SettingsManager.h"
#include "UserMatch.h"

namespace dcpp {

/** This class manages user matching definitions. The list doesn't have to be locked; instead,
special care is taken when updating it. */
class UserMatchManager :
	public Singleton<UserMatchManager>,
	private SettingsManagerListener
{
	typedef std::vector<UserMatch> UserMatches;

public:
	/// Retrieve the list of user matching definitions.
	const UserMatches& getList() const;
	/// Assign a new list of user matching definitions. All current users will be re-matched.
	void setList(UserMatches&& newList);

	/** Match the given user against current user matching definitions. The "match" member of the
	user's identity object will point to the properties of the matched definition on success. */
	void match(OnlineUser& user) const;

	void ignoreChat(const HintedUser& user, bool ignore);

private:
	friend class Singleton<UserMatchManager>;

	const UserMatches list; // const to make sure only setList can change this.

	UserMatchManager();
	virtual ~UserMatchManager();

	void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept;
	void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept;
};

} // namespace dcpp

#endif
