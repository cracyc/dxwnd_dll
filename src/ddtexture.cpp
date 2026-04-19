#define  _CRT_SECURE_NO_WARNINGS

#define _MODULE "dxwnd"

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
extern GetColorKey_Type pGetColorKeyMethod(int);
extern SetColorKey_Type pSetColorKeyMethod(int);

extern int Set_dwSize_From_Surface();

#define GRIDSIZE 16

/* RS Hash Function */

static unsigned int Hash(BYTE *buf, int len)
{
   unsigned int b    = 378551;
   unsigned int a    = 63689;
   DWORD hash = 0;
   for(int i = 0; i < len; i++){
      hash = hash * a + buf[i];
      a    = a * b;
   }
   return hash;
}

unsigned int HashSurface(BYTE *buf, int pitch, int width, int height)
{
	unsigned int b    = 378551;
	unsigned int a    = 63689;
	int pixelsize;
	DWORD hash = 0;
	// integer divide, intentionally throwing reminder away
	if (width == 0) return 0; // avoid DivBy0 error
	pixelsize = pitch / width; 
	for(int y = 0; y < height; y++){
		BYTE *p = buf + (y * pitch);
		for(int x = 0; x < width; x++){
			for(int pixelbyte = 0; pixelbyte < pixelsize; pixelbyte++){
				hash = (hash * a) + (*p++);
				a    = a * b;
			}
		}
	}
	return hash;
}

char *SurfaceType(DDPIXELFORMAT ddpfPixelFormat)
{
	static char sSurfaceType[81];
	char sColorType[21];
	DWORD mask;
	int i, count;

	if(ddpfPixelFormat.dwRGBBitCount == 8) return "RGB8";

	strcpy(sSurfaceType, "");
	// red
	mask=ddpfPixelFormat.dwRBitMask;
	for (i=0, count=0; i<32; i++) {
		if(mask & 0x1) count++;
		mask >>= 1;
	}
	sprintf(sColorType, "R%d", count);
	strcat(sSurfaceType, sColorType);
	// green
	mask=ddpfPixelFormat.dwGBitMask;
	for (i=0, count=0; i<32; i++) {
		if(mask & 0x1) count++;
		mask >>= 1;
	}
	sprintf(sColorType, "G%d", count);
	strcat(sSurfaceType, sColorType);
	// blue
	mask=ddpfPixelFormat.dwBBitMask;
	for (i=0, count=0; i<32; i++) {
		if(mask & 0x1) count++;
		mask >>= 1;
	}
	sprintf(sColorType, "B%d", count);
	strcat(sSurfaceType, sColorType);
	// alpha channel
	mask=ddpfPixelFormat.dwRGBAlphaBitMask;
	if(mask){
		for (i=0, count=0; i<32; i++) {
			if(mask & 0x1) count++;
			mask >>= 1;
		}
		sprintf(sColorType, "A%d", count);
		strcat(sSurfaceType, sColorType);
	}
	return sSurfaceType;
}

