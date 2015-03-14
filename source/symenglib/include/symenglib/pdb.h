#pragma once

#include "msf.h"
#include "gsi.h"
#include "err.h"

typedef struct _PDB
{
	PSTR szPdbFileName;
	PMSF msf;  // MSF handle for loaded multi-stream file
	MSF_STREAM_REF<PDBStream70> pdb;
	MSF_STREAM_REF<NewDBIHdr> dbi;
	PSYMD Symd;
} PDB, *PPDB;

BOOL PdbGetImageSignatureAndPath (PVOID Image, char *lpBuffer, int cchMax);

PPDB PdbOpen (char *szPdbFileName);
PPDB PdbOpenForImage (PVOID Image);
PPDB PdbOpenForImageFile (char *szImageFileName);
VOID PdbClose (PPDB Pdb);

PPDB PdbFindMatchingPdbForImage (PVOID Image, PSTR szSymbolSearchPath);
PPDB PdbFindMatchingPdbForImageFile (PSTR szImageFileName, PSTR szSymbolSearchPath);
