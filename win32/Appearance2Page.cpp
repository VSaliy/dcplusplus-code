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

#include "stdafx.h"

#include "resource.h"

#include "Appearance2Page.h"

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

Appearance2Page::SoundOption Appearance2Page::soundOptions[] = {
	{ N_("Every time a main chat message is received"), SettingsManager::SOUND_MAIN_CHAT, Util::emptyStringT, IDH_SETTINGS_APPEARANCE2_SOUND_MAIN_CHAT },
	{ N_("Every time a private message is received"), SettingsManager::SOUND_PM, Util::emptyStringT, IDH_SETTINGS_APPEARANCE2_SOUND_PM },
	{ N_("When a private message window is opened"), SettingsManager::SOUND_PM_WINDOW, Util::emptyStringT, IDH_SETTINGS_APPEARANCE2_SOUND_PM_WINDOW },
	{ 0, 0, Util::emptyStringT }
};

Appearance2Page::Appearance2Page(dwt::Widget* parent) :
PropPage(parent),
grid(0),
example(0),
sounds(0),
beepFileLabel(0),
beepFile(0),
browse(0),
oldSelection(-1)
{
	setHelpId(IDH_APPEARANCE2PAGE);

	grid = addChild(Grid::Seed(3, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Colors")))->addChild(Grid::Seed(2, 3));
		cur->column(1).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setHelpId(IDH_SETTINGS_APPEARANCE2_COLORS);

		ButtonPtr windowColor = cur->addChild(Button::Seed(T_("Select &window color")));
		windowColor->onClicked(std::tr1::bind(&Appearance2Page::handleBackgroundClicked, this));
		windowColor->setHelpId(IDH_SETTINGS_APPEARANCE2_SELWINCOLOR);

		Label::Seed seed(T_("Donate \342\202\254\342\202\254\342\202\254:s! (ok, dirty dollars are fine as well =) (see help menu)"));
		seed.style |= SS_NOPREFIX | SS_SUNKEN;
		example = cur->addChild(seed);
		cur->setWidget(example, 0, 1, 2, 1);
		example->setHelpId(IDH_SETTINGS_APPEARANCE2_COLORS);

		ButtonPtr uploads = cur->addChild(Button::Seed(T_("Uploads")));
		uploads->onClicked(std::tr1::bind(&Appearance2Page::handleULClicked, this));
		uploads->setHelpId(IDH_SETTINGS_APPEARANCE2_UPLOAD_BAR_COLOR);

		ButtonPtr textStyle	= cur->addChild(Button::Seed(T_("Select &text style")));
		textStyle->onClicked(std::tr1::bind(&Appearance2Page::handleTextClicked, this));
		textStyle->setHelpId(IDH_SETTINGS_APPEARANCE2_SELTEXT);

		ButtonPtr downloads	= cur->addChild(Button::Seed(T_("Downloads")));
		downloads->onClicked(std::tr1::bind(&Appearance2Page::handleDLClicked, this));
		downloads->setHelpId(IDH_SETTINGS_APPEARANCE2_DOWNLOAD_BAR_COLOR);
	}

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Sounds")))->addChild(Grid::Seed(2, 3));
		cur->column(1).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setHelpId(IDH_SETTINGS_APPEARANCE2_BEEPFILE);

		sounds = cur->addChild(WinUtil::Seeds::Dialog::optionsTable);
		cur->setWidget(sounds, 0, 0, 1, 3);
		sounds->setHelpId(IDH_SETTINGS_APPEARANCE2_BEEPFILE);

		beepFileLabel = cur->addChild(Label::Seed(T_("Notification sound")));
		beepFileLabel->setHelpId(IDH_SETTINGS_APPEARANCE2_BEEPFILE);

		beepFile = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		beepFile->setHelpId(IDH_SETTINGS_APPEARANCE2_BEEPFILE);

		browse = cur->addChild(Button::Seed(T_("&Browse...")));
		browse->setHelpId(IDH_SETTINGS_APPEARANCE2_BEEPFILE);
		browse->onClicked(std::tr1::bind(&Appearance2Page::handleBrowseClicked, this));
	}

	grid->addChild(Label::Seed(T_("Note; most of these options require that you restart DC++")))->setHelpId(IDH_SETTINGS_APPEARANCE_REQUIRES_RESTART);

	fg = SETTING(TEXT_COLOR);
	bg = SETTING(BACKGROUND_COLOR);
	upBar = SETTING(UPLOAD_BAR_COLOR);
	downBar = SETTING(DOWNLOAD_BAR_COLOR);

	WinUtil::decodeFont(Text::toT(SETTING(TEXT_FONT)), logFont);
	font = dwt::FontPtr(new dwt::Font(::CreateFontIndirect(&logFont), true));

	example->setColor(fg, bg);
	example->setFont(font);

	setBeepEnabled(false);

	sounds->createColumns(TStringList(1));

	for(size_t i = 0; soundOptions[i].setting != 0; ++i) {
		soundOptions[i].file = Text::toT(SettingsManager::getInstance()->get((SettingsManager::StrSetting)soundOptions[i].setting));

		TStringList row;
		row.push_back(T_(soundOptions[i].text));
		sounds->setChecked(sounds->insert(row), !soundOptions[i].file.empty());
	}

	sounds->setColumnWidth(0, LVSCW_AUTOSIZE);

	saveSoundOptions();

	sounds->onHelp(std::tr1::bind(&Appearance2Page::handleSoundsHelp, this, _1, _2));
	sounds->onSelectionChanged(std::tr1::bind(&Appearance2Page::handleSelectionChanged, this));
}

