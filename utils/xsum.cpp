#include <iostream>

#include <dcpp/stdinc.h>
#include <dcpp/FileReader.h>
#include <dcpp/Util.h>

#include <boost/date_time/posix_time/ptime.hpp>
using namespace boost::posix_time;

using namespace std;
using namespace dcpp;

int main(int argc, char** argv)
{
	if(argc != 2) {
		cout << "You need to supply a file name" << endl;
		return 1;
	}

	char x[_MAX_PATH] = { 0 };
	char * tmp;
	if(!(tmp = _fullpath(x, argv[1], _MAX_PATH))) {
		cout << "Can't get full path" << endl;
		return 1;
	}
	try {
		auto start = microsec_clock::universal_time();
		FileReader fr(true, 0);

		size_t total = 0;
		fr.read(tmp, [&](void* x, size_t n) {
			total += n;
			if(total % (1024*1024) == 0) {
				std::cout << ".";
			}
		});
		auto diff = (microsec_clock::universal_time() - start).total_microseconds();
		auto s = diff / 1000000.0;
		if(s == 0) s = 1;
		cout << endl << Util::formatBytes(total) << ", " << s << " s, " << Util::formatBytes(total / s) << " b/s" << endl;
	} catch(const std::exception& e) {
		cout << "Error: " << e.what() << endl;
	}

	return 0;
}



