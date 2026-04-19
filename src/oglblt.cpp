#define _CRT_SECURE_NO_WARNINGS
#define INITGUID
//#define FULLHEXDUMP
#define USETEXTURERECTANGLE
//#define OPENGLTEX2DRECOVERY
#define OGLROTATION
#define _MODULE "dxwnd"

#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "ddrawi.h"
#include "dxwcore.hpp"
#include "stdio.h" 
#include "hddraw.h"
#include "dxhelper.h"
#include "syslibs.h"
#include <wingdi.h>
#include <gl.h>
#include <glext.h>

extern GetDC_Type pGetDCMethod();
extern ReleaseDC_Type pReleaseDCMethod();
extern Lock_Type pLockMethod(int);
extern Unlock4_Type pUnlockMethod(int);
extern SetClipper_Type pSetClipperMethod(int);
extern LPVOID dxwConvertFourCC(LPDDSURFACEDESC2, int);
extern DWORD PaletteEntries[];

typedef HGLRC(WINAPI *wglCreateContext_Type)(HDC);
typedef HGLRC(WINAPI *wglGetCurrentContext_Type)(void);
typedef BOOL (WINAPI *wglDeleteContext_Type)(HGLRC);
typedef BOOL (WINAPI *wglMakeCurrent_Type)(HDC, HGLRC);
typedef void (WINAPI *glViewport_Type)(GLint, GLint, GLsizei, GLsizei);
typedef void (WINAPI *glGenTextures_Type)(GLsizei, GLuint *);
typedef void (WINAPI *glBindTexture_Type)(GLenum, GLuint);
typedef void (WINAPI *glTexImage2D_Type)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
typedef void (WINAPI *glBegin_Type)(GLenum);
typedef void (WINAPI *glEnd_Type)(void);
typedef void (WINAPI *glTexCoord2i_Type)(GLint, GLint);
typedef void (WINAPI *glTexParameteri_Type)(GLenum, GLenum, GLint);
typedef void (WINAPI *glEnum_Type)(GLenum);
typedef void (WINAPI *glMatrixMode_Type)(GLenum);
typedef void (WINAPI *glLoadIdentity_Type)(void);
typedef void (WINAPI *glClearColor_Type)(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (WINAPI *glClear_Type)(GLbitfield);
typedef void (WINAPI *glColor3f_Type)(GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *glColor4f_Type)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *glTexCoord2f_Type)(GLfloat, GLfloat);
typedef void (WINAPI *glVertex2f_Type)(GLfloat, GLfloat);
typedef void (WINAPI *glFinish_Type)(void);
typedef void (WINAPI *glBlendFunc_Type)(GLenum, GLenum);
typedef GLint (WINAPI *glGetError_Type)(void);

struct wgltype {
	wglCreateContext_Type pwglCreateContext;
	wglGetCurrentContext_Type pwglGetCurrentContext;
	wglDeleteContext_Type pwglDeleteContext;
	wglMakeCurrent_Type pwglMakeCurrent;
} wgl;

struct ogltype {
	glViewport_Type pglViewport;
	glGenTextures_Type pglGenTextures;
	glBindTexture_Type pglBindTexture;
	glTexImage2D_Type pglTexImage2D;
	glBegin_Type pglBegin;
	glEnd_Type pglEnd;
	glTexCoord2i_Type pglTexCoord2i;
	glTexParameteri_Type pglTexParameteri;
	glEnum_Type pglEnum;
	glMatrixMode_Type pglMatrixMode;
	glLoadIdentity_Type pglLoadIdentity;
	glClearColor_Type pglClearColor;
	glClear_Type pglClear;
	glColor3f_Type pglColor3f;
	glColor4f_Type pglColor4f;
	glTexCoord2f_Type pglTexCoord2f;
	glVertex2f_Type pglVertex2f;
	glFinish_Type pglFinish;
	glBlendFunc_Type pglBlendFunc;
	glGetError_Type pglGetError;
	glEnum_Type pglEnable;
	glEnum_Type pglDisable;
	glEnum_Type pglShadeModel;
} ogl;

typedef enum {
	OGL_ROTATION_0 = 0,
	OGL_ROTATION_90,
	OGL_ROTATION_180,
	OGL_ROTATION_270};

//#define OGLROTATION
#ifdef OGLROTATION
static int oglRotation = OGL_ROTATION_180;
#endif // OGLROTATION

#ifdef USETEXTURERECTANGLE
static int uppower2(int size) 
{
	return ((size >> 2) << 2); 
}
#else
static int uppower2(int size)
{
	int shift;
	for(shift=0; size; shift++) size >>= 1;
	return 1 << shift;
}
#endif

static DWORD *oglPalette16BPP = NULL;

static void oglSetPalette16BPP()
{
	unsigned int pi;
	OutTraceOGL("oglSetPalette16BPP\n");
	oglPalette16BPP = (DWORD *)malloc(0x10000 * sizeof(DWORD));
	if (dxw.dwFlags3 & BLACKWHITE){
		// actually, it should be like this: R/G/B = (red * 0.30) + (green * 0.59) + (blue * 0.11) 
		// (http://www.codeproject.com/Articles/66253/Converting-Colors-to-Gray-Shades)
		DWORD grey;
		if (dxw.dwFlags1 & USERGB565){
			for (pi=0; pi<0x10000; pi++) {
				grey = (((((pi & 0x1F)<<3) * 30) + (((pi & 0x7E0)>>3) * 59) + (((pi & 0xF800)>>8) * 11)) / 100) & 0xFF;
				oglPalette16BPP[pi] = (grey<<16) + (grey<<8) + (grey);				
			}
		}
		else {
			for (pi=0; pi<0x10000; pi++) {
				grey = (((((pi & 0x1F)<<3) * 30) + (((pi & 0x3E0)>>2) * 59) + (((pi & 0x7C00)>>7) * 11)) / 100) & 0xFF;
				oglPalette16BPP[pi] = (grey<<16) + (grey<<8) + (grey);
			}
		}
	}
	else {
		if (dxw.dwFlags1 & USERGB565){
			for (pi=0; pi<0x10000; pi++) {
				oglPalette16BPP[pi]=(pi & 0x1F)<<19 | (pi & 0x7E0)<<5 | (pi & 0xF800)>>8; // RGB565
			}
		}
		else {
			for (pi=0; pi<0x10000; pi++) {
				oglPalette16BPP[pi]=(pi & 0x1F)<<19 | (pi & 0x3E0)<<6 | (pi & 0x7C00)>>7; // RGB555
			}
		}
	}
}

