/**
 * err.cpp  -  a kind of error reporting. just two exported routines to set and get last error value.
 * Error value codes are defined in err.h
 *
 * This is the part of the PDB parser.
 * (C) Great, 2010.
 * xgreatx@gmail.com
 */

#include <windows.h>
#include "err.h"

ULONG TlsErrIndex;
static bool bTlsErrInitialized = false;

static VOID PdbErrInit()
{
	TlsErrIndex = TlsAlloc ();
}

VOID PdbSetLastError(ULONG Err)
{
	if (!bTlsErrInitialized)
	{
		PdbErrInit();
		bTlsErrInitialized = true;
	}
	
	TlsSetValue (TlsErrIndex, (LPVOID)Err);
}

ULONG PdbGetLastError()
{
	if (!bTlsErrInitialized)
	{
		PdbErrInit();
		bTlsErrInitialized = true;
	}
	
	return (ULONG)TlsGetValue (TlsErrIndex);
}


static const char* szPdbErrorDescriptions[] = {
	"The operation succeeded.",
	"Not enough memory to complete the operation.",
	"No debug directory present in the specified image.",
	"Bad or missing MSF file.",
	"No matching PDB has been found in the local symbol storage. Try downloading it from the Web.",
	"GUID signature in the PDB does not match GUID signature in the file.",
	"Age in the image debug directory is greater than PDB age. PDB is too old.",
	"No symbol information present in the MSF file specified as PDB. Possibly, this file is not a PDB file.",
};

const char *PdbGetErrorDescription (ULONG err)
{
	if (err < PDBERR_MAX_ERR)
		return szPdbErrorDescriptions[err];
	return "Unknown error.";
}
