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

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "sdk_player.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"
#include "team.h"
#include "hl2mp_gamerules.h"
#include "sdk_bot_temp.h"
#include "hl2mp_player.h"

class CSDKBot;
void Bot_Think( CSDKBot *pBot );


ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "bot_forceattackon", "0", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "bot_flipout", "1", 0, "When on, all bots fire their guns." );
ConVar bot_changeclass( "bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
static ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "0", 0, "Offsets the bot yaw." );

ConVar bot_sendcmd( "bot_sendcmd", "", 0, "Forces bots to send the specified command." );

ConVar bot_crouch( "bot_crouch", "0", 0, "Bot crouches" );

//BG2 - Tjoppen
ConVar bot_aim( "bot_aim", "1", 0, "Bot aims on enemies" );

int g_CurBotNumber = 1;

//LINK_ENTITY_TO_CLASS( sdk_bot, CSDKBot );

CSDKBot	gBots[MAX_PLAYERS];

/*class CBotManager
{
public:
	static CBasePlayer* ClientPutInServerOverride_Bot( edict_t *pEdict, const char *playername )
	{
		// This tells it which edict to use rather than creating a new one.
		CBasePlayer::s_PlayerEdict = pEdict;

		CSDKBot *pPlayer = static_cast<CSDKBot *>( CreateEntityByName( "sdk_bot" ) );
		if ( pPlayer )
		{
			char trimmedName[MAX_PLAYER_NAME_LENGTH];
			Q_strncpy( trimmedName, playername, sizeof( trimmedName ) );
			pPlayer->PlayerData()->netname = AllocPooledString( trimmedName );
		}

		return pPlayer;
	}
};*/


//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer( bool bFrozen )
{
	char botname[ 64 ];
	Q_snprintf( botname, sizeof( botname ), "Bot%02i", g_CurBotNumber );

	// This trick lets us create a CSDKBot for this client instead of the CSDKPlayer
	// that we would normally get when ClientPutInServer is called.
	//ClientPutInServerOverride( &CBotManager::ClientPutInServerOverride_Bot );
	edict_t *pEdict = engine->CreateFakeClient( botname );
	//ClientPutInServerOverride( NULL );

	if (!pEdict)
	{
		Msg( "Failed to create Bot(%s).\n", botname );
		return NULL;
	}

	// Allocate a player entity for the bot, and call spawn
	//CSDKBot *pPlayer = ((CSDKBot*)CBaseEntity::Instance( pEdict ));
	CSDKPlayer *pPlayer = ((CSDKPlayer *)CBaseEntity::Instance( pEdict ));

	if( !pPlayer )
	{
		Msg( "Couldn't cast bot to player\n" );
		return NULL;
	}

	gBots[pPlayer->GetClientIndex()].m_bInuse = true;
	gBots[pPlayer->GetClientIndex()].m_pPlayer = pPlayer;
	//BG2 - Tjoppen - start thinking immediately
	gBots[pPlayer->GetClientIndex()].m_flNextThink = gpGlobals->curtime;
	gBots[pPlayer->GetClientIndex()].reload = 0;
	gBots[pPlayer->GetClientIndex()].attack = 0;
	gBots[pPlayer->GetClientIndex()].attack2 = 0;
	gBots[pPlayer->GetClientIndex()].respawn = 0;

	/*if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );*/

	//this doesn't work anymore.. for some reason. maybe it never worked?
	/*static int lastclass = 0;
	switch( lastclass++ )
	{
	case 0:
		engine->ClientCommand( pEdict, "light_a" );
		break;
	case 1:
		engine->ClientCommand( pEdict, "medium_a" );
		break;
	case 2:
		engine->ClientCommand( pEdict, "heavy_a" );
		break;
	case 3:
		engine->ClientCommand( pEdict, "light_b" );
		break;
	case 4:
		engine->ClientCommand( pEdict, "medium_b" );
		break;
	case 5:
		engine->ClientCommand( pEdict, "heavy_b" );
		break;
	}*/

	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

    //pPlayer->ChangeTeam( TEAM_UNASSIGNED );
	//pPlayer->RemoveAllItems( true );
	//pPlayer->Spawn();

	//new team/class picking code. it works.
	static int last = 0, last2 = 0;

	int tmp = last2;
	((CHL2MP_Player*)pPlayer)->SetNextClass( last2 );
	last2 = (last2 + 1) % 3;

	pPlayer->ChangeTeam( TEAM_AMERICANS + last );
	last = !last;

	//HACKHACKHACK
	const char	*szNewModelName = ((CHL2MP_Player*)pPlayer)->PlayermodelTeamClass( pPlayer->GetTeamNumber(), tmp );

	//if( Q_strncmp( szOldModelName, szNewModelName, 256 ) )
	{
		//((CHL2MP_Player*)pPlayer)->m_flNextModelChangeTime = gpGlobals->curtime - 1;

		char cmd[512];
		Q_snprintf( cmd, sizeof (cmd), "cl_playermodel %s\n", szNewModelName );
		engine->ClientCommand ( pPlayer->edict(), cmd );
	}
	//

	pPlayer->Spawn();

	g_CurBotNumber++;

	return pPlayer;
}

