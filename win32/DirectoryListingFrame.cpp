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

#include "DirectoryListingFrame.h"
#include "LineDlg.h"
#include "TextFrame.h"
#include "HoldRedraw.h"

#include "resource.h"

#include <dcpp/ADLSearch.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/File.h>
#include <dcpp/QueueManager.h>
#include <dcpp/StringSearch.h>
#include <dcpp/ClientManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/WindowInfo.h>

const string DirectoryListingFrame::id = "DirectoryListing";
const string& DirectoryListingFrame::getId() const { return id; }

static const ColumnInfo filesColumns[] = {
	{ N_("File"), 300, false },
	{ N_("Type"), 60, false },
	{ N_("Size"), 80, true },
	{ N_("Exact size"), 100, true },
	{ N_("TTH"), 300, false }
};

DirectoryListingFrame::UserMap DirectoryListingFrame::lists;

int DirectoryListingFrame::ItemInfo::getImage() const {
	if(type == DIRECTORY || type == USER) {
		return dir->getComplete() ? WinUtil::getDirIconIndex() : WinUtil::getDirMaskedIndex();
	}

	return WinUtil::getIconIndex(getText(COLUMN_FILENAME));
}

int DirectoryListingFrame::ItemInfo::compareItems(ItemInfo* a, ItemInfo* b, int col) {
	if(a->type == DIRECTORY) {
		if(b->type == DIRECTORY) {
			switch(col) {
			case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
			case COLUMN_SIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
			default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
			}
		} else {
			return -1;
		}
	} else if(b->type == DIRECTORY) {
		return 1;
	} else {
		switch(col) {
		case COLUMN_EXACTSIZE: return compare(a->file->getSize(), b->file->getSize());
		case COLUMN_SIZE: return compare(a->file->getSize(), b->file->getSize());
		default: return lstrcmp(a->columns[col].c_str(), b->columns[col].c_str());
		}
	}
}

void DirectoryListingFrame::openWindow(dwt::TabView* mdiParent, const tstring& aFile, const tstring& aDir, const UserPtr& aUser, int64_t aSpeed) {
	UserIter i = lists.find(aUser);
	if(i != lists.end()) {
		i->second->speed = aSpeed;
		if(!BOOLSETTING(POPUNDER_FILELIST)) {
			//i->second->activate();
		}
	} else {
		DirectoryListingFrame* frame = new DirectoryListingFrame(mdiParent, aUser, aSpeed);
		frame->loadFile(aFile, aDir);
	}
}

void DirectoryListingFrame::openOwnList(dwt::TabView* parent) {
	string ownListFile = ShareManager::getInstance()->getOwnListFile();
	if(!ownListFile.empty()) {
		openWindow(parent, Text::toT(ownListFile), Util::emptyStringT, ClientManager::getInstance()->getMe(), 0);
	}
}

void DirectoryListingFrame::closeAll(){
	for(UserIter i = lists.begin(); i != lists.end(); ++i)
		::PostMessage(i->second->handle(), WM_CLOSE, 0, 0);
}

const StringMap DirectoryListingFrame::getWindowParams() const {
	if(!path.empty() && dl->getUser() != ClientManager::getInstance()->getMe())
		QueueManager::getInstance()->noDeleteFileList(path);

	return getWindowParams_();
}

const StringMap DirectoryListingFrame::getWindowParams_() const {
	StringMap ret;
	ret[WindowInfo::title] = Text::fromT(getText());
	ret["Path"] = dl->getUser() == ClientManager::getInstance()->getMe() ? "OwnList" : path;
	ret["Speed"] = Util::toString(speed);
	return ret;
}

void DirectoryListingFrame::parseWindowParams(dwt::TabView* parent, const StringMap& params) {
	StringMap::const_iterator path = params.find("Path");
	StringMap::const_iterator speed = params.find("Speed");
	if(path != params.end() && speed != params.end()) {
		if(path->second == "OwnList") {
			openOwnList(parent);
		} else if(File::getSize(path->second) != -1) {
			UserPtr u = DirectoryListing::getUserFromFilename(path->second);
			if(u)
				openWindow(parent, Text::toT(path->second), Util::emptyStringT, u, Util::toInt64(speed->second));
		}
	}
}

