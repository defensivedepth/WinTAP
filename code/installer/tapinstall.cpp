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

// tapinstall.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "netcfgapi.h"

#define APP_NAME    L"WTapInstall"

BOOLEAN bVerbose = TRUE;

VOID __cdecl ErrMsg (HRESULT hr, LPCTSTR  lpFmt, ...)
{
    LPTSTR   lpSysMsg;
    TCHAR    buf[400];
    SIZE_T    offset = 0L;
    va_list  vArgList; 

    if ( hr != 0 )
    {
        wsprintf( buf, L"Error %#lx: ", hr );
    }
    else
    {
        buf[0] = 0;
    }

    offset = wcslen( buf );
	
    va_start( vArgList, lpFmt );
    vswprintf_s( buf+offset, _countof(buf)-offset, lpFmt, vArgList );
    va_end( vArgList );
	
	
    if ( hr != 0 ) {
        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpSysMsg,
            0,
            NULL
            );

        if ( lpSysMsg )
        {
            offset = wcslen( buf );
            swprintf_s( buf+offset, _countof(buf)-offset, L"\n\nPossible cause:\n\n" );
            offset = wcslen( buf );
            wcscat_s( buf+offset, _countof(buf)-offset, lpSysMsg );
            LocalFree( (HLOCAL)lpSysMsg );
        }
    }
	
    wprintf( buf );
    return;
}

DWORD GetServiceInfFilePath( 
    IN LPTSTR lpFilename,
    IN DWORD nSize
    )
{
    // Get Path to This Module
    DWORD   nResult;
    WCHAR   szDrive[ _MAX_DRIVE ];
    WCHAR   szDir[ _MAX_DIR ];
    WCHAR   szFname[ _MAX_FNAME ];
    WCHAR   szExt[ _MAX_EXT ];

    nResult = GetModuleFileName( NULL, lpFilename, nSize );

    if( nResult == 0 )
    {
        return 0;
    }

    _wsplitpath_s( lpFilename, szDrive, _countof(szDrive), szDir, _countof(szDir), szFname, _countof(szFname), szExt, _countof(szExt) );
    _wmakepath_s( lpFilename, nSize, szDrive, szDir, NDISPROT_SERVICE_INF_FILE, L".inf" );

    return (DWORD )wcslen( lpFilename );
}

HRESULT InstallSpecifiedComponent(
    IN LPTSTR lpszInfFile,
    IN LPTSTR lpszPnpID,
    IN const GUID *pguidClass
    )
{
    INetCfg    *pnc;
    LPTSTR     lpszApp;
    HRESULT    hr;

    hr = HrGetINetCfg( TRUE, APP_NAME, &pnc, &lpszApp );

    if ( hr == S_OK )
    {
        //
        // Install the network component.
        //
        hr = HrInstallNetComponent(
            pnc,
            lpszPnpID,
            pguidClass,
            lpszInfFile
            );

        if ( (hr == S_OK) || (hr == NETCFG_S_REBOOT) )
        {
            hr = pnc->Apply();
        }
        else
        {
            if ( hr != HRESULT_FROM_WIN32(ERROR_CANCELLED) )
            {
                ErrMsg( hr, L"Couldn't install the network component." );
            }
        }

        HrReleaseINetCfg( pnc, TRUE );
    }
    else
    {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp )
        {
            ErrMsg( hr, L"%s currently holds the lock, try later.", lpszApp );
            CoTaskMemFree( lpszApp );
        }
        else
        {
            ErrMsg( hr, L"Couldn't the get notify object interface." );
        }
    }

    return hr;
}

DWORD InstallDriver()
{
    DWORD   nResult;

    wprintf( L"Installing %s...\n", NDISPROT_FRIENDLY_NAME );

    // Get Path to Service INF File
    // ----------------------------
    // The INF file is assumed to be in the same folder as this application...
    WCHAR   szFileFullPath[ _MAX_PATH ];

    nResult = GetServiceInfFilePath( szFileFullPath, MAX_PATH );

    if( nResult == 0 )
    {
        _tprintf( _T("Unable to get INF file path.\n") );
        return 0;
    }

    wprintf( L"INF Path: %s\n", szFileFullPath );
    HRESULT hr = S_OK;
	wprintf( L"PnpID: %s\n", NDISPROT_SERVICE_PNP_DEVICE_ID );

    hr = InstallSpecifiedComponent(
        szFileFullPath,
        NDISPROT_SERVICE_PNP_DEVICE_ID,
        &GUID_DEVCLASS_NETTRANS
        );

    if( hr != S_OK )
    {
        ErrMsg( hr, L"InstallSpecifiedComponent\n" );
    }

    return 0;
}

