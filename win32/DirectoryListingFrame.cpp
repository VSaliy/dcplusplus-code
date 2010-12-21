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

#include "stdafx.h"

#include "DirectoryListingFrame.h"

#include "MainWindow.h"
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

TStringList DirectoryListingFrame::lastSearches;

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

void DirectoryListingFrame::openWindow(dwt::TabView* mdiParent, const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed) {
	UserIter prev = lists.find(aUser);
	if(prev == lists.end()) {
		openWindow_(false, mdiParent, aFile, aDir, aUser, aSpeed);
	} else {
		bool wasActive = prev->second->isActive();
		prev->second->close();
		mdiParent->callAsync(std::bind(&DirectoryListingFrame::openWindow_, wasActive, mdiParent, aFile, aDir, aUser, aSpeed));
	}
}

void DirectoryListingFrame::openWindow_(bool activate, dwt::TabView* mdiParent, const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed) {
	DirectoryListingFrame* frame = new DirectoryListingFrame(mdiParent, aUser, aSpeed);
	frame->loadFile(aFile, aDir);

	if(!activate && BOOLSETTING(POPUNDER_FILELIST))
		frame->setDirty(SettingsManager::BOLD_FL);
	else
		frame->activate();
}

void DirectoryListingFrame::openOwnList(dwt::TabView* parent) {
	string ownListFile = ShareManager::getInstance()->getOwnListFile();
	if(!ownListFile.empty()) {
		openWindow(parent, Text::toT(ownListFile), Util::emptyStringT, HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString), 0);
	}
}

void DirectoryListingFrame::closeAll(){
	for(UserIter i = lists.begin(); i != lists.end(); ++i)
		i->second->close(true);
}

const StringMap DirectoryListingFrame::getWindowParams() const {
	StringMap ret;
	ret[WindowInfo::title] = Text::fromT(getText());
	ret[WindowInfo::cid] = dl->getUser().user->getCID().toBase32();
	ret[WindowInfo::fileList] = (dl->getUser() == ClientManager::getInstance()->getMe()) ? "" : path;
	ret["Hub"] = dl->getUser().hint;
	ret["Speed"] = Util::toString(speed);
	return ret;
}

void DirectoryListingFrame::parseWindowParams(dwt::TabView* parent, const StringMap& params) {
	StringMap::const_iterator path = params.find(WindowInfo::fileList);
	StringMap::const_iterator hub = params.find("Hub");
	StringMap::const_iterator speed = params.find("Speed");
	if(path != params.end() && speed != params.end()) {
		if(path->second.empty()) {
			openOwnList(parent);
		} else if(File::getSize(path->second) != -1) {
			UserPtr u = DirectoryListing::getUserFromFilename(path->second);
			if(u) {
				openWindow(parent, Text::toT(path->second), Util::emptyStringT, HintedUser(u,
					(hub == params.end()) ? Util::emptyString : hub->second), Util::toInt64(speed->second));
			}
		}
	}
}

bool DirectoryListingFrame::isFavorite(const StringMap& params) {
	StringMap::const_iterator path = params.find(WindowInfo::fileList);
	if(path != params.end() && path->second != "OwnList" && File::getSize(path->second) != -1) {
		UserPtr u = DirectoryListing::getUserFromFilename(path->second);
		if(u)
			return FavoriteManager::getInstance()->isFavoriteUser(u);
	}
	return false;
}

void DirectoryListingFrame::openWindow(dwt::TabView* mdiParent, const HintedUser& aUser, const string& txt, int64_t aSpeed) {
	UserIter i = lists.find(aUser);
	if(i != lists.end()) {
		i->second->speed = aSpeed;
		i->second->loadXML(txt);
		i->second->setDirty(SettingsManager::POPUNDER_FILELIST); /// @todo add a setting
	} else {
		DirectoryListingFrame* frame = new DirectoryListingFrame(mdiParent, aUser, aSpeed);
		frame->loadXML(txt);
		if(BOOLSETTING(POPUNDER_FILELIST))
			frame->setDirty(SettingsManager::POPUNDER_FILELIST); /// @todo add a setting
		else
			frame->activate();
	}
}

