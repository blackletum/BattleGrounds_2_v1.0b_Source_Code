/*
	The Battle Grounds 2 - A Source modification
	Copyright (C) 2005, The Battle Grounds 2 Team and Contributors

	The Battle Grounds 2 free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	The Battle Grounds 2 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

	Contact information:
		Tomas "Tjoppen" Härdin		tjoppen@gamedev.se

	You may also contact the (future) team via the Battle Grounds website and/or forum at:
		www.bgmod.com

	 Note that because of the sheer volume of files in the Source SDK this
	notice cannot be put in all of them, but merely the ones that have any
	changes from the original SDK.
	 In order to facilitate easy searching, all changes are and must be
	commented on the following form:

	//BG2 - <name of contributer>[ - <small description>]
*/

#include "weapon_bg2base.h"
#include "ammodef.h"

//BG2 - Draco - no hl2 player stuff on server!
#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"

	#include "cbase.h"
	#include "model_types.h"
	#include "ClientEffectPrecacheSystem.h"
	#include "fx.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "IEffects.h"
	#include "Sprite.h"
	#include "SpriteTrail.h"
	#include "beam_shared.h"

	#include "shot_manipulator.h"
#endif

#include "takedamageinfo.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "effect_dispatch_data.h"
#include "bullet.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_simulatedbullets( "sv_simulatedbullets", "1", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEMO,
		"EXPERIMENTAL!\nWhen non-zero, makes all firearms shoot \"real\" bullets.");

ConVar sv_simulatedbullets_drag( "sv_simulatedbullets_drag", "0.00003", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEMO,
	   "Tweak this value to affect how fast the speed and thus damage of bullets drop off with distance.\n\tLower values => more damage over distance" );

ConVar sv_simulatedbullets_overshoot_range( "sv_simulatedbullets_overshoot_range", "50", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEMO,
	   "At what range in yards overshoot reaches maximum" );

ConVar sv_simulatedbullets_overshoot_force( "sv_simulatedbullets_overshoot_force", "3", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEMO,
	   "How much stronger than gravity is the overshoot force at t=0?" );

ConVar sv_infiniteammo( "sv_infiniteammo", "0", FCVAR_CHEAT | FCVAR_NOTIFY | FCVAR_REPLICATED, "Bullet weapons don\'t use up ammo\n" );
ConVar sv_turboshots( "sv_turboshots", "0", FCVAR_CHEAT | FCVAR_NOTIFY | FCVAR_REPLICATED, "Turns all guns into machineguns\n" );
ConVar sv_perfectaim( "sv_perfectaim", "0", FCVAR_CHEAT | FCVAR_NOTIFY | FCVAR_REPLICATED, "No spread for bullet weapons\n" );
ConVar sv_steadyhand( "sv_steadyhand", "0", FCVAR_CHEAT | FCVAR_NOTIFY | FCVAR_REPLICATED, "No recoil for bullet weapons\n" );

ConVar mp_disable_melee( "mp_disable_melee", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "When non-zero, melee weapons are disabled" );
ConVar mp_disable_firearms( "mp_disable_firearms", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "When non-zero, firearms are disabled" );


//-----------------------------------------------------------------------------
// CBaseBG2Weapon
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( BaseBG2Weapon, DT_BaseBG2Weapon )

BEGIN_NETWORK_TABLE( CBaseBG2Weapon, DT_BaseBG2Weapon )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CBaseBG2Weapon )
END_PREDICTION_DATA()

/*LINK_ENTITY_TO_CLASS( weapon_revolutionnaire, CBaseBG2Weapon );
PRECACHE_WEAPON_REGISTER( weapon_revolutionnaire );*/

#ifndef CLIENT_DLL

#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBaseBG2Weapon::CBaseBG2Weapon( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= true;
	m_bDontAutoreload	= true;

	m_Attackinfos[0].m_iAttacktype = ATTACKTYPE_NONE;
	m_Attackinfos[1].m_iAttacktype = ATTACKTYPE_NONE;

	//m_nViewModelIndex	= random->RandomInt( 0, 1 );	//test..
}

//PrimaryAttack
void CBaseBG2Weapon::PrimaryAttack( void )
{
	//disallow holding reload button
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner == NULL || pOwner->m_nButtons & IN_RELOAD || m_bInReload )
		return;

	int drain = 0;
	if( GetAttackType( ATTACK_PRIMARY ) == ATTACKTYPE_STAB || GetAttackType( ATTACK_PRIMARY ) == ATTACKTYPE_SLASH )
	{
		if( GetOwner() && (GetOwner()->GetFlags() & FL_DUCKING) )
			return;

		if( mp_disable_melee.GetInt() )
			return;

		drain = Swing( ATTACK_PRIMARY );
	}
	else if( GetAttackType( ATTACK_PRIMARY ) == ATTACKTYPE_FIREARM )
	{
		if( mp_disable_firearms.GetInt() )
			return;

		drain = Fire( ATTACK_PRIMARY );
	}
	else
		return;	//don't drain stamina

	//BG2 - Draco - decrease stam when attacking
