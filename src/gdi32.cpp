#define _CRT_SECURE_NO_WARNINGS

//#define BEST_QUALITY ANTIALIASED_QUALITY
#define BEST_QUALITY CLEARTYPE_NATURAL_QUALITY

//#define TRACEALL
//#define TRACEPALETTE
//#define TRACEREGIONS
//#define TRACEFONTS
//#define HOOKTEXTMETRICS

#ifdef TRACEALL
#define TRACEPALETTE
#define TRACEREGIONS
#define TRACEFONTS
#define HOOKTEXTMETRICS
#endif

#define _MODULE "gdi32"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "hddraw.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "shareddc.hpp"
#include "asyncdc.h"

#include "stdio.h"

#include <d3dtypes.h>
#include <d3dumddi.h>

#ifndef DXW_NOTRACES
#define TraceError() OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError())
#define IfTraceError() if(!res) TraceError()
extern void DumpDibSection(char *, HDC, const BITMAPINFO *, UINT, VOID *);
extern void DumpBitmap(char *, HBITMAP);
extern void DumpHDC(HDC, int, int, int, int);
extern void DumpFullHDC(char *, HDC);
#else
#define TraceError()
#define IfTraceError()
#endif

static BOOL bGDIRecursionFlag = FALSE;

extern BYTE GetCharsetFromANSICodepage(UINT);

typedef int (WINAPI *MultiByteToWideChar_Type)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
extern MultiByteToWideChar_Type pMultiByteToWideChar;
typedef int (WINAPI *WideCharToMultiByte_Type)(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
extern WideCharToMultiByte_Type pWideCharToMultiByte;

static void Stopper(char *s, int line)
{
	char sMsg[81];
	sprintf_s(sMsg, 80, "break: \"%s\"", s);
	MessageBox(0, sMsg, "break", MB_OK | MB_ICONEXCLAMATION);
}

//#define STOPPER_TEST // comment out to eliminate
#ifdef STOPPER_TEST
#define STOPPER(s) Stopper(s, __LINE__)
#else
#define STOPPER(s)
#endif
#define _Warn(s) MessageBox(0, s, "to do", MB_ICONEXCLAMATION)

#ifndef D3DKMT_SETGAMMARAMP
typedef struct _D3DKMT_SETGAMMARAMP
{
    D3DKMT_HANDLE                   hDevice;       // in: device handle
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    D3DDDI_GAMMARAMP_TYPE           Type;          // in: Gamma ramp type
    union
    {
        D3DDDI_GAMMA_RAMP_RGB256x3x16* pGammaRampRgb256x3x16;
        D3DDDI_GAMMA_RAMP_DXGI_1*      pGammaRampDXGI1;
    };
    UINT                            Size;
} D3DKMT_SETGAMMARAMP;
#endif

#ifdef HOOKTEXTMETRICS
typedef BOOL (WINAPI *GetTextMetricsA_Type)(HDC, LPTEXTMETRICA);
GetTextMetricsA_Type pGetTextMetricsA;
BOOL WINAPI extGetTextMetricsA(HDC, LPTEXTMETRICA);

typedef DWORD (WINAPI *GetCharacterPlacementA_Type)(HDC, LPCSTR, int, int, LPGCP_RESULTSA, DWORD);
GetCharacterPlacementA_Type pGetCharacterPlacementA;
DWORD WINAPI extGetCharacterPlacementA(HDC, LPCSTR, int, int, LPGCP_RESULTSA, DWORD);

typedef DWORD (WINAPI *GetFontLanguageInfo_Type)(HDC);
GetFontLanguageInfo_Type pGetFontLanguageInfo;
DWORD WINAPI extGetFontLanguageInfo(HDC);

typedef UINT (WINAPI *SetTextAlign_Type)(HDC, UINT);
SetTextAlign_Type pSetTextAlign;
UINT WINAPI extSetTextAlign(HDC, UINT);

#endif

typedef LONG (WINAPI *D3DKMTSetGammaRamp_Type)(const D3DKMT_SETGAMMARAMP *);
D3DKMTSetGammaRamp_Type pD3DKMTSetGammaRamp;
LONG WINAPI extD3DKMTSetGammaRamp(const D3DKMT_SETGAMMARAMP *);

typedef BOOL (WINAPI *UpdateColors_Type)(HDC);
UpdateColors_Type pUpdateColors;
BOOL WINAPI extUpdateColors(HDC);
typedef UINT (WINAPI *GetDIBColorTable_Type)(HDC, UINT, UINT, RGBQUAD *);
GetDIBColorTable_Type pGetDIBColorTable = NULL;
UINT WINAPI extGetDIBColorTable(HDC, UINT, UINT, RGBQUAD *);
typedef UINT (WINAPI *SetDIBColorTable_Type)(HDC, UINT, UINT, const RGBQUAD *);
SetDIBColorTable_Type pSetDIBColorTable = NULL;
UINT WINAPI extSetDIBColorTable(HDC, UINT, UINT, const RGBQUAD *);
typedef BOOL (WINAPI *SwapBuffers_Type)(HDC);
SwapBuffers_Type pSwapBuffers;
BOOL WINAPI extSwapBuffers(HDC);
typedef int (WINAPI *GetMapMode_Type)(HDC);
GetMapMode_Type pGetMapMode;
int WINAPI extGetMapMode(HDC);
typedef COLORREF (WINAPI *SetColor_Type)(HDC, COLORREF);
SetColor_Type pSetTextColor;
COLORREF WINAPI extSetTextColor(HDC, COLORREF);
COLORREF WINAPI extSetBkColor(HDC, COLORREF);

// -- NLS
typedef HGDIOBJ (WINAPI *GetStockObject_Type)(int);
GetStockObject_Type pGetStockObject;
HGDIOBJ WINAPI extGetStockObject(int);

//
typedef UINT (WINAPI *GetNearestPaletteIndex_Type)(HPALETTE, COLORREF);
GetNearestPaletteIndex_Type pGetNearestPaletteIndex;
UINT WINAPI extGetNearestPaletteIndex(HPALETTE, COLORREF);
typedef COLORREF (WINAPI *SetBkColor_Type)(HDC, COLORREF);
SetBkColor_Type pSetBkColor = NULL;

typedef BOOL (WINAPI *UnrealizeObject_Type)(HGDIOBJ);
UnrealizeObject_Type pUnrealizeObject;
BOOL WINAPI extUnrealizeObject(HGDIOBJ);

typedef LONG (WINAPI *GetBitmapBits_Type)(HBITMAP, LONG, LPVOID);
GetBitmapBits_Type pGetBitmapBits;
LONG WINAPI extGetBitmapBits(HBITMAP, LONG, LPVOID);

#ifdef TRACEALL
typedef HENHMETAFILE (WINAPI *SetWinMetaFileBits_Type)(UINT, const BYTE *, HDC, const METAFILEPICT *);
SetWinMetaFileBits_Type pSetWinMetaFileBits;
HENHMETAFILE WINAPI extSetWinMetaFileBits(UINT, const BYTE *, HDC, const METAFILEPICT *);
typedef int (WINAPI *SetBkMode_Type)(HDC, int);
SetBkMode_Type pSetBkMode = NULL;
int WINAPI extSetBkMode(HDC, int);
COLORREF WINAPI extSetBkColor(HDC, COLORREF);
typedef BOOL (WINAPI *ScaleWindowExtEx_Type)(HDC, int, int, int, int, LPSIZE);
ScaleWindowExtEx_Type pGDIScaleWindowExtEx;
extern BOOL WINAPI extScaleWindowExtEx(HDC, int, int, int, int, LPSIZE);
typedef HDC	(WINAPI *CreateICA_Type)(LPCTSTR, LPCTSTR, LPCTSTR, const DEVMODEA *);
CreateICA_Type pCreateICA;
HDC WINAPI extCreateICA(LPCTSTR, LPCTSTR, LPCTSTR, const DEVMODEA *);
typedef HDC	(WINAPI *CreateICW_Type)(LPCWSTR, LPCWSTR, LPCWSTR, const DEVMODEW *);
CreateICW_Type pCreateICW;
HDC WINAPI extCreateICW(LPCWSTR, LPCWSTR, LPCWSTR, const DEVMODEW *);
typedef int (WINAPI *GetObject_Type)(HANDLE, int, LPVOID);
GetObject_Type pGetObjectA, pGetObjectW;
int WINAPI extGetObjectA(HANDLE, int, LPVOID);
int WINAPI extGetObjectW(HANDLE, int, LPVOID);
typedef HGDIOBJ (WINAPI *GetCurrentObject_Type)(HDC, UINT);
GetCurrentObject_Type pGetCurrentObject;
HGDIOBJ WINAPI extGetCurrentObject(HDC, UINT);
#endif // TRACEALL

#ifdef TRACEFONTS
typedef BOOL (WINAPI *GetCharWidth_Type)(HDC, UINT, UINT, LPINT);
BOOL WINAPI extGetCharWidthA(HDC, UINT, UINT, LPINT);
BOOL WINAPI extGetCharWidthW(HDC, UINT, UINT, LPINT);
BOOL WINAPI extGetCharWidth32A(HDC, UINT, UINT, LPINT);
BOOL WINAPI extGetCharWidth32W(HDC, UINT, UINT, LPINT);
GetCharWidth_Type pGetCharWidthA, pGetCharWidthW, pGetCharWidth32A, pGetCharWidth32W;
typedef BOOL (WINAPI *GetCharABCWidthsA_Type)(HDC, UINT, UINT, LPABC);
GetCharABCWidthsA_Type pGetCharABCWidthsA;
BOOL WINAPI extGetCharABCWidthsA(HDC, UINT, UINT, LPABC);
typedef DWORD (WINAPI *GetGlyphOutlineA_Type)(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, const MAT2 *);
GetGlyphOutlineA_Type pGetGlyphOutlineA;
DWORD WINAPI extGetGlyphOutlineA(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, const MAT2 *);
#endif // TRACEFONTS

#ifdef TRACEREGIONS
typedef BOOL (WINAPI *PtInRegion_Type)(HRGN, int, int);
typedef int (WINAPI *CombineRgn_Type)(HRGN, HRGN, HRGN, int);
typedef HRGN (WINAPI *CreateEllipticRgn_Type)(int, int, int, int);
typedef HRGN (WINAPI *CreateEllipticRgnIndirect_Type)(const RECT *);
typedef HRGN (WINAPI *CreateRectRgn_Type)(int, int, int, int);
typedef HRGN (WINAPI *CreateRectRgnIndirect_Type)(const RECT *);
typedef HRGN (WINAPI *CreateRoundRectRgn_Type)(int, int, int, int, int, int);
typedef HRGN (WINAPI *CreatePolygonRgn_Type)(const POINT *, int, int);
typedef HRGN (WINAPI *CreatePolyPolygonRgn_Type)(const POINT *, CONST int *, int, int);
typedef BOOL (WINAPI *SetRectRgn_Type)(HRGN, int, int, int, int);
typedef int (WINAPI *OffsetRgn_Type)(HRGN, int, int);
typedef int (WINAPI *GetRgnBox_Type)(HRGN, LPRECT);
typedef int (WINAPI *GetMetaRgn_Type)(HDC, HRGN);
typedef int (WINAPI *SetMetaRgn_Type)(HDC);
typedef int (WINAPI *GetRandomRgn_Type)(HDC, HRGN, INT);

PtInRegion_Type pPtInRegion;
CombineRgn_Type pCombineRgn;
CreateEllipticRgn_Type pCreateEllipticRgn;
CreateEllipticRgnIndirect_Type pCreateEllipticRgnIndirect;
CreateRectRgn_Type pCreateRectRgn;
CreateRectRgnIndirect_Type pCreateRectRgnIndirect;
CreateRoundRectRgn_Type pCreateRoundRectRgn;
CreatePolygonRgn_Type pCreatePolygonRgn;
CreatePolyPolygonRgn_Type pCreatePolyPolygonRgn;
SetRectRgn_Type pSetRectRgn;
OffsetRgn_Type pOffsetRgn;
GetRgnBox_Type pGetRgnBox;
GetMetaRgn_Type pGetMetaRgn;
SetMetaRgn_Type pSetMetaRgn;
GetRandomRgn_Type pGetRandomRgn;

BOOL WINAPI extPtInRegion(HRGN, int, int);
int WINAPI extCombineRgn(HRGN, HRGN, HRGN, int);
HRGN WINAPI extCreateEllipticRgn(int, int, int, int);
HRGN WINAPI extCreateEllipticRgnIndirect(const RECT *);
HRGN WINAPI extCreateRectRgn(int, int, int, int);
HRGN WINAPI extCreateRectRgnIndirect(const RECT *);
HRGN WINAPI extCreateRoundRectRgn(int, int, int, int, int, int);
HRGN WINAPI extCreatePolygonRgn(const POINT *, int, int);
HRGN WINAPI extCreatePolyPolygonRgn(const POINT *, CONST int *, int, int);
BOOL WINAPI extSetRectRgn(HRGN, int, int, int, int);
int WINAPI extOffsetRgn(HRGN, int, int);
int WINAPI extGetRgnBox(HRGN, LPRECT);
int WINAPI extGetMetaRgn(HDC, HRGN);
int WINAPI extSetMetaRgn(HDC);
int WINAPI extGetRandomRgn(HDC, HRGN, INT);
#endif // TRACEREGIONS

typedef HBITMAP (WINAPI *CreateBitmap_Type)(int, int, UINT, UINT, CONST VOID *);
CreateBitmap_Type pCreateBitmap;
HBITMAP WINAPI extCreateBitmap(int, int, UINT, UINT, CONST VOID *);

typedef BOOL (WINAPI *SetBrushOrgEx_Type)(HDC, int, int, LPPOINT);
SetBrushOrgEx_Type pSetBrushOrgEx;
BOOL WINAPI extSetBrushOrgEx(HDC, int, int, LPPOINT);
typedef BOOL (WINAPI *GetBrushOrgEx_Type)(HDC, LPPOINT);
GetBrushOrgEx_Type pGetBrushOrgEx;
BOOL WINAPI extGetBrushOrgEx(HDC, LPPOINT);

typedef BOOL (WINAPI *PtVisible_Type)(HDC, int, int);
PtVisible_Type pPtVisible;
BOOL WINAPI extPtVisible(HDC, int, int);
typedef BOOL (WINAPI *RectVisible_Type)(HDC, const RECT *);
RectVisible_Type pRectVisible;
BOOL WINAPI extRectVisible(HDC, const RECT *);
typedef BOOL (WINAPI *SelectClipPath_Type)(HDC, int);
SelectClipPath_Type pSelectClipPath;
BOOL WINAPI extSelectClipPath(HDC, int);

typedef BOOL (WINAPI *GetWindowExtEx_Type)(HDC, LPSIZE);
GetWindowExtEx_Type pGetWindowExtEx;
BOOL WINAPI extGetWindowExtEx(HDC, LPSIZE);

#ifdef TRACEFONTS
typedef int	(WINAPI *EnumFontsA_Type)(HDC, LPCSTR, FONTENUMPROC, LPARAM);
typedef int (WINAPI *EnumFontFamiliesA_Type)(HDC, LPCSTR, FONTENUMPROCA, LPARAM);
typedef HFONT (WINAPI *CreateFontIndirectA_Type)(const LOGFONTA *);

EnumFontsA_Type pEnumFontsA;
EnumFontFamiliesA_Type pEnumFontFamiliesA;
CreateFontIndirectA_Type pCreateFontIndirectA;

int WINAPI extEnumFontsA(HDC, LPCSTR, FONTENUMPROC, LPARAM);
int WINAPI extEnumFontFamiliesA(HDC, LPCSTR, FONTENUMPROCA, LPARAM);
HFONT WINAPI extCreateFontIndirectA(const LOGFONTA *);
#endif // TRACEFONTS

#ifdef TRACEPALETTE
typedef BOOL (WINAPI *ResizePalette_Type)(HPALETTE, UINT);

ResizePalette_Type pResizePalette = NULL;

BOOL WINAPI extResizePalette(HPALETTE, UINT);
#endif // TRACEPALETTE

// to do (?): for virtual CD/HD path translation
// CreateMetaFileA / W  (*)
// CreateEnhMetaFileA / W  (*)
// GetEnhMetaFileA / W
// GetMetaFileA / W
// (*) in "Spearhead"

static HookEntryEx_Type Hooks[]={
	//{HOOK_IAT_CANDIDATE, 0, "DPtoLP", (FARPROC)DPtoLP, (FARPROC *)&pDPtoLP, (FARPROC)extDPtoLP},
#ifdef HOOKTEXTMETRICS
	{HOOK_IAT_CANDIDATE, 0, "GetTextMetricsA", (FARPROC)GetTextMetricsA, (FARPROC *)&pGetTextMetricsA, (FARPROC)extGetTextMetricsA},
	{HOOK_IAT_CANDIDATE, 0, "GetCharacterPlacementA", (FARPROC)GetCharacterPlacementA, (FARPROC *)&pGetCharacterPlacementA, (FARPROC)extGetCharacterPlacementA},
	{HOOK_IAT_CANDIDATE, 0, "GetFontLanguageInfo", (FARPROC)GetFontLanguageInfo, (FARPROC *)&pGetFontLanguageInfo, (FARPROC)extGetFontLanguageInfo},
	{HOOK_IAT_CANDIDATE, 0, "SetTextAlign", (FARPROC)SetTextAlign, (FARPROC *)&pSetTextAlign, (FARPROC)extSetTextAlign},
#endif

	{HOOK_IAT_CANDIDATE, 0, "GetDeviceCaps", (FARPROC)GetDeviceCaps, (FARPROC *)&pGDIGetDeviceCaps, (FARPROC)extGetDeviceCaps},
	{HOOK_IAT_CANDIDATE, 0, "SaveDC", (FARPROC)SaveDC, (FARPROC *)&pGDISaveDC, (FARPROC)extGDISaveDC},
	{HOOK_IAT_CANDIDATE, 0, "RestoreDC", (FARPROC)RestoreDC, (FARPROC *)&pGDIRestoreDC, (FARPROC)extGDIRestoreDC},
	{HOOK_HOT_CANDIDATE, 0, "AnimatePalette", (FARPROC)AnimatePalette, (FARPROC *)&pAnimatePalette, (FARPROC)extAnimatePalette},
	{HOOK_HOT_CANDIDATE, 0, "UpdateColors", (FARPROC)UpdateColors, (FARPROC *)&pUpdateColors, (FARPROC)extUpdateColors},
	{HOOK_HOT_CANDIDATE, 0, "CreatePalette", (FARPROC)CreatePalette, (FARPROC *)&pGDICreatePalette, (FARPROC)extGDICreatePalette},
	{HOOK_HOT_CANDIDATE, 0, "SelectPalette", (FARPROC)SelectPalette, (FARPROC *)&pGDISelectPalette, (FARPROC)extSelectPalette},
	{HOOK_HOT_CANDIDATE, 0, "RealizePalette", (FARPROC)RealizePalette, (FARPROC *)&pGDIRealizePalette, (FARPROC)extRealizePalette},
	{HOOK_HOT_CANDIDATE, 0, "GetSystemPaletteEntries", (FARPROC)GetSystemPaletteEntries, (FARPROC *)&pGDIGetSystemPaletteEntries, (FARPROC)extGetSystemPaletteEntries},
	{HOOK_HOT_CANDIDATE, 0, "SetSystemPaletteUse", (FARPROC)SetSystemPaletteUse, (FARPROC *)&pSetSystemPaletteUse, (FARPROC)extSetSystemPaletteUse},
	{HOOK_HOT_CANDIDATE, 0, "SetPixelFormat", (FARPROC)NULL, (FARPROC *)&pGDISetPixelFormat, (FARPROC)extGDISetPixelFormat},
	{HOOK_IAT_CANDIDATE, 0, "GetPixelFormat", (FARPROC)NULL, (FARPROC *)&pGDIGetPixelFormat, (FARPROC)extGDIGetPixelFormat},
	{HOOK_IAT_CANDIDATE, 0, "ChoosePixelFormat", (FARPROC)NULL, (FARPROC *)&pChoosePixelFormat, (FARPROC)extChoosePixelFormat},
	{HOOK_IAT_CANDIDATE, 0, "DescribePixelFormat", (FARPROC)NULL, (FARPROC *)&pDescribePixelFormat, (FARPROC)extDescribePixelFormat},
	{HOOK_HOT_CANDIDATE, 0, "GetPaletteEntries", (FARPROC)GetPaletteEntries, (FARPROC *)&pGetPaletteEntries, (FARPROC)extGetPaletteEntries},
	{HOOK_HOT_CANDIDATE, 0, "SetPaletteEntries", (FARPROC)SetPaletteEntries, (FARPROC *)&pSetPaletteEntries, (FARPROC)extSetPaletteEntries},
	{HOOK_HOT_CANDIDATE, 0, "GetSystemPaletteUse", (FARPROC)GetSystemPaletteUse, (FARPROC *)&pGetSystemPaletteUse, (FARPROC)extGetSystemPaletteUse},
	{HOOK_HOT_CANDIDATE, 0, "SetROP2", (FARPROC)SetROP2, (FARPROC *)&pSetROP2, (FARPROC)extSetROP2}, // Titanic
	{HOOK_HOT_CANDIDATE, 0, "GetDIBColorTable", (FARPROC)GetDIBColorTable, (FARPROC *)&pGetDIBColorTable, (FARPROC)extGetDIBColorTable},
	{HOOK_HOT_CANDIDATE, 0, "SetDIBColorTable", (FARPROC)SetDIBColorTable, (FARPROC *)&pSetDIBColorTable, (FARPROC)extSetDIBColorTable},
	{HOOK_HOT_CANDIDATE, 0, "SwapBuffers", (FARPROC)SwapBuffers, (FARPROC *)&pSwapBuffers, (FARPROC)extSwapBuffers},
	{HOOK_IAT_CANDIDATE, 0, "UnrealizeObject", (FARPROC)NULL, (FARPROC *)&pUnrealizeObject, (FARPROC)extUnrealizeObject},
	{HOOK_IAT_CANDIDATE, 0, "StretchDIBits", (FARPROC)StretchDIBits, (FARPROC *)&pStretchDIBits, (FARPROC)extStretchDIBits},
	{HOOK_HOT_CANDIDATE, 0, "SetDIBits", (FARPROC)SetDIBits, (FARPROC *)&pSetDIBits, (FARPROC)extSetDIBits},
#ifdef TRACEPALETTE
	{HOOK_IAT_CANDIDATE, 0, "ResizePalette", (FARPROC)ResizePalette, (FARPROC *)&pResizePalette, (FARPROC)extResizePalette},
#endif	
	// 
	// regions related functions unrelated with DC handles
	//
#ifdef TRACEREGIONS
	{HOOK_IAT_CANDIDATE, 0, "CombineRgn", (FARPROC)CombineRgn, (FARPROC *)&pCombineRgn, (FARPROC)extCombineRgn},
	// commented out since they alter text on screen...... (see Imperialism II difficulty level menu)
	// v2.03.47 - restored: needed for "688(I) Hunter Killer" periscope ....
	{HOOK_IAT_CANDIDATE, 0, "CreateEllipticRgn", (FARPROC)CreateEllipticRgn, (FARPROC *)&pCreateEllipticRgn, (FARPROC)extCreateEllipticRgn},
	{HOOK_IAT_CANDIDATE, 0, "CreateEllipticRgnIndirect", (FARPROC)CreateEllipticRgnIndirect, (FARPROC *)&pCreateEllipticRgnIndirect, (FARPROC)extCreateEllipticRgnIndirect},
	{HOOK_IAT_CANDIDATE, 0, "CreateRectRgn", (FARPROC)CreateRectRgn, (FARPROC *)&pCreateRectRgn, (FARPROC)extCreateRectRgn},
	{HOOK_IAT_CANDIDATE, 0, "CreateRectRgnIndirect", (FARPROC)CreateRectRgnIndirect, (FARPROC *)&pCreateRectRgnIndirect, (FARPROC)extCreateRectRgnIndirect},
	{HOOK_IAT_CANDIDATE, 0, "CreateRoundRectRgn", (FARPROC)CreateRoundRectRgn, (FARPROC *)&pCreateRoundRectRgn, (FARPROC)extCreateRoundRectRgn},
	{HOOK_IAT_CANDIDATE, 0, "CreatePolygonRgn", (FARPROC)CreatePolygonRgn, (FARPROC *)&pCreatePolygonRgn, (FARPROC)extCreatePolygonRgn},
	{HOOK_IAT_CANDIDATE, 0, "CreatePolyPolygonRgn", (FARPROC)CreatePolyPolygonRgn, (FARPROC *)&pCreatePolyPolygonRgn, (FARPROC)extCreatePolyPolygonRgn},
	{HOOK_IAT_CANDIDATE, 0, "PtInRegion", (FARPROC)PtInRegion, (FARPROC *)&pPtInRegion, (FARPROC)extPtInRegion},
	{HOOK_IAT_CANDIDATE, 0, "SetRectRgn", (FARPROC)SetRectRgn, (FARPROC *)&pSetRectRgn, (FARPROC)extSetRectRgn}, 
	{HOOK_IAT_CANDIDATE, 0, "GetRgnBox", (FARPROC)GetRgnBox, (FARPROC *)&pGetRgnBox, (FARPROC)extGetRgnBox},
	{HOOK_IAT_CANDIDATE, 0, "GetRandomRgn", (FARPROC)GetRandomRgn, (FARPROC *)&pGetRandomRgn, (FARPROC)extGetRandomRgn},
	{HOOK_IAT_CANDIDATE, 0, "GetMetaRgn", (FARPROC)GetMetaRgn, (FARPROC *)&pGetMetaRgn, (FARPROC)extGetMetaRgn},
	{HOOK_IAT_CANDIDATE, 0, "SetMetaRgn", (FARPROC)SetMetaRgn, (FARPROC *)&pSetMetaRgn, (FARPROC)extSetMetaRgn},
#endif // TRACEREGIONS
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "SetBkColor", (FARPROC)SetBkColor, (FARPROC *)&pSetBkColor, (FARPROC)extSetBkColor}, // hot patching causes recursion here (on Win11) on "Amber Journey Beyond" ...
	{HOOK_HOT_CANDIDATE, 0, "SetBkMode", (FARPROC)SetBkMode, (FARPROC *)&pSetBkMode, (FARPROC)extSetBkMode},
	{HOOK_IAT_CANDIDATE, 0, "ScaleWindowExtEx", (FARPROC)ScaleWindowExtEx, (FARPROC *)&pGDIScaleWindowExtEx, (FARPROC)extScaleWindowExtEx},
	{HOOK_HOT_CANDIDATE, 0, "CreateICA", (FARPROC)CreateICA, (FARPROC *)&pCreateICA, (FARPROC)extCreateICA}, // Riven, Everquest
	{HOOK_HOT_CANDIDATE, 0, "CreateICW", (FARPROC)CreateICW, (FARPROC *)&pCreateICW, (FARPROC)extCreateICW}, // unseen ...
	{HOOK_HOT_CANDIDATE, 0, "GetObjectA", (FARPROC)GetObjectA, (FARPROC *)&pGetObjectA, (FARPROC)extGetObjectA}, // 
	{HOOK_HOT_CANDIDATE, 0, "GetObjectW", (FARPROC)GetObjectW, (FARPROC *)&pGetObjectW, (FARPROC)extGetObjectW}, // 
	{HOOK_HOT_CANDIDATE, 0, "GetCurrentObject", (FARPROC)GetCurrentObject, (FARPROC *)&pGetCurrentObject, (FARPROC)extGetCurrentObject}, // 
	{HOOK_IAT_CANDIDATE, 0, "SetWinMetaFileBits", (FARPROC)SetWinMetaFileBits, (FARPROC *)&pSetWinMetaFileBits, (FARPROC)extSetWinMetaFileBits},
#endif // TRACEALL
#ifdef TRACEFONTS
	{HOOK_IAT_CANDIDATE, 0, "GetCharWidthA", (FARPROC)GetCharWidthA, (FARPROC *)&pGetCharWidthA, (FARPROC)extGetCharWidthA},
	{HOOK_IAT_CANDIDATE, 0, "GetCharWidthW", (FARPROC)GetCharWidthW, (FARPROC *)&pGetCharWidthW, (FARPROC)extGetCharWidthW},
#endif // TRACEFONTS
	{HOOK_IAT_CANDIDATE, 0, "CreateBitmap", (FARPROC)CreateBitmap, (FARPROC *)&pCreateBitmap, (FARPROC)extCreateBitmap},
	{HOOK_IAT_CANDIDATE, 0, "GetNearestPaletteIndex", (FARPROC)GetNearestPaletteIndex, (FARPROC *)&pGetNearestPaletteIndex, (FARPROC)extGetNearestPaletteIndex},
	{HOOK_HOT_CANDIDATE, 0, "DeleteObject", (FARPROC)DeleteObject, (FARPROC *)&pDeleteObject, (FARPROC)extDeleteObject}, // font scaling ....
	
	{HOOK_IAT_CANDIDATE, 0, "TextOutA", (FARPROC)TextOutA, (FARPROC *)&pGDITextOutA, (FARPROC)extTextOutA},
	{HOOK_IAT_CANDIDATE, 0, "TextOutW", (FARPROC)TextOutW, (FARPROC *)&pGDITextOutW, (FARPROC)extTextOutW},
	{HOOK_IAT_CANDIDATE, 0, "ExtTextOutA", (FARPROC)ExtTextOutA, (FARPROC *)&pExtTextOutA, (FARPROC)extExtTextOutA},
	{HOOK_IAT_CANDIDATE, 0, "ExtTextOutW", (FARPROC)ExtTextOutW, (FARPROC *)&pExtTextOutW, (FARPROC)extExtTextOutW},
	{HOOK_IAT_CANDIDATE, 0, "PolyTextOutA", (FARPROC)PolyTextOutA, (FARPROC *)&pPolyTextOutA, (FARPROC)extPolyTextOutA},
	{HOOK_IAT_CANDIDATE, 0, "PolyTextOutW", (FARPROC)PolyTextOutW, (FARPROC *)&pPolyTextOutW, (FARPROC)extPolyTextOutW},
	{HOOK_HOT_CANDIDATE, 0, "CreateFontA", (FARPROC)CreateFontA, (FARPROC *)&pGDICreateFontA, (FARPROC)extCreateFontA},
	{HOOK_HOT_CANDIDATE, 0, "CreateFontIndirectA", (FARPROC)CreateFontIndirectA, (FARPROC *)&pGDICreateFontIndirectA, (FARPROC)extCreateFontIndirectA}, // Valhalla Chronicles
	{HOOK_HOT_CANDIDATE, 0, "CreateFontW", (FARPROC)CreateFontW, (FARPROC *)&pGDICreateFontW, (FARPROC)extCreateFontW},
	{HOOK_HOT_CANDIDATE, 0, "CreateFontIndirectW", (FARPROC)CreateFontIndirectW, (FARPROC *)&pGDICreateFontIndirectW, (FARPROC)extCreateFontIndirectW},
#ifdef TRACEFONTS
	{HOOK_IAT_CANDIDATE, 0, "GetCharABCWidthsA", (FARPROC)GetCharABCWidthsA, (FARPROC *)&pGetCharABCWidthsA, (FARPROC)extGetCharABCWidthsA},
	{HOOK_IAT_CANDIDATE, 0, "GetGlyphOutlineA", (FARPROC)GetGlyphOutlineA, (FARPROC *)&pGetGlyphOutlineA, (FARPROC)extGetGlyphOutlineA},
#endif // TRACEFONTS
	{HOOK_IAT_CANDIDATE, 0, "D3DKMTSetGammaRamp", (FARPROC)NULL, (FARPROC *)&pD3DKMTSetGammaRamp, (FARPROC)extD3DKMTSetGammaRamp},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
}; 
 
static HookEntryEx_Type RemapHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "SetViewportOrgEx", (FARPROC)SetViewportOrgEx, (FARPROC *)&pSetViewportOrgEx, (FARPROC)extSetViewportOrgEx}, // needed in ShowBanner
	{HOOK_IAT_CANDIDATE, 0, "SetViewportExtEx", (FARPROC)SetViewportExtEx, (FARPROC *)&pSetViewportExtEx, (FARPROC)extSetViewportExtEx},
	{HOOK_IAT_CANDIDATE, 0, "GetViewportOrgEx", (FARPROC)GetViewportOrgEx, (FARPROC *)&pGetViewportOrgEx, (FARPROC)extGetViewportOrgEx},
	{HOOK_IAT_CANDIDATE, 0, "GetViewportExtEx", (FARPROC)GetViewportExtEx, (FARPROC *)&pGetViewportExtEx, (FARPROC)extGetViewportExtEx},
	{HOOK_IAT_CANDIDATE, 0, "GetWindowOrgEx", (FARPROC)GetWindowOrgEx, (FARPROC *)&pGetWindowOrgEx, (FARPROC)extGetWindowOrgEx},
	{HOOK_IAT_CANDIDATE, 0, "SetWindowOrgEx", (FARPROC)SetWindowOrgEx, (FARPROC *)&pSetWindowOrgEx, (FARPROC)extSetWindowOrgEx},
	{HOOK_HOT_CANDIDATE, 0, "GetWindowExtEx", (FARPROC)GetWindowExtEx, (FARPROC *)&pGetWindowExtEx, (FARPROC)extGetWindowExtEx},
	{HOOK_IAT_CANDIDATE, 0, "SetWindowExtEx", (FARPROC)SetWindowExtEx, (FARPROC *)&pSetWindowExtEx, (FARPROC)extSetWindowExtEx},
	{HOOK_IAT_CANDIDATE, 0, "SetBrushOrgEx", (FARPROC)SetBrushOrgEx, (FARPROC *)&pSetBrushOrgEx, (FARPROC)extSetBrushOrgEx},
	{HOOK_IAT_CANDIDATE, 0, "GetBrushOrgEx", (FARPROC)GetBrushOrgEx, (FARPROC *)&pGetBrushOrgEx, (FARPROC)extGetBrushOrgEx},
	{HOOK_IAT_CANDIDATE, 0, "GetCurrentPositionEx", (FARPROC)GetCurrentPositionEx, (FARPROC *)&pGetCurrentPositionEx, (FARPROC)extGetCurrentPositionEx},
	//{HOOK_IAT_CANDIDATE, 0, "GetRegionData", (FARPROC)NULL, (FARPROC *)&pGetRegionData, (FARPROC)extGetRegionData},
	{HOOK_IAT_CANDIDATE, 0, "CreateCompatibleDC", (FARPROC)CreateCompatibleDC, (FARPROC *)&pGDICreateCompatibleDC, (FARPROC)extGDICreateCompatibleDC}, /* to check */
	//TODO {HOOK_IAT_CANDIDATE, 0, "DrawEscape", (FARPROC)DrawEscape, (FARPROC *)&pDrawEscape, (FARPROC)extDrawEscape}, /* to check */
	{HOOK_IAT_CANDIDATE, 0, "GetDCOrgEx", (FARPROC)GetDCOrgEx, (FARPROC *)&pGetDCOrgEx, (FARPROC)extGetDCOrgEx}, 
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator 
};

