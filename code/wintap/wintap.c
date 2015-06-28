/* wintap.c : Defines the entry point for the application.
*
*
* Copyright 2013-2015, Defensive Depth (Defensivedepth.com)
*
* All rights reserved.
* 
* This file is part of WinTAP.  
* WinTAP is dual-licensed under the MIT License http://opensource.org/licenses/MIT, 
* as well as the GNU General Public License, version 3.
*
* GPL 3:
* WinTAP is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* WinTAP is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with WinTAP.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
* This simple program to setup a uni-directional, user-level bridge to 
* create a soft-tap, like daemonlogger.
* It opens two adapters specified by the user and starts a packet 
* copying thread. It receives packets from adapter 1 and sends them down
* to adapter 2.
*/

// options:
//        -e: Enumerate devices
//		  -m <in> <out> Mirror traffic from <in> interface to <out> interface
//

#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4127)   // conditional expression is constant

#include <signal.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winioctl.h>
#include <memory.h>
#include <ctype.h>
#include <malloc.h>

#include <winerror.h>
#include <winsock.h>
#include <intsafe.h>

#include <ntddndis.h>
#include "protuser.h"

// this is needed to prevent compiler from complaining about
// pragma prefast statements below
#ifndef _PREFAST_
#pragma warning(disable:4068)
#endif

#ifndef NDIS_STATUS
#define NDIS_STATUS     ULONG
#endif

#if DBG
#define DEBUGP(stmt)    printf stmt
#else
#define DEBUGP(stmt)
#endif

#define PRINTF(stmt)    printf stmt

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN                    6
#endif

#define MAX_NDIS_DEVICE_NAME_LEN        256

CHAR            NdisProtDevice[] = "\\\\.\\\\Wintap";
CHAR *          pNdisProtDevice = &NdisProtDevice[0];

BOOLEAN         DoEnumerate = FALSE;
BOOLEAN         DoReads = FALSE;
INT             NumberOfPackets = -1;
ULONG           PacketLength = 65536;	// Portion of the packet to capture.
// 65536 grants that the whole packet will be captured on every link layer.

UCHAR           SrcMacAddr[MAC_ADDR_LEN];
UCHAR           DstMacAddr[MAC_ADDR_LEN];
UCHAR           FakeSrcMacAddr[MAC_ADDR_LEN] = {0};

BOOLEAN         bDstMacSpecified = FALSE;
//CHAR *          pNdisDeviceName = "JUNK";
USHORT          EthType = 0x8e88;
BOOLEAN         bUseFakeAddress = FALSE;

HANDLE      DeviceHandleIn;
HANDLE      DeviceHandleOut;
INT			DeviceIndex1, DeviceIndex2;

#include <pshpack1.h>

typedef struct _ETH_HEADER
{
	UCHAR       DstAddr[MAC_ADDR_LEN];
	UCHAR       SrcAddr[MAC_ADDR_LEN];
	USHORT      EthType;
} ETH_HEADER, *PETH_HEADER;

#include <poppack.h>


/* Storage data structure used to pass parameters to the threads */
typedef struct _MIRROR_ADAPTERS
{
	unsigned int state;        /* Some simple state information */
	HANDLE hNdisDeviceIn;
	HANDLE hNdisDeviceOut;
} MIRROR_ADAPTERS, *PMIRROR_ADAPTERS;

/* Prototypes */
DWORD WINAPI CaptureAndForwardThread(LPVOID lpParameter);
void ctrlc_handler(int sig);

/* This prevents the two threads to mess-up when they do printfs */
CRITICAL_SECTION print_cs;

/* Thread handlers. Global because we wait on the threads from the CTRL+C handler */
HANDLE threads[2];

/* This global variable tells the forwarder threads they must terminate */
volatile int kill_forwaders = 0;

/*******************************************************************/

