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

#include "TabsPage.h"

#include <dwt/widgets/Slider.h>
#include <dwt/util/win32/Version.h>

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

PropPage::ListItem TabsPage::listItems[] = {
	{ SettingsManager::BOLD_HUB, N_("Hub"), IDH_SETTINGS_TABS_BOLD_HUB },
	{ SettingsManager::BOLD_PM, N_("Private message"), IDH_SETTINGS_TABS_BOLD_PM },
	{ SettingsManager::BOLD_FL, N_("File List"), IDH_SETTINGS_TABS_BOLD_FL },
	{ SettingsManager::BOLD_SEARCH, N_("Search"), IDH_SETTINGS_TABS_BOLD_SEARCH },
	{ SettingsManager::BOLD_SEARCH_SPY, N_("Search Spy"), IDH_SETTINGS_TABS_BOLD_SEARCH_SPY },
	{ SettingsManager::BOLD_SYSTEM_LOG, N_("System Log"), IDH_SETTINGS_TABS_BOLD_SYSTEM_LOG },
	{ SettingsManager::BOLD_QUEUE, N_("Download Queue"), IDH_SETTINGS_TABS_BOLD_QUEUE },
	{ SettingsManager::BOLD_FINISHED_DOWNLOADS, N_("Finished Downloads"), IDH_SETTINGS_TABS_BOLD_FINISHED_DOWNLOADS },
	{ SettingsManager::BOLD_WAITING_USERS, N_("Waiting Users"), IDH_SETTINGS_TABS_BOLD_WAITING_USERS },
	{ SettingsManager::BOLD_FINISHED_UPLOADS, N_("Finished Uploads"), IDH_SETTINGS_TABS_BOLD_FINISHED_UPLOADS },
	{ 0, 0 }
};

TabsPage::TabsPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
dcppDraw(0),
buttonStyle(0),
themeGroup(0),
browserTheme(0),
tabWidth(0),
previewGroup(0),
options(0)
{
	setHelpId(IDH_TABSPAGE);

	grid = addChild(Grid::Seed(4, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(3).mode = GridInfo::FILL;
	grid->row(3).align = GridInfo::STRETCH;
	grid->setSpacing(10);

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 3));

		GroupBoxPtr group = cur->addChild(GroupBox::Seed());
		group->setHelpId(IDH_SETTINGS_TABS_DRAW);
		GridPtr cur2 = group->addChild(Grid::Seed(2, 1));
		dcppDraw = cur2->addChild(RadioButton::Seed(T_("Let DC++ draw tabs")));
		dcppDraw->onClicked(std::bind(&TabsPage::createPreview, this));
		dcppDraw->onClicked([&themeGroup]() { themeGroup->setEnabled(true); });
		RadioButtonPtr button = cur2->addChild(RadioButton::Seed(T_("Use standard Windows tabs")));
		button->onClicked(std::bind(&TabsPage::createPreview, this));
		button->onClicked([&themeGroup]() { themeGroup->setEnabled(false); });
		if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_OD)
			dcppDraw->setChecked();
		else
			button->setChecked();

		group = cur->addChild(GroupBox::Seed());
		group->setHelpId(IDH_SETTINGS_TABS_STYLE);
		cur2 = group->addChild(Grid::Seed(2, 1));
		button = cur2->addChild(RadioButton::Seed(T_("Tab style")));
		button->onClicked(std::bind(&TabsPage::createPreview, this));
		buttonStyle = cur2->addChild(RadioButton::Seed(T_("Button style")));
		buttonStyle->onClicked(std::bind(&TabsPage::createPreview, this));
		if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BUTTONS)
			buttonStyle->setChecked();
		else
			button->setChecked();

		themeGroup = cur->addChild(GroupBox::Seed());
		themeGroup->setHelpId(IDH_SETTINGS_TABS_STYLE);
		cur2 = themeGroup->addChild(Grid::Seed(2, 1));
		button = cur2->addChild(RadioButton::Seed(T_("Default theme")));
		button->onClicked(std::bind(&TabsPage::createPreview, this));
		browserTheme = cur2->addChild(RadioButton::Seed(T_("Browser theme")));
		if(dwt::util::win32::ensureVersion(dwt::util::win32::VISTA))
			browserTheme->onClicked(std::bind(&TabsPage::createPreview, this));
		else
			browserTheme->setEnabled(false);
		if(browserTheme->getEnabled() && (SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BROWSER))
			browserTheme->setChecked();
		else
			button->setChecked();
		if(!(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_OD))
			themeGroup->setEnabled(false);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Tab width")));
		group->setHelpId(IDH_SETTINGS_TAB_WIDTH);

		GridPtr cur = group->addChild(Grid::Seed(1, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).size = 30;
		cur->row(0).mode = GridInfo::STATIC;
		cur->row(0).align = GridInfo::STRETCH;

		tabWidth = cur->addChild(Slider::Seed());
		tabWidth->setRange(100, 1000);
		tabWidth->setPosition(SETTING(TAB_WIDTH));
		tabWidth->onScrollHorz(std::bind(&TabsPage::createPreview, this));
	}

	previewGroup = grid->addChild(GroupBox::Seed(T_("Preview")));
	previewGroup->setHelpId(IDH_SETTINGS_TAB_PREVIEW);
	createPreview();

	options = grid->addChild(GroupBox::Seed(T_("Tab highlight on content change")))->addChild(WinUtil::Seeds::Dialog::optionsTable);

	PropPage::read(listItems, options);
}

