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
#include "UserMatchDlg.h"

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/version.h>

#include <dwt/widgets/Button.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Button;
using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

UserMatchDlg::UserMatchDlg(dwt::Widget* parent, const UserMatch* initialMatcher) :
GridDialog(parent, 500, DS_CONTEXTHELP),
name(0),
favs(0),
ops(0),
bots(0),
rules(0),
noChat(0)
{
	onInitDialog([this, initialMatcher] { return handleInitDialog(initialMatcher); });
	onHelp(&WinUtil::help);
}

UserMatchDlg::~UserMatchDlg() {
}

bool UserMatchDlg::handleInitDialog(const UserMatch* initialMatcher) {
	setHelpId(IDH_USER_MATCH);

	grid = addChild(Grid::Seed(5, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->setSpacing(8);

	name = grid->addChild(GroupBox::Seed(T_("Name")))->addChild(WinUtil::Seeds::Dialog::textBox);

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(1, 3));
		cur->setSpacing(grid->getSpacing());

		favs = cur->addChild(CheckBox::Seed(T_("Match favorite users")));
		ops = cur->addChild(CheckBox::Seed(T_("Match operators")));
		bots = cur->addChild(CheckBox::Seed(T_("Match bots")));
	}

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		rules = cur->addChild(Grid::Seed(0, 4));
		rules->column(1).mode = GridInfo::FILL;

		auto button = cur->addChild(Grid::Seed(1, 1))->addChild(Button::Seed(T_("Add a rule")));
		button->onClicked([this] { addRow(); layoutRules(); });
	}

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(1, 1));
		cur->setSpacing(grid->getSpacing());

		noChat = cur->addChild(CheckBox::Seed(T_("Ignore chat messages")));
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 3));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(grid->getSpacing());

		WinUtil::addDlgButtons(cur,
			[this] { handleOKClicked(); },
			[this] { endDialog(IDCANCEL); });

		WinUtil::addHelpButton(cur)->onClicked([this] { WinUtil::help(this, IDH_USER_MATCH); });
	}

	if(initialMatcher) {
		name->setText(Text::toT(initialMatcher->name));

		if(initialMatcher->isSet(UserMatch::FAVS)) { favs->setChecked(true); }
		if(initialMatcher->isSet(UserMatch::OPS)) { ops->setChecked(true); }
		if(initialMatcher->isSet(UserMatch::BOTS)) { bots->setChecked(true); }

		for(auto i = initialMatcher->rules.cbegin(), iend = initialMatcher->rules.cend(); i != iend; ++i) {
			addRow(&*i);
		}

		if(initialMatcher->props->noChat) { noChat->setChecked(true); }

		result.props = initialMatcher->props;
	}

	setText(T_("User matching definition"));

	layout();
	centerWindow();

	return false;
}

void UserMatchDlg::handleOKClicked() {
	result.name = Text::fromT(name->getText());

	if(result.name.empty()) {
		dwt::MessageBox(this).show(T_("Specify a name for this user matching definition"),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return;
	}

	if(favs->getChecked()) { result.setFlag(UserMatch::FAVS); }
	if(ops->getChecked()) { result.setFlag(UserMatch::OPS); }
	if(bots->getChecked()) { result.setFlag(UserMatch::BOTS); }

	auto controls = rules->getChildren<Control>();
	int8_t counter = -1;
	std::unique_ptr<UserMatch::Rule> rule;
	boost::for_each(controls, [this, &counter, &rule](Control* control) {
		enum { RuleField, RuleSearch, RuleRegex, RuleRemove };
		switch(++counter) {
		case RuleField:
			{
				rule.reset(new UserMatch::Rule());
				rule->field = static_cast<decltype(rule->field)>(static_cast<ComboBoxPtr>(control)->getSelected());
				break;
			}
		case RuleSearch:
			{
				rule->pattern = Text::fromT(static_cast<TextBoxPtr>(control)->getText());
				break;
			}
		case RuleRegex:
			{
				if(static_cast<CheckBoxPtr>(control)->getChecked())
					rule->setRegEx(true);
				break;
			}
		case RuleRemove:
			{
				this->result.addRule(std::move(*rule));
				counter = -1;
				break;
			}
		}
	});

	if(result.empty()) {
		dwt::MessageBox(this).show(T_("This user matching definition won't match any user; specify some rules"),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return;
	}

	if(!result.props) {
		result.props = new UserMatchProps();
		result.props->textColor = -1;
		result.props->bgColor = -1;
	}
	result.props->noChat = noChat->getChecked();

	endDialog(IDOK);
}

void UserMatchDlg::layoutRules() {
	rules->layout();
	layout();
	centerWindow();
}

void UserMatchDlg::addRow(const UserMatch::Rule* rule) {
	rules->addRow();

	auto field = rules->addChild(WinUtil::Seeds::Dialog::comboBox);

	auto search = rules->addChild(WinUtil::Seeds::Dialog::textBox);

	auto regex = rules->addChild(CheckBox::Seed(_T("RE")));

	{
		auto seed = Button::Seed(_T("X"));
		seed.padding.x = 10;
		auto remove = rules->addChild(seed);
		remove->onClicked([this, remove] { callAsync([=] {
			rules->removeRow(remove);
			layoutRules();
		}); });
	}

	tstring fields[UserMatch::Rule::FIELD_LAST] = { T_("Nick"), T_("CID"), T_("IP") };
	std::for_each(fields, fields + UserMatch::Rule::FIELD_LAST, [field](const tstring& str) { field->addValue(str); });
	field->setSelected(rule ? rule->field : 0);

	if(rule) {
		search->setText(Text::toT(rule->pattern));
		if(rule->isRegEx()) { regex->setChecked(true); }
	}
}
