/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2011, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <dwt/widgets/Tree.h>
#include <dwt/widgets/Header.h>
#include <dwt/WidgetCreator.h>

namespace dwt {

const TCHAR Tree::TreeView::windowClass[] = WC_TREEVIEW;

Tree::TreeView::TreeView(Widget* parent) : Control(parent, ChainingDispatcher::superClass<TreeView>()) { }

bool Tree::TreeView::handleMessage(const MSG& msg, LRESULT &retVal) {
	if(BaseType::handleMessage(msg, retVal)) {
		return true;
	}

	if(msg.message == WM_NOTIFY) {
		// Forward tree notifications
		return getParent()->handleMessage(msg, retVal);
	}

	if(msg.message == WM_RBUTTONDOWN) {
		// Tree view control does strange things to rbuttondown, preventing wm_contextmenu from reaching it
		retVal = ::DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
		return true;
	}

	return false;
}

Tree::Seed::Seed() :
	BaseType::Seed(WS_CHILD | WS_TABSTOP | TVS_DISABLEDRAGDROP | TVS_HASLINES | TVS_NONEVENHEIGHT | TVS_SHOWSELALWAYS),
	font(0)
{
}

void Tree::create( const Seed & cs )
{
	Control::Seed mySeed(WS_CHILD, WS_EX_CONTROLPARENT);

	BaseType::create(mySeed);
	tree = WidgetCreator<TreeView>::create(this, cs);

	onSized([&](const SizedEvent& e) { layout(); });

	tree->onCustomDraw([=](NMTVCUSTOMDRAW& x) { return draw(x); });

	setFont(cs.font);
	layout();
}

HTREEITEM Tree::insert(const tstring& text, HTREEITEM parent, LPARAM param, bool expanded, int iconIndex, int selectedIconIndex) {
	TVITEMEX t = { TVIF_TEXT };
	if ( param != 0 )
	{
		t.mask |= TVIF_PARAM;
		t.lParam = param;
	}
	if(expanded) {
		t.mask |= TVIF_STATE;
		t.state = TVIS_EXPANDED;
		t.stateMask = TVIS_EXPANDED;
	}
	if ( itsNormalImageList )
	{
		t.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		t.iImage = ( iconIndex == - 1 ? I_IMAGECALLBACK : iconIndex );
		t.iSelectedImage = ( selectedIconIndex == - 1 ? t.iImage : selectedIconIndex );
	}
	t.pszText = const_cast < TCHAR * >( text.c_str() );

	TVINSERTSTRUCT tv = { parent, TVI_LAST };
#ifdef WINCE
	tv.item = t;
#else
	tv.itemex = t;
#endif
	return TreeView_InsertItem(treeHandle(), &tv);
}

tstring Tree::getSelectedText() {
	return getText(TreeView_GetSelection(treeHandle()));
}

tstring Tree::getText( HTREEITEM node )
{
	if(node == NULL) {
		return tstring();
	}

	TVITEMEX item = { TVIF_HANDLE | TVIF_TEXT, node };
	TCHAR buffer[1024];
	buffer[0] = '\0';
	item.cchTextMax = 1022;
	item.pszText = buffer;
	if ( TreeView_GetItem(treeHandle(), &item) )
	{
		return buffer;
	}
	return tstring();
}

void Tree::eraseChildren( HTREEITEM node )
{
	HTREEITEM next_node, current_node;

	if ( (current_node = getNext( node, TVGN_CHILD ) ) == NULL )
		return;

	while ( (next_node = getNext( current_node, TVGN_NEXT )) != NULL )
	{
		erase( current_node );
		current_node = next_node;
	}

	erase( current_node );
}

void Tree::setNormalImageList( ImageListPtr imageList ) {
	  itsNormalImageList = imageList;
	  TreeView_SetImageList(treeHandle(), imageList->getImageList(), TVSIL_NORMAL);
}

void Tree::setStateImageList( ImageListPtr imageList ) {
	  itsStateImageList = imageList;
	  TreeView_SetImageList(treeHandle(), imageList->getImageList(), TVSIL_STATE);
}

LPARAM Tree::getDataImpl(HTREEITEM item) {
	TVITEM tvitem = { TVIF_PARAM | TVIF_HANDLE, item };
	if(!TreeView_GetItem(treeHandle(), &tvitem)) {
		return 0;
	}
	return tvitem.lParam;
}

void Tree::setDataImpl(HTREEITEM item, LPARAM lParam) {
	TVITEM tvitem = { TVIF_PARAM | TVIF_HANDLE, item };
	tvitem.lParam = lParam;
	TreeView_SetItem(treeHandle(), &tvitem);
}

int Tree::getItemHeight() {
	return TreeView_GetItemHeight(treeHandle());
}

void Tree::setItemHeight(int h) {
	TreeView_SetItemHeight(treeHandle(), h);
}

ScreenCoordinate Tree::getContextMenuPos() {
	HTREEITEM item = getSelected();
	POINT pt = { 0 };
	if(item) {
		RECT trc = getItemRect(item);
		pt.x = trc.left;
		pt.y = trc.top + ((trc.bottom - trc.top) / 2);
	}
	return ClientCoordinate(pt, this);
}

void Tree::select(const ScreenCoordinate& pt) {
	HTREEITEM ht = hitTest(pt);
	if(ht != NULL && ht != getSelected()) {
		setSelected(ht);
	}
}

Rectangle Tree::getItemRect(HTREEITEM item) {
	RECT rc;
	TreeView_GetItemRect(treeHandle(), item, &rc, TRUE);
	return Rectangle(rc);
}

HeaderPtr Tree::getHeader() {
	if(header == NULL) {
		header = WidgetCreator<Header>::create(this);
		layout();
	}

	return header;
}

void Tree::layout() {
	auto client = getClientSize();
	if(header) {
		auto hsize = header->getPreferredSize();
		header->resize(Rectangle(0, 0, client.x, hsize.y));
		tree->resize(Rectangle(0, hsize.y, client.x, client.y - hsize.y));
	} else {
		tree->resize(Rectangle(client));
	}
}

int Tree::insertColumnImpl(const Column& column, int after) {
	auto h = getHeader();

	return h->insert(column.header, column.width, 0, after);
}

void Tree::eraseColumnImpl(unsigned column) {
	if(!header) {
		return;
	}

	header->erase(column);
	if(getColumnCount() == 0) {
		header->close(false);
		header = 0;

		layout();
	}
}

unsigned Tree::getColumnCountImpl() const {
	return header ? header->size() : 0;
}

Column Tree::getColumnImpl(unsigned column) const {
	if(!header) {
		return Column();
	}

	HDITEM item = { HDI_FORMAT | HDI_TEXT | HDI_WIDTH };
	TCHAR buf[1024] = { 0 };
	item.pszText = buf;
	item.cchTextMax = 1023;
	Header_GetItem(header->handle(), column, &item);

	return Column(item.pszText, item.cxy); // TODO fmt
}

std::vector<Column> Tree::getColumnsImpl() const {
	std::vector<Column> ret;
	if(!header) {
		return ret;
	}

	ret.resize(getColumnCount());
	for(size_t i = 0; i < ret.size(); ++i) {
		ret[i] = getColumn(i);
	}

	return ret;
}

std::vector<int> Tree::getColumnOrderImpl() const {
	return std::vector<int>(); // TODO
}

void Tree::setColumnOrderImpl(const std::vector<int>& columns) {
	// TODO
}

std::vector<int> Tree::getColumnWidthsImpl() const {
	return std::vector<int>(); // TODO
}

void Tree::setColumnWidthImpl(unsigned column, int width) {
	// TODO
}

LRESULT Tree::draw(NMTVCUSTOMDRAW& x) {
	if(getColumnCount() < 2) {
		return CDRF_DODEFAULT;
	}

	//TODO
	return CDRF_DODEFAULT;
}

}
