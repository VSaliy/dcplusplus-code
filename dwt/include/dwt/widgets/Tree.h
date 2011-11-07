/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2011, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
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

#ifndef DWT_Tree_h
#define DWT_Tree_h

#include "../Rectangle.h"
#include "../resources/ImageList.h"
#include "../aspects/Clickable.h"
#include "../aspects/Collection.h"
#include "../aspects/Colorable.h"
#include "../aspects/CustomDraw.h"
#include "../aspects/Data.h"
#include "../aspects/Selection.h"
#include "Control.h"

namespace dwt {

class Tree;

class TreeItem : boost::noncopyable {
public:

private:
	friend class Tree;

	TreeItem(HTREEITEM handle);

	HTREEITEM handle;
};

/// TreeView class
 /** \ingroup WidgetControls
   * \WidgetUsageInfo
   * \image html treeview.PNG
   * A Tree is a treview control, like for instance the documentation to
   * dwt which you are probably reading right now would ( in the web version )
   * have a tree view to the left. <br>
   * Another good example of a tree view is the Explorer of Windows, it has a tree
   * view to the left where you can see the different directories.
   */
class Tree :
	public CommonControl,
	public aspects::Clickable<Tree>,
	public aspects::Collection<Tree, HTREEITEM>,
	public aspects::Colorable<Tree>,
	public aspects::CustomDraw<Tree, NMTVCUSTOMDRAW>,
	public aspects::Data<Tree, HTREEITEM>,
	public aspects::Selection<Tree, HTREEITEM>
{
	typedef CommonControl BaseType;

protected:
	friend class WidgetCreator<Tree>;
	friend class aspects::Collection<Tree, HTREEITEM>;
	friend class aspects::Colorable<Tree>;
	friend class aspects::Data<Tree, HTREEITEM>;
	friend class aspects::Selection<Tree, HTREEITEM>;
	friend class aspects::Clickable<Tree>;

public:
	/// Class type
	typedef Tree ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	/// Seed class
	 /** This class contains all of the values needed to create the widget. It also
	   * knows the type of the class whose seed values it contains. Every widget
	   * should define one of these.
	   */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		FontPtr font;

		/// Fills with default parameters
		Seed();
	};

	/// Inserts a "node" into the TreeView
	/** The return value from a call to this function is a Node. <br>
	  * If you later wish to inserts CHILDREN to that node, pass the return value
	  * from the first call as the second parameter into this function. <br>
	  * If you wish to insert a ( a TreeView can have several "root" nodes ) "root"
	  * node then don't pass anything as the second parameter. ( or pass Node() )
	  * <br>
	  * The "param" parameter ( optionally ) is a unique unsigned integer which must
	  * be higher than 0 and can later be used to retrieve unique identification of
	  * which item was e.g. selected etc... <br>
	  * Especially useful when text of nodes is not unique or text might change.
	  * The "iconIndex" optionally specifies the icon index of the item in the
	  * associated image list, if there is one. <br>
	  * The "selectedIconIndex" optionally specifies the icon index of the item in the
	  * selected state (if not specified or -1, it defaults to the iconIndex)
	  */
	HTREEITEM insert(const tstring& text, HTREEITEM parent = NULL, LPARAM param = 0, bool expanded = false, int iconIndex = - 1, int selectedIconIndex = - 1);

	HTREEITEM getNext(HTREEITEM node, unsigned flag);

	HTREEITEM getChild(HTREEITEM node);

	HTREEITEM getNextSibling(HTREEITEM node);

	using BaseType::getParent;
	HTREEITEM getParent(HTREEITEM node);

	HTREEITEM getRoot();
	HTREEITEM getFirst();
	HTREEITEM getLast();

	int getItemHeight();
	void setItemHeight(int h);

	ScreenCoordinate getContextMenuPos();

	bool isExpanded(HTREEITEM node);
	void expand(HTREEITEM node);
	void collapse(HTREEITEM node);

	void select(const ScreenCoordinate& pt);

