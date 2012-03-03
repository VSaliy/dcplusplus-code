#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>

#include <dcpp/SimpleXMLReader.h>
#include <unordered_map>

#include "gtest.h"

#define _(x)

using namespace dcpp;

typedef std::unordered_map<std::string, int> Counter;

class Collector : public SimpleXMLReader::CallBack {
public:

	void startTag(const std::string& name, dcpp::StringPairList& attribs, bool simple) {
		if(simple) {
			simpleTags[name]++;
		} else {
			startTags[name]++;
		}

		for(auto& i: attribs) {
			attribKeys[i.first]++;
			attribValues[i.second]++;
		}
	}

	void data(const std::string& data) {
		dataValues[data]++;
	}

	void endTag(const std::string& name) {
		endTags[name]++;
	}

	Counter simpleTags;
	Counter startTags;
	Counter endTags;
	Counter attribKeys;
	Counter attribValues;
	Counter dataValues;
};

TEST(testxml, test_simple)
{
	Collector collector;
    SimpleXMLReader reader(&collector);

    const char xml[] = "<?xml version='1.0' encoding='utf-8' ?><complex a='1'> data <simple b=\"2\"/><complex2> data </complex2></complex>";
    for(size_t i = 0, iend = sizeof(xml); i < iend; ++i) {
    	reader.parse(xml + i, 1);
    }

    ASSERT_EQ(collector.simpleTags["simple"], 1);
    ASSERT_EQ(collector.startTags["complex"], 1);
    ASSERT_EQ(collector.endTags["complex"], 1);
    ASSERT_EQ(collector.attribKeys["a"], 1);
    ASSERT_EQ(collector.attribValues["1"], 1);
    ASSERT_EQ(collector.attribKeys["b"], 1);
    ASSERT_EQ(collector.attribValues["2"], 1);
    ASSERT_EQ(collector.startTags["complex2"], 1);
    ASSERT_EQ(collector.endTags["complex2"], 1);
    ASSERT_EQ(collector.dataValues[" data "], 2);
}

TEST(testxml, test_entref)
{
	Collector collector;
	SimpleXMLReader reader(&collector);

    const char xml[] = "<root ab='&apos;&amp;&quot;'>&lt;&gt;</root>";
    for(size_t i = 0, iend = sizeof(xml); i < iend; ++i) {
    	reader.parse(xml + i, 1);
    }

    ASSERT_EQ(collector.startTags["root"], 1);
    ASSERT_EQ(collector.endTags["root"], 1);
    ASSERT_EQ(collector.attribKeys["ab"], 1);
    ASSERT_EQ(collector.attribValues["'&\""], 1);
    ASSERT_EQ(collector.dataValues["<>"], 1);
}

TEST(testxml, test_comment)
{
	Collector collector;
	SimpleXMLReader reader(&collector);

    const char xml[] = "<root><!-- comment <i>,;&--></root>";
    for(size_t i = 0, iend = sizeof(xml); i < iend; ++i) {
    	reader.parse(xml + i, 1);
    }

    ASSERT_EQ(collector.startTags["root"], 1);
    ASSERT_EQ(collector.endTags["root"], 1);
}

#include <dcpp/File.h>

TEST(testxml, test_file)
{
	if(File::getSize("test.xml") != -1) {
		Collector collector;
		SimpleXMLReader reader(&collector);
		File f("test.xml", File::READ, File::OPEN);
		reader.parse(f);
	} else {
		SUCCEED() << "test.xml not found";
	}
}

