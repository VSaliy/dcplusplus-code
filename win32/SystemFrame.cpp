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

#include "SystemFrame.h"

#include <dcpp/File.h>
#include <dcpp/LogManager.h>

#include "HoldRedraw.h"
#include "ShellMenu.h"
#include "WinUtil.h"

const string SystemFrame::id = "SystemLog";
const string& SystemFrame::getId() const { return id; }

SystemFrame::SystemFrame(TabViewPtr parent) :
	BaseType(parent, T_("System Log"), IDH_SYSTEM_LOG, IDI_DCPP),
	log(0)
{
	{
		TextBox::Seed cs = WinUtil::Seeds::textBox;
		cs.style |= WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY;
		log = addChild(cs);
		addWidget(log);
		log->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleContextMenu(sc); });
		log->onLeftMouseDblClick([this](const dwt::MouseEvent &me) { return handleDoubleClick(me); });
		WinUtil::handleDblClicks(log);
	}

	initStatus();

	auto path = Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM)));
	status->onDblClicked(STATUS_STATUS, [path] { WinUtil::openFile(path); });

	layout();

	LogManager::List oldMessages = LogManager::getInstance()->getLastLogs();
	// Technically, we might miss a message or two here, but who cares...
	LogManager::getInstance()->addListener(this);

	for(auto i = oldMessages.begin(); i != oldMessages.end(); ++i) {
		addLine(i->first, Text::toT(i->second));
	}
}

SystemFrame::~SystemFrame() {

}

void SystemFrame::addLine(time_t t, const tstring& msg) {
	bool scroll = log->scrollIsAtEnd();
	HoldRedraw hold(log, !scroll);

	size_t limit = log->getTextLimit();
	if(log->length() + msg.size() > limit) {
		HoldRedraw hold2(log, scroll);
		log->setSelection(0, log->lineIndex(log->lineFromChar(limit / 10)));
		log->replaceSelection(_T(""));
	}
	log->addText(Text::toT("\r\n[" + Util::getShortTimeString(t) + "] ") + msg);

	if(scroll)
		log->scrollToBottom();

	setDirty(SettingsManager::BOLD_SYSTEM_LOG);
}

void SystemFrame::layout() {
	dwt::Rectangle r(this->getClientSize());

	r.size.y -= status->refresh();

	log->resize(r);
}

bool SystemFrame::preClosing() {
	LogManager::getInstance()->removeListener(this);
	return true;
}

bool SystemFrame::handleContextMenu(const dwt::ScreenCoordinate& pt) {
	tstring path = log->textUnderCursor(pt, true);
	string path_a = Text::fromT(path);
	if(File::getSize(path_a) != -1) {
		ShellMenuPtr menu = addChild(ShellMenu::Seed());
		menu->setTitle(escapeMenu(path), WinUtil::fileImages->getIcon(WinUtil::getFileIcon(path_a)));
		menu->appendItem(T_("&Open"), [path] { WinUtil::openFile(path); }, dwt::IconPtr(), true, true);
		menu->appendItem(T_("Open &folder"), [path] { WinUtil::openFolder(path); });
		menu->appendShellMenu(StringList(1, path_a));
		menu->open(pt);
		return true;
	}
	return false;
}

bool SystemFrame::handleDoubleClick(const dwt::MouseEvent& mouseEvent) {
	tstring path = log->textUnderCursor(mouseEvent.pos, true);
	if(File::getSize(Text::fromT(path)) != -1) {
		WinUtil::openFile(path);
		return true;
	}
	return false;
}

void SystemFrame::on(Message, time_t t, const string& message) noexcept {
	callAsync([=] { addLine(t, Text::toT(message)); });
}
