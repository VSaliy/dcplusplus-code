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

/** @file implement UI-specific functions of the plugin API. */

#include "stdafx.h"

#include <dcpp/PluginApiImpl.h>

#include <dcpp/Text.h>

#include "MainWindow.h"
#include "WinUtil.h"

namespace dcpp {

// Functions for DCUI
void PluginApiImpl::addCommand(const char* name, DCCommandFunc command) {
	MainWindow::addPluginCommand(Text::toT(name), command);
}

void PluginApiImpl::removeCommand(const char* name) {
	MainWindow::removePluginCommand(Text::toT(name));
}

void PluginApiImpl::playSound(const char* path) {
	WinUtil::playSound(Text::toT(path));
}

} // namespace dcpp
