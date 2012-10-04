/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_TASK_H
#define DCPLUSPLUS_DCPP_TASK_H

#include <memory>
#include <vector>

#include <boost/noncopyable.hpp>

#include "CriticalSection.h"

namespace dcpp {

using std::pair;
using std::unique_ptr;
using std::vector;

struct Task {
	virtual ~Task() { };
};

struct StringTask : Task {
	StringTask(string str) : str(move(str)) { }
	string str;
};

template<bool threadsafe>
class TaskQueue : private boost::noncopyable {
protected:
	typedef vector<pair<int, unique_ptr<Task>>> List;

public:
	virtual ~TaskQueue() {
		clear();
	}

	void add(int type, std::unique_ptr<Task> && data) { tasks.emplace_back(type, move(data)); }
	List get() { return move(tasks); }
	void clear() { tasks.clear(); }

private:
	List tasks;
};

template<>
class TaskQueue<true> : public TaskQueue<false> {
	typedef TaskQueue<false> BaseType;

public:
	void add(int type, std::unique_ptr<Task> && data) { Lock l(cs); BaseType::add(type, move(data)); }
	List get() { Lock l(cs); return BaseType::get(); }
	void clear() { Lock l(cs); BaseType::clear(); }

private:
	CriticalSection cs;
};

} // namespace dcpp

#endif
