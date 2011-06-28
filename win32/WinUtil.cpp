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

#include "stdafx.h"

#include "WinUtil.h"

#include "resource.h"
#include <mmsystem.h>

#include <boost/lexical_cast.hpp>

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

#include <dwt/DWTException.h>
#include <dwt/LibraryLoader.h>
#include <dwt/util/GDI.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/SaveDialog.h>

#include "ParamDlg.h"
#include "MagnetDlg.h"
#include "HubFrame.h"
#include "SearchFrame.h"
#include "MainWindow.h"
#include "PrivateFrame.h"

#ifdef HAVE_HTMLHELP_H
#include <htmlhelp.h>

#ifndef _MSC_VER
// HTML Help libs from recent MS SDKs are compiled with /GS, so link in the following.
#ifdef _WIN64
#define HH_GS_CALL __cdecl
#else
#define HH_GS_CALL __fastcall
#endif
extern "C" {
	void HH_GS_CALL __GSHandlerCheck() { }
	void HH_GS_CALL __security_check_cookie(uintptr_t) { }
#ifndef _WIN64
	uintptr_t __security_cookie;
#endif
}
#undef HH_GS_CALL
#endif

#endif

using dwt::Container;
using dwt::Grid;
using dwt::GridInfo;
using dwt::LoadDialog;
using dwt::SaveDialog;
using dwt::Widget;

// def taken from <gettextP.h>
extern "C" const char *_nl_locale_name_default(void);

WinUtil::Notification WinUtil::notifications[NOTIFICATION_LAST] = {
	{ SettingsManager::SOUND_FINISHED_DL, SettingsManager::BALLOON_FINISHED_DL, N_("Download finished"), IDI_DOWNLOAD },
	{ SettingsManager::SOUND_FINISHED_FL, SettingsManager::BALLOON_FINISHED_FL, N_("File list downloaded"), IDI_DIRECTORY },
	{ SettingsManager::SOUND_MAIN_CHAT, SettingsManager::BALLOON_MAIN_CHAT, N_("Main chat message received"), IDI_BALLOON },
	{ SettingsManager::SOUND_PM, SettingsManager::BALLOON_PM, N_("Private message received"), IDI_PRIVATE },
	{ SettingsManager::SOUND_PM_WINDOW, SettingsManager::BALLOON_PM_WINDOW, N_("Private message window opened"), IDI_PRIVATE }
};

tstring WinUtil::tth;
dwt::BrushPtr WinUtil::bgBrush;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
dwt::FontPtr WinUtil::font;
dwt::ImageListPtr WinUtil::fileImages;
dwt::ImageListPtr WinUtil::userImages;
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
const ComboBox::Seed WinUtil::Seeds::Dialog::comboBoxEdit;
const TextBox::Seed WinUtil::Seeds::Dialog::textBox;
const TextBox::Seed WinUtil::Seeds::Dialog::intTextBox;
const RichTextBox::Seed WinUtil::Seeds::Dialog::richTextBox;
const Table::Seed WinUtil::Seeds::Dialog::table;
const Table::Seed WinUtil::Seeds::Dialog::optionsTable;

