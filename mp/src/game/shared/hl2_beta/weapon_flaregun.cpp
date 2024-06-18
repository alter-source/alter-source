//========= Copyright © 1996-2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: fssssssssssssssssssss
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "decals.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
	#include "input.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "IEffects.h"
	#include "Sprite.h"
	#include "SpriteTrail.h"
	#include "beam_shared.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "effect_dispatch_data.h"
#include "soundenvelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define BOLT_MODEL			"models/crossbow_bolt.mdl"
#define	SF_FLARE_NO_DLIGHT	0x00000001
#define	SF_FLARE_NO_SMOKE	0x00000002
#define	SF_FLARE_INFINITE	0x00000004
#define	SF_FLARE_START_OFF	0x00000008

#define	FLARE_DURATION		30.0f
#define FLARE_DECAY_TIME	10.0f
#define FLARE_BLIND_TIME	6.0f
#define	FLARE_LAUNCH_SPEED	1500


#ifndef CLIENT_DLL

//extern ConVar sk_plr_dmg_crossbow;
//extern ConVar sk_npc_dmg_crossbow;

//void TE_StickyBolt( IRecipientFilter& filter, float delay,	Vector vecDirection, const Vector *origin );

//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CFlare : public CBaseCombatCharacter
{
public:
	DECLARE_CLASS( CFlare, CBaseCombatCharacter );

	CFlare();
	~CFlare();

	static CFlare *	GetActiveFlares( void );
	CFlare *		GetNextFlare( void ) const { return m_pNextFlare; }

	static CFlare *Create( Vector vecOrigin, QAngle vecAngles, CBaseEntity *pOwner, float lifetime );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	void	Spawn( void );
	void	Precache( void );
	int		Restore( IRestore &restore );
	void	Activate( void );

	void	StartBurnSound( void );

	void	Start( float lifeTime );
	void	Die( float fadeTime );
	void	Launch( const Vector &direction, float speed );
#ifdef CLIENT_DLL
	Class_T Classify( void );
#endif
	void	FlareTouch( CBaseEntity *pOther );
	void	FlareBurnTouch( CBaseEntity *pOther );
	void	FlareThink( void );

	void	InputStart( inputdata_t &inputdata );
	void	InputDie( inputdata_t &inputdata );
	void	InputLaunch( inputdata_t &inputdata );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	static CFlare *activeFlares;

	CBaseEntity *m_pOwner;
	int			m_nBounces;			// how many times has this flare bounced?
	CNetworkVar( float, m_flTimeBurnOut );	// when will the flare burn out?
	CNetworkVar( float, m_flScale );
	float		m_flDuration;
	float		m_flNextDamage;
	
	CSoundPatch	*m_pBurnSound;
	bool		m_bFading;
	CNetworkVar( bool, m_bLight );
	CNetworkVar( bool, m_bSmoke );
	CNetworkVar( bool, m_bPropFlare );

	bool		m_bInActiveList;
	CFlare *	m_pNextFlare;

	void		RemoveFromActiveFlares( void );
	void		AddToActiveFlares( void );
};
LINK_ENTITY_TO_CLASS( env_flare, CFlare );

BEGIN_DATADESC( CFlare )

	DEFINE_FIELD( m_pOwner,			FIELD_CLASSPTR ),
	DEFINE_FIELD( m_nBounces,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flTimeBurnOut,	FIELD_TIME ),
	DEFINE_KEYFIELD( m_flScale,		FIELD_FLOAT, "scale" ),
	DEFINE_KEYFIELD( m_flDuration,	FIELD_FLOAT, "duration" ),
	DEFINE_FIELD( m_flNextDamage,	FIELD_TIME ),
	DEFINE_SOUNDPATCH( m_pBurnSound ),
	DEFINE_FIELD( m_bFading,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLight,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bSmoke,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPropFlare,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInActiveList,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_pNextFlare,		FIELD_CLASSPTR ),
#ifdef CLIENT_DLL	
	//Input functions
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Start", InputStart ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Die", InputDie ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Launch", InputLaunch),

	// Function Pointers
	DEFINE_FUNCTION( FlareTouch ),
	DEFINE_FUNCTION( FlareBurnTouch ),
	DEFINE_FUNCTION( FlareThink ),
