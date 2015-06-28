/*
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
*
* 
*/

#ifndef _PROTINSTALL_H_
#define _PROTINSTALL_H_

////////////////////////////////////////////////////////////////////////////
//// Device Naming String Definitions
//

//
// "Friendly" Name
//
#define NDISPROT_FRIENDLY_NAME_A          "WinTap Protocol Driver"
#define NDISPROT_FRIENDLY_NAME_W          L"WinTap Protocol Driver"

#ifdef UNICODE
#define NDISPROT_FRIENDLY_NAME            NDISPROT_FRIENDLY_NAME_W
#else
#define NDISPROT_FRIENDLY_NAME            NDISPROT_FRIENDLY_NAME_A
#endif

//
// Protocol Name
// -------------
// This is the name of the protocol, and is a parameter passed to
// NdisRegisterProtocol().
//
#define NDISPROT_PROTOCOL_NAME_W  L"WINTAP"
#define NDISPROT_PROTOCOL_NAME_A  "WINTAP"

#ifdef UNICODE
#define NDISPROT_PROTOCOL_NAME NDISPROT_PROTOCOL_NAME_W
#else
#define NDISPROT_PROTOCOL_NAME NDISPROT_PROTOCOL_NAME_A
#endif

//
// Driver WDM Device Object Name
// -----------------------------
// This is the name of the NDISPROT driver WDM device object.
//
#define NDISPROT_WDM_DEVICE_NAME_W  L"\\Device\\WINTAP"
#define NDISPROT_WDM_DEVICE_NAME_A  "\\Device\\WINTAP"

#ifdef UNICODE
#define NDISPROT_WDM_DEVICE_NAME NDISPROT_WDM_DEVICE_NAME_W
#else
#define NDISPROT_WDM_DEVICE_NAME NDISPROT_WDM_DEVICE_NAME_A
#endif

//
// Driver Device WDM Symbolic Link
// -------------------------------
// This is the name of the NDISPROT driver device WDM symbolic link. This
// is a user-visible name that can be used by Win32 applications to access
// the NDISPROT driver WDM interface.
//
#define NDISPROT_WDM_SYMBOLIC_LINK_W  L"\\DosDevices\\WINTAP"
#define NDISPROT_WDM_SYMBOLIC_LINK_A  "\\DosDevices\\WINTAP"

#ifdef UNICODE
#define NDISPROT_WDM_SYMBOLIC_LINK  NDISPROT_WDM_SYMBOLIC_LINK_W
#else
#define NDISPROT_WDM_SYMBOLIC_LINK  NDISPROT_WDM_SYMBOLIC_LINK_A
#endif

//
// Driver WDM Device Filename
// --------------------------
// This is the name that Win32 applications pass to CreateFile to open
// the TPA-REDIR symbolic link.
//
#define NDISPROT_WDM_DEVICE_FILENAME_W  L"\\\\.\\WINTAP"
#define NDISPROT_WDM_DEVICE_FILENAME_A  "\\\\.\\WINTAP"

#ifdef UNICODE
#define NDISPROT_WDM_DEVICE_FILENAME NDISPROT_WDM_DEVICE_FILENAME_W
#else
#define NDISPROT_WDM_DEVICE_FILENAME NDISPROT_WDM_DEVICE_FILENAME_A
#endif

//
// Driver INF File and PnP ID Names
//
#define NDISPROT_SERVICE_PNP_DEVICE_ID_A      "WINTAP_TAPROTO"
#define NDISPROT_SERVICE_PNP_DEVICE_ID_W      L"WINTAP_TAPROTO"

#define NDISPROT_SERVICE_INF_FILE_A           "WTAPDRV"
#define NDISPROT_SERVICE_INF_FILE_W           L"WTAPDRV"

#ifdef UNICODE
#define NDISPROT_SERVICE_PNP_DEVICE_ID        NDISPROT_SERVICE_PNP_DEVICE_ID_W
#define NDISPROT_SERVICE_INF_FILE             NDISPROT_SERVICE_INF_FILE_W
#else
#define NDISPROT_SERVICE_PNP_DEVICE_ID        NDISPROT_SERVICE_PNP_DEVICE_ID_A
#define NDISPROT_SERVICE_INF_FILE             NDISPROT_SERVICE_INF_FILE_A
#endif

/////////////////////////////////////////////////////////////////////////////
//// Registry Path Strings
//

#define NDISPROT_REGSTR_PATH_PARAMETERS_W   L"WINTAP\\Parameters"
#define NDISPROT_REGSTR_PATH_PARAMETERS_A   "WINTAP\\Parameters"

#ifdef UNICODE
#define NDISPROT_REGSTR_PATH_PARAMETERS     NDISPROT_REGSTR_PATH_PARAMETERS_W
#else
#define NDISPROT_REGSTR_PATH_PARAMETERS     NDISPROT_REGSTR_PATH_PARAMETERS_A
#endif


/////////////////////////////////////////////////////////////////////////////
//// Registry Key Strings
//

#define NDISPROT_REGSTR_KEY_PARAMETERS_W   L"Parameters"
#define NDISPROT_REGSTR_KEY_PARAMETERS_A   "Parameters"

#ifdef UNICODE
#define NDISPROT_REGSTR_KEY_PARAMETERS     NDISPROT_REGSTR_KEY_PARAMETERS_W
#else
#define NDISPROT_REGSTR_KEY_PARAMETERS     NDISPROT_REGSTR_KEY_PARAMETERS_A
#endif

#endif // _PROTINSTALL_H_
