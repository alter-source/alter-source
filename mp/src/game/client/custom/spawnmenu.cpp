//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: пока debug options....
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "spawnmenu.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/MenuButton.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MenuItem.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyDialog.h>
#include "vgui_imagebutton.h"
#include "filesystem.h"

#include <iostream>
#include <cstring>
#include <string>

#include "tier0/memdbgon.h"

using namespace vgui;

bool endsWith(const char* str, const char* suffix) {
	size_t str_len = std::strlen(str);
	size_t suffix_len = std::strlen(suffix);

	if (str_len < suffix_len) {
		return false;
	}

	return std::strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

class CSMLCommandButton : public vgui::Button
{
	typedef vgui::Button BaseClass;

public:
	CSMLCommandButton(vgui::Panel *parent, const char *panelName, const char *labelText, const char *command)
		: BaseClass(parent, panelName, labelText)
	{
		AddActionSignalTarget(this);
		SetCommand(command);
	}

	virtual void OnCommand(const char *command)
	{
		engine->ClientCmd((char *)command);
	}

	virtual void OnTick(void)
	{
	}

	virtual void Paint()
	{
		BaseClass::Paint();
	}
};

class CSMLPage : public vgui::PropertyPage
{
	typedef vgui::PropertyPage BaseClass;
public:
	CSMLPage(vgui::Panel *parent, const char *panelName)
		: BaseClass(parent, panelName)
	{
		vgui::ivgui()->AddTickSignal(GetVPanel(), 250);
	}

	virtual void OnTick(void)
	{
		BaseClass::OnTick();

		if (!IsVisible())
			return;

		int c = m_LayoutItems.Count();
		for (int i = 0; i < c; i++)
		{
			vgui::Panel *p = m_LayoutItems[i];
			p->OnTick();
		}
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		int x = 5;
		int y = 5;
		int w = 150;
		int h = 18;
		int gap = 2;

		int c = m_LayoutItems.Count();
		int tall = GetTall();

		for (int i = 0; i < c; i++)
		{
			vgui::Panel *p = m_LayoutItems[i];
			p->SetBounds(x, y, w, h);

			y += (h + gap);
			if (y >= tall - h)
			{
				x += (w + gap);
				y = 5;
			}
		}
	}

	void Init(KeyValues *kv)
	{
		for (KeyValues *control = kv->GetFirstSubKey(); control != NULL; control = control->GetNextKey())
		{
			int iType = control->GetInt("imagebutton", 0);

			if (iType == 1)
			{
				const char *a, *b, *c, *t;
				//command
				t = control->GetString("#", "");
				a = control->GetString("normal", "");
				b = control->GetString("overimage", "");
				c = control->GetString("mouseclick", "");

				if (t, a, b, c && t[0], a[0], b[0], c[0])
				{

				}
			}
			else
			{
				const char *m;

				//command
				m = control->GetString("#", "");
				if (m && m[0])
				{
					if (endsWith(control->GetName(), "(Episodic)")) {
						if (!cvar->FindVar("hl2_episodic")->GetBool()) {
							break;
						}
					}
					else if (endsWith(control->GetName(), "(EP2)")) {
						if (!cvar->FindVar("ep2_mounted")->GetBool()) {
							break;
						}
					}
					else if (endsWith(control->GetName(), "(Lost Coast)")) {
						if (!cvar->FindVar("lc_mounted")->GetBool()) {
							break;
						}
					}
					else if (endsWith(control->GetName(), "(Portal)")) {
						if (!cvar->FindVar("portal_mounted")->GetBool()) {
							break;
						}
					}
					else if (endsWith(control->GetName(), "(TF2)")) {
						if (!cvar->FindVar("tf2_mounted")->GetBool()) {
							break;
						}
					}
					else if (endsWith(control->GetName(), "(HLS)")) {
						if (!cvar->FindVar("hl_source_mounted")->GetBool()) {
							break;
						}
					}

					CSMLCommandButton *btn = new CSMLCommandButton(this, "CommandButton", control->GetName(), m);
					m_LayoutItems.AddToTail(btn);
					continue;
				}
			}
		}
	}
private:
	CUtlVector< vgui::Panel * >		m_LayoutItems;
};

ConVar as_spawnmenu("as_spawnmenu", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Enable the Spawn Menu");
ConVar as_spawnmenu_width("as_spawnmenu_width", "934", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set width of Spawn Menu [WORKS AFTER MAP RELOAD]");
ConVar as_spawnmenu_height("as_spawnmenu_height", "670", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set height of Spawn Menu [WORKS AFTER MAP RELOAD");
ConVar spawn("spawnmenu", "0", FCVAR_CLIENTDLL, "Spawn Menu");

class CSMLMenu : public vgui::PropertyDialog
{
	typedef vgui::PropertyDialog BaseClass;
public:

	CSMLMenu(vgui::VPANEL *parent, const char *panelName)
		: BaseClass(NULL, "SMLenu")
	{
		SetTitle("Spawn Menu", true);

		KeyValues *kv = new KeyValues("SMLenu");
		if (kv)
		{
			if (kv->LoadFromFile(g_pFullFileSystem, "scripts/props.txt"))
			{
				for (KeyValues *dat = kv->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
				{
					if (!Q_strcasecmp(dat->GetName(), "width")) // KeyValues are good, but not for this xD
					{
						//SetWide(dat->GetInt());
						continue;
					}
					else if (!Q_strcasecmp(dat->GetName(), "height"))
					{
						//SetTall(dat->GetInt());
						continue;
					}

					const char* tabName = dat->GetName();

					if (Q_strcasecmp(tabName, "Portal") == 0 && !cvar->FindVar("portal_mounted")->GetBool())
					{
						continue;
					}

					if (Q_strcasecmp(tabName, "TF2") == 0 && !cvar->FindVar("tf2_mounted")->GetBool())
					{
						continue;
					}

					CSMLPage *page = new CSMLPage(this, tabName);
					page->Init(dat);

					AddPage(page, tabName);
				}
			}
			kv->deleteThis();
		}

		vgui::ivgui()->AddTickSignal(GetVPanel(), 100);
		GetPropertySheet()->SetTabWidth(72);
		SetVisible(true);
		SetMouseInputEnabled(true);
		SetKeyBoardInputEnabled(false); // Allow movement;
		SetSizeable(true);

		/*size*/
		SetWide(as_spawnmenu_width.GetInt());
		SetTall(as_spawnmenu_height.GetInt());

		/*position*/
		int ww, wt;
		ww = ScreenWidth();
		wt = ScreenHeight();
		SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
	}

	void Init(KeyValues *kv);

	void OnTick()
	{
		BaseClass::OnTick();
		ConVar* pAsGameModeCVar = g_pCVar->FindVar("as_gamemode");
		const char* gameModeValue = pAsGameModeCVar->GetString();

		/*size also here*/
		SetWide(as_spawnmenu_width.GetInt());
		SetTall(as_spawnmenu_height.GetInt());

		if (strcmp(gameModeValue, "Sandbox") == 0 || as_spawnmenu.GetBool()) {
			SetVisible(spawn.GetBool());
		}
		else {
			return;
		}
	}

	void OnCommand(const char *command)
	{
		BaseClass::OnCommand(command);

		//if (!Q_stricmp(command, "Close"))
		//{
		//	spawn.SetValue(0);
		//}
	}

};

void CSMLMenu::Init(KeyValues *kv)
{
}

class CSMLPanelInterface : public SMLPanel
{
private:
	CSMLMenu *SMLPanel;
public:
	CSMLPanelInterface()
	{
		SMLPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		SMLPanel = new CSMLMenu(&parent, "SMenu");
	}
	void Destroy()
	{
		if (SMLPanel)
		{
			SMLPanel->SetParent((vgui::Panel *)NULL);
			delete SMLPanel;
		}
	}
	void Activate(void)
	{
		if (SMLPanel)
		{
			SMLPanel->Activate();
		}
	}
};
static CSMLPanelInterface g_SMLPanel;
SMLPanel* smlmenu = (SMLPanel*)&g_SMLPanel;