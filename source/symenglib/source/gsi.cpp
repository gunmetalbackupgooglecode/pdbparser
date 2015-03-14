/**
* gsi.cpp  -  miscellaneous routines for symbol table parsing (SYM* and GSI*)
*  This file contains GSI, PSI and Symbol information parsing routines.
*  Generally, you SHOULD NOT call GSI*, SYM* and other routines directly.
*  Call Sym* routines (descibed in sym.cpp)
*
* This is the part of the PDB parser.
* (C) Great, 2010.
* xgreatx@gmail.com
*/

#include <windows.h>
#include <stdio.h>
#include "msf.h"
#include "tpi.h"
#include "gsi.h"
#include "misc.h"

//
// This is a common symbol union.
// We can cast any pointer from symbol table to PSYM and retrieve the appropriate information.
//

typedef union _SYM
{
	ALIGNSYM Sym;
	ANNOTATIONSYM Annotation;
	ATTRMANYREGSYM AttrManyReg;
	ATTRMANYREGSYM2 AttrManyReg2;
	ATTRREGREL AttrRegRel;
	ATTRREGSYM AttrReg;
	ATTRSLOTSYM AttrSlot;
	BLOCKSYM Block;
	BLOCKSYM16 Block16;
	BLOCKSYM32 Block32;
	BPRELSYM16 BpRel16;
	BPRELSYM32 BpRel32;
	BPRELSYM32_16t BpRel32_16t;
	CEXMSYM16 Cexm16;
	CEXMSYM32 Cexm32;
	CFLAGSYM CFlag;
	COMPILESYM Compile;
	CONSTSYM Const;
	CONSTSYM_16t Const_16t;
	DATASYM16 Data16;
	DATASYM32 Data32;
	ENTRYTHISSYM EntryThis;
	FRAMEPROCSYM FrameProc;
	FRAMERELSYM FrameRel;
	LABELSYM16 Label16;
	LABELSYM32 Label32;
	MANPROCSYM ManProc;
	MANPROCSYMMIPS ManProcMips;
	MANTYPREF ManTypRef;
	MANYREGSYM_16t ManyReg_16t;
	MANYREGSYM ManyReg;
	MANYREGSYM2 ManyReg2;
	OBJNAMESYM ObjName;
	OEMSYMBOL Oem;
	PROCSYM16 Proc16;
	PROCSYM32 Proc32;
	PROCSYM32_16t Proc32_16t;
	PROCSYMIA64 ProcIA64;
	PROCSYMMIPS ProcMips;
	PROCSYMMIPS_16t ProcMips_16t;
	PUBSYM32 Pub32;
	REFSYM Ref;
	REFSYM2 Ref2;
	REGREL16 RegRel16;
	REGREL32_16t RegRel32_16t;
	REGREL32 RegRel32;
	REGSYM Reg;
	REGSYM_16t Reg_16t;
	RETURNSYM Return;
	SEARCHSYM Search;
	SLINK32 Slink32;
	SLOTSYM32 Slot32;
	SYMTYPE SymType;
	THREADSYM32_16t Thread_16t;
	THUNKSYM Thunk;
	THUNKSYM16 Thunk16;
	THUNKSYM32 Thunk32;
	TRAMPOLINESYM Trampoline;
	UDTSYM Udt;
	UDTSYM_16t Udt_16t;
	UNAMESPACE UNameSpace;
	VPATHSYM16 VPath16;
	VPATHSYM32 VPath32;
	VPATHSYM32_16t VPath32_16t;
	WITHSYM16 With16;
	WITHSYM32 With32;
} SYM, *PSYM;

// Go to the next symbol.
#define NextSym(S) ((PSYM)((PUCHAR)(S) + (S)->Sym.reclen + sizeof(WORD)))

//
// Load symbols for the MSF
//
// Return Value: symbol descriptor (SYMD) for the supplied PDB
//
PSYMD SYMLoadSymbols (MSF* msf)
{
	PSYMD Symd = (PSYMD) halloc (sizeof(SYMD));

	if (Symd != NULL)
	{
		Symd->msf = msf;
		Symd->dbi = (PNewDBIHdr) MsfReferenceStream (msf, PDB_STREAM_DBI, &Symd->DbiSize);
		if (Symd->dbi != NULL && Symd->DbiSize >= sizeof(NewDBIHdr))
		{
			Symd->SymRecs = (PSYM) MsfReferenceStream (msf, Symd->dbi->snSymRecs, &Symd->SymSize);
			if (Symd->SymRecs != NULL)
			{
				Symd->SymMac = (PSYM)((PUCHAR)Symd->SymRecs + Symd->SymSize);

				Symd->TpiHdr = TPILoadTypeInfo (msf);
				if (Symd->TpiHdr != NULL)
				{
					return Symd;
				}
			}
		}

		hfree (Symd);
	}

	return NULL;
}
//
// Free loaded symbol information.
//
VOID SYMUnloadSymbols (PSYMD Symd)
{
	TPIFreeTypeInfo (Symd->msf, Symd->TpiHdr);
	MsfDereferenceStream (Symd->msf, Symd->dbi->snSymRecs);
	MsfDereferenceStream (Symd->msf, PDB_STREAM_DBI);
	hfree (Symd);
}

