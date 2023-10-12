//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_edit.cpp
// Implementation of Navigation Mesh edit mode
// Author: Michael Booth, 2003-2004

#include "cbase.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "nav_node.h"
#include "nav_colors.h"
#include "Color.h"
#include "tier0/vprof.h"

ConVar nav_show_area_info( "nav_show_area_info", "0.5", FCVAR_GAMEDLL, "Duration in seconds to show nav area ID and attributes while editing" );
ConVar nav_snap_to_grid( "nav_snap_to_grid", "0", FCVAR_GAMEDLL, "Snap to the nav generation grid when creating new nav areas" );

#if DEBUG_NAV_NODES
ConVar nav_show_nodes( "nav_show_nodes", "1" );
#endif // DEBUG_NAV_NODES


//--------------------------------------------------------------------------------------------------------------
void EditNav_Precache(void *pUser)
{
	CBaseEntity::PrecacheScriptSound( "Bot.EditSwitchOn" );
	CBaseEntity::PrecacheScriptSound( "EDIT_TOGGLE_PLACE_MODE" );
	CBaseEntity::PrecacheScriptSound( "Bot.EditSwitchOff" );
	CBaseEntity::PrecacheScriptSound( "EDIT_PLACE_PICK" );
	CBaseEntity::PrecacheScriptSound( "EDIT_DELETE" );
	CBaseEntity::PrecacheScriptSound( "EDIT.ToggleAttribute" );
	CBaseEntity::PrecacheScriptSound( "EDIT_SPLIT.MarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_SPLIT.NoMarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MERGE.Enable" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MERGE.Disable" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MARK.Enable" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MARK.Disable" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MARK_UNNAMED.Enable" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MARK_UNNAMED.NoMarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MARK_UNNAMED.MarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_CONNECT.AllDirections" );
	CBaseEntity::PrecacheScriptSound( "EDIT_CONNECT.Added" );
	CBaseEntity::PrecacheScriptSound( "EDIT_DISCONNECT.MarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_DISCONNECT.NoMarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_SPLICE.MarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_SPLICE.NoMarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_SELECT_CORNER.MarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_SELECT_CORNER.NoMarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MOVE_CORNER.MarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_MOVE_CORNER.NoMarkedArea" );
	CBaseEntity::PrecacheScriptSound( "EDIT_BEGIN_AREA.Creating" );
	CBaseEntity::PrecacheScriptSound( "EDIT_BEGIN_AREA.NotCreating" );
	CBaseEntity::PrecacheScriptSound( "EDIT_END_AREA.Creating" );
	CBaseEntity::PrecacheScriptSound( "EDIT_END_AREA.NotCreating" );
	CBaseEntity::PrecacheScriptSound( "EDIT_WARP_TO_MARK" );
}

#ifdef CSTRIKE_DLL
PRECACHE_REGISTER_FN( EditNav_Precache );
#endif


//--------------------------------------------------------------------------------------------------------------
inline float round( float val, float unit )
{
	val = val + ((val < 0.0f) ? -unit*0.5f : unit*0.5f);
	return (float)( ((int)val) / (int)unit );
}


//--------------------------------------------------------------------------------------------------------------
/**
 *  Convenience function to find the nav area a player is looking at, for editing commands
 */
bool CNavMesh::GetActiveNavArea( void )
{
	VPROF( "CNavMesh::GetActiveNavArea" );

	m_splitAlongX = false;
	m_splitEdge = 0.0f;
	m_selectedArea = NULL;
	m_climbableSurface = false;
	m_selectedLadder = NULL;

	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return false;

	const float maxRange = 1000.0f;		// 500

	Vector dir;
	AngleVectors( player->EyeAngles() + player->GetPunchAngle(), &dir );

	Vector from = player->EyePosition();
	Vector to = from + maxRange * dir;

	trace_t result;
	UTIL_TraceLine( from, to, MASK_PLAYERSOLID_BRUSHONLY, player, COLLISION_GROUP_NONE, &result );

	if (result.fraction != 1.0f)
	{
		if ( !m_isCreatingNavArea )
		{
			m_climbableSurface = physprops->GetSurfaceData( result.surface.surfaceProps )->game.climbable != 0;
			if ( !m_climbableSurface )
			{
				m_climbableSurface = (result.contents & CONTENTS_LADDER) != 0;
			}
			m_surfaceNormal = result.plane.normal;

			if ( m_climbableSurface )
			{
				// check if we're on the same plane as the original point when we're building a ladder
				if ( m_isCreatingLadder )
				{
					if ( m_surfaceNormal != m_ladderNormal )
					{
						m_climbableSurface = false;
					}
				}

				if ( m_surfaceNormal.z > 0.9f )
				{
					m_climbableSurface = false; // don't try to build ladders on flat ground
				}
			}
		}

		if ( m_climbableSurface && !m_isCreatingLadder )
		{
			float closestDistSqr = 200.0f * 200.0f;

			FOR_EACH_LL( m_ladderList, it )
			{
				CNavLadder *ladder = m_ladderList[ it ];

				Vector absMin = ladder->m_bottom;
				Vector absMax = ladder->m_top;

				Vector left( 0, 0, 0), right(0, 0, 0);
				AddDirectionVector( &left, DirectionLeft( ladder->m_dir ), ladder->m_width*0.5f );
				AddDirectionVector( &right, DirectionRight( ladder->m_dir ), ladder->m_width*0.5f );

				absMin.x += min( left.x, right.x );
				absMin.y += min( left.y, right.y );

				absMax.x += max( left.x, right.x );
				absMax.y += max( left.y, right.y );

				Extent e;
				e.lo = absMin + Vector( -5, -5, -5 );
				e.hi = absMax + Vector( 5, 5, 5 );

				if ( e.Contains( m_editCursorPos ) )
				{
					m_selectedLadder = ladder;
					break;
				}

				Vector p1 = (ladder->m_bottom + ladder->m_top)/2;
				Vector p2 = m_editCursorPos;
				float distSqr = p1.DistToSqr( p2 );

				if ( distSqr < closestDistSqr )
				{
					m_selectedLadder = ladder;
					closestDistSqr = distSqr;
				}
			}
		}

		m_editCursorPos = result.endpos;

		// find the area the player is pointing at
		if ( !m_climbableSurface )
		{
			m_selectedArea = TheNavMesh->GetNearestNavArea( result.endpos, false, 500.0f );
		}

		if ( m_selectedArea )
		{
			float yaw = player->EyeAngles().y;
			while( yaw > 360.0f )
				yaw -= 360.0f;

			while( yaw < 0.0f )
				yaw += 360.0f;

			if ((yaw < 45.0f || yaw > 315.0f) || (yaw > 135.0f && yaw < 225.0f))
			{
				m_splitEdge = GenerationStepSize * round( result.endpos.y, GenerationStepSize );
				m_splitAlongX = true;
			}
			else
			{
				m_splitEdge = GenerationStepSize * round( result.endpos.x, GenerationStepSize );
				m_splitAlongX = false;
			}
		}

		if ( nav_snap_to_grid.GetBool() && !m_climbableSurface && !m_isCreatingLadder )
		{
			m_editCursorPos.x = GenerationStepSize * round( result.endpos.x, GenerationStepSize );
			m_editCursorPos.y = GenerationStepSize * round( result.endpos.y, GenerationStepSize );
		}

		return true;
	}
	
	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Flood fills all areas with current place
 */
class PlaceFloodFillFunctor
{
public:
	PlaceFloodFillFunctor( CNavArea *area )
	{
		m_initialPlace = area->GetPlace();
	}

	bool operator() ( CNavArea *area )
	{
		if (area->GetPlace() != m_initialPlace)
			return false;

		area->SetPlace( TheNavMesh->GetNavPlace() );

		return true;
	}

private:
	unsigned int m_initialPlace;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Called when edit mode has just been enabled
 */
void CNavMesh::OnEditModeStart( void )
{
/*
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	const Vector &eye = player->EyePosition();
	Vector forward, right, up;
	AngleVectors( player->EyeAngles(), &forward, &right, &up );

	// darken the world so the edit lines show up clearly
	const float screenHalfSize = 1000.0f;
	const float screenRange = 50.0f;
	Vector upLeft = eye + screenRange * forward + screenHalfSize * (up - right);
	Vector upRight = eye + screenRange * forward + screenHalfSize * (up + right);
	Vector downLeft = eye + screenRange * forward - screenHalfSize * (up + right);
	Vector downRight = eye + screenRange * forward + screenHalfSize * (right - up);

	const int alpha = 200;
	NDebugOverlay::Triangle( downLeft, upLeft, upRight, 0, 0, 0, alpha, true, 999999.9f );
	NDebugOverlay::Triangle( downLeft, upRight, downRight, 0, 0, 0, alpha, true, 999999.9f );
*/
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Called when edit mode has just been disabled
 */
void CNavMesh::OnEditModeEnd( void )
{
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw navigation areas and edit them
 * @todo Clean the whole edit system up - its structure is legacy from peculiarities in GoldSrc.
 */
void CNavMesh::DrawEditMode( void )
{
	VPROF( "CNavMesh::DrawEditMode" );

	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	const float maxRange = 1000.0f;		// 500

#if DEBUG_NAV_NODES
	if ( nav_show_nodes.GetBool() )
	{
		for ( CNavNode *node = CNavNode::GetFirst(); node != NULL; node = node->GetNext() )
		{
			if ( m_editCursorPos.DistToSqr( *node->GetPosition() ) < 150*150 )
			{
				node->Draw();
			}
		}
	}
#endif // DEBUG_NAV_NODES

	// draw approach points for marked area
	if (nav_show_approach_points.GetBool() && GetMarkedArea())
	{
		Vector ap;
		float halfWidth;
		for( int i=0; i<GetMarkedArea()->GetApproachInfoCount(); ++i )
		{
			const CNavArea::ApproachInfo *info = GetMarkedArea()->GetApproachInfo( i );

			// compute approach point
			if (info->hereToNextHow <= GO_WEST)
			{
				info->here.area->ComputePortal( info->next.area, (NavDirType)info->hereToNextHow, &ap, &halfWidth );
				ap.z = info->next.area->GetZ( ap );
			}
			else
			{
				// use the area's center as an approach point
				ap = info->here.area->GetCenter();
			}

			NavDrawLine( ap + Vector( 0, 0, 50 ), ap + Vector( 10, 0, 0 ), NavApproachPointColor );
			NavDrawLine( ap + Vector( 0, 0, 50 ), ap + Vector( -10, 0, 0 ), NavApproachPointColor );
			NavDrawLine( ap + Vector( 0, 0, 50 ), ap + Vector( 0, 10, 0 ), NavApproachPointColor );
			NavDrawLine( ap + Vector( 0, 0, 50 ), ap + Vector( 0, -10, 0 ), NavApproachPointColor );
		}
	}

	Vector dir;
	AngleVectors( player->EyeAngles() + player->GetPunchAngle(), &dir );

	Vector from = player->EyePosition();
	Vector to = from + maxRange * dir;

	if ( GetActiveNavArea() )
	{
		// draw cursor
		float cursorSize = 10.0f;

		if ( m_climbableSurface )
		{
			NDebugOverlay::Cross3D( m_editCursorPos, cursorSize, 0, 255, 0, true, 0.1f );
		}
		else
		{
			NavDrawLine( m_editCursorPos + Vector( 0, 0, cursorSize ), m_editCursorPos,								NavCursorColor );
			NavDrawLine( m_editCursorPos + Vector( cursorSize, 0, 0 ), m_editCursorPos + Vector( -cursorSize, 0, 0 ),	NavCursorColor );
			NavDrawLine( m_editCursorPos + Vector( 0, cursorSize, 0 ), m_editCursorPos + Vector( 0, -cursorSize, 0 ),	NavCursorColor );
		}

		// show drag rectangle when creating areas and ladders
		if ( m_isCreatingNavArea )
		{
			float z = m_anchor.z + 2.0f;
			NavDrawLine( Vector( m_editCursorPos.x, m_editCursorPos.y, z ), Vector( m_anchor.x, m_editCursorPos.y, z ),	NavCreationColor );
			NavDrawLine( Vector( m_anchor.x, m_anchor.y, z ), Vector( m_anchor.x, m_editCursorPos.y, z ),					NavCreationColor );
			NavDrawLine( Vector( m_anchor.x, m_anchor.y, z ), Vector( m_editCursorPos.x, m_anchor.y, z ),					NavCreationColor );
			NavDrawLine( Vector( m_editCursorPos.x, m_editCursorPos.y, z ), Vector( m_editCursorPos.x, m_anchor.y, z ),	NavCreationColor );
		}
		else if ( m_isCreatingLadder )
		{
			NavDrawLine( m_editCursorPos, Vector( m_editCursorPos.x, m_editCursorPos.y, m_ladderAnchor.z ),	NavCreationColor );
			NavDrawLine( Vector( m_editCursorPos.x, m_editCursorPos.y, m_ladderAnchor.z ), m_ladderAnchor,		NavCreationColor );
			NavDrawLine( m_ladderAnchor, Vector( m_ladderAnchor.x, m_ladderAnchor.y, m_editCursorPos.z ),		NavCreationColor );
			NavDrawLine( Vector( m_ladderAnchor.x, m_ladderAnchor.y, m_editCursorPos.z ), m_editCursorPos,		NavCreationColor );
		}

		if ( m_selectedLadder )
		{
			m_lastSelectedArea = NULL;

			// if ladder changed, print its ID
			if (m_selectedLadder != m_lastSelectedLadder || nav_show_area_info.GetBool())
			{
				m_lastSelectedLadder = m_selectedLadder;

				// print ladder info
				char buffer[80];

				Q_snprintf( buffer, sizeof( buffer ), "Ladder #%d\n", m_selectedLadder->GetID() );
				NDebugOverlay::ScreenText( 0.5, 0.53, buffer, 255, 255, 0, 128, nav_show_area_info.GetBool() ? 0.1 : 0.5 );
			}

			// draw the ladder we are pointing at and all connected areas
			m_selectedLadder->DrawLadder();
			m_selectedLadder->DrawConnectedAreas();
		}

		if ( m_markedLadder && !m_isPlaceMode )
		{
			// draw the "marked" ladder
			m_markedLadder->DrawLadder();
		}

		if ( m_markedArea && !m_isPlaceMode )
		{
			// draw the "marked" area
			m_markedArea->Draw();

			if ( m_markedCorner != NUM_CORNERS )
			{
				m_markedArea->DrawMarkedCorner( m_markedCorner );
			}
		}

		// find the area the player is pointing at
		if (m_selectedArea)
		{
			m_lastSelectedLadder = NULL;

			// if area changed, print its ID
			if ( m_selectedArea != m_lastSelectedArea )
			{
				m_showAreaInfoTimer.Start( nav_show_area_info.GetFloat() );
				m_lastSelectedArea = m_selectedArea;
			}

			if (m_showAreaInfoTimer.HasStarted() && !m_showAreaInfoTimer.IsElapsed() )
			{
				char buffer[80];
				char attrib[80];
				char locName[80];

				if (m_selectedArea->GetPlace())
				{
					const char *name = TheNavMesh->PlaceToName( m_selectedArea->GetPlace() );
					if (name)
						strcpy( locName, name );
					else
						strcpy( locName, "ERROR" );
				}
				else
				{
					locName[0] = '\000';
				}

				if (m_isPlaceMode)
				{
					attrib[0] = '\000';
				}
				else
				{
					attrib[0] = 0;
					int attributes = m_selectedArea->GetAttributes();
					if ( attributes & NAV_MESH_CROUCH )		Q_strncat( attrib, "CROUCH ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_JUMP )		Q_strncat( attrib, "JUMP ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_PRECISE )	Q_strncat( attrib, "PRECISE ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_NO_JUMP )	Q_strncat( attrib, "NO_JUMP ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_STOP )		Q_strncat( attrib, "STOP ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_RUN )		Q_strncat( attrib, "RUN ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_WALK )		Q_strncat( attrib, "WALK ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_AVOID )		Q_strncat( attrib, "AVOID ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_TRANSIENT )	Q_strncat( attrib, "TRANSIENT ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_DONT_HIDE )	Q_strncat( attrib, "DONT_HIDE ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_STAND )		Q_strncat( attrib, "STAND ", sizeof( attrib ), -1 );
					if ( m_selectedArea->IsBlocked() )		Q_strncat( attrib, "BLOCKED ", sizeof( attrib ), -1 );
				}

				Q_snprintf( buffer, sizeof( buffer ), "Area #%d %s %s\n", m_selectedArea->GetID(), locName, attrib );
				NDebugOverlay::ScreenText( 0.5, 0.53, buffer, 255, 255, 0, 128, 0.1 );

				// do "place painting"
				if (m_isPlacePainting)
				{				
					if (m_selectedArea->GetPlace() != TheNavMesh->GetNavPlace())
					{
						m_selectedArea->SetPlace( TheNavMesh->GetNavPlace() );
						player->EmitSound( "Bot.EditSwitchOn" );
					}
				}
			}

			if (m_isPlaceMode)
			{
				m_selectedArea->DrawConnectedAreas();

			}
			else	// normal editing mode
			{
				// draw split line
				const Extent &extent = m_selectedArea->GetExtent();

				float yaw = player->EyeAngles().y;
				while( yaw > 360.0f )
					yaw -= 360.0f;

				while( yaw < 0.0f )
					yaw += 360.0f;

				if (m_splitAlongX)
				{
					from.x = extent.lo.x;
					from.y = m_splitEdge;
					from.z = m_selectedArea->GetZ( from );

					to.x = extent.hi.x;
					to.y = m_splitEdge;
					to.z = m_selectedArea->GetZ( to );
				}
				else
				{
					from.x = m_splitEdge;
					from.y = extent.lo.y;
					from.z = m_selectedArea->GetZ( from );

					to.x = m_splitEdge;
					to.y = extent.hi.y;
					to.z = m_selectedArea->GetZ( to );
				}

				NavDrawLine( from, to, NavSplitLineColor );

				// draw the area we are pointing at and all connected areas
				m_selectedArea->DrawConnectedAreas();
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::SetEditMode( bool isPlaceMode )
{
	if ( isPlaceMode )
	{
		m_markedLadder = NULL;
		m_markedArea = NULL;
		m_markedCorner = NUM_CORNERS;
	}

	m_isPlaceMode = isPlaceMode;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::SetPlacePaintingMode( bool painting )
{
	if ( m_isPlaceMode )
	{
		m_markedLadder = NULL;
		m_markedArea = NULL;
		m_markedCorner = NUM_CORNERS;

		m_isPlacePainting = painting;
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::SetMarkedLadder( CNavLadder *ladder )
{
	m_markedLadder = ladder;
	m_markedArea = NULL;
	m_markedCorner = NUM_CORNERS;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::SetMarkedArea( CNavArea *area )
{
	m_markedLadder = NULL;
	m_markedArea = area;
	m_markedCorner = NUM_CORNERS;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavDelete( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		player->EmitSound( "EDIT_DELETE" );
		TheNavAreaList.FindAndRemove( m_selectedArea );
		delete m_selectedArea;
	}
	else if ( m_selectedLadder )
	{
		player->EmitSound( "EDIT_DELETE" );
		m_ladderList.FindAndRemove( m_selectedLadder );
		delete m_selectedLadder;
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavSplit( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if (m_selectedArea->SplitEdit( m_splitAlongX, m_splitEdge ))
			player->EmitSound( "EDIT_SPLIT.MarkedArea" );
		else
			player->EmitSound( "EDIT_SPLIT.NoMarkedArea" );
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
bool MakeSniperSpots( CNavArea *area )
{
	if ( !area )
		return false;

	bool splitAlongX;
	float splitEdge;

	const float minSplitSize = 2.0f; // ensure the first split is larger than this

	if ( area->GetSizeX() > GenerationStepSize )
	{
		splitEdge = GenerationStepSize * round( area->GetExtent().lo.x, GenerationStepSize );
		if ( splitEdge < area->GetExtent().lo.x + minSplitSize )
			splitEdge += GenerationStepSize;
		splitAlongX = false;
	}
	else if ( area->GetSizeY() > GenerationStepSize )
	{
		splitEdge = GenerationStepSize * round( area->GetExtent().lo.y, GenerationStepSize );
		if ( splitEdge < area->GetExtent().lo.y + minSplitSize )
			splitEdge += GenerationStepSize;
		splitAlongX = true;
	}
	else
	{
		return false;
	}

	CNavArea *first, *second;
	if ( !area->SplitEdit( splitAlongX, splitEdge, &first, &second ) )
	{
		return false;
	}

	first->Disconnect( second );
	second->Disconnect( first );

	MakeSniperSpots( first );
	MakeSniperSpots( second );

	return true;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMakeSniperSpots( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		// recursively split the area
		if ( MakeSniperSpots( m_selectedArea ) )
		{
			player->EmitSound( "EDIT_SPLIT.MarkedArea" );
		}
		else
		{
			player->EmitSound( "EDIT_SPLIT.NoMarkedArea" );
		}
	}
	else
	{
		player->EmitSound( "EDIT_SPLIT.NoMarkedArea" );
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMerge( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if ( m_markedArea && m_markedArea != m_selectedArea )
		{
			if ( m_selectedArea->MergeEdit( GetMarkedArea() ) )
				player->EmitSound( "EDIT_MERGE.Enable" );
			else
				player->EmitSound( "EDIT_MERGE.Disable" );
		}
		else
		{
			Msg( "To merge, mark an area, highlight a second area, then invoke the merge command" );
			player->EmitSound( "EDIT_MERGE.Disable" );
		}
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMark( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_markedArea || m_markedLadder )
	{
		// Unmark area or ladder
		player->EmitSound( "EDIT_MARK.Enable" );
		SetMarkedArea( NULL );
	}
	else if ( m_selectedArea )
	{
		// Mark an area
		player->EmitSound( "EDIT_MARK.Disable" );
		SetMarkedArea( m_selectedArea );

		int connected = 0;
		connected += GetMarkedArea()->GetAdjacentCount( NORTH );
		connected += GetMarkedArea()->GetAdjacentCount( SOUTH );
		connected += GetMarkedArea()->GetAdjacentCount( EAST );
		connected += GetMarkedArea()->GetAdjacentCount( WEST );

		Msg( "Marked Area is connected to %d other Areas\n", connected );
	}
	else if ( m_selectedLadder )
	{
		// Mark a ladder
		player->EmitSound( "EDIT_MARK.Disable" );
		SetMarkedLadder( m_selectedLadder );

		int connected = 0;
		connected += m_markedLadder->m_topForwardArea != NULL;
		connected += m_markedLadder->m_topLeftArea != NULL;
		connected += m_markedLadder->m_topRightArea != NULL;
		connected += m_markedLadder->m_topBehindArea != NULL;
		connected += m_markedLadder->m_bottomArea != NULL;

		Msg( "Marked Ladder is connected to %d Areas\n", connected );
	}

	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavUnmark( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	player->EmitSound( "EDIT_MARK.Enable" );
	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavBeginArea( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isPlaceMode )
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
		return;
	}

	GetActiveNavArea();

	if (m_isCreatingNavArea)
	{
		m_isCreatingNavArea = false;
		player->EmitSound( "EDIT_BEGIN_AREA.Creating" );
	}
	else if ( m_isCreatingLadder )
	{
		m_isCreatingLadder = false;
		player->EmitSound( "EDIT_BEGIN_AREA.Creating" );
	}
	else if ( m_climbableSurface )
	{
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
		m_isCreatingLadder = true;

		// m_ladderAnchor starting corner
		m_ladderAnchor = m_editCursorPos;
		m_ladderNormal = m_surfaceNormal;
	}
	else
	{
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
		m_isCreatingNavArea = true;

		// m_anchor starting corner
		m_anchor = m_editCursorPos;
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavEndArea( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isPlaceMode )
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
		return;
	}

	if ( m_isCreatingNavArea )
	{
		// create the new nav area
		CNavArea *newArea = new CNavArea( m_anchor, m_editCursorPos );
		TheNavAreaList.AddToTail( newArea );
		TheNavMesh->AddNavArea( newArea );
		player->EmitSound( "EDIT_END_AREA.Creating" );

		newArea->PlaceOnGround( NUM_CORNERS );

		// if we have a marked area, inter-connect the two
		if (GetMarkedArea())
		{
			const Extent &extent = GetMarkedArea()->GetExtent();

			if (m_anchor.x > extent.hi.x && m_editCursorPos.x > extent.hi.x)
			{
				GetMarkedArea()->ConnectTo( newArea, EAST );
				newArea->ConnectTo( GetMarkedArea(), WEST );
			}
			else if (m_anchor.x < extent.lo.x && m_editCursorPos.x < extent.lo.x)
			{
				GetMarkedArea()->ConnectTo( newArea, WEST );
				newArea->ConnectTo( GetMarkedArea(), EAST );
			}
			else if (m_anchor.y > extent.hi.y && m_editCursorPos.y > extent.hi.y)
			{
				GetMarkedArea()->ConnectTo( newArea, SOUTH );
				newArea->ConnectTo( GetMarkedArea(), NORTH );
			}
			else if (m_anchor.y < extent.lo.y && m_editCursorPos.y < extent.lo.y)
			{
				GetMarkedArea()->ConnectTo( newArea, NORTH );
				newArea->ConnectTo( GetMarkedArea(), SOUTH );
			}

			// propogate marked area to new area
			SetMarkedArea( newArea );
		}

		m_isCreatingNavArea = false;
	}
	else if ( m_isCreatingLadder )
	{
		player->EmitSound( "EDIT_END_AREA.Creating" );

		// the two points defining the ladder are m_ladderAnchor and m_editCursorPos.  The normal is m_ladderNormal.
		Vector mins, maxs;

		mins.x = min( m_ladderAnchor.x, m_editCursorPos.x );
		mins.y = min( m_ladderAnchor.y, m_editCursorPos.y );
		mins.z = min( m_ladderAnchor.z, m_editCursorPos.z );

		maxs.x = max( m_ladderAnchor.x, m_editCursorPos.x );
		maxs.y = max( m_ladderAnchor.y, m_editCursorPos.y );
		maxs.z = max( m_ladderAnchor.z, m_editCursorPos.z );

		Vector2D ladderDir = m_ladderNormal.AsVector2D();
		CreateLadder( mins, maxs, &ladderDir );

		m_isCreatingLadder = false;
	}
	else
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
	}

	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavConnect( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if ( m_markedLadder )
		{
			m_markedLadder->ConnectTo( m_selectedArea );
			player->EmitSound( "EDIT_CONNECT.Added" );
		}
		else if ( m_markedArea )
		{
			NavDirType dir = GetMarkedArea()->ComputeDirection( &m_editCursorPos );
			if (dir == NUM_DIRECTIONS)
			{
				player->EmitSound( "EDIT_CONNECT.AllDirections" );
			}
			else
			{
				m_markedArea->ConnectTo( m_selectedArea, dir );
				player->EmitSound( "EDIT_CONNECT.Added" );
			}
		}
		else
		{
			Msg( "To connect areas, mark an area, highlight a second area, then invoke the connect command. Make sure the cursor is directly north, south, east, or west of the marked area." );
			player->EmitSound( "EDIT_CONNECT.AllDirections" );
		}
	}
	else if ( m_selectedLadder )
	{
		if ( m_markedArea )
		{
			m_markedArea->ConnectTo( m_selectedLadder );
			player->EmitSound( "EDIT_CONNECT.Added" );
		}
		else
		{
			Msg( "To connect areas, mark an area, highlight a second area, then invoke the connect command. Make sure the cursor is directly north, south, east, or west of the marked area." );
			player->EmitSound( "EDIT_CONNECT.AllDirections" );
		}
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavDisconnect( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if ( m_markedArea )
		{
			m_markedArea->Disconnect( m_selectedArea );
			m_selectedArea->Disconnect( m_markedArea );
			player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
		}
		else
		{
			if ( m_markedLadder )
			{
				m_markedLadder->Disconnect( m_selectedArea );
				m_selectedArea->Disconnect( m_markedLadder );
				player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
			}
			else
			{
				Msg( "To disconnect areas, mark an area, highlight a second area, then invoke the disconnect command. This will remove all connections between the two areas." );
				player->EmitSound( "EDIT_DISCONNECT.NoMarkedArea" );
			}
		}
	}
	else if ( m_selectedLadder )
	{
		if ( m_markedArea )
		{
			m_markedArea->Disconnect( m_selectedLadder );
			m_selectedLadder->Disconnect( m_markedArea );
			player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
		}
		else
		{
			Msg( "To disconnect areas, mark an area, highlight a second area, then invoke the disconnect command. This will remove all connections between the two areas." );
			player->EmitSound( "EDIT_DISCONNECT.NoMarkedArea" );
		}
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavSplice( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			if (m_selectedArea->SpliceEdit( GetMarkedArea() ))
				player->EmitSound( "EDIT_SPLICE.MarkedArea" );
			else
				player->EmitSound( "EDIT_SPLICE.NoMarkedArea" );
		}
		else
		{
			Msg( "To splice, mark an area, highlight a second area, then invoke the splice command to create an area between them" );
			player->EmitSound( "EDIT_SPLICE.NoMarkedArea" );
		}
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavToggleAttribute( NavAttributeType attribute )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		player->EmitSound( "EDIT.ToggleAttribute" );
		m_selectedArea->SetAttributes( m_selectedArea->GetAttributes() ^ attribute );
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavTogglePlaceMode( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder )
		return;

	player->EmitSound( "EDIT_TOGGLE_PLACE_MODE" );
	m_isPlaceMode = !m_isPlaceMode;

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavPlaceFloodFill( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || !m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		PlaceFloodFillFunctor pff( m_selectedArea );
		SearchSurroundingAreas( m_selectedArea, m_selectedArea->GetCenter(), pff );
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavPlacePick( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || !m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		player->EmitSound( "EDIT_PLACE_PICK" );
		TheNavMesh->SetNavPlace( m_selectedArea->GetPlace() );
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavTogglePlacePainting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || !m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if (m_isPlacePainting)
		{
			m_isPlacePainting = false;
			player->EmitSound( "Bot.EditSwitchOff" );
		}
		else
		{
			m_isPlacePainting = true;

			player->EmitSound( "Bot.EditSwitchOn" );

			// paint the initial area
			m_selectedArea->SetPlace( TheNavMesh->GetNavPlace() );
		}
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMarkUnnamed( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			player->EmitSound( "EDIT_MARK_UNNAMED.Enable" );
			SetMarkedArea( NULL );
		}
		else
		{
			SetMarkedArea( NULL );
			FOR_EACH_LL( TheNavAreaList, it )
			{
				CNavArea *area = TheNavAreaList[ it ];

				if ( area->GetPlace() == 0 )
				{
					SetMarkedArea( area );
					break;
				}
			}
			if ( !GetMarkedArea() )
			{
				player->EmitSound( "EDIT_MARK_UNNAMED.NoMarkedArea" );
			}
			else
			{
				player->EmitSound( "EDIT_MARK_UNNAMED.MarkedArea" );

				int connected = 0;
				connected += GetMarkedArea()->GetAdjacentCount( NORTH );
				connected += GetMarkedArea()->GetAdjacentCount( SOUTH );
				connected += GetMarkedArea()->GetAdjacentCount( EAST );
				connected += GetMarkedArea()->GetAdjacentCount( WEST );

				int totalUnnamedAreas = 0;
				FOR_EACH_LL( TheNavAreaList, it )
				{
					CNavArea *area = TheNavAreaList[ it ];
					if ( area->GetPlace() == 0 )
					{
						++totalUnnamedAreas;
					}
				}

				Msg( "Marked Area is connected to %d other Areas - there are %d total unnamed areas\n", connected, totalUnnamedAreas );
			}
		}
	}

	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerSelect( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			int corner = (m_markedCorner + 1) % (NUM_CORNERS + 1);
			m_markedCorner = (NavCornerType)corner;
			player->EmitSound( "EDIT_SELECT_CORNER.MarkedArea" );
		}
		else
		{
			player->EmitSound( "EDIT_SELECT_CORNER.NoMarkedArea" );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerRaise( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			GetMarkedArea()->RaiseCorner( m_markedCorner, 1 );
			player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
		}
		else
		{
			player->EmitSound( "EDIT_MOVE_CORNER.NoMarkedArea" );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerLower( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			GetMarkedArea()->RaiseCorner( m_markedCorner, -1 );
			player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
		}
		else
		{
			player->EmitSound( "EDIT_MOVE_CORNER.NoMarkedArea" );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerPlaceOnGround( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedArea )
	{
		float inset = 0.0f;
		if ( engine->Cmd_Argc() == 2 )
			inset = atof(engine->Cmd_Argv(1));
		if ( m_markedArea )
		{
			m_markedArea->PlaceOnGround( m_markedCorner, inset );
		}
		else
		{
			m_selectedArea->PlaceOnGround( NUM_CORNERS, inset );
		}
		player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
	}
	else
	{
		player->EmitSound( "EDIT_MOVE_CORNER.NoMarkedArea" );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavWarpToMark( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	if (GetMarkedArea())
	{
		if ( ( player->IsDead() || player->IsObserver() ) && player->GetObserverMode() == OBS_MODE_ROAMING )
		{
			Vector origin = GetMarkedArea()->GetCenter() + Vector( 0, 0, 0.75f * HumanHeight );
			UTIL_SetOrigin( player, origin );
		}
		else
		{
			player->EmitSound( "EDIT_WARP_TO_MARK" );
		}
	}
	else if (GetMarkedLadder())
	{
		CNavLadder *ladder = GetMarkedLadder();
		if ( ( player->IsDead() || player->IsObserver() ) && player->GetObserverMode() == OBS_MODE_ROAMING )
		{
			Vector origin = (ladder->m_top + ladder->m_bottom)/2;
			origin.x += ladder->m_dirVector.x * GenerationStepSize;
			origin.y += ladder->m_dirVector.y * GenerationStepSize;
			UTIL_SetOrigin( player, origin );
		}
		else
		{
			player->EmitSound( "EDIT_WARP_TO_MARK" );
		}
	}
	else
	{
		player->EmitSound( "EDIT_WARP_TO_MARK" );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavLadderFlip( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( m_isCreatingNavArea || m_isCreatingLadder || m_isPlaceMode )
		return;

	GetActiveNavArea();

	if ( m_selectedLadder )
	{
		CNavArea *area;

		// flip direction
		player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
		m_selectedLadder->m_dir = OppositeDirection( m_selectedLadder->m_dir );
		DirectionToVector2D( m_selectedLadder->m_dir, &m_selectedLadder->m_dirVector );

		// and reverse ladder's area pointers
		area = m_selectedLadder->m_topBehindArea;
		m_selectedLadder->m_topBehindArea = m_selectedLadder->m_topForwardArea;
		m_selectedLadder->m_topForwardArea = area;

		area = m_selectedLadder->m_topRightArea;
		m_selectedLadder->m_topRightArea = m_selectedLadder->m_topLeftArea;
		m_selectedLadder->m_topLeftArea = area;
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------