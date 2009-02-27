/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#include "PropPage.h"

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

PropPage::PropPage(dwt::Widget* parent) : WidgetFactory<dwt::ModelessDialog>(parent) {
}

PropPage::~PropPage() {
}

void PropPage::read(const ItemList& items) {
	SettingsManager* settings = SettingsManager::getInstance();

	for(ItemList::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		switch(i->type)
		{
		case T_STR:
			if(!settings->isDefault(i->setting)) {
				static_cast<TextBoxPtr>(i->widget)->setText(Text::toT(settings->get((SettingsManager::StrSetting)i->setting)));
			}
			break;
		case T_INT:
			if(!settings->isDefault(i->setting)) {
				static_cast<TextBoxPtr>(i->widget)->setText(Text::toT(Util::toString(settings->get((SettingsManager::IntSetting)i->setting))));
			}
			break;
		case T_INT_WITH_SPIN:
			static_cast<TextBoxPtr>(i->widget)->setText(Text::toT(Util::toString(settings->get((SettingsManager::IntSetting)i->setting))));
			break;
		case T_BOOL:
			static_cast<CheckBoxPtr>(i->widget)->setChecked(settings->getBool((SettingsManager::IntSetting)i->setting));
			break;
		}
	}
}

void PropPage::read(const ListItem* listItems, TablePtr list) {
	dcassert(listItems && list);

	list->createColumns(TStringList(1));

	SettingsManager* settings = SettingsManager::getInstance();
	for(size_t i = 0; listItems[i].setting != 0; ++i) {
		TStringList row;
		row.push_back(T_(listItems[i].desc));
		list->setChecked(list->insert(row), settings->getBool(SettingsManager::IntSetting(listItems[i].setting), true));
	}

	list->setColumnWidth(0, LVSCW_AUTOSIZE);

	list->onHelp(std::tr1::bind(&PropPage::handleListHelp, this, _1, _2, listItems, list));
}

void PropPage::write(const ItemList& items) {
	SettingsManager* settings = SettingsManager::getInstance();

	for(ItemList::const_iterator i = items.begin(); i != items.end(); ++i) {
		switch(i->type) {
		case T_STR:
				settings->set((SettingsManager::StrSetting)i->setting, Text::fromT(static_cast<TextBoxPtr>(i->widget)->getText()));
				break;
		case T_INT:
		case T_INT_WITH_SPIN:
			settings->set((SettingsManager::IntSetting)i->setting, Text::fromT(static_cast<TextBoxPtr>(i->widget)->getText()));
			break;
		case T_BOOL:
			settings->set((SettingsManager::IntSetting)i->setting, static_cast<CheckBoxPtr>(i->widget)->getChecked());
			break;
		}
	}
}

void PropPage::write(const ListItem* listItems, TablePtr list) {
	dcassert(listItems && list);
	SettingsManager* settings = SettingsManager::getInstance();
	for(size_t i = 0; listItems[i].setting != 0; ++i)
		settings->set(SettingsManager::IntSetting(listItems[i].setting), list->isChecked(i));
}

void PropPage::handleBrowseDir(const Item& i) {
	TextBoxPtr box = static_cast<TextBoxPtr>(i.widget);
	tstring dir = box->getText();
	if(dir.empty())
		dir = Text::toT(SettingsManager::getInstance()->get((SettingsManager::StrSetting)i.setting));
	if(createFolderDialog().open(dir))
		box->setText(dir);
}

void PropPage::handleBrowseFile(const Item& i) {
	TextBoxPtr box = static_cast<TextBoxPtr>(i.widget);
	tstring target = box->getText();
	if(target.empty())
		target = Text::toT(SettingsManager::getInstance()->get((SettingsManager::StrSetting)i.setting));
	if(createLoadDialog().setInitialDirectory(target).open(target))
		box->setText(target);
}

void PropPage::handleListHelp(HWND hWnd, unsigned id, const ListItem* listItems, TablePtr list) {
	// we have the help id of the whole list-view; convert to the one of the specific option the user wants help for
	int item =
		isKeyPressed(VK_F1) ? list->getSelected() :
		list->hitTest(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos())));
	if(item >= 0 && listItems[item].helpId)
		id = listItems[item].helpId;
	WinUtil::help(hWnd, id);
}
