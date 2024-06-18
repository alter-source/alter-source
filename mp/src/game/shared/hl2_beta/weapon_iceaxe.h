//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2MP_WEAPON_ICEAXE_H
#define HL2MP_WEAPON_ICEAXE_H
#pragma once


#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"


#ifdef CLIENT_DLL
#define CWeaponIceaxe C_WeaponIceaxe
#endif

//-----------------------------------------------------------------------------
// CWeaponIceaxe
//-----------------------------------------------------------------------------

class CWeaponIceaxe : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponIceaxe, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponIceaxe();

	float		GetRange( void );
	float		GetFireRate( void );

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		SecondaryAttack( void )	{	return;	}

	void		Drop( const Vector &vecVelocity );


	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	int WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif

	CWeaponIceaxe( const CWeaponIceaxe & );

private:
		
};


#endif // HL2MP_WEAPON_ICEAXE_H

