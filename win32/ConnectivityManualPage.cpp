/*
 * Copyright (C) 2001-2015 Jacek Sieka, arnetheduck on gmail point com
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
active(0),
upnp(0),
passive(0),
mapper(0),
v4Bind(0),
v6Bind(0),
transferBox(0),
tlstransferBox(0)
{
	setHelpId(IDH_CONNECTIVITYMANUALPAGE);

	grid->column(0).mode = GridInfo::FILL;

	{
		autoGroup = grid->addChild(GroupBox::Seed(T_("Automatic connectivity setup")));
		autoGroup->setHelpId(IDH_SETTINGS_CONNECTIVITY_AUTODETECT);

		auto cur = autoGroup->addChild(Grid::Seed(1, 1));
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		autoDetect = cur->addChild(CheckBox::Seed(T_("Let DC++ determine IPv4 settings")));
		autoDetect->onClicked([this] { handleAutoClicked(/*false*/); });
	}

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		active = cur->addChild(RadioButton::Seed(T_("Active mode (I have no router / I have configured my router)")));
		active->setHelpId(IDH_SETTINGS_CONNECTIVITY_ACTIVE);

		upnp = cur->addChild(RadioButton::Seed(T_("Active mode (let DC++ configure my router with NAT-PMP / UPnP)")));
		upnp->setHelpId(IDH_SETTINGS_CONNECTIVITY_UPNP);

		passive = cur->addChild(RadioButton::Seed(T_("Passive mode (last resort - has serious limitations)")));
		passive->setHelpId(IDH_SETTINGS_CONNECTIVITY_PASSIVE);
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("External / WAN IPv4 and IPv6")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_EXTERNAL_IP);

		auto cur = group->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		auto externalIPv4 = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		externalIPv4->setCue(T_("Enter your external IPv4 address here"));
		items.emplace_back(externalIPv4, SettingsManager::EXTERNAL_IP, PropPage::T_STR);
		WinUtil::preventSpaces(externalIPv4);

		auto externalIPv6 = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		externalIPv6->setCue(T_("Enter your external IPv6 address here"));
		items.emplace_back(externalIPv6, SettingsManager::EXTERNAL_IP6, PropPage::T_STR);
		WinUtil::preventSpaces(externalIPv6);

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

			auto inputBox = boxGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
			items.emplace_back(inputBox, setting, PropPage::T_INT);

			return inputBox;
		};

		transferBox = addPortBox(T_("Transfer"), SettingsManager::TCP_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_TCP);
		transferBox->onUpdated([this] { onTransferPortUpdated(); });

		tlstransferBox = addPortBox(T_("Encrypted transfer"), SettingsManager::TLS_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_TLS);
		tlstransferBox->onUpdated([this] { onTLSTransferPortUpdated(); });

		addPortBox(T_("Search"), SettingsManager::UDP_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_UDP);
	}

	{
		auto cur = grid->addChild(Grid::Seed(3, 1));
		cur->setSpacing(grid->getSpacing());


		auto group = cur->addChild(GroupBox::Seed(T_("Preferred port mapper")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_MAPPER);

		mapper = group->addChild(WinUtil::Seeds::Dialog::comboBox);

		group = cur->addChild(GroupBox::Seed(T_("IPv4 Bind Address")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_BIND_ADDRESS);
		v4Bind = group->addChild(WinUtil::Seeds::Dialog::comboBox);

		group = cur->addChild(GroupBox::Seed(T_("IPv6 Bind Address")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_BIND_ADDRESS6);
		v6Bind = group->addChild(WinUtil::Seeds::Dialog::comboBox);
	}

	read();

	updateAuto();

	ConnectivityManager::getInstance()->addListener(this);
}

ConnectivityManualPage::~ConnectivityManualPage() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void ConnectivityManualPage::write() {
	if(transferBox->getText() == tlstransferBox->getText())
	{
		tlstransferBox->setText(Util::emptyStringT);
	}

	PropPage::write(items);

	// Set the connection mode
	int c = upnp->getChecked() ? SettingsManager::INCOMING_ACTIVE_UPNP :
		passive->getChecked() ? SettingsManager::INCOMING_PASSIVE :
		SettingsManager::INCOMING_ACTIVE;
	if(SETTING(INCOMING_CONNECTIONS) != c) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, c);
	}


	auto saveBind = [](ComboBoxPtr bind, bool v6) {
		auto setting = Text::fromT(bind->getText());
		size_t found = setting.rfind(" - "); // "friendly_name - ip_address"
		setting.erase(0, found+3);
		v6 ?
			SettingsManager::getInstance()->set(SettingsManager::BIND_ADDRESS6, setting):
			SettingsManager::getInstance()->set(SettingsManager::BIND_ADDRESS, setting);
	};

	SettingsManager::getInstance()->set(SettingsManager::MAPPER, Text::fromT(mapper->getText()));
	saveBind(v4Bind, false);
	saveBind(v6Bind, true);
}

void ConnectivityManualPage::handleAutoClicked() {
	// apply immediately so that ConnectivityManager updates.
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, autoDetect->getChecked());
	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
}

void ConnectivityManualPage::updateAuto() {
	bool setting = SETTING(AUTO_DETECT_CONNECTION);
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
	case SettingsManager::INCOMING_ACTIVE_UPNP: upnp->setChecked(); break;
	case SettingsManager::INCOMING_PASSIVE: passive->setChecked(); break;
	default: active->setChecked(); break;
	}

	PropPage::read(items);

	{
		const auto& setting = SETTING(MAPPER);
		int sel = 0;

		auto mappers = ConnectivityManager::getInstance()->getMappers(false);
		for (const auto& name : mappers) {
			auto pos = mapper->addValue(Text::toT(name));
			if (!sel && name == setting)
				sel = pos;
		}
		mapper->setSelected(sel);
	}

	auto fillBind = [](ComboBoxPtr bindBox, bool v6) {
		const auto& setting = v6 ? SETTING(BIND_ADDRESS6) : SETTING(BIND_ADDRESS);
		int sel = 0;

		const auto& addresses = Util::getIpAddresses(v6);
		for (const auto& ipstr : addresses) {
			auto valStr = Text::toT(ipstr.adapterName) + _T(" - ") + Text::toT(ipstr.ip);
			auto pos = bindBox->addValue(valStr);
			if (!sel && Text::fromT(valStr) == setting) {
				sel = pos;
			}
		}
		bindBox->setSelected(sel);
	};

	fillBind(v4Bind, false);
	fillBind(v6Bind, true);
}

void ConnectivityManualPage::on(SettingChanged) noexcept {
	callAsync([this] {
		updateAuto();

		// reload settings in case they have been changed (eg by the "Edit detected settings" feature).

		active->setChecked(false);
		upnp->setChecked(false);
		passive->setChecked(false);

		mapper->clear();
		v4Bind->clear();
		v6Bind->clear();

		read();
	});
}

void ConnectivityManualPage::onTransferPortUpdated()
{
	validatePort(transferBox, tlstransferBox, T_("Transfer"), T_("encrypted transfer"));
}

void ConnectivityManualPage::onTLSTransferPortUpdated()
{
	validatePort(tlstransferBox, transferBox, T_("Encrypted transfer"), T_("transfer"));
}

void ConnectivityManualPage::validatePort(TextBoxPtr sourcebox, TextBoxPtr otherbox, const tstring& source, const tstring& other)
{
	if(sourcebox->getText() == otherbox->getText())
	{
		sourcebox->showPopup(T_("Invalid value"), str(TF_("%1% port cannot be the same as the %2% port") % source % other), TTI_ERROR);
	}
}
