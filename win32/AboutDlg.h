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

#ifndef DCPLUSPLUS_WIN32_ABOUT_DLG_H
#define DCPLUSPLUS_WIN32_ABOUT_DLG_H

#include <dcpp/HttpConnection.h>

class AboutDlg :
	public dwt::ModalDialog,
	private HttpConnectionListener
{
public:
	AboutDlg(dwt::Widget* parent);
	virtual ~AboutDlg();

	int run();

private:
	GridPtr grid;
	LabelPtr version;

	HttpConnection c;
	string downBuf;

	bool handleInitDialog();

	void layout();

	virtual void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw();
	virtual void on(HttpConnectionListener::Complete, HttpConnection* conn, const string&, bool) throw();
	virtual void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw();
};

#endif // !defined(DCPLUSPLUS_WIN32_ABOUT_DLG_H)
