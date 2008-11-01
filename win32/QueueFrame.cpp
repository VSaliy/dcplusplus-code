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

#include "QueueFrame.h"
#include "WinUtil.h"
#include "resource.h"
#include "HoldRedraw.h"
#include "PrivateFrame.h"

#include <dcpp/QueueManager.h>
#include <dcpp/version.h>

#include <dwt/util/StringUtils.h>

static const ColumnInfo filesColumns[] = {
	{ N_("Filename"), 200, false },
	{ N_("Status"), 300, false },
	{ N_("Size"), 80, true },
	{ N_("Downloaded"), 110, true },
	{ N_("Priority"), 75, false },
	{ N_("Users"), 200, false },
	{ N_("Path"), 200, false },
	{ N_("Exact size"), 100, true },
	{ N_("Errors"), 200, false },
	{ N_("Added"), 100, false },
	{ N_("TTH"), 300, false },
	{ N_("Type"), 60, false }
};

#define FILE_LIST_NAME _T("File Lists")

void QueueFrame::QueueItemInfo::remove() {
	QueueManager::getInstance()->remove(getTarget());
}

QueueFrame::QueueFrame(dwt::TabView* mdiParent) :
	BaseType(mdiParent, T_("Download Queue"), IDH_QUEUE, IDR_QUEUE),
	dirs(0),
	files(0),
	paned(0),
	showTree(0),
	dirty(true),
	usingDirMenu(false),
	queueSize(0),
	queueItems(0),
	fileLists(0)
{
	paned = addChild(WidgetVPaned::Seed(SETTING(QUEUE_PANED_POS)));

	{
		dirs = addChild(WidgetDirs::Seed());
		addWidget(dirs);
		paned->setFirst(dirs);

		dirs->setNormalImageList(WinUtil::fileImages);

		dirs->onSelectionChanged(std::tr1::bind(&QueueFrame::updateFiles, this));
		dirs->onKeyDown(std::tr1::bind(&QueueFrame::handleKeyDownDirs, this, _1));
		dirs->onContextMenu(std::tr1::bind(&QueueFrame::handleDirsContextMenu, this, _1));
	}

	{
		files = addChild(WidgetFiles::Seed());
		addWidget(files, true);
		paned->setSecond(files);

		files->setSmallImageList(WinUtil::fileImages);
		WinUtil::makeColumns(files, filesColumns, COLUMN_LAST, SETTING(QUEUEFRAME_ORDER), SETTING(QUEUEFRAME_WIDTHS));
		files->setSort(COLUMN_TARGET);

		files->onKeyDown(std::tr1::bind(&QueueFrame::handleKeyDownFiles, this, _1));
		files->onSelectionChanged(std::tr1::bind(&QueueFrame::callAsync, this,
			dwt::Application::Callback(std::tr1::bind(&QueueFrame::updateStatus, this))));
		files->onContextMenu(std::tr1::bind(&QueueFrame::handleFilesContextMenu, this, _1));
	}

	{
		CheckBox::Seed cs(_T("+/-"));
		cs.style &= ~WS_TABSTOP;
		showTree = addChild(cs);
		showTree->setHelpId(IDH_QUEUE_SHOW_TREE);
		showTree->setChecked(BOOLSETTING(QUEUEFRAME_SHOW_TREE));
		showTree->onClicked(std::tr1::bind(&QueueFrame::handleShowTreeClicked, this));
	}

	initStatus();
	statusSizes[STATUS_SHOW_TREE] = 16;

	setStatusHelpId(STATUS_PARTIAL_COUNT, IDH_QUEUE_PARTIAL_COUNT);
	setStatusHelpId(STATUS_PARTIAL_BYTES, IDH_QUEUE_PARTIAL_BYTES);
	setStatusHelpId(STATUS_TOTAL_COUNT, IDH_QUEUE_TOTAL_COUNT);
	setStatusHelpId(STATUS_TOTAL_BYTES, IDH_QUEUE_TOTAL_BYTES);

	addQueueList(QueueManager::getInstance()->lockQueue());
	QueueManager::getInstance()->unlockQueue();
	QueueManager::getInstance()->addListener(this);

	updateStatus();
	layout();
}

QueueFrame::~QueueFrame() {

}

void QueueFrame::layout() {
	dwt::Rectangle r(getClientAreaSize());

	layoutStatus(r);

	mapWidget(STATUS_SHOW_TREE, showTree);

	bool checked = showTree->getChecked();
	if(checked && !paned->getFirst()) {
		paned->setFirst(dirs);
	} else if(!checked && paned->getFirst()) {
		paned->setFirst(0);
	}
	paned->setRect(r);

}

bool QueueFrame::handleKeyDownDirs(int c) {
	if(c == VK_DELETE) {
		removeSelectedDir();
	}
	return false;
}

