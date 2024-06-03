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

	LoadMainMenuHTML();
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

void OverrideUI_RootPanel::LoadMainMenuHTML()
{
	m_MainMenuAwesomium = new VAwesomium(this, "MainMenuAwesomium");
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	m_MainMenuAwesomium->SetBounds(0, 0, wide, tall);
	m_MainMenuAwesomium->OpenHTMLFile("html/main.html");

	IGameUI* gameUI = GetGameUI();

	if (m_pResumeButton)
		m_pResumeButton->DeletePanel();

	if (m_pStartButton)
		m_pStartButton->DeletePanel();

	if (m_pOptionsButton)
		m_pOptionsButton->DeletePanel();

	if (m_pFindButton)
		m_pFindButton->DeletePanel();

	if (m_pExitButton)
		m_pExitButton->DeletePanel();

	if (m_pDisconnectButton)
		m_pDisconnectButton->DeletePanel();

	if (gameUI && IsPlayerInGame())
	{
		m_pResumeButton = new Button(this, "mResumeButton", "Resume Game", this, "ResumeGame");
		m_pResumeButton->SetBounds(80, 140, 150, 30);
	}

	m_pStartButton = new Button(this, "mStartGameButton", "Start Game", this, "StartGame");
	m_pStartButton->SetAlpha(255);
	m_pStartButton->SetBounds(80, 180, 250, 30);

	m_pOptionsButton = new Button(this, "mOptionsButton", "Settings", this, "Options");
	m_pOptionsButton->SetBounds(80, 220, 150, 30);

	m_pFindButton = new Button(this, "mFindButton", "Find Servers", this, "FindServers");
	m_pFindButton->SetBounds(80, 260, 150, 30);

	if (gameUI && IsPlayerInGame())
	{
		m_pDisconnectButton = new Button(this, "mDisconnectButton", "Disconnect", this, "DisconnectGame");
		m_pDisconnectButton->SetBounds(80, 300, 150, 30);
	}

	m_pExitButton = new Button(this, "mExitButton", "Exit", this, "ExitGame");
	m_pExitButton->SetBounds(80, 380, 150, 30);
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

	if (FStrEq(command, "StartGame"))
	{
		gameUI->SendMainMenuCommand("OpenCreateMultiplayerGameDialog");
	}
	else if (FStrEq(command, "Options")) {
		gameUI->SendMainMenuCommand("OpenOptionsDialog");
	}
	else if (FStrEq(command, "Exit")) {
		gameUI->OnConfirmQuit();
	}
	else if (FStrEq(command, "FindServers")) {
		gameUI->SendMainMenuCommand("OpenServerBrowser");
	}
	else if (FStrEq(command, "DisconnectGame")) {
		gameUI->SendMainMenuCommand("Disconnect");
	}
	else if (FStrEq(command, "ResumeGame")) {
		gameUI->SendMainMenuCommand("ResumeGame");
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void ReloadHTML(const CCommand &args)
{
	if (guiroot)
	{
		guiroot->LoadMainMenuHTML();
	}
}

static ConCommand reload_html("reload_html", ReloadHTML, "Reload the main HTML file for the UI.", FCVAR_CLIENTDLL);