static HookEntryEx_Type SyscallHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetDIBits", (FARPROC)GetDIBits, (FARPROC *)&pGetDIBits, (FARPROC)extGetDIBits},
	{HOOK_HOT_CANDIDATE, 0, "GetBitmapBits", (FARPROC)GetBitmapBits, (FARPROC *)&pGetBitmapBits, (FARPROC)extGetBitmapBits}, // Win16 compatibility
	{HOOK_IAT_CANDIDATE, 0, "CreateCompatibleBitmap", (FARPROC)CreateCompatibleBitmap, (FARPROC *)&pCreateCompatibleBitmap, (FARPROC)extCreateCompatibleBitmap}, 
	{HOOK_IAT_CANDIDATE, 0, "CreateDIBitmap", (FARPROC)CreateDIBitmap, (FARPROC *)&pCreateDIBitmap, (FARPROC)extCreateDIBitmap}, 
	{HOOK_HOT_CANDIDATE, 0, "CreateDIBSection", (FARPROC)CreateDIBSection, (FARPROC *)&pCreateDIBSection, (FARPROC)extCreateDIBSection}, 
	{HOOK_IAT_CANDIDATE, 0, "CreateDiscardableBitmap", (FARPROC)NULL, (FARPROC *)&pCreateDiscardableBitmap, (FARPROC)extCreateDiscardableBitmap}, 
	{HOOK_IAT_CANDIDATE, 0, "ExtFloodFill", (FARPROC)NULL, (FARPROC *)&pExtFloodFill, (FARPROC)extExtFloodFill}, 
	{HOOK_IAT_CANDIDATE, 0, "GdiAlphaBlend", (FARPROC)NULL, (FARPROC *)&pGdiAlphaBlend, (FARPROC)extGdiAlphaBlend}, 
	{HOOK_IAT_CANDIDATE, 0, "GdiGradientFill", (FARPROC)NULL, (FARPROC *)&pGdiGradientFill, (FARPROC)extGdiGradientFill},
	{HOOK_IAT_CANDIDATE, 0, "GdiTransparentBlt", (FARPROC)NULL, (FARPROC *)&pGdiTransparentBlt, (FARPROC)extGdiTransparentBlt}, 
	{HOOK_IAT_CANDIDATE, 0, "Pie", (FARPROC)NULL, (FARPROC *)&pPie, (FARPROC)extPie},
	{HOOK_IAT_CANDIDATE, 0, "AngleArc", (FARPROC)NULL, (FARPROC *)&pAngleArc, (FARPROC)extAngleArc}, 
	{HOOK_IAT_CANDIDATE, 0, "PolyPolyline", (FARPROC)NULL, (FARPROC *)&pPolyPolyline, (FARPROC)extPolyPolyline},
	{HOOK_IAT_CANDIDATE, 0, "FillRgn", (FARPROC)NULL, (FARPROC *)&pFillRgn, (FARPROC)extFillRgn},
	{HOOK_IAT_CANDIDATE, 0, "FrameRgn", (FARPROC)NULL, (FARPROC *)&pFrameRgn, (FARPROC)extFrameRgn}, 
	{HOOK_IAT_CANDIDATE, 0, "InvertRgn", (FARPROC)NULL, (FARPROC *)&pInvertRgn, (FARPROC)extInvertRgn}, 
	{HOOK_IAT_CANDIDATE, 0, "PaintRgn", (FARPROC)NULL, (FARPROC *)&pPaintRgn, (FARPROC)extPaintRgn}, 
	{HOOK_IAT_CANDIDATE, 0, "SetMapMode", (FARPROC)NULL, (FARPROC *)&pSetMapMode, (FARPROC)extSetMapMode}, 
	{HOOK_IAT_CANDIDATE, 0, "GetMapMode", (FARPROC)NULL, (FARPROC *)&pGetMapMode, (FARPROC)extGetMapMode}, 
	{HOOK_IAT_CANDIDATE, 0, "SetDIBitsToDevice", (FARPROC)SetDIBitsToDevice, (FARPROC *)&pSetDIBitsToDevice, (FARPROC)extSetDIBitsToDevice}, // does the stretching
	{HOOK_IAT_CANDIDATE, 0, "Polyline", (FARPROC)Polyline, (FARPROC *)&pPolyline, (FARPROC)extPolyline},
	{HOOK_IAT_CANDIDATE, 0, "BitBlt", (FARPROC)BitBlt, (FARPROC *)&pGDIBitBlt, (FARPROC)extGDIBitBlt},
	{HOOK_IAT_CANDIDATE, 0, "StretchBlt", (FARPROC)StretchBlt, (FARPROC *)&pGDIStretchBlt, (FARPROC)extGDIStretchBlt},
	{HOOK_IAT_CANDIDATE, 0, "PatBlt", (FARPROC)PatBlt, (FARPROC *)&pGDIPatBlt, (FARPROC)extGDIPatBlt},
	{HOOK_IAT_CANDIDATE, 0, "MaskBlt", (FARPROC)MaskBlt, (FARPROC *)&pMaskBlt, (FARPROC)extMaskBlt},
	{HOOK_IAT_CANDIDATE, 0, "Rectangle", (FARPROC)Rectangle, (FARPROC *)&pGDIRectangle, (FARPROC)extRectangle},
	{HOOK_IAT_CANDIDATE, 0, "RoundRect", (FARPROC)RoundRect, (FARPROC *)&pRoundRect, (FARPROC)extRoundRect},
	{HOOK_IAT_CANDIDATE, 0, "Polygon", (FARPROC)Polygon, (FARPROC *)&pPolygon, (FARPROC)extPolygon},
	{HOOK_IAT_CANDIDATE, 0, "PolyPolygon", (FARPROC)PolyPolygon, (FARPROC *)&pPolyPolygon, (FARPROC)extPolyPolygon},
	{HOOK_IAT_CANDIDATE, 0, "PolyBezier", (FARPROC)PolyBezier, (FARPROC *)&pPolyBezier, (FARPROC)extPolyBezier},
	{HOOK_IAT_CANDIDATE, 0, "PolyBezierTo", (FARPROC)PolyBezierTo, (FARPROC *)&pPolyBezierTo, (FARPROC)extPolyBezierTo},
	{HOOK_IAT_CANDIDATE, 0, "PolylineTo", (FARPROC)PolylineTo, (FARPROC *)&pPolylineTo, (FARPROC)extPolylineTo},
	{HOOK_IAT_CANDIDATE, 0, "PolyDraw", (FARPROC)PolyDraw, (FARPROC *)&pPolyDraw, (FARPROC)extPolyDraw},
	{HOOK_IAT_CANDIDATE, 0, "GetPixel", (FARPROC)GetPixel, (FARPROC *)&pGetPixel, (FARPROC)extGetPixel},
	{HOOK_IAT_CANDIDATE, 0, "PlgBlt", (FARPROC)PlgBlt, (FARPROC *)&pPlgBlt, (FARPROC)extPlgBlt},
	{HOOK_IAT_CANDIDATE, 0, "SetPixel", (FARPROC)SetPixel, (FARPROC *)&pSetPixel, (FARPROC)extSetPixel},
	{HOOK_IAT_CANDIDATE, 0, "SetPixelV", (FARPROC)SetPixelV, (FARPROC *)&pSetPixelV, (FARPROC)extSetPixelV},
	{HOOK_IAT_CANDIDATE, 0, "Chord", (FARPROC)Chord, (FARPROC *)&pChord, (FARPROC)extChord},
	{HOOK_IAT_CANDIDATE, 0, "Ellipse", (FARPROC)Ellipse, (FARPROC *)&pEllipse, (FARPROC)extEllipse},
	{HOOK_IAT_CANDIDATE, 0, "ArcTo", (FARPROC)ArcTo, (FARPROC *)&pArcTo, (FARPROC)extArcTo},
	{HOOK_IAT_CANDIDATE, 0, "LineTo", (FARPROC)LineTo, (FARPROC *)&pLineTo, (FARPROC)extLineTo},
	{HOOK_IAT_CANDIDATE, 0, "Arc", (FARPROC)Arc, (FARPROC *)&pArc, (FARPROC)extArc},
	{HOOK_IAT_CANDIDATE, 0, "MoveToEx", (FARPROC)MoveToEx, (FARPROC *)&pMoveToEx, (FARPROC)extMoveToEx},
	//{HOOK_IAT_CANDIDATE, 0, "DeleteDC", (FARPROC)DeleteDC, (FARPROC *)&pGDIDeleteDC, (FARPROC)extGDIDeleteDC}, // for tracing only! (commented: crashes Dylan Dog HLP!!)
	{HOOK_IAT_CANDIDATE, 0, "PlayEnhMetaFile", (FARPROC)PlayEnhMetaFile, (FARPROC *)&pPlayEnhMetaFile, (FARPROC)extPlayEnhMetaFile}, 

	{HOOK_HOT_CANDIDATE, 0, "SelectObject", (FARPROC)SelectObject, (FARPROC *)&pSelectObject, (FARPROC)extSelectObject}, // font scaling ....
	{HOOK_IAT_CANDIDATE, 0, "SetTextColor", (FARPROC)SetTextColor, (FARPROC *)&pSetTextColor, (FARPROC)extSetTextColor}, // font color ....
	{HOOK_IAT_CANDIDATE, 0, "SetBkColor", (FARPROC)SetBkColor, (FARPROC *)&pSetBkColor, (FARPROC)extSetBkColor}, // font background ....

	//
	// clip region related functions
	//
	{HOOK_HOT_CANDIDATE, 0, "GetClipRgn", (FARPROC)GetClipRgn, (FARPROC *)&pGetClipRgn, (FARPROC)extGetClipRgn}, 
	{HOOK_HOT_CANDIDATE, 0, "SelectClipRgn", (FARPROC)SelectClipRgn, (FARPROC *)&pSelectClipRgn, (FARPROC)extSelectClipRgn}, // Sid Meier's Alpha Centaury
	{HOOK_HOT_CANDIDATE, 0, "ExtSelectClipRgn", (FARPROC)ExtSelectClipRgn, (FARPROC *)&pExtSelectClipRgn, (FARPROC)extExtSelectClipRgn}, 
	{HOOK_HOT_CANDIDATE, 0, "OffsetClipRgn", (FARPROC)OffsetClipRgn, (FARPROC *)&pOffsetClipRgn, (FARPROC)extOffsetClipRgn}, 
	{HOOK_HOT_CANDIDATE, 0, "IntersectClipRect", (FARPROC)IntersectClipRect, (FARPROC *)&pIntersectClipRect, (FARPROC)extIntersectClipRect}, // Riven !!
	{HOOK_HOT_CANDIDATE, 0, "ExcludeClipRect", (FARPROC)ExcludeClipRect, (FARPROC *)&pExcludeClipRect, (FARPROC)extExcludeClipRect}, 
	{HOOK_HOT_CANDIDATE, 0, "GetClipBox", (FARPROC)GetClipBox, (FARPROC *)&pGDIGetClipBox, (FARPROC)extGetClipBox},
	{HOOK_HOT_CANDIDATE, 0, "PtVisible", (FARPROC)PtVisible, (FARPROC *)&pPtVisible, (FARPROC)extPtVisible},
	{HOOK_HOT_CANDIDATE, 0, "RectVisible", (FARPROC)RectVisible, (FARPROC *)&pRectVisible, (FARPROC)extRectVisible},
	{HOOK_HOT_CANDIDATE, 0, "SelectClipPath", (FARPROC)SelectClipPath, (FARPROC *)&pSelectClipPath, (FARPROC)extSelectClipPath},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator 
};

static HookEntryEx_Type CreateDCHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "CreateDCA", (FARPROC)CreateDCA, (FARPROC *)&pGDICreateDCA, (FARPROC)extGDICreateDCA}, 
	{HOOK_IAT_CANDIDATE, 0, "CreateDCW", (FARPROC)CreateDCW, (FARPROC *)&pGDICreateDCW, (FARPROC)extGDICreateDCW}, 
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator 
};

static HookEntryEx_Type EmulateHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "GetObjectType", (FARPROC)GetObjectType, (FARPROC *)&pGetObjectType, (FARPROC)extGetObjectType},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type TextHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "GetTextExtentPointA", (FARPROC)NULL, (FARPROC *)&pGetTextExtentPointA, (FARPROC)extGetTextExtentPointA}, 
	{HOOK_IAT_CANDIDATE, 0, "GetTextExtentPoint32A", (FARPROC)NULL, (FARPROC *)&pGetTextExtentPoint32A, (FARPROC)extGetTextExtentPoint32A}, 
#ifdef TRACEFONTS
	{HOOK_HOT_CANDIDATE, 0, "EnumFontsA", (FARPROC)EnumFontsA, (FARPROC *)&pEnumFontsA, (FARPROC)extEnumFontsA}, // Titanic
	{HOOK_HOT_CANDIDATE, 0, "EnumFontFamiliesA", (FARPROC)EnumFontFamiliesA, (FARPROC *)&pEnumFontFamiliesA, (FARPROC)extEnumFontFamiliesA}, // Valhalla Chronicles
#endif
	//{HOOK_HOT_CANDIDATE, 0, "SelectObject", (FARPROC)SelectObject, (FARPROC *)&pSelectObject, (FARPROC)extSelectObject}, // font scaling ....
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type GammaHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "SetDeviceGammaRamp", (FARPROC)SetDeviceGammaRamp, (FARPROC *)&pGDISetDeviceGammaRamp, (FARPROC)extSetDeviceGammaRamp},
	{HOOK_IAT_CANDIDATE, 0, "GetDeviceGammaRamp", (FARPROC)GetDeviceGammaRamp, (FARPROC *)&pGDIGetDeviceGammaRamp, (FARPROC)extGetDeviceGammaRamp},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type FontHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "CreateScalableFontResourceA", (FARPROC)CreateScalableFontResourceA, (FARPROC *)&pCreateScalableFontResourceA, (FARPROC)extCreateScalableFontResourceA},
	{HOOK_IAT_CANDIDATE, 0, "CreateScalableFontResourceW", (FARPROC)CreateScalableFontResourceW, (FARPROC *)&pCreateScalableFontResourceW, (FARPROC)extCreateScalableFontResourceW},
	{HOOK_IAT_CANDIDATE, 0, "AddFontResourceA", (FARPROC)AddFontResourceA, (FARPROC *)&pAddFontResourceA, (FARPROC)extAddFontResourceA},
	{HOOK_IAT_CANDIDATE, 0, "AddFontResourceW", (FARPROC)AddFontResourceW, (FARPROC *)&pAddFontResourceW, (FARPROC)extAddFontResourceW},
	// v2.04.05: Used by "Warhammer: Shadow of the Horned Rat"
	{HOOK_IAT_CANDIDATE, 0, "RemoveFontResourceA", (FARPROC)RemoveFontResourceA, (FARPROC *)&pRemoveFontResourceA, (FARPROC)extRemoveFontResourceA},
	{HOOK_IAT_CANDIDATE, 0, "RemoveFontResourceW", (FARPROC)RemoveFontResourceW, (FARPROC *)&pRemoveFontResourceW, (FARPROC)extRemoveFontResourceW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type ProjectHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "SwapBuffers", (FARPROC)SwapBuffers, (FARPROC *)&pSwapBuffers, (FARPROC)extSwapBuffers},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

extern HRESULT WINAPI extDirectDrawCreate(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);
extern HRESULT WINAPI extDirectDrawCreateEx(GUID FAR *, LPDIRECTDRAW FAR *, REFIID, IUnknown FAR *);

static char *libname = "gdi32.dll";

void HookGDI32Init()
{
	HookLibInitEx(Hooks);
	HookLibInitEx(RemapHooks);
	HookLibInitEx(SyscallHooks);
	HookLibInitEx(EmulateHooks);
	HookLibInitEx(TextHooks);
	HookLibInitEx(GammaHooks);
}

void HookGDI32(HMODULE module)
{
	HookLibraryEx(module, Hooks, libname);

	if (dxw.GDIEmulationMode != GDIMODE_NONE) HookLibraryEx(module, SyscallHooks, libname);
	if (dxw.dwFlags1 & CLIENTREMAPPING)		HookLibraryEx(module, RemapHooks, libname);
	if (dxw.dwFlags3 & GDIEMULATEDC)		HookLibraryEx(module, EmulateHooks, libname);	
	if (dxw.dwFlags1 & FIXTEXTOUT)			HookLibraryEx(module, TextHooks, libname);
	//if ((dxw.dwFlags11 & SHRINKFONTWIDTH) || (dxw.dwFlags8 & QUALITYFONTS))		
	//	HookLibraryEx(module, TextSizeHooks, libname);
	if (dxw.dwFlags2 & DISABLEGAMMARAMP)	HookLibraryEx(module, GammaHooks, libname);
	// v2.02.33 - for "Stratego" compatibility option
	if(dxw.dwFlags3 & FONTBYPASS)			HookLibraryEx(module, FontHooks, libname);
	if(dxw.dwFlags10 & CREATEDCHOOK)		HookLibraryEx(module, CreateDCHooks, libname);
	//if(dxw.dwFlags11 & CUSTOMLOCALE)		HookLibraryEx(module, NlsHooks, libname);
	if(dxw.dwFlags12 & PROJECTBUFFER)			HookLibraryEx(module, ProjectHooks, libname);
}

FARPROC Remap_GDI32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;

	if(addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;

	if (dxw.GDIEmulationMode != GDIMODE_NONE) if(addr=RemapLibraryEx(proc, hModule, SyscallHooks)) return addr;
	if (dxw.dwFlags1 & CLIENTREMAPPING)		if(addr=RemapLibraryEx(proc, hModule, RemapHooks)) return addr;
	if (dxw.dwFlags3 & GDIEMULATEDC)		if (addr=RemapLibraryEx(proc, hModule, EmulateHooks)) return addr;
	if (dxw.dwFlags1 & FIXTEXTOUT)			if(addr=RemapLibraryEx(proc, hModule, TextHooks)) return addr;
	//if ((dxw.dwFlags11 & SHRINKFONTWIDTH) || (dxw.dwFlags8 & QUALITYFONTS))
	//	if(addr=RemapLibraryEx(proc, hModule, TextSizeHooks)) return addr;
	if (dxw.dwFlags2 & DISABLEGAMMARAMP)	if(addr=RemapLibraryEx(proc, hModule, GammaHooks)) return addr;
	// v2.02.33 - for "Stratego" compatibility option
	if (dxw.dwFlags3 & FONTBYPASS)			if(addr=RemapLibraryEx(proc, hModule, FontHooks)) return addr;
	if (dxw.dwFlags10 & CREATEDCHOOK)		if(addr=RemapLibraryEx(proc, hModule, CreateDCHooks)) return addr;
	//if (dxw.dwFlags11 & CUSTOMLOCALE)		if(addr=RemapLibraryEx(proc, hModule, NlsHooks)) return addr;
	if (dxw.dwFlags12 & PROJECTBUFFER)		if(addr=RemapLibraryEx(proc, hModule, ProjectHooks)) return addr;

	return NULL;
}

//--------------------------------------------------------------------------------------------
//
// extern and common functions
//
//--------------------------------------------------------------------------------------------

extern DWORD PaletteEntries[256];
extern Unlock4_Type pUnlockMethod(int);
extern HRESULT WINAPI sBlt(int, Blt_Type, char *, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX, BOOL);

extern GetDC_Type pGetDC;
extern ReleaseDC_Type pReleaseDC1;

#ifndef DXW_NOTRACES
static char *ExplainDIBUsage(UINT u)
{
	char *p;
	switch(u){
		case DIB_PAL_COLORS: p="DIB_PAL_COLORS"; break;
		case DIB_RGB_COLORS: p="DIB_RGB_COLORS"; break;
		default: p="invalid"; break;
	}
	return p;
}

static char *sBMIDump(BITMAPINFO *pbmi)
{
	static char s[256];
	if(!pbmi) return ""; // safeguard
	char *sCompr[7]={"RGB", "RLE8", "RLE4", "BITFIELDS", "JPEG", "PNG", "FOURCC" };
	DWORD dwCompr = pbmi->bmiHeader.biCompression;
	sprintf(s, "colors RGBX=(%#x,%#x,%#x,%#x) header bitc=%d size=(%dx%d) compr=%#x(%s)", 
		pbmi->bmiColors->rgbRed, pbmi->bmiColors->rgbGreen, pbmi->bmiColors->rgbBlue, pbmi->bmiColors->rgbReserved,
		pbmi->bmiHeader.biBitCount, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight, 
		dwCompr, sCompr[dwCompr > BI_PNG ? (BI_PNG+1) : dwCompr]);
	return s;
}

static void TraceBITMAPINFOHEADER(char *fName, BITMAPINFOHEADER *bmi)
{
	if(!IsTraceGDI) return;
	OutTrace("%s: BitmapInfo {Size=%d dim=(%dx%d) Planes=%d bitcount=%d Compression=%#x SizeImage=%d PelsPerMeter=%dx%d colors=U%d:I%d}\n",
		fName, bmi->biSize, bmi->biWidth, bmi->biHeight, bmi->biPlanes, bmi->biBitCount, bmi->biCompression, 
		bmi->biSizeImage, bmi->biXPelsPerMeter, bmi->biYPelsPerMeter, bmi->biClrUsed, bmi->biClrImportant);
	if(bmi->biSize > sizeof(BITMAPINFOHEADER)){
		BITMAPV4HEADER *bm4 = (BITMAPV4HEADER *)bmi;
		OutTrace("%s: BitmapInfoV4 {RGBA mask=%#x:%#x:%#x:%#x cstype=%#x gamma RGB=%#x:%#x:%#x}\n",
			fName, bm4->bV4RedMask, bm4->bV4GreenMask, bm4->bV4BlueMask, bm4->bV4AlphaMask,
			bm4->bV4CSType, bm4->bV4GammaRed, bm4->bV4GammaGreen, bm4->bV4GammaBlue);
	}
	if(bmi->biSize > sizeof(BITMAPV4HEADER)){
		BITMAPV5HEADER *bm5 = (BITMAPV5HEADER *)bmi;
		OutTrace("%s: BitmapInfoV5 {intent=%#x profiledata=%#x profilesize=%#x resvd=%#x}\n",
			fName, bm5->bV5Intent, bm5->bV5ProfileData, bm5->bV5ProfileSize, bm5->bV5Reserved);
	}
}

static char *ExplainGetRgnCode(int code)
{
	char *s;
	switch(code){
		case 0: s="no region"; break;
		case 1: s="success"; break;
		case -1: s="error"; break;
		default: s="unknown"; break;
	}
	return s;
}

static char *ExplainRgnMode(int mode)
{
	char *s;
	switch(mode){
		case RGN_AND: s="AND"; break;
		case RGN_COPY: s="COPY"; break;
		case RGN_DIFF: s="DIFF"; break;
		case RGN_OR: s="OR"; break;
		case RGN_XOR: s="XOR"; break;
		default: s="unknown"; break;
	}
	return s;
}

static char *sGradientMode(ULONG mode)
{
	char *s;
	switch(mode){
		case GRADIENT_FILL_RECT_H: s="RECT_H"; break;
		case GRADIENT_FILL_RECT_V: s="RECT_V"; break;
		case GRADIENT_FILL_TRIANGLE: s="TRIANGLE"; break;
		default: s="???"; break;
	}
	return s;
}

char *objname(int f)
{
	char *s;
	static char *objnames[]={
		"WHITE_BRUSH", "LTGRAY_BRUSH", "GRAY_BRUSH", "DKGRAY_BRUSH", "BLACK_BRUSH",
		"NULL_BRUSH", "WHITE_PEN", "BLACK_PEN", "NULL_PEN", "", "OEM_FIXED_FONT", 
		"ANSI_FIXED_FONT", "ANSI_VAR_FONT", "SYSTEM_FONT", "DEVICE_DEFAULT_FONT",
		"DEFAULT_PALETTE", "SYSTEM_FIXED_FONT", "DEFAULT_GUI_FONT", "DC_BRUSH", "DC_PEN"};

	s = "???";
	if(f <= STOCK_LAST) s = objnames[f]; 
	return s;
}

static void dxwDumpRgn(char *api, HRGN hrgn)
{
	LPRGNDATA lpRgnData;
	size_t size;
	DWORD nCount;
	LPRECT lpRect;
	if(hrgn == 0) return; // don't dump NULL region
	size = GetRegionData(hrgn, 0, NULL);
	lpRgnData = (LPRGNDATA)malloc(size);
	GetRegionData(hrgn, size, lpRgnData);
	OutTraceGDI("%s: region\n", ApiRef); 
	OutTraceGDI("> dwSize=%d\n", lpRgnData->rdh.dwSize);
	OutTraceGDI("> iType=%d\n", lpRgnData->rdh.iType);
	OutTraceGDI("> nCount=%d\n", lpRgnData->rdh.nCount);
	OutTraceGDI("> nRgnSize=%d\n", lpRgnData->rdh.nRgnSize);
	OutTraceGDI("> rcBound=(%d,%d)-(%d,%d)\n", 
		lpRgnData->rdh.rcBound.left, 
		lpRgnData->rdh.rcBound.top, 
		lpRgnData->rdh.rcBound.right, 
		lpRgnData->rdh.rcBound.bottom
		);
	lpRect = (LPRECT)(&lpRgnData->Buffer);
	nCount = lpRgnData->rdh.nCount;
	//dxw.MapClient(&(lpRgnData->rdh.rcBound));
	for(DWORD i=0; i<nCount; i++) {
		OutTraceGDI("> rect[%d]=(%d,%d)-(%d,%d)\n", 
			i,
			lpRect->left, 
			lpRect->top, 
			lpRect->right, 
			lpRect->bottom);
		lpRect++;
	}
}
#else
#define dxwDumpRgn(a, h) 
#endif

#define MINSIZEW 320
#define MINSIZEH 200

BOOL dxwStretchBlt(HDC hdcDest, int nXDest, int nYDest, int nWDest, int nHDest, 
				   HDC hdcSrc, int nXSrc, int nYSrc, int nWidth, int nHeight, 
				   DWORD dwRop)
{
	BOOL res;
	static HDC hdcSmall = NULL;
	static HBITMAP bmapSmall = NULL;
	static HDC hdcScaled = NULL;
	static HBITMAP bmapScaled = NULL;
	static int h = 0;
	static int w = 0;
	static DWORD lastFlush = 0;

	if((nWidth > MINSIZEW) && (nHeight > MINSIZEH)){
		res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWDest, nHDest, hdcSrc, nXSrc, nYSrc, nWidth, nHeight, dwRop);
    } else {
		if(!hdcSmall){
			hdcSmall = CreateCompatibleDC(hdcSrc);
			HBITMAP bmapSmall = CreateCompatibleBitmap(hdcSrc, dxw.GetScreenWidth(), dxw.GetScreenHeight());
			SelectObject(hdcSmall, bmapSmall);
		}
		RECT rect;
		(*pGetClientRect)(dxw.GethWnd(), &rect);
		if((w != rect.right) || (h != rect.bottom)){
			if(bmapScaled) DeleteObject(bmapScaled);
			if(hdcScaled) DeleteObject(hdcScaled);
			w = rect.right;
			h = rect.bottom;
			hdcScaled = CreateCompatibleDC(hdcDest);
			bmapScaled = CreateCompatibleBitmap(hdcDest, w, h);
			SetStretchBltMode(hdcScaled, HALFTONE);
		}
		(*pGDIBitBlt)(hdcSmall, nXSrc, nYSrc, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
		SelectObject(hdcScaled, bmapScaled);
		(*pGDIStretchBlt)(hdcScaled, 0, 0, w, h, hdcSrc, 0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight(), dwRop);
		(*pGDIBitBlt)(hdcDest, nXDest, nYDest, nWDest, nHDest, hdcScaled, nXDest, nYDest, dwRop);
		ReleaseDC(0, hdcScaled);
		ReleaseDC(0, hdcSmall);
		res = TRUE;
    }
	return res;
}

static void dxwGDIAsyncFlush(void)
{
	static DWORD lastUpdate = 0;
	DWORD now = (*pGetTickCount)();
	if(now - lastUpdate > 20) {
		dxw.ReleaseEmulatedDC(dxw.GethWnd());
		lastUpdate = now;
	}
}

//--------------------------------------------------------------------------------------------
//
// API hookers
//
//--------------------------------------------------------------------------------------------

int WINAPI extGetDeviceCaps(HDC hdc, int nindex)
{
	DWORD res;
	ApiName("GetDeviceCaps");
	
	res = (*pGDIGetDeviceCaps)(hdc, nindex);

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sInfo[1024];
		char sCaps[128];
		char sBuf[256];
		sprintf(sInfo, "%s: hdc=%#x index=%#x(%s)", ApiRef, hdc, nindex, ExplainDeviceCaps(nindex));
		switch(nindex){
			case RASTERCAPS:
				sprintf(sBuf, " res=0x%04x(%s)\n", res, ExplainRasterCaps(res, sCaps, 128)); 
				break;
			case BITSPIXEL:
			case COLORRES:
			case VERTRES:
			case HORZRES:
			case SIZEPALETTE:
			case NUMRESERVED:
				sprintf(sBuf, " res=%d\n", res);
				break;
			default:
				sprintf(sBuf, " res=0x%04x\n", res); 
				break;
		}
		strcat(sInfo, sBuf);
		OutTrace(sInfo);
	}
#endif

	switch(nindex){
	case VERTRES:
		if(dxw.isScaled){
			if(dxw.IsDesktop(WindowFromDC(hdc))) 
				res= dxw.GetScreenHeight();
			else {
				if(OBJ_DC == (*pGetObjectType)(hdc)){
					switch(dxw.GDIEmulationMode){
						case GDIMODE_SHAREDDC:
						case GDIMODE_EMULATED:
						default:
							break;
						case GDIMODE_STRETCHED:
							int dummy = 0;
							dxw.UnmapClient(&dummy, (int *)&res);
							break;
					}
				}
			}
			OutTraceDW("%s: fix(1) VERTRES cap=%d\n", ApiRef, res);
		}
		break;
	case HORZRES:
		if(dxw.isScaled){
			if(dxw.IsDesktop(WindowFromDC(hdc))) 
				res= dxw.GetScreenWidth();
			else {
				if(OBJ_DC == (*pGetObjectType)(hdc)){
					switch(dxw.GDIEmulationMode){
						case GDIMODE_SHAREDDC:
						case GDIMODE_EMULATED:
						default:
							break;
						case GDIMODE_STRETCHED:
							int dummy = 0;
							dxw.UnmapClient((int *)&res, &dummy);
							break;
					}
				}
			}
			OutTraceDW("%s: fix(2) HORZRES cap=%d\n", ApiRef, res);
		}
		break;
	case HORZSIZE:
		// t.b.d. maybe I should make all this only on condition that hdc = desktop dc?
		// this works on "Sherlock Holmes - Secret of the Silver Earring".
		if(dxw.dwFlags13 & FIXASPECTRATIO){
			// fix only when either SVGA or HDTV are set, not none or both!
			switch(dxw.dwFlags4 & (SUPPORTSVGA|SUPPORTHDTV)) {
				case SUPPORTSVGA:
					res = ((*pGDIGetDeviceCaps)(hdc, VERTSIZE) * 8) / 6;
					OutTraceDW("%s: fix(3) HORZSIZE (SVGA 8:6 ratio) cap=%d\n", ApiRef, res);
					break;
				case SUPPORTHDTV:
					res = ((*pGDIGetDeviceCaps)(hdc, VERTSIZE) * 15) / 9;
					OutTraceDW("%s: fix(3) HORZSIZE (SVGA 15:9 ratio) cap=%d\n", ApiRef, res);
					break;
				}
			}
		break;
	// WARNING: in no-emu mode, the INIT8BPP flag expose a RC_PALETTE capability that
	// is NOT implemented and may cause later troubles!
	case RASTERCAPS:
		if((dxw.dwFlags17 & LOCKPIXELFORMAT) && (dxw.dwFlags2 & INIT8BPP)) {
			res |= RC_PALETTE; // v2.02.12
			OutTraceDW("%s: fix(4) RASTERCAPS setting RC_PALETTE cap=%#x\n", ApiRef, res);
		}
		break;
	case BITSPIXEL:
		// v2.05.98 fix: do not confuse the initial value with the current value
		if(dxw.dwFlags17 & LOCKPIXELFORMAT){
			if(dxw.dwFlags2 & INIT8BPP) res = 8;
			if(dxw.dwFlags2 & INIT16BPP) res = 16;
			if(dxw.dwFlags7 & INIT24BPP) res = 24;
			if(dxw.dwFlags7 & INIT32BPP) res = 32;
			OutTraceDW("%s: fix(5) BITSPIXEL cap=%d\n", ApiRef, res);
		}
		break;
	}

	if(dxw.IsEmulated){
		switch(nindex){
		case RASTERCAPS:
			if((dxw.VirtualPixelFormat.dwRGBBitCount==8) || (dxw.dwFlags2 & INIT8BPP)){
				res |= RC_PALETTE;
#ifndef DXW_NOTRACES
				char sCaps[128];
				OutTraceDW("%s: fix(6) RASTERCAPS setting RC_PALETTE cap=%#x(%s)\n", ApiRef, res, ExplainRasterCaps(res, sCaps, 128));
#endif // DXW_NOTRACES
			}
			break;
		case BITSPIXEL:
			int PrevRes;
			PrevRes=res;
			if(dxw.VirtualPixelFormat.dwRGBBitCount!=0) res = dxw.VirtualPixelFormat.dwRGBBitCount;
			else res = dxw.ActualPixelFormat.dwRGBBitCount;
			if(dxw.dwFlags17 & LOCKPIXELFORMAT){
				if(dxw.dwFlags2 & INIT8BPP) res = 8;
				if(dxw.dwFlags2 & INIT16BPP) res = 16;
				if(dxw.dwFlags7 & INIT24BPP) res = 24;
				if(dxw.dwFlags7 & INIT32BPP) res = 32;
			}
			if(res > 32) res = 32; // safeguard
			if(PrevRes != res) OutTraceDW("%s: fix(7) BITSPIXEL cap=%d\n", ApiRef, res);
			break;
		case COLORRES:
			// v2.05.73: fix from Narzoul's DDrawCompat: COLORRES is the actual color
			// resolution, 24 bits is a always valid value, also for palettized modes
			//if(dxw.VirtualPixelFormat.dwRGBBitCount==8){
			res = 24;
			OutTraceDW("%s: fix(8) COLORRES cap=%d\n", ApiRef, res);
			//}
			break;
		case SIZEPALETTE:
			// v2.06.10: @#@ "Princess Maker" uses the 0 return code to tell that the desktop is NOT
			// set to 8 bit color mode. So, the 256 value should be used only for 8 bit color mode.
			if (dxw.VirtualPixelFormat.dwRGBBitCount == 8) {
				res = 256;
				OutTraceDW("%s: fix(9) SIZEPALETTE cap=%#x\n", ApiRef, res);
			}
			break;
		// v2.05.98: recovering NUMRESERVED & NUMCOLORS bad fixes.
		// don't trust MS documents: the situaation is more complex and depends
		// on the OS release, so these code blocks were determined experimentally
		// testing on real and virtual machines.
		case NUMRESERVED:
			if(dxw.EmulateWin95 && (dxw.VirtualPixelFormat.dwRGBBitCount > 8)){
				// v2.05.82: emulation of Microsoft shim "EmulateGetDeviceCaps"
				res = 0;
				OutTraceDW("%s: fix(10) NUMRESERVED cap=%#x\n", ApiRef, res);
			}
			else {
				// v2.05.73: fix from Narzoul's DDrawCompat: the value 20 fixes
				// the color problems of Premier Manager 97
				res = 20;
				OutTraceDW("%s: fix(11) NUMRESERVED cap=%#x\n", ApiRef, res);
			}
			break;
		case NUMCOLORS:
			if(dxw.VirtualPixelFormat.dwRGBBitCount == 8){
				// v2.05.73: fix from Narzoul's DDrawCompat: the value 20 fixes
				// the color problems of Premier Manager 97
				// 20 is the default return value for 8bit color depth on legacy Windows OS
				res = 20;
				OutTraceDW("%s: fix(12) NUMCOLORS cap=%#x\n", ApiRef, res);
			}
			else {
				// Fix for Lego LOCO -1 should be returned for higher than 8bit colors depths
				res = -1;
				OutTraceDW("%s: fix(13) NUMCOLORS cap=%#x\n", ApiRef, res);
			}
			break;
		}
	}
	return res;
}

