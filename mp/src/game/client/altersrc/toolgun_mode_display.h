#ifndef ALTERSRC_TOOLGUN_MODE_DISPLAY
#define ALTERSRC_TOOLGUN_MODE_DISPLAY

#include "IToolGunModeDisplay.h"

#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
using namespace vgui;

class CToolgunModeDisplay : public vgui::Frame
{
    DECLARE_CLASS_SIMPLE(CToolgunModeDisplay, vgui::Frame);

    CToolgunModeDisplay(vgui::VPANEL parent);
    ~CToolgunModeDisplay() { };

public:
    virtual void SetLabel(char text);

protected:
    virtual void OnTick();
    virtual void OnCommand(const char *pCommand);

private:
    vgui::Label *m_lblGunMode;
}

//Class: CMyPanelInterface Class. Used for construction.
class CToolgunModeDisplayInterface : public IToolGunModeDisplay
{
private:
	CToolgunModeDisplay *ToolgunModeDisplay;
public:
	CToolgunModeDisplayInterface()
	{
		ToolgunModeDisplay = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		ToolgunModeDisplay = new CToolgunModeDisplay(parent);
	}
	void Destroy()
	{
		if (ToolgunModeDisplay)
		{
			ToolgunModeDisplay->SetParent( (vgui::Panel *)NULL);
			delete ToolgunModeDisplay;
		}
	}
};
static CToolgunModeDisplayInterface g_ToolGunModeDisplay;
IToolGunModeDisplay* toolgunmodedisplay = (IToolGunModeDisplay*)&g_ToolGunModeDisplay;

#endif // !ALTERSRC_TOOLGUN_MODE_DISPLAY