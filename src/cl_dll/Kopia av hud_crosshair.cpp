//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_crosshair.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "IVRenderView.h"
//BG2 - Tjoppen - #includes
#include "../dlls/hl2_dll/weapon_bg2base.h"
//

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar crosshair( "crosshair", "1", FCVAR_ARCHIVE );
ConVar cl_observercrosshair( "cl_observercrosshair", "1", FCVAR_ARCHIVE );

using namespace vgui;

int ScreenTransform( const Vector& point, Vector& screen );

DECLARE_HUDELEMENT( CHudCrosshair );

CHudCrosshair::CHudCrosshair( const char *pElementName ) :
  CHudElement( pElementName ), BaseClass( NULL, "HudCrosshair" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCrosshair = 0;

	m_clrCrosshair = Color( 0, 0, 0, 0 );

	m_vecCrossHairOffsetAngle.Init();

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR );
}

void CHudCrosshair::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_pDefaultCrosshair = gHUD.GetIcon("crosshair_default");
	SetPaintBackgroundEnabled( false );
}

void CHudCrosshair::Paint( void )
{
	if ( !crosshair.GetInt() )
		return;

	if ( !g_pClientMode->ShouldDrawCrosshair() )
		return;

	if ( !m_pCrosshair )
		return;

	if ( engine->IsDrawingLoadingImage() || engine->IsPaused() )
		return;
	
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	// draw a crosshair only if alive or spectating in eye
	bool shouldDraw = false;
	if ( pPlayer->IsAlive() )
		shouldDraw = true;

	if ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
		shouldDraw = true;

	if ( pPlayer->GetObserverMode() == OBS_MODE_ROAMING && cl_observercrosshair.GetBool() )
		shouldDraw = true;

	if ( !shouldDraw )
		return;

	if ( pPlayer->GetFlags() & FL_FROZEN )
		return;

	if ( pPlayer->entindex() != render->GetViewEntity() )
		return;

	m_curViewAngles = CurrentViewAngles();
	m_curViewOrigin = CurrentViewOrigin();

	float x, y;
	x = ScreenWidth()/2;
	y = ScreenHeight()/2;

	// MattB - m_vecCrossHairOffsetAngle is the autoaim angle.
	// if we're not using autoaim, just draw in the middle of the 
	// screen
	if( m_vecCrossHairOffsetAngle != vec3_angle )
	{
		QAngle angles;
		Vector forward;
		Vector point, screen;

		// this code is wrong
		angles = m_curViewAngles + m_vecCrossHairOffsetAngle;
		AngleVectors( angles, &forward );
		VectorAdd( m_curViewOrigin, forward, point );
		ScreenTransform( point, screen );

		x += 0.5f * screen[0] * ScreenWidth() + 0.5f;
		y += 0.5f * screen[1] * ScreenHeight() + 0.5f;
	}

	//BG2 - Tjoppen - other crosshair..
	/*int max = 0.02f * pPlayer->GetLocalVelocity().Length(),
		duck = pPlayer->GetFlags() & FL_DUCKING ? 0 : 2;

    for( int i = 3 + duck; i < 5 + max + duck; i++ )
	{
		int c = (i-3-duck)*10;
		if( c > 255 )
			c = 255;
		surface()->DrawSetColor( Color( c, c, c, 255 - c ) );
		surface()->DrawOutlinedRect( x - i - max/2, y - i - max/2, x + i + max/2, y + i + max/2 );
	}*/

	C_BaseBG2Weapon *weapon = (C_BaseBG2Weapon*)pPlayer->GetActiveWeapon();
	if( weapon )
	{
		/*Msg( "%f %f %f\n", weapon->GetSpread( C_BaseBG2Weapon::ATTACK_PRIMARY ).x,
							weapon->GetSpread( C_BaseBG2Weapon::ATTACK_PRIMARY ).y,
							weapon->GetSpread( C_BaseBG2Weapon::ATTACK_PRIMARY ).z );*/

		int w = ScreenWidth(),
			h = ScreenHeight();
		
		float	cx = w / 2,
				cy = h / 2,
				r = min( w, h ) / 2;

		if( weapon->GetAttackType( C_BaseBG2Weapon::ATTACK_PRIMARY ) == C_BaseBG2Weapon::ATTACKTYPE_FIREARM )
			r *= weapon->GetSpread( C_BaseBG2Weapon::ATTACK_PRIMARY ).x;
		else
			r *= 0.05f;

		//Msg( "%f\n", gpGlobals->frametime );
		static float lastr = 0;

		//r = lastr = r * 5.0f * gpGlobals->frametime + lastr * (1.f - 5.0f * gpGlobals->frametime);

		r = lastr = r + (lastr - r) * expf( -5.0f * gpGlobals->frametime );

		//Msg( "%f %f %f\n", cx, cy, r );

		int step = 10;

		surface()->DrawSetColor( Color( 0, 150, 30, 35 ) );
		for( int dx = -1; dx <= 1; dx++ )
			for( int dy = -1; dy <= 1; dy++ )
			{
				if( dx == 0 && dy == 0 )
					continue;

				int cx2 = cx + dx,
					cy2 = cy + dy;

				int lastx = (int)(cx2 + r*cosf((float)-step * M_PI / 180.f)),
					lasty =	(int)(cy2 + r*sinf((float)-step * M_PI / 180.f));

				for( int i = 0, j = 0; i < 360; i += step, j++ )
				{
					float	a = (float)i * M_PI / 180.f;

					int x = (int)(cx2 + r*cosf(a)),
						y = (int)(cy2 + r*sinf(a));

					//surface()->DrawSetColor( Color( (j&1)*255, (j&1)*255, (j&1)*255, 255 ) );

					surface()->DrawLine( lastx, lasty, x, y );

					lastx = x;
					lasty = y;
				}
			}

        int lastx = (int)(cx + r*cosf((float)-step * M_PI / 180.f)),
			lasty =	(int)(cy + r*sinf((float)-step * M_PI / 180.f));

		surface()->DrawSetColor( Color( 0, 221, 47, 167 ) );
		for( int i = 0, j = 0; i < 360; i += step, j++ )
		{
			float	a = (float)i * M_PI / 180.f;

			int x = (int)(cx + r*cosf(a)),
				y = (int)(cy + r*sinf(a));

			surface()->DrawLine( lastx, lasty, x, y );

			lastx = x;
			lasty = y;
		}
	}

	/*m_pCrosshair->DrawSelf( 
			x - 0.5f * m_pCrosshair->Width(), 
			y - 0.5f * m_pCrosshair->Height(),
			m_clrCrosshair );*/
	//
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshairAngle( const QAngle& angle )
{
	VectorCopy( angle, m_vecCrossHairOffsetAngle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshair( CHudTexture *texture, Color& clr )
{
	m_pCrosshair = texture;
	m_clrCrosshair = clr;
}

//-----------------------------------------------------------------------------
// Purpose: Resets the crosshair back to the default
//-----------------------------------------------------------------------------
void CHudCrosshair::ResetCrosshair()
{
	SetCrosshair( m_pDefaultCrosshair, Color(255, 255, 255, 255) );
}
