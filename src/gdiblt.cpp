#define _CRT_SECURE_NO_WARNINGS
#define INITGUID
#define _MODULE "dxwnd"
//#define FULLHEXDUMP

#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "dxhook.h"
#include "ddrawi.h"
#include "dxwcore.hpp"
#include "stdio.h" 
#include "hddraw.h"
#include "dxhelper.h"
#include "syslibs.h"

extern GetDC_Type pGetDCMethod();
extern ReleaseDC_Type pReleaseDCMethod();
extern HRESULT DDRawBlitToEmu(int, Blt_Type, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);

HRESULT GDISyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	HDC shdc, thdc;
	RECT client;
	HRESULT res;
	BOOL ret;
	HWND w;
	RECT dstrect;

	//res=(*pBlt)(lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
	res = DDRawBlitToEmu(dxversion, pBlt, lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
	if(res != DD_OK){
		OutTraceE("%s: Blt ERROR err=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		if(res == DDERR_UNSUPPORTED) {
			// do not desist, try blitting directly from source
			lpdds = s;
		}
		else {
			return res;
		}
	}

	w = dxw.GethWnd();
	res=(*pGetDCMethod())(lpdds, &shdc);
	if(res) {
		OutTraceE("%s: ddraw GetDC error lpdds=%#x res=%#x(%s)\n", ApiRef, lpdds, res, ExplainDDError(res));
		return DD_FALSE;
	}
	thdc=(*pGDIGetDC)(w);
	if(!thdc) {
		OutTraceE("%s: GDI GetDC error=%d\n", ApiRef, GetLastError());
		return DD_FALSE;
	}

	if(dxw.FilterId ==  DXW_FILTER_HALFTONE) {
		ret=SetStretchBltMode(thdc, HALFTONE);
		_if((!ret) || (ret==ERROR_INVALID_PARAMETER)) {
			OutTraceE("%s: GDI SetStretchBltMode error=%d\n", ApiRef, GetLastError());
		}
	}

	if(dxw.dwDFlags & CENTERTOWIN){
		int x, y;
		client = dxw.MapClientRect(NULL);
		x = (client.left + client.right - dxw.GetScreenWidth()) >> 1; // right-shift 1 bit means divide by 2!
		y = (client.top + client.bottom - dxw.GetScreenHeight()) >> 1;
		ret=(*pGDIBitBlt)(thdc, x, y, dxw.GetScreenWidth(), dxw.GetScreenHeight(), shdc, 0, 0, SRCCOPY);
		_if(!ret) OutTraceE("%s: BitBlt error=%d\n", ApiRef, GetLastError());
	}
	else{
		dstrect = dxw.MapClientRect(NULL);
		ret=(*pGDIStretchBlt)(thdc, 
			dstrect.left, dstrect.top, dstrect.right-dstrect.left, dstrect.bottom-dstrect.top, 
			shdc, 0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight(), SRCCOPY);
		_if(!ret) OutTraceE("%s: GDI StretchBlt error=%d\n", ApiRef, GetLastError());
	}
	dxw.ShowOverlay(thdc);
	res=(*pReleaseDCMethod())(lpdds, shdc);
	_if(res) OutTraceE("%s: ddraw ReleaseDC error lpdds=%#x res=%#x(%s)\n", ApiRef, lpdds, res, ExplainDDError(res));
	ret=(*pGDIReleaseDC)(w, thdc);
	_if(!ret) OutTraceE("%s: GDI ReleaseDC error=%d\n", ApiRef, GetLastError());
	return DD_OK;
}

