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

#include "NetworkPage.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>

#include <dcpp/SettingsManager.h>
#include <dcpp/Socket.h>
#include "WinUtil.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_DIRECT, IDH_SETTINGS_NETWORK_DIRECT },
	{ IDC_FIREWALL_UPNP, IDH_SETTINGS_NETWORK_FIREWALL_UPNP },
	{ IDC_FIREWALL_NAT, IDH_SETTINGS_NETWORK_FIREWALL_NAT },
	{ IDC_FIREWALL_PASSIVE, IDH_SETTINGS_NETWORK_FIREWALL_PASSIVE },
	{ IDC_SETTINGS_PORT_TCP, IDH_SETTINGS_NETWORK_PORT_TCP },
	{ IDC_PORT_TCP, IDH_SETTINGS_NETWORK_PORT_TCP },
	{ IDC_SETTINGS_PORT_UDP, IDH_SETTINGS_NETWORK_PORT_UDP },
	{ IDC_PORT_UDP, IDH_SETTINGS_NETWORK_PORT_UDP },
	{ IDC_SETTINGS_PORT_TLS, IDH_SETTINGS_NETWORK_PORT_TLS },
	{ IDC_PORT_TLS, IDH_SETTINGS_NETWORK_PORT_TLS },
	{ IDC_SETTINGS_IP, IDH_SETTINGS_NETWORK_EXTERNAL_IP },
	{ IDC_EXTERNAL_IP, IDH_SETTINGS_NETWORK_EXTERNAL_IP },
	{ IDC_OVERRIDE, IDH_SETTINGS_NETWORK_OVERRIDE },
	{ IDC_DIRECT_OUT, IDH_SETTINGS_NETWORK_DIRECT_OUT },
	{ IDC_SOCKS5, IDH_SETTINGS_NETWORK_SOCKS5 },
	{ IDC_SETTINGS_SOCKS5_IP, IDH_SETTINGS_NETWORK_SOCKS_SERVER },
	{ IDC_SOCKS_SERVER, IDH_SETTINGS_NETWORK_SOCKS_SERVER },
	{ IDC_SETTINGS_SOCKS5_PORT, IDH_SETTINGS_NETWORK_SOCKS_PORT },
	{ IDC_SOCKS_PORT, IDH_SETTINGS_NETWORK_SOCKS_PORT },
	{ IDC_SETTINGS_SOCKS5_USERNAME, IDH_SETTINGS_NETWORK_SOCKS_USER },
	{ IDC_SOCKS_USER, IDH_SETTINGS_NETWORK_SOCKS_USER },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, IDH_SETTINGS_NETWORK_SOCKS_PASSWORD },
	{ IDC_SOCKS_PASSWORD, IDH_SETTINGS_NETWORK_SOCKS_PASSWORD },
	{ IDC_SOCKS_RESOLVE, IDH_SETTINGS_NETWORK_SOCKS_RESOLVE },
	{ 0, 0 }
};
*/

