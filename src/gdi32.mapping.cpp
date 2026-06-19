#define _MODULE "gdi32"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "hddraw.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "shareddc.hpp"
#include "asyncdc.h"

#include "stdio.h"

// ===== Viewport calls =====

BOOL WINAPI extSetViewportOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("SetViewportOrgEx");
	OutTraceGDI("%s: hdc=%#x pos=(%d,%d) lpp=%#x\n", ApiRef, hdc, X, Y, lpPoint);

	BOOL bToBeScaled = dxw.IsToRemap(hdc);
 	if (bToBeScaled){
		dxw.MapClient(&X, &Y); // fix v2.06.14
		OutTraceGDI("%s: fixed ViewPortOrg=(%d,%d)\n", ApiRef, X, Y);
	}

	ret=(*pSetViewportOrgEx)(hdc, X, Y, lpPoint);

	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: prev. ViewPortOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			if(bToBeScaled) {
				dxw.UnmapClient(lpPoint);
				OutTraceGDI("%s: fixed prev. ViewPortOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			}
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

BOOL WINAPI extOffsetViewportOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("OffsetViewportOrgEx");
	OutTraceGDI("%s: hdc=%#x pos=(%d,%d) lpp=%#x\n", ApiRef, hdc, X, Y, lpPoint);

	BOOL bToBeScaled = dxw.IsToRemap(hdc);
 	if (bToBeScaled){
		dxw.MapClient(&X, &Y); // fix v2.06.14
		OutTraceGDI("%s: fixed ViewPortOrg offset=(%d,%d)\n", ApiRef, X, Y);
	}

	ret=(*pOffsetViewportOrgEx)(hdc, X, Y, lpPoint);

	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: prev. ViewPortOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			if(bToBeScaled) {
				dxw.UnmapClient(lpPoint);
				OutTraceGDI("%s: fixed prev. ViewPortOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			}
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

// v2.06.13: since graphic is scaled in blit operations, viewport extension should remain unscaled
// hopefully fixed to 1 pixel
BOOL WINAPI extSetViewportExtEx(HDC hdc, int X, int Y, LPSIZE lpSize)
{
	BOOL ret;
	ApiName("SetViewportExtEx");
	OutTraceGDI("%s: hdc=%#x pos=(%d,%d) lps=%#x\n", ApiRef, hdc, X, Y, lpSize);

	ret=(*pSetViewportExtEx)(hdc, X, Y, lpSize);

	if(ret) {
		if(lpSize) {
			OutTraceGDI("%s: prev. ViewPortExt=(%d,%d)\n", ApiRef, lpSize->cx, lpSize->cy);
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

BOOL WINAPI extGetViewportOrgEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("GetViewportOrgEx");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	ret=(*pGetViewportOrgEx)(hdc, lpPoint);
	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: hdc=%#x ViewportOrg=(%d,%d)\n", ApiRef, hdc, lpPoint->x, lpPoint->y);
 			if (dxw.IsToRemap(hdc)){
				dxw.UnmapClient(lpPoint); // fix v2.06.14
				OutTraceGDI("%s: fixed ViewportOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			}
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

// v2.06.13: since graphic is scaled in blit operations, viewport extension should remain unscaled
// hopefully fixed to 1 pixel
BOOL WINAPI extGetViewportExtEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("GetViewportExtEx");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	ret=(*pGetViewportExtEx)(hdc, lpPoint);
	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: ViewportExt=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

// ===== Window calls =====

BOOL WINAPI extSetWindowOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("SetWindowOrgEx");
	OutTraceGDI("%s: hdc=%#x origin=(%d,%d)\n", ApiRef, hdc, X, Y);

 	if (dxw.Windowize){
		dxw.MapClient(&X, &Y);
		OutTraceGDI("%s: fixed origin=(%d,%d)\n", ApiRef, X, Y);
	}

	ret=(*pSetWindowOrgEx)(hdc, X, Y, lpPoint);
	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: previous WindowOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			if(dxw.Windowize) {
				dxw.UnmapWindow(lpPoint);
				OutTraceGDI("%s: fixed previous WindowOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			}
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

BOOL WINAPI extGetWindowOrgEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("GetWindowOrgEx");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	ret=(*pGetWindowOrgEx)(hdc, lpPoint);

 	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: hdc=%#x WindowOrg=(%d,%d)\n", ApiRef, hdc, lpPoint->x, lpPoint->y);
			if (dxw.Windowize){
				dxw.UnmapClient(lpPoint);
				OutTraceGDI("%s: fixed WindowOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			}
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

// v2.06.13: since window position is scaled in move operations, window extension should remain unscaled
BOOL WINAPI extSetWindowExtEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("SetWindowExtEx");
	OutTraceGDI("%s: hdc=%#x extent=(%d,%d)\n", ApiRef, hdc, X, Y);

	ret=(*pSetWindowExtEx)(hdc, X, Y, lpPoint);
	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: prev. extent=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

BOOL WINAPI extGetWindowExtEx(HDC hdc, LPSIZE lpsize)
{
	BOOL ret;
	ApiName("GetWindowExtEx");

	ret = (*pGetWindowExtEx)(hdc, lpsize);
	if(ret) {
		if(lpsize){
			OutTraceGDI("%s: hdc=%#x extent=(%d,%d)\n", ApiRef, hdc, lpsize->cx, lpsize->cy);
		}
	}
	else {
		OutErrorGDI("%s: hdc=#x ERROR err=%d\n", ApiRef, hdc, GetLastError());
	}
	return ret;
}

BOOL WINAPI extOffsetWindowOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("OffsetWindowOrgEx");
	OutTraceGDI("%s: hdc=%#x offset=(%d,%d)\n", ApiRef, hdc, X, Y);

 	if (dxw.Windowize){
		dxw.MapClient(&X, &Y);
		OutTraceGDI("%s: fixed offset=(%d,%d)\n", ApiRef, X, Y);
	}

	ret=(*pOffsetWindowOrgEx)(hdc, X, Y, lpPoint);
	if(ret) {
		if(lpPoint) {
			OutTraceGDI("%s: previous WindowOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			if(dxw.Windowize) {
				dxw.UnmapWindow(lpPoint);
				OutTraceGDI("%s: fixed previous WindowOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
			}
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}