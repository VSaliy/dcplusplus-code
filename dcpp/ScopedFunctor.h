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

#ifndef DCPLUSPLUS_DCPP_SCOPED_FUNCTOR_H
#define DCPLUSPLUS_DCPP_SCOPED_FUNCTOR_H

namespace dcpp {

/* helper class to run a functor when the object goes out of scope.
the additional #defines facilitate the following:
- compilers can't always deduce the type of some functors (especially lambdas), so we use decltype.
- use __COUNTER__ to generate a unique object name.
for example, the call:
	ScopedFunctor([] { printf("hello"); })
will print "hello" when the current scope ends. */
template<typename F>
struct ScopedFunctor {
	explicit ScopedFunctor(const F& f_) : f(f_) { }
	~ScopedFunctor() { f(); }
private:
	const F& f;
};
#define ScopedFunctor_gen(ScopedFunctor_name, ScopedFunctor_counter) ScopedFunctor_name##ScopedFunctor_counter
#define ScopedFunctor_(ScopedFunctor_f, ScopedFunctor_counter) \
	auto ScopedFunctor_gen(ScopedFunctor_f_, ScopedFunctor_counter) = ScopedFunctor_f; \
	ScopedFunctor<decltype(ScopedFunctor_gen(ScopedFunctor_f_, ScopedFunctor_counter))> \
	ScopedFunctor_gen(ScopedFunctor_object, ScopedFunctor_counter) \
	(ScopedFunctor_gen(ScopedFunctor_f_, ScopedFunctor_counter))
#define ScopedFunctor(ScopedFunctor_f) ScopedFunctor_(ScopedFunctor_f, __COUNTER__)

} // namespace dcpp

#endif