DWORD UninstallDriver()
{
    wprintf( L"Uninstalling %s...\n", NDISPROT_FRIENDLY_NAME );

    INetCfg              *pnc;
    INetCfgComponent     *pncc;
    INetCfgClass         *pncClass;
    INetCfgClassSetup    *pncClassSetup;
    LPTSTR               lpszApp;
    GUID                 guidClass;
    OBO_TOKEN            obo;
    HRESULT              hr;

    hr = HrGetINetCfg( TRUE, APP_NAME, &pnc, &lpszApp );

    if ( hr == S_OK ) {

        //
        // Get a reference to the network component to uninstall.
        //
        hr = pnc->FindComponent( NDISPROT_SERVICE_PNP_DEVICE_ID, &pncc );

        if ( hr == S_OK )
        {
            //
            // Get the class GUID.
            //
            hr = pncc->GetClassGuid( &guidClass );

            if ( hr == S_OK )
            {
                //
                // Get a reference to component's class.
                //

                hr = pnc->QueryNetCfgClass( &guidClass, IID_INetCfgClass, (PVOID *)&pncClass );
                if ( hr == S_OK )
                {
                    //
                    // Get the setup interface.
                    //

                    hr = pncClass->QueryInterface( IID_INetCfgClassSetup,
                        (LPVOID *)&pncClassSetup );

                    if ( hr == S_OK )
                    {
                        //
                        // Uninstall the component.
                        //

                        ZeroMemory( &obo, sizeof(OBO_TOKEN) );

                        obo.Type = OBO_USER;

                        hr = pncClassSetup->DeInstall( pncc, &obo, NULL );
                        if ( (hr == S_OK) || (hr == NETCFG_S_REBOOT) )
                        {
                            hr = pnc->Apply();

                            if ( (hr != S_OK) && (hr != NETCFG_S_REBOOT) )
                            {
                                ErrMsg( hr,
                                    L"Couldn't apply the changes after"
                                    L" uninstalling %s.",
                                    NDISPROT_SERVICE_PNP_DEVICE_ID );
                            }
                        }
                        else
                        {
                            ErrMsg( hr,
                                L"Failed to uninstall %s.",
                                NDISPROT_SERVICE_PNP_DEVICE_ID );
                        }

                        ReleaseRef( pncClassSetup );
                    }
                    else
                    {
                        ErrMsg( hr,
                            L"Couldn't get an interface to setup class." );
                    }

                    ReleaseRef( pncClass );
                }
                else
                {
                    ErrMsg( hr,
                        L"Couldn't get a pointer to class interface "
                        L"of %s.",
                        NDISPROT_SERVICE_PNP_DEVICE_ID );
                }
            }
            else
            {
                ErrMsg( hr,
                    L"Couldn't get the class guid of %s.",
                    NDISPROT_SERVICE_PNP_DEVICE_ID );
            }

            ReleaseRef( pncc );
        }
        else
        {
            ErrMsg( hr,
                L"Couldn't get an interface pointer to %s.",
                NDISPROT_SERVICE_PNP_DEVICE_ID );
        }

        HrReleaseINetCfg( pnc,
            TRUE );
    }
    else
    {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp )
        {
            ErrMsg( hr,
                L"%s currently holds the lock, try later.",
                lpszApp );

            CoTaskMemFree( lpszApp );
        }
        else
        {
            ErrMsg( hr, L"Couldn't get the notify object interface." );
        }
    }

    return 0;
}

int __cdecl wmain(int argc, _TCHAR* argv[]) // __stdcall
{
    SetConsoleTitle( L"Installing NDIS Intermediate Filter Driver" );

    if( argc < 2 )
    {
        return 0;
    }

    if( argc > 2 )
    {
        if( _tcsicmp( argv[2], _T("/v") ) == 0 )
        {
            bVerbose = TRUE;
        }
    }

    if( argc > 2 )
    {
        if( _tcsicmp( argv[2], _T("/hide") ) == 0 )
        {
            bVerbose = FALSE;
        }
    }

    if( !bVerbose )
    {
        ShowWindow( GetConsoleWindow(), SW_HIDE );
    }

    // Handle Driver Install
    if( _tcsicmp( argv[1], _T("/Install") ) == 0 )
    {
        return InstallDriver();
    }

    // Handle Driver Uninstall
    if( _tcsicmp( argv[1], _T("/Uninstall") ) == 0 )
    {
        return UninstallDriver();
    }

    return 0;
}

