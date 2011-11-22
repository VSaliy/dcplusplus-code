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
#include "StylesPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Button.h>
#include <dwt/widgets/ColorDialog.h>
#include <dwt/widgets/FontDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>

#include "resource.h"
#include "TypedTable.h"
#include "WinUtil.h"

using dwt::Button;
using dwt::ColorDialog;
using dwt::FontDialog;
using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

static const ColumnInfo columns[] = {
	{ "", 100, false }
};

/// @todo help
#define IDH_SETTINGS_STYLES_PREVIEW 0
#define IDH_SETTINGS_STYLES_FONT 0
#define IDH_SETTINGS_STYLES_TEXT 0
#define IDH_SETTINGS_STYLES_BG 0
#define IDH_SETTINGS_STYLES_GLOBAL 0
#define IDH_SETTINGS_STYLES_UPLOADS 0
#define IDH_SETTINGS_STYLES_DOWNLOADS 0

StylesPage::StylesPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
table(0),
preview(0),
customFont(0),
font(0),
customTextColor(0),
textColor(0),
customBgColor(0),
bgColor(0)
{
	setHelpId(IDH_STYLESPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto cur = grid->addChild(GroupBox::Seed(T_("Styles")))->addChild(Grid::Seed(5, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(grid->getSpacing());

		{
			auto seed = WinUtil::Seeds::Dialog::table;
			seed.style &= ~LVS_SHOWSELALWAYS;
			seed.style |= LVS_SINGLESEL | LVS_NOCOLUMNHEADER;
			table = cur->addChild(Table::Seed(seed));
			table->handleGroupDraw();
		}

		{
			Label::Seed seed;
			seed.exStyle |= WS_EX_CLIENTEDGE;
			preview = cur->addChild(seed);
			preview->setHelpId(IDH_SETTINGS_STYLES_PREVIEW);
		}

		auto cur2 = cur->addChild(Grid::Seed(1, 2));
		cur2->setHelpId(IDH_SETTINGS_STYLES_FONT);

		customFont = cur2->addChild(CheckBox::Seed(T_("Custom font")));
		customFont->onClicked([this] { handleCustomFont(); });

		font = cur2->addChild(Button::Seed(T_("Select font")));
		font->onClicked([this] { handleFont(); });

		cur2 = cur->addChild(Grid::Seed(1, 2));
		cur2->setHelpId(IDH_SETTINGS_STYLES_TEXT);

		customTextColor = cur2->addChild(CheckBox::Seed(T_("Custom text color")));
		customTextColor->onClicked([this] { handleCustomTextColor(); });

		textColor = cur2->addChild(Button::Seed(T_("Select color")));
		textColor->onClicked([this] { handleTextColor(); });

		cur2 = cur->addChild(Grid::Seed(1, 2));
		cur2->setHelpId(IDH_SETTINGS_STYLES_BG);

		customBgColor = cur2->addChild(CheckBox::Seed(T_("Custom background color")));
		customBgColor->onClicked([this] { handleCustomBgColor(); });

		bgColor = cur2->addChild(Button::Seed(T_("Select color")));
		bgColor->onClicked([this] { handleBgColor(); });
	}

	WinUtil::makeColumns(table, columns, COLUMN_LAST);

	TStringList groups(GROUP_LAST);
	groups[GROUP_GENERAL] = T_("General");
	groups[GROUP_TRANSFERS] = T_("Transfers");
	table->setGroups(groups);
	auto grouped = table->isGrouped();

	auto add = [this, grouped](tstring&& text, unsigned helpId, int group, int fontSetting, int textColorSetting, int bgColorSetting) -> Data* {
		auto data = new Data(forward<tstring>(text), helpId, fontSetting, textColorSetting, bgColorSetting);
		table->insert(grouped ? group : -1, data);
		return data;
	};

	globalData = add(T_("Global application style"), IDH_SETTINGS_STYLES_GLOBAL, GROUP_GENERAL,
		SettingsManager::MAIN_FONT, SettingsManager::TEXT_COLOR, SettingsManager::BACKGROUND_COLOR);
	add(T_("Uploads"), IDH_SETTINGS_STYLES_UPLOADS, GROUP_TRANSFERS,
		SettingsManager::UPLOAD_FONT, SettingsManager::UPLOAD_TEXT_COLOR, SettingsManager::UPLOAD_BG_COLOR);
	add(T_("Downloads"), IDH_SETTINGS_STYLES_DOWNLOADS, GROUP_TRANSFERS,
		SettingsManager::DOWNLOAD_FONT, SettingsManager::DOWNLOAD_TEXT_COLOR, SettingsManager::DOWNLOAD_BG_COLOR);

	update(globalData);

	handleSelectionChanged();

	table->onSelectionChanged([this] { handleSelectionChanged(); });

	table->onHelp([this](Widget*, unsigned id) { handleTableHelp(id); });
	table->setHelpId([this](unsigned& id) { handleTableHelpId(id); });
}

StylesPage::~StylesPage() {
}

void StylesPage::layout() {
	PropPage::layout();

	table->setColumnWidth(COLUMN_TEXT, table->getWindowSize().x - 20);
}

void StylesPage::write() {
	table->forEach(&StylesPage::Data::write);
}

StylesPage::Data::Data(tstring&& text, unsigned helpId, int fontSetting, int textColorSetting, int bgColorSetting) :
text(forward<tstring>(text)),
helpId(helpId),
fontSetting(fontSetting),
textColorSetting(textColorSetting),
bgColorSetting(bgColorSetting),
customFont(!SettingsManager::getInstance()->isDefault(static_cast<SettingsManager::StrSetting>(fontSetting))),
customTextColor(!SettingsManager::getInstance()->isDefault(static_cast<SettingsManager::IntSetting>(textColorSetting))),
textColor(SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(textColorSetting))),
customBgColor(!SettingsManager::getInstance()->isDefault(static_cast<SettingsManager::IntSetting>(bgColorSetting))),
bgColor(SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(bgColorSetting)))
{
	makeFont(font, SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(fontSetting)));
	makeFont(defaultFont, SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::StrSetting>(fontSetting)));
}

