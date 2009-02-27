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

#include "CertificatesPage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/CryptoManager.h>
#include "WinUtil.h"

PropPage::ListItem CertificatesPage::listItems[] = {
	{ SettingsManager::USE_TLS, N_("Use TLS when remote client supports it"), IDH_SETTINGS_CERTIFICATES_USE_TLS },
	{ SettingsManager::ALLOW_UNTRUSTED_HUBS, N_("Allow TLS connections to hubs without trusted certificate"), IDH_SETTINGS_CERTIFICATES_ALLOW_UNTRUSTED_HUBS },
	{ SettingsManager::ALLOW_UNTRUSTED_CLIENTS, N_("Allow TLS connections to clients without trusted certificate"), IDH_SETTINGS_CERTIFICATES_ALLOW_UNTRUSTED_CLIENTS },
	{ 0, 0 }
};

CertificatesPage::CertificatesPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
options(0)
{
	createDialog(IDD_CERTIFICATESPAGE);
	setHelpId(IDH_CERTIFICATESPAGE);

	grid = addChild(Grid::Seed(5, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(2).mode = GridInfo::FILL;
	grid->row(2).align = GridInfo::STRETCH;

	{
		GridPtr cur = grid->addChild(Grid::Seed(3, 3));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		Button::Seed dots(_T("..."));
		dots.padding.x = 10;

		cur->addChild(Label::Seed(T_("Private key file")))->setHelpId(IDH_SETTINGS_CERTIFICATES_PRIVATE_KEY_FILE);
		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		items.push_back(Item(box, SettingsManager::TLS_PRIVATE_KEY_FILE, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_CERTIFICATES_PRIVATE_KEY_FILE);	
		ButtonPtr button = cur->addChild(dots);
		button->onClicked(std::tr1::bind(&CertificatesPage::handleBrowseFile, this, items.back()));
		button->setHelpId(IDH_SETTINGS_CERTIFICATES_PRIVATE_KEY_FILE);			

		cur->addChild(Label::Seed(T_("Own certificate file")))->setHelpId(IDH_SETTINGS_CERTIFICATES_CERTIFICATE_FILE);
		box = cur->addChild(WinUtil::Seeds::Dialog::TextBox);
		items.push_back(Item(box, SettingsManager::TLS_CERTIFICATE_FILE, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_CERTIFICATES_CERTIFICATE_FILE);	
		button = cur->addChild(dots);
		button->onClicked(std::tr1::bind(&CertificatesPage::handleBrowseFile, this, items.back()));
		button->setHelpId(IDH_SETTINGS_CERTIFICATES_CERTIFICATE_FILE);			

		cur->addChild(Label::Seed(T_("Trusted certificates path")))->setHelpId(IDH_SETTINGS_CERTIFICATES_TRUSTED_CERTIFICATES_PATH);
		box = cur->addChild(WinUtil::Seeds::Dialog::TextBox);	
		items.push_back(Item(box, SettingsManager::TLS_TRUSTED_CERTIFICATES_PATH, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_CERTIFICATES_TRUSTED_CERTIFICATES_PATH);	
		button = cur->addChild(dots);
		button->onClicked(std::tr1::bind(&CertificatesPage::handleBrowseDir, this, items.back()));
		button->setHelpId(IDH_SETTINGS_CERTIFICATES_TRUSTED_CERTIFICATES_PATH);			
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;

		ButtonPtr gen = cur->addChild(Button::Seed(T_("Generate certificates")));
		gen->onClicked(std::tr1::bind(&CertificatesPage::handleGenerateCertsClicked, this));
	}

	options = grid->addChild(WinUtil::Seeds::Dialog::optionsTable);

	{
		LabelPtr label = grid->addChild(Label::Seed(T_("Under construction, restart DC++ to see effects...")));

		label = grid->addChild(Label::Seed(T_("Experimental feature, don't consider DC++ secure in any way")));
	}

	PropPage::read(items);
	PropPage::read(listItems, options);
}

CertificatesPage::~CertificatesPage() {
}

void CertificatesPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientAreaSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void CertificatesPage::write() {
	PropPage::write(items);
	PropPage::write(listItems, options);
}

void CertificatesPage::handleGenerateCertsClicked() {
	try {
		CryptoManager::getInstance()->generateCertificate();
	} catch(const CryptoException& e) {
		createMessageBox().show(Text::toT(e.getError()), T_("Error generating certificate"), MessageBox::BOX_OK, MessageBox::BOX_ICONSTOP);
	}
}
