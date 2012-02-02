/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// inspired by Bjarke Viksoe's Simple HTML Viewer <http://www.viksoe.dk/code/simplehtmlviewer.htm>.

#include "stdafx.h"
#include "HtmlToRtf.h"

#include <dcpp/debug.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/StringTokenizer.h>
#include <dcpp/Text.h>

#include <dwt/util/GDI.h>
#include <dwt/widgets/RichTextBox.h>

struct Parser : SimpleXMLReader::CallBack {
	Parser(dwt::RichTextBox* box);
	void startTag(const string& name, StringPairList& attribs, bool simple);
	void endTag(const string& name, const string& data);
	tstring finalize();

private:
	int addFont(string&& font);
	static int rtfFontSize(float px);
	int addColor(COLORREF color);

	void parseFont(const string& s);
	void parseColor(int& contextColor, const string& s);

	tstring ret;

	StringList fonts;
	StringList colors;

	struct {
		int font;
		int fontSize;
		int textColor;
		int bgColor;
		void clear() { font = fontSize = textColor = bgColor = -1; }
	} context, defaultContext;

	string header;
};

tstring HtmlToRtf::convert(const string& html, dwt::RichTextBox* box) {
	Parser parser(box);
	try { SimpleXMLReader(&parser).parse(html); }
	catch(const SimpleXMLException& e) { return Text::toT(e.getError()); }
	return parser.finalize();
}

Parser::Parser(dwt::RichTextBox* box) {
	auto lf = box->getFont()->getLogFont();
	defaultContext.font = addFont("{\\f0\\fnil\\fcharset" + Util::toString(lf.lfCharSet) + " " + Text::fromT(lf.lfFaceName) + ";}");
	defaultContext.fontSize = rtfFontSize(abs(lf.lfHeight));

	defaultContext.textColor = addColor(box->getTextColor());
	defaultContext.bgColor = addColor(box->getBgColor());

	context.clear();
}

int Parser::addFont(string&& font) {
	auto ret = fonts.size();
	fonts.push_back(std::forward<string>(font));
	return ret;
}

int Parser::rtfFontSize(float px) {
	return px * 72.0 / 96.0 // px -> font points
		* dwt::util::dpiFactor() // respect DPI settings
		* 2.0; // RTF font sizes are expressed in half-points
}

int Parser::addColor(COLORREF color) {
	auto ret = colors.size();
	colors.push_back("\\red" + Util::toString(GetRValue(color)) +
		"\\green" + Util::toString(GetGValue(color)) +
		"\\blue" + Util::toString(GetBValue(color)) + ";");
	return ret;
}

static const string styleAttr = "style";
static const string fontStyle = "font";
static const string textColorStyle = "color";
static const string bgColorStyle = "background-color";
static string tmp;

void Parser::startTag(const string& name, StringPairList& attribs, bool simple) {
	if(attribs.empty()) {
		return;
	}

	const auto& style = getAttrib(attribs, styleAttr, 0);

	enum { Declaration, Font, TextColor, BgColor } state = Declaration;

	size_t i = 0, j;
	while((j = style.find_first_of(":;", i)) != string::npos) {
		tmp = style.substr(i, j - i);
		i = j + 1;

		switch(state) {
		case Declaration:
			{
				if(tmp == fontStyle) { state = Font; }
				else if(tmp == textColorStyle) { state = TextColor; }
				else if(tmp == bgColorStyle) { state = BgColor; }
				break;
			}

		case Font:
			{
				parseFont(tmp);
				state = Declaration;
				break;
			}

		case TextColor:
			{
				parseColor(context.textColor, tmp);
				state = Declaration;
				break;
			}

		case BgColor:
			{
				parseColor(context.bgColor, tmp);
				state = Declaration;
				break;
			}
		}
	}
}

void Parser::endTag(const string& name, const string& data) {
	if(context.font == -1) { context.font = defaultContext.font; }
	if(context.fontSize == -1) { context.fontSize = defaultContext.fontSize; }
	if(context.textColor == -1) { context.textColor = defaultContext.textColor; }
	if(context.bgColor == -1) { context.bgColor = defaultContext.bgColor; }

	ret += Text::toT("{\\f" + Util::toString(context.font) + "\\fs" + Util::toString(context.fontSize) +
		"\\cf" + Util::toString(context.textColor) + "\\highlight" + Util::toString(context.bgColor) +
		header + " ") + dwt::RichTextBox::rtfEscape(Text::toT(Text::toDOS(data))) + _T("}");

	context.clear();
	header.clear();
}

tstring Parser::finalize() {
	return Text::toT("{{\\fonttbl" + Util::toString(Util::emptyString, fonts) +
		"}{\\colortbl" + Util::toString(Util::emptyString, colors) + "}") + ret + _T("}");
}

void Parser::parseFont(const string& s) {
	// this contains multiple params separated by spaces.
	StringTokenizer<string> tok(s, ' ');
	auto& l = tok.getTokens();

	// remove empty strings.
	l.erase(std::remove_if(l.begin(), l.end(), [](const string& s) { return s.empty(); }), l.end());

	auto params = l.size();
	if(params < 2) // the last 2 params (font size & font family) are compulsory.
		return;

	// the last param (font family) may contain spaces; merge if that is the case.
	while(*(l.back().end() - 1) == '\'' && (l.back().size() <= 1 || *l.back().begin() != '\'')) {
		*(l.end() - 2) += ' ' + std::move(l.back());
		l.erase(l.end() - 1);
		if(l.size() < 2)
			return;
	}

	// parse the last param (font family).
	/// @todo handle multiple fonts separated by commas...
	auto& family = l.back();
	family.erase(std::remove(family.begin(), family.end(), '\''), family.end());
	if(family.empty())
		return;
	context.font = addFont("{\\f" + Util::toString(context.font) + "\\fnil " + std::move(family) + ";}");

	// parse the second to last param (font size).
	/// @todo handle more than px sizes
	auto& size = *(l.end() - 2);
	if(size.size() > 2 && *(size.end() - 2) == 'p' && *(size.end() - 1) == 'x') { // 16px
		context.fontSize = rtfFontSize(Util::toFloat(size.substr(0, size.size() - 2)));
	}

	// parse the optional third to last param (font weight).
	if(params >= 3 && Util::toInt(*(l.end() - 3)) >= FW_BOLD) {
		header += "\\b";
	}

	// parse the optional first param (font style).
	if(params >= 4 && l[0] == "italic") {
		header += "\\i";
	}
}

void Parser::parseColor(int& contextColor, const string& s) {
	auto sharp = s.find('#');
	if(sharp != string::npos && s.size() > sharp + 6) {
		try {
			size_t pos = 0;
			auto color = std::stol(s.substr(sharp + 1, 6), &pos, 16);
			contextColor = addColor(RGB((color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF));
		} catch(const std::exception& e) { dcdebug("color parsing exception: %s with str: %s\n", e.what(), s.c_str()); }
	}
}
