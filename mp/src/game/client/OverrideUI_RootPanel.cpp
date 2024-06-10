#include "cbase.h"
#include "overrideui_rootpanel.h"
#include "ioverrideinterface.h"

#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"
#include <engine/IEngineSound.h>
#include "filesystem.h"

using namespace vgui;

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUIDLL("GameUI");

OverrideUI_RootPanel *guiroot = NULL;

void OverrideGameUI()
{
	if (!OverrideUI->GetPanel())
	{
		OverrideUI->Create(NULL);
	}
	if (guiroot->GetGameUI())
	{
		guiroot->GetGameUI()->SetMainMenuOverride(guiroot->GetVPanel());
		return;
	}
}

OverrideUI_RootPanel::OverrideUI_RootPanel(VPANEL parent) : Panel(NULL, "OverrideUIRootPanel")
{
	SetParent(parent);
	guiroot = this;

	m_bCopyFrameBuffer = false;
	gameui = NULL;

	LoadGameUI();

	m_ExitingFrameCount = 0;

	LoadMainMenuHTML(false);
	ivgui()->AddTickSignal(GetVPanel());
}

void OverrideUI_RootPanel::OnTick()
{
	RequestFocus();
	BaseClass::OnTick();
}

void OverrideUI_RootPanel::OnKeyCodePressed(KeyCode code)
{
	if (code == KEY_ESCAPE)
	{
		LoadMainMenuHTML(true);
	}

	BaseClass::OnKeyCodePressed(code);
}

IGameUI *OverrideUI_RootPanel::GetGameUI()
{
	if (!gameui)
	{
		if (!LoadGameUI())
			return NULL;
	}

	return gameui;
}

bool OverrideUI_RootPanel::LoadGameUI()
{
	if (!gameui)
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
		if (gameUIFactory)
		{
			gameui = (IGameUI *)gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL);
			if (!gameui)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

void OverrideUI_RootPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
}

void OverrideUI_RootPanel::LoadMainMenuHTML(bool reload)
{
	if (reload)
	{
		m_MainMenuAwesomium->DeletePanel();
	}

	m_MainMenuAwesomium = new Alter_VAwesomium(this, "MainMenuAwesomium");
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	m_MainMenuAwesomium->SetBounds(0, 0, wide, tall);
	m_MainMenuAwesomium->OpenHTMLFile("html/main.html");
}

bool OverrideUI_RootPanel::IsPlayerInGame()
{
	int localPlayerIndex = GetLocalPlayerIndex();
	return localPlayerIndex != 0;
}

OverrideUI_RootPanel::~OverrideUI_RootPanel()
{
	gameui = NULL;
	g_GameUIDLL.Unload();
}

void OverrideUI_RootPanel::OnCommand(const char *command)
{
	IGameUI* gameUI = GetGameUI();
	if (!gameUI) { return; }
}

void ReloadHTML(const CCommand &args)
{
	if (guiroot)
	{
		guiroot->LoadMainMenuHTML(true);
	}
}

static ConCommand reload_html("reload_html", ReloadHTML, "Reload the main HTML file for the UI.", FCVAR_CLIENTDLL);
