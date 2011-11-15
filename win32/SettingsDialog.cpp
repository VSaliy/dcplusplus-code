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

#include "SettingsDialog.h"

#include <dcpp/SettingsManager.h>

#include <dwt/util/GDI.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/ScrolledContainer.h>

#include "WinUtil.h"

#include "GeneralPage.h"

#include "ConnectivityPage.h"
#include "ConnectivityManualPage.h"
#include "BandwidthLimitPage.h"
#include "ProxyPage.h"

#include "DownloadPage.h"
#include "FavoriteDirsPage.h"
#include "QueuePage.h"

#include "UploadPage.h"

#include "AppearancePage.h"
#include "StylesPage.h"
#include "TabsPage.h"
#include "WindowsPage.h"

#include "NotificationsPage.h"

#include "HistoryPage.h"
#include "LogPage.h"

#include "AdvancedPage.h"
#include "ExpertsPage.h"
#include "UCPage.h"
#include "CertificatesPage.h"
#include "SearchTypesPage.h"

using dwt::Grid;
using dwt::GridInfo;

SettingsDialog::SettingsDialog(dwt::Widget* parent) :
dwt::ModalDialog(parent),
currentPage(0),
grid(0),
tree(0),
help(0)
{
	onInitDialog([this] { return initDialog(); });
	onHelp([this](Control* c, unsigned id) { handleHelp(c, id); });
	onClosing([this] { return handleClosing(); });
}

int SettingsDialog::run() {
	auto sizeVal = [](SettingsManager::IntSetting setting) {
		return std::max(SettingsManager::getInstance()->get(setting), 200);
	};
	create(Seed(dwt::Point(sizeVal(SettingsManager::SETTINGS_WIDTH), sizeVal(SettingsManager::SETTINGS_HEIGHT)),
		WS_SIZEBOX | DS_CONTEXTHELP));
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

	grid = addChild(Grid::Seed(1, 2));
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->setSpacing(8);

	grid->column(0).size = 170;
	grid->column(0).mode = GridInfo::STATIC;
	grid->column(1).mode = GridInfo::FILL;

	{
		auto seed = Tree::Seed();
		seed.style |= WS_BORDER;
		tree = grid->addChild(seed);
		tree->setHelpId(IDH_SETTINGS_TREE);
		tree->setItemHeight(tree->getItemHeight() * 5 / 4);
		tree->onSelectionChanged([this] { handleSelectionChanged(); });
	}

	{
		const dwt::Point size(16, 16);
		dwt::ImageListPtr images(new dwt::ImageList(size));
		tree->setNormalImageList(images);

		auto cur = grid->addChild(Grid::Seed(3, 1));
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->column(0).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		auto container = cur->addChild(dwt::ScrolledContainer::Seed(WS_BORDER));

		const size_t setting = SETTING(SETTINGS_PAGE);
		auto addPage = [&](const tstring& title, PropPage* page, unsigned icon, HTREEITEM parent) -> HTREEITEM {
			auto index = pages.size();
			images->add(dwt::Icon(icon, size));
			page->onVisibilityChanged([=](bool b) { if(b) {
				setSmallIcon(WinUtil::createIcon(icon, 16));
				setLargeIcon(WinUtil::createIcon(icon, 32));
			} });
			auto item = tree->insert(title, parent, 0, true, index);
			if(index == setting)
				callAsync([=] { tree->setSelected(item); tree->ensureVisible(item); });
			pages.push_back(std::make_pair(page, item));
			return item;
		};

		addPage(T_("Personal information"), new GeneralPage(container), IDI_USER, TVI_ROOT);

		{
			HTREEITEM item = addPage(T_("Connectivity"), new ConnectivityPage(container), IDI_CONN_BLUE, TVI_ROOT);
			addPage(T_("Manual configuration"), new ConnectivityManualPage(container), IDI_CONN_GREY, item);
			addPage(T_("Bandwidth limiting"), new BandwidthLimitPage(container), IDI_BW_LIMITER, item);
			addPage(T_("Proxy"), new ProxyPage(container), IDI_PROXY, item);
		}

		{
			HTREEITEM item = addPage(T_("Downloads"), new DownloadPage(container), IDI_DOWNLOAD, TVI_ROOT);
			addPage(T_("Favorites"), new FavoriteDirsPage(container), IDI_FAVORITE_DIRS, item);
			addPage(T_("Queue"), new QueuePage(container), IDI_QUEUE, item);
		}

		addPage(T_("Sharing"), new UploadPage(container), IDI_UPLOAD, TVI_ROOT);

		{
			HTREEITEM item = addPage(T_("Appearance"), new AppearancePage(container), IDI_DCPP, TVI_ROOT);
			addPage(T_("Styles"), new StylesPage(container), IDI_STYLES, item);
			addPage(T_("Tabs"), new TabsPage(container), IDI_TABS, item);
			addPage(T_("Windows"), new WindowsPage(container), IDI_WINDOWS, item);
		}

		addPage(T_("Notifications"), new NotificationsPage(container), IDI_NOTIFICATIONS, TVI_ROOT);

		{
			HTREEITEM item = addPage(T_("History"), new HistoryPage(container), IDI_CLOCK, TVI_ROOT);
			addPage(T_("Logs"), new LogPage(container), IDI_LOGS, item);
		}

		{
			HTREEITEM item = addPage(T_("Advanced"), new AdvancedPage(container), IDI_ADVANCED, TVI_ROOT);
			addPage(T_("Experts only"), new ExpertsPage(container), IDI_EXPERT, item);
			addPage(T_("User commands"), new UCPage(container), IDI_USER_OP, item);
			addPage(T_("Security certificates"), new CertificatesPage(container), IDI_SECURE, item);
			addPage(T_("Search types"), new SearchTypesPage(container), IDI_SEARCH, item);
		}

		Grid::Seed gs(1, 1);
		gs.style |= WS_BORDER;
		auto helpGrid = cur->addChild(gs);
		helpGrid->column(0).mode = GridInfo::FILL;

		auto ts = WinUtil::Seeds::Dialog::richTextBox;
		ts.style &= ~ES_SUNKEN;
		ts.exStyle &= ~WS_EX_CLIENTEDGE;
		ts.lines = 6;
		help = helpGrid->addChild(ts);
		help->onRaw([this](WPARAM w, LPARAM) { return helpDlgCode(w); }, dwt::Message(WM_GETDLGCODE));

		cur = cur->addChild(Grid::Seed(1, 3));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(grid->getSpacing());

		WinUtil::addDlgButtons(cur,
			[this] { handleClosing(); handleOKClicked(); },
			[this] { handleClosing(); endDialog(IDCANCEL); });

		Button::Seed seed(T_("Help"));
		seed.padding.x = 10;
		ButtonPtr button = cur->addChild(seed);
		button->setHelpId(IDH_DCPP_HELP);
		button->setImage(WinUtil::buttonIcon(IDI_HELP));
		button->onClicked([this] { handleHelp(this, IDH_INDEX); });
	}

	/*
	* catch WM_SETFOCUS messages (onFocus events) sent to every children of this dialog. the normal
	* way to do it would be to use an Application::Filter, but unfortunately these messages don't
	* go through there but instead are sent directly to the control's wndproc.
	*/
	/// @todo when dwt has better tracking of children, improve this
	::EnumChildWindows(handle(), EnumChildProc, reinterpret_cast<LPARAM>(this));

	addAccel(FCONTROL, VK_TAB, [this] { handleCtrlTab(false); });
	addAccel(FCONTROL | FSHIFT, VK_TAB, [this] { handleCtrlTab(true); });
	initAccels();

	updateTitle();

	centerWindow();
	onWindowPosChanged([this](const dwt::Rectangle &) { layout(); });

	return false;
}

