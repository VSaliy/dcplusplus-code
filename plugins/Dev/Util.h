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

#ifndef PLUGINS_DEV_UTIL_H
#define PLUGINS_DEV_UTIL_H

#ifdef _WIN32
# define PATH_SEPARATOR '\\'
# define PATH_SEPARATOR_STR "\\"
#else
# define PATH_SEPARATOR '/'
# define PATH_SEPARATOR_STR "/"
# include <sys/stat.h> 
#endif

#ifdef _WIN32
# ifdef _MSC_VER
#  define snprintf _snprintf
#  define snwprintf _snwprintf
# endif
#elif __GNUC__
# define stricmp strcasecmp
# define strnicmp strncasecmp
#else
# error No supported compiler found
#endif

using std::string;

class Util
{
public:
	static void initialize(DCUtilsPtr coreUtils, DCConfigPtr coreConfig, DCLogPtr coreLogger) {
		utils = coreUtils;
		config = coreConfig;
		logger = coreLogger;
	}

	static string toString(uint16_t val) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
		return buf;
	}

	static string toString(int32_t val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}

	static bool fileExists(const string& aFile) {
#ifdef _WIN32
		DWORD attr = GetFileAttributesA(aFile.c_str());
		return (attr != 0xFFFFFFFF);
#else
		struct stat fi;
		return (stat(aFile.c_str(), &fi) == 0);
#endif
	}

	// API Stuff
	static string getPath(PathType type) {
		return config->get_path(type);
	}

	static void logMessage(const string& message) {
		logger->log(message.c_str());
	}

	static void setConfig(const char* name, const char* value);
	static void setConfig(const char* name, const string& value) { setConfig(name, value.c_str()); }
	static void setConfig(const char* name, bool state) { setConfig(name, string(state ? "1" : "0")); }

	static string getConfig(const char *name);
	static bool getBoolConfig(const char* name);

	static ConfigValuePtr getCoreConfig(const char* name);
	static void freeCoreConfig(ConfigValuePtr val) { config->release(val); }

	static string fromT(const tstring& str);
	static tstring toT(const string& str);

private:
	static DCUtilsPtr utils;
	static DCConfigPtr config;
	static DCLogPtr logger;
};

#define SETTING(k) Util::getConfig(#k)
#define BOOLSETTING(k) (Util::getBoolConfig(#k))

#endif
