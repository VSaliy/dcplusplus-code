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

#include "stdafx.h"

#include "resource.h"

#include "Advanced3Page.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Spinner.h>

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_SETTINGS_MAX_HASH_SPEED, IDH_SETTINGS_ADVANCED3_MAX_HASH_SPEED },
	{ IDC_MAX_HASH_SPEED, IDH_SETTINGS_ADVANCED3_MAX_HASH_SPEED },
	{ IDC_SETTINGS_MBS, IDH_SETTINGS_ADVANCED3_MAX_HASH_SPEED },
	{ IDC_SETTINGS_PM_HISTORY, IDH_SETTINGS_ADVANCED3_PM_HISTORY },
	{ IDC_SHOW_LAST_LINES_LOG, IDH_SETTINGS_ADVANCED3_PM_HISTORY },
	{ IDC_SETTINGS_TEXT_MINISLOT, IDH_SETTINGS_ADVANCED3_MINISLOT_SIZE },
	{ IDC_SET_MINISLOT_SIZE, IDH_SETTINGS_ADVANCED3_MINISLOT_SIZE },
	{ IDC_SETTINGS_KB2, IDH_SETTINGS_ADVANCED3_MINISLOT_SIZE },
	{ IDC_SETTINGS_MAX_FILELIST_SIZE, IDH_SETTINGS_ADVANCED3_MAX_FILELIST_SIZE },
	{ IDC_MAX_FILELIST_SIZE, IDH_SETTINGS_ADVANCED3_MAX_FILELIST_SIZE },
	{ IDC_SETTINGS_MB, IDH_SETTINGS_ADVANCED3_MAX_FILELIST_SIZE },
	{ IDC_SETTINGS_PID, IDH_SETTINGS_ADVANCED3_PRIVATE_ID },
	{ IDC_PRIVATE_ID, IDH_SETTINGS_ADVANCED3_PRIVATE_ID },
	{ IDC_SETTINGS_AUTO_REFRESH_TIME, IDH_SETTINGS_ADVANCED3_AUTO_REFRESH_TIME },
	{ IDC_AUTO_REFRESH_TIME, IDH_SETTINGS_ADVANCED3_AUTO_REFRESH_TIME },
	{ IDC_SETTINGS_WRITE_BUFFER, IDH_SETTINGS_ADVANCED3_BUFFERSIZE },
	{ IDC_BUFFERSIZE, IDH_SETTINGS_ADVANCED3_BUFFERSIZE },
	{ IDC_SETTINGS_KB, IDH_SETTINGS_ADVANCED3_BUFFERSIZE },
	{ IDC_SETTINGS_AUTO_SEARCH_LIMIT, IDH_SETTINGS_ADVANCED3_AUTO_SEARCH_LIMIT },
	{ IDC_AUTO_SEARCH_LIMIT, IDH_SETTINGS_ADVANCED3_AUTO_SEARCH_LIMIT },
	{ IDC_SETTINGS_SEARCH_HISTORY, IDH_SETTINGS_ADVANCED3_SEARCH_HISTORY },
	{ IDC_SEARCH_HISTORY, IDH_SETTINGS_ADVANCED3_SEARCH_HISTORY },
	{ IDC_SEARCH_HISTORY_SPIN, IDH_SETTINGS_ADVANCED3_SEARCH_HISTORY },
	{ IDC_SETTINGS_BIND_ADDRESS, IDH_SETTINGS_ADVANCED3_BIND_ADDRESS },
	{ IDC_BIND_ADDRESS, IDH_SETTINGS_ADVANCED3_BIND_ADDRESS },
	{ IDC_SETTINGS_SOCKET_IN_BUFFER, IDH_SETTINGS_ADVANCED3_SOCKET_IN_BUFFER },
	{ IDC_SOCKET_IN_BUFFER, IDH_SETTINGS_ADVANCED3_SOCKET_IN_BUFFER },
	{ IDC_SETTINGS_B2, IDH_SETTINGS_ADVANCED3_SOCKET_IN_BUFFER },
	{ IDC_SETTINGS_SOCKET_OUT_BUFFER, IDH_SETTINGS_ADVANCED3_SOCKET_OUT_BUFFER },
	{ IDC_SOCKET_OUT_BUFFER, IDH_SETTINGS_ADVANCED3_SOCKET_OUT_BUFFER },
	{ IDC_SETTINGS_B3, IDH_SETTINGS_ADVANCED3_SOCKET_OUT_BUFFER },
	{ 0, 0 }
};
*/

Advanced3Page::Advanced3Page(dwt::Widget* parent) : PropPage(parent) {
	createDialog(IDD_ADVANCED3PAGE);
	setHelpId(IDH_ADVANCED3PAGE);

	grid = addChild(Grid::Seed(6, 6));
	grid->column(1).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;

	grid->addChild(Label::Seed(T_("Max hash speed")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::MAX_HASH_SPEED, PropPage::T_INT));
	grid->addChild(Label::Seed(T_("MiB/s")));

	grid->addChild(Label::Seed(T_("Write buffer size")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::BUFFER_SIZE, PropPage::T_INT));
	grid->addChild(Label::Seed(T_("KiB")));

	grid->addChild(Label::Seed(T_("PM history")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::SHOW_LAST_LINES_LOG, PropPage::T_INT));
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Auto-search limit")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::AUTO_SEARCH_LIMIT, PropPage::T_INT));
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Mini slot size")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::SET_MINISLOT_SIZE, PropPage::T_INT));
	grid->addChild(Label::Seed(T_("KiB")));

	grid->addChild(Label::Seed(T_("Search history")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::SEARCH_HISTORY, PropPage::T_INT_WITH_SPIN));
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Max filelist size")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::MAX_FILELIST_SIZE, PropPage::T_INT));
	grid->addChild(Label::Seed(T_("MiB")));

	grid->addChild(Label::Seed(T_("Bind address")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::BIND_ADDRESS, PropPage::T_STR));
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("PID")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::PRIVATE_ID, PropPage::T_STR));
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Socket read buffer")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::SOCKET_IN_BUFFER, PropPage::T_INT));
	grid->addChild(Label::Seed(T_("B")));

	grid->addChild(Label::Seed(T_("Auto refresh time")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::AUTO_REFRESH_TIME, PropPage::T_INT));
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Socket write buffer")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::SOCKET_OUT_BUFFER, PropPage::T_INT));
	grid->addChild(Label::Seed(T_("B")));

	PropPage::read(items);

	/** @todo PropPage could add these automagically when T_INT_WITH_SPIN?
	SpinnerPtr spinner = attachChild<Spinner>(IDC_SEARCH_HISTORY_SPIN);
	spinner->setRange(0, 100);
	*/
}

Advanced3Page::~Advanced3Page() {
}

void Advanced3Page::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientAreaSize();
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