const tstring& StylesPage::Data::getText(int) const {
	return text;
}

int StylesPage::Data::getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const {
	auto f = getFont();
	if(f.first) {
		font = f.first->handle();
	}

	auto color = getTextColor();
	if(color >= 0) {
		textColor = color;
	}

	color = getBgColor();
	if(color >= 0) {
		bgColor = color;
	}

	return CDRF_NEWFONT;
}

const StylesPage::Data::Font& StylesPage::Data::getFont() const {
	return customFont ? font : defaultFont;
}

COLORREF StylesPage::Data::getTextColor() const {
	return customTextColor ? textColor : SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(textColorSetting));
}

COLORREF StylesPage::Data::getBgColor() const {
	return customBgColor ? bgColor : SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(bgColorSetting));
}

void StylesPage::Data::write() {
	if(customFont) {
		SettingsManager::getInstance()->set(static_cast<SettingsManager::StrSetting>(fontSetting), Text::fromT(WinUtil::encodeFont(font.second)));
	} else {
		SettingsManager::getInstance()->unset(static_cast<SettingsManager::StrSetting>(fontSetting));
	}

	if(customTextColor) {
		SettingsManager::getInstance()->set(static_cast<SettingsManager::IntSetting>(textColorSetting), static_cast<int>(textColor));
	} else {
		SettingsManager::getInstance()->unset(static_cast<SettingsManager::IntSetting>(textColorSetting));
	}

	if(customBgColor) {
		SettingsManager::getInstance()->set(static_cast<SettingsManager::IntSetting>(bgColorSetting), static_cast<int>(bgColor));
	} else {
		SettingsManager::getInstance()->unset(static_cast<SettingsManager::IntSetting>(bgColorSetting));
	}
}

