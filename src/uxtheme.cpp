#define _CRT_SECURE_NO_WARNINGS

#define _MODULE "uxtheme"

//#define TRACEALL
#ifdef TRACEALL
#endif

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "uxtheme.h"

HRESULT WINAPI extGetThemeRect(HTHEME, int, int, int, LPRECT);
typedef HRESULT (WINAPI *GetThemeRect_Type)(HTHEME, int, int, int, LPRECT);
GetThemeRect_Type pGetThemeRect;

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "GetThemeRect", (FARPROC)NULL, (FARPROC *)&pGetThemeRect, (FARPROC)extGetThemeRect},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
}; 

static char *libname = "uxtheme.dll";

void HookUXTheme(HMODULE module)
{
	HookLibraryEx(module, Hooks, libname);
}

FARPROC Remap_UXTheme_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if(addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

HRESULT WINAPI extGetThemeRect(HTHEME hTheme, int iPartId, int iStateId, int iPropId, LPRECT pRect)
{
	HRESULT ret;
	ApiName("GetThemeRect");
	OutTraceSYS("%s: htheme=%#x PartId=%d StateId=%d PropId=%d\n", ApiRef, hTheme, iPartId, iStateId, iPropId); 
	ret = (*pGetThemeRect)(hTheme, iPartId, iStateId, iPropId, pRect); 
	if(ret){
		OutTraceE("%s: ERROR err=%d\n", ApiRef, GetLastError);
	}
	else {
		OutTraceSYS("%s: rect=(%d,%d)-(%d,%d)\n", ApiRef, pRect->left, pRect->top, pRect->right, pRect->bottom);
	}
	return ret;
}