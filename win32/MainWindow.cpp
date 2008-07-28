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

#include "MainWindow.h"
#include "resource.h"

#include "LineDlg.h"
#include "HashProgressDlg.h"
#include "SettingsDialog.h"
#include "TextFrame.h"
#include "SingleInstance.h"
#include "AboutDlg.h"
#include "UPnP.h"
#include "TransferView.h"
#include "HubFrame.h"
#include "PrivateFrame.h"
#include "DirectoryListingFrame.h"
#include "SearchFrame.h"
#include "ADLSearchFrame.h"
#include "FavHubsFrame.h"
#include "FinishedDLFrame.h"
#include "FinishedULFrame.h"
#include "NotepadFrame.h"
#include "PublicHubsFrame.h"
#include "QueueFrame.h"
#include "SearchFrame.h"
#include "SpyFrame.h"
#include "StatsFrame.h"
#include "SystemFrame.h"
#include "UsersFrame.h"
#include "WaitingUsersFrame.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/ResourceManager.h>
#include <dcpp/version.h>
#include <dcpp/DownloadManager.h>
#include <dcpp/UploadManager.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/Client.h>
#include <dcpp/TimerManager.h>
#include <dcpp/SearchManager.h>
#include <dcpp/ConnectionManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/Download.h>

#include <dwt/widgets/ToolBar.h>
#include <dwt/widgets/Spinner.h>
#include <dwt/widgets/Notification.h>
#include <dwt/LibraryLoader.h>
#include <dwt/util/StringUtils.h>