VOID SYMpDumpSymbol (PSYMD Symd, PSYM Sym)
{
	printf("Sym [%p]: %04x %04x | ", (ULONG)Sym - (ULONG)Symd->SymRecs, Sym->Sym.reclen, Sym->Sym.rectyp);

	switch (Sym->Sym.rectyp)
	{
	case S_PUB32:
		printf("S_PUB32| [%04x] public%s%s %p = %s (type %04x)",
			Sym->Pub32.seg, // 0x0c
			Sym->Pub32.pubsymflags.fCode ? " code" : "", 
			Sym->Pub32.pubsymflags.fFunction ? " function" : "",
			Sym->Pub32.off, Sym->Pub32.name, // 0x08 0x0e
			Sym->Data32.typind); // 0x04
		break;

	case S_CONSTANT:
		printf("S_CONSTANT| const %04x = %s", Sym->Const.value, Sym->Const.name);
		break;

	case S_UDT:
		printf("S_UDT| [%04x] typedef %s;", Sym->Udt.typind, 
			TPIGetSymbolDeclaration (Symd->TpiHdr, 
			TPILookupTypeName (Symd->TpiHdr, Sym->Udt.typind), 
			(char*)Sym->Udt.name
			));
		break;

	case S_LPROCREF:
	case S_PROCREF:
		printf("S_%sPROCREF| procref [%s; mod %04x; ibSym %x] %s",
			Sym->Sym.rectyp == S_LPROCREF ? "L" : "", 
			Sym->Sym.rectyp == S_LPROCREF ? "local" : "global", 
			Sym->Ref2.imod, // 0x0c
			Sym->Ref2.ibSym, // 0x08
			Sym->Ref2.name); // 0x0e
		break;

	case S_LDATA32:
	case S_GDATA32:
		{
			char *type = TPILookupTypeName (Symd->TpiHdr, Sym->Data32.typind);
			char *decl = TPIGetSymbolDeclaration (Symd->TpiHdr, type, (char*)Sym->Data32.name);
			printf("S_%sDATA32| data [%s; type %04x] %p = %s",
				Sym->Sym.rectyp == S_LDATA32 ? "L" : "G",
				Sym->Sym.rectyp == S_LDATA32 ? "local" : "global",
				Sym->Data32.typind, Sym->Data32.off, decl);
		}
		break;

	default:
		printf("unknown symbol len %04x type %04x\n", Sym->Sym.reclen, Sym->Sym.rectyp);
		Sleep (1000);
	}

	printf("\n");
}

VOID SYMDumpSymbols (PSYMD Symd, WORD SymbolMask)
{
	PSYM Sym = Symd->SymRecs;
	while (Sym < Symd->SymMac)
	{
		if (Sym->Sym.rectyp & SymbolMask)
		{
			SYMpDumpSymbol (Symd, Sym);
		}

		Sym = NextSym (Sym);
	}
}

// VOID GSIDumpSymbols (MSF* msf, PNewDBIHdr dbi, WORD SymbolMask)
// {
// 	printf("GSI dump\n");
// 
// 	ULONG GsiSize;
// 	PGSIHashHdr gsi = (PGSIHashHdr) MsfReferenceStream (msf, dbi->snGSSyms, &GsiSize);
// 
// 	if (gsi != NULL && (LONG)GsiSize > gsi->cbHr)
// 	{
// 		printf("GSI Signature: %08x\n", gsi->verSignature);
// 		printf("    Version:   %08x\n", gsi->verHdr);
// 		printf("    cbBuckets: %d\n", gsi->cbBuckets);
// 		printf("    cbHdr:     %d\n", gsi->cbHr);
// 	}
// }

