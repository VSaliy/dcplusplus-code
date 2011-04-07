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

#include "resource.h"

#include "ConnectivityPage.h"

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

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
		GroupBoxPtr autoGroup = grid->addChild(GroupBox::Seed(T_("Automatic connection type detection")));
		GridPtr cur = autoGroup->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(1).mode = GridInfo::FILL;
		cur->row(1).align = GridInfo::STRETCH;

		GridPtr cur2 = cur->addChild(Grid::Seed(1, 2));
		cur2->column(1).mode = GridInfo::FILL;
		cur2->column(1).align = GridInfo::BOTTOM_RIGHT;

		autoDetect = cur2->addChild(CheckBox::Seed(T_("Enable automatic incoming connection type detection")));
		autoDetect->setHelpId(IDH_SETTINGS_CONNECTIVITY_AUTODETECT);
		items.push_back(Item(autoDetect, SettingsManager::AUTO_DETECT_CONNECTION, PropPage::T_BOOL));
		autoDetect->onClicked([this] { handleAutoClicked(); });

		detectNow = cur2->addChild(Button::Seed(T_("Detect now")));
		detectNow->setHelpId(IDH_SETTINGS_CONNECTIVITY_DETECT_NOW);
		detectNow->onClicked([] { ConnectivityManager::getInstance()->detectConnection(); });

		GroupBoxPtr logGroup = cur->addChild(GroupBox::Seed(T_("Detection log")));
		logGroup->setHelpId(IDH_SETTINGS_CONNECTIVITY_DETECTION_LOG);

		auto seed = WinUtil::Seeds::Dialog::richTextBox;
		seed.lines = 0;
		log = logGroup->addChild(seed);
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
		detectNow->setEnabled(BOOLSETTING(AUTO_DETECT_CONNECTION) && !ConnectivityManager::getInstance()->isRunning());
	});
}
