/**
 * msf.cpp  -  MSF parser core.
 * PDB parser uses this engine to parse a .pdb file.
 *
 * This is the part of the PDB parser.
 * (C) Great, 2010.
 * xgreatx@gmail.com
 */

#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>
#include <common/mapping.h>
#include "msf.h"
#include "misc.h"


//
// Caller supplies pointer to an array of page indices, and number of pages.
// This routine tries to load the corresponding pages from MSF file to the 
//  memory and returns pointer to it.
// This is an internal routine.
//
static PVOID MsfLoadPages (MSF* msf, ULONG *pdwPointers, SIZE_T nPages)
{
	PVOID Data = halloc (nPages * msf->hdr->dwPageSize);
	if (Data != NULL)
	{
		for (ULONG i=0; i<nPages; i++)
		{
			PVOID Page = PAGE(msf, pdwPointers[i]);
			SIZE_T Offset = msf->hdr->dwPageSize * i;

			memcpy ((PUCHAR)Data + Offset, Page, msf->hdr->dwPageSize);
		}
	}

	return Data;
}

//
// Open multi-stream file, parse its header and load root directory.
// Return Value: handle for successfully opened MSF or NULL on error.
//
MSF* MsfOpen (char *szFileName)
{
	SIZE_T Size;
	PVOID Map = MapExistingFile (szFileName, MAP_READ, NULL, &Size);

	if (Map != NULL)
	{
		MSF* msf = (MSF*) halloc (sizeof(MSF));

		if (msf != NULL)
		{
			msf->MapV = Map;
			msf->MsfSize = Size;
			
			msf->RootPages = STREAM_SPAN_PAGES (msf, msf->hdr->dwRootSize);
			msf->RootPointersPages = STREAM_SPAN_PAGES (msf, msf->RootPages*sizeof(ULONG));

			msf->pdwRootPointers = (PULONG) MsfLoadPages (msf, msf->hdr->dwRootPointers, msf->RootPointersPages);

			if (msf->pdwRootPointers != NULL)
			{
				msf->Root = (MSF_ROOT*) MsfLoadPages (msf, msf->pdwRootPointers, msf->RootPages);

				if (msf->Root != NULL)
				{
					msf->pLoadedStreams = (MSF_STREAM*) halloc (sizeof(MSF_STREAM) * msf->Root->dwStreamCount);
					if (msf->pLoadedStreams != NULL)
					{
						return msf;
					}

					hfree (msf->Root);
				}

				hfree (msf->pdwRootPointers);
			}

			hfree (msf);
		}

		UnmapViewOfFile (Map);
	}

	return NULL;
}

//
// Unmap mapped in memory MSF file, so file can safely be deleted.
// Call this function when all necessary information has already been loaded in memory.
//

VOID MsfUnmapAndKeepStreams (MSF* msf)
{
	msf->HdrBackStorage = *msf->hdr;
	UnmapViewOfFile (msf->MapV);
	msf->hdr = &msf->HdrBackStorage;
}

//
// No need in this routine anymore.
// pdb.cpp contains RSDS parsing routines.
//

// MSF* MsfOpenPdbForImage (PVOID Image)
// {
// 	PUCHAR Map = (PUCHAR)Image;
// 	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Map;
// 	PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)(Map + DosHeader->e_lfanew);
// 
// 	if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE &&
// 		NtHeaders->Signature == IMAGE_NT_SIGNATURE)
// 	{
// 		PIMAGE_DEBUG_DIRECTORY Debug;
// 		ULONG DebugRVA = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
// 
// 		Debug = (PIMAGE_DEBUG_DIRECTORY)(Map + DebugRVA);
// 
// 		if (DebugRVA != 0 && Debug->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
// 		{
// 			//CV_INFO_PDB* pdb = (CV_INFO_PDB*)(Map + Debug->AddressOfRawData);
// 			//return MsfOpen ((char*)pdb->PdbFileName);
// 
// 			PRSDSI rsds = (PRSDSI) (Map + Debug->AddressOfRawData);
// 
// 			if (rsds->dwSig == 'SDSR')
// 			{
// 				return MsfOpen ((char*)rsds->szPdb);
// 			}
// 		}
// 	}
// 
// 	return NULL;
// }

//
// Get the array of page indices for the specified stream.
// NB: this function does not have the return value. So, if
// the specified stream does not contain any pages, the pointer
// still will be returned. In this case, you should not try to
// dereference it - these pages are the pages of the NEXT stream.
//

void MsfGetStreamPointers (MSF* msf, ULONG iStreamNumber, PULONG* pdwPointers)
{
	PULONG StreamPointers = &msf->Root->dwStreamSizes[msf->Root->dwStreamCount];

	ULONG j = 0;
	for (ULONG i=0; i<iStreamNumber; i++)
	{
		ULONG nPages = STREAM_SPAN_PAGES (msf, msf->Root->dwStreamSizes[i]);

		j += nPages;
	}

	*pdwPointers = &StreamPointers[j];
}