DirectoryListingFrame::DirectoryListingFrame(dwt::TabView* mdiParent, const HintedUser& aUser, int64_t aSpeed) :
	BaseType(mdiParent, _T(""), IDH_FILE_LIST, IDI_DIRECTORY),
	grid(0),
	dirs(0),
	files(0),
	searchGrid(0),
	searchBox(0),
	listDiff(0),
	matchQueue(0),
	find(0),
	speed(aSpeed),
	dl(new DirectoryListing(aUser)),
	user(aUser),
	usingDirMenu(false),
	historyIndex(0),
	treeRoot(NULL),
	updating(false),
	searching(false)
{
	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->row(1).size = 0;
	grid->row(1).mode = GridInfo::STATIC;

	VSplitterPtr paned = grid->addChild(VSplitter::Seed(0.3));

	{
		dirs = grid->addChild(WidgetDirs::Seed());
		dirs->setHelpId(IDH_FILE_LIST_DIRS);
		addWidget(dirs);
		paned->setFirst(dirs);
		dirs->setNormalImageList(WinUtil::fileImages);
		dirs->onSelectionChanged(std::bind(&DirectoryListingFrame::handleSelectionChanged, this));
		dirs->onContextMenu(std::bind(&DirectoryListingFrame::handleDirsContextMenu, this, _1));
		dirs->onXMouseUp(std::bind(&DirectoryListingFrame::handleXMouseUp, this, _1));
	}

	{
		files = grid->addChild(WidgetFiles::Seed());
		files->setHelpId(IDH_FILE_LIST_FILES);
		addWidget(files);
		paned->setSecond(files);

		files->setSmallImageList(WinUtil::fileImages);
		WinUtil::makeColumns(files, filesColumns, COLUMN_LAST, SETTING(DIRECTORYLISTINGFRAME_ORDER), SETTING(DIRECTORYLISTINGFRAME_WIDTHS));
		files->setSort(COLUMN_FILENAME);

		files->onSelectionChanged(std::bind(&DirectoryListingFrame::callAsync, this,
			dwt::Application::Callback(std::bind(&DirectoryListingFrame::updateStatus, this))));
		files->onDblClicked(std::bind(&DirectoryListingFrame::handleDoubleClickFiles, this));
		files->onKeyDown(std::bind(&DirectoryListingFrame::handleKeyDownFiles, this, _1));
		files->onSysKeyDown(std::bind(&DirectoryListingFrame::handleKeyDownFiles, this, _1));
		files->onContextMenu(std::bind(&DirectoryListingFrame::handleFilesContextMenu, this, _1));
		files->onXMouseUp(std::bind(&DirectoryListingFrame::handleXMouseUp, this, _1));
	}

	{
		Button::Seed cs = WinUtil::Seeds::button;

		searchGrid = grid->addChild(Grid::Seed(1, 4));
		grid->setWidget(searchGrid, 1, 0);
		searchGrid->column(0).mode = GridInfo::FILL;

		searchBox = searchGrid->addChild(WinUtil::Seeds::comboBoxEdit);
		searchBox->setHelpId(IDH_FILE_LIST_SEARCH_BOX);
		addWidget(searchBox);

		cs.caption = T_("Find from the beginning");
		ButtonPtr button = searchGrid->addChild(cs);
		button->setHelpId(IDH_FILE_LIST_FIND_START);
		button->setImage(WinUtil::buttonIcon(IDI_SEARCH));
		button->onClicked(std::bind(&DirectoryListingFrame::handleFind, this, FIND_START));
		addWidget(button);

		cs.caption = T_("Previous");
		button = searchGrid->addChild(cs);
		button->setHelpId(IDH_FILE_LIST_FIND_PREV);
		button->setImage(WinUtil::buttonIcon(IDI_LEFT));
		button->onClicked(std::bind(&DirectoryListingFrame::handleFind, this, FIND_PREV));
		addWidget(button);

		cs.caption = T_("Next");
		button = searchGrid->addChild(cs);
		button->setHelpId(IDH_FILE_LIST_FIND_NEXT);
		button->setImage(WinUtil::buttonIcon(IDI_RIGHT));
		button->onClicked(std::bind(&DirectoryListingFrame::handleFind, this, FIND_NEXT));
		addWidget(button);

		cs.caption = T_("Subtract list");
		listDiff = addChild(cs);
		listDiff->setHelpId(IDH_FILE_LIST_SUBSTRACT);
		listDiff->onClicked(std::bind(&DirectoryListingFrame::handleListDiff, this));

		cs.caption = T_("Match queue");
		matchQueue = addChild(cs);
		matchQueue->setHelpId(IDH_FILE_LIST_MATCH_QUEUE);
		matchQueue->onClicked(std::bind(&DirectoryListingFrame::handleMatchQueue, this));

		cs.caption = T_("Find") + _T(" \u25B2") /* up arrow */;
		find = addChild(cs);
		find->setHelpId(IDH_FILE_LIST_FIND);
		find->setImage(WinUtil::buttonIcon(IDI_SEARCH));
		find->onClicked(std::bind(&DirectoryListingFrame::handleFindToggle, this));
	}

	searchGrid->setEnabled(false);

	initStatus();

	status->setSize(STATUS_FILE_LIST_DIFF, listDiff->getPreferredSize().x);
	status->setSize(STATUS_MATCH_QUEUE, matchQueue->getPreferredSize().x);
	status->setSize(STATUS_FIND, find->getPreferredSize().x);

	treeRoot = dirs->insert(NULL, new ItemInfo(true, dl->getRoot()));

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
		addRecent();
		ADLSearchManager::getInstance()->matchListing(*dl);
		refreshTree(dir);
	} catch(const Exception& e) {
		error = Text::toT(": " + e.getError());
		updateTitle();
	}

	initStatusText();
}

