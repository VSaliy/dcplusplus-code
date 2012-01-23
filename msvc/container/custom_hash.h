#ifndef DCPP_BOOST_CONTAINER_CUSTOM_HASH
#define DCPP_BOOST_CONTAINER_CUSTOM_HASH

/* boost & STL have different ways of specifying a custom hash. since DC++ only uses the STL way,
 * this defines a generic boost hash function that redirects to STL ones. */

namespace boost {
	template<typename T>
	inline size_t hash_value(const T& t) {
		return std::hash<T>()(t);
	}
}

#endif

// vim: set filetype=cpp :
