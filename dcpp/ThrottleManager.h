/* 
 * Copyright (C) 2009-2010 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef _THROTTLEMANAGER_H
#define _THROTTLEMANAGER_H

#include "Singleton.h"
#include "Socket.h"
#include "Thread.h"
#include "TimerManager.h"
#include "SettingsManager.h"

namespace dcpp
{
	/**
	 * Manager for limiting traffic flow.
	 * Inspired by Token Bucket algorithm: http://en.wikipedia.org/wiki/Token_bucket
	 */
	class ThrottleManager :
		public Singleton<ThrottleManager>, private TimerManagerListener
	{
	public:

		/*
		 * Limits a traffic and reads a packet from the network
		 */
		int read(Socket* sock, void* buffer, size_t len);

		/*
		 * Limits a traffic and writes a packet to the network
		 * We must handle this a little bit differently than downloads, because of that stupidity in OpenSSL
		 */
		int write(Socket* sock, void* buffer, size_t& len);

		SettingsManager::IntSetting getCurSetting(SettingsManager::IntSetting setting);
	private:

		// download limiter
		CriticalSection	downCS;
		int64_t			downTokens;

		// upload limiter
		CriticalSection	upCS;
		int64_t			upTokens;

		HANDLE			hEvent;

		friend class Singleton<ThrottleManager>;

		// constructor
		ThrottleManager(void) : downTokens(0), upTokens(0)
		{
			hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			TimerManager::getInstance()->addListener(this);
		}

		// destructor
		~ThrottleManager(void)
		{
			TimerManager::getInstance()->removeListener(this);

			CloseHandle(hEvent);
		}

		// TimerManagerListener
		void on(TimerManagerListener::Second, uint32_t aTick) throw();
	};

}	// namespace dcpp
#endif	// _THROTTLEMANAGER_H