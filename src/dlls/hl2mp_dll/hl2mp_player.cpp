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
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h"
#include "hl2mp_player.h"
#include "globalstate.h"
#include "game.h"
#include "gamerules.h"
#include "hl2mp_player_shared.h"
#include "predicted_viewmodel.h"
#include "in_buttons.h"
#include "hl2mp_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "grenade_satchel.h"
#include "eventqueue.h"

//BG2 - Tjoppen - #includes
#include "triggers.h"
#include "bg2/flag.h"
#include "bg2/vcomm.h"
#include "bg2/spawnpoint.h"
//

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

extern ConVar mp_autobalanceteams;
extern ConVar mp_autobalancetolerance;

//BG2 - Tjoppen - sv_voicecomm_text, set to zero for realism
ConVar sv_voicecomm_text( "sv_voicecomm_text", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Show voicecomm text on clients?" );
//

int g_iLastCitizenModel = 0;
int g_iLastCombineModel = 0;

//BG2 - Tjoppen - away with these
/*CBaseEntity	 *g_pLastCombineSpawn = NULL;
CBaseEntity	 *g_pLastRebelSpawn = NULL;
extern CBaseEntity				*g_pLastSpawn;*/
//

void ClientKill( edict_t *pEdict );
//BG2 - Tjoppen - don't need this
//void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade );
//

LINK_ENTITY_TO_CLASS( player, CHL2MP_Player );

//BG2 - Tjoppen - CSpawnPoint
BEGIN_DATADESC( CSpawnPoint )
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
END_DATADESC()
//

LINK_ENTITY_TO_CLASS( info_player_combine, CSpawnPoint );
LINK_ENTITY_TO_CLASS( info_player_rebel, CSpawnPoint );
//BG2 - Tjoppen - info_player_american/british
LINK_ENTITY_TO_CLASS( info_player_american, CSpawnPoint );
LINK_ENTITY_TO_CLASS( info_player_british, CSpawnPoint );
//

IMPLEMENT_SERVERCLASS_ST(CHL2MP_Player, DT_HL2MP_Player)
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11, SPROP_CHANGES_OFTEN ),
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropInt( SENDINFO( m_iSpawnInterpCounter), 4 ),
	SendPropInt( SENDINFO( m_iPlayerSoundType), 3 ),
	
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	//BG2 - Tjoppen - send stamina via C_HL2MP_Player <=> DT_HL2MP_Player <=> CHL2MP_Player
	SendPropInt( SENDINFO( m_iStamina ), 7, SPROP_UNSIGNED ),	//0 <= stamina <= 100, 7 bits enough
	//
	//BG2 - Tjoppen - m_iClass is a network var
	SendPropInt( SENDINFO( m_iClass), 2, SPROP_UNSIGNED ),	//BG2 - Tjoppen - remember: max four classes or increase this
	//
	//BG2 - Tjoppen - rewards put on hold
	/*SendPropInt( SENDINFO( m_iOfficerReward), 7, SPROP_UNSIGNED ),	//BG2 - Draco - Rewards
	SendPropInt( SENDINFO( m_iSniperReward), 7, SPROP_UNSIGNED ),	//BG2 - Draco - Rewards
	SendPropInt( SENDINFO( m_iInfantryReward), 7, SPROP_UNSIGNED ),	//BG2 - Draco - Rewards*/

//	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
//	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
	
END_SEND_TABLE()

BEGIN_DATADESC( CHL2MP_Player )
END_DATADESC()

const char *g_ppszRandomCitizenModels[] = 
{
	//BG2 - Tjoppen - models
	"models/player/british/jager/jager.mdl",
	"models/player/british/medium_b/medium_b.mdl",
	"models/player/british/light_b/light_b.mdl",
	/*"models/heavy_b/heavy_b.mdl",
	"models/medium_b/medium_b.mdl",*/
	/*"models/humans/group03/male_01.mdl",
	"models/humans/group03/male_02.mdl",
	"models/humans/group03/female_01.mdl",
	"models/humans/group03/male_03.mdl",
	"models/humans/group03/female_02.mdl",
	"models/humans/group03/male_04.mdl",
	"models/humans/group03/female_03.mdl",
	"models/humans/group03/male_05.mdl",
	"models/humans/group03/female_04.mdl",
	"models/humans/group03/male_06.mdl",
	"models/humans/group03/female_06.mdl",
	"models/humans/group03/male_07.mdl",
	"models/humans/group03/female_07.mdl",
	"models/humans/group03/male_08.mdl",
	"models/humans/group03/male_09.mdl",*/
};

const char *g_ppszRandomCombineModels[] =
{
	//BG2 - Tjoppen - models
	"models/player/american/heavy_a/heavy_a.mdl",
	"models/player/american/medium_a/medium_a.mdl",
	"models/player/american/light_a/light_a.mdl",
	/*"models/humans/light_a/light_a.mdl",
	"models/humans/medium_a/medium_a.mdl",*/
	/*"models/combine_soldier.mdl",
	"models/combine_soldier_prisonguard.mdl",
	"models/combine_super_soldier.mdl",
	"models/police.mdl",*/
};


#define MAX_COMBINE_MODELS 4
#define MODEL_CHANGE_INTERVAL 5.0f
#define TEAM_CHANGE_INTERVAL 5.0f

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

#pragma warning( disable : 4355 )

CHL2MP_Player::CHL2MP_Player() : m_PlayerAnimState( this )
{
	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	m_iSpawnInterpCounter = 0;

//	UseClientSideAnimation();

	//BG2 - Tjoppen - don't pick a class..
	m_iClass = m_iNextClass = -1;//RandomInt( 0, 2 );
	//
}

CHL2MP_Player::~CHL2MP_Player( void )
{

}

void CHL2MP_Player::UpdateOnRemove( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	BaseClass::UpdateOnRemove();
}

void CHL2MP_Player::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel ( "sprites/glow01.vmt" );

	//Precache Citizen models
	int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );
	int i;	

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCitizenModels[i] );

	//Precache Combine Models
	nHeads = ARRAYSIZE( g_ppszRandomCombineModels );

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCombineModels[i] );

	PrecacheFootStepSounds();

	//BG2 - Tjoppen - precache sounds
	PrecacheScriptSound( "NPC_MetroPolice.Die" );
	PrecacheScriptSound( "NPC_CombineS.Die" );
	PrecacheScriptSound( "NPC_Citizen.die" );
	PrecacheScriptSound( "BG2Player.die" );
	//PrecacheScriptSound( "BG2Player.pain" );

	//assume right leg is the biggest
	for( i = 0; i <= HITGROUP_RIGHTLEG; i++ )
		PrecacheScriptSound( GetHitgroupPainSound(i)  );

	char msg[512];
	
	for( i = 0; i < VCOMM1_NUM; i++ )
	{	
		Q_snprintf( msg, 512, "VoicecommsA%s", pVComms[i]);
		PrecacheScriptSound( msg );
	}

	for( i = 0; i < VCOMM1_NUM; i++ )
	{	
		Q_snprintf( msg, 512, "VoicecommsB%s", pVComms[i]);
		PrecacheScriptSound( msg );
	}
	
	for( i = VCOMM2_START; i < VCOMM2_START+VCOMM2_NUM; i++ )
	{	
		Q_snprintf( msg, 512, "VoicecommsA%s", pVComms[i]);
		PrecacheScriptSound( msg );
	}

	for( i = VCOMM2_START; i < VCOMM2_START+VCOMM2_NUM; i++ )
	{
		Q_snprintf( msg, 512, "VoicecommsB%s", pVComms[i]);
		PrecacheScriptSound( msg );
	}
}

