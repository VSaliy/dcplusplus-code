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
#include "BandwidthLimitPage.h"

SettingsDialog::SettingsDialog(dwt::Widget* parent) :
dwt::ModalDialog(parent),
currentPage(0),
grid(0),
pageTree(0),
help(0)
{
	onInitDialog(std::tr1::bind(&SettingsDialog::initDialog, this));
	onHelp(std::tr1::bind(&SettingsDialog::handleHelp, this, _1, _2));
}

int SettingsDialog::run() {
	create(Seed(dwt::Point(653, 480), DS_CONTEXTHELP));
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

	grid = addChild(Grid::Seed(3, 1));

	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->column(0).mode = GridInfo::FILL;

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));

		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;

		cur->column(0).size = 155;
		cur->column(0).mode = GridInfo::STATIC;
		cur->column(1).mode = GridInfo::FILL;

		pageTree = cur->addChild(Tree::Seed());
		pageTree->setHelpId(IDH_SETTINGS_TREE);
		pageTree->onSelectionChanged(std::tr1::bind(&SettingsDialog::handleSelectionChanged, this));

		addPage(T_("Personal information"), cur, new GeneralPage(cur));

		addPage(T_("Connection settings"), cur, new NetworkPage(cur));

		{
			HTREEITEM item = addPage(T_("Downloads"), cur, new DownloadPage(cur));
			addPage(T_("Favorites"), cur, new FavoriteDirsPage(cur), item);
			addPage(T_("Queue"), cur, new QueuePage(cur), item);
		}

		addPage(T_("Sharing"), cur, new UploadPage(cur));

		{
			HTREEITEM item = addPage(T_("Appearance"),cur,  new AppearancePage(cur));
			addPage(T_("Colors and sounds"), cur, new Appearance2Page(cur), item);
			addPage(T_("Tabs"), cur, new TabsPage(cur), item);
			addPage(T_("Windows"), cur, new WindowsPage(cur), item);
		}

		{
			HTREEITEM item = addPage(T_("Advanced"), cur, new AdvancedPage(cur));
			addPage(T_("Logs"), cur, new LogPage(cur), item);
			addPage(T_("Experts only"), cur, new Advanced3Page(cur), item);
			addPage(T_("User Commands"), cur, new UCPage(cur), item);
			addPage(T_("Security Certificates"), cur, new CertificatesPage(cur), item);
			addPage(T_("Bandwidth Limiting"), cur, new BandwidthLimitPage(cur), item);
		}
	}

	{
		TextBox::Seed seed = WinUtil::Seeds::Dialog::textBox;
		seed.style &= ~ES_AUTOHSCROLL;
		seed.style |= ES_MULTILINE | WS_VSCROLL | ES_READONLY;
		seed.lines = 4;
		help = grid->addChild(seed);
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 3));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;

		WinUtil::addDlgButtons(cur,
			std::tr1::bind(&SettingsDialog::handleOKClicked, this),
			std::tr1::bind(&SettingsDialog::endDialog, this, IDCANCEL));

		Button::Seed seed(T_("Help"));
		seed.padding.x = 10;
		ButtonPtr button = cur->addChild(seed);
		button->setHelpId(IDH_DCPP_HELP);
		button->onClicked(std::tr1::bind(&SettingsDialog::handleHelp, this, handle(), IDH_INDEX));
	}

	/*
	* catch WM_SETFOCUS messages (onFocus events) sent to every children of this dialog. the normal
	* way to do it would be to use an Application::Filter, but unfortunately these messages don't
	* go through there but instead are sent directly to the control's wndproc.
	*/
	/// @todo when dwt has better tracking of children, improve this
	::EnumChildWindows(handle(), EnumChildProc, reinterpret_cast<LPARAM>(this));

	updateTitle();

	layout();
	centerWindow();

	return false;
}

BOOL CALLBACK SettingsDialog::EnumChildProc(HWND hwnd, LPARAM lParam) {
	SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(lParam);
	dwt::Control* widget = dwt::hwnd_cast<dwt::Control*>(hwnd);
	if(widget && widget != dialog->help)
		widget->onFocus(std::tr1::bind(&SettingsDialog::handleFocus, dialog, widget));
	return TRUE;
}

void SettingsDialog::handleFocus(dwt::Control* widget) {
	help->setText(Text::toT(WinUtil::getHelpText(widget->getHelpId())));
}

HTREEITEM SettingsDialog::addPage(const tstring& title, GridPtr upper, PropPage* page, HTREEITEM parent) {
	upper->setWidget(page, 0, 1);
	pages.push_back(page);
	return pageTree->insert(title, parent, reinterpret_cast<LPARAM>(page), true);
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

void SettingsDialog::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
