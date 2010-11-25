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

#include "stdafx.h"

#include "resource.h"

#include "Advanced3Page.h"

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

Advanced3Page::Advanced3Page(dwt::Widget* parent) :
PropPage(parent),
grid(0)
{
	setHelpId(IDH_ADVANCED3PAGE);

	grid = addChild(Grid::Seed(6, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->setSpacing(10);

	addItem(T_("Max hash speed"), SettingsManager::MAX_HASH_SPEED, true, IDH_SETTINGS_ADVANCED3_MAX_HASH_SPEED, T_("MiB/s"));
	addItem(T_("Write buffer size"), SettingsManager::BUFFER_SIZE, true, IDH_SETTINGS_ADVANCED3_BUFFERSIZE, T_("KiB"));
	addItem(T_("Auto-search limit"), SettingsManager::AUTO_SEARCH_LIMIT, true, IDH_SETTINGS_ADVANCED3_AUTO_SEARCH_LIMIT);
	addItem(T_("Mini slot size"), SettingsManager::SET_MINISLOT_SIZE, true, IDH_SETTINGS_ADVANCED3_MINISLOT_SIZE, T_("KiB"));
	addItem(T_("Max filelist size"), SettingsManager::MAX_FILELIST_SIZE, true, IDH_SETTINGS_ADVANCED3_MAX_FILELIST_SIZE, T_("MiB"));
	addItem(T_("Bind address"), SettingsManager::BIND_ADDRESS, false, IDH_SETTINGS_ADVANCED3_BIND_ADDRESS);
	addItem(T_("PID"), SettingsManager::PRIVATE_ID, false, IDH_SETTINGS_ADVANCED3_PRIVATE_ID);
	addItem(T_("Auto refresh time"), SettingsManager::AUTO_REFRESH_TIME, true, IDH_SETTINGS_ADVANCED3_AUTO_REFRESH_TIME, T_("minutes"));
	addItem(T_("Settings save interval"), SettingsManager::SETTINGS_SAVE_INTERVAL, true, IDH_SETTINGS_ADVANCED3_SETTINGS_SAVE_INTERVAL, T_("minutes"));
	addItem(T_("Socket read buffer"), SettingsManager::SOCKET_IN_BUFFER, true, IDH_SETTINGS_ADVANCED3_SOCKET_IN_BUFFER, T_("B"));
	addItem(T_("Socket write buffer"), SettingsManager::SOCKET_OUT_BUFFER, true, IDH_SETTINGS_ADVANCED3_SOCKET_OUT_BUFFER, T_("B"));

	PropPage::read(items);
}

Advanced3Page::~Advanced3Page() {
}

void Advanced3Page::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void Advanced3Page::write() {
	PropPage::write(items);

	SettingsManager* settings = SettingsManager::getInstance();
	if(SETTING(SET_MINISLOT_SIZE) < 64)
		settings->set(SettingsManager::SET_MINISLOT_SIZE, 64);
	if(SETTING(AUTO_SEARCH_LIMIT) > 5)
		settings->set(SettingsManager::AUTO_SEARCH_LIMIT, 5);
	else if(SETTING(AUTO_SEARCH_LIMIT) < 1)
		settings->set(SettingsManager::AUTO_SEARCH_LIMIT, 1);
}

void Advanced3Page::addItem(const tstring& text, int setting, bool isInt, unsigned helpId, const tstring& text2) {
	GroupBoxPtr group = grid->addChild(GroupBox::Seed(text));
	group->setHelpId(helpId);

	GridPtr cur = group->addChild(Grid::Seed(1, 2));
	cur->column(0).mode = GridInfo::FILL;

	TextBoxPtr box = cur->addChild(isInt ? WinUtil::Seeds::Dialog::intTextBox : WinUtil::Seeds::Dialog::textBox);
	items.push_back(Item(box, setting, isInt ? PropPage::T_INT : PropPage::T_STR));

	if(text2.empty())
		cur->setWidget(box, 0, 0, 1, 2);
	else
		cur->addChild(Label::Seed(text2));
}