void CHL2MP_Player::GiveAllItems( void )
{
	EquipSuit();

	/*CBasePlayer::GiveAmmo( 255,	"Pistol");
	CBasePlayer::GiveAmmo( 255,	"AR2" );
	CBasePlayer::GiveAmmo( 5,	"AR2AltFire" );
	CBasePlayer::GiveAmmo( 255,	"SMG1");
	CBasePlayer::GiveAmmo( 1,	"smg1_grenade");
	CBasePlayer::GiveAmmo( 255,	"Buckshot");*/
	CBasePlayer::GiveAmmo( 32,	"357" );
	/*CBasePlayer::GiveAmmo( 3,	"rpg_round");

	CBasePlayer::GiveAmmo( 1,	"grenade" );
	CBasePlayer::GiveAmmo( 2,	"slam" );

	GiveNamedItem( "weapon_crowbar" );
	GiveNamedItem( "weapon_stunstick" );
	GiveNamedItem( "weapon_pistol" );
	GiveNamedItem( "weapon_357" );

	GiveNamedItem( "weapon_smg1" );
	GiveNamedItem( "weapon_ar2" );
	
	GiveNamedItem( "weapon_shotgun" );
	GiveNamedItem( "weapon_frag" );
	
	GiveNamedItem( "weapon_crossbow" );
	
	GiveNamedItem( "weapon_rpg" );

	GiveNamedItem( "weapon_slam" );

	GiveNamedItem( "weapon_physcannon" );*/
	
	//BG2 - Tjoppen - impulse 101
	//GiveNamedItem( "weapon_revolutionnaire" );
	GiveNamedItem( "weapon_brownbess" );
	GiveNamedItem( "weapon_charleville" );
	GiveNamedItem( "weapon_pennsylvania" );
	GiveNamedItem( "weapon_jaeger" );
	GiveNamedItem( "weapon_pistol_a" );
	GiveNamedItem( "weapon_pistol_b" );
	GiveNamedItem( "weapon_sabre_a" );
	GiveNamedItem( "weapon_sabre_b" );
	GiveNamedItem( "weapon_hirschf" );
	GiveNamedItem( "weapon_knife" );
}

void CHL2MP_Player::GiveDefaultItems( void )
{
	EquipSuit();

	//BG2 - Tjoppen - default equipment - also strip old equipment
	/*RemoveAllAmmo();
	Weapon_DropAll( true );*/
	RemoveAllItems( false );

	/*CBasePlayer::GiveAmmo( 255,	"Pistol");
	CBasePlayer::GiveAmmo( 45,	"SMG1");
	CBasePlayer::GiveAmmo( 1,	"grenade" );
	CBasePlayer::GiveAmmo( 6,	"Buckshot");*/
	CBasePlayer::GiveAmmo( 24,	"357", true );
	
	if( GetTeam()->GetTeamNumber() == TEAM_AMERICANS )	//Americans
	{
		switch( m_iClass )
		{
		case CLASS_INFANTRY:
			GiveNamedItem( "weapon_charleville" );
			break;
		case CLASS_OFFICER:
			GiveNamedItem( "weapon_pistol_a" );
			GiveNamedItem( "weapon_sabre_a" );
			break;
		case CLASS_SNIPER:
			//GiveNamedItem( "weapon_revolutionnaire" );	//rev is no more
			GiveNamedItem( "weapon_pennsylvania" );
			GiveNamedItem( "weapon_knife" );
			break;
		}

		//Weapon_Switch( Weapon_OwnsThisType( "weapon_revolutionnaire" ) );
	}
	else if( GetTeam()->GetTeamNumber() == TEAM_BRITISH )	//british
	{
		switch( m_iClass )
		{
		case CLASS_INFANTRY:
			GiveNamedItem( "weapon_brownbess" );
			break;
		case CLASS_OFFICER:
			GiveNamedItem( "weapon_pistol_b" );
			GiveNamedItem( "weapon_sabre_b" );
			break;
		case CLASS_SNIPER:
			GiveNamedItem( "weapon_jaeger" );
			GiveNamedItem( "weapon_hirschf" );
			break;
		}
		
		//Weapon_Switch( Weapon_OwnsThisType( "weapon_brownbess" ) );
	}

	/*if ( GetPlayerModelType() == PLAYER_SOUNDS_METROPOLICE || GetPlayerModelType() == PLAYER_SOUNDS_COMBINESOLDIER )
	{
		GiveNamedItem( "weapon_stunstick" );
	}
	else if ( GetPlayerModelType() == PLAYER_SOUNDS_CITIZEN )
	{
		GiveNamedItem( "weapon_crowbar" );
	}
	
	GiveNamedItem( "weapon_pistol" );
	GiveNamedItem( "weapon_smg1" );
	GiveNamedItem( "weapon_frag" );
	GiveNamedItem( "weapon_physcannon" );

	const char *szDefaultWeaponName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_defaultweapon" );

	CBaseCombatWeapon *pDefaultWeapon = Weapon_OwnsThisType( szDefaultWeaponName );

	if ( pDefaultWeapon )
	{
		Weapon_Switch( pDefaultWeapon );
	}
	else
	{
		Weapon_Switch( Weapon_OwnsThisType( "weapon_physcannon" ) );
	}*/
}

//BG2 - Tjoppen - g_pLastIntermission
CBaseEntity	*g_pLastIntermission = NULL;
//

