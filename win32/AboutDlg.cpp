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

#include "resource.h"

#include "AboutDlg.h"

#include <dcpp/SimpleXML.h>
#include <dcpp/version.h>
#include "WinUtil.h"

static const char thanks[] = "Big thanks to all donators and people who have contributed with ideas "
"and code! Thanks go out to sourceforge.net for hosting the project. "
"This product uses bzip2 (www.bzip.org), thanks to Julian Seward and team for providing it. "
"This product uses zlib (www.zlib.net), thanks to Jean-loup Gailly and Mark Adler for providing it. "
"This product includes GeoIP data created by MaxMind, available from http://maxmind.com/. "
"This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit. (http://www.openssl.org/). "
"This product uses the MiniUPnP client library (http://miniupnp.tuxfamily.org) by Thomas Bernard. "
"The following people have contributed code to DC++ (I hope I haven't missed someone, they're "
"roughly in chronological order...=):\r\n"
"geoff, carxor, luca rota, dan kline, mike, anton, zc, sarf, farcry, kyrre aalerud, opera, "
"patbateman, xeroc, fusbar, vladimir marko, kenneth skovhede, ondrea, todd pederzani, who, "
"sedulus, sandos, henrik engstr\303\266m, dwomac, robert777, saurod, atomicjo, bzbetty, orkblutt, "
"distiller, citruz, dan fulger, cologic, christer palm, twink, ilkka sepp\303\244l\303\244, johnny, ciber, "
"theparanoidone, gadget, naga, tremor, joakim tosteberg, pofis, psf8500, lauris ievins, "
"defr, ullner, fleetcommand, liny, xan, olle svensson, mark gillespie, jeremy huddleston, "
"bsod, sulan, jonathan stone, tim burton, izzzo, guitarm, paka, nils maier, jens oknelid, yoji, "
"krzysztof tyszecki, poison, mikejj, pur, bigmuscle, martin, jove, bart vullings, "
"steven sheehy, tobias nygren, poy, dorian, stephan hohe, mafa_45, mikael eman, james ross, "
"stanislav maslovski, david grundberg, pavel andreev, yakov suraev, kulmegil, smir, emtee, individ, "
"pseudonym, crise, ben, ximin luo, radox, razzloss, andrew browne, darkklor, vasily.n, netcelli, "
"gennady proskurin. Keep it coming!";

AboutDlg::AboutDlg(dwt::Widget* parent) :
dwt::ModalDialog(parent),
grid(0),
version(0)
{
	onInitDialog(std::bind(&AboutDlg::handleInitDialog, this));
}

AboutDlg::~AboutDlg() {
}

int AboutDlg::run() {
	create(dwt::Point(371, 490));
	return show();
}

bool AboutDlg::handleInitDialog() {
	grid = addChild(Grid::Seed(6, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	// horizontally centered seeds
	GroupBox::Seed gs;
	gs.style |= BS_CENTER;
	gs.padding.y = 2;
	Label::Seed ls;
	ls.style |= SS_CENTER;

	{
		GridPtr cur = grid->addChild(gs)->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::CENTER;

		cur->addChild(Label::Seed(IDI_DCPP));

		ls.caption = Text::toT(dcpp::fullVersionString) + _T("\n(c) Copyright 2001-2010 Jacek Sieka\n");
		ls.caption += T_("Ex-codeveloper: Per Lind\303\251n\nGraphics: Martin Skogevall et al.\nDC++ is licenced under GPL\n");
		ls.caption += _T("http://dcplusplus.sourceforge.net/");
		cur->addChild(ls);

		gs.caption = T_("TTH");
		TextBox::Seed seed = WinUtil::Seeds::Dialog::textBox;
		seed.style |= ES_READONLY;
		seed.exStyle &= ~WS_EX_CLIENTEDGE;
		seed.caption = WinUtil::tth;
		cur->addChild(gs)->addChild(seed);
	}

	{
		gs.caption = T_("Greetz and Contributors");
		TextBox::Seed seed = WinUtil::Seeds::Dialog::textBox;
		seed.style &= ~ES_AUTOHSCROLL;
		seed.style |= ES_MULTILINE | WS_VSCROLL | ES_READONLY;
		seed.caption = Text::toT(thanks);
		grid->addChild(gs)->addChild(seed);
	}

	{
		gs.caption = T_("Totals");
		GridPtr cur = grid->addChild(gs)->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		ls.caption = str(TF_("Upload: %1%, Download: %2%") % Text::toT(Util::formatBytes(SETTING(TOTAL_UPLOAD))) % Text::toT(Util::formatBytes(SETTING(TOTAL_DOWNLOAD))));
		cur->addChild(ls);

		ls.caption = (SETTING(TOTAL_DOWNLOAD) > 0)
			? str(TF_("Ratio (up/down): %1$0.2f") % (((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD))))
			: T_("No transfers yet");
		cur->addChild(ls);
	}

	gs.caption = T_("Latest stable version");
	ls.caption = T_("Downloading...");
	version = grid->addChild(gs)->addChild(ls);

	WinUtil::addDlgButtons(grid,
		std::bind(&AboutDlg::endDialog, this, IDOK),
		std::bind(&AboutDlg::endDialog, this, IDCANCEL)).second->setVisible(false);

	setText(T_("About DC++"));

	layout();
	centerWindow();

	c.addListener(this);
	c.downloadFile("http://dcplusplus.sourceforge.net/version.xml");

	return false;
}

void AboutDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}

void AboutDlg::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw() {
	downBuf.append((char*)buf, len);
}

void AboutDlg::on(HttpConnectionListener::Complete, HttpConnection* conn, const string&, bool) throw() {
	tstring x;
	if(!downBuf.empty()) {
		try {
			SimpleXML xml;
			xml.fromXML(downBuf);
			if(xml.findChild("DCUpdate")) {
				xml.stepIn();
				if(xml.findChild("Version")) {
					x = Text::toT(xml.getChildData());
				}
			}
		} catch(const SimpleXMLException&) { }
	}
	if(x.empty())
		x = T_("Error processing version information");
	callAsync(std::bind(&Label::setText, version, x));

	conn->removeListener(this);
}

void AboutDlg::on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw() {
	callAsync(std::bind(&Label::setText, version, Text::toT(aLine)));
	conn->removeListener(this);
}