#ifndef CLIENT_DLL
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer( GetOwner() );

	pHL2Player->m_iStamina -= drain;
	if( pHL2Player->m_iStamina < 0 )
		pHL2Player->m_iStamina = 0;
#endif
}

//SecondaryAttack
void CBaseBG2Weapon::SecondaryAttack( void )
{
	//disallow holding reload button
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner == NULL || pOwner->m_nButtons & IN_RELOAD || m_bInReload )
		return;

	int drain = 0;
	if( GetAttackType( ATTACK_SECONDARY ) == ATTACKTYPE_STAB || GetAttackType( ATTACK_SECONDARY ) == ATTACKTYPE_SLASH )
	{
		if( GetOwner() && (GetOwner()->GetFlags() & FL_DUCKING) )
			return;

		if( mp_disable_melee.GetInt() )
			return;

		drain = Swing( ATTACK_SECONDARY );
	}
	else if( GetAttackType( ATTACK_SECONDARY ) == ATTACKTYPE_FIREARM )
	{
		if( mp_disable_firearms.GetInt() )
			return;

		drain = Fire( ATTACK_SECONDARY );
	}
	else
		return;	//don't drain stamina

	//BG2 - Draco - decrease stam when attacking
#ifndef CLIENT_DLL
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer( GetOwner() );

	pHL2Player->m_iStamina -= drain;
	if( pHL2Player->m_iStamina < 0 )
		pHL2Player->m_iStamina = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CBaseBG2Weapon::Fire( int iAttack )
{
	m_bLastAttackStab = false;

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return 0;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.15;
		}

		return 0;
	}

	if( pPlayer->GetWaterLevel() == 3 )
	{
		// This weapon doesn't fire underwater
		WeaponSound(EMPTY);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.2;
		return 0;
	}

	if( sv_turboshots.GetInt() == 0 )
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetAttackRate( iAttack );
	else
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;

	WeaponSound(SINGLE);

	if( sv_turboshots.GetInt() == 0 )
	{
		pPlayer->DoMuzzleFlash();

		//BG2 - Tjoppen -	the weapon animation !!MUST!! play before the player animation or there's
		//					 no muzzle flash. don't forget this! ever! it's annoying as hell!
		SendWeaponAnim( GetPrimaryAttackActivity() );

		// player "shoot" animation
		pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	
	if( sv_infiniteammo.GetInt() == 0 )
		m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );	

	if( sv_simulatedbullets.GetBool() )
	{
#ifndef CLIENT_DLL
		CShotManipulator Manipulator( vecAiming );
		Vector vecDir = Manipulator.ApplySpread( sv_perfectaim.GetInt() == 0 ? GetSpread( iAttack ) : vec3_origin );

		QAngle angDir;
		VectorAngles( vecDir, angDir );

		CBullet::BulletCreate( vecSrc, angDir, GetDamage(iAttack),
								m_Attackinfos[iAttack].m_flConstantDamageRange,
								m_Attackinfos[iAttack].m_flRelativeDrag, pPlayer );
#endif
	}
	else
	{
		FireBulletsInfo_t info( 1, vecSrc, vecAiming,
								sv_perfectaim.GetInt() == 0 ? GetSpread( iAttack ) : vec3_origin,
								GetRange( iAttack ), m_iPrimaryAmmoType );
		info.m_pAttacker = pPlayer;
		info.m_iPlayerDamage = GetDamage( iAttack );
		info.m_iDamage = -1;		//ancient chinese secret..
		info.m_iTracerFreq = 1;		//always do tracers

		// Fire the bullets, and force the first shot to be perfectly accuracy
		pPlayer->FireBullets( info );
	}

	//Disorient the player
	if( sv_steadyhand.GetInt() == 0 )
	{
		int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
		RandomSeed( iSeed );

		//BG2 - Tjoppen - HACKHACK: weapon attacks get called multiple times on client. until we figure out
		//							why, multiple recoils must be supressed.
		if( m_flLastRecoil + 0.1f < gpGlobals->curtime )
			pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) * GetRecoil(iAttack) );

		m_flLastRecoil = gpGlobals->curtime;
	}

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}

	return 15;
}

