#define _MODULE "dxwnd"

#include <D3D8.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhelper.h"

extern void TextureHandling(LPDIRECTDRAWSURFACE);

typedef HRESULT (WINAPI *LockRect_Type)(void *, UINT, D3DLOCKED_RECT *, CONST RECT *, DWORD);
typedef HRESULT (WINAPI *UnlockRect_Type)(void *, UINT);
typedef HRESULT (WINAPI *GetFrontBuffer_Type)(void *, LPDIRECTDRAWSURFACE);
typedef HRESULT (WINAPI *GetAdapterDisplayMode_Type)(void *, UINT, D3DDISPLAYMODE *);
typedef HRESULT	(WINAPI *CopyRects_Type)(void *, LPDIRECTDRAWSURFACE, CONST RECT *, UINT, LPDIRECTDRAWSURFACE, CONST POINT *);
typedef HRESULT (WINAPI *GetDirect3D8_Type)(void *, void **);

extern LockRect_Type pLockRect8;
extern UnlockRect_Type pUnlockRect8;
extern GetAdapterDisplayMode_Type pGetAdapterDisplayMode8;
extern CopyRects_Type pCopyRects;
extern GetFrontBuffer_Type pGetFrontBuffer;
extern GetDirect3D8_Type pGetDirect3D8;

extern void D3DTextureDump(D3DSURFACE_DESC, D3DLOCKED_RECT);
extern void D3DTextureHighlight(D3DSURFACE_DESC, D3DLOCKED_RECT);
extern void D3DTextureHack(D3DSURFACE_DESC, D3DLOCKED_RECT);
extern void D3DTextureTransp(D3DSURFACE_DESC, D3DLOCKED_RECT);
extern char *ExplainD3DSurfaceFormat(DWORD);

static char *sTexType(D3DRESOURCETYPE type)
{
	char *s;
	switch(type){
		case D3DRTYPE_SURFACE:			s="SURFACE";		break;
		case D3DRTYPE_VOLUME:			s="VOLUME";			break;
		case D3DRTYPE_TEXTURE:			s="TEXTURE";		break;
		case D3DRTYPE_VOLUMETEXTURE:	s="VOLUMETEXTURE";	break;
		case D3DRTYPE_CUBETEXTURE:		s="CUBETEXTURE";	break;
		case D3DRTYPE_VERTEXBUFFER:		s="VERTEXBUFFER";	break;
		case D3DRTYPE_INDEXBUFFER:		s="INDEXBUFFER";	break;
		default:						s="unknown";		break;
	}
	return s;
}

void MergeFrontBuffers8(LPVOID lpd3d)
{
	ApiName("MergeFrontBuffers8");
	LPDIRECTDRAWSURFACE lpDDSPrim = dxwss.GetPrimarySurface();
	LPDIRECT3DDEVICE8 lpd3dd = (LPDIRECT3DDEVICE8)lpd3d;
	OutTrace("%s: lpDDSPrim=%#x\n", ApiRef, lpDDSPrim);
	if(lpDDSPrim){
		HRESULT ret;
		//surface pointer
		LPDIRECT3DSURFACE8 pSurface ;
		//grab back buffer
		ret = lpd3dd->GetBackBuffer ( 0 , D3DBACKBUFFER_TYPE_MONO , &pSurface ) ;
		//ret = lpd3dd->GetFrontBufferData (0, &pSurface);
		if(ret) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, ret, ExplainDDError(ret), __LINE__);
		/*do something with surface here*/
		//ret = lpDDSPrim->Blt(NULL, (LPDIRECTDRAWSURFACE)pSurface, NULL, 0, 0);
		D3DLOCKED_RECT lr;
		ret = pSurface->LockRect(&lr, NULL, D3DLOCK_READONLY);
		if(ret) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, ret, ExplainDDError(ret), __LINE__);
		DDSURFACEDESC2 lpdds;
		memset(&lpdds, 0, sizeof(DDSURFACEDESC2));
		lpdds.dwSize = sizeof(DDSURFACEDESC2);
		ret = lpDDSPrim->Lock(NULL, (LPDDSURFACEDESC)&lpdds, DDLOCK_WRITEONLY | DDLOCK_WAIT, NULL);
		//typedef HRESULT (WINAPI *Lock_Type)(LPDIRECTDRAWSURFACE, LPRECT, LPDDSURFACEDESC, DWORD, HANDLE);
		//extern Lock_Type pLock7;
		//ret = (*pLock7)(lpDDSPrim, NULL, (LPDDSURFACEDESC)&lpdds, DDLOCK_WRITEONLY | DDLOCK_WAIT, NULL);
		if(ret) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, ret, ExplainDDError(ret), __LINE__);
		LPBYTE lpdd = (LPBYTE)lpdds.lpSurface;
		LPBYTE lp3d = (LPBYTE)lr.pBits;
		OutTrace("%s: DDRAW{size=%dx%d pitch=%d} D3D{pitch=%d}\n",
			ApiRef,
			lpdds.dwWidth, lpdds.dwHeight, lpdds.lPitch,
			lr.Pitch);
		for(UINT i=0; i<lpdds.dwHeight; i++){
			memcpy(lpdd, lp3d, lr.Pitch);
			lpdd += lpdds.lPitch;
			lp3d += lr.Pitch;
		}
		ret = lpDDSPrim->Unlock(NULL);
		ret = pSurface->UnlockRect();
		//BitBlt(hdc, 0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight(),
		//	hdc8, 0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight(), SRCCOPY);
		//release the surface
		pSurface->Release ( );
	}
}

