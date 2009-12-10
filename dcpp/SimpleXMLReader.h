
#ifndef DCPLUSPLUS_DCPP_SIMPLEXMLREADER_H_
#define DCPLUSPLUS_DCPP_SIMPLEXMLREADER_H_

namespace dcpp {

class InputStream;

class SimpleXMLReader {
public:
	struct CallBack : private boost::noncopyable {
		virtual ~CallBack() { }
		virtual void startTag(const std::string& name, dcpp::StringPairList& attribs, bool simple) = 0;
		virtual void endTag(const std::string& name, const std::string& data) = 0;

	protected:
		static const std::string& getAttrib(dcpp::StringPairList& attribs, const std::string& name, size_t hint);
	};

	SimpleXMLReader(CallBack* callback);
	virtual ~SimpleXMLReader() { }

	void parse(InputStream& is);
	bool parse(const char* data, size_t len, bool more);
private:
	enum ParseState {
		/// Start of document
		STATE_START,

		/// In <?xml declaration, expect version
		STATE_DECL_VERSION,

		/// In <?xml declaration, expect =
		STATE_DECL_VERSION_EQ,

		/// In <?xml declaration, expect version number
		STATE_DECL_VERSION_NUM,

		/// In <?xml declaration, expect encoding
		STATE_DECL_ENCODING,

		/// In <?xml declaration, expect =
		STATE_DECL_ENCODING_EQ,

		/// In <?xml declaration, expect encoding name
		STATE_DECL_ENCODING_NAME,

		STATE_DECL_ENCODING_NAME_APOS,

		STATE_DECL_ENCODING_NAME_QUOT,

		/// in <?xml declaration, expect standalone
		STATE_DECL_STANDALONE,

		/// in <?xml declaration, expect =
		STATE_DECL_STANDALONE_EQ,

		/// in <?xml declaration, expect standalone yes
		STATE_DECL_STANDALONE_YES,

		/// In <?xml declaration, expect %>
		STATE_DECL_END,

		/// In < element, expect element name
		STATE_ELEMENT_NAME,

		/// In < element, expect attribute or element end
		STATE_ELEMENT_ATTR,

		/// In < element, in attribute name
		STATE_ELEMENT_ATTR_NAME,

		/// In < element, expect %
		STATE_ELEMENT_ATTR_EQ,

		/// In < element, waiting for attribute value start
		STATE_ELEMENT_ATTR_VALUE,

		STATE_ELEMENT_ATTR_VALUE_QUOT,

		STATE_ELEMENT_ATTR_VALUE_APOS,

		STATE_ELEMENT_END_SIMPLE,

		STATE_ELEMENT_END,

		STATE_ELEMENT_END_END,

		/// In <!-- comment field
		STATE_COMMENT,

		STATE_CONTENT,

		STATE_END
	};


	std::string buf;
	std::string::size_type bufPos;

	dcpp::StringPairList attribs;
	std::string value;

	CallBack* cb;
	std::string encoding;

	ParseState state;

	dcpp::StringList elements;

	bool needChars(size_t n) const;
	int charAt(size_t n) const;
	bool skipSpace();
	void advancePos(size_t n = 1);
	std::string::size_type bufSize() const;

	bool literal(const char* lit, size_t len, bool withSpace, ParseState newState);
	bool character(int c, ParseState newState);

	bool declVersionNum();
	bool declEncodingValue();

	bool element();
	bool elementName();
	bool elementEnd();
	bool elementEndEnd();
	bool elementEndSimple();
	bool elementEndComplex();
	bool elementAttr();
	bool elementAttrName();
	bool elementAttrValue();

	bool comment();

	bool content();

	bool entref(std::string& d);

	bool process();
	bool spaceOrError(const char* error);
};


}
#endif /* DCPP_SIMPLEXMLREADER_H_ */
