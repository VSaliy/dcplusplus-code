/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include <dwt/widgets/TableTree.h>

#include "HoldRedraw.h"
#include "resource.h"
#include "ShellMenu.h"
#include "TypedTable.h"
#include "WinUtil.h"

using dwt::util::escapeMenu;

static const ColumnInfo columns[] = {
	{ N_("File"), 200, false },
	{ N_("Path"), 300, false },
	{ N_("Status"), 350, false },
	{ N_("User"), 125, false },
	{ N_("Hub"), 100, false },
	{ N_("Time left"), 100, true },
	{ N_("Speed"), 100, true },
	{ N_("Transferred (Ratio)"), 80, true },
	{ N_("Size"), 80, true },
	{ N_("Cipher"), 100, false },
	{ N_("IP"), 100, false },
	{ N_("Country"), 100, false }
};

TransferView::TransferView(dwt::Widget* parent, TabViewPtr mdi_) :
	dwt::Container(parent),
	transfers(0),
	mdi(mdi_),
	downloadIcon(WinUtil::createIcon(IDI_DOWNLOAD, 16)),
	uploadIcon(WinUtil::createIcon(IDI_UPLOAD, 16)),
	updateList(false)
{
	create();
	setHelpId(IDH_TRANSFERS);

	transfers = addChild(WidgetTransfers::Seed(WinUtil::Seeds::table));

	transfers->setSmallImageList(WinUtil::fileImages);

	WinUtil::makeColumns(transfers, columns, COLUMN_LAST, SETTING(TRANSFERS_ORDER), SETTING(TRANSFERS_WIDTHS));
	WinUtil::setTableSort(transfers, COLUMN_LAST, SettingsManager::TRANSFERS_SORT, COLUMN_STATUS);

	WinUtil::setColor(transfers);

	transfers->onCustomDraw([this](NMLVCUSTOMDRAW& data) { return handleCustomDraw(data); });
	transfers->onKeyDown([this](int c) { return handleKeyDown(c); });
	transfers->onDblClicked([this] { handleDblClicked(); });
	transfers->onContextMenu([this](const dwt::ScreenCoordinate& sc) { return handleContextMenu(sc); });

	onDestroy([this] { handleDestroy(); });
	noEraseBackground();

	layout();

	setTimer([this] { return handleTimer(); }, 500);

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
}

TransferView::~TransferView() {
}

void TransferView::prepareClose() {
	QueueManager::getInstance()->removeListener(this);
	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
}

TransferView::ItemInfo::ItemInfo() :
	speed(0),
	actual(0),
	transferred(0),
	size(0)
{
}

const tstring& TransferView::ItemInfo::getText(int col) const {
	return columns[col];
}

int TransferView::ItemInfo::getImage(int col) const {
	return -1;
}

int TransferView::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
	if(const_cast<ItemInfo*>(a)->transfer().download != const_cast<ItemInfo*>(b)->transfer().download) {
		// sort downloads before uploads.
		return const_cast<ItemInfo*>(a)->transfer().download ? -1 : 1;
	}

	auto ca = dynamic_cast<const ConnectionInfo*>(a), cb = dynamic_cast<const ConnectionInfo*>(b);
	if(ca && cb && ca->status != cb->status) {
		// sort running conns first.
		return ca->status == ConnectionInfo::STATUS_RUNNING ? -1 : 1;
	}

	switch(col) {
	case COLUMN_STATUS:
		{
			return !a->transferred && !b->transferred ? compare(a->size, b->size) :
				!b->transferred ? -1 :
				!a->transferred ? 1 :
				compare(a->size / a->transferred, b->size / b->transferred);
		}
	case COLUMN_TIMELEFT: return compare(a->timeleft, b->timeleft);
	case COLUMN_SPEED: return compare(a->speed, b->speed);
	case COLUMN_TRANSFERRED: return compare(a->transferred, b->transferred);
	case COLUMN_SIZE: return compare(a->size, b->size);
	default: return compare(a->getText(col), b->getText(col));
	}
}