static void oglTransform8Pal(LPVOID pixels, LPVOID texbuf, UINT w, UINT h, UINT tpitch, UINT lPitch)
{
	BYTE *p8 = (LPBYTE)pixels;
	DWORD *t32 = (LPDWORD)texbuf;
	// build a reversed palette array
	DWORD quads[256];
	for(UINT c = 0; c < 256; c++) {
		DWORD pixel = PaletteEntries[c];
		quads[c] = ((pixel & 0xFF) << 16) | (pixel & 0xFF00) | ((pixel & 0xFF0000) >> 16);
	}
	// convert 8bpp pixels to 32bpp using palette
	for(UINT y = 0; y < h; y++){
		for(UINT x = 0; x < w; x++){
			t32[y*tpitch + x] = quads[*(p8 ++)];
		}
		// v2.05.18: added pitch compensation
		p8 += (lPitch - w);
	}
}

static void oglDeinterlace8Pal(LPVOID pixels, LPVOID texbuf, UINT w, UINT h, UINT tpitch, UINT lPitch)
{
	BYTE *p8 = (LPBYTE)pixels;
	DWORD *t32 = (LPDWORD)texbuf;
	UINT x, y;
	// build a reversed palette array
	DWORD quads[256];
	for(UINT c = 0; c < 256; c++) {
		DWORD pixel = PaletteEntries[c];
		quads[c] = ((pixel & 0xFF) << 16) | (pixel & 0xFF00) | ((pixel & 0xFF0000) >> 16);
	}
	int allBlack = TRUE;
	while(true){ // fake loop
		// try black odd lines
		y = ((h >> 2) << 1) + 1;
		for(x = 0; x < w; x ++){
			if(*((p8 ++) + (y * lPitch))) {
				allBlack = FALSE; 
				break;
			}
		}
		p8 = (LPBYTE)pixels;
		if(allBlack){
			for(UINT y = 0; y < h; y+=2){
				for(UINT x = 0; x < w; x++){
					DWORD p32 = quads[*(p8 ++)];
					t32[y*tpitch + x] = p32;
					t32[(y+1)*tpitch + x] = p32;
				}
			// v2.05.18: added pitch compensation
			p8 += ((2 * lPitch) - w);
			}
			break;
		}
		// try black even lines (untested)
		y = ((h >> 2) << 1);
		for(x = 0; x < w; x ++){
			if(*((p8 ++) + (y * lPitch))) {
				allBlack = FALSE; 
				break;
			}
		}
		p8 = (LPBYTE)pixels;
		if(allBlack){
			for(UINT y = 1; y < h; y+=2){
				for(UINT x = 0; x < w; x++){
					DWORD p32 = quads[*(p8 ++)];
					t32[y*tpitch + x] = p32;
					t32[(y-1)*tpitch + x] = p32;
				}
			// v2.05.18: added pitch compensation
			p8 += ((2 * lPitch) - w);
			}
			break;
		}
		// no black lines
		p8 = (LPBYTE)pixels;
		for(UINT y = 0; y < h; y++){
			for(UINT x = 0; x < w; x++){
				t32[y*tpitch + x] = quads[*(p8 ++)];
			}
			// v2.05.18: added pitch compensation
			p8 += (lPitch - w);
		}
		break; // exit the fake loop
	}
}

static void oglInterlace8Pal(LPVOID pixels, LPVOID texbuf, UINT w, UINT h, UINT tpitch, UINT lPitch)
{
	BYTE *p8 = (LPBYTE)pixels;
	DWORD *t32 = (LPDWORD)texbuf;
	// build a reversed palette array
	DWORD quads[256];
	for(UINT c = 0; c < 256; c++) {
		DWORD pixel = PaletteEntries[c];
		quads[c] = ((pixel & 0xFF) << 16) | (pixel & 0xFF00) | ((pixel & 0xFF0000) >> 16);
	}
	// convert 8bpp pixels to 32bpp using palette
	for(UINT y = 0; y < h; y+=2){
		for(UINT x = 0; x < w; x++){
			t32[y*tpitch + x] = quads[*(p8 ++)];
			if ((y+1) < h) t32[(y+1)*tpitch + x] = 0;
		}
		// v2.05.18: added pitch compensation
		p8 += ((2*lPitch) - w);
	}
}

static void oglTransform16(LPVOID pixels, LPVOID texbuf, UINT w, UINT h, UINT tpitch, UINT lPitch)
{
	WORD *p16 = (LPWORD)pixels;
	DWORD *t32 = (LPDWORD)texbuf;

	// build a reversed fale-16bit palette array
	if (!oglPalette16BPP) oglSetPalette16BPP();
	// convert 8bpp pixels to 32bpp using palette
	for(UINT y = 0; y < h; y++){
		for(UINT x = 0; x < w; x++){
			t32[y*tpitch + x] = oglPalette16BPP[*(p16 ++)];
		}
		// v2.05.18: added pitch compensation
		p16 += ((lPitch >> 1) - w);
	}
}

static void oglTransform24(LPVOID pixels, LPVOID texbuf, UINT w, UINT h, UINT tpitch, UINT lPitch)
{
	LPBYTE p8 = (LPBYTE)pixels;
	LPBYTE t8 = (LPBYTE)texbuf;

	// v2.05.25: new color depth
	for(UINT y = 0; y < h; y++){
		t8 = (LPBYTE)texbuf + (y*tpitch*sizeof(DWORD)); // tpitch is in DWORDs !!
		p8 = (LPBYTE)pixels + (y*lPitch);
		for(UINT x = 0; x < w; x++){
			*(t8)     = *(p8 + 2);
			*(t8 + 1) = *(p8 + 1);
			*(t8 + 2) = *(p8);
			*(t8 + 3) = 0;
			t8 += 4;
			p8 += 3;
		}
	}
}

