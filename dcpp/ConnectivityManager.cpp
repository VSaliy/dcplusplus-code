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

#include "stdinc.h"
#include "ConnectivityManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "format.h"

namespace dcpp {

ConnectivityManager::ConnectivityManager() :
autoDetected(false),
running(false)
{
}

void ConnectivityManager::startSocket() {
	autoDetected = false;

	disconnect();

	if(ClientManager::getInstance()->isActive()) {
		listen();

		// must be done after listen calls; otherwise ports won't be set
		if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP)
			MappingManager::getInstance()->open();
	}
}

void ConnectivityManager::detectConnection() {
	if(running)
		return;
	running = true;

	status.clear();
	fire(ConnectivityManagerListener::Started());

	// restore connectivity settings to their default value.
	SettingsManager::getInstance()->unset(SettingsManager::TCP_PORT);
	SettingsManager::getInstance()->unset(SettingsManager::UDP_PORT);
	SettingsManager::getInstance()->unset(SettingsManager::TLS_PORT);
	SettingsManager::getInstance()->unset(SettingsManager::EXTERNAL_IP);
	SettingsManager::getInstance()->unset(SettingsManager::NO_IP_OVERRIDE);
	SettingsManager::getInstance()->unset(SettingsManager::BIND_ADDRESS);

	if (MappingManager::getInstance()->getOpened()) {
		MappingManager::getInstance()->close();
	}

	disconnect();

	log(_("Determining the best connectivity settings..."));
	try {
		listen();
	} catch(const Exception& e) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
		log(str(F_("Unable to open %1% port(s); connectivity settings must be configured manually") % e.getError()));
		fire(ConnectivityManagerListener::Finished());
		running = false;
		return;
	}

	autoDetected = true;

	if (!Util::isPrivateIp(Util::getLocalIp())) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_DIRECT);
		log(_("Public IP address detected, selecting active mode with direct connection"));
		fire(ConnectivityManagerListener::Finished());
		running = false;
		return;
	}

	SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_UPNP);
	log(_("Local network with possible NAT detected, trying to map the ports..."));

	if (!MappingManager::getInstance()->open()) {
		running = false;
	}
}

void ConnectivityManager::setup(bool settingsChanged) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		if (!autoDetected) detectConnection();
	} else {
		if(autoDetected || settingsChanged) {
			if(settingsChanged || (SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_FIREWALL_UPNP)) {
				MappingManager::getInstance()->close();
			}
			startSocket();
		} else if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP && !MappingManager::getInstance()->getOpened()) {
			// previous mappings had failed; try again
			MappingManager::getInstance()->open();
		}
	}
}

void ConnectivityManager::mappingFinished(const string& mapper) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		if(mapper.empty()) {
			disconnect();
			SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
			log(_("Active mode could not be achieved; a manual configuration is recommended for better connectivity"));
		} else {
			SettingsManager::getInstance()->set(SettingsManager::MAPPER, mapper);
		}
		fire(ConnectivityManagerListener::Finished());
	}

	running = false;
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

void ConnectivityManager::log(const string& message) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		status = move(message);
		LogManager::getInstance()->message(_("Connectivity: ") + status);
		fire(ConnectivityManagerListener::Message(), status);
	} else {
		LogManager::getInstance()->message(message);
	}
}

} // namespace dcpp