void DirectoryListingFrame::openWindow(dwt::TabView* mdiParent, const UserPtr& aUser, const string& txt, int64_t aSpeed) {
	UserIter i = lists.find(aUser);
	if(i != lists.end()) {
		i->second->speed = aSpeed;
		i->second->loadXML(txt);
	} else {
		DirectoryListingFrame* frame = new DirectoryListingFrame(mdiParent, aUser, aSpeed);
		frame->loadXML(txt);
	}
}

DirectoryListingFrame::DirectoryListingFrame(dwt::TabView* mdiParent, const UserPtr& aUser, int64_t aSpeed) :
	BaseType(mdiParent, _T(""), IDH_FILE_LIST, IDR_DIRECTORY, !BOOLSETTING(POPUNDER_FILELIST)),
	dirs(0),
	files(0),
	paned(0),
	find(0),
	findNext(0),
	listDiff(0),
	matchQueue(0),
	speed(aSpeed),
	dl(new DirectoryListing(aUser)),
	usingDirMenu(false),
	historyIndex(0),
	treeRoot(NULL),
	skipHits(0),
	updating(false),
	searching(false)
{
	paned = addChild(WidgetVPaned::Seed(0.3));

	{
		dirs = addChild(WidgetDirs::Seed());
		dirs->setHelpId(IDH_FILE_LIST_DIRS);
		addWidget(dirs);
		paned->setFirst(dirs);
		dirs->setNormalImageList(WinUtil::fileImages);
		dirs->onSelectionChanged(std::tr1::bind(&DirectoryListingFrame::handleSelectionChanged, this));
		dirs->onContextMenu(std::tr1::bind(&DirectoryListingFrame::handleDirsContextMenu, this, _1));
		dirs->onXMouseUp(std::tr1::bind(&DirectoryListingFrame::handleXMouseUp, this, _1));
	}

	{
		files = addChild(WidgetFiles::Seed());
		files->setHelpId(IDH_FILE_LIST_FILES);
		addWidget(files);
		paned->setSecond(files);

		files->setSmallImageList(WinUtil::fileImages);
		WinUtil::makeColumns(files, filesColumns, COLUMN_LAST, SETTING(DIRECTORYLISTINGFRAME_ORDER), SETTING(DIRECTORYLISTINGFRAME_WIDTHS));
		files->setSort(COLUMN_FILENAME);

		files->onSelectionChanged(std::tr1::bind(&DirectoryListingFrame::callAsync, this,
			dwt::Application::Callback(std::tr1::bind(&DirectoryListingFrame::updateStatus, this))));
		files->onDblClicked(std::tr1::bind(&DirectoryListingFrame::handleDoubleClickFiles, this));
		files->onKeyDown(std::tr1::bind(&DirectoryListingFrame::handleKeyDownFiles, this, _1));
		files->onSysKeyDown(std::tr1::bind(&DirectoryListingFrame::handleKeyDownFiles, this, _1));
		files->onContextMenu(std::tr1::bind(&DirectoryListingFrame::handleFilesContextMenu, this, _1));
		files->onXMouseUp(std::tr1::bind(&DirectoryListingFrame::handleXMouseUp, this, _1));
	}

	{
		Button::Seed cs = WinUtil::Seeds::button;

		cs.caption = T_("Subtract list");
		listDiff = addChild(cs);
		listDiff->setHelpId(IDH_FILE_LIST_SUBSTRACT);
		listDiff->onClicked(std::tr1::bind(&DirectoryListingFrame::handleListDiff, this));

		cs.caption = T_("Match queue");
		matchQueue = addChild(cs);
		matchQueue->setHelpId(IDH_FILE_LIST_MATCH_QUEUE);
		matchQueue->onClicked(std::tr1::bind(&DirectoryListingFrame::handleMatchQueue, this));

		cs.caption = T_("Find");
		find = addChild(cs);
		find->setHelpId(IDH_FILE_LIST_FIND);
		find->onClicked(std::tr1::bind(&DirectoryListingFrame::handleFind, this));

		cs.caption = T_("Next");
		findNext = addChild(cs);
		findNext->setHelpId(IDH_FILE_LIST_NEXT);
		findNext->onClicked(std::tr1::bind(&DirectoryListingFrame::handleFindNext, this));
	}

	initStatus();

	// This will set the widths correctly
	status->setText(STATUS_FILE_LIST_DIFF, T_("Subtract list"));
	status->setText(STATUS_MATCH_QUEUE, T_("Match queue"));
	status->setText(STATUS_FIND, T_("Find"));
	status->setText(STATUS_NEXT, T_("Next"));

	string nick = ClientManager::getInstance()->getNicks(dl->getUser()->getCID())[0];
	treeRoot = dirs->insert(NULL, new ItemInfo(Text::toT(nick), dl->getRoot()));

	ClientManager::getInstance()->addListener(this);
	updateTitle();

	layout();

	lists.insert(std::make_pair(aUser, this));
}

