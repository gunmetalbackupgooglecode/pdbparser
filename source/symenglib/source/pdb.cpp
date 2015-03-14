/**
 * pdb.cpp  -  PDB parser core.
 * SYM and GSI routines use this engine to parse a .pdb file.
 *
 * This is the part of the PDB parser.
 * (C) Great, 2010.
 * xgreatx@gmail.com
 */

#include <windows.h>
#include <stdio.h>
#include "misc.h"
#include "msf.h"
#include "tpi.h"
#include "gsi.h"
#include "pdb.h"
#include <common/mapping.h>
#include <shlwapi.h>
#include <common/dbg.h>

// These functions have been removed from gsi.h 'cause they're not exported.
// They are used as internal functions (called from PdbOpenValidate and PdbClose)
VOID GSIInit (PSYMD Symd);
VOID GSIClose (PSYMD Symd);

extern void (_cdecl *PrintfCallout)(char*, ...);

//
// This internal debug routine dumps the contents of MSF header.
// In fact, it should be in msf.cpp, but...
// This routine is never used.
//
// static void MsfDumpHeader (MSF* msf)
// {
// 	printf("msf file loaded at %p\n", msf);
// 	printf("msf signature: %s\n", msf->hdr->szMagic);
// 	printf(" page size: %08x\n", msf->hdr->dwPageSize);
// 	printf(" FPM page: %08x\n", msf->hdr->dwFpmPage);
// 	printf(" page count: %08x\n", msf->hdr->dwPageCount);
// 	printf(" (full size: %08x)\n", msf->hdr->dwPageSize * msf->hdr->dwPageCount);
// 
// 	ULONG RootPages = STREAM_SPAN_PAGES(msf, msf->hdr->dwRootSize);
// 	ULONG RootPointersPages = STREAM_SPAN_PAGES(msf, RootPages*sizeof(ULONG));
// 
// 	printf(" root size: %08x (0x%x pages)\n", msf->hdr->dwRootSize, RootPages);
// 	printf(" root pointers size: %08x (0x%x pages)\n", RootPages*sizeof(ULONG), RootPointersPages);
// 
// 	printf(" reserved: %08x\n", msf->hdr->dwReserved);
// 	for (ULONG i=0; i<RootPointersPages; i++)
// 		printf(" root[%d]: %08x\n", i, msf->hdr->dwRootPointers[i]);
// 
// 	printf(" (root[0] offset: %08x)\n", msf->hdr->dwRootPointers[0] * msf->hdr->dwPageSize);
// 
// 	printf("Root Pointers:\n");
// 	for (ULONG i=0; i<RootPages; i++)
// 		printf("[%d] = %08x\n", i, msf->pdwRootPointers[i]);
// 
// 	printf("Root: %p\n", msf->Root);
// 	printf(" number of streams: %08x\n", msf->Root->dwStreamCount);
// }

//
// This routine looks for debug directory entry in image NT header.
// If the debug directory is present, and its type is CodeView debug information,
//  the function returns the pointer to CodeView debug information (RSDSI structure).
// Also, this structure contains the name of PDB file.
// Return Value: pointer to RSDSI debug directory (if present), NULL otherwise.
//
PRSDSI PdbpGetImageRsds (PVOID Image)
{
	PUCHAR Map = (PUCHAR)Image;
	if (Image != NULL)
	{
		PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Map;
		PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)(Map + DosHeader->e_lfanew);

		if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE &&
			NtHeaders->Signature == IMAGE_NT_SIGNATURE)
		{
			PIMAGE_DEBUG_DIRECTORY Debug;
			ULONG DebugRVA = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

			Debug = (PIMAGE_DEBUG_DIRECTORY)(Map + DebugRVA);

			if (DebugRVA != 0 && Debug->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
			{
				//CV_INFO_PDB* pdb = (CV_INFO_PDB*)(Map + Debug->AddressOfRawData);
				//return MsfOpen ((char*)pdb->PdbFileName);

				PRSDSI rsds = (PRSDSI) (Map + Debug->AddressOfRawData);

				if (rsds->dwSig == 'SDSR')
				{
					return rsds;
				}
			}
		}
	}

	return NULL;
}

//
// This function simply retrieves the name of PDB from RSDSI CodeView information
//  located in image debug directory.
// Return Value: PDB name ASCIIZ string if debug directory is present, NULL otherwise.
//
PSTR PdbpGetImagePdbName (PVOID Image)
{
	PRSDSI rsds = PdbpGetImageRsds (Image);
	return rsds ? (PSTR)rsds->szPdb : NULL;
}

