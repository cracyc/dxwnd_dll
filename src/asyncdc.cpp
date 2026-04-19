#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhelper.h"
#include "resource.h"
#include "asyncdc.h"

CRITICAL_SECTION locker;

static DWORD WINAPI AsyncBlitter(LPVOID lpParameter)
{
	#define DXWREFRESHINTERVAL 16
	AsyncDC *lpADC = (AsyncDC *)lpParameter;
	while(TRUE){
		Sleep(DXWREFRESHINTERVAL);
		if(!dxw.IsFullScreen()) continue;
		HDC hdc = (*pGDIGetDC)(dxw.GethWnd());
		lpADC->GetDC(hdc);
		lpADC->RefreshDC();
		(*pGDIReleaseDC)(dxw.GethWnd(), hdc);
	}
    return 0;
}

AsyncDC::AsyncDC()
{
	CacheHDC = NULL;
	CacheHWnd = NULL;
	CacheHThread = NULL;
	lastW = lastH = 0;
	InitializeCriticalSection(&locker);
}

void AsyncDC::AcquireDC()
{
	ApiName("AsyncDC::AcquireDC");

	HDC wdc;
	HWND hwnd;
	RECT WinRect;
	HGDIOBJ PrevSelection;
	BOOL ret;

	//OutTrace("ACQUIRE hwnd=%#x vhdc=%#x pic=%#x\n", hwnd, CacheHDC, CachePic);
	hwnd = dxw.GethWnd();
	if(hwnd && CacheHWnd && (hwnd != CacheHWnd)){
		if((lastW != dxw.GetScreenWidth()) || (lastH != dxw.GetScreenHeight())){
			OutTrace("%s: renew cache\n", ApiRef);
			// to do ...
			lastW = dxw.GetScreenWidth();
			lastH = dxw.GetScreenHeight();
			if(CacheHDC) {
				while(TRUE){
					if(!(*pDeleteObject)(CacheHDC))
						OutTraceE("%s: DeleteObject ERROR hdc=%#x err=%d at=%d\n", ApiRef, CacheHDC, GetLastError(), __LINE__);
					else break;
					(*pSleep)(1);
				}
			}
			if(CachePic) {
				while(TRUE){
					if(!(*pDeleteObject)(CachePic))
						OutTraceE("%s: DeleteObject ERROR pic=%#x err=%d at=%d\n", ApiRef, CachePic, GetLastError(), __LINE__);
					else break;
					(*pSleep)(1);
				}
			}
			CacheHDC = 0;
			CacheHWnd = hwnd;
		}
	}

	if(CacheHDC) {
		OutDebugDW("%s: keeping cached hdc=%#x\n", ApiRef, CacheHDC);
		return;
	}

	if(!(wdc=(*pGDIGetDC)(hwnd))){ // potential DC leakage
		OutTraceE("%s: GetDC ERROR hwnd=%#x err=%d at=%d\n", ApiRef, hwnd, GetLastError(), __LINE__);
		CacheHDC = NULL;
		return;
	}

	(*pGetClientRect)(hwnd, &WinRect);
	CacheRect = WinRect;
	dxw.UnmapClient(&CacheRect);

	OutDebugDW("%s: hwnd=%#x Desktop=%#x WinRect=(%d,%d)(%d,%d) VirtRect=(%d,%d)(%d,%d)\n",
			ApiRef, hwnd, dxw.IsDesktop(hwnd), 
			WinRect.left, WinRect.top, WinRect.right, WinRect.bottom,
			CacheRect.left, CacheRect.top, CacheRect.right, CacheRect.bottom);

	CacheHDC=CreateCompatibleDC(wdc);
	_if(!CacheHDC) OutTraceE("%s: CreateCompatibleDC ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);

	CachePic=CreateCompatibleBitmap(wdc, dxw.GetScreenWidth(), dxw.GetScreenHeight());
	_if(!CachePic) OutTraceE("AcquireEmulatedDC: CreateCompatibleBitmap ERROR err=%d at=%d\n", GetLastError(), __LINE__);

	if(!(PrevSelection=(*pSelectObject)(CacheHDC, CachePic)))
		OutTraceE("%s: SelectObject ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
	else {
		ret = (*pDeleteObject)(PrevSelection);
		_if(!ret) OutTraceE("%s: DeleteObject ERROR pic=%#x err=%d at=%d\n", ApiRef, PrevSelection, GetLastError(), __LINE__);
	}

	OutTrace("%s: RENEW_IMAGE hdc=%#x pic=%#x cache size=(%dx%d) win size=(%dx%d)\n", 
		ApiRef, CacheHDC, CachePic, dxw.GetScreenWidth(), dxw.GetScreenHeight(), CacheRect.right, CacheRect.bottom);

	ret = (*pGDIStretchBlt)(CacheHDC, CacheRect.left, CacheRect.top, CacheRect.right, CacheRect.bottom, wdc, 0, 0, WinRect.right, WinRect.bottom, SRCCOPY);
	_if(!ret) OutTraceE("%s: StretchBlt ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);

	ret = (*pGDIReleaseDC)(hwnd, wdc);
	_if(!ret) OutTraceE("%s: ReleaseDC ERROR hdc=%#x err=%d at=%d\n", ApiRef, wdc, GetLastError(), __LINE__);

}

HDC AsyncDC::TestDC()
{
	return CacheHDC;
}

HDC AsyncDC::GetDC(HDC hdc)
{
	AcquireDC();
	if (!CacheHThread) CacheHThread = CreateThread(NULL, 0, AsyncBlitter, this, 0, NULL);
	if(dxw.IsFullScreen() && (WindowFromDC(hdc) == dxw.GethWnd())){
		EnterCriticalSection(&locker);
		mainWin = TRUE;
		return CacheHDC;
	}
	else {
		mainWin = FALSE;
		return hdc;
	}
}

BOOL AsyncDC::ReleaseDC()
{
	if(mainWin) LeaveCriticalSection(&locker);
	return TRUE;
}

BOOL AsyncDC::RefreshDC()
{
	ApiName("AsyncDC::RefreshDC");
	HDC wdc;
	RECT WinRect;
	BOOL ret;
	HWND hwnd = dxw.GethWnd();

	(*pGetClientRect)(hwnd, &WinRect);

	OutDebugDW("%s: hwnd=%#x Desktop=%#x WinRect=(%d,%d)(%d,%d) VirtRect=(%d,%d)(%d,%d)\n",
		ApiRef, hwnd, dxw.IsDesktop(hwnd), 
		WinRect.left, WinRect.top, WinRect.right, WinRect.bottom,
		CacheRect.left, CacheRect.top, CacheRect.right, CacheRect.bottom);

	GdiFlush();

	if(!(wdc=(*pGDIGetDC)(hwnd))) { // potential DC leakage
		OutTraceE("%s: GetDC ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
		LeaveCriticalSection(&locker);
		return FALSE;
	}
	if(dxw.dwFlags15 & FORCEHALFTONE) {
		ret=SetStretchBltMode(wdc, HALFTONE);
		_if((!ret) || (ret==ERROR_INVALID_PARAMETER)) 
			OutTraceE("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
	}

	ret = (*pGDIStretchBlt)(wdc, 0, 0, WinRect.right, WinRect.bottom, CacheHDC, 0, 0, CacheRect.right, CacheRect.bottom, SRCCOPY);
	_if(!ret) OutTraceE("%s: StretchBlt ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);

	(*pInvalidateRect)(hwnd, NULL, 0);

	ret = (*pGDIReleaseDC)(hwnd, wdc); // fixed DC leakage
	_if(!ret) OutTraceE("%s: ReleaseDC ERROR hwnd=%#x hdc=%#x err=%d at=%d\n",
		ApiRef, hwnd, wdc, GetLastError(), __LINE__);
	
	LeaveCriticalSection(&locker);
	return TRUE;
}