HANDLE
	OpenHandle(
	_In_ PSTR pDeviceName
	)
{
	DWORD   DesiredAccess;
	DWORD   ShareMode;
	LPSECURITY_ATTRIBUTES   lpSecurityAttributes = NULL;

	DWORD   CreationDistribution;
	DWORD   FlagsAndAttributes;
	HANDLE  Handle;
	DWORD   BytesReturned;

	DesiredAccess = GENERIC_READ|GENERIC_WRITE;
	ShareMode = 0;
	CreationDistribution = OPEN_EXISTING;
	FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

	Handle = CreateFileA(
		pDeviceName,
		DesiredAccess,
		ShareMode,
		lpSecurityAttributes,
		CreationDistribution,
		FlagsAndAttributes,
		NULL
		);
	if (Handle == INVALID_HANDLE_VALUE)
	{
		DEBUGP(("Creating file failed, error %x\n", GetLastError()));
		return Handle;
	}
	//
	//  Wait for the driver to finish binding.
	//
	if (!DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_BIND_WAIT,
		NULL,
		0,
		NULL,
		0,
		&BytesReturned,
		NULL))
	{
		DEBUGP(("IOCTL_NDISIO_BIND_WAIT failed, error %x\n", GetLastError()));
		CloseHandle(Handle);
		Handle = INVALID_HANDLE_VALUE;
	}

	return (Handle);
}


BOOL
	OpenNdisDevice(
	_In_ HANDLE                     Handle,
	_In_ PCTSTR                      pszDeviceName,
	_In_ SIZE_T						lDeviceNameLength
	)
{
	DWORD   BytesReturned;
	DWORD	DeviceNameLength;
	HRESULT hr = S_OK;


	PRINTF(("\nTrying to access NDIS Device: %ws \n", pszDeviceName));
	OutputDebugString(_T("wintap: "));
	OutputDebugString(_T("Trying to access NDIS Device: "));
	OutputDebugString(pszDeviceName);
	OutputDebugString(_T("\n"));
	
	hr = SIZETToDWord(lDeviceNameLength, &DeviceNameLength);
	if(FAILED(hr))
	{
		PRINTF(("\nFailed to access NDIS Device: %ws \n", pszDeviceName));
		return FALSE;
	}

	return (DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_OPEN_DEVICE,
		(LPVOID)pszDeviceName,
		DeviceNameLength,
		NULL,
		0,
		&BytesReturned,
		NULL));

}

_Success_(return)
	BOOL
	GetSrcMac(
	_In_ HANDLE  Handle,
	_Out_writes_bytes_(MAC_ADDR_LEN) PUCHAR  pSrcMacAddr
	)
{
	DWORD       BytesReturned;
	BOOLEAN     bSuccess;
	UCHAR       QueryBuffer[sizeof(NDISPROT_QUERY_OID) + MAC_ADDR_LEN];
	PNDISPROT_QUERY_OID  pQueryOid;


	DEBUGP(("Trying to get src mac address\n"));

	pQueryOid = (PNDISPROT_QUERY_OID)&QueryBuffer[0];
	pQueryOid->Oid = OID_802_3_CURRENT_ADDRESS;
	pQueryOid->PortNumber = 0;

	bSuccess = (BOOLEAN)DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_QUERY_OID_VALUE,
		(LPVOID)&QueryBuffer[0],
		sizeof(QueryBuffer),
		(LPVOID)&QueryBuffer[0],
		sizeof(QueryBuffer),
		&BytesReturned,
		NULL);

	if (bSuccess)
	{
		DEBUGP(("GetSrcMac: IoControl success, BytesReturned = %d\n",
			BytesReturned));

#pragma warning(suppress:6202) // buffer overrun warning - enough space allocated in QueryBuffer
		memcpy(pSrcMacAddr, pQueryOid->Data, MAC_ADDR_LEN);
	}
	else
	{
		DEBUGP(("GetSrcMac: IoControl failed: %d\n", GetLastError()));
	}

	return (bSuccess);
}


