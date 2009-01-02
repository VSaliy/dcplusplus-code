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

#ifndef DWT_ModalDialog_h
#define DWT_ModalDialog_h

#include "../aspects/AspectDialog.h"
#include "Frame.h"
#include "../Application.h"

namespace dwt {

/// Modal Dialog class
/** \ingroup WidgetControls
  * \image html dialog.PNG
  * Class for creating a Modal Dialog based optionally on an embedded resource. <br>
  * Use createDialog( unsigned resourceId ) if you define the dialog in a .rc file,
  * and use createDialog() if you define the dialog completly in C++ source. <br>
  * Use the createDialog function to actually create a dialog. <br>
  * Class is a public superclass of Frame and therefore can use all
  * features of Frame. <br>
  * Note! <br>
  * Usually you create a ModalDialog on the stack. <br>
  * This Widget does NOT have selfdestructive semantics and should normally be
  * constructed on the stack! <br>
  * The createDialog function does NOT return before the Widget is destroyed! <br>
  * Thus, you must declare the "onInitDialog" event handler before calling the
  * "createDialog()", either in the contructor, or in some intialization routine
  * called before createDialog();
  */

class ModalDialog :
	public Frame< Policies::ModalDialog >,
	public AspectDialog<ModalDialog >
{
	typedef Frame< Policies::ModalDialog > BaseType;
	struct Dispatcher
	{
		typedef std::tr1::function<bool ()> F;

		Dispatcher(const F& f_) : f(f_) { }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			ret = f() ? TRUE : FALSE;
			return true;
		}

		F f;
	};

public:

	/// Class type
	typedef ModalDialog ThisType;

	/// Object type
	/** Note, not a pointer!!!!
	  */
	typedef ThisType ObjectType;

	/// Creates a Modal Dialog Window from a resource id.
	/** This version creates a window from a Dialog Resource ID. <br>
	  * To be called by the invoker of the dialog. <br>
	  * The return comes from the parameter to endDialog() <br>
	  * You must call onInitDialog( &MyDialogWidget::initDialog ); or similar either
	  * in the constructor of your dialog or right before calling this function. <br>
	  * And in your initDialog, you must call attachXxxx for all of the controls
	  * you wish to use, and set the event handlers for all controls and events you
	  * wish to handle. <br>
	  * Example : <br>
	  * LabelPtr prompt = attachLabel( IDC_PROMPT ); <br>
	  * prompt->onClicked( &X::myClickMethod ); <br>
	  * ...etc...
	  */
	void createDialog(unsigned resourceId);

	/// Creates a Modal Dialog Window defined in C++ alone.
	/** This version creates a dialog window without using a Dialog Resource ID. <br>
	  * To be called by the invoker of the dialog. <br>
	  * The return comes from the parameter to endDialog() <br>
	  * You must call onInitDialog( &MyModalDialogWidget::initDialog ); in the
	  * constructor of your dialog, <br>
	  * and in your initDialog you create the dialog's Widgets yourself. <br>
	  * Example : <br>
	  * LabelPtr prompt = createLabel(); <br>
	  * prompt->setBounds( 10, 100, 100, 50 ); <br>
	  * prompt->setText( _T("testing") );
	  */
	void createDialog();

	/// Display the dialog and return only when the dialog has been dismissed
	int show();

	/// Ends the Modal Dialog Window started with createDialog().
	/** Pass a return value for createDialog() and close the dialog. <br>
	  * To be called by the dialog class when it should close. <br>
	  * Note that the member variables of the ModalDialog class still exist,
	  * but not any subwindows or Control Widgets.
	  */
	void endDialog( int returnValue );

	/// Dialog Init Event Handler setter
	/** This would normally be the event handler where you attach your Widget
	  * controls and do all the initializing etc... <br>
	  * It's important that you declare this event handler BEFORE calling the
	  * createDialog function since that function doesn't actually return before the
	  * dialog is destroyed! <br>
	  * Method signature must be bool foo(); <br>
	  * If you return true from your Event Handler the system will NOT mess up the
	  * initial focus you have chosen, if you return false the system will decide
	  * which control to initially have focus according to the tab order of the
	  * controls!
	  */
	void onInitDialog(const Dispatcher::F& f) {
		addCallback(
			Message(WM_INITDIALOG), Dispatcher(f)
		);
	}

protected:
	// Protected since this Widget we HAVE to inherit from
	explicit ModalDialog( Widget * parent = 0 );

	virtual ~ModalDialog();

	/// Specify how a resourceless dialog's window appears.
	/** The derived pure dialog class can control the DLGTEMPLATE parameters used in
	  * createDialog() with this protected call. <br>
	  * The calling layer is prevented from doing so. <br>
	  * See DLGTEMPLATE as used in ::DialogBoxIndirectParam for details.
	  */
	void setDlgTemplate( DLGTEMPLATE inTemplate );

	/// Called by default when WM_CLOSE is posted to the dialog
	bool defaultClosing() {
		endDialog(IDCANCEL);
		return true;
	}

private:
	bool quit;
	int ret;

	Application::FilterIter filterIter;
	bool filter(MSG& msg);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void ModalDialog::endDialog(int retv) {
	quit = true;
	ret = retv;
}

}

#endif