// Handler for the "bot" command.
void BotAdd_f()
{
	/*extern int FindEngineArgInt( const char *pName, int defaultVal );
	extern const char* FindEngineArg( const char *pName );

	// Look at -count.
	int count = FindEngineArgInt( "-count", 1 );
	count = clamp( count, 1, 16 );

	// Look at -frozen.
	bool bFrozen = !!FindEngineArg( "-frozen" );*/

	/*int count = 0;
	if( cc_Bot.GetInt() > 0 )
	{
		count = cc_Bot.GetInt();
		cc_Bot.SetValue( 0 );
	}*/

	/*int count = 1;
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{*/
		BotPutInServer( false );//bFrozen );
	//}
}

ConCommand cc_Bot( "bot_add", BotAdd_f, "Add a bot", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = /*ToSDKPlayer(*/ UTIL_PlayerByIndex( i );// );

		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) )
		{
			CSDKBot *pBot = &gBots[pPlayer->GetClientIndex()];//dynamic_cast< CSDKBot* >( pPlayer );
			if( pBot && pBot->m_bInuse )
				Bot_Think( pBot );
			/*else
			{
				CUserCmd cmd;
				Q_memset( &cmd, 0, sizeof( cmd ) );

				if( !RandomInt( 0, 10 ) )
					cmd.buttons |= IN_ATTACK;

				if( !RandomInt( 0, 100 ) )
					cmd.buttons |= IN_RELOAD;

				cmd.forwardmove	= RandomFloat( -1000, 1000 );
				cmd.sidemove	= RandomFloat( -1000, 1000 );
				cmd.mousedx		= RandomFloat( -100, 100 );
				cmd.mousedy		= RandomFloat( -100, 100 );

				RunPlayerMove( pPlayer, cmd, gpGlobals->frametime );

				/*if( !pPlayer->IsAlive() )
				{
					Msg( "bot dead - respawning\n" );
					pPlayer->Respawn();
				}*//*
			}*/
		}
	}
}

CBasePlayer *FindClosestEnemy( CSDKBot *pBot, bool insight, float *pdist )
{
	//Msg( "FindClosestEnemy(insight=%s)\n", insight ? "true" : "false" );
	if( !pBot->m_pPlayer->IsAlive() )
		return NULL;

	int team = pBot->m_pPlayer->GetTeam()->GetTeamNumber();

	if( team < TEAM_AMERICANS && team < TEAM_BRITISH )
		return NULL;	//spectator or unassigned

	int otherteam = team == TEAM_AMERICANS ? TEAM_BRITISH : TEAM_AMERICANS;

	CBasePlayer *pClosest = NULL;
	float		mindist = 3000.0f;

	CTeam *pTeam = g_Teams[otherteam];
	for( int x = 0; x < pTeam->GetNumPlayers(); x++ )
	{
		CBasePlayer *pEnemy = pTeam->GetPlayer(x);
		if( !pEnemy->IsAlive() )
			continue;

		float dist = (pEnemy->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin()).Length();
		if( dist < mindist )
		{
			if( insight )
			{
				//check to make sure enemy is in sight
				trace_t tr;	
				UTIL_TraceLine( pBot->m_pPlayer->GetLocalOrigin() + Vector(0,0,36), 
								pEnemy->GetLocalOrigin() + Vector(0,0,36),
								MASK_SOLID, pBot->m_pPlayer, COLLISION_GROUP_DEBRIS_TRIGGER, &tr );

				if( tr.DidHitWorld() )
					continue;
			}

			mindist = dist;
			pClosest = pEnemy;
		}
	}

	/*Msg( "%s closest enemy is ", pBot->m_pPlayer->PlayerData()->netname );
	if( pClosest )
		Msg( "%s\n", pClosest->PlayerData()->netname );
	else
		Msg( "(null)\n" );*/

	if( pdist )
		*pdist = mindist;

	return pClosest;
}