Appearance2Page::~Appearance2Page() {
}

void Appearance2Page::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientAreaSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void Appearance2Page::write() {
	SettingsManager* settings = SettingsManager::getInstance();

	settings->set(SettingsManager::TEXT_COLOR, (int)fg);
	settings->set(SettingsManager::BACKGROUND_COLOR, (int)bg);
	settings->set(SettingsManager::UPLOAD_BAR_COLOR, (int)upBar);
	settings->set(SettingsManager::DOWNLOAD_BAR_COLOR, (int)downBar);
	settings->set(SettingsManager::TEXT_FONT, Text::fromT(WinUtil::encodeFont(logFont)));

	saveSoundOptions();
	for(size_t i = 0; soundOptions[i].setting != 0; ++i)
		settings->set((SettingsManager::StrSetting)soundOptions[i].setting, Text::fromT(soundOptions[i].file));
}

void Appearance2Page::handleBackgroundClicked() {
	ColorDialog::ColorParams colorParams(bg);
	if(ColorDialog(this).open(colorParams)) {
		bg = colorParams.getColor();
		example->setColor(fg, bg);
		example->redraw();
	}
}

void Appearance2Page::handleTextClicked() {
	LOGFONT logFont_ = logFont;
	DWORD fg_ = fg;
	if(FontDialog(this).open(CF_EFFECTS | CF_SCREENFONTS, logFont_, fg_)) {
		logFont = logFont_;
		fg = fg_;
		font = dwt::FontPtr(new dwt::Font(::CreateFontIndirect(&logFont), true));
		example->setColor(fg, bg);
		example->setFont(font);
		example->redraw();
	}
}

void Appearance2Page::handleULClicked() {
	ColorDialog::ColorParams colorParams(upBar);
	if(ColorDialog(this).open(colorParams)) {
		upBar = colorParams.getColor();
	}
}

void Appearance2Page::handleDLClicked() {
	ColorDialog::ColorParams colorParams(downBar);
	if(ColorDialog(this).open(colorParams)) {
		downBar = colorParams.getColor();
	}
}

void Appearance2Page::handleSoundsHelp(HWND hWnd, unsigned id) {
	// same as PropPage::handleListHelp
	int item = sounds->hitTest(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos())));
	if(item >= 0 && soundOptions[item].helpId)
		id = soundOptions[item].helpId;
	WinUtil::help(hWnd, id);
}

void Appearance2Page::handleSelectionChanged() {
	saveSoundOptions();

	int sel = sounds->getSelected();
	if(sel >= 0) {
		bool checked = sounds->isChecked(sel);
		setBeepEnabled(checked);
		beepFile->setText(
			(checked && soundOptions[sel].file != _T("beep"))
			? soundOptions[sel].file
			: Util::emptyStringT
		);
	} else {
		setBeepEnabled(false);
		beepFile->setText(Util::emptyStringT);
	}
	oldSelection = sel;
}

void Appearance2Page::handleBrowseClicked() {
	tstring x = beepFile->getText();
	if(LoadDialog(this).open(x)) {
		beepFile->setText(x);
	}
}

void Appearance2Page::setBeepEnabled(bool enabled) {
	beepFileLabel->setEnabled(enabled);
	beepFile->setEnabled(enabled);
	browse->setEnabled(enabled);
}

void Appearance2Page::saveSoundOptions() {
	for(size_t i = 0; i < sounds->size(); ++i) {
		if(sounds->isChecked(i)) {
			if(soundOptions[i].file.empty())
				soundOptions[i].file = _T("beep");
		} else
			soundOptions[i].file = Util::emptyStringT;
	}

	if(oldSelection >= 0) {
		if(sounds->isChecked(oldSelection)) {
			tstring text = beepFile->getText();
			soundOptions[oldSelection].file = text.empty() ? _T("beep") : text;
		} else
			soundOptions[oldSelection].file = Util::emptyStringT;
	}
}
