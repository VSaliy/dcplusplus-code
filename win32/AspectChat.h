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

protected:
	AspectChat() : message(0), chat(0) { }
	virtual ~AspectChat() { }

	void clearChat(const tstring& param) {
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
	}

	dwt::TextBoxPtr message;
	dwt::TextBoxPtr chat;
};

#endif // !defined(DCPLUSPLUS_WIN32_ASPECT_CHAT_H)
