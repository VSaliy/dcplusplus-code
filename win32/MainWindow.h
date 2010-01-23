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

#ifndef DCPLUSPLUS_WIN32_MAIN_WINDOW_H
#define DCPLUSPLUS_WIN32_MAIN_WINDOW_H

#include <dcpp/forward.h>

#include <dcpp/QueueManagerListener.h>
#include <dcpp/LogManagerListener.h>
#include <dcpp/WindowManagerListener.h>
#include <dcpp/HttpConnection.h>
#include <dcpp/User.h>

#include "AspectStatus.h"

class UPnP;
class TransferView;

class MainWindow :
	public dwt::Window,
	private HttpConnectionListener,
	private QueueManagerListener,
	private LogManagerListener,
	private WindowManagerListener,
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

	dwt::TabViewPtr getTabView() { return tabs; }

	virtual bool handleMessage( const MSG & msg, LRESULT & retVal );

	MainWindow();

	virtual ~MainWindow();

	void handleSettings();

	void setStaticWindowState(const string& id, bool open);

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
		tstring geoipfile;
		tstring faq;
		tstring help;
		tstring discuss;
		tstring features;
		tstring bugs;
		tstring donate;
	} links;

	WidgetHPanedPtr paned;
	MenuPtr mainMenu;
	MenuPtr viewMenu;
	TransferView* transfers;
	ToolBarPtr toolbar;
	dwt::TabViewPtr tabs;
	dwt::SpinnerPtr slotsSpin;

	typedef unordered_map<string, unsigned> StaticIndexes;
	StaticIndexes staticIndexes; /// indexes of menu commands of the "View" menu that open static windows

	/** Was the window maximized when minimizing it? */
	bool maximized;

	HttpConnection* c;
	string versionInfo;

	HANDLE stopperThread;

	int64_t lastUp;
	int64_t lastDown;
	uint64_t lastTick;

	dwt::Application::FilterIter filterIter;
	dwt::AcceleratorPtr accel;
	dwt::NotificationPtr notify;

	std::auto_ptr<UPnP> pUPnP;

	void initWindow();
	void initMenu();
	void initToolbar();
	void initStatusBar();
	void initTabs();
	void initTransfers();
	void initSecond();
	void initTray();

	// User actions
	void handleExit();
	void handleFavHubsDropDown(const dwt::ScreenCoordinate& pt);
	void handleRecent(const dwt::ScreenCoordinate& pt);
	void handleConfigureRecent(const string& id, const tstring& title);
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
	void handleForward(WPARAM wParam);
	LRESULT handleEndSession();
	void handleToolbarCustomized();
	bool handleToolbarContextMenu(const dwt::ScreenCoordinate& pt);
	bool handleSlotsUpdate(int pos, int delta);

	// Other events
	void handleSized(const dwt::SizedEvent& sz);
	void handleMinimized();

	LRESULT handleCopyData(LPARAM lParam);
	LRESULT handleWhereAreYou();

	void handleTabsTitleChanged(const tstring& title);

	void handleTrayContextMenu();
	void handleTrayClicked();
	void handleTrayUpdate();

	void layout();
	void layoutSlotsSpin();
	bool eachSecond();
	void updateStatus();
	void startSocket();
	void startUPnP();
	bool initUPnP();
	void stopUPnP();
	void saveWindowSettings();
	void parseCommandLine(const tstring& line);
	void viewAndDelete(const string& fileName);
	bool chooseFavHubGroup(const tstring& title, tstring& group);

	bool filter(MSG& msg);

	bool handleClosing();
	void handleRestore();

	static DWORD WINAPI stopper(void* p);

	// LogManagerListener
	virtual void on(LogManagerListener::Message, time_t t, const string& m) throw();

	// HttpConnectionListener
	virtual void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/, bool /*fromCoral*/) throw();
	virtual void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw();

	// QueueManagerListener
	virtual void on(QueueManagerListener::Finished, QueueItem* qi, const string& dir, int64_t speed) throw();
	virtual void on(PartialList, const HintedUser&, const string& text) throw();

	// WindowManagerListener
	virtual void on(WindowManagerListener::Window, const string& id, const StringMap& params) throw();
};

#endif // !defined(MAIN_FRM_H)
