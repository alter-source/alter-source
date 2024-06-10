#pragma once

#include "vgui_controls/Panel.h"
#include "GameUI/IGameUI.h"
#include "Alter_VAwesomium.h"
#include "vgui_controls/Button.h"


// This class is what is actually used instead of the main menu.
class OverrideUI_RootPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(OverrideUI_RootPanel, vgui::Panel);
public:
	OverrideUI_RootPanel(vgui::VPANEL parent);
	virtual ~OverrideUI_RootPanel();

	IGameUI* GetGameUI();
	void LoadMainMenuHTML(bool reload);
	void StartGame(void);

	bool OverrideUI_RootPanel::IsPlayerInGame();
	void OverrideUI_RootPanel::OnTick();

protected:
	virtual void OnKeyCodePressed(vgui::KeyCode code) override;

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char *command) override;

private:
	vgui::Button *m_pStartButton;
	vgui::Button *m_pOptionsButton;
	vgui::Button *m_pFindButton;
	vgui::Button *m_pExitButton;
	vgui::Button *m_pDisconnectButton;
	vgui::Button *m_pResumeButton;

	bool LoadGameUI();

	int m_ExitingFrameCount;
	bool m_bCopyFrameBuffer;

	IGameUI* gameui;
	Alter_VAwesomium* m_MainMenuAwesomium;
};
