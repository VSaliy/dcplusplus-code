/*
 * Copyright (C) 2012 Jacek Sieka, arnetheduck on gmail point com
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

/* Helpers around the DCTagger interface. */

#include "Tagger.h"

#include "Core.h"

namespace dcapi {

DCTaggerPtr Tagger::tagger;

bool Tagger::init() {
	if(!Core::handle()) { return false; }
	init(reinterpret_cast<DCTaggerPtr>(Core::handle()->query_interface(DCINTF_DCPP_TAGGER, DCINTF_DCPP_TAGGER_VER)));
	return tagger;
}
void Tagger::init(DCTaggerPtr coreTagger) { tagger = coreTagger; }
DCTaggerPtr Tagger::handle() { return tagger; }

} // namespace dcapi
