////////////////////////////////////////////////////////////////////////////
//     HALF-LIFE 2: BETA DEATHMATCH
//
//     Weapon: HMG1
//
////////////////////////////////////////////////////////////////////////////	   
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
 
#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif
 
#include "weapon_hl2mpbasehlmpcombatweapon.h"
 
//modify this to alter the rate of fire
//#define ROF 0.080f //800 rounds/min
//modify this to alter the length of the burst. Set it to 1 to make a gun Semi-Auto. Set it to some really big number, like 1000 to make it full auto.
#define BURST 500;
 
#ifdef CLIENT_DLL
#define CWeaponHMG1 C_WeaponHMG1
#endif
 
//-----------------------------------------------------------------------------
// CWeaponHMG1
//-----------------------------------------------------------------------------
 
class CWeaponHMG1 : public CBaseHL2MPCombatWeapon
{
public:
DECLARE_CLASS( CWeaponHMG1, CBaseHL2MPCombatWeapon );
 
CWeaponHMG1(void);
 
DECLARE_NETWORKCLASS();
DECLARE_PREDICTABLE();
 
void Precache( void );
void ItemPostFrame( void );
//void ItemPreFrame( void );
//void ItemBusyFrame( void );
void PrimaryAttack( void );
void SecondaryAttack( void );
void AddViewKick( void );
void DryFire( void );
void GetStance( void );
bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL ); // Required so that you know to un-zoom when switching weapons
Activity GetPrimaryAttackActivity( void );
 
virtual bool Reload( void );
 
int GetMinBurst() { return 2; }
int GetMaxBurst() { return 5; }
float GetFireRate( void ); //{return ROF; } This edit is for the "Slow Down Fire when Zoomed" feature that you see in the Beta.
 
//modify this part to control the accuracy
virtual const Vector& GetBulletSpread( void )
{
static Vector cone=VECTOR_CONE_1DEGREES;
//if (m_bInZoom)
//{
if (m_iStance==E_DUCK) { cone = VECTOR_CONE_1DEGREES;}
if (m_iStance==E_STAND) { cone = VECTOR_CONE_1DEGREES;}
if (m_iStance==E_MOVE) { cone = VECTOR_CONE_2DEGREES;}
if (m_iStance==E_RUN) { cone = VECTOR_CONE_3DEGREES;}
if (m_iStance==E_INJURED) { cone = VECTOR_CONE_4DEGREES;}
if (m_iStance==E_JUMP) { cone = VECTOR_CONE_5DEGREES;}
if (m_iStance==E_DYING) { cone = VECTOR_CONE_10DEGREES;}
//}
//if (!m_bInZoom)
//{
//if (m_iStance==E_DUCK) { cone = VECTOR_CONE_1DEGREES;}
//if (m_iStance==E_STAND) { cone = VECTOR_CONE_1DEGREES;}
//if (m_iStance==E_MOVE) { cone = VECTOR_CONE_2DEGREES;}
//if (m_iStance==E_RUN) { cone = VECTOR_CONE_3DEGREES;}
//if (m_iStance==E_INJURED) { cone = VECTOR_CONE_4DEGREES;}
//if (m_iStance==E_JUMP) { cone = VECTOR_CONE_5DEGREES;}
//if (m_iStance==E_DYING) { cone = VECTOR_CONE_10DEGREES;}
//}
int bs=BURST;
if (m_iBurst!=bs)
{
for (int i=0;i<(bs-m_iBurst);i++)
{cone=cone+VECTOR_CONE_1DEGREES;}
}
 
return cone;
}
//void ToggleZoom( void );
void CheckZoomToggle( void );
 
 
DECLARE_ACTTABLE();
 
 
private:
CNetworkVar( int, m_iBurst );
//CNetworkVar( bool, m_bInZoom );
CNetworkVar( float, m_flAttackEnds );
CNetworkVar( int, m_iStance);
CNetworkVar( int, m_nShotsFired);
 
private:
CWeaponHMG1( const CWeaponHMG1 & );
};
 
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponHMG1, DT_WeaponHMG1 )
 
