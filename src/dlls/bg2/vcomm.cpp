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

//vcomm.cpp - arrays holding data for voice command menus and stuff

#include "vcomm.h"

#ifdef CLIENT_DLL
char *pVComms[NUM_VOICECOMMS] =
{
	"1. Yes",
	"2. No",
	"3. Follow me",
	"4. Leave it to me",
	"5. Under fire",
	"6. Spread out",
	"7. Battle cry",
	"8. Medic",
	"0. Cancel",

	"1. Advance",
	"2. Retreat",
	"3. Rally the men",
	"4. Halt",
	"5. Line up",
	"6. Make ready",
	"7. Present arms",
	"8. Fire",
	"9. Cease fire",
	"0. Cancel",
};

char *pVChats[NUM_VOICECOMMS] =
{
	"Yes",
	"No",
	"Follow me",
	"Leave it to me",
	"Under attack, need assistance",
	"Spread out!",
	"",
	"Require medical aid!",
	"",

	"Advance!",
	"Retreat!",
	"Rally round lads",
	"Halt!",
	"Line formation",
	"Make ready",
	"Present!",
	"Fire at will!",
	"Cease fire!",
	"",
};
#else
char *pVComms[NUM_VOICECOMMS] =
{
	".Yes",
	".No",
	".FollowMe",
	".LeaveItToMe",
	".UnderFire",
	".SpreadOut",
	".BattleCry",//"Voicecomms.BattleCry", //different for american and british( ABattleCry/BBattleCry )
	".Medic",
	"",

	".Advance",
	".Retreat",
	".RallyTheMen",
	".Halt",
	".LineUp",
	".MakeReady",
	".PresentArms",
	".Fire",
	".CeaseFire",
	"",
};
#endif