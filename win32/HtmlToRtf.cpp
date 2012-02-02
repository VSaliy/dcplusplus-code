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
#include <dcpp/Text.h>

#include <dwt/widgets/RichTextBox.h>

#ifdef TODO
HubFrame::FormattedChatMessage HubFrame::format(const ChatMessage& message, int* pmInfo) const {
	if(!nick.empty()) {
		nick = chat->rtfEscape(nick);
		tstring rtfHeader;
		tstring rtfFormat;
		if(!style.font.empty()) {
			auto cached = WinUtil::getUserMatchFont(style.font);
			if(cached.get()) {
				auto lf = cached->getLogFont();
				rtfHeader += _T("{\\fonttbl{\\f0\\fnil\\fcharset") + Text::toT(Util::toString(lf.lfCharSet)) + _T(" ") + lf.lfFaceName + _T(";}}");
				rtfFormat += _T("\\f0\\fs") + Text::toT(Util::toString(lf.lfHeight * 2));
				if(lf.lfWeight >= FW_BOLD) { rtfFormat += _T("\\b"); }
				if(lf.lfItalic) { rtfFormat += _T("\\i"); }
			}
		}
		if(!rtfFormat.empty() || style.textColor != -1 || style.bgColor != -1) {
			/* when creating a new context (say for a font table), always redefine colors as the Rich Edit
			control seems to randomly reset them like a boss. */
			if(style.textColor == -1) { style.textColor = chat->getTextColor(); }
			if(style.bgColor == -1) { style.bgColor = chat->getBgColor(); }
			rtfHeader += _T("{\\colortbl") + toRTF(style.textColor) + toRTF(style.bgColor) + _T("}");
			rtfFormat += _T("\\cf0\\highlight1");
		}
		if(!rtfFormat.empty()) {
			nick = _T("{") + rtfHeader + rtfFormat + _T(" ") + nick + _T("}");
		}
		ret.second += message.thirdPerson ? _T("* ") + nick + _T(" ") : _T("<") + nick + _T("> ");
	}
}
#endif

struct Parser : SimpleXMLReader::CallBack {
	Parser() : colorIndex(0) { }
	void startTag(const string& name, StringPairList& attribs, bool simple);
	void endTag(const string& name, const string& data);
	tstring finalize();
private:
	void parseColor(const char* rtfCode, const string& s);
	tstring ret;
	string fonts;
	string colors;
	size_t colorIndex;
	string header;
};

tstring HtmlToRtf::convert(const string& html) {
	Parser parser;
	try { SimpleXMLReader(&parser).parse(html); }
	catch(const SimpleXMLException& e) { return Text::toT(e.getError()); }
	return parser.finalize();
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
				state = Declaration;
				break;
			}

		case TextColor:
			{
				parseColor("\\cf", tmp);
				state = Declaration;
				break;
			}

		case BgColor:
			{
				parseColor("\\highlight", tmp);
				state = Declaration;
				break;
			}
		}
	}
}

void Parser::endTag(const string& name, const string& data) {
	if(header.empty()) {
		// no new RTF enclosing; write directly.
		ret += dwt::RichTextBox::rtfEscape(Text::toT(Text::toDOS(data)));

	} else {
		ret += _T("{") + Text::toT(header) + _T(" ") + dwt::RichTextBox::rtfEscape(Text::toT(Text::toDOS(data))) + _T("}");
		header.clear();
	}
}

tstring Parser::finalize() {
	if(fonts.empty() && colors.empty()) {
		// no new RTF enclosing; write directly.
		return ret;
	}

	return _T("{") + Text::toT(fonts + "{\\colortbl" + colors + "}") + ret + _T("}");
}

void Parser::parseColor(const char* rtfCode, const string& s) {
	auto sharp = s.find('#');
	if(sharp != string::npos && s.size() > sharp + 6) {
		try {
			size_t pos = 0;
			auto color = std::stol(s.substr(sharp + 1, 6), &pos, 16);
			colors += "\\red" + Util::toString((color & 0xFF0000) >> 16) +
				"\\green" + Util::toString((color & 0xFF00) >> 8) +
				"\\blue" + Util::toString(color & 0xFF) + ";";
			header += rtfCode;
			header += Util::toString(colorIndex++);
		} catch(const std::exception& e) { dcdebug("color parsing exception: %s with str: %s\n", e.what(), s.c_str()); }
	}
}
