/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#include "HubListsDlg.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/StringTokenizer.h>

HubListsDlg::HubListsDlg(dwt::Widget* parent) :
StringListDlg(parent, getHubLists())
{
}

int HubListsDlg::run() {
	int ret = StringListDlg::run();
	if(ret == IDOK) {
		SettingsManager::getInstance()->set(SettingsManager::HUBLIST_SERVERS,
			Text::fromT(Util::toString(_T(";"), getValues())));
	}
	return ret;
}

tstring HubListsDlg::getTitle() const {
	return T_("Configured Public Hub Lists");
}

tstring HubListsDlg::getEditTitle() const {
	return T_("Hublist");
}

tstring HubListsDlg::getEditDescription() const {
	return T_("Address");
}

unsigned HubListsDlg::getHelpId(HelpFields field) const {
	switch(field) {
		case HELP_DIALOG: return IDH_PUBLIC_HUB_LISTS;
		case HELP_EDIT_BOX: return IDH_PUBLIC_HUB_LISTS_EDIT_BOX;
		case HELP_LIST: return IDH_PUBLIC_HUB_LISTS_LIST;
		case HELP_ADD: return IDH_PUBLIC_HUB_LISTS_ADD;
		case HELP_MOVE_UP: return IDH_PUBLIC_HUB_LISTS_MOVE_UP;
		case HELP_MOVE_DOWN: return IDH_PUBLIC_HUB_LISTS_MOVE_DOWN;
		case HELP_EDIT: return IDH_PUBLIC_HUB_LISTS_EDIT;
		case HELP_REMOVE: return IDH_PUBLIC_HUB_LISTS_REMOVE;
	}
}

void HubListsDlg::add(const tstring& s) {
	StringTokenizer<tstring> t(s, ';');
	for(TStringIterC i = t.getTokens().begin(), iend = t.getTokens().end(); i != iend; ++i)
		if(!i->empty())
			insert(*i);
}

TStringList HubListsDlg::getHubLists() {
	TStringList ret;
	StringList lists(FavoriteManager::getInstance()->getHubLists());
	for(StringIterC i = lists.begin(), iend = lists.end(); i != iend; ++i)
		ret.push_back(Text::toT(*i));
	return ret;
}
