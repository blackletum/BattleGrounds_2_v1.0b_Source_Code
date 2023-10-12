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

/*#ifdef CLIENT_DLL
#define CFlag C_Flag
#endif*/

#ifdef CLIENT_DLL
#error DO NOT USE ON CLIENT! c_flag.cpp/.h is for that
#endif

const int CFlag_START_DISABLED = 1;		// spawnflag definition

//BG2 - Tjoppen - TODO: replace the use of ClientPrintAll with custom usermessages
void ClientPrintAll( char *str, bool printfordeadplayers = false, bool forcenextclientprintall = false );

class CFlag : public CBaseAnimating
{
	DECLARE_CLASS( CFlag, CBaseAnimating );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	CNetworkVar( int, m_iLastTeam );		//which team is near this flag?
	CNetworkVar( int, m_iRequestingCappers); //yeah, extra int, im too drunk to think of a better way :o
	CNetworkVar( float, m_flNextCapture );

	CNetworkVar( int, m_iCapturePlayers );	//how many player must be nearby to capture this flag?
	CNetworkVar( int, m_iNearbyPlayers );	//how many players ARE nearby to capture this flag?
											// alternatively: how overloaded is this flag
	CNetworkVar( int, m_iForTeam );

	CNetworkVar( int, m_iHUDSlot );		//in which slot is the icon for this flag?

	CNetworkVar( bool, m_bActive );		//BG2 - Tjoppen - adding SaintGreg's flag stuff from way back as a placeholder
										//				  until the new flag code is done.

	CNetworkVar( bool, m_bNotUncappable );	//is flag non-uncappable?

#ifndef CLIENT_DLL
	//BG2 - SaintGreg - Output functions similar to BG's
	COutputEvent m_OnAmericanStartCapture;
	COutputEvent m_OnBritishStartCapture;
	COutputEvent m_OnStartCapture;
	COutputEvent m_OnAmericanCapture;
	COutputEvent m_OnBritishCapture;
	COutputEvent m_OnCapture;
	COutputEvent m_OnAmericanStopCapture;
	COutputEvent m_OnBritishStopCapture;
	COutputEvent m_OnStopCapture;
	COutputEvent m_OnAmericanLosePoint;
	COutputEvent m_OnBritishLosePoint;
	COutputEvent m_OnLosePoint;
	COutputEvent m_OnEnable;
	COutputEvent m_OnDisable;
#endif // CLIENT_DLL

	bool IsActive( void );
	void InputEnable( inputdata_t &inputData );
	void InputDisable( inputdata_t &inputData );
	void InputToggle( inputdata_t &inputData );


	int		m_iUncap,
			m_iTeamBonus,
			m_iTeamBonusInterval,
			m_iPlayerBonus;

	bool	m_bUncapOnDeath;

	float	m_flNextTeamBonus;

	string_t	m_sNeutralFlagModelName,
				m_sDisabledFlagModelName,
				m_sBritishFlagModelName,
				m_sAmericanFlagModelName;

	const char* GetNeutralModelName()
	{
		//for backward compatibility without this name defined
		if( strlen(STRING( m_sNeutralFlagModelName )) == 0 )
			return "models/other/flag_n.mdl";
		else
			return STRING( m_sNeutralFlagModelName );
	}

	const char* GetDisabledModelName()
	{
		//for backward compatibility without this name defined
		if( strlen(STRING( m_sDisabledFlagModelName )) == 0 )
			return "models/other/flag_w.mdl";
		else
			return STRING( m_sDisabledFlagModelName );
	}

	const char* GetBritishModelName()
	{
		//for backward compatibility without this name defined
		if( strlen(STRING( m_sBritishFlagModelName )) == 0 )
			return "models/other/flag_b.mdl";
		else
			return STRING( m_sBritishFlagModelName );
	}

