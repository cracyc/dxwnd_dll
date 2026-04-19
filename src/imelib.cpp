#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _MODULE "imm32"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhook.h"
#include "syslibs.h"
#include "dxhelper.h"

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "ImmNotifyIME", NULL, (FARPROC *)&pImmNotifyIME, (FARPROC)extImmNotifyIME},
	{HOOK_IAT_CANDIDATE, 0, "ImmCreateContext", NULL, (FARPROC *)&pImmCreateContext, (FARPROC)extImmCreateContext},
	{HOOK_IAT_CANDIDATE, 0, "ImmDestroyContext", NULL, (FARPROC *)&pImmDestroyContext, (FARPROC)extImmDestroyContext},
	{HOOK_IAT_CANDIDATE, 0, "ImmSetOpenStatus", NULL, (FARPROC *)&pImmSetOpenStatus, (FARPROC)extImmSetOpenStatus},
	{HOOK_IAT_CANDIDATE, 0, "ImmAssociateContext", NULL, (FARPROC *)&pImmAssociateContext, (FARPROC)extImmAssociateContext},
	{HOOK_IAT_CANDIDATE, 0, "ImmSetCompositionWindow", NULL, (FARPROC *)&pImmSetCompositionWindow, (FARPROC)extImmSetCompositionWindow},
	{HOOK_IAT_CANDIDATE, 0, "ImmSetCompositionStringA", NULL, (FARPROC *)&pImmSetCompositionString, (FARPROC)extImmSetCompositionString},
	{HOOK_IAT_CANDIDATE, 0, "ImmGetOpenStatus", NULL, (FARPROC *)&pImmGetOpenStatus, (FARPROC)extImmGetOpenStatus},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookImeLib(HMODULE module)
{
	if(!(dxw.dwFlags2 & SUPPRESSIME)) return; 
	HookLibraryEx(module, Hooks, "IMM32.dll");
}

FARPROC Remap_ImeLib_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if(!(dxw.dwFlags2 & SUPPRESSIME)) return NULL; 
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

BOOL WINAPI extImmNotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
	ApiName("ImmNotifyIME");
	OutTraceDW("%s: hIMC=%#x dwAction=%#x dwIndex=%#x dwValue=%#x\n", ApiRef, hIMC, dwAction, dwIndex, dwValue);
	return(*pImmNotifyIME)(hIMC, dwAction, dwIndex, dwValue);
}

HIMC WINAPI extImmCreateContext(void)
{
	ApiName("ImmCreateContext");
	OutTraceDW("%s\n", ApiRef);
	return(*pImmCreateContext)();
}

BOOL WINAPI extImmDestroyContext(HIMC hIMC)
{
	ApiName("ImmDestroyContext");
	OutTraceDW("%s: hIMC=%#x\n", ApiRef, hIMC);
	return(*pImmDestroyContext)(hIMC);
}

BOOL WINAPI extImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
	ApiName("ImmSetOpenStatus");
	OutTraceDW("%s: hIMC=%#x fOpen=%#x\n", ApiRef, hIMC, fOpen);
	return(*pImmSetOpenStatus)(hIMC, fOpen);
}

HIMC WINAPI extImmAssociateContext(HWND hwnd, HIMC hIMC)
{
	ApiName("ImmAssociateContext");
	OutTraceDW("%s: hwnd=%#x hIMC=%#x\n", ApiRef, hwnd, hIMC);
	//return NULL;
	return(*pImmAssociateContext)(hwnd, hIMC);
}

BOOL WINAPI extImmSetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
	ApiName("ImmSetCompositionWindow");
	OutTraceDW("%s: hIMC=%#x lpCompForm=%#x\n", ApiRef, hIMC, lpCompForm);
	return(*pImmSetCompositionWindow)(hIMC, lpCompForm);
}

BOOL WINAPI extImmSetCompositionString(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen)
{
	ApiName("ImmSetCompositionString");
	OutTraceDW("%s: hIMC=%#x dwIndex=%#x lpComp=%#x dwCompLen=%#x dwRead=%#x dwReadLen=%#x\n", 
		ApiRef, hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
	return (*pImmSetCompositionString)(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
}

BOOL WINAPI extImmGetOpenStatus(HIMC hIMC)
{
	ApiName("ImmGetOpenStatus");
	OutTraceDW("%s: hIMC=%#x\n", ApiRef, hIMC);
	return(*pImmGetOpenStatus)(hIMC);
}
