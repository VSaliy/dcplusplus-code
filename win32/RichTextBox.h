/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_RichTextBox_H_
#define DCPLUSPLUS_WIN32_RichTextBox_H_

#include <dcpp/typedefs.h>

#include <dwt/widgets/RichTextBox.h>

#include "forward.h"

/// our rich text boxes that provide find functions
class RichTextBox : public dwt::RichTextBox {
	typedef dwt::RichTextBox BaseType;
	friend class dwt::WidgetCreator<RichTextBox>;
public:
	typedef RichTextBox ThisType;
	
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed();
	};

	explicit RichTextBox(dwt::Widget* parent);

	bool handleMessage(const MSG& msg, LRESULT& retVal);

	MenuPtr getMenu();

	tstring findTextPopup();
	void findTextNew();
	void findTextNext();

private:
	bool handleKeyDown(int c);
};

typedef RichTextBox::ObjectType RichTextBoxPtr;

#endif /*RichTextBox_H_*/
