#define _CRT_SECURE_NO_DEPRECATE 1
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <dsound.h>
#include <d3d.h>
#include <dinput.h>
#include <mciavi.h>
#include <digitalv.h>
#include <CommCtrl.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "audioenginebaseapo.h"

#ifndef DXW_NOTRACES
 
// debug functions to make the log more readable

void strscat(char *dest, int destMaxLen, char *src)
{
	int destLen = strlen(dest);
	if(destLen < destMaxLen) strncat(dest, src, destMaxLen - destLen);
}

char *sGUID(GUID *guid)
{
	static char sRIIDBuffer[81];
	if(((DWORD)guid & 0xFFFF0000) == 0){
		switch ((DWORD)guid){
			case NULL:						strcpy(sRIIDBuffer, "NULL"); break;
			case DDCREATE_HARDWAREONLY:		strcpy(sRIIDBuffer, "DDCREATE_HARDWAREONLY"); break;
			case DDCREATE_EMULATIONONLY:	strcpy(sRIIDBuffer, "DDCREATE_EMULATIONONLY"); break;
			default:						sprintf(sRIIDBuffer, "ATOM(0x%04X)", (DWORD)guid); break;
		}
	}
	else {
		OLECHAR* guidString;
		StringFromCLSID(*guid, &guidString);
		sprintf_s(sRIIDBuffer, 80, "%ls",  guidString);
		::CoTaskMemFree(guidString);
	}
	return sRIIDBuffer;
}

