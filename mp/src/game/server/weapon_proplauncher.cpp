//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Prop Launcher
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "gamestats.h"
#include "props.h"

#include "weapon_hl2mpbasebasebludgeon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	PROPLAUNCHER_FASTEST_REFIRE_TIME		0.1f
#define PROPLAUNCHER_FASTEST_DRY_REFIRE_TIME	0.2f

#define	PROPLAUNCHER_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	PROPLAUNCHER_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

ConVar	PROPLAUNCHER_use_new_accuracy("proplauncher_use_new_accuracy", "1");

//-----------------------------------------------------------------------------
// CWeaponPropLauncher
//-----------------------------------------------------------------------------

class CWeaponPropLauncher : public CBaseHL2MPBludgeonWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponPropLauncher, CBaseHL2MPBludgeonWeapon);

	CWeaponPropLauncher(void);

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

	virtual const Vector& GetBulletSpread(void)
	{
		// Handle NPCs first
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if (GetOwner() && GetOwner()->IsNPC())
			return npcCone;

		static Vector cone;

		if (PROPLAUNCHER_use_new_accuracy.GetBool())
		{
			float ramp = RemapValClamped(m_flAccuracyPenalty,
				0.0f,
				PROPLAUNCHER_ACCURACY_MAXIMUM_PENALTY_TIME,
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
		return 0.5f;
	}

	DECLARE_ACTTABLE();

private:
	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;
	float	m_flAccuracyPenalty;
	int		m_nNumShotsFired;
};


IMPLEMENT_SERVERCLASS_ST(CWeaponPropLauncher, DT_WeaponPROPLAUNCHER)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_PROPLAUNCHER, CWeaponPropLauncher);
PRECACHE_WEAPON_REGISTER(weapon_PROPLAUNCHER);

BEGIN_DATADESC(CWeaponPropLauncher)

DEFINE_FIELD(m_flSoonestPrimaryAttack, FIELD_TIME),
DEFINE_FIELD(m_flLastAttackTime, FIELD_TIME),
DEFINE_FIELD(m_flAccuracyPenalty, FIELD_FLOAT), //NOTENOTE: This is NOT tracking game time
DEFINE_FIELD(m_nNumShotsFired, FIELD_INTEGER),

END_DATADESC()

acttable_t	CWeaponPropLauncher::m_acttable[] =
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


IMPLEMENT_ACTTABLE(CWeaponPropLauncher);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPropLauncher::CWeaponPropLauncher(void)
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1 = 24;
	m_fMaxRange1 = 1500;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 200;

	m_bFiresUnderwater = true;
	m_flNextPrimaryAttack = gpGlobals->curtime;
}

