//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_grenadelauncher.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "model_types.h"
	#include "beamdraw.h"
	#include "fx_line.h"
	#include "view.h"
#else
	#include "basecombatcharacter.h"
	#include "movie_explosion.h"
	#include "soundent.h"
	#include "player.h"
	#include "rope.h"
	#include "vstdlib/random.h"
	#include "engine/IEngineSound.h"
	#include "explode.h"
	#include "util.h"
	#include "in_buttons.h"
	#include "shake.h"
	#include "te_effect_dispatch.h"
	#include "triggers.h"
	#include "smoke_trail.h"
	#include "collisionutils.h"
	#include "hl2_shareddefs.h"
#endif

#include "debugoverlay_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	ERROR_SPEED	1500

#ifndef CLIENT_DLL
const char *g_pErrorLaserDotThink = "ErrorLaserThinkContext";

static ConVar sk_apc_missile_damage("sk_apc_missile_damage", "15");
#define APC_MISSILE_DAMAGE	sk_apc_missile_damage.GetFloat()

#endif

#ifdef CLIENT_DLL
#define CErrorLaserDot C_ErrorLaserDot
#endif

//-----------------------------------------------------------------------------
// Laser Dot
//-----------------------------------------------------------------------------
class CErrorLaserDot : public CBaseEntity
{
	DECLARE_CLASS( CErrorLaserDot, CBaseEntity );
public:

	CErrorLaserDot( void );
	~CErrorLaserDot( void );

	static CErrorLaserDot *Create( const Vector &origin, CBaseEntity *pOwner = NULL, bool bVisibleDot = true );

	void	SetTargetEntity( CBaseEntity *pTarget ) { m_hTargetEnt = pTarget; }
	CBaseEntity *GetTargetEntity( void ) { return m_hTargetEnt; }

	void	SetLaserPosition( const Vector &origin, const Vector &normal );
	Vector	GetChasePosition();
	void	TurnOn( void );
	void	TurnOff( void );
	bool	IsOn() const { return m_bIsOn; }

	void	Toggle( void );

	int		ObjectCaps() { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	void	MakeInvisible( void );

#ifdef CLIENT_DLL

	virtual bool			IsTransparent( void ) { return true; }
	virtual RenderGroup_t	GetRenderGroup( void ) { return RENDER_GROUP_TRANSLUCENT_ENTITY; }
	virtual int				DrawModel( int flags );
	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual bool			ShouldDraw( void ) { return (IsEffectActive(EF_NODRAW)==false); }

	CMaterialReference	m_hSpriteMaterial;
#endif

protected:
	Vector				m_vecSurfaceNormal;
	EHANDLE				m_hTargetEnt;
	bool				m_bVisibleLaserDot;
	bool				m_bIsOn;

	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
public:
	CErrorLaserDot			*m_pNext;
};

IMPLEMENT_NETWORKCLASS_ALIASED( ErrorLaserDot, DT_ErrorLaserDot )

BEGIN_NETWORK_TABLE( CErrorLaserDot, DT_ErrorLaserDot )
END_NETWORK_TABLE()

#ifndef CLIENT_DLL

// a list of laser dots to search quickly
CEntityClassList<CErrorLaserDot> g_LaserDotList;
template <> CErrorLaserDot *CEntityClassList<CErrorLaserDot>::m_pClassList = NULL;
CErrorLaserDot *GetLaserDotList()
{
	return g_LaserDotList.m_pClassList;
}

BEGIN_DATADESC( CErrorMissile )

	DEFINE_FIELD( m_hOwner,					FIELD_EHANDLE ),
	DEFINE_FIELD( m_hRocketTrail,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_flAugerTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flMarkDeadTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flGracePeriodEndsAt,	FIELD_TIME ),
	DEFINE_FIELD( m_flDamage,				FIELD_FLOAT ),
	
