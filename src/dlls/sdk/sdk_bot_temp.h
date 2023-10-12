//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SDK_BOT_TEMP_H
#define SDK_BOT_TEMP_H
#ifdef _WIN32
#pragma once
#endif

//BG2 - Tjoppen - HACKHACK
#define CSDKPlayer CBasePlayer

static void RunPlayerMove( CSDKPlayer *fakeclient, CUserCmd &cmd, float frametime );

// This is our bot class.
class CSDKBot // : public CSDKPlayer
{
public:
	bool			m_bBackwards;

	float			m_flNextTurnTime;
	bool			m_bLastTurnToRight;

	float			m_flNextStrafeTime;
	float			m_flSideMove,
					m_flForwardMove;

	QAngle			m_ForwardAngle;
	QAngle			m_LastAngles;

	bool			m_bInuse;
	CSDKPlayer		*m_pPlayer;

	//BG2 - Tjoppen
	int				reload,
					attack,
					attack2,
					respawn;
	float			m_flNextThink;

	CUserCmd		m_LastCmd;

	CSDKBot()
	{
		m_bInuse = false;
		m_pPlayer = NULL;

		m_flSideMove = m_flForwardMove = 0;
		m_flNextThink = 0;
		reload = attack = attack2 = respawn = 0;
	}
};

extern int		g_CurBotNumber;
extern CSDKBot	gBots[MAX_PLAYERS];

// If iTeam or iClass is -1, then a team or class is randomly chosen.
CBasePlayer *BotPutInServer( bool bFrozen, int iTeam, int iClass );

void Bot_RunAll();


#endif // SDK_BOT_TEMP_H