NetworkPage::NetworkPage(dwt::Widget* parent) : PropPage(parent) {
	createDialog(IDD_NETWORKPAGE);
	setHelpId(IDH_NETWORKPAGE);

	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;

	GroupBoxPtr groupIn = grid->addChild(GroupBox::Seed(T_("Incoming connection settings")));
	GridPtr gridIn = groupIn->addChild(Grid::Seed(1, 2));
	gridIn->row(0).align = GridInfo::TOP_LEFT;

	GridPtr connType = gridIn->addChild(Grid::Seed(7, 1));

	directIn = connType->addChild(RadioButton::Seed(T_("Direct connection")));
	upnp = connType->addChild(RadioButton::Seed(T_("Firewall with UPnP")));
	nat = connType->addChild(RadioButton::Seed(T_("Firewall with manual port forwarding")));
	connType->addChild(Label::Seed(T_("External / WAN IP")));
	externalIP = connType->addChild(TextBox::Seed());
	items.push_back(Item(externalIP, SettingsManager::EXTERNAL_IP, PropPage::T_STR));
	overrideIP = connType->addChild(CheckBox::Seed(T_("Don't allow hub/UPnP to override")));
	items.push_back(Item(overrideIP, SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL));
	passive = connType->addChild(RadioButton::Seed(T_("Firewall (passive, last resort)")));

	ports = gridIn->addChild(Grid::Seed(4, 2));

	ports->column(1).size = 30;
	ports->column(1).mode = dwt::GridInfo::STATIC;

	LabelPtr portsLabel = ports->addChild(Label::Seed(T_("Ports")));
	ports->setWidget(portsLabel, 0, 0, 1, 2);

	ports->addChild(Label::Seed(T_("TCP")));
	tcp = ports->addChild(TextBox::Seed());
	items.push_back(Item(tcp, SettingsManager::TCP_PORT, PropPage::T_INT));

	ports->addChild(Label::Seed(T_("UDP")));
	udp = ports->addChild(TextBox::Seed());
	items.push_back(Item(udp, SettingsManager::UDP_PORT, PropPage::T_INT));

	ports->addChild(Label::Seed(T_("TLS")));
	tls = ports->addChild(TextBox::Seed());
	items.push_back(Item(tls, SettingsManager::TLS_PORT, PropPage::T_INT));

	GroupBoxPtr groupOut = grid->addChild(GroupBox::Seed(T_("Outgoing connection settings")));
	GridPtr gridOut = groupOut->addChild(Grid::Seed(7, 2));

	directOut = gridOut->addChild(RadioButton::Seed(T_("Direct connection")));
	gridOut->setWidget(directOut, 0, 0, 1, 2);
	socks5 = gridOut->addChild(RadioButton::Seed(T_("SOCKS5")));
	gridOut->setWidget(socks5, 1, 0, 1, 2);
	gridOut->addChild(Label::Seed(T_("Socks IP")));
	gridOut->addChild(Label::Seed(T_("Port")));
	socksServer = gridOut->addChild(TextBox::Seed());
	items.push_back(Item(socksServer, SettingsManager::SOCKS_SERVER, PropPage::T_STR));
	socksPort = gridOut->addChild(TextBox::Seed());
	items.push_back(Item(socksPort, SettingsManager::SOCKS_PORT, PropPage::T_INT));
	gridOut->addChild(Label::Seed(T_("Login")));
	gridOut->addChild(Label::Seed(T_("Password")));
	socksLogin = gridOut->addChild(TextBox::Seed());
	items.push_back(Item(socksLogin, SettingsManager::SOCKS_USER, PropPage::T_INT));
	socksPass = gridOut->addChild(TextBox::Seed());
	items.push_back(Item(socksPass, SettingsManager::SOCKS_PASSWORD, PropPage::T_INT));
	socksResolve = gridOut->addChild(CheckBox::Seed(T_("Use SOCKS5 server to resolve host names")));
	gridOut->setWidget(socksResolve, 6, 0, 1, 2);
	items.push_back(Item(socksResolve, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL));

	tcp->setNumbersOnly();
	udp->setNumbersOnly();
	tls->setNumbersOnly();
	socksPort->setNumbersOnly();

	socksServer->setTextLimit(250);
	socksPort->setTextLimit(250);
	socksLogin->setTextLimit(250);
	socksPass->setTextLimit(250);

	if(!(WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1 //WinXP & WinSvr2003
		|| WinUtil::getOsMajor() >= 6 )) //Vista
	{
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
	grid->layout(dwt::Rectangle(7, 4, getClientAreaSize().x - 14, gridSize.y));
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

	externalIP->setEnabled(enable);
	overrideIP->setEnabled(enable);

	ports->setEnabled(enable);
}

void NetworkPage::fixControlsOut() {
	bool enable = socks5->getChecked();

	socksServer->setEnabled(enable);
	socksPort->setEnabled(enable);
	socksLogin->setEnabled(enable);
	socksPass->setEnabled(enable);
	socksResolve->setEnabled(enable);
}
