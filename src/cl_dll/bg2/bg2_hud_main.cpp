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
		Jason "Draco" Houston		iamlorddraco@gmail.com

	You may also contact us via the Battle Grounds website and/or forum at:
		www.bgmod.com
*/
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"

// hintbox header
#include "hintbox.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//==============================================
// CHudBG2
// Displays flag status
//==============================================
class CHudBG2 : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudBG2, vgui::Panel );
public:
	CHudBG2( const char *pElementName );
	void Init( void );
	void VidInit( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void OnThink();
	void Reset( void );

	void MsgFunc_HitVerif( bf_read &msg );
	
	void HideShowAll( bool visible );

private:

	CHudTexture		* m_Base; 
	CHudTexture		* m_Straps; 
	CHudTexture		* m_Stamina;
	CHudTexture		* m_Health;

	vgui::Label * m_pLabelBScore; 
	vgui::Label * m_pLabelAScore; 
	vgui::Label * m_pLabelAmmo; 
	vgui::Label * m_pLabelWaveTime; 
	//vgui::Label * m_pLabelBGVersion; 
	vgui::Label *m_pLabelDamageVerificator,
				*m_pLabelLMS;		//BG2 - Tjoppen - TODO: remove this when hintbox works correctly

	const char* HitgroupName( int hitgroup );

	float m_flExpireTime;
};

using namespace vgui;

DECLARE_HUDELEMENT( CHudBG2 );
DECLARE_HUD_MESSAGE( CHudBG2, HitVerif );

//==============================================
// CHudBG2's CHudFlags
// Constructor
//==============================================
CHudBG2::CHudBG2( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudBG2" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_ALL );//HIDEHUD_MISCSTATUS );

	m_Base = NULL; 
	m_Stamina = NULL;
	m_Health = NULL;

	m_flExpireTime = 0;

	Color ColourWhite( 255, 255, 255, 255 );

	m_pLabelBScore = new vgui::Label( this, "RoundState_warmup", "");
	m_pLabelBScore->SetPaintBackgroundEnabled( false );
	m_pLabelBScore->SetPaintBorderEnabled( false );
	m_pLabelBScore->SizeToContents();
	m_pLabelBScore->SetContentAlignment( vgui::Label::a_west );
	m_pLabelBScore->SetFgColor( ColourWhite );

	m_pLabelAScore = new vgui::Label( this, "RoundState_warmup", "");
	m_pLabelAScore->SetPaintBackgroundEnabled( false );
	m_pLabelAScore->SetPaintBorderEnabled( false );
	m_pLabelAScore->SizeToContents();
	m_pLabelAScore->SetContentAlignment( vgui::Label::a_west );
	m_pLabelAScore->SetFgColor( ColourWhite );

	m_pLabelAmmo = new vgui::Label( this, "RoundState_warmup", "");
	m_pLabelAmmo->SetPaintBackgroundEnabled( false );
	m_pLabelAmmo->SetPaintBorderEnabled( false );
	m_pLabelAmmo->SizeToContents();
	m_pLabelAmmo->SetContentAlignment( vgui::Label::a_west );
	m_pLabelAmmo->SetFgColor( ColourWhite );

	m_pLabelWaveTime = new vgui::Label( this, "RoundState_warmup", "");
	m_pLabelWaveTime->SetPaintBackgroundEnabled( false );
	m_pLabelWaveTime->SetPaintBorderEnabled( false );
	m_pLabelWaveTime->SizeToContents();
	m_pLabelWaveTime->SetContentAlignment( vgui::Label::a_west );
	m_pLabelWaveTime->SetFgColor( ColourWhite );

	/*m_pLabelBGVersion = new vgui::Label( this, "RoundState_warmup", "");
	m_pLabelBGVersion->SetPaintBackgroundEnabled( false );
	m_pLabelBGVersion->SetPaintBorderEnabled( false );
	m_pLabelBGVersion->SizeToContents();
	m_pLabelBGVersion->SetContentAlignment( vgui::Label::a_west );
	m_pLabelBGVersion->SetFgColor( ColourWhite );*/

	m_pLabelDamageVerificator = new vgui::Label( pParent, "RoundState_warmup", "");
	m_pLabelDamageVerificator->SetPaintBackgroundEnabled( false );
	m_pLabelDamageVerificator->SetPaintBorderEnabled( false );
	m_pLabelDamageVerificator->SizeToContents();
	m_pLabelDamageVerificator->SetContentAlignment( vgui::Label::a_west );
	m_pLabelDamageVerificator->SetFgColor( ColourWhite );

	m_pLabelLMS = new vgui::Label( this, "RoundState_warmup", "");
	m_pLabelLMS->SetPaintBackgroundEnabled( false );
	m_pLabelLMS->SetPaintBorderEnabled( false );
	m_pLabelLMS->SizeToContents();
	m_pLabelLMS->SetContentAlignment( vgui::Label::a_west );
	m_pLabelLMS->SetFgColor( ColourWhite );

	//hide all
	HideShowAll(false);
}

