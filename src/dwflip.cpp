/* ------------------------------------------------------------------ */
// DirectDraw flipchain implementation
/* ------------------------------------------------------------------ */

//#define TRACEFLIPCHAIN

#ifdef TRACEFLIPCHAIN
#define log(f, ...) OutTrace(f, __VA_ARGS__)
#define logerror(res) 	if(res) log("swap: error res=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__)
#else
#define log(f, ...)
#define logerror(res)
#endif

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "hddraw.h"

extern char *ExplainDDError(DWORD);
extern ReleaseS_Type pReleaseSMethod(int);

#ifdef TRACEFLIPCHAIN
void dxwFlipChain::dumpFlipChain()
{
	char s[256];
	sprintf(s, "=== %s len=%d idx=%d lpdds=", label, length, index);
	for(int i=0; i<length; i++) sprintf(s, "%s%#x ", s, flipChain[i]);
	strcat(s, "\n");
	log(s);
}
#else
#define dumpFlipChain()
#endif

extern GetSurfaceDesc_Type pGetSurfaceDesc1, pGetSurfaceDesc2, pGetSurfaceDesc3;
extern GetSurfaceDesc2_Type pGetSurfaceDesc4, pGetSurfaceDesc7;
extern Blt_Type pBlt1, pBlt2, pBlt3, pBlt4, pBlt7;

dxwFlipChain::dxwFlipChain(char *name)
{
	log(">> dxwFlipChain::dxwFlipChain\n");
	index = 0;
	length = 0;
	label = name;
	for(int i=0; i<MAXFLIPCHAIN; i++) flipChain[i] = NULL;
}

void dxwFlipChain::Initialize(int dxversion, LPDIRECTDRAWSURFACE lpdds, BOOL bCycle)
{
	log(">> dxwFlipChain::Initialize(%#x)\n", lpdds);
	version = dxversion;
	for(int i=0; i<MAXFLIPCHAIN; i++) flipChain[i] = NULL;
	index = 0;
	length = 1;
	cycle = bCycle;
	flipChain[0] = lpdds;
	switch(version){
		case 1: 
			dwSize = sizeof(DDSURFACEDESC);
			pGetSurfaceDesc = (GetSurfaceDesc2_Type)pGetSurfaceDesc1;
			pBlt = (Blt_Type)pBlt1;
			break;
		case 2: 
			dwSize = sizeof(DDSURFACEDESC);
			pGetSurfaceDesc = (GetSurfaceDesc2_Type)pGetSurfaceDesc2;
			pBlt = (Blt_Type)pBlt2;
			break;
		case 3: 
			dwSize = sizeof(DDSURFACEDESC);
			pGetSurfaceDesc = (GetSurfaceDesc2_Type)pGetSurfaceDesc3;
			pBlt = (Blt_Type)pBlt3;
			break;
		case 4: 
			dwSize = sizeof(DDSURFACEDESC2);
			pGetSurfaceDesc = pGetSurfaceDesc4;
			pBlt = (Blt_Type)pBlt4;
			break;
		case 7: 
			dwSize = sizeof(DDSURFACEDESC2);
			pGetSurfaceDesc = pGetSurfaceDesc7;
			pBlt = (Blt_Type)pBlt7;
			break;
		default:
			MessageBox(0, "Impossible flip", "DxWnd", 0);
			return;
	}
}

BOOL dxwFlipChain::AddFlipSurface(LPDIRECTDRAWSURFACE lpdds)
{
	log(">> dxwFlipChain::AddFlipSurface(%#x)\n", lpdds);
	if(length == (MAXFLIPCHAIN-1)) {
		log("dxwAddFlipSurface: ERROR reached max length=%d\n", MAXFLIPCHAIN);
		return FALSE;
	}
	// v2.06.03: @#@ "Atria" (Ko)
	for(int i=0; i<length; i++) if(flipChain[i] == lpdds) return TRUE; // surface already in flipchain 
	flipChain[length] = lpdds;
	length++;
	return TRUE;
}

LPDIRECTDRAWSURFACE dxwFlipChain::GetPrimarySurface()
{
	log(">> dxwFlipChain::GetPrimarySurface()\n");
	dumpFlipChain();
	if(length == 0) return 0;
	return flipChain[0];
}

BOOL dxwFlipChain::SetPrimarySurface(LPDIRECTDRAWSURFACE lpdds)
{
	log(">> dxwFlipChain::SetPrimarySurface()\n");
	dumpFlipChain();
	if(length == 0) return FALSE;
	flipChain[0] = lpdds;
	return TRUE;
}