BOOL WINAPI extTextOutA(HDC hdc, int nXStart, int nYStart, LPCTSTR lpString, int cchString)
{
	BOOL ret;
	ApiName("TextOutA");
	extern BOOL gFixed;
	OutTraceGDI("%s: hdc=%#x xy=(%d,%d) str=(%d)\"%.*s\"\n", ApiRef, hdc, nXStart, nYStart, cchString, cchString, lpString);
	OutHexSYS((LPBYTE)lpString, cchString);

	if((dxw.dwFlags19 & TRIMTEXTYPOS) && (nYStart < 0)) nYStart = 0; // patch for @#@ "Lineage"!!!

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextA(ApiRef, nXStart, nYStart, lpString, cchString);

	// sometimes useless (but not wrong!) conversion, once selected the proper charset codepage 
	// you can print also with UTF8 strings.
	// v2.06.04: uncommented, it is necessary for @#@ "Top Blade V" (Ko 2002)
	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		LPWSTR wstr = NULL;
		LPSTR cstr = NULL;
		int n;
		if(cchString == 0) return TRUE;
		cstr = (LPSTR)malloc(cchString+1);
		memcpy(cstr, lpString, cchString);
		cstr[cchString]=0;
		wstr = (LPWSTR)malloc((cchString+1)<<1);
		n = (*pMultiByteToWideChar)(dxw.CodePage, 0, cstr, cchString, wstr, cchString);
		wstr[n] = (WCHAR)0; // add terminator !!
		_if(n==0) OutTrace("!!! err=%d @%d\n", GetLastError(), __LINE__);
		if(!pGDITextOutW) pGDITextOutW = TextOutW;
		// v2.06.04 fix: when transformed to WIDE format, the text length is n, not cchString!
		// @#@ "Quovadis" (Ko 1998)
		// v2.06.07 fix: fixed for "Spirit Stallion of the Cimarron" on Russian locale
		//ret = extTextOutW(hdc, nXStart, nYStart, wstr, cchString);
		ret = extTextOutW(hdc, nXStart, nYStart, wstr, n);
		free(wstr);
		free(cstr);
		return ret;
	}

	if (!gFixed && dxw.IsToRemap(hdc)){

		switch(dxw.GDIEmulationMode){
			case GDIMODE_ASYNCDC:
				ret=(*pGDITextOutA)(lpADC->GetDC(hdc), nXStart, nYStart, lpString, cchString);
				lpADC->ReleaseDC();
				_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
				return ret;
				break;
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pGDITextOutA)(sdc.GetHdc(), nXStart, nYStart, lpString, cchString);
				// update whole screen to avoid make calculations about text position & size
				sdc.PutPrimaryDC(hdc, TRUE, FALSE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nXStart, &nYStart);
				OutTraceDW("%s: fixed dest=(%d,%d)\n", ApiRef, nXStart, nYStart);
				break;
		}
	}

	ret=(*pGDITextOutA)(hdc, nXStart, nYStart, lpString, cchString);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extTextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cchString)
{
	BOOL ret;
	ApiName("TextOutW");
	extern BOOL gFixed;
	OutTraceGDI("%s: hdc=%#x xy=(%d,%d) str=(%d)\"%.*ls\"\n", ApiRef, hdc, nXStart, nYStart, cchString, cchString, lpString);
	OutHexSYS((LPBYTE)lpString, cchString<<1);

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextW(ApiRef, nXStart, nYStart, lpString, cchString);

	if (!gFixed && dxw.IsToRemap(hdc)){

		switch(dxw.GDIEmulationMode){
			case GDIMODE_ASYNCDC:
				ret=(*pGDITextOutW)(lpADC->GetDC(hdc), nXStart, nYStart, lpString, cchString);
				lpADC->ReleaseDC();
				_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
				return ret;
				break;
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pGDITextOutW)(sdc.GetHdc(), nXStart, nYStart, lpString, cchString);
				// update whole screen to avoid make calculations about text position & size
				sdc.PutPrimaryDC(hdc, TRUE, FALSE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nXStart, &nYStart);
				OutTraceDW("%s: fixed dest=(%d,%d)\n", ApiRef, nXStart, nYStart);
				break;
		}
	}

	ret=(*pGDITextOutW)(hdc, nXStart, nYStart, lpString, cchString);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

#ifdef TRACEALL
BOOL WINAPI extScaleWindowExtEx(HDC hdc, int Xnum, int Xdenom, int Ynum, int Ydenom, LPSIZE lpSize)
{
	BOOL ret;
	ApiName("ScaleWindowExtEx");
	OutTraceGDI("%s: hdc=%#x num=(%d,%d) denom=(%d,%d) lpSize=%d\n",
		ApiRef, hdc, Xnum, Ynum, Xdenom, Ydenom, lpSize);

	// MessageBox(0, "ScaleWindowExtEx", "to fix", MB_OK | MB_ICONEXCLAMATION);
	// call found in "Lego Marvel Superheroes" by Mchaidang

	ret = (*pGDIScaleWindowExtEx)(hdc, Xnum, Xdenom, Ynum, Ydenom, lpSize);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}
#endif // TRACEALL

BOOL WINAPI extRectangle(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	BOOL ret;
	ApiName("Rectangle");

	OutTraceGDI("%s: hdc=%#x xy=(%d,%d)-(%d,%d)\n", ApiRef, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_ASYNCDC:
				ret=(*pGDIRectangle)(lpADC->GetDC(hdc), nLeftRect, nTopRect, nRightRect, nBottomRect);
				lpADC->ReleaseDC();
				_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
				return ret;
				break;
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pGDIRectangle)(sdc.GetHdc(), nLeftRect, nTopRect, nRightRect, nBottomRect);
				sdc.PutPrimaryDC(hdc, TRUE, nLeftRect, nTopRect, nRightRect-nLeftRect, nBottomRect-nTopRect);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nLeftRect, &nTopRect, &nRightRect, &nBottomRect);
				OutTraceDW("%s: fixed dest=(%d,%d)-(%d,%d)\n", ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect);
				break;
		}
	}

	ret=(*pGDIRectangle)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

int WINAPI extGDISaveDC(HDC hdc)
{
	int ret;

#ifndef DXW_NOTRACES
	if((dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(hdc, 0, 0, 0, 0);
#endif // DXW_NOTRACES

	ret=(*pGDISaveDC)(hdc);
	OutTraceGDI("SaveDC: hdc=%#x ret=%#x\n", hdc, ret);
	return ret;
}

BOOL WINAPI extGDIRestoreDC(HDC hdc, int nSavedDC)
{
	BOOL ret;
	ret=(*pGDIRestoreDC)(hdc, nSavedDC);
	OutTraceGDI("RestoreDC: hdc=%#x nSavedDC=%#x ret=%#x\n", hdc, nSavedDC, ret);
	return ret;
}

/* --------------------------------------------------------------------------- */
/*     Palette handling                                                        */
/* --------------------------------------------------------------------------- */

extern HDC hFlippedDC;
extern BOOL bFlippedDC;
extern void mySetPalette(int, int, LPPALETTEENTRY);

// v2.1.75: Hooking for GDI32 CreatePalette, SelectPalette, RealizePalette: 
// maps the GDI palette to the buffered DirectDraw one. This fixes the screen 
// output for "Dementia" (a.k.a. "Armed & Delirious").

// In emulated mode (when color depyth is 8BPP ?) it may happen that the game
// expects to get the requested system palette entries, while the 32BPP screen
// returns 0. "Mission Force Cyberstorm" is one of these. Returning the same
// value as nEntries, even though lppe is untouched, fixes the problem.

PALETTEENTRY DefaultSystemPalette[256]={ // default palette, captured on my PC with video mode set to 8BPP
	{0x00,0x00,0x00,0x00},{0x80,0x00,0x00,0x00},{0x00,0x80,0x00,0x00},{0x80,0x80,0x00,0x00},
	{0x00,0x00,0x80,0x00},{0x80,0x00,0x80,0x00},{0x00,0x80,0x80,0x00},{0xc0,0xc0,0xc0,0x00},
	{0xa0,0xa0,0xa0,0x00},{0xf0,0xf0,0xf0,0x00},{0xc0,0xdc,0xc0,0x00},{0xa6,0xca,0xf0,0x00},
	{0x04,0x04,0x04,0x00},{0x08,0x08,0x08,0x00},{0x0c,0x0c,0x0c,0x00},{0x11,0x11,0x11,0x00},
	{0x16,0x16,0x16,0x00},{0x1c,0x1c,0x1c,0x00},{0x22,0x22,0x22,0x00},{0x29,0x29,0x29,0x00},
	{0x55,0x55,0x55,0x00},{0x4d,0x4d,0x4d,0x00},{0x42,0x42,0x42,0x00},{0x39,0x39,0x39,0x00},
	{0xff,0x7c,0x80,0x00},{0xff,0x50,0x50,0x00},{0xd6,0x00,0x93,0x00},{0xcc,0xec,0xff,0x00},
	{0xef,0xd6,0xc6,0x00},{0xe7,0xe7,0xd6,0x00},{0xad,0xa9,0x90,0x00},{0x33,0x00,0x00,0x00},
	{0x66,0x00,0x00,0x00},{0x99,0x00,0x00,0x00},{0xcc,0x00,0x00,0x00},{0x00,0x33,0x00,0x00},
	{0x33,0x33,0x00,0x00},{0x66,0x33,0x00,0x00},{0x99,0x33,0x00,0x00},{0xcc,0x33,0x00,0x00},
	{0xff,0x33,0x00,0x00},{0x00,0x66,0x00,0x00},{0x33,0x66,0x00,0x00},{0x66,0x66,0x00,0x00},
	{0x99,0x66,0x00,0x00},{0xcc,0x66,0x00,0x00},{0xff,0x66,0x00,0x00},{0x00,0x99,0x00,0x00},
	{0x33,0x99,0x00,0x00},{0x66,0x99,0x00,0x00},{0x99,0x99,0x00,0x00},{0xcc,0x99,0x00,0x00},
	{0xff,0x99,0x00,0x00},{0x00,0xcc,0x00,0x00},{0x33,0xcc,0x00,0x00},{0x66,0xcc,0x00,0x00},
	{0x99,0xcc,0x00,0x00},{0xcc,0xcc,0x00,0x00},{0xff,0xcc,0x00,0x00},{0x66,0xff,0x00,0x00},
	{0x99,0xff,0x00,0x00},{0xcc,0xff,0x00,0x00},{0x00,0x00,0x33,0x00},{0x33,0x00,0x33,0x00},
	{0x66,0x00,0x33,0x00},{0x99,0x00,0x33,0x00},{0xcc,0x00,0x33,0x00},{0xff,0x00,0x33,0x00},
	{0x00,0x33,0x33,0x00},{0x33,0x33,0x33,0x00},{0x66,0x33,0x33,0x00},{0x99,0x33,0x33,0x00},
	{0xcc,0x33,0x33,0x00},{0xff,0x33,0x33,0x00},{0x00,0x66,0x33,0x00},{0x33,0x66,0x33,0x00},
	{0x66,0x66,0x33,0x00},{0x99,0x66,0x33,0x00},{0xcc,0x66,0x33,0x00},{0xff,0x66,0x33,0x00},
	{0x00,0x99,0x33,0x00},{0x33,0x99,0x33,0x00},{0x66,0x99,0x33,0x00},{0x99,0x99,0x33,0x00},
	{0xcc,0x99,0x33,0x00},{0xff,0x99,0x33,0x00},{0x00,0xcc,0x33,0x00},{0x33,0xcc,0x33,0x00},
	{0x66,0xcc,0x33,0x00},{0x99,0xcc,0x33,0x00},{0xcc,0xcc,0x33,0x00},{0xff,0xcc,0x33,0x00},
	{0x33,0xff,0x33,0x00},{0x66,0xff,0x33,0x00},{0x99,0xff,0x33,0x00},{0xcc,0xff,0x33,0x00},
	{0xff,0xff,0x33,0x00},{0x00,0x00,0x66,0x00},{0x33,0x00,0x66,0x00},{0x66,0x00,0x66,0x00},
	{0x99,0x00,0x66,0x00},{0xcc,0x00,0x66,0x00},{0xff,0x00,0x66,0x00},{0x00,0x33,0x66,0x00},
	{0x33,0x33,0x66,0x00},{0x66,0x33,0x66,0x00},{0x99,0x33,0x66,0x00},{0xcc,0x33,0x66,0x00},
	{0xff,0x33,0x66,0x00},{0x00,0x66,0x66,0x00},{0x33,0x66,0x66,0x00},{0x66,0x66,0x66,0x00},
	{0x99,0x66,0x66,0x00},{0xcc,0x66,0x66,0x00},{0x00,0x99,0x66,0x00},{0x33,0x99,0x66,0x00},
	{0x66,0x99,0x66,0x00},{0x99,0x99,0x66,0x00},{0xcc,0x99,0x66,0x00},{0xff,0x99,0x66,0x00},
	{0x00,0xcc,0x66,0x00},{0x33,0xcc,0x66,0x00},{0x99,0xcc,0x66,0x00},{0xcc,0xcc,0x66,0x00},
	{0xff,0xcc,0x66,0x00},{0x00,0xff,0x66,0x00},{0x33,0xff,0x66,0x00},{0x99,0xff,0x66,0x00},
	{0xcc,0xff,0x66,0x00},{0xff,0x00,0xcc,0x00},{0xcc,0x00,0xff,0x00},{0x00,0x99,0x99,0x00},
	{0x99,0x33,0x99,0x00},{0x99,0x00,0x99,0x00},{0xcc,0x00,0x99,0x00},{0x00,0x00,0x99,0x00},
	{0x33,0x33,0x99,0x00},{0x66,0x00,0x99,0x00},{0xcc,0x33,0x99,0x00},{0xff,0x00,0x99,0x00},
	{0x00,0x66,0x99,0x00},{0x33,0x66,0x99,0x00},{0x66,0x33,0x99,0x00},{0x99,0x66,0x99,0x00},
	{0xcc,0x66,0x99,0x00},{0xff,0x33,0x99,0x00},{0x33,0x99,0x99,0x00},{0x66,0x99,0x99,0x00},
	{0x99,0x99,0x99,0x00},{0xcc,0x99,0x99,0x00},{0xff,0x99,0x99,0x00},{0x00,0xcc,0x99,0x00},
	{0x33,0xcc,0x99,0x00},{0x66,0xcc,0x66,0x00},{0x99,0xcc,0x99,0x00},{0xcc,0xcc,0x99,0x00},
	{0xff,0xcc,0x99,0x00},{0x00,0xff,0x99,0x00},{0x33,0xff,0x99,0x00},{0x66,0xcc,0x99,0x00},
	{0x99,0xff,0x99,0x00},{0xcc,0xff,0x99,0x00},{0xff,0xff,0x99,0x00},{0x00,0x00,0xcc,0x00},
	{0x33,0x00,0x99,0x00},{0x66,0x00,0xcc,0x00},{0x99,0x00,0xcc,0x00},{0xcc,0x00,0xcc,0x00},
	{0x00,0x33,0x99,0x00},{0x33,0x33,0xcc,0x00},{0x66,0x33,0xcc,0x00},{0x99,0x33,0xcc,0x00},
	{0xcc,0x33,0xcc,0x00},{0xff,0x33,0xcc,0x00},{0x00,0x66,0xcc,0x00},{0x33,0x66,0xcc,0x00},
	{0x66,0x66,0x99,0x00},{0x99,0x66,0xcc,0x00},{0xcc,0x66,0xcc,0x00},{0xff,0x66,0x99,0x00},
	{0x00,0x99,0xcc,0x00},{0x33,0x99,0xcc,0x00},{0x66,0x99,0xcc,0x00},{0x99,0x99,0xcc,0x00},
	{0xcc,0x99,0xcc,0x00},{0xff,0x99,0xcc,0x00},{0x00,0xcc,0xcc,0x00},{0x33,0xcc,0xcc,0x00},
	{0x66,0xcc,0xcc,0x00},{0x99,0xcc,0xcc,0x00},{0xcc,0xcc,0xcc,0x00},{0xff,0xcc,0xcc,0x00},
	{0x00,0xff,0xcc,0x00},{0x33,0xff,0xcc,0x00},{0x66,0xff,0x99,0x00},{0x99,0xff,0xcc,0x00},
	{0xcc,0xff,0xcc,0x00},{0xff,0xff,0xcc,0x00},{0x33,0x00,0xcc,0x00},{0x66,0x00,0xff,0x00},
	{0x99,0x00,0xff,0x00},{0x00,0x33,0xcc,0x00},{0x33,0x33,0xff,0x00},{0x66,0x33,0xff,0x00},
	{0x99,0x33,0xff,0x00},{0xcc,0x33,0xff,0x00},{0xff,0x33,0xff,0x00},{0x00,0x66,0xff,0x00},
	{0x33,0x66,0xff,0x00},{0x66,0x66,0xcc,0x00},{0x99,0x66,0xff,0x00},{0xcc,0x66,0xff,0x00},
	{0xff,0x66,0xcc,0x00},{0x00,0x99,0xff,0x00},{0x33,0x99,0xff,0x00},{0x66,0x99,0xff,0x00},
	{0x99,0x99,0xff,0x00},{0xcc,0x99,0xff,0x00},{0xff,0x99,0xff,0x00},{0x00,0xcc,0xff,0x00},
	{0x33,0xcc,0xff,0x00},{0x66,0xcc,0xff,0x00},{0x99,0xcc,0xff,0x00},{0xcc,0xcc,0xff,0x00},
	{0xff,0xcc,0xff,0x00},{0x33,0xff,0xff,0x00},{0x66,0xff,0xcc,0x00},{0x99,0xff,0xff,0x00},
	{0xcc,0xff,0xff,0x00},{0xff,0x66,0x66,0x00},{0x66,0xff,0x66,0x00},{0xff,0xff,0x66,0x00},
	{0x66,0x66,0xff,0x00},{0xff,0x66,0xff,0x00},{0x66,0xff,0xff,0x00},{0xa5,0x00,0x21,0x00},
	{0x5f,0x5f,0x5f,0x00},{0x77,0x77,0x77,0x00},{0x86,0x86,0x86,0x00},{0x96,0x96,0x96,0x00},
	{0xcb,0xcb,0xcb,0x00},{0xb2,0xb2,0xb2,0x00},{0xd7,0xd7,0xd7,0x00},{0xdd,0xdd,0xdd,0x00},
	{0xe3,0xe3,0xe3,0x00},{0xea,0xea,0xea,0x00},{0xff,0xfb,0xf0,0x00},{0xa0,0xa0,0xa4,0x00},
	{0x80,0x80,0x80,0x00},{0xff,0x00,0x00,0x00},{0x00,0xff,0x00,0x00},{0xff,0xff,0x00,0x00},
	{0x00,0x00,0xff,0x00},{0xff,0x00,0xff,0x00},{0x00,0xff,0xff,0x00},{0xff,0xff,0xff,0x00}
};

HPALETTE WINAPI extGDICreatePalette(CONST LOGPALETTE *plpal)
{
	HPALETTE ret;
	ApiName("CreatePalette");

	OutTraceGDI("%s: plpal=%#x version=%#x NumEntries=%d\n", ApiRef, plpal, plpal->palVersion, plpal->palNumEntries);
	if(IsDebugGDI) dxw.DumpPalette(plpal->palNumEntries, (LPPALETTEENTRY)plpal->palPalEntry);
	dxw.SetDIBColors(plpal);
	ret=(*pGDICreatePalette)(plpal);
	OutTraceGDI("%s: hPalette=%#x\n", ApiRef, ret);
	return ret;
}

HPALETTE hDesktopPalette=NULL;

HPALETTE WINAPI extSelectPalette(HDC hdc, HPALETTE hpal, BOOL bForceBackground)
{
	HPALETTE ret;
	ApiName("SelectPalette");

	OutTraceGDI("%s: hdc=%#x hpal=%#x ForceBackground=%#x\n", ApiRef, hdc, hpal, bForceBackground);

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_EMULATED:
				if(hdc==dxw.RealHDC) hdc= dxw.VirtualHDC; 
				break;
			//case GDIMODE_SHAREDDC:
			//case GDIMODE_STRETCHED: 
			//	break;
		}
	}

	ret=(*pGDISelectPalette)(hdc, hpal, bForceBackground);

	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

static DWORD dwLastRealizeTime = 0;
static HPALETTE hLastRealizedPalette = 0;

BOOL WINAPI extAnimatePalette(HPALETTE hpal, UINT iStartIndex, UINT cEntries, const PALETTEENTRY *ppe)
{
	// Invoked by @#@ "Pharaoh's Ascent 1.4"
	// Used by @#@ "Yu-No"
	// used in "Ecco the Dolphin"
	BOOL ret;
	ApiName("AnimatePalette");

	OutTraceGDI("%s: hpal=%#x startindex=%d entries=%d\n", ApiRef, hpal, iStartIndex, cEntries);
	if(IsDebugGDI) dxw.DumpPalette(cEntries, (LPPALETTEENTRY)ppe+iStartIndex);

	ret=(*pAnimatePalette)(hpal, iStartIndex, cEntries, ppe);

	if(dxw.IsEmulated && (dxw.dwFlags6 & SYNCPALETTE) && (hpal == hLastRealizedPalette)){
		PALETTEENTRY PalEntries[256];
		OutTraceDW("%s: AnimatePalette on hpal=%#x ret=%d\n", ApiRef, hpal, ret);
		memcpy(&PalEntries[iStartIndex], ppe, cEntries * sizeof(PALETTEENTRY));
		mySetPalette(iStartIndex, cEntries, PalEntries); 
		if(IsDebugDW && cEntries) dxw.DumpPalette(cEntries, PalEntries);  
		ret=cEntries;

		if(dxw.IsFullScreen() && (dxw.dwFlags3 & REFRESHONREALIZE)){
			HWND hwnd = dxw.GethWnd();
			if(!dxw.IsRealDesktop(hwnd)){
				(*pRedrawWindow)(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_NOFRAME);
			}
		}
	}

	if(!ret) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
		//ret = TRUE;
		if (dxw.IsEmulated) ret = cEntries;
	}

	return ret;
}

UINT WINAPI extRealizePalette(HDC hdc) 
{
	UINT ret;
	ApiName("RealizePalette");

	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	if((dxw.IsEmulated) && (dxw.dwFlags6 & SYNCPALETTE)){
		PALETTEENTRY PalEntries[256];
		UINT cEntries;
		HPALETTE hPal;
		if(bFlippedDC) hdc = hFlippedDC;
		ret=(*pGDIRealizePalette)(hdc);
		OutTraceDW("%s: RealizePalette on hdc=%#x ret=%d\n", ApiRef, hdc, ret);
		hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
		hPal = (*pGDISelectPalette)(hdc, hPal, FALSE);
		hLastRealizedPalette = hPal;
		(*pGDISelectPalette)(hdc, hPal, FALSE);
		cEntries=(*pGetPaletteEntries)(hPal, 0, 256, PalEntries);
		OutTraceDW("%s: GetPaletteEntries on hdc=%#x ret=%d\n", ApiRef, hdc, cEntries);
		mySetPalette(0, 256, PalEntries); 
		if(IsDebugDW && cEntries) dxw.DumpPalette(cEntries, PalEntries);  
		ret=cEntries;

		// refresh on resize conditioned to configuration flag. It should be possible, though, to predict whether
		// a window refresh is needed or not given the conditions. To be done ....
		if(dxw.IsFullScreen() && (dxw.dwFlags3 & REFRESHONREALIZE)){
			DWORD now = (*pGetTickCount)();
			if((now - dwLastRealizeTime) > 10){
				HWND hwnd = dxw.GethWnd();
				if(!dxw.IsRealDesktop(hwnd)){
					(*pRedrawWindow)(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_NOFRAME);
					dwLastRealizeTime = now;
				}
			}
		}
	}
	else
		ret=(*pGDIRealizePalette)(hdc);

	OutTraceGDI("%s: hdc=%#x nEntries=%d\n", ApiRef, hdc, ret);


	return ret;
}

UINT WINAPI extGetSystemPaletteEntries(HDC hdc, UINT iStartIndex, UINT nEntries, LPPALETTEENTRY lppe)
{
	int ret;
	ApiName("GetSystemPaletteEntries");

	OutTraceGDI("%s: hdc=%#x start=%d num=%d\n", ApiRef, hdc, iStartIndex, nEntries);
	ret=(*pGDIGetSystemPaletteEntries)(hdc, iStartIndex, nEntries, lppe);
	OutTraceGDI("%s: ret=%d\n", ApiRef, ret);
	if((ret == 0) && (dxw.IsEmulated) && (dxw.dwFlags6 & SYNCPALETTE)) {
		// use static default data...
		for(UINT idx=0; idx<nEntries; idx++) lppe[idx]=DefaultSystemPalette[iStartIndex+idx]; 
		ret = nEntries;
		OutTraceDW("%s: FIXED ret=%d\n", ApiRef, ret);
	}
	if(IsDebugDW) dxw.DumpPalette(nEntries, lppe);
	return ret;
}

UINT WINAPI extSetSystemPaletteUse(HDC hdc, UINT uUsage)
{
	//BOOL res;
	ApiName("SetSystemPaletteUse");
	OutTraceGDI("%s: hdc=%#x Usage=%#x(%s)\n", ApiRef, hdc, uUsage, ExplainPaletteUse(uUsage));
	return SYSPAL_NOSTATIC256;
}

// to be verified: useful ? correct ??
static PALETTEENTRY MapPaletteColor(DWORD color)
{
	PALETTEENTRY pe;
	pe.peRed   = (BYTE)(color & 0x0000FF);
	pe.peGreen = (BYTE)((color & 0x00FF00) >> 8);
	pe.peBlue  = (BYTE)((color & 0xFF0000) >> 16);
	return pe;
}

UINT WINAPI extGetPaletteEntries(HPALETTE hpal, UINT iStartIndex, UINT nEntries, LPPALETTEENTRY lppe)
{
	UINT res;
	ApiName("GetPaletteEntries");
	OutTraceGDI("%s: hpal=%#x iStartIndex=%d nEntries=%d lppe=%#x\n", ApiRef, hpal, iStartIndex, nEntries, lppe);
	res=(*pGetPaletteEntries)(hpal, iStartIndex, nEntries, lppe);
	OutTraceGDI("%s: res-nEntries=%d\n", ApiRef, res);
	if((res < nEntries) && (dxw.dwFlags6 & SYNCPALETTE) && lppe) { 
		if((iStartIndex + nEntries) > 256) nEntries = 256 - iStartIndex;
		res = nEntries;
		for(UINT i=0; i<nEntries; i++){
			DWORD color = PaletteEntries[iStartIndex + i];
			*lppe = MapPaletteColor(color);
			lppe++;
		}
		OutTraceDW("%s: faking missing entries=%d\n", ApiRef, res);
	}
	// GDI Palette applied to ddraw: needed to color the gameplay 3D screen of "Hyperblade".
	// commented: it crashes @#@ "Wizardry GOLD"
	//if ((dxw.IsEmulated) && (dxw.dwFlags6 & SYNCPALETTE)) mySetPalette(0, nEntries, lppe); 
	if(IsDebugDW && res && lppe) dxw.DumpPalette(res, lppe);
	return res;
}

UINT WINAPI extSetPaletteEntries(HPALETTE hpal, UINT iStart, UINT cEntries, const PALETTEENTRY *lppe)
{
	UINT ret;
	ApiName("SetPaletteEntries");
	OutTraceGDI("%s: hpal=%#x start=%d entries=%d\n", ApiRef, hpal, iStart, cEntries);
	ret = (*pSetPaletteEntries)(hpal, iStart, cEntries, lppe);
	// the only purpose of hooking this call is the fact that in windowed mode a palette update
	// does not flush the HDC updates to the device like in fullscreen (Win98?) mode.
	// if(dxw.IsFullScreen()) (*pInvalidateRect)(dxw.GethWnd(), NULL, FALSE);
	OutTraceGDI("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

UINT WINAPI extGetSystemPaletteUse(HDC hdc)
{
	UINT res;
	ApiName("GetSystemPaletteUse");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);
	res=(*pGetSystemPaletteUse)(hdc);

	if((res == SYSPAL_ERROR) && (dxw.dwFlags6 & SYNCPALETTE)) res = SYSPAL_NOSTATIC;

	OutTraceGDI("%s: res=%#x(%s)\n", ApiRef, res, ExplainPaletteUse(res));
	return res;
}

BOOL WINAPI extUpdateColors(HDC hdc)
{
	BOOL res;
	ApiName("UpdateColors");
	res=(*pUpdateColors)(hdc);
	OutTraceGDI("%s: hdc=%#x ret=%d\n", ApiRef, hdc, res);
	if (dxw.IsEmulated) res = TRUE;
	return res;
}

BOOL WINAPI extUnrealizeObject(HGDIOBJ h)
{
	BOOL res;
	ApiName("UnrealizeObject");

	OutTraceGDI("%s: obj=%#x\n",ApiRef, h);

	if(dxw.IsEmulated){
		res = TRUE;
	}
	else {
		res = (*pUnrealizeObject)(h);
	}
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

UINT WINAPI extGetNearestPaletteIndex(HPALETTE h, COLORREF color)
{
	UINT res;
	ApiName("GetNearestPaletteIndex");

	OutTraceGDI("%s: hpal=%#x color=%#x\n", ApiRef, h, color);
	if(dxw.dwFlags6 & SYNCPALETTE){
		DWORD best = 0xFFFFFFFF;
		res = 0;
		for (int i=0; i<256; i++){
			int diff_red = (int)(color & 0x000000FF) - (int)(PaletteEntries[i] & 0x000000FF);
			int diff_green = (int)((color & 0x0000FF00)>>8) - (int)((PaletteEntries[i] & 0x0000FF00)>>8);
			int diff_blue = (int)((color & 0x00FF0000)>>16) - (int)((PaletteEntries[i] & 0x00FF0000)>>16);
			DWORD diff2 = (diff_red * diff_red) + (diff_green * diff_green) + (diff_blue * diff_blue);
			if(diff2 < best){
				res = i;
				best = diff2;
			}
		}
		OutTraceGDI("%s: res(idx)=%d color=%#x\n", ApiRef, res, PaletteEntries[res]);
	}
	else{
		res = (*pGetNearestPaletteIndex)(h, color);
		OutTraceGDI("%s: res(idx)=%d\n", ApiRef, res);
	}
	return res;
}

/* --------------------------------------------------------------------------- */
/*     Palette handling - END                                                  */
/* --------------------------------------------------------------------------- */

HDC WINAPI extGDICreateDCA(LPSTR lpszDriver, LPSTR lpszDevice, LPSTR lpszOutput, CONST DEVMODE *lpdvmInit)
{
	HDC WinHDC, RetHDC;
	ApiName("CreateDCA");
	OutTraceGDI("%s: Driver=%s Device=%s Output=%s InitData=%#x\n", ApiRef,
		lpszDriver?lpszDriver:"(NULL)", lpszDevice?lpszDevice:"(NULL)", lpszOutput?lpszOutput:"(NULL)", lpdvmInit);

	if ((!lpszDriver || !strncmp(lpszDriver,"DISPLAY",7)) && dxw.GethWnd()) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_NONE:
			case GDIMODE_STRETCHED:
			case GDIMODE_EMULATED:
			case GDIMODE_SHAREDDC:
			default:
				OutTraceDW("%s: returning window surface DC\n", ApiRef);
				WinHDC=(*pGDIGetDC)(dxw.GethWnd());
				RetHDC=(*pGDICreateCompatibleDC)(WinHDC);
				(*pGDIReleaseDC)(dxw.GethWnd(), WinHDC);
				break;
		}
	}
	else{
		RetHDC=(*pGDICreateDCA)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);
	}
	if(RetHDC){
		OutTraceGDI("%s: returning HDC=%#x\n", ApiRef, RetHDC);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return RetHDC;
}

HDC WINAPI extGDICreateDCW(LPWSTR lpszDriver, LPWSTR lpszDevice, LPWSTR lpszOutput, CONST DEVMODE *lpdvmInit)
{
	HDC WinHDC, RetHDC;
	ApiName("CreateDCW");
	OutTraceGDI("%s: Driver=%ls Device=%ls Output=%ls InitData=%#x\n", ApiRef,
		lpszDriver?lpszDriver:L"(NULL)", lpszDevice?lpszDevice:L"(NULL)", lpszOutput?lpszOutput:L"(NULL)", lpdvmInit);

	if ((!lpszDriver || !wcsncmp(lpszDriver,L"DISPLAY",7)) && dxw.GethWnd()){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_NONE:
			case GDIMODE_STRETCHED:
			case GDIMODE_EMULATED:
			case GDIMODE_SHAREDDC:
			default:
				OutTraceDW("%s: returning window surface DC\n", ApiRef);
				WinHDC=(*pGDIGetDC)(dxw.GethWnd());
				RetHDC=(*pGDICreateCompatibleDC)(WinHDC);
				(*pGDIReleaseDC)(dxw.GethWnd(), WinHDC);
				break;
		}
	}
	else{
		RetHDC=(*pGDICreateDCW)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);
	}
	if(RetHDC)
		OutTraceGDI("%s: returning HDC=%#x\n", ApiRef, RetHDC);
	else
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return RetHDC;
}

#ifdef TRACEALL
HDC WINAPI extCreateICA(LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, const DEVMODEA *lpdvmInit)
{
	HDC RetHDC;
	ApiName("CreateICA");
	OutTraceGDI("%s: Driver=%s Device=%s Output=%s InitData=%#x\n", ApiRef,
		lpszDriver?lpszDriver:"(NULL)", lpszDevice?lpszDevice:"(NULL)", lpszOutput?lpszOutput:"(NULL)", lpdvmInit);

	RetHDC = (*pCreateICA)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);

	OutTraceGDI("%s: ret=%#x\n", ApiRef, RetHDC);
	return RetHDC;
}
#endif // TRACEALL

#ifdef TRACEALL
HDC WINAPI extCreateICW(LPCWSTR lpszDriver, LPCWSTR lpszDevice, LPCWSTR lpszOutput, const DEVMODEW *lpdvmInit)
{
	HDC RetHDC;
	ApiName("CreateICW");
	OutTraceGDI("%s: Driver=%ls Device=%ls Output=%ls InitData=%#x\n", ApiRef, 
		lpszDriver?lpszDriver:L"(NULL)", lpszDevice?lpszDevice:L"(NULL)", lpszOutput?lpszOutput:L"(NULL)", lpdvmInit);

	RetHDC = (*pCreateICW)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);

	OutTraceGDI("%s: ret=%#x\n", ApiRef, RetHDC);
	return RetHDC;
}
#endif // TRACEALL

