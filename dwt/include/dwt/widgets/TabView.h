/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2010, Jacek Sieka

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

#ifndef DWT_WIDGETTABVIEW_H_
#define DWT_WIDGETTABVIEW_H_

#include "../resources/ImageList.h"
#include "../Rectangle.h"
#include "../aspects/AspectCollection.h"
#include "../aspects/AspectPainting.h"
#include "../aspects/AspectSelection.h"
#include "../aspects/AspectText.h"
#include "Control.h"
#include <dwt/Theme.h>

#include <list>
#include <vector>

namespace dwt {
/**
 * A container that keeps widgets in tabs and handles switching etc
 */
class TabView :
	public CommonControl,
	// Aspects
	private AspectCollection<TabView, int>,
	public AspectPainting< TabView >,
	public AspectSelection< TabView, int >,
	public AspectText< TabView >
{
	typedef CommonControl BaseType;
	friend class AspectCollection<TabView, int>;
	friend class AspectSelection<TabView, int>;
	friend class WidgetCreator< TabView >;
	typedef std::function<void (const tstring&)> TitleChangedFunction;
	typedef std::function<bool (const ScreenCoordinate&)> ContextMenuFunction;

public:
	/// Class type
	typedef TabView ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		enum {
			WinDefault, /// use the default Windows style.

			WinBrowser /// use the "Browser" Windows style, only available on Win >= Vista.
		} tabStyle;

		FontPtr font;

		/** for owner-drawn tabs (that have the TCS_OWNERDRAWFIXED style), defines the width of
		each tab (min 100 pixels). otherwise, sets the maximum number of characters per tab (in
		that case, any value <= 3 means infinite). */
		unsigned widthConfig;

		bool toggleActive; /// switch the active tab when clicking on the current active tab
		bool ctrlTab; /// handle Ctrl+Tab and Ctrl+Shift+Tab

		/// Fills with default parameters
		Seed(unsigned widthConfig_ = 150, bool toggleActive_ = false, bool ctrlTab_ = false);
	};

	void add(ContainerPtr w, const IconPtr& icon = IconPtr());

	void mark(ContainerPtr w);

	void remove(ContainerPtr w);

	void next(bool reverse = false);

	ContainerPtr getActive() const;
	void setActive(ContainerPtr w) { setActive(findTab(w)); }

	IconPtr getIcon(ContainerPtr w) const;
	void setIcon(ContainerPtr w, const IconPtr& icon);

	void onTitleChanged(const TitleChangedFunction& f) {
		titleChangedFunction = f;
	}

	void onTabContextMenu(ContainerPtr w, const ContextMenuFunction& f);

	bool filter(const MSG& msg);

	const Rectangle& getClientSize() const { return clientSize; }

	typedef std::vector<ContainerPtr> ChildList;
	const ChildList getChildren() const;

	void create( const Seed & cs = Seed() );

	virtual bool handleMessage( const MSG & msg, LRESULT & retVal );

	using Widget::layout;

	// tab controls only use WM_DRAWITEM
	static bool handlePainting(LPDRAWITEMSTRUCT t) {
		TabInfo* ti = reinterpret_cast<TabInfo*>(t->itemData);
		return ti->control->handlePainting(t, ti);
	}
	static bool handlePainting(LPMEASUREITEMSTRUCT) { return false; }

protected:
	explicit TabView(Widget* parent);

	virtual ~TabView() { }

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	struct TabInfo {
		TabView* control; // for painting messages
		ContainerPtr w;
		tstring text;
		ContextMenuFunction handleContextMenu;
		bool marked;
		TabInfo(TabView* control_, ContainerPtr w_) : control(control_), w(w_), handleContextMenu(0), marked(false) { }
	};

	Theme theme;
	Theme windowTheme; // to draw the close button
	ToolTipPtr tip;

	TitleChangedFunction titleChangedFunction;

	// these can be set through the Seed
	unsigned widthConfig;
	bool toggleActive;
	FontPtr font;
	FontPtr boldFont;

	bool inTab;
	int highlighted;
	bool highlightClose;
	bool closeAuthorized;

	typedef std::list<ContainerPtr> WindowList;
	typedef WindowList::iterator WindowIter;
	WindowList viewOrder;
	Rectangle clientSize;
	ImageListPtr imageList;
	std::vector<IconPtr> icons;
	int active;
	ContainerPtr dragging;
	tstring tipText;
	Rectangle closeRect;

	int findTab(ContainerPtr w) const;

	void setActive(int i);
	TabInfo* getTabInfo(ContainerPtr w) const;
	TabInfo* getTabInfo(int i) const;

	void setTop(ContainerPtr w);

	void handleCtrlTab(bool shift);
	void handleTextChanging(ContainerPtr w, const tstring& newText);
	void handleTabSelected();
	LRESULT handleToolTip(LPARAM lParam);
	bool handleLeftMouseDown(const MouseEvent& mouseEvent);
	bool handleLeftMouseUp(const MouseEvent& mouseEvent);
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	bool handleMiddleMouseDown(const MouseEvent& mouseEvent);
	bool handleXMouseUp(const MouseEvent& mouseEvent);
	bool handleMouseMove(const MouseEvent& mouseEvent);
	void handleMouseLeave();
	bool handlePainting(LPDRAWITEMSTRUCT info, TabInfo* ti);
	void handlePainting(PaintCanvas& canvas);

	tstring formatTitle(tstring title);
	void layout();

	int addIcon(const IconPtr& icon);
	void swapWidgets(ContainerPtr oldW, ContainerPtr newW);

	IconPtr getIcon(unsigned index) const;
	void setText(unsigned idx, const tstring& text);
	void redraw(unsigned index);
	void draw(Canvas& canvas, unsigned index, Rectangle&& rect, bool isSelected);
	bool inCloseRect(const ScreenCoordinate& pos) const;

	// AspectCollection
	void eraseImpl( int row );
	void clearImpl();
	size_t sizeImpl() const;

	// AspectHelp
	void helpImpl(unsigned& id);

	// AspectSelection
	int getSelectedImpl() const;
	void setSelectedImpl( int idx );
	// AspectSelection expectation implementation
	static Message getSelectionChangedMessage();

	const ImageListPtr& getImageList() const;

	int hitTest(const ScreenCoordinate& pt);

	/// Get the area not used by the tabs
	/** This function should be used after adding the pages, so that the area not used by
	  * the tabs can be calculated accurately. It returns coordinates respect to the
	  * TabControl, this is, you have to adjust for the position of the control itself.
	  */
	Rectangle getUsableArea(bool cutBorders = false) const;

};

inline Message TabView::getSelectionChangedMessage() {
	return Message( WM_NOTIFY, TCN_SELCHANGE );
}

inline void TabView::eraseImpl(int i) {
	TabCtrl_DeleteItem(this->handle(), i);
}

inline void TabView::clearImpl() {
	TabCtrl_DeleteAllItems(handle());
}

inline size_t TabView::sizeImpl() const {
	return static_cast<size_t>(TabCtrl_GetItemCount(this->handle()));
}

inline void TabView::setSelectedImpl( int idx ) {
	TabCtrl_SetCurSel( this->handle(), idx );
}

inline int TabView::getSelectedImpl() const {
	return TabCtrl_GetCurSel( this->handle() );
}

}
#endif /*WIDGETTABVIEW_H_*/
