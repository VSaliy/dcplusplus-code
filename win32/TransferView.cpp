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
#include "TransferView.h"

#include <dcpp/ClientManager.h>
#include <dcpp/ConnectionManager.h>
#include <dcpp/Download.h>
#include <dcpp/DownloadManager.h>
#include <dcpp/GeoManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/Upload.h>
#include <dcpp/UploadManager.h>
#include <dcpp/UserConnection.h>

#include <dwt/resources/Pen.h>
#include <dwt/util/StringUtils.h>

#include "resource.h"
#include "HoldRedraw.h"
#include "TypedTable.h"
#include "WinUtil.h"

using dwt::util::escapeMenu;

int TransferView::connectionIndexes[] = { CONNECTION_COLUMN_USER, CONNECTION_COLUMN_HUB, CONNECTION_COLUMN_STATUS, CONNECTION_COLUMN_SPEED, CONNECTION_COLUMN_CHUNK, CONNECTION_COLUMN_TRANSFERED, CONNECTION_COLUMN_QUEUED, CONNECTION_COLUMN_CIPHER, CONNECTION_COLUMN_IP, CONNECTION_COLUMN_COUNTRY };
int TransferView::connectionSizes[] = { 125, 100, 375, 100, 125, 125, 75, 100, 100, 100 };

int TransferView::downloadIndexes[] = { DOWNLOAD_COLUMN_FILE, DOWNLOAD_COLUMN_PATH, DOWNLOAD_COLUMN_STATUS, DOWNLOAD_COLUMN_TIMELEFT, DOWNLOAD_COLUMN_SPEED, DOWNLOAD_COLUMN_DONE, DOWNLOAD_COLUMN_SIZE };
int TransferView::downloadSizes[] = { 200, 300, 150, 200, 125, 100, 100 };

static const ColumnInfo connectionColumns[] = {
	{ N_("User"), 125, false },
	{ N_("Hub"), 100, false },
	{ N_("Status"), 375, false },
	{ N_("Speed"), 100, true },
	{ N_("Chunk"), 125, true },
	{ N_("Transferred (Ratio)"), 125, true },
	{ N_("Queued"), 80, true },
	{ N_("Cipher"), 150, false },
	{ N_("IP"), 100, false },
	{ N_("Country"), 50, false }
};

static const ColumnInfo downloadColumns[] = {
	{ N_("Filename"), 200, false },
	{ N_("Path"), 300, false },
	{ N_("Status"), 150, false },
	{ N_("Time left"), 200, true },
	{ N_("Speed"), 125, true },
	{ N_("Done"), 80, true },
	{ N_("Size"), 80, true }
};

static bool noClose() {
	return false;
}

