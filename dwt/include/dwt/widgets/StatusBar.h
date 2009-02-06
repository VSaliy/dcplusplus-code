/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2008, Jacek Sieka

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

#ifndef DWT_StatusBar_h
#define DWT_StatusBar_h

#include "../aspects/AspectClickable.h"
#include "../aspects/AspectDblClickable.h"
#include "../aspects/AspectFont.h"
#include "../aspects/AspectPainting.h"
#include "Control.h"

#include <vector>

namespace dwt {

/// StatusBar class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html status.PNG
  * Class for creating a Status Bar Control. <br>
  * A status bar is a status info line normally residing in the bottom of a
  * Window or other container type of Widget. <br>
  * You can then send text to that window to show e.g. "status info" regarding the
  * Window which owns the Status Bar Control. A good example of an application
  * with a status bar is for instance Internet Explorer which ( unless you have made
  * it invisible ) has a strip of status information at the bottom showing for
  * instance the security settings of the current page and how far in the download
  * process you are currently etc...
  */
class StatusBar :
	public CommonControl,

	// Aspects
	public AspectClickable<StatusBar>,
	public AspectDblClickable<StatusBar>,
	public AspectFont<StatusBar>,
	public AspectPainting<StatusBar>
{
	typedef CommonControl BaseType;
	typedef AspectClickable<StatusBar> ClickType;
	typedef AspectDblClickable<StatusBar> DblClickType;

	friend class WidgetCreator<StatusBar>;
	friend class AspectClickable<StatusBar>;
	friend class AspectDblClickable<StatusBar>;

	typedef Dispatchers::VoidVoid<>::F F;

public:
	/// Class type
	typedef StatusBar ThisType;

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

		unsigned parts;

		/// index of the part that fills all the remaining space of the bar.
		unsigned fill;

		/// associate a tooltip to the part marked by the "fill" parameter.
		bool tooltip;

		/// Fills with default parameters
		explicit Seed(unsigned parts_ = 1, unsigned fill_ = 0, bool sizeGrip = true, bool tooltip_ = true);
	};

	/// Sets the initial size of the given part
	void setSize(unsigned part, unsigned size);

	/**
	* Sets the text of the given part
	* @param part index of the part to update.
	* @param text new text to put into "s".
	* @param alwaysResize if false, the part will be resized only if the new text is too big for
	* the current; if true, the size of the part will always be adjusted to the text it contains.
	* note: setting "alwaysResize" to true for often changing parts might result in flickering.
	*/
	void setText(unsigned part, const tstring& text, bool alwaysResize = false);

	/// Sets the help id of the given part. If not set, the help id of the whole status bar is used instead.
	void setHelpId(unsigned part, unsigned id);

	void mapWidget(unsigned part, Widget* widget, const Rectangle& padding = Rectangle(0, 0, 0, 0));

	void onClicked(unsigned part, const F& f);
	void onDblClicked(unsigned part, const F& f);

	/// Actually creates the StatusBar
	/** You should call WidgetFactory::createStatusBar if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create(const Seed& cs = Seed());

	void layout(Rectangle& r);

	virtual bool tryFire(const MSG& msg, LRESULT& retVal);

protected:
	// Constructor Taking pointer to parent
	explicit StatusBar(Widget* parent);

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~StatusBar()
	{}

	// Contract needed by AspectClickable Aspect class
	static Message getClickMessage() { return Message(WM_NOTIFY, NM_CLICK); }

	// Contract needed by AspectDblClickable Aspect class
	static Message getDblClickMessage() { return Message(WM_NOTIFY, NM_DBLCLK); }

private:
	struct Part {
		Part() : size(0), fill(false), helpId(0), clickF(0), dblClickF(0) { }

		unsigned size;
		bool fill;

		unsigned helpId;

		F clickF;
		F dblClickF;
	};
	typedef std::vector<Part> Parts;
	Parts parts;

	ToolTipPtr tip;
	std::vector<tstring> lastLines;
	enum { MAX_LINES = 10 }; /// @todo configurable?

	void layoutSections(const Point& sz);
	Part* getClickedPart();

	void handleToolTip(tstring& text);
	void handleClicked();
	void handleDblClicked();

	// AspectHelp
	void helpImpl(unsigned& id);
};

// end namespace dwt
}

#endif