TabsPage::~TabsPage() {
}

void TabsPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void TabsPage::write() {
	int tabStyle = 0;
	if(dcppDraw->getChecked())
		tabStyle |= SettingsManager::TAB_STYLE_OD;
	if(buttonStyle->getChecked())
		tabStyle |= SettingsManager::TAB_STYLE_BUTTONS;
	if(browserTheme->getChecked())
		tabStyle |= SettingsManager::TAB_STYLE_BROWSER;
	SettingsManager::getInstance()->set(SettingsManager::TAB_STYLE, tabStyle);

	SettingsManager::getInstance()->set(SettingsManager::TAB_WIDTH, tabWidth->getPosition());

	PropPage::write(options);
}

void TabsPage::createPreview() {
	GridPtr cur = previewGroup->addChild(Grid::Seed(1, 1));
	cur->column(0).mode = GridInfo::FILL;
	cur->row(0).size = 100;
	cur->row(0).mode = GridInfo::STATIC;
	cur->row(0).align = GridInfo::STRETCH;

	TabView::Seed seed = WinUtil::Seeds::tabs;
	seed.widthConfig = tabWidth->getPosition();
	seed.style |= WS_DISABLED;
	if(dcppDraw->getChecked()) {
		if(browserTheme->getChecked())
			seed.tabStyle = TabView::Seed::WinBrowser;
	} else {
		seed.style &= ~TCS_OWNERDRAWFIXED;
		seed.widthConfig -= 100; // max width to max chars
	}
	if(buttonStyle->getChecked())
		seed.style |= TCS_BUTTONS;
	TabViewPtr tabs = cur->addChild(seed);

	auto makeTab = [&tabs](const tstring& text) -> ContainerPtr {
		Container::Seed cs;
		cs.caption = text;
		ContainerPtr ret = dwt::WidgetCreator<Container>::create(tabs, cs);
		// the tab control sends WM_ACTIVATE messages; catch them, otherwise the dialog gets messed up.
		ret->onRaw([](WPARAM, LPARAM) { return 0; }, dwt::Message(WM_ACTIVATE));
		return ret;
	};

	tabs->add(makeTab(T_("Example hub tab")), WinUtil::tabIcon(IDI_HUB));
	dwt::IconPtr icon = WinUtil::tabIcon(IDI_DCPP);
	tabs->add(makeTab(T_("Selected tab")), icon);
	ContainerPtr highlighted = makeTab(T_("Highlighted tab"));
	tabs->add(highlighted, icon);
	tabs->add(makeTab(T_("Yet another tab")), icon);
	tabs->setSelected(1);
	tabs->mark(highlighted);

	// refresh
	dwt::Rectangle rect = previewGroup->getWindowRect();
	rect.pos -= grid->getWindowRect().pos; // screen->client coords
	previewGroup->layout(rect);
}