void WinUtil::init() {

	SettingsManager::getInstance()->setDefault(SettingsManager::BACKGROUND_COLOR, dwt::Color::predefined(COLOR_WINDOW));
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_COLOR, dwt::Color::predefined(COLOR_WINDOWTEXT));

	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	bgBrush = dwt::BrushPtr(new dwt::Brush(bgColor));

	if(SettingsManager::getInstance()->isDefault(SettingsManager::MAIN_FONT)) {
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
		SettingsManager::getInstance()->setDefault(SettingsManager::MAIN_FONT, Text::fromT(encodeFont(metrics.lfMessageFont)));
	}

	{
		LOGFONT lf;
		decodeFont(Text::toT(SETTING(MAIN_FONT)), lf);
		font = dwt::FontPtr(new dwt::Font(lf));
	}

	fileImages = dwt::ImageListPtr(new dwt::ImageList(dwt::Point(16, 16)));

	// get the directory icon (DIR_ICON).
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		::SHFILEINFO info;
		if(::SHGetFileInfo(_T("./"), FILE_ATTRIBUTE_DIRECTORY, &info, sizeof(info),
			SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES) && info.hIcon)
		{
			dwt::Icon icon(info.hIcon);
			fileImages->add(icon);
		}
	}
	if(fileImages->empty()) {
		fileImages->add(*createIcon(IDI_DIRECTORY, 16));
	}

	// create the incomplete directory icon (DIR_ICON_INCOMPLETE).
	{
		dwt::ImageList icons(dwt::Point(16, 16));
		icons.add(*fileImages->getIcon(DIR_ICON));
		icons.add(*createIcon(IDI_EXEC, 16));
		fileImages->add(*dwt::util::merge(icons));
	}

	// add the generic file icon (FILE_ICON_GENERIC).
	fileImages->add(*createIcon(IDI_FILE, 16));

	{
		const dwt::Point size(16, 16);
		userImages = dwt::ImageListPtr(new dwt::ImageList(size));

		const unsigned baseCount = USER_ICON_MOD_START;
		const unsigned modifierCount = USER_ICON_LAST - USER_ICON_MOD_START;

		auto userIcon = [](unsigned id) { return createIcon(id, 16); };
		dwt::IconPtr bases[baseCount] = { userIcon(IDI_USER), userIcon(IDI_USER_AWAY), userIcon(IDI_USER_BOT) };
		dwt::IconPtr modifiers[modifierCount] = { userIcon(IDI_USER_NOCON), userIcon(IDI_USER_NOSLOT), userIcon(IDI_USER_OP) };

		for(size_t iBase = 0; iBase < baseCount; ++iBase) {
			for(size_t i = 0, n = modifierCount * modifierCount; i < n; ++i) {
				dwt::ImageList icons(size);

				icons.add(*bases[iBase]);

				for(size_t iMod = 0; iMod < modifierCount; ++iMod)
					if(i & (1 << iMod))
						icons.add(*modifiers[iMod]);

				userImages->add(*dwt::util::merge(icons));
			}
		}
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
	RichTextBox::Seed& xdRichTextBox = const_cast<RichTextBox::Seed&> (Seeds::Dialog::richTextBox);
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
	xTable.lvStyle |= LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;
	xTable.font = font;

	xtextBox.exStyle = WS_EX_CLIENTEDGE;
	xtextBox.font = font;

	xRichTextBox.exStyle = WS_EX_CLIENTEDGE;
	xRichTextBox.font = font;
	xRichTextBox.foregroundColor = textColor;
	xRichTextBox.backgroundColor = bgColor;

	xTabs.font = font;

	xtreeView.style |= TVS_HASBUTTONS | TVS_LINESATROOT;
	xtreeView.exStyle = WS_EX_CLIENTEDGE;
	xtreeView.font = font;

	xdComboBox.style |= CBS_DROPDOWNLIST;

	xdTextBox.style |= ES_AUTOHSCROLL;

	xdintTextBox = xdTextBox;
	xdintTextBox.style |= ES_NUMBER;

	xdRichTextBox.style |= ES_READONLY | ES_SUNKEN;
	xdRichTextBox.exStyle = WS_EX_CLIENTEDGE;

	xdTable.style |= WS_HSCROLL | WS_VSCROLL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
	xdTable.exStyle = WS_EX_CLIENTEDGE;
	xdTable.lvStyle |= LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;

	xdoptionsTable = xdTable;
	xdoptionsTable.style |= LVS_SINGLESEL | LVS_NOCOLUMNHEADER;
	xdoptionsTable.lvStyle |= LVS_EX_CHECKBOXES;

	xtextBox.menuSeed = Seeds::menu;
	xdTextBox.menuSeed = Seeds::menu;
	xRichTextBox.menuSeed = Seeds::menu;
	xdRichTextBox.menuSeed = Seeds::menu;

	init_helpPath();

	if(!helpPath.empty()) {
		// load up context-sensitive help texts
		try {
			helpTexts = StringTokenizer<string> (File(Util::getFilePath(Text::fromT(helpPath)) + "cshelp.rtf",
				File::READ, File::OPEN).read(), "\r\n").getTokens();
		} catch (const FileException&) {
		}
	}

#ifdef HAVE_HTMLHELP_H
	::HtmlHelp(NULL, NULL, HH_INITIALIZE, reinterpret_cast<DWORD_PTR> (&helpCookie));
#endif
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
#ifdef HAVE_HTMLHELP_H
	::HtmlHelp(NULL, NULL, HH_UNINITIALIZE, helpCookie);
#endif
}