bool QueueFrame::handleKeyDownFiles(int c) {
	if(c == VK_DELETE) {
		removeSelected();
	} else if(c == VK_ADD){
		// Increase Item priority
		changePriority(true);
	} else if(c == VK_SUBTRACT){
		// Decrease item priority
		changePriority(false);
	}
	return false;
}

void QueueFrame::addQueueList(const QueueItem::StringMap& li) {
	HoldRedraw hold(files);
	HoldRedraw hold2(dirs);

	for(QueueItem::StringMap::const_iterator j = li.begin(); j != li.end(); ++j) {
		QueueItem* aQI = j->second;
		QueueItemInfo* ii = new QueueItemInfo(*aQI);
		addQueueItem(ii, true);
	}

	files->resort();
}

QueueFrame::QueueItemInfo* QueueFrame::getItemInfo(const string& target) {
	string path = Util::getFilePath(target);
	DirectoryPair items = directories.equal_range(path);
	for(DirectoryIter i = items.first; i != items.second; ++i) {
		if(i->second->getTarget() == target) {
			return i->second;
		}
	}
	return 0;
}

bool QueueFrame::isCurDir(const std::string& aDir) const {
	 return Util::stricmp(curDir, aDir) == 0;
}

void QueueFrame::updateStatus() {
	int64_t total = 0;
	int cnt = files->countSelected();
	if(cnt < 2) {
		cnt = files->size();
		if(showTree->getChecked()) {
			for(int i = 0; i < cnt; ++i) {
				QueueItemInfo* ii = files->getData(i);
				total += (ii->getSize() > 0) ? ii->getSize() : 0;
			}
		} else {
			total = queueSize;
		}
	} else {
		int i = -1;
		while( (i = files->getNext(i, LVNI_SELECTED)) != -1) {
			QueueItemInfo* ii = files->getData(i);
			total += (ii->getSize() > 0) ? ii->getSize() : 0;
		}
	}

	setStatus(STATUS_PARTIAL_COUNT, str(TF_("Items: %1%") % cnt));
	setStatus(STATUS_PARTIAL_BYTES, str(TF_("Size: %1%") % Text::toT(Util::formatBytes(total))));

	if(dirty) {
		setStatus(STATUS_TOTAL_COUNT, str(TF_("Files: %1%") % queueItems));
		setStatus(STATUS_TOTAL_BYTES, str(TF_("Size: %1%") % Text::toT(Util::formatBytes(queueSize))));
		dirty = false;
	}
}

bool QueueFrame::preClosing() {
	QueueManager::getInstance()->removeListener(this);
	return true;
}

void QueueFrame::postClosing() {
	HTREEITEM ht = dirs->getRoot();
	while(ht != NULL) {
		clearTree(ht);
		ht = dirs->getNextSibling(ht);
	}

	SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_SHOW_TREE, showTree->getChecked());
	SettingsManager::getInstance()->set(SettingsManager::QUEUE_PANED_POS, paned->getRelativePos());

	for(DirectoryIter i = directories.begin(); i != directories.end(); ++i) {
		delete i->second;
	}
	directories.clear();
	files->clear();

	SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_ORDER, WinUtil::toString(files->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_WIDTHS, WinUtil::toString(files->getColumnWidths()));
}

void QueueFrame::addQueueItem(QueueItemInfo* ii, bool noSort) {
	if(!ii->isSet(QueueItem::FLAG_USER_LIST)) {
		queueSize+=ii->getSize();
	}
	queueItems++;
	dirty = true;

	const string& dir = ii->getPath();

	bool updateDir = (directories.find(dir) == directories.end());
	directories.insert(make_pair(dir, ii));

	if(updateDir) {
		addDirectory(dir, ii->isSet(QueueItem::FLAG_USER_LIST));
	}
	if(!showTree->getChecked() || isCurDir(dir)) {
		ii->update();
		if(noSort) {
			files->insert(files->size(), ii);
		} else {
			files->insert(ii);
		}
	}
}

void QueueFrame::handleShowTreeClicked() {
	bool checked = showTree->getChecked();

	dirs->setVisible(checked);
	paned->setVisible(checked);

	layout();
}

void QueueFrame::updateFiles() {
	HoldRedraw hold(files);

	files->clear();
	pair<DirectoryIter, DirectoryIter> i;
	if(showTree->getChecked()) {
		i = directories.equal_range(getSelectedDir());
	} else {
		i.first = directories.begin();
		i.second = directories.end();
	}

	for(DirectoryIter j = i.first; j != i.second; ++j) {
		QueueItemInfo* ii = j->second;
		ii->update();
		files->insert(files->size(), ii);
	}

	files->resort();

	curDir = getSelectedDir();
	updateStatus();
}

