/*
* Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

class ShellMenu : public dwt::Menu
{
	typedef dwt::Menu BaseType;

	struct Dispatcher : dwt::Dispatchers::Base<bool (const MSG&)> {
		typedef dwt::Dispatchers::Base<bool (const MSG&)> BaseType;
		Dispatcher(const BaseType::F& f_, LPCONTEXTMENU3 handler_) : BaseType(f_), handler(handler_) { }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			if(f(msg)) {
				handler->HandleMenuMsg2(msg.message, msg.wParam, msg.lParam, &ret);
				return true;
			}
			return false;
		}

	private:
		LPCONTEXTMENU3 handler;
	};

public:
	typedef ShellMenu ThisType;

	typedef std::tr1::shared_ptr<ShellMenu> ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed();
	};

	explicit ShellMenu( dwt::Widget * parent );
	~ShellMenu();

	void appendShellMenu(const wstring& path);
	void open(const dwt::ScreenCoordinate& pt, unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON);

private:
	dwt::MenuPtr menu;
	LPCONTEXTMENU3 handler;
	unsigned sel_id;

	bool handleDrawItem(const MSG& msg);
	bool handleMeasureItem(const MSG& msg);
	bool handleMenuChar();
	bool handleInitMenuPopup(const MSG& msg);
	bool handleUnInitMenuPopup(const MSG& msg);
	bool handleMenuSelect(const MSG& msg);
};

#endif // !defined(DCPLUSPLUS_WIN32_SHELL_MENU_H)
