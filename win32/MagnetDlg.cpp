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

#include "MagnetDlg.h"

#include "WinUtil.h"

MagnetDlg::MagnetDlg(dwt::Widget* parent, const tstring& aHash, const tstring& aFileName, const tstring& aKeySearch) :
dwt::ModalDialog(parent),
grid(0),
//queue(0),
search(0),
//doNothing(0),
//remember(0),
mHash(aHash),
mFileName(aFileName),
mKeySearch(aKeySearch)
{
	onInitDialog(std::bind(&MagnetDlg::handleInitDialog, this));
}

MagnetDlg::~MagnetDlg() {
}

int MagnetDlg::run() {
	create(dwt::Point(460 * WinUtil::dpiFactor, 180 * WinUtil::dpiFactor));
	return show();
}

bool MagnetDlg::handleInitDialog() {
	grid = addChild(Grid::Seed(3, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(1).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(10);

		cur->addChild(Label::Seed(IDI_MAGNET));

		cur->addChild(Label::Seed(T_("DC++ has detected a MAGNET link with a file hash that can be searched for on the Direct Connect network.  What would you like to do?")));
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(2, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		TextBox::Seed seed = WinUtil::Seeds::Dialog::textBox;
		seed.style |= ES_READONLY;

		if (!mHash.empty()) {
			cur->addChild(Label::Seed(T_("File Hash:")));
			cur->addChild(seed)->setText(mHash);
		}
		else if (!mKeySearch.empty())
		{
			cur->addChild(Label::Seed(T_("Key Search:")));
			cur->addChild(seed)->setText(mKeySearch);
		}

		cur->addChild(Label::Seed(T_("Filename:")));
		cur->addChild(seed)->setText(mFileName);
	}

	{
		GridPtr bottom = grid->addChild(Grid::Seed(1, 2));
		bottom->column(1).mode = GridInfo::FILL;
		bottom->column(1).align = GridInfo::BOTTOM_RIGHT;

		//GridPtr cur = bottom->addChild(Grid::Seed(4, 1));
		GridPtr cur = bottom->addChild(Grid::Seed(2, 1));

		//queue = cur->addChild(RadioButton::Seed(T_("Add this file to your download queue")));
		//queue->onClicked(std::bind(&MagnetDlg::handleRadioButtonClicked, this, queue));

		search = cur->addChild(RadioButton::Seed(T_("Start a search for this file")));
		search->setChecked();
		//search->onClicked(std::bind(&MagnetDlg::handleRadioButtonClicked, this, search));

		//doNothing = cur->addChild(CheckBox::Seed(T_("Do nothing")));
		//doNothing->onClicked(std::bind(&MagnetDlg::handleRadioButtonClicked, this, doNothing));
		cur->addChild(RadioButton::Seed(T_("Do nothing")));

		//remember = cur->addChild(CheckBox::Seed(T_("Do the same action next time without asking")));

		WinUtil::addDlgButtons(bottom->addChild(Grid::Seed(2, 1)),
			std::bind(&MagnetDlg::handleOKClicked, this),
			std::bind(&MagnetDlg::endDialog, this, IDCANCEL));
	}

	setText(T_("MAGNET Link detected"));

	layout();
	centerWindow();

	return false;
}

//void MagnetDlg::handleRadioButtonClicked(RadioButtonPtr radioButton) {
	//if(radioButton == queue || radioButton == search)
		//remember->setEnabled(true);
	//else if(radioButton == doNothing) {
		//if(remember->getChecked())
			//remember->setChecked(false);
		//remember->setEnabled(false);
	//}
//}

void MagnetDlg::handleOKClicked() {
	//if(remember->getChecked()) {
	//	SettingsManager::getInstance()->set(SettingsManager::MAGNET_ASK,  false);
	//	if(queue->getChecked())
	//		SettingsManager::getInstance()->set(SettingsManager::MAGNET_ACTION, SettingsManager::MAGNET_AUTO_DOWNLOAD);
	//	else if(search->getChecked())
	//		SettingsManager::getInstance()->set(SettingsManager::MAGNET_ACTION, SettingsManager::MAGNET_AUTO_SEARCH);
	//}

	if(search->getChecked()) {
		if (!mHash.empty()) {
			TTHValue tmphash(Text::fromT(mHash));
			WinUtil::searchHash(tmphash);
		}else if (!mKeySearch.empty()) {
			WinUtil::searchAny(mKeySearch);
		}
	} //else if(queue->getChecked()) {
		// FIXME: Write this code when the queue is more tth-centric
	//}

	endDialog(IDOK);
}

void MagnetDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->resize(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