static void oglTransform32(LPVOID pixels, LPVOID texbuf, UINT w, UINT h, UINT tpitch, UINT lPitch)
{
	LPBYTE p8 = (LPBYTE)pixels;
	LPBYTE t8 = (LPBYTE)texbuf;

	// just move pixels
	// v2.05.24: fix - RGBA must be byte swapped to BGRA !!
	for(UINT y = 0; y < h; y++){
		t8 = (LPBYTE)texbuf + (y*tpitch*sizeof(DWORD)); // tpitch is in DWORDs !!
		p8 = (LPBYTE)pixels + (y*lPitch);
		for(UINT x = 0; x < w; x++){
			*(t8)     = *(p8 + 2);
			*(t8 + 1) = *(p8 + 1);
			*(t8 + 2) = *(p8);
			*(t8 + 3) = 0;
			t8 += 4;
			p8 += 4;
		}
	}
}

static BOOL RebuildContext(char *api, wgltype *wgl)
{
	HGLRC oglContext = NULL;
	HWND hwnd = dxw.GethWnd();
	HDC gWindowDC = (*pGDIGetDC)(hwnd);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0, 0, 0, // RGBA sizes & shifts
		0, // accum buffer
		0, 0, 0, 0, // accum bits
		(BYTE)dxw.ActualPixelFormat.dwRGBBitCount, //32,
		0, 0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0};

	int kount = 0;
	do {
		int iPixelFormat;
		kount++;
		iPixelFormat = ChoosePixelFormat(gWindowDC, &pfd);
		OutTraceOGL("%s: pixel format=%d retry=%d\n", api, iPixelFormat, kount);
		if(!SetPixelFormat(gWindowDC, iPixelFormat, &pfd)){
			OutTraceOGL("%s: SetPixelFormat failed err=%d at=%d\n", api, GetLastError(), __LINE__);
		}
		if(oglContext) {
			OutTraceOGL("%s: wglDeleteContext oglctx=%#x @%d\n", api, oglContext, __LINE__);
			(*wgl->pwglMakeCurrent)(gWindowDC, NULL);
			(*wgl->pwglDeleteContext)(oglContext);
		}
		oglContext=(*wgl->pwglCreateContext)(gWindowDC);
		if(!oglContext && (kount > 5)){
			OutTraceOGL("%s: ERROR wglCreateContext failed err=%d at=%d\n", api, GetLastError(), __LINE__);
			(*pGDIReleaseDC)(hwnd, gWindowDC);
			return FALSE;
		}
		if(oglContext != NULL) break;
		Sleep(20);
	} while(TRUE);

	(*wgl->pwglMakeCurrent)(gWindowDC, oglContext);
	(*pGDIReleaseDC)(hwnd, gWindowDC);
	OutTraceOGL("%s: opengl context initialized hdc=%#x context=%#x\n", api, gWindowDC, oglContext);
	return TRUE;
}

static BOOL oglInitialize(char *api, wgltype *wgl, ogltype *ogl)
{
	HMODULE hOGL;

	// load SDL function pointers
	hOGL = (*pLoadLibraryA)("opengl32.dll");
	if(!hOGL) {
		OutTraceE("%s: load opengl32.dll FAILED\n", api);
		return DD_FALSE;
	}
	wgl->pwglCreateContext = (wglCreateContext_Type)(*pGetProcAddress)(hOGL, "wglCreateContext");
	wgl->pwglGetCurrentContext = (wglGetCurrentContext_Type)(*pGetProcAddress)(hOGL, "wglGetCurrentContext");
	wgl->pwglDeleteContext = (wglDeleteContext_Type)(*pGetProcAddress)(hOGL, "wglDeleteContext");
	wgl->pwglMakeCurrent = (wglMakeCurrent_Type)(*pGetProcAddress)(hOGL, "wglMakeCurrent");
	ogl->pglViewport = (glViewport_Type)(*pGetProcAddress)(hOGL, "glViewport");
	ogl->pglGenTextures = (glGenTextures_Type)(*pGetProcAddress)(hOGL, "glGenTextures");
	ogl->pglBindTexture = (glBindTexture_Type)(*pGetProcAddress)(hOGL, "glBindTexture");
	ogl->pglTexImage2D = (glTexImage2D_Type)(*pGetProcAddress)(hOGL, "glTexImage2D");
	ogl->pglBegin = (glBegin_Type)(*pGetProcAddress)(hOGL, "glBegin");
	ogl->pglEnd = (glEnd_Type)(*pGetProcAddress)(hOGL, "glEnd");
	ogl->pglTexCoord2i = (glTexCoord2i_Type)(*pGetProcAddress)(hOGL, "glTexCoord2i");
	ogl->pglTexParameteri = (glTexParameteri_Type)(*pGetProcAddress)(hOGL, "glTexParameteri");
	ogl->pglEnable = (glEnum_Type)(*pGetProcAddress)(hOGL, "glEnable");
	ogl->pglDisable = (glEnum_Type)(*pGetProcAddress)(hOGL, "glDisable");
	ogl->pglShadeModel = (glEnum_Type)(*pGetProcAddress)(hOGL, "glShadeModel");
	ogl->pglMatrixMode = (glMatrixMode_Type)(*pGetProcAddress)(hOGL, "glMatrixMode");
	ogl->pglLoadIdentity = (glLoadIdentity_Type)(*pGetProcAddress)(hOGL, "glLoadIdentity");
	ogl->pglClearColor = (glClearColor_Type)(*pGetProcAddress)(hOGL, "glClearColor");
	ogl->pglClear = (glClear_Type)(*pGetProcAddress)(hOGL, "glClear");
	ogl->pglColor3f = (glColor3f_Type)(*pGetProcAddress)(hOGL, "glColor3f");
	ogl->pglTexCoord2f = (glTexCoord2f_Type)(*pGetProcAddress)(hOGL, "glTexCoord2f");
	ogl->pglVertex2f = (glVertex2f_Type)(*pGetProcAddress)(hOGL, "glVertex2f");
	ogl->pglFinish = (glFinish_Type)(*pGetProcAddress)(hOGL, "glFinish");
	ogl->pglColor4f = (glColor4f_Type)(*pGetProcAddress)(hOGL, "glColor4f");
	ogl->pglBlendFunc = (glBlendFunc_Type)(*pGetProcAddress)(hOGL, "glBlendFunc");
	ogl->pglGetError = (glGetError_Type)(*pGetProcAddress)(hOGL, "glGetError");

	if(!(wgl->pwglCreateContext && wgl->pwglDeleteContext && wgl->pwglMakeCurrent && wgl->pwglGetCurrentContext && 
		ogl->pglViewport && ogl->pglGenTextures && ogl->pglBindTexture &&
		ogl->pglTexImage2D && ogl->pglBegin && ogl->pglEnd && ogl->pglTexCoord2i && ogl->pglTexParameteri && 
		ogl->pglEnable && ogl->pglDisable && ogl->pglShadeModel && ogl->pglMatrixMode && ogl->pglLoadIdentity && 
		ogl->pglClearColor && ogl->pglClear && ogl->pglColor3f && ogl->pglTexCoord2f && ogl->pglVertex2f)){
		OutTraceE("%s: ERROR initialize opengl32 failed\n", api);
		return FALSE;
	}

	switch(dxw.FilterId){
		case DXW_FILTER_ROTATE90: oglRotation = OGL_ROTATION_90; break;
		case DXW_FILTER_ROTATE180: oglRotation = OGL_ROTATION_180; break;
		case DXW_FILTER_ROTATE270: oglRotation = OGL_ROTATION_270; break;
		default: oglRotation = OGL_ROTATION_0; break;
	}

	return TRUE;
}

