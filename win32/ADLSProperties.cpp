/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

#include "ADLSProperties.h"

#include <dcpp/ADLSearch.h>
#include <dcpp/FavoriteManager.h>
#include "WinUtil.h"

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_ADLSP_SEARCH, IDH_ADLSP_SEARCH_STRING },
	{ IDC_SEARCH_STRING, IDH_ADLSP_SEARCH_STRING },
	{ IDC_ADLSP_TYPE, IDH_ADLSP_SOURCE_TYPE },
	{ IDC_SOURCE_TYPE, IDH_ADLSP_SOURCE_TYPE },
	{ IDC_ADLSP_SIZE_MIN, IDH_ADLSP_MIN_FILE_SIZE },
	{ IDC_MIN_FILE_SIZE, IDH_ADLSP_MIN_FILE_SIZE },
	{ IDC_ADLSP_SIZE_MAX, IDH_ADLSP_MAX_FILE_SIZE },
	{ IDC_MAX_FILE_SIZE, IDH_ADLSP_MAX_FILE_SIZE },
	{ IDC_ADLSP_UNITS, IDH_ADLSP_SIZE_TYPE },
	{ IDC_SIZE_TYPE, IDH_ADLSP_SIZE_TYPE },
	{ IDC_ADLSP_DESTINATION, IDH_ADLSP_DEST_DIR },
	{ IDC_DEST_DIR, IDH_ADLSP_DEST_DIR },
	{ IDC_IS_ACTIVE, IDH_ADLSP_ENABLED },
	{ IDC_AUTOQUEUE, IDH_ADLSP_AUTOQUEUE },
	{ IDOK, IDH_DCPP_OK },
	{ IDCANCEL, IDH_DCPP_CANCEL },
	{ 0, 0 }
};
*/

ADLSProperties::ADLSProperties(dwt::Widget* parent, ADLSearch *_search) :
WidgetFactory<dwt::ModalDialog>(parent),
grid(0),
searchString(0),
searchType(0),
minSize(0),
maxSize(0),
sizeType(0),
destDir(0),
active(0),
autoQueue(0),
search(_search)
{
	onInitDialog(std::tr1::bind(&ADLSProperties::handleInitDialog, this));
	onFocus(std::tr1::bind(&ADLSProperties::handleFocus, this));
}

ADLSProperties::~ADLSProperties() {
}

int ADLSProperties::run() {
	createDialog(IDD_ADLS_PROPERTIES);
	return show();
}

bool ADLSProperties::handleInitDialog() {
	setHelpId(IDH_ADLSP);

	grid = addChild(Grid::Seed(4, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;

	GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Search String")));
	searchString = group->addChild(WinUtil::Seeds::Dialog::TextBox);
	searchString->setText(Text::toT(search->searchString));

	group = grid->addChild(GroupBox::Seed(T_("Source Type")));
	searchType = group->addChild(WinUtil::Seeds::comboBoxStatic);
	searchType->addValue(T_("Filename"));
	searchType->addValue(T_("Directory"));
	searchType->addValue(T_("Full Path"));
	searchType->setSelected(search->sourceType);

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(1).mode = GridInfo::FILL;

		group = cur->addChild(GroupBox::Seed(T_("Min FileSize")));
		minSize = group->addChild(WinUtil::Seeds::Dialog::intTextBox);
		minSize->setText((search->minFileSize > 0) ? Text::toT(Util::toString(search->minFileSize)) : Util::emptyStringT);

		group = cur->addChild(GroupBox::Seed(T_("Max FileSize")));
		maxSize = group->addChild(WinUtil::Seeds::Dialog::intTextBox);
		maxSize->setText((search->maxFileSize > 0) ? Text::toT(Util::toString(search->maxFileSize)) : Util::emptyStringT);
	}

	group = grid->addChild(GroupBox::Seed(T_("Size Type")));
	sizeType = group->addChild(WinUtil::Seeds::comboBoxStatic);
	sizeType->addValue(T_("B"));
	sizeType->addValue(T_("KiB"));
	sizeType->addValue(T_("MiB"));
	sizeType->addValue(T_("GiB"));
	sizeType->setSelected(search->typeFileSize);

	group = grid->addChild(GroupBox::Seed(T_("Destination Directory")));
	destDir = group->addChild(WinUtil::Seeds::Dialog::TextBox);
	destDir->setText(Text::toT(search->destDir));

	{
		GridPtr cur = grid->addChild(Grid::Seed(2, 1));

		active = cur->addChild(CheckBox::Seed(T_("Enabled")));
		active->setChecked(search->isActive);

		autoQueue = cur->addChild(CheckBox::Seed(T_("Download Matches")));
		autoQueue->setChecked(search->isAutoQueue);
	}

	{
		ButtonPtr button = grid->addChild(Button::Seed(T_("OK")));
		button->onClicked(std::tr1::bind(&ADLSProperties::handleOKClicked, this));

		button = grid->addChild(Button::Seed(T_("Cancel")));
		button->onClicked(std::tr1::bind(&ADLSProperties::endDialog, this, IDCANCEL));
	}

	setText(T_("ADLSearch Properties"));

	layout();
	centerWindow();
	handleFocus();

	return false;
}

void ADLSProperties::handleFocus() {
	searchString->setFocus();
}

void ADLSProperties::handleOKClicked() {
	search->searchString = Text::fromT(searchString->getText());

	search->sourceType = ADLSearch::SourceType(searchType->getSelected());

	tstring minFileSize = minSize->getText();
	search->minFileSize = minFileSize.empty() ? -1 : Util::toInt64(Text::fromT(minFileSize));

	tstring maxFileSize = maxSize->getText();
	search->maxFileSize = maxFileSize.empty() ? -1 : Util::toInt64(Text::fromT(maxFileSize));

	search->typeFileSize = ADLSearch::SizeType(sizeType->getSelected());

	search->destDir = Text::fromT(destDir->getText());

	search->isActive = active->getChecked();

	search->isAutoQueue = autoQueue->getChecked();

	endDialog(IDOK);
}

void ADLSProperties::layout() {
	dwt::Point sz = getClientSize();
	grid->layout(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}
