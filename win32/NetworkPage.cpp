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

PropPage::TextItem NetworkPage::texts[] = {
	{ IDC_DIRECT_OUT, N_("Direct connection") },
	{ IDC_FIREWALL_PASSIVE, N_("") },
	{ IDC_OVERRIDE, N_("") },
	{ IDC_SOCKS5, N_("SOCKS5") },
	{ IDC_SETTINGS_PORTS, N_() },
	{ IDC_SETTINGS_IP, N_("") },
	{ IDC_SETTINGS_PORT_TCP, N_("") },
	{ IDC_SETTINGS_PORT_UDP, N_("") },
	{ IDC_SETTINGS_PORT_TLS, N_("") },
	{ IDC_SETTINGS_SOCKS5_IP, N_("Socks IP") },
	{ IDC_SETTINGS_SOCKS5_PORT, N_("Port") },
	{ IDC_SETTINGS_SOCKS5_USERNAME, N_("Login") },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, N_("Password") },
	{ IDC_SOCKS_RESOLVE, N_("Use SOCKS5 server to resolve host names") },
	{ IDC_SETTINGS_OUTGOING, N_("Outgoing connection settings") },
	{ 0, 0 }
};
/*
PropPage::Item NetworkPage::items[] = {
	{ IDC_OVERRIDE,		 },
	{ IDC_SOCKS_SERVER, SettingsManager::SOCKS_SERVER,	PropPage::T_STR },
	{ IDC_SOCKS_PORT,	SettingsManager::SOCKS_PORT,	PropPage::T_INT },
	{ IDC_SOCKS_USER,	SettingsManager::SOCKS_USER,	PropPage::T_STR },
	{ IDC_SOCKS_PASSWORD, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR },
	{ IDC_SOCKS_RESOLVE, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};
*/

NetworkPage::NetworkPage(dwt::Widget* parent) : PropPage(parent) {
	createDialog(IDD_NETWORKPAGE);
	setHelpId(IDH_NETWORKPAGE);

	grid = addChild(Grid::Seed(2, 1));

	GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Incoming connection settings")));
	GridPtr gridIn = group->addChild(Grid::Seed(1, 2));

	GridPtr gridLeft = gridIn->addChild(Grid::Seed(7, 1));
	directIn = gridLeft->addChild(RadioButton::Seed(T_("Direct connection")));
	upnp = gridLeft->addChild(RadioButton::Seed(T_("Firewall with UPnP")));
	nat = gridLeft->addChild(RadioButton::Seed(T_("Firewall with manual port forwarding")));
	gridLeft->addChild(Label::Seed(T_("External / WAN IP")));
	items.push_back(Item(gridLeft->addChild(TextBox::Seed()), SettingsManager::EXTERNAL_IP, PropPage::T_STR));
	items.push_back(Item(gridLeft->addChild(CheckBox::Seed(T_("Don't allow hub/UPnP to override"))), SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL));
	passive = gridLeft->addChild(RadioButton::Seed(T_("Firewall (passive, last resort)")));

	GridPtr gridRight = gridIn->addChild(Grid::Seed(4, 2));
	gridRight->column(1).mode = dwt::GridInfo::FILL;

	gridRight->addChild(Label::Seed()); /// @todo empty label, figure out how to avoid it
	gridRight->addChild(Label::Seed(T_("Ports")));

	gridRight->addChild(Label::Seed(T_("TCP")));
	tcp = gridRight->addChild(TextBox::Seed());
	items.push_back(Item(tcp, SettingsManager::TCP_PORT, PropPage::T_INT));

	gridRight->addChild(Label::Seed(T_("UDP")));
	udp = gridRight->addChild(TextBox::Seed());
	items.push_back(Item(udp, SettingsManager::UDP_PORT, PropPage::T_INT));

	gridRight->addChild(Label::Seed(T_("TLS")));
	tls = gridRight->addChild(TextBox::Seed());
	items.push_back(Item(tls, SettingsManager::TLS_PORT, PropPage::T_INT));

	grid->addChild(Label::Seed(_T("2nd grid here...")));

	tcp->setNumbersOnly();
	udp->setNumbersOnly();
	tls->setNumbersOnly();

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

	/*
	switch(SETTING(OUTGOING_CONNECTIONS)) {
		case SettingsManager::OUTGOING_SOCKS5: socks5->setChecked(); break;
		default: directOut->setChecked(); break;
	}
	*/

	PropPage::read(items);

	fixControlsIn();
	fixControlsOut();

	directIn->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));
	upnp->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));
	nat->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));
	passive->onClicked(std::tr1::bind(&NetworkPage::fixControlsIn, this));

	/*
	directOut->onClicked(std::tr1::bind(&NetworkPage::fixControlsOut, this));
	socks5->onClicked(std::tr1::bind(&NetworkPage::fixControlsOut, this));
	*/