HRESULT OpenGLSyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	static BOOL bInitialized = FALSE;
	static LONG CurW = 0, CurH = 0;
	ULONG PicW, PicH;
	RECT Rect;
	DDSURFACEDESC2 ddsd;
	HRESULT res;
	int tex_w, tex_h;
	LPVOID texbuf;
	HGLRC oglContext;


	// initialization, just once and for all
	if(!bInitialized){
		GLuint Tex;
		if(!oglInitialize(api, &wgl, &ogl)) return DD_FALSE;
		(*ogl.pglGenTextures)(1, &Tex);
		(*ogl.pglBindTexture)(GL_TEXTURE_2D, Tex);
		OutTraceOGL("%s: opengl renderer initialized\n", api);
		//RebuildContext(api, pwglMakeCurrent, pwglCreateContext, pwglDeleteContext);
		RebuildContext(api, &wgl);
		bInitialized = TRUE;
	}

	// v2.05.43: what is the correct criteria to renew the opengl context?
	// ddhack doesn't pose the question: the WC games only require a single context created at startup
	// this is equivalent here to line "method 1:", oglContext set only once first time through
	// but this doesn't work in general if you change something, like window size or color depth
	// line "method 2:" was the previous DxWnd implementation, context is rebuilt every time it has a different value
	// but this leads to repeated and unnecessary context rebuild and a crash in "A.M.E.R.I.C.A No Peace Beyond the Line"
	// current solution is to rebuild the context whenever a valid context no longer exists.
	// method 1: if(oglContext == (HGLRC)-1){
	// method 2: if((*pwglGetCurrentContext)() != oglContext){
	//if((*pwglGetCurrentContext)() == NULL){
	// v2.05.46: it still happens that the context gets invalidated. Setting gWindowDC to NULL is a criteria
	// to notify the blitter loop to rebuild a new context. This makes "Re-Volt" working also after switching
	// between 2D and 3D modes.
	// v2.05.48: revised all behavior, taking in proper account the fact that a glGetCurrentContext returning NULL
	// with 0 as errorcode means a valid opengl rendering context. Should reduce the hdc leakage in DK2.
	// v2.05.56: reworked again ...
	SetLastError(0);
	oglContext = (*wgl.pwglGetCurrentContext)();
	if((oglContext == NULL) && GetLastError()){
		OutTrace("wglGetCurrentContext returns NULL err=%d\n", GetLastError());
		RebuildContext(api, &wgl);
	}

	// clear the clipper to avoid conflicts with the ogl Viewport
	// fixes "Age of Empires" partial blitting
	// warning: this DOESN'T fix "Abuse", you still have to disable video clipper.
	extern SetClipper_Type pSetClipper1, pSetClipper2, pSetClipper3, pSetClipper4, pSetClipper7;
	switch(dxversion){
		case 1: if(pSetClipper1) (*pSetClipper1)(lpdds, NULL); break;
		case 2: if(pSetClipper2) (*pSetClipper2)(lpdds, NULL); break;
		case 3: if(pSetClipper3) (*pSetClipper3)(lpdds, NULL); break;
		case 4: if(pSetClipper4) (*pSetClipper4)(lpdds, NULL); break;
		case 7: if(pSetClipper7) (*pSetClipper7)(lpdds, NULL); break;
	}

#if 0 // debug
	LPRECT p = lpdestrect;
	if (p)
		OutTrace(">>> dstrect=(%d,%d)-(%d,%d)\n", p->left, p->top, p->right, p->bottom);
	else 
		OutTrace(">>> dstrect=NULL\n");
	p = lpsrcrect;
	if (p)
		OutTrace(">>> srcrect=(%d,%d)-(%d,%d)\n", p->left, p->top, p->right, p->bottom);
	else 
		OutTrace(">>> srcrect=NULL\n");
	OutTrace(">>> flags=%#x fx=%#x\n", dwflags, lpddbltfx);
