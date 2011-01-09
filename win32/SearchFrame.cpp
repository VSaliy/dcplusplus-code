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
#include "resource.h"

#include "SearchFrame.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/ShareManager.h>
#include <set>

const string SearchFrame::id = "Search";
const string& SearchFrame::getId() const { return id; }

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
	return sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getFileIcon(Text::toT(sr->getFile())) : WinUtil::DIR_ICON;
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


void SearchFrame::openWindow(TabViewPtr parent, const tstring& str, SearchManager::TypeModes type) {
	SearchFrame* pChild = new SearchFrame(parent, str, type);
	frames.insert(pChild);
}

void SearchFrame::closeAll() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		(*i)->close(true);
}

SearchFrame::SearchFrame(TabViewPtr parent, const tstring& initialString, SearchManager::TypeModes initialType_) :
BaseType(parent, T_("Search"), IDH_SEARCH, IDI_SEARCH),
paned(0),
options(0),
searchBox(0),
isHash(false),
mode(0),
size(0),
sizeMode(0),
fileType(0),
slots(0),
onlyFree(BOOLSETTING(SEARCH_ONLY_FREE_SLOTS)),
filter(0),
filterShared(BOOLSETTING(SEARCH_FILTER_SHARED)),
merge(0),
bMerge(BOOLSETTING(SEARCH_MERGE)),
hubs(0),
results(0),
showUI(0),
initialType(initialType_),
droppedResults(0)
{
	paned = addChild(VSplitter::Seed(SETTING(SEARCH_PANED_POS)));

	{
		options = addChild(Grid::Seed(5, 1));
		options->column(0).mode = GridInfo::FILL;
		options->row(4).mode = GridInfo::FILL;
		options->row(4).align = GridInfo::STRETCH;
		paned->setFirst(options);

		GroupBox::Seed gs = WinUtil::Seeds::group;

		gs.caption = T_("Search for");
		GroupBoxPtr group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_SEARCH_FOR);

		GridPtr cur = group->addChild(Grid::Seed(2, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(1).mode = GridInfo::FILL;

		searchBox = cur->addChild(WinUtil::Seeds::comboBoxEdit);
		cur->setWidget(searchBox, 0, 0, 1, 2);
		addWidget(searchBox);

		for(auto i = lastSearches.crbegin(), iend = lastSearches.crend(); i != iend; ++i)
			searchBox->addValue(*i);
		searchBox->getTextBox()->onKeyDown(std::bind(&SearchFrame::handleSearchKeyDown, this, _1));
		searchBox->getTextBox()->onChar(std::bind(&SearchFrame::handleSearchChar, this, _1));

		Button::Seed bs = WinUtil::Seeds::button;
		bs.caption = T_("Purge");
		ButtonPtr button = cur->addChild(bs);
		button->setHelpId(IDH_SEARCH_PURGE);
		button->onClicked(std::bind(&SearchFrame::handlePurgeClicked, this));

		bs.style |= BS_DEFPUSHBUTTON;
		bs.caption = T_("Search");
		button = cur->addChild(bs);
		button->setHelpId(IDH_SEARCH_SEARCH);
		button->onClicked(std::bind(&SearchFrame::runSearch, this));

		gs.caption = T_("Size");
		group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_SIZE);

		cur = group->addChild(Grid::Seed(1, 3));
		cur->column(1).mode = GridInfo::FILL;

		mode = cur->addChild(WinUtil::Seeds::comboBoxStatic);
		addWidget(mode);

		mode->addValue(T_("Normal"));
		mode->addValue(T_("At least"));
		mode->addValue(T_("At most"));

		TextBox::Seed ts = WinUtil::Seeds::textBox;
		ts.style |= ES_AUTOHSCROLL | ES_NUMBER;
		size = cur->addChild(ts);
		addWidget(size);

		sizeMode = cur->addChild(WinUtil::Seeds::comboBoxStatic);
		addWidget(sizeMode);

		sizeMode->addValue(T_("B"));
		sizeMode->addValue(T_("KiB"));
		sizeMode->addValue(T_("MiB"));
		sizeMode->addValue(T_("GiB"));
		sizeMode->setSelected(2);

		gs.caption = T_("File type");
		group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_TYPE);

		{
			ComboBox::Seed cs = WinUtil::Seeds::comboBoxStatic;
			cs.style |= CBS_SORT;
			fileType = group->addChild(cs);
			addWidget(fileType);
			fillFileType(Text::toT(initialString.empty() ? SETTING(LAST_SEARCH_TYPE) : SearchManager::getTypeStr(initialType)));
		}

		gs.caption = T_("Search options");
		cur = options->addChild(gs)->addChild(Grid::Seed(3, 1));

		CheckBox::Seed cs = WinUtil::Seeds::checkBox;
		
		cs.caption = T_("Only users with free slots");
		slots = cur->addChild(cs);
		slots->setHelpId(IDH_SEARCH_SLOTS);
		slots->setChecked(onlyFree);
		slots->onClicked(std::bind(&SearchFrame::handleSlotsClicked, this));

		cs.caption = T_("Hide files already in share");
		filter = cur->addChild(cs);
		filter->setHelpId(IDH_SEARCH_SHARE);
		filter->setChecked(filterShared);
		filter->onClicked(std::bind(&SearchFrame::handleFilterClicked, this));

		cs.caption = T_("Merge results for the same file");
		merge = cur->addChild(cs);
		merge->setHelpId(IDH_SEARCH_MERGE);
		merge->setChecked(bMerge);
		merge->onClicked(std::bind(&SearchFrame::handleMergeClicked, this));

		gs.caption = T_("Hubs");
		group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_HUBS);

		WidgetHubs::Seed ls;
		ls.style |= LVS_NOCOLUMNHEADER;
		ls.lvStyle |= LVS_EX_CHECKBOXES;
		hubs = group->addChild(ls);
		addWidget(hubs);

		hubs->createColumns(TStringList(1));

		hubs->onRaw(std::bind(&SearchFrame::handleHubItemChanged, this, _1, _2), dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));

		hubs->insert(new HubInfo(Util::emptyStringT, T_("Only where I'm op"), false));
		hubs->setChecked(0, false);
	}

	results = addChild(WidgetResults::Seed());
	addWidget(results);
	paned->setSecond(results);

	results->setSmallImageList(WinUtil::fileImages);
	WinUtil::makeColumns(results, resultsColumns, COLUMN_LAST, SETTING(SEARCHFRAME_ORDER), SETTING(SEARCHFRAME_WIDTHS));

	results->onDblClicked(std::bind(&SearchFrame::handleDownload, this));
	results->onKeyDown(std::bind(&SearchFrame::handleKeyDown, this, _1));
	results->onContextMenu(std::bind(&SearchFrame::handleContextMenu, this, _1));

	showUI = addChild(WinUtil::Seeds::splitCheckBox);
	showUI->setChecked(true);
	showUI->onClicked(std::bind(&SearchFrame::handleShowUIClicked, this));

	initStatus();
	status->setSize(STATUS_SHOW_UI, showUI->getPreferredSize().x);

	layout();
	activate();

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
	SettingsManager::getInstance()->addListener(this);

	if(!initialString.empty()) {
		lastSearches.push_back(initialString);
		searchBox->insertValue(0, initialString);
		searchBox->setSelected(0);
		runSearch();
	} else {
		mode->setSelected(1);
	}
	searchBox->setFocus();
}

