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

#include "CommandDlg.h"

#include <dcpp/UserCommand.h>
#include <dcpp/NmdcHub.h>
#include <dcpp/version.h>
#include "WinUtil.h"

CommandDlg::CommandDlg(dwt::Widget* parent, int type_, int ctx_, const tstring& name_, const tstring& command_, const tstring& hub_) :
WidgetFactory<dwt::ModalDialog>(parent),
grid(0),
separator(0),
raw(0),
chat(0),
PM(0),
hubMenu(0),
userMenu(0),
searchMenu(0),
fileListMenu(0),
nameBox(0),
commandBox(0),
hubBox(0),
nick(0),
once(0),
result(0),
openHelp(0),
type(type_),
ctx(ctx_),
name(name_),
command(command_),
hub(hub_)
{
	onInitDialog(std::tr1::bind(&CommandDlg::handleInitDialog, this));
	onHelp(std::tr1::bind(&WinUtil::help, _1, _2));
}

CommandDlg::~CommandDlg() {
}

int CommandDlg::run() {
	createDialog(IDD_USER_COMMAND);
	return show();
}

bool CommandDlg::handleInitDialog() {
	setHelpId(IDH_USER_COMMAND);

	grid = addChild(Grid::Seed(6, 3));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Command Type")));
		grid->setWidget(group, 0, 0, 1, 3);

		GridPtr cur = group->addChild(Grid::Seed(2, 2));
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		separator = cur->addChild(RadioButton::Seed(T_("Separator")));
		separator->setHelpId(IDH_USER_COMMAND_SEPARATOR);
		separator->onClicked(std::tr1::bind(&CommandDlg::handleTypeChanged, this));

		chat = cur->addChild(RadioButton::Seed(T_("Chat")));
		chat->setHelpId(IDH_USER_COMMAND_CHAT);
		chat->onClicked(std::tr1::bind(&CommandDlg::handleTypeChanged, this));

		raw = cur->addChild(RadioButton::Seed(T_("Raw")));
		raw->setHelpId(IDH_USER_COMMAND_RAW);
		raw->onClicked(std::tr1::bind(&CommandDlg::handleTypeChanged, this));

		PM = cur->addChild(RadioButton::Seed(T_("PM")));
		PM->setHelpId(IDH_USER_COMMAND_PM);
		PM->onClicked(std::tr1::bind(&CommandDlg::handleTypeChanged, this));
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Context")));
		grid->setWidget(group, 1, 0, 1, 3);
		group->setHelpId(IDH_USER_COMMAND_CONTEXT);

		GridPtr cur = group->addChild(Grid::Seed(2, 2));
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		hubMenu = cur->addChild(RadioButton::Seed(T_("Hub Menu")));
		hubMenu->setHelpId(IDH_USER_COMMAND_HUB_MENU);

		searchMenu = cur->addChild(RadioButton::Seed(T_("Search Menu")));
		searchMenu->setHelpId(IDH_USER_COMMAND_SEARCH_MENU);

		userMenu = cur->addChild(RadioButton::Seed(T_("User Menu")));
		userMenu->setHelpId(IDH_USER_COMMAND_USER_MENU);

		fileListMenu = cur->addChild(RadioButton::Seed(T_("Filelist Menu")));
		fileListMenu->setHelpId(IDH_USER_COMMAND_FILELIST_MENU);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Parameters")));
		grid->setWidget(group, 2, 0, 1, 3);

		GridPtr cur = group->addChild(Grid::Seed(9, 1));
		cur->column(0).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Name")))->setHelpId(IDH_USER_COMMAND_NAME);
		nameBox = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		nameBox->setHelpId(IDH_USER_COMMAND_NAME);

		cur->addChild(Label::Seed(T_("Command")))->setHelpId(IDH_USER_COMMAND_COMMAND);
		TextBox::Seed seed = WinUtil::Seeds::Dialog::TextBox;
		seed.style |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;
		commandBox = cur->addChild(seed);
		commandBox->setHelpId(IDH_USER_COMMAND_COMMAND);
		commandBox->onUpdated(std::tr1::bind(&CommandDlg::updateCommand, this));

		cur->addChild(Label::Seed(T_("Hub IP / DNS (empty = all, 'op' = where operator)")))->setHelpId(IDH_USER_COMMAND_HUB);
		hubBox = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		hubBox->setHelpId(IDH_USER_COMMAND_HUB);

		cur->addChild(Label::Seed(T_("To")))->setHelpId(IDH_USER_COMMAND_NICK);
		nick = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		nick->setHelpId(IDH_USER_COMMAND_NICK);
		nick->onUpdated(std::tr1::bind(&CommandDlg::updateCommand, this));

		once = cur->addChild(CheckBox::Seed(T_("Send once per nick")));
		once->setHelpId(IDH_USER_COMMAND_ONCE);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Text sent to hub")));
		grid->setWidget(group, 3, 0, 1, 3);
		result = group->addChild(WinUtil::Seeds::Dialog::TextBox);
	}

	openHelp = grid->addChild(CheckBox::Seed(T_("Always open help file with this dialog")));
	grid->setWidget(openHelp, 4, 0, 1, 3);
	bool bOpenHelp = BOOLSETTING(OPEN_USER_CMD_HELP);
	openHelp->setChecked(bOpenHelp);

	WinUtil::addDlgButtons(grid,
		std::tr1::bind(&CommandDlg::handleOKClicked, this),
		std::tr1::bind(&CommandDlg::endDialog, this, IDCANCEL));

	{
		ButtonPtr button = grid->addChild(Button::Seed(T_("Help")));
		button->setHelpId(IDH_DCPP_HELP);
		button->onClicked(std::tr1::bind(&WinUtil::help, handle(), IDH_USER_COMMAND));
	}

	if(bOpenHelp) {
		// launch the help file, instead of having the help in the dialog
		WinUtil::help(handle(), IDH_USER_COMMAND);
	}

	if(type == UserCommand::TYPE_SEPARATOR) {
		separator->setChecked(true);
	} else {
		// More difficult, determine type by what it seems to be...
		if((_tcsncmp(command.c_str(), _T("$To: "), 5) == 0) &&
			(command.find(_T(" From: %[myNI] $<%[myNI]> ")) != string::npos ||
			command.find(_T(" From: %[mynick] $<%[mynick]> ")) != string::npos) &&
			command.find(_T('|')) == command.length() - 1) // if it has | anywhere but the end, it is raw
		{
			string::size_type i = command.find(_T(' '), 5);
			dcassert(i != string::npos);
			tstring to = command.substr(5, i-5);
			string::size_type cmd_pos = command.find(_T('>'), 5) + 2;
			tstring cmd = Text::toT(NmdcHub::validateMessage(Text::fromT(command.substr(cmd_pos, command.length()-cmd_pos-1)), true));
			PM->setChecked(true);
			nick->setText(to);
			commandBox->setText(cmd.c_str());
		} else if(((_tcsncmp(command.c_str(), _T("<%[mynick]> "), 12) == 0) ||
			(_tcsncmp(command.c_str(), _T("<%[myNI]> "), 10) == 0)) &&
			command[command.length()-1] == '|')
		{
			// Looks like a chat thing...
			string::size_type cmd_pos = command.find(_T('>')) + 2;
			tstring cmd = Text::toT(NmdcHub::validateMessage(Text::fromT(command.substr(cmd_pos, command.length()-cmd_pos-1)), true));
			chat->setChecked(true);
			commandBox->setText(cmd);
		} else {
			tstring cmd = command;
			raw->setChecked(true);
			commandBox->setText(cmd);
		}
		if(type == UserCommand::TYPE_RAW_ONCE) {
			once->setChecked(true);
			type = 1;
		}
	}

	hubBox->setText(hub);
	nameBox->setText(name);

	if(ctx & UserCommand::CONTEXT_HUB)
		hubMenu->setChecked(true);
	if(ctx & UserCommand::CONTEXT_CHAT)
		userMenu->setChecked(true);
	if(ctx & UserCommand::CONTEXT_SEARCH)
		searchMenu->setChecked(true);
	if(ctx & UserCommand::CONTEXT_FILELIST)
		fileListMenu->setChecked(true);

	updateControls();
	updateCommand();

	setText(T_("Create / Modify User Command"));

	layout();
	centerWindow();

	return false;
}

