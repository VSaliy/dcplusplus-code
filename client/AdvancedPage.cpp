/* 
 * Copyright (C) 2001 Jacek Sieka, j_s@telia.com
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
#include "dcplusplus.h"
#include "AdvancedPage.h"
#include "SettingsManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::Item AdvancedPage::items[] = {
	{ IDC_ROLLBACK, SettingsManager::ROLLBACK, PropPage::T_INT }, 
	{ IDC_CLVERSION, SettingsManager::CLIENTVERSION, PropPage::T_STR }, 
	{ IDC_AUTOFOLLOW, SettingsManager::AUTO_FOLLOW, PropPage::T_BOOL },
	{ IDC_CLEARSEARCH, SettingsManager::CLEAR_SEARCH, PropPage::T_BOOL },
	{ IDC_REMOVENOTAVAILABLE, SettingsManager::REMOVE_NOT_AVAILABLE, PropPage::T_BOOL }, 
	{ IDC_REMOVEFINISHED, SettingsManager::REMOVE_FINISHED, PropPage::T_BOOL },
	{ IDC_OPENPUBLIC, SettingsManager::OPEN_PUBLIC, PropPage::T_BOOL },
	{ IDC_OPENQUEUE, SettingsManager::OPEN_QUEUE, PropPage::T_BOOL },
	{ IDC_AUTOSEARCH, SettingsManager::AUTO_SEARCH, PropPage::T_BOOL },
	{ IDC_IGNOREOFFLINE, SettingsManager::IGNORE_OFFLINE, PropPage::T_BOOL },
	{ IDC_POPUPOFFLINE, SettingsManager::POPUP_OFFLINE, PropPage::T_BOOL },
	{ IDC_REMOVEDUPES, SettingsManager::REMOVE_DUPES, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

AdvancedPage::AdvancedPage(SettingsManager *s) : PropPage(s)
{
}

AdvancedPage::~AdvancedPage()
{
}

LRESULT AdvancedPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::read((HWND)*this, items);

	// Do specialized reading here
	return TRUE;
}

void AdvancedPage::write()
{
	PropPage::write((HWND)*this, items);

	// Do specialized writing here
	// settings->set(XX, YY);
}

/**
 * @file AdvancedPage.cpp
 * $Id: AdvancedPage.cpp,v 1.11 2002/03/10 22:41:08 arnetheduck Exp $
 * @if LOG
 * $Log: AdvancedPage.cpp,v $
 * Revision 1.11  2002/03/10 22:41:08  arnetheduck
 * Working on internationalization...
 *
 * Revision 1.10  2002/02/28 00:10:47  arnetheduck
 * Some fixes to the new user model
 *
 * Revision 1.9  2002/02/25 15:39:28  arnetheduck
 * Release 0.154, lot of things fixed...
 *
 * Revision 1.8  2002/02/07 17:25:28  arnetheduck
 * many bugs fixed, time for 0.152 I think
 *
 * Revision 1.7  2002/02/03 01:06:56  arnetheduck
 * More bugfixes and some minor changes
 *
 * Revision 1.6  2002/02/01 02:00:24  arnetheduck
 * A lot of work done on the new queue manager, hopefully this should reduce
 * the number of crashes...
 *
 * Revision 1.5  2002/01/26 14:59:22  arnetheduck
 * Fixed disconnect crash
 *
 * Revision 1.4  2002/01/26 12:52:51  arnetheduck
 * More minor fixes
 *
 * @endif
 */

