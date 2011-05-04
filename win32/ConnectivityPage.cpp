/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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
#include "ConnectivityPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::GroupBox;

ConnectivityPage::ConnectivityPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
autoDetect(0),
detectNow(0),
log(0)
{
	setHelpId(IDH_CONNECTIVITYPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Automatic connectivity setup")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_AUTODETECT);

		auto cur = group->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(1).mode = GridInfo::FILL;
		cur->row(1).align = GridInfo::STRETCH;

		auto cur2 = cur->addChild(Grid::Seed(1, 2));
		cur2->column(1).mode = GridInfo::FILL;
		cur2->column(1).align = GridInfo::BOTTOM_RIGHT;

		autoDetect = cur2->addChild(CheckBox::Seed(T_("Let DC++ determine the best connectivity settings")));
		items.push_back(Item(autoDetect, SettingsManager::AUTO_DETECT_CONNECTION, PropPage::T_BOOL));
		autoDetect->onClicked([this] { handleAutoClicked(); });

		detectNow = cur2->addChild(Button::Seed(T_("Detect now")));
		detectNow->setHelpId(IDH_SETTINGS_CONNECTIVITY_DETECT_NOW);
		detectNow->onClicked([] { ConnectivityManager::getInstance()->detectConnection(); });

		group = cur->addChild(GroupBox::Seed(T_("Detection log")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_DETECTION_LOG);

		log = group->addChild(WinUtil::Seeds::Dialog::richTextBox);
	}

	PropPage::read(items);

	handleAutoClicked();

	ConnectivityManager::getInstance()->addListener(this);
}

ConnectivityPage::~ConnectivityPage() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void ConnectivityPage::write() {
	PropPage::write(items);
}

void ConnectivityPage::handleAutoClicked() {
	// apply immediately so that ConnectivityManager updates.
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, autoDetect->getChecked());
	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
}

void ConnectivityPage::addLogLine(const tstring& msg) {
	/// @todo factor out to dwt
	const tstring message = Text::toT("{\\urtf1\n") + log->rtfEscape(msg + Text::toT("\r\n")) + Text::toT("}\n");
	log->addTextSteady(message, message.size());
}

void ConnectivityPage::on(Message, const string& message) noexcept {
	callAsync([this, message] { addLogLine(Text::toT(message)); });
}

void ConnectivityPage::on(Started) noexcept {
	callAsync([this] {
		detectNow->setEnabled(false);
		this->log->setText(Util::emptyStringT);
	});
}

void ConnectivityPage::on(Finished) noexcept {
	callAsync([this] { detectNow->setEnabled(true); });
}

void ConnectivityPage::on(SettingChanged) noexcept {
	callAsync([this] {
		bool setting = BOOLSETTING(AUTO_DETECT_CONNECTION);
		autoDetect->setChecked(setting);
		detectNow->setEnabled(setting && !ConnectivityManager::getInstance()->isRunning());
	});
}
