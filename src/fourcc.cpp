#define _CRT_SECURE_NO_WARNINGS
#define INITGUID
#define _MODULE "dxwnd"

#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

static float clip( float n, float lower, float upper)
{
    n = ( n > lower ) * n + !( n > lower ) * lower;
    return ( n < upper ) * n + !( n < upper ) * upper;
}


LPVOID dxwCopyPixels(LPDDSURFACEDESC2 lpddsd)
{
	LPBYTE yuy2buf;
	DWORD y;
	yuy2buf = (LPBYTE)malloc(lpddsd->dwHeight * lpddsd->lPitch);
	LPBYTE psrc = (LPBYTE)lpddsd->lpSurface;
	LPBYTE pdst = yuy2buf;
	for(y=0; y<lpddsd->dwHeight; y++){
		memcpy((LPVOID)pdst, (LPVOID)psrc, lpddsd->lPitch);
		psrc += lpddsd->lPitch;
		pdst += lpddsd->lPitch;
	}
	return yuy2buf;
}

LPVOID dxwConvert16to32(LPDDSURFACEDESC2 lpddsd)
{
	ApiName("dxwConvert16to32");
	OutTrace("%s: begin\n", ApiRef);
	LPBYTE yuy2buf;
	DWORD x, y, dwGap;
	yuy2buf = (LPBYTE)malloc(lpddsd->dwHeight * lpddsd->lPitch * (sizeof(DWORD) / 2));
	LPWORD psrc = (LPWORD)lpddsd->lpSurface;
	LPDWORD pdst = (LPDWORD)yuy2buf;
	dwGap = (lpddsd->lPitch >> 1) - lpddsd->dwWidth;
	if (lpddsd->ddpfPixelFormat.dwGBitMask == 0x7E0){
		for(y=0; y<lpddsd->dwHeight; y++){
			for(x=0; x < lpddsd->dwWidth; x++){
				WORD src = *psrc++;
				*pdst++ =(src & 0x1F)<<3 | (src & 0x7E0)<<5 | (src & 0xF800)<<8; // RGB565
			}
		}
		psrc += (dwGap << 1);
		pdst += dwGap;
	}
	else {
		for(y=0; y<lpddsd->dwHeight; y++){
			for(x=0; x < lpddsd->dwWidth; x++){
				WORD src = *psrc++;
				*pdst++ =(src & 0x1F)<<3 | (src & 0x3E0)<<6 | (src & 0x7C00)<<9; // RGB555
			}
		}
		psrc += (dwGap << 1);
		pdst += dwGap;
	}
	OutTrace("%s: end\n", ApiRef);
	return yuy2buf;
}

LPVOID dxwConvertYUY216(LPDDSURFACEDESC2 lpddsd)
{
	// see https://msdn.microsoft.com/en-us/library/windows/desktop/dd206750(v=vs.85).aspx
	LPBYTE yuy2buf;
	DWORD x, y, dwGap;
	int y0, y1, u, v;
	WORD r, g, b;
	yuy2buf = (LPBYTE)malloc(lpddsd->dwHeight * lpddsd->lPitch);
	LPBYTE psrc = (LPBYTE)lpddsd->lpSurface;
	LPWORD pdst = (LPWORD)yuy2buf;
	dwGap = (lpddsd->lPitch >> 1) - lpddsd->dwWidth;
	for(y=0; y<lpddsd->dwHeight; y++){
		for(x=0; x < (lpddsd->dwWidth>>1); x++){
			y0 = (int)*psrc++ -16;
			u  = (int)*psrc++ -128;
			y1 = (int)*psrc++ -16;
			v  = (int)*psrc++ -128;
			// unoptimized, floating point calculation
			b  = (WORD)clip((float)(y0 +                 ( 1.140 * v )), 0, 255);
			g  = (WORD)clip((float)(y0 - ( 0.395 * u ) - ( 0.581 * v )), 0, 255);
			r  = (WORD)clip((float)(y0 + ( 2.032 * u )                ), 0, 255);
			// R5G6B5 conversion
			*pdst++ = ((r >> 3) & 0x1F) | (((g >> 2) & 0x3f)<<5) | (((b >> 3) & 0x1f)<<11);
			// unoptimized, floating point calculation
			b  = (WORD)clip((float)(y1 +                 ( 1.140 * v )), 0, 255);
			g  = (WORD)clip((float)(y1 - ( 0.395 * u ) - ( 0.581 * v )), 0, 255);
			r  = (WORD)clip((float)(y1 + ( 2.032 * u )                ), 0, 255);
			// R5G6B5 conversion
			*pdst++ = ((r >> 3) & 0x1F) | (((g >> 2) & 0x3f)<<5) | (((b >> 3) & 0x1f)<<11);
		}
		psrc += (dwGap << 1);
		pdst += dwGap;
	}
	return yuy2buf;
}

