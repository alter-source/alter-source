//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Ice Axe - all purpose pointy beating stick
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "vstdlib/random.h"

#define	ICEAXE_RANGE	60
#define	ICEAXE_REFIRE	0.25f

ConVar    sk_plr_dmg_iceaxe("sk_plr_dmg_iceaxe", "10");
ConVar    sk_npc_dmg_iceaxe("sk_npc_dmg_iceaxe", "1");

//-----------------------------------------------------------------------------
// CWeaponIceAxe
//-----------------------------------------------------------------------------

class CWeaponIceAxe : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponIceAxe, CBaseHL2MPBludgeonWeapon);

	DECLARE_SERVERCLASS();

	CWeaponIceAxe();

	float		GetRange(void)		{ return	ICEAXE_RANGE; }
	float		GetFireRate(void)		{ return	ICEAXE_REFIRE; }

	void		ImpactSound(bool isWorld)	{ return; }
	
	virtual void			AddViewmodelBob(CBaseViewModel *viewmodel, Vector &origin, QAngle &angles) {};
	virtual float			CalcViewmodelBob(void) { return 0.0f; };

	void		AddViewKick(void);
	float		GetDamageForActivity(Activity hitActivity);
	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponIceAxe, DT_WeaponIceAxe)
END_SEND_TABLE()

#ifndef CLIENT_DLL

acttable_t	CWeaponIceAxe::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_MELEE, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_MELEE, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_MELEE, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_MELEE, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_MELEE, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_MELEE, false },

	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
};

IMPLEMENT_ACTTABLE(CWeaponIceAxe);

#endif

LINK_ENTITY_TO_CLASS(weapon_iceaxe, CWeaponIceAxe);
PRECACHE_WEAPON_REGISTER(weapon_iceaxe);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponIceAxe::CWeaponIceAxe(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponIceAxe::GetDamageForActivity(Activity hitActivity)
{
	if ((GetOwner() != NULL) && (GetOwner()->IsPlayer()))
		return sk_plr_dmg_iceaxe.GetFloat();

	return sk_npc_dmg_iceaxe.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponIceAxe::AddViewKick(void)
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