int dxwFlipChain::GetLength()
{
	log(">> dxwFlipChain::GetLength\n");
	dumpFlipChain();
	log("<< %d\n", length);
	return length;
}

void dxwFlipChain::ReleaseFlipChain()
{
	log(">> dxwFlipChain::ReleaseFlipChain\n");
	dumpFlipChain();
	// release all except the primary surface that is release outside
	for(int i=1; i<length; i++) {
		LPDIRECTDRAWSURFACE lpdds = flipChain[i];
		__try { // try/except useful when using proxy dll 
			while(lpdds->Release());
		}
		__except(EXCEPTION_EXECUTE_HANDLER){ 
			log(">> dxwFlipChain::ReleaseFlipChain: EXCEPTION on lpdds=%#x\n", lpdds);
		}
		dxwss.UnrefSurface(lpdds);
		flipChain[i] = NULL;
	}
	flipChain[0] = NULL;
	index = 0;
	length = 0;
}

BOOL dxwFlipChain::GetAttachedSurface(LPDIRECTDRAWSURFACE lpdds, LPDIRECTDRAWSURFACE *lplpdds)
{
	log(">> dxwFlipChain::GetAttachedSurface(%#x)\n", lpdds);
	dumpFlipChain();
	for(int i=0; i<length; i++){
		if(lpdds == flipChain[i]){
			if(i < (length - 1)) {
				i++;
				*lplpdds = flipChain[i];
				log("<< dxwFlipChain::GetAttachedSurface: lpdds=%#x -> attach=%#x\n", lpdds, *lplpdds);
				return TRUE;
			}
			else {
				log("<< dxwFlipChain::GetAttachedSurface: lpdds=%#x has no attach\n", lpdds);
				*lplpdds = NULL;
				return FALSE;
			}
		}
	}
	log("<< dxwFlipChain::GetAttachedSurface: ERROR can't find lpdds=%#x\n", lpdds);
	*lplpdds = NULL;
	return FALSE;
}

LPDIRECTDRAWSURFACE dxwFlipChain::GetNext(LPDIRECTDRAWSURFACE lpdds)
{
	for(int i=0; i<length; i++){
		if(flipChain[i] == lpdds){
			return flipChain[(i+1) % length];
		}
	}
	return 0;
}

extern int iPrimarySurfaceVersion;
extern LPDIRECTDRAW lpPrimaryDD;
extern CreateSurface_Type pCreateSurfaceMethod(int);
extern HRESULT WINAPI sBlt(int, Blt_Type, ApiArg, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX, BOOL);