//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CBaseBG2Weapon::Hit( trace_t &traceHit, int iAttack )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if( pHitEntity != NULL )
	{
		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		float	damage		= GetDamage( iAttack );	

		//BG2 - Tjoppen - apply no force
		CTakeDamageInfo info( GetOwner(), GetOwner(), damage, DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE | DMG_NEVERGIB );
		info.SetDamagePosition( traceHit.endpos );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit );	//negative dir for weird reasons
		ApplyMultiDamage();

		//WeaponSound( MELEE_HIT );
		if( pHitEntity->IsPlayer() )
		{
			WeaponSound( MELEE_HIT );
#ifndef CLIENT_DLL
			ImpactEffect( traceHit );
#endif
		}
		else
		{
			WeaponSound( MELEE_HIT_WORLD );
			if( GetAttackType(iAttack) != ATTACKTYPE_SLASH )
				ImpactEffect( traceHit );
		}
	}
	else if( GetAttackType(iAttack) != ATTACKTYPE_SLASH )
	{
		// Apply an impact effect
		ImpactEffect( traceHit );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &traceHit - 
//-----------------------------------------------------------------------------
bool CBaseBG2Weapon::ImpactWater( const Vector &start, const Vector &end )
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...
	
	// We must start outside the water
	if ( UTIL_PointContents( start ) & (CONTENTS_WATER|CONTENTS_SLIME))
		return false;

	// We must end inside of water
	if ( !(UTIL_PointContents( end ) & (CONTENTS_WATER|CONTENTS_SLIME)))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine( start, end, (CONTENTS_WATER|CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace );

	if ( waterTrace.fraction < 1.0f )
	{
#ifndef CLIENT_DLL
		CEffectData	data;

		data.m_fFlags  = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = 8.0f;

		// See if we hit slime
		if ( waterTrace.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect( "watersplash", data );			
#endif
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseBG2Weapon::ImpactEffect( trace_t &traceHit )
{
	// See if we hit water (we don't do the other impact effects in this case)
	if ( ImpactWater( traceHit.startpos, traceHit.endpos ) )
		return;

	//FIXME: need new decals
	UTIL_ImpactTrace( &traceHit, DMG_BULLET );	//BG2 - Tjoppen - surface blood
}

int CBaseBG2Weapon::Swing( int iAttack )
{
	trace_t traceHit;

	// Try a ray
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	if( GetAttackType(iAttack) == ATTACKTYPE_STAB )
		m_bLastAttackStab = true;
	else
		m_bLastAttackStab = false;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange(iAttack);
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &traceHit );

#ifndef CLIENT_DLL
	// Like bullets, bludgeon traces have to trace against triggers.
	CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), GetDamage(iAttack), DMG_CLUB );

	TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin );
#endif

	if ( traceHit.fraction == 1.0f )
	{
		//miss, do any waterimpact stuff
		Vector testEnd = swingStart + forward * GetRange(iAttack);
		ImpactWater( swingStart, testEnd );

		WeaponSound( SPECIAL1 );	//miss
	}
	else
	{
		Hit( traceHit, iAttack );
	}

	// Send the anim
	SendWeaponAnim( GetActivity( iAttack ) );
	pOwner->SetAnimation( PLAYER_ATTACK2 );

	//BG2 - Draco - you cant stab very fast when your nackered, add quarter of a second
#ifndef CLIENT_DLL
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer( GetOwner() );
#else
	C_HL2MP_Player *pHL2Player = ToHL2MPPlayer( GetOwner() );
#endif

	if (pHL2Player->m_iStamina <= 35)
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetAttackRate(iAttack) + 0.25f;
	else
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetAttackRate(iAttack);

#ifndef CLIENT_DLL
	//BG2 - Tjoppen - melee lag fix - this makes sure we get lag compensation for the next five frames or so
	pHL2Player->NoteWeaponFired();
#endif

	return 25;
}

void CBaseBG2Weapon::Drop( const Vector &vecVelocity )
{
/*#ifndef CLIENT_DLL
	UTIL_Remove( this );
#endif*/
	BaseClass::Drop(vecVelocity);
}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CBaseBG2Weapon::WeaponIdle( void )
{
	//See if we should idle high or low
	if ( HasWeaponIdleTimeElapsed() ) 
	{
		if( m_iClip1 == 0 )
			SendWeaponAnim( ACT_VM_IDLE_EMPTY );
		else
			SendWeaponAnim( ACT_VM_IDLE );
	}
}

/*void CBaseBG2Weapon::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();
}*/

Activity CBaseBG2Weapon::GetDrawActivity( void )
{
	if( m_iClip1 == 0 )
		return ACT_VM_DRAW_EMPTY;

	return ACT_VM_DRAW;
}

bool CBaseBG2Weapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	//stop reload
	StopWeaponSound( RELOAD );

	return BaseClass::Holster( pSwitchingTo );
}