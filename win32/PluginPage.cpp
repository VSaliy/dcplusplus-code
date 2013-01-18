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
#include "PluginPage.h"

#include "HoldRedraw.h"
#include "resource.h"
#include "WinUtil.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Link.h>
#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/MessageBox.h>

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/ScopedFunctor.h>

#include <dcpp/format.h>
#include <dcpp/version.h>

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Link;
using dwt::LoadDialog;

static const ColumnInfo columns[] = {
	{ "", 100, false },
};

PluginPage::PluginPage(dwt::Widget* parent) :
PropPage(parent, 3, 1),
plugins(0),
add(0),
configure(0),
moveUp(0),
moveDown(0),
remove(0),
pluginInfo(0)
{

	setHelpId(IDH_PLUGINPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;
	grid->setSpacing(6);
	{

		{
			auto group = grid->addChild(GroupBox::Seed(T_("Installed Plugins")));

			auto cur = group->addChild(Grid::Seed(2, 1));
			cur->column(0).mode = GridInfo::FILL;
			cur->row(0).mode = GridInfo::FILL;
			cur->row(0).align = GridInfo::STRETCH;
			cur->setHelpId(IDH_SETTINGS_PLUGINS_LIST);
			cur->setSpacing(6);
	
			auto seed = WinUtil::Seeds::Dialog::table;
			seed.style |= LVS_SINGLESEL | LVS_NOCOLUMNHEADER;
			plugins = cur->addChild(seed);

			plugins->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleContextMenu(sc); });	

			{
				auto buttons = cur->addChild(Grid::Seed(1, 5));	
				buttons->column(0).mode = GridInfo::FILL;
				buttons->column(0).align = GridInfo::STRETCH;
				buttons->column(1).mode = GridInfo::FILL;
				buttons->column(1).align = GridInfo::STRETCH;
				buttons->column(2).mode = GridInfo::FILL;
				buttons->column(2).align = GridInfo::STRETCH;
				buttons->column(3).mode = GridInfo::FILL;
				buttons->column(3).align = GridInfo::STRETCH;
				buttons->column(4).mode = GridInfo::FILL;
				buttons->column(4).align = GridInfo::STRETCH;

				add = buttons->addChild(Button::Seed(T_("&Add")));
				add->onClicked([this] { handleAddPlugin(); });
				add->setImage(WinUtil::buttonIcon(IDI_OK));
				add->setHelpId(IDH_SETTINGS_PLUGINS_ADD);

				configure = buttons->addChild(Button::Seed(T_("&Configure")));
				configure->onClicked([this] { handleConfigurePlugin(); });
				configure->setImage(WinUtil::buttonIcon(IDI_SETTINGS));
				configure->setHelpId(IDH_SETTINGS_PLUGINS_CONFIGURE);

				moveUp = buttons->addChild(Button::Seed(T_("Move &up")));
				moveUp->onClicked([this] { handleMovePluginUp(); });
				moveUp->setImage(WinUtil::buttonIcon(IDI_UPLOAD));
				moveUp->setHelpId(IDH_SETTINGS_PLUGINS_MOVE_UP);

				moveDown = buttons->addChild(Button::Seed(T_("Move &down")));
				moveDown->onClicked([this] { handleMovePluginDown(); });
				moveDown->setImage(WinUtil::buttonIcon(IDI_DOWNLOAD));
				moveDown->setHelpId(IDH_SETTINGS_PLUGINS_MOVE_DOWN);

				remove = buttons->addChild(Button::Seed(T_("&Remove")));
				remove->onClicked([this] { handleRemovePlugin(); });
				remove->setImage(WinUtil::buttonIcon(IDI_CANCEL));
				remove->setHelpId(IDH_SETTINGS_PLUGINS_REMOVE);
			}
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Plugin information")));
		group->setHelpId(IDH_SETTINGS_PLUGINS_INFO);
		pluginInfo = group->addChild(Grid::Seed(1, 1));
		pluginInfo->column(0).mode = GridInfo::FILL;
		pluginInfo->setSpacing(grid->getSpacing());
	}

		grid->addChild(Label::Seed(str(TF_("Some plugins may require you to restart %1%") % Text::toT(APPNAME))));
	}

	WinUtil::makeColumns(plugins, columns, 1);

	for(auto& plugin: PluginManager::getInstance()->getPluginList()) {
		addEntry(plugins->size(), plugin->getInfo());
	}

	handleSelectionChanged();

	plugins->onDblClicked([this] { handleDoubleClick(); });
	plugins->onKeyDown([this] (int c) { return handleKeyDown(c); });
	plugins->onSelectionChanged([this] { handleSelectionChanged(); });
}

PluginPage::~PluginPage() {
}

void PluginPage::layout() {
	PropPage::layout();

	plugins->setColumnWidth(0, plugins->getWindowSize().x - 20);
}

void PluginPage::handleDoubleClick() {
	if(plugins->hasSelected()) {
		handleConfigurePlugin();
	}
}

bool PluginPage::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddPlugin();
		return true;
	case VK_DELETE:
		handleRemovePlugin();
		return true;
	}
	return false;
}

