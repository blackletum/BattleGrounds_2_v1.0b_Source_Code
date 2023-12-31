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
		Tomas "Tjoppen" H�rdin		tjoppen@gamedev.se

	You may also contact the (future) team via the Battle Grounds website and/or forum at:
		www.bgmod.com

	 Note that because of the sheer volume of files in the Source SDK this
	notice cannot be put in all of them, but merely the ones that have any
	changes from the original SDK.
	 In order to facilitate easy searching, all changes are and must be
	commented on the following form:

	//BG2 - <name of contributer>[ - <small description>]
*/

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"

#define BLUDGEON_HULL_DIM		16

static const Vector g_bludgeonMins(-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM);
static const Vector g_bludgeonMaxs(BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM);

#ifdef CLIENT_DLL
#define CBaseBG2Weapon C_BaseBG2Weapon
#endif

class CBaseBG2Weapon : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CBaseBG2Weapon, CBaseHL2MPCombatWeapon );
public:
	enum
	{
		ATTACKTYPE_NONE = 0,
		ATTACKTYPE_STAB,
		ATTACKTYPE_SLASH,
		ATTACKTYPE_FIREARM,
		/*
		other attacktypes might be
		ATTACKTYPE_THROW,

		*/
	};

	enum
	{
		ATTACK_NONE = -1,		//this enum used for separating different attacks and stuff
		ATTACK_PRIMARY = 0,
		ATTACK_SECONDARY = 1,
	};

	struct attackinfo
	{
		int		m_iAttacktype;
		Activity	m_iAttackActivity,
					m_iAttackActivityEmpty;

		float	m_flRange,
				m_flDamage,
				m_flRecoil,				//factor of 357 standard recoil - firearm only
				//m_flCosAngleTolerance,	//tolerance of melee hits(sqrt(0.5) for crowbar, or 45 degrees)
										// - melee only
				m_flAttackrate;

		/*Vector	m_vDuckSpread,			//firearm only
				m_vStandSpread;			//firearm only*/
		
		//cone values.. firearms only
		float	m_flStandMoving,
				m_flStandStill,
				m_flCrouchMoving,
				m_flCrouchStill,
				m_flConstantDamageRange,	//how long until we start losing damage?
				m_flRelativeDrag;			//how does the drag on this bullet compare to a musket's?
	};

	attackinfo	m_Attackinfos[2];

	/*float		m_flRange,
				m_flFireRate,
				m_flImpactDelay,
				m_flShotDamage,
				m_flStabDamage;

	Vector		m_vDuckSpread,
				m_vStandSpread;*/

	const char	*m_pBayonetDeathNotice;

	CBaseBG2Weapon( void );

	void	PrimaryAttack( void );
	void	SecondaryAttack( void );

	//BG2 - Tjoppen - this one from base_weapon.cpp
	//works out the accuracy cone, so we can use cones of varying sizes
	Vector Cone(float flConeSize)
	{
		const double flBaseSize = 0.008725;
		float flCone = (float)flBaseSize * flConeSize;

		return Vector(flCone, flCone, flCone);
	}

	int		GetAttackType( int iAttack )
	{
		return iAttack != ATTACK_NONE ? m_Attackinfos[iAttack].m_iAttacktype : ATTACK_NONE;
	}

	float	GetRange( int iAttack )
	{
		return iAttack != ATTACK_NONE ? m_Attackinfos[iAttack].m_flRange : 0;
	}

	float	GetAttackRate( int iAttack )
	{
		if( iAttack == ATTACK_NONE )
			return 0;

		if( m_Attackinfos[iAttack].m_flAttackrate < 0 )
			return SequenceDuration();

		return m_Attackinfos[iAttack].m_flAttackrate;
	}

	float	GetRecoil( int iAttack )
	{
		if( iAttack == ATTACK_NONE )
			return 0;

		return m_Attackinfos[iAttack].m_flRecoil;
	}

	float	GetDamage( int iAttack )
	{
		if( iAttack == ATTACK_NONE )
			return 0;

		float scale = GetAttackType( iAttack ) == ATTACKTYPE_STAB ? 0.8f : 1.0f;	//adjusting stabs

		return m_Attackinfos[iAttack].m_flDamage * scale;/*
					* random->RandomFloat( 0.97f, 1.03f );	//+-3% dmg*/
	}

	Vector	GetSpread( int iAttack )
	{
		if( iAttack == ATTACK_NONE )
			return vec3_origin;

		bool moving = false;
		if( GetOwner() && GetOwner()->GetLocalVelocity().Length() > 10.0f ||
			!(GetOwner()->GetFlags() & FL_ONGROUND) )
			moving = true;	//moving fast enough or jumping increases spread..

		/*if( GetOwner() && (GetOwner()->GetFlags() & FL_DUCKING) )
			return m_Attackinfos[iAttack].m_vDuckSpread * (moving ? 2.0f : 1.0f);

		return m_Attackinfos[iAttack].m_vStandSpread * (moving ? 2.0f : 1.0f);*/

		if( GetOwner() && (GetOwner()->GetFlags() & FL_DUCKING) )
		{
			if( moving )
				return Cone( m_Attackinfos[iAttack].m_flCrouchMoving );
			else
				return Cone( m_Attackinfos[iAttack].m_flCrouchStill );
		}
		else
		{
			if( moving )
				return Cone( m_Attackinfos[iAttack].m_flStandMoving );
			else
				return Cone( m_Attackinfos[iAttack].m_flStandStill );
		}
	}

	Activity	GetActivity( int iAttack )
	{
		if( iAttack == ATTACK_NONE )
			return (Activity)0;

		if( m_iClip1 == 0 && m_Attackinfos[iAttack].m_iAttacktype == ATTACKTYPE_STAB )
			return m_Attackinfos[iAttack].m_iAttackActivityEmpty;

		return m_Attackinfos[iAttack].m_iAttackActivity;
	}

	//minor hack to get bayonet deathnotice
	bool m_bLastAttackStab;
	char *GetDeathNoticeName( void )
	{
		if( m_bLastAttackStab && m_pBayonetDeathNotice )
			return (char*)m_pBayonetDeathNotice;
		else
			return (char*)GetClassname();
	}

	float	m_flLastRecoil;		//for multiple recoil supression on client

	/*bool	IsMeleeWeapon( void )
	{
		return m_Attackinfos[ATTACK_PRIMARY].m_iAttacktype == ATTACKTYPE_STAB ? true : false;
	}*/

	//void		ItemPostFrame( void );
	int			Fire( int iAttack );
	int			FireBullet( int iAttack );
	int			Swing( int iAttack );

	void		Hit( trace_t &traceHit, int iAttack );
	Activity	ChooseIntersectionPointAndActivity( int iAttack, trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner );
	bool		ImpactWater( const Vector &start, const Vector &end );
	void		ImpactEffect( trace_t &traceHit );

	void		Drop( const Vector &vecVelocity );

	virtual void		WeaponIdle( void );
	virtual Activity	GetDrawActivity( void );

	bool		Holster( CBaseCombatWeapon *pSwitchingTo );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

