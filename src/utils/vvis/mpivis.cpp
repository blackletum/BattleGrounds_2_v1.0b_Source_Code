//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include <windows.h>
#include "vis.h"
#include "threads.h"
#include "stdlib.h"
#include "pacifier.h"
#include "mpi_stats.h"
#include "vmpi.h"
#include "vmpi_dispatch.h"
#include "vmpi_filesystem.h"
#include "vmpi_distribute_work.h"
#include "iphelpers.h"
#include "threadhelpers.h"
#include "vstdlib/random.h"
#include "vmpi_tools_shared.h"


#define VMPI_VVIS_PACKET_ID						1
	// Sub packet IDs.
	#define VMPI_SUBPACKETID_DISCONNECT_NOTIFY	3	// We send ourselves this when there is a disconnect.
	#define VMPI_SUBPACKETID_BASEPORTALVIS		5
	#define VMPI_SUBPACKETID_PORTALFLOW			6
	#define VMPI_BASEPORTALVIS_RESULTS			7
	#define VMPI_BASEPORTALVIS_WORKER_DONE		8
	#define VMPI_PORTALFLOW_RESULTS				9
	#define VMPI_SUBPACKETID_BASEPORTALVIS_SYNC	11
	#define VMPI_SUBPACKETID_PORTALFLOW_SYNC	12
	#define VMPI_SUBPACKETID_MC_ADDR			13

// DistributeWork owns this packet ID.
#define VMPI_DISTRIBUTEWORK_PACKETID			2


extern bool fastvis;

// The worker waits until these are true.
bool g_bBasePortalVisSync = false;
bool g_bPortalFlowSync = false;

CUtlVector<char> g_BasePortalVisResultsFilename;

CCycleCount g_CPUTime;


// This stuff is all for the multicast channel the master uses to send out the portal results.
ISocket *g_pPortalMCSocket = NULL;
CIPAddr g_PortalMCAddr;
bool g_bGotMCAddr = false;
HANDLE g_hMCThread = NULL;
CEvent g_MCThreadExitEvent;
unsigned long g_PortalMCThreadUniqueID = 0;
int g_nMulticastPortalsReceived = 0;


// Handle VVIS packets.
bool VVIS_DispatchFn( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	switch ( pBuf->data[1] )
	{
		case VMPI_SUBPACKETID_MC_ADDR:
		{
			pBuf->setOffset( 2 );
			pBuf->read( &g_PortalMCAddr, sizeof( g_PortalMCAddr ) );
			g_bGotMCAddr = true;
			return true;
		}

		case VMPI_SUBPACKETID_DISCONNECT_NOTIFY:
		{
			// This is just used to cause nonblocking dispatches to jump out so loops like the one
			// in AppBarrier can handle the fact that there are disconnects.
			return true;
		}
		
		case VMPI_SUBPACKETID_BASEPORTALVIS_SYNC:
		{
			g_bBasePortalVisSync = true;
			return true;
		}
		
		case VMPI_SUBPACKETID_PORTALFLOW_SYNC:
		{
			g_bPortalFlowSync = true;
			return true;
		}
		
		case VMPI_BASEPORTALVIS_RESULTS:
		{
			const char *pFilename = &pBuf->data[2];
			g_BasePortalVisResultsFilename.CopyArray( pFilename, strlen( pFilename ) + 1 );
			return true;
		}

		default:
		{
			return false;
		}
	}
}
CDispatchReg g_VVISDispatchReg( VMPI_VVIS_PACKET_ID, VVIS_DispatchFn ); // register to handle the messages we want
CDispatchReg g_DistributeWorkReg( VMPI_DISTRIBUTEWORK_PACKETID, DistributeWorkDispatch );



void VMPI_DeletePortalMCSocket()
{
	// Stop the thread if it exists.
	if ( g_hMCThread )
	{
		g_MCThreadExitEvent.SetEvent();
		WaitForSingleObject( g_hMCThread, INFINITE );
		CloseHandle( g_hMCThread );
		g_hMCThread = NULL;
	}

	if ( g_pPortalMCSocket )
	{
		g_pPortalMCSocket->Release();
		g_pPortalMCSocket = NULL;
	}
}