char *ExplainFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DDSD_", eblen);
	if (c & DDSD_CAPS) strscat(eb, eblen, "CAPS+");
	if (c & DDSD_HEIGHT) strscat(eb, eblen, "HEIGHT+");
	if (c & DDSD_WIDTH) strscat(eb, eblen, "WIDTH+");
	if (c & DDSD_PITCH) strscat(eb, eblen, "PITCH+");
	if (c & DDSD_BACKBUFFERCOUNT) strscat(eb, eblen, "BACKBUFFERCOUNT+");
	if (c & DDSD_ZBUFFERBITDEPTH) strscat(eb, eblen, "ZBUFFERBITDEPTH+");
	if (c & DDSD_ALPHABITDEPTH) strscat(eb, eblen, "ALPHABITDEPTH+");
	if (c & DDSD_LPSURFACE) strscat(eb, eblen, "LPSURFACE+");
	if (c & DDSD_PIXELFORMAT) strscat(eb, eblen, "PIXELFORMAT+");
	if (c & DDSD_CKDESTOVERLAY) strscat(eb, eblen, "CKDESTOVERLAY+");
	if (c & DDSD_CKDESTBLT) strscat(eb, eblen, "CKDESTBLT+");
	if (c & DDSD_CKSRCOVERLAY) strscat(eb, eblen, "CKSRCOVERLAY+");
	if (c & DDSD_CKSRCBLT) strscat(eb, eblen, "CKSRCBLT+");
	if (c & DDSD_MIPMAPCOUNT) strscat(eb, eblen, "MIPMAPCOUNT+");
	if (c & DDSD_REFRESHRATE) strscat(eb, eblen, "REFRESHRATE+");
	if (c & DDSD_LINEARSIZE) strscat(eb, eblen, "LINEARSIZE+");
	if (c & DDSD_TEXTURESTAGE) strscat(eb, eblen, "TEXTURESTAGE+");
	if (c & DDSD_SRCVBHANDLE) strscat(eb, eblen, "SRCVBHANDLE+");
	if (c & DDSD_DEPTH) strscat(eb, eblen, "DEPTH+");
	if (c & DDSD_FVF) strscat(eb, eblen, "FVF+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDSD_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainPFFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strcpy(eb,"PFD_");
	if (c & PFD_DOUBLEBUFFER) strscat(eb, eblen, "DOUBLEBUFFER+");
	if (c & PFD_STEREO) strscat(eb, eblen, "STEREO+");
	if (c & PFD_DRAW_TO_WINDOW) strscat(eb, eblen, "DRAW_TO_WINDOW+");
	if (c & PFD_DRAW_TO_BITMAP) strscat(eb, eblen, "DRAW_TO_BITMAP+");
	if (c & PFD_SUPPORT_GDI) strscat(eb, eblen, "SUPPORT_GDI+");
	if (c & PFD_SUPPORT_OPENGL) strscat(eb, eblen, "SUPPORT_OPENGL+");
	if (c & PFD_GENERIC_FORMAT) strscat(eb, eblen, "GENERIC_FORMAT+");
	if (c & PFD_NEED_PALETTE) strscat(eb, eblen, "NEED_PALETTE+");
	if (c & PFD_NEED_SYSTEM_PALETTE) strscat(eb, eblen, "NEED_SYSTEM_PALETTE+");
	if (c & PFD_SWAP_EXCHANGE) strscat(eb, eblen, "SWAP_EXCHANGE+");
	if (c & PFD_SWAP_COPY) strscat(eb, eblen, "SWAP_COPY+");
	if (c & PFD_SWAP_LAYER_BUFFERS) strscat(eb, eblen, "SWAP_LAYER_BUFFERS+");
	if (c & PFD_GENERIC_ACCELERATED) strscat(eb, eblen, "GENERIC_ACCELERATED+");
	if (c & PFD_SUPPORT_DIRECTDRAW) strscat(eb, eblen, "SUPPORT_DIRECTDRAW+");
	if (c & PFD_DIRECT3D_ACCELERATED) strscat(eb, eblen, "DIRECT3D_ACCELERATED+");
	if (c & PFD_SUPPORT_COMPOSITION) strscat(eb, eblen, "SUPPORT_COMPOSITION+");
	if (c & PFD_DEPTH_DONTCARE) strscat(eb, eblen, "DEPTH_DONTCARE+");
	if (c & PFD_DOUBLEBUFFER_DONTCARE) strscat(eb, eblen, "DOUBLEBUFFER_DONTCARE+");
	if (c & PFD_STEREO_DONTCARE) strscat(eb, eblen, "STEREO_DONTCARE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("PFD_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDSCaps(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDSCAPS_", eblen);
	if (c & DDSCAPS_RESERVED1) strscat(eb, eblen, "RESERVED1+");
	if (c & DDSCAPS_ALPHA) strscat(eb, eblen, "ALPHA+");
	if (c & DDSCAPS_BACKBUFFER) strscat(eb, eblen, "BACKBUFFER+");
	if (c & DDSCAPS_COMPLEX) strscat(eb, eblen, "COMPLEX+");
	if (c & DDSCAPS_FLIP) strscat(eb, eblen, "FLIP+");
	if (c & DDSCAPS_FRONTBUFFER) strscat(eb, eblen, "FRONTBUFFER+");
	if (c & DDSCAPS_OFFSCREENPLAIN) strscat(eb, eblen, "OFFSCREENPLAIN+");
	if (c & DDSCAPS_OVERLAY) strscat(eb, eblen, "OVERLAY+");
	if (c & DDSCAPS_PALETTE) strscat(eb, eblen, "PALETTE+");
	if (c & DDSCAPS_PRIMARYSURFACE) strscat(eb, eblen, "PRIMARYSURFACE+");
	if (c & DDSCAPS_SYSTEMMEMORY) strscat(eb, eblen, "SYSTEMMEMORY+");
	if (c & DDSCAPS_TEXTURE) strscat(eb, eblen, "TEXTURE+");
	if (c & DDSCAPS_3DDEVICE) strscat(eb, eblen, "3DDEVICE+");
	if (c & DDSCAPS_VIDEOMEMORY) strscat(eb, eblen, "VIDEOMEMORY+");
	if (c & DDSCAPS_VISIBLE) strscat(eb, eblen, "VISIBLE+");
	if (c & DDSCAPS_WRITEONLY) strscat(eb, eblen, "WRITEONLY+");
	if (c & DDSCAPS_ZBUFFER) strscat(eb, eblen, "ZBUFFER+");
	if (c & DDSCAPS_OWNDC) strscat(eb, eblen, "OWNDC+");
	if (c & DDSCAPS_LIVEVIDEO) strscat(eb, eblen, "LIVEVIDEO+");
	if (c & DDSCAPS_HWCODEC) strscat(eb, eblen, "HWCODEC+");
	if (c & DDSCAPS_MODEX) strscat(eb, eblen, "MODEX+");
	if (c & DDSCAPS_MIPMAP) strscat(eb, eblen, "MIPMAP+"); 
	if (c & DDSCAPS_RESERVED2) strscat(eb, eblen, "RESERVED2+");
	if (c & DDSCAPS_ALLOCONLOAD) strscat(eb, eblen, "ALLOCONLOAD+");
	if (c & DDSCAPS_VIDEOPORT) strscat(eb, eblen, "VIDEOPORT+");
	if (c & DDSCAPS_LOCALVIDMEM) strscat(eb, eblen, "LOCALVIDMEM+");
	if (c & DDSCAPS_NONLOCALVIDMEM) strscat(eb, eblen, "NONLOCALVIDMEM+");
	if (c & DDSCAPS_STANDARDVGAMODE) strscat(eb, eblen, "STANDARDVGAMODE+");
	if (c & DDSCAPS_OPTIMIZED) strscat(eb, eblen, "OPTIMIZED+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDSCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDSCaps2(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDSCAPS2_", eblen);
	if (c & DDSCAPS2_RESERVED4) strscat(eb, eblen, "RESERVED4+");
	if (c & DDSCAPS2_HINTDYNAMIC) strscat(eb, eblen, "HINTDYNAMIC+");
	if (c & DDSCAPS2_HINTSTATIC) strscat(eb, eblen, "HINTSTATIC+");
	if (c & DDSCAPS2_TEXTUREMANAGE) strscat(eb, eblen, "TEXTUREMANAGE+");
	if (c & DDSCAPS2_RESERVED1) strscat(eb, eblen, "RESERVED1+");
	if (c & DDSCAPS2_RESERVED2) strscat(eb, eblen, "RESERVED2+");
	if (c & DDSCAPS2_OPAQUE) strscat(eb, eblen, "OPAQUE+");
	if (c & DDSCAPS2_HINTANTIALIASING) strscat(eb, eblen, "HINTANTIALIASING+");
	if (c & DDSCAPS2_CUBEMAP) strscat(eb, eblen, "CUBEMAP+");
	if (c & DDSCAPS2_CUBEMAP_POSITIVEX) strscat(eb, eblen, "CUBEMAP_POSITIVEX+");
	if (c & DDSCAPS2_CUBEMAP_NEGATIVEX) strscat(eb, eblen, "CUBEMAP_NEGATIVEX+");
	if (c & DDSCAPS2_CUBEMAP_POSITIVEY) strscat(eb, eblen, "CUBEMAP_POSITIVEY+");
	if (c & DDSCAPS2_CUBEMAP_NEGATIVEY) strscat(eb, eblen, "CUBEMAP_NEGATIVEY+");
	if (c & DDSCAPS2_CUBEMAP_POSITIVEZ) strscat(eb, eblen, "CUBEMAP_POSITIVEZ+");
	if (c & DDSCAPS2_CUBEMAP_NEGATIVEZ) strscat(eb, eblen, "CUBEMAP_NEGATIVEZ+");
	if (c & DDSCAPS2_MIPMAPSUBLEVEL) strscat(eb, eblen, "MIPMAPSUBLEVEL+");
	if (c & DDSCAPS2_D3DTEXTUREMANAGE) strscat(eb, eblen, "D3DTEXTUREMANAGE+");
	if (c & DDSCAPS2_DONOTPERSIST) strscat(eb, eblen, "DONOTPERSIST+");
	if (c & DDSCAPS2_STEREOSURFACELEFT) strscat(eb, eblen, "STEREOSURFACELEFT+");
	if (c & DDSCAPS2_VOLUME) strscat(eb, eblen, "VOLUME+");
	if (c & DDSCAPS2_NOTUSERLOCKABLE) strscat(eb, eblen, "NOTUSERLOCKABLE+");
	if (c & DDSCAPS2_POINTS) strscat(eb, eblen, "POINTS+");
	if (c & DDSCAPS2_RTPATCHES) strscat(eb, eblen, "RTPATCHES+");
	if (c & DDSCAPS2_NPATCHES) strscat(eb, eblen, "NPATCHES+");
	if (c & DDSCAPS2_RESERVED3) strscat(eb, eblen, "RESERVED3+");
	if (c & DDSCAPS2_DISCARDBACKBUFFER) strscat(eb, eblen, "DISCARDBACKBUFFER+");
	if (c & DDSCAPS2_ENABLEALPHACHANNEL) strscat(eb, eblen, "ENABLEALPHACHANNEL+");
	if (c & DDSCAPS2_EXTENDEDFORMATPRIMARY) strscat(eb, eblen, "EXTENDEDFORMATPRIMARY+");
	if (c & DDSCAPS2_ADDITIONALPRIMARY) strscat(eb, eblen, "ADDITIONALPRIMARY+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDSCAPS2_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDSCaps3(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDSCAPS3_", eblen);
	if (c & DDSCAPS3_MULTISAMPLE_MASK) strscat(eb, eblen, "DDSCAPS3_MULTISAMPLE_MASK+");
	if (c & DDSCAPS3_RESERVED1) strscat(eb, eblen, "DDSCAPS3_RESERVED1+");
	if (c & DDSCAPS3_RESERVED2) strscat(eb, eblen, "DDSCAPS3_RESERVED2+");
	if (c & DDSCAPS3_LIGHTWEIGHTMIPMAP) strscat(eb, eblen, "DDSCAPS3_LIGHTWEIGHTMIPMAP+");
	if (c & DDSCAPS3_AUTOGENMIPMAP) strscat(eb, eblen, "DDSCAPS3_AUTOGENMIPMAP+");
	if (c & DDSCAPS3_DMAP) strscat(eb, eblen, "DDSCAPS3_DMAP+");
	if (c & DDSCAPS3_MULTISAMPLE_QUALITY_MASK) {
		DWORD dwQuality;
		char sQuality[32];
		dwQuality = (c & DDSCAPS3_MULTISAMPLE_QUALITY_MASK) >> DDSCAPS3_MULTISAMPLE_QUALITY_SHIFT;
		sprintf(sQuality, "QUALITY(%d)+", dwQuality);
		strscat(eb, eblen, sQuality);
	}
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDSCAPS3_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDSCaps4(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDSCAPS4_", eblen);
	// insert here ....
	// if (c & DDSCAPS4_XXX) strscat(eb, eblen, "XXX+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDSCAPS4_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDDCaps(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDCAPS_", eblen);
	if (c & DDCAPS_3D) strscat(eb, eblen, "3D+");
	if (c & DDCAPS_ALIGNBOUNDARYDEST) strscat(eb, eblen, "ALIGNBOUNDARYDEST+");
	if (c & DDCAPS_ALIGNSIZEDEST) strscat(eb, eblen, "ALIGNSIZEDEST+");
	if (c & DDCAPS_ALIGNBOUNDARYSRC) strscat(eb, eblen, "ALIGNBOUNDARYSRC+");
	if (c & DDCAPS_ALIGNSIZESRC) strscat(eb, eblen, "ALIGNSIZESRC+");
	if (c & DDCAPS_ALIGNSTRIDE) strscat(eb, eblen, "ALIGNSTRIDE+");
	if (c & DDCAPS_BLT) strscat(eb, eblen, "BLT+");
	if (c & DDCAPS_BLTQUEUE) strscat(eb, eblen, "BLTQUEUE+");
	if (c & DDCAPS_BLTFOURCC) strscat(eb, eblen, "BLTFOURCC+");
	if (c & DDCAPS_BLTSTRETCH) strscat(eb, eblen, "BLTSTRETCH+");
	if (c & DDCAPS_GDI) strscat(eb, eblen, "GDI+");
	if (c & DDCAPS_OVERLAY) strscat(eb, eblen, "OVERLAY+");
	if (c & DDCAPS_OVERLAYCANTCLIP) strscat(eb, eblen, "OVERLAYCANTCLIP+");
	if (c & DDCAPS_OVERLAYFOURCC) strscat(eb, eblen, "OVERLAYFOURCC+");
	if (c & DDCAPS_OVERLAYSTRETCH) strscat(eb, eblen, "OVERLAYSTRETCH+");
	if (c & DDCAPS_PALETTE) strscat(eb, eblen, "PALETTE+");
	if (c & DDCAPS_PALETTEVSYNC) strscat(eb, eblen, "PALETTEVSYNC+");
	if (c & DDCAPS_READSCANLINE) strscat(eb, eblen, "READSCANLINE+");
	if (c & DDCAPS_RESERVED1) strscat(eb, eblen, "RESERVED1+");
	if (c & DDCAPS_VBI) strscat(eb, eblen, "VBI+");
	if (c & DDCAPS_ZBLTS) strscat(eb, eblen, "ZBLTS+");
	if (c & DDCAPS_ZOVERLAYS) strscat(eb, eblen, "ZOVERLAYS+");
	if (c & DDCAPS_COLORKEY) strscat(eb, eblen, "COLORKEY+");
	if (c & DDCAPS_ALPHA) strscat(eb, eblen, "ALPHA+");
	if (c & DDCAPS_COLORKEYHWASSIST) strscat(eb, eblen, "COLORKEYHWASSIST+");
	if (c & DDCAPS_NOHARDWARE) strscat(eb, eblen, "NOHARDWARE+");
	if (c & DDCAPS_BLTCOLORFILL) strscat(eb, eblen, "BLTCOLORFILL+");
	if (c & DDCAPS_BANKSWITCHED) strscat(eb, eblen, "BANKSWITCHED+");
	if (c & DDCAPS_BLTDEPTHFILL) strscat(eb, eblen, "BLTDEPTHFILL+");
	if (c & DDCAPS_CANCLIP) strscat(eb, eblen, "CANCLIP+");
	if (c & DDCAPS_CANCLIPSTRETCHED) strscat(eb, eblen, "CANCLIPSTRETCHED+");
	if (c & DDCAPS_CANBLTSYSMEM) strscat(eb, eblen, "CANBLTSYSMEM+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDDCaps2(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DDCAPS2_", eblen);
	if (c & DDCAPS2_CERTIFIED) strscat(eb, eblen, "CERTIFIED+");
	if (c & DDCAPS2_NO2DDURING3DSCENE) strscat(eb, eblen, "NO2DDURING3DSCENE+");
	if (c & DDCAPS2_VIDEOPORT) strscat(eb, eblen, "VIDEOPORT+");
	if (c & DDCAPS2_AUTOFLIPOVERLAY) strscat(eb, eblen, "AUTOFLIPOVERLAY+");
	if (c & DDCAPS2_CANBOBINTERLEAVED) strscat(eb, eblen, "CANBOBINTERLEAVED+");
	if (c & DDCAPS2_CANBOBNONINTERLEAVED) strscat(eb, eblen, "CANBOBNONINTERLEAVED+");
	if (c & DDCAPS2_COLORCONTROLOVERLAY) strscat(eb, eblen, "COLORCONTROLOVERLAY+");
	if (c & DDCAPS2_COLORCONTROLPRIMARY) strscat(eb, eblen, "COLORCONTROLPRIMARY+");
	if (c & DDCAPS2_CANDROPZ16BIT) strscat(eb, eblen, "CANDROPZ16BIT+");
	if (c & DDCAPS2_NONLOCALVIDMEM) strscat(eb, eblen, "NONLOCALVIDMEM+");
	if (c & DDCAPS2_NONLOCALVIDMEMCAPS) strscat(eb, eblen, "NONLOCALVIDMEMCAPS+");
	if (c & DDCAPS2_NOPAGELOCKREQUIRED) strscat(eb, eblen, "NOPAGELOCKREQUIRED+");
	if (c & DDCAPS2_WIDESURFACES) strscat(eb, eblen, "WIDESURFACES+");
	if (c & DDCAPS2_CANFLIPODDEVEN) strscat(eb, eblen, "CANFLIPODDEVEN+");
	if (c & DDCAPS2_CANBOBHARDWARE) strscat(eb, eblen, "CANBOBHARDWARE+");
	if (c & DDCAPS2_COPYFOURCC) strscat(eb, eblen, "COPYFOURCC+");
	if (c & DDCAPS2_PRIMARYGAMMA) strscat(eb, eblen, "PRIMARYGAMMA+");
	if (c & DDCAPS2_CANRENDERWINDOWED) strscat(eb, eblen, "CANRENDERWINDOWED+");
	if (c & DDCAPS2_CANCALIBRATEGAMMA) strscat(eb, eblen, "CANCALIBRATEGAMMA+");
	if (c & DDCAPS2_FLIPINTERVAL) strscat(eb, eblen, "FLIPINTERVAL+");
	if (c & DDCAPS2_FLIPNOVSYNC) strscat(eb, eblen, "FLIPNOVSYNC+");
	if (c & DDCAPS2_CANMANAGETEXTURE) strscat(eb, eblen, "CANMANAGETEXTURE+");
	if (c & DDCAPS2_TEXMANINNONLOCALVIDMEM) strscat(eb, eblen, "TEXMANINNONLOCALVIDMEM+");
	if (c & DDCAPS2_STEREO) strscat(eb, eblen, "STEREO+");
	if (c & DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL) strscat(eb, eblen, "SYSTONONLOCAL_AS_SYSTOLOCAL+");
	if (c & DDCAPS2_RESERVED1) strscat(eb, eblen, "RESERVED1/PUREHAL+");
	if (c & DDCAPS2_CANMANAGERESOURCE) strscat(eb, eblen, "CANMANAGERESOURCE+");
	if (c & DDCAPS2_DYNAMICTEXTURES) strscat(eb, eblen, "DYNAMICTEXTURES+");
	if (c & DDCAPS2_CANAUTOGENMIPMAP) strscat(eb, eblen, "CANAUTOGENMIPMAP+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDCAPS2_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDFXALPHACaps(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DDFXALPHACAPS_", eblen);
	if (c & DDFXALPHACAPS_BLTALPHAEDGEBLEND) strscat(eb, eblen, "BLTALPHAEDGEBLEND+");
	if (c & DDFXALPHACAPS_BLTALPHAPIXELS) strscat(eb, eblen, "BLTALPHAPIXELS+");
	if (c & DDFXALPHACAPS_BLTALPHAPIXELSNEG) strscat(eb, eblen, "BLTALPHAPIXELSNEG+");
	if (c & DDFXALPHACAPS_BLTALPHASURFACES) strscat(eb, eblen, "BLTALPHASURFACES+");
	if (c & DDFXALPHACAPS_BLTALPHASURFACESNEG) strscat(eb, eblen, "BLTALPHASURFACESNEG+");
	if (c & DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND) strscat(eb, eblen, "OVERLAYALPHAEDGEBLEND+");
	if (c & DDFXALPHACAPS_OVERLAYALPHAPIXELS) strscat(eb, eblen, "OVERLAYALPHAPIXELS+");
	if (c & DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG) strscat(eb, eblen, "OVERLAYALPHAPIXELSNEG+");
	if (c & DDFXALPHACAPS_OVERLAYALPHASURFACES) strscat(eb, eblen, "OVERLAYALPHASURFACES+");
	if (c & DDFXALPHACAPS_OVERLAYALPHASURFACESNEG) strscat(eb, eblen, "OVERLAYALPHASURFACESNEG+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDFXALPHACAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return eb;
}

char *ExplainDDFXCaps(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DDFXCAPS_", eblen);
	if (c & DDFXCAPS_BLTARITHSTRETCHY) strscat(eb, eblen, "BLTARITHSTRETCHY+");
	if (c & DDFXCAPS_BLTARITHSTRETCHYN) strscat(eb, eblen, "BLTARITHSTRETCHYN+");
	if (c & DDFXCAPS_BLTMIRRORLEFTRIGHT) strscat(eb, eblen, "BLTMIRRORLEFTRIGHT+");
	if (c & DDFXCAPS_BLTMIRRORUPDOWN) strscat(eb, eblen, "BLTMIRRORUPDOWN+");
	if (c & DDFXCAPS_BLTROTATION) strscat(eb, eblen, "BLTROTATION+");
	if (c & DDFXCAPS_BLTROTATION90) strscat(eb, eblen, "BLTROTATION90+");
	if (c & DDFXCAPS_BLTSHRINKX) strscat(eb, eblen, "BLTSHRINKX+");
	if (c & DDFXCAPS_BLTSHRINKXN) strscat(eb, eblen, "BLTSHRINKXN+");
	if (c & DDFXCAPS_BLTSHRINKY) strscat(eb, eblen, "BLTSHRINKY+");
	if (c & DDFXCAPS_BLTSHRINKYN) strscat(eb, eblen, "BLTSHRINKYN+");
	if (c & DDFXCAPS_BLTSTRETCHX) strscat(eb, eblen, "BLTSTRETCHX+");
	if (c & DDFXCAPS_BLTSTRETCHXN) strscat(eb, eblen, "BLTSTRETCHXN+");
	if (c & DDFXCAPS_BLTSTRETCHY) strscat(eb, eblen, "BLTSTRETCHY+");
	if (c & DDFXCAPS_BLTSTRETCHYN) strscat(eb, eblen, "BLTSTRETCHYN+");
	if (c & DDFXCAPS_OVERLAYARITHSTRETCHY) strscat(eb, eblen, "OVERLAYARITHSTRETCHY+");
	if (c & DDFXCAPS_OVERLAYARITHSTRETCHYN) strscat(eb, eblen, "OVERLAYARITHSTRETCHYN+");
	if (c & DDFXCAPS_OVERLAYSHRINKX) strscat(eb, eblen, "OVERLAYSHRINKX+");
	if (c & DDFXCAPS_OVERLAYSHRINKXN) strscat(eb, eblen, "OVERLAYSHRINKXN+");
	if (c & DDFXCAPS_OVERLAYSHRINKY) strscat(eb, eblen, "OVERLAYSHRINKY+");
	if (c & DDFXCAPS_OVERLAYSHRINKYN) strscat(eb, eblen, "OVERLAYSHRINKYN+");
	if (c & DDFXCAPS_OVERLAYSTRETCHX) strscat(eb, eblen, "OVERLAYSTRETCHX+");
	if (c & DDFXCAPS_OVERLAYSTRETCHXN) strscat(eb, eblen, "OVERLAYSTRETCHXN+");
	if (c & DDFXCAPS_OVERLAYSTRETCHY) strscat(eb, eblen, "OVERLAYSTRETCHY+");
	if (c & DDFXCAPS_OVERLAYSTRETCHYN) strscat(eb, eblen, "OVERLAYSTRETCHYN+");
	if (c & DDFXCAPS_OVERLAYMIRRORLEFTRIGHT) strscat(eb, eblen, "OVERLAYMIRRORLEFTRIGHT+");
	if (c & DDFXCAPS_OVERLAYMIRRORUPDOWN) strscat(eb, eblen, "OVERLAYMIRRORUPDOWN+");
	if (c & DDFXCAPS_OVERLAYDEINTERLACE) strscat(eb, eblen, "OVERLAYDEINTERLACE+");
	if (c & DDFXCAPS_BLTALPHA) strscat(eb, eblen, "BLTALPHA+");
	if (c & DDFXCAPS_BLTFILTER) strscat(eb, eblen, "BLTFILTER+");
	if (c & DDFXCAPS_OVERLAYALPHA) strscat(eb, eblen, "OVERLAYALPHA+");
	if (c & DDFXCAPS_BLTARITHSTRETCHY) strscat(eb, eblen, "BLTARITHSTRETCHY+");
	if (c & DDFXCAPS_OVERLAYFILTER) strscat(eb, eblen, "OVERLAYFILTER+");
	if (c & DDFXCAPS_OVERLAYARITHSTRETCHY) strscat(eb, eblen, "OVERLAYARITHSTRETCHY+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDFXCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return eb;
}

char *ExplainDDPalCaps(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DDPCAPS_", eblen);
	if (c & DDPCAPS_ALPHA) strscat(eb, eblen, "ALPHA+");
	if (c & DDPCAPS_4BIT) strscat(eb, eblen, "4BIT+");
	if (c & DDPCAPS_8BITENTRIES) strscat(eb, eblen, "8BITENTRIES+");
	if (c & DDPCAPS_8BIT) strscat(eb, eblen, "8BIT+");
	if (c & DDPCAPS_INITIALIZE) strscat(eb, eblen, "INITIALIZE+");
	if (c & DDPCAPS_PRIMARYSURFACELEFT) strscat(eb, eblen, "PRIMARYSURFACELEFT+");
	if (c & DDPCAPS_ALLOW256) strscat(eb, eblen, "ALLOW256+");
	if (c & DDPCAPS_VSYNC) strscat(eb, eblen, "VSYNC+");
	if (c & DDPCAPS_1BIT) strscat(eb, eblen, "1BIT+");
	if (c & DDPCAPS_2BIT) strscat(eb, eblen, "2BIT+");
	if (c & DDPCAPS_PRIMARYSURFACE) strscat(eb, eblen, "PRIMARYSURFACE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDPCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDDCKeyCaps(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDCKEYCAPS_", eblen);
	if (c & DDCKEYCAPS_DESTBLT) strscat(eb, eblen, "DESTBLT+");
	if (c & DDCKEYCAPS_DESTBLTCLRSPACE) strscat(eb, eblen, "DESTBLTCLRSPACE+");
	if (c & DDCKEYCAPS_DESTBLTCLRSPACEYUV) strscat(eb, eblen, "DESTBLTCLRSPACEYUV+");
	if (c & DDCKEYCAPS_DESTBLTYUV) strscat(eb, eblen, "DESTBLTYUV+");
	if (c & DDCKEYCAPS_DESTOVERLAY) strscat(eb, eblen, "DESTOVERLAY+");
	if (c & DDCKEYCAPS_DESTOVERLAYCLRSPACE) strscat(eb, eblen, "DESTOVERLAYCLRSPACE+");
	if (c & DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV) strscat(eb, eblen, "DESTOVERLAYCLRSPACEYUV+");
	if (c & DDCKEYCAPS_DESTOVERLAYONEACTIVE) strscat(eb, eblen, "DESTOVERLAYONEACTIVE+");
	if (c & DDCKEYCAPS_DESTOVERLAYYUV) strscat(eb, eblen, "DESTOVERLAYYUV+");
	if (c & DDCKEYCAPS_SRCBLT) strscat(eb, eblen, "SRCBLT+");
	if (c & DDCKEYCAPS_SRCBLTCLRSPACE) strscat(eb, eblen, "SRCBLTCLRSPACE+");
	if (c & DDCKEYCAPS_SRCBLTCLRSPACEYUV) strscat(eb, eblen, "SRCBLTCLRSPACEYUV+");
	if (c & DDCKEYCAPS_SRCBLTYUV) strscat(eb, eblen, "SRCBLTYUV+");
	if (c & DDCKEYCAPS_SRCOVERLAY) strscat(eb, eblen, "SRCOVERLAY+");
	if (c & DDCKEYCAPS_SRCOVERLAYCLRSPACE) strscat(eb, eblen, "SRCOVERLAYCLRSPACE+");
	if (c & DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV) strscat(eb, eblen, "SRCOVERLAYCLRSPACEYUV+");
	if (c & DDCKEYCAPS_SRCOVERLAYONEACTIVE) strscat(eb, eblen, "SRCOVERLAYONEACTIVE+");
	if (c & DDCKEYCAPS_SRCOVERLAYYUV) strscat(eb, eblen, "SRCOVERLAYYUV+");
	if (c & DDCKEYCAPS_NOCOSTOVERLAY) strscat(eb, eblen, "NOCOSTOVERLAY+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDCKEYCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainCoopFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDSCL_", eblen);
	if (c & DDSCL_FULLSCREEN) strscat(eb, eblen, "FULLSCREEN+");
	if (c & DDSCL_ALLOWREBOOT) strscat(eb, eblen, "ALLOWREBOOT+");
	if (c & DDSCL_NOWINDOWCHANGES) strscat(eb, eblen, "NOWINDOWCHANGES+");
	if (c & DDSCL_NORMAL) strscat(eb, eblen, "NORMAL+");
	if (c & DDSCL_EXCLUSIVE) strscat(eb, eblen, "EXCLUSIVE+");
	if (c & DDSCL_ALLOWMODEX) strscat(eb, eblen, "ALLOWMODEX+");
	if (c & DDSCL_SETFOCUSWINDOW) strscat(eb, eblen, "SETFOCUSWINDOW+");
	if (c & DDSCL_SETDEVICEWINDOW) strscat(eb, eblen, "SETDEVICEWINDOW+");
	if (c & DDSCL_CREATEDEVICEWINDOW) strscat(eb, eblen, "CREATEDEVICEWINDOW+");
	if (c & DDSCL_MULTITHREADED) strscat(eb, eblen, "MULTITHREADED+");
	if (c & DDSCL_FPUSETUP) strscat(eb, eblen, "FPUSETUP+");
	if (c & DDSCL_FPUPRESERVE) strscat(eb, eblen, "FPUPRESERVE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDSCL_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainPixelFormatFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDPF_", eblen);
	if (c & DDPF_ALPHAPIXELS) strscat(eb, eblen, "ALPHAPIXELS+");
	if (c & DDPF_ALPHA) strscat(eb, eblen, "ALPHA+");
	if (c & DDPF_FOURCC) strscat(eb, eblen, "FOURCC+");
	if (c & DDPF_PALETTEINDEXED4) strscat(eb, eblen, "PALETTEINDEXED4+");
	if (c & DDPF_PALETTEINDEXEDTO8) strscat(eb, eblen, "PALETTEINDEXEDTO8+");
	if (c & DDPF_PALETTEINDEXED8) strscat(eb, eblen, "PALETTEINDEXED8+");
	if (c & DDPF_RGB) strscat(eb, eblen, "RGB+");
	if (c & DDPF_COMPRESSED) strscat(eb, eblen, "COMPRESSED+");
	if (c & DDPF_RGBTOYUV) strscat(eb, eblen, "RGBTOYUV+");
	if (c & DDPF_YUV) strscat(eb, eblen, "YUV+");
	if (c & DDPF_ZBUFFER) strscat(eb, eblen, "ZBUFFER+");
	if (c & DDPF_PALETTEINDEXED1) strscat(eb, eblen, "PALETTEINDEXED1+");
	if (c & DDPF_PALETTEINDEXED2) strscat(eb, eblen, "PALETTEINDEXED2+");
	if (c & DDPF_ZPIXELS) strscat(eb, eblen, "ZPIXELS+");
	if (c & DDPF_STENCILBUFFER) strscat(eb, eblen, "STENCILBUFFER+");
	if (c & DDPF_ALPHAPREMULT) strscat(eb, eblen, "ALPHAPREMULT+");
	if (c & DDPF_LUMINANCE) strscat(eb, eblen, "LUMINANCE+");
	if (c & DDPF_BUMPLUMINANCE) strscat(eb, eblen, "BUMPLUMINANCE+");
	if (c & DDPF_BUMPDUDV) strscat(eb, eblen, "BUMPDUDV+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDPF_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainFlipFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDFLIP_", eblen);
	if (c & DDFLIP_WAIT) strscat(eb, eblen, "WAIT+");
	if (c & DDFLIP_EVEN) strscat(eb, eblen, "EVEN+");
	if (c & DDFLIP_ODD) strscat(eb, eblen, "ODD+");
	if (c & DDFLIP_NOVSYNC) strscat(eb, eblen, "NOVSYNC+");
	if (c & DDFLIP_INTERVAL3) strscat(eb, eblen, "INTERVAL3+");
	else if (c & DDFLIP_INTERVAL2) strscat(eb, eblen, "INTERVAL2+");
	if (c & DDFLIP_INTERVAL4) strscat(eb, eblen, "INTERVAL4+");
	if (c & DDFLIP_STEREO) strscat(eb, eblen, "STEREO+");
	if (c & DDFLIP_DONOTWAIT) strscat(eb, eblen, "DONOTWAIT+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDFLIP_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainBltFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDBLT_", eblen);
	if (c & DDBLT_ALPHADEST) strscat(eb, eblen, "ALPHADEST+");
	if (c & DDBLT_ALPHADESTCONSTOVERRIDE) strscat(eb, eblen, "ALPHADESTCONSTOVERRIDE+");
	if (c & DDBLT_ALPHADESTNEG) strscat(eb, eblen, "ALPHADESTNEG+");
	if (c & DDBLT_ALPHADESTSURFACEOVERRIDE) strscat(eb, eblen, "ALPHADESTSURFACEOVERRIDE+");
	if (c & DDBLT_ALPHAEDGEBLEND) strscat(eb, eblen, "ALPHAEDGEBLEND+");
	if (c & DDBLT_ALPHASRC) strscat(eb, eblen, "ALPHASRC+");
	if (c & DDBLT_ALPHASRCCONSTOVERRIDE) strscat(eb, eblen, "ALPHASRCCONSTOVERRIDE+");
	if (c & DDBLT_ALPHASRCNEG) strscat(eb, eblen, "ALPHASRCNEG+");
	if (c & DDBLT_ALPHASRCSURFACEOVERRIDE) strscat(eb, eblen, "ALPHASRCSURFACEOVERRIDE+");
	if (c & DDBLT_ASYNC) strscat(eb, eblen, "ASYNC+");
	if (c & DDBLT_COLORFILL) strscat(eb, eblen, "COLORFILL+");
	if (c & DDBLT_DDFX) strscat(eb, eblen, "DDFX+");
	if (c & DDBLT_DDROPS) strscat(eb, eblen, "DDROPS+");
	if (c & DDBLT_KEYDEST) strscat(eb, eblen, "KEYDEST+");
	if (c & DDBLT_KEYDESTOVERRIDE) strscat(eb, eblen, "KEYDESTOVERRIDE+");
	if (c & DDBLT_KEYSRC) strscat(eb, eblen, "KEYSRC+");
	if (c & DDBLT_KEYSRCOVERRIDE) strscat(eb, eblen, "KEYSRCOVERRIDE+");
	if (c & DDBLT_ROP) strscat(eb, eblen, "ROP+");
	if (c & DDBLT_ROTATIONANGLE) strscat(eb, eblen, "ROTATIONANGLE+");
	if (c & DDBLT_ZBUFFER) strscat(eb, eblen, "ZBUFFER+");
	if (c & DDBLT_ZBUFFERDESTCONSTOVERRIDE) strscat(eb, eblen, "ZBUFFERDESTCONSTOVERRIDE+");
	if (c & DDBLT_ZBUFFERDESTOVERRIDE) strscat(eb, eblen, "ZBUFFERDESTOVERRIDE+");
	if (c & DDBLT_ZBUFFERSRCCONSTOVERRIDE) strscat(eb, eblen, "ZBUFFERSRCCONSTOVERRIDE+");
	if (c & DDBLT_ZBUFFERSRCOVERRIDE) strscat(eb, eblen, "ZBUFFERSRCOVERRIDE+");
	if (c & DDBLT_WAIT) strscat(eb, eblen, "WAIT+");
	if (c & DDBLT_DEPTHFILL) strscat(eb, eblen, "DEPTHFILL+");
	if (c & DDBLT_DONOTWAIT) strscat(eb, eblen, "DONOTWAIT+");
	if (c & DDBLT_ROTATIONANGLE) strscat(eb, eblen, "ROTATIONANGLE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDBLT_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainBltFastFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDBLTFAST_", eblen);
	if (!(c & (DDBLTFAST_SRCCOLORKEY|DDBLTFAST_DESTCOLORKEY))) strscat(eb, eblen, "NOCOLORKEY+");
	if (c & DDBLTFAST_SRCCOLORKEY) strscat(eb, eblen, "SRCCOLORKEY+");
	if (c & DDBLTFAST_DESTCOLORKEY) strscat(eb, eblen, "DESTCOLORKEY+");
	if (c & DDBLTFAST_WAIT) strscat(eb, eblen, "WAIT+");
	if (c & DDBLTFAST_DONOTWAIT) strscat(eb, eblen, "DONOTWAIT+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDBLTFAST_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

#define DDPCAPS_INITIALIZE_LEGACY 0x00000008l

char *ExplainCreatePaletteFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDPCAPS_", eblen);
	if (c & DDPCAPS_4BIT) strscat(eb, eblen, "4BIT+");
	if (c & DDPCAPS_8BITENTRIES) strscat(eb, eblen, "8BITENTRIES+");
	if (c & DDPCAPS_8BIT) strscat(eb, eblen, "8BIT+");
	//if (c & DDPCAPS_INITIALIZE) strscat(eb, eblen, "INITIALIZE+");
	// DDPCAPS_INITIALIZE is obsolete and redefined to 0x0, but that is not the legacy value embedded in assembly!
	if (c & DDPCAPS_INITIALIZE_LEGACY) strscat(eb, eblen, "INITIALIZE+");
	if (c & DDPCAPS_PRIMARYSURFACE) strscat(eb, eblen, "PRIMARYSURFACE+");
	if (c & DDPCAPS_PRIMARYSURFACELEFT) strscat(eb, eblen, "PRIMARYSURFACELEFT+");
	if (c & DDPCAPS_ALLOW256) strscat(eb, eblen, "ALLOW256+");
	if (c & DDPCAPS_VSYNC) strscat(eb, eblen, "VSYNC+");
	if (c & DDPCAPS_1BIT) strscat(eb, eblen, "1BIT+");
	if (c & DDPCAPS_2BIT) strscat(eb, eblen, "2BIT+");
	if (c & DDPCAPS_ALPHA) strscat(eb, eblen, "ALPHA+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDPCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainROP(DWORD c)
{
	char *eb;
	switch(c){
		case SRCCOPY: eb="SRCCOPY"; break; 
		case SRCPAINT: eb="SRCPAINT"; break; 
		case SRCAND: eb="SRCAND"; break; 
		case SRCINVERT: eb="SRCINVERT"; break; 
		case SRCERASE: eb="SRCERASE"; break; 
		case NOTSRCCOPY: eb="NOTSRCCOPY"; break; 
		case NOTSRCERASE: eb="NOTSRCERASE"; break; 
		case MERGECOPY: eb="MERGECOPY"; break; 
		case MERGEPAINT: eb="MERGEPAINT"; break; 
		case PATCOPY: eb="PATCOPY"; break; 
		case PATPAINT: eb="PATPAINT"; break; 
		case PATINVERT: eb="PATINVERT"; break; 
		case DSTINVERT: eb="DSTINVERT"; break; 
		case BLACKNESS: eb="BLACKNESS"; break; 
		case WHITENESS: eb="WHITENESS"; break; 
		case NOMIRRORBITMAP: eb="NOMIRRORBITMAP"; break; 
		case CAPTUREBLT: eb="CAPTUREBLT"; break;
			// from Charles Petzold "Programming Windows 95"
		case 0x0500A9: eb="~(P | D)"; break;
		case 0x0A0329: eb="~P & D"; break;
		case 0x0F0001: eb="~P"; break;
		case 0x500325: eb="P & ~D"; break;
		case 0x5F00E9: eb="~(P & D)"; break;
		case 0xA000C9: eb="P & D"; break;
		case 0xA50065: eb="~(P ^ D)"; break;
		case 0xAA0029: eb="D"; break;
		case 0xAF0229: eb="~P | D"; break;
		case 0xF50225: eb="P | ~D"; break;
		case 0xFA0089: eb="P | D "; break;
		default: eb="unknown"; break; 
	}
	return(eb);
}

char *ExplainLockFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDLOCK_", eblen);
	if (c & DDLOCK_WAIT) strscat(eb, eblen, "WAIT+");
	if (c & DDLOCK_EVENT) strscat(eb, eblen, "EVENT+");
	if (c & DDLOCK_READONLY) strscat(eb, eblen, "READONLY+");
	if (c & DDLOCK_WRITEONLY) strscat(eb, eblen, "WRITEONLY+");
	if (c & DDLOCK_NOSYSLOCK) strscat(eb, eblen, "NOSYSLOCK+");
	if (c & DDLOCK_NOOVERWRITE) strscat(eb, eblen, "NOOVERWRITE+");
	if (c & DDLOCK_DISCARDCONTENTS) strscat(eb, eblen, "DISCARDCONTENTS+");
	if (c & DDLOCK_DONOTWAIT) strscat(eb, eblen, "DONOTWAIT+");
	if (c & DDLOCK_HASVOLUMETEXTUREBOXRECT) strscat(eb, eblen, "HASVOLUMETEXTUREBOXRECT+");
	if (c & DDLOCK_NODIRTYUPDATE) strscat(eb, eblen, "NODIRTYUPDATE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDLOCK_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb,"DDLOCK_SURFACEMEMORYPTR", eblen); // when zero...
	}
	return(eb);
}

char *ExplainStyle(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"WS_", eblen);
	// v2.05.79 fix: WS_CAPTION = WS_DLGFRAME|WS_BORDER !!
	if ((c & WS_CAPTION) == WS_CAPTION) strscat(eb, eblen, "CAPTION+");
	else{
		if (c & WS_DLGFRAME) strscat(eb, eblen, "DLGFRAME+");
		if (c & WS_BORDER) strscat(eb, eblen, "BORDER+");
	}
	if (c & WS_CHILD) strscat(eb, eblen, "CHILD+");
	if (c & WS_CLIPCHILDREN) strscat(eb, eblen, "CLIPCHILDREN+");
	if (c & WS_CLIPSIBLINGS) strscat(eb, eblen, "CLIPSIBLINGS+");
	if (c & WS_DISABLED) strscat(eb, eblen, "DISABLED+");
	if (c & WS_GROUP) strscat(eb, eblen, "GROUP+");
	if (c & WS_HSCROLL) strscat(eb, eblen, "HSCROLL+");
	if (c & WS_MAXIMIZE) strscat(eb, eblen, "MAXIMIZE+");
	if (c & WS_MAXIMIZEBOX) strscat(eb, eblen, "MAXIMIZEBOX+");
	if (c & WS_MINIMIZE) strscat(eb, eblen, "MINIMIZE+");
	if (c & WS_MINIMIZEBOX) strscat(eb, eblen, "MINIMIZEBOX+");
	if (c & WS_POPUP) strscat(eb, eblen, "POPUP+");
	if (c & WS_SIZEBOX) strscat(eb, eblen, "SIZEBOX+");
	if (c & WS_SYSMENU) strscat(eb, eblen, "SYSMENU+");
	if (c & WS_TABSTOP) strscat(eb, eblen, "TABSTOP+");
	if (c & WS_THICKFRAME) strscat(eb, eblen, "THICKFRAME+");
	if (c & WS_TILED) strscat(eb, eblen, "TILED+");
	if (c & WS_VISIBLE) strscat(eb, eblen, "VISIBLE+");
	if (c & WS_VSCROLL) strscat(eb, eblen, "VSCROLL+");
	l=strlen(eb);
	if (l>strlen("WS_")) eb[l-1]=0; // delete last '+' if any
	else strncpy(eb,"WS_OVERLAPPED", eblen); // when zero ...

	char *s = &eb[strlen(eb)];
	strcat(s, "+CS_");
	if (c & CS_VREDRAW) strscat(eb, eblen, "VREDRAW+");
	if (c & CS_HREDRAW) strscat(eb, eblen, "HREDRAW+");
	if (c & CS_DBLCLKS) strscat(eb, eblen, "DBLCLKS+");
	if (c & CS_OWNDC) strscat(eb, eblen, "OWNDC+");
	if (c & CS_CLASSDC) strscat(eb, eblen, "CLASSDC+");
	if (c & CS_PARENTDC) strscat(eb, eblen, "PARENTDC+");
	if (c & CS_NOCLOSE) strscat(eb, eblen, "NOCLOSE+");
	if (c & CS_SAVEBITS) strscat(eb, eblen, "SAVEBITS+");
	if (c & CS_BYTEALIGNCLIENT) strscat(eb, eblen, "BYTEALIGNCLIENT+");
	if (c & CS_BYTEALIGNWINDOW) strscat(eb, eblen, "BYTEALIGNWINDOW+");
	if (c & CS_GLOBALCLASS) strscat(eb, eblen, "GLOBALCLASS+");
	if (c & CS_IME) strscat(eb, eblen, "IME+");
	if (c & CS_DROPSHADOW) strscat(eb, eblen, "DROPSHADOW+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		l=strlen(s);
		if (l>strlen("+CS_")) s[l-1]=0; // delete last '+' if any
		else *s = 0; // when zero ...
	}
	return(eb);
}

char *ExplainExStyle(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"WS_EX_", eblen);
	if (c & WS_EX_ACCEPTFILES) strscat(eb, eblen, "ACCEPTFILES+");
	if (c & WS_EX_APPWINDOW) strscat(eb, eblen, "APPWINDOW+");
	if (c & WS_EX_CLIENTEDGE) strscat(eb, eblen, "CLIENTEDGE+");
	if (c & WS_EX_COMPOSITED) strscat(eb, eblen, "COMPOSITED+");
	if (c & WS_EX_CONTEXTHELP) strscat(eb, eblen, "CONTEXTHELP+");
	if (c & WS_EX_CONTROLPARENT) strscat(eb, eblen, "CONTROLPARENT+");
	if (c & WS_EX_DLGMODALFRAME) strscat(eb, eblen, "DLGMODALFRAME+");
	if (c & WS_EX_LAYERED) strscat(eb, eblen, "LAYERED+");
	if (c & WS_EX_LAYOUTRTL) strscat(eb, eblen, "LAYOUTRTL+");
	if (c & WS_EX_LEFT) strscat(eb, eblen, "LEFT+");
	if (c & WS_EX_LEFTSCROLLBAR) strscat(eb, eblen, "LEFTSCROLLBAR+");
	if (c & WS_EX_LTRREADING) strscat(eb, eblen, "LTRREADING+");
	if (c & WS_EX_MDICHILD) strscat(eb, eblen, "MDICHILD+");
	if (c & WS_EX_NOACTIVATE) strscat(eb, eblen, "NOACTIVATE+");
	if (c & WS_EX_NOINHERITLAYOUT) strscat(eb, eblen, "NOINHERITLAYOUT+");
	if (c & WS_EX_NOPARENTNOTIFY) strscat(eb, eblen, "NOPARENTNOTIFY+");
	if (c & WS_EX_RIGHT) strscat(eb, eblen, "RIGHT+");
	if (c & WS_EX_RTLREADING) strscat(eb, eblen, "RTLREADING+");
	if (c & WS_EX_STATICEDGE) strscat(eb, eblen, "STATICEDGE+");
	if (c & WS_EX_TOOLWINDOW) strscat(eb, eblen, "TOOLWINDOW+");
	if (c & WS_EX_TOPMOST) strscat(eb, eblen, "TOPMOST+");
	if (c & WS_EX_TRANSPARENT) strscat(eb, eblen, "TRANSPARENT+");
	if (c & WS_EX_WINDOWEDGE) strscat(eb, eblen, "WINDOWEDGE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("WS_EX_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb,"WS_EX_RIGHTSCROLLBAR", eblen); // when zero ...
	}
	return(eb);
}

char *ExplainShowCmd(int c)
{
	char *eb;
	switch(c)
	{
	case SW_HIDE: eb="SW_HIDE"; break; // 0
	case SW_SHOWNORMAL: eb="SW_SHOWNORMAL"; break; // 1
	case SW_SHOWMINIMIZED: eb="SW_SHOWMINIMIZED"; break; // 2
	case SW_MAXIMIZE: eb="SW_MAXIMIZE"; break; // 3
	//case SW_SHOWMAXIMIZED: eb="SW_SHOWMAXIMIZED"; break; // 3 = SW_MAXIMIZE
	case SW_SHOWNOACTIVATE: eb="SW_SHOWNOACTIVATE"; break; // 4
	case SW_SHOW: eb="SW_SHOW"; break; // 5
	case SW_MINIMIZE: eb="SW_MINIMIZE"; break; // 6
	case SW_SHOWMINNOACTIVE: eb="SW_SHOWMINNOACTIVE"; break; // 7
	case SW_SHOWNA: eb="SW_SHOWNA"; break; // 8
	case SW_RESTORE: eb="SW_RESTORE"; break; // 9
	case SW_SHOWDEFAULT: eb="SW_SHOWDEFAULT"; break; // 10
	case SW_FORCEMINIMIZE: eb="SW_FORCEMINIMIZE"; break; // 11
	default: eb="unknown"; break;
	}
	return(eb);
}

char *ExplainBltStatus(DWORD c)
{
	char *eb;
	switch(c){
		case DDGBS_CANBLT:		eb="DDGBS_CANBLT"; break;
		case DDGBS_ISBLTDONE:	eb="DDGBS_ISBLTDONE"; break;
		default:				eb="invalid"; break;
	}
	return(eb);
}

char *ExplainFlipStatus(DWORD c)
{
	char *eb;
	switch(c){
		case DDGFS_CANFLIP:		eb="DDGFS_CANFLIP"; break;
		case DDGFS_ISFLIPDONE:	eb="DDGFS_ISFLIPDONE"; break;
		default:				eb="invalid"; break;
	}
	return(eb);
}


char *ExplainResizing(DWORD c)
{
	char *eb;
	switch(c){
		case SIZE_MAXHIDE: 		 		eb="SIZE_MAXHIDE"; break;
		case SIZE_MAXIMIZED: 		 	eb="SIZE_MAXIMIZED"; break;
		case SIZE_MAXSHOW: 		 		eb="SIZE_MAXSHOW"; break;
		case SIZE_MINIMIZED: 		 	eb="SIZE_MINIMIZED"; break;
		case SIZE_RESTORED: 		 	eb="SIZE_RESTORED"; break;
		default:						eb="???"; break;
	}
	return eb;
}

char *ExplainDeviceCaps(DWORD c)
{
	char *eb;
	switch(c){
		case DRIVERVERSION: 		 	eb="DRIVERVERSION"; break;
		case TECHNOLOGY: 		 		eb="TECHNOLOGY"; break;
		case HORZSIZE: 		 			eb="HORZSIZE"; break;
		case VERTSIZE: 		 			eb="VERTSIZE"; break;
		case HORZRES: 		 			eb="HORZRES"; break;
		case VERTRES: 		 			eb="VERTRES"; break;
		case LOGPIXELSX: 		 		eb="LOGPIXELSX"; break;
		case LOGPIXELSY: 		 		eb="LOGPIXELSY"; break;
		case BITSPIXEL: 		 		eb="BITSPIXEL"; break;
		case PLANES: 		 			eb="PLANES"; break;
		case NUMBRUSHES: 		 		eb="NUMBRUSHES"; break;
		case NUMPENS: 		 			eb="NUMPENS"; break;
		case NUMFONTS: 		 			eb="NUMFONTS"; break;
		case NUMCOLORS: 		 		eb="NUMCOLORS"; break;
		case ASPECTX: 		 			eb="ASPECTX"; break;
		case ASPECTY: 		 			eb="ASPECTY"; break;
		case ASPECTXY: 		 			eb="ASPECTXY"; break;
		case PDEVICESIZE: 		 		eb="PDEVICESIZE"; break;
		case CLIPCAPS: 		 			eb="CLIPCAPS"; break;
		case SIZEPALETTE: 		 		eb="SIZEPALETTE"; break;
		case NUMRESERVED: 		 		eb="NUMRESERVED"; break;
		case COLORRES: 		 			eb="COLORRES"; break;
		case PHYSICALWIDTH: 		 	eb="PHYSICALWIDTH"; break;
		case PHYSICALHEIGHT: 		 	eb="PHYSICALHEIGHT"; break;
		case PHYSICALOFFSETX: 		 	eb="PHYSICALOFFSETX"; break;
		case PHYSICALOFFSETY: 		 	eb="PHYSICALOFFSETY"; break;
		case VREFRESH: 		 			eb="VREFRESH"; break;
		case SCALINGFACTORX: 		 	eb="SCALINGFACTORX"; break;
		case SCALINGFACTORY: 		 	eb="SCALINGFACTORY"; break;
		case BLTALIGNMENT: 		 		eb="BLTALIGNMENT"; break;
		case SHADEBLENDCAPS: 		 	eb="SHADEBLENDCAPS"; break;
		case RASTERCAPS: 		 		eb="RASTERCAPS"; break;
		case CURVECAPS: 		 		eb="CURVECAPS"; break;
		case LINECAPS: 		 			eb="LINECAPS"; break;
		case POLYGONALCAPS: 		 	eb="POLYGONALCAPS"; break;
		case TEXTCAPS: 		 			eb="TEXTCAPS"; break;
		case COLORMGMTCAPS: 		 	eb="COLORMGMTCAPS"; break;
		default:						eb="???"; break;
	}
	return eb;
}

char *ExplainDisplaySettingsRetcode(DWORD c)
{
	char *eb;
	switch(c){
		case DISP_CHANGE_SUCCESSFUL:	eb="DISP_CHANGE_SUCCESSFUL"; break;
		//case DISP_CHANGE_BADDUALVIEW:	eb="DISP_CHANGE_BADDUALVIEW"; break;
		case DISP_CHANGE_BADFLAGS:		eb="DISP_CHANGE_BADFLAGS"; break;
		case DISP_CHANGE_BADMODE:		eb="DISP_CHANGE_BADMODE"; break;
		case DISP_CHANGE_BADPARAM:		eb="DISP_CHANGE_BADPARAM"; break;
		case DISP_CHANGE_FAILED:		eb="DISP_CHANGE_FAILED"; break;
		case DISP_CHANGE_NOTUPDATED:	eb="DISP_CHANGE_NOTUPDATED"; break;
		case DISP_CHANGE_RESTART:		eb="DISP_CHANGE_RESTART"; break;
		default:						eb="???"; break;
	}
	return eb;
}

char *ExplainSetWindowIndex(DWORD c)
{
	char *eb;
	switch(c){
		case GWL_EXSTYLE:		eb="GWL_EXSTYLE"; break;
		case GWL_HINSTANCE:		eb="GWL_HINSTANCE"; break;
		case GWL_ID:			eb="GWL_ID"; break;
		case GWL_STYLE:			eb="GWL_STYLE"; break;
		case GWL_USERDATA:		eb="GWL_USERDATA"; break;
		case GWL_WNDPROC:		eb="GWL_WNDPROC"; break;
		case GWL_HWNDPARENT:	eb="GWL_HWNDPARENT"; break;
		case DWL_DLGPROC:		eb="DWL_DLGPROC"; break;
		case DWL_MSGRESULT:		eb="DWL_MSGRESULT"; break;
		case DWL_USER:			eb="DWL_USER"; break;
		case CB_ERR:            eb="-1"; break;
		case CB_ERRSPACE:       eb="-2"; break;
		default:				eb="???"; break;
	}
	return eb;
} 

char *ExplainColorKeyFlag(DWORD c)
{
	char *eb;
	switch(c){
		case 0:						eb=""; break;
		case DDCKEY_COLORSPACE:		eb="DDCKEY_COLORSPACE"; break;
		case DDCKEY_DESTBLT:		eb="DDCKEY_DESTBLT"; break;
		case DDCKEY_DESTOVERLAY:	eb="DDCKEY_DESTOVERLAY"; break;
		case DDCKEY_SRCBLT:			eb="DDCKEY_SRCBLT"; break;
		case DDCKEY_SRCOVERLAY:		eb="DDCKEY_SRCOVERLAY"; break;
		default:					eb="???"; break;
	}
	return eb;
}

char *ExplainNChitTest(DWORD c)
{
	char *eb;
	switch(c){
		case HTERROR:			eb="HTERROR"; break;
		case HTTRANSPARENT:		eb="HTTRANSPARENT"; break;
		case HTNOWHERE:			eb="HTNOWHERE"; break;
		case HTCLIENT:			eb="HTCLIENT"; break;
		case HTCAPTION:			eb="HTCAPTION"; break;
		case HTSYSMENU:			eb="HTSYSMENU"; break;
		case HTGROWBOX:			eb="HTGROWBOX"; break;
		case HTMENU:			eb="HTMENU"; break;
		case HTHSCROLL:			eb="HTHSCROLL"; break;
		case HTVSCROLL:			eb="HTVSCROLL"; break;
		case HTMINBUTTON:		eb="HTMINBUTTON"; break;
		case HTMAXBUTTON:		eb="HTMAXBUTTON"; break;
		case HTLEFT:			eb="HTLEFT"; break;
		case HTRIGHT:			eb="HTRIGHT"; break;
		case HTTOP:				eb="HTTOP"; break;
		case HTTOPLEFT:			eb="HTTOPLEFT"; break;
		case HTTOPRIGHT:		eb="HTTOPRIGHT"; break;
		case HTBOTTOM:			eb="HTBOTTOM"; break;
		case HTBOTTOMLEFT:		eb="HTBOTTOMLEFT"; break;
		case HTBOTTOMRIGHT:		eb="HTBOTTOMRIGHT"; break;
		case HTBORDER:			eb="HTBORDER"; break;
		case HTOBJECT:			eb="HTOBJECT"; break;
		case HTCLOSE:			eb="HTCLOSE"; break;
		case HTHELP:			eb="HTHELP"; break;
		default:				eb="???"; break;
	}
	return eb;
}

char *ExplainDDEnumerateFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDENUM_", eblen);
	if (c & DDENUM_ATTACHEDSECONDARYDEVICES) strscat(eb, eblen, "ATTACHEDSECONDARYDEVICES+");
	if (c & DDENUM_DETACHEDSECONDARYDEVICES) strscat(eb, eblen, "DETACHEDSECONDARYDEVICES+");
	if (c & DDENUM_NONDISPLAYDEVICES) strscat(eb, eblen, "NONDISPLAYDEVICES+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDENUM_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}

 char *ExplainsSystemMetrics(DWORD c)
{
	char *Captions[94]={
		"SM_CXSCREEN", "SM_CYSCREEN", "SM_CXVSCROLL", "SM_CYHSCROLL", "SM_CYCAPTION",
		"SM_CXBORDER", "SM_CYBORDER", "SM_CXDLGFRAME", "SM_CYDLGFRAME", "SM_CYVTHUMB",
		"SM_CXHTHUMB", "SM_CXICON", "SM_CYICON", "SM_CXCURSOR", "SM_CYCURSOR",
		"SM_CYMENU", "SM_CXFULLSCREEN", "SM_CYFULLSCREEN", "SM_CYKANJIWINDOW", "SM_MOUSEPRESENT",
		"SM_CYVSCROLL", "SM_CXHSCROLL", "SM_DEBUG", "SM_SWAPBUTTON", "SM_RESERVED1",
		"SM_RESERVED2", "SM_RESERVED3", "SM_RESERVED4", "SM_CXMIN", "SM_CYMIN",
		"SM_CXSIZE", "SM_CYSIZE", "SM_CXFRAME", "SM_CYFRAME", "SM_CXMINTRACK",
		"SM_CYMINTRACK", "SM_CXDOUBLECLK", "SM_CYDOUBLECLK", "SM_CXICONSPACING", "SM_CYICONSPACING",
		"SM_MENUDROPALIGNMENT", "SM_PENWINDOWS", "SM_DBCSENABLED", "SM_CMOUSEBUTTONS", "SM_SECURE",
		"SM_CXEDGE", "SM_CYEDGE", "SM_CXMINSPACING", "SM_CYMINSPACING", "SM_CXSMICON",
		"SM_CYSMICON", "SM_CYSMCAPTION", "SM_CXSMSIZE", "SM_CYSMSIZE", "SM_CXMENUSIZE",
		"SM_CYMENUSIZE", "SM_ARRANGE", "SM_CXMINIMIZED", "SM_CYMINIMIZED", "SM_CXMAXTRACK",
		"SM_CYMAXTRACK", "SM_CXMAXIMIZED", "SM_CYMAXIMIZED", "SM_NETWORK", "64???",
		"65???", "66???", "SM_CLEANBOOT", "SM_CXDRAG", "SM_CYDRAG",
		"SM_SHOWSOUNDS", "SM_CXMENUCHECK", "SM_CYMENUCHECK", "SM_SLOWMACHINE", "SM_MIDEASTENABLED",
		"SM_MOUSEWHEELPRESENT", "SM_XVIRTUALSCREEN", "SM_YVIRTUALSCREEN", "SM_CXVIRTUALSCREEN", "SM_CYVIRTUALSCREEN",
		"SM_CMONITORS", "SM_SAMEDISPLAYFORMAT", "SM_IMMENABLED", "SM_CXFOCUSBORDER", "SM_CYFOCUSBORDER",
		"85???", "SM_TABLETPC", "SM_MEDIACENTER", "SM_STARTER", "SM_SERVERR2",
		"SM_CMETRICS(Win 0x501)", "SM_MOUSEHORIZONTALWHEELPRESENT", "SM_CXPADDEDBORDER", "SM_CMETRICS"};

	if (c>=0 && c<94) return Captions[c];
	switch(c){
		case SM_REMOTESESSION: return "SM_REMOTESESSION";
		case SM_SHUTTINGDOWN: return "SM_SHUTTINGDOWN";
		case SM_REMOTECONTROL: return "SM_REMOTECONTROL";
		case SM_CARETBLINKINGENABLED: return "SM_CARETBLINKINGENABLED";
	}
	return "???";
 }

char *ExplainWPFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"SWP_", eblen);
	if (c & SWP_NOSIZE) strscat(eb, eblen, "NOSIZE+");
	if (c & SWP_NOMOVE) strscat(eb, eblen, "NOMOVE+");
	if (c & SWP_NOZORDER) strscat(eb, eblen, "NOZORDER+");
	if (c & SWP_NOREDRAW) strscat(eb, eblen, "NOREDRAW+");
	if (c & SWP_NOACTIVATE) strscat(eb, eblen, "NOACTIVATE+");
	if (c & SWP_FRAMECHANGED) strscat(eb, eblen, "FRAMECHANGED+");
	if (c & SWP_SHOWWINDOW) strscat(eb, eblen, "SHOWWINDOW+");
	if (c & SWP_HIDEWINDOW) strscat(eb, eblen, "HIDEWINDOW+");
	if (c & SWP_NOCOPYBITS) strscat(eb, eblen, "NOCOPYBITS+");
	if (c & SWP_NOOWNERZORDER) strscat(eb, eblen, "NOOWNERZORDER+");
	if (c & SWP_NOSENDCHANGING) strscat(eb, eblen, "NOSENDCHANGING+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("SWP_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}

char *ExplainLoadLibFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"", eblen);
	if (c & DONT_RESOLVE_DLL_REFERENCES) strscat(eb, eblen, "DONT_RESOLVE_DLL_REFERENCES+");
	if (c & LOAD_LIBRARY_AS_DATAFILE) strscat(eb, eblen, "LOAD_LIBRARY_AS_DATAFILE+");
	if (c & LOAD_WITH_ALTERED_SEARCH_PATH) strscat(eb, eblen, "LOAD_WITH_ALTERED_SEARCH_PATH+");
	if (c & LOAD_IGNORE_CODE_AUTHZ_LEVEL) strscat(eb, eblen, "LOAD_IGNORE_CODE_AUTHZ_LEVEL+");
	if (c & LOAD_LIBRARY_AS_IMAGE_RESOURCE) strscat(eb, eblen, "LOAD_LIBRARY_AS_IMAGE_RESOURCE+");
	if (c & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) strscat(eb, eblen, "LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE+");
	l=strlen(eb);
	if (l>0) eb[l-1]=0; // delete last '+' if any
	else strncpy(eb, "NULL", eblen);
	return(eb);
}

char *ExplainDevModeFields(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DM_", eblen);
	if (c & DM_ORIENTATION) strscat(eb, eblen, "ORIENTATION+");
	if (c & DM_PAPERSIZE) strscat(eb, eblen, "PAPERSIZE+");
	if (c & DM_PAPERLENGTH) strscat(eb, eblen, "PAPERLENGTH+");
	if (c & DM_PAPERWIDTH) strscat(eb, eblen, "PAPERWIDTH+");
	if (c & DM_SCALE) strscat(eb, eblen, "SCALE+");
	if (c & DM_COPIES) strscat(eb, eblen, "COPIES+");
	if (c & DM_DEFAULTSOURCE) strscat(eb, eblen, "DEFAULTSOURCE+");
	if (c & DM_PRINTQUALITY) strscat(eb, eblen, "PRINTQUALITY+");
	if (c & DM_POSITION) strscat(eb, eblen, "POSITION+");
	if (c & DM_DISPLAYORIENTATION) strscat(eb, eblen, "DISPLAYORIENTATION+");
	if (c & DM_DISPLAYFIXEDOUTPUT) strscat(eb, eblen, "DISPLAYFIXEDOUTPUT+");
	if (c & DM_COLOR) strscat(eb, eblen, "COLOR+");
	if (c & DM_DUPLEX) strscat(eb, eblen, "DUPLEX+");
	if (c & DM_YRESOLUTION) strscat(eb, eblen, "YRESOLUTION+");
	if (c & DM_TTOPTION) strscat(eb, eblen, "TTOPTION+");
	if (c & DM_COLLATE) strscat(eb, eblen, "COLLATE+");
	if (c & DM_FORMNAME) strscat(eb, eblen, "FORMNAME+");
	if (c & DM_LOGPIXELS) strscat(eb, eblen, "LOGPIXELS+");
	if (c & DM_BITSPERPEL) strscat(eb, eblen, "BITSPERPEL+");
	if (c & DM_PELSWIDTH) strscat(eb, eblen, "PELSWIDTH+");
	if (c & DM_PELSHEIGHT) strscat(eb, eblen, "PELSHEIGHT+");
	if (c & DM_DISPLAYFLAGS) strscat(eb, eblen, "DISPLAYFLAGS+");
	if (c & DM_NUP) strscat(eb, eblen, "NUP+");
	if (c & DM_DISPLAYFREQUENCY) strscat(eb, eblen, "DISPLAYFREQUENCY+");
	if (c & DM_ICMMETHOD) strscat(eb, eblen, "ICMMETHOD+");
	if (c & DM_ICMINTENT) strscat(eb, eblen, "ICMINTENT+");
	if (c & DM_MEDIATYPE) strscat(eb, eblen, "MEDIATYPE+");
	if (c & DM_DITHERTYPE) strscat(eb, eblen, "DITHERTYPE+");
	if (c & DM_PANNINGWIDTH) strscat(eb, eblen, "PANNINGWIDTH+");
	if (c & DM_PANNINGHEIGHT) strscat(eb, eblen, "PANNINGHEIGHT+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DM_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}

char *ExplainRegType(DWORD c)
{
	char *Captions[12]={
		"REG_NONE", "REG_SZ", "REG_EXPAND_SZ", "REG_BINARY", 
		"REG_DWORD", "REG_DWORD_BIG_ENDIAN", "REG_LINK", "REG_MULTI_SZ", 
		"REG_RESOURCE_LIST", "REG_FULL_RESOURCE_DESCRIPTOR",
		"REG_RESOURCE_REQUIREMENTS_LIST", "REG_QWORD"};

	if (c>=0 && c<12) return Captions[c];
	return "???";
}

char *ExplainDCType(DWORD c)
{
	char *Captions[GDI_OBJ_LAST+1]={
		"NULL", "OBJ_PEN", "OBJ_BRUSH", "OBJ_DC", 
		"OBJ_METADC", "OBJ_PAL", "OBJ_FONT", "OBJ_BITMAP", 
		"OBJ_REGION", "OBJ_METAFILE", "OBJ_MEMDC", "OBJ_EXTPEN",
		"OBJ_ENHMETADC", "OBJ_ENHMETAFILE", "OBJ_COLORSPACE"};

	if (c>=0 && c<=GDI_OBJ_LAST) return Captions[c];
	return "???";
}

char *ExplainPeekRemoveMsg(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, (c & PM_REMOVE) ? "PM_REMOVE" : "PM_NOREMOVE", eblen);
	if(c & PM_NOYIELD) strscat(eb, eblen, "+NOYIELD");
	c >>= 16;
	if(c & QS_MOUSEMOVE) strscat(eb, eblen, "+MOUSEMOVE");
	if(c & QS_MOUSEBUTTON) strscat(eb, eblen, "+MOUSEBUTTON");
	if(c & QS_KEY) strscat(eb, eblen, "+KEY");
	if(c & QS_RAWINPUT) strscat(eb, eblen, "+RAWINPUT");
	if(c & QS_PAINT) strscat(eb, eblen, "+PAINT");
	if(c & QS_POSTMESSAGE) strscat(eb, eblen, "+POSTMESSAGE");
	if(c & QS_HOTKEY) strscat(eb, eblen, "+HOTKEY");
	if(c & QS_TIMER) strscat(eb, eblen, "+TIMER");
	if(c & QS_SENDMESSAGE) strscat(eb, eblen, "+SENDMESSAGE");
	if(c & QS_ALLPOSTMESSAGE) strscat(eb, eblen, "+ALLPOSTMESSAGE");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	return(eb);
}

char *ExplainGetDCExFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DCX_", eblen);
	if(c & DCX_WINDOW) strscat(eb, eblen, "WINDOW+");
	if(c & DCX_CACHE) strscat(eb, eblen, "CACHE+");
	if(c & DCX_PARENTCLIP) strscat(eb, eblen, "PARENTCLIP+");
	if(c & DCX_CLIPSIBLINGS) strscat(eb, eblen, "CLIPSIBLINGS+");
	if(c & DCX_CLIPCHILDREN) strscat(eb, eblen, "CLIPCHILDREN+");
	if(c & DCX_NORESETATTRS) strscat(eb, eblen, "NORESETATTRS+");
	if(c & DCX_EXCLUDERGN) strscat(eb, eblen, "EXCLUDERGN+");
	if(c & DCX_EXCLUDEUPDATE) strscat(eb, eblen, "EXCLUDEUPDATE+");
	if(c & DCX_INTERSECTRGN) strscat(eb, eblen, "INTERSECTRGN+");
	if(c & DCX_INTERSECTUPDATE) strscat(eb, eblen, "INTERSECTUPDATE+");
	if(c & DCX_VALIDATE) strscat(eb, eblen, "VALIDATE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DCX_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}

char *ExplainPaletteUse(UINT uUsage)
{
	char *eb = "SYSPAL_ERROR";
	switch(uUsage){
		case SYSPAL_STATIC: eb="SYSPAL_STATIC"; break;
		case SYSPAL_NOSTATIC: eb="SYSPAL_NOSTATIC"; break;
		case SYSPAL_NOSTATIC256: eb="SYSPAL_NOSTATIC256"; break;
	}
	return eb;
}

char *ExplainRasterCaps(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"RC_", eblen);
	if(c & RC_BITBLT) strscat(eb, eblen, "BITBLT+");
	if(c & RC_BANDING) strscat(eb, eblen, "BANDING+");
	if(c & RC_SCALING) strscat(eb, eblen, "SCALING+");
	if(c & RC_BITMAP64) strscat(eb, eblen, "BITMAP64+");
	if(c & RC_GDI20_OUTPUT) strscat(eb, eblen, "GDI20_OUTPUT+");
	if(c & RC_GDI20_STATE) strscat(eb, eblen, "GDI20_STATE+");
	if(c & RC_SAVEBITMAP) strscat(eb, eblen, "SAVEBITMAP+");
	if(c & RC_DI_BITMAP) strscat(eb, eblen, "DI_BITMAP+");
	if(c & RC_PALETTE) strscat(eb, eblen, "PALETTE+");
	if(c & RC_DIBTODEV) strscat(eb, eblen, "DIBTODEV+");
	if(c & RC_BIGFONT) strscat(eb, eblen, "BIGFONT+");
	if(c & RC_STRETCHBLT) strscat(eb, eblen, "STRETCHBLT+");
	if(c & RC_FLOODFILL) strscat(eb, eblen, "FLOODFILL+");
	if(c & RC_STRETCHDIB) strscat(eb, eblen, "STRETCHDIB+");
	if(c & RC_OP_DX_OUTPUT) strscat(eb, eblen, "OP_DX_OUTPUT+");
	if(c & RC_DEVBITS) strscat(eb, eblen, "DEVBITS+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("RC_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);	
}

#ifndef D3DRS_SOFTWAREVERTEXPROCESSING
typedef enum _D3DRENDERSTATETYPE_D3D8 {
	D3DRS_SOFTWAREVERTEXPROCESSING  = 153,
    D3DRS_POINTSIZE                 = 154,   /* float point size */
    D3DRS_POINTSIZE_MIN             = 155,   /* float point size min threshold */
    D3DRS_POINTSPRITEENABLE         = 156,   /* BOOL point texture coord control */
    D3DRS_POINTSCALEENABLE          = 157,   /* BOOL point size scale enable */
    D3DRS_POINTSCALE_A              = 158,   /* float point attenuation A value */
    D3DRS_POINTSCALE_B              = 159,   /* float point attenuation B value */
    D3DRS_POINTSCALE_C              = 160,   /* float point attenuation C value */
    D3DRS_MULTISAMPLEANTIALIAS      = 161,  // BOOL - set to do FSAA with multisample buffer
    D3DRS_MULTISAMPLEMASK           = 162,  // DWORD - per-sample enable/disable
    D3DRS_PATCHEDGESTYLE            = 163,  // Sets whether patch edges will use float style tessellation
    D3DRS_PATCHSEGMENTS             = 164,  // Number of segments per edge when drawing patches
    D3DRS_DEBUGMONITORTOKEN         = 165,  // DEBUG ONLY - token to debug monitor
    D3DRS_POINTSIZE_MAX             = 166,   /* float point size max threshold */
    D3DRS_INDEXEDVERTEXBLENDENABLE  = 167,
    D3DRS_COLORWRITEENABLE          = 168,  // per-channel write enable
    D3DRS_TWEENFACTOR               = 170,   // float tween factor
    D3DRS_BLENDOP                   = 171,   // D3DBLENDOP setting
    D3DRS_POSITIONORDER             = 172,   // NPatch position interpolation order. D3DORDER_LINEAR or D3DORDER_CUBIC (default)
    D3DRS_NORMALORDER               = 173,   // NPatch normal interpolation order. D3DORDER_LINEAR (default) or D3DORDER_QUADRATIC

    D3DRS_FORCE_DWORD               = 0x7fffffff, /* force 32-bit size enum */
} D3DRENDERSTATETYPE_D3D8;
#endif

char *ExplainD3DRenderState(DWORD c)
{
	char *p;
	if((c>=D3DRENDERSTATE_STIPPLEPATTERN00) && (c<=D3DRENDERSTATE_STIPPLEPATTERN31)) p="STIPPLEPATTERNnn";
	else
	if((c>=D3DRENDERSTATE_WRAP0) && (c<=D3DRENDERSTATE_WRAP7)) p="WRAPn";
	else
	switch(c){
		case D3DRENDERSTATE_ANTIALIAS: p="ANTIALIAS"; break;
		case D3DRENDERSTATE_TEXTUREPERSPECTIVE: p="TEXTUREPERSPECTIVE"; break;
		case D3DRENDERSTATE_ZENABLE: p="ZENABLE"; break;
		case D3DRENDERSTATE_FILLMODE: p="FILLMODE"; break;
		case D3DRENDERSTATE_SHADEMODE: p="SHADEMODE"; break;
		case D3DRENDERSTATE_LINEPATTERN: p="LINEPATTERN"; break;
		case D3DRENDERSTATE_ZWRITEENABLE: p="ZWRITEENABLE"; break;
		case D3DRENDERSTATE_ALPHATESTENABLE: p="ALPHATESTENABLE"; break;
		case D3DRENDERSTATE_LASTPIXEL: p="LASTPIXEL"; break;
		case D3DRENDERSTATE_SRCBLEND: p="SRCBLEND"; break;
		case D3DRENDERSTATE_DESTBLEND: p="DESTBLEND"; break;
		case D3DRENDERSTATE_CULLMODE: p="CULLMODE"; break;
		case D3DRENDERSTATE_ZFUNC: p="ZFUNC"; break;
		case D3DRENDERSTATE_ALPHAREF: p="ALPHAREF"; break;
		case D3DRENDERSTATE_ALPHAFUNC: p="ALPHAFUNC"; break;
		case D3DRENDERSTATE_DITHERENABLE: p="DITHERENABLE"; break;
		case D3DRENDERSTATE_ALPHABLENDENABLE: p="ALPHABLENDENABLE"; break;
		case D3DRENDERSTATE_FOGENABLE: p="FOGENABLE"; break;
		case D3DRENDERSTATE_SPECULARENABLE: p="SPECULARENABLE"; break;
		case D3DRENDERSTATE_ZVISIBLE: p="ZVISIBLE"; break;
		case D3DRENDERSTATE_STIPPLEDALPHA: p="STIPPLEDALPHA"; break;
		case D3DRENDERSTATE_FOGCOLOR: p="FOGCOLOR"; break;
		case D3DRENDERSTATE_FOGTABLEMODE: p="FOGTABLEMODE"; break;
		case D3DRENDERSTATE_FOGSTART: p="FOGSTART"; break;
		case D3DRENDERSTATE_FOGEND: p="FOGEND"; break;
		case D3DRENDERSTATE_FOGDENSITY: p="FOGDENSITY"; break;
		case D3DRENDERSTATE_EDGEANTIALIAS: p="EDGEANTIALIAS"; break;
		case D3DRENDERSTATE_COLORKEYENABLE: p="COLORKEYENABLE"; break;
		case D3DRENDERSTATE_STENCILENABLE: p="STENCILENABLE"; break;
		case D3DRENDERSTATE_ZBIAS: p="ZBIAS"; break;
		case D3DRENDERSTATE_RANGEFOGENABLE: p="RANGEFOGENABLE"; break;
		case D3DRENDERSTATE_STENCILFAIL: p="STENCILFAIL"; break;
		case D3DRENDERSTATE_STENCILZFAIL: p="STENCILZFAIL"; break;
		case D3DRENDERSTATE_STENCILPASS: p="STENCILPASS"; break;
		case D3DRENDERSTATE_STENCILFUNC: p="STENCILFUNC"; break;
		case D3DRENDERSTATE_STENCILREF: p="STENCILREF"; break;
		case D3DRENDERSTATE_STENCILMASK: p="STENCILMASK"; break;
		case D3DRENDERSTATE_STENCILWRITEMASK: p="STENCILWRITEMASK"; break;
		case D3DRENDERSTATE_TEXTUREFACTOR: p="TEXTUREFACTOR"; break;
		case D3DRENDERSTATE_CLIPPING: p="CLIPPING"; break;
		case D3DRENDERSTATE_LIGHTING: p="LIGHTING"; break;
		case D3DRENDERSTATE_EXTENTS: p="EXTENTS"; break;
		case D3DRENDERSTATE_AMBIENT: p="AMBIENT"; break;
		case D3DRENDERSTATE_FOGVERTEXMODE: p="FOGVERTEXMODE"; break;
		case D3DRENDERSTATE_COLORVERTEX: p="COLORVERTEX"; break;
		case D3DRENDERSTATE_LOCALVIEWER: p="LOCALVIEWER"; break;
		case D3DRENDERSTATE_NORMALIZENORMALS: p="NORMALIZENORMALS"; break;
		case D3DRENDERSTATE_COLORKEYBLENDENABLE: p="COLORKEYBLENDENABLE"; break;
		case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE: p="DIFFUSEMATERIALSOURCE"; break;
		case D3DRENDERSTATE_SPECULARMATERIALSOURCE: p="SPECULARMATERIALSOURCE"; break;
		case D3DRENDERSTATE_AMBIENTMATERIALSOURCE: p="AMBIENTMATERIALSOURCE"; break;
		case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE: p="EMISSIVEMATERIALSOURCE"; break;
		case D3DRENDERSTATE_VERTEXBLEND: p="VERTEXBLEND"; break;
		case D3DRENDERSTATE_CLIPPLANEENABLE: p="CLIPPLANEENABLE"; break;

		// D3D8 only

		case D3DRS_SOFTWAREVERTEXPROCESSING: p="SOFTWAREVERTEXPROCESSING"; break;
		case D3DRS_POINTSIZE: p="POINTSIZE"; break;
		case D3DRS_POINTSIZE_MIN: p="POINTSIZE_MIN"; break;
		case D3DRS_POINTSPRITEENABLE: p="POINTSPRITEENABLE"; break;
		case D3DRS_POINTSCALEENABLE: p="POINTSCALEENABLE"; break;
		case D3DRS_POINTSCALE_A: p="POINTSCALE_A"; break;
		case D3DRS_POINTSCALE_B: p="POINTSCALE_B"; break;
		case D3DRS_POINTSCALE_C: p="POINTSCALE_C"; break;
		case D3DRS_MULTISAMPLEANTIALIAS: p="MULTISAMPLEANTIALIAS"; break;
		case D3DRS_MULTISAMPLEMASK: p="MULTISAMPLEMASK"; break;
		case D3DRS_PATCHEDGESTYLE: p="PATCHEDGESTYLE"; break;
		case D3DRS_PATCHSEGMENTS: p="PATCHSEGMENTS"; break; // Number of segments per edge when drawing patches
		case D3DRS_DEBUGMONITORTOKEN: p="DEBUGMONITORTOKEN"; break; // DEBUG ONLY - token to debug monitor
		case D3DRS_POINTSIZE_MAX: p="POINTSIZE_MAX"; break;  /* float point size max threshold */
		case D3DRS_INDEXEDVERTEXBLENDENABLE: p="INDEXEDVERTEXBLENDENABLE"; break;
		case D3DRS_COLORWRITEENABLE: p="COLORWRITEENABLE"; break; // per-channel write enable
		case D3DRS_TWEENFACTOR: p="TWEENFACTOR"; break; // float tween factor
		case D3DRS_BLENDOP: p="BLENDOP"; break; // D3DBLENDOP setting
		case D3DRS_POSITIONORDER: p="POSITIONORDER"; break; // NPatch position interpolation order. D3DORDER_LINEAR or D3DORDER_CUBIC (default)
		case D3DRS_NORMALORDER: p="NORMALORDER"; break; // NPatch normal interpolation order. D3DORDER_LINEAR (default) or D3DORDER_QUADRATIC

		// unsupported legacy

		case D3DRENDERSTATE_TEXTUREHANDLE: p="TEXTUREHANDLE"; break;
		case D3DRENDERSTATE_TEXTUREADDRESS: p="TEXTUREADDRESS"; break;
		case D3DRENDERSTATE_WRAPU: p="WRAPU"; break;
		case D3DRENDERSTATE_WRAPV: p="WRAPV"; break;
		case D3DRENDERSTATE_MONOENABLE: p="MONOENABLE"; break;
		case D3DRENDERSTATE_ROP2: p="ROP2"; break;
		case D3DRENDERSTATE_PLANEMASK: p="PLANEMASK"; break;
		case D3DRENDERSTATE_TEXTUREMAG: p="TEXTUREMAG"; break;
		case D3DRENDERSTATE_TEXTUREMIN: p="TEXTUREMIN"; break;
		case D3DRENDERSTATE_TEXTUREMAPBLEND: p="TEXTUREMAPBLEND"; break;
		case D3DRENDERSTATE_SUBPIXEL: p="SUBPIXEL"; break;
		case D3DRENDERSTATE_SUBPIXELX: p="SUBPIXELX"; break;
		case D3DRENDERSTATE_STIPPLEENABLE: p="STIPPLEENABLE"; break;
		case D3DRENDERSTATE_BORDERCOLOR: p="BORDERCOLOR"; break;
		case D3DRENDERSTATE_TEXTUREADDRESSU: p="TEXTUREADDRESSU"; break;
		case D3DRENDERSTATE_TEXTUREADDRESSV: p="TEXTUREADDRESSV"; break;
		case D3DRENDERSTATE_MIPMAPLODBIAS: p="MIPMAPLODBIAS"; break;
		case D3DRENDERSTATE_ANISOTROPY: p="ANISOTROPY"; break;
		case D3DRENDERSTATE_FLUSHBATCH: p="FLUSHBATCH"; break;
		//case D3DRENDERSTATE_FORCE_DWORD: p="FORCE_DWORD"; break;
		default: p="???"; break;
	}
	return p;
}

char *ExplainRenderstateValue(DWORD Value)
{
	char *p;
	switch(Value){
	case D3DCMP_NEVER               : p="D3DCMP_NEVER"; break;
	case D3DCMP_LESS                : p="D3DCMP_LESS"; break;
	case D3DCMP_EQUAL               : p="D3DCMP_EQUAL"; break;
	case D3DCMP_LESSEQUAL           : p="D3DCMP_LESSEQUAL"; break;
	case D3DCMP_GREATER             : p="D3DCMP_GREATER"; break;
	case D3DCMP_NOTEQUAL            : p="D3DCMP_NOTEQUAL"; break;
	case D3DCMP_GREATEREQUAL        : p="D3DCMP_GREATEREQUAL"; break;
	case D3DCMP_ALWAYS              : p="D3DCMP_ALWAYS"; break;
	default							: p="???"; break;
	}
	return p;
}

char *ExplainWfPFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"CWP_", eblen);
	if(c & CWP_SKIPDISABLED) strscat(eb, eblen, "SKIPDISABLED+");
	if(c & CWP_SKIPINVISIBLE) strscat(eb, eblen, "SKIPINVISIBLE+");
	if(c & CWP_SKIPTRANSPARENT) strscat(eb, eblen, "SKIPTRANSPARENT+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("CWP_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "CWP_ALL", eblen);
	}
	return(eb);
}

char *ExplainChangeDisplaySettingsFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"CDS_", eblen);
	if(c & CDS_UPDATEREGISTRY) strscat(eb, eblen, "UPDATEREGISTRY+");
	if(c & CDS_TEST) strscat(eb, eblen, "TEST+");
	if(c & CDS_FULLSCREEN) strscat(eb, eblen, "FULLSCREEN+");
	if(c & CDS_GLOBAL) strscat(eb, eblen, "GLOBAL+");
	if(c & CDS_SET_PRIMARY) strscat(eb, eblen, "SET_PRIMARY+");
	if(c & CDS_VIDEOPARAMETERS) strscat(eb, eblen, "VIDEOPARAMETERS+");
	if(c & CDS_ENABLE_UNSAFE_MODES) strscat(eb, eblen, "ENABLE_UNSAFE_MODES+");
	if(c & CDS_DISABLE_UNSAFE_MODES) strscat(eb, eblen, "DISABLE_UNSAFE_MODES+");
	if(c & CDS_RESET) strscat(eb, eblen, "RESET+");
	if(c & CDS_NORESET) strscat(eb, eblen, "NORESET+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("CDS_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}

char *ExplainDICooperativeFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DISCL_", eblen);
	if(c & DISCL_EXCLUSIVE) strscat(eb, eblen, "EXCLUSIVE+");
	if(c & DISCL_NONEXCLUSIVE) strscat(eb, eblen, "NONEXCLUSIVE+");
	if(c & DISCL_FOREGROUND) strscat(eb, eblen, "FOREGROUND+");
	if(c & DISCL_BACKGROUND) strscat(eb, eblen, "BACKGROUND+");
	if(c & DISCL_NOWINKEY) strscat(eb, eblen, "NOWINKEY+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DISCL_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}

char *ExplainRegionType(DWORD c)
{
	static char *sRetCodes[4]={"ERROR", "NULLREGION", "SIMPLEREGION", "COMPLEXREGION"};
	if(c<4) return sRetCodes[c];
	return "unknown";
}

char *ExplainZBufferBitDepths(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"DDBD_", eblen);
	if(c & DDBD_8) strscat(eb, eblen, "8+");
	if(c & DDBD_16) strscat(eb, eblen, "16+");
	if(c & DDBD_24) strscat(eb, eblen, "24+");
	if(c & DDBD_32) strscat(eb, eblen, "32+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DDBD_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}


#define _FACD3D  0x876
#define MAKE_D3DHRESULT( code )  MAKE_HRESULT( 1, _FACD3D, code )
#define MAKE_D3DSTATUS( code )  MAKE_HRESULT( 0, _FACD3D, code )

#define D3DERR_DRIVERINTERNALERROR              MAKE_D3DHRESULT(2087)
#define D3DERR_NOTFOUND                         MAKE_D3DHRESULT(2150)
#define D3DERR_MOREDATA                         MAKE_D3DHRESULT(2151)
#define D3DERR_DEVICELOST                       MAKE_D3DHRESULT(2152)
#define D3DERR_DEVICENOTRESET                   MAKE_D3DHRESULT(2153)
#define D3DERR_NOTAVAILABLE                     MAKE_D3DHRESULT(2154)
#define D3DERR_OUTOFVIDEOMEMORY                 MAKE_D3DHRESULT(380)
#define D3DERR_INVALIDDEVICE                    MAKE_D3DHRESULT(2155)
#define D3DERR_INVALIDCALL                      MAKE_D3DHRESULT(2156)
#define D3DERR_DRIVERINVALIDCALL                MAKE_D3DHRESULT(2157)
#define D3DERR_WASSTILLDRAWING                  MAKE_D3DHRESULT(540)
#define D3DOK_NOAUTOGEN                         MAKE_D3DSTATUS(2159)

#define D3DERR_DEVICEREMOVED                    MAKE_D3DHRESULT(2160)
#define S_NOT_RESIDENT                          MAKE_D3DSTATUS(2165)
#define S_RESIDENT_IN_SHARED_MEMORY             MAKE_D3DSTATUS(2166)
#define S_PRESENT_MODE_CHANGED                  MAKE_D3DSTATUS(2167)
#define S_PRESENT_OCCLUDED                      MAKE_D3DSTATUS(2168)
#define D3DERR_DEVICEHUNG                       MAKE_D3DHRESULT(2164)
#define D3DERR_UNSUPPORTEDOVERLAY               MAKE_D3DHRESULT(2171)
#define D3DERR_UNSUPPORTEDOVERLAYFORMAT         MAKE_D3DHRESULT(2172)
#define D3DERR_CANNOTPROTECTCONTENT             MAKE_D3DHRESULT(2173)
#define D3DERR_UNSUPPORTEDCRYPTO                MAKE_D3DHRESULT(2174)
#define D3DERR_PRESENT_STATISTICS_DISJOINT      MAKE_D3DHRESULT(2180)

char *ExplainDDError(DWORD c)
{
	char *eb;
	switch(c)
	{
		case DD_OK: 							eb="DD_OK"; break;
		case DIERR_OBJECTNOTFOUND:				eb="DIERR_OBJECTNOTFOUND"; break;
		case DDERR_ALREADYINITIALIZED:			eb="DDERR_ALREADYINITIALIZED"; break;
		case DDERR_BLTFASTCANTCLIP:				eb="DDERR_BLTFASTCANTCLIP"; break;
		case DDERR_CANNOTATTACHSURFACE: 		eb="DDERR_CANNOTATTACHSURFACE"; break;
		case DDERR_CANNOTDETACHSURFACE: 		eb="DDERR_CANNOTDETACHSURFACE"; break;
		case DDERR_CANTCREATEDC: 				eb="DDERR_CANTCREATEDC"; break;
		case DDERR_CANTDUPLICATE: 				eb="DDERR_CANTDUPLICATE"; break;
		case DDERR_CANTLOCKSURFACE: 			eb="DDERR_CANTLOCKSURFACE"; break;
		case DDERR_CANTPAGELOCK: 				eb="DDERR_CANTPAGELOCK"; break;
		case DDERR_CANTPAGEUNLOCK: 				eb="DDERR_CANTPAGEUNLOCK"; break;
		case DDERR_CLIPPERISUSINGHWND: 			eb="DDERR_CLIPPERISUSINGHWND"; break;
		case DDERR_COLORKEYNOTSET: 				eb="DDERR_COLORKEYNOTSET"; break;
		case DDERR_CURRENTLYNOTAVAIL: 			eb="DDERR_CURRENTLYNOTAVAIL"; break;
		case DDERR_DCALREADYCREATED: 			eb="DDERR_DCALREADYCREATED"; break;
		case DDERR_DEVICEDOESNTOWNSURFACE: 		eb="DDERR_DEVICEDOESNTOWNSURFACE"; break;
		case DDERR_DIRECTDRAWALREADYCREATED: 	eb="DDERR_DIRECTDRAWALREADYCREATED"; break;
		case DDERR_EXCEPTION: 					eb="DDERR_EXCEPTION"; break;
		case DDERR_EXCLUSIVEMODEALREADYSET: 	eb="DDERR_EXCLUSIVEMODEALREADYSET"; break;
		case DDERR_EXPIRED: 					eb="DDERR_EXPIRED"; break;
		case DDERR_GENERIC: 					eb="DDERR_GENERIC"; break;
		case DDERR_HEIGHTALIGN: 				eb="DDERR_HEIGHTALIGN"; break; 	
		case DDERR_HWNDALREADYSET: 	 			eb="DDERR_HWNDALREADYSET"; break;
		case DDERR_HWNDSUBCLASSED: 	 			eb="DDERR_HWNDSUBCLASSED"; break;
		case DDERR_IMPLICITLYCREATED: 			eb="DDERR_IMPLICITLYCREATED"; break;
		case DDERR_INCOMPATIBLEPRIMARY: 	 	eb="DDERR_INCOMPATIBLEPRIMARY"; break;
		case DDERR_INVALIDCAPS: 		 		eb="DDERR_INVALIDCAPS"; break;
		case DDERR_INVALIDCLIPLIST:	 			eb="DDERR_INVALIDCLIPLIST"; break;
		case DDERR_INVALIDDIRECTDRAWGUID:	 	eb="DDERR_INVALIDDIRECTDRAWGUID"; break;
		case DDERR_INVALIDMODE: 	 			eb="DDERR_INVALIDMODE"; break;
		case DDERR_INVALIDOBJECT: 	 			eb="DDERR_INVALIDOBJECT"; break;
		case DDERR_INVALIDPARAMS: 	 			eb="DDERR_INVALIDPARAMS"; break;
		case DDERR_INVALIDPIXELFORMAT:	 		eb="DDERR_INVALIDPIXELFORMAT"; break;
		case DDERR_INVALIDPOSITION: 		 	eb="DDERR_INVALIDPOSITION"; break;
		case DDERR_INVALIDRECT: 		 		eb="DDERR_INVALIDRECT"; break;
		case DDERR_INVALIDSTREAM:	 			eb="DDERR_INVALIDSTREAM"; break;
		case DDERR_INVALIDSURFACETYPE:	 		eb="DDERR_INVALIDSURFACETYPE"; break;
		case DDERR_LOCKEDSURFACES: 		 		eb="DDERR_LOCKEDSURFACES"; break;
		case DDERR_MOREDATA: 		 			eb="DDERR_MOREDATA"; break;
		case DDERR_NO3D: 		 				eb="DDERR_NO3D"; break;
		case DDERR_NOALPHAHW:	 				eb="DDERR_NOALPHAHW"; break;
		case DDERR_NOBLTHW: 	 				eb="DDERR_NOBLTHW"; break;
		case DDERR_NOCLIPLIST: 	 				eb="DDERR_NOCLIPLIST"; break;
		case DDERR_NOCLIPPERATTACHED: 	 		eb="DDERR_NOCLIPPERATTACHED"; break;
		case DDERR_NOCOLORCONVHW: 		 		eb="DDERR_NOCOLORCONVHW"; break;
		case DDERR_NOCOLORKEY: 		 			eb="DDERR_NOCOLORKEY"; break;
		case DDERR_NOCOLORKEYHW: 	 			eb="DDERR_NOCOLORKEYHW"; break;
		case DDERR_NOCOOPERATIVELEVELSET: 	 	eb="DDERR_NOCOOPERATIVELEVELSET"; break;
		case DDERR_NODC: 		 				eb="DDERR_NODC"; break;
		case DDERR_NODDROPSHW:	 				eb="DDERR_NODDROPSHW"; break;
		case DDERR_NODIRECTDRAWHW:	 			eb="DDERR_NODIRECTDRAWHW"; break;
		case DDERR_NODIRECTDRAWSUPPORT: 	 	eb="DDERR_NODIRECTDRAWSUPPORT"; break;
		case DDERR_NOEMULATION: 		 		eb="DDERR_NOEMULATION"; break;
		case DDERR_NOEXCLUSIVEMODE: 	 		eb="DDERR_NOEXCLUSIVEMODE"; break;
		case DDERR_NOFLIPHW: 		 			eb="DDERR_NOFLIPHW"; break;
		case DDERR_NOFOCUSWINDOW: 		 		eb="DDERR_NOFOCUSWINDOW"; break;
		case DDERR_NOGDI: 		 				eb="DDERR_NOGDI"; break;
		case DDERR_NOHWND: 		 				eb="DDERR_NOHWND"; break;
		case DDERR_NOMIPMAPHW: 	 				eb="DDERR_NOMIPMAPHW"; break;
		case DDERR_NOMIRRORHW: 	 				eb="DDERR_NOMIRRORHW"; break;
		case DDERR_NONONLOCALVIDMEM: 	 		eb="DDERR_NONONLOCALVIDMEM"; break;
		case DDERR_NOOPTIMIZEHW: 		 		eb="DDERR_NOOPTIMIZEHW"; break;
		case DDERR_NOOVERLAYDEST: 		 		eb="DDERR_NOOVERLAYDEST"; break;
		case DDERR_NOOVERLAYHW: 		 		eb="DDERR_NOOVERLAYHW"; break;
		case DDERR_NOPALETTEATTACHED:	 		eb="DDERR_NOPALETTEATTACHED"; break;
		case DDERR_NOPALETTEHW: 		 		eb="DDERR_NOPALETTEHW"; break;
		case DDERR_NORASTEROPHW: 	 			eb="DDERR_NORASTEROPHW"; break;
		case DDERR_NOROTATIONHW: 	 			eb="DDERR_NOROTATIONHW"; break;
		case DDERR_NOSTRETCHHW: 		 		eb="DDERR_NOSTRETCHHW"; break;
		case DDERR_NOT4BITCOLOR: 	 			eb="DDERR_NOT4BITCOLOR"; break;
		case DDERR_NOT4BITCOLORINDEX: 	 		eb="DDERR_NOT4BITCOLORINDEX"; break;
		case DDERR_NOT8BITCOLOR: 		 		eb="DDERR_NOT8BITCOLOR"; break;
		case DDERR_NOTAOVERLAYSURFACE: 	 		eb="DDERR_NOTAOVERLAYSURFACE"; break;
		case DDERR_NOTEXTUREHW: 	 			eb="DDERR_NOTEXTUREHW"; break;
		case DDERR_NOTFLIPPABLE: 	 			eb="DDERR_NOTFLIPPABLE"; break;
		case DDERR_NOTFOUND: 	 				eb="DDERR_NOTFOUND"; break;
		case DDERR_NOTINITIALIZED:	 			eb="DDERR_NOTINITIALIZED"; break;
		case DDERR_NOTLOADED: 		 			eb="DDERR_NOTLOADED"; break;
		case DDERR_NOTLOCKED: 		 			eb="DDERR_NOTLOCKED"; break;
		case DDERR_NOTPAGELOCKED: 		 		eb="DDERR_NOTPAGELOCKED"; break;
		case DDERR_NOTPALETTIZED: 		 		eb="DDERR_NOTPALETTIZED"; break;
		case DDERR_NOVSYNCHW: 		 			eb="DDERR_NOVSYNCHW"; break;
		case DDERR_NOZBUFFERHW: 		 		eb="DDERR_NOZBUFFERHW"; break;
		case DDERR_NOZOVERLAYHW: 	 			eb="DDERR_NOZOVERLAYHW"; break;
		case DDERR_OUTOFCAPS: 		 			eb="DDERR_OUTOFCAPS"; break;
		case DDERR_OUTOFMEMORY: 		 		eb="DDERR_OUTOFMEMORY"; break;
		case DDERR_OUTOFVIDEOMEMORY: 	 		eb="DDERR_OUTOFVIDEOMEMORY"; break;
		case DDERR_OVERLAPPINGRECTS: 	 		eb="DDERR_OVERLAPPINGRECTS"; break;
		case DDERR_OVERLAYCANTCLIP: 		 	eb="DDERR_OVERLAYCANTCLIP"; break;
		case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:eb="DDERR_OVERLAYCOLORKEYONLYONEACTIVE"; break;
		case DDERR_OVERLAYNOTVISIBLE: 		 	eb="DDERR_OVERLAYNOTVISIBLE"; break;
		case DDERR_PALETTEBUSY: 		 		eb="DDERR_PALETTEBUSY"; break;
		case DDERR_PRIMARYSURFACEALREADYEXISTS: eb="DDERR_PRIMARYSURFACEALREADYEXISTS"; break;
		case DDERR_REGIONTOOSMALL: 	 			eb="DDERR_REGIONTOOSMALL"; break;
		case DDERR_SURFACEALREADYATTACHED: 	 	eb="DDERR_SURFACEALREADYATTACHED"; break;
		case DDERR_SURFACEALREADYDEPENDENT: 	eb="DDERR_SURFACEALREADYDEPENDENT"; break;
		case DDERR_SURFACEBUSY: 		 		eb="DDERR_SURFACEBUSY"; break;
		case DDERR_SURFACEISOBSCURED: 	 		eb="DDERR_SURFACEISOBSCURED"; break;
		case DDERR_SURFACELOST: 		 		eb="DDERR_SURFACELOST"; break;
		case DDERR_SURFACENOTATTACHED: 		 	eb="DDERR_SURFACENOTATTACHED"; break;
		case DDERR_TOOBIGHEIGHT: 		 		eb="DDERR_TOOBIGHEIGHT"; break;
		case DDERR_TOOBIGSIZE: 		 			eb="DDERR_TOOBIGSIZE"; break;
		case DDERR_TOOBIGWIDTH: 		 		eb="DDERR_TOOBIGWIDTH"; break;
		case DDERR_UNSUPPORTED: 		 		eb="DDERR_UNSUPPORTED"; break;
		case DDERR_UNSUPPORTEDFORMAT: 	 		eb="DDERR_UNSUPPORTEDFORMAT"; break;
		case DDERR_UNSUPPORTEDMASK: 		 	eb="DDERR_UNSUPPORTEDMASK"; break;
		case DDERR_UNSUPPORTEDMODE: 		 	eb="DDERR_UNSUPPORTEDMODE"; break;
		case DDERR_VERTICALBLANKINPROGRESS: 	eb="DDERR_VERTICALBLANKINPROGRESS"; break;
		case DDERR_VIDEONOTACTIVE: 		 		eb="DDERR_VIDEONOTACTIVE"; break;
		case DDERR_WASSTILLDRAWING: 		 	eb="DDERR_WASSTILLDRAWING"; break;
		case DDERR_WRONGMODE: 		 			eb="DDERR_WRONGMODE"; break;
		case DDERR_XALIGN: 		 				eb="DDERR_XALIGN"; break;
		case DDERR_NOTONMIPMAPSUBLEVEL:			eb="DDERR_NOTONMIPMAPSUBLEVEL"; break;

		// D3D errors
		case D3DERR_WRONGTEXTUREFORMAT: 		eb="D3DERR_WRONGTEXTUREFORMAT"; break;
		case D3DERR_UNSUPPORTEDCOLOROPERATION: 	eb="D3DERR_UNSUPPORTEDCOLOROPERATION"; break;
		case D3DERR_UNSUPPORTEDCOLORARG: 		eb="D3DERR_UNSUPPORTEDCOLORARG"; break;
		case D3DERR_UNSUPPORTEDALPHAOPERATION: 	eb="D3DERR_UNSUPPORTEDALPHAOPERATION"; break;
		case D3DERR_UNSUPPORTEDALPHAARG: 		eb="D3DERR_UNSUPPORTEDALPHAARG"; break;
		case D3DERR_TOOMANYOPERATIONS: 		 	eb="D3DERR_TOOMANYOPERATIONS"; break;
		case D3DERR_CONFLICTINGTEXTUREFILTER: 	eb="D3DERR_CONFLICTINGTEXTUREFILTER"; break;
		case D3DERR_UNSUPPORTEDFACTORVALUE: 	eb="D3DERR_UNSUPPORTEDFACTORVALUE"; break;
		case D3DERR_CONFLICTINGRENDERSTATE: 	eb="D3DERR_CONFLICTINGRENDERSTATE"; break;
		case D3DERR_UNSUPPORTEDTEXTUREFILTER: 	eb="D3DERR_UNSUPPORTEDTEXTUREFILTER"; break;
		case D3DERR_CONFLICTINGTEXTUREPALETTE: 	eb="D3DERR_CONFLICTINGTEXTUREPALETTE"; break;
		case D3DERR_DRIVERINTERNALERROR: 		eb="D3DERR_DRIVERINTERNALERROR"; break;
		case D3DERR_NOTFOUND: 		 			eb="D3DERR_NOTFOUND"; break;
		case D3DERR_MOREDATA: 		 			eb="D3DERR_MOREDATA"; break;
		case D3DERR_DEVICELOST: 		 		eb="D3DERR_DEVICELOST"; break;
		case D3DERR_DEVICENOTRESET: 		 	eb="D3DERR_DEVICENOTRESET"; break;
		case D3DERR_NOTAVAILABLE: 		 		eb="D3DERR_NOTAVAILABLE"; break;
		//case D3DERR_OUTOFVIDEOMEMORY: 		 	eb="D3DERR_OUTOFVIDEOMEMORY"; break;
		case D3DERR_INVALIDDEVICE: 		 		eb="D3DERR_INVALIDDEVICE"; break;
		case D3DERR_INVALIDCALL: 		 		eb="D3DERR_INVALIDCALL"; break;
		case D3DERR_DRIVERINVALIDCALL: 		 	eb="D3DERR_DRIVERINVALIDCALL"; break;
		//case D3DERR_WASSTILLDRAWING: 		 	eb="D3DERR_WASSTILLDRAWING"; break;
		case D3DOK_NOAUTOGEN: 		 			eb="D3DOK_NOAUTOGEN"; break;
		case D3DERR_DEVICEREMOVED: 		 		eb="D3DERR_DEVICEREMOVED"; break;
		case D3DERR_DEVICEHUNG: 		 		eb="D3DERR_DEVICEHUNG"; break;
		case D3DERR_UNSUPPORTEDOVERLAY: 		eb="D3DERR_UNSUPPORTEDOVERLAY"; break;
		case D3DERR_UNSUPPORTEDOVERLAYFORMAT: 	eb="D3DERR_UNSUPPORTEDOVERLAYFORMAT"; break;
		case D3DERR_CANNOTPROTECTCONTENT: 		eb="D3DERR_CANNOTPROTECTCONTENT"; break;
		case D3DERR_UNSUPPORTEDCRYPTO: 		 	eb="D3DERR_UNSUPPORTEDCRYPTO"; break;
		case D3DERR_PRESENT_STATISTICS_DISJOINT:eb="D3DERR_PRESENT_STATISTICS_DISJOINT"; break;

		case D3DERR_INBEGIN:					eb="D3DERR_INBEGIN"; break;
		case D3DERR_NOTINBEGIN:					eb="D3DERR_NOTINBEGIN"; break;
		case D3DERR_NOVIEWPORTS:				eb="D3DERR_NOVIEWPORTS"; break;
		case D3DERR_VIEWPORTDATANOTSET:			eb="D3DERR_VIEWPORTDATANOTSET"; break;
		case D3DERR_VIEWPORTHASNODEVICE:		eb="D3DERR_VIEWPORTHASNODEVICE"; break;
		case D3DERR_NOCURRENTVIEWPORT:			eb="D3DERR_NOCURRENTVIEWPORT"; break;

		case D3DERR_INVALIDVERTEXFORMAT:        eb="D3DERR_INVALIDVERTEXFORMAT"; break;
		case D3DERR_COLORKEYATTACHED:           eb="D3DERR_COLORKEYATTACHED"; break;
		case D3DERR_VERTEXBUFFEROPTIMIZED:      eb="D3DERR_VERTEXBUFFEROPTIMIZED"; break;
		case D3DERR_VBUF_CREATE_FAILED:         eb="D3DERR_VBUF_CREATE_FAILED"; break;
		case D3DERR_VERTEXBUFFERLOCKED:         eb="D3DERR_VERTEXBUFFERLOCKED"; break;
		case D3DERR_VERTEXBUFFERUNLOCKFAILED:   eb="D3DERR_VERTEXBUFFERUNLOCKFAILED"; break;
		case D3DERR_ZBUFFER_NOTPRESENT:         eb="D3DERR_ZBUFFER_NOTPRESENT"; break;
		case D3DERR_STENCILBUFFER_NOTPRESENT:   eb="D3DERR_STENCILBUFFER_NOTPRESENT"; break;
		case D3DERR_TOOMANYPRIMITIVES:          eb="D3DERR_TOOMANYPRIMITIVES"; break;
		case D3DERR_INVALIDMATRIX:              eb="D3DERR_INVALIDMATRIX"; break;
		case D3DERR_TOOMANYVERTICES:            eb="D3DERR_TOOMANYVERTICES"; break;
		case D3DERR_INVALIDSTATEBLOCK:          eb="D3DERR_INVALIDSTATEBLOCK"; break;
		case D3DERR_INBEGINSTATEBLOCK:          eb="D3DERR_INBEGINSTATEBLOCK"; break;
		case D3DERR_NOTINBEGINSTATEBLOCK:       eb="D3DERR_NOTINBEGINSTATEBLOCK"; break;
		case D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY:   eb="D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY"; break;
		case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY:    eb="D3DERR_ZBUFF_NEEDS_VIDEOMEMORY"; break;

		// ddrawex error codes
		case DDERR_LOADFAILED:                  eb="DDERR_LOADFAILED"; break;
		case DDERR_BADVERSIONINFO:              eb="DDERR_BADVERSIONINFO"; break;
		case DDERR_BADPROCADDRESS:              eb="DDERR_BADPROCADDRESS"; break;
		case DDERR_LEGACYUSAGE:                 eb="DDERR_LEGACYUSAGE"; break;

		// DSOUND error codes
		case DSERR_ALLOCATED:                   eb="DSERR_ALLOCATED"; break;
		case DS_NO_VIRTUALIZATION:              eb="DS_NO_VIRTUALIZATION"; break;
		case DSERR_CONTROLUNAVAIL:              eb="DSERR_CONTROLUNAVAIL"; break;
		//case DSERR_INVALIDPARAM:                eb="DSERR_INVALIDPARAM"; break;
		case DSERR_INVALIDCALL:                 eb="DSERR_INVALIDCALL"; break;
		case DSERR_PRIOLEVELNEEDED:             eb="DSERR_PRIOLEVELNEEDED"; break;
		//case DSERR_OUTOFMEMORY:                 eb="DSERR_OUTOFMEMORY"; break;
		case DSERR_BADFORMAT:                   eb="DSERR_BADFORMAT"; break;
		case DSERR_NODRIVER:                    eb="DSERR_NODRIVER"; break;
		case DSERR_ALREADYINITIALIZED:          eb="DSERR_ALREADYINITIALIZED"; break;
		case DSERR_NOAGGREGATION:               eb="DSERR_NOAGGREGATION"; break;
		case DSERR_BUFFERLOST:                  eb="DSERR_BUFFERLOST"; break;
		case DSERR_OTHERAPPHASPRIO:             eb="DSERR_OTHERAPPHASPRIO"; break;
		case DSERR_UNINITIALIZED:               eb="DSERR_UNINITIALIZED"; break;
		case DSERR_NOINTERFACE:                 eb="DSERR_NOINTERFACE"; break;
		case DSERR_ACCESSDENIED:                eb="DSERR_ACCESSDENIED"; break;
		case DSERR_BUFFERTOOSMALL:              eb="DSERR_BUFFERTOOSMALL"; break;
		case DSERR_DS8_REQUIRED:                eb="DSERR_DS8_REQUIRED"; break;
		case DSERR_SENDLOOP:                    eb="DSERR_SENDLOOP"; break;
		case DSERR_BADSENDBUFFERGUID:           eb="DSERR_BADSENDBUFFERGUID"; break;
		case DSERR_OBJECTNOTFOUND:              eb="DSERR_OBJECTNOTFOUND"; break;
		case DSERR_FXUNAVAILABLE:               eb="DSERR_FXUNAVAILABLE"; break;

		case APOERR_ALREADY_INITIALIZED:		eb="APOERR_ALREADY_INITIALIZED"; break;
		case APOERR_NOT_INITIALIZED:			eb="APOERR_NOT_INITIALIZED"; break;
		case APOERR_FORMAT_NOT_SUPPORTED:		eb="APOERR_FORMAT_NOT_SUPPORTED"; break;
		case APOERR_INVALID_APO_CLSID:			eb="APOERR_INVALID_APO_CLSID"; break;
		case APOERR_BUFFERS_OVERLAP:			eb="APOERR_BUFFERS_OVERLAP"; break;
		case APOERR_ALREADY_UNLOCKED:			eb="APOERR_ALREADY_UNLOCKED"; break;
		case APOERR_NUM_CONNECTIONS_INVALID:	eb="APOERR_NUM_CONNECTIONS_INVALID"; break;
		case APOERR_INVALID_OUTPUT_MAXFRAMECOUNT:	eb="APOERR_INVALID_OUTPUT_MAXFRAMECOUNT"; break;
		case APOERR_INVALID_CONNECTION_FORMAT:	eb="APOERR_INVALID_CONNECTION_FORMAT"; break;
		case APOERR_APO_LOCKED:					eb="APOERR_APO_LOCKED"; break;
		case APOERR_INVALID_COEFFCOUNT:			eb="APOERR_INVALID_COEFFCOUNT"; break;
		case APOERR_INVALID_COEFFICIENT:		eb="APOERR_INVALID_COEFFICIENT"; break;
		case APOERR_INVALID_CURVE_PARAM:		eb="APOERR_INVALID_CURVE_PARAM"; break;

		// DINPUT errors
		case DIERR_INPUTLOST:					eb="DIERR_INPUTLOST"; break;
		//case DIERR_INVALIDPARAM:				eb="DIERR_INVALIDPARAM"; break;
		case DIERR_NOTACQUIRED:					eb="DIERR_NOTACQUIRED"; break;
		case DIERR_NOTINITIALIZED:				eb="DIERR_NOTINITIALIZED"; break;
		case DIERR_NOTBUFFERED:					eb="DIERR_NOTBUFFERED"; break;	
		case DIERR_OLDDIRECTINPUTVERSION:		eb="DIERR_OLDDIRECTINPUTVERSION"; break;
		case E_PENDING:							eb="E_PENDING"; break;

		/*
		// DXGI errors
		case DXGI_ERROR_ACCESS_DENIED:			eb="DXGI_ERROR_ACCESS_DENIED"; break;
		case DXGI_ERROR_ACCESS_LOST:			eb="DXGI_ERROR_ACCESS_LOST"; break;
		case DXGI_ERROR_ALREADY_EXISTS:			eb="DXGI_ERROR_ALREADY_EXISTS"; break;
		case DXGI_ERROR_CANNOT_PROTECT_CONTENT:	eb="DXGI_ERROR_CANNOT_PROTECT_CONTENT"; break;
		case DXGI_ERROR_DEVICE_HUNG:			eb="DXGI_ERROR_DEVICE_HUNG"; break;
		case DXGI_ERROR_DEVICE_REMOVED:			eb="DXGI_ERROR_DEVICE_REMOVED"; break;
		case DXGI_ERROR_DEVICE_RESET:			eb="DXGI_ERROR_DEVICE_RESET"; break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	eb="DXGI_ERROR_DRIVER_INTERNAL_ERROR"; break;
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:	eb="DXGI_ERROR_FRAME_STATISTICS_DISJOINT"; break;
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:	eb="DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE"; break;
		case DXGI_ERROR_INVALID_CALL:			eb="DXGI_ERROR_INVALID_CALL"; break;
		case DXGI_ERROR_MORE_DATA:				eb="DXGI_ERROR_MORE_DATA"; break;
		case DXGI_ERROR_NAME_ALREADY_EXISTS:	eb="DXGI_ERROR_NAME_ALREADY_EXISTS"; break;
		case DXGI_ERROR_NONEXCLUSIVE:			eb="DXGI_ERROR_NONEXCLUSIVE"; break;
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:	eb="DXGI_ERROR_NOT_CURRENTLY_AVAILABLE"; break;
		case DXGI_ERROR_NOT_FOUND:				eb="DXGI_ERROR_NOT_FOUND"; break;
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:	eb="DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED"; break;
		case DXGI_ERROR_REMOTE_OUTOFMEMORY:		eb="DXGI_ERROR_REMOTE_OUTOFMEMORY"; break;
		case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:	eb="DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE"; break;
		case DXGI_ERROR_SDK_COMPONENT_MISSING:	eb="DXGI_ERROR_SDK_COMPONENT_MISSING"; break;
		case DXGI_ERROR_SESSION_DISCONNECTED:	eb="DXGI_ERROR_SESSION_DISCONNECTED"; break;
		case DXGI_ERROR_UNSUPPORTED:			eb="DXGI_ERROR_UNSUPPORTED"; break;
		case DXGI_ERROR_WAIT_TIMEOUT:			eb="DXGI_ERROR_WAIT_TIMEOUT"; break;
		case DXGI_ERROR_WAS_STILL_DRAWING:		eb="DXGI_ERROR_WAS_STILL_DRAWING"; break;
		*/

		// weirdos ...
		case RPC_E_CHANGED_MODE:				eb="RPC_E_CHANGED_MODE"; break; // in "Rubik's Games"
		case D3DERR_SCENE_NOT_IN_SCENE:			eb="D3DERR_SCENE_NOT_IN_SCENE"; break; // in "Messiah"

		default:								eb="unknown"; break;
	}
	return eb;
}

typedef enum _myD3DFORMAT
{
    D3DFMT_UNKNOWN              =  0,
    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A2B10G10R10          = 31,
    D3DFMT_A8B8G8R8             = 32,
    D3DFMT_X8B8G8R8             = 33,
    D3DFMT_G16R16               = 34,
    D3DFMT_A2R10G10B10          = 35,
    D3DFMT_A16B16G16R16         = 36,
    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,
    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,
    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,
    D3DFMT_A2W10V10U10          = 67,
    D3DFMT_D16_LOCKABLE         = 70,
    D3DFMT_D32                  = 71,
    D3DFMT_D15S1                = 73,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D24X4S4              = 79,
    D3DFMT_D16                  = 80,
    D3DFMT_D32F_LOCKABLE        = 82,
    D3DFMT_D24FS8               = 83,
    D3DFMT_D32_LOCKABLE         = 84,
    D3DFMT_S8_LOCKABLE          = 85,
    D3DFMT_L16                  = 81,
    D3DFMT_VERTEXDATA           =100,
    D3DFMT_INDEX16              =101,
    D3DFMT_INDEX32              =102,
    D3DFMT_Q16W16V16U16         =110,
    D3DFMT_R16F                 = 111,
    D3DFMT_G16R16F              = 112,
    D3DFMT_A16B16G16R16F        = 113,
    D3DFMT_R32F                 = 114,
    D3DFMT_G32R32F              = 115,
    D3DFMT_A32B32G32R32F        = 116,
    D3DFMT_CxV8U8               = 117,
    D3DFMT_A1                   = 118,
    D3DFMT_A2B10G10R10_XR_BIAS  = 119,
    D3DFMT_BINARYBUFFER         = 199,
    D3DFMT_FORCE_DWORD          =0x7fffffff
} myD3DFORMAT;

/*
char *sCompression(DWORD c)
{
	char *t[]={"RGB", "RLE8", "RLE4", "BITFIELDS", "JPEG", "PNG" };
	if (c <= BI_PNG) return t[c];
	return "unknown";
}
*/

char *sFourCC(DWORD fcc)
{
	static char sRet[7];
	char *pRet;
	char c;
	int i;
	switch(fcc){
		case BI_RGB						: pRet = "RGB"; break;
		case BI_RLE8					: pRet = "RLE8"; break;
		case BI_RLE4					: pRet = "RLE4"; break;
		case BI_BITFIELDS               : pRet = "BITFIELDS"; break;
		case BI_JPEG					: pRet = "JPEG"; break;
		case BI_PNG						: pRet = "PNG"; break;
		//case D3DFMT_UNKNOWN              : pRet = "UNKNOWN"; break;
    	case D3DFMT_R8G8B8               : pRet = "R8G8B8"; break;
    	case D3DFMT_A8R8G8B8             : pRet = "A8R8G8B8"; break;
    	case D3DFMT_X8R8G8B8             : pRet = "X8R8G8B8"; break;
    	case D3DFMT_R5G6B5               : pRet = "R5G6B5"; break;
    	case D3DFMT_X1R5G5B5             : pRet = "X1R5G5B5"; break;
    	case D3DFMT_A1R5G5B5             : pRet = "A1R5G5B5"; break;
    	case D3DFMT_A4R4G4B4             : pRet = "A4R4G4B4"; break;
    	case D3DFMT_R3G3B2               : pRet = "R3G3B2"; break;
    	case D3DFMT_A8                   : pRet = "A8"; break;
    	case D3DFMT_A8R3G3B2             : pRet = "A8R3G3B2"; break;
    	case D3DFMT_X4R4G4B4             : pRet = "X4R4G4B4"; break;
    	case D3DFMT_A2B10G10R10          : pRet = "A2B10G10R10"; break;
    	case D3DFMT_A8B8G8R8             : pRet = "A8B8G8R8"; break;
    	case D3DFMT_X8B8G8R8             : pRet = "X8B8G8R8"; break;
    	case D3DFMT_G16R16               : pRet = "G16R16"; break;
    	case D3DFMT_A2R10G10B10          : pRet = "A2R10G10B10"; break;
    	case D3DFMT_A16B16G16R16         : pRet = "A16B16G16R16"; break;
    	case D3DFMT_A8P8                 : pRet = "A8P8"; break;
    	case D3DFMT_P8                   : pRet = "P8"; break;
    	case D3DFMT_L8                   : pRet = "L8"; break;
    	case D3DFMT_A8L8                 : pRet = "A8L8"; break;
    	case D3DFMT_A4L4                 : pRet = "A4L4"; break;
    	case D3DFMT_V8U8                 : pRet = "V8U8"; break;
    	case D3DFMT_L6V5U5               : pRet = "L6V5U5"; break;
    	case D3DFMT_X8L8V8U8             : pRet = "X8L8V8U8"; break;
    	case D3DFMT_Q8W8V8U8             : pRet = "Q8W8V8U8"; break;
    	case D3DFMT_V16U16               : pRet = "V16U16"; break;
    	case D3DFMT_A2W10V10U10          : pRet = "A2W10V10U10"; break;
    	case D3DFMT_D16_LOCKABLE         : pRet = "D16_LOCKABLE"; break;
    	case D3DFMT_D32                  : pRet = "D32"; break;
    	case D3DFMT_D15S1                : pRet = "D15S1"; break;
    	case D3DFMT_D24S8                : pRet = "D24S8"; break;
    	case D3DFMT_D24X8                : pRet = "D24X8"; break;
    	case D3DFMT_D24X4S4              : pRet = "D24X4S4"; break;
    	case D3DFMT_D16                  : pRet = "D16"; break;
    	case D3DFMT_D32F_LOCKABLE        : pRet = "D32F_LOCKABLE"; break;
    	case D3DFMT_D24FS8               : pRet = "D24FS8"; break;
    	case D3DFMT_D32_LOCKABLE         : pRet = "D32_LOCKABLE"; break;
    	case D3DFMT_S8_LOCKABLE          : pRet = "S8_LOCKABLE"; break;
    	case D3DFMT_L16                  : pRet = "L16"; break;
    	case D3DFMT_VERTEXDATA           : pRet = "VERTEXDATA"; break;
    	case D3DFMT_INDEX16              : pRet = "INDEX16"; break;
    	case D3DFMT_INDEX32              : pRet = "INDEX32"; break;
    	case D3DFMT_Q16W16V16U16         : pRet = "Q16W16V16U16"; break;
    	case D3DFMT_R16F                 : pRet = "R16F"; break;
    	case D3DFMT_G16R16F              : pRet = "G16R16F"; break;
    	case D3DFMT_A16B16G16R16F        : pRet = "A16B16G16R16F"; break;
    	case D3DFMT_R32F                 : pRet = "R32F"; break;
    	case D3DFMT_G32R32F              : pRet = "G32R32F"; break;
    	case D3DFMT_A32B32G32R32F        : pRet = "A32B32G32R32F"; break;
    	case D3DFMT_CxV8U8               : pRet = "CxV8U8"; break;
    	case D3DFMT_A1                   : pRet = "A1"; break;
    	case D3DFMT_A2B10G10R10_XR_BIAS  : pRet = "A2B10G10R10_XR_BIAS"; break;
    	case D3DFMT_BINARYBUFFER         : pRet = "BINARYBUFFER"; break;
		default:
			char *t=&sRet[0];
			*(t++)='\"';
			for(i=0; i<4; i++){
				c = fcc & (0xFF);
				*(t++) = isprint(c) ? c : '.';
				fcc = fcc >> 8;
			}
			*(t++)='\"';
			*t = 0;
			pRet = sRet;
			break;
	}
	return pRet;
}

char *ExplainPixelFormat(LPDDPIXELFORMAT ddpfPixelFormat, char *sBuf, int len)
{
	char sItem[256];
	char sPFormat[256];
	DWORD flags=ddpfPixelFormat->dwFlags;
	sprintf_s(sBuf, len, " PixelFormat flags=%#x(%s) BPP=%d", 
		flags, ExplainPixelFormatFlags(flags, sPFormat, 256), ddpfPixelFormat->dwRGBBitCount);
	if (flags & DDPF_RGB) {
		if (flags & DDPF_ALPHAPIXELS) {
			sprintf_s(sItem, 256, " RGBA=(%#x,%#x,%#x,%#x)", 
				ddpfPixelFormat->dwRBitMask,
				ddpfPixelFormat->dwGBitMask,
				ddpfPixelFormat->dwBBitMask,
				ddpfPixelFormat->dwRGBAlphaBitMask);
		}
		else {
			sprintf_s(sItem, 256, " RGB=(%#x,%#x,%#x)", 
				ddpfPixelFormat->dwRBitMask,
				ddpfPixelFormat->dwGBitMask,
				ddpfPixelFormat->dwBBitMask);
		}
		strscat(sBuf, len, sItem);
	}
	if (flags & DDPF_YUV) {
		sprintf_s(sItem, 256, " YUVA=(%#x,%#x,%#x,%#x)", 
			ddpfPixelFormat->dwYBitMask,
			ddpfPixelFormat->dwUBitMask,
			ddpfPixelFormat->dwVBitMask,
			ddpfPixelFormat->dwYUVAlphaBitMask);
		strscat(sBuf, len, sItem);
	}
	if (flags & DDPF_ZBUFFER) {
		sprintf_s(sItem, 256, " SdZSbL=(%#x,%#x,%#x,%#x)", 
			ddpfPixelFormat->dwStencilBitDepth,
			ddpfPixelFormat->dwZBitMask,
			ddpfPixelFormat->dwStencilBitMask,
			ddpfPixelFormat->dwLuminanceAlphaBitMask);
		strscat(sBuf, len, sItem);
	}
	if (flags & DDPF_ALPHA) {
		sprintf_s(sItem, 256, " LBdBlZ=(%#x,%#x,%#x,%#x)", 
			ddpfPixelFormat->dwLuminanceBitMask,
			ddpfPixelFormat->dwBumpDvBitMask,
			ddpfPixelFormat->dwBumpLuminanceBitMask,
			ddpfPixelFormat->dwRGBZBitMask);
		strscat(sBuf, len, sItem);
	}
	if (flags & DDPF_LUMINANCE) {
		sprintf_s(sItem, 256, " BMbMF=(%#x,%#x,%#x,%#x)", 
			ddpfPixelFormat->dwBumpDuBitMask,
			ddpfPixelFormat->MultiSampleCaps.wBltMSTypes,
			ddpfPixelFormat->MultiSampleCaps.wFlipMSTypes,
			ddpfPixelFormat->dwYUVZBitMask);
		strscat(sBuf, len, sItem);
	}
	if (flags & DDPF_BUMPDUDV) {
		sprintf_s(sItem, 256, " O=(%#x)", 
			ddpfPixelFormat->dwOperations);
		strscat(sBuf, len, sItem);
	}
	if (flags & DDPF_FOURCC) {
		sprintf_s(sItem, 256, " FourCC=%#x(%s)", 
			ddpfPixelFormat->dwFourCC, sFourCC(ddpfPixelFormat->dwFourCC));
		strscat(sBuf, len, sItem);
	}
	return sBuf;
} 

char *GetObjectTypeStr(HDC hdc)
{
	char *s;
	static char sBuf[21];
	DWORD ot = (*pGetObjectType)(hdc);
	switch (ot){
		case OBJ_PEN:			s="PEN"; break;
		case OBJ_BRUSH:			s="BRUSH"; break;
		case OBJ_DC:			s="DC"; break;
		case OBJ_METADC:		s="METADC"; break;
		case OBJ_PAL:			s="PAL"; break;
		case OBJ_FONT:			s="FONT"; break;
		case OBJ_BITMAP:		s="BITMAP"; break;
		case OBJ_REGION:		s="REGION"; break;
		case OBJ_METAFILE:		s="METAFILE"; break;
		case OBJ_MEMDC:			s="MEMDC"; break;
		case OBJ_EXTPEN:		s="EXTPEN"; break;
		case OBJ_ENHMETADC:		s="ENHMETADC"; break;
		case OBJ_ENHMETAFILE:	s="ENHMETAFILE"; break;
		case OBJ_COLORSPACE:	s="COLORSPACE"; break;
		default:				
			//s="unknown"; break;
			sprintf(sBuf, "%#X", ot);
			s = sBuf;
			break;
	}
	return s;
}

char *ExplainRgnType(int type)
{
	char *s;
	switch(type){
		case NULLREGION: s="NULLREGION"; break;
		case SIMPLEREGION: s="SIMPLEREGION"; break;
		case COMPLEXREGION: s="COMPLEXREGION"; break;
		case ERROR: s="ERROR"; break;
		default: s="unknown"; break;
	}
	return s;
}

char *ExplainWSBFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strcpy(eb,"WSB_PROP");
	if (c & WSB_PROP_CYVSCROLL) strscat(eb, eblen, "CYVSCROLL+");
	if (c & WSB_PROP_CXHSCROLL) strscat(eb, eblen, "CXHSCROLL+");
	if (c & WSB_PROP_CYHSCROLL) strscat(eb, eblen, "CYHSCROLL+");
	if (c & WSB_PROP_CXVSCROLL) strscat(eb, eblen, "CXVSCROLL+");
	if (c & WSB_PROP_CXHTHUMB) strscat(eb, eblen, "CXHTHUMB+");
	if (c & WSB_PROP_CYVTHUMB) strscat(eb, eblen, "CYVTHUMB+");
	if (c & WSB_PROP_VSTYLE) strscat(eb, eblen, "VSTYLE+");
	if (c & WSB_PROP_VBKGCOLOR) strscat(eb, eblen, "VBKGCOLOR+");
	if (c & WSB_PROP_HBKGCOLOR) strscat(eb, eblen, "HBKGCOLOR+");
	if (c & WSB_PROP_HSTYLE) strscat(eb, eblen, "HSTYLE+");
	if (c & WSB_PROP_WINSTYLE) strscat(eb, eblen, "WINSTYLE+");
	if (c & WSB_PROP_PALETTE) strscat(eb, eblen, "PALETTE+");
	if (c & PFD_SWAP_LAYER_BUFFERS) strscat(eb, eblen, "SWAP_LAYER_BUFFERS+");
	if (c & PFD_GENERIC_ACCELERATED) strscat(eb, eblen, "GENERIC_ACCELERATED+");
	if (c & PFD_SUPPORT_DIRECTDRAW) strscat(eb, eblen, "SUPPORT_DIRECTDRAW+");
	if (c & PFD_DIRECT3D_ACCELERATED) strscat(eb, eblen, "DIRECT3D_ACCELERATED+");
	if (c & PFD_SUPPORT_COMPOSITION) strscat(eb, eblen, "SUPPORT_COMPOSITION+");
	if (c & PFD_DEPTH_DONTCARE) strscat(eb, eblen, "DEPTH_DONTCARE+");
	if (c & PFD_DOUBLEBUFFER_DONTCARE) strscat(eb, eblen, "DOUBLEBUFFER_DONTCARE+");
	if (c & PFD_STEREO_DONTCARE) strscat(eb, eblen, "STEREO_DONTCARE+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("WSB_PROP")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

#endif