HDC WINAPI extGDICreateCompatibleDC(HDC hdc)
{
	// v2.03.75: fixed dc leakage that crashed "Mechwarrior 3"
	HDC RetHdc;
	ApiName("CreateCompatibleDC");
	DWORD LastError;
	BOOL bSwitchedToMainWin = FALSE;

	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);
	// n.b. useful? hdc == 0 doesn't imply the desktop but rather the current window
	// from MSDN:
	// hdc[in] A handle to an existing DC. If this handle is NULL the function creates
	// a memory DC compatible with the application's current screen.
	if(hdc==0 || (WindowFromDC(hdc)==0)) { // v2.03.99: Star Trek Armada
		//switch(dxw.GDIEmulationMode){
		//	//case GDIMODE_ASYNCDC:
		//		//hdc = dxw.AcquireEmulatedDC(dxw.GethWnd());
		//		//break;
		//	default:
				hdc=(*pGDIGetDC)(dxw.GethWnd()); // potential DC leakage
				bSwitchedToMainWin = TRUE;
				//break;
		//}
		OutTraceDW("%s: duplicating win HDC hWnd=%#x\n", ApiRef, dxw.GethWnd()); 
	}

	// eliminated error message for errorcode 0.
	SetLastError(0);
	RetHdc=(*pGDICreateCompatibleDC)(hdc);
	if(bSwitchedToMainWin) (*pGDIReleaseDC)(dxw.GethWnd(), hdc); // fixed DC leakage
	LastError=GetLastError();
	if(LastError == 0){
		OutTraceGDI("%s: returning HDC=%#x\n", ApiRef, RetHdc);
	}
	else{
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, LastError);
	}

	return RetHdc;
}

/*-------------------------------------------*/

//#undef OutDebugGDI
//#define OutDebugGDI OutTrace
//#undef OutDebugDW
//#define OutDebugDW OutTrace


BOOL WINAPI extGDIBitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop)
{
	BOOL res;
	ApiName("BitBlt");
	BOOL IsToScreen;
	BOOL IsFromScreen;
	BOOL IsDCLeakageSrc = FALSE;
	BOOL IsDCLeakageDest = FALSE;
	int Flux;
	int nOrigHeight = nHeight;

	OutTraceGDI("%s: HDC=%#x(type=%s) nXDest=%d nYDest=%d nWidth=%d nHeight=%d hdcSrc=%#x(type=%s) nXSrc=%d nYSrc=%d dwRop=%#x(%s)\n", 
		ApiRef, hdcDest, GetObjectTypeStr(hdcDest), nXDest, nYDest, nWidth, nHeight, 
		hdcSrc, GetObjectTypeStr(hdcSrc), nXSrc, nYSrc, dwRop, ExplainROP(dwRop));

	OutDebugGDI("%s: DEBUG FullScreen=%#x target hdctype=%#x(%s) hwnd=%#x\n", 
		ApiRef, dxw.IsFullScreen(), (*pGetObjectType)(hdcDest), ExplainDCType((*pGetObjectType)(hdcDest)), WindowFromDC(hdcDest));

	if(dxw.dwDFlags2 & DUMPBLITSRC){
		DumpHDC(hdcSrc, nXSrc, nYSrc, nWidth, nHeight);
		DumpHDC(hdcDest, nXSrc, nYSrc, nWidth, nHeight);
	}

	if(dxw.dwFlags5 & MESSAGEPUMP) dxw.MessagePump();

	// beware: HDC could refer to screen DC that are written directly on screen, or memory DC that will be scaled to
	// the screen surface later on, on ReleaseDC or ddraw Blit / Flip operation. Scaling of rect coordinates is 
	// needed only in the first case, and must be avoided on the second, otherwise the image would be scaled twice!

	if(hdcDest == NULL){
		// happens in Reah, hdc is NULL despite the fact that BeginPaint returns a valid DC. Too bad, we recover here ...
		hdcDest = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc dest=NULL->%#x\n", ApiRef, hdcDest);
		IsDCLeakageDest = TRUE;
	}
	if(hdcSrc == NULL){
		hdcSrc = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc src=NULL->%#x\n", ApiRef, hdcSrc);
		IsDCLeakageSrc = TRUE;
	}

	if((dxw.dwFlags10 & CHAOSOVERLORDSFIX) && (dwRop == SRCAND)) dwRop = SRCINVERT; // ok

	IsToScreen=(OBJ_DC == (*pGetObjectType)(hdcDest));
	IsFromScreen=(OBJ_DC == (*pGetObjectType)(hdcSrc));

	if(IsToScreen && (dxw.dwFlags1 & SUPPRESSCLIPPING)) SelectClipRgn(hdcDest, NULL);

	// v2.05.88 fix. To be completed??
	switch(dwRop){
		case BLACKNESS: 
		case WHITENESS: 
			IsFromScreen = FALSE;
			break;
	}
	Flux = (IsToScreen ? 1 : 0) + (IsFromScreen ? 2 : 0); 

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC){
		res=(*pGDIBitBlt)(lpADC->GetDC(hdcDest), nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
		lpADC->ReleaseDC();
		return res;
	}

	OutDebugDW("%s: flux=%d fullscreen=%d\n", ApiRef, Flux, dxw.IsFullScreen());
	// v2.04.30 fixed handlers leakage 
	if (IsToScreen && (dxw.dwDFlags & NOGDIBLT)) {
		if(IsDCLeakageSrc) (*pGDIReleaseDC)(dxw.GethWnd(), hdcSrc);
		if(IsDCLeakageDest) (*pGDIReleaseDC)(dxw.GethWnd(), hdcDest);
		return TRUE;
	} 	

#ifndef DXW_NOTRACES
	if(IsToScreen && (dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(hdcSrc, nXSrc, nYSrc, nWidth, nHeight);
#endif // DXW_NOTRACES

	if(IsToScreen) dxw.HandleFPS(); // handle refresh delays

	if(dxw.IsFullScreen()){
		//int nWSrc, nHSrc, 
		int nWDest, nHDest;
		int prevMode;
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				switch(Flux){
					case 0: // memory to memory
						res=(*pGDIBitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
						break;
					case 1: // memory to screen
					case 3: // screen to screen
						sdc.SetOrigin(nXSrc, nYSrc);
						sdc.GetPrimaryDC(hdcDest, hdcSrc);
						res=(*pGDIBitBlt)(sdc.GetHdc(), nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
						sdc.PutPrimaryDC(hdcDest, TRUE, nXDest, nYDest, nWidth, nHeight);
						break;
					case 2: // screen to memory
						sdc.GetPrimaryDC(hdcSrc);
						res=(*pGDIBitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, sdc.GetHdc(), nXSrc, nYSrc, dwRop);
						sdc.PutPrimaryDC(hdcSrc, TRUE, nXDest, nYDest, nWidth, nHeight);
						break;
				}
				break;
			case GDIMODE_STRETCHED: 
				nWDest= nWidth;
				nHDest= nHeight;
				prevMode = SetStretchBltMode(hdcDest, (dxw.dwFlags15 & FORCEHALFTONE) ? HALFTONE : COLORONCOLOR);
				_if((!prevMode) || (prevMode==ERROR_INVALID_PARAMETER)) {
					OutErrorGDI("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
				}
				switch(Flux){
					case 1: // memory to screen
						// v1.03.58: BitBlt can blitfrom negative coordinates, StretchBlt can't!
						if(dxw.isScaled){
							if(nXDest < 0){
								int nXshift = -nXDest;
								nXDest = 0;
								nXSrc += nXshift;
								nWidth -= nXshift;
								nWDest -= nXshift;
							}
							if(nYDest < 0){
								int nYshift = -nYDest;
								nYDest = 0;
								nYSrc += nYshift;
								nHeight -= nYshift;
								nHDest -= nYshift;
							}
							dxw.MapClient(&nXDest, &nYDest, &nWDest, &nHDest);
						}
						OutDebugDW("%s: FIXED dest pos=(%d,%d) size=(%d,%d)\n", ApiRef, nXDest, nYDest, nWDest, nHDest);
						if (dxw.dwFlags15 & FORCEHALFTONETINY)
							res=dxwStretchBlt(hdcDest, nXDest, nYDest, nWDest, nHDest, hdcSrc, nXSrc, nYSrc, nWidth, nHeight, dwRop);
						else
							res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWDest, nHDest, hdcSrc, nXSrc, nYSrc, nWidth, nHeight, dwRop);
						break;
					case 2: // screen to memory
						// v2.06.13 fix:
						//dxw.MapClient(&nXSrc, &nYSrc, &nWidth, &nHeight);
						dxw.UnmapClient(&nXDest, &nYDest, &nWDest, &nHDest);
						res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWDest, nHDest, hdcSrc, nXSrc, nYSrc, nWidth, nHeight, dwRop);
						break;
					default:
						// v2.04.32: avoid StretchBlt for intra-memory or intra-video operations!
						// Fixes "Avernum 3" transparency problems 
						res=(*pGDIBitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
						break;
				}
				if((prevMode) && (prevMode!=ERROR_INVALID_PARAMETER)) SetStretchBltMode(hdcDest, prevMode);
				OutDebugDW("%s: DEBUG DC flux=%d dest=%#x(%s) pos=(%d,%d) size=(%d,%d) src=%#x(%s) pos=(%d,%d)\n", 
					ApiRef, Flux,
					hdcDest, IsToScreen ? "VID" : "MEM", nXDest, nYDest, nWidth, nHeight,
					hdcSrc, IsFromScreen ? "VID" : "MEM", nXSrc, nYSrc);
				break;
			case GDIMODE_EMULATED:
				if (hdcSrc==dxw.RealHDC) { 
					hdcSrc=dxw.VirtualHDC;
				}
				if (hdcDest==dxw.RealHDC) { 
					hdcDest=dxw.VirtualHDC;
				}
			    res=(*pGDIBitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
				OutDebugDW("%s: DEBUG emulated hdc dest=%#x->%#x\n", ApiRef, dxw.RealHDC, hdcDest);
				break;
			default:
				res=(*pGDIBitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
				break;
		}
	}
	else {
		res=(*pGDIBitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
	}

	if(IsDCLeakageSrc) (*pGDIReleaseDC)(dxw.GethWnd(), hdcSrc);
	if(IsDCLeakageDest) (*pGDIReleaseDC)(dxw.GethWnd(), hdcDest);
	if(res && IsToScreen) {
		dxw.ShowOverlay(hdcDest);
		if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdcDest, FALSE, COLORMARKERBITBLT, nXDest, nYDest, nWidth, nHeight);
	}
	_if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);

	if(res && (dxw.dwFlags14 & FIXEDBITBLT)){
		res = nOrigHeight;
		OutTraceDW("%s: FIXEDBITBLT ret=%d\n", ApiRef, res);
	}

	return res;
}

BOOL WINAPI extGDIStretchBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, 
							 HDC hdcSrc, int nXSrc, int nYSrc, int nWSrc, int nHSrc, DWORD dwRop)
{
	BOOL res;
	ApiName("StretchBlt");
	BOOL IsToScreen;
	BOOL IsFromScreen;
	BOOL IsDCLeakageSrc = FALSE;
	BOOL IsDCLeakageDest = FALSE;
	int Flux;

	OutTraceGDI("%s: HDC=%#x(type=%s) nXDest=%d nYDest=%d nWidth=%d nHeight=%d hdcSrc=%#x(type=%s) nXSrc=%d nYSrc=%d nWSrc=%d nHSrc=%d dwRop=%#x(%s)\n", 
		ApiRef, hdcDest, GetObjectTypeStr(hdcDest), nXDest, nYDest, nWidth, nHeight, hdcSrc, GetObjectTypeStr(hdcSrc), nXSrc, nYSrc, nWSrc, nHSrc, dwRop, ExplainROP(dwRop));

	OutDebugGDI("%s: DEBUG FullScreen=%#x target hdctype=%#x(%s) hwnd=%#x\n", 
		ApiRef, dxw.IsFullScreen(), (*pGetObjectType)(hdcDest), ExplainDCType((*pGetObjectType)(hdcDest)), WindowFromDC(hdcDest));

	if(dxw.dwDFlags2 & DUMPBLITSRC){
		DumpHDC(hdcSrc, nXSrc, nYSrc, nWidth, nHeight);
		DumpHDC(hdcDest, nXSrc, nYSrc, nWidth, nHeight);
	}

	if(dxw.GDIEmulationMode == GDIMODE_EMULATED){
		if (hdcDest==dxw.RealHDC) hdcDest=dxw.VirtualHDC;
		OutDebugDW("%s: DEBUG emulated hdc dest=%#x->%#x\n", ApiRef, dxw.RealHDC, hdcDest);
	}

	if(dxw.dwFlags5 & MESSAGEPUMP) dxw.MessagePump();

	if(hdcDest == NULL){
		// happens in Reah, hdc is NULL despite the fact that BeginPaint returns a valid DC. Too bad, we recover here ...
		hdcDest = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc dest=NULL->%#x\n", ApiRef, hdcDest);
		IsDCLeakageDest = TRUE;
	}
	if(hdcSrc == NULL){
		hdcSrc = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc src=NULL->%#x\n", ApiRef, hdcSrc);
		IsDCLeakageSrc = TRUE;
	}

	IsToScreen=(OBJ_DC == (*pGetObjectType)(hdcDest));
	IsFromScreen=(OBJ_DC == (*pGetObjectType)(hdcSrc));
	Flux = (IsToScreen ? 1 : 0) + (IsFromScreen ? 2 : 0); 
	if (IsToScreen && (dxw.dwDFlags & NOGDIBLT)) return TRUE;

	if(IsToScreen && (dxw.dwFlags1 & SUPPRESSCLIPPING)) SelectClipRgn(hdcDest, NULL);

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC){
		//lpADC->AcquireDC();
		res=(*pGDIStretchBlt)(lpADC->GetDC(hdcDest), nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
		lpADC->ReleaseDC();
		return res;
	}

#ifndef DXW_NOTRACES
	if(IsToScreen && (dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(hdcSrc, nXSrc, nYSrc, nWidth, nHeight);
#endif // DXW_NOTRACES

	if(IsToScreen) dxw.HandleFPS(); // handle refresh delays

	if(dxw.dwFlags15 & FORCEHALFTONE) {
		res=SetStretchBltMode(hdcDest, HALFTONE);
		_if((!res) || (res==ERROR_INVALID_PARAMETER)) 
			OutErrorGDI("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
	}
	// v2.06.13: fix, scaling must occur in more cases
	if(dxw.IsFullScreen() && (hdcDest != hdcSrc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				switch(Flux){
					case 0: // memory to memory
						res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
						break;
					case 1: // memory to screen
					case 3: // screen to screen
						sdc.GetPrimaryDC(hdcDest, hdcSrc);
						sdc.SetOrigin(nXSrc, nYSrc);
						res=(*pGDIStretchBlt)(sdc.GetHdc(), nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
						sdc.PutPrimaryDC(hdcDest, TRUE, nXDest, nYDest, nWidth, nHeight);
						break;
					case 2: // screen to memory using virtual screen
						sdc.GetPrimaryDC(hdcSrc);
						res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, sdc.GetHdc(), nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
						sdc.PutPrimaryDC(hdcSrc, TRUE, nXSrc, nYSrc, nWSrc, nHSrc);
						break;
				}
				break;
			case GDIMODE_STRETCHED: {
				int prevMode;
				prevMode = SetStretchBltMode(hdcDest, (dxw.dwFlags15 & FORCEHALFTONE) ? HALFTONE : COLORONCOLOR);
				_if((!prevMode) || (prevMode==ERROR_INVALID_PARAMETER)) {
					OutErrorGDI("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
				}
				switch(Flux){
					case 1: // memory to screen
						dxw.MapClient(&nXDest, &nYDest, &nWidth, &nHeight);
						break;
					case 2: // screen to memory
						// v2.06.13 fix:
						dxw.MapClient(&nXSrc, &nYSrc, &nWSrc, &nHSrc);
						break;
					default:
						break;
				}
				if (dxw.dwFlags15 & FORCEHALFTONETINY){
					res=dxwStretchBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
				}
				else {
					res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
				}
				if((prevMode) && (prevMode!=ERROR_INVALID_PARAMETER)) SetStretchBltMode(hdcDest, prevMode);
				OutDebugDW("%s: DEBUG DC flux=%d dest=%#x(%s) pos=(%d,%d) size=(%d,%d) src=%#x(%s) pos=(%d,%d) size=(%d,%d)\n", 
					ApiRef, Flux,
					hdcDest, IsToScreen ? "VID" : "MEM", nXDest, nYDest, nWidth, nHeight,
					hdcSrc, IsFromScreen ? "VID" : "MEM", nXSrc, nYSrc, nWSrc, nHSrc);
				}
				break;
			//case GDIMODE_ASYNCDC:
			//	res=(*pGDIStretchBlt)(dxw.VirtualHDC, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
			//	dxwGDIAsyncFlush();	
			//	break;
			case GDIMODE_EMULATED:
				if (hdcDest==dxw.RealHDC) { 
					hdcDest=dxw.VirtualHDC;
				}
				if (hdcSrc==dxw.RealHDC) { 
					hdcDest=dxw.VirtualHDC;
				}
				// fall through
			default:
				res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
				break;
		}
	}
	else {
		res=(*pGDIStretchBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, dwRop);
	}

	//if(dxw.dwDFlags2 & EXPERIMENTAL6){
	//	DumpFullHDC("hdcSrc", hdcSrc);
	//	DumpFullHDC("hdcDest", hdcDest);
	//}

	if(IsDCLeakageSrc) (*pGDIReleaseDC)(dxw.GethWnd(), hdcSrc);
	if(IsDCLeakageDest) (*pGDIReleaseDC)(dxw.GethWnd(), hdcDest);
	if(res && IsToScreen) {
		dxw.ShowOverlay(hdcDest);
		if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdcDest, FALSE, COLORMARKERSTRETCHBLT, nXDest, nYDest, nWidth, nHeight);
	}
	_if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
	return res;
}

BOOL WINAPI extGDIPatBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, DWORD dwRop)
{
	BOOL res;
	ApiName("PatBlt");
	BOOL IsToScreen;
	BOOL IsDCLeakageDest = FALSE;

	OutTraceGDI("%s: HDC=%#x(type=%s) nXDest=%d nYDest=%d nWidth=%d nHeight=%d dwRop=%#x(%s)\n", 
		ApiRef, hdcDest, GetObjectTypeStr(hdcDest), nXDest, nYDest, nWidth, nHeight, dwRop, ExplainROP(dwRop));

	OutDebugGDI("%s: DEBUG FullScreen=%#x target hdctype=%#x(%s) hwnd=%#x\n", 
		ApiRef, dxw.IsFullScreen(), (*pGetObjectType)(hdcDest), ExplainDCType((*pGetObjectType)(hdcDest)), WindowFromDC(hdcDest));

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC){
		res=(*pGDIPatBlt)(lpADC->GetDC(hdcDest),  nXDest, nYDest, nWidth, nHeight, dwRop);
		lpADC->ReleaseDC();
		_if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return res;
	}

	if(dxw.GDIEmulationMode == GDIMODE_EMULATED){
		if (hdcDest==dxw.RealHDC) hdcDest=dxw.VirtualHDC;
		OutDebugDW("%s: DEBUG emulated hdc dest=%#x->%#x\n", ApiRef, dxw.RealHDC, hdcDest);
	}

	if(hdcDest == NULL){
		// happens in Reah, hdc is NULL despite the fact that BeginPaint returns a valid DC. Too bad, we recover here ...
		hdcDest = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc dest=NULL->%#x\n", ApiRef, hdcDest);
		IsDCLeakageDest = TRUE;
	}

	IsToScreen=(OBJ_DC == (*pGetObjectType)(hdcDest));

	if (IsToScreen && (dxw.dwDFlags & NOGDIBLT)) return TRUE;

	if(IsToScreen && (dxw.dwFlags1 & SUPPRESSCLIPPING)) SelectClipRgn(hdcDest, NULL);

	if(dxw.dwFlags15 & FORCEHALFTONE) {
		res=SetStretchBltMode(hdcDest, HALFTONE);
		_if((!res) || (res==ERROR_INVALID_PARAMETER)) 
			OutErrorGDI("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
	}

	if(dxw.IsToRemap(hdcDest)) {

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdcDest);
				res=(*pGDIPatBlt)(sdc.GetHdc(),  nXDest, nYDest, nWidth, nHeight, dwRop);
				sdc.PutPrimaryDC(hdcDest, TRUE, nXDest, nYDest, nWidth, nHeight);
				return res;
				break;
			case GDIMODE_STRETCHED: {
				int nWDest, nHDest;
				nWDest= nWidth;
				nHDest= nHeight;
				dxw.MapClient(&nXDest, &nYDest, &nWDest, &nHDest);
				res=(*pGDIPatBlt)(hdcDest, nXDest, nYDest, nWDest, nHDest, dwRop);
				OutDebugDW("%s: DEBUG DC dest=(%d,%d) size=(%d,%d)\n", ApiRef, nXDest, nYDest, nWDest, nHDest);
				}
				break;
			case GDIMODE_EMULATED:
				if (hdcDest==dxw.RealHDC) { 
					hdcDest=dxw.VirtualHDC;
				}
				// fall through
			default:
				res=(*pGDIPatBlt)(hdcDest,  nXDest, nYDest, nWidth, nHeight, dwRop);
				break;
		}
	}
	else {
		res=(*pGDIPatBlt)(hdcDest,  nXDest, nYDest, nWidth, nHeight, dwRop);
	}

	if(IsDCLeakageDest) (*pGDIReleaseDC)(dxw.GethWnd(), hdcDest);
	if(res && IsToScreen) {
		dxw.ShowOverlay(hdcDest);
		if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdcDest, FALSE, COLORMARKERPATBLT, nXDest, nYDest, nWidth, nHeight);
	}
	_if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
	return res;
}

//#undef OutDebugGDI
//#undef OutDebugDW
//#define OutDebugGDI(f, ...) if(IsDebugGDI) OutTrace(f, __VA_ARGS__)
//#define OutDebugDW(f, ...) if(IsDebugDW) OutTrace(f, __VA_ARGS__)

// NOTE: when creating a font with FIXTEXTOUT option, you have to build two fonts actually, because
// the scaled font should be used only with video DC, while memory DC should use the original sized
// font since the whole DC will be scaled later, eventually.

HFONT WINAPI extCreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
				 DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet,
				 DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality,
				 DWORD fdwPitchAndFamily, LPCTSTR lpszFace)
{
	ApiName("CreateFontA");
	HFONT HFont, HScaled;

	OutTraceGDI("%s: h=%d w=%d face=\"%s\"\n", ApiRef, nHeight, nWidth, lpszFace); // v2.05.35 fix
	if(IsDebugGDI){
		OutTrace("> Escapement: %d\n", nEscapement);
		OutTrace("> Orientation: %d\n", nOrientation);
		OutTrace("> fnWeight: %d\n", fnWeight);
		OutTrace("> fdwItalic: %d\n", fdwItalic);
		OutTrace("> fdwUnderline: %d\n", fdwUnderline);
		OutTrace("> fdwStrikeOut: %d\n", fdwStrikeOut);
		OutTrace("> fdwCharSet: %d\n", fdwCharSet);
		OutTrace("> fdwOutputPrecision: %d\n", fdwOutputPrecision);
		OutTrace("> fdwClipPrecision: %d\n", fdwClipPrecision);
		OutTrace("> fdwQuality: %d\n", fdwQuality);
		OutTrace("> fdwPitchAndFamily: %d\n", fdwPitchAndFamily);
	}

	if(dxw.dwFlags8 & QUALITYFONTS) fdwQuality = BEST_QUALITY;

	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		fdwCharSet = (DWORD)GetCharsetFromANSICodepage(dxw.CodePage);
		OutTraceDW("%s: using charset=%d for codepage=%d\n", ApiRef, fdwCharSet, dxw.CodePage);
	}

	if(dxw.dwFlags11 & SHRINKFONTWIDTH){
		nWidth = (nWidth * 9) / 10;
		OutTraceDW("%s: shrink font size width=%d\n", ApiRef, nWidth);
	}

	HFont = (*pGDICreateFontA)(nHeight, nWidth, nEscapement, nOrientation, fnWeight,
				 fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet,
				 fdwOutputPrecision, fdwClipPrecision, fdwQuality, 
				 fdwPitchAndFamily, lpszFace);

	if((dxw.dwFlags1 & FIXTEXTOUT) && HFont) {
		if(nHeight == 0) {
			// documented way, but how do you get the PitchSize ???
			// nHeight = MulDiv(PitchSize, (*pGDIGetDeviceCaps)(hDC, LOGPIXELSY), 72);
			// an imperfect method to calculate the effective height of a font selected with 0 height.
			HDC hDC = (*pGDIGetDC)(dxw.GethWnd());
			RECT rect = {0, 0, 800, 600}; // big enough
			DrawText(hDC, "Xy", 2, &rect, DT_CALCRECT);
			ReleaseDC(dxw.GethWnd(), hDC);
			nHeight = (rect.bottom * 5) / 4; // multiply by 5/4 to count the interline spacing
			OutTrace("%s: mapping 0 font height to %d\n", ApiRef, nHeight);
			dxw.MapClient(&nWidth, &nHeight);
			nWidth = 0;
		}
		else
		if(nHeight > 0) {
			dxw.MapClient(&nWidth, &nHeight);
		}
		else {
			nHeight= -nHeight;
			dxw.MapClient(&nWidth, &nHeight);
			nHeight= -nHeight;
		}
		HScaled = (*pGDICreateFontA)(nHeight, nWidth, nEscapement, nOrientation, fnWeight,
					 fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet,
					 fdwOutputPrecision, fdwClipPrecision, fdwQuality, 
					 fdwPitchAndFamily, lpszFace); 

		if(HScaled){
			OutTraceGDI("%s: associate font=%#x scaled=%#x\n", ApiRef, HFont, HScaled);
			fontdb.Push(HFont, HScaled);
		}
		else{
			OutErrorGDI("%s: ERROR scaled font err=%d\n", ApiRef, GetLastError());
		}
	}

	if(HFont)
		OutTraceGDI("%s: hfont=%#x\n", ApiRef, HFont);
	else
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return HFont;
}

HFONT WINAPI extCreateFontW(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
				 DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet,
				 DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality,
				 DWORD fdwPitchAndFamily, LPCWSTR lpszFace)
{
	ApiName("CreateFontW");
	HFONT HFont, HScaled;

	OutTraceGDI("%s: h=%d w=%d face=\"%ls\"\n", ApiRef, nHeight, nWidth, lpszFace); // v2.05.35 fix
	if(IsDebugGDI){
		OutTrace("> Escapement: %d\n", nEscapement);
		OutTrace("> Orientation: %d\n", nOrientation);
		OutTrace("> fnWeight: %d\n", fnWeight);
		OutTrace("> fdwItalic: %d\n", fdwItalic);
		OutTrace("> fdwUnderline: %d\n", fdwUnderline);
		OutTrace("> fdwStrikeOut: %d\n", fdwStrikeOut);
		OutTrace("> fdwCharSet: %d\n", fdwCharSet);
		OutTrace("> fdwOutputPrecision: %d\n", fdwOutputPrecision);
		OutTrace("> fdwClipPrecision: %d\n", fdwClipPrecision);
		OutTrace("> fdwQuality: %d\n", fdwQuality);
		OutTrace("> fdwPitchAndFamily: %d\n", fdwPitchAndFamily);
	}

	if(dxw.dwFlags8 & QUALITYFONTS) fdwQuality = BEST_QUALITY;

	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		fdwCharSet = (DWORD)GetCharsetFromANSICodepage(dxw.CodePage);
		OutTraceDW("%s: using charset=%d for codepage=%d\n", ApiRef, fdwCharSet, dxw.CodePage);
	}

	if(dxw.dwFlags11 & SHRINKFONTWIDTH){
		nWidth = (nWidth * 9) / 10;
		OutTraceDW("%s: shrink font size width=%d\n", ApiRef, nWidth);
	}

	HFont = (*pGDICreateFontW)(nHeight, nWidth, nEscapement, nOrientation, fnWeight,
				 fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet,
				 fdwOutputPrecision, fdwClipPrecision, fdwQuality, 
				 fdwPitchAndFamily, lpszFace);

	if((dxw.dwFlags1 & FIXTEXTOUT) && HFont) {
		if(nHeight > 0) dxw.MapClient(&nWidth, &nHeight);
		else {
			nHeight= -nHeight;
			dxw.MapClient(&nWidth, &nHeight);
			nHeight= -nHeight;
		}
		HScaled = (*pGDICreateFontW)(nHeight, nWidth, nEscapement, nOrientation, fnWeight,
					 fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet,
					 fdwOutputPrecision, fdwClipPrecision, fdwQuality, 
					 fdwPitchAndFamily, lpszFace); 

		if(HScaled){
			OutTraceGDI("%s: associate font=%#x scaled=%#x\n", ApiRef, HFont, HScaled);
			fontdb.Push(HFont, HScaled);
		}
		else{
			OutErrorGDI("%s: ERROR scaled font err=%d\n", ApiRef, GetLastError());
		}
	}

	if(HFont)
		OutTraceGDI("%s: hfont=%#x\n", ApiRef, HFont);
	else
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return HFont;
}

// CreateFontIndirect hook routine to avoid font aliasing that prevents reverse blitting working on palettized surfaces
// NONANTIALIASED_QUALITY no longer necessary, since reverse blitting is no longer used

HFONT WINAPI extCreateFontIndirectA(const LOGFONTA *lplf)
{
	ApiName("CreateFontIndirectA");
	HFONT HFont, HScaled;
	LOGFONTA lf;
	OutTraceGDI("%s: h=%d w=%d face=\"%s\"\n", ApiRef, lplf->lfHeight, lplf->lfWidth, lplf->lfFaceName);
	if(IsDebugGDI){
		OutTrace("> Escapement: %d\n", lplf->lfEscapement);
		OutTrace("> Orientation: %d\n", lplf->lfOrientation);
		OutTrace("> fnWeight: %d\n", lplf->lfWeight);
		OutTrace("> fdwItalic: %d\n", lplf->lfItalic);
		OutTrace("> fdwUnderline: %d\n", lplf->lfUnderline);
		OutTrace("> fdwStrikeOut: %d\n", lplf->lfStrikeOut);
		OutTrace("> fdwCharSet: %d\n", lplf->lfCharSet);
		OutTrace("> fdwOutputPrecision: %d\n", lplf->lfOutPrecision);
		OutTrace("> fdwClipPrecision: %d\n", lplf->lfClipPrecision);
		OutTrace("> fdwQuality: %d\n", lplf->lfQuality);
		OutTrace("> fdwPitchAndFamily: %d\n", lplf->lfPitchAndFamily);
	}

	memcpy((char *)&lf, (char *)lplf, sizeof(LOGFONTA));

	if(dxw.dwFlags8 & QUALITYFONTS) lf.lfQuality = BEST_QUALITY; 

	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		lf.lfCharSet = (DWORD)GetCharsetFromANSICodepage(dxw.CodePage);
		OutTraceDW("%s: using charset=%d for codepage=%d\n", ApiRef, lf.lfCharSet, dxw.CodePage);
	}

	if(dxw.dwFlags11 & SHRINKFONTWIDTH){
		lf.lfWidth = (lf.lfWidth * 9) / 10;
		OutTraceDW("%s: shrink font size width=%d\n", ApiRef, lf.lfWidth);
	}

	HFont=(*pGDICreateFontIndirectA)(&lf);

	if(HFont && (dxw.dwFlags1 & FIXTEXTOUT)) { // v2.04.93: don't build a scaled font if the first failed.
		memcpy((char *)&lf, (char *)lplf, sizeof(LOGFONT));
		if(dxw.dwFlags8 & QUALITYFONTS) lf.lfQuality = BEST_QUALITY; 
		if(lf.lfHeight > 0) dxw.MapClient((int *)&lf.lfWidth, (int *)&lf.lfHeight);
		else {
			lf.lfHeight= -lf.lfHeight;
			dxw.MapClient((int *)&lf.lfWidth, (int *)&lf.lfHeight);
			lf.lfHeight= -lf.lfHeight;
		}
		HScaled=((*pGDICreateFontIndirectA)(&lf));

		if(HScaled){
			OutTraceGDI("%s: associate font=%#x scaled=%#x size(wxh)=(%dx%d)\n", ApiRef, HFont, HScaled, lf.lfWidth, lf.lfHeight);
			fontdb.Push(HFont, HScaled);
		}
		else{
			OutErrorGDI("%s: ERROR scaled font err=%d\n", ApiRef, GetLastError());
		}
	}

	if(HFont){
		OutTraceGDI("%s: hfont=%#x\n", ApiRef, HFont);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return HFont;
}

HFONT WINAPI extCreateFontIndirectW(const LOGFONTW *lplf)
{
	ApiName("CreateFontIndirectW");
	HFONT HFont, HScaled;
	LOGFONTW lf;
	OutTraceGDI("%s: h=%d w=%d face=\"%ls\"\n", ApiRef, lplf->lfHeight, lplf->lfWidth, lplf->lfFaceName);
	if(IsDebugGDI){
		OutTrace("> Escapement: %d\n", lplf->lfEscapement);
		OutTrace("> Orientation: %d\n", lplf->lfOrientation);
		OutTrace("> fnWeight: %d\n", lplf->lfWeight);
		OutTrace("> fdwItalic: %d\n", lplf->lfItalic);
		OutTrace("> fdwUnderline: %d\n", lplf->lfUnderline);
		OutTrace("> fdwStrikeOut: %d\n", lplf->lfStrikeOut);
		OutTrace("> fdwCharSet: %d\n", lplf->lfCharSet);
		OutTrace("> fdwOutputPrecision: %d\n", lplf->lfOutPrecision);
		OutTrace("> fdwClipPrecision: %d\n", lplf->lfClipPrecision);
		OutTrace("> fdwQuality: %d\n", lplf->lfQuality);
		OutTrace("> fdwPitchAndFamily: %d\n", lplf->lfPitchAndFamily);
	}

	memcpy((char *)&lf, (char *)lplf, sizeof(LOGFONTW));

	if(dxw.dwFlags8 & QUALITYFONTS) lf.lfQuality = BEST_QUALITY; 

	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		lf.lfCharSet = (DWORD)GetCharsetFromANSICodepage(dxw.CodePage);
		OutTraceDW("%s: using charset=%d for codepage=%d\n", ApiRef, lf.lfCharSet, dxw.CodePage);
	}

	if(dxw.dwFlags11 & SHRINKFONTWIDTH){
		lf.lfWidth = (lf.lfWidth * 9) / 10;
		OutTraceDW("%s: shrink font size width=%d\n", ApiRef, lf.lfWidth);
	}

	HFont=(*pGDICreateFontIndirectW)(&lf);

	if(HFont && (dxw.dwFlags1 & FIXTEXTOUT)) { // v2.04.93: don't build a scaled font if the first failed.
		memcpy((char *)&lf, (char *)lplf, sizeof(LOGFONT));
		if(dxw.dwFlags8 & QUALITYFONTS) lf.lfQuality = BEST_QUALITY; 
		if(lf.lfHeight > 0) dxw.MapClient((int *)&lf.lfWidth, (int *)&lf.lfHeight);
		else {
			lf.lfHeight= -lf.lfHeight;
			dxw.MapClient((int *)&lf.lfWidth, (int *)&lf.lfHeight);
			lf.lfHeight= -lf.lfHeight;
		}
		HScaled=((*pGDICreateFontIndirectW)(&lf));

		if(HScaled){
			OutTraceGDI("%s: associate font=%#x scaled=%#x\n", ApiRef, HFont, HScaled);
			fontdb.Push(HFont, HScaled);
		}
		else{
			OutErrorGDI("%s: ERROR scaled font err=%d\n", ApiRef, GetLastError());
		}
	}

	if(HFont){
		OutTraceGDI("%s: hfont=%#x\n", ApiRef, HFont);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return HFont;
}