TransferView::ConnectionInfo::ConnectionInfo(const HintedUser& u, TransferInfo& parent) :
	ItemInfo(),
	UserInfoBase(u),
	transferFailed(false),
	status(STATUS_WAITING),
	parent(parent)
{
	columns[COLUMN_FILE] = parent.getText(COLUMN_FILE);
	columns[COLUMN_PATH] = parent.getText(COLUMN_PATH);
	columns[COLUMN_USER] = WinUtil::getNicks(u);
	columns[COLUMN_HUB] = WinUtil::getHubNames(u).first;
	columns[COLUMN_STATUS] = T_("Idle");
}

bool TransferView::ConnectionInfo::operator==(const ConnectionInfo& other) const {
	return other.parent.download == parent.download && other.getUser() == getUser();
}

int TransferView::ConnectionInfo::getImage(int col) const {
	return col == COLUMN_FILE ? WinUtil::TRANSFER_ICON_USER : ItemInfo::getImage(col);
}

void TransferView::ConnectionInfo::update(const UpdateInfo& ui) {
	if(ui.updateMask & UpdateInfo::MASK_PATH) {
		parent.path = ui.path;
		parent.updatePath();
		columns[COLUMN_FILE] = parent.getText(COLUMN_FILE);
		columns[COLUMN_PATH] = parent.getText(COLUMN_PATH);
	}

	if(ui.updateMask & UpdateInfo::MASK_STATUS) {
		status = ui.status;
	}

	if(ui.updateMask & UpdateInfo::MASK_STATUS_STRING) {
		// No slots etc from transfermanager better than disconnected from connectionmanager
		if(!transferFailed)
			columns[COLUMN_STATUS] = ui.statusString;
		transferFailed = ui.transferFailed;
	}

	if(ui.updateMask & UpdateInfo::MASK_TRANSFERRED) {
		actual = ui.actual;
		transferred = ui.transferred;
		size = ui.size;
		if(transferred > 0) {
			columns[COLUMN_TRANSFERRED] = str(TF_("%1% (%2$0.2f)")
				% Text::toT(Util::formatBytes(transferred))
				% (static_cast<double>(actual) / static_cast<double>(transferred)));
		} else {
			columns[COLUMN_TRANSFERRED].clear();
		}
		if(size > 0) {
			columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(size));
		} else {
			columns[COLUMN_SIZE].clear();
		}
	}

	if((ui.updateMask & UpdateInfo::MASK_STATUS) || (ui.updateMask & UpdateInfo::MASK_SPEED)) {
		speed = std::max(ui.speed, 0LL); // sometimes the speed is negative; avoid problems.
		if(status == STATUS_RUNNING && speed > 0) {
			columns[COLUMN_SPEED] = str(TF_("%1%/s") % Text::toT(Util::formatBytes(speed)));
		} else {
			columns[COLUMN_SPEED].clear();
		}
	}

	if((ui.updateMask & UpdateInfo::MASK_STATUS) || (ui.updateMask & UpdateInfo::MASK_TRANSFERRED) || (ui.updateMask & UpdateInfo::MASK_SPEED)) {
		if(status == STATUS_RUNNING && size > 0 && speed > 0) {
			timeleft = static_cast<double>(size - transferred) / speed;
			columns[COLUMN_TIMELEFT] = Text::toT(Util::formatSeconds(timeleft));
		} else {
			timeleft = 0;
			columns[COLUMN_TIMELEFT].clear();
		}
	}

	if(ui.updateMask & UpdateInfo::MASK_CIPHER) {
		columns[COLUMN_CIPHER] = ui.cipher;
	}

	if(ui.updateMask & UpdateInfo::MASK_IP) {
		columns[COLUMN_IP] = ui.ip;
	}
	
	if(ui.updateMask & UpdateInfo::MASK_COUNTRY) {
		columns[COLUMN_COUNTRY] = ui.country;
	}
}

TransferView::TransferInfo& TransferView::ConnectionInfo::transfer() {
	return parent;
}

double TransferView::ConnectionInfo::barPos() const {
	return status == STATUS_RUNNING && size > 0 && transferred >= 0 ?
		static_cast<double>(transferred) / static_cast<double>(size) : -1;
}

void TransferView::ConnectionInfo::force() {
	columns[COLUMN_STATUS] = T_("Connecting (forced)");
	parent.update();
	ConnectionManager::getInstance()->force(user);
}

void TransferView::ConnectionInfo::disconnect() {
	ConnectionManager::getInstance()->disconnect(user, parent.download);
}