void CHL2MP_Player::PickDefaultSpawnTeam( void )
{
	//BG2 - Tjoppen - make players just joining the game be in intermission..
	//	this is a bit flaky at the moment.. maybe later

	if( GetTeamNumber() != TEAM_UNASSIGNED )
	{
		m_pIntermission = NULL;	//just to be safe
		return;					//we get called on spawn, so anyone that has a team shouldn't get teleported around and stuff
	}

	//show classmenu
	//engine->ClientCommand( edict(), "classmenu" );
	SetModel( "models/player/british/jager/jager.mdl" );	//shut up about no model already!
	AddEffects( EF_NODRAW );
	//try to find a spot..
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( g_pLastIntermission, "info_intermission");
	if( !pSpot )
	{
		//reached end, start over
		g_pLastIntermission = pSpot = gEntList.FindEntityByClassname( NULL, "info_intermission");

		if( !pSpot )
		{
			Msg( "WARNING: no info_intermission in current map. tell the mapper\n" );
			return;	//oh no! no info_intermission
		}
	}
	else
	{
		g_pLastIntermission = pSpot;
	}

	m_pIntermission = pSpot;

	SetAbsOrigin( pSpot->GetAbsOrigin() );
	SnapEyeAngles( pSpot->GetAbsAngles() );

	//Msg( "* %f %f %f\n", pSpot->GetAbsAngles().x, pSpot->GetAbsAngles().y, pSpot->GetAbsAngles().z );
	//Msg( "* %f %f %f\n", pSpot->GetLocalAngles().x, pSpot->GetLocalAngles().y, pSpot->GetLocalAngles().z );
	pl.fixangle = FIXANGLE_ABSOLUTE;

	return;
	//

	if ( GetTeamNumber() == 0 )
	{
		if ( HL2MPRules()->IsTeamplay() == false )
		{
			if ( GetModelPtr() == NULL )
			{
				const char *szModelName = NULL;
				szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

				if ( ValidatePlayerModel( szModelName ) == false )
				{
					char szReturnString[512];

					//BG2 - Tjoppen - default model
					//Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel models/combine_soldier.mdl\n" );
					int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
					g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
					Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s", g_ppszRandomCombineModels[g_iLastCombineModel] );
					engine->ClientCommand ( edict(), szReturnString );
				}

				ChangeTeam( TEAM_UNASSIGNED );
			}
		}
		else
		{
			CTeam *pCombine = g_Teams[TEAM_AMERICANS];
			CTeam *pRebels = g_Teams[TEAM_BRITISH];

			if ( pCombine == NULL || pRebels == NULL )
			{
				ChangeTeam( random->RandomInt( TEAM_AMERICANS, TEAM_BRITISH ) );
			}
			else
			{
				if ( pCombine->GetNumPlayers() > pRebels->GetNumPlayers() )
				{
					ChangeTeam( TEAM_BRITISH );
				}
				else if ( pCombine->GetNumPlayers() < pRebels->GetNumPlayers() )
				{
					ChangeTeam( TEAM_AMERICANS );
				}
				else
				{
					ChangeTeam( random->RandomInt( TEAM_AMERICANS, TEAM_BRITISH ) );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CHL2MP_Player::Spawn(void)
{
	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	//BG2 - Tjoppen - stop water sounds
	StopSound( "Player.AmbientUnderWater" );
	StopSound( "Player.DrownStart" );
	StopSound( "Player.Wade" );
	//

	//BG2 - Tjoppen - reenable spectators
	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return;	//we're done
	//

	PickDefaultSpawnTeam();

	//BG2 - Tjoppen - reenable spectators
	if ( GetTeamNumber() <= TEAM_SPECTATOR )
		return;	//we're done
	//

	//BG2 - Tjoppen - pick correct model
	m_iClass = m_iNextClass;				//BG2 - Tjoppen - sometimes these may not match

	const char	//*szOldModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" ),
				*szNewModelName = PlayermodelTeamClass( GetTeamNumber(), m_iClass );

	//if( Q_strncmp( szOldModelName, szNewModelName, 256 ) )
	{
		m_flNextModelChangeTime = gpGlobals->curtime - 1; //HACKHACKHACK

		char cmd[512];
		Q_snprintf( cmd, sizeof (cmd), "cl_playermodel %s\n", szNewModelName );
		engine->ClientCommand ( edict(), cmd );
	}
	//

	BaseClass::Spawn();

	m_flNextVoicecomm = gpGlobals->curtime;	//BG2 - Tjoppen - reset voicecomm timer
	m_iStamina = 100;						//BG2 - Draco - reset stamina to 100
	m_fNextStamRegen = gpGlobals->curtime;	//BG2 - Draco - regen stam now!

	pl.deadflag = false;
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	RemoveEffects( EF_NODRAW );

	StopObserverMode();

	GiveDefaultItems();

	RemoveEffects( EF_NOINTERP );

	SetNumAnimOverlays( 3 );
	ResetAnimation();

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;
	
	AddFlag(FL_ONGROUND); // set the player on the ground at the start of the round.

	m_impactEnergyScale = HL2MPPLAYER_PHYSDAMAGE_SCALE;

	if ( HL2MPRules()->IsIntermission() )
	{
		AddFlag( FL_FROZEN );
	}
	else
	{
		RemoveFlag( FL_FROZEN );
	}

	m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;

	m_Local.m_bDucked = false;
}

void CHL2MP_Player::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	
}

bool CHL2MP_Player::StartObserverMode( int mode )
{
	//BG2 - Tjoppen - reenable spectators
	return BaseClass::StartObserverMode( mode );
	//
	//Do nothing.

	return false;
}

bool CHL2MP_Player::ValidatePlayerModel( const char *pModel )
{
	int iModels = ARRAYSIZE( g_ppszRandomCitizenModels );
	int i;	

	for ( i = 0; i < iModels; ++i )
	{
		if ( !Q_stricmp( g_ppszRandomCitizenModels[i], pModel ) )
		{
			return true;
		}
	}

	iModels = ARRAYSIZE( g_ppszRandomCombineModels );

	for ( i = 0; i < iModels; ++i )
	{
	   	if ( !Q_stricmp( g_ppszRandomCombineModels[i], pModel ) )
		{
			return true;
		}
	}

	return false;
}

void CHL2MP_Player::SetPlayerTeamModel( void )
{
	const char *szModelName = NULL;
	szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( modelIndex == -1 || ValidatePlayerModel( szModelName ) == false )
	{
		//BG2 - Tjoppen - default model
		//szModelName = "models/Combine_Soldier.mdl";
		int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];
		m_iModelType = TEAM_AMERICANS;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}

	if ( GetTeamNumber() == TEAM_AMERICANS )
	{
		//BG2 - Tjoppen - grab random model
		//if ( Q_stristr( szModelName, "models/human") )
		//if ( !Q_stristr( szModelName, "models/human") )
		if ( !Q_stristr( szModelName, "models/player/american") )	//did we pick a non-american model?
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		
			g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
			szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];
		}

		m_iModelType = TEAM_AMERICANS;
	}
	else if ( GetTeamNumber() == TEAM_BRITISH )
	{
		//BG2 - Tjoppen - grab random model
		//if ( !Q_stristr( szModelName, "models/human") )
		//if ( Q_stristr( szModelName, "models/human") )
		if ( !Q_stristr( szModelName, "models/player/british") )	//did we pick a non-british model?
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

			g_iLastCitizenModel = ( g_iLastCitizenModel + 1 ) % nHeads;
			szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];
		}

		m_iModelType = TEAM_BRITISH;
	}
	
	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );

	//BG2 - Tjoppen - TEAM_UNASSIGNED doesn't limit teamchange.. and stuff
	if( GetTeamNumber() == TEAM_UNASSIGNED )
		m_flNextModelChangeTime = gpGlobals->curtime;
	else
	//
	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

void CHL2MP_Player::SetPlayerModel( void )
{
	const char *szModelName = NULL;
	const char *pszCurrentModelName = modelinfo->GetModelName( GetModel());

	szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	if ( ValidatePlayerModel( szModelName ) == false )
	{
		char szReturnString[512];

		if ( ValidatePlayerModel( pszCurrentModelName ) == false )
		{
			//BG2 - Tjoppen - default model
			//pszCurrentModelName = "models/Combine_Soldier.mdl";
			int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
			g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
			pszCurrentModelName = g_ppszRandomCombineModels[g_iLastCombineModel];
		}

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pszCurrentModelName );
		engine->ClientCommand ( edict(), szReturnString );

		szModelName = pszCurrentModelName;
	}

	if ( GetTeamNumber() == TEAM_AMERICANS )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		
		g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];

		m_iModelType = TEAM_AMERICANS;
	}
	else if ( GetTeamNumber() == TEAM_BRITISH )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

		g_iLastCitizenModel = ( g_iLastCitizenModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];

		m_iModelType = TEAM_BRITISH;
	}
	else
	{
		if ( Q_strlen( szModelName ) == 0 ) 
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );
			g_iLastCitizenModel = ( g_iLastCitizenModel  + 1 ) % nHeads;
			szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];
		}

		//if ( Q_stristr( szModelName, "models/human") )
		if ( Q_stristr( szModelName, "models/player/british") )
		{
			m_iModelType = TEAM_BRITISH;
		}
		else
		{
			m_iModelType = TEAM_AMERICANS;
		}
	}

	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( modelIndex == -1 )
	{
		//BG2 - Tjoppen - default model
		//szModelName = "models/Combine_Soldier.mdl";
		int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];
		m_iModelType = TEAM_AMERICANS;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}

	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );

	//BG2 - Tjoppen - TEAM_UNASSIGNED doesn't limit teamchange.. and stuff
	if( GetTeamNumber() == TEAM_UNASSIGNED )
		m_flNextModelChangeTime = gpGlobals->curtime;
	else
	//
	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

void CHL2MP_Player::SetupPlayerSoundsByModel( const char *pModelName )
{
	//BG2 - Tjoppen - only one sound type
	m_iPlayerSoundType = PLAYER_SOUNDS_CITIZEN;
	return;
	//
	if ( Q_stristr( pModelName, "models/human") )
	{
		m_iPlayerSoundType = PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr(pModelName, "police" ) )
	{
		m_iPlayerSoundType = PLAYER_SOUNDS_METROPOLICE;
	}
	else if ( Q_stristr(pModelName, "combine" ) )
	{
		m_iPlayerSoundType = PLAYER_SOUNDS_COMBINESOLDIER;
	}
}

