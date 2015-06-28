// Debug macros for WinTap
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

#ifndef _NPROTDEBUG__H
#define _NPROTDEBUG__H

//
// Message verbosity: lower values indicate higher urgency
//
#define DL_EXTRA_LOUD       20
#define DL_VERY_LOUD        10
#define DL_LOUD             8
#define DL_INFO             6
#define DL_WARN             4
#define DL_ERROR            2
#define DL_FATAL            0

#if DBG_SPIN_LOCK

typedef struct _NPROT_LOCK
{
    ULONG                   Signature;
    ULONG                   IsAcquired;
    PKTHREAD                OwnerThread;
    ULONG                   TouchedByFileNumber;
    ULONG                   TouchedInLineNumber;
    NDIS_SPIN_LOCK          NdisLock;
} NPROT_LOCK, *PNPROT_LOCK;

#define NPROTL_SIG    'KCOL'

extern NDIS_SPIN_LOCK       ndisprotDbgLogLock;

extern
VOID
ndisprotAllocateSpinLock(
    IN  PNPROT_LOCK          pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
ndisprotFreeSpinLock(
    IN  PNPROT_LOCK          pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
ndisprotAcquireSpinLock(
    IN  PNPROT_LOCK          pLock,
    IN  BOOLEAN             DispatchLevel,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
ndisprotReleaseSpinLock(
    IN  PNPROT_LOCK          pLock,
    IN  BOOLEAN             DispatchLevel,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern 
VOID
ndisprotFreeDbgLock(
    VOID
);

#define CHECK_LOCK_COUNT(Count)                                         \
            {                                                           \
                if ((INT)(Count) < 0)                                   \
                {                                                       \
                    DbgPrint("Lock Count %d is < 0! File %s, Line %d\n",\
                        Count, __FILE__, __LINE__);                     \
                    DbgBreakPoint();                                    \
                }                                                       \
            }
#else

#define CHECK_LOCK_COUNT(Count)

typedef NDIS_SPIN_LOCK      NPROT_LOCK;
typedef PNDIS_SPIN_LOCK     PNPROT_LOCK;

#endif    // DBG_SPIN_LOCK

#if DBG

extern INT                ndisprotDebugLevel;


#define DEBUGP(lev, stmt)                                               \
        {                                                               \
            if ((lev) <= ndisprotDebugLevel)                             \
            {                                                           \
                DbgPrint("Wtapdrv: "); DbgPrint stmt;                   \
            }                                                           \
        }

#define DEBUGPDUMP(lev, pBuf, Len)                                      \
        {                                                               \
            if ((lev) <= ndisprotDebugLevel)                             \
            {                                                           \
                DbgPrintHexDump((PUCHAR)(pBuf), (ULONG)(Len));          \
            }                                                           \
        }

#define NPROT_ASSERT(exp)                                                \
        {                                                               \
            if (!(exp))                                                 \
            {                                                           \
                DbgPrint("Wtapdrv: assert " #exp " failed in"           \
                    " file %s, line %d\n", __FILE__, __LINE__);         \
                DbgBreakPoint();                                        \
            }                                                           \
        }

#define NPROT_SET_SIGNATURE(s, t)\
        (s)->t##_sig = t##_signature;

#define NPROT_STRUCT_ASSERT(s, t)                                        \
        if ((s)->t##_sig != t##_signature)                              \
        {                                                               \
            DbgPrint("Wtapdrv: assertion failure"                       \
            " for type " #t " at 0x%p in file %s, line %d\n",           \
             s, __FILE__, __LINE__);                            \
            DbgBreakPoint();                                            \
        }


//
// Memory Allocation/Freeing Audit:
//

//
// The NPROTD_ALLOCATION structure stores all info about one allocation
//
typedef struct _NPROTD_ALLOCATION {

        ULONG                    Signature;
        struct _NPROTD_ALLOCATION *Next;
        struct _NPROTD_ALLOCATION *Prev;
        ULONG                    FileNumber;
        ULONG                    LineNumber;
        ULONG                    Size;
        ULONG_PTR                Location;  // where the returned ptr was stored
        union
        {
            ULONGLONG            Alignment;
            UCHAR                    UserData;
        };

} NPROTD_ALLOCATION, *PNPROTD_ALLOCATION;

#define NPROTD_MEMORY_SIGNATURE    (ULONG)'CSII'

extern
PVOID
ndisprotAuditAllocMem (
    PVOID        pPointer,
    ULONG        Size,
    ULONG        FileNumber,
    ULONG        LineNumber
);

extern
VOID
ndisprotAuditFreeMem(
    PVOID        Pointer
);

extern
VOID
ndisprotAuditShutdown(
    VOID
);

extern
VOID
DbgPrintHexDump(
    PUCHAR        pBuffer,
    ULONG        Length
);

#else

//
// No debug
//
#define DEBUGP(lev, stmt)
#define DEBUGPDUMP(lev, pBuf, Len)

#define NPROT_ASSERT(exp)
#define NPROT_SET_SIGNATURE(s, t) UNREFERENCED_PARAMETER(s)
#define NPROT_STRUCT_ASSERT(s, t) UNREFERENCED_PARAMETER(s)

#endif    // DBG


#endif // _NPROTDEBUG__H