//
// This routine searches for PDB name for the supplied image and returns the string
//  containing signature GUID and age for the PDB (which can be passed to a symbol server, for example).
// Return Value: TRUE if debug directory is present, FALSE otherwise.
//
BOOL PdbGetImageSignatureAndPath (PVOID Image, char *lpBuffer, int cchMax)
{
	PRSDSI rsds = PdbpGetImageRsds(Image);
	if (rsds != NULL)
	{
		lpBuffer[cchMax - 1] = 0;
		_snprintf(lpBuffer, cchMax - 1,
			"%s\\%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%d\\%s",
			(char*)rsds->szPdb,
			rsds->guidSig.Data1,
			rsds->guidSig.Data2,
			rsds->guidSig.Data3,
			rsds->guidSig.Data4[0], rsds->guidSig.Data4[1], rsds->guidSig.Data4[2], rsds->guidSig.Data4[3],
			rsds->guidSig.Data4[4], rsds->guidSig.Data4[5], rsds->guidSig.Data4[6], rsds->guidSig.Data4[7],
			rsds->age,
			(char*)rsds->szPdb);

		return TRUE;
	}

	return FALSE;
}

//
// Try to open PDB (with EXACT path) routine and validate its signature (GUID and age supplied).
// Return Value: pointer to PDB handle on success, NULL otherwise.
//  You should treat this value as opaque, but in rare cases you can safely read PDB structure (NOT modify it).
// For example, you can access symbol table : (pdb->Symd->SymRecs).
// NB: You can supply NULL in guidSig and age, in this case no checks will be performed.
// Also, you can use PdbOpen(), which does the same.
// NB2: No search if performed, szPdbFileName should have the EXACT CORRECT path to pdb file. Otherwise, function fails.
//
PPDB PdbOpenValidate (char *szPdbFileName, GUID* guidSig OPTIONAL, DWORD age OPTIONAL)
{
	PPDB pdb;

	// Create PDB descriptor
	pdb = (PPDB) halloc (sizeof(PDB));
	if (pdb != NULL)
	{
		// Open file
		pdb->msf = MsfOpen (szPdbFileName);
		if (pdb->msf != NULL)
		{
			// Load all streams.
			for (ULONG i=0; i<pdb->msf->Root->dwStreamCount; i++)
			{
				MsfLoadStream (pdb->msf, i);
			}

			MsfReferenceStreamType (pdb->msf, PDB_STREAM_PDB, &pdb->pdb);
			MsfReferenceStreamType (pdb->msf, PDB_STREAM_DBI, &pdb->dbi);

			if (ARGUMENT_PRESENT (guidSig))
			{
				if (pdb->pdb.Data->sig70 != *guidSig)
				{
					// GUID mismatch.
					PdbClose (pdb);
					PdbSetLastError (PDBERR_SIGNATURE_MISMATCH);
					return NULL;
				}

				if (pdb->pdb.Data->pdbstream.age < age)
				{
					// age mismatch.
					PdbClose (pdb);
					PdbSetLastError (PDBERR_PDB_TOO_OLD);
					return NULL;
				}

				// verified.
			}

			// Load symbol information from PDB
			pdb->Symd = SYMLoadSymbols (pdb->msf);

			if (pdb->Symd != NULL)
			{
				pdb->szPdbFileName = (char*) halloc (MAX_PATH);
				GetFullPathName (szPdbFileName, MAX_PATH-1, pdb->szPdbFileName, 0);

				// all ok!
				// we are ready to return new handle..

				// .. but we have to initialize GSI
				GSIInit (pdb->Symd);

				// done! return the handle.
				return pdb;
			}

			PdbSetLastError (PDBERR_NO_SYMBOL_INFORMATION);

			MsfClose (pdb->msf);
		}
		else
		{
			PdbSetLastError (PDBERR_BAD_OR_MISSING_FILE);
		}

		hfree (pdb);
	}
	else
	{
		PdbSetLastError (PDBERR_NO_MEMORY);
	}

	return NULL;
}

//
// Try to find & open PDB with no signature checks.
// Return Value: PPDB handle (see PdbOpenValidate for details)
//
PPDB PdbOpen (char *szPdbFileName)
{
	return PdbOpenValidate (szPdbFileName, NULL, 0);
}

