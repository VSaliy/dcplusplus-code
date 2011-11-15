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

#ifndef DCPLUSPLUS_WIN32_STYLES_PAGE_H
#define DCPLUSPLUS_WIN32_STYLES_PAGE_H

#include "PropPage.h"

class StylesPage : public PropPage
{
public:
	StylesPage(dwt::Widget* parent);
	virtual ~StylesPage();

	virtual void layout();
	virtual void write();

private:
	enum {
		COLUMN_TEXT,
		COLUMN_LAST
	};

	class Data {
	public:
		Data(tstring&& text, unsigned helpId, int fontSetting, int textColorSetting, int bgColorSetting);

		const tstring& getText(int) const;
		int getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const;

		COLORREF getTextColor() const;
		COLORREF getBgColor() const;

		void updateFont();

		void write();

		const tstring text;
		unsigned helpId;

		int fontSetting;
		int textColorSetting;
		int bgColorSetting;

		bool customFont;
		dwt::FontPtr font;
		LOGFONT logFont;

		bool customTextColor;
		COLORREF textColor;

		bool customBgColor;
		COLORREF bgColor;
	};

	Data* globalData;

	typedef TypedTable<Data> Table;
	Table* table;

	LabelPtr preview;

	CheckBoxPtr customFont;
	ButtonPtr font;

	CheckBoxPtr customTextColor;
	ButtonPtr textColor;

	CheckBoxPtr customBgColor;
	ButtonPtr bgColor;

	void handleSelectionChanged();
	void handleTableHelp(unsigned id);
	void handleTableHelpId(unsigned& id);

	void handleCustomFont();
	void handleFont();

	void handleCustomTextColor();
	void handleTextColor();

	void handleCustomBgColor();
	void handleBgColor();

	void colorDialog(COLORREF& color);
	void update(Data* const data);
	void updatePreview(Data* const data);
};

#endif