CBasePlayer *FindClosestFriend( CSDKBot *pBot, bool insight, float *pdist )
{
	//Msg( "FindClosestEnemy(insight=%s)\n", insight ? "true" : "false" );
	if( !pBot->m_pPlayer->IsAlive() )
		return NULL;

	int team = pBot->m_pPlayer->GetTeam()->GetTeamNumber();

	if( team < TEAM_AMERICANS && team < TEAM_BRITISH )
		return NULL;	//spectator or unassigned

	CBasePlayer *pClosest = NULL;
	float		mindist = 3000.0f;

	CTeam *pTeam = g_Teams[team];
	for( int x = 0; x < pTeam->GetNumPlayers(); x++ )
	{
		CBasePlayer *pEnemy = pTeam->GetPlayer(x);
		if( !pEnemy->IsAlive() )
			continue;

		float dist = (pEnemy->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin()).Length();
		if( dist < mindist )
		{
			if( insight )
			{
				//check to make sure enemy is in sight
				trace_t tr;	
				UTIL_TraceLine( pBot->m_pPlayer->GetLocalOrigin() + Vector(0,0,36), 
								pEnemy->GetLocalOrigin() + Vector(0,0,36),
								MASK_SOLID, pBot->m_pPlayer, COLLISION_GROUP_NONE, &tr );

				if( tr.DidHitWorld() )
					continue;
			}

			mindist = dist;
			pClosest = pEnemy;
		}
	}

	/*Msg( "%s closest enemy is ", pBot->m_pPlayer->PlayerData()->netname );
	if( pClosest )
		Msg( "%s\n", pClosest->PlayerData()->netname );
	else
		Msg( "(null)\n" );*/

	if( pdist )
		*pdist = mindist;

	return pClosest;
}

CBaseEntity *FindClosestFlag( CSDKBot *pBot, bool insight, float *pdist )
{
	//Msg( "FindClosestEnemy(insight=%s)\n", insight ? "true" : "false" );
	if( !pBot->m_pPlayer->IsAlive() )
		return NULL;

	int team = pBot->m_pPlayer->GetTeam()->GetTeamNumber();

	if( team < TEAM_AMERICANS && team < TEAM_BRITISH )
		return NULL;	//spectator or unassigned

	CBaseEntity *pClosest = NULL;
	float		mindist = 1000000000.0f;

	//CTeam *pTeam = g_Teams[team];
	CBaseEntity *pEntity = NULL;
	while( (pEntity = gEntList.FindEntityByClassname( pEntity, "flag" )) != NULL )
	{
		if (pEntity->GetTeam()->GetTeamNumber() != team)
		{
			float dist = (pEntity->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin()).Length();
			mindist = dist;
			pClosest = pEntity;
		}
		else
		{
			continue;
		}
	}

	//for( int x = 0; x < pTeam->GetNumPlayers(); x++ )
	//{
		//CBasePlayer *pEnemy = pTeam->GetPlayer(x);
	//	if( !pEnemy->IsAlive() )
		//	continue;

		/*float dist = (pEnemy->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin()).Length();
		if( dist < mindist )
		{
			if( insight )
			{
				//check to make sure enemy is in sight
				trace_t tr;	
				UTIL_TraceLine( pBot->m_pPlayer->GetLocalOrigin() + Vector(0,0,36), 
								pEnemy->GetLocalOrigin() + Vector(0,0,36),
								MASK_SOLID, pBot->m_pPlayer, COLLISION_GROUP_NONE, &tr );

				if( tr.DidHitWorld() )
					continue;
			}

			mindist = dist;
			pClosest = pEnemy;
		}*/
	//}

	/*Msg( "%s closest enemy is ", pBot->m_pPlayer->PlayerData()->netname );
	if( pClosest )
		Msg( "%s\n", pClosest->PlayerData()->netname );
	else
		Msg( "(null)\n" );*/

	if( pdist )
		*pdist = mindist;

	return pClosest;
}

bool Bot_RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;

	
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt()  );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

	if( bot_crouch.GetInt() )
		cmd.buttons |= IN_DUCK;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			m_flSideMove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
