/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "ConnectivityManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "debug.h"
#include "format.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "version.h"

namespace dcpp {

ConnectivityManager::ConnectivityManager() :
autoDetectedV4(false),
autoDetectedV6(false),
runningV4(false),
runningV6(false),
mapperV4(false),
mapperV6(true)
{
}

bool ConnectivityManager::get(SettingsManager::BoolSetting setting) const {
	if(SETTING(AUTO_DETECT_CONNECTION) || SETTING(AUTO_DETECT_CONNECTION6)) {
		Lock l(cs);
		auto i = autoSettings.find(setting);
		if(i != autoSettings.end()) {
			return boost::get<bool>(i->second);
		}
	}
	return SettingsManager::getInstance()->get(setting);
}

int ConnectivityManager::get(SettingsManager::IntSetting setting) const {
	if(SETTING(AUTO_DETECT_CONNECTION) || SETTING(AUTO_DETECT_CONNECTION6)) {
		Lock l(cs);
		auto i = autoSettings.find(setting);
		if(i != autoSettings.end()) {
			return boost::get<int>(i->second);
		}
	}
	return SettingsManager::getInstance()->get(setting);
}

const string& ConnectivityManager::get(SettingsManager::StrSetting setting) const {
	if(SETTING(AUTO_DETECT_CONNECTION) || SETTING(AUTO_DETECT_CONNECTION6)) {
		Lock l(cs);
		auto i = autoSettings.find(setting);
		if(i != autoSettings.end()) {
			return boost::get<const string&>(i->second);
		}
	}
	return SettingsManager::getInstance()->get(setting);
}

void ConnectivityManager::set(SettingsManager::StrSetting setting, const string& str) {
	if(SETTING(AUTO_DETECT_CONNECTION) || SETTING(AUTO_DETECT_CONNECTION6)) {
		Lock l(cs);
		autoSettings[setting] = str;
	} else {
		SettingsManager::getInstance()->set(setting, str);
	}
}

void ConnectivityManager::clearAutoSettings(bool v6, bool resetDefaults) {
	int settings6[] = { SettingsManager::EXTERNAL_IP6, SettingsManager::BIND_ADDRESS6, 
		SettingsManager::NO_IP_OVERRIDE6, SettingsManager::INCOMING_CONNECTIONS6 };

	int settings4[] = { SettingsManager::EXTERNAL_IP, SettingsManager::NO_IP_OVERRIDE,
		SettingsManager::BIND_ADDRESS, SettingsManager::INCOMING_CONNECTIONS };

	int portSettings[] = { SettingsManager::TCP_PORT, SettingsManager::UDP_PORT,
		SettingsManager::TLS_PORT };


	Lock l(cs);

	//erase the old settings first
	for(const auto setting: v6 ? settings6 : settings4) {
		autoSettings.erase(setting);
	}
	for(const auto setting: portSettings) {
		autoSettings.erase(setting);
	}

	if (resetDefaults) {
		for(const auto setting: v6 ? settings6 : settings4) {
			if(setting >= SettingsManager::STR_FIRST && setting < SettingsManager::STR_LAST) {
				autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::StrSetting>(setting));
			} else if(setting >= SettingsManager::INT_FIRST && setting < SettingsManager::INT_LAST) {
				autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(setting));
			} else if(setting >= SettingsManager::BOOL_FIRST && setting < SettingsManager::BOOL_LAST) {
				autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::BoolSetting>(setting));
			} else {
				dcassert(0);
			}
		}

		for(const auto setting: portSettings)
			autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(setting));
	}
}

