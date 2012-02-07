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

#include "stdinc.h"
#include "ChatMessage.h"

#include "Client.h"
#include "format.h"
#include "OnlineUser.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "Util.h"

namespace dcpp {

namespace {

// helpers that convert style settings to CSS declarations (implementation dependant).

string cssColor(int color) {
#ifdef _WIN32
	// assume it's a COLORREF.
	char buf[8];
	snprintf(buf, sizeof(buf), "%.2X%.2X%.2X", GetRValue(color), GetGValue(color), GetBValue(color));
	return buf;
#else
	///@todo
	return string();
#endif
}

string cssFont(const string& font) {
#ifdef _WIN32
	StringTokenizer<string> st(font, ',');
	auto& l = st.getTokens();
	if(l.size() >= 4) {
		std::stringstream stream;
		stream << (Util::toInt(l[3]) ? "italic" : "normal") << " " << l[2] << " " <<
			abs(Util::toFloat(l[1])) << "px '" << l[0] << "'";
		return stream.str();
	}
	return string();
#else
	///@todo
	return string();
#endif
}

} // unnamed namespace

ChatMessage::ChatMessage(const string& text, const OnlineUser* from,
	const OnlineUser* to, const OnlineUser* replyTo,
	bool thirdPerson, time_t messageTimestamp) :
from(from->getUser()),
to(to ? to->getUser() : nullptr),
replyTo(replyTo ? replyTo->getUser() : nullptr),
timestamp(time(0)),
thirdPerson(thirdPerson),
messageTimestamp(messageTimestamp)
{
	/* format the message to plain text and HTML strings before handing them over to plugins for
	further	processing. */

	string tmp;
	string xmlTmp;

	auto addSpan = [&xmlTmp](char* id, const string& content, const string& style) -> string {
		std::stringstream stream;
		stream << "<span id=\"" << id << "\"";
		if(!style.empty()) { stream << " style=\"" << SimpleXML::escape(style, xmlTmp, true) << "\""; }
		stream << ">" << SimpleXML::escape(content, xmlTmp, false) << "</span>";
		return stream.str();
	};

	htmlMessage += "<span id=\"message\">";

	if(BOOLSETTING(TIME_STAMPS)) {
		tmp = "[" + Util::getShortTimeString(timestamp) + "]";
		htmlMessage += addSpan("timestamp", tmp, Util::emptyString) + " ";
	}

	if(messageTimestamp) {
		tmp = "["; tmp += str(F_("Sent %1%") % Util::getShortTimeString(messageTimestamp)); tmp += "]";
		message += tmp + " ";
		htmlMessage += addSpan("messageTimestamp", tmp, Util::emptyString) + " ";
	}

	tmp = from->getIdentity().getNick();
	// let's *not* obey the spec here and add a space after the star. :P
	tmp = thirdPerson ? "* " + tmp + " " : "<" + tmp + ">";
	message += tmp + " ";

	auto style = from->getIdentity().getStyle();
	string styleAttr;
	if(!style.font.empty()) { styleAttr += "font: " + cssFont(style.font) + ";"; }
	if(style.textColor != -1) { styleAttr += "color: #" + cssColor(style.textColor) + ";"; }
	if(style.bgColor != -1) { styleAttr += "background-color: #" + cssColor(style.bgColor) + ";"; }
	htmlMessage += addSpan("nick", tmp, styleAttr) + " ";

	// Check all '<' and '[' after newlines as they're probably pastes...
	tmp = text;
	size_t i = 0;
	while((i = tmp.find('\n', i)) != string::npos) {
		++i;
		if(i < tmp.size()) {
			if(tmp[i] == '[' || tmp[i] == '<') {
				tmp.insert(i, "- ");
				i += 2;
			}
		}
	}

	message += tmp;
	htmlMessage += addSpan("text", tmp, Util::emptyString) + "</span>";

	/// @todo send this to plugins
}

} // namespace dcpp