static PSYMBOL SYMpConvertSymbol (PSYMD Symd, PSYM Sym, ULONG Displacement)
{
	PSYMBOL Symbol;

	Symbol = (PSYMBOL) halloc (sizeof(SYMBOL));
	if (Symbol != NULL)
	{
		switch (Sym->Sym.rectyp)
		{
		case S_PUB32:
			Symbol->VirtualAddress = Sym->Pub32.off;
			Symbol->SymbolName = xstrdup ((char*)Sym->Pub32.name);
			Symbol->Type = ST_PUB;
			break;

		case S_CONSTANT:
			Symbol->Value = Sym->Const.value;
			Symbol->Type = ST_CONST;
			Symbol->SymbolName = xstrdup ((char*)Sym->Const.name);
			break;

		case S_UDT:
			Symbol->Type = ST_TYPEDEF;
			Symbol->SymbolName = xstrdup ((char*)Sym->Udt.name);
			Symbol->TypeInfo = TPILookupTypeRecord (
				Symd->TpiHdr, 
				Sym->Udt.typind,
				NULL,
				NULL);
			Symbol->FullSymbolDeclaration = TPIGetSymbolDeclaration (
				Symd->TpiHdr,
				TPILookupTypeName(Symd->TpiHdr, Sym->Udt.typind),
				(char*)Sym->Udt.name
				);
			break;

		case S_LPROCREF:
		case S_PROCREF:
			Symbol->Type = ST_PROC;
			Symbol->SymbolName = xstrdup ((char*)Sym->Ref2.name);
			break;

		case S_LDATA32:
		case S_GDATA32:
			Symbol->Type = ST_DATA;
			Symbol->SymbolName = xstrdup ((char*)Sym->Data32.name);
			Symbol->FullSymbolDeclaration = 
				TPIGetSymbolDeclaration(
					Symd->TpiHdr,
					TPILookupTypeName (Symd->TpiHdr, Sym->Data32.typind),
					(char*)Sym->Data32.name
					);
			Symbol->TypeInfo = TPILookupTypeRecord (
				Symd->TpiHdr,
				Sym->Data32.typind,
				NULL,
				NULL);
			Symbol->VirtualAddress = Sym->Data32.off;
			break;
		}

		Symbol->Displacement = Displacement;
		Symbol->EngSymPtr = Sym;
	}
	
	return Symbol;
}

PSYMBOL SYMGetSymFromAddr (PSYMD Symd, USHORT Segment, ULONG Rva)
{
	PSYM Sym;
	PSYM SymBestMatch = NULL;
	LONG Diff = -1;

// 	printf(__FUNCTION__ ": Rva = %p\n", Rva);
	
	Sym = Symd->SymRecs;
	while (Sym < Symd->SymMac)
	{
		if (Sym->Sym.rectyp == S_PUB32)
		{
			if (Sym->Pub32.seg == Segment)
			{
//  				SYMpDumpSymbol (Symd, Sym);
				LONG LocalDiff = (LONG)(Rva - Sym->Pub32.off);
				if (LocalDiff >= 0 && (LocalDiff < Diff || Diff == -1))
				{
					SymBestMatch = Sym;
					Diff = LocalDiff;
				}
			}
		}

		Sym = NextSym (Sym);
	}

	if (SymBestMatch)
	{
		return SYMpConvertSymbol (Symd, SymBestMatch, Diff);
	}

	return NULL; // not found
}

VOID SYMFreeSymbol (PSYMBOL Symbol)
{
	if (Symbol->FullSymbolDeclaration)
		hfree (Symbol->FullSymbolDeclaration);

	if (Symbol->SymbolName)
		hfree (Symbol->SymbolName);

	hfree (Symbol);
}

PSYMBOL SYMNextSym (PSYMD Symd, PSYMBOL Symbol)
{
	PSYM Sym = (PSYM) Symbol->EngSymPtr;
	SYMFreeSymbol (Symbol);

	Sym = NextSym (Sym);
	return SYMpConvertSymbol (Symd, Sym, 0);
}

//////////////////////////////////////////////////////////////////////////
// GSI & PSGSI
//

struct HRFile {
	PSYM psym;
	int cRef;
};
enum { iphrHash = 4096, iphrFree = iphrHash, iphrMax };
ULONG rgphrBuckets[iphrMax];

struct HR {
	HR* pnext;
	PSYM psym;
	int cRef;
};

