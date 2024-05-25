//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Toolgun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "gamestats.h"
#include "beam_shared.h"
#include "props.h"
#include "baseentity.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Frame.h>

#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	TOOLGUN_FASTEST_REFIRE_TIME		0.1f
#define TOOLGUN_FASTEST_DRY_REFIRE_TIME	0.2f

#define	TOOLGUN_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	TOOLGUN_ACCURACY_MAXIMUM_PENALTY_TIME	0.3f	// Maximum penalty to deal out

ConVar	toolgun_use_new_accuracy("toolgun_use_new_accuracy", "1");

static int g_beam1;
#define BEAM_SPRITE1	"sprites/physbeam1.vmt"

//-----------------------------------------------------------------------------
// CWeaponToolgun
//-----------------------------------------------------------------------------

class CWeaponToolgun : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponToolgun, CBaseHLCombatWeapon);

	CWeaponToolgun(void);

	DECLARE_SERVERCLASS();

	void	Precache(void);
	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);
	void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	void	UpdatePenaltyTime(void);

	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	Activity	GetPrimaryAttackActivity(void);

	virtual bool Reload(void);

	void ShowModeSwitchMessage(const char* modeName);
	void DeleteModeSwitchMessage();

	virtual const Vector& GetBulletSpread(void)
	{
		// Handle NPCs first
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if (GetOwner() && GetOwner()->IsNPC())
			return npcCone;

		static Vector cone;

		if (toolgun_use_new_accuracy.GetBool())
		{
			float ramp = RemapValClamped(m_flAccuracyPenalty,
				0.0f,
				TOOLGUN_ACCURACY_MAXIMUM_PENALTY_TIME,
				0.0f,
				1.0f);

			// We lerp from very accurate to inaccurate over time
			VectorLerp(VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone);
		}
		else
		{
			// Old value
			cone = VECTOR_CONE_4DEGREES;
		}

		return cone;
	}

	virtual int	GetMinBurst()
	{
		return 1;
	}

	virtual int	GetMaxBurst()
	{
		return 3;
	}

	virtual float GetFireRate(void)
	{
		return 0.314f;
	}

	DECLARE_ACTTABLE();

private:
	CBeam *m_pBeam;

private:
	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;
	float	m_flAccuracyPenalty;
	int		m_nNumShotsFired;

	float m_flNextModeSwitch;

	enum ToolgunMode
	{
		MODE_DELETE = 0,
		MODE_IGNITER,
		MODE_DUPLICATOR,
		MODE_MAX
	};

	ToolgunMode m_Mode;

	void SwitchMode();
	void NotifyMode(CBasePlayer* pPlayer);
};


IMPLEMENT_SERVERCLASS_ST(CWeaponToolgun, DT_WeaponToolgun)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_toolgun, CWeaponToolgun);
PRECACHE_WEAPON_REGISTER(weapon_toolgun);

BEGIN_DATADESC(CWeaponToolgun)

DEFINE_FIELD(m_flSoonestPrimaryAttack, FIELD_TIME),
DEFINE_FIELD(m_flLastAttackTime, FIELD_TIME),
DEFINE_FIELD(m_flAccuracyPenalty, FIELD_FLOAT), //NOTENOTE: This is NOT tracking game time
DEFINE_FIELD(m_nNumShotsFired, FIELD_INTEGER),

END_DATADESC()

acttable_t	CWeaponToolgun::m_acttable[] =
{
	{ ACT_IDLE, ACT_IDLE_PISTOL, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_PISTOL, true },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, true },
	{ ACT_RELOAD, ACT_RELOAD_PISTOL, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_PISTOL, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_PISTOL, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_PISTOL, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD_PISTOL_LOW, false },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_PISTOL_LOW, false },
	{ ACT_COVER_LOW, ACT_COVER_PISTOL_LOW, false },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_PISTOL_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_PISTOL, false },
	{ ACT_WALK, ACT_WALK_PISTOL, false },
	{ ACT_RUN, ACT_RUN_PISTOL, false },
};


