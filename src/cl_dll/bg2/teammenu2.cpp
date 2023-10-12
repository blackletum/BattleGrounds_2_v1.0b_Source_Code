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
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>

#include <stdio.h> // _snprintf define

//#include "spectatorgui.h"
#include "teammenu2.h"

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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

// void DuckMessage(const char *str); // from vgui_teamfortressviewport.cpp

using namespace vgui;

#if 0

//-----------------------------------------------------------------------------
// Purpose: left and right buttons pointing buttons
//-----------------------------------------------------------------------------
/*class CClassButton : public Button
{
public:
	CClassButton(Panel *parent, const char *panelName): Button(parent, panelName, "") {}

private:
	void ApplySchemeSettings(vgui::IScheme *pScheme)
	{
		Button::ApplySchemeSettings(pScheme);
		SetFont(pScheme->GetFont("Marlett", IsProportional()) );
	}
};*/

class CTeamButton : public vgui::Button
{
public:
	DECLARE_CLASS_SIMPLE( CTeamButton, vgui::Button );

	//CClassButton(Panel *parent, const char *panelName, const wchar_t *label ): Button(parent, panelName, label) {}
	CTeamButton(Panel *parent, const char *panelName, const char *text) : Button( parent, panelName, text ) { m_pCommand = NULL; }

	/*void ApplySchemeSettings(vgui::IScheme *pScheme)
	{
		Button::ApplySchemeSettings(pScheme);
		SetFont(pScheme->GetFont("Marlett", IsProportional()) );
	}*/

	

	void SetCommand( const char *command )
	{
		m_pCommand = command;
	}

	void OnMousePressed(MouseCode code)
	{
		GetParent()->SetVisible( false );
		SetSelected( false );
		PerformCommand();
	}

