#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <winnt.h>

#include "dxwnd.h"
#include "dxwcore.hpp"

/*
*  ReactOS kernel
*  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
* PROJECT:         ReactOS user32.dll
* FILE:            win32ss/user/user32/misc/winhelp.c
* PURPOSE:         WinHelp
* PROGRAMMER:      Robert Dickenson(robd@reactos.org)
* UPDATE HISTORY:
*      23-08-2002  RDD  Created from wine sources
*/

/*
*  PROJECT:         DxWnd (C)GHO
*  Code was adapted from Wine and Winevdm implementations to fit the DxWnd project.
*/

/* WinHelp internal structure */
typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
} WINHELP, *LPWINHELP;

/* WinHelp internal structure */
typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
    WORD ofsPath;
} WINEHELP, *LPWINEHELP;

typedef struct {
    INT16  wStructSize;
    INT16  x;
    INT16  y;
    INT16  dx;
    INT16  dy;
    INT16  wMax;
    CHAR   rgchMember[2];
} HELPWININFO16, *PHELPWININFO16, *LPHELPWININFO16;
/* magic number for this message:
*  aide means help is French ;-)
*  SOS means ???
*/
#define WINHELP_MAGIC   0xA1DE505

static BOOL is_builtin_winhlp32_stub(ApiArg)
{
    WCHAR windir[MAX_PATH];
    WCHAR winhlp32[MAX_PATH];
    static BOOL is_stub = FALSE;
    static BOOL detected;
    DWORD ret;
    if (detected)
        return is_stub;
    detected = TRUE;
    GetSystemWindowsDirectoryW(windir, MAX_PATH);
    ret = SearchPathW(windir, L"winhlp32.exe", NULL, MAX_PATH, winhlp32, NULL);
    if (ret && ret < MAX_PATH)
    {
        DWORD size = GetFileVersionInfoSizeW(winhlp32, NULL);
        LPVOID vd = HeapAlloc(GetProcessHeap(), 0, size);
#ifndef FILE_VER_GET_NEUTRAL
#define FILE_VER_GET_NEUTRAL 0x02
#endif

        typedef BOOL (WINAPI*GetFileVersionInfoExW_t)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
        static GetFileVersionInfoExW_t pGetFileVersionInfoExW = NULL;
        if (!pGetFileVersionInfoExW)
        {
            pGetFileVersionInfoExW = (GetFileVersionInfoExW_t)GetProcAddress(GetModuleHandleA("version"), "GetFileVersionInfoExW");
			// v2.06.10.fx: if GetFileVersionInfoExW can't be found, the system is old and the built-in WinHlp.exe is not necessary
			if(!pGetFileVersionInfoExW) return FALSE;
		}

        if ((*pGetFileVersionInfoExW)(FILE_VER_GET_NEUTRAL, winhlp32, 0, size, vd))
        {
            WCHAR *internalname = NULL;
            UINT ulen;
            if (VerQueryValueW(vd, L"\\StringFileInfo\\040904B0\\InternalName", (LPVOID *)&internalname, &ulen))
            {
                if (!memcmp(internalname, L"WINHSTB", sizeof(L"WINHSTB")))
                {
                    is_stub = TRUE;
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, vd);
    }
	OutTraceGDI("%s: is_stub=%d\n", ApiRef, is_stub);
    return is_stub;
}
//winhe

/**********************************************************************
*		WinHelpA (USER32.@)
*/
BOOL WINAPI extWinHelp(ApiArg, HWND hWnd, LPCSTR lpHelpFile, UINT wCommand, ULONG_PTR dwData)
{
    COPYDATASTRUCT      cds;
    HWND                hDest;
    int                 size, dsize, nlen, plen;
    char                path[MAX_PATH];
    char*               pathend;
    WINEHELP*           lpwh;
    LRESULT             ret;
	char				cmdLine[512];

	OutTraceGDI("%s: hWnd=%#x lpszHelp=%s uCommand=%d dwData=%#x\n", ApiRef, hWnd, lpHelpFile, wCommand, dwData);

    hDest = FindWindowA("MS_WINHELP", NULL);
    if (!hDest)
    {
        if (wCommand == HELP_QUIT) return TRUE;

		if(is_builtin_winhlp32_stub(ApiRef))
			_snprintf_s(cmdLine, 256, 256, "%s\\winhlp32.exe -x", GetDxWndPath());
		else 
			strcpy_s(cmdLine, 256, "c:\\Windows\\WinHelp\\winhlp32.exe -x");

        if (WinExec(cmdLine, SW_SHOWNORMAL) < 32) {
			OutTraceGDI("%s: ERROR can't start \"winhlp32.exe -x\" err=%d\n", ApiRef, GetLastError());
            return FALSE;
        }
        //sleep 100ms~2000ms...
        for (int i = 0; i < 20; i++) {
            hDest = FindWindowA("MS_WINHELP", NULL);
            if (hDest) break;
            Sleep(100);
        }
        if (!(hDest = FindWindowA("MS_WINHELP", NULL))){
			OutTraceGDI("%s: Did not find a MS_WINHELP Window\n", ApiRef);
            return FALSE;
        }
    }
    MULTIKEYHELPA *multikey32 = NULL;
    HELPWININFOA info32;
    switch (wCommand)
    {
    case HELP_CONTEXT:
    case HELP_SETCONTENTS:
    case HELP_CONTENTS:
    case HELP_CONTEXTPOPUP:
    case HELP_FORCEFILE:
    case HELP_HELPONHELP:
    case HELP_FINDER:
    case HELP_QUIT:
        dsize = 0;
        break;
    case HELP_KEY:
    case HELP_PARTIALKEY:
    case HELP_COMMAND:
        dsize = dwData ? strlen((LPSTR)dwData) + 1 : 0;
        break;
    case HELP_MULTIKEY: 
	{
        //this conversion is not implemented.
		OutTraceGDI("%s: HELP_MULTIKEY\n", ApiRef);
    }
    break;
    case HELP_SETWINPOS:
    {
        //this conversion is not tested.
        LPHELPWININFO16 info16 = (LPHELPWININFO16)dwData;
		OutTraceGDI("%s: HELP_SETWINPOS pos=(%d,%d) size=(%dx%d)\n", 
			ApiRef, info16->x, info16->y, info16->dx, info16->dy);
        info32.dx = info16->dx;
        info32.dy = info16->dy;
        info32.x = info16->x;
        info32.y = info16->y;
        info32.wStructSize = info16->wStructSize - (sizeof(HELPWININFOA) - sizeof(HELPWININFO16));
        info32.rgchMember[0] = info16->rgchMember[0];
        info32.rgchMember[2] = info16->rgchMember[1];
        dwData = (ULONG_PTR)&info32;
        dsize = ((LPHELPWININFOA)dwData)->wStructSize;
    }
    break;
    default:
		OutTraceGDI("%s: Unknown help command %#x\n", ApiRef, wCommand);
        return FALSE;
    }

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpHelpFile = dxwTranslatePathA(lpHelpFile, NULL);
	}

#if 1
    if (lpHelpFile){
        GetModuleFileName(NULL, path, MAX_PATH);
        pathend = strrchr(path, '\\');
        if (pathend) {
            *pathend = '\0';
            plen = strlen(path) + 1;
        }
        else
            plen = 0;
        nlen = strlen(lpHelpFile) + 1;
    }
    else {
        plen = 0;
        nlen = 0;
    }
    size = sizeof(WINEHELP) + nlen + dsize + plen;
#else
	if (lpHelpFile) {
		nlen = plen = strlen(lpHelpFile) + 1;
	}
	else {
		nlen = plen = 0;
	}
    size = sizeof(WINEHELP) + nlen + dsize + plen;
#endif

    lpwh = (WINEHELP *)HeapAlloc(GetProcessHeap(), 0, size);
    if (!lpwh) return FALSE;

    cds.dwData = WINHELP_MAGIC;
    cds.cbData = size;
    cds.lpData = (void*)lpwh;

    lpwh->size = size;
    lpwh->command = wCommand;
    lpwh->data = dwData;
    if (nlen){
        strcpy(((char*)lpwh) + sizeof(WINEHELP), lpHelpFile);
        lpwh->ofsFilename = sizeof(WINEHELP);
    }
    else
        lpwh->ofsFilename = 0;
    if (dsize){
        memcpy(((char*)lpwh) + sizeof(WINEHELP) + nlen, (LPSTR)dwData, dsize);
        lpwh->ofsData = sizeof(WINEHELP) + nlen;
    }
    else
        lpwh->ofsData = 0;
    if (plen){
        strcpy(((char*)lpwh) + sizeof(WINEHELP) + nlen + dsize, path);
        lpwh->ofsPath = sizeof(WINEHELP) + nlen + dsize;
    }
    else
        lpwh->ofsPath = 0;
	OutTraceGDI("%s: Sending[%u]: cmd=%u data=%08x fn=%s\n",
		ApiRef,
        lpwh->size, lpwh->command, lpwh->data,
        lpwh->ofsFilename ? (LPSTR)lpwh + lpwh->ofsFilename : "");

    ret = SendMessageA(hDest, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);
    HeapFree(GetProcessHeap(), 0, lpwh);
    if (multikey32)
        HeapFree(GetProcessHeap(), 0, multikey32);
    return ret;
}

BOOL WINAPI extWinHelpA(HWND hWnd, LPCSTR lpHelpFile, UINT wCommand, ULONG_PTR dwData)
{ return extWinHelp("user32.WinHelpA", hWnd, lpHelpFile, wCommand, dwData); }
BOOL WINAPI extWinHelpW(HWND hWnd, LPCWSTR lpHelpFile, UINT wCommand, ULONG_PTR dwData)
{ 
	BOOL ret;
	char *file;
    int len = WideCharToMultiByte( CP_ACP, 0, lpHelpFile, -1, NULL, 0, NULL, NULL );
    if ((file = (char *)malloc(len))) {
        WideCharToMultiByte( CP_ACP, 0, lpHelpFile, -1, file, len, NULL, NULL );
        ret = extWinHelp("user32.WinHelpW", hWnd, (LPCSTR)file, wCommand, dwData );
        free(file);
    }
	return ret; 
}
