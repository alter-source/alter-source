#include "cbase.h"
#include "Alter_VAwesomium.h"

using namespace Awesomium;

#ifdef PostMessage
#undef PostMessage
#endif

// practicemedicine-addict: these gameui codes here are from alter source's custom main menu code

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUIDLL( "GameUI" );

//extern ConVar cl_showmypanel; // practicemedicine-addict: from my unreleased mod aridmess :pensive:

IGameUI *Alter_VAwesomium::GetGameUI()
{
	if (!gameui)
	{
		if (!LoadGameUI())
			return NULL;
	}

	return gameui;
}

bool Alter_VAwesomium::LoadGameUI()
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

void SendCommand(const char* command) {
	engine->ClientCmd_Unrestricted(command);
}

void Alter_VAwesomium::OnDocumentReady(WebView* caller, const WebURL& url)
{
	JSValue result = caller->CreateGlobalJavascriptObject(WSLit("Object"));

	JSObject &myObject = result.ToObject();
	myObject.SetCustomMethod(WSLit("OpenOptions"), false);
	myObject.SetCustomMethod(WSLit("Disconnect"), false);
	myObject.SetCustomMethod(WSLit("ResumeGame"), false);
	myObject.SetCustomMethod(WSLit("FindServers"), false);
	myObject.SetCustomMethod(WSLit("CreateServer"), false);
	myObject.SetCustomMethod(WSLit("Quit"), false);
	myObject.SetCustomMethod(WSLit("Addons"), false);
	myObject.SetCustomMethod(WSLit("InGame"), false);

	caller->ExecuteJavascript(WSLit("AwesomiumInitialized()"), WSLit(""));
}

bool IsPlayerInGame() {
	return engine->IsInGame();
}

void Alter_VAwesomium::OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args)
{
	// FIXME:: god damn it why is it so long

	IGameUI* gameui = GetGameUI();
	if (method_name == WSLit("OpenOptions"))
	{
#ifdef _DEBUG
		SendCommand("gamemenucommand openoptionsdialog");
#else
		gameui->SendMainMenuCommand("openoptionsdialog");
#endif
	}
	if (method_name == WSLit("CreateServer")) {
#ifdef _DEBUG
		SendCommand("gamemenucommand opencreatemultiplayergamedialog");
#else
		gameui->SendMainMenuCommand("opencreatemultiplayergamedialog");
#endif
	}
	if (method_name == WSLit("FindServers")) {
#ifdef _DEBUG
		SendCommand("gamemenucommand openserverbrowser");
#else
		gameui->SendMainMenuCommand("openserverbrowser");
#endif
	}
	if (method_name == WSLit("Quit")) {
#ifdef _DEBUG
		SendCommand("gamemenucommand quit");
#else
		gameui->SendMainMenuCommand("quit");
#endif
	}
	if (method_name == WSLit("Disconnect")) {
#ifdef _DEBUG
		SendCommand("gamemenucommand disconnect");
#else
		gameui->SendMainMenuCommand("disconnect");
#endif
	}
	if (method_name == WSLit("ResumeGame")) {
#ifdef _DEBUG
		SendCommand("gamemenucommand resumegame");
#else
		gameui->SendMainMenuCommand("resumegame");
#endif
	}
	if (method_name == WSLit("Addons")) {
		SendCommand("show_addons");
	}
	if (method_name == WSLit("InGame")) {
		bool inGame = IsPlayerInGame();
		std::string jsCode = "OnInGameResult(" + std::string(inGame ? "true" : "false") + ");";
		caller->ExecuteJavascript(WSLit(jsCode.c_str()), WSLit(""));
	}
}