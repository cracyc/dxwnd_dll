#define _CRT_SECURE_NO_WARNINGS

#define STRETCHDRAWPIXELS 
#define STRETCHBITMAPS
#define MAXEXTLEN 1024

#define _MODULE "opengl32"

//#define SCALEDUMP
//#define TRACEALL
//#define NOOPENGLARBEXTENSIONS


#include "stdio.h"
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include <gl\gl.h>
#include "wglext.h"
#include "glext.h"
#include "float.h"
#ifdef STRETCHBITMAPS
#include <math.h>
#endif
#include "MinHook.h" 

#define DXWDECLARATIONS TRUE
#include "glhook.h"
#undef DXWDECLARATIONS

#ifndef COMPRESSED_RGB_S3TC_DXT1_EXT
#define COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3
#endif

// v2.06.07 fix: do NOT use glGetError() to print error messages within the wrappers because this call will clear the
// error code so preventing the calling application to detect any error code

// gbScreenOverlap: "Scratches (Director's Cut)" uses a trick to emulate gamma correction in OpenGL: it copies the screen 
// image to a texture that is recolored and overlapped to the original image with transparency to simulate a clearer image.
// By default DxWnd would scale this texture as well as any other, so the gamma correction would produce artifacts.
// The gbScreenOverlap boolean keep track of texture copies from screen and un-scales the texture coordinates to compensate
// and reproduce the original effect. 
// N.b. The game uses a 1024x1024 texture for this purpose, so if the game is scaled to bigger sizes the trick doesn't work.
// Also, though it could be possible to generalize the gamma correction schema, this hack only affects the OpenGL calls used
// in this single game. 
BOOL gbScreenOverlap = FALSE;

//#include "logall.h"
extern void glHandleTexture(GLenum, GLint, GLenum, GLsizei, GLsizei , GLenum, const GLvoid *);
extern void glHandleCompressedTexture(GLenum, GLint, GLsizei, GLsizei , GLsizei, const GLvoid *);

typedef void (WINAPI *glTexCoord2f_Type)(GLfloat, GLfloat);
glTexCoord2f_Type pglTexCoord2f;
void WINAPI extglTexCoord2f(GLfloat, GLfloat);
typedef void (WINAPI *glTexCoord2fv_Type)(const GLfloat *);
glTexCoord2fv_Type pglTexCoord2fv;
void WINAPI extglTexCoord2fv(const GLfloat *);

#ifdef SUPPRESSLIGHTMODETWOSIDE
typedef void (WINAPI *glLightModeli_Type)(GLenum, GLint);
glLightModeli_Type pglLightModeli;
void WINAPI extglLightModeli(GLenum, GLint);
#endif

