#define  _CRT_SECURE_NO_WARNINGS

#define _MODULE "lz32"
//#define TRACEALL

#include <windows.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"

typedef INT (WINAPI *LZOpenFileA_Type)(LPSTR, LPOFSTRUCT, WORD);
LZOpenFileA_Type pLZOpenFileA;
INT WINAPI extLZOpenFileA(LPSTR, LPOFSTRUCT, WORD);
typedef INT (WINAPI *LZOpenFileW_Type)(LPWSTR, LPOFSTRUCT, WORD);
LZOpenFileW_Type pLZOpenFileW;
INT WINAPI extLZOpenFileW(LPWSTR, LPOFSTRUCT, WORD);

#ifdef TRACEALL
typedef INT (WINAPI *LZRead_Type)(INT, LPSTR, INT);
LZRead_Type pLZRead;
INT WINAPI extLZRead(INT, LPSTR, INT);
typedef LONG (WINAPI *LZSeek_Type)(INT, LONG, INT);
LZSeek_Type pLZSeek;
LONG WINAPI extLZSeek(INT, LONG, INT);
typedef VOID (WINAPI *LZClose_Type)(INT);
LZClose_Type pLZClose;
VOID WINAPI extLZClose(INT);
#endif

static HookEntryEx_Type HooksLz32[]={
	{HOOK_IAT_CANDIDATE, 0, "LZOpenFileA", (FARPROC)NULL, (FARPROC *)&pLZOpenFileA, (FARPROC)extLZOpenFileA},
	{HOOK_IAT_CANDIDATE, 0, "LZOpenFileW", (FARPROC)NULL, (FARPROC *)&pLZOpenFileW, (FARPROC)extLZOpenFileW},
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "LZRead", (FARPROC)NULL, (FARPROC *)&pLZRead, (FARPROC)extLZRead},
	{HOOK_IAT_CANDIDATE, 0, "LZSeek", (FARPROC)NULL, (FARPROC *)&pLZSeek, (FARPROC)extLZSeek},
	{HOOK_IAT_CANDIDATE, 0, "LZClose", (FARPROC)NULL, (FARPROC *)&pLZClose, (FARPROC)extLZClose},
#endif
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookLz32(HMODULE hModule)
{
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		HookLibraryEx(hModule, HooksLz32, "lz32.dll");
	}
}

FARPROC Remap_Lz32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		if (addr=RemapLibraryEx(proc, hModule, HooksLz32)) return addr;
	}

	return NULL;
}

INT WINAPI extLZOpenFileA(LPSTR lpFileName, LPOFSTRUCT lpReOpenBuf, WORD wStyle)
{
	INT res;
	ApiName("LZOpenFileA");
	OutTraceSYS("%s: path=\"%s\" style=%#x\n", ApiRef, lpFileName, wStyle);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		DWORD mapping;
		lpFileName = (LPSTR)dxwTranslatePathA(lpFileName, &mapping);
		if((mapping == DXW_FAKE_CD) && (wStyle & (OF_CREATE | OF_DELETE | OF_READWRITE | OF_WRITE))){
			OutTraceSYS("%s: returning LZERROR_WRITE for write access on fake CD device res=%#x\n", ApiRef, LZERROR_WRITE);
			return LZERROR_WRITE;
		}
	}

	res = (*pLZOpenFileA)(lpFileName, lpReOpenBuf, wStyle);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

INT WINAPI extLZOpenFileW(LPWSTR lpFileName, LPOFSTRUCT lpReOpenBuf, WORD wStyle)
{
	INT res;
	ApiName("LZOpenFileW");
	OutTraceSYS("%s: path=\"%ls\" style=%#x\n", ApiRef, lpFileName, wStyle);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		DWORD mapping;
		lpFileName = (LPWSTR)dxwTranslatePathW(lpFileName, &mapping);
		if((mapping == DXW_FAKE_CD) && (wStyle & (OF_CREATE | OF_DELETE | OF_READWRITE | OF_WRITE))){
			OutTraceSYS("%s: returning LZERROR_WRITE for write access on fake CD device res=%#x\n", ApiRef, LZERROR_WRITE);
			return LZERROR_WRITE;
		}
	}

	res = (*pLZOpenFileW)(lpFileName, lpReOpenBuf, wStyle);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

#ifdef TRACEALL
INT WINAPI extLZRead(INT h, LPSTR s, INT l)
{
	INT ret;
	ApiName("LZRead");
	OutTraceSYS("%s: h=%#x len=%d\n", ApiRef, h, l);
	ret = (*pLZRead)(h, s, l);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

LONG WINAPI extLZSeek(INT h, LONG seek, INT w)
{
	LONG ret;
	ApiName("LZSeek");
	OutTraceSYS("%s: h=%#x seek=%#x where=%d\n", ApiRef, h, seek, w);
	ret = (*pLZSeek)(h, seek, w);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

VOID WINAPI extLZClose(INT h)
{
	ApiName("LZClose");
	OutTraceSYS("%s: h=%#x\n", ApiRef, h);
	(*pLZClose)(h);
}
#endif
