#include "VAwesomium.h"

#define EXIT_COMMAND "ExitAlterVAwesomium"

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

class Alter_VAwesomium : public VAwesomium
{
	DECLARE_CLASS_SIMPLE(Alter_VAwesomium, VAwesomium);
public:
	Alter_VAwesomium(vgui::Panel *parent, const char *panelName) : VAwesomium(parent, panelName){};

	virtual void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url);

	virtual void OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args);
};