/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#include "WinUtil.h"

#include "resource.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/HashManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/StringTokenizer.h>
#include <dcpp/version.h>
#include <dcpp/File.h>
#include <dcpp/UserCommand.h>
#include <dcpp/UploadManager.h>
#include <dcpp/ThrottleManager.h>

#include "ParamDlg.h"
#include "MagnetDlg.h"
#include "HubFrame.h"
#include "SearchFrame.h"
#include "MainWindow.h"
#include "PrivateFrame.h"

#include <dwt/DWTException.h>

// def taken from <gettextP.h>
extern "C" const char *_nl_locale_name_default(void);

tstring WinUtil::tth;
dwt::BrushPtr WinUtil::bgBrush;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
dwt::FontPtr WinUtil::font;
dwt::FontPtr WinUtil::monoFont;
dwt::ImageListPtr WinUtil::fileImages;
dwt::ImageListPtr WinUtil::userImages;
int WinUtil::fileImageCount;
int WinUtil::dirIconIndex;
int WinUtil::dirMaskedIndex;
TStringList WinUtil::lastDirs;
MainWindow* WinUtil::mainWindow = 0;
bool WinUtil::urlDcADCRegistered = false;
bool WinUtil::urlMagnetRegistered = false;
WinUtil::ImageMap WinUtil::fileIndexes;
DWORD WinUtil::helpCookie = 0;
tstring WinUtil::helpPath;
StringList WinUtil::helpTexts;

const Button::Seed WinUtil::Seeds::button;
const ComboBox::Seed WinUtil::Seeds::comboBoxStatic;
const ComboBox::Seed WinUtil::Seeds::comboBoxEdit;
const CheckBox::Seed WinUtil::Seeds::checkBox;
const CheckBox::Seed WinUtil::Seeds::splitCheckBox;
const GroupBox::Seed WinUtil::Seeds::group;
const Menu::Seed WinUtil::Seeds::menu;
const Table::Seed WinUtil::Seeds::table;
const TextBox::Seed WinUtil::Seeds::textBox;
const RichTextBox::Seed WinUtil::Seeds::richTextBox;
const TabView::Seed WinUtil::Seeds::tabs;
const Tree::Seed WinUtil::Seeds::treeView;

const ComboBox::Seed WinUtil::Seeds::Dialog::comboBox;
const TextBox::Seed WinUtil::Seeds::Dialog::textBox;
const TextBox::Seed WinUtil::Seeds::Dialog::intTextBox;
const Table::Seed WinUtil::Seeds::Dialog::table;
const Table::Seed WinUtil::Seeds::Dialog::optionsTable;

void WinUtil::init() {

	SettingsManager::getInstance()->setDefault(SettingsManager::BACKGROUND_COLOR, (int) (GetSysColor(COLOR_WINDOW)));
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_COLOR, (int) (GetSysColor(COLOR_WINDOWTEXT)));

	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	bgBrush = dwt::BrushPtr(new dwt::Brush(bgColor));

	LOGFONT lf;
	::GetObject((HFONT) GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, Text::fromT(encodeFont(lf)));
	decodeFont(Text::toT(SETTING(TEXT_FONT)), lf);

	font = dwt::FontPtr(new dwt::Font(::CreateFontIndirect(&lf), true));
	monoFont = dwt::FontPtr(new dwt::Font((BOOLSETTING(USE_OEM_MONOFONT) ? dwt::OemFixedFont : dwt::AnsiFixedFont)));

	fileImages = dwt::ImageListPtr(new dwt::ImageList(dwt::Point(16, 16)));

	dirIconIndex = fileImageCount++;
	dirMaskedIndex = fileImageCount++;

	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		::SHGetFileInfo(_T("."), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON
			| SHGFI_USEFILEATTRIBUTES);
		dwt::Icon tmp(fi.hIcon);
		fileImages->add(tmp);
		// @todo This one should be masked further for the incomplete folder thing
		fileImages->add(tmp);
	} else {
		dwt::Bitmap tmp(IDB_FOLDERS);
		fileImages->add(tmp, RGB(255, 0, 255));

		// Unknown file
		fileImageCount++;
	}

	{
		userImages = dwt::ImageListPtr(new dwt::ImageList(dwt::Point(16, 16)));
		dwt::Bitmap tmp(IDB_USERS);
		userImages->add(tmp, RGB(255, 0, 255));
	}

	registerHubHandlers();
	registerMagnetHandler();

	// Const so that noone else will change them after they've been initialized
	Button::Seed& xbutton = const_cast<Button::Seed&> (Seeds::button);
	ComboBox::Seed& xcomboBoxEdit = const_cast<ComboBox::Seed&> (Seeds::comboBoxEdit);
	ComboBox::Seed& xcomboBoxStatic = const_cast<ComboBox::Seed&> (Seeds::comboBoxStatic);
	CheckBox::Seed& xCheckBox = const_cast<CheckBox::Seed&> (Seeds::checkBox);
	CheckBox::Seed& xSplitCheckBox = const_cast<CheckBox::Seed&> (Seeds::splitCheckBox);
	GroupBox::Seed& xgroup = const_cast<GroupBox::Seed&> (Seeds::group);
	Menu::Seed& xmenu = const_cast<Menu::Seed&> (Seeds::menu);
	Table::Seed& xTable = const_cast<Table::Seed&> (Seeds::table);
	TextBox::Seed& xtextBox = const_cast<TextBox::Seed&> (Seeds::textBox);
	RichTextBox::Seed& xRichTextBox = const_cast<RichTextBox::Seed&> (Seeds::richTextBox);
	TabView::Seed& xTabs = const_cast<TabView::Seed&> (Seeds::tabs);
	Tree::Seed& xtreeView = const_cast<Tree::Seed&> (Seeds::treeView);
	ComboBox::Seed& xdComboBox = const_cast<ComboBox::Seed&> (Seeds::Dialog::comboBox);
	TextBox::Seed& xdTextBox = const_cast<TextBox::Seed&> (Seeds::Dialog::textBox);
	TextBox::Seed& xdintTextBox = const_cast<TextBox::Seed&> (Seeds::Dialog::intTextBox);
	Table::Seed& xdTable = const_cast<Table::Seed&> (Seeds::Dialog::table);
	Table::Seed& xdoptionsTable = const_cast<Table::Seed&> (Seeds::Dialog::optionsTable);

	xbutton.font = font;

	xcomboBoxStatic.style |= CBS_DROPDOWNLIST;
	xcomboBoxStatic.font = font;

	xcomboBoxEdit.font = font;

	xCheckBox.font = font;

	xSplitCheckBox = xCheckBox;
	xSplitCheckBox.caption = _T("+/-");
	xSplitCheckBox.style &= ~WS_TABSTOP;

	xgroup.font = font;

	xmenu.ownerDrawn = BOOLSETTING(OWNER_DRAWN_MENUS);
	if(xmenu.ownerDrawn)
		xmenu.font = font;

	xTable.style |= WS_HSCROLL | WS_VSCROLL | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS;
	xTable.exStyle = WS_EX_CLIENTEDGE;
	xTable.lvStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	xTable.font = font;

	xtextBox.exStyle = WS_EX_CLIENTEDGE;
	xtextBox.font = font;

	xRichTextBox.exStyle = WS_EX_CLIENTEDGE;
	xRichTextBox.font = font;
	xRichTextBox.foregroundColor = textColor;
	xRichTextBox.backgroundColor = bgColor;

	xTabs.font = font;

	xtreeView.style |= TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP;
	xtreeView.exStyle = WS_EX_CLIENTEDGE;
	xtreeView.font = font;

	xdComboBox.style |= CBS_DROPDOWNLIST;

	xdTextBox.style |= ES_AUTOHSCROLL;

	xdintTextBox = xdTextBox;
	xdintTextBox.style |= ES_NUMBER;

	xdTable.style |= WS_HSCROLL | WS_VSCROLL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
	xdTable.exStyle = WS_EX_CLIENTEDGE;
	xdTable.lvStyle |= LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;

	xdoptionsTable = xdTable;
	xdoptionsTable.style |= LVS_SINGLESEL | LVS_NOCOLUMNHEADER;
	xdoptionsTable.lvStyle |= LVS_EX_CHECKBOXES;

	init_helpPath();

	if(!helpPath.empty()) {
		// load up context-sensitive help texts
		try {
			helpTexts = StringTokenizer<string> (File(Util::getFilePath(Text::fromT(helpPath)) + "cshelp.rtf",
				File::READ, File::OPEN).read(), "\r\n").getTokens();
		} catch (const FileException&) {
		}
	}

	::HtmlHelp(NULL, NULL, HH_INITIALIZE, reinterpret_cast<DWORD_PTR> (&helpCookie));
}

