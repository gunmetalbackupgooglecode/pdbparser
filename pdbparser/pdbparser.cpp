#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>
#include <imagehlp.h>
#include "sym.h"
#include "tpi.h"
#include "misc.h"
#include "err.h"
#include <conio.h>
#include <common/mapping.h>

// For TPI dumping.
typedef struct _MY_STRUCT
{
	ULONG *aa;
	void (__fastcall *callback_routine)( void (__stdcall *routine)(int,char), void*, _MY_STRUCT* );
} MY_STRUCT, *PMY_STRUCT;

//////////////////////////////////////////////////////////////////////////
// Config for tests.
// Set 1 to those tests you want to enable.

#define TEST_STACK_WALK                    0
#define TEST_DUMP_TPI                      0
#define TEST_NTOSKRNL                      0
#define TEST_SELF_DLL                      1
#define TEST_SELF_EXE                      1
#define TEST_TEST_EXE                      1

//////////////////////////////////////////////////////////////////////////
// Entry point
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


BOOL SympFindBestOffsetInMatchingSection (PVOID ImageBase, PVOID Address, ULONG *SectionBaseRVA, ULONG *ImageRVA, ULONG *Offset, USHORT *Segment);
PVOID RtlGetModuleBase (PVOID Addr, ULONG *Rva);

void DumpFPOnSections (PPDB_SYMBOLS syms)
{
// 	ULONG size;
// 	PVOID pSections = MsfReferenceStream (syms->pdb->msf, 5);

	printf("stream info:\n");
	printf(" GSI stream: %d\n", syms->pdb->dbi.Data->snGSSyms);
	printf(" PSGSI stream: %d\n", syms->pdb->dbi.Data->snPSSyms);
	printf(" SYM stream: %d\n", syms->pdb->dbi.Data->snSymRecs);

	ULONG fpoSize;
	PFPO_DATA fpo = (PFPO_DATA) MsfReferenceStream (syms->pdb->msf, PDB_STREAM_FPO, &fpoSize);

	int nFpoCount = fpoSize / sizeof(FPO_DATA);

// 	for (int i=0; i<nFpoCount; i++)
// 	{
// 		printf(
// 			"start %08x size %08x locals %08x params %04x "
// 			"prolog %02x regs %x SEH? %x EBP? %x rsvd %x frameType %x\n",
// 			fpo[i].ulOffStart,
// 			fpo[i].cbProcSize,
// 			fpo[i].cdwLocals,
// 			fpo[i].cdwParams,
// 			fpo[i].cbProlog,
// 			fpo[i].cbRegs,
// 			fpo[i].fHasSEH,
// 			fpo[i].fUseBP,
// 			fpo[i].reserved,
// 			fpo[i].cbFrame);
// 	}

	MsfDereferenceStream (syms->pdb->msf, PDB_STREAM_FPO);

	{
		ULONG sectSize;
		PVOID pSect = MsfReferenceStream (syms->pdb->msf, 8, &sectSize);

		int nSect = sectSize / sizeof (IMAGE_SECTION_HEADER);
		PIMAGE_SECTION_HEADER Sections = (PIMAGE_SECTION_HEADER)pSect;

		for (int i=0; i<nSect; i++)
		{
			printf("%s (VA %08x * %08x RawSize %08x Misc %08x)\n",
				Sections[i].Name,
				Sections[i].VirtualAddress,
				Sections[i].PointerToRawData,
				Sections[i].SizeOfRawData,
				Sections[i].Misc.VirtualSize);
		}
		printf("\n");

		MsfDereferenceStream (syms->pdb->msf, 8);
	}

	{
		ULONG sectSize;
		PVOID pSect = MsfReferenceStream (syms->pdb->msf, 11, &sectSize);

		int nSect = sectSize / sizeof (IMAGE_SECTION_HEADER);
		PIMAGE_SECTION_HEADER Sections = (PIMAGE_SECTION_HEADER)pSect;

		for (int i=0; i<nSect; i++)
		{
			printf("%s (VA %08x * %08x RawSize %08x Misc %08x)\n",
				Sections[i].Name,
				Sections[i].VirtualAddress,
				Sections[i].PointerToRawData,
				Sections[i].SizeOfRawData,
				Sections[i].Misc.VirtualSize);
		}
		printf("\n");

		MsfDereferenceStream (syms->pdb->msf, 11);
	}

	{
// 		PIMAGE_DOS_HEADER Dos = (PIMAGE_DOS_HEADER)GetModuleHandle("kernel32.dll");
// 		PIMAGE_NT_HEADERS Nt = (PIMAGE_NT_HEADERS)((PUCHAR)Dos + Dos->e_lfanew);
// 		PIMAGE_SECTION_HEADER Sections = (PIMAGE_SECTION_HEADER)((PUCHAR)&Nt->OptionalHeader + Nt->FileHeader.SizeOfOptionalHeader);
		PIMAGE_SECTION_HEADER Sections = syms->Sections;

		for (int i=0; i<syms->Nt->FileHeader.NumberOfSections; i++)
		{
			printf("%s (VA %08x * %08x RawSize %08x Misc %08x)\n",
				Sections[i].Name,
				Sections[i].VirtualAddress,
				Sections[i].PointerToRawData,
				Sections[i].SizeOfRawData,
				Sections[i].Misc.VirtualSize);
		}
		printf("\n");
	}
}

