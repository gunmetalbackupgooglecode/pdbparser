#pragma once

#pragma warning(disable:4200)

//
// Microsoft MSF (Multi-Stream File) file format, version 7.0
//

//
// File starts with version-dependent headers. This structures apply only to the MSF 7.0
//

// 32-byte signature
#define MSF_SIGNATURE_700 "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0"

// MSF File Header
typedef struct MSF_HDR
{
	char szMagic[32];          // 0x00  Signature
	DWORD dwPageSize;          // 0x20  Number of bytes in the pages (i.e. 0x400)
	DWORD dwFpmPage;           // 0x24  FPM (free page map) page (i.e. 0x2)
	DWORD dwPageCount;         // 0x28  Page count (i.e. 0x1973)
	DWORD dwRootSize;          // 0x2c  Size of stream directory (in bytes; i.e. 0x6540)
	DWORD dwReserved;          // 0x30  Always zero.
	DWORD dwRootPointers[0x49];// 0x34  Array of pointers to root pointers stream. 
} *PMSF_HDR;

//
// MSF_HDR::dwRootPointers -> Root Directory Pointers -> Root Directory (stream pointers) -> Stream data
//

// Free Page Map
typedef struct MSF_FPM
{
	DWORD iwMax;
	DWORD iwRover;
	LONG cbPg;
	BOOLEAN fBigMsf;
	BYTE reserved1;
	WORD reserved2;
	struct {
		DWORD rgt;
		DWORD itMac;
		DWORD itMax;
	} rgw;
	DWORD wFill;
} *PMSF_FPM;

//
// Some PDB constants.
//

#define PDB_TI_MIN        0x00001000  // type info base
#define PDB_TI_MAX        0x00FFFFFF  // type info limit

// PDB Stream IDs
#define PDB_STREAM_ROOT   0
#define PDB_STREAM_PDB    1
#define PDB_STREAM_TPI    2
#define PDB_STREAM_DBI    3
#define PDB_STREAM_FPO    5

// Additional PDB format specification
#include "pdb_info.h"


// see RSDSI in pdb_info.h
//// CodeView debug info
//typedef struct CV_INFO_PDB
//{
//	DWORD CvSignature; // RSDS
//	GUID Signature;
//	DWORD Age;
//	BYTE PdbFileName[];
//} *PCV_INFO_PDB;

typedef struct MSF_ROOT
{
	DWORD dwStreamCount;
	DWORD dwStreamSizes[];
} *PMSF_ROOT;

#define spnNil ((DWORD)-1)

#pragma warning(default:4200)


//
// Useful macroses.
//

#define ALIGN_DOWN(x, align) ((x) & ~(align-1))
#define ALIGN_UP(x, align) (((x) & (align-1))?ALIGN_DOWN(x,align)+align:(x))

//
// The following structures describe a loaded in memory PDB, stream, etc.
// This is not a part of MSF/PDB format.
//

struct MSF_STREAM;

typedef struct MSF
{
	union {
		MSF_HDR* hdr;
		PVOID MapV;
		PUCHAR MapB;
	};
	ULONG MsfSize;
	ULONG RootPages;
	ULONG RootPointersPages;
	ULONG *pdwRootPointers;
	MSF_ROOT *Root;
	MSF_STREAM *pLoadedStreams;

	MSF_HDR HdrBackStorage;
} *PMSF;

#define PAGE(msf,x) (msf->MapB + msf->hdr->dwPageSize * (x))
#define STREAM_SPAN_PAGES(msf,size) (ALIGN_UP(size,msf->hdr->dwPageSize)/msf->hdr->dwPageSize)

MSF* MsfOpen (char *szFileName);
void MsfClose (MSF* msf);

typedef struct MSF_STREAM
{
	MSF* msf;
	ULONG iStreamNumber;
	LONG ReferenceCount;
	PVOID pStreamData;
	ULONG uStreamSize;
	ULONG nStreamPages;
	PULONG pdwStreamPointers;
} *PMSF_STREAM;

MSF_STREAM* MsfLoadStream (MSF* msf, ULONG iStreamNumber);

VOID MsfUnmapAndKeepStreams (MSF* msf);

PVOID MsfReferenceStream (MSF* msf, ULONG iStreamNumber, SIZE_T *uStreamSize OPTIONAL);
VOID MsfDereferenceStream (MSF* msf, ULONG iStreamNumber);

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(x) ((x) != 0)
#endif

template <class T>
struct MSF_STREAM_REF
{
	T* Data;
	ULONG Size;
};

template <class T>
BOOL MsfReferenceStreamType (MSF* msf, ULONG iStreamNumber, MSF_STREAM_REF<T> *streamRef)
{
	return ((streamRef->Data = (T*) MsfReferenceStream (msf, iStreamNumber, &streamRef->Size)) != NULL);
}
