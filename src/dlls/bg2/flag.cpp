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

#include "cbase.h"
#include "triggers.h"
#include "flag.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "gamerules.h"
#include "team.h"
#include "engine/IEngineSound.h"
#include "bg2/spawnpoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#ifndef CLIENT_DLL
//BG2 - Tjoppen - just make these global so we can reset them
float	flNextClientPrintAll = 0;
bool	bNextClientPrintAllForce = false;

void ClientPrintAll( char *str, bool printfordeadplayers, bool forcenextclientprintall )
{
	if( !str )
		return;

	//Msg( "ClientPrintAll: %s printfordeadplayers=%i forcenextclientprintall=%i\t", str, printfordeadplayers, forcenextclientprintall );

	if( flNextClientPrintAll > gpGlobals->curtime && (!printfordeadplayers || bNextClientPrintAllForce) && !forcenextclientprintall )
	{
		//Msg( "returned\n" );
		return;
	}

	flNextClientPrintAll = gpGlobals->curtime + 1.0f;
	bNextClientPrintAllForce = forcenextclientprintall;

	CBasePlayer *pPlayer = NULL;

	while( (pPlayer = dynamic_cast<CBasePlayer*>(gEntList.FindEntityByClassname( pPlayer, "player" ))) != NULL )
	{
		if( !pPlayer->IsAlive() && !printfordeadplayers )	//this doesn't concern dead players
			continue;

		ClientPrint( pPlayer, HUD_PRINTCENTER, str );
	}

	//Msg( "done\n" );
}
//#endif


/*===========================================================================================



			T	H	E			B	A	S	E			P	O	I	N	T

A brush that tells us who is in each of the key points of a map, handles logic and calls 
efffects via outputs.
===========================================================================================*/

//=========================================================
//CBasePoint's Spawn
//The usual entity spawn, initilisation.
//=========================================================
void CBasePoint::Spawn( void )
{
	//BaseClass::Spawn();
    // We should collide with physics
    SetSolid( SOLID_NONE );
    // We push things out of our way
    SetMoveType( MOVETYPE_NONE );
    // Use our brushmodel
    SetModel( STRING( GetModelName() ) );
	//m_hPlayers.Purge();
	//m_iTeam = TEAM_UNASSIGNED;
	//m_iCaptured = 0;
	m_flNextCaptureTime = 0;
	return;
}


