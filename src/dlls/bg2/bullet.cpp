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

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "effect_dispatch_data.h"
#include "bullet.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define BOLT_MODEL			"models/crossbow_bolt.mdl"
#define BOLT_MODEL			"models/game/musket_ball.mdl"
//#define BOLT_MODEL	"models/weapons/w_missile_closed.mdl"

//#define BOLT_AIR_VELOCITY	3500
#define BOLT_AIR_VELOCITY	14400
#define BOLT_WATER_VELOCITY	1500
/*#define	BOLT_SKIN_NORMAL	0
#define BOLT_SKIN_GLOW		1*/


#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( bullet, CBullet );

BEGIN_DATADESC( CBullet )
	// Function Pointers
	DEFINE_FUNCTION( BubbleThink ),
	DEFINE_FUNCTION( BoltTouch ),

	// These are recreated on reload, they don't need storage
	DEFINE_FIELD( m_pGlowSprite, FIELD_EHANDLE ),
	//DEFINE_FIELD( m_pGlowTrail, FIELD_EHANDLE ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CBullet, DT_Bullet )
END_SEND_TABLE()

CBullet *CBullet::BulletCreate( const Vector &vecOrigin, const QAngle &angAngles, int iDamage, float flConstantDamageRange, float flRelativeDrag, CBasePlayer *pentOwner )
{
	// Create a new entity with CBullet private data
	CBullet *pBullet = (CBullet *)CreateEntityByName( "bullet" );
	UTIL_SetOrigin( pBullet, vecOrigin );
	pBullet->SetAbsAngles( angAngles );
	Vector vecDir;
	AngleVectors( angAngles, &vecDir );
	pBullet->SetAbsVelocity( vecDir * BOLT_AIR_VELOCITY );

	pBullet->Spawn();
	pBullet->SetOwnerEntity( pentOwner );

	pBullet->m_iDamage = iDamage;

#define LIFETIME	10.f
	pBullet->m_flDyingTime = gpGlobals->curtime + LIFETIME;
	pBullet->m_flConstantDamageRange = flConstantDamageRange;
	pBullet->m_flRelativeDrag = flRelativeDrag;
	pBullet->m_vTrajStart = pBullet->GetAbsOrigin();
	pBullet->m_bHasPlayedNearmiss = false;

	return pBullet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBullet::~CBullet( void )
{
	if ( m_pGlowSprite )
	{
		UTIL_Remove( m_pGlowSprite );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBullet::CreateVPhysics( void )
{
	// Create the object in the physics system
	//BG2 - Tjoppen - SOLID_VPHYSICS
	//VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );
	VPhysicsInitNormal( SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false );	

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CBullet::PhysicsSolidMaskForEntity() const
{
	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*bool CBullet::CreateSprites( void )
{
	// Start up the eye glow
	m_pGlowSprite = CSprite::SpriteCreate( "sprites/light_glow02_noz.vmt", GetLocalOrigin(), false );

	if ( m_pGlowSprite != NULL )
	{
		m_pGlowSprite->FollowEntity( this );
		m_pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		m_pGlowSprite->SetScale( 0.2f );
		m_pGlowSprite->TurnOff();
	}

	return true;
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBullet::Spawn( void )
{
	Precache( );

	//SetModel( "models/crossbow_bolt.mdl" );
	SetModel( BOLT_MODEL );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	//UTIL_SetSize( this, -Vector(1,1,1), Vector(1,1,1) );
	//SetSolid( SOLID_BBOX );
	SetSolid( SOLID_VPHYSICS );
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	//SetGravity( 0.05f );
	SetGravity( 1.0f );
	//VPhysicsGetObject()->EnableDrag( true );
	/*VPhysicsGetObject()->Wake();
	VPhysicsGetObject()->SetMass( 1 );*/
	//SetFriction( 1000.0f );

	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CBullet::BoltTouch );

	SetThink( &CBullet::BubbleThink );
	SetNextThink( gpGlobals->curtime );//+ 0.01f );
	
	//CreateSprites();

	// Make us glow until we've hit the wall
	//m_nSkin = BOLT_SKIN_GLOW;
}


void CBullet::Precache( void )
{
	PrecacheModel( BOLT_MODEL );

	//BG2 - Tjoppen - Bullet.Hit*
	//PrecacheScriptSound( "Bullet.HitWorld" );
	//PrecacheScriptSound( "Bullet.HitBody" );
	PrecacheScriptSound( "Bullets.DefaultNearmiss" );

	// This is used by C_TEStickyBolt, despte being different from above!!!
	//PrecacheModel( "models/crossbow_bolt.mdl" );

	//PrecacheModel( "sprites/light_glow02_noz.vmt" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBullet::BoltTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr;
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

		float	speed = GetAbsVelocity().Length();
		if( speed < 100 )
		{
			//too slow. die
			UTIL_RemoveImmediate( this );
			return;
		}

		float	dmg;
		if( (GetAbsOrigin() - m_vTrajStart).Length() < m_flConstantDamageRange )
			dmg = (float)m_iDamage;
		else
			dmg = floorf( (float)m_iDamage * speed * speed / (float)(BOLT_AIR_VELOCITY*BOLT_AIR_VELOCITY) );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		//BG2 - Tjoppen - We want musket balls to hit body parts, not just HITGROUP_GENERIC
		UTIL_TraceLine( GetAbsOrigin() - vForward * speed * gpGlobals->frametime, GetAbsOrigin() + vForward * speed * gpGlobals->frametime, MASK_SHOT, GetOwnerEntity(), COLLISION_GROUP_NONE, &tr );
		if( tr.fraction == 1.0 || (tr.hitgroup == HITGROUP_GENERIC && pOther->IsPlayer()) )
		{
            //only hit hull - keep going
			//move forward a bit so we don't get stuck
			SetAbsOrigin( GetAbsOrigin() + vForward * 4.f );
			return;
		}

		UTIL_ImpactTrace( &tr, DMG_BULLET );	//BG2 - Tjoppen - surface blood

		//no force!
		CTakeDamageInfo	dmgInfo( GetOwnerEntity(), GetOwnerEntity(), dmg, DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE | DMG_NEVERGIB );
		dmgInfo.SetDamagePosition( tr.endpos );
		pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			 return;

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
		//BG2 - Tjoppen - no sound..
		//EmitSound( "Bullet.HitBody" );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * 128, MASK_OPAQUE, pOther, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0f )
		{
//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr.m_pEnt == NULL || ( tr.m_pEnt && tr.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr.fraction != 1.0f;
			
				DispatchEffect( "Impact", data );
			}
		}
		
		SetTouch( NULL );
		SetThink( NULL );

		//BG2 - Tjoppen - remove bullets immediately before they have time to mess with the target
		//this doesn't seem to completely solve the problem of players flying when being hit by bullets though
		//UTIL_Remove( this );
		UTIL_RemoveImmediate( this );
		//
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			//BG2 - Tjoppen - Bullet.HitWorld
			//EmitSound( "Bullet.HitWorld" );

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );
			
			// BG2 - BP original was( hitDot < 0.5f ) but a musket ball should not bounce off walls if the angle is too big
			//BG2 - Tjoppen - don't ricochet unless we're hitting a surface at a 60 degree horisontal angle or more
			//					this is a hack so that bullets don't ricochet off the ground
			//if ( ( hitDot < 0.2f ) && ( speed > 100 ) )
			if ( ( hitDot < 0.2f ) && ( speed > 100 ) && tr.plane.normal.z < 0.5f )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;
				
				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );
			}
			else
			{
				SetThink( &CBullet::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );
				
				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );
			
				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize ( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;
			
				DispatchEffect( "Impact", data ); 
				
				UTIL_ImpactTrace( &tr, DMG_BULLET );

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CBullet::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );

				if ( m_pGlowSprite != NULL )
				{
					m_pGlowSprite->TurnOn();
					m_pGlowSprite->FadeAndDie( 3.0f );
				}
			}
			
			//BG2 - BP  TODO: musket balls only create sparks on metal surfaces Shoot some sparks
			/*if ( UTIL_PointContents( GetAbsOrigin() ) == CONTENTS_WATER)
			{
				g_pEffects->Sparks( GetAbsOrigin() );
			}*/
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}

			UTIL_Remove( this );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
