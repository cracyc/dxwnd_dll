#define _CRT_SECURE_NO_WARNINGS

#define _MODULE "comctl32"

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"

//#define HOOKFLATSB

#ifdef HOOKFLATSB
typedef BOOL (WINAPI *InitializeFlatSB_Type)(HWND);
InitializeFlatSB_Type pInitializeFlatSB = NULL;
BOOL WINAPI extInitializeFlatSB(HWND);
typedef BOOL (WINAPI *UninitializeFlatSB_Type)(HWND);
InitializeFlatSB_Type pUninitializeFlatSB = NULL;
BOOL WINAPI extUninitializeFlatSB(HWND);
typedef BOOL (WINAPI *FlatSB_EnableScrollBar_Type)(HWND, int, UINT);
FlatSB_EnableScrollBar_Type pFlatSB_EnableScrollBar;
BOOL WINAPI extFlatSB_EnableScrollBar(HWND, int, UINT);
typedef BOOL (WINAPI *FlatSB_ShowScrollBar_Type)(HWND, int, BOOL);
FlatSB_ShowScrollBar_Type pFlatSB_ShowScrollBar;
BOOL WINAPI extFlatSB_ShowScrollBar(HWND, int, BOOL);
typedef BOOL (WINAPI *FlatSB_SetScrollProp_Type)(HWND, UINT, INT_PTR, BOOL);
FlatSB_SetScrollProp_Type pFlatSB_SetScrollProp;
BOOL WINAPI extFlatSB_SetScrollProp(HWND, UINT, INT_PTR, BOOL);
typedef BOOL (WINAPI *FlatSB_GetScrollProp_Type)(HWND, int, LPINT);
FlatSB_GetScrollProp_Type pFlatSB_GetScrollProp;
BOOL WINAPI extFlatSB_GetScrollProp(HWND, int, LPINT);
typedef int (WINAPI *FlatSB_SetScrollPos_Type)(HWND, int, int, BOOL);
FlatSB_SetScrollPos_Type pFlatSB_SetScrollPos;
int WINAPI extFlatSB_SetScrollPos(HWND, int, int, BOOL);
typedef int (WINAPI *FlatSB_GetScrollPos_Type)(HWND, int);
FlatSB_GetScrollPos_Type pFlatSB_GetScrollPos;
int WINAPI extFlatSB_GetScrollPos(HWND, int);
typedef BOOL (WINAPI *FlatSB_GetScrollRange_Type)(HWND, int, LPINT, LPINT);
FlatSB_GetScrollRange_Type pFlatSB_GetScrollRange;
BOOL WINAPI extFlatSB_GetScrollRange(HWND, int, LPINT, LPINT);
typedef int (WINAPI *FlatSB_SetScrollRange_Type)(HWND, int, int, int, BOOL);
FlatSB_SetScrollRange_Type pFlatSB_SetScrollRange;
int WINAPI extFlatSB_SetScrollRange(HWND, int, int, int, BOOL);
typedef int (WINAPI *FlatSB_SetScrollInfo_Type)(HWND, int, LPSCROLLINFO, BOOL);
FlatSB_SetScrollInfo_Type pFlatSB_SetScrollInfo;
int WINAPI extFlatSB_SetScrollInfo(HWND, int, LPSCROLLINFO, BOOL);
typedef BOOL (WINAPI *FlatSB_GetScrollInfo_Type)(HWND, int, LPSCROLLINFO);
FlatSB_GetScrollInfo_Type pFlatSB_GetScrollInfo;
BOOL WINAPI extFlatSB_GetScrollInfo(HWND, int, LPSCROLLINFO);
#endif // HOOKFLATSB
typedef INT_PTR (WINAPI *PropertySheetA_Type)(LPCPROPSHEETHEADERA);
PropertySheetA_Type pPropertySheetA;
INT_PTR WINAPI extPropertySheetA(LPCPROPSHEETHEADERA);
typedef INT_PTR (WINAPI *PropertySheetW_Type)(LPCPROPSHEETHEADERW);
PropertySheetW_Type pPropertySheetW;
INT_PTR WINAPI extPropertySheetW(LPCPROPSHEETHEADERW);

