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

#include "BandwidthLimitPage.h"

#include <dwt/widgets/Spinner.h>

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

BandwidthLimitPage::BandwidthLimitPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
globalSettingsGrid(0),
throttleEnable(0)
{
	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::AUTO;
	grid->row(0).align = GridInfo::STRETCH;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	throttleEnable = grid->addChild(CheckBox::Seed(T_("Enable Transfer Rate Limiting")));
	items.push_back(Item(throttleEnable, SettingsManager::THROTTLE_ENABLE, PropPage::T_BOOL));
	throttleEnable->setHelpId(IDH_SETTINGS_NETWORK_OVERRIDE);
	throttleEnable->onClicked(std::tr1::bind(&BandwidthLimitPage::fixControls, this));

	{
		globalSettingsGrid = grid->addChild(GroupBox::Seed(T_("Transfer Rate Limiting")))->addChild(Grid::Seed(2, 2));
		globalSettingsGrid->column(0).size = 50;
		globalSettingsGrid->column(0).mode = GridInfo::STATIC;

		TextBoxPtr box = globalSettingsGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_UPLOAD_SPEED_CURRENT, PropPage::T_INT_WITH_SPIN));

		SpinnerPtr spin = globalSettingsGrid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		globalSettingsGrid->setWidget(spin);

		globalSettingsGrid->addChild(Label::Seed(T_("Maximum Upload Rate (KiB/s) (0 = infinite)")));

		box = globalSettingsGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_DOWNLOAD_SPEED_CURRENT, PropPage::T_INT_WITH_SPIN));

		spin = globalSettingsGrid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		globalSettingsGrid->setWidget(spin);

		globalSettingsGrid->addChild(Label::Seed(T_("Maximum Download Rate (KiB/s) (0 = infinite)")));
	}
	PropPage::read(items);

	fixControls();
}

BandwidthLimitPage::~BandwidthLimitPage() {
}

void BandwidthLimitPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void BandwidthLimitPage::write() {
	PropPage::write(items);
}

void BandwidthLimitPage::fixControls() {
	globalSettingsGrid->setEnabled(throttleEnable->getChecked());
}