	// Function Pointers
	DEFINE_FUNCTION( MissileTouch ),
	DEFINE_FUNCTION( AccelerateThink ),
	DEFINE_FUNCTION( AugerThink ),
	DEFINE_FUNCTION( IgniteThink ),
	DEFINE_FUNCTION( SeekThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( error_missile, CErrorMissile );

class CWeaponGrenadeLauncher;

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CErrorMissile::CErrorMissile()
{
	m_hRocketTrail = NULL;
}

CErrorMissile::~CErrorMissile()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CErrorMissile::Precache( void )
{
	PrecacheModel( "models/rocket_error.mdl" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CErrorMissile::Spawn( void )
{
	Precache();

	SetSolid( SOLID_BBOX );
	SetModel("models/rocket_error.mdl");
	UTIL_SetSize( this, -Vector(4,4,4), Vector(4,4,4) );

	SetTouch( &CErrorMissile::MissileTouch );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetThink( &CErrorMissile::IgniteThink );
	
	SetNextThink( gpGlobals->curtime + 0.3f );

	m_takedamage = DAMAGE_YES;
	m_iHealth = m_iMaxHealth = 100;
	m_bloodColor = DONT_BLEED;
	m_flGracePeriodEndsAt = 0;

	AddFlag( FL_OBJECT );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CErrorMissile::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;

	ShotDown();
}

unsigned int CErrorMissile::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CErrorMissile::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( ( info.GetDamageType() & (DMG_MISSILEDEFENSE | DMG_AIRBOAT) ) == false )
		return 0;

	bool bIsDamaged;
	if( m_iHealth <= AugerHealth() )
	{
		// This missile is already damaged (i.e., already running AugerThink)
		bIsDamaged = true;
	}
	else
	{
		// This missile isn't damaged enough to wobble in flight yet
		bIsDamaged = false;
	}
	
	int nRetVal = BaseClass::OnTakeDamage_Alive( info );

	if( !bIsDamaged )
	{
		if ( m_iHealth <= AugerHealth() )
		{
			ShotDown();
		}
	}

	return nRetVal;
}


//-----------------------------------------------------------------------------
// Purpose: Stops any kind of tracking and shoots dumb
//-----------------------------------------------------------------------------
void CErrorMissile::DumbFire( void )
{
	SetThink( NULL );
	SetMoveType( MOVETYPE_FLY );

	SetModel("models/rocket_error.mdl");
	UTIL_SetSize( this, vec3_origin, vec3_origin );

	EmitSound( "Missile.Ignite" );

	// Smoke trail.
	CreateSmokeTrail();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorMissile::SetGracePeriod( float flGracePeriod )
{
	m_flGracePeriodEndsAt = gpGlobals->curtime + flGracePeriod;

	// Go non-solid until the grace period ends
	AddSolidFlags( FSOLID_NOT_SOLID );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CErrorMissile::AccelerateThink( void )
{
	Vector vecForward;

	// !!!UNDONE - make this work exactly the same as HL1 RPG, lest we have looping sound bugs again!
	EmitSound( "Missile.Accelerate" );

	// SetEffects( EF_LIGHT );

	AngleVectors( GetLocalAngles(), &vecForward );
	SetAbsVelocity( vecForward * ERROR_SPEED );

	SetThink( &CErrorMissile::SeekThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

#define AUGER_YDEVIANCE 20.0f
#define AUGER_XDEVIANCEUP 8.0f
#define AUGER_XDEVIANCEDOWN 1.0f

//---------------------------------------------------------
//---------------------------------------------------------
void CErrorMissile::AugerThink( void )
{
	// If we've augered long enough, then just explode
	if ( m_flAugerTime < gpGlobals->curtime )
	{
		Explode();
		return;
	}

	if ( m_flMarkDeadTime < gpGlobals->curtime )
	{
		m_lifeState = LIFE_DYING;
	}

	QAngle angles = GetLocalAngles();

	angles.y += random->RandomFloat( -AUGER_YDEVIANCE, AUGER_YDEVIANCE );
	angles.x += random->RandomFloat( -AUGER_XDEVIANCEDOWN, AUGER_XDEVIANCEUP );

	SetLocalAngles( angles );

	Vector vecForward;

	AngleVectors( GetLocalAngles(), &vecForward );
	
	SetAbsVelocity( vecForward * 1000.0f );

	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: Causes the missile to spiral to the ground and explode, due to damage
//-----------------------------------------------------------------------------
void CErrorMissile::ShotDown( void )
{
	CEffectData	data;
	data.m_vOrigin = GetAbsOrigin();

	DispatchEffect( "RPGShotDown", data );

	if ( m_hRocketTrail != NULL )
	{
		m_hRocketTrail->m_bDamaged = true;
	}

	SetThink( &CErrorMissile::AugerThink );
	SetNextThink( gpGlobals->curtime );
	m_flAugerTime = gpGlobals->curtime + 1.5f;
	m_flMarkDeadTime = gpGlobals->curtime + 0.75;

	// Let the RPG start reloading immediately
	if ( m_hOwner != NULL )
	{
		m_hOwner->NotifyRocketDied();
		m_hOwner = NULL;
	}
}


//-----------------------------------------------------------------------------
// The actual explosion 
//-----------------------------------------------------------------------------
void CErrorMissile::DoExplosion( void )
{
	// Explode
	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), GetOwnerEntity(), GetDamage(), GetDamage() * 2, 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0.0f, this);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorMissile::Explode( void )
{
	// Don't explode against the skybox. Just pretend that 
	// the missile flies off into the distance.
	Vector forward;

	GetVectors( &forward, NULL, NULL );

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + forward * 16, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	m_takedamage = DAMAGE_NO;
	SetSolid( SOLID_NONE );
	if( tr.fraction == 1.0 || !(tr.surface.flags & SURF_SKY) )
	{
		DoExplosion();
	}

	if( m_hRocketTrail )
	{
		m_hRocketTrail->SetLifetime(0.1f);
		m_hRocketTrail = NULL;
	}

	if ( m_hOwner != NULL )
	{
		m_hOwner->NotifyRocketDied();
		m_hOwner = NULL;
	}

	StopSound( "Missile.Ignite" );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CErrorMissile::MissileTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	
	// Don't touch triggers (but DO hit weapons)
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER|FSOLID_VOLUME_CONTENTS) && pOther->GetCollisionGroup() != COLLISION_GROUP_WEAPON )
		return;

	Explode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorMissile::CreateSmokeTrail( void )
{
	if ( m_hRocketTrail )
		return;

	// Smoke trail.
	if ( (m_hRocketTrail = RocketTrail::CreateRocketTrail()) != NULL )
	{
		m_hRocketTrail->m_Opacity = 0.2f;
		m_hRocketTrail->m_SpawnRate = 100;
		m_hRocketTrail->m_ParticleLifetime = 0.5f;
		m_hRocketTrail->m_StartColor.Init( 0.65f, 0.65f , 0.65f );
		m_hRocketTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		m_hRocketTrail->m_StartSize = 8;
		m_hRocketTrail->m_EndSize = 32;
		m_hRocketTrail->m_SpawnRadius = 4;
		m_hRocketTrail->m_MinSpeed = 2;
		m_hRocketTrail->m_MaxSpeed = 16;
		
		m_hRocketTrail->SetLifetime( 999 );
		m_hRocketTrail->FollowEntity( this, "0" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorMissile::IgniteThink( void )
{
	SetMoveType( MOVETYPE_FLY );
	SetModel("models/rocket_error.mdl");
	UTIL_SetSize( this, vec3_origin, vec3_origin );
 	RemoveSolidFlags( FSOLID_NOT_SOLID );

	//TODO: Play opening sound

	Vector vecForward;

	EmitSound( "Missile.Ignite" );

	AngleVectors( GetLocalAngles(), &vecForward );
	SetAbsVelocity( vecForward * ERROR_SPEED );

	SetThink( &CErrorMissile::SeekThink );
	SetNextThink( gpGlobals->curtime );

	if ( m_hOwner && m_hOwner->GetOwner() )
	{
		CBasePlayer *pPlayer = ToBasePlayer( m_hOwner->GetOwner() );

		color32 white = { 255,225,205,64 };
		UTIL_ScreenFade( pPlayer, white, 0.1f, 0.0f, FFADE_IN );
	}

	CreateSmokeTrail();
}


//-----------------------------------------------------------------------------
// Gets the shooting position 
//-----------------------------------------------------------------------------
void CErrorMissile::GetShootPosition( CErrorLaserDot *pErrorLaserDot, Vector *pShootPosition )
{
	if ( pErrorLaserDot->GetOwnerEntity() != NULL )
	{
		//FIXME: Do we care this isn't exactly the muzzle position?
		*pShootPosition = pErrorLaserDot->GetOwnerEntity()->WorldSpaceCenter();
	}
	else
	{
		*pShootPosition = pErrorLaserDot->GetChasePosition();
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define	ERROR_HOMING_SPEED	0.125f

void CErrorMissile::ComputeActualDotPosition( CErrorLaserDot *pErrorLaserDot, Vector *pActualDotPosition, float *pHomingSpeed )
{
	*pHomingSpeed = ERROR_HOMING_SPEED;
	if ( pErrorLaserDot->GetTargetEntity() )
	{
		*pActualDotPosition = pErrorLaserDot->GetChasePosition();
		return;
	}

	Vector vLaserStart;
	GetShootPosition( pErrorLaserDot, &vLaserStart );

	//Get the laser's vector
	Vector vLaserDir;
	VectorSubtract( pErrorLaserDot->GetChasePosition(), vLaserStart, vLaserDir );
	
	//Find the length of the current laser
	float flLaserLength = VectorNormalize( vLaserDir );
	
	//Find the length from the missile to the laser's owner
	float flMissileLength = GetAbsOrigin().DistTo( vLaserStart );

	//Find the length from the missile to the laser's position
	Vector vecTargetToMissile;
	VectorSubtract( GetAbsOrigin(), pErrorLaserDot->GetChasePosition(), vecTargetToMissile ); 
	float flTargetLength = VectorNormalize( vecTargetToMissile );

	// See if we should chase the line segment nearest us
	if ( ( flMissileLength < flLaserLength ) || ( flTargetLength <= 512.0f ) )
	{
		*pActualDotPosition = UTIL_PointOnLineNearestPoint( vLaserStart, pErrorLaserDot->GetChasePosition(), GetAbsOrigin() );
		*pActualDotPosition += ( vLaserDir * 256.0f );
	}
	else
	{
		// Otherwise chase the dot
		*pActualDotPosition = pErrorLaserDot->GetChasePosition();
	}

//	NDebugOverlay::Line( pErrorLaserDot->GetChasePosition(), vLaserStart, 0, 255, 0, true, 0.05f );
//	NDebugOverlay::Line( GetAbsOrigin(), *pActualDotPosition, 255, 0, 0, true, 0.05f );
//	NDebugOverlay::Cross3D( *pActualDotPosition, -Vector(4,4,4), Vector(4,4,4), 255, 0, 0, true, 0.05f );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorMissile::SeekThink( void )
{
	CBaseEntity	*pBestDot	= NULL;
	float		flBestDist	= MAX_TRACE_LENGTH;
	float		dotDist;

	// If we have a grace period, go solid when it ends
	if ( m_flGracePeriodEndsAt )
	{
		if ( m_flGracePeriodEndsAt < gpGlobals->curtime )
		{
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			m_flGracePeriodEndsAt = 0;
		}
	}

	//Search for all dots relevant to us
	for( CErrorLaserDot *pEnt = GetLaserDotList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if ( !pEnt->IsOn() )
			continue;

		if ( pEnt->GetOwnerEntity() != GetOwnerEntity() )
			continue;

		dotDist = (GetAbsOrigin() - pEnt->GetAbsOrigin()).Length();

		//Find closest
		if ( dotDist < flBestDist )
		{
			pBestDot	= pEnt;
			flBestDist	= dotDist;
		}
	}

	//If we have a dot target
	if ( pBestDot == NULL )
	{
		//Think as soon as possible
		SetNextThink( gpGlobals->curtime );
		return;
	}

	CErrorLaserDot *pErrorLaserDot = (CErrorLaserDot *)pBestDot;
	Vector	targetPos;

	float flHomingSpeed; 
	Vector vecLaserDotPosition;
	ComputeActualDotPosition( pErrorLaserDot, &targetPos, &flHomingSpeed );

	if ( IsSimulatingOnAlternateTicks() )
		flHomingSpeed *= 2;

	Vector	vTargetDir;
	VectorSubtract( targetPos, GetAbsOrigin(), vTargetDir );
	float flDist = VectorNormalize( vTargetDir );

	Vector	vDir	= GetAbsVelocity();
	float	flSpeed	= VectorNormalize( vDir );
	Vector	vNewVelocity = vDir;
	if ( gpGlobals->frametime > 0.0f )
	{
		if ( flSpeed != 0 )
		{
			vNewVelocity = ( flHomingSpeed * vTargetDir ) + ( ( 1 - flHomingSpeed ) * vDir );

			// This computation may happen to cancel itself out exactly. If so, slam to targetdir.
			if ( VectorNormalize( vNewVelocity ) < 1e-3 )
			{
				vNewVelocity = (flDist != 0) ? vTargetDir : vDir;
			}
		}
		else
		{
			vNewVelocity = vTargetDir;
		}
	}

	QAngle	finalAngles;
	VectorAngles( vNewVelocity, finalAngles );
	SetAbsAngles( finalAngles );

	vNewVelocity *= flSpeed;
	SetAbsVelocity( vNewVelocity );

	if( GetAbsVelocity() == vec3_origin )
	{
		// Strange circumstances have brought this missile to halt. Just blow it up.
		Explode();
		return;
	}

	// Think as soon as possible
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : &vecOrigin - 
//			&vecAngles - 
//			NULL - 
//
// Output : CErrorMissile
//-----------------------------------------------------------------------------
CErrorMissile *CErrorMissile::Create( const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner = NULL )
{
	//CErrorMissile *pMissile = (CErrorMissile *)CreateEntityByName("rpg_missile" );
	CErrorMissile *pMissile = (CErrorMissile *) CBaseEntity::Create( "error_missile", vecOrigin, vecAngles, CBaseEntity::Instance( pentOwner ) );
	pMissile->SetOwnerEntity( Instance( pentOwner ) );
	pMissile->Spawn();
	pMissile->AddEffects( EF_NOSHADOW );
	
	Vector vecForward;
	AngleVectors( vecAngles, &vecForward );

	pMissile->SetAbsVelocity( vecForward * 300 + Vector( 0,0, 128 ) );

	return pMissile;
}



//-----------------------------------------------------------------------------
// This entity is used to create little force boxes that the helicopter
// should avoid. 
//-----------------------------------------------------------------------------
class CInfoAPCErrorMissileHint : public CBaseEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CInfoAPCErrorMissileHint, CBaseEntity );

	virtual void Spawn( );
	virtual void Activate();
	virtual void UpdateOnRemove();

	static CBaseEntity *FindAimTarget( CBaseEntity *pMissile, const char *pTargetName, 
		const Vector &vecCurrentTargetPos, const Vector &vecCurrentTargetVel );

private:
	EHANDLE	m_hTarget;

	typedef CHandle<CInfoAPCErrorMissileHint> APCErrorMissileHintHandle_t;	
	static CUtlVector< APCErrorMissileHintHandle_t > s_APCErrorMissileHints; 
};


//-----------------------------------------------------------------------------
//
// This entity is used to create little force boxes that the helicopters should avoid. 
//
//-----------------------------------------------------------------------------
CUtlVector< CInfoAPCErrorMissileHint::APCErrorMissileHintHandle_t > CInfoAPCErrorMissileHint::s_APCErrorMissileHints; 

LINK_ENTITY_TO_CLASS( info_apc_errormissile_hint, CInfoAPCErrorMissileHint );

BEGIN_DATADESC( CInfoAPCErrorMissileHint )
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Spawn, remove
//-----------------------------------------------------------------------------
void CInfoAPCErrorMissileHint::Spawn( )
{
	SetModel( STRING( GetModelName() ) );
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
}

void CInfoAPCErrorMissileHint::Activate( )
{
	BaseClass::Activate();

	m_hTarget = gEntList.FindEntityByName( NULL, m_target );
	if ( m_hTarget == NULL )
	{
		DevWarning( "%s: Could not find target '%s'!\n", GetClassname(), STRING( m_target ) );
	}
	else
	{
		s_APCErrorMissileHints.AddToTail( this );
	}
}

void CInfoAPCErrorMissileHint::UpdateOnRemove( )
{
	s_APCErrorMissileHints.FindAndRemove( this );
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Where are how should we avoid?
//-----------------------------------------------------------------------------
#define HINT_PREDICTION_TIME 3.0f

CBaseEntity *CInfoAPCErrorMissileHint::FindAimTarget( CBaseEntity *pMissile, const char *pTargetName, 
	const Vector &vecCurrentEnemyPos, const Vector &vecCurrentEnemyVel )
{
	if ( !pTargetName )
		return NULL;

	float flOOSpeed = pMissile->GetAbsVelocity().Length();
	if ( flOOSpeed != 0.0f )
	{
		flOOSpeed = 1.0f / flOOSpeed;
	}

	for ( int i = s_APCErrorMissileHints.Count(); --i >= 0; )
	{
		CInfoAPCErrorMissileHint *pHint = s_APCErrorMissileHints[i];
		if ( !pHint->NameMatches( pTargetName ) )
			continue;

		if ( !pHint->m_hTarget )
			continue;

		Vector vecMissileToHint, vecMissileToEnemy;
		VectorSubtract( pHint->m_hTarget->WorldSpaceCenter(), pMissile->GetAbsOrigin(), vecMissileToHint );
		VectorSubtract( vecCurrentEnemyPos, pMissile->GetAbsOrigin(), vecMissileToEnemy );
		float flDistMissileToHint = VectorNormalize( vecMissileToHint );
		VectorNormalize( vecMissileToEnemy );
		if ( DotProduct( vecMissileToHint, vecMissileToEnemy ) < 0.866f )
			continue;

		// Determine when the target will be inside the volume.
		// Project at most 3 seconds in advance
		Vector vecRayDelta;
		VectorMultiply( vecCurrentEnemyVel, HINT_PREDICTION_TIME, vecRayDelta );

		BoxTraceInfo_t trace;
		if ( !IntersectRayWithOBB( vecCurrentEnemyPos, vecRayDelta, pHint->CollisionProp()->CollisionToWorldTransform(),
			pHint->CollisionProp()->OBBMins(), pHint->CollisionProp()->OBBMaxs(), 0.0f, &trace ))
		{
			continue;
		}

		// Determine the amount of time it would take the missile to reach the target
		// If we can reach the target within the time it takes for the enemy to reach the 
		float tSqr = flDistMissileToHint * flOOSpeed / HINT_PREDICTION_TIME;
		if ( (tSqr < (trace.t1 * trace.t1)) || (tSqr > (trace.t2 * trace.t2)) )
			continue;

		return pHint->m_hTarget;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// a list of missiles to search quickly
//-----------------------------------------------------------------------------
CEntityClassList<CAPCErrorMissile> g_APCErrorMissileList;
template <> CAPCErrorMissile *CEntityClassList<CAPCErrorMissile>::m_pClassList = NULL;
CAPCErrorMissile *GetAPCErrorMissileList()
{
	return g_APCErrorMissileList.m_pClassList;
}

//-----------------------------------------------------------------------------
// Finds apc missiles in cone
//-----------------------------------------------------------------------------
CAPCErrorMissile *FindAPCErrorMissileInCone( const Vector &vecOrigin, const Vector &vecDirection, float flAngle )
{
	float flCosAngle = cos( DEG2RAD( flAngle ) );
	for( CAPCErrorMissile *pEnt = GetAPCErrorMissileList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if ( !pEnt->IsSolid() )
			continue;

		Vector vecDelta;
		VectorSubtract( pEnt->GetAbsOrigin(), vecOrigin, vecDelta );
		VectorNormalize( vecDelta );
		float flDot = DotProduct( vecDelta, vecDirection );
		if ( flDot > flCosAngle )
			return pEnt;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
//
// Specialized version of the missile
//
//-----------------------------------------------------------------------------
#define MAX_HOMING_DISTANCE 2250.0f
#define MIN_HOMING_DISTANCE 1250.0f
#define MAX_NEAR_HOMING_DISTANCE 1750.0f
#define MIN_NEAR_HOMING_DISTANCE 1000.0f
#define DOWNWARD_BLEND_TIME_START 0.2f
#define MIN_HEIGHT_DIFFERENCE	250.0f
#define MAX_HEIGHT_DIFFERENCE	550.0f
#define CORRECTION_TIME		0.2f
#define	APC_LAUNCH_HOMING_SPEED	0.1f
#define	APC_HOMING_SPEED	0.025f
#define HOMING_SPEED_ACCEL	0.01f

BEGIN_DATADESC( CAPCErrorMissile )

	DEFINE_FIELD( m_flReachedTargetTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flIgnitionTime,			FIELD_TIME ),
	DEFINE_FIELD( m_bGuidingDisabled,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hSpecificTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_strHint,				FIELD_STRING ),
	DEFINE_FIELD( m_flLastHomingSpeed,		FIELD_FLOAT ),
//	DEFINE_FIELD( m_pNext,					FIELD_CLASSPTR ),

	DEFINE_THINKFUNC( BeginSeekThink ),
	DEFINE_THINKFUNC( AugerStartThink ),
	DEFINE_THINKFUNC( ExplodeThink ),

	DEFINE_FUNCTION( APCErrorMissileTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( apc_errormissile, CAPCErrorMissile );

CAPCErrorMissile *CAPCErrorMissile::Create( const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseEntity *pOwner )
{
	CAPCErrorMissile *pMissile = (CAPCErrorMissile *)CBaseEntity::Create( "apc_errormissile", vecOrigin, vecAngles, pOwner );
	pMissile->SetOwnerEntity( pOwner );
	pMissile->Spawn();
	pMissile->SetAbsVelocity( vecVelocity );
	pMissile->AddFlag( FL_NOTARGET );
	pMissile->AddEffects( EF_NOSHADOW );
	return pMissile;
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CAPCErrorMissile::CAPCErrorMissile()
{
	g_APCErrorMissileList.Insert( this );
}

CAPCErrorMissile::~CAPCErrorMissile()
{
	g_APCErrorMissileList.Remove( this );
}


//-----------------------------------------------------------------------------
// Shared initialization code
//-----------------------------------------------------------------------------
void CAPCErrorMissile::Init()
{
	SetMoveType( MOVETYPE_FLY );
	SetModel("models/rocket_error.mdl");
	UTIL_SetSize( this, vec3_origin, vec3_origin );
	CreateSmokeTrail();
	SetTouch( &CAPCErrorMissile::APCErrorMissileTouch );
	m_flLastHomingSpeed = APC_HOMING_SPEED;
}


//-----------------------------------------------------------------------------
// For hitting a specific target
//-----------------------------------------------------------------------------
void CAPCErrorMissile::AimAtSpecificTarget( CBaseEntity *pTarget )
{
	m_hSpecificTarget = pTarget;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CAPCErrorMissile::APCErrorMissileTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() && !pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	Explode();
}


//-----------------------------------------------------------------------------
// Specialized version of the missile
//-----------------------------------------------------------------------------
void CAPCErrorMissile::IgniteDelay( void )
{
	m_flIgnitionTime = gpGlobals->curtime + 0.3f;

	SetThink( &CAPCErrorMissile::BeginSeekThink );
	SetNextThink( m_flIgnitionTime );
	Init();
	AddSolidFlags( FSOLID_NOT_SOLID );
}

void CAPCErrorMissile::AugerDelay( float flDelay )
{
	m_flIgnitionTime = gpGlobals->curtime;
	SetThink( &CAPCErrorMissile::AugerStartThink );
	SetNextThink( gpGlobals->curtime + flDelay );
	Init();
	DisableGuiding();
}

void CAPCErrorMissile::AugerStartThink()
{
	if ( m_hRocketTrail != NULL )
	{
		m_hRocketTrail->m_bDamaged = true;
	}
	m_flAugerTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 2.0f );
	SetThink( &CAPCErrorMissile::AugerThink );
	SetNextThink( gpGlobals->curtime );
}

void CAPCErrorMissile::ExplodeDelay( float flDelay )
{
	m_flIgnitionTime = gpGlobals->curtime;
	SetThink( &CAPCErrorMissile::ExplodeThink );
	SetNextThink( gpGlobals->curtime + flDelay );
	Init();
	DisableGuiding();
}


void CAPCErrorMissile::BeginSeekThink( void )
{
 	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetThink( &CAPCErrorMissile::SeekThink );
	SetNextThink( gpGlobals->curtime );
}

void CAPCErrorMissile::ExplodeThink()
{
	DoExplosion();
}

//-----------------------------------------------------------------------------
// Health lost at which augering starts
//-----------------------------------------------------------------------------
int CAPCErrorMissile::AugerHealth()
{
	return m_iMaxHealth - 25;
}

	
//-----------------------------------------------------------------------------
// Health lost at which augering starts
//-----------------------------------------------------------------------------
void CAPCErrorMissile::DisableGuiding()
{
	m_bGuidingDisabled = true;
}

	
//-----------------------------------------------------------------------------
// Guidance hints
//-----------------------------------------------------------------------------
void CAPCErrorMissile::SetGuidanceHint( const char *pHintName )
{
	m_strHint = MAKE_STRING( pHintName );
}


//-----------------------------------------------------------------------------
// The actual explosion 
//-----------------------------------------------------------------------------
void CAPCErrorMissile::DoExplosion( void )
{
	if ( GetWaterLevel() != 0 )
	{
		CEffectData data;
		data.m_vOrigin = WorldSpaceCenter();
		data.m_flMagnitude = 128;
		data.m_flScale = 128;
		data.m_fFlags = 0;
		DispatchEffect( "WaterSurfaceExplosion", data );
	}
	else
	{
		ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), GetOwnerEntity(), 
			APC_MISSILE_DAMAGE, 100, true, 20000 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCErrorMissile::ComputeLeadingPosition( const Vector &vecShootPosition, CBaseEntity *pTarget, Vector *pLeadPosition )
{
	Vector vecTarget = pTarget->BodyTarget( vecShootPosition, false );
	float flShotSpeed = GetAbsVelocity().Length();
	if ( flShotSpeed == 0 )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	Vector vecVelocity = pTarget->GetSmoothedVelocity();
	vecVelocity.z = 0.0f;
	float flTargetSpeed = VectorNormalize( vecVelocity );
	Vector vecDelta;
	VectorSubtract( vecShootPosition, vecTarget, vecDelta );
	float flTargetToShooter = VectorNormalize( vecDelta );
	float flCosTheta = DotProduct( vecDelta, vecVelocity );

	// Law of cosines... z^2 = x^2 + y^2 - 2xy cos Theta
	// where z = flShooterToPredictedTargetPosition = flShotSpeed * predicted time
	// x = flTargetSpeed * predicted time
	// y = flTargetToShooter
	// solve for predicted time using at^2 + bt + c = 0, t = (-b +/- sqrt( b^2 - 4ac )) / 2a
	float a = flTargetSpeed * flTargetSpeed - flShotSpeed * flShotSpeed;
	float b = -2.0f * flTargetToShooter * flCosTheta * flTargetSpeed;
	float c = flTargetToShooter * flTargetToShooter;
	
	float flDiscrim = b*b - 4*a*c;
	if (flDiscrim < 0)
	{
		*pLeadPosition = vecTarget;
		return;
	}

	flDiscrim = sqrt(flDiscrim);
	float t = (-b + flDiscrim) / (2.0f * a);
	float t2 = (-b - flDiscrim) / (2.0f * a);
	if ( t < t2 )
	{
		t = t2;
	}

	if ( t <= 0.0f )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	VectorMA( vecTarget, flTargetSpeed * t, vecVelocity, *pLeadPosition );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCErrorMissile::ComputeActualDotPosition( CErrorLaserDot *pErrorLaserDot, Vector *pActualDotPosition, float *pHomingSpeed )
{
	if ( m_bGuidingDisabled )
	{
		*pActualDotPosition = GetAbsOrigin();
		*pHomingSpeed = 0.0f;
		m_flLastHomingSpeed = *pHomingSpeed;
		return;
	}

	if ( ( m_strHint != NULL_STRING ) && (!m_hSpecificTarget) )
	{
		Vector vecOrigin, vecVelocity;
		CBaseEntity *pTarget = pErrorLaserDot->GetTargetEntity();
		if ( pTarget )
		{
			vecOrigin = pTarget->BodyTarget( GetAbsOrigin(), false );
			vecVelocity	= pTarget->GetSmoothedVelocity();
		}
		else
		{
			vecOrigin = pErrorLaserDot->GetChasePosition();
			vecVelocity = vec3_origin;
		}

		m_hSpecificTarget = CInfoAPCErrorMissileHint::FindAimTarget( this, STRING( m_strHint ), vecOrigin, vecVelocity );
	}

	CBaseEntity *pLaserTarget = m_hSpecificTarget ? m_hSpecificTarget.Get() : pErrorLaserDot->GetTargetEntity();
	if ( !pLaserTarget )
	{
		BaseClass::ComputeActualDotPosition( pErrorLaserDot, pActualDotPosition, pHomingSpeed );
		m_flLastHomingSpeed = *pHomingSpeed;
		return;
	}
	
	if ( pLaserTarget->ClassMatches( "npc_bullseye" ) )
	{
		if ( m_flLastHomingSpeed != ERROR_HOMING_SPEED )
		{
			if (m_flLastHomingSpeed > ERROR_HOMING_SPEED)
			{
				m_flLastHomingSpeed -= HOMING_SPEED_ACCEL * UTIL_GetSimulationInterval();
				if ( m_flLastHomingSpeed < ERROR_HOMING_SPEED )
				{
					m_flLastHomingSpeed = ERROR_HOMING_SPEED;
				}
			}
			else
			{
				m_flLastHomingSpeed += HOMING_SPEED_ACCEL * UTIL_GetSimulationInterval();
				if ( m_flLastHomingSpeed > ERROR_HOMING_SPEED )
				{
					m_flLastHomingSpeed = ERROR_HOMING_SPEED;
				}
			}
		}
		*pHomingSpeed = m_flLastHomingSpeed;
		*pActualDotPosition = pLaserTarget->WorldSpaceCenter();
		return;
	}

	Vector vLaserStart;
	GetShootPosition( pErrorLaserDot, &vLaserStart );
	*pHomingSpeed = APC_LAUNCH_HOMING_SPEED;

	//Get the laser's vector
	Vector vecTargetPosition = pLaserTarget->BodyTarget( GetAbsOrigin(), false );

	// Compute leading position
	Vector vecLeadPosition;
	ComputeLeadingPosition( GetAbsOrigin(), pLaserTarget, &vecLeadPosition );

	Vector vecTargetToMissile, vecTargetToShooter;
	VectorSubtract( GetAbsOrigin(), vecTargetPosition, vecTargetToMissile ); 
	VectorSubtract( vLaserStart, vecTargetPosition, vecTargetToShooter );

	*pActualDotPosition = vecLeadPosition;

	float flMinHomingDistance = MIN_HOMING_DISTANCE;
	float flMaxHomingDistance = MAX_HOMING_DISTANCE;
	float flBlendTime = gpGlobals->curtime - m_flIgnitionTime;
	if ( flBlendTime > DOWNWARD_BLEND_TIME_START )
	{
		if ( m_flReachedTargetTime != 0.0f )
		{
			*pHomingSpeed = APC_HOMING_SPEED;
			float flDeltaTime = clamp( gpGlobals->curtime - m_flReachedTargetTime, 0.0f, CORRECTION_TIME );
			*pHomingSpeed = SimpleSplineRemapVal( flDeltaTime, 0.0f, CORRECTION_TIME, 0.2f, *pHomingSpeed );
			flMinHomingDistance = SimpleSplineRemapVal( flDeltaTime, 0.0f, CORRECTION_TIME, MIN_NEAR_HOMING_DISTANCE, flMinHomingDistance );
			flMaxHomingDistance = SimpleSplineRemapVal( flDeltaTime, 0.0f, CORRECTION_TIME, MAX_NEAR_HOMING_DISTANCE, flMaxHomingDistance );
		}
		else
		{
			flMinHomingDistance = MIN_NEAR_HOMING_DISTANCE;
			flMaxHomingDistance = MAX_NEAR_HOMING_DISTANCE;
			Vector vecDelta;
			VectorSubtract( GetAbsOrigin(), *pActualDotPosition, vecDelta );
			if ( vecDelta.z > MIN_HEIGHT_DIFFERENCE )
			{
				float flClampedHeight = clamp( vecDelta.z, MIN_HEIGHT_DIFFERENCE, MAX_HEIGHT_DIFFERENCE );
				float flHeightAdjustFactor = SimpleSplineRemapVal( flClampedHeight, MIN_HEIGHT_DIFFERENCE, MAX_HEIGHT_DIFFERENCE, 0.0f, 1.0f );

				vecDelta.z = 0.0f;
				float flDist = VectorNormalize( vecDelta );

				float flForwardOffset = 2000.0f;
				if ( flDist > flForwardOffset )
				{
					Vector vecNewPosition;
					VectorMA( GetAbsOrigin(), -flForwardOffset, vecDelta, vecNewPosition );
					vecNewPosition.z = pActualDotPosition->z;

					VectorLerp( *pActualDotPosition, vecNewPosition, flHeightAdjustFactor, *pActualDotPosition );
				}
			}
			else
			{
				m_flReachedTargetTime = gpGlobals->curtime;
			}
		}

		// Allows for players right at the edge of rocket range to be threatened
		if ( flBlendTime > 0.6f )
		{
			float flTargetLength = GetAbsOrigin().DistTo( pLaserTarget->WorldSpaceCenter() );
			flTargetLength = clamp( flTargetLength, flMinHomingDistance, flMaxHomingDistance ); 
			*pHomingSpeed = SimpleSplineRemapVal( flTargetLength, flMaxHomingDistance, flMinHomingDistance, *pHomingSpeed, 0.01f );
		}
	}

	float flDot = DotProduct2D( vecTargetToShooter.AsVector2D(), vecTargetToMissile.AsVector2D() );
	if ( ( flDot < 0 ) || m_bGuidingDisabled )
	{
		*pHomingSpeed = 0.0f;
	}

	m_flLastHomingSpeed = *pHomingSpeed;

//	NDebugOverlay::Line( vecLeadPosition, GetAbsOrigin(), 0, 255, 0, true, 0.05f );
//	NDebugOverlay::Line( GetAbsOrigin(), *pActualDotPosition, 255, 0, 0, true, 0.05f );
//	NDebugOverlay::Cross3D( *pActualDotPosition, -Vector(4,4,4), Vector(4,4,4), 255, 0, 0, true, 0.05f );
}

#endif

#define	ERROR_BEAM_SPRITE		"effects/laser1.vmt"
#define	ERROR_BEAM_SPRITE_NOZ	"effects/laser1_noz.vmt"
#define	ERROR_LASER_SPRITE	"sprites/redglow1"

//=============================================================================
// ERROR
//=============================================================================

LINK_ENTITY_TO_CLASS( weapon_grenadelauncher, CWeaponGrenadeLauncher );
PRECACHE_WEAPON_REGISTER(weapon_grenadelauncher);

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGrenadeLauncher, DT_WeaponGrenadeLauncher )

#ifdef CLIENT_DLL
void RecvProxy_ErrorMissleDied( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CWeaponGrenadeLauncher *pERROR = ((CWeaponGrenadeLauncher*)pStruct);

	RecvProxy_IntToEHandle( pData, pStruct, pOut );

	CBaseEntity *pNewMissile = pERROR->GetMissile();

	if ( pNewMissile == NULL )
	{
		if ( pERROR->GetOwner() && pERROR->GetOwner()->GetActiveWeapon() == pERROR )
		{
			pERROR->NotifyRocketDied();
		}
	}
}

#endif

BEGIN_NETWORK_TABLE( CWeaponGrenadeLauncher, DT_WeaponGrenadeLauncher )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bInitialStateUpdate ) ),
	RecvPropBool( RECVINFO( m_bGuiding ) ),
	RecvPropBool( RECVINFO( m_bHideGuiding ) ),
	RecvPropEHandle( RECVINFO( m_hMissile ), RecvProxy_ErrorMissleDied ),
	RecvPropVector( RECVINFO( m_vecLaserDot ) ),
#else
	SendPropBool( SENDINFO( m_bInitialStateUpdate ) ),
	SendPropBool( SENDINFO( m_bGuiding ) ),
	SendPropBool( SENDINFO( m_bHideGuiding ) ),
	SendPropEHandle( SENDINFO( m_hMissile ) ),
	SendPropVector( SENDINFO( m_vecLaserDot ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL

BEGIN_PREDICTION_DATA( CWeaponGrenadeLauncher )
	DEFINE_PRED_FIELD( m_bInitialStateUpdate, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bGuiding, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bHideGuiding, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

#endif

#ifndef CLIENT_DLL
acttable_t	CWeaponGrenadeLauncher::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_RPG,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_RPG,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_RPG,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_RPG,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_RPG,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_RPG,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_RPG,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_RPG,				false },
};

IMPLEMENT_ACTTABLE(CWeaponGrenadeLauncher);

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponGrenadeLauncher::CWeaponGrenadeLauncher()
{
	m_bReloadsSingly = true;
	m_bInitialStateUpdate= false;
	m_bHideGuiding = false;
	m_bGuiding = false;

	m_fMinRange1 = m_fMinRange2 = 40*12;
	m_fMaxRange1 = m_fMaxRange2 = 500*12;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponGrenadeLauncher::~CWeaponGrenadeLauncher()
{
#ifndef CLIENT_DLL
	if ( m_hLaserDot != NULL )
	{
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Missile.Ignite" );
	PrecacheScriptSound( "Missile.Accelerate" );

	// Laser dot...
	PrecacheModel( "sprites/redglow1.vmt" );
	PrecacheModel( ERROR_LASER_SPRITE );
	PrecacheModel( ERROR_BEAM_SPRITE );
	PrecacheModel( ERROR_BEAM_SPRITE_NOZ );

#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "error_missile" );
#endif

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::Activate( void )
{
	BaseClass::Activate();

	// Restore the laser pointer after transition
	if ( m_bGuiding )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		
		if ( pOwner == NULL )
			return;

		//if ( pOwner->GetActiveWeapon() == this )
		//{
			//StartGuiding();
		//}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::HasAnyAmmo( void )
{
	if ( m_hMissile != NULL )
		return true;

	return BaseClass::HasAnyAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::WeaponShouldBeLowered( void )
{
	// Lower us if we're out of ammo
	if ( !HasAnyAmmo() )
		return true;
	
	return BaseClass::WeaponShouldBeLowered();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	// Can't have an active missile out
	if ( m_hMissile != NULL )
		return;

	// Can't be reloading
	if ( GetActivity() == ACT_VM_RELOAD )
		return;

	Vector vecOrigin;
	Vector vecForward;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	Vector	vForward, vRight, vUp;

	pOwner->EyeVectors( &vForward, &vRight, &vUp );

	Vector	muzzlePoint = pOwner->Weapon_ShootPosition() + vForward * 12.0f + vRight * 6.0f + vUp * -3.0f;

#ifndef CLIENT_DLL
	QAngle vecAngles;
	VectorAngles( vForward, vecAngles );

	CErrorMissile *pMissile = CErrorMissile::Create( muzzlePoint, vecAngles, GetOwner()->edict() );
	pMissile->m_hOwner = this;

	// If the shot is clear to the player, give the missile a grace period
	trace_t	tr;
	Vector vecEye = pOwner->EyePosition();
	UTIL_TraceLine( vecEye, vecEye + vForward * 128, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0 )
	{
		pMissile->SetGracePeriod( 0.3 );
	}

	pMissile->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );

	m_hMissile = pMissile;
#endif

	DecrementAmmo( GetOwner() );
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	WeaponSound( SINGLE );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	// Take away our primary ammo type
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::SuppressGuiding( bool state )
{
	m_bHideGuiding = state;

#ifndef CLIENT_DLL

	if ( m_hLaserDot == NULL )
	{
		//StartGuiding();

		//STILL!?
		if ( m_hLaserDot == NULL )
			 return;
	}

	if ( state )
	{
		m_hLaserDot->TurnOff();
	}
	else
	{
		m_hLaserDot->TurnOn();
	}
#endif
	
}

//-----------------------------------------------------------------------------
// Purpose: Override this if we're guiding a missile currently
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::Lower( void )
{
	if ( m_hMissile != NULL && IsGuiding() )
		return false;

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	//If we're pulling the weapon out for the first time, wait to draw the laser
	if ( ( m_bInitialStateUpdate ) && ( GetActivity() != ACT_VM_DRAW ) )
	{
		//StartGuiding();
		m_bInitialStateUpdate = false;
	}

	// Supress our guiding effects if we're lowered
	if ( GetIdealActivity() == ACT_VM_IDLE_LOWERED )
	{
		SuppressGuiding();
	}
	else
	{
		SuppressGuiding( false );
	}

	//Move the laser
	UpdateLaserPosition();

	if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 && m_hMissile == NULL )
	{
		StopGuiding();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CWeaponGrenadeLauncher::GetLaserPosition( void )
{
#ifndef CLIENT_DLL
	CreateLaserPointer();

	if ( m_hLaserDot != NULL )
		return m_hLaserDot->GetAbsOrigin();

	//FIXME: The laser dot sprite is not active, this code should not be allowed!
	assert(0);
#endif
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: NPC RPG users cheat and directly set the laser pointer's origin
// Input  : &vecTarget - 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::UpdateNPCLaserPosition( const Vector &vecTarget )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::SetNPCLaserPosition( const Vector &vecTarget ) 
{ 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector &CWeaponGrenadeLauncher::GetNPCLaserPosition( void )
{
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true if the rocket is being guided, false if it's dumb
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::IsGuiding( void )
{
	return m_bGuiding;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::Deploy( void )
{
	m_bInitialStateUpdate = true;

	return BaseClass::Deploy();
}

bool CWeaponGrenadeLauncher::CanHolster( void )
{
	//Can't have an active missile out
	if ( m_hMissile != NULL )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopGuiding();

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the guiding laser
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::StartGuiding( void )
{
	// Don't start back up if we're overriding this
	if ( m_bHideGuiding )
		return;

	m_bGuiding = true;

#ifndef CLIENT_DLL
	WeaponSound(SPECIAL1);

	CreateLaserPointer();
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Turn off the guiding laser
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::StopGuiding( void )
{
	m_bGuiding = false;

#ifndef CLIENT_DLL

	WeaponSound( SPECIAL2 );

	// Kill the dot completely
	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOff();
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot = NULL;
	}
#else
	if ( m_pBeam )
	{
		//Tell it to die right away and let the beam code free it.
		m_pBeam->brightness = 0.0f;
		m_pBeam->flags &= ~FBEAM_FOREVER;
		m_pBeam->die = gpGlobals->curtime - 0.1;
		m_pBeam = NULL;
	}
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Toggle the guiding laser
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::ToggleGuiding( void )
{
	if ( IsGuiding() )
	{
		StopGuiding();
	}
	else
	{
		StartGuiding();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::Drop( const Vector &vecVelocity )
{
	StopGuiding();

	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::UpdateLaserPosition( Vector vecMuzzlePos, Vector vecEndPos )
{

#ifndef CLIENT_DLL
	if ( vecMuzzlePos == vec3_origin || vecEndPos == vec3_origin )
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
			return;

		vecMuzzlePos = pPlayer->Weapon_ShootPosition();
		Vector	forward;
		pPlayer->EyeVectors( &forward );
		vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );
	}

	//Move the laser dot, if active
	trace_t	tr;
	
	// Trace out for the endpoint
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, (MASK_SHOT & ~CONTENTS_WINDOW), GetOwner(), COLLISION_GROUP_NONE, &tr );

	// Move the laser sprite
	if ( m_hLaserDot != NULL )
	{
		Vector	laserPos = tr.endpos;
		m_hLaserDot->SetLaserPosition( laserPos, tr.plane.normal );
				
		if ( tr.DidHitNonWorldEntity() )
		{
			CBaseEntity *pHit = tr.m_pEnt;

			if ( ( pHit != NULL ) && ( pHit->m_takedamage ) )
			{
				m_hLaserDot->SetTargetEntity( pHit );
			}
			else
			{
				m_hLaserDot->SetTargetEntity( NULL );
			}
		}
		else
		{
			m_hLaserDot->SetTargetEntity( NULL );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::CreateLaserPointer( void )
{
#ifndef CLIENT_DLL
	if ( m_hLaserDot != NULL )
		return;

	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( pOwner == NULL )
		return;

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return;

	m_hLaserDot = CErrorLaserDot::Create( GetAbsOrigin(), GetOwner() );
	m_hLaserDot->TurnOff();

	UpdateLaserPosition();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::NotifyRocketDied( void )
{
	m_hMissile = NULL;

	if ( GetActivity() == ACT_VM_RELOAD )
		return;

	Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::Reload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	WeaponSound( RELOAD );
	
	SendWeaponAnim( ACT_VM_RELOAD );

	return true;
}

#ifdef CLIENT_DLL

#define	ERROR_MUZZLE_ATTACHMENT		1
#define	ERROR_GUIDE_ATTACHMENT		2
#define	ERROR_GUIDE_TARGET_ATTACHMENT	3

#define	ERROR_GUIDE_ATTACHMENT_3RD		4
#define	ERROR_GUIDE_TARGET_ATTACHMENT_3RD	5

#define	ERROR_LASER_BEAM_LENGTH	128

extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

//-----------------------------------------------------------------------------
// Purpose: Returns the attachment point on either the world or viewmodel
//			This should really be worked into the CBaseCombatWeapon class!
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::GetWeaponAttachment( int attachmentId, Vector &outVector, Vector *dir /*= NULL*/ )
{
	QAngle	angles;

	if ( ShouldDrawUsingViewModel() )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		
		if ( pOwner != NULL )
		{
			pOwner->GetViewModel()->GetAttachment( attachmentId, outVector, angles );
			::FormatViewModelAttachment( outVector, true );
		}
	}
	else
	{
		// We offset the IDs to make them correct for our world model
		BaseClass::GetAttachment( attachmentId, outVector, angles );
	}

	// Supply the direction, if requested
	if ( dir != NULL )
	{
		AngleVectors( angles, dir, NULL, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup our laser beam
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::InitBeam( void )
{
	if ( m_pBeam != NULL )
		return;

	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( pOwner == NULL )
		return;

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return;


	BeamInfo_t beamInfo;

	CBaseEntity *pEntity = NULL;

	if ( ShouldDrawUsingViewModel() )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		
		if ( pOwner != NULL )
		{
			pEntity = pOwner->GetViewModel();
		}
	}
	else
	{
		pEntity = this;
	}

	beamInfo.m_pStartEnt = pEntity;
	beamInfo.m_pEndEnt = pEntity;
	beamInfo.m_nType = TE_BEAMPOINTS;
	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_vecEnd = vec3_origin;
	
	beamInfo.m_pszModelName = ( ShouldDrawUsingViewModel() ) ? ERROR_BEAM_SPRITE_NOZ : ERROR_BEAM_SPRITE;
	
	beamInfo.m_flHaloScale = 0.0f;
	beamInfo.m_flLife = 0.0f;
	
	if ( ShouldDrawUsingViewModel() )
	{
		beamInfo.m_flWidth = 2.0f;
		beamInfo.m_flEndWidth = 2.0f;
		beamInfo.m_nStartAttachment = ERROR_GUIDE_ATTACHMENT;
		beamInfo.m_nEndAttachment = ERROR_GUIDE_TARGET_ATTACHMENT;
	}
	else
	{
		beamInfo.m_flWidth = 1.0f;
		beamInfo.m_flEndWidth = 1.0f;
		beamInfo.m_nStartAttachment = ERROR_GUIDE_ATTACHMENT_3RD;
		beamInfo.m_nEndAttachment = ERROR_GUIDE_TARGET_ATTACHMENT_3RD;
	}

	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = 0;
	beamInfo.m_flBrightness = 255.0;
	beamInfo.m_flSpeed = 1.0f;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 30.0;
	beamInfo.m_flRed = 255.0;
	beamInfo.m_flGreen = 0.0;
	beamInfo.m_flBlue = 0.0;
	beamInfo.m_nSegments = 4;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = (FBEAM_FOREVER|FBEAM_SHADEOUT);

	m_pBeam = beams->CreateBeamEntPoint( beamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Draw effects for our weapon
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::DrawEffects( void )
{
	// Must be guiding and not hidden
	if ( !m_bGuiding || m_bHideGuiding )
	{
		if ( m_pBeam != NULL )
		{
			m_pBeam->brightness = 0;
		}

		return;
	}

	// Setup our sprite
	if ( m_hSpriteMaterial == NULL )
	{
		m_hSpriteMaterial.Init( ERROR_LASER_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
	}

	// Setup our beam
	if ( m_hBeamMaterial == NULL )
	{
		m_hBeamMaterial.Init( ERROR_BEAM_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
	}

	color32 color={255,255,255,255};
	Vector	vecAttachment, vecDir;
	QAngle	angles;

	float scale = 8.0f + random->RandomFloat( -2.0f, 2.0f );

	int	attachmentID = ( ShouldDrawUsingViewModel() ) ? ERROR_GUIDE_ATTACHMENT : ERROR_GUIDE_ATTACHMENT_3RD;

	GetWeaponAttachment( attachmentID, vecAttachment, &vecDir );

	// Draw the sprite
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );
	DrawSprite( vecAttachment, scale, scale, color );
	
	// Get the beam's run
	trace_t tr;
	UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * ERROR_LASER_BEAM_LENGTH ), MASK_SHOT, GetOwner(), COLLISION_GROUP_NONE, &tr );
	
	InitBeam();

	if ( m_pBeam != NULL )
	{
		m_pBeam->fadeLength = ERROR_LASER_BEAM_LENGTH * tr.fraction;
		m_pBeam->brightness = random->RandomInt( 128, 200 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on third-person weapon drawing
//-----------------------------------------------------------------------------
int	CWeaponGrenadeLauncher::DrawModel( int flags )
{
	// Only render these on the transparent pass
	if ( flags & STUDIO_TRANSPARENCY )
	{
		DrawEffects();
		return 1;
	}

	// Draw the model as normal
	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: Called after first-person viewmodel is drawn
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::ViewModelDrawn( C_BaseViewModel *pBaseViewModel )
{
	// Draw our laser effects
	DrawEffects();
	
	BaseClass::ViewModelDrawn( pBaseViewModel );
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine sorting of model when drawn
//-----------------------------------------------------------------------------
bool CWeaponGrenadeLauncher::IsTranslucent( void )
{
	// Must be guiding and not hidden
	if ( m_bGuiding && !m_bHideGuiding )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Turns off effects when leaving the PVS
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit(state);

	if ( state == SHOULDTRANSMIT_END )
	{
		if ( m_pBeam != NULL )
		{
			m_pBeam->brightness = 0.0f;
		}
	}
}

#endif	//CLIENT_DLL


//=============================================================================
// Laser Dot
//=============================================================================

LINK_ENTITY_TO_CLASS( env_errorlaserdot, CErrorLaserDot );

BEGIN_DATADESC( CErrorLaserDot )
	DEFINE_FIELD( m_vecSurfaceNormal,	FIELD_VECTOR ),
	DEFINE_FIELD( m_hTargetEnt,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_bVisibleLaserDot,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsOn,				FIELD_BOOLEAN ),

	//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),	// don't save - regenerated by constructor
END_DATADESC()


//-----------------------------------------------------------------------------
// Finds missiles in cone
//-----------------------------------------------------------------------------
CBaseEntity *CreateErrorLaserDot( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
	return CErrorLaserDot::Create( origin, pOwner, bVisibleDot );
}

void SetErrorLaserDotTarget( CBaseEntity *pLaserDot, CBaseEntity *pTarget )
{
	CErrorLaserDot *pDot = assert_cast< CErrorLaserDot* >(pLaserDot );
	pDot->SetTargetEntity( pTarget );
}

void EnableErrorLaserDot( CBaseEntity *pLaserDot, bool bEnable )
{
	CErrorLaserDot *pDot = assert_cast< CErrorLaserDot* >(pLaserDot );
	if ( bEnable )
	{
		pDot->TurnOn();
	}
	else
	{
		pDot->TurnOff();
	}
}

CErrorLaserDot::CErrorLaserDot( void )
{
	m_hTargetEnt = NULL;
	m_bIsOn = true;
#ifndef CLIENT_DLL
	g_LaserDotList.Insert( this );
#endif
}

CErrorLaserDot::~CErrorLaserDot( void )
{
#ifndef CLIENT_DLL
	g_LaserDotList.Remove( this );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CErrorLaserDot
//-----------------------------------------------------------------------------
CErrorLaserDot *CErrorLaserDot::Create( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
#ifndef CLIENT_DLL
	CErrorLaserDot *pErrorLaserDot = (CErrorLaserDot *) CBaseEntity::Create( "env_errorlaserdot", origin, QAngle(0,0,0) );

	if ( pErrorLaserDot == NULL )
		return NULL;

	pErrorLaserDot->m_bVisibleLaserDot = bVisibleDot;
	pErrorLaserDot->SetMoveType( MOVETYPE_NONE );
	pErrorLaserDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pErrorLaserDot->AddEffects( EF_NOSHADOW );
	UTIL_SetSize( pErrorLaserDot, -Vector(4,4,4), Vector(4,4,4) );

	pErrorLaserDot->SetOwnerEntity( pOwner );

	pErrorLaserDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	if ( !bVisibleDot )
	{
		pErrorLaserDot->MakeInvisible();
	}

	return pErrorLaserDot;
#else
	return NULL;
#endif
}

void CErrorLaserDot::SetLaserPosition( const Vector &origin, const Vector &normal )
{
	SetAbsOrigin( origin );
	m_vecSurfaceNormal = normal;
}

Vector CErrorLaserDot::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorLaserDot::TurnOn( void )
{
	m_bIsOn = true;
	if ( m_bVisibleLaserDot )
	{
		//BaseClass::TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorLaserDot::TurnOff( void )
{
	m_bIsOn = false;
	if ( m_bVisibleLaserDot )
	{
		//BaseClass::TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CErrorLaserDot::MakeInvisible( void )
{
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Draw our sprite
//-----------------------------------------------------------------------------
int CErrorLaserDot::DrawModel( int flags )
{
	color32 color={255,255,255,255};
	Vector	vecAttachment, vecDir;
	QAngle	angles;

	float	scale;
	Vector	endPos;

	C_HL2MP_Player *pOwner = ToHL2MPPlayer( GetOwnerEntity() );

	if ( pOwner != NULL && pOwner->IsDormant() == false )
	{
		// Always draw the dot in front of our faces when in first-person
		if ( pOwner->IsLocalPlayer() )
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();
		}
		else
		{
			// Take the eye position and direction
			vecAttachment = pOwner->EyePosition();
			
			QAngle angles = pOwner->GetAnimEyeAngles();
			AngleVectors( angles, &vecDir );
		}
		
		trace_t tr;
		UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * MAX_TRACE_LENGTH ), MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
		
		// Backup off the hit plane
		endPos = tr.endpos + ( tr.plane.normal * 4.0f );
	}
	else
	{
		// Just use our position if we can't predict it otherwise
		endPos = GetAbsOrigin();
	}

	// Randomly flutter
	scale = 16.0f + random->RandomFloat( -4.0f, 4.0f );

	// Draw our laser dot in space
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );
	DrawSprite( endPos, scale, scale, color );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Setup our sprite reference
//-----------------------------------------------------------------------------
void CErrorLaserDot::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_hSpriteMaterial.Init( ERROR_LASER_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
}

#endif	//CLIENT_DLL
