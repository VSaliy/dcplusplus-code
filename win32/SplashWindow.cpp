/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "SplashWindow.h"

#include <dcpp/format.h>
#include <dcpp/Text.h>
#include <dcpp/version.h>

#include <dwt/DWTException.h>

#include "resource.h"
#include "WinUtil.h"

SplashWindow::SplashWindow() :
dwt::Window(0)
{
	// 256x icons only work on >= Vista. on failure, try loading a 48x image.
	try {
		iconSize = 256;
		icon = WinUtil::createIcon(IDI_DCPP, iconSize);
	} catch(const dwt::DWTException&) {
		iconSize = 48;
		icon = WinUtil::createIcon(IDI_DCPP, iconSize);
	}

	{
		// create a layered window (WS_EX_LAYERED); it will be shown later with UpdateLayeredWindow.
		Seed seed(_T(APPNAME) _T(" ") _T(VERSIONSTRING));
		seed.style = WS_POPUP | WS_VISIBLE;
		seed.exStyle = WS_EX_LAYERED;
		create(seed);
	}

	setFont(0); // default font since settings haven't been loaded yet

	// start showing when loading settings. language info isn't loaded at this point...
	operator()(Text::fromT(_T(APPNAME)));
}

SplashWindow::~SplashWindow() {
}

void SplashWindow::operator()(const string& status) {
	auto text = str(TF_("Loading DC++, please wait... (%1%)") % Text::toT(status));

	// set up sizes.
	const long spacing { 6 }; // space between the icon and the text
	const dwt::Point padding { 4, 4 }; // padding borders around the text
	const auto textSize = getTextSize(text) + padding + padding;
	SIZE size { std::max(iconSize, textSize.x), iconSize + spacing + textSize.y };
	dwt::Rectangle textRect { std::max(iconSize - textSize.x, 0L) / 2, size.cy - textSize.y, textSize.x, textSize.y };

	dwt::UpdateCanvas windowCanvas { this };
	dwt::CompatibleCanvas canvas { windowCanvas.handle() };

	// create the bitmap with CreateDIBSection to have access to its bits (used when drawing text).
	BITMAPINFO info { { sizeof(BITMAPINFOHEADER), size.cx, -size.cy, 1, 32, BI_RGB } };
	RGBQUAD* bits;
	dwt::Bitmap bitmap { ::CreateDIBSection(windowCanvas.handle(), &info, DIB_RGB_COLORS, &reinterpret_cast<void*&>(bits), 0, 0) };
	auto select(canvas.select(bitmap));

	// draw the icon.
	canvas.drawIcon(icon, dwt::Rectangle(std::max(textSize.x - iconSize, 0L) / 2, 0, iconSize, iconSize));

	// draw text borders and fill the text background.
	::RECT rc = textRect;
	::DrawEdge(canvas.handle(), &rc, EDGE_BUMP, BF_RECT | BF_MIDDLE);

	// draw the text.
	auto bkMode(canvas.setBkMode(true));
	canvas.setTextColor(dwt::Color::predefined(COLOR_WINDOWTEXT));
	auto selectFont(canvas.select(*getFont()));
	canvas.drawText(text.c_str(), textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// set bits within the text rectangle to not be transparent. rgbReserved is the alpha value.
	for(long x = textRect.left(), xn = textRect.right(); x < xn; ++x) {
		for(long y = textRect.top(), yn = textRect.bottom(); y < yn; ++y) {
			bits[x + y * size.cx].rgbReserved = 255;
		}
	}

	auto desktop = getPrimaryDesktopSize();
	POINT pt { std::max(desktop.x - size.cx, 0L) / 2, std::max(desktop.y - size.cy - iconSize, 0L) / 2 };
	POINT canvasPt { 0 };
	BLENDFUNCTION blend { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	::UpdateLayeredWindow(handle(), windowCanvas.handle(), &pt, &size, canvas.handle(), &canvasPt, 0, &blend, ULW_ALPHA);
}
