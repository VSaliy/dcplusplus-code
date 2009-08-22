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

// Based on <http://www.codeproject.com/shell/shellcontextmenu.asp> by R. Engels.

#ifndef DCPLUSPLUS_WIN32_SHELL_MENU_H
#define DCPLUSPLUS_WIN32_SHELL_MENU_H

#include <dwt/widgets/Menu.h>

class ShellMenu : public Menu
{
	typedef Menu BaseType;

	struct Dispatcher : dwt::Dispatchers::Base<bool (const MSG&, LRESULT& ret)> {
		typedef dwt::Dispatchers::Base<bool (const MSG&, LRESULT& ret)> BaseType;
		Dispatcher(const BaseType::F& f_) : BaseType(f_) { }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			return f(msg, ret);
		}
	};

public:
	typedef ShellMenu ThisType;

	typedef std::tr1::shared_ptr<ShellMenu> ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed();
	};

	explicit ShellMenu(dwt::Widget* parent);
	~ShellMenu();

	void appendShellMenu(const StringList& paths);
	void open(const dwt::ScreenCoordinate& pt, unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON);

private:
	typedef std::pair<MenuPtr, LPCONTEXTMENU3> handlers_pair;
	typedef std::vector<handlers_pair> handlers_type;
	handlers_type handlers;

	LPCONTEXTMENU3 handler;
	unsigned sel_id;

	typedef std::pair<dwt::Message, dwt::Widget::CallbackIter> callbacks_pair;
	typedef std::vector<callbacks_pair> callbacks_type;
	callbacks_type callbacks;

	bool handleDrawItem(const MSG& msg, LRESULT& ret);
	bool handleMeasureItem(const MSG& msg, LRESULT& ret);
	bool handleInitMenuPopup(const MSG& msg, LRESULT& ret);
	bool handleUnInitMenuPopup(const MSG& msg, LRESULT& ret);
	bool handleMenuSelect(const MSG& msg);

	bool dispatch(const MSG& msg, LRESULT& ret);
};

#endif // !defined(DCPLUSPLUS_WIN32_SHELL_MENU_H)