SearchFrame::~SearchFrame() {
}

void SearchFrame::fillFileType(const tstring& toSelect) {
	typedef vector<pair<string, int> > values_type;
	values_type values;
	for(int type = SearchManager::TYPE_ANY; type != SearchManager::TYPE_LAST; ++type)
		values.push_back(make_pair(SearchManager::getTypeStr(type), type));

	const SettingsManager::SearchTypes& searchTypes = SettingsManager::getInstance()->getSearchTypes();
	for(SettingsManager::SearchTypesIterC i = searchTypes.begin(), iend = searchTypes.end(); i != iend; ++i) {
		if(i->first.size() > 1 || i->first[0] < '1' || i->first[0] > '6') { //Custom type
			values.push_back(make_pair(i->first, SearchManager::TYPE_ANY));
		}
	}

	bool selected = false;
	for(values_type::const_iterator i = values.begin(), iend = values.end(); i != iend; ++i) {
		tstring value = Text::toT(i->first);
		int pos = fileType->addValue(value);
		if(i->second != SearchManager::TYPE_ANY)
			fileType->setData(pos, i->second);
		if(!selected && value == toSelect) {
			fileType->setSelected(pos);
			selected = true;
		}
	}
	if(!selected)
		fileType->setSelected(0);
}

void SearchFrame::searchTypesChanged() {
	tstring sel = fileType->getText();
	fileType->clear();
	fillFileType(sel);
}

