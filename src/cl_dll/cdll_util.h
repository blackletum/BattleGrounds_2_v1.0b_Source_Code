//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( UTIL_H )
#define UTIL_H

#ifdef _WIN32
#pragma once
#endif

#include <soundflags.h>
#include <vector.h>
#include <shareddefs.h>

#include "ispatialpartition.h"
#include "materialsystem/materialsystemutil.h"

class Vector;
class QAngle;
class IMaterial;
class ITexture;
class IClientEntity;
class CHudTexture;
class CGameTrace;
class C_BaseEntity;

struct Ray_t;
struct client_textmessage_t;
typedef CGameTrace trace_t;

namespace vgui
{
	typedef unsigned long HFont;
};


enum ImageFormat;
enum ShakeCommand_t;


void	SetScreenSize( void );
// ScreenHeight returns the height of the screen, in pixels
int		ScreenHeight( void );
// ScreenWidth returns the width of the screen, in pixels
int		ScreenWidth( void );

#define XRES(x)	( x  * ( ( float )ScreenWidth() / 640.0 ) )
#define YRES(y)	( y  * ( ( float )ScreenHeight() / 480.0 ) )

int		UTIL_ComputeStringWidth( vgui::HFont& font, const char *str );
int		UTIL_ComputeStringWidth( vgui::HFont& font, const wchar_t *str );
float	UTIL_AngleDiff( float destAngle, float srcAngle );
void	UTIL_Bubbles( const Vector& mins, const Vector& maxs, int count );
void	UTIL_Smoke( const Vector &origin, const float scale, const float framerate );
void	UTIL_ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName = NULL );
int		UTIL_PrecacheDecal( const char *name, bool preload = false );
void	UTIL_EmitAmbientSound( C_BaseEntity *entity, const Vector &vecOrigin, const char *samp, float vol, soundlevel_t soundlevel, int fFlags, int pitch );
void	UTIL_SetOrigin( C_BaseEntity *entity, const Vector &vecOrigin );
void	UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake=false );
byte	*UTIL_LoadFileForMe( const char *filename, int *pLength );
void	UTIL_FreeFile( byte *buffer );
void	UTIL_MakeSafeName( const char *oldName, char *newName, int newNameBufSize );

// Fade out an entity based on distance fades
unsigned char UTIL_ComputeEntityFade( C_BaseEntity *pEntity, float flMinDist, float flMaxDist, float flFadeScale );

client_textmessage_t	*TextMessageGet( const char *pName );

char	*VarArgs( char *format, ... );
	

// Get the entity the local player is spectating (can be a player or a ragdoll entity).
int GetSpectatorTarget();

// Get the player that the local player is spectating.
int		GetSpectatorTargetPlayer();

int		GetSpectatorMode( void );
bool	IsPlayerIndex( int index );
int		GetLocalPlayerIndex( void );
int		GetLocalPlayerTeam( void );
bool	IsLocalPlayerSpectator( void );
void	NormalizeAngles( QAngle& angles );
void	InterpolateAngles( const QAngle& start, const QAngle& end, QAngle& output, float frac );
void	InterpolateVector( float frac, const Vector& src, const Vector& dest, Vector& output );

const char *nexttoken(char *token, const char *str, char sep);

//-----------------------------------------------------------------------------
// Base light indices to avoid index collision
//-----------------------------------------------------------------------------

enum
{
	LIGHT_INDEX_TE_DYNAMIC = 0x10000000,
	LIGHT_INDEX_PLAYER_BRIGHT = 0x20000000,
	LIGHT_INDEX_MUZZLEFLASH = 0x40000000,
};



// These are accessed by appropriate macros
float	SharedRandomFloat( const char *filename, int line, float flMinVal, float flMaxVal, int additionalSeed = 0 );
int		SharedRandomInt( const char *filename, int line, int iMinVal, int iMaxVal, int additionalSeed = 0 );
Vector	SharedRandomVector( const char *filename, int line, float minVal, float maxVal, int additionalSeed = 0 );
QAngle	SharedRandomAngle( const char *filename, int line, float minVal, float maxVal, int additionalSeed = 0 );