void CHL2MP_Player::ResetAnimation( void )
{
	if ( IsAlive() )
	{
		SetSequence ( -1 );
		SetActivity( ACT_INVALID );

		if (!GetAbsVelocity().x && !GetAbsVelocity().y)
			SetAnimation( PLAYER_IDLE );
		else if ((GetAbsVelocity().x || GetAbsVelocity().y) && ( GetFlags() & FL_ONGROUND ))
			SetAnimation( PLAYER_WALK );
		else if (GetWaterLevel() > 1)
			SetAnimation( PLAYER_WALK );
	}
}


bool CHL2MP_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );

	if ( bRet == true )
	{
		ResetAnimation();
	}

	return bRet;
}

//=============================================================
//CHL2MP_Player's IncreaseReward
//increases the points towards a bonus, 1 for class, 2 for team
//=============================================================
//BG2 - Tjoppen - rewards put on hold
/*void CHL2MP_Player::IncreaseReward(int iType)
{
/*	switch (iType)
	{
		case 1://class specific
			switch (m_iClass)
			{
				case CLASS_INFANTRY:
					m_iInfantryReward++;
					switch (m_iInfantryReward)
					{
						case 10:
							m_iInfantryLevel = 2;
							break;
						case 20:
							m_iInfantryLevel = 3;
							break;
						case 30:
							m_iInfantryLevel = 4;
							break;
					}
					break;
				case CLASS_OFFICER:
					m_iOfficerReward++;
					switch (m_iOfficerReward)
					{
						case 10:
							m_iOfficerLevel = 2;
							break;
						case 6:
							m_iOfficerLevel = 3;
							break;
						case 9:
							m_iOfficerLevel = 4;
							break;
					}
					break;
				case CLASS_SNIPER:
					m_iSniperReward++;
					switch (m_iSniperReward)
					{
						case 3:
							m_iSniperLevel = 2;
							break;
						case 6:
							m_iSniperLevel = 3;
							break;
						case 9:
							m_iSniperLevel = 4;
							break;
					}
					break;
			}
			break;
		case 2://team specific
			switch (GetTeamNumber())
			{
				case TEAM_AMERICANS:
					m_iAmericanReward++;
					switch (m_iAmericanReward)
					{
						case 3:
							m_iAmericanLevel = 2;
							break;
						case 6:
							m_iAmericanLevel = 3;
							break;
						case 9:
							m_iAmericanLevel = 4;
							break;
					}
					break;
				case TEAM_BRITISH:
					m_iBritishReward++;
					switch (m_iBritishReward)
					{
						case 3:
							m_iBritishLevel = 2;
							break;
						case 6:
							m_iBritishLevel = 3;
							break;
						case 9:
							m_iBritishLevel = 4;
							break;
					}
					break;
			}
			break;
	}*//*
}*/

void CHL2MP_Player::HandleSpeedChanges( void )
{
	//int buttonsChanged = m_afButtonPressed | m_afButtonReleased;
	//BG2 - Tjoppen - walk/run speeds
	//BG2 - Draco - stamina changes speed
	//float scale = 0.5f + 0.5f * expf( -0.01f * (float)(100 - GetHealth()) );
	float scale = expf( -0.01f * (float)(100 - m_iStamina) );
	
	if( GetActiveWeapon() && GetActiveWeapon()->m_bInReload )
		scale *= 0.5f;

	int iSpeed = 190;
	int iSpeed2 = 150;

	switch (m_iClass)
	{
		case CLASS_INFANTRY:
			iSpeed = 190;
			iSpeed2 = 120;
			break;
		case CLASS_OFFICER:
			/*switch (m_iOfficerLevel)
			{
				case 1:*/
					iSpeed = 220;
					iSpeed2 = 140;
					/*break;
				case 2:
					iSpeed = 205;
					iSpeed2 = 145;
					break;
				case 3:
					iSpeed = 210;
					iSpeed2 = 150;
					break;
				case 4:
					iSpeed = 215;
					iSpeed2 = 155;
					break;
			}*/
			break;
		case CLASS_SNIPER:
			iSpeed = 200;
			iSpeed2 = 130;
			break;
	}

	if( m_nButtons & IN_WALK )
		SetMaxSpeed( iSpeed2 * scale );
	else
		SetMaxSpeed( iSpeed * scale );
}

void CHL2MP_Player::PreThink( void )
{
	QAngle vOldAngles = GetLocalAngles();
	QAngle vTempAngles = GetLocalAngles();

	vTempAngles = EyeAngles();

	if ( vTempAngles[PITCH] > 180.0f )
	{
		vTempAngles[PITCH] -= 360.0f;
	}

	SetLocalAngles( vTempAngles );

	BaseClass::PreThink();

	HandleSpeedChanges();

	//Reset bullet force accumulator, only lasts one frame
	m_vecTotalBulletForce = vec3_origin;
	SetLocalAngles( vOldAngles );
}

void CHL2MP_Player::PostThink( void )
{
	BaseClass::PostThink();

	//BG2 - Draco - recurring stamina regen
	//BG2 - Tjoppen - update stamina 3 units at a time, at about 4Hz, to reduce network load
	if ( m_fNextStamRegen <= gpGlobals->curtime )
	{
		if( m_iStamina < 100 )
		{
			m_iStamina += 3;

			/*CSingleUserRecipientFilter user( this );
			user.MakeReliable();
			UserMessageBegin( user, "Stamina" );
				WRITE_SHORT( m_iStamina );
			MessageEnd();*/
		}

		m_fNextStamRegen = gpGlobals->curtime + 0.15;	//was 0.225
		//m_fNextStamRegen = gpGlobals->curtime + 0.075;
	}

	if( m_iStamina > 100 )
		m_iStamina = 100;	//cap if for some reason it went over 100
	//

	m_PlayerAnimState.Update();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );

	//BG2 - Tjoppen - follow info_intermission
	if( m_pIntermission && GetTeamNumber() == TEAM_UNASSIGNED )
	{
		SetAbsOrigin( m_pIntermission->GetAbsOrigin() );
		SnapEyeAngles( m_pIntermission->GetAbsAngles() );
		pl.fixangle = FIXANGLE_ABSOLUTE;
	}
}

void CHL2MP_Player::FireBullets ( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t modinfo = info;

	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>( GetActiveWeapon() );

	if ( pWeapon )
	{
		//BG2 - Tjoppen - maintain compatibility with HL2 weapons...
		if( modinfo.m_iDamage == -1 )
			modinfo.m_iDamage = modinfo.m_iPlayerDamage;
		else
		//
		modinfo.m_iPlayerDamage = modinfo.m_iDamage = pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
	}

	NoteWeaponFired();

	BaseClass::FireBullets( modinfo );
}

void CHL2MP_Player::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

extern ConVar sv_maxunlag;

bool CHL2MP_Player::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	//BG2 - Tjoppen - MELEE FIX! I've never been so relieved! So relieved - and ANGRY!
	//if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
	if ( !( pCmd->buttons & IN_ATTACK ) && !( pCmd->buttons & IN_ATTACK2 ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
	//
		return false;

	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pPlayer->entindex() ) )
		return false;

	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pPlayer->GetAbsOrigin();

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5 * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );
	
	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );

	float flCosAngle = 0.707107f;	// 45 degree angle
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

Activity CHL2MP_Player::TranslateTeamActivity( Activity ActToTranslate )
{
	if ( m_iModelType == TEAM_AMERICANS )
		 return ActToTranslate;
	
	if ( ActToTranslate == ACT_RUN )
		 return ACT_RUN_AIM_AGITATED;

	if ( ActToTranslate == ACT_IDLE )
		 return ACT_IDLE_AIM_AGITATED;

	if ( ActToTranslate == ACT_WALK )
		 return ACT_WALK_AIM_AGITATED;

	return ActToTranslate;
}

