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

#include <dwt/widgets/ComboBox.h>

namespace dwt {

const TCHAR ComboBox::windowClass[] = WC_COMBOBOX;

ComboBox::Seed::Seed() :
BaseType::Seed(CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VSCROLL),
font(0),
extended(true)
{
}

ComboBox::ComboBox(Widget* parent ) : BaseType(parent, ChainingDispatcher::superClass<ComboBox>()), dropDownHeight(::GetSystemMetrics(SM_CYSCREEN) / 3) {

}

void ComboBox::create( const Seed & cs ) {
	BaseType::create(cs);
	setFont(cs.font);
	if(cs.extended)
		sendMessage(CB_SETEXTENDEDUI, TRUE);
}

tstring ComboBox::getValue( int index ) {
	// Uses CB_GETLBTEXTLEN and CB_GETLBTEXT
	int txtLength = ComboBox_GetLBTextLen( handle(), index );
	tstring retVal(txtLength, '\0');
	ComboBox_GetLBText( handle(), index, &retVal[0] );
	return retVal;
}

Point ComboBox::getPreferredSize() {
	// Pixels between text and arrow
	const int MARGIN = 2;

	UpdateCanvas c(this);
	auto select(c.select(*getFont()));
	Point ret;
	for(size_t i = 0; i < size(); ++i) {
		Point ext = c.getTextExtent(getValue(i));
		ret.x = std::max(ret.x, ext.x);
	}

	ret.x += ::GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + ::GetSystemMetrics(SM_CXSMICON) + MARGIN;
	ret.y = sendMessage(CB_GETITEMHEIGHT, -1) + ::GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
	return ret;
}

void ComboBox::layout() {
	/* TODO
	Rectangle copy(r);
	copy.size.y = dropDownHeight;
	BaseType::layout(copy);
	*/
	BaseType::layout();
}

LPARAM ComboBox::getDataImpl(int i) {
	return sendMessage(CB_GETITEMDATA, i);
}

void ComboBox::setDataImpl(int i, LPARAM data) {
	sendMessage(CB_SETITEMDATA, i, data);
}

}