// struct SC {
// 	ULONG   isect;
// 	ULONG   off;
// 	ULONG   cb;
// 	DWORD   dwCharacteristics;
// 	ULONG   imod;
// };
// 
// 
// struct SC20 {
// 	ULONG   isect;
// 	ULONG   off;
// 	ULONG   cb;
// 	ULONG   imod;
// };
//
// struct MODI {
// 	PVOID pmod;                                     // currently open mod
// 	SC sc;                                          // this module's first section contribution
// 	USHORT fWritten :  1;                           // TRUE if mod has been written since DBI opened
// 	USHORT unused   : 15;                           // spare
// 	LONG sn;                                        // SN of module debug info (syms, lines, fpo), or snNil
// 	LONG cbSyms;                                    // size of local symbols debug info in stream sn
// 	LONG cbLines;                                   // size of line number debug info in stream sn
// 	LONG cbFpo;                                     // size of frame pointer opt debug info in stream sn
// 	USHORT ifileMac;                                // number of files contributing to this module
// 	LONG* mpifileichFile;                           // array [0..ifileMac) of offsets into dbi.bufFilenames
// 	char rgch[];                                    // szModule followed by szObjFile
// };


VOID GSIInit (PSYMD Symd)
{
	// Load GSI & PSGSI
	MsfReferenceStreamType (
		Symd->msf, 
		Symd->dbi->snGSSyms,
		&Symd->gsi
		);
	MsfReferenceStreamType(
		Symd->msf,
		Symd->dbi->snPSSyms,
		&Symd->psi
		);

	// Parse GSI
	Symd->pGSHr = (PUCHAR)(Symd->gsi.Data + 1);
	Symd->pGSBuckets = (PUCHAR)(Symd->gsi.Data + 1) + Symd->gsi.Data->cbHr;
	Symd->nGSHrs = Symd->gsi.Data->cbHr / sizeof(HRFile);
	Symd->nGSBuckets = Symd->gsi.Data->cbBuckets / sizeof(ULONG);

	// parse PSGSI
	Symd->pgsiPSHash = (PGSIHashHdr)(Symd->psi.Data + 1);
	Symd->pPSHr = (PUCHAR)(Symd->pgsiPSHash + 1);
	Symd->pPSBuckets = (PUCHAR)(Symd->pgsiPSHash + 1) + Symd->pgsiPSHash->cbHr;
	Symd->nPSHrs = Symd->pgsiPSHash->cbHr / sizeof(HRFile);
	Symd->nPSBuckets = Symd->pgsiPSHash->cbBuckets / sizeof(ULONG);
	Symd->pPSAddrMap = (PUCHAR)(Symd->pgsiPSHash + 1) + Symd->psi.Data->cbSymHash;
	Symd->nPSAddrMap = Symd->psi.Data->cbAddrMap / sizeof(ULONG);
}

VOID GSIClose (PSYMD Symd)
{
	MsfDereferenceStream (Symd->msf, Symd->dbi->snGSSyms);
	MsfDereferenceStream (Symd->msf, Symd->dbi->snPSSyms);
}

BOOL SympFindBestOffsetInMatchingSection (PVOID ImageBase, PVOID Address, ULONG *SectionBaseRVA, ULONG *ImageRVA, ULONG *Offset, USHORT *Segment);
PVOID RtlGetModuleBase (PVOID Addr, ULONG *Rva);

PSYM GSINearestSym (PSYMD Symd, PVOID Eip, ULONG *displacement)
{
	ULONG off = 0, Rva = 0, ImageRVA, SectionBaseRVA, Offset;
	USHORT Segment;	
	PVOID Image;

	Image = RtlGetModuleBase (Eip, &Rva);
	SympFindBestOffsetInMatchingSection (
		Image,
		Eip,
		&SectionBaseRVA,
		&ImageRVA,
		&Offset,
		&Segment);

	ULONG *ppsymLo, *ppsymHi;
	PULONG pAddrMap = (PULONG)Symd->pPSAddrMap;
	int nAddrMap = Symd->nPSAddrMap;

	ppsymLo = (ULONG*)pAddrMap;
	ppsymHi = (ULONG*)(pAddrMap + nAddrMap);

	while (ppsymLo < ppsymHi)
	{
		ULONG *ppsym = ppsymLo + ((ppsymHi - ppsymLo + 1) >> 1);

		PPUBSYM32 psymLo = (PPUBSYM32)((PUCHAR)Symd->SymRecs + *ppsymLo);
		PPUBSYM32 psymHi = (PPUBSYM32)((PUCHAR)Symd->SymRecs + *ppsymHi);
		PPUBSYM32 psym = (PPUBSYM32)((PUCHAR)Symd->SymRecs + *ppsym);

		// isect = Segment
		// off = Offset
		// elem = ppsym

		USHORT isect2 = psym->seg;
		ULONG off2 = psym->off;
		USHORT isect1 = Segment;
		ULONG off1 = Offset;

		int cmp;

		cmp = (isect1 == isect2) ? (long)off1 - (long)off2 : (short)isect1 - (short)isect2;

		if (cmp < 0)
			ppsymHi = ppsym - 1;
		else if (cmp > 0)
			ppsymLo = ppsym;
		else
			ppsymLo = ppsymHi = ppsym;
	}

	PPUBSYM32 psymLo = (PPUBSYM32)((PUCHAR)Symd->SymRecs + *ppsymLo);

	// if the offset is negative, but offset from the prev symbol will be positive or zero, 
	//  use the prev symbol.
	PPUBSYM32 psymPrev = (PPUBSYM32)((PUCHAR)Symd->SymRecs + *(ppsymLo-1));

	if (psymLo->seg == Segment && ((long)Offset - (long)psymLo->off) < 0 &&
		((long)Offset - (long)psymPrev->off) >= 0)
	{
		psymLo = psymPrev;
	}


	if (psymLo->seg < Segment && 
		ppsymLo < ((ULONG*)pAddrMap + nAddrMap - 1))
	{
		++ppsymLo;
		psymLo = (PPUBSYM32)((PUCHAR)Symd->SymRecs + *ppsymLo);
	}

	ULONG off2 = psymLo->off;
	//off2 -= 0x3d3d; ??

	ULONG disp = ((ULONG)Offset - off2);

	*displacement = disp;
	return (PSYM)psymLo;
}