extern ConVar hl2_normspeed;

// Set the activity based on an event or current state
void CHL2MP_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;

	float speed;

	speed = GetAbsVelocity().Length2D();

	
	// bool bRunning = true;

	//Revisit!
/*	if ( ( m_nButtons & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) )
	{
		if ( speed > 1.0f && speed < hl2_normspeed.GetFloat() - 20.0f )
		{
			bRunning = false;
		}
	}*/

	if ( GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_HL2MP_RUN;

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if ( playerAnim == PLAYER_JUMP )
	{
		idealActivity = ACT_HL2MP_JUMP;
	}
	else if ( playerAnim == PLAYER_DIE )
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			return;
		}
	}
	else if ( playerAnim == PLAYER_ATTACK1 )
	{
		if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
		else
		{
			idealActivity = ACT_HL2MP_GESTURE_RANGE_ATTACK;
		}
	}
	//BG2 - Tjoppen - PLAYER_ATTACK2
	else if ( playerAnim == PLAYER_ATTACK2 )
	{
		if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
		else
		{
			idealActivity = ACT_RANGE_ATTACK2;
		}
	}
	//
	else if ( playerAnim == PLAYER_RELOAD )
	{
		idealActivity = ACT_HL2MP_GESTURE_RELOAD;
	}
	else if ( playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK )
	{
		if ( !( GetFlags() & FL_ONGROUND ) && GetActivity( ) == ACT_HL2MP_JUMP )	// Still jumping
		{
			idealActivity = GetActivity( );
		}
		/*
		else if ( GetWaterLevel() > 1 )
		{
			if ( speed == 0 )
				idealActivity = ACT_HOVER;
			else
				idealActivity = ACT_SWIM;
		}
		*/
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				if ( speed > 0 )
				{
					idealActivity = ACT_HL2MP_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE_CROUCH;
				}
			}
			else
			{
				if ( speed > 0 )
				{
					/*
					if ( bRunning == false )
					{
						idealActivity = ACT_WALK;
					}
					else
					*/
					{
						idealActivity = ACT_HL2MP_RUN;
					}
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE;
				}
			}
		}

		//BG2 - Tjoppen - idle weapons.. but not until smoke and stuff have come out
		if( GetActiveWeapon() && GetActiveWeapon()->m_flNextPrimaryAttack < gpGlobals->curtime &&
			GetActiveWeapon()->m_flNextSecondaryAttack < gpGlobals->curtime )
			Weapon_SetActivity( Weapon_TranslateActivity( ACT_HL2MP_IDLE ), 0 );
		//

		idealActivity = TranslateTeamActivity( idealActivity );
	}
	
	if ( idealActivity == ACT_HL2MP_GESTURE_RANGE_ATTACK )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );

		// FIXME: this seems a bit wacked
		Weapon_SetActivity( Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );

		return;
	}
	//BG2 - Tjoppen - ACT_RANGE_ATTACK2
	else if ( idealActivity == ACT_RANGE_ATTACK2 )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );
		Weapon_SetActivity( Weapon_TranslateActivity( ACT_RANGE_ATTACK2 ), 0 );
		return;
	}
	//
	else if ( idealActivity == ACT_HL2MP_GESTURE_RELOAD )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );
		return;
	}
	else
	{
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( Weapon_TranslateActivity ( idealActivity ) );

		if (animDesired == -1)
		{
			animDesired = SelectWeightedSequence( idealActivity );

			if ( animDesired == -1 )
			{
				animDesired = 0;
			}
		}
	
		// Already using the desired animation?
		if ( GetSequence() == animDesired )
			return;

		m_flPlaybackRate = 1.0;
		ResetSequence( animDesired );
		SetCycle( 0 );
		return;
	}

	// Already using the desired animation?
	if ( GetSequence() == animDesired )
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	SetCycle( 0 );
}


extern int	gEvilImpulse101;
//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CHL2MP_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( IsEFlagSet( EFL_NO_WEAPON_PICKUP ) )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
	{
		return false;
	}

	bool bOwnsWeaponAlready = !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType());

	if ( bOwnsWeaponAlready == true ) 
	{
		//If we have room for the ammo, then "take" the weapon too.
		 if ( Weapon_EquipAmmoOnly( pWeapon ) )
		 {
			 pWeapon->CheckRespawn();

			 UTIL_Remove( pWeapon );
			 return true;
		 }
		 else
		 {
			 return false;
		 }
	}

	pWeapon->CheckRespawn();
	Weapon_Equip( pWeapon );

	return true;
}

void CHL2MP_Player::ChangeTeam( int iTeam )
{
/*	if ( GetNextTeamChangeTime() >= gpGlobals->curtime )
	{
		char szReturnString[128];
		Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch teams again.\n", (int)(GetNextTeamChangeTime() - gpGlobals->curtime) );

		ClientPrint( this, HUD_PRINTTALK, szReturnString );
		return;
	}*/

	bool bKill = false;

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( iTeam != GetTeamNumber() && GetTeamNumber() != TEAM_UNASSIGNED )
		{
			bKill = true;
		}
	}

	//BG2 - Tjoppen - changing team. remove self from flags
	if( iTeam != GetTeamNumber() )
		RemoveSelfFromFlags();
	//

	BaseClass::ChangeTeam( iTeam );

	m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		SetPlayerTeamModel();
	}
	else
	{
		SetPlayerModel();
	}

	if ( bKill == true )
	{
		//BG2 - Tjoppen - teamchange suicides have no time limit...
		SetSuicideTime( gpGlobals->curtime );
		//
		ClientKill( edict() );
	}
}

//BG2 - Tjoppen - ClientPrinttTalkAll
void ClientPrinttTalkAll( char *str )
{
	CBasePlayer *pPlayer = NULL;
	while( (pPlayer = (CBasePlayer*)gEntList.FindEntityByClassname( pPlayer, "player" )) != NULL )
		ClientPrint( pPlayer, HUD_PRINTTALK, str );
}
//

//BG2 - Tjoppen - PlayermodelTeamClass - gives models for specified team/class
const char* CHL2MP_Player::PlayermodelTeamClass( int team, int classid )
{
	switch( team )
	{
	case TEAM_AMERICANS:
		switch( classid )
		{
		case CLASS_INFANTRY:
			return "models/player/american/heavy_a/heavy_a.mdl";

		case CLASS_OFFICER:
			return "models/player/american/light_a/light_a.mdl";

		case CLASS_SNIPER:
			return "models/player/american/medium_a/medium_a.mdl";
		}
	case TEAM_BRITISH:
		switch( classid )
		{
		case CLASS_INFANTRY:
			return "models/player/british/medium_b/medium_b.mdl";

		case CLASS_OFFICER:
			return "models/player/british/light_b/light_b.mdl";

		case CLASS_SNIPER:
			return "models/player/british/jager/jager.mdl";
		}
	}

	//default model
	return "models/player/british/jager/jager.mdl";
}

//BG2 - Tjoppen - CHL2MP_Player::MayRespawn()
bool CHL2MP_Player::MayRespawn( void )
{
	//note: this function only checks if we MAY respawn. not if we can(due to crowded spawns perhaps)
	if( GetTeamNumber() <= TEAM_SPECTATOR )
		return false;

	CTeam	*pAmer = g_Teams[TEAM_AMERICANS],
			*pBrit = g_Teams[TEAM_BRITISH];

	if( !pAmer || !pBrit )
		return false;

	if( pAmer->GetNumPlayers() <= 0 || pBrit->GetNumPlayers() <= 0 )
		return true;

	extern ConVar	mp_respawnstyle,
					mp_respawntime;

	switch( mp_respawnstyle.GetInt() )
	{
	default:
		if( gpGlobals->curtime > GetDeathTime() + DEATH_ANIMATION_TIME )
			return true;
		else
			return false;
	case 1:
		//waves - we're not allowed to respawn by ourselves. the gamerules decide for us
		return false;
	case 2:
		//rounds - we're not allowed to respawn by ourselves. the gamerules decide for us
		return false;
		break;
	}
}
//

