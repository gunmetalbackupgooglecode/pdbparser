#include <windows.h>
#include "resource.h"
#include "misc.h"
#include "sym.h"
#include <common/mapping.h>
#include <stdarg.h>
#include <stdio.h>

#define WM_DOWNLOAD_COMPLETED (WM_USER + 1)

char *szFiles[256] = {0};
int nFiles = 0;

HWND ghWnd;
HWND hLog;

HLOCAL hLogData;

char *gszLog;
int LogLen;
int LogSize;

VOID _cdecl LogPrintf (char *format, ...)
{
	va_list va;
	char msg[512];
	int MsgLen;

	va_start (va, format);
	MsgLen = _vsnprintf (msg, sizeof(msg), format, va);

	for (char* sp = msg; *sp != 0; sp++)
	{
		if (sp[0] == '\n' && sp[-1] != '\r')
		{
			memmove (&sp[1], sp, strlen(sp)+1);
			sp[0] = '\r';
		}
	}
	MsgLen = strlen(msg);

	if (gszLog == NULL)
	{
		LogSize = 20;
		gszLog = (char*)LocalAlloc (LHND, LogSize);
	}

	if (LogLen + MsgLen + 1 > LogSize)
	{
		LogSize += 20;
		gszLog = (char*)LocalReAlloc ()
	}
}

DWORD WINAPI DownloadSymbols (LPVOID)
{
	char szServerUrl[MAX_PATH];
	char szLocalStorage[MAX_PATH];

	GetDlgItemText (ghWnd, IDC_SYMSRV, szServerUrl, sizeof(szServerUrl)-1);
	GetDlgItemText (ghWnd, IDC_SYMPATH, szLocalStorage, sizeof(szLocalStorage)-1);

	for (int i=0; i<nFiles; i++)
	{
		char *szFile = szFiles[i];

		LogPrintf ("Loading '%s' ...\n", szFile);

		PVOID Image = MapExistingFile (szFile, MAP_IMAGE | MAP_READ, 0, NULL);
		if (Image != NULL)
		{
			if (!SymDownloadFromServerForImage (Image, szServerUrl, szLocalStorage))
			{
				LogPrintf ("FAILED (Could not load symbols for image)\r\n");
			}
			else
			{
				LogPrintf ("OK\r\n");
			}

			UnmapViewOfFile(Image);
		}
		else
		{
			LogPrintf ("FAILED (Could not open image)\r\n");
		}
	}

	SendMessage (ghWnd, WM_DOWNLOAD_COMPLETED, 0, 0);

	return 0;
}

INT_PTR CALLBACK DlgProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			char szSymbolPath[MAX_PATH] = "";
			char szSymbolServer[MAX_PATH] = "http://msdl.microsoft.com/download/symbols";

			ghWnd = hWnd;
			hLog = GetDlgItem (hWnd, IDC_LOG);

			LogPrintf ("Hello 1\n");
			LogPrintf ("Hello 2\n");
			LogPrintf ("Hello 3\n");

			if (GetEnvironmentVariable ("_NT_SYMBOL_PATH", szSymbolPath, sizeof(szSymbolPath)-1) == 0)
			{
				GetWindowsDirectory (szSymbolPath, sizeof(szSymbolPath)-1);
				strcat (szSymbolPath, "\\Symbols");
				
			}
			else
			{
				if (!_strnicmp(szSymbolPath, "srv*", 4))
				{
					PSTR symsrv = strchr (&szSymbolPath[4], '*');
					if (symsrv != NULL)
					{
						strcpy (szSymbolServer, symsrv + 1);
						
						size_t sympathlen = symsrv - &szSymbolPath[4];
						strncpy (szSymbolPath, &szSymbolPath[4], sympathlen);
						szSymbolPath[sympathlen] = 0;
					}
					else
					{
						strcpy (szSymbolPath, &szSymbolPath[4]);
					}
				}
			}
			SetDlgItemText (hWnd, IDC_SYMSRV, szSymbolServer);
			SetDlgItemText (hWnd, IDC_SYMPATH, szSymbolPath);
		}
		break;

	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)(wParam);
			int nDropFiles = DragQueryFile (hDrop, -1, 0, 0);

			for (int i=0; i<nDropFiles; i++)
			{
				char szFileName[MAX_PATH];
				DragQueryFile (hDrop, i, szFileName, sizeof(szFileName));

				szFiles[nFiles + i] = xstrdup (szFileName);

				SendDlgItemMessage (hWnd, IDC_FILELIST, LB_ADDSTRING, 0, (LPARAM)szFileName);
			}

			nFiles += nDropFiles;
			DragFinish (hDrop);
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog (hWnd, 0);
			break;

		case IDOK:
			if (nFiles == 0)
			{
				MessageBox (hWnd, "No files selected for download", "Symbol retriver", MB_USERICON);
				return 0;
			}

			SymSetPrintfCallout (LogPrintf);

			EnableWindow (GetDlgItem (hWnd, IDOK), FALSE);

			HANDLE hThread = CreateThread (0, 0, DownloadSymbols, 0, 0, 0);
			if (hThread == NULL)
			{
				MessageBox (hWnd, "Failed to create thread", "Symbol retriever", MB_ICONHAND);
				EnableWindow (GetDlgItem (hWnd, IDOK), TRUE);
			}
			else
			{
				CloseHandle (hThread);
			}
		}
		break;

	case WM_DOWNLOAD_COMPLETED:
		EnableWindow (GetDlgItem (hWnd, IDOK), TRUE);
		break;		
	}
	return 0;
}

int APIENTRY WinMain ( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
	return DialogBoxParam (GetModuleHandle (0), MAKEINTRESOURCE(IDD_DIALOG1), 0, DlgProc, 0);
}
