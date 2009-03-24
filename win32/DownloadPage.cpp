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

#include "stdafx.h"

#include "resource.h"

#include "DownloadPage.h"

#include <dwt/widgets/Spinner.h>

#include <dcpp/SettingsManager.h>
#include "HubListsDlg.h"
#include "WinUtil.h"

DownloadPage::DownloadPage(dwt::Widget* parent) :
PropPage(parent),
grid(0)
{
	setHelpId(IDH_DOWNLOADPAGE);

	grid = addChild(Grid::Seed(3, 1));
	grid->column(0).mode = GridInfo::FILL;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Directories")));
		group->setHelpId(IDH_SETTINGS_DOWNLOAD_DOWNLOADDIR);

		GridPtr cur = group->addChild(Grid::Seed(4, 2));
		cur->column(0).mode = GridInfo::FILL;

		LabelPtr label = cur->addChild(Label::Seed(T_("Default download directory")));
		cur->setWidget(label, 0, 0, 1, 2);
		label->setHelpId(IDH_SETTINGS_DOWNLOAD_DOWNLOADDIR);

		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::DOWNLOAD_DIRECTORY, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_DOWNLOAD_DOWNLOADDIR);

		ButtonPtr browse = cur->addChild(Button::Seed(T_("Browse...")));
		browse->onClicked(std::tr1::bind(&DownloadPage::handleBrowseDir, this, items.back()));
		browse->setHelpId(IDH_SETTINGS_DOWNLOAD_DOWNLOADDIR);

		label = cur->addChild(Label::Seed(T_("Unfinished downloads directory")));
		cur->setWidget(label, 2, 0, 1, 2);
		label->setHelpId(IDH_SETTINGS_DOWNLOAD_TEMP_DOWNLOAD_DIRECTORY);

		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::TEMP_DOWNLOAD_DIRECTORY, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_DOWNLOAD_TEMP_DOWNLOAD_DIRECTORY);

		browse = cur->addChild(Button::Seed(T_("Browse...")));
		browse->onClicked(std::tr1::bind(&DownloadPage::handleBrowseDir, this, items.back()));
		browse->setHelpId(IDH_SETTINGS_DOWNLOAD_TEMP_DOWNLOAD_DIRECTORY);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Limits")));
		group->setHelpId(IDH_SETTINGS_DOWNLOAD_LIMITS);

		GridPtr cur = group->addChild(Grid::Seed(3, 2));
		cur->column(0).size = 40;
		cur->column(0).mode = GridInfo::STATIC;

		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::DOWNLOAD_SLOTS, PropPage::T_INT_WITH_SPIN));
		box->setHelpId(IDH_SETTINGS_DOWNLOAD_DOWNLOADS);

		SpinnerPtr spin = cur->addChild(Spinner::Seed(0, 100, box));
		cur->setWidget(spin);
		spin->setHelpId(IDH_SETTINGS_DOWNLOAD_DOWNLOADS);

		cur->addChild(Label::Seed(T_("Maximum simultaneous downloads (0 = infinite)")))->setHelpId(IDH_SETTINGS_DOWNLOAD_DOWNLOADS);

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_DOWNLOAD_SPEED, PropPage::T_INT_WITH_SPIN));
		box->setHelpId(IDH_SETTINGS_DOWNLOAD_MAXSPEED);

		spin = cur->addChild(Spinner::Seed(0, 10000, box));
		cur->setWidget(spin);
		spin->setHelpId(IDH_SETTINGS_DOWNLOAD_MAXSPEED);

		cur->addChild(Label::Seed(T_("No new downloads if speed exceeds (KiB/s, 0 = disable)")))->setHelpId(IDH_SETTINGS_DOWNLOAD_MAXSPEED);

		// xgettext:no-c-format
		LabelPtr label = cur->addChild(Label::Seed(T_("Note; because of changing download speeds, this is not 100% accurate...")));
		cur->setWidget(label, 2, 0, 1, 2);
		label->setHelpId(IDH_SETTINGS_DOWNLOAD_LIMITS);
	}

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Public Hubs list")))->addChild(Grid::Seed(4, 1));
		cur->column(0).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Public Hubs list URL")));

		// dummy grid so that the button doesn't fill the whole row.
		cur->addChild(Grid::Seed(1, 1))->addChild(Button::Seed(T_("Configure Public Hub Lists")))->onClicked(std::tr1::bind(&DownloadPage::handleConfigHubLists, this));

		cur->addChild(Label::Seed(T_("HTTP Proxy (for hublist only)")))->setHelpId(IDH_SETTINGS_DOWNLOAD_PROXY);
		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::HTTP_PROXY, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_DOWNLOAD_PROXY);
	}

	PropPage::read(items);
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