// glut + wgl
typedef void (WINAPI *glutFullScreen_Type)(void);
glutFullScreen_Type pglutFullScreen;
void WINAPI extglutFullScreen(void);
typedef void (WINAPI *glutInitWindowSize_Type)(int, int);
glutInitWindowSize_Type pglutInitWindowSize;
void extglutInitWindowSize(int, int);
typedef void (WINAPI *glutInitWindowPosition_Type)(int, int);
glutInitWindowPosition_Type pglutInitWindowPosition;
void extglutInitWindowPosition(int, int);
typedef void (WINAPI *glutSetWindow_Type)(HWND);
glutSetWindow_Type pglutSetWindow;
void WINAPI extglutSetWindow(HWND);
typedef const GLubyte* (WINAPI *glGetString_Type)(GLenum);
glGetString_Type pglGetString;
const GLubyte* WINAPI extglGetString(GLenum);
typedef char* (WINAPI *wglGetExtensionsStringEXT_Type)(void);
wglGetExtensionsStringEXT_Type pwglGetExtensionsStringEXT;
char* WINAPI extwglGetExtensionsStringEXT(void);
#ifdef NOOPENGLARBEXTENSIONS
typedef char* (WINAPI *wglGetExtensionsStringARB_Type)(DWORD);
wglGetExtensionsStringARB_Type pwglGetExtensionsStringARB;
char* WINAPI extwglGetExtensionsStringARB(DWORD);
#endif
typedef const GLubyte* (WINAPI *gluGetString_Type)(GLenum);
gluGetString_Type pgluGetString;
const GLubyte* WINAPI extgluGetString(GLenum);
#ifdef TRACEALL
typedef void (WINAPI *glDeleteLists_Type)(GLuint, GLsizei);
glDeleteLists_Type pglDeleteLists;
void WINAPI extglDeleteLists(GLuint, GLsizei);
typedef void (WINAPI *glutMotionFunc_Type)(void (*func)(int x, int y));
glutMotionFunc_Type pglutMotionFunc, pglutPassiveMotionFunc;
void WINAPI extglutMotionFunc(void (*func)(int x, int y));
void WINAPI extglutPassiveMotionFunc(void (*func)(int x, int y));
typedef void (WINAPI *glutSwapBuffers_Type)(void);
glutSwapBuffers_Type pglutSwapBuffers;
void WINAPI extglutSwapBuffers(void);
typedef void (WINAPI *gluBuild1DMipmaps_Type)(GLenum, GLint, GLint, GLenum, GLenum, const void *);
gluBuild1DMipmaps_Type pgluBuild1DMipmaps;
void WINAPI extgluBuild1DMipmaps(GLenum, GLint, GLint, GLenum, GLenum, const void *);
typedef void (WINAPI *gluBuild2DMipmaps_Type)(GLenum, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
gluBuild2DMipmaps_Type pgluBuild2DMipmaps;
void WINAPI extgluBuild2DMipmaps(GLenum, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
typedef void (WINAPI *glGenTextures_Type)(GLsizei, GLuint *);
glGenTextures_Type pglGenTextures;
void WINAPI extglGenTextures(GLsizei, GLuint *);
typedef void (WINAPI *gluOrtho2D_Type)(GLdouble, GLdouble, GLdouble, GLdouble);
gluOrtho2D_Type pgluOrtho2D;
void WINAPI extgluOrtho2D(GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (WINAPI *glHint_Type)(GLenum, GLenum);
glHint_Type pglHint;
void WINAPI extglHint(GLenum, GLenum);
typedef void (WINAPI *glFrontFace_Type)(GLenum);
glFrontFace_Type pglFrontFace;
void WINAPI extglFrontFace(GLenum);
typedef void (WINAPI *glEnableClientState_Type)(GLenum);
glEnableClientState_Type pglEnableClientState;
void WINAPI extglEnableClientState(GLenum);
typedef void (WINAPI *glMateriali_Type)(GLenum, GLenum, GLint);
glMateriali_Type pglMateriali;
void WINAPI extglMateriali(GLenum, GLenum, GLint);
typedef void (WINAPI *glMaterialfv_Type)(GLenum, GLenum, const GLfloat *);
glMaterialfv_Type pglMaterialfv;
void WINAPI extglMaterialfv(GLenum, GLenum, const GLfloat *);
typedef void (WINAPI *glLightf_Type)(GLenum, GLenum, GLfloat);
glLightf_Type pglLightf;
void WINAPI extglLightf(GLenum, GLenum, GLfloat);
typedef void (WINAPI *glLighti_Type)(GLenum, GLenum, GLint);
glLighti_Type pglLighti;
void WINAPI extglLighti(GLenum, GLenum, GLint);
typedef void (WINAPI *glLightfv_Type)(GLenum, GLenum, const GLfloat *);
glLightfv_Type pglLightfv;
void WINAPI extglLightfv(GLenum, GLenum, const GLfloat *);
typedef void (WINAPI *glLightiv_Type)(GLenum, GLenum, const GLint *);
glLightiv_Type pglLightiv;
void WINAPI extglLightiv(GLenum, GLenum, const GLint *);
typedef void (WINAPI *glPushMatrix_Type)();
glPushMatrix_Type pglPushMatrix;
void WINAPI extglPushMatrix();
typedef void (WINAPI *glRotated_Type)(GLdouble, GLdouble, GLdouble, GLdouble);
glRotated_Type pglRotated;
void WINAPI extglRotated(GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (WINAPI *glRotatef_Type)(GLfloat, GLfloat, GLfloat, GLfloat);
glRotatef_Type pglRotatef;
void WINAPI extglRotatef(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *glArrayElement_Type)(GLint);
glArrayElement_Type pglArrayElement;
void WINAPI extglArrayElement(GLint);
typedef void (WINAPI *glRasterPos4fv_Type)(const GLfloat *);
glRasterPos4fv_Type pglRasterPos4fv;
void WINAPI extglRasterPos4fv(const GLfloat *);
typedef void (WINAPI *glRasterPos2i_Type)(GLint, GLint);
glRasterPos2i_Type pglRasterPos2i;
void WINAPI extglRasterPos2i(GLint, GLint);
typedef void (WINAPI *glRasterPos2d_Type)(GLdouble, GLdouble);
glRasterPos2d_Type pglRasterPos2d;
void WINAPI extglRasterPos2d(GLdouble, GLdouble);
typedef void (WINAPI *glRasterPos2f_Type)(GLfloat, GLfloat);
glRasterPos2f_Type pglRasterPos2f;
void WINAPI extglRasterPos2f(GLfloat, GLfloat);
typedef void (WINAPI *glBlendColor_Type)(GLfloat, GLfloat,GLfloat, GLfloat);
glBlendColor_Type pglBlendColor;
void WINAPI extglBlendColor(GLfloat, GLfloat,GLfloat, GLfloat);
typedef void (WINAPI *glReadPixels_Type)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
typedef void (WINAPI *glReadnPixels_Type)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, void *);
glReadPixels_Type pglReadPixels;
glReadnPixels_Type pglReadnPixels;
void WINAPI extglReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
void WINAPI extglReadnPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, void *);
typedef void (WINAPI *glListBase_Type)(GLuint);
glListBase_Type pglListBase;
void WINAPI extglListBase(GLuint);
typedef void (WINAPI *glTexCoordPointer_Type)(GLint, GLenum, GLsizei, const void *);
glTexCoordPointer_Type pglTexCoordPointer;
void WINAPI extglTexCoordPointer(GLint, GLenum, GLsizei, const void *);
#endif

typedef void (WINAPI *glBitmap_Type)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
glBitmap_Type pglBitmap;
void WINAPI extglBitmap(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
typedef void (WINAPI *glTexSubImage1D_Type)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
glTexSubImage1D_Type pglTexSubImage1D;
void WINAPI extglTexSubImage1D(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
typedef void (WINAPI *glTexSubImage2D_Type)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
glTexSubImage2D_Type pglTexSubImage2D;
void WINAPI extglTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
typedef void (WINAPI *glTexParameteri_Type)(GLenum, GLenum, GLint);
glTexParameteri_Type pglTexParameteri, pglTexEnvi;
void WINAPI extglTexParameteri(GLenum, GLenum, GLint);
typedef void (WINAPI *glTexParameterf_Type)(GLenum, GLenum, GLfloat);
glTexParameterf_Type pglTexParameterf, pglTexEnvf;
void WINAPI extglTexParameterf(GLenum, GLenum, GLfloat);

typedef void (WINAPI *glBlendFunc_Type)(GLenum, GLenum);
glBlendFunc_Type pglBlendFunc = 0;
void WINAPI extglBlendFunc(GLenum, GLenum);
typedef void (WINAPI *glBlendFunci_Type)(GLuint, GLenum, GLenum);
glBlendFunci_Type pglBlendFunci = 0;
void WINAPI extglBlendFunci(GLuint, GLenum, GLenum);
typedef void (WINAPI *glEnable_Type)(GLenum);
glEnable_Type pglEnable = 0;
void WINAPI extglEnable(GLenum);
typedef void (WINAPI *glDisable_Type)(GLenum);
glDisable_Type pglDisable = 0;
void WINAPI extglDisable(GLenum);
typedef void (WINAPI *glFogf_Type)(GLenum, GLfloat);
glFogf_Type pglFogf = 0;
void WINAPI extglFogf(GLenum, GLfloat);
typedef void (WINAPI *glFogi_Type)(GLenum, GLint);
glFogi_Type pglFogi = 0;
void WINAPI extglFogi(GLenum, GLint);
typedef void (WINAPI *glFogfv_Type)(GLenum, const GLfloat *);
glFogfv_Type pglFogfv = 0;
void WINAPI extglFogfv(GLenum, const GLfloat *);
typedef void (WINAPI *glFogiv_Type)(GLenum, const GLint *);
glFogiv_Type pglFogiv = 0;
void WINAPI extglFogiv(GLenum, const GLint *);

#ifdef TRACEALL
typedef void (WINAPI *glColor4f_Type)(GLfloat, GLfloat, GLfloat, GLfloat);
glColor4f_Type pglColor4f;
void WINAPI extglColor4f(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *glDepthFunc_Type)(GLenum);
glDepthFunc_Type pglDepthFunc;
void WINAPI extglDepthFunc(GLenum);
typedef void (WINAPI *glClearColor_Type)(GLclampf, GLclampf, GLclampf, GLclampf);
glClearColor_Type pglClearColor = 0;
void WINAPI extglClearColor(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (WINAPI *glColorMask_Type)(GLboolean, GLboolean, GLboolean, GLboolean);
glColorMask_Type pglColorMask;
void WINAPI extglColorMask(GLboolean, GLboolean, GLboolean, GLboolean);
typedef void (WINAPI *glVertex2f_Type)(GLfloat, GLfloat);
glVertex2f_Type pglVertex2f;
void WINAPI extglVertex2f(GLfloat, GLfloat);
typedef void (WINAPI *glVertex3f_Type)(GLfloat, GLfloat, GLfloat);
glVertex3f_Type pglVertex3f;
void WINAPI extglVertex3f(GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *glFrustum_Type)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
glFrustum_Type pglFrustum;
void WINAPI extglFrustum(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (WINAPI *glDrawArrays_Type)(GLenum mode, GLint first, GLsizei count);
glDrawArrays_Type pglDrawArrays;
void WINAPI extglDrawArrays(GLenum mode, GLint first, GLsizei count);
typedef void (WINAPI *glTranslatef_Type)(GLfloat, GLfloat, GLfloat);
glTranslatef_Type pglTranslatef;
void WINAPI extglTranslatef(GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *glScalef_Type)(GLfloat, GLfloat, GLfloat);
glScalef_Type pglScalef;
void WINAPI extglScalef(GLfloat, GLfloat, GLfloat);
typedef void (WINAPI *glOrtho_Type)(GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble);
glOrtho_Type pglOrtho;
void WINAPI extglOrtho(GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble);
void WINAPI extglTexEnvi(GLenum, GLenum, GLint);
void WINAPI extglTexEnvf(GLenum, GLenum, GLfloat);
typedef void (WINAPI *glDeleteTextures_Type)(GLsizei, const GLuint *);
glDeleteTextures_Type pglDeleteTextures;
void WINAPI extglDeleteTextures(GLsizei, const GLuint *);
typedef void (WINAPI *glMatrixMode_Type)(GLenum);
glMatrixMode_Type pglMatrixMode;
void WINAPI extglMatrixMode(GLenum);
typedef void (WINAPI *glLoadIdentity_Type)(void);
glLoadIdentity_Type pglLoadIdentity;
void WINAPI extglLoadIdentity(void);
typedef void (WINAPI *glCullFace_Type)(GLenum);
glCullFace_Type pglCullFace = 0;
void WINAPI extglCullFace(GLenum);
#endif

typedef HGLRC (WINAPI *wglCreateLayerContext_Type)(HDC, int);
wglCreateLayerContext_Type pwglCreateLayerContext;
HGLRC WINAPI extwglCreateLayerContext(HDC, int);
typedef BOOL (WINAPI *wglDeleteContext_Type)(HGLRC);
wglDeleteContext_Type pwglDeleteContext;
BOOL WINAPI extwglDeleteContext(HGLRC);
typedef HGLRC (WINAPI *wglGetCurrentContext_Type)(void);
wglGetCurrentContext_Type pwglGetCurrentContext;
HGLRC WINAPI extwglGetCurrentContext(void);
typedef BOOL (WINAPI *wglCopyContext_Type)(HGLRC, HGLRC, UINT);
wglCopyContext_Type pwglCopyContext;
BOOL WINAPI extwglCopyContext(HGLRC, HGLRC, UINT);
typedef BOOL	(WINAPI *wglSetPixelFormat_Type)(HDC, int, const PIXELFORMATDESCRIPTOR *);
wglSetPixelFormat_Type pwglSetPixelFormat;
BOOL WINAPI extwglSetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR *);
typedef BOOL	(WINAPI *wglGetPixelFormat_Type)(HDC);
wglGetPixelFormat_Type pwglGetPixelFormat;
BOOL WINAPI extwglGetPixelFormat(HDC);
typedef int		(WINAPI *wglChoosePixelFormat_Type)(HDC, const PIXELFORMATDESCRIPTOR *);
wglChoosePixelFormat_Type pwglChoosePixelFormat;
int	WINAPI extwglChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR *);
typedef int		(WINAPI *wglDescribePixelFormat_Type)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
wglDescribePixelFormat_Type pwglDescribePixelFormat;
int	WINAPI extwglDescribePixelFormat(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
#ifdef TRACEALL
typedef void (WINAPI *glViewportArrayv_Type)(GLuint, GLsizei, const GLfloat *);
glViewportArrayv_Type pglViewportArrayv;
void WINAPI extglViewportArrayv(GLuint, GLsizei, const GLfloat *);

typedef void (WINAPI *glViewportIndexedf_Type)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
glViewportIndexedf_Type pglViewportIndexedf;
void WINAPI extglViewportIndexedf(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);

typedef void (WINAPI *glViewportIndexedfv_Type)(GLuint, const GLfloat *);
glViewportIndexedfv_Type pglViewportIndexedfv;
void WINAPI extglViewportIndexedfv(GLuint, const GLfloat *);

typedef void (WINAPI *glScissorArrayv_Type)(GLuint, GLsizei, const GLfloat *);
glScissorArrayv_Type pglScissorArrayv;
void WINAPI extglScissorArrayv(GLuint, GLsizei, const GLfloat *);

typedef void (WINAPI *glScissorIndexed_Type)(GLuint, GLint, GLint, GLsizei, GLsizei);
glScissorIndexed_Type pglScissorIndexed;
void WINAPI extglScissorIndexed(GLuint, GLint, GLint, GLsizei, GLsizei);

typedef void (WINAPI *glScissorIndexedf_Type)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
glScissorIndexedf_Type pglScissorIndexedf;
void WINAPI extglScissorIndexedf(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);

typedef void (WINAPI *glScissorIndexedv_Type)(GLuint, const GLint *);
glScissorIndexedv_Type pglScissorIndexedv;
void WINAPI extglScissorIndexedv(GLuint, const GLint *);

typedef void (WINAPI *glScissorIndexedfv_Type)(GLuint, const GLfloat *);
glScissorIndexedfv_Type pglScissorIndexedfv;
void WINAPI extglScissorIndexedfv(GLuint, const GLfloat *);

typedef void (WINAPI *glWindowPos2s_Type)(GLshort, GLshort);
glWindowPos2s_Type pglWindowPos2s;
void WINAPI extglWindowPos2s(GLshort, GLshort);

typedef void (WINAPI *glWindowPos2i_Type)(GLint, GLint);
glWindowPos2i_Type pglWindowPos2i;
void WINAPI extglWindowPos2i(GLint, GLint);

typedef void (WINAPI *glWindowPos2f_Type)(GLfloat, GLfloat);
glWindowPos2f_Type pglWindowPos2f;
void WINAPI extglWindowPos2f(GLfloat, GLfloat);

typedef void (WINAPI *glVertex2i_Type)(GLint, GLint);
glVertex2i_Type pglVertex2i;
void WINAPI extglVertex2i(GLint, GLint);

typedef void (WINAPI *glVertex3i_Type)(GLint, GLint, GLint);
glVertex3i_Type pglVertex3i;
void WINAPI extglVertex3i(GLint, GLint, GLint);
#endif

typedef void (WINAPI *glCopyTexSubImage1D_Type)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
glCopyTexSubImage1D_Type pglCopyTexSubImage1D;
void WINAPI extglCopyTexSubImage1D(GLenum, GLint, GLint, GLint, GLint, GLsizei);

typedef void (WINAPI *glCopyTexSubImage2D_Type)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
glCopyTexSubImage2D_Type pglCopyTexSubImage2D;
void WINAPI extglCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);

typedef void (WINAPI *glCompressedTexImage2D_Type)(GLenum, GLint, GLenum, GLsizei, GLsizei,GLint, GLsizei, const void *);
glCompressedTexImage2D_Type pglCompressedTexImage2D;
void WINAPI extglCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei,GLint, GLsizei, const void *);

#ifdef STRETCHDRAWPIXELS
void WINAPI extglDrawPixels(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
typedef void (WINAPI *glDrawPixels_Type)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
glDrawPixels_Type pglDrawPixels = NULL;
#endif

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "glEnable", NULL, (FARPROC *)&pglEnable, (FARPROC)extglEnable},
	{HOOK_IAT_CANDIDATE, 0, "glDisable", NULL, (FARPROC *)&pglDisable, (FARPROC)extglDisable},
	{HOOK_IAT_CANDIDATE, 0, "glBlendFunc", NULL, (FARPROC *)&pglBlendFunc, (FARPROC)extglBlendFunc},
	{HOOK_IAT_CANDIDATE, 0, "glBlendFunci", NULL, (FARPROC *)&pglBlendFunci, (FARPROC)extglBlendFunci},
	{HOOK_IAT_CANDIDATE, 0, "glGetError", NULL, (FARPROC *)&pglGetError, (FARPROC)extglGetError},
	{HOOK_IAT_CANDIDATE, 0, "glViewport", NULL, (FARPROC *)&pglViewport, (FARPROC)extglViewport},
	{HOOK_IAT_CANDIDATE, 0, "glScissor", NULL, (FARPROC *)&pglScissor, (FARPROC)extglScissor},
	{HOOK_IAT_CANDIDATE, 0, "glGetIntegerv", NULL, (FARPROC *)&pglGetIntegerv, (FARPROC)&extglGetIntegerv},
	{HOOK_IAT_CANDIDATE, 0, "glDrawBuffer", NULL, (FARPROC *)&pglDrawBuffer, (FARPROC)extglDrawBuffer},
	{HOOK_IAT_CANDIDATE, 0, "glPolygonMode", NULL, (FARPROC *)&pglPolygonMode, (FARPROC)extglPolygonMode},
	{HOOK_IAT_CANDIDATE, 0, "glGetFloatv", NULL, (FARPROC *)&pglGetFloatv, (FARPROC)extglGetFloatv},
	{HOOK_IAT_CANDIDATE, 0, "glClear", NULL, (FARPROC *)&pglClear, (FARPROC)extglClear},
	{HOOK_IAT_CANDIDATE, 0, "glTexImage1D", NULL, (FARPROC *)&pglTexImage1D, (FARPROC)extglTexImage1D},
	{HOOK_IAT_CANDIDATE, 0, "glTexImage2D", NULL, (FARPROC *)&pglTexImage2D, (FARPROC)extglTexImage2D},
	{HOOK_IAT_CANDIDATE, 0, "glTexImage3D", NULL, (FARPROC *)&pglTexImage3D, (FARPROC)extglTexImage3D},
	{HOOK_IAT_CANDIDATE, 0, "glTexSubImage1D", NULL, (FARPROC *)&pglTexSubImage1D, (FARPROC)extglTexSubImage1D},
	{HOOK_IAT_CANDIDATE, 0, "glTexSubImage2D", NULL, (FARPROC *)&pglTexSubImage2D, (FARPROC)extglTexSubImage2D},
	{HOOK_IAT_CANDIDATE, 0, "glPixelZoom", NULL, (FARPROC *)&pglPixelZoom, (FARPROC)extglPixelZoom},
	{HOOK_IAT_CANDIDATE, 0, "glBindTexture", NULL, (FARPROC *)&pglBindTexture, (FARPROC)extglBindTexture},
	{HOOK_IAT_CANDIDATE, 0, "glBitmap", NULL, (FARPROC *)&pglBitmap, (FARPROC)extglBitmap},
	{HOOK_IAT_CANDIDATE, 0, "glGetString", NULL, (FARPROC *)&pglGetString, (FARPROC)extglGetString},
	{HOOK_IAT_CANDIDATE, 0, "glTexParameteri", NULL, (FARPROC *)&pglTexParameteri, (FARPROC)extglTexParameteri},
	{HOOK_IAT_CANDIDATE, 0, "glTexParameterf", NULL, (FARPROC *)&pglTexParameterf, (FARPROC)extglTexParameterf},
	{HOOK_IAT_CANDIDATE, 0, "glFogf", NULL, (FARPROC *)&pglFogf, (FARPROC)extglFogf},
	{HOOK_IAT_CANDIDATE, 0, "glFogi", NULL, (FARPROC *)&pglFogi, (FARPROC)extglFogi},
	{HOOK_IAT_CANDIDATE, 0, "glFogfv", NULL, (FARPROC *)&pglFogfv, (FARPROC)extglFogfv},
	{HOOK_IAT_CANDIDATE, 0, "glFogiv", NULL, (FARPROC *)&pglFogiv, (FARPROC)extglFogiv},
	{HOOK_IAT_CANDIDATE, 0, "glBegin", NULL, (FARPROC *)&pglBegin, (FARPROC)extglBegin},
	{HOOK_IAT_CANDIDATE, 0, "glEnd", NULL, (FARPROC *)&pglEnd, (FARPROC)extglEnd},
#ifdef SUPPRESSLIGHTMODETWOSIDE
	{HOOK_IAT_CANDIDATE, 0, "glLightModeli", NULL, (FARPROC *)&pglLightModeli, (FARPROC)extglLightModeli},
#endif
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "glDeleteLists", NULL, (FARPROC *)&pglDeleteLists, (FARPROC)extglDeleteLists},
	{HOOK_IAT_CANDIDATE, 0, "glArrayElement", NULL, (FARPROC *)&pglArrayElement, (FARPROC)extglArrayElement},
	{HOOK_IAT_CANDIDATE, 0, "glMaterialfv", NULL, (FARPROC *)&pglMaterialfv, (FARPROC)extglMaterialfv},
	{HOOK_IAT_CANDIDATE, 0, "glCullFace", NULL, (FARPROC *)&pglCullFace, (FARPROC)extglCullFace},
	{HOOK_IAT_CANDIDATE, 0, "glViewportArrayv", NULL, (FARPROC *)&pglViewportArrayv, (FARPROC)extglViewportArrayv},
	{HOOK_IAT_CANDIDATE, 0, "glViewportIndexedf", NULL, (FARPROC *)&pglViewportIndexedf, (FARPROC)extglViewportIndexedf},
	{HOOK_IAT_CANDIDATE, 0, "glViewportIndexedfv", NULL, (FARPROC *)&pglViewportIndexedfv, (FARPROC)extglViewportIndexedfv},
	{HOOK_IAT_CANDIDATE, 0, "glDepthFunc", NULL, (FARPROC *)&pglDepthFunc, (FARPROC)extglDepthFunc},
	{HOOK_IAT_CANDIDATE, 0, "glScissorArrayv", NULL, (FARPROC *)&pglScissorArrayv, (FARPROC)extglScissorArrayv},
	{HOOK_IAT_CANDIDATE, 0, "glScissorIndexed", NULL, (FARPROC *)&pglScissorIndexed, (FARPROC)extglScissorIndexed},
	{HOOK_IAT_CANDIDATE, 0, "glScissorIndexedf", NULL, (FARPROC *)&pglScissorIndexedf, (FARPROC)extglScissorIndexedf},
	{HOOK_IAT_CANDIDATE, 0, "glScissorIndexedfv", NULL, (FARPROC *)&pglScissorIndexedfv, (FARPROC)extglScissorIndexedfv},
	{HOOK_IAT_CANDIDATE, 0, "glScissorIndexedv", NULL, (FARPROC *)&pglScissorIndexedv, (FARPROC)extglScissorIndexedv},
	{HOOK_IAT_CANDIDATE, 0, "glScissorIndexedfv", NULL, (FARPROC *)&pglScissorIndexedfv, (FARPROC)extglScissorIndexedfv},
	{HOOK_IAT_CANDIDATE, 0, "glWindowPos2s", NULL, (FARPROC *)&pglWindowPos2s, (FARPROC)extglWindowPos2s},
	{HOOK_IAT_CANDIDATE, 0, "glWindowPos2i", NULL, (FARPROC *)&pglWindowPos2i, (FARPROC)extglWindowPos2i},
	{HOOK_IAT_CANDIDATE, 0, "glWindowPos2f", NULL, (FARPROC *)&pglWindowPos2f, (FARPROC)extglWindowPos2f},
	{HOOK_IAT_CANDIDATE, 0, "glClearColor", NULL, (FARPROC *)&pglClearColor, (FARPROC)extglClearColor},
	{HOOK_IAT_CANDIDATE, 0, "glColorMask", NULL, (FARPROC *)&pglColorMask, (FARPROC)extglColorMask},
	{HOOK_IAT_CANDIDATE, 0, "glBlendColor", NULL, (FARPROC *)&pglBlendColor, (FARPROC)extglBlendColor},
	{HOOK_IAT_CANDIDATE, 0, "glColor4f", NULL, (FARPROC *)&pglColor4f, (FARPROC)extglColor4f},
	{HOOK_IAT_CANDIDATE, 0, "glReadPixels", NULL, (FARPROC *)&pglReadPixels, (FARPROC)extglReadPixels},
	{HOOK_IAT_CANDIDATE, 0, "glReadnPixels", NULL, (FARPROC *)&pglReadnPixels, (FARPROC)extglReadnPixels},
	{HOOK_IAT_CANDIDATE, 0, "glVertex2f", NULL, (FARPROC *)&pglVertex2f, (FARPROC)extglVertex2f},
	{HOOK_IAT_CANDIDATE, 0, "glVertex3f", NULL, (FARPROC *)&pglVertex3f, (FARPROC)extglVertex3f},
	{HOOK_IAT_CANDIDATE, 0, "glFrustum", NULL, (FARPROC *)&pglFrustum, (FARPROC)extglFrustum},
	{HOOK_IAT_CANDIDATE, 0, "glDrawArrays", NULL, (FARPROC *)&pglDrawArrays, (FARPROC)extglDrawArrays},
	{HOOK_IAT_CANDIDATE, 0, "glTranslatef", NULL, (FARPROC *)&pglTranslatef, (FARPROC)extglTranslatef},
	{HOOK_IAT_CANDIDATE, 0, "glScalef", NULL, (FARPROC *)&pglScalef, (FARPROC)extglScalef},
	{HOOK_IAT_CANDIDATE, 0, "glOrtho", NULL, (FARPROC *)&pglOrtho, (FARPROC)extglOrtho},
	{HOOK_IAT_CANDIDATE, 0, "glPixelStorei", NULL, (FARPROC *)&pglPixelStorei, (FARPROC)extglPixelStorei},
	{HOOK_IAT_CANDIDATE, 0, "glGenTextures", NULL, (FARPROC *)&pglGenTextures, (FARPROC)extglGenTextures},
	{HOOK_IAT_CANDIDATE, 0, "glTexEnvi", NULL, (FARPROC *)&pglTexEnvi, (FARPROC)extglTexEnvi},
	{HOOK_IAT_CANDIDATE, 0, "glTexEnvf", NULL, (FARPROC *)&pglTexEnvf, (FARPROC)extglTexEnvf},
	{HOOK_IAT_CANDIDATE, 0, "glDeleteTextures", NULL, (FARPROC *)&pglDeleteTextures, (FARPROC)extglDeleteTextures},
	{HOOK_IAT_CANDIDATE, 0, "glMatrixMode", NULL, (FARPROC *)&pglMatrixMode, (FARPROC)extglMatrixMode},
	{HOOK_IAT_CANDIDATE, 0, "glLoadIdentity", NULL, (FARPROC *)&pglLoadIdentity, (FARPROC)extglLoadIdentity},
	{HOOK_IAT_CANDIDATE, 0, "glHint", NULL, (FARPROC *)&pglHint, (FARPROC)extglHint},
	{HOOK_IAT_CANDIDATE, 0, "glFrontFace", NULL, (FARPROC *)&pglFrontFace, (FARPROC)extglFrontFace},
	{HOOK_IAT_CANDIDATE, 0, "glEnableClientState", NULL, (FARPROC *)&pglEnableClientState, (FARPROC)extglEnableClientState},
	{HOOK_IAT_CANDIDATE, 0, "glMateriali", NULL, (FARPROC *)&pglMateriali, (FARPROC)extglMateriali},
	{HOOK_IAT_CANDIDATE, 0, "glMaterialfv", NULL, (FARPROC *)&pglMaterialfv, (FARPROC)extglMaterialfv},
	{HOOK_IAT_CANDIDATE, 0, "glLightf", NULL, (FARPROC *)&pglLightf, (FARPROC)extglLightf},
	{HOOK_IAT_CANDIDATE, 0, "glLighti", NULL, (FARPROC *)&pglLighti, (FARPROC)extglLighti},
	{HOOK_IAT_CANDIDATE, 0, "glLightfv", NULL, (FARPROC *)&pglLightfv, (FARPROC)extglLightfv},
	{HOOK_IAT_CANDIDATE, 0, "glLightiv", NULL, (FARPROC *)&pglLightiv, (FARPROC)extglLightiv},
	{HOOK_IAT_CANDIDATE, 0, "glPushMatrix", NULL, (FARPROC *)&pglPushMatrix, (FARPROC)extglPushMatrix},
	{HOOK_IAT_CANDIDATE, 0, "glRotated", NULL, (FARPROC *)&pglRotated, (FARPROC)extglRotated},
	{HOOK_IAT_CANDIDATE, 0, "glRotatef", NULL, (FARPROC *)&pglRotatef, (FARPROC)extglRotatef},
	{HOOK_IAT_CANDIDATE, 0, "glArrayElement", NULL, (FARPROC *)&pglArrayElement, (FARPROC)extglArrayElement},
	{HOOK_IAT_CANDIDATE, 0, "glRasterPos2i", NULL, (FARPROC *)&pglRasterPos2i, (FARPROC)extglRasterPos2i},
	{HOOK_IAT_CANDIDATE, 0, "glRasterPos2d", NULL, (FARPROC *)&pglRasterPos2d, (FARPROC)extglRasterPos2d},
	{HOOK_IAT_CANDIDATE, 0, "glRasterPos2f", NULL, (FARPROC *)&pglRasterPos2f, (FARPROC)extglRasterPos2f}, // impacts on glBitmap
	{HOOK_IAT_CANDIDATE, 0, "glRasterPos4fv", NULL, (FARPROC *)&pglRasterPos4fv, (FARPROC)extglRasterPos4fv},
	{HOOK_IAT_CANDIDATE, 0, "glVertex2i", NULL, (FARPROC *)&pglVertex2i, (FARPROC)extglVertex2i},
	{HOOK_IAT_CANDIDATE, 0, "glVertex3i", NULL, (FARPROC *)&pglVertex3i, (FARPROC)extglVertex3i},
	{HOOK_IAT_CANDIDATE, 0, "glListBase", NULL, (FARPROC *)&pglListBase, (FARPROC)extglListBase},
	{HOOK_IAT_CANDIDATE, 0, "glTexCoordPointer", NULL, (FARPROC *)&pglTexCoordPointer, (FARPROC)extglTexCoordPointer},
#endif
#ifdef STRETCHDRAWPIXELS
	{HOOK_IAT_CANDIDATE, 0, "glDrawPixels", NULL, (FARPROC *)&pglDrawPixels, (FARPROC)extglDrawPixels},
#endif
	//{HOOK_IAT_CANDIDATE, 0, "glTexCoord2f", NULL, (FARPROC *)&pglTexCoord2f, (FARPROC)extglTexCoord2f},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type TexHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "glCopyTexSubImage1D", NULL, (FARPROC *)&pglCopyTexSubImage1D, (FARPROC)extglCopyTexSubImage1D},
	{HOOK_IAT_CANDIDATE, 0, "glCopyTexSubImage2D", NULL, (FARPROC *)&pglCopyTexSubImage2D, (FARPROC)extglCopyTexSubImage2D},
	{HOOK_IAT_CANDIDATE, 0, "glCopyTexImage1D", NULL, (FARPROC *)&pglCopyTexImage1D, (FARPROC)extglCopyTexImage1D},
	{HOOK_IAT_CANDIDATE, 0, "glCopyTexImage2D", NULL, (FARPROC *)&pglCopyTexImage2D, (FARPROC)extglCopyTexImage2D},
	{HOOK_IAT_CANDIDATE, 0, "glCompressedTexImage2D", NULL, (FARPROC *)&pglCompressedTexImage2D, (FARPROC)extglCompressedTexImage2D},
	{HOOK_IAT_CANDIDATE, 0, "glTexCoord2f", NULL, (FARPROC *)&pglTexCoord2f, (FARPROC)extglTexCoord2f},
	{HOOK_IAT_CANDIDATE, 0, "glTexCoord2fv", NULL, (FARPROC *)&pglTexCoord2fv, (FARPROC)extglTexCoord2fv},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type WiggieHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "wglCreateContext", NULL, (FARPROC *)&pwglCreateContext, (FARPROC)extwglCreateContext},
	{HOOK_IAT_CANDIDATE, 0, "wglCreateLayerContext", NULL, (FARPROC *)&pwglCreateLayerContext, (FARPROC)extwglCreateLayerContext},
	{HOOK_IAT_CANDIDATE, 0, "wglDeleteContext", NULL, (FARPROC *)&pwglDeleteContext, (FARPROC)extwglDeleteContext},
	{HOOK_IAT_CANDIDATE, 0, "wglGetCurrentContext", NULL, (FARPROC *)&pwglGetCurrentContext, (FARPROC)extwglGetCurrentContext},
	{HOOK_IAT_CANDIDATE, 0, "wglCopyContext", NULL, (FARPROC *)&pwglCopyContext, (FARPROC)extwglCopyContext},
	{HOOK_IAT_CANDIDATE, 0, "wglMakeCurrent", NULL, (FARPROC *)&pwglMakeCurrent, (FARPROC)extwglMakeCurrent},
	{HOOK_IAT_CANDIDATE, 0, "wglGetProcAddress", NULL, (FARPROC *)&pwglGetProcAddress, (FARPROC)extwglGetProcAddress},
	{HOOK_IAT_CANDIDATE, 0, "wglGetExtensionsStringEXT", NULL, (FARPROC *)&pwglGetExtensionsStringEXT, (FARPROC)extwglGetExtensionsStringEXT},
	{HOOK_IAT_CANDIDATE, 0, "wglChoosePixelFormat", NULL, (FARPROC *)&pwglChoosePixelFormat, (FARPROC)extwglChoosePixelFormat},
	{HOOK_IAT_CANDIDATE, 0, "wglDescribePixelFormat", NULL, (FARPROC *)&pwglDescribePixelFormat, (FARPROC)extwglDescribePixelFormat},
	{HOOK_IAT_CANDIDATE, 0, "wglGetPixelFormat", NULL, (FARPROC *)&pwglGetPixelFormat, (FARPROC)extwglGetPixelFormat},
	{HOOK_IAT_CANDIDATE, 0, "wglSetPixelFormat", NULL, (FARPROC *)&pwglSetPixelFormat, (FARPROC)extwglSetPixelFormat},
	// used by @#@ "Doom 3"
	{HOOK_IAT_CANDIDATE, 0, "glCompressedTexImage2DARB", NULL, (FARPROC *)&pglCompressedTexImage2D, (FARPROC)extglCompressedTexImage2D},
#ifdef NOOPENGLARBEXTENSIONS
	{HOOK_IAT_CANDIDATE, 0, "wglGetExtensionsStringARB", NULL, (FARPROC *)&pwglGetExtensionsStringARB, (FARPROC)extwglGetExtensionsStringARB},
#endif
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

// to do:
//	glutInitDisplayMode
//  glutCreateWindow,  glutCreateSubWindow
//	glutPositionWindow,  glutReshapeWindow
//	glGetFloatv ( GL_SCISSOR_BOX - GL_VIEWPORT )

static HookEntryEx_Type GlutHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "glutFullScreen", NULL, (FARPROC *)&pglutFullScreen, (FARPROC)extglutFullScreen},
	{HOOK_IAT_CANDIDATE, 0, "glutInitWindowSize", NULL, (FARPROC *)&pglutInitWindowSize, (FARPROC)extglutInitWindowSize},
	{HOOK_IAT_CANDIDATE, 0, "glutInitWindowPosition", NULL, (FARPROC *)&pglutInitWindowPosition, (FARPROC)extglutInitWindowPosition},
	{HOOK_IAT_CANDIDATE, 0, "glutSetWindow", NULL, (FARPROC *)&pglutSetWindow, (FARPROC)extglutSetWindow},
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "glutPassiveMotionFunc", NULL, (FARPROC *)&pglutPassiveMotionFunc, (FARPROC)extglutPassiveMotionFunc},
	{HOOK_IAT_CANDIDATE, 0, "glutMotionFunc", NULL, (FARPROC *)&pglutMotionFunc, (FARPROC)extglutMotionFunc},
	{HOOK_IAT_CANDIDATE, 0, "glutSwapBuffers", NULL, (FARPROC *)&pglutSwapBuffers, (FARPROC)extglutSwapBuffers},
	{HOOK_IAT_CANDIDATE, 0, "gluBuild1DMipmaps", NULL, (FARPROC *)&pgluBuild1DMipmaps, (FARPROC)extgluBuild1DMipmaps},
	{HOOK_IAT_CANDIDATE, 0, "gluBuild2DMipmaps", NULL, (FARPROC *)&pgluBuild2DMipmaps, (FARPROC)extgluBuild2DMipmaps},
