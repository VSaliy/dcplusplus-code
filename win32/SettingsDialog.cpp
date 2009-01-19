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

#include "SettingsDialog.h"
#include "resource.h"

#include "WinUtil.h"

#include "GeneralPage.h"
#include "NetworkPage.h"
#include "DownloadPage.h"
#include "FavoriteDirsPage.h"
#include "QueuePage.h"
#include "UploadPage.h"
#include "AppearancePage.h"
#include "Appearance2Page.h"
#include "TabsPage.h"
#include "WindowsPage.h"
#include "AdvancedPage.h"
#include "LogPage.h"
#include "Advanced3Page.h"
#include "UCPage.h"
#include "CertificatesPage.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_SETTINGS_PAGES, IDH_SETTINGS_TREE },
	{ IDOK, IDH_DCPP_OK },
	{ IDCANCEL, IDH_DCPP_CANCEL },
	{ IDHELP, IDH_DCPP_HELP },
	{ 0, 0 }
};
*/

SettingsDialog::SettingsDialog(dwt::Widget* parent) : WidgetFactory<dwt::ModalDialog>(parent), currentPage(0) {
	onInitDialog(std::tr1::bind(&SettingsDialog::initDialog, this));
	onHelp(std::tr1::bind(&SettingsDialog::handleHelp, this, _1, _2));
	onSized(std::tr1::bind(&SettingsDialog::handleSized, this, _1));
}

int SettingsDialog::run() {
	createDialog(IDD_SETTINGS);
	return show();
}

SettingsDialog::~SettingsDialog() {
}

bool SettingsDialog::initDialog() {
	/*
	* set this to IDH_INDEX so that clicking in an empty space of the dialog generates a WM_HELP
	* message with no error; then SettingsDialog::handleHelp will convert IDH_INDEX to the current
	* page's help id.
	*/
	setHelpId(IDH_INDEX);

	setText(T_("Settings"));

	grid = addChild(Grid::Seed(2, 1));

	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->column(0).mode = GridInfo::FILL;

	GridPtr upper = grid->addChild(Grid::Seed(1, 2));

	upper->row(0).mode = GridInfo::FILL;
	upper->row(0).align = GridInfo::STRETCH;

	upper->column(0).size = 90;
	upper->column(0).mode = GridInfo::STATIC;
	upper->column(1).mode = GridInfo::FILL;

	pageTree = upper->addChild(Tree::Seed());

	pageTree->onSelectionChanged(std::tr1::bind(&SettingsDialog::handleSelectionChanged, this));

	addPage(T_("Personal information"), upper, new GeneralPage(upper));

	addPage(T_("Connection settings"), upper, new NetworkPage(upper));

	{
		HTREEITEM item = addPage(T_("Downloads"), upper, new DownloadPage(upper));
		addPage(T_("Favorites"), upper, new FavoriteDirsPage(upper), item);
		addPage(T_("Queue"), upper, new QueuePage(upper), item);
	}

	addPage(T_("Sharing"), upper, new UploadPage(upper));

	{
		HTREEITEM item = addPage(T_("Appearance"),upper,  new AppearancePage(upper));
		addPage(T_("Colors and sounds"), upper, new Appearance2Page(upper), item);
		addPage(T_("Tabs"), upper, new TabsPage(upper), item);
		addPage(T_("Windows"), upper, new WindowsPage(upper), item);
	}

	{
		HTREEITEM item = addPage(T_("Advanced"), upper, new AdvancedPage(upper));
		addPage(T_("Logs"), upper, new LogPage(upper), item);
		addPage(T_("Experts only"), upper, new Advanced3Page(upper), item);
		addPage(T_("User Commands"), upper, new UCPage(upper), item);
		addPage(T_("Security Certificates"), upper, new CertificatesPage(upper), item);
	}

	GridPtr lower = grid->addChild(Grid::Seed(1, 3));

	{
		ButtonPtr button = lower->addChild(Button::Seed());
		button->setText(T_("OK"));
		button->onClicked(std::tr1::bind(&SettingsDialog::handleOKClicked, this));

		button = lower->addChild(Button::Seed());
		button->setText(T_("Cancel"));
		button->onClicked(std::tr1::bind(&SettingsDialog::endDialog, this, IDCANCEL));

		button = lower->addChild(Button::Seed());
		button->setText(T_("Help"));
		button->onClicked(std::tr1::bind(&SettingsDialog::handleHelp, this, handle(), IDH_INDEX));
	}


	updateTitle();

	layout();

	return false;
}

HTREEITEM SettingsDialog::addPage(const tstring& title, GridPtr upper, PropPage* page, HTREEITEM parent) {
	upper->setWidget(page, 0, 1);

	pages.push_back(page);

	HTREEITEM item = pageTree->insert(title, parent, reinterpret_cast<LPARAM>(page));
	pageTree->expand(parent);
	return item;
}

void SettingsDialog::handleHelp(HWND hWnd, unsigned id) {
	if(id == IDH_INDEX && currentPage)
		id = currentPage->getHelpId();
	WinUtil::help(hWnd, id);
}

void SettingsDialog::handleSelectionChanged() {
	PropPage* page = reinterpret_cast<PropPage*>(pageTree->getData(pageTree->getSelected()));
	if(page) {
		if(currentPage) {
			currentPage->setVisible(false);
			currentPage = 0;
		}

		page->setVisible(true);
		currentPage = page;

		updateTitle();
		layout();
	}
}

void SettingsDialog::handleOKClicked() {
	write();
	endDialog(IDOK);
}

void SettingsDialog::updateTitle() {
	tstring title = pageTree->getSelectedText();
	setText(title.empty() ? T_("Settings") : T_("Settings") + _T(" - [") + title + _T("]"));
}

void SettingsDialog::write() {
	for(PageList::iterator i = pages.begin(); i != pages.end(); ++i) {
		(*i)->write();
	}
}

void SettingsDialog::handleSized(const dwt::SizedEvent& sz) {
	layout();
}

void SettingsDialog::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
