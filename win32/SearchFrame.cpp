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
#include "resource.h"

#include "SearchFrame.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/ShareManager.h>
#include <set>

static const ColumnInfo resultsColumns[] = {
	{ N_("File"), 200, false },
	{ N_("User"), 100, false },
	{ N_("Type"), 60, false },
	{ N_("Size"), 80, true },
	{ N_("Path"), 100, false },
	{ N_("Slots"), 40, true },
	{ N_("Connection"), 70, false },
	{ N_("Hub"), 150, false },
	{ N_("Exact size"), 100, true },
	{ N_("IP"), 100, false },
	{ N_("TTH"), 300, false },
	{ N_("CID"), 300, false }
};

TStringList SearchFrame::lastSearches;

SearchFrame::FrameSet SearchFrame::frames;

int SearchFrame::SearchInfo::getImage() {
	const SearchResultPtr& sr = srs[0];
	return sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getIconIndex(Text::toT(sr->getFile())) : WinUtil::getDirIconIndex();
}

int SearchFrame::SearchInfo::compareItems(SearchInfo* a, SearchInfo* b, int col) {

	switch(col) {
	case COLUMN_NICK:
		if(a->srs.size() > 1 && b->srs.size() > 1)
			return compare(a->srs.size(), b->srs.size());
		else if (a->srs.size() > 1 || b->srs.size() > 1)
			return(a->srs.size() > 1) ? -1 : 1;
		else
			return lstrcmpi(a->columns[COLUMN_NICK].c_str(), b->columns[COLUMN_NICK].c_str());
	case COLUMN_TYPE:
		if(a->srs[0]->getType() == b->srs[0]->getType())
			return lstrcmpi(a->columns[COLUMN_TYPE].c_str(), b->columns[COLUMN_TYPE].c_str());
		else
			return(a->srs[0]->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
	case COLUMN_SLOTS:
		if(a->srs[0]->getFreeSlots() == b->srs[0]->getFreeSlots())
			return compare(a->srs[0]->getSlots(), b->srs[0]->getSlots());
		else
			return compare(a->srs[0]->getFreeSlots(), b->srs[0]->getFreeSlots());
	case COLUMN_SIZE:
	case COLUMN_EXACT_SIZE: return compare(a->srs[0]->getSize(), b->srs[0]->getSize());
	default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
	}
}


void SearchFrame::openWindow(dwt::TabView* mdiParent, const tstring& str, SearchManager::TypeModes type) {
	SearchFrame* pChild = new SearchFrame(mdiParent, str, type);
	frames.insert(pChild);
}

void SearchFrame::closeAll() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		(*i)->close(true);
}