#endif // TRACEALL
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type GluHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "gluGetString", NULL, (FARPROC *)&pgluGetString, (FARPROC)extgluGetString},
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "gluOrtho2D", NULL, (FARPROC *)&pgluOrtho2D, (FARPROC)extgluOrtho2D},
#endif // TRACEALL
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

#undef _MODULE
#define _MODULE "dxwnd"

PROC Remap_wgl_ProcAddress(ApiArg, LPCSTR proc)
{
	int i;
	HookEntryEx_Type *Hook;
	if(!(dxw.dwFlags2 & HOOKOPENGL)) return NULL; 
	for(i=0; WiggieHooks[i].APIName; i++){
		Hook=&WiggieHooks[i];
		if (!strcmp(proc,Hook->APIName)){
			if (Hook->StoreAddress) { // avoid clearing function pointers
				PROC Addr = (*pwglGetProcAddress)(proc);
				if(Addr) *(Hook->StoreAddress)=Addr;
				else return NULL; // v2.05.77 - avoid clearing a good address
			}
			OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), (Hook->StoreAddress) ? *(Hook->StoreAddress) : 0);
			return Hook->HookerAddress;
		}
	}
	// v2.04.39: SDL2 game bstone (window port of "Blak Stone") loads all gl* methods through wglGetProcAddress !!
	for(i=0; Hooks[i].APIName; i++){
		Hook=&Hooks[i];
		if (!strcmp(proc,Hook->APIName)){
			if (Hook->StoreAddress) { // avoid clearing function pointers
				PROC Addr = (*pwglGetProcAddress)(proc);
				if(Addr) *(Hook->StoreAddress)=Addr;
				else return NULL; // v2.05.77 - avoid clearing a good address
			}
			OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), (Hook->StoreAddress) ? *(Hook->StoreAddress) : 0);
			return Hook->HookerAddress;
		}
	}
	// NULL -> keep the original call address
	return NULL;
}

FARPROC Remap_gl_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if(!(dxw.dwFlags2 & HOOKOPENGL)) return NULL; 
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
#ifdef TRACEALL
	if(addr=RemapLibraryEx(proc, hModule, TexHooks)) return addr;
#else
	if (dxw.dwFlags17 & COMPENSATEOGLCOPY) if(addr=RemapLibraryEx(proc, hModule, TexHooks)) return addr;
#endif
	// v2.05.83 fix: can't use RemapLibraryEx on wiggie hooks because calls can be present
	// or absent inside opencl32.dll, so in case the GetProcAddress returns 0 the hook
	// to the wrapper function can't be made and the return must be NULL
	if (dxw.dwFlags9 & HOOKWGLCONTEXT) if(addr=RemapLibraryOpt(proc, hModule, WiggieHooks)) return addr;
	if (dxw.dwFlags7 & HOOKGLUT32) if(addr=RemapLibraryEx(proc, hModule, GlutHooks)) return addr;
	// NULL -> keep the original call address
	return NULL;
}

void ForceHookOpenGL(HMODULE base) // to test .....
{
	ApiName("ForceHookOpenGL");
	HMODULE hGlLib;
	static int DoOnce=FALSE;

	if(DoOnce) return;
	DoOnce = TRUE;

	hGlLib=(*pLoadLibraryA)("OpenGL32.dll");
	OutTraceDW("%s: hGlLib=%#x\n", ApiRef, hGlLib);
	if(!hGlLib){
		OutErrorOGL("LoadLibrary(\"OpenGL32.dll\") ERROR: err=%d @%d\n", GetLastError(), __LINE__);
		return;
	}

	int i;
	HookEntryEx_Type *Hook;
	for(i=0; Hooks[i].APIName; i++){
		Hook=&Hooks[i];
		Hook->OriginalAddress = GetProcAddress(hGlLib, Hook->APIName);
		if(Hook->OriginalAddress) {
			OutDebugOGL("!!! hooking %s %#x->%#x\n", Hook->APIName, Hook->OriginalAddress, Hook->StoreAddress);
			HookAPI(base, "opengl32", Hook->StoreAddress, Hook->APIName, Hook->HookerAddress);
		}
	}

	if(dxw.dwFlags9 & HOOKWGLCONTEXT) {
		for(i=0; WiggieHooks[i].APIName; i++){
			Hook=&WiggieHooks[i];
			Hook->OriginalAddress = GetProcAddress(hGlLib, Hook->APIName);
			if(Hook->OriginalAddress) {
				OutDebugOGL("!!! hooking %s %#x->%#x\n", Hook->APIName, Hook->OriginalAddress, Hook->StoreAddress);
				HookAPI(base, "opengl32", Hook->StoreAddress, Hook->APIName, Hook->HookerAddress);
			}
		}
	}

	if(dxw.dwFlags17 & COMPENSATEOGLCOPY) {
		for(i=0; TexHooks[i].APIName; i++){
			Hook=&TexHooks[i];
			Hook->OriginalAddress = GetProcAddress(hGlLib, Hook->APIName);
			if(Hook->OriginalAddress) {
				OutDebugOGL("!!! hooking %s %#x->%#x\n", Hook->APIName, Hook->OriginalAddress, Hook->StoreAddress);
				HookAPI(base, "opengl32", Hook->StoreAddress, Hook->APIName, Hook->HookerAddress);
			}
		}
	}

	MH_EnableHook(MH_ALL_HOOKS);
}

