/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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
#include <dcpp/Socket.h>
#include "WinUtil.h"
#include <dwt/util/win32/Version.h>

NetworkPage::NetworkPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
directIn(0),
upnp(0),
nat(0),
passive(0),
ipGroup(0),
externalIP(0),
ports(0),
tcp(0),
udp(0),
tls(0),
directOut(0),
socks5(0),
socksSettings(0),
socksServer(0)
{
	setHelpId(IDH_NETWORKPAGE);

	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->setSpacing(10);

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Incoming connection settings")))->addChild(Grid::Seed(1, 2));
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		GridPtr connType = cur->addChild(Grid::Seed(6, 1));
		connType->setSpacing(6);
 
		directIn = connType->addChild(RadioButton::Seed(T_("My computer is directly connected to Internet (no router)")));
		directIn->setHelpId(IDH_SETTINGS_NETWORK_DIRECT);

		upnp = connType->addChild(RadioButton::Seed(T_("Use UPnP to let DC++ configure my router")));
		upnp->setHelpId(IDH_SETTINGS_NETWORK_FIREWALL_UPNP);

		/// @todo add a combo which will be layout'd on the right of the UPnP radio button to select the UPnP lib to use

		nat = connType->addChild(RadioButton::Seed(T_("Manual port forwarding (I have configured my router by myself)")));
		nat->setHelpId(IDH_SETTINGS_NETWORK_FIREWALL_NAT);

		{
			ipGroup = connType->addChild(GroupBox::Seed(T_("External / WAN IP")));
			ipGroup->setHelpId(IDH_SETTINGS_NETWORK_EXTERNAL_IP);

			GridPtr ipGrid = ipGroup->addChild(Grid::Seed(2, 1));
			ipGrid->column(0).mode = GridInfo::FILL;

			externalIP = ipGrid->addChild(WinUtil::Seeds::Dialog::textBox);
			items.push_back(Item(externalIP, SettingsManager::EXTERNAL_IP, PropPage::T_STR));

			CheckBoxPtr overrideIP = ipGrid->addChild(CheckBox::Seed(T_("Don't allow hub/UPnP to override")));
			items.push_back(Item(overrideIP, SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL));
			overrideIP->setHelpId(IDH_SETTINGS_NETWORK_OVERRIDE);
		}

		passive = connType->addChild(RadioButton::Seed(T_("Passive mode (last resort - has serious limitations)")));
		passive->setHelpId(IDH_SETTINGS_NETWORK_FIREWALL_PASSIVE);

		ports = cur->addChild(GroupBox::Seed(T_("Ports")));

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

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Outgoing connection settings")))->addChild(Grid::Seed(1, 2));
		cur->row(0).align = GridInfo::TOP_LEFT;
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		{
			GridPtr outChoice = cur->addChild(Grid::Seed(2, 1));

			directOut = outChoice->addChild(RadioButton::Seed(T_("Direct connection")));
			directOut->setHelpId(IDH_SETTINGS_NETWORK_DIRECT_OUT);

			socks5 = outChoice->addChild(RadioButton::Seed(T_("SOCKS5")));
			cur->setWidget(socks5, 1, 0);
			socks5->setHelpId(IDH_SETTINGS_NETWORK_SOCKS5);
		}

		socksSettings = cur->addChild(GroupBox::Seed(T_("SOCKS5")));
		cur = socksSettings->addChild(Grid::Seed(5, 2));
		cur->addChild(Label::Seed(T_("IP")))->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_SERVER);
		cur->addChild(Label::Seed(T_("Port")))->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_PORT);
		socksServer = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(socksServer, SettingsManager::SOCKS_SERVER, PropPage::T_STR));
		socksServer->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_SERVER);
		socksServer->setTextLimit(250);
		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::SOCKS_PORT, PropPage::T_INT));
		box->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_PORT);
		box->setTextLimit(250);
		cur->addChild(Label::Seed(T_("Login")))->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_USER);
		cur->addChild(Label::Seed(T_("Password")))->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_PASSWORD);
		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::SOCKS_USER, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_USER);
		box->setTextLimit(250);
		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_PASSWORD);
		box->setTextLimit(250);
		CheckBoxPtr socksResolve = cur->addChild(CheckBox::Seed(T_("Use SOCKS5 server to resolve host names")));
		cur->setWidget(socksResolve, 4, 0, 1, 2);
		items.push_back(Item(socksResolve, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL));
		socksResolve->setHelpId(IDH_SETTINGS_NETWORK_SOCKS_RESOLVE);
	}

	if(!dwt::util::win32::ensureVersion(dwt::util::win32::XP)) {
		upnp->setEnabled(false);
	}
	switch(SETTING(INCOMING_CONNECTIONS)) {
		case SettingsManager::INCOMING_FIREWALL_UPNP: upnp->setChecked(); break;
		case SettingsManager::INCOMING_FIREWALL_NAT: nat->setChecked(); break;
		case SettingsManager::INCOMING_FIREWALL_PASSIVE: passive->setChecked(); break;
		default: directIn->setChecked(); break;
	}

	switch(SETTING(OUTGOING_CONNECTIONS)) {
		case SettingsManager::OUTGOING_SOCKS5: socks5->setChecked(); break;
		default: directOut->setChecked(); break;
	}

	PropPage::read(items);

	fixControlsIn();
	fixControlsOut();

	directIn->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));
	upnp->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));
	nat->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));
	passive->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));

	directOut->onClicked(std::tr1::bind(&NetworkPage::fixControlsOut, this));
	socks5->onClicked(std::tr1::bind(&NetworkPage::fixControlsOut, this));
}

NetworkPage::~NetworkPage() {
}

void NetworkPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point gridSize = grid->getPreferedSize();
	grid->layout(dwt::Rectangle(7, 4, getClientSize().x - 14, gridSize.y));
}

void NetworkPage::write()
{
	tstring x = externalIP->getText();
	tstring::size_type i;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	externalIP->setText(x);

	x = socksServer->getText();

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	socksServer->setText(x);

	PropPage::write(items);

	SettingsManager* settings = SettingsManager::getInstance();

	// Set connection active/passive
	int ct = SettingsManager::INCOMING_DIRECT;

	if(upnp->getChecked())
		ct = SettingsManager::INCOMING_FIREWALL_UPNP;
	else if(nat->getChecked())
		ct = SettingsManager::INCOMING_FIREWALL_NAT;
	else if(passive->getChecked())
		ct = SettingsManager::INCOMING_FIREWALL_PASSIVE;

	if(SETTING(INCOMING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::INCOMING_CONNECTIONS, ct);
	}

	ct = SettingsManager::OUTGOING_DIRECT;

	if(socks5->getChecked())
		ct = SettingsManager::OUTGOING_SOCKS5;

	if(SETTING(OUTGOING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::OUTGOING_CONNECTIONS, ct);
		Socket::socksUpdated();
	}
}

void NetworkPage::fixControlsIn() {
	bool enable = directIn->getChecked() || upnp->getChecked() || nat->getChecked();
	ipGroup->setEnabled(enable);
	ports->setEnabled(enable);
}

void NetworkPage::fixControlsOut() {
	socksSettings->setEnabled(socks5->getChecked());
}
