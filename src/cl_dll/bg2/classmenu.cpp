//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
#include <globalvars_base.h>
#include <cdll_util.h>
#include <KeyValues.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
/*#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>*/

#include <stdio.h> // _snprintf define

//#include "spectatorgui.h"
#include "c_team.h"
#include "vguicenterprint.h"

#include <cl_dll/iviewport.h>
#include "commandmenu.h"
#include "hltvcamera.h"

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include <igameresources.h>

#include "classmenu.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details



using namespace vgui;

//
//	CClassButton
//
void CClassButton::SetCommand( int command )
{
	m_iCommand = command;
}

void CClassButton::OnMousePressed(MouseCode code)
{
	CClassMenu * pThisMenu = (CClassMenu *)GetParent();
	pThisMenu->ToggleButtons(1);
	GetParent()->SetVisible( false );
	SetSelected( false );
	PerformCommand();
}

void CClassButton::OnCursorEntered( void )
{
	BaseClass::OnCursorEntered();

	CClassMenu *pThisMenu = (CClassMenu*)GetParent();
	const wchar_t *pPath = NULL;

	if( m_iCommand == 1 )
	{
		//officer
		if( pThisMenu->m_iTeamSelection == TEAM_AMERICANS )
			pPath = vgui::localize()->Find("#BG2_InfoHTML_A_Off_Path");
		else if( pThisMenu->m_iTeamSelection == TEAM_BRITISH )
			pPath = vgui::localize()->Find("#BG2_InfoHTML_B_Off_Path");
	}
	else if( m_iCommand == 2 )
	{
		//infantry
		if( pThisMenu->m_iTeamSelection == TEAM_AMERICANS )
			pPath = vgui::localize()->Find("#BG2_InfoHTML_A_Inf_Path");
		else if( pThisMenu->m_iTeamSelection == TEAM_BRITISH )
			pPath = vgui::localize()->Find("#BG2_InfoHTML_B_Inf_Path");
	}
	else if( m_iCommand == 3 )
	{
		//rifleman
		if( pThisMenu->m_iTeamSelection == TEAM_AMERICANS )
			pPath = vgui::localize()->Find("#BG2_InfoHTML_A_Rif_Path");
		else if( pThisMenu->m_iTeamSelection == TEAM_BRITISH )
			pPath = vgui::localize()->Find("#BG2_InfoHTML_B_Rif_Path");
	}

	if( pPath )
	{
		char pANSIPath[512];

		vgui::localize()->ConvertUnicodeToANSI( pPath, pANSIPath, sizeof pANSIPath );
		pThisMenu->ShowFile( pANSIPath );
	}
}

void CClassButton::PerformCommand( void )
{
	CClassMenu *pThisMenu = (CClassMenu*)GetParent();

    if( m_iCommand == 1 )
	{
		//officer
		if( pThisMenu->m_iTeamSelection == TEAM_AMERICANS )
			engine->ServerCmd( "light_a" );
		else if( pThisMenu->m_iTeamSelection == TEAM_BRITISH )
			engine->ServerCmd( "light_b" );
	}
	else if( m_iCommand == 2 )
	{
		//infantry
		if( pThisMenu->m_iTeamSelection == TEAM_AMERICANS )
			engine->ServerCmd( "heavy_a" );
		else if( pThisMenu->m_iTeamSelection == TEAM_BRITISH )
			engine->ServerCmd( "medium_b" );
	}
	else if( m_iCommand == 3 )
	{
		//rifleman
		if( pThisMenu->m_iTeamSelection == TEAM_AMERICANS )
			engine->ServerCmd( "medium_a" );
		else if( pThisMenu->m_iTeamSelection == TEAM_BRITISH )
			engine->ServerCmd( "heavy_b" );
	}
}

//
//	CTeamButton
//
void CTeamButton::SetCommand( int command )
{
	m_iCommand = command;
}

