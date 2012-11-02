#include "testbase.h"

#include <dcpp/atomic.h>

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/stack.hpp>

using namespace dcpp;

TEST(testlockfree, test_atomic)
{
	ASSERT_EQ(true, atomic<bool>().is_lock_free());
	ASSERT_EQ(true, atomic<int32_t>().is_lock_free());
	ASSERT_EQ(true, atomic<uint32_t>().is_lock_free());
	ASSERT_EQ(true, atomic<int64_t>().is_lock_free());
	ASSERT_EQ(true, atomic<uint64_t>().is_lock_free());
}

TEST(testlockfree, test_queue)
{
	boost::lockfree::queue<int> container(128);
	ASSERT_EQ(true, container.is_lock_free());
}

TEST(testlockfree, test_spsc_queue)
{
	boost::lockfree::spsc_queue<int> container(128);
	ASSERT_EQ(true, container.is_lock_free());
}

TEST(testlockfree, test_stack)
{
	boost::lockfree::stack<int> container(128);
	ASSERT_EQ(true, container.is_lock_free());
}