void QueueFrame::QueueItemInfo::update() {
	if(display != NULL) {
		int colMask = updateMask;
		updateMask = 0;

		if(colMask & MASK_TARGET) {
			display->columns[COLUMN_TARGET] = Text::toT(Util::getFileName(getTarget()));
		}
		int online = 0;
		if(colMask & MASK_USERS || colMask & MASK_STATUS) {
			tstring tmp;

			for(QueueItem::SourceIter j = getSources().begin(); j != getSources().end(); ++j) {
				if(tmp.size() > 0)
					tmp += _T(", ");

				if(j->getUser()->isOnline())
					online++;

				tmp += WinUtil::getNicks(j->getUser());
			}
			display->columns[COLUMN_USERS] = tmp.empty() ? T_("No users") : tmp;
		}
		if(colMask & MASK_STATUS) {
			if(!getRunning()) {
				if(online > 0) {
					if(getSources().size() == 1) {
						display->columns[COLUMN_STATUS] = T_("Waiting (User online)");
					} else {
						display->columns[COLUMN_STATUS] = str(TF_("Waiting (%1% of %2% users online)") % online % getSources().size());
					}
				} else {
					if(getSources().size() == 0) {
						display->columns[COLUMN_STATUS] = T_("No users to download from");
					} else if(getSources().size() == 1) {
						display->columns[COLUMN_STATUS] = T_("User offline");
					} else {
						display->columns[COLUMN_STATUS] = str(TF_("All %1% users offline") % getSources().size());
					}
				}
			} else {
				display->columns[COLUMN_STATUS] = T_("Running...");
			}
		}
		if(colMask & MASK_SIZE) {
			display->columns[COLUMN_SIZE] = (getSize() == -1) ? T_("Unknown") : Text::toT(Util::formatBytes(getSize()));
			display->columns[COLUMN_EXACT_SIZE] = (getSize() == -1) ? T_("Unknown") : Text::toT(Util::formatExactSize(getSize()));
		}
		if(colMask & MASK_DOWNLOADED) {
			if(getSize() > 0)
				display->columns[COLUMN_DOWNLOADED] = Text::toT(Util::formatBytes(getDownloadedBytes()) + " (" + Util::toString((double)getDownloadedBytes()*100.0/(double)getSize()) + "%)");
			else
				display->columns[COLUMN_DOWNLOADED].clear();
		}
		if(colMask & MASK_PRIORITY) {
			switch(getPriority()) {
		case QueueItem::PAUSED: display->columns[COLUMN_PRIORITY] = T_("Paused"); break;
		case QueueItem::LOWEST: display->columns[COLUMN_PRIORITY] = T_("Lowest"); break;
		case QueueItem::LOW: display->columns[COLUMN_PRIORITY] = T_("Low"); break;
		case QueueItem::NORMAL: display->columns[COLUMN_PRIORITY] = T_("Normal"); break;
		case QueueItem::HIGH: display->columns[COLUMN_PRIORITY] = T_("High"); break;
		case QueueItem::HIGHEST: display->columns[COLUMN_PRIORITY] = T_("Highest"); break;
		default: dcasserta(0); break;
			}
		}

		if(colMask & MASK_PATH) {
			display->columns[COLUMN_PATH] = Text::toT(getPath());
		}

		if(colMask & MASK_ERRORS) {
			tstring tmp;
			for(QueueItem::SourceIter j = getBadSources().begin(); j != getBadSources().end(); ++j) {
				if(!j->isSet(QueueItem::Source::FLAG_REMOVED)) {
					if(tmp.size() > 0)
						tmp += _T(", ");
					tmp += WinUtil::getNicks(j->getUser());
					tmp += _T(" (");
					if(j->isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
						tmp += T_("File not available");
					} else if(j->isSet(QueueItem::Source::FLAG_PASSIVE)) {
						tmp += T_("Passive user");
					} else if(j->isSet(QueueItem::Source::FLAG_CRC_FAILED)) {
						tmp += T_("CRC32 inconsistency (SFV-Check)");
					} else if(j->isSet(QueueItem::Source::FLAG_BAD_TREE)) {
						tmp += T_("Full tree does not match TTH root");
					} else if(j->isSet(QueueItem::Source::FLAG_SLOW_SOURCE)) {
						tmp += T_("Source too slow");
					} else if(j->isSet(QueueItem::Source::FLAG_NO_TTHF)) {
						tmp += T_("Remote client does not fully support TTH - cannot download");
					} else if(j->isSet(QueueItem::Source::FLAG_UNTRUSTED)) {
						tmp += T_("User certificate not trusted");
					}
					tmp += ')';
				}
			}
			display->columns[COLUMN_ERRORS] = tmp.empty() ? T_("No errors") : tmp;
		}

		if(colMask & MASK_ADDED) {
			display->columns[COLUMN_ADDED] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M", getAdded()));
		}
		if(colMask & MASK_TTH) {
			display->columns[COLUMN_TTH] = Text::toT(getTTH().toBase32());
		}
		if(colMask & MASK_TYPE) {
			display->columns[COLUMN_TYPE] = Text::toT(Util::getFileExt(getTarget()));
			if(display->columns[COLUMN_TYPE].size() > 0 && display->columns[COLUMN_TYPE][0] == '.')
				display->columns[COLUMN_TYPE].erase(0, 1);
		}
	}
}