//		SetThink( &CBullet::ExplodeThink );
//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBullet::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime );	//BG2 - Tjoppen - think every frame

	if( gpGlobals->curtime > m_flDyingTime )
	{
		UTIL_Remove( this );
		return;
	}

	if ( GetWaterLevel()  == 0 )
	{
		//apply drag
		Vector	vecDir = GetAbsVelocity();
		extern	ConVar	sv_simulatedbullets_drag,
						sv_simulatedbullets_overshoot_range,
						sv_simulatedbullets_overshoot_force,
						sv_gravity;

		float	speed = vecDir.NormalizeInPlace(),
				//drag = 0.0001f;
				drag = sv_simulatedbullets_drag.GetFloat() * m_flRelativeDrag;

		speed -= drag * speed*speed * gpGlobals->frametime;
		if( speed < 1000 )
			speed = 1000;	//clamp

		SetAbsVelocity( vecDir * speed );

		if( sv_simulatedbullets_overshoot_force.GetFloat() > 0 &&
			sv_simulatedbullets_overshoot_range.GetFloat() > 0 )
		{
			//add lift due to bullet rotation which causes overshoot at medium range
			//this extra force declines exponentially and will equal to gravity at time tmax, thus putting the maximum there
			float	t = gpGlobals->curtime - m_flDyingTime + LIFETIME,	//how long we've existed..
					tmax = sv_simulatedbullets_overshoot_range.GetFloat() * 36.f / BOLT_AIR_VELOCITY,	//how long until max?
					F0 = sv_gravity.GetFloat() * sv_simulatedbullets_overshoot_force.GetFloat(),
					//F = F0 * expf( -logf(sv_simulatedbullets_overshoot_force.GetFloat()) * t / tmax );
					F = F0 * powf( sv_simulatedbullets_overshoot_force.GetFloat(), -t / tmax );

			//approx.
			vecDir = GetAbsVelocity();
			vecDir.z += F * gpGlobals->frametime;
			SetAbsVelocity( vecDir );
		}

		//8 units "safety margin" to make sure we passed our victim's head
		float	margin = 8,
				headTolerance = 32,		//how close to the head must the bullet be?
				desiredBacktrace = margin + speed * gpGlobals->frametime * 2;	//*2 due to frametime variations
		if( !m_bHasPlayedNearmiss && (GetAbsOrigin() - m_vTrajStart).LengthSqr() > desiredBacktrace*desiredBacktrace )
		{
			//has gone desiredBacktrace units and not played nearmiss so far. trace backward
			//find any player except the shooter who is within the margin and desiredBacktrace distance when projected
			//onto the ray
			//the ray in this case extends behind the bullet

			//re-normalize because overshoot messed it up
			vecDir.NormalizeInPlace();

			//Msg( "%f\t", vecDir.Length() );
			for( int x = 1; x <= gpGlobals->maxClients; x++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( x );

				//only want connected, alive players
				if( !pPlayer || !pPlayer->IsAlive() || x == GetOwnerEntity()->entindex() )
					continue;

				//distance along ray, behind the bullet
				float d = vecDir.Dot(GetAbsOrigin() - pPlayer->EyePosition());

				//Msg( "%i: %f < %f < %f\t", x, desiredBacktrace, d, margin );

				if( d < margin || d > desiredBacktrace )
					continue;

				//Msg( "go on.. " );

				//shortest distance to eyes from ray must be less than headTolerance
				if( (GetAbsOrigin() - pPlayer->EyePosition()).LengthSqr() - d*d < headTolerance*headTolerance )
				{
					//this player's head is within the desired cylinder.. play the sound and make sure it doesn't
					//play again for this bullet(to avoid HORRIBLE SOUNDS)

					//FIXME: make me play correctly
					//we might have to make some sort of clientside thing, since the server appearently is too stupid
					//to be able to play a sound in a static point in space

					/*const char *name =  "Bullets.DefaultNearmiss";
					const char *name =  "BaseExplosionEffect.Sound";
					CPASAttenuationFilter filter( this, name );
					Vector soundOrigin = GetAbsOrigin() - vecDir * d;

					new CSound();
					//why does EmitSound want a Vector pointer? this seems unsafe. soundOrigin will go missing once
					//its out of scope...
					EmitSound( filter, entindex(), name, &soundOrigin );
					//EmitSound( filter, entindex(), "BaseExplosionEffect.Sound", &soundOrigin );*/
					
					EmitSound( "Bullets.DefaultNearmiss" );
					m_bHasPlayedNearmiss = true;

					//Msg( "nearmiss!\n" );

					break;
				}
			}

			//Msg( "\n" );
		}

		return;
	}

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 5 );
}