void DumpStreams (char *name, PPDB_SYMBOLS syms)
{
	for (ULONG i=0; i<syms->pdb->msf->Root->dwStreamCount; i++)
	{
		char szFile[200];

		wsprintf (szFile, "P:\\pdbparser\\%s.%03d", name, i);

		HANDLE hFile;
		ULONG size;
		PVOID pStream = MsfReferenceStream (syms->pdb->msf, i, &size);

		hFile = CreateFile (szFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
		WriteFile (hFile, pStream, size, &size, 0);
		CloseHandle (hFile);

		MsfDereferenceStream (syms->pdb->msf, i);
	}
}

void DumpGSI (PPDB_SYMBOLS syms)
{
	ULONG size;
	PVOID pGSI = MsfReferenceStream (syms->pdb->msf, syms->pdb->dbi.Data->snGSSyms, &size);
	printf("GSI %p (%08x)\n", pGSI, size);

	{
		PGSIHashHdr pgsiGS = (PGSIHashHdr) pGSI;

		printf(" verSignature %08x\n", pgsiGS->verSignature);
		printf(" verHdr       %08x\n", pgsiGS->verHdr);
		printf(" cbHr       %x\n", pgsiGS->cbHr);
		printf(" cbBuckets  %x\n", pgsiGS->cbBuckets);

		HRFile* pHr = (HRFile*) ((PUCHAR)(pgsiGS + 1));
		PULONG pBuckets = (PULONG) ((PUCHAR)(pgsiGS + 1) + pgsiGS->cbHr);

		int nHrs = pgsiGS->cbHr / sizeof(HRFile);
		int nBuckets = pgsiGS->cbBuckets / sizeof(ULONG);

		printf("nHrs %d,  nBuckets %d\n", nHrs, nBuckets);
	}

	MsfDereferenceStream (syms->pdb->msf, syms->pdb->dbi.Data->snGSSyms);

	PVOID pPSI = MsfReferenceStream (syms->pdb->msf, syms->pdb->dbi.Data->snPSSyms, &size);
	printf("PSGSI %p (%08x)\n", pPSI, size);

	{
		PPSGSIHDR pgsiPS = (PPSGSIHDR) pPSI;
		PGSIHashHdr pgsiPSHash = (PGSIHashHdr) (pgsiPS+1);

		printf(" cbSymHash %08x\n", pgsiPS->cbSymHash);
		printf(" cbAddrMap %08x\n", pgsiPS->cbAddrMap);
		printf(" nThunks %08x\n", pgsiPS->nThunks);
		printf(" cbSizeOfThunk %08x\n", pgsiPS->cbSizeOfThunk);
		printf(" isectThunkTable %04x\n", pgsiPS->isectThunkTable);
		printf(" reserved %04x\n", pgsiPS->reserved);
		printf(" offThunkTable %08x\n", pgsiPS->offThunkTable);
		printf(" nSects %08x\n", pgsiPS->nSects);
		printf("GSI Hash Hdr\n");
		printf(" verSignature %08x\n", pgsiPSHash->verSignature);
		printf(" verHdr       %08x\n", pgsiPSHash->verHdr);
		printf(" cbHr      %08x\n", pgsiPSHash->cbHr);
		printf(" cbBuckets %08x\n", pgsiPSHash->cbBuckets);

		HRFile* pHr = (HRFile*) ((PUCHAR)(pgsiPSHash + 1));
		PULONG pBuckets = (PULONG) ((PUCHAR)(pgsiPSHash + 1) + pgsiPSHash->cbHr);

		int nHrs = pgsiPSHash->cbHr / sizeof(HRFile);
		int nBuckets = pgsiPSHash->cbBuckets / sizeof(ULONG);
		printf("nHrs %d,  nBuckets %d\n", nHrs, nBuckets);

		for (int i=0; i<nHrs; i++)
		{
			PPUBSYM32 Sym = (PPUBSYM32)((PUCHAR)syms->pdb->Symd->SymRecs + (ULONG)pHr[i].psym - 1);
//			printf("pSym %08x  ref %08x    %s\n", pHr[i].psym, pHr[i].cRef, Sym->name);

			//if (strstr((char*)Sym->name, "BaseProcessStart"))
			if (0)
				printf("pSym %08x  ref %08x    %p[%04x] : (%04x:%08x) %s\n", pHr[i].psym, pHr[i].cRef, Sym, Sym->rectyp, Sym->seg, Sym->off, Sym->name);
		}

// 		for (int i=iphrFree; i >= 0; i--)
// 		{
// 			printf("bucket[%d] = %08x\n", i, pBuckets[i]);
// 			if ((iphrFree-i) % 79 == 1) _getch();
// 		}

		PULONG pAddrMap = (PULONG) ((PUCHAR)(pgsiPSHash + 1) + pgsiPS->cbSymHash);
		int nAddrMap = pgsiPS->cbAddrMap / sizeof(ULONG);

		for (int i=0; i<nAddrMap; i++)
		{
			PPUBSYM32 Sym = (PPUBSYM32)((PUCHAR)syms->pdb->Symd->SymRecs + (ULONG)pHr[i].psym - 1);
// 			printf("addr[%d] = %08x\n", i, pAddrMap[i]);

			//if (strstr((char*)Sym->name, "BaseProcessStart"))
			if (strstr((char*)Sym->name, "LdrpInitialize"))
			{
				ULONG bias = 0;

//				bias = -0x3d3d;

				ULONG addr = bias + Sym->off + syms->Sections[Sym->seg-1].VirtualAddress + (ULONG)syms->ImageBase;

				printf("addr[%d] = %08x    %p[%04x] : (%04x:%08x) [%08x] %s\n", i, 
					pAddrMap[i],
					Sym, Sym->rectyp, 
					Sym->seg, Sym->off,
					addr, Sym->name);

				PPUBSYM32 s2 = (PPUBSYM32)((PUCHAR)syms->pdb->Symd->SymRecs + (ULONG)Sym->off);

				__asm nop;

			}
		}

		//PVOID Eip = (PVOID) 0x7c817054; // BaseProcessStart
		PVOID Eip = (PVOID) (0x7C91B077 + 3); // LdrpInitialize + 3
		ULONG off = 0;
		ULONG Rva = 0;
		ULONG ImageRVA, SectionBaseRVA, Offset;
		USHORT Segment;

		PVOID Image = RtlGetModuleBase (Eip, &Rva);

		SympFindBestOffsetInMatchingSection (
			Image,
			Eip,
			&SectionBaseRVA,
			&ImageRVA,
			&Offset,
			&Segment);

		ULONG *ppsymLo, *ppsymHi;

		ppsymLo = (ULONG*)pAddrMap;
		ppsymHi = (ULONG*)(pAddrMap + nAddrMap);

		typedef PPUBSYM32 PSYM;

		while (ppsymLo < ppsymHi)
		{
			ULONG *ppsym = ppsymLo + ((ppsymHi - ppsymLo + 1) >> 1);

			PSYM psymLo = (PSYM)((PUCHAR)syms->pdb->Symd->SymRecs + *ppsymLo);
			PSYM psymHi = (PSYM)((PUCHAR)syms->pdb->Symd->SymRecs + *ppsymHi);
			PSYM psym = (PSYM)((PUCHAR)syms->pdb->Symd->SymRecs + *ppsym);

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

		PSYM psymLo = (PSYM)((PUCHAR)syms->pdb->Symd->SymRecs + *ppsymLo);

		if (psymLo->seg < Segment && 
			ppsymLo < ((ULONG*)pAddrMap - 1))
		{
			++ppsymLo;
			psymLo = (PSYM)((PUCHAR)syms->pdb->Symd->SymRecs + *ppsymLo);
		}

		ULONG off2 = psymLo->off;
		//off2 -= 0x3d3d; ??

		ULONG disp = ((ULONG)Offset - off2);

		printf("%08x = %s+0x%x\n", Eip, psymLo->name, disp);

	}

	MsfDereferenceStream (syms->pdb->msf, syms->pdb->dbi.Data->snPSSyms);

	//ULONG modsize;
	//PVOID mod = MsfReferenceStream (syms->pdb->msf, syms->pdb->dbi.Data->
// 	PVOID mod = (syms->pdb->dbi.Data + 1);
// 	printf("%p\n", mod);
}

PCHAR GetModuleLib (PMODI m)
{
	PCHAR pData = (PCHAR)(m + 1);
	pData += strlen(pData) + 1;
	return pData;
}

PMODI NextModule (PMODI m)
{
	PCHAR pData = (PCHAR)(m + 1);
	pData += strlen(pData) + 1;
	pData += strlen(pData) + 1;
	//*(ULONG*)&pData |= 3;
	pData = (PCHAR) ALIGN_UP ((ULONG)pData, 4);
	return (PMODI) pData;
}

void DumpMod (PPDB_SYMBOLS syms)
{
	PMODI modi = (PMODI)(syms->pdb->dbi.Data + 1);
	ULONG modiSize = syms->pdb->dbi.Data->cbGpModi;
	PMODI modiMac = (PMODI)((PUCHAR)modi + modiSize);

	for (PMODI m = modi; m < modiMac; m = NextModule(m))
	{
		printf("m[imod %d isect %d %08x] = %s  [%s]\n", 
			(signed short)m->sc.sc40.imod, (signed short)m->sc.sc40.isect, m->sc.sc40.dwCharacteristics,
			m->rgch,
			GetModuleLib (m)
			);
	}
}

bool bApplyFromNtPath = false;

void FromNtPath()
{
	char NtSymbolPath [1024] = "";
	GetEnvironmentVariable ("_NT_SYMBOL_PATH", NtSymbolPath, sizeof(NtSymbolPath));

	printf("_NT_SYMBOL_PATH = %s\n", NtSymbolPath);

	{
		char szSuggestedPath[MAX_PATH];
		bool bSuggested = false;

		if (!_strnicmp(NtSymbolPath, "srv*", 4))
		{
			PSTR pPath = &NtSymbolPath[4];
			PSTR pAsterisk = strchr (pPath, '*');
			if (pAsterisk)
			{
				SIZE_T nLength = pAsterisk - pPath;
				strncpy (szSuggestedPath, pPath, nLength);
				szSuggestedPath[nLength] = 0;
				bSuggested = true;
			}
		}
		
		if (!bSuggested)
		{
			strcpy (szSuggestedPath, NtSymbolPath);
		}
		
		printf("Suggested path from _NT_SYMBOL_PATH: %s\n", szSuggestedPath);

		if (bApplyFromNtPath == false)
		{
			printf("Apply? (Y/N): ");

			char ch;
			do 
			{
				ch = _getch();
			} while (ch != 'y' && ch != 'Y' && ch != 'n' && ch != 'N');

			printf("%c\n", ch);

			if (ch == 'y' || ch == 'Y')
				bApplyFromNtPath = true;
		}

		if (bApplyFromNtPath == true)
		{
			SymSetSymbolPath(szSuggestedPath);
			printf("Applied\n");
		}
	}
}

void walkstack()
{
	STACKFRAME64 StackFrame = {0};

	CONTEXT Context = {CONTEXT_FULL};
	RtlCaptureContext (&Context);

	StackFrame.AddrStack.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = (DWORD64)(ULONG_PTR)Context.Esp;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrPC.Offset = (DWORD64)(ULONG_PTR)Context.Eip;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = (DWORD64)(ULONG_PTR)Context.Ebp;


//	PPDB_SYMBOLS syms;

// 	//////////////////////////////////////////////////////////////////////////
// 
// 	SymSetPrintfCallout ((PPRINTF_CALLOUT)printf);
// 
// 	if (!SymDownloadFromServerForImage (GetModuleHandle("kernel32.dll"), "http://msdl.microsoft.com/download/symbols", "."))
// 		return (void)printf("could not download symbols for kernel32\n");
// 
// 	SymSetSymbolPath (".");
// 
// 	syms = SymLoadSymbolsForImage (GetModuleHandle ("kernel32.dll"));
// 	if (!syms)
// 	{
// 		return (void)printf("could not find symbols for kernel32\n");
// 	}
// 
// 	printf("loaded symbols for kernel32: %p (%s)\n", syms, syms->pdb->szPdbFileName);
// 
// 	return;
// 
	//////////////////////////////////////////////////////////////////////////

// 	SymSetSymbolPath ("P:\\tiny_for_pdb\\Debug");
// 
// 	syms = SymLoadSymbolsForImageFile ("\\tiny_for_pdb\\Debug\\tiny_for_pdb.exe");
// 	if (!syms)
// 	{
// 		return (void)printf("could not find symbols for tiny_for_pdb\n");
// 	}
// 	printf("symbols for tiny_for_pdb: %s\n", syms->pdb->szPdbFileName);
// 
// // 	syms = SymLoadSymbolsForImage(GetModuleHandle(0));
// 
// #define HashSz(s) HashPbCb((PBYTE)(s), strlen(s), 256)
// 
// 	printf("%p\n", HashSz("main"));
// 
// 	SYMDumpSymbols (syms->pdb->Symd, S_ALL);
// 
// 	DumpGSI (syms);
// 	return;
// 
// 
// 	TPIDumpTypes (syms->pdb->Symd->TpiHdr);
// 
//  	return;

	//////////////////////////////////////////////////////////////////////////

 	//	SymSetSymbolPath ("D:\\SymbolsSP3");
	//	SymSetSymbolPath ("D:\\Soft\\gr8drvr");

// 	// reload for kernel32
// 	syms = SymLoadSymbolsForImage (GetModuleHandle ("kernel32.dll"));
// 	if (!syms)
// 	{
// 		return (void)printf("could not find symbols for kernel32\n");
// 	}
// 	printf("symbols for kernel32: %s\n", syms->pdb->szPdbFileName);

#if 0
	PVOID Image = GetModuleHandle ("ntdll.dll");
	syms = SymLoadSymbolsForImage (Image);
	if (!syms)
	{
#if 0
		if (!SymDownloadFromServerForImage (Image, "http://msdl.microsoft.com/download/symbols", "D:\\Symbols"))
			return (void) printf("could not download symbols for ntdll\n");
		else
		{
			printf("downloaded, re-loading\n");
		}

		if ((syms = SymLoadSymbolsForImage (Image)) == NULL)
#endif
			return (void) printf("could not load symbols for ntdll (%s)\n", PdbGetErrorDescription(PdbGetLastError()));	
	}
	printf("symbols for ntdll: %s\n", syms->pdb->szPdbFileName);
#endif

//	TPIDumpTypes (syms->pdb->Symd->TpiHdr);

//  	GSIInit (syms->pdb->msf, syms->pdb->Symd);
//  
//  	ULONG disp = 0;
//  	PPUBSYM32 s = (PPUBSYM32) GSINearestSym (syms->pdb->Symd, (void*)0x7c817027, &disp);
//  	printf("%s + 0x%x\n", s->name, disp);
// 
// 	//DumpStreams (syms);
// // 	DumpMod (syms);
// // 	DumpFPOnSections (syms);
// // 	DumpGSI (syms);
// 
// 	SymUnloadAll();
// 	return;

// 	PVOID p;
// 
// 	__asm
// 	{
// 		call _1
// _1:	pop [p]
// 	}
// 
// // 	p = &walkstack;
// 
// 	PSYMBOL s = SymFromAddress (p);
// 
// 	if (s == NULL)
// 		return (void) printf("SymFromAddress() failed\n");
// 	
// 	printf("%p = %s+0x%lx\n", p, s->SymbolName, s->Displacement);
// 
// 	return;
// 
// 	SYMDumpSymbols (syms->pdb->Symd, S_ALL);

	SymSetPrintfCallout ((PPRINTF_CALLOUT)printf);

	printf("\n*********************************\n");

	while (StackWalk64 (IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentProcess(), &StackFrame, 0, 0, 0, 0, 0))
	{
		PVOID Eip = (PVOID)(ULONG_PTR)StackFrame.AddrPC.Offset;
		ULONG Offset;
		PVOID Base = RtlGetModuleBase (Eip, &Offset);
		PSYMBOL Symbol = NULL;
		char moduleName[64];
		PSTR szBaseModuleName = NULL;

		if (Base)
		{
			GetModuleFileName ((HMODULE)Base, moduleName, sizeof(moduleName)-1);
			szBaseModuleName = PathFindFileName (moduleName);

// 			if ((ULONG)Base == 0x7c800000)
// 				__debugbreak();

			Symbol = SymFromAddress (Eip);
		}

		if (Symbol)
		{
			printf("ebp %08x ret %08x  | eip %08x | %08x %08x %08x | %s!%s+0x%x\n",
				(ULONG)StackFrame.AddrFrame.Offset,
				(ULONG)StackFrame.AddrReturn.Offset,
				(ULONG)StackFrame.AddrPC.Offset,
				(ULONG)StackFrame.Params[0], (ULONG)StackFrame.Params[1], 
				(ULONG)StackFrame.Params[2],
				szBaseModuleName,
				Symbol->SymbolName,
				Symbol->Displacement);

			SymFreeSymbol (Symbol);
		}
		else if (Base)
		{
			printf("ebp %08x ret %08x  | eip %08x | %08x %08x %08x | %s+0x%x\n",
				(ULONG)StackFrame.AddrFrame.Offset,
				(ULONG)StackFrame.AddrReturn.Offset,
				(ULONG)StackFrame.AddrPC.Offset,
				(ULONG)StackFrame.Params[0], (ULONG)StackFrame.Params[1], 
				(ULONG)StackFrame.Params[2],
				szBaseModuleName,
				Offset);
		}
		else
		{
			printf("ebp %08x ret %08x  | eip %08x | %08x %08x %08x | %08x\n",
				(ULONG)StackFrame.AddrFrame.Offset,
				(ULONG)StackFrame.AddrReturn.Offset,
				(ULONG)StackFrame.AddrPC.Offset,
				(ULONG)StackFrame.Params[0], (ULONG)StackFrame.Params[1], 
				(ULONG)StackFrame.Params[2],
				(ULONG)StackFrame.AddrPC.Offset);
		}
	}
}

void f2()
{
	walkstack();
}

void f1()
{
	f2();
}

void PrintEntryPointNameForImage (char *ImageName)
{
	char windir[MAX_PATH];
	GetWindowsDirectory (windir, sizeof(windir)-1);

	strcat (windir, ImageName);

	ULONG Size;
	PVOID Kernel = MapExistingFile (windir, MAP_READ|MAP_IMAGE, NULL, &Size);

	if (Kernel != NULL)
	{
		PPDB_SYMBOLS syms = SymLoadSymbolsForImage (Kernel);

		if (syms == NULL)
		{
			if (PdbGetLastError() == PDBERR_NOT_FOUND_IN_STORAGE)
			{
				SymSetPrintfCallout ((PPRINTF_CALLOUT)printf);

				if (!SymDownloadFromServerForImage (Kernel, "http://msdl.microsoft.com/download/symbols", 0))
					return (void) printf("could not download symbols :(\n");

				// re-open
				if ((syms = SymLoadSymbolsForImage (Kernel)) == NULL)
					return (void) printf("can't load symbols for kernel. again. (%s)\n", PdbGetErrorDescription(PdbGetLastError()));
			}

			return (void) printf("can't load symbols for kernel (%s)\n", PdbGetErrorDescription(PdbGetLastError()));
		}

		printf("symbols for nt (%p): %p\n", Kernel, syms);

		PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Kernel;
		PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)((PUCHAR)Kernel + DosHeader->e_lfanew);

		ULONG Rva = NtHeaders->OptionalHeader.AddressOfEntryPoint;
		//ULONG Rva = 0x000e0dd5;
		//ULONG Rva = 0x000091B2;

		PVOID Address = (PVOID)((PUCHAR)Kernel + Rva);

		ULONG disp = 0;
		PSYM s = GSINearestSym (syms->pdb->Symd, Address, &disp);

		printf("%s + 0x%x\n", ((PPUBSYM32)s)->name, disp);
		UnmapViewOfFile (Kernel);
	}
}

void test_kernel()
{
	PrintEntryPointNameForImage ("\\system32\\smss.exe");
	printf("\n\n");
	PrintEntryPointNameForImage ("\\system32\\ntkrnlpa.exe");
	printf("\n\n");
	PrintEntryPointNameForImage ("\\system32\\kernel32.dll");
	printf("\n\n");
	PrintEntryPointNameForImage ("\\system32\\drivers\\tcpip.sys");
	printf("\n\n");
	//PrintEntryPointNameForImage ("\\system32\\user32.dll");
	//printf("\n\n");
}

void test_selfdll()
{
	PPDB_SYMBOLS syms;
	PVOID Image;

	Image = GetModuleHandle("symeng.dll");
	syms = SymLoadSymbolsForImage (Image);

	if (syms == NULL)
		return (void) printf("could not load self symbols (%s)\n", PdbGetErrorDescription(PdbGetLastError()));

	printf("syms for symeng: %p, %s\n", syms, syms->pdb->szPdbFileName);

	//SYMDumpSymbols (syms->pdb->Symd, S_ALL);

	ULONG disp = 0;
	PVOID addr;
	addr = GetProcAddress ((HMODULE)Image, "GSINearestSym");

	PSYM s = GSINearestSym (syms->pdb->Symd, addr, &disp);

	printf("DLL: [image %p sym %p]symbol = %p (%s), displacement 0x%x\n", Image, addr, s, ((PPUBSYM32)s)->name, disp);

	SymUnloadSymbols (syms);
}

__declspec(naked) PVOID GetCallerAddress()
{
	__asm
	{
		mov eax, [esp]
		ret
	}
}

void test_selfexe()
{
	PPDB_SYMBOLS syms;
	PVOID Image;

	Image = GetModuleHandle (0);
	syms = SymLoadSymbolsForImage (Image);

	if (syms == NULL)
		return (void) printf("Could not load symbols for self exe (%s)\n", PdbGetErrorDescription(PdbGetLastError()));

	ULONG disp = 0;
	PSYM s = GSINearestSym (syms->pdb->Symd, GetCallerAddress(), &disp);

	printf("EXE: symbol = %p (%s), displacement 0x%x\n", s, ((PPUBSYM32)s)->name, disp);

	SymUnloadSymbols (syms);
}

void test_another_mod_ep (char *mod)
{
	PPDB_SYMBOLS syms;
	PVOID Image;
	ULONG FileSize;

	Image = MapExistingFile (mod, MAP_READ|MAP_IMAGE, NULL, &FileSize);
	if (Image == NULL)
		printf("TEST: cannot map test exe\n");
	else
	{
		printf("TEST: testexe mapped at %p\n", Image);

		syms = SymLoadSymbolsForImage (Image);
		if (syms == NULL)
			printf("TEST: Could not load symbols for test exe (%s)\n", PdbGetErrorDescription(PdbGetLastError()));
		else
		{
			PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Image;
			PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)((PUCHAR)Image + DosHeader->e_lfanew);

			ULONG EpRva = NtHeaders->OptionalHeader.AddressOfEntryPoint + 1;
			PVOID Ep = (PUCHAR)Image + EpRva;

			ULONG disp = 0;
			PSYM s = GSINearestSym (syms->pdb->Symd, Ep, &disp);
			//PSYMBOL s = SymFromAddress (Ep);

			printf("TEST: testexe entry point: 0x%x %p (%s), disp 0x%x\n", EpRva, s, ((PPUBSYM32)s)->name, disp);

//			SYMDumpSymbols (syms->pdb->Symd, S_ALL);

			SymUnloadSymbols (syms);
		}

		UnmapViewOfFile (Image);
	}
}