void MakeXRayEffect()
{
	ApiName("MakeXRayEffect");
	if(!dxw.bCustomKeyToggle) return;
	if(pglDisable) (*pglDisable)(GL_DEPTH_TEST);
	if(pglEnable) (*pglEnable)(GL_BLEND);
	if(pglBlendFunc) (*pglBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	OutTraceOGL("%s: enable X-Ray tweak\n", ApiRef);
}

void HookOpenGL(HMODULE module)
{
	ApiName("HookOpenGL");
	if(!(dxw.dwFlags2 & HOOKOPENGL)) return;
	pglGetError = 0;

	//customlib = SysLibsTable[SYSLIBIDX_OPENGL].name;
	char *customlib = "OpenGL32.dll"; // v2.04.66: the .dll suffix is REQUIRED!!
	// Beware: if not defined, the char *dxw.CustomOpenGLLib is set to NULL at initialization
	if (dxw.CustomOpenGLLib) customlib = dxw.CustomOpenGLLib;

	OutTraceDW("%s: module=%#x lib=\"%s\" forced=%#x\n", 
		ApiRef, module, customlib, (dxw.dwFlags3 & FORCEHOOKOPENGL)?1:0);
	if (dxw.dwFlags3 & FORCEHOOKOPENGL) {
		ForceHookOpenGL(module);
	}
	else{
		HookLibraryEx(module, Hooks, customlib);
#ifdef TRACEALL
		HookLibraryEx(module, WiggieHooks, customlib);
		HookLibraryEx(module, TexHooks, customlib);
#else
		if(dxw.dwFlags9 & HOOKWGLCONTEXT) HookLibraryEx(module, WiggieHooks, customlib);
		if(dxw.dwFlags17 & COMPENSATEOGLCOPY) HookLibraryEx(module, TexHooks, customlib);
#endif
	}

	if(dxw.dwFlags7 & HOOKGLUT32) HookLibraryEx(module, GlutHooks, "glut32.dll");
	if(dxw.dwFlags7 & HOOKGLUT32) HookLibraryEx(module, GluHooks, "glu32.dll");

	if(dxw.dwFlags13 & XRAYTWEAK){
		if(!pglBlendFunc) {
			pglBlendFunc = (glBlendFunc_Type)(*pGetProcAddress)(module, "glBlendFunc");
			OutTraceOGL("%s: glBlendFunc=%x\n", ApiRef, pglBlendFunc);
		}
		if(!pglEnable) {
			pglEnable = (glEnable_Type)(*pGetProcAddress)(module, "glEnable");
			OutTraceOGL("%s: glEnable=%#x\n", ApiRef, pglEnable);
		}
		if(!pglDisable) {
			pglDisable = (glDisable_Type)(*pGetProcAddress)(module, "glDisable");
			OutTraceOGL("%s: glDisable=%#x\n", ApiRef, pglDisable);
		}
#if 0
		if(!pglClearColor) {
			pglClearColor = (glClearColor_Type)(*pGetProcAddress)(module, "glClearColor");
			OutTraceOGL("%s: glClearColor=%#x\n", ApiRef, pglClearColor);
		}
		if(!pglColorMask) {
			pglColorMask = (glColorMask_Type)(*pGetProcAddress)(module, "glColorMask");
			OutTraceOGL("%s: glColorMask=%#x\n", ApiRef, pglColorMask);
		}
		if(!pglCullFace) {
			pglCullFace = (glCullFace_Type)(*pGetProcAddress)(module, "glCullFace");
			OutTraceOGL("%s: glCullFace=%#x\n", ApiRef, pglCullFace);
		}
		if(!pglColor4f) {
			pglColor4f = (glColor4f_Type)(*pGetProcAddress)(module, "glColor4f");
			OutTraceOGL("%s: glColor4f=%#x\n", ApiRef, pglColor4f);
		}
		if(!pglDepthFunc) {
			pglDepthFunc = (glDepthFunc_Type)(*pGetProcAddress)(module, "glDepthFunc");
			OutTraceOGL("%s: glDepthFunc=%#x\n", ApiRef, pglDepthFunc);
		}
#endif
	}
}

#ifndef DXW_NOTRACES
static char *sglEnum(GLint t)
{
	char *p;
	switch(t){
		case GL_TEXTURE_1D: p="GL_TEXTURE_1D"; break;
		case GL_TEXTURE_2D: p="GL_TEXTURE_2D"; break;
		case GL_TEXTURE_3D: p="GL_TEXTURE_3D"; break;
		case GL_PROXY_TEXTURE_2D: p="GL_PROXY_TEXTURE_2D"; break;
		case GL_TEXTURE_1D_ARRAY: p="GL_TEXTURE_1D_ARRAY"; break;
		case GL_PROXY_TEXTURE_1D_ARRAY: p="GL_PROXY_TEXTURE_1D_ARRAY"; break;
		case GL_TEXTURE_RECTANGLE_ARB: p="GL_TEXTURE_RECTANGLE_ARB"; break;
		case GL_PROXY_TEXTURE_RECTANGLE_ARB: p="GL_PROXY_TEXTURE_RECTANGLE_ARB"; break;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X: p="GL_TEXTURE_CUBE_MAP_POSITIVE_X"; break;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: p="GL_TEXTURE_CUBE_MAP_NEGATIVE_X"; break;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: p="GL_TEXTURE_CUBE_MAP_POSITIVE_Y"; break;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: p="GL_TEXTURE_CUBE_MAP_NEGATIVE_Y"; break;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: p="GL_TEXTURE_CUBE_MAP_POSITIVE_Z"; break;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: p="GL_TEXTURE_CUBE_MAP_NEGATIVE_Z"; break;
		case GL_TEXTURE_CUBE_MAP: p="GL_TEXTURE_CUBE_MAP"; break;
		case GL_RGB4: p="GL_RGB4"; break;
		case GL_RGB5: p="GL_RGB5"; break;
		case GL_RGB8: p="GL_RGB8"; break;
		case GL_RGB10: p="GL_RGB10"; break;
		case GL_RGB12: p="GL_RGB12"; break;
		case GL_RGB16: p="GL_RGB16"; break;
		case GL_RGBA2: p="GL_RGBA2"; break;
		case GL_RGBA4: p="GL_RGBA4"; break;
		case GL_RGB5_A1: p="GL_RGB5_A1"; break;
		case GL_RGBA8: p="GL_RGBA8"; break;
		case GL_RGB10_A2: p="GL_RGB10_A2"; break;
		case GL_RGBA12: p="GL_RGBA12"; break;
		case GL_RGBA16: p="GL_RGBA16"; break;	
		case GL_TEXTURE_MIN_FILTER: p="GL_TEXTURE_MIN_FILTER"; break;
		case GL_TEXTURE_MAG_FILTER: p="GL_TEXTURE_MAG_FILTER"; break;
		case GL_TEXTURE_WRAP_S: p="GL_TEXTURE_WRAP_S"; break;
		case GL_TEXTURE_WRAP_T: p="GL_TEXTURE_WRAP_T"; break;
		case GL_TEXTURE_ENV_MODE: p="GL_TEXTURE_ENV_MODE"; break;
		case GL_ALPHA_TEST: p="GL_ALPHA_TEST"; break;
		case GL_AUTO_NORMAL: p="GL_AUTO_NORMAL"; break;
		case GL_CLIP_PLANE0: p="GL_CLIP_PLANE0"; break;
		case GL_CLIP_PLANE1: p="GL_CLIP_PLANE1"; break;
		case GL_CLIP_PLANE2: p="GL_CLIP_PLANE2"; break;
		case GL_CLIP_PLANE3: p="GL_CLIP_PLANE3"; break;
		case GL_CLIP_PLANE4: p="GL_CLIP_PLANE4"; break;
		case GL_CLIP_PLANE5: p="GL_CLIP_PLANE5"; break;
		case GL_COLOR_LOGIC_OP: p="GL_COLOR_LOGIC_OP"; break;
		case GL_COLOR_SUM: p="GL_COLOR_SUM"; break;
		case GL_COLOR_TABLE: p="GL_COLOR_TABLE"; break;
		case GL_CONVOLUTION_1D: p="GL_CONVOLUTION_1D"; break;
		case GL_CONVOLUTION_2D: p="GL_CONVOLUTION_2D"; break;
		case GL_DEPTH_TEST: p="GL_DEPTH_TEST"; break;
		case GL_DITHER: p="GL_DITHER"; break;
		case GL_HISTOGRAM: p="GL_HISTOGRAM"; break;
		case GL_INDEX_LOGIC_OP: p="GL_INDEX_LOGIC_OP"; break;
		case GL_LINE_SMOOTH: p="GL_LINE_SMOOTH"; break;
		case GL_LINE_STIPPLE: p="GL_LINE_STIPPLE"; break;
		case GL_MAP1_COLOR_4: p="GL_MAP1_COLOR_4"; break;
		case GL_MAP1_INDEX: p="GL_MAP1_INDEX"; break;
		case GL_MAP1_NORMAL: p="GL_MAP1_NORMAL"; break;
		case GL_MAP1_TEXTURE_COORD_1: p="GL_MAP1_TEXTURE_COORD_1"; break;
		case GL_MAP1_TEXTURE_COORD_2: p="GL_MAP1_TEXTURE_COORD_2"; break;
		case GL_MAP1_TEXTURE_COORD_3: p="GL_MAP1_TEXTURE_COORD_3"; break;
		case GL_MAP1_TEXTURE_COORD_4: p="GL_MAP1_TEXTURE_COORD_4"; break;
		case GL_MAP1_VERTEX_3: p="GL_MAP1_VERTEX_3"; break;
		case GL_MAP1_VERTEX_4: p="GL_MAP1_VERTEX_4"; break;
		case GL_MAP2_COLOR_4: p="GL_MAP2_COLOR_4"; break;
		case GL_MAP2_INDEX: p="GL_MAP2_INDEX"; break;
		case GL_MAP2_NORMAL: p="GL_MAP2_NORMAL"; break;
		case GL_MAP2_TEXTURE_COORD_1: p="GL_MAP2_TEXTURE_COORD_1"; break;
		case GL_MAP2_TEXTURE_COORD_2: p="GL_MAP2_TEXTURE_COORD_2"; break;
		case GL_MAP2_TEXTURE_COORD_3: p="GL_MAP2_TEXTURE_COORD_3"; break;
		case GL_MAP2_TEXTURE_COORD_4: p="GL_MAP2_TEXTURE_COORD_4"; break;
		case GL_MAP2_VERTEX_3: p="GL_MAP2_VERTEX_3"; break;
		case GL_MAP2_VERTEX_4: p="GL_MAP2_VERTEX_4"; break;
		case GL_MINMAX: p="GL_MINMAX"; break;
		case GL_MULTISAMPLE: p="GL_MULTISAMPLE"; break;
		case GL_POINT_SMOOTH: p="GL_POINT_SMOOTH"; break;
		case GL_POINT_SPRITE: p="GL_POINT_SPRITE"; break;
		case GL_POST_COLOR_MATRIX_COLOR_TABLE: p="GL_POST_COLOR_MATRIX_COLOR_TABLE"; break;
		case GL_POST_CONVOLUTION_COLOR_TABLE: p="GL_POST_CONVOLUTION_COLOR_TABLE"; break;
		case GL_RESCALE_NORMAL: p="GL_RESCALE_NORMAL"; break;
		case GL_SAMPLE_ALPHA_TO_COVERAGE: p="GL_SAMPLE_ALPHA_TO_COVERAGE"; break;
		case GL_SAMPLE_ALPHA_TO_ONE: p="GL_SAMPLE_ALPHA_TO_ONE"; break;
		case GL_SAMPLE_COVERAGE: p="GL_SAMPLE_COVERAGE"; break;
		case GL_SEPARABLE_2D: p="GL_SEPARABLE_2D"; break;
		case GL_SCISSOR_TEST: p="GL_SCISSOR_TEST"; break;
		case GL_STENCIL_TEST: p="GL_STENCIL_TEST"; break;
		case GL_TEXTURE_GEN_Q: p="GL_TEXTURE_GEN_Q"; break;
		case GL_TEXTURE_GEN_R: p="GL_TEXTURE_GEN_R"; break;
		case GL_TEXTURE_GEN_S: p="GL_TEXTURE_GEN_S"; break;
		case GL_TEXTURE_GEN_T: p="GL_TEXTURE_GEN_T"; break;
		case GL_VERTEX_PROGRAM_POINT_SIZE: p="GL_VERTEX_PROGRAM_POINT_SIZE"; break;
		case GL_VERTEX_PROGRAM_TWO_SIDE: p="GL_VERTEX_PROGRAM_TWO_SIDE"; break;	
		/* Matrix Mode */
		case GL_MATRIX_MODE: p="GL_MATRIX_MODE"; break;	
		case GL_MODELVIEW: p="GL_MODELVIEW"; break;	
		case GL_PROJECTION: p="GL_PROJECTION"; break;	
		case GL_TEXTURE: p="GL_TEXTURE"; break;	
		/* Hints */
		case GL_FOG_HINT: p="GL_FOG_HINT"; break;
		case GL_LINE_SMOOTH_HINT: p="GL_LINE_SMOOTH_HINT"; break;
		case GL_PERSPECTIVE_CORRECTION_HINT: p="GL_PERSPECTIVE_CORRECTION_HINT"; break;
		case GL_POINT_SMOOTH_HINT: p="GL_POINT_SMOOTH_HINT"; break;
		case GL_POLYGON_SMOOTH_HINT: p="GL_POLYGON_SMOOTH_HINT"; break;
		case GL_DONT_CARE: p="GL_DONT_CARE"; break;
		case GL_FASTEST: p="GL_FASTEST"; break;
		case GL_NICEST: p="GL_NICEST"; break;
		/* Polygons */
		case GL_POINT: p="GL_POINT"; break;
		case GL_LINE: p="GL_LINE"; break;
		case GL_FILL: p="GL_FILL"; break;
		case GL_CW: p="GL_CW"; break;
		case GL_CCW: p="GL_CCW"; break;
		case GL_FRONT: p="GL_FRONT"; break;
		case GL_BACK: p="GL_BACK"; break;
		case GL_POLYGON_MODE: p="GL_POLYGON_MODE"; break;
		case GL_POLYGON_SMOOTH: p="GL_POLYGON_SMOOTH"; break;
		case GL_POLYGON_STIPPLE: p="GL_POLYGON_STIPPLE"; break;
		case GL_EDGE_FLAG: p="GL_EDGE_FLAG"; break;
		case GL_CULL_FACE: p="GL_CULL_FACE"; break;
		case GL_CULL_FACE_MODE: p="GL_CULL_FACE_MODE"; break;
		case GL_FRONT_FACE: p="GL_FRONT_FACE"; break;
		case GL_POLYGON_OFFSET_FACTOR: p="GL_POLYGON_OFFSET_FACTOR"; break;
		case GL_POLYGON_OFFSET_UNITS: p="GL_POLYGON_OFFSET_UNITS"; break;
		case GL_POLYGON_OFFSET_POINT: p="GL_POLYGON_OFFSET_POINT"; break;
		case GL_POLYGON_OFFSET_LINE: p="GL_POLYGON_OFFSET_LINE"; break;
		case GL_POLYGON_OFFSET_FILL: p="GL_POLYGON_OFFSET_FILL"; break;
		/* Vertex Arrays */
		case GL_VERTEX_ARRAY: p="GL_VERTEX_ARRAY"; break;
		case GL_NORMAL_ARRAY: p="GL_NORMAL_ARRAY"; break;
		case GL_COLOR_ARRAY: p="GL_COLOR_ARRAY"; break;
		case GL_INDEX_ARRAY: p="GL_INDEX_ARRAY"; break;
		case GL_TEXTURE_COORD_ARRAY: p="GL_TEXTURE_COORD_ARRAY"; break;
		case GL_EDGE_FLAG_ARRAY: p="GL_EDGE_FLAG_ARRAY"; break;
		case GL_VERTEX_ARRAY_SIZE: p="GL_VERTEX_ARRAY_SIZE"; break;
		case GL_VERTEX_ARRAY_TYPE: p="GL_VERTEX_ARRAY_TYPE"; break;
		case GL_VERTEX_ARRAY_STRIDE: p="GL_VERTEX_ARRAY_STRIDE"; break;
		case GL_NORMAL_ARRAY_TYPE: p="GL_NORMAL_ARRAY_TYPE"; break;
		case GL_NORMAL_ARRAY_STRIDE: p="GL_NORMAL_ARRAY_STRIDE"; break;
		case GL_COLOR_ARRAY_SIZE: p="GL_COLOR_ARRAY_SIZE"; break;
		case GL_COLOR_ARRAY_TYPE: p="GL_COLOR_ARRAY_TYPE"; break;
		case GL_COLOR_ARRAY_STRIDE: p="GL_COLOR_ARRAY_STRIDE"; break;
		case GL_INDEX_ARRAY_TYPE: p="GL_INDEX_ARRAY_TYPE"; break;
		case GL_INDEX_ARRAY_STRIDE: p="GL_INDEX_ARRAY_STRIDE"; break;
		case GL_TEXTURE_COORD_ARRAY_SIZE: p="GL_TEXTURE_COORD_ARRAY_SIZE"; break;
		case GL_TEXTURE_COORD_ARRAY_TYPE: p="GL_TEXTURE_COORD_ARRAY_TYPE"; break;
		case GL_TEXTURE_COORD_ARRAY_STRIDE: p="GL_TEXTURE_COORD_ARRAY_STRIDE"; break;
		case GL_EDGE_FLAG_ARRAY_STRIDE: p="GL_EDGE_FLAG_ARRAY_STRIDE"; break;
		case GL_VERTEX_ARRAY_POINTER: p="GL_VERTEX_ARRAY_POINTER"; break;
		case GL_NORMAL_ARRAY_POINTER: p="GL_NORMAL_ARRAY_POINTER"; break;
		case GL_COLOR_ARRAY_POINTER: p="GL_COLOR_ARRAY_POINTER"; break;
		case GL_INDEX_ARRAY_POINTER: p="GL_INDEX_ARRAY_POINTER"; break;
		case GL_TEXTURE_COORD_ARRAY_POINTER: p="GL_TEXTURE_COORD_ARRAY_POINTER"; break;
		case GL_EDGE_FLAG_ARRAY_POINTER: p="GL_EDGE_FLAG_ARRAY_POINTER"; break;
		case GL_V2F: p="GL_V2F"; break;
		case GL_V3F: p="GL_V3F"; break;
		case GL_C4UB_V2F: p="GL_C4UB_V2F"; break;
		case GL_C4UB_V3F: p="GL_C4UB_V3F"; break;
		case GL_C3F_V3F: p="GL_C3F_V3F"; break;
		case GL_N3F_V3F: p="GL_N3F_V3F"; break;
		case GL_C4F_N3F_V3F: p="GL_C4F_N3F_V3F"; break;
		case GL_T2F_V3F: p="GL_T2F_V3F"; break;
		case GL_T4F_V4F: p="GL_T4F_V4F"; break;
		case GL_T2F_C4UB_V3F: p="GL_T2F_C4UB_V3F"; break;
		case GL_T2F_C3F_V3F: p="GL_T2F_C3F_V3F"; break;
		case GL_T2F_N3F_V3F: p="GL_T2F_N3F_V3F"; break;
		case GL_T2F_C4F_N3F_V3F: p="GL_T2F_C4F_N3F_V3F"; break;
		case GL_T4F_C4F_N3F_V4F: p="GL_T4F_C4F_N3F_V4F"; break;
		/* Lighting */
		case GL_LIGHTING: p="GL_LIGHTING"; break;
		case GL_LIGHT0: p="GL_LIGHT0"; break;
		case GL_LIGHT1: p="GL_LIGHT1"; break;
		case GL_LIGHT2: p="GL_LIGHT2"; break;
		case GL_LIGHT3: p="GL_LIGHT3"; break;
		case GL_LIGHT4: p="GL_LIGHT4"; break;
		case GL_LIGHT5: p="GL_LIGHT5"; break;
		case GL_LIGHT6: p="GL_LIGHT6"; break;
		case GL_LIGHT7: p="GL_LIGHT7"; break;
		case GL_SPOT_EXPONENT: p="GL_SPOT_EXPONENT"; break;
		case GL_SPOT_CUTOFF: p="GL_SPOT_CUTOFF"; break;
		case GL_CONSTANT_ATTENUATION: p="GL_CONSTANT_ATTENUATION"; break;
		case GL_LINEAR_ATTENUATION: p="GL_LINEAR_ATTENUATION"; break;
		case GL_QUADRATIC_ATTENUATION: p="GL_QUADRATIC_ATTENUATION"; break;
		case GL_AMBIENT: p="GL_AMBIENT"; break;
		case GL_DIFFUSE: p="GL_DIFFUSE"; break;
		case GL_SPECULAR: p="GL_SPECULAR"; break;
		case GL_SHININESS: p="GL_SHININESS"; break;
		case GL_EMISSION: p="GL_EMISSION"; break;
		case GL_POSITION: p="GL_POSITION"; break;
		case GL_SPOT_DIRECTION: p="GL_SPOT_DIRECTION"; break;
		case GL_AMBIENT_AND_DIFFUSE: p="GL_AMBIENT_AND_DIFFUSE"; break;
		case GL_COLOR_INDEXES: p="GL_COLOR_INDEXES"; break;
		case GL_LIGHT_MODEL_TWO_SIDE: p="GL_LIGHT_MODEL_TWO_SIDE"; break;
		case GL_LIGHT_MODEL_LOCAL_VIEWER: p="GL_LIGHT_MODEL_LOCAL_VIEWER"; break;
		case GL_LIGHT_MODEL_AMBIENT: p="GL_LIGHT_MODEL_AMBIENT"; break;
		case GL_FRONT_AND_BACK: p="GL_FRONT_AND_BACK"; break;
		case GL_SHADE_MODEL: p="GL_SHADE_MODEL"; break;
		case GL_FLAT: p="GL_FLAT"; break;
		case GL_SMOOTH: p="GL_SMOOTH"; break;
		case GL_COLOR_MATERIAL: p="GL_COLOR_MATERIAL"; break;
		case GL_COLOR_MATERIAL_FACE: p="GL_COLOR_MATERIAL_FACE"; break;
		case GL_COLOR_MATERIAL_PARAMETER: p="GL_COLOR_MATERIAL_PARAMETER"; break;
		case GL_NORMALIZE: p="GL_NORMALIZE"; break;
		/* Fog */
		case GL_FOG: p="GL_FOG"; break;
		case GL_FOG_MODE: p="GL_FOG_MODE"; break;
		case GL_FOG_DENSITY: p="GL_FOG_DENSITY"; break;
		case GL_FOG_COLOR: p="GL_FOG_COLOR"; break;
		case GL_FOG_INDEX: p="GL_FOG_INDEX"; break;
		case GL_FOG_START: p="GL_FOG_START"; break;
		case GL_FOG_END: p="GL_FOG_END"; break;
		case GL_LINEAR: p="GL_LINEAR"; break;
		case GL_EXP: p="GL_EXP"; break;
		case GL_EXP2: p="GL_EXP2"; break;
		/* Blending */
		case GL_BLEND: p="GL_BLEND"; break;
		case GL_BLEND_SRC: p="GL_BLEND_SRC"; break;
		case GL_BLEND_DST: p="GL_BLEND_DST"; break;
		case GL_ZERO: p="GL_ZERO"; break;
		case GL_ONE: p="GL_ONE"; break;
		case GL_SRC_COLOR: p="GL_SRC_COLOR"; break;
		case GL_ONE_MINUS_SRC_COLOR: p="GL_ONE_MINUS_SRC_COLOR"; break;
		case GL_SRC_ALPHA: p="GL_SRC_ALPHA"; break;
		case GL_ONE_MINUS_SRC_ALPHA: p="GL_ONE_MINUS_SRC_ALPHA"; break;
		case GL_DST_ALPHA: p="GL_DST_ALPHA"; break;
		case GL_ONE_MINUS_DST_ALPHA: p="GL_ONE_MINUS_DST_ALPHA"; break;
		case GL_DST_COLOR: p="GL_DST_COLOR"; break;
		case GL_ONE_MINUS_DST_COLOR: p="GL_ONE_MINUS_DST_COLOR"; break;
		case GL_SRC_ALPHA_SATURATE: p="GL_SRC_ALPHA_SATURATE"; break;
		case GL_CONSTANT_COLOR: p="GL_CONSTANT_COLOR"; break;
		case GL_ONE_MINUS_CONSTANT_COLOR: p="GL_ONE_MINUS_CONSTANT_COLOR"; break;
		case GL_CONSTANT_ALPHA: p="GL_CONSTANT_ALPHA"; break;
		case GL_ONE_MINUS_CONSTANT_ALPHA: p="GL_ONE_MINUS_CONSTANT_ALPHA"; break;
		// anisotropic filtering extensions
		case GL_TEXTURE_MAX_ANISOTROPY_EXT: p="GL_TEXTURE_MAX_ANISOTROPY_EXT"; break;
		case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: p="GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT"; break;

		case GL_TEXTURE_ENV: p="GL_TEXTURE_ENV"; break;
		case GL_MODULATE: p="GL_MODULATE"; break;
		case GL_DECAL: p="GL_DECAL"; break;
		case GL_REPLACE: p="GL_REPLACE"; break;
		case GL_REPEAT: p="GL_REPEAT"; break;

		/* unknown */
		default: p="unknown"; break;
	}
	return p;
}
#endif // DXW_NOTRACES

#undef _MODULE
#define _MODULE "opengl32"

GLenum WINAPI extglGetError()
{
	if (pglGetError) return (*pglGetError)();
	return GL_NO_ERROR;
}

void WINAPI extglViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	ApiName("glViewport");
	OutTraceOGL("%s: pos=(%d,%d) size=(%d,%d)\n", ApiRef, x, y, width, height);

	if(!dxw.isScaled) {
		(*pglViewport)(x, y, width, height);
		return;
	}

	BOOL bViewportBypass;
	if(!(dxw.dwFlags11 & SCALEMAINVIEWPORT)){
		bViewportBypass = TRUE;
	}
	else {
		bViewportBypass = (width == (int)dxw.GetScreenWidth()) && (height == (int)dxw.GetScreenHeight());
		OutTraceOGL("%s: BYPASS=%#x\n", ApiRef, bViewportBypass);
	}

	if(bViewportBypass) {
		if(x==CW_USEDEFAULT) x=0;
		if(y==CW_USEDEFAULT) y=0;
		// v2.04.89: casted to int type to handle negative x,y values like in SW:KOTOR 
		x = (x * (int)dxw.iSizX) / (int)dxw.GetScreenWidth();
		y = (y * (int)dxw.iSizY) / (int)dxw.GetScreenHeight();
		width = (width * (int)dxw.iSizX) / (int)dxw.GetScreenWidth();
		height = (height * (int)dxw.iSizY) / (int)dxw.GetScreenHeight();
		OutTraceOGL("%s: remapped pos=(%d,%d) size=(%d,%d)\n", ApiRef, x, y, width, height);
	}

	(*pglViewport)(x, y, width, height);
}

