/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#include <dcpp/forward.h>

#include <dwt/widgets/ModalDialog.h>

#include "forward.h"

using std::unique_ptr;

class AboutDlg : public dwt::ModalDialog
{
public:
	AboutDlg(dwt::Widget* parent);
	virtual ~AboutDlg();

	int run();

private:
	GridPtr grid;
	LabelPtr version;

	unique_ptr<HttpDownload> c;

	bool handleInitDialog();

	void layout();

	void completeDownload(bool success, const string& result);
};

#endif // !defined(DCPLUSPLUS_WIN32_ABOUT_DLG_H)
