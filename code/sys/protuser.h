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

// Constants and types to access the WinTap driver.
// Users must also include ntddndis.h

#ifndef __NPROTUSER__H
#define __NPROTUSER__H


#define FSCTL_NDISPROT_BASE      FILE_DEVICE_NETWORK

#define _NDISPROT_CTL_CODE(_Function, _Method, _Access)  \
            CTL_CODE(FSCTL_NDISPROT_BASE, _Function, _Method, _Access)

#define IOCTL_NDISPROT_OPEN_DEVICE   \
            _NDISPROT_CTL_CODE(0x200, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_QUERY_OID_VALUE   \
            _NDISPROT_CTL_CODE(0x201, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_SET_OID_VALUE   \
            _NDISPROT_CTL_CODE(0x205, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_QUERY_BINDING   \
            _NDISPROT_CTL_CODE(0x203, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_NDISPROT_BIND_WAIT   \
            _NDISPROT_CTL_CODE(0x204, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)




//
//  Structure to go with IOCTL_NDISPROT_QUERY_OID_VALUE.
//  The Data part is of variable length, determined by
//  the input buffer length passed to DeviceIoControl.
//
typedef struct _NDISPROT_QUERY_OID
{
    NDIS_OID            Oid;
    NDIS_PORT_NUMBER    PortNumber;
    UCHAR               Data[sizeof(ULONG)];
} NDISPROT_QUERY_OID, *PNDISPROT_QUERY_OID;

//
//  Structure to go with IOCTL_NDISPROT_SET_OID_VALUE.
//  The Data part is of variable length, determined
//  by the input buffer length passed to DeviceIoControl.
//
typedef struct _NDISPROT_SET_OID
{
    NDIS_OID            Oid;
    NDIS_PORT_NUMBER    PortNumber;
    UCHAR               Data[sizeof(ULONG)];
} NDISPROT_SET_OID, *PNDISPROT_SET_OID;


//
//  Structure to go with IOCTL_NDISPROT_QUERY_BINDING.
//  The input parameter is BindingIndex, which is the
//  index into the list of bindings active at the driver.
//  On successful completion, we get back a device name
//  and a device descriptor (friendly name).
//
typedef struct _NDISPROT_QUERY_BINDING
{
    ULONG            BindingIndex;        // 0-based binding number
    ULONG            DeviceNameOffset;    // from start of this struct
    ULONG            DeviceNameLength;    // in bytes
    ULONG            DeviceDescrOffset;    // from start of this struct
    ULONG            DeviceDescrLength;    // in bytes

} NDISPROT_QUERY_BINDING, *PNDISPROT_QUERY_BINDING;
 
#endif // __NPROTUSER__H