void ConnectivityManager::detectConnection() {
	if(isRunning())
		return;

	bool detectV4 = false;
	if (SETTING(AUTO_DETECT_CONNECTION)) {
		detectV4 = true;
		runningV4 = true;
	}

	bool detectV6 = false;
	if (SETTING(AUTO_DETECT_CONNECTION6)) {
		detectV6 = true;
		runningV6 = true;
	}

	if (detectV4) {
		statusV4.clear();
		fire(ConnectivityManagerListener::Started(), false);
	}

	if (detectV6) {
		statusV6.clear();
		fire(ConnectivityManagerListener::Started(), true);
	}

	if(detectV4 && mapperV4.getOpened()) {
		mapperV4.close();
	}

	if(detectV4 && mapperV6.getOpened()) {
		mapperV6.close();
	}

	disconnect();

	// restore auto settings to their default value.
	if (detectV6)
		clearAutoSettings(true, true);
	if (detectV4)
		clearAutoSettings(false, true);

	log(_("Determining the best connectivity settings..."), TYPE_BOTH);
	try {
		listen();
	} catch(const Exception& e) {
		{
			Lock l(cs);
			if (detectV4)
				autoSettings[SettingsManager::INCOMING_CONNECTIONS] = SettingsManager::INCOMING_PASSIVE;
			if (detectV6)
				autoSettings[SettingsManager::INCOMING_CONNECTIONS6] = SettingsManager::INCOMING_PASSIVE;
		}

		log(str(F_("Unable to open %1% port(s); connectivity settings must be configured manually") % e.getError()), TYPE_NORMAL);
		fire(ConnectivityManagerListener::Finished(), false, true);
		fire(ConnectivityManagerListener::Finished(), true, true);
		if (detectV4)
			runningV4 = false;
		if (detectV6)
			runningV6 = false;
		return;
	}

	autoDetectedV4 = detectV4;
	autoDetectedV6 = detectV6;

	if(detectV4 && !Util::getLocalIp(false, false).empty()) {
		{
			Lock l(cs);
			autoSettings[SettingsManager::INCOMING_CONNECTIONS] = SettingsManager::INCOMING_ACTIVE;
		}

		log(_("Public IP address detected, selecting active mode with direct connection"), TYPE_V4);
		fire(ConnectivityManagerListener::Finished(), false, false);
		runningV4 = false;
		detectV4 = false;
	}

	if(detectV6) {
		if(!Util::getLocalIp(true, false).empty()) {
			{
				Lock l(cs);
				autoSettings[SettingsManager::INCOMING_CONNECTIONS6] = SettingsManager::INCOMING_ACTIVE;
			}

			log(_("Public IP address detected, selecting active mode with direct connection"), TYPE_V6);
		} else {
			//disable IPv6 if no public IP address is available
			{
				Lock l(cs);
				autoSettings[SettingsManager::INCOMING_CONNECTIONS6] = SettingsManager::INCOMING_DISABLED;
			}
			log(_("IPv6 connectivity has been disabled as no public IPv6 address was detected"), TYPE_V6);
		}

		fire(ConnectivityManagerListener::Finished(), true, false);
		runningV6 = false;
		detectV6 = false;
	}

	if(!detectV6 && !detectV4)
		return;

	if(detectV4) {
		Lock l(cs);
		autoSettings[SettingsManager::INCOMING_CONNECTIONS] = SettingsManager::INCOMING_ACTIVE_UPNP;
	}

	if(detectV6) {
		Lock l(cs);
		autoSettings[SettingsManager::INCOMING_CONNECTIONS6] = SettingsManager::INCOMING_ACTIVE_UPNP;
	}

	log(_("Local network with possible NAT detected, trying to map the ports..."), (detectV4 && detectV6 ? TYPE_BOTH : detectV4 ? TYPE_V4 : TYPE_V6));

	if(detectV6) {
		startMapping(true);
	}
	if(detectV4) {
		startMapping(false);
	}
}

void ConnectivityManager::setup(bool v4SettingsChanged, bool v6SettingsChanged) {
	bool autoDetect4 = SETTING(AUTO_DETECT_CONNECTION);
	bool autoDetect6 = SETTING(AUTO_DETECT_CONNECTION6);

	if(v4SettingsChanged || (autoDetectedV4 && !autoDetect4)) {
		mapperV4.close();
		autoDetectedV4 = false;
	}

	if(v6SettingsChanged || (autoDetectedV6 && !autoDetect6)) {
		mapperV6.close();
		autoDetectedV6 = false;
	}


	if(!autoDetect6) {
		clearAutoSettings(true, false);
	}
		
	if(!autoDetect4) {
		clearAutoSettings(false, false);
	}

	bool autoDetect = false;
	if(autoDetect4  || autoDetect6) {
		if ((!autoDetectedV4 && autoDetect4) || (!autoDetectedV6 && autoDetect6) || autoSettings.empty()) {
			detectConnection();
			autoDetect = true;
		}
	}

	if(!autoDetect && (v4SettingsChanged || v6SettingsChanged)) {
		startSocket();
	}

	if(!autoDetect4 && SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_ACTIVE_UPNP && !runningV4) // previous mappings had failed; try again
		startMapping(false);

	if(!autoDetect6 && SETTING(INCOMING_CONNECTIONS6) == SettingsManager::INCOMING_ACTIVE_UPNP && !runningV4) // previous mappings had failed; try again
		startMapping(true);
}

void ConnectivityManager::close() {
	mapperV4.close();
	mapperV6.close();
}

void ConnectivityManager::editAutoSettings() {
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, false);

	auto sm = SettingsManager::getInstance();
	for(auto i = autoSettings.cbegin(), iend = autoSettings.cend(); i != iend; ++i) {
		if(i->first >= SettingsManager::STR_FIRST && i->first < SettingsManager::STR_LAST) {
			sm->set(static_cast<SettingsManager::StrSetting>(i->first), boost::get<const string&>(i->second));
		} else if(i->first >= SettingsManager::INT_FIRST && i->first < SettingsManager::INT_LAST) {
			sm->set(static_cast<SettingsManager::IntSetting>(i->first), boost::get<int>(i->second));
		}
	}
	autoSettings.clear();

	fire(ConnectivityManagerListener::SettingChanged());
}

