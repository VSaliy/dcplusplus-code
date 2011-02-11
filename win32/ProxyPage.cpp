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

#include "ProxyPage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/Socket.h>
#include "WinUtil.h"
#include <dwt/util/win32/Version.h>

ProxyPage::ProxyPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
directOut(0),
socks5(0),
socksSettings(0),
socksServer(0)
{
	setHelpId(IDH_PROXYPAGE);

	grid = addChild(Grid::Seed(1, 1));
	grid->column(0).mode = GridInfo::FILL;

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Proxy settings for outgoing connections")))->addChild(Grid::Seed(1, 2));
		cur->row(0).align = GridInfo::TOP_LEFT;
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		{
			GridPtr outChoice = cur->addChild(Grid::Seed(2, 1));

			directOut = outChoice->addChild(RadioButton::Seed(T_("Direct connection")));
			directOut->setHelpId(IDH_SETTINGS_PROXY_DIRECT_OUT);

			socks5 = outChoice->addChild(RadioButton::Seed(T_("SOCKS5")));
			socks5->setHelpId(IDH_SETTINGS_PROXY_SOCKS5);
		}

		socksSettings = cur->addChild(GroupBox::Seed(T_("SOCKS5")));
		cur = socksSettings->addChild(Grid::Seed(5, 2));
		cur->addChild(Label::Seed(T_("IP")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_SERVER);
		cur->addChild(Label::Seed(T_("Port")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PORT);
		socksServer = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(socksServer, SettingsManager::SOCKS_SERVER, PropPage::T_STR));
		socksServer->setHelpId(IDH_SETTINGS_PROXY_SOCKS_SERVER);
		socksServer->setTextLimit(250);
		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::SOCKS_PORT, PropPage::T_INT));
		box->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PORT);
		box->setTextLimit(250);
		cur->addChild(Label::Seed(T_("Login")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_USER);
		cur->addChild(Label::Seed(T_("Password")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PASSWORD);
		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::SOCKS_USER, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_PROXY_SOCKS_USER);
		box->setTextLimit(250);
		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PASSWORD);
		box->setTextLimit(250);
		CheckBoxPtr socksResolve = cur->addChild(CheckBox::Seed(T_("Use SOCKS5 server to resolve host names")));
		cur->setWidget(socksResolve, 4, 0, 1, 2);
		items.push_back(Item(socksResolve, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL));
		socksResolve->setHelpId(IDH_SETTINGS_PROXY_SOCKS_RESOLVE);
	}

	switch(SETTING(OUTGOING_CONNECTIONS)) {
		case SettingsManager::OUTGOING_SOCKS5: socks5->setChecked(); break;
		default: directOut->setChecked(); break;
	}

	PropPage::read(items);

	fixControlsOut();
	directOut->onClicked(std::bind(&ProxyPage::fixControlsOut, this));
	socks5->onClicked(std::bind(&ProxyPage::fixControlsOut, this));
}

ProxyPage::~ProxyPage() {
}

void ProxyPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point gridSize = grid->getPreferredSize();
	grid->layout(dwt::Rectangle(7, 4, getClientSize().x - 14, gridSize.y));
}

void ProxyPage::write()
{
	tstring x = socksServer->getText();
	tstring::size_type i;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	socksServer->setText(x);

	PropPage::write(items);

	int ct = SettingsManager::OUTGOING_DIRECT;

	if(socks5->getChecked())
		ct = SettingsManager::OUTGOING_SOCKS5;

	if(SETTING(OUTGOING_CONNECTIONS) != ct) {
		SettingsManager::getInstance()->set(SettingsManager::OUTGOING_CONNECTIONS, ct);
		Socket::socksUpdated();
	}
}

void ProxyPage::fixControlsOut() {
	socksSettings->setEnabled(socks5->getChecked());
}
