#include "stdafx.h"

#include <windows.h>
#include <ddraw.h>
#include <d3dtypes.h>
#include <d3dumddi.h>
#include "stdio.h" 

extern void OutTrace(const char *, ...);

#if 0
static PIMAGE_NT_HEADERS getImageNtHeaders(HMODULE module)
{
	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	if (IMAGE_DOS_SIGNATURE != dosHeader->e_magic) return NULL;

	PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<char*>(dosHeader) + dosHeader->e_lfanew);
	if (IMAGE_NT_SIGNATURE != ntHeaders->Signature) return NULL;

	return ntHeaders;
}

static FARPROC *findProcAddressInIat(HMODULE module, const char* procName)
{
	if (!module || !procName) return NULL;

	PIMAGE_NT_HEADERS ntHeaders = getImageNtHeaders(module);
	if (!ntHeaders) return NULL;

	char* moduleBase = reinterpret_cast<char*>(module);
	PIMAGE_IMPORT_DESCRIPTOR importDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase +
		ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (PIMAGE_IMPORT_DESCRIPTOR desc = importDesc;
		0 != desc->Characteristics && 0xFFFF != desc->Name;
		++desc){
		PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(moduleBase + desc->FirstThunk);
		PIMAGE_THUNK_DATA origThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(moduleBase + desc->OriginalFirstThunk);
		while (0 != thunk->u1.AddressOfData && 0 != origThunk->u1.AddressOfData){
			PIMAGE_IMPORT_BY_NAME origImport = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
				moduleBase + origThunk->u1.AddressOfData);

			if (0 == strcmp((char *)origImport->Name, procName)){
				return reinterpret_cast<FARPROC*>(&thunk->u1.Function);
			}

			++thunk;
			++origThunk;
		}
	}
	return NULL;
}

BOOL QueryWDDM()
{
	HINSTANCE hinst;
	//static BOOL ret = 2;
	//if(ret != 2) return ret;
	BOOL ret = TRUE;

	hinst=LoadLibraryA("ddraw.dll");
	if(!hinst){
		OutTrace("QueryWDDM: LoadLibrary ddraw.dll ERROR err=%d @%d\n", GetLastError(), __LINE__);
		return FALSE;
	}
	//hookIatFunction(hinst, "GetProcAddress", DxGetProcAddress);
	if(!findProcAddressInIat(hinst, "OpenAdapter")) {
		OutTrace("QueryWDDM: missing OpenAdapter WDDM=FALSE\n");
		ret = FALSE;
	}
	if(!findProcAddressInIat(hinst, "GetPrivateDDITable")) {
		OutTrace("QueryWDDM: missing call=GetPrivateDDITable WDDM=FALSE\n");
		ret = FALSE;
	}
	FreeLibrary(hinst);
	OutTrace("QueryWDDM: WDDM support=%s\n", ret ? "Yes" : "No");
	return ret;
}
#else
extern BOOL IsWinXP();
#define UNASSIGNED 0x2
BOOL QueryWDDM()
{
	static BOOL hasWDDM = UNASSIGNED;
	if(hasWDDM != UNASSIGNED) return hasWDDM;
	hasWDDM = !IsWinXP();
	OutTrace("QueryWDDM: WDDM support=%s\n", hasWDDM ? "Yes" : "No");
	return hasWDDM;
}

#endif