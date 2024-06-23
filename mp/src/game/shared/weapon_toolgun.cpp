//========= Copyright Wheatley Laboratories, All rights reserved. ============//
//
// Purpose:		Toolgun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "vphysics/constraints.h"

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
#include "ipredictionsystem.h"
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
#include "recipientfilter.h"
#endif

#ifndef CLIENT_DLL
#include "EntityFlame.h"
#include "EntityDissolve.h"
#include "physics.h"
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

private:
	bool m_bWeldFirstClick;
	CBaseEntity* m_pFirstWeldEntity;
	Vector m_vFirstWeldPos;

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
		MODE_WELD,
		MODE_MAX
	};

	ToolgunMode m_Mode;

	void SwitchMode();
	void NotifyMode(CBasePlayer* pPlayer);
	void WeldObjects(CBaseEntity* pEntity1, CBaseEntity* pEntity2);
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
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PISTOL, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PISTOL, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PISTOL, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PISTOL, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_PISTOL, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PISTOL, false },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, false },

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

	m_bWeldFirstClick = false;
	m_pFirstWeldEntity = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::Precache(void)
{
	BaseClass::Precache();

	g_beam1 = PrecacheModel(BEAM_SPRITE1);
	PrecacheMaterial(BEAM_SPRITE1);
	PrecacheParticleSystem("muzzle_smg1");
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
		pOperator->FireBullets(1, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);
		pOperator->DoMuzzleFlash();
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
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponToolgun::PrimaryAttack(void)
{
	if ((gpGlobals->curtime - m_flLastAttackTime) > 0.5f)
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		m_nNumShotsFired++;
	}

	m_flLastAttackTime = gpGlobals->curtime;

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecAiming = pOwner->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	Vector vecRight, vecUp;
	VectorVectors(vecAiming, vecRight, vecUp);

	float x, y, z;

	do {
		x = random->RandomFloat(-0.5f, 0.5f) + random->RandomFloat(-0.5f, 0.5f);
		y = random->RandomFloat(-0.5f, 0.5f) + random->RandomFloat(-0.5f, 0.5f);
		z = x * x + y * y;
	} while (z > 1);

	Vector vecDir = vecAiming +
		x * GetBulletSpread().x * vecRight +
		y * GetBulletSpread().y * vecUp;

	VectorNormalize(vecDir);

	trace_t tr;
	UTIL_TraceLine(vecSrc, vecSrc + vecDir * MAX_TRACE_LENGTH, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

	DispatchParticleEffect("muzzle_smg1", tr.endpos, vec3_angle);

	switch (m_Mode)
	{
	case MODE_DELETE:
#pragma warning(push)
#pragma warning(disable: 4706)
		if (dynamic_cast<CBaseAnimating*>(tr.m_pEnt) && !tr.m_pEnt->IsPlayer())
		{
#ifndef CLIENT_DLL
			QAngle vecAngles(0, GetAbsAngles().y - 90, 0);

			Vector vForward, vRight, vUp;
			pOwner->EyeVectors(&vForward, &vRight, &vUp);
			Vector m_p = pOwner->Weapon_ShootPosition() + vForward + vRight + vUp;
			Vector vecAiming = pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);

			trace_t tr;
			UTIL_TraceLine(m_p, m_p + vForward * MAX_TRACE_LENGTH, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
			if (tr.fraction == 1.0)
				return;
			Vector vecShootOrigin, vecShootDir;
			Vector VPhysicsGetObject;
			vecShootOrigin = pOwner->Weapon_ShootPosition();

			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;

			if (tr.m_pEnt->IsNPC() || tr.m_pEnt->VPhysicsGetObject())
			{
				UTIL_Remove(m_pIgniter);
				CEntityDissolve::Create(tr.m_pEnt, STRING(m_szModelName), gpGlobals->curtime, NULL);
			}
#endif // !CLIENT_DLL
		}
		break;
		break;

	case MODE_IGNITER:
	{
#ifndef CLIENT_DLL
		CEntityFlame *pZhopa = CEntityFlame::Create(tr.m_pEnt, true);
		pZhopa->SetLifetime(10e10);
#endif
		break;
	}
	case MODE_LIGHT:
		CreateFlashlight(tr.endpos);
		break;

	case MODE_WELD:
		if (!m_bWeldFirstClick)
		{
			ShowModeSwitchMessage("great! now click on the second prop..\n");
			m_bWeldFirstClick = true;
			m_pFirstWeldEntity = tr.m_pEnt;
			m_vFirstWeldPos = tr.endpos;
		}
		else
		{
			ShowModeSwitchMessage("welded!\n");
			m_bWeldFirstClick = false;
			CBaseEntity* pSecondEntity = tr.m_pEnt;

			if (m_pFirstWeldEntity && pSecondEntity)
			{
				WeldObjects(m_pFirstWeldEntity, pSecondEntity);
			}
		}
		break;

	default:
		break;
	}

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer)
	{
		pPlayer->ViewPunch(QAngle(random->RandomFloat(-0.25f, -0.5f), random->RandomFloat(-0.6f, 0.6f), 0));
	}

	//Disorient the player
	AddViewKick();

	if (!pPlayer)
		return;

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	// Don't fire again until fire animation has completed
	m_flSoonestPrimaryAttack = gpGlobals->curtime + TOOLGUN_FASTEST_REFIRE_TIME;
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += TOOLGUN_ACCURACY_SHOT_PENALTY_TIME;

	// Allow a refire right away so players don't feel the drag
	m_iClip1--;

	if (m_iClip1 <= 0)
	{
		// Just shoot the thing dry
		if (!pPlayer->GetAmmoCount(m_iPrimaryAmmoType))
		{
			// HEV suit - indicate out of ammo condition
			pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		}
	}
}

