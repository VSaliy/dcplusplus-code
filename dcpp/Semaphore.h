/*
 * Copyright (C) 2001-2015 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_SEMAPHORE_H
#define DCPLUSPLUS_DCPP_SEMAPHORE_H

#include <boost/noncopyable.hpp>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace dcpp {

class Semaphore : boost::noncopyable
{
public:
	Semaphore() noexcept : semaphore(0) {
	}

	void signal() noexcept {
		semaphore.post();
	}

	bool wait() noexcept {
		semaphore.wait();
		return true;
	}

	bool wait(uint32_t millis) noexcept {
		return semaphore.timed_wait(
			boost::posix_time::microsec_clock::universal_time() + boost::posix_time::millisec(millis));
	}

private:
	boost::interprocess::interprocess_semaphore semaphore;
};

} // namespace dcpp

#endif // DCPLUSPLUS_DCPP_SEMAPHORE_H
