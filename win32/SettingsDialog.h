/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_SETTINGS_DIALOG_H
#define DCPLUSPLUS_WIN32_SETTINGS_DIALOG_H

#include "PropPage.h"
#include "WidgetFactory.h"

class SettingsDialog : public WidgetFactory<SmartWin::WidgetModalDialog>
{
public:
	SettingsDialog(SmartWin::Widget* parent);
	
	int run();
	
	virtual ~SettingsDialog();
	
private:
	typedef std::vector<PropPage*> PageList;
	PageList pages;
	PropPage* currentPage;
	
	WidgetTreeViewPtr pageTree;
	
	void addPage(const tstring& title, PropPage* page);
	void write();
	
	bool initDialog();
	void handleOKClicked();
	void selectionChanged();
	void showPage(PropPage* page);
	
	HTREEITEM createTree(const tstring& str, HTREEITEM parent, PropPage* page);	
	HTREEITEM findItem(const tstring& str, HTREEITEM start);
};

#endif 