MainWindow::MainWindow() :
	WidgetFactory<dwt::Window>(0),
	paned(0),
	transfers(0),
	toolbar(0),
	tabs(0),
	slotsSpin(0),
	maximized(false),
	c(0),
	stopperThread(NULL),
	lastUp(0),
	lastDown(0),
	lastTick(GET_TICK())
{
	links.homepage = _T("http://dcplusplus.sourceforge.net/");
	links.downloads = links.homepage + _T("download/");
	links.geoipfile = _T("http://www.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip");
	links.faq = links.homepage + _T("faq/");
	links.help = links.homepage + _T("help/");
	links.discuss = links.homepage + _T("discussion/");
	links.features = links.homepage + _T("featurereq/");
	links.bugs = links.homepage + _T("bugs/");
	links.donate = _T("https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=arnetheduck%40gmail%2ecom&item_name=DCPlusPlus&no_shipping=1&return=http%3a%2f%2fdcplusplus%2esf%2enet%2f&cancel_return=http%3a%2f%2fdcplusplus%2esf%2enet%2f&cn=Greeting&tax=0&currency_code=EUR&bn=PP%2dDonationsBF&charset=UTF%2d8");

	initWindow();
	initTabs();
	initMenu();
	initToolbar();
	initStatusBar();
	initTransfers();
	initSecond();
	initTray();

	onActivate(std::tr1::bind(&MainWindow::handleActivate, this, _1));
	onSized(std::tr1::bind(&MainWindow::handleSized, this, _1));
	onHelp(std::tr1::bind(&WinUtil::help, _1, _2));
	updateStatus();
	layout();

	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);

	onClosing(std::tr1::bind(&MainWindow::closing, this));

	onRaw(std::tr1::bind(&MainWindow::handleEndSession, this), dwt::Message(WM_ENDSESSION));
	onRaw(std::tr1::bind(&MainWindow::handleCopyData, this, _2), dwt::Message(WM_COPYDATA));
	onRaw(std::tr1::bind(&MainWindow::handleWhereAreYou, this), dwt::Message(SingleInstance::WMU_WHERE_ARE_YOU));

	// commands used in the accelerator table
	onCommand(std::tr1::bind(&QueueFrame::openWindow, getTabView()), IDC_QUEUE);
	onCommand(std::tr1::bind(&MainWindow::handleRefreshFileList, this), IDC_REFRESH_FILE_LIST);
	onCommand(std::tr1::bind(&FavHubsFrame::openWindow, getTabView()), IDC_FAVORITE_HUBS);
	onCommand(std::tr1::bind(&MainWindow::handleOpenFileList, this), IDC_OPEN_FILE_LIST);
	onCommand(std::tr1::bind(&NotepadFrame::openWindow, getTabView()), IDC_NOTEPAD);
	onCommand(std::tr1::bind(&PublicHubsFrame::openWindow, getTabView()), IDC_PUBLIC_HUBS);
	onCommand(std::tr1::bind(&MainWindow::handleQuickConnect, this), IDC_QUICK_CONNECT);
	onCommand(std::tr1::bind(&MainWindow::handleForward, this, IDC_RECONNECT), IDC_RECONNECT);
	onCommand(std::tr1::bind(&SearchFrame::openWindow, getTabView(), Util::emptyStringT, SearchManager::TYPE_ANY), IDC_SEARCH);
	onCommand(std::tr1::bind(&MainWindow::handleForward, this, IDC_FOLLOW), IDC_FOLLOW);
	onCommand(std::tr1::bind(&UsersFrame::openWindow, getTabView()), IDC_FAVUSERS);

	filterIter = dwt::Application::instance().addFilter(std::tr1::bind(&MainWindow::filter, this, _1));
	accel = dwt::AcceleratorPtr(new dwt::Accelerator(this, IDR_MAINFRAME));

	TimerManager::getInstance()->start();

	c = new HttpConnection;
	c->addListener(this);
	c->downloadFile("http://dcplusplus.sourceforge.net/version.xml");

	File::ensureDirectory(SETTING(LOG_DIRECTORY));
	startSocket();

	if(BOOLSETTING(OPEN_SYSTEM_LOG)) SystemFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_FAVORITE_USERS)) UsersFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_QUEUE)) QueueFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_FINISHED_DOWNLOADS)) FinishedDLFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_WAITING_USERS)) WaitingUsersFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_FINISHED_UPLOADS)) FinishedULFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_SEARCH_SPY)) SpyFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_NETWORK_STATISTICS)) StatsFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_NOTEPAD)) NotepadFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_PUBLIC)) PublicHubsFrame::openWindow(getTabView());
	if(BOOLSETTING(OPEN_FAVORITE_HUBS)) FavHubsFrame::openWindow(getTabView());

	if (!WinUtil::isShift()) {
		dwt::Application::instance().callAsync(std::tr1::bind(&MainWindow::autoConnect, this));
	}

	dwt::Application::instance().callAsync(std::tr1::bind(&MainWindow::parseCommandLine, this, tstring(::GetCommandLine())));

	int cmdShow = dwt::Application::instance().getCmdShow();
	::ShowWindow(handle(), ((cmdShow == SW_SHOWDEFAULT) || (cmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : cmdShow);

	if(SETTING(NICK).empty()) {
		WinUtil::help(handle(), IDH_GET_STARTED);
		handleSettings();
	}

	if(dwt::LibraryLoader::getCommonControlsVersion() < PACK_COMCTL_VERSION(5,80))
		createMessageBox().show(T_("Your version of windows common controls is too old for DC++ to run correctly, and you will most probably experience problems with the user interface. You should download version 5.80 or higher from the DC++ homepage or from Microsoft directly."), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONEXCLAMATION);
}

void MainWindow::initWindow() {
	// Create main window
	dcdebug("initWindow\n");

	Seed cs(_T(APPNAME) _T(" ") _T(VERSIONSTRING));

	int pos_x = SETTING(MAIN_WINDOW_POS_X);
	int pos_y = SETTING(MAIN_WINDOW_POS_Y);
	int size_x = SETTING(MAIN_WINDOW_SIZE_X);
	int size_y = SETTING(MAIN_WINDOW_SIZE_Y);
	if ( (pos_x != static_cast<int>(CW_USEDEFAULT)) &&(pos_y != static_cast<int>(CW_USEDEFAULT))&&(size_x
	    != static_cast<int>(CW_USEDEFAULT))&&(size_y != static_cast<int>(CW_USEDEFAULT))&&(pos_x > 0&& pos_y > 0)
	    &&(size_x > 10&& size_y > 10)) {
		cs.location = dwt::Rectangle(pos_x, pos_y, size_x, size_y);
	}

	cs.exStyle |= WS_EX_APPWINDOW;
	if (ResourceManager::getInstance()->isRTL())
		cs.exStyle |= WS_EX_RTLREADING;

	cs.icon = dwt::IconPtr(new dwt::Icon(IDR_MAINFRAME));
	cs.background = (HBRUSH)(COLOR_3DFACE + 1);
	create(cs);

	setHelpId(IDH_MAIN);

	paned = addChild(WidgetHPaned::Seed(SETTING(TRANSFERS_PANED_POS)));
}

void MainWindow::initMenu() {
	dcdebug("initMenu\n");

	{
		Menu::Seed cs = WinUtil::Seeds::menu;
		cs.popup = false;
		mainMenu = addChild(cs);
	}

	{
		MenuPtr file = mainMenu->appendPopup(T_("&File"));

		file->appendItem(T_("&Quick Connect ...\tCtrl+Q"), std::tr1::bind(&MainWindow::handleQuickConnect, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_HUB)));
		file->appendItem(T_("&Reconnect\tCtrl+R"), std::tr1::bind(&MainWindow::handleForward, this, IDC_RECONNECT), dwt::BitmapPtr(new dwt::Bitmap(IDB_RECONNECT)));
		file->appendItem(T_("Follow last redirec&t\tCtrl+T"), std::tr1::bind(&MainWindow::handleForward, this, IDC_FOLLOW), dwt::BitmapPtr(new dwt::Bitmap(IDB_FOLLOW)));
		file->appendSeparator();

		file->appendItem(T_("Open file list...\tCtrl+L"), std::tr1::bind(&MainWindow::handleOpenFileList, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_OPEN_FILE_LIST)));
		file->appendItem(T_("Open own list"), std::tr1::bind(&MainWindow::handleOpenOwnList, this));
		file->appendItem(T_("Match downloaded lists"), std::tr1::bind(&MainWindow::handleMatchAll, this));
		file->appendItem(T_("Refresh file list\tCtrl+E"), std::tr1::bind(&MainWindow::handleRefreshFileList, this));
		file->appendItem(T_("Open downloads directory"), std::tr1::bind(&MainWindow::handleOpenDownloadsDir, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_OPEN_DL_DIR)));
		file->appendSeparator();

		file->appendItem(T_("Settings..."), std::tr1::bind(&MainWindow::handleSettings, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_SETTINGS)));
		file->appendSeparator();
		file->appendItem(T_("E&xit"), std::tr1::bind(&MainWindow::handleExit, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_EXIT)));
	}

	{
		MenuPtr view = mainMenu->appendPopup(T_("&View"));

		view->appendItem(T_("&Public Hubs\tCtrl+P"), std::tr1::bind(&PublicHubsFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_PUBLIC_HUBS)));
		view->appendItem(T_("&Favorite Hubs\tCtrl+F"), std::tr1::bind(&FavHubsFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_FAVORITE_HUBS)));
		view->appendItem(T_("Favorite &Users\tCtrl+U"), std::tr1::bind(&UsersFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_FAVORITE_USERS)));
		view->appendSeparator();
		view->appendItem(T_("&Download Queue\tCtrl+D"), std::tr1::bind(&QueueFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_DL_QUEUE)));
		view->appendItem(T_("Finished Downloads"), std::tr1::bind(&FinishedDLFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_FINISHED_DL)));
		view->appendItem(T_("Waiting Users"), std::tr1::bind(&WaitingUsersFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_WAITING_USERS)));
		view->appendItem(T_("Finished Uploads"), std::tr1::bind(&FinishedULFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_FINISHED_UL)));
		view->appendSeparator();
		view->appendItem(T_("&Search\tCtrl+S"), std::tr1::bind(&SearchFrame::openWindow, getTabView(), Util::emptyStringT, SearchManager::TYPE_ANY), dwt::BitmapPtr(new dwt::Bitmap(IDB_SEARCH)));
		view->appendItem(T_("ADL Search"), std::tr1::bind(&ADLSearchFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_ADL_SEARCH)));
		view->appendItem(T_("Search Spy"), std::tr1::bind(&SpyFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_SEARCH_SPY)));
		view->appendSeparator();
		view->appendItem(T_("&Notepad\tCtrl+N"), std::tr1::bind(&NotepadFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_NOTEPAD)));
		view->appendItem(T_("System Log"), std::tr1::bind(&SystemFrame::openWindow, getTabView()));
		view->appendItem(T_("Network Statistics"), std::tr1::bind(&StatsFrame::openWindow, getTabView()), dwt::BitmapPtr(new dwt::Bitmap(IDB_NETWORK_STATS)));
		view->appendItem(T_("Indexing progress"), std::tr1::bind(&MainWindow::handleHashProgress, this));
	}

	{
		MenuPtr window = mainMenu->appendPopup(T_("&Window"));

		window->appendItem(T_("Close disconnected"), std::tr1::bind(&HubFrame::closeDisconnected));
		window->appendItem(T_("Close all PM windows"), std::tr1::bind(&PrivateFrame::closeAll));
		window->appendItem(T_("Close all offline PM windows"), std::tr1::bind(&PrivateFrame::closeAllOffline));
		window->appendItem(T_("Close all file list windows"), std::tr1::bind(&DirectoryListingFrame::closeAll));
		window->appendItem(T_("Close all search windows"), std::tr1::bind(&SearchFrame::closeAll));
	}

	{
		MenuPtr help = mainMenu->appendPopup(T_("&Help"));

		help->appendItem(T_("Help &Contents\tF1"), std::tr1::bind(&WinUtil::help, handle(), IDH_INDEX), dwt::BitmapPtr(new dwt::Bitmap(IDB_HELP)));
		help->appendItem(T_("Get started"), std::tr1::bind(&WinUtil::help, handle(), IDH_GET_STARTED));
		help->appendSeparator();
		help->appendItem(T_("Change Log"), std::tr1::bind(&WinUtil::help, handle(), IDH_CHANGELOG));
		help->appendItem(T_("About DC++..."), std::tr1::bind(&MainWindow::handleAbout, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_DCPP)));
		help->appendSeparator();
		help->appendItem(T_("DC++ Homepage"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.homepage)));
		help->appendItem(T_("Downloads"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.downloads)));
		help->appendItem(T_("GeoIP database update"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.geoipfile)));
		help->appendItem(T_("Frequently asked questions"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.faq)));
		help->appendItem(T_("Help forum"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.help)));
		help->appendItem(T_("DC++ discussion forum"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.discuss)));
		help->appendItem(T_("Request a feature"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.features)));
		help->appendItem(T_("Report a bug"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.bugs)));
		help->appendItem(T_("Donate (paypal)"), std::tr1::bind(&WinUtil::openLink, std::tr1::cref(links.donate)));
	}

	mainMenu->setMenu();
}