TransferView::TransferView(dwt::Widget* parent, TabViewPtr mdi_) :
	dwt::Container(parent),
	connections(0),
	connectionsWindow(0),
	downloads(0),
	downloadsWindow(0),
	mdi(mdi_)
{
	create();

	{
		TabView::Seed seed = WinUtil::Seeds::tabs;
		seed.style &= ~TCS_OWNERDRAWFIXED;
		seed.location = dwt::Rectangle(getClientSize());
		seed.widthConfig = 0;
		seed.ctrlTab = true;
		tabs = addChild(seed);
		tabs->onHelp(&WinUtil::help);
	}

	{
		Container::Seed cs;
		cs.caption = T_("Connections");
		cs.location = tabs->getClientSize();
		connectionsWindow = dwt::WidgetCreator<Container>::create(tabs, cs);
		connectionsWindow->setHelpId(IDH_CONNECTIONS);
		connectionsWindow->onClosing(&noClose);
		tabs->add(connectionsWindow);

		cs.style &= ~WS_VISIBLE;
		cs.caption = T_("Downloads");
		downloadsWindow = dwt::WidgetCreator<Container>::create(tabs, cs);
		downloadsWindow->setHelpId(IDH_DOWNLOADS);
		downloadsWindow->onClosing(&noClose);
		tabs->add(downloadsWindow);
	}

	{
		connections = connectionsWindow->addChild(WidgetConnections::Seed(WinUtil::Seeds::table));

		arrows = dwt::ImageListPtr(new dwt::ImageList(dwt::Point(16, 16)));
		arrows->add(*WinUtil::createIcon(IDI_DOWNLOAD, 16));
		arrows->add(*WinUtil::createIcon(IDI_UPLOAD, 16));
		connections->setSmallImageList(arrows);

		WinUtil::makeColumns(connections, connectionColumns, CONNECTION_COLUMN_LAST, SETTING(CONNECTIONS_ORDER), SETTING(CONNECTIONS_WIDTHS));

		WinUtil::setColor(connections);
		connections->setSort(CONNECTION_COLUMN_USER);

		connections->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleConnectionsMenu(sc); });
		connections->onKeyDown([this](int c) { return handleKeyDown(c); });
		connections->onDblClicked([this] { handleDblClicked(); });
		connections->onCustomDraw([this](NMLVCUSTOMDRAW& data) { return handleCustomDraw(data); });
	}

	{
		downloads = downloadsWindow->addChild(WidgetDownloads::Seed(WinUtil::Seeds::table));

		WinUtil::makeColumns(downloads, downloadColumns, DOWNLOAD_COLUMN_LAST, SETTING(DOWNLOADS_ORDER), SETTING(DOWNLOADS_WIDTHS));
		downloads->setSort(DOWNLOAD_COLUMN_STATUS);
		WinUtil::setColor(downloads);
		downloads->setSmallImageList(WinUtil::fileImages);

		downloads->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleDownloadsMenu(sc); });
		downloads->onCustomDraw([this](NMLVCUSTOMDRAW& data) { return handleCustomDraw(data); });
	}

	onRaw([this](WPARAM, LPARAM) { return handleDestroy(); }, dwt::Message(WM_DESTROY));
	noEraseBackground();

	layout();

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
}

TransferView::~TransferView() {

}

void TransferView::layout() {
	tabs->resize(dwt::Rectangle(getClientSize()));
}

void TransferView::prepareClose() {
	QueueManager::getInstance()->removeListener(this);
	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
}

LRESULT TransferView::handleDestroy() {
	SettingsManager::getInstance()->set(SettingsManager::CONNECTIONS_ORDER, WinUtil::toString(connections->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::CONNECTIONS_WIDTHS, WinUtil::toString(connections->getColumnWidths()));

	SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_ORDER, WinUtil::toString(downloads->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_WIDTHS, WinUtil::toString(downloads->getColumnWidths()));

	return 0;
}

bool TransferView::handleConnectionsMenu(dwt::ScreenCoordinate pt) {
	size_t sel = connections->countSelected();
	if(sel) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = connections->getContextMenuPos();
		}

		auto selData = (sel == 1) ? connections->getSelectedData() : 0;

		MenuPtr menu = addChild(WinUtil::Seeds::menu);

		menu->setTitle(selData ? escapeMenu(selData->getText(CONNECTION_COLUMN_USER)) : str(TF_("%1% users") % sel),
			selData ? arrows->getIcon(selData->getImage(0)) : 0);

		appendUserItems(mdi, menu, false);
		menu->appendSeparator();

		menu->appendItem(T_("&Force attempt"), [this] { handleForce(); });
		menu->appendItem(T_("Copy &nick to clipboard"), [this] { handleCopyNick(); });
		menu->appendSeparator();
		menu->appendItem(T_("&Disconnect"), [this] { handleDisconnect(); });

		menu->open(pt);
		return true;
	}
	return false;
}

bool TransferView::handleDownloadsMenu(dwt::ScreenCoordinate pt) {
	size_t sel = downloads->countSelected();
	if(sel) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = downloads->getContextMenuPos();
		}

		auto selData = (sel == 1) ? downloads->getSelectedData() : 0;

		MenuPtr menu = addChild(WinUtil::Seeds::menu);

		menu->setTitle(selData ? escapeMenu(selData->getText(DOWNLOAD_COLUMN_FILE)) : str(TF_("%1% files") % sel),
			selData ? WinUtil::fileImages->getIcon(selData->getImage(0)) : 0);

		if(selData) {
			WinUtil::addHashItems(menu, selData->tth, selData->getText(DOWNLOAD_COLUMN_FILE), selData->size);
		} else {
			for(size_t i = 0; i < sel; ++i) {
				selData = downloads->getData(i);
				if(selData) {
					const tstring& file = selData->getText(DOWNLOAD_COLUMN_FILE);
					WinUtil::addHashItems(menu->appendPopup(file, WinUtil::fileImages->getIcon(selData->getImage(0))),
						selData->tth, file, selData->size);
				}
			}
		}

		menu->open(pt);
		return true;
	}
	return false;
}