const char* g_PropModels[] = {
	"models/props_junk/bicycle01a.mdl",
	"models/props_junk/cardboard_box001a.mdl",
	"models/props_junk/cardboard_box001a_gib01.mdl",
	"models/props_junk/cardboard_box001b.mdl",
	"models/props_junk/cardboard_box002a.mdl",
	"models/props_junk/cardboard_box002a_gib01.mdl",
	"models/props_junk/cardboard_box002b.mdl",
	"models/props_junk/cardboard_box003a.mdl",
	"models/props_junk/cardboard_box003a_gib01.mdl",
	"models/props_junk/cardboard_box003b.mdl",
	"models/props_junk/cardboard_box003b_gib01.mdl",
	"models/props_junk/cardboard_box004a.mdl",
	"models/props_junk/cardboard_box004a_gib01.mdl",
	"models/props_junk/cinderblock01a.mdl",
	"models/props_junk/garbage128_composite001a.mdl",
	"models/props_junk/garbage128_composite001b.mdl",
	"models/props_junk/garbage128_composite001c.mdl",
	"models/props_junk/garbage128_composite001d.mdl",
	"models/props_junk/garbage256_composite001a.mdl",
	"models/props_junk/garbage256_composite001b.mdl",
	"models/props_junk/garbage256_composite002a.mdl",
	"models/props_junk/garbage256_composite002b.mdl",
	"models/props_junk/garbage_bag001a.mdl",
	"models/props_junk/garbage_carboard001a.mdl",
	"models/props_junk/garbage_carboard002a.mdl",
	"models/props_junk/garbage_coffeemug001a.mdl",
	"models/props_junk/garbage_coffeemug001a_chunk01.mdl",
	"models/props_junk/garbage_coffeemug001a_chunk02.mdl",
	"models/props_junk/garbage_coffeemug001a_chunk03.mdl",
	"models/props_junk/garbage_glassbottle001a.mdl",
	"models/props_junk/garbage_glassbottle001a_chunk01.mdl",
	"models/props_junk/garbage_glassbottle001a_chunk02.mdl",
	"models/props_junk/garbage_glassbottle001a_chunk03.mdl",
	"models/props_junk/garbage_glassbottle001a_chunk04.mdl",
	"models/props_junk/garbage_glassbottle002a.mdl",
	"models/props_junk/garbage_glassbottle002a_chunk01.mdl",
	"models/props_junk/garbage_glassbottle002a_chunk02.mdl",
	"models/props_junk/garbage_glassbottle003a.mdl",
	"models/props_junk/garbage_glassbottle003a_chunk01.mdl",
	"models/props_junk/garbage_glassbottle003a_chunk02.mdl",
	"models/props_junk/garbage_glassbottle003a_chunk03.mdl",
	"models/props_junk/garbage_metalcan001a.mdl",
	"models/props_junk/garbage_metalcan002a.mdl",
	"models/props_junk/garbage_milkcarton001a.mdl",
	"models/props_junk/garbage_milkcarton002a.mdl",
	"models/props_junk/garbage_newspaper001a.mdl",
	"models/props_junk/garbage_plasticbottle001a.mdl",
	"models/props_junk/garbage_plasticbottle002a.mdl",
	"models/props_junk/garbage_plasticbottle003a.mdl",
	"models/props_junk/garbage_takeoutcarton001a.mdl",
	"models/props_junk/gascan001a.mdl",
	"models/props_junk/glassbottle01a.mdl",
	"models/props_junk/glassbottle01a_chunk01a.mdl",
	"models/props_junk/glassbottle01a_chunk02a.mdl",
	"models/props_junk/glassjug01.mdl",
	"models/props_junk/glassjug01_chunk01.mdl",
	"models/props_junk/glassjug01_chunk02.mdl",
	"models/props_junk/glassjug01_chunk03.mdl",
	"models/props_junk/harpoon002a.mdl",
	"models/props_junk/ibeam01a.mdl",
	"models/props_junk/ibeam01a_cluster01.mdl",
	"models/props_junk/meathook001a.mdl",
	"models/props_junk/metalbucket01a.mdl",
	"models/props_junk/metalbucket02a.mdl",
	"models/props_junk/metalgascan.mdl",
	"models/props_junk/metal_paintcan001a.mdl",
	"models/props_junk/metal_paintcan001b.mdl",
	"models/props_junk/plasticbucket001a.mdl",
	"models/props_junk/plasticcrate01a.mdl",
	"models/props_junk/popcan01a.mdl",
	"models/props_junk/propanecanister001a.mdl",
	"models/props_junk/propane_tank001a.mdl",
	"models/props_junk/pushcart01a.mdl",
	"models/props_junk/ravenholmsign.mdl",
	"models/props_junk/rock001a.mdl",
	"models/props_junk/sawblade001a.mdl",
	"models/props_junk/shoe001a.mdl",
	"models/props_junk/shovel01a.mdl",
	"models/props_junk/terracotta01.mdl",
	"models/props_junk/terracotta_chunk01a.mdl",
	"models/props_junk/terracotta_chunk01b.mdl",
	"models/props_junk/terracotta_chunk01f.mdl",
	"models/props_junk/trafficcone001a.mdl",
	"models/props_junk/trashbin01a.mdl",
	"models/props_junk/trashcluster01a.mdl",
	"models/props_junk/trashdumpster01a.mdl",
	"models/props_junk/trashdumpster02.mdl",
	"models/props_junk/trashdumpster02b.mdl",
	"models/props_junk/vent001.mdl",
	"models/props_junk/vent001_chunk1.mdl",
	"models/props_junk/vent001_chunk2.mdl",
	"models/props_junk/vent001_chunk3.mdl",
	"models/props_junk/vent001_chunk4.mdl",
	"models/props_junk/vent001_chunk5.mdl",
	"models/props_junk/vent001_chunk6.mdl",
	"models/props_junk/vent001_chunk7.mdl",
	"models/props_junk/vent001_chunk8.mdl",
	"models/props_junk/watermelon01.mdl",
	"models/props_junk/watermelon01_chunk01a.mdl",
	"models/props_junk/watermelon01_chunk01b.mdl",
	"models/props_junk/watermelon01_chunk01c.mdl",
	"models/props_junk/watermelon01_chunk02a.mdl",
	"models/props_junk/watermelon01_chunk02b.mdl",
	"models/props_junk/watermelon01_chunk02c.mdl",
	"models/props_junk/wheebarrow01a.mdl",
	"models/props_junk/wood_crate001a.mdl",
	"models/props_junk/wood_crate001a_chunk01.mdl",
	"models/props_junk/wood_crate001a_chunk02.mdl",
	"models/props_junk/wood_crate001a_chunk03.mdl",
	"models/props_junk/wood_crate001a_chunk04.mdl",
	"models/props_junk/wood_crate001a_chunk05.mdl",
	"models/props_junk/wood_crate001a_chunk06.mdl",
	"models/props_junk/wood_crate001a_chunk07.mdl",
	"models/props_junk/wood_crate001a_chunk09.mdl",
	"models/props_junk/wood_crate001a_damaged.mdl",
	"models/props_junk/wood_crate001a_damagedmax.mdl",
	"models/props_junk/wood_crate002a.mdl",
	"models/props_junk/wood_pallet001a.mdl",
	"models/props_junk/wood_pallet001a_chunka.mdl",
	"models/props_junk/wood_pallet001a_chunka1.mdl",
	"models/props_junk/wood_pallet001a_chunka3.mdl",
	"models/props_junk/wood_pallet001a_chunka4.mdl",
	"models/props_junk/wood_pallet001a_chunkb2.mdl",
	"models/props_junk/wood_pallet001a_chunkb3.mdl",
	"models/props_junk/wood_pallet001a_shard01.mdl"
};

