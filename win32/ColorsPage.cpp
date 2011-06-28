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
#include "ColorsPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Button.h>
#include <dwt/widgets/ColorDialog.h>
#include <dwt/widgets/FontDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Button;
using dwt::ColorDialog;
using dwt::FontDialog;
using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

ColorsPage::ColorsPage(dwt::Widget* parent) :
PropPage(parent, 2, 1),
example(0)
{
	setHelpId(IDH_COLORSPAGE);

	grid->column(0).mode = GridInfo::FILL;

	{
		auto cur = grid->addChild(GroupBox::Seed(T_("Colors")))->addChild(Grid::Seed(2, 3));
		cur->column(1).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setHelpId(IDH_SETTINGS_COLORS_COLORS);

		ButtonPtr windowColor = cur->addChild(Button::Seed(T_("Select &window color")));
		windowColor->onClicked([this] { handleBackgroundClicked(); });
		windowColor->setHelpId(IDH_SETTINGS_COLORS_SELWINCOLOR);

		Label::Seed seed(T_("Donate \342\202\254\342\202\254\342\202\254:s! (ok, dirty dollars are fine as well =) (see help menu)"));
		seed.style |= SS_SUNKEN;
		example = cur->addChild(seed);
		cur->setWidget(example, 0, 1, 2, 1);
		example->setHelpId(IDH_SETTINGS_COLORS_COLORS);

		auto uploads = cur->addChild(Button::Seed(T_("Uploads")));
		uploads->onClicked([this] { handleULClicked(); });
		uploads->setHelpId(IDH_SETTINGS_COLORS_UPLOAD_BAR_COLOR);

		auto textStyle	= cur->addChild(Button::Seed(T_("Select &text style")));
		textStyle->onClicked([this] { handleTextClicked(); });
		textStyle->setHelpId(IDH_SETTINGS_COLORS_SELTEXT);

		auto downloads	= cur->addChild(Button::Seed(T_("Downloads")));
		downloads->onClicked([this] { handleDLClicked(); });
		downloads->setHelpId(IDH_SETTINGS_COLORS_DOWNLOAD_BAR_COLOR);
	}

	grid->addChild(Label::Seed(T_("Note; most of these options require that you restart DC++")))->setHelpId(IDH_SETTINGS_APPEARANCE_REQUIRES_RESTART);

	fg = SETTING(TEXT_COLOR);
	bg = SETTING(BACKGROUND_COLOR);
	upBar = SETTING(UPLOAD_BAR_COLOR);
	downBar = SETTING(DOWNLOAD_BAR_COLOR);

	WinUtil::decodeFont(Text::toT(SETTING(MAIN_FONT)), logFont);
	font = dwt::FontPtr(new dwt::Font(logFont));

	example->setColor(fg, bg);
	example->setFont(font);
}

ColorsPage::~ColorsPage() {
}

void ColorsPage::write() {
	SettingsManager* settings = SettingsManager::getInstance();

	settings->set(SettingsManager::TEXT_COLOR, (int)fg);
	settings->set(SettingsManager::BACKGROUND_COLOR, (int)bg);
	settings->set(SettingsManager::UPLOAD_BAR_COLOR, (int)upBar);
	settings->set(SettingsManager::DOWNLOAD_BAR_COLOR, (int)downBar);
	settings->set(SettingsManager::MAIN_FONT, Text::fromT(WinUtil::encodeFont(logFont)));
}

void ColorsPage::handleBackgroundClicked() {
	ColorDialog::ColorParams colorParams(bg);
	if(ColorDialog(this).open(colorParams)) {
		bg = colorParams.getColor();
		example->setColor(fg, bg);
		example->redraw();
	}
}

void ColorsPage::handleTextClicked() {
	FontDialog::Options options;
	options.strikeout = false;
	options.underline = false;
	options.bgColor = bg;
	if(FontDialog(this).open(logFont, fg, &options)) {
		font = dwt::FontPtr(new dwt::Font(logFont));
		example->setColor(fg, bg);
		example->setFont(font);
		example->redraw();
	}
}

void ColorsPage::handleULClicked() {
	ColorDialog::ColorParams colorParams(upBar);
	if(ColorDialog(this).open(colorParams)) {
		upBar = colorParams.getColor();
	}
}

void ColorsPage::handleDLClicked() {
	ColorDialog::ColorParams colorParams(downBar);
	if(ColorDialog(this).open(colorParams)) {
		downBar = colorParams.getColor();
	}
}