BOOL WINAPI extSetDeviceGammaRamp(HDC hDC, LPVOID lpRamp)
{
	BOOL ret;
	OutTraceGDI("SetDeviceGammaRamp: hdc=%#x\n", hDC);

	if(dxw.dwFlags2 & DISABLEGAMMARAMP) {
		OutTraceGDI("SetDeviceGammaRamp: SUPPRESS\n");
		return TRUE;
	}

	if((dxw.dwFlags10 & FORCED3DGAMMARAMP) && (dxw.pInitialRamp == NULL)) dxw.InitGammaRamp();

	ret=(*pGDISetDeviceGammaRamp)(hDC, lpRamp);
	_if(!ret) OutErrorGDI("SetDeviceGammaRamp: ERROR err=%d\n", GetLastError());
	return ret;
}

BOOL WINAPI extGetDeviceGammaRamp(HDC hDC, LPVOID lpRamp)
{
	BOOL ret;
	OutTraceGDI("GetDeviceGammaRamp: hdc=%#x\n", hDC);
	ret=(*pGDIGetDeviceGammaRamp)(hDC, lpRamp);
	_if(!ret) OutErrorGDI("GetDeviceGammaRamp: ERROR err=%d\n", GetLastError());
	return ret;
}

int WINAPI extGetClipBox(HDC hdc, LPRECT lprc)
{
	// v2.02.31: needed in "Imperialism II" to avoid blit clipping
	int ret;
	ApiName("GetClipBox");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pGDIGetClipBox)(sdc.GetHdc(), lprc);
				sdc.PutPrimaryDC(hdc, FALSE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				ret=(*pGDIGetClipBox)(hdc, lprc);
				OutTraceGDI("%s: scaling main win coordinates (%d,%d)-(%d,%d)\n",
					ApiRef, lprc->left, lprc->top, lprc->right, lprc->bottom);
				dxw.UnmapClient(lprc);
				break;
			default:
				ret=(*pGDIGetClipBox)(hdc, lprc);
				break;
		}
	}
	else 
		ret=(*pGDIGetClipBox)(hdc, lprc);

	OutTraceGDI("%s: ret=%#x(%s) rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, ret, ExplainRegionType(ret), lprc->left, lprc->top, lprc->right, lprc->bottom);
	return ret;
}

static int WINAPI HandleClipRect(
	char *api,
	HandleClipRect_Type pHandleClipRect, 
	HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	int ret;
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d)\n", api, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pHandleClipRect)(sdc.GetHdc(), nLeftRect, nTopRect, nRightRect, nBottomRect);
				sdc.PutPrimaryDC(hdc, FALSE);
				return ret;
				break;
			case GDIMODE_STRETCHED:
				dxw.MapClient(&nLeftRect, &nTopRect, &nRightRect, &nBottomRect);
				OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", api, nLeftRect, nTopRect, nRightRect, nBottomRect);
				break;
		}							
	}

	ret=(*pHandleClipRect)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
	OutTraceGDI("%s: ret=%#x(%s)\n", api, ret, ExplainRegionType(ret)); 
	return ret;
}

int WINAPI extExcludeClipRect(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{ ApiName("ExcludeClipRect"); return HandleClipRect(ApiRef, pExcludeClipRect, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect); }
int WINAPI extIntersectClipRect(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{ ApiName("IntersectClipRect"); return HandleClipRect(ApiRef, pIntersectClipRect, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect); }

BOOL WINAPI extPolyline(HDC hdc, const POINT *lppt, int cPoints)
{
	// LOGTOBEFIXED
	BOOL ret;
	ApiName("Polyline");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		int i;
		OutTrace("%s: hdc=%#x cPoints=%d pt=", ApiRef, hdc, cPoints); 
		for(i=0; i<cPoints; i++) OutTrace("(%d,%d) ", lppt[i].x, lppt[i].y);
		OutTrace("\n");
	}
#endif

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolyline)(sdc.GetHdc(), lppt, cPoints);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED:
				for(int i=0; i<cPoints; i++) dxw.MapClient((LPPOINT)&lppt[i]);
#ifndef DXW_NOTRACES
				if(IsTraceGDI){
					OutTrace("%s: fixed cPoints=%d pt=", ApiRef, cPoints); 
					for(int i=0; i<cPoints; i++) OutTrace("(%d,%d) ", lppt[i].x, lppt[i].y);
					OutTrace("\n");
				}
#endif
				break;
		}
	}
	ret=(*pPolyline)(hdc, lppt, cPoints);
	_if(!ret)OutErrorGDI("%s: ERROR ret=%#x\n", ApiRef, ret); 
	return ret;
}

BOOL WINAPI extLineTo(HDC hdc, int nXEnd, int nYEnd)
{
	BOOL ret;
	ApiName("LineTo");
	OutTraceGDI("%s: hdc=%#x pt=(%d,%d)\n", ApiRef, hdc, nXEnd, nYEnd);
	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pLineTo)(sdc.GetHdc(), nXEnd, nYEnd);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nXEnd, &nYEnd);
				OutTraceGDI("%s: fixed pt=(%d,%d)\n", ApiRef, nXEnd, nYEnd);
				break;
		}
	}
	ret=(*pLineTo)(hdc, nXEnd, nYEnd);
	_if(!ret)OutErrorGDI("%s: ERROR ret=%#x\n", ApiRef, ret); 
	return ret;
}

BOOL WINAPI extArcTo(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2)
{
	BOOL ret;
	ApiName("ArcTo");
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)(%d,%d) radial=(%d,%d)(%d,%d)\n", 
		ApiRef, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pArcTo)(sdc.GetHdc(), nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;	
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nLeftRect, &nTopRect, &nRightRect, &nBottomRect);
				dxw.MapClient(&nXRadial1, &nYRadial1, &nXRadial2, &nYRadial2);
				OutTraceGDI("%s: fixed rect=(%d,%d)(%d,%d) radial=(%d,%d)(%d,%d)\n", 
					ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
				break;
		}
	}
	ret=(*pArcTo)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
	_if(!ret)OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extMoveToEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("MoveToEx");
	OutTraceGDI("%s: hdc=%#x pt=(%d,%d)\n", ApiRef, hdc, X, Y);
	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pMoveToEx)(sdc.GetHdc(), X, Y, lpPoint);	
				sdc.PutPrimaryDC(hdc, FALSE);
				return ret; // no need to update the screen!
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&X, &Y);
				OutTraceGDI("%s: fixed pt=(%d,%d)\n", ApiRef, X, Y);
				break;
		}
	}
	ret=(*pMoveToEx)(hdc, X, Y, lpPoint);
	_if(!ret)OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extPolyDraw(HDC hdc, const POINT *lpPoints, const BYTE *lpbTypes, int cCount)
{
	BOOL ret;
	ApiName("PolyDraw");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("%s: hdc=%#x cCount=%d\n", ApiRef, hdc, cCount); 
		for(int i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
	}
#endif

	if (dxw.IsToRemap(hdc)){
		int i, size;
		POINT *lpRemPoints;
		
		size = cCount * sizeof(POINT);
		lpRemPoints = (LPPOINT)malloc(size);
		memcpy(lpRemPoints, lpPoints, size);

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				free(lpRemPoints);
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolyDraw)(sdc.GetHdc(), lpPoints, lpbTypes, cCount);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				for(i=0; i<cCount; i++) {
					dxw.MapClient(&lpRemPoints[i]);
				}
#ifndef DXW_NOTRACES
				if(IsTraceGDI){
					OutTrace("%s: fixed cCount=%d\n", ApiRef, cCount); 
					for(int i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
				}
#endif
				break;
			case GDIMODE_EMULATED:
				break;	
		}
		ret=(*pPolyDraw)(hdc, lpRemPoints, lpbTypes, cCount);
		free(lpRemPoints);
	}
	else {
		ret=(*pPolyDraw)(hdc, lpPoints, lpbTypes, cCount);
	}
	_if(!ret)OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extPolylineTo(HDC hdc, const POINT *lpPoints, DWORD cCount)
{
	BOOL ret;
	ApiName("PolylineTo");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("%s: hdc=%#x cCount=%d\n", ApiRef, hdc, cCount); 
		for(DWORD i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
	}
#endif

	if (dxw.IsToRemap(hdc)){

		DWORD i, size;
		POINT *lpRemPoints;
		
		size = cCount * sizeof(POINT);
		lpRemPoints = (LPPOINT)malloc(size);
		memcpy(lpRemPoints, lpPoints, size);

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				free(lpRemPoints);
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolylineTo)(sdc.GetHdc(), lpPoints, cCount);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				for(i=0; i<cCount; i++) {
					dxw.MapClient(&lpRemPoints[i]);
				}
#ifndef DXW_NOTRACES
				if(IsTraceGDI){
					OutTrace("%s: fixed cCount=%d\n", ApiRef, cCount); 
					for(DWORD i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
				}
#endif
				break;
			case GDIMODE_EMULATED:
				break;	
		}
		ret=(*pPolylineTo)(hdc, lpRemPoints, cCount);
		free(lpRemPoints);
	}
	else {
		ret=(*pPolylineTo)(hdc, lpPoints, cCount);
	}
	_if(!ret)OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extPolyBezierTo(HDC hdc, const POINT *lpPoints, DWORD cCount)
{
	BOOL ret;
	ApiName("PolyBezierTo");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("%s: hdc=%#x cCount=%d\n", ApiRef, hdc, cCount); 
		for(DWORD i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
	}
#endif

	if (dxw.IsToRemap(hdc)){
		DWORD i, size;
		POINT *lpRemPoints;
		
		size = cCount * sizeof(POINT);
		lpRemPoints = (LPPOINT)malloc(size);
		memcpy(lpRemPoints, lpPoints, size);

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				free(lpRemPoints);
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolyBezierTo)(sdc.GetHdc(), lpPoints, cCount);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				for(i=0; i<cCount; i++) {
					dxw.MapClient(&lpRemPoints[i]);
				}
#ifndef DXW_NOTRACES
				if(IsTraceGDI){
					OutTrace("%s: fixed cCount=%d\n", ApiRef, cCount); 
					for(i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, &lpRemPoints[i].x, &lpRemPoints[i].y);
				}
#endif
				break;
			case GDIMODE_EMULATED:
				break;	
		}
		ret=(*pPolyBezierTo)(hdc, lpRemPoints, cCount);
		free(lpRemPoints);
	}
	else {
		ret=(*pPolyBezierTo)(hdc, lpPoints, cCount);
	}
	_if(!ret)OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extPolyBezier(HDC hdc, const POINT *lpPoints, DWORD cCount)
{
	BOOL ret;
	ApiName("PolyBezier");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		DWORD i;
		OutTrace("%s: hdc=%#x cCount=%d\n", ApiRef, hdc, cCount); 
		for(i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
	}
#endif

	if (dxw.IsToRemap(hdc)){
		DWORD i, size;
		POINT *lpRemPoints;
		
		size = cCount * sizeof(POINT);
		lpRemPoints = (LPPOINT)malloc(size);
		memcpy(lpRemPoints, lpPoints, size);

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				free(lpRemPoints);
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolyBezier)(sdc.GetHdc(), lpPoints, cCount);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				for(i=0; i<cCount; i++) {
					dxw.MapClient(&lpRemPoints[i]);
				}
#ifndef DXW_NOTRACES
				if(IsTraceGDI){
					OutTrace("%s: fixed cCount=%d\n", ApiRef, cCount); 
					for(i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
				}
#endif
				break;
			case GDIMODE_EMULATED:
				break;	
		}
		ret=(*pPolyBezier)(hdc, lpRemPoints, cCount);
		free(lpRemPoints);
	}
	else {
		ret=(*pPolyBezier)(hdc, lpPoints, cCount);
	}
	_if(!ret)OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

int WINAPI extStretchDIBits(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, 
				  const VOID *lpBits, const BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop)
{
	int ret;
	ApiName("StretchDIBits");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTraceGDI("%s: hdc=%#x dest=(%d,%d):(%dx%d) src=(%d,%d):(%dx%d) rop=%#x(%s)\n", 
			ApiRef, hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, dwRop, ExplainROP(dwRop));
		TraceBITMAPINFOHEADER(ApiRef, (BITMAPINFOHEADER *)&(lpBitsInfo->bmiHeader));
	}
#endif

	if(dxw.IsToRemap(hdc)){
		dxw.HandleDIB(); // handle refresh delays
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pStretchDIBits)(sdc.GetHdc(), XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);
				sdc.PutPrimaryDC(hdc, TRUE, XDest, YDest, nDestWidth, nDestHeight);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&XDest, &YDest, &nDestWidth, &nDestHeight);
				OutTraceGDI("%s: fixed STRETCHED dest=(%d,%d):(%dx%d)\n", ApiRef, XDest, YDest, nDestWidth, nDestHeight);
				break;
			case GDIMODE_EMULATED:
				break;
			default:
				break;
		}
	}

#ifndef DXW_NOTRACES
	if(dxw.dwDFlags & DUMPDIBSECTION) DumpDibSection(ApiRef, hdc, lpBitsInfo, iUsage, (LPBYTE)lpBits);
#endif // DXW_NOTRACES
	ret=(*pStretchDIBits)(hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);
	if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdc, FALSE, COLORMARKERSTRETCHDIB, XDest, YDest, nDestWidth, nDestHeight);
	_if(!ret || (ret==GDI_ERROR)) OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
	return ret;
}

int WINAPI extSetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, const VOID *lpvBits, const BITMAPINFO *lpbmi, UINT fuColorUse)
{
	int ret;
	ApiName("SetDIBits");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("%s: hdc=%#x%s hbmp=%#x lines=(%d,%d) ColorUse=%#x(%s)\n", 
			ApiRef, hdc, dxw.IsToRemap(hdc)?"(R)":"", hbmp, uStartScan, cScanLines, fuColorUse, ExplainDIBUsage(fuColorUse));
		TraceBITMAPINFOHEADER("SetDIBits", (BITMAPINFOHEADER *)&(lpbmi->bmiHeader));
	}
#endif

	// v2.04.60: hdc of primary window strangely returns 0 as GetObjectType (error) in "Honour & Freedom"
	//if(dxw.IsToRemap(hdc) && !bGDIRecursionFlag){
	if((dxw.IsToRemap(hdc) || ((*pGetObjectType)(hdc)==0 )) && !bGDIRecursionFlag){
		HDC hTempDc;
		HBITMAP hbmPic;
		DWORD OrigWidth, OrigHeight;
		dxw.HandleDIB(); // handle refresh delays
		HGDIOBJ obj;
		BOOL ret2;
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: // this will flicker !!!!
				sdc.GetPrimaryDC(hdc);
				ret=(*pSetDIBits)(sdc.GetHdc(), hbmp, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
				_if(!ret || (ret==GDI_ERROR)) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
				sdc.PutPrimaryDC(hdc, TRUE, 0, 0, lpbmi->bmiHeader.biWidth, lpbmi->bmiHeader.biHeight);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				// v2.04.60: reinserted logic for stretching DIBs for "Honour & Freedom" 16bit DIB
				OrigWidth=lpbmi->bmiHeader.biWidth;
				OrigHeight=lpbmi->bmiHeader.biHeight;
				if(lpbmi->bmiHeader.biHeight < 0) OrigHeight = -lpbmi->bmiHeader.biHeight;
				// blitting to primary surface !!!
				dxw.MapClient((int *)&OrigWidth, (int *)&OrigHeight);
				OutTraceGDI("%s: fixed size=(%dx%d)\n", ApiRef, OrigWidth, OrigHeight);
				hTempDc=CreateCompatibleDC(hdc);
				_if(!hTempDc) OutErrorGDI("%s: CreateCompatibleDC ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
				// tricky part: CreateCompatibleBitmap is needed to set the dc size, but it has to be performed
				// against hdc to set for color depth, then selected (through SelectObject) against the temporary
				// dc to assign the needed size and color space to the temporary dc.
				hbmPic=CreateCompatibleBitmap(hdc, OrigWidth, OrigHeight);
				_if(!hbmPic) ("%s: CreateCompatibleBitmap ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
				obj = (*pSelectObject)(hTempDc, hbmPic);
				_if(!obj) OutErrorGDI("%s SelectObject ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
				if((lpbmi->bmiHeader.biBitCount == 8) && (dxw.dwFlags2 & PALDIBEMULATION)){
					VOID *lpvNewBits;
					BITMAPINFO NewBmi;
					lpvNewBits = dxw.EmulateDIB((LPVOID)lpvBits, lpbmi, &NewBmi, fuColorUse);
#ifndef DXW_NOTRACES
					if(dxw.dwDFlags & DUMPDIBSECTION)
						DumpDibSection(ApiRef, hdc, &NewBmi, DIB_RGB_COLORS, lpvNewBits);
#endif // DXW_NOTRACES
					ret=(*pSetDIBitsToDevice)(hTempDc, 0, 0, OrigWidth, OrigHeight, 0, 0, uStartScan, cScanLines, lpvNewBits, &NewBmi, DIB_RGB_COLORS);
					_if(!ret) OutErrorGDI("%s: SetDIBitsToDevice ERROR err=%d\n", ApiRef, GetLastError());
					free(lpvNewBits);
				}
				else {
					ret=(*pSetDIBits)(hTempDc, hbmp, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
					_if(!ret) OutErrorGDI("%s: SetDIBits ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
				}
				bGDIRecursionFlag = FALSE;
#ifndef DXW_NOTRACES
				if((dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(hTempDc, 0, 0, OrigWidth, OrigHeight);
#endif // DXW_NOTRACES
				// v2.02.94: set HALFTONE stretching. Fixes "Celtic Kings Rage of War"
				SetStretchBltMode(hdc,HALFTONE);
				ret=(*pGDIStretchBlt)(hdc, 0, 0, lpbmi->bmiHeader.biWidth, lpbmi->bmiHeader.biHeight, hTempDc, 0, 0, OrigWidth, OrigHeight, SRCCOPY);
				_if(!ret) OutErrorGDI("%s: StretchBlt ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
				ret2 = DeleteObject(hbmPic); // v2.02.32 - avoid resource leakage
				_if(!ret2) OutErrorGDI("%s: DeleteObject ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
				ret2 = DeleteDC(hTempDc);
				_if(!ret2) OutErrorGDI("%s: DeleteDC ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
				if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdc, FALSE, COLORMARKERSETDIB,  0, 0, lpbmi->bmiHeader.biWidth, lpbmi->bmiHeader.biHeight);
				// v2.06.09 fix suggested by nervoushammer - SetDIBits should return the number of processed lines
				if(ret) ret = cScanLines;
				return ret;
				break;
			case GDIMODE_EMULATED:
#if 0
				if (dxw.IsVirtual(hdc)){
					int X, Y;
					X=XDest;
					Y=YDest;
					ret=(*pSetDIBits)(sdc.GetHdc(), hbmp, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
					if(!ret || (ret==GDI_ERROR)) OutErrorGDI("SetDIBits: ERROR ret=%#x err=%d\n", ret, GetLastError()); 
					return ret;
				}
#endif
				break;
			default:
				break;
		}
	}

	ret = (*pSetDIBits)(hdc, hbmp, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
#ifndef DXW_NOTRACES
	if(dxw.dwDFlags & DUMPDIBSECTION) DumpDibSection(ApiRef, hdc, lpbmi, fuColorUse, (LPBYTE)lpvBits);
#endif // DXW_NOTRACES
	_if(!ret || (ret==GDI_ERROR)) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

static BYTE InvertPalette32(DWORD pixel, LPPALETTEENTRY Pal)
{
	DWORD BestCost = (DWORD)-1;
	BYTE BestByte = 0;
	BYTE bRed   = pixel & 0x000000FF;
	BYTE bGreen = pixel & 0x0000FF00 >> 8;
	BYTE bBlue  = pixel & 0x00FF0000 >> 16;
	//BYTE bBlue  = pixel & 0x000000FF;
	//BYTE bGreen = pixel & 0x0000FF00 >> 8;
	//BYTE bRed   = pixel & 0x00FF0000 >> 16;
	for (int i=0; i<255; i++){
		int RCost = (bRed - (Pal[i]).peRed); if(RCost < 0) RCost = -RCost;
		int GCost = (bGreen - (Pal[i]).peGreen); if(GCost < 0) GCost = -GCost;
		int BCost = (bBlue - (Pal[i]).peBlue); if(RCost < 0) BCost = -BCost;
		DWORD Cost = (RCost * RCost) + (GCost * GCost) + (BCost * BCost);
		if(Cost < BestCost){
			BestByte = i;
			BestCost = Cost;
		}
	}
	//OutTrace("Pixel=%#x byte=%#x cost=%d\n", pixel, BestByte, BestCost); 
	return BestByte;
}

int WINAPI extGetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT uUsage)
{
	int ret;
	ApiName("GetDIBits");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("%s: hdc=%#x hbmp=%#x lines=(%d,%d) lpvbits=%#x ColorUse=%#x(%s)\n", ApiRef, hdc, hbmp, uStartScan, cScanLines, lpvBits, uUsage, ExplainDIBUsage(uUsage));
		TraceBITMAPINFOHEADER(ApiRef, (BITMAPINFOHEADER *)&(lpbmi->bmiHeader));
	}
#endif

	if(dxw.IsToRemap(hdc) && !bGDIRecursionFlag){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: // this will flicker !!!!
				sdc.GetPrimaryDC(hdc);
				ret=(*pGetDIBits)(sdc.GetHdc(), hbmp, uStartScan, cScanLines, lpvBits, lpbmi, uUsage);
				sdc.PutPrimaryDC(hdc, FALSE);
#ifndef DXW_NOTRACES
				if(dxw.dwDFlags & DUMPDIBSECTION) DumpDibSection("GetDIBits", hdc, lpbmi, uUsage, (LPBYTE)lpvBits);
#endif // DXW_NOTRACES
				return ret;
				break;
			case GDIMODE_EMULATED:
			default:
				break;
		}
	}

	ret = (*pGetDIBits)(hdc, hbmp, uStartScan, cScanLines, lpvBits, lpbmi, uUsage);
	_if(!ret || (ret==GDI_ERROR)) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
		return ret;
	}

#ifndef DXW_NOTRACES
	if(dxw.dwDFlags & DUMPDIBSECTION) DumpDibSection(ApiRef, hdc, lpbmi, uUsage, (LPBYTE)lpvBits);
	TraceBITMAPINFOHEADER(ApiRef, (BITMAPINFOHEADER *)&(lpbmi->bmiHeader));
#endif // DXW_NOTRACES

	if ((dxw.dwFlags12 & REVERTDIBPALETTE) &&
		(lpbmi->bmiHeader.biBitCount == 32)){
		if(dxw.VirtualPixelFormat.dwRGBBitCount == 8){
			if(lpvBits){
				int w = lpbmi->bmiHeader.biWidth;
				int h = lpbmi->bmiHeader.biHeight;
				if (h<0) h = -h;
				HPALETTE hpal;
				PALETTEENTRY DCPaletteEntries[256];	
				// hellish trick: to get the DC palette change it twice, but the replacement must be successful,
				// so you must use a valid palette handle to be replaced: GetStockObject(DEFAULT_PALETTE) is ok.
				hpal=(*pGDISelectPalette)(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), 0);
				if(hpal){
					int nEntries;
					(*pGDISelectPalette)(hdc, hpal, 0);
					nEntries=(*pGetPaletteEntries)(hpal, 0, 256, DCPaletteEntries);
				}
				LPDWORD pw = (LPDWORD)lpvBits;
				LPBYTE pb = (LPBYTE)lpvBits;
				int iByteAlign = w - ((w >> 2) << 2);
				for(int y=0; y<h; y++) {
					for(int x=0; x<w; x++) {
						*pb++ = InvertPalette32(*pw++, DCPaletteEntries);
					}
					pb += iByteAlign;
				}
			}
			lpbmi->bmiHeader.biBitCount = 8;
			lpbmi->bmiHeader.biSizeImage = lpbmi->bmiHeader.biSizeImage / 4;
		}

		if(dxw.VirtualPixelFormat.dwRGBBitCount == 16){
			if(lpvBits){
				MessageBox(NULL, "TO DO: pixel conversion", "DxWnd", 0);
			}
			lpbmi->bmiHeader.biBitCount = 16;
			lpbmi->bmiHeader.biSizeImage = lpbmi->bmiHeader.biSizeImage / 2;
		}
	}

	return ret;
}

LONG WINAPI extGetBitmapBits(HBITMAP hbit, LONG cb, LPVOID lpvBits)
{
	LONG res;
	ApiName("GetBitmapBits");

	OutTraceGDI("%s: hbitmap=%#x cb=%d\n", ApiRef, hbit, cb);
	res = (*pGetBitmapBits)(hbit, cb, lpvBits);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

int WINAPI extSetDIBitsToDevice(HDC hdc, int XDest, int YDest, DWORD dwWidth, DWORD dwHeight, int XSrc, int YSrc, UINT uStartScan, UINT cScanLines, 
					const VOID *lpvBits, const BITMAPINFO *lpbmi, UINT fuColorUse)
{
	int ret;
	ApiName("SetDIBitsToDevice");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("%s: hdc=%#x dest=(%d,%d)-(%dx%d) src=(%d,%d) lines=(%d,%d) bits=%#x ColorUse=%#x(%s)\n", 
			ApiRef, hdc, XDest, YDest, dwWidth, dwHeight, XSrc, YSrc, uStartScan, cScanLines, lpvBits, fuColorUse, ExplainDIBUsage(fuColorUse));
		TraceBITMAPINFOHEADER(ApiRef, (BITMAPINFOHEADER *)&(lpbmi->bmiHeader));
		if(IsDebugGDI && IsTraceHex){
			if(fuColorUse == DIB_RGB_COLORS) 
				HexTrace((LPBYTE)&(lpbmi->bmiColors[0]), 256 * sizeof(DWORD));
			else
				HexTrace((LPBYTE)&(lpbmi->bmiColors[0]), 256 * sizeof(WORD));
		}
	}

	if(dxw.dwDFlags & DUMPDIBSECTION) DumpDibSection(ApiRef, hdc, lpbmi, fuColorUse, (LPBYTE)lpvBits);
#endif // DXW_NOTRACES

	bGDIRecursionFlag = TRUE; // beware: it seems that SetDIBitsToDevice calls SetDIBits internally
	if(dxw.IsToRemap(hdc)){
		HDC hTempDc;
		HBITMAP hbmPic;
		DWORD OrigWidth, OrigHeight;
		int OrigXDest, OrigYDest;
		HGDIOBJ obj;
		BOOL ret2;
		OrigWidth=dwWidth;
		OrigHeight=dwHeight;
		OrigXDest=XDest;
		OrigYDest=YDest;
		dxw.HandleDIB(); // handle refresh delays
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				if(lpbmi->bmiHeader.biBitCount == 8){
					HPALETTE hpal = (*pGDISelectPalette)(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
					(*pGDISelectPalette)(sdc.GetHdc(), hpal, TRUE);
					(*pGDISelectPalette)(hdc, hpal, TRUE);
				}
				ret=(*pSetDIBitsToDevice)(sdc.GetHdc(), XDest, YDest, dwWidth, dwHeight, XSrc, YSrc, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
#ifndef DXW_NOTRACES
				if((dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(sdc.GetHdc(), XDest, YDest, dwWidth, dwHeight);
#endif // DXW_NOTRACES
				sdc.PutPrimaryDC(hdc, TRUE, XDest, YDest, dwWidth, dwHeight);
				bGDIRecursionFlag = FALSE;
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				// blitting to primary surface !!!
				dxw.MapClient(&XDest, &YDest, (int *)&dwWidth, (int *)&dwHeight);
				OutTraceGDI("%s: fixed dest=(%d,%d)-(%dx%d)\n", ApiRef, XDest, YDest, dwWidth, dwHeight);
				hTempDc=CreateCompatibleDC(hdc);
				_if(!hTempDc) OutErrorGDI("%s: CreateCompatibleDC ERROR err=%d\n", ApiRef, GetLastError());
				// tricky part: CreateCompatibleBitmap is needed to set the dc size, but it has to be performed
				// against hdc to set for color depth, then selected (through SelectObject) against the temporary
				// dc to assign the needed size and color space to the temporary dc.
				hbmPic=CreateCompatibleBitmap(hdc, OrigWidth, OrigHeight);
				_if(!hbmPic) OutErrorGDI("%s: CreateCompatibleBitmap ERROR err=%d\n", ApiRef, GetLastError());
				obj = (*pSelectObject)(hTempDc, hbmPic);
				_if(!obj) OutErrorGDI("%s: SelectObject ERROR err=%d\n", ApiRef, GetLastError());
				// v2.05.67: copy hdc palette to hTempDc. Tricky: to get the system palette you have to replace it twice,
				// but the operation must be successful, so you can't use (HPALETTE)NULL, it is necessary to use a valid
				// palette like (HPALETTE)GetStockObject(DEFAULT_PALETTE).
				if(lpbmi->bmiHeader.biBitCount == 8){
					HPALETTE hpal = (*pGDISelectPalette)(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
					(*pGDISelectPalette)(hTempDc, hpal, TRUE);
					(*pGDISelectPalette)(hdc, hpal, TRUE);
				}
				if((lpbmi->bmiHeader.biBitCount == 8) && (dxw.dwFlags2 & PALDIBEMULATION)){
					VOID *lpvNewBits;
					BITMAPINFO NewBmi;
					lpvNewBits = dxw.EmulateDIB((LPVOID)lpvBits, lpbmi, &NewBmi, fuColorUse);
#ifndef DXW_NOTRACES
					if(dxw.dwDFlags & DUMPDIBSECTION)
						DumpDibSection(ApiRef, hdc, &NewBmi, DIB_RGB_COLORS, lpvNewBits);
#endif // DXW_NOTRACES
					ret=(*pSetDIBitsToDevice)(hTempDc, 0, 0, OrigWidth, OrigHeight, XSrc, YSrc, uStartScan, cScanLines, lpvNewBits, &NewBmi, DIB_RGB_COLORS);
					_if(!ret) OutErrorGDI("%s: SetDIBitsToDevice ERROR err=%d\n", ApiRef, GetLastError());
					free(lpvNewBits);
				}
				else {
					ret=(*pSetDIBitsToDevice)(hTempDc, 0, 0, OrigWidth, OrigHeight, XSrc, YSrc, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
					_if(!ret) OutErrorGDI("%s: SetDIBitsToDevice ERROR err=%d\n", ApiRef, GetLastError());
				}
				bGDIRecursionFlag = FALSE;
#ifndef DXW_NOTRACES
				if((dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(hTempDc, 0, 0, OrigWidth, OrigHeight);
#endif // DXW_NOTRACES
				// v2.02.94: set HALFTONE stretching. Fixes "Celtic Kings Rage of War"
				SetStretchBltMode(hdc,HALFTONE);
				ret=(*pGDIStretchBlt)(hdc, XDest, YDest, dwWidth, dwHeight, hTempDc, 0, 0, OrigWidth, OrigHeight, SRCCOPY);
				_if(!ret) OutErrorGDI("%s: StretchBlt ERROR err=%d\n", ApiRef, GetLastError());
				ret2 = DeleteObject(hbmPic); // v2.02.32 - avoid resource leakage
				_if(!ret2) OutErrorGDI("%s: DeleteObject ERROR err=%d\n", ApiRef, GetLastError());
				ret2 = DeleteDC(hTempDc);
				_if(!ret2) OutErrorGDI("%s: DeleteDC ERROR err=%d\n", ApiRef, GetLastError());
				if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdc, FALSE, COLORMARKEROUTDIB,  XDest, YDest, dwWidth, dwHeight);
				// v2.06.09 fix suggested by nervoushammer - SetDIBitsToDevice should return the number of processed lines
				if(ret) ret = cScanLines;
				return ret;
				break;
			case GDIMODE_EMULATED:
				ret=(*pSetDIBitsToDevice)(hdc, XDest, YDest, dwWidth, dwHeight, XSrc, YSrc, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
				bGDIRecursionFlag = FALSE;
#ifndef DXW_NOTRACES
				if((dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(hdc, XDest, YDest, dwWidth, dwHeight);
#endif // DXW_NOTRACES
				_if(!ret || (ret==GDI_ERROR)) OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
				if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdc, FALSE, COLORMARKEROUTDIB,  XDest, YDest, dwWidth, dwHeight);
				return ret;
			default:
#ifndef DXW_NOTRACES
				if((dxw.dwDFlags & DUMPDEVCONTEXT) && dxw.bCustomKeyToggle) DumpHDC(hdc, XDest, YDest, dwWidth, dwHeight);
#endif // DXW_NOTRACES
				break;
		}
	}

	ret=(*pSetDIBitsToDevice)(hdc, XDest, YDest, dwWidth, dwHeight, XSrc, YSrc, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
	bGDIRecursionFlag = FALSE;
	_if(!ret || (ret==GDI_ERROR)) OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
	if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdc, FALSE, COLORMARKEROUTDIB,  XDest, YDest, dwWidth, dwHeight);
	dxw.HandleDIB(); // v2.05.98: required to slow "Interstate 76" with -gdi option
	return ret;
}

UINT WINAPI extGetDIBColorTable(HDC hdc, UINT uStartIndex, UINT cEntries, RGBQUAD *pColors)
{
	UINT ret;
	ApiName("GetDIBColorTable");
	OutTraceGDI("%s: hdc=%#x start=%d entries=%d\n", ApiRef, hdc, uStartIndex, cEntries);

	if(dxw.dwFlags2 & PALDIBEMULATION){
		ret = dxw.GetDIBColors(hdc, uStartIndex, cEntries, pColors);
	}
	else{
		ret = (*pGetDIBColorTable)(hdc, uStartIndex, cEntries, pColors);
	}

	OutTraceGDI("%s: ret=%d\n", ApiRef, ret);
	if(IsDebugDW) dxw.DumpPalette(cEntries, (PALETTEENTRY *)pColors);
	return ret;
}

UINT WINAPI extSetDIBColorTable(HDC hdc, UINT uStartIndex, UINT cEntries, const RGBQUAD *pColors)
{
	UINT ret;
	ApiName("SetDIBColorTable");
	OutTraceGDI("%s: hdc=%#x start=%d entries=%d\n", ApiRef, hdc, uStartIndex, cEntries);
	if(IsDebugDW) dxw.DumpPalette(cEntries, (PALETTEENTRY *)pColors);

	if(dxw.dwFlags2 & PALDIBEMULATION){
		ret = dxw.SetDIBColors(hdc, uStartIndex, cEntries, pColors);
	}
	else {
		ret = (*pSetDIBColorTable)(hdc, uStartIndex, cEntries, pColors);
	}
	OutTraceGDI("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

HBITMAP WINAPI extCreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight)
{
	HBITMAP ret;
	ApiName("CreateCompatibleBitmap");
	OutTraceGDI("%s: hdc=%#x size=(%d,%d)\n", ApiRef, hdc, nWidth, nHeight);

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pCreateCompatibleBitmap)(sdc.GetHdc(), nWidth, nHeight);
				sdc.PutPrimaryDC(hdc, FALSE);
#ifndef DXW_NOTRACES
				if(ret) {
					OutTraceGDI("%s: hbitmap=%#x\n", ApiRef, ret); 
				} else {
					OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
				}
#endif // DXW_NOTRACES
				return ret;
				break;
			case GDIMODE_STRETCHED:
				// v2.05.69: commented out, the compatible bitmap should not be stretched at this time
				// because it should be stretched later when going to video DC, or not stretched at all
				// if used to build a custom cursor (see "Julia Szalone Lata 20.!"
				//dxw.MapClient(&nWidth, &nHeight);
				//OutTraceGDI("%s: fixed size=(%d,%d)\n", ApiRef, nWidth, nHeight);
				break;
			default:
				break;
		}
	}

	if(dxw.dwFlags13 & FIXDCCOLORDEPTH){
		HWND hwnd = WindowFromDC(hdc);
		if(dxw.IsDesktop(hwnd) && (dxw.VirtualPixelFormat.dwRGBBitCount != dxw.ActualPixelFormat.dwRGBBitCount)){
			OutTraceDW("%s: fixing hdc color depth\n", ApiRef);
			hdc = (*pGDICreateCompatibleDC)(hdc);
			LPVOID bits;
			HBITMAP hbmp;
			BITMAPINFO bmpInfo;
			ZeroMemory (&bmpInfo, sizeof (BITMAPINFO));
			bmpInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
			bmpInfo.bmiHeader.biWidth = 1;
			bmpInfo.bmiHeader.biHeight = 1;
			bmpInfo.bmiHeader.biPlanes = 1;
			bmpInfo.bmiHeader.biBitCount = (WORD)dxw.VirtualPixelFormat.dwRGBBitCount;
			bmpInfo.bmiHeader.biCompression = BI_RGB;
			bmpInfo.bmiHeader.biSizeImage = 0;
			bmpInfo.bmiHeader.biXPelsPerMeter = 0;
			bmpInfo.bmiHeader.biYPelsPerMeter = 0;
			bmpInfo.bmiHeader.biClrUsed = 0;
			bmpInfo.bmiHeader.biClrImportant = 0;
			hbmp = (*pCreateDIBSection)(hdc, &bmpInfo, DIB_RGB_COLORS, &bits, NULL, NULL);
			OutTrace("!!! hbmp=%x err=%d bcount=%d\n", hbmp, GetLastError(), dxw.VirtualPixelFormat.dwRGBBitCount);
			if(hbmp){
				(*pSelectObject)(hdc, hbmp);
				(*pDeleteObject)((HGDIOBJ)hbmp);
			}
			else{
				char buf[81];
				sprintf(buf, "hbmp NULL err=%d", GetLastError());
				MessageBox(0, buf, "dxwnd", 0);
			}
		}
	}

	ret=(*pCreateCompatibleBitmap)(hdc, nWidth, nHeight);

#ifndef DXW_NOTRACES
	if(ret) {
		OutTraceGDI("%s: hbitmap=%#x\n", ApiRef, ret); 
	} else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	}
#endif // DXW_NOTRACES
	return ret;
}

COLORREF WINAPI extSetPixel(HDC hdc, int X, int Y, COLORREF crColor)
{
	COLORREF ret;
	ApiName("SetPixel");
	OutTraceGDI("%s: hdc=%#x color=%#x point=(%d,%d)\n", ApiRef, hdc, crColor, X, Y);

	if(dxw.IsToRemap(hdc)){

		switch(dxw.GDIEmulationMode){
			case GDIMODE_ASYNCDC:
				ret=(*pSetPixel)(lpADC->GetDC(hdc), X, Y, crColor);
				lpADC->ReleaseDC();
				return ret;
				break;
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pSetPixel)(sdc.GetHdc(), X, Y, crColor);
				sdc.PutPrimaryDC(hdc, TRUE, X, Y, 1, 1); // ????
				return ret; // this returns a COLORREF type
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&X, &Y);
				OutTraceGDI("%s: fixed pos=(%d,%d)\n", ApiRef, X, Y);
				break;
		}
	}

	ret=(*pSetPixel)(hdc, X, Y, crColor);
	// both 0x00000000 and 0xFFFFFFFF are legitimate colors and therefore valid return codes...
	//if(ret==GDI_ERROR) OutErrorGDI("SetPixel: ERROR ret=%#x err=%d\n", ret, GetLastError()); 
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extSetPixelV(HDC hdc, int X, int Y, COLORREF crColor)
{
	BOOL ret;
	ApiName("SetPixelV");
	OutTraceGDI("%s: hdc=%#x color=%#x point=(%d,%d)\n", ApiRef, hdc, crColor, X, Y);

	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pSetPixelV)(sdc.GetHdc(), X, Y, crColor);
				sdc.PutPrimaryDC(hdc, TRUE, X, Y, 1, 1); // ????
				return ret; 
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&X, &Y);
				OutTraceGDI("%s: fixed pos=(%d,%d)\n", ApiRef, X, Y);
				break;
		}
	}

	ret=(*pSetPixelV)(hdc, X, Y, crColor);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extEllipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	int ret;
	ApiName("Ellipse");
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d)\n", ApiRef, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pEllipse)(sdc.GetHdc(),nLeftRect, nTopRect, nRightRect, nBottomRect);
				sdc.PutPrimaryDC(hdc, TRUE, nLeftRect, nTopRect, nRightRect-nLeftRect, nBottomRect-nTopRect);
				return ret; 
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nLeftRect, &nTopRect, &nRightRect, &nBottomRect);
				OutTraceGDI("%s: fixed dest=(%d,%d)-(%d,%d)\n", ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect);
				break;
		}
	}

	ret=(*pEllipse)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extPolygon(HDC hdc, const POINT *lpPoints, int cCount)
{
	BOOL ret;
	ApiName("Polygon");

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		int i;
		OutTrace("%s: hdc=%#x cCount=%d\n", ApiRef, hdc, cCount); 
		if(IsDebugGDI){
			for(i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
		}
	}
#endif

	if (dxw.IsToRemap(hdc)){
		int i, size;
		POINT *lpRemPoints;
		
		size = cCount * sizeof(POINT);
		lpRemPoints = (LPPOINT)malloc(size);
		memcpy(lpRemPoints, lpPoints, size);

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				free(lpRemPoints);
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolygon)(sdc.GetHdc(), lpPoints, cCount);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret; 
				break;
			case GDIMODE_STRETCHED: 
				for(i=0; i<cCount; i++) {
					dxw.MapClient(&lpRemPoints[i]);
				}
#ifndef DXW_NOTRACES
				if(IsTraceGDI){
					OutTrace("%s: fixed cCount=%d\n", ApiRef, cCount); 
					if(IsDebugGDI){
						for(i=0; i<cCount; i++) OutTrace("> pt[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
					}
				}
#endif
				break;
			case GDIMODE_EMULATED:
				break;	
		}
		ret=(*pPolygon)(hdc, lpRemPoints, cCount);
		free(lpRemPoints);
	}
	else {
		ret=(*pPolygon)(hdc, lpPoints, cCount);
	}
	_if(!ret)OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extArc(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nXStartArc, int nYStartArc, int nXEndArc, int nYEndArc)
{
	int ret;
	ApiName("Arc");
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d) start=(%d,%d) end=(%d,%d)\n", 
		ApiRef, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXStartArc, nYStartArc, nXEndArc, nYEndArc);

	if (dxw.IsToRemap(hdc)){
		dxw.MapClient(&nLeftRect, &nTopRect, &nRightRect, &nBottomRect);
		dxw.MapClient(&nXStartArc, &nYStartArc, &nXEndArc, &nYEndArc);
		OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d) start=(%d,%d) end=(%d,%d)\n", 
			ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect, nXStartArc, nYStartArc, nXEndArc, nYEndArc);
	}

	ret=(*pArc)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXStartArc, nYStartArc, nXEndArc, nYEndArc);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extMaskBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc,
					 int nXSrc, int nYSrc, HBITMAP hbmMask, int xMask, int yMask, DWORD dwRop)
{
	// found in "Spearhead"
	BOOL ret;
	ApiName("MaskBlt");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hdcDest=%#x pos=(%d,%d) size=(%dx%d) hdcSrc=%#x pos=(%d,%d) hbmMask=%#x Mask=(%d,%d) dwRop=%#x\n",
		ApiRef, hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, hbmMask, xMask, yMask, dwRop);