void WinUtil::init_helpPath() {
	// find the current locale
	string lang = SETTING(LANGUAGE);
	if(lang.empty())
		lang = _nl_locale_name_default();

	// find the path to the help file
	string path;
	if(!lang.empty() && lang != "C") {
		while(true) {
			path = Util::getPath(Util::PATH_LOCALE) + lang
				+ PATH_SEPARATOR_STR "help" PATH_SEPARATOR_STR "DCPlusPlus.chm";
			if(File::getSize(path) != -1)
				break;
			// if the lang has extra information (after '_' or '@'), try to remove it
			string::size_type pos = lang.find_last_of("_@");
			if(pos == string::npos)
				break;
			lang = lang.substr(0, pos);
		}
	}
	if(path.empty() || File::getSize(path) == -1) {
		path = Util::getPath(Util::PATH_RESOURCES) + "DCPlusPlus.chm";
		if(File::getSize(path) == -1) {
			/// @todo also check that the file is up-to-date
			/// @todo alert the user that the help file isn't found/up-to-date
			return;
		}
	}
	helpPath = Text::toT(path);
}

void WinUtil::uninit() {
	::HtmlHelp(NULL, NULL, HH_UNINITIALIZE, helpCookie);
}

tstring WinUtil::encodeFont(LOGFONT const& font) {
	tstring res(font.lfFaceName);
	res += _T(',');
	res += Text::toT(Util::toString(font.lfHeight));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfWeight));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfItalic));
	return res;
}

std::string WinUtil::toString(const std::vector<int>& tokens) {
	std::string ret;
	for(std::vector<int>::const_iterator i = tokens.begin(); i != tokens.end(); ++i) {
		ret += Util::toString(*i) + ',';
	}
	if(!ret.empty())
		ret.erase(ret.size() - 1);
	return ret;
}

void WinUtil::decodeFont(const tstring& setting, LOGFONT &dest) {
	StringTokenizer<tstring> st(setting, _T(','));
	TStringList &sl = st.getTokens();

	::GetObject((HFONT) GetStockObject(DEFAULT_GUI_FONT), sizeof(dest), &dest);
	tstring face;
	if(sl.size() == 4) {
		face = sl[0];
		dest.lfHeight = Util::toInt(Text::fromT(sl[1]));
		dest.lfWeight = Util::toInt(Text::fromT(sl[2]));
		dest.lfItalic = (BYTE) Util::toInt(Text::fromT(sl[3]));
	}

	if(!face.empty()) {
		::ZeroMemory(dest.lfFaceName, LF_FACESIZE);
		_tcscpy(dest.lfFaceName, face.c_str());
	}
}

void WinUtil::setStaticWindowState(const string& id, bool open) {
	mainWindow->setStaticWindowState(id, open);
}

