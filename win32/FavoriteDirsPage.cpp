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

#include "FavoriteDirsPage.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>

#include <dcpp/SettingsManager.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>
#include "WinUtil.h"
#include "LineDlg.h"

static const ColumnInfo dirColumns[] = {
	{ N_("Favorite name"), 100, false },
	{ N_("Directory"), 100, false }
};

/** @todo cshelp
static const WinUtil::HelpItem helpItems[] = {
	{ IDC_SETTINGS_FAVORITE_DIRECTORIES, IDH_SETTINGS_FAVORITE_DIRS_FAVORITE_DIRECTORIES },
	{ IDC_FAVORITE_DIRECTORIES, IDH_SETTINGS_FAVORITE_DIRS_FAVORITE_DIRECTORIES },
	{ IDC_RENAME, IDH_SETTINGS_FAVORITE_DIRS_RENAME },
	{ IDC_REMOVE, IDH_SETTINGS_FAVORITE_DIRS_REMOVE },
	{ IDC_ADD, IDH_SETTINGS_FAVORITE_DIRS_ADD },
	{ 0, 0 }
};
*/

FavoriteDirsPage::FavoriteDirsPage(dwt::Widget* parent) : PropPage(parent) {
	createDialog(IDD_FAVORITE_DIRSPAGE);
	setHelpId(IDH_FAVORITE_DIRSPAGE);

	group = addChild(GroupBox::Seed(T_("Favorite download directories")));

	GridPtr grid = group->addChild(Grid::Seed(2, 3));
	grid->column(0).mode = dwt::GridInfo::FILL;
	grid->column(1).mode = dwt::GridInfo::FILL;
	grid->column(2).mode = dwt::GridInfo::FILL;
	grid->row(0).mode = dwt::GridInfo::FILL;
	grid->row(0).align = dwt::GridInfo::STRETCH;

	directories = grid->addChild(WinUtil::Seeds::Dialog::Table);
	grid->setWidget(directories, 0, 0, 1, 3);

	rename = grid->addChild(Button::Seed(T_("Re&name")));
	rename->onClicked(std::tr1::bind(&FavoriteDirsPage::handleRenameClicked, this));
	remove = grid->addChild(Button::Seed(T_("&Remove")));
	remove->onClicked(std::tr1::bind(&FavoriteDirsPage::handleRemoveClicked, this));
	grid->addChild(Button::Seed(T_("&Add folder")))->onClicked(std::tr1::bind(&FavoriteDirsPage::handleAddClicked, this));

	WinUtil::makeColumns(directories, dirColumns, 2, "", "");

	StringPairList dirs = FavoriteManager::getInstance()->getFavoriteDirs();
	for(StringPairIter j = dirs.begin(); j != dirs.end(); j++) {
		TStringList row;
		row.push_back(Text::toT(j->second));
		row.push_back(Text::toT(j->first));
		directories->insert(row);
	}

	directories->onDblClicked(std::tr1::bind(&FavoriteDirsPage::handleDoubleClick, this));
	directories->onKeyDown(std::tr1::bind(&FavoriteDirsPage::handleKeyDown, this, _1));
	directories->onSelectionChanged(std::tr1::bind(&FavoriteDirsPage::handleSelectionChanged, this));

	onDragDrop(std::tr1::bind(&FavoriteDirsPage::handleDragDrop, this, _1));
}

FavoriteDirsPage::~FavoriteDirsPage() {
}

void FavoriteDirsPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientAreaSize();
	group->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));
}

void FavoriteDirsPage::handleDoubleClick() {
	if(directories->hasSelected()) {
		handleRenameClicked();
	} else {
		handleAddClicked();
	}
}

bool FavoriteDirsPage::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddClicked();
		return true;
	case VK_DELETE:
		handleRemoveClicked();
		return true;
	}
	return false;
}

void FavoriteDirsPage::handleSelectionChanged() {
	bool enable = directories->hasSelected();
	rename->setEnabled(enable);
	remove->setEnabled(enable);
}

void FavoriteDirsPage::handleDragDrop(const TStringList& files) {
	for(TStringIterC i = files.begin(); i != files.end(); ++i)
		if(PathIsDirectory(i->c_str()))
			addDirectory(*i);
}

void FavoriteDirsPage::handleRenameClicked() {
	int i = -1;
	while((i = directories->getNext(i, LVNI_SELECTED)) != -1) {
		tstring old = directories->getText(i, 0);
		LineDlg dlg(this, T_("Favorite name"), T_("Under what name you see the directory"), old);
		if(dlg.run() == IDOK) {
			tstring line = dlg.getLine();
			if (FavoriteManager::getInstance()->renameFavoriteDir(Text::fromT(old), Text::fromT(line))) {
				directories->setText(i, 0, line);
			} else {
				createMessageBox().show(T_("Directory or directory name already exists"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONSTOP);
			}
		}
	}
}

void FavoriteDirsPage::handleRemoveClicked() {
	int i = -1;
	while((i = directories->getNext(-1, LVNI_SELECTED)) != -1)
		if(FavoriteManager::getInstance()->removeFavoriteDir(Text::fromT(directories->getText(i, 1))))
			directories->erase(i);
}

void FavoriteDirsPage::handleAddClicked() {
	tstring target;
	if(createFolderDialog().open(target)) {
		addDirectory(target);
	}
}

void FavoriteDirsPage::addDirectory(const tstring& aPath) {
	tstring path = aPath;
	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	LineDlg dlg(this, T_("Favorite name"), T_("Under what name you see the directory"), Util::getLastDir(path));
	if(dlg.run() == IDOK) {
		tstring line = dlg.getLine();
		if (FavoriteManager::getInstance()->addFavoriteDir(Text::fromT(path), Text::fromT(line))) {
			TStringList row;
			row.push_back(line);
			row.push_back(path);
			directories->insert(row);
		} else {
			createMessageBox().show(T_("Directory or directory name already exists"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MessageBox::BOX_OK, MessageBox::BOX_ICONSTOP);
		}
	}
}
