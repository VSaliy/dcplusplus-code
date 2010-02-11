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

#ifndef DCPLUSPLUS_WIN32_ComboBox_H_
#define DCPLUSPLUS_WIN32_ComboBox_H_

/// @todo this should obviously be moved somewhere else
/** Wraps the drop-down control of a ComboBox */
class ListBox :
	public dwt::Control,
	public dwt::AspectColor<ListBox>,
	public dwt::AspectColorCtlImpl<ListBox>
{
	typedef dwt::Control BaseType;
	friend class dwt::WidgetCreator<ListBox>;
public:
	typedef ListBox ThisType;
	typedef ThisType* ObjectType;
	struct Seed : BaseType::Seed { typedef ThisType WidgetType; };
	explicit ListBox(dwt::Widget* parent) : BaseType(parent, dwt::ChainingDispatcher::superClass<ListBox>()) { }
private:
	friend class dwt::ChainingDispatcher;
	static const TCHAR windowClass[];
};
typedef ListBox::ObjectType ListBoxPtr;

class ComboBox : public dwt::ComboBox {
	typedef dwt::ComboBox BaseType;
	friend class dwt::WidgetCreator<ComboBox>;
public:
	typedef ComboBox ThisType;

	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed();
	};

	explicit ComboBox(dwt::Widget* parent);

	ListBoxPtr getListBox();
	TextBoxPtr getTextBox();

private:
	ListBoxPtr listBox;
	TextBoxPtr textBox;
};

typedef ComboBox::ObjectType ComboBoxPtr;

#endif /*ComboBox_H_*/
