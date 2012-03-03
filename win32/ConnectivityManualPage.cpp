/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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
#include "ConnectivityManualPage.h"

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/format.h>
#include <dcpp/MappingManager.h>
#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/RadioButton.h>

#include "WinUtil.h"
#include "resource.h"

using dwt::Control;
using dwt::Grid;
using dwt::GridInfo;
using dwt::RadioButton;

ConnectivityManualPage::ConnectivityManualPage(dwt::Widget* parent) :
PropPage(parent, 5, 1),
autoGroup(0),
autoDetect(0),
directIn(0),
upnp(0),
manual(0),
passive(0),
externalIP(0),
mapper(0)
{
	setHelpId(IDH_CONNECTIVITYMANUALPAGE);

	grid->column(0).mode = GridInfo::FILL;

	{
		autoGroup = grid->addChild(GroupBox::Seed(T_("Automatic connectivity setup")));
		autoGroup->setHelpId(IDH_SETTINGS_CONNECTIVITY_AUTODETECT);

		autoDetect = autoGroup->addChild(CheckBox::Seed(T_("Let DC++ determine the best connectivity settings")));
		autoDetect->onClicked([this] { handleAutoClicked(); });
	}

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(4, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		directIn = cur->addChild(RadioButton::Seed(T_("My computer is directly connected to the Internet (no router)")));
		directIn->setHelpId(IDH_SETTINGS_CONNECTIVITY_DIRECT);

		upnp = cur->addChild(RadioButton::Seed(T_("Let DC++ configure my router (NAT-PMP / UPnP)")));
		upnp->setHelpId(IDH_SETTINGS_CONNECTIVITY_FIREWALL_UPNP);

		manual = cur->addChild(RadioButton::Seed(T_("Manual port forwarding (I have configured my router by myself)")));
		manual->setHelpId(IDH_SETTINGS_CONNECTIVITY_FIREWALL_NAT);

		passive = cur->addChild(RadioButton::Seed(T_("Passive mode (last resort - has serious limitations)")));
		passive->setHelpId(IDH_SETTINGS_CONNECTIVITY_FIREWALL_PASSIVE);
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("External / WAN IP")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_EXTERNAL_IP);

		auto cur = group->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		externalIP = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(externalIP, SettingsManager::EXTERNAL_IP, PropPage::T_STR);

		auto overrideIP = cur->addChild(CheckBox::Seed(T_("Don't allow hubs/NAT-PMP/UPnP to override")));
		items.emplace_back(overrideIP, SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL);
		overrideIP->setHelpId(IDH_SETTINGS_CONNECTIVITY_OVERRIDE);
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 3));
		cur->setSpacing(grid->getSpacing());

		auto addPortBox = [this, cur](const tstring& text, int setting, unsigned helpId) {
			auto group = cur->addChild(GroupBox::Seed(str(TF_("%1% port") % text)));
			group->setHelpId(helpId);

			auto boxGrid = group->addChild(Grid::Seed(1, 1));
			boxGrid->column(0).size = 40;
			boxGrid->column(0).mode = GridInfo::STATIC;

			items.emplace_back(boxGrid->addChild(WinUtil::Seeds::Dialog::intTextBox), setting, PropPage::T_INT);
		};

		addPortBox(T_("Transfer"), SettingsManager::TCP_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_TCP);
		addPortBox(T_("Encrypted transfer"), SettingsManager::TLS_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_TLS);
		addPortBox(T_("Search"), SettingsManager::UDP_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_UDP);
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(1).mode = GridInfo::FILL;

		auto group = cur->addChild(GroupBox::Seed(T_("Preferred port mapping interface")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_MAPPER);

		mapper = group->addChild(WinUtil::Seeds::Dialog::comboBox);

		group = cur->addChild(GroupBox::Seed(T_("Bind address")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_BIND_ADDRESS);

		items.emplace_back(group->addChild(WinUtil::Seeds::Dialog::textBox), SettingsManager::BIND_ADDRESS, PropPage::T_STR);
	}

	read();

	updateAuto();

	ConnectivityManager::getInstance()->addListener(this);
}

ConnectivityManualPage::~ConnectivityManualPage() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void ConnectivityManualPage::write() {
	tstring x = externalIP->getText();
	tstring::size_type i;
	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	externalIP->setText(x);

	PropPage::write(items);

	// Set the connection mode
	int c = upnp->getChecked() ? SettingsManager::INCOMING_FIREWALL_UPNP :
		manual->getChecked() ? SettingsManager::INCOMING_FIREWALL_NAT :
		passive->getChecked() ? SettingsManager::INCOMING_FIREWALL_PASSIVE :
		SettingsManager::INCOMING_DIRECT;
	if(SETTING(INCOMING_CONNECTIONS) != c) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, c);
	}

	SettingsManager::getInstance()->set(SettingsManager::MAPPER, Text::fromT(mapper->getText()));
}

void ConnectivityManualPage::handleAutoClicked() {
	// apply immediately so that ConnectivityManager updates.
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, autoDetect->getChecked());
	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
}

void ConnectivityManualPage::updateAuto() {
	bool setting = BOOLSETTING(AUTO_DETECT_CONNECTION);
	autoDetect->setChecked(setting);

	auto controls = grid->getChildren<Control>();
	boost::for_each(controls, [this, setting](Control* control) {
		if(control != autoGroup) {
			control->setEnabled(!setting);
		}
	});
}

void ConnectivityManualPage::read() {
	switch(SETTING(INCOMING_CONNECTIONS)) {
	case SettingsManager::INCOMING_FIREWALL_UPNP: upnp->setChecked(); break;
	case SettingsManager::INCOMING_FIREWALL_NAT: manual->setChecked(); break;
	case SettingsManager::INCOMING_FIREWALL_PASSIVE: passive->setChecked(); break;
	default: directIn->setChecked(); break;
	}

	PropPage::read(items);

	const auto& setting = SETTING(MAPPER);
	int sel = 0;

	auto mappers = MappingManager::getInstance()->getMappers();
	for(const auto& name: mappers) {
		auto pos = mapper->addValue(Text::toT(name));
		if(!sel && name == setting)
			sel = pos;
	}

	mapper->setSelected(sel);
}

void ConnectivityManualPage::on(SettingChanged) noexcept {
	callAsync([this] {
		updateAuto();

		// reload settings in case they have been changed (eg by the "Edit detected settings" feature).

		directIn->setChecked(false);
		upnp->setChecked(false);
		manual->setChecked(false);
		passive->setChecked(false);

		mapper->clear();

		read();
	});
}