void CTeamButton::OnMousePressed(MouseCode code)
{
	CClassMenu *pThisMenu = (CClassMenu *)GetParent();
	SetSelected( false );

	if( m_iCommand == TEAM_UNASSIGNED )
	{
		//join spectators
		engine->ServerCmd( "spectate", true );
		pThisMenu->ToggleButtons(1);
		pThisMenu->ShowPanel( false );
		return;
	}

	pThisMenu->ToggleButtons(2);
	
	if( m_iCommand == -1 )
	{
		//autoassign
		int americans = g_Teams[TEAM_AMERICANS]->Get_Number_Players(),
			british = g_Teams[TEAM_BRITISH]->Get_Number_Players();

		//pick team with least players. or if equal, pick random
		if( americans > british )
			pThisMenu->m_iTeamSelection = TEAM_BRITISH;
		else if( americans < british )
			pThisMenu->m_iTeamSelection = TEAM_AMERICANS;
		else
			pThisMenu->m_iTeamSelection = random->RandomInt( TEAM_AMERICANS, TEAM_BRITISH );
	
		return;
	}

	PerformCommand();
}

void CTeamButton::OnCursorEntered( void )
{
	BaseClass::OnCursorEntered();
	
	CClassMenu *pThisMenu = (CClassMenu *)GetParent();
	const wchar_t *pPath = NULL;

	if( m_iCommand == TEAM_AMERICANS )
		pPath = vgui::localize()->Find("#BG2_InfoHTML_Americans_Path");
	if( m_iCommand == TEAM_BRITISH )
		pPath = vgui::localize()->Find("#BG2_InfoHTML_British_Path");
	else if( m_iCommand == TEAM_UNASSIGNED )	//spectate
		pPath = vgui::localize()->Find("#BG2_InfoHTML_Spectate_Path");
	else if( m_iCommand == -1 )					//autoassign
		pPath = vgui::localize()->Find("#BG2_InfoHTML_Autoassign_Path");
	
	if( pPath )
	{
		char pANSIPath[512];

		vgui::localize()->ConvertUnicodeToANSI( pPath, pANSIPath, sizeof pANSIPath );
		pThisMenu->ShowFile( pANSIPath );
	}
}

void CTeamButton::PerformCommand( void )
{
	CClassMenu * pThisMenu = (CClassMenu *)GetParent();
	pThisMenu->m_iTeamSelection = m_iCommand;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClassMenu::CClassMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_CLASSES )
{
	m_iInfantryKey = m_iOfficerKey = m_iSniperKey = -1;
	m_iCancelKey = -1;
	m_iSpectateKey = -1;
	teammenu = classmenu = commmenu = commmenu2 = -1;
		
	m_pViewPort = pViewPort;

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	SetTitleBarVisible( false ); // don't draw a title bar
	SetMoveable( false );
	SetSizeable( false );
	//SetProportional(true);
	SetProportional(false);

	SetScheme("ClientScheme");

	/*SetSize( 640, 480 );
	SetPos( (ScreenWidth() - 640) / 2, (ScreenHeight() - 480) / 2 );*/

	m_pBritishButton = new CTeamButton( this, "BritishButton", "1. British" );
	m_pAmericanButton = new CTeamButton( this, "AmericanButton", "2. American" );
	m_pAutoassignButton = new CTeamButton( this, "AutoassignButton", "3. Autoassign team" );
	m_pSpectateButton = new CTeamButton( this, "SpectateButton", "4. Spectate" );
	
	m_pInfantryButton = new CClassButton( this, "InfantryButton", "1. Infantry(in code)" );
	m_pOfficerButton = new CClassButton( this, "OfficerButton", "2. Officer(in code)" );
	m_pSniperButton = new CClassButton( this, "RiflemanButton", "3. Rifleman(in code)" );

	m_pCancelButton = new CClassButton( this, "CancelButton", "0. Cancel" );

	m_pInfoHTML = new HTML( this, "InfoHTML" );
	//m_pInfoHTML->SetPos( 100, 100 );
	//m_pInfoHTML->SetSize( 540, 380 );

	LoadControlSettings( "Resource/BG2Classmenu.res" );

	m_pBritishButton->SetCommand(TEAM_BRITISH);
	m_pAmericanButton->SetCommand(TEAM_AMERICANS);
	m_pAutoassignButton->SetCommand(-1);
	m_pSpectateButton->SetCommand(TEAM_UNASSIGNED);

	m_pInfantryButton->SetCommand( 2 );
	m_pOfficerButton->SetCommand( 1 );
	m_pSniperButton->SetCommand( 3 );

	//ShowFile( "readme.txt" );

	ToggleButtons( 1 );
}