/*#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif*/

private:
	CBaseBG2Weapon( const CBaseBG2Weapon & );
};


//these things must go before the macro. appropriately named of course(CWeapon##name C_Weapon##name
//the reason for this is that #define's can't be inside other #define's
//#ifdef CLIENT_DLL
//#define CWeaponRevolutionnaire C_WeaponRevolutionnaire
//#endif
#define DECLARE_BG2_WEAPON( name )								\
class CWeapon##name : public CBaseBG2Weapon {					\
	DECLARE_CLASS( CWeapon##name, CBaseBG2Weapon );				\
public:															\
	CWeapon##name( void );										\
	DECLARE_NETWORKCLASS();										\
	DECLARE_PREDICTABLE();										\
	DECLARE_ACTTABLE();											\
private:														\
	CWeapon##name( const CWeapon##name & );						\
};																\
IMPLEMENT_NETWORKCLASS_ALIASED( Weapon##name, DT_Weapon##name )	\
BEGIN_NETWORK_TABLE( CWeapon##name, DT_Weapon##name )			\
END_NETWORK_TABLE()												\
BEGIN_PREDICTION_DATA( CWeapon##name )							\
END_PREDICTION_DATA()											\
LINK_ENTITY_TO_CLASS( weapon_##name, CWeapon##name );			\
PRECACHE_WEAPON_REGISTER( weapon_##name );						\
CWeapon##name::CWeapon##name( void )							//constructor follows

//acttable def for bayonet firearms, only muskets so far. could a rifle use a bayonet? it's just a name anyway
#define MUSKET_ACTTABLE( name )\
acttable_t CWeapon##name::m_acttable[] = \
{\
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },\
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },\
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },\
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },\
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },\
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },\
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },\
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },\
	{ ACT_RANGE_ATTACK2,				ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,					false },\
};\
IMPLEMENT_ACTTABLE( CWeapon##name );

//acttable def for rifles, without bayo
#define RIFLE_ACTTABLE( name )\
acttable_t CWeapon##name::m_acttable[] = \
{\
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },\
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },\
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },\
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },\
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },\
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },\
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },\
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },\
	{ ACT_RANGE_ATTACK2,				ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,					false },\
};\
IMPLEMENT_ACTTABLE( CWeapon##name );

//acttable def for pistols
#define PISTOL_ACTTABLE( name )\
acttable_t CWeapon##name::m_acttable[] = \
{\
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },\
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },\
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },\
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },\
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },\
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },\
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },\
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },\
	{ ACT_RANGE_ATTACK2,				ACT_VM_SECONDARYATTACK,					false },\
};\
IMPLEMENT_ACTTABLE( CWeapon##name );

//acttable def for slash weapons
#define MELEE_ACTTABLE( name )\
acttable_t CWeapon##name::m_acttable[] =\
{\
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM,					true },\
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE,					false },\
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE,					false },\
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },\
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },\
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },\
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },\
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE,					false },\
	{ ACT_RANGE_ATTACK2,				ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },\
};\
IMPLEMENT_ACTTABLE( CWeapon##name );