int TransferView::find(const string& path) {
	for(size_t i = 0; i < downloads->size(); ++i) {
		DownloadInfo* di = downloads->getData(i);
		if(Util::stricmp(di->path, path) == 0) {
			return i;
		}
	}
	return -1;
}


void TransferView::handleDisconnect() {
	connections->forEachSelected(&ConnectionInfo::disconnect);
}

void TransferView::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	int i = -1;
	while((i = connections->getNext(i, LVNI_SELECTED)) != -1) {
		ConnectionInfo* itemI = connections->getData(i);
		if(!itemI->getUser().user->isOnline())
			continue;

		auto tmp = ucParams;
		/// @todo tmp["fileFN"] = Text::fromT(itemI->path + itemI->file);

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];

		ClientManager::getInstance()->userCommand(itemI->getUser(), uc, tmp, true);
	}
}

bool TransferView::handleKeyDown(int c) {
	if(c == VK_DELETE) {
		connections->forEachSelected(&ConnectionInfo::disconnect);
		return true;
	}
	return false;
}

void TransferView::handleForce() {
	int i = -1;
	while( (i = connections->getNext(i, LVNI_SELECTED)) != -1) {
		connections->getData(i)->columns[CONNECTION_COLUMN_STATUS] = T_("Connecting (forced)");
		connections->update(i);
		ConnectionManager::getInstance()->force(connections->getData(i)->getUser());
	}
}

void TransferView::handleCopyNick() {
	int i = -1;

	/// @todo Fix when more items are selected
	while( (i = connections->getNext(i, LVNI_SELECTED)) != -1) {
		WinUtil::setClipboard(WinUtil::getNicks(connections->getData(i)->getUser()));
	}
}

namespace { void drawProgress(HDC hdc, const dwt::Rectangle& rcItem, int item, int column, const tstring& text, double pos, bool download) {
	// draw something nice...
	COLORREF barBase = download ? SETTING(DOWNLOAD_BG_COLOR) : SETTING(UPLOAD_BG_COLOR);
	COLORREF bgBase = WinUtil::bgColor;
	int mod = (HLS_L(RGB2HLS(bgBase)) >= 128) ? -30 : 30;

	// Dark, medium and light shades
	COLORREF barPal[3] = { HLS_TRANSFORM(barBase, -40, 50), barBase, HLS_TRANSFORM(barBase, 40, -30) };

	// Two shades of the background color
	COLORREF bgPal[2] = { HLS_TRANSFORM(bgBase, mod, 0), HLS_TRANSFORM(bgBase, mod/2, 0) };

	dwt::FreeCanvas canvas(hdc);

	dwt::Rectangle rc = rcItem;

	// draw background

	{
		dwt::Brush brush(::CreateSolidBrush(bgPal[1]));
		auto selectBg(canvas.select(brush));

		dwt::Pen pen(bgPal[0]);
		auto selectPen(canvas.select(pen));

		// TODO Don't draw where the finished part will be drawn
		canvas.rectangle(rc.left(), rc.top() - 1, rc.right(), rc.bottom());
	}

	rc.pos.x += 1;
	rc.size.x -= 2;
	rc.size.y -= 1;

	dwt::Rectangle textRect;

	{
		dwt::Brush brush(::CreateSolidBrush(barPal[1]));
		auto selectBg(canvas.select(brush));

		{
			dwt::Pen pen(barPal[0]);
			auto selectPen(canvas.select(pen));

			// "Finished" part
			rc.size.x = (int) (rc.width() * pos);

			canvas.rectangle(rc.left(), rc.top(), rc.right(), rc.bottom());
		}

		textRect = rc;

		// draw progressbar highlight
		if(rc.width() > 2) {
			dwt::Pen pen(barPal[2], dwt::Pen::Solid, 1);
			auto selectPen(canvas.select(pen));

			rc.pos.y += 2;
			canvas.moveTo(rc.left() + 1, rc.top());
			canvas.lineTo(rc.right() - 2, rc.top());
		}
	}