void CClassMenu::ShowFile( const char *filename )
{
	char localURL[_MAX_PATH + 7];
	Q_strncpy( localURL, "file://", sizeof( localURL ) );
		
	char pPathData[ _MAX_PATH ];
	vgui::filesystem()->GetLocalPath( filename, pPathData, sizeof(pPathData) );
	Q_strncat( localURL, pPathData, sizeof( localURL ), COPY_ALL_CHARACTERS );

	//Msg( "ShowFile(%s)\n", localURL );

	m_pInfoHTML->OpenURL( localURL );
}

void CClassMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetSize( 640, 480 );
	SetPos( (ScreenWidth() - 640) / 2, (ScreenHeight() - 480) / 2 );

	m_pAmericanButton->MakeReadyForUse();
	m_pBritishButton->MakeReadyForUse();
	m_pAmericanButton->SetBgColor( BLACK_BAR_COLOR );
	m_pBritishButton->SetBgColor( BLACK_BAR_COLOR );

	m_pInfantryButton->MakeReadyForUse();
	m_pOfficerButton->MakeReadyForUse();
	m_pSniperButton->MakeReadyForUse();

	m_pInfantryButton->SetBgColor( BLACK_BAR_COLOR );
	m_pOfficerButton->SetBgColor( BLACK_BAR_COLOR );
	m_pSniperButton->SetBgColor( BLACK_BAR_COLOR );

	m_pCancelButton->MakeReadyForUse();
	m_pCancelButton->SetBgColor( BLACK_BAR_COLOR );
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CClassMenu::PerformLayout()
{
}