//=========================================================
//CBasePoint's Think
//Where the real magic is! Checks for all touchers, acts on
//Them. Holds touchers in an array and checks on them every
//Think and acts on what it finds
//=========================================================
void CBasePoint::Think()
{
	Msg("Thinking %i players %i min %f is the time for capture %f is current time %i is the team involved\n", m_iPlayers, m_iMinPlayers, m_flNextCaptureTime, gpGlobals->curtime, m_iTeam);
	CTeam *pAmericans = g_Teams[TEAM_AMERICANS];
	CTeam *pBritish = g_Teams[TEAM_BRITISH];
	//First of all we must check if we were not in the middle of a capture last Think.
	if (m_hPlayers.Count() > 0)
	{
		Msg("Thinking %i players %i min %f is the time for capture %f is current time %i is the team involved\n", m_iPlayers, m_iMinPlayers, m_flNextCaptureTime, gpGlobals->curtime, m_iTeam);
		//Go through all the players we currently have, check for deaths and exits
		int i = 0;
		while ( i < m_hPlayers.Count())
		{
			//Check if the fellow hasn't left the game
			if (m_hPlayers[i])
			{
				if (!m_hPlayers[i]->IsAlive())
				{
					//The player died, so alert everyone.
					IGameEvent * event = gameeventmanager->CreateEvent( "point_player_death" );
					if ( event )
					{
						event->SetString( "name", STRING(m_hPlayers[i]->pl.netname) );
						gameeventmanager->FireEvent( event );
					}
					m_hPlayers.FindAndRemove(m_hPlayers[i]);
					i--;
				}
				else if (!IsTouching(m_hPlayers[i]))
				{
					//The player left, so alert everyone.
					IGameEvent * event = gameeventmanager->CreateEvent( "point_player_left" );
					if ( event )
					{
						event->SetString( "name", STRING(m_hPlayers[i]->pl.netname) );
						gameeventmanager->FireEvent( event );
					}
					m_hPlayers.FindAndRemove(m_hPlayers[i]);
					i--;
				}
			}
			else
			{
				//The player disconnected, so alert everyone.
				IGameEvent * event = gameeventmanager->CreateEvent( "point_player_disconnected" );
				if ( event )
				{
					gameeventmanager->FireEvent( event );
				}
			}

			//Now we go through all the players on the team and check for touchers who aren't
			//on the list or dead.
			int j = 0;
			while (j < g_Teams[GetTeam()]->GetNumPlayers())
			{
				if (g_Teams[GetTeam()]->GetPlayer(j))
				{
					if (IsTouching(g_Teams[GetTeam()]->GetPlayer(j)))
					{
						//if the player is alive and not in the list already
						if ((g_Teams[GetTeam()]->GetPlayer(j)->IsAlive()) && (m_hPlayers.Find(g_Teams[GetTeam()]->GetPlayer(j)) > 0 ))
						{
							m_hPlayers.AddToTail(g_Teams[GetTeam()]->GetPlayer(j));
							//The player entered, so alert everyone.
							char msg[512];
							//Q_snprintf( msg, 512, "%s is going to help secure %s(%i/%i) for The %s", STRING(pAmericans->GetPlayer(i)->pl.netname), "Test Point", GetPlayers(), GetMinPlayers(), ? "Americans":"British");
							ClientPrintAll( msg, true, true );
							//IGameEvent * event = gameeventmanager->CreateEvent( "point_player_entered" );
							//if ( event )
							//{
							//	event->SetString( "name", STRING(g_Teams[GetTeam()]->GetPlayer(j)->pl.netname) );
							//	gameeventmanager->FireEvent( event );
							//}
						}
					}
				}
				//check the next unknown player
				j++;
			}
			//check the next known player
			i++;
		}
		//update amount of players for HUD
		m_iPlayers = m_hPlayers.Count();
		if (m_iPlayers < m_iMinPlayers)
		{
			m_iCaptured = 0;
			//Uncaptured
			char msg[512];
			Q_snprintf( msg, 512, "%s has been abandoned!(%i/%i)", "Test Point", GetPlayers(), GetMinPlayers());
			ClientPrintAll( msg, true, true );
			//IGameEvent * event = gameeventmanager->CreateEvent( "point_player_entered" );
			//if ( event )
			//{
			//	event->SetString( "name", STRING(g_Teams[GetTeam()]->GetPlayer(j)->pl.netname) );
			//	gameeventmanager->FireEvent( event );
			//}
		}
		else//lets see if we are done
		{
			if (m_flNextCaptureTime < gpGlobals->curtime)
			{
				m_iCaptured = 1;
				m_iCapturing = 0;
				char msg[512];
				switch (m_iTeam)
				{
					case TEAM_AMERICANS:
						pAmericans->AddScore(20);
						Q_snprintf( msg, 512, "%i players have secured %s for The Americans!", GetPlayers(), "Test Point");
						ClientPrintAll( msg, true, true );
						break;
					case TEAM_BRITISH:
						//When Britain first at Heav'n's command
						//Arose from out the azure main;
						//This was the charter of the land,
						//And guardian angels sang this strain;
						//Rule, Britannia! Britannia, rule the waves:
						//Britons never will be slaves.
						pBritish->AddScore(20);
						Q_snprintf( msg, 512, "%i players have secured %s for The British!", GetPlayers(), "Test Point");
						ClientPrintAll( msg, true, true );
						break;
				}
			}
		}
	}
	else//We are not capturing, check for an aggressor
	{
		//Americans...
		int i = 0;
		while (i < pAmericans->GetNumPlayers())
		{
			if (pAmericans->GetPlayer(i))
			{
				if /*(*/(IsTouching(pAmericans->GetPlayer(i)))/* && (pAmericans->GetPlayer(i)->IsAlive()))*/
				{
					//First we check if this is the first player we are finding this frame. 
					//The enemy might of wiped out the previous owners and have muliple 
					//players inside
					if (m_hPlayers.Count() > 0)
					{
						m_hPlayers.AddToTail(pAmericans->GetPlayer(i));
						//The player entered, so alert everyone.
						char msg[512];
						Q_snprintf( msg, 512, "%s is going to help secure %s(%i/%i)", STRING(pAmericans->GetPlayer(i)->pl.netname), "Test Point", GetPlayers(), GetMinPlayers());
						ClientPrintAll( msg, true, true );
						//IGameEvent * event = gameeventmanager->CreateEvent( "point_player_entered" );
						//if ( event )
						//{
						//	event->SetInt( "team", TEAM_AMERICANS);
						//	event->SetString( "name", STRING(pAmericans->GetPlayer(i)->pl.netname) );
						//	gameeventmanager->FireEvent( event );
						//}
					}
					else
					{
						m_hPlayers.AddToTail(pAmericans->GetPlayer(i));
						//The team entered, so alert everyone.
						char msg[512];
						Q_snprintf( msg, 512, "%s has begun securing %s for the Americans(%i/%i)", STRING(pAmericans->GetPlayer(i)->pl.netname), "Test Point", GetPlayers(), GetMinPlayers());
						ClientPrintAll( msg, true, true );
						//IGameEvent * event = gameeventmanager->CreateEvent( "point_team_entered" );
						//if ( event )
						//{
						//	event->SetInt( "team", TEAM_AMERICANS);
						//	event->SetString( "name", STRING(pAmericans->GetPlayer(i)->pl.netname) );
						//	gameeventmanager->FireEvent( event );
						//}
					}
				}
				//continue on
				i++;
			}
		}
		//British...
		i = 0;
		while (i < pBritish->GetNumPlayers())
		{
			if (pBritish->GetPlayer(i))
			{
				if /*(*/(IsTouching(pBritish->GetPlayer(i)))/* && (pBritish->GetPlayer(i)->IsAlive()))*/
				{
					//First we check if this is the first player we are finding this frame. 
					//The enemy might of wiped out the previous owners and have muliple 
					//players inside
					if (m_hPlayers.Count() > 0)
					{
						m_hPlayers.AddToTail(pBritish->GetPlayer(i));
						//The player entered, so alert everyone.
						char msg[512];
						Q_snprintf( msg, 512, "%s is going to help secure %s(%i/%i)", STRING(pAmericans->GetPlayer(i)->pl.netname), "Test Point", GetPlayers(), GetMinPlayers());
						ClientPrintAll( msg, true, true );
						//IGameEvent * event = gameeventmanager->CreateEvent( "point_player_entered" );
						//if ( event )
						//{
						//	event->SetInt( "team", TEAM_BRITISH);
						//	event->SetString( "name", STRING(pBritish->GetPlayer(i)->pl.netname) );
						//	gameeventmanager->FireEvent( event );
						//}
					}
					else
					{
						m_hPlayers.AddToTail(pBritish->GetPlayer(i));
						//The team entered, so alert everyone.
						char msg[512];
						Q_snprintf( msg, 512, "%s has begun securing %s for the Americans(%i/%i)", STRING(pAmericans->GetPlayer(i)->pl.netname), "Test Point", GetPlayers(), GetMinPlayers());
						ClientPrintAll( msg, true, true );
						//IGameEvent * event = gameeventmanager->CreateEvent( "point_team_entered" );
						//if ( event )
						//{
						//	event->SetInt( "team", TEAM_BRITISH);
						//	event->SetString( "name", STRING(pBritish->GetPlayer(i)->pl.netname) );
						//	gameeventmanager->FireEvent( event );
						//}
					}
				}
				//continue on
				i++;
			}
		}
	}
	m_iPlayers = m_hPlayers.Count();
	return;
}