#ifdef TRACEALL
void WINAPI extglViewportArrayv(GLuint first, GLsizei count, const GLfloat *v)
{
	ApiName("glViewportArrayv");
	OutTraceOGL("%s: first=%d count=%d\n", ApiRef, first, count);

	(*pglViewportArrayv)(first, count, v);
}

void WINAPI extglViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h)
{
	ApiName("glViewportIndexedf");
	OutTraceOGL("%s: index=%d x=%f y=%f w=%f h=%f\n", ApiRef, index, x, y, w, h);

	(*pglViewportIndexedf)(index, x, y, w, h);
}

void WINAPI extglViewportIndexedfv(GLuint index, const GLfloat *v)
{
	ApiName("glViewportIndexedfv");
	OutTraceOGL("%s: index=%d x=%f y=%f w=%f h=%f\n", ApiRef, index, v[0], v[1], v[2], v[3]);

	(*pglViewportIndexedfv)(index, v);
}
#endif

void WINAPI extglScissor(GLint  x,  GLint  y,  GLsizei  width,  GLsizei  height)
{
	ApiName("glScissor");
	OutTraceOGL("%s: pos=(%d,%d) size=(%d,%d)\n", ApiRef, x, y, width, height);

	if(dxw.isScaled) {
		// v2.04.89: casted to int type to handle negative x,y values like in @#@ "Star Wars Knights of the Old Republic" 
		if(dxw.GetScreenWidth()){
			x = (x * (int)dxw.iSizX) / (int)dxw.GetScreenWidth();
			width = (width * (int)dxw.iSizX) / (int)dxw.GetScreenWidth();
		}
		if(dxw.GetScreenHeight()){
			y = (y * (int)dxw.iSizY) / (int)dxw.GetScreenHeight();
			height = (height * (int)dxw.iSizY) / (int)dxw.GetScreenHeight();
		}
		OutTraceOGL("%s: remapped pos=(%d,%d) size=(%d,%d)\n", ApiRef, x, y, width, height);
	}

	(*pglScissor)(x, y, width, height);

	if((dxw.dwFlags12 & LOCKGLVIEWPORT) && pglViewport){
		OutTraceOGL("%s: SYNC glViewport pos=(%d,%d) size=(%d,%d)\n", ApiRef, 0, 0, dxw.iSiz0X, dxw.iSiz0Y);
		(*pglViewport)(0, 0, dxw.iSiz0X, dxw.iSiz0Y);
	}

	if(dxw.dwFlags12 & PROJECTBUFFER) dxw.Project();
}

#ifdef TRACEALL
void WINAPI extglScissorArrayv(GLuint first, GLsizei count, const GLfloat *v)
{
	ApiName("glScissorArrayv");
	OutTraceOGL("%s: first=%d count=%d\n", ApiRef, first, count);

	(*pglScissorArrayv)(first, count, v);
}

void WINAPI extglScissorIndexed(GLuint index, GLint x, GLint y, GLsizei w, GLsizei h)
{
	ApiName("glScissorIndexedf");
	OutTraceOGL("%s: index=%d x=%d y=%d w=%d h=%d\n", ApiRef, index, x, y, w, h);

	(*pglScissorIndexed)(index, x, y, w, h);
}

void WINAPI extglScissorIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h)
{
	ApiName("glScissorIndexedf");
	OutTraceOGL("%s: index=%d x=%f y=%f w=%f h=%f\n", ApiRef, index, x, y, w, h);

	(*pglScissorIndexedf)(index, x, y, w, h);
}

void WINAPI extglScissorIndexedv(GLuint index, const GLint *v)
{
	ApiName("glScissorIndexedv");
	OutTraceOGL("%s: index=%d x=%d y=%d w=%d h=%d\n", ApiRef, index, v[0], v[1], v[2], v[3]);

	(*pglScissorIndexedv)(index, v);
}

void WINAPI extglScissorIndexedfv(GLuint index, const GLfloat *v)
{
	ApiName("glScissorIndexedfv");
	OutTraceOGL("%s: index=%d x=%f y=%f w=%f h=%f\n", ApiRef, index, v[0], v[1], v[2], v[3]);

	(*pglScissorIndexedfv)(index, v);
}
#endif

#ifdef TRACEALL
void WINAPI extglWindowPos2s(GLshort x, GLshort y)
{
	ApiName("glWindowPos2s");
	OutTraceOGL("%s: x=%d y=%d\n", ApiRef, x, y);
	(*pglWindowPos2s)(x, y);
}
 
void WINAPI extglWindowPos2i(GLint x, GLint y)
{
	ApiName("glWindowPos2i");
	OutTraceOGL("%s: x=%d y=%d\n", ApiRef, x, y);
	(*pglWindowPos2i)(x, y);
}

void WINAPI extglWindowPos2f(GLfloat x, GLfloat y)
{
	ApiName("glWindowPos2f");
	OutTraceOGL("%s: x=%f y=%f\n", ApiRef, x, y);
	(*pglWindowPos2f)(x, y);
}

/*
void WINAPI extglWindowPos2d(GLdouble x, GLdouble y)
 
void WINAPI extglWindowPos3s(GLshort x, GLshort y, GLshort z)
 
void WINAPI extglWindowPos3i(GLint x, GLint y, GLint z)
 
void WINAPI extglWindowPos3f(GLfloat x, GLfloat y, GLfloat z)
 
void WINAPI extglWindowPos3d(GLdouble x, GLdouble y, GLdouble z)
*/
#endif

void WINAPI extglGetIntegerv(GLenum pname, GLint *params) 
{
	ApiName("glGetIntegerv");
	(*pglGetIntegerv)(pname, params);
	OutDebugOGL("%s: pname=%#x\n", ApiRef, pname);

	if(!dxw.isScaled) return;

	if(pname == GL_VIEWPORT){
		BOOL bViewportBypass;
		if(!(dxw.dwFlags11 & SCALEMAINVIEWPORT)){
			bViewportBypass = TRUE;
		}
		else {
			bViewportBypass = (
				(params[0] == 0) &&
				(params[1] == 0) &&
				(params[2] == dxw.iSizX) &&
				(params[3] == dxw.iSizY));
			OutTraceOGL("%s: GL_VIEWPORT BYPASS=%#x\n", ApiRef, bViewportBypass);
		}

		if(dxw.isScaled && bViewportBypass) {
			OutTraceOGL("%s: GL_VIEWPORT pos=(%i,%i) siz=(%i,%i)\n", params[0], params[1], params[2], params[3]);
			if(dxw.iSizX && dxw.iSizY){
				// v2.04.89: casted to int type to handle negative x,y values like in SW:KOTOR 
				params[0] = (params[0] * (int)dxw.GetScreenWidth()) / (int)dxw.iSizX;
				params[1] = (params[1] * (int)dxw.GetScreenHeight()) / (int)dxw.iSizY;
				params[2] = (params[2] * (int)dxw.GetScreenWidth()) / (int)dxw.iSizX;
				params[3] = (params[3] * (int)dxw.GetScreenHeight()) / (int)dxw.iSizY;
			}
			OutTraceOGL("%s: GL_VIEWPORT FIXED pos=(%i,%i) siz=(%i,%i)\n", params[0], params[1], params[2], params[3]);
		}
	}
}

void WINAPI extglDrawBuffer(GLenum mode)
{
	ApiName("glDrawBuffer");
	OutDebugOGL("%s: mode=%#x\n", ApiRef, mode);
	if(dxw.dwFlags2 & WIREFRAME) (*pglClear)(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT) ; // clear screen for wireframe mode....
	// handle FPS only to backbuffer updates (if stereo, on left backbuffer...)
	// using the frontbuffer seems less reliable: Return to Castle Wolfenstein doesn't use it at all!
	if (dxw.dwFlags2 & HANDLEFPS){
		switch (mode){
			//case GL_FRONT_LEFT:
			case GL_BACK_LEFT:
			//case GL_FRONT:
			case GL_BACK:
			case GL_LEFT:
			case GL_FRONT_AND_BACK:
				if(dxw.HandleFPS()) return;
		}
	}
	(*pglDrawBuffer)(mode);
	dxw.ShowOverlay();
}

void WINAPI extglPolygonMode(GLenum face, GLenum mode)
{
	ApiName("glPolygonMode");
	OutTraceOGL("%s: face=%#x mode=%#x\n", ApiRef, face, mode);
	//OutTraceOGL("glPolygonMode: extglPolygonMode=%#x pglPolygonMode=%#x\n", extglPolygonMode, pglPolygonMode);
	if(dxw.dwFlags2 & WIREFRAME) {
		OutTraceOGL("%s: WIREFRAME forcind mode=GL_LINE\n", ApiRef);
		mode = GL_LINE; // trick to set wireframe mode....
	}
	(*pglPolygonMode)(face, mode);
}

void WINAPI extglGetFloatv(GLenum pname, GLboolean *params)
{
	ApiName("glGetFloatv");
	OutTraceOGL("%s: pname=%#x\n", ApiRef, pname);
	(*pglGetFloatv)(pname, params);
}

void WINAPI extglClear(GLbitfield mask)
{
	(*pglClear)(mask);
}

//BEWARE: SetPixelFormat must be issued on the same hdc used by OpenGL wglCreateContext, otherwise 
// a failure err=2000 ERROR INVALID PIXEL FORMAT occurs!!

