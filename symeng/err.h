#pragma once

///////////////////////////////////////////////////////////////////////////////

// Not enough memory to allocate some system structures. Try again later, please.
#define PDBERR_NO_MEMORY                0x0001

// No debug directory present in the file.
#define PDBERR_NO_DEBUG_DIRECTORY       0x0002

// File was found, but could not be opened. Possibly, that file was not a PDB file,
//  or it was the PDB file for another module, or the FS has bad sectors.
#define PDBERR_BAD_OR_MISSING_FILE      0x0003

// Search has been performed in the local symbol storage, but no PDB file 
// with the suggested name was found in the local symbol storage. Try to download it.
#define PDBERR_NOT_FOUND_IN_STORAGE     0x0004

// GUID signature from RSDS image debug directory does not match GUID signature in the file.
#define PDBERR_SIGNATURE_MISMATCH       0x0005

// Age in the image debug directory is greater than PDB age.
#define PDBERR_PDB_TOO_OLD              0x0006

// This multi-stream file does not contain any symbol information.
// Maybe you have supplied a path to BSC or any other MSF-like file?
#define PDBERR_NO_SYMBOL_INFORMATION    0x0007

#define PDBERR_MAX_ERR                  0x0008

///////////////////////////////////////////////////////////////////////////////

// Symbol server did not answer to the request.
#define PDBERR_SYMBOL_SERVER_DOWN       0x0101

// Symbol server rejected the request. Maybe, it is not a symbol server?
#define PDBERR_SYMBOL_SERVER_FAILED     0x0102

// Compressed PDB file has been downloaded from symbol server, but 'expand' command
//  failed. You have to manually extract pdb from cabinet file .pd_
#define PDBERR_EXPAND_FAILED            0x0103

///////////////////////////////////////////////////////////////////////////////

VOID PdbSetLastError(ULONG Err);
ULONG PdbGetLastError();
const char *PdbGetErrorDescription (ULONG err);

///////////////////////////////////////////////////////////////////////////////