void VVIS_SetupMPI( int argc, char **argv )
{
	if ( !VMPI_FindArg( argc, argv, "-mpi", "" ) )
		return;

	CmdLib_AtCleanup( VMPI_Stats_Term );
	CmdLib_AtCleanup( VMPI_DeletePortalMCSocket );

	VMPI_Stats_InstallSpewHook();

	// Force local mode?
	VMPIRunMode mode = VMPI_RUN_NETWORKED_MULTICAST;
	if ( VMPI_FindArg( argc, argv, "-mpi_broadcast", "" ) )
		mode = VMPI_RUN_NETWORKED_BROADCAST;
	if ( VMPI_FindArg( argc, argv, "-mpi_local", "" ) )
		mode = VMPI_RUN_LOCAL;

	//
	//  Extract mpi specific arguments
	//
	Msg( "MPI - Establishing connections...\n" );
	if ( !VMPI_Init( argc, argv, "dependency_info_vvis.txt", HandleMPIDisconnect, mode ) )
	{
		Error( "MPI_Init failed." );
	}

	if ( !g_bMPI_NoStats )
		StatsDB_InitStatsDatabase( argc, argv, "dbinfo_vvis.txt" );
}


void ProcessBasePortalVis( int iThread, int iPortal, MessageBuffer *pBuf )
{
	CTimeAdder adder( &g_CPUTime );
		BasePortalVis( iThread, iPortal );
	adder.End();

	// Send my result to the master
	portal_t * p = &portals[iPortal];
	pBuf->write( p->portalfront, portalbytes );
	pBuf->write( p->portalflood, portalbytes );
}


void ReceiveBasePortalVis( int iWorkUnit, MessageBuffer *pBuf, int iWorker )
{
	portal_t * p = &portals[iWorkUnit];
	if ( p->portalflood != 0 || p->portalfront != 0 || p->portalvis != 0) 
	{
		Msg("Duplicate portal %d\n", iWorkUnit);
	}
	
	if ( pBuf->getLen() - pBuf->getOffset() != portalbytes*2 )
		Error( "Invalid packet in ReceiveBasePortalVis." );

	//
	// allocate memory for bitwise vis solutions for this portal
	//
	p->portalfront = (byte*)malloc (portalbytes);
	pBuf->read( p->portalfront, portalbytes );
	
	p->portalflood = (byte*)malloc (portalbytes);
	pBuf->read( p->portalflood, portalbytes );

	p->portalvis = (byte*)malloc (portalbytes);
	memset (p->portalvis, 0, portalbytes);

	p->nummightsee = CountBits( p->portalflood, g_numportals*2 );
}


//-----------------------------------------
//
// Run BasePortalVis across all available processing nodes
// Then collect and redistribute the results.
//
void RunMPIBasePortalVis()
{
	int i;

	Msg( "\n\nportalbytes: %d\nNum Work Units: %d\nTotal data size: %d\n", portalbytes, g_numportals*2, portalbytes*g_numportals*2 );
    Msg("%-20s ", "BasePortalVis:");
	StartPacifier("");


	VMPI_SetCurrentStage( "RunMPIBasePortalVis" );

	// Note: we're aiming for about 1500 portals in a map, so about 3000 work units.
	g_CPUTime.Init();
	double elapsed = DistributeWork( 
		g_numportals * 2,		// # work units
		VMPI_DISTRIBUTEWORK_PACKETID,	// packet ID
		ProcessBasePortalVis,	// Worker function to process work units
		ReceiveBasePortalVis	// Master function to receive work results
		);

	EndPacifier( false );
	Msg( " (%d)\n", (int)elapsed );
	
	//
	// Distribute the results to all the workers.
	//
	if ( g_bMPIMaster )
	{
		if ( !fastvis )
		{
			VMPI_SetCurrentStage( "SendPortalResults" );

			// Store all the portal results in a temp file and multicast that to the workers.
			CUtlVector<char> allPortalData;
			allPortalData.SetSize( g_numportals * 2 * portalbytes * 2 );

			char *pOut = allPortalData.Base();
			for ( i=0; i < g_numportals * 2; i++) 
			{
				portal_t *p = &portals[i];
				
				memcpy( pOut, p->portalfront, portalbytes );
				pOut += portalbytes;
				
				memcpy( pOut, p->portalflood, portalbytes );
				pOut += portalbytes;
			}

			const char *pVirtualFilename = "--portal-results--";
			VMPI_FileSystem_CreateVirtualFile( pVirtualFilename, allPortalData.Base(), allPortalData.Count() );

			char cPacketID[2] = { VMPI_VVIS_PACKET_ID, VMPI_BASEPORTALVIS_RESULTS };
			VMPI_Send2Chunks( cPacketID, sizeof( cPacketID ), pVirtualFilename, strlen( pVirtualFilename ) + 1, VMPI_PERSISTENT );
		}
	}
	else
	{
		VMPI_SetCurrentStage( "RecvPortalResults" );

		// Wait until we've received the filename from the master.
		while ( g_BasePortalVisResultsFilename.Count() == 0 )
		{
			VMPI_DispatchNextMessage();
		}

		// Open 
		FileHandle_t fp = g_pFileSystem->Open( g_BasePortalVisResultsFilename.Base(), "rb", VMPI_VIRTUAL_FILES_PATH_ID );
		if ( !fp )
			Error( "Can't open '%s' to read portal info.", g_BasePortalVisResultsFilename.Base() );

		for ( i=0; i < g_numportals * 2; i++) 
		{
			portal_t *p = &portals[i];

			p->portalfront = (byte*)malloc (portalbytes);
			g_pFileSystem->Read( p->portalfront, portalbytes, fp );
			
			p->portalflood = (byte*)malloc (portalbytes);
			g_pFileSystem->Read( p->portalflood, portalbytes, fp );
		
			p->portalvis = (byte*)malloc (portalbytes);
			memset (p->portalvis, 0, portalbytes);
		
			p->nummightsee = CountBits (p->portalflood, g_numportals*2);
		}

		g_pFileSystem->Close( fp );
	}

	
	if ( !g_bMPIMaster )
	{
		Msg( "\n%% worker CPU utilization during BasePortalVis: %.1f\n", 
			(g_CPUTime.GetSeconds() * 100.0f / elapsed) / numthreads );
	}
}