#define TEXTBOX_LIMIT(id) attachChild<TextBox>(id)->setTextLimit(250)
	/*
	TEXTBOX_LIMIT(IDC_SOCKS_SERVER);
	TEXTBOX_LIMIT(IDC_SOCKS_PORT);
	TEXTBOX_LIMIT(IDC_SOCKS_USER);
	TEXTBOX_LIMIT(IDC_SOCKS_PASSWORD);
	*/
#undef TEXTBOX_LIMIT

	/*
	attachChild<TextBox>(IDC_PORT_TCP);
	attachChild<TextBox>(IDC_PORT_UDP);
	attachChild<TextBox>(IDC_PORT_TLS);
	attachChild<TextBox>(IDC_EXTERNAL_IP);
	*/

	grid->setBounds(dwt::Point(0, 0), getClientAreaSize());
}

NetworkPage::~NetworkPage() {
}

void NetworkPage::layout() {
	grid->layout(grid->getBounds());
}

void NetworkPage::write()
{
	TCHAR tmp[1024];
	::GetDlgItemText(handle(), IDC_EXTERNAL_IP, tmp, 1024);
	tstring x = tmp;
	tstring::size_type i;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	setItemText(IDC_EXTERNAL_IP, x);

	::GetDlgItemText(handle(), IDC_SOCKS_SERVER, tmp, 1024);
	x = tmp;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	setItemText(IDC_SOCKS_SERVER, x);

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

	if(::IsDlgButtonChecked(handle(), IDC_SOCKS5))
		ct = SettingsManager::OUTGOING_SOCKS5;

	if(SETTING(OUTGOING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::OUTGOING_CONNECTIONS, ct);
		Socket::socksUpdated();
	}
}

void NetworkPage::fixControlsIn() {
	bool enable = directIn->getChecked() || upnp->getChecked() || nat->getChecked();

	/*
	::EnableWindow(::GetDlgItem(handle(), IDC_EXTERNAL_IP), enable);
	::EnableWindow(::GetDlgItem(handle(), IDC_OVERRIDE), enable);

	::EnableWindow(::GetDlgItem(handle(), IDC_PORT_TCP), enable);
	::EnableWindow(::GetDlgItem(handle(), IDC_PORT_UDP), enable);
	::EnableWindow(::GetDlgItem(handle(), IDC_PORT_TLS), enable);
	*/
}

void NetworkPage::fixControlsOut() {
	/*
	BOOL socks = ::IsDlgButtonChecked(handle(), IDC_SOCKS5);
	::EnableWindow(::GetDlgItem(handle(), IDC_SOCKS_SERVER), socks);
	::EnableWindow(::GetDlgItem(handle(), IDC_SOCKS_PORT), socks);
	::EnableWindow(::GetDlgItem(handle(), IDC_SOCKS_USER), socks);
	::EnableWindow(::GetDlgItem(handle(), IDC_SOCKS_PASSWORD), socks);
	::EnableWindow(::GetDlgItem(handle(), IDC_SOCKS_RESOLVE), socks);
	*/
}
