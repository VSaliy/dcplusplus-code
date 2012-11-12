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

/* Helpers around the DCLog interface. */

#ifndef PLUGINSDK_LOGGER_H
#define PLUGINSDK_LOGGER_H

#include <string>

#include <pluginsdk/PluginDefs.h>

namespace dcapi {

using std::string;

class Logger
{
public:
	static bool init();
	static void init(DCLogPtr coreLogger);
	static DCLogPtr handle();

	static void log(const string& message);

private:
	static DCLogPtr logger;
};

} // namespace dcapi

#endif
