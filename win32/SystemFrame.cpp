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

#include "SystemFrame.h"

#include <dcpp/LogManager.h>
#include "HoldRedraw.h"
#include "WinUtil.h"

SystemFrame::SystemFrame() : 
	BaseType(T_("System Log"), IDH_SYSTEM_LOG, IDR_MAINFRAME),
	log(0) 
{
	{
		TextBox::Seed cs = WinUtil::Seeds::textBox;
		cs.style |= WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY;
		log = addChild(cs);
		addWidget(log);
	}

	initStatus();
	status->onDblClicked(std::tr1::bind(&WinUtil::openFile, Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM)))));

	layout();
	
	LogManager::List oldMessages = LogManager::getInstance()->getLastLogs();
	// Technically, we might miss a message or two here, but who cares...
	LogManager::getInstance()->addListener(this);

	onSpeaker(std::tr1::bind(&SystemFrame::handleSpeaker, this, _1));
	
	for(LogManager::List::const_iterator i = oldMessages.begin(); i != oldMessages.end(); ++i) {
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
		log->sendMessage(WM_VSCROLL, SB_BOTTOM);

	setDirty(SettingsManager::BOLD_SYSTEM_LOG);
}

void SystemFrame::layout() {
	bool scroll = log->scrollIsAtEnd();

	dwt::Rectangle r(this->getClientAreaSize());

	layoutStatus(r);

	log->setBounds(r);

	if(scroll)
		log->sendMessage(WM_VSCROLL, SB_BOTTOM);
}

LRESULT SystemFrame::handleSpeaker(WPARAM wp) {
	boost::scoped_ptr<LogManager::Pair> msg(reinterpret_cast<LogManager::Pair*>(wp));
	addLine(msg->first, Text::toT(msg->second));
	return 0;
}

bool SystemFrame::preClosing() {
	LogManager::getInstance()->removeListener(this);
	return true;	
}

void SystemFrame::on(Message, time_t t, const string& message) throw() { 
	speak(reinterpret_cast<WPARAM>(new LogManager::Pair(t, message))); 
}
