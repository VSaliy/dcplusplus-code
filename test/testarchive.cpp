#include "testbase.h"

#include <dcpp/Archive.h>
#include <dcpp/File.h>
#include <dcpp/MD5Hash.h>

using namespace dcpp;

TEST(testarchive, test_archive)
{
	try {
		Archive("test/data/gtest_h.tar.gz").extract("test/data/out/");
	}
	catch(const Exception& e) {
		FAIL() << e.getError();
	}

	auto md5 = [](string path) {
		File f(path, File::READ, File::OPEN);
		MD5Hash h;
		h.update(f.read().c_str(), f.getSize());
		return MD5Value(h.finalize());
	};

	ASSERT_EQ(md5("test/gtest.h"), md5("test/data/out/gtest.h"));
}