bool WinUtil::checkNick() {
	if(SETTING(NICK).empty()) {
		dwt::MessageBox(mainWindow).show(T_("Please enter a nickname in the settings dialog!"),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
		mainWindow->handleSettings();
		return false;
	}

	return true;
}

#define LINE2 _T("-- http://dcplusplus.sourceforge.net <DC++ ") _T(VERSIONSTRING) _T(">")
const TCHAR
	*msgs[] = {
		_T("\r\n-- I'm a happy dc++ user. You could be happy too.\r\n") LINE2,
		_T("\r\n-- Neo-...what? Nope...never heard of it...\r\n") LINE2,
		_T("\r\n-- Evolution of species: Ape --> Man\r\n-- Evolution of science: \"The Earth is Flat\" --> \"The Earth is Round\"\r\n-- Evolution of sharing: NMDC --> DC++\r\n") LINE2,
		_T("\r\n-- I share, therefore I am.\r\n") LINE2,
		_T("\r\n-- I came, I searched, I found...\r\n") LINE2,
		_T("\r\n-- I came, I shared, I sent...\r\n") LINE2,
		_T("\r\n-- I can set away mode, can't you?\r\n") LINE2,
		_T("\r\n-- I don't have to see any ads, do you?\r\n") LINE2,
		_T("\r\n-- I don't have to see those annoying kick messages, do you?\r\n") LINE2,
		_T("\r\n-- I can resume my files to a different filename, can you?\r\n") LINE2,
		_T("\r\n-- I can share huge amounts of files, can you?\r\n") LINE2,
		_T("\r\n-- My client doesn't spam the chat with useless debug messages, does yours?\r\n") LINE2,
		_T("\r\n-- I can add multiple users to the same download and have the client connect to another automatically when one goes offline, can you?\r\n") LINE2,
		_T("\r\n-- These addies are pretty annoying, aren't they? Get revenge by sending them yourself!\r\n") LINE2,
		_T("\r\n-- My client supports TTH hashes, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports XML file lists, does yours?\r\n") LINE2 };

#define MSGS 16

tstring
	WinUtil::commands =
		_T("/refresh, /me <msg>, /clear [lines to keep], /slots #, /dslots #, /search <string>, /dc++, /away <msg>, /back, /g <searchstring>, /imdb <imdbquery>, /u <url>, /rebuild, /ts, /download, /upload");

bool WinUtil::checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson) {
	string::size_type i = cmd.find(' ');
	if(i != string::npos) {
		param = cmd.substr(i + 1);
		cmd = cmd.substr(1, i - 1);
	} else {
		cmd = cmd.substr(1);
	}

	if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
		if(Util::stricmp(param.c_str(), _T("system")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM))));
		} else if(Util::stricmp(param.c_str(), _T("downloads")) == 0) {
			WinUtil::openFile(Text::toT(
				Util::validateFileName(LogManager::getInstance()->getPath(LogManager::DOWNLOAD))));
		} else if(Util::stricmp(param.c_str(), _T("uploads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::UPLOAD))));
		} else {
			return false;
		}
	} else if(Util::stricmp(cmd.c_str(), _T("me")) == 0) {
		message = param;
		thirdPerson = true;
	} else if(Util::stricmp(cmd.c_str(), _T("refresh")) == 0) {
		try {
			ShareManager::getInstance()->setDirty();
			ShareManager::getInstance()->refresh(true);
		} catch (const ShareException& e) {
			status = Text::toT(e.getError());
		}
	} else if(Util::stricmp(cmd.c_str(), _T("slots")) == 0) {
		int j = Util::toInt(Text::fromT(param));
		if(j > 0) {
			SettingsManager::getInstance()->set(ThrottleManager::getInstance()->getCurSetting(SettingsManager::SLOTS), j);
			status = T_("Slots set");
			ClientManager::getInstance()->infoUpdated();
		} else {
			status = T_("Invalid number of slots");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("dslots")) == 0) {
		int j = Util::toInt(Text::fromT(param));
		if(j >= 0) {
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, j);
			status = T_("Download slots set");
		} else {
			status = T_("Invalid number of slots");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("search")) == 0) {
		if(!param.empty()) {
			SearchFrame::openWindow(mainWindow->getTabView(), param, SearchManager::TYPE_ANY);
		} else {
			status = T_("Specify a search string");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("dc++")) == 0) {
		message = msgs[GET_TICK() % MSGS];
	} else if(Util::stricmp(cmd.c_str(), _T("away")) == 0) {
		if(Util::getAway() && param.empty()) {
			Util::setAway(false);
			Util::setManualAway(false);
			status = T_("Away mode off");
		} else {
			Util::setAway(true);
			Util::setManualAway(true);
			Util::setAwayMessage(Text::fromT(param));
			status = str(TF_("Away mode on: %1%") % Text::toT(Util::getAwayMessage()));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("back")) == 0) {
		Util::setAway(false);
		Util::setManualAway(false);
		status = T_("Away mode off");
	} else if(Util::stricmp(cmd.c_str(), _T("g")) == 0) {
		if(param.empty()) {
			status = T_("Specify a search string");
		} else {
			WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("imdb")) == 0) {
		if(param.empty()) {
			status = T_("Specify a search string");
		} else {
			WinUtil::openLink(_T("http://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("u")) == 0) {
		if(param.empty()) {
			status = T_("Specify a URL");
		} else {
			WinUtil::openLink(Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("rebuild")) == 0) {
		HashManager::getInstance()->rebuild();
	} else if(Util::stricmp(cmd.c_str(), _T("upload")) == 0) {
		SettingsManager::getInstance()->set(ThrottleManager::getInstance()->getCurSetting(SettingsManager::MAX_UPLOAD_SPEED_MAIN), Util::toInt(Text::fromT(param)));
		ClientManager::getInstance()->infoUpdated();
		if(Util::toInt(Text::fromT(param))) {
			TCHAR* temp;
			temp = new TCHAR[T_("Upload limit set to %d KiB/s").size() + 32];
			_stprintf(temp, T_("Upload limit set to %d KiB/s").c_str(), Util::toInt(Text::fromT(param)));
			status = temp;
			delete[] temp;
		} else {
			status = T_("Upload limit disabled").c_str();
		}
	} else if(Util::stricmp(cmd.c_str(), _T("download")) == 0) {
		SettingsManager::getInstance()->set(ThrottleManager::getInstance()->getCurSetting(SettingsManager::MAX_DOWNLOAD_SPEED_MAIN),
			Util::toInt(Text::fromT(param)));
		ClientManager::getInstance()->infoUpdated();
		if(Util::toInt(Text::fromT(param))) {
			TCHAR* temp;
			temp = new TCHAR[T_("Download limit set to %d KiB/s").size() + 32];
			_stprintf(temp, T_("Download limit set to %d KiB/s").c_str(), Util::toInt(Text::fromT(param)));
			status = temp;
			delete[] temp;
		} else {
			status = T_("Download limit disabled");
		}
	} else {
		return false;
	}

	return true;
}

void WinUtil::playSound(int setting) {
	string sound = SettingsManager::getInstance()->get((SettingsManager::StrSetting) setting);
	if(!sound.empty()) {
		if(sound == "beep")
			::MessageBeep(MB_OK);
		else
			::PlaySound(Text::toT(sound).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
}

void WinUtil::openFile(const tstring& file) {
	::ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void WinUtil::openFolder(const tstring& file) {
	if(File::getSize(Text::fromT(file)) != -1)
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, /select, \"" + (Text::fromT(file))
			+ "\"").c_str(), NULL, SW_SHOWNORMAL);
	else
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, \"" + Util::getFilePath(
			Text::fromT(file)) + "\"").c_str(), NULL, SW_SHOWNORMAL);
}

tstring WinUtil::getNicks(const CID& cid, const string& hintUrl) {
	return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(cid, hintUrl)));
}

