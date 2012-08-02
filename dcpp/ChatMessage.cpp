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
#include "Magnet.h"
#include "OnlineUser.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "Util.h"
#include "PluginManager.h"

#include <boost/range/adaptor/reversed.hpp>

namespace dcpp {

using std::list;

ChatMessage::ChatMessage(const string& text, OnlineUser* from,
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

	if(SETTING(TIME_STAMPS)) {
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

	struct Tag { size_t pos; string s; bool opening; Tag* otherTag; };
	list<Tag> tags;

	/* link formatting - optimize the lookup a bit by using the fact that every link identifier
	(except www ones) contains a colon. */
	/// @todo add support for spaces within links enclosed by brackets / quotes (see URI RFC)

	auto addLinkStr = [&xmlTmp, &tags](size_t begin, size_t end, const string& link) {
		Tag openingTag = { begin, "<a href=\"" + SimpleXML::escape(link, xmlTmp, true) + "\">", true },
			closingTag = { end, "</a>", false };

		tags.push_back(std::move(openingTag));
		auto& opening = tags.back();

		tags.push_back(std::move(closingTag));
		auto& closing = tags.back();

		opening.otherTag = &closing;
		closing.otherTag = &opening;
	};

	auto addLink = [&tmp, &addLinkStr](size_t begin, size_t end) {
		addLinkStr(begin, end, tmp.substr(begin, end - begin));
	};

	static const string delimiters = " \t\r\n<>\"";

	i = 0;
	size_t begin, end;
	auto n = tmp.size();
	while((i = tmp.find(':', i)) != string::npos) {

		if((begin = tmp.find_last_of(delimiters, i)) == string::npos) begin = 0; else ++begin;
		if((end = tmp.find_first_of(delimiters, i + 1)) == string::npos) end = n;

		if(i > 0 && (
			(i + 4 < n && tmp[i + 1] == '/' && tmp[i + 2] == '/') || // "http://", etc
			(i == begin + 6 && i + 1 <= n && !tmp.compare(begin, 6, "mailto"))))
		{
			addLink(begin, end);
			i = end;

		} else if(i == begin + 6 && i + 2 <= n && !tmp.compare(begin, 6, "magnet") && tmp[i + 1] == '?') {
			string link = tmp.substr(begin, end - begin), hash, name, key;
			if(Magnet::parseUri(link, hash, name, key)) {

				if(!name.empty()) {
					// magnet link: replace with the friendly name
					name += " (magnet)";
					tmp.replace(begin, end - begin, name);

					// the size of the string has changed; update counts.
					auto delta = name.size() - link.size();
					end += delta;
					n += delta;
				}

				addLinkStr(begin, end, link);
				i = end;

			} else {
				++i;
			}

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
				continue;
			}
		}
		i += 5;
	}

	htmlMessage += "<span id=\"text\">";

	/* write tags and the message data. support entangled tags, such as:
	<a> <b> </a> </b> -> <a> <b> </b></a><b> </b> */

	tags.sort([](const Tag& a, const Tag& b) { return a.pos < b.pos; });

	size_t pos = 0;
	vector<Tag*> openTags;

	for(auto& tag: tags) {
		htmlMessage += SimpleXML::escape(tmp.substr(pos, tag.pos - pos), xmlTmp, false);
		pos = tag.pos;

		if(tag.opening) {
			htmlMessage += tag.s;
			openTags.push_back(&tag);

		} else {
			if(openTags.back() == tag.otherTag) {
				// common case: no entangled tag; just write the closing tag.
				htmlMessage += tag.s;
				openTags.pop_back();

			} else {
				// there are entangled tags: write closing & opening tags of currently open tags.
				for(auto openTag: openTags | boost::adaptors::reversed) { if(openTag->pos >= tag.otherTag->pos) {
					htmlMessage += openTag->otherTag->s;
				} }
				openTags.erase(remove(openTags.begin(), openTags.end(), tag.otherTag), openTags.end());
				for(auto openTag: openTags) { if(openTag->pos >= tag.otherTag->pos) {
					htmlMessage += openTag->s;
				} }
			}
		}
	}

	if(pos != n) {
		htmlMessage += SimpleXML::escape(tmp.substr(pos), xmlTmp, false);
	}

	htmlMessage += "</span></span>";

	/// forward to plugins
	PluginManager::getInstance()->onChatDisplay(htmlMessage, from);
}

} // namespace dcpp
