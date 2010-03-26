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

#ifndef DCPLUSPLUS_WIN32_WIN_UTIL_H
#define DCPLUSPLUS_WIN32_WIN_UTIL_H

#include <dcpp/StringTokenizer.h>
#include <dcpp/Util.h>
#include <dcpp/User.h>
#include <dcpp/forward.h>
#include <dcpp/MerkleTree.h>

// Some utilities for handling HLS colors, taken from Jean-Michel LE FOL's codeproject
// article on WTL OfficeXP Menus
typedef DWORD HLSCOLOR;
#define HLS(h,l,s) ((HLSCOLOR)(((BYTE)(h)|((WORD)((BYTE)(l))<<8))|(((DWORD)(BYTE)(s))<<16)))
#define HLS_H(hls) ((BYTE)(hls))
#define HLS_L(hls) ((BYTE)(((WORD)(hls)) >> 8))
#define HLS_S(hls) ((BYTE)((hls)>>16))

HLSCOLOR RGB2HLS (COLORREF rgb);
COLORREF HLS2RGB (HLSCOLOR hls);

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S);

class MainWindow;

struct ColumnInfo {
	const char* name;
	const int size;
	const bool rightAlign;
};

class WinUtil {
public:
	static tstring tth;

	static dwt::BrushPtr bgBrush;
	static COLORREF textColor;
	static COLORREF bgColor;
	static dwt::FontPtr font;
	static dwt::FontPtr monoFont;
	static tstring commands;
	static dwt::ImageListPtr fileImages;
	static dwt::ImageListPtr userImages;
	static int fileImageCount;
	static int dirIconIndex;
	static int dirMaskedIndex;
	static TStringList lastDirs;
	static MainWindow* mainWindow;
	//static dwt::TabView* mdiParent;

	typedef unordered_map<string, int> ImageMap;
	typedef ImageMap::iterator ImageIter;
	static ImageMap fileIndexes;

	struct Seeds {
		static const Button::Seed button;
		static const ComboBox::Seed comboBoxStatic;
		static const ComboBox::Seed comboBoxEdit;
		static const CheckBox::Seed checkBox;
		static const CheckBox::Seed splitCheckBox; // +/-
		static const GroupBox::Seed group;
		static const Menu::Seed menu;
		static const Table::Seed table;
		static const TextBox::Seed textBox;
		static const RichTextBox::Seed richTextBox;
		static const TabView::Seed tabs;
		static const Tree::Seed treeView;

		struct Dialog {
			static const ComboBox::Seed comboBox;
			static const TextBox::Seed textBox;
			static const TextBox::Seed intTextBox;
			static const Table::Seed table;
			static const Table::Seed optionsTable;
		};
	};

	static void init();
	static void uninit();

	static tstring encodeFont(LOGFONT const& font);
	static void decodeFont(const tstring& setting, LOGFONT &dest);

	static void setStaticWindowState(const string& id, bool open);

	static bool checkNick();
	static void handleDblClicks(dwt::TextBoxBase* box);

