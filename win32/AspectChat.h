/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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
	messageLines(1),
	timeStamps(BOOLSETTING(TIME_STAMPS)),
	curCommandPosition(0)
	{
	}

	void createChat(dwt::Widget *parent) {
		{
			RichTextBox::Seed cs = WinUtil::Seeds::richTextBox;
			cs.style |= ES_READONLY;
			chat = dwt::WidgetCreator<RichTextBox>::create(parent, cs);
			chat->setTextLimit(65536);
			WinUtil::handleDblClicks(chat);
		}

		{
			TextBox::Seed cs = WinUtil::Seeds::textBox;
			cs.style |= WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL;
			message = t().addChild(cs);
			message->onUpdated(std::bind(&ThisType::handleMessageUpdated, this));
		}

		t().addAccel(FALT, 'C', std::bind(&dwt::Control::setFocus, chat));
		t().addAccel(FALT, 'M', std::bind(&dwt::Control::setFocus, message));
		t().addAccel(FALT, 'S', std::bind(&ThisType::sendMessage_, this));
		t().addAccel(0, VK_ESCAPE, std::bind(&ThisType::handleEscape, this));
		t().addAccel(FCONTROL, 'F', [this] { GCC_WTF->chat->findTextNew(); });
		t().addAccel(0, VK_F3, [this] { GCC_WTF->chat->findTextNext(); });
	}

	virtual ~AspectChat() { }

	tstring formatChatMessage(Client* aClient, const tstring& aLine) {
		/// @todo factor out to dwt
		/// @todo Text::toT works but _T doesn't, verify this.
		return Text::toT("{\\urtf1\n") + chat->rtfEscape(aLine) + Text::toT("}\n");
	}

	void addChat(Client* aClient, const tstring& aLine) {
		tstring line;
		if(chat->length() > 0)
			line += _T("\r\n");
		if(timeStamps)
			line += Text::toT("[" + Util::getShortTimeString() + "] ");
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
					{
						HoldRedraw hold(chat);
						chat->setSelection(0, chat->lineIndex(lineCount - linesToKeep));
						chat->replaceSelection(_T(""));
					}
					chat->redraw();
				}
			} else {
				chat->setSelection();
				chat->replaceSelection(_T(""));
			}

		} else if(Util::stricmp(cmd.c_str(), _T("f")) == 0) {
			chat->findText(param.empty() ? chat->findTextPopup() : param);

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

	bool handleMessageKeyDown(int c) {
		switch(c) {
		case VK_RETURN: {
			if(t().isShiftPressed() || t().isControlPressed() || t().isAltPressed()) {
				return false;
			}
			return sendMessage();
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

	void handleEscape() {
		chat->sendMessage(WM_KEYDOWN, VK_ESCAPE);
		message->setFocus();
	}

	RichTextBox* chat;
	dwt::TextBoxPtr message;

	unsigned messageLines;

private:
	bool timeStamps;

	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition; //can't use an iterator because StringList is a vector, and vector iterators become invalid after resizing

	void handleMessageUpdated() {
		unsigned lineCount = message->getLineCount();

		// make sure we don't resize to 0 lines...
		const unsigned min_setting = max(SETTING(MIN_MESSAGE_LINES), 1);
		const unsigned max_setting = max(SETTING(MAX_MESSAGE_LINES), 1);

		if(lineCount < min_setting)
			lineCount = min_setting;
		else if(lineCount > max_setting)
			lineCount = max_setting;

		if(lineCount != messageLines) {
			messageLines = lineCount;
			t().layout();
		}
	}

	bool historyActive() const {
		return t().isAltPressed() || (BOOLSETTING(USE_CTRL_FOR_LINE_HISTORY) && t().isControlPressed());
	}

	bool sendMessage() {
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
	void sendMessage_() { sendMessage(); }
};

#endif // !defined(DCPLUSPLUS_WIN32_ASPECT_CHAT_H)