//
// This function returns byte count occupied by the stream specified by its number.
// If the return value is -1, the stream does not exist.
//

ULONG MsfSizeOfStream (MSF* msf, ULONG iStreamNumber)
{
	return msf->Root->dwStreamSizes[iStreamNumber];
}

//
// This function tries to load stream from MSF with the specified number.
// On success, return value is a handle to loaded stream.
// On error, return value is 0.
// Therefore, you should not use this routine, 'cause all streams are loaded
//  in MsfOpen() and you should just reference them by MsfReferenceStream/MsfReferenceStreamByType call.
//

MSF_STREAM* MsfLoadStream (MSF* msf, ULONG iStreamNumber)
{
	MSF_STREAM* stream = &msf->pLoadedStreams[iStreamNumber];
	if (stream != NULL)
	{
		stream->msf = msf;
		stream->iStreamNumber = iStreamNumber;
		stream->ReferenceCount = 1;
		stream->uStreamSize = msf->Root->dwStreamSizes[iStreamNumber];

		if (stream->uStreamSize == spnNil)
		{
			stream->pStreamData = NULL;
			stream->nStreamPages = 0;
			stream->pdwStreamPointers = NULL;
			return stream;
		}

		stream->nStreamPages = STREAM_SPAN_PAGES (msf, stream->uStreamSize);

		MsfGetStreamPointers (msf, iStreamNumber, &stream->pdwStreamPointers);

		stream->pStreamData = MsfLoadPages (msf, stream->pdwStreamPointers, stream->nStreamPages);

		if (stream->pStreamData != NULL)
		{
			return stream;
		}

		memset (stream, 0, sizeof(MSF_STREAM));
	}

	return NULL;
}

//
// Free the memory occupied by stream related to MSF file.
// You can use MsfLoadStream() again later for the same stream.
//
void MsfUnloadStream (MSF_STREAM *s)
{
	if (s->pStreamData != NULL)
	{
		if (s->ReferenceCount > 0)
			printf("warning: trying to unload referenced stream #%d, refcount %d\n", s->iStreamNumber, s->ReferenceCount);

		hfree (s->pStreamData);
		memset (s, 0, sizeof(MSF_STREAM));
	}
}

//
// All streams are loaded when MSF is being opened.
// This routine just searches for the pointer of loaded stream.
// If there is no stream with the supplied number, the function returns NULL.
// Optionally, it returns stream size (in bytes) in the last parameter.
//

PVOID MsfReferenceStream (MSF* msf, ULONG iStreamNumber, SIZE_T *uStreamSize OPTIONAL)
{
	MSF_STREAM *s = &msf->pLoadedStreams[iStreamNumber];
	PVOID pData = s->pStreamData;

	if (pData != NULL)
	{
		InterlockedExchangeAdd (&s->ReferenceCount, (LONG)1);

		if (ARGUMENT_PRESENT (uStreamSize))
			*uStreamSize = s->uStreamSize;
	}

	return pData;
}

//
// Dereference loaded stream. In theory, all referenced streams should be dereferenced when are no longer need.
// But, this functionality is not implemented yet.
// All loaded streams resides in the system memory until the whole PDB is unloaded.
// When stream reference count reaches zero, the stream is being unloaded.
//
VOID MsfDereferenceStream (MSF* msf, ULONG iStreamNumber)
{
	MSF_STREAM *s = &msf->pLoadedStreams[iStreamNumber];

	// TODO: add synchronization here.

	LONG refCount = InterlockedExchangeAdd (&s->ReferenceCount, (LONG)-1);

	if (refCount == 1)
	{
		// reference count is zero.
		MsfUnloadStream (s);
	}
	else if (refCount == 0)
	{
		s->ReferenceCount = 0; // write back zero.
		
		//BUGBUG: dereferencing free stream! this is an error!
	}
}

//
// Close opened MSF file. So, it means: unload all streams,
//  free memory occupied by header-parser and unmap the whole MSF from memory.
//
void MsfClose (MSF *msf)
{
	for (ULONG i=0; i<msf->Root->dwStreamCount; i++)
	{
		MSF_STREAM *s = &msf->pLoadedStreams[i];
		if (s->pStreamData != NULL)
		{
			//MsfUnloadStream (s);
			MsfDereferenceStream (msf, i);

			MsfDereferenceStream (msf, i);

			if (s->ReferenceCount != 0)
				printf("warning: trying to close msf, unloading referenced stream #%d, refcount %d\n", s->iStreamNumber, s->ReferenceCount);
		}
	}

	hfree (msf->pLoadedStreams);
	hfree (msf->Root);
	hfree (msf->pdwRootPointers);

	if (msf->hdr != &msf->HdrBackStorage)
	{
		UnmapViewOfFile (msf->MapV);
	}

	hfree (msf);
}
