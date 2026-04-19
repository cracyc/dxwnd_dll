#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IVMRSurfaceAllocator"

#include <dxdiag.h>
#include <dsound.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include <strmif.h> // includes "dshow.h"
#include <control.h> // includes "dshow.h"
#include <dshow.h>
#include <strmif.h>
#include <comdef.h> // for BSTR type manipulation
#include <evr.h> // for video control

#include "dshow.wrap.hpp"

extern LPDIRECTDRAW lpPrimaryDD;
extern void HookDDSession(LPDIRECTDRAW *, int);
extern HWND hOverlayWnd;

#define DSHOWHACKS
//#define DSHOWHACK2

void dxwVMRSurfaceAllocator::Hook(IVMRSurfaceAllocator *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocator::QueryInterface(const IID &riid, void **obp)
{
	ApiName("QueryInterface");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x REFIID=%s(%s)\n", ApiRef, m_this, sRIID(riid), ExplainGUID((GUID *)&riid));
	if(res = CheckInterfaceDS(ApiRef, riid)) return res;
	res = m_this->QueryInterface(riid, obp);
	if(res) {
		OutTraceDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTraceDSHOW("%s: OK obp=%#x\n", ApiRef, *obp);
		HookInterfaceDS(ApiRef, m_this, riid, obp);
	}
	return res;
}

ULONG STDMETHODCALLTYPE dxwVMRSurfaceAllocator::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwVMRSurfaceAllocator::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwVMRSurfaceAllocator();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocator::AllocateSurface(DWORD_PTR dwUserID, VMRALLOCATIONINFO *lpAllocInfo, DWORD *lpdwActualBuffers, LPDIRECTDRAWSURFACE7 *lplpSurface)
{
	ApiName("AllocateSurface");
	HRESULT res;
#ifndef DXW_NOTRACES
	if(IsTraceDSHOW){
		OutTrace("%s: obj=%#x userid=%#x\n", ApiRef, m_this, dwUserID);
		OutTrace("> dwFlags=%#x\n", lpAllocInfo->dwFlags);
		OutTrace(">> lpHdr.biBitCount=%d\n", lpAllocInfo->lpHdr->biBitCount);
		OutTrace(">> lpHdr.biWidth=%d\n", lpAllocInfo->lpHdr->biWidth);
		OutTrace(">> lpHdr.biHeight=%d\n", lpAllocInfo->lpHdr->biHeight);
		OutTrace(">> lpHdr.biClrUsed=%d\n", lpAllocInfo->lpHdr->biClrUsed);
		OutTrace(">> lpHdr.biCompression=%d\n", lpAllocInfo->lpHdr->biCompression);
		if(lpAllocInfo->lpPixFmt){
			char sBuf[512+1];
			OutTrace("> lpPixFmt=%s\n", ExplainPixelFormat(lpAllocInfo->lpPixFmt, sBuf, 512));
		}
		else{
			OutTrace("> lpPixFmt=NULL\n");
		}
		OutTrace("> dwInterlaceFlags=%#x\n", lpAllocInfo->dwInterlaceFlags);
		OutTrace("> dwMaxBuffers=%d\n", lpAllocInfo->dwMaxBuffers);
		OutTrace("> dwMinBuffers=%d\n", lpAllocInfo->dwMinBuffers);
		OutTrace("> szAspectRatio=(%dx%d)\n", lpAllocInfo->szAspectRatio.cx, lpAllocInfo->szAspectRatio.cy);
		OutTrace("> szNativeSize=(%dx%d)\n", lpAllocInfo->szNativeSize.cx, lpAllocInfo->szNativeSize.cy);
	}
#endif // DXW_NOTRACES 
	
	BOOL isEmulatedOverlay = (dxw.dwFlags16 & (EMULATEOVERLAY | TRANSPARENTOVERLAY));

#ifdef DSHOWHACKS
	if (dxw.IsFullScreen() && dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)) {
		if(isEmulatedOverlay){
			dxw.SetScreenSize(lpAllocInfo->lpHdr->biWidth, lpAllocInfo->lpHdr->biHeight);
			dxw.VirtualPixelFormat.dwRGBBitCount = lpAllocInfo->lpHdr->biBitCount;
			// providing fixed size, needed by @#@ "Wild Rides WaterPark Factory"
			lpAllocInfo->lpHdr->biWidth = dxw.GetScreenWidth();
			lpAllocInfo->lpHdr->biHeight = dxw.GetScreenHeight();
			lpAllocInfo->lpHdr->biCompression = 0;
		}
		else {
			// providing fixed size, needed by @#@ "Wild Rides WaterPark Factory"
			lpAllocInfo->lpHdr->biWidth = dxw.iSizX;
			lpAllocInfo->lpHdr->biHeight = dxw.iSizY;
			// providing fixed color depth seems unnecessary ...
			lpAllocInfo->lpHdr->biBitCount = (WORD)dxw.ActualPixelFormat.dwRGBBitCount;
			lpAllocInfo->lpHdr->biCompression = 0;
		}
		OutTraceDSHOW("%s: fixed size=(%d x %d) compr=%#x bitc=%d\n", 
			ApiRef, lpAllocInfo->lpHdr->biWidth, lpAllocInfo->lpHdr->biHeight, lpAllocInfo->lpHdr->biCompression, lpAllocInfo->lpHdr->biBitCount);
	}
#endif
	res = m_this->AllocateSurface(dwUserID, lpAllocInfo, lpdwActualBuffers, lplpSurface);
	if((res == DD_OK) && *lplpSurface){
#ifdef DSHOWHACKS
		// v2.06.06 fix: forcing the hook with the ddraw resources
		if (dxw.IsFullScreen() && dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)) {
			LPDIRECTDRAWSURFACE7 lpSurface = *lplpSurface;
			LPDIRECTDRAW lpdd;
			//if(isEmulatedOverlay){
			//	//dxw.SethWnd(GetActiveWindow());
			//	//hOverlayWnd = dxw.GethWnd();
			//	hOverlayWnd = GetActiveWindow();
			//}
			lpSurface->GetDDInterface((LPVOID *)&lpdd);
			lpPrimaryDD = lpdd;
			OutTraceDSHOW("%s: GetDDInterface lpdd=%#x\n", ApiRef, lpdd);
			HookDDSession(&lpdd, 7);
			HookDDSurface((LPDIRECTDRAWSURFACE *)lplpSurface, 7, FALSE);
		}
#endif
		OutTraceDSHOW("%s: OK lpSurface=%x\n", ApiRef, *lplpSurface);
	}
	else {
		OutTraceDSHOW("%s: ERROR res=%x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocator::FreeSurface(DWORD_PTR dwID)
{
	ApiName("FreeSurface");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x id=%#x\n", ApiRef, m_this, dwID);

	res = m_this->FreeSurface(dwID);

#ifdef DSHOWHACKS
	// v2.06.06: the overlay emulation can create a fake desktop window. Destroy it here.
	if (dxw.IsFullScreen() && dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)) {
		HWND hwnd = FindWindow("Static", "DxWnd Overlay");
		if(hwnd) DestroyWindow(hwnd);
	}
#endif
	
	OutTraceDSHOW("%s: res=%x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocator::PrepareSurface(DWORD_PTR dwUserID, LPDIRECTDRAWSURFACE7 lpSurface, DWORD dwSurfaceFlags)
{
	ApiName("PrepareSurface");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x userid=%#x lpdds=%#x flags=%#x(%s)\n", ApiRef, m_this, dwUserID, lpSurface, 
		dwSurfaceFlags, dwSurfaceFlags & AM_GBF_NOTASYNCPOINT ? "AM_GBF_NOTASYNCPOINT" : "NONE" );

	res = m_this->PrepareSurface(dwUserID, lpSurface, dwSurfaceFlags);

	OutTrace("%s: res=%x\n", ApiRef, res);
	return res;
}
        
HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocator::AdviseNotify(IVMRSurfaceAllocatorNotify *lpIVMRSurfAllocNotify)
{
	ApiName("AdviseNotify");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x notify=%#x\n", ApiRef, m_this, lpIVMRSurfAllocNotify);

	res = m_this->AdviseNotify(lpIVMRSurfAllocNotify);

	OutTraceDSHOW("%s: res=%x\n", ApiRef, res);
	return res;
}