//==============================================
// CHudBG2's ApplySchemeSettings
// applies the schemes
//==============================================
void CHudBG2::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );

}

//==============================================
// CHudBG2's Init
// Inits any vars needed
//==============================================
void CHudBG2::Init( void )
{
	HOOK_HUD_MESSAGE( CHudBG2, HitVerif );
	//BG2 - Tjoppen - serverside blood, a place to hook as good as any
	extern void  __MsgFunc_ServerBlood( bf_read &msg );
	HOOK_MESSAGE( ServerBlood );
	//
}

//==============================================
// CHudBG2's VidInit
// Inits any textures needed
//==============================================
void CHudBG2::VidInit( void )
{
	m_Base = gHUD.GetIcon( "hud_base" );
	m_Stamina = gHUD.GetIcon( "hud_stamina" );
	m_Health = gHUD.GetIcon( "hud_health" );
	m_Straps = gHUD.GetIcon( "hud_straps" );
}

//==============================================
// CHudBG2's ShouldDraw
// whether the panel should be drawing
//==============================================
bool CHudBG2::ShouldDraw( void )
{
	bool temp = CHudElement::ShouldDraw();
	HideShowAll(temp);	//BG2 - Tjoppen - HACKHACK
	return temp;
}

//==============================================
// CHudBG2's Paint
// errr... paints the panel
//==============================================
//BG2 - Tjoppen - have to copy this from hl2mp_gamerules.cpp
ConVar mp_respawnstyle( "mp_respawnstyle", "1", FCVAR_REPLICATED | FCVAR_NOTIFY );	//0 = regular dm, 1 = waves, 2 = rounds
ConVar mp_respawntime( "mp_respawntime", "5", FCVAR_REPLICATED | FCVAR_NOTIFY );
static ConVar cl_draw_lms_indicator( "cl_draw_lms_indicator", "1", FCVAR_CLIENTDLL, "Draw last man standing indicator string?" );
//
void CHudBG2::Paint()
{
	if( !m_Base || !m_Straps || !m_Stamina || !m_Health )
		return;

	C_HL2MP_Player *pHL2Player = dynamic_cast<C_HL2MP_Player*>(C_HL2MP_Player::GetLocalPlayer());
	if (!pHL2Player || !pHL2Player->IsAlive())
	{
		//spectating
		int n = GetSpectatorTargetPlayer();
		if( n <= 0 )
		{
			HideShowAll(false);
			return;	//don't look at worldspawn..
		}

		pHL2Player = dynamic_cast<C_HL2MP_Player*>(UTIL_PlayerByIndex(n));

		if( !pHL2Player )
		{
			HideShowAll(false);
			return;
		}
	}

	C_BaseCombatWeapon *wpn = pHL2Player->GetActiveWeapon();
	if (!wpn)
	{
		HideShowAll(false);
		return;
	}

	HideShowAll(true);

	Color ColourRed( 255, 0, 0, 255 );
	Color ColourWhite( 255, 255, 255, 255 );

	char msg2[512];

	int w,h;
	int ystart = GetTall() - m_Base->Height();

	m_Base->DrawSelf(0,ystart,ColourWhite);
	int healthheight = pHL2Player->GetHealth()* 1.05;
	int healthy = 105 - healthheight;
	int stamheight = pHL2Player->m_iStamina * 1.05;
	int stamy = 105 - stamheight;
	m_Stamina->DrawSelfCropped(50,ystart + 2 + stamy,0,/*m_Stamina->Height()*//*20*/ stamy, 25, stamheight/*20*/,ColourWhite);
	m_Health->DrawSelfCropped(15,ystart + 2 + healthy,0,/*m_Health->Height()*//*40*/ healthy, 25, healthheight /*10*/,ColourWhite);
	m_Straps->DrawSelf(10,ystart + 6,ColourWhite);
	
	C_Team *pAmer = GetGlobalTeam(TEAM_AMERICANS);
	C_Team *pBrit = GetGlobalTeam(TEAM_BRITISH);
	Q_snprintf( msg2, 512, "%i ", pBrit ? pBrit->Get_Score() : 0);	//BG2 - Tjoppen - avoid NULL
	m_pLabelBScore->SetText(msg2);
	m_pLabelBScore->SizeToContents();
	m_pLabelWaveTime->GetSize( w, h );
	m_pLabelBScore->SetPos(90,ystart + 40 - h/2);
	m_pLabelBScore->SetFgColor( ColourWhite );
	
	Q_snprintf( msg2, 512, "%i ", pAmer ? pAmer->Get_Score() : 0);	//BG2 - Tjoppen - avoid NULL
	m_pLabelAScore->SetText(msg2);
	m_pLabelAScore->SizeToContents();
	m_pLabelWaveTime->GetSize( w, h );
	m_pLabelAScore->SetPos(135,ystart + 40 - h/2);
	m_pLabelAScore->SetFgColor( ColourWhite );
	
	int iAmmoCount = pHL2Player->GetAmmoCount(wpn->GetPrimaryAmmoType()) + wpn->Clip1();
	if( iAmmoCount >= 0 )
	{
		Q_snprintf( msg2, 512, "%i ", iAmmoCount);
		m_pLabelAmmo->SetText(msg2);
		if (wpn->Clip1() != 1)
		{
			m_pLabelAmmo->SetFgColor(ColourRed);
		}
		else
		{
			m_pLabelAmmo->SetFgColor(ColourWhite);
		}
		m_pLabelAmmo->SizeToContents();
		m_pLabelAmmo->GetSize( w, h );
		m_pLabelAmmo->SetPos(135,ystart + 100 - h/2);
	}
	else
		m_pLabelAmmo->SetVisible( false );

	//split seconds into two numbers so we don't get stupid 1:7 but rather 1:07
	int wavetime = ceilf(HL2MPRules()->m_fLastRespawnWave + mp_respawntime.GetFloat() - gpGlobals->curtime);
	if(	wavetime < 0 )
		wavetime = 0;

	int tens = (wavetime % 60) / 10,
		ones = (wavetime % 60) % 10;
	Q_snprintf( msg2, 512, "%i:%i%i ", (wavetime / 60), tens, ones );
	m_pLabelWaveTime->SetText(msg2);
	m_pLabelWaveTime->SizeToContents();
	m_pLabelWaveTime->GetSize( w, h );
	m_pLabelWaveTime->SetPos(120,ystart + 67 - h/2);
	m_pLabelWaveTime->SetFgColor( ColourWhite );

	// BP - BG version display at lower right bottom of screen
	/*Q_snprintf( msg2, 512, "%s ", HL2MPRules()->GetGameDescription());
	m_pLabelBGVersion->SetText(msg2);
	m_pLabelBGVersion->SizeToContents();
	m_pLabelBGVersion->GetSize( w, h );
	m_pLabelBGVersion->SetPos(5, 60);	
	m_pLabelBGVersion->SetFgColor( ColourWhite );*/

	m_pLabelDamageVerificator->SizeToContents();
	//center and put somewhat below crosshair
	m_pLabelDamageVerificator->SetPos((ScreenWidth()-m_pLabelDamageVerificator->GetWide())/2, (ScreenHeight()*5)/8);

	//fade out the last second
	float alpha = (m_flExpireTime - gpGlobals->curtime) * 255.0f;
	if( alpha < 0.0f )
		alpha = 0.0f;
	else if( alpha > 255.0f )
		alpha = 255.0f;

	m_pLabelDamageVerificator->SetFgColor( Color( 255, 255, 255, (int)alpha ) );

	m_pLabelLMS->SetText( localize()->Find("#LMS") );
	m_pLabelLMS->SizeToContents();
	m_pLabelLMS->GetSize( w, h );
	m_pLabelLMS->SetPos(5, ystart - 1.3*h);
	m_pLabelLMS->SetFgColor( ColourWhite );
}

