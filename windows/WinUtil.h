/* 
 * Copyright (C) 2001 Jacek Sieka, j_s@telia.com
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

#ifndef __WINUTIL_H
#define __WINUTIL_H

class WinUtil {
public:
	static CImageList fileImages;
	static HBRUSH bgBrush;
	static COLORREF textColor;
	static COLORREF bgColor;
	static HFONT font;
	static CMenu mainMenu;
	static int dirIconIndex;
	
	static void buildMenu();

	static void decodeFont(const string& setting, LOGFONT &dest);
	
	static string encodeFont(LOGFONT const& font)
	{
		string res(font.lfFaceName);
		res += ',';
		res += Util::toString(font.lfHeight);
		res += ',';
		res += Util::toString(font.lfWeight);
		res += ',';
		res += Util::toString(font.lfItalic);
		return res;
	}
	
	static bool browseFile(string& target, HWND owner = NULL, bool save = true, const string& initialDir = Util::emptyString);
	static bool browseDirectory(string& target, HWND owner = NULL);

	static int getIconIndex(const string& aFileName) {
		if(BOOLSETTING(USE_SYSTEM_ICONS)) {
			SHFILEINFO fi;
			CImageList il = (HIMAGELIST)::SHGetFileInfo(aFileName.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
			while(il.GetImageCount() > fileImages.GetImageCount()) {
				HICON hi = il.GetIcon(fileImages.GetImageCount());
				fileImages.AddIcon(hi);
				DestroyIcon(hi);
			}
			return fi.iIcon;
		} else {
			return 2;
		}
	}

	static int getDirIconIndex() {
		return dirIconIndex;
	}
	
private:
	static int CALLBACK browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData);		
	
};

#endif // __WINUTIL_H

/**
 * @file WinUtil.h
 * $Id: WinUtil.h,v 1.4 2002/05/01 21:22:08 arnetheduck Exp $
 */
