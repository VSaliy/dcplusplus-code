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
#include "QueueFrame.h"

#include <dcpp/QueueManager.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/FolderDialog.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/SplitterContainer.h>

#include "WinUtil.h"
#include "resource.h"
#include "HoldRedraw.h"
#include "PrivateFrame.h"
#include "TypedTable.h"
#include "TypedTree.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::SplitterContainer;
using dwt::FolderDialog;

const string QueueFrame::id = "Queue";
const string& QueueFrame::getId() const { return id; }

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

QueueFrame::QueueFrame(TabViewPtr parent) :
BaseType(parent, T_("Download Queue"), IDH_QUEUE, IDI_QUEUE),
paned(0),
dirs(0),
files(0),
dirty(true),
usingDirMenu(false),
queueSize(0),
queueItems(0),
fileLists(0)
{
	paned = addChild(SplitterContainer::Seed(SETTING(QUEUE_PANED_POS)));

	{
		dirs = paned->addChild(WidgetDirs::Seed(WinUtil::Seeds::treeView));
		addWidget(dirs);

		dirs->setNormalImageList(WinUtil::fileImages);

		dirs->onSelectionChanged([this] { updateFiles(); });
		dirs->onKeyDown([this](int c) { return handleKeyDownDirs(c); });
		dirs->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleDirsContextMenu(sc); });
	}

	{
		files = paned->addChild(WidgetFiles::Seed(WinUtil::Seeds::table));
		addWidget(files, true);

		files->setSmallImageList(WinUtil::fileImages);
		WinUtil::makeColumns(files, filesColumns, COLUMN_LAST, SETTING(QUEUEFRAME_ORDER), SETTING(QUEUEFRAME_WIDTHS));
		files->setSort(COLUMN_TARGET);

		files->onKeyDown([this](int c) { return handleKeyDownFiles(c); });
		files->onSelectionChanged([this] { callAsync([&] { updateStatus(); }); });
		files->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleFilesContextMenu(sc); });

		if(!BOOLSETTING(QUEUEFRAME_SHOW_TREE)) {
			paned->maximize(files);
		}
	}

	initStatus();

	{
		auto showTree = addChild(WinUtil::Seeds::splitCheckBox);
		showTree->setHelpId(IDH_QUEUE_SHOW_TREE);
		showTree->setChecked(BOOLSETTING(QUEUEFRAME_SHOW_TREE));
		showTree->onClicked([this, showTree] {
			auto checked = showTree->getChecked();
			SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_SHOW_TREE, checked);
			paned->maximize(checked ? nullptr : files);
		});
		status->setWidget(STATUS_SHOW_TREE, showTree);
	}

	status->setHelpId(STATUS_PARTIAL_COUNT, IDH_QUEUE_PARTIAL_COUNT);
	status->setHelpId(STATUS_PARTIAL_BYTES, IDH_QUEUE_PARTIAL_BYTES);
	status->setHelpId(STATUS_TOTAL_COUNT, IDH_QUEUE_TOTAL_COUNT);
	status->setHelpId(STATUS_TOTAL_BYTES, IDH_QUEUE_TOTAL_BYTES);

	QueueManager::getInstance()->addListener(this, [this](const QueueItem::StringMap& qsm) { addQueueList(qsm); });

	updateStatus();

	layout();
}

QueueFrame::~QueueFrame() {

}

void QueueFrame::layout() {
	dwt::Rectangle r(getClientSize());

	r.size.y -= status->refresh();

	paned->resize(r);
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

	for(auto j = li.begin(); j != li.end(); ++j) {
		QueueItem* aQI = j->second;
		QueueItemInfo* ii = new QueueItemInfo(*aQI);
		addQueueItem(ii, true);
	}

	files->resort();
}