	// draw status text

	canvas.setBkMode(true);
	auto& font = download ? WinUtil::downloadFont : WinUtil::uploadFont;
	if(!font.get()) {
		font = WinUtil::font;
	}
	auto selectFont(canvas.select(*font));

	textRect.pos.x += 6;

	long left = textRect.left();

	TEXTMETRIC tm;
	canvas.getTextMetrics(tm);
	long top = textRect.top() + (textRect.bottom() - textRect.top() - tm.tmHeight) / 2;

	canvas.setTextColor(download ? SETTING(DOWNLOAD_TEXT_COLOR) : SETTING(UPLOAD_TEXT_COLOR));
	/// @todo ExtTextOut to dwt
	::RECT r = textRect;
	::ExtTextOut(hdc, left, top, ETO_CLIPPED, &r, text.c_str(), text.size(), NULL);

	r.left = r.right;
	r.right = rcItem.right();

	canvas.setTextColor(WinUtil::textColor);
	::ExtTextOut(hdc, left, top, ETO_CLIPPED, &r, text.c_str(), text.size(), NULL);
} }

LRESULT TransferView::handleCustomDraw(NMLVCUSTOMDRAW& data) {
	switch(data.nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
	{
		int column = data.iSubItem;

		// Let's draw a box if needed...
		if(data.nmcd.hdr.hwndFrom == connections->handle() && column == CONNECTION_COLUMN_STATUS) {
			const auto& info = *reinterpret_cast<ConnectionInfo*>(data.nmcd.lItemlParam);
			if(info.status == ConnectionInfo::STATUS_RUNNING && info.chunk > 0 && info.chunkPos >= 0) {
				int item = static_cast<int>(data.nmcd.dwItemSpec);
				drawProgress(data.nmcd.hdc, connections->getRect(item, column, LVIR_BOUNDS), item, column,
					info.getText(column), static_cast<double>(info.chunkPos) / static_cast<double>(info.chunk), info.download);
				return CDRF_SKIPDEFAULT;
			}
		} else if(data.nmcd.hdr.hwndFrom == downloads->handle() && column == DOWNLOAD_COLUMN_STATUS) {
			const auto& info = *reinterpret_cast<DownloadInfo*>(data.nmcd.lItemlParam);
			if(info.size > 0 && info.done >= 0) {
				int item = static_cast<int>(data.nmcd.dwItemSpec);
				drawProgress(data.nmcd.hdc, downloads->getRect(item, column, LVIR_BOUNDS), item, column,
					info.getText(column), static_cast<double>(info.done) / static_cast<double>(info.size), true);
				return CDRF_SKIPDEFAULT;
			}
		}
	}
		// Fall through
	default:
		return CDRF_DODEFAULT;
	}
}

void TransferView::handleDblClicked() {
	ConnectionInfo* ii = connections->getSelectedData();
	if(ii) {
		ii->pm(mdi);
	}
}

TransferView::UserInfoList TransferView::selectedUsersImpl() const {
	return usersFromTable(connections);
}

int TransferView::ConnectionInfo::compareItems(const ConnectionInfo* a, const ConnectionInfo* b, int col) {
	if(BOOLSETTING(ALT_SORT_ORDER)) {
		if(a->download == b->download) {
			if(a->status != b->status) {
				return (a->status == ConnectionInfo::STATUS_RUNNING) ? -1 : 1;
			}
		} else {
			return a->download ? -1 : 1;
		}
	} else {
		if(a->status == b->status) {
			if(a->download != b->download) {
				return a->download ? -1 : 1;
			}
		} else {
			return (a->status == ConnectionInfo::STATUS_RUNNING) ? -1 : 1;
		}
	}
	switch(col) {
	case CONNECTION_COLUMN_STATUS: return 0;
	case CONNECTION_COLUMN_SPEED: return compare(a->speed, b->speed);
	case CONNECTION_COLUMN_TRANSFERED: return compare(a->transfered, b->transfered);
	case CONNECTION_COLUMN_QUEUED: return compare(a->queued, b->queued);
	case CONNECTION_COLUMN_CHUNK: return compare(a->chunk, b->chunk);
	default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
	}
}