	HTREEITEM hitTest(const ScreenCoordinate& pt);

	Rectangle getItemRect(HTREEITEM item);
	/// Deletes just the children of a "node" from the TreeView< br >
	/** Cycles through all the children of node, and deletes them. <br>
	  * The node itself is preserved.
	  */
	void eraseChildren( HTREEITEM node );

	/// Edits the label of the "node"
	/** The label of the node is put to edit modus. The node edited must be visible.
	  */
	void editLabel( HTREEITEM node );

	/// Ensures that the node is visible.
	/** Nodes may not be visible if they are indicated with a + , or do not appear in
	  * the window due to scrolling.
	  */
	void ensureVisible( HTREEITEM node );

	/// Adds a plus/minus sign in front of items.
	/** To add items also at the root node call setLinesAtRoot
	  */
	void setHasButtons( bool value = true );

	/// Adds lines in front of the root item.
	/** Is ignored if you don't also call setHasLines.
	  */
	void setLinesAtRoot( bool value = true );

	/// Adds lines in front of items.
	/** To set lines in front of also the root item call setLinesAtRoot.
	  */
	void setHasLines( bool value = true );

	/// Add Track Selection to the Tree View Control
	/** Pass true to let the Tree View Control go into Track Selction modus which is
	  * quite a neat visual style.
	  */
	void setTrackSelect( bool value = true );

	/// Enables full - row selection in the tree view.
	/** The entire row of the selected item is highlighted, and clicking anywhere on
	  * an item's row causes it to be selected. <br>
	  * This style is mutually exclusive with setHasLines and setLinesAtRoot
	  */
	void setFullRowSelect( bool value = true );

	/// Allows the user to edit the labels of tree - view items.
	/** Note that if the onValidate event handler is defined it will override this
	  * function no matter what
	  */
	void setEditLabels( bool value = true );

	/// Set the normal image list for the Tree View.
	/** normalImageList is the image list that contains the images
	  * for the selected and nonselected item icons.
	  */
	void setNormalImageList( ImageListPtr normalImageList );

	/// Set the state image list for the Tree View.
	/** stateImageList is the image list that contains the images
	  * for the item states.
	  */
	void setStateImageList( ImageListPtr stateImageList );
	/// Returns the text of the current selected node
	/** Returns the text of the current selected node in the tree view.
	  */
	tstring getSelectedText();

	/// Returns the text of a particular node
	/** Returns the text of a particular node.
	  */
	tstring getText( HTREEITEM node );

	/// Returns true if fired, else false
	virtual bool handleMessage( const MSG & msg, LRESULT & retVal );