static tstring getDisplayName(const string& name) {
	if(name.empty())
		return Util::emptyStringT;
	if(name.length() == 3 && name[1] == ':') {
		return Text::toT(name);
	}
	return Text::toT(name.substr(0, name.length() - 1));
}

QueueFrame::DirItemInfo::DirItemInfo(const string& dir_) : dir(dir_), text(getDisplayName(dir_)) {

}

int QueueFrame::DirItemInfo::getImage() {
	return WinUtil::getDirIconIndex();
}

int QueueFrame::DirItemInfo::getSelectedImage() {
	return WinUtil::getDirIconIndex();
}

HTREEITEM QueueFrame::addDirectory(const string& dir, bool isFileList /* = false */, HTREEITEM startAt /* = NULL */) {
	if(isFileList) {
		// We assume we haven't added it yet, and that all filelists go to the same
		// directory...
		dcassert(fileLists == NULL);
		fileLists = dirs->insert(NULL, new DirItemInfo(dir, FILE_LIST_NAME), true);
		return fileLists;
	}

	// More complicated, we have to find the last available tree item and then see...
	string::size_type i = 0;
	string::size_type j;

	HTREEITEM next = NULL;
	HTREEITEM parent = NULL;

	if(startAt == NULL) {
		// First find the correct drive letter
		dcassert(dir[1] == ':');
		dcassert(dir[2] == '\\');

		next = dirs->getRoot();

		while(next != NULL) {
			if(next != fileLists) {
				if(Util::strnicmp(getDir(next), dir, 3) == 0)
					break;
			}
			next = dirs->getNextSibling(next);
		}

		if(next == NULL) {
			// First addition, set commonStart to the dir minus the last part...
			i = dir.rfind('\\', dir.length()-2);
			if(i != string::npos) {
				next = dirs->insert(NULL, new DirItemInfo(dir.substr(0, i+1)), true);
			} else {
				dcassert(dir.length() == 3);
				next = dirs->insert(NULL, new DirItemInfo(dir, Text::toT(dir)), true);
			}
		}

		// Ok, next now points to x:\... find how much is common

		DirItemInfo* rootInfo = dirs->getData(next);
		const string& rootStr = rootInfo->getDir();

		i = 0;

		for(;;) {
			j = dir.find('\\', i);
			if(j == string::npos)
				break;
			if(Util::strnicmp(dir.c_str() + i, rootStr.c_str() + i, j - i + 1) != 0)
				break;
			i = j + 1;
		}

		if(i < rootStr.length()) {
			HTREEITEM oldRoot = next;

			// Create a new root
			HTREEITEM newRoot = dirs->insert(NULL, new DirItemInfo(rootStr.substr(0, i)), true);

			parent = addDirectory(rootStr, false, newRoot);

			next = dirs->getChild(oldRoot);
			while(next != NULL) {
				moveNode(next, parent);
				next = dirs->getChild(oldRoot);
			}
			delete rootInfo;
			dirs->erase(oldRoot);
			parent = newRoot;
		} else {
			// Use this root as parent
			parent = next;
			next = dirs->getChild(parent);
		}
	} else {
		parent = startAt;
		next = dirs->getChild(parent);
		i = getDir(parent).length();
		dcassert(Util::strnicmp(getDir(parent), dir, i) == 0);
	}

	while( i < dir.length() ) {
		while(next != NULL) {
			if(next != fileLists) {
				const string& n = getDir(next);
				if(Util::strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Found a part, we assume it's the best one we can find...
					i = n.length();

					parent = next;
					next = dirs->getChild(next);
					break;
				}
			}
			next = dirs->getNextSibling(next);
		}

		if(next == NULL) {
			// We didn't find it, add...
			j = dir.find('\\', i);
			dcassert(j != string::npos);
			parent = dirs->insert(parent, new DirItemInfo(dir.substr(0, j+1), Text::toT(dir.substr(i, j-i))), true);
			i = j + 1;
		}
	}

	return parent;
}