void dxwFlipChain::Flip()
{
	log(">> dxwFlipChain::Flip(%d)\n", cycle);
	dumpFlipChain();
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE lpDDSBack;
	LPDIRECTDRAWSURFACE lpddsTmp;
	HRESULT res;
	if(length == 1) return; 
	index = 0; // just in case ...
	lpDDSBack = flipChain[1];

	memset(&ddsd, 0, dwSize);
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwSize = dwSize;
	res = (*pGetSurfaceDesc)((LPDIRECTDRAWSURFACE2)lpDDSBack, (LPDDSURFACEDESC2)&ddsd);
	logerror(res);
	log("BLIT: back=%#x size(w*h)=(%dx%d)\n", lpDDSBack, ddsd.dwWidth, ddsd.dwHeight);
	RECT rect = {0, 0, ddsd.dwWidth, ddsd.dwHeight};
	LPRECT lpRect = &rect;
	if(cycle){
		// create a temporary working surface
		// v2.04.09 fix: dxversion replaced with iPrimarySurfaceVersion - fixes "Gruntz" crash
		res=(*pCreateSurfaceMethod(iPrimarySurfaceVersion))(lpPrimaryDD, &ddsd, &lpddsTmp, NULL); 
		//res=(*pCreateSurfaceMethod(version))(lpPrimaryDD, &ddsd, &lpddsTmp, NULL); 
		logerror(res);
		log("BLIT: from(%d:%#x) to(tmp:%#x)\n", 0, flipChain[0], lpddsTmp);
		if(res){
			OutTraceE("swap: error res=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
			cycle = FALSE;
		}
		else {
			res= (*pBlt)(lpddsTmp, lpRect, flipChain[0], lpRect, DDBLT_WAIT, NULL);
			logerror(res);
		}
	}
	for(int i=0; i<length-1; i++){
		RECT DestRect;
		LPRECT lpDestRect = lpRect;
		if((i==0) && (!dxw.IsEmulated)) {
			DestRect = dxw.GetUnmappedScreenRect();
			lpDestRect = &DestRect;
		}
		log("BLIT: from(%d:%#x) to(%d:%#x)\n", i+1, flipChain[i+1], i, flipChain[i]);
		res = pBlt(flipChain[i], lpDestRect, flipChain[i+1], lpRect, DDBLT_WAIT, NULL);
		logerror(res);
	}
	if(cycle){
		// restore flipped backbuffer and delete temporary surface
		log("BLIT: from(tmp:%#x) to(%d:%#x)\n", lpddsTmp, length-1, flipChain[length-1]);
		res = (*pBlt)(flipChain[length-1], lpRect, lpddsTmp, lpRect, DDBLT_WAIT, NULL);
		logerror(res);
		(*pReleaseSMethod(version))(lpddsTmp);
	}
}

void dxwFlipChain::Flip(LPDIRECTDRAWSURFACE lpddsp, LPDIRECTDRAWSURFACE lpddsb)
{
	log(">> dxwFlipChain::Flip(%d) lpddsp=%#x lpddsb=%#x\n", cycle, lpddsp, lpddsb);
	dumpFlipChain();
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE lpddsTmp;
	HRESULT res;
	if(length == 1) return; 
	if(lpddsp == lpddsb) return;
	index = 0; // just in case ...

	memset(&ddsd, 0, dwSize);
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwSize = dwSize;
	res = (*pGetSurfaceDesc)((LPDIRECTDRAWSURFACE2)lpddsb, (LPDDSURFACEDESC2)&ddsd);
	logerror(res);
	log("BLIT: back=%#x size(w*h)=(%dx%d)\n", lpddsb, ddsd.dwWidth, ddsd.dwHeight);
	RECT rect = {0, 0, ddsd.dwWidth, ddsd.dwHeight};
	LPRECT lpRect = &rect;
	if(cycle){
		// create a temporary working surface
		// v2.04.09 fix: dxversion replaced with iPrimarySurfaceVersion - fixes "Gruntz" crash
		res=(*pCreateSurfaceMethod(iPrimarySurfaceVersion))(lpPrimaryDD, &ddsd, &lpddsTmp, NULL); 
		logerror(res);
		log("BLIT: from(%#x) to(tmp:%#x)\n", lpddsp, lpddsTmp);
		if(res){
			OutTraceE("swap: error res=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
			cycle = FALSE;
		}
		else {
			res= (*pBlt)(lpddsTmp, lpRect, lpddsp, lpRect, DDBLT_WAIT, NULL);
			logerror(res);
		}
	}
	log("BLIT: from(%#x) to(%#x)\n", lpddsb, lpddsp);
	res = pBlt(lpddsp, lpRect, lpddsb, lpRect, DDBLT_WAIT, NULL);
	logerror(res);
	if(cycle){
		// restore flipped backbuffer and delete temporary surface
		log("BLIT: from(tmp:%#x) to(%#x)\n", lpddsTmp, lpddsb);
		res = (*pBlt)(lpddsb, lpRect, lpddsTmp, lpRect, DDBLT_WAIT, NULL);
		logerror(res);
		(*pReleaseSMethod(iPrimarySurfaceVersion))(lpddsTmp);
	}
}

void dxwFlipChain::FlipPrimary(DWORD dwBltFlags)
{
	log(">> dxwFlipChain::Flip(%d)\n", cycle);
	dumpFlipChain();
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE lpDDSBack;
	LPDIRECTDRAWSURFACE lpddsTmp;
	HRESULT res;
	if(length == 1) return; 
	index = 0; // just in case ...
	lpDDSBack = flipChain[1];

	memset(&ddsd, 0, dwSize);
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwSize = dwSize;
	res = (*pGetSurfaceDesc)((LPDIRECTDRAWSURFACE2)lpDDSBack, (LPDDSURFACEDESC2)&ddsd);
	logerror(res);
	log("BLIT: back=%#x size(w*h)=(%dx%d)\n", lpDDSBack, ddsd.dwWidth, ddsd.dwHeight);
	RECT rect = {0, 0, ddsd.dwWidth, ddsd.dwHeight};
	LPRECT lpRect = &rect;
	if(cycle){
		// create a temporary working surface
		// v2.04.09 fix: dxversion replaced with iPrimarySurfaceVersion - fixes "Gruntz" crash
		res=(*pCreateSurfaceMethod(iPrimarySurfaceVersion))(lpPrimaryDD, &ddsd, &lpddsTmp, NULL); 
		//res=(*pCreateSurfaceMethod(version))(lpPrimaryDD, &ddsd, &lpddsTmp, NULL); 
		logerror(res);
		log("BLIT: from(%d:%#x) to(tmp:%#x)\n", 0, flipChain[0], lpddsTmp);
		if(res){
			OutTraceE("swap: error res=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
			cycle = FALSE;
		}
		else {
			res= (*pBlt)(lpddsTmp, lpRect, flipChain[0], lpRect, DDBLT_WAIT, NULL);
			logerror(res);
		}
	}
	for(int i=0; i<length-1; i++){
		//RECT DestRect;
		LPRECT lpDestRect = lpRect;
		log("BLIT: from(%d:%#x) to(%d:%#x)\n", i+1, flipChain[i+1], i, flipChain[i]);
		if(i==0) {
			res = sBlt(version, pBlt, "Flip", flipChain[0], NULL, flipChain[1], NULL, dwBltFlags, NULL, TRUE);
		}
		else {
			res = pBlt(flipChain[i], lpDestRect, flipChain[i+1], lpRect, DDBLT_WAIT, NULL);
		}
		logerror(res);
	}
	if(cycle){
		// restore flipped backbuffer and delete temporary surface
		log("BLIT: from(tmp:%#x) to(%d:%#x)\n", lpddsTmp, length-1, flipChain[length-1]);
		res = (*pBlt)(flipChain[length-1], lpRect, lpddsTmp, lpRect, DDBLT_WAIT, NULL);
		logerror(res);
		(*pReleaseSMethod(version))(lpddsTmp);
	}
}

void dxwFlipChain::FlipPrimary(DWORD dwBltFlags, LPDIRECTDRAWSURFACE lpddsp, LPDIRECTDRAWSURFACE lpddsb)
{
	log(">> dxwFlipChain::Flip(%d) lpddsp=%#x lpddsb=%#x\n", cycle, lpddsp, lpddsb);
	dumpFlipChain();
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE lpddsTmp;
	HRESULT res;
	if(length == 1) return; 
	if(lpddsp == lpddsb) return;
	index = 0; // just in case ...

	memset(&ddsd, 0, dwSize);
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwSize = dwSize;
	res = (*pGetSurfaceDesc)((LPDIRECTDRAWSURFACE2)lpddsb, (LPDDSURFACEDESC2)&ddsd);
	logerror(res);
	log("BLIT: back=%#x size(w*h)=(%dx%d)\n", lpddsb, ddsd.dwWidth, ddsd.dwHeight);
	RECT rect = {0, 0, ddsd.dwWidth, ddsd.dwHeight};
	LPRECT lpRect = &rect;
	if(cycle){
		// create a temporary working surface
		// v2.04.09 fix: dxversion replaced with iPrimarySurfaceVersion - fixes "Gruntz" crash
		res=(*pCreateSurfaceMethod(iPrimarySurfaceVersion))(lpPrimaryDD, &ddsd, &lpddsTmp, NULL); 
		logerror(res);
		log("BLIT: from(%#x) to(tmp:%#x)\n", lpddsp, lpddsTmp);
		if(res){
			OutTraceE("swap: error res=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
			cycle = FALSE;
		}
		else {
			res= (*pBlt)(lpddsTmp, lpRect, lpddsp, lpRect, DDBLT_WAIT, NULL);
			logerror(res);
		}
	}
	log("BLIT: from(%#x) to(%#x)\n", lpddsb, lpddsp);
	res = sBlt(version, pBlt, "Flip", lpddsp, NULL, lpddsb, NULL, dwBltFlags, NULL, TRUE);
	logerror(res);
	if(cycle){
		// restore flipped backbuffer and delete temporary surface
		log("BLIT: from(tmp:%#x) to(%#x)\n", lpddsTmp, lpddsb);
		res = (*pBlt)(lpddsb, lpRect, lpddsTmp, lpRect, DDBLT_WAIT, NULL);
		logerror(res);
		(*pReleaseSMethod(iPrimarySurfaceVersion))(lpddsTmp);
	}
}
