//========= Copyright © 2023-2024, GameWave Studio, All rights reserved. ============//
//
// Purpose: Implements a camera tools/weapon.
//			
//			Primary attack: Screenshot
//			Secondary attack: Zoom (In/Out)
//
// Note: This Tools, Or Maybe Weapon? is Heavily Modifed From Sniper rifle weapon.
//====================================================================================//

#include "cbase.h"
#include "NPCEvent.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"				// For g_pGameRules
#include "in_buttons.h"
#include "soundent.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAMERA_CONE_PLAYER					vec3_origin	// Spread cone when fired by the player.
#define CAMERA_CONE_NPC						vec3_origin	// Spread cone when fired by NPCs.
#define CAMERA_BULLET_COUNT_PLAYER			1			// Fire n bullets per shot fired by the player.
#define CAMERA_BULLET_COUNT_NPC				1			// Fire n bullets per shot fired by NPCs.
#define CAMERA_TRACER_FREQUENCY_PLAYER		0			// Draw a tracer every nth shot fired by the player.
#define CAMERA_TRACER_FREQUENCY_NPC			0			// Draw a tracer every nth shot fired by NPCs.
#define CAMERA_KICKBACK						3			// Range for punchangle when firing.

#define CAMERA_ZOOM_RATE					0.15		// Interval between zoom levels in seconds.


//-----------------------------------------------------------------------------
// Discrete zoom levels for the scope.
//-----------------------------------------------------------------------------
static int g_nZoomFOV[] =
{
	0,
	40,
	30,
	20,
	10,
	5
};


class CWeaponCamera : public CBaseHL2MPCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS(CWeaponCamera, CBaseHL2MPCombatWeapon);

	CWeaponCamera(void);

	DECLARE_SERVERCLASS();

	void Precache(void);

	int CapabilitiesGet(void) const;

	const Vector &GetBulletSpread(void);

	bool Holster(CBaseHL2MPCombatWeapon *pSwitchingTo = NULL);
	void ItemPostFrame(void);
	void PrimaryAttack(void);
	bool Reload(void);
	void Zoom(void);
	virtual float GetFireRate(void) { return 1; };

	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	DECLARE_ACTTABLE();

protected:

	float m_fNextZoom;
	int m_nZoomLevel;
	color32 lightRed;  /*= { 50, 255, 170, 32 };*/
};


IMPLEMENT_SERVERCLASS_ST(CWeaponCamera, DT_WeaponCamera)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_camera, CWeaponCamera);
PRECACHE_WEAPON_REGISTER(weapon_camera);

BEGIN_DATADESC(CWeaponCamera)

DEFINE_FIELD(m_fNextZoom, FIELD_FLOAT),
DEFINE_FIELD(m_nZoomLevel, FIELD_INTEGER),

END_DATADESC()

//-----------------------------------------------------------------------------
// Maps base activities to weapons-specific ones so our characters do the right things.
//-----------------------------------------------------------------------------
acttable_t	CWeaponCamera::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_AR2, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_AR2, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_AR2, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_AR2, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_AR2, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_AR2, false },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR2, false },

	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE, ACT_IDLE_SMG1, true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },		// FIXME: hook to AR2 unique

	{ ACT_WALK, ACT_WALK_RIFLE, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims
	//End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_AR2, false },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_AR2_LOW, false },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true },
	//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
};

IMPLEMENT_ACTTABLE(CWeaponCamera);


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CWeaponCamera::CWeaponCamera(void)
{
	m_fNextZoom = gpGlobals->curtime;
	m_nZoomLevel = 0;

	m_bReloadsSingly = true;

	m_fMinRange1 = 65;
	m_fMinRange2 = 65;
	m_fMaxRange1 = 2048;
	m_fMaxRange2 = 2048;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponCamera::CapabilitiesGet(void) const
{
	return bits_CAP_WEAPON_RANGE_ATTACK1;
}



//-----------------------------------------------------------------------------
// Purpose: Turns off the zoom when the rifle is holstered.
//-----------------------------------------------------------------------------
bool CWeaponCamera::Holster(CBaseHL2MPCombatWeapon *pSwitchingTo)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer != NULL)
	{
		if (m_nZoomLevel != 0)
		{
			if (pPlayer->SetFOV(this, 0))
			{
				pPlayer->ShowViewModel(true);
				m_nZoomLevel = 0;
			}
		}
	}

	return BaseClass::Holster(pSwitchingTo);
}