#endif

	if (dxw.IsToRemap(hdcDest)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_ASYNCDC:
				ret=(*pMaskBlt)(lpADC->GetDC(hdcDest), nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, hbmMask, xMask, yMask, dwRop);
				lpADC->ReleaseDC();
				return ret;
				break;
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdcDest);
				ret=(*pMaskBlt)(sdc.GetHdc(), nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, hbmMask, xMask, yMask, dwRop);
				sdc.PutPrimaryDC(hdcDest, TRUE, nXDest, nYDest, nWidth, nHeight);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nXDest, &nYDest, &nWidth, &nHeight);
				OutTraceGDI("%s: fixed pos=(%d,%d) size=(%dx%d)\n", ApiRef, nXDest, nYDest, nWidth, nHeight);
				break;
			case GDIMODE_EMULATED:
				// to be implemented
				break;
			default:
				break;
		}
	}

	ret=(*pMaskBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, hbmMask, xMask, yMask, dwRop);
	_if(!ret) OutErrorGDI("%s: ERROR rerr=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extSetViewportOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("SetViewportOrgEx");
	OutTraceGDI("%s: hdc=%#x pos=(%d,%d)\n", ApiRef, hdc, X, Y);

 	if (dxw.IsToRemap(hdc)){
		dxw.MapClient(&X, &Y);
		OutTraceGDI("%s: fixed origin=(%d,%d)\n", ApiRef, X, Y);
	}
	ret=(*pSetViewportOrgEx)(hdc, X, Y, lpPoint);
	if(ret && lpPoint) {
		OutTraceGDI("%s: previous ViewPortOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
		dxw.UnmapClient(lpPoint);
		OutTraceGDI("%s: fixed previous ViewPort=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
	}

	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

// v2.06.13: since graphic is scaled in blit operations, viewport extension should remain unscaled
// hopefully fixed to 1 pixel
BOOL WINAPI extSetViewportExtEx(HDC hdc, int X, int Y, LPSIZE lpSize)
{
	BOOL ret;
	ApiName("SetViewportExtEx");
	OutTraceGDI("%s: hdc=%#x pos=(%d,%d)\n", ApiRef, hdc, X, Y);

	ret=(*pSetViewportExtEx)(hdc, X, Y, lpSize);

	_if(ret && lpSize) {
		OutTraceGDI("%s: previous ViewPortExt=(%d,%d)\n", ApiRef, lpSize->cx, lpSize->cy);
	}
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extGetViewportOrgEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("GetViewportOrgEx");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	ret=(*pGetViewportOrgEx)(hdc, lpPoint);
 	if (dxw.IsToRemap(hdc)){
		dxw.UnmapClient(lpPoint);
		OutTraceGDI("%s: fixed ViewportOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
	}

	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

// v2.06.13: since graphic is scaled in blit operations, viewport extension should remain unscaled
// hopefully fixed to 1 pixel
BOOL WINAPI extGetViewportExtEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("GetViewportExtEx");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	ret=(*pGetViewportExtEx)(hdc, lpPoint);
	if(ret && lpPoint) {
		OutTraceGDI("%s: prev. ViewportExt=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
	}

	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extGetWindowOrgEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("GetWindowOrgEx");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	ret=(*pGetWindowOrgEx)(hdc, lpPoint);

 	if (dxw.Windowize){
		if(dxw.IsFullScreen()){
			lpPoint->x -= dxw.iPosX;
			lpPoint->y -= dxw.iPosY;
		}
		dxw.UnmapClient(lpPoint);
		OutTraceGDI("%s: fixed origin=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
	}

	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extSetWindowOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("SetWindowOrgEx");
	OutTraceGDI("%s: hdc=%#x origin=(%d,%d)\n", ApiRef, hdc, X, Y);

 	if (dxw.Windowize){
		dxw.MapClient(&X, &Y);
		OutTraceGDI("%s: fixed origin=(%d,%d)\n", ApiRef, X, Y);
	}
	if(dxw.IsFullScreen()){
		X += dxw.iPosX;
		Y += dxw.iPosY;
	}
	ret=(*pSetWindowOrgEx)(hdc, X, Y, lpPoint);
	if(ret && lpPoint) {
		OutTraceGDI("%s: previous WindowOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
		dxw.UnmapWindow(lpPoint);
		OutTraceGDI("%s: fixed previous WindowOrg=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
	}

	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

// v2.06.13: since window position is scaled in move operations, window extension should remain unscaled
BOOL WINAPI extSetWindowExtEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("SetWindowExtEx");
	OutTraceGDI("%s: hdc=%#x extent=(%d,%d)\n", ApiRef, hdc, X, Y);

	ret=(*pSetWindowExtEx)(hdc, X, Y, lpPoint);
	if(ret && lpPoint) {
		OutTraceGDI("%s: previous extent=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
	}

	return ret;
}

BOOL WINAPI extGetWindowExtEx(HDC hdc, LPSIZE lpsize)
{
	BOOL ret;
	ApiName("GetWindowExtEx");

	ret = (*pGetWindowExtEx)(hdc, lpsize);
	if(ret && lpsize){
		OutTraceGDI("%s: hdc=%#x extent=(%d,%d)\n", ApiRef, hdc, lpsize->cx, lpsize->cy);
	}
	if(!ret){
		OutErrorGDI("%s: hdc=#x ERROR err=%d\n", ApiRef, hdc, GetLastError());
	}
	return ret;
}

BOOL WINAPI extGetCurrentPositionEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ApiName("GetCurrentPositionEx");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	ret=(*pGetCurrentPositionEx)(hdc, lpPoint);
	if(ret) {
		OutTraceGDI("%s: pos=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
		if (dxw.IsToRemap(hdc)){
			switch(dxw.GDIEmulationMode){
				case GDIMODE_STRETCHED:
					dxw.UnmapClient(lpPoint);
					break;
				case GDIMODE_SHAREDDC:
					sdc.GetPrimaryDC(hdc);
					ret=(*pGetCurrentPositionEx)(sdc.GetHdc(), lpPoint);
					sdc.PutPrimaryDC(hdc, FALSE);
					return ret;
					break;
				case GDIMODE_EMULATED:
				default:
					break;
			}
			OutTraceGDI("%s: fixed pos=(%d,%d)\n", ApiRef, lpPoint->x, lpPoint->y);
		}
	}
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extCreateScalableFontResourceA(DWORD fdwHidden, LPCTSTR lpszFontRes, LPCTSTR lpszFontFile, LPCTSTR lpszCurrentPath)
{
	BOOL res;
	ApiName("CreateScalableFontResourceA");
	OutTraceGDI("%s: hidden=%d FontRes=\"%s\" FontFile=\"%s\" CurrentPath=\"%s\"\n",
		ApiRef, fdwHidden, lpszFontRes, lpszFontFile, lpszCurrentPath);
	if(dxw.dwFlags3 & FONTBYPASS) return TRUE;
	res=(*pCreateScalableFontResourceA)(fdwHidden, lpszFontRes, lpszFontFile, lpszCurrentPath);
	_if(!res) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

BOOL WINAPI extCreateScalableFontResourceW(DWORD fdwHidden, LPCWSTR lpszFontRes, LPCWSTR lpszFontFile, LPCWSTR lpszCurrentPath)
{
	BOOL res;
	ApiName("CreateScalableFontResourceW");
	OutTraceGDI("%s: hidden=%d FontRes=\"%ls\" FontFile=\"%ls\" CurrentPath=\"%ls\"\n",
		ApiRef, fdwHidden, lpszFontRes, lpszFontFile, lpszCurrentPath);
	if(dxw.dwFlags3 & FONTBYPASS) return TRUE;
	res=(*pCreateScalableFontResourceW)(fdwHidden, lpszFontRes, lpszFontFile, lpszCurrentPath);
	_if(!res) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

int WINAPI extAddFontResourceA(LPCTSTR lpszFontFile)
{
	BOOL res;
	ApiName("AddFontResourceA");
	OutTraceGDI("%s: FontFile=\"%s\"\n", ApiRef, lpszFontFile);
	if(dxw.dwFlags3 & FONTBYPASS) {
		OutTraceDW("%s: SUPPRESS FontFile=\"%s\"\n", ApiRef, lpszFontFile);
		return TRUE;
	}
	res=(*pAddFontResourceA)(lpszFontFile);
	_if(!res) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

int WINAPI extRemoveFontResourceA(LPCTSTR lpszFontFile)
{
	BOOL res;
	ApiName("RemoveFontResourceA");
	OutTraceGDI("RemoveFontResource: FontFile=\"%s\"\n", lpszFontFile);
	if(dxw.dwFlags3 & FONTBYPASS) {
		OutTraceDW("%s: SUPPRESS FontFile=\"%s\"\n", ApiRef, lpszFontFile);
		return TRUE;
	}
	res=(*pRemoveFontResourceA)(lpszFontFile);
	_if(!res) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

int WINAPI extAddFontResourceW(LPCWSTR lpszFontFile)
{
	BOOL res;
	ApiName("AddFontResourceW");
	OutTraceGDI("%s: FontFile=\"%ls\"\n", ApiRef, lpszFontFile);
	if(dxw.dwFlags3 & FONTBYPASS) {
		OutTraceDW("%s: SUPPRESS FontFile=\"%ls\"\n", ApiRef, lpszFontFile);
		return TRUE;
	}
	res=(*pAddFontResourceW)(lpszFontFile);
	_if(!res) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

int WINAPI extRemoveFontResourceW(LPCWSTR lpszFontFile)
{
	BOOL res;
	ApiName("RemoveFontResourceW");
	OutTraceGDI("%s: FontFile=\"%ls\"\n", ApiRef, lpszFontFile);
	if(dxw.dwFlags3 & FONTBYPASS) {
		OutTraceDW("%s: SUPPRESS FontFile=\"%ls\"\n", ApiRef, lpszFontFile);
		return TRUE;
	}
	res=(*pRemoveFontResourceW)(lpszFontFile);
	_if(!res) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

#ifdef TRACEFONTS
int WINAPI extEnumFontsA(HDC hdc, LPCSTR lpFaceName, FONTENUMPROC lpFontFunc, LPARAM lParam)
{
	int res;
	ApiName("EnumFonts");
	OutTraceGDI("%s: hdc=%#x facename=\"%s\" fontfunc=%#x lparam=%#x\n", ApiRef, hdc, lpFaceName, lpFontFunc, lParam);
	res = (*pEnumFontsA)(hdc, lpFaceName, lpFontFunc, lParam);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

int WINAPI extEnumFontFamiliesA(HDC hdc, LPCSTR lpLogfont, FONTENUMPROCA lpProc, LPARAM lParam)
{
	int res;
	ApiName("EnumFontFamiliesA");
	OutTraceGDI("%s: hdc=%#x logfont=\"%s\" fontfunc=%#x lparam=%#x\n", ApiRef, hdc, lpLogfont, lpProc, lParam);
	res = (*pEnumFontFamiliesA)(hdc, lpLogfont, lpProc, lParam);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif

//BEWARE: SetPixelFormat must be issued on the same hdc used by OpenGL wglCreateContext, otherwise 
// a failure err=2000 ERROR INVALID PIXEL FORMAT occurs!!
//
//Remarks: https://msdn.microsoft.com/en-us/library/ms537559(VS.85).aspx
//
// If hdc references a window, calling the SetPixelFormat function also changes the pixel format of the window. 
// Setting the pixel format of a window more than once can lead to significant complications for the Window 
// Manager and for multithread applications, so it is not allowed. An application can only set the pixel format
// of a window one time. Once a window's pixel format is set, it cannot be changed.

BOOL WINAPI dxwSetPixelFormat(ApiArg, GDISetPixelFormat_Type pSetPixelFormat, HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
	BOOL res;
	//ApiName("SetPixelFormat");
	BOOL bRemappedDC = FALSE;
	static int iCounter = 0;
	static HDC hLastDC = 0;

	// v2.04.87: ppfd can be NULL (in "Neverwinter Nights Platinum")
	if(IsTraceGDI){
		if(ppfd){
			OutTrace("%s: hdc=%#x PixelFormat=%d ppfd={Flags=%#x PixelType=%#x(%s) ColorBits=%d RGBdepth=(%d,%d,%d) RGBshift=(%d,%d,%d)}\n", 
				ApiRef, hdc, iPixelFormat, 
				ppfd->dwFlags, ppfd->iPixelType, ppfd->iPixelType?"PFD_TYPE_COLORINDEX":"PFD_TYPE_RGBA", ppfd->cColorBits,
				ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits,
				ppfd->cRedShift, ppfd->cGreenShift, ppfd->cBlueShift);
		} else {
			OutTrace("%s: hdc=%#x PixelFormat=%d ppfd=(NULL)\n", 
				ApiRef, hdc, iPixelFormat);
		}
	}

	if(dxw.Windowize && dxw.IsRealDesktopDC(hdc)){
		HDC oldhdc = hdc;
		hdc=(*pGDIGetDC)(dxw.GethWnd()); // potential leakage
		bRemappedDC = TRUE;
		OutTraceGDI("%s: remapped desktop hdc=%#x->%#x hWnd=%#x\n", ApiRef, oldhdc, hdc, dxw.GethWnd());
	}

	if(dxw.IsEmulated) {
		if (dxw.dwFlags12 & NOSETPIXELFORMAT) {
			OutTraceDW("%s: BYPASS\n", ApiRef); 
			return TRUE;
		}
		// this block must be disabled to allow switch to non emulable color rendering like OpenGL
#ifdef DISABLED_FOR_NONEMULABLE_MODES
		//PIXELFORMATDESCRIPTOR pfd;
		//res = (*pDescribePixelFormat)(hdc, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
		//if(!res) {
		//	OutErrorGDI("%s: DescribePixelFormat ERROR err=%d\n", ApiRef, GetLastError());
		//	return res;
		//}
		//int bpp = pfd.cRedBits + pfd.cGreenBits + pfd.cBlueBits + pfd.cAlphaBits;
		//OutTraceDW("%s: setting bpp=%d\n", ApiRef, bpp);
		//dxw.VirtualPixelFormat.dwRGBBitCount = bpp;
		//return TRUE;
#endif
	}

	// from here, non emulated colors
	if(iCounter && (hdc == hLastDC))
		res = TRUE; // avoid calling SetPixelFormat more than once on same hdc
	else {
		res=(*pSetPixelFormat)(hdc, iPixelFormat, ppfd);
		iCounter++;
		hLastDC = hdc;
	}
	if(dxw.Windowize && bRemappedDC) (*pGDIReleaseDC)(dxw.GethWnd(), hdc); // fixed DC leakage
	if (ppfd) dxw.ActualPixelFormat.dwRGBBitCount = ppfd->cColorBits;
	_if(!res) OutErrorGDI("%s: ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
	return res;
}

BOOL WINAPI extGDISetPixelFormat(HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{ ApiName("SetPixelFormat"); return dxwSetPixelFormat(ApiRef, pGDISetPixelFormat, hdc, iPixelFormat, ppfd); }

int WINAPI dxwGetPixelFormat(ApiArg, GDIGetPixelFormat_Type pGetPixelFormat, HDC hdc)
{
	int res;
	//ApiName("GetPixelFormat");
	BOOL bRemappedDC = FALSE;
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);
	if(dxw.Windowize && dxw.IsDesktop(WindowFromDC(hdc))){
		HDC oldhdc = hdc;
		hdc=(*pGDIGetDC)(dxw.GethWnd()); // potential DC leakage
		bRemappedDC = TRUE;
		OutTraceGDI("%s: remapped desktop hdc=%#x->%#x hWnd=%#x\n", ApiRef, oldhdc, hdc, dxw.GethWnd());
	}	
	res=(*pGetPixelFormat)(hdc);
#ifndef DXW_NOTRACES
	if(!res) {
		OutErrorGDI("%s: ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
	}
	else {
		OutTraceGDI("%s: res=%d\n", ApiRef, res);
	}
#endif // DXW_NOTRACES
	if(dxw.Windowize && bRemappedDC)(*pGDIReleaseDC)(dxw.GethWnd(), hdc); // fixed DC leakage
	return res;
}

int WINAPI extGDIGetPixelFormat(HDC hdc)
{ ApiName("GetPixelFormat"); return dxwGetPixelFormat(ApiRef, pGDIGetPixelFormat, hdc); }

int WINAPI dxwChoosePixelFormat(ApiArg, ChoosePixelFormat_Type pChoosePixelFormat, HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd)
{
	int res;
	BOOL bRemappedDC = FALSE;
	PIXELFORMATDESCRIPTOR pfd;

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char fbuf[256];
		OutTrace("%s: hdc=%#x Flags=%#x(%s) PixelType=%#x(%s) ColorBits=%d RGBdepth=(%d,%d,%d) RGBshift=(%d,%d,%d)\n", 
			ApiRef, hdc, 
			ppfd->dwFlags, ExplainPFFlags(ppfd->dwFlags, fbuf, 256),
			ppfd->iPixelType, ppfd->iPixelType?"PFD_TYPE_COLORINDEX":"PFD_TYPE_RGBA", ppfd->cColorBits,
			ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits,
			ppfd->cRedShift, ppfd->cGreenShift, ppfd->cBlueShift);
	}
#endif // DXW_NOTRACES

	if(dxw.Windowize && dxw.IsRealDesktopDC(hdc)){
		HDC oldhdc = hdc;
		hdc=(*pGDIGetDC)(dxw.GethWnd()); // potential leakage
		bRemappedDC = TRUE;
		OutTraceGDI("%s: remapped desktop hdc=%#x->%#x hWnd=%#x\n", ApiRef, oldhdc, hdc, dxw.GethWnd());
		if(dxw.IsEmulated && ppfd) {
			pfd = *ppfd;
			pfd.cColorBits = (BYTE)dxw.ActualPixelFormat.dwRGBBitCount;
			ppfd = &pfd;
		}
	}

	res=(*pChoosePixelFormat)(hdc, ppfd);
	if(dxw.Windowize && bRemappedDC) (*pGDIReleaseDC)(dxw.GethWnd(), hdc); // fixed DC leakage

#ifndef DXW_NOTRACES
	if(!res) {
		OutErrorGDI("%s: ERROR err=%d at=%d\n", ApiRef, GetLastError(), __LINE__);
	}
	else {
		OutTraceGDI("%s: res=%d\n", ApiRef, res);
	}
#endif // DXW_NOTRACES
	return res;
}

int WINAPI extChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd)
{ ApiName("ChoosePixelFormat"); return dxwChoosePixelFormat(ApiRef, pChoosePixelFormat, hdc, ppfd); }

int WINAPI dxwDescribePixelFormat(ApiArg, DescribePixelFormat_Type pDescribePixelFormat, HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
	int res;
	//ApiName("DescribePixelFormat");
	OutTraceGDI("%s: hdc=%#x PixelFormat=%d Bytes=%d\n", ApiRef, hdc, iPixelFormat, nBytes);

	res=(*pDescribePixelFormat)(hdc, iPixelFormat, nBytes, ppfd);
	if(!res){
		OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return res;
	}
	if (ppfd && (nBytes==sizeof(PIXELFORMATDESCRIPTOR))){
		char fbuf[256];
		OutTraceGDI("%s: res=%d Flags=%#x(%s) PixelType=%#x(%s) ColorBits=%d RGBdepth=(%d,%d,%d) RGBshift=(%d,%d,%d)\n", 
			ApiRef, res, 
			ppfd->dwFlags, ExplainPFFlags(ppfd->dwFlags, fbuf, 256), ppfd->iPixelType, 
			ppfd->iPixelType?"PFD_TYPE_COLORINDEX":"PFD_TYPE_RGBA", ppfd->cColorBits,
			ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits,
			ppfd->cRedShift, ppfd->cGreenShift, ppfd->cBlueShift);

		// v2.05.93 fix: generalize the detection of a real or virtual desktop window
		// fixes The OpenGL rendering of "Magna Carta".
		//if((hdc==0) && dxw.IsFullScreen() && (ppfd->iPixelType==PFD_TYPE_RGBA)){ 
		//if(dxw.IsDesktop(WindowFromDC(hdc)) && dxw.IsFullScreen() && (ppfd->iPixelType==PFD_TYPE_RGBA)){ 
		if(dxw.IsEmulated && 
			dxw.IsDesktop(WindowFromDC(hdc)) && 
			dxw.IsFullScreen() && 
			(ppfd && ppfd->iPixelType==PFD_TYPE_RGBA)){ 
			OutTraceGDI("%s: emulating virtual desktop pixelformat bpp=%d\n", ApiRef, dxw.VirtualPixelFormat.dwRGBBitCount); 
			switch(dxw.VirtualPixelFormat.dwRGBBitCount){
				case 8:
					ppfd->cColorBits = 8;
					OutTrace("colorbits=%d to be fixed!\n", ppfd->cColorBits);
					break;
				case 16:
					ppfd->cColorBits = 16;
					switch(dxw.VirtualPixelFormat.dwGBitMask){
						case 0x0007E0: // RGB565
							ppfd->cColorBits=16;
							ppfd->cRedBits=5;
							ppfd->cRedShift=0;
							ppfd->cGreenBits=6;
							ppfd->cGreenShift=5;
							ppfd->cBlueBits=5;
							ppfd->cBlueShift=11;
							ppfd->cAlphaBits=0;
							ppfd->cAlphaShift=0;
							break;
						case 0x0003E0: // RGB555
							ppfd->cColorBits=15;
							ppfd->cRedBits=5;
							ppfd->cRedShift=0;
							ppfd->cGreenBits=5;
							ppfd->cGreenShift=5;
							ppfd->cBlueBits=5;
							ppfd->cBlueShift=10;
							ppfd->cAlphaBits=1;
							ppfd->cAlphaShift=15;
							break;
					}
					break;
				case 24:
					_if (ppfd->cColorBits != 24) OutTrace("colorbits=%d to be fixed!\n", ppfd->cColorBits);
					break;
				case 32:
					_if (ppfd->cColorBits != 24) OutTrace("colorbits=%d to be fixed!\n", ppfd->cColorBits);
					break;
				default:
					OutTrace("colorbits=%d to be fixed!\n", ppfd->cColorBits);
					break;
			}
		}
	}
	OutTraceGDI("%s: res=%d\n", ApiRef, res);
	return res;
}

int WINAPI extDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{ ApiName("DescribePixelFormat"); return dxwDescribePixelFormat(ApiRef, pDescribePixelFormat, hdc, iPixelFormat, nBytes, ppfd); }

DWORD WINAPI extGetObjectType(HGDIOBJ h)
{
	DWORD res;
	res=(*pGetObjectType)(h);
	OutTraceGDI("GetObjectType: h=%#x type=%#x\n", h, res);
	if((dxw.GDIEmulationMode == GDIMODE_EMULATED) && (h == dxw.VirtualHDC)) {
		OutTraceGDI("GetObjectType: REMAP h=%#x type=%#x->%#x\n", h, res, OBJ_DC);
		res=OBJ_DC;
	}
	return res;
}

extern BOOL gFixed;

BOOL WINAPI extExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, const RECT *lprc, LPCSTR lpString, UINT cbCount, const INT *lpDx)
{
	RECT rc;
	BOOL ret;
	ApiName("ExtTextOutA");

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sRect[81];
		if(lprc) sprintf(sRect, "(%d,%d)-(%d,%d)", lprc->left, lprc->top, lprc->right, lprc->bottom);
		else strcpy(sRect, "NULL");
		OutTrace("%s: hdc=%#x opt=%#x pos=(%d,%d) String=(%d)\"%.*s\" rect=%s\n", 
			ApiRef, hdc, fuOptions, X, Y, cbCount, cbCount, lpString, sRect);
		OutHexSYS((LPBYTE)lpString, cbCount);
	}
#endif

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextA(ApiRef, X, Y, lpString, cbCount);
	
	if(lprc) rc = *lprc; // copy to a local RECT that could be scaled if necessary

	// useless (but not wrong!) conversion, once selected the proper charset codepage 
	// you can print also with UTF8 strings.
	// v2.06.04: uncommented, it could be necessary ...
	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		LPWSTR wstr = NULL;
		int n;
		int size = lstrlenA(lpString);
		if(size == 0) return TRUE;
		wstr = (LPWSTR)malloc((size+1)<<1);
		n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpString, size, wstr, size);
		wstr[n] = (WCHAR)0; // add terminator !!
		_if(n==0) OutTrace("!!! err=%d @%d\n", GetLastError(), __LINE__);
		if(!pExtTextOutW) pExtTextOutW = ExtTextOutW;
		ret = extExtTextOutW(hdc, X, Y, fuOptions, lprc, wstr, n, lpDx);
		free(wstr);
		return ret;
	}

	if (dxw.IsToRemap(hdc) && !gFixed){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pExtTextOutA)(sdc.GetHdc(), X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
				if(lprc){
					sdc.PutPrimaryDC(hdc, TRUE, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
				}
				else
					sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&X, &Y);
				if(lprc) {
					dxw.MapClient(&rc);
				}
				OutTraceGDI("%s: fixed pos=(%d,%d)\n", ApiRef, X, Y);
				break;
		}
	}

	if(lprc)
		ret = (*pExtTextOutA)(hdc, X, Y, fuOptions, &rc, lpString, cbCount, lpDx);
	else
		ret = (*pExtTextOutA)(hdc, X, Y, fuOptions, NULL, lpString, cbCount, lpDx);

	if(!ret) OutErrorGDI("%s: ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	return ret;
}

BOOL WINAPI extExtTextOutW(HDC hdc, int X, int Y, UINT fuOptions, const RECT *lprc, LPCWSTR lpString, UINT cbCount, const INT *lpDx)
{
	RECT rc;
	BOOL ret;
	ApiName("ExtTextOutW");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sRect[81];
		if(lprc) sprintf(sRect, "(%d,%d)-(%d,%d)", lprc->left, lprc->top, lprc->right, lprc->bottom);
		else strcpy(sRect, "NULL");
		//OutTrace("%s: hdc=%#x opt=%#x pos=(%d,%d) String=\"%.*ls\" rect=%s\n", 
		//	ApiRef, hdc, fuOptions, X, Y, cbCount, lpString, sRect);
		OutTrace("%s: hdc=%#x opt=%#x pos=(%d,%d) count=%d rect=%s\n", 
			ApiRef, hdc, fuOptions, X, Y, cbCount, sRect);
		OutTrace("%s: String=(%d)\"%.*ls\"\n", ApiRef, cbCount, cbCount, lpString);
		OutHexSYS((LPBYTE)lpString, cbCount<<1);
	}
#endif

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextW(ApiRef, X, Y, lpString, cbCount);

	if(lprc) rc = *lprc; // copy to a local RECT that could be scaled if necessary

	if (dxw.IsToRemap(hdc) && !gFixed){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pExtTextOutW)(sdc.GetHdc(), X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
				if(lprc){
					sdc.PutPrimaryDC(hdc, TRUE, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
				}
				else
					sdc.PutPrimaryDC(hdc, TRUE);
			return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&X, &Y);
				if(lprc) {
					dxw.MapClient(&rc);
				}
				OutTraceGDI("%s: fixed pos=(%d,%d)\n", ApiRef, X, Y);
				break;
		}
	}

	if(lprc)
		ret = (*pExtTextOutW)(hdc, X, Y, fuOptions, &rc, lpString, cbCount, lpDx);
	else
		ret = (*pExtTextOutW)(hdc, X, Y, fuOptions, NULL, lpString, cbCount, lpDx);

	if(!ret) OutErrorGDI("%s: ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	return ret;
}

#ifdef TRACEPALETTE
BOOL WINAPI extResizePalette(HPALETTE hpal, UINT nEntries)
{
	OutTrace("ResizePalette: hpal=%#x nEntries=%d\n", hpal, nEntries);
	return (*pResizePalette)(hpal, nEntries);
}
#endif

#ifndef DXW_NOTRACES
char *sBkMode(int m)
{
	char *s;
	switch(m){
		case TRANSPARENT: s="TRANSPARENT"; break;
		case OPAQUE: s="OPAQUE"; break;
		default:s="invalid"; break;
	}
	return s;
}
#endif

#ifdef TRACEALL
int WINAPI extSetBkMode(HDC hdc, int iBkMode)
{
	int ret;
	ApiName("SetBkMode");
	OutTraceGDI("%s: hdc=%#x bkmode=%#x(%s)\n", ApiRef, hdc, iBkMode, sBkMode(iBkMode));
	ret = (*pSetBkMode)(hdc, iBkMode);
	OutTraceGDI("%s: ret=%#x(%s)\n", ApiRef, ret, sBkMode(ret));
	return ret;
}
#endif // TRACEALL

COLORREF WINAPI extSetTextColor(HDC hdc, COLORREF crColor)
{
	COLORREF ret;
	ApiName("SetTextColor");
	OutTraceGDI("%s: hdc=%#x color=%#x\n", ApiRef, hdc, crColor);

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC) {
		ret = (*pSetTextColor)(lpADC->GetDC(hdc), crColor);
		lpADC->ReleaseDC();
		return ret;
	}

	if(dxw.dwFlags12 & W98OPAQUEFONT) crColor &= 0x00FFFFFF; // strip alpha 

	ret = (*pSetTextColor)(hdc, crColor);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

COLORREF WINAPI extGetPixel(HDC hdc, int nXPos, int nYPos)
{
	COLORREF ret;
	ApiName("GetPixel");
	OutTraceGDI("%s: hdc=%#x pos(x,y)=(%d,%d)\n", ApiRef, hdc, nXPos, nYPos);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pGetPixel)(sdc.GetHdc(), nXPos, nYPos);
				sdc.PutPrimaryDC(hdc, FALSE);
				return ret;
				break;			
			default:
				// to do .....
				break;
		}
	}
	
	ret=(*pGetPixel)(hdc, nXPos, nYPos);
	if(ret==CLR_INVALID) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	else {
		OutTraceGDI("%s: color=%#x\n", ApiRef, ret);
	}

	return ret;
}

BOOL WINAPI extPlgBlt(HDC hdcDest, const POINT *lpPoint, HDC hdcSrc, int nXSrc, int nYSrc, int nWidth, int nHeight, HBITMAP hbmMask, int xMask, int yMask)
{
	// found in "Spearhead" 
	MessageBox(0, "PlgBlt", "DxWnd", MB_OK);
	return (COLORREF)0;
}

BOOL WINAPI extChord(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2)
{
	// v2.04.96: found in "Fallen Haven"
	BOOL ret;
	ApiName("Chord");
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d) rad1=(%d,%d) rad2=(%d,%d)\n", 
		ApiRef, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);

	if (dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pChord)(sdc.GetHdc(), nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nLeftRect, &nTopRect, &nRightRect, &nBottomRect);
				dxw.MapClient(&nXRadial1, &nYRadial1);
				dxw.MapClient(&nXRadial2, &nYRadial2);
				OutTraceDW("%s: FIXED rect=(%d,%d)-(%d,%d) rad1=(%d,%d) rad2=(%d,%d)\n", 
					ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
				break;
		}
	}
	ret = (*pChord)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extPolyTextOutA(HDC hdc, const POLYTEXTA *pptxt, int cStrings)
{
	MessageBox(0, "PolyTextOutA", "DxWnd", MB_OK);
	return TRUE;
}

BOOL WINAPI extPolyTextOutW(HDC hdc, const POLYTEXTW *pptxt, int cStrings)
{
	MessageBox(0, "PolyTextOutW", "DxWnd", MB_OK);
	return TRUE;
}

HBITMAP WINAPI extCreateDIBitmap(HDC hdc, BITMAPINFOHEADER *lpbmih, DWORD fdwInit, const VOID *lpbInit, const BITMAPINFO *lpbmi, UINT fuUsage)
{
	HBITMAP ret;
	ApiName("CreateDIBitmap");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("%s: hdc=%#x init=%#x%s data=%#x usage=%#x(%s)\n", 
			ApiRef, hdc, fdwInit, fdwInit==CBM_INIT?"(CBM_INIT)":"", lpbInit, 
			fuUsage, ExplainDIBUsage(fuUsage));
		if(fdwInit==CBM_INIT) TraceBITMAPINFOHEADER("CreateDIBitmap(lpbmih)", lpbmih);
		TraceBITMAPINFOHEADER("CreateDIBitmap(lpbmi)", (BITMAPINFOHEADER *)&(lpbmi->bmiHeader));
	}
#endif

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pCreateDIBitmap)(sdc.GetHdc(), lpbmih, fdwInit, lpbInit, lpbmi, fuUsage);
				_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
				sdc.PutPrimaryDC(hdc, FALSE);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret = (*pCreateDIBitmap)(hdc, lpbmih, fdwInit, lpbInit, lpbmi, fuUsage);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	if(ret && (fdwInit & CBM_INIT) && (dxw.dwDFlags2 & DUMPBITMAPS)) DumpBitmap(ApiRef, ret);
	return ret;
}

HBITMAP WINAPI extCreateDIBSection(HDC hdc, const BITMAPINFO *pbmi, UINT iUsage, VOID **ppvBits, HANDLE hSection, DWORD dwOffset)
{
	ApiName("CreateDIBSection");
	HBITMAP ret;
	OutTraceGDI("%s: hdc=%#x bmi={%s} usage=%s hsect=%#x offset=%#x\n", ApiRef, hdc, sBMIDump((BITMAPINFO *)pbmi), ExplainDIBUsage(iUsage), hSection, dwOffset);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pCreateDIBSection)(sdc.GetHdc(), pbmi, iUsage, ppvBits, hSection, dwOffset);
				sdc.PutPrimaryDC(hdc, FALSE);
				break;			
			default:
				ret=(*pCreateDIBSection)(hdc, pbmi, iUsage, ppvBits, hSection, dwOffset);
				break;
		}
	}
	else {
		ret=(*pCreateDIBSection)(hdc, pbmi, iUsage, ppvBits, hSection, dwOffset); 
	}

	// v2.06.14: On Win8.1 the operation fails but the error code is 0 ...
	//if(!ret && (GetLastError() == ERROR_INVALID_PARAMETER) && (pbmi->bmiHeader.biCompression == 3)){
	//if(!ret && (pbmi->bmiHeader.biCompression == 3)){
	if(!ret && (pbmi->bmiHeader.biCompression != BI_RGB)){
		OutTraceDW("%s: suppress DIB compression\n", ApiRef);
		(DWORD)(pbmi->bmiHeader.biCompression) = BI_RGB;
		return extCreateDIBSection(hdc, pbmi, iUsage, ppvBits, hSection, dwOffset);
	}

	if(!ret) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	else {
		OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
		// beware: it is worth dumping the DIB section only when hSection is not NULL
		// which means the bitmap is connected to a shared memory file
		// v2.04.54: flag moved to debug flags, dump activated by toggle key
#ifndef DXW_NOTRACES
		// if(ret && (dxw.dwDFlags2 & DUMPBITMAPS)) DumpBitmap(ApiRef, ret);
		if((dxw.dwDFlags & DUMPDIBSECTION) && hSection) 
			DumpDibSection(ApiRef, hdc, pbmi, iUsage, *ppvBits);
#endif // DXW_NOTRACES
	}

	return ret;
}

HBITMAP WINAPI extCreateDiscardableBitmap(HDC hdc, int nWidth, int nHeight)
{
	HBITMAP ret;
	ApiName("CreateDiscardableBitmap");
	OutTraceGDI("%s: hdc=%#x size=(%dx%d)\n", ApiRef, hdc, nWidth, nHeight);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pCreateDiscardableBitmap)(sdc.GetHdc(), nWidth, nHeight);
				sdc.PutPrimaryDC(hdc, FALSE);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret=(*pCreateDiscardableBitmap)(hdc, nWidth, nHeight);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extExtFloodFill(HDC hdc, int nXStart, int nYStart, COLORREF crColor, UINT fuFillType)
{
	BOOL ret;
	ApiName("ExtFloodFill");
	OutTraceGDI("%s: hdc=%#x pos=(%d,%d)\n", ApiRef, hdc, nXStart, nYStart);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pExtFloodFill)(sdc.GetHdc(), nXStart, nYStart, crColor, fuFillType);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret=(*pExtFloodFill)(hdc, nXStart, nYStart, crColor, fuFillType);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extGdiAlphaBlend(HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn)
{
	BOOL ret;
	ApiName("GdiAlphaBlend");
	int Flux;
	BOOL IsToScreen, IsFromScreen;
	BOOL IsDCLeakageSrc = FALSE;
	BOOL IsDCLeakageDest = FALSE;

	// v2.04.05: call found in Rhem during savegame load
	OutTraceGDI("%s: dest {hdc=%#x pos=(%d,%d) size=(%dx%d)} source {hdc=%#x pos=(%d,%d) size=(%dx%d)} ftn=%#x\n", 
		ApiRef, hdcDest, xoriginDest, yoriginDest, wDest, hDest, hdcSrc, xoriginSrc, yoriginSrc, wSrc, hSrc, ftn);

	if(hdcDest == NULL){
		hdcDest = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc dest=NULL->%#x\n", ApiRef, hdcDest);
		IsDCLeakageDest = TRUE;
	}
	if(hdcSrc == NULL){
		hdcSrc = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc src=NULL->%#x\n", ApiRef, hdcSrc);
		IsDCLeakageSrc = TRUE;
	}

	IsToScreen=(OBJ_DC == (*pGetObjectType)(hdcDest));
	IsFromScreen=(OBJ_DC == (*pGetObjectType)(hdcSrc));
	Flux = (IsToScreen ? 1 : 0) + (IsFromScreen ? 2 : 0); 
	if (IsToScreen && (dxw.dwDFlags & NOGDIBLT)) return TRUE;

	//_Warn("GdiAlphaBlend");
	switch(dxw.GDIEmulationMode){
		case GDIMODE_SHAREDDC: 
			switch(Flux){
				case 0: // memory to memory
					ret=(*pGdiAlphaBlend)(hdcSrc, xoriginDest, yoriginDest, wDest, hDest, hdcSrc, xoriginSrc, yoriginSrc, wSrc, hSrc, ftn);
					break;
				case 1: // memory to screen
				case 3: // screen to screen
					sdc.GetPrimaryDC(hdcDest);
					ret=(*pGdiAlphaBlend)(sdc.GetHdc(), xoriginDest, yoriginDest, wDest, hDest, hdcSrc, xoriginSrc, yoriginSrc, wSrc, hSrc, ftn);
					sdc.PutPrimaryDC(hdcDest, TRUE, xoriginDest, yoriginDest, wDest, hDest);
					break;
				case 2: // screen to memory using virtual screen
					sdc.GetPrimaryDC(hdcSrc);
					ret=(*pGdiAlphaBlend)(hdcDest, xoriginDest, yoriginDest, wDest, hDest, sdc.GetHdc(), xoriginSrc, yoriginSrc, wSrc, hSrc, ftn);
					sdc.PutPrimaryDC(hdcSrc, FALSE, xoriginSrc, yoriginSrc, wSrc, hSrc);
					break;
			}
			break;			
		case GDIMODE_STRETCHED: 
			switch(Flux){
				case 1: // memory to screen
					dxw.MapClient(&xoriginDest, &yoriginDest, &wDest, &hDest);
					break;
				case 2: // screen to memory
					dxw.MapClient(&xoriginSrc, &yoriginSrc, &wSrc, &hSrc);
					break;
				default:
					break;
			}
			// fallthrough ....
		case GDIMODE_EMULATED:
		default:
			ret=(*pGdiAlphaBlend)(hdcDest, xoriginDest, yoriginDest, wDest, hDest, hdcSrc, xoriginSrc, yoriginSrc, wSrc, hSrc, ftn);
			break;
	}

	if(IsDCLeakageSrc) (*pGDIReleaseDC)(dxw.GethWnd(), hdcSrc);
	if(IsDCLeakageDest) (*pGDIReleaseDC)(dxw.GethWnd(), hdcDest);
	if(ret && IsToScreen) dxw.ShowOverlay(hdcDest);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extGdiGradientFill(HDC hdc, PTRIVERTEX pVertex, ULONG nVertex, PVOID pMesh, ULONG nMesh, ULONG ulMode)
{
	// v2.04.06.fx2: found in RHEM when DirectX emulation is off (?).
	// temporary version - doesn't scale nor return error
	BOOL ret;
	ApiName("GdiGradientFill");
	OutTraceGDI("%s: HDC=%#x nVertex=%d nMesh=%d mode=%#x(%s)\n", 
		ApiRef, hdc, nVertex, nMesh, ulMode, sGradientMode(ulMode));

	ret = (*pGdiGradientFill)(hdc, pVertex, nVertex, pMesh, nMesh, ulMode);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return TRUE;
}

BOOL WINAPI extGdiTransparentBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, 
							 HDC hdcSrc, int nXSrc, int nYSrc, int nWSrc, int nHSrc, UINT crTransparent)
{
	BOOL res;
	ApiName("GdiTransparentBlt");
	BOOL IsToScreen;
	BOOL IsFromScreen;
	BOOL IsDCLeakageSrc = FALSE;
	BOOL IsDCLeakageDest = FALSE;
	int Flux;

	OutTraceGDI("%s: HDC=%#x nXDest=%d nYDest=%d nWidth=%d nHeight=%d hdcSrc=%#x nXSrc=%d nYSrc=%d nWSrc=%d nHSrc=%d transp=%#x\n", 
		ApiRef, hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, crTransparent);

	OutDebugGDI("%s: DEBUG FullScreen=%#x target hdctype=%#x(%s) hwnd=%#x\n", 
		ApiRef, dxw.IsFullScreen(), (*pGetObjectType)(hdcDest), ExplainDCType((*pGetObjectType)(hdcDest)), WindowFromDC(hdcDest));

	if(dxw.GDIEmulationMode == GDIMODE_EMULATED){
		if (hdcDest==dxw.RealHDC) hdcDest=dxw.VirtualHDC;
		OutDebugDW("%s: DEBUG emulated hdc dest=%#x->%#x\n", ApiRef, dxw.RealHDC, hdcDest);
	}

	if(hdcDest == NULL){
		// happens in Reah, hdc is NULL despite the fact that BeginPaint returns a valid DC. Too bad, we recover here ...
		hdcDest = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc dest=NULL->%#x\n", ApiRef, hdcDest);
		IsDCLeakageDest = TRUE;
	}
	if(hdcSrc == NULL){
		hdcSrc = (*pGDIGetDC)(dxw.GethWnd());
		OutDebugDW("%s: DEBUG hdc src=NULL->%#x\n", ApiRef, hdcSrc);
		IsDCLeakageSrc = TRUE;
	}

	IsToScreen=(OBJ_DC == (*pGetObjectType)(hdcDest));
	IsFromScreen=(OBJ_DC == (*pGetObjectType)(hdcSrc));
	Flux = (IsToScreen ? 1 : 0) + (IsFromScreen ? 2 : 0); 
	if (IsToScreen && (dxw.dwDFlags & NOGDIBLT)) return TRUE;

	if(dxw.IsToRemap(hdcDest) && (hdcDest != hdcSrc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				switch(Flux){
					case 0: // memory to memory
						res=(*pGdiTransparentBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, crTransparent);
						break;
					case 1: // memory to screen
					case 3: // screen to screen
						sdc.GetPrimaryDC(hdcDest);
						res=(*pGdiTransparentBlt)(sdc.GetHdc(), nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, crTransparent);
						sdc.PutPrimaryDC(hdcDest, TRUE, nXDest, nYDest, nWidth, nHeight);
						break;
					case 2: // screen to memory using virtual screen
						sdc.GetPrimaryDC(hdcSrc);
						res=(*pGdiTransparentBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, sdc.GetHdc(), nXSrc, nYSrc, nWSrc, nHSrc, crTransparent);
						sdc.PutPrimaryDC(hdcSrc, FALSE, nXSrc, nYSrc, nWSrc, nHSrc);
						break;
				}
				break;
			case GDIMODE_STRETCHED: {
				int nWDest, nHDest;
				nWDest= nWidth;
				nHDest= nHeight;
				switch(Flux){
					case 1: // memory to screen
						dxw.MapClient(&nXDest, &nYDest, &nWDest, &nHDest);
						break;
					case 2: // screen to memory
						dxw.MapClient(&nXSrc, &nYSrc, &nWidth, &nHeight);
						break;
					default:
						break;
				}
				res=(*pGdiTransparentBlt)(hdcDest, nXDest, nYDest, nWDest, nHDest, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, crTransparent);
				OutDebugDW("%s: DEBUG DC dest=(%d,%d) size=(%d,%d)\n", ApiRef, nXDest, nYDest, nWDest, nHDest);
				}
				break;
			case GDIMODE_EMULATED:
			default:
				res=(*pGdiTransparentBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, crTransparent);
				break;
		}
	}
	else {
		res=(*pGdiTransparentBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWSrc, nHSrc, crTransparent);
	}

	if(IsDCLeakageSrc) (*pGDIReleaseDC)(dxw.GethWnd(), hdcSrc);
	if(IsDCLeakageDest) (*pGDIReleaseDC)(dxw.GethWnd(), hdcDest);
	if(res && IsToScreen) {
		dxw.ShowOverlay(hdcDest);
		if(dxw.dwDFlags & MARKGDI32) dxw.Mark(hdcDest, FALSE, COLORMARKEROUTDIB, nXDest, nYDest, nWidth, nHeight);
	}
	_if(!res) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

BOOL WINAPI extPie(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2)
{
	BOOL ret;
	ApiName("Pie");
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d)\n", ApiRef, hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pPie)(sdc.GetHdc(), nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret=(*pPie)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nXRadial1, nYRadial1, nXRadial2, nYRadial2);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extAngleArc(HDC hdc, int X, int Y, DWORD dwRadius, FLOAT eStartAngle, FLOAT eSweepAngle)
{
	BOOL ret;
	ApiName("AngleArc");
	OutTraceGDI("%s: hdc=%#x pos=(%d,%d)\n", ApiRef, hdc, X, Y);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pAngleArc)(sdc.GetHdc(), X, Y, dwRadius, eStartAngle, eSweepAngle);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret=(*pAngleArc)(hdc, X, Y, dwRadius, eStartAngle, eSweepAngle);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extPolyPolyline(HDC hdc, const POINT *lppt, const DWORD *lpdwPolyPoints, DWORD cCount)
{
	BOOL ret;
	ApiName("PolyPolyline");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolyPolyline)(sdc.GetHdc(), lppt, lpdwPolyPoints, cCount);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret=(*pPolyPolyline)(hdc, lppt, lpdwPolyPoints, cCount);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

int WINAPI extSelectClipRgn(HDC hdc, HRGN hrgn)
{
	int ret;
	ApiName("SelectClipRgn");
	OutTraceGDI("%s: hdc=%#x hrgn=%#x\n", ApiRef, hdc, hrgn);

	if(dxw.dwDFlags & MARKCLIPRGN){
		RECT rc;
		//COLORREF color = (WindowFromDC(hdc) == dxw.GethWnd()) ? RGB(255, 0, 0) : RGB(255, 255, 0);
		GetRgnBox(hrgn, &rc);
		//dxw.Mark(hdc, FALSE, color, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
		dxw.Mark(hdc, FALSE, COLORMARKERRGN, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
		HDC upperDc = (*pGDIGetDC)(dxw.GethWnd());
		dxw.Mark(upperDc, FALSE, COLORMARKERRGN, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
		(*pGDIReleaseDC)(dxw.GethWnd(), upperDc);
	}
	dxw.bIsSelectClipRgnRecursed = TRUE;
	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pSelectClipRgn)(sdc.GetHdc(), hrgn);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			case GDIMODE_STRETCHED:
				// from MSDN https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectcliprgn
				// Only a copy of the selected region is used. 
				// The region itself can be selected for any number of other device contexts or it can be deleted.
				if(hrgn) { // beware: hrgn can be NULL !!!
					HRGN hrgnScaled;
					dxw.bIsSelectClipRgnRecursed = FALSE;
					hrgnScaled = dxw.MapRegion(ApiRef, hrgn);
					ret=(*pSelectClipRgn)(hdc, hrgnScaled);
					(*pDeleteObject)(hrgnScaled);
					OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
					return ret;			
				}
			default:
				break;
		}
	}

	ret = (*pSelectClipRgn)(hdc, hrgn);
	dxw.bIsSelectClipRgnRecursed = FALSE;
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
	return ret;
}

int WINAPI extExtSelectClipRgn(HDC hdc, HRGN hrgn, int fnMode)
{
	int ret;
	ApiName("ExtSelectClipRgn");
	
	// recursion bypass, don't even log the operation !!
	if(dxw.bIsSelectClipRgnRecursed) return(*pExtSelectClipRgn)(hdc, hrgn, fnMode);

	OutTraceGDI("%s: hdc=%#x hrgn=%#x mode=%#x(%s)\n", 
		ApiRef, hdc, hrgn, fnMode, ExplainRgnMode(fnMode));

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pExtSelectClipRgn)(sdc.GetHdc(), hrgn, fnMode);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			case GDIMODE_STRETCHED: 
				// from MSDN https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-extselectcliprgn
				// Only a copy of the selected region is used. 
				// The region itself can be selected for any number of other device contexts or it can be deleted.
				if(hrgn) { // beware: hrgn can be NULL !!!
					HRGN hrgnScaled;
					hrgnScaled = dxw.MapRegion(ApiRef, hrgn);
					ret=(*pExtSelectClipRgn)(hdc, hrgnScaled, fnMode);
					(*pDeleteObject)(hrgnScaled);
					OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
					return ret;			
				}
			default:
				break;
		}
	}

	ret = (*pExtSelectClipRgn)(hdc, hrgn, fnMode);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
	return ret;
}