#endif // end debug

	// v2.05.56: fix - don't blit if the surfaces are the same (after a Update on primary surface)
	if(lpdds != s) {
		res = (*pBlt)(lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
		if(res != DD_OK){
			OutTraceE("%s: opengl Blt ERROR err=%#x(%s)\n", api, res, ExplainDDError(res));
			if(res == DDERR_UNSUPPORTED) {
				// do not desist, try blitting directly from source
				lpdds = s;
			}
			else {
				return res;
			}
		}
		else {
			// don't show overlay on s surface!!
			dxw.ShowOverlay(lpdds);
		}
	}

	// check for size changes (including initial call)
	HWND hwnd = dxw.GethWnd();
	(*pGetClientRect)(hwnd, &Rect);
	if((Rect.right != CurW) || (Rect.bottom != CurH)){
		CurW = Rect.right;
		CurH = Rect.bottom;
		// change Viewport ???
		(*ogl.pglViewport)(0, 0, CurW, CurH);
		OutTraceOGL("%s: set video hwnd=%#x size=(%dx%d)\n", api, hwnd, CurW, CurH);
	}

	// get surface infos & lock
	memset(&ddsd,0,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = Set_dwSize_From_Surface();
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	if(res=(*pLockMethod(dxversion))(lpdds, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_READONLY, 0)){	
		OutTraceE("%s: Lock ERROR res=%#x(%s) @%d\n", api, res, ExplainDDError(res), __LINE__);
		(*pUnlockMethod(dxversion))(lpdds, NULL);
		return DD_FALSE;
	}
	OutTraceOGL("%s: surface size=(%dx%d) data=%#x pitch=%#x bpp=%d fourcc=%#x rgba=%#x.%#x.%#x.%#x\n", 
		api, ddsd.dwWidth, ddsd.dwHeight, ddsd.lpSurface, ddsd.lPitch,
		ddsd.ddpfPixelFormat.dwRGBBitCount, ddsd.ddpfPixelFormat.dwFourCC,
		ddsd.ddpfPixelFormat.dwRBitMask, ddsd.ddpfPixelFormat.dwGBitMask, ddsd.ddpfPixelFormat.dwBBitMask,
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask);

	// save the picture size
	PicW = ddsd.dwWidth;
	PicH = ddsd.dwHeight;

	LPVOID pixels = ddsd.lpSurface;

	// convert FourCC format (if possible)
	BOOL bFourCC = FALSE;
	if(ddsd.ddpfPixelFormat.dwFourCC) {
		OutDebugOGL("%s: Convert FourCC=%#x\n", api, ddsd.ddpfPixelFormat.dwFourCC);
		pixels = dxwConvertFourCC(&ddsd, ddsd.ddpfPixelFormat.dwRGBBitCount);
		if(pixels) {
			bFourCC = TRUE;
		}
		else {
			OutTraceE("%s: ConvertFourCC ERROR\n", api);
			(*pUnlockMethod(dxversion))(lpdds, NULL);
			return DD_FALSE;
		}
	}

	tex_w = uppower2(ddsd.dwWidth);
	tex_h = uppower2(ddsd.dwHeight);
	OutDebugOGL("%s: surface size=(%dx%d) texture size=(%dx%d)\n", 
		api, ddsd.dwWidth, ddsd.dwHeight, tex_w, tex_h);

	texbuf = malloc((tex_w * tex_h * sizeof(DWORD)) + (0x10 * sizeof(DWORD)));
	if(!texbuf){
		OutTraceE("%s: ERROR texture buffer alloc failed at=%d\n", api, __LINE__);
		return DD_FALSE;
	}

	// make sure texptr is on a 4 byte boundary
	LPVOID texptr = texbuf;
	if((DWORD)texbuf & 0x0000000F) texptr = (LPVOID)(((DWORD)texbuf & 0xFFFFFFF0) + 0x00000010);

	switch(ddsd.ddpfPixelFormat.dwRGBBitCount){
		case 8:
			switch(dxw.FilterId){
				case DXW_FILTER_DEINTERLACE:
					oglDeinterlace8Pal(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
					break;
				case DXW_FILTER_INTERLACE:
					oglInterlace8Pal(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
					break;
				default:
					oglTransform8Pal(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
					break;
			}
			break;
		case 16:
			oglTransform16(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
			break;
		case 24:
			oglTransform24(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
			break;
		case 32:
			oglTransform32(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
			break;
		default:
			MessageBox(NULL, "bad pixel color depth", "DxWnd", 0);
			break;
	}

	// v2.05.56: if pixels points to a dynamic area allocated by dxwConvertFourCC, free it.
	if(bFourCC) free(pixels);

	// V2.05.24: despite all declarations, the Windows implementation of OpenGL glTexImage2D, though
	// with valid arguments, COULD throw an exception! In that case the best policy is to ignore the 
	// problem and keep going. It happens in "Dungeon Keeper 2" DKII-DX.EXE and this workaround fixes it.
	// v2.05.56: placed all two error conditions (exception and GetError) in a recovery loop with
	// rebuilding of wgl context

	while(true) {
		BOOL success = TRUE;
		__try {
			(*ogl.pglTexImage2D)(
				GL_TEXTURE_2D, 
				0, 
				GL_RGB, 
				tex_w, tex_h,
				0, 
				GL_RGBA, 
				GL_UNSIGNED_BYTE, 
				texptr);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			OutTraceE("%s: ERROR glTexImage2D exception at=%d\n", api, __LINE__);
			RebuildContext(api, &wgl);
			success = FALSE;
		}

		if(success && ogl.pglGetError && ((*ogl.pglGetError)() != GL_NO_ERROR)){
			OutTraceE("%s: ERROR glTexImage2D err=%#x texptr=%#x at=%d\n", api, (*ogl.pglGetError)(), texptr, __LINE__);
			RebuildContext(api, &wgl);
			success = FALSE;
		}
		if (success) break;
		else Sleep(5);
	}

	if(dxw.dwFlags5 & BILINEARFILTER){
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	else {
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	(*ogl.pglEnable)(GL_TEXTURE_2D);
	(*ogl.pglShadeModel)(GL_SMOOTH);
	(*ogl.pglClearColor)(0.0f, 0.0f, 0.0f, 0.0f);
	(*ogl.pglViewport)(0, 0, CurW, CurH); // necessary here?

	(*ogl.pglMatrixMode)(GL_PROJECTION);
	(*ogl.pglLoadIdentity)();

	(*ogl.pglMatrixMode)(GL_MODELVIEW);
	(*ogl.pglLoadIdentity)();

	if(dxw.FilterId == DXW_FILTER_BLUR){
		(*ogl.pglEnable)(GL_BLEND);
		(*ogl.pglColor4f)(1.0f,1.0f,1.0f,1.5f / (4 + 1.0f)); 
		(*ogl.pglBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		(*ogl.pglClear)(GL_COLOR_BUFFER_BIT);
		(*ogl.pglDisable)(GL_BLEND);
		(*ogl.pglColor3f)(1.0f,1.0f,1.0f); 
	}

	// Do the actual rendering.

	float w = 1.0f, h = 1.0f;

 	// Handle the fact that while our texture is a power of two,
	// the area we're using isn't.
	float wRatio = (float)PicW / (float)tex_w;
	float hRatio = (float)PicH / (float)tex_h;
	// beware of float rounding errors, anything greater than 1.0 means overflow
	if(wRatio > 1.0f) wRatio = 1.0f;
	if(hRatio > 1.0f) hRatio = 1.0f;

	//if(ogl.pglFinish) (*ogl.pglFinish)();
    (*ogl.pglBegin)(GL_TRIANGLE_FAN);
#ifdef OGLROTATION
	switch(oglRotation){
		case OGL_ROTATION_0:
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)( -w,  h);
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)(  w,  h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)(  w, -h);	
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)( -w, -h);
			break;
		case OGL_ROTATION_90:
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)(  w, -h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)( -w, -h);
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)( -w,  h);	
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)(  w,  h);
			break;
		case OGL_ROTATION_180:
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)(  w, -h);
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)( -w, -h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)( -w,  h);	
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)(  w,  h);
			break;
		case OGL_ROTATION_270:
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)( -w,  h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)(  w,  h);
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)(  w, -h);	
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)( -w, -h);
			break;
	}