//
// This internal routine tries to find & open PDB in the specified directory.
// Return Value: PPDB handle.
//
static PPDB PdbpFindInDirectory (PRSDSI rsds, PSTR szPdbName, PSTR szSymbolSearchPath)
{
	WIN32_FIND_DATAA wfd;
	char PreviousDirectory[MAX_PATH];
	PPDB pdb;

	GetCurrentDirectory (sizeof(PreviousDirectory), PreviousDirectory);
	SetCurrentDirectory (szSymbolSearchPath);

	HANDLE hSearch = FindFirstFileA ("*.*", &wfd);
	if (hSearch)
	{
		do 
		{
			if (wfd.cFileName[0] == '.')
				continue;

			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Try this directory.
				pdb = PdbpFindInDirectory (rsds, szPdbName, wfd.cFileName);

				if (pdb != NULL)
				{
					// Found. Return this handle.
					SetCurrentDirectory (PreviousDirectory);
					return pdb;
				}
				
				// Not found, go next.
			}
			else if (!_stricmp (PathFindExtension (wfd.cFileName), ".pdb"))
			{
				// Try this file.
				pdb = PdbOpenValidate (wfd.cFileName, &rsds->guidSig, rsds->age);

				if (pdb != NULL)
				{
					// Found. Return this handle.
					SetCurrentDirectory (PreviousDirectory);
					return pdb;
				}

				// Not found, go next.
			}
		} 
		while (FindNextFileA (hSearch, &wfd));

		FindClose (hSearch);
	}
	
	SetCurrentDirectory (PreviousDirectory);
	return NULL;
}

#if DBG

//
// This debug routine simply dumps RSDS contents for the debug directory of image.
//
VOID PdbpDumpRsds (PRSDSI rsds)
{
	printf("sig %08x age %08x guid %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x pdb '%s'\n",
		rsds->dwSig,
		rsds->age,
		rsds->guidSig.Data1,
		rsds->guidSig.Data2,
		rsds->guidSig.Data3,
		rsds->guidSig.Data4[0], rsds->guidSig.Data4[1], rsds->guidSig.Data4[2], rsds->guidSig.Data4[3],
		rsds->guidSig.Data4[4], rsds->guidSig.Data4[5], rsds->guidSig.Data4[6], rsds->guidSig.Data4[7],
		rsds->szPdb);
}

#else

#define PdbpDumpRsds(RSDS) ((void)0)

#endif

//
// This routine tries to open PDB from symbol storage.
// Usually, PDBs are located in the following way:
// D:\SymbolStorage\kernel32.pdb\4CD38B3D90FF4A919CD8FC60762EE4B32\kernel32.pdb
// Where the part 'D:\SymbolStorage\' is the absolute path to symbol storage,
//       the part 'kernel32.pdb\' is the name of the pdb (actually, a directory with PDBs for different versions of kernel32),
//       the part '4CD38B3D90FF4A919CD8FC60762EE4B32' is a GUID signature and age,
//       the part '\kernel32.pdb' is a actual relative name of PDB file.
// If the PDB on the file system does not have the specified name, this function fails.
//
PPDB PdbpTryOpenFromStorage (PVOID Image, PSTR szSymbolSearchPath)
{
	char PreviousDirectory[MAX_PATH];
	char SymbolFileName[1024];
	PPDB pdb = NULL;
	PRSDSI rsds;

	if (PdbGetImageSignatureAndPath (Image, SymbolFileName, sizeof(SymbolFileName)))
	{
		GetCurrentDirectory (sizeof(PreviousDirectory), PreviousDirectory);
		SetCurrentDirectory (szSymbolSearchPath);

		rsds = PdbpGetImageRsds (Image);
		pdb = PdbOpenValidate (SymbolFileName, &rsds->guidSig, rsds->age);

		SetCurrentDirectory (PreviousDirectory);
	}
	else
	{
		PdbSetLastError (PDBERR_NO_DEBUG_DIRECTORY);
	}

	return pdb;
}