void ProcessPortalFlow( int iThread, int iPortal, MessageBuffer *pBuf )
{
	// Process Portal and distribute results
	CTimeAdder adder( &g_CPUTime );
		PortalFlow( iThread, iPortal );
	adder.End();

	// Send my result to root and potentially the other slaves
	// The slave results are read in RecursiveLeafFlow
	//
	portal_t * p = sorted_portals[iPortal];
	pBuf->write( p->portalvis, portalbytes );
}


void ReceivePortalFlow( int iWorkUnit, MessageBuffer *pBuf, int iWorker )
{
	portal_t *p = sorted_portals[iWorkUnit];

	if ( p->status != stat_done )
	{
		pBuf->read( p->portalvis, portalbytes );
		p->status = stat_done;

		
		// Multicast the status of this portal out.
		if ( g_pPortalMCSocket )
		{
			char cPacketID[2] = { VMPI_VVIS_PACKET_ID, VMPI_PORTALFLOW_RESULTS }; 
			void *chunks[4] = { cPacketID, &g_PortalMCThreadUniqueID, &iWorkUnit, p->portalvis };
			int chunkLengths[4] = { sizeof( cPacketID ), sizeof( g_PortalMCThreadUniqueID ), sizeof( iWorkUnit ), portalbytes };

			g_pPortalMCSocket->SendChunksTo( &g_PortalMCAddr, chunks, chunkLengths, ARRAYSIZE( chunks ) );
		}
	}
}


DWORD WINAPI PortalMCThreadFn( LPVOID p )
{
	CUtlVector<char> data;
	data.SetSize( portalbytes + 128 );

	DWORD waitTime = 0;
	while ( WaitForSingleObject( g_MCThreadExitEvent.GetEventHandle(), waitTime ) != WAIT_OBJECT_0 )
	{
		CIPAddr ipFrom;
		int len = g_pPortalMCSocket->RecvFrom( data.Base(), data.Count(), &ipFrom );
		if ( len == -1 )
		{
			waitTime = 20;
		}
		else
		{
			// These lengths must match exactly what is sent in ReceivePortalFlow.
			if ( len == 2 + sizeof( g_PortalMCThreadUniqueID ) + sizeof( int ) + portalbytes )
			{
				// Perform more validation...
				if ( data[0] == VMPI_VVIS_PACKET_ID && data[1] == VMPI_PORTALFLOW_RESULTS )
				{
					if ( *((unsigned long*)&data[2]) == g_PortalMCThreadUniqueID )
					{
						int iWorkUnit = *((int*)&data[6]);
						if ( iWorkUnit >= 0 && iWorkUnit < g_numportals*2 )
						{
							portal_t *p = sorted_portals[iWorkUnit];
							if ( p )
							{
								++g_nMulticastPortalsReceived;
								memcpy( p->portalvis, &data[10], portalbytes );
								p->status = stat_done;
								waitTime = 0;
							}
						}
					}
				}
			}
		}
	}
	
	return 0;
}


