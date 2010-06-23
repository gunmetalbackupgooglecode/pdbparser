#pragma once

#include "msf.h"

typedef union _SYM SYM, *PSYM;

typedef struct _SYMD
{
	MSF *msf;

	PNewDBIHdr dbi;
	ULONG DbiSize;
	PSYM SymRecs;
	PSYM SymMac;
	ULONG SymSize;
	PHDR TpiHdr;

	MSF_STREAM_REF<GSIHashHdr> gsi;
	PVOID pGSHr;
	PVOID pGSBuckets;
	int nGSHrs;
	int nGSBuckets;
	MSF_STREAM_REF<PSGSIHDR> psi;
	PGSIHashHdr pgsiPSHash;
	PVOID pPSHr;
	PVOID pPSBuckets;
	PVOID pPSAddrMap;
	int nPSHrs;
	int nPSBuckets;
	int nPSAddrMap;

} SYMD, *PSYMD;

PSYMD SYMLoadSymbols (MSF* msf);
VOID SYMUnloadSymbols (PSYMD Symd);

#define S_ALL ((WORD)-1)

VOID SYMDumpSymbols (PSYMD Symd, WORD SymbolMask);

enum SYMBOL_TYPE
{
	ST_PUB,
	ST_CONST,
	ST_TYPEDEF,
	ST_PROC,
	ST_DATA
};

typedef struct _SYMBOL
{
	ULONG Type;
	union {
		ULONG VirtualAddress;
		ULONG Value;
	};
	PVOID EngSymPtr;
	ULONG Displacement;
	PSTR SymbolName;
	PlfRecord TypeInfo;
	PSTR FullSymbolDeclaration;
} SYMBOL, *PSYMBOL;

PSYMBOL SYMGetSymFromAddr (PSYMD Symd, USHORT Segment, ULONG Rva);
VOID SYMFreeSymbol (PSYMBOL Symbol);

PSYMBOL SYMNextSym (PSYMD Symd, PSYMBOL Symbol);

// VOID GSIInit (PSYMD Symd);
// VOID GSIClose (PSYMD Symd);

PSYM GSINearestSym (PSYMD Symd, PVOID Eip, ULONG *displacement);
PSYM GSIFindExact (PSYMD Symd, ULONG Offset, USHORT Segment);
VOID GSIEnumAll (PSYMD Symd, USHORT Segment OPTIONAL);
PSYM GSIByName (PSYMD Symd, char *name, BOOL CaseInSensitive);
PSYM SYMByName (PSYMD Symd, char *name, BOOL CaseInSensitive);
