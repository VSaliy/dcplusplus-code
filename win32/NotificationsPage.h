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

#ifndef DCPLUSPLUS_WIN32_NOTIFICATIONS_PAGE_H
#define DCPLUSPLUS_WIN32_NOTIFICATIONS_PAGE_H

#include "PropPage.h"

class NotificationsPage : public PropPage
{
public:
	NotificationsPage(dwt::Widget* parent);
	virtual ~NotificationsPage();

	virtual void layout();
	virtual void write();

private:
	struct Option {
		const char* text;
		int soundSetting;
		tstring sound;
		int balloonSetting;
		bool balloon;
		unsigned helpId;
	};
	static Option options[];

	enum {
		COLUMN_DUMMY, // so that the first column doesn't have a blank space.
		COLUMN_TEXT,
		COLUMN_SOUND,
		COLUMN_BALLOON,
		COLUMN_LAST
	};

	TablePtr table;
	CheckBoxPtr sound;
	CheckBoxPtr balloon;
	GroupBoxPtr beepGroup;
	TextBoxPtr beepFile;

	int prevSelection;

	void handleSelectionChanged();
	void handleDblClicked();
	void handleTableHelp(unsigned id);
	void handleTableHelpId(unsigned& id);
	void handleSoundClicked();
	void handleBalloonClicked();
	void handlePlayClicked();
	void handleBrowseClicked();
	void handleExampleClicked();

	void update(size_t row, size_t column, bool enable);
};

#endif // !defined(DCPLUSPLUS_WIN32_NOTIFICATIONS_PAGE_H)
