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

#ifndef DCPLUSPLUS_WIN32_PARAMDLG_H
#define DCPLUSPLUS_WIN32_PARAMDLG_H

#include <dcpp/Util.h>

class ParamDlg : public dwt::ModalDialog
{
public:
	ParamDlg(dwt::Widget* parent, const tstring& title);

	/// shorthand constructor that adds a text box
	ParamDlg(dwt::Widget* parent, const tstring& title, const tstring& name, const tstring& value = Util::emptyStringT, bool password = false);
	/// shorthand constructor that adds a combo box
	ParamDlg(dwt::Widget* parent, const tstring& title, const tstring& name, const TStringList& choices, size_t sel = 0);

	void addTextBox(const tstring& name, const tstring& value = Util::emptyStringT, bool password = false);
	void addIntTextBox(const tstring& name, const tstring& value, const int min = UD_MINVAL, const int max = UD_MAXVAL);
	void addComboBox(const tstring& name, const TStringList& choices, size_t sel = 0);

	int run();

	const TStringList& getValues() const { return values; }
	const tstring& getValue() const { return values[0]; }

private:
	GridPtr grid;
	GridPtr left;
	std::vector<std::tr1::function<void ()> > initFs;
	std::vector<std::tr1::function<tstring ()> > valueFs;

	TStringList values;

	void initTextBox(const tstring& name, const tstring& value, bool password);
	void initIntTextBox(const tstring& name, const tstring& value, const int min, const int max);
	void initComboBox(const tstring& name, const TStringList& choices, size_t sel);

	bool initDialog(const tstring& title);
	void okClicked();

	void layout();
};

#endif