#if 0
void TextureFix(LPDIRECTDRAWSURFACE s, int dxversion)
{
	DDSURFACEDESC2 ddsd;
	int x, y, w, h;
	HRESULT res;

	OutDebugDW("TextureFix(%d): lpdds=%#x\n", dxversion, s);

	memset(&ddsd,0,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = Set_dwSize_From_Surface();
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	if(res=(*pLockMethod(dxversion))(s, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_WAIT, 0)){	
		OutTraceE("TextureFix(%d): Lock ERROR res=%#x(%s) @%d\n", dxversion, res, ExplainDDError(res), __LINE__);
		return;
	}
	if((ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) && !dxwss.IsABackBufferSurface(s)) {
		OutTraceDW("TextureFix(%d): lpdds=%#x BitCount=%d size=(%dx%d)\n", 
			dxversion, s, ddsd.ddpfPixelFormat.dwRGBBitCount, ddsd.dwWidth, ddsd.dwHeight);
		w = ddsd.dwWidth;
		h = ddsd.dwHeight;
		switch (ddsd.ddpfPixelFormat.dwRGBBitCount){
			case 16: 
				{
					SHORT *p;
					for(y=0; y<h; y++){
						p = (SHORT *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 1);
						for(x=0; x<w; x++) {
							USHORT pixel = *(p+x);
							if(pixel == (USHORT)0xFFFF) *(p+x) = (USHORT)0xF7FF;
						}
					}
				}
				break;
		}
	}
	res=(*pUnlockMethod(dxversion))(s, NULL);
	_if (res) OutTraceE("TextureHigh: Unlock ERROR lpdds=%#x res=%#x(%s) @%d\n", s, res, ExplainDDError(res), __LINE__);
}
#endif

void TextureHighlight(LPDIRECTDRAWSURFACE s, int dxversion)
{
	ApiName("TextureHigh");
	DDSURFACEDESC2 ddsd;
	int x, y, w, h;
	HRESULT res;

	OutDebugDW("%s(%d): lpdds=%#x\n", ApiRef, dxversion, s);

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
			case 8:
				{ 
					BYTE *p;
					BYTE color;
					color=(BYTE)(rand() & 0xFF);
					for(y=0; y<h; y++){
						p = (BYTE *)ddsd.lpSurface + (y * ddsd.lPitch);
						for(x=0; x<w; x++) *(p+x) = color;
					}
					for(y=0; y<h; y++){
						p = (BYTE *)ddsd.lpSurface + (y * ddsd.lPitch);
						for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
						if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
					}
				}
				break;
			case 16: 
				{
					SHORT *p;
					SHORT color;
					color=(SHORT)(rand() & 0x7FFF);
					for(y=0; y<h; y++){
						p = (SHORT *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 1);
						for(x=0; x<w; x++) *(p+x) = color;
					}
					for(y=0; y<h; y++){
						p = (SHORT *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 1);
						for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
						if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
					}
				}
				break;
			case 32: 
				{
					DWORD *p;
					DWORD color;
					color=(DWORD)(rand() & 0xFFFFFFFF);
					for(y=0; y<h; y++){
						p = (DWORD *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 2);
						for(x=0; x<w; x++) *(p+x) = color;
					}
					for(y=0; y<h; y++){
						p = (DWORD *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 2);
						for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
						if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
					}
				}
				break;
		}
	}
	res=(*pUnlockMethod(dxversion))(s, NULL);
	_if (res) OutTraceE("%s: Unlock ERROR lpdds=%#x res=%#x(%s) @%d\n", 
		ApiRef, s, res, ExplainDDError(res), __LINE__);
}

