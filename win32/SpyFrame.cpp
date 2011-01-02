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

#include "SpyFrame.h"

#include <dcpp/ShareManager.h>
#include <dcpp/ClientManager.h>

#include "SearchFrame.h"

const string SpyFrame::id = "SearchSpy";
const string& SpyFrame::getId() const { return id; }

#ifdef __MINGW32__
const size_t SpyFrame::AVG_TIME; // TODO gcc needs this - why?
#endif

static const ColumnInfo searchesColumns[] = {
	{ N_("Search string"), 305, false },
	{ N_("Count"), 70, true },
	{ N_("Time"), 85, false }
};

SpyFrame::SpyFrame(dwt::TabView* mdiParent) :
	BaseType(mdiParent, T_("Search Spy"), IDH_SEARCH_SPY, IDI_SPY),
	searches(0),
	ignoreTTH(0),
	bIgnoreTTH(BOOLSETTING(SPY_FRAME_IGNORE_TTH_SEARCHES)),
	total(0),
	cur(0)
{
	memset(perSecond, 0, sizeof(perSecond));

	{
		Table::Seed cs = WinUtil::Seeds::table;
		cs.style |= LVS_SINGLESEL;
		searches = addChild(cs);
		addWidget(searches);

		WinUtil::makeColumns(searches, searchesColumns, COLUMN_LAST, SETTING(SPYFRAME_ORDER), SETTING(SPYFRAME_WIDTHS));
		searches->setSort(COLUMN_COUNT, dwt::Table::SORT_INT, false);

		searches->onColumnClick(std::bind(&SpyFrame::handleColumnClick, this, _1));
		searches->onContextMenu(std::bind(&SpyFrame::handleContextMenu, this, _1));
	}

	{
		CheckBox::Seed seed = WinUtil::Seeds::checkBox;
		seed.caption = T_("Ignore TTH searches");
		ignoreTTH = addChild(seed);
		ignoreTTH->setHelpId(IDH_SPY_IGNORE_TTH);
		ignoreTTH->setChecked(bIgnoreTTH);
		ignoreTTH->onClicked(std::bind(&SpyFrame::handleIgnoreTTHClicked, this));
	}

	initStatus();
	status->setSize(STATUS_IGNORE_TTH, ignoreTTH->getPreferredSize().x);

	status->setHelpId(STATUS_TOTAL, IDH_SPY_TOTAL);
	status->setHelpId(STATUS_AVG_PER_SECOND, IDH_SPY_AVG_PER_SECOND);
	status->setHelpId(STATUS_HITS, IDH_SPY_HITS);
	status->setHelpId(STATUS_HIT_RATIO, IDH_SPY_HIT_RATIO);

	layout();
	activate();

	ShareManager::getInstance()->setHits(0);

	ClientManager::getInstance()->addListener(this);

	initSecond();
}

SpyFrame::~SpyFrame() {
}

void SpyFrame::layout() {
	dwt::Rectangle r(this->getClientSize());

	status->layout(r);
	status->mapWidget(STATUS_IGNORE_TTH, ignoreTTH);

	searches->layout(r);
}

void SpyFrame::initSecond() {
	setTimer(std::bind(&SpyFrame::eachSecond, this), 1000);
}

bool SpyFrame::eachSecond() {
	size_t tot = std::accumulate(perSecond, perSecond + AVG_TIME, 0u);
	size_t t = std::max(static_cast<size_t>(1), std::min(cur, AVG_TIME));

	float x = static_cast<float>(tot)/t;

	cur++;
	perSecond[cur % AVG_TIME] = 0;
	status->setText(STATUS_AVG_PER_SECOND, str(TF_("Average/s: %1%") % x));
	return true;
}

bool SpyFrame::preClosing() {
	ClientManager::getInstance()->removeListener(this);
	return true;
}

void SpyFrame::postClosing() {
	searches->clear();

	SettingsManager::getInstance()->set(SettingsManager::SPYFRAME_ORDER, WinUtil::toString(searches->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::SPYFRAME_WIDTHS, WinUtil::toString(searches->getColumnWidths()));

	SettingsManager::getInstance()->set(SettingsManager::SPY_FRAME_IGNORE_TTH_SEARCHES, bIgnoreTTH);
}

void SpyFrame::handleColumnClick(int column) {
	if(column == searches->getSortColumn()) {
		if (!searches->isAscending())
			searches->setSort(-1, searches->getSortType());
		else
			searches->setSort(searches->getSortColumn(), searches->getSortType(), false);
	} else {
		if(column == COLUMN_COUNT) {
			searches->setSort(column, dwt::Table::SORT_INT);
		} else {
			searches->setSort(column, dwt::Table::SORT_STRING_NOCASE);
		}
	}
}

bool SpyFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(searches->countSelected() == 1) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = searches->getContextMenuPos();
		}

		tstring selText = searches->getText(searches->getSelected(), COLUMN_STRING);

		MenuPtr menu = addChild(WinUtil::Seeds::menu);
		menu->setTitle(escapeMenu(selText), getParent()->getIcon(this));
		menu->appendItem(T_("&Search"), std::bind(&SpyFrame::handleSearch, this, selText), WinUtil::menuIcon(IDI_SEARCH));

		menu->open(pt);
		return true;
	}
	return false;
}

void SpyFrame::handleSearch(const tstring& searchString) {
	if(Util::strnicmp(searchString.c_str(), _T("TTH:"), 4) == 0)
		SearchFrame::openWindow(getParent(), searchString.substr(4), SearchManager::TYPE_TTH);
	else
		SearchFrame::openWindow(getParent(), searchString, SearchManager::TYPE_ANY);
}

void SpyFrame::handleIgnoreTTHClicked() {
	bIgnoreTTH = ignoreTTH->getChecked();
}

void SpyFrame::add(const tstring& x) {
	total++;

	// Not thread safe, but who cares really...
	perSecond[cur % AVG_TIME]++;

	int j = searches->find(x);
	if(j == -1) {
		TStringList a;
		a.push_back(x);
		a.push_back(Text::toT(Util::toString(1)));
		a.push_back(Text::toT(Util::getTimeString()));
		searches->insert(a);
		if(searches->size() > 500) {
			searches->erase(searches->size() - 1);
		}
	} else {
		searches->setText(j, COLUMN_COUNT, Text::toT(Util::toString(Util::toInt(Text::fromT(searches->getText(j, COLUMN_COUNT))) + 1)));
		searches->setText(j, COLUMN_TIME, Text::toT(Util::getTimeString()));
		if(searches->getSortColumn() == COLUMN_COUNT || searches->getSortColumn() == COLUMN_TIME )
			searches->resort();
	}

	status->setText(STATUS_TOTAL, str(TF_("Total: %1%") % total));
	status->setText(STATUS_HITS, str(TF_("Hits: %1%") % ShareManager::getInstance()->getHits()));
	double ratio = total > 0 ? ((double)ShareManager::getInstance()->getHits()) / (double)total : 0.0;
	status->setText(STATUS_HIT_RATIO, str(TF_("Hit Ratio: %1%") % ratio));

	setDirty(SettingsManager::BOLD_SEARCH_SPY);
}

void SpyFrame::on(ClientManagerListener::IncomingSearch, const string& s) throw() {
	if(bIgnoreTTH && s.compare(0, 4, "TTH:") == 0)
		return;
	tstring x = Text::toT(s);
	tstring::size_type i = 0;
	while( (i=x.find(_T('$'))) != string::npos) {
		x[i] = _T(' ');
	}
	callAsync(std::bind(&SpyFrame::add, this, x));
}
