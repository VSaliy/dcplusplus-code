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
secondarySettingsToggleGrid(0),
globalSettingsGrid(0),
secondarySettingsGrid(0),
throttleEnable(0),
throttleTime(0)
{
	grid = addChild(Grid::Seed(4, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::AUTO;
	grid->row(0).align = GridInfo::STRETCH;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;
	grid->row(2).mode = GridInfo::AUTO;
	grid->row(2).align = GridInfo::STRETCH;
	grid->row(3).mode = GridInfo::FILL;
	grid->row(3).align = GridInfo::STRETCH;

	throttleEnable = grid->addChild(CheckBox::Seed(T_("Enable Transfer Rate Limiting")));
	items.push_back(Item(throttleEnable, SettingsManager::THROTTLE_ENABLE, PropPage::T_BOOL));
	throttleEnable->onClicked(std::tr1::bind(&BandwidthLimitPage::fixControls, this));

	{
		globalSettingsGrid = grid->addChild(GroupBox::Seed(T_("Transfer Rate Limiting")))->addChild(Grid::Seed(2, 2));
		globalSettingsGrid->column(0).size = 50;
		globalSettingsGrid->column(0).mode = GridInfo::STATIC;

		TextBoxPtr box = globalSettingsGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_UPLOAD_SPEED_MAIN, PropPage::T_INT_WITH_SPIN));

		SpinnerPtr spin = globalSettingsGrid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		globalSettingsGrid->setWidget(spin);

		globalSettingsGrid->addChild(Label::Seed(T_("Maximum Upload Rate (KiB/s) (0 = infinite)")));

		box = globalSettingsGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_DOWNLOAD_SPEED_MAIN, PropPage::T_INT_WITH_SPIN));

		spin = globalSettingsGrid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		globalSettingsGrid->setWidget(spin);

		globalSettingsGrid->addChild(Label::Seed(T_("Maximum Download Rate (KiB/s) (0 = infinite)")));
	}

	{
		secondarySettingsToggleGrid = grid->addChild(GroupBox::Seed(T_("")))->addChild(Grid::Seed(1, 4));
		secondarySettingsToggleGrid->column(0).mode = GridInfo::AUTO;
		secondarySettingsToggleGrid->column(1).mode = GridInfo::FILL;
		secondarySettingsToggleGrid->column(1).align = GridInfo::CENTER;
		secondarySettingsToggleGrid->column(2).mode = GridInfo::FILL;
		secondarySettingsToggleGrid->column(2).align = GridInfo::CENTER;
		secondarySettingsToggleGrid->column(3).mode = GridInfo::FILL;
		secondarySettingsToggleGrid->column(3).align = GridInfo::CENTER;

		throttleTime = secondarySettingsToggleGrid->addChild(CheckBox::Seed(T_("Use second set of bandwidth limits from")));
		items.push_back(Item(throttleTime, SettingsManager::TIME_DEPENDENT_THROTTLE, PropPage::T_BOOL));
		throttleTime->onClicked(std::tr1::bind(&BandwidthLimitPage::fixControls, this));

		timeBound[0] = secondarySettingsToggleGrid->addChild(WinUtil::Seeds::Dialog::comboBox);
		secondarySettingsToggleGrid->addChild(Label::Seed(T_("to")));
		timeBound[1] = secondarySettingsToggleGrid->addChild(WinUtil::Seeds::Dialog::comboBox);
	}

	for (int i = 0; i < 2; ++i) {
		timeBound[i]->addValue(T_("Midnight"));
		for (int j = 1; j < 12; ++j)
			timeBound[i]->addValue(Text::toT(Util::toString(j) +" AM").c_str());
	 	timeBound[i]->addValue(T_("Noon"));
		for (int j = 1; j < 12; ++j)
			timeBound[i]->addValue(Text::toT(Util::toString(j) +" PM").c_str());
		timeBound[i]->setSelected(i?SETTING(BANDWIDTH_LIMIT_END):SETTING(BANDWIDTH_LIMIT_START));
	}

	{
		secondarySettingsGrid = grid->addChild(GroupBox::Seed(T_("Secondary Transfer Rate Limiting")))->addChild(Grid::Seed(3, 2));
		secondarySettingsGrid->column(0).size = 50;
		secondarySettingsGrid->column(0).mode = GridInfo::STATIC;

		TextBoxPtr box = secondarySettingsGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_UPLOAD_SPEED_ALTERNATE, PropPage::T_INT_WITH_SPIN));

		SpinnerPtr spin = secondarySettingsGrid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		secondarySettingsGrid->setWidget(spin);

		secondarySettingsGrid->addChild(Label::Seed(T_("Maximum Upload Rate (KiB/s) (0 = infinite)")));

		box = secondarySettingsGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_DOWNLOAD_SPEED_ALTERNATE, PropPage::T_INT_WITH_SPIN));

		spin = secondarySettingsGrid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		secondarySettingsGrid->setWidget(spin);

		secondarySettingsGrid->addChild(Label::Seed(T_("Maximum Download Rate (KiB/s) (0 = infinite)")));

		box = secondarySettingsGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::SLOTS_ALTERNATE_LIMITING, PropPage::T_INT_WITH_SPIN));

		spin = secondarySettingsGrid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		secondarySettingsGrid->setWidget(spin);

		secondarySettingsGrid->addChild(Label::Seed(T_("Upload Slots")));
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

	SettingsManager::getInstance()->set(SettingsManager::BANDWIDTH_LIMIT_START, timeBound[0]->getSelected());
	SettingsManager::getInstance()->set(SettingsManager::BANDWIDTH_LIMIT_END, timeBound[1]->getSelected());
}

void BandwidthLimitPage::fixControls() {
	bool stateEnabled = throttleEnable->getChecked();
	bool stateTime = throttleTime->getChecked();
	globalSettingsGrid->setEnabled(stateEnabled);
	secondarySettingsGrid->setEnabled(stateEnabled && stateTime);
}