TransferView::TransferInfo::TransferInfo(const TTHValue& tth, bool download, const string& path, const string& tempPath) :
	ItemInfo(),
	tth(tth),
	download(download),
	path(path),
	tempPath(tempPath),
	startPos(0)
{
}

bool TransferView::TransferInfo::operator==(const TransferInfo& other) const {
	return other.download == download && other.path == path;
}

int TransferView::TransferInfo::getImage(int col) const {
	return col == COLUMN_FILE ? WinUtil::getFileIcon(path) : ItemInfo::getImage(col);
}

void TransferView::TransferInfo::update() {
	timeleft = 0;
	speed = 0;
	transferred = startPos;

	if(conns.empty()) {
		// this should never happen, but let's play safe.
		for(auto& col: columns) {
			col.clear();
		}
		return;
	}

	set<string> hubs;
	for(auto& conn: conns) {
		if(!download) { timeleft += conn.timeleft; }
		speed += conn.speed;
		transferred += conn.transferred;
		hubs.insert(conn.getUser().hint);
	}

	if(size == -1) {
		// this can happen with file lists... get the size of the first connection.
		size = conns.front().size;
	}

	if(download && size > 0 && speed > 0) {
		timeleft = static_cast<double>(size - transferred) / speed;
	}

	if(conns.size() == 1) {
		auto& conn = conns.front();
		columns[COLUMN_STATUS] = conn.getText(COLUMN_STATUS);
		columns[COLUMN_USER] = conn.getText(COLUMN_USER);
		columns[COLUMN_HUB] = conn.getText(COLUMN_HUB);
		columns[COLUMN_CIPHER] = conn.getText(COLUMN_CIPHER);
		columns[COLUMN_IP] = conn.getText(COLUMN_IP);
		columns[COLUMN_COUNTRY] = conn.getText(COLUMN_COUNTRY);

	} else {
		auto users = conns.size();
		columns[COLUMN_STATUS] = download ?
			str(TF_("Downloading from %1% users") % users) :
			str(TF_("Uploading to %1% users") % users);
		columns[COLUMN_USER] = str(TF_("%1% users") % users);
		if(hubs.size() == 1) {
			columns[COLUMN_HUB] = conns.front().getText(COLUMN_HUB);
		} else {
			columns[COLUMN_HUB] = str(TF_("%1% hubs") % hubs.size());
		}
	}

	columns[COLUMN_TIMELEFT] = Text::toT(Util::formatSeconds(timeleft));
	columns[COLUMN_SPEED] = str(TF_("%1%/s") % Text::toT(Util::formatBytes(speed)));
	columns[COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(transferred));
	columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(size));
}

void TransferView::TransferInfo::updatePath() {
	columns[COLUMN_FILE] = Text::toT(Util::getFileName(path));
	columns[COLUMN_PATH] = Text::toT(Util::getFilePath(path));
}

TransferView::TransferInfo& TransferView::TransferInfo::transfer() {
	return *this;
}

double TransferView::TransferInfo::barPos() const {
	if(download) {
		return size > 0 && transferred >= 0 ?
			static_cast<double>(transferred) / static_cast<double>(size) : -1;
	} else {
		return conns.size() == 1 ? conns.front().barPos() : -1;
	}
}

void TransferView::TransferInfo::force() {
	for(auto& conn: conns) {
		conn.force();
	}
}

void TransferView::TransferInfo::disconnect() {
	for(auto& conn: conns) {
		conn.disconnect();
	}
}