SearchFrame::SearchFrame(dwt::TabView* mdiParent, const tstring& initialString, SearchManager::TypeModes initialType_) :
	BaseType(mdiParent, T_("Search"), IDH_SEARCH, IDR_SEARCH),
	searchLabel(0),
	searchBox(0),
	purge(0),
	doSearch(0),
	sizeLabel(0),
	mode(0),
	size(0),
	sizeMode(0),
	typeLabel(0),
	fileType(0),
	optionLabel(0),
	slots(0),
	onlyFree(BOOLSETTING(SEARCH_ONLY_FREE_SLOTS)),
	filter(0),
	filterShared(BOOLSETTING(SEARCH_FILTER_SHARED)),
	hubsLabel(0),
	hubs(0),
	results(0),
	showUI(0),
	bShowUI(true),
	isHash(false),
	initialType(initialType_),
	droppedResults(0)
{
	{
		Label::Seed cs;
		cs.exStyle = WS_EX_TRANSPARENT;

		cs.caption = T_("Search for");
		searchLabel = addChild(cs);
		searchLabel->setHelpId(IDH_SEARCH_SEARCH_FOR);

		cs.caption = T_("Size");
		sizeLabel = addChild(cs);
		sizeLabel->setHelpId(IDH_SEARCH_SIZE);

		cs.caption = T_("File type");
		typeLabel = addChild(cs);
		typeLabel->setHelpId(IDH_SEARCH_TYPE);

		cs.caption = T_("Search options");
		optionLabel = addChild(cs);

		cs.caption = T_("Hubs");
		hubsLabel = addChild(cs);
		hubsLabel->setHelpId(IDH_SEARCH_HUBS);
	}

	{
		searchBox = addChild(WinUtil::Seeds::comboBoxEdit);
		searchBox->setHelpId(IDH_SEARCH_SEARCH_FOR);
		addWidget(searchBox);

		for(TStringIter i = lastSearches.begin(); i != lastSearches.end(); ++i) {
			searchBox->insertValue(0, *i);
		}
		searchBox->getTextBox()->onKeyDown(std::tr1::bind(&SearchFrame::handleSearchKeyDown, this, _1));
		searchBox->getTextBox()->onChar(std::tr1::bind(&SearchFrame::handleSearchChar, this, _1));
	}

	{
		Button::Seed cs = WinUtil::Seeds::button;
		cs.caption = T_("Purge");
		purge = addChild(cs);
		purge->setHelpId(IDH_SEARCH_PURGE);
		purge->onClicked(std::tr1::bind(&SearchFrame::handlePurgeClicked, this));

		cs.style |= BS_DEFPUSHBUTTON;
		cs.caption = T_("Search");
		doSearch = addChild(cs);
		doSearch->setHelpId(IDH_SEARCH_SEARCH);
		doSearch->onClicked(std::tr1::bind(&SearchFrame::runSearch, this));
	}

	{
		mode = addChild(WinUtil::Seeds::comboBoxStatic);
		mode->setHelpId(IDH_SEARCH_SIZE);
		addWidget(mode);

		mode->addValue(T_("Normal"));
		mode->addValue(T_("At least"));
		mode->addValue(T_("At most"));
	}

	{
		TextBox::Seed cs = WinUtil::Seeds::textBox;
		cs.style |= ES_AUTOHSCROLL | ES_NUMBER;
		size = addChild(cs);
		size->setHelpId(IDH_SEARCH_SIZE);
		addWidget(size);
	}

	{
		sizeMode = addChild(WinUtil::Seeds::comboBoxStatic);
		sizeMode->setHelpId(IDH_SEARCH_SIZE);
		addWidget(sizeMode);

		sizeMode->addValue(T_("B"));
		sizeMode->addValue(T_("KiB"));
		sizeMode->addValue(T_("MiB"));
		sizeMode->addValue(T_("GiB"));
		sizeMode->setSelected(2);
	}

	{
		fileType = addChild(WinUtil::Seeds::comboBoxStatic);
		fileType->setHelpId(IDH_SEARCH_TYPE);
		addWidget(fileType);

		fileType->addValue(T_("Any"));
		fileType->addValue(T_("Audio"));
		fileType->addValue(T_("Compressed"));
		fileType->addValue(T_("Document"));
		fileType->addValue(T_("Executable"));
		fileType->addValue(T_("Picture"));
		fileType->addValue(T_("Video"));
		fileType->addValue(T_("Directory"));
		fileType->addValue(T_("TTH"));
	}

	{
		CheckBox::Seed cs(T_("Only users with free slots"));
		slots = addChild(cs);
		slots->setHelpId(IDH_SEARCH_SLOTS);
		slots->setChecked(onlyFree);
		slots->onClicked(std::tr1::bind(&SearchFrame::handleSlotsClicked, this)) ;

		cs.caption = T_("Hide files already in share");
		filter = addChild(cs);
		filter->setHelpId(IDH_SEARCH_SHARE);
		filter->setChecked(filterShared);
		filter->onClicked(std::tr1::bind(&SearchFrame::handleFilterClicked, this)) ;
	}

	{
		WidgetHubs::Seed cs;
		cs.style |= LVS_NOCOLUMNHEADER;
		cs.lvStyle |= LVS_EX_CHECKBOXES;
		hubs = addChild(cs);
		hubs->setHelpId(IDH_SEARCH_HUBS);
		addWidget(hubs);

		TStringList dummy;
		dummy.push_back(Util::emptyStringT);
		hubs->createColumns(dummy);

		hubs->onRaw(std::tr1::bind(&SearchFrame::handleHubItemChanged, this, _1, _2), dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));

		hubs->insert(new HubInfo(Util::emptyStringT, T_("Only where I'm op"), false));
		hubs->setChecked(0, false);
	}

	{
		results = addChild(WidgetResults::Seed());
		addWidget(results);

		results->setSmallImageList(WinUtil::fileImages);
		WinUtil::makeColumns(results, resultsColumns, COLUMN_LAST, SETTING(SEARCHFRAME_ORDER), SETTING(SEARCHFRAME_WIDTHS));

		results->onDblClicked(std::tr1::bind(&SearchFrame::handleDoubleClick, this));
		results->onKeyDown(std::tr1::bind(&SearchFrame::handleKeyDown, this, _1));
		results->onContextMenu(std::tr1::bind(&SearchFrame::handleContextMenu, this, _1));
	}

	{
		CheckBox::Seed cs(_T("+/-"));
		cs.style &= ~WS_TABSTOP;
		showUI = addChild(cs);
		showUI->setChecked(bShowUI);

		showUI->onClicked(std::tr1::bind(&SearchFrame::handleShowUIClicked, this));
	}

	initStatus();

	statusSizes[STATUS_SHOW_UI] = 16; ///@todo get real checkbox width

	layout();

	ClientManager* clientMgr = ClientManager::getInstance();
	clientMgr->lock();
	clientMgr->addListener(this);
	Client::List& clients = clientMgr->getClients();
	for(Client::List::iterator it = clients.begin(); it != clients.end(); ++it) {
		Client* client = *it;
		if(!client->isConnected())
			continue;

		onHubAdded(new HubInfo(client));
	}
	clientMgr->unlock();

	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);

	SearchManager::getInstance()->addListener(this);

	if(!initialString.empty()) {
		lastSearches.push_back(initialString);
		searchBox->insertValue(0, initialString);
		searchBox->setSelected(0);
		fileType->setSelected(initialType);
		runSearch();
	} else {
		mode->setSelected(1);
		fileType->setSelected(SETTING(LAST_SEARCH_TYPE));
	}
	searchBox->setFocus();
}