int WINAPI extGetClipRgn(HDC hdc, HRGN hrgn)
{
	int ret;
	ApiName("GetClipRgn");
	OutTraceGDI("%s: hdc=%#x hrgn=%#x\n", ApiRef, hdc, hrgn);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pGetClipRgn)(sdc.GetHdc(), hrgn);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			case GDIMODE_STRETCHED: 
				if(hrgn) { // beware: hrgn can be NULL !!!
					HRGN hrgnScaled;
					ret=(*pGetClipRgn)(hdc, hrgn);
					hrgnScaled = dxw.UnmapRegion(ApiRef, hrgn);
					CombineRgn(hrgn, hrgnScaled, NULL, RGN_COPY);
					(*pDeleteObject)(hrgnScaled);
					OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainGetRgnCode(ret));
					_if(ret == 1) dxwDumpRgn(ApiRef, hrgn);
					return ret;			
				}
			default:
				break;
		}
	}

	ret = (*pGetClipRgn)(hdc, hrgn);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainGetRgnCode(ret));
	_if(ret == 1) dxwDumpRgn(ApiRef, hrgn);
	return ret;
}

int WINAPI extOffsetClipRgn(HDC hdc, int nXOffset, int nYOffset)
{
	int ret;
	ApiName("OffsetClipRgn");
	OutTraceGDI("%s: hdc=%#x offset(x,y)=(%d,%d)\n", ApiRef, nXOffset, nYOffset);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pOffsetClipRgn)(sdc.GetHdc(), nXOffset, nYOffset);
				sdc.PutPrimaryDC(hdc, TRUE);
				OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
				return ret;
				break;			
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&nXOffset, &nYOffset);
			default:
				break;
		}
	}

	ret = (*pOffsetClipRgn)(hdc, nXOffset, nYOffset);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
	return ret;
}

BOOL WINAPI extFillRgn(HDC hdc, HRGN hrgn, HBRUSH hbr)
{
	BOOL ret;
	ApiName("FrameRgn");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pFillRgn)(sdc.GetHdc(), hrgn, hbr);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			case GDIMODE_STRETCHED: 
				{
					HRGN hrgnScaled = dxw.MapRegion(ApiRef, hrgn);
					ret=(*pFillRgn)(hdc, hrgnScaled, hbr);
					(*pDeleteObject)(hrgnScaled);
					_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
					return ret;			
				}
			default:
				break;
		}
	}
		
	ret=(*pFillRgn)(hdc, hrgn, hbr);
	_if(!ret) OutErrorGDI("%s ERROR: err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extFrameRgn(HDC hdc, HRGN hrgn, HBRUSH hbr, int nWidth, int nHeight)
{
	BOOL ret;
	ApiName("FrameRgn");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pFrameRgn)(sdc.GetHdc(), hrgn, hbr, nWidth, nHeight);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			case GDIMODE_STRETCHED: 
				{
					HRGN hrgnScaled = dxw.MapRegion(ApiRef, hrgn);
					ret=(*pFrameRgn)(hdc, hrgnScaled, hbr, nWidth, nHeight);
					(*pDeleteObject)(hrgnScaled);
					_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
					return ret;
				}
				break;
			default:
				break;
		}
	}
		
	ret=(*pFrameRgn)(hdc, hrgn, hbr, nWidth, nHeight);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extInvertRgn(HDC hdc, HRGN hrgn)
{
	BOOL ret;
	ApiName("InvertRgn");
	OutTraceGDI("%s: hdc=%#x hrgn=%#x\n", ApiRef, hdc, hrgn);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pInvertRgn)(sdc.GetHdc(), hrgn);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			case GDIMODE_STRETCHED: 
				{
					HRGN hrgnScaled = dxw.MapRegion(ApiRef, hrgn);
					ret=(*pInvertRgn)(hdc, hrgnScaled);
					(*pDeleteObject)(hrgnScaled);
					_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
					return ret;
				}
				break;
			default:
				break;
		}
	}
		
	ret=(*pInvertRgn)(hdc, hrgn);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extPaintRgn(HDC hdc, HRGN hrgn)
{
	BOOL ret;
	ApiName("PaintRgn");
	OutTraceGDI("%s: hdc=%#x hrgn=%#x\n", ApiRef, hdc, hrgn);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pPaintRgn)(sdc.GetHdc(), hrgn);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			case GDIMODE_STRETCHED: 
				{
					HRGN hrgnScaled = dxw.MapRegion(ApiRef, hrgn);
					ret=(*pPaintRgn)(hdc, hrgnScaled);
					(*pDeleteObject)(hrgnScaled);
					_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
					return ret;
				}
				break;
			default:
				break;
		}
	}
		
	ret=(*pPaintRgn)(hdc, hrgn);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

int WINAPI extSetMapMode(HDC hdc, int fnMapMode)
{
	int ret;
	ApiName("SetMapMode");
	OutTraceGDI("%s: hdc=%#x MapMode=%#x\n", ApiRef, hdc, fnMapMode);

#if 0
	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pSetMapMode)(sdc.GetHdc(), fnMapMode);
				sdc.PutPrimaryDC(hdc, FALSE);
				return ret;
				break;			
			default:
				break;
		}
	}
#endif 

	ret=(*pSetMapMode)(hdc, fnMapMode);
	OutTraceGDI("%s: ret=%d\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

int WINAPI extGetMapMode(HDC hdc)
{
	int ret;
	ApiName("GetMapMode");

	ret = (*pGetMapMode)(hdc);
	OutTraceGDI("%s: hdc=%#x res=%d\n", ApiRef, hdc, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extRoundRect(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nWidth, int nHeight)
{
	int ret;
	OutTraceGDI("RoundRect: hdc=%#x rect=(%d,%d)-(%d,%d) ellipse=(%dx%d)\n", hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nWidth, nHeight);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_STRETCHED: 
				// v2.04.28: found in "iF-22"
				dxw.MapClient(&nLeftRect, &nTopRect, &nRightRect, &nBottomRect);
				dxw.MapClient(&nWidth, &nHeight);
				OutTraceGDI("RoundRect: FIXED rect=(%d,%d)-(%d,%d) ellipse=(%dx%d)\n", nLeftRect, nTopRect, nRightRect, nBottomRect, nWidth, nHeight);
				break;
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pRoundRect)(sdc.GetHdc(), nLeftRect, nTopRect, nRightRect, nBottomRect, nWidth, nHeight);
				sdc.PutPrimaryDC(hdc, TRUE, nLeftRect, nTopRect, nRightRect, nBottomRect);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret=(*pRoundRect)(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect, nWidth, nHeight);
	_if(!ret) OutErrorGDI("RoundRect ERROR: err=%d\n", GetLastError());
	return ret;
}

BOOL WINAPI extPolyPolygon(HDC hdc, const POINT *lpPoints, const INT *lpPolyCounts, int nCount)
{
	BOOL ret;
	OutTraceGDI("PolyPolygon: hdc=%#x\n", hdc);

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pPolyPolygon)(sdc.GetHdc(), lpPoints, lpPolyCounts, nCount);
				sdc.PutPrimaryDC(hdc, TRUE);
				return ret;
				break;			
			default:
				break;
		}
	}
		
	ret=(*pPolyPolygon)(hdc, lpPoints, lpPolyCounts, nCount);
	_if(!ret) OutErrorGDI("PolyPolygon ERROR: err=%d\n", GetLastError());
	return ret;
}

