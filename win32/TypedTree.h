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

#ifndef DCPLUSPLUS_WIN32_TYPED_TREE_H
#define DCPLUSPLUS_WIN32_TYPED_TREE_H

#include "forward.h"
#include "WinUtil.h"

/** Tree with an object associated to each item.

@tparam ContentType Type of the objects associated to each item.

@tparam managed Whether this class should handle deleting associated objects. */
template<typename ContentType, bool managed>
class TypedTree : public dwt::Tree
{
	typedef typename dwt::Tree BaseType;
	typedef TypedTree<ContentType, managed> ThisType;

public:
	typedef ThisType* ObjectType;

	explicit TypedTree( dwt::Widget* parent ) : BaseType(parent) { }

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(const BaseType::Seed& seed) : BaseType::Seed(seed) {
		}
	};

	virtual ~TypedTree() {
	}

	void create(const Seed& seed) {
		BaseType::create(seed);

		this->onRaw([this](WPARAM, LPARAM lParam) -> LRESULT {
			auto& data = *reinterpret_cast<NMTVDISPINFO*>(lParam);
			this->handleDisplay(data);
			return 0;
		}, dwt::Message(WM_NOTIFY, TVN_GETDISPINFO));

		if(managed) {
			onDestroy([this] { this->clear(); });
		}
	}

	HTREEITEM insert(HTREEITEM parent, ContentType* data, bool expanded = false) {
		TVINSERTSTRUCT item = { parent, TVI_SORT, { { TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT } } };
		if(expanded) {
			item.itemex.mask |= TVIF_STATE;
			item.itemex.state = TVIS_EXPANDED;
			item.itemex.stateMask = TVIS_EXPANDED;
		}
		item.itemex.pszText = LPSTR_TEXTCALLBACK;
		item.itemex.iImage = I_IMAGECALLBACK;
		item.itemex.iSelectedImage = I_IMAGECALLBACK;
		item.itemex.lParam = reinterpret_cast<LPARAM>(data);
		return this->insert(&item);
	}

	HTREEITEM insert(TVINSERTSTRUCT* tvis) {
		return TreeView_InsertItem(this->treeHandle(), tvis);
	}

	ContentType* getData(HTREEITEM item) {
		return reinterpret_cast<ContentType*>(BaseType::getData(item));
	}

	void getItem(TVITEMEX* item) {
		TreeView_GetItem(this->treeHandle(), item);
	}

	ContentType* getSelectedData() {
		HTREEITEM item = this->getSelected();
		return item == NULL ? 0 : getData(item);
	}

	void setItemState(HTREEITEM item, int state, int mask) {
		TreeView_SetItemState(this->treeHandle(), item, state, mask);
	}

	void clear() {
		if(managed) {
			auto item = this->getRoot();
			while(item) {
				this->clear(item);
				item = this->getNextSibling(item);
			}
		}

		this->BaseType::clear();
	}

	void erase(HTREEITEM item) { if(managed) delete getData(item); this->BaseType::erase(item); }

private:
	void handleDisplay(NMTVDISPINFO& data) {
		if(data.item.mask & TVIF_TEXT) {
			ContentType* content = reinterpret_cast<ContentType*>(data.item.lParam);
			const tstring& text = content->getText();
			_tcsncpy(data.item.pszText, text.data(), std::min(text.size(), static_cast<size_t>(data.item.cchTextMax)));
			if(text.size() < static_cast<size_t>(data.item.cchTextMax)) {
				data.item.pszText[text.size()] = 0;
			}
		}
		if(data.item.mask & TVIF_IMAGE) {
			ContentType* content = reinterpret_cast<ContentType*>(data.item.lParam);
			data.item.iImage = content->getImage();
		}
		if(data.item.mask & TVIF_SELECTEDIMAGE) {
			ContentType* content = reinterpret_cast<ContentType*>(data.item.lParam);
			data.item.iSelectedImage = content->getSelectedImage();
		}
	}

	void clear(HTREEITEM item) {
		auto next = this->getChild(item);
		while(next) {
			clear(next);
			next = this->getNextSibling(next);
		}
		delete this->getData(item);
	}
};

#endif