//-----------------------------------------------------------------------------
// Purpose: when duck is pressed it hides the active part of the GUI
//-----------------------------------------------------------------------------
void CClassMenu::OnKeyCodePressed(KeyCode code)
{

//	C_BasePlayer *pPlayer = (C_BasePlayer *) C_BasePlayer::GetLocalPlayer();


	// we can't compare the keycode to a known code, because translation from bound keys
	// to vgui key codes is not 1:1. Get the engine version of the key for the binding
	// and the actual pressed key, and compare those..
	int iLastTrappedKey = engine->GetLastPressedEngineKey();	// the enginekey version of the code param

	MouseCode code2 = MOUSE_LEFT;	//for faking mouse presses
	if( iLastTrappedKey == m_iInfantryKey )
	{
		if( m_pBritishButton->IsVisible() )
			m_pBritishButton->OnMousePressed( code2 );
		else
		{
			m_pInfantryButton->OnMousePressed( code2 );
			m_pViewPort->ShowPanel( this, false );
		}
	}
	else if( iLastTrappedKey == m_iOfficerKey )
	{
		if( m_pAmericanButton->IsVisible() )
			m_pAmericanButton->OnMousePressed( code2 );
		else
		{
			m_pOfficerButton->OnMousePressed( code2 );
			m_pViewPort->ShowPanel( this, false );
		}
	}
	else if( iLastTrappedKey == m_iSniperKey )
	{
		if( m_pAutoassignButton->IsVisible() )
			m_pAutoassignButton->OnMousePressed( code2 );
		else if( m_pSniperButton->IsVisible() )
		{
			m_pSniperButton->OnMousePressed( code2 );
			m_pViewPort->ShowPanel( this, false );
		}
	}
	else if( iLastTrappedKey == m_iSpectateKey )
	{
		if( m_pSpectateButton->IsVisible() )
		{
			m_pSpectateButton->OnMousePressed( code2 );
			m_pViewPort->ShowPanel( this, false );
		}
	}
	else if( iLastTrappedKey == m_iCancelKey )
	{
		ToggleButtons(1);
		m_pViewPort->ShowPanel( this, false );
	}
	else if( iLastTrappedKey == teammenu )
	{
		if( m_pBritishButton->IsVisible() )
		{
			m_pViewPort->ShowPanel( PANEL_CLASSES, false );
			m_pViewPort->ShowPanel( PANEL_COMM, false );
			m_pViewPort->ShowPanel( PANEL_COMM2, false );
		}
		else
			ToggleButtons( 1 );
		
		return;
	}
	else if( iLastTrappedKey == classmenu )
	{
		if( IsInClassMenu() )
		{
			m_pViewPort->ShowPanel( PANEL_CLASSES, false );
			m_pViewPort->ShowPanel( PANEL_COMM, false );
			m_pViewPort->ShowPanel( PANEL_COMM2, false );
		}
		else if( !IsInClassMenu() && C_BasePlayer::GetLocalPlayer()->GetTeamNumber() <= TEAM_SPECTATOR )
		{
			internalCenterPrint->Print( "You can\'t select class before selecting team" );
			m_pViewPort->ShowPanel( PANEL_CLASSES, false );
			m_pViewPort->ShowPanel( PANEL_COMM, false );
			m_pViewPort->ShowPanel( PANEL_COMM2, false );
		}
		else
			ToggleButtons( 2 );
		
		return;
	}
	else if( iLastTrappedKey == commmenu )
	{
		m_pViewPort->ShowPanel( PANEL_CLASSES, false );
		m_pViewPort->ShowPanel( PANEL_COMM, true );
		m_pViewPort->ShowPanel( PANEL_COMM2, false );
		
		return;
	}
	else if( iLastTrappedKey == commmenu2 )
	{
		m_pViewPort->ShowPanel( PANEL_CLASSES, false );
		m_pViewPort->ShowPanel( PANEL_COMM, false );
		m_pViewPort->ShowPanel( PANEL_COMM2, true );
		
		return;
	}
	else
		BaseClass::OnKeyCodePressed( code );
}

void CClassMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}

	//m_pInfantryButton->SetVisible( bShow );
	//m_pOfficerButton->SetVisible( bShow );
	//m_pSniperButton->SetVisible( bShow );

	m_pCancelButton->SetVisible( bShow );
}





//-----------------------------------------------------------------------------
// Purpose: Resets the list of players
//-----------------------------------------------------------------------------
void CClassMenu::Update( void )
{
	if( m_iInfantryKey < 0 ) m_iInfantryKey = gameuifuncs->GetEngineKeyCodeForBind( "slot1" );
	if( m_iOfficerKey < 0 ) m_iOfficerKey = gameuifuncs->GetEngineKeyCodeForBind( "slot2" );
	if( m_iSniperKey < 0 ) m_iSniperKey = gameuifuncs->GetEngineKeyCodeForBind( "slot3" );
	if( m_iSpectateKey < 0 ) m_iSpectateKey = gameuifuncs->GetEngineKeyCodeForBind( "slot4" );

	if( m_iCancelKey < 0 ) m_iCancelKey = gameuifuncs->GetEngineKeyCodeForBind( "slot10" );

	if( teammenu < 0 ) teammenu = gameuifuncs->GetEngineKeyCodeForBind( "teammenu" );
	if( classmenu < 0 ) classmenu = gameuifuncs->GetEngineKeyCodeForBind( "classmenu" );
	if( commmenu < 0 ) commmenu = gameuifuncs->GetEngineKeyCodeForBind( "commmenu" );
	if( commmenu2 < 0 ) commmenu2 = gameuifuncs->GetEngineKeyCodeForBind( "commmenu2" );
}

void CClassMenu::OnThink()
{
	BaseClass::OnThink();
	//BG2 - Tjoppen - show autoassign button if we're not in the class part and we're unassigned or spectator
	m_pAutoassignButton->SetVisible( IsInClassMenu() ? false : C_BasePlayer::GetLocalPlayer() ? C_BasePlayer::GetLocalPlayer()->GetTeamNumber() <= TEAM_SPECTATOR : false );

	//show number of players in teams..
	char tmp[256];
	if( g_Teams[TEAM_BRITISH] )		//check just in case..
	{
		sprintf( tmp, "1. British (%i)", g_Teams[TEAM_BRITISH]->Get_Number_Players() );
		m_pBritishButton->SetText( tmp );
	}

	if( g_Teams[TEAM_AMERICANS] )	//check just in case..
	{
		sprintf( tmp, "2. American (%i)", g_Teams[TEAM_AMERICANS]->Get_Number_Players() );
		m_pAmericanButton->SetText( tmp );
	}

	//BG2 - Draco - set the proper names for the classes
	//BG2 - Tjoppen - updated to use UpdateClassButtonText()
	UpdateClassButtonText( m_pInfantryButton, CLASS_INFANTRY, 
			m_iTeamSelection == TEAM_AMERICANS ? "1. Continental Soldier" : "1. Royal Infantry" );

	UpdateClassButtonText( m_pOfficerButton, CLASS_OFFICER, 
			m_iTeamSelection == TEAM_AMERICANS ? "2. Continental Officer" : "2. Royal Commander" );

	UpdateClassButtonText( m_pSniperButton, CLASS_SNIPER, 
			m_iTeamSelection == TEAM_AMERICANS ? "3. Frontiersman" : "3. Jaeger" );
}

void CClassMenu::UpdateClassButtonText( CClassButton *pButton, int iClass, const char *pPrefix )
{
	//BG2 - Tjoppen - this function figures out what to print on the specified button's text field
	//					based on class limits and such
	char temp[512];
	int limit = HL2MPRules()->GetLimitTeamClass(m_iTeamSelection, iClass);

	if( m_iTeamSelection != TEAM_AMERICANS && m_iTeamSelection != TEAM_BRITISH )
		return;

	C_Team *pCurrentTeam = g_Teams[m_iTeamSelection];

	if( !pCurrentTeam )
		return;

	if( limit >= 0 )
		Q_snprintf( temp, sizeof temp, "%s (%i/%i)", pPrefix, pCurrentTeam->GetNumOfClass(iClass), limit );
	else
		Q_snprintf( temp, sizeof temp, "%s (%i)", pPrefix, pCurrentTeam->GetNumOfClass(iClass) );

	pButton->SetText( temp );
}

void CClassMenu::SetData(KeyValues *data)
{
	//HACKHACK
	if( (int)data == 1 )
		ToggleButtons(2);
	else if( (int)data == 0 )
		ToggleButtons(1);
}

void CClassMenu::ToggleButtons(int iShowScreen)
{
	switch (iShowScreen)
	{
		case 1:
			m_pBritishButton->SetVisible(true);
			m_pAmericanButton->SetVisible(true);
			//BG2 - Tjoppen - show autoassign button if we're not in the class part and we're unassigned or spectator
			m_pAutoassignButton->SetVisible( IsInClassMenu() ? false : C_BasePlayer::GetLocalPlayer() ? C_BasePlayer::GetLocalPlayer()->GetTeamNumber() <= TEAM_SPECTATOR : false );
			//
			m_pSpectateButton->SetVisible(true);
			m_pOfficerButton->SetVisible(false);
			m_pInfantryButton->SetVisible(false);
			m_pSniperButton->SetVisible(false);
			break;
		case 2:
			m_pBritishButton->SetVisible(false);
			m_pAmericanButton->SetVisible(false);
			m_pAutoassignButton->SetVisible(false);
			m_pSpectateButton->SetVisible(false);
			m_pOfficerButton->SetVisible(true);
			m_pInfantryButton->SetVisible(true);
			m_pSniperButton->SetVisible(true);
			break;
	}
}