void QueueFrame::removeDirectory(const string& dir, bool isFileList /* = false */) {
	// First, find the last name available
	string::size_type i = 0;

	HTREEITEM next = dirs->getRoot();
	HTREEITEM parent = NULL;

	if(isFileList) {
		dcassert(fileLists != NULL);
		delete dirs->getData(fileLists);
		dirs->erase(fileLists);
		fileLists = NULL;
		return;
	} else {
		while(i < dir.length()) {
			while(next != NULL) {
				if(next != fileLists) {
					const string& n = getDir(next);
					if(Util::strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
						// Match!
						parent = next;
						next = dirs->getChild(next);
						i = n.length();
						break;
					}
				}
				next = dirs->getNextSibling(next);
			}
			if(next == NULL)
				break;
		}
	}

	next = parent;

	while((dirs->getChild(next) == NULL) && (directories.find(getDir(next)) == directories.end())) {
		delete dirs->getData(next);
		parent = dirs->getParent(next);

		dirs->erase(next);
		if(parent == NULL)
			break;
		next = parent;
	}
}

void QueueFrame::removeDirectories(HTREEITEM ht) {
	HTREEITEM next = dirs->getChild(ht);
	while(next != NULL) {
		removeDirectories(next);
		next = dirs->getNextSibling(ht);
	}
	delete dirs->getData(ht);
	dirs->erase(ht);
}

void QueueFrame::removeSelected() {
	if(!BOOLSETTING(CONFIRM_ITEM_REMOVAL) || createMessageBox().show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_YESNO, MessageBox::BOX_ICONQUESTION) == IDYES)
		files->forEachSelected(&QueueItemInfo::remove);
}

void QueueFrame::removeSelectedDir() {
	if(!BOOLSETTING(CONFIRM_ITEM_REMOVAL) || createMessageBox().show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_YESNO, MessageBox::BOX_ICONQUESTION) == IDYES)
		removeDir(dirs->getSelected());
}

void QueueFrame::moveSelected() {
	int n = files->countSelected();
	if(n == 1) {
		// Single file, get the full filename and move...
		QueueItemInfo* ii = files->getSelectedData();
		tstring target = Text::toT(ii->getTarget());

		if(WinUtil::browseSaveFile(createSaveDialog(), target)) {
			QueueManager::getInstance()->move(ii->getTarget(), Text::fromT(target));
		}
	} else if(n > 1) {
		tstring name;
		if(showTree->getChecked()) {
			name = Text::toT(curDir);
		}
		if(createFolderDialog().open(name)) {
			int i = -1;
			while( (i = files->getNext(i, LVNI_SELECTED)) != -1) {
				QueueItemInfo* ii = files->getData(i);
				QueueManager::getInstance()->move(ii->getTarget(), Text::fromT(name) + Util::getFileName(ii->getTarget()));
			}
		}
	}
}

void QueueFrame::moveSelectedDir() {
	HTREEITEM item = dirs->getSelected();
	if(!item)
		return;

	dcassert(!curDir.empty());
	tstring name = Text::toT(curDir);

	if(createFolderDialog().open(name)) {
		moveDir(item, Text::fromT(name));
	}
}

void QueueFrame::moveDir(HTREEITEM ht, const string& target) {
	HTREEITEM next = dirs->getChild(ht);
	while(next != NULL) {
		// must add path separator since getLastDir only give us the name
		moveDir(next, target + Util::getLastDir(getDir(next)) + PATH_SEPARATOR);
		next = dirs->getNextSibling(next);
	}
	const string& dir = getDir(ht);

	DirectoryPair p = directories.equal_range(dir);

	for(DirectoryIter i = p.first; i != p.second; ++i) {
		QueueItemInfo* ii = i->second;
		QueueManager::getInstance()->move(ii->getTarget(), target + Util::getFileName(ii->getTarget()));
	}
}

void QueueFrame::handleBrowseList(const UserPtr& user) {

	if(files->countSelected() == 1) {
		try {
			QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception&) {
		}
	}
}

void QueueFrame::handleReadd(const UserPtr& user) {

	if(files->countSelected() == 1) {
		QueueItemInfo* ii = files->getSelectedData();

		if(!user) {
			// re-add all sources
			for(QueueItem::SourceIter s = ii->getBadSources().begin(); s != ii->getBadSources().end(); ++s) {
				QueueManager::getInstance()->readd(ii->getTarget(), s->getUser());
			}
		} else {
			try {
				QueueManager::getInstance()->readd(ii->getTarget(), user);
			} catch(const Exception& e) {
				setStatus(STATUS_STATUS, Text::toT(e.getError()));
			}
		}
	}
}

void QueueFrame::handleRemove() {
	usingDirMenu ? removeSelectedDir() : removeSelected();
}