DirectoryListingFrame::~DirectoryListingFrame() {
	dcassert(lists.find(dl->getUser()) != lists.end());
	lists.erase(dl->getUser());
}

void DirectoryListingFrame::loadFile(const tstring& name, const tstring& dir) {
	try {
		dl->loadFile(Text::fromT(name));
		path = Text::fromT(name);
		WindowManager::getInstance()->addRecent(id, getWindowParams_());
		ADLSearchManager::getInstance()->matchListing(*dl);
		refreshTree(dir);
	} catch(const Exception& e) {
		error = Text::toT(": " + e.getError());
	}

	initStatusText();
}

void DirectoryListingFrame::loadXML(const string& txt) {
	try {
		refreshTree(Text::toT(Util::toNmdcFile(dl->loadXML(txt, true))));
	} catch(const Exception& e) {
		error = Text::toT(": " + e.getError());
	}

	initStatusText();
}

void DirectoryListingFrame::layout() {
	dwt::Rectangle r(getClientAreaSize());

	status->layout(r);

	status->mapWidget(STATUS_FILE_LIST_DIFF, listDiff);
	status->mapWidget(STATUS_MATCH_QUEUE, matchQueue);
	status->mapWidget(STATUS_FIND, find);
	status->mapWidget(STATUS_NEXT, findNext);

	paned->setRect(r);
}

bool DirectoryListingFrame::preClosing() {
	ClientManager::getInstance()->removeListener(this);
	return true;
}

void DirectoryListingFrame::postClosing() {
	clearList();

	SettingsManager::getInstance()->set(SettingsManager::DIRECTORYLISTINGFRAME_ORDER, WinUtil::toString(files->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::DIRECTORYLISTINGFRAME_WIDTHS, WinUtil::toString(files->getColumnWidths()));
}

void DirectoryListingFrame::handleFind() {
	searching = true;
	findFile(false);
	searching = false;
	updateStatus();
}

void DirectoryListingFrame::handleFindNext() {
	searching = true;
	findFile(true);
	searching = false;
	updateStatus();
}

void DirectoryListingFrame::handleMatchQueue() {
	// TODO provide hubHint?
	int matched = QueueManager::getInstance()->matchListing(*dl, Util::emptyString);
	status->setText(STATUS_STATUS, str(TFN_("Matched %1% file", "Matched %1% files", matched) % matched));
}

void DirectoryListingFrame::handleListDiff() {
	tstring file;
	if(WinUtil::browseFileList(createLoadDialog(), file)) {
		DirectoryListing dirList(dl->getUser());
		try {
			dirList.loadFile(Text::fromT(file));
			dl->getRoot()->filterList(dirList);
			refreshTree(Util::emptyStringT);
			initStatusText();
			updateStatus();
		} catch(const Exception&) {
			/// @todo report to user?
		}
	}
}

void DirectoryListingFrame::refreshTree(const tstring& root) {
	HoldRedraw hold(dirs);
	HTREEITEM ht = findItem(treeRoot, root);
	if(ht == NULL) {
		ht = treeRoot;
	}

	DirectoryListing::Directory* d = dirs->getData(ht)->dir;

	HTREEITEM next = NULL;
	while((next = dirs->getChild(ht)) != NULL) {
		dirs->erase(next);
	}
	updateTree(d, ht);

	dirs->setSelected(NULL);
	selectItem(root);

	dcdebug("setSelecteded");
	dirs->expand(treeRoot);
}

void DirectoryListingFrame::updateTitle() {
	tstring text = WinUtil::getNicks(dl->getUser());
	if(error.empty())
		text += _T(" - ") + WinUtil::getHubNames(dl->getUser()).first;
	else
		text += _T(": ") + error;
	setText(text);
}

DirectoryListingFrame::ShellMenuPtr DirectoryListingFrame::makeSingleMenu(ItemInfo* ii) {
	ShellMenuPtr menu = addChild(ShellMenu::Seed());

	menu->appendItem(T_("&Download"), std::tr1::bind(&DirectoryListingFrame::handleDownload, this), dwt::IconPtr(), true, true);
	addTargets(menu, ii);

	if(ii->type == ItemInfo::FILE) {
		menu->appendItem(T_("&View as text"), std::tr1::bind(&DirectoryListingFrame::handleViewAsText, this));

		menu->appendSeparator();

		WinUtil::addHashItems(menu, ii->file->getTTH(), Text::toT(ii->file->getName()));
	}

	if((ii->type == ItemInfo::FILE && ii->file->getAdls()) ||
		(ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()) )	{
		menu->appendSeparator();
		menu->appendItem(T_("&Go to directory"), std::tr1::bind(&DirectoryListingFrame::handleGoToDirectory, this));
	}

	addUserCommands(menu);
	return menu;
}

DirectoryListingFrame::ShellMenuPtr DirectoryListingFrame::makeMultiMenu() {
	ShellMenuPtr menu = addChild(ShellMenu::Seed());

	menu->appendItem(T_("&Download"), std::tr1::bind(&DirectoryListingFrame::handleDownload, this), dwt::IconPtr(), true, true);
	addTargets(menu);
	addUserCommands(menu);

	return menu;
}

MenuPtr DirectoryListingFrame::makeDirMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	menu->appendItem(T_("&Download"), std::tr1::bind(&DirectoryListingFrame::handleDownload, this));
	addTargets(menu);
	return menu;
}

