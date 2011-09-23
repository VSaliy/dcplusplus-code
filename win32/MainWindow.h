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

#ifndef DCPLUSPLUS_WIN32_MAIN_WINDOW_H
#define DCPLUSPLUS_WIN32_MAIN_WINDOW_H

#include <dcpp/QueueManagerListener.h>
#include <dcpp/LogManagerListener.h>
#include <dcpp/HttpConnection.h>
#include <dcpp/User.h>
#include <dcpp/WindowInfo.h>

#include <dwt/widgets/Window.h>

#include "forward.h"
#include "AspectStatus.h"

class MainWindow :
	public dwt::Window,
	private HttpConnectionListener,
	private QueueManagerListener,
	private LogManagerListener,
	public AspectStatus<MainWindow>
{
public:
	enum Status {
		STATUS_STATUS,
		STATUS_AWAY,
		STATUS_COUNTS,
		STATUS_SLOTS,
		STATUS_SLOTS_SPIN,
		STATUS_DOWN_TOTAL,
		STATUS_UP_TOTAL,
		STATUS_DOWN_DIFF,
		STATUS_UP_DIFF,
		STATUS_DOWN_LIMIT,
		STATUS_UP_LIMIT,
		STATUS_DUMMY,
		STATUS_LAST
	};

	TabViewPtr getTabView() { return tabs; }

	virtual bool handleMessage( const MSG & msg, LRESULT & retVal );

	MainWindow();

	virtual ~MainWindow();

	void handleSettings();

	/** show a balloon popup. refer to the dwt::Notification::addMessage doc for info about parameters. */
	void notify(const tstring& title, const tstring& message, const std::function<void ()>& callback = 0, const dwt::IconPtr& balloonIcon = 0);
	void setStaticWindowState(const string& id, bool open);
	void TrayPM();

	bool closing() const { return stopperThread != 0; }

private:
	class DirectoryListInfo {
	public:
		DirectoryListInfo(const UserPtr& aUser, const tstring& aFile, const tstring& aDir, int64_t aSpeed) : user(aUser), file(aFile), dir(aDir), speed(aSpeed) { }
		UserPtr user;
		tstring file;
		tstring dir;
		int64_t speed;
	};
	class DirectoryBrowseInfo {
	public:
		DirectoryBrowseInfo(const UserPtr& ptr, string aText) : user(ptr), text(aText) { }
		UserPtr user;
		string text;
	};

	struct {
		tstring homepage;
		tstring downloads;
		tstring geoip6;
		tstring geoip4;
		tstring faq;
		tstring help;
		tstring discuss;
		tstring features;
		tstring bugs;
		tstring donate;
		tstring blog;
		tstring community;
	} links;

	enum {
		TIMER_STATUS,
		TIMER_SAVE
	};

	RebarPtr rebar;
	SplitterContainerPtr paned;
	MenuPtr mainMenu;
	MenuPtr viewMenu;
	TransferView* transfers;
	ToolBarPtr toolbar;
	TabViewPtr tabs;
	SpinnerPtr slotsSpin;

	typedef unordered_map<string, unsigned> ViewIndexes;
	ViewIndexes viewIndexes; /// indexes of menu commands of the "View" menu that open static windows

	bool tray_pm;

	HttpConnection* c;
	string versionInfo;

	HANDLE stopperThread;

	int64_t lastUp;
	int64_t lastDown;
	uint64_t lastTick;
	bool prevAway;
	bool fullSlots;

	dwt::Application::FilterIter filterIter;
	dwt::NotificationPtr notifier;

	void initWindow();
	void initMenu();
	void initToolbar();
	void initStatusBar();
	void initTabs();
	void initTransfers();
	void initTray();

	// User actions
	void handleFavHubsDropDown(const dwt::ScreenCoordinate& pt);
	void handleRecent(const dwt::ScreenCoordinate& pt);
	void handleConfigureRecent(const string& id, const tstring& title);
	void handleLimiterMenu(bool upload);
	void handleQuickConnect();
	void handleConnectFavHubGroup();
	void handleOpenFileList();
	void handleOpenOwnList();
	void handleRefreshFileList();
	void handleMatchAll();
	void handleOpenDownloadsDir();
	void handleCloseFavGroup(bool reversed);
	void handleAbout();
	void handleHashProgress();
	void handleWhatsThis();
	void handleSize();
	void handleActivate(bool active);
	LRESULT handleEndSession();
	void handleToolbarCustomized();
	bool handleToolbarContextMenu(const dwt::ScreenCoordinate& pt);
	void handleToolbarSize(int size);
	void switchToolbar();
	void switchTransfers();
	void switchStatus();
	bool handleSlotsUpdate(int delta);
	void handleReconnect();
	void handleRedirect();
	void forwardHub(void (HubFrame::*f_t)());

	// Other events
	void handleSized(const dwt::SizedEvent& sz);
	void handleMinimized();
	LRESULT handleActivateApp(WPARAM wParam);
	LRESULT handleCopyData(LPARAM lParam);
	LRESULT handleWhereAreYou();

	void handleTabsTitleChanged(const tstring& title);

	void handleTrayContextMenu();
	void handleTrayClicked();
	void handleTrayUpdate();

	void openWindow(const string& id, const WindowParams& params);
	void layout();
	void updateStatus();
	void updateAwayStatus();
	void showPortsError(const string& port);
	void setSaveTimer();
	void saveSettings();
	void saveWindowSettings();
	void parseCommandLine(const tstring& line);
	bool chooseFavHubGroup(const tstring& title, tstring& group);
	void fillLimiterMenu(MenuPtr menu, bool upload);
	void statusMessage(time_t t, const string& m);

	bool filter(MSG& msg);

	bool handleClosing();
	void handleRestore();

	static DWORD WINAPI stopper(void* p);

	// LogManagerListener
	virtual void on(LogManagerListener::Message, time_t t, const string& m) noexcept;

	// HttpConnectionListener
	virtual void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/, bool /*fromCoral*/) noexcept;
	virtual void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) noexcept;
	virtual void on(HttpConnectionListener::Retried, HttpConnection* /*conn*/, const bool Connected) noexcept;		
		
	// QueueManagerListener
	virtual void on(QueueManagerListener::Finished, QueueItem* qi, const string& dir, int64_t speed) noexcept;
	virtual void on(PartialList, const HintedUser&, const string& text) noexcept;
};

#endif // !defined(MAIN_FRM_H)