BEGIN_NETWORK_TABLE( CWeaponHMG1, DT_WeaponHMG1 )
#ifdef CLIENT_DLL
RecvPropInt( RECVINFO( m_iBurst) ),
//RecvPropBool( RECVINFO( m_bInZoom ) ),
RecvPropTime( RECVINFO( m_flAttackEnds ) ),
RecvPropInt( RECVINFO( m_iStance ) ),
RecvPropInt( RECVINFO( m_nShotsFired) ),
#else
SendPropInt( SENDINFO( m_iBurst ) ),
//SendPropBool( SENDINFO( m_bInZoom ) ),
SendPropTime( SENDINFO( m_flAttackEnds ) ),
SendPropInt( SENDINFO( m_iStance ) ),
SendPropInt( SENDINFO( m_nShotsFired) ),
#endif
END_NETWORK_TABLE()
 
BEGIN_PREDICTION_DATA( CWeaponHMG1 )
END_PREDICTION_DATA()
 
LINK_ENTITY_TO_CLASS( weapon_HMG1, CWeaponHMG1 );
PRECACHE_WEAPON_REGISTER( weapon_HMG1 );
 
 
acttable_t CWeaponHMG1::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_AR2,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_AR2,				false },
 
	{ ACT_MP_RUN,						ACT_HL2MP_RUN_AR2,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_AR2,				false },
 
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
 
	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_AR2,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_AR2,			false },
 
	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_AR2,					false },
 
};
 
 
IMPLEMENT_ACTTABLE( CWeaponHMG1 );
 
 
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponHMG1::CWeaponHMG1( void )
{
 
m_iBurst=BURST;
m_iStance=10;
 
m_fMinRange1 = 65;
m_fMaxRange1 = 2048;
m_fMinRange2 = 256;
m_fMaxRange2 = 1024;
 
m_bFiresUnderwater = false;

m_nShotsFired	= 0;
 
 
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponHMG1::Precache( void )
{
BaseClass::Precache();
}
 
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponHMG1::DryFire( void )
{
WeaponSound( EMPTY );
SendWeaponAnim( ACT_VM_DRYFIRE );
 
m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponHMG1::PrimaryAttack( void )
{
if (m_iBurst!=0)
{
CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
 
	if ( !pPlayer )
	{
		return;
	}
 
	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();
 
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	//ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
 
if( pOwner )
{
// Each time the player fires the pistol, reset the view punch. This prevents
// the aim from 'drifting off' when the player fires very quickly. This may
// not be the ideal way to achieve this, but it's cheap and it works, which is
// great for a feature we're evaluating. (sjb)
pOwner->ViewPunchReset();
}
 
BaseClass::PrimaryAttack();
 
// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
m_iBurst--;
 
m_flNextPrimaryAttack =gpGlobals->curtime + GetFireRate();
m_flAttackEnds = gpGlobals->curtime + SequenceDuration();

m_nShotsFired++;
 
}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponHMG1::SecondaryAttack( void )
{
BaseClass::SecondaryAttack();
}
 
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
//void CWeaponHMG1::ItemPreFrame( void )
//{
 
//GetStance();
//BaseClass::ItemPreFrame();
//}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
//void CWeaponHMG1::ItemBusyFrame( void )
//{
	// Allow zoom toggling even when we're reloading
	//CheckZoomToggle();
 
	//BaseClass::ItemBusyFrame();
//}
 
//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponHMG1::ItemPostFrame( void )
{
BaseClass::ItemPostFrame();
 
if ( m_bInReload )
return;
 
CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
if ( pOwner == NULL )
return;
if ( pOwner->m_nButtons & IN_ATTACK )
{
if (m_flAttackEnds<gpGlobals->curtime)
{
SendWeaponAnim(ACT_VM_IDLE);
}
}
else
{
m_iBurst=BURST;
if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
{
DryFire();
}
}
//CheckZoomToggle();
GetStance();

 
}
 
//-----------------------------------------------------------------------------
// Purpose:
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponHMG1::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_HITLEFT;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_HITLEFT2;

	return ACT_VM_HITRIGHT;

//if (m_iBurst!=0)
//{
//return ACT_VM_PRIMARYATTACK;
//}
//else
//{
//return ACT_VM_IDLE;
//}
}
 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponHMG1::Reload( void )
{
bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
if ( fRet )
{
WeaponSound( RELOAD );
//ToHL2MPPlayer(GetOwner())->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
m_iBurst=BURST;
}
return fRet;

//if ( m_bInZoom )
//{
//ToggleZoom();
//}

}
 
bool CWeaponHMG1::Holster(CBaseCombatWeapon *pSwitchingTo /* = NULL */)
{
//if ( m_bInZoom )
//{
//ToggleZoom();
//}
 
return BaseClass::Holster( pSwitchingTo );
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponHMG1::AddViewKick( void )
{
CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
 
if ( pPlayer == NULL )
return;
 
int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
RandomSeed( iSeed );
 
QAngle viewPunch;
 
viewPunch.x = random->RandomFloat( 0.25f, 0.5f );
viewPunch.y = random->RandomFloat( -.6f, .6f );
viewPunch.z = 0.0f;
 
//Add it to the view punch
pPlayer->ViewPunch( viewPunch );
}
 
//void CWeaponHMG1::ToggleZoom( void )
//{
//CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
 
//if ( pPlayer == NULL )
//return;
//color32 lightGreen = { 50, 255, 170, 32 };
//#ifndef CLIENT_DLL
//if ( m_bInZoom )
//{
// Narrowing the Field Of View here is what gives us the zoomed effect
//if ( pPlayer->SetFOV( this, 0, 0.2f ) )
//{
//WeaponSound(SPECIAL2);
//m_bInZoom = false;
//UTIL_ScreenFade( pPlayer, lightGreen, 0.2f, 0, (FFADE_IN|FFADE_PURGE) );
 
// Send a message to hide the scope
/* CSingleUserRecipientFilter filter(pPlayer);
UserMessageBegin(filter, "ShowScope");
WRITE_BYTE(0);
MessageEnd();*/
//}
//}
//else
//{
//if ( pPlayer->SetFOV( this, 45, 0.1f ) )
//{
//WeaponSound(SPECIAL1);
//m_bInZoom = true;
//UTIL_ScreenFade( pPlayer, lightGreen, 0.2f, 0, (FFADE_OUT|FFADE_PURGE|FFADE_STAYOUT) );
 
// Send a message to Show the scope
/* CSingleUserRecipientFilter filter(pPlayer);
UserMessageBegin(filter, "ShowScope");
WRITE_BYTE(1);
MessageEnd();*/
//}
//}
//#endif
//}

float CWeaponHMG1::GetFireRate( void )
{
	return 0.080f;

	//old code

	//CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	//if (pPlayer->m_nButtons &= ~IN_ATTACK2);
	//if ( pPlayer && (pPlayer->m_afButtonPressed & IN_ATTACK2) );
		//return 0.3f;
}
 
//void CWeaponHMG1::CheckZoomToggle( void )
//{
//CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
 
//if ( pPlayer && (pPlayer->m_afButtonPressed & IN_ATTACK2))
//{
//ToggleZoom();
//}
//}
 
void CWeaponHMG1::GetStance( void )
{
 
CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
if ( pPlayer == NULL )
return;
 
if (pPlayer->m_nButtons & IN_DUCK) { m_iStance= E_DUCK;} else { m_iStance= E_STAND;}
if (pPlayer->m_nButtons & IN_FORWARD) { m_iStance= E_MOVE;}
if (pPlayer->m_nButtons & IN_BACK) { m_iStance= E_MOVE;}
if (pPlayer->m_nButtons & IN_MOVERIGHT) { m_iStance= E_MOVE;}
if (pPlayer->m_nButtons & IN_MOVELEFT) { m_iStance= E_MOVE;}
if (pPlayer->m_nButtons & IN_RUN) { m_iStance= E_RUN;}
if (pPlayer->m_nButtons & IN_SPEED) { m_iStance= E_RUN;}
if (pPlayer->m_nButtons & IN_JUMP) { m_iStance= E_JUMP;}
if ( pPlayer->GetHealth()<25) { m_iStance= E_INJURED;}
if ( pPlayer->GetHealth()<10) { m_iStance= E_DYING;}
 
}