void MainWindow::initToolbar() {
	dcdebug("initToolbar\n");

	ToolBar::Seed cs;
	cs.style |= TBSTYLE_FLAT;
	toolbar = addChild(cs);
	{
		dwt::ImageListPtr list(new dwt::ImageList(20, 20, ILC_COLOR32 | ILC_MASK));
		dwt::Bitmap bmp(IDB_TOOLBAR20);
		list->add(bmp, RGB(255, 0, 255));

		toolbar->setNormalImageList(list);
	}
	{
		dwt::ImageListPtr list(new dwt::ImageList(20, 20, ILC_COLOR32 | ILC_MASK));
		dwt::Bitmap bmp(IDB_TOOLBAR20_HOT);
		list->add(bmp, RGB(255, 0, 255));

		toolbar->setHotImageList(list);
	}

	int image = 0;
	toolbar->appendItem(image++, T_("Public Hubs"), IDH_TOOLBAR_PUBLIC_HUBS, std::tr1::bind(&PublicHubsFrame::openWindow, getTabView()));
	toolbar->appendSeparator();
	toolbar->appendItem(image++, T_("Reconnect"), IDH_TOOLBAR_RECONNECT, std::tr1::bind(&MainWindow::handleForward, this, IDC_RECONNECT));
	toolbar->appendItem(image++, T_("Follow last redirect"), IDH_TOOLBAR_FOLLOW, std::tr1::bind(&MainWindow::handleForward, this, IDC_FOLLOW));
	toolbar->appendSeparator();
	toolbar->appendItem(image++, T_("Favorite Hubs"), IDH_TOOLBAR_FAVORITE_HUBS, std::tr1::bind(&FavHubsFrame::openWindow, getTabView()), std::tr1::bind(&MainWindow::handleFavHubsDropDown, this, _1));
	toolbar->appendItem(image++, T_("Favorite Users"), IDH_TOOLBAR_FAVORITE_USERS, std::tr1::bind(&UsersFrame::openWindow, getTabView()));
	toolbar->appendSeparator();
	toolbar->appendItem(image++, T_("Download Queue"), IDH_TOOLBAR_QUEUE, std::tr1::bind(&QueueFrame::openWindow, getTabView()));
	toolbar->appendItem(image++, T_("Finished Downloads"), IDH_TOOLBAR_FINISHED_DL, std::tr1::bind(&FinishedDLFrame::openWindow, getTabView()));
	toolbar->appendItem(image++, T_("Waiting Users"), IDH_TOOLBAR_WAITING_USERS, std::tr1::bind(&WaitingUsersFrame::openWindow, getTabView()));
	toolbar->appendItem(image++, T_("Finished Uploads"), IDH_TOOLBAR_FINISHED_UL, std::tr1::bind(&FinishedULFrame::openWindow, getTabView()));
	toolbar->appendSeparator();
	toolbar->appendItem(image++, T_("Search"), IDH_TOOLBAR_SEARCH, std::tr1::bind(&SearchFrame::openWindow, getTabView(), Util::emptyStringT, SearchManager::TYPE_ANY));
	toolbar->appendItem(image++, T_("ADL Search"), IDH_TOOLBAR_ADL_SEARCH, std::tr1::bind(&ADLSearchFrame::openWindow, getTabView()));
	toolbar->appendItem(image++, T_("Search Spy"), IDH_TOOLBAR_SEARCH_SPY, std::tr1::bind(&SpyFrame::openWindow, getTabView()));
	toolbar->appendSeparator();
	toolbar->appendItem(image++, T_("Open file list..."), IDH_TOOLBAR_FILE_LIST, std::tr1::bind(&MainWindow::handleOpenFileList, this));
	toolbar->appendSeparator();
	toolbar->appendItem(image++, T_("Settings"), IDH_TOOLBAR_SETTINGS, std::tr1::bind(&MainWindow::handleSettings, this));
	toolbar->appendItem(image++, T_("Notepad"), IDH_TOOLBAR_NOTEPAD, std::tr1::bind(&NotepadFrame::openWindow, getTabView()));
	toolbar->appendSeparator();
	toolbar->appendItem(image++, T_("What's This?"), IDH_TOOLBAR_WHATS_THIS, std::tr1::bind(&MainWindow::handleWhatsThis, this));

	toolbar->onHelp(std::tr1::bind(&WinUtil::help, _1, _2));
}