void TransferView::addTask(int type, Task* ui) {
	tasks.add(type, unique_ptr<Task>(ui));
	callAsync([this] { execTasks(); });
}

void TransferView::execTasks() {
	TaskQueue::List t;
	tasks.get(t);

	bool sortConn = false;
	bool sortDown = false;

	HoldRedraw hold(connections, t.size() > 1);
	HoldRedraw hold2(downloads, t.size() > 1);

	for(auto i = t.begin(); i != t.end(); ++i) {
		if(i->first == CONNECTIONS_ADD) {
			auto &ui = static_cast<UpdateInfo&>(*i->second);
			ConnectionInfo* ii = new ConnectionInfo(ui.user, ui.download);
			ii->update(ui);
			connections->insert(ii);
		} else if(i->first == CONNECTIONS_REMOVE) {
			auto &ui = static_cast<UpdateInfo&>(*i->second);
			int ic = connections->size();
			for(int i = 0; i < ic; ++i) {
				ConnectionInfo* ii = connections->getData(i);
				if(ui == *ii) {
					connections->erase(i);
					break;
				}
			}
		} else if(i->first == CONNECTIONS_UPDATE) {
			auto &ui = static_cast<UpdateInfo&>(*i->second);
			int ic = connections->size();
			for(int i = 0; i < ic; ++i) {
				ConnectionInfo* ii = connections->getData(i);
				if(ii->download == ui.download && ii->getUser() == ui.user) {
					ii->update(ui);
					connections->update(i);
					sortConn = true;
					break;
				}
			}
		} else if(i->first == DOWNLOADS_ADD_USER) {
			auto &ti = static_cast<TickInfo&>(*i->second);
			int i = find(ti.path);
			if(i == -1) {
				int64_t size = QueueManager::getInstance()->getSize(ti.path);
				TTHValue tth;
				if(size != -1 && QueueManager::getInstance()->getTTH(ti.path, tth)) {
					i = downloads->insert(new DownloadInfo(ti.path, size, tth));
				}
			} else {
				downloads->getData(i)->users++;
				downloads->update(i);
			}
		} else if(i->first == DOWNLOADS_TICK) {
			auto &ti = static_cast<TickInfo&>(*i->second);
			int i = find(ti.path);
			if(i != -1) {
				DownloadInfo* di = downloads->getData(i);
				di->update(ti);
				downloads->update(i);
				sortDown = true;
			}
		} else if(i->first == DOWNLOADS_REMOVE_USER) {
			auto &ti = static_cast<TickInfo&>(*i->second);
			int i = find(ti.path);

			if(i != -1) {
				DownloadInfo* di = downloads->getData(i);
				if(--di->users == 0) {
					di->bps = 0;
				}
				di->update();
				downloads->update(i);
			}
		} else if(i->first == DOWNLOADS_REMOVED) {
			auto &ti = static_cast<TickInfo&>(*i->second);
			int i = find(ti.path);
			if(i != -1) {
				downloads->erase(i);
			}
		}

	}

	if(sortConn) {
		connections->resort();
	}
	if(sortDown) {
		downloads->resort();
	}
}

TransferView::ConnectionInfo::ConnectionInfo(const HintedUser& u, bool aDownload) :
	UserInfoBase(u),
	download(aDownload),
	transferFailed(false),
	status(STATUS_WAITING),
	actual(0),
	lastActual(0),
	transfered(0),
	lastTransfered(0),
	queued(0),
	speed(0),
	chunk(0),
	chunkPos(0)
{
	columns[CONNECTION_COLUMN_USER] = WinUtil::getNicks(u);
	columns[CONNECTION_COLUMN_HUB] = WinUtil::getHubNames(u).first;
	columns[CONNECTION_COLUMN_STATUS] = T_("Idle");
	columns[CONNECTION_COLUMN_TRANSFERED] = Text::toT(Util::toString(0));
	if(aDownload) {
		queued = QueueManager::getInstance()->getQueued(u).second;
		columns[CONNECTION_COLUMN_QUEUED] = Text::toT(Util::formatBytes(queued));
	}
}

