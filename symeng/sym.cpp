/**
* sym.cpp  -  miscellaneous routines for symbol management 
*  (downloading, searching, loading symbol files, looking for symbols, etc..)
*
* This is the part of the PDB parser.
* (C) Great, 2010.
* xgreatx@gmail.com
*/

#include <winsock2.h>
#include <ws2tcpip.h>
#include "sym.h"
#include "pdb.h"
#include <common/list.h>
#include "misc.h"
#include <stdio.h>
#include <common/assert.h>
#include <common/mapping.h>

extern "C" PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader( PVOID ImageBase );

void _cdecl PrintfNull (char*, ...)
{
	// do nothing.
}

void (_cdecl *PrintfCallout)(char*, ...) = &PrintfNull;

VOID SymSetPrintfCallout (PPRINTF_CALLOUT Callout)
{
	if (Callout != NULL)
		PrintfCallout = Callout;
	else
		PrintfCallout = &PrintfNull;
}

PVOID RtlGetModuleBase (PVOID Addr, ULONG *Rva)
{
	ULONG_PTR p;

	for (p = (ULONG_PTR)Addr & 0xFFFF0000; ; p -= 0x10000)
	{
		if (IsBadCodePtr((FARPROC)p))
			break;

		if (*(PUSHORT)p == IMAGE_DOS_SIGNATURE && RtlImageNtHeader((PVOID)p)->Signature == IMAGE_NT_SIGNATURE)
			break;
	}

	if (IsBadCodePtr((FARPROC)p))
		return NULL;

	*Rva = (ULONG_PTR)Addr - p;

	return (PVOID)p;
}

// no synchronization. this library is single-threaded.
static LIST_ENTRY SymbolsListHead = {&SymbolsListHead, &SymbolsListHead};

static char szSymbolPath[MAX_PATH];

VOID SymSetSymbolPath (PSTR SymbolPath)
{
	strcpy (szSymbolPath, SymbolPath);
}

PPDB_SYMBOLS SymFindLoadedSymbolsForImage (PVOID Image)
/*++
	Lookup loaded symbols for the image specified by its base address.
--*/
{
	for (PLIST_ENTRY e = SymbolsListHead.Flink; e != &SymbolsListHead; e = e->Flink)
	{
		PPDB_SYMBOLS syms = (PPDB_SYMBOLS) e;
		if (syms->ImageBase == Image)
		{
			return syms;
		}
	}
	return NULL;
}

static VOID SympGetSegmentsForImage (PPDB_SYMBOLS syms)
{
	PUCHAR Image = (PUCHAR)syms->ImageBase;
	syms->Dos = (PIMAGE_DOS_HEADER)Image;
	syms->Nt = (PIMAGE_NT_HEADERS)(Image + syms->Dos->e_lfanew);
	syms->Sections = (PIMAGE_SECTION_HEADER)((PUCHAR)&syms->Nt->OptionalHeader + syms->Nt->FileHeader.SizeOfOptionalHeader);
}

PPDB_SYMBOLS SymLoadSymbolsForImage (PVOID Image)
/*++
	Load symbols for the image
--*/
{
	PPDB_SYMBOLS syms;

	if ((syms = SymFindLoadedSymbolsForImage(Image)) != NULL)
		return syms; // already loaded.

	PrintfCallout ("Symbols have not been loaded for image %p, searching..\n", Image);

	syms = (PPDB_SYMBOLS) halloc (sizeof(PDB_SYMBOLS));
	if (syms != NULL)
	{
		PPDB pdb = PdbFindMatchingPdbForImage (Image, szSymbolPath);
		if (pdb != NULL)
		{
			syms->ImageBase = Image;
			syms->pdb = pdb;
			SympGetSegmentsForImage (syms);

			InsertTailList (&SymbolsListHead, &syms->Entry);

			return syms;
		}

		PrintfCallout ("Symbols could not be loaded for image %p\n", Image);

		hfree (syms);
	}

	return NULL;
}

PPDB_SYMBOLS SymLoadSymbolsForImageFile (PSTR szFileName)
/*++
	Load image from the specified file & load symbols for it.
--*/
{
	PPDB_SYMBOLS syms = NULL;

	PVOID Image = MapExistingFile (szFileName, MAP_IMAGE|MAP_READ, 0, 0);
	if (Image)
	{
		syms = SymLoadSymbolsForImage (Image);
		UnmapViewOfFile (Image);
	}

	return syms;
}

VOID SymUnloadSymbols (PPDB_SYMBOLS syms)
/*++
	Unload image symbols.
--*/
{
	RemoveEntryList(&syms->Entry);
	PdbClose (syms->pdb);
	hfree (syms);
}