_Success_(return)
	INT
	EnumerateDevices(
	_In_ HANDLE  Handle
	)
{
	typedef __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) QueryBindingCharBuf;
	QueryBindingCharBuf		Buf[1024];
	DWORD       		BufLength = sizeof(Buf);
	DWORD       		BytesWritten;
	DWORD       		i;
	PNDISPROT_QUERY_BINDING 	pQueryBinding;

	pQueryBinding = (PNDISPROT_QUERY_BINDING)Buf;

	i = 0;
	for (pQueryBinding->BindingIndex = i;
		/* NOTHING */;
		pQueryBinding->BindingIndex = ++i)
	{
		if (DeviceIoControl(
			Handle,
			IOCTL_NDISPROT_QUERY_BINDING,
			pQueryBinding,
			sizeof(NDISPROT_QUERY_BINDING),
			Buf,
			BufLength,
			&BytesWritten,
			NULL))
		{
			PRINTF(("%2d. %ws\n     - %ws\n",
				pQueryBinding->BindingIndex,
				(WCHAR *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset),
				(WCHAR *)((PUCHAR )pQueryBinding + pQueryBinding->DeviceDescrOffset)));

			memset(Buf, 0, BufLength);
		}
		else
		{
			ULONG   rc = GetLastError();
			if (rc != ERROR_NO_MORE_ITEMS)
			{
				PRINTF(("\nNo interfaces found! Make sure WinTap driver is installed.\n, error %d\n", rc));
			}
			break;
		}
	}

	return i;
}


BOOL
	GetDevice(
	_In_ HANDLE  Handle,
	_In_ int  bindingIndex,
	_Out_ PTSTR pDeviceName,
	_Out_ PSIZE_T pDeviceNameLength
	)
{
	typedef __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) QueryBindingCharBuf;
	QueryBindingCharBuf		Buf[1024];

	BOOLEAN				bSuccess;
	DWORD       		BufLength = sizeof(Buf);
	DWORD       		BytesWritten;
	PNDISPROT_QUERY_BINDING 	pQueryBinding;

	pQueryBinding = (PNDISPROT_QUERY_BINDING)Buf;
	pQueryBinding->BindingIndex = bindingIndex;


	bSuccess = (BOOLEAN)DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_QUERY_BINDING,
		pQueryBinding,
		sizeof(NDISPROT_QUERY_BINDING),
		Buf,
		BufLength,
		&BytesWritten,
		NULL);
	if (bSuccess)   
	{
		PRINTF(("%2d. %ws\n     - %ws\n",
			pQueryBinding->BindingIndex,
			(WCHAR *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset),
			(WCHAR *)((PUCHAR )pQueryBinding + pQueryBinding->DeviceDescrOffset)));

#pragma warning(suppress:6202) // buffer overrun warning - enough space allocated in QueryBuffer
		memcpy(pDeviceName, ((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset), pQueryBinding->DeviceNameLength);
		*pDeviceNameLength = pQueryBinding->DeviceNameLength;
		memset(Buf, 0, BufLength);

		return bSuccess;
	}
	else
	{
		ULONG   rc = GetLastError();
		if (rc != ERROR_NO_MORE_ITEMS)
		{
			PRINTF(("\nNo interfaces found! Make sure WinTap driver is installed.\n, error %d\n", rc));
		}
		return bSuccess;
	}
}

BOOL OpenTapDevice()
{
	DeviceHandleIn = OpenHandle(pNdisProtDevice);
	if (DeviceHandleIn == INVALID_HANDLE_VALUE)
	{
		PRINTF(("Failed to open source device %s\n", pNdisProtDevice));
		return FALSE;
	}

	DeviceHandleOut = OpenHandle(pNdisProtDevice);
	if (DeviceHandleOut == INVALID_HANDLE_VALUE)
	{
		PRINTF(("Failed to open destination device %s\n", pNdisProtDevice));
		return FALSE;
	}

	return TRUE;
}