void DirectoryListingFrame::loadXML(const string& txt) {
	try {
		refreshTree(Text::toT(Util::toNmdcFile(dl->updateXML(txt))));
	} catch(const Exception& e) {
		error = Text::toT(": " + e.getError());
	}

	initStatusText();
}

void DirectoryListingFrame::layout() {
	dwt::Rectangle r(getClientSize());

	status->layout(r);

	status->mapWidget(STATUS_FILE_LIST_DIFF, listDiff);
	status->mapWidget(STATUS_MATCH_QUEUE, matchQueue);
	status->mapWidget(STATUS_FIND, find);

	grid->layout(r);
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

void DirectoryListingFrame::handleFind(FindMode mode) {
	searching = true;
	findFile(mode);
	searching = false;
	updateStatus();
}

void DirectoryListingFrame::handleMatchQueue() {
	int matched = QueueManager::getInstance()->matchListing(*dl);
	status->setText(STATUS_STATUS, str(TFN_("Matched %1% file", "Matched %1% files", matched) % matched));
}

void DirectoryListingFrame::handleListDiff() {
	tstring file;
	if(WinUtil::browseFileList(this, file)) {
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

void DirectoryListingFrame::handleFindToggle() {
	if(searchGrid->getEnabled()) {
		searchBox->clear();
		searchGrid->setEnabled(false);
		grid->row(1).mode = GridInfo::STATIC;
	} else {
		for(auto i = lastSearches.crbegin(), iend = lastSearches.crend(); i != iend; ++i)
			searchBox->addValue(*i);
		searchGrid->setEnabled(true);
		grid->row(1).mode = GridInfo::AUTO;
		searchBox->setFocus();
	}
	layout();
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

	dirs->expand(treeRoot);
}

void DirectoryListingFrame::updateTitle() {
	tstring text = WinUtil::getNicks(dl->getUser());
	if(error.empty())
		text += _T(" - ") + WinUtil::getHubNames(dl->getUser()).first;
	else
		text += _T(": ") + error;
	setText(text);

	dirs->getData(treeRoot)->setText(text);
	dirs->redraw();
}

ShellMenuPtr DirectoryListingFrame::makeSingleMenu(ItemInfo* ii) {
	ShellMenuPtr menu = addChild(ShellMenu::Seed());

	menu->setTitle(escapeMenu(ii->getText(COLUMN_FILENAME)), WinUtil::fileImages->getIcon(ii->getImage()));

	menu->appendItem(T_("&Download"), std::bind(&DirectoryListingFrame::handleDownload, this), WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargets(menu, ii);

	if(ii->type == ItemInfo::FILE) {
		menu->appendItem(T_("&View as text"), std::bind(&DirectoryListingFrame::handleViewAsText, this));

		menu->appendSeparator();

		WinUtil::addHashItems(menu, ii->file->getTTH(), Text::toT(ii->file->getName()), ii->file->getSize());
	}

	if((ii->type == ItemInfo::FILE && ii->file->getAdls()) ||
		(ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()) )	{
		menu->appendSeparator();
		menu->appendItem(T_("&Go to directory"), std::bind(&DirectoryListingFrame::handleGoToDirectory, this));
	}

	addUserCommands(menu);
	return menu;
}

ShellMenuPtr DirectoryListingFrame::makeMultiMenu() {
	ShellMenuPtr menu = addChild(ShellMenu::Seed());

	size_t sel = files->countSelected();
	menu->setTitle(str(TF_("%1% items") % sel), getParent()->getIcon(this));

	menu->appendItem(T_("&Download"), std::bind(&DirectoryListingFrame::handleDownload, this), WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargets(menu);
	addUserCommands(menu);

	return menu;
}

ShellMenuPtr DirectoryListingFrame::makeDirMenu(ItemInfo* ii) {
	ShellMenuPtr menu = addChild(ShellMenu::Seed());

	menu->setTitle(escapeMenu(ii ? ii->getText(COLUMN_FILENAME) : getText()),
		ii ? WinUtil::fileImages->getIcon(ii->getImage()) : getParent()->getIcon(this));

	menu->appendItem(T_("&Download"), std::bind(&DirectoryListingFrame::handleDownload, this), WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargets(menu);
	return menu;
}

void DirectoryListingFrame::addUserCommands(const MenuPtr& parent) {
	prepareMenu(parent, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubs(dl->getUser().user->getCID(), dl->getUser().hint));
}

void DirectoryListingFrame::addShellPaths(const ShellMenuPtr& menu, const vector<ItemInfo*>& sel) {
	StringList ShellMenuPaths;
	for(auto i = sel.cbegin(), iend = sel.cend(); i != iend; ++i) {
		ItemInfo* ii = *i;
		StringList paths;
		switch(ii->type) {
			case ItemInfo::FILE: paths = dl->getLocalPaths(ii->file); break;
			case ItemInfo::DIRECTORY: paths = dl->getLocalPaths(ii->dir); break;
		}
		if(!paths.empty())
			ShellMenuPaths.insert(ShellMenuPaths.end(), paths.begin(), paths.end());
	}
	menu->appendShellMenu(ShellMenuPaths);
}

void DirectoryListingFrame::addUserMenu(const MenuPtr& menu) {
	appendUserItems(getParent(), menu->appendPopup(T_("User")), true, true, false);
}

void DirectoryListingFrame::addTargets(const MenuPtr& parent, ItemInfo* ii) {
	MenuPtr menu = parent->appendPopup(T_("Download &to..."));
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	size_t i = 0;
	for(; i < spl.size(); ++i) {
		menu->appendItem(Text::toT(spl[i].second), std::bind(&DirectoryListingFrame::handleDownloadFavorite, this, i));
	}

	if(i > 0) {
		menu->appendSeparator();
	}

	menu->appendItem(T_("&Browse..."), std::bind(&DirectoryListingFrame::handleDownloadBrowse, this));

	targets.clear();

	if(ii && ii->type == ItemInfo::FILE) {
		QueueManager::getInstance()->getTargets(ii->file->getTTH(), targets);
		if(!targets.empty()) {
			menu->appendSeparator();
			for(i = 0; i < targets.size(); ++i) {
				menu->appendItem(Text::toT(targets[i]), std::bind(&DirectoryListingFrame::handleDownloadTarget, this, i));
			}
		}
	}

	if(WinUtil::lastDirs.size() > 0) {
		menu->appendSeparator();

		for(i = 0; i < WinUtil::lastDirs.size(); ++i) {
			menu->appendItem(WinUtil::lastDirs[i], std::bind(&DirectoryListingFrame::handleDownloadLastDir, this, i));
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

		if(dl->getUser() == ClientManager::getInstance()->getMe()) {
			vector<ItemInfo*> sel;
			for(auto i = selected.cbegin(), iend = selected.cend(); i != iend; ++i)
				sel.push_back(files->getData(*i));
			addShellPaths(menu, sel);
		}

		addUserMenu(menu);

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
		ItemInfo* ii = dirs->getSelectedData();
		ShellMenuPtr menu = makeDirMenu(ii);

		if(ii && dl->getUser() == ClientManager::getInstance()->getMe()) {
			addShellPaths(menu, vector<ItemInfo*>(1, ii));
		}

		addUserMenu(menu);

		usingDirMenu = true;
		menu->open(pt);
		return true;
	}
	return false;
}

void DirectoryListingFrame::downloadFiles(const string& aTarget, bool view /* = false */) {
	int i=-1;
	while((i = files->getNext(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = files->getData(i);
		if(view && dl->getUser() == ClientManager::getInstance()->getMe()) {
			StringList paths = dl->getLocalPaths(ii->file);
			if(!paths.empty()) {
				TextFrame::openWindow(this->getParent(), paths[0]);
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
			if(FolderDialog(this).open(target)) {
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
					if(SaveDialog(this).open(target)) {
						WinUtil::addLastDir(Util::getFilePath(target));
						dl->download(ii->file, Text::fromT(target), false, WinUtil::isShift());
					}
				} else {
					tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
					if(FolderDialog(this).open(target)) {
						WinUtil::addLastDir(target);
						dl->download(ii->dir, Text::fromT(target), WinUtil::isShift());
					}
				}
			} catch(const Exception& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(FolderDialog(this).open(target)) {
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
		dirs->setSelected(ht);
		dirs->ensureVisible(ht);
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
		if(dl->getUser().user->isOnline()) {
			try {
				QueueManager::getInstance()->addList(dl->getUser(), QueueItem::FLAG_PARTIAL_LIST, dl->getPath(d));
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

pair<HTREEITEM, int> DirectoryListingFrame::findFile(const StringSearch& str, bool reverse, HTREEITEM item, int pos, vector<HTREEITEM>& collapse) {
	// try to match the names currently in the list pane
	const int n = files->size();
	if(reverse && pos == -1)
		pos = n;
	for(reverse ? --pos : ++pos; reverse ? (pos >= 0) : (pos < n); reverse ? --pos : ++pos) {
		const ItemInfo& ii = *files->getData(pos);
		const string& name = (ii.type == ItemInfo::FILE) ? ii.file->getName() : ii.dir->getName();
		if(str.match(name))
			return make_pair(item, pos);
	}

	// flow to the next directory
	if(!reverse && dirs->getChild(item) && !dirs->isExpanded(item)) {
		dirs->expand(item);
		collapse.push_back(item);
	}
	HTREEITEM next = dirs->getNext(item, reverse ? TVGN_PREVIOUSVISIBLE : TVGN_NEXTVISIBLE);
	if(next) {
		if(reverse && dirs->getChild(next) && !dirs->isExpanded(next)) {
			dirs->expand(next);
			collapse.push_back(next);
			next = dirs->getNext(item, TVGN_PREVIOUSVISIBLE);
		}

		// refresh the list pane to respect sorting etc
		changeDir(dirs->getData(next)->dir);

		return findFile(str, reverse, next, -1, collapse);
	}

	return make_pair(nullptr, 0);
}

void DirectoryListingFrame::findFile(FindMode mode) {
	const tstring findStr = searchBox->getText();
	if(findStr.empty())
		return;

	{
		// make sure the new search string is at the top of the list
		auto prev = std::find(lastSearches.begin(), lastSearches.end(), findStr);
		if(prev == lastSearches.end()) {
			size_t i = max(SETTING(SEARCH_HISTORY) - 1, 0);
			while(lastSearches.size() > i) {
				lastSearches.erase(lastSearches.begin());
			}
		} else {
			searchBox->erase(lastSearches.end() - 1 - prev); // the GUI list is in reverse order...
			searchBox->setText(findStr); // it erases the text-box too...
			lastSearches.erase(prev);
		}
		lastSearches.push_back(findStr);
		searchBox->insertValue(0, findStr);
	}

	HoldRedraw hold(files);
	HoldRedraw hold2(dirs);
	HoldRedraw hold3(status);

	HTREEITEM const oldDir = dirs->getSelected();

	auto selectDir = [this, oldDir](HTREEITEM newDir) {
		// SelectItem won't update the list if SetRedraw was set to FALSE and then
		// to TRUE and the selected item is the same as the last one... workaround:
		if(newDir == oldDir)
			dirs->setSelected(nullptr);
		dirs->setSelected(newDir);
		dirs->ensureVisible(newDir);
	};

	if(mode == FIND_START) {
		dirs->setSelected(treeRoot);
		files->clearSelection();
	}

	vector<HTREEITEM> collapse;
	auto search = findFile(StringSearch(Text::fromT(findStr)), mode == FIND_PREV,
		(mode == FIND_START) ? treeRoot : oldDir, files->getSelected(), collapse);
	for(auto i = collapse.cbegin(), iend = collapse.cend(); i != iend; ++i)
		dirs->collapse(*i);

	if(search.first) {
		selectDir(search.first);

		// Remove prev. selection from file list
		files->clearSelection();

		// Highlight and focus the file
		files->setFocus();
		files->setSelected(search.second);
		files->ensureVisible(search.second);

	} else {
		selectDir(oldDir);
		dwt::MessageBox(this).show(T_("No matches found for:") + _T("\n") + findStr, T_("Search for file"));
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
		if(uc.once()) {
			if(users.find(dl->getUser()) != users.end())
				continue;
			users.insert(dl->getUser());
		}
		if(!dl->getUser().user->isOnline())
			return;
		ucParams["fileTR"] = "NONE";
		if(ii->type == ItemInfo::FILE) {
			ucParams["type"] = "File";
			ucParams["fileFN"] = dl->getPath(ii->file) + ii->file->getName();
			ucParams["fileSI"] = Util::toString(ii->file->getSize());
			ucParams["fileSIshort"] = Util::formatBytes(ii->file->getSize());
			ucParams["fileTR"] = ii->file->getTTH().toBase32();
			ucParams["fileMN"] = WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize());
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
	if(c == VK_LEFT && isControlPressed()) {
		back();
		return true;
	}
	if(c == VK_RIGHT && isControlPressed()) {
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

void DirectoryListingFrame::tabMenuImpl(dwt::MenuPtr& menu) {
	addUserMenu(menu);
	menu->appendSeparator();
}

DirectoryListingFrame::UserInfoList DirectoryListingFrame::selectedUsersImpl() {
	return UserInfoList(1, &user);
}

void DirectoryListingFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) throw() {
	if(aUser.getUser() == dl->getUser().user)
		callAsync(std::bind(&DirectoryListingFrame::updateTitle, this));
}
void DirectoryListingFrame::on(ClientManagerListener::UserConnected, const UserPtr& aUser) throw() {
	if(aUser == dl->getUser().user)
		callAsync(std::bind(&DirectoryListingFrame::updateTitle, this));
}
void DirectoryListingFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
	if(aUser == dl->getUser().user)
		callAsync(std::bind(&DirectoryListingFrame::updateTitle, this));
}
