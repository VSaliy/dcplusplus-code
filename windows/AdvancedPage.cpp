/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "AdvancedPage.h"
#include "CommandDlg.h"

#include "../client/SettingsManager.h"
#include "../client/HubManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::Item AdvancedPage::items[] = {
	{ IDC_ROLLBACK, SettingsManager::ROLLBACK, PropPage::T_INT }, 
	{ IDC_CLVERSION, SettingsManager::CLIENTVERSION, PropPage::T_STR }, 
	{ IDC_BUFFERSIZE, SettingsManager::BUFFER_SIZE, PropPage::T_INT },
	{ IDC_MAX_TAB_ROWS, SettingsManager::MAX_TAB_ROWS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

AdvancedPage::ListItem AdvancedPage::listItems[] = {
	{ SettingsManager::AUTO_AWAY, ResourceManager::SETTINGS_ADVANCED_AUTO_AWAY },
	{ SettingsManager::AUTO_FOLLOW, ResourceManager::SETTINGS_ADVANCED_AUTO_FOLLOW },
	{ SettingsManager::CLEAR_SEARCH, ResourceManager::SETTINGS_ADVANCED_CLEAR_SEARCH },
	{ SettingsManager::OPEN_PUBLIC, ResourceManager::SETTINGS_ADVANCED_OPEN_PUBLIC },
	{ SettingsManager::OPEN_QUEUE, ResourceManager::SETTINGS_ADVANCED_OPEN_QUEUE },
	{ SettingsManager::AUTO_SEARCH, ResourceManager::SETTINGS_ADVANCED_AUTO_SEARCH },
	{ SettingsManager::POPUP_PMS, ResourceManager::SETTINGS_ADVANCED_POPUP_PMS },
	{ SettingsManager::IGNORE_OFFLINE, ResourceManager::SETTINGS_ADVANCED_IGNORE_OFFLINE },
	{ SettingsManager::POPUP_OFFLINE, ResourceManager::SETTINGS_ADVANCED_POPUP_OFFLINE },
	{ SettingsManager::REMOVE_DUPES, ResourceManager::SETTINGS_ADVANCED_REMOVE_DUPES },
	{ SettingsManager::URL_HANDLER, ResourceManager::SETTINGS_ADVANCED_URL_HANDLER },
	{ SettingsManager::SMALL_SEND_BUFFER, ResourceManager::SETTINGS_ADVANCED_SMALL_SEND_BUFFER },
	{ SettingsManager::KEEP_LISTS, ResourceManager::SETTINGS_ADVANCED_KEEP_LISTS },
	{ SettingsManager::AUTO_KICK, ResourceManager::SETTINGS_ADVANCED_AUTO_KICK },
	{ SettingsManager::SHOW_PROGRESS_BARS, ResourceManager::SETTINGS_ADVANCED_SHOW_PROGRESS_BARS },
	{ SettingsManager::SFV_CHECK, ResourceManager::SETTINGS_ADVANCED_SFV_CHECK },
	{ SettingsManager::AUTO_UPDATE_LIST, ResourceManager::SETTINGS_ADVANCED_AUTO_UPDATE_LIST },
	{ 0, ResourceManager::SETTINGS_ADVANCED_AUTO_AWAY }
};

LRESULT AdvancedPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	CRect rc;

	ctrlCommands.Attach(GetDlgItem(IDC_MENU_ITEMS));
	ctrlCommands.GetClientRect(rc);

	ctrlCommands.InsertColumn(0, CSTRING(SETTINGS_ADVANCED_NAME), LVCFMT_LEFT, rc.Width()/5, 0);
	ctrlCommands.InsertColumn(1, CSTRING(SETTINGS_ADVANCED_COMMAND), LVCFMT_LEFT, rc.Width()*2 / 5, 1);
	ctrlCommands.InsertColumn(2, CSTRING(HUB), LVCFMT_LEFT, rc.Width() / 5, 2);
	ctrlCommands.InsertColumn(3, CSTRING(NICK), LVCFMT_LEFT, rc.Width() / 5, 3);

	if(BOOLSETTING(FULL_ROW_SELECT))
		ctrlCommands.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	UserCommand::List& ul = HubManager::getInstance()->getUserCommands();
	StringList cols;
	for(UserCommand::Iter j = ul.begin(); j != ul.end(); ++j) {
		cols.push_back(j->getName());
		cols.push_back(j->getCommand());
		cols.push_back(j->getHub());
		cols.push_back(j->getNick());
		ctrlCommands.insert(cols);
		cols.clear();
	}

	// Do specialized reading here
	return TRUE;
}

LRESULT AdvancedPage::onAddMenu(WORD , WORD , HWND , BOOL& ) {
	CommandDlg dlg;

	if(dlg.DoModal() == IDOK) {
		StringList lst;
		lst.push_back(dlg.name);
		lst.push_back(dlg.command);
		lst.push_back(dlg.hub);
		lst.push_back(dlg.nick);
		ctrlCommands.insert(lst);
	}
	return 0;
}

#define BUFLEN 256
LRESULT AdvancedPage::onChangeMenu(WORD , WORD , HWND , BOOL& ) {
	if(ctrlCommands.GetSelectedCount() == 1) {
		int sel = ctrlCommands.GetSelectedIndex();
		char buf[BUFLEN];
		CommandDlg dlg;
		ctrlCommands.GetItemText(sel, 0, buf, BUFLEN);
		dlg.name = buf;
		ctrlCommands.GetItemText(sel, 1, buf, BUFLEN);
		dlg.command = buf;
		ctrlCommands.GetItemText(sel, 2, buf, BUFLEN);
		dlg.hub = buf;
		ctrlCommands.GetItemText(sel, 3, buf, BUFLEN);
		dlg.nick = buf;

		if(dlg.DoModal() == IDOK) {
			ctrlCommands.SetItemText(sel, 0, dlg.name.c_str());
			ctrlCommands.SetItemText(sel, 1, dlg.command.c_str());
			ctrlCommands.SetItemText(sel, 2, dlg.hub.c_str());
			ctrlCommands.SetItemText(sel, 3, dlg.nick.c_str());
		}
	}
	return 0;
}

void AdvancedPage::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	int items = ctrlCommands.GetItemCount();
#define BUFLEN 256
	char buf[BUFLEN];
	string name, command, hub, pm;
	UserCommand::List& ul = HubManager::getInstance()->getUserCommands();
	ul.clear();
	for(int i = 0; i < items; ++i) {
		ctrlCommands.GetItemText(i, 0, buf, BUFLEN);
		name = buf;
		ctrlCommands.GetItemText(i, 1, buf, BUFLEN);
		command = buf;
		ctrlCommands.GetItemText(i, 2, buf, BUFLEN);
		hub = buf;
		ctrlCommands.GetItemText(i, 3, buf, BUFLEN);
		pm = buf;
		ul.push_back(UserCommand(name, command, hub, pm));
	}
	HubManager::getInstance()->save();

	ctrlCommands.Detach();
}

/**
 * @file AdvancedPage.cpp
 * $Id: AdvancedPage.cpp,v 1.10 2003/03/26 08:47:39 arnetheduck Exp $
 */

