#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "stdio.h" 
#include "hddraw.h"
#include "dxhelper.h"
#include "syslibs.h"

extern HWND hOverlayWnd;
extern GetSurfaceDesc_Type pGetSurfaceDesc1;
extern GetSurfaceDesc_Type pGetSurfaceDesc2;
extern GetSurfaceDesc_Type pGetSurfaceDesc3;
extern GetSurfaceDesc2_Type pGetSurfaceDesc4;
extern GetSurfaceDesc2_Type pGetSurfaceDesc7;
extern CreateSurface1_Type pCreateSurface1;
extern CreateSurface1_Type pCreateSurface2;
extern CreateSurface1_Type pCreateSurface3;
extern CreateSurface2_Type pCreateSurface4;
extern CreateSurface2_Type pCreateSurface7;
extern Lock_Type pLock1;
extern Lock_Type pLock2;
extern Lock_Type pLock3;
extern Lock_Type pLock4;
extern Lock_Type pLock7;
extern Unlock1_Type pUnlock1;
extern Unlock1_Type pUnlock2;
extern Unlock1_Type pUnlock3;
extern Unlock4_Type pUnlock4;
extern Unlock4_Type pUnlock7;
extern GetDC_Type pGetDC1;
extern GetDC_Type pGetDC2;
extern GetDC_Type pGetDC3;
extern GetDC_Type pGetDC4;
extern GetDC_Type pGetDC7;
extern ReleaseDC_Type pReleaseDC1;
extern ReleaseDC_Type pReleaseDC2;
extern ReleaseDC_Type pReleaseDC3;
extern ReleaseDC_Type pReleaseDC4;
extern ReleaseDC_Type pReleaseDC7;
extern ReleaseS_Type pReleaseS1;
extern ReleaseS_Type pReleaseS2;
extern ReleaseS_Type pReleaseS3;
extern ReleaseS_Type pReleaseS4;
extern ReleaseS_Type pReleaseS7;
extern void FixPixelFormat(int, LPDDPIXELFORMAT);
extern LPDIRECTDRAW lpPrimaryDD;
extern LPVOID dxwConvertFourCC(LPDDSURFACEDESC2, int);
extern RECT dwOverlayArea;

