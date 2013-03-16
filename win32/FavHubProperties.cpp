/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "FavHubProperties.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/HubEntry.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"
#include "FavHubGroupsDlg.h"
#include "HoldRedraw.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

FavHubProperties::FavHubProperties(dwt::Widget* parent, FavoriteHubEntry *_entry) :
GridDialog(parent, 320, DS_CONTEXTHELP),
name(0),
address(0),
hubDescription(0),
nick(0),
password(0),
description(0),
email(0),
userIp(0),
showJoins(0),
favShowJoins(0),
logMainChat(0),
groups(0),
entry(_entry)
{
	onInitDialog([this] { return handleInitDialog(); });
	onHelp(&WinUtil::help);
}

FavHubProperties::~FavHubProperties() {
}

bool FavHubProperties::handleInitDialog() {
	setHelpId(IDH_FAVORITE_HUB);

	grid = addChild(Grid::Seed(5, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Hub")));
		grid->setWidget(group, 0, 0, 1, 2);

		auto cur = group->addChild(Grid::Seed(3, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Name")))->setHelpId(IDH_FAVORITE_HUB_NAME);
		name = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		name->setText(Text::toT(entry->getName()));
		name->setHelpId(IDH_FAVORITE_HUB_NAME);

		cur->addChild(Label::Seed(T_("Address")))->setHelpId(IDH_FAVORITE_HUB_ADDRESS);
		address = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		address->setText(Text::toT(entry->getServer()));
		address->setHelpId(IDH_FAVORITE_HUB_ADDRESS);
		WinUtil::preventSpaces(address);

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_FAVORITE_HUB_DESC);
		hubDescription = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		hubDescription->setText(Text::toT(entry->getHubDescription()));
		hubDescription->setHelpId(IDH_FAVORITE_HUB_DESC);
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Identification (leave blank for defaults)")));
		grid->setWidget(group, 1, 0, 1, 2);

		auto cur = group->addChild(Grid::Seed(5, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Nick")))->setHelpId(IDH_FAVORITE_HUB_NICK);
		nick = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		nick->setText(Text::toT(entry->get(HubSettings::Nick)));
		nick->setHelpId(IDH_FAVORITE_HUB_NICK);
		WinUtil::preventSpaces(nick);

		cur->addChild(Label::Seed(T_("Password")))->setHelpId(IDH_FAVORITE_HUB_PASSWORD);
		password = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		password->setPassword();
		password->setText(Text::toT(entry->getPassword()));
		password->setHelpId(IDH_FAVORITE_HUB_PASSWORD);
		WinUtil::preventSpaces(password);

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_FAVORITE_HUB_USER_DESC);
		description = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		description->setText(Text::toT(entry->get(HubSettings::Description)));
		description->setHelpId(IDH_FAVORITE_HUB_USER_DESC);

		cur->addChild(Label::Seed(T_("Email")))->setHelpId(IDH_FAVORITE_HUB_EMAIL);
		email = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		email->setText(Text::toT(entry->get(HubSettings::Email)));
		email->setHelpId(IDH_FAVORITE_HUB_EMAIL);

		cur->addChild(Label::Seed(T_("IP")))->setHelpId(IDH_FAVORITE_HUB_USER_IP);
		userIp = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		userIp->setText(Text::toT(entry->get(HubSettings::UserIp)));
		userIp->setHelpId(IDH_FAVORITE_HUB_USER_IP);
		WinUtil::preventSpaces(userIp);
	}

	{
		auto cur = grid->addChild(Grid::Seed(3, 2));
		grid->setWidget(cur, 2, 0, 1, 2);
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;

		cur->addChild(Label::Seed(T_("Show joins / parts in chat by default")));
		showJoins = cur->addChild(WinUtil::Seeds::Dialog::comboBox);
		WinUtil::fillTriboolCombo(showJoins);
		showJoins->setSelected(toInt(entry->get(HubSettings::ShowJoins)));

		cur->addChild(Label::Seed(T_("Only show joins / parts for favorite users")));
		favShowJoins = cur->addChild(WinUtil::Seeds::Dialog::comboBox);
		WinUtil::fillTriboolCombo(favShowJoins);
		favShowJoins->setSelected(toInt(entry->get(HubSettings::FavShowJoins)));

		cur->addChild(Label::Seed(T_("Log main chat")));
		logMainChat = cur->addChild(WinUtil::Seeds::Dialog::comboBox);
		WinUtil::fillTriboolCombo(logMainChat);
		logMainChat->setSelected(toInt(entry->get(HubSettings::LogMainChat)));
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Group")));
		grid->setWidget(group, 3, 0, 1, 2);

		auto cur = group->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;

		auto seed = WinUtil::Seeds::Dialog::comboBox;
		seed.style |= CBS_SORT;
		groups = cur->addChild(seed);
		groups->setHelpId(IDH_FAVORITE_HUB_GROUP);

		auto manage = cur->addChild(Button::Seed(T_("Manage &groups")));
		manage->setHelpId(IDH_FAVORITE_HUBS_MANAGE_GROUPS);
		manage->onClicked([this] { handleGroups(); });
	}

	WinUtil::addDlgButtons(grid,
		[this] { handleOKClicked(); },
		[this] { endDialog(IDCANCEL); });

	fillGroups();

	setText(T_("Favorite Hub Properties"));

	layout();
	centerWindow();

	return false;
}

void FavHubProperties::handleOKClicked() {
	tstring addressText = address->getText();
	if(addressText.empty()) {
		dwt::MessageBox(this).show(T_("Hub address cannot be empty"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return;
	}
	entry->setServer(Text::fromT(addressText));
	entry->setName(Text::fromT(name->getText()));
	entry->setHubDescription(Text::fromT(hubDescription->getText()));
	entry->get(HubSettings::Nick) = Text::fromT(nick->getText());
	entry->setPassword(Text::fromT(password->getText()));
	entry->get(HubSettings::Description) = Text::fromT(description->getText());
	entry->get(HubSettings::Email) = Text::fromT(email->getText());
	entry->get(HubSettings::UserIp) = Text::fromT(userIp->getText());
	entry->get(HubSettings::ShowJoins) = to3bool(showJoins->getSelected());
	entry->get(HubSettings::FavShowJoins) = to3bool(favShowJoins->getSelected());
	entry->get(HubSettings::LogMainChat) = to3bool(logMainChat->getSelected());
	entry->setGroup(Text::fromT(groups->getText()));
	FavoriteManager::getInstance()->save();
	endDialog(IDOK);
}

void FavHubProperties::handleGroups() {
	FavHubGroupsDlg(this, entry).run();

	HoldRedraw hold { groups };
	groups->clear();
	fillGroups();
}

void FavHubProperties::fillGroups() {
	const string& entryGroup = entry->getGroup();
	bool needSel = true;

	groups->addValue(_T(""));

	for(auto& i: FavoriteManager::getInstance()->getFavHubGroups()) {
		const string& name = i.first;
		auto pos = groups->addValue(Text::toT(name));
		if(needSel && name == entryGroup) {
			groups->setSelected(pos);
			needSel = false;
		}
	}

	if(needSel)
		groups->setSelected(0);
}
