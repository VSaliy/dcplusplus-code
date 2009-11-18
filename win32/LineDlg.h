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

#ifndef DCPLUSPLUS_WIN32_LINE_DLG_H
#define DCPLUSPLUS_WIN32_LINE_DLG_H

#include <dcpp/Util.h>

class LineDlg : public dwt::ModalDialog
{
public:
	LineDlg(dwt::Widget* parent, const tstring& title, const tstring& desc, const tstring& text_ = Util::emptyStringT, bool password = false);

	int run();

	const tstring& getLine() const { return text; }

private:
	GridPtr grid;
	TextBoxPtr line;

	tstring text;

	bool initDialog(const tstring& title, const tstring& desc, bool password);
	void okClicked();

	void layout();
};

#endif // !defined(LINE_DLG_H)