//=========================================================
//CBasePoint's GetTeam
//Accessor function for m_iTeam
//=========================================================
int	CBasePoint::GetTeam()
{
	return m_iTeam;
}

//=========================================================
//CBasePoint's GetCaptureTime
//Accessor function for m_flCaptureTime
//=========================================================
float CBasePoint::GetCaptureTime()
{
	return m_flCaptureTime;
}

//=========================================================
//CBasePoint's GetNextCaptureTime
//Accessor function for m_flNextCaptureTime
//=========================================================
float CBasePoint::GetNextCaptureTime()
{
	return m_flNextCaptureTime;
}

//=========================================================
//CBasePoint's GetTricklePoints
//Accessor function for m_iTricklePoints
//=========================================================
int	CBasePoint::GetTricklePoints()
{
	return m_iTricklePoints;
}

//=========================================================
//CBasePoint's GetTeamCapturePoints
//Accessor function for m_iTeamCapturePoints
//=========================================================
int	CBasePoint::GetTeamCapturePoints()
{
	return m_iTeamCapturePoints;
}

//=========================================================
//CBasePoint's GetPlayerCapturePoints
//Accessor function for m_iPlayerCapturePoints
//=========================================================
int	CBasePoint::GetPlayerCapturePoints()
{
	return m_iPlayerCapturePoints;
}

//=========================================================
//CBasePoint's GetAssaultTeam
//Accessor function for m_iAssaultTeam
//=========================================================
int	CBasePoint::GetAssaultTeam()
{
	return m_iAssaultTeam;
}

//=========================================================
//CBasePoint's GetMinPlayers
//Accessor function for m_iMinPlayers
//=========================================================
int	CBasePoint::GetMinPlayers()
{
	return m_iMinPlayers;
}

//=========================================================
//CBasePoint's GetPlayers
//Accessor function for m_iPlayers
//=========================================================
int	CBasePoint::GetPlayers()
{
	return m_iPlayers;
}

//=========================================================
//CBasePoint's GetUncapturable
//Accessor function for m_iUncapturable
//=========================================================
int	CBasePoint::GetUncapturable()
{
	return m_iUncapturable;
}

//IMPLEMENT_NETWORKCLASS_ALIASED( Flag, DT_Flag )

//BEGIN_NETWORK_TABLE( CFlag, DT_Flag )
//	SendPropInt( SENDINFO( m_iLastTeam ), Q_log2(NUM_TEAMS), SPROP_UNSIGNED ),
//	SendPropInt( SENDINFO( m_iRequestingCappers ), Q_log2(NUM_TEAMS), SPROP_UNSIGNED ),
//	SendPropFloat( SENDINFO( m_flNextCapture ) ),
//	SendPropInt( SENDINFO( m_iCapturePlayers ), Q_log2(MAX_PLAYERS), SPROP_UNSIGNED ),
//	SendPropInt( SENDINFO( m_iForTeam ), 2, SPROP_UNSIGNED ),
//	SendPropFloat( SENDINFO( m_flCaptureTime ) ),
//	SendPropStringT( SENDINFO( m_sFlagName ) ),
//	SendPropInt( SENDINFO( m_iHUDSlot ), 4, SPROP_UNSIGNED ),	//15 slots.. 0 = sequential tile
//	SendPropBool( SENDINFO( m_bActive ) ),
//END_NETWORK_TABLE()