static void RunPlayerMove( CSDKPlayer *fakeclient, CUserCmd &cmd, float frametime )
{
	if ( !fakeclient )
		return;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}



void Bot_UpdateStrafing( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( gpGlobals->curtime >= pBot->m_flNextStrafeTime )
	{
		pBot->m_flNextStrafeTime = gpGlobals->curtime + RandomFloat( 0.7f, 1.5f );

		if ( RandomInt( 0, 5 ) == 0 )
		{
			pBot->m_flSideMove = -600.0f + 1200.0f * RandomFloat( 0, 2 );
		}
		else
		{
			pBot->m_flSideMove = 0;
		}
		cmd.sidemove = pBot->m_flSideMove;

		if ( RandomInt( 0, 20 ) == 0 )
		{
			pBot->m_bBackwards = true;
		}
		else
		{
			pBot->m_bBackwards = false;
		}
	}
}


void Bot_UpdateDirection( CSDKBot *pBot )
{
	//BG2 - Tjoppen
	if( bot_aim.GetBool() )
	{
		float dist;
		CBasePlayer *pEnemy = FindClosestEnemy( pBot, true, &dist );
		CBaseEntity *pFlag = FindClosestFlag( pBot, true, &dist );
		if( pEnemy )
		{
			QAngle angles;
			//Vector forward = pEnemy->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin();
			//aim for the head if you can find it...
			Vector forward = pEnemy->Weapon_ShootPosition() - pBot->m_pPlayer->Weapon_ShootPosition();
			forward.z -= 8;	//slightly below
			VectorAngles( forward, angles );
			pBot->m_pPlayer->SetLocalAngles( angles );
			return;
		}
		else if( pFlag )
		{
			QAngle angles;
			//Vector forward = pEnemy->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin();
			//aim for the head if you can find it...
			Vector forward = pFlag->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin();
			forward.z -= 8;	//slightly below
			VectorAngles( forward, angles );
			pBot->m_pPlayer->SetLocalAngles( angles );
			return;
		}
	}

	float angledelta = 15.0;
	QAngle angle;

	int maxtries = (int)360.0/angledelta;

	if ( pBot->m_bLastTurnToRight )
	{
		angledelta = -angledelta;
	}

	angle = pBot->m_pPlayer->GetLocalAngles();

	trace_t trace;
	Vector vecSrc, vecEnd, forward;
	while ( --maxtries >= 0 )
	{
		AngleVectors( angle, &forward );

		vecSrc = pBot->m_pPlayer->GetLocalOrigin() + Vector( 0, 0, 36 );

		vecEnd = vecSrc + forward * 10;

		UTIL_TraceHull( vecSrc, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, 
			MASK_PLAYERSOLID, pBot->m_pPlayer, COLLISION_GROUP_NONE, &trace );

		if ( trace.fraction == 1.0 )
		{
			if ( gpGlobals->curtime < pBot->m_flNextTurnTime )
			{
				break;
			}
		}

		angle.y += angledelta;

		if ( angle.y > 180 )
			angle.y -= 360;
		else if ( angle.y < -180 )
			angle.y += 360;

		pBot->m_flNextTurnTime = gpGlobals->curtime + 2.0;
		pBot->m_bLastTurnToRight = RandomInt( 0, 1 ) == 0 ? true : false;

		pBot->m_ForwardAngle = angle;
		pBot->m_LastAngles = angle;
	}
	
	pBot->m_pPlayer->SetLocalAngles( angle );
}


void Bot_FlipOut( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( bot_flipout.GetInt() > 0 && pBot->m_pPlayer->IsAlive() )
	{
		if ( bot_forceattackon.GetBool() || pBot->attack++ >= 20 )//(RandomFloat(0.0,1.0) > 0.5) )
		{
			//cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
			cmd.buttons |= IN_ATTACK;
			pBot->attack = 0;
		}

		if( pBot->attack2++ >= 13 )
		{
			cmd.buttons |= IN_ATTACK2;
			pBot->attack2 = 0;
		}

		/*if( RandomFloat( 0, 1 ) > 0.9 )
			cmd.buttons |= IN_RELOAD;*/
		if( pBot->reload++ >= 40 )
		{
			cmd.buttons |= IN_RELOAD;
			pBot->reload = 0;
		}

		if ( bot_flipout.GetInt() >= 2 )
		{
			QAngle angOffset = RandomAngle( -1, 1 );

			pBot->m_LastAngles += angOffset;

			for ( int i = 0 ; i < 2; i++ )
			{
				if ( fabs( pBot->m_LastAngles[ i ] - pBot->m_ForwardAngle[ i ] ) > 15.0f )
				{
					if ( pBot->m_LastAngles[ i ] > pBot->m_ForwardAngle[ i ] )
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] + 15;
					}
					else
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] - 15;
					}
				}
			}

			pBot->m_LastAngles[ 2 ] = 0;

			pBot->m_pPlayer->SetLocalAngles( pBot->m_LastAngles );
		}
	}
}


