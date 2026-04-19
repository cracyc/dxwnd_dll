#define _CRT_SECURE_NO_WARNINGS

#define _MODULE "comdlg32"

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"

typedef BOOL (WINAPI *GetFileNameA_Type)(LPOPENFILENAMEA);
typedef BOOL (WINAPI *GetFileNameW_Type)(LPOPENFILENAMEW);
typedef BOOL (WINAPI *PrintDlgA_Type)(LPPRINTDLGA);
typedef BOOL (WINAPI *PrintDlgW_Type)(LPPRINTDLGW);
GetFileNameA_Type pGetOpenFileNameA = NULL;
GetFileNameA_Type pGetSaveFileNameA = NULL;
GetFileNameW_Type pGetOpenFileNameW = NULL;
GetFileNameW_Type pGetSaveFileNameW = NULL;
PrintDlgA_Type pPrintDlgA = NULL;
PrintDlgW_Type pPrintDlgW = NULL;
BOOL WINAPI extGetSaveFileNameA(LPOPENFILENAMEA);
BOOL WINAPI extGetOpenFileNameA(LPOPENFILENAMEA);
BOOL WINAPI extGetSaveFileNameW(LPOPENFILENAMEW);
BOOL WINAPI extGetOpenFileNameW(LPOPENFILENAMEW);
BOOL WINAPI extPrintDlgA(LPPRINTDLGA);
BOOL WINAPI extPrintDlgW(LPPRINTDLGW);

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "GetSaveFileNameA", NULL, (FARPROC *)&pGetSaveFileNameA, (FARPROC)extGetSaveFileNameA},
	{HOOK_IAT_CANDIDATE, 0, "GetOpenFileNameA", NULL, (FARPROC *)&pGetOpenFileNameA, (FARPROC)extGetOpenFileNameA},
	{HOOK_IAT_CANDIDATE, 0, "GetSaveFileNameW", NULL, (FARPROC *)&pGetSaveFileNameW, (FARPROC)extGetSaveFileNameW},
	{HOOK_IAT_CANDIDATE, 0, "GetOpenFileNameW", NULL, (FARPROC *)&pGetOpenFileNameW, (FARPROC)extGetOpenFileNameW},
	{HOOK_IAT_CANDIDATE, 0, "PrintDlgA", NULL, (FARPROC *)&pPrintDlgA, (FARPROC)extPrintDlgA},
	{HOOK_IAT_CANDIDATE, 0, "PrintDlgW", NULL, (FARPROC *)&pPrintDlgW, (FARPROC)extPrintDlgW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookComDlg32(HMODULE module)
{
	HookLibraryEx(module, Hooks, "comdlg32.dll");
}

FARPROC Remap_ComDlg32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

BOOL WINAPI extGetSaveFileNameA(LPOPENFILENAMEA lpofn)
{
	BOOL ret, FullScreen;
	ApiName("GetSaveFileNameA");
	FullScreen = dxw.IsFullScreen();
	OutTraceSYS("%s: FullScreen=%#x\n", ApiRef, FullScreen);
	dxw.ClearFullScreen();
	if(dxw.dwFlags15 & FIXFILEDIALOG) lpofn->Flags &= ~(OFN_EXPLORER | OFN_ENABLEHOOK);
	ret = (*pGetSaveFileNameA)(lpofn);
	dxw.SetFullScreen(FullScreen);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

static char *expandFilter(LPCSTR filter)
{
	int len = 0;
	char *p = (char *)filter;
	static char *buf = NULL;

	// step 1: calculate the size of the buffer
	while(*p){
		len += strlen(p) + 1;
		p += strlen(p) + 1;
		len += strlen(p) + 1;
		p += strlen(p) + 1;
	}

	// step 2: allocate/reallocate the buffer
	if(buf) free(buf);
	buf = (char *)malloc(len+1);

	// step 3: fill the dump buffer. 
	// Comma ',' separates caption and extension
	// Semicolon ';' separates the caption/extension entries
	*buf = 0;
	p = (char *)filter;
	while(*p){
		strcat(buf, p);
		strcat(buf,",");
		p += strlen(p) + 1;
		strcat(buf, p);
		strcat(buf,";");
		len += strlen(p) + 1;
		p += strlen(p) + 1;
	}
	return buf;
}

BOOL WINAPI extGetOpenFileNameA(LPOPENFILENAMEA lpofn)
{
	BOOL ret, FullScreen;
	ApiName("GetOpenFileNameA");
	FullScreen = dxw.IsFullScreen();
	OutTraceSYS("%s: FullScreen=%#x\n", ApiRef, FullScreen);
	if(IsDebugDW){
		OutTrace("> lStructSize=%d\n", lpofn->lStructSize);
		OutTrace("> hwndOwner=%#x\n", lpofn->hwndOwner);
		OutTrace("> hInstance=%#x\n", lpofn->hInstance);
		OutTrace("> lpstrFilter=%s\n", expandFilter(lpofn->lpstrFilter));
		OutTrace("> lpstrCustomFilter=%s\n", lpofn->lpstrCustomFilter);
		OutTrace("> nMaxCustFilter=%d\n", lpofn->nMaxCustFilter);
		OutTrace("> nFilterIndex=%d\n", lpofn->nFilterIndex);
		OutTrace("> lpstrFile=%s\n", lpofn->lpstrFile);
		OutTrace("> nMaxFile=%d\n", lpofn->nMaxFile);
		OutTrace("> lpstrFileTitle=%s\n", lpofn->lpstrFileTitle);
		OutTrace("> nMaxFileTitle=%d\n", lpofn->nMaxFileTitle);
		OutTrace("> lpstrInitialDir=%s\n", lpofn->lpstrInitialDir);
		OutTrace("> lpstrTitle=%s\n", lpofn->lpstrTitle);
		OutTrace("> Flags=%#x\n", lpofn->Flags);
		OutTrace("> nFileOffset=%d\n", lpofn->nFileOffset);
		OutTrace("> nFileExtension=%d\n", lpofn->nFileExtension);
		OutTrace("> lpstrDefExt=%s\n", lpofn->lpstrDefExt);
		OutTrace("> lCustData=%#x\n", lpofn->lCustData);
		OutTrace("> lpfnHook=%#x\n", lpofn->lpfnHook);
		OutTrace("> lpTemplateName=%s\n", lpofn->lpTemplateName);
	}
	dxw.ClearFullScreen();
	if(dxw.dwFlags15 & FIXFILEDIALOG) lpofn->Flags &= ~(OFN_EXPLORER | OFN_ENABLEHOOK);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		if(lpofn->lpstrInitialDir){
			DWORD mapping;
			lpofn->lpstrInitialDir = dxwTranslatePathA(lpofn->lpstrInitialDir, &mapping);
			if(mapping != DXW_NO_FAKE) OutTraceDW("%s: remapped InitialDir=\"%s\"\n", ApiRef, lpofn->lpstrInitialDir);
		}
	}
	ret = (*pGetOpenFileNameA)(lpofn);
	dxw.SetFullScreen(FullScreen);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extGetSaveFileNameW(LPOPENFILENAMEW lpofn)
{
	BOOL ret, FullScreen;
	ApiName("GetSaveFileNameW");
	FullScreen = dxw.IsFullScreen();
	OutTraceSYS("%s: FullScreen=%#x\n", ApiRef, FullScreen);
	if(IsDebugDW){
		OutTrace("> lStructSize=%d\n", lpofn->lStructSize);
		OutTrace("> hwndOwner=%#x\n", lpofn->hwndOwner);
		OutTrace("> hInstance=%#x\n", lpofn->hInstance);
		OutTrace("> lpstrFilter=%ls\n", lpofn->lpstrFilter);
		OutTrace("> lpstrCustomFilter=%ls\n", lpofn->lpstrCustomFilter);
		OutTrace("> nMaxCustFilter=%d\n", lpofn->nMaxCustFilter);
		OutTrace("> nFilterIndex=%d\n", lpofn->nFilterIndex);
		OutTrace("> lpstrFile=%ls\n", lpofn->lpstrFile);
		OutTrace("> nMaxFile=%d\n", lpofn->nMaxFile);
		OutTrace("> lpstrFileTitle=%ls\n", lpofn->lpstrFileTitle);
		OutTrace("> nMaxFileTitle=%d\n", lpofn->nMaxFileTitle);
		OutTrace("> lpstrInitialDir=%ls\n", lpofn->lpstrInitialDir);
		OutTrace("> lpstrTitle=%ls\n", lpofn->lpstrTitle);
		OutTrace("> Flags=%#x\n", lpofn->Flags);
		OutTrace("> nFileOffset=%d\n", lpofn->nFileOffset);
		OutTrace("> nFileExtension=%d\n", lpofn->nFileExtension);
		OutTrace("> lpstrDefExt=%ls\n", lpofn->lpstrDefExt);
		OutTrace("> lCustData=%#x\n", lpofn->lCustData);
		OutTrace("> lpfnHook=%#x\n", lpofn->lpfnHook);
		OutTrace("> lpTemplateName=%ls\n", lpofn->lpTemplateName);
	}
	dxw.ClearFullScreen();
	if(dxw.dwFlags15 & FIXFILEDIALOG) lpofn->Flags &= ~(OFN_EXPLORER | OFN_ENABLEHOOK);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		if(lpofn->lpstrInitialDir){
			DWORD mapping;
			lpofn->lpstrInitialDir = dxwTranslatePathW(lpofn->lpstrInitialDir, &mapping);
			if(mapping != DXW_NO_FAKE) OutTraceDW("%s: remapped InitialDir=\"%ls\"\n", ApiRef, lpofn->lpstrInitialDir);
		}
	}
	ret = (*pGetSaveFileNameW)(lpofn);
	dxw.SetFullScreen(FullScreen);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extGetOpenFileNameW(LPOPENFILENAMEW lpofn)
{
	BOOL ret, FullScreen;
	ApiName("GetOpenFileNameW");
	FullScreen = dxw.IsFullScreen();
	OutTraceSYS("%s: FullScreen=%#x\n", ApiRef, FullScreen);
	dxw.ClearFullScreen();
	if(dxw.dwFlags15 & FIXFILEDIALOG) lpofn->Flags &= ~(OFN_EXPLORER | OFN_ENABLEHOOK);
	ret = (*pGetOpenFileNameW)(lpofn);
	dxw.SetFullScreen(FullScreen);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

// v2.05.60: added wrapper for PrintDlgA to fix "Amber: Journey Beyond" exception

BOOL WINAPI extPrintDlgA(LPPRINTDLGA lppd)
{
	BOOL ret, FullScreen;
	ApiName("PrintDlgA");
	FullScreen = dxw.IsFullScreen();
	OutTraceSYS("%s: FullScreen=%#x\n", ApiRef, FullScreen);
	dxw.ClearFullScreen();
	__try { ret = (*pPrintDlgA)(lppd); } __except(EXCEPTION_EXECUTE_HANDLER) { ret = 0; }
	dxw.SetFullScreen(FullScreen);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extPrintDlgW(LPPRINTDLGW lppd)
{
	BOOL ret, FullScreen;
	ApiName("PrintDlgW");
	FullScreen = dxw.IsFullScreen();
	OutTraceSYS("%s: FullScreen=%#x\n", ApiRef, FullScreen);
	dxw.ClearFullScreen();
	__try { ret = (*pPrintDlgW)(lppd); } __except(EXCEPTION_EXECUTE_HANDLER) { ret = 0; }
	dxw.SetFullScreen(FullScreen);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}