#else
	(*ogl.pglTexCoord2f)(0.0f,0.0f); (*ogl.pglVertex2f)( -w,  h);
	(*ogl.pglTexCoord2f)(wRatio,0.0f); (*ogl.pglVertex2f)(  w,  h);
	(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)(  w, -h);	
	(*ogl.pglTexCoord2f)(0.0f,hRatio); (*ogl.pglVertex2f)( -w, -h);
#endif // OGLROTATION
    (*ogl.pglEnd)();

	free(texbuf);

	HDC gWindowDC = (*pGDIGetDC)(hwnd);
	if(!SwapBuffers(gWindowDC)){
		OutTraceE("%s: SwapBuffers(%#x) ERROR err=%d\n", api, gWindowDC, GetLastError());
	}
	// unlock
	res=(*pUnlockMethod(dxversion))(lpdds, NULL);
	(*pGDIReleaseDC)(hwnd, gWindowDC);

#ifndef DXW_NOTRACES
	if (res) OutTraceE("%s: Unlock ERROR res=%#x(%s) @%d\n", api, res, ExplainDDError(res), __LINE__);
#endif // DXW_NOTRACES

	if((dxw.dwDFlags & CAPTURESCREENS) && dxw.bCustomKeyToggle) dxw.ScreenShot(); 

	return DD_OK;
}

int curdxversion;