void StylesPage::Data::makeFont(Font& dest, const string& setting) {
	if(!setting.empty()) {
		WinUtil::decodeFont(Text::toT(setting), dest.second);
		dest.first.reset(new dwt::Font(dest.second));
	}
}

void StylesPage::handleSelectionChanged() {
	auto data = table->getSelectedData();

	bool enable = data;
	if(data) {
		updatePreview(data);
	}
	preview->setText(data ? data->text : Util::emptyStringT);
	preview->setEnabled(enable);
	preview->setVisible(enable);
	preview->getParent()->layout();

	enable = data && data->customFont;
	customFont->setChecked(enable);
	customFont->setEnabled(data);
	font->setEnabled(enable);

	enable = data && data->customTextColor;
	customTextColor->setChecked(enable);
	customTextColor->setEnabled(data);
	textColor->setEnabled(enable);

	enable = data && data->customBgColor;
	customBgColor->setChecked(enable);
	customBgColor->setEnabled(data);
	bgColor->setEnabled(enable);
}

void StylesPage::handleTableHelp(unsigned id) {
	// same as PropPage::handleListHelp
	int item =
		isKeyPressed(VK_F1) ? table->getSelected() :
		table->hitTest(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos()))).first;
	if(item >= 0)
		id = table->getData(item)->helpId;
	WinUtil::help(table, id);
}

void StylesPage::handleTableHelpId(unsigned& id) {
	// same as PropPage::handleListHelpId
	int item = table->getSelected();
	if(item >= 0)
		id = table->getData(item)->helpId;
}

void StylesPage::handleCustomFont() {
	auto data = table->getSelectedData();
	data->customFont = customFont->getChecked();
	update(data);
	handleSelectionChanged();
}

void StylesPage::handleFont() {
	auto data = table->getSelectedData();
	FontDialog::Options options;
	options.strikeout = false;
	options.underline = false;
	options.color = false;
	options.bgColor = data->bgColor;
	if(!data->font.first) {
		// initialize the LOGFONT structure.
		data->font.second = data->defaultFont.first ? data->defaultFont.second : globalData->getFont().second;
	}
	if(FontDialog(this).open(data->font.second, data->textColor, &options)) {
		data->font.first.reset(new dwt::Font(data->font.second));
		update(data);
	}
}

void StylesPage::handleCustomTextColor() {
	auto data = table->getSelectedData();
	data->customTextColor = customTextColor->getChecked();
	update(data);
	handleSelectionChanged();
}

void StylesPage::handleTextColor() {
	auto data = table->getSelectedData();
	colorDialog(data->textColor);
	update(data);
}

void StylesPage::handleCustomBgColor() {
	auto data = table->getSelectedData();
	data->customBgColor = customBgColor->getChecked();
	update(data);
	handleSelectionChanged();
}

void StylesPage::handleBgColor() {
	auto data = table->getSelectedData();
	colorDialog(data->bgColor);
	update(data);
}

void StylesPage::colorDialog(COLORREF& color) {
	ColorDialog::ColorParams colorParams(color);
	if(ColorDialog(this).open(colorParams)) {
		color = colorParams.getColor();
	}
}

void StylesPage::update(Data* const data) {
	if(data == globalData) {
		table->setFont(globalData->getFont().first);
		table->setColor(globalData->getTextColor(), globalData->getBgColor());
		table->Control::redraw(true);
	} else {
		table->update(data);
	}
	updatePreview(data);
	preview->getParent()->layout();
}

void StylesPage::updatePreview(Data* const data) {
	auto font = data->getFont();
	preview->setFont(font.first ? font.first : globalData->getFont().first);
	auto textColor = data->getTextColor();
	auto bgColor = data->getBgColor();
	preview->setColor((textColor >= 0) ? textColor : globalData->getTextColor(), (bgColor >= 0) ? bgColor : globalData->getBgColor());
	preview->redraw(true);
}