BEGIN_DATADESC( CBasePoint )

	DEFINE_KEYFIELD( m_iMinPlayers, FIELD_INTEGER, "CapturePlayers" ),
	//DEFINE_KEYFIELD( m_flCaptureRadius, FIELD_FLOAT, "CaptureRadius" ),
	DEFINE_KEYFIELD( m_flCaptureTime, FIELD_FLOAT, "CaptureTime" ),
	DEFINE_KEYFIELD( m_iUncapturable, FIELD_INTEGER, "Uncap" ),
	DEFINE_KEYFIELD( m_iTeamCapturePoints, FIELD_INTEGER, "TeamBonus" ),
	//DEFINE_KEYFIELD( m_iTeamBonusInterval, FIELD_INTEGER, "TeamBonusInterval" ),
	DEFINE_KEYFIELD( m_iPlayerCapturePoints, FIELD_INTEGER, "PlayerBonus" ),
	DEFINE_KEYFIELD( m_iAssaultTeam, FIELD_INTEGER, "AssaultTeam" ),
	DEFINE_KEYFIELD( m_sFlagName, FIELD_STRING, "FlagName" ),
	DEFINE_KEYFIELD( m_iHUDSlot, FIELD_INTEGER, "HUDSlot" ),

	//BG2 - SaintGreg - dynamic flags
	//DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	//DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	//DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	DEFINE_OUTPUT( m_OnAmericanStartCapture, "OnAmericanStartCapture" ),
	DEFINE_OUTPUT( m_OnBritishStartCapture, "OnBritishStartCapture" ),
	DEFINE_OUTPUT( m_OnStartCapture, "OnStartCapture" ),
	DEFINE_OUTPUT( m_OnAmericanCapture, "OnAmericanCapture" ),
	DEFINE_OUTPUT( m_OnBritishCapture, "OnBritishCapture" ),
	DEFINE_OUTPUT( m_OnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_OnAmericanStopCapture, "OnAmericanStopCapture" ),
	DEFINE_OUTPUT( m_OnBritishStopCapture, "OnBritishStopCapture" ),
	DEFINE_OUTPUT( m_OnStopCapture, "OnStopCapture" ),
	DEFINE_OUTPUT( m_OnAmericanLosePoint, "OnAmericanLosePoint" ),
	DEFINE_OUTPUT( m_OnBritishLosePoint, "OnBritishLosePoint" ),
	DEFINE_OUTPUT( m_OnLosePoint, "OnLosePoint" ),
	DEFINE_OUTPUT( m_OnEnable, "OnEnable" ),
	DEFINE_OUTPUT( m_OnDisable, "OnDisable" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS(capturepoint, CBasePoint);

/*===========================================================================================


			T	H	E			P	O	I	N	T			F	L	A	G

A model that is used to display the status of the point and show players where it is.
===========================================================================================*/

//BG2 - SaintGreg - dynamic flag settings
//input functions
bool CFlag::IsActive( void )
{
	return m_bActive;
}

void CFlag::InputEnable( inputdata_t &inputData )
{
	if( m_bActive )
		return;

	//char msg[512];
	//Q_snprintf( msg, 512, "flag(%s) has been enabled", STRING( GetEntityName() ) );
	//ClientPrintAll( msg, true, true ); // saying whether it is enabled or not is important, so force

	m_bActive = true;
	ChangeTeam( TEAM_UNASSIGNED );
	m_iLastTeam = TEAM_UNASSIGNED;
	SetModel( GetNeutralModelName() );
	m_OnEnable.FireOutput( inputData.pActivator, this );
	Think(); // think immediately and restart the thinking cycle
}
void CFlag::InputDisable( inputdata_t &inputData )
{
	if( !m_bActive )
		return;

	//char msg[512];
	//Q_snprintf( msg, 512, "flag(%s) has been disabled", STRING( GetEntityName() ) );
	//ClientPrintAll( msg, true, true ); // saying whether it is enabled or not is important, so force

	m_bActive = false;
	if( GetTeamNumber() == TEAM_AMERICANS )
	{
		m_OnAmericanLosePoint.FireOutput( this, this );
		m_OnLosePoint.FireOutput( this, this );
	}
	else if( GetTeamNumber() == TEAM_BRITISH )
	{
		m_OnBritishLosePoint.FireOutput( this, this );
		m_OnLosePoint.FireOutput( this, this );
	}
	ChangeTeam( TEAM_UNASSIGNED );
	m_iLastTeam = TEAM_UNASSIGNED;
	SetModel( GetDisabledModelName() );
	m_OnDisable.FireOutput( inputData.pActivator, this );
	//CFlagHandler::Update();
}
void CFlag::InputToggle( inputdata_t &inputData )
{
	if (m_bActive)
		InputDisable( inputData );
	else
		InputEnable( inputData );
}

void CFlag::Spawn( void )
{
	Precache( );

	if (HasSpawnFlags( CFlag_START_DISABLED ))
	{
		m_bActive = false;
		SetModel( GetDisabledModelName() );
	}
	else
	{
		m_bActive = true;
		ChangeTeam( TEAM_UNASSIGNED );	//ChangeTeam handles everything..
	}

	m_iRequestingCappers = TEAM_UNASSIGNED;
	m_iLastTeam = TEAM_UNASSIGNED;

	/*int m_nIdealSequence = SelectWeightedSequence( ACT_VM_IDLE );
	//SetActivity( ACT_VM_IDLE );
	SetSequence( m_nIdealSequence );	
	//SendViewModelAnim( m_nIdealSequence );
	//SetIdealActivity( ACT_VM_IDLE );
	//SetTouch(&CFlag::MyTouch);*/
	//SetSequence(0);
	
	BaseClass::Spawn( );

	SetSequence( SelectWeightedSequence( ACT_VM_IDLE ) );

	SetThink( &CFlag::Think );
	SetNextThink( gpGlobals->curtime );
}
void CFlag::Precache( void )
{
	PrecacheModel( GetNeutralModelName() );
	PrecacheModel( GetAmericanModelName() );
	PrecacheModel( GetBritishModelName() );
	PrecacheModel( GetDisabledModelName() );  // BG2 - SaintGreg - flag when disabled

	PrecacheScriptSound( "British.win" );
	PrecacheScriptSound( "Americans.win" );
	PrecacheScriptSound( "Flag.capture" );
}

void CFlag::Think( void )
{
	/*
	1.0F style flags

	when not capped, they behave just like the old flags
	- wait for m_iCapturePlayers(of only one team?) to stand near it for m_flCaptureTime seconds
	- after said time, flag is captured by that team
	- a held flag will periodically award the team with some amount of points

	the difference now is:
	- when all the people who captured the flag die, it's uncapped(tweak this to something other than zero players?)
	- more than m_iCapturePlayers players can hold the flag by touching it after it has been captured(overloading)
	- any enemy walking into a capture zone without a friendly player inside guarding the flag, will uncap the flag

	*/

	SetNextThink( gpGlobals->curtime + 0.5f );

	if (!m_bActive)	// if inactive, stop the thinking cycle
		return;

	//award any time bonii
	switch( GetTeamNumber() )
	{
		case TEAM_AMERICANS:
			if (m_iTeamBonusInterval != 0)
			{
				if ((m_flNextTeamBonus <= gpGlobals->curtime) && (m_iForTeam != 1))
				{
					g_Teams[TEAM_AMERICANS]->AddScore( m_iTeamBonus );
					m_flNextTeamBonus += m_iTeamBonusInterval;
				}
			}
			break;
		case TEAM_BRITISH:
			if (m_iTeamBonusInterval != 0)
			{
				if ((m_flNextTeamBonus <= gpGlobals->curtime) && (m_iForTeam != 2))
				{
					g_Teams[TEAM_BRITISH]->AddScore( m_iTeamBonus );
					m_flNextTeamBonus += m_iTeamBonusInterval;
				}
			}
			break;
		default:
			switch (m_iForTeam)
			{
				case 1://amer
					if (m_iTeamBonusInterval != 0)
					{
						if (m_flNextTeamBonus <= gpGlobals->curtime)
						{
							g_Teams[TEAM_BRITISH]->AddScore( m_iTeamBonus );
							m_flNextTeamBonus += m_iTeamBonusInterval;
						}
					}
					break;
				case 2://brit
					if (m_iTeamBonusInterval != 0)
					{
						if (m_flNextTeamBonus <= gpGlobals->curtime)
						{
							g_Teams[TEAM_AMERICANS]->AddScore( m_iTeamBonus );
							m_flNextTeamBonus += m_iTeamBonusInterval;
						}
					}
					break;
			}
	}

	//safeguard against the possibility that m_flNextTeamBonus is somehow out of bounds after above check
	//could happen if the flag stays uncapped for a long time. if so, the team that caps it would get a lot of "stored up"
	//points.
	if (m_flNextTeamBonus <= gpGlobals->curtime)
		m_flNextTeamBonus = gpGlobals->curtime + m_iTeamBonusInterval;

	if( GetTeamNumber() != TEAM_UNASSIGNED )
		ThinkCapped();	//don't need to check for overloading or uncapping on uncapped flags

	ThinkUncapped();	//always run this, so non-uncappable flags can be taken
}

//this is actually the old think funtion, slightly modified
void CFlag::ThinkUncapped( void )
{
	CBasePlayer *pPlayer = NULL;

	int americans = 0,
		british = 0;

	while( (pPlayer = dynamic_cast<CBasePlayer*>(gEntList.FindEntityByClassnameWithin( pPlayer, "player", GetLocalOrigin(), m_flCaptureRadius ))) != NULL )
	{
		if( !pPlayer->IsAlive() )	//dead players don't cap
			continue;

		switch( pPlayer->GetTeamNumber() )
		{
		case TEAM_AMERICANS:
			if ((m_iForTeam == 1) || (m_iForTeam == 0))
			{
				americans++;
			}
			break;
		case TEAM_BRITISH:
			if ((m_iForTeam == 2) || (m_iForTeam == 0))
			{
				british++;
			}
			break;
		default:
			break;
		}
	}

	//char *msg = NULL;

	//default number in flag indicator
	m_iNearbyPlayers = m_vOverloadingPlayers.Count();

	if( americans + british > 0 && (americans <= 0 || british <= 0) )
	{
		//Msg( "\namericans = %i  british = %i\n", americans, british );

		//only americans or british at the flag
		//if we don't already own it, and we've been here for at least three seconds - capture

		if( americans > 0 && GetTeamNumber() != TEAM_AMERICANS )
		{
			m_iRequestingCappers = TEAM_AMERICANS;
			m_iNearbyPlayers = americans;

			if (americans >= min( m_iCapturePlayers, g_Teams[TEAM_AMERICANS]->GetNumPlayers() ) )
			{
				//Msg( "americans\n" );
				if( m_iLastTeam != TEAM_AMERICANS )
				{
					//char msg2[512];
					//Msg( "americans are capturing a flag(\"%s\")\n", STRING( m_sFlagName.Get() ) );
					//Q_snprintf( msg2, 512, "The americans are capturing a flag(%s)", STRING( m_sFlagName.Get() ) );
					//msg = msg2;
					m_iLastTeam = TEAM_AMERICANS;
					m_flNextCapture = gpGlobals->curtime + m_flCaptureTime;

					m_OnAmericanStartCapture.FireOutput( this, this );
					m_OnStartCapture.FireOutput( this, this );
				}
				else if( gpGlobals->curtime >= m_flNextCapture )
				{
					//char msg2[512];
					//CFlagHandler::PlayCaptureSound();
					//Q_snprintf( msg2, 512, "The americans captured a flag(%s)", STRING( m_sFlagName.Get() ) );
					//msg = msg2;

					Capture( TEAM_AMERICANS );
				}
			}
		}
		else if( british > 0 && GetTeamNumber() != TEAM_BRITISH )
		{
			m_iRequestingCappers = TEAM_BRITISH;
			m_iNearbyPlayers = british;

			if (british >= min( m_iCapturePlayers, g_Teams[TEAM_BRITISH]->GetNumPlayers() ) )
			{
				//Msg( "british\n" );
				if( m_iLastTeam != TEAM_BRITISH )
				{
					//char msg2[512];
					//Msg( "british are capturing a flag(\"%s\")\n", STRING( m_sFlagName.Get() ) );
					//Q_snprintf( msg2, 512, "The british are capturing a flag(%s)", STRING( m_sFlagName.Get() ) );
					//msg = msg2;
					m_iLastTeam = TEAM_BRITISH;
					m_flNextCapture = gpGlobals->curtime + m_flCaptureTime;

					m_OnBritishStartCapture.FireOutput( this, this );
					m_OnStartCapture.FireOutput( this, this );
				}
				else if( gpGlobals->curtime >= m_flNextCapture )
				{
					//char msg2[512];
					//CFlagHandler::PlayCaptureSound();
					//Q_snprintf( msg2, 512, "The british captured a flag(%s)", STRING( m_sFlagName.Get() ) );
					//msg = msg2;

					Capture( TEAM_BRITISH );
				}
			}
		}
	}
	else
	{
		if( m_iLastTeam != TEAM_UNASSIGNED && GetTeamNumber() != m_iLastTeam )
		{
			//Msg( "stopped capturing a flag\n" );
			if( m_iLastTeam == TEAM_AMERICANS )
			{
				//char msg2[512];
				//Q_snprintf( msg2, 512, "The americans stopped capturing a flag(%s)", STRING( m_sFlagName.Get() ) );
				//msg = msg2;

				m_OnAmericanStopCapture.FireOutput( this, this );
				m_OnStopCapture.FireOutput( this, this );
			}
			else if( m_iLastTeam == TEAM_BRITISH )
			{
				//char msg2[512];
				//Q_snprintf( msg2, 512, "The british stopped capturing a flag(%s)", STRING( m_sFlagName.Get() ) );
				//msg = msg2;

				m_OnBritishStopCapture.FireOutput( this, this );
				m_OnStopCapture.FireOutput( this, this );
			}
		}
		//noone here
		m_iLastTeam = TEAM_UNASSIGNED;
		m_iRequestingCappers = TEAM_UNASSIGNED;
		//m_iNearbyPlayers = 0;
		
		m_flNextCapture = 0;
	}

	//ClientPrintAll( msg );
}

void CFlag::Capture( int iTeam )
{
	//iTeam is either americans or british
	//this function handles a lot of the capture related stuff

	EmitSound( "Flag.capture" );
	g_Teams[iTeam]->AddScore( m_iTeamBonus );
	m_flNextTeamBonus = (gpGlobals->curtime + m_iTeamBonusInterval);

	//award capping players some points and put them on the overload list
	m_vOverloadingPlayers.RemoveAll();

	CHL2MP_Player *pPlayer = NULL;
	while( (pPlayer = dynamic_cast<CHL2MP_Player*>(gEntList.FindEntityByClassnameWithin( pPlayer, "player", GetLocalOrigin(), m_flCaptureRadius ))) != NULL )
	{
		if( !pPlayer->IsAlive() )	//dead players don't cap
			continue;

		if( pPlayer->GetTeamNumber() == iTeam )
		{
			pPlayer->IncrementFragCount(m_iPlayerBonus);
			//BG2 - Tjoppen - rewards put on hold
			//pPlayer->IncreaseReward(1);
			m_vOverloadingPlayers.AddToHead( pPlayer );
		}
	}
	m_iLastTeam = TEAM_UNASSIGNED;
	m_iRequestingCappers = TEAM_UNASSIGNED;
	m_iNearbyPlayers = m_vOverloadingPlayers.Count();

	// before we change team, if they stole the point, fire the output
	if (GetTeamNumber() != iTeam)
	{
		//fire appropriate output
		if( GetTeamNumber() == TEAM_AMERICANS )
			m_OnAmericanLosePoint.FireOutput( this, this );
		else if( GetTeamNumber() == TEAM_BRITISH )
			m_OnBritishLosePoint.FireOutput( this, this );

		m_OnLosePoint.FireOutput( this, this );
	}

	ChangeTeam( iTeam );
	//CFlagHandler::Update();

	if( iTeam == TEAM_AMERICANS )
		m_OnAmericanCapture.FireOutput( this, this );
	else if( iTeam == TEAM_BRITISH )
		m_OnBritishCapture.FireOutput( this, this );

	m_OnCapture.FireOutput( this, this );
}

void CFlag::ThinkCapped( void )
{
	//check if anyone's overloading or uncapping this flag
	//start by counting people near the flag

	int friendlies = 0,
		enemies = 0;

	CBasePlayer *pPlayer = NULL;

	while( (pPlayer = dynamic_cast<CBasePlayer*>(gEntList.FindEntityByClassnameWithin( pPlayer, "player", GetLocalOrigin(), m_flCaptureRadius ))) != NULL )
	{
		if( !pPlayer->IsAlive() )	//dead players don't cap
			continue;

		//BG2 - Tjoppen - TODO: m_iForTeam troubles?
		if( pPlayer->GetTeamNumber() == GetTeamNumber() )
		{
			friendlies++;
			if( m_vOverloadingPlayers.Find( pPlayer ) == -1 )
			{
				//friendly player that's not on the list. add
				//BG2 - Tjoppen - TODO: add some sort of bonus for overloading?
				m_vOverloadingPlayers.AddToHead( pPlayer );
			}
		}
		else
			enemies++;
	}

	//if someone steals the flag, or we run out of holders - uncap
	//don't uncap flags that aren't non-uncappable
	if( !m_bNotUncappable && (enemies > 0 && friendlies <= 0 || (m_bUncapOnDeath && m_vOverloadingPlayers.Count() <= 0)) )
	{
		//uncap
		ChangeTeam( TEAM_UNASSIGNED );
		m_iNearbyPlayers = 0;
		m_vOverloadingPlayers.RemoveAll();
	}
	else
	{
		//we're safe
		m_iNearbyPlayers = m_vOverloadingPlayers.Count();
	}
}

void CFlag::ChangeTeam( int iTeamNum )
{
	switch( iTeamNum )
	{
	case TEAM_AMERICANS:
		SetModel( GetAmericanModelName() );
		break;
	case TEAM_BRITISH:
		SetModel( GetBritishModelName() );
		break;
	default:
		switch( m_iForTeam )
		{
			case 1://amer
				SetModel( GetBritishModelName() );
				break;
			case 2://brit
				SetModel( GetAmericanModelName() );
				break;
			default:
				SetModel( GetNeutralModelName() );
				break;
		}
		break;
	}

	//m_iLastTeam = iTeamNum;
	//m_iLastTeam = TEAM_UNASSIGNED;

	BaseClass::ChangeTeam( iTeamNum );
}

int CFlag::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

IMPLEMENT_NETWORKCLASS_ALIASED( Flag, DT_Flag )

int SendProxyArrayLength_IsOverloading( const void *pStruct, int objectID )
{
	//BG2 - Tjoppen - TODO: only send as many bits as there are players connected?
	return gpGlobals->maxClients;
}

void SendProxy_IsOverloading_Bit( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	//each bit corresponds to player #id overloading current flag or not
	CFlag *pFlag = (CFlag*)pStruct;
	
	if( pFlag )
	{
		//see if iElement has corresponding client id overloading pFlag
		for( int x = 0; x < pFlag->m_vOverloadingPlayers.Count(); x++ )
			if( pFlag->m_vOverloadingPlayers[x]->GetClientIndex() == iElement )
			{
				pOut->m_Int = 1;
				return;
			}

		pOut->m_Int = 0;
	}
	else
	{
		pOut->m_Int = 0;
	}
}

BEGIN_NETWORK_TABLE( CFlag, DT_Flag )
	SendPropInt( SENDINFO( m_iLastTeam ), Q_log2(NUM_TEAMS), SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iRequestingCappers ), Q_log2(NUM_TEAMS), SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flNextCapture ) ),
	SendPropInt( SENDINFO( m_iCapturePlayers ), Q_log2(MAX_PLAYERS), SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iNearbyPlayers ), Q_log2(MAX_PLAYERS), SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iForTeam ), 2, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flCaptureTime ) ),
	SendPropStringT( SENDINFO( m_sFlagName ) ),
	SendPropInt( SENDINFO( m_iHUDSlot ), 5 ),	//15 slots.. 0 = sequential tile, -1 = hidden(don't draw)
	SendPropBool( SENDINFO( m_bActive ) ),
	SendPropBool( SENDINFO( m_bNotUncappable ) ),
	
	//each bit corresponds to player #id overloading current flag or not
	//BG2 - Tjoppen - TODO: is there a way to set a specific bit depending on who the recipient is?
	//						At the moment bandwidth usage for this is O(N²) instead of O(N) for N clients.
	//						I'd like to send to each client only one bit indicating if that client is
	//						overloading the current flag or not, instead of sending a bitmask of all client's
	//						overload state to all clients.. The current method reveals too much information
	//						for any would-be cheater on the opposing team. Or perhaps it's just good because
	//						we can figure out the names of the people overloading the flag and print them.
	//						This depends on gameplay stuff - do we want/need everyone knowing which flags
	//						everyone else is overloading? Perhaps.
	//BG2 - Tjoppen - TODO: implement this on client, with drawing in hud etc.
	SendPropArray2( 
		SendProxyArrayLength_IsOverloading,
		SendPropInt("IsOverloading_Bit", 0, SIZEOF_IGNORE, 1, SPROP_UNSIGNED, SendProxy_IsOverloading_Bit),
		MAX_PLAYERS, 
		0,
		"IsOverloading"
		),

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CFlag )
END_PREDICTION_DATA()

