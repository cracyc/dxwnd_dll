#define  _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <d3d.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhook.h"
#include "syslibs.h"
#include "dxhelper.h"
#include "dxdds.h"
#include "hddraw.h"

extern char *ExplainDDError(DWORD);

typedef HRESULT (WINAPI *Lock_Type)(LPDIRECTDRAWSURFACE, LPRECT, LPDDSURFACEDESC, DWORD, HANDLE);
typedef HRESULT (WINAPI *Unlock4_Type)(LPDIRECTDRAWSURFACE, LPRECT);
typedef HRESULT (WINAPI *Unlock1_Type)(LPDIRECTDRAWSURFACE, LPVOID);

extern Lock_Type pLockMethod(int);
extern Unlock4_Type pUnlockMethod(int);
extern int Set_dwSize_From_Surface();

void ddColor2Alpha(LPDIRECTDRAWSURFACE s, int dxversion)
{
	ApiName("ddColor2Alpha");
	DDSURFACEDESC2 ddsd;
	int x, y, w, h;
	HRESULT res;

	OutDebugDW("%s(%d): lpdds=%#x color=%#x\n", ApiRef, dxversion, s, dxw.dwTransparentColor);

	if(dxw.dwTransparentColor == 0xFFFFFFFF) return;
	memset(&ddsd,0,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = Set_dwSize_From_Surface();
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	if(res=(*pLockMethod(dxversion))(s, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_WAIT, 0)){	
		OutTraceE("%s(%d): Lock ERROR res=%#x(%s) @%d\n", ApiRef, dxversion, res, ExplainDDError(res), __LINE__);
		return;
	}
	if((ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) && !dxwss.IsABackBufferSurface(s)) {
		OutTraceDW("%s(%d): lpdds=%#x BitCount=%d size=(%dx%d)\n", 
			ApiRef, dxversion, s, ddsd.ddpfPixelFormat.dwRGBBitCount, ddsd.dwWidth, ddsd.dwHeight);
		w = ddsd.dwWidth;
		h = ddsd.dwHeight;
		switch (ddsd.ddpfPixelFormat.dwRGBBitCount){
			//case 8:
			//	// no way
			//	break;
			case 16: 
				{
					WORD *p;
					WORD alphaMask = (WORD)ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
					WORD colorMask = ~(WORD)ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
					for(y=0; y<h; y++){
						p = (WORD *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 1);
						for(x=0; x<w; x++) 
							if((*(p+x) & colorMask) == (WORD)dxw.dwTransparentColor) 
								*(p+x) = *(p+x) & colorMask;
					}
				}
				break;
			case 32: 
				{
					DWORD *p;
					DWORD alphaMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
					DWORD colorMask = ~ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
					for(y=0; y<h; y++){
						p = (DWORD *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 2);
						for(x=0; x<w; x++)
							if((*(p+x) & colorMask) == dxw.dwTransparentColor) 
								*(p+x) = *(p+x) & colorMask;
					}
				}
				break;
		}
	}
	res=(*pUnlockMethod(dxversion))(s, NULL);
	_if (res) OutTraceE("%s: Unlock ERROR lpdds=%#x res=%#x(%s) @%d\n", ApiRef, s, res, ExplainDDError(res), __LINE__);
}