#define SHARED_RANDOMFLOAT_SEED( minval, maxval, seed ) \
	SharedRandomFloat( __FILE__, __LINE__, minval, maxval, seed )
#define SHARED_RANDOMINT_SEED( minval, maxval, seed ) \
	SharedRandomInt( __FILE__, __LINE__, minval, maxval, seed )
#define SHARED_RANDOMVECTOR_SEED( minval, maxval, seed ) \
	SharedRandomVector( __FILE__, __LINE__, minval, maxval, seed )
#define SHARED_RANDOMANGLE_SEED( minval, maxval, seed ) \
	SharedRandomAngle( __FILE__, __LINE__, minval, maxval, seed )

#define SHARED_RANDOMFLOAT( minval, maxval ) \
	SharedRandomFloat( __FILE__, __LINE__, minval, maxval, 0 )
#define SHARED_RANDOMINT( minval, maxval ) \
	SharedRandomInt( __FILE__, __LINE__, minval, maxval, 0 )
#define SHARED_RANDOMVECTOR( minval, maxval ) \
	SharedRandomVector( __FILE__, __LINE__, minval, maxval, 0 )
#define SHARED_RANDOMANGLE( minval, maxval ) \
	SharedRandomAngle( __FILE__, __LINE__, minval, maxval, 0 )

void UTIL_PrecacheOther( const char *szClassname );

void UTIL_SetTrace(trace_t& tr, const Ray_t& ray, C_BaseEntity *edict, float fraction, int hitgroup, unsigned int contents, const Vector& normal, float intercept );

bool GetVectorInScreenSpace( Vector pos, int& iX, int& iY, Vector *vecOffset = NULL );
bool GetTargetInScreenSpace( C_BaseEntity *pTargetEntity, int& iX, int& iY, Vector *vecOffset = NULL );

// prints messages through the HUD (stub in client .dll right now )
class C_BasePlayer;
void ClientPrint( C_BasePlayer *player, int msg_dest, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );

// Pass in an array of pointers and an array size, it fills the array and returns the number inserted
int			UTIL_EntitiesInBox( C_BaseEntity **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask, int partitionMask = PARTITION_CLIENT_NON_STATIC_EDICTS );
int			UTIL_EntitiesInSphere( C_BaseEntity **pList, int listMax, const Vector &center, float radius, int flagMask, int partitionMask = PARTITION_CLIENT_NON_STATIC_EDICTS );

// make this a fixed size so it just sits on the stack
#define MAX_SPHERE_QUERY	256
class CEntitySphereQuery
{
public:
	// currently this builds the list in the constructor
	// UNDONE: make an iterative query of ISpatialPartition so we could
	// make queries like this optimal
	CEntitySphereQuery( const Vector &center, float radius, int flagMask=0, int partitionMask = PARTITION_CLIENT_NON_STATIC_EDICTS );
	C_BaseEntity *GetCurrentEntity();
	inline void NextEntity() { m_listIndex++; }

private:
	int			m_listIndex;
	int			m_listCount;
	C_BaseEntity *m_pList[MAX_SPHERE_QUERY];
};

// creates an entity by name, and ensure it's correctness
// does not spawn the entity
// use the CREATE_ENTITY() macro which wraps this, instead of using it directly
template< class T >
T *_CreateEntity( T *newClass, const char *className )
{
	T *newEnt = dynamic_cast<T*>( CreateEntityByName(className) );
	if ( !newEnt )
	{
		Warning( "classname %s used to create wrong class type\n" );
		Assert(0);
	}

	return newEnt;
}

#define CREATE_ENTITY( newClass, className ) _CreateEntity( (newClass*)NULL, className )
#define CREATE_UNSAVED_ENTITY( newClass, className ) _CreateEntityTemplate( (newClass*)NULL, className )

// Misc useful
inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(stricmp(sz1, sz2) == 0);
}

#endif // !UTIL_H