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

#include "MainWindow.h"
#include "resource.h"

#include "ParamDlg.h"
#include "HashProgressDlg.h"
#include "SettingsDialog.h"
#include "TextFrame.h"
#include "SingleInstance.h"
#include "AboutDlg.h"
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
#include <dcpp/ConnectivityManager.h>
#include <dcpp/Download.h>
#include <dcpp/WindowInfo.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/ThrottleManager.h>

#include <dwt/Application.h>
#include <dwt/widgets/Notification.h>
#include <dwt/widgets/Rebar.h>
#include <dwt/widgets/Spinner.h>
#include <dwt/widgets/Splitter.h>
#include <dwt/widgets/ToolBar.h>

#ifdef HAVE_HTMLHELP_H
#include <htmlhelp.h>
#endif

static dwt::IconPtr mainIcon(WinUtil::createIcon(IDI_DCPP, 32));
static dwt::IconPtr mainSmallIcon(WinUtil::createIcon(IDI_DCPP, 16));

MainWindow::MainWindow() :
dwt::Window(0, dwt::NormalDispatcher::newClass<MainWindow>(mainIcon, mainSmallIcon)),
rebar(0),
paned(0),
transfers(0),
toolbar(0),
tabs(0),
slotsSpin(0),
maximized(false),
tray_pm(false),
c(0),
stopperThread(NULL),
lastUp(0),
lastDown(0),
lastTick(GET_TICK()),
prevAway(false),
fullSlots(false)
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
	links.blog = _T("http://dcpp.wordpress.com");
	links.community = _T("http://www.adcportal.com");

	initWindow();
	initTabs();
	initMenu();
	initToolbar();
	initStatusBar();
	initTransfers();
	initTray();

	addAccel(FCONTROL, '1', std::bind(&MainWindow::switchToolbar, this));
	addAccel(FCONTROL, '2', std::bind(&MainWindow::switchTransfers, this));
	addAccel(FCONTROL, '3', std::bind(&MainWindow::switchStatus, this));
	addAccel(FCONTROL, 'D', [this] { QueueFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'E', std::bind(&MainWindow::handleRefreshFileList, this));
	addAccel(FCONTROL, 'F', [this] { FavHubsFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'G', std::bind(&MainWindow::handleConnectFavHubGroup, this));
	addAccel(FCONTROL, 'L', std::bind(&MainWindow::handleOpenFileList, this));
	addAccel(FCONTROL, 'N', [this] { NotepadFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'P', [this] { PublicHubsFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'Q', std::bind(&MainWindow::handleQuickConnect, this));
	addAccel(FCONTROL, 'S', [this] { SearchFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'U', [this] { UsersFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, VK_F3, std::bind(&MainWindow::handleSettings, this));
	addAccel(0, VK_F5, std::bind(&MainWindow::handleRefreshFileList, this));
	initAccels();

	onActivate(std::bind(&MainWindow::handleActivate, this, _1));
	onSized(std::bind(&MainWindow::handleSized, this, _1));
	onHelp(std::bind(&WinUtil::help, _1, _2));

	updateStatus();
	setTimer([this]() -> bool { updateStatus(); return true; }, 1000, TIMER_STATUS);

	layout();

	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);
	WindowManager::getInstance()->addListener(this);

	onClosing(std::bind(&MainWindow::handleClosing, this));

	onRaw(std::bind(&MainWindow::handleActivateApp, this, _1), dwt::Message(WM_ACTIVATEAPP));
	onRaw(std::bind(&MainWindow::handleEndSession, this), dwt::Message(WM_ENDSESSION));
	onRaw(std::bind(&MainWindow::handleCopyData, this, _2), dwt::Message(WM_COPYDATA));
	onRaw(std::bind(&MainWindow::handleWhereAreYou, this), dwt::Message(SingleInstance::WMU_WHERE_ARE_YOU));

	filterIter = dwt::Application::instance().addFilter(std::bind(&MainWindow::filter, this, _1));

	TimerManager::getInstance()->start();

	c = new HttpConnection;
	c->addListener(this);
	c->downloadFile("http://dcplusplus.sourceforge.net/version.xml");

	File::ensureDirectory(SETTING(LOG_DIRECTORY));

	try {
		ConnectivityManager::getInstance()->setup(true, SettingsManager::INCOMING_DIRECT);
	} catch (const Exception& e) {
		showPortsError(e.getError());
	}

	WindowManager::getInstance()->autoOpen(WinUtil::isShift());

	callAsync(std::bind(&MainWindow::parseCommandLine, this, tstring(::GetCommandLine())));

	int cmdShow = dwt::Application::instance().getCmdShow();
	::ShowWindow(handle(), (cmdShow == SW_SHOWDEFAULT || cmdShow == SW_SHOWNORMAL) ? SETTING(MAIN_WINDOW_STATE) : cmdShow);
	if(cmdShow == SW_MINIMIZE || cmdShow == SW_SHOWMINIMIZED || cmdShow == SW_SHOWMINNOACTIVE)
		handleMinimized();

	if(SETTING(NICK).empty()) {
		callAsync([this]() {
			SystemFrame::openWindow(getTabView(), false, false);

			WinUtil::help(this, IDH_GET_STARTED);
			handleSettings();
		});
	}

	if(SETTING(SETTINGS_SAVE_INTERVAL) > 0)
		setSaveTimer();
}

void MainWindow::initWindow() {
	// Create main window
	dcdebug("initWindow\n");

	Seed cs(_T(APPNAME) _T(" ") _T(VERSIONSTRING));

	int pos_x = SETTING(MAIN_WINDOW_POS_X);
	int pos_y = SETTING(MAIN_WINDOW_POS_Y);
	int size_x = SETTING(MAIN_WINDOW_SIZE_X);
	int size_y = SETTING(MAIN_WINDOW_SIZE_Y);
	if(pos_x != static_cast<int>(CW_USEDEFAULT) && pos_y != static_cast<int>(CW_USEDEFAULT) &&
		size_x != static_cast<int>(CW_USEDEFAULT) && size_y != static_cast<int>(CW_USEDEFAULT) &&
		pos_x > 0 && pos_y > 0 && size_x > 10 && size_y > 10 &&
		pos_x < getDesktopSize().x && pos_y < getDesktopSize().y)
	{
		cs.location = dwt::Rectangle(pos_x, pos_y, size_x, size_y);
	}

	cs.exStyle |= WS_EX_APPWINDOW;
	if (ResourceManager::getInstance()->isRTL())
		cs.exStyle |= WS_EX_RTLREADING;

	create(cs);

	setHelpId(IDH_MAIN);

	rebar = addChild(Rebar::Seed());

	paned = addChild(HSplitter::Seed(SETTING(TRANSFERS_PANED_POS)));
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

		file->appendItem(T_("&Quick connect...\tCtrl+Q"), [this] { handleQuickConnect(); }, WinUtil::menuIcon(IDI_HUB));
		file->appendItem(T_("Connect to a favorite hub &group...\tCtrl+G"), [this] { handleConnectFavHubGroup(); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		file->appendSeparator();

		file->appendItem(T_("&Reconnect\tCtrl+R"), [this] { handleReconnect(); }, WinUtil::menuIcon(IDI_RECONNECT));
		file->appendItem(T_("Follow last redirec&t\tCtrl+T"), [this] { handleRedirect(); }, WinUtil::menuIcon(IDI_FOLLOW));
		file->appendSeparator();

		file->appendItem(T_("Open file list...\tCtrl+L"), [this] { handleOpenFileList(); }, WinUtil::menuIcon(IDI_OPEN_FILE_LIST));
		file->appendItem(T_("Open own list"), [this] { DirectoryListingFrame::openOwnList(getTabView()); });
		file->appendItem(T_("Match downloaded lists"), [this] { handleMatchAll(); });
		file->appendItem(T_("Refresh file list\tF5"), [this] { handleRefreshFileList(); }, WinUtil::menuIcon(IDI_REFRESH));
		file->appendItem(T_("Open downloads directory"), [this] { handleOpenDownloadsDir(); }, WinUtil::menuIcon(IDI_OPEN_DL_DIR));
		file->appendSeparator();

		file->appendItem(T_("Settings\tCtrl+F3"), [this] { handleSettings(); }, WinUtil::menuIcon(IDI_SETTINGS));
		file->appendSeparator();
		file->appendItem(T_("E&xit\tAlt+F4"), [this] { GCC_WTF->close(true); }, WinUtil::menuIcon(IDI_EXIT));
	}

	{
		viewMenu = mainMenu->appendPopup(T_("&View"));

		viewIndexes[PublicHubsFrame::id] = viewMenu->appendItem(T_("&Public Hubs\tCtrl+P"),
			[this] { PublicHubsFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_PUBLICHUBS));
		viewIndexes[FavHubsFrame::id] = viewMenu->appendItem(T_("&Favorite Hubs\tCtrl+F"),
			[this] { FavHubsFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		viewIndexes[UsersFrame::id] = viewMenu->appendItem(T_("&Users\tCtrl+U"),
			[this] { UsersFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_FAVORITE_USERS));
		viewMenu->appendSeparator();
		viewIndexes[QueueFrame::id] = viewMenu->appendItem(T_("&Download Queue\tCtrl+D"),
			[this] { QueueFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_QUEUE));
		viewIndexes[FinishedDLFrame::id] = viewMenu->appendItem(T_("Finished Downloads"),
			[this] { FinishedDLFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_FINISHED_DL));
		viewIndexes[WaitingUsersFrame::id] = viewMenu->appendItem(T_("Waiting Users"),
			[this] { WaitingUsersFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_WAITING_USERS));
		viewIndexes[FinishedULFrame::id] = viewMenu->appendItem(T_("Finished Uploads"),
			[this] { FinishedULFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_FINISHED_UL));
		viewMenu->appendSeparator();
		viewMenu->appendItem(T_("&Search\tCtrl+S"), [this] { SearchFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_SEARCH));
		viewIndexes[ADLSearchFrame::id] = viewMenu->appendItem(T_("ADL Search"),
			[this] { ADLSearchFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_ADLSEARCH));
		viewIndexes[SpyFrame::id] = viewMenu->appendItem(T_("Search Spy"),
			[this] { SpyFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_SPY));
		viewMenu->appendSeparator();
		viewIndexes[NotepadFrame::id] = viewMenu->appendItem(T_("&Notepad\tCtrl+N"),
			[this] { NotepadFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_NOTEPAD));
		viewIndexes[SystemFrame::id] = viewMenu->appendItem(T_("System Log"),
			[this] { SystemFrame::openWindow(getTabView()); });
		viewIndexes[StatsFrame::id] = viewMenu->appendItem(T_("Network Statistics"),
			[this] { StatsFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_NET_STATS));
		viewMenu->appendItem(T_("Indexing progress"), std::bind(&MainWindow::handleHashProgress, this), WinUtil::menuIcon(IDI_INDEXING));
		viewMenu->appendSeparator();
		viewIndexes["Toolbar"] = viewMenu->appendItem(T_("Toolbar\tCtrl+1"),
			std::bind(&MainWindow::switchToolbar, this));
		viewIndexes["Transfers"] = viewMenu->appendItem(T_("Transfer view\tCtrl+2"),
			std::bind(&MainWindow::switchTransfers, this));
		viewIndexes["Status"] = viewMenu->appendItem(T_("Status bar\tCtrl+3"),
			std::bind(&MainWindow::switchStatus, this));
	}

	{
		MenuPtr window = mainMenu->appendPopup(T_("&Window"));

		window->appendItem(T_("Close all hubs"), std::bind(&HubFrame::closeAll, true), WinUtil::menuIcon(IDI_HUB));
		window->appendItem(T_("Close disconnected hubs"), std::bind(&HubFrame::closeAll, false), WinUtil::menuIcon(IDI_HUB_OFF));
		window->appendItem(T_("Close all hubs of a favorite group"), std::bind(&MainWindow::handleCloseFavGroup, this, false), WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		window->appendItem(T_("Close hubs not in a favorite group"), std::bind(&MainWindow::handleCloseFavGroup, this, true), WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		window->appendSeparator();

		window->appendItem(T_("Close all PM windows"), std::bind(&PrivateFrame::closeAll), WinUtil::menuIcon(IDI_PRIVATE));
		window->appendItem(T_("Close all offline PM windows"), std::bind(&PrivateFrame::closeAllOffline), WinUtil::menuIcon(IDI_PRIVATE_OFF));
		window->appendSeparator();

		window->appendItem(T_("Close all file list windows"), std::bind(&DirectoryListingFrame::closeAll), WinUtil::menuIcon(IDI_DIRECTORY));
		window->appendSeparator();

		window->appendItem(T_("Close all search windows"), std::bind(&SearchFrame::closeAll), WinUtil::menuIcon(IDI_SEARCH));
	}

	{
		MenuPtr help = mainMenu->appendPopup(T_("&Help"));

		help->appendItem(T_("Help &Contents\tF1"), std::bind(&WinUtil::help, this, IDH_INDEX), WinUtil::menuIcon(IDI_HELP));
		help->appendItem(T_("Get started"), std::bind(&WinUtil::help, this, IDH_GET_STARTED), WinUtil::menuIcon(IDI_GET_STARTED));
		help->appendSeparator();
		help->appendItem(T_("Change Log"), std::bind(&WinUtil::help, this, IDH_CHANGELOG), WinUtil::menuIcon(IDI_CHANGELOG));
		help->appendItem(T_("About DC++"), std::bind(&MainWindow::handleAbout, this), WinUtil::menuIcon(IDI_DCPP));
		help->appendSeparator();

		help = help->appendPopup(T_("Links"), WinUtil::menuIcon(IDI_LINKS));
		help->appendItem(T_("Homepage"), std::bind(&WinUtil::openLink, std::cref(links.homepage)));
		help->appendItem(T_("Downloads"), std::bind(&WinUtil::openLink, std::cref(links.downloads)));
		help->appendItem(T_("Frequently asked questions"), std::bind(&WinUtil::openLink, std::cref(links.faq)));
		help->appendItem(T_("Help center"), std::bind(&WinUtil::openLink, std::cref(links.help)));
		help->appendItem(T_("Discussion forum"), std::bind(&WinUtil::openLink, std::cref(links.discuss)));
		help->appendItem(T_("Request a feature"), std::bind(&WinUtil::openLink, std::cref(links.features)));
		help->appendItem(T_("Report a bug"), std::bind(&WinUtil::openLink, std::cref(links.bugs)));
		help->appendItem(T_("Donate (paypal)"), std::bind(&WinUtil::openLink, std::cref(links.donate)), WinUtil::menuIcon(IDI_DONATE));
		help->appendItem(T_("Blog"), std::bind(&WinUtil::openLink, std::cref(links.blog)));
		help->appendItem(T_("Community news"), std::bind(&WinUtil::openLink, std::cref(links.community)));
		help->appendSeparator();

		help->appendItem(T_("GeoIP database update"), std::bind(&WinUtil::openLink, std::cref(links.geoipfile)));
	}

	mainMenu->setMenu();
}

void MainWindow::initToolbar() {
	if(!BOOLSETTING(SHOW_TOOLBAR))
		return;

	dcdebug("initToolbar\n");
	toolbar = addChild(ToolBar::Seed());

	toolbar->addButton(PublicHubsFrame::id, WinUtil::toolbarIcon(IDI_PUBLICHUBS), 0, T_("Public Hubs"), IDH_TOOLBAR_PUBLIC_HUBS,
		[this] { PublicHubsFrame::openWindow(getTabView()); });
	toolbar->addButton("Reconnect", WinUtil::toolbarIcon(IDI_RECONNECT), 0, T_("Reconnect"), IDH_TOOLBAR_RECONNECT,
		std::bind(&MainWindow::handleReconnect, this));
	toolbar->addButton("Redirect", WinUtil::toolbarIcon(IDI_FOLLOW), 0, T_("Follow last redirect"), IDH_TOOLBAR_FOLLOW,
		std::bind(&MainWindow::handleRedirect, this));
	toolbar->addButton(FavHubsFrame::id, WinUtil::toolbarIcon(IDI_FAVORITE_HUBS), 0, T_("Favorite Hubs"), IDH_TOOLBAR_FAVORITE_HUBS,
		[this] { FavHubsFrame::openWindow(getTabView()); }, std::bind(&MainWindow::handleFavHubsDropDown, this, _1));
	toolbar->addButton(UsersFrame::id, WinUtil::toolbarIcon(IDI_FAVORITE_USERS), 0, T_("Users"), IDH_TOOLBAR_FAVORITE_USERS,
		[this] { UsersFrame::openWindow(getTabView()); });
	toolbar->addButton(QueueFrame::id, WinUtil::toolbarIcon(IDI_QUEUE), 0, T_("Download Queue"), IDH_TOOLBAR_QUEUE,
		[this] { QueueFrame::openWindow(getTabView()); });
	toolbar->addButton(FinishedDLFrame::id, WinUtil::toolbarIcon(IDI_FINISHED_DL), 0, T_("Finished Downloads"), IDH_TOOLBAR_FINISHED_DL,
		[this] { FinishedDLFrame::openWindow(getTabView()); });
	toolbar->addButton(WaitingUsersFrame::id, WinUtil::toolbarIcon(IDI_WAITING_USERS), 0, T_("Waiting Users"), IDH_TOOLBAR_WAITING_USERS,
		[this] { WaitingUsersFrame::openWindow(getTabView()); });
	toolbar->addButton(FinishedULFrame::id, WinUtil::toolbarIcon(IDI_FINISHED_UL), 0, T_("Finished Uploads"), IDH_TOOLBAR_FINISHED_UL,
		[this] { FinishedULFrame::openWindow(getTabView()); });
	toolbar->addButton(SearchFrame::id, WinUtil::toolbarIcon(IDI_SEARCH), 0, T_("Search"), IDH_TOOLBAR_SEARCH,
		[this] { SearchFrame::openWindow(getTabView()); });
	toolbar->addButton(ADLSearchFrame::id, WinUtil::toolbarIcon(IDI_ADLSEARCH), 0, T_("ADL Search"), IDH_TOOLBAR_ADL_SEARCH,
		[this] { ADLSearchFrame::openWindow(getTabView()); });
	toolbar->addButton(SpyFrame::id, WinUtil::toolbarIcon(IDI_SPY), 0, T_("Search Spy"), IDH_TOOLBAR_SEARCH_SPY,
		[this] { SpyFrame::openWindow(getTabView()); });
	toolbar->addButton(StatsFrame::id, WinUtil::toolbarIcon(IDI_NET_STATS), 0, T_("Network Statistics"), IDH_TOOLBAR_NET_STATS,
		[this] { StatsFrame::openWindow(getTabView()); });
	toolbar->addButton("OpenFL", WinUtil::toolbarIcon(IDI_OPEN_FILE_LIST), 0, T_("Open file list..."), IDH_TOOLBAR_FILE_LIST,
		std::bind(&MainWindow::handleOpenFileList, this));
	toolbar->addButton("Recents", WinUtil::toolbarIcon(IDI_RECENTS), 0, T_("Recent windows"), IDH_TOOLBAR_RECENT,
		0, std::bind(&MainWindow::handleRecent, this, _1));
	toolbar->addButton("Settings", WinUtil::toolbarIcon(IDI_SETTINGS), 0, T_("Settings"), IDH_TOOLBAR_SETTINGS,
		std::bind(&MainWindow::handleSettings, this));
	toolbar->addButton(NotepadFrame::id, WinUtil::toolbarIcon(IDI_NOTEPAD), 0, T_("Notepad"), IDH_TOOLBAR_NOTEPAD,
		[this] { NotepadFrame::openWindow(getTabView()); });
	toolbar->addButton("Refresh", WinUtil::toolbarIcon(IDI_REFRESH), 0, T_("Refresh file list"), IDH_TOOLBAR_REFRESH,
		std::bind(&MainWindow::handleRefreshFileList, this));
	toolbar->addButton("CSHelp", WinUtil::toolbarIcon(IDI_WHATS_THIS), 0, T_("What's This?"), IDH_TOOLBAR_WHATS_THIS,
		std::bind(&MainWindow::handleWhatsThis, this));

	if(SettingsManager::getInstance()->isDefault(SettingsManager::TOOLBAR)) {
		// gotta create a default layout for the toolbar
		const string comma(",");
		SettingsManager::getInstance()->setDefault(SettingsManager::TOOLBAR,
			PublicHubsFrame::id + comma +
			comma +
			"Reconnect" + comma +
			"Redirect" + comma +
			FavHubsFrame::id + comma +
			UsersFrame::id + comma +
			comma +
			QueueFrame::id + comma +
			FinishedDLFrame::id + comma +
			WaitingUsersFrame::id + comma +
			FinishedULFrame::id + comma +
			comma +
			SearchFrame::id + comma +
			ADLSearchFrame::id + comma +
			SpyFrame::id + comma +
			comma +
			StatsFrame::id + comma +
			comma +
			"OpenFL" + comma +
			"Recents" + comma +
			comma +
			"Settings" + comma +
			NotepadFrame::id + comma +
			"Refresh" + comma +
			comma +
			"CSHelp");
	}
	toolbar->setLayout(StringTokenizer<string>(SETTING(TOOLBAR), ',').getTokens());
	toolbar->onCustomized(std::bind(&MainWindow::handleToolbarCustomized, this));
	toolbar->onCustomizeHelp(std::bind(&WinUtil::help, toolbar, IDH_CUSTOMIZE_TB));
	toolbar->onContextMenu(std::bind(&MainWindow::handleToolbarContextMenu, this, _1));

	toolbar->onHelp(std::bind(&WinUtil::help, _1, _2));

	rebar->add(toolbar);

	viewMenu->checkItem(viewIndexes["Toolbar"], true);
}

void MainWindow::initStatusBar() {
	if(!BOOLSETTING(SHOW_STATUSBAR))
		return;

	dcdebug("initStatusBar\n");

	slotsSpin = addChild(Spinner::Seed(1));
	slotsSpin->setHelpId(IDH_MAIN_SLOTS_SPIN);
	slotsSpin->onUpdate(std::bind(&MainWindow::handleSlotsUpdate, this, _1, _2));

	initStatus(true);
	status->setSize(STATUS_SLOTS_SPIN, 22);
	///@todo set to checkbox width + resizedrag width really
	status->setSize(STATUS_DUMMY, 32);

	updateAwayStatus();

	status->setIcon(STATUS_COUNTS, WinUtil::statusIcon(IDI_HUB));
	status->setIcon(STATUS_SLOTS, WinUtil::statusIcon(IDI_SLOTS));
	{
		dwt::IconPtr icon_DL(WinUtil::statusIcon(IDI_DOWNLOAD));
		dwt::IconPtr icon_UL(WinUtil::statusIcon(IDI_UPLOAD));
		status->setIcon(STATUS_DOWN_TOTAL, icon_DL);
		status->setIcon(STATUS_UP_TOTAL, icon_UL);
		status->setIcon(STATUS_DOWN_DIFF, icon_DL);
		status->setIcon(STATUS_UP_DIFF, icon_UL);
	}

	status->onDblClicked(STATUS_STATUS, std::bind(&WinUtil::openFile, Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM)))));
	status->onDblClicked(STATUS_AWAY, std::bind(&Util::switchAway));
	{
		auto f = [this] { StatsFrame::openWindow(getTabView(), false); };
		status->onDblClicked(STATUS_DOWN_TOTAL, f);
		status->onDblClicked(STATUS_UP_TOTAL, f);
		status->onDblClicked(STATUS_DOWN_DIFF, f);
		status->onDblClicked(STATUS_UP_DIFF, f);
	}

	status->setHelpId(STATUS_STATUS, IDH_MAIN_STATUS);
	status->setHelpId(STATUS_AWAY, IDH_MAIN_AWAY);
	status->setHelpId(STATUS_COUNTS, IDH_MAIN_HUBS);
	status->setHelpId(STATUS_SLOTS, IDH_MAIN_SLOTS);
	status->setHelpId(STATUS_DOWN_TOTAL, IDH_MAIN_DOWN_TOTAL);
	status->setHelpId(STATUS_UP_TOTAL, IDH_MAIN_UP_TOTAL);
	status->setHelpId(STATUS_DOWN_DIFF, IDH_MAIN_DOWN_DIFF);
	status->setHelpId(STATUS_UP_DIFF, IDH_MAIN_UP_DIFF);
	status->setHelpId(STATUS_DOWN_LIMIT, IDH_MAIN_DOWN_LIMIT);
	status->setHelpId(STATUS_UP_LIMIT, IDH_MAIN_UP_LIMIT);

	viewMenu->checkItem(viewIndexes["Status"], true);
}

void MainWindow::initTabs() {
	dcdebug("initTabs\n");
	TabView::Seed seed = WinUtil::Seeds::tabs;
	seed.widthConfig = SETTING(TAB_WIDTH);
	if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_OD) {
		if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BROWSER)
			seed.tabStyle = TabView::Seed::WinBrowser;
	} else {
		seed.style &= ~TCS_OWNERDRAWFIXED;
		seed.widthConfig = (seed.widthConfig - 100) / 9; // max width to max chars
	}
	if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BUTTONS)
		seed.style |= TCS_BUTTONS;
	seed.closeIcon = WinUtil::tabIcon(IDI_EXIT);
	seed.toggleActive = BOOLSETTING(TOGGLE_ACTIVE_WINDOW);
	seed.ctrlTab = true;
	tabs = addChild(seed);
	tabs->initTaskbar(this);
	tabs->onTitleChanged(std::bind(&MainWindow::handleTabsTitleChanged, this, _1));
	tabs->onHelp(std::bind(&WinUtil::help, _1, _2));
	paned->setFirst(tabs);
}

void MainWindow::initTransfers() {
	if(!BOOLSETTING(SHOW_TRANSFERVIEW))
		return;

	dcdebug("initTransfers\n");
	transfers = new TransferView(this, getTabView());
	paned->setSecond(transfers);

	viewMenu->checkItem(viewIndexes["Transfers"], true);
}

void MainWindow::initTray() {
	dcdebug("initTray\n");
	notify = dwt::NotificationPtr(new dwt::Notification(this));
	notify->create(dwt::Notification::Seed(mainSmallIcon));
	notify->onContextMenu(std::bind(&MainWindow::handleTrayContextMenu, this));
	notify->onIconClicked(std::bind(&MainWindow::handleTrayClicked, this));
	notify->onUpdateTip(std::bind(&MainWindow::handleTrayUpdate, this));
	if(SETTING(ALWAYS_TRAY)) {
		notify->setVisible(true);
	}
}

bool MainWindow::filter(MSG& msg) {
	if(tabs && tabs->filter(msg)) {
		return true;
	}

#ifdef HAVE_HTMLHELP_H
	if(::HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, reinterpret_cast<DWORD_PTR>(&msg))) {
		return true;
	}
#endif

	Container* active = getTabView()->getActive();
	if(active) {
		if(::IsDialogMessage( active->handle(), & msg )) {
			return true;
		}
	}

	return false;
}

void MainWindow::setStaticWindowState(const string& id, bool open) {
	if(toolbar)
		toolbar->setButtonChecked(id, open);

	ViewIndexes::const_iterator i = viewIndexes.find(id);
	if(i != viewIndexes.end())
		viewMenu->checkItem(i->second, open);
}

void MainWindow::TrayPM() {
	if(!tray_pm && notify->isVisible() && ::GetAncestor(::GetForegroundWindow(), GA_ROOTOWNER) != handle()) {
		static dwt::IconPtr icon(WinUtil::createIcon(IDI_TRAY_PM, 16));
		notify->setIcon(icon);
		tray_pm = true;
	}
}

void MainWindow::handleTabsTitleChanged(const tstring& title) {
	setText(title.empty() ? _T(APPNAME) _T(" ") _T(VERSIONSTRING) : _T(APPNAME) _T(" ") _T(VERSIONSTRING) _T(" - [") + title + _T("]"));
}

static void multiConnect(const string& group, TabViewPtr parent) {
	FavoriteHubEntryList hubs = FavoriteManager::getInstance()->getFavoriteHubs(group);
	for(FavoriteHubEntryList::const_iterator i = hubs.begin(), iend = hubs.end(); i != iend; ++i)
		HubFrame::openWindow(parent, (*i)->getServer());
}

void MainWindow::handleFavHubsDropDown(const dwt::ScreenCoordinate& pt) {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	typedef map<string, MenuPtr, noCaseStringLess> GroupMenus;
	GroupMenus groupMenus;

	const FavHubGroups& groups = FavoriteManager::getInstance()->getFavHubGroups();
	for(FavHubGroups::const_iterator i = groups.begin(), iend = groups.end(); i != iend; ++i)
		groupMenus.insert(make_pair(i->first, MenuPtr()));

	for(GroupMenus::iterator i = groupMenus.begin(); i != groupMenus.end(); ++i) {
		i->second = menu->appendPopup(escapeMenu(Text::toT(i->first)));
		i->second->appendItem(T_("Connect to all hubs in this group"), std::bind(&multiConnect, i->first, getTabView()));
		i->second->appendSeparator();
	}

	const FavoriteHubEntryList& hubs = FavoriteManager::getInstance()->getFavoriteHubs();
	for(FavoriteHubEntryList::const_iterator i = hubs.begin(), iend = hubs.end(); i != iend; ++i) {
		FavoriteHubEntry* entry = *i;
		GroupMenus::iterator groupMenu = groupMenus.find(entry->getGroup());
		((groupMenu == groupMenus.end()) ? menu : groupMenu->second)->appendItem(
			escapeMenu(Text::toT(entry->getName())),
			[this, entry] { HubFrame::openWindow(getTabView(), entry->getServer()); });
	}

	menu->open(pt);
}

template<typename T, typename configureF>
static void addRecentMenu(const WindowManager::RecentList& recent, MenuPtr& menu, MainWindow* mainWindow,
						  const tstring& text, unsigned iconId, unsigned favIconId, configureF f)
{
	MenuPtr popup = menu->appendPopup(text, WinUtil::menuIcon(iconId));
	popup->appendItem(T_("&Configure..."), std::bind(f, mainWindow, T::id, text),
		WinUtil::menuIcon(IDI_SETTINGS), true, true);
	popup->appendSeparator();

	WindowManager::RecentList::const_iterator it = recent.find(T::id);
	if(it == recent.end()) {
		popup->appendItem(T_("(No recent item found)"), 0, 0, false);
	} else {

		dwt::IconPtr favIcon = WinUtil::menuIcon(favIconId);

		const WindowManager::WindowInfoList& list = it->second;
		for(WindowManager::WindowInfoList::const_iterator i = list.begin(), iend = list.end(); i != iend; ++i) {
			StringMap params = i->getParams();

			StringMap::const_iterator title = params.find(WindowInfo::title);
			if(title == params.end() || title->second.empty())
				continue;

			popup->appendItem(escapeMenu(Text::toT(title->second)),
				std::bind(&T::parseWindowParams, mainWindow->getTabView(), params),
				T::isFavorite(params) ? favIcon : 0);
		}
	}
}

void MainWindow::handleRecent(const dwt::ScreenCoordinate& pt) {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	{
		WindowManager* wm = WindowManager::getInstance();
		auto lock = wm->lock();
		const WindowManager::RecentList& recent = wm->getRecent();

		typedef void (MainWindow::*configureF)(const string&, const tstring&);
		configureF f = &MainWindow::handleConfigureRecent;
		addRecentMenu<HubFrame>(recent, menu, this, T_("Recent hubs"), IDI_HUB, IDI_FAVORITE_HUBS, f);
		addRecentMenu<PrivateFrame>(recent, menu, this, T_("Recent PMs"), IDI_PRIVATE, IDI_FAVORITE_USERS, f);
		addRecentMenu<DirectoryListingFrame>(recent, menu, this, T_("Recent file lists"), IDI_DIRECTORY, IDI_FAVORITE_USERS, f);
	}

	menu->open(pt);
}

void MainWindow::handleConfigureRecent(const string& id, const tstring& title) {
	ParamDlg dlg(this, title);
	dlg.addIntTextBox(T_("Maximum number of recent items to save"),
		Text::toT(Util::toString(WindowManager::getInstance()->getMaxRecentItems(id))), 0);
	if(dlg.run() == IDOK) {
		const unsigned max = Util::toUInt(Text::fromT(dlg.getValue()));
		dwt::MessageBox(this).show(str(TF_("%1% recent items will be saved from now on.") % max), title,
			dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONINFORMATION);
		WindowManager::getInstance()->setMaxRecentItems(id, max);
	}
}

void MainWindow::handleReconnect() {
	forwardHub(&HubFrame::handleReconnect);
}

void MainWindow::handleRedirect() {
	forwardHub(&HubFrame::handleFollow);
}

void MainWindow::forwardHub(void (HubFrame::*f)()) {
	HubFrame* active = dynamic_cast<HubFrame*>(getTabView()->getActive());
	if(active) {
		(active->*f)();
	}
}

void MainWindow::handleQuickConnect() {
	if(!WinUtil::checkNick())
		return;

	ParamDlg dlg(this, T_("Quick Connect"), T_("Address"));

	if (dlg.run() == IDOK) {

		tstring tmp = dlg.getValue();
		// Strip out all the spaces
		string::size_type i;
		while ((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		HubFrame::openWindow(getTabView(), Text::fromT(tmp));
	}
}

void MainWindow::handleConnectFavHubGroup() {
	if(!WinUtil::checkNick())
		return;

	tstring group;
	if(chooseFavHubGroup(T_("Connect to a favorite hub group"), group)) {
		FavoriteHubEntryList hubs = FavoriteManager::getInstance()->getFavoriteHubs(Text::fromT(group));
		for(FavoriteHubEntryList::const_iterator hub = hubs.begin(), hub_end = hubs.end(); hub != hub_end; ++hub) {
			HubFrame::openWindow(getTabView(), (*hub)->getServer());
		}
	}
}

void MainWindow::handleSized(const dwt::SizedEvent& sz) {
	if(sz.isMinimized) {
		handleMinimized();
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

void MainWindow::handleMinimized() {
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
}

LRESULT MainWindow::handleActivateApp(WPARAM wParam) {
	if(wParam == TRUE && tray_pm) {
		notify->setIcon(mainSmallIcon);
		tray_pm = false;
	}
	return 0;
}

void MainWindow::statusMessage(time_t t, const string& m) {
	if(!status)
		return;

	string message(m);
	WinUtil::reducePaths(message);
	status->setText(STATUS_STATUS, Text::toT("[" + Util::getShortTimeString(t) + "] " + message));
}

void MainWindow::on(LogManagerListener::Message, time_t t, const string& m) throw() {
	callAsync(std::bind(&MainWindow::statusMessage, this, t, m));
}

bool MainWindow::chooseFavHubGroup(const tstring& title, tstring& group) {
	set<tstring, noCaseStringLess> groups;

	const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
	if(favHubGroups.empty()) {
		dwt::MessageBox(this).show(T_("No favorite hub group found"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return false;
	}

	for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i)
		groups.insert(Text::toT(i->first));

	ParamDlg dlg(this, title, T_("Select a favorite hub group"), TStringList(groups.begin(), groups.end()));
	if(dlg.run() == IDOK) {
		group = dlg.getValue();
		return true;
	}
	return false;
}

void MainWindow::setSaveTimer() {
	setTimer([this]() -> bool { saveSettings(); return true; }, SETTING(SETTINGS_SAVE_INTERVAL) * 60 * 1000, TIMER_SAVE);
}

void MainWindow::saveSettings() {
	saveWindowSettings();
	SettingsManager::getInstance()->save();
}

void MainWindow::saveWindowSettings() {
	{
		const auto& views = tabs->getChildren();
		auto active = tabs->getActive();

		WindowManager* wm = WindowManager::getInstance();
		auto lock = wm->lock();
		wm->clear();

		for(auto i = views.cbegin(), iend = views.cend(); i != iend; ++i) {
			auto child = static_cast<MDIChildFrame<dwt::Container>*>(*i);
			auto params = child->getWindowParams();
			if(child == active)
				params["Active"] = "1";
			wm->add(child->getId(), params);
		}
	}

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

bool MainWindow::handleClosing() {
	if(!closing()) {
		if ( !BOOLSETTING(CONFIRM_EXIT) || (dwt::MessageBox(this).show(T_("Really exit?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES)) {
			if (c != NULL) {
				c->removeListener(this);
				delete c;
				c = NULL;
			}
			saveWindowSettings();

			setVisible(false);
			if(transfers)
				transfers->prepareClose();

			WindowManager::getInstance()->removeListener(this);
			LogManager::getInstance()->removeListener(this);
			QueueManager::getInstance()->removeListener(this);

			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();

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

void MainWindow::layout() {
	dwt::Rectangle r(getClientSize());

	if(!rebar->empty()) {
		rebar->refresh();
		dwt::Point pt = rebar->getWindowSize();
		r.pos.y += pt.y;
		r.size.y -= pt.y;
	}

	if(status) {
		status->layout(r);
		layoutSlotsSpin();
	}

	paned->layout(r);
}

void MainWindow::layoutSlotsSpin() {
	status->mapWidget(STATUS_SLOTS_SPIN, slotsSpin, dwt::Rectangle(0, 1, 3, 2));
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

	if(!status)
		return;

	if(Util::getAway() != prevAway) {
		prevAway = !prevAway;
		updateAwayStatus();
	}

	tstring s = Text::toT(Client::getCounts());
	status->setText(STATUS_COUNTS, s);
	status->setToolTip(STATUS_COUNTS, str(TF_("Hubs: %1%") % s));

	auto freeSlots = UploadManager::getInstance()->getFreeSlots();
	auto totalSlots = SETTING(SLOTS);
	status->setText(STATUS_SLOTS, str(TF_("%1%/%2%") % freeSlots % totalSlots), true);
	status->setToolTip(STATUS_SLOTS, str(TF_("%1% free slots of %2% total upload slots") % freeSlots % totalSlots));
	if(!freeSlots ^ fullSlots) {
		fullSlots = !freeSlots;
		status->setIcon(STATUS_SLOTS, WinUtil::statusIcon(fullSlots ? IDI_SLOTS_FULL : IDI_SLOTS));
	}

	s = Text::toT(Util::formatBytes(down));
	status->setText(STATUS_DOWN_TOTAL, s);
	status->setToolTip(STATUS_DOWN_TOTAL, str(TF_("D: %1%") % s));

	s = Text::toT(Util::formatBytes(up));
	status->setText(STATUS_UP_TOTAL, s);
	status->setToolTip(STATUS_UP_TOTAL, str(TF_("U: %1%") % s));

	s = Text::toT(Util::formatBytes((downdiff*1000)/tdiff));
	status->setText(STATUS_DOWN_DIFF, str(TF_("%1%/s") % s));
	status->setToolTip(STATUS_DOWN_DIFF, str(TF_("D: %1%/s (%2%)") % s % DownloadManager::getInstance()->getDownloadCount()));

	s = Text::toT(Util::formatBytes((updiff*1000)/tdiff));
	status->setText(STATUS_UP_DIFF, str(TF_("%1%/s") % s));
	status->setToolTip(STATUS_UP_DIFF, str(TF_("U: %1%/s (%2%)") % s % UploadManager::getInstance()->getUploadCount()));

	status->setText(STATUS_DOWN_LIMIT, str(TF_("D Lim: %1%/s") % Text::toT(Util::formatBytes(ThrottleManager::getInstance()->getDownLimit()*1024))));
	status->setText(STATUS_UP_LIMIT, str(TF_("U Lim: %1%/s") % Text::toT(Util::formatBytes(ThrottleManager::getInstance()->getUpLimit()*1024))));

	layoutSlotsSpin();
}

void MainWindow::updateAwayStatus() {
	status->setIcon(STATUS_AWAY, WinUtil::statusIcon(prevAway ? IDI_USER_AWAY : IDI_USER));
	status->setToolTip(STATUS_AWAY, prevAway ? (T_("Status: Away - Double-click to switch to Available")) :
		(T_("Status: Available - Double-click to switch to Away")));
}

MainWindow::~MainWindow() {
	dwt::Application::instance().removeFilter(filterIter);
}

void MainWindow::handleSettings() {
	// this may be called directly from the tray icon when the main window is still hidden
	if(isIconic())
		handleRestore();

	SettingsDialog dlg(this);

	unsigned short lastTCP = static_cast<unsigned short>(SETTING(TCP_PORT));
	unsigned short lastUDP = static_cast<unsigned short>(SETTING(UDP_PORT));
	unsigned short lastTLS = static_cast<unsigned short>(SETTING(TLS_PORT));

	int lastConn = SETTING(INCOMING_CONNECTIONS);
	bool lastSortFavUsersFirst = BOOLSETTING(SORT_FAVUSERS_FIRST);
	bool lastURLReg = BOOLSETTING(URL_HANDLER);
	bool lastMagnetReg = BOOLSETTING(MAGNET_REGISTER);
	int lastSettingsSave = SETTING(SETTINGS_SAVE_INTERVAL);

	if(dlg.run() == IDOK) {
		SettingsManager::getInstance()->save();

		try {
			ConnectivityManager::getInstance()->setup(SETTING(INCOMING_CONNECTIONS) != lastConn || SETTING(TCP_PORT) != lastTCP || SETTING(UDP_PORT) != lastUDP || SETTING(TLS_PORT) != lastTLS, lastConn);
		} catch (const Exception& e) {
			showPortsError(e.getError());
		}

		if(BOOLSETTING(SORT_FAVUSERS_FIRST) != lastSortFavUsersFirst)
			HubFrame::resortUsers();

		if(BOOLSETTING(URL_HANDLER) != lastURLReg)
			WinUtil::registerHubHandlers();
		if(BOOLSETTING(MAGNET_REGISTER) != lastMagnetReg)
			WinUtil::registerMagnetHandler();

		ClientManager::getInstance()->infoUpdated();

		if(SETTING(SETTINGS_SAVE_INTERVAL) != lastSettingsSave)
			setSaveTimer();
	}
}

void MainWindow::showPortsError(const string& port) {
	dwt::MessageBox(this).show(Text::toT(str(F_("Unable to open %1% port. Searching or file transfers will not work correctly until you change settings or turn off any application that might be using that port.") % port)), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
}

void MainWindow::handleOpenFileList() {
	tstring file;
	if(WinUtil::browseFileList(this, file)) {
		UserPtr u = DirectoryListing::getUserFromFilename(Text::fromT(file));
		if (u) {
			DirectoryListingFrame::openWindow(getTabView(), file, Util::emptyStringT, HintedUser(u, Util::emptyString), 0);
		} else {
			dwt::MessageBox(this).show(T_("Invalid file list name"), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
		}
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
			HintedUser user(u, Util::emptyString);
			DirectoryListing dl(user);
			try {
				dl.loadFile(*i);
				int matched = QueueManager::getInstance()->matchListing(dl);
				LogManager::getInstance()->message(str(FN_("%1%: matched %2% file", "%1%: matched %2% files", matched)
				% Util::toString(ClientManager::getInstance()->getNicks(user))
				% matched));
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

	if( (j = cmdLine.find(_T("dchub://"), i)) != string::npos ||
		(j = cmdLine.find(_T("adc://"), i)) != string::npos ||
		(j = cmdLine.find(_T("adcs://"), i)) != string::npos ||
		(j = cmdLine.find(_T("magnet:?"), i)) != string::npos )
	{
		WinUtil::parseDBLClick(cmdLine.substr(j));
	}
}

LRESULT MainWindow::handleCopyData(LPARAM lParam) {
	parseCommandLine(Text::toT(WinUtil::getAppName() + " ") + reinterpret_cast<LPCTSTR>(reinterpret_cast<COPYDATASTRUCT*>(lParam)->lpData));
	return TRUE;
}

void MainWindow::handleHashProgress() {
	HashProgressDlg(this, false).run();
}

void MainWindow::handleCloseFavGroup(bool reversed) {
	tstring group;
	if(chooseFavHubGroup(reversed ? T_("Close hubs not in a favorite group") : T_("Close all hubs of a favorite group"), group))
		HubFrame::closeFavGroup(Text::fromT(group), reversed);
}

void MainWindow::handleAbout() {
	AboutDlg(this).run();
}

void MainWindow::handleOpenDownloadsDir() {
	WinUtil::openFile(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
}

void MainWindow::on(HttpConnectionListener::Complete, HttpConnection* /*aConn*/, const string&, bool) throw() {
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
							dwt::MessageBox(this).show(Text::toT(msg), Text::toT(title));
						} else {
							if(dwt::MessageBox(this).show(str(TF_("%1%\nOpen download page?") % Text::toT(xml.getChildData())), Text::toT(title), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES) {
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
			xml.resetCurrentChild();
			if(xml.findChild("Blog")) {
				links.blog = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Community")) {
				links.community = Text::toT(xml.getChildData());
			}
			xml.stepOut();
		}

		xml.resetCurrentChild();
		if(xml.findChild("Blacklist")) {
			xml.stepIn();
			while(xml.findChild("Blacklisted")) {
				const string& domain = xml.getChildAttrib("Domain");
				if(domain.empty())
					continue;
				const string& reason = xml.getChildAttrib("Reason");
				if(reason.empty())
					continue;
				FavoriteManager::getInstance()->addBlacklist(domain, reason);
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

	saveSettings();
	QueueManager::getInstance()->saveQueue();

	return 0;
}

void MainWindow::handleToolbarCustomized() {
	string setting;
	StringList list = toolbar->getLayout();
	for(StringIterC i = list.begin(), iend = list.end(); i != iend; ++i) {
		setting += *i;
		if(i != iend - 1)
			setting += ',';
	}
	SettingsManager::getInstance()->set(SettingsManager::TOOLBAR, setting);
}

bool MainWindow::handleToolbarContextMenu(const dwt::ScreenCoordinate& pt) {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);
	menu->setTitle(T_("Toolbar"));

	menu->appendItem(T_("&Customize\tDouble-click"), std::bind(&ToolBar::customize, toolbar),
		WinUtil::menuIcon(IDI_SETTINGS), true, true);

	{
		MenuPtr size = menu->appendPopup(T_("Size"));
		int sizes[] = { 16, 20, 22, 24, 32 };
		const int& setting = SETTING(TOOLBAR_SIZE);
		for(size_t i = 0, iend = sizeof(sizes) / sizeof(int); i < iend; ++i) {
			const int& n = sizes[i];
			unsigned pos = size->appendItem(Text::toT(Util::toString(n)), std::bind(&MainWindow::handleToolbarSize, this, n));
			if(n == setting)
				size->checkItem(pos);
		}
	}

	menu->appendSeparator();
	menu->appendItem(T_("&Hide\tCtrl+1"), std::bind(&MainWindow::switchToolbar, this));

	menu->open(pt);
	return true;
}

void MainWindow::handleToolbarSize(int size) {
	SettingsManager::getInstance()->set(SettingsManager::TOOLBAR_SIZE, size);
	switchToolbar();
	switchToolbar();
}

void MainWindow::switchToolbar() {
	if(toolbar) {
		rebar->remove(toolbar);
		::DestroyWindow(toolbar->handle());
		toolbar = 0;

		SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, false);
		viewMenu->checkItem(viewIndexes["Toolbar"], false);

	} else {
		SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, true);
		initToolbar();

		// re-check currently opened static windows
		const auto& views = tabs->getChildren();
		for(auto i = views.cbegin(), iend = views.cend(); i != iend; ++i) {
			toolbar->setButtonChecked(static_cast<MDIChildFrame<dwt::Container>*>(*i)->getId(), true);
		}
	}

	layout();
}

void MainWindow::switchTransfers() {
	if(transfers) {
		transfers->prepareClose();
		::DestroyWindow(transfers->handle());
		transfers = 0;
		paned->setSecond(transfers);

		SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, false);
		viewMenu->checkItem(viewIndexes["Transfers"], false);

	} else {
		SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, true);
		initTransfers();
	}

	layout();
}

void MainWindow::switchStatus() {
	if(status) {
		::DestroyWindow(status->handle());
		status = 0;

		::DestroyWindow(slotsSpin->handle());
		slotsSpin = 0;

		SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, false);
		viewMenu->checkItem(viewIndexes["Status"], false);

	} else {
		SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, true);
		initStatusBar();
	}

	layout();
}

bool MainWindow::handleSlotsUpdate(int /* pos */, int delta) {
	// Prevent double-info-updated
	int newSlots = SETTING(SLOTS) + delta;
	SettingsManager::getInstance()->set(SettingsManager::SLOTS, newSlots);

	SettingsManager::getInstance()->set(ThrottleManager::getInstance()->getCurSetting(SettingsManager::SLOTS), newSlots);
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

bool MainWindow::handleMessage(const MSG& msg, LRESULT& retVal) {
	bool handled = dwt::Window::handleMessage(msg, retVal);
	if(!handled && msg.message == WM_COMMAND && tabs) {
		handled = tabs->handleMessage(msg, retVal);
	}
	return handled;
}

void MainWindow::handleTrayContextMenu() {
	MenuPtr trayMenu = addChild(WinUtil::Seeds::menu);

	trayMenu->appendItem(T_("Show"), std::bind(&MainWindow::handleRestore, this), WinUtil::menuIcon(IDI_DCPP), true, true);
	trayMenu->appendItem(T_("Open downloads directory"), std::bind(&MainWindow::handleOpenDownloadsDir, this), WinUtil::menuIcon(IDI_OPEN_DL_DIR));
	trayMenu->appendItem(T_("Settings"), std::bind(&MainWindow::handleSettings, this), WinUtil::menuIcon(IDI_SETTINGS));
	trayMenu->appendSeparator();
	trayMenu->appendItem(T_("Exit"), std::bind(&MainWindow::close, this, true), WinUtil::menuIcon(IDI_EXIT));

	dwt::ScreenCoordinate pt;
	::GetCursorPos(&pt.getPoint());
	trayMenu->open(pt, TPM_BOTTOMALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON);
}

void MainWindow::handleTrayClicked() {
	if(isIconic()) {
		handleRestore();
	} else {
		minimize();
	}
}

void MainWindow::handleTrayUpdate() {
	notify->setTooltip(Text::toT(str(F_("D: %1%/s (%2%)\r\nU: %3%/s (%4%)") %
		Util::formatBytes(DownloadManager::getInstance()->getRunningAverage()) %
		DownloadManager::getInstance()->getDownloadCount() %
		Util::formatBytes(UploadManager::getInstance()->getRunningAverage()) %
		UploadManager::getInstance()->getUploadCount())));
}

void MainWindow::handleWhatsThis() {
	sendMessage(WM_SYSCOMMAND, SC_CONTEXTHELP);
}

void MainWindow::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw() {
	versionInfo += string((const char*)buf, len);
}

 void MainWindow::on(HttpConnectionListener::Retried, HttpConnection* /*conn*/, const bool Connected) throw() {
 	if (Connected)
 		versionInfo = Util::emptyString;
 }

void MainWindow::on(PartialList, const HintedUser& aUser, const string& text) throw() {
	callAsync([this, aUser, text] { DirectoryListingFrame::openWindow(getTabView(), aUser, text, 0); });
}

void MainWindow::on(QueueManagerListener::Finished, QueueItem* qi, const string& dir, int64_t speed) throw() {
	if (qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if (qi->isSet(QueueItem::FLAG_USER_LIST)) {
			tstring file = Text::toT(qi->getListName());
			HintedUser user = qi->getDownloads()[0]->getHintedUser();
			callAsync([this, file, dir, user, speed] { DirectoryListingFrame::openWindow(getTabView(), file, Text::toT(dir), user, speed); });
		} else if (qi->isSet(QueueItem::FLAG_TEXT)) {
			string file = qi->getTarget();
			callAsync([this, file] {
				TextFrame::openWindow(getTabView(), file);
				File::deleteFile(file);
			});
		}
	}
}

void MainWindow::on(WindowManagerListener::Window, const string& id, const StringMap& params) throw() {
	if(0);
#define compare_id(frame) else if(frame::id == id) callAsync([this, params] { frame::parseWindowParams(getTabView(), params); })
	compare_id(HubFrame);
	compare_id(PrivateFrame);
	compare_id(DirectoryListingFrame);
	compare_id(PublicHubsFrame);
	compare_id(FavHubsFrame);
	compare_id(UsersFrame);
	compare_id(QueueFrame);
	compare_id(FinishedDLFrame);
	compare_id(WaitingUsersFrame);
	compare_id(FinishedULFrame);
	compare_id(ADLSearchFrame);
	compare_id(SpyFrame);
	compare_id(NotepadFrame);
	compare_id(SystemFrame);
	compare_id(StatsFrame);
#undef compare_id
}
