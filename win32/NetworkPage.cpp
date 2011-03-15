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

#include "NetworkPage.h"

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

NetworkPage::NetworkPage(dwt::Widget* parent) :
PropPage(parent, 2, 1),
directIn(0),
upnp(0),
nat(0),
passive(0),
externalIP(0),
tcp(0),
udp(0),
tls(0)
{
	setHelpId(IDH_NETWORKPAGE);

	grid->column(0).mode = GridInfo::FILL;

	{
		GroupBoxPtr autoGroup = grid->addChild(GroupBox::Seed(T_("Automatic connection type detection")));
		GridPtr cur = autoGroup->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		GridPtr cur2 = cur->addChild(Grid::Seed(1, 2));
		cur2->column(1).mode = GridInfo::FILL;
		cur2->column(1).align = GridInfo::BOTTOM_RIGHT;

		autoDetect = cur2->addChild(CheckBox::Seed(T_("Enable automatic incoming connection type detection")));
		autoDetect->setHelpId(IDH_SETTINGS_NETWORK_AUTODETECT);
		items.push_back(Item(autoDetect, SettingsManager::AUTO_DETECT_CONNECTION, PropPage::T_BOOL));
		autoDetect->onClicked(std::bind(&NetworkPage::handleAutoClicked, this));

		detectNow = cur2->addChild(Button::Seed(T_("Detect now")));
		detectNow->setHelpId(IDH_SETTINGS_NETWORK_DETECT_NOW);
		detectNow->onClicked(std::bind(&NetworkPage::handleDetectClicked, this));

		GroupBoxPtr logGroup = cur->addChild(GroupBox::Seed(T_("Detection log")));
		log = logGroup->addChild(WinUtil::Seeds::Dialog::richTextBox);
		logGroup->setHelpId(IDH_SETTINGS_NETWORK_DETECTION_LOG);
	}

	{
		incoming = grid->addChild(GroupBox::Seed(T_("Incoming connection settings")));
		GridPtr cur = incoming->addChild(Grid::Seed(2, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(10);

		GridPtr connType = cur->addChild(Grid::Seed(4, 1));
		connType->setSpacing(6);
 
		directIn = connType->addChild(RadioButton::Seed(T_("My computer is directly connected to Internet (no router)")));
		directIn->setHelpId(IDH_SETTINGS_NETWORK_DIRECT);

		upnp = connType->addChild(RadioButton::Seed(T_("Let DC++ configure my router (UPnP / NAT-PMP)")));
		upnp->setHelpId(IDH_SETTINGS_NETWORK_FIREWALL_UPNP);

		nat = connType->addChild(RadioButton::Seed(T_("Manual port forwarding (I have configured my router by myself)")));
		nat->setHelpId(IDH_SETTINGS_NETWORK_FIREWALL_NAT);

		passive = connType->addChild(RadioButton::Seed(T_("Passive mode (last resort - has serious limitations)")));
		passive->setHelpId(IDH_SETTINGS_NETWORK_FIREWALL_PASSIVE);

		{
			GroupBoxPtr ipGroup = cur->addChild(GroupBox::Seed(T_("External / WAN IP")));
			cur->setWidget(ipGroup, 1, 0);
			ipGroup->setHelpId(IDH_SETTINGS_NETWORK_EXTERNAL_IP);

			GridPtr ipGrid = ipGroup->addChild(Grid::Seed(2, 1));
			ipGrid->column(0).mode = GridInfo::FILL;

			externalIP = ipGrid->addChild(WinUtil::Seeds::Dialog::textBox);
			items.push_back(Item(externalIP, SettingsManager::EXTERNAL_IP, PropPage::T_STR));

			overrideIP = ipGrid->addChild(CheckBox::Seed(T_("Don't allow hubs/UPnP/NAT-PMP to override")));
			items.push_back(Item(overrideIP, SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL));
			overrideIP->setHelpId(IDH_SETTINGS_NETWORK_OVERRIDE);
		}

		GroupBoxPtr ports = cur->addChild(GroupBox::Seed(T_("Ports")));
		cur->setWidget(ports, 0, 1, 2);
		cur = ports->addChild(Grid::Seed(3, 2));
		cur->column(1).size = 40;
		cur->column(1).mode = GridInfo::STATIC;

		cur->addChild(Label::Seed(T_("TCP")))->setHelpId(IDH_SETTINGS_NETWORK_PORT_TCP);
		tcp = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(tcp, SettingsManager::TCP_PORT, PropPage::T_INT));
		tcp->setHelpId(IDH_SETTINGS_NETWORK_PORT_TCP);

		cur->addChild(Label::Seed(T_("UDP")))->setHelpId(IDH_SETTINGS_NETWORK_PORT_UDP);
		udp = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(udp, SettingsManager::UDP_PORT, PropPage::T_INT));
		udp->setHelpId(IDH_SETTINGS_NETWORK_PORT_UDP);

		cur->addChild(Label::Seed(T_("TLS")))->setHelpId(IDH_SETTINGS_NETWORK_PORT_TLS);
		tls = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(tls, SettingsManager::TLS_PORT, PropPage::T_INT));
		tls->setHelpId(IDH_SETTINGS_NETWORK_PORT_TLS);
	}

	PropPage::read(items);

	setRadioButtons();
	
	handleAutoClicked();

	ConnectivityManager::getInstance()->addListener(this);
}

