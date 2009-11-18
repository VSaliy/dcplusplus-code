/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#include <dcpp/File.h>

#include "HoldRedraw.h"
#include "RichTextBox.h"

template<typename T>
class AspectChat {
	typedef AspectChat<T> ThisType;

	const T& t() const { return *static_cast<const T*>(this); }
	T& t() { return *static_cast<T*>(this); }

protected:
	AspectChat() :
	chat(0),
	message(0),
	timeStamps(BOOLSETTING(TIME_STAMPS)),
	curCommandPosition(0)
	{
		{
			RichTextBox::Seed cs = WinUtil::Seeds::richTextBox;
			cs.style |= ES_READONLY;
			chat = t().addChild(cs);
			chat->setTextLimit(32768);
		}

		{
			TextBox::Seed cs = WinUtil::Seeds::textBox;
			cs.style |= WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE;
			message = t().addChild(cs);
		}
	}

	virtual ~AspectChat() { }

	tstring formatChatMessage(Client* aClient, const tstring& aLine) {
		uint32_t color = SETTING(TEXT_COLOR);
		string prologue =  "{\\urtf1\\ul0\\b0\\i0\n{\\colortbl;\\red" + Util::toString(color & 0xFF) +
		  "\\green" + Util::toString((color >> 8) & 0xFF) + "\\blue" +
		  Util::toString((color >> 16) & 0xFF) + ";}\n\\cf1\n";

		return Text::toT(prologue) + chat->rtfEscape(aLine) + Text::toT("}\n");
	}

	void addChat(Client* aClient, const tstring& aLine) {
		tstring line;
		if(timeStamps) {
			line = Text::toT("\r\n[" + Util::getShortTimeString() + "] ");
		} else {
			line = _T("\r\n");
		}
		line += Text::toDOS(aLine);
		chat->addTextSteady(formatChatMessage(aClient, line), line.size());
	}

	void readLog(const string& logPath, const unsigned setting) {
		if(setting == 0)
			return;

		StringList lines;

		try {
			const int MAX_SIZE = 32 * 1024;

			File f(logPath.empty() ? t().getLogPath() : logPath, File::READ, File::OPEN);
			if(f.getSize() > MAX_SIZE) {
				f.setEndPos(-MAX_SIZE + 1);
			}

			lines = StringTokenizer<string>(f.read(MAX_SIZE), "\r\n").getTokens();
		} catch(const FileException&) { }

		if(lines.empty())
			return;

		// the last line in the log file is an empty line; remove it
		lines.pop_back();

		const size_t linesCount = lines.size();
		for(size_t i = ((linesCount > setting) ? (linesCount - setting) : 0); i < linesCount; ++i) {
			addChat(0, _T("- ") + Text::toT(lines[i]));
		}
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

	bool handleChatKeyDown(int c) {
		switch(c) {
			case VK_F3:
				chat->findTextNext();
				break;
			case VK_ESCAPE:
				chat->setSelection(-1, -1);
				chat->sendMessage(WM_VSCROLL, SB_BOTTOM);
				message->setFocus();
				chat->clearCurrentNeedle();
				break;
		}

		return false;
	}
	
	bool handleMessageKeyDown(int c) {
		switch(c) {
		case VK_RETURN: {
			if(t().isShiftPressed() || t().isControlPressed() || t().isAltPressed()) {
				return false;
			}
			tstring s = message->getText();
			if(s.empty()) {
				::MessageBeep(MB_ICONEXCLAMATION);
				return false;
			}

			// save command in history, reset current buffer pointer to the newest command
			curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
			if (curCommandPosition == 0 || (curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s)) {
				++curCommandPosition;
				prevCommands.push_back(s);
			}
			currentCommand = _T("");

			t().enterImpl(s);

			return true;
		}
		case VK_UP:
			if ( historyActive() ) {
				//scroll up in chat command history
				//currently beyond the last command?
				if (curCommandPosition > 0) {
					//check whether current command needs to be saved
					if (curCommandPosition == prevCommands.size()) {
						currentCommand = message->getText();
					}

					//replace current chat buffer with current command
					message->setText(prevCommands[--curCommandPosition]);
				}
				// move cursor to end of line
				message->setSelection(message->length(), message->length());
				return true;
			}
			break;
		case VK_DOWN:
			if ( historyActive() ) {
				//scroll down in chat command history

				//currently beyond the last command?
				if (curCommandPosition + 1 < prevCommands.size()) {
					//replace current chat buffer with current command
					message->setText(prevCommands[++curCommandPosition]);
				} else if (curCommandPosition + 1 == prevCommands.size()) {
					//revert to last saved, unfinished command

					message->setText(currentCommand);
					++curCommandPosition;
				}
				// move cursor to end of line
				message->setSelection(message->length(), message->length());
				return true;
			}
			break;
		case VK_PRIOR: // page up
			{
				chat->sendMessage(WM_VSCROLL, SB_PAGEUP);
				return true;
			} break;
		case VK_NEXT: // page down
			{
				chat->sendMessage(WM_VSCROLL, SB_PAGEDOWN);
				return true;
			} break;
		case VK_HOME:
			if (!prevCommands.empty() && historyActive() ) {
				curCommandPosition = 0;
				currentCommand = message->getText();

				message->setText(prevCommands[curCommandPosition]);
				return true;
			}
			break;
		case VK_END:
			if (historyActive()) {
				curCommandPosition = prevCommands.size();

				message->setText(currentCommand);
				return true;
			}
			break;
		}
		return false;
	}

	bool handleMessageChar(int c) {
		switch(c) {
		case VK_RETURN: {
			if(!(t().isShiftPressed() || t().isControlPressed() || t().isAltPressed())) {
				return true;
			}
		} break;
		}
		return false;
	}

	RichTextBox* chat;
	dwt::TextBoxPtr message;

private:
	bool timeStamps;

	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition; //can't use an iterator because StringList is a vector, and vector iterators become invalid after resizing

	bool historyActive() {
		return t().isAltPressed() || (BOOLSETTING(USE_CTRL_FOR_LINE_HISTORY) && t().isControlPressed());
	}
};

#endif // !defined(DCPLUSPLUS_WIN32_ASPECT_CHAT_H)
