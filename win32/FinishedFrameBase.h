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

#ifndef DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H
#define DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H

#include "StaticFrame.h"
#include "TypedTable.h"
#include "TextFrame.h"
#include "ShellContextMenu.h"
#include "HoldRedraw.h"

#include <dcpp/File.h>
#include <dcpp/TaskQueue.h>
#include <dcpp/FinishedItem.h>
#include <dcpp/FinishedManager.h>
#include <dcpp/TimerManager.h>
#include <dcpp/ClientManager.h>

static void fills(dwt::ContainerPtr parent, dwt::TablePtr control) {
	control->setBounds(dwt::Rectangle(parent->getClientAreaSize()));
}

template<class T, bool in_UL>
class FinishedFrameBase :
	public StaticFrame<T>,
	private FinishedManagerListener
{
	typedef StaticFrame<T> BaseType;
	typedef FinishedFrameBase<T, in_UL> ThisType;

public:
	enum Status {
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
		usersWindow(0)
	{
		{
			dwt::TabView::Seed cs;
			cs.location = this->getBounds();
			tabs = this->addChild(cs);
		}

		{
			dwt::Container::Seed cs;
			cs.caption = T_("Grouped by files");
			cs.background = (HBRUSH)(COLOR_3DFACE + 1);
			cs.location = tabs->getClientSize();
			filesWindow = dwt::WidgetCreator<dwt::Container>::create(tabs, cs);
			tabs->add(filesWindow);

			cs.style &= ~WS_VISIBLE;
			cs.caption = T_("Grouped by users");
			usersWindow = dwt::WidgetCreator<dwt::Container>::create(tabs, cs);
			tabs->add(usersWindow);
		}

		{
			files = filesWindow->addChild(typename WidgetFiles::Seed());
			files->setTableStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
			addWidget(files);

			files->createColumns(WinUtil::getStrings(filesNames));
			files->setColumnOrder(WinUtil::splitTokens(SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_FILES_ORDER : SettingsManager::FINISHED_DL_FILES_ORDER), filesIndexes));
			files->setColumnWidths(WinUtil::splitTokens(SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_FILES_WIDTHS : SettingsManager::FINISHED_DL_FILES_WIDTHS), filesSizes));
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

			users->createColumns(WinUtil::getStrings(usersNames));
			users->setColumnOrder(WinUtil::splitTokens(SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_USERS_ORDER : SettingsManager::FINISHED_DL_USERS_ORDER), usersIndexes));
			users->setColumnWidths(WinUtil::splitTokens(SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_USERS_WIDTHS : SettingsManager::FINISHED_DL_USERS_WIDTHS), usersSizes));
			users->setSort(USERS_COLUMN_TIME);

			users->setSmallImageList(WinUtil::userImages);

			users->onKeyDown(std::tr1::bind(&ThisType::handleUsersKeyDown, this, _1));
			users->onContextMenu(std::tr1::bind(&ThisType::handleUsersContextMenu, this, _1));
		}

		filesWindow->onSized(std::tr1::bind(&fills, filesWindow, files));
		usersWindow->onSized(std::tr1::bind(&fills, usersWindow, users));

		this->initStatus();

		this->setStatusHelpId(STATUS_COUNT, in_UL ? IDH_FINISHED_UL_COUNT : IDH_FINISHED_DL_COUNT);
		this->setStatusHelpId(STATUS_BYTES, in_UL ? IDH_FINISHED_UL_BYTES : IDH_FINISHED_DL_BYTES);
		this->setStatusHelpId(STATUS_SPEED, in_UL ? IDH_FINISHED_UL_SPEED : IDH_FINISHED_DL_SPEED);

		layout();

		filesWindow->onActivate(std::tr1::bind(&ThisType::updateStatus, this, _1));
		usersWindow->onActivate(std::tr1::bind(&ThisType::updateStatus, this, _1));

		onSpeaker(std::tr1::bind(&ThisType::handleSpeaker, this));

		FinishedManager::getInstance()->addListener(this);

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

	virtual ~FinishedFrameBase() { }

	void layout() {
		dwt::Rectangle r(this->getClientAreaSize());

		this->layoutStatus(r);
		tabs->setBounds(r);
	}

	bool preClosing() {
		FinishedManager::getInstance()->removeListener(this);
		return true;
	}

	void postClosing() {
		tasks.clear();
		clearTables();

		saveColumns(files,
			in_UL ? SettingsManager::FINISHED_UL_FILES_ORDER : SettingsManager::FINISHED_DL_FILES_ORDER,
			in_UL ? SettingsManager::FINISHED_UL_FILES_WIDTHS : SettingsManager::FINISHED_DL_FILES_WIDTHS);
		saveColumns(users,
			in_UL ? SettingsManager::FINISHED_UL_USERS_ORDER : SettingsManager::FINISHED_DL_USERS_ORDER,
			in_UL ? SettingsManager::FINISHED_UL_USERS_WIDTHS : SettingsManager::FINISHED_DL_USERS_WIDTHS);
	}

