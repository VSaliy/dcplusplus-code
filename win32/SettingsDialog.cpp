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
#include "HistoryPage.h"
#include "LogPage.h"
#include "AdvancedPage.h"
#include "Advanced3Page.h"
#include "UCPage.h"
#include "CertificatesPage.h"
#include "BandwidthLimitPage.h"
#include "SearchTypesPage.h"
#include "ProxyPage.h"

SettingsDialog::SettingsDialog(dwt::Widget* parent) :
dwt::ModalDialog(parent),
currentPage(0),
grid(0),
tree(0),
help(0)
{
	onInitDialog(std::bind(&SettingsDialog::initDialog, this));
	onHelp(std::bind(&SettingsDialog::handleHelp, this, _1, _2));
}

int SettingsDialog::run() {
	create(Seed(dwt::Point(700 * WinUtil::dpiFactor, 580 * WinUtil::dpiFactor), DS_CONTEXTHELP));
	return show();
}

SettingsDialog::~SettingsDialog() {
}

static LRESULT helpDlgCode(WPARAM wParam) {
	if(wParam == VK_TAB || wParam == VK_RETURN)
		return 0;
	return DLGC_WANTMESSAGE;
}

bool SettingsDialog::initDialog() {
	/*
	* set this to IDH_INDEX so that clicking in an empty space of the dialog generates a WM_HELP
	* message with no error; then SettingsDialog::handleHelp will convert IDH_INDEX to the current
	* page's help id.
	*/
	setHelpId(IDH_INDEX);

	grid = addChild(Grid::Seed(3, 1));
	grid->setSpacing(8);

	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->column(0).mode = GridInfo::FILL;

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->setSpacing(8);

		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;

		cur->column(0).size = 170;
		cur->column(0).mode = GridInfo::STATIC;
		cur->column(1).mode = GridInfo::FILL;

		{
			auto seed = Tree::Seed();
			seed.style |= WS_BORDER;
			tree = cur->addChild(seed);
			tree->setHelpId(IDH_SETTINGS_TREE);
			tree->onSelectionChanged(std::bind(&SettingsDialog::handleSelectionChanged, this));
		}

		Grid::Seed seed(1, 1);
		seed.style |= WS_BORDER;
		cur = cur->addChild(seed);
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;

		addPage(T_("Personal information"), new GeneralPage(cur));

		{
			HTREEITEM item = addPage(T_("Connection settings"), new NetworkPage(cur));
			addPage(T_("Bandwidth Limiting"), new BandwidthLimitPage(cur), item);
			addPage(T_("Proxy Settings"), new ProxyPage(cur), item);
		}

		{
			HTREEITEM item = addPage(T_("Downloads"), new DownloadPage(cur));
			addPage(T_("Favorites"), new FavoriteDirsPage(cur), item);
			addPage(T_("Queue"), new QueuePage(cur), item);
		}

		addPage(T_("Sharing"), new UploadPage(cur));

		{
			HTREEITEM item = addPage(T_("Appearance"), new AppearancePage(cur));
			addPage(T_("Colors and sounds"), new Appearance2Page(cur), item);
			addPage(T_("Tabs"), new TabsPage(cur), item);
			addPage(T_("Windows"), new WindowsPage(cur), item);
		}

		{
			HTREEITEM item = addPage(T_("History"), new HistoryPage(cur));
			addPage(T_("Logs"), new LogPage(cur), item);
		}

		{
			HTREEITEM item = addPage(T_("Advanced"), new AdvancedPage(cur));
			addPage(T_("Experts only"), new Advanced3Page(cur), item);
			addPage(T_("User Commands"), new UCPage(cur), item);
			addPage(T_("Security Certificates"), new CertificatesPage(cur), item);
			addPage(T_("Search Types"), new SearchTypesPage(cur), item);
		}
	}

	{
		Grid::Seed gs(1, 1);
		gs.style |= WS_BORDER;
		auto cur = grid->addChild(gs);
		cur->column(0).mode = GridInfo::FILL;

		auto ts = WinUtil::Seeds::Dialog::richTextBox;
		ts.style &= ~ES_SUNKEN;
		ts.exStyle &= ~WS_EX_CLIENTEDGE;
		help = cur->addChild(ts);
		help->onRaw(std::bind(&helpDlgCode, _1), dwt::Message(WM_GETDLGCODE));
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 3));
		cur->setSpacing(8);
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;

		WinUtil::addDlgButtons(cur,
			std::bind(&SettingsDialog::handleOKClicked, this),
			std::bind(&SettingsDialog::endDialog, this, IDCANCEL));

		Button::Seed seed(T_("Help"));
		seed.padding.x = 10;
		ButtonPtr button = cur->addChild(seed);
		button->setHelpId(IDH_DCPP_HELP);
		button->setImage(WinUtil::buttonIcon(IDI_HELP));
		button->onClicked(std::bind(&SettingsDialog::handleHelp, this, this, IDH_INDEX));
	}

	/*
	* catch WM_SETFOCUS messages (onFocus events) sent to every children of this dialog. the normal
	* way to do it would be to use an Application::Filter, but unfortunately these messages don't
	* go through there but instead are sent directly to the control's wndproc.
	*/
	/// @todo when dwt has better tracking of children, improve this
	::EnumChildWindows(handle(), EnumChildProc, reinterpret_cast<LPARAM>(this));

	addAccel(FCONTROL, VK_TAB, std::bind(&SettingsDialog::handleCtrlTab, this, false));
	addAccel(FCONTROL | FSHIFT, VK_TAB, std::bind(&SettingsDialog::handleCtrlTab, this, true));
	initAccels();

	updateTitle();

	layout();
	centerWindow();

	return false;
}

