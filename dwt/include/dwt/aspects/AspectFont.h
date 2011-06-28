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

#ifndef DWT_AspectFont_h
#define DWT_AspectFont_h

#include "../resources/Font.h"
#include "../CanvasClasses.h"

namespace dwt {

/// Aspect class used by Widgets that have the possibility of setting the
/// "font" property of their objects.
/** \ingroup AspectClasses
  * E.g. the Table have a "font" Aspect therefore it realizes the AspectFont
  * through inheritance. <br>
  * Realizing the AspectFont means that a Widget can set the font used to render text
  * in the Widget, for a ComboBox this means that it will render items in the
  * dropdownlist and in the selected area with the given font while for a textbox
  * this means that it will render all text with the given font. <br>
  * Most Widgets which can render text in some way realize this Aspect.
  */
template< class WidgetType >
class AspectFont
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
public:
	/// Fills a Point with the size of text to be drawn in the Widget's font.
	/** getTextSize determines the height and width that text will take. <br>
	  * This is useful if you want to allocate enough space to fit known text. <br>
	  * It accounts for the set font too.
	  */
	Point getTextSize(const tstring& text);

	/// Sets the font used by the Widget
	/** Changes the font of the Widget to the given font. Use the class Font to
	  * construct a font in which to set by this function.
	  */
	void setFont( const FontPtr& font, bool forceUpdate = true );

	/// Returns the font used by the Widget
	/** Returns the Font object currently being used by the Widget
	  */
	const FontPtr& getFont();

private:
	// Keep a reference around so it doesn't get deleted
	FontPtr font;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template< class WidgetType >
Point AspectFont< WidgetType >::getTextSize( const tstring & text )
{
	UpdateCanvas c(&W());
	auto select(c.select(*W().getFont()));

	Rectangle rect;
	c.drawText(text, rect, DT_CALCRECT | DT_NOPREFIX);

	return rect.size;
}


template< class WidgetType >
void AspectFont< WidgetType >::setFont( const FontPtr& font_, bool forceUpdate )
{
	font = font_ ? font_ : new Font(Font::DefaultGui);
	W().sendMessage(WM_SETFONT, reinterpret_cast< WPARAM >( font->handle() ), static_cast< LPARAM >( forceUpdate ) );
}

template< class WidgetType >
const FontPtr& AspectFont< WidgetType >::getFont()
{
	if(!font) {
		font = new Font(reinterpret_cast<HFONT>(W().sendMessage(WM_GETFONT)), false);
	}
	return font;
}

}

#endif
