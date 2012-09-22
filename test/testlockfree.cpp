#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/stack.hpp>

#include "gtest.h"

using namespace dcpp;

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