void Bot_HandleSendCmd( CSDKBot *pBot )
{
	if ( strlen( bot_sendcmd.GetString() ) > 0 )
	{
		//send the cmd from this bot
		pBot->m_pPlayer->ClientCommand( bot_sendcmd.GetString() );

		bot_sendcmd.SetValue("");
	}
}


// If bots are being forced to fire a weapon, see if I have it
void Bot_ForceFireWeapon( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( bot_forcefireweapon.GetString() )
	{
		CBaseCombatWeapon *pWeapon = pBot->m_pPlayer->Weapon_OwnsThisType( bot_forcefireweapon.GetString() );
		if ( pWeapon )
		{
			// Switch to it if we don't have it out
			CBaseCombatWeapon *pActiveWeapon = pBot->m_pPlayer->GetActiveWeapon();

			// Switch?
			if ( pActiveWeapon != pWeapon )
			{
				pBot->m_pPlayer->Weapon_Switch( pWeapon );
			}
			else
			{
				// Start firing
				// Some weapons require releases, so randomise firing
				if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
				{
					cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
				}
			}
		}
	}
}


void Bot_SetForwardMovement( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( !pBot->m_pPlayer->IsEFlagSet(EFL_BOT_FROZEN) )
	{
		if ( 1 )// pBot->m_pPlayer->m_iHealth == 100 )
		{
			cmd.forwardmove = 600 * ( pBot->m_bBackwards ? -1 : 1 );
			if ( pBot->m_flSideMove != 0.0f )
			{
				cmd.forwardmove *= RandomFloat( 0.1, 1.0f );
			}
		}
		else
		{
			// Stop when shot
			cmd.forwardmove = 0;
		}
	}
}


void Bot_HandleRespawn( CSDKBot *pBot, CUserCmd &cmd )
{
	// Wait for Reinforcement wave
	if ( !pBot->m_pPlayer->IsAlive() )
	{
		// Try hitting my buttons occasionally
		//if ( RandomInt( 0, 100 ) > 80 )
		if( pBot->respawn++ >= 30 )
		{
			pBot->respawn = 0;
			// Respawn the bot
			/*if ( RandomInt( 0, 1 ) == 0 )
			{*/
				cmd.buttons |= IN_JUMP;
			/*}
			else
			{
				cmd.buttons = 0;
			}*/
		}
	}
}