tstring WinUtil::getNicks(const UserPtr& u, const string& hintUrl) {
	return getNicks(u->getCID(), hintUrl);
}

tstring WinUtil::getNicks(const CID& cid, const string& hintUrl, bool priv) {
	return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(cid, hintUrl, priv)));
}

static pair<tstring, bool> formatHubNames(const StringList& hubs) {
	if(hubs.empty()) {
		return make_pair(T_("Offline"), false);
	} else {
		return make_pair(Text::toT(Util::toString(hubs)), true);
	}
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid, const string& hintUrl) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid, hintUrl));
}

pair<tstring, bool> WinUtil::getHubNames(const UserPtr& u, const string& hintUrl) {
	return getHubNames(u->getCID(), hintUrl);
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid, const string& hintUrl, bool priv) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid, hintUrl, priv));
}

int WinUtil::getIconIndex(const tstring& aFileName) {
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		string x = Text::toLower(Util::getFileExt(Text::fromT(aFileName)));
		if(!x.empty()) {
			ImageIter j = fileIndexes.find(x);
			if(j != fileIndexes.end())
				return j->second;
		}
		tstring fn = Text::toT(Text::toLower(Util::getFileName(Text::fromT(aFileName))));
		::SHGetFileInfo(fn.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON
			| SHGFI_USEFILEATTRIBUTES);
		if(!fi.hIcon) {
			return 2;
		}
		try {
			dwt::Icon tmp(fi.hIcon);
			fileImages->add(tmp);

			fileIndexes[x] = fileImageCount++;
			return fileImageCount - 1;
		} catch (const dwt::DWTException&) {
			return 2;
		}
	} else {
		return 2;
	}
}

void WinUtil::reducePaths(string& message) {
	string::size_type start = 0;
	while((start = message.find('<', start)) != string::npos) {
		string::size_type end = message.find('>', start);
		if(end == string::npos)
			return;

		if(count(message.begin() + start, message.begin() + end, PATH_SEPARATOR) >= 2) {
			message.erase(end, 1);
			message.erase(start, message.rfind(PATH_SEPARATOR, message.rfind(PATH_SEPARATOR, end) - 1) - start);
			message.insert(start, "...");
		}

		start++;
	}
}

void WinUtil::addHashItems(const dwt::Menu::ObjectType& menu, const TTHValue& tth, const tstring& filename) {
	menu->appendItem(T_("Search for alternates"), std::tr1::bind(&WinUtil::searchHash, tth));
	menu->appendItem(T_("Lookup TTH at Bitzi.com"), std::tr1::bind(WinUtil::bitziLink, tth));
	menu->appendItem(T_("Copy magnet link to clipboard"), std::tr1::bind(&WinUtil::copyMagnet, tth, filename));
}

void WinUtil::bitziLink(const TTHValue& aHash) {
	// to use this free service by bitzi, we must not hammer or request information from bitzi
	// except when the user requests it (a mass lookup isn't acceptable), and (if we ever fetch
	// this data within DC++, we must identify the client/mod in the user agent, so abuse can be
	// tracked down and the code can be fixed
	openLink(_T("http://bitzi.com/lookup/tree:tiger:") + Text::toT(aHash.toBase32()));
}

void WinUtil::copyMagnet(const TTHValue& aHash, const tstring& aFile) {
	if(!aFile.empty()) {
		setClipboard(_T("magnet:?xt=urn:tree:tiger:") + Text::toT(aHash.toBase32()) + _T("&dn=") + Text::toT(
			Util::encodeURI(Text::fromT(aFile))));
	}
}

void WinUtil::searchHash(const TTHValue& aHash) {
	SearchFrame::openWindow(mainWindow->getTabView(), Text::toT(aHash.toBase32()), SearchManager::TYPE_TTH);
}

void WinUtil::addLastDir(const tstring& dir) {
	TStringIter i = find(lastDirs.begin(), lastDirs.end(), dir);
	if(i != lastDirs.end()) {
		lastDirs.erase(i);
	}
	while(lastDirs.size() >= 10) {
		lastDirs.erase(lastDirs.begin());
	}
	lastDirs.push_back(dir);
}

bool WinUtil::browseSaveFile(dwt::Widget* parent, tstring& file) {
	tstring ext = Util::getFileExt(file);
	tstring path = Util::getFilePath(file);

	SaveDialog dlg(parent);

	if(!ext.empty()) {
		ext = ext.substr(1); // remove leading dot so default extension works when browsing for file
		dlg.addFilter(str(TF_("%1% files") % ext), _T("*.") + ext);
		dlg.setDefaultExtension(ext);
	}
	dlg.addFilter(T_("All files"), _T("*.*"));
	dlg.setInitialDirectory(path);

	return dlg.open(file);
}