void MainWindow::initStatusBar() {
	dcdebug("initStatusBar\n");

	slotsSpin = addChild(Spinner::Seed(1));
	slotsSpin->setHelpId(IDH_MAIN_SLOTS_SPIN);
	updateSlotsSpin();
	slotsSpin->onUpdate(std::tr1::bind(&MainWindow::handleSlotsUpdate, this, _1, _2));

	initStatus(true);
	status->onDblClicked(std::tr1::bind(&WinUtil::openFile, Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM)))));

	statusSizes[STATUS_AWAY] = status->getTextSize(T_("AWAY")).x + 12;
	statusSizes[STATUS_SLOTS_SPIN] = 20;
	///@todo set to checkbox width + resizedrag width really
	statusSizes[STATUS_DUMMY] = 32;

	setStatusHelpId(STATUS_STATUS, IDH_MAIN_STATUS);
	setStatusHelpId(STATUS_AWAY, IDH_MAIN_AWAY);
	setStatusHelpId(STATUS_COUNTS, IDH_MAIN_HUBS);
	setStatusHelpId(STATUS_SLOTS, IDH_MAIN_SLOTS);
	setStatusHelpId(STATUS_DOWN_TOTAL, IDH_MAIN_DOWN_TOTAL);
	setStatusHelpId(STATUS_UP_TOTAL, IDH_MAIN_UP_TOTAL);
	setStatusHelpId(STATUS_DOWN_DIFF, IDH_MAIN_DOWN_DIFF);
	setStatusHelpId(STATUS_UP_DIFF, IDH_MAIN_UP_DIFF);
}

void MainWindow::initTabs() {
	dcdebug("initTabs\n");
	tabs = addChild(dwt::TabView::Seed(SETTING(MAX_TAB_CHARS), BOOLSETTING(TOGGLE_ACTIVE_WINDOW)));
	tabs->onTitleChanged(std::tr1::bind(&MainWindow::handleTabsTitleChanged, this, _1));
	tabs->onHelp(std::tr1::bind(&WinUtil::help, _1, _2));
	paned->setFirst(tabs);
}

void MainWindow::initTransfers() {
	dcdebug("initTransfers\n");
	transfers = new TransferView(this, getTabView());
	paned->setSecond(transfers);
}

void MainWindow::initTray() {
	dcdebug("initTray\n");
	notify = dwt::NotificationPtr(new dwt::Notification(this));
	notify->create(dwt::Notification::Seed::Seed(dwt::IconPtr(new dwt::Icon(IDR_MAINFRAME))));
	notify->onContextMenu(std::tr1::bind(&MainWindow::handleTrayContextMenu, this));
	notify->onIconClicked(std::tr1::bind(&MainWindow::handleTrayClicked, this));
	notify->onUpdateTip(std::tr1::bind(&MainWindow::handleTrayUpdate, this));
	if(SETTING(ALWAYS_TRAY)) {
		notify->setVisible(true);
	}
}

bool MainWindow::filter(MSG& msg) {
	if(tabs && tabs->filter(msg)) {
		return true;
	}

	if(accel && accel->translate(msg)) {
		return true;
	}

	if(::HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, reinterpret_cast<DWORD_PTR>(&msg))) {
		return true;
	}

	Container* active = getTabView()->getActive();
	if(active) {
		if(::IsDialogMessage( active->handle(), & msg )) {
			return true;
		}
	}

	return false;
}

