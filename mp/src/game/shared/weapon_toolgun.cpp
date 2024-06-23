//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Toolgun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#ifdef CLIENT_DLL
#include "npcevent.h"
#include "c_basehlcombatweapon.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Label.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui/ILocalize.h"
#include "beam_shared.h"
#include "props_shared.h"
#else
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
#endif

#ifndef CLIENT_DLL
#include "EntityFlame.h"
#include "EntityDissolve.h"
#endif

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

class CWeaponToolgun :
	public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponToolgun,
	CBaseHLCombatWeapon
		);

	CWeaponToolgun(void);

#ifndef CLIENT_DLL
	DECLARE_SERVERCLASS();
#endif

	void	Precache(void);
	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);
	void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	void CreateFlashlight(const Vector& position);

	void	UpdatePenaltyTime(void);

#ifndef CLIENT_DLL
	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
#endif

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

#ifndef CLIENT_DLL
private:
	CHandle<CEntityFlame> m_pIgniter;
	CHandle<CEntityDissolve> m_pDissolver;
	string_t m_szModelName;
#endif

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
		MODE_LIGHT,
		MODE_MAX
	};

	ToolgunMode m_Mode;

	void SwitchMode();
	void NotifyMode(CBasePlayer* pPlayer);
};

#ifndef CLIENT_DLL
IMPLEMENT_SERVERCLASS_ST(CWeaponToolgun, DT_WeaponToolgun)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_toolgun, CWeaponToolgun);
#endif

PRECACHE_WEAPON_REGISTER(weapon_toolgun);

BEGIN_DATADESC(CWeaponToolgun)

DEFINE_FIELD(m_flSoonestPrimaryAttack, FIELD_TIME),
DEFINE_FIELD(m_flLastAttackTime, FIELD_TIME),
DEFINE_FIELD(m_flAccuracyPenalty, FIELD_FLOAT), //NOTENOTE: This is NOT tracking game time
DEFINE_FIELD(m_nNumShotsFired, FIELD_INTEGER),

END_DATADESC()

acttable_t CWeaponToolgun::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },

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


IMPLEMENT_ACTTABLE( CWeaponToolgun );

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
#ifndef CLIENT_DLL
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
#endif
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

	/*CBasePlayer* pOwner = nullptr;
	QAngle vecAngles;
	Vector vForward, vRight, vUp, muzzlePoint, vecAiming, vecShootOrigin, vecShootDir;
	IPhysicsObject *VPhysicsGetObject = nullptr;*/

	switch (m_Mode)
	{
	case MODE_DELETE:
	{
#pragma warning(push)
#pragma warning(disable: 4706)
		if (dynamic_cast<CBaseAnimating*>(tr.m_pEnt) && !tr.m_pEnt->IsPlayer())
		{
#ifndef CLIENT_DLL
			QAngle vecAngles(0, GetAbsAngles().y - 90, 0);

			Vector vForward, vRight, vUp;
			pPlayer->EyeVectors(&vForward, &vRight, &vUp);
			Vector muzzlePoint = pPlayer->Weapon_ShootPosition() + vForward + vRight + vUp;
			Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

			trace_t tr;
			UTIL_TraceLine(muzzlePoint, muzzlePoint + vForward * MAX_TRACE_LENGTH, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
			if (tr.fraction == 1.0)
				return;
			Vector vecShootOrigin, vecShootDir;
			Vector VPhysicsGetObject;
			vecShootOrigin = pPlayer->Weapon_ShootPosition();

			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;

			if (tr.m_pEnt->IsNPC() || tr.m_pEnt->VPhysicsGetObject())
			{
				UTIL_Remove(m_pIgniter);
				m_pDissolver = CEntityDissolve::Create(tr.m_pEnt, STRING(m_szModelName), gpGlobals->curtime, NULL);
			}
#endif // !CLIENT_DLL
		}
		break;
	}
	case MODE_IGNITER:
	{
#pragma warning(push)
#pragma warning(disable: 4706)
#ifndef CLIENT_DLL
		QAngle vecAngles(0, GetAbsAngles().y - 90, 0);

		Vector vForward, vRight, vUp;
		pPlayer->EyeVectors(&vForward, &vRight, &vUp);
		Vector muzzlePoint = pPlayer->Weapon_ShootPosition() + vForward + vRight + vUp;
		Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

		UTIL_TraceLine(muzzlePoint, muzzlePoint + vForward * MAX_TRACE_LENGTH, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction == 1.0)
			return;

		if (tr.m_pEnt->IsNPC() || (tr.m_pEnt->VPhysicsGetObject()))
		{
			if (m_pIgniter)
			{
				UTIL_Remove(m_pIgniter);
			}
			m_pIgniter = CEntityFlame::Create(tr.m_pEnt, true);
			if (tr.m_pEnt->IsPlayer())
			{
				ClientPrint(pPlayer, HUD_PRINTCONSOLE, "brotha it cant be set on fire");
			}
		}
#endif
		break;
	}
#pragma warning(pop)
	case MODE_LIGHT:
		break;
	}

	WeaponSound(SINGLE);
	SendWeaponAnim(GetPrimaryAttackActivity());
	pPlayer->SetAnimation(PLAYER_ATTACK1);
	AddViewKick();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flSoonestPrimaryAttack = gpGlobals->curtime + TOOLGUN_FASTEST_REFIRE_TIME;

	m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
#endif
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
	case MODE_LIGHT:
		modeString = "Light";
		break;
	default:
		modeString = "Unknown Mode";
		break;
	}

#ifdef CLIENT_DLL
	
#endif
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