void CommandDlg::handleTypeChanged() {
	updateType();
	updateCommand();
	updateControls();
}

void CommandDlg::handleOKClicked() {
	name = nameBox->getText();
	if((type != 0) && (name.empty() || commandBox->getText().empty())) {
		createMessageBox().show(T_("Name and command must not be empty"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONEXCLAMATION);
		return;
	}

	ctx = 0;
	if(hubMenu->getChecked())
		ctx |= UserCommand::CONTEXT_HUB;
	if(userMenu->getChecked())
		ctx |= UserCommand::CONTEXT_CHAT;
	if(searchMenu->getChecked())
		ctx |= UserCommand::CONTEXT_SEARCH;
	if(fileListMenu->getChecked())
		ctx |= UserCommand::CONTEXT_FILELIST;

	hub = hubBox->getText();

	if(type != 0)
		type = once->getChecked() ? 2 : 1;

	SettingsManager::getInstance()->set(SettingsManager::OPEN_USER_CMD_HELP, openHelp->getChecked());

	endDialog(IDOK);
}

void CommandDlg::updateType() {
	if(separator->getChecked()) {
		type = 0;
	} else if(raw->getChecked()) {
		type = 1;
	} else if(chat->getChecked()) {
		type = 2;
	} else if(PM->getChecked()) {
		type = 3;
	}
}

void CommandDlg::updateCommand() {
	if(type == 0) {
		command.clear();
	} else if(type == 1) {
		command = commandBox->getText();
	} else if(type == 2) {
		command = Text::toT("<%[myNI]> " + NmdcHub::validateMessage(Text::fromT(commandBox->getText()), false) + "|");
	} else if(type == 3) {
		command = _T("$To: ") + nick->getText() + _T(" From: %[myNI] $<%[myNI]> ") + Text::toT(NmdcHub::validateMessage(Text::fromT(commandBox->getText()), false)) + _T("|");
	}
	result->setText(command);
}

void CommandDlg::updateControls() {
	switch(type) {
		case 0:
			nameBox->setEnabled(false);
			commandBox->setEnabled(false);
			nick->setEnabled(false);
			break;
		case 1:
		case 2:
			nameBox->setEnabled(true);
			commandBox->setEnabled(true);
			nick->setEnabled(false);
			break;
		case 3:
			nameBox->setEnabled(true);
			commandBox->setEnabled(true);
			nick->setEnabled(true);
			break;
	}
}

void CommandDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
