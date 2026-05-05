//#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "stdio.h"
#include "hddraw.h"
#include "dxhelper.h"

extern Lock_Type pLockMethod(int);
extern Unlock4_Type pUnlockMethod(int);

#if 1
void dxwXMirrorBlt(ApiArg, int dxversion, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect, LPDIRECTDRAWSURFACE lpddssrc, LPRECT lpsrcrect)
{
	// initialize surface descriptors
	DDSURFACEDESC2 ddsd, ddsdsrc;
	int dwSize = (dxversion<4)?sizeof(DDSURFACEDESC):sizeof(DDSURFACEDESC2);
	memset(&ddsd, 0, dwSize);
	memset(&ddsdsrc, 0, dwSize);
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	ddsdsrc.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	ddsd.dwSize = dwSize;
	ddsdsrc.dwSize = dwSize;
	HRESULT res;

	OutTrace("%s: XMIRRORING from=%#x to=%#x\n", ApiRef, lpddssrc, lpdds);

	// get surface access
	res=(*pLockMethod(dxversion))(lpdds, NULL, (LPDDSURFACEDESC)&ddsd, DDLOCK_WRITEONLY|DDLOCK_WAIT, 0);
	if(res) OutTrace("mirrorblt: err=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	res=(*pLockMethod(dxversion))(lpddssrc, NULL, (LPDDSURFACEDESC)&ddsdsrc, DDLOCK_WAIT, 0);
	if(res) OutTrace("mirrorblt: err=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);

	OutTrace("%s: XMIRRORING size from=(%dx%d) to=(%dx%d) bpp=%d @%d\n", 
		ApiRef, ddsdsrc.dwWidth, ddsdsrc.dwHeight, ddsd.dwWidth, ddsd.dwHeight, ddsd.ddpfPixelFormat.dwRGBBitCount, __LINE__);

	switch(ddsd.ddpfPixelFormat.dwRGBBitCount){
		case 8:
			{
				BYTE *pxFrom, *pxTo;
				pxTo = (BYTE *)ddsd.lpSurface;
				pxFrom = (BYTE *)ddsdsrc.lpSurface;
				OutTrace("copy from %#x to %#x\n", pxFrom, pxTo);
				for(DWORD y=0; y<ddsd.dwHeight; y++){
					for(DWORD x=0; x<ddsd.dwWidth; x++) {
						pxTo[x]=pxFrom[ddsd.dwWidth - x -1];
					}
					pxTo += ddsd.lPitch;
					pxFrom += ddsdsrc.lPitch;
				}
			}
			break;
		case 16:
			{
				WORD *pxFrom, *pxTo;
				pxTo = (WORD *)ddsd.lpSurface;
				pxFrom = (WORD *)ddsdsrc.lpSurface;
				OutTrace("copy from %#x to %#x\n", pxFrom, pxTo);
				for(DWORD y=0; y<ddsd.dwHeight; y++){
					for(DWORD x=0; x<ddsd.dwWidth; x++) {
						pxTo[x]=pxFrom[ddsd.dwWidth - x -1];
					}
					pxTo += ddsd.lPitch >> 1;
					pxFrom += ddsdsrc.lPitch >> 1;
				}
			}
			break;
		case 32:
			{
				DWORD *pxFrom, *pxTo;
				pxTo = (DWORD *)ddsd.lpSurface;
				pxFrom = (DWORD *)ddsdsrc.lpSurface;
				OutTrace("copy from %#x to %#x\n", pxFrom, pxTo);
				for(DWORD y=0; y<ddsd.dwHeight; y++){
					for(DWORD x=0; x<ddsd.dwWidth; x++) {
						pxTo[x]=pxFrom[ddsd.dwWidth - x -1];
					}
					pxTo += ddsd.lPitch >> 2;
					pxFrom += ddsdsrc.lPitch >> 2;
				}
			}
			break;
	}

	res=(*pUnlockMethod(dxversion))(lpdds, lpdestrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pUnlockMethod(dxversion))(lpddssrc, lpsrcrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
}
#else
void dxwXMirrorBlt(ApiArg, int dxversion, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect, LPDIRECTDRAWSURFACE lpddssrc, LPRECT lpsrcrect)
{
	DDSURFACEDESC2 ddsd, ddsdsrc;
	HDC hSrc, hDst;
	int dwSize = (dxversion<4)?sizeof(DDSURFACEDESC):sizeof(DDSURFACEDESC2);
	memset(&ddsd, 0, dwSize);
	memset(&ddsdsrc, 0, dwSize);
	ddsd.dwSize = dwSize;
	ddsdsrc.dwSize = dwSize;
	int fixedWidth;
	res = (*pGetSurfaceDescMethod(dxversion))((LPDIRECTDRAWSURFACE2)lpdds, &ddsd);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res = (*pGetSurfaceDescMethod(dxversion))((LPDIRECTDRAWSURFACE2)lpddssrc, &ddsdsrc);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pGetDCMethod())(lpddssrc, &hSrc);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pGetDCMethod())(lpdds, &hDst);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);

	fixedWidth = (ddsdsrc.dwWidth >> 2) << 2;

	res=(*pGDIStretchBlt)(hDst, fixedWidth, 0, -fixedWidth, ddsd.dwHeight, hSrc, 0, 0, fixedWidth, ddsdsrc.dwHeight, SRCCOPY);

	if(!res) OutTrace("%s: XMIRRORING err=%d @%d\n", GetLastError(), __LINE__);
	res=(*pUnlockMethod(dxversion))(lpdds, lpdestrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pUnlockMethod(dxversion))(lpddssrc, lpsrcrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
}
#endif

#if 1
void dxwYMirrorBlt(ApiArg, int dxversion, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect, LPDIRECTDRAWSURFACE lpddssrc, LPRECT lpsrcrect)
{
	// initialize surface descriptors
	DDSURFACEDESC2 ddsd, ddsdsrc;
	int dwSize = (dxversion<4)?sizeof(DDSURFACEDESC):sizeof(DDSURFACEDESC2);
	memset(&ddsd, 0, dwSize);
	memset(&ddsdsrc, 0, dwSize);
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	ddsdsrc.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	ddsd.dwSize = dwSize;
	ddsdsrc.dwSize = dwSize;
	HRESULT res;

	OutTrace("%s: YMIRRORING from=%#x to=%#x\n", ApiRef, lpddssrc, lpdds);

	// get surface access
	res=(*pLockMethod(dxversion))(lpdds, NULL, (LPDDSURFACEDESC)&ddsd, DDLOCK_WRITEONLY|DDLOCK_WAIT, 0);
	if(res) OutTrace("mirrorblt: err=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	res=(*pLockMethod(dxversion))(lpddssrc, NULL, (LPDDSURFACEDESC)&ddsdsrc, DDLOCK_WAIT, 0);
	if(res) OutTrace("mirrorblt: err=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);

	OutTrace("%s: YMIRRORING size from=(%dx%d) to=(%dx%d) bpp=%d @%d\n", 
		ApiRef, ddsdsrc.dwWidth, ddsdsrc.dwHeight, ddsd.dwWidth, ddsd.dwHeight, ddsd.ddpfPixelFormat.dwRGBBitCount, __LINE__);

	BYTE *pxFrom, *pxTo;
	OutTrace("copy from %#x to %#x\n", ddsdsrc.lpSurface, ddsd.lpSurface);
	for(DWORD y=0; y<ddsd.dwHeight; y++){
		pxTo = (BYTE *)ddsd.lpSurface + (ddsd.lPitch * y);
		pxFrom = (BYTE *)ddsdsrc.lpSurface + (ddsdsrc.lPitch * (ddsd.dwHeight - y - 1));
		memcpy(pxTo, pxFrom, ddsd.lPitch);
	}

	res=(*pUnlockMethod(dxversion))(lpdds, lpdestrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pUnlockMethod(dxversion))(lpddssrc, lpsrcrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
}
#else
void dxwXMirrorBlt(ApiArg, int dxversion, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect, LPDIRECTDRAWSURFACE lpddssrc, LPRECT lpsrcrect)
{
	DDSURFACEDESC2 ddsd, ddsdsrc;
	HDC hSrc, hDst;
	int dwSize = (dxversion<4)?sizeof(DDSURFACEDESC):sizeof(DDSURFACEDESC2);
	memset(&ddsd, 0, dwSize);
	memset(&ddsdsrc, 0, dwSize);
	ddsd.dwSize = dwSize;
	ddsdsrc.dwSize = dwSize;
	int fixedWidth;
	res = (*pGetSurfaceDescMethod(dxversion))((LPDIRECTDRAWSURFACE2)lpdds, &ddsd);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res = (*pGetSurfaceDescMethod(dxversion))((LPDIRECTDRAWSURFACE2)lpddssrc, &ddsdsrc);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pGetDCMethod())(lpddssrc, &hSrc);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pGetDCMethod())(lpdds, &hDst);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);

	res=(*pGDIStretchBlt)(hDst, 0, -ddsd.dwHeight, 0, ddsd.dwHeight, hSrc, 0, 0, ddsdsrc.dwWidth, ddsdsrc.dwHeight, SRCCOPY);

	if(!res) OutTrace("%s: XMIRRORING err=%d @%d\n", GetLastError(), __LINE__);
	res=(*pUnlockMethod(dxversion))(lpdds, lpdestrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	res=(*pUnlockMethod(dxversion))(lpddssrc, lpsrcrect);
	if(res) OutTrace("%s: XMIRRORING err=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
}
#endif