VOID SymUnloadAll()
/*++
	Unload all loaded symbols.
--*/
{
	for (PLIST_ENTRY e = SymbolsListHead.Flink; e != &SymbolsListHead; )
	{
		PPDB_SYMBOLS syms = (PPDB_SYMBOLS) e;
		e = syms->Entry.Flink;

		SymUnloadSymbols (syms);
	}
}

BOOL SympFindBestOffsetInMatchingSection (PVOID ImageBase, PVOID Address, ULONG *SectionBaseRVA, ULONG *ImageRVA, ULONG *Offset, USHORT *Segment)
{
	PIMAGE_NT_HEADERS NtHeaders = RtlImageNtHeader (ImageBase);
	PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION (NtHeaders);

	*ImageRVA = (ULONG_PTR)Address - (ULONG_PTR)ImageBase;

	for (USHORT i=0; i<NtHeaders->FileHeader.NumberOfSections; i++)
	{
		ULONG VirtualSize;

		VirtualSize = ALIGN_UP (Sections[i].SizeOfRawData, NtHeaders->OptionalHeader.SectionAlignment);
		if (Sections[i].Misc.VirtualSize > VirtualSize)
			VirtualSize = Sections[i].Misc.VirtualSize;

		if (*ImageRVA >= Sections[i].VirtualAddress &&
			*ImageRVA < Sections[i].VirtualAddress + VirtualSize)
		{
			*SectionBaseRVA = Sections[i].VirtualAddress;
			*Segment = i+1;
			*Offset = *ImageRVA - *SectionBaseRVA;
			return TRUE;
		}
	}

	return FALSE;
}

PSYMBOL SymFromAddress (PVOID Address)
/*++
	Get symbol from address.
	Returned PSYMBOL must be freed by SymFreeSymbol() when is not needed anymore.
--*/
{
	ULONG Offset;
	PVOID Base = RtlGetModuleBase (Address, &Offset);

	if (Base != NULL)
	{
		PPDB_SYMBOLS syms;
		
		//syms = SymLoadSymbolsForImage (Base);
		syms = SymFindLoadedSymbolsForImage (Base);

		PIMAGE_NT_HEADERS NtHeaders = RtlImageNtHeader (Base);

		ULONG InSectionOffset, ImageRVA, SectionBaseRVA;
		USHORT Segment;

		if (SympFindBestOffsetInMatchingSection (Base, Address, &SectionBaseRVA, &ImageRVA, &InSectionOffset, &Segment))
		{
			if (syms != NULL)
			{
				return SYMGetSymFromAddr (syms->pdb->Symd, Segment, InSectionOffset);
			}
		}
	}

	return NULL;
}

VOID SymFreeSymbol (PSYMBOL Symbol)
/*++
	Free symbol information.
--*/
{
	SYMFreeSymbol (Symbol);
}

PSYMBOL SymNextSym (PPDB_SYMBOLS syms, PSYMBOL Symbol)
/*++
	Go to the next symbol.
--*/
{
	return SYMNextSym (syms->pdb->Symd, Symbol);
}

//////////////////////////////////////////////////////////////////////////

#define MS_SYMBOL_SERVER_AGENT "Microsoft-Symbol-Server/6.6.0007.5"

typedef struct _CRACKED_URL
{
	PSTR szProtocol;
	PSTR szHostName;
	USHORT uPort;
	PSTR szObjectPath;
} CRACKED_URL, *PCRACKED_URL;

static VOID FreeUrl (PCRACKED_URL Url)
/*++
	Free cracked url.
--*/
{
	hfree (Url->szProtocol);
}

static BOOL CrackUrl (PSTR szServerUrl, PCRACKED_URL Url)
/*++
	Crack URL and split into the parts.
	Cracked url must be freed by FreeUrl()
--*/
{
	PSTR szUrl = xstrdup (szServerUrl);

	PSTR pProto = strstr (szUrl, "://");
	if (pProto)
	{
		Url->szProtocol = szUrl;
		*pProto = 0;

		PSTR pFirstSlash = strchr (&pProto[3], '/');
		if (pFirstSlash)
		{
			SIZE_T Len = pFirstSlash - &pProto[3];

			memmove (&pProto[2], &pProto[3], Len);
			pFirstSlash[-1] = 0;

			Url->szHostName = &pProto[2];
			Url->szObjectPath = pFirstSlash;
		}
		else
		{
			Url->szObjectPath = "/";
			Url->szHostName = &pProto[3];
		}

		Url->uPort = 80;

		PSTR pPort = strchr (Url->szHostName, ':');
		if (pPort)
		{
			*(pPort++) = 0;
			Url->uPort = atoi (pPort);
		}

		return TRUE;
	}

	return FALSE;
}