void TransferView::handleDestroy() {
	SettingsManager::getInstance()->set(SettingsManager::TRANSFERS_ORDER, WinUtil::toString(transfers->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::TRANSFERS_WIDTHS, WinUtil::toString(transfers->getColumnWidths()));
	SettingsManager::getInstance()->set(SettingsManager::TRANSFERS_SORT, WinUtil::getTableSort(transfers));
}

bool TransferView::handleContextMenu(dwt::ScreenCoordinate pt) {
	auto sel = transfers->getSelection();
	if(!sel.empty()) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = transfers->getContextMenuPos();
		}

		auto selData = (sel.size() == 1) ? transfers->getSelectedData() : nullptr;
		auto transfer = dynamic_cast<TransferInfo*>(selData);

		auto menu = addChild(ShellMenu::Seed(WinUtil::Seeds::menu));

		tstring title;
		dwt::IconPtr icon;
		if(selData) {
			title = escapeMenu(selData->getText(transfer ? COLUMN_FILE : COLUMN_USER));
			if(title.empty()) {
				title = escapeMenu(selData->getText(COLUMN_USER));
				icon = WinUtil::fileImages->getIcon(WinUtil::TRANSFER_ICON_USER);
			} else {
				icon = WinUtil::fileImages->getIcon(selData->getImage(COLUMN_FILE));
			}
		} else {
			title = str(TF_("%1% items") % sel.size());
		}
		menu->setTitle(title, icon);

		appendUserItems(mdi, menu.get(), false);

		set<TransferInfo*> files;
		for(auto i: sel) {
			if(!transfer->getText(COLUMN_FILE).empty()) {
				files.insert(&transfers->getData(i)->transfer());
			}
		}
		if(files.size() == 1) {
			menu->appendSeparator();
			transfer = *files.begin();
			WinUtil::addHashItems(menu.get(), transfer->tth, transfer->getText(COLUMN_FILE), transfer->size);
		} else if(!files.empty()) {
			menu->appendSeparator();
			for(auto transfer: files) {
				auto file = transfer->getText(COLUMN_FILE);
				WinUtil::addHashItems(
					menu->appendPopup(file, WinUtil::fileImages->getIcon(transfer->getImage(COLUMN_FILE))),
					transfer->tth, file, transfer->size);
			}
		}

		StringList paths;
		for(auto transfer: files) {
			if(File::getSize(transfer->path) != -1) {
				paths.push_back(transfer->path);
			} else if(!transfer->tempPath.empty() && File::getSize(transfer->tempPath) != -1) {
				paths.push_back(transfer->tempPath);
			}
		}
		menu->appendShellMenu(paths);

		menu->appendSeparator();
		menu->appendItem(T_("&Force attempt"), [this] { handleForce(); });
		menu->appendSeparator();
		menu->appendItem(T_("&Disconnect"), [this] { handleDisconnect(); });

		WinUtil::addCopyMenu(menu.get(), transfers);

		menu->open(pt);
		return true;
	}
	return false;
}

void TransferView::handleForce() {
	HoldRedraw hold { transfers };
	transfers->forEachSelected(&ItemInfo::force);
	transfers->resort();
}

void TransferView::handleDisconnect() {
	transfers->forEachSelected(&ItemInfo::disconnect);
}

void TransferView::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	int i = -1;
	while((i = transfers->getNext(i, LVNI_SELECTED)) != -1) {
		auto conn = dynamic_cast<ConnectionInfo*>(transfers->getData(i));
		if(!conn) { continue; }
		if(!conn->getUser().user->isOnline()) { continue; }

		auto tmp = ucParams;
		tmp["fileFN"] = conn->transfer().path;

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];

		ClientManager::getInstance()->userCommand(conn->getUser(), uc, tmp, true);
	}
}

bool TransferView::handleKeyDown(int c) {
	if(c == VK_DELETE) {
		handleDisconnect();
		return true;
	}
	return false;
}

void TransferView::handleDblClicked() {
	auto users = selectedUsersImpl();
	if(users.size() == 1) {
		users[0]->pm(mdi);
	}
}