SearchFrame::~SearchFrame() {
}

void SearchFrame::layout() {
	dwt::Rectangle r(getClientAreaSize());

	layoutStatus(r);
	mapWidget(STATUS_SHOW_UI, showUI);

	if(showUI->getChecked()) {

		const long width = 220, labelH = 16, margin = 4, spacing = 2, groupSpacing = 4;

		dwt::Rectangle rect = r;

		results->setBounds(rect.getRight(rect.width() - width));

		rect.size.x = width;

		long yedit = size->getTextSize(_T("A")).y + 8;
		long comboH = 5 * yedit;

		rect.pos.x += margin;
		rect.size.x -= margin * 2;

		rect.size.y = labelH;

		// "Search for"
		searchLabel->setBounds(rect);

		rect.pos.y += rect.size.y + spacing;
		rect.size.y = comboH;

		searchBox->setBounds(rect);

		rect.pos.y += yedit + spacing;

		purge->setBounds(dwt::Rectangle(rect.x(), rect.y(), 75, yedit));
		doSearch->setBounds(dwt::Rectangle(rect.right() - 100, rect.y(), 100, yedit));

		rect.pos.y += yedit + groupSpacing;

		rect.size.y = labelH;
		sizeLabel->setBounds(rect);

		rect.pos.y += rect.size.y + spacing;

		long w2 = rect.size.x - 2 * spacing;

		dwt::Rectangle third = rect;

		third.size.y = comboH;
		third.size.x = w2 / 3;

		mode->setBounds(third);

		third.pos.x += third.size.x + spacing;
		third.size.y = yedit;

		size->setBounds(third);

		third.pos.x += third.size.x + spacing;
		third.size.y = comboH;

		sizeMode->setBounds(third);

		rect.pos.y += yedit + groupSpacing;
		rect.size.y = labelH;

		typeLabel->setBounds(rect);

		rect.pos.y += rect.size.y + spacing;
		rect.size.y = comboH;
		fileType->setBounds(rect);

		rect.pos.y += yedit + groupSpacing;
		rect.size.y = labelH;

		optionLabel->setBounds(rect);

		rect.pos.y += rect.size.y + spacing;
		rect.size.y = yedit;
		slots->setBounds(rect);

		rect.pos.y += rect.size.y + spacing;
		rect.size.y = yedit;
		filter->setBounds(rect);

		rect.pos.y += rect.size.y + groupSpacing;
		rect.size.y = labelH;
		hubsLabel->setBounds(rect);

		rect.pos.y += rect.size.y + spacing;
		rect.size.y = std::min(yedit * 5, r.size.y - rect.pos.y - margin * 2);
		hubs->setBounds(rect);
	} else {
		results->setBounds(r);
	}
}

bool SearchFrame::preClosing() {
	SearchManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);

	frames.erase(this);

	return true;
}

void SearchFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::SEARCHFRAME_ORDER, WinUtil::toString(results->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::SEARCHFRAME_WIDTHS, WinUtil::toString(results->getColumnWidths()));
}