bool WinUtil::browseFileList(dwt::Widget* parent, tstring& file) {
	return LoadDialog(parent).addFilter(T_("File Lists"), _T("*.xml.bz2;*.xml")) .addFilter(T_("All files"), _T("*.*")) .setInitialDirectory(
		Text::toT(Util::getListPath())) .open(file);
}

void WinUtil::setClipboard(const tstring& str) {
	if(!mainWindow)
		return;

	if(!::OpenClipboard(mainWindow->handle())) {
		return;
	}

	EmptyClipboard();

	// Allocate a global memory object for the text.
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(TCHAR));
	if(hglbCopy == NULL) {
		CloseClipboard();
		return;
	}

	// Lock the handle and copy the text to the buffer.
	TCHAR* lptstrCopy = (TCHAR*) GlobalLock(hglbCopy);
	_tcscpy(lptstrCopy, str.c_str());
	GlobalUnlock(hglbCopy);

	// Place the handle on the clipboard.
#ifdef UNICODE
	SetClipboardData(CF_UNICODETEXT, hglbCopy);
#else
	SetClipboardData(CF_TEXT, hglbCopy);
#endif

	CloseClipboard();
}

bool WinUtil::getUCParams(dwt::Widget* parent, const UserCommand& uc, StringMap& sm) throw () {
	ParamDlg dlg(parent, Text::toT(Util::toString(" > ", uc.getDisplayName())));

	string::size_type i = 0;
	StringList names;

	while((i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j - i);
		if(find(names.begin(), names.end(), name) == names.end()) {
			string caption = name;
			if(uc.adc()) {
				Util::replace("\\\\", "\\", caption);
				Util::replace("\\s", " ", caption);
			}

			dlg.addTextBox(Text::toT(caption), Text::toT(sm["line:" + name]));
			names.push_back(name);
		}
		i = j + 1;
	}

	if(names.empty())
		return true;

	if(dlg.run() == IDOK) {
		const TStringList& values = dlg.getValues();
		for(size_t i = 0, iend = values.size(); i < iend; ++i) {
			sm["line:" + names[i]] = Text::fromT(values[i]);
		}
		return true;
	}
	return false;
}

class HelpPopup: public Container {
	typedef Container BaseType;

public:
	explicit HelpPopup(dwt::Control* parent, const tstring& text_) :
		BaseType(parent, dwt::NormalDispatcher::newClass<HelpPopup>(0, 0, dwt::Dispatcher::getDefaultCursor(),
			reinterpret_cast<HBRUSH> (COLOR_INFOBK + 1))), text(text_)
	{
		// where to position the tooltip
		dwt::Point pt;
		if(isKeyPressed(VK_F1)) {
			dwt::Rectangle rect = parent->getWindowRect();
			pt.x = rect.left() + rect.width() / 2;
			pt.y = rect.top();
		} else {
			pt = dwt::Point::fromLParam(::GetMessagePos());
		}

		// create the popup container (invisible at first)
		Seed cs(WS_POPUP | WS_BORDER, WS_EX_CLIENTEDGE);
		cs.location = dwt::Rectangle(pt, dwt::Point()); // set the position but not the size
		create(cs);
		onLeftMouseDown(std::tr1::bind(&HelpPopup::terminate, this));
		onKeyDown(std::tr1::bind(&HelpPopup::terminate, this));
		onHelp(std::tr1::bind(&HelpPopup::handleHelp, this));

		// create the inner text control
		ts = WinUtil::Seeds::richTextBox;
		ts.style = WS_CHILD | WS_VISIBLE | ES_READONLY;
		ts.exStyle = 0;
		ts.location = dwt::Rectangle(margins, dwt::Point(maxWidth, 0));
		ts.foregroundColor = ::GetSysColor(COLOR_INFOTEXT);
		ts.backgroundColor = ::GetSysColor(COLOR_INFOBK);
		createBox();
	}

private:
	void createBox() {
		box = addChild(ts);

		// let the control figure out what the best size is
		box->onRaw(std::tr1::bind(&HelpPopup::resize, this, _2), dwt::Message(WM_NOTIFY, EN_REQUESTRESIZE));
		box->sendMessage(EM_SETEVENTMASK, 0, ENM_REQUESTRESIZE); ///@todo move to dwt
		box->setText(text);
	}

	void multilineBox() {
		// can't add ES_MULTILINE at run time, so we create the control again
		::DestroyWindow(box->handle());
		ts.style |= ES_MULTILINE;
		createBox();
	}

	LRESULT resize(LPARAM lParam) {
		{
			dwt::Rectangle rect(reinterpret_cast<REQRESIZE*> (lParam)->rc);
			if(rect.width() > maxWidth && (ts.style & ES_MULTILINE) != ES_MULTILINE) {
				callAsync(std::tr1::bind(&HelpPopup::multilineBox, this));
				return 0;
			}

			box->layout(rect);
		}

		// now that the text control is correctly sized, resize the container window
		dwt::Rectangle rect = box->getWindowRect();
		rect.pos -= margins;
		rect.size += margins + margins;
		rect.size.x += ::GetSystemMetrics(SM_CXEDGE) * 2;
		rect.size.y += ::GetSystemMetrics(SM_CYEDGE) * 2;

		// make sure the window fits in within the screen
		dwt::Point pt = getDesktopSize();
		if(rect.right() > pt.x)
			rect.pos.x -= rect.size.x;
		if(rect.bottom() > pt.y)
			rect.pos.y -= rect.size.y;

		layout(rect);

		// go live!
		setVisible(true);
		::SetCapture(handle());

		return 0;
	}

	bool terminate() {
		::ReleaseCapture();
		close(true);
		return true;
	}

	void handleHelp() {
		// someone pressed F1 while the popup was shown... do nothing.
	}

	static const dwt::Point margins;
	static const long maxWidth = 400;

	RichTextBoxPtr box;
	RichTextBox::Seed ts;
	tstring text;
};
const dwt::Point HelpPopup::margins(6, 6);

