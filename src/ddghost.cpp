#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "hddraw.h"
#include "dxwcore.hpp"

#ifdef GHOSTTEXTURE
#define TEXMINLIMIT 64

extern char *ExplainDDError(DWORD);

GUID TextureLinkGUID;

void InitGhostGUID(void)
{
	CoCreateGuid(&TextureLinkGUID);
}

void BuildGhostTexture(LPDIRECTDRAW lpdd, LPDIRECTDRAWSURFACE lpdds, DDSURFACEDESC2 *lpddsd, int dxversion, CreateSurface_Type pCreateSurface)
{
	ApiName("BuildGhostTexture");
	HRESULT res;

	OutTrace("%s: MAKE GHOST RGBA lpdds=%#x\n", ApiRef, lpdds);
	LPDIRECTDRAWSURFACE lpddsRGBA;
	LPDIRECTDRAWSURFACE lpdds2;
	DDSURFACEDESC2 ddsd2;
	if((lpddsd->dwHeight < TEXMINLIMIT) || (lpddsd->dwWidth < TEXMINLIMIT)) {
		OutTrace("%s: filter small TEXTURE size=(%d,%d)\n", ApiRef, lpddsd->dwWidth, lpddsd->dwHeight);
		return;
	}

	memcpy(&ddsd2, lpddsd, sizeof(ddsd2)); 
#if 0
	memset(&ddsd2.ddpfPixelFormat, 0, sizeof(DDPIXELFORMAT));
	ddsd2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
#if TRUE
	ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
	ddsd2.ddpfPixelFormat.dwBBitMask = 0x00FF0000;
	ddsd2.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	ddsd2.ddpfPixelFormat.dwRBitMask = 0x000000FF;
	ddsd2.ddpfPixelFormat.dwRGBBitCount = 32;
	ddsd2.ddpfPixelFormat.dwFlags = DDPF_RGB;
#else
	// try : RGB=(0xf800,0x7e0,0x1f)
	ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
	ddsd2.ddpfPixelFormat.dwBBitMask = 0xf800;
	ddsd2.ddpfPixelFormat.dwGBitMask = 0x7e0;
	ddsd2.ddpfPixelFormat.dwRBitMask = 0x1f;
	ddsd2.ddpfPixelFormat.dwRGBBitCount = 16;
	ddsd2.ddpfPixelFormat.dwFlags = DDPF_RGB;
#endif
	//EnumTextureFormats: CALLBACK context=0x19fae0  PixelFormat flags=0x41(DDPF_ALPHAPIXELS+RGB) BPP=16 RGBA=(0xf00,0xf0,0xf,0xf000) 
	//ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
	//ddsd2.ddpfPixelFormat.dwBBitMask = 0xf00;
	//ddsd2.ddpfPixelFormat.dwGBitMask = 0xf0;
	//ddsd2.ddpfPixelFormat.dwRBitMask = 0xf;
	//ddsd2.ddpfPixelFormat.dwRGBBitCount = 16;
	//ddsd2.ddpfPixelFormat.dwFlags = DDPF_RGB;

#endif
	ddsd2.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	//ddsd2.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE;
	//ddsd2.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;

	res =( *pCreateSurface)(lpdd, &ddsd2, &lpddsRGBA, NULL);
	if(res) OutTrace("%s: CREATESURFACE ERROR %#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);

	typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID, LPVOID *);
	extern QueryInterface_Type pQueryInterfaceS1;
	res = (*pQueryInterfaceS1)(lpdds, IID_IDirectDrawSurface4, (LPVOID *)&lpdds2);
	if(res) OutTrace("%s: UPGRADE ERROR %#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);

	extern GUID TextureLinkGUID;
	res = ((LPDIRECTDRAWSURFACE4)lpdds2)->SetPrivateData(TextureLinkGUID, &lpddsRGBA, sizeof(LPDIRECTDRAWSURFACE), 0);
	if(res) OutTrace("%s: SET ERROR %#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	if(!res) OutTrace("%s: SET lpddsRGBA=%#x @%d\n", ApiRef, lpddsRGBA, __LINE__);
	//DWORD size;
	//res = ((LPDIRECTDRAWSURFACE4)(*lplpdds))->GetPrivateData(TextureLinkGUID, &lpddsRGBA, &size);
	//if(!res) OutTrace("%s: GET lpddsRGBA=%#x size=%d @%d\n", ApiRef, lpddsRGBA, size, __LINE__);
	LONG ref;
	while ((ref = lpdds2->Release()) > 0);
	if(ref) OutTrace("%s: ref = @%d\n", ApiRef, res, __LINE__);
	OutTrace("%s: END\n", ApiRef);
}

LPDIRECTDRAWSURFACE ReplaceTexture(int dxversion, LPDIRECTDRAWSURFACE lpdds)
{
	ApiName("ReplaceTexture");
	HRESULT res;
	LPDIRECTDRAWSURFACE lpddsghost = lpdds;
	extern void TextureDump(LPDIRECTDRAWSURFACE, int);
	extern void TextureHighlight(LPDIRECTDRAWSURFACE, int);
	extern char *ExplainDDError(DWORD);
	while(TRUE){ // fake loop
		//dxw.dwDFlags2 &= ~EXPERIMENTAL8;
		OutTrace("%s: BEGIN lpdds=%#x\n", ApiRef, lpdds);
		DDSURFACEDESC2 ddsd;
		extern GetSurfaceDesc2_Type pGetSurfaceDescMethod(int);
		ddsd.dwSize = (dxversion < 4) ? sizeof(DDSURFACEDESC) : sizeof(DDSURFACEDESC2);
		res = (*pGetSurfaceDescMethod(dxversion))((LPDIRECTDRAWSURFACE2)lpdds, &ddsd);
		if(res){
			OutTrace("%s: GetSurfaceDesc ERROR res=%#x\n", ApiRef, res);
			break;
		}
		if(!(ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE)) {
			OutTrace("%s: GetSurfaceDesc NO TEXTURE res=%#x\n", ApiRef, res);
			break;
		}
		if((ddsd.dwHeight < TEXMINLIMIT) || (ddsd.dwWidth < TEXMINLIMIT)) {
			OutTrace("%s: filter small TEXTURE size=(%d,%d)\n", ApiRef, ddsd.dwWidth, ddsd.dwHeight);
			break;
		}
		extern GUID TextureLinkGUID;
		LPDIRECTDRAWSURFACE lpddsRGBA;
		LPDIRECTDRAWSURFACE lpdds2;
		DWORD size;
		typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID, LPVOID *);
		extern QueryInterface_Type pQueryInterfaceS1;
		res = (*pQueryInterfaceS1)(lpdds, IID_IDirectDrawSurface4, (LPVOID *)&lpdds2);
		if(res) {
			OutTrace("%s: UPGRADE ERROR %#x @%d\n", ApiRef, res, __LINE__);
			break;
		}
		size = sizeof(LPDIRECTDRAWSURFACE);
		res = ((LPDIRECTDRAWSURFACE4)lpdds2)->GetPrivateData(TextureLinkGUID, &lpddsRGBA, &size);
		if(res) {
			OutTrace("%s: GetPrivateData ERROR %#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
			lpdds2->Release();
			break;
		}
		OutTrace("%s: GetPrivateData lpddsRGBA=%#x\n", ApiRef, lpddsRGBA);
#if 1
		TextureHighlight(lpddsRGBA, dxversion);
		TextureDump(lpddsRGBA, dxversion);
		TextureDump(lpdds, dxversion);
#else
		HDC hdcOrig, hdcGhost;
		lpddsRGBA->GetDC(&hdcGhost);
		((LPDIRECTDRAWSURFACE)lpdds)->GetDC(&hdcOrig);
		//res = lpddsRGBA->Blt(NULL, (LPDIRECTDRAWSURFACE)lpdds, NULL, 0, 0);
		//if(res) OutTrace("%s: BLT ERROR %#x @%d\n", ApiRef, res, __LINE__);
		if(!BitBlt(hdcGhost, 0, 0, ddsd.dwWidth, ddsd.dwHeight, hdcOrig, 0, 0, SRCCOPY)){
			OutTrace("%s: BITBLT ERROR %d @%d\n", ApiRef, GetLastError(), __LINE__);
		} 
		extern void TextureDump(LPDIRECTDRAWSURFACE, int);
		TextureDump(lpddsRGBA, 4);
#endif
		lpddsghost = lpddsRGBA;
		lpdds2->Release();
		break;
	}
	//dxw.dwDFlags2 |= EXPERIMENTAL8;
	OutTrace("%s: returning lpdds=%#x\n", ApiRef, lpddsghost);
	return lpddsghost;
}

#endif