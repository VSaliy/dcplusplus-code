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

#include "GeneralPage.h"

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

GeneralPage::GeneralPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
nick(0),
connections(0)
{
	setHelpId(IDH_GENERALPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	auto group = grid->addChild(GroupBox::Seed(T_("Personal Information")));
	group->setHelpId(IDH_SETTINGS_GENERAL_PERSONAL_INFORMATION);

	{
		GridPtr grid = group->addChild(Grid::Seed(4, 2));
		grid->column(0).align = GridInfo::BOTTOM_RIGHT;
		grid->column(1).mode = GridInfo::FILL;

		grid->addChild(Label::Seed(T_("Nick")))->setHelpId(IDH_SETTINGS_GENERAL_NICK);
		nick = grid->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(nick, SettingsManager::NICK, PropPage::T_STR));
		nick->setHelpId(IDH_SETTINGS_GENERAL_NICK);

		grid->addChild(Label::Seed(T_("E-Mail")))->setHelpId(IDH_SETTINGS_GENERAL_EMAIL);
		TextBoxPtr box = grid->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::EMAIL, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_GENERAL_EMAIL);

		grid->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_SETTINGS_GENERAL_DESCRIPTION);
		box = grid->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::DESCRIPTION, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_GENERAL_DESCRIPTION);

		grid->addChild(Label::Seed(T_("Line speed (upload)")))->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);

		GridPtr cur = grid->addChild(Grid::Seed(1, 2));

		connections = cur->addChild(WinUtil::Seeds::Dialog::comboBox);
		connections->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);

		cur->addChild(Label::Seed(T_("MiBits/s")))->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);
	}

	PropPage::read(items);

	nick->onUpdated(std::bind(&GeneralPage::handleNickTextChanged, this));

	int selected = 0, j = 0;
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i, ++j) {
		connections->addValue(Text::toT(*i).c_str());
		if(selected == 0 && SETTING(UPLOAD_SPEED) == *i) {
			selected = j;
		}
	}
	connections->setSelected(selected);
}

GeneralPage::~GeneralPage() {
}

void GeneralPage::write() {
	PropPage::write(items);

	SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, Text::fromT(connections->getText()));
}

void GeneralPage::handleNickTextChanged() {
	tstring text = nick->getText();
	bool update = false;

	// Strip ' '
	tstring::size_type i;
	while((i = text.find(' ')) != string::npos) {
		text.erase(i, 1);
		update = true;
	}

	if(update) {
		// Something changed; update window text without changing cursor pos
		long caretPos = nick->getCaretPos() - 1;
		nick->setText(text);
		nick->setSelection(caretPos, caretPos);
	}
}