HGLRC WINAPI extwglCreateContext(HDC hdc)
{
	ApiName("wglCreateContext");
	HGLRC ret;
	OutTraceOGL("%s: hdc=%#x\n", ApiRef, hdc);
	BOOL bRemappedDC = FALSE;

	// v2.02.31: don't let it use desktop hdc
	if(dxw.Windowize && dxw.IsRealDesktopDC(hdc)){
		HDC oldhdc = hdc;
		hdc=(*pGDIGetDC)(dxw.GethWnd());
		bRemappedDC = TRUE;
		OutTraceDW("%s: remapped desktop hdc=%#x->%#x hWnd=%#x\n", ApiRef, oldhdc, hdc, dxw.GethWnd());
	}

	// v2.04.73: FORCECLIPCHILDREN on wgl context
	if(dxw.dwFlags4 & FORCECLIPCHILDREN){
		HWND hwnd = WindowFromDC(hdc);
		LONG dwStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
		if(!(dwStyle & WS_CLIPCHILDREN)) {
#ifndef DXW_NOTRACES
			char sStyle[256];
			OutTraceDW("%s: fixed style +WS_CLIPCHILDREN hdc=%#x hwnd=%#x style=%#x(%s)\n", 
				ApiRef, hdc, hwnd, dwStyle, ExplainStyle(dwStyle, sStyle, 256));
#endif // DXW_NOTRACES
			(*pSetWindowLong)(hwnd, GWL_STYLE, dwStyle | WS_CLIPCHILDREN);
		}
	}

	// v2.05.23: added wglCreateContext in emulation mode, doesn't apply the selected pixel format
	// but adapts to the current one.
	if(dxw.IsEmulated){
		int kount = 0;
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(pfd));
		pfd.cColorBits = (byte)dxw.ActualPixelFormat.dwRGBBitCount;
		int iPixelFormat;
		do {
			kount++;
			iPixelFormat = ChoosePixelFormat(hdc, &pfd);
			OutTraceOGL("%s: pixel format=%d retry=%d\n", ApiRef, iPixelFormat, kount);
			//if(!(*pGDISetPixelFormat)(hdc, iPixelFormat, &pfd)){
			if(!SetPixelFormat(hdc, iPixelFormat, &pfd)){
				OutTraceOGL("%s: SetPixelFormat failed err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
			}
			ret=(*pwglCreateContext)(hdc);
			if(!ret && (kount > 5)){
				OutTraceOGL("%s: ERROR glCreateContext failed err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
				break;
			}
			if(ret) break;
			Sleep(20);
		} while(TRUE);
	}
	else {
		ret=(*pwglCreateContext)(hdc);
	}

	if(dxw.dwFlags13 & XRAYTWEAK) MakeXRayEffect();

#ifndef DXW_NOTRACES
	if(!ret) OutErrorOGL("%s: ERROR err=%d\n", ApiRef, GetLastError());
#endif // DXW_NOTRACES
	if(bRemappedDC) (*pGDIReleaseDC)(dxw.GethWnd(), hdc); // v2.04.97: fixed DC leakage
	return ret;
}

HGLRC WINAPI extwglCreateLayerContext(HDC hdc, int iLayerPlane)
{
	ApiName("wglCreateLayerContext");
	HGLRC ret;
	BOOL bRemappedDC = FALSE;
	OutTraceOGL("%s: hdc=%#x layer=%d\n", ApiRef, hdc, iLayerPlane);
	// v2.02.31: don't let it use desktop hdc
	if(dxw.Windowize && dxw.IsRealDesktopDC(hdc)){
		HDC oldhdc = hdc;
		hdc=(*pGDIGetDC)(dxw.GethWnd());
		bRemappedDC = TRUE;
		OutTraceDW("%s: remapped desktop hdc=%#x->%#x hWnd=%#x\n", ApiRef, oldhdc, hdc, dxw.GethWnd());
	}
	ret=(*pwglCreateLayerContext)(hdc, iLayerPlane);
#ifndef DXW_NOTRACES
	if(!ret) OutErrorOGL("%s: ERROR err=%d\n", ApiRef, GetLastError());
#endif // DXW_NOTRACES
	if(bRemappedDC) (*pGDIReleaseDC)(dxw.GethWnd(), hdc); // v2.04.97: fixed DC leakage
	return ret;
}

BOOL WINAPI extwglDeleteContext(HGLRC hglrc)
{
	ApiName("wglDeleteContext");
	BOOL ret;
	OutTraceOGL("%s: hglrc=%#x\n", ApiRef, hglrc);
	ret = (*pwglDeleteContext)(hglrc);
#ifndef DXW_NOTRACES
	if(!ret) OutErrorOGL("%s: ERROR err=%d\n", ApiRef, GetLastError());
#endif // DXW_NOTRACES
	return ret;
}

HGLRC WINAPI extwglGetCurrentContext(void)
{
	ApiName("wglGetCurrentContext");
	HGLRC ret;
	ret = (*pwglGetCurrentContext)();
	OutTraceOGL("%s: hglrc=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extwglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
	BOOL ret;
	ApiName("wglCopyContext");
	OutTraceOGL("%s: src=%#x dst=%#x mask=%#x\n", ApiRef, hglrcSrc, hglrcDst, mask);
	ret = (*pwglCopyContext)(hglrcSrc, hglrcDst, mask);
#ifndef DXW_NOTRACES
	if(!ret) OutErrorOGL("%s: ERROR err=%d\n", ApiRef, GetLastError());
#endif // DXW_NOTRACES

	return ret;
}

PROC WINAPI extwglGetProcAddress(LPCSTR proc)
{
	PROC procaddr;
	ApiName("wglGetProcAddress");

	OutTraceOGL("%s: proc=%s\n", ApiRef, proc);
	procaddr=Remap_wgl_ProcAddress(ApiRef, proc);
	if (!procaddr) procaddr=(*pwglGetProcAddress)(proc);
	return procaddr;
}

BOOL WINAPI extwglMakeCurrent(HDC hdc, HGLRC hglrc)
{
	ApiName("wglMakeCurrent");
	BOOL ret;
	BOOL bRemappedDC = FALSE;

	OutTraceOGL("%s: hdc=%#x hglrc=%#x\n", ApiRef, hdc, hglrc);

	if(dxw.isColorEmulatedMode){
		return (*pwglMakeCurrent)(hdc, hglrc);
	}

	// v2.02.31: don't let it use desktop hdc
	if(dxw.Windowize && dxw.IsRealDesktopDC(hdc)){
		HDC oldhdc = hdc;
		hdc=(*pGDIGetDC)(dxw.GethWnd());
		bRemappedDC = TRUE;
		OutTraceDW("%s: remapped desktop hdc=%#x->%#x\n", ApiRef, oldhdc, hdc);
	}
	ret=(*pwglMakeCurrent)(hdc, hglrc);
	// v2.05.23: it seems that OpenGL/wglMakeCurrent doesn't like the hdc if it has a clipping
	// region set. In this case, you can try clearing the clipper. Helps in "Sid Meier's SimGolf".
	if(!ret && (ERROR_CLIPPING_NOT_SUPPORTED == GetLastError())){
		(*pSelectClipRgn)(hdc, NULL);
		ret=(*pwglMakeCurrent)(hdc, hglrc);
	}
	if(ret){
		HWND hWnd;
		hWnd = WindowFromDC(hdc);
		DWORD dwStyle = (*pGetWindowLong)(hWnd, GWL_STYLE);
		if((hWnd != dxw.GethWnd()) && !((dwStyle & WS_CHILD))){
			OutTraceDW("%s: setting hwnd=%#x\n", ApiRef, hWnd);
			dxw.SethWnd(hWnd);

			// v2.04.28 addition: fixes "18 Wheels of Steel Across America" window positioning
			// v2.04.34: moved from wglCreateContext wrapper to here
			if (hWnd && dxw.isScaled && dxw.IsFullScreen()) {
				dxw.FixWindowFrame(hWnd);
				dxw.AdjustWindowPos(hWnd, dxw.iSizX, dxw.iSizY);
			}
			// v2.04.28, v2.04.34 end
		}

	}
	else{
		OutErrorOGL("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}

	if(bRemappedDC) (*pGDIReleaseDC)(dxw.GethWnd(), hdc); // v2.04.97: fixed DC leakage
	return ret;
}

void WINAPI extglTexImage1D(
	GLenum target, 
	GLint level, 
	GLint internalFormat, 
	GLsizei width, 
	GLint border, 
	GLenum format, 
	GLenum type, 
	const void * data)
{
	ApiName("glTexImage1D");
	OutTraceOGL("%s: TEXTURE target=%#x(%s) level=%#x internalformat=%#x format=%#x type=%#x width=%d border=%d\n", 
		ApiRef, target, sglEnum(target), level, internalFormat, format, type, width, border);

	glHandleTexture(target, internalFormat, format, width, 1, type, data);

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglTexImage1D)(target, level, internalFormat, width, border, format, type, data);
}

void WINAPI extglTexImage2D(
	GLenum target,
  	GLint level,
  	GLint internalFormat,
  	GLsizei width,
  	GLsizei height,
  	GLint border,
  	GLenum format,
  	GLenum type,
  	const GLvoid * data)
{
	ApiName("glTexImage2D");
	OutTraceOGL("%s: TEXTURE target=%#x(%s) level=%#x internalformat=%#x format=%#x type=%#x size=(%dx%d) border=%d\n", 
		ApiRef, target, sglEnum(target), level, internalFormat, format, type, width, height);

	glHandleTexture(target, internalFormat, format, width, height, type, data);

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglTexImage2D)(target, level, internalFormat, width, height, border, format, type, data);
}

void WINAPI extglCompressedTexImage2D( 	
	GLenum target,
  	GLint level,
  	GLenum internalFormat,
  	GLsizei width,
  	GLsizei height,
  	GLint border,
  	GLsizei imageSize,
  	const void * data)
{
	ApiName("glCompressedTexImage2D");
	OutTraceOGL("%s: TEXTURE target=%#x(%s) level=%#x internalformat=%#x size=(%dx%d) border=%d size=%d\n", 
		ApiRef, target, sglEnum(target), level, internalFormat, width, height, border, imageSize);

	glHandleCompressedTexture(target, internalFormat, width, height, imageSize, data);

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglCompressedTexImage2D)(target, level, internalFormat, width, height, border, imageSize, data);
}

void WINAPI extglTexImage3D(
	GLenum target,
  	GLint level,
  	GLint internalFormat,
  	GLsizei width,
  	GLsizei height,
  	GLsizei depth,
  	GLint border,
  	GLenum format,
  	GLenum type,
  	const GLvoid * data)
{
	ApiName("glTexImage3D");
	OutTraceOGL("%s: TEXTURE target=%#x(%s) level=%#x internalformat=%#x(%s) format=%#x(%s) type=%#x size=(%dx%d) depth=%d border=%d\n", 
		ApiRef, target, sglEnum(target), level, internalFormat, sglEnum(internalFormat), format, sglEnum(format), type, width, height);

	glHandleTexture(target, internalFormat, format, width, height, type, data);

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglTexImage3D)(target, level, internalFormat, width, height, depth, border, format, type, data);
}

void WINAPI extglTexSubImage1D(
	GLenum target, 
	GLint level, 
	GLint xoffset, 
	GLsizei width, 
	GLenum format, 
	GLenum type, 
	const GLvoid *pixels)
{
	ApiName("glTexSubImage1D");
	OutTraceOGL("%s: target=%#x level=%d offset=%d width=%dx format=%#x type=%#x \n", 
		ApiRef, target, level, xoffset, width, format, type);

	glHandleTexture(target, format, format, width, 1, type, pixels);

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglTexSubImage1D)(target, level, xoffset, width, format, type, pixels);
}

void WINAPI extglTexSubImage2D(
	GLenum target, 
	GLint level, 
	GLint xoffset, GLint yoffset, 
	GLsizei width, GLsizei height, 
	GLenum format, 
	GLenum type, 
	const GLvoid *pixels)
{
	ApiName("glTexSubImage2D");
	OutTraceOGL("%s: target=%#x level=%d offset=(%d,%d) size=(%dx%d) format=%#x type=%#x \n", 
		ApiRef, target, level, xoffset, yoffset, width, height, format, type);

	glHandleTexture(target, format, format, width, height, type, pixels);

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

#ifdef STRETCHDRAWPIXELS
char *ExplainDrawPixelsFormat(DWORD c)
{
	static char *eb;
	switch(c)
	{
		case GL_COLOR_INDEX: 				eb="GL_COLOR_INDEX"; break;
		case GL_STENCIL_INDEX: 				eb="GL_STENCIL_INDEX"; break;
		case GL_DEPTH_COMPONENT: 			eb="GL_DEPTH_COMPONENT"; break;
		case GL_RGB: 						eb="GL_RGB"; break;
		case GL_BGR: 						eb="GL_BGR"; break;
		case GL_RGBA: 						eb="GL_RGBA"; break;
		case GL_BGRA: 						eb="GL_BGRA"; break;
		case GL_RED: 						eb="GL_RED"; break;
		case GL_GREEN: 						eb="GL_GREEN"; break;
		case GL_BLUE: 						eb="GL_BLUE"; break;
		case GL_ALPHA: 						eb="GL_ALPHA"; break;
		case GL_LUMINANCE: 					eb="GL_LUMINANCE"; break;
		case GL_LUMINANCE_ALPHA: 			eb="GL_LUMINANCE_ALPHA"; break;
		default: 							eb="unknown"; break;
	}
	return eb;
}

// v2.04.28: Scaling in glDrawPixels is useful to stretch the "Crazy Marble" initial splash screen
void WINAPI extglDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data)
{
	ApiName("glDrawPixels");
	OutTraceOGL("%s: size=(%dx%d) format=%#x(%s) type=%d data=%#x\n", 
		ApiRef, width, height, format, ExplainDrawPixelsFormat(format), type, data);

	if(dxw.dwFlags6 & FIXPIXELZOOM){
		GLfloat xfactor, yfactor;		
		RECT desktop;
		(*pGetClientRect)(dxw.GethWnd(), &desktop);
		xfactor = (GLfloat)desktop.right / (GLfloat)dxw.GetScreenWidth();
		yfactor = (GLfloat)desktop.bottom / (GLfloat)dxw.GetScreenHeight();
		if(!pglPixelZoom){
			HMODULE hGlLib;
			hGlLib=(*pLoadLibraryA)("OpenGL32.dll"); // Beware: not compatible with custom OpenGL specfication....
			pglPixelZoom = (glPixelZoom_Type)(*GetProcAddress)(hGlLib, "glPixelZoom"); 
		}
		if (pglPixelZoom) (*pglPixelZoom)(xfactor, yfactor);
		OutTraceDW("%s: glPixelZoom FIXED x,y factor=(%f,%f)\n", ApiRef, xfactor, yfactor);
	}
	// clear error code
	extglGetError();
	(*pglDrawPixels)(width, height, format, type, data);
}
#endif

void WINAPI extglPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
	ApiName("glPixelZoom");
	OutTraceOGL("%s: x,y factor=(%f,%f)\n", ApiRef, xfactor, yfactor);

	if(dxw.dwFlags6 & FIXPIXELZOOM){
		RECT desktop;
		(*pGetClientRect)(dxw.GethWnd(), &desktop);
		xfactor = (xfactor * desktop.right) / dxw.GetScreenWidth();
		yfactor = (yfactor * desktop.bottom) / dxw.GetScreenHeight();
		OutTraceOGL("%s: FIXED x,y factor=(%f,%f)\n", ApiRef, xfactor, yfactor);
	}
	(*pglPixelZoom)(xfactor, yfactor);
}

#ifndef DXW_NOTRACES
static char *sglmode(GLenum m)
{
	char *labels[]={
		"POINTS", "LINES", "LINE_LOOP", "LINE_STRIP",
		"TRIANGLES", "TRIANGLE_STRIP", "TRIANGLE_FAN", "QUADS",
		"QUAD_STRIP", "POLYGON", "unknown"};
	if(m > GL_POLYGON) m=GL_POLYGON+1;
	return labels[m];
}
#endif //DXW_NOTRACES

void WINAPI extglBegin(GLenum mode)
{
	// handling of glErr causing exceptions in "Tha Banished", needs a better fix
	ApiName("glBegin");
	OutTraceOGL("%s: mode=%#x(%s)\n", ApiRef, mode, sglmode(mode));
	(*pglBegin)(mode);
}

void WINAPI extglEnd(void)
{
	ApiName("glEnd");
	OutTraceOGL("%s\n", ApiRef);
	gbScreenOverlap = FALSE;
	(*pglEnd)();
}

void WINAPI extglBindTexture(GLenum target, GLuint texture)
{
	ApiName("glBindTexture");
	OutTraceOGL("%s: target=%#x(%s) texture=%#x\n", ApiRef, target, sglEnum(target), texture);

	if(dxw.dwFlags7 & FIXBINDTEXTURE) {
		static GLuint uiLastTex = 0;
		if(uiLastTex) (*pglBindTexture)(target, 0);
		uiLastTex = texture;
	}

	extglGetError(); // clears the error code
	(*pglBindTexture)(target, texture);
}

#ifdef TRACEALL
void WINAPI extglPixelStorei(GLenum pname,  GLint param)
{
	ApiName("glPixelStorei");
	OutTraceOGL("%s: pname=%#x param=%#x\n", ApiRef, pname, param);
	(*pglPixelStorei)(pname, param);
}
#endif

void WINAPI extglutFullScreen(void)
{
	ApiName("glutFullScreen");
	OutTraceOGL("%s\n", ApiRef);
	if(!dxw.isScaled) return (*pglutFullScreen)();
	OutTraceDW("%s: BYPASS\n", ApiRef);
	dxw.SetFullScreen(TRUE);
}

void extglutInitWindowSize(int width, int height)
{
	ApiName("glutInitWindowSize");
	int dummy1, dummy2;
	OutTraceOGL("%s: size=(%dx%d)\n", ApiRef, width, height);
	if(dxw.isScaled){
		dummy1=0;
		dummy2=0;
		dxw.MapWindow(&dummy1, &dummy2, &width, &height);	
		OutTraceDW("%s: FIXED width=%d height=%d\n", ApiRef, width, height);
	}
	(*pglutInitWindowSize)(width, height);
}

void extglutInitWindowPosition(int x, int y)
{
	ApiName("glutInitWindowPosition");
	int dummy1, dummy2;
	OutTraceOGL("%s: pos=(%d,%d)\n", ApiRef, x, y);
	if(dxw.isScaled){
		dummy1=0;
		dummy2=0;
		dxw.MapWindow(&x, &y, &dummy1, &dummy2);
		OutTraceDW("%s: FIXED pos=(%d,%d)\n", ApiRef, x, y);
	}
	(*pglutInitWindowPosition)(x, y);
}

void WINAPI extglutSetWindow(HWND win)
{
	ApiName("glutSetWindow");
	OutTraceOGL("%s: win=%#x\n", ApiRef, win);
	if(dxw.isScaled && dxw.IsRealDesktop(win)) win=dxw.GethWnd();
	(*pglutSetWindow)(win);
}

static char *glStringName(GLenum name)
{
	char *ret;
	switch(name){
		case GL_VENDOR: ret="GL_VENDOR"; break;
		case GL_RENDERER: ret="GL_RENDERER"; break;
		case GL_VERSION: ret="GL_VERSION"; break;
		case GL_SHADING_LANGUAGE_VERSION: ret="GL_SHADING_LANGUAGE_VERSION"; break;
		case GL_EXTENSIONS: ret="GL_EXTENSIONS"; break;
		default: ret="unknown"; break;
	}
	return ret;
}

const  GLubyte* WINAPI extglGetString(GLenum name)
{
	const GLubyte* ret;
	ApiName("glGetString");

	OutTraceOGL("%s: name=%#x(%s)\n", ApiRef, name, glStringName(name));
	ret = (*pglGetString)(name);
#ifndef DXW_NOTRACES
	if(IsDebugOGL){
		if(strlen((const char *)ret)<80)
			OutTrace("%s: name=%#x(%s) ret=\"%.80s\"\n", ApiRef, name, glStringName(name), ret);
		else{
			const GLubyte *p = ret;
			OutTrace("%s: name=%#x(%s) ret=(%d)\n", ApiRef, name, glStringName(name), strlen((const char *)ret));
			while(strlen((const char *)p)>80){
				OutTrace("%s: \"%.80s\" +\n", ApiRef, p);
				p += 80;
			}
			OutTrace("%s: \"%.80s\"\n", ApiRef, p);
		}
	}
#endif // DXW_NOTRACES

	if((name == GL_EXTENSIONS) && (dxw.dwFlags12 & GLEXTENSIONSLIE)) {
		// returns a dummy, unknown extension
		OutTraceOGL("%s: returning GL_EXT_LIE\n", ApiRef);
		return (GLubyte *)"GL_EXT_LIE"; 
	}
	if((name == GL_EXTENSIONS) && (dxw.dwFlags13 & GLEXTENSIONSTRIM)) {
		// returns a trimmed extension string
		if(strlen((char *)ret)>MAXEXTLEN){
			if(dxw.trimmedGLExt == NULL){
				dxw.trimmedGLExt = (char *)malloc(MAXEXTLEN+1);
				strncpy(dxw.trimmedGLExt, (char *)ret, MAXEXTLEN);
				dxw.trimmedGLExt[MAXEXTLEN]=0;
				for(int i=MAXEXTLEN; i>0; i--) {
					if(dxw.trimmedGLExt[i]==' ') {
						dxw.trimmedGLExt[i]=0;
						break;
					}
				}
			}
			ret = (GLubyte *)dxw.trimmedGLExt;
			OutTraceOGL("%s: returning trimmed extensions %s\n", ApiRef, ret);
		}
	}
	return ret;
}