LPVOID dxwConvertYUY232(LPDDSURFACEDESC2 lpddsd)
{
	//OutTrace("dxwConvertYUY232\n");
	LPBYTE yuy2buf;
	DWORD x, y, dwGap;
	int y0, y1, u, v;
	WORD r, g, b;
	yuy2buf = (LPBYTE)malloc(lpddsd->dwHeight * lpddsd->lPitch * (sizeof(DWORD) / 2));
	//OutTrace("w=%d h=%d pitch=%d\n", lpddsd->dwWidth, lpddsd->dwHeight, lpddsd->lPitch);
	////yuy2buf = (LPBYTE)malloc(lpddsd->dwHeight * lpddsd->dwWidth * sizeof(DWORD));
	LPBYTE psrc = (LPBYTE)lpddsd->lpSurface;
	LPDWORD pdst = (LPDWORD)yuy2buf;
	dwGap = (lpddsd->lPitch >> 1) - lpddsd->dwWidth;
	for(y=0; y<lpddsd->dwHeight; y++){
		for(x=0; x < (lpddsd->dwWidth>>1); x++){
			y0 = (int)*psrc++ -16;
			u  = (int)*psrc++ -128;
			y1 = (int)*psrc++ -16;
			v  = (int)*psrc++ -128;
			// unoptimized, floating point calculation
			b  = (WORD)clip((float)(y0 +                 ( 1.140 * v )), 0, 255);
			g  = (WORD)clip((float)(y0 - ( 0.395 * u ) - ( 0.581 * v )), 0, 255);
			r  = (WORD)clip((float)(y0 + ( 2.032 * u )                ), 0, 255);
			// R8G8B8 conversion
			*pdst++ = ((r & 0xFF) | ((g & 0xFF)<<8) | ((b & 0xFF)<<16));
			// unoptimized, floating point calculation
			b  = (WORD)clip((float)(y1 +                 ( 1.140 * v )), 0, 255);
			g  = (WORD)clip((float)(y1 - ( 0.395 * u ) - ( 0.581 * v )), 0, 255);
			r  = (WORD)clip((float)(y1 + ( 2.032 * u )                ), 0, 255);
			// R8G8B8 conversion
			*pdst++ = ((r & 0xFF) | ((g & 0xFF)<<8) | ((b & 0xFF)<<16)); 
		}
		//psrc += (dwGap << 1);
		//pdst += dwGap;
	}
	//OutTrace("dxwConvertYUY232 end\n");
	return yuy2buf;
}

LPVOID dxwConvertYUY224(LPDDSURFACEDESC2 lpddsd)
{
	LPBYTE yuy2buf;
	DWORD x, y, dwGap;
	int y0, y1, u, v;
	WORD r, g, b;
	yuy2buf = (LPBYTE)malloc(lpddsd->dwHeight * lpddsd->lPitch * (sizeof(DWORD) / 2));
	LPBYTE psrc = (LPBYTE)lpddsd->lpSurface;
	LPBYTE pdst = (LPBYTE)yuy2buf;
	dwGap = lpddsd->lPitch - (3 * lpddsd->dwWidth);
	for(y=0; y<lpddsd->dwHeight; y++){
		for(x=0; x < (lpddsd->dwWidth); x+=3){
			y0 = (int)*psrc++ -16;
			u  = (int)*psrc++ -128;
			y1 = (int)*psrc++ -16;
			v  = (int)*psrc++ -128;
			// unoptimized, floating point calculation
			b  = (WORD)clip((float)(y0 +                 ( 1.140 * v )), 0, 255);
			g  = (WORD)clip((float)(y0 - ( 0.395 * u ) - ( 0.581 * v )), 0, 255);
			r  = (WORD)clip((float)(y0 + ( 2.032 * u )                ), 0, 255);
			// R8G8B8 conversion
			*pdst++ = (r & 0xFF);
			*pdst++ = (g & 0xFF);
			*pdst++ = (b & 0xFF);
			// unoptimized, floating point calculation
			b  = (WORD)clip((float)(y1 +                 ( 1.140 * v )), 0, 255);
			g  = (WORD)clip((float)(y1 - ( 0.395 * u ) - ( 0.581 * v )), 0, 255);
			r  = (WORD)clip((float)(y1 + ( 2.032 * u )                ), 0, 255);
			// R8G8B8 conversion
			*pdst++ = (r & 0xFF);
			*pdst++ = (g & 0xFF);
			*pdst++ = (b & 0xFF);
		}
		psrc += dwGap;
		pdst += dwGap;
	}
	return yuy2buf;
}

LPVOID dxwConvertFourCC(LPDDSURFACEDESC2 lpddsd, int bpp)
{
	DWORD dwFourCC = lpddsd->ddpfPixelFormat.dwFourCC;
	DWORD dwRGBBitCount = lpddsd->ddpfPixelFormat.dwRGBBitCount;
	switch(dwFourCC){
		case 0: // no FourCC
			switch (bpp){
				case 16:
					switch(dwRGBBitCount){
						case 16:
							return dxwCopyPixels(lpddsd);
							break;
						case 24:
							return NULL;
							break;
						case 32:
							//return dxwConvert32to16(lpddsd);
							return NULL;
							break;
						default:
							return NULL;
							break;
					}
					break;
				case 24:
					return NULL;
					break;
				case 32:
					switch(dwRGBBitCount){
						case 16:
							return dxwConvert16to32(lpddsd);
							break;
						case 24:
							return NULL;
							break;
						case 32:
							return dxwCopyPixels(lpddsd);
							break;
						default:
							return NULL;
							break;
					}
				default:
					//OutTraceE("dxwConvertFourCC: unsupported depth BPP=%d\n", dwRGBBitCount);
					return NULL;
			}
			break;
		case 0x32595559: /*YUY2*/
			switch (bpp){
				case 16:
					return dxwConvertYUY216(lpddsd);
					break;
				case 24:
					return dxwConvertYUY224(lpddsd);
					break;
				case 32:
					return dxwConvertYUY232(lpddsd);
					break;
				default:
					//OutTraceE("dxwConvertFourCC: unsupported depth BPP=%d\n", dwRGBBitCount);
					return NULL;
					break;
			}
			break;
		case 0x32315659: /*YV12*/
		// found in "Soccer Nation" - bpp=12!! to be implemented
		default:
			//OutTraceE("dxwConvertFourCC: unsupported codec FourCC=%#x\n", dwFourCC);
			return NULL;
	}
	return NULL;
}