VOID GSIEnumAll (PSYMD Symd, USHORT Segment OPTIONAL)
{
	PULONG pAddrMap = (PULONG)Symd->pPSAddrMap;
	int nAddrMap = Symd->nPSAddrMap;
	ULONG prevOffset = 0;

	for (int i=0; i<nAddrMap; i++)
	{
		PPUBSYM32 s = (PPUBSYM32)((PUCHAR)Symd->SymRecs + pAddrMap[i]);

		if (Segment == 0 || s->seg == Segment)
		{
			printf("GSI#%d = %04x:%08x (%08x) = %s\n", i, s->seg, s->off, s->off - prevOffset, s->name);
			prevOffset = s->off;
		}
	}
}

PSYM GSIFindExact (PSYMD Symd, ULONG Offset, USHORT Segment)
{
	PULONG pAddrMap = (PULONG)Symd->pPSAddrMap;
	int nAddrMap = Symd->nPSAddrMap;

	for (int i=0; i<nAddrMap; i++)
	{
		PPUBSYM32 s = (PPUBSYM32)((PUCHAR)Symd->SymRecs + pAddrMap[i]);

		if (/*s->seg == Segment &&*/ s->off == Offset)
		{
			return (PSYM)s;
		}
	}

	return 0;
}

PSYM GSIByName (PSYMD Symd, char *name, BOOL CaseInSensitive)
{
	PULONG pAddrMap = (PULONG)Symd->pPSAddrMap;
	int nAddrMap = Symd->nPSAddrMap;

	for (int i=0; i<nAddrMap; i++)
	{
		PPUBSYM32 s = (PPUBSYM32)((PUCHAR)Symd->SymRecs + pAddrMap[i]);

		if ((CaseInSensitive ? strcmp : _stricmp) (name, (char*)s->name) == 0)
		{
			return (PSYM)s;
		}
	}

	return 0;
}

PSYM SYMByName (PSYMD Symd, char *name, BOOL CaseInSensitive)
{
	for (PSYM Sym = Symd->SymRecs; Sym < Symd->SymMac; Sym = NextSym (Sym))
	{
		bool success = false;

		switch (Sym->Sym.rectyp)
		{
		case S_PUB32:
			success = ((CaseInSensitive ? strcmp : _stricmp) (name, (char*)Sym->Pub32.name) == 0);
			break;

		case S_CONSTANT:
			success = ((CaseInSensitive ? strcmp : _stricmp) (name, (char*)Sym->Const.name) == 0);
			break;

		case S_UDT:
			success = ((CaseInSensitive ? strcmp : _stricmp) (name, (char*)Sym->Udt.name) == 0);
			break;

		case S_LPROCREF:
		case S_PROCREF:
			success = ((CaseInSensitive ? strcmp : _stricmp) (name, (char*)Sym->Ref2.name) == 0);
			break;

		case S_LDATA32:
		case S_GDATA32:
			success = ((CaseInSensitive ? strcmp : _stricmp) (name, (char*)Sym->Data32.name) == 0);
			break;
		}

		if (success) return Sym;
	}

	return NULL;
}
