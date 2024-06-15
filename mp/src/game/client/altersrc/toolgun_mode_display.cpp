#include "cbase.h"
#include "toolgun_mode_display.h"

CToolgunModeDisplay::CToolgunModeDisplay(vgui::VPANEL parent)
{
    SetParent( parent );
	
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	
	SetProportional( false );
	SetTitleBarVisible( true );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetVisible( true );


	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	LoadControlSettings("resource/toolgunmodedisplay.res");
    // oh no i dont know how to do .res files :sob:

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	
	DevMsg("ToolgunModeDisplay has been constructed\n");
}