void WinUtil::enableDEP() {
	dwt::LibraryLoader kernel(_T("kernel32.dll"));
	typedef BOOL (WINAPI *SPDP)(DWORD);
	SPDP spdp = (SPDP)kernel.getProcAddress(_T("SetProcessDEPPolicy"));
	if (spdp)
		dcdebug("SetProcessDEPPolicy %s\n", (*spdp)(1)?"succeeded":"failed");
	else
		dcdebug("SetProcessDEPPolicy not present\n");	
}

tstring WinUtil::encodeFont(LOGFONT const& font) {
	tstring res(font.lfFaceName);
	res += _T(',');
	res += Text::toT(Util::toString(static_cast<int>(font.lfHeight / dwt::util::dpiFactor())));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfWeight));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfItalic));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfCharSet));
	return res;
}

std::string WinUtil::toString(const std::vector<int>& tokens) {
	std::string ret;
	auto start = tokens.cbegin();
	for(auto i = start, iend = tokens.cend(); i != iend; ++i) {
		if(i != start)
			ret += ',';
		ret += Util::toString(*i);
	}
	return ret;
}

void WinUtil::decodeFont(const tstring& setting, LOGFONT &dest) {
	StringTokenizer<tstring> st(setting, _T(','));
	TStringList &sl = st.getTokens();

	NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
	dest = metrics.lfMessageFont;

	tstring face;
	if(sl.size() >= 4) {
		face = sl[0];
		dest.lfHeight = Util::toInt(Text::fromT(sl[1])) * dwt::util::dpiFactor();
		dest.lfWeight = Util::toInt(Text::fromT(sl[2]));
		dest.lfItalic = static_cast<BYTE>(Util::toInt(Text::fromT(sl[3])));
		if(sl.size() >= 5) {
			dest.lfCharSet = static_cast<BYTE>(Util::toInt(Text::fromT(sl[4])));
		}
	}

	if(!face.empty()) {
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

void WinUtil::handleDblClicks(dwt::TextBoxBase* box) {
	box->onLeftMouseDblClick([box](const dwt::MouseEvent &me) { return WinUtil::handleBoxDblClick(box, me); });
}

bool WinUtil::handleBoxDblClick(dwt::TextBoxBase* box, const dwt::MouseEvent& ev) {
	return parseDBLClick(box->textUnderCursor(ev.pos));
}

#define LINE2 _T("-- http://dcplusplus.sourceforge.net <DC++ ") _T(VERSIONSTRING) _T(">")
const TCHAR
	*msgs[] = {
		_T("\r\n-- I'm a happy DC++ user. You could be happy too.\r\n") LINE2,
		_T("\r\n-- Neo-...what? Nope...never heard of it...\r\n") LINE2,
		_T("\r\n-- Evolution of species: Ape --> Man\r\n-- Evolution of science: \"The Earth is Flat\" --> \"The Earth is Round\"\r\n-- Evolution of DC: NMDC --> ADC\r\n") LINE2,
		_T("\r\n-- I share, therefore I am.\r\n") LINE2,
		_T("\r\n-- I came, I searched, I found...\r\n") LINE2,
		_T("\r\n-- I came, I shared, I sent...\r\n") LINE2,
		_T("\r\n-- I can fully configure the toolbar, can you?\r\n") LINE2,
		_T("\r\n-- I don't have to see any ads, do you?\r\n") LINE2,
		_T("\r\n-- My client supports passive-passive downloads, does yours?\r\n") LINE2,
		_T("\r\n-- I can refresh my opened file lists, can you?\r\n") LINE2,
		_T("\r\n-- I can get help for every part of GUI and settings in my client, can you?\r\n") LINE2,
		_T("\r\n-- My client automatically detects the connection, does yours?\r\n") LINE2,
		_T("\r\n-- My client keeps track of all the recent opened windows, does yours?\r\n") LINE2,
		_T("\r\n-- These addies are pretty annoying, aren't they? Get revenge by sending them yourself!\r\n") LINE2,
		_T("\r\n-- My client supports taskbar thumbnails and Aero Peek previews, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports secure communication and transfers, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports grouping favorite hubs, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports segmented downloading, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports browsing file lists, does yours?\r\n") LINE2 };

#define MSGS 19

tstring
	WinUtil::commands =
		_T("/refresh, /me <msg>, /clear [lines to keep], /slots #, /dslots #, /search <string>, /f <string>, /dc++, /away <msg>, /back, /g <searchstring>, /imdb <imdbquery>, /u <url>, /rebuild, /ts, /download, /upload");

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
			ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::SLOTS), j);
			status = T_("Slots set");
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
		auto value = Util::toInt(Text::fromT(param));
		ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::MAX_UPLOAD_SPEED_MAIN), value);
		status = value ? str(TF_("Upload limit set to %1% KiB/s") % value) : T_("Upload limit disabled");
	} else if(Util::stricmp(cmd.c_str(), _T("download")) == 0) {
		auto value = Util::toInt(Text::fromT(param));
		ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::MAX_DOWNLOAD_SPEED_MAIN), value);
		status = value ? str(TF_("Download limit set to %1% KiB/s") % value) : T_("Download limit disabled");
	} else {
		return false;
	}

	return true;
}

