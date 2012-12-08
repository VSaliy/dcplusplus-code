// Tool to hash files using the interfaces provided by DC++ and gather some statistics.

#include "base.h"

#include <ctime>
#include <iostream>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

#include <dcpp/File.h>
#include <dcpp/FileReader.h>
#include <dcpp/MerkleTree.h>
#include <dcpp/TigerHash.h>

using namespace std;
using namespace dcpp;
using namespace boost::accumulators;

void help() {
	cout << "Arguments to run hash with:" << endl << "\t hash <path> [count]" << endl
		<< "<path> is the file to hash." << endl
		<< "[count] (optional) is the amount of times the file should be hashed to establish statistics." << endl;
}

enum { Path = 1, LastCompulsory = Path, Count };

struct Info {
	string root;
	clock_t time;
	double speed;
};

Info run(const string& path) {
	// adapted from dcpp::HashManager

	File f { path, File::READ, File::OPEN };
	auto size = f.getSize();
	f.close();

	static const int64_t MIN_BLOCK_SIZE = 64 * 1024;
	auto bs = std::max(TigerTree::calcBlockSize(size, 10), MIN_BLOCK_SIZE);

	TigerTree tt { bs };

	auto start = clock();

	FileReader(true).read(path, [&](const void* buf, size_t n) -> bool {
		tt.update(buf, n);
		return true;
	});

	tt.finalize();

	auto end = clock();

	double speed = 0.0;
	if(end > start) {
		speed = static_cast<double>(size) * 1000.0 / static_cast<double>(end - start);
	}

	Info ret { tt.getRoot().toBase32(), end - start, speed };
	return ret;
}

int main(int argc, char* argv[]) {
	if(argc <= LastCompulsory) {
		help();
		return 1;
	}

	string path { argv[Path] };

	try {
		cout << "Hashing <" << path << ">..." << endl;
		auto info = run(path);
		cout << "Hashed <" << path << ">:" << endl
			<< "\tTTH: " << info.root << endl
			<< "\tTime: " << info.time << " ms" << endl
			<< "\tSpeed: " << info.speed << " bytes/s" << endl;
	} catch(const FileException& e) {
		cout << "Error reading <" << path << ">: " << e.getError() << endl;
		help();
		return 2;
	}

	if(argc > Count) {
		auto count = Util::toUInt(argv[Count]);
		if(count < 1) {
			cout << "Error: invalid count (" << count << ")." << endl;
			help();
			return 1;
		}

		cout << "Hashing again " << count << " more times..." << endl;

		accumulator_set<double, stats<tag::variance>> time;
		accumulator_set<double, stats<tag::variance>> speed;

		for(decltype(count) i = 0; i < count; ++i) {
			try {
				auto info = run(path);
				time(info.time);
				speed(info.speed);
			} catch(const FileException&) { }
		}

		if(boost::accumulators::count(time) != count || boost::accumulators::count(speed) != count) {
			cout << "Error: couldn't gather statistics for " << count << " runs." << endl;
			return 3;
		}

		cout << "Statistics on " << count << " runs:" << endl
			<< "\tTime: mean = " << mean(time) << " s, std dev = " << sqrt(variance(time)) << " s" << endl
			<< "\tSpeed: mean = " << mean(speed) << " bytes/s, std dev = " << sqrt(variance(speed)) << " bytes/s" << endl;
	}

	return 0;
}