bool CHL2MP_Player::AttemptJoin( int iTeam, int iClass, const char *pClassName )
{
	//returns true on success, false otherwise
	CTeam	*pAmericans = g_Teams[TEAM_AMERICANS],
			*pBritish = g_Teams[TEAM_BRITISH];
	
	if( GetTeamNumber() == iTeam && m_iNextClass == iClass )
			return true;
	
	//check so we don't ruin the team balance..
	//BG2 - Tjoppen - don't bother with checking balance if we're just changing class, not team.
	//					CHL2MPRules::Think() will make sure the teams are kept balanced.
	//					We want to allow people to change class even if their team is too big.
	if( mp_autobalanceteams.GetInt() == 1 && GetTeamNumber() != iTeam )
	{
		int iAutoTeamBalanceTeamDiff,
			iAutoTeamBalanceBiggerTeam;
		
		if (pAmericans->GetNumPlayers() > pBritish->GetNumPlayers())
		{
			iAutoTeamBalanceTeamDiff = ((pAmericans->GetNumPlayers() - pBritish->GetNumPlayers()) + 1);
			iAutoTeamBalanceBiggerTeam = TEAM_AMERICANS;
		}
		else
		{
			iAutoTeamBalanceTeamDiff = ((pBritish->GetNumPlayers() - pAmericans->GetNumPlayers()) + 1);
			iAutoTeamBalanceBiggerTeam = TEAM_BRITISH;
		}

		if ((iAutoTeamBalanceTeamDiff >= mp_autobalancetolerance.GetInt()) && (iAutoTeamBalanceBiggerTeam == iTeam))
		{
			//BG2 - Tjoppen - TODO: usermessage this
			ClientPrint( this, HUD_PRINTCENTER, "There are too many players in this team\n" );
			return false;
		}
	}

	//check if there's a limit for this class, and if it has been exceeded
	//if we're changing back to the class we currently are, because perhaps we regret choosing
	// a new class, skip the limit check
	int limit = HL2MPRules()->GetLimitTeamClass( iTeam, iClass );
	if( limit >= 0 && g_Teams[iTeam]->GetNumOfClass(iClass) >= limit &&
		!(GetTeamNumber() == iTeam && m_iClass == iClass ) )
	{
		//BG2 - Tjoppen - TODO: usermessage this
		ClientPrint( this, HUD_PRINTCENTER, "There are too many of this class on your team\n" );
		return false;
	}

	//BG2 - Tjoppen - TODO: usermessage this
	char str[512];
	Q_snprintf( str, 512, "%s is going to fight as %s for the %s\n", STRING(PlayerData()->netname), pClassName, iTeam == TEAM_AMERICANS ? "Americans" : "British" );
	ClientPrinttTalkAll( str );

	m_iNextClass = iClass;

	//The following line prevents anyone else from stealing our spot..
	//Without this line several teamswitching/new players can pick a free class, so there can be for instance 
	// two loyalists even though the limit is one.
	//This may be slightly unfair since a still living player may "steal" a spot without spawning as that class,
	// since it's possible to switch classes around very fast. a player could block the use of a limited class,
	// though it'd be very tedious. It's a tradeoff.
	m_iClass = m_iNextClass;

	if( GetTeamNumber() != iTeam )
	{
		//let Spawn() figure the model out
		ClientKill( edict() );

		if( GetTeamNumber() <= TEAM_SPECTATOR )
		{
			ChangeTeam( iTeam );
			if( MayRespawn() )
				Spawn();
		}
		else
			ChangeTeam( iTeam );
	}

	return true;
}

bool CHL2MP_Player::ClientCommand( const char *cmd )
{
	//BG2 - Tjoppen - class selection
	if ( FStrEq( cmd, "heavy_a" ) )
	{
		AttemptJoin( TEAM_AMERICANS, CLASS_INFANTRY, "a Continental Soldier" );
		return true;
	}
	else if ( FStrEq( cmd, "light_a" ) )
	{
		AttemptJoin( TEAM_AMERICANS, CLASS_OFFICER, "a Continental Officer" );
		return true;
	}
	else if ( FStrEq( cmd, "medium_a" ) )
	{
		AttemptJoin( TEAM_AMERICANS, CLASS_SNIPER, "a Frontiersman" );
		return true;
	}
	if ( FStrEq( cmd, "medium_b" ) )
	{
		AttemptJoin( TEAM_BRITISH, CLASS_INFANTRY, "Royal Infantry" );
		return true;
	}
	else if ( FStrEq( cmd, "light_b" ) )
	{
		AttemptJoin( TEAM_BRITISH, CLASS_OFFICER, "a Royal Commander" );
		return true;
	}
	else if ( FStrEq( cmd, "heavy_b" ) )
	{
		AttemptJoin( TEAM_BRITISH, CLASS_SNIPER, "a Jaeger" );
		return true;
	}
	//BG2 - Tjoppen - voice comms
	else if( FStrEq( cmd, "voicecomm" ) && engine->Cmd_Argc() > 1 )
	{
		int comm = atoi( engine->Cmd_Argv( 1 ) );
		bool teamonly = comm != 6;	//battlecries are not team only (global)

		//only alive assigned player can do voice comms
		//also make sure index not out of bounds
		//make sure global voicecomms are only played ever so often - no FREEDOM! spam
		if( GetTeamNumber() > TEAM_SPECTATOR && IsAlive() && m_flNextVoicecomm <= gpGlobals->curtime &&
			comm >= 0 && comm < NUM_VOICECOMMS && comm != VCOMM1_NUM && comm != VCOMM2_START+VCOMM2_NUM &&
			(teamonly || m_flNextGlobalVoicecomm <= gpGlobals->curtime) )
		{
			char snd[512];

			if( GetTeamNumber() == TEAM_AMERICANS )
				Q_snprintf( snd, 512, "VoicecommsA%s", pVComms[comm]);
			else if( GetTeamNumber() == TEAM_BRITISH )
				Q_snprintf( snd, 512, "VoicecommsB%s", pVComms[comm]);

			//play voicecomm, with attenuation
			EmitSound( snd );

			//done. possibly also tell clients to draw text and stuff if sv_voicecomm_text is true
			m_flNextVoicecomm		= gpGlobals->curtime + 2.0f;

			if( !teamonly )
				m_flNextGlobalVoicecomm	= gpGlobals->curtime + 10.0f;

			if( sv_voicecomm_text.GetBool() )
			{
				//figure out which players should get this message
				CRecipientFilter recpfilter;
				CBasePlayer *client = NULL;

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					client = UTIL_PlayerByIndex( i );
					if ( !client || !client->edict() )
						continue;

					if ( !(client->IsNetClient()) )	// Not a client ? (should never be true)
						continue;

					if ( teamonly && !g_pGameRules->PlayerCanHearChat( client, this ) )//!= GR_TEAMMATE )
						continue;

					if ( !client->CanHearChatFrom( this ) )
						continue;

					recpfilter.AddRecipient( client );
				}

				//make reliable and send
				recpfilter.MakeReliable();

				UserMessageBegin( recpfilter, "VoiceComm" );
					WRITE_BYTE( entindex() );	//voicecomm originator
					WRITE_BYTE( comm | (GetTeamNumber() == TEAM_AMERICANS ? 32 : 0) );	//pack comm number and team
				MessageEnd();
			}
		}

		return true;
	}
	//

	return BaseClass::ClientCommand( cmd );
}

