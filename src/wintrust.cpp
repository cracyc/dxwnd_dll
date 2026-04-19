#define _MODULE "wintrust"

#include <dxdiag.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"

typedef LONG	(WINAPI *WinVerifyTrust_Type)(HWND, GUID *, LPVOID);
WinVerifyTrust_Type pWinVerifyTrust, pWinVerifyTrustEx;
extern LONG WINAPI extWinVerifyTrust(HWND, GUID *, LPVOID);
extern LONG WINAPI extWinVerifyTrustEx(HWND, GUID *, LPVOID);

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "WinVerifyTrust", NULL, (FARPROC *)&pWinVerifyTrust, (FARPROC)extWinVerifyTrust},
	{HOOK_IAT_CANDIDATE, 0, "WinVerifyTrustEx", NULL, (FARPROC *)&pWinVerifyTrustEx, (FARPROC)extWinVerifyTrustEx},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookTrust(HMODULE module)
{
	HookLibraryEx(module, Hooks, "wintrust.dll");
}

FARPROC Remap_trust_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;}

LONG WINAPI extWinVerifyTrust(HWND hWnd, GUID *pgActionID, LPVOID pWVTData)
{
	ApiName("WinVerifyTrust");
	LONG ret;
	OutTraceDW("%s: hwnd=%#x pgActionID=%#x pWVTData=%#x\n", ApiRef, hWnd, pgActionID, pWVTData);
	ret = (*pWinVerifyTrust)(hWnd, pgActionID, pWVTData);
	OutTraceDW("%s: ret=%lx\n", ApiRef, ret);
	return ret;
}

LONG WINAPI extWinVerifyTrustEx(HWND hWnd, GUID *pgActionID, LPVOID pWVTData)
{
	ApiName("WinVerifyTrustEx");
	LONG ret;
	OutTraceDW("%s: hwnd=%#x pgActionID=%#x pWVTData=%#x\n", ApiRef, hWnd, pgActionID, pWVTData);
	ret = (*pWinVerifyTrustEx)(hWnd, pgActionID, pWVTData);
	OutTraceDW("%s: ret=%lx\n", ApiRef, ret);
	return ret;
}