void MainWindow::handleTabsTitleChanged(const tstring& title) {
	setText(title.empty() ? _T(APPNAME) _T(" ") _T(VERSIONSTRING) : _T(APPNAME) _T(" ") _T(VERSIONSTRING) _T(" - [") + title + _T("]"));
}

void MainWindow::handleFavHubsDropDown(const dwt::ScreenCoordinate& pt) {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();
	for(FavoriteHubEntryList::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		FavoriteHubEntry* entry = *i;
		menu->appendItem(dwt::util::escapeMenu(Text::toT(entry->getName())), std::tr1::bind(&HubFrame::openWindow, getTabView(), entry->getServer()));
	}

	menu->open(pt);
}

void MainWindow::handleExit() {
	close(true);
}

void MainWindow::handleForward(WPARAM wParam) {
	Container* active = getTabView()->getActive();
	if(active) {
		active->sendMessage(WM_COMMAND, wParam);
	}
}

void MainWindow::handleQuickConnect() {
	if (SETTING(NICK).empty()) {
		handleSettings();
		return;
	}

	LineDlg dlg(this, T_("Quick Connect"), T_("Address"));

	if (dlg.run() == IDOK) {

		tstring tmp = dlg.getLine();
		// Strip out all the spaces
		string::size_type i;
		while ((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		HubFrame::openWindow(getTabView(), Text::fromT(tmp));
	}
}

void MainWindow::handleSized(const dwt::SizedEvent& sz) {
	if(sz.isMinimized) {
		if(BOOLSETTING(AUTO_AWAY) && !Util::getManualAway()) {
			Util::setAway(true);
		}
		if(BOOLSETTING(MINIMIZE_TRAY) != WinUtil::isShift()) {
			if(!SETTING(ALWAYS_TRAY)) {
				notify->setVisible(true);
			}
			setVisible(false);
		}
		maximized = isZoomed();
	} else if( sz.isMaximized || sz.isRestored ) {
		if(BOOLSETTING(AUTO_AWAY) && !Util::getManualAway()) {
			Util::setAway(false);
		}
		if(!SETTING(ALWAYS_TRAY)) {
			notify->setVisible(false);
		}
		layout();
	}
}

void MainWindow::on(LogManagerListener::Message, time_t t, const string& m) throw() {
	tstring line = Text::toT("[" + Util::getShortTimeString(t) + "] " + m);
	dwt::Application::instance().callAsync(std::tr1::bind(&MainWindow::setStatus, this, STATUS_STATUS, line, true));
}

void MainWindow::viewAndDelete(const string& fileName) {
	TextFrame::openWindow(getTabView(), fileName);
	File::deleteFile(fileName);
}

void MainWindow::autoConnect() {
	const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();
	for(FavoriteHubEntryList::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		FavoriteHubEntry* entry = *i;
		if (entry->getConnect()) {
			if (!entry->getNick().empty() || !SETTING(NICK).empty()) {
				HubFrame::openWindow(getTabView(), entry->getServer());
			}
		}
	}
}

void MainWindow::saveWindowSettings() {
	SettingsManager::getInstance()->set(SettingsManager::TRANSFERS_PANED_POS, paned->getRelativePos());

	WINDOWPLACEMENT wp = { sizeof(wp)};
	::GetWindowPlacement(this->handle(), &wp);
	if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, static_cast<int>(wp.rcNormalPosition.left));
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, static_cast<int>(wp.rcNormalPosition.top));
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, static_cast<int>(wp.rcNormalPosition.right - wp.rcNormalPosition.left));
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, static_cast<int>(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top));
	}
	if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
	SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);
}

