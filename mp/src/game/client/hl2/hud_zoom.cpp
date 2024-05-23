//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "../hud_crosshair.h"

#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Draws the zoom screen
//-----------------------------------------------------------------------------
class CHudZoom : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudZoom, vgui::Panel );

public:
	CHudZoom( const char *pElementName );
	
	bool	ShouldDraw( void );
	void	Init( void );
	void	LevelInit( void );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint( void );

private:
	bool	m_bZoomOn;
	float	m_flZoomStartTime;
	bool	m_bPainted;

	CPanelAnimationVarAliasType( float, m_flCircle1Radius, "Circle1Radius", "66", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flCircle2Radius, "Circle2Radius", "74", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDashGap, "DashGap", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDashHeight, "DashHeight", "4", "proportional_float" );

	CMaterialReference m_ZoomMaterial;
};

DECLARE_HUDELEMENT( CHudZoom );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudZoom::CHudZoom( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudZoom")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: standard hud element init function
//-----------------------------------------------------------------------------
void CHudZoom::Init( void )
{
	m_bZoomOn = false;
	m_bPainted = false;
	m_flZoomStartTime = -999.0f;
	m_ZoomMaterial.Init( "vgui/zoom", TEXTURE_GROUP_VGUI );
}

//-----------------------------------------------------------------------------
// Purpose: standard hud element init function
//-----------------------------------------------------------------------------
void CHudZoom::LevelInit( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: sets scheme colors
//-----------------------------------------------------------------------------
void CHudZoom::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
	SetFgColor(scheme->GetColor("ZoomReticleColor", GetFgColor()));

	SetForceStereoRenderToFrameBuffer( true );
	int x, y;
	int screenWide, screenTall;
	surface()->GetFullscreenViewport( x, y, screenWide, screenTall );
	SetBounds(0, 0, screenWide, screenTall);
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudZoom::ShouldDraw( void )
{
	bool bNeedsDraw = false;

	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());
	if ( pPlayer == NULL )
		return false;

	if ( pPlayer->m_HL2Local.m_bZooming )
	{
		// need to paint
		bNeedsDraw = true;
	}
	else if ( m_bPainted )
	{
		// keep painting until state is finished
		bNeedsDraw = true;
	}

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

#define	ZOOM_FADE_TIME	0.2f
//-----------------------------------------------------------------------------
// Purpose: draws the zoom effect
//-----------------------------------------------------------------------------
void CHudZoom::Paint( void )
{
	m_bPainted = false;

	// see if we're zoomed any
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());
	if ( pPlayer == NULL )
		return;

	if ( pPlayer->m_HL2Local.m_bZooming && m_bZoomOn == false )
	{
		m_bZoomOn = true;
		m_flZoomStartTime = gpGlobals->curtime;
	}
	else if ( pPlayer->m_HL2Local.m_bZooming == false && m_bZoomOn )
	{
		m_bZoomOn = false;
		m_flZoomStartTime = gpGlobals->curtime;
	}

	// draw the appropriately scaled zoom animation
	float scale = clamp(ZOOM_FADE_TIME, 0.0f, 1.0f);

	float alpha;

	if (m_bZoomOn) {
		alpha = scale;
	}
	else {
		float invScale = 1.0f - scale;
		if (invScale >= 1.0f)
			return;

		alpha = invScale * 0.25f;
		scale = invScale * 0.5f;
	}

	Color col = GetFgColor();
	col[3] = alpha * 64;

	surface()->DrawSetColor( col );
	
	m_bPainted = true;
}
