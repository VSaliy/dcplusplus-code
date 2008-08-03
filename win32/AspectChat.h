/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_ASPECT_CHAT_H
#define DCPLUSPLUS_WIN32_ASPECT_CHAT_H

#include "HoldRedraw.h"

template<typename T>
class AspectChat {
	typedef AspectChat<T> ThisType;

	T& t() { return *static_cast<T*>(this); }
	const T& t() const { return *static_cast<const T*>(this); }

protected:
	AspectChat() :
	chat(0),
	message(0),
	timeStamps(BOOLSETTING(TIME_STAMPS))
	{
		{
			TextBox::Seed cs = WinUtil::Seeds::textBox;
			cs.style |= WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY;
			chat = t().addChild(cs);
			chat->setTextLimit(0);
		}

		{
			TextBox::Seed cs = WinUtil::Seeds::textBox;
			cs.style |= WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE;
			message = t().addChild(cs);
		}
	}

	virtual ~AspectChat() { }

	void addChat(const tstring& aLine) {
		tstring line;
		if(timeStamps) {
			line = Text::toT("\r\n[" + Util::getShortTimeString() + "] ");
		} else {
			line = _T("\r\n");
		}
		line += Text::toDOS(aLine);

		bool scroll = chat->scrollIsAtEnd();
		HoldRedraw hold(chat, !scroll);

		size_t limit = chat->getTextLimit();
		if(chat->length() + line.size() > limit) {
			HoldRedraw hold2(chat, scroll);
			chat->setSelection(0, chat->lineIndex(chat->lineFromChar(limit / 10)));
			chat->replaceSelection(_T(""));
		}

		chat->addText(line);

		if(scroll)
			chat->sendMessage(WM_VSCROLL, SB_BOTTOM);
	}

	bool checkCommand(const tstring& cmd, const tstring& param, tstring& status) {
		if(Util::stricmp(cmd.c_str(), _T("clear")) == 0) {
			unsigned linesToKeep = 0;
			if(!param.empty())
				linesToKeep = Util::toInt(Text::fromT(param));
			if(linesToKeep) {
				unsigned lineCount = chat->getLineCount();
				if(linesToKeep < lineCount) {
					HoldRedraw hold(chat);
					chat->setSelection(0, chat->lineIndex(lineCount - linesToKeep));
					chat->replaceSelection(_T(""));
				}
			} else
				chat->setText(_T(""));
		} else if(Util::stricmp(cmd.c_str(), _T("ts")) == 0) {
			timeStamps = !timeStamps;
			if(timeStamps) {
				status = T_("Timestamps enabled");
			} else {
				status = T_("Timestamps disabled");
			}
		} else {
			return false;
		}
		return true;
	}

	dwt::TextBoxPtr chat;
	dwt::TextBoxPtr message;

private:
	bool timeStamps;
};

#endif // !defined(DCPLUSPLUS_WIN32_ASPECT_CHAT_H)