IMPLEMENT_ACTTABLE(CWeaponToolgun);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponToolgun::CWeaponToolgun(void)
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1 = 24;
	m_fMaxRange1 = 1500;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 200;

	m_bFiresUnderwater = true;
	m_flNextModeSwitch = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::Precache(void)
{
	BaseClass::Precache();

	g_beam1 = PrecacheModel(BEAM_SPRITE1);
	PrecacheParticleSystem("blood_impact_red_01");
	PrecacheMaterial(BEAM_SPRITE1);
	PrecacheParticleSystem("impact_fx");
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponToolgun::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_PISTOL_FIRE:
	{
		Vector vecShootOrigin, vecShootDir;
		vecShootOrigin = pOperator->Weapon_ShootPosition();

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		ASSERT(npc != NULL);

		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

		CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

		WeaponSound(SINGLE_NPC);
		pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);
		pOperator->DoMuzzleFlash();
		m_iClip1 = m_iClip1 - 1;
	}
	break;
	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponToolgun::DryFire(void)
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flSoonestPrimaryAttack = gpGlobals->curtime + TOOLGUN_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponToolgun::PrimaryAttack(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
	{
		return;
	}

	Vector vecStart = pPlayer->EyePosition();
	Vector vecForward;
	pPlayer->EyeVectors(&vecForward);

	Vector vecEnd = vecStart + (vecForward * MAX_TRACE_LENGTH);
	trace_t tr;
	UTIL_TraceLine(vecStart, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

	if (tr.m_pEnt && tr.m_pEnt->IsSolid())
	{
		switch (m_Mode)
		{
		case MODE_DELETE:
			if (dynamic_cast<CBaseAnimating*>(tr.m_pEnt) && !tr.m_pEnt->IsPlayer())
			{
				DispatchParticleEffect("impact_fx", tr.m_pEnt->GetAbsOrigin(), tr.m_pEnt->GetAbsAngles());
				UTIL_Remove(tr.m_pEnt);
			}
			break;
		case MODE_IGNITER:
			if (!tr.m_pEnt->IsPlayer())
			{
				CBaseProp* pProp = dynamic_cast<CBaseProp*>(tr.m_pEnt);
				if (pProp)
				{
					if (IsServer()) {
						pProp->Ignite(60.0f, false, 0.0f, true);
					}
					else {
						Warning("not server");
					}
				}
			}
		case MODE_DUPLICATOR:
			if (!tr.m_pEnt->IsPlayer() && dynamic_cast<CBaseAnimating*>(tr.m_pEnt))
			{
				CBaseEntity* pDuplicate = CreateEntityByName(tr.m_pEnt->GetClassname());
				pDuplicate->SetAbsOrigin(tr.m_pEnt->GetAbsOrigin() + Vector(50, 0, 0));
				pDuplicate->SetAbsAngles(tr.m_pEnt->GetAbsAngles());
				DispatchSpawn(pDuplicate);
			}
			break;
		}
	}

	WeaponSound(SINGLE);
	SendWeaponAnim(GetPrimaryAttackActivity());
	pPlayer->SetAnimation(PLAYER_ATTACK1);
	AddViewKick();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flSoonestPrimaryAttack = gpGlobals->curtime + TOOLGUN_FASTEST_REFIRE_TIME;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::UpdatePenaltyTime(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	// Check our penalty time decay
	if (((pOwner->m_nButtons & IN_ATTACK) == false) && (m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f, TOOLGUN_ACCURACY_MAXIMUM_PENALTY_TIME);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::ItemPreFrame(void)
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::ItemBusyFrame(void)
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponToolgun::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	// If the player released the reload button, reset in reload state
	if (m_bInReload && !(pOwner->m_nButtons & IN_RELOAD))
		m_bInReload = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponToolgun::GetPrimaryAttackActivity(void)
{
	if (m_nNumShotsFired < 1)
		return ACT_VM_PRIMARYATTACK;

	if (m_nNumShotsFired < 2)
		return ACT_VM_RECOIL1;

	if (m_nNumShotsFired < 3)
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
// Purpose: Switch between delete and duplicate modes
//-----------------------------------------------------------------------------
void CWeaponToolgun::SwitchMode()
{
	m_Mode = static_cast<ToolgunMode>((m_Mode + 1) % MODE_MAX);
}

//-----------------------------------------------------------------------------
// Purpose: Reload the toolgun
//-----------------------------------------------------------------------------
bool CWeaponToolgun::Reload(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return false;

	if (gpGlobals->curtime < m_flNextModeSwitch)
		return false;

	SwitchMode();
	NotifyMode(pOwner);

	m_flNextModeSwitch = gpGlobals->curtime + 0.314f;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Notify the player about the current mode
//-----------------------------------------------------------------------------
void CWeaponToolgun::NotifyMode(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	const char* modeString = nullptr;

	switch (m_Mode)
	{
	case MODE_DELETE:
		modeString = "Remover";
		break;
	case MODE_IGNITER:
		modeString = "Igniter";
		break;
	case MODE_DUPLICATOR:
		modeString = "Duplicator";
		break;
	default:
		modeString = "Unknown Mode";
		break;
	}

	char buf[256];
	Q_snprintf(buf, sizeof(buf), "%s\n", modeString);
	CRecipientFilter filter;
	filter.AddRecipient(pPlayer);
	UTIL_ClientPrintFilter(filter, HUD_PRINTTALK, buf);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::AddViewKick(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat(0.25f, 0.5f);
	viewPunch.y = random->RandomFloat(-.6f, .6f);
	viewPunch.z = 0.0f;

	pPlayer->ViewPunch(viewPunch);
}