bool MainWindow::closing() {
	if (stopperThread == NULL) {
		if ( !BOOLSETTING(CONFIRM_EXIT) || (createMessageBox().show(T_("Really exit?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_YESNO, MessageBox::BOX_ICONQUESTION) == IDYES)) {
			if (c != NULL) {
				c->removeListener(this);
				delete c;
				c = NULL;
			}
			saveWindowSettings();

			setVisible(false);
			transfers->prepareClose();

			LogManager::getInstance()->removeListener(this);
			QueueManager::getInstance()->removeListener(this);

			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();

			stopUPnP();

			DWORD id;
			dcdebug("Starting stopper\n");
			stopperThread = CreateThread(NULL, 0, &stopper, this, 0, &id);
		}
		return false;
	}

	dcdebug("Waiting for stopper\n");
	// This should end immediately, as it only should be the stopper that sends another WM_CLOSE
	WaitForSingleObject(stopperThread, 60*1000);
	CloseHandle(stopperThread);
	stopperThread = NULL;
	::PostQuitMessage(0);
	dcdebug("Quit message posted\n");
	return true;
}

void MainWindow::initSecond() {
	createTimer(std::tr1::bind(&MainWindow::eachSecond, this), 1000);
}

bool MainWindow::eachSecond() {
	updateStatus();
	return true;
}

void MainWindow::layout() {
	dwt::Rectangle r(getClientAreaSize());

	toolbar->refresh();
	dwt::Point pt = toolbar->getSize();
	r.pos.y += pt.y;
	r.size.y -= pt.y;

	layoutStatus(r);
	mapWidget(STATUS_SLOTS_SPIN, slotsSpin);

	paned->setRect(r);
}

LRESULT MainWindow::handleWhereAreYou() {
	return SingleInstance::WMU_WHERE_ARE_YOU;
}

void MainWindow::updateStatus() {
	uint64_t now= GET_TICK();
	uint64_t tdiff = now - lastTick;
	if (tdiff < 100) {
		tdiff = 1;
	}

	uint64_t up = Socket::getTotalUp();
	uint64_t down = Socket::getTotalDown();
	uint64_t updiff = up - lastUp;
	uint64_t downdiff = down - lastDown;

	lastTick = now;
	lastUp = up;
	lastDown = down;

	/** @todo move this to client/ */
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + static_cast<int64_t>(updiff));
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + static_cast<int64_t>(downdiff));

	bool statusResized = false;
	statusResized |= setStatus(STATUS_AWAY, Util::getAway() ? T_("AWAY") : _T(""), false);
	statusResized |= setStatus(STATUS_COUNTS, Text::toT(Client::getCounts()), false);
	statusResized |= setStatus(STATUS_SLOTS, str(TF_("Slots: %1%/%2%") % UploadManager::getInstance()->getFreeSlots() % (SETTING(SLOTS))), false);
	statusResized |= setStatus(STATUS_DOWN_TOTAL, str(TF_("D: %1%") % Text::toT(Util::formatBytes(down))), false);
	statusResized |= setStatus(STATUS_UP_TOTAL, str(TF_("U: %1%") % Text::toT(Util::formatBytes(up))), false);
	statusResized |= setStatus(STATUS_DOWN_DIFF, str(TF_("D: %1%/s (%2%)") % Text::toT(Util::formatBytes((downdiff*1000)/tdiff)) % DownloadManager::getInstance()->getDownloadCount()), false);
	statusResized |= setStatus(STATUS_UP_DIFF, str(TF_("U: %1%/s (%2%)") % Text::toT(Util::formatBytes((updiff*1000)/tdiff)) % UploadManager::getInstance()->getUploadCount()), false);
	if(statusResized)
		layout();
}

MainWindow::~MainWindow() {
	dwt::Application::instance().removeFilter(filterIter);
}

void MainWindow::updateSlotsSpin() {
	slotsSpin->setValue(SETTING(SLOTS));
}

void MainWindow::handleSettings() {
	SettingsDialog dlg(this);

	unsigned short lastTCP = static_cast<unsigned short>(SETTING(TCP_PORT));
	unsigned short lastUDP = static_cast<unsigned short>(SETTING(UDP_PORT));
	unsigned short lastTLS = static_cast<unsigned short>(SETTING(TLS_PORT));

	int lastConn= SETTING(INCOMING_CONNECTIONS);
	int lastSlots = SETTING(SLOTS);
	bool lastSortFavUsersFirst= BOOLSETTING(SORT_FAVUSERS_FIRST);

	if (dlg.run() == IDOK) {
		SettingsManager::getInstance()->save();
		if (SETTING(INCOMING_CONNECTIONS) != lastConn || SETTING(TCP_PORT) != lastTCP || SETTING(UDP_PORT) != lastUDP || SETTING(TLS_PORT) != lastTLS) {
			startSocket();
		}
		ClientManager::getInstance()->infoUpdated();

		if(SETTING(SLOTS) != lastSlots)
			updateSlotsSpin();

		if(BOOLSETTING(SORT_FAVUSERS_FIRST) != lastSortFavUsersFirst)
			HubFrame::resortUsers();

		if(BOOLSETTING(URL_HANDLER)) {
			WinUtil::registerDchubHandler();
			WinUtil::registerADChubHandler();
			WinUtil::urlDcADCRegistered = true;
		} else if(WinUtil::urlDcADCRegistered) {
			WinUtil::unRegisterDchubHandler();
			WinUtil::unRegisterADChubHandler();
			WinUtil::urlDcADCRegistered = false;
		}
		if(BOOLSETTING(MAGNET_REGISTER)) {
			WinUtil::registerMagnetHandler();
			WinUtil::urlMagnetRegistered = true;
		} else if(WinUtil::urlMagnetRegistered) {
			WinUtil::unRegisterMagnetHandler();
			WinUtil::urlMagnetRegistered = false;
		}
	}
}

void MainWindow::startSocket() {
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();

	if (ClientManager::getInstance()->isActive()) {
		try {
			ConnectionManager::getInstance()->listen();
		} catch(const Exception&) {
			createMessageBox().show(T_("Unable to open TCP/TLS port. File transfers will not work correctly until you change settings or turn off any application that might be using the TCP/TLS port"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONSTOP);
		}
		try {
			SearchManager::getInstance()->listen();
		} catch(const Exception&) {
			createMessageBox().show(T_("Unable to open UDP port. Searching will not work correctly until you change settings or turn off any application that might be using the UDP port"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONSTOP);
		}
	}

	startUPnP();
}

void MainWindow::startUPnP() {
	stopUPnP();

	if( SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP ) {
		bool ok = true;

		uint16_t port = ConnectionManager::getInstance()->getPort();
		if(port != 0) {
			UPnP_TCP.reset(new UPnP( Util::getLocalIp(), "TCP", str(F_(APPNAME " Transfer Port (%1% TCP)") % port), port));
			ok &= UPnP_TCP->open();
		}
		port = ConnectionManager::getInstance()->getSecurePort();
		if(ok && port != 0) {
			UPnP_TLS.reset(new UPnP( Util::getLocalIp(), "TCP", str(F_(APPNAME " Encrypted Transfer Port (%1% TCP)") % port), port));
			ok &= UPnP_TLS->open();
		}
		port = SearchManager::getInstance()->getPort();
		if(ok && port != 0) {
			UPnP_UDP.reset(new UPnP( Util::getLocalIp(), "UDP", str(F_(APPNAME " Search Port (%1% UDP)") % port), port));
			ok &= UPnP_UDP->open();
		}

		if(ok) {
			if(!BOOLSETTING(NO_IP_OVERRIDE)) {
				// now lets configure the external IP (connect to me) address
				string ExternalIP = UPnP_TCP->GetExternalIP();
				if ( !ExternalIP.empty() ) {
					// woohoo, we got the external IP from the UPnP framework
					SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, ExternalIP );
				} else {
					//:-( Looks like we have to rely on the user setting the external IP manually
					// no need to do cleanup here because the mappings work
					LogManager::getInstance()->message(_("Failed to get external IP via  UPnP. Please set it yourself."));
					createMessageBox().show(T_("Failed to get external IP via  UPnP. Please set it yourself."), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
				}
			}
		} else {
			LogManager::getInstance()->message(_("Failed to create port mappings. Please set up your NAT yourself."));
			createMessageBox().show(T_("Failed to create port mappings. Please set up your NAT yourself."), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
			stopUPnP();
		}
	}
}

void MainWindow::stopUPnP() {
	// Just check if the port mapping objects are initialized (NOT NULL)
	if(UPnP_TCP.get()) {
		if(!UPnP_TCP->close()) {
			LogManager::getInstance()->message(_("Failed to remove port mappings"));
		}
		UPnP_TCP.reset();
	}
	if(UPnP_TLS.get()) {
		if(!UPnP_TLS->close()) {
			LogManager::getInstance()->message(_("Failed to remove port mappings"));
		}
		UPnP_TLS.reset();
	}
	if(UPnP_UDP.get()) {
		if(!UPnP_UDP->close()) {
			LogManager::getInstance()->message(_("Failed to remove port mappings"));
		}
		UPnP_UDP.reset();
	}
}

void MainWindow::handleOpenFileList() {
	tstring file;
	if(WinUtil::browseFileList(createLoadDialog(), file)) {
		UserPtr u = DirectoryListing::getUserFromFilename(Text::fromT(file));
		if (u) {
			DirectoryListingFrame::openWindow(getTabView(), file, Text::toT(Util::emptyString), u, 0);
		} else {
			createMessageBox().show(T_("Invalid file list name"), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
		}
	}
}

void MainWindow::handleOpenOwnList() {
	if (!ShareManager::getInstance()->getOwnListFile().empty()) {
		DirectoryListingFrame::openWindow(getTabView(), Text::toT(ShareManager::getInstance()->getOwnListFile()), Text::toT(Util::emptyString), ClientManager::getInstance()->getMe(), 0);
	}
}

DWORD WINAPI MainWindow::stopper(void* p) {
	MainWindow* mf = reinterpret_cast<MainWindow*>(p);
	HWND wnd, wnd2 = NULL;

	while( (wnd=::GetWindow(mf->getTabView()->handle(), GW_CHILD)) != NULL) {
		if(wnd == wnd2) {
			::Sleep(100);
		} else {
			::PostMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}

	::PostMessage(mf->handle(), WM_CLOSE, 0, 0);
	return 0;
}

class ListMatcher : public Thread {
public:
	ListMatcher(StringList files_) :
		files(files_) {

	}
	virtual int run() {
		for (StringIter i = files.begin(); i != files.end(); ++i) {
			UserPtr u = DirectoryListing::getUserFromFilename(*i);
			if (!u)
				continue;
			DirectoryListing dl(u);
			try {
				dl.loadFile(*i);
				LogManager::getInstance()->message(str(FN_("%1%: matched %2% file", "%1%: matched %2% files", QueueManager::getInstance()->matchListing(dl))
				% Util::toString(ClientManager::getInstance()->getNicks(u->getCID()))
				% QueueManager::getInstance()->matchListing(dl)));
			} catch(const Exception&) {

			}
		}
		delete this;
		return 0;
	}
	StringList files;
};

void MainWindow::handleMatchAll() {
	ListMatcher* matcher = new ListMatcher(File::findFiles(Util::getListPath(), "*.xml*"));
	try {
		matcher->start();
	} catch(const ThreadException&) {
		///@todo add error message
		delete matcher;
	}
}

void MainWindow::handleActivate(bool active) {
	// Forward to active tab window
	Container* w = getTabView()->getActive();
	if(w) {
		w->sendMessage(WM_ACTIVATE, active ? WA_ACTIVE : WA_INACTIVE);
	}
}

void MainWindow::parseCommandLine(const tstring& cmdLine)
{
	string::size_type i = 0;
	string::size_type j;

	if( (j = cmdLine.find(_T("dchub://"), i)) != string::npos) {
		WinUtil::parseDchubUrl(cmdLine.substr(j));
	}
	if( (j = cmdLine.find(_T("adc://"), i)) != string::npos) {
		WinUtil::parseADChubUrl(cmdLine.substr(j));
	}
	if( (j = cmdLine.find(_T("magnet:?"), i)) != string::npos) {
		WinUtil::parseMagnetUri(cmdLine.substr(j));
	}
}

LRESULT MainWindow::handleCopyData(LPARAM lParam) {
	parseCommandLine(Text::toT(WinUtil::getAppName() + " ") + reinterpret_cast<LPCTSTR>(reinterpret_cast<COPYDATASTRUCT*>(lParam)->lpData));
	return TRUE;
}

void MainWindow::handleHashProgress() {
	HashProgressDlg(this, false).run();
}

void MainWindow::handleAbout() {
	AboutDlg(this).run();
}

void MainWindow::handleOpenDownloadsDir() {
	WinUtil::openFile(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
}

void MainWindow::on(HttpConnectionListener::Complete, HttpConnection* /*aConn*/, const string&) throw() {
	try {
		SimpleXML xml;
		xml.fromXML(versionInfo);
		xml.stepIn();

		string url = Text::fromT(links.homepage);

		if(xml.findChild("URL")) {
			url = xml.getChildData();
		}

		xml.resetCurrentChild();
		if(xml.findChild("Version")) {
			if(Util::toDouble(xml.getChildData()) > VERSIONFLOAT) {
				xml.resetCurrentChild();
				if(xml.findChild("Title")) {
					const string& title = xml.getChildData();
					xml.resetCurrentChild();
					if(xml.findChild("Message")) {
						if(url.empty()) {
							const string& msg = xml.getChildData();
							createMessageBox().show(Text::toT(msg), Text::toT(title));
						} else {
							if(createMessageBox().show(str(TF_("%1%\nOpen download page?") % Text::toT(xml.getChildData())), Text::toT(title), MessageBox::BOX_YESNO, MessageBox::BOX_ICONQUESTION) == IDYES) {
								WinUtil::openLink(Text::toT(url));
							}
						}
					}
				}
			}
		}
		xml.resetCurrentChild();
		if(xml.findChild("Links")) {
			xml.stepIn();
			if(xml.findChild("Homepage")) {
				links.homepage = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Downloads")) {
				links.downloads = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("GeoIP database update")) {
				links.geoipfile = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Faq")) {
				links.faq = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Bugs")) {
				links.bugs = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Features")) {
				links.features = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Help")) {
				links.help = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Forum")) {
				links.discuss = Text::toT(xml.getChildData());
			}
			xml.stepOut();
		}
		xml.stepOut();
	} catch (const Exception&) {
		// ...
	}
}

LRESULT MainWindow::handleEndSession() {
	if (c != NULL) {
		c->removeListener(this);
		delete c;
		c = NULL;
	}

	saveWindowSettings();
	QueueManager::getInstance()->saveQueue();
	SettingsManager::getInstance()->save();

	return 0;
}

bool MainWindow::handleSlotsUpdate(int pos, int delta) {
	SettingsManager::getInstance()->set(SettingsManager::SLOTS, pos + delta);
	updateStatus();
	ClientManager::getInstance()->infoUpdated();
	return true;
}

void MainWindow::handleRefreshFileList() {
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true);
}

void MainWindow::handleRestore() {
	setVisible(true);
	if(maximized) {
		maximize();
	} else {
		restore();
	}
}

bool MainWindow::tryFire(const MSG& msg, LRESULT& retVal) {
	bool handled = dwt::Window::tryFire(msg, retVal);
	if(!handled && msg.message == WM_COMMAND && tabs) {
		handled = tabs->tryFire(msg, retVal);
	}
	return handled;
}

void MainWindow::handleTrayContextMenu() {
	MenuPtr trayMenu = addChild(WinUtil::Seeds::menu);

	trayMenu->appendItem(T_("Show"), std::tr1::bind(&MainWindow::handleRestore, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_DCPP)), true, true);
	trayMenu->appendItem(T_("Open downloads directory"), std::tr1::bind(&MainWindow::handleOpenDownloadsDir, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_OPEN_DL_DIR)));
	trayMenu->appendItem(T_("Settings..."), std::tr1::bind(&MainWindow::handleSettings, this), dwt::BitmapPtr(new dwt::Bitmap(IDB_SETTINGS)));
	trayMenu->appendSeparator();
	trayMenu->appendItem(T_("Exit"), std::tr1::bind(&MainWindow::close, this, true), dwt::BitmapPtr(new dwt::Bitmap(IDB_EXIT)));

	dwt::ScreenCoordinate pt;
	::GetCursorPos(&pt.getPoint());
	trayMenu->open(pt, TPM_BOTTOMALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON);
}

void MainWindow::handleTrayClicked() {
	if(getVisible()) {
		minimize();
	} else {
		handleRestore();
	}
}

void MainWindow::handleTrayUpdate() {
	notify->setTooltip(Text::toT(str(F_("D: %1%/s (%2%)\r\nU: %3%/s (%4%)") %
		Util::formatBytes(DownloadManager::getInstance()->getRunningAverage()) %
		DownloadManager::getInstance()->getDownloadCount() %
		Util::formatBytes(UploadManager::getInstance()->getRunningAverage()) %
		UploadManager::getInstance()->getUploadCount())));
}

#ifdef PORT_ME
LRESULT MainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE; // initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1); // toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !transferView.IsWindowVisible();
	if(!bVisible) {
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
		SetSinglePaneMode(SPLIT_PANE_TOP);
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
		SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	UISetCheck(ID_VIEW_TRANSFER_VIEW, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, bVisible);
	return 0;
}
#endif

void MainWindow::handleWhatsThis() {
	sendMessage(WM_SYSCOMMAND, SC_CONTEXTHELP);
}

void MainWindow::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw() {
	versionInfo += string((const char*)buf, len);
}

void MainWindow::on(PartialList, const UserPtr& aUser, const string& text) throw() {
	dwt::Application::instance().callAsync(
		std::tr1::bind((void (*)(dwt::TabView*, const UserPtr&, const string&, int64_t))(&DirectoryListingFrame::openWindow), getTabView(),
		aUser, text, 0));
}

void MainWindow::on(QueueManagerListener::Finished, QueueItem* qi, const string& dir, int64_t speed) throw() {
	if (qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if (qi->isSet(QueueItem::FLAG_USER_LIST)) {
			dwt::Application::instance().callAsync(std::tr1::bind((void(*)(dwt::TabView*, const tstring&, const tstring&, const UserPtr&, int64_t))(&DirectoryListingFrame::openWindow), getTabView(),
				Text::toT(qi->getListName()), Text::toT(dir), qi->getDownloads()[0]->getUser(), speed));
		} else if (qi->isSet(QueueItem::FLAG_TEXT)) {
			dwt::Application::instance().callAsync(std::tr1::bind(&MainWindow::viewAndDelete, this, qi->getTarget()));
		}
	}
}