//
// This routine performs full search in the symbol storage.
// First, this routine checks if PDB name in the image is absolute. In this case, function just opens it.
// Then routine checks the following cases (for example pdb name in image is 'kernel32.pdb', symbol path is 'D:\Symbols')
//  - D:\Symbols\kernel32.pdb
//  - D:\Symbols\kernel32.pdb\4CD38B3D90FF4A919CD8FC60762EE4B32\kernel32.pdb
// (where 4CD38B3D90FF4A919CD8FC60762EE4B32 is GUID+signature from RSDS)
// If no PDB would be found with these names, function fails.
//
PPDB PdbFindMatchingPdbForImage (PVOID Image, PSTR szSymbolSearchPath)
{
	WIN32_FIND_DATAA wfd;
	PSTR szPdbName;
	char PreviousDirectory[MAX_PATH];
	PRSDSI rsds;
	PPDB pdb = NULL;

	rsds = PdbpGetImageRsds (Image);
	if (rsds == NULL)
	{
		PrintfCallout (__FUNCTION__ ": no debug information present (IMAGE_DIRECTORY_ENTRY_DEBUG) in image or it is not valid.\n");

		PdbSetLastError (PDBERR_NO_DEBUG_DIRECTORY);

		// no debug info
		return NULL;
	}

	PdbpDumpRsds (rsds);

	szPdbName = (char*)rsds->szPdb;

	if (PathIsRelative (szPdbName) == FALSE)
	{
		//if ((pdb = PdbOpenForImage (Image)) != NULL)

		if ((pdb = PdbOpenValidate (szPdbName, &rsds->guidSig, rsds->age)) != NULL)
		{
			PrintfCallout (__FUNCTION__ ": opened by absolute path: %s\n", szPdbName);
			return pdb;
		}

		PrintfCallout (__FUNCTION__ ": absolute path to PDB file is invalid: %s.\n", szPdbName);
	}

	PrintfCallout (__FUNCTION__ ": found relative PDB name in debug info: %s, searching..\n", szPdbName);

	GetCurrentDirectory (sizeof(PreviousDirectory), PreviousDirectory);
	SetCurrentDirectory (szSymbolSearchPath);

	char *szRelativePDBName = PathFindFileName (szPdbName);
	if (szRelativePDBName)
	{
		// try just open pdb from storage (i.e. D:\Symbols\kernel32.pdb)

		if ((pdb = PdbOpenValidate (szRelativePDBName, &rsds->guidSig, rsds->age)) != NULL)
		{
			PrintfCallout (__FUNCTION__ ": opened by simple path: %s\\%s\n", szSymbolSearchPath, szPdbName);
			goto exit;
		}
	}

	// Try to open PDB by its full path with GUID and age
	if ((pdb = PdbpTryOpenFromStorage (Image, szSymbolSearchPath)) != NULL)
	{
		PrintfCallout (__FUNCTION__ ": opened from storage\n");
		return pdb;
	}

	// Enumerate all PDBs...
	HANDLE hSearch = FindFirstFileA ("*.pdb", &wfd);
	if (hSearch)
	{
		do 
		{
			PSTR ext = PathFindExtensionA (wfd.cFileName);
			if (!_stricmp (ext, ".pdb"))
			{
				if (!_stricmp (wfd.cFileName, szPdbName))
				{
					if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						pdb = PdbpFindInDirectory (rsds, szPdbName, szPdbName);
						break;
					}
				}
			}
		}
		while (FindNextFileA(hSearch, &wfd));

		FindClose (hSearch);
	}

	if (pdb == NULL)
		PdbSetLastError (PDBERR_NOT_FOUND_IN_STORAGE);

exit:
	SetCurrentDirectory (PreviousDirectory);
	return pdb;
}

//
// This routine tries to open image file and find&open PDB for it.
//
PPDB PdbFindMatchingPdbForImageFile (PSTR szImageFileName, PSTR szSymbolSearchPath)
{
	PVOID Image;
	PPDB Pdb = NULL;

	Image = MapExistingFile (szImageFileName, MAP_IMAGE | MAP_READ, 0, NULL);
	if (Image != NULL)
	{
		Pdb = PdbFindMatchingPdbForImage (Image, szSymbolSearchPath);

		UnmapViewOfFile (Image);
	}

	return Pdb;
}

//
// This routine tries to open PDB by absolute path for the specified image.
// If the PDB path in image debug directory is absolute, function fails.
//
PPDB PdbOpenForImage (PVOID Image)
{
	PSTR szPdbPath = PdbpGetImagePdbName (Image);
	if (szPdbPath && PathIsRelativeA(szPdbPath) == FALSE)
	{
		return PdbOpen (szPdbPath);
	}

	PdbSetLastError (PDBERR_NOT_FOUND_IN_STORAGE);

	return NULL;
}

//
// This routine tries to open PDB for image specified by its file name.
// After mapping image file, this routine calls PdbOpen()
//
PPDB PdbOpenForImageFile (char *szImageFileName)
{
	PVOID Image;
	PPDB Pdb = NULL;

	Image = MapExistingFile (szImageFileName, MAP_IMAGE | MAP_READ, 0, NULL);
	if (Image != NULL)
	{
		Pdb = PdbOpenForImage (Image);

		UnmapViewOfFile (Image);
	}

	return Pdb;
}

//
// This routine closes PDB opened by any of the previous routines.
// The supplied handle will be no longer available after the call.
// All streams in MSF will be dereferenced & freed too, so referenced pointers
//  will be no longer available too.
//
VOID PdbClose (PPDB pdb)
{
	GSIClose (pdb->Symd);
	if (pdb->szPdbFileName != NULL)
	{
		hfree (pdb->szPdbFileName);
	}
	if (pdb->Symd != NULL)
	{
		SYMUnloadSymbols (pdb->Symd);
	}
	MsfDereferenceStream (pdb->msf, PDB_STREAM_DBI);
	MsfDereferenceStream (pdb->msf, PDB_STREAM_PDB);
	MsfClose (pdb->msf);
	hfree (pdb);
}
