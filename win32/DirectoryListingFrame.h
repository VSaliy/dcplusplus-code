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

#ifndef DCPLUSPLUS_WIN32_DIRECTORY_LISTING_FRAME_H
#define DCPLUSPLUS_WIN32_DIRECTORY_LISTING_FRAME_H

#include <dcpp/forward.h>
#include <dcpp/FastAlloc.h>
#include <dcpp/DirectoryListing.h>
#include <dcpp/ClientManagerListener.h>
#include <dcpp/User.h>

#include "MDIChildFrame.h"
#include "IRecent.h"
#include "UserInfoBase.h"
#include "AspectUserCommand.h"

class DirectoryListingFrame :
	public MDIChildFrame<DirectoryListingFrame>,
	public IRecent<DirectoryListingFrame>,
	private ClientManagerListener,
	public AspectUserInfo<DirectoryListingFrame>,
	public AspectUserCommand<DirectoryListingFrame>
{
	typedef MDIChildFrame<DirectoryListingFrame> BaseType;

	friend class MDIChildFrame<DirectoryListingFrame>;
	friend class AspectUserInfo<DirectoryListingFrame>;
	friend class AspectUserCommand<DirectoryListingFrame>;

	using IRecent<DirectoryListingFrame>::setText;

public:
	enum Status {
		STATUS_SHOW_TREE,
		STATUS_STATUS,
		STATUS_SPEED,
		STATUS_TOTAL_FILES,
		STATUS_TOTAL_SIZE,
		STATUS_SELECTED_FILES,
		STATUS_SELECTED_SIZE,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

	enum Activation {
		FOLLOW_SETTING,
		FORCE_ACTIVE,
		FORCE_INACTIVE
	};
	static void openWindow(TabViewPtr parent, const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed, Activation activate = FOLLOW_SETTING);
private:
	static void openWindow_(TabViewPtr parent, const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed, Activation activate);
public:
	static void openWindow(TabViewPtr parent, const HintedUser& aUser, const string& txt, int64_t aSpeed);
	static void activateWindow(const HintedUser& aUser);
	static void openOwnList(TabViewPtr parent, const tstring& dir = Util::emptyStringT, Activation activate = FOLLOW_SETTING);
	static void closeAll();

	WindowParams getWindowParams() const;
	static void parseWindowParams(TabViewPtr parent, const WindowParams& params);
	static bool isFavorite(const WindowParams& params);

private:
	enum {
		COLUMN_FILENAME,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_EXACTSIZE,
		COLUMN_TTH,
		COLUMN_LAST
	};

	class ItemInfo : public FastAlloc<ItemInfo> {
	public:
		enum ItemType {
			FILE,
			DIRECTORY,
			USER
		} type;

		union {
			DirectoryListing::File* file;
			DirectoryListing::Directory* dir;
		};

		ItemInfo(bool root, DirectoryListing::Directory* d) : type(USER), dir(d) { }

		ItemInfo(DirectoryListing::File* f) : type(FILE), file(f) {
			columns[COLUMN_FILENAME] = Text::toT(f->getName());
			columns[COLUMN_TYPE] = Util::getFileExt(columns[COLUMN_FILENAME]);
			if(columns[COLUMN_TYPE].size() > 0 && columns[COLUMN_TYPE][0] == '.')
				columns[COLUMN_TYPE].erase(0, 1);

			columns[COLUMN_EXACTSIZE] = Text::toT(Util::formatExactSize(f->getSize()));
			columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(f->getSize()));
			columns[COLUMN_TTH] = Text::toT(f->getTTH().toBase32());
		}
		ItemInfo(DirectoryListing::Directory* d) : type(DIRECTORY), dir(d) {
			columns[COLUMN_FILENAME] = Text::toT(d->getName());
			columns[COLUMN_EXACTSIZE] = d->getComplete() ? Text::toT(Util::formatExactSize(d->getTotalSize())) : _T("?");
			columns[COLUMN_SIZE] = d->getComplete() ? Text::toT(Util::formatBytes(d->getTotalSize())) : _T("?");
		}

		const tstring& getText() const {
			return columns[COLUMN_FILENAME];
		}

		int getImage(int col = 0) const;

		int getSelectedImage() const {
			return getImage(0);
		}

		const tstring& getText(int col) const {
			return columns[col];
		}

		void setText(const tstring& text, int col = COLUMN_FILENAME) {
			columns[col] = text;
		}

		struct TotalSize {
			TotalSize() : total(0) { }
			void operator()(ItemInfo* a) { total += a->type == DIRECTORY ? a->dir->getTotalSize() : a->file->getSize(); }
			int64_t total;
		};

		static int compareItems(ItemInfo* a, ItemInfo* b, int col);
	private:
		tstring columns[COLUMN_LAST];
	};
	
	RebarPtr rebar;
	ComboBoxPtr pathBox;

	GridPtr grid;

	GridPtr searchGrid;
	ComboBoxPtr searchBox;

	typedef TypedTree<ItemInfo> WidgetDirs;
	typedef WidgetDirs* WidgetDirsPtr;
	WidgetDirsPtr dirs;

	typedef TypedTable<ItemInfo> WidgetFiles;
	typedef WidgetFiles* WidgetFilesPtr;
	WidgetFilesPtr files;

	CheckBoxPtr showTree;

	int64_t speed;		/**< Speed at which this file list was downloaded */

	std::unique_ptr<DirectoryListing> dl;
	string path;

	// override the default match-queue method not to download the file list again.
	struct UserHolder : UserInfoBase {
		UserHolder(const HintedUser& u) : UserInfoBase(u) { }
		void matchQueue();
	} user;

	tstring error;
	bool usingDirMenu;
	StringList targets;
	deque<string> history;
	size_t historyIndex;

	HTREEITEM treeRoot;

	string size;

	bool loaded;
	bool updating;
	bool searching;

	static TStringList lastSearches;

	ParamMap ucLineParams;

	typedef unordered_map<UserPtr, DirectoryListingFrame*, User::Hash> UserMap;

	static UserMap lists;

	DirectoryListingFrame(TabViewPtr parent, const HintedUser& aUser, int64_t aSpeed);
	virtual ~DirectoryListingFrame();

	void layout();
	bool preClosing();
	void postClosing();

	ShellMenuPtr makeSingleMenu(ItemInfo* ii);
	ShellMenuPtr makeMultiMenu();
	ShellMenuPtr makeDirMenu(ItemInfo* ii);

	void runUserCommand(const UserCommand& uc);

	void addTargets(const MenuPtr& menu, ItemInfo* ii = 0);
	void addUserCommands(const MenuPtr& menu);
	void addShellPaths(const ShellMenuPtr& menu, const vector<ItemInfo*>& sel);
	void addUserMenu(const MenuPtr& menu);

	void handleFind(bool reverse);
	void handleListDiff();
	void handleMatchQueue();
	void handleFindToggle();

	void handleDownload();
	void handleViewAsText();
	void handleGoToDirectory();
	void handleDownloadLastDir(unsigned index);
	void handleDownloadTarget(unsigned index);
	void handleDownloadFavorite(unsigned index);
	void handleDownloadBrowse();
	bool handleKeyDownDirs(int c);
	bool handleKeyDownFiles(int c);
	bool handleSearchKeyDown(int c);
	bool handleSearchChar(int c);

	void handleDoubleClickFiles();
	void handleSelectionChanged();

	void download(const string& aDir);
	void download(ItemInfo* ii, const string& aDir, bool view = false);
	void downloadFiles(const string& aTarget, bool view = false);

	bool handleDirsContextMenu(dwt::ScreenCoordinate pt);
	bool handleFilesContextMenu(dwt::ScreenCoordinate pt);
	bool handleXMouseUp(const dwt::MouseEvent& mouseEvent);

	void changeDir(DirectoryListing::Directory* d);
	void updateTree(DirectoryListing::Directory* tree, HTREEITEM treeItem);
	HTREEITEM findItem(HTREEITEM ht, const tstring& name);
	void selectItem(const tstring& name);
	void clearList();
	void updateTitle();

	void loadFile(const tstring& dir);
	void loadXML(const string& txt);
	void refreshTree(const tstring& root);

	void addHistory(const string& name);
	void up();
	void back();
	void forward();

	void initStatusText();
	void updateStatus();

	void findFile(bool reverse);

	// MDIChildFrame
	void tabMenuImpl(dwt::MenuPtr& menu);

	// AspectUserInfo
	UserInfoList selectedUsersImpl();

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept;
	virtual void on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept;
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;
};

#endif