private:
	enum Tasks {
		ADD_FILE,
		ADD_USER,
		UPDATE_FILE,
		UPDATE_USER,
		REMOVE_FILE,
		REMOVE_USER,
		REMOVE_ALL
	};

	struct FileItemTask : public Task {
		FileItemTask(const string& file_, const FinishedFileItemPtr& entry_) : file(file_), entry(entry_) { }

		string file;
		FinishedFileItemPtr entry;
	};

	struct UserItemTask : public Task {
		UserItemTask(const UserPtr& user_, const FinishedUserItemPtr& entry_) : user(user_), entry(entry_) { }

		UserPtr user;
		FinishedUserItemPtr entry;
	};

	struct UserTask : public Task {
		UserTask(const UserPtr& user_) : user(user_) { }

		UserPtr user;
	};

	enum {
		FILES_COLUMN_FIRST,
		FILES_COLUMN_FILE = FILES_COLUMN_FIRST,
		FILES_COLUMN_PATH,
		FILES_COLUMN_NICKS,
		FILES_COLUMN_TRANSFERRED,
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

	static int filesSizes[FILES_COLUMN_LAST];
	static int filesIndexes[FILES_COLUMN_LAST];
	static const char* filesNames[FILES_COLUMN_LAST];

	static int usersSizes[USERS_COLUMN_LAST];
	static int usersIndexes[USERS_COLUMN_LAST];
	static const char* usersNames[USERS_COLUMN_LAST];

	class FileInfo : public FastAlloc<FileInfo> {
	public:
		FileInfo(const string& file_, const FinishedFileItemPtr& entry_) : file(file_), entry(entry_) {
			columns[FILES_COLUMN_FILE] = Text::toT(Util::getFileName(file));
			columns[FILES_COLUMN_PATH] = Text::toT(Util::getFilePath(file));
			update();
		}

		void update() {
			{
				StringList nicks;
				for(FinishedFileItem::UserSet::const_iterator i = entry->getUsers().begin(); i != entry->getUsers().end(); ++i)
					nicks.push_back(Util::toString(ClientManager::getInstance()->getNicks((*i)->getCID())));
				columns[FILES_COLUMN_NICKS] = Text::toT(Util::toString(nicks));
			}
			columns[FILES_COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(entry->getTransferred()));
			columns[FILES_COLUMN_SPEED] = Text::toT(Util::formatBytes(entry->getAverageSpeed()) + "/s");
			columns[FILES_COLUMN_CRC32] = entry->getCrc32Checked() ? T_("Yes") : T_("No");
			columns[FILES_COLUMN_TIME] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));
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
				case FILES_COLUMN_SPEED: return compare(a->entry->getAverageSpeed(), b->entry->getAverageSpeed());
				default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
			}
		}

		void openFile() {
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
		UserInfo(const UserPtr& user_, const FinishedUserItemPtr& entry_) : user(user_), entry(entry_) {
			columns[USERS_COLUMN_NICK] = WinUtil::getNicks(user);
			columns[USERS_COLUMN_HUB] = Text::toT(Util::toString(ClientManager::getInstance()->getHubNames(user->getCID())));
			update();
		}

		void update() {
			columns[USERS_COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(entry->getTransferred()));
			columns[USERS_COLUMN_SPEED] = Text::toT(Util::formatBytes(entry->getAverageSpeed()) + "/s");
			columns[USERS_COLUMN_FILES] = Text::toT(Util::toString(entry->getFiles()));
			columns[USERS_COLUMN_TIME] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));
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

		UserPtr user;
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

	TaskQueue tasks;

	LRESULT handleSpeaker() {
		TaskQueue::List t;
		tasks.get(t);
		for(TaskQueue::Iter i = t.begin(); i != t.end(); ++i) {
			if(i->first == ADD_FILE) {
				FileItemTask& task = *static_cast<FileItemTask*>(i->second);
				addFile(task.file, task.entry);
				updateStatus();
				this->setDirty(in_UL ? SettingsManager::BOLD_FINISHED_UPLOADS : SettingsManager::BOLD_FINISHED_DOWNLOADS);
			} else if(i->first == ADD_USER) {
				UserItemTask& task = *static_cast<UserItemTask*>(i->second);
				addUser(task.user, task.entry);
				updateStatus();
				this->setDirty(in_UL ? SettingsManager::BOLD_FINISHED_UPLOADS : SettingsManager::BOLD_FINISHED_DOWNLOADS);
			} else if(i->first == UPDATE_FILE) {
				FileInfo* data = findFileInfo(static_cast<StringTask*>(i->second)->str);
				if(data) {
					data->update();
					files->update(data);
				}
			} else if(i->first == UPDATE_USER) {
				UserInfo* data = findUserInfo(static_cast<UserTask*>(i->second)->user);
				if(data) {
					data->update();
					users->update(data);
				}
			} else if(i->first == REMOVE_FILE) {
				FileInfo* data = findFileInfo(static_cast<StringTask*>(i->second)->str);
				if(data)
					files->erase(data);
				updateStatus();
			} else if(i->first == REMOVE_USER) {
				UserInfo* data = findUserInfo(static_cast<UserTask*>(i->second)->user);
				if(data)
					users->erase(data);
				updateStatus();
			} else if(i->first == REMOVE_ALL) {
				clearTables();
				updateStatus();
			}
		}
		return 0;
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
			for(FinishedFileItem::UserSet::const_iterator i = data->entry->getUsers().begin(); i != data->entry->getUsers().end(); ++i)
				users.push_back(*i);
		}
		void operator()(UserInfo* data) {
			users.push_back(data->user);
		}
		UserList users;
	};

	bool handleFilesContextMenu(dwt::ScreenCoordinate pt) {
		if(files->hasSelected()) {
			if(pt.x() == -1 && pt.y() == -1) {
				pt = files->getContextMenuPos();
			}

			if(BOOLSETTING(SHOW_SHELL_MENU) && files->countSelected() == 1) {
				string path = files->getSelectedData()->file;
				if(File::getSize(path) != -1) {
					CShellContextMenu shellMenu;
					shellMenu.SetPath(Text::utf8ToWide(path));

					dwt::Menu::Seed cs = WinUtil::Seeds::menu;
					cs.ownerDrawn = false;
					dwt::MenuPtr menu = this->addChild(cs);
					menu->appendItem(IDC_VIEW_AS_TEXT, T_("&View as text"), std::tr1::bind(&ThisType::handleViewAsText, this));
					menu->appendItem(IDC_OPEN_FILE, T_("&Open"), std::tr1::bind(&ThisType::handleOpenFile, this));
					menu->appendItem(IDC_OPEN_FOLDER, T_("Open &folder"), std::tr1::bind(&ThisType::handleOpenFolder, this));
					menu->appendSeparatorItem();
					menu->appendItem(IDC_REMOVE, T_("&Remove"), std::tr1::bind(&ThisType::handleRemoveFiles, this));
					menu->appendItem(IDC_REMOVE_ALL, T_("Remove &all"), std::tr1::bind(&ThisType::handleRemoveAll, this));
					menu->appendSeparatorItem();
					WinUtil::addUserItems(menu, files->forEachSelectedT(UserCollector()).users, this->getParent());
					menu->appendSeparatorItem();

					UINT idCommand = shellMenu.ShowContextMenu(menu, pt);
					if(idCommand != 0)
						this->postMessage(WM_COMMAND, idCommand);
					return true;
				}
			}

			dwt::MenuPtr menu = this->addChild(WinUtil::Seeds::menu);
			menu->appendItem(IDC_VIEW_AS_TEXT, T_("&View as text"), std::tr1::bind(&ThisType::handleViewAsText, this));
			menu->appendItem(IDC_OPEN_FILE, T_("&Open"), std::tr1::bind(&ThisType::handleOpenFile, this));
			menu->appendItem(IDC_OPEN_FOLDER, T_("Open &folder"), std::tr1::bind(&ThisType::handleOpenFolder, this));
			menu->appendSeparatorItem();
			menu->appendItem(IDC_REMOVE, T_("&Remove"), std::tr1::bind(&ThisType::handleRemoveFiles, this));
			menu->appendItem(IDC_REMOVE_ALL, T_("Remove &all"), std::tr1::bind(&ThisType::handleRemoveAll, this));
			menu->appendSeparatorItem();
			WinUtil::addUserItems(menu, files->forEachSelectedT(UserCollector()).users, this->getParent());
			menu->setDefaultItem(IDC_OPEN_FILE);

			menu->trackPopupMenu(pt, TPM_LEFTALIGN | TPM_RIGHTBUTTON);
			return true;
		}
		return false;
	}

	bool handleUsersContextMenu(dwt::ScreenCoordinate pt) {
		if(users->hasSelected()) {
			if(pt.x() == -1 && pt.y() == -1) {
				pt = users->getContextMenuPos();
			}

			dwt::MenuPtr menu = this->addChild(WinUtil::Seeds::menu);
			menu->appendItem(IDC_REMOVE, T_("&Remove"), std::tr1::bind(&ThisType::handleRemoveUsers, this));
			menu->appendItem(IDC_REMOVE_ALL, T_("Remove &all"), std::tr1::bind(&ThisType::handleRemoveAll, this));
			menu->appendSeparatorItem();
			WinUtil::addUserItems(menu, users->forEachSelectedT(UserCollector()).users, this->getParent());

			menu->trackPopupMenu(pt, TPM_LEFTALIGN | TPM_RIGHTBUTTON);
			return true;
		}
		return false;
	}

	void handleViewAsText() {
		int i = -1;
		while((i = files->getNext(i, LVNI_SELECTED)) != -1)
			new TextFrame(this->getParent(), files->getData(i)->file);
	}

	void handleOpenFile() {
		files->forEachSelected(&FileInfo::openFile);
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

		this->setStatus(STATUS_COUNT, str(TFN_("%1% item", "%1% items", count) % count));
		this->setStatus(STATUS_BYTES, Text::toT(Util::formatBytes(bytes)));
		this->setStatus(STATUS_SPEED, str(TF_("%1%/s") % Text::toT(Util::formatBytes((time > 0) ? bytes * ((int64_t)1000) / time : 0))));
	}

	void addFile(const string& file, const FinishedFileItemPtr& entry) {
		int loc = files->insert(new FileInfo(file, entry));
		if(files->getVisible())
			files->ensureVisible(loc);
	}

	void addUser(const UserPtr& user, const FinishedUserItemPtr& entry) {
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

	UserInfo* findUserInfo(const UserPtr& user) {
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

	using BaseType::speak;
	void speak(Tasks s) {
		tasks.add(s, 0);
		this->speak();
	}
	template<typename TaskType>
	void speak(Tasks s, TaskType task) {
		tasks.add(s, task);
		this->speak();
	}

	virtual void on(AddedFile, bool upload, const string& file, const FinishedFileItemPtr& entry) throw() {
		if(upload == in_UL)
			speak(ADD_FILE, new FileItemTask(file, entry));
	}

	virtual void on(AddedUser, bool upload, const UserPtr& user, const FinishedUserItemPtr& entry) throw() {
		if(upload == in_UL)
			speak(ADD_USER, new UserItemTask(user, entry));
	}

	virtual void on(UpdatedFile, bool upload, const string& file) throw() {
		if(upload == in_UL)
			speak(UPDATE_FILE, new StringTask(file));
	}

	virtual void on(UpdatedUser, bool upload, const UserPtr& user) throw() {
		if(upload == in_UL)
			speak(UPDATE_USER, new UserTask(user));
	}

	virtual void on(RemovedFile, bool upload, const string& file) throw() {
		if(upload == in_UL)
			speak(REMOVE_FILE, new StringTask(file));
	}

	virtual void on(RemovedUser, bool upload, const UserPtr& user) throw() {
		if(upload == in_UL)
			speak(REMOVE_USER, new UserTask(user));
	}

	virtual void on(RemovedAll, bool upload) throw() {
		if(upload == in_UL)
			speak(REMOVE_ALL);
	}
};

template<class T, bool in_UL>
int FinishedFrameBase<T, in_UL>::filesIndexes[] = {
	FILES_COLUMN_FILE,
	FILES_COLUMN_PATH,
	FILES_COLUMN_NICKS,
	FILES_COLUMN_TRANSFERRED,
	FILES_COLUMN_SPEED,
	FILES_COLUMN_CRC32,
	FILES_COLUMN_TIME
};

template<class T, bool in_UL>
int FinishedFrameBase<T, in_UL>::filesSizes[] = {
	125,
	250,
	250,
	100,
	100,
	80,
	125
};

template<class T, bool in_UL>
const char* FinishedFrameBase<T, in_UL>::filesNames[] = {
	N_("Filename"),
	N_("Path"),
	N_("Nicks"),
	N_("Transferred"),
	N_("Speed"),
	N_("CRC Checked"),
	N_("Time")
};

template<class T, bool in_UL>
int FinishedFrameBase<T, in_UL>::usersIndexes[] = {
	USERS_COLUMN_NICK,
	USERS_COLUMN_HUB,
	USERS_COLUMN_TRANSFERRED,
	USERS_COLUMN_SPEED,
	USERS_COLUMN_FILES,
	USERS_COLUMN_TIME
};

template<class T, bool in_UL>
int FinishedFrameBase<T, in_UL>::usersSizes[] = {
	125,
	125,
	100,
	100,
	300,
	125
};

template<class T, bool in_UL>
const char* FinishedFrameBase<T, in_UL>::usersNames[] = {
	N_("Nick"),
	N_("Hub"),
	N_("Transferred"),
	N_("Speed"),
	N_("Files"),
	N_("Time")
};

#endif // !defined(DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H)