void QueueFrame::handleMove() {
	usingDirMenu ? moveSelectedDir() : moveSelected();
}

void QueueFrame::handleRemoveSource(const UserPtr& user) {

	if(files->countSelected() == 1) {
		QueueItemInfo* ii = files->getSelectedData();

		if(!user) {
			for(QueueItem::SourceIter si = ii->getSources().begin(); si != ii->getSources().end(); ++si) {
				QueueManager::getInstance()->removeSource(ii->getTarget(), si->getUser(), QueueItem::Source::FLAG_REMOVED);
			}
		} else {
			QueueManager::getInstance()->removeSource(ii->getTarget(), user, QueueItem::Source::FLAG_REMOVED);
		}
	}
}

void QueueFrame::handleRemoveSources(const UserPtr& user) {
	QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
}

void QueueFrame::handlePM(const UserPtr& user) {
	if(files->countSelected() == 1) {
		PrivateFrame::openWindow(getParent(), user);
	}
}

void QueueFrame::handlePriority(QueueItem::Priority p) {
	if(usingDirMenu) {
		setPriority(dirs->getSelected(), p);
	} else {
		std::vector<size_t> selected = files->getSelection();
		for(std::vector<size_t>::iterator i = selected.begin(); i != selected.end(); ++i) {
			QueueManager::getInstance()->setPriority(files->getData(*i)->getTarget(), p);
		}
	}
}

void QueueFrame::removeDir(HTREEITEM ht) {
	if(ht == NULL)
		return;
	HTREEITEM child = dirs->getChild(ht);
	while(child) {
		removeDir(child);
		child = dirs->getNextSibling(child);
	}
	const string& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	for(DirectoryIter i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->remove(i->second->getTarget());
	}
}

/*
 * @param inc True = increase, False = decrease
 */
void QueueFrame::changePriority(bool inc){
	std::vector<size_t> selected = files->getSelection();
	for(std::vector<size_t>::iterator i = selected.begin(); i != selected.end(); ++i) {
		QueueItemInfo* ii = files->getData(*i);
		QueueItem::Priority p = ii->getPriority();

		if ((inc && p == QueueItem::HIGHEST) || (!inc && p == QueueItem::PAUSED)){
			// Trying to go higher than HIGHEST or lower than PAUSED
			// so do nothing
			continue;
		}

		switch(p){
			case QueueItem::HIGHEST: p = inc ? QueueItem::HIGHEST : QueueItem::HIGH; break;
			case QueueItem::HIGH:    p = inc ? QueueItem::HIGHEST : QueueItem::NORMAL; break;
			case QueueItem::NORMAL:  p = inc ? QueueItem::HIGH    : QueueItem::LOW; break;
			case QueueItem::LOW:     p = inc ? QueueItem::NORMAL  : QueueItem::LOWEST; break;
			case QueueItem::LOWEST:  p = inc ? QueueItem::LOW     : QueueItem::PAUSED; break;
			case QueueItem::PAUSED:  p = inc ? QueueItem::LOWEST : QueueItem::PAUSED; break;
		}

		QueueManager::getInstance()->setPriority(ii->getTarget(), p);
	}
}

void QueueFrame::setPriority(HTREEITEM ht, const QueueItem::Priority& p) {
	if(ht == NULL)
		return;
	HTREEITEM child = dirs->getChild(ht);
	while(child) {
		setPriority(child, p);
		child = dirs->getNextSibling(child);
	}
	const string& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	for(DirectoryIter i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setPriority(i->second->getTarget(), p);
	}
}


void QueueFrame::clearTree(HTREEITEM item) {
	HTREEITEM next = dirs->getChild(item);
	while(next != NULL) {
		clearTree(next);
		next = dirs->getNextSibling(next);
	}
	delete dirs->getData(item);
}

// Put it here to avoid a copy for each recursion...
static TCHAR tmpBuf[1024];
void QueueFrame::moveNode(HTREEITEM item, HTREEITEM parent) {
	TVINSERTSTRUCT tvis = { 0 };
	tvis.itemex.hItem = item;
	tvis.itemex.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM |
		TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_TEXT;
	tvis.itemex.pszText = tmpBuf;
	tvis.itemex.cchTextMax = 1024;
	dirs->getItem(&tvis.itemex);
	tvis.hInsertAfter =	TVI_SORT;
	tvis.hParent = parent;
	HTREEITEM ht = dirs->insert(&tvis);
	HTREEITEM next = dirs->getChild(item);
	while(next != NULL) {
		moveNode(next, ht);
		next = dirs->getChild(item);
	}
	dirs->erase(item);
}

const string& QueueFrame::getSelectedDir() {
	DirItemInfo* info = dirs->getSelectedData();
	return info == NULL ? Util::emptyString : info->getDir();
}

