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

#ifndef DCPLUSPLUS_WIN32_SEARCH_FRAME_H
#define DCPLUSPLUS_WIN32_SEARCH_FRAME_H

#include "MDIChildFrame.h"
#include "TypedTable.h"
#include "AspectUserCommand.h"

#include <dcpp/SearchResult.h>
#include <dcpp/SearchManager.h>
#include <dcpp/ClientManagerListener.h>

class SearchFrame :
	public MDIChildFrame<SearchFrame>,
	private SearchManagerListener,
	private ClientManagerListener,
	public AspectUserCommand<SearchFrame>
{
	typedef MDIChildFrame<SearchFrame> BaseType;
public:
	enum Status {
		STATUS_SHOW_UI,
		STATUS_STATUS,
		STATUS_COUNT,
		STATUS_FILTERED,
		STATUS_LAST
	};

	static void openWindow(dwt::TabView* mdiParent, const tstring& str, SearchManager::TypeModes type);
	static void closeAll();

private:
	friend class MDIChildFrame<SearchFrame>;
	friend class AspectUserCommand<SearchFrame>;

	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		COLUMN_NICK,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
		COLUMN_IP,
		COLUMN_TTH,
		COLUMN_CID,
		COLUMN_LAST
	};

	class SearchInfo {
	public:
		SearchInfo(const SearchResultPtr& aSR);
		~SearchInfo() {	}

		void view();

		struct Download {
			Download(const tstring& aTarget) : tgt(aTarget) { }
			void operator()(SearchInfo* si) const;

		protected:
			const tstring& tgt;

			void addFile(SearchInfo* si, const string& target) const;
			void addDir(SearchInfo* si) const;
		};

		struct DownloadWhole : Download {
			DownloadWhole(const tstring& aTarget) : Download(aTarget) { }
			void operator()(SearchInfo* si) const;
		};

		struct DownloadTarget : Download {
			DownloadTarget(const tstring& aTarget) : Download(aTarget) { }
			void operator()(SearchInfo* si) const;
		};

		struct CheckTTH {
			CheckTTH() : firstHubs(true), op(true), hasTTH(false), firstTTH(true) { }
			void operator()(SearchInfo* si);
			bool firstHubs;
			StringList hubs;
			bool op;
			bool hasTTH;
			bool firstTTH;
			tstring tth;
		};

		const tstring& getText(int col) const { return columns[col]; }
		int getImage();

		static int compareItems(SearchInfo* a, SearchInfo* b, int col);

		void update();

		SearchResultList srs;

		tstring columns[COLUMN_LAST];
	};

	struct HubInfo : public FastAlloc<HubInfo> {
		HubInfo(const tstring& aUrl, const tstring& aName, bool aOp) : url(aUrl),
			name(aName), op(aOp) { }
		HubInfo(Client* client) : url(Text::toT(client->getHubUrl())),
			name(Text::toT(client->getHubName())), op(client->getMyIdentity().isOp()) { }

		const tstring& getText(int col) const {
			return (col == 0) ? name : Util::emptyStringT;
		}
		int getImage() const {
			return 0;
		}
		static int compareItems(HubInfo* a, HubInfo* b, int col) {
			return (col == 0) ? lstrcmpi(a->name.c_str(), b->name.c_str()) : 0;
		}
		tstring url;
		tstring name;
		bool op;
	};

	typedef set<SearchFrame*> FrameSet;
	typedef FrameSet::iterator FrameIter;
	static FrameSet frames;

	WidgetVPanedPtr paned;

	GridPtr options;

	ComboBoxPtr searchBox;
	bool isHash;

	ComboBoxPtr mode;

	TextBoxPtr size;
	ComboBoxPtr sizeMode;

	ComboBoxPtr fileType;

	CheckBoxPtr slots;
	bool onlyFree;

	CheckBoxPtr filter;
	bool filterShared;

	CheckBoxPtr merge;
	bool bMerge;

	typedef TypedTable<HubInfo> WidgetHubs;
	typedef WidgetHubs* WidgetHubsPtr;
	WidgetHubsPtr hubs;

	typedef TypedTable<SearchInfo> WidgetResults;
	typedef WidgetResults* WidgetResultsPtr;
	WidgetResultsPtr results;

	CheckBoxPtr showUI;

	SearchManager::TypeModes initialType;

	static TStringList lastSearches;

	size_t droppedResults;

	TStringList currentSearch;
	StringList targets;

	CriticalSection cs;

	StringMap ucLineParams;

	std::string token;

	SearchFrame(dwt::TabView* mdiParent, const tstring& initialString, SearchManager::TypeModes initialType_);
	virtual ~SearchFrame();

	void handlePurgeClicked();
	void handleSlotsClicked();
	void handleFilterClicked();
	void handleMergeClicked();
	void handleShowUIClicked();
	LRESULT handleHubItemChanged(WPARAM wParam, LPARAM lParam);
	void handleDoubleClick();
	bool handleKeyDown(int c);
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	void handleDownload();
	void handleDownloadFavoriteDirs(unsigned index);
	void handleDownloadTo();
	void handleDownloadTarget(unsigned index);
	void handleDownloadDir();
	void handleDownloadWholeFavoriteDirs(unsigned index);
	void handleDownloadWholeTarget(unsigned index);
	void handleDownloadDirTo();
	void handleViewAsText();
	void handleRemove();
	bool handleSearchKeyDown(int c);
	bool handleSearchChar(int c);

	void layout();
	bool preClosing();
	void postClosing();
	void initSecond();
	bool eachSecond();

	void runUserCommand(const UserCommand& uc);
	void runSearch();
	void updateStatusFiltered();

	MenuPtr makeMenu();
	void addTargetMenu(const MenuPtr& parent, const StringPairList& favoriteDirs, const SearchInfo::CheckTTH& checkTTH);
	void addTargetDirMenu(const MenuPtr& parent, const StringPairList& favoriteDirs);

	WidgetResultsPtr getUserList() { return results; }

	// SearchManagerListener
	virtual void on(SearchManagerListener::SR, const SearchResultPtr& aResult) throw();
	void addResult(SearchInfo* si);

	// ClientManagerListener
	virtual void on(ClientConnected, Client* c) throw();
	virtual void on(ClientUpdated, Client* c) throw();
	virtual void on(ClientDisconnected, Client* c) throw();

	void onHubAdded(HubInfo* info);
	void onHubChanged(HubInfo* info);
	void onHubRemoved(HubInfo* info);
};

#endif // !defined(DCPLUSPLUS_WIN32_SEARCH_FRAME_H)