char *WINAPI extwglGetExtensionsStringEXT(void)
{
	char *ret;
	ApiName("wglGetExtensionsStringEXT"); 

	OutTraceOGL("%s: void\n", ApiRef);
	ret = (*pwglGetExtensionsStringEXT)();
#ifndef DXW_NOTRACES
	if(IsDebugOGL){
		if(strlen((const char *)ret)<80)
			OutTrace("%s: ret=\"%.80s\"\n", ApiRef, ret);
		else{
			const char *p = ret;
			OutTrace("%s: ret=(%d)\n", ApiRef, strlen((const char *)ret));
			while(strlen((const char *)p)>80){
				OutTrace("%s: \"%.80s\" +\n", ApiRef, p);
				p += 80;
			}
			OutTrace("%s: \"%.80s\"\n", ApiRef, p);
		}
	}
#endif // DXW_NOTRACES

	if(dxw.dwFlags12 & GLEXTENSIONSLIE) {
		// returns a dummy, unknown extension
		OutTraceOGL("%s: returning GL_EXT_LIE\n", ApiRef);
		return "GL_EXT_LIE"; 
	}

	if(dxw.dwFlags13 & GLEXTENSIONSTRIM) {
		// returns a trimmed extension string
		if(strlen((char *)ret)>MAXEXTLEN){
			if(dxw.trimmedGLExt == NULL){
				dxw.trimmedGLExt = (char *)malloc(MAXEXTLEN+1);
				strncpy(dxw.trimmedGLExt, (char *)ret, MAXEXTLEN);
				dxw.trimmedGLExt[MAXEXTLEN]=0;
				for(int i=MAXEXTLEN; i>0; i--) {
					if(dxw.trimmedGLExt[i]==' ') {
						dxw.trimmedGLExt[i]=0;
						break;
					}
				}
			}
			ret = dxw.trimmedGLExt;
			OutTraceOGL("%s: returning trimmed extensions %s\n", ApiRef, ret);
		}
	}

	return ret;
}

#ifdef NOOPENGLARBEXTENSIONS
char *WINAPI extwglGetExtensionsStringARB(DWORD hdc)
{
	char *ret;
	ApiName("wglGetExtensionsStringARB");
	return "GL_ARB_LIE"; 
}
#endif

const GLubyte* WINAPI extgluGetString(GLenum name)
{
	const GLubyte* ret;
	ApiName("gluGetString");

	OutTraceOGL("%s: name=%#x(%s)\n", ApiRef, name, glStringName(name));
	ret = (*pgluGetString)(name);
#ifndef DXW_NOTRACES
	if(IsDebugOGL){
		if(strlen((const char *)ret)<80)
			OutTrace("%s: name=%#x(%s) ret=\"%.80s\"\n", ApiRef, name, glStringName(name), ret);
		else{
			const GLubyte *p = ret;
			OutTrace("%s: name=%#x(%s) ret=(%d)\n", ApiRef, name, glStringName(name), strlen((const char *)ret));
			while(strlen((const char *)p)>80){
				OutTrace("%s: \"%.80s\" +\n", ApiRef, p);
				p += 80;
			}
			OutTrace("%s: \"%.80s\"\n", ApiRef, p);
		}
	}
#endif // DXW_NOTRACES

	if((name == GL_EXTENSIONS) && (dxw.dwFlags12 & GLEXTENSIONSLIE)) {
		// returns a dummy, unknown extension
		OutTraceOGL("%s: returning GL_EXT_LIE\n", ApiRef);
		return (GLubyte *)"GL_EXT_LIE"; 
	}
	return ret;
}

#ifdef TRACEALL
void WINAPI extgluOrtho2D(GLdouble left, GLdouble right, GLdouble top, GLdouble bottom)
{
	ApiName("gluOrtho2D");
	OutTraceOGL("%s: rect=(%f,%f)-(%f,%f)\n", ApiRef, left, right, top, bottom);
	(*pgluOrtho2D)(left, right, top, bottom);
}

void WINAPI extgluBuild2DMipmaps(GLenum target, GLint intFormat, GLsizei w, GLsizei h, GLenum format, GLenum type, const void *data)
{
	ApiName("gluBuild2DMipmaps");
	OutTraceOGL("%s: size=%dx%d\n", ApiRef, w, h);
	(*pgluBuild2DMipmaps)(target, intFormat, w, h, format, type, data);
}

void WINAPI extgluBuild1DMipmaps(GLenum target, GLint components, GLsizei width, GLenum format, GLenum type, const void   *data)
{
	ApiName("gluBuild1DMipmaps");
	OutTraceOGL("%s: target=%d components=%d width=%d format=%d type=%d data=%#x\n", 
		ApiRef, target, components, width, format, type, data);
	(*pgluBuild1DMipmaps)(target, components, width, format, type, data);
}

void WINAPI extglRasterPos4fv(const GLfloat *v)
{
	ApiName("glRasterPos4fv");
	OutTraceOGL("%s: x=%f y=%f z=%f w=%f\n", ApiRef, v[0], v[1], v[2], v[3]);
	//v2.05.29 fix: no scaling - the coordinates are in window logical values!!
	//dxw.UnmapClient(&w[0], &w[1]);
	(*pglRasterPos4fv)((const GLfloat *)v);
}

void WINAPI extglRasterPos2i(GLint x, GLint y)
{
	ApiName("glRasterPos2i");
	OutTraceOGL("%s: x=%d y=%d\n", ApiRef, x, y);
	//v2.05.29 fix: no scaling - the coordinates are in window logical values!!
	//dxw.UnmapClient(&x, &y);
	(*pglRasterPos2i)(x, y);
}

void WINAPI extglRasterPos2d(GLdouble x, GLdouble y)
{
	ApiName("glRasterPos2d");
	OutTraceOGL("%s: x=%f y=%f\n", ApiRef, (float)x, (float)y);
	//v2.05.29 fix: no scaling - the coordinates are in window logical values!!
	//dxw.UnmapClient(&x, &y);
	(*pglRasterPos2d)(x, y);
}

void WINAPI extglRasterPos2f(GLfloat x, GLfloat y)
{
	ApiName("glRasterPos2f");
	OutTraceOGL("%s: x=%f y=%f\n", ApiRef, x, y);
	//v2.05.29 fix: no scaling - the coordinates are in window logical values!!
	//dxw.UnmapClient(&x, &y);
	(*pglRasterPos2f)(x, y);
}
#endif // TRACEALL

// for glBitmap, see http://www.dei.isep.ipp.pt/~matos/cg/docs/OpenGL_PG/ch09.html
// from http://www.dei.isep.ipp.pt/~matos/cg/docs/OpenGL_PG/index.html

#ifdef STRETCHBITMAPS

// undocumented, but true: MS glBitmaps are padded to DWORD boundaries!
#define padlen(x) (((x+31)/32) * 4)

static int getbit(BYTE *map, int y, int x, int h, int w)
{
	BYTE mask = 0x1 << (7-(x%8));
	BYTE byte;
	int linelen = padlen(w);
	byte = map[((h-(y+1))*linelen)+(x/8)];
	return (byte & mask);
}

static void putbit(BYTE *map, int y, int x, int h, int w, int val)
{
	BYTE mask = 0x1 << (7-(x%8));
	BYTE *byte;
	int linelen = padlen(w);
	byte = &map[((h-(y+1))*linelen)+(x/8)];
	if(val)
		*byte |= mask;
	// following lines useless if the array is initialized to 0s
	//else
	//	*byte &= ~mask; 
}

void resample(GLubyte *bitmap, GLubyte *scaledmap, int oldw, int oldh, int neww, int newh)
{
	OutTraceDW("> resample: scaling (%dx%d)->(%dx%d)\n", oldw, oldh, neww,  newh);
#ifdef SCALEDUMP
	for(int y=0; y<oldh; y++){
		for(int x=0; x<oldw; x++){
			OutTrace("%c",getbit((BYTE *)bitmap, y, x, oldh, oldw)?'X':'_');
		}
		OutTrace("\n");
	}
#endif

	double xscale = (neww+0.0) / oldw;
	double yscale = (newh+0.0) / oldh;
	double threshold = 0.5 / (xscale * yscale);
	double yend = 0.0;
	for (int f = 0; f < newh; f++) // y on output
	{
		double ystart = yend;
		yend = (f + 1) / yscale;
		if (yend >= oldh) yend = oldh - 0.000001;
		double xend = 0.0;
		for (int g = 0; g < neww; g++) // x on output
		{
			double xstart = xend;
			xend = (g + 1) / xscale;
			if (xend >= oldw) xend = oldw - 0.000001;
			double sum = 0.0;
			for (int y = (int)ystart; y <= (int)yend; ++y)
			{
				double yportion = 1.0;
				if (y == (int)ystart) yportion -= ystart - y;
				if (y == (int)yend) yportion -= y+1 - yend;
				for (int x = (int)xstart; x <= (int)xend; ++x)
				{
					double xportion = 1.0;
					if (x == (int)xstart) xportion -= xstart - x;
					if (x == (int)xend) xportion -= x+1 - xend;
					sum += (double)getbit((BYTE *)bitmap, y, x, oldh, oldw) * yportion * xportion;
				}
			}
			//scaledmap[f][g] = (sum > threshold) ? 1 : 0;
			putbit((BYTE *)scaledmap, f, g, newh, neww, (sum > threshold) ? 1 : 0);
		}
	}
#ifdef SCALEDUMP
	for(int y=0; y<newh; y++){
		for(int x=0; x<neww; x++){
			OutTrace("%c",getbit((BYTE *)scaledmap, y, x, newh, neww)?'X':'_');
		}
		OutTrace("\n");
	}
#endif
}
#endif

void WINAPI extglBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	ApiName("glBitmap");
	OutTraceOGL("%s: size=(%dx%d) orig=(%f,%f) move=(%f,%f)\n", ApiRef, width, height, xorig, yorig, xmove, ymove);
#ifdef STRETCHBITMAPS
	if(dxw.dwFlags9 & SCALEGLBITMAPS){
		int scaledw, scaledh, bufsize;
		GLubyte *scaledmap;
		scaledw=width;
		scaledh=height;
		dxw.MapClient(&scaledw, &scaledh);
		dxw.MapClient(&xorig, &yorig);
		dxw.MapClient(&xmove, &ymove);
		bufsize = padlen(scaledw) * scaledh;
		scaledmap=(GLubyte *)malloc(bufsize); 
		memset(scaledmap, 0, bufsize);
		resample((GLubyte *)bitmap, (GLubyte *)scaledmap, width, height, scaledw, scaledh);
		(*pglBitmap)(scaledw, scaledh, xorig, yorig, xmove, ymove, scaledmap);
		free(scaledmap);
	}
	else {
		(*pglBitmap)(width, height, xorig, yorig, xmove, ymove, bitmap);
	}
#else
	(*pglBitmap)(width, height, xorig, yorig, xmove, ymove, bitmap);
#endif
}

void WINAPI extglTexParameteri(GLenum target, GLenum pname, GLint param)
{
	ApiName("glTexParameteri");
	OutTraceOGL("%s: target=%#x(%s) pname=%#x(%s) param=%#x(%s)\n", 
		ApiRef, 
		target, sglEnum(target), 
		pname, sglEnum(pname), 
		param, sglEnum(param));

	if((dxw.dwFlags13 & GLFIXCLAMP) && (target == GL_TEXTURE_2D)) {
		if ((pname == GL_TEXTURE_WRAP_T) || (pname == GL_TEXTURE_WRAP_S)) {
			if(param == GL_CLAMP) {
				OutTraceOGL("%s: forced GL_CLAMP_TO_EDGE\n", ApiRef);
				param = GL_CLAMP_TO_EDGE;
			}
		}
	}

	(*pglTexParameteri)(target, pname, param);
}

void WINAPI extglTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	ApiName("glTexParameterf");
	OutTraceOGL("%s: target=%#x(%s) pname=%#x(%s) param=%f\n", ApiRef, target, sglEnum(target), pname, sglEnum(pname), param);

	if((dxw.dwFlags13 & GLFIXCLAMP) && (target == GL_TEXTURE_2D)) {
		if ((pname == GL_TEXTURE_WRAP_T) || (pname == GL_TEXTURE_WRAP_S)) {
			if(param == (float)GL_CLAMP) {
				OutTraceOGL("%s: forced GL_CLAMP_TO_EDGE\n", ApiRef);
				param = (float)GL_CLAMP_TO_EDGE;
			}
		}
	}

	(*pglTexParameterf)(target, pname, param);
}

#ifdef TRACEALL

void WINAPI extglVertex2f(GLfloat x, GLfloat y)
{
	ApiName("glVertex2f");
	OutDebugOGL("%s: x=%f y=%f\n", ApiRef, x, y);
	(*pglVertex2f)(x, y);
}
void WINAPI extglVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	ApiName("glVertex3f");
	OutDebugOGL("%s: x=%f y=%f z=%f\n", ApiRef, x, y, z);
	(*pglVertex3f)(x, y, z);
}

void WINAPI extglFrustum(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal)
{
	OutTraceOGL("glFrustum: rect=(%f,%f)-(%f,%f) near=%f far=%f\n", 
		(float)left, (float)top, (float)right, (float)bottom, (float)nearVal, (float)farVal);
	(*pglFrustum)(left, right, bottom, top, nearVal,farVal);
}

void WINAPI extglDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	OutTraceOGL("glDrawArrays: mode=%#x first=%d count=%d\n", mode, first, count);
	(*pglDrawArrays)(mode, first, count);
}

void WINAPI extglTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	OutTraceOGL("glTranslatef: xyz=(%f:%f:%f)\n", x, y, z);
	(*pglTranslatef)(x, y, z);
}

void WINAPI extglScalef(GLfloat x, GLfloat y, GLfloat z)
{
	OutTraceOGL("glScalef: xyz=(%f:%f:%f)\n", x, y, z);
	(*pglScalef)(x, y, z);
}

void WINAPI extglOrtho(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal)
{
	OutTraceOGL("glOrtho: rect=(%f,%f)-(%f,%f) near=%f far=%f\n", left, right, top, bottom, nearVal, farVal);
	(*pglOrtho)(left, right, bottom, top, nearVal, farVal);
}

void WINAPI extglGenTextures(GLsizei n, GLuint *textures)
{
	OutTraceOGL("glGenTextures: n=%d tex=%#x\n", n, textures);
	(*pglGenTextures)(n, textures);
}

void WINAPI extglTexEnvi(GLenum target, GLenum pname, GLint param)
{
	OutTraceOGL("glTexEnvi: target=%#x(%s) pname=%#x(%s) param=%#x(%s)\n", 
		target, sglEnum(target), 
		pname, sglEnum(pname), 
		param, sglEnum(param));
	(*pglTexEnvi)(target, pname, param);
}

void WINAPI extglTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	OutTraceOGL("glTexEnvf: target=%#x(%s) pname=%#x(%s) param=%f(%s)\n", 
		target, sglEnum(target), 
		pname, sglEnum(pname), 
		param, sglEnum((GLint)param));
	(*pglTexEnvf)(target, pname, param);
}

void WINAPI extglDeleteTextures(GLsizei n, const GLuint *textures)
{
	OutTraceOGL("glDeleteTextures: n=%d tex=%#x\n", n, textures);
	(*pglDeleteTextures)(n, textures);
}

void WINAPI extglMatrixMode(GLenum mode)
{
	OutTraceOGL("glMatrixMode: mode=%#x(%s)\n", mode, sglEnum(mode));
	(*pglMatrixMode)(mode);
}

void WINAPI extglLoadIdentity(void)
{
	OutTraceOGL("glLoadIdentity:\n");
	(*pglLoadIdentity)();
}

void WINAPI extglutSwapBuffers()
{
	OutTraceOGL("glutSwapBuffers:\n");
	(*pglutSwapBuffers)();
}

void WINAPI extglHint(GLenum target, GLenum mode)
{
	OutTraceOGL("glHint: target=%#x(%s) mode=%#x(%s)\n", target, sglEnum(target), mode, sglEnum(mode));
	(*pglHint)(target, mode);
}

void WINAPI extglFrontFace(GLenum mode)
{
	OutTraceOGL("glFrontFace: mode=%#x(%s)\n", mode, sglEnum(mode));
	(*pglFrontFace)(mode);
}

void WINAPI extglEnableClientState(GLenum cap)
{
	OutTraceOGL("glEnableClientState: cap=%#x(%s)\n", cap, sglEnum(cap));
	(*pglEnableClientState)(cap);
}

void WINAPI extglMateriali(GLenum face, GLenum pname, GLint param)
{
	OutTraceOGL("glMateriali: face=%#x(%s) pname=%#x(%s) param=%d\n", 
		face, sglEnum(face), pname, sglEnum(pname), param);
	(*pglMateriali)(face, pname, param);
}

void WINAPI extglMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	OutTraceOGL("glMaterialfv: face=%#x(%s) pname=%#x(%s)\n", 
		face, sglEnum(face), pname, sglEnum(pname));
	(*pglMaterialfv)(face, pname, params);
}

void WINAPI extglLightf(GLenum light, GLenum pname, GLfloat param)
{
	OutTraceOGL("glLightf: light=%#x(%s) pname=%#x(%s) param=%f\n", 
		light, sglEnum(light), pname, sglEnum(pname), param);
	(*pglLightf)(light, pname, param);
}

void WINAPI extglLighti(GLenum light, GLenum pname, GLint param)
{
	OutTraceOGL("glLighti: light=%#x(%s) pname=%#x(%s) param=%d\n", 
		light, sglEnum(light), pname, sglEnum(pname), param);
	(*pglLighti)(light, pname, param);
}

void WINAPI extglLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	OutTraceOGL("glLightfv: light=%#x(%s) pname=%#x(%s)\n", 
		light, sglEnum(light), pname, sglEnum(pname));
	(*pglLightfv)(light, pname, params);
}