void SearchFrame::SearchInfo::view() {
	try {
		if(srs[0]->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(Util::getTempPath() + srs[0]->getFileName(),
				srs[0]->getSize(), srs[0]->getTTH(), srs[0]->getUser(),
				QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	try {
		if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt + si->columns[COLUMN_FILENAME]);
			for(SearchResultList::const_iterator i = si->srs.begin(); i != si->srs.end(); ++i) {
				const SearchResultPtr& sr = *i;
				try {
					QueueManager::getInstance()->add(target, sr->getSize(),
						sr->getTTH(), sr->getUser());
				} catch(const QueueException&) {} 
			}
			if(WinUtil::isShift())
				QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->srs[0]->getFile(), si->srs[0]->getUser(), Text::fromT(tgt),
				WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
	try {
		// TODO Add all users...
		QueueItem::Priority prio = WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT;
		if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->addDirectory(Text::fromT(si->columns[COLUMN_PATH]),
				si->srs[0]->getUser(), Text::fromT(tgt), prio);
		} else {
			QueueManager::getInstance()->addDirectory(si->srs[0]->getFile(), si->srs[0]->getUser(),
				Text::fromT(tgt), prio);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
	try {
		if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt);
			for(SearchResultList::const_iterator i = si->srs.begin(); i != si->srs.end(); ++i) {
				const SearchResultPtr& sr = *i;
				try {
					QueueManager::getInstance()->add(target, sr->getSize(),
						sr->getTTH(), sr->getUser());
				} catch(const QueueException&) {} 
			}

			if(WinUtil::isShift())
				QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->srs[0]->getFile(), si->srs[0]->getUser(), Text::fromT(tgt),
				WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::CheckTTH::operator()(SearchInfo* si) {
	if(firstTTH) {
		tth = si->columns[COLUMN_TTH];
		hasTTH = true;
		firstTTH = false;
	} else if(hasTTH) {
		if(tth != si->columns[COLUMN_TTH]) {
			hasTTH = false;
		}
	}

	if(firstHubs && hubs.empty()) {
		hubs = ClientManager::getInstance()->getHubs(si->srs[0]->getUser()->getCID());
		firstHubs = false;
	} else if(!hubs.empty()) {
		Util::intersect(hubs, ClientManager::getInstance()->getHubs(si->srs[0]->getUser()->getCID()));
	}
}

SearchFrame::SearchInfo::SearchInfo(const SearchResultPtr& aSR) {
	srs.push_back(aSR);
	update();
}

void SearchFrame::SearchInfo::update() {
	// TODO Use most common value, not the first one...
	const SearchResultPtr& sr = srs[0];
	if(sr->getType() == SearchResult::TYPE_FILE) {
		if(sr->getFile().rfind(_T('\\')) == tstring::npos) {
			columns[COLUMN_FILENAME] = Text::toT(sr->getFile());
		} else {
			columns[COLUMN_FILENAME] = Text::toT(Util::getFileName(sr->getFile()));
			columns[COLUMN_PATH] = Text::toT(Util::getFilePath(sr->getFile()));
		}

		columns[COLUMN_TYPE] = Text::toT(Util::getFileExt(Text::fromT(columns[COLUMN_FILENAME])));
		if(!columns[COLUMN_TYPE].empty() && columns[COLUMN_TYPE][0] == _T('.'))
			columns[COLUMN_TYPE].erase(0, 1);
		columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(sr->getSize()));
		columns[COLUMN_EXACT_SIZE] = Text::toT(Util::formatExactSize(sr->getSize()));
        columns[COLUMN_TTH] = Text::toT(sr->getTTH().toBase32());
	} else {
		columns[COLUMN_FILENAME] = Text::toT(sr->getFileName());
		columns[COLUMN_PATH] = Text::toT(sr->getFile());
		columns[COLUMN_TYPE] = T_("Directory");
		if(sr->getSize() > 0) {
			columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(sr->getSize()));
			columns[COLUMN_EXACT_SIZE] = Text::toT(Util::formatExactSize(sr->getSize()));
		}
	}
	if(srs.size() > 1) {
		columns[COLUMN_NICK] = str(TFN_("%1% user", "%1% users", srs.size()) % srs.size());
		columns[COLUMN_CONNECTION].clear();
		columns[COLUMN_IP].clear();
		columns[COLUMN_CID].clear();

		std::set<std::string> hubs;
		for(SearchResultList::const_iterator i = srs.begin(), iend = srs.end(); i != iend; ++i) {
			hubs.insert((*i)->getHubName());
		}
		columns[COLUMN_HUB] = Text::toT(Util::toString(StringList(hubs.begin(), hubs.end())));
	} else {
		columns[COLUMN_NICK] = WinUtil::getNicks(sr->getUser());
		columns[COLUMN_CONNECTION] = Text::toT(ClientManager::getInstance()->getConnection(sr->getUser()->getCID()));
		columns[COLUMN_SLOTS] = Text::toT(sr->getSlotString());
		columns[COLUMN_IP] = Text::toT(sr->getIP());
		if (!columns[COLUMN_IP].empty()) {
			// Only attempt to grab a country mapping if we actually have an IP address
			tstring tmpCountry = Text::toT(Util::getIpCountry(sr->getIP()));
			if(!tmpCountry.empty())
				columns[COLUMN_IP] = tmpCountry + _T(" (") + columns[COLUMN_IP] + _T(")");
		}
		columns[COLUMN_HUB] = Text::toT(sr->getHubName());
		columns[COLUMN_CID] = Text::toT(sr->getUser()->getCID().toBase32());
	}
}

void SearchFrame::addResult(SearchInfo* si) {
	bool added = false;
	// Newly added ones always have just one result - we combine here
	dcassert(si->srs.size() == 1);
	const SearchResultPtr& sr = si->srs[0];
	// Check previous search results for dupes
	for(int i = 0, iend = results->size(); !added && i < iend; ++i) {
		SearchInfo* si2 = results->getData(i);
		for(SearchResultList::iterator j = si2->srs.begin(), jend = si2->srs.end(); j != jend; ++j) {
			SearchResultPtr& sr2 = *j;
			if((sr->getUser()->getCID() == sr2->getUser()->getCID()) && (sr->getFile() == sr2->getFile())) {
				delete si;
				return;
			} else if(sr->getType() == SearchResult::TYPE_FILE && sr2->getType() == SearchResult::TYPE_FILE && sr->getTTH() == sr2->getTTH()) {
				if(sr->getSize() != sr2->getSize()) {
					delete si;
					return;
				}
				si2->srs.push_back(sr);
				si2->update();
				delete si;
				added = true;
				results->update(i);
				break;
			}
		}
	}

	if(!added) {
		results->insert(si);
	}
	setStatus(STATUS_COUNT, str(TFN_("%1% item", "%1% items", results->size()) % results->size()));
	setDirty(SettingsManager::BOLD_SEARCH);
}

void SearchFrame::handlePurgeClicked() {
	searchBox->clear();
	lastSearches.clear();
}

void SearchFrame::handleSlotsClicked() {
	onlyFree = slots->getChecked();
}

void SearchFrame::handleFilterClicked() {
	filterShared = filter->getChecked();
}

void SearchFrame::handleShowUIClicked() {
	bShowUI = showUI->getChecked();

	searchLabel->setVisible(bShowUI);
	searchBox->setVisible(bShowUI);
	purge->setVisible(bShowUI);
	doSearch->setVisible(bShowUI);
	sizeLabel->setVisible(bShowUI);
	mode->setVisible(bShowUI);
	size->setVisible(bShowUI);
	sizeMode->setVisible(bShowUI);
	typeLabel->setVisible(bShowUI);
	fileType->setVisible(bShowUI);
	optionLabel->setVisible(bShowUI);
	slots->setVisible(bShowUI);
	hubsLabel->setVisible(bShowUI);
	hubs->setVisible(bShowUI);

	layout();
}

LRESULT SearchFrame::handleHubItemChanged(WPARAM wParam, LPARAM lParam) {
	LPNMLISTVIEW lv = (LPNMLISTVIEW)lParam;
	if(lv->iItem == 0 && (lv->uNewState ^ lv->uOldState) & LVIS_STATEIMAGEMASK) {
		if (((lv->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) {
			for(int iItem = 0; (iItem = hubs->getNext(iItem, LVNI_ALL)) != -1; ) {
				HubInfo* client = hubs->getData(iItem);
				if (!client->op)
					hubs->setChecked(iItem, false);
			}
		}
	}
	return 0;
}

void SearchFrame::handleDoubleClick() {
	results->forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
}

bool SearchFrame::handleKeyDown(int c) {
	if(c == VK_DELETE) {
		handleRemove();
		return true;
	}
	return false;
}

bool SearchFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(results->countSelected() > 0) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = results->getContextMenuPos();
		}

		MenuPtr contextMenu = makeMenu();
		contextMenu->open(pt);
		return true;
	}
	return false;
}

void SearchFrame::handleDownload() {
	results->forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
}

void SearchFrame::handleDownloadFavoriteDirs(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if(index < spl.size()) {
		results->forEachSelectedT(SearchInfo::Download(Text::toT(spl[index].first)));
	} else {
		dcassert((index - spl.size()) < targets.size());
		results->forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[index - spl.size()])));
	}
}