#if 0
BOOL WINAPI extDPtoLP(HDC hdc, LPPOINT lpPoints, int nCount)
{
	BOOL ret;
	OutTrace("DPtoLP: hdc=%#x, nCount=%d\n", hdc, nCount);
	for(int i=0; i<nCount; i++) OutTrace("point[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
	ret = (*pDPtoLP)(hdc, lpPoints, nCount);
	for(int i=0; i<nCount; i++) OutTrace("point[%d]=(%d,%d)\n", i, lpPoints[i].x, lpPoints[i].y);
	return ret;
}
#endif

#ifdef TRACEALL
HENHMETAFILE WINAPI extSetWinMetaFileBits(UINT nSize, const BYTE *lpMeta16Data, HDC hdcRef, const METAFILEPICT *lpMFP)
{
	ApiName("SetWinMetaFileBits");
	HENHMETAFILE ret;
	OutTrace("%s: size=%d hdcref=%#x mfp={}\n", ApiRef, hdcRef, lpMFP->xExt, lpMFP->yExt);
	ret = (*pSetWinMetaFileBits)(nSize, lpMeta16Data, hdcRef, lpMFP);
	OutTrace("%s: ret(hmf)=%#x\n", ApiRef, ret);
	return ret;
}
#endif // TRACEALL

BOOL WINAPI extPlayEnhMetaFile(HDC hdc, HENHMETAFILE hemf, const RECT *lpRect)
{
	BOOL ret;
	ApiName("PlayEnhMetaFile");
	RECT rect;

	// @#@ met in "Jane's Combat Simulations: Israeli Air Force" (1998) on a memory DC
	// MessageBox(0, "PlayEnhMetaFile", "dxwnd", MB_OK);
	if(lpRect){
		OutTraceGDI("%s: hdc=%#x hemf=%#x rect=(%d,%d)-(%d,%d)\n", ApiRef, hdc, hemf, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	else{
		OutTraceGDI("%s: hdc=%#x hemf=%#x rect=NULL\n", ApiRef, hdc, hemf);
	}
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				ret=(*pPlayEnhMetaFile)(sdc.GetHdc(), hemf, lpRect);
				sdc.PutPrimaryDC(hdc, TRUE);
				if(!ret) OutTraceE("%s: ERROR err=%d\n", ApiRef, GetLastError());
				return ret;
				break;		
			case GDIMODE_STRETCHED:
				if(lpRect){
					rect = *lpRect;
					dxw.MapClient(&rect);
					lpRect = &rect;
					OutTraceGDI("%s: remapped rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				break;
			default:
				break;
		}
	}
		
	ret = pPlayEnhMetaFile(hdc, hemf, lpRect);
	if(!ret) OutTraceE("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extGetDCOrgEx(HDC hdc, LPPOINT lpPoint)
{
	BOOL ret;
	ret = pGetDCOrgEx(hdc, lpPoint);
	OutTraceGDI("GetDCOrgEx: hdc=%#x pt=(%d,%d)\n", hdc, lpPoint->x, lpPoint->y);
	if(ret && dxw.IsFullScreen()){ // ?? and dxw.isDesktop() ???
		dxw.UnmapClient(lpPoint);
		OutTraceGDI("GetDCOrgEx: fixed pt=(%d,%d)\n", lpPoint->x, lpPoint->y);
	}
	return ret;
}

int WINAPI extSetROP2(HDC hdc, int fnDrawMode)
{
	// In early OS the SetROP2 caused some sort of screen refresh, that does no longer happen in recent ones.
	// So wrapping the function and inserting a InvalidateRect right after that fixes the problem.
	// This trick fixes the black screens in "Titanic - an adventure out of time" on Win10.
	// N.b. Titanic calls SetTextColor, SetBkColor and SetROP2 in sequence, it might be possible that the 
	// API to hook for refreshing is another, or even them all!
	int ret;
#ifndef DXW_NOTRACE
	char *sROPs[] = {
		"???", "BLACK", "NOTMERGEPEN", "MASKNOTPEN", "NOTCOPYPEN", "MASKPENNOT", "NOT", "XORPEN", "NOTMASKPEN", 
		"MASKPEN", "NOTXORPEN", "NOP", "MERGENOTPEN", "COPYPEN", "MERGEPENNOT", "MERGEPEN", "WHITE"
	};
	OutTraceGDI("SetROP2: hdc=%#x drawmode=%d(%s)\n", hdc, fnDrawMode, (UINT)fnDrawMode <= R2_LAST ? sROPs[fnDrawMode] : "???" );
#endif
	ret = (*pSetROP2)(hdc, fnDrawMode);
	return ret;
}

// v2.04.05: GetTextExtentPointA hooker for "Warhammer Shadow of the Horned Rat"
BOOL WINAPI extGetTextExtentPointA(HDC hdc, LPCTSTR lpString, int cbString, LPSIZE lpSize)
{
	BOOL ret;
	OutTraceGDI("GetTextExtentPointA: hdc=%#x string=\"%s\"(%d)\n", hdc, lpString, cbString);

	ret = (*pGetTextExtentPointA)(hdc, lpString, cbString, lpSize);
	if(!ret){
		OutTraceGDI("GetTextExtentPointA ERROR: err=%d\n", GetLastError);
		return ret;
	}
	
	OutTraceGDI("GetTextExtentPointA: size=(%dx%d)\n", lpSize->cx, lpSize->cy);
	// beware: size scaling is appropriate only when referred to video DC
	switch(dxw.GDIEmulationMode){
		case GDIMODE_STRETCHED: 
			if(dxw.isScaled && (OBJ_DC == (*pGetObjectType)(hdc))){
				dxw.UnmapClient((LPPOINT)lpSize);
				OutTraceGDI("GetTextExtentPointA: remapped size=(%dx%d)\n", lpSize->cx, lpSize->cy);
			}
			break;
		default:
			break;
	}
	return ret;
}

// v2.04.05: GetTextExtentPoint32A hooker for "Warhammer Shadow of the Horned Rat"
BOOL WINAPI extGetTextExtentPoint32A(HDC hdc, LPCTSTR lpString, int cbString, LPSIZE lpSize)
{
	BOOL ret;
	OutTraceGDI("GetTextExtentPoint32A: hdc=%#x(%s) string=\"%s\"(%d)\n", hdc, GetObjectTypeStr(hdc), lpString, cbString);

	ret = (*pGetTextExtentPoint32A)(hdc, lpString, cbString, lpSize);
	if(!ret){
		OutTraceGDI("GetTextExtentPoint32A ERROR: err=%d\n", GetLastError);
		return ret;
	}
	
	OutTraceGDI("GetTextExtentPoint32A: size=(%dx%d)\n", lpSize->cx, lpSize->cy);
	// beware: size scaling is appropriate only when referred to video DC
	switch(dxw.GDIEmulationMode){
		case GDIMODE_STRETCHED: 
			if(dxw.isScaled && (OBJ_DC == (*pGetObjectType)(hdc))){
				dxw.UnmapClient((LPPOINT)lpSize);
				OutTraceGDI("GetTextExtentPoint32A: remapped size=(%dx%d)\n", lpSize->cx, lpSize->cy);
			}
			break;
		default:
			break;
	}
	return ret;
}

#if 0
LONG WINAPI extSetBitmapBits(HBITMAP hbmp, DWORD cBytes, VOID *lpBits)
{
	LONG ret;
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		OutTrace("SetDIBits: hdc=%#x hbmp=%#x lines=(%d,%d) ColorUse=%#x(%s)\n", hdc, hbmp, uStartScan, cScanLines, fuColorUse, ExplainDIBUsage(fuColorUse));
		TraceBITMAPINFOHEADER("SetDIBits", (BITMAPINFOHEADER *)&(lpbmi->bmiHeader));
	}
#endif

	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: // this will flicker !!!!
				sdc.GetPrimaryDC(hdc);
				ret=(*pSetDIBits)(sdc.GetHdc(), hbmp, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
				if(!ret || (ret==GDI_ERROR)) OutErrorGDI("SetDIBits: ERROR err=%d\n", GetLastError()); 
				sdc.PutPrimaryDC(hdc, TRUE, 0, 0, lpbmi->bmiHeader.biWidth, lpbmi->bmiHeader.biHeight);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
			case GDIMODE_EMULATED:
			default:
				break;
		}
	}

	ret = (*pSetBitmapBits)(hbmp, cBytes, lpBits);
	if(!ret || (ret==GDI_ERROR)) OutErrorGDI("SetDIBits: ERROR err=%d\n", GetLastError()); 
	return ret;
}
#endif

HGDIOBJ WINAPI extSelectObject(HDC hdc, HGDIOBJ hgdiobj)
{
	HGDIOBJ ret;
	ApiName("SelectObject");
	DWORD dwObjectType = GetObjectType(hgdiobj);

	OutTraceGDI("%s: hdc=%#x(%s) obj=%#x(%s)\n", ApiRef, hdc, GetObjectTypeStr(hdc), hgdiobj, GetObjectTypeStr((HDC)hgdiobj));

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC){
		//lpADC->AcquireDC();
		ret = (*pSelectObject)(lpADC->GetDC(hdc), hgdiobj);
		lpADC->ReleaseDC();
		return ret;
	}

	switch(dwObjectType){
		case OBJ_FONT: 
			{
				if((dxw.dwFlags1 & FIXTEXTOUT) && (GetObjectType(hdc)==OBJ_DC)){
					HGDIOBJ scaled;
					scaled = fontdb.GetScaledFont((HFONT)hgdiobj);
					if(scaled) {
						hgdiobj=scaled;
						OutTraceGDI("%s: replaced font obj=%#x\n", ApiRef, hgdiobj);
					}
					else{
						OutErrorGDI("%s: unmatched font obj=%#x\n", ApiRef, hgdiobj);
					}
				}
			}
			break;
#ifndef DXW_NOTRACES
		case OBJ_BITMAP:
			if(dxw.dwDFlags2 & DUMPBITMAPS) DumpBitmap(ApiRef, (HBITMAP)hgdiobj);
			break;
#endif // DXW_NOTRACES
	}
	ret = (*pSelectObject)(hdc, hgdiobj);
	return ret;
}

BOOL WINAPI extDeleteObject(HGDIOBJ hgdiobj)
{
	BOOL ret;
	ApiName("DeleteObject");
	HGDIOBJ scaledobj;
	DWORD objType;
	OutTraceGDI("%s: obj=%#x(%s)\n", ApiRef, hgdiobj, GetObjectTypeStr((HDC)hgdiobj));

	objType = GetObjectType(hgdiobj);
	switch(objType){
		case OBJ_FONT:
			if(dxw.dwFlags1 & FIXTEXTOUT){
				scaledobj=fontdb.DeleteFont((HFONT)hgdiobj); // v2.04.94: moved up
				if(scaledobj) (*pDeleteObject)(scaledobj);
				OutTraceGDI("%s: deleted font obj=%#x scaled=%#x\n", ApiRef, hgdiobj, scaledobj);
			}
			break;
#ifndef DXW_NOTRACES
		case OBJ_BITMAP:
			if(dxw.dwDFlags2 & DUMPBITMAPS) DumpBitmap(ApiRef, (HBITMAP)hgdiobj);
			break;
#endif // DXW_NOTRACES
	}
	ret = (*pDeleteObject)(hgdiobj);

	if(dxw.EmulateWin95) ret = TRUE; // same as Microsoft shim "EmulateDeleteObject"
	return ret;
}

// --- NLS

// v2.05.14: GetStockObject wrapper here almost duly copied from ntleas code, who in turn seems  
// copied from Arianrhod's Locale Emulator project, no longer available at that git address

static HFONT GetFontFromFont(BYTE DefaultCharset, LPCSTR DefaultFontFace, HFONT Font) {
	LOGFONTW LogFont;
	if (GetObjectW(Font, sizeof(LogFont), &LogFont) == 0) return NULL;
	LogFont.lfCharSet = DefaultCharset;
	MultiByteToWideChar(CP_ACP, 0, DefaultFontFace, -1, LogFont.lfFaceName, LF_FACESIZE);
	OutTraceGDI("GetFontFromFont: facename=%ls size=(%dx%d)\n", LogFont.lfFaceName, LogFont.lfWidth, LogFont.lfHeight);
	return CreateFontIndirectW(&LogFont);
}

__inline BYTE ToCharSet(int lfcharset) {
	return lfcharset > 0 ? (BYTE)lfcharset : DEFAULT_CHARSET;
}

extern int CodePageToCharset(UINT);

HGDIOBJ WINAPI extGetStockObject(int fnObject)
{
	HGDIOBJ ret;
	ApiName("GetStockObject");

	// this code is considered from LE : 
	// https://github.com/Arianrhod/Arianrhod/blob/master/Source/LocaleEmulator/LocaleEmulator/Gdi32Hook.cpp
	static const int StockObjectIndex[] = {
		OEM_FIXED_FONT, ANSI_FIXED_FONT, ANSI_VAR_FONT, 
		SYSTEM_FONT, DEVICE_DEFAULT_FONT, SYSTEM_FIXED_FONT, DEFAULT_GUI_FONT,
	};
	static HFONT fontstock[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	static int lfcharset = 0;

	if(!lfcharset) {
		lfcharset = CodePageToCharset(dxw.CodePage);
		OutTraceGDI("%s: initialized charset=%d cp=%d\n", ApiRef, lfcharset, dxw.CodePage);
	}
	for (int i = 0; i < ARRAYSIZE(StockObjectIndex); ++i) {
		if (StockObjectIndex[i] == fnObject) {
			if (!fontstock[fnObject]) 
				//fontstock[fnObject] = GetFontFromFont(ToCharSet(settings.lfcharset), (LPCSTR)settings.lpFontFaceName, GetStockObjectJ(fnObject));
				//fontstock[fnObject] = GetFontFromFont(ToCharSet(lfcharset), (LPCSTR)"MingLiu regular", (HFONT)(*pGetStockObject)(fnObject));
				//fontstock[fnObject] = GetFontFromFont(ToCharSet(lfcharset), (LPCSTR)"Microsoft Sans Serif", (HFONT)(*pGetStockObject)(fnObject));
				fontstock[fnObject] = GetFontFromFont(ToCharSet(lfcharset), (LPCSTR)"", (HFONT)(*pGetStockObject)(fnObject));
			OutTraceGDI("%s: fnObject=%d(%s) remapped ret=%#x\n", ApiRef, fnObject, objname(fnObject), fontstock[fnObject]);
			return fontstock[fnObject];
		}
	}
	ret = (*pGetStockObject)(fnObject);
	OutTraceGDI("%s: fnObject=%d(%s) ret=%#x\n", ApiRef, fnObject, objname(fnObject), ret);
	return ret;
}

BOOL WINAPI extSwapBuffers(HDC hdc)
{
	ApiName("SwapBuffers");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);
	if(dxw.dwFlags12 & PROJECTBUFFER) dxw.Project(hdc);
	return (*pSwapBuffers)(hdc);
}

#ifdef TRACEREGIONS
BOOL WINAPI extPtInRegion(HRGN hrgn, int x, int y)
{
	ApiName("PtInRegion");
	BOOL ret;
	OutTraceGDI("%s: hrgn=%#x xy=(%d, %d)\n", ApiRef, hrgn, x, y);
	ret = (*pPtInRegion)(hrgn, x, y);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

int WINAPI extCombineRgn(HRGN hrgnDest, HRGN hrgn1, HRGN hrgn2, int iMode)
{
	int ret;
	ApiName("CombineRgn");
	OutTraceGDI("%s: hrgnDest=%#x hrgn1=%#x hrgn2=%#x mode=%d(%s)\n", 
		ApiRef, hrgnDest, hrgn1, hrgn2, iMode, ExplainRgnMode(iMode));
	ret = (*pCombineRgn)(hrgnDest, hrgn1, hrgn2, iMode);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
	return ret;
}

HRGN WINAPI extCreateEllipticRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	HRGN ret;
	ApiName("CreateEllipticRgn");
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect);
	ret=(*pCreateEllipticRgn)(nLeftRect, nTopRect, nRightRect, nBottomRect);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

HRGN WINAPI extCreateEllipticRgnIndirect(const RECT *lprc)
{
	HRGN ret;
	ApiName("CreateEllipticRgnIndirect");
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprc->left, lprc->top, lprc->right, lprc->bottom);
	ret=(*pCreateEllipticRgnIndirect)(lprc);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

HRGN WINAPI extCreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	HRGN ret;
	ApiName("CreateRectRgn");
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect);
	ret=(*pCreateRectRgn)(nLeftRect, nTopRect, nRightRect, nBottomRect);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

HRGN WINAPI extCreateRectRgnIndirect(const RECT *lprc)
{
	// v2.05.92 fix
	HRGN ret;
	ApiName("CreateRectRgnIndirect");
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprc->left, lprc->top, lprc->right, lprc->bottom);
	ret=(*pCreateRectRgnIndirect)(lprc);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

HRGN WINAPI extCreateRoundRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int w, int h)
{
	HRGN ret;
	ApiName("CreateRoundRectRgn");
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d) ellypse=(%d,%d)\n", 
		ApiRef, nLeftRect, nTopRect, nRightRect, nBottomRect, w, h);
	ret=(*pCreateRoundRectRgn)(nLeftRect, nTopRect, nRightRect, nBottomRect, w, h);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

HRGN WINAPI extCreatePolygonRgn(const POINT *lpPoints, int cPoints, int fnPolyFillMode)
{
	HRGN ret;
	ApiName("CreatePolygonRgn");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		int i;
		OutTrace("%s: PolyFillMode=%#x cCount=%d pt=", ApiRef, fnPolyFillMode, cPoints); 
		if(IsDebugGDI){
			for(i=0; i<cPoints; i++) OutTrace("> pt[%d]=(%d,%d) ", i, lpPoints[i].x, lpPoints[i].y);
		}
	}
#endif
	ret=(*pCreatePolygonRgn)(lpPoints, cPoints, fnPolyFillMode);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

HRGN WINAPI extCreatePolyPolygonRgn(const POINT *lpPoints, CONST int *lpCounts, int cCount, int fnPolyFillMode)
{
	HRGN ret;
	ApiName("CreatePolygonRgn");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		int i;
		OutTrace("%s: PolyFillMode=%#x cCount=%d pt=", ApiRef, fnPolyFillMode, cCount); 
		if(IsDebugGDI){
			int k = 0;
			for(i=0; i<cCount; i++) 
				for(int j=0; j<lpCounts[i]; j++, k++) 
					OutTrace("> pt[%d-%d]=(%d,%d) ", i, j, lpPoints[k].x, lpPoints[k].y);
		}
	}
#endif
	ret=(*pCreatePolyPolygonRgn)(lpPoints, lpCounts, cCount, fnPolyFillMode);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

BOOL WINAPI extSetRectRgn(HRGN hrgn, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	BOOL ret;
	ApiName("SetRectRgn");
	OutTraceGDI("%s: hrgn=%#x rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, hrgn, nLeftRect, nTopRect, nRightRect, nBottomRect);
	ret = (*pSetRectRgn)(hrgn, nLeftRect, nTopRect, nRightRect, nBottomRect);
	_if(!ret) OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError()); 
	return ret;
}

int WINAPI extOffsetRgn(HRGN hrgn, int nXOffset, int nYOffset)
{
	int ret;
	ApiName("OffsetRgn");
	OutTraceGDI("%s: hrgn=%#x nXOffset=%d nYOffset=%d\n", ApiRef, hrgn, nXOffset, nYOffset);
	ret = (*pOffsetRgn)(hrgn, nXOffset, nYOffset);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
	return ret;
}

int WINAPI extGetRgnBox(HRGN hrgn, LPRECT lprc)
{
	int ret;
	ApiName("GetRgnBox");
	OutTraceGDI("%s: hrgn=%#x\n", ApiRef, hrgn);
	ret=(*pGetRgnBox)(hrgn, lprc);
	OutTraceGDI("%s: ret=%d(%s) rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, ret, ExplainRegionType(ret), lprc->left, lprc->top, lprc->right, lprc->bottom);
	return ret;
}

int WINAPI extGetRandomRgn(HDC hdc, HRGN hrgn, INT i)
{
	int ret;
	ApiName("GetRandomRgn");
	OutTraceGDI("%s: hdc=%#x hrgn=%#x i=%d\n", ApiRef, hdc, hrgn, i);
	MessageBox(0, "GetRandomRgn", "DxWnd", 0);
	ret=(*pGetRandomRgn)(hdc, hrgn, i);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ret ? (ret == 1 ? "OK" : "ERROR") : "NULL");
	return ret;
}

int WINAPI extGetMetaRgn(HDC hdc, HRGN hrgn)
{
	int ret;
	ApiName("GetMetaRgn");
	OutTraceGDI("%s: hdc=%#x hrgn=%#x\n", ApiRef, hdc, hrgn);
	MessageBox(0, "GetMetaRgn", "DxWnd", 0);
	ret=(*pGetMetaRgn)(hdc, hrgn);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ret ? "OK" : "ERROR");
	return ret;
}

int WINAPI extSetMetaRgn(HDC hdc)
{
	int ret;
	ApiName("SetMetaRgn");
	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);
	MessageBox(0, "SetMetaRgn", "DxWnd", 0);
	ret=(*pSetMetaRgn)(hdc);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRegionType(ret));
	return ret;
}
#endif 

BOOL WINAPI extPtVisible(HDC hdc, int x, int y)
{
	BOOL ret;
	ApiName("PtVisible");

	OutTraceGDI("%s: hdc=%#x pt=(%d,%d)\n", ApiRef, hdc, x, y);
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				hdc = sdc.GetPrimaryDC(hdc);
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&x, &y);
			case GDIMODE_EMULATED:
			default:
				break;
		}
	}

	ret = (*pPtVisible)(hdc, x, y);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extRectVisible(HDC hdc, const RECT *lprect)
{
	BOOL ret;
	ApiName("RectVisible");
	RECT rect;

	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, hdc, lprect->left, lprect->top, lprect->right, lprect->bottom);
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				hdc = sdc.GetPrimaryDC(hdc);
				break;
			case GDIMODE_STRETCHED: 
				rect = *lprect; // work on a copy
				lprect = &rect;
				dxw.MapClient((LPRECT)lprect);
			case GDIMODE_EMULATED:
			default:
				break;
		}
	}

	ret = (*pRectVisible)(hdc, lprect);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extSelectClipPath(HDC hdc, int mode)
{
	BOOL ret;
	ApiName("SelectClipPath");

	// so far, just proxy: how to handle this ???
	OutTraceGDI("%s: hdc=%#x mode=%d(%s)\n", ApiRef, hdc, mode, ExplainRgnMode(mode));

	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				hdc = sdc.GetPrimaryDC(hdc);
				break;
			case GDIMODE_STRETCHED: 
				{
					int cpt;
					cpt = GetPath(hdc, NULL, NULL, 0);
					OutTraceGDI("%s: current path count=%d\n", ApiRef, cpt);
					// to do : scale current path ???
				}
			case GDIMODE_EMULATED:
			default:
				break;
		}
	}

	ret = (*pSelectClipPath)(hdc, mode);
	OutTraceGDI("%s: ret=%#x(%s)\n", ApiRef, ret, ret ? "OK" : "ERR");
	return ret;

}

HBITMAP WINAPI extCreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitCount, CONST VOID *lpBits)
{
	HBITMAP res;
	ApiName("CreateBitmap");
	OutTraceGDI("%s: size=(%d x %d) planes=%d bitc=%d bits=%#x\n", 
		ApiRef, nWidth, nHeight, nPlanes, nBitCount, lpBits);

	if(dxw.dwFlags19 & FIXBITMAPCOLOR){
		HDC hdc = GetWindowDC(0);
		nBitCount = GetDeviceCaps(hdc, BITSPIXEL);
		ReleaseDC(0, hdc);
	}

	// v2.05.82: when loading a bitmap from memory the DIB is created with reference to the
	// current desktop color depth. This adapt the bitmap to the required 8 bit color depth.
	// fixes "Felix the Cat" games.
	// t.b.d. the same situation could occur also with 16 bit bitmaps?
	// v2.06.13: generalized for 16 bits bitmaps
	if(dxw.IsEmulated && (nBitCount != dxw.ActualPixelFormat.dwRGBBitCount)){
		extern GetDC_Type pGetDCMethod();
		extern ReleaseDC_Type pReleaseDCMethod();
		HDC hdc;
		LPDIRECTDRAWSURFACE lpDDSPrim;
		lpDDSPrim = dxwss.GetPrimarySurface();
		if(lpDDSPrim){
			HBITMAP hbmp;
			HRESULT ddres;
			//BITMAPINFO bi;
			struct {
				BITMAPINFOHEADER    bmiHeader;
				RGBQUAD             bmiColors[256];
			} bi;
			memset(&bi, 0, sizeof(bi));
			bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bi.bmiHeader.biBitCount = (WORD)dxw.VirtualPixelFormat.dwRGBBitCount;
			bi.bmiHeader.biHeight = -nHeight;
			bi.bmiHeader.biWidth = nWidth;
			bi.bmiHeader.biPlanes = nPlanes;
			if(nBitCount == 8)
				memcpy(&bi.bmiColors[0], PaletteEntries, sizeof(PaletteEntries)); 
			if((ddres = (*pGetDCMethod())(lpDDSPrim, &hdc)) != DD_OK){
				OutErrorGDI("%s: ERROR in GetDC(%#x) res=%#x(%s) @%d\n", 
					ApiRef, lpDDSPrim, ddres, ExplainDDError(ddres), __LINE__);
			};
#if 0
			hbmp = (*pCreateCompatibleBitmap)(hdc, nWidth, nHeight);
			(*pSetDIBits)(hdc, hbmp, 0, nHeight, lpBits, &bi, DIB_RGB_COLORS);
#else
			hbmp = (*pCreateDIBitmap)(hdc, &bi.bmiHeader, CBM_INIT, lpBits, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
			if(!hbmp) {
				OutErrorGDI("%s: ERROR in CreateDIBitmap(%#x) @%d\n", ApiRef, hdc, __LINE__);
			}
#endif
			if((ddres = (*pReleaseDCMethod())(lpDDSPrim, hdc)) != DD_OK){
				OutErrorGDI("%s: ERROR in ReleaseDC(%#x %#x) res=%#x(%s) @%d\n", 
					ApiRef, lpDDSPrim, hdc, ddres, ExplainDDError(ddres), __LINE__);
			};
			res = hbmp;
		}
		else{
			res=(*pCreateBitmap)(nWidth, nHeight, nPlanes, nBitCount, lpBits);
		}
	}
	else {
		res=(*pCreateBitmap)(nWidth, nHeight, nPlanes, nBitCount, lpBits);
	}
	if(res){
		OutTraceGDI("%s: hbitmap=%#x\n", ApiRef, res);
#ifndef DXW_NOTRACES
		if(dxw.dwDFlags2 & DUMPBITMAPS) DumpBitmap(ApiRef, res);
#endif // DXW_NOTRACES
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

#ifdef TRACEALL
int WINAPI extGetObject(ApiArg, GetObject_Type pGetObject, HANDLE h, int c, LPVOID pv)
{
	int res;
	DWORD dwObjectType = GetObjectType(h);
	OutTraceGDI("%s: hobj=%#x(%s) c=%d\n", ApiRef, h, GetObjectTypeStr((HDC)h), c);
	res=(*pGetObjectW)(h, c, pv);
	if(res && IsTraceGDI){
		OutTrace("%s: res=%d\n", ApiRef, res);
		if(pv && res) {
			switch(dwObjectType){
				case OBJ_BITMAP:
					if(c >= sizeof(BITMAP)) {		
						LPBITMAP lpbmp = (LPBITMAP)pv;
						OutTrace("> type=%d\n", lpbmp->bmType);
						OutTrace("> size=(%d x %d)\n", lpbmp->bmWidth, lpbmp->bmHeight);
						OutTrace("> widthbytes=%d\n", lpbmp->bmWidthBytes);
						OutTrace("> planes=%d\n", lpbmp->bmPlanes);
						OutTrace("> bpp=%d\n", lpbmp->bmBitsPixel);
					}
					if(dxw.dwDFlags2 & DUMPBITMAPS) DumpBitmap(ApiRef, (HBITMAP)h);
					break;
				case OBJ_PAL:
					OutTrace("> entries=%d\n", (WORD)pv);
					break;
				case OBJ_PEN:
					if(c == sizeof(LOGPEN)){
						LPLOGPEN lppen = (LPLOGPEN)pv;
						OutTrace("> style=%d\n", lppen->lopnStyle);
						OutTrace("> width=%d\n", lppen->lopnWidth);
						OutTrace("> color=%#x\n", lppen->lopnColor);
					}
					if(c == sizeof(EXTLOGPEN)){
						LPEXTLOGPEN lppen = (LPEXTLOGPEN)pv;
						OutTrace("> pen style=%d\n", lppen->elpPenStyle);
						OutTrace("> width=%d\n", lppen->elpWidth);
						OutTrace("> color=%#x\n", lppen->elpColor);
						OutTrace("> brush style=%d\n", lppen->elpBrushStyle);
						OutTrace("> hatch=%d\n", lppen->elpHatch);
						OutTrace("> numentries=%d\n", lppen->elpNumEntries);
					}
					break;
				case OBJ_BRUSH:
					if(c == sizeof(LOGBRUSH)){
						LPLOGBRUSH lpbrush = (LPLOGBRUSH)pv;
						OutTrace("> style=%d\n", lpbrush->lbStyle);
						OutTrace("> hatch=%d\n", lpbrush->lbHatch);
						OutTrace("> color=%#x\n", lpbrush->lbColor);
					}
					break;
				case OBJ_FONT:
					if(c == sizeof(LOGFONTA)){
						LPLOGFONTA lplf = (LPLOGFONTA)pv;
						OutTrace("> FaceName: \"%s\"\n", lplf->lfFaceName);
						OutTrace("> size(w,h): (%d,%d)\n", lplf->lfWidth, lplf->lfHeight);
						OutTrace("> Escapement: %d\n", lplf->lfEscapement);
						OutTrace("> Orientation: %d\n", lplf->lfOrientation);
						OutTrace("> fnWeight: %d\n", lplf->lfWeight);
						OutTrace("> fdwItalic: %d\n", lplf->lfItalic);
						OutTrace("> fdwUnderline: %d\n", lplf->lfUnderline);
						OutTrace("> fdwStrikeOut: %d\n", lplf->lfStrikeOut);
						OutTrace("> fdwCharSet: %d\n", lplf->lfCharSet);
						OutTrace("> fdwOutputPrecision: %d\n", lplf->lfOutPrecision);
						OutTrace("> fdwClipPrecision: %d\n", lplf->lfClipPrecision);
						OutTrace("> fdwQuality: %d\n", lplf->lfQuality);
						OutTrace("> fdwPitchAndFamily: %d\n", lplf->lfPitchAndFamily);
					}
					if(c == sizeof(LOGFONTW)){
						LPLOGFONTW lplf = (LPLOGFONTW)pv;
						OutTrace("> FaceName: \"%ls\"\n", lplf->lfFaceName);
						OutTrace("> size(w,h): (%d,%d)\n", lplf->lfWidth, lplf->lfHeight);
						OutTrace("> Escapement: %d\n", lplf->lfEscapement);
						OutTrace("> Orientation: %d\n", lplf->lfOrientation);
						OutTrace("> fnWeight: %d\n", lplf->lfWeight);
						OutTrace("> fdwItalic: %d\n", lplf->lfItalic);
						OutTrace("> fdwUnderline: %d\n", lplf->lfUnderline);
						OutTrace("> fdwStrikeOut: %d\n", lplf->lfStrikeOut);
						OutTrace("> fdwCharSet: %d\n", lplf->lfCharSet);
						OutTrace("> fdwOutputPrecision: %d\n", lplf->lfOutPrecision);
						OutTrace("> fdwClipPrecision: %d\n", lplf->lfClipPrecision);
						OutTrace("> fdwQuality: %d\n", lplf->lfQuality);
						OutTrace("> fdwPitchAndFamily: %d\n", lplf->lfPitchAndFamily);
					}
					break;
				default:
					break;
			}
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

int WINAPI extGetObjectA(HANDLE h, int c, LPVOID pv)
{ ApiName("GetObjectA"); return extGetObject(ApiRef, pGetObjectA, h, c, pv); }
int WINAPI extGetObjectW(HANDLE h, int c, LPVOID pv)
{ ApiName("GetObjectW"); return extGetObject(ApiRef, pGetObjectW, h, c, pv); }

HGDIOBJ WINAPI extGetCurrentObject(HDC hdc, UINT type)
{
	HGDIOBJ res;
	ApiName("GetCurrentObject");
	OutTrace("%s: hdc=%#x type=%d(%s)\n", ApiRef, hdc, type, ExplainDCType(type));
	res=(*pGetCurrentObject)(hdc, type);
	OutTrace("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif // TRACEALL

#ifdef TRACEFONTS
BOOL WINAPI extGetCharWidth(ApiArg, GetCharWidth_Type pGetCharWidth, HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer)
{
	UINT res;

	OutTraceGDI("%s: hdc=%#x first/last=%i/%i\n", ApiRef, hdc, iFirst, iLast);
	res = (*pGetCharWidth)(hdc, iFirst, iLast, lpBuffer);
	if(res){
		for(UINT i=0; i<(iLast-iFirst); i++)
			OutTraceGDI("> width[%i]=%i\n", i, lpBuffer[i]);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extGetCharWidthA(HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer)
{ ApiName("GetCharWidthA"); return extGetCharWidth(ApiRef, pGetCharWidthA, hdc, iFirst, iLast, lpBuffer); }
BOOL WINAPI extGetCharWidthW(HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer)
{ ApiName("GetCharWidthW"); return extGetCharWidth(ApiRef, pGetCharWidthW, hdc, iFirst, iLast, lpBuffer); }
BOOL WINAPI extGetCharWidth32A(HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer)
{ ApiName("GetCharWidth32A"); return extGetCharWidth(ApiRef, pGetCharWidth32A, hdc, iFirst, iLast, lpBuffer); }
BOOL WINAPI extGetCharWidth32W(HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer)
{ ApiName("GetCharWidth32W"); return extGetCharWidth(ApiRef, pGetCharWidth32W, hdc, iFirst, iLast, lpBuffer); }
#endif // TRACEFONTS

COLORREF WINAPI extSetBkColor(HDC hdc, COLORREF color)
{
	ApiName("SetBkColor");
	COLORREF res;

	OutTraceGDI("%s: hdc=%#x color=%#x\n", ApiRef, hdc, color);

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC) {
		res = (*pSetBkColor)(lpADC->GetDC(hdc), color);
		lpADC->ReleaseDC();
		return res;
	}

	res = (*pSetBkColor)(hdc, color);
	return res;
}

BOOL WINAPI extSetBrushOrgEx(HDC hdc, int x, int y, LPPOINT lppt)
{
	ApiName("SetBrushOrgEx");
	BOOL res;

	OutTraceGDI("%s: hdc=%#x p(x,y)=(%d, %d) lppt=%#x\n", ApiRef, hdc, x, y, lppt);

	switch(dxw.GDIEmulationMode){
		case GDIMODE_STRETCHED: 
			dxw.MapClient(&x, &y);
			res = (*pSetBrushOrgEx)(hdc, x, y, lppt);
			if(lppt) dxw.UnmapClient(lppt);
			return res;
			break;
	}
	res = (*pSetBrushOrgEx)(hdc, x, y, lppt);
	return res;
}

BOOL WINAPI extGetBrushOrgEx(HDC hdc, LPPOINT lppt)
{
	ApiName("GetBrushOrgEx");
	BOOL res;

	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	switch(dxw.GDIEmulationMode){
		case GDIMODE_STRETCHED: 
			res = (*pGetBrushOrgEx)(hdc, lppt);
			if(res) dxw.UnmapClient(lppt);
			return res;
			break;
	}
	res = (*pGetBrushOrgEx)(hdc, lppt);
	return res;
}

#ifdef TRACEFONTS
BOOL WINAPI extGetCharABCWidthsA(HDC hdc, UINT wFirst, UINT wLast, LPABC lpABC)
{
	BOOL res;
	ApiName("GetCharABCWidthsA");
	OutTraceGDI("%s: hdc=%#x first/last=%#x/%#x\n", ApiRef, wFirst, wLast, lpABC);
	res = (*pGetCharABCWidthsA)(hdc, wFirst, wLast, lpABC);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

DWORD WINAPI extGetGlyphOutlineA(HDC hdc, UINT uChar, UINT fuFormat, LPGLYPHMETRICS lpgm, DWORD cjBuffer, LPVOID pvBuffer, const MAT2 *lpmat2)
{
	DWORD res;
	ApiName("GetGlyphOutlineA");
	char *captions[]={"METRICS", "BITMAP", "NATIVE", "BEZIER ", "GRAY2_BITMAP", "GRAY4_BITMAP", "GRAY8_BITMAP"};
	OutTraceGDI("%s: hdc=%#x char=%#x format=%#x(%s) cbuf=%d\n", ApiRef, hdc, uChar,
		fuFormat, fuFormat <= GGO_GRAY8_BITMAP ? captions[fuFormat] : "???", 
		cjBuffer);
	res = (*pGetGlyphOutlineA)(hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif // TRACEFONTS

static char *sGammaRampType(DWORD t)
{char *captions[] = {
	"D3DDDI_GAMMARAMP_UNINITIALIZED", 
	"D3DDDI_GAMMARAMP_DEFAULT",
	"D3DDDI_GAMMARAMP_RGB256x3x16",
	"D3DDDI_GAMMARAMP_DXGI_1"};
	return(t <= D3DDDI_GAMMARAMP_DXGI_1 ? captions[t] : "???");
}

LONG WINAPI extD3DKMTSetGammaRamp(const D3DKMT_SETGAMMARAMP *gr)
{
	LONG res;
	ApiName("D3DKMTSetGammaRamp");

	OutTrace("%s: hDev=%#x adapter=%#x gtype=%d(%s) size=%d\n",
		ApiRef, gr->hDevice, gr->VidPnSourceId, gr->Type, sGammaRampType(gr->Type), gr->Size);

	res = (*pD3DKMTSetGammaRamp)(gr);
	OutTrace("%s: res=%#x\n", ApiRef, res);
	MessageBox(0, ApiRef, "DxWnd", 0);
	return res;
}

#ifdef HOOKTEXTMETRICS
BOOL WINAPI extGetTextMetricsA(HDC hdc, LPTEXTMETRICA lptm)
{
	BOOL res;
	ApiName("GetTextMetricsA");

	OutTrace("%s: hdc=%#x\n", ApiRef, hdc);

	res = (*pGetTextMetricsA)(hdc, lptm);
	if(res){
		OutTrace("> tmHeight=%d tmAscent=%d tmDescent=%d\n", 
			lptm->tmHeight, lptm->tmAscent, lptm->tmDescent);
		OutTrace("> tmInternalLeading=%d tmExternalLeading=%d tmAveCharWidth=%d tmMaxCharWidth=%d\n", 
			lptm->tmInternalLeading, lptm->tmExternalLeading, lptm->tmAveCharWidth, lptm->tmMaxCharWidth);
		OutTrace("> tmWeight=%d tmOverhang=%d tmDigitizedAspectX=%d tmDigitizedAspectY=%d\n", 
			lptm->tmWeight, lptm->tmOverhang, lptm->tmDigitizedAspectX, lptm->tmDigitizedAspectY);
		/*
			BYTE        tmFirstChar;
			BYTE        tmLastChar;
			BYTE        tmDefaultChar;
			BYTE        tmBreakChar;
			BYTE        tmItalic;
			BYTE        tmUnderlined;
			BYTE        tmStruckOut;
			BYTE        tmPitchAndFamily;
			BYTE        tmCharSet;
		*/
	}
	else {
		OutTrace("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

DWORD WINAPI extGetCharacterPlacementA(HDC hdc, LPCSTR lpString, int nCount, int nMexExtent, LPGCP_RESULTSA lpResults, DWORD dwFlags)
{
	BOOL res;
	ApiName("GetCharacterPlacementA");

	OutTrace("%s: hdc=%#x str=(%d)\"%.*s\" MexExtent=%d flags=%#x\n", 
		ApiRef, hdc, nCount, nCount, lpString, nMexExtent, dwFlags);

	res = (*pGetCharacterPlacementA)(hdc, lpString, nCount, nMexExtent, lpResults, dwFlags);
	OutTrace("%s: res=%#x->(w=%d h=%d)\n", ApiRef, res, res & 0xFFFF, (res & 0xFFFF0000) >> 16);
	return res;
}

DWORD WINAPI extGetFontLanguageInfo(HDC hdc)
{
	DWORD res;
	ApiName("GetFontLanguageInfo");

	res = (*pGetFontLanguageInfo)(hdc);
	OutTrace("%s: hdc=%#x res=%#x\n", ApiRef, hdc, res);
	return res;
}

UINT WINAPI extSetTextAlign(HDC hdc, UINT align)
{
	DWORD res;
	ApiName("SetTextAlign");

	OutTrace("%s: hdc=%#x align=%#x\n", ApiRef, hdc, align);
	res = (*pSetTextAlign)(hdc, align);
	OutTrace("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif

