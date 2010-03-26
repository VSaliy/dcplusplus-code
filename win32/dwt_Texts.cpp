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

#include <dwt/Texts.h>

#include <dcpp/Text.h>

namespace dwt {

tstring Texts::get(Text text) {
	switch(text) {
	case undo: return T_("&Undo\tCtrl+Z");
	case cut: return T_("Cu&t\tCtrl+X");
	case copy: return T_("&Copy\tCtrl+C");
	case paste: return T_("&Paste\tCtrl+V");
	case del: return T_("&Delete\tDel");
	case selAll: return T_("Select &All\tCtrl+A");
	}

	assert(0);
	return tstring();
}

}
