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

#include "UCPage.h"

#include <dwt/widgets/Grid.h>

#include <dcpp/SettingsManager.h>
#include <dcpp/FavoriteManager.h>
#include "CommandDlg.h"
#include "HoldRedraw.h"
#include "WinUtil.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_MENU_ITEMS, IDH_SETTINGS_UC_LIST },
	{ IDC_ADD_MENU, IDH_SETTINGS_UC_ADD },
	{ IDC_CHANGE_MENU, IDH_SETTINGS_UC_CHANGE },
	{ IDC_MOVE_UP, IDH_SETTINGS_UC_MOVE_UP },
	{ IDC_MOVE_DOWN, IDH_SETTINGS_UC_MOVE_DOWN },
	{ IDC_REMOVE_MENU, IDH_SETTINGS_UC_REMOVE },
	{ 0, 0 }
};
*/

UCPage::UCPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
commands(0)
{
	createDialog(IDD_UCPAGE);
	setHelpId(IDH_UCPAGE);

	grid = addChild(Grid::Seed(2, 5));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;
	grid->column(3).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	commands = grid->addChild(WinUtil::Seeds::Dialog::Table);
	grid->setWidget(commands, 0, 0, 1, 5);

	grid->addChild(Button::Seed(T_("&Add")))->onClicked(std::tr1::bind(&UCPage::handleAddClicked, this));
	grid->addChild(Button::Seed(T_("&Change")))->onClicked(std::tr1::bind(&UCPage::handleChangeClicked, this));
	grid->addChild(Button::Seed(T_("Move &Up")))->onClicked(std::tr1::bind(&UCPage::handleMoveUpClicked, this));
	grid->addChild(Button::Seed(T_("Move &Down")))->onClicked(std::tr1::bind(&UCPage::handleMoveDownClicked, this));
	grid->addChild(Button::Seed(T_("&Remove")))->onClicked(std::tr1::bind(&UCPage::handleRemoveClicked, this));

	TStringList columns;
	columns.push_back(T_("Name"));
	columns.push_back(T_("Command"));
	columns.push_back(T_("Hub"));
	commands->createColumns(columns);
	commands->setColumnWidth(0, 100);
	commands->setColumnWidth(1, commands->getSize().x - 220);
	commands->setColumnWidth(2, 100);

	UserCommand::List lst = FavoriteManager::getInstance()->getUserCommands();
	for(UserCommand::List::const_iterator i = lst.begin(); i != lst.end(); ++i) {
		const UserCommand& uc = *i;
		if(!uc.isSet(UserCommand::FLAG_NOSAVE))
			addEntry(uc);
	}

	commands->onDblClicked(std::tr1::bind(&UCPage::handleDoubleClick, this));
	commands->onKeyDown(std::tr1::bind(&UCPage::handleKeyDown, this, _1));
}

UCPage::~UCPage() {
}

void UCPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientAreaSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void UCPage::handleDoubleClick() {
	if(commands->hasSelected()) {
		handleChangeClicked();
	} else {
		handleAddClicked();
	}
}

bool UCPage::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddClicked();
		return true;
	case VK_DELETE:
		handleRemoveClicked();
		return true;
	}
	return false;
}

void UCPage::handleAddClicked() {
	CommandDlg dlg(this);
	if(dlg.run() == IDOK)
		addEntry(FavoriteManager::getInstance()->addUserCommand(dlg.getType(), dlg.getCtx(), 0, Text::fromT(dlg.getName()), Text::fromT(dlg.getCommand()), Text::fromT(dlg.getHub())));
}

void UCPage::handleChangeClicked() {
	if(commands->countSelected() == 1) {
		int i = commands->getSelected();
		UserCommand uc;
		FavoriteManager::getInstance()->getUserCommand(commands->getData(i), uc);

		CommandDlg dlg(this, uc.getType(), uc.getCtx(), Text::toT(uc.getName()), Text::toT(uc.getCommand()), Text::toT(uc.getHub()));
		if(dlg.run() == IDOK) {
			commands->setText(0, i, (dlg.getType() == UserCommand::TYPE_SEPARATOR) ? T_("Separator") : dlg.getName());
			commands->setText(1, i, dlg.getCommand());
			commands->setText(2, i, dlg.getHub());

			uc.setName(Text::fromT(dlg.getName()));
			uc.setCommand(Text::fromT(dlg.getCommand()));
			uc.setHub(Text::fromT(dlg.getHub()));
			uc.setType(dlg.getType());
			uc.setCtx(dlg.getCtx());
			FavoriteManager::getInstance()->updateUserCommand(uc);
		}
	}
}

void UCPage::handleMoveUpClicked() {
	if(commands->countSelected() == 1) {
		int i = commands->getSelected();
		if(i == 0)
			return;
		int n = commands->getData(i);
		FavoriteManager::getInstance()->moveUserCommand(n, -1);
		HoldRedraw hold(commands);
		commands->erase(i);
		UserCommand uc;
		FavoriteManager::getInstance()->getUserCommand(n, uc);
		addEntry(uc, --i);
		commands->setSelected(i);
		commands->ensureVisible(i);
	}
}

void UCPage::handleMoveDownClicked() {
	if(commands->countSelected() == 1) {
		int i = commands->getSelected();
		if(i == commands->size() - 1)
			return;
		int n = commands->getData(i);
		FavoriteManager::getInstance()->moveUserCommand(n, 1);
		HoldRedraw hold(commands);
		commands->erase(i);
		UserCommand uc;
		FavoriteManager::getInstance()->getUserCommand(n, uc);
		addEntry(uc, ++i);
		commands->setSelected(i);
		commands->ensureVisible(i);
	}
}

void UCPage::handleRemoveClicked() {
	if(commands->countSelected() == 1) {
		int i = commands->getSelected();
		FavoriteManager::getInstance()->removeUserCommand(commands->getData(i));
		commands->erase(i);
	}
}

void UCPage::addEntry(const UserCommand& uc, int index) {
	TStringList row;
	row.push_back((uc.getType() == UserCommand::TYPE_SEPARATOR) ? T_("Separator") : Text::toT(uc.getName()));
	row.push_back(Text::toT(uc.getCommand()));
	row.push_back(Text::toT(uc.getHub()));
	commands->insert(row, (LPARAM)uc.getId(), index);
}