static DWORD WINAPI GDIAsyncBlit()
{
	ApiName("GDIAsyncBlit");
	HDC shdc, thdc;
	RECT client;
	BOOL ret;
	HRESULT res;
	HWND w;
	RECT dstrect;

	w = dxw.GethWnd();
	res=(*pGetDCMethod())(dxw.lpBlitterSurface, &shdc);
	if(res) {
		OutTraceE("%s: ddraw GetDC error lpdds=%#x res=%#x(%s)\n", 
			ApiRef, dxw.lpBlitterSurface, res, ExplainDDError(res));
		return DD_FALSE;
	}
	thdc=(*pGDIGetDC)(w);
	if(!thdc) {
		OutTraceE("%s: GDI GetDC error=%d\n", ApiRef, GetLastError());
		return DD_FALSE;
	}

	if(dxw.FilterId ==  DXW_FILTER_HALFTONE) {
		ret=SetStretchBltMode(thdc, HALFTONE);
		_if((!ret) || (ret==ERROR_INVALID_PARAMETER)) {
			OutTraceE("%s: GDI SetStretchBltMode error=%d\n", ApiRef, GetLastError());
		}
	}

	if(dxw.dwDFlags & CENTERTOWIN){
		int x, y;
		client = dxw.MapClientRect(NULL);
		x = (client.left + client.right - dxw.GetScreenWidth()) >> 1; // right-shift 1 bit means divide by 2!
		y = (client.top + client.bottom - dxw.GetScreenHeight()) >> 1;
		ret=(*pGDIBitBlt)(thdc, x, y, dxw.GetScreenWidth(), dxw.GetScreenHeight(), shdc, 0, 0, SRCCOPY);
		_if(!ret) OutTraceE("%s: BitBlt error=%d\n", ApiRef, GetLastError());
	}
	else{
		dstrect = dxw.MapClientRect(NULL);
		ret=(*pGDIStretchBlt)(thdc, 
			dstrect.left, dstrect.top, dstrect.right-dstrect.left, dstrect.bottom-dstrect.top, 
			shdc, 0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight(), SRCCOPY);
		_if(!ret) OutTraceE("%s: GDI StretchBlt error=%d\n", ApiRef, GetLastError());
	}
	dxw.ShowOverlay(thdc);
	res=(*pReleaseDCMethod())(dxw.lpBlitterSurface, shdc);
	_if(res) OutTraceE("%s: ddraw ReleaseDC error lpdds=%#x res=%#x(%s)\n", 
		ApiRef, dxw.lpBlitterSurface, res, ExplainDDError(res));
	ret=(*pGDIReleaseDC)(w, thdc);
	_if(!ret) OutTraceE("%s: GDI ReleaseDC error=%d\n", ApiRef, GetLastError());
	return DD_OK;
}

static DWORD WINAPI GDIAsyncBlitter(LPVOID lpParameter)
{
	while(TRUE){
		extern void LimitFrameHz(DWORD);  
		LimitFrameHz(60);

		if (!dxw.lpBlitterSurface) continue;

		// Request ownership of the critical section.
		EnterCriticalSection(&dxw.CriticalSection); 
		GDIAsyncBlit();
		// Release ownership of the critical section.
		LeaveCriticalSection(&dxw.CriticalSection);
	}
}

HRESULT GDIAsyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	HRESULT res;

	if(!dxw.bAsybcBlitStarted) {
		if (!InitializeCriticalSectionAndSpinCount(&dxw.CriticalSection, 0x00000400) ) {
			OutTraceE("%s: InitializeCriticalSectionAndSpinCount ERROR err=%d\n", api, GetLastError());
			return -1;
		}
		CreateThread(NULL, 0, GDIAsyncBlitter, (LPVOID)lpdds, 0, NULL);
		dxw.bAsybcBlitStarted = TRUE;
		OutTrace("%s: GDIAsyncBlitter started\n", api, GetLastError());
	}

	dxw.lpBlitterSurface = lpdds;
	EnterCriticalSection(&dxw.CriticalSection); 
	//res=(*pBlt)(lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
	res = DDRawBlitToEmu(dxversion, pBlt, lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
	LeaveCriticalSection(&dxw.CriticalSection);
	if(res != DD_OK){
		OutTraceE("%s: Blt ERROR err=%#x(%s)\n", api, res, ExplainDDError(res));
	}

	return DD_OK;
}