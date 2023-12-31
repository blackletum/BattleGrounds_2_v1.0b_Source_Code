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

//BG2 - Tjoppen - CSpawnPoint can be enabled/disabled
class CSpawnPoint : public CPointEntity
{
	DECLARE_CLASS( CSpawnPoint, CPointEntity );
	DECLARE_DATADESC();

	bool	m_bEnabled;
	float	m_flNextSetEnabled;
#define SPAWNPOINT_START_DISABLED	1

public:
	void Spawn( void ) { Reset(); }

	void Reset( void )
	{
		//on round resets and spawns we reset the enabled flag
        
		m_flNextSetEnabled = 0;	//just to make sure SetEnabled() gets called
		SetEnabled( !HasSpawnFlags( SPAWNPOINT_START_DISABLED ) );

		m_flNextSetEnabled = gpGlobals->curtime + 0.1f;				//wait for other outputs and stuff to fire
	}

	bool IsEnabled( void ) { return m_bEnabled; }
	
	void SetEnabled( bool bEnabled )
	{
		if( m_flNextSetEnabled > gpGlobals->curtime )
		{
			//Msg( "preventing SetEnabled from getting called with %i too early\n", bEnabled );
			return;
		}

		//Msg( "m_bEnabled = %i\n", bEnabled );
		m_bEnabled = bEnabled;
	}

	void InputEnable( inputdata_t &inputData ) { /*Msg( "enablind spawn\n" );*/ SetEnabled( true ); }
	void InputDisable( inputdata_t &inputData ) { /*Msg( "disabling spawn\n" );*/ SetEnabled( false ); }
	void InputToggle( inputdata_t &inputData ) { /*Msg( "toggling spawn\n" );*/ SetEnabled( !IsEnabled() ); }
};