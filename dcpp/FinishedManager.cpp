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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "FinishedManager.h"

#include "FinishedItem.h"
#include "FinishedManagerListener.h"
#include "Download.h"
#include "Upload.h"
#include "DownloadManager.h"
#include "QueueManager.h"
#include "UploadManager.h"

namespace dcpp {

FinishedManager::FinishedManager() {
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
}

FinishedManager::~FinishedManager() throw() {
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);

	clearDLs();
	clearULs();
}

void FinishedManager::lockLists() {
	cs.enter();
}

const FinishedManager::MapByFile& FinishedManager::getMapByFile(bool upload) const {
	return upload ? ULByFile : DLByFile;
}

const FinishedManager::MapByUser& FinishedManager::getMapByUser(bool upload) const {
	return upload ? ULByUser : DLByUser;
}

void FinishedManager::unLockLists() {
	cs.leave();
}

void FinishedManager::remove(bool upload, const string& file) {
	{
		Lock l(cs);
		MapByFile& map = upload ? ULByFile : DLByFile;
		MapByFile::iterator it = map.find(file);
		if(it != map.end()) {
			delete it->second;
			map.erase(it);
		} else
			return;
	}
	fire(FinishedManagerListener::RemovedFile(), upload, file);
}

void FinishedManager::remove(bool upload, const UserPtr& user) {
	{
		Lock l(cs);
		MapByUser& map = upload ? ULByUser : DLByUser;
		MapByUser::iterator it = map.find(user);
		if(it != map.end()) {
			delete it->second;
			map.erase(it);
		} else
			return;
	}
	fire(FinishedManagerListener::RemovedUser(), upload, user);
}

void FinishedManager::removeAll(bool upload) {
	if(upload)
		clearULs();
	else
		clearDLs();
	fire(FinishedManagerListener::RemovedAll(), upload);
}

void FinishedManager::clearDLs() {
	Lock l(cs);
	for(MapByFile::iterator i = DLByFile.begin(); i != DLByFile.end(); ++i)
		delete i->second;
	for(MapByUser::iterator i = DLByUser.begin(); i != DLByUser.end(); ++i)
		delete i->second;
	DLByFile.clear();
	DLByUser.clear();
}

void FinishedManager::clearULs() {
	Lock l(cs);
	for(MapByFile::iterator i = ULByFile.begin(); i != ULByFile.end(); ++i)
		delete i->second;
	for(MapByUser::iterator i = ULByUser.begin(); i != ULByUser.end(); ++i)
		delete i->second;
	ULByFile.clear();
	ULByUser.clear();
}

void FinishedManager::onComplete(Transfer* t, bool upload, bool crc32Checked) {
	if(t->getType() == Transfer::TYPE_FILE || (t->getType() == Transfer::TYPE_FULL_LIST && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		string file = t->getPath();
		const UserPtr& user = t->getUser();

		int64_t size = upload ? File::getSize(file) : ( t->getType() == Transfer::TYPE_FULL_LIST ? t->getSize() : QueueManager::getInstance()->getSize(file));

		Lock l(cs);

		{
			MapByFile& map = upload ? ULByFile : DLByFile;
			MapByFile::iterator it = map.find(file);
			if(it == map.end()) {
				FinishedFileItemPtr p = new FinishedFileItem(
					t->getPos(),
					GET_TICK() - t->getStart(),
					GET_TIME(),
					size,
					crc32Checked,
					user
					);
				map[file] = p;
				fire(FinishedManagerListener::AddedFile(), upload, file, p);
			} else {
				it->second->update(
					t->getPos(),
					GET_TICK() - t->getStart(),
					GET_TIME(),
					crc32Checked,
					user
					);
				fire(FinishedManagerListener::UpdatedFile(), upload, file);
			}
		}

		{
			MapByUser& map = upload ? ULByUser : DLByUser;
			MapByUser::iterator it = map.find(user);
			if(it == map.end()) {
				FinishedUserItemPtr p = new FinishedUserItem(
					t->getPos(),
					GET_TICK() - t->getStart(),
					GET_TIME(),
					file
					);
				map[user] = p;
				fire(FinishedManagerListener::AddedUser(), upload, user, p);
			} else {
				it->second->update(
					t->getPos(),
					GET_TICK() - t->getStart(),
					GET_TIME(),
					file
					);
				fire(FinishedManagerListener::UpdatedUser(), upload, user);
			}
		}
	}
}

void FinishedManager::on(DownloadManagerListener::Complete, Download* d) throw() {
	onComplete(d, false, d->isSet(Download::FLAG_CRC32_OK));
}

void FinishedManager::on(UploadManagerListener::Complete, Upload* u) throw() {
	onComplete(u, true);
}

} // namespace dcpp