BOOL GetSrcDst(
	_In_ INT MaxDeviceIndex,
	_In_ BOOL bIndexDefined)
{
	/* Get the first interface number*/
	PRINTF(("\nEnter the number of the first interface to use (0-%d):",MaxDeviceIndex-1));

	if( !bIndexDefined )
	{
		scanf_s("%d", &DeviceIndex1);
	}
	else
	{
		printf("%d\n", DeviceIndex1);
	}

	if(DeviceIndex1 < 0 || DeviceIndex1 > MaxDeviceIndex-1 )
	{
		PRINTF(("\nSource Interface number out of range.\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return FALSE;
	}

	/* Get the second interface number*/
	PRINTF(("Enter the number of the second interface to use (0-%d):",MaxDeviceIndex-1));

	if( !bIndexDefined )
	{
		scanf_s("%d", &DeviceIndex2);
	}
	else
	{
		printf("%d\n", DeviceIndex2);
	}

	if(DeviceIndex2 < 0 || DeviceIndex2 > MaxDeviceIndex-1)
	{
		PRINTF(("\nDestination Interface number out of range.\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return FALSE;
	}

	if(DeviceIndex1 == DeviceIndex2 )
	{
		PRINTF(("\nCannot bridge packets on the same interface.\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return FALSE;
	}

	return TRUE;
}

int wmain(int argc, _TCHAR* argv[])
{
	//INT			DeviceIndex1, DeviceIndex2;
	INT			MaxDeviceIndex = 0;
	//HANDLE      DeviceHandleIn;
	//HANDLE      DeviceHandleOut;
	MIRROR_ADAPTERS AdapterCouple;
	TCHAR       NdisDeviceNameIn[MAX_NDIS_DEVICE_NAME_LEN];
	TCHAR       NdisDeviceNameOut[MAX_NDIS_DEVICE_NAME_LEN];
	SIZE_T		DeviceNameLength = 0;
	PSIZE_T		pDeviceNameLength = &DeviceNameLength;
	TCHAR		ConsoleTitle[100] = L"";
	BOOL		bIndexDefined = FALSE;
	BOOL		bDevicesDefined = FALSE;

	DeviceHandleIn = INVALID_HANDLE_VALUE;
	DeviceHandleOut = INVALID_HANDLE_VALUE;

	// Handle mirroring.
	if( argc == 5 && _tcsicmp( argv[1], _T("/src") ) == 0 && _tcsicmp( argv[3], _T("/dst") ) == 0 )
	{
		wcscpy_s(NdisDeviceNameIn, _countof(NdisDeviceNameIn), argv[2]);
		wcscpy_s(NdisDeviceNameOut, _countof(NdisDeviceNameOut), argv[4]);
		bDevicesDefined = TRUE;

	}

	if( argc == 3 )
	{
		DeviceIndex1 = _wtoi(argv[1]);
		DeviceIndex2 = _wtoi(argv[2]);
		bIndexDefined = TRUE;
	}

	if( !OpenTapDevice())
	{
		exit(1);
	}

	PRINTF(("\nAvailable Devices:\n"));
	MaxDeviceIndex = EnumerateDevices(DeviceHandleIn);

	//Get source and destination interfaces

	if( !bDevicesDefined )
	{
		if(!GetSrcDst(MaxDeviceIndex, bIndexDefined))
		{
			return -1;
		}
	}

	/*
	* Open the specified couple of adapters
	*/

	/* 
	* Open the first adapter.
	*/

	if( !bDevicesDefined)
	{
		if (!GetDevice(DeviceHandleIn, DeviceIndex1, NdisDeviceNameIn, pDeviceNameLength))
		{
			PRINTF(("\nUnable to get the source adapter. Device is not supported by WinTap\n"));
			if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
			{
				CloseHandle(DeviceHandleIn);
				CloseHandle(DeviceHandleOut);
			}
			return -1;
		}

		NdisDeviceNameIn[DeviceNameLength] = L'\0';
	}
	else
	{
		DeviceNameLength = min((wcslen(NdisDeviceNameIn) * sizeof(TCHAR)), _countof(NdisDeviceNameIn));
	}

	DEBUGP(("Source Adapter %ws with length: %I64u\n", NdisDeviceNameIn, DeviceNameLength));

	if (!OpenNdisDevice(DeviceHandleIn, NdisDeviceNameIn, DeviceNameLength))
	{
		PRINTF(("\nUnable to open the source adapter %ws. Device not supported by WinTap\n", NdisDeviceNameIn));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}

	PRINTF(("\nOpened source interface successfully!\n"));

	if (!GetSrcMac(DeviceHandleIn, SrcMacAddr))
	{
		PRINTF(("\nUnable to get the source adapter MAC. Device is not supported by WinTap\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}


	PRINTF(("Got source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		SrcMacAddr[0],
		SrcMacAddr[1],
		SrcMacAddr[2],
		SrcMacAddr[3],
		SrcMacAddr[4],
		SrcMacAddr[5]));

	/* 
	* Open the second adapter.
	*/

	if(!bDevicesDefined)
	{

		if (!GetDevice(DeviceHandleOut, DeviceIndex2, NdisDeviceNameOut, pDeviceNameLength))
		{
			PRINTF(("\nUnable to get the destination adapter. Device is not supported by WinTap\n"));
			if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
			{
				CloseHandle(DeviceHandleIn);
				CloseHandle(DeviceHandleOut);
			}
			return -1;
		}

		NdisDeviceNameOut[DeviceNameLength] = L'\0';
	}
	else
	{
		DeviceNameLength = min((wcslen(NdisDeviceNameOut) * sizeof(TCHAR)), _countof(NdisDeviceNameOut));
	}

	DEBUGP(("Destination Adapter %ws with length: %I64u\n", NdisDeviceNameOut, DeviceNameLength));

	if (!OpenNdisDevice(DeviceHandleOut, NdisDeviceNameOut, DeviceNameLength))
	{
		PRINTF(("\nUnable to open the destination adapter. Device is not supported by WinTap\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}

	PRINTF(("\nOpened destination interface successfully!\n"));

	if (!GetSrcMac(DeviceHandleOut, DstMacAddr))
	{
		PRINTF(("\nUnable to get the destination adapter MAC. Device not supported by WinTap\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}


	PRINTF(("Got destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		DstMacAddr[0],
		DstMacAddr[1],
		DstMacAddr[2],
		DstMacAddr[3],
		DstMacAddr[4],
		DstMacAddr[5]));

	wsprintf( ConsoleTitle, L"Reflecting %02x:%02x:%02x:%02x:%02x:%02x >> %02x:%02x:%02x:%02x:%02x:%02x ", 
		SrcMacAddr[0],
		SrcMacAddr[1],
		SrcMacAddr[2],
		SrcMacAddr[3],
		SrcMacAddr[4],
		SrcMacAddr[5],
		DstMacAddr[0],
		DstMacAddr[1],
		DstMacAddr[2],
		DstMacAddr[3],
		DstMacAddr[4],
		DstMacAddr[5]);
	SetConsoleTitle( ConsoleTitle );

	/* 
	* Start the threads that will forward the packets 
	*/

	/* Initialize the critical section that will be used by the threads for console output */
	InitializeCriticalSection(&print_cs);

	/* Init input parameters of the threads */
	AdapterCouple.state = 0;
	AdapterCouple.hNdisDeviceIn = DeviceHandleIn;
	AdapterCouple.hNdisDeviceOut = DeviceHandleOut;

	/* Start first thread */
	if((threads[0] = CreateThread(
		NULL,
		0,
		CaptureAndForwardThread,
		&AdapterCouple,
		0,
		NULL)) == NULL)
	{
		fprintf(stderr, "error creating the first forward thread");

		/* Close the adapters */
		PRINTF(("\nError creating the forward thread\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}

	/*
	* Install a CTRL+C handler that will do the cleanups on exit
	*/
	signal(SIGINT, ctrlc_handler);

	/* 
	* Done! 
	* Wait for the Greek calends... 
	*/
	printf("\nStarted reflecting the adapter...\n");
	Sleep(INFINITE);
	return 0;
}

/*******************************************************************
* Forwarding thread.
* Gets the packets from the input adapter and sends them to the output one.
*******************************************************************/
DWORD WINAPI CaptureAndForwardThread(LPVOID lpParameter)
{
	//	const u_char *pkt_data;
	//	int res = 0;
	MIRROR_ADAPTERS* ad_couple = lpParameter;
	ULONG n_fwd = 0;

	PUCHAR      pReadBuf = NULL;
	INT         ReadCount = 0;
	BOOLEAN     bSuccess = FALSE;
	ULONG       BytesRead;
	DWORD       BytesWritten;
	PETH_HEADER pEthHeader;

	/*
	* Loop receiving packets from the first input adapter
	*/

	while((!kill_forwaders))
	{        
		//PRINTF(("DoReadProc\n"));

		do
		{
			ReadCount = 0;
			//while (TRUE)
			//{
			pReadBuf = malloc(PacketLength);

			if (pReadBuf == NULL)
			{
				PRINTF(("DoReadProc: failed to alloc %d bytes\n", PacketLength));
				break;
			}

			//PRINTF(("DoReadProc: ReadFile %u\n", ad_couple->hNdisDeviceIn));
			bSuccess = (BOOLEAN)ReadFile(
				ad_couple->hNdisDeviceIn,
				(LPVOID)pReadBuf,
				PacketLength,
				&BytesRead,
				NULL);

			if (!bSuccess)
			{
				EnterCriticalSection(&print_cs);
				PRINTF(("DoReadProc: ReadFile failed on Handle %p, error %x\n",
					ad_couple->hNdisDeviceIn, GetLastError()));
				LeaveCriticalSection(&print_cs);
				break;
			}
			else
			{
				ReadCount++;

#if _DEBUG
				/* 
				* Print something, just to show when we have activity.
				* BEWARE: acquiring a critical section and printing strings with printf
				* is something inefficient that you seriously want to avoid in your packet loop!    
				* However, since this DEBUG mode, we privilege visual output to efficiency.
				*/
				EnterCriticalSection(&print_cs);					

				if(ad_couple->state == 0)
					DEBUGP((">>: read pkt - %d bytes\n", BytesRead));
				else
					DEBUGP(("<<: read pkt - %d bytes\n", BytesRead));

				LeaveCriticalSection(&print_cs); 
#endif
				/*
				* Send the just received packet to the output adaper
				*/

				pEthHeader = (PETH_HEADER)pReadBuf;
				memcpy(pEthHeader->DstAddr, DstMacAddr, MAC_ADDR_LEN);

				bSuccess = (BOOLEAN)WriteFile(
					ad_couple->hNdisDeviceOut,
					pReadBuf,
					BytesRead,
					&BytesWritten,
					NULL);
				if (!bSuccess)
				{
					EnterCriticalSection(&print_cs);
					PRINTF(("DoWriteProc: WriteFile failed on Handle %p\n", ad_couple->hNdisDeviceOut));
					LeaveCriticalSection(&print_cs);
					break;
				}
				else
				{
					n_fwd++;
				}

				if (pReadBuf)
				{
					free(pReadBuf);
				}
			}				
			//}
		}
		while (FALSE);

		//if (pReadBuf)
		//{
		//	free(pReadBuf);
		//}

		//PRINTF(("DoReadProc finished: read %d packets\n", ReadCount));
	}


	// We're out of the main loop. Check the reason.

	if (!bSuccess)
	{
		EnterCriticalSection(&print_cs);

		printf("Error capturing the packets\n");
		fflush(stdout);

		LeaveCriticalSection(&print_cs); 
	}
	else
	{
		EnterCriticalSection(&print_cs);

		printf("End of bridging on interface %u. Forwarded packets:%u\n",
			ad_couple->state,
			n_fwd);
		fflush(stdout);

		LeaveCriticalSection(&print_cs);
	}

	return 0;
}

/*******************************************************************
* CTRL+C hanlder.
* We order the threads to die and then we patiently wait for their
* suicide.
*******************************************************************/
void ctrlc_handler(int sig)
{
	/*
	* unused variable
	*/
	(VOID)(sig);

	kill_forwaders = 1;

	WaitForMultipleObjects(2,
		threads,
		TRUE,        /* Wait for all the handles */
		5000);        /* Timeout */

	exit(0);
}