void TransferView::ConnectionInfo::update(const UpdateInfo& ui) {
	if(ui.updateMask & UpdateInfo::MASK_STATUS) {
		lastTransfered = lastActual = 0;
		status = ui.status;
		if(download) {
			// Also update queued when status changes...
			queued = QueueManager::getInstance()->getQueued(user).second;
			columns[CONNECTION_COLUMN_QUEUED] = Text::toT(Util::formatBytes(queued));
		}
	}

	if(ui.updateMask & UpdateInfo::MASK_STATUS_STRING) {
		// No slots etc from transfermanager better than disconnected from connectionmanager
		if(!transferFailed)
			columns[CONNECTION_COLUMN_STATUS] = ui.statusString;
		transferFailed = ui.transferFailed;
	}

	if(ui.updateMask & UpdateInfo::MASK_TRANSFERED) {
		actual += ui.actual - lastActual;
		lastActual = ui.actual;
		transfered += ui.transfered - lastTransfered;
		lastTransfered = ui.transfered;
		if(actual == transfered) {
			columns[CONNECTION_COLUMN_TRANSFERED] = Text::toT(Util::formatBytes(transfered));
		} else {
			columns[CONNECTION_COLUMN_TRANSFERED] = str(TF_("%1% (%2$0.2f)")
				% Text::toT(Util::formatBytes(transfered))
				% (static_cast<double>(actual) / transfered));
		}
	}

	if(ui.updateMask & UpdateInfo::MASK_CHUNK) {
		chunkPos = ui.chunkPos;
		chunk = ui.chunk;
		columns[CONNECTION_COLUMN_CHUNK] = Text::toT(Util::formatBytes(chunkPos) + "/" + Util::formatBytes(chunk));
	}

	if(ui.updateMask & UpdateInfo::MASK_SPEED) {
		speed = ui.speed;
		if (status == STATUS_RUNNING) {
			columns[CONNECTION_COLUMN_SPEED] = str(TF_("%1%/s") % Text::toT(Util::formatBytes(speed)));
		} else {
			columns[CONNECTION_COLUMN_SPEED] = Util::emptyStringT;
		}
	}

	if(ui.updateMask & UpdateInfo::MASK_IP) {
		columns[CONNECTION_COLUMN_IP] = ui.ip;
	}
	
	if(ui.updateMask & UpdateInfo::MASK_COUNTRY) {
		columns[CONNECTION_COLUMN_COUNTRY] = ui.country;
	}

	if(ui.updateMask & UpdateInfo::MASK_CIPHER) {
		columns[CONNECTION_COLUMN_CIPHER] = ui.cipher;
	}
}

TransferView::DownloadInfo::DownloadInfo(const string& target, int64_t size_, const TTHValue& tth_) :
	path(target),
	done(QueueManager::getInstance()->getPos(target)),
	size(size_),
	bps(0),
	users(1),
	tth(tth_)
{
	columns[DOWNLOAD_COLUMN_FILE] = Text::toT(Util::getFileName(target));
	columns[DOWNLOAD_COLUMN_PATH] = Text::toT(Util::getFilePath(target));
	columns[DOWNLOAD_COLUMN_SIZE] = Text::toT(Util::formatBytes(size));

	update();
}

int TransferView::DownloadInfo::getImage(int col) const {
	return col == 0 ? WinUtil::getFileIcon(path) : -1;
}

void TransferView::DownloadInfo::update(const TransferView::TickInfo& ti) {
	done = ti.done + QueueManager::getInstance()->getInstance()->getPos(ti.path);
	bps = ti.bps;
	update();
}

void TransferView::DownloadInfo::update() {
	if(users == 0) {
		columns[DOWNLOAD_COLUMN_STATUS] = T_("Waiting for slot");
		columns[DOWNLOAD_COLUMN_TIMELEFT].clear();
		columns[DOWNLOAD_COLUMN_SPEED].clear();
	} else {
		columns[DOWNLOAD_COLUMN_STATUS] = str(TFN_("Downloading from %1% user", "Downloading from %1% users", users) % users);
		columns[DOWNLOAD_COLUMN_TIMELEFT] = Text::toT(Util::formatSeconds(static_cast<int64_t>(timeleft())));
		columns[DOWNLOAD_COLUMN_SPEED] = str(TF_("%1%/s") % Text::toT(Util::formatBytes(static_cast<int64_t>(bps))));
	}
	columns[DOWNLOAD_COLUMN_DONE] = Text::toT(Util::formatBytes(done));
}