void SearchFrame::handleDownloadTo() {
	if(results->countSelected() == 1) {
		int i = results->getNext(-1, LVNI_SELECTED);
		dcassert(i != -1);
		SearchInfo* si = results->getData(i);
		const SearchResultPtr& sr = si->srs[0];

		if(sr->getType() == SearchResult::TYPE_FILE) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + si->columns[COLUMN_FILENAME];
			if(WinUtil::browseSaveFile(createSaveDialog(), target)) {
				WinUtil::addLastDir(Util::getFilePath(target));
				results->forEachSelectedT(SearchInfo::DownloadTarget(target));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(createFolderDialog().open(target)) {
				WinUtil::addLastDir(target);
				results->forEachSelectedT(SearchInfo::Download(target));
			}
		}
	} else {
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(createFolderDialog().open(target)) {
			WinUtil::addLastDir(target);
			results->forEachSelectedT(SearchInfo::Download(target));
		}
	}
}

void SearchFrame::handleDownloadTarget(unsigned index) {
	if(index < WinUtil::lastDirs.size()) {
		results->forEachSelectedT(SearchInfo::Download(WinUtil::lastDirs[index]));
	} else {
		dcassert((index - WinUtil::lastDirs.size()) < targets.size());
		results->forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[index - WinUtil::lastDirs.size()])));
	}
}

