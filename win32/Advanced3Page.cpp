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

#include <dwt/widgets/Spinner.h>

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

Advanced3Page::Advanced3Page(dwt::Widget* parent) :
PropPage(parent),
grid(0)
{
	createDialog(IDD_ADVANCED3PAGE);
	setHelpId(IDH_ADVANCED3PAGE);

	grid = addChild(Grid::Seed(6, 6));
	grid->column(1).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;

	grid->addChild(Label::Seed(T_("Max hash speed")))->setHelpId(IDH_SETTINGS_ADVANCED3_MAX_HASH_SPEED);
	TextBoxPtr box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::MAX_HASH_SPEED, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_MAX_HASH_SPEED);
	grid->addChild(Label::Seed(T_("MiB/s")))->setHelpId(IDH_SETTINGS_ADVANCED3_MAX_HASH_SPEED);

	grid->addChild(Label::Seed(T_("Write buffer size")))->setHelpId(IDH_SETTINGS_ADVANCED3_BUFFERSIZE);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::BUFFER_SIZE, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_BUFFERSIZE);
	grid->addChild(Label::Seed(T_("KiB")))->setHelpId(IDH_SETTINGS_ADVANCED3_BUFFERSIZE);

	grid->addChild(Label::Seed(T_("PM history")))->setHelpId(IDH_SETTINGS_ADVANCED3_PM_HISTORY);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::SHOW_LAST_LINES_LOG, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_PM_HISTORY);
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Auto-search limit")))->setHelpId(IDH_SETTINGS_ADVANCED3_AUTO_SEARCH_LIMIT);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::AUTO_SEARCH_LIMIT, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_AUTO_SEARCH_LIMIT);
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Mini slot size")))->setHelpId(IDH_SETTINGS_ADVANCED3_MINISLOT_SIZE);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::SET_MINISLOT_SIZE, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_MINISLOT_SIZE);
	grid->addChild(Label::Seed(T_("KiB")))->setHelpId(IDH_SETTINGS_ADVANCED3_MINISLOT_SIZE);

	grid->addChild(Label::Seed(T_("Search history")))->setHelpId(IDH_SETTINGS_ADVANCED3_SEARCH_HISTORY);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::SEARCH_HISTORY, PropPage::T_INT_WITH_SPIN));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_SEARCH_HISTORY);
	{
		SpinnerPtr spin = grid->addChild(Spinner::Seed(0, 100, box));
		grid->setWidget(spin);
		spin->setHelpId(IDH_SETTINGS_ADVANCED3_SEARCH_HISTORY);
	}
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Max filelist size")))->setHelpId(IDH_SETTINGS_ADVANCED3_MAX_FILELIST_SIZE);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::MAX_FILELIST_SIZE, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_MAX_FILELIST_SIZE);
	grid->addChild(Label::Seed(T_("MiB")))->setHelpId(IDH_SETTINGS_ADVANCED3_MAX_FILELIST_SIZE);

	grid->addChild(Label::Seed(T_("Bind address")))->setHelpId(IDH_SETTINGS_ADVANCED3_BIND_ADDRESS);
	box = grid->addChild(WinUtil::Seeds::Dialog::TextBox);
	items.push_back(Item(box, SettingsManager::BIND_ADDRESS, PropPage::T_STR));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_BIND_ADDRESS);
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("PID")))->setHelpId(IDH_SETTINGS_ADVANCED3_PRIVATE_ID);
	box = grid->addChild(WinUtil::Seeds::Dialog::TextBox);
	items.push_back(Item(box, SettingsManager::PRIVATE_ID, PropPage::T_STR));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_PRIVATE_ID);
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Socket read buffer")))->setHelpId(IDH_SETTINGS_ADVANCED3_SOCKET_IN_BUFFER);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::SOCKET_IN_BUFFER, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_SOCKET_IN_BUFFER);
	grid->addChild(Label::Seed(T_("B")))->setHelpId(IDH_SETTINGS_ADVANCED3_SOCKET_IN_BUFFER);

	grid->addChild(Label::Seed(T_("Auto refresh time")))->setHelpId(IDH_SETTINGS_ADVANCED3_AUTO_REFRESH_TIME);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::AUTO_REFRESH_TIME, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_AUTO_REFRESH_TIME);
	grid->addChild(Label::Seed(tstring()));

	grid->addChild(Label::Seed(T_("Socket write buffer")))->setHelpId(IDH_SETTINGS_ADVANCED3_SOCKET_OUT_BUFFER);
	box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	items.push_back(Item(box, SettingsManager::SOCKET_OUT_BUFFER, PropPage::T_INT));
	box->setHelpId(IDH_SETTINGS_ADVANCED3_SOCKET_OUT_BUFFER);
	grid->addChild(Label::Seed(T_("B")))->setHelpId(IDH_SETTINGS_ADVANCED3_SOCKET_OUT_BUFFER);

	PropPage::read(items);
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
