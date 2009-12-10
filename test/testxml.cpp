#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>

#include <dcpp/SimpleXMLReader.h>
#include <unordered_map>

#define BOOST_TEST_MODULE testxml
#include <boost/test/included/unit_test.hpp>

using namespace dcpp;

typedef std::tr1::unordered_map<std::string, int> Counter;

class Collector : public SimpleXMLReader::CallBack {
public:

	virtual void startTag(const std::string& name, dcpp::StringPairList& attribs, bool simple) {
		if(simple) {
			simpleTags[name]++;
		} else {
			startTags[name]++;
		}

		for(dcpp::StringPairIter i = attribs.begin(), iend = attribs.end(); i != iend; ++i) {
			attribKeys[i->first]++;
			attribValues[i->second]++;
		}
	}

	virtual void endTag(const std::string& name, const std::string& data) {
		endTags[name]++;
		dataValues[data]++;
	}

	Counter simpleTags;
	Counter startTags;
	Counter endTags;
	Counter attribKeys;
	Counter attribValues;
	Counter dataValues;
};

BOOST_AUTO_TEST_CASE( test_simple )
{
	Collector collector;
    SimpleXMLReader reader(&collector);

    const char xml[] = "<?xml version='1.0' encoding='utf-8' ?><complex a='1'><simple b='2'/><complex2>data</complex2></complex>";
    for(size_t i = 0, iend = sizeof(xml); i < iend; ++i) {
    	reader.parse(xml + i, 1, true);
    }

    BOOST_CHECK_EQUAL(collector.simpleTags["simple"], 1);
    BOOST_CHECK_EQUAL(collector.startTags["complex"], 1);
    BOOST_CHECK_EQUAL(collector.endTags["complex"], 1);
    BOOST_CHECK_EQUAL(collector.attribKeys["a"], 1);
    BOOST_CHECK_EQUAL(collector.attribValues["1"], 1);
    BOOST_CHECK_EQUAL(collector.attribKeys["b"], 1);
    BOOST_CHECK_EQUAL(collector.attribValues["2"], 1);
    BOOST_CHECK_EQUAL(collector.startTags["complex2"], 1);
    BOOST_CHECK_EQUAL(collector.endTags["complex2"], 1);
    BOOST_CHECK_EQUAL(collector.dataValues["data"], 1);
}