void SearchFrame::layout() {
	dwt::Rectangle r(getClientSize());

	status->layout(r);
	status->mapWidget(STATUS_SHOW_UI, showUI);

	paned->layout(r);
}

bool SearchFrame::preClosing() {
	SettingsManager::getInstance()->removeListener(this);
	SearchManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);

	frames.erase(this);

	return true;
}

void SearchFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::SEARCH_PANED_POS, paned->getRelativePos());

	SettingsManager::getInstance()->set(SettingsManager::SEARCHFRAME_ORDER, WinUtil::toString(results->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::SEARCHFRAME_WIDTHS, WinUtil::toString(results->getColumnWidths()));
}

void SearchFrame::SearchInfo::view() {
	if(srs[0]->getType() == SearchResult::TYPE_FILE) {
		QueueManager::getInstance()->add(Util::getTempPath() + srs[0]->getFileName(),
			srs[0]->getSize(), srs[0]->getTTH(), HintedUser(srs[0]->getUser(), srs[0]->getHubURL()),
			QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT);
	}
}

void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
		addFile(si, Text::fromT(tgt + si->columns[COLUMN_FILENAME]));
	} else {
		addDir(si);
	}
}

void SearchFrame::SearchInfo::Download::addFile(SearchInfo* si, const string& target) {
	for(SearchResultList::const_iterator i = si->srs.begin(); i != si->srs.end(); ++i) {
		total++;
		const SearchResultPtr& sr = *i;
		try {
			QueueManager::getInstance()->add(target, sr->getSize(), sr->getTTH(), HintedUser(sr->getUser(), sr->getHubURL()));
		} catch(const Exception& e) {
			ignored++;
			error = e.getError();
		}
	}

	if(WinUtil::isShift())
		QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
}

void SearchFrame::SearchInfo::Download::addDir(SearchInfo* si, const string& target) {
	total++;
	// TODO Add all users...
	QueueManager::getInstance()->addDirectory(target.empty() ? si->srs[0]->getFile() : target,
		HintedUser(si->srs[0]->getUser(), si->srs[0]->getHubURL()), Text::fromT(tgt),
		WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
	if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
		addDir(si, Text::fromT(si->columns[COLUMN_PATH]));
	} else {
		addDir(si);
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
	if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
		addFile(si, Text::fromT(tgt));
	} else {
		addDir(si);
	}
}

void SearchFrame::SearchInfo::CheckTTH::operator()(SearchInfo* si) {
	const SearchResultPtr& sr = si->srs[0];

	if(firstTTH) {
		hasTTH = true;
		firstTTH = false;
		tth = sr->getTTH();
		name = si->getText(COLUMN_FILENAME);
		size = sr->getSize();
	} else if(hasTTH) {
		if(tth != sr->getTTH()) {
			hasTTH = false;
		}
	}

	if(firstHubs && hubs.empty()) {
		hubs = ClientManager::getInstance()->getHubs(sr->getUser()->getCID(), sr->getHubURL());
		firstHubs = false;
	} else if(!hubs.empty()) {
		Util::intersect(hubs, ClientManager::getInstance()->getHubs(sr->getUser()->getCID(), sr->getHubURL()));
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
		columns[COLUMN_NICK] = WinUtil::getNicks(sr->getUser(), sr->getHubURL());
		columns[COLUMN_CONNECTION] = Text::toT(ClientManager::getInstance()->getConnection(sr->getUser()->getCID()));
		columns[COLUMN_SLOTS] = Text::toT(sr->getSlotString());
		columns[COLUMN_IP] = Text::toT(sr->getIP());
		if(!columns[COLUMN_IP].empty()) {
			// Only attempt to grab a country mapping if we actually have an IP address
			const string& country = Util::getIpCountry(sr->getIP());
			if(!country.empty())
				columns[COLUMN_IP] = Text::toT(country) + _T(" (") + columns[COLUMN_IP] + _T(")");
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

			bool sameUser = sr->getUser()->getCID() == sr2->getUser()->getCID();
			if(sameUser && (sr->getFile() == sr2->getFile())) {
				// dupe
				delete si;
				return;
			} else if(sr->getType() == SearchResult::TYPE_FILE && sr2->getType() == SearchResult::TYPE_FILE && sr->getTTH() == sr2->getTTH()) {
				if(sameUser || (sr->getSize() != sr2->getSize())) {
					// dupe
					delete si;
					return;
				}

				if(bMerge) {
					si2->srs.push_back(sr);
					si2->update();
					delete si;
					added = true;
					results->update(i);
				}
				break;
			}
		}
	}

	if(!added) {
		results->insert(si);
	}
	status->setText(STATUS_COUNT, str(TFN_("%1% item", "%1% items", results->size()) % results->size()));
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

void SearchFrame::handleMergeClicked() {
	bMerge = merge->getChecked();
}

void SearchFrame::handleShowUIClicked() {
	bool checked = showUI->getChecked();

	if(checked && !paned->getFirst()) {
		paned->setFirst(options);
	} else if(!checked && paned->getFirst()) {
		paned->setFirst(0);
	}

	options->setVisible(checked);
	paned->setVisible(checked);

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
	treat(results->forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY)))));
}