void TransferView::on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) noexcept {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());

	ui->setStatus(ConnectionInfo::STATUS_WAITING);
	ui->setStatusString(T_("Connecting"));
	addTask(CONNECTIONS_ADD, ui);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) noexcept {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());

	ui->setStatusString((aCqi->getState() == ConnectionQueueItem::CONNECTING) ? T_("Connecting") : T_("Waiting to retry"));

	addTask(CONNECTIONS_UPDATE, ui);
}

void TransferView::on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) noexcept {
	addTask(CONNECTIONS_REMOVE, new UpdateInfo(aCqi->getUser(), aCqi->getDownload()));
}

void TransferView::on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) noexcept {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	if(aCqi->getUser().user->isSet(User::OLD_CLIENT)) {
		ui->setStatusString(T_("Remote client does not fully support TTH - cannot download"));
	} else {
		ui->setStatusString(Text::toT(aReason));
	}
	addTask(CONNECTIONS_UPDATE, ui);
}

static tstring getFile(Transfer* t) {
	tstring file;

	if(t->getType() == Transfer::TYPE_TREE) {
		file = str(TF_("TTH: %1%") % Text::toT(Util::getFileName(t->getPath())));
	} else if(t->getType() == Transfer::TYPE_FULL_LIST || t->getType() == Transfer::TYPE_PARTIAL_LIST) {
		file = T_("file list");
	} else {
		file = Text::toT(Util::getFileName(t->getPath()) +
			" (" + Util::formatBytes(t->getStartPos()) +
			" - " + Util::formatBytes(t->getStartPos() + t->getSize()) + ")");
	}
	return file;
}

void TransferView::starting(UpdateInfo* ui, Transfer* t) {
	ui->setStatus(ConnectionInfo::STATUS_RUNNING);
	ui->setTransfered(t->getPos(), t->getActual());
	ui->setChunk(t->getPos(), t->getSize());
	const UserConnection& uc = t->getUserConnection();
	ui->setCipher(Text::toT(uc.getCipherName()));
	const auto& country = GeoManager::getInstance()->getCountry(uc.getRemoteIp());
	if(!country.empty())
		ui->setCountry(Text::toT(country));
	ui->setIP(Text::toT(uc.getRemoteIp()));
}

void TransferView::on(DownloadManagerListener::Requesting, Download* d) noexcept {
	UpdateInfo* ui = new UpdateInfo(d->getHintedUser(), true);

	starting(ui, d);

	ui->setStatusString(str(TF_("Requesting %1%") % getFile(d)));

	addTask(CONNECTIONS_UPDATE, ui);

	addTask(DOWNLOADS_ADD_USER, new TickInfo(d->getPath()));
}

void TransferView::on(DownloadManagerListener::Starting, Download* d) noexcept {
	UpdateInfo* ui = new UpdateInfo(d->getHintedUser(), true);

	tstring statusString;

	if(d->getUserConnection().isSecure()) {
		if(d->getUserConnection().isTrusted()) {
			statusString += _T("[S]");
		} else {
			statusString += _T("[U]");
		}
	}
	if(d->isSet(Download::FLAG_TTH_CHECK)) {
		statusString += _T("[T]");
	}
	if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
		statusString += _T("[Z]");
	}
	if(!statusString.empty()) {
		statusString += _T(" ");
	}
	statusString += str(TF_("Downloading %1%") % getFile(d));

	ui->setStatusString(statusString);
	addTask(CONNECTIONS_UPDATE, ui);
}

void TransferView::onTransferTick(Transfer* t, bool isDownload) {
	UpdateInfo* ui = new UpdateInfo(t->getHintedUser(), isDownload);
	ui->setTransfered(t->getPos(), t->getActual());
	ui->setSpeed(t->getAverageSpeed());
	ui->setChunk(t->getPos(), t->getSize());
	tasks.add(CONNECTIONS_UPDATE, unique_ptr<Task>(ui));
}

