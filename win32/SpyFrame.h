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

#ifndef DCPLUSPLUS_WIN32_SPY_FRAME_H
#define DCPLUSPLUS_WIN32_SPY_FRAME_H

#include <dcpp/ClientManagerListener.h>

#include "StaticFrame.h"

class SpyFrame : public StaticFrame<SpyFrame>, private ClientManagerListener {
	typedef StaticFrame<SpyFrame> BaseType;
public:
	enum Status {
		STATUS_IGNORE_TTH,
		STATUS_STATUS,
		STATUS_TOTAL,
		STATUS_AVG_PER_SECOND,
		STATUS_HITS,
		STATUS_HIT_RATIO,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	friend class StaticFrame<SpyFrame>;
	friend class MDIChildFrame<SpyFrame>;

	SpyFrame(TabViewPtr parent);
	virtual ~SpyFrame();

	void layout();

	bool preClosing();
	void postClosing();

private:
	static const size_t AVG_TIME = 60;

	enum {
		COLUMN_FIRST,
		COLUMN_STRING = COLUMN_FIRST,
		COLUMN_COUNT,
		COLUMN_TIME,
		COLUMN_LAST
	};

	TablePtr searches;

	CheckBoxPtr ignoreTTH;
	bool bIgnoreTTH;

	size_t total, cur, perSecond[AVG_TIME];

	void initSecond();
	bool eachSecond();

	void handleColumnClick(int column);
	bool handleContextMenu(dwt::ScreenCoordinate pt);

	void handleSearch(const tstring& searchString);

	void handleIgnoreTTHClicked();

	void add(const tstring& x);

	// ClientManagerListener
	virtual void on(ClientManagerListener::IncomingSearch, const string& s) noexcept;
};

#endif // !defined(DCPLUSPLUS_WIN32_SPY_FRAME_H)