#endif
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFlare, DT_Flare )
	SendPropFloat( SENDINFO( m_flTimeBurnOut ), 0,	SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flScale ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_bLight ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bSmoke ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bPropFlare ), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

CFlare *CFlare::activeFlares = NULL;

CFlare *CFlare::GetActiveFlares( void )
{
	return CFlare::activeFlares;
}
#ifdef CLIENT_DLL
Class_T CFlare::Classify( void )
{
	return CLASS_FLARE; 
}
#endif
CBaseEntity *CreateFlare( Vector vOrigin, QAngle Angles, CBaseEntity *pOwner, float flDuration )
{
	CFlare *pFlare = CFlare::Create( vOrigin, Angles, pOwner, flDuration );

	if ( pFlare )
	{
		pFlare->m_bPropFlare = true;
	}

	return pFlare;
}

void KillFlare( CBaseEntity *pOwnerEntity, CBaseEntity *pEntity, float flKillTime )
{
	CFlare *pFlare = dynamic_cast< CFlare *>( pEntity );

	if ( pFlare )
	{
		float flDieTime = (pFlare->m_flTimeBurnOut - gpGlobals->curtime) - flKillTime;

		if ( flDieTime > 1.0f )
		{
			pFlare->Die( flDieTime );
			pOwnerEntity->SetNextThink( gpGlobals->curtime + flDieTime + 3.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFlare::CFlare( void )
{
	m_flScale		= 1.0f;
	m_nBounces		= 0;
	m_bFading		= false;
	m_bLight		= true;
	m_bSmoke		= true;
	m_flNextDamage	= gpGlobals->curtime;
	m_lifeState		= LIFE_ALIVE;
	m_iHealth		= 100;
	m_bPropFlare	= false;
	m_bInActiveList	= false;
	m_pNextFlare	= NULL;
}

CFlare::~CFlare()
{
	CSoundEnvelopeController::GetController().SoundDestroy( m_pBurnSound );
	m_pBurnSound = NULL;

	RemoveFromActiveFlares();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::Precache( void )
{
	PrecacheModel("models/weapons/flare.mdl" );

	PrecacheScriptSound( "Weapon_FlareGun.Burn" );

  	// FIXME: needed to precache the fire model.  Shouldn't have to do this.
  	UTIL_PrecacheOther( "_firesmoke" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &restore - 
// Output : int
//-----------------------------------------------------------------------------
int CFlare::Restore( IRestore &restore )
{
	int result = BaseClass::Restore( restore );

	if ( m_spawnflags & SF_FLARE_NO_DLIGHT )
	{
		m_bLight = false;
	}

	if ( m_spawnflags & SF_FLARE_NO_SMOKE )
	{
		m_bSmoke = false;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::Spawn( void )
{
	Precache();

	SetModel( "models/weapons/flare.mdl" );

	UTIL_SetSize( this, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_NONE );
	SetFriction( 0.6f );
	SetGravity( UTIL_ScaleForGravity( 400 ) );
	m_flTimeBurnOut = gpGlobals->curtime + 30;

	AddEffects( EF_NOSHADOW|EF_NORECEIVESHADOW );

	if ( m_spawnflags & SF_FLARE_NO_DLIGHT )
	{
		m_bLight = false;
	}

	if ( m_spawnflags & SF_FLARE_NO_SMOKE )
	{
		m_bSmoke = false;
	}

	if ( m_spawnflags & SF_FLARE_INFINITE )
	{
		m_flTimeBurnOut = -1.0f;
	}

	if ( m_spawnflags & SF_FLARE_START_OFF )
	{
		AddEffects( EF_NODRAW );
	}

	AddFlag( FL_OBJECT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::Activate( void )
{
	BaseClass::Activate();

	// Start the burning sound if we're already on
	if ( ( m_spawnflags & SF_FLARE_START_OFF ) == false )
	{
		StartBurnSound();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::StartBurnSound( void )
{
	if ( m_pBurnSound == NULL )
	{
		CPASAttenuationFilter filter( this );
		m_pBurnSound = CSoundEnvelopeController::GetController().SoundCreate( 
			filter, entindex(), CHAN_WEAPON, "Weapon_FlareGun.Burn", 3.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecOrigin - 
//			vecAngles - 
//			*pOwner - 
// Output : CFlare
//-----------------------------------------------------------------------------
CFlare *CFlare::Create( Vector vecOrigin, QAngle vecAngles, CBaseEntity *pOwner, float lifetime )
{
	CFlare *pFlare = (CFlare *) CreateEntityByName( "env_flare" );

	if ( pFlare == NULL )
		return NULL;

	UTIL_SetOrigin( pFlare, vecOrigin );

	pFlare->SetLocalAngles( vecAngles );
	pFlare->Spawn();
	pFlare->SetTouch( &CFlare::FlareTouch );
	pFlare->SetThink( &CFlare::FlareThink );
	
	//Start up the flare
	pFlare->Start( lifetime );

	//Don't start sparking immediately
	pFlare->SetNextThink( gpGlobals->curtime + 0.5f );

	//Burn out time
	pFlare->m_flTimeBurnOut = gpGlobals->curtime + lifetime;

	pFlare->RemoveSolidFlags( FSOLID_NOT_SOLID );
	pFlare->AddSolidFlags( FSOLID_NOT_STANDABLE );

	pFlare->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );

	pFlare->SetOwnerEntity( pOwner );
	pFlare->m_pOwner	= pOwner;

	return pFlare;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
unsigned int CFlare::PhysicsSolidMaskForEntity( void ) const
{
	return MASK_NPCSOLID;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::FlareThink( void )
{
	float	deltaTime = ( m_flTimeBurnOut - gpGlobals->curtime );

	if ( !m_bInActiveList && ( ( deltaTime > FLARE_BLIND_TIME ) || ( m_flTimeBurnOut == -1.0f ) ) )
	{
		AddToActiveFlares();
	}

	if ( m_flTimeBurnOut != -1.0f )
	{
		//Fading away
		if ( ( deltaTime <= FLARE_DECAY_TIME ) && ( m_bFading == false ) )
		{
			m_bFading = true;
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pBurnSound, 60, deltaTime );
			CSoundEnvelopeController::GetController().SoundFadeOut( m_pBurnSound, deltaTime );
		}

		// if flare is no longer bright, remove it from active flare list
		if ( m_bInActiveList && ( deltaTime <= FLARE_BLIND_TIME ) )
		{
			RemoveFromActiveFlares();
		}

		//Burned out
		if ( m_flTimeBurnOut < gpGlobals->curtime )
		{
			UTIL_Remove( this );
			return;
		}
	}
	
	//Act differently underwater
	if ( GetWaterLevel() > 1 )
	{
		UTIL_Bubbles( GetAbsOrigin() + Vector( -2, -2, -2 ), GetAbsOrigin() + Vector( 2, 2, 2 ), 1 );
		m_bSmoke = false;
	}
	else
	{
		//Shoot sparks
		if ( random->RandomInt( 0, 8 ) == 1 )
		{
			g_pEffects->Sparks( GetAbsOrigin() );
		}
	}

	//Next update
	SetNextThink( gpGlobals->curtime + 0.1f );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CFlare::FlareBurnTouch( CBaseEntity *pOther )
{
	if ( pOther && pOther->m_takedamage && ( m_flNextDamage < gpGlobals->curtime ) )
	{
		pOther->TakeDamage( CTakeDamageInfo( this, m_pOwner, 1, (DMG_BULLET|DMG_BURN) ) );
		m_flNextDamage = gpGlobals->curtime + 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CFlare::FlareTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	if ( ( m_nBounces < 10 ) && ( GetWaterLevel() < 1 ) )
	{
		// Throw some real chunks here
		g_pEffects->Sparks( GetAbsOrigin() );
	}

	//If the flare hit a person or NPC, do damage here.
	if ( pOther && pOther->m_takedamage )
	{
		/*
			The Flare is the iRifle round right now. No damage, just ignite. (sjb)

		//Damage is a function of how fast the flare is flying.
		int iDamage = GetAbsVelocity().Length() / 50.0f;

		if ( iDamage < 5 )
		{
			//Clamp minimum damage
			iDamage = 5;
		}

		//Use m_pOwner, not GetOwnerEntity()
		pOther->TakeDamage( CTakeDamageInfo( this, m_pOwner, iDamage, (DMG_BULLET|DMG_BURN) ) );
		m_flNextDamage = gpGlobals->curtime + 1.0f;
		*/

		CBaseAnimating *pAnim;

		pAnim = dynamic_cast<CBaseAnimating*>(pOther);
		if( pAnim )
		{
			pAnim->Ignite( 30.0f );
		}

		Vector vecNewVelocity = GetAbsVelocity();
		vecNewVelocity	*= 0.1f;
		SetAbsVelocity( vecNewVelocity );

		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
		SetGravity(1.0f);


		Die( 0.5 );

		return;
	}
	else
	{
		// hit the world, check the material type here, see if the flare should stick.
		trace_t tr;
		tr = CBaseEntity::GetTouchTrace();

		//Only do this on the first bounce
		if ( m_nBounces == 0 )
		{
			const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );	

			if ( pdata != NULL )
			{
				//Only embed into concrete and wood (jdw: too obscure for players?)
				//if ( ( pdata->gameMaterial == 'C' ) || ( pdata->gameMaterial == 'W' ) )
				{
					Vector	impactDir = ( tr.endpos - tr.startpos );
					VectorNormalize( impactDir );

					float	surfDot = tr.plane.normal.Dot( impactDir );

					//Do not stick to ceilings or on shallow impacts
					if ( ( tr.plane.normal.z > -0.5f ) && ( surfDot < -0.9f ) )
					{
						RemoveSolidFlags( FSOLID_NOT_SOLID );
						AddSolidFlags( FSOLID_TRIGGER );
						UTIL_SetOrigin( this, tr.endpos + ( tr.plane.normal * 2.0f ) );
						SetAbsVelocity( vec3_origin );
						SetMoveType( MOVETYPE_NONE );
						
						SetTouch( &CFlare::FlareBurnTouch );
						
						int index = decalsystem->GetDecalIndexForName( "SmallScorch" );
						if ( index >= 0 )
						{
							CBroadcastRecipientFilter filter;
							te->Decal( filter, 0.0, &tr.endpos, &tr.startpos, ENTINDEX( tr.m_pEnt ), tr.hitbox, index );
						}
						
						CPASAttenuationFilter filter2( this, "Flare.Touch" );
						EmitSound( filter2, entindex(), "Flare.Touch" );

						return;
					}
				}
			}
		}

		//Scorch decal
		if ( GetAbsVelocity().LengthSqr() > (250*250) )
		{
			int index = decalsystem->GetDecalIndexForName( "FadingScorch" );
			if ( index >= 0 )
			{
				CBroadcastRecipientFilter filter;
				te->Decal( filter, 0.0, &tr.endpos, &tr.startpos, ENTINDEX( tr.m_pEnt ), tr.hitbox, index );
			}
		}

		// Change our flight characteristics
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
		SetGravity( UTIL_ScaleForGravity( 640 ) );
		
		m_nBounces++;

		//After the first bounce, smacking into whoever fired the flare is fair game
		SetOwnerEntity( this );	

		// Slow down
		Vector vecNewVelocity = GetAbsVelocity();
		vecNewVelocity.x *= 0.8f;
		vecNewVelocity.y *= 0.8f;
		SetAbsVelocity( vecNewVelocity );

		//Stopped?
		if ( GetAbsVelocity().Length() < 64.0f )
		{
			SetAbsVelocity( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			AddSolidFlags( FSOLID_TRIGGER );
			SetTouch( &CFlare::FlareBurnTouch );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::Start( float lifeTime )
{
	StartBurnSound();

	if ( m_pBurnSound != NULL )
	{		
		CSoundEnvelopeController::GetController().Play( m_pBurnSound, 0.0f, 60 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pBurnSound, 0.8f, 2.0f );
		CSoundEnvelopeController::GetController().SoundChangePitch( m_pBurnSound, 100, 2.0f );
	}

	if ( lifeTime > 0 )
	{
		m_flTimeBurnOut = gpGlobals->curtime + lifeTime;
	}
	else
	{
		m_flTimeBurnOut = -1.0f;
	}

	RemoveEffects( EF_NODRAW );

	SetThink( &CFlare::FlareThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::Die( float fadeTime )
{
	m_flTimeBurnOut = gpGlobals->curtime + fadeTime;

	SetThink( &CFlare::FlareThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlare::Launch( const Vector &direction, float speed )
{
	// Make sure we're visible
	if ( m_spawnflags & SF_FLARE_INFINITE )
	{
		Start( -1 );
	}
	else
	{
		Start( 8.0f );
	}

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );

	// Punch our velocity towards our facing
	SetAbsVelocity( direction * speed );

	SetGravity( 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFlare::InputStart( inputdata_t &inputdata )
{
	Start( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFlare::InputDie( inputdata_t &inputdata )
{
	Die( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFlare::InputLaunch( inputdata_t &inputdata )
{
	Vector	direction;
	AngleVectors( GetAbsAngles(), &direction );

	float	speed = inputdata.value.Float();

	if ( speed == 0 )
	{
		speed = FLARE_LAUNCH_SPEED;
	}

	Launch( direction, speed );
}

//-----------------------------------------------------------------------------
// Purpose: Removes flare from active flare list
//-----------------------------------------------------------------------------
void CFlare::RemoveFromActiveFlares( void )
{
	CFlare *pFlare;
	CFlare *pPrevFlare;

	if ( !m_bInActiveList )
		return;

	pPrevFlare = NULL;
	for( pFlare = CFlare::activeFlares; pFlare != NULL; pFlare = pFlare->m_pNextFlare )
	{
		if ( pFlare == this )
		{
			if ( pPrevFlare )
			{
				pPrevFlare->m_pNextFlare = m_pNextFlare;
			}
			else
			{
				activeFlares = m_pNextFlare;
			}
			break;
		}
		pPrevFlare = pFlare;
	}

	m_pNextFlare = NULL;
	m_bInActiveList = false;
}

//-----------------------------------------------------------------------------
// Purpose: Adds flare to active flare list
//-----------------------------------------------------------------------------
void CFlare::AddToActiveFlares( void )
{
	if ( !m_bInActiveList )
	{
		m_pNextFlare = CFlare::activeFlares;
		CFlare::activeFlares = this;
		m_bInActiveList = true;
	}
}
#endif
//-----------------------------------------------------------------------------
// CWeaponCrossbow
//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL
#define CWeaponFlaregun C_WeaponFlaregun
#endif

class CWeaponFlaregun : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponFlaregun, CBaseHL2MPCombatWeapon );
public:

	CWeaponFlaregun( void );
	
	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );

#ifndef CLIENT_DLL
//	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:

	DECLARE_ACTTABLE();

private:

#ifndef CLIENT_DLL

#endif

	CWeaponFlaregun( const CWeaponFlaregun & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFlaregun, DT_WeaponFlaregun )

BEGIN_NETWORK_TABLE( CWeaponFlaregun, DT_WeaponFlaregun )
#ifdef CLIENT_DLL

#else

#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponFlaregun )

END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_flaregun, CWeaponFlaregun );

PRECACHE_WEAPON_REGISTER( weapon_flaregun );

acttable_t	CWeaponFlaregun::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_PISTOL,					false },
};

IMPLEMENT_ACTTABLE(CWeaponFlaregun);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponFlaregun::CWeaponFlaregun( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlaregun::Precache( void )
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "env_flare" );
#endif
	PrecacheScriptSound( "Flare.Touch" );

	PrecacheScriptSound( "Weapon_FlareGun.Burn" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponFlaregun::PrimaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( m_iClip1 <= 0 )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
		return;
	}

	m_iClip1 = m_iClip1 - 1;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pOwner->m_flNextAttack = gpGlobals->curtime + 1;
#ifndef CLIENT_DLL
	CFlare *pFlare = CFlare::Create( pOwner->Weapon_ShootPosition(), pOwner->EyeAngles(), pOwner, FLARE_DURATION );

	if ( pFlare == NULL )
		return;

	Vector forward;
	pOwner->EyeVectors( &forward );

	pFlare->SetAbsVelocity( forward * 1500 );
#endif
	WeaponSound( SINGLE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponFlaregun::SecondaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( m_iClip1 <= 0 )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
		return;
	}

	m_iClip1 = m_iClip1 - 1;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pOwner->m_flNextAttack = gpGlobals->curtime + 1;
#ifndef CLIENT_DLL
	CFlare *pFlare = CFlare::Create( pOwner->Weapon_ShootPosition(), pOwner->EyeAngles(), pOwner, FLARE_DURATION );

	if ( pFlare == NULL )
		return;

	Vector forward;
	pOwner->EyeVectors( &forward );

	pFlare->SetAbsVelocity( forward * 500 );
	pFlare->SetGravity(1.0f);
	pFlare->SetFriction( 0.85f );
	pFlare->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
#endif
	WeaponSound( SINGLE );
}


