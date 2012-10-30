/*
 * Copyright (C) 2012 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef PLUGINS_DEV_DIALOG_H
#define PLUGINS_DEV_DIALOG_H

#include <unordered_set>

#include <boost/lockfree/queue.hpp>
#include <boost/regex.hpp>

using std::move;
using std::string;
using std::unordered_set;

class Dialog
{
public:
	Dialog();
	~Dialog();

	void create();
	void write(bool hubOrUser, bool sending, string ip, string peer, string message);
	void close();

	static HINSTANCE instance;

private:
	static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM);

	// events called by DialogProc
	BOOL init();
	void timer();
	void command(WPARAM wParam);

	// other helpers
	void initFilter();
	void filterSelChanged();
	void applyRegex();
	void copy();
	void clear();

	HWND hwnd;

	// store the messages to be displayed here; process them with a timer.
	struct Message { bool hubOrUser; bool sending; string ip; string peer; string message; };
	boost::lockfree::queue<Message> messages;

	uint16_t counter;
	bool scroll;
	bool hubMessages;
	bool userMessages;
	unordered_set<tstring> filter;
	tstring filterSel;
	boost::regex regex;

	// objects associated to each list litem as LPARAMs.
	struct Item {
		tstring index;
		tstring dir;
		tstring ip;
		tstring peer;
		tstring message;
	};
};

#endif