void WinUtil::notify(NotificationType notification, const tstring& balloonText, const std::function<void ()>& balloonCallback) {
	const auto& n = notifications[notification];

	const string& s = SettingsManager::getInstance()->get((SettingsManager::StrSetting)n.sound);
	if(!s.empty()) {
		playSound(Text::toT(s));
	}

	int b = SettingsManager::getInstance()->get((SettingsManager::IntSetting)n.balloon);
	if(b == SettingsManager::BALLOON_ALWAYS || (b == SettingsManager::BALLOON_BACKGROUND && !mainWindow->onForeground())) {
		mainWindow->notify(T_(n.title), balloonText, balloonCallback, createIcon(n.icon, 16));
	}
}

void WinUtil::playSound(const tstring& sound) {
	if(sound == _T("beep")) {
		::MessageBeep(MB_OK);
	} else {
		::PlaySound(sound.c_str(), 0, SND_FILENAME | SND_ASYNC);
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

size_t WinUtil::getFileIcon(const string& fileName) {
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		string ext = Text::toLower(Util::getFileExt(fileName));
		if(!ext.empty()) {
			auto index = fileIndexes.find(ext);
			if(index != fileIndexes.end())
				return index->second;
		}

		::SHFILEINFO info;
		if(::SHGetFileInfo(Text::toT(Text::toLower(Util::getFileName(fileName))).c_str(), FILE_ATTRIBUTE_NORMAL,
			&info, sizeof(info), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES) && info.hIcon)
		{
			size_t ret = fileImages->size();
			fileIndexes[ext] = ret;

			dwt::Icon icon(info.hIcon);
			fileImages->add(icon);

			return ret;
		}
	}

	return FILE_ICON_GENERIC;
}

size_t WinUtil::getFileIcon(const tstring& fileName) {
	return getFileIcon(Text::fromT(fileName));
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

void WinUtil::addHashItems(const dwt::Menu::ObjectType& menu, const TTHValue& tth, const tstring& filename, int64_t size) {
	menu->appendItem(T_("Search for alternates"), [=] { searchHash(tth); }, menuIcon(IDI_SEARCH));
	menu->appendItem(T_("Lookup TTH at Bitzi.com"), [=] { bitziLink(tth); });
	menu->appendItem(T_("Copy magnet link to clipboard"), [=] { copyMagnet(tth, filename, size); }, menuIcon(IDI_MAGNET));
}

void WinUtil::bitziLink(const TTHValue& aHash) {
	// to use this free service by bitzi, we must not hammer or request information from bitzi
	// except when the user requests it (a mass lookup isn't acceptable), and (if we ever fetch
	// this data within DC++, we must identify the client/mod in the user agent, so abuse can be
	// tracked down and the code can be fixed
	openLink(_T("http://bitzi.com/lookup/tree:tiger:") + Text::toT(aHash.toBase32()));
}

void WinUtil::copyMagnet(const TTHValue& aHash, const tstring& aFile, int64_t size) {
	if(!aFile.empty()) {
		setClipboard(Text::toT(makeMagnet(aHash, Text::fromT(aFile), size)));
	}
}

string WinUtil::makeMagnet(const TTHValue& aHash, const string& aFile, int64_t size) {
	string ret = "magnet:?xt=urn:tree:tiger:" + aHash.toBase32();
	if(size > 0)
		ret += "&xl=" + Util::toString(size);
	return ret + "&dn=" + Util::encodeURI(aFile);
}

void WinUtil::searchAny(const tstring& aSearch) {
	SearchFrame::openWindow(mainWindow->getTabView(), aSearch, SearchManager::TYPE_ANY);
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

bool WinUtil::getUCParams(dwt::Widget* parent, const UserCommand& uc, StringMap& sm) noexcept {
	ParamDlg dlg(parent, Text::toT(Util::toString(" > ", uc.getDisplayName())));

	string::size_type i = 0;
	StringList names;

	while((i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		const string name = uc.getCommand().substr(i, j - i);
		if(find(names.begin(), names.end(), name) == names.end()) {
			tstring caption = Text::toT(name);
			if(uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), caption);
				Util::replace(_T("\\s"), _T(" "), caption);
			}

			// let's break between slashes (while ignoring double-slashes) to see if it's a combo
			int combo_sel = -1;
			tstring combo_caption = caption;
			Util::replace(_T("//"), _T("\t"), combo_caption);
			TStringList combo_values = StringTokenizer<tstring>(combo_caption, _T('/')).getTokens();
			if(combo_values.size() > 2) { // must contain at least: caption, default sel, 1 value

				TStringIter first = combo_values.begin();
				combo_caption = *first;
				combo_values.erase(first);

				first = combo_values.begin();
				try { combo_sel = boost::lexical_cast<size_t>(Text::fromT(*first)); }
				catch(const boost::bad_lexical_cast&) { combo_sel = -1; }
				combo_values.erase(first);
				if(static_cast<size_t>(combo_sel) >= combo_values.size())
					combo_sel = -1; // default selection value too high
			}

			if(combo_sel >= 0) {
				for(TStringIter i = combo_values.begin(), iend = combo_values.end(); i != iend; ++i)
					Util::replace(_T("\t"), _T("/"), *i);

				// if the combo has already been displayed before, retrieve the prev value and bypass combo_sel
				TStringIterC prev = find(combo_values.begin(), combo_values.end(), Text::toT(sm["line:" + name]));
				if(prev != combo_values.end())
					combo_sel = prev - combo_values.begin();

				dlg.addComboBox(combo_caption, combo_values, combo_sel);

			} else {
				dlg.addTextBox(caption, Text::toT(sm["line:" + name]));
			}

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
		onLeftMouseDown([this](const dwt::MouseEvent&) { return close(); });
		onKeyDown([this](int) { return close(); });
		onHelp([this](Widget*, unsigned) { handleHelp(); });

		// create the inner text control
		ts = WinUtil::Seeds::richTextBox;
		ts.style = WS_CHILD | WS_VISIBLE | ES_READONLY;
		ts.exStyle = 0;
		ts.location = dwt::Rectangle(margins, dwt::Point(maxWidth, 0));
		ts.foregroundColor = dwt::Color::predefined(COLOR_INFOTEXT);
		ts.backgroundColor = dwt::Color::predefined(COLOR_INFOBK);
		createBox();
	}

private:
	void createBox() {
		box = addChild(ts);

		// let the control figure out what the best size is
		box->onRaw([this](WPARAM, LPARAM l) { return resize(l); }, dwt::Message(WM_NOTIFY, EN_REQUESTRESIZE));
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
				callAsync([this] { multilineBox(); });
				return 0;
			}

			box->resize(rect);
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

		BaseType::resize(rect);

		// go live!
		setVisible(true);
		::SetCapture(handle());

		return 0;
	}

	bool close() {
		::ReleaseCapture();
		BaseType::close(true);
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
#ifdef HAVE_HTMLHELP_H
		if(id < IDH_BEGIN || id > IDH_END)
			id = IDH_INDEX;
		::HtmlHelp(widget->handle(), helpPath.c_str(), HH_HELP_CONTEXT, id);
#endif
	}
}

string WinUtil::getHelpText(unsigned id) {
	if(id >= IDH_CSHELP_BEGIN) {
		id -= IDH_CSHELP_BEGIN;

		if(id < helpTexts.size()) {
			const auto& ret = helpTexts[id];
			if(!ret.empty()) {
				return ret;
			}
		}
	}

	return _("No help information available");
}

void WinUtil::toInts(const string& str, std::vector<int>& array) {
	StringTokenizer<string> t(str, _T(','));
	StringList& l = t.getTokens();

	for(size_t i = 0; i < l.size() && i < array.size(); ++i) {
		array[i] = Util::toInt(l[i]);
	}
}

pair<ButtonPtr, ButtonPtr> WinUtil::addDlgButtons(GridPtr grid)
{
	Button::Seed seed;

	seed.caption = T_("OK");
	seed.menuHandle = reinterpret_cast<HMENU> (IDOK);
	seed.padding.x = 20;
	ButtonPtr ok = grid->addChild(seed);
	ok->setHelpId(IDH_DCPP_OK);
	ok->setImage(buttonIcon(IDI_OK));

	seed.caption = T_("Cancel");
	seed.menuHandle = reinterpret_cast<HMENU> (IDCANCEL);
	seed.padding.x = 10;
	ButtonPtr cancel = grid->addChild(seed);
	cancel->setHelpId(IDH_DCPP_CANCEL);
	cancel->setImage(buttonIcon(IDI_CANCEL));

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

bool WinUtil::parseDBLClick(const tstring& str) {
	auto url = Text::fromT(str);
	string proto, host, file, query, fragment;
	uint16_t port;
	Util::decodeUrl(url, proto, host, port, file, query, fragment);

	if(Util::stricmp(proto.c_str(), "adc") == 0 ||
		Util::stricmp(proto.c_str(), "adcs") == 0 ||
		Util::stricmp(proto.c_str(), "dchub") == 0 )
	{
		if(!host.empty()) {
			HubFrame::openWindow(mainWindow->getTabView(), url);
		}

		if(!file.empty()) {
			if(file[0] == '/') {
				// Remove any '/' in from of the file
				file = file.substr(1);
				if(file.empty()) return true;
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

		return true;
	} else if(!proto.empty() ||
		Util::strnicmp(str.c_str(), _T("www."), 4) == 0 ||
		Util::strnicmp(str.c_str(), _T("mailto:"), 7) == 0) {
		openLink(str);
		return true;
	} else if(host == "magnet") {
		parseMagnetUri(str);
		return true;
	}

	return false;
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
		tstring fname, fhash, type, param, fkey;
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
			} else if(type.length() == 2 && Util::strnicmp(type.c_str(), _T("kt"), 2) == 0) {
				fkey = param;
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
		if(!fhash.empty() || !fkey.empty()) {
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
			MagnetDlg(mainWindow, fhash, fname, fkey).run();
			//}
		} else {
			dwt::MessageBox(mainWindow).show(
				T_("A MAGNET link was given to DC++, but it didn't contain a valid file hash for use on the Direct Connect network.  No action will be taken."),
				T_("MAGNET Link detected"), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		}
	}
}

typedef std::function<void(const HintedUser&, const string&)> UserFunction;

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
		menu->appendItem(T_("All"), [=] { eachUser(users, dirs, f); });

		menu->appendSeparator();

		for(size_t i = 0, iend = users.size(); i < iend; ++i) {
			menu->appendItem(WinUtil::getNicks(users[i]), [=] { eachUser(HintedUserList(1, users[i]),
				StringList(1, (i < dirs.size()) ? dirs[i] : Util::emptyString), f); });
		}
	} else {
		menu->appendItem(text, [=] { eachUser(users, dirs, f); }, icon);
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

void WinUtil::addUserItems(MenuPtr menu, const HintedUserList& users, TabViewPtr parent, const StringList& dirs) {
	QueueManager* qm = QueueManager::getInstance();

	addUsers(menu, T_("&Get file list"), users, [=](const HintedUser &u, const string& s) {
		qm->addList(u, QueueItem::FLAG_CLIENT_VIEW, s); }, dwt::IconPtr(), dirs);

	addUsers(menu, T_("&Browse file list"), filter(users, &isAdc), [=](const HintedUser &u, const string& s) {
		qm->addList(u, QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST, s); }, dwt::IconPtr(), dirs);

	addUsers(menu, T_("&Match queue"), users, [=](const HintedUser &u, const string& s) {
		qm->addList(u, QueueItem::FLAG_MATCH_QUEUE, Util::emptyString); });

	addUsers(menu, T_("&Send private message"), users, [parent](const HintedUser& u, const string&) {
		PrivateFrame::openWindow(parent, u); });

	addUsers(menu, T_("Add To &Favorites"), filter(users, &isFav), [=](const HintedUser &u, const string& s) {
		FavoriteManager::getInstance()->addFavoriteUser(u); }, dwt::IconPtr(new dwt::Icon(IDI_FAVORITE_USERS)));

	addUsers(menu, T_("Grant &extra slot"), users, [=](const HintedUser &u, const string& s) {
		UploadManager::getInstance()->reserveSlot(u); });

	typedef void (QueueManager::*qmp)(const UserPtr&, int);
	addUsers(menu, T_("Remove user from queue"), users, [=](const HintedUser &u, const string& s) {
		qm->removeSource(u, QueueItem::Source::FLAG_REMOVED); });
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

dwt::IconPtr WinUtil::toolbarIcon(unsigned id) {
	return createIcon(id, SETTING(TOOLBAR_SIZE));
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