void TextureDump(LPDIRECTDRAWSURFACE s, int dxversion)
{
	ApiName("TextureDump");
	DDSURFACEDESC2 ddsd;
	int w, h, iSurfaceSize, iScanLineSize;
	HRESULT res;
	char pszFile[MAX_PATH];
	char *sExt;
	size_t wlen;

	OutDebugDW("%s(%d): lpdds=%#x\n", ApiRef, dxversion, s);

	memset(&ddsd,0,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = Set_dwSize_From_Surface();
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	if(res=(*pLockMethod(dxversion))(s, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_WAIT, 0)){	
		OutTraceE("%s: Lock ERROR res=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
		return;
	}

	if((ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) && !dxwss.IsABackBufferSurface(s)) while (TRUE) {
		w = ddsd.dwWidth;
		h = ddsd.dwHeight;
		if(!dxw.VerifyTextureLimits(w, h)){
			OutTraceDW("%s: SKIP texture\n", ApiRef);
			break;
		}		
		
		if((ddsd.lPitch == 0) || (ddsd.dwHeight == 0)) {
			OutTraceDW("%s: SKIP void texture\n", ApiRef);
			break;
		}

		iSurfaceSize = ddsd.dwHeight * ddsd.lPitch;

		FILE *hf;
		BITMAPFILEHEADER hdr;       // bitmap file-header 
		BITMAPV4HEADER pbi;			// bitmap info-header  

		memset((void *)&pbi, 0, sizeof(BITMAPV4HEADER));
		pbi.bV4Size = sizeof(BITMAPV4HEADER); 
		pbi.bV4Width = ddsd.dwWidth;
		pbi.bV4Height = ddsd.dwHeight;
		pbi.bV4BitCount = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
		pbi.bV4SizeImage = ((pbi.bV4Width * pbi.bV4BitCount + 0x1F) & ~0x1F)/8 * pbi.bV4Height; 
		pbi.bV4Height = - pbi.bV4Height;
		pbi.bV4Planes = 1;
		pbi.bV4V4Compression = BI_BITFIELDS;
		if(pbi.bV4BitCount == 8) pbi.bV4V4Compression = BI_RGB;
		pbi.bV4XPelsPerMeter = 1;
		pbi.bV4YPelsPerMeter = 1;
		pbi.bV4ClrUsed = 0;
		if(pbi.bV4BitCount == 8) pbi.bV4ClrUsed = 256;
		pbi.bV4ClrImportant = 0;
		pbi.bV4RedMask = ddsd.ddpfPixelFormat.dwRBitMask;
		pbi.bV4GreenMask = ddsd.ddpfPixelFormat.dwGBitMask;
		pbi.bV4BlueMask = ddsd.ddpfPixelFormat.dwBBitMask;
		pbi.bV4AlphaMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
		pbi.bV4CSType = LCS_CALIBRATED_RGB;
		iScanLineSize = ((pbi.bV4Width * pbi.bV4BitCount + 0x1F) & ~0x1F)/8;

		// calculate the bitmap hash
		DWORD hash;
		hash = HashSurface((BYTE *)ddsd.lpSurface, ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight); 
		if(!hash) {
			OutTraceDW("%s: lpdds=%#x hash=NULL\n", ApiRef, s); 
			break; // almost certainly, an empty black surface!
		}

		OutTraceDW("%s(%d): lpdds=%#x hash=%08.8X size=(%dx%d) bits=%d mask(ARGB)=%#x.%#x.%#x.%#x\n", 
			ApiRef, dxversion, s, hash,
			ddsd.dwWidth, ddsd.dwHeight, ddsd.ddpfPixelFormat.dwRGBBitCount,
			ddsd.ddpfPixelFormat.dwRGBAlphaBitMask, 
			ddsd.ddpfPixelFormat.dwRBitMask,
			ddsd.ddpfPixelFormat.dwGBitMask,
			ddsd.ddpfPixelFormat.dwBBitMask
			);

		// Create the .BMP file. 
		switch (dxw.iTextureFileFormat){
			case FORMAT_BMP: sExt = "bmp"; break; 
			case FORMAT_RAW: sExt = "raw"; break; 
			case FORMAT_DDS: sExt = "dds"; break; 
		}
		sprintf_s(pszFile, MAX_PATH, "%s\\texture.out\\texture.%03d.%03d.%s.%08X.%s", 
			GetDxWndPath(), ddsd.dwWidth, ddsd.dwHeight, SurfaceType(ddsd.ddpfPixelFormat), hash, sExt);
		hf = fopen(pszFile, "wb");
		if(!hf) break;

		switch(dxw.iTextureFileFormat){

			case FORMAT_RAW:

				wlen = fwrite((BYTE *)ddsd.lpSurface, ddsd.lPitch * ddsd.dwHeight, 1, hf);
				_if(wlen != 1) OutTraceE("%s: fwrite ERROR err=%d\n", ApiRef, GetLastError());
				break;

			case FORMAT_DDS: {

				// no good for 8bpp textured bitmaps !!!
				DDS_HEADER ddsh;
				wlen = fwrite("DDS ", 4, 1, hf);
				_if(wlen != 1) OutTraceE("%s: fwrite ERROR err=%d\n", ApiRef, GetLastError());
				memset(&ddsh, 0, sizeof(ddsh));
				ddsh.dwSize = sizeof(ddsh);
				ddsh.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH;
				ddsh.dwHeight = ddsd.dwHeight;
				ddsh.dwWidth = ddsd.dwWidth;
				ddsh.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
				ddsh.ddspf.dwFlags = DDPF_RGB;			
				ddsh.dwPitchOrLinearSize = (DWORD)ddsd.lPitch;
				ddsh.ddspf.dwABitMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
				ddsh.ddspf.dwRBitMask = ddsd.ddpfPixelFormat.dwRBitMask;
				ddsh.ddspf.dwGBitMask = ddsd.ddpfPixelFormat.dwGBitMask;
				ddsh.ddspf.dwBBitMask = ddsd.ddpfPixelFormat.dwBBitMask;
				ddsh.ddspf.dwRGBBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
				wlen = fwrite((BYTE *)&ddsh, sizeof(ddsh), 1, hf);
				_if(wlen != 1) OutTraceE("%s: fwrite ERROR err=%d\n", ApiRef, GetLastError());
				wlen = fwrite((BYTE *)ddsd.lpSurface, ddsd.lPitch * ddsd.dwHeight, 1, hf);
				_if(wlen != 1) OutTraceE("%s: fwrite ERROR err=%d\n", ApiRef, GetLastError());
				}
				break;

			case FORMAT_BMP:

				hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
				// Compute the size of the entire file.  
				hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof(RGBQUAD) + pbi.bV4SizeImage); 
				hdr.bfReserved1 = 0; 
				hdr.bfReserved2 = 0; 

				// Compute the offset to the array of color indices.  
				hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof (RGBQUAD); 

				// Copy the BITMAPFILEHEADER into the .BMP file.  
				fwrite((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, hf);

				// Copy the BITMAPINFOHEADER array into the file.  
				fwrite((LPVOID)&pbi, sizeof(BITMAPV4HEADER), 1, hf);

				// Copy the RGBQUAD array into the file.  
				if(pbi.bV4ClrUsed){
					LPDIRECTDRAWPALETTE pal;
					PALETTEENTRY pe[256];
					RGBQUAD q[256];
					HRESULT res2;
					res2 = s->GetPalette(&pal);
					if(res2){
						extern DWORD PaletteEntries[256];
						fwrite((LPVOID)PaletteEntries, pbi.bV4ClrUsed * sizeof (RGBQUAD), 1, hf);
					}
					else {
						extern ReleaseP_Type pReleaseP;
						pal->GetEntries(0, 0, 256, pe);
						//pal->Release();
						(*pReleaseP)(pal);
						for(UINT i=0; i<pbi.bV4ClrUsed; i++) {
							q[i].rgbReserved = 0;
							q[i].rgbRed = pe[i].peRed;
							q[i].rgbGreen = pe[i].peGreen;
							q[i].rgbBlue = pe[i].peBlue;
						}
						fwrite((LPVOID)q, pbi.bV4ClrUsed * sizeof (RGBQUAD), 1, hf);
					}
				}

				// Copy the array of color indices into the .BMP file.  
				for(int y=0; y<(int)ddsd.dwHeight; y++)
					fwrite((BYTE *)ddsd.lpSurface + (y*ddsd.lPitch), iScanLineSize, 1, hf);
				break;
		}

		// Close the .BMP file.  
		fclose(hf);

		break;
	}

	res=(*pUnlockMethod(dxversion))(s, NULL);
	_if (res) OutTraceE("%s: Unlock ERROR lpdds=%#x res=%#x(%s) @%d\n", ApiRef, s, res, ExplainDDError(res), __LINE__);
}

static void TextureHack(LPDIRECTDRAWSURFACE s, int dxversion)
{
	ApiName("TextureHack");
	DDSURFACEDESC2 ddsd;
	int w, h, iSurfaceSize, iScanLineSize;
	HRESULT res;
	char *sExt;
	size_t rlen;

	OutDebugDW("%s(%d): lpdds=%#x\n", ApiRef, dxversion, s);

	memset(&ddsd,0,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = Set_dwSize_From_Surface();
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	if(res=(*pLockMethod(dxversion))(s, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_WAIT, 0)){	
		OutTraceE("%s: Lock ERROR res=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
		return;
	}
	if((ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) && !dxwss.IsABackBufferSurface(s)) while (TRUE) { // fake loop to ensure final Unlock
		OutTraceDW("%s(%d): lpdds=%#x BitCount=%d size=(%dx%d) surface=%#x\n", 
			ApiRef, dxversion, s, ddsd.ddpfPixelFormat.dwRGBBitCount, ddsd.dwWidth, ddsd.dwHeight, ddsd.lpSurface);
		w = ddsd.dwWidth;
		h = ddsd.dwHeight;

		if(!dxw.VerifyTextureLimits(w, h)){
			OutTraceDW("%s: SKIP texture\n", ApiRef);
			break;
		}		

		iSurfaceSize = ddsd.dwHeight * ddsd.lPitch;

		FILE *hf;
		BITMAPFILEHEADER hdr;       // bitmap file-header 
		BITMAPINFOHEADER pbi;		// bitmap info-header  
		char pszFile[MAX_PATH];
		int iSizeImage;

		// calculate the bitmap hash
		DWORD hash;
		hash = HashSurface((BYTE *)ddsd.lpSurface, ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight); 
		if(!hash) break; // almost certainly, an empty black surface!

		// Look for the .BMP file. 
		switch (dxw.iTextureFileFormat){
			case FORMAT_BMP: sExt = "bmp"; break; 
			case FORMAT_RAW: sExt = "raw"; break; 
			case FORMAT_DDS: sExt = "dds"; break; 
		}
		sprintf_s(pszFile, MAX_PATH, "%s\\texture.in\\texture.%03d.%03d.%s.%08X.%s", 
			GetDxWndPath(), ddsd.dwWidth, ddsd.dwHeight, SurfaceType(ddsd.ddpfPixelFormat), hash, sExt);
		OutDebugDW("%s: searching for path=%s\n", ApiRef, pszFile);

		hf = fopen(pszFile, "rb");
		if(!hf) break; // no updated texture to load

		OutTraceDW("%s: IMPORT path=%s\n", ApiRef, pszFile);

		switch(dxw.iTextureFileFormat){

			case FORMAT_RAW: {

				rlen = fread((BYTE *)ddsd.lpSurface, ddsd.lPitch * ddsd.dwHeight, 1, hf);
				_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
				}
				break;

			case FORMAT_DDS: {

				BYTE magic[4];
				DDS_HEADER ddsh;
				// assume the file is sane, read and throw away magic and dds header
				rlen = fread(magic, 4, 1, hf);
				_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
				rlen = fread((BYTE *)&ddsh, sizeof(ddsh), 1, hf);
				_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
				memset(&ddsh, 0, sizeof(ddsh));
				rlen = fread((BYTE *)ddsd.lpSurface, ddsd.lPitch * ddsd.dwHeight, 1, hf);
				_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			}
			break;

			case FORMAT_BMP: 

				memset((void *)&pbi, 0, sizeof(BITMAPINFOHEADER));
				pbi.biSize = sizeof(BITMAPINFOHEADER); 
				pbi.biWidth = ddsd.dwWidth;
				pbi.biHeight = ddsd.dwHeight;
				pbi.biBitCount = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
				pbi.biSizeImage = ((pbi.biWidth * pbi.biBitCount + 0x1F) & ~0x1F)/8 * pbi.biHeight; 
				iSizeImage = pbi.biSizeImage;
				iScanLineSize = ((pbi.biWidth * pbi.biBitCount + 0x1F) & ~0x1F)/8;

				while(TRUE) { // fake loop to ensure final fclose
					// Read the BITMAPFILEHEADER from the .BMP file (and throw away ...).  
					if(fread((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, hf) != 1)break;

					// Read the BITMAPINFOHEADER (and throw away ...).  
					// If the file contains BITMAPV4HEADER or BITMAPV5HEADER, no problem: next fseek will settle things
					if(fread((LPVOID)&pbi, sizeof(BITMAPINFOHEADER), 1, hf) != 1) break;

					// skip the RGBQUAD array if the editor inserted one
					fseek(hf, hdr.bfOffBits, SEEK_SET);

					// Read the new texture  from the .BMP file.  
					if(pbi.biHeight < 0){
						// biHeight < 0 -> scan lines from top to bottom, same as surface/texture convention
						for(int y=0; y<(int)ddsd.dwHeight; y++){
							BYTE *p = (BYTE *)ddsd.lpSurface + (ddsd.lPitch * y);
							fseek(hf, hdr.bfOffBits + (iScanLineSize * y), SEEK_SET);
							if(fread((LPVOID)p, ddsd.lPitch, 1, hf) != 1) {
								OutTraceDW("%s: TEXTURE LOAD BROKEN err=%d\n", ApiRef, GetLastError());
								break;
							}
						}
					}
					else {
						// biHeight > 0 -> scan lines from bottom to top, inverse order as surface/texture convention
						for(int y=0; y<(int)ddsd.dwHeight; y++){
							BYTE *p = (BYTE *)ddsd.lpSurface + (ddsd.lPitch * ((ddsd.dwHeight-1) - y));
							fseek(hf, hdr.bfOffBits + (iScanLineSize * y), SEEK_SET);
							if(fread((LPVOID)p, ddsd.lPitch, 1, hf) != 1) {
								OutTraceDW("%s: TEXTURE LOAD BROKEN err=%d\n", ApiRef, GetLastError());
								break;
							}
						}
					}
					OutTraceDW("%s: TEXTURE LOAD DONE\n", ApiRef);
					break;
				}
			break;
		}
		// Close the .BMP file.  
		fclose(hf);
		break;
	}
	res=(*pUnlockMethod(dxversion))(s, NULL);
	_if (res) OutTraceE("%s: Unlock ERROR lpdds=%#x res=%#x(%s) @%d\n", ApiRef, s, res, ExplainDDError(res), __LINE__);
}

void TextureTransp(LPDIRECTDRAWSURFACE s, int dxversion)
{
	ApiName("TextureTransp");
	DDSURFACEDESC2 ddsd;
	int x, y, w, h;
	HRESULT res;

	OutDebugDW("%s(%d): lpdds=%#x\n", ApiRef, dxversion, s);

	DDCOLORKEY ck;
	BOOL hasColorKey = FALSE;
	DWORD ckh;
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = Set_dwSize_From_Surface();
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	if(res=(*pLockMethod(dxversion))(s, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_READONLY|DDLOCK_WAIT, 0)){	
		OutTraceE("%s(%d): Lock ERROR res=%#x(%s) @%d\n", ApiRef, dxversion, res, ExplainDDError(res), __LINE__);
		return;
	}
	if((ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) && !dxwss.IsABackBufferSurface(s)) {
		if(res=(*pGetColorKeyMethod(dxversion))(s, DDCKEY_SRCBLT, &ck)){
			OutTrace("%s(%d): GetColorKey DDCKEY_SRCBLT ERROR res=%#x(%s) @%d\n", ApiRef, dxversion, res, ExplainDDError(res), __LINE__);
			switch(ddsd.ddpfPixelFormat.dwRGBBitCount){
				case 8:
					ckh = 0; // to do ....
					break;
				case 16:
					ckh = 0xF81F; // 16 bit RGB565 magenta
					break;
				case 32:
					ckh = 0xFF00FF; // 32 bit RGB888 magenta
					break;
			}
			ck.dwColorSpaceHighValue = ck.dwColorSpaceLowValue = ckh;
			res=(*pSetColorKeyMethod(dxversion))(s, DDCKEY_SRCBLT, &ck);
			if(res){
				OutTrace("%s: SetColorKey ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res)); 
				hasColorKey = FALSE;
			} else hasColorKey = TRUE;
		}
		else {
			hasColorKey = TRUE;
			OutTrace("%s: lpdds=%#x DDCKEY_SRCBLT colorkey=%#x-%#x\n", ApiRef, s, ck.dwColorSpaceLowValue, ck.dwColorSpaceHighValue);
			ckh = ck.dwColorSpaceHighValue;
		}
		OutTraceDW("%s(%d): lpdds=%#x BitCount=%d size=(%dx%d)\n", 
			ApiRef, dxversion, s, ddsd.ddpfPixelFormat.dwRGBBitCount, ddsd.dwWidth, ddsd.dwHeight);
		w = ddsd.dwWidth;
		h = ddsd.dwHeight;
		if(!dxw.VerifyTextureLimits(w, h)){
			OutTraceDW("%s: SKIP texture\n", ApiRef);
		}
		else {
			switch (ddsd.ddpfPixelFormat.dwRGBBitCount){
				case 8:
					if(hasColorKey) {
						BYTE *p;
						BYTE color = ckh & 0xFF; 
						for(y=0; y<h; y++){
							p = (BYTE *)ddsd.lpSurface + (y * ddsd.lPitch);
							if(y & 0x1)
								for(x=0; x<w; x+=2) *(p+x) = color;
							else
								for(x=1; x<w; x+=2) *(p+x) = color;
						}
					}
					break;
				case 16: 
					if(hasColorKey) {
						WORD *p;
						WORD color = ckh & 0xFFFF; 
						for(y=0; y<h; y++){
							p = (WORD *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 1);
							if(y & 0x1)
								for(x=0; x<w; x+=2) *(p+x) = color;
							else
								for(x=1; x<w; x+=2) *(p+x) = color;
						}
					}
					break;
				case 32: 
					if(hasColorKey) {
						DWORD *p;
						DWORD color = ckh & 0xFFFFFF; 
						for(y=0; y<h; y++){
							p = (DWORD *)ddsd.lpSurface + ((y * ddsd.lPitch) >> 2);
							if(y & 0x1)
								for(x=0; x<w; x+=2) *(p+x) = color;
							else
								for(x=1; x<w; x+=2) *(p+x) = color;
						}
					}
					break;
			}
		}
	}
	res=(*pUnlockMethod(dxversion))(s, NULL);
	_if (res) OutTraceE("%s: Unlock ERROR lpdds=%#x res=%#x(%s) @%d\n", ApiRef, s, res, ExplainDDError(res), __LINE__);
}

void TextureHandling(LPDIRECTDRAWSURFACE s, int dxversion)
{
	//OutTraceDW("TextureHandling(1-7): dxw.dwFlags5 = %#x\n", dxw.dwFlags5 & (TEXTUREHIGHLIGHT|TEXTUREDUMP|TEXTUREHACK));
	switch(dxw.dwFlags5 & TEXTUREMASK){
		default:
		case TEXTUREHIGHLIGHT: 
			TextureHighlight(s, dxversion);
			break;
		case TEXTUREDUMP: 
			TextureDump(s, dxversion);
			break;
		case TEXTUREHACK:
			TextureHack(s, dxversion);
			break;
		case TEXTURETRANSP:
			// commented and moved into QueryInterface wrapper
			//TextureTransp(s, dxversion);
			break;
	}
}

#if 0
static LPVOID lpDiffBuffer = NULL;

void SurfaceDiffPush(int dxversion, LPDDSURFACEDESC lpddsdesc)
{
	int size;

	OutTraceDW("SurfaceDiffPush(%d): lpddsdesc=%#x\n", dxversion, lpddsdesc);

	if(lpDiffBuffer) {
		free(lpDiffBuffer);
		lpDiffBuffer = NULL;
	}
	size = lpddsdesc->dwHeight * lpddsdesc->lPitch;
	if(size && lpddsdesc->lpSurface) {
		lpDiffBuffer = malloc(size + 20);
		if(!lpDiffBuffer) return;
		memcpy(lpDiffBuffer, lpddsdesc->lpSurface, size);
		OutTraceDW("SurfaceDiffPush: size=(%dx%d) pitch=%d bufsize=%d\n", 
			lpddsdesc->dwWidth, lpddsdesc->dwHeight, lpddsdesc->lPitch, size);
	}
}

void SurfaceDiffDump(int dxversion, LPDDSURFACEDESC lpddsdesc)
{
	int w, h, iSurfaceSize, iScanLineSize;
	static int MinTexX, MinTexY, MaxTexX, MaxTexY;
	static BOOL DoOnce = TRUE;
	char pszFile[MAX_PATH];

	OutTraceDW("SurfaceDiffDump(%d): lpddsdesc=%#x\n", dxversion, lpddsdesc);

	if(!lpDiffBuffer) return;

	if(DoOnce){
		sprintf_s(pszFile, MAX_PATH, "%s\\diff.out", GetDxWndPath());
		CreateDirectory(pszFile, NULL);
		DoOnce = FALSE;
	}

	//if(!dxwss.IsABackBufferSurface(s)) while (TRUE) {
	while (TRUE) { // fake loop
		OutTraceDW("SurfaceDiffDump(%d): BitCount=%d size=(%dx%d)\n", 
			dxversion, lpddsdesc->ddpfPixelFormat.dwRGBBitCount, lpddsdesc->dwWidth, lpddsdesc->dwHeight);
		w = lpddsdesc->dwWidth;
		h = lpddsdesc->dwHeight;

		iSurfaceSize = lpddsdesc->dwHeight * lpddsdesc->lPitch;

		FILE *hf;
		BITMAPFILEHEADER hdr;       // bitmap file-header 
		BITMAPV4HEADER pbi;			// bitmap info-header  

		memset((void *)&pbi, 0, sizeof(BITMAPV4HEADER));
		pbi.bV4Size = sizeof(BITMAPV4HEADER); 
		pbi.bV4Width = lpddsdesc->dwWidth;
		pbi.bV4Height = lpddsdesc->dwHeight;
		pbi.bV4BitCount = (WORD)lpddsdesc->ddpfPixelFormat.dwRGBBitCount;
		pbi.bV4SizeImage = ((pbi.bV4Width * pbi.bV4BitCount + 0x1F) & ~0x1F)/8 * pbi.bV4Height; 
		pbi.bV4Height = - pbi.bV4Height;
		pbi.bV4Planes = 1;
		pbi.bV4V4Compression = BI_BITFIELDS;
		if(pbi.bV4BitCount == 8) pbi.bV4V4Compression = BI_RGB;
		pbi.bV4XPelsPerMeter = 1;
		pbi.bV4YPelsPerMeter = 1;
		pbi.bV4ClrUsed = 0;
		if(pbi.bV4BitCount == 8) pbi.bV4ClrUsed = 256;
		pbi.bV4ClrImportant = 0;
		pbi.bV4RedMask = lpddsdesc->ddpfPixelFormat.dwRBitMask;
		pbi.bV4GreenMask = lpddsdesc->ddpfPixelFormat.dwGBitMask;
		pbi.bV4BlueMask = lpddsdesc->ddpfPixelFormat.dwBBitMask;
		pbi.bV4AlphaMask = lpddsdesc->ddpfPixelFormat.dwRGBAlphaBitMask;
		pbi.bV4CSType = LCS_CALIBRATED_RGB;
		iScanLineSize = ((pbi.bV4Width * pbi.bV4BitCount + 0x1F) & ~0x1F)/8;

		// Calculate the differences. 
		switch(lpddsdesc->ddpfPixelFormat.dwRGBBitCount){
			case 8:
				for(int y=0; y<(int)lpddsdesc->dwHeight; y++){
					LPBYTE p1 = (LPBYTE)lpDiffBuffer + (y * lpddsdesc->lPitch);
					LPBYTE p2 = (LPBYTE)lpddsdesc->lpSurface + (y * lpddsdesc->lPitch);
					for(int x=0; x<(int)lpddsdesc->dwWidth; x++, p1++, p2++){
						if(*p1 == *p2) *p1 = 0;
					}
				}
				break;
			case 16:
				for(int y=0; y<(int)lpddsdesc->dwHeight; y++){
					LPWORD p1 = (LPWORD)lpDiffBuffer + ((y * lpddsdesc->lPitch) >> 1);
					LPWORD p2 = (LPWORD)lpddsdesc->lpSurface + ((y * lpddsdesc->lPitch) >> 1);
					for(int x=0; x<(int)lpddsdesc->dwWidth; x++, p1++, p2++){
						if(*p1 == *p2) *p1 = 0;
					}
				}
				break;
			case 32:
				for(int y=0; y<(int)lpddsdesc->dwHeight; y++){
					LPDWORD p1 = (LPDWORD)lpDiffBuffer + ((y * lpddsdesc->lPitch) >> 2);
					LPDWORD p2 = (LPDWORD)lpddsdesc->lpSurface + ((y * lpddsdesc->lPitch) >> 2);
					for(int x=0; x<(int)lpddsdesc->dwWidth; x++, p1++, p2++){
						if(*p1 == *p2) *p1 = 0;
					}
				}
				break;
		}

		// calculate the bitmap hash
		DWORD hash;
		hash = HashSurface((BYTE *)lpDiffBuffer, lpddsdesc->lPitch, lpddsdesc->dwWidth, lpddsdesc->dwHeight); 
		if(!hash) {
			OutTraceDW("SurfaceDiffDump: hash=NULL\n"); 
			break; // almost certainly, an empty black surface!
		}
		OutTraceDW("SurfaceDiffDump: hash=%#x\n", hash); 

		// Create the .BMP file. 
		sprintf_s(pszFile, MAX_PATH, "%s\\diff.out\\texture.%03d.%03d.%s.%08X.bmp", 
			GetDxWndPath(), lpddsdesc->dwWidth, lpddsdesc->dwHeight, SurfaceType(lpddsdesc->ddpfPixelFormat), hash);
		hf = fopen(pszFile, "wb");
		if(!hf) {
			OutTraceDW("SurfaceDiffDump: fopen \"%s\" failed error %d\n", pszFile, GetLastError());
			break;
		}

		hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
		// Compute the size of the entire file.  
		hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof(RGBQUAD) + pbi.bV4SizeImage); 
		hdr.bfReserved1 = 0; 
		hdr.bfReserved2 = 0; 

		// Compute the offset to the array of color indices.  
		hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof (RGBQUAD); 

		// Copy the BITMAPFILEHEADER into the .BMP file.  
		fwrite((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, hf);

		// Copy the BITMAPINFOHEADER array into the file.  
		fwrite((LPVOID)&pbi, sizeof(BITMAPV4HEADER), 1, hf);

		// Copy the RGBQUAD array into the file.  
		if(pbi.bV4ClrUsed){
			extern DWORD PaletteEntries[256];
			fwrite((LPVOID)PaletteEntries, pbi.bV4ClrUsed * sizeof (RGBQUAD), 1, hf);
		}

		// Copy the array of color indices into the .BMP file. 
		for(int y=0; y<(int)lpddsdesc->dwHeight; y++){
			fwrite((LPBYTE)lpDiffBuffer + (y * lpddsdesc->lPitch), iScanLineSize, 1, hf);
		}

		// Close the .BMP file.  
		fclose(hf);
		break;
	}

	free(lpDiffBuffer);
	lpDiffBuffer = NULL;
}
#endif

