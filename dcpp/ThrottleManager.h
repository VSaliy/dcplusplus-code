/* 
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#include "DownloadManager.h"
#include "Singleton.h"
#include "Socket.h"
#include "Thread.h"
#include "TimerManager.h"
#include "UploadManager.h"
#include "ClientManager.h"

namespace dcpp
{

	#define POLL_TIMEOUT 250
	
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
		int read(Socket* sock, void* buffer, size_t len)
		{
			size_t downs = DownloadManager::getInstance()->getDownloadCount();
			if(!BOOLSETTING(THROTTLE_ENABLE) || downTokens == -1 || downs == 0)
				return sock->read(buffer, len);
			
			{
				Lock l(downCS);
				
				if(downTokens > 0)
				{
					size_t slice = (SETTING(MAX_DOWNLOAD_SPEED_CURRENT) * 1024) / downs;
					size_t readSize = min(slice, min(len, static_cast<size_t>(downTokens)));
					
					// read from socket
					readSize = sock->read(buffer, readSize);
					
					if(readSize > 0)
						downTokens -= readSize;
								
					return readSize;
				}
			}
			
			// no tokens, wait for them
			WaitForSingleObject(hEvent, POLL_TIMEOUT);
			return -1;	// from BufferedSocket: -1 = retry, 0 = connection close
		}
		
		/*
		 * Limits a traffic and writes a packet to the network
		 * We must handle this a little bit differently than downloads, because of that stupidity in OpenSSL
		 */		
		int write(Socket* sock, void* buffer, size_t& len)
		{
			size_t ups = UploadManager::getInstance()->getUploadCount();
			if(!BOOLSETTING(THROTTLE_ENABLE) || upTokens == -1 || ups == 0)
				return sock->write(buffer, len);
			
			{
				Lock l(upCS);
				
				if(upTokens > 0)
				{
					size_t slice = (SETTING(MAX_UPLOAD_SPEED_CURRENT) * 1024) / ups;
					len = min(slice, min(len, static_cast<size_t>(upTokens)));
					upTokens -= len;
					
					// write to socket			
					return sock->write(buffer, len);
				}
			}
			
			// no tokens, wait for them
			WaitForSingleObject(hEvent, POLL_TIMEOUT);
			return 0;	// from BufferedSocket: -1 = failed, 0 = retry
		}
		
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
		void on(TimerManagerListener::Second, uint32_t aTick) throw()
		{
			if(!BOOLSETTING(THROTTLE_ENABLE))
				return;

			int downLimit    = SETTING(MAX_DOWNLOAD_SPEED_MAIN);
			int upLimit      = SETTING(MAX_UPLOAD_SPEED_MAIN);
			int currentSlots = SETTING(SLOTS);
			int newSlots     = SETTING(SLOTS_PRIMARY);

			// alternative limiter
			time_t currentTime;
			time(&currentTime);
			int currentHour = localtime(&currentTime)->tm_hour;
			if (SETTING(TIME_DEPENDENT_THROTTLE) &&
				((SETTING(BANDWIDTH_LIMIT_START) < SETTING(BANDWIDTH_LIMIT_END) &&
					currentHour >= SETTING(BANDWIDTH_LIMIT_START) && currentHour < SETTING(BANDWIDTH_LIMIT_END)) ||
				(SETTING(BANDWIDTH_LIMIT_START) > SETTING(BANDWIDTH_LIMIT_END) &&
					(currentHour >= SETTING(BANDWIDTH_LIMIT_START) || currentHour < SETTING(BANDWIDTH_LIMIT_END)))))
			{
				downLimit = SETTING(MAX_UPLOAD_SPEED_ALTERNATE);
				upLimit   = SETTING(MAX_DOWNLOAD_SPEED_ALTERNATE);
				newSlots  = SETTING(SLOTS_ALTERNATE_LIMITING);
			} 

			// readd tokens
			{
				Lock l(downCS);
				if(downLimit > 0)
					downTokens = downLimit * 1024;
				else
					downTokens = -1;
			}

			{
				Lock l(upCS);
				if(upLimit > 0)
					upTokens = upLimit * 1024;
				else
					upTokens = -1;
			}

			SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_CURRENT, downLimit);
			SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_CURRENT, upLimit);
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, newSlots);
			if(newSlots != currentSlots)
				ClientManager::getInstance()->infoUpdated();
			
			PulseEvent(hEvent);
		}
				
	};

}	// namespace dcpp
#endif	// _THROTTLEMANAGER_H