//-----------------------------------------
//
// Run PortalFlow across all available processing nodes
//
void RunMPIPortalFlow()
{
    Msg( "%-20s ", "MPIPortalFlow:" );
	StartPacifier("");


	// Workers wait until we get the MC socket address.
	g_PortalMCThreadUniqueID = StatsDB_GetUniqueJobID();
	if ( g_bMPIMaster )
	{
		CCycleCount cnt;
		cnt.Sample();
		CUniformRandomStream randomStream;
		randomStream.SetSeed( cnt.GetMicroseconds() );

		g_PortalMCAddr.port = randomStream.RandomInt( 22000, 25000 ); // Pulled out of something else.
		g_PortalMCAddr.ip[0] = (unsigned char)RandomInt( 225, 238 );
		g_PortalMCAddr.ip[1] = (unsigned char)RandomInt( 0, 255 );
		g_PortalMCAddr.ip[2] = (unsigned char)RandomInt( 0, 255 );
		g_PortalMCAddr.ip[3] = (unsigned char)RandomInt( 3, 255 );

		g_pPortalMCSocket = CreateIPSocket();
		for ( int i=0; i < 5; i++ )
		{
			if ( g_pPortalMCSocket->BindToAny( randomStream.RandomInt( 20000, 30000 ) ) )
				break;
		}
		if ( i == 5 )
		{
			Error( "RunMPIPortalFlow: can't open a socket to multicast on." );
		}

		char cPacketID[2] = { VMPI_VVIS_PACKET_ID, VMPI_SUBPACKETID_MC_ADDR };
		VMPI_Send2Chunks( cPacketID, sizeof( cPacketID ), &g_PortalMCAddr, sizeof( g_PortalMCAddr ), VMPI_PERSISTENT );
	}
	else
	{
		VMPI_SetCurrentStage( "wait for MC address" );

		while ( !g_bGotMCAddr )
		{
			VMPI_DispatchNextMessage();
		}

		// Open our multicast receive socket.
		g_pPortalMCSocket = CreateMulticastListenSocket( g_PortalMCAddr );
		if ( !g_pPortalMCSocket )
		{
			char err[512];
			IP_GetLastErrorString( err, sizeof( err ) );
			Error( "RunMPIPortalFlow: CreateMulticastListenSocket failed. (%s).", err );
		}

		// Make a thread to listen for the data on the multicast socket.
		DWORD dwDummy = 0;
		g_MCThreadExitEvent.Init( false, false );
		g_hMCThread = CreateThread( 
			NULL,
			0,
			PortalMCThreadFn,
			NULL,
			0,
			&dwDummy );

		if ( !g_hMCThread )
		{
			Error( "RunMPIPortalFlow: CreateThread failed for multicast receive thread." );
		}			
	}

	VMPI_SetCurrentStage( "RunMPIBasePortalFlow" );

	g_CPUTime.Init();
	double elapsed = DistributeWork( 
		g_numportals * 2,		// # work units
		VMPI_DISTRIBUTEWORK_PACKETID,	// packet ID
		ProcessPortalFlow,		// Worker function to process work units
		ReceivePortalFlow		// Master function to receive work results
		);

	// Stop the multicast stuff.
	VMPI_DeletePortalMCSocket();

	if( !g_bMPIMaster )
	{
		Msg( "Received %d (out of %d) portals from multicast.\n", g_nMulticastPortalsReceived, g_numportals * 2 );

		Msg( "%% worker CPU utilization during PortalFlow: %.1f\n", 
			(g_CPUTime.GetSeconds() * 100.0f / elapsed) / numthreads );

		Msg( "VVIS worker finished. Over and out.\n" );
		VMPI_SetCurrentStage( "worker done" );

		CmdLib_Exit( 0 );
	}

	EndPacifier( false );
	Msg( " (%d)\n", (int)elapsed );
}

