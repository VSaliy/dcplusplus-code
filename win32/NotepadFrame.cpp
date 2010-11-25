/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#include "NotepadFrame.h"

#include <dcpp/File.h>
#include <dcpp/Text.h>

const string NotepadFrame::id = "Notepad";
const string& NotepadFrame::getId() const { return id; }

NotepadFrame::NotepadFrame(dwt::TabView* mdiParent) :
	BaseType(mdiParent, T_("Notepad"), IDH_NOTEPAD, IDI_NOTEPAD),
	pad(0)
{
	{
		TextBox::Seed cs = WinUtil::Seeds::textBox;
		cs.style |= WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_WANTRETURN;
		pad = addChild(cs);
		addWidget(pad, false, false);
		WinUtil::handleDblClicks(pad);
	}

	initStatus();

	pad->setTextLimit(0);
	try {
		pad->setText(Text::toT(File(Util::getNotepadFile(), File::READ, File::OPEN).read()));
	} catch(const FileException& e) {
		status->setText(STATUS_STATUS, str(TF_("Error loading the notepad file: %1%") % Text::toT(e.getError())));
	}

	pad->setModify(false);
	SettingsManager::getInstance()->addListener(this);

	layout();
	activate();
}

NotepadFrame::~NotepadFrame() {
}

bool NotepadFrame::preClosing() {
	SettingsManager::getInstance()->removeListener(this);
	save();
	return true;
}

void NotepadFrame::layout() {
	dwt::Rectangle r(dwt::Point(0, 0), getClientSize());

	status->layout(r);

	pad->layout(r);
}

void NotepadFrame::save() {
	if(pad->getModify()) {
		try {
			dcdebug("Writing notepad contents\n");
			File(Util::getNotepadFile(), File::WRITE, File::CREATE | File::TRUNCATE).write(Text::fromT(pad->getText()));
		} catch(const FileException& e) {
			dcdebug("Writing failed: %s\n", e.getError().c_str());
			///@todo Notify user
		}
	}
}

void NotepadFrame::on(SettingsManagerListener::Save, SimpleXML&) throw() {
	callAsync([this] { save(); });
}