void SearchFrame::handleDownloadDir() {
	results->forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
}

void SearchFrame::handleDownloadWholeFavoriteDirs(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	dcassert(index < spl.size());
	results->forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(spl[index].first)));
}

void SearchFrame::handleDownloadWholeTarget(unsigned index) {
	dcassert(index < WinUtil::lastDirs.size());
	results->forEachSelectedT(SearchInfo::DownloadWhole(WinUtil::lastDirs[index]));
}

void SearchFrame::handleDownloadDirTo() {
	tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
	if(createFolderDialog().open(target)) {
		WinUtil::addLastDir(target);
		results->forEachSelectedT(SearchInfo::DownloadWhole(target));
	}
}

void SearchFrame::handleViewAsText() {
	results->forEachSelected(&SearchInfo::view);
}

void SearchFrame::handleRemove() {
	int i = -1;
	while((i = results->getNext(-1, LVNI_SELECTED)) != -1) {
		results->erase(i);
	}
}

struct UserCollector {
	template<typename T>
	void operator()(T* si) {
		for(SearchResultList::const_iterator i = si->srs.begin(), iend = si->srs.end(); i != iend; ++i) {
			const SearchResultPtr& sr = *i;
			if(std::find(users.begin(), users.end(), sr->getUser()) == users.end())
				users.push_back(sr->getUser());
		}
	}
	UserList users;
};

SearchFrame::MenuPtr SearchFrame::makeMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	StringPairList favoriteDirs = FavoriteManager::getInstance()->getFavoriteDirs();
	SearchInfo::CheckTTH checkTTH = results->forEachSelectedT(SearchInfo::CheckTTH());

	menu->appendItem(T_("&Download"), std::tr1::bind(&SearchFrame::handleDownload, this), dwt::BitmapPtr(), true, true);
	addTargetMenu(menu, favoriteDirs, checkTTH);
	menu->appendItem(T_("Download whole directory"), std::tr1::bind(&SearchFrame::handleDownloadDir, this));
	addTargetDirMenu(menu, favoriteDirs);
	menu->appendItem(T_("&View as text"), std::tr1::bind(&SearchFrame::handleViewAsText, this));
	menu->appendSeparator();
	SearchInfo* si = results->getSelectedData();
	if(checkTTH.hasTTH) {
		WinUtil::addHashItems(menu, TTHValue(Text::fromT(checkTTH.tth)), si->getText(COLUMN_FILENAME));
	}
	menu->appendSeparator();

	UserCollector users = results->forEachSelectedT(UserCollector());
	WinUtil::addUserItems(menu, users.users, getParent(), si ? Text::fromT(si->getText(COLUMN_PATH)) : "");

	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), std::tr1::bind(&SearchFrame::handleRemove, this));
	prepareMenu(menu, UserCommand::CONTEXT_SEARCH, checkTTH.hubs);

	return menu;
}

void SearchFrame::addTargetMenu(const MenuPtr& parent, const StringPairList& favoriteDirs, const SearchInfo::CheckTTH& checkTTH) {
	MenuPtr menu = parent->appendPopup(T_("Download to..."));

	int n = 0;
	if(favoriteDirs.size() > 0) {
		for(StringPairList::const_iterator i = favoriteDirs.begin(); i != favoriteDirs.end(); i++)
			menu->appendItem(Text::toT(i->second), std::tr1::bind(&SearchFrame::handleDownloadFavoriteDirs, this, n++));
		menu->appendSeparator();
	}

	n = 0;
	menu->appendItem(T_("&Browse..."), std::tr1::bind(&SearchFrame::handleDownloadTo, this));
	if(WinUtil::lastDirs.size() > 0) {
		menu->appendSeparator();
		for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i)
			menu->appendItem(*i, std::tr1::bind(&SearchFrame::handleDownloadTarget, this, n++));
	}

	if(checkTTH.hasTTH) {
		targets.clear();

		QueueManager::getInstance()->getTargets(TTHValue(Text::fromT(checkTTH.tth)), targets);
		if(targets.size() > 0) {
			menu->appendSeparator();
			for(StringIter i = targets.begin(); i != targets.end(); ++i)
				menu->appendItem(Text::toT(*i), std::tr1::bind(&SearchFrame::handleDownloadTarget, this, n++));
		}
	}
}

