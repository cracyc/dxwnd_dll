#define _CRT_SECURE_NO_WARNINGS
#define _COMPONENT "dxwCore"

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhelper.h"
#include "resource.h"

// SAVELASTDC: optimized mode, if the last window handle and size didn't change, 
// it is not necessary to create a new VirtualHDC and attached bitmap, you can reuse 
// the last one, with the further advantage of avoiding multiple scaling to and from 
// the virtual image with quality loss (see "BattleZone 1998").
// Unfortunately something is wrong with Imperialism II, so better not activate ....
#ifdef DXWND_SAVELASTDC
static HWND gLasthWnd = NULL;
static RECT gLastRect = {0,0,0,0};
#endif

HDC dxwCore::AcquireEmulatedDC(HWND hwnd)
{
	ApiName("AcquireEmulatedDC");
	HDC wdc;
	RECT WinRect;
	HGDIOBJ PrevSelection;
	BOOL ret;
	//OutTrace("ACQUIRE hwnd=%#x vhdc=%#x pic=%#x\n", hwnd, VirtualHDC, VirtualPic);

	if(!(wdc=(*pGDIGetDC)(hwnd))){ // potential DC leakage
		OutTraceE("%s: GetDC ERROR hwnd=%#x err=%d at=%d\n", ApiRef, hwnd, GetLastError(), __LINE__);
		return NULL;
	}

	(*pGetClientRect)(hwnd, &WinRect);
	VirtualPicRect = WinRect;
	dxw.UnmapClient(&VirtualPicRect);

#ifdef DXWND_SAVELASTDC
	if((hwnd == gLasthWnd) && (WinRect.right == gLastRect.right) && (WinRect.bottom == gLastRect.bottom)) {
		if(!(*pGDIStretchBlt)(VirtualHDC, VirtualPicRect.left, VirtualPicRect.top, VirtualPicRect.right, VirtualPicRect.bottom, wdc, 0, 0, WinRect.right, WinRect.bottom, SRCCOPY))
			OutTraceE("%s: StretchBlt ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
		if(!(pGDIReleaseDC)(hwnd, wdc))
			OutTraceE("%s: ReleaseDC ERROR hdc=%#x err=%d at=%d\n", ApiRef, wdc, GetLastError(), __LINE__);
		return VirtualHDC;
	}
	gLasthWnd = hwnd;
	gLastRect = WinRect;
#endif

	//OutTrace("WINDOW picrect=(%d,%d)-(%d,%d)\n", 
	//	VirtualPicRect.left, VirtualPicRect.top, VirtualPicRect.right, VirtualPicRect.bottom);

	OutDebugDW("%s: hwnd=%#x Desktop=%#x WinRect=(%d,%d)(%d,%d) VirtRect=(%d,%d)(%d,%d)\n",
			ApiRef, hwnd, dxw.IsDesktop(hwnd), 
			WinRect.left, WinRect.top, WinRect.right, WinRect.bottom,
			VirtualPicRect.left, VirtualPicRect.top, VirtualPicRect.right, VirtualPicRect.bottom);

	// v2.03.91.fx3: loop required to eliminate resource leakage ("Yu No")
	if(VirtualHDC) {
		while(TRUE){
			if(!(*pDeleteObject)(VirtualHDC))
				OutTraceE("%s: DeleteObject ERROR hdc=%#x err=%d at=%d\n", ApiRef, VirtualHDC, GetLastError(), __LINE__);
			else break;
			(*pSleep)(1);
		}
	}
	if(VirtualPic) {
		while(TRUE){
			if(!(*pDeleteObject)(VirtualPic))
				OutTraceE("%s: DeleteObject ERROR pic=%#x err=%d at=%d\n", ApiRef, VirtualPic, GetLastError(), __LINE__);
			else break;
			(*pSleep)(1);
		}
	}

	VirtualHDC=CreateCompatibleDC(wdc);
	_if(!VirtualHDC) OutTraceE("%s: CreateCompatibleDC ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);

	VirtualPic=CreateCompatibleBitmap(wdc, dxw.GetScreenWidth(), dxw.GetScreenHeight());
	_if(!VirtualPic) OutTraceE("%s: CreateCompatibleBitmap ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);

	if(!(PrevSelection=(*pSelectObject)(VirtualHDC, VirtualPic)))
		OutTraceE("%s: SelectObject ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
	else {
		ret = (*pDeleteObject)(PrevSelection);
		_if(!ret) OutTraceE("%s: DeleteObject ERROR pic=%#x err=%d at=%d\n", ApiRef, PrevSelection, GetLastError(), __LINE__);
	}

	//OutTrace("RENEW_IMAGE hdc=%#x pic=%#x\n", VirtualHDC, VirtualPic);

	ret = (*pGDIStretchBlt)(VirtualHDC, VirtualPicRect.left, VirtualPicRect.top, VirtualPicRect.right, VirtualPicRect.bottom, wdc, 0, 0, WinRect.right, WinRect.bottom, SRCCOPY);
	_if(!ret) OutTraceE("%s: StretchBlt ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);

	ret = (*pGDIReleaseDC)(hwnd, wdc);
	_if(!ret) OutTraceE("%s: ReleaseDC ERROR hdc=%#x err=%d at=%d\n", ApiRef, wdc, GetLastError(), __LINE__);

	return VirtualHDC;
}

BOOL dxwCore::ReleaseEmulatedDC(HWND hwnd)
{
	HDC wdc;
	RECT WinRect;
	BOOL ret;
	ApiName("ReleaseEmulatedDC");
	//OutTrace("RELEASE hwnd=%#x\n", hwnd);

	(*pGetClientRect)(hwnd, &WinRect);

	OutDebugDW("%s: hwnd=%#x Desktop=%#x WinRect=(%d,%d)(%d,%d) VirtRect=(%d,%d)(%d,%d)\n",
		ApiRef, hwnd, dxw.IsDesktop(hwnd), 
		WinRect.left, WinRect.top, WinRect.right, WinRect.bottom,
		VirtualPicRect.left, VirtualPicRect.top, VirtualPicRect.right, VirtualPicRect.bottom);

	if(!(wdc=(*pGDIGetDC)(hwnd))) { // potential DC leakage
		OutTraceE("%s: GetDC ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
		return FALSE;
	}
	if(dxw.dwFlags15 & FORCEHALFTONE) {
		ret=SetStretchBltMode(wdc, HALFTONE);
		_if((!ret) || (ret==ERROR_INVALID_PARAMETER)) 
			OutTraceE("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
	}

	ret = (*pGDIStretchBlt)(wdc, 0, 0, WinRect.right, WinRect.bottom, VirtualHDC, 0, 0, VirtualPicRect.right, VirtualPicRect.bottom, SRCCOPY);
	_if(!ret) OutTraceE("%s: StretchBlt ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);

	ret = (*pGDIReleaseDC)(hwnd, wdc); // fixed DC leakage
	_if(!ret) OutTraceE("%s: ReleaseDC ERROR hwnd=%#x hdc=%#x err=%d at=%d\n", 
		ApiRef, hwnd, wdc, GetLastError(), __LINE__);
	
	return TRUE;
}

void dxwCore::ResetEmulatedDC()
{
	//OutTrace("RESET\n");
	VirtualHDC=NULL;
	VirtualPic=NULL;
}

BOOL dxwCore::IsVirtual(HDC hdc)
{
	return (hdc==VirtualHDC) && (GDIEmulationMode == GDIMODE_EMULATED);
}
