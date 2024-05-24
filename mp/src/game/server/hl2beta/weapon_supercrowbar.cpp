//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Ice Axe - all purpose pointy beating stick
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "vstdlib/random.h"

#define	SUPERCROWBAR_RANGE	60
#define	SUPERCROWBAR_REFIRE	0.25f

ConVar    sk_plr_dmg_supercrowbar("sk_plr_dmg_supercrowbar", "100");
ConVar    sk_npc_dmg_supercrowbar("sk_npc_dmg_supercrowbar", "100");

//-----------------------------------------------------------------------------
// CWeaponSuperCrowbar
//-----------------------------------------------------------------------------

class CWeaponSuperCrowbar : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponSuperCrowbar, CBaseHL2MPBludgeonWeapon);

	DECLARE_SERVERCLASS();

	CWeaponSuperCrowbar();

	float		GetRange(void)		{ return	SUPERCROWBAR_RANGE; }
	float		GetFireRate(void)		{ return	SUPERCROWBAR_REFIRE; }

	void		ImpactSound(bool isWorld)	{ return; }

	void		AddViewKick(void);
	float		GetDamageForActivity(Activity hitActivity);
	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponSuperCrowbar, DT_WeaponSuperCrowbar)
END_SEND_TABLE()

#ifndef CLIENT_DLL

acttable_t	CWeaponSuperCrowbar::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SLAM, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SLAM, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SLAM, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SLAM, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SLAM, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SLAM, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SLAM, false },

	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },

};

IMPLEMENT_ACTTABLE(CWeaponSuperCrowbar);

#endif

LINK_ENTITY_TO_CLASS(weapon_supercrowbar, CWeaponSuperCrowbar);
PRECACHE_WEAPON_REGISTER(weapon_supercrowbar);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponSuperCrowbar::CWeaponSuperCrowbar(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponSuperCrowbar::GetDamageForActivity(Activity hitActivity)
{
	if ((GetOwner() != NULL) && (GetOwner()->IsPlayer()))
		return sk_plr_dmg_supercrowbar.GetFloat();

	return sk_npc_dmg_supercrowbar.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponSuperCrowbar::AddViewKick(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());;

	if (pPlayer == NULL)
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat(2.0f, 4.0f);
	punchAng.y = random->RandomFloat(-2.0f, -1.0f);
	punchAng.z = 0.0f;

	pPlayer->ViewPunch(punchAng);
}