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

#include "stdafx.h"

#include "resource.h"

#include "HashProgressDlg.h"

#include <dwt/widgets/ProgressBar.h>

#include <dcpp/HashManager.h>
#include "WinUtil.h"

HashProgressDlg::HashProgressDlg(dwt::Widget* parent, bool aAutoClose) :
dwt::ModalDialog(parent),
grid(0),
file(0),
stat(0),
speed(0),
left(0),
progress(0),
pauseResume(0),
autoClose(aAutoClose)
{
	onInitDialog(std::tr1::bind(&HashProgressDlg::handleInitDialog, this));
	onHelp(std::tr1::bind(&WinUtil::help, _1, _2));
}

HashProgressDlg::~HashProgressDlg() {
	HashManager::getInstance()->setPriority(Thread::IDLE);
}

int HashProgressDlg::run() {
	create(Seed(dwt::Point(657, 185), DS_CONTEXTHELP));
	return show();
}

bool HashProgressDlg::handleInitDialog() {
	setHelpId(IDH_HASH_PROGRESS);

	grid = addChild(Grid::Seed(5, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(3).size = 20;
	grid->row(3).mode = GridInfo::STATIC;
	grid->row(3).align = GridInfo::STRETCH;

	grid->addChild(Label::Seed(T_("Please wait while DC++ indexes your files (they won't be shared until they've been indexed)...")));

	file = grid->addChild(Label::Seed());

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Statistics")))->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;

		Label::Seed seed;
		stat = cur->addChild(seed);
		speed = cur->addChild(seed);
		left = cur->addChild(seed);
	}

	progress = grid->addChild(ProgressBar::Seed());
	progress->setRange(0, 10000);

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 3));
		cur->column(2).mode = GridInfo::FILL;
		cur->column(2).align = GridInfo::BOTTOM_RIGHT;

		pair<ButtonPtr, ButtonPtr> buttons = WinUtil::addDlgButtons(cur,
			std::tr1::bind(&HashProgressDlg::endDialog, this, IDOK),
			std::tr1::bind(&HashProgressDlg::endDialog, this, IDCANCEL));
		buttons.first->setText(T_("Run in background"));
		buttons.second->setVisible(false);

		pauseResume = cur->addChild(Button::Seed());
		pauseResume->onClicked(std::tr1::bind(&HashProgressDlg::handlePauseResume, this));
		setButtonState();
	}

	string tmp;
	startTime = GET_TICK();
	HashManager::getInstance()->getStats(tmp, startBytes, startFiles);
	updateStats();

	HashManager::getInstance()->setPriority(Thread::NORMAL);
	createTimer(std::tr1::bind(&HashProgressDlg::updateStats, this), 1000);

	setText(T_("Creating file index..."));

	layout();
	centerWindow();

	return false;
}

bool HashProgressDlg::updateStats() {
	string path;
	int64_t bytes = 0;
	size_t files = 0;
	uint32_t tick = GET_TICK();

	HashManager::getInstance()->getStats(path, bytes, files);
	if(bytes > startBytes)
		startBytes = bytes;

	if(files > startFiles)
		startFiles = files;

	if(autoClose && files == 0) {
		close(true);
		return true;
	}
	double diff = tick - startTime;
	bool paused = HashManager::getInstance()->isHashingPaused();
	if(diff < 1000 || files == 0 || bytes == 0 || paused) {
		stat->setText(str(TF_("-.-- files/h, %1% files left") % (uint32_t)files));
		speed->setText(str(TF_("-.-- B/s, %1% left") % Text::toT(Util::formatBytes(bytes))));
		if(paused) {
			left->setText(T_("Paused"));
		} else {
			left->setText(T_("-:--:-- left"));
			progress->setPosition(0);
		}
	} else {
		double filestat = (((double)(startFiles - files)) * 60 * 60 * 1000) / diff;
		double speedStat = (((double)(startBytes - bytes)) * 1000) / diff;

		stat->setText(str(TF_("%1% files/h, %2% files left") % filestat % (uint32_t)files));
		speed->setText(str(TF_("%1%/s, %2% left") % Text::toT(Util::formatBytes((int64_t)speedStat)) % Text::toT(Util::formatBytes(bytes))));

		if(filestat == 0 || speedStat == 0) {
			left->setText(T_("-:--:-- left"));
		} else {
			double fs = files * 60 * 60 / filestat;
			double ss = bytes / speedStat;
			left->setText(str(TF_("%1% left") % Text::toT(Util::formatSeconds((int64_t)(fs + ss) / 2))));
		}
	}

	if(files == 0) {
		file->setText(T_("Done"));
	} else {
		file->setText(Text::toT(path));
	}

	if(startFiles == 0 || startBytes == 0) {
		progress->setPosition(0);
	} else {
		progress->setPosition((int)(10000 * ((0.5 * (startFiles - files)/startFiles) + 0.5 * (startBytes - bytes) / startBytes)));
	}

	return true;
}

void HashProgressDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}

void HashProgressDlg::handlePauseResume() {
	if(HashManager::getInstance()->isHashingPaused()) {
		HashManager::getInstance()->resumeHashing();
	} else {
		HashManager::getInstance()->pauseHashing();
	}

	setButtonState();
	layout();
}

void HashProgressDlg::setButtonState() {
	pauseResume->setText(HashManager::getInstance()->isHashingPaused() ? T_("Resume hashing") : T_("Pause hashing"));
}