void PluginPage::handleSelectionChanged() {
	auto enable = plugins->hasSelected();
	configure->setEnabled(enable);
	moveUp->setEnabled(enable);
	moveDown->setEnabled(enable);
	remove->setEnabled(enable);

	ScopedFunctor([&] { pluginInfo->layout(); pluginInfo->redraw(); });

	HoldRedraw hold(pluginInfo);

	pluginInfo->clearRows();

	/* destroy previous children. store them in a vector beforehand or the enumeration will fail
	(since they're getting destroyed)... */
	auto children = pluginInfo->getChildren<Control>();
	boost::for_each(std::vector<Control*>(children.first, children.second), [](Control* w) { w->close(); });

	if(plugins->countSelected() != 1) {
		pluginInfo->addRow(GridInfo(0, GridInfo::FILL, GridInfo::STRETCH));
		pluginInfo->addChild(Label::Seed(T_("No plugin has been selected")));
		return;
	}

	pluginInfo->addRow(GridInfo(0, GridInfo::FILL, GridInfo::STRETCH));
	auto infoGrid = pluginInfo->addChild(Grid::Seed(0, 2));
	infoGrid->column(1).mode = GridInfo::FILL;
	infoGrid->setSpacing(pluginInfo->getSpacing());

	enum Type { Name, Version, Description, Author, Website };

	auto addInfo = [this, infoGrid](tstring name, const string& value, Type type) {
		if(type == Description) {
			infoGrid->addRow(GridInfo(0, GridInfo::FILL, GridInfo::STRETCH));
		} else {
			infoGrid->addRow();
		}
		infoGrid->addChild(Label::Seed(name));
		if(type == Website && !value.empty()) {
			infoGrid->addChild(Link::Seed(Text::toT(value), true));
		} else {
			infoGrid->addChild(Label::Seed(value.empty() ?
				T_("<Information unavailable>") : Text::toT(value)));
		}
	};

	auto info = reinterpret_cast<MetaData*>(plugins->getData(plugins->getSelected()));

	addInfo(T_("Name: "), info->name, Name);
	addInfo(T_("Version: "), Util::toString(info->version), Version);
	addInfo(T_("Description: "), info->description, Description);
	addInfo(T_("Author: "), info->author, Author);
	addInfo(T_("Website: "), info->web, Website);
}

bool PluginPage::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(plugins->countSelected() > 0) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = plugins->getContextMenuPos();
		}
		MenuPtr contextMenu = makeMenu();
		contextMenu->open(pt);
		return true;
	}
	return false;
}

MenuPtr PluginPage::makeMenu() {
	MenuPtr menu = addChild(WinUtil::Seeds::menu);
	
	menu->setTitle(T_("Plugin options"), WinUtil::menuIcon(IDI_PLUGINS));
	menu->appendItem(T_("Add plugin"), [this] { handleAddPlugin(); }, WinUtil::menuIcon(IDI_OK));
	menu->appendItem(T_("Configure plugin"), [this] { handleConfigurePlugin(); }, WinUtil::menuIcon(IDI_SETTINGS));
	menu->appendSeparator();
	menu->appendItem(T_("Move plugin up"), [this] { handleMovePluginUp(); }, WinUtil::menuIcon(IDI_UPLOAD));
	menu->appendItem(T_("Move plugin down"), [this] { handleMovePluginDown(); }, WinUtil::menuIcon(IDI_DOWNLOAD));
	menu->appendSeparator();
	menu->appendItem(T_("Remove plugin"), [this] { handleRemovePlugin(); }, WinUtil::menuIcon(IDI_CANCEL));
	
	return menu;
}

void PluginPage::handleAddPlugin() {
	tstring path;
	if(LoadDialog(this).addFilter(T_("DLL files"), _T("*.dll"))
		.setInitialDirectory(Text::toT(Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Plugins")).open(path))
	{
		auto idx = plugins->size();
		if(PluginManager::getInstance()->loadPlugin(Text::fromT(path), [this, &path](const string& str) {
			dwt::MessageBox(this).show(Text::toT(str), Text::toT(Util::getFileName(Text::fromT(path))),
				dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
		}, true)) {
			addEntry(idx, PluginManager::getInstance()->getPlugin(idx)->getInfo());
		}
	}
}

void PluginPage::handleConfigurePlugin() {
	auto sel = plugins->getSelected();
	const PluginInfo *p = PluginManager::getInstance()->getPlugin(sel);
	if(!p->dcMain(ON_CONFIGURE, PluginManager::getInstance()->getCore(), this->handle())) {
		dwt::MessageBox(this).show(
			str(TF_("%1% doesn't need any additional configuration") % Text::toT(p->getInfo().name)),
			Text::toT(p->getInfo().name), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONINFORMATION);
	}
}

void PluginPage::handleMovePluginUp() {
	if(plugins->countSelected() != 1)
		return;

	auto idx = plugins->getSelected();
	if(idx == 0)
		return;
	HoldRedraw hold(plugins);
	PluginManager::getInstance()->movePlugin(idx, -1);
	plugins->erase(idx);
	idx -=1;
	addEntry(idx, PluginManager::getInstance()->getPlugin(idx)->getInfo());
	plugins->setSelected(idx);
	plugins->ensureVisible(idx);
}

void PluginPage::handleMovePluginDown() {
	if(plugins->countSelected() != 1)
		return;

	auto idx = plugins->getSelected();
	if(static_cast<uint32_t>(idx) == plugins->size() - 1)
		return;
	HoldRedraw hold(plugins);
	PluginManager::getInstance()->movePlugin(idx, 1);
	plugins->erase(idx);
	idx +=1;
	addEntry(idx, PluginManager::getInstance()->getPlugin(idx)->getInfo());
	plugins->setSelected(idx);
	plugins->ensureVisible(idx);
}

void PluginPage::handleRemovePlugin() {
	auto sel = plugins->getSelected();
	PluginManager::getInstance()->unloadPlugin(sel);
	plugins->erase(sel);
}

void PluginPage::addEntry(size_t idx, const MetaData& info) {
	TStringList row;
	row.push_back(Text::toT(info.name));
	plugins->insert(row, (LPARAM)&info, idx);
}