int main (int argc, char **argv)
{
	if (argc == 2)
	{
		if (!_stricmp (argv[1], "--apply-from-nt-path"))
		{
			bApplyFromNtPath = true;
			argc = 1;
		}

		if (!_stricmp (argv[1], "/?") ||
			!_stricmp (argv[1], "-?") || 
			!_stricmp (argv[1], "--help") || 
			!_stricmp (argv[1], "-h"))
		{
			return printf("%s [path-to-pdb-file] [--apply-from-nt-path]\n--apply-from-nt-path      apply symbol search path from _NT_SYMBOL_PATH\n[c] Great, 2006-2010\n", PathFindFileName(argv[0]));
		}
	}
	else if (argc == 3)
	{
		if (!_stricmp (argv[2], "--apply-from-nt-path"))
		{
			bApplyFromNtPath = true;
			argc = 2;
		}
	}

	PPDB_SYMBOLS syms;

	FromNtPath ();

	// forcibily load symbols for self module and for kernel32.

	syms = SymLoadSymbolsForImage (GetModuleHandle (0));
	if (syms == NULL)
		return printf("Can't load self symbols (%s)\n", PdbGetErrorDescription(PdbGetLastError()));

	syms = SymLoadSymbolsForImage (GetModuleHandle ("kernel32.dll"));
	if (syms == NULL)
		return printf("Can't load symbols for kernel32 (%s)\n", PdbGetErrorDescription(PdbGetLastError()));

#if TEST_STACK_WALK
	f1();
#endif
	
#if TEST_DUMP_TPI
	if (syms)
	{
		TPIDumpTypes (syms->pdb->Symd->TpiHdr);
	}
#endif

#if TEST_NTOSKRNL
	test_kernel();
#endif

#if TEST_SELF_DLL
	test_selfdll();
#endif

#if TEST_SELF_EXE
	test_selfexe();
#endif

#if TEST_TEST_EXE
	test_another_mod_ep ("P:\\pdbparser\\testexe\\Debug\\testexe.exe");
	test_another_mod_ep ("P:\\pdbparser\\testexe\\Debug\\testdll.dll");
#endif

	SymUnloadAll();
	return 0;

}
