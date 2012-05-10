/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include "DebugFrame.h"

#include <dcpp/ClientManager.h>
#include <dcpp/File.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Button.h>

#include "HoldRedraw.h"
#include "ShellMenu.h"
#include "WinUtil.h"

#define IDH_DEBUG_FRAME 0

using dwt::Grid;
using dwt::GridInfo;


const string DebugFrame::id = "DebugMessages";
const string& DebugFrame::getId() const { return id; }

DebugFrame::DebugFrame(TabViewPtr parent) :
BaseType(parent, T_("Debug Messages"), IDH_DEBUG_FRAME, IDI_DCPP, false),
grid(0),
debug(0),
clientMsg(0),
showClientMsg(true),
hubMsg(0),
showHubMsg(true),
filterHub(0),
filterByHub(false),
hubs(0)
{
	grid = addChild(Grid::Seed(2, 5));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;
	grid->column(3).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		TextBox::Seed seed = WinUtil::Seeds::textBox;
		seed.style |= WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY | ES_SUNKEN;
		debug = grid->addChild(seed);
		grid->setWidget(debug, 0, 0, 1, 5);
		debug->setTextLimit(1024*64*5);
		addWidget(debug);

		debug->onKeyDown([this](int c) { return handleKeyDown(c); });
	}
	
	{
		ButtonPtr button;
		Button::Seed cs = WinUtil::Seeds::button;
		
		cs.caption = T_("&Clear");
		button = grid->addChild(cs);
		button->onClicked([this] { clearMessages(); });
		addWidget(button);
		
		CheckBox::Seed cb = WinUtil::Seeds::checkBox;
		
		cb.caption = T_("Show client messages");
		clientMsg = grid->addChild(cb);
		clientMsg->setChecked(showClientMsg);
		clientMsg->onClicked([this] { handleClientMsg(); });
		
		cb.caption = T_("Show hub messages");
		hubMsg = grid->addChild(cb);
		hubMsg->setChecked(showHubMsg);
		hubMsg->onClicked([this] { handleHubMsg(); });
		
		cb.caption = T_("Filter by hub");
		filterHub = grid->addChild(cb);
		filterHub->setChecked(filterByHub);
		filterHub->onClicked([this] { handleFilterHub(); });

		ComboBox::Seed cbSeed = WinUtil::Seeds::comboBox;
		hubs = grid->addChild(cbSeed);
		addWidget(hubs);

	}

	initStatus();

	layout();
	activate();

	ClientManager* clientMgr = ClientManager::getInstance();
	{
		auto lock = clientMgr->lock();
		clientMgr->addListener(this);
		auto& clients = clientMgr->getClients();
		for(auto it = clients.begin(); it != clients.end(); ++it) {
			Client* client = *it;
			if(!client->isConnected())
				continue;

			onHubAdded(new HubInfo(client));
		}
	}
	
	updateStatus();
	
	start();
	DebugManager::getInstance()->addListener(this);	
}

DebugFrame::~DebugFrame() {
}

void DebugFrame::layout() {
	dwt::Rectangle r(this->getClientSize());

	r.size.y -= status->refresh();

	grid->resize(r);	
}

void DebugFrame::updateStatus() {	
	
	if(showClientMsg) {
		status->setText(STATUS_CLIENT, T_("Showing client debug messages"));
    } else {
		status->setText(STATUS_CLIENT, T_("Client debug messages are disabled"));
	}

	if(showHubMsg) {
		status->setText(STATUS_HUB, T_("Showing hub debug messages"));
	} else {
		status->setText(STATUS_HUB, T_("Hub debug messages are disabled"));
	}
	
	if(filterByHub) {
		status->setText(STATUS_HUB_ADDRESS, T_("Hub address filtering is enabled"));
	} else {
		status->setText(STATUS_HUB_ADDRESS, T_("Hub address filtering is disabled"));
	}
}

void DebugFrame::addLine(const tstring& msg) {
	bool scroll = debug->scrollIsAtEnd();

	debug->addText(Text::toT("\r\n[" + Util::getTimeString() + "] ") + msg);

	if(scroll)
		debug->scrollToBottom();

	setDirty(SettingsManager::BOLD_SYSTEM_LOG);
}

bool DebugFrame::preClosing() {
	ClientManager::getInstance()->removeListener(this);
	DebugManager::getInstance()->removeListener(this);	
	
	stop = true;
	s.signal();
		
	return true;
}

void DebugFrame::clearMessages() { 
	debug->setSelection();
	debug->replaceSelection(_T(""));
}

void DebugFrame::handleClientMsg() {
	showClientMsg = clientMsg->getChecked();
	updateStatus();
}

void DebugFrame::handleHubMsg() {
	showHubMsg = hubMsg->getChecked();
	updateStatus();
}

void DebugFrame::handleFilterHub() {
	filterByHub = filterHub->getChecked();
	updateStatus();	
}

bool DebugFrame::handleKeyDown(int c) {
	switch(c) {
	case VK_DELETE:
		clearMessages();
		return true;
	}
	return false;
}

void DebugFrame::on(ClientConnected, Client* c) noexcept {
	auto hi = new HubInfo(c);
	callAsync([=] { onHubAdded(hi); });
}

void DebugFrame::on(ClientDisconnected, Client* c) noexcept {
	auto hi = new HubInfo(c);
	callAsync([=] { onHubRemoved(hi); });
}

void DebugFrame::onHubAdded(HubInfo* info) {
	hubs->addValue(info->ip);
}

void DebugFrame::onHubRemoved(HubInfo* info) {
	auto idx = hubs->findString(info->ip);
	hubs->erase(idx);
}

void DebugFrame::on(DebugCommand, const string& message) noexcept {
	callAsync([=] { addLine(Text::toT(message)); });
}
