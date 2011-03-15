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

#include "AppearancePage.h"

#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/Spinner.h>

#include <dcpp/SettingsManager.h>
#include <dcpp/File.h>

#include "WinUtil.h"

PropPage::ListItem AppearancePage::listItems[] = {
	{ SettingsManager::ALT_SORT_ORDER, N_("Sort all downloads first"), IDH_SETTINGS_APPEARANCE_ALT_SORT_ORDER },
	{ SettingsManager::MINIMIZE_TRAY, N_("Minimize to tray"), IDH_SETTINGS_APPEARANCE_MINIMIZE_TRAY },
	{ SettingsManager::ALWAYS_TRAY, N_("Always display tray icon"), IDH_SETTINGS_APPEARANCE_ALWAYS_TRAY },
	{ SettingsManager::TIME_STAMPS, N_("Show timestamps in chat by default"), IDH_SETTINGS_APPEARANCE_TIME_STAMPS },
	{ SettingsManager::STATUS_IN_CHAT, N_("View status messages in main chat"), IDH_SETTINGS_APPEARANCE_STATUS_IN_CHAT },
	{ SettingsManager::FILTER_MESSAGES, N_("Filter spam messages"), IDH_SETTINGS_APPEARANCE_FILTER_MESSAGES },
	{ SettingsManager::SHOW_JOINS, N_("Show joins / parts in chat by default"), IDH_SETTINGS_APPEARANCE_SHOW_JOINS },
	{ SettingsManager::FAV_SHOW_JOINS, N_("Only show joins / parts for favorite users"), IDH_SETTINGS_APPEARANCE_FAV_SHOW_JOINS },
	{ SettingsManager::SORT_FAVUSERS_FIRST, N_("Sort favorite users first"), IDH_SETTINGS_APPEARANCE_SORT_FAVUSERS_FIRST },
	{ SettingsManager::USE_SYSTEM_ICONS, N_("Use system icons when browsing files (slows browsing down a bit)"), IDH_SETTINGS_APPEARANCE_USE_SYSTEM_ICONS },
	{ SettingsManager::USE_OEM_MONOFONT, N_("Use OEM monospaced font for viewing text files"), IDH_SETTINGS_APPEARANCE_USE_OEM_MONOFONT },
	{ SettingsManager::GET_USER_COUNTRY, N_("Guess user country from IP"), IDH_SETTINGS_APPEARANCE_GET_USER_COUNTRY },
	{ 0, 0 }
};

AppearancePage::AppearancePage(dwt::Widget* parent) :
PropPage(parent, 5, 2),
options(0),
languages(0)
{
	setHelpId(IDH_APPEARANCEPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Options")));
		grid->setWidget(group, 0, 0, 1, 2);
		options = group->addChild(WinUtil::Seeds::Dialog::optionsTable);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Default away message")));
		group->setHelpId(IDH_SETTINGS_APPEARANCE_DEFAULT_AWAY_MESSAGE);

		TextBox::Seed seed = WinUtil::Seeds::Dialog::textBox;
		seed.style |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;
		TextBoxPtr box = group->addChild(seed);
		box->setHelpId(IDH_SETTINGS_APPEARANCE_DEFAULT_AWAY_MESSAGE);
		items.push_back(Item(box, SettingsManager::DEFAULT_AWAY_MESSAGE, PropPage::T_STR));
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Set timestamps")));
		group->setHelpId(IDH_SETTINGS_APPEARANCE_TIME_STAMPS_FORMAT);

		TextBoxPtr box = group->addChild(WinUtil::Seeds::Dialog::textBox);
		box->setHelpId(IDH_SETTINGS_APPEARANCE_TIME_STAMPS_FORMAT);
		items.push_back(Item(box, SettingsManager::TIME_STAMPS_FORMAT, PropPage::T_STR));
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Height of the message editing box")));
		grid->setWidget(group, 2, 0, 1, 2);
		group->setHelpId(IDH_SETTINGS_APPEARANCE_MESSAGE_LINES);

		GridPtr cur = group->addChild(Grid::Seed(1, 5));
		cur->column(1).size = 40;
		cur->column(1).mode = GridInfo::STATIC;
		cur->column(3).size = 40;
		cur->column(3).mode = GridInfo::STATIC;

		cur->addChild(Label::Seed(T_("Keep it between")));

		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MIN_MESSAGE_LINES, PropPage::T_INT_WITH_SPIN));
		SpinnerPtr spin = cur->addChild(Spinner::Seed(1, UD_MAXVAL, box));
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("and")));

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MAX_MESSAGE_LINES, PropPage::T_INT_WITH_SPIN));
		spin = cur->addChild(Spinner::Seed(1, UD_MAXVAL, box));
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("lines")));
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Language")));
		grid->setWidget(group, 3, 0, 1, 2);
		group->setHelpId(IDH_SETTINGS_APPEARANCE_LANGUAGE);

		languages = group->addChild(WinUtil::Seeds::Dialog::comboBox);
		languages->setHelpId(IDH_SETTINGS_APPEARANCE_LANGUAGE);
	}

	LabelPtr label = grid->addChild(Label::Seed(T_("Note; most of these options require that you restart DC++")));
	label->setHelpId(IDH_SETTINGS_APPEARANCE_REQUIRES_RESTART);
	grid->setWidget(label, 4, 0, 1, 2);

	PropPage::read(items);
	PropPage::read(listItems, options);

	typedef map<string, string, noCaseStringLess> lang_map;
	lang_map langs;
	langs["en"] = "English (United States)";

	StringList dirs = File::findFiles(Util::getPath(Util::PATH_LOCALE), "*");
	for(StringList::const_iterator i = dirs.begin(); i != dirs.end(); ++i) {
		string dir = *i + "LC_MESSAGES" PATH_SEPARATOR_STR;
		StringList files = File::findFiles(dir, "*.mo");
		if(find(files.begin(), files.end(), dir + "dcpp.mo") == files.end() && find(files.begin(), files.end(), dir + "dcpp-win32.mo") == files.end()) {
			continue;
		}

		string text = Util::getLastDir(*i);
		try {
			langs[text] = File(*i + "name.txt", File::READ, File::OPEN).read();
		} catch(const FileException&) {
			langs[text] = "";
		}
	}

	languages->addValue(T_("Default"));

	int selected = 0, j = 1;
	const string& cur = SETTING(LANGUAGE);
	for(lang_map::const_iterator i = langs.begin(); i != langs.end(); ++i, ++j) {
		string text = i->first;
		if(!i->second.empty())
			text += ": " + i->second;
		languages->addValue(Text::toT(text));

		if(selected == 0 && (i->first == cur || (i->first == "en" && cur == "C"))) {
			selected = j;
		}
	}

	languages->setSelected(selected);
}

AppearancePage::~AppearancePage() {
}

void AppearancePage::write()
{
	PropPage::write(items);
	PropPage::write(options);

	tstring lang = languages->getText();
	size_t col = lang.find(':');
	if(col != string::npos)
		lang = lang.substr(0, col);

	if(lang == T_("Default")) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE, "");
	} else if(lang == _T("en")) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE, "C");
	} else {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE, Text::fromT(lang));
	}
}
