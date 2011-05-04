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
#include "FavHubProperties.h"

#include <dcpp/FavoriteManager.h>
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
description(0),
nick(0),
password(0),
userDescription(0),
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

	grid = addChild(Grid::Seed(4, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Hub")));
		grid->setWidget(group, 0, 0, 1, 2);

		GridPtr cur = group->addChild(Grid::Seed(3, 2));
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

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_FAVORITE_HUB_DESC);
		description = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		description->setText(Text::toT(entry->getDescription()));
		description->setHelpId(IDH_FAVORITE_HUB_DESC);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Identification (leave blank for defaults)")));
		grid->setWidget(group, 1, 0, 1, 2);

		GridPtr cur = group->addChild(Grid::Seed(3, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Nick")))->setHelpId(IDH_FAVORITE_HUB_NICK);
		nick = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		nick->setText(Text::toT(entry->getNick(false)));
		nick->onUpdated([this] { handleTextChanged(nick); });
		nick->setHelpId(IDH_FAVORITE_HUB_NICK);

		cur->addChild(Label::Seed(T_("Password")))->setHelpId(IDH_FAVORITE_HUB_PASSWORD);
		password = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		password->setPassword();
		password->setText(Text::toT(entry->getPassword()));
		password->onUpdated([this] { handleTextChanged(password); });
		password->setHelpId(IDH_FAVORITE_HUB_PASSWORD);

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_FAVORITE_HUB_USER_DESC);
		userDescription = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		userDescription->setText(Text::toT(entry->getUserDescription()));
		userDescription->setHelpId(IDH_FAVORITE_HUB_USER_DESC);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Group")));
		grid->setWidget(group, 2, 0, 1, 2);

		GridPtr cur = group->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;

		ComboBox::Seed seed = WinUtil::Seeds::Dialog::comboBox;
		seed.style |= CBS_SORT;
		groups = cur->addChild(seed);
		groups->setHelpId(IDH_FAVORITE_HUB_GROUP);

		ButtonPtr manage = cur->addChild(Button::Seed(T_("Manage &groups")));
		manage->setHelpId(IDH_FAVORITE_HUBS_MANAGE_GROUPS);
		manage->onClicked([this] { handleGroups(); });
	}

	WinUtil::addDlgButtons(grid,
		[this] { handleOKClicked(); },
		[this] { GCC_WTF->endDialog(IDCANCEL); });

	fillGroups();

	setText(T_("Favorite Hub Properties"));

	layout();
	centerWindow();

	return false;
}

void FavHubProperties::handleTextChanged(TextBoxPtr textBox) {
	tstring text = textBox->getText();
	bool update = false;

	// Strip ' '
	tstring::size_type i;
	while((i = text.find(' ')) != string::npos) {
		text.erase(i, 1);
		update = true;
	}

	if(update) {
		// Something changed; update window text without changing cursor pos
		long caretPos = textBox->getCaretPos() - 1;
		textBox->setText(text);
		textBox->setSelection(caretPos, caretPos);
	}
}

void FavHubProperties::handleOKClicked() {
	tstring addressText = address->getText();
	if(addressText.empty()) {
		dwt::MessageBox(this).show(T_("Hub address cannot be empty"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return;
	}
	entry->setServer(Text::fromT(addressText));
	entry->setName(Text::fromT(name->getText()));
	entry->setDescription(Text::fromT(description->getText()));
	entry->setNick(Text::fromT(nick->getText()));
	entry->setPassword(Text::fromT(password->getText()));
	entry->setUserDescription(Text::fromT(userDescription->getText()));
	entry->setGroup(Text::fromT(groups->getText()));
	FavoriteManager::getInstance()->save();
	endDialog(IDOK);
}

void FavHubProperties::handleGroups() {
	FavHubGroupsDlg(this, entry).run();

	HoldRedraw hold(groups);
	groups->clear();
	fillGroups();
}

void FavHubProperties::fillGroups() {
	const string& entryGroup = entry->getGroup();
	int sel = 0;

	groups->addValue(_T(""));

	const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
	for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i) {
		const string& name = i->first;
		auto pos = groups->addValue(Text::toT(name));
		if(!sel && name == entryGroup)
			sel = pos;
	}

	groups->setSelected(sel);
}