	/// Actually creates the TreeView
	/** You should call WidgetFactory::createTreeView if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create( const Seed & cs = Seed() );

protected:
	// Constructor Taking pointer to parent
	explicit Tree( Widget * parent );

	// To assure nobody accidentally deletes any heaped object of this type, parent
	// is supposed to do so when parent is killed...
	virtual ~Tree()
	{}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	ImageListPtr itsNormalImageList;
	ImageListPtr itsStateImageList;

	// aspects::Data
	LPARAM getDataImpl(HTREEITEM item);
	void setDataImpl(HTREEITEM item, LPARAM data);

	// aspects::Collection
	void eraseImpl( HTREEITEM node );
	void clearImpl();
	size_t sizeImpl() const;

	// aspects::Colorable
	void setColorImpl(COLORREF text, COLORREF background);

	// aspects::Selection
	HTREEITEM getSelectedImpl() const;
	void setSelectedImpl( HTREEITEM item );
	size_t countSelectedImpl() const;
	static Message getSelectionChangedMessage();

	// aspects::Clickable
	static Message getClickMessage();
	static Message getRightClickMessage();
	static Message getDblClickMessage();
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline HTREEITEM Tree::getNext( HTREEITEM node, unsigned flag ) {
	return TreeView_GetNextItem( handle(), node, flag );
}

inline HTREEITEM Tree::getChild(HTREEITEM node) {
	return TreeView_GetChild(handle(), node);
}

inline HTREEITEM Tree::getNextSibling(HTREEITEM node) {
	return TreeView_GetNextSibling(handle(), node);
}

inline HTREEITEM Tree::getParent(HTREEITEM node) {
	return TreeView_GetParent(handle(), node);
}

inline HTREEITEM Tree::getRoot() {
	return TreeView_GetRoot(handle());
}

inline HTREEITEM Tree::getFirst() {
	return TreeView_GetFirstVisible(handle());
}

inline HTREEITEM Tree::getLast() {
	return TreeView_GetLastVisible(handle());
}

inline void Tree::setColorImpl(COLORREF text, COLORREF background) {
	TreeView_SetTextColor(handle(), text);
	TreeView_SetBkColor(handle(), background);
}

inline HTREEITEM Tree::hitTest(const ScreenCoordinate& pt) {
	ClientCoordinate cc(pt, this);
	TVHITTESTINFO tvhti = { cc.getPoint() };
	return TreeView_HitTest(handle(), &tvhti);
}

inline bool Tree::isExpanded(HTREEITEM node) {
	return TreeView_GetItemState(handle(), node, TVIS_EXPANDED) & TVIS_EXPANDED;
}

inline void Tree::expand(HTREEITEM node) {
	TreeView_Expand(handle(), node, TVE_EXPAND);
}

inline void Tree::collapse(HTREEITEM node) {
	TreeView_Expand(handle(), node, TVE_COLLAPSE);
}

inline void Tree::clearImpl() {
	TreeView_DeleteAllItems( handle() );
}

inline void Tree::eraseImpl( HTREEITEM node ) {
	TreeView_DeleteItem( handle(), node );
}

inline size_t Tree::sizeImpl() const {
	return static_cast<size_t>(TreeView_GetCount(handle()));
}

inline void Tree::editLabel( HTREEITEM node ) {
	static_cast<void>(TreeView_EditLabel( handle(), node ));
}

inline void Tree::ensureVisible( HTREEITEM node ) {
	TreeView_EnsureVisible( handle(), node );
}

inline void Tree::setHasButtons( bool value ) {
	this->Widget::addRemoveStyle( TVS_HASBUTTONS, value );
}

inline void Tree::setLinesAtRoot( bool value ) {
	this->Widget::addRemoveStyle( TVS_LINESATROOT, value );
}

inline void Tree::setHasLines( bool value ) {
	this->Widget::addRemoveStyle( TVS_HASLINES, value );
}

inline void Tree::setTrackSelect( bool value ) {
	this->Widget::addRemoveStyle( TVS_TRACKSELECT, value );
}

inline void Tree::setFullRowSelect( bool value ) {
	this->Widget::addRemoveStyle( TVS_FULLROWSELECT, value );
}

inline void Tree::setEditLabels( bool value ) {
	this->Widget::addRemoveStyle( TVS_EDITLABELS, value );
}

inline Message Tree::getSelectionChangedMessage() {
	return Message(WM_NOTIFY, TVN_SELCHANGED);
}

inline Message Tree::getClickMessage() {
	return Message(WM_NOTIFY, NM_CLICK);
}

inline Message Tree::getRightClickMessage() {
	return Message(WM_NOTIFY, NM_RCLICK);
}

inline Message Tree::getDblClickMessage() {
	return Message(WM_NOTIFY, NM_DBLCLK);
}

inline HTREEITEM Tree::getSelectedImpl() const {
	return TreeView_GetSelection( handle() );
}

inline void Tree::setSelectedImpl(HTREEITEM item) {
	TreeView_SelectItem( handle(), item );
}

inline size_t Tree::countSelectedImpl() const {
	return getSelected() == NULL ? 0 : 1;
}

inline Tree::Tree( Widget * parent )
	: BaseType(parent, ChainingDispatcher::superClass<Tree>())
{
}

}

#endif