namespace { void drawProgress(HDC hdc, const dwt::Rectangle& rcItem, int item, int column,
	const dwt::IconPtr& icon, const tstring& text, double pos, bool download)
{
	// draw something nice...
	COLORREF barBase = download ? SETTING(DOWNLOAD_BG_COLOR) : SETTING(UPLOAD_BG_COLOR);
	COLORREF bgBase = WinUtil::bgColor;
	int mod = (HLS_L(RGB2HLS(bgBase)) >= 128) ? -30 : 30;

	// Dark, medium and light shades
	COLORREF barPal[3] { HLS_TRANSFORM(barBase, -40, 50), barBase, HLS_TRANSFORM(barBase, 40, -30) };

	// Two shades of the background color
	COLORREF bgPal[2] { HLS_TRANSFORM(bgBase, mod, 0), HLS_TRANSFORM(bgBase, mod/2, 0) };

	dwt::FreeCanvas canvas { hdc };

	dwt::Rectangle rc = rcItem;

	// draw background

	{
		dwt::Brush brush { ::CreateSolidBrush(bgPal[1]) };
		auto selectBg(canvas.select(brush));

		dwt::Pen pen { bgPal[0] };
		auto selectPen(canvas.select(pen));

		// TODO Don't draw where the finished part will be drawn
		canvas.rectangle(rc.left(), rc.top() - 1, rc.right(), rc.bottom());
	}

	rc.pos.x += 1;
	rc.size.x -= 2;
	rc.size.y -= 1;

	{
		// draw the icon then shift the rect.
		const long iconSize = 16;
		const long iconTextSpace = 2;

		dwt::Rectangle iconRect { rc.left(), rc.top() + std::max(rc.height() - iconSize, 0L) / 2, iconSize, iconSize };

		canvas.drawIcon(icon, iconRect);

		rc.pos.x += iconSize + iconTextSpace;
		rc.size.x -= iconSize + iconTextSpace;
	}

	dwt::Rectangle textRect;

	if(pos >= 0) {
		// the transfer is ongoing; paint a background to represent that.

		dwt::Brush brush { ::CreateSolidBrush(barPal[1]) };
		auto selectBg(canvas.select(brush));

		{
			dwt::Pen pen { barPal[0] };
			auto selectPen(canvas.select(pen));

			// "Finished" part
			rc.size.x *= pos;

			canvas.rectangle(rc);
		}

		textRect = rc;

		// draw progressbar highlight
		if(rc.width() > 2) {
			dwt::Pen pen { barPal[2], dwt::Pen::Solid, 1 };
			auto selectPen(canvas.select(pen));

			rc.pos.y += 2;
			canvas.moveTo(rc.left() + 1, rc.top());
			canvas.lineTo(rc.right() - 2, rc.top());
		}

	} else {
		textRect = rc;
	}

	// draw status text

	auto bkMode(canvas.setBkMode(true));
	auto& font = download ? WinUtil::downloadFont : WinUtil::uploadFont;
	if(!font.get()) {
		font = WinUtil::font;
	}
	auto selectFont(canvas.select(*font));

	textRect.pos.x += 6;
	textRect.size.x -= 6;

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
		// Let's draw a box if needed...
		auto col = data.iSubItem;
		if(col == COLUMN_STATUS) {
			auto& info = *reinterpret_cast<ItemInfo*>(data.nmcd.lItemlParam);
			auto item = static_cast<int>(data.nmcd.dwItemSpec);
			drawProgress(data.nmcd.hdc, transfers->getRect(item, col, LVIR_BOUNDS), item, col,
				info.transfer().download ? downloadIcon : uploadIcon, info.getText(col),
				info.barPos(), info.transfer().download);
				return CDRF_SKIPDEFAULT;
		}
	}
		// Fall through
	default:
		return CDRF_DODEFAULT;
	}
}

bool TransferView::handleTimer() {
	if(updateList) {
		updateList = false;
		callAsync([this] { execTasks(); });
	}
	return true;
}

void TransferView::layout() {
	transfers->resize(dwt::Rectangle(getClientSize()));
}

void TransferView::addConn(const UpdateInfo& ui) {
	TransferInfo* transfer = nullptr;
	auto conn = findConn(ui.user, ui.download);

	if(ui.updateMask & UpdateInfo::MASK_PATH) {
		// adding a connection we know the transfer of.
		transfer = findTransfer(ui.path, ui.download);
		if(conn && &conn->parent != transfer) {
			removeConn(*conn);
			conn = nullptr;
		}
		if(!transfer) {
			transferItems.emplace_back(ui.tth, ui.download, ui.path, ui.tempPath);
			transfer = &transferItems.back();
			transfers->insert(transfer);
		}
		if(ui.download) {
			QueueManager::getInstance()->getSizeInfo(transfer->size, transfer->startPos, ui.path);
		} else {
			transfer->size = File::getSize(ui.path);
		}

	} else {
		// this connection has just been created; we don't know what file it is for yet.
		if(conn) {
			removeConn(*conn);
			conn = nullptr;
		}
		transferItems.emplace_back(TTHValue(), ui.download, ui.user.user->getCID().toBase32(), Util::emptyString);
		transfer = &transferItems.back();
		transfers->insert(transfer);
	}

	if(!conn) {
		transfer->conns.emplace_back(ui.user, *transfer);
		conn = &transfer->conns.back();

		// only show the child connection item when there are multiple children.
		auto connCount = transfer->conns.size();
		if(connCount > 1) {
			if(connCount == 2) {
				// add the previous child.
				transfers->insertChild(reinterpret_cast<LPARAM>(transfer), reinterpret_cast<LPARAM>(&transfer->conns.front()));
			}
			transfers->insertChild(reinterpret_cast<LPARAM>(transfer), reinterpret_cast<LPARAM>(conn));
		}
	}

	conn->update(ui);
	transfer->update();
}

