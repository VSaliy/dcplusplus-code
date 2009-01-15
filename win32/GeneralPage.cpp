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

#include "GeneralPage.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_SETTINGS_PERSONAL_INFORMATION, IDH_SETTINGS_GENERAL_PERSONAL_INFORMATION },
	{ IDC_SETTINGS_NICK, IDH_SETTINGS_GENERAL_NICK },
	{ IDC_NICK, IDH_SETTINGS_GENERAL_NICK },
	{ IDC_SETTINGS_EMAIL, IDH_SETTINGS_GENERAL_EMAIL },
	{ IDC_EMAIL, IDH_SETTINGS_GENERAL_EMAIL },
	{ IDC_SETTINGS_DESCRIPTION, IDH_SETTINGS_GENERAL_DESCRIPTION },
	{ IDC_DESCRIPTION, IDH_SETTINGS_GENERAL_DESCRIPTION },
	{ IDC_SETTINGS_UPLOAD_LINE_SPEED, IDH_SETTINGS_GENERAL_CONNECTION },
	{ IDC_CONNECTION, IDH_SETTINGS_GENERAL_CONNECTION },
	{ IDC_SETTINGS_MEBIBITS, IDH_SETTINGS_GENERAL_CONNECTION },
	{ 0, 0 }
};
*/

GeneralPage::GeneralPage(dwt::Widget* parent) : PropPage(parent), nick(0) {
	createDialog(IDD_GENERALPAGE);
	setHelpId(IDH_GENERALPAGE);

	group = addChild(GroupBox::Seed(T_("Personal Information")));

	GridPtr grid = group->addChild(Grid::Seed(4, 3));
	grid->column(1).mode = dwt::GridInfo::FILL;

	grid->addChild(Label::Seed(T_("Nick")));

	nick = grid->addChild(WinUtil::Seeds::Dialog::TextBox);
	grid->setWidget(nick, 0, 1, 1, 2);
	items.push_back(Item(nick, SettingsManager::NICK, PropPage::T_STR));

	grid->addChild(Label::Seed(T_("E-Mail")));

	TextBoxPtr box = grid->addChild(WinUtil::Seeds::Dialog::TextBox);
	grid->setWidget(box, 1, 1, 1, 2);
	items.push_back(Item(box, SettingsManager::EMAIL, PropPage::T_STR));

	grid->addChild(Label::Seed(T_("Description")));

	box = grid->addChild(WinUtil::Seeds::Dialog::TextBox);
	box->setTextLimit(35);
	grid->setWidget(box, 2, 1, 1, 2);
	items.push_back(Item(box, SettingsManager::DESCRIPTION, PropPage::T_STR));

	grid->addChild(Label::Seed(T_("Line speed (upload)")));

	ComboBoxPtr connections = grid->addChild(ComboBox::Seed());
	items.push_back(Item(connections, SettingsManager::UPLOAD_SPEED, PropPage::T_STR));

	grid->addChild(Label::Seed(T_("MiBits/s")));

	PropPage::read(items);

	nick->setTextLimit(35);
	nick->onUpdated(std::tr1::bind(&GeneralPage::handleNickTextChanged, this));

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

void GeneralPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point groupSize = group->getPreferedSize();
	group->layout(dwt::Rectangle(7, 4, getClientAreaSize().x - 14, groupSize.y));
}

void GeneralPage::write() {
	PropPage::write(items);
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