const string& QueueFrame::getDir(HTREEITEM item) {
	DirItemInfo* info = dirs->getData(item);
	return info == NULL ? Util::emptyString : info->getDir();
}

QueueFrame::MenuPtr QueueFrame::makeSingleMenu(QueueItemInfo* qii) {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	WinUtil::addHashItems(menu, qii->getTTH(), Text::toT(Util::getFileName(qii->getTarget())));
	menu->appendItem(T_("&Move/Rename"), std::tr1::bind(&QueueFrame::handleMove, this));
	addPriorityMenu(menu);
	addBrowseMenu(menu, qii);
	addPMMenu(menu, qii);
	menu->appendSeparator();
	addReaddMenu(menu, qii);
	addRemoveMenu(menu, qii);
	addRemoveSourcesMenu(menu, qii);
	menu->appendItem(T_("&Remove"), std::tr1::bind(&QueueFrame::handleRemove, this));

	return menu;
}

QueueFrame::MenuPtr QueueFrame::makeMultiMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	addPriorityMenu(menu);

	menu->appendItem(T_("&Move/Rename"), std::tr1::bind(&QueueFrame::handleMove, this));
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), std::tr1::bind(&QueueFrame::handleRemove, this));
	return menu;
}

QueueFrame::MenuPtr QueueFrame::makeDirMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	addPriorityMenu(menu);
	menu->appendItem(T_("&Move/Rename"), std::tr1::bind(&QueueFrame::handleMove, this));
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), std::tr1::bind(&QueueFrame::handleRemove, this));
	return menu;
}

void QueueFrame::addPriorityMenu(const MenuPtr& parent) {
	MenuPtr menu = parent->appendPopup(T_("Set priority"));
	menu->appendItem(T_("Paused"), std::tr1::bind(&QueueFrame::handlePriority, this, QueueItem::PAUSED));
	menu->appendItem(T_("Lowest"), std::tr1::bind(&QueueFrame::handlePriority, this, QueueItem::LOWEST));
	menu->appendItem(T_("Low"), std::tr1::bind(&QueueFrame::handlePriority, this, QueueItem::LOW));
	menu->appendItem(T_("Normal"), std::tr1::bind(&QueueFrame::handlePriority, this, QueueItem::NORMAL));
	menu->appendItem(T_("High"), std::tr1::bind(&QueueFrame::handlePriority, this, QueueItem::HIGH));
	menu->appendItem(T_("Highest"), std::tr1::bind(&QueueFrame::handlePriority, this, QueueItem::HIGHEST));
}

void QueueFrame::addBrowseMenu(const MenuPtr& parent, QueueItemInfo* qii) {
	unsigned int pos = parent->getCount();
	MenuPtr menu = parent->appendPopup(T_("&Get file list"));
	if(!addUsers(menu, &QueueFrame::handleBrowseList, qii->getSources(), false))
		parent->setItemEnabled(pos, false);
}

void QueueFrame::addPMMenu(const MenuPtr& parent, QueueItemInfo* qii) {
	unsigned int pos = parent->getCount();
	MenuPtr menu = parent->appendPopup(T_("&Send private message"));
	if(!addUsers(menu, &QueueFrame::handlePM, qii->getSources(), false))
		parent->setItemEnabled(pos, false);
}

void QueueFrame::addReaddMenu(const MenuPtr& parent, QueueItemInfo* qii) {
	unsigned int pos = parent->getCount();
	MenuPtr menu = parent->appendPopup(T_("Re-add source"));
	menu->appendItem(T_("All"), std::tr1::bind(&QueueFrame::handleReadd, this, UserPtr()));
	menu->appendSeparator();
	if(!addUsers(menu, &QueueFrame::handleReadd, qii->getBadSources(), true))
		parent->setItemEnabled(pos, false);
}

void QueueFrame::addRemoveMenu(const MenuPtr& parent, QueueItemInfo* qii) {
	unsigned int pos = parent->getCount();
	MenuPtr menu = parent->appendPopup(T_("Remove source"));
	menu->appendItem(T_("All"), std::tr1::bind(&QueueFrame::handleRemoveSource, this, UserPtr()));
	menu->appendSeparator();
	if(!addUsers(menu, &QueueFrame::handleRemoveSource, qii->getSources(), true))
		parent->setItemEnabled(pos, false);
}

void QueueFrame::addRemoveSourcesMenu(const MenuPtr& parent, QueueItemInfo* qii) {
	unsigned int pos = parent->getCount();
	MenuPtr menu = parent->appendPopup(T_("Remove user from queue"));
	if(!addUsers(menu, &QueueFrame::handleRemoveSources, qii->getSources(), true))
		parent->setItemEnabled(pos, false);
}