static HookEntryEx_Type Hooks[]={
#ifdef HOOKFLATSB
	{HOOK_IAT_CANDIDATE, 0, "InitializeFlatSB", NULL, (FARPROC *)&pInitializeFlatSB, (FARPROC)extInitializeFlatSB},
	{HOOK_IAT_CANDIDATE, 0, "UninitializeFlatSB", NULL, (FARPROC *)&pUninitializeFlatSB, (FARPROC)extUninitializeFlatSB},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_GetScrollProp", NULL, (FARPROC *)&pFlatSB_GetScrollProp, (FARPROC)extFlatSB_GetScrollProp},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_SetScrollProp", NULL, (FARPROC *)&pFlatSB_SetScrollProp, (FARPROC)extFlatSB_SetScrollProp},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_EnableScrollBar", NULL, (FARPROC *)&pFlatSB_EnableScrollBar, (FARPROC)extFlatSB_EnableScrollBar},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_ShowScrollBar", NULL, (FARPROC *)&pFlatSB_ShowScrollBar, (FARPROC)extFlatSB_ShowScrollBar},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_GetScrollRange", NULL, (FARPROC *)&pFlatSB_GetScrollRange, (FARPROC)extFlatSB_GetScrollRange},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_GetScrollInfo", NULL, (FARPROC *)&pFlatSB_GetScrollInfo, (FARPROC)extFlatSB_GetScrollInfo},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_GetScrollPos", NULL, (FARPROC *)&pFlatSB_GetScrollPos, (FARPROC)extFlatSB_GetScrollPos},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_SetScrollPos", NULL, (FARPROC *)&pFlatSB_SetScrollPos, (FARPROC)extFlatSB_SetScrollPos},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_SetScrollInfo", NULL, (FARPROC *)&pFlatSB_SetScrollInfo, (FARPROC)extFlatSB_SetScrollInfo},
	{HOOK_IAT_CANDIDATE, 0, "FlatSB_SetScrollRange", NULL, (FARPROC *)&pFlatSB_SetScrollRange, (FARPROC)extFlatSB_SetScrollRange},
#endif // HOOKFLATSB
	{HOOK_IAT_CANDIDATE, 0, "PropertySheetA", NULL, (FARPROC *)&pPropertySheetA, (FARPROC)extPropertySheetA},
	{HOOK_IAT_CANDIDATE, 0, "PropertySheetW", NULL, (FARPROC *)&pPropertySheetW, (FARPROC)extPropertySheetW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookComCtl32(HMODULE module)
{
	HookLibraryEx(module, Hooks, "comctl32.dll");
}

FARPROC Remap_ComCtl32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

// ---- wrappers ----

#ifdef HOOKFLATSB
BOOL WINAPI extInitializeFlatSB(HWND hwnd)
{
	BOOL ret;
	ApiName("InitializeFlatSB");

	OutTraceSYS("%s: hwnd=%#x\n", api, hwnd);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pInitializeFlatSB)(hwnd);
#ifndef DXW_NOTRACES
	if(!ret)OutTraceSYS("InitializeFlatSB: ret=%#x\n", ret);
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extUninitializeFlatSB(HWND hwnd)
{
	BOOL ret;
	ApiName("UninitializeFlatSB");

	OutTraceSYS("%s: hwnd=%#x\n", api, hwnd);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pUninitializeFlatSB)(hwnd);
#ifndef DXW_NOTRACES
	if(!ret)OutTraceSYS("%s: ret=%#x\n", api, ret);
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extFlatSB_EnableScrollBar(HWND hwnd, int p1, UINT p2)
{
	BOOL ret;
	ApiName("FlatSB_EnableScrollBar");

	OutTraceSYS("%s: hwnd=%#x\n", api, hwnd);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_EnableScrollBar)(hwnd, p1, p2);
#ifndef DXW_NOTRACES
	if(!ret)OutErrorSYS("%s: ERROR err=%d\n", api, GetLastError());
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extFlatSB_ShowScrollBar(HWND hwnd, int code, BOOL bShow)
{
	BOOL ret;
	ApiName("FlatSB_ShowScrollBar");

	OutTraceSYS("%s: hwnd=%#x type=%d show=%d\n", api, hwnd, code, bShow);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_ShowScrollBar)(hwnd, code, bShow);
#ifndef DXW_NOTRACES
	if(!ret)OutErrorSYS("%s: ERROR err=%d\n", api, GetLastError());
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extFlatSB_GetScrollProp(HWND hwnd, int propIndex, LPINT lpProp)
{
	BOOL ret;
	ApiName("FlatSB_GetScrollProp");
	char sBuf[80+1];

	OutTraceSYS("%s: hwnd=%#x index=%#x(%s)\n", api, hwnd, propIndex, ExplainWSBFlags(propIndex, sBuf, 80));
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_GetScrollProp)(hwnd, propIndex, lpProp);
#ifndef DXW_NOTRACES
	if(ret){
		OutTraceSYS("%s: prop=%#x\n", ApiRef, *lpProp);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extFlatSB_SetScrollProp(HWND hwnd, UINT propIndex, INT_PTR newValue, BOOL bRedraw)
{
	BOOL ret;
	ApiName("FlatSB_SetScrollProp");
	char sBuf[80+1];

	OutTraceSYS("%s: hwnd=%#x index=%#x(%s) val=%#x redraw=%#x\n", 
		ApiRef, hwnd, propIndex, ExplainWSBFlags(propIndex, sBuf, 80), newValue, bRedraw);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_SetScrollProp)(hwnd, propIndex, newValue, bRedraw);
#ifndef DXW_NOTRACES
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", api, GetLastError());
#endif // DXW_NOTRACES
	return ret;
}

int WINAPI extFlatSB_GetScrollPos(HWND hwnd, int code)
{
	int ret;
	ApiName("FlatSB_GetScrollPos");

	OutTraceSYS("%s: hwnd=%#x code=%#x(%s)\n", ApiRef, hwnd, code, code == SB_HORZ ? "HORZ" : "VERT");
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", ApiRef, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_GetScrollPos)(hwnd, code);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

int WINAPI extFlatSB_SetScrollPos(HWND hwnd, int code, int pos, BOOL fRedraw)
{
	int ret;
	ApiName("FlatSB_SetScrollPos");

	OutTraceSYS("%s: hwnd=%#x code=%#x(%s) pos=%#x redraw=%d\n", api, hwnd, code, code == SB_HORZ ? "HORZ" : "VERT", pos, fRedraw);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_SetScrollPos)(hwnd, code, pos, fRedraw);
	OutTraceSYS("%s: ret=%#x\n", api, ret);
	return ret;
}

BOOL WINAPI extFlatSB_GetScrollRange(HWND hwnd, int code, LPINT lpiMin, LPINT lpiMax)
{
	BOOL ret;
	ApiName("FlatSB_GetScrollRange");

	OutTraceSYS("%s: hwnd=%#x code=%#x\n", api, hwnd, code);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_GetScrollRange)(hwnd, code, lpiMin, lpiMax);
#ifndef DXW_NOTRACES
	if(ret){
		OutTraceSYS("%s: min=%d max=%d\n", api, lpiMin ? *lpiMin : 0, lpiMax ? *lpiMax : 0);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", api, GetLastError());
	}
#endif // DXW_NOTRACES
	return ret;
}