void SearchFrame::addTargetDirMenu(const MenuPtr& parent, const StringPairList& favoriteDirs) {
	MenuPtr menu = parent->appendPopup(T_("Download whole directory to..."));

	int n = 0;
	if(favoriteDirs.size() > 0) {
		for(StringPairList::const_iterator i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i)
			menu->appendItem(Text::toT(i->second), std::tr1::bind(&SearchFrame::handleDownloadWholeFavoriteDirs, this, n++));
		menu->appendSeparator();
	}

	n = 0;
	menu->appendItem(T_("&Browse..."), std::tr1::bind(&SearchFrame::handleDownloadDirTo, this));
	if(WinUtil::lastDirs.size() > 0) {
		menu->appendSeparator();
		for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i)
			menu->appendItem(*i, std::tr1::bind(&SearchFrame::handleDownloadWholeTarget, this, n++));
	}
}

void SearchFrame::on(SearchManagerListener::SR, const SearchResultPtr& aResult) throw() {
	// Check that this is really a relevant search result...
	{
		Lock l(cs);

		if(currentSearch.empty()) {
			return;
		}

		if(!aResult->getToken().empty() && token != aResult->getToken()) {
			droppedResults++;
			dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::updateStatusFiltered, this));
			return;
		}

		if(isHash) {
			if(aResult->getType() != SearchResult::TYPE_FILE || TTHValue(Text::fromT(currentSearch[0])) != aResult->getTTH()) {
				droppedResults++;
				dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::updateStatusFiltered, this));
				return;
			}
		} else {
			// match all here
			for(TStringIter j = currentSearch.begin(); j != currentSearch.end(); ++j) {
				if((*j->begin() != _T('-') && Util::findSubString(aResult->getFile(), Text::fromT(*j)) == tstring::npos) ||
					(*j->begin() == _T('-') && j->size() != 1 && Util::findSubString(aResult->getFile(), Text::fromT(j->substr(1))) != tstring::npos)
					)
				{
					droppedResults++;
					dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::updateStatusFiltered, this));
					return;
				}
			}
		}
	}

	// Filter already shared files
	if( filterShared ) {
		const TTHValue& t = aResult->getTTH();
		if( ShareManager::getInstance()->isTTHShared(t) ) {
			droppedResults++;
			dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::updateStatusFiltered, this));
			return;
		}
	}

	// Reject results without free slots
	if((onlyFree && aResult->getFreeSlots() < 1))
	{
		droppedResults++;
		dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::updateStatusFiltered, this));
		return;
	}

	dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::addResult, this, new SearchInfo(aResult)));
}

void SearchFrame::onHubAdded(HubInfo* info) {
	int nItem = hubs->insert(info);
	hubs->setChecked(nItem, (hubs->isChecked(0) ? info->op : true));
	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubChanged(HubInfo* info) {
	int nItem = 0;
	int n = hubs->size();
	for(; nItem < n; nItem++) {
		if(hubs->getData(nItem)->url == info->url)
			break;
	}
	if (nItem == n)
		return;

	hubs->setData(nItem, info);
	hubs->update(nItem);

	if (hubs->isChecked(0))
		hubs->setChecked(nItem, info->op);

	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubRemoved(HubInfo* info) {
	int nItem = 0;
	int n = hubs->size();
	for(; nItem < n; nItem++) {
		if(hubs->getData(nItem)->url == info->url)
			break;
	}
	if (nItem == n)
		return;

	hubs->erase(nItem);
	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::runSearch() {
	StringList clients;

	// Change Default Settings If Changed
	if (onlyFree != BOOLSETTING(SEARCH_ONLY_FREE_SLOTS))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_ONLY_FREE_SLOTS, onlyFree);
	if (filterShared != BOOLSETTING(SEARCH_FILTER_SHARED))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_FILTER_SHARED, filterShared);
	if (!initialType && fileType->getSelected() != SETTING(LAST_SEARCH_TYPE))
		SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_TYPE, fileType->getSelected());

	tstring s = searchBox->getText();

	if(s.empty())
		return;

	int n = hubs->size();
	for(int i = 0; i < n; i++) {
		if(hubs->isChecked(i)) {
			clients.push_back(Text::fromT(hubs->getData(i)->url));
		}
	}

	if(clients.empty())
		return;

	tstring tsize = size->getText();

	double lsize = Util::toDouble(Text::fromT(tsize));
	switch(sizeMode->getSelected()) {
	case 1:
		lsize*=1024.0; break;
	case 2:
		lsize*=1024.0*1024.0; break;
	case 3:
		lsize*=1024.0*1024.0*1024.0; break;
	}

	int64_t llsize = (int64_t)lsize;

	results->clear();

	// Add new searches to the last-search dropdown list
	if(find(lastSearches.begin(), lastSearches.end(), s) == lastSearches.end())
	{
		int i = max(SETTING(SEARCH_HISTORY)-1, 0);

		if(searchBox->size() > i)
			searchBox->erase(i);
		searchBox->insertValue(0, s);

		while(lastSearches.size() > (TStringList::size_type)i) {
			lastSearches.erase(lastSearches.begin());
		}
		lastSearches.push_back(s);
	}

	{
		Lock l(cs);
		currentSearch = StringTokenizer<tstring>(s, ' ').getTokens();
		s.clear();
		//strip out terms beginning with -
		for(TStringList::iterator si = currentSearch.begin(); si != currentSearch.end(); ) {
			if(si->empty()) {
				si = currentSearch.erase(si);
				continue;
			}
			if ((*si)[0] != _T('-'))
				s += *si + _T(' ');
			++si;
		}

		s = s.substr(0, max(s.size(), static_cast<tstring::size_type>(1)) - 1);
		token = Util::toString(Util::rand());
	}

	SearchManager::SizeModes searchMode((SearchManager::SizeModes)mode->getSelected());
	if(llsize == 0)
		searchMode = SearchManager::SIZE_DONTCARE;

	int ftype = fileType->getSelected();

	setStatus(STATUS_STATUS, str(TF_("Searching for %1%...") % s));
	setStatus(STATUS_COUNT, Util::emptyStringT);
	setStatus(STATUS_FILTERED, Util::emptyStringT);
	droppedResults = 0;
	isHash = (ftype == SearchManager::TYPE_TTH);

	setText(str(TF_("Search - %1%") % s));

	if(SearchManager::getInstance()->okToSearch()) {
		SearchManager::getInstance()->search(clients, Text::fromT(s), llsize,
			(SearchManager::TypeModes)ftype, searchMode, token);
		if(BOOLSETTING(CLEAR_SEARCH)) // Only clear if the search was sent
			searchBox->setText(Util::emptyStringT);
	} else {
		int32_t waitFor = SearchManager::getInstance()->timeToSearch();
		tstring msg = str(TFN_("Searching too soon, next search in %1% second", "Searching too soon, next search in %1% seconds", waitFor) % waitFor);

		setStatus(STATUS_STATUS, msg);
		setStatus(STATUS_COUNT, Util::emptyStringT);
		setStatus(STATUS_FILTERED, Util::emptyStringT);

		setText(str(TF_("Search - %1%") % msg));
		// Start the countdown timer
		initSecond();
	}
}