void SearchFrame::handleDownloadFavoriteDirs(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if(index < spl.size()) {
		treat(results->forEachSelectedT(SearchInfo::Download(Text::toT(spl[index].first))));
	} else {
		dcassert((index - spl.size()) < targets.size());
		treat(results->forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[index - spl.size()]))));
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
			if(WinUtil::browseSaveFile(this, target)) {
				WinUtil::addLastDir(Util::getFilePath(target));
				treat(results->forEachSelectedT(SearchInfo::DownloadTarget(target)));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(FolderDialog(this).open(target)) {
				WinUtil::addLastDir(target);
				treat(results->forEachSelectedT(SearchInfo::Download(target)));
			}
		}
	} else {
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(FolderDialog(this).open(target)) {
			WinUtil::addLastDir(target);
			treat(results->forEachSelectedT(SearchInfo::Download(target)));
		}
	}
}

void SearchFrame::handleDownloadTarget(unsigned index) {
	if(index < WinUtil::lastDirs.size()) {
		treat(results->forEachSelectedT(SearchInfo::Download(WinUtil::lastDirs[index])));
	} else {
		dcassert((index - WinUtil::lastDirs.size()) < targets.size());
		treat(results->forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[index - WinUtil::lastDirs.size()]))));
	}
}

void SearchFrame::handleDownloadDir() {
	treat(results->forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(SETTING(DOWNLOAD_DIRECTORY)))));
}

void SearchFrame::handleDownloadWholeFavoriteDirs(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	dcassert(index < spl.size());
	treat(results->forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(spl[index].first))));
}

void SearchFrame::handleDownloadWholeTarget(unsigned index) {
	dcassert(index < WinUtil::lastDirs.size());
	treat(results->forEachSelectedT(SearchInfo::DownloadWhole(WinUtil::lastDirs[index])));
}

void SearchFrame::handleDownloadDirTo() {
	tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
	if(FolderDialog(this).open(target)) {
		WinUtil::addLastDir(target);
		treat(results->forEachSelectedT(SearchInfo::DownloadWhole(target)));
	}
}

void SearchFrame::handleViewAsText() {
	try {
		results->forEachSelected(&SearchInfo::view);
	} catch(const Exception& e) {
		status->setText(STATUS_STATUS, Text::toT(e.getError()));
	}
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
			if(std::find(users.begin(), users.end(), sr->getUser()) == users.end()) {
				users.push_back(HintedUser(sr->getUser(), sr->getHubURL()));
				dirs.push_back(Util::getFilePath(sr->getFile()));
			}
		}
	}
	HintedUserList users;
	StringList dirs;
};

MenuPtr SearchFrame::makeMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);

	StringPairList favoriteDirs = FavoriteManager::getInstance()->getFavoriteDirs();
	SearchInfo::CheckTTH checkTTH = results->forEachSelectedT(SearchInfo::CheckTTH());

	menu->setTitle(checkTTH.hasTTH ? escapeMenu(checkTTH.name) : str(TF_("%1% results") % results->countSelected()),
		getParent()->getIcon(this));

	menu->appendItem(T_("&Download"), std::bind(&SearchFrame::handleDownload, this), WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargetMenu(menu, favoriteDirs, checkTTH);
	menu->appendItem(T_("Download whole directory"), std::bind(&SearchFrame::handleDownloadDir, this));
	addTargetDirMenu(menu, favoriteDirs);
	menu->appendItem(T_("&View as text"), std::bind(&SearchFrame::handleViewAsText, this));

	if(checkTTH.hasTTH) {
		menu->appendSeparator();
		WinUtil::addHashItems(menu, checkTTH.tth, checkTTH.name, checkTTH.size);
	}

	menu->appendSeparator();
	UserCollector users = results->forEachSelectedT(UserCollector());
	WinUtil::addUserItems(menu, users.users, getParent(), users.dirs);

	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), std::bind(&SearchFrame::handleRemove, this));

	prepareMenu(menu, UserCommand::CONTEXT_SEARCH, checkTTH.hubs);

	return menu;
}