int WINAPI extFlatSB_SetScrollRange(HWND hwnd, int code, int min, int max, BOOL fRedraw)
{
	int ret;
	ApiName("FlatSB_SetScrollRange");

	OutTraceSYS("%s: hwnd=%#x code=%#x min=%d max=%d bRedraw=%d\n", api, hwnd, code, min, max, fRedraw);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_SetScrollRange)(hwnd, code, min, max, fRedraw);
#ifndef DXW_NOTRACES
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", api, GetLastError());
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extFlatSB_GetScrollInfo(HWND hwnd, int code, LPSCROLLINFO lpsi)
{
	int ret;
	ApiName("FlatSB_GetScrollInfo");

	OutTraceSYS("%s: hwnd=%#x code=%#x\n", api, hwnd, code);
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_GetScrollInfo)(hwnd, code, lpsi);
#ifndef DXW_NOTRACES
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", api, GetLastError());
	else {
		if(IsTraceSYS){
			OutTrace("> cbSize=%d\n", lpsi->cbSize);
			OutTrace("> fMask=%#x\n", lpsi->fMask);
			OutTrace("> min/max=%d/%d\n", lpsi->nMin, lpsi->nMax);
 			OutTrace("> nPage=%d\n", lpsi->nPage);
 			OutTrace("> nPos=%d\n", lpsi->nPos);
 			OutTrace("> nTrackPos=%d\n", lpsi->nTrackPos);
		}
	}
#endif // DXW_NOTRACES
	return ret;
}

int WINAPI extFlatSB_SetScrollInfo(HWND hwnd, int code, LPSCROLLINFO lpsi, BOOL fRedraw)
{
	int ret;
	ApiName("FlatSB_SetScrollInfo");

	OutTraceSYS("%s: hwnd=%#x code=%#x lpsi=%#x redraw=%d\n", api, hwnd, code, lpsi, fRedraw);
	if(IsTraceSYS){
		OutTrace("> cbSize=%d\n", lpsi->cbSize);
		OutTrace("> fMask=%#x\n", lpsi->fMask);
		OutTrace("> min/max=%d/%d\n", lpsi->nMin, lpsi->nMax);
 		OutTrace("> nPage=%d\n", lpsi->nPage);
 		OutTrace("> nPos=%d\n", lpsi->nPos);
 		OutTrace("> nTrackPos=%d\n", lpsi->nTrackPos);
	}
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hwnd)) {
		OutTraceDW("%s: hwnd=%#x->%#x\n", api, hwnd, dxw.GethWnd());
		hwnd = dxw.GethWnd();
	}
	ret = (*pFlatSB_SetScrollInfo)(hwnd, code, lpsi, fRedraw);
#ifndef DXW_NOTRACES
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", api, GetLastError());
#endif // DXW_NOTRACES
	return ret;
}
#endif // HOOKFLATSB

INT_PTR WINAPI extPropertySheetA(LPCPROPSHEETHEADERA pps)
{
	INT_PTR ret;
	DWORD dwFlags1;
	ApiName("PropertySheetA");
	OutTraceSYS("%s: pps=%#x\n", ApiRef, pps);
	dwFlags1 = dxw.dwFlags1;
	dxw.dwFlags1 &= ~UNNOTIFY;
	ret = (*pPropertySheetA)(pps);
	dxw.dwFlags1 = dwFlags1;
	return ret;
}

INT_PTR WINAPI extPropertySheetW(LPCPROPSHEETHEADERW pps)
{
	INT_PTR ret;
	DWORD dwFlags1;
	ApiName("PropertySheetW");
	OutTraceSYS("%s: pps=%#x\n", ApiRef, pps);
	dwFlags1 = dxw.dwFlags1;
	dxw.dwFlags1 &= ~UNNOTIFY;
	ret = (*pPropertySheetW)(pps);
	dxw.dwFlags1 = dwFlags1;
	return ret;
}
