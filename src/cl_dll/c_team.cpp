//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTeam class
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_team.h"
//BG2 - Tjoppen - #includes
#include "hl2mp_gamerules.h"
#include "c_hl2mp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Team's player UtlVector to entindexes
//-----------------------------------------------------------------------------
//BG2 - Tjoppen - don't need this
/*void RecvProxy_PlayerList(  const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Team *pTeam = (C_Team*)pOut;
	pTeam->m_aPlayers[pData->m_iElement] = pData->m_Value.m_Int;
}


void RecvProxyArrayLength_PlayerArray( void *pStruct, int objectID, int currentArrayLength )
{
	C_Team *pTeam = (C_Team*)pStruct;
	
	if ( pTeam->m_aPlayers.Size() != currentArrayLength )
		pTeam->m_aPlayers.SetSize( currentArrayLength );
}*/


IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_Team, DT_Team, CTeam)
	RecvPropInt( RECVINFO(m_iTeamNum)),
	RecvPropInt( RECVINFO(m_iScore)),
	RecvPropString( RECVINFO(m_szTeamname)),
	
	//BG2 - Tjoppen - don't need this
	/*RecvPropArray2( 
		RecvProxyArrayLength_PlayerArray,
		RecvPropInt( "player_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerList ), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		)*/
END_RECV_TABLE()

// Global list of client side team entities
CUtlVector< C_Team * > g_Teams;

//=================================================================================================
// C_Team functionality

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Team::C_Team()
{
	m_iScore = 0;
	memset( m_szTeamname, 0, sizeof(m_szTeamname) );

	m_iDeaths = 0;
	m_iPing = 0;
	m_iPacketloss = 0;

	// Add myself to the global list of team entities
	g_Teams.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Team::~C_Team()
{
	g_Teams.FindAndRemove( this );
}

//BG2 - Tjoppen - part of bandwidth saving
/*void C_Team::RemoveAllPlayers()
{
	m_aPlayers.RemoveAll();
}*/

void C_Team::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
}


//-----------------------------------------------------------------------------
// Gets the ith player on the team (may return NULL) 
//-----------------------------------------------------------------------------
//BG2 - Tjoppen - part of bandwidth saving
/*C_BasePlayer* C_Team::GetPlayer( int idx )
{
	return (C_BasePlayer*)cl_entitylist->GetEnt(m_aPlayers[idx]);
}*/


int C_Team::GetTeamNumber()
{
	return m_iTeamNum;
}


//=================================================================================================
// TEAM HANDLING
//=================================================================================================
// Purpose: 
//-----------------------------------------------------------------------------
char *C_Team::Get_Name( void )
{
	return m_szTeamname;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_Team::Get_Score( void )
{
	return m_iScore;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_Team::Get_Deaths( void )
{
	return m_iDeaths;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_Team::Get_Ping( void )
{
	return m_iPing;
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of players in this team
//-----------------------------------------------------------------------------
int C_Team::Get_Number_Players( void )
{
	//BG2 - Tjoppen - part of bandwidth saving
	int n = 0;
	for( int x = 1; x <= gpGlobals->maxClients; x++ )
	{
		C_BasePlayer *pPlayer = static_cast<C_BasePlayer*>( cl_entitylist->GetEnt(x) );

		if( pPlayer && pPlayer->GetTeamNumber() == GetTeamNumber() )
			n++;
	}

	return n;
	//return m_aPlayers.Size();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the specified player is on this team
//-----------------------------------------------------------------------------
//BG2 - Tjoppen - part of bandwidth saving
/*bool C_Team::ContainsPlayer( int iPlayerIndex )
{
	for (int i = 0; i < m_aPlayers.Size(); i++ )
	{
		if ( m_aPlayers[i] == iPlayerIndex )
			return true;
	}

	return false;
}*/


void C_Team::ClientThink()
{
}


//=================================================================================================
// GLOBAL CLIENT TEAM HANDLING
//=================================================================================================
// Purpose: Get the C_Team for the local player
//-----------------------------------------------------------------------------
C_Team *GetLocalTeam( void )
{
	if ( !engine )
		return NULL;
	
	return GetPlayersTeam( engine->GetLocalPlayer() );
}

//-----------------------------------------------------------------------------
// Purpose: Get the C_Team for the specified team number
//-----------------------------------------------------------------------------
C_Team *GetGlobalTeam( int iTeamNumber )
{
	for (int i = 0; i < g_Teams.Count(); i++ )
	{
		if ( g_Teams[i]->GetTeamNumber() == iTeamNumber )
			return g_Teams[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of teams you can access via GetGlobalTeam() (hence the +1)
//-----------------------------------------------------------------------------
int GetNumTeams()
{
	return g_Teams.Count() + 1; 
}

//-----------------------------------------------------------------------------
// Purpose: Get the team of the specified player
//-----------------------------------------------------------------------------
C_Team *GetPlayersTeam( int iPlayerIndex )
{
	//BG2 - Tjoppen - part of bandwidth saving
	C_BasePlayer *pPlayer = static_cast<C_BasePlayer*>( cl_entitylist->GetEnt(iPlayerIndex) );

	if( !pPlayer )
		return NULL;

	for (int i = 0; i < g_Teams.Count(); i++ )
	{
		//if ( g_Teams[i]->ContainsPlayer( iPlayerIndex ) )
		if( g_Teams[i]->GetTeamNumber() == pPlayer->GetTeamNumber() )
			return g_Teams[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the team of the specified player
//-----------------------------------------------------------------------------
C_Team *GetPlayersTeam( C_BasePlayer *pPlayer )
{
	return GetPlayersTeam( pPlayer->entindex() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the two specified players are on the same team
//-----------------------------------------------------------------------------
bool ArePlayersOnSameTeam( int iPlayerIndex1, int iPlayerIndex2 )
{
	//BG2 - Tjoppen - part of bandwidth saving
	C_BasePlayer	*pPlayer1 = static_cast<C_BasePlayer*>( cl_entitylist->GetEnt(iPlayerIndex1) ),
					*pPlayer2 = static_cast<C_BasePlayer*>( cl_entitylist->GetEnt(iPlayerIndex2) );

	return pPlayer1 && pPlayer2 && pPlayer1->GetTeamNumber() == pPlayer2->GetTeamNumber();

	/*for (int i = 0; i < g_Teams.Count(); i++ )
	{
		if ( g_Teams[i]->ContainsPlayer( iPlayerIndex1 ) && g_Teams[i]->ContainsPlayer( iPlayerIndex2 ) )
			return true;
	}

	return false;*/
}

//BG2 - Tjoppen - stuff in C_Team
int C_Team::GetNumInfantry()
{
	return GetNumOfClass( CLASS_INFANTRY );
}

int C_Team::GetNumOfficers()
{
	return GetNumOfClass( CLASS_OFFICER );
}

int C_Team::GetNumSnipers()
{
	return GetNumOfClass( CLASS_SNIPER );
}

int C_Team::GetNumOfClass( int iClass )
{
	int iAmount = 0;
	
	for( int x = 1; x <= gpGlobals->maxClients; x++ )
	{
		CHL2MP_Player *pHL2Player = ToHL2MPPlayer( cl_entitylist->GetEnt(x) );

		if( pHL2Player && pHL2Player->GetTeamNumber() == GetTeamNumber() && pHL2Player->GetClass() == iClass )
			iAmount++;
	}

	return iAmount;
}
//