void D3D8TextureHandling(void *arg, int Level)
{
	ApiName("D3D8TextureHandling");
	HRESULT res;
	LPDIRECT3DBASETEXTURE8 lpd3dbase = (LPDIRECT3DBASETEXTURE8)arg;
	LPDIRECT3DTEXTURE8 lpd3dtex = (LPDIRECT3DTEXTURE8)arg;
	IDirect3DSurface8 *pSurfaceLevel;
	D3DSURFACE_DESC Desc;
	D3DLOCKED_RECT LockedRect;
	D3DRESOURCETYPE TexType = lpd3dbase->GetType();
	OutDebugD3D("%s: arg=%#x level=%d type=%d(%s)\n", ApiRef, (DWORD)arg, Level, TexType, sTexType(TexType));
	// v2.04.26: skip attempts to dump objects not D3DRTYPE_TEXTURE type
	if(TexType!=D3DRTYPE_TEXTURE) return;
	// Beware: attempts to dump surfaces at level > 0 result in stack corruption!!!
	// v2.04.26: commented out, the stack corruption is protected by statement above
	// if(Level > 0) return;
	if(res=lpd3dtex->GetSurfaceLevel(Level, &pSurfaceLevel)){
		OutErrorD3D("%s: Texture::GetSurfaceLevel ERROR: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return;
	}
	if(res=lpd3dtex->GetLevelDesc(Level, &Desc)){
		OutErrorD3D("%s: Texture::GetLevelDesc ERROR: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return;
	}
	pSurfaceLevel->Release();
	OutDebugD3D("%s: level=%d type=%#x usage=%#x\n", ApiRef, Level, Desc.Type, Desc.Usage);
	switch(Desc.Type){
		case D3DRTYPE_SURFACE:
		case D3DRTYPE_TEXTURE:
			break;
		default:
			OutDebugD3D("%s: SKIP type=%#x\n", ApiRef, Desc.Type);
			return;
			break;
	}
	if(Desc.Usage == D3DUSAGE_RENDERTARGET){
		OutDebugD3D("%s: SKIP usage=%#x\n", ApiRef, Desc.Usage);
		return;
	}
	//pSurfaceLevel->GetRenderTargetData(&pRenderTarget, &pDestSurface);	
	res=(*pLockRect8)(lpd3dtex, Level, &LockedRect, NULL, 0);
	OutTraceDW("%s: lpd3dtex=%#x level=%d format=%#x(%s) size=(%dx%d) bits=%#x pitch=%d\n", 
		ApiRef,
		lpd3dtex, Level, Desc.Format, ExplainD3DSurfaceFormat(Desc.Format), 
		Desc.Width, Desc.Height, LockedRect.pBits, LockedRect.Pitch);
	switch(dxw.dwFlags5 & TEXTUREMASK){
		case TEXTUREHIGHLIGHT: 
			D3DTextureHighlight(Desc, LockedRect);
			break;
		case TEXTUREDUMP: 
			D3DTextureDump(Desc, LockedRect);
			break;
		case TEXTUREHACK:
			D3DTextureHack(Desc, LockedRect);
			break;
		case TEXTURETRANSP:
			D3DTextureTransp(Desc, LockedRect);
			break;
	}
	res=(*pUnlockRect8)(lpd3dtex, Level);
}

/*
from http://realmike.org/blog/projects/taking-screenshots-with-direct3d-8/

Accessing the Front Buffer

The IDirect3DDevice8 interface provides the GetFrontBuffer and GetBackBuffer methods to gain access to the swap chain of a Direct3D 8 application. These methods have the 
following characteristics:

    GetBackBuffer – By using this method you can obtain an IDirect3DSurface8 interface pointer for each of the buffers in the swap chain. 
	However, unless you explicitily requested a lockable back buffer when creating the device (by using the D3DPRESENTFLAG_LOCKABLE_BACKBUFFER flag), 
	you are not allowed to lock the surface. The SDK docs mention a “performance cost” when using lockable back buffers, even if they’re not actually locked. 
	Our Screenshot function should not require you to rewrite your device creation code, let alone degrade performance, therefore we won’t use the GetBackBuffer method here.
    GetFrontBuffer – This method copies the contents of the front buffer to a system-memory surface that is provided by the application. 
	What makes the GetFrontBuffer method especially useful for our purposes is that it converts the data into a 32-bit ARGB format so that we don’t have to handle 
	different formats manually.

Note: When using the GetFrontBuffer method, we’ll always capture the entire screen, which might be undesired in a windowed application. However, the majority of 
applications are full-screen. Therefore, we’ll ignore this issue.

The GetFrontBuffer method requires us to provide a system-memory surface of the same dimensions as the screen. This surface will be filled with a copy of the front buffer’s 
contents. So, how do we retrieve the screen dimensions when all we have is a pointer to the IDirect3DDevice8 interface? We can use the GetAdapterDisplayMode method of the 
IDirect3D8 interface to query information about the current display mode of a given adapter. A pointer to the IDirect3D8 interface can be obtained by calling the GetDirect3D 
method of the IDirect3DDevice8 interface. The adapter identifier that is expected by the GetAdapterDisplayMode method can be obtained by using the 
IDirect3DDevice8::GetCreationParameters method. To summarize, these are the required steps to retrieve the screen dimensions:

    Call the IDirect3DDevice8::GetDirect3D method to retrieve a pointer to an IDirect3D8 interface.
    Call the IDirect3DDevice8::GetCreationParameters method, which returns the identifier of the adapter that the Direct3D device uses.
    Call the IDirect3D8::GetAdapterDisplayMode with the adapter identifier that we retrieved in Step 2.

The following code snippet performs these three steps:

D3DDEVICE_CREATION_PARAMETERS dcp;
dcp.AdapterOrdinal = D3DADAPTER_DEFAULT;
lpDevice->GetCreationParameters(&dcp);

D3DDISPLAYMODE dm;
dm.Width = dm.Height = 0;

// retrieve pointer to IDirect3D8 interface,
// which provides the GetAdapterDisplayMode method
LPDIRECT3D8 lpD3D = NULL;
lpDevice->GetDirect3D(&lpD3D);
if (lpD3D)
{
    // query the screen dimensions of the current adapter
    lpD3D->GetAdapterDisplayMode(dcp.AdapterOrdinal, &dm);
    SAFERELEASE(lpD3D);
}

Now we can pass the values in dm.Width and dm.Height to the IDirect3DDevice8::CreateImageSurface method to create a system-memory surface that can be used by the GetFrontBuffer method. The expected format of the surface is D3DFMT_A8R8G8B8, which means that there are 8 bits each for the blue, green, red, and alpha components of the colors. The following code snippet creates the surface and calls the GetFrontBuffer method to fill the surface:

LPDIRECT3DSURFACE8 lpSurface = NULL;
lpDevice->CreateImageSurface(
    dm.Width, dm.Height,
    D3DFMT_A8R8G8B8,
    &lpSurface
);

lpDevice->GetFrontBuffer(lpSurface);
*/

HRESULT dxGetFrontBuffer8(void *lpd3dd, LPDIRECTDRAWSURFACE xdest)
{
	ApiName("dxGetFrontBuffer8");
	HRESULT res;
	D3DDEVICE_CREATION_PARAMETERS dcp;
	LPDIRECT3DSURFACE8 lpSurface = NULL;
	IDirect3DDevice8 *lpDevice = (IDirect3DDevice8 *)lpd3dd;
	IDirect3DSurface8 *dest = (IDirect3DSurface8 *)xdest;

	dcp.AdapterOrdinal = D3DADAPTER_DEFAULT;
	lpDevice->GetCreationParameters(&dcp);

	D3DDISPLAYMODE dm;
	dm.Width = dm.Height = 0;

	// retrieve pointer to IDirect3D8 interface,
	// which provides the GetAdapterDisplayMode method
	LPDIRECT3D8 lpD3D = NULL;
	//res = lpDevice->GetDirect3D(&lpD3D);
	res = (*pGetDirect3D8)(lpDevice, (void **)&lpD3D);
	if(res) {
		OutErrorD3D("%s: GetDirect3D ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

	// query the screen dimensions of the current adapter
	//res = lpD3D->GetAdapterDisplayMode(dcp.AdapterOrdinal, &dm);
	res = (*pGetAdapterDisplayMode8)(lpD3D, dcp.AdapterOrdinal, &dm);
	lpD3D->Release();
	if(res) {
		OutErrorD3D("%s: GetAdapterDisplayMode ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}
	else {
		OutDebugD3D("%s: screen size=(%dx%d)\n", ApiRef, dm.Width, dm.Height);
	}

	res = lpDevice->CreateImageSurface(dm.Width, dm.Height, D3DFMT_A8R8G8B8, &lpSurface);
	if(res) {
		OutErrorD3D("%s: CreateImageSurface ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

	//res = lpDevice->GetFrontBuffer(lpSurface);
	res = (*pGetFrontBuffer)(lpDevice, (LPDIRECTDRAWSURFACE)lpSurface);
	if(res) {
		OutErrorD3D("%s: GetFrontBuffer ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		lpSurface->Release();
		return res;
	}

	RECT p0 = dxw.GetUnmappedScreenRect();
	RECT srcrect = dxw.GetScreenRect();
	OffsetRect(&srcrect, p0.left, p0.top);
	OutDebugD3D("%s: screen rect=(%d,%d)-(%d,%d)\n", ApiRef, srcrect.left, srcrect.top, srcrect.right, srcrect.bottom);
	POINT destpoint = {0, 0};
	//res = lpDevice->CopyRects(lpSurface, (CONST RECT *)&srcrect, 1, dest, (CONST POINT *)&destpoint);
	res = (*pCopyRects)(lpDevice, (LPDIRECTDRAWSURFACE)lpSurface, &srcrect, 1, (LPDIRECTDRAWSURFACE)dest, &destpoint);
	lpSurface->Release();
	if(res) {
		OutErrorD3D("%s: CopyRects ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

#ifndef DXW_NOTRACES
		if((dxw.dwDFlags & DUMPSURFACES) && dxw.bCustomKeyToggle) SurfaceDump(xdest, 8);
#endif // DXW_NOTRACES

	return DD_OK;
}

#ifndef min
#define min(a,b) (a , b) ? a : b
#endif

HRESULT  dxCopyRects(void *lpd3dd, LPDIRECTDRAWSURFACE psrc, LPDIRECTDRAWSURFACE pdst, UINT count, const RECT *src, const POINT *dst)
{
	HRESULT res;
	ApiName("dxCopyRects");
	IDirect3DSurface8 *lpsrc = (IDirect3DSurface8 *)psrc;
	IDirect3DSurface8 *lpdst = (IDirect3DSurface8 *)pdst;
	IDirect3DDevice8 *lpDevice = (IDirect3DDevice8 *)lpd3dd;
	LPDIRECT3DSURFACE8 lpImageSrc = NULL;
	LPDIRECT3DSURFACE8 lpImageDst = NULL;
	D3DSURFACE_DESC SrcDesc, DstDesc;
	D3DLOCKED_RECT SrcLockedRect, DstLockedRect;
	typedef ULONG	(WINAPI *CreateImageSurface8_Type)(void *, UINT, UINT, D3DFORMAT, void**);
	extern CreateImageSurface8_Type pCreateImageSurface8;

	// first, build  source image
	if(res = lpsrc->GetDesc(&SrcDesc)){
		OutErrorD3D("%s: GetDesc ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}
	else{
		OutDebugD3D("%s: source size=(%dx%d) format=%d(%s)\n", 
			ApiRef, SrcDesc.Width, SrcDesc.Height, SrcDesc.Format, ExplainD3DSurfaceFormat(SrcDesc.Format));
	}

	//if(res = lpDevice->CreateImageSurface(SrcDesc.Width, SrcDesc.Height, SrcDesc.Format, &lpImageSrc)){
	if(res = (*pCreateImageSurface8)((void *)lpDevice, SrcDesc.Width, SrcDesc.Height, SrcDesc.Format, (void **)&lpImageSrc)){
		OutErrorD3D("%s: CreateImageSurface ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

	// get source image
	if(res = (*pCopyRects)(lpDevice, (LPDIRECTDRAWSURFACE)psrc, src, count, (LPDIRECTDRAWSURFACE)lpImageSrc, dst)){
		OutErrorD3D("%s: CopyRects ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

	// build a target image similar to destination 
	if(res = lpdst->GetDesc(&DstDesc)){
		OutErrorD3D("%s: GetDesc ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}
	else{
		OutDebugD3D("%s: dest size=(%dx%d) format=%d(%s)\n", 
			ApiRef, DstDesc.Width, DstDesc.Height, DstDesc.Format, ExplainD3DSurfaceFormat(DstDesc.Format));
	}

	//if(res = lpDevice->CreateImageSurface(DstDesc.Width, DstDesc.Height, DstDesc.Format, &lpImageDst)){
	if(res = (*pCreateImageSurface8)((void *)lpDevice, DstDesc.Width, DstDesc.Height, DstDesc.Format, (void **)&lpImageDst)){
		OutErrorD3D("%s: GetDesc ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

	// make the conversion here ....
	if (res=lpImageSrc->LockRect(&SrcLockedRect, NULL, D3DLOCK_READONLY)){
		OutErrorD3D("%s: LockRect ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}
	if(res = lpImageDst->LockRect(&DstLockedRect, NULL, 0)){
		OutErrorD3D("%s: LockRect ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

	RECT r;
	POINT p;
	// if count == 0 it means copy the whole surface rect
	if(count == 0){
		count = 1;
		r.left = 0;
		r.top = 0;
		// v2.05.70 fix: src and dest surfaces may have different size
		// fixes "Dirt track racing 2 demo".
		r.right = min(SrcDesc.Width, DstDesc.Width);
		r.bottom = min(SrcDesc.Height, DstDesc.Height);
		p.x = 0;
		p.y = 0;
		src=&r;
		dst=&p;
	}

	// v2.05.70: added unsupported boolean to avoid returning a positive result
	// in case of unsupported pixel formats (that may be added in the future).
	// Honestly declaring failure fixes the texture handling for menu texts in
	// "Max Payne 2".
	BOOL unsupported = FALSE;
	for(UINT i=0; i<count; i++){
		// pixel conversion here
		switch(SrcDesc.Format){
			case D3DFMT_A8R8G8B8:
			case D3DFMT_X8R8G8B8:
				switch(DstDesc.Format){
					case D3DFMT_R5G6B5:
						{
							OutDebugD3D("%s: converting 32 to 16 BPP\n", ApiRef, res, __LINE__);
							DWORD *srcpix;
							WORD *dstpix;
							for(LONG y=src[i].top; y<src[i].bottom; y++){
								LONG line = y - src[i].top;
								srcpix = (DWORD *)SrcLockedRect.pBits + ((y * SrcLockedRect.Pitch + src[i].left)/ 4);
								dstpix = (WORD *)DstLockedRect.pBits + (((line + dst[i].y) * DstLockedRect.Pitch + dst[i].x) / 2);
								for(LONG x=src[i].left; x<src[i].right; x++){
									*(dstpix++) = *(srcpix++) & 0xFFFF;
								}
							}
						}
						break;
					case D3DFMT_A8R8G8B8:
					case D3DFMT_X8R8G8B8:
						{
							OutDebugD3D("%s: converting 32 to 32 BPP\n", ApiRef, res, __LINE__);
							DWORD *srcpix;
							DWORD *dstpix;
							for(LONG y=src[i].top; y<src[i].bottom; y++){
								LONG line = y - src[i].top;
								srcpix = (DWORD *)SrcLockedRect.pBits + ((y * SrcLockedRect.Pitch + src[i].left)/ 4);
								dstpix = (DWORD *)DstLockedRect.pBits + (((line + dst[i].y) * DstLockedRect.Pitch + dst[i].x) / 4);
								for(LONG x=src[i].left; x<src[i].right; x++){
									*(dstpix ++) = *(srcpix ++); 
								}
							}
						}
					default:
						OutErrorD3D("%s: UNSUPPORTED dst format src=%#x dst=%#x\n", ApiRef, SrcDesc.Format, DstDesc.Format);
						unsupported = TRUE;
						break;
				}
				break;
			case D3DFMT_R5G6B5:
				switch(DstDesc.Format){
					case D3DFMT_A8R8G8B8:
					case D3DFMT_X8R8G8B8:
						{
							OutDebugD3D("%s: converting 16 to 32 BPP\n", ApiRef, res, __LINE__);
							WORD *srcpix;
							DWORD *dstpix;
							extern void SetPalette16BPP(void);
							extern DWORD *Palette16BPP;
							if (!Palette16BPP) SetPalette16BPP();
							for(LONG y=src[i].top; y<src[i].bottom; y++){
								LONG line = y - src[i].top;
								srcpix = (WORD *)SrcLockedRect.pBits + ((y * SrcLockedRect.Pitch + src[i].left)/ 2);
								dstpix = (DWORD *)DstLockedRect.pBits + (((line + dst[i].y) * DstLockedRect.Pitch + dst[i].x) / 4);
								for(LONG x=src[i].left; x<src[i].right; x++){
									*(dstpix ++) = Palette16BPP[*(srcpix ++)]; 
								}
							}
						}
						break;
					case D3DFMT_R5G6B5:
						{
							OutDebugD3D("%s: converting 16 to 16 BPP\n", ApiRef, res, __LINE__);
							WORD *srcpix;
							WORD *dstpix;
							for(LONG y=src[i].top; y<src[i].bottom; y++){
								LONG line = y - src[i].top;
								srcpix = (WORD *)SrcLockedRect.pBits + ((y * SrcLockedRect.Pitch + src[i].left)/ 2);
								dstpix = (WORD *)DstLockedRect.pBits + (((line + dst[i].y) * DstLockedRect.Pitch + dst[i].x) / 2);
								for(LONG x=src[i].left; x<src[i].right; x++){
									*(dstpix ++) = *(srcpix ++); 
								}
							}
						}
					default:
						OutErrorD3D("%s: UNSUPPORTED dst format src=%#x dst=%#x\n", ApiRef, SrcDesc.Format, DstDesc.Format);
						unsupported = TRUE;
						break;
				}
				break;
			case D3DFMT_A4R4G4B4:
				// Found in "Spirit: Stallion of the Cimarron - Forever Free" in "best" options mode 
				switch(DstDesc.Format){
					case D3DFMT_R8G8B8:
					case D3DFMT_A8R8G8B8:
						{
							OutDebugD3D("%s: converting A4R4G4B4 to 32 BPP\n", ApiRef, res, __LINE__);
							WORD *srcpix;
							DWORD *dstpix;
							for(LONG y=src[i].top; y<src[i].bottom; y++){
								LONG line = y - src[i].top;
								srcpix = (WORD *)SrcLockedRect.pBits + ((y * SrcLockedRect.Pitch + src[i].left)/ 2);
								dstpix = (DWORD *)DstLockedRect.pBits + (((line + dst[i].y) * DstLockedRect.Pitch + dst[i].x) / 4);
								for(LONG x=src[i].left; x<src[i].right; x++){
									// alpha channel ...
									DWORD pix = (*srcpix & 0xF000) ? 0xFF000000 : 0x00000000;
									// RGB colors
									pix |= (*srcpix & 0x0F00) << 12;
									pix |= (*srcpix & 0x00F0) << 8;
									pix |= (*srcpix & 0x000F) << 4;
									*(dstpix ++) = pix;
									srcpix ++;
								}
							}
						}
						break;
					default:
						OutErrorD3D("%s: UNSUPPORTED dst format src=%#x dst=%#x\n", ApiRef, SrcDesc.Format, DstDesc.Format);
						unsupported = TRUE;
						break;
				}
				break;
			default:
				OutErrorD3D("%s: UNSUPPORTED src format src=%#x dst=%#x\n", ApiRef, SrcDesc.Format, DstDesc.Format);
				unsupported = TRUE;
				break;
		}
	}

	lpImageSrc->UnlockRect();
	lpImageDst->UnlockRect();

	if(unsupported) {
		lpImageSrc->Release();
		lpImageDst->Release();
		return D3DERR_INVALIDCALL;
	}

	// copy to target surface
	if(res = (*pCopyRects)(lpDevice, (LPDIRECTDRAWSURFACE)lpImageDst, NULL, 0, (LPDIRECTDRAWSURFACE)lpdst, NULL)){
		OutErrorD3D("%s: CopyRects ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return res;
	}

	// clean up
	lpImageSrc->Release();
	lpImageDst->Release();

#ifndef DXW_NOTRACES
	if((dxw.dwDFlags & DUMPSURFACES) && dxw.bCustomKeyToggle) SurfaceDump((LPDIRECTDRAWSURFACE)lpdst, 8);
	if((dxw.dwDFlags2 & DUMPBLITSRC) && dxw.bCustomKeyToggle) SurfaceDump((LPDIRECTDRAWSURFACE)lpsrc, 8);
#endif // DXW_NOTRACES

	return res;
}

LPDIRECT3DSURFACE8 D3D8EmulatedBackBuffer;
LPDIRECT3DSURFACE8 D3D8RealBackBuffer;
LPDIRECT3DSURFACE8 D3D8RealDepthBuffer;
LPDIRECT3DTEXTURE8 D3D8EmulatedBackBufferTexture;
IDirect3DSurface8* D3D8EmulatedBackBufferTextureLevel0;

LPDIRECTDRAWSURFACE dwGetVirtualBackBuffer()
{
	return (LPDIRECTDRAWSURFACE)D3D8EmulatedBackBuffer;
}

HRESULT dwD3D8InitEmulation(void *lpd3ddx)
{
	HRESULT res;
	char *api = "dxwnd.dwD3D8InitEmulation";
	LPDIRECT3DDEVICE8 lpd3dd = (LPDIRECT3DDEVICE8)lpd3ddx;

    // Create the render target which will be used as the real back buffer.
    res = lpd3dd->CreateRenderTarget(dxw.GetScreenWidth(), dxw.GetScreenHeight(), D3DFMT_R5G6B5, D3DMULTISAMPLE_NONE, TRUE, &D3D8EmulatedBackBuffer);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: CreateRenderTarget ERROR res=%#x @%d\n", api, res, __LINE__);
#endif // DXW_NOTRACES

    res = lpd3dd->CreateTexture(dxw.GetScreenWidth(), dxw.GetScreenHeight(), 1, 0, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &D3D8EmulatedBackBufferTexture);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: CreateTexture ERROR res=%#x @%d\n", api, res, __LINE__);
#endif // DXW_NOTRACES

    res = D3D8EmulatedBackBufferTexture->GetSurfaceLevel(0, &D3D8EmulatedBackBufferTextureLevel0);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: GetSurfaceLevel ERROR res=%#x @%d\n", api, res, __LINE__);
#endif // DXW_NOTRACES

    // Locate the real buffers.
    res = lpd3dd->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &D3D8RealBackBuffer);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: GetBackBuffer ERROR res=%#x @%d\n", api, res, __LINE__);
#endif // DXW_NOTRACES

    res = lpd3dd->GetDepthStencilSurface(&D3D8RealDepthBuffer);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: GetDepthStencilSurface ERROR res=%#x @%d\n", api, res, __LINE__);
#endif // DXW_NOTRACES

    // Switch the render target to the emulated one by default.

    res = lpd3dd->SetRenderTarget(D3D8EmulatedBackBuffer, D3D8RealDepthBuffer);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: SetRenderTarget ERROR res=%#x @%d\n", api, res, __LINE__);
#endif // DXW_NOTRACES

	OutTraceD3D("%s: OK\n", api);
	OutTraceD3D("> D3D8EmulatedBackBuffer=%#x\n", D3D8EmulatedBackBuffer);
	OutTraceD3D("> D3D8EmulatedBackBufferTexture=%#x\n", D3D8EmulatedBackBufferTexture);
	OutTraceD3D("> D3D8EmulatedBackBufferTextureLevel0=%#x\n", D3D8EmulatedBackBufferTextureLevel0);
	OutTraceD3D("> D3D8RealBackBuffer=%#x\n", D3D8RealBackBuffer);
	OutTraceD3D("> D3D8RealDepthBuffer=%#x\n", D3D8RealDepthBuffer);
    return D3D_OK;
}

void dwD3D8ShutdownEmulation(void *lpd3ddx)
{
	ApiName("dwD3D8ShutdownEmulation");
	HRESULT res;
	LPDIRECT3DDEVICE8 lpd3dd = (LPDIRECT3DDEVICE8)lpd3ddx;
    
	// Restore targets.

    res=lpd3dd->SetRenderTarget(D3D8RealBackBuffer, D3D8RealDepthBuffer);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: SetRenderTarget ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
#endif // DXW_NOTRACES
    res=D3D8RealBackBuffer->Release();
    res=D3D8RealDepthBuffer->Release();

    // Destroy emulation objects.

    res=D3D8EmulatedBackBufferTextureLevel0->Release();
    res=D3D8EmulatedBackBufferTexture->Release();
    res=D3D8EmulatedBackBuffer->Release();
}

static DWORD set_rs(LPDIRECT3DDEVICE8 lpd3dd, const D3DRENDERSTATETYPE type, const DWORD value)
{
    DWORD old_value;
    lpd3dd->GetRenderState(type, &old_value);
    lpd3dd->SetRenderState(type, value);
    return old_value;
}

static DWORD set_tss(LPDIRECT3DDEVICE8 lpd3dd, const DWORD stage, const D3DTEXTURESTAGESTATETYPE type, const DWORD value)
{
    DWORD old_value;
    lpd3dd->GetTextureStageState(stage, type, &old_value);
    lpd3dd->SetTextureStageState(stage, type, value);
    return old_value;
}

HRESULT dwD3D8Present(void *lpd3ddx, CONST RECT *pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	ApiName("dwD3D8Present");
	HRESULT res;
	LPDIRECT3DDEVICE8 lpd3dd = (LPDIRECT3DDEVICE8)lpd3ddx;

	// from MSDN:
	// hDestWindowOverride
	// [in] Pointer to a destination window whose client area is taken as the target for this presentation. 
	// If this value is NULL, then the hWndDeviceWindow member of D3DPRESENT_PARAMETERS is taken.
	// pDirtyRegion
	// [in] This parameter is not used and should be set to NULL.

	if (hDestWindowOverride && (hDestWindowOverride != dxw.GethWnd())) {
		// V2.05.70: though not supported, a tentative operation could be better than nothing.
		// in particular this fixes the Japanese game "Gun Star".
        OutTraceD3D("%s: WARNING only full window Present is supported\n", ApiRef);
        //return D3DERR_INVALIDCALL;
    }
	if (pDirtyRegion) {
		OutTraceD3D("%s: WARNING pDirtyRegion=%#x should be NULL\n", ApiRef, pDirtyRegion);
	}
	if (pSourceRect || pDestRect) {
        OutErrorD3D("%s: WARNING only full surfaces are supported\n", ApiRef);
        pSourceRect = NULL;
		pDestRect = NULL;
    }

    // Blit the render target to the texture.

    res = lpd3dd->CopyRects(D3D8EmulatedBackBuffer, NULL, 0, D3D8EmulatedBackBufferTextureLevel0, NULL);
#ifndef DXW_NOTRACES
    if(res) OutErrorD3D("%s: CopyRects ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
#endif // DXW_NOTRACES

    // Render the texture to the real back buffer.

    res = lpd3dd->BeginScene();
	if(res) {
		OutErrorD3D("%s: CopyRects ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		return D3DERR_INVALIDCALL;
	}

    LPDIRECT3DSURFACE8 old_back_buffer;
    LPDIRECT3DSURFACE8 old_depth_buffer;
    lpd3dd->GetRenderTarget(&old_back_buffer);
    lpd3dd->GetDepthStencilSurface(&old_depth_buffer);

    lpd3dd->SetRenderTarget(D3D8RealBackBuffer, D3D8RealDepthBuffer);

    LPDIRECT3DBASETEXTURE8 old_txt;
    lpd3dd->GetTexture(0, &old_txt);
    lpd3dd->SetTexture(0, D3D8EmulatedBackBufferTexture);

    DWORD old_vs;
    lpd3dd->GetVertexShader(&old_vs);
    lpd3dd->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);

    IDirect3DVertexBuffer8 *old_stream_0;
    UINT old_stream_0_stride;
    lpd3dd->GetStreamSource(0, &old_stream_0, &old_stream_0_stride);

    const DWORD old_cull = set_rs(lpd3dd, D3DRS_CULLMODE, D3DCULL_NONE);
    const DWORD old_atest = set_rs(lpd3dd, D3DRS_ALPHATESTENABLE, FALSE);
    const DWORD old_blend = set_rs(lpd3dd, D3DRS_ALPHABLENDENABLE, FALSE);
    const DWORD old_z_enable = set_rs(lpd3dd, D3DRS_ZENABLE, FALSE);
    const DWORD old_z_write = set_rs(lpd3dd, D3DRS_ZWRITEENABLE, FALSE);
    const DWORD old_stencil = set_rs(lpd3dd, D3DRS_STENCILENABLE, FALSE);
    const DWORD old_fog = set_rs(lpd3dd, D3DRS_FOGENABLE, FALSE);
    const DWORD old_specular = set_rs(lpd3dd, D3DRS_SPECULARENABLE, FALSE);
    const DWORD old_zbias = set_rs(lpd3dd, D3DRS_ZBIAS, 0);

    const DWORD old_colorop_0 = set_tss(lpd3dd, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    const DWORD old_colorarg_0 = set_tss(lpd3dd, 0, D3DTSS_COLORARG0, D3DTA_TEXTURE);
    const DWORD old_colorop_1 = set_tss(lpd3dd, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    const DWORD old_mag_filter_0 = set_tss(lpd3dd, 0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    const DWORD old_min_filter_0 = set_tss(lpd3dd, 0, D3DTSS_MINFILTER, D3DTEXF_POINT);

    const float right = static_cast<float>(dxw.GetScreenWidth());
    const float bottom = static_cast<float>(dxw.GetScreenHeight());

    const struct QuadVertex {
        float x, y, z, w;
        float u, v;
    } quad[] = {
        {-0.5f + 0.0f,  -0.5f + 0.0f,   0.5f, 1.0f, 0.0f, 0.0f},
        {-0.5f + right, -0.5f + 0.0f,   0.5f, 1.0f, 1.0f, 0.0f},
        {-0.5f + right, -0.5f + bottom, 0.5f, 1.0f, 1.0f, 1.0f},
        {-0.5f + 0.0f,  -0.5f + bottom, 0.5f, 1.0f, 0.0f, 1.0f},
    };
    lpd3dd->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(QuadVertex));

    // Restore whatever parts of the state are necessary.
    // Currently we do not restore viewport.

    lpd3dd->SetRenderTarget(old_back_buffer, old_depth_buffer);
    lpd3dd->SetTexture(0, old_txt);
    lpd3dd->SetVertexShader(old_vs);

    lpd3dd->SetRenderState(D3DRS_CULLMODE, old_cull);
    lpd3dd->SetRenderState(D3DRS_ALPHATESTENABLE, old_atest);
    lpd3dd->SetRenderState(D3DRS_ALPHABLENDENABLE, old_blend);
    lpd3dd->SetRenderState(D3DRS_ZENABLE, old_z_enable);
    lpd3dd->SetRenderState(D3DRS_ZWRITEENABLE, old_z_write);
    lpd3dd->SetRenderState(D3DRS_STENCILENABLE, old_stencil);
    lpd3dd->SetRenderState(D3DRS_FOGENABLE, old_fog);
    lpd3dd->SetRenderState(D3DRS_SPECULARENABLE, old_specular);
    lpd3dd->SetRenderState(D3DRS_ZBIAS, old_zbias);

    lpd3dd->SetTextureStageState(0, D3DTSS_COLOROP, old_colorop_0);
    lpd3dd->SetTextureStageState(0, D3DTSS_COLORARG0, old_colorarg_0);
    lpd3dd->SetTextureStageState(1, D3DTSS_COLOROP, old_colorop_1);
    lpd3dd->SetTextureStageState(0, D3DTSS_MAGFILTER, old_mag_filter_0);
    lpd3dd->SetTextureStageState(0, D3DTSS_MINFILTER, old_min_filter_0);

    lpd3dd->SetStreamSource(0, old_stream_0, old_stream_0_stride);
    lpd3dd->EndScene();
	return D3D_OK;
}