//-----------------------------------------------------------------------------
// Purpose: Overloaded to handle the zoom functionality.
//-----------------------------------------------------------------------------
void CWeaponCamera::ItemPostFrame(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer == NULL)
	{
		return;
	}

	if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		FinishReload();
		m_bInReload = false;
	}

	if (pPlayer->m_nButtons & IN_ATTACK2)
	{
		if (m_fNextZoom <= gpGlobals->curtime)
		{
			Zoom();
			pPlayer->m_nButtons &= ~IN_ATTACK2;
		}
	}
	else if ((pPlayer->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		if ((m_iClip1 == 0 && UsesClipsForAmmo1()) || (!UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType)))
		{
			m_bFireOnEmpty = true;
		}

		// Fire underwater?
		if (pPlayer->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if (pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK)
			{
				m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if (pPlayer->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if (!((pPlayer->m_nButtons & IN_ATTACK) || (pPlayer->m_nButtons & IN_ATTACK2) || (pPlayer->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if (!HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime)
		{
			// weapon isn't useable, switch.
			if (!(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pPlayer->SwitchToNextBestWeapon(this))
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if (m_iClip1 == 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime)
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCamera::Precache(void)
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Same as base reload but doesn't change the owner's next attack time. This
//			lets us zoom out while reloading. This hack is necessary because our
//			ItemPostFrame is only called when the owner's next attack time has
//			expired.
// Output : Returns true if the weapon was reloaded, false if no more ammo.
//-----------------------------------------------------------------------------
bool CWeaponCamera::Reload(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
	{
		return false;
	}

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		int primary = min(GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));
		int secondary = min(GetMaxClip2() - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));

		if (primary > 0 || secondary > 0)
		{
			// Play reload on different channel as it happens after every fire
			// and otherwise steals channel away from fire sound
			WeaponSound(RELOAD);
			SendWeaponAnim(ACT_VM_RELOAD);

			m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

			m_bInReload = true;
		}

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCamera::PrimaryAttack(void)
{
	// Ensure the player has ammo
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	// Execute the console command on the client
	if (pPlayer->IsNetClient())
	{
		// Ensure the player is a valid network client
		engine->ClientCommand(pPlayer->edict(), "jpeg\n");
	}

	// Optionally, you can include the default primary attack behavior
	BaseClass::PrimaryAttack();
}


//-----------------------------------------------------------------------------
// Purpose: Zooms in using the sniper rifle scope.
//-----------------------------------------------------------------------------
void CWeaponCamera::Zoom(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
	{
		return;
	}

	if (m_nZoomLevel >= sizeof(g_nZoomFOV) / sizeof(g_nZoomFOV[0]))
	{
		if (pPlayer->SetFOV(this, 0))
		{
			pPlayer->ShowViewModel(true);

			/*
			// Hide hud
			engine->ServerCommand("cl_drawhud 0");

			// Zoom out to the default zoom level
			WeaponSound(SPECIAL2);
			m_nZoomLevel = 0;
			*/

			WeaponSound(SPECIAL2);

			// Execute the console command on the client
			if (pPlayer->IsNetClient())
			{
				// Ensure the player is a valid network client
				engine->ClientCommand(pPlayer->edict(), "cl_drawhud 1");
			}
		}
	}
	else
	{
		if (pPlayer->SetFOV(this, g_nZoomFOV[m_nZoomLevel]))
		{
			if (m_nZoomLevel == 0)
			{
				pPlayer->ShowViewModel(false);

				WeaponSound(SPECIAL2);

				// Execute the console command on the client
				if (pPlayer->IsNetClient())
				{
					// Ensure the player is a valid network client
					engine->ClientCommand(pPlayer->edict(), "cl_drawhud 0");
				}
			}

			WeaponSound(SPECIAL1);

			m_nZoomLevel++;
		}
	}

	m_fNextZoom = gpGlobals->curtime + CAMERA_ZOOM_RATE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : virtual const Vector&
//-----------------------------------------------------------------------------
const Vector &CWeaponCamera::GetBulletSpread(void)
{
	static Vector cone = CAMERA_CONE_PLAYER;
	return cone;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponCamera::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SNIPER_RIFLE_FIRE:
	{
		Vector vecShootOrigin, vecShootDir;
		vecShootOrigin = pOperator->Weapon_ShootPosition();

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		Vector vecSpread;
		if (npc)
		{
			vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);
			vecSpread = VECTOR_CONE_PRECALCULATED;
		}
		else
		{
			AngleVectors(pOperator->GetLocalAngles(), &vecShootDir);
			vecSpread = GetBulletSpread();
		}
		WeaponSound(SINGLE_NPC);
		pOperator->FireBullets(CAMERA_BULLET_COUNT_NPC, vecShootOrigin, vecShootDir, vecSpread, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, CAMERA_TRACER_FREQUENCY_NPC);
		pOperator->DoMuzzleFlash();
		break;
	}

	default:
	{
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
	}
}

