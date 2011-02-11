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

#ifndef DCPLUSPLUS_DCPP_MAPPING_MANAGER_H
#define DCPLUSPLUS_DCPP_MAPPING_MANAGER_H

#include "forward.h"
#include "Mapper.h"
#include "TimerManager.h"

#include <atomic>

#include <boost/ptr_container/ptr_vector.hpp>

namespace dcpp {

class MappingManager :
	public Singleton<MappingManager>,
	private Thread,
	private TimerManagerListener
{
public:
	/**
	* add an implementation, derived from the base Mapper class.
	* must be allocated on the heap; its deletion will be managed by MappingManager.
	* the first added mapper will be tried first.
	*/
	void addImplementation(Mapper* mapper);
	bool open();
	void close();

	bool getOpened() const { return opened; }

private:
	friend class Singleton<MappingManager>;

	boost::ptr_vector<Mapper> mappers;

	bool opened;
	atomic_flag busy;

	uint64_t renewal; /// when the next renewal should happen, if requested by the mapper.
	size_t working; /// index of the currently working implementation (used for renewal).

	MappingManager() : opened(false), busy(false), renewal(0), working(0) { }
	virtual ~MappingManager() throw() { join(); }

	int run();

	void close(Mapper& mapper);
	void log(const string& message);
	string deviceString(Mapper& mapper) const;

	void on(TimerManagerListener::Minute, uint64_t tick) throw();
};

} // namespace dcpp

#endif