static DWORD OGLAsyncBlit()
{
	ApiName("OGLAsyncBlt");
	HGLRC oglContext;
	HRESULT res;
	static LONG CurW = 0, CurH = 0;
	ULONG PicW, PicH;
	RECT Rect;
	DDSURFACEDESC2 ddsd;
	int tex_w, tex_h;
	LPVOID texbuf;
	int dxversion = dxw.BlitterDXVersion;
	LPDIRECTDRAWSURFACE lpdds = dxw.lpBlitterSurface;

	// v2.05.43: what is the correct criteria to renew the opengl context?
	// ddhack doesn't pose the question: the WC games only require a single context created at startup
	// this is equivalent here to line "method 1:", oglContext set only once first time through
	// but this doesn't work in general if you change something, like window size or color depth
	// line "method 2:" was the previous DxWnd implementation, context is rebuilt every time it has a different value
	// but this leads to repeated and unnecessary context rebuild and a crash in "A.M.E.R.I.C.A No Peace Beyond the Line"
	// current solution is to rebuild the context whenever a valid context no longer exists.
	// method 1: if(oglContext == (HGLRC)-1){
	// method 2: if((*pwglGetCurrentContext)() != oglContext){
	//if((*pwglGetCurrentContext)() == NULL){
	// v2.05.46: it still happens that the context gets invalidated. Setting gWindowDC to NULL is a criteria
	// to notify the blitter loop to rebuild a new context. This makes "Re-Volt" working also after switching
	// between 2D and 3D modes.
	// v2.05.48: revised all behavior, taking in proper account the fact that a glGetCurrentContext returning NULL
	// with 0 as errorcode means a valid opengl rendering context. Should reduce the hdc leakage in DK2.
	// v2.05.56: reworked again ...
	SetLastError(0);
	oglContext = (*wgl.pwglGetCurrentContext)();
	if((oglContext == NULL) && GetLastError()){
		OutTrace("%s: wglGetCurrentContext returns NULL err=%d\n", ApiRef, GetLastError());
		RebuildContext(api, &wgl);
	}

	// check for size changes (including initial call)
	HWND hwnd = dxw.GethWnd();
	(*pGetClientRect)(hwnd, &Rect);
	if((Rect.right != CurW) || (Rect.bottom != CurH)){
		CurW = Rect.right;
		CurH = Rect.bottom;
		// change Viewport ???
		(*ogl.pglViewport)(0, 0, CurW, CurH);
		OutTraceOGL("%s: set video hwnd=%#x size=(%dx%d)\n", ApiRef, hwnd, CurW, CurH);
	}

	// get surface infos & lock
	memset(&ddsd,0,sizeof(DDSURFACEDESC2));
	ddsd.dwSize = Set_dwSize_From_Surface();
	ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
	if(res=(*pLockMethod(dxversion))(lpdds, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_READONLY, 0)){	
		OutTraceE("%s: Lock ERROR res=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
		(*pUnlockMethod(dxversion))(lpdds, NULL);
		return DD_FALSE;
	}
	OutTraceOGL("%s: surface size=(%dx%d) data=%#x pitch=%#x bpp=%d fourcc=%#x rgba=%#x.%#x.%#x.%#x\n", 
		ApiRef, ddsd.dwWidth, ddsd.dwHeight, ddsd.lpSurface, ddsd.lPitch,
		ddsd.ddpfPixelFormat.dwRGBBitCount, ddsd.ddpfPixelFormat.dwFourCC,
		ddsd.ddpfPixelFormat.dwRBitMask, ddsd.ddpfPixelFormat.dwGBitMask, ddsd.ddpfPixelFormat.dwBBitMask,
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask);

	// save the picture size
	PicW = ddsd.dwWidth;
	PicH = ddsd.dwHeight;

	LPVOID pixels = ddsd.lpSurface;

	// convert FourCC format (if possible)
	BOOL bFourCC = FALSE;
	if(ddsd.ddpfPixelFormat.dwFourCC) {
		OutDebugOGL("%s: Convert FourCC=%#x\n", ApiRef, ddsd.ddpfPixelFormat.dwFourCC);
		pixels = dxwConvertFourCC(&ddsd, ddsd.ddpfPixelFormat.dwRGBBitCount);
		if(pixels) {
			bFourCC = TRUE;
		}
		else {
			OutTraceE("%s: ConvertFourCC ERROR\n", ApiRef);
			(*pUnlockMethod(dxversion))(lpdds, NULL);
			return DD_FALSE;
		}
	}

	tex_w = uppower2(ddsd.dwWidth);
	tex_h = uppower2(ddsd.dwHeight);
	OutDebugOGL("%s: surface size=(%dx%d) texture size=(%dx%d)\n", 
		ApiRef, ddsd.dwWidth, ddsd.dwHeight, tex_w, tex_h);

	texbuf = malloc((tex_w * tex_h * sizeof(DWORD)) + (0x10 * sizeof(DWORD)));
	if(!texbuf){
		OutTraceE("%s: ERROR texture buffer alloc failed at=%d\n", ApiRef, __LINE__);
		return DD_FALSE;
	}

	// make sure texptr is on a 4 byte boundary
	LPVOID texptr = texbuf;
	if((DWORD)texbuf & 0x0000000F) texptr = (LPVOID)(((DWORD)texbuf & 0xFFFFFFF0) + 0x00000010);

	switch(ddsd.ddpfPixelFormat.dwRGBBitCount){
		case 8:
			switch(dxw.FilterId){
				case DXW_FILTER_DEINTERLACE:
					oglDeinterlace8Pal(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
					break;
				case DXW_FILTER_INTERLACE:
					oglInterlace8Pal(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
					break;
				default:
					oglTransform8Pal(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
					break;
			}
			break;
		case 16:
			oglTransform16(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
			break;
		case 24:
			oglTransform24(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
			break;
		case 32:
			oglTransform32(pixels, texptr, ddsd.dwWidth, ddsd.dwHeight, tex_w, ddsd.lPitch);
			break;
		default:
			MessageBox(NULL, "bad pixel color depth", "DxWnd", 0);
			break;
	}

	// v2.05.56: if pixels points to a dynamic area allocated by dxwConvertFourCC, free it.
	if(bFourCC) free(pixels);

	// V2.05.24: despite all declarations, the Windows implementation of OpenGL glTexImage2D, though
	// with valid arguments, COULD throw an exception! In that case the best policy is to ignore the 
	// problem and keep going. It happens in "Dungeon Keeper 2" DKII-DX.EXE and this workaround fixes it.
	// v2.05.56: placed all two error conditions (exception and GetError) in a recovery loop with
	// rebuilding of wgl context

	while(true) {
		BOOL success = TRUE;
		__try {
			(*ogl.pglTexImage2D)(
				GL_TEXTURE_2D, 
				0, 
				GL_RGB, 
				tex_w, tex_h,
				0, 
				GL_RGBA, 
				GL_UNSIGNED_BYTE, 
				texptr);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			OutTraceE("%s: ERROR glTexImage2D exception at=%d\n", ApiRef, __LINE__);
			RebuildContext(api, &wgl);
			success = FALSE;
		}

		if(success && ogl.pglGetError && ((*ogl.pglGetError)() != GL_NO_ERROR)){
			OutTraceE("%s: ERROR glTexImage2D err=%#x texptr=%#x at=%d\n", ApiRef, (*ogl.pglGetError)(), texptr, __LINE__);
			RebuildContext(api, &wgl);
			success = FALSE;
		}
		if (success) break;
		else Sleep(5);
	}

	if(dxw.dwFlags5 & BILINEARFILTER){
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	else {
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		(*ogl.pglTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	(*ogl.pglEnable)(GL_TEXTURE_2D);
	(*ogl.pglShadeModel)(GL_SMOOTH);
	(*ogl.pglClearColor)(0.0f, 0.0f, 0.0f, 0.0f);
	(*ogl.pglViewport)(0, 0, CurW, CurH); // necessary here?

	(*ogl.pglMatrixMode)(GL_PROJECTION);
	(*ogl.pglLoadIdentity)();

	(*ogl.pglMatrixMode)(GL_MODELVIEW);
	(*ogl.pglLoadIdentity)();

	if(dxw.FilterId == DXW_FILTER_BLUR){
		(*ogl.pglEnable)(GL_BLEND);
		(*ogl.pglColor4f)(1.0f,1.0f,1.0f,1.5f / (4 + 1.0f)); 
		(*ogl.pglBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		(*ogl.pglClear)(GL_COLOR_BUFFER_BIT);
		(*ogl.pglDisable)(GL_BLEND);
		(*ogl.pglColor3f)(1.0f,1.0f,1.0f); 
	}

	// Do the actual rendering.

	float w = 1.0f, h = 1.0f;

 	// Handle the fact that while our texture is a power of two,
	// the area we're using isn't.
	float wRatio = (float)PicW / (float)tex_w;
	float hRatio = (float)PicH / (float)tex_h;
	// beware of float rounding errors, anything greater than 1.0 means overflow
	if(wRatio > 1.0f) wRatio = 1.0f;
	if(hRatio > 1.0f) hRatio = 1.0f;

	//if(ogl.pglFinish) (*ogl.pglFinish)();
    (*ogl.pglBegin)(GL_TRIANGLE_FAN);
#ifdef OGLROTATION
	switch(oglRotation){
		case OGL_ROTATION_0:
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)( -w,  h);
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)(  w,  h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)(  w, -h);	
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)( -w, -h);
			break;
		case OGL_ROTATION_90:
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)(  w, -h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)( -w, -h);
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)( -w,  h);	
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)(  w,  h);
			break;
		case OGL_ROTATION_180:
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)(  w, -h);
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)( -w, -h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)( -w,  h);	
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)(  w,  h);
			break;
		case OGL_ROTATION_270:
			(*ogl.pglTexCoord2f)(wRatio,0); (*ogl.pglVertex2f)( -w,  h);
			(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)(  w,  h);
			(*ogl.pglTexCoord2f)(0,hRatio); (*ogl.pglVertex2f)(  w, -h);	
			(*ogl.pglTexCoord2f)(0,0); (*ogl.pglVertex2f)( -w, -h);
			break;
	}
#else
	(*ogl.pglTexCoord2f)(0.0f,0.0f); (*ogl.pglVertex2f)( -w,  h);
	(*ogl.pglTexCoord2f)(wRatio,0.0f); (*ogl.pglVertex2f)(  w,  h);
	(*ogl.pglTexCoord2f)(wRatio,hRatio); (*ogl.pglVertex2f)(  w, -h);	
	(*ogl.pglTexCoord2f)(0.0f,hRatio); (*ogl.pglVertex2f)( -w, -h);
#endif // OGLROTATION
    (*ogl.pglEnd)();

	free(texbuf);

	HDC gWindowDC = (*pGDIGetDC)(hwnd);
	if(!SwapBuffers(gWindowDC)){
		OutTraceE("%s: SwapBuffers(%#x) ERROR err=%d\n", ApiRef, gWindowDC, GetLastError());
	}
	// unlock
	res=(*pUnlockMethod(dxversion))(lpdds, NULL);
	(*pGDIReleaseDC)(hwnd, gWindowDC);

#ifndef DXW_NOTRACES
	if (res) OutTraceE("%s: Unlock ERROR res=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
#endif // DXW_NOTRACES

	if((dxw.dwDFlags & CAPTURESCREENS) && dxw.bCustomKeyToggle) dxw.ScreenShot(); 
	return DD_OK;
}

static DWORD WINAPI OGLAsyncBlitter(LPVOID lpParameter)
{
	while(TRUE){
		if (!dxw.lpBlitterSurface) continue;

		extern void LimitFrameHz(DWORD);  
		LimitFrameHz(60);

		// Request ownership of the critical section.
		EnterCriticalSection(&dxw.CriticalSection); 
		OGLAsyncBlit();
		// Release ownership of the critical section.
		LeaveCriticalSection(&dxw.CriticalSection);
	}
}

HRESULT OpenGLAsyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	HRESULT res = DD_OK;

	dxw.lpBlitterSurface = lpdds;
	dxw.BlitterDXVersion = dxversion;

	// initialization, just once and for all
	if(!dxw.bAsybcBlitStarted){
		GLuint Tex;
		if(!oglInitialize(api, &wgl, &ogl)) return DD_FALSE;
		(*ogl.pglGenTextures)(1, &Tex);
		(*ogl.pglBindTexture)(GL_TEXTURE_2D, Tex);
		OutTraceOGL("%s: opengl renderer initialized\n", api);
		//RebuildContext(api, pwglMakeCurrent, pwglCreateContext, pwglDeleteContext);
		RebuildContext(api, &wgl);
		// Initialize the critical section one time only.
		if (!InitializeCriticalSectionAndSpinCount(&dxw.CriticalSection, 0x00000400) ) {
			OutTraceE("%s: ERROR in InitializeCriticalSectionAndSpinCount err=%d\n", api, GetLastError());
			return -1;
		}
		CreateThread(NULL, 0, OGLAsyncBlitter, (LPVOID)lpdds, 0, NULL);
		dxw.bAsybcBlitStarted = TRUE;
	}

	// clear the clipper to avoid conflicts with the ogl Viewport
	// fixes "Age of Empires" partial blitting
	// warning: this DOESN'T fix "Abuse", you still have to disable video clipper.
	extern SetClipper_Type pSetClipper1, pSetClipper2, pSetClipper3, pSetClipper4, pSetClipper7;
	switch(dxversion){
		case 1: if(pSetClipper1) (*pSetClipper1)(lpdds, NULL); break;
		case 2: if(pSetClipper2) (*pSetClipper2)(lpdds, NULL); break;
		case 3: if(pSetClipper3) (*pSetClipper3)(lpdds, NULL); break;
		case 4: if(pSetClipper4) (*pSetClipper4)(lpdds, NULL); break;
		case 7: if(pSetClipper7) (*pSetClipper7)(lpdds, NULL); break;
	}

#if 0 // debug
	LPRECT p = lpdestrect;
	if (p)
		OutTrace(">>> dstrect=(%d,%d)-(%d,%d)\n", p->left, p->top, p->right, p->bottom);
	else 
		OutTrace(">>> dstrect=NULL\n");
	p = lpsrcrect;
	if (p)
		OutTrace(">>> srcrect=(%d,%d)-(%d,%d)\n", p->left, p->top, p->right, p->bottom);
	else 
		OutTrace(">>> srcrect=NULL\n");
	OutTrace(">>> flags=%#x fx=%#x\n", dwflags, lpddbltfx);
#endif // end debug

	// v2.05.56: fix - don't blit if the surfaces are the same (after a Update on primary surface)
	if(lpdds != s) {
		// Request ownership of the critical section.
		EnterCriticalSection(&dxw.CriticalSection); 
		res = (*pBlt)(lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
		// Release ownership of the critical section.
		LeaveCriticalSection(&dxw.CriticalSection);
		if(res != DD_OK){
			OutTraceE("%s: opengl Blt ERROR err=%#x(%s)\n", api, res, ExplainDDError(res));
		}
		else {
			// don't show overlay on s surface!!
			dxw.ShowOverlay(lpdds);
		}
	}

	return DD_OK; // better cheat a little ...
	// return res;
}
