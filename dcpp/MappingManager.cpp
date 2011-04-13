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
#include "MappingManager.h"

#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "LogManager.h"
#include "SearchManager.h"
#include "ScopedFunctor.h"
#include "version.h"
#include "format.h"

namespace dcpp {

StringList MappingManager::getMappers() const {
	StringList ret;
	for(auto i = mappers.cbegin(), iend = mappers.cend(); i != iend; ++i)
		ret.push_back(i->first);
	return ret;
}

bool MappingManager::open() {
	if(getOpened())
		return false;

	if(mappers.empty()) {
		log(_("No port mapping interface available"));
		return false;
	}

	if(busy.test_and_set()) {
		log(_("Another port mapping attempt is in progress..."));
		return false;
	}

	start();

	return true;
}

void MappingManager::close() {
	TimerManager::getInstance()->removeListener(this);

	if(working.get()) {
		close(*working);
		working.reset();
	}
}

bool MappingManager::getOpened() const {
	return working.get();
}

int MappingManager::run() {
	ScopedFunctor([this] { busy.clear(); });

	// cache these
	const unsigned short
		conn_port = ConnectionManager::getInstance()->getPort(),
		secure_port = ConnectionManager::getInstance()->getSecurePort(),
		search_port = SearchManager::getInstance()->getPort();

	if(renewal) {
		Mapper& mapper = *working;

		ScopedFunctor([&mapper] { mapper.uninit(); });
		if(!mapper.init()) {
			// can't renew; try again later.
			renewal = GET_TICK() + std::max(mapper.renewal(), 10u) * 60 * 1000;
			return 0;
		}

		auto addRule = [this, &mapper](const unsigned short port, Mapper::Protocol protocol, const string& description) {
			// just launch renewal requests - don't bother with possible failures.
			if(port) {
				mapper.open(port, protocol, str(F_("%1% %2% port (%3% %4%)") %
					APPNAME % description % port % Mapper::protocols[protocol]));
			}
		};

		addRule(conn_port, Mapper::PROTOCOL_TCP, _("Transfer"));
		addRule(secure_port, Mapper::PROTOCOL_TCP, _("Encrypted transfer"));
		addRule(search_port, Mapper::PROTOCOL_UDP, _("Search"));

		auto minutes = mapper.renewal();
		if(minutes) {
			renewal = GET_TICK() + std::max(minutes, 10u) * 60 * 1000;
		} else {
			TimerManager::getInstance()->removeListener(this);
		}

		return 0;
	}

	// move the preferred mapper to the top of the stack.
	const auto& setting = SETTING(MAPPER);
	for(auto i = mappers.begin(); i != mappers.end(); ++i) {
		if(i->first == setting) {
			if(i != mappers.begin()) {
				auto mapper = *i;
				mappers.erase(i);
				mappers.insert(mappers.begin(), mapper);
			}
			break;
		}
	}

	for(auto i = mappers.begin(); i != mappers.end(); ++i) {
		unique_ptr<Mapper> pMapper(i->second());
		Mapper& mapper = *pMapper;

		ScopedFunctor([&mapper] { mapper.uninit(); });
		if(!mapper.init()) {
			log(str(F_("Failed to initalize the %1% interface") % mapper.getName()));
			continue;
		}

		auto addRule = [this, &mapper](const unsigned short port, Mapper::Protocol protocol, const string& description) -> bool {
			if(port && !mapper.open(port, protocol, str(F_("%1% %2% port (%3% %4%)") %
				APPNAME % description % port % Mapper::protocols[protocol])))
			{
				this->log(str(F_("Failed to map the %1% port (%2% %3%) with the %4% interface") %
					description % port % Mapper::protocols[protocol] % mapper.getName()));
				mapper.close();
				return false;
			}
			return true;
		};

		if(!(addRule(conn_port, Mapper::PROTOCOL_TCP, _("Transfer")) &&
			addRule(secure_port, Mapper::PROTOCOL_TCP, _("Encrypted transfer")) &&
			addRule(search_port, Mapper::PROTOCOL_UDP, _("Search"))))
			continue;

		log(str(F_("Successfully created port mappings (Transfers: %1%, Search: %2%, Encrypted transfers: %3%) on the %4% device with the %5% interface") %
			conn_port % search_port % secure_port % deviceString(mapper) % mapper.getName()));

		working = move(pMapper);

		if(!BOOLSETTING(NO_IP_OVERRIDE)) {
			string externalIP = mapper.getExternalIP();
			if(!externalIP.empty()) {
				SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, externalIP);
			} else {
				// no cleanup because the mappings work and hubs will likely provide the correct IP.
				log(_("Failed to get external IP"));
			}
		}

		ConnectivityManager::getInstance()->mappingFinished(mapper.getName());

		auto minutes = mapper.renewal();
		if(minutes) {
			renewal = GET_TICK() + std::max(minutes, 10u) * 60 * 1000;
			TimerManager::getInstance()->addListener(this);
		}
		break;
	}

	if(!getOpened()) {
		log(_("Failed to create port mappings"));
		ConnectivityManager::getInstance()->mappingFinished(Util::emptyString);
	}

	return 0;
}

void MappingManager::close(Mapper& mapper) {
	if(mapper.hasRules()) {
		bool ret = mapper.init();
		if(ret) {
			ret = mapper.close();
			mapper.uninit();
		}
		log(ret ?
			str(F_("Successfully removed port mappings from the %1% device with the %2% interface") % deviceString(mapper) % mapper.getName()) :
			str(F_("Failed to remove port mappings from the %1% device with the %2% interface") % deviceString(mapper) % mapper.getName()));
	}
}

void MappingManager::log(const string& message) {
	ConnectivityManager::getInstance()->log(str(F_("Port mapping: %1%") % message));
}

string MappingManager::deviceString(Mapper& mapper) const {
	string name(mapper.getDeviceName());
	if(name.empty())
		name = _("Generic");
	return '"' + name + '"';
}

void MappingManager::on(TimerManagerListener::Minute, uint64_t tick) noexcept {
	if(tick >= renewal && !busy.test_and_set())
		start();
}

} // namespace dcpp
