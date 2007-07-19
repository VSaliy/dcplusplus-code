#include "../../include/smartwin/widgets/WidgetMenu.h"

namespace SmartWin {

void WidgetMenu::appendItem
	( unsigned int id, const SmartUtil::tstring & name
	, ULONG_PTR data, const IdDispatcher::F& f
	)
{
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	
	mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	mii.fType = MFT_STRING;
	mii.dwTypeData = const_cast<LPTSTR>(name.c_str());
	mii.dwItemData = data;
	mii.wID = id;
	::InsertMenuItem(this->handle(), this->getCount(), TRUE, &mii);

	Application::instance().registerCommand(Message(WM_COMMAND, id), IdDispatcher(f), this->handle());
}


void WidgetMenu::appendItem
	( unsigned int id, const SmartUtil::tstring & name
	, ULONG_PTR data, const SimpleDispatcher::F& f
	)
{
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	
	mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	mii.fType = MFT_STRING;
	mii.dwTypeData = const_cast<LPTSTR>(name.c_str());
	mii.dwItemData = data;
	mii.wID = id;
	::InsertMenuItem(this->handle(), this->getCount(), TRUE, &mii);

	Application::instance().registerCommand(Message(WM_COMMAND, id), SimpleDispatcher(f), this->handle());
}

SmartUtil::tstring WidgetMenu::getText( unsigned id, bool byPosition )
{
	MENUITEMINFO mi;
	memset( & mi, 0, sizeof( MENUITEMINFO ) );

	mi.cbSize = sizeof( MENUITEMINFO );
	mi.fMask = MIIM_TYPE;
	if ( ::GetMenuItemInfo( this->handle(), id, byPosition, & mi ) == 0 )
	{
		xAssert( false, _T( "Error while trying to get MenuItemInfo in WidgetMenu::getText..." ) );
	}
	boost::scoped_array< TCHAR > buffer( new TCHAR[++mi.cch] );
	mi.dwTypeData = buffer.get();
	if ( ::GetMenuItemInfo( this->handle(), id, FALSE, & mi ) == 0 )
	{
		xAssert( false, _T( "Error while trying to get MenuItemInfo in WidgetMenu::getText..." ) );
	}
	SmartUtil::tstring retVal = mi.dwTypeData;
	return retVal;
}

void WidgetMenu::setText( unsigned id, const SmartUtil::tstring& text )
{
	MENUITEMINFO info = { sizeof( MENUITEMINFO ) };

	// set flag
	info.fMask = MIIM_STRING;
	info.dwTypeData = (TCHAR*) text.c_str();

	if ( ::SetMenuItemInfo( this->handle(), id, FALSE, & info ) == FALSE )
		throw xCeption( _T( "Couldn't set item info in setItemText()" ) );
}

}
