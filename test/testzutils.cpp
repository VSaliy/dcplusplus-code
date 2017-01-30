/* Tests around the wrappers the dcpp lib has around zlib. These tests are as close as possible to
 * the way the dcpp lib uses these wrappers to compress / decompress files when uploading /
 * downloading them.
 *
 * We also run tests from the zlib project itself (they are in the "zlib/test" directory). */

#include "testbase.h"

#include <dcpp/File.h>
#include <dcpp/FilteredFile.h>
#include <dcpp/MD5Hash.h>
#include <dcpp/ZUtils.h>

using namespace dcpp;

auto md5(auto path) {
	File f(path, File::READ, File::OPEN);
	MD5Hash h;
	auto buf = f.read();
	h.update(buf.c_str(), buf.size());
	return MD5Value(h.finalize());
}

auto runTransfer(auto& inStream, auto& outStream) {
	try {
		while(true) {
			size_t bufSize = 64 * 1024;
			ByteVector buf(bufSize);
			auto newBufSize = inStream.read(&buf[0], bufSize);
			if(newBufSize == 0) {
				break;
			}
			outStream.write(&buf[0], newBufSize);
		}
	} catch(const Exception& e) {
		FAIL() << e.getError();
	}
}

TEST(testzutils, test_compression)
{
	// Test a compression using dcpp::ZFilter.

	{
		File f_in("test/gtest.h", File::READ, File::OPEN);
		File f_out("test/data/out/gtest_h_compressed", File::WRITE, File::CREATE | File::TRUNCATE);
		FilteredInputStream<ZFilter, false> stream_in(&f_in);

		runTransfer(stream_in, f_out);
	}

	ASSERT_EQ(md5("test/data/gtest_h_compressed"), md5("test/data/out/gtest_h_compressed"));
}

TEST(testzutils, test_decompression)
{
	// Test a decompression using dcpp::UnZFilter.

	{
		File f_in("test/data/gtest_h_compressed", File::READ, File::OPEN);
		File f_out("test/data/out/gtest_h_decompressed", File::WRITE, File::CREATE | File::TRUNCATE);
		FilteredOutputStream<UnZFilter, false> stream_out(&f_out);

		runTransfer(f_in, stream_out);
	}

	ASSERT_EQ(md5("test/gtest.h"), md5("test/data/out/gtest_h_decompressed"));
}