HRESULT FlipToOverlay(ApiArg, int dxversion, LPDIRECTDRAWSURFACE lpdds, LPRECT lpSrcRect, LPRECT lpDestRect)
{
	HRESULT res;

	// initialize variables
	DWORD srcColor = dxw.OverlayPixelFormat.dwRGBBitCount;
	DWORD dstColor = dxw.ActualPixelFormat.dwRGBBitCount;

	GetSurfaceDesc_Type pGetSurfaceDesc;
	CreateSurface2_Type pCreateSurface;
	GetDC_Type pGetDC;
	Lock_Type pLock;
	Unlock1_Type pUnlock;
	ReleaseDC_Type pReleaseDC;
	ReleaseS_Type pReleaseS;
	switch(dxversion){
		case 1: 
			pGetSurfaceDesc = pGetSurfaceDesc1; 
			pCreateSurface = (CreateSurface2_Type)pCreateSurface1; 
			pLock = pLock1;
			pUnlock = pUnlock1;
			pGetDC = pGetDC1;
			pReleaseDC = pReleaseDC1; 
			pReleaseS = pReleaseS1;
			break;
		case 2: 
			pGetSurfaceDesc = pGetSurfaceDesc2; 
			pCreateSurface = (CreateSurface2_Type)pCreateSurface2; 
			pLock = pLock2;
			pUnlock = pUnlock2;
			pGetDC = pGetDC2;
			pReleaseDC = pReleaseDC2; 
			pReleaseS = pReleaseS2;
			break;
		case 3: 
			pGetSurfaceDesc = pGetSurfaceDesc3; 
			pCreateSurface = (CreateSurface2_Type)pCreateSurface3; 
			pLock = pLock3;
			pUnlock = pUnlock3;
			pGetDC = pGetDC3;
			pReleaseDC = pReleaseDC3; 
			pReleaseS = pReleaseS3;
			break;
		case 4: 
			pGetSurfaceDesc = (GetSurfaceDesc_Type)pGetSurfaceDesc4; 
			pCreateSurface = (CreateSurface2_Type)pCreateSurface4; 
			pLock = pLock4;
			pUnlock = (Unlock1_Type)pUnlock4;
			pGetDC = pGetDC4;
			pReleaseDC = pReleaseDC4; 
			pReleaseS = pReleaseS4;
			break;
		case 7: 
			pGetSurfaceDesc = (GetSurfaceDesc_Type)pGetSurfaceDesc7; 
			pCreateSurface = (CreateSurface2_Type)pCreateSurface7; 
			pLock = pLock7;
			pUnlock = (Unlock1_Type)pUnlock7;
			pGetDC = pGetDC7;
			pReleaseDC = pReleaseDC7; 
			pReleaseS = pReleaseS7;
			break;
		default:
			MessageBox(0, "invalid dxversion", "DxWnd", 0);
			break;
	}
	if((dxw.dwDFlags2 & DUMPBLITSRC) && dxw.bCustomKeyToggle) SurfaceDump(lpdds, dxversion);

	// can't blit stretching and changing color depth too. If colors depths don't match
	// it is necessary to create a temporary surface with the same size but the target 
	// color depth.
	LPDIRECTDRAWSURFACE lpddsTmp;
	DDSURFACEDESC2 ddsd;
	DDSURFACEDESC2 ddsdtmp;
	memset(&ddsdtmp, 0, sizeof(DDSURFACEDESC2));
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = dxversion >= 4 ? sizeof(DDSURFACEDESC2) : sizeof(DDSURFACEDESC);
	res = (*pGetSurfaceDesc)(lpdds, (LPDDSURFACEDESC)&ddsd);
	if(res) {
		OutTraceE("%s: GetSurfaceDesc ERROR: lpdds=%#x err=%#x(%s) @%d\n", ApiRef, lpdds, res, ExplainDDError(res), __LINE__);
		return res;
	}
	DWORD dwHeight = ddsd.dwHeight;
	DWORD dwWidth = ddsd.dwWidth;
	//OutTrace("STEP: wxh=%dx%d pitch=%d\n", dwWidth, dwHeight, ddsd.lPitch);
	memcpy(&ddsdtmp, &ddsd, sizeof(DDSURFACEDESC2));
	ddsdtmp.ddpfPixelFormat.dwFourCC = 0;
	ddsdtmp.dwBackBufferCount = 0;
	FixPixelFormat(dxw.ActualPixelFormat.dwRGBBitCount, &ddsdtmp.ddpfPixelFormat);
	// v2.06.06: fixed flags, needed to fix IVMRSurfaceAllocator::AllocateSurface wrapper
	ddsdtmp.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsdtmp.dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	res=(*pCreateSurface)(lpPrimaryDD, &ddsdtmp, (LPDIRECTDRAWSURFACE *)&lpddsTmp, NULL);
	if(res) {
		char buf[81];
		OutTraceE(">>%s: CreateSurface ERROR: err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
		OutTraceE(">>%s: dwSize=%d size=(%d x %d)\n", ApiRef, ddsdtmp.dwSize, ddsdtmp.dwWidth, ddsdtmp.dwHeight);
		OutTraceE(">>%s: lpPrimaryDD=%#x pixelFormat=%s\n", ApiRef, lpPrimaryDD, ExplainPixelFormat(&(ddsdtmp.ddpfPixelFormat), buf, 80));
		return res;
	}
	// now copy or convert the surface pixels
	res=(*pLock)(lpdds, NULL, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_READONLY, 0);
	if(res) {
		OutTraceE("%s: GetSurfaceDesc ERROR: lpdds=%#x err=%#x(%s) @%d\n", ApiRef, lpdds, res, ExplainDDError(res), __LINE__);
		return res;
	}

	ddsd.ddpfPixelFormat = dxw.OverlayPixelFormat;
	//OutTrace("STEP: fourcc=%#x dwRGBBitCount=%d lpitch=%d rgbcount=%d\n", 
		 //ddsd.ddpfPixelFormat.dwFourCC, 
		 //ddsd.ddpfPixelFormat.dwRGBBitCount,
		 //ddsd.lPitch,
		 //dxw.ActualPixelFormat.dwRGBBitCount);
	LPVOID pixels = dxwConvertFourCC(&ddsd, dxw.ActualPixelFormat.dwRGBBitCount);
	res=(*pUnlock)(lpdds, NULL);
	_if(res) OutTraceE("Unlock ERROR: err=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	if(!pixels){
		OutTraceE("%s: ConvertFourCC ERROR\n", ApiRef);
		return DDERR_UNSUPPORTED;
	}
	if( dxversion >= 4){
		ddsdtmp.dwSize = sizeof(DDSURFACEDESC2);
		res=(*pLock)(lpddsTmp, 0, (LPDDSURFACEDESC)&ddsdtmp, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY, 0);
	}
	else {
		ddsdtmp.dwSize = sizeof(DDSURFACEDESC);
		res=(*pLock)(lpddsTmp, 0, (LPDDSURFACEDESC)&ddsdtmp, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY, 0);
	}
	if(res) {
		free(pixels);
		OutTraceE("%s: Lock ERROR: err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
		return res;
	}
	LPBYTE ps, pd;
	ps = (LPBYTE)pixels;
	pd = (LPBYTE)ddsdtmp.lpSurface;
	//OutTrace("STEP: ps=%#x pd=%#x dwHeight=%d pitch=%d\n", ps, pd, dwHeight, ddsdtmp.lPitch);
	for(unsigned int y=0; y<dwHeight; y++){
		memcpy(pd, ps, ddsdtmp.lPitch);
		ps += ddsdtmp.lPitch;
		pd += ddsdtmp.lPitch;
	}
	free(pixels);
	res=(*pUnlock)(lpddsTmp, NULL);
	_if(res) OutTraceE("Unlock ERROR: err=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	if((dxw.dwDFlags & DUMPSURFACES) && dxw.bCustomKeyToggle) SurfaceDump(lpddsTmp, dxversion);

	// if not available yet, create a fake-overlay window
	if(hOverlayWnd == NULL){
		HINSTANCE hinst=NULL;

		HWND hParent;
		hParent = dxw.GethWnd();
		if(dxw.dwFlags16 & TRANSPARENTOVERLAY){
			hOverlayWnd=(*pCreateWindowExA)(WS_EX_LAYERED, "Static", "DxWnd Overlay", WS_POPUP, dxw.iPosX, dxw.iPosY, dxw.iSizX, dxw.iSizY, hParent, NULL, hinst, NULL);
			if(!hOverlayWnd){
				OutTraceE("%s: CreateWindowEx ERROR: err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
				return DDERR_UNSUPPORTED;
			}
			SetLayeredWindowAttributes(hOverlayWnd, RGB(0,0,0), 0, LWA_COLORKEY);
		}
		else {
			hOverlayWnd=(*pCreateWindowExA)(0, "Static", "DxWnd Overlay", WS_POPUP, dxw.iPosX, dxw.iPosY, dxw.iSizX, dxw.iSizY, hParent, NULL, hinst, NULL);
			if(!hOverlayWnd){
				OutTraceE("%s: CreateWindowEx ERROR: err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
				return DDERR_UNSUPPORTED;
			}
		}
		// this SetWindowPos HWND_TOPMOST would place the overlay on top of some dialogs,
		// commenting it the overlay stays below.
		//(*pSetWindowPos)(hOverlayWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		(*pShowWindow)(hOverlayWnd, SW_SHOW);
		OutTraceDW("%s: created overlay emulation window: hwnd=%#x\n", ApiRef, hOverlayWnd);
	}

	// now stretchblt to the overlay window
	HDC shdc, thdc;
	BOOL ret;
	res=(*pGetDC)(lpddsTmp, &shdc);
	if(res) {
		OutTraceE("%s: ddraw GetDC ERROR lpdds=%#x res=%#x(%s)\n", ApiRef, lpddsTmp, res, ExplainDDError(res));
		return DD_FALSE;
	}
	thdc=(*pGDIGetDC)(hOverlayWnd);
	if(!thdc) {
		OutTraceE("%s: GDI GetDC ERROR err=%d\n", ApiRef, GetLastError());
		return DD_FALSE;
	}

	if(dxw.dwFlags15 & FORCEHALFTONE) {
		ret=SetStretchBltMode(thdc, HALFTONE);
		_if((!ret) || (ret==ERROR_INVALID_PARAMETER)) 
			OutTraceE("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
	}

	if(dxw.dwFlags16 & TRANSPARENTOVERLAY){
		ret = (*pGDIBitBlt)(thdc, 0, 0, dxw.iSizX, dxw.iSizY, NULL, 0, 0, BLACKNESS);
		_if(!ret) OutTraceE("%s: BitBlt ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
	}
	RECT s, d;
	if(lpDestRect){
		RECT destRect = *lpDestRect;;
		d = dxw.MapClientRect(&destRect);
	}
	else {
		memcpy(&d, &dwOverlayArea, sizeof(RECT));
		d = dxw.MapClientRect(&d);
	}
	if(lpSrcRect){
		s = *lpSrcRect;
	}
	else {
		s.left = d.top = 0;
		s.right = dwWidth;
		s.bottom = dwHeight;
	}
	ret=(*pGDIStretchBlt)(
		thdc, d.left, d.top, d.right-d.left, d.bottom-d.top, 
		shdc, s.left, s.top, s.right-s.left, s.bottom-s.top, 
		SRCCOPY);
	_if(!ret) OutTraceE("%s: StretchBlt ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
	// clean resources
	res=(*pReleaseDC)(lpddsTmp, shdc);
	_if(res) OutTraceE("%s: ddraw ReleaseDC ERROR lpdds=%#x res=%#x(%s) @%d\n", ApiRef, lpdds, res, ExplainDDError(res), __LINE__);
	ret=(*pGDIReleaseDC)(hOverlayWnd, thdc);
	_if(!ret) OutTraceE("%s: GDI ReleaseDC ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);

	while((*pReleaseS)(lpddsTmp));
	return DD_OK;
}