void DirectoryListingFrame::addUserCommands(const MenuPtr& parent) {
	prepareMenu(parent, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubs(dl->getUser()->getCID()));
}

void DirectoryListingFrame::addTargets(const MenuPtr& parent, ItemInfo* ii) {
	MenuPtr menu = parent->appendPopup(T_("Download &to..."));
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	size_t i = 0;
	for(; i < spl.size(); ++i) {
		menu->appendItem(Text::toT(spl[i].second), std::tr1::bind(&DirectoryListingFrame::handleDownloadFavorite, this, i));
	}

	if(i > 0) {
		menu->appendSeparator();
	}

	menu->appendItem(T_("&Browse..."), std::tr1::bind(&DirectoryListingFrame::handleDownloadBrowse, this));

	targets.clear();

	if(ii && ii->type == ItemInfo::FILE) {
		QueueManager::getInstance()->getTargets(ii->file->getTTH(), targets);
		if(!targets.empty()) {
			menu->appendSeparator();
			for(i = 0; i < targets.size(); ++i) {
				menu->appendItem(Text::toT(targets[i]), std::tr1::bind(&DirectoryListingFrame::handleDownloadTarget, this, i));
			}
		}
	}

	if(WinUtil::lastDirs.size() > 0) {
		menu->appendSeparator();

		for(i = 0; i < WinUtil::lastDirs.size(); ++i) {
			menu->appendItem(WinUtil::lastDirs[i], std::tr1::bind(&DirectoryListingFrame::handleDownloadLastDir, this, i));
		}
	}
}

bool DirectoryListingFrame::handleFilesContextMenu(dwt::ScreenCoordinate pt) {
	std::vector<unsigned> selected = files->getSelection();
	if(!selected.empty()) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = files->getContextMenuPos();
		}

		ShellMenuPtr menu;

		if(selected.size() == 1) {
			menu = makeSingleMenu(files->getData(selected[0]));
		} else {
			menu = makeMultiMenu();
		}

		StringList ShellMenuPaths;
		for(std::vector<unsigned>::iterator i = selected.begin(); i != selected.end(); ++i) {
			ItemInfo* ii = files->getData(*i);
			if(ii->type == ItemInfo::FILE) {
				string localPath = dl->getLocalPath(ii->file);
				if(!localPath.empty())
					ShellMenuPaths.push_back(localPath);
			}
		}
		menu->appendShellMenu(ShellMenuPaths);

		usingDirMenu = false;
		menu->open(pt);
		return true;
	}
	return false;
}

