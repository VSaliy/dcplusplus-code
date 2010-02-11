/*
* Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H
#define DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H

#include "StaticFrame.h"
#include "TypedTable.h"
#include "TextFrame.h"
#include "HoldRedraw.h"
#include "DirectoryListingFrame.h"

#include <dcpp/File.h>
#include <dcpp/FinishedItem.h>
#include <dcpp/FinishedManager.h>
#include <dcpp/TimerManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/DirectoryListing.h>
#include <dcpp/ShareManager.h>
#include <dcpp/ClientManager.h>

template<class T, bool in_UL>
class FinishedFrameBase :
	public StaticFrame<T>,
	private FinishedManagerListener
{
	typedef StaticFrame<T> BaseType;
	typedef FinishedFrameBase<T, in_UL> ThisType;

public:
	enum Status {
		STATUS_ONLY_FULL,
		STATUS_STATUS,
		STATUS_COUNT,
		STATUS_BYTES,
		STATUS_SPEED,
		STATUS_LAST
	};

protected:
	friend class StaticFrame<T>;
	friend class MDIChildFrame<T>;

	FinishedFrameBase(dwt::TabView* mdiParent, const tstring& title, unsigned helpId, unsigned resourceId) :
		BaseType(mdiParent, title, helpId, resourceId),
		tabs(0),
		files(0),
		filesWindow(0),
		users(0),
		usersWindow(0),
		onlyFull(0),
		bOnlyFull(false)
	{
		{
			dwt::TabView::Seed cs = WinUtil::Seeds::tabs;
			cs.location = dwt::Rectangle(this->getClientSize());
			cs.maxLength = 0;
			tabs = this->addChild(cs);
		}

		{
			dwt::Container::Seed cs;
			cs.caption = T_("Grouped by files");
			cs.location = tabs->getClientSize();
			filesWindow = dwt::WidgetCreator<dwt::Container>::create(tabs, cs);
			filesWindow->setHelpId(in_UL ? IDH_FINISHED_UL : IDH_FINISHED_DL);
			filesWindow->onClosing(std::tr1::bind(&noClose));
			tabs->add(filesWindow, dwt::IconPtr(new dwt::Icon(in_UL ? IDR_FINISHED_UL_BY_FILE : IDR_FINISHED_DL_BY_FILE)));

			cs.style &= ~WS_VISIBLE;
			cs.caption = T_("Grouped by users");
			usersWindow = dwt::WidgetCreator<dwt::Container>::create(tabs, cs);
			usersWindow->setHelpId(in_UL ? IDH_FINISHED_UL : IDH_FINISHED_DL);
			usersWindow->onClosing(std::tr1::bind(&noClose));
			tabs->add(usersWindow, dwt::IconPtr(new dwt::Icon(in_UL ? IDR_FINISHED_UL_BY_USER : IDR_FINISHED_DL_BY_USER)));
		}

		{
			files = filesWindow->addChild(typename WidgetFiles::Seed());
			files->setTableStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
			addWidget(files);

			WinUtil::makeColumns(files, filesColumns, FILES_COLUMN_LAST, SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_FILES_ORDER : SettingsManager::FINISHED_DL_FILES_ORDER),
				SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_FILES_WIDTHS : SettingsManager::FINISHED_DL_FILES_WIDTHS));
			files->setSort(FILES_COLUMN_TIME);

			files->setSmallImageList(WinUtil::fileImages);

			files->onDblClicked(std::tr1::bind(&ThisType::handleOpenFile, this));
			files->onKeyDown(std::tr1::bind(&ThisType::handleFilesKeyDown, this, _1));
			files->onContextMenu(std::tr1::bind(&ThisType::handleFilesContextMenu, this, _1));
		}

		{
			users = usersWindow->addChild(typename WidgetUsers::Seed());
			users->setTableStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
			addWidget(users);

			WinUtil::makeColumns(users, usersColumns, USERS_COLUMN_LAST, SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_USERS_ORDER : SettingsManager::FINISHED_DL_USERS_ORDER),
				SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_USERS_WIDTHS : SettingsManager::FINISHED_DL_USERS_WIDTHS));
			users->setSort(USERS_COLUMN_TIME);

			users->setSmallImageList(WinUtil::userImages);

			users->onKeyDown(std::tr1::bind(&ThisType::handleUsersKeyDown, this, _1));
			users->onContextMenu(std::tr1::bind(&ThisType::handleUsersContextMenu, this, _1));
		}

		filesWindow->onSized(std::tr1::bind(&ThisType::fills, filesWindow, files));
		usersWindow->onSized(std::tr1::bind(&ThisType::fills, usersWindow, users));

		if(!in_UL) {
			bOnlyFull = BOOLSETTING(FINISHED_DL_ONLY_FULL);
			{
				CheckBox::Seed seed = WinUtil::Seeds::checkBox;
				seed.caption = T_("Only show fully downloaded files");
				onlyFull = this->addChild(seed);
				onlyFull->setHelpId(IDH_FINISHED_DL_ONLY_FULL);
				onlyFull->setChecked(bOnlyFull);
				onlyFull->onClicked(std::tr1::bind(&ThisType::handleOnlyFullClicked, this));
			}

			filesWindow->onActivate(std::tr1::bind(&dwt::CheckBox::setVisible, onlyFull, _1));
		}

		this->initStatus();
		if(onlyFull)
			this->status->setSize(STATUS_ONLY_FULL, onlyFull->getPreferedSize().x);
		this->status->onDblClicked(STATUS_STATUS, std::tr1::bind(&WinUtil::openFile, Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(in_UL ? LogManager::UPLOAD : LogManager::DOWNLOAD)))));

		this->status->setHelpId(STATUS_COUNT, in_UL ? IDH_FINISHED_UL_COUNT : IDH_FINISHED_DL_COUNT);
		this->status->setHelpId(STATUS_BYTES, in_UL ? IDH_FINISHED_UL_BYTES : IDH_FINISHED_DL_BYTES);
		this->status->setHelpId(STATUS_SPEED, in_UL ? IDH_FINISHED_UL_SPEED : IDH_FINISHED_DL_SPEED);

		layout();
		this->activate();

		filesWindow->onActivate(std::tr1::bind(&ThisType::updateStatus, this, _1));
		usersWindow->onActivate(std::tr1::bind(&ThisType::updateStatus, this, _1));

		FinishedManager::getInstance()->addListener(this);

		updateLists();
	}

	virtual ~FinishedFrameBase() { }

	void layout() {
		dwt::Rectangle r(this->getClientSize());

		this->status->layout(r);
		if(onlyFull)
			this->status->mapWidget(STATUS_ONLY_FULL, onlyFull);

		tabs->layout(r);
	}

	bool preClosing() {
		FinishedManager::getInstance()->removeListener(this);
		return true;
	}

	void postClosing() {
		clearTables();

		saveColumns(files,
			in_UL ? SettingsManager::FINISHED_UL_FILES_ORDER : SettingsManager::FINISHED_DL_FILES_ORDER,
			in_UL ? SettingsManager::FINISHED_UL_FILES_WIDTHS : SettingsManager::FINISHED_DL_FILES_WIDTHS);
		saveColumns(users,
			in_UL ? SettingsManager::FINISHED_UL_USERS_ORDER : SettingsManager::FINISHED_DL_USERS_ORDER,
			in_UL ? SettingsManager::FINISHED_UL_USERS_WIDTHS : SettingsManager::FINISHED_DL_USERS_WIDTHS);

		if(!in_UL)
			SettingsManager::getInstance()->set(SettingsManager::FINISHED_DL_ONLY_FULL, bOnlyFull);
	}

private:
	enum {
		FILES_COLUMN_FIRST,
		FILES_COLUMN_FILE = FILES_COLUMN_FIRST,
		FILES_COLUMN_PATH,
		FILES_COLUMN_NICKS,
		FILES_COLUMN_TRANSFERRED,
		FILES_COLUMN_FILESIZE,
		FILES_COLUMN_PERCENTAGE,
		FILES_COLUMN_SPEED,
		FILES_COLUMN_CRC32,
		FILES_COLUMN_TIME,
		FILES_COLUMN_LAST
	};

	enum {
		USERS_COLUMN_FIRST,
		USERS_COLUMN_NICK = USERS_COLUMN_FIRST,
		USERS_COLUMN_HUB,
		USERS_COLUMN_TRANSFERRED,
		USERS_COLUMN_SPEED,
		USERS_COLUMN_FILES,
		USERS_COLUMN_TIME,
		USERS_COLUMN_LAST
	};

	static const ColumnInfo filesColumns[FILES_COLUMN_LAST];

	static const ColumnInfo usersColumns[USERS_COLUMN_LAST];

	class FileInfo : public FastAlloc<FileInfo> {
	public:
		FileInfo(const string& file_, const FinishedFileItemPtr& entry_) : file(file_), entry(entry_) {
			columns[FILES_COLUMN_FILE] = Text::toT(Util::getFileName(file));
			columns[FILES_COLUMN_PATH] = Text::toT(Util::getFilePath(file));
			columns[FILES_COLUMN_FILESIZE] = Text::toT(Util::formatBytes(entry->getFileSize()));

			update();
		}

		/// @return whether the list needs to be re-sorted.
		bool update(int sortCol = -1) {
			tstring old;
			if(sortCol != -1)
				old = columns[sortCol];

			{
				StringList nicks;
				for(HintedUserList::const_iterator i = entry->getUsers().begin(), iend = entry->getUsers().end(); i != iend; ++i)
					nicks.push_back(Util::toString(ClientManager::getInstance()->getNicks(*i)));
				columns[FILES_COLUMN_NICKS] = Text::toT(Util::toString(nicks));
			}
			columns[FILES_COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(entry->getTransferred()));
			columns[FILES_COLUMN_PERCENTAGE] = Text::toT(Util::toString(entry->getTransferredPercentage()) + '%');
			columns[FILES_COLUMN_SPEED] = Text::toT(Util::formatBytes(entry->getAverageSpeed()) + "/s");
			columns[FILES_COLUMN_CRC32] = entry->getCrc32Checked() ? T_("Yes") : T_("No");
			columns[FILES_COLUMN_TIME] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));

			return (sortCol != -1) ? (old != columns[sortCol]) : false;
		}

		const tstring& getText(int col) const {
			return columns[col];
		}
		int getImage() const {
			return WinUtil::getIconIndex(Text::toT(file));
		}

		static int compareItems(FileInfo* a, FileInfo* b, int col) {
			switch(col) {
				case FILES_COLUMN_TRANSFERRED: return compare(a->entry->getTransferred(), b->entry->getTransferred());
				case FILES_COLUMN_FILESIZE: return compare(a->entry->getFileSize(), b->entry->getFileSize());
				case FILES_COLUMN_PERCENTAGE: return compare(a->entry->getTransferredPercentage(), b->entry->getTransferredPercentage());
				case FILES_COLUMN_SPEED: return compare(a->entry->getAverageSpeed(), b->entry->getAverageSpeed());
				default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
			}
		}

		void viewAsText(dwt::TabViewPtr parent) {
			TextFrame::openWindow(parent, file);
		}

		void open(dwt::TabViewPtr parent, const string& ownList) {
			if (in_UL && file == ownList) {
				DirectoryListingFrame::openOwnList(parent);
				return;
			}

			UserPtr u = DirectoryListing::getUserFromFilename(file);
			if(u)
				DirectoryListingFrame::openWindow(parent, Text::toT(file), Util::emptyStringT, HintedUser(u, Util::emptyString), entry->getAverageSpeed());
			else
				WinUtil::openFile(Text::toT(file));
		}

		void openFolder() {
			WinUtil::openFolder(Text::toT(file));
		}

		void remove() {
			FinishedManager::getInstance()->remove(in_UL, file);
		}

		string file;
		FinishedFileItemPtr entry;

	private:
		tstring columns[FILES_COLUMN_LAST];
	};

	class UserInfo : public FastAlloc<UserInfo> {
	public:
		UserInfo(const HintedUser& user_, const FinishedUserItemPtr& entry_) : user(user_), entry(entry_) {
			update();
		}

		/// @return whether the list needs to be re-sorted.
		bool update(int sortCol = -1) {
			tstring old;
			if(sortCol != -1)
				old = columns[sortCol];

			columns[USERS_COLUMN_NICK] = WinUtil::getNicks(user);
			columns[USERS_COLUMN_HUB] = Text::toT(Util::toString(ClientManager::getInstance()->getHubNames(user)));
			columns[USERS_COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(entry->getTransferred()));
			columns[USERS_COLUMN_SPEED] = Text::toT(Util::formatBytes(entry->getAverageSpeed()) + "/s");
			columns[USERS_COLUMN_FILES] = Text::toT(Util::toString(entry->getFiles()));
			columns[USERS_COLUMN_TIME] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));

			return (sortCol != -1) ? (old != columns[sortCol]) : false;
		}

		const tstring& getText(int col) const {
			return columns[col];
		}
		int getImage() const {
			return 0;
		}

		static int compareItems(UserInfo* a, UserInfo* b, int col) {
			switch(col) {
				case USERS_COLUMN_TRANSFERRED: return compare(a->entry->getTransferred(), b->entry->getTransferred());
				case USERS_COLUMN_SPEED: return compare(a->entry->getAverageSpeed(), b->entry->getAverageSpeed());
				default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
			}
		}

		void remove() {
			FinishedManager::getInstance()->remove(in_UL, user);
		}

		HintedUser user;
		FinishedUserItemPtr entry;

	private:
		tstring columns[USERS_COLUMN_LAST];
	};

	dwt::TabViewPtr tabs;

	typedef TypedTable<FileInfo> WidgetFiles;
	typedef WidgetFiles* WidgetFilesPtr;
	WidgetFilesPtr files;
	dwt::ContainerPtr filesWindow;

	typedef TypedTable<UserInfo> WidgetUsers;
	typedef WidgetUsers* WidgetUsersPtr;
	WidgetUsersPtr users;
	dwt::ContainerPtr usersWindow;

	dwt::CheckBoxPtr onlyFull;
	bool bOnlyFull;

	static bool noClose() {
		return false;
	}

	static void fills(dwt::ContainerPtr parent, dwt::TablePtr control) {
		control->layout(dwt::Rectangle(parent->getClientSize()));
	}

	bool handleFilesKeyDown(int c) {
		switch(c) {
			case VK_RETURN: handleOpenFile(); return true;
			case VK_DELETE: handleRemoveFiles(); return true;
			default: return false;
		}
	}

	bool handleUsersKeyDown(int c) {
		switch(c) {
			case VK_DELETE: handleRemoveUsers(); return true;
			default: return false;
		}
	}

	struct UserCollector {
		void operator()(FileInfo* data) {
			const HintedUserList& users_ = data->entry->getUsers();
			users.insert(users.end(), users_.begin(), users_.end());
		}
		void operator()(UserInfo* data) {
			users.push_back(data->user);
		}
		HintedUserList users;
	};

	struct FileChecker {
		FileChecker() : allFilesExist(true), isBz2(false) { }
		void operator()(FileInfo* data) {
			if(File::getSize(data->file) != -1)
				ShellMenuPaths.push_back(data->file);
			else
				allFilesExist = false;
 			isBz2 |= Util::getFileExt(data->file) == ".bz2";
 		}
		bool allFilesExist;
		StringList ShellMenuPaths;
		bool isBz2;
	};

	bool handleFilesContextMenu(dwt::ScreenCoordinate pt) {
		if(files->hasSelected()) {
			if(pt.x() == -1 && pt.y() == -1) {
				pt = files->getContextMenuPos();
			}

			FileChecker checker = files->forEachSelectedT(FileChecker());

			ShellMenuPtr menu = filesWindow->addChild(ShellMenu::Seed());

			menu->appendItem(T_("&View as text"), std::tr1::bind(&ThisType::handleViewAsText, this), dwt::IconPtr(), checker.allFilesExist && !checker.isBz2);
			menu->appendItem(T_("&Open"), std::tr1::bind(&ThisType::handleOpenFile, this), dwt::IconPtr(), checker.allFilesExist, true);
			menu->appendItem(T_("Open &folder"), std::tr1::bind(&ThisType::handleOpenFolder, this));
			menu->appendSeparator();
			menu->appendItem(T_("&Remove"), std::tr1::bind(&ThisType::handleRemoveFiles, this));
			menu->appendItem(T_("Remove &all"), std::tr1::bind(&ThisType::handleRemoveAll, this));
			menu->appendSeparator();
			WinUtil::addUserItems(menu, files->forEachSelectedT(UserCollector()).users, this->getParent());
			menu->appendShellMenu(checker.ShellMenuPaths);

			menu->open(pt);
			return true;
		}
		return false;
	}

	bool handleUsersContextMenu(dwt::ScreenCoordinate pt) {
		if(users->hasSelected()) {
			if(pt.x() == -1 && pt.y() == -1) {
				pt = users->getContextMenuPos();
			}

			dwt::MenuPtr menu = usersWindow->addChild(WinUtil::Seeds::menu);
			menu->appendItem(T_("&Remove"), std::tr1::bind(&ThisType::handleRemoveUsers, this));
			menu->appendItem(T_("Remove &all"), std::tr1::bind(&ThisType::handleRemoveAll, this));
			menu->appendSeparator();
			WinUtil::addUserItems(menu, users->forEachSelectedT(UserCollector()).users, this->getParent());

			menu->open(pt);
			return true;
		}
		return false;
	}

	void handleViewAsText() {
		files->forEachSelectedT(std::tr1::bind(&FileInfo::viewAsText, _1, this->getParent()));
	}

	void handleOpenFile() {
		files->forEachSelectedT(std::tr1::bind(&FileInfo::open, _1, this->getParent(),
			in_UL ? ShareManager::getInstance()->getOwnListFile() : Util::emptyString));
	}

	void handleOpenFolder() {
		files->forEachSelected(&FileInfo::openFolder);
	}

	void handleRemoveFiles() {
		files->forEachSelected(&FileInfo::remove);
	}

	void handleRemoveUsers() {
		users->forEachSelected(&UserInfo::remove);
	}

	void handleRemoveAll() {
		FinishedManager::getInstance()->removeAll(in_UL);
	}

	void handleOnlyFullClicked() {
		bOnlyFull = onlyFull->getChecked();

		clearTables();
		updateLists();
	}

	void updateLists() {
		FinishedManager::getInstance()->lockLists();
		{
			HoldRedraw hold(files);
			const FinishedManager::MapByFile& map = FinishedManager::getInstance()->getMapByFile(in_UL);
			for(FinishedManager::MapByFile::const_iterator i = map.begin(); i != map.end(); ++i)
				addFile(i->first, i->second);
		}
		{
			HoldRedraw hold(users);
			const FinishedManager::MapByUser& map = FinishedManager::getInstance()->getMapByUser(in_UL);
			for(FinishedManager::MapByUser::const_iterator i = map.begin(); i != map.end(); ++i)
				addUser(i->first, i->second);
		}
		FinishedManager::getInstance()->unLockLists();

		updateStatus();
	}

	void updateStatus(bool activate = true) {
		if(!activate)
			return;

		size_t count = 0;
		int64_t bytes = 0;
		int64_t time = 0;
		if(users->getVisible()) {
			count = users->size();
			for(size_t i = 0; i < count; ++i) {
				const FinishedUserItemPtr& entry = users->getData(i)->entry;
				bytes += entry->getTransferred();
				time += entry->getMilliSeconds();
			}
		} else {
			count = files->size();
			for(size_t i = 0; i < count; ++i) {
				const FinishedFileItemPtr& entry = files->getData(i)->entry;
				bytes += entry->getTransferred();
				time += entry->getMilliSeconds();
			}
		}

		this->status->setText(STATUS_COUNT, str(TFN_("%1% item", "%1% items", count) % count));
		this->status->setText(STATUS_BYTES, Text::toT(Util::formatBytes(bytes)));
		this->status->setText(STATUS_SPEED, str(TF_("%1%/s") % Text::toT(Util::formatBytes((time > 0) ? bytes * ((int64_t)1000) / time : 0))));
	}

	bool addFile(const string& file, const FinishedFileItemPtr& entry) {
		if(bOnlyFull && !entry->isFull())
			return false;

		int loc = files->insert(new FileInfo(file, entry));
		if(files->getVisible())
			files->ensureVisible(loc);
		return true;
	}

	void addUser(const HintedUser& user, const FinishedUserItemPtr& entry) {
		int loc = users->insert(new UserInfo(user, entry));
		if(users->getVisible())
			users->ensureVisible(loc);
	}

	FileInfo* findFileInfo(const string& file) {
		for(size_t i = 0; i < files->size(); ++i) {
			FileInfo* data = files->getData(i);
			if(data->file == file)
				return data;
		}
		return 0;
	}

	UserInfo* findUserInfo(const HintedUser& user) {
		for(size_t i = 0; i < users->size(); ++i) {
			UserInfo* data = users->getData(i);
			if(data->user == user)
				return data;
		}
		return 0;
	}

	void saveColumns(dwt::TablePtr table, SettingsManager::StrSetting order, SettingsManager::StrSetting widths) {
		SettingsManager::getInstance()->set(order, WinUtil::toString(table->getColumnOrder()));
		SettingsManager::getInstance()->set(widths, WinUtil::toString(table->getColumnWidths()));
	}

	template<typename TableType>
	void clearTable(TableType* table) {
		table->clear();
	}
	inline void clearTables() {
		clearTable(files);
		clearTable(users);
	}

	void onAddedFile(const string& file, const FinishedFileItemPtr& entry) {
		FileInfo* data = findFileInfo(file);
		if(data) {
			// this file already exists; simply update it
			onUpdatedFile(file);
		} else if(addFile(file, entry)) {
			updateStatus();
			this->setDirty(in_UL ? SettingsManager::BOLD_FINISHED_UPLOADS : SettingsManager::BOLD_FINISHED_DOWNLOADS);
		}
	}

	void onAddedUser(const HintedUser& user, const FinishedUserItemPtr& entry) {
		addUser(user, entry);
		updateStatus();
	}

	void onUpdatedFile(const string& file) {
		FileInfo* data = findFileInfo(file);
		if(data) {
			bool resort = data->update(files->getSortColumn());
			files->update(data);
			if(resort)
				files->resort();
			updateStatus();
		}
	}

	void onUpdatedUser(const HintedUser& user) {
		UserInfo* data = findUserInfo(user);
		if(data) {
			bool resort = data->update(users->getSortColumn());
			users->update(data);
			if(resort)
				users->resort();
			updateStatus();
		}
	}

	void onRemovedFile(const string& file) {
		FileInfo* data = findFileInfo(file);
		if(data) {
			files->erase(data);
			updateStatus();
		}
	}

	void onRemovedUser(const HintedUser& user) {
		UserInfo* data = findUserInfo(user);
		if(data) {
			users->erase(data);
			updateStatus();
		}
	}

	void onRemovedAll() {
		clearTables();
		updateStatus();
	}

	virtual void on(AddedFile, bool upload, const string& file, const FinishedFileItemPtr& entry) throw() {
		if(upload == in_UL)
			callAsync(std::tr1::bind(&ThisType::onAddedFile, this, file, entry));
	}

	virtual void on(AddedUser, bool upload, const HintedUser& user, const FinishedUserItemPtr& entry) throw() {
		if(upload == in_UL)
			callAsync(std::tr1::bind(&ThisType::onAddedUser, this, user, entry));
	}

	virtual void on(UpdatedFile, bool upload, const string& file, const FinishedFileItemPtr& entry) throw() {
		if(upload == in_UL) {
			if(bOnlyFull && entry->isFull())
				callAsync(std::tr1::bind(&ThisType::onAddedFile, this, file, entry));
			else
				callAsync(std::tr1::bind(&ThisType::onUpdatedFile, this, file));
		}
	}

	virtual void on(UpdatedUser, bool upload, const HintedUser& user) throw() {
		if(upload == in_UL)
			callAsync(std::tr1::bind(&ThisType::onUpdatedUser, this, user));
	}

	virtual void on(RemovedFile, bool upload, const string& file) throw() {
		if(upload == in_UL)
			callAsync(std::tr1::bind(&ThisType::onRemovedFile, this, file));
	}

	virtual void on(RemovedUser, bool upload, const HintedUser& user) throw() {
		if(upload == in_UL)
			callAsync(std::tr1::bind(&ThisType::onRemovedUser, this, user));
	}

	virtual void on(RemovedAll, bool upload) throw() {
		if(upload == in_UL)
			callAsync(std::tr1::bind(&ThisType::onRemovedAll, this));
	}
};

template<class T, bool in_UL>
const ColumnInfo FinishedFrameBase<T, in_UL>::filesColumns[] = {
	{ N_("Filename"), 125, false },
	{ N_("Path"), 250, false },
	{ N_("Nicks"), 250, false },
	{ N_("Transferred"), 80, true },
	{ N_("File size"), 80, true },
	{ N_("% transferred"), 80, true },
	{ N_("Speed"), 100, true },
	{ N_("CRC Checked"), 80, false },
	{ N_("Time"), 125, false }
};

template<class T, bool in_UL>
const ColumnInfo FinishedFrameBase<T, in_UL>::usersColumns[] = {
	{ N_("Nick"), 125, false },
	{ N_("Hub"), 125, false },
	{ N_("Transferred"), 80, true },
	{ N_("Speed"), 100, true },
	{ N_("Files"), 300, false },
	{ N_("Time"), 125, false }
};

#endif // !defined(DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H)