QueueFrame::QueueItemInfo* QueueFrame::getItemInfo(const string& target) {
	auto path = Util::getFilePath(target);
	auto items = directories.equal_range(path);
	for(auto i = items.first; i != items.second; ++i) {
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
		if(BOOLSETTING(QUEUEFRAME_SHOW_TREE)) {
			for(auto i = 0; i < cnt; ++i) {
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

	status->setText(STATUS_PARTIAL_COUNT, str(TF_("Items: %1%") % cnt));
	status->setText(STATUS_PARTIAL_BYTES, str(TF_("Size: %1%") % Text::toT(Util::formatBytes(total))));

	if(dirty) {
		status->setText(STATUS_TOTAL_COUNT, str(TF_("Files: %1%") % queueItems));
		status->setText(STATUS_TOTAL_BYTES, str(TF_("Size: %1%") % Text::toT(Util::formatBytes(queueSize))));
		dirty = false;
	}
}

bool QueueFrame::preClosing() {
	QueueManager::getInstance()->removeListener(this);
	return true;
}

void QueueFrame::postClosing() {
	dirs->clear();
	files->clear();
	for(auto i = directories.begin(); i != directories.end(); ++i) {
		delete i->second;
	}
	directories.clear();

	SettingsManager::getInstance()->set(SettingsManager::QUEUE_PANED_POS, paned->getSplitterPos(0));
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
	if(!BOOLSETTING(QUEUEFRAME_SHOW_TREE) || isCurDir(dir)) {
		ii->update();
		if(noSort) {
			files->insert(files->size(), ii);
		} else {
			files->insert(ii);
		}
	}
}

void QueueFrame::updateFiles() {
	HoldRedraw hold(files);

	files->clear();
	pair<DirectoryIter, DirectoryIter> i;
	if(BOOLSETTING(QUEUEFRAME_SHOW_TREE)) {
		i = directories.equal_range(getSelectedDir());
	} else {
		i.first = directories.begin();
		i.second = directories.end();
	}

	for(auto j = i.first; j != i.second; ++j) {
		QueueItemInfo* ii = j->second;
		ii->update();
		files->insert(files->size(), ii);
	}

	files->resort();

	curDir = getSelectedDir();
	updateStatus();
}

void QueueFrame::QueueItemInfo::update() {
	if(display.get()) {
		int colMask = updateMask;
		updateMask = 0;

		if(colMask & MASK_TARGET) {
			display->columns[COLUMN_TARGET] = Text::toT(Util::getFileName(getTarget()));
		}
		int online = 0;
		if(colMask & (MASK_USERS | MASK_STATUS)) {
			tstring tmp;

			for(auto j = getSources().begin(); j != getSources().end(); ++j) {
				if(tmp.size() > 0)
					tmp += _T(", ");

				if(j->getUser().user->isOnline())
					online++;

				tmp += WinUtil::getNicks(j->getUser());
			}
			display->columns[COLUMN_USERS] = tmp.empty() ? T_("No users") : tmp;
		}
		if(colMask & MASK_STATUS) {
			if(getStatus() == STATUS_WAITING) {
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
			} else if(getStatus() == STATUS_RUNNING) {
				display->columns[COLUMN_STATUS] = T_("Running...");
			} else if(getStatus() == STATUS_FINISHED) {
				display->columns[COLUMN_STATUS] = T_("Finished");
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
		default: dcassert(0); break;
			}
		}

		if(colMask & MASK_PATH) {
			display->columns[COLUMN_PATH] = Text::toT(getPath());
		}

		if(colMask & MASK_ERRORS) {
			tstring tmp;
			for(auto j = getBadSources().begin(); j != getBadSources().end(); ++j) {
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

void QueueFrame::QueueItemInfo::recheck() {
	QueueManager::getInstance()->recheck(getTarget());
}

void QueueFrame::QueueItemInfo::remove() {
	QueueManager::getInstance()->remove(getTarget());
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

int QueueFrame::DirItemInfo::getImage(int col) {
	return col == 0 ? WinUtil::DIR_ICON : -1;
}

int QueueFrame::DirItemInfo::getSelectedImage() {
	return WinUtil::DIR_ICON;
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

		const string& rootStr = dirs->getData(next)->getDir();

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
	dirs->erase(ht);
}

void QueueFrame::removeSelected() {
	if(!BOOLSETTING(CONFIRM_ITEM_REMOVAL) || dwt::MessageBox(this).show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES)
		files->forEachSelected(&QueueItemInfo::remove);
}

void QueueFrame::removeSelectedDir() {
	if(!BOOLSETTING(CONFIRM_ITEM_REMOVAL) || dwt::MessageBox(this).show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES)
		removeDir(dirs->getSelected());
}

void QueueFrame::moveSelected() {
	int n = files->countSelected();
	if(n == 1) {
		// Single file, get the full filename and move...
		QueueItemInfo* ii = files->getSelectedData();
		tstring target = Text::toT(ii->getTarget());

		if(WinUtil::browseSaveFile(this, target)) {
			QueueManager::getInstance()->move(ii->getTarget(), Text::fromT(target));
		}
	} else if(n > 1) {
		tstring name;
		if(BOOLSETTING(QUEUEFRAME_SHOW_TREE)) {
			name = Text::toT(curDir);
		}
		if(FolderDialog(this).open(name)) {
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

	if(FolderDialog(this).open(name)) {
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

	auto p = directories.equal_range(dir);

	for(auto i = p.first; i != p.second; ++i) {
		QueueItemInfo* ii = i->second;
		QueueManager::getInstance()->move(ii->getTarget(), target + Util::getFileName(ii->getTarget()));
	}
}

void QueueFrame::handleGetList(const HintedUser& user) {
	if(files->countSelected() == 1) {
		try {
			int flags = QueueItem::FLAG_CLIENT_VIEW;
			if(!user.user->isSet(User::NMDC)) {
				flags |= QueueItem::FLAG_PARTIAL_LIST;
			}
			QueueManager::getInstance()->addList(user, flags);
		} catch(const Exception&) {
		}
	}
}

void QueueFrame::handleReadd(const HintedUser& user) {

	if(files->countSelected() == 1) {
		QueueItemInfo* ii = files->getSelectedData();

		if(!user.user) {
			// re-add all sources
			for(auto s = ii->getBadSources().begin(); s != ii->getBadSources().end(); ++s) {
				QueueManager::getInstance()->readd(ii->getTarget(), s->getUser());
			}
		} else {
			try {
				QueueManager::getInstance()->readd(ii->getTarget(), user);
			} catch(const Exception& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		}
	}
}

void QueueFrame::handleRemove() {
	usingDirMenu ? removeSelectedDir() : removeSelected();
}

void QueueFrame::handleRecheck() {
	files->forEachSelected(&QueueItemInfo::recheck);
}

void QueueFrame::handleMove() {
	usingDirMenu ? moveSelectedDir() : moveSelected();
}

void QueueFrame::handleRemoveSource(const HintedUser& user) {

	if(files->countSelected() == 1) {
		QueueItemInfo* ii = files->getSelectedData();

		if(!user.user) {
			for(auto si = ii->getSources().begin(); si != ii->getSources().end(); ++si) {
				QueueManager::getInstance()->removeSource(ii->getTarget(), si->getUser(), QueueItem::Source::FLAG_REMOVED);
			}
		} else {
			QueueManager::getInstance()->removeSource(ii->getTarget(), user, QueueItem::Source::FLAG_REMOVED);
		}
	}
}

void QueueFrame::handleRemoveSources(const HintedUser& user) {
	QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
}

void QueueFrame::handlePM(const HintedUser& user) {
	if(files->countSelected() == 1) {
		PrivateFrame::openWindow(getParent(), user);
	}
}

void QueueFrame::handlePriority(QueueItem::Priority p) {
	if(usingDirMenu) {
		setPriority(dirs->getSelected(), p);
	} else {
		std::vector<unsigned> selected = files->getSelection();
		for(auto i = selected.begin(); i != selected.end(); ++i) {
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
	auto dp = directories.equal_range(name);
	for(auto i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->remove(i->second->getTarget());
	}
}

/*
 * @param inc True = increase, False = decrease
 */
void QueueFrame::changePriority(bool inc){
	std::vector<unsigned> selected = files->getSelection();
	for(auto i = selected.begin(); i != selected.end(); ++i) {
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
			default: dcassert(false); break;
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
	auto dp = directories.equal_range(name);
	for(auto i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setPriority(i->second->getTarget(), p);
	}
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
	TreeView_DeleteItem(dirs->handle(), item);
}

const string& QueueFrame::getSelectedDir() {
	DirItemInfo* info = dirs->getSelectedData();
	return info == NULL ? Util::emptyString : info->getDir();
}

const string& QueueFrame::getDir(HTREEITEM item) {
	DirItemInfo* info = dirs->getData(item);
	return info == NULL ? Util::emptyString : info->getDir();
}

MenuPtr QueueFrame::makeSingleMenu(QueueItemInfo* qii) {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	menu->setTitle(escapeMenu(qii->getText(COLUMN_TARGET)), WinUtil::fileImages->getIcon(qii->getImage(0)));

	WinUtil::addHashItems(menu, qii->getTTH(), Text::toT(Util::getFileName(qii->getTarget())), qii->getSize());
	menu->appendItem(T_("&Move/Rename"), [this] { handleMove(); });
	menu->appendItem(T_("Re&check integrity"), [this] { handleRecheck(); });
	addPriorityMenu(menu);
	addBrowseMenu(menu, qii);
	addPMMenu(menu, qii);
	menu->appendSeparator();
	addReaddMenu(menu, qii);
	addRemoveMenu(menu, qii);
	addRemoveSourcesMenu(menu, qii);
	menu->appendItem(T_("&Remove"), [this] { handleRemove(); });
	return menu;
}

MenuPtr QueueFrame::makeMultiMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	size_t sel = files->countSelected();
	menu->setTitle(str(TF_("%1% files") % sel), getParent()->getIcon(this));

	menu->appendItem(T_("&Move/Rename"), [this] { handleMove(); });
	menu->appendItem(T_("Re&check integrity"), [this] { handleRecheck(); });
	addPriorityMenu(menu);
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), [this] { handleRemove(); });
	return menu;
}

MenuPtr QueueFrame::makeDirMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	auto selData = dirs->getSelectedData();
	menu->setTitle(escapeMenu(selData ? selData->getText() : getText()),
		selData ? WinUtil::fileImages->getIcon(selData->getImage(0)) : getParent()->getIcon(this));

	addPriorityMenu(menu);
	menu->appendItem(T_("&Move/Rename"), [this] { handleMove(); });
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), [this] { handleRemove(); });
	return menu;
}

void QueueFrame::addPriorityMenu(const MenuPtr& parent) {
	MenuPtr menu = parent->appendPopup(T_("Set priority"));
	menu->appendItem(T_("Paused"), [this] { handlePriority(QueueItem::PAUSED); });
	menu->appendItem(T_("Lowest"), [this] { handlePriority(QueueItem::LOWEST); });
	menu->appendItem(T_("Low"), [this] { handlePriority(QueueItem::LOW); });
	menu->appendItem(T_("Normal"), [this] { handlePriority(QueueItem::NORMAL); });
	menu->appendItem(T_("High"), [this] { handlePriority(QueueItem::HIGH); });
	menu->appendItem(T_("Highest"), [this] { handlePriority(QueueItem::HIGHEST); });
}

void QueueFrame::addBrowseMenu(const MenuPtr& parent, QueueItemInfo* qii) {
	unsigned int pos = parent->getCount();
	MenuPtr menu = parent->appendPopup(T_("&Get file list"));
	if(!addUsers(menu, &QueueFrame::handleGetList, qii->getSources(), false))
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
	menu->appendItem(T_("All"), [this] { handleReadd(HintedUser(UserPtr(), Util::emptyString)); });
	menu->appendSeparator();
	if(!addUsers(menu, &QueueFrame::handleReadd, qii->getBadSources(), true))
		parent->setItemEnabled(pos, false);
}

void QueueFrame::addRemoveMenu(const MenuPtr& parent, QueueItemInfo* qii) {
	unsigned int pos = parent->getCount();
	MenuPtr menu = parent->appendPopup(T_("Remove source"));
	menu->appendItem(T_("All"), [this] { handleRemoveSource(HintedUser(UserPtr(), Util::emptyString)); });
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

bool QueueFrame::addUsers(const MenuPtr& menu, void (QueueFrame::*handler)(const HintedUser&), const QueueItem::SourceList& sources, bool offline) {
	bool added = false;
	for(auto i = sources.begin(); i != sources.end(); ++i) {
		const QueueItem::Source& source = *i;
		const HintedUser& user = source.getUser();
		if(offline || user.user->isOnline()) {
			menu->appendItem(escapeMenu(WinUtil::getNicks(user)), [=] { (this->*handler)(user); });
			added = true;
		}
	}
	return added;
}

bool QueueFrame::handleFilesContextMenu(dwt::ScreenCoordinate pt) {
	size_t sel = files->countSelected();
	if(sel) {
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

	if(!BOOLSETTING(QUEUEFRAME_SHOW_TREE) || isCurDir(ii->getPath()) ) {
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

	auto i = directories.equal_range(ii->getPath());
	auto j = i.first;
	for(; j != i.second; ++j) {
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

void QueueFrame::onUpdated(QueueItem* qi) {
	QueueItemInfo* ii = getItemInfo(qi->getTarget());

	ii->setPriority(qi->getPriority());
	ii->setStatus(QueueItemInfo::getStatus(*qi));
	ii->setDownloadedBytes(qi->getDownloadedBytes());
	ii->setSources(qi->getSources());
	ii->setBadSources(qi->getBadSources());

	ii->updateMask |= QueueItemInfo::MASK_PRIORITY | QueueItemInfo::MASK_USERS | QueueItemInfo::MASK_ERRORS | QueueItemInfo::MASK_STATUS | QueueItemInfo::MASK_DOWNLOADED;

	if(!BOOLSETTING(QUEUEFRAME_SHOW_TREE) || isCurDir(ii->getPath())) {
		dcassert(files->find(ii) != -1);
		ii->update();
		files->update(ii);
	}

	delete qi;
}

void QueueFrame::onRechecked(const string& target, const tstring& message) {
	callAsync([=] { status->setText(STATUS_STATUS,
		str(TF_("Integrity check: %1% (%2%)") % message % Text::toT(target)), false); });
}

void QueueFrame::on(QueueManagerListener::Added, QueueItem* aQI) noexcept {
	auto qii = new QueueItemInfo(*aQI);
	callAsync([=] { onAdded(qii); });
}

void QueueFrame::on(QueueManagerListener::Removed, QueueItem* aQI) noexcept {
	auto target = aQI->getTarget();
	callAsync([=] { onRemoved(target); });
}

void QueueFrame::on(QueueManagerListener::Moved, QueueItem* aQI, const string& oldTarget) noexcept {
	auto qii = new QueueItemInfo(*aQI);
	callAsync([=] { onRemoved(oldTarget); });
	callAsync([=] { onAdded(qii); });
}

void QueueFrame::on(QueueManagerListener::SourcesUpdated, QueueItem* aQI) noexcept {
	auto qi = new QueueItem(*aQI);
	callAsync([=] { onUpdated(qi); });
}

void QueueFrame::on(QueueManagerListener::RecheckStarted, const string& target) noexcept {
	onRechecked(target, T_("Started..."));
}
void QueueFrame::on(QueueManagerListener::RecheckNoFile, const string& target) noexcept {
	onRechecked(target, T_("Unfinished file not found"));
}
void QueueFrame::on(QueueManagerListener::RecheckFileTooSmall, const string& target) noexcept {
	onRechecked(target, T_("Unfinished file too small"));
}
void QueueFrame::on(QueueManagerListener::RecheckDownloadsRunning, const string& target) noexcept {
	onRechecked(target, T_("Downloads running, please disconnect them"));
}
void QueueFrame::on(QueueManagerListener::RecheckNoTree, const string& target) noexcept {
	onRechecked(target, T_("No full tree available"));
}
void QueueFrame::on(QueueManagerListener::RecheckAlreadyFinished, const string& target) noexcept {
	onRechecked(target, T_("File is already finished"));
}
void QueueFrame::on(QueueManagerListener::RecheckDone, const string& target) noexcept {
	onRechecked(target, T_("Done."));
}