BOOL CALLBACK SettingsDialog::EnumChildProc(HWND hwnd, LPARAM lParam) {
	SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(lParam);
	dwt::Control* widget = dwt::hwnd_cast<dwt::Control*>(hwnd);
	if(widget && widget != dialog->help) {
		widget->onFocus([=] { dialog->handleChildHelp(widget); });
		TablePtr table = dynamic_cast<TablePtr>(widget);
		if(table)
			table->onSelectionChanged([=] { dialog->handleChildHelp(widget); });
	}
	return TRUE;
}

void SettingsDialog::handleChildHelp(dwt::Control* widget) {
	help->setText(Text::toT(WinUtil::getHelpText(widget->getHelpId())));
}

void SettingsDialog::handleHelp(dwt::Control* widget, unsigned id) {
	if(id == IDH_INDEX && currentPage)
		id = currentPage->getHelpId();
	WinUtil::help(widget, id);
}

bool SettingsDialog::handleClosing() {
	dwt::Point pt = getWindowSize();
	SettingsManager::getInstance()->set(SettingsManager::SETTINGS_WIDTH,
		static_cast<int>(static_cast<float>(pt.x) / dwt::util::dpiFactor()));
	SettingsManager::getInstance()->set(SettingsManager::SETTINGS_HEIGHT,
		static_cast<int>(static_cast<float>(pt.y) / dwt::util::dpiFactor()));

	SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PAGE,
		static_cast<int>(find_if(pages.begin(), pages.end(), CompareFirst<PropPage*, HTREEITEM>(currentPage)) - pages.begin()));

	return true;
}

void SettingsDialog::handleSelectionChanged() {
	auto sel = tree->getSelected();
	if(sel) {
		auto page = find_if(pages.begin(), pages.end(), CompareSecond<PropPage*, HTREEITEM>(sel))->first;

		// move to the top of the Z order so the ScrolledContainer thinks this is the only child.
		::SetWindowPos(page->handle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
		page->setVisible(true);

		if(currentPage) {
			currentPage->setVisible(false);
		}
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
	tstring title;
	auto item = tree->getSelected();
	while(item) {
		title = _T(" > ") + tree->getText(item) + title;
		item = tree->getParent(item);
	}
	setText(T_("Settings") + title);
}

void SettingsDialog::write() {
	for(PageList::iterator i = pages.begin(); i != pages.end(); ++i) {
		i->first->write();
	}
}

void SettingsDialog::layout() {
	dwt::Point sz = getClientSize();
	grid->resize(dwt::Rectangle(8, 8, sz.x - 16, sz.y - 16));

	currentPage->getParent()->layout();
}