bool DirectoryListingFrame::handleDirsContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = dirs->getContextMenuPos();
	} else {
		dirs->select(pt);
	}

	if(dirs->getSelected()) {
		MenuPtr contextMenu = makeDirMenu();
		usingDirMenu = true;
		contextMenu->open(pt);

		return true;
	}
	return false;
}

void DirectoryListingFrame::downloadFiles(const string& aTarget, bool view /* = false */) {
	int i=-1;
	while((i = files->getNext(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = files->getData(i);
		if(view) {
			string localPath = dl->getLocalPath(ii->file);
			if(!localPath.empty()) {
				TextFrame::openWindow(this->getParent(), localPath);
				continue;
			}
		}
		download(ii, aTarget, view);
	}
}

void DirectoryListingFrame::download(ItemInfo* ii, const string& dir, bool view) {
	try {
		if(ii->type == ItemInfo::FILE) {
			if(view) {
				File::deleteFile(dir + Util::validateFileName(ii->file->getName()));
			}
			dl->download(ii->file, dir + Text::fromT(ii->getText(COLUMN_FILENAME)), view, WinUtil::isShift() || view);
		} else if(!view) {
			dl->download(ii->dir, dir, WinUtil::isShift());
		}

	} catch(const Exception& e) {
		status->setText(STATUS_STATUS, Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::handleDownload() {
	if(usingDirMenu) {
		ItemInfo* ii = dirs->getSelectedData();
		if(ii) {
			download(ii, SETTING(DOWNLOAD_DIRECTORY));
		}
	} else {
		downloadFiles(SETTING(DOWNLOAD_DIRECTORY));
	}
}

void DirectoryListingFrame::handleDownloadBrowse() {
	if(usingDirMenu) {
		ItemInfo* ii = dirs->getSelectedData();
		if(ii) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(createFolderDialog().open(target)) {
				WinUtil::addLastDir(target);
				download(ii, Text::fromT(target));
			}
		}
	} else {
		if(files->countSelected() == 1) {
			ItemInfo* ii = files->getSelectedData();
			try {
				if(ii->type == ItemInfo::FILE) {
					tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + ii->getText(COLUMN_FILENAME);
					if(createSaveDialog().open(target)) {
						WinUtil::addLastDir(Util::getFilePath(target));
						dl->download(ii->file, Text::fromT(target), false, WinUtil::isShift());
					}
				} else {
					tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
					if(createFolderDialog().open(target)) {
						WinUtil::addLastDir(target);
						dl->download(ii->dir, Text::fromT(target), WinUtil::isShift());
					}
				}
			} catch(const Exception& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(createFolderDialog().open(target)) {
				WinUtil::addLastDir(target);
				downloadFiles(Text::fromT(target));
			}
		}
	}
}

void DirectoryListingFrame::handleDownloadLastDir(unsigned index) {
	if(index >= WinUtil::lastDirs.size()) {
		return;
	}
	download(Text::fromT(WinUtil::lastDirs[index]));
}

void DirectoryListingFrame::handleDownloadFavorite(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if(index >= spl.size()) {
		return;
	}
	download(spl[index].first);
}

void DirectoryListingFrame::handleDownloadTarget(unsigned index) {
	if(index >= targets.size()) {
		return;
	}

	if(files->countSelected() != 1) {
		return;
	}

	const string& target = targets[index];
	ItemInfo* ii = files->getSelectedData();
	try {
		dl->download(ii->file, target, false, WinUtil::isShift());
	} catch(const Exception& e) {
		status->setText(STATUS_STATUS, Text::toT(e.getError()));
	}
}


void DirectoryListingFrame::handleGoToDirectory() {
	if(files->countSelected() != 1)
		return;

	tstring fullPath;
	ItemInfo* ii = files->getSelectedData();

	if(ii->type == ItemInfo::FILE) {
		if(!ii->file->getAdls())
			return;

		DirectoryListing::Directory* pd = ii->file->getParent();
		while(pd != NULL && pd != dl->getRoot()) {
			fullPath = Text::toT(pd->getName()) + _T("\\") + fullPath;
			pd = pd->getParent();
		}
	} else if(ii->type == ItemInfo::DIRECTORY) {
		if(!(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()))
			return;
		fullPath = Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath());
	}

	selectItem(fullPath);
}

void DirectoryListingFrame::download(const string& target) {
	if(usingDirMenu) {
		ItemInfo* ii = dirs->getSelectedData();
		if(ii) {
			download(ii, target);
		}
	} else {
		if(files->countSelected() == 1) {
			ItemInfo* ii = files->getSelectedData();
			download(ii, target);
		} else {
			downloadFiles(target);
		}
	}
}

void DirectoryListingFrame::handleViewAsText() {
	downloadFiles(Util::getTempPath(), true);
}

HTREEITEM DirectoryListingFrame::findItem(HTREEITEM ht, const tstring& name) {
	string::size_type i = name.find(_T('\\'));
	if(i == string::npos)
		return ht;

	for(HTREEITEM child = dirs->getChild(ht); child != NULL; child = dirs->getNextSibling(child)) {
		DirectoryListing::Directory* d = dirs->getData(child)->dir;
		if(Text::toT(d->getName()) == name.substr(0, i)) {
			return findItem(child, name.substr(i+1));
		}
	}
	return NULL;
}

void DirectoryListingFrame::selectItem(const tstring& name) {
	HTREEITEM ht = findItem(treeRoot, name);
	if(ht != NULL) {
		dirs->ensureVisible(ht);
		dirs->setSelected(ht);
	}
}

void DirectoryListingFrame::updateTree(DirectoryListing::Directory* aTree, HTREEITEM aParent) {
	for(DirectoryListing::Directory::Iter i = aTree->directories.begin(); i != aTree->directories.end(); ++i) {
		HTREEITEM ht = dirs->insert(aParent, new ItemInfo(*i));
		if((*i)->getAdls())
			dirs->setItemState(ht, TVIS_BOLD, TVIS_BOLD);
		updateTree(*i, ht);
	}
}

void DirectoryListingFrame::initStatusText() {
	status->setText(STATUS_TOTAL_FILES, str(TF_("Files: %1%") % dl->getTotalFileCount(true)));
	status->setText(STATUS_TOTAL_SIZE, str(TF_("Size: %1%") % Text::toT(Util::formatBytes(dl->getTotalSize(true)))));
	status->setText(STATUS_SPEED, str(TF_("Speed: %1%/s") % Text::toT(Util::formatBytes(speed))));
}

void DirectoryListingFrame::updateStatus() {
	if(!searching && !updating) {
		int cnt = files->countSelected();
		int64_t total = 0;
		if(cnt == 0) {
			cnt = files->size();
			total = files->forEachT(ItemInfo::TotalSize()).total;
		} else {
			total = files->forEachSelectedT(ItemInfo::TotalSize()).total;
		}

		status->setText(STATUS_SELECTED_FILES, str(TF_("Files: %1%") % cnt));

		status->setText(STATUS_SELECTED_SIZE, str(TF_("Size: %1%") % Text::toT(Util::formatBytes(total))));
	}
}

void DirectoryListingFrame::handleSelectionChanged() {
	ItemInfo* ii = dirs->getSelectedData();
	if(!ii) {
		return;
	}

	DirectoryListing::Directory* d = ii->dir;
	if(d == 0) {
		return;
	}
	HoldRedraw hold(files);
	changeDir(d);
	addHistory(dl->getPath(d));
}

void DirectoryListingFrame::changeDir(DirectoryListing::Directory* d) {

	updating = true;
	clearList();

	for(DirectoryListing::Directory::Iter i = d->directories.begin(); i != d->directories.end(); ++i) {
		files->insert(files->size(), new ItemInfo(*i));
	}
	for(DirectoryListing::File::Iter j = d->files.begin(); j != d->files.end(); ++j) {
		ItemInfo* ii = new ItemInfo(*j);
		files->insert(files->size(), ii);
	}
	files->resort();

	updating = false;
	updateStatus();

	if(!d->getComplete()) {
		dcdebug("Directory incomplete\n");
		if(dl->getUser()->isOnline()) {
			try {
				// TODO provide hubHint?
				QueueManager::getInstance()->addList(dl->getUser(), Util::emptyString, QueueItem::FLAG_PARTIAL_LIST, dl->getPath(d));
				status->setText(STATUS_STATUS, T_("Downloading list..."));
			} catch(const QueueException& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		} else {
			status->setText(STATUS_STATUS, T_("User offline"));
		}
	}
}

void DirectoryListingFrame::clearList() {
	files->clear();
}

void DirectoryListingFrame::addHistory(const string& name) {
	history.erase(history.begin() + historyIndex, history.end());
	while(history.size() > 25)
		history.pop_front();
	history.push_back(name);
	historyIndex = history.size();
}

void DirectoryListingFrame::up() {
	HTREEITEM t = dirs->getSelected();
	if(t == NULL)
		return;
	t = dirs->getParent(t);
	if(t == NULL)
		return;
	dirs->setSelected(t);
}

void DirectoryListingFrame::back() {
	if(history.size() > 1 && historyIndex > 1) {
		size_t n = min(historyIndex, history.size()) - 1;
		deque<string> tmp = history;
		selectItem(Text::toT(history[n - 1]));
		historyIndex = n;
		history = tmp;
	}
}

void DirectoryListingFrame::forward() {
	if(history.size() > 1 && historyIndex < history.size()) {
		size_t n = min(historyIndex, history.size() - 1);
		deque<string> tmp = history;
		selectItem(Text::toT(history[n]));
		historyIndex = n + 1;
		history = tmp;
	}
}

HTREEITEM DirectoryListingFrame::findFile(const StringSearch& str, HTREEITEM root,
										  int &foundFile, int &skipHits)
{
	// Check dir name for match
	DirectoryListing::Directory* dir = dirs->getData(root)->dir;
	if(str.match(dir->getName())) {
		if(skipHits == 0) {
			foundFile = -1;
			return root;
		} else {
			skipHits--;
		}
	}

	// Force list pane to contain files of current dir
	changeDir(dir);

	// Check file names in list pane
	for(size_t i = 0; i < files->size(); i++) {
		ItemInfo* ii = files->getData(i);
		if(ii->type == ItemInfo::FILE) {
			if(str.match(ii->file->getName())) {
				if(skipHits == 0) {
					foundFile = i;
					return root;
				} else {
					skipHits--;
				}
			}
		}
	}

	dcdebug("looking for directories...\n");
	// Check subdirs recursively
	HTREEITEM item = dirs->getChild(root);
	while(item != NULL) {
		HTREEITEM srch = findFile(str, item, foundFile, skipHits);
		if(srch)
			return srch;
		else
			item = dirs->getNextSibling(item);
	}

	return 0;
}

void DirectoryListingFrame::findFile(bool findNext)
{
	if(!findNext) {
		// Prompt for substring to find
		LineDlg dlg(this, T_("Search for file"), T_("Enter search string"));

		if(dlg.run() != IDOK)
			return;

		findStr = Text::fromT(dlg.getLine());
		skipHits = 0;
	} else {
		skipHits++;
	}

	if(findStr.empty())
		return;

	HoldRedraw hold(files);
	HoldRedraw hold2(dirs);
	HoldRedraw hold3(status);

	// Do a search
	int foundFile = -1, skipHitsTmp = skipHits;
	HTREEITEM const oldDir = dirs->getSelected();
	HTREEITEM const foundDir = findFile(StringSearch(findStr), treeRoot, foundFile, skipHitsTmp);

	if(foundDir) {
		// Highlight the directory tree and list if the parent dir/a matched dir was found
		if(foundFile >= 0) {
			// SelectItem won't update the list if SetRedraw was set to FALSE and then
			// to TRUE and the item setSelecteded is the same as the last one... workaround:
			if(oldDir == foundDir)
				dirs->setSelected(NULL);

			dirs->setSelected(foundDir);
		} else {
			// Got a dir; setSelected its parent directory in the tree if there is one
			HTREEITEM parentItem = dirs->getParent(foundDir);
			if(parentItem) {
				// Go to parent file list
				dirs->setSelected(parentItem);

				// Locate the dir in the file list
				DirectoryListing::Directory* dir = dirs->getData(foundDir)->dir;

				foundFile = files->find(Text::toT(dir->getName()), -1, false);
			} else {
				// If no parent exists, just the dir tree item and skip the list highlighting
				dirs->setSelected(foundDir);
			}
		}

		// Remove prev. setSelectedion from file list
		if(files->hasSelected()) {
			files->clearSelection();
		}

		// Highlight and focus the dir/file if possible
		if(foundFile >= 0) {
			files->setFocus();
			files->setSelected(foundFile);
			files->ensureVisible(foundFile);
		} else {
			dirs->setFocus();
		}
	} else {
		dirs->setSelected(NULL);
		dirs->setSelected(oldDir);
		createMessageBox().show(T_("No matches"), T_("Search for file"));
	}
}

void DirectoryListingFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	set<UserPtr> users;

	int sel = -1;
	while((sel = files->getNext(sel, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = files->getData(sel);
		if(uc.getType() == UserCommand::TYPE_RAW_ONCE) {
			if(users.find(dl->getUser()) != users.end())
				continue;
			users.insert(dl->getUser());
		}
		if(!dl->getUser()->isOnline())
			return;
		ucParams["fileTR"] = "NONE";
		if(ii->type == ItemInfo::FILE) {
			ucParams["type"] = "File";
			ucParams["fileFN"] = dl->getPath(ii->file) + ii->file->getName();
			ucParams["fileSI"] = Util::toString(ii->file->getSize());
			ucParams["fileSIshort"] = Util::formatBytes(ii->file->getSize());
			ucParams["fileTR"] = ii->file->getTTH().toBase32();
		} else {
			ucParams["type"] = "Directory";
			ucParams["fileFN"] = dl->getPath(ii->dir) + ii->dir->getName();
			ucParams["fileSI"] = Util::toString(ii->dir->getTotalSize());
			ucParams["fileSIshort"] = Util::formatBytes(ii->dir->getTotalSize());
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		StringMap tmp = ucParams;
		ClientManager::getInstance()->userCommand(dl->getUser(), uc, tmp, true);
	}
}

void DirectoryListingFrame::handleDoubleClickFiles() {

	HTREEITEM t = dirs->getSelected();
	int i = files->getSelected();
	if(t != NULL && i != -1) {
		ItemInfo* ii = files->getData(i);

		if(ii->type == ItemInfo::FILE) {
			try {
				dl->download(ii->file, SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(ii->getText(COLUMN_FILENAME)), false, WinUtil::isShift());
			} catch(const Exception& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		} else {
			HTREEITEM ht = dirs->getChild(t);
			while(ht != NULL) {
				if(dirs->getData(ht)->dir == ii->dir) {
					dirs->setSelected(ht);
					break;
				}
				ht = dirs->getNextSibling(ht);
			}
		}
	}
}

bool DirectoryListingFrame::handleXMouseUp(const dwt::MouseEvent& mouseEvent) {
	switch(mouseEvent.ButtonPressed) {
	case dwt::MouseEvent::X1: back(); break;
	case dwt::MouseEvent::X2: forward(); break;
	}
	return true;
}

bool DirectoryListingFrame::handleKeyDownFiles(int c) {
	if(c == VK_BACK) {
		up();
		return true;
	}
	if(c == VK_LEFT && WinUtil::isAlt()) {
		back();
		return true;
	}
	if(c == VK_RIGHT && WinUtil::isAlt()) {
		forward();
		return true;
	}
	if(c == VK_RETURN) {
		if(files->countSelected() == 1) {
			ItemInfo* ii = files->getSelectedData();
			if(ii->type == ItemInfo::DIRECTORY) {
				HTREEITEM ht = dirs->getChild(dirs->getSelected());
				while(ht != NULL) {
					if(dirs->getData(ht)->dir == ii->dir) {
						dirs->setSelected(ht);
						break;
					}
					ht = dirs->getNextSibling(ht);
				}
			} else {
				downloadFiles(SETTING(DOWNLOAD_DIRECTORY));
			}
		} else {
			downloadFiles(SETTING(DOWNLOAD_DIRECTORY));
		}
		return true;
	}
	return false;
}

void DirectoryListingFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) throw() {
	if(aUser.getUser() == dl->getUser())
		callAsync(std::tr1::bind(&DirectoryListingFrame::updateTitle, this));
}
void DirectoryListingFrame::on(ClientManagerListener::UserConnected, const UserPtr& aUser) throw() {
	if(aUser == dl->getUser())
		callAsync(std::tr1::bind(&DirectoryListingFrame::updateTitle, this));
}
void DirectoryListingFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
	if(aUser == dl->getUser())
		callAsync(std::tr1::bind(&DirectoryListingFrame::updateTitle, this));
}
