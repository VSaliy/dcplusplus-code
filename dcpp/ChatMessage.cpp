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
#include "Util.h"

#include <boost/range/adaptor/reversed.hpp>

namespace dcpp {

using std::make_pair;
using std::map;
using std::pair;

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

	htmlMessage += "<span id=\"message\" style=\"white-space: pre-wrap;\">";

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
	if(!style.font.empty()) { styleAttr += "font: " + Util::cssFont(style.font) + ";"; }
	if(style.textColor != -1) { styleAttr += "color: #" + Util::cssColor(style.textColor) + ";"; }
	if(style.bgColor != -1) { styleAttr += "background-color: #" + Util::cssColor(style.bgColor) + ";"; }
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

	/* format the message; this will involve adding custom tags. this table holds the tags to be
	added along with their position. */
	struct Tag { string s; bool opening; size_t otherTag; };
	map<size_t, Tag> tags;

	/* link formatting - optimize the lookup a bit by using the fact that every link identifier
	(except www ones) contains a colon. */
	/// @todo add support for spaces within links enclosed by brackets / quotes (see URI RFC)
	/// @todo friendly magnet links
	auto addLink = [&tmp, &xmlTmp, &tags](size_t begin, size_t end) {
		Tag openingTag = { "<a href=\"" + SimpleXML::escape(tmp.substr(begin, end - begin), xmlTmp, true) + "\">", true, end },
			closingTag = { "</a>", false, begin };
		tags.emplace(begin, std::move(openingTag));
		tags.emplace(end, std::move(closingTag));
	};

	static const string delimiters = " \t\r\n<>\"";

	i = 0;
	size_t begin, end;
	const auto n = tmp.size();
	while((i = tmp.find(':', i)) != string::npos) {

		if((begin = tmp.find_last_of(delimiters, i)) == string::npos) begin = 0; else ++begin;
		if((end = tmp.find_first_of(delimiters, i + 1)) == string::npos) end = n;

		if(i > 0 &&
			(i + 4 < n && tmp[i + 1] == '/' && tmp[i + 2] == '/') || // "http://", etc
			(i >= begin + 6 && !tmp.compare(begin, 6, "magnet")) ||
			(i >= begin + 6 && !tmp.compare(begin, 6, "mailto")))
		{
			addLink(begin, end);
			i = end;

		} else {
			++i;
		}
	}

	// check for www links.
	i = 0;
	while((i = tmp.find("www.", i)) != string::npos) {
		if(i + 5 <= n && (i == 0 || delimiters.find(tmp[i - 1]) != string::npos)) {
			if((end = tmp.find_first_of(delimiters, i + 4)) == string::npos) end = n;
			if(i + 5 <= end) {
				addLink(i, end);
				i = end;
			}
		}
		i += 5;
	}

	htmlMessage += "<span id=\"text\">";

	/* write tags and the message data. support entangled tags, such as:
	<a> <b> </a> </b> -> <a> <b> </b></a><b> </b> */

	size_t pos = 0;
	vector<size_t> openTags;

	for(auto& tag: tags) {
		htmlMessage += SimpleXML::escape(tmp.substr(pos, tag.first - pos), xmlTmp, false);
		pos = tag.first;

		if(tag.second.opening) {
			htmlMessage += tag.second.s;
			openTags.push_back(tag.first);

		} else {
			if(openTags.back() == tag.second.otherTag) {
				// common case: no entangled tag; just write the closing tag.
				htmlMessage += tag.second.s;
				openTags.pop_back();

			} else {
				// there are entangled tags: write closing & opening tags of currently open tags.
				for(auto key: openTags | boost::adaptors::reversed) { if(key >= tag.second.otherTag) {
					htmlMessage += tags[tags[key].otherTag].s;
				} }
				openTags.erase(remove(openTags.begin(), openTags.end(), tag.second.otherTag), openTags.end());
				for(auto key: openTags) { if(key >= tag.second.otherTag) {
					htmlMessage += tags[key].s;
				} }
			}
		}
	}

	if(pos != n) {
		htmlMessage += SimpleXML::escape(tmp.substr(pos), xmlTmp, false);
	}

	htmlMessage += "</span></span>";

	dcdebug("html: %s\n", htmlMessage.c_str());

	/// @todo send this to plugins
}

} // namespace dcpp