void WINAPI extglLightiv(GLenum light, GLenum pname, const GLint *params)
{
	OutTraceOGL("glLightiv: light=%#x(%s) pname=%#x(%s)\n", 
		light, sglEnum(light), pname, sglEnum(pname));
	(*pglLightiv)(light, pname, params);
}

void WINAPI extglPushMatrix()
{
	OutTraceOGL("glPushMatrix:\n");
	(*pglPushMatrix)();
}

void WINAPI extglRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	OutTraceOGL("glRotated: angle=%f x=%f y=%f z=%f\n", angle, x, y, z);
	(*pglRotated)(angle, x, y, z);
}

void WINAPI extglRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	OutTraceOGL("glRotatef: angle=%f x=%f y=%f z=%f\n", angle, x, y, z);
	(*pglRotatef)(angle, x, y, z);
}

void WINAPI extglArrayElement(GLint i)
{
	OutTraceOGL("glArrayElement: i=%d\n", i);
	(*pglArrayElement)(i);
}

void WINAPI extglCullFace(GLenum mode)
{
	OutTraceOGL("glCullFace: mode=%#x(%s)\n", mode, sglEnum(mode));

	(*pglCullFace)(mode);
}

void WINAPI extglClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	ApiName("glClearColor");
	OutTraceOGL("%s: red=%f green=%f blue=%f alpha=%f\n", ApiRef, red, green, blue, alpha);

	(*pglClearColor)(red, green, blue, alpha);
}

void WINAPI extglColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	ApiName("glColorMask");
	OutTraceOGL("%s: red=%#x green=%#x blue=%#x alpha=%#x\n", ApiRef, red, green, blue, alpha);

	(*pglColorMask)(red, green, blue, alpha);
}

void WINAPI extglDepthFunc(GLenum func)
{
	OutTraceOGL("glDepthFunc: func=%#x(%s)\n", func, sglEnum(func));

	(*pglDepthFunc)(func);
}
#endif // TRACEALL

void WINAPI extglBlendFunc(GLenum sfactor, GLenum dfactor)
{
	ApiName("glBlendFunc");
	OutTraceOGL("%s: sfactor=%#x(%s) dfactor=%#x(%s)\n", ApiRef, sfactor, sglEnum(sfactor), dfactor, sglEnum(dfactor));

	(*pglBlendFunc)(sfactor, dfactor);
	if(dxw.dwFlags13 & XRAYTWEAK) MakeXRayEffect();
}

void WINAPI extglBlendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
	ApiName("glBlendFunci");
	OutTraceOGL("%s: buf=%d sfactor=%#x(%s) dfactor=%#x(%s)\n", ApiRef, buf, sfactor, sglEnum(sfactor), dfactor, sglEnum(dfactor));

	(*pglBlendFunci)(buf, sfactor, dfactor);
	if(dxw.dwFlags13 & XRAYTWEAK) MakeXRayEffect();
}

void WINAPI extglEnable(GLenum cap)
{
	ApiName("glEnable");
	OutTraceOGL("%s: cap=%#x(%s)\n", ApiRef, cap, sglEnum(cap));

	if((dxw.dwFlags14 & SAMPLE2COVERAGE) && (cap == GL_ALPHA_TEST)){
		OutTraceOGL("%s: force GL_SAMPLE_ALPHA_TO_COVERAGE\n", ApiRef);
		cap = GL_SAMPLE_ALPHA_TO_COVERAGE;
	}

	if((dxw.dwFlags4 & DISABLEFOGGING) && (cap == GL_FOG)){
		OutTraceOGL("%s: skip enable GL_FOG\n", ApiRef);
		return;
	}

	if((dxw.dwFlags18 & NOSCISSORTEST) && (cap == GL_SCISSOR_TEST)){
		OutTraceOGL("%s: skip enable GL_SCISSOR_TEST\n", ApiRef);
		return;
	}

	(*pglEnable)(cap);
	if(dxw.dwFlags13 & XRAYTWEAK) MakeXRayEffect();
}


void WINAPI extglDisable(GLenum cap)
{
	ApiName("glDisable");
	OutTraceOGL("%s: cap=%#x(%s)\n", ApiRef, cap, sglEnum(cap));
	if((dxw.dwFlags14 & SAMPLE2COVERAGE) && (cap == GL_ALPHA_TEST)){
		OutTraceOGL("glEnable: disable GL_SAMPLE_ALPHA_TO_COVERAGE\n");
		cap = GL_SAMPLE_ALPHA_TO_COVERAGE;
	}
	(*pglDisable)(cap);
	if(dxw.dwFlags13 & XRAYTWEAK) MakeXRayEffect();
}

void WINAPI extglFogf(GLenum cap, GLfloat param)
{
	ApiName("glFogf");
	OutTraceOGL("%s: cap=%#x(%s) param=%f\n", ApiRef, cap, sglEnum(cap), param);
	
	if(dxw.dwFlags4 & DISABLEFOGGING){
		OutTraceOGL("%s: skip fog setting\n", ApiRef);
		return;
	}
	if((dxw.dwFlags13 & CONTROLFOGGING) && (cap == GL_FOG_DENSITY)){
		float f = param;
		if(f == 0.0f) f = 1.0f; // make the fog not null
		f = f * GetHookInfo()->FogFactor;
		OutTraceOGL("%s: FIXED State=GL_FOG_DENSITY factor=%f\n", ApiRef, f);
		param = f;
	}
	if((dxw.dwFlags13 & CONTROLFOGGING) && ((cap == GL_FOG_START) || (cap == GL_FOG_END))){
		float f = param;
		// try-catch to handle divide by zero or float overflow events
		__try {
			f = f / GetHookInfo()->FogFactor;
		}
		__except (EXCEPTION_EXECUTE_HANDLER){ 
			f = FLT_MAX;
		}; 
		OutTraceOGL("%s: FIXED State=%s end=%f\n", ApiRef, sglEnum(cap), f);
		param = f;
	}

	(*pglFogf)(cap, param);
}

void WINAPI extglFogi(GLenum cap, GLint param)
{
	ApiName("glFogi");
	OutTraceOGL("%s: cap=%#x(%s) param=%i\n", ApiRef, cap, sglEnum(cap), param);
	if(cap == GL_FOG_MODE) OutTraceOGL("> fog mode=%s\n", sglEnum(param));
	
	if(dxw.dwFlags4 & DISABLEFOGGING){
		OutTraceOGL("%s: skip fog setting\n", ApiRef);
		return;
	}

	(*pglFogi)(cap, param);
}

void WINAPI extglFogfv(GLenum cap, const GLfloat *param)
{
	ApiName("glFogfv");
	OutTraceOGL("%s: cap=%#x(%s)\n", ApiRef, cap, sglEnum(cap));
	if(cap == GL_FOG_COLOR) {
		OutTraceOGL("> color=(%f,%f,%f,%f)\n", 
		param[0],
		param[1],
		param[2],
		param[3]
		);
	}

	if(dxw.dwFlags4 & DISABLEFOGGING){
		OutTraceOGL("%s: skip fog setting\n", ApiRef);
		return;
	}
	if((dxw.dwFlags16 & SETFOGCOLOR) && (cap == GL_FOG_COLOR)){
		DWORD r, g, b;
		r = dxw.fogColor & 0x000000FF;
		g = (dxw.fogColor & 0x0000FF00) >> 8;
		b = (dxw.fogColor & 0x00FF0000) >> 16;
		GLfloat fogColor[4];
		fogColor[0] = r / 255.0f;
		fogColor[1] = g / 255.0f;
		fogColor[2] = b / 255.0f;
		fogColor[3] = param[3]; // alpha, pass through
		OutTraceOGL("%s: FIXED State=GL_FOG_COLOR color=(%f,%f,%f,%f)\n", ApiRef, 
			fogColor[0],
			fogColor[1],
			fogColor[2],
			fogColor[3]
			);
		(*pglFogfv)(cap, fogColor);
		return;
	}
	
	(*pglFogfv)(cap, param);
}

void WINAPI extglFogiv(GLenum cap, const GLint *param)
{
	ApiName("glFogiv");
	OutTraceOGL("%s: cap=%#x(%s)\n", ApiRef, cap, sglEnum(cap));
	if(cap == GL_FOG_COLOR) {
		OutTraceOGL("> color=(%#x,%#x,%#x,%#x)\n", 
		param[0],
		param[1],
		param[2],
		param[3]
		);
	}
	
	if(dxw.dwFlags4 & DISABLEFOGGING){
		OutTraceOGL("%s: skip fog setting\n", ApiRef);
		return;
	}
	if((dxw.dwFlags16 & SETFOGCOLOR) && (cap == GL_FOG_COLOR)){
		DWORD r, g, b;
		r = dxw.fogColor & 0x000000FF;
		g = (dxw.fogColor & 0x0000FF00) >> 8;
		b = (dxw.fogColor & 0x00FF0000) >> 16;
		GLint fogColor[4];
		fogColor[0] = r;
		fogColor[1] = g;
		fogColor[2] = b;
		fogColor[3] = param[3]; // alpha, pass through
		OutTraceOGL("%s: FIXED State=GL_FOG_COLOR Value=(%#x,%#x,%#x,%#x)\n", ApiRef, 
			fogColor[0],
			fogColor[1],
			fogColor[2],
			fogColor[3]
			);
		(*pglFogiv)(cap, fogColor);
		return;
	}	

	(*pglFogiv)(cap, param);
}

extern BOOL WINAPI dxwSetPixelFormat(char *, GDISetPixelFormat_Type, HDC, int, const PIXELFORMATDESCRIPTOR *);
extern int WINAPI dxwGetPixelFormat(ApiArg, GDIGetPixelFormat_Type pGetPixelFormat, HDC hdc);
extern int WINAPI dxwChoosePixelFormat(char *, ChoosePixelFormat_Type, HDC, const PIXELFORMATDESCRIPTOR *);
extern int WINAPI dxwDescribePixelFormat(char *, DescribePixelFormat_Type, HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);

BOOL WINAPI extwglSetPixelFormat(HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{ ApiName("wglSetPixelFormat"); return dxwSetPixelFormat(ApiRef, pwglSetPixelFormat, hdc, iPixelFormat, ppfd); }

int WINAPI extwglGetPixelFormat(HDC hdc)
{ ApiName("wglGetPixelFormat"); return dxwGetPixelFormat(ApiRef, pwglGetPixelFormat, hdc); }

int WINAPI extwglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd)
{ ApiName("wglGetPixelFormat"); return dxwChoosePixelFormat(ApiRef, pwglChoosePixelFormat, hdc, ppfd); }

int WINAPI extwglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{ ApiName("wglDescribePixelFormat"); return dxwDescribePixelFormat(ApiRef, pwglDescribePixelFormat, hdc, iPixelFormat, nBytes, ppfd); }

#ifdef TRACEALL
void WINAPI extglBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	ApiName("glBlendColor");
	OutTraceOGL("%s: RGBA=(%f, %f, %f, %f)\n", ApiRef, red, green, blue, alpha);
	(*pglBlendColor)(red, green, blue, alpha);
}

void WINAPI extglColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	ApiName("glColor4f");
	OutTraceOGL("%s: RGBA=(%f, %f, %f, %f)\n", ApiRef, red, green, blue, alpha);
	(*pglColor4f)(red, green, blue, alpha);
}

void WINAPI extglVertex2i(GLint x, GLint y)
{
	ApiName("glVertex2i");
	OutDebugOGL("%s: xy=(%d, %d)\n", ApiRef, x, y);
	(*pglVertex2i)(x, y);
}

void WINAPI extglVertex3i(GLint x, GLint y, GLint z)
{
	ApiName("glVertex3i");
	OutDebugOGL("%s: xyz=(%d, %d, %d)\n", ApiRef, x, y, z);
	(*pglVertex3i)(x, y, z);
}

void WINAPI extglReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *data)
{
	ApiName("glReadPixels");
	OutTraceOGL("%s: pos=(%d,%d) size=(%dx%d) format=%#x type=%#x\n",
		ApiRef, x, y, width, height, format, type);
	//MessageBox(0, ApiRef, "dxwnd", 0);
	return (*pglReadPixels)(x, y, width, height, format, type, data);
}

void WINAPI extglReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data)
{
	ApiName("glReadnPixels");
	OutTraceOGL("%s: pos=(%d,%d) size=(%dx%d) format=%#x type=%#x size=%d\n",
		ApiRef, x, y, width, height, format, type);
	//MessageBox(0, ApiRef, "dxwnd", 0);
	return (*pglReadnPixels)(x, y, width, height, format, type, bufSize, data);
}

void WINAPI extglListBase(GLuint base)
{
	ApiName("glListBase");

	OutTraceOGL("%s: base=%d\n", ApiRef, base);
	(*pglListBase)(base);
}

void WINAPI extglTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	ApiName("glTexCoordPointer");
	OutTraceOGL("%s: size=%d type=%#x stride=%d\n", ApiRef, size, type, stride, pointer);

	return (*pglTexCoordPointer)(size, type, stride, pointer);
}
#endif

void WINAPI extglCopyTexImage2D(
	GLenum target,
  	GLint level,
  	GLenum internalFormat,
  	GLint x,
  	GLint y,
  	GLsizei width,
  	GLsizei height,
  	GLint border)
{
	ApiName("glCopyTexImage2D");
	OutTraceOGL("%s: target=%#x(%s) level=%#x internalformat=%#x pos=(%d,%d) size=(%dx%d) border=%d\n", 
		ApiRef, target, sglEnum(target), level, internalFormat, x, y, width, height, border);

	if(dxw.dwFlags17 & COMPENSATEOGLCOPY){
		gbScreenOverlap = TRUE;
		dxw.MapClient(&x, &y, &width, &height);
	}

	if(dxw.dwFlags4 & NOTEXTURES) return;

	return (*pglCopyTexImage2D)(target, level, internalFormat, x, y, width, height, border);
}

void WINAPI extglCopyTexImage1D(
	GLenum target,
  	GLint level,
  	GLenum internalFormat,
  	GLint x,
  	GLint y,
  	GLsizei width,
  	GLint border)
{
	ApiName("glCopyTexImage1D");
	OutTraceOGL("%s: target=%#x(%s) level=%#x internalformat=%#x pos=(%d,%d) width=%d border=%d\n", 
		ApiRef, target, sglEnum(target), level, internalFormat, x, y, width, border);

	//if(dxw.dwFlags17 & COMPENSATEOGLCOPY){
	//	gbScreenOverlap = TRUE;
	//	dxw.MapClient(&x, &y, &width, &height);
	//}

	if(dxw.dwFlags4 & NOTEXTURES) return;

	return (*pglCopyTexImage1D)(target, level, internalFormat, x, y, width, border);
}


void WINAPI extglCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	ApiName("glCopyTexSubImage2D");
	OutTraceOGL("%s: target=%#x(%s)  level=%d qoffsetxy=(%d, %d) xy=(%d, %d) size=(%d x %d)\n", 
		ApiRef, target, sglEnum(target), level, xoffset, yoffset, x, y, width, height);

	if(dxw.dwFlags17 & COMPENSATEOGLCOPY){
		gbScreenOverlap = TRUE;
		dxw.MapClient(&x, &y, &width, &height);
	}

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglCopyTexSubImage2D)(target, level, xoffset, yoffset, x, y, width, height);
}

void WINAPI extglCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	ApiName("glCopyTexSubImage1D");
	OutTraceOGL("%s: target=%#x(%s)  level=%d offset=%d xy=(%d, %d) width=%d\n", 
		ApiRef, target, sglEnum(target), level, xoffset, x, y, width);

	//if(dxw.dwFlags17 & COMPENSATEOGLCOPY){
	//	gbScreenOverlap = TRUE;
	//	dxw.MapClient(&x, &y, &width, &height);
	//}

	if(dxw.dwFlags4 & NOTEXTURES) return;

	(*pglCopyTexSubImage1D)(target, level, xoffset, x, y, width);
}

void WINAPI extglTexCoord2f(GLfloat s, GLfloat t)
{
	if(gbScreenOverlap) dxw.MapClient(&s, &t);
	(*pglTexCoord2f)(s, t);
}

void WINAPI extglTexCoord2fv(const GLfloat *v)
{
	ApiName("glTexCoord2fv");
	OutTraceOGL("%s: s=%f v=%f\n", ApiRef, v[0], v[1]);
	(*pglTexCoord2fv)(v);
}

#ifdef TRACEALL
void WINAPI extglutMotionFunc(void (*func)(int x, int y))
{
	ApiName("glutMotionFunc");
	OutTraceOGL("%s: func=%#x\n", ApiRef, func);
	(*pglutMotionFunc)(func);
	//MessageBox(0, ApiRef, "dxwnd", 0);
}

void WINAPI extglutPassiveMotionFunc(void (*func)(int x, int y))
{
	ApiName("glutPassiveMotionFunc");
	OutTraceOGL("%s: func=%#x\n", ApiRef, func);
	(*pglutPassiveMotionFunc)(func);
	//MessageBox(0, ApiRef, "dxwnd", 0);
}

void WINAPI extglDeleteLists(GLuint list, GLsizei range)
{
	ApiName("glDeleteLists");
	OutTraceOGL("%s: list=%d range=%d\n", ApiRef, list, range);
	(*pglDeleteLists)(list, range);
}
#endif // TRACEALL

#ifdef SUPPRESSLIGHTMODETWOSIDE
void WINAPI extglLightModeli(GLenum name, GLint param)
{
	if(name == GL_LIGHT_MODEL_TWO_SIDE) return;
	(*pglLightModeli)(name, param);
}
#endif