BOOL CALLBACK SettingsDialog::EnumChildProc(HWND hwnd, LPARAM lParam) {
	SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(lParam);
	dwt::Control* widget = dwt::hwnd_cast<dwt::Control*>(hwnd);
	if(widget && widget != dialog->help) {
		widget->onFocus(std::bind(&SettingsDialog::handleChildHelp, dialog, widget));
		TablePtr table = dynamic_cast<TablePtr>(widget);
		if(table)
			table->onSelectionChanged(std::bind(&SettingsDialog::handleChildHelp, dialog, widget));
	}
	return TRUE;
}

void SettingsDialog::handleChildHelp(dwt::Control* widget) {
	help->setText(Text::toT(WinUtil::getHelpText(widget->getHelpId())));
}

HTREEITEM SettingsDialog::addPage(const tstring& title, PropPage* page, HTREEITEM parent) {
	static_cast<GridPtr>(page->getParent())->setWidget(page, 0, 0);
	pages.push_back(page);
	return tree->insert(title, parent, reinterpret_cast<LPARAM>(page), true);
}

void SettingsDialog::handleHelp(dwt::Control* widget, unsigned id) {
	if(id == IDH_INDEX && currentPage)
		id = currentPage->getHelpId();
	WinUtil::help(widget, id);
}

void SettingsDialog::handleSelectionChanged() {
	PropPage* page = reinterpret_cast<PropPage*>(tree->getData(tree->getSelected()));
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

void SettingsDialog::handleCtrlTab(bool shift) {
	HTREEITEM sel = tree->getSelected();
	HTREEITEM next = 0;
	if(!sel)
		next = tree->getFirst();
	else if(shift) {
		if(sel == tree->getFirst())
			next = tree->getLast();
	} else if(sel == tree->getLast())
		next = tree->getFirst();
	if(!next)
		next = tree->getNext(sel, shift ? TVGN_PREVIOUSVISIBLE : TVGN_NEXTVISIBLE);
	tree->setSelected(next);
}

void SettingsDialog::updateTitle() {
	tstring title = tree->getSelectedText();
	setText(title.empty() ? T_("Settings") : T_("Settings") + _T(" - [") + title + _T("]"));
}

void SettingsDialog::write() {
	for(PageList::iterator i = pages.begin(); i != pages.end(); ++i) {
		(*i)->write();
	}
}

void SettingsDialog::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(8, 8, sz.x - 16, sz.y - 16));
}
