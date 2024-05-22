//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( VGUI_INT_H )
#define VGUI_INT_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
}

bool VGui_Startup(CreateInterfaceFn appSystemFactory);
void VGui_Shutdown(void);
void VGui_CreateGlobalPanels(void);
vgui::VPANEL VGui_GetClientDLLRootPanel(void);
void VGUI_CreateClientDLLRootPanel(void);
void VGUI_DestroyClientDLLRootPanel(void);
void VGui_PreRender();
void VGui_PostRender();

//class ToolMenu
//{
//public:
//	virtual void        Create(vgui::VPANEL parent) = 0;
//	virtual void        Destroy(void) = 0;
//	virtual void        Activate(void) = 0;
//};

//extern ToolMenu* toolmenu;

//class SMPanel
//{
//public:
//virtual void		Create(vgui::VPANEL parent) = 0;
//	virtual void		Destroy(void) = 0;
//	virtual void		Activate(void) = 0;
//};

//extern SMPanel* smenu;

class SMLPanel
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy(void) = 0;
	virtual void		Activate(void) = 0;
};

extern SMLPanel* smlmenu;

#endif // VGUI_INT_H