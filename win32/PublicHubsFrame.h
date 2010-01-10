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

#ifndef DCPLUSPLUS_WIN32_PUBLIC_HUBS_FRAME_H
#define DCPLUSPLUS_WIN32_PUBLIC_HUBS_FRAME_H

#include "StaticFrame.h"

#include "TypedTable.h"

#include <dcpp/HubEntry.h>
#include <dcpp/FavoriteManagerListener.h>
#include "resource.h"

class PublicHubsFrame :
	public StaticFrame<PublicHubsFrame>,
	public FavoriteManagerListener
{
	typedef StaticFrame<PublicHubsFrame> BaseType;
public:
	enum Status {
		STATUS_STATUS,
		STATUS_HUBS,
		STATUS_USERS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

private:
	friend class StaticFrame<PublicHubsFrame>;
	friend class MDIChildFrame<PublicHubsFrame>;

	PublicHubsFrame(dwt::TabView* mdiParent);
	virtual ~PublicHubsFrame();

	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_USERS,
		COLUMN_SERVER,
		COLUMN_COUNTRY,
		COLUMN_SHARED,
		COLUMN_MINSHARE,
		COLUMN_MINSLOTS,
		COLUMN_MAXHUBS,
		COLUMN_MAXUSERS,
		COLUMN_RELIABILITY,
		COLUMN_RATING,
		COLUMN_LAST
	};

	enum FilterModes{
		NONE,
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL
	};

	class HubInfo {
	public:
		HubInfo(const HubEntry* entry_);

		static int compareItems(const HubInfo* a, const HubInfo* b, int col);
		const tstring& getText(int column) const { return columns[column]; }
		int getImage() const { return 0; }
		const HubEntry* entry;
		tstring columns[COLUMN_LAST];
	};

	GridPtr grid;

	GridPtr upper;
	TextBoxPtr blacklist;

	typedef TypedTable<HubInfo> WidgetHubs;
	typedef WidgetHubs* WidgetHubsPtr;
	WidgetHubsPtr hubs;

	TextBoxPtr filter;
	ComboBoxPtr filterSel;
	ComboBoxPtr lists;

	size_t visibleHubs;
	int users;

	string filterString;

	HubEntryList entries;

	void layout();
	bool preClosing();
	void postClosing();
	void handleConfigure();
	void handleRefresh();
	void handleConnect();
	void handleAdd();
	void handleCopyHub();
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	bool handleKeyDown(int c);
	void handleListSelChanged();
	bool handleFilterKeyDown(int c);

	void updateStatus();
	void updateList();
	void updateDropDown();
	void openSelected();

	bool parseFilter(FilterModes& mode, double& size);
	bool matchFilter(const HubEntry& entry, const int& sel, bool doSizeCompare, const FilterModes& mode, const double& size);

	void onFinished(const tstring& s);

	virtual void on(DownloadStarting, const string& l) throw();
	virtual void on(DownloadFailed, const string& l) throw();
	virtual void on(DownloadFinished, const string& l, bool fromCoral) throw();
	virtual void on(LoadedFromCache, const string& l, const string& d) throw();
	virtual void on(Corrupted, const string& l) throw();
};

#endif // !defined(PUBLIC_HUBS_FRM_H)