static const char *szSymRequestFormat = 
"GET %s%s%s HTTP/1.1\r\n"
"User-Agent: %s\r\n"
"Host: %s\r\n"
"Connection: Keep-Alive\r\n"
"Cache-Control: no-cache\r\n"
"Cookie: A=I&I=AxUFAAAAAAA/BwAAiyuzHeFC9Wr/2d6GgxRHGg!!&M=1; MC1=GUID=9cd0ed4ebbb93448869ee604aac421da&HASH=4eed&LV=20102&V=3\r\n"
"\r\n";

static BOOL RequestSucceeded (PSTR response)
{
	PSTR pSpace = strchr (response, ' ');
	if (pSpace != NULL)
	{
		pSpace++;
		if (pSpace[0] == '2' && pSpace[1] == '0' && pSpace[2] == '0')
		{
			return TRUE;
		}
	}
	return FALSE;
}

static int recv_http_header (int sock, char* data, int datalen, int flags)
{
	int pos = 0;
	int bytes = 0;

	do 
	{
		bytes = recv (sock, &data[pos], datalen - pos, 0);
		if (bytes == SOCKET_ERROR)
			break;
		data[bytes+pos] = 0;
		pos += bytes;
		if (strstr (data, "\r\n\r\n") != NULL)
			break;
	} 
	while (pos < datalen);

	return pos;
}

static BOOL MakePath (PSTR szSymSignature)
{
	PSTR pSlash, pPrev = szSymSignature;
	char directory[MAX_PATH];

	while ((pSlash = strchr (pPrev, '\\')) != NULL)
	{
		size_t len = pSlash - szSymSignature;
		strncpy (directory, szSymSignature, len);
		directory[len] = 0;

		if (!CreateDirectory (directory, 0))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				return FALSE;
			}
		}

		pPrev = pSlash + 1;
	}

	return TRUE;
}

static BOOL SymSocketSaveTo (PSTR bufferTail, int tailLength, int sock, PSTR szFilePath, ULONG Length)
{
	int bytesRead = tailLength;
	DWORD wr;
	BOOL bResult = FALSE;
	HANDLE hFile = CreateFile (szFilePath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile (hFile, bufferTail, tailLength, &wr, 0);

		char buffer[1024];
		int bytes;

		while (bytesRead < (int)Length)
		{
			PrintfCallout ("bytes read: %-10d\r", bytesRead);

			bytes = recv (sock, buffer, min(sizeof(buffer),Length-bytesRead), 0);
			if (bytes == SOCKET_ERROR)
				break;

			WriteFile (hFile, buffer, bytes, &wr, 0);
			bytesRead += bytes;
		}

		PrintfCallout ("bytes read: %-10d\n", bytesRead);

		if (bytes != SOCKET_ERROR)
		{
			bResult = TRUE;
		}
		else
		{
			PrintfCallout("Failed receiving file\n");
		}

		CloseHandle (hFile);
	}

	return bResult;
}

static BOOL SympHttpHandleConnection (PSTR szSymSignature, int sock, PCRACKED_URL Url, PSTR szLocalStorage)
{
	char *szRequest = (char*) halloc (
		strlen (szSymRequestFormat) +
		strlen(Url->szHostName) + 
		strlen(Url->szObjectPath) + 
		strlen(szSymSignature) + 
		strlen(MS_SYMBOL_SERVER_AGENT)
		);
	char buffer[1024];
	int l;
	BOOL bResult = FALSE;

	if (szRequest != NULL)
	{
		char *szSymPath = xstrdup (szSymSignature);
		
		if (szSymPath != NULL)
		{
			xstrreplacechar (szSymPath, '\\', '/');

			l = sprintf (szRequest, 
				szSymRequestFormat, 
				Url->szObjectPath,
				Url->szObjectPath[strlen(Url->szObjectPath)-1] == '/' ? "" : "/",
				szSymPath,
				MS_SYMBOL_SERVER_AGENT,
				Url->szHostName);

			send (sock, szRequest, l, 0);
			l = recv_http_header (sock, buffer, sizeof(buffer)-1, 0);

			if (l != SOCKET_ERROR)
			{
				if (RequestSucceeded (buffer))
				{
					PSTR pHeaderEnd = strstr (buffer, "\r\n\r\n");
					if (pHeaderEnd != NULL)
					{
						pHeaderEnd[2] = 0;

						_strlwr (buffer);

						PCSTR szContentLength = xstrstrskip (buffer, "content-length:");
						if (szContentLength != NULL)
						{
							while (isspace(*szContentLength)) szContentLength++;
							ULONG ContentLength = atoi (szContentLength);

							PrintfCallout("PDB file size: %d bytes\n", ContentLength);

							char PreviousDirectory[MAX_PATH];
							GetCurrentDirectory (sizeof(PreviousDirectory)-1, PreviousDirectory);
							SetCurrentDirectory (szLocalStorage);

							bResult = MakePath (szSymSignature);

							if (bResult)
							{
								bResult = SymSocketSaveTo (&pHeaderEnd[4], l - (&pHeaderEnd[4] - buffer), sock, szSymSignature, ContentLength);
							}

							SetCurrentDirectory (PreviousDirectory);
						}
					}
				}
			}

			hfree (szSymPath);
		}

		hfree (szRequest);
	}

	return bResult;
}

