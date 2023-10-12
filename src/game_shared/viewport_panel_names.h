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
// Purpose: 
//
//=============================================================================//

#ifndef VIEWPORT_PANEL_NAMES_H
#define VIEWPORT_PANEL_NAMES_H
#ifdef _WIN32
#pragma once
#endif


// default panel name definitions
#define PANEL_ALL			"all"		// all current panels
#define PANEL_ACTIVE		"active"	// current active panel			

#define PANEL_SCOREBOARD	"scores"
#define PANEL_OVERVIEW		"overview"
#define PANEL_CLASS			"class"
#define PANEL_TEAM			"team"
#define PANEL_SPECGUI		"specgui"	// passive spectator elements (top/bottom bars)
#define PANEL_SPECMENU		"specmenu"  // active spectator elements (options menus etc)
#define PANEL_INFO			"info"
#define PANEL_BUY			"buy"
#define PANEL_BUY_CT		"buy_ct"
#define PANEL_BUY_TER		"buy_ter"
#define PANEL_BUY_EQUIP_CT	"buyequip_ct"
#define PANEL_BUY_EQUIP_TER	"buyequip_ter"
#define PANEL_NAV_PROGRESS	"nav_progress"
#define PANEL_BUYPRESET_MAIN	"buypreset_main"
#define PANEL_BUYPRESET_EDIT	"buypreset_edit"
//BG2 - Tjoppen - class selection menu
#define PANEL_CLASSES		"classmenu"
//#define PANEL_TEAMS			"teammenu"
#define PANEL_COMM			"commmenu"
#define PANEL_COMM2			"commmenu2"
//


#endif // VIEWPORT_PANEL_NAMES_H