	/**
	 * Check if this is a common /-command.
	 * @param cmd The whole text string, will be updated to contain only the command.
	 * @param param Set to any parameters.
	 * @param message Message that should be sent to the chat.
	 * @param status Message that should be shown in the status line.
	 * @param thirdPerson True if the /me command was used.
	 * @return True if the command was processed, false otherwise.
	 */
	static bool checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);

	static void playSound(int setting);

	static void openFile(const tstring& file);
	static void openFolder(const tstring& file);

	static void makeColumns(dwt::TablePtr table, const ColumnInfo* columnInfo, size_t columnCount,
		const string& order = Util::emptyString, const string& widths = Util::emptyString);

	template<typename T>
	static std::vector<int> splitTokens(const string& str, const T& defaults) {
		const size_t n = sizeof(defaults) / sizeof(defaults[0]);
		std::vector<int> ret(defaults, defaults + n);
		StringTokenizer<string> tokens(str, ',') ;
		const StringList& l = tokens.getTokens();
		for(size_t i = 0; i < std::min(ret.size(), l.size()); ++i) {
			ret[i] = Util::toInt(l[i]);
		}
		return ret;
	}
	static std::string toString(const std::vector<int>& tokens);
	static void toInts(const string& str, std::vector<int>& tokens);

	template<typename T>
	static TStringList getStrings(const T& t) {
		const size_t n = sizeof(t) / sizeof(t[0]);
		TStringList ret(n);
		for(size_t i = 0; i < n; ++i) {
			ret[i] = T_(t[i]);
		}
		return ret;
	}

	static pair<ButtonPtr, ButtonPtr> addDlgButtons(
		GridPtr grid,
		const dwt::Application::Callback& f_ok,
		const dwt::Application::Callback& f_cancel);

	static int getIconIndex(const tstring& aFileName);
	static int getDirIconIndex() { return dirIconIndex; }
	static int getDirMaskedIndex() { return dirMaskedIndex; }

	static bool isShift() { return (::GetKeyState(VK_SHIFT) & 0x8000) > 0; }
	static bool isAlt() { return (::GetKeyState(VK_MENU) & 0x8000) > 0; }
	static bool isCtrl() { return (::GetKeyState(VK_CONTROL) & 0x8000) > 0; }

	static tstring getNicks(const CID& cid, const string& hintUrl);
	static tstring getNicks(const UserPtr& u, const string& hintUrl);
	static tstring getNicks(const CID& cid, const string& hintUrl, bool priv);
	static tstring getNicks(const HintedUser& user) { return getNicks(user.user->getCID(), user.hint); }

	/** @return Pair of hubnames as a string and a bool representing the user's online status */
	static pair<tstring, bool> getHubNames(const CID& cid, const string& hintUrl);
	static pair<tstring, bool> getHubNames(const UserPtr& u, const string& hintUrl);
	static pair<tstring, bool> getHubNames(const CID& cid, const string& hintUrl, bool priv);
	static pair<tstring, bool> getHubNames(const HintedUser& user) { return getHubNames(user.user->getCID(), user.hint); }

	static void reducePaths(string& message);

	// Hash related
	static void addHashItems(const dwt::Menu::ObjectType& menu, const TTHValue& tth, const tstring& filename);
	static void bitziLink(const TTHValue& /*aHash*/);
	static void copyMagnet(const TTHValue& /*aHash*/, const tstring& /*aFile*/);
	static void searchHash(const TTHValue& /*aHash*/);

	static void addLastDir(const tstring& dir);

	static void openLink(const tstring& url);
	static bool browseSaveFile(dwt::Widget* parent, tstring& file);
	static bool browseFileList(dwt::Widget* parent, tstring& file);

	static void setClipboard(const tstring& str);

	static bool getUCParams(dwt::Widget* parent, const UserCommand& cmd, StringMap& sm) throw();

	static bool parseDBLClick(const tstring& aString);
	static void parseDchubUrl(const tstring& /*aUrl*/);
	static void parseADChubUrl(const tstring& /*aUrl*/, bool isSecure);
	static void parseMagnetUri(const tstring& /*aUrl*/, bool aOverride = false);

	static void help(dwt::Control* widget, unsigned id);
	static string getHelpText(unsigned id);

	// URL related
	static void registerHubHandlers();
	static void registerMagnetHandler();

	static string getAppName() {
		TCHAR buf[MAX_PATH+1];
		DWORD x = GetModuleFileName(NULL, buf, MAX_PATH);
		return Text::fromT(tstring(buf, x));
	}

	static void addUserItems(MenuPtr menu, const HintedUserList& users, dwt::TabViewPtr parent, const StringList& dirs = StringList());

	/* utility functions to create icons. use these throughout the prog to make it easier to change
	sizes globally should the need arise to later on. */
	static dwt::IconPtr createIcon(unsigned id, long size);
	static inline dwt::IconPtr menuIcon(unsigned id) { return createIcon(id, 16); }
	static inline dwt::IconPtr statusIcon(unsigned id) { return createIcon(id, 16); }
	static inline dwt::IconPtr tabIcon(unsigned id) { return createIcon(id, 16); }

#ifdef PORT_ME
	static double toBytes(TCHAR* aSize);
#endif

private:
	static bool handleBoxDblClick(dwt::TextBoxBase* box, const dwt::MouseEvent& ev);

	static void init_helpPath();

	static DWORD helpCookie;
	static tstring helpPath;
	static StringList helpTexts;

	static bool urlDcADCRegistered;
	static bool urlMagnetRegistered;
};

#endif // !defined(WIN_UTIL_H)