void CHL2MP_Player::CheatImpulseCommands( int iImpulse )
{
	//BG2 - Tjoppen - spectators don't do impulses
	if( GetTeamNumber() <= TEAM_SPECTATOR )
		return;
	//

	switch ( iImpulse )
	{
		case 101:
			{
				if( sv_cheats->GetBool() )
				{
					GiveAllItems();
				}
			}
			break;
		//BG2 - Tjoppen - impulse for figuring out weapon/model/class of player we're looking at
		case 102:
			if( sv_cheats->GetBool() )
			{
				CBaseEntity *FindEntityForward( CBasePlayer *pMe, bool fHull );

				CHL2MP_Player *pPlayer = ToHL2MPPlayer( FindEntityForward( this, true ) );

				if ( pPlayer )
				{
					Msg( "model: %s, class: %i, weapon: %s\n", pPlayer->GetModelName(), pPlayer->m_iClass, pPlayer->GetActiveWeapon() ? pPlayer->GetActiveWeapon()->GetClassname() : "none" );				
				}
			}
			break;
		//

		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

void CHL2MP_Player::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

bool CHL2MP_Player::BecomeRagdollOnClient( const Vector &force )
{
	return true;
}

// -------------------------------------------------------------------------------- //
// Ragdoll entities.
// -------------------------------------------------------------------------------- //

class CHL2MPRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CHL2MPRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( hl2mp_ragdoll, CHL2MPRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CHL2MPRagdoll, DT_HL2MPRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()


void CHL2MP_Player::CreateRagdollEntity( void )
{
	//BG2 - Tjoppen - here's where we put code for multiple ragdolls
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	// If we already have a ragdoll, don't make another one.
	CHL2MPRagdoll *pRagdoll = dynamic_cast< CHL2MPRagdoll* >( m_hRagdoll.Get() );
	
	//BG2 - Tjoppen - here's another place where we put code for multiple ragdolls
	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CHL2MPRagdoll* >( CreateEntityByName( "hl2mp_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		//BG2 - Tjoppen - clamp bullet force
		if( m_vecTotalBulletForce.Length() > 512.f )
		{
			VectorNormalize( m_vecTotalBulletForce );
			m_vecTotalBulletForce *= 512.f;
		}
		//
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
	}

	//BG2 - Tjoppen - remember to remove all ragdolls on round restart
	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2MP_Player::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
		EmitSound( "HL2Player.FlashlightOn" );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	
	if( IsAlive() )
	{
		EmitSound( "HL2Player.FlashlightOff" );
	}
}

void CHL2MP_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	//Drop a grenade if it's primed.
	//BG2 - Tjoppen - don't need this
	/*if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType("weapon_frag");

		if ( GetActiveWeapon() == pGrenade )
		{
			if ( ( m_nButtons & IN_ATTACK ) || (m_nButtons & IN_ATTACK2) )
			{
				DropPrimedFragGrenade( this, pGrenade );
				return;
			}
		}
	}*/
	//

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
}

//BG2 - Tjoppen - don't need this
/*void CHL2MP_Player::DetonateTripmines( void )
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "npc_satchel" )) != NULL)
	{
		CSatchelCharge *pSatchel = dynamic_cast<CSatchelCharge *>(pEntity);
		if (pSatchel->m_bIsLive && pSatchel->GetThrower() == this )
		{
			g_EventQueue.AddEvent( pSatchel, "Explode", 0.20, this, this );
		}
	}

	// Play sound for pressing the detonator
	EmitSound( "Weapon_SLAM.SatchelDetonate" );
}*/

void CHL2MP_Player::Event_Killed( const CTakeDamageInfo &info )
{
	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce( m_vecTotalBulletForce );

	SetNumAnimOverlays( 0 );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	//BG2 - Tjoppen - reenable spectators - no dead bodies raining from the sky
	if( GetTeamNumber() > TEAM_SPECTATOR )
		CreateRagdollEntity();

	//BG2 - Tjoppen - if class changed, change model
	//let Spawn() sort it out
	/*if( m_iClass != m_iNextClass )
	{
		m_iClass = m_iNextClass;

		char cmd[512];
		Q_strncpy( cmd, "cl_playermodel ", 512 );
		strncat( cmd, PlayermodelTeamClass( GetTeamNumber(), GetClass() ), 512 );

		engine->ClientCommand( edict(), cmd );
		//SetModel( PlayermodelTeamClass( GetTeamNumber(), GetClass() ) );
	}*/

	RemoveSelfFromFlags();
	//

	//BG2 - Tjoppen - don't need this
	//DetonateTripmines();

	BaseClass::Event_Killed( subinfo );

	if ( info.GetDamageType() & DMG_DISSOLVE )
	{
		if ( m_hRagdoll )
		{
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
		}
	}

	CBaseEntity *pAttacker = info.GetAttacker();

	if ( pAttacker )
	{
		int iScoreToAdd = 1;

		if ( pAttacker == this )
		{
			iScoreToAdd = -1;
		}

		//BG2 - Tjoppen - only score by winnings rounds..
		//GetGlobalTeam( pAttacker->GetTeamNumber() )->AddScore( iScoreToAdd );
	}

	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects( EF_NODRAW );	// still draw player body
	StopZooming();
}

//BG2 - Tjoppen - GetHitgroupPainSound
const char* CHL2MP_Player::GetHitgroupPainSound( int hitgroup )
{
	if( GetTeamNumber() == TEAM_AMERICANS )
	{
		switch( hitgroup )
		{
		default:
		case HITGROUP_GENERIC	: return "BG2Player.American.pain_generic";
		case HITGROUP_HEAD		: return "BG2Player.American.pain_head";
		case HITGROUP_CHEST		: return "BG2Player.American.pain_chest";
		case HITGROUP_STOMACH	: return "BG2Player.American.pain_stomach";
		case HITGROUP_LEFTARM	: return "BG2Player.American.pain_arm";
		case HITGROUP_RIGHTARM	: return "BG2Player.American.pain_arm";
		case HITGROUP_LEFTLEG	: return "BG2Player.American.pain_leg";
		case HITGROUP_RIGHTLEG	: return "BG2Player.American.pain_leg";
		}
	}
	else
	{
		switch( hitgroup )
		{
		default:
		case HITGROUP_GENERIC	: return "BG2Player.British.pain_generic";
		case HITGROUP_HEAD		: return "BG2Player.British.pain_head";
		case HITGROUP_CHEST		: return "BG2Player.British.pain_chest";
		case HITGROUP_STOMACH	: return "BG2Player.British.pain_stomach";
		case HITGROUP_LEFTARM	: return "BG2Player.British.pain_arm";
		case HITGROUP_RIGHTARM	: return "BG2Player.British.pain_arm";
		case HITGROUP_LEFTLEG	: return "BG2Player.British.pain_leg";
		case HITGROUP_RIGHTLEG	: return "BG2Player.British.pain_leg";
		}
	}
}
//

int CHL2MP_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	m_vecTotalBulletForce += inputInfo.GetDamageForce();
	
	//BG2 - Tjoppen - pain sound
	const char *pModelName = STRING( GetModelName() );

	CSoundParameters params;
	//if ( GetParametersForSound( "BG2Player.pain", params, pModelName ) == false )
	if( GetParametersForSound( GetHitgroupPainSound(LastHitGroup()), params, pModelName ) == false )
		return BaseClass::OnTakeDamage( inputInfo );

	Vector vecOrigin = GetAbsOrigin();
	
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );

	//BG2 - Draco
	CBaseEntity * pAttacker = inputInfo.GetInflictor();
	if (pAttacker->IsPlayer())
	{
		CBasePlayer * pAttacker2 = (CBasePlayer *)pAttacker;
		//subtract 3x damage for attacking teammates!
		pAttacker2->IncrementDeathCount( (int)inputInfo.GetDamage()
										* (GetTeamNumber() == pAttacker2->GetTeamNumber() ? -3 : 1) );
	}

	//BG2 - Tjoppen - take damage => lose some stamina
	m_iStamina -= inputInfo.GetDamage() * 0.3;	//enough to just be noticed, doesn't affect much
	if( m_iStamina < 0 )
		m_iStamina = 0;
	//

	return BaseClass::OnTakeDamage( inputInfo );
}

void CHL2MP_Player::DeathSound( void )
{
	if ( m_hRagdoll && m_hRagdoll->GetBaseAnimating()->IsDissolving() )
		 return;

	//BG2 - Tjoppen - unassigned/spectator don't make deathsound
	if( GetTeamNumber() <= TEAM_SPECTATOR )
		return;
	//

	char szStepSound[128];

	Q_snprintf( szStepSound, sizeof( szStepSound ), "%s.Die", GetPlayerModelSoundPrefix() );

	const char *pModelName = STRING( GetModelName() );

	CSoundParameters params;
	if ( GetParametersForSound( szStepSound, params, pModelName ) == false )
		return;

	Vector vecOrigin = GetAbsOrigin();
	
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );
}

