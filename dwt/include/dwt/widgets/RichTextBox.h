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

#ifndef DWT_RichTextBox_h
#define DWT_RichTextBox_h

#ifndef WINCE // Doesn't exist in Windows CE based systems

#include "TextBox.h"
#include "../tstring.h"
#include <richedit.h>

#include <boost/algorithm/string.hpp>

namespace dwt {

/// RichEdit Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html richedit.PNG
  * Class for creating a Rich Edit control. <br>
  * A Rich Edit Control is derived from TextBox and inherits ( mostly ) all
  * properties and member functions of the TextBox Widget. <br>
  * In addition to the TextBox RichTextBox can display colored text,
  * hyperlinks, OLE objects etc. <br>
  * A good example of the difference between those Widgets is the difference between
  * notepad ( TextBox ) and wordpad ( RichTextBox )
  */
class RichTextBox :
	public TextBoxBase
{
	friend class WidgetCreator< RichTextBox >;
	typedef TextBoxBase BaseType;
public:
	/// Class type
	typedef RichTextBox ThisType;

	/// Object type
	typedef ThisType * ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed
	{
		typedef RichTextBox::ThisType WidgetType;

		FontPtr font;
		COLORREF foregroundColor;
		COLORREF backgroundColor;
		bool scrollBarHorizontallyFlag;
		bool scrollBarVerticallyFlag;

		/// Fills with default parameters
		Seed();
	};

	/// Actually creates the Rich Edit Control
	/** You should call WidgetFactory::createRichTextBox if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create( const Seed & cs = Seed() );

	/// Sets the background color of the RichTextBox
	/** Call this function to alter the background color of the WidgetRichEdit. <br>
	  * To create a COLORREF ( color ) use the RGB macro.
	  */
	void setBackgroundColor( COLORREF color );

	/// Sets default character formatting of the WidgetRichTextBox
	void setDefaultCharFormat( CHARFORMAT cf );

	int charFromPos(const ScreenCoordinate& pt);

	int lineFromPos(const ScreenCoordinate& pt);

	Point posFromChar(int charOffset);

	tstring getSelection() const;

	Point getScrollPos() const;

	void setScrollPos(Point& scrollPos);

	tstring textUnderCursor(const ScreenCoordinate& p);

	/// Appends text to the richtext box
	/** The txt parameter is the new text to append to the text box.
	  */
	void addText( const std::string & txt );

	void addTextSteady ( const tstring & txtRaw, std::size_t len );

	void findText(tstring const& needle) throw();

	void clearCurrentNeedle();

protected:
	tstring currentNeedle;		// search in chat window
	int currentNeedlePos;		// search in chat window

	LRESULT onUnsuppRtf(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	int fixupLineEndings(tstring::const_iterator begin, tstring::const_iterator end,
		tstring::difference_type ibo) const;

	typedef boost::iterator_range<boost::range_const_iterator<tstring>::type> tstring_range;

	std::string static unicodeEscapeFormatter(const tstring_range& match);
	std::string escapeUnicode(const tstring& str);

	// Constructor Taking pointer to parent
	explicit RichTextBox( dwt::Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~RichTextBox()
	{}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline RichTextBox::RichTextBox( dwt::Widget * parent )
	: TextBoxBase( parent )
{
}

inline void RichTextBox::setBackgroundColor( COLORREF color )
{
	this->sendMessage(EM_SETBKGNDCOLOR, 0, static_cast< LPARAM >( color ) );
}

inline void RichTextBox::setDefaultCharFormat( CHARFORMAT cf )
{
	this->sendMessage(EM_SETCHARFORMAT, 0, reinterpret_cast< LPARAM >(&cf));
}

// end namespace dwt
}

#endif //! WINCE

#endif
