#pragma once

#include "pdb.h"

PVOID RtlGetModuleBase (PVOID Addr, ULONG *Rva);

typedef struct _PDB_SYMBOLS
{
	LIST_ENTRY Entry;
	PPDB pdb;
	PVOID ImageBase;
	PIMAGE_DOS_HEADER Dos;
	PIMAGE_NT_HEADERS Nt;
	PIMAGE_SECTION_HEADER Sections;
} PDB_SYMBOLS, *PPDB_SYMBOLS;

VOID SymSetSymbolPath (PSTR SymbolPath);
PPDB_SYMBOLS SymFindLoadedSymbolsForImage (PVOID Image);
PPDB_SYMBOLS SymLoadSymbolsForImage (PVOID Image);
VOID SymUnloadSymbols (PPDB_SYMBOLS syms);
VOID SymUnloadAll();
PSYMBOL SymFromAddress (PVOID Address);
VOID SymFreeSymbol (PSYMBOL Symbol);
PSYMBOL SymNextSym (PPDB_SYMBOLS syms, PSYMBOL Symbol);
BOOL SymDownloadFromServerForImage (PVOID Image, PSTR szServerUrl, PSTR szLocalStorage);

typedef void (_cdecl *PPRINTF_CALLOUT)(PSTR, ...);
VOID SymSetPrintfCallout (PPRINTF_CALLOUT Callout);
PPDB_SYMBOLS SymLoadSymbolsForImageFile (PSTR szFileName);
