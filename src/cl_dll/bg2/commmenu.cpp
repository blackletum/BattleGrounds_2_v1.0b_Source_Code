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
#include <vgui_controls/Label.h>

#include <stdio.h> // _snprintf define

//#include "spectatorgui.h"
#include "commmenu.h"
#include "../dlls/bg2/vcomm.h"
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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

CCommMenu::CCommMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_COMM )
{
	slot1 = slot2 = slot3 = slot4 = slot5 = slot6 = slot7 = slot8 = /*slot9 =*/ slot10 = -1;
	teammenu = classmenu = commmenu = commmenu2 = -1;
		
	m_pViewPort = pViewPort;

	SetMouseInputEnabled( false );
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

	int w,h;
	surface()->GetScreenSize(w, h);

	SetPos( 50, h - GetTall() );
	SetAlpha( 0 );
	SetPaintBackgroundEnabled( false );
	SetBorder( NULL );
	//SetVisible( true );

	char text[512];
	strcpy( text, "" );
	for( int x = 0; x < VCOMM1_NUM; x++ )
	{
		strcat( text, pVComms[x] );
		strcat( text, "\n" );
	}

	strcat( text, "\n" );
	strcat( text, pVComms[VCOMM1_NUM] );
	strcat( text, "\n" );

	m_pLabel = new Label( this, "label", text );
	m_pLabel->SetPos( 50, 50 );
	m_pLabel->SizeToContents();
}

void CCommMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int w,h;
	surface()->GetScreenSize(w, h);

	SetPos( 50, h - GetTall() );
}

void CCommMenu::PerformLayout()
{
}

void CCommMenu::OnKeyCodePressed(KeyCode code)
{
	// we can't compare the keycode to a known code, because translation from bound keys
	// to vgui key codes is not 1:1. Get the engine version of the key for the binding
	// and the actual pressed key, and compare those..
	int iLastTrappedKey = engine->GetLastPressedEngineKey();	// the enginekey version of the code param

#define ifkey( index, index2 ) if( iLastTrappedKey == slot##index ) { engine->ServerCmd( "voicecomm " #index2 ); ShowPanel( false ); }
			ifkey( 1, 0 )
	else	ifkey( 2, 1 )
	else	ifkey( 3, 2 )
	else	ifkey( 4, 3 )
	else	ifkey( 5, 4 )
	else	ifkey( 6, 5 )
	else	ifkey( 7, 6 )
	else	ifkey( 8, 7 )
	else if( iLastTrappedKey == slot10 )
		m_pViewPort->ShowPanel( this, false );
	else if( iLastTrappedKey == teammenu )
	{
		m_pViewPort->ShowPanel( PANEL_CLASSES, true );
		m_pViewPort->ShowPanel( PANEL_COMM, false );
		m_pViewPort->ShowPanel( PANEL_COMM2, false );
		
		IViewPortPanel *panel = gViewPortInterface->FindPanelByName( PANEL_CLASSES );
		if( panel )
			panel->SetData( (KeyValues*)0 );	//HACKHACK

		return;
	}
	else if( iLastTrappedKey == classmenu )
	{
		if( C_BasePlayer::GetLocalPlayer()->GetTeamNumber() > TEAM_SPECTATOR )
		{
			m_pViewPort->ShowPanel( PANEL_CLASSES, true );

			IViewPortPanel *panel = gViewPortInterface->FindPanelByName( PANEL_CLASSES );
			if( panel )
				panel->SetData( (KeyValues*)1 );	//HACKHACK
		}
		else
		{
			internalCenterPrint->Print( "You can\'t select class before selecting team" );
			m_pViewPort->ShowPanel( PANEL_CLASSES, false );
		}

		m_pViewPort->ShowPanel( PANEL_COMM, false );
		m_pViewPort->ShowPanel( PANEL_COMM2, false );

		return;
	}
	else if( iLastTrappedKey == commmenu )
	{
		m_pViewPort->ShowPanel( PANEL_CLASSES, false );
		m_pViewPort->ShowPanel( PANEL_COMM, false );
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

void CCommMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}

	m_pLabel->SetVisible( bShow );
}

void CCommMenu::Update( void )
{
#define getkey( index ) if( slot##index < 0 ) slot##index = gameuifuncs->GetEngineKeyCodeForBind( "slot" #index )
	getkey( 1 );
	getkey( 2 );
	getkey( 3 );
	getkey( 4 );
	getkey( 5 );
	getkey( 6 );
	getkey( 7 );
	getkey( 8 );
	getkey( 10 );

	if( teammenu < 0 ) teammenu = gameuifuncs->GetEngineKeyCodeForBind( "teammenu" );
	if( classmenu < 0 ) classmenu = gameuifuncs->GetEngineKeyCodeForBind( "classmenu" );
	if( commmenu < 0 ) commmenu = gameuifuncs->GetEngineKeyCodeForBind( "commmenu" );
	if( commmenu2 < 0 ) commmenu2 = gameuifuncs->GetEngineKeyCodeForBind( "commmenu2" );
}