NetworkPage::~NetworkPage() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void NetworkPage::setRadioButtons() {
	switch(SETTING(INCOMING_CONNECTIONS)) {
		case SettingsManager::INCOMING_FIREWALL_UPNP: upnp->setChecked(); break;
		case SettingsManager::INCOMING_FIREWALL_NAT: nat->setChecked(); break;
		case SettingsManager::INCOMING_FIREWALL_PASSIVE: passive->setChecked(); break;
		default: directIn->setChecked(); break;
	}
}

void NetworkPage::write()
{
	tstring x = externalIP->getText();
	tstring::size_type i;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	externalIP->setText(x);

	PropPage::write(items);

	// Set connection active/passive
	int ct = SettingsManager::INCOMING_DIRECT;

	if(upnp->getChecked())
		ct = SettingsManager::INCOMING_FIREWALL_UPNP;
	else if(nat->getChecked())
		ct = SettingsManager::INCOMING_FIREWALL_NAT;
	else if(passive->getChecked())
		ct = SettingsManager::INCOMING_FIREWALL_PASSIVE;

	if(SETTING(INCOMING_CONNECTIONS) != ct) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, ct);
	}
}

void NetworkPage::handleDetectClicked() {
	log->setText(Util::emptyStringT);
	detectNow->setEnabled(false);
	externalIP->setText(Util::emptyStringT);
	overrideIP->setChecked(false);
	ConnectivityManager::getInstance()->detectConnection();
}

void NetworkPage::handleAutoClicked() {
	bool enabled = autoDetect->getChecked();

	incoming->setEnabled(!enabled);
	detectNow->setEnabled(enabled && !ConnectivityManager::getInstance()->isRunning());

	//Save the checkbox state for ConnectivityManager
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, enabled);
}

void NetworkPage::addLogLine(const tstring& msg) {
	/// @todo factor out to dwt
	const tstring message = Text::toT("{\\urtf1\n") + log->rtfEscape(msg + Text::toT("\r\n")) + Text::toT("}\n");
	log->addTextSteady(message, message.size());
}

void NetworkPage::detectionFinished() {
	directIn->setChecked(false);
	upnp->setChecked(false);
	nat->setChecked(false);
	passive->setChecked(false);

	setRadioButtons();
	externalIP->setText(Text::toT(SETTING(EXTERNAL_IP)));
	detectNow->setEnabled(true);
}

void NetworkPage::on(Message, const string& message) throw() {
	callAsync(std::bind(&NetworkPage::addLogLine, this, Text::toT(message)));
}

void NetworkPage::on(Finished) throw() {
	callAsync(std::bind(&NetworkPage::detectionFinished, this));
}
