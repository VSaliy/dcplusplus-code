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

#ifndef DCPLUSPLUS_WIN32_STRING_LIST_DLG_H
#define DCPLUSPLUS_WIN32_STRING_LIST_DLG_H

/// generic dialog to manage a list of strings, that can be used as is or specialized.
class StringListDlg : public dwt::ModalDialog
{
public:
	StringListDlg(dwt::Widget* parent, const TStringList& initialValues);
	virtual ~StringListDlg();

	int run();

	const TStringList& getValues() const { return values; }

protected:
	enum HelpFields {
		HELP_DIALOG,
		HELP_EDIT_BOX,
		HELP_LIST,
		HELP_ADD,
		HELP_MOVE_UP,
		HELP_MOVE_DOWN,
		HELP_EDIT,
		HELP_REMOVE
	};

	void insert(const tstring& line, int index = -1);
	void modify(unsigned row, const tstring& value);

private:
	virtual tstring getTitle() const;
	virtual tstring getEditTitle() const;
	virtual tstring getEditDescription() const;
	virtual unsigned getHelpId(HelpFields field) const;
	virtual void add(const tstring& s);
	virtual void edit(unsigned row, const tstring& s);

	GridPtr grid;
	TextBoxPtr editBox;
	TablePtr list;

	TStringList values;

	bool handleInitDialog(const TStringList& initialValues);
	void handleDoubleClick();
	bool handleKeyDown(int c);
	void handleAddClicked();
	void handleMoveUpClicked();
	void handleMoveDownClicked();
	void handleEditClicked();
	void handleRemoveClicked();
	void handleOKClicked();

	void layout();
};

#endif // !defined(DCPLUSPLUS_WIN32_STRING_LIST_DLG_H)
