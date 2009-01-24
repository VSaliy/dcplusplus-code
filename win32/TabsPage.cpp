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

#include "TabsPage.h"

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_SETTINGS_MAX_TAB_CHARS, IDH_SETTINGS_MAX_TAB_CHARS },
	{ IDC_MAX_TAB_CHARS, IDH_SETTINGS_MAX_TAB_CHARS },
	{ IDC_MAX_TAB_CHARS_SPIN, IDH_SETTINGS_MAX_TAB_CHARS },
	{ 0, 0 }
};
*/

PropPage::ListItem TabsPage::listItems[] = {
	{ SettingsManager::BOLD_HUB, N_("Hub"), IDH_SETTINGS_TABS_BOLD_HUB },
	{ SettingsManager::BOLD_PM, N_("Private message"), IDH_SETTINGS_TABS_BOLD_PM },
	{ SettingsManager::BOLD_SEARCH, N_("Search"), IDH_SETTINGS_TABS_BOLD_SEARCH },
	{ SettingsManager::BOLD_SEARCH_SPY, N_("Search Spy"), IDH_SETTINGS_TABS_BOLD_SEARCH_SPY },
	{ SettingsManager::BOLD_SYSTEM_LOG, N_("System Log"), IDH_SETTINGS_TABS_BOLD_SYSTEM_LOG },
	{ SettingsManager::BOLD_QUEUE, N_("Download Queue"), IDH_SETTINGS_TABS_BOLD_QUEUE },
	{ SettingsManager::BOLD_FINISHED_DOWNLOADS, N_("Finished Downloads"), IDH_SETTINGS_TABS_BOLD_FINISHED_DOWNLOADS },
	{ SettingsManager::BOLD_WAITING_USERS, N_("Waiting Users"), IDH_SETTINGS_TABS_BOLD_WAITING_USERS },
	{ SettingsManager::BOLD_FINISHED_UPLOADS, N_("Finished Uploads"), IDH_SETTINGS_TABS_BOLD_FINISHED_UPLOADS },
	{ 0, 0 }
};

TabsPage::TabsPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
options(0)
{
	createDialog(IDD_TABSPAGE);
	setHelpId(IDH_TABSPAGE);

	grid = addChild(Grid::Seed(2, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Tab highlight on content change")));
		grid->setWidget(group, 0, 0, 1, 2);
		options = group->addChild(WinUtil::Seeds::Dialog::optionsTable);
	}

	grid->addChild(Label::Seed(T_("Max characters per tab (0 = infinite)")));
	items.push_back(Item(grid->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::MAX_TAB_CHARS, PropPage::T_INT));

	PropPage::read(items);
	PropPage::read(listItems, options);

}

TabsPage::~TabsPage() {
}

void TabsPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientAreaSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void TabsPage::write() {
	PropPage::write(items);
	PropPage::write(listItems, options);
}
