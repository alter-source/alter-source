//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#define	ALYXGUN_FASTEST_REFIRE_TIME		0.1f
#define	ALYXGUN_FASTEST_DRY_REFIRE_TIME	0.2f

#define	ALYXGUN_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	ALYXGUN_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

#ifdef CLIENT_DLL
#define CWeaponAlyxGun C_WeaponAlyxGun
#endif

//-----------------------------------------------------------------------------
// CWeaponAlyxGun
//-----------------------------------------------------------------------------

class CWeaponAlyxGun : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponAlyxGun, CBaseHL2MPCombatWeapon );

	CWeaponAlyxGun(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	Precache( void );
	void	ItemPostFrame( void );
	void	ItemPreFrame( void );
	void	ItemBusyFrame( void );
	void	PrimaryAttack( void );
	void	AddViewKick( void );
	void	DryFire( void );

	void   	SecondaryAttack( void );

	void	UpdatePenaltyTime( void );

	Activity	GetPrimaryAttackActivity( void );

	virtual bool Reload( void );

	virtual const Vector& GetBulletSpread( void )
	{		
		static Vector cone;

		float ramp = RemapValClamped(	m_flAccuracyPenalty, 
											0.0f, 
											ALYXGUN_ACCURACY_MAXIMUM_PENALTY_TIME, 
											0.0f, 
											1.0f ); 

			// We lerp from very accurate to inaccurate over time
		VectorLerp( VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone );

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

	virtual float GetFireRate( void ) 
	{
		return 0.5f; 
	}
	
#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	CNetworkVar( float,	m_flSoonestPrimaryAttack );
	CNetworkVar( float,	m_flLastAttackTime );
	CNetworkVar( float,	m_flAccuracyPenalty );
	CNetworkVar( int,	m_nNumShotsFired );

private:
	CWeaponAlyxGun( const CWeaponAlyxGun & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAlyxGun, DT_WeaponAlyxGun )

BEGIN_NETWORK_TABLE( CWeaponAlyxGun, DT_WeaponAlyxGun )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flSoonestPrimaryAttack ) ),
	RecvPropTime( RECVINFO( m_flLastAttackTime ) ),
	RecvPropFloat( RECVINFO( m_flAccuracyPenalty ) ),
	RecvPropInt( RECVINFO( m_nNumShotsFired ) ),
#else
	SendPropTime( SENDINFO( m_flSoonestPrimaryAttack ) ),
	SendPropTime( SENDINFO( m_flLastAttackTime ) ),
	SendPropFloat( SENDINFO( m_flAccuracyPenalty ) ),
	SendPropInt( SENDINFO( m_nNumShotsFired ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponAlyxGun )
	DEFINE_PRED_FIELD( m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_alyx_gun, CWeaponAlyxGun );
PRECACHE_WEAPON_REGISTER( weapon_alyx_gun );

#ifndef CLIENT_DLL
acttable_t CWeaponAlyxGun::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
};


IMPLEMENT_ACTTABLE( CWeaponAlyxGun );

#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponAlyxGun::CWeaponAlyxGun( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1		= 24;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;

	m_bFiresUnderwater	= false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::Precache( void )
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flSoonestPrimaryAttack	= gpGlobals->curtime + ALYXGUN_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::PrimaryAttack( void )
{
	if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.5f )
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		m_nNumShotsFired++;
	}

	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestPrimaryAttack = gpGlobals->curtime + ALYXGUN_FASTEST_REFIRE_TIME;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner )
	{
		// Each time the player fires the alyxgun, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += ALYXGUN_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponAlyxGun::SecondaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;
	
	// Abort here to handle burst and auto fire modes
	if ( (UsesClipsForAmmo1() && m_iClip1 == 0) || ( !UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType) ) )
		return;

	m_nNumShotsFired++;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = 0.075f;

	while ( m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CHLMachineGun
		WeaponSound(SINGLE, m_flNextSecondaryAttack);
		m_flNextSecondaryAttack = m_flNextSecondaryAttack + fireRate;
		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if ( UsesClipsForAmmo1() )
	{
		if ( iBulletsToFire > m_iClip1 )
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}

	CHL2MP_Player *pHL2MPPlayer = ToHL2MPPlayer( pPlayer );

		// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pHL2MPPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	info.m_vecSpread = pHL2MPPlayer->GetAttackSpread( this );
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;
	FireBullets( info );

	//Factor in the view kick
	AddViewKick();
	
	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	SendWeaponAnim( GetSecondaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Check our penalty time decay
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, ALYXGUN_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::ItemPreFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::ItemBusyFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
		
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;
	
	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		m_flLastAttackTime = gpGlobals->curtime + ALYXGUN_FASTEST_REFIRE_TIME;
		m_flSoonestPrimaryAttack = gpGlobals->curtime + ALYXGUN_FASTEST_REFIRE_TIME;
		m_flNextPrimaryAttack = gpGlobals->curtime + ALYXGUN_FASTEST_REFIRE_TIME;
	}

	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		DryFire();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponAlyxGun::GetPrimaryAttackActivity( void )
{
	if ( m_nNumShotsFired < 1 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nNumShotsFired < 2 )
		return ACT_VM_RECOIL1;

	if ( m_nNumShotsFired < 3 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponAlyxGun::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
		m_flAccuracyPenalty = 0.0f;
	}
	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAlyxGun::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "alyxgunpax", 0.25f, 0.5f );
	viewPunch.y = SharedRandomFloat( "alyxgunpay", -.6f, .6f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}
