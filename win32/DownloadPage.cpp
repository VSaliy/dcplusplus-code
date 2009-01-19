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

#include "DownloadPage.h"

#include <dwt/widgets/Spinner.h>

#include <dcpp/SettingsManager.h>
#include "HubListsDlg.h"
#include "WinUtil.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_SETTINGS_DOWNLOAD_DIRECTORY, IDH_SETTINGS_DOWNLOAD_DOWNLOADDIR },
	{ IDC_DOWNLOADDIR, IDH_SETTINGS_DOWNLOAD_DOWNLOADDIR },
	{ IDC_BROWSEDIR, IDH_SETTINGS_DOWNLOAD_DOWNLOADDIR },
	{ IDC_SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY, IDH_SETTINGS_DOWNLOAD_TEMP_DOWNLOAD_DIRECTORY },
	{ IDC_TEMP_DOWNLOAD_DIRECTORY, IDH_SETTINGS_DOWNLOAD_TEMP_DOWNLOAD_DIRECTORY },
	{ IDC_BROWSETEMPDIR, IDH_SETTINGS_DOWNLOAD_TEMP_DOWNLOAD_DIRECTORY },
	{ IDC_SETTINGS_DOWNLOAD_LIMITS, IDH_SETTINGS_DOWNLOAD_LIMITS },
	{ IDC_DOWNLOADS, IDH_SETTINGS_DOWNLOAD_DOWNLOADS },
	{ IDC_SLOTSSPIN, IDH_SETTINGS_DOWNLOAD_DOWNLOADS },
	{ IDC_SETTINGS_DOWNLOADS_MAX, IDH_SETTINGS_DOWNLOAD_DOWNLOADS },
	{ IDC_MAXSPEED, IDH_SETTINGS_DOWNLOAD_MAXSPEED },
	{ IDC_SPEEDSPIN, IDH_SETTINGS_DOWNLOAD_MAXSPEED },
	{ IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE, IDH_SETTINGS_DOWNLOAD_MAXSPEED },
	{ IDC_SETTINGS_SPEEDS_NOT_ACCURATE, IDH_SETTINGS_DOWNLOAD_LIMITS },
	{ IDC_SETTINGS_PUBLIC_HUB_LIST_HTTP_PROXY, IDH_SETTINGS_DOWNLOAD_PROXY },
	{ IDC_PROXY, IDH_SETTINGS_DOWNLOAD_PROXY },
	{ 0, 0 }
};
*/

DownloadPage::DownloadPage(dwt::Widget* parent) :
PropPage(parent),
grid(0)
{
	createDialog(IDD_DOWNLOADPAGE);
	setHelpId(IDH_DOWNLOADPAGE);

	grid = addChild(Grid::Seed(3, 1));
	grid->column(0).mode = GridInfo::FILL;

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Directories")))->addChild(Grid::Seed(4, 2));
		cur->column(0).mode = GridInfo::FILL;

		cur->setWidget(cur->addChild(Label::Seed(T_("Default download directory"))), 0, 0, 1, 2);
		items.push_back(Item(cur->addChild(WinUtil::Seeds::Dialog::TextBox), SettingsManager::DOWNLOAD_DIRECTORY, PropPage::T_STR));
		cur->addChild(Button::Seed(T_("Browse...")))->onClicked(std::tr1::bind(&DownloadPage::handleBrowseDir, this, items.back()));

		cur->setWidget(cur->addChild(Label::Seed(T_("Unfinished downloads directory"))), 2, 0, 1, 2);
		items.push_back(Item(cur->addChild(WinUtil::Seeds::Dialog::TextBox), SettingsManager::TEMP_DOWNLOAD_DIRECTORY, PropPage::T_STR));
		cur->addChild(Button::Seed(T_("Browse...")))->onClicked(std::tr1::bind(&DownloadPage::handleBrowseDir, this, items.back()));
	}

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Limits")))->addChild(Grid::Seed(3, 2));
		cur->column(0).mode = GridInfo::FILL;

		items.push_back(Item(cur->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::DOWNLOAD_SLOTS, PropPage::T_INT_WITH_SPIN));
		cur->addChild(Label::Seed(T_("Maximum simultaneous downloads (0 = infinite)")));

		items.push_back(Item(cur->addChild(WinUtil::Seeds::Dialog::intTextBox), SettingsManager::MAX_DOWNLOAD_SPEED, PropPage::T_INT_WITH_SPIN));
		cur->addChild(Label::Seed(T_("No new downloads if speed exceeds (KiB/s, 0 = disable)")));

		// xgettext:no-c-format
		cur->setWidget(cur->addChild(Label::Seed(T_("Note; because of changing download speeds, this is not 100% accurate..."))), 2, 0, 1, 2);
	}

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Public Hubs list")))->addChild(Grid::Seed(4, 1));

		cur->addChild(Label::Seed(T_("Public Hubs list URL")));
		cur->addChild(Button::Seed(T_("Configure Public Hub Lists")))->onClicked(std::tr1::bind(&DownloadPage::handleConfigHubLists, this));
		cur->addChild(Label::Seed(T_("HTTP Proxy (for hublist only)")));
		items.push_back(Item(cur->addChild(WinUtil::Seeds::Dialog::TextBox), SettingsManager::HTTP_PROXY, PropPage::T_STR));
	}

	PropPage::read(items);

	/** @todo PropPage could add these automagically when T_INT_WITH_SPIN?

	SpinnerPtr spinner = attachChild<Spinner>(IDC_SLOTSSPIN);
	spinner->setRange(0, 100);

	attachChild(spinner, IDC_SPEEDSPIN);
	spinner->setRange(0, 10000);
	*/
}

DownloadPage::~DownloadPage() {
}

void DownloadPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point gridSize = grid->getPreferedSize();
	grid->layout(dwt::Rectangle(7, 4, getClientAreaSize().x - 14, gridSize.y));
}

void DownloadPage::write() {
	PropPage::write(items);

	const string& s = SETTING(DOWNLOAD_DIRECTORY);
	if(s.length() > 0 && s[s.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_DIRECTORY, s + '\\');
	}
	const string& t = SETTING(TEMP_DOWNLOAD_DIRECTORY);
	if(t.length() > 0 && t[t.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::TEMP_DOWNLOAD_DIRECTORY, t + '\\');
	}

}

void DownloadPage::handleConfigHubLists() {
	HubListsDlg(this).run();
}