void SearchFrame::addTargetMenu(const MenuPtr& parent, const StringPairList& favoriteDirs, const SearchInfo::CheckTTH& checkTTH) {
	MenuPtr menu = parent->appendPopup(T_("Download to..."));

	int n = 0;
	if(favoriteDirs.size() > 0) {
		for(StringPairList::const_iterator i = favoriteDirs.begin(); i != favoriteDirs.end(); i++)
			menu->appendItem(Text::toT(i->second), std::bind(&SearchFrame::handleDownloadFavoriteDirs, this, n++));
		menu->appendSeparator();
	}

	n = 0;
	menu->appendItem(T_("&Browse..."), std::bind(&SearchFrame::handleDownloadTo, this));
	if(WinUtil::lastDirs.size() > 0) {
		menu->appendSeparator();
		for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i)
			menu->appendItem(*i, std::bind(&SearchFrame::handleDownloadTarget, this, n++));
	}

	if(checkTTH.hasTTH) {
		targets.clear();

		QueueManager::getInstance()->getTargets(checkTTH.tth, targets);
		if(targets.size() > 0) {
			menu->appendSeparator();
			for(StringIter i = targets.begin(); i != targets.end(); ++i)
				menu->appendItem(Text::toT(*i), std::bind(&SearchFrame::handleDownloadTarget, this, n++));
		}
	}
}

void SearchFrame::addTargetDirMenu(const MenuPtr& parent, const StringPairList& favoriteDirs) {
	MenuPtr menu = parent->appendPopup(T_("Download whole directory to..."));

	int n = 0;
	if(favoriteDirs.size() > 0) {
		for(StringPairList::const_iterator i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i)
			menu->appendItem(Text::toT(i->second), std::bind(&SearchFrame::handleDownloadWholeFavoriteDirs, this, n++));
		menu->appendSeparator();
	}

	n = 0;
	menu->appendItem(T_("&Browse..."), std::bind(&SearchFrame::handleDownloadDirTo, this));
	if(WinUtil::lastDirs.size() > 0) {
		menu->appendSeparator();
		for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i)
			menu->appendItem(*i, std::bind(&SearchFrame::handleDownloadWholeTarget, this, n++));
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
			callAsync(std::bind(&SearchFrame::updateStatusFiltered, this));
			return;
		}

		if(isHash) {
			if(aResult->getType() != SearchResult::TYPE_FILE || TTHValue(Text::fromT(currentSearch[0])) != aResult->getTTH()) {
				droppedResults++;
				callAsync(std::bind(&SearchFrame::updateStatusFiltered, this));
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
					callAsync(std::bind(&SearchFrame::updateStatusFiltered, this));
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
			callAsync(std::bind(&SearchFrame::updateStatusFiltered, this));
			return;
		}
	}

	// Reject results without free slots
	if((onlyFree && aResult->getFreeSlots() < 1))
	{
		droppedResults++;
		callAsync(std::bind(&SearchFrame::updateStatusFiltered, this));
		return;
	}

	callAsync(std::bind(&SearchFrame::addResult, this, new SearchInfo(aResult)));
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
	if (bMerge != BOOLSETTING(SEARCH_MERGE))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_MERGE, bMerge);
	if(initialType == SearchManager::TYPE_ANY) {
		string text = Text::fromT(fileType->getText());
		if(text != SETTING(LAST_SEARCH_TYPE))
			SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_TYPE, text);
	}

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
		size_t i = max(SETTING(SEARCH_HISTORY)-1, 0);

		if(searchBox->size() > i)
			searchBox->erase(i);
		searchBox->insertValue(0, s);

		while(lastSearches.size() > i) {
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

	int ftype = fileType->getData(fileType->getSelected());

	// Get ADC searchtype extensions if any is selected
	StringList extList;
	try {
		if(ftype == SearchManager::TYPE_ANY) {
			// Custom searchtype
			extList = SettingsManager::getInstance()->getExtensions(Text::fromT(fileType->getText()));
		} else if(ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_DIRECTORY) {
			// Predefined searchtype
			extList = SettingsManager::getInstance()->getExtensions(string(1, '0' + ftype));
		}
	} catch(const SearchTypeException&) {
		ftype = SearchManager::TYPE_ANY;
	}

	status->setText(STATUS_STATUS, str(TF_("Searching for %1%...") % s));
	status->setText(STATUS_COUNT, Util::emptyStringT);
	status->setText(STATUS_FILTERED, Util::emptyStringT);
	droppedResults = 0;
	isHash = (ftype == SearchManager::TYPE_TTH);

	setText(str(TF_("Search - %1%") % s));

	if(SearchManager::getInstance()->okToSearch()) {
		SearchManager::getInstance()->search(clients, Text::fromT(s), llsize,
			(SearchManager::TypeModes)ftype, searchMode, token, extList);
		if(BOOLSETTING(CLEAR_SEARCH)) // Only clear if the search was sent
			searchBox->setText(Util::emptyStringT);
	} else {
		int32_t waitFor = SearchManager::getInstance()->timeToSearch();
		tstring msg = str(TFN_("Searching too soon, next search in %1% second", "Searching too soon, next search in %1% seconds", waitFor) % waitFor);

		status->setText(STATUS_STATUS, msg);
		status->setText(STATUS_COUNT, Util::emptyStringT);
		status->setText(STATUS_FILTERED, Util::emptyStringT);

		setText(str(TF_("Search - %1%") % msg));
		// Start the countdown timer
		initSecond();
	}
}