void TransferView::updateConn(const UpdateInfo& ui) {
	auto conn = findConn(ui.user, ui.download);
	if(conn) {
		conn->update(ui);
		conn->parent.update();
	}
}

void TransferView::removeConn(const UpdateInfo& ui) {
	auto conn = findConn(ui.user, ui.download);
	if(conn) {
		auto& parent = conn->parent;
		auto ungroup = parent.conns.size() == 2;

		removeConn(*conn);

		if(ungroup) {
			transfers->erase(&parent.conns.front());
		}
	}
}

TransferView::ConnectionInfo* TransferView::findConn(const HintedUser& user, bool download) {
	if(!user) { return nullptr; }
	for(auto& transfer: transferItems) {
		if(transfer.download == download) {
			for(auto& conn: transfer.conns) {
				if(conn.getUser() == user) {
					return &conn;
				}
			}
		}
	}
	return nullptr;
}

TransferView::TransferInfo* TransferView::findTransfer(const string& path, bool download) {
	if(path.empty()) { return nullptr; }
	for(auto& transfer: transferItems) {
		if(transfer.download == download && transfer.path == path) {
			return &transfer;
		}
	}
	return nullptr;
}

void TransferView::removeConn(ConnectionInfo& conn) {
	transfers->erase(&conn);

	auto& transfer = conn.parent;

	transfer.conns.remove(conn);

	if(transfer.conns.empty()) {
		removeTransfer(transfer);
	} else {
		transfer.update();
	}
}

void TransferView::removeTransfer(TransferInfo& transfer) {
	transfers->erase(&transfer);
	transferItems.remove(transfer);
}

TransferView::UserInfoList TransferView::selectedUsersImpl() const {
	// AspectUserInfo::usersFromTable won't do because not every item represents a user.
	UserInfoList users;
	transfers->forEachSelectedT([&users](ItemInfo* ii) {
		auto conn = dynamic_cast<ConnectionInfo*>(ii);
		if(conn) {
			users.push_back(conn);
		} else {
			auto transfer = dynamic_cast<TransferInfo*>(ii);
			if(transfer && transfer->conns.size() == 1) {
				users.push_back(&transfer->conns.front());
			}
		}
	});
	return users;
}

void TransferView::execTasks() {
	updateList = false;

	HoldRedraw hold { transfers };

	for(auto& task: tasks) {
		task.first(*task.second);
	}
	tasks.clear();

	transfers->resort();
}

void TransferView::on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) noexcept {
	auto ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	ui->setStatus(ConnectionInfo::STATUS_WAITING);
	ui->setStatusString(T_("Connecting"));

	addedConn(ui);
}

void TransferView::on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) noexcept {
	removedConn(new UpdateInfo(aCqi->getUser(), aCqi->getDownload()));
}

void TransferView::on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) noexcept {
	auto ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	ui->setStatusString(aCqi->getUser().user->isSet(User::OLD_CLIENT) ?
		T_("Remote client does not fully support TTH - cannot download") :
		Text::toT(aReason));

	updatedConn(ui);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) noexcept {
	auto ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	ui->setStatusString((aCqi->getState() == ConnectionQueueItem::CONNECTING) ? T_("Connecting") : T_("Waiting to retry"));

	updatedConn(ui);
}

namespace { tstring getFile(Transfer* t) {
	if(t->getType() == Transfer::TYPE_TREE) {
		return T_("TTH");
	}
	if(t->getType() == Transfer::TYPE_FULL_LIST || t->getType() == Transfer::TYPE_PARTIAL_LIST) {
		return T_("file list");
	}
	return Text::toT(str(F_("(%1%)") % (Util::formatBytes(t->getStartPos()) + " - " +
		Util::formatBytes(t->getStartPos() + t->getSize()))));
} }