void CWeaponToolgun::AddViewKick(void)
{
#define	EASY_DAMPEN			0.5f
#define	MAX_VERTICAL_KICK	1.0f	//Degrees
#define	SLIDE_LIMIT			2.0f	//Seconds

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat(0.25f, 0.5f);
	viewPunch.y = random->RandomFloat(-.6f, .6f);
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch(viewPunch);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
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
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	if (m_flNextModeSwitch <= gpGlobals->curtime)
	{
		CBasePlayer *pOwner = ToBasePlayer(GetOwner());
		if (pOwner && pOwner->m_afButtonPressed & IN_RELOAD)
		{
			SwitchMode();
			m_flNextModeSwitch = gpGlobals->curtime + 0.5f;
		}
	}
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
	if ((pOwner->m_nButtons & IN_ATTACK) == false)
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = max(0.0f, m_flAccuracyPenalty);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponToolgun::GetPrimaryAttackActivity(void)
{
	return ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponToolgun::Reload(void)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::SwitchMode()
{
	m_Mode = (ToolgunMode)((m_Mode + 1) % MODE_MAX);

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer)
	{
		NotifyMode(pPlayer);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::NotifyMode(CBasePlayer* pPlayer)
{
	const char* modeName = "";

	switch (m_Mode)
	{
	case MODE_DELETE:
		modeName = "Remover";
		break;
	case MODE_IGNITER:
		modeName = "Igniter";
		break;
	case MODE_LIGHT:
		modeName = "Light";
		break;
	case MODE_WELD:
		modeName = "Weld";
		break;
	default:
		break;
	}

	ShowModeSwitchMessage(modeName);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::ShowModeSwitchMessage(const char* modeName)
{
	if (CBasePlayer* pPlayer = ToBasePlayer(GetOwner()))
	{
#ifndef CLIENT_DLL
		char buf[256];
		Q_snprintf(buf, sizeof(buf), "%s\n", modeName);
		CRecipientFilter filter;
		filter.AddRecipient(pPlayer);
		UTIL_ClientPrintFilter(filter, HUD_PRINTTALK, buf);
#endif
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponToolgun::CreateFlashlight(const Vector& position)
{
	if (CBasePlayer* pPlayer = ToBasePlayer(GetOwner()))
	{
#ifndef CLIENT_DLL
		engine->ClientCommand(pPlayer->edict(), "create_flashlight");
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Weld two objects together
//-----------------------------------------------------------------------------
void CWeaponToolgun::WeldObjects(CBaseEntity* pEntity1, CBaseEntity* pEntity2)
{
	if (pEntity1 && pEntity2)
	{
		IPhysicsObject* pPhys1 = pEntity1->VPhysicsGetObject();
		IPhysicsObject* pPhys2 = pEntity2->VPhysicsGetObject();

		if (pPhys1 && pPhys2)
		{
			constraint_fixedparams_t fixedConstraint;
			fixedConstraint.Defaults();
			fixedConstraint.InitWithCurrentObjectState(pPhys1, pPhys2);

			physenv->CreateFixedConstraint(pPhys1, pPhys2, nullptr, fixedConstraint);
		}
	}
}