void SearchFrame::updateStatusFiltered() {
	setStatus(STATUS_FILTERED, str(TF_("%1% filtered") % droppedResults));
}

void SearchFrame::initSecond() {
	createTimer(std::tr1::bind(&SearchFrame::eachSecond, this), 1000);
}

bool SearchFrame::eachSecond() {
	int32_t waitFor = SearchManager::getInstance()->timeToSearch();
	if(waitFor > 0) {
		tstring msg = str(TFN_("Searching too soon, next search in %1% second", "Searching too soon, next search in %1% seconds", waitFor) % waitFor);
		setStatus(STATUS_STATUS, msg);
		setText(str(TF_("Search - %1%") % msg));
		return true;
	}

	setStatus(STATUS_STATUS, T_("Ready to search..."));
	setText(T_("Search - Ready to search..."));

	return false;
}

void SearchFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = results->getNext(sel, LVNI_SELECTED)) != -1) {
		const SearchResultPtr& sr = results->getData(sel)->srs[0];

		if(!sr->getUser()->isOnline())
			continue;

		if(uc.getType() == UserCommand::TYPE_RAW_ONCE) {
			if(users.find(sr->getUser()->getCID()) != users.end())
				continue;
			users.insert(sr->getUser()->getCID());
		}

		ucParams["fileFN"] = sr->getFile();
		ucParams["fileSI"] = Util::toString(sr->getSize());
		ucParams["fileSIshort"] = Util::formatBytes(sr->getSize());
		if(sr->getType() == SearchResult::TYPE_FILE) {
			ucParams["fileTR"] = sr->getTTH().toBase32();
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		StringMap tmp = ucParams;
		ClientManager::getInstance()->userCommand(sr->getUser(), uc, tmp, true);
	}
}

bool SearchFrame::handleSearchKeyDown(int c) {
	if(c == VK_RETURN && !(WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt())) {
		runSearch();
		return true;
	}
	return false;
}

bool SearchFrame::handleSearchChar(int c) {
	// avoid the "beep" sound when enter is pressed
	return c == VK_RETURN;
}

void SearchFrame::on(ClientConnected, Client* c) throw() {
	dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::onHubAdded, this, new HubInfo(c)));
}

void SearchFrame::on(ClientUpdated, Client* c) throw() {
	dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::onHubChanged, this, new HubInfo(c)));
}

void SearchFrame::on(ClientDisconnected, Client* c) throw() {
	dwt::Application::instance().callAsync(std::tr1::bind(&SearchFrame::onHubRemoved, this, new HubInfo(c)));
}