BEGIN_DATADESC( CFlag )

	DEFINE_KEYFIELD( m_iCapturePlayers, FIELD_INTEGER, "CapturePlayers" ),
	DEFINE_KEYFIELD( m_flCaptureRadius, FIELD_FLOAT, "CaptureRadius" ),
	DEFINE_KEYFIELD( m_flCaptureTime, FIELD_FLOAT, "CaptureTime" ),
	DEFINE_KEYFIELD( m_iUncap, FIELD_INTEGER, "Uncap" ),
	DEFINE_KEYFIELD( m_iTeamBonus, FIELD_INTEGER, "TeamBonus" ),
	DEFINE_KEYFIELD( m_iTeamBonusInterval, FIELD_INTEGER, "TeamBonusInterval" ),
	DEFINE_KEYFIELD( m_iPlayerBonus, FIELD_INTEGER, "PlayerBonus" ),
	DEFINE_KEYFIELD( m_iForTeam, FIELD_INTEGER, "ForTeam" ),
	DEFINE_KEYFIELD( m_sFlagName, FIELD_STRING, "FlagName" ),
	DEFINE_KEYFIELD( m_iHUDSlot, FIELD_INTEGER, "HUDSlot" ),
	DEFINE_KEYFIELD( m_bNotUncappable, FIELD_BOOLEAN, "NotUncappable" ),
	DEFINE_KEYFIELD( m_bUncapOnDeath, FIELD_BOOLEAN, "UncapOnDeath" ),
	DEFINE_KEYFIELD( m_sNeutralFlagModelName, FIELD_STRING, "NeutralFlagModelName" ),
	DEFINE_KEYFIELD( m_sDisabledFlagModelName, FIELD_STRING, "DisabledFlagModelName" ),
	DEFINE_KEYFIELD( m_sBritishFlagModelName, FIELD_STRING, "BritishFlagModelName" ),
	DEFINE_KEYFIELD( m_sAmericanFlagModelName, FIELD_STRING, "AmericanFlagModelName" ),

