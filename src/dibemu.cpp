#define _CRT_SECURE_NO_WARNINGS
#define _COMPONENT "dxwCore"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "hddraw.h"
#include "dxhook.h"
#include "dxhelper.h"

#include "stdio.h"

void dxwCore::InitPalette(){
	ApiName("InitPalette");
	OutTraceGDI("%s\n", ApiRef);
	if(DIBRGBQuadEntries == NULL) DIBRGBQuadEntries = (RGBQUAD *)malloc(256 * sizeof(RGBQUAD));
	extern PALETTEENTRY DefaultSystemPalette[256];
	for(int i=0; i<256; i++){
		DIBRGBQuadEntries[i].rgbBlue=DefaultSystemPalette[i].peBlue; 
		DIBRGBQuadEntries[i].rgbRed=DefaultSystemPalette[i].peRed; 
		DIBRGBQuadEntries[i].rgbGreen=DefaultSystemPalette[i].peGreen; 
		DIBRGBQuadEntries[i].rgbReserved = 0;
	}
}

UINT dxwCore::SetDIBColors(HDC hdc, UINT uStartIndex, UINT cEntries, const RGBQUAD *pColors)
{
	ApiName("SetDIBColors");
	OutTraceGDI("%s: start=%d entries=%d\n", ApiRef, uStartIndex, cEntries);
	if(DIBRGBQuadEntries == NULL) InitPalette();

	if((uStartIndex+cEntries) > 256) cEntries = 256 - uStartIndex; // trim color number if exceeding size
	memcpy(&DIBRGBQuadEntries[uStartIndex], pColors, cEntries * sizeof(RGBQUAD));
	return cEntries;
}

UINT dxwCore::SetDIBColors(CONST LOGPALETTE *plpal)
{
	ApiName("SetDIBColors");
	OutTraceGDI("%s\n", ApiRef);
	if(DIBRGBQuadEntries == NULL) InitPalette();

	for(UINT i = 0; i < plpal->palNumEntries; i++){
		DIBRGBQuadEntries[i].rgbRed = plpal->palPalEntry[i].peRed;
		DIBRGBQuadEntries[i].rgbGreen = plpal->palPalEntry[i].peGreen;
		DIBRGBQuadEntries[i].rgbBlue = plpal->palPalEntry[i].peBlue;
		DIBRGBQuadEntries[i].rgbReserved = 0;
	}
	return plpal->palNumEntries;
}

UINT dxwCore::GetDIBColors(HDC hdc, UINT uStartIndex, UINT cEntries, const RGBQUAD *pColors)
{
	ApiName("GetDIBColors");
	OutTraceGDI("%s: start=%d entries=%d\n", ApiRef, uStartIndex, cEntries);
	if(DIBRGBQuadEntries == NULL) InitPalette(); // in case of malloc failure ....
	if((uStartIndex+cEntries) > 256) cEntries = 256 - uStartIndex; // trim color number if exceeding size
	memcpy((LPVOID)pColors, &DIBRGBQuadEntries[uStartIndex], cEntries * sizeof(RGBQUAD));
	return cEntries;
}

LPVOID dxwCore::EmulateDIB(LPVOID lpvBits, const BITMAPINFO *lpbmiIn, BITMAPINFO *lpbmiOut, UINT fuColorUse)
{
	ApiName("EmulateDIB");
	OutTraceGDI("%s\n", ApiRef);
	LPVOID lpvNewBits;
	RGBQUAD *quad;
	LPDWORD pixel32;
	LPBYTE pixel8;
	RGBQUAD QPalette[256];
	int w, h, pitch;

	if(DIBRGBQuadEntries == NULL) InitPalette(); 
	w = lpbmiIn->bmiHeader.biWidth;
	h = lpbmiIn->bmiHeader.biHeight;
	pitch = ((w + 3) >> 2) << 2;
	if(h < 0) h = -h;
	lpvNewBits = malloc(pitch * h * sizeof(DWORD));

	// v2.04.94: copy ddraw virtual palette to DIBRGBQuadEntries
	// the operation must be repeated for each frame because ddraw palette could have
	// been changed in meanwhile. Fixes "iM1A2 Abrams".
	if(fuColorUse == DIB_RGB_COLORS){
		quad = (RGBQUAD *)lpbmiIn->bmiColors;
	}
	else {
		WORD *indexes = (WORD *)(lpbmiIn->bmiColors);
		for(int i=0; i < 256; i++){
			int index = indexes[i] % 256;
			QPalette[i] = DIBRGBQuadEntries[index];
		}
		quad = QPalette;
	}

	memcpy(lpbmiOut, lpbmiIn, sizeof(BITMAPINFO));
	lpbmiOut->bmiHeader.biClrImportant = 0;
	lpbmiOut->bmiHeader.biClrUsed = 0;
	lpbmiOut->bmiHeader.biCompression = 0;
	lpbmiOut->bmiHeader.biSizeImage = 0;
	lpbmiOut->bmiHeader.biBitCount = 32;
	pixel32 = (DWORD *)lpvNewBits;
	pixel8 = (LPBYTE)lpvBits;
	for (int y=0; y<h; y++){
		for (int x=0; x<w; x++){
			RGBQUAD q = quad[*pixel8++]; 
			*pixel32++ = (q.rgbRed << 16) | (q.rgbGreen << 8) | (q.rgbBlue << 0);
		}
		pixel8 = &((LPBYTE)lpvBits)[y * pitch];
		pixel32 = &((LPDWORD)lpvNewBits)[y * w];
	}
	return lpvNewBits;
}