//-----------------------------------------------------------------------------
// Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
int lastclass = 0;
void Bot_Think( CSDKBot *pBot )
{
	if( !pBot )
		return;
	if( !pBot->m_bInuse || !pBot->m_pPlayer )
		return;

	// Make sure we stay being a bot
	pBot->m_pPlayer->AddFlag( FL_FAKECLIENT );

	CUserCmd cmd;
	Q_memset( &cmd, 0, sizeof( cmd ) );

	if( pBot->m_flNextThink > gpGlobals->curtime )
	{
		cmd = pBot->m_LastCmd;
		cmd.viewangles = pBot->m_pPlayer->GetLocalAngles();
		cmd.upmove = 0;
		cmd.impulse = 0;

		float frametime = gpGlobals->frametime;
		RunPlayerMove( pBot->m_pPlayer, cmd, frametime );
		return;
	}

	pBot->m_flNextThink = gpGlobals->curtime + 0.1f;

	/*FindClosestEnemy( pBot, false );
	FindClosestEnemy( pBot, true );*/

	/*if( pBot->m_flNextThink < gpGlobals->curtime )
	{
		if( !pBot->m_pPlayer->IsAlive() )
		{
			Bot_HandleRespawn( pBot, cmd );
			float frametime = gpGlobals->frametime;
			RunPlayerMove( pBot->m_pPlayer, cmd, frametime );
			return;
		}

		float dist, fdist;
		CBasePlayer *pEnemy = FindClosestEnemy( pBot, true, &dist ),
				*pFriend = FindClosestFriend( pBot, true, &fdist );

		//bool hasammo = pBot->m_pPlayer->GetActiveWeapon() && pBot->m_pPlayer->GetActiveWeapon()->m_iClip1 > 0;

		if( pEnemy )
		{
			QAngle angles;
			Vector forward = pEnemy->GetLocalOrigin() - pBot->m_pPlayer->GetLocalOrigin();
			VectorAngles( forward, angles );
			pBot->m_pPlayer->SetLocalAngles( angles );

			pBot->m_flSideMove = RandomFloat( -100, 100 );
			pBot->m_flForwardMove = RandomFloat( -75, 200 );

			if( RandomInt( 0, 100 ) > 50 )
				cmd.buttons |= IN_ATTACK2;

			if( dist < 512 && RandomInt( 0, 100 ) > 50 )
				cmd.buttons |= IN_ATTACK;

			if( dist > 768 || RandomInt( 0, 100 ) > 70 )
				cmd.buttons |= IN_RELOAD;				//damn reloaders!
		}
		else if( pFriend )
		{
			//no enemies around - reload
			if( RandomInt( 0, 100 ) > 50 )
				cmd.buttons |= IN_RELOAD;

			//face same way as friend now and then
			if( RandomInt( 0, 100 ) > 70 )
				pBot->m_pPlayer->SetLocalAngles( pFriend->GetLocalAngles() );

			if( RandomInt( 0, 100 ) > 70 )
			{
				pBot->m_LastAngles = pBot->m_pPlayer->GetLocalAngles();
				pBot->m_LastAngles.y += RandomInt( -90, 90 );
			}

			pBot->m_flSideMove = RandomFloat( -50, 50 );
			pBot->m_flForwardMove = RandomFloat( -25, 200 );
		}
		else	//all alone..
		{
			//walk randomly
			if( RandomInt( 0, 100 ) > 70 )
			{
				pBot->m_LastAngles = pBot->m_pPlayer->GetLocalAngles();
				pBot->m_LastAngles.y += RandomInt( -90, 90 );
			}

			pBot->m_flSideMove = RandomFloat( -50, 50 );
			pBot->m_flForwardMove = RandomFloat( -25, 200 );
		}

		pBot->m_flNextThink = gpGlobals->curtime + RandomFloat( 0.2, 0.5 );
	}
/*s	else	//fall back on default behaviour
	{*/
	// Finally, override all this stuff if the bot is being forced to mimic a player.
		if ( !Bot_RunMimicCommand( cmd ) )
		{
			cmd.sidemove = pBot->m_flSideMove;

			if ( pBot->m_pPlayer->IsAlive() && (pBot->m_pPlayer->GetSolid() == SOLID_BBOX) )
			{
				Bot_SetForwardMovement( pBot, cmd );

				// Only turn if I haven't been hurt
				if ( 1 )//!pBot->m_pPlayer->IsEFlagSet(EFL_BOT_FROZEN) && pBot->m_pPlayer->m_iHealth == 100 )
				{
					Bot_UpdateDirection( pBot );
					Bot_UpdateStrafing( pBot, cmd );
				}

				// Handle console settings.
				Bot_ForceFireWeapon( pBot, cmd );
				Bot_HandleSendCmd( pBot );
			}
			else
			{
				Bot_HandleRespawn( pBot, cmd );
			}

			Bot_FlipOut( pBot, cmd );

			// Fix up the m_fEffects flags
			pBot->m_pPlayer->PostClientMessagesSent();

			
			
			cmd.viewangles = pBot->m_pPlayer->GetLocalAngles();
			cmd.upmove = 0;
			cmd.impulse = 0;
		}
	//}*/

	/*cmd.forwardmove = pBot->m_flForwardMove;
	cmd.sidemove	= pBot->m_flSideMove;

	pBot->m_pPlayer->PostClientMessagesSent();*/

	//BG2 - Tjoppen - longer between RunPlayerMove...
	/*cmd.forwardmove *= 10;
	cmd.sidemove *= 10;*/

	cmd.viewangles = pBot->m_pPlayer->GetLocalAngles();
	cmd.upmove = 0;
	cmd.impulse = 0;

	pBot->m_LastCmd = cmd;

	float frametime = /*pBot->m_flNextThink - gpGlobals->curtime;//*/gpGlobals->frametime;
	RunPlayerMove( pBot->m_pPlayer, cmd, frametime );
}