void SearchFrame::updateStatusFiltered() {
	status->setText(STATUS_FILTERED, str(TF_("%1% filtered") % droppedResults));
}

void SearchFrame::initSecond() {
	setTimer(std::bind(&SearchFrame::eachSecond, this), 1000);
}

bool SearchFrame::eachSecond() {
	int32_t waitFor = SearchManager::getInstance()->timeToSearch();
	if(waitFor > 0) {
		tstring msg = str(TFN_("Searching too soon, next search in %1% second", "Searching too soon, next search in %1% seconds", waitFor) % waitFor);
		status->setText(STATUS_STATUS, msg);
		setText(str(TF_("Search - %1%") % msg));
		return true;
	}

	status->setText(STATUS_STATUS, T_("Ready to search..."));
	setText(T_("Search - Ready to search..."));

	return false;
}

void SearchFrame::treat(const SearchInfo::Download& dl) {
	if(dl.total) {
		status->setText(STATUS_STATUS,
			dl.ignored ? str(TF_("%1% / %2% sources ignored: %3%") % dl.ignored % dl.total % Text::toT(dl.error))
			: str(TF_("%1% sources added") % dl.total));
	}
}

void SearchFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = results->getNext(sel, LVNI_SELECTED)) != -1) {
		SearchInfo* si = results->getData(sel);
		for(SearchResultList::const_iterator i = si->srs.begin(), iend = si->srs.end(); i != iend; ++i) {
			const SearchResultPtr& sr = *i;

			if(!sr->getUser()->isOnline())
				continue;

			if(uc.once()) {
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
			ucParams["fileMN"] = WinUtil::makeMagnet(sr->getTTH(), sr->getFile(), sr->getSize());

			// compatibility with 0.674 and earlier
			ucParams["file"] = ucParams["fileFN"];
			ucParams["filesize"] = ucParams["fileSI"];
			ucParams["filesizeshort"] = ucParams["fileSIshort"];
			ucParams["tth"] = ucParams["fileTR"];

			StringMap tmp = ucParams;
			ClientManager::getInstance()->userCommand(HintedUser(sr->getUser(), sr->getHubURL()), uc, tmp, true);
		}
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
	callAsync(std::bind(&SearchFrame::onHubAdded, this, new HubInfo(c)));
}

void SearchFrame::on(ClientUpdated, Client* c) throw() {
	callAsync(std::bind(&SearchFrame::onHubChanged, this, new HubInfo(c)));
}

void SearchFrame::on(ClientDisconnected, Client* c) throw() {
	callAsync(std::bind(&SearchFrame::onHubRemoved, this, new HubInfo(c)));
}

void SearchFrame::on(SettingsManagerListener::SearchTypesChanged) throw() {
	callAsync(std::bind(&SearchFrame::searchTypesChanged, this));
}