void WinUtil::help(dwt::Control* widget, unsigned id) {
	if(id >= IDH_CSHELP_BEGIN && id <= IDH_CSHELP_END) {
		// context-sensitive help
		new HelpPopup(widget, Text::toT(getHelpText(id)));
	} else {
		if(id < IDH_BEGIN || id > IDH_END)
			id = IDH_INDEX;
		::HtmlHelp(widget->handle(), helpPath.c_str(), HH_HELP_CONTEXT, id);
	}
}

string WinUtil::getHelpText(unsigned id) {
	if(id < IDH_CSHELP_BEGIN)
		return Util::emptyString;
	id -= IDH_CSHELP_BEGIN;

	if(id >= helpTexts.size())
		return Util::emptyString;

	return helpTexts[id];
}

void WinUtil::toInts(const string& str, std::vector<int>& array) {
	StringTokenizer<string> t(str, _T(','));
	StringList& l = t.getTokens();

	for(size_t i = 0; i < l.size() && i < array.size(); ++i) {
		array[i] = Util::toInt(l[i]);
	}
}

pair<ButtonPtr, ButtonPtr> WinUtil::addDlgButtons(GridPtr grid, const dwt::Application::Callback& f_ok,
	const dwt::Application::Callback& f_cancel)
{
	Button::Seed seed;

	seed.caption = T_("OK");
	seed.menuHandle = reinterpret_cast<HMENU> (IDOK);
	seed.padding.x = 20;
	ButtonPtr ok = grid->addChild(seed);
	ok->setHelpId(IDH_DCPP_OK);
	ok->onClicked(f_ok);

	seed.caption = T_("Cancel");
	seed.menuHandle = reinterpret_cast<HMENU> (IDCANCEL);
	seed.padding.x = 10;
	ButtonPtr cancel = grid->addChild(seed);
	cancel->setHelpId(IDH_DCPP_CANCEL);
	cancel->onClicked(f_cancel);

	return make_pair(ok, cancel);
}

HLSCOLOR RGB2HLS(COLORREF rgb) {
	unsigned char minval = min(GetRValue(rgb), min(GetGValue(rgb), GetBValue(rgb)));
	unsigned char maxval = max(GetRValue(rgb), max(GetGValue(rgb), GetBValue(rgb)));
	float mdiff = float(maxval) - float(minval);
	float msum = float(maxval) + float(minval);

	float luminance = msum / 510.0f;
	float saturation = 0.0f;
	float hue = 0.0f;

	if(maxval != minval) {
		float rnorm = (maxval - GetRValue(rgb)) / mdiff;
		float gnorm = (maxval - GetGValue(rgb)) / mdiff;
		float bnorm = (maxval - GetBValue(rgb)) / mdiff;

		saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

		if(GetRValue(rgb) == maxval)
			hue = 60.0f * (6.0f + bnorm - gnorm);
		if(GetGValue(rgb) == maxval)
			hue = 60.0f * (2.0f + rnorm - bnorm);
		if(GetBValue(rgb) == maxval)
			hue = 60.0f * (4.0f + gnorm - rnorm);
		if(hue > 360.0f)
			hue = hue - 360.0f;
	}
	return HLS ((hue*255)/360, luminance*255, saturation*255);
}

static inline BYTE _ToRGB(float rm1, float rm2, float rh) {
	if(rh > 360.0f)
		rh -= 360.0f;
	else if(rh < 0.0f)
		rh += 360.0f;

	if(rh < 60.0f)
		rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;
	else if(rh < 180.0f)
		rm1 = rm2;
	else if(rh < 240.0f)
		rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;

	return (BYTE) (rm1 * 255);
}

COLORREF HLS2RGB(HLSCOLOR hls) {
	float hue = ((int) HLS_H(hls) * 360) / 255.0f;
	float luminance = HLS_L(hls) / 255.0f;
	float saturation = HLS_S(hls) / 255.0f;

	if(saturation == 0.0f) {
		return RGB (HLS_L(hls), HLS_L(hls), HLS_L(hls));
	}
	float rm1, rm2;

	if(luminance <= 0.5f)
		rm2 = luminance + luminance * saturation;
	else
		rm2 = luminance + saturation - luminance * saturation;
	rm1 = 2.0f * luminance - rm2;
	BYTE red = _ToRGB(rm1, rm2, hue + 120.0f);
	BYTE green = _ToRGB(rm1, rm2, hue);
	BYTE blue = _ToRGB(rm1, rm2, hue - 120.0f);

	return RGB (red, green, blue);
}

COLORREF HLS_TRANSFORM(COLORREF rgb, int percent_L, int percent_S) {
	HLSCOLOR hls = RGB2HLS(rgb);
	BYTE h = HLS_H(hls);
	BYTE l = HLS_L(hls);
	BYTE s = HLS_S(hls);

	if(percent_L > 0) {
		l = BYTE(l + ((255 - l) * percent_L) / 100);
	} else if(percent_L < 0) {
		l = BYTE((l * (100 + percent_L)) / 100);
	}
	if(percent_S > 0) {
		s = BYTE(s + ((255 - s) * percent_S) / 100);
	} else if(percent_S < 0) {
		s = BYTE((s * (100 + percent_S)) / 100);
	}
	return HLS2RGB(HLS(h, l, s));
}

bool registerHandler_(const tstring& name) {
	HKEY hk;
	TCHAR Buf[512];
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name + _T("\\Shell\\Open\\Command")).c_str(),
		0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS)
	{
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE) Buf, &bufLen);
		::RegCloseKey(hk);
	}

	tstring app = _T("\"") + Text::toT(WinUtil::getAppName()) + _T("\" %1");
	if(Util::stricmp(app.c_str(), Buf) == 0) {
		// already registered to us
		return true;
	}

	if(::RegCreateKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name).c_str(),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
		return false;

	TCHAR tmp[] = _T("URL:Direct Connect Protocol");
	::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE) tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
	::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE) _T(""), sizeof(TCHAR));
	::RegCloseKey(hk);

	if(::RegCreateKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name + _T("\\Shell\\Open\\Command")).c_str(),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
		return false;
	bool ret = ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE) app.c_str(), sizeof(TCHAR) * (app.length() + 1)) == ERROR_SUCCESS;
	::RegCloseKey(hk);

	if(::RegCreateKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name + _T("\\DefaultIcon")).c_str(),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS)
	{
		app = Text::toT(WinUtil::getAppName());
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE) app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);
	}

	return ret;
}

