/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(QUEUE_MANAGER_LISTENER_H)
#define QUEUE_MANAGER_LISTENER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class QueueItem;

class QueueManagerListener {
public:
	virtual ~QueueManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Added;
	typedef X<1> Finished;
	typedef X<2> Removed;
	typedef X<3> Moved;
	typedef X<4> SourcesUpdated;
	typedef X<5> StatusUpdated;
	typedef X<6> SearchStringUpdated;
	typedef X<7> PartialList;

	virtual void on(Added, QueueItem*) throw() { }
	virtual void on(Finished, QueueItem*, int64_t) throw() { }
	virtual void on(Removed, QueueItem*) throw() { }
	virtual void on(Moved, QueueItem*) throw() { }
	virtual void on(SourcesUpdated, QueueItem*) throw() { }
	virtual void on(StatusUpdated, QueueItem*) throw() { }
	virtual void on(SearchStringUpdated, QueueItem*) throw() { }
	virtual void on(PartialList, const User::Ptr&, const string&) throw() { }
};

#endif // !defined(QUEUE_MANAGER_LISTENER_H)
