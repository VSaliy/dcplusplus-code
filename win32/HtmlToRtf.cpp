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

#include "stdafx.h"
#include "HtmlToRtf.h"

#include <dcpp/SimpleXML.h>

#ifdef TODO
namespace { tstring toRTF(COLORREF color) {
	return Text::toT("\\red" + Util::toString(GetRValue(color)) +
		"\\green" + Util::toString(GetGValue(color)) +
		"\\blue" + Util::toString(GetBValue(color)) + ";");
} }

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

	auto tmp = Text::toT(Text::toDOS(text));
	ret.first += tmp;
	ret.second += chat->rtfEscape(tmp);
}
#endif

struct Parser : SimpleXMLReader::CallBack {
	Parser() { }
	void startTag(const string& name, StringPairList& attribs, bool simple);
	void endTag(const string& name, const string& data);
	string finalize();
private:
	string ret;
};

string HtmlToRtf::convert(const string& html) {
	Parser parser;
	try { SimpleXMLReader(&parser).parse(html); }
	catch(const SimpleXMLException& e) { return e.getError(); }
	return parser.finalize();
}

void Parser::startTag(const string& name, StringPairList& attribs, bool simple) {
	ret += name;
}

void Parser::endTag(const string& name, const string& data) {
	ret += data;
}

string Parser::finalize() {
	return ret;
}