#else

IMPLEMENT_CLIENTCLASS_DT( C_Bullet, DT_Bullet, CBullet )
END_RECV_TABLE()


/*CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectCrossbow )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash1" )
CLIENTEFFECT_REGISTER_END()*/

//extern void DrawHalo( IMaterial* pMaterial, const Vector &source, float scale, float const *color );

//
// Crossbow bolt
//


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Bullet::C_Bullet( void )
{
	//SetModel( BOLT_MODEL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_Bullet::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_bUpdated = false;
		m_vecLastOrigin = GetAbsOrigin();
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_Bullet::DrawModel( int flags )
{
	// See if we're drawing the motion blur
	/*if ( flags & STUDIO_TRANSPARENCY )
	{
		float		color[3];
		IMaterial	*pBlurMaterial = materials->FindMaterial( "effects/muzzleflash1", NULL, false );

		Vector	vecDir = GetAbsOrigin() - m_vecLastOrigin;
		float	speed = VectorNormalize( vecDir );
		
		speed = clamp( speed, 0, 32 );
		
		if ( speed > 0 )
		{
			float	stepSize = min( ( speed * 0.5f ), 4.0f );

			Vector	spawnPos = GetAbsOrigin() + ( vecDir * 24.0f );
			Vector	spawnStep = -vecDir * stepSize;

			materials->Bind( pBlurMaterial );

			float	alpha;

			// Draw the motion blurred trail
			for ( int i = 0; i < 20; i++ )
			{
				spawnPos += spawnStep;

				alpha = RemapValClamped( i, 5, 11, 0.25f, 0.05f );

				color[0] = color[1] = color[2] = alpha;

				DrawHalo( pBlurMaterial, spawnPos, 3.0f, color );
			}
		}

		if ( gpGlobals->frametime > 0.0f && !m_bUpdated)
		{
			m_bUpdated = true;
			m_vecLastOrigin = GetAbsOrigin();
		}

		return 1;
	}*/

	// Draw the normal portion
	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Bullet::ClientThink( void )
{
	m_bUpdated = false;
}

#endif