	void PerformCommand( void )
	{
		if( m_pCommand )
		{
			//engine->ServerCmd( m_pCommand );
			engine->ClientCmd( m_pCommand );
		}
	}

private:
	const char *m_pCommand;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu2::CTeamMenu2( IViewPort *pViewPort ) : Frame( NULL, PANEL_TEAMS )
{
	//m_iDuckKey = -1;
	//m_iInfantryKey = m_iOfficerKey = m_iSniperKey = -1;
	m_iLightAKey = m_iMediumAKey = m_iMediumBKey = m_iHeavyBKey = -1;
	m_iCancelKey = -1;
		
	m_pViewPort = pViewPort;

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	SetTitleBarVisible( false ); // don't draw a title bar
	SetMoveable( false );
	//SetMoveable( true );
	SetSizeable( false );
	//SetSizeable( true );
	SetProportional(true);
	//SetProportional(false);

	SetScheme("ClientScheme");

	SetSize( 640, 480 );
	SetPos( 100, 100 );
	//SetVisible( true );

	/*m_pPlayerList = new ComboBox(this, "playercombo", 10 , false);
	m_pViewOptions = new ComboBox(this, "viewcombo", 10 , false );
	m_pConfigSettings = new ComboBox(this, "settingscombo", 10 , false );	

	m_pLeftButton = new CSpecButton( this, "specprev");
	m_pLeftButton->SetText("3");
	m_pRightButton = new CSpecButton( this, "specnext");
	m_pRightButton->SetText("4");

	m_pPlayerList->SetText("");
	m_pViewOptions->SetText("#Spec_Modes");
	m_pConfigSettings->SetText("#Spec_Options");

	m_pPlayerList->SetOpenDirection( ComboBox::UP );
	m_pViewOptions->SetOpenDirection( ComboBox::UP );
	m_pConfigSettings->SetOpenDirection( ComboBox::UP );*/

	/*m_pInfantryButton = new Button( this, "infantrybutton", _T("Infantry") );
	m_pOfficerButton = new Button( this, "officebutton", _T("Officer") );
	m_pSniperButton = new Button( this, "sniperbutton", _T("Sniper") );*/

	/*m_pInfantryButton = new CClassButton( this, "infantrybutton", "1. Infantry" );
	m_pOfficerButton = new CClassButton( this, "officerbutton", "2. Officer" );
	m_pSniperButton = new CClassButton( this, "sniperbutton", "3. Sniper" );*/

	//m_pLightAButton =  m_pMediumAButton = m_pMediumBButton = m_pHeavyBButton = NULL;

	m_pLightAButton = new CTeamButton( this, "lightabutton", "1. American Light" );
	m_pMediumAButton = new CTeamButton( this, "mediumabutton", "2. American Medium" );
	m_pMediumBButton = new CTeamButton( this, "mediumbbutton", "3. Britih Medium" );
	m_pHeavyBButton = new CTeamButton( this, "heavybbutton", "4. British Heavy" );

	m_pCancelButton = new CTeamButton( this, "cancelbutton", "0. Cancel" );

	//Msg( "sizeof(CTeamMenu2) = %i\n", sizeof(*this) );

	/*m_pInfantryButton->SetPos( 50, 50 );
	m_pInfantryButton->SetSize( 150, 30 );
	m_pInfantryButton->SetCommand( "infantry" );

	m_pOfficerButton->SetPos( 50, 100 );
	m_pOfficerButton->SetSize( 150, 30 );
	m_pOfficerButton->SetCommand( "officer" );

	m_pSniperButton->SetPos( 50, 150 );
	m_pSniperButton->SetSize( 150, 30 );
	m_pSniperButton->SetCommand( "sniper" );*/

	
	m_pLightAButton->SetPos( 50, 50 );
	m_pLightAButton->SetSize( 180, 30 );
	m_pLightAButton->SetCommand( "cl_playermodel models/humans/light_a/light_a.mdl" );

	m_pMediumAButton->SetPos( 50, 100 );
	m_pMediumAButton->SetSize( 180, 30 );
	m_pMediumAButton->SetCommand( "cl_playermodel models/humans/medium_a/medium_a.mdl" );

	m_pMediumBButton->SetPos( 250, 50 );
	m_pMediumBButton->SetSize( 180, 30 );
	m_pMediumBButton->SetCommand( "cl_playermodel models/medium_b/medium_b.mdl" );

	m_pHeavyBButton->SetPos( 250, 100 );
	m_pHeavyBButton->SetSize( 180, 30 );
	m_pHeavyBButton->SetCommand( "cl_playermodel models/heavy_b/heavy_b.mdl" );

	m_pCancelButton->SetPos( 50, 175 );
	m_pCancelButton->SetSize( 150, 30 );

	// create view config menu
	/*CommandMenu * menu = new CommandMenu(m_pViewOptions, "spectatormenu", gViewPortInterface);
	menu->LoadFromFile( "Resource/spectatormenu.res" );
	m_pConfigSettings->SetMenu( menu );	// attach menu to combo box

	// create view mode menu
	menu = new CommandMenu(m_pViewOptions, "spectatormodes", gViewPortInterface);
	menu->LoadFromFile("Resource/spectatormodes.res");
	m_pViewOptions->SetMenu( menu );	// attach menu to combo box

	LoadControlSettings("Resource/UI/BottomSpectator.res");*/
}

void CTeamMenu2::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// need to MakeReadyForUse() on the menus so we can set their bg color before they are displayed
	/*m_pConfigSettings->GetMenu()->MakeReadyForUse();
	m_pViewOptions->GetMenu()->MakeReadyForUse();
	m_pPlayerList->GetMenu()->MakeReadyForUse();

	m_pConfigSettings->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
	m_pViewOptions->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
	m_pPlayerList->GetMenu()->SetBgColor( BLACK_BAR_COLOR );*/

	/*m_pInfantryButton->MakeReadyForUse();
	m_pOfficerButton->MakeReadyForUse();
	m_pSniperButton->MakeReadyForUse();

	m_pInfantryButton->SetBgColor( BLACK_BAR_COLOR );
	m_pOfficerButton->SetBgColor( BLACK_BAR_COLOR );
	m_pSniperButton->SetBgColor( BLACK_BAR_COLOR );*/

	m_pLightAButton->MakeReadyForUse();
	m_pMediumAButton->MakeReadyForUse();
	m_pMediumBButton->MakeReadyForUse();
	m_pHeavyBButton->MakeReadyForUse();