const char* GetRandomPropModel()
{
	int index = random->RandomInt(0, ARRAYSIZE(g_PropModels) - 1);
	return g_PropModels[index];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::Precache(void)
{
	BaseClass::Precache();
	for (int i = 0; i < ARRAYSIZE(g_PropModels); ++i)
	{
		PrecacheModel(g_PropModels[i]);
	}
	PrecacheScriptSound("Weapon_Pistol.Single");
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
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
void CWeaponPropLauncher::DryFire(void)
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flSoonestPrimaryAttack = gpGlobals->curtime + PROPLAUNCHER_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::PrimaryAttack(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	Vector vecShootOrigin = pOwner->Weapon_ShootPosition();
	Vector vecShootDir = pOwner->GetAutoaimVector(AUTOAIM_2DEGREES);
	const char* propModel = GetRandomPropModel();
	CBaseEntity *pProp = CreateEntityByName("prop_physics");
	if (pProp)
	{
		pProp->SetModel(propModel);
		pProp->SetAbsOrigin(vecShootOrigin);
		pProp->SetAbsAngles(pOwner->EyeAngles());
		pProp->Spawn();

		IPhysicsObject *pPhysicsObject = pProp->VPhysicsGetObject();
		if (pPhysicsObject)
		{
			Vector vecVelocity = vecShootDir * 1000.0f;
			pPhysicsObject->AddVelocity(&vecVelocity, NULL);
		}
	}

	WeaponSound(SINGLE);
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pOwner->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.05f;
	m_flLastAttackTime = gpGlobals->curtime;

	//m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pOwner, true, GetClassname());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::UpdatePenaltyTime(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	// Check our penalty time decay
	if (((pOwner->m_nButtons & IN_ATTACK) == false) && (m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f, PROPLAUNCHER_ACCURACY_MAXIMUM_PENALTY_TIME);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::ItemPreFrame(void)
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::ItemBusyFrame(void)
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	if (m_bInReload)
		return;

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	//Allow a refire as fast as the player can click
	if (((pOwner->m_nButtons & IN_ATTACK) == false) && (m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack < gpGlobals->curtime) && (m_iClip1 <= 0))
	{
		DryFire();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponPropLauncher::GetPrimaryAttackActivity(void)
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
//-----------------------------------------------------------------------------
bool CWeaponPropLauncher::Reload(void)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropLauncher::AddViewKick(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat(0.25f, 0.5f);
	viewPunch.y = random->RandomFloat(-.6f, .6f);
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch(viewPunch);
}