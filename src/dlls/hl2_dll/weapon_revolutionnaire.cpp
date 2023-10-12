//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "weapon_bg2base.h"

#ifdef CLIENT_DLL
#define CWeaponrevolutionnaire C_Weaponrevolutionnaire
#endif
DECLARE_BG2_WEAPON( revolutionnaire )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
	m_bDontAutoreload	= true;
	m_bInSwing			= false;
	m_bUsesDelay		= false;

	m_flRange			= 90.f;
	m_flFireRate		= 0.8f;
	m_flImpactDelay		= 0.3f;
	
	m_flShotDamage		= 75.f;
	m_flStabDamage		= 60.f;

	m_vDuckSpread		= VECTOR_CONE_2DEGREES;
	m_vStandSpread		= VECTOR_CONE_5DEGREES;
}