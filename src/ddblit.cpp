#define _CRT_SECURE_NO_WARNINGS
#define INITGUID
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

extern HRESULT DDRawBlitToEmu(int, Blt_Type, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);

static HRESULT DDrawBlit(ApiArg, LPRECT lpdestrect)
{
	HRESULT res;
	RECT emurect;
	LPDIRECTDRAWSURFACE lpdds = dxw.lpBlitterSurface;
	extern LPDIRECTDRAWSURFACE lpDDSEmu_Prim;
	int dxversion = dxw.BlitterDXVersion;
	extern Blt_Type pGetBltMethod(int);
	Blt_Type pBlt = pGetBltMethod(dxversion);
	LPDIRECTDRAWSURFACE lpDDSSource;

	OutDebugDW("%s\n", ApiRef);

	if(!dxw.lpBlitterSurface) return DD_OK;

	emurect = lpdestrect ? *lpdestrect : dxw.GetScreenRect();

	if (res=(*pColorConversion)(dxversion, lpdds, emurect, &lpDDSSource)) {
		OutTraceE("sBlt ERROR: Color conversion failed res=%#x(%s)\n", res, ExplainDDError(res));
		if(dxw.dwFlags1 & SUPPRESSDXERRORS) res=DD_OK;
		return res;
	}

	extern LPDIRECTDRAWSURFACE lpDDSEmu_Prim;
	if(lpDDSEmu_Prim->IsLost()) lpDDSEmu_Prim->Restore();

	dxw.ShowOverlay(lpDDSSource);
	if(dxw.FilterXScalingFactor){
		emurect.right *= dxw.FilterXScalingFactor;
		emurect.bottom *= dxw.FilterYScalingFactor;
	}

	RECT destrect = dxw.MapWindowRect(lpdestrect);
	extern PrimaryBlt_Type pPrimaryBlt;
	res=(*pPrimaryBlt)(dxversion, pBlt, lpDDSEmu_Prim, &destrect, lpDDSSource, &emurect, DDBLT_WAIT, NULL);

	OutDebugDW("%s: done ret=%#x @%d\n", api, res, __LINE__);

	if((dxw.dwDFlags & CAPTURESCREENS) && dxw.bCustomKeyToggle) dxw.ScreenShot(); 
	return res;	
}

HRESULT DDrawSyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	HRESULT res;
	dxw.lpBlitterSurface = lpdds;
	dxw.BlitterDXVersion = dxversion;

	OutDebugDW("DDrawSyncBlitter\n");

	res = DDRawBlitToEmu(dxversion, pBlt, lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);

	if(res == DD_OK) res = DDrawBlit("DDrawSyncBlitter", lpdestrect);

	if(dxw.dwFlags1 & SUPPRESSDXERRORS) res=DD_OK;
	return res;
}

static DWORD WINAPI EMUAsyncThread(LPVOID lpParameter)
{
	while(TRUE){
		if (!dxw.lpBlitterSurface) continue;

		extern void LimitFrameHz(DWORD);  
		LimitFrameHz(60);

		// Request ownership of the critical section.
		EnterCriticalSection(&dxw.CriticalSection); 
		DDrawBlit("DDrawAsyncBlitter", NULL);
		// Release ownership of the critical section.
		LeaveCriticalSection(&dxw.CriticalSection);
	}
}

HRESULT DDrawAsyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	HRESULT res;

	if(!dxw.bAsybcBlitStarted) {
		if (!InitializeCriticalSectionAndSpinCount(&dxw.CriticalSection, 0x00000400) ) {
			OutTraceE("DDrawAsyncBlitter: ERROR in InitializeCriticalSectionAndSpinCount err=%d\n", GetLastError());
			return -1;
		}
		CreateThread(NULL, 0, EMUAsyncThread, (LPVOID)lpdds, 0, NULL);
		dxw.bAsybcBlitStarted = TRUE;
		OutTraceDW("DDrawAsyncBlitter: control thread started\n");
	}

	dxw.lpBlitterSurface = lpdds;
	dxw.BlitterDXVersion = dxversion;

	EnterCriticalSection(&dxw.CriticalSection);
	res = DDRawBlitToEmu(dxversion, pBlt, lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
	LeaveCriticalSection(&dxw.CriticalSection);

	if(res != DD_OK){
		OutTraceE("%s: DDRawBlitToEmu ERROR err=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}

	return DD_OK;
}