#include "cbase.h"
#include "Alter_VAwesomium.h"

using namespace Awesomium;

#ifdef PostMessage
#undef PostMessage
#endif

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

	myObject.SetCustomMethod(WSLit("InGame"), false);

	caller->ExecuteJavascript(WSLit("AwesomiumInitialized()"), WSLit(""));
}

bool IsPlayerInGame() {
	return engine->IsInGame();
}

void Alter_VAwesomium::OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args)
{
	if (method_name == WSLit("OpenOptions"))
	{
		SendCommand("gamemenucommand openoptionsdialog");
	}
	if (method_name == WSLit("CreateServer")) {
		SendCommand("gamemenucommand opencreatemultiplayergamedialog");
	}
	if (method_name == WSLit("FindServers")) {
		SendCommand("gamemenucommand openserverbrowser");
	}
	if (method_name == WSLit("Quit")) {
		SendCommand("gamemenucommand quit");
	}
	if (method_name == WSLit("Disconnect")) {
		SendCommand("gamemenucommand disconnect");
	}
	if (method_name == WSLit("ResumeGame")) {
		SendCommand("gamemenucommand resumegame");
	}
	if (method_name == WSLit("InGame")) {
		bool inGame = IsPlayerInGame();
		std::string jsCode = "OnInGameResult(" + std::string(inGame ? "true" : "false") + ");";
		caller->ExecuteJavascript(WSLit(jsCode.c_str()), WSLit(""));
	}
}