	const char* GetAmericanModelName()
	{
		//for backward compatibility without this name defined
		if( strlen(STRING( m_sAmericanFlagModelName )) == 0 )
			return "models/other/flag_a.mdl";
		else
			return STRING( m_sAmericanFlagModelName );
	}

	CNetworkVar( float,	m_flCaptureTime );	//.. and for how long?
	float	m_flCaptureRadius;				//.. and how close?

	CNetworkVar( string_t, m_sFlagName );

public:
	CUtlVector<CBasePlayer*>	m_vOverloadingPlayers;	//nearby or overloading players
	/*
	some notes on the list:
	- when a player dies, remove from list
	- when a player changes team, remove from list
	- when a player disconnects, remove from list
	- I couldn't be arsed to make it private with accessors and shit. Java has made me quite sick of that
	*/

	void Spawn( void );
	void Precache( void );
	void Think( void );
	void ThinkUncapped( void );
	void ThinkCapped( void );
	void Capture( int iTeam );	//let iTeam have this flag now..
	void ChangeTeam( int iTeamNum );
	virtual int UpdateTransmitState();
};

class CBasePoint : public CBaseTrigger
{
	DECLARE_CLASS( CBasePoint, CBaseTrigger );
public:
	// The usual suspects. Think is called externally, as brushes don't "think" per se.
	void	Spawn( void );
	void	Think();
	// Accessor functions for private variables
	int		GetTeam();
	float	GetCaptureTime();
	float	GetNextCaptureTime();
	int		GetTricklePoints();
	int		GetTeamCapturePoints();
	int		GetPlayerCapturePoints();
	int		GetAssaultTeam();
	int		GetMinPlayers();
	int		GetPlayers();
	int		GetUncapturable();

private:

	// When a player becomes involved or becomes uninvolved(through touching, leaving and 
	// dieing) in this point they are taken or put in this list.
	CUtlVector< CBasePlayer* >	m_hPlayers;
	
	// The team that is making an action on this point will be kept here, as their TEAM_* constants
	int		m_iTeam;
	// If this is actually captured
	int		m_iCaptured;
	// If this is capturing
	int		m_iCapturing;
	// How long it takes to make a capture
	float	m_flCaptureTime;
	// Time untill next capture
	float	m_flNextCaptureTime;
	// How many points the holding team gets at trickle time
	int		m_iTricklePoints;
	// How many points the new holding team gets at capture time
	int		m_iTeamCapturePoints;
	// How many points each player on the new holding team involved in the point at capture time get
	int		m_iPlayerCapturePoints;
	// The team that this flag is for in Assault maps, leave as 0 for everyone(Domination maps)
	int		m_iAssaultTeam;
	// Amount of players needed to begin holding this point
	int		m_iMinPlayers;
	// Amount of players currently involved in this flag
	int		m_iPlayers;
	// Whether this flag will lose an involved player when they die
	int		m_iUncapturable;

	int m_iHUDSlot;
	string_t m_sFlagName;

	// Entities currently being touched by this trigger
	//CUtlVector< EHANDLE >	m_hTouchingEntities;

	// Outputs allow our mappers to give feedback to the players in much better ways than
	// in previous versions of BGII. Mappers will have to set up their own sounds and effects
	// for any of the events they wish to use. Obviously, something on capture is a must.
	COutputEvent m_OnAmericanStartCapture;
	COutputEvent m_OnBritishStartCapture;
	COutputEvent m_OnStartCapture;
	COutputEvent m_OnAmericanCapture;
	COutputEvent m_OnBritishCapture;
	COutputEvent m_OnCapture;
	COutputEvent m_OnAmericanStopCapture;
	COutputEvent m_OnBritishStopCapture;
	COutputEvent m_OnStopCapture;
	COutputEvent m_OnAmericanLosePoint;
	COutputEvent m_OnBritishLosePoint;
	COutputEvent m_OnLosePoint;
	COutputEvent m_OnEnable;
	COutputEvent m_OnDisable;

	DECLARE_DATADESC();
};