#ifndef CLIENT_DLL
	DEFINE_THINKFUNC( Think ),
#endif

#ifndef CLIENT_DLL
	//BG2 - SaintGreg - dynamic flags
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	//BG2 - SaintGreg - Output functions similar to BG's
	DEFINE_OUTPUT( m_OnAmericanStartCapture, "OnAmericanStartCapture" ),
	DEFINE_OUTPUT( m_OnBritishStartCapture, "OnBritishStartCapture" ),
	DEFINE_OUTPUT( m_OnStartCapture, "OnStartCapture" ),
	DEFINE_OUTPUT( m_OnAmericanCapture, "OnAmericanCapture" ),
	DEFINE_OUTPUT( m_OnBritishCapture, "OnBritishCapture" ),
	DEFINE_OUTPUT( m_OnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_OnAmericanStopCapture, "OnAmericanStopCapture" ),
	DEFINE_OUTPUT( m_OnBritishStopCapture, "OnBritishStopCapture" ),
	DEFINE_OUTPUT( m_OnStopCapture, "OnStopCapture" ),
	DEFINE_OUTPUT( m_OnAmericanLosePoint, "OnAmericanLosePoint" ),
	DEFINE_OUTPUT( m_OnBritishLosePoint, "OnBritishLosePoint" ),
	DEFINE_OUTPUT( m_OnLosePoint, "OnLosePoint" ),
	DEFINE_OUTPUT( m_OnEnable, "OnEnable" ),
	DEFINE_OUTPUT( m_OnDisable, "OnDisable" ),
#endif // CLIENT_DLL

END_DATADESC()

LINK_ENTITY_TO_CLASS(flag, CFlag);
PRECACHE_REGISTER(flag);
