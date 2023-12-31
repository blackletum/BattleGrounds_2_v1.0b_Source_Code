//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTeam class
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TEAM_H
#define C_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "utlvector.h"
#include "client_thinklist.h"


class C_BasePlayer;

class C_Team : public C_BaseEntity
{
	DECLARE_CLASS( C_Team, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

					C_Team();
	virtual			~C_Team();

	virtual void	PreDataUpdate( DataUpdateType_t updateType );

	// Data Handling
	virtual char	*Get_Name( void );
	virtual int		Get_Score( void );
	virtual int		Get_Deaths( void );
	virtual int		Get_Ping( void );

	// Player Handling
	virtual int		Get_Number_Players( void );
	//BG2 - Tjoppen - part of bandwidth saving
	/*virtual bool	ContainsPlayer( int iPlayerIndex );
	C_BasePlayer*	GetPlayer( int idx );*/

	int		GetTeamNumber();

	//BG2 - Tjoppen - part of bandwidth saving
	//void	RemoveAllPlayers();


// IClientThinkable overrides.
public:

	virtual	void				ClientThink();


public:

	// Data received from the server
	//BG2 - Tjoppen - part of bandwidth saving
	//CUtlVector< int > m_aPlayers;
	char	m_szTeamname[ MAX_TEAM_NAME_LENGTH ];
	int		m_iScore;

	// Data for the scoreboard
	int		m_iDeaths;
	int		m_iPing;
	int		m_iPacketloss;
	int		m_iTeamNum;

	//BG2 - Tjoppen - stuff in C_Team
	int GetNumInfantry();
	int GetNumOfficers();
	int GetNumSnipers();
	int GetNumOfClass( int iClass );
	//
};


// Global list of client side team entities
extern CUtlVector< C_Team * > g_Teams;

// Global team handling functions
C_Team *GetLocalTeam( void );
C_Team *GetGlobalTeam( int iTeamNumber );
C_Team *GetPlayersTeam( int iPlayerIndex );
C_Team *GetPlayersTeam( C_BasePlayer *pPlayer );
bool ArePlayersOnSameTeam( int iPlayerIndex1, int iPlayerIndex2 );

#endif // C_TEAM_H