bool registerHandler(const tstring& name) {
	bool ret = registerHandler_(name);
	if(!ret)
		LogManager::getInstance()->message(str(F_("Error registering %1%:// link handler") % Text::fromT(name)));
	return ret;
}

void WinUtil::registerHubHandlers() {
	if(BOOLSETTING(URL_HANDLER)) {
		if(!urlDcADCRegistered) {
			urlDcADCRegistered = registerHandler(_T("dchub")) && registerHandler(_T("adc")) && registerHandler(_T("adcs"));
		}
	} else if(urlDcADCRegistered) {
		urlDcADCRegistered = !(
		(::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\dchub")) == ERROR_SUCCESS) &&
		(::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\adc")) == ERROR_SUCCESS) &&
		(::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\adcs")) == ERROR_SUCCESS));
	}
}

void WinUtil::registerMagnetHandler() {
	if(BOOLSETTING(MAGNET_REGISTER)) {
		if(!urlMagnetRegistered) {
			urlMagnetRegistered = registerHandler(_T("magnet"));
		}
	} else if(urlMagnetRegistered) {
		urlMagnetRegistered = ::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\magnet")) != ERROR_SUCCESS;
	}
}

void WinUtil::openLink(const tstring& url) {
	::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

bool WinUtil::parseDBLClick(const tstring& aString) {
	if((Util::strnicmp(aString.c_str(), _T("http://"), 7) == 0)
		|| (Util::strnicmp(aString.c_str(), _T("www."), 4) == 0) || (Util::strnicmp(aString.c_str(), _T("ftp://"), 6)
		== 0) || (Util::strnicmp(aString.c_str(), _T("irc://"), 6) == 0) || (Util::strnicmp(aString.c_str(),
		_T("https://"), 8) == 0) || (Util::strnicmp(aString.c_str(), _T("mailto:"), 7) == 0))
	{

		openLink(aString);
		return true;
	} else if(Util::strnicmp(aString.c_str(), _T("dchub://"), 8) == 0) {
		parseDchubUrl(aString);
		return true;
	} else if(Util::strnicmp(aString.c_str(), _T("magnet:?"), 8) == 0) {
		parseMagnetUri(aString);
		return true;
	} else if(Util::strnicmp(aString.c_str(), _T("adc://"), 6) == 0) {
		parseADChubUrl(aString, false);
		return true;
	} else if(Util::strnicmp(aString.c_str(), _T("adcs://"), 7) == 0) {
		parseADChubUrl(aString, true);
		return true;
	}
	return false;
}

void WinUtil::parseDchubUrl(const tstring& aUrl) {
	string server, file;
	uint16_t port = 411;
	Util::decodeUrl(Text::fromT(aUrl), server, port, file);
	string url = server + ":" + Util::toString(port);
	if(!server.empty()) {
		HubFrame::openWindow(mainWindow->getTabView(), url);
	}
	if(!file.empty()) {
		if(file[0] == '/') {
			// Remove any '/' in from of the file
			file = file.substr(1);
			if(file.empty()) return;
		}	
		try {
			UserPtr user = ClientManager::getInstance()->findLegacyUser(file);
			if(user)
				QueueManager::getInstance()->addList(HintedUser(user, url), QueueItem::FLAG_CLIENT_VIEW);
			// @todo else report error
		} catch (const Exception&) {
			// ...
		}
	}
}

void WinUtil::parseADChubUrl(const tstring& aUrl, bool isSecure) {
	string server, file;
	uint16_t port = 0; //make sure we get a port since adc doesn't have a standard one
	Util::decodeUrl(Text::fromT(aUrl), server, port, file);
	if(!server.empty() && port > 0) {
		HubFrame::openWindow(mainWindow->getTabView(), string(isSecure ? "adcs" : "adc") + "://" + server + ":" + Util::toString(port));
	}
}

void WinUtil::parseMagnetUri(const tstring& aUrl, bool /*aOverride*/) {
	// official types that are of interest to us
	//  xt = exact topic
	//  xs = exact substitute
	//  as = acceptable substitute
	//  dn = display name
	if(Util::strnicmp(aUrl.c_str(), _T("magnet:?"), 8) == 0) {
		LogManager::getInstance()->message(str(F_("MAGNET Link detected: %1%") % Text::fromT(aUrl)));
		StringTokenizer<tstring> mag(aUrl.substr(8), _T('&'));
		typedef map<tstring, tstring> MagMap;
		MagMap hashes;
		tstring fname, fhash, type, param;
		for(TStringList::iterator idx = mag.getTokens().begin(); idx != mag.getTokens().end(); ++idx) {
			// break into pairs
			string::size_type pos = idx->find(_T('='));
			if(pos != string::npos) {
				type = Text::toT(Text::toLower(Util::encodeURI(Text::fromT(idx->substr(0, pos)), true)));
				param = Text::toT(Util::encodeURI(Text::fromT(idx->substr(pos + 1)), true));
			} else {
				type = Text::toT(Util::encodeURI(Text::fromT(*idx), true));
				param.clear();
			}
			// extract what is of value
			if(param.length() == 85 && Util::strnicmp(param.c_str(), _T("urn:bitprint:"), 13) == 0) {
				hashes[type] = param.substr(46);
			} else if(param.length() == 54 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger:"), 15) == 0) {
				hashes[type] = param.substr(15);
			} else if(param.length() == 55 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger/:"), 16) == 0) {
				hashes[type] = param.substr(16);
			} else if(param.length() == 59 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger/1024:"), 20) == 0) {
				hashes[type] = param.substr(20);
			} else if(type.length() == 2 && Util::strnicmp(type.c_str(), _T("dn"), 2) == 0) {
				fname = param;
			}
		}
		// pick the most authoritative hash out of all of them.
		if(hashes.find(_T("as")) != hashes.end()) {
			fhash = hashes[_T("as")];
		}
		if(hashes.find(_T("xs")) != hashes.end()) {
			fhash = hashes[_T("xs")];
		}
		if(hashes.find(_T("xt")) != hashes.end()) {
			fhash = hashes[_T("xt")];
		}
		if(!fhash.empty()) {
			// ok, we have a hash, and maybe a filename.
			//if(!BOOLSETTING(MAGNET_ASK)) {
			//	switch(SETTING(MAGNET_ACTION)) {
			//		case SettingsManager::MAGNET_AUTO_DOWNLOAD:
			//			break;
			//		case SettingsManager::MAGNET_AUTO_SEARCH:
			//			SearchFrame::openWindow(mainWindow->getTabView(), fhash, SearchManager::TYPE_TTH);
			//			break;
			//	};
			//} else {
			// use aOverride to force the display of the dialog.  used for auto-updating
			MagnetDlg(mainWindow, fhash, fname).run();
			//}
		} else {
			dwt::MessageBox(mainWindow).show(
				T_("A MAGNET link was given to DC++, but it didn't contain a valid file hash for use on the Direct Connect network.  No action will be taken."),
				T_("MAGNET Link detected"), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		}
	}
}

typedef std::tr1::function<void(const HintedUser&, const string&)> UserFunction;

static void eachUser(const HintedUserList& list, const StringList& dirs, const UserFunction& f) {
	size_t j = 0;
	for(HintedUserList::const_iterator i = list.begin(), iend = list.end(); i != iend; ++i) {
		try {
			f(*i, (j < dirs.size()) ? dirs[j] : string());
		} catch (const Exception& e) {
			LogManager::getInstance()->message(e.getError());
		}
		j++;
	}
}

static void addUsers(MenuPtr menu, const tstring& text, const HintedUserList& users, const UserFunction& f,
	const dwt::IconPtr& icon = dwt::IconPtr(), const StringList& dirs = StringList())
{
	if(users.empty())
		return;

	if(users.size() > 1) {
		menu = menu->appendPopup(text, icon);
		menu->appendItem(T_("All"), std::tr1::bind(&eachUser, users, dirs, f));

		menu->appendSeparator();

		for(size_t i = 0, iend = users.size(); i < iend; ++i) {
			menu->appendItem(WinUtil::getNicks(users[i]), std::tr1::bind(&eachUser, HintedUserList(1, users[i]),
				StringList(1, (i < dirs.size()) ? dirs[i] : string()), f));
		}
	} else {
		menu->appendItem(text, std::tr1::bind(&eachUser, users, dirs, f), icon);
	}
}

template<typename F>
HintedUserList filter(const HintedUserList& l, F f) {
	HintedUserList ret;
	for(HintedUserList::const_iterator i = l.begin(), iend = l.end(); i != iend; ++i) {
		if(f(i->user)) {
			ret.push_back(*i);
		}
	}
	return ret;
}

static bool isAdc(const UserPtr& u) {
	return !u->isSet(User::NMDC);
}
static bool isFav(const UserPtr& u) {
	return !FavoriteManager::getInstance()->isFavoriteUser(u);
}

void WinUtil::addUserItems(MenuPtr menu, const HintedUserList& users, dwt::TabViewPtr parent, const StringList& dirs) {
	QueueManager* qm = QueueManager::getInstance();

	addUsers(menu, T_("&Get file list"), users, std::tr1::bind(&QueueManager::addList, qm, _1,
		QueueItem::FLAG_CLIENT_VIEW, _2), dwt::IconPtr(), dirs);

	addUsers(menu, T_("&Browse file list"), filter(users, &isAdc), std::tr1::bind(&QueueManager::addList, qm, _1,
		QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST, _2), dwt::IconPtr(), dirs);

	addUsers(menu, T_("&Match queue"), users, std::tr1::bind(&QueueManager::addList, qm, _1,
		QueueItem::FLAG_MATCH_QUEUE, Util::emptyString));

	addUsers(menu, T_("&Send private message"), users, std::tr1::bind(&PrivateFrame::openWindow, parent, _1,
		Util::emptyStringT, Util::emptyString));

	addUsers(menu, T_("Add To &Favorites"), filter(users, &isFav), std::tr1::bind(&FavoriteManager::addFavoriteUser,
		FavoriteManager::getInstance(), _1), dwt::IconPtr(new dwt::Icon(IDI_FAVORITE_USERS)));

	addUsers(menu, T_("Grant &extra slot"), users, std::tr1::bind(&UploadManager::reserveSlot,
		UploadManager::getInstance(), _1));

	typedef void (QueueManager::*qmp)(const UserPtr&, int);
	addUsers(menu, T_("Remove user from queue"), users, std::tr1::bind((qmp) &QueueManager::removeSource, qm, _1,
		(int) QueueItem::Source::FLAG_REMOVED));
}

void WinUtil::makeColumns(dwt::TablePtr table, const ColumnInfo* columnInfo, size_t columnCount, const string& order,
	const string& widths)
{
	std::vector<tstring> n(columnCount);
	std::vector<int> o(columnCount);
	std::vector<int> w(columnCount);
	std::vector<bool> a(columnCount);

	for(size_t i = 0; i < columnCount; ++i) {
		n[i] = T_(columnInfo[i].name);
		o[i] = i;
		w[i] = columnInfo[i].size;
		a[i] = columnInfo[i].rightAlign;
	}
	toInts(order, o);
	toInts(widths, w);
	table->createColumns(n, w, a, o);
}

dwt::IconPtr WinUtil::createIcon(unsigned id, long size) {
	return new dwt::Icon(id, dwt::Point(size, size));
}

#ifdef PORT_ME

double WinUtil::toBytes(TCHAR* aSize) {
	double bytes = _tstof(aSize);

	if (_tcsstr(aSize, CT_("PiB"))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CT_("TiB"))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CT_("GiB"))) {
		return bytes * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CT_("MiB"))) {
		return bytes * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CT_("KiB"))) {
		return bytes * 1024.0;
	} else {
		return bytes;
	}
}
#endif
