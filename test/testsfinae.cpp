// Make sure some of the clever SFINAE used by DC++ is working as expected on the current compiler.

#include "testbase.h"

#include <dcpp/Util.h>

using namespace dcpp;

struct Funcs {
	void f1() { }
	void f2(int) { }
	int f3() { return 0; }
	void f4(const string&) { }
};

template<typename T>
struct X {
	// these should be true.
	HAS_FUNC(F1, void, f1());
	HAS_FUNC(F2, void, f2(0));
	HAS_FUNC(F3, int, f3());
	HAS_FUNC(F4, void, f4(string()));

	// these should be false.
	HAS_FUNC(FN1, void, lol());
	HAS_FUNC(FN2, bool, f1());
	HAS_FUNC(FN3, bool, f3());

	bool test_F1() { return F1<T>::value; }
	bool test_F2() { return F2<T>::value; }
	bool test_F3() { return F3<T>::value; }
	bool test_F4() { return F4<T>::value; }

	bool test_FN1() { return FN1<T>::value; }
	bool test_FN2() { return FN2<T>::value; }
	bool test_FN3() { return FN3<T>::value; }

	bool i() {
		return i1();
	}

	template<typename U = T> typename std::enable_if<F1<U>::value, bool>::type i1() { return true; }
	template<typename U = T> typename std::enable_if<!F1<U>::value, bool>::type i1() { return false; }
};

TEST(testsfinae, test_has_func)
{
	X<Funcs> x;

	ASSERT_EQ(true, x.test_F1());
	ASSERT_EQ(true, x.test_F2());
	ASSERT_EQ(true, x.test_F3());
	ASSERT_EQ(true, x.test_F4());

	ASSERT_EQ(false, x.test_FN1());
#ifndef _MSC_VER
	/* TODO these tests ensure that the return value of the tested function is correctly validated.
	doesn't work in MSVC, as explained by the definition of HAS_FUNC. */
	ASSERT_EQ(false, x.test_FN2());
	ASSERT_EQ(false, x.test_FN3());
#endif

	ASSERT_EQ(true, x.i());
}