string ConnectivityManager::getInformation() const {
	if(isRunning()) {
		return _("Connectivity settings are being configured; try again later");
	}

	string autoStatusV4 = ok(false) ? str(F_("enabled - %1%") % getStatus(false)) : _("disabled");
	string autoStatusV6 = ok(true) ? str(F_("enabled - %1%") % getStatus(true)) : _("disabled");

	auto getMode = [&](bool v6) -> string { 
		switch(v6 ? CONNSETTING(INCOMING_CONNECTIONS6) : CONNSETTING(INCOMING_CONNECTIONS)) {
		case SettingsManager::INCOMING_ACTIVE: {
				return _("Direct connection to the Internet (no router or manual router configuration)");
				break;
			}
		case SettingsManager::INCOMING_ACTIVE_UPNP: {
				return str(F_("Active mode behind a router that %1% can configure; port mapping status: %2%") % APPNAME % (v6 ? mapperV6.getStatus() : mapperV4.getStatus()));
				break;
			}
		case SettingsManager::INCOMING_PASSIVE: {
				return _("Passive mode");
				break;
			}
		default:
			return _("Disabled");
		}
	};

	auto field = [](const string& s) { return s.empty() ? _("undefined") : s; };

	return str(F_(
		"Connectivity information:\n\n"
		"Automatic connectivity setup (v4) is: %1%\n"
		"Automatic connectivity setup (v6) is: %2%\n\n"
		"\tMode (v4): %3%\n"
		"\tMode (v6): %4%\n"
		"\tExternal IP (v4): %5%\n"
		"\tExternal IP (v6): %6%\n"
		"\tBound interface (v4): %7%\n"
		"\tBound interface (v6): %8%\n"
		"\tTransfer port: %9%\n"
		"\tEncrypted transfer port: %10%\n"
		"\tSearch port: %11%") % autoStatusV4 % autoStatusV6 % getMode(false) % getMode(true) %
		field(CONNSETTING(EXTERNAL_IP)) % field(CONNSETTING(EXTERNAL_IP6)) %
		field(CONNSETTING(BIND_ADDRESS)) % field(CONNSETTING(BIND_ADDRESS6)) %
		field(ConnectionManager::getInstance()->getPort()) % field(ConnectionManager::getInstance()->getSecurePort()) %
		field(SearchManager::getInstance()->getPort()));
}

void ConnectivityManager::startMapping(bool v6) {
	if (v6) {
		runningV6 = true;
		if(!mapperV6.open()) {
			runningV6 = false;
		}
	} else {
		runningV4 = true;
		if(!mapperV4.open()) {
			runningV4 = false;
		}
	}
}

void ConnectivityManager::mappingFinished(const string& mapper, bool v6) {
	if((SETTING(AUTO_DETECT_CONNECTION) && !v6) || (SETTING(AUTO_DETECT_CONNECTION6) && v6)) {
		if(mapper.empty()) {
			{
				Lock l(cs);
				autoSettings[v6 ? SettingsManager::INCOMING_CONNECTIONS6 : SettingsManager::INCOMING_CONNECTIONS] = SettingsManager::INCOMING_PASSIVE;
			}
			log(_("Active mode could not be achieved; a manual configuration is recommended for better connectivity"), v6 ? TYPE_V6 : TYPE_V4);
		} else {
			SettingsManager::getInstance()->set(SettingsManager::MAPPER, mapper);
		}
		fire(ConnectivityManagerListener::Finished(), v6, mapper.empty());
	}

	if (v6)
		runningV6 = false;
	else
		runningV4 = false;
}

void ConnectivityManager::log(const string& message, LogType aType) {

	if (aType == TYPE_NORMAL) {
		LogManager::getInstance()->message(message);
	} else {
		string proto;

		if (aType == TYPE_BOTH && runningV4 && runningV6) {
			statusV6 = message;
			statusV4 = message;
			proto = "IPv4 & IPv6";
		} else if (aType == TYPE_V4 || (aType == TYPE_BOTH && runningV4)) {
			proto = "IPv4";
			statusV4 = message;
		} else if (aType == TYPE_V6 || (aType == TYPE_BOTH && runningV6)) {
			proto = "IPv6";
			statusV6 = message;
		}

		LogManager::getInstance()->message(_("Connectivity:  (") + proto + _("): ") + message);
		fire(ConnectivityManagerListener::Message(), proto + ": " + message);
	}
}

const string& ConnectivityManager::getStatus(bool v6) const { 
	return v6 ? statusV6 : statusV4; 
}

StringList ConnectivityManager::getMappers(bool v6) const {
	if (v6) {
		return mapperV6.getMappers();
	} else {
		return mapperV4.getMappers();
	}
}

void ConnectivityManager::startSocket() {
	autoDetectedV4 = false;
	autoDetectedV6 = false;

	disconnect();

	if(ClientManager::getInstance()->isActive()) {
		listen();

		// must be done after listen calls; otherwise ports won't be set
		startMapping();
	}
}

void ConnectivityManager::startMapping() {
	if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_ACTIVE_UPNP && !runningV4)
		startMapping(false);

	if(SETTING(INCOMING_CONNECTIONS6) == SettingsManager::INCOMING_ACTIVE_UPNP && !runningV6)
		startMapping(true);
}

void ConnectivityManager::listen() {
	try {
		ConnectionManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception(_("Transfer (TCP)"));
	}

	try {
		SearchManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception(_("Search (UDP)"));
	}
}

void ConnectivityManager::disconnect() {
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();
}

} // namespace dcpp