CBaseEntity* CHL2MP_Player::EntSelectSpawnPoint( void )
{
	if( GetTeamNumber() <= TEAM_SPECTATOR )
		return NULL;	//BG2 - Tjoppen - spectators/unassigned don't spawn..

	CUtlVector<CBaseEntity*> spawns;

	//get a non-empty vector of spawns..
	if ( HL2MPRules()->IsTeamplay() )
	{
		if ( GetTeamNumber() == TEAM_AMERICANS )
		{
			//maintain backward compatibility with HL2DM
			CUtlVector<CBaseEntity*> temp;
			gEntList.FindAllEntitiesByClassname( "info_player_american", temp );
			if( temp.Size() == 0 )
			{
				CUtlVector<CBaseEntity*> temp;
				gEntList.FindAllEntitiesByClassname( "info_player_american", temp );
				if( temp.Size() == 0 )
					return NULL;
				else
					spawns = temp;
			}
			else
				spawns = temp;
		}
		else if ( GetTeamNumber() == TEAM_BRITISH )
		{
			//maintain backward compatibility with HL2DM
			CUtlVector<CBaseEntity*> temp;
			gEntList.FindAllEntitiesByClassname( "info_player_british", temp );
			if( temp.Size() == 0 )
			{
				CUtlVector<CBaseEntity*> temp;
				gEntList.FindAllEntitiesByClassname( "info_player_rebel", temp );
				if( temp.Size() == 0 )
					return NULL;
				else
					spawns = temp;
			}
			else
				spawns = temp;
		}
		else
			return NULL;
	}
	else
	{
		//maintain backward compatibility with HL2DM
		CUtlVector<CBaseEntity*> temp;
		gEntList.FindAllEntitiesByClassname( "info_player_deathmatch", temp );
		if( temp.Size() == 0 )
			return NULL;
		else
			spawns = temp;
	}

	//pick a random spot to start attempting spawn at
	int start = random->RandomInt( 0, spawns.Size() - 1 );
	
	for( int i = start;;)
	{
		CBaseEntity *pSpot = spawns[i];
		CSpawnPoint *pSpawn = dynamic_cast<CSpawnPoint*>(pSpot);

		//check that:
		// we don't have NULLs
		// spawn must be enabled
		// it must be valid(noone blocking)
		// it mustn't be at origin(invalid according to original HL2 code)
		if( pSpot && pSpawn &&
			pSpawn->IsEnabled() &&
			g_pGameRules->IsSpawnPointValid( pSpot, this ) &&
			pSpot->GetLocalOrigin() != vec3_origin )
		{
			return pSpot;	//all is fine. return
		}

		//loop around
		i++;
		if( i >= spawns.Size() )
			i = 0;
		else if( i == start )
			return NULL;	//back at start, didn't find anything
	}
}

//BG2 - Tjoppen - CheckSpawnPoints - returns true if there is an unoccupied spawn point
bool CHL2MP_Player::CheckSpawnPoints( void )
{
	if( GetTeamNumber() <= TEAM_SPECTATOR )
		return false;	//BG2 - Tjoppen - spectators/unassigned don't spawn..

	CUtlVector<CBaseEntity*> spawns;

	//get a non-empty vector of spawns..
	if ( HL2MPRules()->IsTeamplay() )
	{
		if ( GetTeamNumber() == TEAM_AMERICANS )
		{
			//maintain backward compatibility with HL2DM
			CUtlVector<CBaseEntity*> temp;
			gEntList.FindAllEntitiesByClassname( "info_player_american", temp );
			if( temp.Size() == 0 )
			{
				CUtlVector<CBaseEntity*> temp;
				gEntList.FindAllEntitiesByClassname( "info_player_american", temp );
				if( temp.Size() == 0 )
					return false;
				else
					spawns = temp;
			}
			else
				spawns = temp;
		}
		else if ( GetTeamNumber() == TEAM_BRITISH )
		{
			//maintain backward compatibility with HL2DM
			CUtlVector<CBaseEntity*> temp;
			gEntList.FindAllEntitiesByClassname( "info_player_british", temp );
			if( temp.Size() == 0 )
			{
				CUtlVector<CBaseEntity*> temp;
				gEntList.FindAllEntitiesByClassname( "info_player_rebel", temp );
				if( temp.Size() == 0 )
					return false;
				else
					spawns = temp;
			}
			else
				spawns = temp;
		}
		else
			return false;
	}
	else
	{
		//maintain backward compatibility with HL2DM
		CUtlVector<CBaseEntity*> temp;
		gEntList.FindAllEntitiesByClassname( "info_player_deathmatch", temp );
		if( temp.Size() == 0 )
			return false;
		else
			spawns = temp;
	}

	//pick a random spot to start attempting spawn at
	int start = random->RandomInt( 0, spawns.Size() - 1 );
	
	for( int i = start;;)
	{
		CBaseEntity *pSpot = spawns[i];
		CSpawnPoint *pSpawn = dynamic_cast<CSpawnPoint*>(pSpot);

		//check that:
		// we don't have NULLs
		// spawn must be enabled
		// it must be valid(noone blocking)
		// it mustn't be at origin(invalid according to original HL2 code)
		if( pSpot && pSpawn &&
			pSpawn->IsEnabled() &&
			g_pGameRules->IsSpawnPointValid( pSpot, this ) &&
			pSpot->GetLocalOrigin() != vec3_origin )
		{
			return true;	//all is fine. return
		}

		//loop around
		i++;
		if( i >= spawns.Size() )
			i = 0;
		else if( i == start )
			return false;	//back at start, didn't find anything
	}
} 

void CHL2MP_Player::RemoveSelfFromFlags( void )
{
	//remove ourself from any overload list on all flags

	CFlag *pFlag = NULL;
	while( (pFlag = dynamic_cast<CFlag*>( gEntList.FindEntityByClassname( pFlag, "flag" ) )) != NULL )
		pFlag->m_vOverloadingPlayers.FindAndRemove( this );
}

//BG2 - Tjoppen - HACKHACK: no more weapon_physcannon
void PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject ){}
bool PlayerHasMegaPhysCannon( void ){return false;}
void PhysCannonForceDrop( CBaseCombatWeapon *pActiveWeapon, CBaseEntity *pOnlyIfHoldingThis ){}
void PhysCannonBeginUpgrade( CBaseAnimating *pAnim ){}
bool PlayerPickupControllerIsHoldingEntity( CBaseEntity *pPickupControllerEntity, CBaseEntity *pHeldEntity ){return false;}
float PhysCannonGetHeldObjectMass( CBaseCombatWeapon *pActiveWeapon, IPhysicsObject *pHeldObject ){return 0;}
CBaseEntity *PhysCannonGetHeldEntity( CBaseCombatWeapon *pActiveWeapon ){return NULL;}
float PlayerPickupGetHeldObjectMass( CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject ){return 0;}
//