bool QueueFrame::addUsers(const MenuPtr& menu, void (QueueFrame::*handler)(const UserPtr&), const QueueItem::SourceList& sources, bool offline) {
	bool added = false;
	for(QueueItem::SourceConstIter i = sources.begin(); i != sources.end(); ++i) {
		const QueueItem::Source& source = *i;
		if(offline || source.getUser()->isOnline()) {
			tstring nick = dwt::util::escapeMenu(WinUtil::getNicks(source.getUser()));
			menu->appendItem(nick, std::tr1::bind(handler, this, source.getUser()));
			added = true;
		}
	}
	return added;
}

bool QueueFrame::handleFilesContextMenu(dwt::ScreenCoordinate pt) {
	if(files->countSelected() > 0) {
		if(pt.x() == -1 || pt.y() == -1) {
			pt = files->getContextMenuPos();
		}

		usingDirMenu = false;
		MenuPtr contextMenu;

		if(files->countSelected() == 1) {
			QueueItemInfo* ii = files->getSelectedData();
			contextMenu = makeSingleMenu(ii);
		} else {
			contextMenu = makeMultiMenu();
		}
		contextMenu->open(pt);

		return true;
	}
	return false;
}

bool QueueFrame::handleDirsContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = dirs->getContextMenuPos();
	} else {
		dirs->select(pt);
	}

	if(dirs->hasSelected()) {
		usingDirMenu = true;
		MenuPtr contextMenu = makeDirMenu();
		contextMenu->open(pt);

		return true;
	}

	return false;
}

void QueueFrame::onAdded(QueueItemInfo* ii) {
	dcassert(files->find(ii) == -1);
	addQueueItem(ii, false);
	updateStatus();
}

void QueueFrame::onRemoved(const string& s) {
	QueueItemInfo* ii = getItemInfo(s);
	if(!ii) {
		dcassert(ii);
		return;
	}

	if(!showTree->getChecked() || isCurDir(ii->getPath()) ) {
		dcassert(files->find(ii) != -1);
		files->erase(ii);
	}

	bool userList = ii->isSet(QueueItem::FLAG_USER_LIST);

	if(!userList) {
		queueSize-=ii->getSize();
		dcassert(queueSize >= 0);
	}
	queueItems--;
	dcassert(queueItems >= 0);

	pair<DirectoryIter, DirectoryIter> i = directories.equal_range(ii->getPath());
	DirectoryIter j;
	for(j = i.first; j != i.second; ++j) {
		if(j->second == ii)
			break;
	}
	dcassert(j != i.second);
	directories.erase(j);
	if(directories.count(ii->getPath()) == 0) {
		removeDirectory(ii->getPath(), ii->isSet(QueueItem::FLAG_USER_LIST));
		if(isCurDir(ii->getPath()))
			curDir.clear();
	}

	delete ii;
	updateStatus();
	if(!userList)
		setDirty(SettingsManager::BOLD_QUEUE);
	dirty = true;
}

void QueueFrame::onUpdated(const QueueItem& qi) {
	QueueItemInfo* ii = getItemInfo(qi.getTarget());

	ii->setPriority(qi.getPriority());
	ii->setRunning(qi.isRunning());
	ii->setDownloadedBytes(qi.getDownloadedBytes());
	ii->setSources(qi.getSources());
	ii->setBadSources(qi.getBadSources());

	ii->updateMask |= QueueItemInfo::MASK_PRIORITY | QueueItemInfo::MASK_USERS | QueueItemInfo::MASK_ERRORS | QueueItemInfo::MASK_STATUS | QueueItemInfo::MASK_DOWNLOADED;

	if(!showTree->getChecked() || isCurDir(ii->getPath())) {
		dcassert(files->find(ii) != -1);
		ii->update();
		files->update(ii);
	}
}

void QueueFrame::on(QueueManagerListener::Added, QueueItem* aQI) throw() {
	callAsync(std::tr1::bind(&QueueFrame::onAdded, this, new QueueItemInfo(*aQI)));
}

void QueueFrame::on(QueueManagerListener::Removed, QueueItem* aQI) throw() {
	callAsync(std::tr1::bind(&QueueFrame::onRemoved, this, aQI->getTarget()));
}

void QueueFrame::on(QueueManagerListener::Moved, QueueItem* aQI, const string& oldTarget) throw() {
	callAsync(std::tr1::bind(&QueueFrame::onRemoved, this, oldTarget));
	callAsync(std::tr1::bind(&QueueFrame::onAdded, this, new QueueItemInfo(*aQI)));
}

void QueueFrame::on(QueueManagerListener::SourcesUpdated, QueueItem* aQI) throw() {
	callAsync(std::tr1::bind(&QueueFrame::onUpdated, this, *aQI));
}