void TransferView::on(DownloadManagerListener::Tick, const DownloadList& dl) noexcept  {
	for(auto i = dl.begin(); i != dl.end(); ++i) {
		onTransferTick(*i, true);
	}

	std::vector<TickInfo*> dis;
	for(auto i = dl.begin(); i != dl.end(); ++i) {
		Download* d = *i;
		if(d->getType() != Transfer::TYPE_FILE) {
			continue;
		}

		TickInfo* ti = 0;
		for(auto j = dis.begin(); j != dis.end(); ++j) {
			TickInfo* ti2 = *j;
			if(Util::stricmp(ti2->path, d->getPath()) == 0) {
				ti = ti2;
				break;
			}
		}
		if(!ti) {
			ti = new TickInfo(d->getPath());
			dis.push_back(ti);
		}
		ti->bps += d->getAverageSpeed();
		ti->done += d->getPos();
	}
	for(auto i = dis.begin(); i != dis.end(); ++i) {
		tasks.add(DOWNLOADS_TICK, unique_ptr<Task>(*i));
	}

	callAsync([this] { execTasks(); });
}

void TransferView::on(DownloadManagerListener::Failed, Download* d, const string& aReason) noexcept {
	onFailed(d, aReason);
}

void TransferView::on(QueueManagerListener::CRCFailed, Download* d, const string& aReason) noexcept {
	onFailed(d, aReason);
}

void TransferView::onFailed(Download* d, const string& aReason) {
 	UpdateInfo* ui = new UpdateInfo(d->getHintedUser(), true, true);
	ui->setStatus(ConnectionInfo::STATUS_WAITING);
	ui->setStatusString(Text::toT(aReason));

	addTask(CONNECTIONS_UPDATE, ui);

	addTask(DOWNLOADS_REMOVE_USER, new TickInfo(d->getPath()));
}

void TransferView::on(UploadManagerListener::Starting, Upload* u) noexcept {
	UpdateInfo* ui = new UpdateInfo(u->getHintedUser(), false);

	starting(ui, u);

	tstring statusString;

	if(u->getUserConnection().isSecure()) {
		if(u->getUserConnection().isTrusted()) {
			statusString += _T("[S]");
		} else {
			statusString += _T("[U]");
		}
	}
	if(u->isSet(Upload::FLAG_ZUPLOAD)) {
		statusString += _T("[Z]");
	}
	if(!statusString.empty()) {
		statusString += _T(" ");
	}
	statusString += str(TF_("Uploading %1%") % getFile(u));

	ui->setStatusString(statusString);

	addTask(CONNECTIONS_UPDATE, ui);
}

void TransferView::on(UploadManagerListener::Tick, const UploadList& ul) noexcept {
	for(auto i = ul.begin(); i != ul.end(); ++i) {
		onTransferTick(*i, false);
	}

	callAsync([this] { execTasks(); });
}

void TransferView::on(DownloadManagerListener::Complete, Download* d) noexcept {
	onTransferComplete(d, true);

	addTask(DOWNLOADS_REMOVE_USER, new TickInfo(d->getPath()));
}

void TransferView::on(UploadManagerListener::Complete, Upload* aUpload) noexcept {
	onTransferComplete(aUpload, false);
}

void TransferView::onTransferComplete(Transfer* aTransfer, bool isDownload) {
	UpdateInfo* ui = new UpdateInfo(aTransfer->getHintedUser(), isDownload);

	ui->setStatus(ConnectionInfo::STATUS_WAITING);
	ui->setStatusString(T_("Idle"));
	ui->setChunk(aTransfer->getPos(), aTransfer->getSize());

	addTask(CONNECTIONS_UPDATE, ui);
}

void TransferView::ConnectionInfo::disconnect() {
	ConnectionManager::getInstance()->disconnect(user, download);
}

void TransferView::on(QueueManagerListener::StatusUpdated, QueueItem* qi) noexcept {
	if(qi->isFinished() || qi->getPriority() == QueueItem::PAUSED || qi->countOnlineUsers() == 0) {
		addTask(DOWNLOADS_REMOVED, new TickInfo(qi->getTarget()));
	}
}

void TransferView::on(QueueManagerListener::Removed, QueueItem* qi) noexcept {
	addTask(DOWNLOADS_REMOVED, new TickInfo(qi->getTarget()));
}