void TransferView::on(DownloadManagerListener::Complete, Download* d) noexcept {
	onTransferComplete(d, true);
}

void TransferView::on(DownloadManagerListener::Failed, Download* d, const string& aReason) noexcept {
	onFailed(d, aReason);
}

void TransferView::on(DownloadManagerListener::Starting, Download* d) noexcept {
	auto ui = new UpdateInfo(d->getHintedUser(), true);

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

	updatedConn(ui);
}

void TransferView::on(DownloadManagerListener::Tick, const DownloadList& dl) noexcept  {
	for(auto i: dl) {
		onTransferTick(i, true);
	}
}

void TransferView::on(DownloadManagerListener::Requesting, Download* d) noexcept {
	auto ui = new UpdateInfo(d->getHintedUser(), true);
	starting(ui, d);
	ui->setTempPath(d->getTempTarget());
	ui->setStatusString(str(TF_("Requesting %1%") % getFile(d)));

	addedConn(ui);
}

void TransferView::on(UploadManagerListener::Complete, Upload* u) noexcept {
	onTransferComplete(u, false);
}

void TransferView::on(UploadManagerListener::Starting, Upload* u) noexcept {
	auto ui = new UpdateInfo(u->getHintedUser(), false);
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

	addedConn(ui);
}

void TransferView::on(UploadManagerListener::Tick, const UploadList& ul) noexcept {
	for(auto i: ul) {
		onTransferTick(i, false);
	}
}

void TransferView::on(QueueManagerListener::CRCFailed, Download* d, const string& aReason) noexcept {
	onFailed(d, aReason);
}

void TransferView::addedConn(UpdateInfo* ui) {
	callAsync([this, ui] {
		tasks.emplace_back([=](const UpdateInfo& ui) { addConn(ui); }, unique_ptr<UpdateInfo>(ui));
		updateList = true;
	});
}

void TransferView::updatedConn(UpdateInfo* ui) {
	callAsync([this, ui] {
		tasks.emplace_back([=](const UpdateInfo& ui) { updateConn(ui); }, unique_ptr<UpdateInfo>(ui));
		updateList = true;
	});
}

void TransferView::removedConn(UpdateInfo* ui) {
	callAsync([this, ui] {
		tasks.emplace_back([=](const UpdateInfo& ui) { removeConn(ui); }, unique_ptr<UpdateInfo>(ui));
		updateList = true;
	});
}

void TransferView::starting(UpdateInfo* ui, Transfer* t) {
	ui->setTTH(t->getTTH());
	ui->setFile(t->getPath());
	ui->setStatus(ConnectionInfo::STATUS_RUNNING);
	ui->setTransferred(t->getPos(), t->getActual(), t->getSize());
	const UserConnection& uc = t->getUserConnection();
	ui->setCipher(Text::toT(uc.getCipherName()));
	ui->setIP(Text::toT(uc.getRemoteIp()));
	ui->setCountry(Text::toT(GeoManager::getInstance()->getCountry(uc.getRemoteIp())));
}

void TransferView::onTransferTick(Transfer* t, bool download) {
	auto ui = new UpdateInfo(t->getHintedUser(), download);
	ui->setTransferred(t->getPos(), t->getActual(), t->getSize());
	ui->setSpeed(t->getAverageSpeed());
	updatedConn(ui);
}

void TransferView::onTransferComplete(Transfer* t, bool download) {
	auto ui = new UpdateInfo(t->getHintedUser(), download);
	ui->setFile(t->getPath());
	ui->setStatus(ConnectionInfo::STATUS_WAITING);
	ui->setStatusString(T_("Idle"));
	ui->setTransferred(t->getPos(), t->getActual(), t->getSize());

	updatedConn(ui);
}

void TransferView::onFailed(Download* d, const string& aReason) {
 	auto ui = new UpdateInfo(d->getHintedUser(), true, true);
	ui->setFile(d->getPath());
	ui->setStatus(ConnectionInfo::STATUS_WAITING);
	ui->setStatusString(Text::toT(aReason));

	updatedConn(ui);
}