//==============================================
// CHudBG2's OnThink
// every think check if we're hidden, if so take away the text
//==============================================
void CHudBG2::OnThink()
{
	// display hintbox if stamina drops below 20%
	C_HL2MP_Player *pHL2Player = dynamic_cast<C_HL2MP_Player*>(C_HL2MP_Player::GetLocalPlayer());
	if (pHL2Player && pHL2Player->m_iStamina < 20)
 		(GET_HUDELEMENT( CHintbox ))->UseHint(HINT_STAMINA, 6, DISPLAY_ONCE);
	
	//let paint sort it out
	/*C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if (player)
	{
		if (player->GetHealth() <= 0)
		{
			HideShowAll(false);
		}
	}*/
}

void CHudBG2::Reset( void )
{
	//mapchange, clear indicators. and stuff.
	m_flExpireTime = 0;
	HideShowAll( false );
}

void CHudBG2::HideShowAll( bool visible )
{
	m_pLabelAScore->SetVisible(visible);
	m_pLabelBScore->SetVisible(visible);
	m_pLabelWaveTime->SetVisible(visible);
	m_pLabelAmmo->SetVisible(visible);
	//m_pLabelBGVersion->SetVisible(false);	// BP: not used yet as its not subtle enough, m_pLabelBGVersion->SetVisible(ShouldDraw());
	m_pLabelDamageVerificator->SetVisible(visible && m_flExpireTime > gpGlobals->curtime);
	m_pLabelLMS->SetVisible( visible && mp_respawnstyle.GetInt() == 2 && cl_draw_lms_indicator.GetBool() );
}