BOOL SymDownloadFromServerPdb (PSTR szPdbSignature, PSTR szServerUrl, PSTR szLocalStorage)
{
	BOOL bResult = FALSE;
	int sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock != SOCKET_ERROR)
	{
		CRACKED_URL Url;
		addrinfo hint, *info;

		if (CrackUrl (szServerUrl, &Url))
		{
			memset (&hint, 0, sizeof(hint));
			hint.ai_family = AF_INET;
			hint.ai_socktype = SOCK_STREAM;
			hint.ai_protocol = IPPROTO_TCP;

			if (getaddrinfo (Url.szHostName, "80", &hint, &info) == 0)
			{
				SOCKADDR_IN sa = {0};
				sa.sin_family = AF_INET;
				sa.sin_addr = ((SOCKADDR_IN*)info->ai_addr)->sin_addr;
				sa.sin_port = htons (Url.uPort);

				freeaddrinfo (info);

				if (connect (sock, (sockaddr*)&sa, sizeof(sa)) != SOCKET_ERROR)
				{
					bResult = SympHttpHandleConnection (szPdbSignature, sock, &Url, szLocalStorage);
				}
			}

			FreeUrl (&Url);
		}

		closesocket (sock);
	}

	return bResult;
}

BOOL SymDecompressPdb (PSTR szCompressedName, PSTR szPdbName, PSTR szLocalStorage)
{
	BOOLEAN bResult = FALSE;
	char commandLine[MAX_PATH];
	sprintf (commandLine, "expand %s %s", szCompressedName, szPdbName);

	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi;

	if (CreateProcess (0, commandLine, 0, 0, FALSE, CREATE_NO_WINDOW, 0, szLocalStorage, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		bResult = TRUE;
	}
	else
	{
		PrintfCallout("failed to create process '%s'\n", commandLine);
	}

	return bResult;
}

BOOL SymDownloadFromServerForImage (PVOID Image, PSTR szServerUrl, PSTR szLocalStorage)
{
	WSADATA WsaData;
	BOOL bResult = FALSE;
	WSAStartup (0x202, &WsaData);

	if (szLocalStorage == NULL)
	{
		szLocalStorage = szSymbolPath;
	}

	char *szDownloadedPdbName = NULL;

	char szSymSignature[MAX_PATH];
	if (PdbGetImageSignatureAndPath (Image, szSymSignature, sizeof(szSymSignature)))
	{
		PrintfCallout ("Trying %s\n", szSymSignature);
		bResult = SymDownloadFromServerPdb (szSymSignature, szServerUrl, szLocalStorage);

		if (bResult)
		{
			szDownloadedPdbName = xstrdup (szSymSignature);
		}
		else
		{
			char *szCompressedName = xstrdup (szSymSignature);

			if (szCompressedName != NULL)
			{
				// try to download compressed PDB
				szCompressedName [strlen(szCompressedName)-1] = '_';

				PrintfCallout ("Trying %s\n", szCompressedName);
				bResult = SymDownloadFromServerPdb (szCompressedName, szServerUrl, szLocalStorage);

				if (bResult)
				{
					PrintfCallout ("Decompressing PDB ... ");
					bResult = SymDecompressPdb (szCompressedName, szSymSignature, szLocalStorage);
					PrintfCallout ("DONE\n");
					szDownloadedPdbName = szCompressedName;
				}
			}
		}
	}

	if (bResult)
	{
		ASSERT (szDownloadedPdbName != NULL);

		PrintfCallout ("Downloaded PDB to %s\\%s\n", szLocalStorage, szDownloadedPdbName);
		hfree (szDownloadedPdbName);
	}

	WSACleanup ();
	return bResult;
}