	m_pLightAButton->SetBgColor( BLACK_BAR_COLOR );
	m_pMediumAButton->SetBgColor( BLACK_BAR_COLOR );
	m_pMediumBButton->SetBgColor( BLACK_BAR_COLOR );
	m_pHeavyBButton->SetBgColor( BLACK_BAR_COLOR );

	m_pCancelButton->MakeReadyForUse();
	m_pCancelButton->SetBgColor( BLACK_BAR_COLOR );
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CTeamMenu2::PerformLayout()
{
	/*int w,h;
	surface()->GetScreenSize(w, h);

	// fill the screen
	SetSize(w,GetTall());*/
}


//-----------------------------------------------------------------------------
// Purpose: Handles changes to combo boxes
//-----------------------------------------------------------------------------
/*void CTeamMenu2::OnTextChanged(KeyValues *data)
{
	Panel *panel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );

	vgui::ComboBox *box = dynamic_cast<vgui::ComboBox *>( panel );

	if( box == m_pConfigSettings) // don't change the text in the config setting combo
	{
		m_pConfigSettings->SetText("#Spec_Options");
	}
	else if ( box == m_pPlayerList )
	{
		KeyValues *kv = box->GetActiveItemUserData();
		if ( kv && GameResources() )
		{
			const char *player = kv->GetString("player");

			int currentPlayerNum = GetSpectatorTargetPlayer();
			const char *currentPlayerName = GameResources()->GetPlayerName( currentPlayerNum );

			if ( !FStrEq( currentPlayerName, player ) )
			{
				char command[128];
				Q_snprintf( command, sizeof(command), "spec_player \"%s\"", player );
				engine->ClientCmd( command );
			}
		}
	}
}*/

/*void CTeamMenu2::OnCommand( const char *command )
{
	if (!stricmp(command, "specnext") )
	{
		engine->ClientCmd("spec_next");
	}
	else if (!stricmp(command, "specprev") )
	{
		engine->ClientCmd("spec_prev");
	}
}*/


//-----------------------------------------------------------------------------
// Purpose: when duck is pressed it hides the active part of the GUI
//-----------------------------------------------------------------------------
void CTeamMenu2::OnKeyCodePressed(KeyCode code)
{
	// we can't compare the keycode to a known code, because translation from bound keys
	// to vgui key codes is not 1:1. Get the engine version of the key for the binding
	// and the actual pressed key, and compare those..
	int iLastTrappedKey = engine->GetLastPressedEngineKey();	// the enginekey version of the code param

	/*if( iLastTrappedKey == m_iDuckKey )
	{
		// hide if DUCK is pressed again
		m_pViewPort->ShowPanel( this, false );
	}*/

	/*if( iLastTrappedKey == m_iInfantryKey )
	{
		m_pInfantryButton->PerformCommand();
		//engine->ServerCmd( "infantry" );
		m_pViewPort->ShowPanel( this, false );
	}
	else if( iLastTrappedKey == m_iOfficerKey )
	{
		m_pOfficerButton->PerformCommand();
		//engine->ServerCmd( "officer" );
		m_pViewPort->ShowPanel( this, false );
	}
	else if( iLastTrappedKey == m_iSniperKey )
	{
		m_pSniperButton->PerformCommand();
		//engine->ServerCmd( "sniper" );
		m_pViewPort->ShowPanel( this, false );
	}*/

	if( iLastTrappedKey == m_iLightAKey )
	{
		m_pLightAButton->PerformCommand();
		m_pViewPort->ShowPanel( this, false );
	}
	else if( iLastTrappedKey == m_iMediumAKey )
	{
		m_pMediumAButton->PerformCommand();
		m_pViewPort->ShowPanel( this, false );
	}
	else if( iLastTrappedKey == m_iMediumBKey )
	{
		m_pMediumBButton->PerformCommand();
		m_pViewPort->ShowPanel( this, false );
	}
	else if( iLastTrappedKey == m_iHeavyBKey )
	{
		m_pHeavyBButton->PerformCommand();
		m_pViewPort->ShowPanel( this, false );
	}
	else if( iLastTrappedKey == m_iCancelKey )
	{
		m_pViewPort->ShowPanel( this, false );
	}
	else
		BaseClass::OnKeyCodePressed( code );
}

void CTeamMenu2::ShowPanel(bool bShow)
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

	/*m_pInfantryButton->SetVisible( bShow );
	m_pOfficerButton->SetVisible( bShow );
	m_pSniperButton->SetVisible( bShow );*/

	m_pLightAButton->SetVisible( bShow );
	m_pMediumAButton->SetVisible( bShow );
	m_pMediumBButton->SetVisible( bShow );
	m_pHeavyBButton->SetVisible( bShow );

	m_pCancelButton->SetVisible( bShow );

	// during HLTV broadcast, some interacitve elements are disabled
	/*if ( engine->IsHLTV() && engine->IsPlayingDemo() )
	{
		m_pLeftButton->SetVisible( true );
		m_pRightButton->SetVisible( true );
		m_pPlayerList->SetVisible( true );
		m_pViewOptions->SetVisible( true );
	}
	else
	{
		m_pLeftButton->SetVisible( false );
		m_pRightButton->SetVisible( false );
		m_pPlayerList->SetVisible( false );
		m_pViewOptions->SetVisible( false );
	}*/
}


/*int CTeamMenu2::PlayerAddItem( int itemID, wchar_t *name, KeyValues *data ) 
{
	if ( m_pPlayerList->IsItemIDValid( itemID ) )
	{	
		m_pPlayerList->UpdateItem( itemID, name, data );
		return itemID + 1;
	}
	else
	{
		return m_pPlayerList->AddItem( name, data ) + 1; 
	}
}*/

/*
void CBottomBar::OnKeyCodePressed(KeyCode code)
{
	if ( m_iChooseTeamKey!=KEY_NONE && m_iChooseTeamKey == code )
	{
		// gViewPortInterface->GetSpectatorInterface()->HideGUI();

		if( m_bChooseTeamCommand )
		{
			engine->ClientCmd("chooseteam");
		}
		else
		{
			engine->ClientCmd("changeteam");
		}
	}
	else
	{
		Panel::OnKeyCodePressed( code ); // chain this to our parent, note its meant to be Panel:: NOT BaseClass:: !!
										 // (as the baseclass is a frame which doesn't chain up)
	}
} */


/*void CTeamMenu2::SetPlayerNameText(const wchar_t *text )
{
	char *ansiText = (char *) _alloca( (wcslen( text ) + 1) * sizeof( char ) );
	if ( ansiText )
	{
		localize()->ConvertUnicodeToANSI( text, ansiText, (wcslen( text ) + 1) * sizeof( char ) );
		m_pPlayerList->SetText( ansiText );
	}
}*/

//-----------------------------------------------------------------------------
// main spectator panel



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
/*CSpectatorGUI::CSpectatorGUI(IViewPort *pViewPort) : Frame( NULL, PANEL_SPECGUI )
{
// 	m_bHelpShown = false;
//	m_bInsetVisible = false;
//	m_iDuckKey = KEY_NONE;
	m_bSpecScoreboard = false;

	m_pViewPort = pViewPort;

	// initialize dialog
	SetVisible(false);
	SetTitle("SpectatorGUI", true);
	SetProportional(true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pTopBar = new Panel( this, "topbar" );
 	m_pBottomBarBlank = new Panel( this, "bottombarblank" );

	// m_pBannerImage = new ImagePanel( m_pTopBar, NULL );
	m_pPlayerLabel = new Label( this, "playerlabel", "" );
	m_pPlayerLabel->SetVisible( false );

	LoadControlSettings("Resource/UI/Spectator.res");
	
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	m_pBottomBarBlank->SetVisible( true );
	m_pTopBar->SetVisible( true );

	// m_pBannerImage->SetVisible(false);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpectatorGUI::~CSpectatorGUI()
{
	
}

//-----------------------------------------------------------------------------
// Purpose: Sets the colour of the top and bottom bars
//-----------------------------------------------------------------------------
void CSpectatorGUI::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent
	m_pTopBar->SetBgColor(BLACK_BAR_COLOR);
	m_pBottomBarBlank->SetBgColor(BLACK_BAR_COLOR);
	// m_pBottomBar->SetBgColor(Color( 0,0,0,0 ));
	SetPaintBorderEnabled(false);

	SetBorder( NULL );

}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorGUI::PerformLayout()
{
	int w,h;
	surface()->GetScreenSize(w, h);

	// fill the screen
	SetBounds(0,0,w,h);

	// stretch the bottom bar across the screen
	m_pBottomBarBlank->SetSize(w,GetTall());
}

//-----------------------------------------------------------------------------
// Purpose: checks spec_scoreboard cvar to see if the scoreboard should be displayed
//-----------------------------------------------------------------------------
void CSpectatorGUI::OnThink()
{
	BaseClass::OnThink();

	if ( IsVisible() )
	{
		if ( m_bSpecScoreboard != spec_scoreboard.GetBool() )
		{
			if ( !spec_scoreboard.GetBool() || !gViewPortInterface->GetActivePanel() )
			{
				m_bSpecScoreboard = spec_scoreboard.GetBool();
				gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, m_bSpecScoreboard );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the image to display for the banner in the top right corner
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLogoImage(const char *image)
{
	if ( m_pBannerImage )
	{
		m_pBannerImage->SetImage( scheme()->GetImage(image, false) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLabelText(const char *textEntryName, wchar_t *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CSpectatorGUI::ShowPanel(bool bShow)
{
	if ( bShow && !IsVisible() )
	{
		m_bSpecScoreboard = false;
	}
	SetVisible( bShow );
	if ( !bShow && m_bSpecScoreboard )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates the gui, rearragnes elements
//-----------------------------------------------------------------------------
void CSpectatorGUI::Update()
{
	int wide, tall;
	int bx, by, bwide, btall;

	surface()->GetScreenSize(wide, tall);
	m_pTopBar->GetBounds( bx, by, bwide, btall );

	IGameResources *gr = GameResources();
	int specmode = GetSpectatorMode();
	int playernum = GetSpectatorTargetPlayer();

	IViewPortPanel *overview = gViewPortInterface->FindPanelByName( PANEL_OVERVIEW );

	if ( overview && overview->IsVisible() )
	{
		int mx, my, mwide, mtall;

		VPANEL p = overview->GetVPanel();
		vgui::ipanel()->GetPos( p, mx, my );
		vgui::ipanel()->GetSize( p, mwide, mtall );
				
		if ( my < btall )
		{
			// reduce to bar 
			m_pTopBar->SetSize( wide - (mx + mwide), btall );
			m_pTopBar->SetPos( (mx + mwide), 0 );
		}
		else
		{
			// full top bar
			m_pTopBar->SetSize( wide , btall );
			m_pTopBar->SetPos( 0, 0 );
		}
	}
	else
	{
		// full top bar
		m_pTopBar->SetSize( wide , btall ); // change width, keep height
		m_pTopBar->SetPos( 0, 0 );
	}

	m_pPlayerLabel->SetVisible( (specmode == OBS_MODE_IN_EYE) || (specmode == OBS_MODE_CHASE) );

	// update player name filed, text & color

	if ( playernum > 0 && gr)
	{
		Color c = gr->GetTeamColor( gr->GetTeam(playernum) ); // Player's team color

		m_pPlayerLabel->SetFgColor( c );
		
		wchar_t playerText[ 80 ], playerName[ 64 ], health[ 10 ];
		wcscpy( playerText, L"Unable to find #Spec_PlayerItem*" );
		memset( playerName, 0x0, sizeof( playerName ) * sizeof( wchar_t ) );

		localize()->ConvertANSIToUnicode( gr->GetPlayerName( playernum ), playerName, sizeof( playerName ) );
		int iHealth = gr->GetHealth( playernum );
		if ( iHealth > 0  && gr->IsAlive(playernum) )
		{
			_snwprintf( health, sizeof( health ), L"%i", iHealth );
			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem_Team" ), 2, playerName,  health );
		}
		else
		{
			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem" ), 1, playerName );
		}

		m_pPlayerLabel->SetText( playerText );
	}

	// update extra info field
	wchar_t string1[1024];
/*	if ( gViewPortInterface->GetClientDllInterface()->IsHLTVMode() ) TODO
	{
		char numplayers[6];
		wchar_t wNumPlayers[6];

		Q_snprintf(numplayers,6,"%i", 666); // TODO show HLTV spectator number
		localize()->ConvertANSIToUnicode(numplayers,wNumPlayers,sizeof(wNumPlayers));
		localize()->ConstructString( string1,sizeof( string1 ), localize()->Find("#Spectators" ),1, wNumPlayers );
	}
	else *//*
	{
		// otherwise show map name
		char mapname[255];
		Q_FileBase( engine->GetLevelName(), mapname, sizeof(mapname) );

		wchar_t wMapName[64];
		localize()->ConvertANSIToUnicode(mapname,wMapName,sizeof(wMapName));
		localize()->ConstructString( string1,sizeof( string1 ), localize()->Find("#Spec_Map" ),1, wMapName );
	}

	SetLabelText("extrainfo", string1 );
}*/


//-----------------------------------------------------------------------------
// Purpose: Resets the list of players
//-----------------------------------------------------------------------------
void CTeamMenu2::Update( void )
{
	//int itemID = 0;

	/*IGameResources *gr = GameResources();

	Reset();

	/*if( m_iDuckKey < 0 )
	{
		m_iDuckKey = gameuifuncs->GetEngineKeyCodeForBind( "duck" );
	}*/

	/*if( m_iInfantryKey < 0 ) m_iInfantryKey = gameuifuncs->GetEngineKeyCodeForBind( "slot1" );
	if( m_iOfficerKey < 0 ) m_iOfficerKey = gameuifuncs->GetEngineKeyCodeForBind( "slot2" );
	if( m_iSniperKey < 0 ) m_iSniperKey = gameuifuncs->GetEngineKeyCodeForBind( "slot3" );
	//*/

	if( m_iLightAKey < 0 ) m_iLightAKey = gameuifuncs->GetEngineKeyCodeForBind( "slot1" );
	if( m_iMediumAKey < 0 ) m_iMediumAKey = gameuifuncs->GetEngineKeyCodeForBind( "slot2" );
	if( m_iMediumBKey < 0 ) m_iMediumBKey = gameuifuncs->GetEngineKeyCodeForBind( "slot3" );
	if( m_iHeavyBKey < 0 ) m_iHeavyBKey = gameuifuncs->GetEngineKeyCodeForBind( "slot4" );

	if( m_iCancelKey < 0 ) m_iCancelKey = gameuifuncs->GetEngineKeyCodeForBind( "slot10" );
	
	/*if ( !gr )
		return;

	/*int iPlayerIndex;
	for ( iPlayerIndex = 1 ; iPlayerIndex <= gpGlobals->maxClients; iPlayerIndex++ )
	{

		// does this slot in the array have a name?
		if ( !gr->IsConnected( iPlayerIndex ) )
			continue;
			
		if ( gr->IsLocalPlayer( iPlayerIndex ) )
			continue;

		if ( !gr->IsAlive( iPlayerIndex ) )
			continue;

		wchar_t playerText[ 80 ], playerName[ 64 ], *team, teamText[ 64 ];
		char localizeTeamName[64];
		localize()->ConvertANSIToUnicode( gr->GetPlayerName(iPlayerIndex), playerName, sizeof( playerName ) );
		const char * teamname = gr->GetTeamName( gr->GetTeam(iPlayerIndex) );
		if ( teamname )
		{	
			Q_snprintf( localizeTeamName, sizeof( localizeTeamName ), "#%s", teamname );
			team=localize()->Find( localizeTeamName );

			if ( !team ) 
			{
				localize()->ConvertANSIToUnicode( teamname , teamText, sizeof( teamText ) );
				team = teamText;
			}

			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem_Team" ), 2, playerName, team );
		}
		else
		{
			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem" ), 1, playerName );
		}

		KeyValues *kv = new KeyValues("UserData", "player", gr->GetPlayerName( iPlayerIndex ) );
		itemID = PlayerAddItem( itemID, playerText, kv ); // -1 means a new slot
		kv->deleteThis();
	}

	// make sure the player combo box is up to date
	int playernum = GetSpectatorTargetPlayer();
	const char *selectedPlayerName = gr->GetPlayerName( playernum );
	for ( iPlayerIndex=0; iPlayerIndex<m_pPlayerList->GetItemCount(); ++iPlayerIndex )
	{
		KeyValues *kv = m_pPlayerList->GetItemUserData( iPlayerIndex );
		if ( kv && FStrEq( kv->GetString( "player" ), selectedPlayerName ) )
		{
			m_pPlayerList->ActivateItemByRow( iPlayerIndex );
			m_pPlayerList->SetText( selectedPlayerName );
			break;
		}
	}*/
}

/*void CTeamMenu2::OnThink()
{
	BaseClass::OnThink();

	IGameResources *gr = GameResources();
	if ( !gr )
		return;

	// make sure the player combo box is up to date
	int playernum = GetSpectatorTargetPlayer();
	const char *selectedPlayerName = gr->GetPlayerName( playernum );
	const char *currentPlayerName = "";
	KeyValues *kv = m_pPlayerList->GetActiveItemUserData();
	if ( kv )
	{
		currentPlayerName = kv->GetString("player");
	}
	if ( !FStrEq( currentPlayerName, selectedPlayerName ) )
	{
		for ( int i=0; i<m_pPlayerList->GetItemCount(); ++i )
		{
			KeyValues *kv = m_pPlayerList->GetItemUserData( i );
			if ( kv && FStrEq( kv->GetString( "player" ), selectedPlayerName ) )
			{
				m_pPlayerList->ActivateItemByRow( i );
				m_pPlayerList->SetText( selectedPlayerName );
				break;
			}
		}
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: Updates the timer label if one exists
//-----------------------------------------------------------------------------
/*void CSpectatorGUI::UpdateTimer()
{
	wchar_t szText[ 63 ];

	int timer = 0;

	_snwprintf ( szText, sizeof( szText ), L"%d:%02d\n", (timer / 60), (timer % 60) );

	szText[63] = 0;


	SetLabelText("timerlabel", szText );
}

static void ForwardSpecCmdToServer()
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( engine->Cmd_Argc() == 1 )
	{
		// just forward the command without parameters
		engine->ServerCmd( engine->Cmd_Argv(0) );
	}
	else if ( engine->Cmd_Argc() == 2 )
	{
		// forward the command with parameter
		char command[128];
		Q_snprintf( command, sizeof(command), "%s \"%s\"", engine->Cmd_Argv(0), engine->Cmd_Argv(1) );
		engine->ServerCmd( command );
	}
}

CON_COMMAND( spec_next, "Spectate next player" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() && engine->IsPlayingDemo() )
	{
		// handle the command clientside
		HLTVCamera()->SpecNextPlayer( false );
	}
	else
	{
		ForwardSpecCmdToServer();
	}
}

CON_COMMAND( spec_prev, "Spectate previous player" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() && engine->IsPlayingDemo() )
	{
		// handle the command clientside
		HLTVCamera()->SpecNextPlayer( true );
	}
	else
	{
		ForwardSpecCmdToServer();
	}
}

CON_COMMAND( spec_mode, "Set spectator mode" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	int mode = 0;

	if ( engine->Cmd_Argc() == 2 )
		mode = Q_atoi( engine->Cmd_Argv(1) );

	if ( engine->IsHLTV() && engine->IsPlayingDemo() )
	{
		// handle the command clientside
		HLTVCamera()->SetMode( mode );
	}
	else
	{
		ForwardSpecCmdToServer();
	}
}

CON_COMMAND( spec_player, "Spectate player by name" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->Cmd_Argc() != 2 )
		return;

	if ( engine->IsHLTV() && engine->IsPlayingDemo() )
	{
		// handle the command clientside
		HLTVCamera()->SpecNamedPlayer( engine->Cmd_Argv(1) );
	}
	else
	{
		ForwardSpecCmdToServer();
	}
}*/



#endif