const char* CHudBG2::HitgroupName( int hitgroup )
{
	//BG2 - Tjoppen - TODO: localize
	switch ( hitgroup )
	{
	case HITGROUP_GENERIC:
		return NULL;
	case HITGROUP_HEAD:
		return "head";
	case HITGROUP_CHEST:
		return "chest";
	case HITGROUP_STOMACH:
		return "stomach";
	case HITGROUP_LEFTARM:
		return "left arm";
	case HITGROUP_RIGHTARM:
		return "right arm";
	case HITGROUP_LEFTLEG:
		return "left leg";
	case HITGROUP_RIGHTLEG:
		return "right leg";
	default:
		return "unknown default case";
	}
}

void CHudBG2::MsgFunc_HitVerif( bf_read &msg )
{
	int attacker, victim, hitgroup, damage;

	attacker	= msg.ReadByte();
	victim		= msg.ReadByte();
	hitgroup	= msg.ReadByte();
	damage		= msg.ReadShort();

	//Msg( "MsgFunc_HitVerif: %i %i %i %i\n", attacker, victim, hitgroup, damage );

	player_info_t sAttackerInfo, sVictimInfo;
	engine->GetPlayerInfo( attacker, &sAttackerInfo );
	engine->GetPlayerInfo( victim, &sVictimInfo );

	if( !C_BasePlayer::GetLocalPlayer() )
		return;

	char txt[512];
	const char *hitgroupname = HitgroupName( hitgroup );

	//BG2 - Tjoppen - TODO: localize
	if( C_BasePlayer::GetLocalPlayer()->entindex() == victim )
	{
		//I'm the victim here!
		if( hitgroupname )
			sprintf( txt, "You were hit in the %s by %s for %i points of damage", hitgroupname, sAttackerInfo.name, damage );
		else
			sprintf( txt, "You were hit by %s for %i points of damage", sAttackerInfo.name, damage );
	}
	else
	{
		//got one!
		if( hitgroupname )
			sprintf( txt, "You hit %s in the %s for %i points of damage", sVictimInfo.name, hitgroupname, damage );
		else
			sprintf( txt, "You hit %s for %i points of damage", sVictimInfo.name, damage );
	}

	m_pLabelDamageVerificator->SetText( txt );
	m_flExpireTime = gpGlobals->curtime + 5.0f;
}