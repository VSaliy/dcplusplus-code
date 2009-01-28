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

#include "FavHubProperties.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>
#include "WinUtil.h"

FavHubProperties::FavHubProperties(dwt::Widget* parent, FavoriteHubEntry *_entry) :
WidgetFactory<dwt::ModalDialog>(parent),
grid(0),
name(0),
address(0),
description(0),
nick(0),
password(0),
userDescription(0),
entry(_entry)
{
	onInitDialog(std::tr1::bind(&FavHubProperties::handleInitDialog, this));
}

FavHubProperties::~FavHubProperties() {
}

int FavHubProperties::run() {
	createDialog(IDD_FAVORITEHUB);
	return show();
}

bool FavHubProperties::handleInitDialog() {
	setHelpId(IDH_FAVORITE_HUB);

	grid = addChild(Grid::Seed(3, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Hub")));
		grid->setWidget(group, 0, 0, 1, 2);

		GridPtr cur = group->addChild(Grid::Seed(3, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Name")))->setHelpId(IDH_FAVORITE_HUB_NAME);
		name = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		name->setText(Text::toT(entry->getName()));
		name->setSelection();
		name->setHelpId(IDH_FAVORITE_HUB_NAME);

		cur->addChild(Label::Seed(T_("Address")))->setHelpId(IDH_FAVORITE_HUB_ADDRESS);
		address = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		address->setText(Text::toT(entry->getServer()));
		address->setHelpId(IDH_FAVORITE_HUB_ADDRESS);

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_FAVORITE_HUB_DESC);
		description = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
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
		nick = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		nick->setTextLimit(35);
		nick->setText(Text::toT(entry->getNick(false)));
		nick->onUpdated(std::tr1::bind(&FavHubProperties::handleTextChanged, this, nick));
		nick->setHelpId(IDH_FAVORITE_HUB_NICK);

		cur->addChild(Label::Seed(T_("Password")))->setHelpId(IDH_FAVORITE_HUB_PASSWORD);
		password = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		password->setPassword();
		password->setText(Text::toT(entry->getPassword()));
		password->onUpdated(std::tr1::bind(&FavHubProperties::handleTextChanged, this, password));
		password->setHelpId(IDH_FAVORITE_HUB_PASSWORD);

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_FAVORITE_HUB_USER_DESC);
		userDescription = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		userDescription->setTextLimit(35);
		userDescription->setText(Text::toT(entry->getUserDescription()));
		userDescription->setHelpId(IDH_FAVORITE_HUB_USER_DESC);
	}

	WinUtil::addDlgButtons(grid,
		std::tr1::bind(&FavHubProperties::handleOKClicked, this),
		std::tr1::bind(&FavHubProperties::endDialog, this, IDCANCEL));

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
		createMessageBox().show(T_("Hub address cannot be empty"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONEXCLAMATION);
		return;
	}
	entry->setServer(Text::fromT(addressText));
	entry->setName(Text::fromT(name->getText()));
	entry->setDescription(Text::fromT(description->getText()));
	entry->setNick(Text::fromT(nick->getText()));
	entry->setPassword(Text::fromT(password->getText()));
	entry->setUserDescription(Text::fromT(userDescription->getText()));
	FavoriteManager::getInstance()->save();
	endDialog(IDOK);
}

void FavHubProperties::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
