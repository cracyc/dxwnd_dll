#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define _MODULE "user32"

#include <stdio.h>
#include <stdlib.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhook.h"
#include "hddraw.h"
#include "dxhelper.h"
#include "shareddc.hpp"
#include <Wingdi.h>
#include <Winuser.h>
#include "dlgstretch.h"
#include "h_user32.h"
#include "dxwlocale.h"
#include "asyncdc.h"

#ifndef DXW_NOTRACES
#define TraceError() OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError())
#define IfTraceError() if(!res) TraceError()
extern void DumpBitmap(char *, HBITMAP);
#else
#define TraceError()
#define IfTraceError()
#endif

//#define TRACEALL
//#define TRACEPOINTS
//#define TRACEWAITOBJECTS
//#define TRACESYSCALLS
//#define TRACEWINDOWS
//#define TRACEIMAGES
//#define TRACEMENUS
//#define TRACEREGIONS
//#define TRACECURSOR
//#define TRACEPROPS

#ifdef TRACEALL
#define TRACEPOINTS
#define TRACEWAITOBJECTS
#define TRACESYSCALLS 
#define TRACEWINDOWS
#define TRACEIMAGES
#define TRACEMENUS
#define TRACEREGIONS
#define TRACECURSOR
#define TRACEPROPS
#endif // TRACEALL

#ifdef TRACEPROPS
typedef HANDLE (WINAPI *GetPropA_Type)(HWND, LPCSTR);
typedef HANDLE (WINAPI *GetPropW_Type)(HWND, LPCWSTR);
typedef BOOL (WINAPI *SetPropA_Type)(HWND, LPCSTR, HANDLE);
typedef BOOL (WINAPI *SetPropW_Type)(HWND, LPCWSTR, HANDLE);
GetPropA_Type pGetPropA;
GetPropW_Type pGetPropW;
SetPropA_Type pSetPropA;
SetPropW_Type pSetPropW;
HANDLE WINAPI extGetPropA(HWND, LPCSTR);
HANDLE WINAPI extGetPropW(HWND, LPCWSTR);
BOOL WINAPI extSetPropA(HWND, LPCSTR, HANDLE);
BOOL WINAPI extSetPropW(HWND, LPCWSTR, HANDLE);
#endif // TRACEPROPS

#ifdef TRACEIMAGES
LoadBitmapA_Type pLoadBitmapA;
LoadBitmapW_Type pLoadBitmapW;
#endif // TRACEIMAGES

#ifdef TRACESYSCALLS 
SetActiveWindow_Type pSetActiveWindow;
IsWindow_Type pIsWindow;
IsWindow_Type pIsWindowEnabled;
IsIconic_Type pIsIconic;
GetAncestor_Type pGetAncestor;
GetWindow_Type pGetWindow;
GetWindowThreadProcessId_Type pGetWindowThreadProcessId;
SetScrollInfo_Type pSetScrollInfo;
GetScrollInfo_Type pGetScrollInfo;
SetScrollPos_Type pSetScrollPos;
GetScrollPos_Type pGetScrollPos;
GetScrollRange_Type pGetScrollRange;
SetScrollRange_Type pSetScrollRange;
TranslateMessage_Type pTranslateMessage;
IsDialogMessageA_Type pIsDialogMessageA;
SetCapture_Type pSetCapture;
ReleaseCapture_Type pReleaseCapture;
//TranslateMessage_Type pTranslateMessage DXWINITIALIZED;
//GetTopWindow_Type pGetTopWindow DXWINITIALIZED;
//EnumDisplayMonitors_Type pEnumDisplayMonitors;
#endif // TRACESYSCALLS
IsZoomed_Type pIsZoomed;

//#define TRANSLATEMESSAGEHOOK
#define FAKEKILLEDWIN 0xDEADDEAD
//#define HOOKWINDOWSHOOKPROCS TRUE
//#define KEEPWINDOWSTATES TRUE
#define HACKVIRTUALKEYS
#ifdef KEEPWINDOWSTATES
HWND hForegroundWindow = NULL;
HWND hActiveWindow = NULL;
#endif

#ifdef TRACEALL
typedef HWND (WINAPI *GetCapture_Type)(void);
GetCapture_Type pGetCapture;
HWND WINAPI extGetCapture();
#endif // TRACEALL

typedef HDWP (WINAPI *BeginDeferWindowPos_Type)(int);
BeginDeferWindowPos_Type pBeginDeferWindowPos;
HDWP WINAPI extBeginDeferWindowPos(int);

typedef BOOL (WINAPI *SetPhysicalCursorPos_Type)(int, int);
SetPhysicalCursorPos_Type pSetPhysicalCursorPos;
BOOL WINAPI extSetPhysicalCursorPos(int, int);

typedef BOOL (WINAPI *UpdateLayeredWindow_Type)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD);
UpdateLayeredWindow_Type pUpdateLayeredWindow;
BOOL WINAPI extUpdateLayeredWindow(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD);

typedef BOOL (WINAPI *SetLayeredWindowAttributes_Type)(HWND, COLORREF, BYTE, DWORD);
SetLayeredWindowAttributes_Type pSetLayeredWindowAttributes;
BOOL WINAPI extSetLayeredWindowAttributes(HWND, COLORREF, BYTE, DWORD);

typedef BOOL (WINAPI *UpdateLayeredWindowIndirect_Type)(HWND, const UPDATELAYEREDWINDOWINFO *);
UpdateLayeredWindowIndirect_Type pUpdateLayeredWindowIndirect;
BOOL WINAPI extUpdateLayeredWindowIndirect(HWND, const UPDATELAYEREDWINDOWINFO *);

typedef HWND (WINAPI *SetParent_Type)(HWND, HWND);
SetParent_Type pSetParent;
HWND WINAPI extSetParent(HWND, HWND);

typedef BOOL (WINAPI *SetSystemCursor_Type)(HCURSOR, DWORD);
SetSystemCursor_Type pSetSystemCursor;
BOOL WINAPI extSetSystemCursor(HCURSOR, DWORD);

typedef BOOL (WINAPI *DrawEdge_Type)(HDC, LPRECT, UINT, UINT);
DrawEdge_Type pDrawEdge;
BOOL WINAPI extDrawEdge(HDC, LPRECT, UINT, UINT);

typedef int (WINAPI *SetWindowRgn_Type)(HWND, HRGN, BOOL);
SetWindowRgn_Type pSetWindowRgn;
int WINAPI extSetWindowRgn(HWND, HRGN, BOOL);
typedef int (WINAPI *GetWindowRgn_Type)(HWND, HRGN);
GetWindowRgn_Type pGetWindowRgn;
int WINAPI extGetWindowRgn(HWND, HRGN);
GetMessagePos_Type pGetMessagePos;

typedef LONG (WINAPI *GetDialogBaseUnits_Type)();
GetDialogBaseUnits_Type pGetDialogBaseUnits;
LONG WINAPI extGetDialogBaseUnits();

typedef int (WINAPI *GetKeyboardType_Type)(int);
GetKeyboardType_Type pGetKeyboardType;
int WINAPI extGetKeyboardType(int);

typedef int (WINAPI *GetKeyNameTextA_Type)(LONG, LPSTR, int);
GetKeyNameTextA_Type pGetKeyNameTextA;
int WINAPI extGetKeyNameTextA(LONG, LPSTR, int);
typedef int (WINAPI *GetKeyNameTextW_Type)(LONG, LPWSTR, int);
GetKeyNameTextW_Type pGetKeyNameTextW;
int WINAPI extGetKeyNameTextW(LONG, LPWSTR, int);

typedef BOOL (WINAPI *EnableMenuItem_Type)(HMENU, UINT, UINT);
EnableMenuItem_Type pEnableMenuItem;
BOOL WINAPI extEnableMenuItem(HMENU, UINT, UINT);

typedef HWND (WINAPI *CreateMDIWindowA_Type)(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HINSTANCE, LPARAM);
CreateMDIWindowA_Type pCreateMDIWindowA;
HWND WINAPI extCreateMDIWindowA(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HINSTANCE, LPARAM);

typedef HWND (WINAPI *CreateMDIWindowW_Type)(LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HINSTANCE, LPARAM);
CreateMDIWindowW_Type pCreateMDIWindowW;
HWND WINAPI extCreateMDIWindowW(LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HINSTANCE, LPARAM);

typedef BOOL (WINAPI *SetDlgItemTextA_Type)(HWND, int, LPCSTR);
SetDlgItemTextA_Type pSetDlgItemTextA;
BOOL WINAPI extSetDlgItemTextA(HWND, int, LPCSTR);

typedef BOOL (WINAPI *SetDlgItemTextW_Type)(HWND, int, LPCWSTR);
SetDlgItemTextW_Type pSetDlgItemTextW;
BOOL WINAPI extSetDlgItemTextW(HWND, int, LPCWSTR);

#ifdef TRACEREGIONS
typedef int (WINAPI *GetWindowRgnBox_Type)(HWND, LPRECT);
GetWindowRgnBox_Type pGetWindowRgnBox;
int WINAPI extGetWindowRgnBox(HWND, LPRECT);
#endif TRACEREGIONS

#define _Warn(s) MessageBox(0, s, "to do", MB_ICONEXCLAMATION)

BOOL IsChangeDisplaySettingsHotPatched = FALSE;
BOOL InMainWinCreation = FALSE;
BOOL MovedInCallback = FALSE;
HWND CurrentActiveMovieWin = NULL;

extern BOOL bFlippedDC;
extern HDC hFlippedDC;
extern void HandleHotKeys(HWND, UINT, LPARAM, WPARAM);

#ifdef TRACEWAITOBJECTS
typedef DWORD (WINAPI *MsgWaitForMultipleObjects_Type)(DWORD, const HANDLE *, BOOL, DWORD, DWORD);
DWORD WINAPI extMsgWaitForMultipleObjects(DWORD, const HANDLE *, BOOL, DWORD, DWORD);
MsgWaitForMultipleObjects_Type pMsgWaitForMultipleObjects;
#endif // TRACEWAITOBJECTS

#ifdef HACKVIRTUALKEYS
typedef UINT (WINAPI *MapVirtualKey_Type)(UINT, UINT);
UINT WINAPI extMapVirtualKeyA(UINT, UINT);
UINT WINAPI extMapVirtualKeyW(UINT, UINT);
MapVirtualKey_Type pMapVirtualKeyA, pMapVirtualKeyW;
#endif // HACKVIRTUALKEYS

#ifdef TRACEALL
typedef BOOL (WINAPI *EndDialog_Type)(HWND, INT_PTR);
EndDialog_Type pEndDialog;
BOOL WINAPI extEndDialog(HWND, INT_PTR);
typedef BOOL (WINAPI *TrackMouseEvent_Type)(LPTRACKMOUSEEVENT);
TrackMouseEvent_Type pTrackMouseEvent;
BOOL WINAPI extTrackMouseEvent(LPTRACKMOUSEEVENT);
typedef BOOL (WINAPI *FlashWindow_Type)(HWND, BOOL);
FlashWindow_Type pFlashWindow;
BOOL WINAPI extFlashWindow(HWND, BOOL);
#endif

#ifdef TRACEPOINTS
typedef BOOL (WINAPI *SetRect_Type)(LPRECT, int, int, int, int);
SetRect_Type pSetRect;
BOOL WINAPI extSetRect(LPRECT, int, int, int, int);
typedef BOOL (WINAPI *PtInRect_Type)(CONST RECT *, POINT);
PtInRect_Type pPtInRect;
BOOL WINAPI extPtInRect(CONST RECT *, POINT);
typedef BOOL (WINAPI *SetRectEmpty_Type)(LPRECT);
SetRectEmpty_Type pSetRectEmpty;
BOOL WINAPI extSetRectEmpty(LPRECT);
typedef BOOL (WINAPI *OffsetRect_Type)(LPRECT, int, int);
OffsetRect_Type pOffsetRect;
BOOL WINAPI extOffsetRect(LPRECT, int, int);
typedef BOOL (WINAPI *InflateRect_Type)(LPRECT, int, int);
InflateRect_Type pInflateRect;
BOOL WINAPI extInflateRect(LPRECT, int, int);
typedef BOOL (WINAPI *OperateRect_Type)(LPRECT, CONST RECT *, CONST RECT *);
OperateRect_Type pIntersectRect, pUnionRect, pSubtractRect;
BOOL WINAPI extIntersectRect(LPRECT, CONST RECT *, CONST RECT *);
BOOL WINAPI extUnionRect(LPRECT, CONST RECT *, CONST RECT *);
BOOL WINAPI extSubtractRect(LPRECT, CONST RECT *, CONST RECT *);
typedef BOOL (WINAPI *IsRectEmpty_Type)(CONST RECT *);
IsRectEmpty_Type pIsRectEmpty;
BOOL WINAPI extIsRectEmpty(CONST RECT *);
typedef BOOL (WINAPI *CopyRect_Type)(LPRECT, CONST RECT *);
CopyRect_Type pCopyRect;
BOOL WINAPI extCopyRect(LPRECT, CONST RECT *);
#endif // TRACEPOINTS

#ifdef TRACEWINDOWS
typedef BOOL (WINAPI *EnableWindow_Type)(HWND, BOOL);
EnableWindow_Type pEnableWindow;
BOOL WINAPI extEnableWindow(HWND, BOOL);
HWND WINAPI extGetFocus(void);
typedef HWND (WINAPI *GetFocus_Type)(void);
GetFocus_Type pGetFocus;
typedef HWND (WINAPI *SetFocus_Type)(HWND);
SetFocus_Type pSetFocus;
HWND WINAPI extSetFocus(HWND);
typedef BOOL (WINAPI *EnumChildWindows_Type)(HWND, WNDENUMPROC, LPARAM);
#endif // TRACEWINDOWS

#ifdef TRACEMENUS
typedef HMENU (WINAPI *CreateMenu_Type)(void);
CreateMenu_Type pCreateMenu, pCreatePopupMenu;
HMENU WINAPI extCreateMenu(void);
HMENU WINAPI extCreatePopupMenu(void);
typedef UINT (WINAPI *GetMenuState_Type)(HMENU, UINT, UINT);
GetMenuState_Type pGetMenuState;
UINT WINAPI extGetMenuState(HMENU, UINT, UINT);
typedef BOOL (WINAPI *GetMenuItemInfoA_Type)(HMENU, UINT, BOOL, LPMENUITEMINFOA);
GetMenuItemInfoA_Type pGetMenuItemInfoA;
BOOL WINAPI extGetMenuItemInfoA(HMENU, UINT, BOOL, LPMENUITEMINFOA);
typedef BOOL (WINAPI *GetMenuItemInfoW_Type)(HMENU, UINT, BOOL, LPMENUITEMINFOW);
GetMenuItemInfoW_Type pGetMenuItemInfoW;
BOOL WINAPI extGetMenuItemInfoW(HMENU, UINT, BOOL, LPMENUITEMINFOW);
typedef BOOL (WINAPI *RemoveMenu_Type)(HMENU, UINT, UINT);
RemoveMenu_Type pRemoveMenu, pDeleteMenu;
BOOL WINAPI extRemoveMenu(HMENU, UINT, UINT);
BOOL WINAPI extDeleteMenu(HMENU, UINT, UINT);
typedef BOOL (WINAPI *DestroyMenu_Type)(HMENU);
DestroyMenu_Type pDestroyMenu;
BOOL WINAPI extDestroyMenu(HMENU);
typedef BOOL (WINAPI *ModifyMenuA_Type)(HMENU, UINT, UINT, UINT_PTR, LPCSTR);
ModifyMenuA_Type pModifyMenuA;
BOOL WINAPI extModifyMenuA(HMENU, UINT, UINT, UINT_PTR, LPCSTR);
typedef BOOL (WINAPI *SetMenuItemBitmaps_Type)(HMENU, UINT, UINT, HBITMAP, HBITMAP);
SetMenuItemBitmaps_Type pSetMenuItemBitmaps;
BOOL WINAPI extSetMenuItemBitmaps(HMENU, UINT, UINT, HBITMAP, HBITMAP);
typedef BOOL (WINAPI *InsertMenuItemW_Type)(HMENU, UINT, BOOL, LPCMENUITEMINFOW);
InsertMenuItemW_Type pInsertMenuItemW;
BOOL WINAPI extInsertMenuItemW(HMENU, UINT, BOOL, LPCMENUITEMINFOW);
typedef BOOL (WINAPI *AppendMenuW_Type)(HMENU, UINT, UINT_PTR, LPCWSTR);
AppendMenuW_Type pAppendMenuW;
BOOL WINAPI extAppendMenuW(HMENU, UINT, UINT_PTR, LPCWSTR);
typedef BOOL (WINAPI *InsertMenuW_Type)(HMENU, UINT, UINT, UINT_PTR, LPCWSTR);
InsertMenuW_Type pInsertMenuW;
BOOL WINAPI extInsertMenuW(HMENU, UINT, UINT, UINT_PTR, LPCWSTR);
typedef LONG (WINAPI *GetMenuCheckMarkDimensions_Type)(void);
GetMenuCheckMarkDimensions_Type pGetMenuCheckMarkDimensions;
LONG WINAPI extGetMenuCheckMarkDimensions();
#endif // TRACEMENUS

typedef HMENU (WINAPI *LoadMenuA_Type)(HINSTANCE, LPCSTR);
LoadMenuA_Type pLoadMenuA;
HMENU WINAPI extLoadMenuA(HINSTANCE, LPCSTR);
typedef HMENU (WINAPI *LoadMenuW_Type)(HINSTANCE, LPCWSTR);
LoadMenuW_Type pLoadMenuW;
HMENU WINAPI extLoadMenuW(HINSTANCE, LPCWSTR);
typedef HMENU (WINAPI *LoadMenuIndirectA_Type)(const MENUTEMPLATEA *);
LoadMenuIndirectA_Type pLoadMenuIndirectA;
HMENU WINAPI extLoadMenuIndirectA(const MENUTEMPLATEA *);
typedef HMENU (WINAPI *LoadMenuIndirectW_Type)(const MENUTEMPLATEW *);
LoadMenuIndirectW_Type pLoadMenuIndirectW;
HMENU WINAPI extLoadMenuIndirectW(const MENUTEMPLATEW *);
typedef BOOL (WINAPI *SetMenuItemInfoA_Type)(HMENU, UINT, BOOL, LPCMENUITEMINFOA);
SetMenuItemInfoA_Type pSetMenuItemInfoA;
BOOL WINAPI extSetMenuItemInfoA(HMENU, UINT, BOOL, LPCMENUITEMINFOA);
typedef BOOL (WINAPI *InsertMenuItemA_Type)(HMENU, UINT, BOOL, LPCMENUITEMINFOA);
InsertMenuItemA_Type pInsertMenuItemA;
BOOL WINAPI extInsertMenuItemA(HMENU, UINT, BOOL, LPCMENUITEMINFOA);
typedef BOOL (WINAPI *InsertMenuA_Type)(HMENU, UINT, UINT, UINT_PTR, LPCSTR);
InsertMenuA_Type pInsertMenuA;
BOOL WINAPI extInsertMenuA(HMENU, UINT, UINT, UINT_PTR, LPCSTR);

typedef HMENU (WINAPI *GetMenu_Type)(HWND);
GetMenu_Type pGetMenu;
HMENU WINAPI extGetMenu(HWND);
typedef BOOL (WINAPI *SetMenu_Type)(HWND, HMENU);
SetMenu_Type pSetMenu;
BOOL WINAPI extSetMenu(HWND, HMENU);
typedef BOOL (WINAPI *SetWindowTextA_Type)(HWND, LPCSTR);
SetWindowTextA_Type pSetWindowTextA;
BOOL WINAPI extSetWindowTextA(HWND, LPCSTR);
typedef BOOL (WINAPI *SetWindowTextW_Type)(HWND, LPCWSTR);
SetWindowTextW_Type pSetWindowTextW;
BOOL WINAPI extSetWindowTextW(HWND, LPCWSTR);
typedef int (WINAPI *GetWindowTextA_Type)(HWND, LPSTR, int);
GetWindowTextA_Type pGetWindowTextA;
int WINAPI extGetWindowTextA(HWND, LPSTR, int);
typedef LPSTR (WINAPI *CharPrevA_Type)(LPCSTR, LPCSTR);
CharPrevA_Type pCharPrevA;
LPSTR WINAPI extCharPrevA(LPCSTR, LPCSTR);
typedef LPSTR (WINAPI *CharNextA_Type)(LPCSTR);
CharNextA_Type pCharNextA;
LPSTR WINAPI extCharNextA(LPCSTR);
typedef BOOL (WINAPI *OemToCharA_Type)(LPCSTR, LPSTR);
OemToCharA_Type pOemToCharA;
BOOL WINAPI extOemToCharA(LPCSTR, LPSTR);
typedef BOOL (WINAPI *CharToOemA_Type)(LPCSTR, LPSTR);
OemToCharA_Type pCharToOemA;
BOOL WINAPI extCharToOemA(LPCSTR, LPSTR);
typedef BOOL (WINAPI *CharToOemBuffA_Type)(LPCSTR, LPSTR, DWORD);
CharToOemBuffA_Type pCharToOemBuffA, pOemToCharBuffA;
BOOL WINAPI extCharToOemBuffA(LPCSTR, LPSTR, DWORD);
BOOL WINAPI extOemToCharBuffA(LPCSTR, LPSTR, DWORD);
typedef HRESULT (WINAPI *MessageBoxA_Type)(HWND, LPCSTR, LPCSTR, UINT);
MessageBoxA_Type pMessageBoxA = 0;
HRESULT WINAPI extMessageBoxA(HWND, LPCSTR, LPCSTR, UINT);
typedef HRESULT (WINAPI *MessageBoxW_Type)(HWND, LPCWSTR, LPCWSTR, UINT);
MessageBoxW_Type pMessageBoxW = 0;
HRESULT WINAPI extMessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT);
typedef LRESULT (WINAPI *DefProc_Type)(HWND, UINT, WPARAM, LPARAM);
DefProc_Type pDefFrameProcA;
LRESULT WINAPI extDefFrameProcA(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI extDefMDIChildProcA(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI extDefDlgProcA(HWND, UINT, WPARAM, LPARAM);
typedef int (WINAPI *GetClassNameA_Type)(HWND, LPSTR, int);
GetClassNameA_Type pGetClassNameA;
int WINAPI extGetClassNameA(HWND, LPSTR, int);
typedef BOOL (WINAPI *GetClassInfoA_Type)(HINSTANCE, LPCSTR, LPWNDCLASSA);
GetClassInfoA_Type pGetClassInfoA;
BOOL WINAPI extGetClassInfoA(HINSTANCE, LPCSTR, LPWNDCLASSA);
typedef BOOL (WINAPI *UnregisterClassA_Type)(LPCSTR, HINSTANCE);
UnregisterClassA_Type pUnregisterClassA;
BOOL WINAPI extUnregisterClassA(LPCSTR, HINSTANCE);

typedef HCURSOR (WINAPI *LoadCursorA_Type)(HINSTANCE, LPCSTR);
LoadCursorA_Type pLoadCursorA;
HCURSOR WINAPI extLoadCursorA(HINSTANCE, LPCSTR);
typedef HCURSOR (WINAPI *LoadCursorW_Type)(HINSTANCE, LPCWSTR);
LoadCursorW_Type pLoadCursorW;
HCURSOR WINAPI extLoadCursorW(HINSTANCE, LPCWSTR);

typedef BOOL (WINAPI *AppendMenuA_Type)(HMENU, UINT, UINT_PTR, LPCSTR);
AppendMenuA_Type pAppendMenuA;
BOOL WINAPI extAppendMenuA(HMENU, UINT, UINT_PTR, LPCSTR);

#ifdef TRACECURSOR
typedef HCURSOR (WINAPI *SetCursor_Type)(HCURSOR);
SetCursor_Type pSetCursor;
HCURSOR WINAPI extSetCursor(HCURSOR);
typedef HCURSOR (WINAPI *CreateCursor_Type)(HINSTANCE, int, int, int, int, const VOID *, const VOID *);
CreateCursor_Type pCreateCursor;
HCURSOR WINAPI extCreateCursor(HINSTANCE, int, int, int, int, const VOID *, const VOID *);
typedef BOOL (WINAPI *DestroyCursor_Type)(HCURSOR);
DestroyCursor_Type pDestroyCursor;
BOOL WINAPI extDestroyCursor(HCURSOR);
typedef HICON (WINAPI *CreateIconIndirect_Type)(PICONINFO);
CreateIconIndirect_Type pCreateIconIndirect;
HICON WINAPI extCreateIconIndirect(PICONINFO);
#endif // TRACECURSOR

typedef HCURSOR (WINAPI *LoadCursorFromFileA_Type)(LPCSTR);
LoadCursorFromFileA_Type pLoadCursorFromFileA;
HCURSOR WINAPI extLoadCursorFromFileA(LPCSTR);
typedef HCURSOR (WINAPI *LoadCursorFromFileW_Type)(LPCWSTR);
LoadCursorFromFileW_Type pLoadCursorFromFileW;
HCURSOR WINAPI extLoadCursorFromFileW(LPCWSTR);
 
typedef int (WINAPI *LoadStringA_Type)(HINSTANCE, UINT, LPSTR, int);
LoadStringA_Type pLoadStringA;
int WINAPI extLoadStringA(HINSTANCE, UINT, LPSTR, int);

typedef DWORD (WINAPI *GetSysColor_Type)(int);
GetSysColor_Type pGetSysColor;
DWORD WINAPI extGetSysColor(int);

typedef BOOL (WINAPI *WinHelpA_Type)(HWND, LPCSTR, UINT, ULONG_PTR);
WinHelpA_Type pWinHelpA;
extern BOOL WINAPI extWinHelpA(HWND, LPCSTR, UINT, ULONG_PTR);
typedef BOOL (WINAPI *WinHelpW_Type)(HWND, LPCWSTR, UINT, ULONG_PTR);
WinHelpW_Type pWinHelpW;
extern BOOL WINAPI extWinHelpW(HWND, LPCWSTR, UINT, ULONG_PTR);

typedef BOOL (WINAPI *DrawFrameControl_Type)(HDC, LPRECT, UINT, UINT);
DrawFrameControl_Type pDrawFrameControl;
BOOL WINAPI extDrawFrameControl(HDC, LPRECT, UINT, UINT);
typedef BOOL (WINAPI *DrawFocusRect_Type)(HDC, const RECT *);
DrawFocusRect_Type pDrawFocusRect;
BOOL WINAPI extDrawFocusRect(HDC, const RECT *);

typedef BOOL (WINAPI *WINNLSEnableIME_Type)(HWND, BOOL);
WINNLSEnableIME_Type pWINNLSEnableIME;
BOOL WINAPI extWINNLSEnableIME(HWND, BOOL);

typedef HWND (WINAPI *FindWindowA_Type)(LPCSTR, LPCSTR);
FindWindowA_Type pFindWindowA;
HWND WINAPI extFindWindowA(LPCSTR, LPCSTR);

typedef HWND (WINAPI *FindWindowW_Type)(LPCWSTR, LPCWSTR);
FindWindowW_Type pFindWindowW;
HWND WINAPI extFindWindowW(LPCWSTR, LPCWSTR);

typedef BOOL (WINAPI *EnumChildWindows_Type)(HWND, WNDENUMPROC, LPARAM);
EnumChildWindows_Type pEnumChildWindows;
BOOL WINAPI extEnumChildWindows(HWND, WNDENUMPROC, LPARAM);

typedef LRESULT (WINAPI *SendDlgItemMessageA_Type)(HWND, int, UINT, WPARAM, LPARAM);
SendDlgItemMessageA_Type pSendDlgItemMessageA;
LRESULT WINAPI extSendDlgItemMessageA(HWND, int, UINT, WPARAM, LPARAM);

typedef UINT (WINAPI *UserRealizePalette_Type)(HDC);
UserRealizePalette_Type pUserRealizePalette;
UINT WINAPI extUserRealizePalette(HDC);

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "GetSysColor", (FARPROC)GetSysColor, (FARPROC *)&pGetSysColor, (FARPROC)extGetSysColor}, 
	{HOOK_HOT_CANDIDATE, 0, "LoadCursorA", (FARPROC)LoadCursorA, (FARPROC *)&pLoadCursorA, (FARPROC)extLoadCursorA},
	{HOOK_HOT_CANDIDATE, 0, "LoadCursorW", (FARPROC)LoadCursorW, (FARPROC *)&pLoadCursorW, (FARPROC)extLoadCursorW},
	{HOOK_IAT_CANDIDATE, 0, "SetSystemCursor", (FARPROC)SetSystemCursor, (FARPROC *)&pSetSystemCursor, (FARPROC)extSetSystemCursor},
#ifdef TRACECURSOR
	{HOOK_IAT_CANDIDATE, 0, "SetCursor", (FARPROC)SetCursor, (FARPROC *)&pSetCursor, (FARPROC)extSetCursor},
	{HOOK_IAT_CANDIDATE, 0, "LoadImageA", (FARPROC)LoadImageA, (FARPROC *)&pLoadImageA, (FARPROC)extLoadImageA}, 
	{HOOK_IAT_CANDIDATE, 0, "LoadImageW", (FARPROC)LoadImageW, (FARPROC *)&pLoadImageW, (FARPROC)extLoadImageW}, 
	{HOOK_IAT_CANDIDATE, 0, "CreateCursor", (FARPROC)CreateCursor, (FARPROC *)&pCreateCursor, (FARPROC)extCreateCursor}, 
	{HOOK_IAT_CANDIDATE, 0, "DestroyCursor", (FARPROC)DestroyCursor, (FARPROC *)&pDestroyCursor, (FARPROC)extDestroyCursor}, 
	{HOOK_IAT_CANDIDATE, 0, "CreateIconIndirect", (FARPROC)CreateIconIndirect, (FARPROC *)&pCreateIconIndirect, (FARPROC)extCreateIconIndirect}, 
#endif // TRACECURSOR
#ifdef TRACEREGIONS
	{HOOK_HOT_CANDIDATE, 0, "GetWindowRgnBox", (FARPROC)GetWindowRgnBox, (FARPROC *)&pGetWindowRgnBox, (FARPROC)extGetWindowRgnBox},
#endif // TRACEREGIONS
#ifdef HACKVIRTUALKEYS
	{HOOK_IAT_CANDIDATE, 0, "MapVirtualKeyA", (FARPROC)MapVirtualKeyA, (FARPROC *)&pMapVirtualKeyA, (FARPROC)extMapVirtualKeyA}, 
	{HOOK_IAT_CANDIDATE, 0, "MapVirtualKeyW", (FARPROC)MapVirtualKeyW, (FARPROC *)&pMapVirtualKeyW, (FARPROC)extMapVirtualKeyW}, 
#endif // HACKVIRTUALKEYS
	{HOOK_HOT_CANDIDATE, 0, "FillRect", (FARPROC)NULL, (FARPROC *)&pFillRect, (FARPROC)extFillRect},
	{HOOK_IAT_CANDIDATE, 0, "UpdateWindow", (FARPROC)UpdateWindow, (FARPROC *)&pUpdateWindow, (FARPROC)extUpdateWindow}, // v2.04.04: needed for "Hide Desktop" option
	{HOOK_HOT_CANDIDATE, 0, "ChangeDisplaySettingsA", (FARPROC)ChangeDisplaySettingsA, (FARPROC *)&pChangeDisplaySettingsA, (FARPROC)extChangeDisplaySettingsA},
	{HOOK_HOT_CANDIDATE, 0, "ChangeDisplaySettingsExA", (FARPROC)ChangeDisplaySettingsExA, (FARPROC *)&pChangeDisplaySettingsExA, (FARPROC)extChangeDisplaySettingsExA},
	{HOOK_HOT_CANDIDATE, 0, "ChangeDisplaySettingsW", (FARPROC)NULL, (FARPROC *)&pChangeDisplaySettingsW, (FARPROC)extChangeDisplaySettingsW}, // ref. by Knights of Honor
	{HOOK_HOT_CANDIDATE, 0, "ChangeDisplaySettingsExW", (FARPROC)NULL, (FARPROC *)&pChangeDisplaySettingsExW, (FARPROC)extChangeDisplaySettingsExW},
	{HOOK_HOT_CANDIDATE, 0, "GetMonitorInfoA", (FARPROC)GetMonitorInfoA, (FARPROC *)&pGetMonitorInfoA, (FARPROC)extGetMonitorInfoA},
	{HOOK_HOT_CANDIDATE, 0, "GetMonitorInfoW", (FARPROC)GetMonitorInfoW, (FARPROC *)&pGetMonitorInfoW, (FARPROC)extGetMonitorInfoW},
	{HOOK_HOT_CANDIDATE, 0, "ShowCursor", (FARPROC)ShowCursor, (FARPROC *)&pShowCursor, (FARPROC)extShowCursor},
	{HOOK_IAT_CANDIDATE, 0, "CreateDialogIndirectParamA", (FARPROC)CreateDialogIndirectParamA, (FARPROC *)&pCreateDialogIndirectParamA, (FARPROC)extCreateDialogIndirectParamA},
	{HOOK_IAT_CANDIDATE, 0, "CreateDialogIndirectParamW", (FARPROC)CreateDialogIndirectParamW, (FARPROC *)&pCreateDialogIndirectParamW, (FARPROC)extCreateDialogIndirectParamW},
	{HOOK_IAT_CANDIDATE, 0, "CreateDialogParamA", (FARPROC)CreateDialogParamA, (FARPROC *)&pCreateDialogParamA, (FARPROC)extCreateDialogParamA},
	{HOOK_IAT_CANDIDATE, 0, "CreateDialogParamW", (FARPROC)CreateDialogParamW, (FARPROC *)&pCreateDialogParamW, (FARPROC)extCreateDialogParamW},
	{HOOK_IAT_CANDIDATE, 0, "DialogBoxParamA", (FARPROC)DialogBoxParamA, (FARPROC *)&pDialogBoxParamA, (FARPROC)extDialogBoxParamA},
	{HOOK_IAT_CANDIDATE, 0, "DialogBoxParamW", (FARPROC)DialogBoxParamW, (FARPROC *)&pDialogBoxParamW, (FARPROC)extDialogBoxParamW},
	{HOOK_HOT_CANDIDATE, 0, "DialogBoxIndirectParamA", (FARPROC)DialogBoxIndirectParamA, (FARPROC *)&pDialogBoxIndirectParamA, (FARPROC)extDialogBoxIndirectParamA},
	{HOOK_HOT_CANDIDATE, 0, "DialogBoxIndirectParamW", (FARPROC)DialogBoxIndirectParamW, (FARPROC *)&pDialogBoxIndirectParamW, (FARPROC)extDialogBoxIndirectParamW},
	{HOOK_HOT_CANDIDATE, 0, "MoveWindow", (FARPROC)MoveWindow, (FARPROC *)&pMoveWindow, (FARPROC)extMoveWindow},
	{HOOK_HOT_CANDIDATE, 0, "SetWindowPos", (FARPROC)SetWindowPos, (FARPROC *)&pSetWindowPos, (FARPROC)extSetWindowPos},
	{HOOK_HOT_CANDIDATE, 0, "EnumDisplaySettingsA", (FARPROC)EnumDisplaySettingsA, (FARPROC *)&pEnumDisplaySettingsA, (FARPROC)extEnumDisplaySettingsA},
	{HOOK_HOT_CANDIDATE, 0, "EnumDisplaySettingsW", (FARPROC)EnumDisplaySettingsW, (FARPROC *)&pEnumDisplaySettingsW, (FARPROC)extEnumDisplaySettingsW},
	{HOOK_IAT_CANDIDATE, 0, "GetClipCursor", (FARPROC)GetClipCursor, (FARPROC*)&pGetClipCursor, (FARPROC)extGetClipCursor},
	{HOOK_HOT_CANDIDATE, 0, "ClipCursor", (FARPROC)ClipCursor, (FARPROC *)&pClipCursor, (FARPROC)extClipCursor},
	// DefWindowProcA is HOOK_HOT_REQUIRED for nls support ....
	{HOOK_HOT_REQUIRED,  0, "DefWindowProcA", (FARPROC)DefWindowProcA, (FARPROC *)&pDefWindowProcA, (FARPROC)extDefWindowProcA},
	{HOOK_HOT_CANDIDATE, 0, "DefWindowProcW", (FARPROC)DefWindowProcW, (FARPROC *)&pDefWindowProcW, (FARPROC)extDefWindowProcW},
	// CreateWindowExA & CreateWindowExW are HOOK_HOT_REQUIRED for nls support ....
	{HOOK_HOT_CANDIDATE, 0, "CreateWindowExA", (FARPROC)CreateWindowExA, (FARPROC *)&pCreateWindowExA, (FARPROC)extCreateWindowExA},
	{HOOK_HOT_CANDIDATE, 0, "CreateWindowExW", (FARPROC)CreateWindowExW, (FARPROC *)&pCreateWindowExW, (FARPROC)extCreateWindowExW},
	{HOOK_IAT_CANDIDATE, 0, "RegisterClassExA", (FARPROC)RegisterClassExA, (FARPROC *)&pRegisterClassExA, (FARPROC)extRegisterClassExA},
	{HOOK_IAT_CANDIDATE, 0, "RegisterClassA", (FARPROC)RegisterClassA, (FARPROC *)&pRegisterClassA, (FARPROC)extRegisterClassA},
	{HOOK_IAT_CANDIDATE, 0, "RegisterClassExW", (FARPROC)RegisterClassExW, (FARPROC *)&pRegisterClassExW, (FARPROC)extRegisterClassExW},
	{HOOK_IAT_CANDIDATE, 0, "RegisterClassW", (FARPROC)RegisterClassW, (FARPROC *)&pRegisterClassW, (FARPROC)extRegisterClassW},
	{HOOK_HOT_CANDIDATE, 0, "GetSystemMetrics", (FARPROC)GetSystemMetrics, (FARPROC *)&pGetSystemMetrics, (FARPROC)extGetSystemMetrics},
	{HOOK_HOT_CANDIDATE, 0, "GetDesktopWindow", (FARPROC)GetDesktopWindow, (FARPROC *)&pGetDesktopWindow, (FARPROC)extGetDesktopWindow},
	{HOOK_IAT_CANDIDATE, 0, "CloseWindow", (FARPROC)NULL, (FARPROC *)&pCloseWindow, (FARPROC)extCloseWindow},
	{HOOK_IAT_CANDIDATE, 0, "DestroyWindow", (FARPROC)NULL, (FARPROC *)&pDestroyWindow, (FARPROC)extDestroyWindow},
	{HOOK_IAT_CANDIDATE, 0, "SetSysColors", (FARPROC)NULL, (FARPROC *)&pSetSysColors, (FARPROC)extSetSysColors},
	{HOOK_HOT_CANDIDATE, 0, "SetWindowLongA", (FARPROC)SetWindowLongA, (FARPROC *)&pSetWindowLongA, (FARPROC)extSetWindowLongA},
	{HOOK_HOT_CANDIDATE, 0, "GetWindowLongA", (FARPROC)GetWindowLongA, (FARPROC *)&pGetWindowLongA, (FARPROC)extGetWindowLongA}, 
	{HOOK_HOT_CANDIDATE, 0, "SetWindowLongW", (FARPROC)SetWindowLongW, (FARPROC *)&pSetWindowLongW, (FARPROC)extSetWindowLongW},
	{HOOK_HOT_CANDIDATE, 0, "GetWindowLongW", (FARPROC)GetWindowLongW, (FARPROC *)&pGetWindowLongW, (FARPROC)extGetWindowLongW}, 
	{HOOK_IAT_CANDIDATE, 0, "IsWindowVisible", (FARPROC)IsWindowVisible, (FARPROC *)&pIsWindowVisible, (FARPROC)extIsWindowVisible}, // ref. in dxw.SetClipper, CreateWindowCommon
	{HOOK_IAT_CANDIDATE, 0, "GetTopWindow", (FARPROC)GetTopWindow, (FARPROC *)&pGetTopWindow, (FARPROC)extGetTopWindow},
	{HOOK_IAT_CANDIDATE, 0, "SetParent", (FARPROC)SetParent, (FARPROC *)&pSetParent, (FARPROC)extSetParent},
	// hot by MinHook since v2.03.07
	{HOOK_HOT_CANDIDATE, 0, "SystemParametersInfoA", (FARPROC)SystemParametersInfoA, (FARPROC *)&pSystemParametersInfoA, (FARPROC)extSystemParametersInfoA},
	{HOOK_HOT_CANDIDATE, 0, "SystemParametersInfoW", (FARPROC)SystemParametersInfoW, (FARPROC *)&pSystemParametersInfoW, (FARPROC)extSystemParametersInfoW}, 
	{HOOK_HOT_CANDIDATE, 0, "BringWindowToTop", (FARPROC)BringWindowToTop, (FARPROC *)&pBringWindowToTop, (FARPROC)extBringWindowToTop},
	{HOOK_HOT_CANDIDATE, 0, "SetForegroundWindow", (FARPROC)SetForegroundWindow, (FARPROC *)&pSetForegroundWindow, (FARPROC)extSetForegroundWindow},
	{HOOK_HOT_CANDIDATE, 0, "ChildWindowFromPoint", (FARPROC)ChildWindowFromPoint, (FARPROC *)&pChildWindowFromPoint, (FARPROC)extChildWindowFromPoint},
	{HOOK_HOT_CANDIDATE, 0, "ChildWindowFromPointEx", (FARPROC)ChildWindowFromPointEx, (FARPROC *)&pChildWindowFromPointEx, (FARPROC)extChildWindowFromPointEx},
	{HOOK_HOT_CANDIDATE, 0, "WindowFromPoint", (FARPROC)WindowFromPoint, (FARPROC *)&pWindowFromPoint, (FARPROC)extWindowFromPoint},
	{HOOK_HOT_REQUIRED,  0 ,"SetWindowsHookExA", (FARPROC)SetWindowsHookExA, (FARPROC *)&pSetWindowsHookExA, (FARPROC)extSetWindowsHookExA},
	{HOOK_HOT_REQUIRED,  0 ,"SetWindowsHookExW", (FARPROC)SetWindowsHookExW, (FARPROC *)&pSetWindowsHookExW, (FARPROC)extSetWindowsHookExW},
	{HOOK_HOT_REQUIRED,  0 ,"UnhookWindowsHookEx", (FARPROC)UnhookWindowsHookEx, (FARPROC *)&pUnhookWindowsHookEx, (FARPROC)extUnhookWindowsHookEx},
	{HOOK_IAT_CANDIDATE, 0, "GetDC", (FARPROC)GetDC, (FARPROC *)&pGDIGetDC, (FARPROC)extGDIGetDC},
	{HOOK_IAT_CANDIDATE, 0, "GetDCEx", (FARPROC)GetDCEx, (FARPROC *)&pGDIGetDCEx, (FARPROC)extGDIGetDCEx},
	{HOOK_IAT_CANDIDATE, 0, "GetWindowDC", (FARPROC)GetWindowDC, (FARPROC *)&pGDIGetWindowDC, (FARPROC)extGDIGetWindowDC}, 
	{HOOK_IAT_CANDIDATE, 0, "ReleaseDC", (FARPROC)ReleaseDC, (FARPROC *)&pGDIReleaseDC, (FARPROC)extGDIReleaseDC},
	{HOOK_HOT_CANDIDATE, 0, "BeginPaint", (FARPROC)BeginPaint, (FARPROC *)&pBeginPaint, (FARPROC)extBeginPaint},
	{HOOK_HOT_CANDIDATE, 0, "EndPaint", (FARPROC)EndPaint, (FARPROC *)&pEndPaint, (FARPROC)extEndPaint},
	{HOOK_HOT_CANDIDATE, 0, "ScrollDC", (FARPROC)NULL, (FARPROC *)&pScrollDC, (FARPROC)extScrollDC},
	// ShowScrollBar and DrawMenuBar both added to fix the Galapagos menu bar, but with no success !!!!
	{HOOK_HOT_CANDIDATE, 0, "ShowScrollBar", (FARPROC)ShowScrollBar, (FARPROC *)&pShowScrollBar, (FARPROC)extShowScrollBar},
	{HOOK_HOT_CANDIDATE, 0, "DrawMenuBar", (FARPROC)DrawMenuBar, (FARPROC *)&pDrawMenuBar, (FARPROC)extDrawMenuBar},
	{HOOK_HOT_CANDIDATE, 0, "EnumDisplayDevicesA", (FARPROC)EnumDisplayDevicesA, (FARPROC *)&pEnumDisplayDevicesA, (FARPROC)extEnumDisplayDevicesA},
	// EnumDisplayDevicesW used by "Battleground Europe" ...
	{HOOK_HOT_CANDIDATE, 0, "EnumDisplayDevicesW", (FARPROC)EnumDisplayDevicesW, (FARPROC *)&pEnumDisplayDevicesW, (FARPROC)extEnumDisplayDevicesW},
	{HOOK_IAT_CANDIDATE, 0, "EnumWindows", (FARPROC)NULL, (FARPROC *)&pEnumWindows, (FARPROC)extEnumWindows},
	{HOOK_IAT_CANDIDATE, 0, "AdjustWindowRect", (FARPROC)NULL, (FARPROC *)&pAdjustWindowRect, (FARPROC)extAdjustWindowRect},
	{HOOK_IAT_CANDIDATE, 0, "AdjustWindowRectEx", (FARPROC)AdjustWindowRectEx, (FARPROC *)&pAdjustWindowRectEx, (FARPROC)extAdjustWindowRectEx},
	{HOOK_HOT_CANDIDATE, 0, "GetActiveWindow", (FARPROC)GetActiveWindow, (FARPROC *)&pGetActiveWindow, (FARPROC)extGetActiveWindow},
	{HOOK_HOT_CANDIDATE, 0, "GetForegroundWindow", (FARPROC)GetForegroundWindow, (FARPROC *)&pGetForegroundWindow, (FARPROC)extGetForegroundWindow},
	{HOOK_HOT_CANDIDATE, 0, "ShowWindow", (FARPROC)ShowWindow, (FARPROC *)&pShowWindow, (FARPROC)extShowWindow},
	{HOOK_HOT_CANDIDATE, 0, "BeginDeferWindowPos", (FARPROC)BeginDeferWindowPos, (FARPROC *)&pBeginDeferWindowPos, (FARPROC)extBeginDeferWindowPos},
	{HOOK_HOT_CANDIDATE, 0, "DeferWindowPos", (FARPROC)DeferWindowPos, (FARPROC *)&pGDIDeferWindowPos, (FARPROC)extDeferWindowPos},
	{HOOK_HOT_CANDIDATE, 0, "CallWindowProcA", (FARPROC)CallWindowProcA, (FARPROC *)&pCallWindowProcA, (FARPROC)extCallWindowProcA},
	{HOOK_HOT_CANDIDATE, 0, "CallWindowProcW", (FARPROC)CallWindowProcW, (FARPROC *)&pCallWindowProcW, (FARPROC)extCallWindowProcW},
	{HOOK_IAT_CANDIDATE, 0, "IsZoomed", (FARPROC)NULL, (FARPROC *)&pIsZoomed, (FARPROC)extIsZoomed},
	{HOOK_HOT_CANDIDATE, 0, "EnumDisplayMonitors", (FARPROC)EnumDisplayMonitors, (FARPROC *)&pEnumDisplayMonitors, (FARPROC)extEnumDisplayMonitors},
	
	// text hooks, needed for CUSTOMLOCALE and DUMPTEXT
	{HOOK_IAT_CANDIDATE, 0, "TabbedTextOutA", (FARPROC)TabbedTextOutA, (FARPROC *)&pTabbedTextOutA, (FARPROC)extTabbedTextOutA},
	{HOOK_IAT_CANDIDATE, 0, "TabbedTextOutW", (FARPROC)TabbedTextOutW, (FARPROC *)&pTabbedTextOutW, (FARPROC)extTabbedTextOutW},
	{HOOK_IAT_CANDIDATE, 0, "DrawTextA", (FARPROC)DrawTextA, (FARPROC *)&pDrawTextA, (FARPROC)extDrawTextA},
	{HOOK_IAT_CANDIDATE, 0, "DrawTextExA", (FARPROC)DrawTextExA, (FARPROC *)&pDrawTextExA, (FARPROC)extDrawTextExA},
	{HOOK_IAT_CANDIDATE, 0, "DrawTextW", (FARPROC)DrawTextW, (FARPROC *)&pDrawTextW, (FARPROC)extDrawTextW},
	{HOOK_IAT_CANDIDATE, 0, "DrawTextExW", (FARPROC)DrawTextExW, (FARPROC *)&pDrawTextExW, (FARPROC)extDrawTextExW},
	{HOOK_IAT_CANDIDATE, 0, "SetDlgItemTextA", (FARPROC)SetDlgItemTextA, (FARPROC *)&pSetDlgItemTextA, (FARPROC)extSetDlgItemTextA},
	{HOOK_IAT_CANDIDATE, 0, "SetDlgItemTextW", (FARPROC)SetDlgItemTextW, (FARPROC *)&pSetDlgItemTextW, (FARPROC)extSetDlgItemTextW},
	{HOOK_IAT_CANDIDATE, 0, "SendDlgItemMessageA", (FARPROC)SendDlgItemMessageA, (FARPROC *)&pSendDlgItemMessageA, (FARPROC)extSendDlgItemMessageA},

#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "GetWindowPlacement", (FARPROC)NULL, (FARPROC *)&pGetWindowPlacement, (FARPROC)extGetWindowPlacement},
	{HOOK_IAT_CANDIDATE, 0, "SetWindowPlacement", (FARPROC)NULL, (FARPROC *)&pSetWindowPlacement, (FARPROC)extSetWindowPlacement},
	{HOOK_IAT_CANDIDATE, 0, "GetCapture", (FARPROC)GetCapture, (FARPROC *)&pGetCapture, (FARPROC)extGetCapture}, 
	{HOOK_IAT_CANDIDATE, 0, "FlashWindow", (FARPROC)FlashWindow, (FARPROC *)&pFlashWindow, (FARPROC)extFlashWindow}, 
#endif // TRACEALL
#ifdef TRACEWAITOBJECTS
	{HOOK_IAT_CANDIDATE, 0, "MsgWaitForMultipleObjects", (FARPROC)MsgWaitForMultipleObjects, (FARPROC *)&pMsgWaitForMultipleObjects, (FARPROC)extMsgWaitForMultipleObjects}, 
#endif // TRACEWAITOBJECTS
#ifdef TRACEIMAGES
	{HOOK_IAT_CANDIDATE, 0, "LoadBitmapA", (FARPROC)NULL, (FARPROC *)&pLoadBitmapA, (FARPROC)extLoadBitmapA},
	{HOOK_IAT_CANDIDATE, 0, "LoadBitmapW", (FARPROC)NULL, (FARPROC *)&pLoadBitmapW, (FARPROC)extLoadBitmapW},
#endif // TRACEIMAGES
#ifdef TRACESYSCALLS
	{HOOK_HOT_CANDIDATE, 0, "LoadStringA", (FARPROC)LoadStringA, (FARPROC *)&pLoadStringA, (FARPROC)extLoadStringA},
	{HOOK_IAT_CANDIDATE, 0, "SetCapture", (FARPROC)SetCapture, (FARPROC *)&pSetCapture, (FARPROC)extSetCapture},
	{HOOK_IAT_CANDIDATE, 0, "ReleaseCapture", (FARPROC)ReleaseCapture, (FARPROC *)&pReleaseCapture, (FARPROC)extReleaseCapture},
	{HOOK_IAT_CANDIDATE, 0, "SetActiveWindow", (FARPROC)SetActiveWindow, (FARPROC *)&pSetActiveWindow, (FARPROC)extSetActiveWindow},
	{HOOK_IAT_CANDIDATE, 0, "IsWindow", (FARPROC)IsWindow, (FARPROC *)&pIsWindow, (FARPROC)extIsWindow},
	{HOOK_IAT_CANDIDATE, 0, "IsWindowEnabled", (FARPROC)IsWindowEnabled, (FARPROC *)&pIsWindowEnabled, (FARPROC)extIsWindowEnabled},
	{HOOK_HOT_CANDIDATE, 0, "IsIconic", (FARPROC)IsIconic, (FARPROC *)&pIsIconic, (FARPROC)extIsIconic},
	{HOOK_HOT_CANDIDATE, 0, "GetAncestor", (FARPROC)GetAncestor, (FARPROC *)&pGetAncestor, (FARPROC)extGetAncestor},
	{HOOK_HOT_CANDIDATE, 0, "GetWindow", (FARPROC)GetWindow, (FARPROC *)&pGetWindow, (FARPROC)extGetWindow},
	{HOOK_HOT_CANDIDATE, 0, "GetWindowThreadProcessId", (FARPROC)GetWindowThreadProcessId, (FARPROC *)&pGetWindowThreadProcessId, (FARPROC)extGetWindowThreadProcessId},
	{HOOK_HOT_CANDIDATE, 0, "SetScrollInfo", (FARPROC)SetScrollInfo, (FARPROC *)&pSetScrollInfo, (FARPROC)extSetScrollInfo},
	{HOOK_HOT_CANDIDATE, 0, "GetScrollInfo", (FARPROC)GetScrollInfo, (FARPROC *)&pGetScrollInfo, (FARPROC)extGetScrollInfo},
	{HOOK_HOT_CANDIDATE, 0, "SetScrollPos", (FARPROC)SetScrollPos, (FARPROC *)&pSetScrollPos, (FARPROC)extSetScrollPos},
	{HOOK_HOT_CANDIDATE, 0, "GetScrollPos", (FARPROC)GetScrollPos, (FARPROC *)&pGetScrollPos, (FARPROC)extGetScrollPos},
	{HOOK_HOT_CANDIDATE, 0, "SetScrollRange", (FARPROC)SetScrollRange, (FARPROC *)&pSetScrollRange, (FARPROC)extSetScrollRange},
	{HOOK_HOT_CANDIDATE, 0, "GetScrollRange", (FARPROC)GetScrollRange, (FARPROC *)&pGetScrollRange, (FARPROC)extGetScrollRange},
	{HOOK_IAT_CANDIDATE, 0, "TranslateMessage", (FARPROC)TranslateMessage, (FARPROC *)&pTranslateMessage, (FARPROC)extTranslateMessage}, 
	{HOOK_IAT_CANDIDATE, 0, "IsDialogMessageA", (FARPROC)IsDialogMessageA, (FARPROC *)&pIsDialogMessageA, (FARPROC)extIsDialogMessageA}, 
#endif // TRACESYSCALLS
#ifdef TRACEPOINTS
	{HOOK_IAT_CANDIDATE, 0, "SetRect", (FARPROC)SetRect, (FARPROC *)&pSetRect, (FARPROC)extSetRect},
	{HOOK_IAT_CANDIDATE, 0, "PtInRect", (FARPROC)PtInRect, (FARPROC *)&pPtInRect, (FARPROC)extPtInRect},
	{HOOK_IAT_CANDIDATE, 0, "SetRectEmpty", (FARPROC)SetRectEmpty, (FARPROC *)&pSetRectEmpty, (FARPROC)extSetRectEmpty},
	{HOOK_IAT_CANDIDATE, 0, "OffsetRect", (FARPROC)OffsetRect, (FARPROC *)&pOffsetRect, (FARPROC)extOffsetRect},
	{HOOK_IAT_CANDIDATE, 0, "InflateRect", (FARPROC)InflateRect, (FARPROC *)&pInflateRect, (FARPROC)extInflateRect},
	{HOOK_IAT_CANDIDATE, 0, "IntersectRect", (FARPROC)IntersectRect, (FARPROC *)&pIntersectRect, (FARPROC)extIntersectRect},
	{HOOK_IAT_CANDIDATE, 0, "UnionRect", (FARPROC)UnionRect, (FARPROC *)&pUnionRect, (FARPROC)extUnionRect},
	{HOOK_IAT_CANDIDATE, 0, "SubtractRect", (FARPROC)SubtractRect, (FARPROC *)&pSubtractRect, (FARPROC)extSubtractRect},
	{HOOK_IAT_CANDIDATE, 0, "CopyRect", (FARPROC)CopyRect, (FARPROC *)&pCopyRect, (FARPROC)extCopyRect},
	{HOOK_IAT_CANDIDATE, 0, "IsRectEmpty", (FARPROC)IsRectEmpty, (FARPROC *)&pIsRectEmpty, (FARPROC)extIsRectEmpty},
#endif // TRACEPOINTS
#ifdef TRACEWINDOWS
	{HOOK_IAT_CANDIDATE, 0, "EnableWindow", (FARPROC)EnableWindow, (FARPROC *)&pEnableWindow, (FARPROC)extEnableWindow},
	{HOOK_HOT_CANDIDATE, 0, "GetFocus", (FARPROC)GetFocus, (FARPROC *)&pGetFocus, (FARPROC)extGetFocus},
	{HOOK_HOT_CANDIDATE, 0, "SetFocus", (FARPROC)SetFocus, (FARPROC *)&pSetFocus, (FARPROC)extSetFocus},
#endif // TRACEWINDOWS
#ifdef TRACEMENUS
	{HOOK_IAT_CANDIDATE, 0, "CreateMenu", (FARPROC)CreateMenu, (FARPROC *)&pCreateMenu, (FARPROC)extCreateMenu},
	{HOOK_IAT_CANDIDATE, 0, "CreatePopupMenu", (FARPROC)CreatePopupMenu, (FARPROC *)&pCreatePopupMenu, (FARPROC)extCreatePopupMenu},
	{HOOK_IAT_CANDIDATE, 0, "GetMenuState", (FARPROC)GetMenuState, (FARPROC *)&pGetMenuState, (FARPROC)extGetMenuState},
	{HOOK_IAT_CANDIDATE, 0, "GetMenuItemInfoA", (FARPROC)GetMenuItemInfoA, (FARPROC *)&pGetMenuItemInfoA, (FARPROC)extGetMenuItemInfoA},
	{HOOK_IAT_CANDIDATE, 0, "GetMenuItemInfoW", (FARPROC)GetMenuItemInfoW, (FARPROC *)&pGetMenuItemInfoW, (FARPROC)extGetMenuItemInfoW},
	{HOOK_IAT_CANDIDATE, 0, "SetMenuItemInfoA", (FARPROC)SetMenuItemInfoA, (FARPROC *)&pSetMenuItemInfoA, (FARPROC)extSetMenuItemInfoA},
	{HOOK_IAT_CANDIDATE, 0, "InsertMenuItemA", (FARPROC)InsertMenuItemA, (FARPROC *)&pInsertMenuItemA, (FARPROC)extInsertMenuItemA},
	{HOOK_IAT_CANDIDATE, 0, "InsertMenuItemW", (FARPROC)InsertMenuItemW, (FARPROC *)&pInsertMenuItemW, (FARPROC)extInsertMenuItemW},
	{HOOK_IAT_CANDIDATE, 0, "RemoveMenu", (FARPROC)RemoveMenu, (FARPROC *)&pRemoveMenu, (FARPROC)extRemoveMenu},
	{HOOK_IAT_CANDIDATE, 0, "DeleteMenu", (FARPROC)DeleteMenu, (FARPROC *)&pDeleteMenu, (FARPROC)extDeleteMenu},
	{HOOK_IAT_CANDIDATE, 0, "DestroyMenu", (FARPROC)DestroyMenu, (FARPROC *)&pDestroyMenu, (FARPROC)extDestroyMenu},
	{HOOK_IAT_CANDIDATE, 0, "ModifyMenuA", (FARPROC)ModifyMenuA, (FARPROC *)&pModifyMenuA, (FARPROC)extModifyMenuA},
	{HOOK_IAT_CANDIDATE, 0, "SetMenuItemBitmaps", (FARPROC)SetMenuItemBitmaps, (FARPROC *)&pSetMenuItemBitmaps, (FARPROC)extSetMenuItemBitmaps},
	{HOOK_IAT_CANDIDATE, 0, "AppendMenuW", (FARPROC)AppendMenuW, (FARPROC *)&pAppendMenuW, (FARPROC)extAppendMenuW},
	{HOOK_IAT_CANDIDATE, 0, "InsertMenuW", (FARPROC)InsertMenuW, (FARPROC *)&pInsertMenuW, (FARPROC)extInsertMenuW},
	{HOOK_IAT_CANDIDATE, 0, "GetMenuCheckMarkDimensions", (FARPROC)GetMenuCheckMarkDimensions, (FARPROC *)&pGetMenuCheckMarkDimensions, (FARPROC)extGetMenuCheckMarkDimensions},
#endif // TRACEMENUS
	{HOOK_IAT_CANDIDATE, 0, "LoadMenuA", (FARPROC)LoadMenuA, (FARPROC *)&pLoadMenuA, (FARPROC)extLoadMenuA},
	{HOOK_IAT_CANDIDATE, 0, "LoadMenuW", (FARPROC)LoadMenuW, (FARPROC *)&pLoadMenuW, (FARPROC)extLoadMenuW},
	{HOOK_IAT_CANDIDATE, 0, "LoadMenuIndirectA", (FARPROC)LoadMenuIndirectA, (FARPROC *)&pLoadMenuIndirectA, (FARPROC)extLoadMenuIndirectA},
	{HOOK_IAT_CANDIDATE, 0, "LoadMenuIndirectW", (FARPROC)LoadMenuIndirectW, (FARPROC *)&pLoadMenuIndirectW, (FARPROC)extLoadMenuIndirectW},
	{HOOK_IAT_CANDIDATE, 0, "AppendMenuA", (FARPROC)AppendMenuA, (FARPROC *)&pAppendMenuA, (FARPROC)extAppendMenuA},
	{HOOK_IAT_CANDIDATE, 0, "InsertMenuA", (FARPROC)InsertMenuA, (FARPROC *)&pInsertMenuA, (FARPROC)extInsertMenuA},
	{HOOK_IAT_CANDIDATE, 0, "GetMenu", (FARPROC)GetMenu, (FARPROC *)&pGetMenu, (FARPROC)extGetMenu},
	{HOOK_IAT_CANDIDATE, 0, "SetMenu", (FARPROC)SetMenu, (FARPROC *)&pSetMenu, (FARPROC)extSetMenu},
	{HOOK_IAT_CANDIDATE, 0, "GetAsyncKeyState", (FARPROC)GetAsyncKeyState, (FARPROC *)&pGetAsyncKeyState, (FARPROC)extGetAsyncKeyState},
	{HOOK_IAT_CANDIDATE, 0, "GetDialogBaseUnits", (FARPROC)GetDialogBaseUnits, (FARPROC *)&pGetDialogBaseUnits, (FARPROC)extGetDialogBaseUnits},
	{HOOK_IAT_CANDIDATE, 0, "GetKeyboardType", (FARPROC)GetKeyboardType, (FARPROC *)&pGetKeyboardType, (FARPROC)extGetKeyboardType},
	{HOOK_IAT_CANDIDATE, 0, "GetKeyNameTextA", (FARPROC)GetKeyNameTextA, (FARPROC *)&pGetKeyNameTextA, (FARPROC)extGetKeyNameTextA},
	{HOOK_IAT_CANDIDATE, 0, "GetKeyNameTextW", (FARPROC)GetKeyNameTextW, (FARPROC *)&pGetKeyNameTextW, (FARPROC)extGetKeyNameTextW},

	{HOOK_IAT_CANDIDATE, 0, "EnableMenuItem", (FARPROC)EnableMenuItem, (FARPROC *)&pEnableMenuItem, (FARPROC)extEnableMenuItem},
	
	{HOOK_IAT_CANDIDATE, 0, "CreateMDIWindowA", (FARPROC)CreateMDIWindowA, (FARPROC *)&pCreateMDIWindowA, (FARPROC)extCreateMDIWindowA},
	{HOOK_IAT_CANDIDATE, 0, "CreateMDIWindowW", (FARPROC)CreateMDIWindowW, (FARPROC *)&pCreateMDIWindowW, (FARPROC)extCreateMDIWindowW},
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "TrackMouseEvent", (FARPROC)TrackMouseEvent, (FARPROC *)&pTrackMouseEvent, (FARPROC)extTrackMouseEvent},
	{HOOK_IAT_CANDIDATE, 0, "EndDialog", (FARPROC)EndDialog, (FARPROC *)&pEndDialog, (FARPROC)extEndDialog},
#endif
	{HOOK_IAT_CANDIDATE, 0, "WinHelpA", (FARPROC)WinHelpA, (FARPROC *)&pWinHelpA, (FARPROC)extWinHelpA},
	{HOOK_IAT_CANDIDATE, 0, "WinHelpW", (FARPROC)WinHelpW, (FARPROC *)&pWinHelpW, (FARPROC)extWinHelpW},
	#ifdef TRACEPROPS
	{HOOK_IAT_CANDIDATE, 0, "GetPropA", (FARPROC)GetPropA, (FARPROC *)&pGetPropA, (FARPROC)extGetPropA},
	{HOOK_IAT_CANDIDATE, 0, "GetPropW", (FARPROC)GetPropW, (FARPROC *)&pGetPropW, (FARPROC)extGetPropW},
	{HOOK_IAT_CANDIDATE, 0, "SetPropA", (FARPROC)SetPropA, (FARPROC *)&pSetPropA, (FARPROC)extSetPropA},
	{HOOK_IAT_CANDIDATE, 0, "SetPropW", (FARPROC)SetPropW, (FARPROC *)&pSetPropW, (FARPROC)extSetPropW},
#endif // TRACEPROPS
	{HOOK_IAT_CANDIDATE, 0, "WINNLSEnableIME", (FARPROC)NULL, (FARPROC *)&pWINNLSEnableIME, (FARPROC)extWINNLSEnableIME},
	{HOOK_IAT_CANDIDATE, 0, "SendMessageA", (FARPROC)SendMessageA, (FARPROC *)&pSendMessageA, (FARPROC)extSendMessageA}, 
	{HOOK_IAT_CANDIDATE, 0, "FindWindowA", (FARPROC)FindWindowA, (FARPROC *)&pFindWindowA, (FARPROC)extFindWindowA}, 
	{HOOK_IAT_CANDIDATE, 0, "FindWindowW", (FARPROC)FindWindowW, (FARPROC *)&pFindWindowW, (FARPROC)extFindWindowW}, 
	{HOOK_IAT_CANDIDATE, 0, "EnumChildWindows", (FARPROC)EnumChildWindows, (FARPROC *)&pEnumChildWindows, (FARPROC)extEnumChildWindows},

	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type WineHooks[]={
	{HOOK_HOT_REQUIRED,  0, "UserRealizePalette", (FARPROC)NULL, (FARPROC *)&pUserRealizePalette, (FARPROC)extUserRealizePalette},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type LayeredHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "UpdateLayeredWindow", (FARPROC)UpdateLayeredWindow, (FARPROC *)&pUpdateLayeredWindow, (FARPROC)extUpdateLayeredWindow},
	{HOOK_HOT_CANDIDATE, 0, "SetLayeredWindowAttributes", (FARPROC)SetLayeredWindowAttributes, (FARPROC *)&pSetLayeredWindowAttributes, (FARPROC)extSetLayeredWindowAttributes},
	// v2.06.12: recovered support for WinXP where UpdateLayeredWindowIndirect is not supported
	{HOOK_HOT_CANDIDATE, 0, "UpdateLayeredWindowIndirect", (FARPROC)NULL, (FARPROC *)&pUpdateLayeredWindowIndirect, (FARPROC)extUpdateLayeredWindowIndirect},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type RawInputHooks[]={
	{HOOK_HOT_REQUIRED,  0, "GetRawInputData", (FARPROC)GetRawInputData, (FARPROC *)&pGetRawInputData, (FARPROC)extGetRawInputData},
	{HOOK_HOT_REQUIRED,  0, "GetRawInputBuffer", (FARPROC)GetRawInputBuffer, (FARPROC *)&pGetRawInputBuffer, (FARPROC)extGetRawInputBuffer},
	{HOOK_HOT_REQUIRED,  0, "RegisterRawInputDevices", (FARPROC)RegisterRawInputDevices, (FARPROC *)&pRegisterRawInputDevices, (FARPROC)extRegisterRawInputDevices},
	{HOOK_HOT_REQUIRED,  0, "GetRawInputDeviceInfoA", (FARPROC)GetRawInputDeviceInfoA, (FARPROC *)&pGetRawInputDeviceInfoA, (FARPROC)extGetRawInputDeviceInfoA},
	{HOOK_HOT_REQUIRED,  0, "GetRawInputDeviceInfoW", (FARPROC)GetRawInputDeviceInfoW, (FARPROC *)&pGetRawInputDeviceInfoW, (FARPROC)extGetRawInputDeviceInfoW},
	{HOOK_HOT_REQUIRED,  0, "GetRawInputDeviceList", (FARPROC)GetRawInputDeviceList, (FARPROC *)&pGetRawInputDeviceList, (FARPROC)extGetRawInputDeviceList},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type RemapHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "ScreenToClient", (FARPROC)ScreenToClient, (FARPROC *)&pScreenToClient, (FARPROC)extScreenToClient},
	{HOOK_HOT_CANDIDATE, 0, "ClientToScreen", (FARPROC)ClientToScreen, (FARPROC *)&pClientToScreen, (FARPROC)extClientToScreen},
	{HOOK_HOT_CANDIDATE, 0, "GetClientRect", (FARPROC)GetClientRect, (FARPROC *)&pGetClientRect, (FARPROC)extGetClientRect},
	{HOOK_HOT_CANDIDATE, 0, "GetWindowRect", (FARPROC)GetWindowRect, (FARPROC *)&pGetWindowRect, (FARPROC)extGetWindowRect},
	{HOOK_HOT_CANDIDATE, 0, "MapWindowPoints", (FARPROC)MapWindowPoints, (FARPROC *)&pMapWindowPoints, (FARPROC)extMapWindowPoints},
	{HOOK_HOT_CANDIDATE, 0, "GetUpdateRgn", (FARPROC)GetUpdateRgn, (FARPROC *)&pGetUpdateRgn, (FARPROC)extGetUpdateRgn},
	{HOOK_IAT_CANDIDATE, 0, "GetUpdateRect", (FARPROC)GetUpdateRect, (FARPROC *)&pGetUpdateRect, (FARPROC)extGetUpdateRect},
	{HOOK_IAT_CANDIDATE, 0, "RedrawWindow", (FARPROC)RedrawWindow, (FARPROC *)&pRedrawWindow, (FARPROC)extRedrawWindow},
	{HOOK_HOT_CANDIDATE, 0, "InvalidateRect", (FARPROC)InvalidateRect, (FARPROC *)&pInvalidateRect, (FARPROC)extInvalidateRect},
	{HOOK_HOT_CANDIDATE, 0, "SetWindowRgn", (FARPROC)SetWindowRgn, (FARPROC *)&pSetWindowRgn, (FARPROC)extSetWindowRgn},
	{HOOK_HOT_CANDIDATE, 0, "GetWindowRgn", (FARPROC)GetWindowRgn, (FARPROC *)&pGetWindowRgn, (FARPROC)extGetWindowRgn},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type SyscallHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "FrameRect", (FARPROC)FrameRect, (FARPROC *)&pFrameRect, (FARPROC)extFrameRect}, 
	{HOOK_HOT_CANDIDATE, 0, "InvalidateRgn", (FARPROC)InvalidateRgn, (FARPROC *)&pInvalidateRgn, (FARPROC)extInvalidateRgn},
	//{HOOK_IAT_CANDIDATE, 0, "TabbedTextOutA", (FARPROC)TabbedTextOutA, (FARPROC *)&pTabbedTextOutA, (FARPROC)extTabbedTextOutA},
	//{HOOK_IAT_CANDIDATE, 0, "TabbedTextOutW", (FARPROC)TabbedTextOutW, (FARPROC *)&pTabbedTextOutW, (FARPROC)extTabbedTextOutW},
	//{HOOK_IAT_CANDIDATE, 0, "DrawTextA", (FARPROC)DrawTextA, (FARPROC *)&pDrawTextA, (FARPROC)extDrawTextA},
	//{HOOK_IAT_CANDIDATE, 0, "DrawTextExA", (FARPROC)DrawTextExA, (FARPROC *)&pDrawTextExA, (FARPROC)extDrawTextExA},
	//{HOOK_IAT_CANDIDATE, 0, "DrawTextW", (FARPROC)DrawTextW, (FARPROC *)&pDrawTextW, (FARPROC)extDrawTextW},
	//{HOOK_IAT_CANDIDATE, 0, "DrawTextExW", (FARPROC)DrawTextExW, (FARPROC *)&pDrawTextExW, (FARPROC)extDrawTextExW},
	{HOOK_HOT_CANDIDATE, 0, "InvertRect", (FARPROC)NULL, (FARPROC *)&pInvertRect, (FARPROC)extInvertRect},
	{HOOK_HOT_CANDIDATE, 0, "DrawIcon", (FARPROC)NULL, (FARPROC *)&pDrawIcon, (FARPROC)extDrawIcon},
	{HOOK_IAT_CANDIDATE, 0, "DrawIconEx", (FARPROC)NULL, (FARPROC *)&pDrawIconEx, (FARPROC)extDrawIconEx},
	{HOOK_HOT_CANDIDATE, 0, "DrawCaption", (FARPROC)NULL, (FARPROC *)&pDrawCaption, (FARPROC)extDrawCaption},
	{HOOK_HOT_CANDIDATE, 0, "DrawEdge", (FARPROC)NULL, (FARPROC *)&pDrawEdge, (FARPROC)extDrawEdge},
	{HOOK_HOT_CANDIDATE, 0, "DrawFocusRect", (FARPROC)NULL, (FARPROC *)&pDrawFocusRect, (FARPROC)extDrawFocusRect},
	{HOOK_HOT_CANDIDATE, 0, "DrawFrameControl", (FARPROC)NULL, (FARPROC *)&pDrawFrameControl, (FARPROC)extDrawFrameControl},
	//TODO {HOOK_HOT_CANDIDATE, 0, "DrawStateA", (FARPROC)NULL, (FARPROC *)&pDrawStateA, (FARPROC)extDrawStateA},
	//TODO {HOOK_HOT_CANDIDATE, 0, "DrawStateW", (FARPROC)NULL, (FARPROC *)&pDrawStateW, (FARPROC)extDrawStateW},
	//TODO {HOOK_HOT_CANDIDATE, 0, "GrayStringA", (FARPROC)NULL, (FARPROC *)&pGrayStringA, (FARPROC)extGrayStringA},
	//TODO {HOOK_HOT_CANDIDATE, 0, "GrayStringW", (FARPROC)NULL, (FARPROC *)&pGrayStringW, (FARPROC)extGrayStringW},
	//TODO {HOOK_HOT_CANDIDATE, 0, "PaintDesktop", (FARPROC)NULL, (FARPROC *)&pPaintDesktop, (FARPROC)extPaintDesktop},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type ScaledHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "ValidateRect", (FARPROC)ValidateRect, (FARPROC *)&pValidateRect, (FARPROC)extValidateRect},
	{HOOK_HOT_CANDIDATE, 0, "ValidateRgn", (FARPROC)ValidateRgn, (FARPROC *)&pValidateRgn, (FARPROC)extValidateRgn},
	{HOOK_IAT_CANDIDATE, 0, "ScrollWindow", (FARPROC)ScrollWindow, (FARPROC *)&pScrollWindow, (FARPROC)extScrollWindow},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type MouseHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetCursorPos", (FARPROC)GetCursorPos, (FARPROC *)&pGetCursorPos, (FARPROC)extGetCursorPos},
	{HOOK_HOT_CANDIDATE, 0, "SetCursorPos", (FARPROC)SetCursorPos, (FARPROC *)&pSetCursorPos, (FARPROC)extSetCursorPos},
	{HOOK_IAT_CANDIDATE, 0, "GetCursorInfo", (FARPROC)GetCursorInfo, (FARPROC *)&pGetCursorInfo, (FARPROC)extGetCursorInfo},
	{HOOK_IAT_CANDIDATE, 0, "SendMessageW", (FARPROC)SendMessageW, (FARPROC *)&pSendMessageW, (FARPROC)extSendMessageW}, 
	{HOOK_HOT_REQUIRED,  0, "mouse_event", (FARPROC)mouse_event, (FARPROC *)&pmouse_event, (FARPROC)extmouse_event}, 
	{HOOK_IAT_CANDIDATE, 0, "SetPhysicalCursorPos", NULL, (FARPROC *)&pSetPhysicalCursorPos, (FARPROC)extSetPhysicalCursorPos}, // ???
	{HOOK_HOT_CANDIDATE, 0, "SendInput", (FARPROC)SendInput, (FARPROC *)&pSendInput, (FARPROC)extSendInput},
	{HOOK_HOT_CANDIDATE, 0, "GetMessagePos", (FARPROC)GetMessagePos, (FARPROC *)&pGetMessagePos, (FARPROC)extGetMessagePos}, 
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type TimeHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "SetTimer", (FARPROC)SetTimer, (FARPROC *)&pSetTimer, (FARPROC)extSetTimer},
	{HOOK_IAT_CANDIDATE, 0, "KillTimer", (FARPROC)KillTimer, (FARPROC *)&pKillTimer, (FARPROC)extKillTimer},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type DesktopHooks[]={ // currently unused, needed for X-Files
	{HOOK_IAT_CANDIDATE, 0, "CreateDesktopA", (FARPROC)CreateDesktopA, (FARPROC *)&pCreateDesktopA, (FARPROC)extCreateDesktopA},
	{HOOK_IAT_CANDIDATE, 0, "SwitchDesktop", (FARPROC)SwitchDesktop, (FARPROC *)&pSwitchDesktop, (FARPROC)extSwitchDesktop},
	{HOOK_IAT_CANDIDATE, 0, "OpenDesktopA", (FARPROC)OpenDesktopA, (FARPROC *)&pOpenDesktop, (FARPROC)extOpenDesktop},
	{HOOK_IAT_CANDIDATE, 0, "CloseDesktop", (FARPROC)CloseDesktop, (FARPROC *)&pCloseDesktop, (FARPROC)extCloseDesktop},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type MsgLoopHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "PeekMessageA", (FARPROC)PeekMessageA, (FARPROC *)&pPeekMessageA, (FARPROC)extPeekMessageA},
	{HOOK_HOT_CANDIDATE, 0, "PeekMessageW", (FARPROC)PeekMessageW, (FARPROC *)&pPeekMessageW, (FARPROC)extPeekMessageW},
	{HOOK_HOT_CANDIDATE, 0, "GetMessageA", (FARPROC)GetMessageA, (FARPROC *)&pGetMessageA, (FARPROC)extGetMessageA},
	{HOOK_HOT_CANDIDATE, 0, "GetMessageW", (FARPROC)GetMessageW, (FARPROC *)&pGetMessageW, (FARPROC)extGetMessageW},
	{HOOK_IAT_CANDIDATE, 0, "PostMessageA", (FARPROC)PostMessageA, (FARPROC *)&pPostMessageA, (FARPROC)extPostMessageA},
	{HOOK_IAT_CANDIDATE, 0, "PostMessageW", (FARPROC)PostMessageW, (FARPROC *)&pPostMessageW, (FARPROC)extPostMessageW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type MessageBoxHooks[]={
	{HOOK_HOT_REQUIRED, 0, "MessageBoxTimeoutA", (FARPROC)NULL, (FARPROC *)&pMessageBoxTimeoutA, (FARPROC)extMessageBoxTimeoutA},
	{HOOK_HOT_REQUIRED, 0, "MessageBoxTimeoutW", (FARPROC)NULL, (FARPROC *)&pMessageBoxTimeoutW, (FARPROC)extMessageBoxTimeoutW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type FakeIOHooks[]={
	//{HOOK_IAT_CANDIDATE, 0, "SendMessageW", (FARPROC)SendMessageW, (FARPROC *)&pSendMessageW, (FARPROC)extSendMessageW}, 
	{HOOK_IAT_CANDIDATE, 0, "LoadImageA", (FARPROC)LoadImageA, (FARPROC *)&pLoadImageA, (FARPROC)extLoadImageA}, 
	{HOOK_IAT_CANDIDATE, 0, "LoadImageW", (FARPROC)LoadImageW, (FARPROC *)&pLoadImageW, (FARPROC)extLoadImageW}, 
	{HOOK_IAT_CANDIDATE, 0, "LoadCursorFromFileA", (FARPROC)LoadCursorFromFileA, (FARPROC *)&pLoadCursorFromFileA, (FARPROC)extLoadCursorFromFileA}, 
	{HOOK_IAT_CANDIDATE, 0, "LoadCursorFromFileW", (FARPROC)LoadCursorFromFileW, (FARPROC *)&pLoadCursorFromFileW, (FARPROC)extLoadCursorFromFileW}, 
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type NlsHooks[]={
	{HOOK_HOT_REQUIRED,  0, "CharPrevA", (FARPROC)CharPrevA, (FARPROC *)&pCharPrevA, (FARPROC)extCharPrevA}, 
	{HOOK_HOT_REQUIRED,  0, "CharNextA", (FARPROC)CharNextA, (FARPROC *)&pCharNextA, (FARPROC)extCharNextA}, //
	{HOOK_HOT_REQUIRED,  0, "SetWindowTextA", (FARPROC)SetWindowTextA, (FARPROC *)&pSetWindowTextA, (FARPROC)extSetWindowTextA}, 
	{HOOK_HOT_REQUIRED,  0, "SetWindowTextW", (FARPROC)SetWindowTextW, (FARPROC *)&pSetWindowTextW, (FARPROC)extSetWindowTextW}, 
	{HOOK_HOT_REQUIRED,  0, "GetWindowTextA", (FARPROC)GetWindowTextA, (FARPROC *)&pGetWindowTextA, (FARPROC)extGetWindowTextA}, 
//	{HOOK_HOT_REQUIRED,  0, "DefFrameProcA", (FARPROC)DefFrameProcA, (FARPROC *)&pDefFrameProcA, (FARPROC)extDefFrameProcA}, 
//	{HOOK_HOT_REQUIRED,  0, "DefMDIChildProcA", (FARPROC)DefMDIChildProcA, (FARPROC *)NULL, (FARPROC)extDefMDIChildProcA}, 
//	{HOOK_HOT_REQUIRED,  0, "DefDlgProcA", (FARPROC)DefDlgProcA, (FARPROC *)NULL, (FARPROC)extDefDlgProcA}, 
	{HOOK_HOT_REQUIRED,  0, "GetClassNameA", (FARPROC)GetClassNameA, (FARPROC *)&pGetClassNameA, (FARPROC)extGetClassNameA}, 
	{HOOK_HOT_REQUIRED,  0, "GetClassInfoA", (FARPROC)GetClassInfoA, (FARPROC *)&pGetClassInfoA, (FARPROC)extGetClassInfoA}, 
	{HOOK_HOT_REQUIRED,  0, "UnregisterClassA", (FARPROC)UnregisterClassA, (FARPROC *)&pUnregisterClassA, (FARPROC)extUnregisterClassA}, 
	// tracing only ....
	{HOOK_HOT_REQUIRED,  0, "OemToCharA", (FARPROC)OemToCharA, (FARPROC *)&pOemToCharA, (FARPROC)extOemToCharA}, 
	{HOOK_HOT_REQUIRED,  0, "CharToOemA", (FARPROC)CharToOemA, (FARPROC *)&pCharToOemA, (FARPROC)extCharToOemA}, 
	{HOOK_HOT_REQUIRED,  0, "OemToCharBuffA", (FARPROC)OemToCharBuffA, (FARPROC *)&pOemToCharBuffA, (FARPROC)extOemToCharBuffA}, 
	{HOOK_HOT_REQUIRED,  0, "CharToOemBuffA", (FARPROC)CharToOemBuffA, (FARPROC *)&pCharToOemBuffA, (FARPROC)extCharToOemBuffA}, 
	{HOOK_HOT_REQUIRED,  0, "MessageBoxTimeoutA", (FARPROC)NULL, (FARPROC *)&pMessageBoxTimeoutA, (FARPROC)extMessageBoxTimeoutA},
	{HOOK_HOT_REQUIRED,  0, "MessageBoxA", (FARPROC)MessageBoxA, (FARPROC *)&pMessageBoxA, (FARPROC)extMessageBoxA},
	{HOOK_IAT_CANDIDATE, 0, "SendMessageW", (FARPROC)SendMessageW, (FARPROC *)&pSendMessageW, (FARPROC)extSendMessageW}, 
	//{HOOK_IAT_CANDIDATE, 0, "DrawTextA", (FARPROC)DrawTextA, (FARPROC *)&pDrawTextA, (FARPROC)extDrawTextA},
	//{HOOK_IAT_CANDIDATE, 0, "DrawTextExA", (FARPROC)DrawTextExA, (FARPROC *)&pDrawTextExA, (FARPROC)extDrawTextExA},
	{HOOK_IAT_CANDIDATE, 0, "SetMenuItemInfoA", (FARPROC)SetMenuItemInfoA, (FARPROC *)&pSetMenuItemInfoA, (FARPROC)extSetMenuItemInfoA},
	{HOOK_IAT_CANDIDATE, 0, "InsertMenuItemA", (FARPROC)InsertMenuItemA, (FARPROC *)&pInsertMenuItemA, (FARPROC)extInsertMenuItemA},

	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static char *libname = "user32.dll";

void HookUser32(HMODULE hModule)
{

	HookLibraryEx(hModule, Hooks, libname);
	HookLibraryEx(hModule, MsgLoopHooks, libname);

	if (dxw.GDIEmulationMode != GDIMODE_NONE) HookLibraryEx(hModule, SyscallHooks, libname);
	if (dxw.dwFlags2 & GDISTRETCHED)	HookLibraryEx(hModule, ScaledHooks, libname);
	if (dxw.dwFlags1 & CLIENTREMAPPING) HookLibraryEx(hModule, RemapHooks, libname);
	if ((dxw.dwFlags1 & (MODIFYMOUSE|SLOWDOWN|TRIMMOUSEPOSITION)) || (dxw.dwFlags2 & KEEPCURSORFIXED)) HookLibraryEx(hModule, MouseHooks, libname);
	if (dxw.dwFlags2 & TIMESTRETCH) HookLibraryEx(hModule, TimeHooks, libname);
	if (dxw.dwFlags9 & NODIALOGS) HookLibraryEx(hModule, MessageBoxHooks, libname);
	if (dxw.dwFlags10 & FIXMOUSERAWINPUT) HookLibraryEx(hModule, RawInputHooks, libname);
	if ((dxw.dwFlags11 & CUSTOMLOCALE) || (dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE))) HookLibraryEx(hModule, FakeIOHooks, libname);
	if (dxw.dwFlags11 & CUSTOMLOCALE) HookLibraryEx(hModule, NlsHooks, libname);
	if (dxw.dwFlags19 & DISABLELAYEREDWIN) HookLibraryEx(hModule, LayeredHooks, libname);
	if (dxw.dwFlags19 & ISWIN16EXECUTABLE) HookLibraryEx(hModule, WineHooks, libname);

	IsChangeDisplaySettingsHotPatched = IsHotPatchedEx(Hooks, "ChangeDisplaySettingsExA") || IsHotPatchedEx(Hooks, "ChangeDisplaySettingsExW");
	return;
}

void HookUser32Init()
{
	HookLibInitEx(Hooks);
	HookLibInitEx(RawInputHooks);
	HookLibInitEx(RemapHooks);
	HookLibInitEx(SyscallHooks);
	HookLibInitEx(ScaledHooks);
	HookLibInitEx(MouseHooks);
	HookLibInitEx(TimeHooks);
	HookLibInitEx(DesktopHooks);
	HookLibInitEx(MsgLoopHooks);
	HookLibInitEx(MessageBoxHooks);
	HookLibInitEx(FakeIOHooks);
	HookLibInitEx(NlsHooks);
}

FARPROC Remap_user32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	if (addr=RemapLibraryEx(proc, hModule, MsgLoopHooks)) return addr;

	if (dxw.dwFlags1 & CLIENTREMAPPING) 
		if (addr=RemapLibraryEx(proc, hModule, RemapHooks)) return addr;
	if (dxw.GDIEmulationMode != GDIMODE_NONE) 
		if(addr=RemapLibraryEx(proc, hModule, SyscallHooks)) return addr;
	if (dxw.dwFlags2 & GDISTRETCHED) 
		if (addr=RemapLibraryEx(proc, hModule, ScaledHooks)) return addr;  
	if ((dxw.dwFlags1 & (MODIFYMOUSE|SLOWDOWN|TRIMMOUSEPOSITION)) || (dxw.dwFlags2 & KEEPCURSORFIXED)) 
		if (addr=RemapLibraryEx(proc, hModule, MouseHooks)) return addr;
	if((dxw.dwFlags2 & TIMESTRETCH) && (dxw.dwFlags4 & STRETCHTIMERS)) 
		if (addr=RemapLibraryEx(proc, hModule, TimeHooks)) return addr;
	if(dxw.dwFlags10 & FIXMOUSERAWINPUT) 
		if (addr=RemapLibraryEx(proc, hModule, RawInputHooks)) return addr;
	if ((dxw.dwFlags11 & CUSTOMLOCALE) || (dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE))) 
		if (addr=RemapLibraryEx(proc, hModule, FakeIOHooks)) return addr;
	if (dxw.dwFlags11 & CUSTOMLOCALE) 
		if (addr=RemapLibraryEx(proc, hModule, NlsHooks)) return addr;
	if (dxw.dwFlags19 & DISABLELAYEREDWIN) 
		if (addr=RemapLibraryEx(proc, hModule, LayeredHooks)) return addr;

	return NULL;
}

/* ------------------------------------------------------------------------------ */
// auxiliary (static) functions
/* ------------------------------------------------------------------------------ */

static void Stopper(char *s, int line)
{
	char sMsg[81];
	sprintf(sMsg, "break: \"%s\"", s);
	MessageBox(0, sMsg, "break", MB_OK | MB_ICONEXCLAMATION);
}

//#define STOPPER_TEST // comment out to eliminate
#ifdef STOPPER_TEST
#define STOPPER(s) Stopper(s, __LINE__)
#else
#define STOPPER(s)
#endif

#ifndef DXW_NOTRACES
static LPCSTR sTemplateNameA(LPCSTR tn)
{
	static char sBuf[20+1];
	if((DWORD)tn >> 16)
		return tn;
	else {
		sprintf(sBuf, "ID:(%#x)", ((DWORD)tn & 0x0000FFFF));
		return sBuf;
	}
}

static LPCWSTR sTemplateNameW(LPCWSTR tn)
{
	static WCHAR sBuf[20+1];
	if((DWORD)tn >> 16)
		return tn;
	else {
		swprintf(sBuf, L"ID:(%#x)", ((DWORD)tn & 0x0000FFFF));
		return sBuf;
	}
}
#endif // DXW_NOTRACES

// --------------------------------------------------------------------------
//
// globals, externs, static functions...
//
// --------------------------------------------------------------------------

// PrimHDC: DC handle of the selected DirectDraw primary surface. NULL when invalid.
HDC PrimHDC=NULL;

LPRECT lpClipRegion=NULL;
RECT ClipRegion;
int LastCurPosX, LastCurPosY;

extern GetDC_Type pGetDC;
extern ReleaseDC_Type pReleaseDC1;
extern HRESULT WINAPI sBlt(int, Blt_Type, char *, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX, BOOL);
char *sChangeDisplaySettingsRet(LONG);

LONG WINAPI intChangeDisplaySettings(ApiArg, BOOL WideChar, void *lpDevMode, DWORD dwflags)
{
	LONG ret; 
	typedef enum {
		BYPASS = 0, // no logic
		COLOREMU, 
		WINDIRECT,
		WINEMU 
	} changeModeType;
	DWORD dmFields, dmBitsPerPel, dmPelsWidth, dmPelsHeight;

	changeModeType changeMode = BYPASS;	
	if(dxw.isColorEmulatedMode) changeMode = COLOREMU;
	else if(dxw.Windowize) changeMode = dxw.IsEmulated ? WINEMU : WINDIRECT;
	OutTrace("%s: changeMode=%d\n", ApiRef, changeMode);

	// trim harmful flags
	// v2.04.37: do not change primary device settings while in Windowed mode. Fixes @#@"Fastlane Pinball"
	// v2.04.90: suppress CDS_RESET flag, that can force a video mode change also without CDS_FULLSCREEN
	// specification. Fixes @#@"LEGO Racers" between intro movies and game main menu panel.
	dwflags &= ~(CDS_GLOBAL | CDS_RESET | CDS_SET_PRIMARY);

	DEVMODEW *lpDevModeW = (DEVMODEW *)lpDevMode;
	DEVMODEA *lpDevModeA = (DEVMODEA *)lpDevMode;
	if(lpDevMode){
		if(WideChar){
			// v2.06.11: safety fix to avoid crash just in case. Needed for "Air Warrior III"
			if(lpDevModeW->dmSize > sizeof(DEVMODEW)) {
				OutErrorGDI("%s: WARNING lpDevMode->dmSize=%d DEVMODEW=%d\n", ApiRef, lpDevModeW->dmSize, sizeof(DEVMODEW));
				lpDevModeW->dmSize = sizeof(DEVMODEW);
			}
			dmFields=lpDevModeW->dmFields;
			dmPelsWidth=lpDevModeW->dmPelsWidth;
			dmPelsHeight=lpDevModeW->dmPelsHeight;
			dmBitsPerPel=lpDevModeW->dmBitsPerPel;
		}
		else{
			if(lpDevModeA->dmSize > sizeof(DEVMODEA)) {
				OutErrorGDI("%s: WARNING lpDevMode->dmSize=%d DEVMODEA=%d\n", ApiRef, lpDevModeA->dmSize, sizeof(DEVMODEA));
				lpDevModeA->dmSize = sizeof(DEVMODEA);
			}
			dmFields=lpDevModeA->dmFields;
			dmPelsWidth=lpDevModeA->dmPelsWidth;
			dmPelsHeight=lpDevModeA->dmPelsHeight;
			dmBitsPerPel=lpDevModeA->dmBitsPerPel;
		}
	}
	else{
		// v2.06.13 fix: WinQuake resets the screen mode by leaving dwflags to CDS_FULLSCREEN
		if((dwflags & ~CDS_FULLSCREEN) == 0){
			// fake default registry settings
			if(changeMode == COLOREMU){
				DEVMODEA DevMode;
				(*pEnumDisplaySettingsA)(NULL, ENUM_REGISTRY_SETTINGS, &DevMode);
				dmPelsWidth=DevMode.dmPelsWidth;
				dmPelsHeight=DevMode.dmPelsHeight;
				dmBitsPerPel=DevMode.dmBitsPerPel;
				OutTraceGDI("%s: default mode for COLOREMU size=(%dx%d) bpp=%d\n", ApiRef, dmPelsWidth, dmPelsHeight, dmBitsPerPel);
			}
			else {
				dmFields = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL;
				dmPelsWidth=dxw.dwDefaultScreenWidth;
				dmPelsHeight=dxw.dwDefaultScreenHeight;
				dmBitsPerPel=dxw.dwDefaultColorDepth;
				OutTraceGDI("%s: default mode size=(%dx%d) bpp=%d\n", ApiRef, dmPelsWidth, dmPelsHeight, dmBitsPerPel);
			}
		}
	}

	ret = DISP_CHANGE_SUCCESSFUL; // optimistic initialization

	// in any case, do not update the registry
	if(dwflags & CDS_UPDATEREGISTRY){
		dxw.dwDefaultScreenWidth = dmPelsWidth;
		dxw.dwDefaultScreenHeight = dmPelsHeight;
		dxw.dwDefaultColorDepth = dmBitsPerPel;
		OutTraceGDI("%s: VIRTUAL REGISTRY UPDATE size=(%dx%d) BPP=%d\n", ApiRef, dmPelsWidth, dmPelsHeight, dmBitsPerPel);
		return DISP_CHANGE_SUCCESSFUL;
	}

	// unless the BYPASS or COLOREMU case, test is always successful
	// otherwise it must reflect the actual condition
	if(dwflags & CDS_TEST){
		if((changeMode != BYPASS) && (changeMode != COLOREMU)){
			OutTraceGDI("%s: BYPASS TEST size=(%dx%d) BPP=%d\n", ApiRef, dmPelsWidth, dmPelsHeight, dmBitsPerPel);
			return DISP_CHANGE_SUCCESSFUL;
		}
	}

	// v2.02.32: reset the emulated DC used in GDIEMULATEDC mode
	dxw.ResetEmulatedDC();

	//if(lpDevMode && (dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) && !(dwflags & CDS_TEST) && !(changeMode == COLOREMU)){
	if(lpDevMode && (dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) && !(dwflags & CDS_TEST)){
		BOOL bGoFullscreen;

		// v2.02.31: when main win is bigger that expected resolution, you're in windowed fullscreen mode
		// v2.04.71: if main window is not defined yet, go FULLSCREEN in any case
		if(dxw.GethWnd()){
			RECT client;
			(*pGetClientRect)(dxw.GethWnd(), &client);
			OutTraceGDI("%s: current hWnd=%#x size=(%d,%d)\n", ApiRef, dxw.GethWnd(), client.right, client.bottom);
			bGoFullscreen = ((client.right>=(LONG)dmPelsWidth) && (client.bottom>=(LONG)dmPelsHeight));
		} else {
			bGoFullscreen = TRUE;
		}

		if(bGoFullscreen) {
			OutTraceGDI("%s: entering FULLSCREEN mode\n", ApiRef);
			dxw.SetFullScreen(TRUE);
		} else {
			OutTraceGDI("%s: current mode: %s\n", ApiRef, dxw.IsFullScreen() ? "FULLSCREEN" : "WINDOWED");
		}
	}

	DEVMODEA DevMode;
	if(dxw.ActualPixelFormat.dwRGBBitCount == 0){
		if((*pEnumDisplaySettingsA)(NULL, ENUM_CURRENT_SETTINGS, &DevMode)) {
			dxw.ActualPixelFormat.dwRGBBitCount = DevMode.dmBitsPerPel;
		}
	}

	switch(changeMode){
		case BYPASS:
			if(WideChar)
				ret = (*pChangeDisplaySettingsExW)(NULL, lpDevModeW, NULL, dwflags, NULL);
			else
				ret = (*pChangeDisplaySettingsExA)(NULL, lpDevModeA, NULL, dwflags, NULL);
			break;

		case COLOREMU:
			// v2.06.13 fix: WinQuake resets the screen mode by leaving dwflags to CDS_FULLSCREEN
			if((lpDevMode == NULL) && ((dwflags & ~CDS_FULLSCREEN) == 0)){
				OutTraceGDI("%s: returning to default mode\n", ApiRef);
				ret = (*pChangeDisplaySettingsA)(NULL, 0);
				break;
			}
			// v2.06.05 fix: in fullscreen emulated-color mode don't change the pixel format. 
			// @#@ "Midtwn Madness" (1999)
			DevMode.dmSize = sizeof(DEVMODEA);
			dmBitsPerPel = dxw.ActualPixelFormat.dwRGBBitCount;
			DevMode.dmFields = dmFields | DM_BITSPERPEL;
			DevMode.dmBitsPerPel = dmBitsPerPel;
			DevMode.dmPelsWidth = dmPelsWidth;
			DevMode.dmPelsHeight = dmPelsHeight;
			// fix: @#@"Daikatana" crashes when using pChangeDisplaySettingsExA
			// ret = (*pChangeDisplaySettingsExA)(NULL, &DevMode, NULL, dwflags, NULL);
			ret = (*pChangeDisplaySettingsA)(&DevMode, dwflags);
			if(!(dwflags & CDS_TEST)){
				if((*pEnumDisplaySettingsA)(NULL, ENUM_CURRENT_SETTINGS, &DevMode)) {
					dxw.SetScreenSize(DevMode.dmPelsWidth, DevMode.dmPelsHeight);
					GetHookInfo()->Height=(short)dxw.GetScreenHeight();
					GetHookInfo()->Width=(short)dxw.GetScreenWidth();
					GetHookInfo()->ColorDepth=(short)DevMode.dmBitsPerPel;
				}
			}
			break;

		case WINDIRECT:
			// only change the color depth if requested, keep the resolution fixed.
			DevMode.dmSize = sizeof(DEVMODEA);
			if(lpDevMode){
				DevMode.dmBitsPerPel = dmBitsPerPel;
				DevMode.dmFields = DM_BITSPERPEL;
			}
			ret = (*pChangeDisplaySettingsExA)(NULL, &DevMode, NULL, dwflags, NULL);
			dxw.SetScreenSize(
				(dmFields & DM_PELSWIDTH) ? dmPelsWidth : dxw.GetScreenWidth(),
				(dmFields & DM_PELSHEIGHT) ? dmPelsHeight : dxw.GetScreenHeight());
			dxw.VirtualPixelFormat.dwRGBBitCount = dmBitsPerPel;
			GetHookInfo()->Height=(short)dxw.GetScreenHeight();
			GetHookInfo()->Width=(short)dxw.GetScreenWidth();
			GetHookInfo()->ColorDepth=(short)DevMode.dmBitsPerPel;
			break;

		case WINEMU:
			// in fully emulated mode, all values are accepted with no real display mode changes
			dxw.SetScreenSize(
				(dmFields & DM_PELSWIDTH) ? dmPelsWidth : dxw.GetScreenWidth(),
				(dmFields & DM_PELSHEIGHT) ? dmPelsHeight : dxw.GetScreenHeight());
			//if(dmFields & DM_BITSPERPEL) dxw.VirtualPixelFormat.dwRGBBitCount = dmPelsWidth;
			if(dmFields & DM_BITSPERPEL) dxw.VirtualPixelFormat.dwRGBBitCount = dmBitsPerPel; // v2.06.14 fix !!!
			// should make a limits check here ...
			GetHookInfo()->Height=(short)dxw.GetScreenHeight();
			GetHookInfo()->Width=(short)dxw.GetScreenWidth();
			GetHookInfo()->ColorDepth=(short)dmBitsPerPel;
			break;
	}

	if(dxw.bAutoScale) dxw.AutoScale();
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, sChangeDisplaySettingsRet(ret));

	return ret;
}

void dxwFixWindowPos(char *ApiName, HWND hwnd, LPARAM lParam)
{
	LPWINDOWPOS wp;
	int MaxX, MaxY;
	wp = (LPWINDOWPOS)lParam;
	MaxX = dxw.iSizX;
	MaxY = dxw.iSizY;
	if (!MaxX) MaxX = dxw.GetScreenWidth();
	if (!MaxY) MaxY = dxw.GetScreenHeight();
	static int iLastCX, iLastCY;
	static int BorderX=-1;
	static int BorderY=-1;
	int cx, cy;

	if(dxw.isColorEmulatedMode) return;

#ifndef DXW_NOTRACES
	char sFlags[256];
	OutTraceGDI("%s: GOT hwnd=%#x pos=(%d,%d) dim=(%d,%d) Flags=%#x(%s)\n", 
		ApiName, hwnd, wp->x, wp->y, wp->cx, wp->cy, wp->flags, ExplainWPFlags(wp->flags, sFlags, 256));
#endif // DXW_NOTRACES
	// if nothing to be moved, do nothing
	if ((wp->flags & (SWP_NOMOVE|SWP_NOSIZE))==(SWP_NOMOVE|SWP_NOSIZE)) return; //v2.02.13

	if (dxw.dwFlags1 & PREVENTMAXIMIZE){
		int UpdFlag = 0;
		WINDOWPOS MaxPos;
		dxw.CalculateWindowPos(hwnd, MaxX, MaxY, &MaxPos);

		if(wp->cx>MaxPos.cx) { wp->cx=MaxPos.cx; UpdFlag=1; }
		if(wp->cy>MaxPos.cy) { wp->cy=MaxPos.cy; UpdFlag=1; }
#ifndef DXW_NOTRACES
		if (UpdFlag) 
			OutTraceGDI("%s: SET max size=(%dx%d)\n", ApiName, wp->cx, wp->cy);
#endif // DXW_NOTRACES
	}

	if (dxw.IsFullScreen() && (hwnd==dxw.GethWnd())){
		if (dxw.dwFlags1 & LOCKWINPOS){ 
			dxw.CalculateWindowPos(hwnd, MaxX, MaxY, wp);
			OutTraceGDI("%s: LOCK pos=(%d,%d) size=(%dx%d)\n", ApiName, wp->x, wp->y, wp->cx, wp->cy);
		}
		// v2.03.95: locked size
		if (dxw.dwFlags2 & LOCKEDSIZE){ 
			WINDOWPOS MaxPos;
			dxw.CalculateWindowPos(hwnd, MaxX, MaxY, &MaxPos);
			wp->cx = MaxPos.cx;
			wp->cy = MaxPos.cy;
			OutTraceGDI("%s: SET locked size=(%dx%d)\n", ApiName, wp->cx, wp->cy);
		}
		if (dxw.dwFlags7 & ANCHORED){ 
			WINDOWPOS MaxPos;
			dxw.CalculateWindowPos(hwnd, MaxX, MaxY, &MaxPos);
			wp->cx = MaxPos.cx;
			wp->cy = MaxPos.cy;
			wp->x  = MaxPos.x;
			wp->y  = MaxPos.y;
			OutTraceGDI("%s: SET anchored pos=(%d,%d) size=(%dx%d)\n", ApiName, wp->x, wp->y, wp->cx, wp->cy);
		}	
	}

	if ((dxw.dwFlags2 & KEEPASPECTRATIO) && dxw.IsFullScreen() && (hwnd==dxw.GethWnd())){ 
		// note: while keeping aspect ration, resizing from one corner doesn't tell
		// which coordinate is prevalent to the other. We made an arbitrary choice.
		// note: v2.1.93: compensation must refer to the client area, not the wp
		// window dimensions that include the window borders.
		if(BorderX==-1){
			// v2.02.92: Fixed for AERO mode, where GetWindowRect substantially LIES!
			RECT client, full;
			LONG dwStyle, dwExStyle;
			HMENU hMenu;
			extern GetWindowLong_Type pGetWindowLong;
			(*pGetClientRect)(hwnd, &client);
			full=client;
			dwStyle=(*pGetWindowLong)(hwnd, GWL_STYLE);
			dwExStyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE);
			hMenu = (dwStyle & WS_CHILD) ? NULL : GetMenu(hwnd);	
			(*pAdjustWindowRectEx)(&full, dwStyle, (hMenu!=NULL), dwExStyle);
			if (hMenu && (hMenu != (HMENU)-1)) __try {CloseHandle(hMenu);} __except(EXCEPTION_EXECUTE_HANDLER){};
			BorderX= full.right - full.left - client.right;
			BorderY= full.bottom - full.top - client.bottom;
			OutTraceGDI("%s: KEEPASPECTRATIO window borders=(%d,%d)\n", ApiName, BorderX, BorderY);
		}
		extern LRESULT LastCursorPos;
		switch (LastCursorPos){
			case HTBOTTOM:
			case HTTOP:
			case HTBOTTOMLEFT:
			case HTBOTTOMRIGHT:
			case HTTOPLEFT:
			case HTTOPRIGHT:
				cx = BorderX + ((wp->cy - BorderY) * dxw.iRatioX) / dxw.iRatioY;
				if(cx!=wp->cx){
					OutTraceGDI("%s: KEEPASPECTRATIO adjusted cx=%d->%d\n", ApiName, wp->cx, cx);
					wp->cx = cx;
				}
				break;
			case HTLEFT:
			case HTRIGHT:
				cy = BorderY + ((wp->cx - BorderX) * dxw.iRatioY) / dxw.iRatioX;
				if(cy!=wp->cy){
					OutTraceGDI("%s: KEEPASPECTRATIO adjusted cy=%d->%d\n", ApiName, wp->cy, cy);
					wp->cy = cy;
				}
				break;
		}
	}

	if ((dxw.dwDFlags & CENTERTOWIN) && dxw.IsFullScreen() && (hwnd==dxw.GethWnd())){ 
		RECT wrect;
		LONG dwStyle, dwExStyle;
		HMENU hMenu;
		int minx, miny;
		wrect = dxw.GetScreenRect();
		dwStyle=(*pGetWindowLong)(hwnd, GWL_STYLE);
		dwExStyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE);
		hMenu = (dwStyle & WS_CHILD) ? NULL : GetMenu(hwnd);	
		(*pAdjustWindowRectEx)(&wrect, dwStyle, (hMenu!=NULL), dwExStyle);
		minx = wrect.right - wrect.left;
		miny = wrect.bottom - wrect.top;
		if(wp->cx < minx) wp->cx = minx;
		if(wp->cy < miny) wp->cy = miny;
	}

	iLastCX= wp->cx;
	iLastCY= wp->cy;
#ifndef DXW_NOTRACES
	OutDebugDW("%s: FIX hwnd=%#x pos=(%d,%d) dim=(%d,%d) Flags=%#x(%s)\n", 
		ApiName, hwnd, wp->x, wp->y, wp->cx, wp->cy, wp->flags, ExplainWPFlags(wp->flags, sFlags, 256));
#endif // DXW_NOTRACES

}

void dxwFixMinMaxInfo(char *ApiName, HWND hwnd, LPARAM lParam)
{
	if (dxw.dwFlags1 & PREVENTMAXIMIZE){
		LPMINMAXINFO lpmmi;
		lpmmi=(LPMINMAXINFO)lParam;
		OutTraceGDI("%s: GOT MaxPosition=(%d,%d) MaxSize=(%d,%d)\n", ApiName, 
			lpmmi->ptMaxPosition.x, lpmmi->ptMaxPosition.y, lpmmi->ptMaxSize.x, lpmmi->ptMaxSize.y);
		lpmmi->ptMaxPosition.x=0;
		lpmmi->ptMaxPosition.y=0;
		lpmmi->ptMaxSize.x = dxw.GetScreenWidth();
		lpmmi->ptMaxSize.y = dxw.GetScreenHeight();

		OutTraceGDI("%s: SET PREVENTMAXIMIZE MaxPosition=(%d,%d) MaxSize=(%d,%d)\n", ApiName, 
			lpmmi->ptMaxPosition.x, lpmmi->ptMaxPosition.y, lpmmi->ptMaxSize.x, lpmmi->ptMaxSize.y);
	}

	// v2.1.75: added logic to fix win coordinates to selected ones. 
	// fixes the problem with "Achtung Spitfire", that can't be managed through PREVENTMAXIMIZE flag.
	if (dxw.dwFlags1 & LOCKWINPOS){
		LPMINMAXINFO lpmmi;
		lpmmi=(LPMINMAXINFO)lParam;
		OutTraceGDI("%s: GOT MaxPosition=(%d,%d) MaxSize=(%d,%d)\n", ApiName, 
			lpmmi->ptMaxPosition.x, lpmmi->ptMaxPosition.y, lpmmi->ptMaxSize.x, lpmmi->ptMaxSize.y);
		lpmmi->ptMaxPosition.x=dxw.iPosX;
		lpmmi->ptMaxPosition.y=dxw.iPosY;
		lpmmi->ptMaxSize.x = dxw.iSizX ? dxw.iSizX : dxw.GetScreenWidth();
		lpmmi->ptMaxSize.y = dxw.iSizY ? dxw.iSizY : dxw.GetScreenHeight();
		OutTraceGDI("%s: SET LOCKWINPOS MaxPosition=(%d,%d) MaxSize=(%d,%d)\n", ApiName, 
			lpmmi->ptMaxPosition.x, lpmmi->ptMaxPosition.y, lpmmi->ptMaxSize.x, lpmmi->ptMaxSize.y);
	}
}

// see https://msdn.microsoft.com/en-us/library/windows/desktop/ms632679%28v=vs.85%29.aspx

// from MSDN:
// If an overlapped window is created with the WS_VISIBLE style bit set and the x parameter is set to CW_USEDEFAULT, 
// then the y parameter determines how the window is shown. If the y parameter is CW_USEDEFAULT, then the window 
// manager calls ShowWindow with the SW_SHOW flag after the window has been created. If the y parameter is some other
// value, then the window manager calls ShowWindow with that value as the nCmdShow parameter.

static BOOL IsFullscreenWindow(
	DWORD dwStyle, 
	DWORD dwExStyle, 
	HWND hWndParent,
	int x, 
	int y, 
	int nWidth, 
	int nHeight)
{
	if (dwExStyle & WS_EX_CONTROLPARENT) return FALSE; // @#@ "Diablo" fix
	if ((dwStyle & WS_CHILD) && (!dxw.IsDesktop(hWndParent))) return FALSE; // @#@ Diablo fix
	// OutTrace("!!! style=%#x parent=%#x isdesktop=%d visible=%d\n", dwStyle, hWndParent, dxw.IsDesktop(hWndParent), IsWindow(hWndParent));
	// if ((dwStyle & WS_CHILD) && dxw.IsDesktop(hWndParent) && IsWindow(hWndParent)) return FALSE; // @#@ "Amber: Journey Beyond" fix
	// if maximized. 
	if(dwStyle & WS_MAXIMIZE) return TRUE; 
	// go through here only when WS_CHILD of desktop window
	// v2.04.87: some programs use values different from CW_USEDEFAULT, but still with CW_USEDEFAULT bit set
	// in this case better use the bitwinse AND rather than the equal operator to find big windows
	if((x & CW_USEDEFAULT) && (dwStyle & (WS_POPUP|WS_CHILD))) x = y = 0;
	if(nWidth & CW_USEDEFAULT){
		if (dwStyle & (WS_POPUP|WS_CHILD)) nWidth = nHeight = 0;
		else nWidth = dxw.GetScreenWidth() - x;
		}
	// msdn undocumented case: x,y=(-1000, CW_USEDEFAULT) w,h=(CW_USEDEFAULT,CW_USEDEFAULT) in "Imperialism"
	if(nHeight & CW_USEDEFAULT){
		y = 0;
		nHeight = dxw.GetScreenHeight();
		}
	// if bigger than screen ...
	if((x<=0)&&
		(y<=0)&&
		(nWidth>=(int)dxw.GetScreenWidth())&&
		(nHeight>=(int)dxw.GetScreenHeight())) return TRUE;
	// v2.04.68.fx1: if there is no main window yet and the IsFullScreen flag is set ...
	if(!dxw.GethWnd() && (dxw.dwFlags3 & FORCEWINDOWING)) return TRUE;
	return FALSE;
}

static BOOL IsRelativePosition(DWORD dwStyle, DWORD dwExStyle, HWND hWndParent){
	// IsRelativePosition TRUE: 
	// tested on Gangsters: coordinates must be window-relative!!!
	// Age of Empires....
	// IsRelativePosition FALSE: 
	// needed for "Diablo", that creates a new WS_EX_CONTROLPARENT window that must be
	// overlapped to the directdraw surface.
	// needed for "Riven", that creates a new WS_POPUP window with the menu bar that must be
	// overlapped to the directdraw surface.
	if ((dwStyle & WS_CHILD) && !dxw.IsRealDesktop(hWndParent) && !(dwStyle & WS_POPUP))
		return TRUE;
	else
		return FALSE;
}

static void dxwCenterDialog(HWND dlgHWnd)
{
	RECT DesktopRect;
	RECT DialogRect;
	POINT TopLeft = {0, 0};
	int x, y;
	(*pGetClientRect)(dxw.GethWnd(), &DesktopRect);
	(*pClientToScreen)(dxw.GethWnd(), &TopLeft);
	(*pGetClientRect)(dlgHWnd, &DialogRect);
	x=TopLeft.x + (DesktopRect.right - DialogRect.right) / 2;
	y=TopLeft.y + (DesktopRect.bottom - DialogRect.bottom) / 2;
	//(*pMoveWindow)(dlgHWnd, x, y, DialogRect.right, DialogRect.bottom, FALSE);
	(*pSetWindowPos)(dlgHWnd, 0, x, y, 0, 0, SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
}

// --------------------------------------------------------------------------
//
// user32 API hookers
//
// --------------------------------------------------------------------------


BOOL WINAPI extInvalidateRect(HWND hwnd, RECT *lpRect, BOOL bErase)
{
	ApiName("InvalidateRect");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sRect[81];
		if(lpRect) sprintf(sRect, "(%d,%d)-(%d,%d)", lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		else strcpy(sRect, "NULL");
		OutTrace("%s: hwnd=%#x rect=%s erase=%#x\n", ApiRef, hwnd, sRect, bErase);
	}
#endif

	if(dxw.dwFlags11 & INVALIDATEFULLRECT) {
		lpRect = NULL;
		OutTraceGDI("%s: INVALIDATEFULLRECT fixed rect=NULL\n", ApiRef);
	}

	if(dxw.Windowize){
		if(dxw.IsRealDesktop(hwnd)){
			hwnd = dxw.GethWnd();
		}

		RECT ScaledRect;
		if(dxw.IsFullScreen()) { 
			switch(dxw.GDIEmulationMode){
				case GDIMODE_STRETCHED:
				case GDIMODE_SHAREDDC:
				case GDIMODE_EMULATED:
					if(lpRect) {
						// v2.03.55: the lpRect area must NOT be altered by the call
						// effect visible in partial updates of Deadlock 2 main menu buttons
						ScaledRect = *lpRect;
						dxw.MapClient(&ScaledRect);
						lpRect = &ScaledRect;
						OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", 
							ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
					}
					break;
				default:
					break;
			}
		}
	}

	return (*pInvalidateRect)(hwnd, lpRect, bErase);
}

BOOL WINAPI extShowWindow(HWND hwnd, int nCmdShow)
{
	BOOL res;
	ApiName("ShowWindow");
	extern HWND hTrayWnd;
	static long iLastSizX, iLastSizY;
	int nOrigCmd;
	//static long iLastPosX, iLastPosY;

	OutTraceGDI("%s: hwnd=%#x, CmdShow=%#x(%s)\n", ApiRef, hwnd, nCmdShow, ExplainShowCmd(nCmdShow));

	if((dxw.dwFlags9 & KILLBLACKWIN) && (hwnd==(HWND)FAKEKILLEDWIN)){
		OutTraceGDI("%s: ignore killed window\n", ApiRef);
		return TRUE;
	}

	if((dxw.dwFlags4 & PREVENTMINIMIZE) &&
		dxw.Windowize && 
		(hwnd == dxw.GethWnd()) && 
		(nCmdShow == SW_MINIMIZE)){
		OutTraceGDI("%s: suppress main window minimize\n", ApiRef);
		return TRUE;
	}

	if(dxw.Windowize && (hwnd == hTrayWnd) && (nCmdShow == SW_HIDE)){
		// v2.03.85: suppress attempts to hide the tray window
		OutTraceGDI("%s: suppress tray window hide\n", ApiRef);
		return TRUE;
	}

	if(dxw.Windowize && dxw.IsFullScreen() && dxw.IsDesktop(hwnd)){
		if(dxw.dwFlags1 & CLIPCURSOR){
			OutTraceGDI("%s: clipper on main win %s\n", ApiRef, (nCmdShow==SW_HIDE)?"OFF":"ON");
			(nCmdShow==SW_HIDE) ? dxw.EraseClipCursor() : dxw.SetClipCursor();
		}
	}

	nOrigCmd = nCmdShow;

	if ((dxw.isScaled) && // v2.04.89: trim window size only in WINDOWIZE mode
		((dxw.dwFlags9 & EMULATEMAXIMIZE) || (dxw.dwFlags1 & PREVENTMAXIMIZE))){
		BOOL bMustTrim = FALSE;
		// v2.04.32: check conditions
		if(nCmdShow==SW_MAXIMIZE) bMustTrim = TRUE;
		if(nCmdShow==SW_SHOWDEFAULT){
			STARTUPINFO StartupInfo;
			GetStartupInfo(&StartupInfo);
			OutTraceGDI("%s DEBUG: StartupInfo dwFlags=%#x ShowWindow=%#x\n", ApiRef, StartupInfo.dwFlags, StartupInfo.wShowWindow);
			if((StartupInfo.dwFlags & STARTF_USESHOWWINDOW) && (StartupInfo.wShowWindow == SW_MAXIMIZE))bMustTrim = TRUE;
		}
		// v2.04.32: trim the maximized window and set it to main window state
		if(bMustTrim){
			if (dxw.dwFlags1 & PREVENTMAXIMIZE){
				OutTraceGDI("%s: suppress SW_MAXIMIZE maximize\n", ApiRef);
				nCmdShow=SW_SHOWNORMAL;
			}
			if (dxw.dwFlags9 & EMULATEMAXIMIZE){
				OutTraceGDI("%s: emulate SW_MAXIMIZE maximize\n", ApiRef);
				nCmdShow=SW_SHOWNORMAL;
				(*pSetWindowPos)(hwnd, 0, dxw.iPosX, dxw.iPosY, dxw.iSizX, dxw.iSizY, SWP_NOZORDER|SWP_SHOWWINDOW);
			}
			// v2.04.32: since it should maximize, then it is a main window
			// v2.04.90: only if there's no other main window active
			if(!dxw.GethWnd()) {
				dxw.SethWnd(hwnd);
				dxw.FixWindowFrame(hwnd);
			}
		}
	}	

	res=(*pShowWindow)(hwnd, nCmdShow);
	// v2.03.95: force zero size when minimize and refresh window coordinates
	if(hwnd == dxw.GethWnd()){
		if(nCmdShow==SW_MINIMIZE) {
			dxw.IsVisible = FALSE;
			iLastSizX = dxw.iSizX;
			iLastSizY = dxw.iSizY;
			dxw.iSizX = dxw.iSizY = 0;
		}
		else {
			dxw.IsVisible = TRUE;
			if((dxw.iSizX == 0) && (dxw.iSizY == 0)){
				dxw.iSizX = iLastSizX;
				dxw.iSizY = iLastSizY;
			}
		}
	}

	//dxw.UpdateDesktopCoordinates();
	OutTraceGDI("%s: hwnd=%#x res=%#x(pre. visible=%s)\n", ApiRef, hwnd, res, res ? "YES" : "NO");

	return res;
}

LONG WINAPI extGetWindowLong(char *ApiName, GetWindowLong_Type pGetWindowLong, HWND hwnd, int nIndex)
{
	LONG res;

	res=(*pGetWindowLong)(hwnd, nIndex);

	OutDebugGDI("%s: hwnd=%#x, Index=%#x(%s) res=%#x\n", ApiName, hwnd, nIndex, ExplainSetWindowIndex(nIndex), res);

	// v2.04.46: handle DWL_DLGPROC only when flag is set
	if((nIndex==GWL_WNDPROC) ||
		((nIndex==DWL_DLGPROC) && (dxw.dwFlags8 & HOOKDLGWIN))){
		WNDPROC wp;
		wp=dxwws.GetProc(hwnd);
		if(wp){
			OutDebugGDI("%s: remapping WindowProc res=%#x -> %#x\n", ApiName, res, (LONG)wp);
			res=(LONG)wp; // if not found, don't alter the value.
		}
		else {
			OutDebugGDI("%s: keep original WindowProc res=%#x\n", ApiName, res);
		}
	}

	// v2.06.05: fix - when pretending fullscreen then pretend the main window is borderless
	if(nIndex==GWL_STYLE) {
		if(dxw.IsFullScreen() && (hwnd == dxw.GethWnd())){
			DWORD oldStyle = res;
			res &= ~WS_OVERLAPPEDWINDOW;
			//res = 0;
			OutTraceDW("%s: pretending main window style is not OVERLAPPEDWINDOW res=%#x->%#x\n", ApiName, oldStyle, res);
		}
	}

	return res;
}

LONG WINAPI extGetWindowLongA(HWND hwnd, int nIndex)
{ ApiName("GetWindowLongA"); return extGetWindowLong(ApiRef, pGetWindowLongA, hwnd, nIndex); }
LONG WINAPI extGetWindowLongW(HWND hwnd, int nIndex)
{ ApiName("GetWindowLongW"); return extGetWindowLong(ApiRef, pGetWindowLongW, hwnd, nIndex); }

LONG WINAPI extSetWindowLong(ApiArg, HWND hwnd, int nIndex, LONG dwNewLong, SetWindowLong_Type pSetWindowLong, GetWindowLong_Type pGetWindowLong)
{
	LONG res;

	OutTraceGDI("%s: hwnd=%#x, Index=%#x(%s) Val=%#x\n", 
		ApiRef, hwnd, nIndex, ExplainSetWindowIndex(nIndex), dwNewLong);

	if(dxw.dwFlags11 & CUSTOMLOCALE){
		pSetWindowLong = (IsWindowUnicode(hwnd)) ? pSetWindowLongW : pSetWindowLongA;
		pGetWindowLong = (IsWindowUnicode(hwnd)) ? pGetWindowLongW : pGetWindowLongA;
	}

	if (dxw.Windowize){
		if(dxw.dwFlags1 & LOCKWINSTYLE){
			if(nIndex==GWL_STYLE){
				OutTraceGDI("%s: Lock GWL_STYLE=%#x\n", ApiRef, dwNewLong);
				return (*pGetWindowLong)(hwnd, nIndex);
			}
			if(nIndex==GWL_EXSTYLE){
				OutTraceGDI("%s: Lock GWL_EXSTYLE=%#x\n", ApiRef, dwNewLong);
				return (*pGetWindowLong)(hwnd, nIndex);
			}
		}

		if (dxw.dwFlags1 & PREVENTMAXIMIZE){
			if(nIndex==GWL_STYLE){
				dwNewLong &= ~WS_MAXIMIZE; 
				if(dxw.IsDesktop(hwnd)){
					OutTraceGDI("%s: GWL_STYLE %#x suppress MAXIMIZE\n", ApiRef, dwNewLong);
					dwNewLong |= WS_OVERLAPPEDWINDOW; 
					dwNewLong &= ~(WS_DLGFRAME|WS_MAXIMIZE|WS_VSCROLL|WS_HSCROLL|WS_CLIPSIBLINGS); 
				}
			}
		}

		if(dxw.IsDesktop(hwnd) && (nIndex==GWL_EXSTYLE)){
			// v2.02.32: disable topmost for main window only
			if(dxw.dwFlags5 & UNLOCKZORDER) {
				OutTraceGDI("%s: GWL_EXSTYLE %#x suppress TOPMOST\n", ApiRef, dwNewLong);
				dwNewLong &= ~(WS_EX_TOPMOST); 
			}
			// v2.04.31: forces topmost for main window only
			if(dxw.dwFlags9 & LOCKTOPZORDER) {
				OutTraceGDI("%s: GWL_EXSTYLE %#x forces TOPMOST\n", ApiRef, dwNewLong);
				dwNewLong |= WS_EX_TOPMOST ;
			}
		}

		// v2.04.44: revised ...
		if((nIndex==GWL_STYLE) && !(dwNewLong & WS_CHILD) && dxw.IsDesktop(hwnd)){
			dwNewLong = dxw.FixWinStyle(dwNewLong);
		}

		if((nIndex==GWL_EXSTYLE) && !(dwNewLong & WS_CHILD) && dxw.IsDesktop(hwnd)){
			dwNewLong = dxw.FixWinExStyle(dwNewLong);
		}
	}

	// v2.03.94.fx2: removed dxw.IsFullScreen() check here ... WinProc routine must be verified in all conditions
	// fixes "Nascar Racing 3" that was setting the WinProc while still in non fullscreen mode!
	// v2.04.46: handle DWL_DLGPROC only when flag is set
	if((nIndex==GWL_WNDPROC) ||
		((nIndex==DWL_DLGPROC) && (dxw.dwFlags8 & HOOKDLGWIN))){
		LONG lres;
		WNDPROC OldProc;
		DWORD WinStyle;
		BOOL bHooked = FALSE;

		// fix ....
		extern LRESULT CALLBACK dw_Hider_Message_Handler(HWND, UINT, WPARAM, LPARAM);
		if(dwNewLong==(LONG)dw_Hider_Message_Handler) {
			return (*pSetWindowLong)(hwnd, nIndex, (LONG)dw_Hider_Message_Handler);
		}

		// GPL fix
		// v2.03.94.fx2: moved dxw.IsFullScreen() check here ...
		if(dxw.IsRealDesktop(hwnd) && dxw.Windowize && dxw.IsFullScreen()) {
			hwnd=dxw.GethWnd();
			OutTraceGDI("%s: DESKTOP hwnd, FIXING hwnd=%#x\n", ApiRef, hwnd);
		}
		// end of GPL fix

		OldProc = (WNDPROC)(*pGetWindowLong)(hwnd, nIndex);
		WinStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);

		while(TRUE){ // fake loop
			lres = -1; // initialize with not 0 value since 0 means error
			if(!(dxw.dwDFlags2 & NOWINDOWHOOKS)){
				// hook extWindowProc to main win ....
				if(dxw.IsDesktop(hwnd)){
					if(OldProc==extWindowProc) OldProc=dxwws.GetProc(hwnd);
					dxwws.PutProc(hwnd, (WNDPROC)dwNewLong);
					res=(LONG)OldProc;
					SetLastError(0);
					lres=(*pSetWindowLong)(hwnd, nIndex, (LONG)extWindowProc);
					OutTraceGDI("%s: DESKTOP hooked %#x->%#x\n", ApiRef, dwNewLong, extWindowProc);
					break;
				}

				// hook extDlgWindowProc to dialog win ....
				if((WinStyle & DWL_DLGPROC) && (dxw.dwFlags8 & HOOKDLGWIN)){
					if(OldProc==extDialogWindowProc) OldProc=dxwws.GetProc(hwnd);
					dxwws.PutProc(hwnd, (WNDPROC)dwNewLong);
					res=(LONG)OldProc;
					SetLastError(0);
					lres=(*pSetWindowLong)(hwnd, nIndex, (LONG)extDialogWindowProc);
					OutTraceGDI("%s: DIALOG hooked %#x->%#x\n", ApiRef, dwNewLong, extDialogWindowProc);
					break;
				}

				// hook extChildWindowProc to child win ....
				if((WinStyle & WS_CHILD) && (dxw.dwFlags1 & HOOKCHILDWIN)){
					if(OldProc==extChildWindowProc) OldProc=dxwws.GetProc(hwnd);
					dxwws.PutProc(hwnd, (WNDPROC)dwNewLong);
					res=(LONG)OldProc;
					SetLastError(0);
					lres=(*pSetWindowLong)(hwnd, nIndex, (LONG)extChildWindowProc);
					OutTraceGDI("%s: CHILD hooked %#x->%#x\n", ApiRef, dwNewLong, extChildWindowProc);
					break;
				}
			}

			// hook dwNewLong if not done otherwise
			res = (*pSetWindowLong)(hwnd, nIndex, dwNewLong);
			break;
		}
#ifndef DXW_NOTRACES
		int error = GetLastError();
		if(!lres && error) {
			OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, error, __LINE__);
		}
#endif // DXW_NOTRACES
	}
	else{
		// through here for any message different from GWL_WNDPROC or DWL_DLGPROC
		res=(*pSetWindowLong)(hwnd, nIndex, dwNewLong);
	}

	OutTraceGDI("%s: hwnd=%#x nIndex=%#x Val=%#x res=%#x\n", ApiRef, hwnd, nIndex, dwNewLong, res);
	return res;
}

LONG WINAPI extSetWindowLongA(HWND hwnd, int nIndex, LONG dwNewLong)
{ ApiName("SetWindowLongA"); return extSetWindowLong(ApiRef, hwnd, nIndex, dwNewLong, pSetWindowLongA, pGetWindowLongA); }
LONG WINAPI extSetWindowLongW(HWND hwnd, int nIndex, LONG dwNewLong)
{ ApiName("SetWindowLongW"); return extSetWindowLong(ApiRef, hwnd, nIndex, dwNewLong, pSetWindowLongW, pGetWindowLongW); }

#ifndef DXW_NOTRACES
char *ExplainSWPFlags(UINT c)
{
	static char eb[256];
	unsigned int l;
	strcpy(eb,"SWP_");
	if (c & SWP_NOSIZE) strcat(eb, "NOSIZE+");
	if (c & SWP_NOMOVE) strcat(eb, "NOMOVE+");
	if (c & SWP_NOZORDER) strcat(eb, "NOZORDER+");
	if (c & SWP_NOREDRAW) strcat(eb, "NOREDRAW+");
	if (c & SWP_NOACTIVATE) strcat(eb, "NOACTIVATE+");
	if (c & SWP_FRAMECHANGED) strcat(eb, "FRAMECHANGED+");
	if (c & SWP_SHOWWINDOW) strcat(eb, "SHOWWINDOW+");
	if (c & SWP_HIDEWINDOW) strcat(eb, "HIDEWINDOW+");
	if (c & SWP_NOCOPYBITS) strcat(eb, "NOCOPYBITS+");
	if (c & SWP_NOOWNERZORDER) strcat(eb, "NOOWNERZORDER+");
	if (c & SWP_NOSENDCHANGING) strcat(eb, "NOSENDCHANGING+");
	if (c & SWP_DEFERERASE) strcat(eb, "DEFERERASE+");
	if (c & SWP_ASYNCWINDOWPOS) strcat(eb, "ASYNCWINDOWPOS+");
	l=strlen(eb);
	if (l>strlen("SWP_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}
#endif // DXW_NOTRACES

BOOL WINAPI extSetWindowPos(HWND hwnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	BOOL res;
	ApiName("SetWindowPos");
	BOOL bMustFixPos = FALSE;
	DWORD dwStyle, dwExStyle;
	HWND hParent;
#ifdef DXWHIDEWINDOWUPDATES
	int origx, origy, origw, origh;
#endif
	OutTraceGDI("%s: hwnd=%#x%s insertafter=%#x pos=(%d,%d) dim=(%d,%d) Flags=%#x(%s)\n", 
		ApiRef, hwnd, dxw.IsFullScreen()?"(FULLSCREEN)":"", 
		hWndInsertAfter, 
		X, Y, cx, cy, 
		uFlags, ExplainSWPFlags(uFlags));

	if((dxw.dwFlags9 & KILLBLACKWIN) && (hwnd==(HWND)FAKEKILLEDWIN)){
		OutTraceGDI("%s: ignore killed window\n", ApiRef);
		return TRUE;
	}

	if(InMainWinCreation) MovedInCallback = TRUE;

	// v2.06.09: if isColorEmulatedMode bypass the proxy
	if(dxw.isColorEmulatedMode){
		// proxy
		res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#ifndef DXW_NOTRACES
		if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
		return res;
	}

#ifdef DXWHIDEWINDOWUPDATES
	origx = X;
	origy = Y;
	origw = cx;
	origh = cy;
#endif

	if(dxw.dwFlags13 & LOCKALLWINDOWS) {
		DWORD dwStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
		if(!(dwStyle & WS_CHILD)) return (*pSetWindowPos)(hwnd, hWndInsertAfter, dxw.iPos0X, dxw.iPos0Y, dxw.iSiz0X, dxw.iSiz0Y, uFlags);
	}

	OutDebugDW("%s: fullscreen=%#x desktop=%#x inmainwincreation=%#x\n", 
		ApiRef, dxw.IsFullScreen(), dxw.IsDesktop(hwnd), InMainWinCreation);

	// when not in fullscreen mode, just proxy the call
	// v2.04.91: ... unless the window is trying to grow quite bigger than desktop: in this case
	// when there's no better main win is better promote it. Ref. "Space Clash"
	if (!dxw.IsFullScreen()){
		// v2.05.98 fix: don't trim the window to the desktop size if resolution changes are allowed
		if(dxw.isScaled && IsFullscreenWindow(
			(*pGetWindowLong)(hwnd, GWL_STYLE), 
			(*pGetWindowLong)(hwnd, GWL_EXSTYLE), 
			GetParent(hwnd), 
			Y, Y, cx, cy
			)){
			OutTraceGDI("%s: expanding fullscreen candidate\n", ApiRef);
			if(!dxw.GethWnd()) {
				dxw.SetFullScreen(TRUE); // v2.05.15: also set fullscreen mode
				dxw.SethWnd(hwnd);
				dxw.FixWindowFrame(hwnd);
			}
			bMustFixPos = TRUE; 
		} else {
			res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#ifndef DXW_NOTRACES
			if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
			return res;
		}
	}

	// in fullscreen, but a child window inside .....
	dwStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
	dwExStyle = (*pGetWindowLong)(hwnd, GWL_EXSTYLE);
	hParent = GetParent(hwnd);
	//if (!dxw.IsDesktop(hwnd) && !InMainWinCreation){
	if (!dxw.IsDesktop(hwnd)){ // v2.04.97 - "Fallen Haven"
		if(IsRelativePosition(dwStyle, dwExStyle, hParent)){
			dxw.MapClient(&X, &Y, &cx, &cy);
			// v2.04.87: inner child size can't exceed desktop size
			if(cx > dxw.iSizX) cx = dxw.iSizX;
			if(cy > dxw.iSizY) cy = dxw.iSizY;

			OutTraceGDI("%s: REMAPPED pos=(%d,%d) dim=(%d,%d)\n", ApiRef, X, Y, cx, cy);

			if((dxw.dwFlags16 & TRIMCHILDWINDOWS) && ((uFlags & (SWP_NOMOVE|SWP_NOSIZE)) != (SWP_NOMOVE|SWP_NOSIZE))){
				int X0 = X;
				int Y0 = Y;
				RECT rect;
				(*pGetClientRect)(hwnd, &rect);
				if(uFlags & SWP_NOMOVE){ // if not moving ...
					X = rect.left;
					Y = rect.right;
				}
				if(uFlags & SWP_NOSIZE){ // if not resizing ...
					cx = rect.right - rect.left;
					cy = rect.bottom - rect.top;
				}
				if((X + cx) > dxw.iSizX) X = dxw.iSizX - cx;
				if((Y + cy) > dxw.iSizY) Y = dxw.iSizY - cy;
			}

			if(dxw.IsRealDesktop(hParent)){
				OutTraceGDI("%s: REMAPPED hparent=%#x->%#x\n", ApiRef, hParent, dxw.GethWnd());
				SetParent(hwnd, dxw.GethWnd());
			}

			// v2.05.60 fix: beware! 0 is the hwnd for the desktop, but also the value for HWND_TOP
			// also it is useless to fix hWndInsertAfter when not necessary
			// fixes "Virus the Game" intro animation
			if(!(uFlags & (SWP_NOZORDER | SWP_NOREPOSITION | SWP_NOOWNERZORDER))){
				if((hWndInsertAfter != HWND_TOP) && dxw.IsRealDesktop(hWndInsertAfter)){
					OutTraceGDI("%s: REMAPPED hWndInsertAfter=%#x->%#x\n", ApiRef, hWndInsertAfter, dxw.GethWnd());
					hWndInsertAfter = dxw.GethWnd();
					uFlags &= ~SWP_NOOWNERZORDER;
				}
			}

#ifdef DXWHIDEWINDOWUPDATES
			if(dxw.dwFlags10 & HIDEWINDOWCHANGES){
				DWORD lpWinCB;
				res=(*pSetWindowPos)(hwnd, hWndInsertAfter, origx, origy, origw, origh, uFlags);
				lpWinCB = (*pGetWindowLong)(hwnd, GWL_WNDPROC);
				(*pSetWindowLong)(hwnd, GWL_WNDPROC, NULL);
				res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
				(*pSetWindowLong)(hwnd, GWL_WNDPROC, lpWinCB);
			}
			else {
				res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
			}
#else
			res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#endif
#ifndef DXW_NOTRACES
			if(!res)OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
			return res;
		}
		else{
			dxw.MapWindow(&X, &Y, &cx, &cy);
		}
	}

	if (dxw.IsDesktop(hwnd) && (dxw.dwFlags1 & LOCKWINPOS) && !bMustFixPos){
		// Note: any attempt to change the window position, no matter where and how, through the
		// SetWindowPos API is causing resizing to the default 1:1 pixel size in Commandos. 
		// in such cases, there is incompatibility between LOCKWINPOS and LOCKWINSTYLE.
		OutTraceGDI("%s: locked position\n", ApiRef);
		// v2.04.90: LOCKWINPOS should NOT prevent other window changes but the position and size.
		// the SetWindowPos can't be skipped, but the new coordinates can be ignored.
		uFlags |= (SWP_NOSIZE | SWP_NOMOVE);
	}

	// v2.04.90: it's no use to trim the size when SWP_NOSIZE is set
	if ((dxw.dwFlags1 & PREVENTMAXIMIZE) && !(uFlags & SWP_NOSIZE)){
		int UpdFlag =0;
		int MaxX, MaxY;
		// v2.03.96: in PREVENTMAXIMIZE mode don't exceed the initial size
		MaxX = dxw.iSiz0X;
		MaxY = dxw.iSiz0Y;
		// v2.04.90: here we have real position & sizes
		//if (!MaxX) MaxX = dxw.GetScreenWidth();
		//if (!MaxY) MaxY = dxw.GetScreenHeight();
		if (!MaxX) MaxX = dxw.iSizX;
		if (!MaxY) MaxY = dxw.iSizY;
		if(cx>MaxX) { cx=MaxX; UpdFlag=1; }
		if(cy>MaxY) { cy=MaxY; UpdFlag=1; }
#ifndef DXW_NOTRACES
		if (UpdFlag) 
			OutTraceGDI("%s: using max dim=(%d,%d)\n", ApiRef, cx, cy);
#endif // DXW_NOTRACES
	}

	// v2.04.90: when moving a big window, set position coordinates. Fixes "Space Clash" intro movies.
	if(bMustFixPos && !(uFlags & SWP_NOMOVE)){
		X = dxw.iPosX;
		Y = dxw.iPosY;
	}

	// useful??? to be demonstrated....
	// when altering main window in fullscreen mode, fix the coordinates for borders
	if(!(uFlags & SWP_NOSIZE)){
		DWORD dwCurStyle, dwExStyle;
		HMENU hMenu;
		RECT rect;
		rect.top=rect.left=0;
		rect.right=cx; rect.bottom=cy;
		dwCurStyle=(*pGetWindowLong)(hwnd, GWL_STYLE);
		dwExStyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE);
		// BEWARE: from MSDN -  If the window is a child window, the return value is undefined. 
		hMenu = (dwCurStyle & WS_CHILD) ? NULL : GetMenu(hwnd);	
		(*pAdjustWindowRectEx)(&rect, dwCurStyle, (hMenu!=NULL), dwExStyle);
		if (hMenu && (hMenu != (HMENU)-1)) __try {CloseHandle(hMenu);} __except(EXCEPTION_EXECUTE_HANDLER){};
		cx=rect.right; cy=rect.bottom;
		OutTraceGDI("%s: main form hwnd=%#x fixed size=(%d,%d)\n", ApiRef, hwnd, cx, cy);
	}

#ifdef DXWHIDEWINDOWUPDATES
	if(dxw.dwFlags10 & HIDEWINDOWCHANGES){
		DWORD lpWinCB;
		res=(*pSetWindowPos)(hwnd, hWndInsertAfter, origx, origy, origw, origh, uFlags);
		lpWinCB = (*pGetWindowLong)(hwnd, GWL_WNDPROC);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, NULL);
		res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, lpWinCB);
	}
	else {
		res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	}
#else
	res=(*pSetWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#endif
#ifndef DXW_NOTRACES
	if(!res)OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
	if(dxw.bAutoScale) dxw.AutoScale();
	return res;
}

HDWP WINAPI extBeginDeferWindowPos(int nNumWindows)
{
	// tracing only wrapper 
	HDWP res;
	ApiName("BeginDeferWindowPos");
	res = (*pBeginDeferWindowPos)(nNumWindows);
	OutTraceGDI("%s: numw=%d hdwp=%#x\n", ApiRef, nNumWindows, res);
	return res;
}

HDWP WINAPI extDeferWindowPos(HDWP hWinPosInfo, HWND hwnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	// v2.02.31: heavily used by "Imperialism II" !!!
	HDWP res;
	ApiName("DeferWindowPos");
	BOOL bMustFixPos = FALSE;
	DWORD dwStyle, dwExStyle;
	HWND hParent;

	OutTraceGDI("%s: hwnd=%#x%s insertafter=%#x pos=(%d,%d) dim=(%d,%d) Flags=%#x(%s)\n", 
		ApiRef, hwnd, dxw.IsFullScreen()?"(FULLSCREEN)":"", 
		hWndInsertAfter, 
		X, Y, cx, cy, 
		uFlags, ExplainSWPFlags(uFlags));

	if(InMainWinCreation) MovedInCallback = TRUE;

	// v2.06.09: if isColorEmulatedMode bypass the proxy
	if(dxw.isColorEmulatedMode){
		// proxy
		res=(*pGDIDeferWindowPos)(hWinPosInfo, hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#ifndef DXW_NOTRACES
		if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
		return res;
	}

#ifdef DXWHIDEWINDOWUPDATES
	origx = X;
	origy = Y;
	origw = cx;
	origh = cy;
#endif

	if(dxw.dwFlags13 & LOCKALLWINDOWS) {
		DWORD dwStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
		if(!(dwStyle & WS_CHILD)) return (*pGDIDeferWindowPos)(hWinPosInfo, hwnd, hWndInsertAfter, dxw.iPos0X, dxw.iPos0Y, dxw.iSiz0X, dxw.iSiz0Y, uFlags);
	}

	OutDebugDW("%s: fullscreen=%#x desktop=%#x inmainwincreation=%#x\n", 
		ApiRef, dxw.IsFullScreen(), dxw.IsDesktop(hwnd), InMainWinCreation);

	// when not in fullscreen mode, just proxy the call
	// v2.04.91: ... unless the window is trying to grow quite bigger than desktop: in this case
	// when there's no better main win is better promote it. Ref. "Space Clash"
	if (!dxw.IsFullScreen()){
		// v2.05.98 fix: don't trim the window to the desktop size if resolution changes are allowed
		if(dxw.isScaled && IsFullscreenWindow(
			(*pGetWindowLong)(hwnd, GWL_STYLE), 
			(*pGetWindowLong)(hwnd, GWL_EXSTYLE), 
			GetParent(hwnd), 
			Y, Y, cx, cy
			)){
			OutTraceGDI("%s: expanding fullscreen candidate\n", ApiRef);
			if(!dxw.GethWnd()) {
				dxw.SetFullScreen(TRUE); // v2.05.15: also set fullscreen mode
				dxw.SethWnd(hwnd);
				dxw.FixWindowFrame(hwnd);
			}
			bMustFixPos = TRUE; 
		} else {
			res=(*pGDIDeferWindowPos)(hWinPosInfo, hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#ifndef DXW_NOTRACES
			if(!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
			return res;
		}
	}

	// in fullscreen, but a child window inside .....
	dwStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
	dwExStyle = (*pGetWindowLong)(hwnd, GWL_EXSTYLE);
	hParent = GetParent(hwnd);
	//if (!dxw.IsDesktop(hwnd) && !InMainWinCreation){
	if (!dxw.IsDesktop(hwnd)){ // v2.04.97 - "Fallen Haven"
		if(IsRelativePosition(dwStyle, dwExStyle, hParent)){
			dxw.MapClient(&X, &Y, &cx, &cy);
			// v2.04.87: inner child size can't exceed desktop size
			if(cx > dxw.iSizX) cx = dxw.iSizX;
			if(cy > dxw.iSizY) cy = dxw.iSizY;

			OutTraceGDI("%s: REMAPPED pos=(%d,%d) dim=(%d,%d)\n", ApiRef, X, Y, cx, cy);

			if((dxw.dwFlags16 & TRIMCHILDWINDOWS) && ((uFlags & (SWP_NOMOVE|SWP_NOSIZE)) != (SWP_NOMOVE|SWP_NOSIZE))){
				int X0 = X;
				int Y0 = Y;
				RECT rect;
				(*pGetClientRect)(hwnd, &rect);
				if(uFlags & SWP_NOMOVE){ // if not moving ...
					X = rect.left;
					Y = rect.right;
				}
				if(uFlags & SWP_NOSIZE){ // if not resizing ...
					cx = rect.right - rect.left;
					cy = rect.bottom - rect.top;
				}
				if((X + cx) > dxw.iSizX) X = dxw.iSizX - cx;
				if((Y + cy) > dxw.iSizY) Y = dxw.iSizY - cy;
			}

			if(dxw.IsRealDesktop(hParent)){
				OutTraceGDI("%s: REMAPPED hparent=%#x->%#x\n", ApiRef, hParent, dxw.GethWnd());
				SetParent(hwnd, dxw.GethWnd());
			}

			// v2.05.60 fix: beware! 0 is the hwnd for the desktop, but also the value for HWND_TOP
			// also it is useless to fix hWndInsertAfter when not necessary
			// fixes "Virus the Game" intro animation
			if(!(uFlags & (SWP_NOZORDER | SWP_NOREPOSITION | SWP_NOOWNERZORDER))){
				if((hWndInsertAfter != HWND_TOP) && dxw.IsRealDesktop(hWndInsertAfter)){
					OutTraceGDI("%s: REMAPPED hWndInsertAfter=%#x->%#x\n", ApiRef, hWndInsertAfter, dxw.GethWnd());
					hWndInsertAfter = dxw.GethWnd();
					uFlags &= ~SWP_NOOWNERZORDER;
				}
			}

#ifdef DXWHIDEWINDOWUPDATES
			if(dxw.dwFlags10 & HIDEWINDOWCHANGES){
				DWORD lpWinCB;
				res=(*pGDIDeferWindowPos)(hwnd, hWndInsertAfter, origx, origy, origw, origh, uFlags);
				lpWinCB = (*pGetWindowLong)(hwnd, GWL_WNDPROC);
				(*pSetWindowLong)(hwnd, GWL_WNDPROC, NULL);
				res=(*pGDIDeferWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
				(*pSetWindowLong)(hwnd, GWL_WNDPROC, lpWinCB);
			}
			else {
				res=(*pDeferWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
			}
#else
			res=(*pGDIDeferWindowPos)(hWinPosInfo, hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#endif
#ifndef DXW_NOTRACES
			if(!res)OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
			return res;
		}
		else{
			dxw.MapWindow(&X, &Y, &cx, &cy);
		}
	}

	if (dxw.IsDesktop(hwnd) && (dxw.dwFlags1 & LOCKWINPOS) && !bMustFixPos){
		// Note: any attempt to change the window position, no matter where and how, through the
		// SetWindowPos API is causing resizing to the default 1:1 pixel size in Commandos. 
		// in such cases, there is incompatibility between LOCKWINPOS and LOCKWINSTYLE.
		OutTraceGDI("%s: locked position\n", ApiRef);
		// v2.04.90: LOCKWINPOS should NOT prevent other window changes but the position and size.
		// the SetWindowPos can't be skipped, but the new coordinates can be ignored.
		uFlags |= (SWP_NOSIZE | SWP_NOMOVE);
	}

	// v2.04.90: it's no use to trim the size when SWP_NOSIZE is set
	if ((dxw.dwFlags1 & PREVENTMAXIMIZE) && !(uFlags & SWP_NOSIZE)){
		int UpdFlag =0;
		int MaxX, MaxY;
		// v2.03.96: in PREVENTMAXIMIZE mode don't exceed the initial size
		MaxX = dxw.iSiz0X;
		MaxY = dxw.iSiz0Y;
		// v2.04.90: here we have real position & sizes
		//if (!MaxX) MaxX = dxw.GetScreenWidth();
		//if (!MaxY) MaxY = dxw.GetScreenHeight();
		if (!MaxX) MaxX = dxw.iSizX;
		if (!MaxY) MaxY = dxw.iSizY;
		if(cx>MaxX) { cx=MaxX; UpdFlag=1; }
		if(cy>MaxY) { cy=MaxY; UpdFlag=1; }
#ifndef DXW_NOTRACES
		if (UpdFlag) 
			OutTraceGDI("%s: using max dim=(%d,%d)\n", ApiRef, cx, cy);
#endif // DXW_NOTRACES
	}

	// v2.04.90: when moving a big window, set position coordinates. Fixes "Space Clash" intro movies.
	if(bMustFixPos && !(uFlags & SWP_NOMOVE)){
		X = dxw.iPosX;
		Y = dxw.iPosY;
	}

	// useful??? to be demonstrated....
	// when altering main window in fullscreen mode, fix the coordinates for borders
	if(!(uFlags & SWP_NOSIZE)){
		DWORD dwCurStyle, dwExStyle;
		HMENU hMenu;
		RECT rect;
		rect.top=rect.left=0;
		rect.right=cx; rect.bottom=cy;
		dwCurStyle=(*pGetWindowLong)(hwnd, GWL_STYLE);
		dwExStyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE);
		// BEWARE: from MSDN -  If the window is a child window, the return value is undefined. 
		hMenu = (dwCurStyle & WS_CHILD) ? NULL : GetMenu(hwnd);	
		(*pAdjustWindowRectEx)(&rect, dwCurStyle, (hMenu!=NULL), dwExStyle);
		if (hMenu && (hMenu != (HMENU)-1)) __try {CloseHandle(hMenu);} __except(EXCEPTION_EXECUTE_HANDLER){};
		cx=rect.right; cy=rect.bottom;
		OutTraceGDI("%s: main form hwnd=%#x fixed size=(%d,%d)\n", ApiRef, hwnd, cx, cy);
	}

#ifdef DXWHIDEWINDOWUPDATES
	if(dxw.dwFlags10 & HIDEWINDOWCHANGES){
		DWORD lpWinCB;
		res=(*pDeferWindowPos)(hwnd, hWndInsertAfter, origx, origy, origw, origh, uFlags);
		lpWinCB = (*pGetWindowLong)(hwnd, GWL_WNDPROC);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, NULL);
		res=(*pDeferWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, lpWinCB);
	}
	else {
		res=(*pDeferWindowPos)(hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	}
#else
	res=(*pGDIDeferWindowPos)(hWinPosInfo, hwnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
#endif
#ifndef DXW_NOTRACES
	if(!res)OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
	if(dxw.bAutoScale) dxw.AutoScale();
	return res;
}

#ifndef DXW_NOTRACES
static char *ExplainScrollMode(WORD m)
{
	char *s;
	switch(m){
		case SB_LINEUP: s="LINEUP/LEFT"; break;
		case SB_LINEDOWN: s="LINEDOWN/RIGHT"; break;
		case SB_PAGEUP: s="PAGEUP/LEFT"; break;
		case SB_PAGEDOWN: s="PAGEDOWN/RIGHT"; break;
		case SB_THUMBPOSITION: s="THUMBPOSITION"; break;
		case SB_THUMBTRACK: s="THUMBTRACK"; break;
		case SB_TOP: s="TOP/LEFT"; break;
		case SB_BOTTOM: s="BOTTOM/RIGHT"; break;
		case SB_ENDSCROLL: s="ENDSCROLL"; break;
		default: s="???";
	}
	return s;
}
#endif // DXW_NOTRACES

#define LVM_GETITEMCOUNT 0x1004

LRESULT WINAPI extSendMessage(char *apiname, SendMessage_Type pSendMessage, BOOL isAnsi, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
#ifndef DXW_NOTRACES
	OutTraceW("%s: hwnd=%#x WinMsg=[%#x]%s(%#x,%#x)\n", 
		apiname, hwnd, Msg, ExplainWinMessage(Msg), wParam, lParam);
	if(Msg == WM_SETTEXT){
		if(isAnsi){
			OutTraceW("%s: WM_SETTEXT text=\"%s\"\n", apiname, (CHAR *)lParam);
		}
		else {
			OutTraceW("%s: WM_SETTEXT text=\"%ls\"\n", apiname, (WCHAR *)lParam);
		}
	}
#endif // DXW_NOTRACES

	if(dxw.MustFixMouse){
		switch (Msg){
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
			// revert here the WindowProc mouse correction
			POINT prev, curr;
			RECT rect;
			prev.x = LOWORD(lParam);
			prev.y = HIWORD(lParam);
			(*pGetClientRect)(dxw.GethWnd(), &rect);
			curr.x = (prev.x * rect.right) / dxw.GetScreenWidth();
			curr.y = (prev.y * rect.bottom) / dxw.GetScreenHeight();
			if (Msg == WM_MOUSEWHEEL){ // v2.02.33 mousewheel fix
				POINT upleft={0,0};
				(*pClientToScreen)(dxw.GethWnd(), &upleft);
				curr = dxw.AddCoordinates(curr, upleft);
			}
			lParam = MAKELPARAM(curr.x, curr.y); 
			OutTraceC("%s: hwnd=%#x pos XY=(%d,%d)->(%d,%d)\n", apiname, hwnd, prev.x, prev.y, curr.x, curr.y);
			break;
		case WM_FONTCHANGE:
			// suppress WM_FONTCHANGE avoids "Warhammer: Shadow of the Horned Rat" crash when entering battle
			OutTraceGDI("%s: WM_FONTCHANGE suppressed\n", apiname);
			return 0;
			break;
		case WM_VSCROLL:
		case WM_HSCROLL:
			OutTraceW("%s: %s pos=%d scroll=%#x(%s) handle=%#x\n", 
				apiname,
				(Msg == WM_VSCROLL) ? "WM_VSCROLL" : "WM_HSCROLL",
				HIWORD(wParam),
				LOWORD(wParam), ExplainScrollMode(LOWORD(wParam)),
				lParam);
			break;
		default:
			break;
		}
	}

#ifndef MCIWNDM_OPENA
#define MCIWNDM_OPENA (WM_USER + 153) // from Vfw.h
#endif
#ifndef MCIWNDM_OPENW
#define MCIWNDM_OPENW	(WM_USER + 252)
#endif

	if(dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE)){
		switch (Msg){
			case MCIWNDM_OPENA:
				OutTrace("%s: mapping path=\"%s\"\n", apiname, (char *)lParam);
				lParam = (WPARAM)dxwTranslatePathA((LPCSTR)lParam, NULL);
				break;
			case MCIWNDM_OPENW: // v2.05.62
				OutTrace("%s: mapping path=\"%ls\"\n", apiname, (char *)lParam);
				lParam = (WPARAM)dxwTranslatePathW((LPCWSTR)lParam, NULL);
				break;
		}
	}

	if((dxw.dwFlags14 & NOSETTEXT) && (Msg == WM_SETTEXT)){
		OutTraceGDI("%s: SUPPRESS WM_SETTEXT\n", apiname);
		return 1;
	}

	//	WCHAR *lpWindTitleW = NULL;
	//	char *lpWindTitleA = (char *)lParam;
	//	OutTraceLOC("%s: ANSI text=\"%s\"\n", apiname, lpWindTitleA);
	//	int size = strlen(lpWindTitleA);
	//	lpWindTitleW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
	//	int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpWindTitleA, size, lpWindTitleW, size);
	//	lpWindTitleW[n] = L'\0'; // make tail ! 
	//	OutTraceLOC("%s: WIDE text=\"%ls\"\n", apiname, lpWindTitleW);
	//	ret=(*pSendMessageW)(hwnd, Msg, wParam, lParam);
	//	OutTraceW("%s: lresult=%#x\n", apiname, ret); 
	//	free(lpWindTitleW);
	//	return ret;
	//}

	ret=(*pSendMessage)(hwnd, Msg, wParam, lParam);
	OutTraceW("%s: lresult=%#x\n", apiname, ret); 
	return ret;
}

LRESULT WINAPI extSendMessageA(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ ApiName("SendMessageA"); return extSendMessage(ApiRef, pSendMessageA, TRUE, hwnd, Msg, wParam, lParam); }
LRESULT WINAPI extSendMessageW(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ ApiName("SendMessageW"); return extSendMessage(ApiRef, pSendMessageW, TRUE, hwnd, Msg, wParam, lParam); }

#define MOUSEHYSTERESIS TRUE
#define HYSTERESISVALUE 3
#define BORDERTOLERANCE 5

BOOL WINAPI extGetCursorPos(LPPOINT lppoint)
{
	ApiName("GetCursorPos");
	HRESULT res;
	static int PrevX, PrevY;
	POINT prev;

	if(dxw.dwFlags1 & SLOWDOWN) dxw.DoSlow(2);

	if (pGetCursorPos) {
		res=(*pGetCursorPos)(lppoint);
		if(dxw.dwFlags20 & CENTERONEXIT){
			if(!dxw.bActive){
				lppoint->x = dxw.iPosX + (dxw.iSizX / 2);
				lppoint->y = dxw.iPosY + (dxw.iSizY / 2);
			}
			else {
				if((lppoint->x < (dxw.iPosX - BORDERTOLERANCE)) || (lppoint->x > (dxw.iPosX + dxw.iSizX + BORDERTOLERANCE))) 
					lppoint->x = dxw.iPosX + (dxw.iSizX / 2);
				if((lppoint->y < (dxw.iPosY - BORDERTOLERANCE)) || (lppoint->y > (dxw.iPosY + dxw.iSizY + BORDERTOLERANCE))) 
					lppoint->y = dxw.iPosY + (dxw.iSizY / 2);
			}
		}
	}
	else {
		lppoint->x =0; lppoint->y=0;
		res=1;
	}

	if(dxw.MustFixMouse){
		dxw.UpdateDesktopCoordinates();
		prev=*lppoint;
		dxw.UnmapWindow(lppoint);
		dxw.FixCursorClipper(lppoint);

		if(MOUSEHYSTERESIS){
			int dx = lppoint->x - LastCurPosX;
			int dy = lppoint->y - LastCurPosY;
			if((dx < HYSTERESISVALUE) && (dx > -HYSTERESISVALUE)) lppoint->x = LastCurPosX;
			if((dy < HYSTERESISVALUE) && (dy > -HYSTERESISVALUE)) lppoint->y = LastCurPosY;
		}

		OutTraceC("%s: FIXED pos=(%d,%d)->(%d,%d)\n", ApiRef, prev.x, prev.y, lppoint->x, lppoint->y);
	}
	else {
		OutTraceC("%s: pos=(%d,%d)\n", ApiRef, lppoint->x, lppoint->y);
	}
	// v2.05.94: added fix for handling x,y axis inversion
	if(dxw.dwFlags11 & INVERTMOUSEXAXIS) lppoint->x = dxw.GetScreenWidth() - lppoint->x;
	if(dxw.dwFlags11 & INVERTMOUSEYAXIS) lppoint->y = dxw.GetScreenHeight() - lppoint->y;
	GetHookInfo()->CursorX=(short)lppoint->x;
	GetHookInfo()->CursorY=(short)lppoint->y;

	if((dxw.dwFlags1 & HIDEHWCURSOR) && dxw.IsFullScreen()) while((*pShowCursor)(0) >= 0);
	if(dxw.dwFlags2 & SHOWHWCURSOR) while((*pShowCursor)(1) < 0);


	return res;
}

static BOOL WINAPI intSetCursorPos(ApiArg, SetCursorPos_Type pSetCursorPos, int x, int y)
{
	BOOL res;
	int PrevX, PrevY;

	OutDebugC("%s: XY=(%d,%d)\n", ApiRef, x, y);
	PrevX=x;
	PrevY=y;

	if(dxw.dwFlags11 & INVERTMOUSEXAXIS) x = dxw.GetScreenWidth() - x;
	if(dxw.dwFlags11 & INVERTMOUSEYAXIS) y = dxw.GetScreenHeight() - y;
	LastCurPosX=x;
	LastCurPosY=y;

	// v2.05.15: SetCursorPos is disabled also when the window loses focus. Requested for "Yu No"
	if(!dxw.bActive || (dxw.dwFlags2 & KEEPCURSORFIXED)) {
		OutTraceC("%s: SUPPRESS pos=(%d,%d)\n", ApiRef, x, y);
		return 1;
	}

	if(dxw.dwFlags1 & SLOWDOWN) dxw.DoSlow(2);

	if(dxw.dwFlags1 & TRIMMOUSEPOSITION){
		// Intercept SetCursorPos outside screen boundaries (used as Cursor OFF in some games)
		if ((y<0)||(y>=(int)dxw.GetScreenHeight())||(x<0)||(x>=(int)dxw.GetScreenWidth())) return 1;
	}

	if(dxw.MustFixMouse){
		// v2.03.41
		dxw.UpdateDesktopCoordinates();
		dxw.MapWindow(&x, &y);
	}

	// v2.04.30: hint by gsky916 - "In specific case SetCursorPos could cause crazy mouse movement 
	// that made the mouse click out of player's control. While the game call this function quite frequently, 
	// if I add some sleep to slow down the crazy mouse movement, the whole game become lag. 
	// Finally I got an idea is using mouse_event to emulate SetCursorPos, and it works"
#if 0
	if((dxw.dwFlags9 & MOUSEMOVEBYEVENT) && pmouse_event) {
		// If MOUSEEVENTF_ABSOLUTE value is specified, dx and dy contain normalized
		// absolute coordinates between 0 and 65,535. The event procedure maps these
		// coordinates onto the display surface. Coordinate (0,0) maps onto the 
		// upper-left corner of the display surface; coordinate (65535,65535) maps onto
		// the lower-right corner. In a multimonitor system, the coordinates map to the
		// primary monitor. 
		DWORD nx = x*65535/GetSystemMetrics(SM_CXSCREEN);
		DWORD ny = y*65535/GetSystemMetrics(SM_CYSCREEN);
		(*pmouse_event)(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_MOVE,nx,ny,0,0);
#else
	if((dxw.dwFlags9 & MOUSEMOVEBYEVENT) && pSendInput) {
		INPUT in;
		in.type = INPUT_MOUSE;
		in.mi.dx = x*65535/GetSystemMetrics(SM_CXSCREEN);
		in.mi.dy = y*65535/GetSystemMetrics(SM_CYSCREEN);
		in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
		in.mi.mouseData = 0;
		in.mi.dwExtraInfo = 0;
		in.mi.time = 0;
		if((*pSendInput)(1, &in, sizeof(in)) != 1){
			OutErrorGDI("%s: SendInput ERROR err=%d\n", ApiRef, GetLastError());
		}
#endif
		return TRUE; // ok
	}

	res=0;
	if (pSetCursorPos) res=(*pSetCursorPos)(x,y);

	OutTraceC("%s: res=%#x XY=(%d,%d)->(%d,%d)\n", ApiRef, res, PrevX, PrevY, x, y);
	return res;
}

BOOL WINAPI extSetCursorPos(int x, int y)
{ return intSetCursorPos("SetCursorPos", pSetCursorPos, x, y); }
BOOL WINAPI extSetPhysicalCursorPos(int x, int y)
{ return intSetCursorPos("SetPhysicalCursorPos", pSetPhysicalCursorPos, x, y); }

static void FixMessageLParam(ApiArg, LPMSG lpMsg, LPPOINT lpPoint)
{
	switch(lpMsg->message){
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
		case WM_MOUSEHWHEEL:
			lpMsg->lParam = MAKELPARAM(lpPoint->x, lpPoint->y);
			OutTraceC("%s: fixed lParam=(%d,%d)\n", api, (short)LOWORD(lpMsg->lParam), (short)HIWORD(lpMsg->lParam));
			break;
	}
}

static BOOL WINAPI extPeekMessage(char *api, PeekMessage_Type pPeekMessage, LPMSG lpMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	BOOL res;
	char *sLabel;

	if(dxw.dwFlags3 & PEEKALLMESSAGES){
		sLabel="(ANY) ";
		if((wMsgFilterMin==0) && (wMsgFilterMax == 0)){
			// no filtering, everything is good
			res=(*pPeekMessage)(lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, (wRemoveMsg & 0x000F));
		}
		else {
			MSG Dummy;
			// better eliminate all messages before and after the selected range !!!!
			//if(wMsgFilterMin)(*pPeekMessage)(&Dummy, hwnd, 0, wMsgFilterMin-1, TRUE);
			if(wMsgFilterMin>0x0F)(*pPeekMessage)(&Dummy, hwnd, 0x0F, wMsgFilterMin-1, TRUE);
			res=(*pPeekMessage)(lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, (wRemoveMsg & 0x000F));
			if(wMsgFilterMax<WM_KEYFIRST)(*pPeekMessage)(&Dummy, hwnd, wMsgFilterMax+1, WM_KEYFIRST-1, TRUE); // don't touch above WM_KEYFIRST !!!!
		}

	}
	else {
		sLabel="";
		res=(*pPeekMessage)(lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, (wRemoveMsg & 0x000F));
	}
#ifndef DXW_NOTRACES
	if(IsTraceW || IsTraceC)  {
		char sMsg[128];
		if(res){
			OutTrace(
				"%s: %slpmsg=%#x hwnd=%#x filter=(%#x-%#x) remove=%#x(%s) res=%#x "
				"msg={message=%#x(%s) hwnd=%#x wparam=%#x lparam=%#x pt=(%d,%d) time=%#x}\n", 
				api, sLabel, lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg, ExplainPeekRemoveMsg(wRemoveMsg, sMsg, 128), res,
				lpMsg->message, ExplainWinMessage(lpMsg->message & 0xFFFF), lpMsg->hwnd,
				lpMsg->wParam, lpMsg->lParam, lpMsg->pt.x, lpMsg->pt.y, lpMsg->time);
		}
		else {
			// v2.04.47: trace void peeks only in debug mode, they can outnumber other logs
			OutDebugW("%s: %slpmsg=%#x hwnd=%#x filter=(%#x-%#x) remove=%#x(%s) res=%#x\n", 
				api, sLabel, lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg, ExplainPeekRemoveMsg(wRemoveMsg, sMsg, 128), res);
		}
	}
#endif

	// v2.04.60: new SWALLOWMOUSEMOVE flag to eliminate mouse movement messages received too often.
	// In "Akte Europa" the reception of too many movement messages blocked the gameplay while the mouse was moved.
	if(res && (dxw.dwFlags5 & SWALLOWMOUSEMOVE)){
		static DWORD LastMouseMessage = 0;
		if(lpMsg->message == WM_MOUSEMOVE){
			DWORD now = (*pGetTickCount)();
			// 50 mSec is for sure greater than any sync delay and gives 20 updates / sec. 
			// v2.05.27: Kayel Gee fix - use MaxFPS when defined
			if((now - LastMouseMessage) > ((dxw.dwFlags2 & LIMITFPS) ? dxw.MaxFPS : 50)){ 
				OutDebugW("%s: passing WM_MOUSEMOVE at %#x\n", api, now);
				LastMouseMessage = (*pGetTickCount)();
			}
			else {
				OutDebugW("%s: swallow WM_MOUSEMOVE at %#x\n", api, now);
				res=(*pPeekMessage)(lpMsg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, TRUE);
				return 0;
			}
		}
	}

	// v2.04.34: when res==0 no message is extracted from the queue, but lpMsg can point to the last valid message
	// so that a loop of failed extractions from queue would keep fixing the last message coordinates. 
	// Useless though apparently not harmful, a if(res) condition was added here.
	if(res && dxw.GethWnd() && dxw.MustFixCoordinates){
		POINT point;
		//res=(*pGetCursorPos)(&point); // can't do this. Why?
		point = lpMsg->pt;

		point=dxw.ScreenToClient(point);
		point=dxw.FixCursorPos(point);
		OutTraceC("%s: FIXED pos=(%d,%d)->(%d,%d)\n", api, lpMsg->pt.x, lpMsg->pt.y, point.x, point.y);
		lpMsg->pt = point;
		if(dxw.dwFlags12 & FIXMOUSELPARAM) FixMessageLParam(ApiRef, lpMsg, &point);
	}

	// perform hot keys processing on messages that would be deleted otherwise ...
	if( wRemoveMsg && (dxw.dwFlags4 & ENABLEHOTKEYS)) {
		switch(lpMsg->message){
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				OutTraceGDI("%s: event %s wparam=%#x lparam=%#x\n", 
					api, (lpMsg->message==WM_SYSKEYDOWN)?"WM_SYSKEYDOWN":"WM_KEYDOWN", lpMsg->wParam, lpMsg->lParam);		
				HandleHotKeys(hwnd, lpMsg->message, lpMsg->lParam, lpMsg->wParam);
				break;
		}
	}

	GetHookInfo()->MessageX=(short)lpMsg->pt.x;
	GetHookInfo()->MessageY=(short)lpMsg->pt.y;

	if(dxw.dwFlags1 & SLOWDOWN) dxw.DoSlow(1);

	return res;
}

BOOL WINAPI extPeekMessageA(LPMSG lpMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{ ApiName("PeekMessageA"); return extPeekMessage(ApiRef, pPeekMessageA, lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg); }
BOOL WINAPI extPeekMessageW(LPMSG lpMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{ ApiName("PeekMessageW"); return extPeekMessage(ApiRef, pPeekMessageW, lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg); }

static BOOL WINAPI extGetMessage(ApiArg, GetMessage_Type pGetMessage, BOOL isAnsi, LPMSG lpMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax){
	BOOL res;
	POINT point;

	// v2.06.14: used in several racing games of "NASCAR Revolution" series.
	if(dxw.dwFlags20 & GETALLMESSAGES) {
		if(hwnd) {
			OutTraceW("%s: forcing hwnd=NULL\n", ApiRef);
			hwnd = NULL;
		}
	}

	//res=(*pGetMessage)(lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax);
	while(TRUE) {
		PeekMessage_Type pPeekMessage = (isAnsi) ? pPeekMessageA : pPeekMessageW;
		if((*pPeekMessage)(lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE)) {
			res = (lpMsg->message == WM_QUIT) ? 0 : 1;
			break;
		}
		if(MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT) == WAIT_TIMEOUT) {
			lpMsg->hwnd = 0;
			lpMsg->message = 0;
			res = 1;
			break;
		}
	}

	OutTraceW("%s: lpmsg=%#x hwnd=%#x filter=(%#x-%#x) msg=%#x(%s) wparam=%#x, lparam=%#x pt=(%d,%d) res=%#x\n", 
		ApiRef, lpMsg, lpMsg->hwnd, wMsgFilterMin, wMsgFilterMax, 
		lpMsg->message, ExplainWinMessage(lpMsg->message & 0xFFFF), 
		lpMsg->wParam, lpMsg->lParam, lpMsg->pt.x, lpMsg->pt.y, res);

	//if(!FixWindowProc(api, lpMsg->hwnd, lpMsg->message, &(lpMsg->wParam), &(lpMsg->lParam), &hres)) return FALSE;

	if((dxw.dwFlags16 & CONFIRMESCAPE) &&
		((lpMsg->message == WM_KEYDOWN) || (lpMsg->message == WM_KEYUP))
		){
		if(lpMsg->wParam == VK_ESCAPE){
			OutTrace("ESC detected in %s:%s\n", ApiRef, lpMsg->message == WM_KEYDOWN ? "DOWN" : "UP");
			BOOL ConfirmEscape = FALSE;
			if(lpMsg->message == WM_KEYDOWN){
				if(MessageBox(0, "Do you confirm the ESC key?", "DxWnd", MB_OKCANCEL) != IDOK) {
					OutTrace("ESC suppressed\n");
					lpMsg->message = 0;
					lpMsg->lParam = 0;
					lpMsg->wParam = 0;
					return 1;
				}
			}
		}
	}

	if((dxw.dwFlags11 & REMAPNUMKEYPAD) &&
		((lpMsg->message == WM_KEYDOWN) || (lpMsg->message == WM_KEYUP))
		){
		switch(lpMsg->wParam){
			case VK_INSERT:	lpMsg->wParam = VK_NUMPAD0; break;
			case VK_END:	lpMsg->wParam = VK_NUMPAD1; break;
			case VK_DOWN:	lpMsg->wParam = VK_NUMPAD2; break;
			case VK_NEXT:	lpMsg->wParam = VK_NUMPAD3; break;
			case VK_LEFT:	lpMsg->wParam = VK_NUMPAD4; break;
			case VK_RIGHT:	lpMsg->wParam = VK_NUMPAD6; break;
			case VK_HOME:	lpMsg->wParam = VK_NUMPAD7; break;
			case VK_UP:		lpMsg->wParam = VK_NUMPAD8; break;
			case VK_PRIOR:	lpMsg->wParam = VK_NUMPAD9; break;
		}
	}

	if(dxw.MustFixMouse){
		point = lpMsg->pt;
		point=dxw.ScreenToClient(point);
		point=dxw.FixCursorPos(point);
		OutTraceC("%s: FIXED pos=(%d,%d)->(%d,%d)\n", ApiRef, lpMsg->pt.x, lpMsg->pt.y, point.x, point.y);
		lpMsg->pt = point;
		if(dxw.dwFlags12 & FIXMOUSELPARAM) FixMessageLParam(ApiRef, lpMsg, &point);
	}

	if (lpMsg->message == WM_SYSKEYDOWN) {
		// v2.05.11 fix: added here to manage hot keys & Alt-F4 in GetMessage loops. Fixes "Road Rash".
		if ((dxw.dwFlags1 & HANDLEALTF4) && (lpMsg->wParam == VK_F4)) {
			OutTraceGDI("%s: WM_SYSKEYDOWN(ALT-F4) - terminating process\n", ApiRef);
			TerminateProcess(GetCurrentProcess(),0);
		}

		if (dxw.dwFlags4 & ENABLEHOTKEYS){
			switch(lpMsg->message){
				case WM_SYSKEYDOWN:
				case WM_KEYDOWN:
						OutTraceDW("%s: event %s wparam=%#x lparam=%#x\n", 
							ApiRef, (lpMsg->message==WM_SYSKEYDOWN)?"WM_SYSKEYDOWN":"WM_KEYDOWN", lpMsg->wParam, lpMsg->lParam);		
						HandleHotKeys(hwnd, lpMsg->message, lpMsg->lParam, lpMsg->wParam);
					break;
			}
		}
	}

	GetHookInfo()->MessageX=(short)lpMsg->pt.x;
	GetHookInfo()->MessageY=(short)lpMsg->pt.y;

	return res;
}

BOOL WINAPI extGetMessageA(LPMSG lpMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{ ApiName("GetMessageA"); return extGetMessage(ApiRef, pGetMessageA, TRUE, lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax); }
BOOL WINAPI extGetMessageW(LPMSG lpMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{ ApiName("GetMessageW"); return extGetMessage(ApiRef, pGetMessageW, FALSE, lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax); }

BOOL WINAPI extPostMessage(char *api, PostMessage_Type pPostMessage, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL res;
	res = (*pPostMessage)(hWnd, Msg, wParam, lParam);
	OutTraceW("%s: hwnd=%#x msg=%#x(%s) wparam=%#x, lparam=%#x res=%#x\n", 
		api, hWnd, Msg, ExplainWinMessage(Msg), wParam, lParam, res);
	return res;
}

BOOL WINAPI extPostMessageA(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ ApiName("PostMessageA"); return extPostMessage(ApiRef, pPostMessageA, hwnd, Msg, wParam, lParam); }
BOOL WINAPI extPostMessageW(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ ApiName("PostMessageW"); return extPostMessage(ApiRef, pPostMessageW, hwnd, Msg, wParam, lParam); }

BOOL WINAPI extClientToScreen(HWND hwnd, LPPOINT lppoint)
{
	// v2.02.10: fully revised to handle scaled windows
	BOOL res;
	ApiName("ClientToScreen");

	OutDebugGDI("%s: hwnd=%#x hWnd=%#x FullScreen=%#x point=(%d,%d)\n", 
		ApiRef, hwnd, dxw.GethWnd(), dxw.IsFullScreen(), lppoint->x, lppoint->y);
	if (lppoint && dxw.IsFullScreen()){
		// optimization: in fullscreen mode, coordinate conversion for the desktop window 
		// should always keep the same values inaltered
		if(hwnd != dxw.GethWnd()){
			*lppoint = dxw.AddCoordinates(*lppoint, dxw.ClientOffset(hwnd));
		}
		OutDebugDW("%s: FIXED point=(%d,%d)\n", ApiRef, lppoint->x, lppoint->y);
		res=TRUE;
	}
	else {
		res=(*pClientToScreen)(hwnd, lppoint);
	}
	return res;
}

BOOL WINAPI extScreenToClient(HWND hwnd, LPPOINT lppoint)
{
	// v2.02.10: fully revised to handle scaled windows
	BOOL res;
	ApiName("ScreenToClient");

	OutDebugGDI("%s: hwnd=%#x hWnd=%#x FullScreen=%#x point=(%d,%d)\n", 
		ApiRef, hwnd, dxw.GethWnd(), dxw.IsFullScreen(), lppoint->x, lppoint->y);

	if (lppoint && (lppoint->x == -32000) && (lppoint->y == -32000)) return 1;

	if (lppoint && dxw.IsFullScreen()){
		// optimization: in fullscreen mode, coordinate conversion for the desktop window 
		// should always keep the same values inaltered
		if(hwnd != dxw.GethWnd()){
			*lppoint = dxw.SubCoordinates(*lppoint, dxw.ClientOffset(hwnd));
			OutDebugDW("%s: FIXED point=(%d,%d)\n", ApiRef, lppoint->x, lppoint->y);
		}
		res=TRUE;
	}
	else {
		res=(*pScreenToClient)(hwnd, lppoint);
	}
	OutDebugGDI("%s: returned point=(%d,%d)\n", ApiRef, lppoint->x, lppoint->y);
	return res;
}

BOOL WINAPI extGetClientRect(HWND hwnd, LPRECT lpRect)
{
	BOOL ret;
	ApiName("GetClientRect");

	OutDebugDW("%s: hwnd=%#x FullScreen=%#x\n", ApiRef, hwnd, dxw.IsFullScreen());

	if(!lpRect) return 0; 
	// v2.04.30: fix for Avernum3 - when GetWindowRect is called before main window hWnd is set,
	// simply return the emulated desktop size.
	if(dxw.IsDesktop(hwnd) && dxw.IsFullScreen()){
		*lpRect = dxw.GetScreenRect();
		OutDebugDW("%s: virtual desktop rect=(%d,%d)-(%d,%d) @%d\n", 
			ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, __LINE__);
		return TRUE;
	}

	// proxed call
	ret=(*pGetClientRect)(hwnd, lpRect);
	if(!ret) {
		OutErrorGDI("%s: ERROR hwnd=%#x err=%d @%d\n", ApiRef, hwnd, GetLastError(), __LINE__);
		return ret;
	}
	OutDebugDW("%s: actual rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

	if (dxw.IsDesktop(hwnd)){
		*lpRect = dxw.GetScreenRect();
		OutDebugDW("%s: desktop rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	else 
	if (dxw.IsFullScreen()){
		*lpRect=dxw.GetClientRect(*lpRect);
		OutDebugDW("%s: fixed rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	return ret;
}

BOOL WINAPI extGetWindowRect(HWND hwnd, LPRECT lpRect)
{
	BOOL ret;
	ApiName("GetWindowRect");

	OutDebugDW("%s: hwnd=%#x hWnd=%#x FullScreen=%#x\n", ApiRef, hwnd, dxw.GethWnd(), dxw.IsFullScreen());

	if(!lpRect) return 0; 
	// v2.04.30: fix for Avernum3 - when GetWindowRect is called before main window hWnd is set,
	// simply return the emulated desktop size.
	if(dxw.IsDesktop(hwnd) && dxw.IsFullScreen()){
		*lpRect = dxw.GetScreenRect();
		OutDebugDW("%s: virtual desktop rect=(%d,%d)-(%d,%d) @%d\n", 
			ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, __LINE__);
		return TRUE;
	}

	if(dxw.IsRealDesktop(hwnd)) {
		// v2.03.52, v2.03.61: fix for "Storm Angel" and "Geneforge" :
		// replace the real desktop with the virtual one only if that doesn't cause troubles.
		HWND hwnd_try = dxw.GethWnd();
		if ((*pGetWindowRect)(hwnd_try, lpRect)) hwnd = hwnd_try; // v2.05.78: fix by hdc0
	}

	ret=(*pGetWindowRect)(hwnd, lpRect);
	if(!ret) {
		OutErrorGDI("%s: GetWindowRect hwnd=%#x error %d @%d\n", ApiRef, hwnd, GetLastError(), __LINE__);
		return ret;
	}
	OutDebugDW("%s: rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	
	// minimized windows behaviour
	if((lpRect->left == -32000)||(lpRect->top == -32000)) return ret;

	if (dxw.IsDesktop(hwnd)){
		// to avoid keeping track of window frame
		*lpRect = dxw.GetScreenRect();
		OutDebugDW("%s: desktop rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	else
	if (dxw.IsFullScreen()){
		*lpRect=dxw.GetWindowRect(*lpRect);

		// Diablo fix: it retrieves coordinates for the explorer window, that are as big as the real desktop!!!
		if(lpRect->left < 0) lpRect->left=0;
//		if(lpRect->left > (LONG)dxw.GetScreenWidth()) lpRect->left=dxw.GetScreenWidth();
//		if(lpRect->right < 0) lpRect->right=0;
		if(lpRect->right > (LONG)dxw.GetScreenWidth()) lpRect->right=dxw.GetScreenWidth();
		if(lpRect->top < 0) lpRect->top=0;
//		if(lpRect->top > (LONG)dxw.GetScreenHeight()) lpRect->top=dxw.GetScreenHeight();
//		if(lpRect->bottom < 0) lpRect->bottom=0;
		if(lpRect->bottom > (LONG)dxw.GetScreenHeight()) lpRect->bottom=dxw.GetScreenHeight();

		OutDebugDW("%s: fixed rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}

	return ret;
}


int WINAPI extMapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints)
{
	UINT pi;
	int ret;
	ApiName("MapWindowPoints");
	// a rarely used API, but responsible for a painful headache: needs hooking for "Commandos 2", "Alien Nations".
	// used also in "Full Pipe" activemovie
	// used also in "NBA Live 99" menu screen	

	OutTraceGDI("%s: hWndFrom=%#x%s hWndTo=%#x%s cPoints=%d FullScreen=%#x\n", 
		ApiRef, hWndFrom, dxw.IsDesktop(hWndFrom)?"(DESKTOP)":"",
		hWndTo, dxw.IsDesktop(hWndTo)?"(DESKTOP)":"",
		cPoints, dxw.IsFullScreen());

#ifndef DXW_NOTRACES
	if(IsDebugGDI){
		for(pi=0; pi<cPoints; pi++) OutTrace("> point[%d]=(%d,%d)\n", pi, lpPoints[pi].x, lpPoints[pi].y);
	}
#endif

	if(dxw.IsFullScreen()){
		if(dxw.IsRealDesktop(hWndTo)) hWndTo=dxw.GethWnd();
		if(dxw.IsRealDesktop(hWndFrom)) hWndFrom=dxw.GethWnd();
		if(hWndFrom == hWndTo){
			// desktop to desktop: don't change anything
			OutTraceGDI("%s: ret=0 (0,0)\n", ApiRef);
			return 0;
		}
	}
	
	ret=(*pMapWindowPoints)(hWndFrom, hWndTo, lpPoints, cPoints);
	// v2.03.16: now must scale every point (fixes "NBA Live 99")
	// v2.03.18: in some cases it should not! "New Your Race"...
	// v2.03.56: scale only on scaled modes
	switch(dxw.GDIEmulationMode){
		case GDIMODE_SHAREDDC:
		case GDIMODE_EMULATED:
		default:
			break;
		case GDIMODE_STRETCHED:
			for(pi=0; pi<cPoints; pi++){
				dxw.UnmapClient(&lpPoints[pi]);
			}			
#ifndef DXW_NOTRACES
			if(IsDebugDW){
				OutTrace("Mapped points: ");
				for(pi=0; pi<cPoints; pi++) OutTrace("(%d,%d)", lpPoints[pi].x, lpPoints[pi].y);
				OutTrace("\n");
			}
#endif
			break;
	}

	// If the function succeeds, the low-order word of the return value is the number of pixels 
	// added to the horizontal coordinate of each source point in order to compute the horizontal 
	// coordinate of each destination point. (In addition to that, if precisely one of hWndFrom 
	// and hWndTo is mirrored, then each resulting horizontal coordinate is multiplied by -1.) 
	// The high-order word is the number of pixels added to the vertical coordinate of each source
	// point in order to compute the vertical coordinate of each destination point.

	OutTraceGDI("%s: ret=%#x (%d,%d)\n", 
		ApiRef, ret, (short)((ret&0xFFFF0000)>>16), (short)(ret&0x0000FFFF));
	return ret;
}

HWND WINAPI extGetDesktopWindow(void)
{
	HWND res;
	ApiName("GetDesktopWindow");

	if((!dxw.Windowize) || (dxw.dwFlags5 & DIABLOTWEAK)) {
		HWND ret;
		ret = (*pGetDesktopWindow)();
		OutTraceGDI("%s: BYPASS ret=%#x\n", ApiRef, ret);
		return ret;
	}

	OutTraceGDI("%s: FullScreen=%#x\n", ApiRef, dxw.IsFullScreen());
	// v2.04.01.fx4: do not return the main window if we still don't have one (dxw.GethWnd() == NULL)
	if (dxw.IsFullScreen() && dxw.GethWnd()){ 
		OutTraceGDI("%s: returning main window hwnd=%#x\n", ApiRef, dxw.GethWnd());
		return dxw.GethWnd();
	}
	else{
		res=(*pGetDesktopWindow)();
		OutTraceGDI("%s: returning desktop window hwnd=%#x\n", ApiRef, res);
		return res;
	}
}

int WINAPI extGetSystemMetrics(int nindex)
{
	HRESULT res;
	ApiName("GetSystemMetrics");

	res=(*pGetSystemMetrics)(nindex);
	OutTraceGDI("%s: index=%#x(%s), res=%d\n", ApiRef, nindex, ExplainsSystemMetrics(nindex), res);

	switch(nindex){
		case SM_CXFULLSCREEN:
		case SM_CXSCREEN:
		case SM_CXVIRTUALSCREEN: // v2.02.31
			if(dxw.isScaled){
				res= dxw.GetScreenWidth();
				OutTraceGDI("%s: fix %s=%d\n", ApiRef, ExplainsSystemMetrics(nindex), res);
			}
			break;
		case SM_CYFULLSCREEN:
		case SM_CYSCREEN:
		case SM_CYVIRTUALSCREEN: // v2.02.31
			if(dxw.isScaled){
				res= dxw.GetScreenHeight();
				OutTraceGDI("%s: fix %s=%d\n", ApiRef, ExplainsSystemMetrics(nindex), res);
			}
			break;
		case SM_CMONITORS:
			if((dxw.dwFlags2 & HIDEMULTIMONITOR) && res>1) {
				res=1;
				OutTraceGDI("%s: fix SM_CMONITORS=%d\n", ApiRef, res);
			}
			break;
		// v2.05.68 fix: simulate positive result to SM_MIDEASTENABLED and SM_DBCSENABLED if CUSTOMLOCALE is set.
		// fixes "Dragon Fantasy II".
		case SM_MIDEASTENABLED:
			if(dxw.dwFlags11 & CUSTOMLOCALE) {
				res=1;
				OutTraceGDI("%s: fix SM_MIDEASTENABLED=%d\n", ApiRef, res);
			}
			break;
		case SM_DBCSENABLED:
			if(dxw.dwFlags11 & CUSTOMLOCALE) {
				res=1;
				OutTraceGDI("%s: fix SM_DBCSENABLED=%d\n", ApiRef, res);
			}
			break;
	}
	return res;
}

ATOM WINAPI extRegisterClassExA(WNDCLASSEXA *lpwcx)
{
	ATOM ret;
	ApiName("RegisterClassExA");

#ifndef DXW_NOTRACES
	char sStyle[256];
	OutTraceGDI("%s: ClassName=\"%s\" style=%#x(%s) WndProc=%#x cbClsExtra=%d cbWndExtra=%d hInstance=%#x\n", 
		ApiRef, lpwcx->lpszClassName, lpwcx->style, ExplainStyle(lpwcx->style, sStyle, 256), lpwcx->lpfnWndProc, lpwcx->cbClsExtra, lpwcx->cbWndExtra, lpwcx->hInstance);
#endif // DXW_NOTRACES

	if ((dxw.dwFlags11 & CUSTOMLOCALE) && 
		(dxw.dwFlags12 & CLASSLOCALE) && 
		pRegisterClassExW){
		LPWSTR lpClassNameW;
		WNDCLASSEXW wcw;
		memcpy(&wcw, lpwcx, sizeof(wcw));
		BOOL IsStringClass = ((DWORD)lpwcx->lpszClassName & WM_CLASSMASK);
		lpClassNameW = IsStringClass ? (LPWSTR)malloc((strlen(lpwcx->lpszClassName) + 1) * 2) : (LPWSTR)lpwcx->lpszClassName;
		if(IsStringClass) {
			int size = strlen(lpwcx->lpszClassName);
			int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpwcx->lpszClassName, size, lpClassNameW, size);
			lpClassNameW[n] = L'\0'; // make tail ! 
			OutTraceLOC("%s: WIDE class=\"%ls\"\n", ApiRef, lpClassNameW);
		}
		wcw.lpszClassName = lpClassNameW;
		ret = (*pRegisterClassExW)(&wcw);
		if(IsStringClass) free(lpClassNameW);
	}
	else {
		ret = (*pRegisterClassExA)(lpwcx);
	}

	if(ret) {
		OutTraceGDI("%s: atom=%#x\n", ApiRef, ret);
	}
	else {
		OutErrorGDI("%s: ERROR atom=NULL err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

ATOM WINAPI extRegisterClassA(WNDCLASSA *lpwcx)
{
	ATOM ret;
	ApiName("RegisterClassA");

	// referenced by Syberia, together with RegisterClassExA
#ifndef DXW_NOTRACES
	char sStyle[256];
	OutTraceGDI("%s: ClassName=\"%s\" style=%#x(%s) WndProc=%#x cbClsExtra=%d cbWndExtra=%d hInstance=%#x\n", 
		ApiRef, lpwcx->lpszClassName, lpwcx->style, ExplainStyle(lpwcx->style, sStyle, 256), lpwcx->lpfnWndProc, lpwcx->cbClsExtra, lpwcx->cbWndExtra, lpwcx->hInstance);
#endif // DXW_NOTRACES

	if ((dxw.dwFlags11 & CUSTOMLOCALE) && 
		(dxw.dwFlags12 & CLASSLOCALE) &&
		!(dxw.dwFlags14 & NOSETTEXT)){
		LPWSTR lpClassNameW;
		WNDCLASSW wcw;
		if(!pRegisterClassW) pRegisterClassW = RegisterClassW;
		memcpy(&wcw, lpwcx, sizeof(wcw));
		BOOL IsStringClass = ((DWORD)lpwcx->lpszClassName & WM_CLASSMASK);
		lpClassNameW = IsStringClass ? (LPWSTR)malloc((strlen(lpwcx->lpszClassName) + 1) * 2) : (LPWSTR)lpwcx->lpszClassName;
		if(IsStringClass) {
			int size = strlen(lpwcx->lpszClassName);
			int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpwcx->lpszClassName, size, lpClassNameW, size);
			lpClassNameW[n] = L'\0'; // make tail ! 
			OutTraceLOC("%s: WIDE class=\"%ls\"\n", ApiRef, lpClassNameW);
		}
		wcw.lpszClassName = lpClassNameW;
		ret = (*pRegisterClassW)(&wcw);
		if(IsStringClass) free(lpClassNameW);
	}
	else {
		ret = (*pRegisterClassA)(lpwcx);
	}

	if(ret) {
		OutTraceGDI("%s: atom=%#x\n", ApiRef, ret);
	}
	else {
		OutErrorGDI("%s: ERROR atom=NULL err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

ATOM WINAPI extRegisterClassExW(WNDCLASSEXW *lpwcx)
{
	ATOM ret;
	ApiName("RegisterClassExW");

#ifndef DXW_NOTRACES
	char sStyle[256];
	OutTraceGDI("%s: ClassName=\"%ls\" style=%#x(%s) WndProc=%#x cbClsExtra=%d cbWndExtra=%d hInstance=%#x\n", 
		ApiRef, lpwcx->lpszClassName, lpwcx->style, ExplainStyle(lpwcx->style, sStyle, 256), lpwcx->lpfnWndProc, lpwcx->cbClsExtra, lpwcx->cbWndExtra, lpwcx->hInstance);
#endif // DXW_NOTRACES

	ret = (*pRegisterClassExW)(lpwcx);
	if(ret) {
		OutTraceGDI("%s: atom=%#x\n", ApiRef, ret);
	}
	else {
		OutErrorGDI("%s: ERROR atom=NULL err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

ATOM WINAPI extRegisterClassW(WNDCLASSW *lpwcx)
{
	ATOM ret;
	ApiName("RegisterClassW");

#ifndef DXW_NOTRACES
	char sStyle[256];
	OutTraceGDI("%s: ClassName=\"%ls\" style=%#x(%s) WndProc=%#x cbClsExtra=%d cbWndExtra=%d hInstance=%#x\n", 
		ApiRef, lpwcx->lpszClassName, lpwcx->style, ExplainStyle(lpwcx->style, sStyle, 256), lpwcx->lpfnWndProc, lpwcx->cbClsExtra, lpwcx->cbWndExtra, lpwcx->hInstance);
#endif // DXW_NOTRACES

	ret = (*pRegisterClassW)(lpwcx);
	if(ret) {
		OutTraceGDI("%s: atom=%#x\n", ApiRef, ret);
	}
	else {
		OutErrorGDI("%s: ERROR atom=NULL err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

int WINAPI extGetClassNameA(HWND hwnd, LPSTR lpClassName, int MaxCount)
{
	int ret;
	ApiName("GetClassNameA");

	OutTraceGDI("%s: hwnd=%#x max=%d\n", ApiRef, hwnd, MaxCount);

#ifndef DXW_NOTRACES
	if(IsDebugGDI){
		char ClassName[MAX_PATH];
		ret = (*pGetClassNameA)(hwnd, ClassName, MAX_PATH);
		OutTrace("%s: FULL class=\"%s\" ret-len=%d\n", ApiRef, ClassName, ret); 
	}
#endif

	if ((dxw.dwFlags11 & CUSTOMLOCALE) &&
		(dxw.dwFlags12 & CLASSLOCALE) &&
		!(dxw.dwFlags14 & NOSETTEXT)){
		// try ANSI first, it could be an ANSI system class
		ret = (*pGetClassNameA)(hwnd, lpClassName, MaxCount);
		if(ret){
			OutTraceLOC("%s: ANSI class=\"%s\"\n", ApiRef, lpClassName);
		}
		else{
			LPWSTR lpClassNameW = (LPWSTR)malloc((MaxCount + 1) * 2);
			ret = GetClassNameW(hwnd, lpClassNameW, MaxCount);
			OutTraceLOC("%s: WIDE class=\"%ls\"\n", lpClassNameW);
			(*pWideCharToMultiByte)(dxw.CodePage, 0, lpClassNameW, MaxCount, lpClassName, MaxCount, NULL, FALSE);
			free(lpClassNameW);
		}
	}
	else {
		ret = (*pGetClassNameA)(hwnd, lpClassName, MaxCount);
	}
	if(ret){
		OutTraceGDI("%s: class=\"%s\" ret-len=%d\n", ApiRef, lpClassName, ret); 
	}
	else{
		OutErrorGDI("%s: err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

BOOL WINAPI extGetClassInfoA(HINSTANCE hInst, LPCSTR lpClassName, LPWNDCLASSA lpWndClass)
{
	BOOL ret;
	ApiName("GetClassInfoA");

	if((DWORD)lpClassName & WM_CLASSMASK) {
		OutTraceGDI("%s: hinst=%#x class=\"%s\"\n", ApiRef, hInst, lpClassName);
	}
	else {
		OutTraceGDI("%s: hinst=%#x class=%#x\n", ApiRef, hInst, lpClassName);
	}

	if ((dxw.dwFlags11 & CUSTOMLOCALE) && 
		(dxw.dwFlags12 & CLASSLOCALE) && 
		(hInst != 0)){
		// try ANSI first, it could be an ANSI system class
		ret = (*pGetClassInfoA)(hInst, lpClassName, lpWndClass);
		if(ret){
			OutTraceLOC("%s: ANSI class=\"%s\"\n", ApiRef, lpClassName);
		}
		else{
			WNDCLASSW WndClassW;
			LPWSTR lpClassNameW;
			BOOL IsStringClass = ((DWORD)lpClassName & WM_CLASSMASK);
			lpClassNameW = IsStringClass ? (LPWSTR)malloc((strlen(lpClassName) + 1) * 2) : (LPWSTR)lpClassName;
			if(IsStringClass) {
				int size = strlen(lpClassName);
				int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpClassName, size, lpClassNameW, size);
				lpClassNameW[n] = L'\0'; // make tail ! 
				OutTraceLOC("%s: WIDE class=\"%ls\"\n", ApiRef, lpClassNameW);
			}
			ret = GetClassInfoW(hInst, lpClassNameW, &WndClassW);
			if(ret){
				memcpy(lpWndClass, &WndClassW, sizeof(WNDCLASSA));
				// v2.06.03: convert WIDE results to ANSI
				IsStringClass = ((DWORD)WndClassW.lpszClassName & WM_CLASSMASK);
				lpWndClass->lpszClassName = IsStringClass ? (LPCSTR)malloc((wcslen(WndClassW.lpszClassName) * 2) + 1) : (LPCSTR)WndClassW.lpszClassName;
				if(IsStringClass) {
					int size = wcslen(WndClassW.lpszClassName);
					(*pWideCharToMultiByte)(dxw.CodePage, 0, WndClassW.lpszClassName, size+1, (LPSTR)(lpWndClass->lpszClassName), size+1, NULL, NULL);
				}
				lpWndClass->lpszClassName = (LPCSTR)malloc(strlen(lpClassName)+1); // no good
				strcpy((LPSTR)(lpWndClass->lpszClassName), lpClassName);
				lpWndClass->lpszClassName = lpClassName;
				lpWndClass->lpszMenuName = NULL; // no good
				if(lpWndClass->lpfnWndProc == DefWindowProcW) lpWndClass->lpfnWndProc = pDefWindowProcA;
				lpWndClass->lpfnWndProc = pDefWindowProcA;
			}
			if(IsStringClass) free(lpClassNameW);
		}
	}
	else {
		ret = (*pGetClassInfoA)(hInst, lpClassName, lpWndClass);
	}
	if(ret){
		// beware: "Piskworky 2001" makes a comparison on the returned lpfnWndProc field!!
		OutTraceGDI("wndproc> %#x\n", lpWndClass->lpfnWndProc); 
		OutTraceGDI("style> %#x\n", lpWndClass->style); 
		OutTraceGDI("hicon> %#x\n", lpWndClass->hIcon); 
		OutTraceGDI("hcurs> %#x\n", lpWndClass->hCursor); 
		OutTraceGDI("hback> %#x\n", lpWndClass->hbrBackground); 
		OutTraceGDI("class> %s\n", lpWndClass->lpszClassName); 
		OutTraceGDI("menu> %s\n", lpWndClass->lpszMenuName); 
		OutTraceGDI("%s: ret=%#x\n", ApiRef, ret); 
	}
	else{
		OutErrorGDI("%s: err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

BOOL WINAPI extUnregisterClassA(LPCSTR lpClassName, HINSTANCE hInst)
{
	BOOL ret;
	ApiName("UnregisterClassA");

	if((DWORD)lpClassName & WM_CLASSMASK) {
		OutTraceGDI("%s: hinst=%#x class=\"%s\"\n", ApiRef, hInst, lpClassName);
	}
	else {
		OutTraceGDI("%s: hinst=%#x class=%#x\n", ApiRef, hInst, lpClassName);
	}
	if ((dxw.dwFlags11 & CUSTOMLOCALE) && 
		(dxw.dwFlags12 & CLASSLOCALE) && 
		(hInst != 0)){
		// try ANSI first, it could be an ANSI system class
		ret = (*pUnregisterClassA)(lpClassName, hInst);
		if(ret){
			OutTraceLOC("%s: ANSI class=\"%s\"\n", ApiRef, lpClassName);
		}
		else{
			LPWSTR lpClassNameW;
			BOOL IsStringClass = ((DWORD)lpClassName & WM_CLASSMASK);
			lpClassNameW = IsStringClass ? (LPWSTR)malloc((strlen(lpClassName) + 1) * 2) : (LPWSTR)lpClassName;
			if(IsStringClass) {
				int size = strlen(lpClassName);
				int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpClassName, size, lpClassNameW, size);
				lpClassNameW[n] = L'\0'; // make tail ! 
				OutTraceLOC("%s: WIDE class=\"%ls\"\n", ApiRef, lpClassNameW);
			}
			ret = UnregisterClassW(lpClassNameW, hInst);
		}
	}
	else {
		ret = (*pUnregisterClassA)(lpClassName, hInst);
	}

	if(ret){
		OutTraceGDI("%s: OK\n", ApiRef); 
	}
	else{
		OutErrorGDI("%s: err=%d\n", ApiRef, GetLastError()); 
	}
	return ret;
}

static void HookChildWndProc(HWND hwnd, DWORD dwStyle, LPCSTR ApiName)
{
	// child window inherit the father's windproc, so if it's redirected to
	// a hooker (either extWindowProc or extChildWindowProc) you have to retrieve
	// the correct value (dxwws.GetProc) before saving it (dxwws.PutProc).
	long res;
	WNDPROC pWindowProc;

	if(dxw.dwDFlags2 & NOWINDOWHOOKS) return;

	pWindowProc = (WNDPROC)(*pGetWindowLong)(hwnd, GWL_WNDPROC);

	extern LRESULT CALLBACK dw_Hider_Message_Handler(HWND, UINT, WPARAM, LPARAM);
	if(pWindowProc==dw_Hider_Message_Handler) return;

	if((pWindowProc == extWindowProc) || 
		(pWindowProc == extChildWindowProc) ||
		(pWindowProc == extDialogWindowProc)){ // avoid recursions 
		HWND Father;
		WNDPROC pFatherProc;
		Father=GetParent(hwnd);
		pFatherProc=dxwws.GetProc(Father);
		OutTraceGDI("%s: WndProc=%s father=%#x WndProc=%#x\n", ApiName, 
			(pWindowProc == extWindowProc) ? "extWindowProc" : ((pWindowProc == extChildWindowProc) ? "extChildWindowProc" : "extDialogWindowProc"), 
			Father, pFatherProc);
		pWindowProc = pFatherProc;
	}
	dxwws.PutProc(hwnd, pWindowProc);
	if(dwStyle & WS_CHILD){
		OutTraceGDI("%s: Hooking CHILD hwnd=%#x father WindowProc %#x->%#x\n", ApiName, hwnd, pWindowProc, extChildWindowProc);
		res=(*pSetWindowLong)(hwnd, GWL_WNDPROC, (LONG)extChildWindowProc);
	}
	else { // must be dwStyle & WS_DLGFRAME
		OutTraceGDI("%s: Hooking DLGFRAME hwnd=%#x father WindowProc %#x->%#x\n", ApiName, hwnd, pWindowProc, extDialogWindowProc);
		res=(*pSetWindowLong)(hwnd, GWL_WNDPROC, (LONG)extDialogWindowProc);
	}
#ifndef DXW_NOTRACES
	if(!res) OutErrorGDI("%s: SetWindowLong ERROR %#x\n", ApiName, GetLastError());
#endif // DXW_NOTRACES
}


static HWND hLastFullScrWin = 0;
static DDPIXELFORMAT ddpLastPixelFormat;
#define SAFEWINDOWCREATION TRUE

typedef HWND (WINAPI *CreateWindow_Type)(DWORD, LPVOID, LPVOID, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

static HWND WINAPI CreateWindowCommon(
  LPCSTR api,
  CreateWindow_Type pCreateWindow,
  BOOL bActiveMovie,
  DWORD dwExStyle,
  void *lpClassName,
  void *lpWindowName,
  DWORD dwStyle,
  int x,
  int y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam) 
{
	HWND hwnd;
	BOOL isValidHandle=TRUE;
	BOOL isNewDesktop;
	int origx, origy, origw, origh;
	DWORD origstyle, origexstyle;
	BOOL bShowSpec = FALSE;
	int CmdShow;

	if((dxw.dwFlags14 & NOSETTEXT) && lpWindowName){
		OutTrace("%s: NOSETTEXT\n", ApiRef);
		lpWindowName = NULL;
	}

	// if the main window was not created yet, start the xbox-to-keyboard mapping
	if((dxw.dwFlags17 & XBOX2KEYBOARD) && !dxw.GethWnd()){
		if(!(*pLoadLibraryA)("xbox2kbd.dll")) 
			OutTrace("%s: ERROR LoadLibrary(xbox2kbd.dll) err=%d\n", ApiRef, GetLastError());
	}

	if(dxw.dwFlags18 & BYPASSACTIVEMOVIE) bActiveMovie = FALSE;

	if(dxw.isColorEmulatedMode){
		if(dxw.dwFlags16 & FORCESIMPLEWINDOW){
			if (!(dwStyle & WS_CHILD)){
				dwStyle &= WS_VISIBLE;
				dwStyle |= WS_OVERLAPPED|WS_POPUP;
				x = y = 0;
				nWidth = (*pGetSystemMetrics)(SM_CXSCREEN);
				nHeight = (*pGetSystemMetrics)(SM_CYSCREEN);
				dwExStyle = WS_EX_TOPMOST;
			}
		}
#ifndef DXW_NOTRACES
		char sStyle[256];
		char sExStyle[256];
		OutDebugDW("%s: CREATION pos=(%d,%d) size=(%d,%d) Style=%#x(%s) ExStyle=%#x(%s)\n",
			ApiRef, x, y, nWidth, nHeight, 
			dwStyle, ExplainStyle(dwStyle, sStyle, 256), 
			dwExStyle, ExplainExStyle(dwExStyle, sExStyle, 256));
#endif // DXW_NOTRACES
		hwnd = (*pCreateWindow)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		OutTraceGDI("%s: ret(hwnd)=%#x\n", ApiRef, hwnd);
		return hwnd;
	}

	origx = x;
	origy = y;
	origw = nWidth;
	origh = nHeight;
	origstyle = dwStyle;
	origexstyle = dwExStyle;

	if(!dxw.isScaled || (hWndParent == HWND_MESSAGE)){ // v2.02.87: don't process message windows (hWndParent == HWND_MESSAGE)
		// v2.04.73: also in fullscreen mode add WS_CLIPCHILDREN if asked to. Fixes "Psychotoxic".
		if((dxw.dwFlags4 & FORCECLIPCHILDREN) && dxw.IsRealDesktop(hWndParent)) {
			OutTraceGDI("%s: fixed style +WS_CLIPCHILDREN\n", ApiRef);
			dwStyle |= WS_CLIPCHILDREN;
			}
		hwnd = (*pCreateWindow)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		if ((dxw.dwFlags1 & HOOKCHILDWIN) && (dwStyle & (WS_CHILD|WS_DLGFRAME)))
			HookChildWndProc(hwnd, dwStyle, ApiRef);

		OutTraceGDI("%s: ret=%#x\n", ApiRef, hwnd);
		return hwnd;
	}

	OutDebugDW("%s: ActiveMovie=%#x\n", ApiRef, bActiveMovie);
	if(bActiveMovie && dxw.isScaled){
		if(dxw.IsRealDesktop(hWndParent)){
			if(dxw.GethWnd()){
				OutTraceGDI("%s: on ActiveMovie Window FIX parent=%#x->%#x\n", ApiRef, hWndParent, dxw.GethWnd());
				hWndParent = dxw.GethWnd();

				// v2.04.90: beware: turning WS_DLGFRAME off seems to cause deafness on listening the ESC key to
				// interrupt the movie in "War Times".
				//dwStyle &= ~(WS_BORDER|WS_CAPTION|WS_DLGFRAME|WS_MAXIMIZEBOX|WS_MINIMIZEBOX|WS_SIZEBOX|WS_THICKFRAME);
				dwStyle &= ~(WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_MAXIMIZEBOX|WS_MINIMIZEBOX|WS_SIZEBOX|WS_THICKFRAME);
				origstyle = dwStyle;
				char sStyle[256];
				OutTraceGDI("%s: STRETCHMOVIES on ActiveMovie Window style=%#x(%s)\n", ApiRef, dwStyle, ExplainStyle(dwStyle, sStyle, 256));
			} else {
				OutTraceGDI("%s: on ActiveMovie Window no parent!!\n", ApiRef);
			}
		}
		if(dxw.dwFlags6 & STRETCHMOVIES){
			nWidth = dxw.GetScreenWidth();
			nHeight = dxw.GetScreenHeight();
			x = 0;
			y = 0;
			OutTraceGDI("%s: STRETCHMOVIES on ActiveMovie Window unmapped size=(%dx%d)\n", ApiRef, nWidth, nHeight);
		}
	}

	// good for all modes
	if(dxw.dwFlags5 & UNLOCKZORDER)  dwExStyle &= ~WS_EX_TOPMOST ;
	if(dxw.dwFlags9 & LOCKTOPZORDER) dwExStyle |=  WS_EX_TOPMOST ;

	// no maximized windows in any case
	if (dxw.dwFlags1 & PREVENTMAXIMIZE){
		OutTraceGDI("%s: handling PREVENTMAXIMIZE mode\n", ApiRef);
		dwStyle &= ~WS_MAXIMIZE;
	}

	// v2.1.92: fixes size & position for auxiliary big window, often used
	// for intro movies etc. : needed for ......
	// evidently, this was supposed to be a fullscreen window....
	// v2.1.100: fixes for "The Grinch": this game creates a new main window for OpenGL
	// rendering using CW_USEDEFAULT placement and 800x600 size while the previous
	// main win was 640x480 only!
	// v2.02.13: if it's a WS_CHILD window, don't reposition the x,y, placement for BIG win.
	// v2.02.30: fix (Fable - lost chapters) Fable creates a bigger win with negative x,y coordinates. 
	// v2.03.53: revised code, logic moved to IsFullscreenWindow

	if((dxw.dwFlags1 & LOCKWINSTYLE) && (dxw.WindowStyle == WSTYLE_MODALSTYLE) && (dwStyle & WS_CAPTION)){
		OutDebugDW("%s: ASSERT suppress WS_CAPTION\n", ApiRef);
		dwStyle &= ~WS_CAPTION;
	}

	if(isNewDesktop=IsFullscreenWindow(dwStyle, dwExStyle, hWndParent, x, y, nWidth, nHeight)){
		OutDebugDW("%s: ASSERT IsFullscreenWindow==TRUE\n", ApiRef);

		// if already in fullscreen mode, save previous settings
		if(dxw.IsFullScreen() && dxw.GethWnd()){
			hLastFullScrWin = dxw.GethWnd();
			ddpLastPixelFormat = dxw.VirtualPixelFormat;
		}

		// inserted some checks here, since the main window could be destroyed
		// or minimized (see "Jedi Outcast") so that you may get a dangerous 
		// zero size. In this case, better renew the hWnd assignement and its coordinates.
		isValidHandle = dxw.IsValidMainWindow();
		// v2.03.58 fix: don't consider CW_USEDEFAULT as a big unsigned integer!! Fixes "Imperialism".
		// v2.04.87 fix: use boolean AND to detect default size, not strict EQUAL operator
		// fix CW_USEDEFAULT coordinates. We haven't scaled yet, so coordinates must be logical

		// from MSDN:
		// X: The initial horizontal position of the window. For an overlapped or pop-up window, the x parameter 
		// is the initial x-coordinate of the window's upper-left corner, in screen coordinates. For a child window, 
		// x is the x-coordinate of the upper-left corner of the window relative to the upper-left corner of the 
		// parent window's client area. If x is set to CW_USEDEFAULT, the system selects the default position for 
		// the window's upper-left corner and ignores the y parameter. CW_USEDEFAULT is valid only for overlapped 
		// windows; if it is specified for a pop-up or child window, the x and y parameters are set to zero.
		//
		// Y: The initial vertical position of the window. For an overlapped or pop-up window, the y parameter is 
		// the initial y-coordinate of the window's upper-left corner, in screen coordinates. For a child window, 
		// y is the initial y-coordinate of the upper-left corner of the child window relative to the upper-left 
		// corner of the parent window's client area. For a list box y is the initial y-coordinate of the upper-left 
		// corner of the list box's client area relative to the upper-left corner of the parent window's client area.
		//
		// If an overlapped window is created with the WS_VISIBLE style bit set and the x parameter is set to 
		// CW_USEDEFAULT, then the y parameter determines how the window is shown. If the y parameter is CW_USEDEFAULT, 
		// then the window manager calls ShowWindow with the SW_SHOW flag after the window has been created. 
		// If the y parameter is some other value, then the window manager calls ShowWindow with that value as the 
		// nCmdShow parameter.

		if(nWidth & CW_USEDEFAULT) nWidth = dxw.GetScreenWidth();
		if(nHeight & CW_USEDEFAULT) nHeight = dxw.GetScreenHeight();
		if(x & CW_USEDEFAULT) {
			if(dwStyle & WS_VISIBLE){
				bShowSpec = TRUE;
				if (y & CW_USEDEFAULT) CmdShow = SW_SHOW;
				else CmdShow = y;
				if(CmdShow == SW_SHOWMAXIMIZED) CmdShow = SW_SHOWNORMAL;
				OutTraceGDI("%s: VISIBLE win CmdShow=%#x(%s)\n", ApiRef, CmdShow, ExplainShowCmd(CmdShow));
			}
			x = 0;
			y = 0; // y value is ignored
		}
		if(y & CW_USEDEFAULT) y = 0;

		OutTraceGDI("%s: unmapped win pos=(%d,%d) size=(%d,%d) valid=%#x\n", 
			ApiRef, x, y, nWidth, nHeight, isValidHandle);

		// declare fullscreen mode
		dxw.SetFullScreen(TRUE);
		// update virtual screen size if it has grown 
		// v2.04.96: do that ONLY on outer window creations!!
		if(!InMainWinCreation) dxw.SetScreenSize(nWidth, nHeight);
	}

	if(!dxw.IsFullScreen()){ // v2.1.63: needed for "Monster Truck Madness"
		hwnd= (*pCreateWindow)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		if(hwnd){
			OutTraceGDI("%s: windowed mode ret=%#x\n", ApiRef, hwnd);
		}
		else {
			OutTraceGDI("%s: windowed mode ERROR err=%d\n", ApiRef, GetLastError());
		}
		return hwnd;
	}

	// from here on, fullscreen is garanteed

	if(dxw.dwFlags16 & TRIMMAINWINDOW){ // required for "Need for Speed SE"
		if(x < 0) x = 0;
		if(y < 0) y = 0;
		if(nWidth > (int)dxw.GetScreenWidth()) nWidth = dxw.GetScreenWidth();
		if(nHeight > (int)dxw.GetScreenHeight()) nHeight = dxw.GetScreenHeight();
	}

	if (IsRelativePosition(dwStyle, dwExStyle, hWndParent)){
		dxw.MapClient(&x, &y, &nWidth, &nHeight);
		OutTraceGDI("%s: fixed RELATIVE pos=(%d,%d) size=(%d,%d)\n", ApiRef, x, y, nWidth, nHeight);
	}
	else {
		dxw.MapWindow(&x, &y, &nWidth, &nHeight);
		OutTraceGDI("%s: fixed ABSOLUTE pos=(%d,%d) size=(%d,%d)\n", ApiRef, x, y, nWidth, nHeight);
	}

	if(dxw.dwFlags4 & FORCECLIPCHILDREN) {
		// v2.04.73: in window mode add WS_CLIPCHILDREN if asked to. Fixes "Psychotoxic".
		OutTraceGDI("%s: fixed style +WS_CLIPCHILDREN\n", ApiRef);
		dwStyle |= WS_CLIPCHILDREN;
		origstyle |= WS_CLIPCHILDREN;
	}

	// if it is a new main window, give it proper style, unless
	// a) it is a child window
	// b) it is a movie window
	// n.b. the WSTYLE_DEFAULT mode is handles internally by FixWinStyle and FixWinExStyle
	if((!isValidHandle) && isNewDesktop && !(
		(dwStyle & WS_CHILD) ||						// a)
		bActiveMovie)								// b)
		){
		OutTraceGDI("%s: set main win style\n", ApiRef); 
		dwStyle = dxw.FixWinStyle(dwStyle);
		dwExStyle = dxw.FixWinExStyle(dwExStyle); 
	}

#ifndef DXW_NOTRACES
	char sStyle[256];
	char sExStyle[256];
	OutDebugDW("%s: CREATION pos=(%d,%d) size=(%d,%d) Style=%#x(%s) ExStyle=%#x(%s)\n",
		ApiRef, origx, origy, origw, origh, 
		origstyle, ExplainStyle(origstyle, sStyle, 256), 
		origexstyle, ExplainExStyle(origexstyle, sExStyle, 256));
#endif // DXW_NOTRACES

	// v2.04.02: InMainWinCreation semaphore, signals to the CreateWin callback that the window to be created will be a main window,
	// so rules about LOCKWINPOS etc. must be applied. Fixes "Civil War 2 Generals" main window displacement. 
	// v2.04.05: the semaphore must be a counter, since within the CreateWin callback there could be other CreateWin calls.
	// happens in "Warhammer: Shadow of the Horned Rat" !
	// SAFEWINDOWCREATION mode: fixes problems of "Warhammer shadow of the Horned rat", but also allows "Diablo" to run in fake fullscreen high-res mode.
	// this way, any creation callback routine invoked within the window creation will receive only the original call parameters, while the new scaled
	// values and adjusted styles will be applied only after the creation.

	MovedInCallback = FALSE;
	InMainWinCreation++;
	hwnd= (*pCreateWindow)(origexstyle, lpClassName, lpWindowName, origstyle, origx, origy, origw, origh, hWndParent, hMenu, hInstance, lpParam);
	InMainWinCreation--;

	if (hwnd==(HWND)NULL){
#ifndef DXW_NOTRACES
		char sStyle[256];
		OutErrorGDI("%s: ERROR err=%d Style=%#x(%s) ExStyle=%#x\n",
			ApiRef, GetLastError(), dwStyle, ExplainStyle(dwStyle, sStyle, 256), dwExStyle);
#endif // DXW_NOTRACES
		return hwnd;
	}

	// v2.05.78: Barrow Hill patch using STICKYWINDOWS flag
	if ((dxw.dwFlags14 & STICKYWINDOWS) && isValidHandle && isNewDesktop) dxw.hStickyWindow = hwnd;

	// if we have no current valid main window handle, then set it now 
	if ((!isValidHandle) && isNewDesktop) dxw.SethWnd(hwnd);

	// if is a control parent, update the current control parent 
	if (dwExStyle & WS_EX_CONTROLPARENT) dxw.hControlParentWnd = hwnd;

#ifndef DXW_NOTRACES
	OutDebugDW("%s: FIXED pos=(%d,%d) size=(%d,%d) Style=%#x(%s) ExStyle=%#x(%s)\n",
		ApiRef, x, y, nWidth, nHeight, 
		dwStyle, ExplainStyle(dwStyle, sStyle, 256), 
		dwExStyle, ExplainExStyle(dwExStyle, sExStyle, 256));
#endif // DXW_NOTRACES

	// assign final position & style
	// v2.06.11 - unless the window position was fixed in the creation callback. Fixes "Delaware St. John" games.
	if(!MovedInCallback) dxw.FixWindow(hwnd, dwStyle, dwExStyle, x, y, nWidth, nHeight);
	MovedInCallback = FALSE;

	// v2.06.14; commented - the ShowWindow inside a CreateWindow wrapper causes the WM_ACTIVATE message to be sent
	// to the app when it is not ready to process it causing exceptions and malfunctioning.
	//
	// v2.06.12: fix, don't try ShowWindow on popup windows
	//if(!(origstyle & WS_POPUP)){
	//	if(bShowSpec) {
	//		(*pShowWindow)(hwnd, CmdShow);
	//	}
	//	else {
	//		// force window showing only for main win - fixes "Diablo" regression
	//		// v2.04.94: possibly unnecessary ... commented out. Check ...
	//		if(dxw.IsDesktop(hwnd))(*pShowWindow)(hwnd, SW_SHOWNORMAL);
	//	}
	//}

	BOOL bWPHooked = FALSE;
	if ((dxw.dwFlags1 & HOOKCHILDWIN) && (dwStyle & WS_CHILD))  {
		HookChildWndProc(hwnd, dwStyle, ApiRef);
		bWPHooked = TRUE;
	}
	if ((dxw.dwFlags8 & HOOKDLGWIN) && (dwStyle & WS_DLGFRAME)) {
		HookChildWndProc(hwnd, dwStyle, ApiRef);
		bWPHooked = TRUE;
	}
	if(!bWPHooked && isNewDesktop){
		// v2.05.40 fix: this was simply forgotten, it is unneeded for ddraw, d3d windows
		// but absolutely necessary for pure GDI. Fixes "Lode Runner Online" !!!
		// fixing windows message handling procedure
		dxw.HookWindowProc(hwnd);
	}

	// "Hoyle Casino Empire" needs to be in a maximized state to continue after the intro movie.
	// Sending a SW_MAXIMIZE message intercepted by the PREVENTMAXIMIZE handling fixes the problem.
	//if (dxw.IsFullScreen() && (dxw.dwFlags1 & PREVENTMAXIMIZE)){
	if ((hwnd == dxw.GethWnd()) && dxw.IsFullScreen() && (dxw.dwFlags1 & PREVENTMAXIMIZE)){
		OutTraceGDI("%s: entering maximized state\n", ApiRef); 
		dxw.IsVisible = TRUE;
		(*pShowWindow)(hwnd, SW_MAXIMIZE);
	}

	if(dxw.dwFlags1 & CLIPCURSOR) dxw.SetClipCursor();
	if(dxw.dwFlags4 & HIDEDESKTOP) dxw.HideDesktop(hwnd);
	if((dxw.dwFlags5 & PUSHACTIVEMOVIE) && bActiveMovie) (*pSetWindowPos)(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE);
	if(bActiveMovie) CurrentActiveMovieWin = hwnd;

	if((dwStyle & WS_CHILD) && (dxw.dwFlags19 & SCALECHILDWIN)){
		// PUSH arguments ...
		dxwcws.Put(hwnd, 0, origx, origy, origw, origh);
	}

	//if(dxw.dwDFlags2 & EXPERIMENTAL4){
	//		(*pSetWindowLong)(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	//		if(!pSetLayeredWindowAttributes) pSetLayeredWindowAttributes = SetLayeredWindowAttributes;
	//		(*pSetLayeredWindowAttributes)(hwnd,
	//		   RGB(0, 0, 0),				// The transparent color
	//		   255,							// Opacity of the window
	//		   LWA_ALPHA | LWA_COLORKEY);	// Enable both features
	//}

	OutTraceGDI("%s: created hwnd ret=%#x\n", ApiRef, hwnd);
	return hwnd;
}


#ifndef DXW_NOTRACES
static LPCSTR ClassToStr(LPCSTR Class)
{
	static char AtomBuf[20+1];
	if(((DWORD)Class & 0xFFFF0000) == 0){
		sprintf(AtomBuf, "ATOM(%X)", (DWORD)Class);
		return AtomBuf;
	}
	return Class;
}

static LPCWSTR ClassToWStr(LPCWSTR Class)
{
	static WCHAR AtomBuf[20+1];
	if(((DWORD)Class & 0xFFFF0000) == 0){
		swprintf(AtomBuf, L"ATOM(%X)", (DWORD)Class);
		return AtomBuf;
	}
	return Class;
}
#endif // DXW_NOTRACES
// to do: implement and use ClassToWStr() for widechar call

HWND WINAPI extCreateWindowExW(
  DWORD dwExStyle,
  LPCWSTR lpClassName,
  LPCWSTR lpWindowName,
  DWORD dwStyle,
  int x,
  int y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam) 
{
	BOOL bActiveMovie;
	ApiName("CreateWindowExW");

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char xString[20], yString[20], wString[20], hString[20];
		char sStyle[256];
		char sExStyle[256];
		if (x==CW_USEDEFAULT) strcpy(xString,"CW_USEDEFAULT"); 
		else sprintf(xString,"%d", x);
		if (y==CW_USEDEFAULT) strcpy(yString,"CW_USEDEFAULT"); 
		else sprintf(yString,"%d", y);
		if (nWidth==CW_USEDEFAULT) strcpy(wString,"CW_USEDEFAULT"); 
		else sprintf(wString,"%d", nWidth);
		if (nHeight==CW_USEDEFAULT) strcpy(hString,"CW_USEDEFAULT"); 
		else sprintf(hString,"%d", nHeight);
		OutTrace("%s: class=\"%ls\" wname=\"%ls\" pos=(%s,%s) size=(%s,%s) Style=%#x(%s) ExStyle=%#x(%s) hWndParent=%#x%s hMenu=%#x depth=%d\n",
			ApiRef, ClassToWStr(lpClassName), lpWindowName, xString, yString, wString, hString, 
			dwStyle, ExplainStyle(dwStyle, sStyle, 256), dwExStyle, ExplainExStyle(dwExStyle, sExStyle, 256),
			hWndParent, hWndParent==HWND_MESSAGE?"(HWND_MESSAGE)":"", hMenu, InMainWinCreation);
	}
	OutDebugGDI("%s: DEBUG fullscreen=%#x mainwin=%#x screen=(%d,%d)\n", 
		api, dxw.IsFullScreen(), dxw.GethWnd(), dxw.GetScreenWidth(), dxw.GetScreenHeight());
#endif

	// v2.05.41 fix: don't wcscmp atoms!
	if((dxw.dwFlags9 & KILLBLACKWIN) && ((DWORD)lpClassName & 0xFFFF0000) && (
		!wcscmp(lpClassName, L"Curtain")						// on "Clear-it" 
		)){
		OutTraceGDI("%s: KILL \"%ls\" window class ret=%#x\n", api, lpClassName, FAKEKILLEDWIN);
		return (HWND)FAKEKILLEDWIN;
	}

	bActiveMovie = lpWindowName && (
		!wcscmp(lpWindowName, L"ActiveMovie Window") ||
		!wcscmp(lpWindowName, L"MSCTFIME UI") // v2.04.82: "Extreme Boards Blades"
		);

	return CreateWindowCommon(
		ApiRef, 
		(CreateWindow_Type)pCreateWindowExW, 
		bActiveMovie, dwExStyle, 
		(void *)lpClassName, 
		(void *)lpWindowName, 
		dwStyle, 
		x, y, nWidth, nHeight, 
		hWndParent, 
		hMenu, 
		hInstance, 
		lpParam); 
}

#define WM_CLASSMASK 0xFFFF0000

// GHO: pro Diablo
HWND WINAPI extCreateWindowExA(
  DWORD dwExStyle,
  LPCSTR lpClassName,
  LPCSTR lpWindowName,
  DWORD dwStyle,
  int x,
  int y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam) 
{
	BOOL bActiveMovie;
	BOOL bBlackOverlay = FALSE;
	BOOL bWizGoldHack = FALSE; 
	ApiName("CreateWindowExA");

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char xString[20], yString[20], wString[20], hString[20];
		char sStyle[256];
		char sExStyle[256];
		if (x==CW_USEDEFAULT) strcpy(xString,"CW_USEDEFAULT"); 
		else sprintf(xString,"%d", x);
		if (y==CW_USEDEFAULT) strcpy(yString,"CW_USEDEFAULT"); 
		else sprintf(yString,"%d", y);
		if (nWidth==CW_USEDEFAULT) strcpy(wString,"CW_USEDEFAULT"); 
		else sprintf(wString,"%d", nWidth);
		if (nHeight==CW_USEDEFAULT) strcpy(hString,"CW_USEDEFAULT"); 
		else sprintf(hString,"%d", nHeight);
		OutTrace("%s: class=\"%s\" wname=\"%s\" pos=(%s,%s) size=(%s,%s) Style=%#x(%s) ExStyle=%#x(%s) hWndParent=%#x%s hMenu=%#x depth=%d\n",
			ApiRef, ClassToStr(lpClassName), lpWindowName, xString, yString, wString, hString, 
			dwStyle, ExplainStyle(dwStyle, sStyle, 256), dwExStyle, ExplainExStyle(dwExStyle, sExStyle, 256),
			hWndParent, hWndParent==HWND_MESSAGE?"(HWND_MESSAGE)":"", hMenu, InMainWinCreation);
	}
	OutDebugGDI("%s: DEBUG fullscreen=%#x mainwin=%#x screen=(%d,%d)\n", 
		ApiRef, dxw.IsFullScreen(), dxw.GethWnd(), dxw.GetScreenWidth(), dxw.GetScreenHeight());
#endif

	bActiveMovie = lpWindowName && (
		!strcmp(lpWindowName, "ActiveMovie Window") ||
		!strcmp(lpWindowName, "MSCTFIME UI") // v2.04.82: "Extreme Boards Blades"
		);

	// v2.05.94: added ""Smacker Window" class to movies. Beware: the flag alone is not enough
	// to ensure the video stretching to full window because the window parent IS NOT the virtual
	// desktop window. You should check for the grand-parent, or (easier and better) remember to
	// set also the "Stretch activemovie window" flag.
	if(((DWORD)lpClassName & 0xFFFF0000) && (
		(!strcmp(lpClassName, "Smacker Window")) ||						// on "Star Wars Rebellion" 
		(!strcmp(lpClassName, "MainVXLIBWindow"))							// on "Metro-Police" 
		)){
		bActiveMovie = TRUE;
	}
	// Wizardry GOLD hack .... 
	// the main window was supposed to be a MDI window, but this does not happen, so its
	// child window "Wizardry Gold Automap" can't be hidden.
	if(((DWORD)lpClassName & 0xFFFF0000) && lpWindowName && (
		(!strcmp(lpClassName, "MAPCLASS") && !strcmp(lpWindowName, "Wizardry Gold Automap")) 
		)){
		OutTrace("%s: Wizardry GOLD hack\n", ApiRef); 
		bWizGoldHack = TRUE;
	}	
	// v2.05.41 fix: don't strcmp atoms!
	if((dxw.dwFlags9 & KILLBLACKWIN) && ((DWORD)lpClassName & 0xFFFF0000) && (
		!strcmp(lpClassName, "DDFullBck") ||				// on "Three Dirty Dwarves", "Ecco the Dolphin" ...
		!strcmp(lpClassName, "Kane & Lynch 2 window 2") ||	// on "Kane & Lynch 2", obviously ...
		!strcmp(lpClassName, "propsfxblack") ||				// on "Crashday" (window name = "black")
		//!strcmp(lpClassName, "Editor") ||					// on "Bloodline"
		!strcmp(lpClassName, "Curtain")						// on "Tennis Critters" demo
		)){
		OutTraceGDI("%s: KILL \"%s\" window class ret=%#x\n", ApiRef, lpClassName, FAKEKILLEDWIN);
		return (HWND)FAKEKILLEDWIN;
	}

	if ((dxw.dwFlags9 & KILLBLACKWIN) && ((DWORD)lpClassName & 0xFFFF0000) && ((DWORD)lpWindowName & 0xFFFF0000)){
		if(!strcmp(lpClassName, "TForm1") && !strcmp(lpWindowName, "Form1")) {	// on "The Three Swordsmen"
			OutTraceGDI("%s: HIDE window class=\"%s\" name=\"%s\"\n", ApiRef, lpClassName, lpWindowName);
			bBlackOverlay = TRUE;
			dwExStyle |= WS_EX_LAYERED;
		}
	}

	if((dxw.dwFlags11 & CUSTOMLOCALE) && 
		pCreateWindowExW && 
		!(dxw.dwFlags14 & NOSETTEXT)){
		OutTraceGDI("%s: using WIDECHAR call\n", ApiRef);
		extern __inline NTLEA_TLS_DATA* GetTlsValueInternal(void);
		extern void InstallCbtHook(NTLEA_TLS_DATA * ptls);
		extern void UninstallCbtHook(NTLEA_TLS_DATA * ptls);
		NTLEA_TLS_DATA* p = GetTlsValueInternal();
		DWORD PrevCallType = p->CurrentCallType; 
		// COMMENTED OUT - NO GOOD, DON'T KNOW WHY ...
		//p->CurrentCallType = CT_CREATE_WINDOW;
		LPWSTR lpClassNameW = NULL;
		LPWSTR lpWindowNameW = NULL;
		BOOL IsStringClass = ((DWORD)lpClassName & WM_CLASSMASK);
		HWND ret;
		if(IsStringClass) {
			OutTraceLOC("%s: ANSI class=\"%s\"\n", ApiRef, lpClassName);
			int len = lstrlenA(lpClassName);
			int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpClassName, len, NULL, 0);
			lpClassNameW = (WCHAR *)malloc((n + 1) * sizeof(WCHAR));
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpClassName, len, lpClassNameW, len);
			lpClassNameW[n]=0;
			OutTraceLOC("%s: WIDE class=\"%ls\"\n", ApiRef, lpClassNameW);
		}
		if(lpWindowName) {
			OutTraceLOC("%s: ANSI wname=\"%s\"\n", ApiRef, lpWindowName);
			int len = lstrlenA(lpWindowName);
			int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpWindowName, len, NULL, 0);
			lpWindowNameW = (WCHAR *)malloc((n + 1) * sizeof(WCHAR));
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpWindowName, len, lpWindowNameW, len);
			lpWindowNameW[n]=0;
			OutTraceLOC("%s: WIDE wname=\"%ls\"\n", ApiRef, lpWindowNameW);
		}
		InstallCbtHook(p);
		ret = CreateWindowCommon(
			ApiRef, 
			(CreateWindow_Type)pCreateWindowExW, 
			bActiveMovie, dwExStyle, 
			(void *)lpClassNameW, 
			(void *)lpWindowNameW, 
			dwStyle, 
			x, y, nWidth, nHeight, 
			hWndParent, 
			hMenu, 
			hInstance, 
			lpParam); 
		SetWindowTextW(ret, lpWindowNameW);
		UninstallCbtHook(p);
		p->CurrentCallType = PrevCallType;
		if(lpClassNameW) free(lpClassNameW);
		if(lpWindowNameW) free(lpWindowNameW);
		return ret;
	}

	HWND ret = CreateWindowCommon(
		ApiRef, 
		(CreateWindow_Type)pCreateWindowExA, 
		bActiveMovie, dwExStyle, 
		(void *)lpClassName, 
		(void *)lpWindowName, 
		dwStyle, 
		x, y, nWidth, nHeight, 
		hWndParent, 
		hMenu, 
		hInstance, 
		lpParam); 
	
	if(bBlackOverlay) SetLayeredWindowAttributes(ret, RGB(0,0,0), 0, LWA_COLORKEY);
	if(bWizGoldHack) (*pShowWindow)(ret, SW_MINIMIZE);

	return ret;
}

#ifndef DXW_NOTRACES
extern void ExplainMsg(char *, BOOL, HWND, UINT, WPARAM, LPARAM);
#endif

extern LRESULT CALLBACK DefConversionProc(char *, LPVOID, HWND, HWND, BOOL, INT, WPARAM, LPARAM);
extern LRESULT CALLBACK TopLevelWindowProcEx(CallWindowProc_Type, WNDPROC, HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI extCallWindowProcCommon(char *api, CallWindowProc_Type pCallWindowProc, BOOL isAnsi,
									   WNDPROC lpPrevWndFunc, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// v2.02.30: fix (Imperialism II): apply to main window only !!!
	HRESULT res;

#ifndef DXW_NOTRACES
	if(IsTraceW) {
		ExplainMsg(ApiRef, isAnsi, hwnd, Msg, wParam, lParam);
		OutTrace("%s: lpPrevWndFunc=%#x\n", ApiRef, lpPrevWndFunc);
	}
#endif

	if(!FixWindowProc(api, hwnd, Msg, &wParam, &lParam, &res)) return res;

	//res = (*pCallWindowProc)(lpPrevWndFunc, hwnd, Msg, wParam, lParam);
	if((dxw.dwFlags11 & CUSTOMLOCALE) && ((UINT_PTR)lpPrevWndFunc != (UINT_PTR)TopLevelWindowProcEx))
		res = TopLevelWindowProcEx(pCallWindowProcA, lpPrevWndFunc, hwnd, Msg, wParam, lParam);
	else
		res = (*pCallWindowProc)(lpPrevWndFunc, hwnd, Msg, wParam, lParam);
	return res;
}

LRESULT WINAPI extCallWindowProcA(WNDPROC lpPrevWndFunc, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ApiName("CallWindowProcA"); return extCallWindowProcCommon(ApiRef, pCallWindowProcA, TRUE, lpPrevWndFunc, hwnd, Msg, wParam, lParam); }
LRESULT WINAPI extCallWindowProcW(WNDPROC lpPrevWndFunc, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ApiName("CallWindowProcW"); return extCallWindowProcCommon(ApiRef, pCallWindowProcW, FALSE, lpPrevWndFunc, hwnd, Msg, wParam, lParam); }

static LRESULT WINAPI DefWindowProcCommon(ApiArg, DefWindowProc_Type pDefWindowProc, BOOL isAnsi, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// v2.02.30: fix (Imperialism II): apply to main window only !!!
	// v2.03.50: fix - do clip cursor only after the window has got focus
	// v2.04.14: fix - erase clip cursor when window loses focus !!!
	HRESULT res;
#ifndef DXW_NOTRACES
	if(IsTraceW) ExplainMsg(api, isAnsi, hwnd, Msg, wParam, lParam);
#endif

	if((Msg == WM_SETTEXT) && (dxw.dwFlags11 & CUSTOMLOCALE)){
		if(isAnsi && IsWindowUnicode(hwnd)){
			if(!pDefWindowProcW) pDefWindowProcW = DefWindowProcW;
			LPCWSTR lParamW = MultiByteToWideCharInternal((LPCSTR)lParam);
			res = (*pDefWindowProcW)(hwnd, Msg, wParam, (LPARAM)lParamW);
			if(lParamW) free((LPVOID)lParamW);
			return res;
		}
	}

	if(!FixWindowProc(api, hwnd, Msg, &wParam, &lParam, &res)) return res;

	// keep it simple ... works with @#@"Aoi Sora no Neosphere"
	res = (*pDefWindowProc)(hwnd, Msg, wParam, lParam);
	return res;
}

LRESULT WINAPI extDefWindowProcW(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ ApiName("DefWindowProcW"); return DefWindowProcCommon(ApiRef, pDefWindowProcW, FALSE, hwnd, Msg, wParam, lParam); }
LRESULT WINAPI extDefWindowProcA(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{ ApiName("DefWindowProcA"); return DefWindowProcCommon(ApiRef, pDefWindowProcA, TRUE, hwnd, Msg, wParam, lParam); }

LRESULT WINAPI extDefFrameProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	ApiName("DefFrameProcA");
	LRESULT res;
	OutDebugGDI("%s: hWnd=%x Msg=%x wParam=%x lParam=%x\n",
		ApiRef, hWnd, Msg, wParam, lParam);

	if(dxw.dwFlags11 & CUSTOMLOCALE){
		res = DefConversionProc(ApiRef, (LPVOID)(DWORD_PTR)DefFrameProcW, hWnd, NULL, FALSE, Msg, wParam, lParam);
	}
	else {
		res = (*pDefFrameProcA)(hWnd, Msg, wParam, lParam);
	}
	return res;
}

LRESULT WINAPI extDefMDIChildProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ ApiName("DefMDIChildProcA"); return DefConversionProc(ApiRef, (LPVOID)(DWORD_PTR)(DWORD_PTR)DefMDIChildProcW, hWnd, NULL, FALSE, uMsg, wParam, lParam); }
LRESULT WINAPI extDefDlgProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ ApiName("DefDlgProcA"); return DefConversionProc(ApiRef, (LPVOID)(DWORD_PTR)DefDlgProcW, hWnd, NULL, FALSE, uMsg, wParam, lParam); }

static int HandleRect(char *api, void *pFun, HDC hdc, const RECT *lprc, HBRUSH hbr)
{
	// used for both FillRect and FrameRect calls
	int res;
	RECT rc;
	OutTraceGDI("%s: hdc=%#x hbrush=%#x rect=(%d,%d)-(%d,%d)\n", api, hdc, hbr, lprc->left, lprc->top, lprc->right, lprc->bottom);

	if(dxw.dwFlags4 & NOFILLRECT) {
		OutTraceGDI("%s: SUPPRESS\n", api, hdc, hbr, lprc->left, lprc->top, lprc->right, lprc->bottom);
		return TRUE;
	}

	// if no emulation, just proxy
	if(dxw.GDIEmulationMode == GDIMODE_NONE) {
		// v2.05.95 fix: rc was uninitialized here!!
		//return (*(FillRect_Type)pFun)(hdc, &rc, hbr);
		return (*(FillRect_Type)pFun)(hdc, lprc, hbr);
	}

	memcpy(&rc, lprc, sizeof(rc));

	// Be careful: when you call CreateCompatibleDC with NULL DC, it is created a memory DC
	// with same characteristics as desktop. That would return true from the call to
	// dxw.IsRealDesktop(WindowFromDC(hdc)) because WindowFromDC(hdc) is null.
	// So, it's fundamental to check also the hdc type (OBJ_DC is a window's DC)

	if((dxw.IsRealDesktopDC(hdc) && (OBJ_DC == (*pGetObjectType)(hdc)))) {
		HWND VirtualDesktop;
		VirtualDesktop=dxw.GethWnd();
		if(VirtualDesktop==NULL){
			OutTraceGDI("%s: no virtual desktop\n", api);
			return TRUE;
		}
		OutTraceGDI("%s: remapped hdc to virtual desktop hwnd=%#x\n", api, dxw.GethWnd());
		hdc=(*pGDIGetDC)(dxw.GethWnd());
	}

	if(dxw.IsToRemap(hdc)) {
		if(rc.left < 0) rc.left = 0;
		if(rc.top < 0) rc.top = 0;
		if((DWORD)rc.right > dxw.GetScreenWidth()) rc.right = dxw.GetScreenWidth();
		if((DWORD)rc.bottom > dxw.GetScreenHeight()) rc.bottom = dxw.GetScreenHeight();

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				res=(*(FillRect_Type)pFun)(sdc.GetHdc(), &rc, hbr);
				sdc.PutPrimaryDC(hdc, TRUE, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
				return res;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&rc);
				OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", api, rc.left, rc.top, rc.right, rc.bottom);	
				break;
			default:
				break;
		}
	}
	else {
		// when not in fullscreen mode, just proxy the call
		// but check coordinates: some games may use excessive coordinates: see "Premier Manager 98"
		RECT client;
		HWND hwnd;
		hwnd=WindowFromDC(hdc);
		// v2.03.76 fix: sometimes WindowFromDC returns NULL with unpredictable results
		// if NULL, try to bount within the main window rect
		if(!hwnd) hwnd=dxw.GethWnd();
		// if still NULL, avoid doing changes
		if(hwnd){
			(*pGetClientRect)(hwnd, &client);
			if(rc.left < client.left) rc.left=client.left;
			if(rc.top < client.top) rc.top=client.top;
			if(rc.right > client.right) rc.right=client.right;
			if(rc.bottom > client.bottom) rc.bottom=client.bottom;
			OutTraceGDI("%s: remapped hdc from hwnd=%#x to rect=(%d,%d)-(%d,%d)\n", api, hwnd, rc.left, rc.top, rc.right, rc.bottom);
		}
	}

	res=(*(FillRect_Type)pFun)(hdc, &rc, hbr);
	return res;
}

int WINAPI extFillRect(HDC hdc, const RECT *lprc, HBRUSH hbr)
{ ApiName("FillRect"); return HandleRect(ApiRef, (void *)pFillRect, hdc, lprc, hbr); }
int WINAPI extFrameRect(HDC hdc, const RECT *lprc, HBRUSH hbr)
{ ApiName("FrameRect"); return HandleRect(ApiRef, (void *)pFrameRect, hdc, lprc, hbr); }

BOOL WINAPI extInvertRect(HDC hdc, const RECT *lprc)
{
	int res;
	ApiName("InvertRect");
	RECT rc;
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d)\n", ApiRef, hdc, lprc->left, lprc->top, lprc->right, lprc->bottom);

	memcpy(&rc, lprc, sizeof(rc));

	if(dxw.IsToRemap(hdc)) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				res=(*pInvertRect)(sdc.GetHdc(), &rc);
				sdc.PutPrimaryDC(hdc, TRUE, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
				return res;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&rc);
				OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", ApiRef, rc.left, rc.top, rc.right, rc.bottom);	
				break;
			default:
				break;
		}
	}

	res=(*pInvertRect)(hdc, &rc);
	return res;
}

int WINAPI extValidateRect(HWND hwnd, const RECT *lpRect)
{
	// v2.03.91: manages the possibility of a NULL lprc value
	int res;
	ApiName("ValidateRect");
	RECT ScaledRect;

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		if (lpRect)
			OutTrace("%s: hwnd=%#x rect=(%d,%d)-(%d,%d)\n", 
				ApiRef, hwnd, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		else 
			OutTrace("%s: hwnd=%#x rect=(NULL)\n", ApiRef, hwnd);
	}
#endif

	if(dxw.Windowize){
		if(dxw.IsRealDesktop(hwnd)){
			hwnd = dxw.GethWnd();
		}
	}

	if(dxw.IsFullScreen()) {
		switch(dxw.GDIEmulationMode){
			case GDIMODE_STRETCHED:
			case GDIMODE_SHAREDDC:
			case GDIMODE_EMULATED:
				if (lpRect) {
					// v2.03.55: the lpRect area must NOT be altered by the call
					// effect visible in partial updates of Deadlock 2 main menu buttons
					ScaledRect = *lpRect;
					dxw.MapClient(&ScaledRect);
					lpRect = &ScaledRect;
					OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", 
						ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
			default:
				break;
		}
	}

	res=(*pValidateRect)(hwnd, lpRect);
	return res;
}

BOOL WINAPI extClipCursor(RECT *lpRectArg)
{
	// reference: hooking and setting ClipCursor is mandatori in "Emergency: Fighters for Life"
	// where the application expects the cursor to be moved just in a inner rect within the 
	// main window surface.

	BOOL res;
	ApiName("ClipCursor");
	RECT *lpRect;
	RECT Rect;

#ifndef DXW_NOTRACES
	if(IsTraceC){
		if (lpRectArg)
			OutTrace("%s: rect=(%d,%d)-(%d,%d)\n", 
				ApiRef, lpRectArg->left,lpRectArg->top,lpRectArg->right,lpRectArg->bottom);
		else 
			OutTrace("%s: rect=(NULL)\n", ApiRef);
	}
#endif

 	if (!(dxw.dwFlags1 & DISABLECLIPPING)) return TRUE;
	if ((dxw.dwFlags8 & CLIPLOCKED) && (lpRectArg == NULL)) return TRUE;

	if(lpRectArg){
		Rect=*lpRectArg;
		lpRect=&Rect;
	}
	else
		lpRect=NULL;

	if(dxw.dwFlags1 & CLIENTREMAPPING){ //v2.03.61
		// save desired clip region
		// v2.02.39: fix - do not attempt to write to NULL lpRect
		if (lpRect) {
			ClipRegion=*lpRectArg;
			lpClipRegion=&ClipRegion;
			*lpRect=dxw.MapWindowRect(lpRect);
		}
		else
			lpClipRegion=NULL;
	}

	if (pClipCursor) res=(*pClipCursor)(lpRect);
#ifndef DXW_NOTRACES
	if (lpRect) OutTraceGDI("%s: REMAPPED rect=(%d,%d)-(%d,%d) res=%#x\n", 
		ApiRef, lpRect->left,lpRect->top,lpRect->right,lpRect->bottom, res);
#endif // DXW_NOTRACES

	return TRUE;
}

BOOL WINAPI extGetClipCursor(LPRECT lpRect)
{
	// v2.1.93: if DISABLECLIPPING, return the saved clip rect coordinates

	BOOL ret;
	ApiName("GetClipCursor");

	// proxy....
	if (!(dxw.dwFlags1 & DISABLECLIPPING)) {
		ret=(*pGetClipCursor)(lpRect);
		// v2.03.11: fix for "SubCulture" mouse movement
		if(lpRect && dxw.isScaled)	*lpRect = dxw.GetScreenRect();
#ifndef DXW_NOTRACES
		if(IsTraceGDI){
			if (lpRect)
				OutTrace("%s: rect=(%d,%d)-(%d,%d) ret=%d\n", 
					ApiRef, lpRect->left,lpRect->top,lpRect->right,lpRect->bottom, ret);
			else 
				OutTrace("%s: rect=(NULL) ret=%d\n", ApiRef, ret);
		}		
#endif
		return ret;
	}

	if(lpRect){
		if(lpClipRegion)
			*lpRect=ClipRegion;
		else 
			*lpRect=dxw.GetScreenRect();
		OutTraceGDI("%s: rect=(%d,%d)-(%d,%d) ret=%d\n", 
			ApiRef, lpRect->left,lpRect->top,lpRect->right,lpRect->bottom, TRUE);
	}

	return TRUE;
}

LONG WINAPI extEnumDisplaySettingsA(LPCSTR lpszDeviceName, DWORD iModeNum, DEVMODEA *lpDevMode)
{
	LONG res;
	ApiName("EnumDisplaySettingsA");
	OSVERSIONINFO osinfo;
	EnumDisplaySettingsA_Type pEnumDisplaySettings = pEnumDisplaySettingsA;

	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	(*pGetVersionExA)(&osinfo);
	OutTraceGDI("%s: Devicename=%s ModeNum=%#x OS=%d.%d\n", 
		ApiRef, lpszDeviceName, iModeNum, osinfo.dwMajorVersion, osinfo.dwMinorVersion);

	if(dxw.dwFlags15 & HIDEDISPLAYMODES){
		extern LONG WINAPI hideEnumDisplaySettingsA(LPCSTR, DWORD, LPDEVMODEA);
		pEnumDisplaySettings = hideEnumDisplaySettingsA;
	}

	if(dxw.dwFlags4 & NATIVERES){
		// lists video card native resolutions, though faking emulated color resolutions
		if((osinfo.dwMajorVersion >= 6) && (dxw.IsEmulated)){
			switch(iModeNum){
			case ENUM_CURRENT_SETTINGS: 
			case ENUM_REGISTRY_SETTINGS: // lie ...
				res=(*pEnumDisplaySettings)(lpszDeviceName, iModeNum, lpDevMode);
				if(dxw.dwFlags17 & LOCKPIXELFORMAT){
					if(dxw.dwFlags2 & INIT8BPP) lpDevMode->dmBitsPerPel = 8;
					if(dxw.dwFlags2 & INIT16BPP) lpDevMode->dmBitsPerPel = 16;
					if(dxw.dwFlags2 & INIT24BPP) lpDevMode->dmBitsPerPel = 24;
					if(dxw.dwFlags2 & INIT32BPP) lpDevMode->dmBitsPerPel = 32;
				}
				if(dxw.dwFlags3 & FORCE16BPP) lpDevMode->dmBitsPerPel = 16;
				break;
			default:
				res=(*pEnumDisplaySettings)(lpszDeviceName, iModeNum / SUPPORTED_DEPTHS_NUMBER, lpDevMode);
				lpDevMode->dmBitsPerPel = (DWORD)SupportedDepths[iModeNum % SUPPORTED_DEPTHS_NUMBER];
				break;
			}
		}
		else
			res=(*pEnumDisplaySettings)(lpszDeviceName, iModeNum, lpDevMode);

	}
	else { // simulated modes: VGA or HDTV
		//int SupportedDepths[5]={8,16,24,32,0};
		DWORD nRes, nDepths, nEntries;
		res=(*pEnumDisplaySettings)(lpszDeviceName, ENUM_CURRENT_SETTINGS, lpDevMode);
		switch(iModeNum){
		case ENUM_CURRENT_SETTINGS: 
		case ENUM_REGISTRY_SETTINGS: // lie ...
			// v2.04.47: set current virtual resolution values
			lpDevMode->dmPelsHeight = dxw.GetScreenHeight();
			lpDevMode->dmPelsWidth = dxw.GetScreenWidth();
			if(dxw.dwFlags17 & LOCKPIXELFORMAT){
				if(dxw.dwFlags2 & INIT8BPP) lpDevMode->dmBitsPerPel = 8;
				if(dxw.dwFlags2 & INIT16BPP) lpDevMode->dmBitsPerPel = 16;
				if(dxw.dwFlags2 & INIT24BPP) lpDevMode->dmBitsPerPel = 24;
				if(dxw.dwFlags2 & INIT32BPP) lpDevMode->dmBitsPerPel = 32;
			}
			if(dxw.dwFlags3 & FORCE16BPP) lpDevMode->dmBitsPerPel = 16;
			break;
		default:
			// v2.04.77: check how many entries we should emulate
			for(nRes=0; SupportedRes[nRes].h; nRes++);
			for(nDepths=0; SupportedDepths[nDepths]; nDepths++);
			nEntries = nRes * nDepths;

			// v2.04.77: added bounds check to avoid random errors when querying high indexes
			if(iModeNum >= nEntries) {
				OutTraceGDI("%s: index overflow max=%d - returning 0\n", ApiRef, nEntries);
				return 0;
			}
			lpDevMode->dmPelsHeight = SupportedRes[iModeNum / SUPPORTED_DEPTHS_NUMBER].h;
			lpDevMode->dmPelsWidth  = SupportedRes[iModeNum / SUPPORTED_DEPTHS_NUMBER].w;
			lpDevMode->dmBitsPerPel = SupportedDepths[iModeNum % SUPPORTED_DEPTHS_NUMBER];
			if(lpDevMode->dmPelsHeight == 0) res = 0; // end of list
			break;
		}
	}

	if(dxw.dwFlags7 & MAXIMUMRES){
		if((lpDevMode->dmPelsWidth > (DWORD)dxw.iMaxW) || (lpDevMode->dmPelsHeight > (DWORD)dxw.iMaxH)){
			OutTraceGDI("%s: limit device size=(%d,%d)\n", ApiRef, dxw.iMaxW, dxw.iMaxH);
			lpDevMode->dmPelsWidth = dxw.iMaxW;
			lpDevMode->dmPelsHeight = dxw.iMaxH;
		}
	}

	OutTraceGDI("%s: color=%dBPP size=(%dx%d) refresh=%dHz\n", 
		ApiRef, lpDevMode->dmBitsPerPel, lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, lpDevMode->dmDisplayFrequency);
	return res;
}

LONG WINAPI extEnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, DEVMODEW *lpDevMode)
{
	LONG res;
	ApiName("EnumDisplaySettingsW");
	OSVERSIONINFO osinfo;
	EnumDisplaySettingsW_Type pEnumDisplaySettings = pEnumDisplaySettingsW;

	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	(*pGetVersionExA)(&osinfo);
	OutTraceGDI("%s: Devicename=%ls ModeNum=%#x OS=%d.%d\n", 
		ApiRef, lpszDeviceName, iModeNum, osinfo.dwMajorVersion, osinfo.dwMinorVersion);

	if(dxw.dwFlags15 & HIDEDISPLAYMODES){
		extern LONG WINAPI hideEnumDisplaySettingsW(LPCWSTR, DWORD, LPDEVMODEW);
		pEnumDisplaySettings = hideEnumDisplaySettingsW;
	}

	if(dxw.dwFlags4 & NATIVERES){
		// lists video card native resolutions, though faking emulated color resolutions
		if((osinfo.dwMajorVersion >= 6) && (dxw.IsEmulated)){
			switch(iModeNum){
			case ENUM_CURRENT_SETTINGS: 
			case ENUM_REGISTRY_SETTINGS: // lie ...
				res=(*pEnumDisplaySettings)(lpszDeviceName, iModeNum, lpDevMode);
				if(dxw.dwFlags17 & LOCKPIXELFORMAT){
					if(dxw.dwFlags2 & INIT8BPP) lpDevMode->dmBitsPerPel = 8;
					if(dxw.dwFlags2 & INIT16BPP) lpDevMode->dmBitsPerPel = 16;
					if(dxw.dwFlags2 & INIT24BPP) lpDevMode->dmBitsPerPel = 24;
					if(dxw.dwFlags2 & INIT32BPP) lpDevMode->dmBitsPerPel = 32;
				}
				if(dxw.dwFlags3 & FORCE16BPP) lpDevMode->dmBitsPerPel = 16;
				break;
			default:
				res=(*pEnumDisplaySettings)(lpszDeviceName, iModeNum / SUPPORTED_DEPTHS_NUMBER, lpDevMode);
				lpDevMode->dmBitsPerPel = (DWORD)SupportedDepths[iModeNum % SUPPORTED_DEPTHS_NUMBER];
				break;
			}
		}
		else
			res=(*pEnumDisplaySettings)(lpszDeviceName, iModeNum, lpDevMode);

	}
	else { // simulated modes: VGA or HDTV
		//int SupportedDepths[5]={8,16,24,32,0};
		DWORD nRes, nDepths, nEntries;
		res=(*pEnumDisplaySettings)(lpszDeviceName, ENUM_CURRENT_SETTINGS, lpDevMode);
		switch(iModeNum){
		case ENUM_CURRENT_SETTINGS: 
		case ENUM_REGISTRY_SETTINGS: // lie ...
			// v2.04.47: set current virtual resolution values
			lpDevMode->dmPelsHeight = dxw.GetScreenHeight();
			lpDevMode->dmPelsWidth = dxw.GetScreenWidth();
			if(dxw.dwFlags17 & LOCKPIXELFORMAT){
				if(dxw.dwFlags2 & INIT8BPP) lpDevMode->dmBitsPerPel = 8;
				if(dxw.dwFlags2 & INIT16BPP) lpDevMode->dmBitsPerPel = 16;
				if(dxw.dwFlags2 & INIT24BPP) lpDevMode->dmBitsPerPel = 24;
				if(dxw.dwFlags2 & INIT32BPP) lpDevMode->dmBitsPerPel = 32;
			}
			if(dxw.dwFlags3 & FORCE16BPP) lpDevMode->dmBitsPerPel = 16;
			break;
		default:
			// v2.04.77: check how many entries we should emulate
			for(nRes=0; SupportedRes[nRes].h; nRes++);
			for(nDepths=0; SupportedDepths[nDepths]; nDepths++);
			nEntries = nRes * nDepths;

			// v2.04.77: added bounds check to avoid random errors when querying high indexes
			if(iModeNum >= nEntries) {
				OutTraceGDI("%s: index overflow max=%d - returning 0\n", ApiRef, nEntries);
				return 0;
			}
			lpDevMode->dmPelsHeight = SupportedRes[iModeNum / SUPPORTED_DEPTHS_NUMBER].h;
			lpDevMode->dmPelsWidth  = SupportedRes[iModeNum / SUPPORTED_DEPTHS_NUMBER].w;
			lpDevMode->dmBitsPerPel = SupportedDepths[iModeNum % SUPPORTED_DEPTHS_NUMBER];
			if(lpDevMode->dmPelsHeight == 0) res = 0; // end of list
			break;
		}
	}

	if(dxw.dwFlags7 & MAXIMUMRES){
		if((lpDevMode->dmPelsWidth > (DWORD)dxw.iMaxW) || (lpDevMode->dmPelsHeight > (DWORD)dxw.iMaxH)){
			OutTraceGDI("%s: limit device size=(%d,%d)\n", ApiRef, dxw.iMaxW, dxw.iMaxH);
			lpDevMode->dmPelsWidth = dxw.iMaxW;
			lpDevMode->dmPelsHeight = dxw.iMaxH;
		}
	}

	OutTraceGDI("%s: color=%dBPP size=(%dx%d) refresh=%dHz\n", 
		ApiRef, lpDevMode->dmBitsPerPel, lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, lpDevMode->dmDisplayFrequency);
	return res;
}

#ifndef DXW_NOTRACES
static void DumpDisplaySettingsA(DEVMODEA *lpDevMode)
{
	OutTrace("> dmDeviceName: %s\n", lpDevMode->dmDeviceName);
	OutTrace("> dmSpecVersion: %#x\n", lpDevMode->dmSpecVersion);
	OutTrace("> dmDriverVersion: %#x\n", lpDevMode->dmDriverVersion);
	OutTrace("> dmSize: %#x\n", lpDevMode->dmSize);
	OutTrace("> dmDriverExtra: %#x\n", lpDevMode->dmDriverExtra);
	OutTrace("> dmPosition: (%d,%d)\n", lpDevMode->dmPosition.x, lpDevMode->dmPosition.y);
	OutTrace("> dmDisplayOrientation: %#x\n", lpDevMode->dmDisplayOrientation);
	OutTrace("> dmDisplayFixedOutput: %#x\n", lpDevMode->dmDisplayFixedOutput);
	OutTrace("> dmColor: %d\n", lpDevMode->dmColor);
	OutTrace("> dmDuplex: %d\n", lpDevMode->dmDuplex);
	OutTrace("> dmYResolution: %d\n", lpDevMode->dmYResolution);
	OutTrace("> dmTTOption: %d\n", lpDevMode->dmTTOption);
	OutTrace("> dmCollate: %d\n", lpDevMode->dmCollate);
	OutTrace("> dmFormName: %s\n", lpDevMode->dmFormName);
	OutTrace("> dmLogPixels: %d\n", lpDevMode->dmLogPixels);
	OutTrace("> dmBitsPerPel: %d\n", lpDevMode->dmBitsPerPel);
	OutTrace("> dmPelsWidth: %d\n", lpDevMode->dmPelsWidth);
	OutTrace("> dmPelsHeight: %d\n", lpDevMode->dmPelsHeight);
	OutTrace("> dmDisplayFlags: %#x\n", lpDevMode->dmDisplayFlags);
	OutTrace("> dmDisplayFrequency: %d\n", lpDevMode->dmDisplayFrequency);
}

static void DumpDisplaySettingsW(DEVMODEW *lpDevMode)
{
	OutTrace("> dmDeviceName: %ls\n", lpDevMode->dmDeviceName);
	OutTrace("> dmSpecVersion: %#x\n", lpDevMode->dmSpecVersion);
	OutTrace("> dmDriverVersion: %#x\n", lpDevMode->dmDriverVersion);
	OutTrace("> dmSize: %#x\n", lpDevMode->dmSize);
	OutTrace("> dmDriverExtra: %#x\n", lpDevMode->dmDriverExtra);
	OutTrace("> dmPosition: (%d,%d)\n", lpDevMode->dmPosition.x, lpDevMode->dmPosition.y);
	OutTrace("> dmDisplayOrientation: %#x\n", lpDevMode->dmDisplayOrientation);
	OutTrace("> dmDisplayFixedOutput: %#x\n", lpDevMode->dmDisplayFixedOutput);
	OutTrace("> dmColor: %d\n", lpDevMode->dmColor);
	OutTrace("> dmDuplex: %d\n", lpDevMode->dmDuplex);
	OutTrace("> dmYResolution: %d\n", lpDevMode->dmYResolution);
	OutTrace("> dmTTOption: %d\n", lpDevMode->dmTTOption);
	OutTrace("> dmCollate: %d\n", lpDevMode->dmCollate);
	OutTrace("> dmFormName: %ls\n", lpDevMode->dmFormName);
	OutTrace("> dmLogPixels: %d\n", lpDevMode->dmLogPixels);
	OutTrace("> dmBitsPerPel: %d\n", lpDevMode->dmBitsPerPel);
	OutTrace("> dmPelsWidth: %d\n", lpDevMode->dmPelsWidth);
	OutTrace("> dmPelsHeight: %d\n", lpDevMode->dmPelsHeight);
	OutTrace("> dmDisplayFlags: %#x\n", lpDevMode->dmDisplayFlags);
	OutTrace("> dmDisplayFrequency: %d\n", lpDevMode->dmDisplayFrequency);
}

char *sChangeDisplaySettingsRet(LONG ret)
{
	char *s;
	switch(ret){
		case DISP_CHANGE_SUCCESSFUL: s="SUCCESSFUL"; break;
		case DISP_CHANGE_BADDUALVIEW: s="BADDUALVIEW"; break;
		case DISP_CHANGE_BADFLAGS: s="BADFLAGS"; break;
		case DISP_CHANGE_BADMODE: s="BADMODE"; break;
		case DISP_CHANGE_BADPARAM: s="BADPARAM"; break;
		case DISP_CHANGE_FAILED: s="FAILED"; break;
		case DISP_CHANGE_NOTUPDATED: s="NOTUPDATED"; break;
		case DISP_CHANGE_RESTART: s="RESTART"; break;
		default: s="???"; break;
	}
	return s;
}
#endif // DXW_NOTRACES

LONG WINAPI extChangeDisplaySettingsA(DEVMODEA *lpDevMode, DWORD dwflags)
{
	ApiName("ChangeDisplaySettingsA");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sInfo[1024];
		char sFields[256];
		char sFlags[128];
		strcpy(sInfo, "");
		// v2.04.04: dmDeviceName not printed since it could be not initialized (Warhammer SOTHR)
		if (lpDevMode) sprintf(sInfo, " fields=%#x(%s) size=(%dx%d) bpp=%d freq=%d", 
			lpDevMode->dmFields, ExplainDevModeFields(lpDevMode->dmFields, sFields, 256),
			lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, 
			lpDevMode->dmBitsPerPel, lpDevMode->dmDisplayFrequency);
		OutTraceGDI("%s: lpDevMode=%#x flags=%#x(%s)%s\n", 
			ApiRef, lpDevMode, dwflags, ExplainChangeDisplaySettingsFlags(dwflags, sFlags, 128), sInfo);
		if(lpDevMode && IsDebugGDI) DumpDisplaySettingsA(lpDevMode);
	}
#endif

	return intChangeDisplaySettings(ApiRef, FALSE, lpDevMode, dwflags);
}

LONG WINAPI extChangeDisplaySettingsW(DEVMODEW *lpDevMode, DWORD dwflags)
{
	ApiName("ChangeDisplaySettingsW");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sInfo[1024];
		char sFields[256];
		char sFlags[128];
		strcpy(sInfo, "");
		// v2.04.04: dmDeviceName not printed since it could be not initialized (Warhammer SOTHR)
		if (lpDevMode) sprintf(sInfo, "fields=%#x(%s) size=(%d x %d) bpp=%d", 
			lpDevMode->dmFields, ExplainDevModeFields(lpDevMode->dmFields, sFields, 256),
			lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, lpDevMode->dmBitsPerPel);
		OutTraceGDI("%s: lpDevMode=%#x flags=%#x(%s)%s\n", 
			ApiRef, lpDevMode, dwflags, ExplainChangeDisplaySettingsFlags(dwflags, sFlags, 128), sInfo);
		if(lpDevMode && IsDebugGDI) DumpDisplaySettingsW(lpDevMode);
	}
#endif

	return intChangeDisplaySettings(ApiRef, TRUE, lpDevMode, dwflags);
}

LONG WINAPI extChangeDisplaySettingsExA(LPCSTR lpszDeviceName, DEVMODEA *lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam)
{
	ApiName("ChangeDisplaySettingsExA");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sInfo[1024];
		char sFields[256];
		char sFlags[128];
		strcpy(sInfo, "");
		if (lpDevMode) sprintf(sInfo, " DeviceName=%s fields=%#x(%s) size=(%d x %d) bpp=%d", 
			lpDevMode->dmDeviceName, lpDevMode->dmFields, ExplainDevModeFields(lpDevMode->dmFields, sFields, 256),
			lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, lpDevMode->dmBitsPerPel);
		OutTraceGDI("%s: DeviceName=%s lpDevMode=%#x flags=%#x(%s)%s\n", 
			ApiRef, lpszDeviceName, lpDevMode, dwflags, ExplainChangeDisplaySettingsFlags(dwflags, sFlags, 128), sInfo);
		if(IsDebugGDI) DumpDisplaySettingsA(lpDevMode);
	}
#endif

	return intChangeDisplaySettings(ApiRef, FALSE, lpDevMode, dwflags);
}

LONG WINAPI extChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, DEVMODEW *lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam)
{
	ApiName("ChangeDisplaySettingsExW");
#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sInfo[1024];
		char sFields[256];
		char sFlags[128];
		strcpy(sInfo, "");
		if (lpDevMode) sprintf(sInfo, " DeviceName=%ls fields=%#x(%s) size=(%d x %d) bpp=%d", 
			lpDevMode->dmDeviceName, lpDevMode->dmFields, ExplainDevModeFields(lpDevMode->dmFields, sFields, 256),
			lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, lpDevMode->dmBitsPerPel);
		OutTraceGDI("%s: DeviceName=%ls lpDevMode=%#x flags=%#x(%s)%s\n", 
			ApiRef, lpszDeviceName, lpDevMode, dwflags, ExplainChangeDisplaySettingsFlags(dwflags, sFlags, 128), sInfo);
		if(IsDebugGDI) DumpDisplaySettingsW(lpDevMode);
	}
#endif

	return intChangeDisplaySettings(ApiRef, TRUE, lpDevMode, dwflags);
}

static HDC WINAPI sGetDC(HWND hwnd, ApiArg)
{
	// to do: add parameter and reference to pGDIGetDCEx to merge properly GetDC and GetDCEx
	HDC ret;
	HWND lochwnd;

	if(!dxw.IsFullScreen()) {
		ret = (*pGDIGetDC)(hwnd);
		OutTraceGDI("%s: hdc=%#x\n", ApiRef, ret);
		return ret;
	}

	lochwnd=hwnd;

	if (dxw.IsRealDesktop(hwnd)) {
		OutTraceGDI("%s: desktop remapping hwnd=%#x->%#x\n", ApiRef, hwnd, dxw.GethWnd());
		lochwnd=dxw.GethWnd();
	}

	switch(dxw.GDIEmulationMode){
		//case GDIMODE_ASYNCDC:
		//	if(hwnd == dxw.GethWnd()){
		//		ret = lpADC->GetDC();
		//		//lpADC->ReleaseDC();
		//	}
		//	else ret=(*pGDIGetDC)(lochwnd);
		//	break;
		case GDIMODE_EMULATED:
			ret=dxw.AcquireEmulatedDC(lochwnd);
			break;
		case GDIMODE_SHAREDDC:
		case GDIMODE_STRETCHED:
		default:
			ret=(*pGDIGetDC)(lochwnd);
			break;
	}

	if(ret){
		OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, lochwnd, ret);
	}
	else{
		int err;
		err=GetLastError();
		OutErrorGDI("%s: ERROR hwnd=%#x err=%d @%d\n", ApiRef, lochwnd, err, __LINE__);
		if((err==ERROR_INVALID_WINDOW_HANDLE) && (lochwnd!=hwnd)){
			ret=(*pGDIGetDC)(hwnd);	
			if(ret)
				OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hwnd, ret);
			else
				OutErrorGDI("%s: ERROR hwnd=%#x err=%d @%d\n", ApiRef, hwnd, GetLastError(), __LINE__);
		}
	}

	return ret;
}

HDC WINAPI extGDIGetDC(HWND hwnd)
{
	ApiName("GetDC");
	HDC ret;
	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, hwnd);
	// v2.06.13: also in color emulated mode
	if(dxw.isColorEmulatedMode || dxw.isWineControlled) ret = (*pGDIGetDC)(hwnd);
	else ret = sGetDC(hwnd, ApiRef);
	OutTraceGDI("%s: hwnd=%#x ret(hdc)=%#x\n", ApiRef, hwnd, ret);
	return ret;
}

HDC WINAPI extGDIGetDCEx(HWND hwnd, HRGN hrgnClip, DWORD flags)
{
	ApiName("GetDCEx");
	HDC ret;
	// used by Star Wars Shadow of the Empire
#ifndef DXW_NOTRACES
	char sFlags[128];
	OutTraceGDI("%s: hwnd=%#x hrgnClip=%#x flags=%#x(%s)\n", ApiRef, hwnd, hrgnClip, flags, ExplainGetDCExFlags(flags, sFlags, 128));
#endif // DXW_NOTRACES
	// v2.06.13: also in color emulated mode
	if(dxw.isColorEmulatedMode || dxw.isWineControlled) ret = (*pGDIGetDCEx)(hwnd, hrgnClip, flags);
	else ret = sGetDC(hwnd, ApiRef);
	OutTraceGDI("%s: hwnd=%#x ret(hdc)=%#x\n", ApiRef, hwnd, ret);
	return ret;
}

HDC WINAPI extGDIGetWindowDC(HWND hwnd)
{
	ApiName("GetWindowDC");
	HDC ret;
	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, hwnd);

	// if not fullscreen or not desktop win, just proxy the call
	// v2.06.13: also in color emulated mode
	if(!dxw.IsFullScreen() || !dxw.IsDesktop(hwnd) || dxw.isColorEmulatedMode || dxw.isWineControlled)
		ret=(*pGDIGetWindowDC)(hwnd);
	else ret = sGetDC(hwnd, ApiRef);
	OutTraceGDI("%s: hwnd=%#x ret(hdc)=%#x\n", ApiRef, hwnd, ret);
	return ret;
}

int WINAPI extGDIReleaseDC(HWND hwnd, HDC hDC)
{
	int res;
	ApiName("ReleaseDC");

	OutTraceGDI("%s: hwnd=%#x hdc=%#x\n", ApiRef, hwnd, hDC);

	// v2.06.13: also in color emulated mode
	if(dxw.isColorEmulatedMode || dxw.isWineControlled) return (*pGDIReleaseDC)(hwnd, hDC);
	if (dxw.IsRealDesktop(hwnd)) hwnd=dxw.GethWnd();
	if(hwnd == 0) return(TRUE);

	switch(dxw.GDIEmulationMode){
		//case GDIMODE_ASYNCDC:
		//	if(hwnd == dxw.GethWnd()){
		//		//lpADC->AcquireDC();
		//		//res = lpADC->GetDC();
		//		lpADC->ReleaseDC();
		//		res = TRUE;
		//	}
		//	else res=(*pGDIReleaseDC)(hwnd, hDC);
		//	break;
		case GDIMODE_EMULATED:
			res=dxw.ReleaseEmulatedDC(hwnd);
			break;
		case GDIMODE_SHAREDDC:
		case GDIMODE_STRETCHED:
		default:
			res=(*pGDIReleaseDC)(hwnd, hDC);
			break;
	}

#ifndef DXW_NOTRACES
	if (!res) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
	return(res);
}

HDC WINAPI extBeginPaint(HWND hwnd, LPPAINTSTRUCT lpPaint)
{
	HDC hdc;
	ApiName("BeginPaint");

	OutTraceGDI("%s: hwnd=%#x lpPaint=%#x FullScreen=%#x\n", ApiRef, hwnd, lpPaint, dxw.IsFullScreen());

	// avoid access to real desktop
	if(dxw.IsRealDesktop(hwnd)) hwnd=dxw.GethWnd();

	if(dxw.dwFlags3 & FULLPAINTRECT) {
		static PAINTSTRUCT Paint;
		memcpy(&Paint, lpPaint, sizeof(PAINTSTRUCT));
		Paint.rcPaint = dxw.GetScreenRect();
		hdc=(*pBeginPaint)(hwnd, &Paint);
		(*pSelectClipRgn)(hdc, NULL);
	} else {
		hdc=(*pBeginPaint)(hwnd, lpPaint);
	}

	// if not in fullscreen mode, that's all!
	if(!dxw.IsFullScreen()) return hdc;

	switch(dxw.GDIEmulationMode){
		case GDIMODE_STRETCHED:
			// v2.05.34 fix: "(LPRECT)&(lpPaint->rcPaint)" is ok, "&(RECT)(lpPaint->rcPaint)" is bad!!
			if(dxw.dwFlags1 & CLIENTREMAPPING) {
				dxw.UnmapClient((LPRECT)&(lpPaint->rcPaint));
			}
			break;
		case GDIMODE_EMULATED:
			HDC EmuHDC; 
			EmuHDC = dxw.AcquireEmulatedDC(hwnd); 
#ifndef DXW_NOTRACES
			if(!DeleteObject(lpPaint->hdc)) 
				OutErrorGDI("%s: DeleteObject ERROR hdc=%#x err=%d @%d\n", ApiRef, lpPaint->hdc, GetLastError(), __LINE__);
#else
			DeleteObject(lpPaint->hdc);
#endif // DXW_NOTRACES
			lpPaint->hdc=EmuHDC;
			hdc = EmuHDC;
			break;
		case GDIMODE_SHAREDDC:
#if 0
			sdc.GetPrimaryDC(hdc);
			lpPaint->hdc = sdc.GetHdc();
			(*pBeginPaint)(hwnd, lpPaint);
			lpPaint->hdc = hdc;
			sdc.PutPrimaryDC(hdc, FALSE);
#endif
			break;
		default:
			break;
	}

	if(hdc){
		OutTraceGDI("%s: ret(hdc)=%#x lpPaint:{hdc=%#x rcPaint=(%d,%d)-(%d,%d)}\n", 
			ApiRef, hdc, lpPaint->hdc,
			lpPaint->rcPaint.left, lpPaint->rcPaint.top, lpPaint->rcPaint.right, lpPaint->rcPaint.bottom);
	}
	else {
		OutTraceGDI("%s: ERROR err=%d\n", GetLastError()); 
	}
	return hdc;
}

BOOL WINAPI extEndPaint(HWND hwnd, const PAINTSTRUCT *lpPaint)
{
	BOOL ret;
	ApiName("EndPaint");
	PAINTSTRUCT Paint;

	OutTraceGDI("%s: hwnd=%#x lpPaint=%#x:{hdc=%#x rcpaint=(%d,%d)-(%d,%d)}\n", 
		ApiRef, hwnd, lpPaint, lpPaint->hdc, 
		lpPaint->rcPaint.left, lpPaint->rcPaint.top, lpPaint->rcPaint.right, lpPaint->rcPaint.bottom);

	// if not fullscreen or not desktop win, just proxy the call
	if(!dxw.IsFullScreen()){
		ret=(*pEndPaint)(hwnd, lpPaint);
		return ret;
	}

	// avoid access to real desktop
	if(dxw.IsRealDesktop(hwnd)) hwnd=dxw.GethWnd();
	dxw.HandleFPS(); // handle refresh delays
	switch(dxw.GDIEmulationMode){
		case GDIMODE_EMULATED:
			ret=dxw.ReleaseEmulatedDC(hwnd);
			break;
		case GDIMODE_SHAREDDC:
#if 1
			if(lpPaint) dxw.MapClient((LPRECT)&(lpPaint->rcPaint));
			ret=(*pEndPaint)(hwnd, lpPaint);
#else
			PAINTSTRUCT Paint;
			Paint = *lpPaint;
			Paint.hdc = sdc.GetHdc();
			(*pEndPaint)(hwnd, &Paint);
			if(lpPaint) dxw.MapClient((LPRECT)&(lpPaint->rcPaint));
			ret=(*pEndPaint)(hwnd, lpPaint);
#endif
			break;
		case GDIMODE_STRETCHED:
			if(lpPaint) {
				Paint = *lpPaint;
				dxw.MapClient((LPRECT)&(Paint.rcPaint));
				lpPaint = &Paint;
				//dxw.MapClient((LPRECT)&(lpPaint->rcPaint));
			}
			ret=(*pEndPaint)(hwnd, lpPaint);
			break;
		default:
			ret=(*pEndPaint)(hwnd, lpPaint);
			break;
	}

	if(ret){
		OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hwnd, ret);
	}
	else{
		OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
	}

	return ret;
}

// v2.04.68: Riitaoja revised fix - it is wise to eliminate theeffects of HOOKDLGWIN flag for all dialog's child windows
// makes Red Alert 2 games working (and maybe the HOOKDLGWIN flag a little less dangerous)

HWND WINAPI extCreateDialogIndirectParam(ApiArg, CreateDialogIndirectParamA_Type pCreateDialogIndirectParam, BOOL isWide, HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	HWND RetHWND;
	DWORD dwFlags8;

	OutTraceGDI("%s: hInstance=%#x lpTemplate=%#x hWndParent=%#x lpDialogFunc=%#x lParamInit=%#x\n",
		ApiRef,
		hInstance, 
		lpTemplate,
		hWndParent, lpDialogFunc, lParamInit);
	
	if(dxw.Windowize && (dxw.dwFlags10 & REPLACEDIALOGS)){
		HWND hwnd;
		hwnd = dxw.CreateVirtualDesktop();
		OutTraceGDI("%s: REPLACEDIALOG hwnd=%#x\n", ApiRef, hwnd);
		dxw.SethWnd(hwnd);
		return hwnd;
	}

	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hWndParent)) hWndParent=dxw.GethWnd();
	
	// v2.04.68: no child dialog hooking
	dwFlags8 = dxw.dwFlags8;
	dxw.dwFlags8 &= ~HOOKDLGWIN;

	if((dxw.dwFlags11 & CUSTOMLOCALE) && !isWide){
		// v2.06.03: WIDE translation of dialogs. Take advantage of the WIDE internal structure of the dialog
		// @#@ to be tested !!!
		return extCreateDialogIndirectParam(ApiRef, pCreateDialogIndirectParamW, TRUE, hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
	}

	if(dxw.Windowize && (dxw.dwFlags10 & STRETCHDIALOGS)){
		DWORD dwSize;
		LPVOID lpScaledRes;
		DWORD gdiMode = dxw.GDIEmulationMode;
		dxw.GDIEmulationMode = GDIMODE_NONE;
		dwSize = dxwStretchDialog((LPVOID)lpTemplate, NULL); // just calculate the dialog size
		dxw.GDIEmulationMode = gdiMode;
#ifndef DXW_NOTRACES
		OutHexDW((LPBYTE)lpTemplate, dwSize);
#endif //DXW_NOTRACES
		lpScaledRes = malloc(dwSize);
		memcpy(lpScaledRes, lpTemplate, dwSize);
		dxwStretchDialog(lpScaledRes, DXW_DIALOGFLAG_DUMP|DXW_DIALOGFLAG_STRETCH|(dxw.dwFlags1 & FIXTEXTOUT ? DXW_DIALOGFLAG_STRETCHFONT : 0));
		RetHWND=(*pCreateDialogIndirectParam)(hInstance, (LPCDLGTEMPLATE)lpScaledRes, hWndParent, lpDialogFunc, lParamInit);
		free(lpScaledRes);
	}
	else {
		RetHWND=(*pCreateDialogIndirectParam)(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
	}

	// recover dialog hooking
	dxw.dwFlags8 = dwFlags8;

	if(dxw.dwFlags13 & CENTERDIALOGS) dxwCenterDialog(RetHWND);

	// v2.02.73: redirect lpDialogFunc only when it is nor NULL: fix for "LEGO Stunt Rally"
	if(lpDialogFunc && (dxw.dwFlags8 & HOOKDLGWIN)){	// v2.03.41 - debug option
		dxwws.PutProc(RetHWND, (WNDPROC)lpDialogFunc);
#ifndef DXW_NOTRACES
		if(!(*pSetWindowLong)(RetHWND, DWL_DLGPROC, (LONG)extDialogWindowProc))
			OutErrorGDI("%s: SetWindowLong ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#else
		(*pSetWindowLong)(RetHWND, DWL_DLGPROC, (LONG)extDialogWindowProc);
#endif // DXW_NOTRACES
	}

	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, RetHWND);
	return RetHWND;
}

HWND WINAPI extCreateDialogIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{ ApiName("CreateDialogIndirectParamA"); return extCreateDialogIndirectParam(ApiRef, pCreateDialogIndirectParamA, FALSE, hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit); }
HWND WINAPI extCreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{ ApiName("CreateDialogIndirectParamW"); return extCreateDialogIndirectParam(ApiRef, pCreateDialogIndirectParamW, TRUE, hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit); }

HWND WINAPI extCreateDialogParam(ApiArg, CreateDialogParam_Type pCreateDialogParam, BOOL isWide, HINSTANCE hInstance, LPVOID lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	HWND RetHWND;
	DWORD dwFlags8;

	if(isWide){
		OutTraceGDI("%s: hInstance=%#x lpTemplateName=%ls hWndParent=%#x lpDialogFunc=%#x lParamInit=%#x\n",
			ApiRef, hInstance, sTemplateNameW((LPCWSTR)lpTemplateName), hWndParent, lpDialogFunc, lParamInit);
	}
	else {
		OutTraceGDI("%s: hInstance=%#x lpTemplateName=%s hWndParent=%#x lpDialogFunc=%#x lParamInit=%#x\n",
			ApiRef, hInstance, sTemplateNameA((LPCSTR)lpTemplateName), hWndParent, lpDialogFunc, lParamInit);
	}
	
	if(dxw.IsFullScreen() && dxw.IsRealDesktop(hWndParent)) {
		OutTraceGDI("%s: set hwndparent=%#x->%#x\n", ApiRef, hWndParent, dxw.GethWnd());
		hWndParent=dxw.GethWnd();
	}
	
	// v2.04.68: no child dialog hooking
	dwFlags8 = dxw.dwFlags8;
	dxw.dwFlags8 &= ~HOOKDLGWIN;

	if((dxw.dwFlags11 & CUSTOMLOCALE) && !isWide){
		// v2.06.03: WIDE translation of dialogs. Take advantage of the WIDE internal structure of the dialog
		// @#@ "Monster Boy III" (Jp)
		return extCreateDialogParam(ApiRef, pCreateDialogParamW, TRUE, hInstance, lpTemplateName, hWndParent, lpDialogFunc, lParamInit);
	}

	if(dxw.Windowize && (dxw.dwFlags10 & STRETCHDIALOGS)){
		HRSRC hRes;
		HGLOBAL hgRes;
		LPVOID lpRes;
		LPVOID lpScaledRes;
		DWORD dwSize;
		if(isWide)
			hRes = FindResourceW(NULL, (LPCWSTR)lpTemplateName, (LPCWSTR)RT_DIALOG);
		else
			hRes = FindResourceA(NULL, (LPCSTR)lpTemplateName, RT_DIALOG);

		if(!hRes) {
			OutErrorGDI("%s: FindResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		hgRes = LoadResource(NULL, hRes);
		if(!hgRes) {
			OutErrorGDI("%s: LoadResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		lpRes = LockResource(hgRes);
		if(!lpRes) {
			OutErrorGDI("%s: LockResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		dwSize = SizeofResource(NULL, hRes);
		if(!dwSize) {
			OutErrorGDI("%s: SizeofResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		lpScaledRes = malloc(dwSize);
		memcpy(lpScaledRes, lpRes, dwSize);
		UnlockResource(lpRes);
#ifndef DXW_NOTRACES
		OutHexDW((LPBYTE)lpScaledRes, dwSize);
#endif //DXW_NOTRACES
		dxwStretchDialog(lpScaledRes, DXW_DIALOGFLAG_DUMP|DXW_DIALOGFLAG_STRETCH);
		RetHWND = (*pCreateDialogIndirectParamA)(hInstance, (LPCDLGTEMPLATE)lpScaledRes, hWndParent, lpDialogFunc, lParamInit);
		free(lpScaledRes);
	}
	else {
		RetHWND=(*pCreateDialogParam)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, lParamInit);
	}

	dxw.dwFlags8 = dwFlags8;

	if(dxw.dwFlags13 & CENTERDIALOGS) dxwCenterDialog(RetHWND);

	if(!RetHWND){
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}

	// v2.02.73: redirect lpDialogFunc only when it is nor NULL: fix for "LEGO Stunt Rally"
	// v2.04.18: HOOKDLGWIN (not to be checked to fix "PBA Bowling 2")
	if(lpDialogFunc && (dxw.dwFlags8 & HOOKDLGWIN)){	// v2.03.41 - debug option
		dxwws.PutProc(RetHWND, (WNDPROC)lpDialogFunc);
#ifndef DXW_NOTRACES
		if(!(*pSetWindowLong)(RetHWND, DWL_DLGPROC, (LONG)extDialogWindowProc))
			OutErrorGDI("%s: SetWindowLong ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#else
		(*pSetWindowLong)(RetHWND, DWL_DLGPROC, (LONG)extDialogWindowProc);
#endif // DXW_NOTRACES
	}

	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, RetHWND);
	return RetHWND;
}

HWND WINAPI extCreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{return extCreateDialogParam("CreateDialogParamA", pCreateDialogParamA, FALSE, hInstance, (LPVOID)lpTemplateName, hWndParent, lpDialogFunc, lParamInit); }
HWND WINAPI extCreateDialogParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)
{return extCreateDialogParam("CreateDialogParamW", pCreateDialogParamW, TRUE, hInstance, (LPVOID)lpTemplateName, hWndParent, lpDialogFunc, lParamInit); }

BOOL WINAPI extMoveWindow(HWND hwnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	ApiName("MoveWindow");
	BOOL ret;
	DWORD dwStyle, dwExStyle;
#ifdef DXWHIDEWINDOWUPDATES
	int origx, origy, origw, origh;
#endif
	OutTraceGDI("%s: hwnd=%#x xy=(%d,%d) size=(%d,%d) repaint=%#x\n",
		ApiRef, hwnd, X, Y, nWidth, nHeight, bRepaint);
	OutDebugDW("> fullscreen=%d InMainWinCreation=%d\n", dxw.IsFullScreen(), InMainWinCreation);

	if(dxw.EmulateWin95){
		// emulation of Microsoft "IgnoreZeroMoveWindow" shim
		if(nWidth == 0) nWidth = 1;
		if(nHeight == 0) nHeight = 1;
	}

	if(InMainWinCreation) MovedInCallback = TRUE;

#if DXWHIDEWINDOWUPDATES
	origx = X;
	origy = Y;
	origw = nWidth;
	origh = nHeight;
#endif
	if(dxw.dwFlags13 & LOCKALLWINDOWS) {
		DWORD dwStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
		if(!(dwStyle & WS_CHILD)) return (*pMoveWindow)(hwnd, dxw.iPos0X, dxw.iPos0Y, dxw.iSiz0X, dxw.iSiz0Y, bRepaint);
	}

	if(dxw.isScaled){ // v2.04.97 - "Fallen Haven"

		if(dxw.IsDesktop(hwnd)){
			// v2.1.93: happens in "Emergency Fighters for Life" ...
			// what is the meaning of this? is it related to video stretching?
			OutTraceGDI("%s: prevent moving desktop win\n", ApiRef);
			return TRUE;
		}

		if((hwnd==dxw.GethWnd()) || (hwnd==dxw.hParentWnd)){
			OutTraceGDI("%s: prevent moving main win\n", ApiRef);
			return TRUE;
		}

		// v2.04.32: trim dimensions - useful for "Mig Alley"
		// v2.05.01: greater or equal!
		if (((DWORD)nWidth>=dxw.GetScreenWidth()) && ((DWORD)nHeight>=dxw.GetScreenHeight())){
			if(dxw.GethWnd() == 0){
				OutTraceGDI("%s: MAIN win hwnd=%#x\n", ApiRef, hwnd);
				// v2.05.01: promote to main win
				dxw.SethWnd(hwnd);
				dxw.SetFullScreen(TRUE);
			}
			else {
				OutTraceGDI("%s: BIG win hwnd=%#x\n", ApiRef, hwnd);
			}
			X = Y = 0;
			nWidth = dxw.GetScreenWidth();
			nHeight = dxw.GetScreenHeight();
		}

		dwStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
		dwExStyle = (*pGetWindowLong)(hwnd, GWL_EXSTYLE);
		if (dxw.IsFullScreen() && (dxw.dwFlags1 & CLIENTREMAPPING)){
			POINT upleft={0,0};
			if (IsRelativePosition(dwStyle, dwExStyle, (HWND)-1)){ // parent = unknown here, but not essential?
				dxw.MapClient(&X, &Y, &nWidth, &nHeight);
				OutTraceGDI("%s: fixed RELATIVE pos=(%d,%d) size=(%d,%d)\n", ApiRef, X, Y, nWidth, nHeight);
			}
			else {
				dxw.MapWindow(&X, &Y, &nWidth, &nHeight);
				OutTraceGDI("%s: fixed ABSOLUTE pos=(%d,%d) size=(%d,%d)\n", ApiRef, X, Y, nWidth, nHeight);
			}
		}
		else{
			if((X==0)&&(Y==0)&&(nWidth==(int)dxw.GetScreenWidth())&&(nHeight==(int)dxw.GetScreenHeight())){
				// evidently, this was supposed to be a fullscreen window....
				RECT screen;
				POINT upleft = {0,0};
				char *sStyle;
				(*pGetClientRect)(dxw.GethWnd(),&screen);
				(*pClientToScreen)(dxw.GethWnd(),&upleft);
				if (IsRelativePosition(dwStyle, dwExStyle, (HWND)-1)){ // parent = unknown here, but not essential?
					// Big main child window: see "Reah"
					X=Y=0;
					sStyle="(relative) ";
				}
				else{
					// Regular big main window, usual case.
					X=upleft.x;
					Y=upleft.y;
					sStyle="(absolute) ";
				}
				nWidth=screen.right;
				nHeight=screen.bottom;
				if (dxw.dwFlags7 & ANCHORED){ 
					WINDOWPOS MaxPos;
					dxw.CalculateWindowPos(hwnd, dxw.iSizX, dxw.iSizY, &MaxPos);
					nWidth = MaxPos.cx;
					nHeight = MaxPos.cy;
					X  = MaxPos.x;
					Y  = MaxPos.y;
				}	
				OutTraceGDI("%s: fixed BIG %swin pos=(%d,%d) size=(%d,%d)\n", ApiRef, sStyle, X, Y, nWidth, nHeight);
			}
		}
	}

#ifdef DXWHIDEWINDOWUPDATES
	if(dxw.dwFlags10 & HIDEWINDOWCHANGES){
		DWORD lpWinCB;
		ret=(*pMoveWindow)(hwnd, origx, origy, origw, origh, FALSE);
		lpWinCB = (*pGetWindowLong)(hwnd, GWL_WNDPROC);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, NULL);
		ret=(*pMoveWindow)(hwnd, X, Y, nWidth, nHeight, bRepaint);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, lpWinCB);
	}
	else {
		ret=(*pMoveWindow)(hwnd, X, Y, nWidth, nHeight, bRepaint);
	}
#else
	ret=(*pMoveWindow)(hwnd, X, Y, nWidth, nHeight, bRepaint);
#endif
#ifndef DXW_NOTRACES
	if(!ret) OutErrorGDI("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
	if(dxw.bAutoScale) dxw.AutoScale();
	return ret;
} 

int WINAPI extShowCursor(BOOL bShow)
{
	static int iFakeCounter = 0;
	ApiName("ShowCursor");
	int ret;

	if((dxw.dwFlags9 & ZERODISPLAYCOUNTER) && !bShow){
		ret=(*pShowCursor)(bShow);
		OutTraceGDI("%s: bShow=%#x ret=%#x FIXED ret=0\n", ApiRef, bShow, ret);
		return 0; 
	}

	OutTraceC("%s: bShow=%#x\n", ApiRef, bShow);
	if (bShow){
		if (dxw.dwFlags1 & HIDEHWCURSOR){
			iFakeCounter++;
			OutTraceC("%s: HIDEHWCURSOR ret=%#x\n", ApiRef, iFakeCounter);
			return iFakeCounter;
		}
	}
	else {
		if (dxw.dwFlags2 & SHOWHWCURSOR){
			iFakeCounter--;
			OutTraceC("%s: SHOWHWCURSOR ret=%#x\n", ApiRef, iFakeCounter);
			return iFakeCounter;
		}
	}
	ret=(*pShowCursor)(bShow);
	OutTraceC("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extScrollDC(HDC hdc, int dx, int dy, const RECT *lprcScroll, const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate)
{
	BOOL res;
	ApiName("ScrollDC");

	if(IsTraceGDI){
		char sRect[81];
		if(lprcScroll) sprintf(sRect, "(%d,%d)-(%d,%d)", lprcScroll->left, lprcScroll->top, lprcScroll->right, lprcScroll->bottom);
		else strcpy(sRect, "NULL");
		char sClip[81];
		if(lprcClip) sprintf(sClip, "(%d,%d)-(%d,%d)", lprcClip->left, lprcClip->top, lprcClip->right, lprcClip->bottom);
		else strcpy(sClip, "NULL");
		OutTraceGDI("%s: hdc=%#x(%s) dxy=(%d,%d) scrollrect=%s cliprect=%s hrgn=%#x\n", 
			ApiRef, hdc, ExplainDCType((*pGetObjectType)(hdc)), dx, dy, sRect, sClip, hrgnUpdate); 
	}

	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_STRETCHED:
				RECT rcSaveScroll, rcSaveClip;
				dxw.MapClient(&dx, &dy);
				OutTraceGDI("%s: FIXED dxy=(%d,%d)\n", ApiRef, dx, dy);
				if(lprcScroll) {
					rcSaveScroll = *lprcScroll;
					dxw.MapClient((LPRECT)lprcScroll);
					OutTraceGDI("%s: FIXED scrollrect=(%d,%d)-(%d,%d)\n", 
						ApiRef, lprcScroll->left, lprcScroll->top, lprcScroll->right, lprcScroll->bottom);
				}
				if(lprcClip){
					rcSaveClip = *lprcClip;
					dxw.MapClient((LPRECT)lprcClip);
					OutTraceGDI("%s: FIXED cliplrect=(%d,%d)-(%d,%d)\n", 
						ApiRef, lprcClip->left, lprcClip->top, lprcClip->right, lprcClip->bottom);
				}
				res=(*pScrollDC)(hdc, dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate);
				if(lprcScroll) *(LPRECT)lprcScroll = rcSaveScroll;
				if(lprcClip) *(LPRECT)lprcClip = rcSaveClip;
				if(res && lprcUpdate){
					OutTraceGDI("%s: updaterect=(%d,%d)-(%d,%d)\n", 
						ApiRef, lprcUpdate->left, lprcUpdate->top, lprcUpdate->right, lprcUpdate->bottom);
					dxw.UnmapClient(lprcUpdate);
					OutTraceGDI("%s: FIXED updaterect=(%d,%d)-(%d,%d)\n", 
						ApiRef, lprcUpdate->left, lprcUpdate->top, lprcUpdate->right, lprcUpdate->bottom);
				}
				return res;	
				break;
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				res=(*pScrollDC)(sdc.GetHdc(), dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate);
				sdc.PutPrimaryDC(hdc, TRUE, lprcUpdate->left, lprcUpdate->top, lprcUpdate->right-lprcUpdate->left, lprcUpdate->bottom-lprcUpdate->top);
				return res;
				break;
			case GDIMODE_EMULATED:
				break;
			default:
				break;
		}
	}

	res=(*pScrollDC)(hdc, dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate);
	if(res){
		if(lprcUpdate) {
			OutTraceGDI("%s: updaterect=(%d,%d)-(%d,%d)\n", 
			ApiRef, lprcUpdate->left, lprcUpdate->top, lprcUpdate->right, lprcUpdate->bottom);
		}
		else {
			OutTraceGDI("%s: updaterect=NULL\n", ApiRef);
		}
	}
	else OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;	
}

HWND WINAPI extGetTopWindow(HWND hwnd)
{
	HWND ret;
	ApiName("GetTopWindow");
	OutTraceGDI("%s: hwnd=%#x fullscreen=%#x\n", ApiRef, hwnd, dxw.IsFullScreen()); 
	ret = (*pGetTopWindow)(hwnd);
	// v2.05.99 + v2.06.01 fix - be very cautios before applying the handle switch
	// test cases: "Monopoly Junior"
	if(hwnd && dxw.GethWnd() && dxw.Windowize && dxw.IsFullScreen() && (hwnd == (*pGetDesktopWindow)())){
		HWND fixret = (*pGetTopWindow)(dxw.GethWnd());
		if(ret != fixret) {
			OutTraceGDI("%s: hwnd=%#x ret=%#x->%#x\n", ApiRef, hwnd, ret, fixret); 
		}
		ret = fixret;
	}
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret); 
	return ret;
}

LONG WINAPI extTabbedTextOutA(HDC hdc, int X, int Y, LPCSTR lpString, int nCount, int nTabPositions, const LPINT lpnTabStopPositions, int nTabOrigin)
{
	BOOL res;
	ApiName("TabbedTextOutA");
	LPINT lpnScaledTabStopPositions;

#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hdc=%#x xy=(%d,%d) nCount=%d nTP=%d nTOS=%d str=(%d)\"%.*s\"\n", 
		ApiRef, hdc, X, Y, nCount, nTabPositions, nTabOrigin, nCount, nCount, lpString);
	for(int iTab=0; iTab<nTabPositions; iTab++){
		OutTraceGDI("> tab[%d]=%d\n", iTab, lpnTabStopPositions[iTab]);
	}
#endif // DXW_NOTRACES

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextA(ApiRef, X, Y, lpString, nCount);

	lpnScaledTabStopPositions = lpnTabStopPositions;
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				res=(*pTabbedTextOutA)(sdc.GetHdc(), X, Y, lpString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin);
				sdc.PutPrimaryDC(hdc, TRUE);
				return res;
				break;
			case GDIMODE_STRETCHED: 
				{	
					// scale all value args, copy & scale ref args.
					nTabOrigin = (nTabOrigin * dxw.iSizX) / dxw.GetScreenWidth();
					lpnScaledTabStopPositions = (LPINT)malloc(sizeof(int)*nTabPositions);
					for(int iTab=0; iTab<nTabPositions; iTab++){
						lpnScaledTabStopPositions[iTab] = (lpnTabStopPositions[iTab] * dxw.iSizX) / dxw.GetScreenWidth();
						OutTraceGDI("> tab[%d]: pos=%d->%d\n", iTab, lpnTabStopPositions[iTab], lpnScaledTabStopPositions[iTab]);
					}
					dxw.MapClient(&X, &Y);
				}
				break;
			case GDIMODE_EMULATED:
				break;
			default:
				break;
		}
		OutTraceGDI("%s: fixed dest=(%d,%d)\n", ApiRef, X, Y);
	}

	res=(*pTabbedTextOutA)(hdc, X, Y, lpString, nCount, nTabPositions, lpnScaledTabStopPositions, nTabOrigin);
	if(lpnScaledTabStopPositions != lpnTabStopPositions) free(lpnScaledTabStopPositions);
	return res;
}

LONG WINAPI extTabbedTextOutW(HDC hdc, int X, int Y, LPCWSTR lpString, int nCount, int nTabPositions, const LPINT lpnTabStopPositions, int nTabOrigin)
{
	BOOL res;
	ApiName("TabbedTextOutW");
	OutTraceGDI("%s: hdc=%#x xy=(%d,%d) nCount=%d nTP=%d nTOS=%d str=(%d)\"%ls\"\n", 
		ApiRef, hdc, X, Y, nCount, nTabPositions, nTabOrigin, lpString);

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextW(ApiRef, X, Y, lpString, nCount);

	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				res=(*pTabbedTextOutW)(sdc.GetHdc(), X, Y, lpString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin);
				sdc.PutPrimaryDC(hdc, TRUE);
				return res;
				break;
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&X, &Y);
				break;
			case GDIMODE_EMULATED:
				break;
			default:
				break;
		}
		OutTraceGDI("%s: fixed dest=(%d,%d)\n", ApiRef, X, Y);
	}

	res=(*pTabbedTextOutW)(hdc, X, Y, lpString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin);
	return res;
}

BOOL WINAPI extDestroyWindow(HWND hWnd)
{
	// v2.02.43: "Empire Earth" builds test surfaces that must be destroyed!
	// v2.03.20: "Prince of Persia 3D" destroys the main window that must be preserved! 
	BOOL res;
	ApiName("DestroyWindow");
	BOOL bReverted = FALSE;

	OutDebugDW("%s: hwnd=%#x\n", ApiRef, hWnd);
	if (hWnd == dxw.GethWnd()) {
		OutTraceGDI("%s: destroy main hwnd=%#x\n", ApiRef, hWnd);
		if(hLastFullScrWin){
			OutTraceGDI("%s: revert to main hwnd=%#x bpp=%d\n", 
				ApiRef, hWnd, ddpLastPixelFormat.dwRGBBitCount);
			dxw.SethWnd(hLastFullScrWin);
			hLastFullScrWin = NULL;
			//dxw.VirtualPixelFormat = ddpLastPixelFormat;
			memcpy(&dxw.VirtualPixelFormat, &ddpLastPixelFormat, sizeof(DDPIXELFORMAT));
			extern int iPrimarySurfaceVersion;
			SetBltTransformations();
			bReverted = TRUE;
		}
		else {
			OutTraceGDI("%s: destroy main hwnd=%#x\n", ApiRef, hWnd);
			dxw.SethWnd(NULL);
		}

		if(dxw.dwFlags6 & NODESTROYWINDOW) {
			OutTraceGDI("%s: do NOT destroy main hwnd=%#x\n", ApiRef, hWnd);
			return TRUE;
		}

		// clear reference to sticky window
		if(dxw.hStickyWindow) dxw.hStickyWindow = 0;

		// v2.05.82: enforce TERMINATEONCLOSE flag
		if((dxw.dwFlags6 & TERMINATEONCLOSE) && !bReverted){
			OutTraceGDI("%s: terminate on destroy main hwnd=%#x\n", ApiRef, hWnd);
			TerminateProcess(GetCurrentProcess(),0);
		}
	}

	// useless ...
	//if(hWnd == wHider){
	//	OutTraceGDI("DestroyWindow: do NOT destroy hider hwnd=%#x\n", hWnd);
	//	return TRUE;
	//}

	if (dxw.hControlParentWnd && (hWnd == dxw.hControlParentWnd)) {
		OutTraceGDI("%s: destroy control parent hwnd=%#x\n", ApiRef, hWnd);
		dxw.hControlParentWnd = NULL;
	}
	res=(*pDestroyWindow)(hWnd);
	IfTraceError();
	if(CurrentActiveMovieWin == hWnd) CurrentActiveMovieWin = NULL;

	if(dxw.dwFlags7 & NOWINERRORS) return TRUE; // v2.03.69: suppress unessential errors
	return res;
}

#ifndef DXW_NOTRACES
static char *ExplainTAAlign(UINT c)
{
	static char eb[256];
	unsigned int l;
	strcpy(eb,"TA_");
	strcat(eb, (c & TA_UPDATECP) ? "UPDATECP+" : "NOUPDATECP+");
	strcat(eb, (c & TA_RIGHT) ? (((c & TA_CENTER) == TA_CENTER) ? "CENTER+" : "RIGHT+") : "LEFT+");
	strcat(eb, (c & TA_BOTTOM) ? "BOTTOM+" : "TOP+");
	if ((c & TA_BASELINE)==TA_BASELINE) strcat(eb, "BASELINE+");
	if (c & TA_RTLREADING) strcat(eb, "RTLREADING+");
	l=strlen(eb);
	eb[l-1]=0; 
	return(eb);
}

static char *ExplainDTFormat(UINT c)
{
	static char eb[256];
	unsigned int l;
	strcpy(eb,"DT_");
	if(!(c & (DT_CENTER|DT_RIGHT))) strcat(eb, "LEFT+");
	if(c & DT_CENTER) strcat(eb, "CENTER+");
	if(c & DT_RIGHT) strcat(eb, "RIGHT+");
	if(!(c & (DT_VCENTER|DT_BOTTOM))) strcat(eb, "TOP+");
	if(c & DT_VCENTER) strcat(eb, "VCENTER+");
	if(c & DT_BOTTOM) strcat(eb, "BOTTOM+");
	if(c & DT_WORDBREAK) strcat(eb, "WORDBREAK+");
	if(c & DT_SINGLELINE) strcat(eb, "SINGLELINE+");
	if(c & DT_EXPANDTABS) strcat(eb, "EXPANDTABS+");
	if(c & DT_TABSTOP) strcat(eb, "TABSTOP+");
	if(c & DT_NOCLIP) strcat(eb, "NOCLIP+");
	if(c & DT_EXTERNALLEADING) strcat(eb, "EXTERNALLEADING+");
	if(c & DT_CALCRECT) strcat(eb, "CALCRECT+");
	if(c & DT_NOPREFIX) strcat(eb, "NOPREFIX+");
	if(c & DT_INTERNAL) strcat(eb, "INTERNAL+");
	l=strlen(eb);
	eb[l-1]=0; 
	return(eb);
}
#endif // DXW_NOTRACES

BOOL gFixed;

// from EmulateDrawText MS shim
long Fix_Coordinate(long nCoord)
{
    if ((nCoord & 0x80000000) && ((nCoord & 0x40000000) == 0)) {
        nCoord &= 0x7FFFFFFF;
    } else if (((nCoord & 0x80000000) == 0) && (nCoord & 0x40000000)) {
        nCoord |= 0x10000000;
    }

    return nCoord;
}

static LPRECT Fix_Coordinates(LPRECT lpRect)
{
    //
    // Check bit 32, if it is on and bit 31 is off or bit 32 is off and
    // bit 31 is on, flip bit 32.
    //
    lpRect->left  = Fix_Coordinate(lpRect->left);
    lpRect->right = Fix_Coordinate(lpRect->right);
    lpRect->top   = Fix_Coordinate(lpRect->top);
    lpRect->bottom= Fix_Coordinate(lpRect->bottom);

    return lpRect;
}

int WINAPI extDrawTextA(HDC hdc, LPCSTR lpchText, int nCount, LPRECT lpRect, UINT uFormat)
{
	int ret;
	ApiName("DrawTextA");

	if(dxw.EmulateWin95){
		if (nCount == 0x0000FFFF) nCount = -1;
		lpRect = Fix_Coordinates(lpRect);
	}

	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d) Format=%#x(%s) Text=(%d)\"%.*s\"\n", 
		ApiRef, hdc, 
		lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, 
		uFormat, ExplainDTFormat(uFormat), 
		nCount, 
		(nCount == -1) ? lstrlenA(lpchText) : nCount, 
		lpchText);

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextA(ApiRef, lpRect->left, lpRect->top, lpchText, nCount);

	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		LPWSTR wstr = NULL;
		int n;
		int size = (nCount == -1) ? lstrlenA(lpchText) : nCount; // v2-05.53 fix
		if(size == 0) return TRUE;
		wstr = (LPWSTR)malloc((size+1)<<1);
		n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpchText, size, wstr, size);
		wstr[n] = (WCHAR)0; // add terminator !!
		_if(n==0) OutTrace("!!! err=%d @%d\n", GetLastError(), __LINE__);
		if(!pDrawTextW) pDrawTextW = DrawTextW;
		ret = extDrawTextW(hdc, wstr, n, lpRect, uFormat);
		free(wstr);
		return ret;
	}

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC){
		//lpADC->AcquireDC();
		ret=(*pDrawTextA)(lpADC->GetDC(hdc), lpchText, nCount, lpRect, uFormat);
		lpADC->ReleaseDC();
		return ret;
	}

    gFixed = TRUE; // semaphore to avoid multiple scaling with HOT patching
	if(dxw.IsToRemap(hdc)){

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pDrawTextA)(sdc.GetHdc(), lpchText, nCount, lpRect, uFormat);
				if(nCount)
					sdc.PutPrimaryDC(hdc, TRUE, lpRect->left, lpRect->top, lpRect->right-lpRect->left, lpRect->bottom-lpRect->top);
				else {
					sdc.PutPrimaryDC(hdc, FALSE); // Diablo makes a DrawText of null string in the intro ...
				}
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				// v2.05.60: don't stretch size calculations
				if((uFormat & DT_CALCRECT)) {
					ret=(*pDrawTextA)(hdc, lpchText, nCount, lpRect, uFormat);
					OutTraceGDI("%s: output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				else {
					dxw.MapClient(lpRect);
					OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
					ret=(*pDrawTextA)(hdc, lpchText, nCount, lpRect, uFormat);
					dxw.UnmapClient((RECT *)lpRect);
					OutTraceGDI("%s: fixed output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				break;
			default:
				ret=(*pDrawTextA)(hdc, lpchText, nCount, lpRect, uFormat);
				break;
		}
	}
	else {
		ret=(*pDrawTextA)(hdc, lpchText, nCount, lpRect, uFormat);
	}
	gFixed = FALSE;

	// if nCount is zero, DrawRect returns 0 as text heigth, but this is not an error! (ref. "Imperialism II")
#ifndef DXW_NOTRACES
	if(nCount && !ret) OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
#endif // DXW_NOTRACES
	return ret;
}

int WINAPI extDrawTextExA(HDC hdc, LPTSTR lpchText, int nCount, LPRECT lpRect, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams)
{
	int ret;
	ApiName("DrawTextExA");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d) DTFormat=%#x Text=(%d)\"%.*s\"\n", 
		ApiRef, hdc, 
		lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, 
		dwDTFormat, 
		nCount, 
		(nCount == -1) ? lstrlenA(lpchText) : nCount, 
		lpchText);
	if (IsDebugGDI){
        if(lpDTParams)
              OutTrace("> DTParams: size=%d (L,R)margins=(%d,%d) TabLength=%d lDrawn=%d\n",
              lpDTParams->cbSize, lpDTParams->iLeftMargin, lpDTParams->iRightMargin,
              lpDTParams->iTabLength, lpDTParams->uiLengthDrawn);
        else
              OutTrace("> DTParams: NULL\n");
	}
#endif

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextA(ApiRef, lpRect->left, lpRect->top, lpchText, nCount);

	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		LPWSTR wstr = NULL;
		int n;
		int size = (nCount == -1) ? lstrlenA(lpchText) : nCount; // v2-05.53 fix
		if(size == 0) return TRUE;
		wstr = (LPWSTR)malloc((size+1)<<1);
		n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpchText, size, wstr, size);
		wstr[n] = (WCHAR)0; // add terminator !!
		_if(n==0) OutTrace("!!! err=%d @%d\n", GetLastError(), __LINE__);
		if(!pDrawTextExW) pDrawTextExW = (DrawTextExW_Type)DrawTextExW;
		ret = extDrawTextExW(hdc, wstr, n, lpRect, dwDTFormat, lpDTParams);
		free(wstr);
		return ret;
	}

    gFixed = TRUE; // semaphore to avoid multiple scaling with HOT patching
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pDrawTextExA)(sdc.GetHdc(), lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
				if(nCount)
					sdc.PutPrimaryDC(hdc, TRUE, lpRect->left, lpRect->top, lpRect->right-lpRect->left, lpRect->bottom-lpRect->top);
				else
					sdc.PutPrimaryDC(hdc, FALSE); // in cases like Diablo that makes a DrawText of null string in the intro ...
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				// v2.05.60: don't stretch size calculations
				if((dwDTFormat & DT_CALCRECT)) {
					ret=(*pDrawTextExA)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
					OutTraceGDI("%s: output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				else {
					dxw.MapClient(lpRect);
					OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
					ret=(*pDrawTextExA)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
					dxw.UnmapClient((RECT *)lpRect);
					OutTraceGDI("%s: fixed output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				break;
			default:
				ret=(*pDrawTextExA)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
				break;
		}
	}
	else {
		ret=(*pDrawTextExA)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
	}
	gFixed = FALSE;

	// if nCount is zero, DrawRect returns 0 as text heigth, but this is not an error! (ref. "Imperialism II")
#ifndef DXW_NOTRACES
	if(nCount && !ret) OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
#endif // DXW_NOTRACES
	return ret;
}

int WINAPI extDrawTextW(HDC hdc, LPCWSTR lpchText, int nCount, LPRECT lpRect, UINT uFormat)
{
	int ret;
	ApiName("DrawTextW");

	if(dxw.EmulateWin95){
		if (nCount == 0x0000FFFF) nCount = -1;
		lpRect = Fix_Coordinates(lpRect);
	}

	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d) Format=%#x(%s) Text=(%d)\"%.*ls\"\n", 
		ApiRef, hdc, 
		lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, 
		uFormat, ExplainDTFormat(uFormat), 
		nCount, 
		(nCount == -1) ? lstrlenW(lpchText) : nCount, 
		lpchText);

	if((dxw.dwFlags14 & DUMPTEXT) && !gFixed) DumpTextW(ApiRef, lpRect->left, lpRect->top, lpchText, nCount);

	gFixed = TRUE; // semaphore to avoid multiple scaling with HOT patching
	if(dxw.IsToRemap(hdc)){

		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pDrawTextW)(sdc.GetHdc(), lpchText, nCount, lpRect, uFormat);
				sdc.PutPrimaryDC(hdc, TRUE, lpRect->left, lpRect->top, lpRect->right-lpRect->left, lpRect->bottom-lpRect->top);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				// v2.05.60: don't stretch size calculations
				if((uFormat & DT_CALCRECT)) {
					ret=(*pDrawTextW)(hdc, lpchText, nCount, lpRect, uFormat);
					OutTraceGDI("%s: output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				else {
					dxw.MapClient(lpRect);
					OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
					ret=(*pDrawTextW)(hdc, lpchText, nCount, lpRect, uFormat);
					dxw.UnmapClient((RECT *)lpRect);
					OutTraceGDI("%s: fixed output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				break;
			default:
				ret=(*pDrawTextW)(hdc, lpchText, nCount, lpRect, uFormat);
				break;
		}
	}
	else {
		ret=(*pDrawTextW)(hdc, lpchText, nCount, lpRect, uFormat);
	}
	gFixed = FALSE;

	// if nCount is zero, DrawRect returns 0 as text heigth, but this is not an error! (ref. "Imperialism II")
#ifndef DXW_NOTRACES
	if(nCount && !ret) OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
#endif // DXW_NOTRACES
	return ret;
}

int WINAPI extDrawTextExW(HDC hdc, LPCWSTR lpchText, int nCount, LPRECT lpRect, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams)
{
	int ret;
	ApiName("DrawTextExW");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d) DTFormat=%#x Text=(%d)\"%ls\"\n", 
		ApiRef, hdc, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, dwDTFormat, nCount, lpchText);
       if (IsDebugGDI){
            if(lpDTParams)
                  OutTrace("DTParams: size=%d (L,R)margins=(%d,%d) TabLength=%d lDrawn=%d\n",
                  lpDTParams->cbSize, lpDTParams->iLeftMargin, lpDTParams->iRightMargin,
                  lpDTParams->iTabLength, lpDTParams->uiLengthDrawn);
            else
                  OutTrace("DTParams: NULL\n");
      }
#endif

	if((dxw.dwFlags14 & DUMPTEXT) && !gFixed) DumpTextW(ApiRef, lpRect->left, lpRect->top, lpchText, nCount);

	gFixed = TRUE; // semaphore to avoid multiple scaling with HOT patching
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC:
				sdc.GetPrimaryDC(hdc);
				ret=(*pDrawTextExW)(sdc.GetHdc(), lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
				sdc.PutPrimaryDC(hdc, TRUE, lpRect->left, lpRect->top, lpRect->right-lpRect->left, lpRect->bottom-lpRect->top);
				return ret;
				break;
			case GDIMODE_STRETCHED: 
				// v2.05.60: don't stretch size calculations
				if((dwDTFormat & DT_CALCRECT)) {
					ret=(*pDrawTextExW)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
					OutTraceGDI("%s: output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				else {
					dxw.MapClient(lpRect);
					OutTraceGDI("%s: fixed rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
					ret=(*pDrawTextExW)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
					dxw.UnmapClient((RECT *)lpRect);
					OutTraceGDI("%s: fixed output rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
				}
				break;
			default:
				ret=(*pDrawTextExW)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
				break;
		}
	}
	else {
		ret=(*pDrawTextExW)(hdc, lpchText, nCount, lpRect, dwDTFormat, lpDTParams);
	}
	gFixed = FALSE;

	// if nCount is zero, DrawRect returns 0 as text heigth, but this is not an error! (ref. "Imperialism II")
#ifndef DXW_NOTRACES
	if(nCount && !ret) OutErrorGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError()); 
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extCloseWindow(HWND hWnd)
{
	// from MSDN: Minimizes (but does not destroy) the specified window.
	BOOL res;
	ApiName("CloseWindow");
	OutDebugDW("%s: hwnd=%#x\n", ApiRef, hWnd);
	if (hWnd == dxw.GethWnd()) {
		OutTraceGDI("%s: close main hwnd=%#x\n", ApiRef, hWnd);
		// do not delete the reference to main hWnd.
	}
	res=(*pCloseWindow)(hWnd);
	IfTraceError();
	return res;
}

BOOL WINAPI extSetSysColors(int cElements, const INT *lpaElements, const COLORREF *lpaRgbValues)
{
	// v2.02.32: added to avoid SysColors changes by "Western Front"
	BOOL res;
	ApiName("SetSysColors");
	OutTraceGDI("%s: Elements=%d\n", ApiRef, cElements);

	if(dxw.dwFlags3 & LOCKSYSCOLORS) return TRUE;

	res=(*pSetSysColors)(cElements, lpaElements, lpaRgbValues);
	IfTraceError();
	return res;
}

BOOL WINAPI extUpdateWindow(HWND hwnd)
{
	BOOL res;
	ApiName("UpdateWindow");
	OutDebugDW("%s: hwnd=%#x\n", ApiRef, hwnd);

	if(dxw.Windowize && dxw.IsRealDesktop(hwnd)){
		OutTraceGDI("%s: remapping hwnd=%#x->%#x\n", ApiRef, hwnd, dxw.GethWnd());
		hwnd=dxw.GethWnd();
	}

	res=(*pUpdateWindow)(hwnd);
	IfTraceError();
	return res;
}

#ifndef DXW_NOTRACES
static char *sRedrawFlags(UINT flags)
{
	static char s[256];
	strcpy(s, "RDW_");
	if(flags & RDW_ERASE) strcat(s, "ERASE+");
	if(flags & RDW_FRAME) strcat(s, "FRAME+");
	if(flags & RDW_INTERNALPAINT) strcat(s, "INTERNALPAINT+");
	if(flags & RDW_INVALIDATE) strcat(s, "INVALIDATE+");
	if(flags & RDW_NOERASE) strcat(s, "NOERASE+");
	if(flags & RDW_NOFRAME) strcat(s, "NOFRAME+");
	if(flags & RDW_NOINTERNALPAINT) strcat(s, "NOINTERNALPAINT+");
	if(flags & RDW_VALIDATE) strcat(s, "VALIDATE+");
	if(flags & RDW_ERASENOW) strcat(s, "ERASENOW+");
	if(flags & RDW_UPDATENOW) strcat(s, "UPDATENOW+");
	if(flags & RDW_ALLCHILDREN) strcat(s, "ALLCHILDREN+");
	if(flags & RDW_NOCHILDREN) strcat(s, "NOCHILDREN+");
	if(strlen(s)>strlen("RDW_")) s[strlen(s)-1]=0;
	else s[0]=0;
	return s;
}
#endif // DXW_NOTRACES

BOOL WINAPI extRedrawWindow(HWND hWnd, const RECT *lprcUpdate, HRGN hrgnUpdate, UINT flags)
{
	RECT rcUpdate;
	BOOL res;
	ApiName("RedrawWindow");

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sRect[81];
		if(lprcUpdate) sprintf(sRect, "(%d,%d)-(%d,%d)", lprcUpdate->left, lprcUpdate->top, lprcUpdate->right, lprcUpdate->bottom);
		else strcpy(sRect, "NULL");
		OutTrace("%s: hwnd=%#x rcupdate=%s hrgn=%#x flags=%#x(%s)\n", ApiRef, hWnd, sRect, hrgnUpdate, flags, sRedrawFlags(flags));
	}
#endif

	// v2.03.64 fix: if hrgnUpdate is set, lprcUpdate is ignored, so it can't be scaled
	// beware: they both could be null, and that means the whole window
	if (!hrgnUpdate && lprcUpdate) rcUpdate = *lprcUpdate;
	// avoid redrawing the whole desktop
	if(dxw.Windowize && dxw.IsRealDesktop(hWnd)) hWnd=dxw.GethWnd();
	if(dxw.IsFullScreen()){
		// v2.03.64 fix: if hrgnUpdate is set, lprcUpdate is ignored, so it can't be scaled
		if (!hrgnUpdate && lprcUpdate) rcUpdate = dxw.MapClientRect((LPRECT)lprcUpdate);
	}

	res = (*pRedrawWindow)(hWnd, lprcUpdate ? &rcUpdate : NULL, hrgnUpdate, flags);
	// v2.04.41: fix error condition is 0 return value
	IfTraceError(); 
	return res;
}

#ifdef TRACEALL
BOOL WINAPI extGetWindowPlacement(HWND hwnd, WINDOWPLACEMENT *lpwndpl)
{
	BOOL ret;
	OutTraceGDI("GetWindowPlacement: hwnd=%#x\n", hwnd);

#ifdef BYPASSED
	if(dxw.IsRealDesktop(hwnd)){
		OutTraceGDI("GetWindowPlacement: remapping hwnd=%#x->%#x\n", hwnd, dxw.GethWnd());
		hwnd=dxw.GethWnd();
	}
#endif // BYPASSED

	ret=(*pGetWindowPlacement)(hwnd, lpwndpl);
	OutTraceGDI("GetWindowPlacement: flags=%#x showCmd=%#x MinPosition=(%d,%d) MaxPosition=(%d,%d) NormalPosition=(%d,%d)-(%d,%d)\n",
		lpwndpl->flags, lpwndpl->showCmd, 
		lpwndpl->ptMinPosition.x, lpwndpl->ptMinPosition.y,
		lpwndpl->ptMaxPosition.x, lpwndpl->ptMaxPosition.y,
		lpwndpl->rcNormalPosition.left, lpwndpl->rcNormalPosition.top, lpwndpl->rcNormalPosition.right, lpwndpl->rcNormalPosition.bottom);

#ifdef BYPASSED
	if (ret && dxw.Windowize && dxw.IsFullScreen()){
		lpwndpl->showCmd = SW_SHOWNORMAL;
		lpwndpl->ptMinPosition.x = -1; lpwndpl->ptMinPosition.y = -1;
		lpwndpl->ptMaxPosition.x = -1; lpwndpl->ptMaxPosition.y = -1;
		OutTraceGDI("GetWindowPlacement: FIXED showCmd=%#x MinPosition=(%d,%d) MaxPosition=(%d,%d) NormalPosition=(%d,%d)-(%d,%d)\n",
			lpwndpl->showCmd, 
			lpwndpl->ptMinPosition.x, lpwndpl->ptMinPosition.y,
			lpwndpl->ptMaxPosition.x, lpwndpl->ptMaxPosition.y,
			lpwndpl->rcNormalPosition.left, lpwndpl->rcNormalPosition.top, 
			lpwndpl->rcNormalPosition.right, lpwndpl->rcNormalPosition.bottom);
	}
#endif // BYPASSED

	if(!ret) OutErrorGDI("GetWindowPlacement: ERROR er=%d\n", GetLastError());
	return ret;
}

BOOL WINAPI extSetWindowPlacement(HWND hwnd, WINDOWPLACEMENT *lpwndpl)
{
	BOOL ret;
	OutTraceGDI("SetWindowPlacement: hwnd=%#x\n", hwnd);

#ifdef BYPASSED
	if(dxw.IsRealDesktop(hwnd)){
		OutTraceGDI("SetWindowPlacement: remapping hwnd=%#x->%#x\n", hwnd, dxw.GethWnd());
		hwnd=dxw.GethWnd();
	}
#endif // BYPASSED

	OutTraceGDI("SetWindowPlacement: flags=%#x showCmd=%#x MinPosition=(%d,%d) MaxPosition=(%d,%d) NormalPosition=(%d,%d)-(%d,%d)\n",
		lpwndpl->flags, lpwndpl->showCmd, 
		lpwndpl->ptMinPosition.x, lpwndpl->ptMinPosition.y,
		lpwndpl->ptMaxPosition.x, lpwndpl->ptMaxPosition.y,
		lpwndpl->rcNormalPosition.left, lpwndpl->rcNormalPosition.top, lpwndpl->rcNormalPosition.right, lpwndpl->rcNormalPosition.bottom);

#ifdef BYPASSED
	switch (lpwndpl->showCmd){
	case SW_MAXIMIZE:
		if (dxw.IsFullScreen()){
			lpwndpl->showCmd = SW_SHOW;
			OutTraceGDI("SetWindowPlacement: forcing SW_SHOW state\n");
		}
		break;
	}
#endif // BYPASSED

	ret=(*pSetWindowPlacement)(hwnd, lpwndpl);
	if(!ret) OutErrorGDI("SetWindowPlacement: ERROR er=%d\n", GetLastError());
	return ret;
}
#endif // BYPASSEDAPI

HWND WINAPI extGetActiveWindow(void)
{
	HWND ret;
	ApiName("GetActiveWindow");

	ret=(*pGetActiveWindow)();
	OutDebugGDI("%s: ret=%#x\n", ApiRef, ret);
	if(dxw.dwFlags10 & SLOWWINPOLLING) dxw.DoSlow(2);
#ifdef KEEPWINDOWSTATES
	if (KEEPWINDOWSTATES && hActiveWindow) {
		OutTraceGDI("GetActiveWindow: ret=%#x->%#x\n", ret, hActiveWindow);
		return hActiveWindow;
	}
#endif
	if((dxw.dwFlags8 & WININSULATION) && dxw.Windowize && dxw.IsFullScreen()) {
		OutTraceGDI("%s: ret=%#x->%#x\n", ApiRef, ret, dxw.GethWnd());
		return dxw.GethWnd();
	}
	return ret;
}

HWND WINAPI extGetForegroundWindow(void)
{
	HWND ret;
	ApiName("GetForegroundWindow");

	ret=(*pGetForegroundWindow)();
	OutDebugDW("%s: ret=%#x\n", ApiRef, ret);
	if(dxw.dwFlags10 & SLOWWINPOLLING) dxw.DoSlow(2);
#ifdef KEEPWINDOWSTATES
	if (KEEPWINDOWSTATES && hForegroundWindow) {
		OutTraceGDI("GetForegroundWindow: ret=%#x->%#x\n", ret, hForegroundWindow);
		return hForegroundWindow;
	}
#endif
	if((dxw.dwFlags8 & WININSULATION) && dxw.Windowize && dxw.IsFullScreen()) {
		OutTraceGDI("%s: ret=%#x->%#x\n", ApiRef, ret, dxw.GethWnd());
		if (dxw.GethWnd()) return dxw.GethWnd(); // v2.04.80: avoid setting a NULL main window
	}
	return ret;
}

#ifdef TRACESYSCALLS

HWND WINAPI extSetCapture(HWND hwnd)
{
	HWND ret;
	ApiName("SetCapture");
	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, hwnd);
	ret=(*pSetCapture)(hwnd);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extReleaseCapture(void)
{
	BOOL ret;
	ApiName("ReleaseCapture");
	OutTraceGDI("%s\n", ApiRef);
	ret=(*pReleaseCapture)();
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

#ifndef DXW_NOTRACES
static char *ExplainGAFlags(UINT f)
{	
	char *s;
	switch(f){
		case GA_PARENT: s="PARENT"; break;
		case GA_ROOT: s="ROOT"; break;
		case GA_ROOTOWNER: s="ROOTOWNER"; break;
		default: s="???";
	}
	return s;
}
#endif // DXW_NOTRACES

HWND WINAPI extGetAncestor(HWND hwnd, UINT gaFlags)
{
	HWND ret;
	ApiName("GetAncestor");
	OutTraceGDI("%s: hwnd=%#x flags=%#x(%s)\n", ApiRef, hwnd, gaFlags, ExplainGAFlags(gaFlags));
	ret=(*pGetAncestor)(hwnd, gaFlags);
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

#ifdef TRACEWINDOWS
HWND WINAPI extGetFocus(void)
{
	HWND ret;
	ApiName("GetFocus");

	ret=(*pGetFocus)();
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}
#endif // TRACEWINDOWS

#ifdef TRACEWINDOWS
HWND WINAPI extSetFocus(HWND hWnd)
{
	HWND ret;
	ApiName("SetFocus");

	ret=(*pSetFocus)(hWnd);
	OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hWnd, ret);
	return ret;
}
#endif // TRACEWINDOWS

#ifndef DXW_NOTRACES
char *sGetWinCmd(UINT c)
{
	static char *capt[] = {
		"HWNDFIRST", "HWNDLAST", "HWNDNEXT", "HWNDPREV", "OWNER", "CHILD", "ENABLEDPOPUP", "???"
	};
	if(c > GW_ENABLEDPOPUP) c = GW_ENABLEDPOPUP + 1;
	return capt[c];
}
#endif // DXW_NOTRACES

HWND WINAPI extGetWindow(HWND hWnd, UINT uCmd)
{
	HWND ret;
	ApiName("GetWindow");

	ret=(*pGetWindow)(hWnd, uCmd);
	OutTraceGDI("%s: hwnd=%#x cmd=%#x(%s) ret=%#x\n", ApiRef, hWnd, uCmd, sGetWinCmd(uCmd), ret);
	return ret;
}

DWORD WINAPI extGetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId)
{
	DWORD ret;
	ApiName("GetWindowThreadProcessId");

	ret=(*pGetWindowThreadProcessId)(hWnd, lpdwProcessId);
	if(lpdwProcessId) OutTraceGDI("%s: pid=%#x\n", ApiRef, *lpdwProcessId);
	OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hWnd, ret);
	return ret;
}

BOOL WINAPI extIsWindow(HWND hWnd)
{
	BOOL ret;
	ApiName("IsWindow");

	ret=(*pIsWindow)(hWnd);
	OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hWnd, ret);
	return ret;
}

BOOL WINAPI extIsWindowEnabled(HWND hWnd)
{
	BOOL ret;
	ApiName("IsWindowEnabled");

	ret=(*pIsWindowEnabled)(hWnd);
	OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hWnd, ret);
	return ret;
}

BOOL WINAPI extIsIconic(HWND hWnd)
{
	BOOL ret;
	ApiName("IsIconic");

	ret = (*pIsIconic)(hWnd);
	OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hWnd, ret);
	return ret;
}

HWND WINAPI extSetActiveWindow(HWND hwnd)
{
	HWND ret;
	ApiName("SetActiveWindow");

	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, hwnd);
	ret=(*pSetActiveWindow)(hwnd);
#ifdef KEEPWINDOWSTATES
	if (KEEPWINDOWSTATES) hActiveWindow = hwnd;
#endif
	OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}
#endif // TRACESYSCALLS

BOOL WINAPI extIsZoomed(HWND hWnd)
{
	BOOL ret;
	ApiName("IsZoomed");

	ret = (*pIsZoomed)(hWnd);
	OutTraceGDI("%s: hwnd=%#x ret=%#x\n", ApiRef, hWnd, ret);

	// v2.06.01: commented, ret = TRUE prevents "Tie Break Tennis 2" main task to run.
	//if(dxw.IsFullScreen()){
	//	RECT rect;
	//	(*pGetClientRect)(hWnd, &rect);
	//	dxw.UnmapClient(&rect);
	//	if(((DWORD)rect.right >= dxw.GetScreenWidth()) && ((DWORD)rect.bottom >= dxw.GetScreenHeight())){
	//		OutTraceGDI("%s: hwnd=%#x ret=%#x -> %x\n", ApiRef, hWnd, ret, TRUE);
	//		ret = TRUE;
	//	}
	//}

	return ret;
}

BOOL WINAPI extIsWindowVisible(HWND hwnd)
{
	BOOL ret;
	ApiName("IsWindowVisible");

	ret=(*pIsWindowVisible)(hwnd);
	OutDebugDW("%s: hwnd=%#x ret=%#x\n", ApiRef, hwnd, ret);
	while(!ret){
		// v2.04.32 
		if(dxw.dwFlags9 & MAKEWINVISIBLE){
			OutTraceGDI("%s: MAKEWINVISIBLE ret=TRUE\n", ApiRef);
			//(*pShowWindow)(hwnd, SW_SHOWNORMAL);
			SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
			ret=TRUE;
			break;
		}
		if(dxw.IsDesktop(hwnd) && (dxw.dwFlags8 & PRETENDVISIBLE) && !ret){
			OutTraceGDI("%s: PRETENDVISIBLE ret=TRUE\n", ApiRef);
			ret=TRUE;
			break;
		}
		break;
	}
	return ret;
}

BOOL WINAPI intSystemParametersInfo(char *api, SystemParametersInfo_Type pSystemParametersInfo, 
									UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	BOOL res;
	OutTraceGDI("%s: Action=0x%04.4X Param=0x%X WinIni=0x%X\n", api, uiAction, uiParam, fWinIni);
	switch(uiAction){
		case SPI_SETKEYBOARDDELAY:
		case SPI_SETKEYBOARDSPEED:
		case SPI_SETSCREENSAVERRUNNING: // v2.03.75 used by Dethkarz, but not really necessary
		case SPI_SETWORKAREA: // v2.05.05 very nasty effect, used by "Jetboat Superchamps 2"
			OutTraceGDI("%s: bypass action=0x%04.4X\n", api, uiAction);
			return TRUE;
			break;
	}

	// some SPI_SETxx cases are commented out since already excluded above 
	if(dxw.dwFlags11 & LOCKSYSSETTINGS){
		switch (uiAction){
			case SPI_SETBEEP:
			case SPI_SETMOUSE:
			case SPI_SETBORDER:
			//case SPI_SETKEYBOARDSPEED:
			case SPI_SETSCREENSAVETIMEOUT:
			case SPI_SETSCREENSAVEACTIVE:
			case SPI_SETGRIDGRANULARITY:
			case SPI_SETDESKWALLPAPER:
			case SPI_SETDESKPATTERN:
			//case SPI_SETKEYBOARDDELAY:
			case SPI_SETICONTITLEWRAP:
			case SPI_SETMENUDROPALIGNMENT:
			case SPI_SETDOUBLECLKWIDTH:
			case SPI_SETDOUBLECLKHEIGHT:
			case SPI_SETDOUBLECLICKTIME:
			case SPI_SETMOUSEBUTTONSWAP:
			case SPI_SETICONTITLELOGFONT:
			case SPI_SETFASTTASKSWITCH:
			case SPI_SETDRAGFULLWINDOWS:
			case SPI_SETNONCLIENTMETRICS:
			case SPI_SETMINIMIZEDMETRICS:
			case SPI_SETICONMETRICS:
			//case SPI_SETWORKAREA:
			case SPI_SETPENWINDOWS:
			case SPI_SETHIGHCONTRAST:
			case SPI_SETKEYBOARDPREF:
			case SPI_SETSCREENREADER:
			case SPI_SETANIMATION:
			case SPI_SETFONTSMOOTHING:
			case SPI_SETDRAGWIDTH:
			case SPI_SETDRAGHEIGHT:
			case SPI_SETHANDHELD:
			case SPI_SETLOWPOWERTIMEOUT:
			case SPI_SETPOWEROFFTIMEOUT:
			case SPI_SETLOWPOWERACTIVE:
			case SPI_SETPOWEROFFACTIVE:
			case SPI_SETCURSORS:
			case SPI_SETICONS:
			case SPI_SETDEFAULTINPUTLANG:
			case SPI_SETLANGTOGGLE:
			case SPI_SETMOUSETRAILS:
			//case SPI_SETSCREENSAVERRUNNING:
			case SPI_SETFILTERKEYS:
			case SPI_SETTOGGLEKEYS:
			case SPI_SETMOUSEKEYS:
			case SPI_SETSHOWSOUNDS:
			case SPI_SETSTICKYKEYS:
			case SPI_SETACCESSTIMEOUT:
			case SPI_SETSERIALKEYS:
			case SPI_SETSOUNDSENTRY:
			case SPI_SETSNAPTODEFBUTTON:
			case SPI_SETMOUSEHOVERWIDTH:
			case SPI_SETMOUSEHOVERHEIGHT:
			case SPI_SETMOUSEHOVERTIME:
			case SPI_SETWHEELSCROLLLINES:
			case SPI_SETMENUSHOWDELAY:
			case SPI_SETWHEELSCROLLCHARS:
			case SPI_SETSHOWIMEUI:
			case SPI_SETMOUSESPEED:
			case SPI_SETAUDIODESCRIPTION:
			case SPI_SETSCREENSAVESECURE:
			// there are more for WINVER >= 0x0500
				OutTraceGDI("%s: lock action=0x%04.4X\n", api, uiAction);
				return TRUE;
				break;
			default:
				break;
		}
	}

	res=(*pSystemParametersInfo)(uiAction, uiParam, pvParam, fWinIni);
	if(uiAction==SPI_GETWORKAREA){
		LPRECT cli = (LPRECT)pvParam;
		*cli = dxw.GetScreenRect();
		OutTraceGDI("%s: resized client workarea rect=(%d,%d)-(%d,%d)\n", api, cli->left, cli->top, cli->right, cli->bottom);
	}
	IfTraceError();
	return res;
}

BOOL WINAPI extSystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{ ApiName("SystemParametersInfoA"); return intSystemParametersInfo(ApiRef, pSystemParametersInfoA, uiAction, uiParam, pvParam, fWinIni); }
BOOL WINAPI extSystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{ ApiName("SystemParametersInfoW"); return intSystemParametersInfo(ApiRef, pSystemParametersInfoW, uiAction, uiParam, pvParam, fWinIni); }

UINT_PTR WINAPI extSetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
	UINT uShiftedElapse;
	UINT_PTR ret;
	ApiName("SetTimer");
	// beware: the quicker the time flows, the more the time clicks are incremented,
	// and the lesser the pauses must be lasting! Shift operations are reverted in
	// GetSystemTime vs. Sleep or SetTimer
	uShiftedElapse = dxw.StretchTime(uElapse);
	OutTraceT("%s: hwnd=%#x TimerFunc=%#x elapse=%d->%d timeshift=%d\n", ApiRef, hWnd, lpTimerFunc, uElapse, uShiftedElapse, dxw.TimeShift);
	ret = (*pSetTimer)(hWnd, nIDEvent, uShiftedElapse, lpTimerFunc);
	if(ret) dxw.PushTimer(hWnd, ret, uElapse, lpTimerFunc);
	OutTraceT("%s: IDEvent=%#x ret=%#x\n", ApiRef, nIDEvent, ret);
	return ret;
}

BOOL WINAPI extKillTimer(HWND hWnd, UINT_PTR uIDEvent)
{
	BOOL ret;
	ApiName("KillTimer");

	OutTraceT("%s: hwnd=%#x IDEvent=%#x\n", ApiRef, hWnd, uIDEvent); 
	ret = (*pKillTimer)(hWnd, uIDEvent);
	OutTraceT("%s: ret=%#x\n", ApiRef, ret);
	if(ret) dxw.PopTimer(hWnd, uIDEvent);
	return ret;
}

BOOL WINAPI extGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL bErase)
{
	BOOL ret;
	ApiName("GetUpdateRect");

	OutTraceGDI("%s: hwnd=%#x lprect=%#x Erase=%#x\n", ApiRef, hWnd, lpRect, bErase); 
	ret = (*pGetUpdateRect)(hWnd, lpRect, bErase);
	// v2.05.50: fix to handle possibly NULL lpRect value, used to test the existence of an updated rect
	// ref. https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getupdaterect
	// fixes Silent Storm 2 & 3
	// v2.05.58: better logging
	if(lpRect){
		OutTraceGDI("%s: ret=%d(%s) rect=(%d,%d)-(%d,%d)\n", 
			ApiRef, ret, ret ? "NOTEMPTY" : "EMPTY",
			lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		if(dxw.IsFullScreen() && ret){
			dxw.UnmapClient(lpRect);
			OutTraceGDI("%s: FIXED rect=(%d,%d)-(%d,%d)\n", ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		}
	}
	else
		OutErrorGDI("%s: ret=%d(%s)\n", ApiRef, ret, ret ? "NOTEMPTY" : "EMPTY");

	return ret;
}

BOOL WINAPI extGetCursorInfo(PCURSORINFO pci)
{
	BOOL ret;
	ApiName("GetCursorInfo");

	OutTraceGDI("%s\n", ApiRef); 
	ret = (*pGetCursorInfo)(pci);
	if(ret){
		OutTraceGDI("%s: flags=%#x hcursor=%#x pos=(%d,%d)\n", 
			ApiRef, pci->flags, pci->hCursor, pci->ptScreenPos.x, pci->ptScreenPos.y);
		if(dxw.IsFullScreen()){
			dxw.UnmapClient(&(pci->ptScreenPos));
			OutTraceGDI("%s: FIXED pos=(%d,%d)\n", ApiRef, pci->ptScreenPos.x, pci->ptScreenPos.y);
		}
	}
	else
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

HWND WINAPI extWindowFromPoint(POINT Point)
{
	HWND ret;
	ApiName("WindowFromPoint");

	OutTraceGDI("%s: point=(%d,%d)\n", ApiRef, Point.x, Point.y); 
	if(dxw.IsFullScreen()){
		dxw.MapWindow(&Point); // v2.03.69 fix
		OutTraceGDI("%s: FIXED point=(%d,%d)\n", ApiRef, Point.x, Point.y);
	}
	ret = (*pWindowFromPoint)(Point);
	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, ret);
	return ret;
}

HWND WINAPI extChildWindowFromPoint(HWND hWndParent, POINT Point)
{
	HWND ret;
	ApiName("ChildWindowFromPoint");

	OutTraceGDI("%s: hWndParent=%#x point=(%d,%d)\n", ApiRef, hWndParent, Point.x, Point.y); 
	if(dxw.IsDesktop(hWndParent) && dxw.IsFullScreen() && dxw.Windowize){
		dxw.MapClient(&Point);
		OutTraceGDI("%s: FIXED point=(%d,%d)\n", ApiRef, Point.x, Point.y);
	}
	ret = (*pChildWindowFromPoint)(hWndParent, Point);
	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, ret);
	return ret;
}

HWND WINAPI extChildWindowFromPointEx(HWND hWndParent, POINT Point, UINT uFlags)
{
	HWND ret;
	ApiName("ChildWindowFromPointEx");

	OutTraceGDI("%s: hWndParent=%#x point=(%d,%d) flags=%#x\n", ApiRef, hWndParent, Point.x, Point.y, uFlags); 
	if(dxw.IsDesktop(hWndParent) && dxw.IsFullScreen() && dxw.Windowize){
		dxw.UnmapClient(&Point);
		OutTraceGDI("%s: FIXED point=(%d,%d)\n", ApiRef, Point.x, Point.y);
	}
	ret = (*pChildWindowFromPointEx)(hWndParent, Point, uFlags);
	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, ret);
	return ret;
}

BOOL extGetMonitorInfo(char *api, GetMonitorInfo_Type pGetMonitorInfo, HMONITOR hMonitor, LPMONITORINFO lpmi)
{
	BOOL res, realres;
	OutTraceGDI("%s: hMonitor=%#x mi=MONITORINFO%s\n", api, hMonitor, lpmi->cbSize==sizeof(MONITORINFO)?"":"EX");
	res=realres=(*pGetMonitorInfo)(hMonitor, lpmi);

	//v2.03.15 - must fix the coordinates also in case of error: that may depend on the windowed mode.
	if(dxw.isScaled){
		if(res){
			OutTraceGDI("%s: FIX Work=(%d,%d)-(%d,%d) Monitor=(%d,%d)-(%d,%d) -> (%d,%d)-(%d,%d)\n", 
				api, lpmi->rcWork.left, lpmi->rcWork.top, lpmi->rcWork.right, lpmi->rcWork.bottom,
				lpmi->rcMonitor.left, lpmi->rcMonitor.top, lpmi->rcMonitor.right, lpmi->rcMonitor.bottom,
				0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight());
		} else {
			OutTraceGDI("%s: ERROR err=%d FIX Work&Monitor -> (%d,%d)-(%d,%d) res=OK\n", 
				api, GetLastError(), 0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight());
		}
		res = TRUE;
		lpmi->rcWork = dxw.GetScreenRect();
		lpmi->rcMonitor = dxw.GetScreenRect();
	}
		
	if(realres) {
		LPMONITORINFOEXA lpmia;
		LPMONITORINFOEXW lpmiw;
		OutTraceGDI("%s: Work=(%d,%d)-(%d,%d) Monitor=(%d,%d)-(%d,%d)\n", api,
			lpmi->rcWork.left, lpmi->rcWork.top, lpmi->rcWork.right, lpmi->rcWork.bottom,
			lpmi->rcMonitor.left, lpmi->rcMonitor.top, lpmi->rcMonitor.right, lpmi->rcMonitor.bottom);
		switch(lpmi->cbSize){
			case sizeof(MONITORINFOEXA): 
				lpmia = (LPMONITORINFOEXA)lpmi;
				OutTraceGDI("%s: Monitor=\"%s\"\n", api, lpmia->szDevice);
				break;
			case sizeof(MONITORINFOEXW): 
				lpmiw = (LPMONITORINFOEXW)lpmi;
				OutTraceGDI("%s: Monitor=\"%ls\"\n", api, lpmiw->szDevice);
				break;
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", api, GetLastError());
	}

	return res;
}

BOOL WINAPI extGetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi)
{ ApiName("GetMonitorInfoA"); return extGetMonitorInfo(ApiRef, pGetMonitorInfoA, hMonitor, lpmi); }
BOOL WINAPI extGetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi)
{ ApiName("GetMonitorInfoW"); return extGetMonitorInfo(ApiRef, pGetMonitorInfoW, hMonitor, lpmi); }

int WINAPI extGetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
	int regionType;
	ApiName("GetUpdateRgn");

	regionType=(*pGetUpdateRgn)(hWnd, hRgn, bErase);
	OutTraceGDI("%s: hwnd=%#x hrgn=%#x erase=%#x regionType=%#x(%s)\n", 
		ApiRef, hWnd, hRgn, bErase, regionType, ExplainRegionType(regionType));    

	if(dxw.IsFullScreen() && (dxw.GDIEmulationMode != GDIMODE_NONE)){
		HRGN hrgnScaled = CreateRectRgn(0, 0, 0, 0);
		hrgnScaled = dxw.UnmapRegion(ApiRef, hRgn);
		CombineRgn(hRgn, hrgnScaled, NULL, RGN_COPY);
		(*pDeleteObject)(hrgnScaled);
	} 

    return regionType; 
}

#ifdef NOUNHOOKED
BOOL WINAPI extValidateRect(HWND hWnd, const RECT *lpRect)
{
	BOOL ret;
	if(IsTraceGDI){
		if(lpRect)
			OutTrace("ValidateRect: hwnd=%#x rect=(%d,%d)-(%d,%d)\n", 
				hWnd, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		else
			OutTrace("ValidateRect: hwnd=%#x rect=NULL\n", hWnd);
	}
	ret = (*pValidateRect)(hWnd, lpRect);
	return ret;
}
#endif

BOOL WINAPI extBringWindowToTop(HWND hwnd)
{
	BOOL res;
	ApiName("BringWindowToTop");

	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, hwnd);
	if(dxw.dwFlags5 & UNLOCKZORDER) return TRUE;
	res=(*pBringWindowToTop)(hwnd);
	return res;
}

BOOL WINAPI extSetForegroundWindow(HWND hwnd)
{
	BOOL res;
	ApiName("SetForegroundWindow");

	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, hwnd);
#ifdef KEEPWINDOWSTATES
	hForegroundWindow = hwnd;
#endif
	if(dxw.dwFlags5 & UNLOCKZORDER) return TRUE;
	res=(*pSetForegroundWindow)(hwnd);
	return res;
}

/*
HOOKPROC glpMouseHookProcessFunction;
LRESULT CALLBACK extMouseHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	OutTrace("HookProc intercepted: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
    MOUSEHOOKSTRUCT * pMouseStruct = (MOUSEHOOKSTRUCT *)lParam;
    if (pMouseStruct != NULL){
		dxw.UnmapWindow(&(pMouseStruct->pt));
    }	
	ret= (*glpMouseHookProcessFunction)(code, wParam, lParam);
	return ret;
}
*/

#ifndef DXW_NOTRACES
#ifndef WH_HARDWARE
#define WH_HARDWARE 8
#endif

static char *sWinHookLabel(int id)
{
	char *s;
	switch(id){
		case WH_MSGFILTER:			s = "MSGFILTER";		break;
		case WH_JOURNALRECORD:		s = "JOURNALRECORD";	break;
		case WH_JOURNALPLAYBACK:	s = "WJOURNALPLAYBACK"; break;
		case WH_KEYBOARD:			s = "WKEYBOARD";		break;
		case WH_GETMESSAGE:			s = "GETMESSAGE";		break;
		case WH_CALLWNDPROC:		s = "CALLWNDPROC";		break;
		case WH_CBT:				s = "CBT";				break;
		case WH_SYSMSGFILTER:		s = "SYSMSGFILTER";		break;
		case WH_MOUSE:				s = "MOUSE";			break;
		case WH_HARDWARE:			s = "HARDWARE";			break;
		case WH_DEBUG:				s = "DEBUG";			break;
		case WH_SHELL:				s = "SHELL";			break;
		case WH_FOREGROUNDIDLE:		s = "FOREGROUNDIDLE";	break;
		case WH_CALLWNDPROCRET:		s = "CALLWNDPROCRET";	break;
		case WH_KEYBOARD_LL:		s = "KEYBOARD_LL";		break;
		case WH_MOUSE_LL:			s = "MOUSE_LL";			break;
		default:					s = "unknown";			break;
	}
	return s;
}
#endif // DXW_NOTRACES

HOOKPROC glpMessageHookProcessFunction;
HOOKPROC glpMouseHookProcessFunction;
HOOKPROC glpMouseHookProcessFunctionLL;
HOOKPROC glpCBTHookProcessFunction;
HOOKPROC glpKeyboardHookProcessFunctionLL;
HOOKPROC glpKeyboardHookProcessFunction;
HOOKPROC glpMsgFilterHookProcessFunction;
#ifdef HOOKWINDOWSHOOKPROCS
HOOKPROC glpWindowsHookProcessFunction;
HOOKPROC glpWindowsHookProcessRetFunction;
#endif // HOOKWINDOWSHOOKPROCS

#if 0
LRESULT CALLBACK extMsgFilterHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	//OutTraceGDI("MsgFilterHookProc: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
	OutTrace("MsgFilterHookProc: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
    MSG * pMessage = (MSG *)lParam;
	ret = NULL;
	if(pMessage){
		UINT message = pMessage->message;
		if ((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST)){		// mouse messages
			//OutTrace("MsgFilterHookProc: MOUSE message\n");
			//lParam = 0;
		}
	}
	ret = (*glpMsgFilterHookProcessFunction)(code, wParam, lParam);
	return ret;
}
#endif

LRESULT CALLBACK extEASportsMessageHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	OutTraceGDI("MessageHookProc: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
    MSG * pMessage = (MSG *)lParam;
	ret = NULL;
	if(pMessage){
		UINT message = pMessage->message;
		if ((message >= 0x600) ||											// custom messages
			((message >= WM_KEYFIRST) && (message <= WM_KEYLAST)) ||		// keyboard messages
			((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST))		// mouse messages
			)			
			ret = (*glpMessageHookProcessFunction)(code, wParam, lParam);
	}
	return ret;
}

LRESULT CALLBACK extMessageHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	OutTraceGDI("MessageHookProc: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);
    MSG *pMessage = (MSG *)lParam;
	ret = NULL;
	if(pMessage && dxw.IsFullScreen()){
		UINT message = pMessage->message;
		POINT pt;
		(*pGetCursorPos)(&pt);
#ifdef PROVENTOBEUSEFUL
		if(dxw.dwFlags20 & CENTERONEXIT){
			if((pt.x < dxw.iPosX) || (pt.x >= dxw.iPosX+dxw.iSizX)) pt.x = dxw.iPosX + (dxw.iSizX / 2);
			if((pt.y < dxw.iPosY) || (pt.y >= dxw.iPosY+dxw.iSizY)) pt.y = dxw.iPosY + (dxw.iSizY / 2);
		}
#endif
		// pt.x -= dxw.iPosX;
		// pt.y -= dxw.iPosY;
		pMessage->pt = pt;
		if ((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST)) {		// mouse messages
			pMessage->lParam = MAKELPARAM(pt.x, pt.y);
		}
		ret = (*glpMessageHookProcessFunction)(code, wParam, lParam);
	}
	return ret;
}

static POINT FixMousePoint(POINT pt)
{
	STEP;
	dxw.UnmapWindow(&pt);
#ifdef PROVENTOBEUSEFUL
	if(dxw.dwFlags20 & CENTERONEXIT){
		if((pt.x < 0) || (pt.x >= (LONG)dxw.GetScreenWidth())) pt.x = dxw.GetScreenWidth() / 2;
		if((pt.y < 0) || (pt.y >= (LONG)dxw.GetScreenHeight())) pt.y = dxw.GetScreenHeight() / 2;
		return pt;
	}
#endif
	if(pt.x < 0) pt.x = 0;
	if(pt.x >= (LONG)dxw.GetScreenWidth()) pt.x = dxw.GetScreenWidth()-1;
	if(pt.y < 0) pt.y = 0;
	if(pt.y >= (LONG)dxw.GetScreenHeight()) pt.y = dxw.GetScreenHeight()-1;
	return pt;
}

LRESULT CALLBACK extMouseHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	OutTraceC("MouseHookProc: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);

	if(lParam){
		MOUSEHOOKSTRUCT MouseStruct = *(MOUSEHOOKSTRUCT *)lParam;
		MouseStruct.pt = FixMousePoint(MouseStruct.pt);
		OutTraceC("MouseHookProc: event=%s pos=(%d,%d)->(%d,%d)\n", 
			ExplainWinMessage(wParam), 
			((MOUSEHOOKSTRUCT *)lParam)->pt.x,  ((MOUSEHOOKSTRUCT *)lParam)->pt.y, 
			MouseStruct.pt.x, MouseStruct.pt.y);
		return (*glpMouseHookProcessFunction)(code, wParam, (LPARAM)&MouseStruct);
	}
	return (*glpMouseHookProcessFunction)(code, wParam, lParam);
}

LRESULT CALLBACK extMouseHookProcLL(int code, WPARAM wParam, LPARAM lParam)
{
	OutTraceC("MouseHookProcLL: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);

	if(lParam){
		MSLLHOOKSTRUCT MouseStruct = *(MSLLHOOKSTRUCT *)lParam;
		MouseStruct.pt = FixMousePoint(MouseStruct.pt);
		OutTraceC("MouseHookProcLL: event=%s pos=(%d,%d)->(%d,%d)\n", 
			ExplainWinMessage(wParam), 
			((MSLLHOOKSTRUCT *)lParam)->pt.x,  ((MSLLHOOKSTRUCT *)lParam)->pt.y, 
			MouseStruct.pt.x, MouseStruct.pt.y);
		return (*glpMouseHookProcessFunctionLL)(code, wParam, (LPARAM)&MouseStruct);
	}
    return (*glpMouseHookProcessFunctionLL)(code, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProcessFunctionLL(int code, WPARAM wParam, LPARAM lParam)
{
	OutDebugC("KeyboardHookProcessFunctionLL: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);

	switch(wParam){
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			// v2.03.54: disable the disable Alt-Tab fix
			// process the WM_SYSKEYDOWN/UP events only - needed by "Recoil".
			// v2.05.06: moved inside DxWnd hook
			if(dxw.dwFlags7 & NODISABLEALTTAB){
				OutTraceGDI("KeyboardHookProcessFunctionLL: NODISABLEALTTAB skip SYSKEY\n");
				return 0;
			}
			break;
		case VK_PRINT:
		case VK_SNAPSHOT:
			if(dxw.dwFlags11 & NODISABLEPRINT){
				OutTraceGDI("KeyboardHookProcessFunction: skip PRINT key\n");
				return 0;
			}
			break;
	}

	return (*glpKeyboardHookProcessFunctionLL)(code, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProcessFunction(int code, WPARAM wParam, LPARAM lParam)
{
	ApiName("dxw.KeyboardHookProcessFunction");
	OutDebugC("%s: code=%#x wParam=%#x lParam=%#x\n", ApiRef, code, wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);

	switch(wParam){
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			if(dxw.dwFlags7 & NODISABLEALTTAB){
				OutTraceGDI("%s: NODISABLEALTTAB skip SYSKEY\n", ApiRef);
				return 0;
			}
			break;
		case VK_PRINT:
		case VK_SNAPSHOT:
			if(dxw.dwFlags11 & NODISABLEPRINT){
				OutTraceGDI("%s: skip PRINT key\n", ApiRef);
				return 0;
			}
			break;
	}

	// v2.05.70: handling of function keys here in case the program starts a KEYBOARD hook
	// Fixes "Wizardry 8" handling of function keys.
	//OutTrace("%s: code=%#x lparam=%#x key=%x\n", ApiRef, code, lParam, wParam);
	if (dxw.dwFlags4 & ENABLEHOTKEYS){
		// 0xE0000000 = bit mask for bits 29-31
		// 0x20000000 = Alt key is down, previous state up, key down
		if((lParam & 0xE0000000) == 0x20000000){
			//OutTrace("%s: wparam=%#x lparam=%#x\n", ApiRef, wParam, lParam);		
			HandleHotKeys(dxw.GethWnd(), WM_SYSKEYDOWN, lParam, wParam);
		}
	}

	return (*glpKeyboardHookProcessFunction)(code, wParam, lParam);
}

#ifndef DXW_NOTRACES
static char *ExplainCBTCode(int code)
{
	char *sCodes[]={
		"MOVESIZE",
		"MINMAX",
		"QS",
		"CREATEWND",
		"DESTROYWND",
		"ACTIVATE",
		"CLICKSKIPPED",
		"KEYSKIPPED",
		"SYSCOMMAND",
		"SETFOCUS",
		"unknown"
	};

	if((code < 0) || (code > HCBT_SETFOCUS)) return sCodes[HCBT_SETFOCUS+1];
	return sCodes[code];
}
#endif // DXW_NOTRACES

LRESULT CALLBACK extCBTHookProcessFunction(int code, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	LPCBT_CREATEWNDA lpCBWndA;
	POINT pt;

	OutTraceGDI("CBTHookProcess: code=%#x(%s) wParam=%#x lParam=%#x\n", code, ExplainCBTCode(code), wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);

	switch(code){
		case HCBT_CREATEWND:
			lpCBWndA = (LPCBT_CREATEWNDA) lParam;
			OutDebugGDI("CBTHookProcess: CREATE pt=(%d,%d) sz=(%d,%d)\n", 
				lpCBWndA->lpcs->x, lpCBWndA->lpcs->y, lpCBWndA->lpcs->cx, lpCBWndA->lpcs->cy);
			pt.x = lpCBWndA->lpcs->x;
			pt.y = lpCBWndA->lpcs->y;
			dxw.MapClient(&pt);
			lpCBWndA->lpcs->x = pt.x;
			lpCBWndA->lpcs->y = pt.y;
			pt.x = lpCBWndA->lpcs->cx;
			pt.y = lpCBWndA->lpcs->cy;
			dxw.MapClient(&pt);
			lpCBWndA->lpcs->cx = pt.x;
			lpCBWndA->lpcs->cy = pt.y;
			OutDebugGDI("CBTHookProcess: FIXED pt=(%d,%d) sz=(%d,%d)\n", 
				lpCBWndA->lpcs->x, lpCBWndA->lpcs->y, lpCBWndA->lpcs->cx, lpCBWndA->lpcs->cy);
			break;
		default:
			break;
	}

    ret = (*glpCBTHookProcessFunction)(code, wParam, lParam);
	return 0;
}

#ifdef HOOKWINDOWSHOOKPROCS

LRESULT CALLBACK extWindowsHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	OutTraceGDI("HookWndProc: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);

	if(lParam){
		CWPSTRUCT *cwp;
		WINDOWPOS *lpwp;
		CREATESTRUCTA *lpcs;
		int width, height;
		cwp = (LPCWPSTRUCT)lParam;
		OutTraceGDI("HookWndProc: msg=%#x(%s) wParam=%#x lParam=%#x hwnd=%#x\n", 
			cwp->message, ExplainWinMessage(cwp->message), cwp->wParam, cwp->lParam, cwp->hwnd);
		switch(cwp->message){
			case WM_SIZE:
				width = LOWORD(cwp->lParam);
				height = HIWORD(cwp->lParam);
				OutTraceGDI("HookWndProc: original WM_SIZE size=(%dx%d)\n", width, height);
				dxw.UnmapClient(&width, &height);
				cwp->lParam = MAKELPARAM(width, height);
				OutTraceGDI("HookWndProc: remapped WM_SIZE size=(%dx%d)\n", width, height);
				break;
			case WM_WINDOWPOSCHANGING:
			case WM_WINDOWPOSCHANGED:
				lpwp = (LPWINDOWPOS)cwp->lParam;
				OutTraceGDI("HookWndProc: original WM_WINDOWPOSCHANGx flags=%#x pos=(%d,%d) size=(%dx%d)\n", 
					lpwp->flags, lpwp->x, lpwp->y, lpwp->cx, lpwp->cy);
				if(!(lpwp->flags & (SWP_NOSIZE|SWP_NOMOVE))){
					if(dxw.IsDesktop(cwp->hwnd)){
						lpwp->x = 0;
						lpwp->y = 0;
						lpwp->cx = dxw.GetScreenWidth();
						lpwp->cy = dxw.GetScreenHeight();
					} else {
						dxw.UnmapWindow(&(lpwp->x), &(lpwp->y), &(lpwp->cx), &(lpwp->cy));
					}
				}
				OutTraceGDI("HookWndProc: remapped WM_WINDOWPOSCHANGx flags=%#x pos=(%d,%d) size=(%dx%d)\n", 
					lpwp->flags, lpwp->x, lpwp->y, lpwp->cx, lpwp->cy);
				break;
			case WM_CREATE:
			case WM_NCCREATE:
				lpcs = (LPCREATESTRUCTA)cwp->lParam;
				OutTraceGDI("HookWndProc: original pos=(%d,%d) size=(%dx%d)\n", 
					lpcs->x, lpcs->y, lpcs->cx, lpcs->cy);
				dxw.UnmapWindow(&(lpcs->x), &(lpcs->y), &(lpcs->cx), &(lpcs->cy));
				OutTraceGDI("HookWndProc: remapped pos=(%d,%d) size=(%dx%d)\n", 
					lpcs->x, lpcs->y, lpcs->cx, lpcs->cy);
			case WM_NCCALCSIZE:
				if(wParam){
				} else {
					LPRECT lprect = (LPRECT)cwp->lParam;
					OutTraceGDI("HookWndProc: original WM_NCCALCSIZE rect=(%d,%d)-(%d,%d)\n",
						lprect->left, lprect->top, lprect->right, lprect->bottom);
					dxw.UnmapWindow(lprect);
					OutTraceGDI("HookWndProc: remapped WM_NCCALCSIZE rect=(%d,%d)-(%d,%d)\n",
						lprect->left, lprect->top, lprect->right, lprect->bottom);
				}
			// tbd: useful or harmful? Should fix points only inside client area ??
			//case WM_NCHITTEST:
			//	width = LOWORD(cwp->lParam);
			//	height = HIWORD(cwp->lParam);
			//	OutTraceGDI("HookWndProc: original WM_NCHITTEST pos=(%d,%d)\n", width, height);
			//	dxw.UnmapClient(&width, &height);
			//	cwp->lParam = MAKELPARAM(width, height);
			//	OutTraceGDI("HookWndProc: remapped WM_NCHITTEST pos=(%d,%d)\n", width, height);
			//	break;
		}
	}
    return (*glpWindowsHookProcessFunction)(code, wParam, lParam);
}

LRESULT CALLBACK extWindowsHookProcRet(int code, WPARAM wParam, LPARAM lParam)
{
	OutTraceGDI("HookWndProcRet: code=%#x wParam=%#x lParam=%#x\n", code, wParam, lParam);
	if(code < 0) return CallNextHookEx(0, code, wParam, lParam);

	if(lParam){
		CWPRETSTRUCT *cwp;
		WINDOWPOS *lpwp;
		CREATESTRUCTA *lpcs;
		int width, height;
		cwp = (LPCWPRETSTRUCT)lParam;
		OutTraceGDI("HookWndProcRet: res=%#x msg=%#x(%s) wParam=%#x lParam=%#x hwnd=%#x\n", 
			cwp->lResult, cwp->message, ExplainWinMessage(cwp->message), cwp->wParam, cwp->lParam, cwp->hwnd);
		switch(cwp->message){
			case WM_SIZE:
				width = LOWORD(cwp->lParam);
				height = HIWORD(cwp->lParam);
				OutTraceGDI("HookWndProcRet: original WM_SIZE size=(%dx%d)\n", width, height);
				dxw.UnmapClient(&width, &height);
				cwp->lParam = MAKELPARAM(width, height);
				OutTraceGDI("HookWndProcRet: remapped WM_SIZE size=(%dx%d)\n", width, height);
				break;
			case WM_WINDOWPOSCHANGING:
			case WM_WINDOWPOSCHANGED:
				lpwp = (LPWINDOWPOS)cwp->lParam;
				OutTraceGDI("HookWndProcRet: original WM_WINDOWPOSCHANGx flags=%#x pos=(%d,%d) size=(%dx%d)\n", 
					lpwp->flags, lpwp->x, lpwp->y, lpwp->cx, lpwp->cy);
				if(!(lpwp->flags & (SWP_NOSIZE|SWP_NOMOVE))){
					if(dxw.IsDesktop(cwp->hwnd)){
					lpwp->x = 0;
					lpwp->y = 0;
					lpwp->cx = dxw.GetScreenWidth();
					lpwp->cy = dxw.GetScreenHeight();
					} else {
						dxw.UnmapWindow(&(lpwp->x), &(lpwp->y), &(lpwp->cx), &(lpwp->cy));
					}
				}
				OutTraceGDI("HookWndProcRet: remapped WM_WINDOWPOSCHANGx flags=%#x pos=(%d,%d) size=(%dx%d)\n", 
					lpwp->flags, lpwp->x, lpwp->y, lpwp->cx, lpwp->cy);
				break;
			case WM_CREATE:
			case WM_NCCREATE:
				lpcs = (LPCREATESTRUCTA)cwp->lParam;
				OutTraceGDI("HookWndProcRet: original pos=(%d,%d) size=(%dx%d)\n", 
					lpcs->x, lpcs->y, lpcs->cx, lpcs->cy);
				dxw.UnmapWindow(&(lpcs->x), &(lpcs->y), &(lpcs->cx), &(lpcs->cy));
				OutTraceGDI("HookWndProc: remapped pos=(%d,%d) size=(%dx%d)\n", 
					lpcs->x, lpcs->y, lpcs->cx, lpcs->cy);
				break;
			case WM_NCCALCSIZE:
				if(wParam){
				} else {
					LPRECT lprect = (LPRECT)cwp->lParam;
					OutTraceGDI("HookWndProcRet: original WM_NCCALCSIZE rect=(%d,%d)-(%d,%d)\n",
						lprect->left, lprect->top, lprect->right, lprect->bottom);
					dxw.UnmapWindow(lprect);
					OutTraceGDI("HookWndProcRet: remapped WM_NCCALCSIZE rect=(%d,%d)-(%d,%d)\n",
						lprect->left, lprect->top, lprect->right, lprect->bottom);
				}
				break;		
		}
	}
    return (*glpWindowsHookProcessRetFunction)(code, wParam, lParam);
}
#endif // HOOKWINDOWSHOOKPROCS

LRESULT CALLBACK DummyHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx((HHOOK)0, code, wParam, lParam);
}

static HHOOK WINAPI extSetWindowsHookEx(char *api, SetWindowsHookEx_Type pSetWindowsHookEx, 
										int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)
{
	HHOOK ret;

	OutTraceGDI("%s: id=%#x(%s) lpfn=%#x hmod=%#x threadid=%#x\n", api, idHook, sWinHookLabel(idHook), lpfn, hMod, dwThreadId);

	if(dxw.dwDFlags2 & DISABLEWINHOOKS){
		ret = (*pSetWindowsHookEx)(idHook, DummyHookProc, hMod, dwThreadId);
		OutTraceGDI("%s: DISABLEWINHOOKS hhook=%#x\n", ApiRef, ret);
		return ret;
	}

	if(dxw.dwFlags5 & EASPORTSHACK){
		OutTraceGDI("%s: EASPORTSHACK bypass active\n", api);
		if(idHook == WH_MOUSE) return NULL;
		if(idHook == WH_GETMESSAGE) {
			glpMessageHookProcessFunction = lpfn;
			lpfn=extEASportsMessageHookProc;
		}
	}

	if((dxw.dwFlags11 & FIXMESSAGEHOOK) && (idHook == WH_GETMESSAGE)) {
		OutTraceGDI("%s: MESSAGEHOOK filter active\n", api);
		glpMessageHookProcessFunction = lpfn;
		lpfn=extMessageHookProc;
	}

	if(idHook == WH_KEYBOARD) {
		// v2.03.39: "One Must Fall Battlegrounds" keyboard fix
		if(dwThreadId == NULL){
			dwThreadId = GetCurrentThreadId();
			OutTraceGDI("%s: fixing WH_KEYBOARD thread=0->%#x\n", api, dwThreadId);
		}
		OutTraceGDI("%s: WH_KEYBOARD bypass active\n", api);
		glpKeyboardHookProcessFunction = lpfn;
		lpfn = KeyboardHookProcessFunction;
	}

	if(idHook == WH_KEYBOARD_LL) {
		OutTraceGDI("%s: WH_KEYBOARD_LL bypass active\n", api);
		glpKeyboardHookProcessFunctionLL = lpfn;
		lpfn = KeyboardHookProcessFunctionLL;
	}

	// v2.04.13: "Starsiege" mouse control fix
	if((idHook == WH_CBT) && (dwThreadId == NULL)) {
		dwThreadId = GetCurrentThreadId();
		OutTraceGDI("%s: fixing WH_CBT thread=0->%#x\n", api, dwThreadId);
	}

	if((idHook == WH_CBT) && (dxw.dwFlags12 & SCALECBTHOOK)) {
		OutTraceGDI("%s: WH_CBT bypass active\n", api);
		glpCBTHookProcessFunction = lpfn;
		lpfn = extCBTHookProcessFunction;
	}

	if(dxw.dwFlags8 & FIXMOUSEHOOK){
		if(dwThreadId == 0) dwThreadId = GetCurrentThreadId(); // GameStation mouse input fix
		if(idHook == WH_MOUSE){
			OutTraceGDI("%s: FIXMOUSEHOOK filter active on WH_MOUSE\n", api);
			glpMouseHookProcessFunction = lpfn;
			lpfn=extMouseHookProc;
		}
		if (idHook == WH_MOUSE_LL){
			OutTraceGDI("%s: FIXMOUSEHOOK filter active on WH_MOUSE_LL\n", api);
			glpMouseHookProcessFunctionLL = lpfn;
			lpfn=extMouseHookProcLL;
		}
	}

#ifdef HOOKWINDOWSHOOKPROCS
		if(HOOKWINDOWSHOOKPROCS){
		if(idHook == WH_CALLWNDPROC){
			OutTraceGDI("%s: FIXWINDOWSHOOK filter active on WH_CALLWNDPROC\n", api);
			glpWindowsHookProcessFunction = lpfn;
			lpfn=extWindowsHookProc;
		}
		if(idHook == WH_CALLWNDPROCRET){
			OutTraceGDI("%s: FIXWINDOWSHOOK filter active on WH_CALLWNDPROCRET\n", api);
			glpWindowsHookProcessRetFunction = lpfn;
			lpfn=extWindowsHookProcRet;
		}
	}
#endif // HOOKWINDOWSHOOKPROCS

	ret=(*pSetWindowsHookEx)(idHook, lpfn, hMod, dwThreadId);

	OutTraceGDI("%s: hhk=%#x\n", api, ret);
	return ret;
}

HHOOK WINAPI extSetWindowsHookExA(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)
{ ApiName("SetWindowsHookExA"); return extSetWindowsHookEx(ApiRef, pSetWindowsHookExA, idHook, lpfn, hMod, dwThreadId); }
HHOOK WINAPI extSetWindowsHookExW(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)
{ ApiName("SetWindowsHookExW"); return extSetWindowsHookEx(ApiRef, pSetWindowsHookExW, idHook, lpfn, hMod, dwThreadId); }

BOOL WINAPI extUnhookWindowsHookEx(HHOOK hhk)
{
	BOOL ret;
	ApiName("UnhookWindowsHookEx");

	OutTraceGDI("%s: hhk=%#x\n", ApiRef, hhk);
	ret = (*pUnhookWindowsHookEx)(hhk);
	if(!ret){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

HRESULT WINAPI extMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	HRESULT res;
	ApiName("MessageBoxA");
	OutTraceGDI("%s: hwnd=%#x text=\"%s\" caption=\"%s\" type=%d\n", ApiRef, hWnd, lpText, lpCaption, uType);
	DWORD dwFlags1;
	if(dxw.dwFlags9 & NODIALOGS){
		switch (uType & 0x7){
			case MB_OK:					res = IDOK;			break;
			case MB_OKCANCEL:			res = IDCANCEL;		break;
			case MB_ABORTRETRYIGNORE:	res = IDIGNORE;		break;
			case MB_YESNOCANCEL:		res = IDCANCEL;		break;
			case MB_YESNO:				res = IDYES;		break;
			case MB_RETRYCANCEL:		res = IDCANCEL;		break;
			case MB_CANCELTRYCONTINUE:	res = IDCONTINUE;	break;
			default:					res = IDOK;			break;
		}
		return res;
	}
	if(dxw.dwFlags11 & CUSTOMLOCALE){
		LPWSTR wstr = NULL;
		LPWSTR wcaption = NULL;
		int n;
		if (lpText) {
			int size = lstrlenA(lpText);
			wstr = (LPWSTR)malloc((size+1)<<1);
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpText, size, wstr, size);
			wstr[n] = L'\0'; // make tail ! 
		}
		if (lpCaption) {
			int size = lstrlenA(lpCaption);
			wcaption = (LPWSTR)malloc((size+1)<<1);
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpCaption, size, wcaption, size);
			wcaption[n] = L'\0'; // make tail ! 
		}
		if(!pMessageBoxW){
			HMODULE hinst=(*pLoadLibraryA)("user32.dll");
			pMessageBoxW=(MessageBoxW_Type)(*pGetProcAddress)(hinst, "MessageBoxW");
			if(!pMessageBoxW) return FALSE;
		}
		res=(*pMessageBoxW)(hWnd, wstr, wcaption, uType);
		if (wcaption) free((LPVOID)wcaption);
		if (wstr) free((LPVOID)wstr);
		return res;
	}

	dwFlags1 = dxw.dwFlags1;
	dxw.dwFlags1 &= ~UNNOTIFY;
	res=(*pMessageBoxA)(hWnd, lpText, lpCaption, uType);
	dxw.dwFlags1 = dwFlags1;
	return res;
}

HRESULT WINAPI extMessageBoxTimeoutA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId, DWORD dwMilliseconds)
{
	HRESULT res;
	ApiName("MessageBoxTimeoutA");
	DWORD dwFlags1;
	OutTraceGDI("%s: hwnd=%#x text=\"%s\" caption=\"%s\" type=%d lang=%#x msec=%d\n", 
		ApiRef, hWnd, lpText, lpCaption, uType, wLanguageId, dwMilliseconds);
	if(dxw.dwFlags9 & NODIALOGS){
		switch (uType & 0x7){
			case MB_OK:					res = IDOK;			break;
			case MB_OKCANCEL:			res = IDCANCEL;		break;
			case MB_ABORTRETRYIGNORE:	res = IDIGNORE;		break;
			case MB_YESNOCANCEL:		res = IDCANCEL;		break;
			case MB_YESNO:				res = IDYES;		break;
			case MB_RETRYCANCEL:		res = IDCANCEL;		break;
			case MB_CANCELTRYCONTINUE:	res = IDCONTINUE;	break;
			default:					res = IDOK;			break;
		}
		return res;
	}
	if(dxw.dwFlags11 & CUSTOMLOCALE){
		LPWSTR wstr = NULL;
		LPWSTR wcaption = NULL;
		int n;
		if (lpText) {
			int size = lstrlenA(lpText);
			wstr = (LPWSTR)malloc((size+1)<<1);
			//n = MultiByteToWideChar(CP_ACP, 0, lpString, size, wstr, size);
			n = MultiByteToWideChar(dxw.CodePage, 0, lpText, size, wstr, size);
			wstr[n] = L'\0'; // make tail ! 
		}
		if (lpCaption) {
			int size = lstrlenA(lpCaption);
			wcaption = (LPWSTR)malloc((size+1)<<1);
			//n = MultiByteToWideChar(CP_ACP, 0, lpString, size, wstr, size);
			n = MultiByteToWideChar(dxw.CodePage, 0, lpCaption, size, wcaption, size);
			wcaption[n] = L'\0'; // make tail ! 
		}
		if(!pMessageBoxTimeoutW){
			HMODULE hinst=(*pLoadLibraryA)("user32.dll");
			pMessageBoxTimeoutW=(MessageBoxTimeoutW_Type)(*pGetProcAddress)(hinst, "MessageBoxTimeoutW");
			if(!pMessageBoxTimeoutW) return FALSE;
		}
		dwFlags1 = dxw.dwFlags1;
		dxw.dwFlags1 &= ~UNNOTIFY;
		res=(*pMessageBoxTimeoutW)(hWnd, wstr, wcaption, uType, dxw.Locale, dwMilliseconds);
		dxw.dwFlags1 = dwFlags1;
		if (wcaption) free((LPVOID)wcaption);
		if (wstr) free((LPVOID)wstr);
		return res;
	}

	dwFlags1 = dxw.dwFlags1;
	dxw.dwFlags1 &= ~UNNOTIFY;
	res=(*pMessageBoxTimeoutA)(hWnd, lpText, lpCaption, uType, wLanguageId, dwMilliseconds);
	dxw.dwFlags1 = dwFlags1;
	return res;
}

HRESULT WINAPI extMessageBoxTimeoutW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId, DWORD dwMilliseconds)
{
	HRESULT res;
	ApiName("MessageBoxTimeoutW");
	DWORD dwFlags1;
	OutTraceGDI("%s: hwnd=%#x text=\"%ls\" caption=\"%ls\" type=%d lang=%#x msec=%d\n", 
		ApiRef, hWnd, lpText, lpCaption, uType, wLanguageId, dwMilliseconds);
	if(dxw.dwFlags9 & NODIALOGS) return 1;
	dwFlags1 = dxw.dwFlags1;
	dxw.dwFlags1 &= ~UNNOTIFY;
	res=(*pMessageBoxTimeoutW)(hWnd, lpText, lpCaption, uType, wLanguageId, dwMilliseconds);
	dxw.dwFlags1 = dwFlags1;
	return res;
}

HDESK WINAPI extCreateDesktopA( LPCSTR lpszDesktop, LPCSTR lpszDevice, DEVMODE *pDevmode, DWORD dwFlags, ACCESS_MASK dwDesiredAccess, LPSECURITY_ATTRIBUTES lpsa)
{
	OutTraceGDI("CreateDesktop: SUPPRESS flags=%#x access=%#x\n", dwFlags, dwDesiredAccess);
	return (HDESK)0xDEADBEEF; // fake handle
}

BOOL WINAPI extSwitchDesktop(HDESK hDesktop)
{
	OutTraceGDI("SwitchDesktop: SUPPRESS hDesktop=%#x\n", hDesktop);
	return TRUE;
}

HDESK WINAPI extOpenDesktop(LPTSTR lpszDesktop, DWORD dwFlags, BOOL fInherit, ACCESS_MASK dwDesiredAccess)
{
	OutTraceGDI("OpenDesktop: SUPPRESS flags=%#x access=%#x\n", dwFlags, dwDesiredAccess);
	return (HDESK)0xDEADBEEF; // fake handle
	//return (HDESK)NULL; // fake handle
}

BOOL WINAPI extCloseDesktop(HDESK hDesktop)
{
	OutTraceGDI("CloseDesktop: SUPPRESS hDesktop=%#x\n", hDesktop);
	return TRUE;
}

INT_PTR WINAPI extDialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	BOOL ret;
	ApiName("DialogBoxParamW");
	DWORD dwFlags1;

	OutTraceGDI("%s: FullScreen=%#x TemplateName=\"%ls\" WndParent=%#x\n", 
		ApiRef, dxw.IsFullScreen(), sTemplateNameW(lpTemplateName), hWndParent);

	if(dxw.Windowize && (hWndParent == 0)){
		hWndParent = dxw.GethWnd();
		OutTraceGDI("%s: remap hWndParent 0->%#x\n", ApiRef, hWndParent);
	}
	BOOL WasWindowized = dxw.Windowize;
	if (WasWindowized && (dxw.dwFlags16 & NATIVEDIALOGS)){
		dxw.Windowize = FALSE;
		OutTraceGDI("%s: set no windowize\n", ApiRef);
	}

	InMainWinCreation++;
	dwFlags1 = dxw.dwFlags1;
	dxw.dwFlags1 &= ~UNNOTIFY;

	if(dxw.Windowize && (dxw.dwFlags10 & STRETCHDIALOGS)){
		// this used by "Aaron vs. Ruth"
		HRSRC hRes;
		HGLOBAL hgRes;
		LPVOID lpRes;
		LPVOID lpScaledRes;
		DWORD dwSize;
		hRes = FindResourceW(NULL, lpTemplateName, (LPCWSTR)RT_DIALOG);
		if(!hRes) {
			OutErrorGDI("%s: FindResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		hgRes = LoadResource(NULL, hRes);
		if(!hgRes) {
			OutErrorGDI("%s: LoadResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		lpRes = LockResource(hgRes);
		if(!lpRes) {
			OutErrorGDI("%s: LockResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		dwSize = SizeofResource(NULL, hRes);
		if(!dwSize) {
			OutErrorGDI("%s: SizeofResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		lpScaledRes = malloc(dwSize);
		memcpy(lpScaledRes, lpRes, dwSize);
		UnlockResource(lpRes);
#ifndef DXW_NOTRACES
		OutHexDW((LPBYTE)lpScaledRes, dwSize);
#endif //DXW_NOTRACES
		dxwStretchDialog(lpScaledRes, DXW_DIALOGFLAG_DUMP|DXW_DIALOGFLAG_STRETCH|(dxw.dwFlags1 & FIXTEXTOUT ? DXW_DIALOGFLAG_STRETCHFONT : 0));
		//ret = (*pDialogBoxIndirectParamW)(hInstance, (LPCDLGTEMPLATE)lpScaledRes, hWndParent, lpDialogFunc, dwInitParam);
		ret = DialogBoxIndirectParamW(hInstance, (LPCDLGTEMPLATE)lpScaledRes, hWndParent, lpDialogFunc, dwInitParam);
		free(lpScaledRes);
	}
	else {
		//ret = (*pDialogBoxParamW)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
		ret = DialogBoxParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	}

	InMainWinCreation--;
	dxw.dwFlags1 = dwFlags1;

	dxw.Windowize = WasWindowized;
	if((ret == (INT_PTR)0) || (ret == (INT_PTR)-1)){
		OutTraceGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	else {
		OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	}
	return ret;
}

typedef INT_PTR (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);

struct lParamHolder{
	LPARAM lparam;
	DLGPROC dlgProc;
	//DLGPROC dlgProc;
} gLParam;

DLGPROC gCallback;

INT_PTR CALLBACK DlgprocWrapper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ApiName("DlgprocWrapper");
	OutTraceGDI("%s: hwnd=%#x msg=%#x(%s) w/lParam=(%#x/%#x)\n", ApiRef, hwnd, msg, ExplainWinMessage(msg), wParam, lParam);
	RECT clientRect;
	INT_PTR ret;
	//WINDOWPOS WinPos;

	switch(msg){
		case WM_INITDIALOG:
			if(dxw.dwFlags19 & TRANSPARENTDIALOG){
				(*pSetWindowLong)(hwnd, GWL_EXSTYLE, (*pGetWindowLong)(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
				SetLayeredWindowAttributes(hwnd, RGB(255,0,255), 0, LWA_COLORKEY); // set magenta color key
			}
			break;
		case WM_ERASEBKGND:
			if(dxw.dwFlags19 & TRANSPARENTDIALOG){
				(*pGetClientRect)(hwnd, &clientRect) ;
				FillRect(GetDC(hwnd), &clientRect, CreateSolidBrush(RGB(255,0,255)));  // paint background in magenta
			return TRUE;
			}
			break;
		//case WM_SETFONT:
		//	if((dxw.dwFlags1 & FIXTEXTOUT) && ((dxw.GDIEmulationMode == GDIMODE_STRETCHED))) {
		//		wParam = (WPARAM)fontdb.GetScaledFont((HFONT)wParam);
		//		OutTraceGDI("%s: replaced scaled font hfnt=%#x\n", ApiRef, wParam);
		//	}
		//	break;
		//case WM_WINDOWPOSCHANGING:
		//case WM_WINDOWPOSCHANGED:
		//	{
		//		//WINDOWPOS *wp = (WINDOWPOS *)lParam;
		//		WINDOWPOS *wp = &WinPos;
		//		memcpy(wp, (WINDOWPOS *)lParam, sizeof(WINDOWPOS));
		//		RECT rect;
		//		rect.left = wp->x; rect.top = wp->y;
		//		rect.right = wp->x + wp->cx; rect.bottom = wp->y + wp->cy;
		//		rect = dxw.MapClientRect(&rect);
		//		wp->x = rect.left; wp->y = rect.top;
		//		wp->cx = rect.right - wp->x; wp->cy = rect.bottom - wp->y;
		//		lParam = (LPARAM)&WinPos;
		//	}
		//	break;
	}

	ret = (*gCallback)(hwnd, msg, wParam, lParam);
	return ret;
}

INT_PTR WINAPI extDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	BOOL ret;
	ApiName("DialogBoxParamA");
	DWORD dwFlags1;

	if(dxw.dwFlags19 & TRANSPARENTDIALOG){
		gCallback = lpDialogFunc;
		lpDialogFunc = DlgprocWrapper;
	}

	if(dxw.dwFlags11 & CUSTOMLOCALE){
		// v2.05.62: simplified NTLEA handling, see nthook.c CallWindowSendMessage
		LPCWSTR lpTemplateW = ((DWORD_PTR)lpTemplateName & WM_CLASSMASK ? MultiByteToWideCharInternal(lpTemplateName) : NULL);
		//INT_PTR ret = DialogBoxParamW(hInstance, (lpTemplateW ? lpTemplateW : (LPCWSTR)lpTemplateName), hWndParent, TopLevelDialogProc, lParamInit);
		INT_PTR ret = extDialogBoxParamW(hInstance, (lpTemplateW ? lpTemplateW : (LPCWSTR)lpTemplateName), hWndParent, lpDialogFunc, dwInitParam);
		if (lpTemplateW) FreeStringInternal((LPVOID)lpTemplateW);
		return ret;
	}

	OutTraceGDI("%s: FullScreen=%#x TemplateName=\"%s\" WndParent=%#x\n", 
		ApiRef, dxw.IsFullScreen(), sTemplateNameA(lpTemplateName), hWndParent);
	// v2.04.78: "Sentinel Returns" creates a full-sized top game window, initially invisible, and then a launcher dialog through DialogBoxParamA
	// appended to parent 0. In order to make everything work in window mode, two actions are necessary
	// 1) append the dialog to the virtual desktop window instead of 0 to avoid the launcer to be overlapped by the game window
	// 2) handle the dialog as if it were not in fullscreen mode
	if(dxw.Windowize && (hWndParent == 0)){
		hWndParent = dxw.GethWnd();
		OutTraceGDI("%s: remap hWndParent 0->%#x\n", ApiRef, hWndParent);
	}
	BOOL WasWindowized = dxw.Windowize;
	if (WasWindowized && (dxw.dwFlags16 & NATIVEDIALOGS)){
		dxw.Windowize = FALSE;
		OutTraceGDI("%s: set no windowize\n", ApiRef);
	}
	// attempt to fix "Colonial Project 2" dialog. Doesn't work, but it could be ok.....
	//if(FullScreen && dxw.IsRealDesktop(hWndParent)){
	//	OutTraceGDI("DialogBoxParamA: remap WndParent=%#x->%#x\n", hWndParent, dxw.GethWnd());
	//	hWndParent = dxw.GethWnd();
	//}
	InMainWinCreation++;
	dwFlags1 = dxw.dwFlags1;
	dxw.dwFlags1 &= ~UNNOTIFY;

	if(dxw.Windowize && (dxw.dwFlags10 & STRETCHDIALOGS)){
		// this used by "Aaron vs. Ruth"
		HRSRC hRes;
		HGLOBAL hgRes;
		LPVOID lpRes;
		LPVOID lpScaledRes;
		DWORD dwSize;
		hRes = FindResource(hInstance, lpTemplateName, RT_DIALOG);
		if(!hRes) {
			OutErrorGDI("%s: FindResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		hgRes = LoadResource(hInstance, hRes);
		if(!hgRes) {
			OutErrorGDI("%s: LoadResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		lpRes = LockResource(hgRes);
		if(!lpRes) {
			OutErrorGDI("%s: LockResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		dwSize = SizeofResource(hInstance, hRes);
		if(!dwSize) {
			OutErrorGDI("%s: SizeofResource ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
		lpScaledRes = malloc(dwSize);
		memcpy(lpScaledRes, lpRes, dwSize);
		UnlockResource(lpRes);
#ifndef DXW_NOTRACES
		OutHexDW((LPBYTE)lpScaledRes, dwSize);
#endif //DXW_NOTRACES
		dxwStretchDialog(lpScaledRes, DXW_DIALOGFLAG_DUMP|DXW_DIALOGFLAG_STRETCH|(dxw.dwFlags1 & FIXTEXTOUT ? DXW_DIALOGFLAG_STRETCHFONT : 0));
		ret = (*pDialogBoxIndirectParamA)(hInstance, (LPCDLGTEMPLATE)lpScaledRes, hWndParent, lpDialogFunc, dwInitParam);
		free(lpScaledRes);
	}
	else {
		ret = (*pDialogBoxParamA)(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	}

	InMainWinCreation--;
	dxw.dwFlags1 = dwFlags1;

	dxw.Windowize = WasWindowized;
	if((ret == (INT_PTR)0) || (ret == (INT_PTR)-1)){
		OutTraceGDI("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	else {
		OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	}
	return ret;
}

BOOL WINAPI extScrollWindow(HWND hWnd, int XAmount, int YAmount, const RECT *lpRect, const RECT *lpClipRect)
{
	RECT Rect, ClipRect;
	BOOL res;
	ApiName("ScrollWindow");

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char sRect[81];
		char sClipRect[81];
		if(lpRect) sprintf(sRect, "(%d,%d)-(%d,%d)", lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		else strcpy(sRect, "(NULL)");
		if(lpClipRect) sprintf(sClipRect, "(%d,%d)-(%d,%d)", lpClipRect->left, lpClipRect->top, lpClipRect->right, lpClipRect->bottom);
		else strcpy(sClipRect, "(NULL)");
		OutTrace("%s: hwnd=%#x amount=(%d,%d) rect=%s clip=%s\n", ApiRef, hWnd, XAmount, YAmount, sRect, sClipRect);
	}
#endif

	if(dxw.Windowize && dxw.IsFullScreen()){
		dxw.MapClient(&XAmount, &YAmount);
		if(lpRect){
			Rect = *lpRect;
			dxw.MapClient(&Rect);
			lpRect = &Rect;
		}
		if(lpClipRect){
			ClipRect = *lpClipRect;
			dxw.MapClient(&ClipRect);
			lpClipRect = &ClipRect;
		}
	}

	res=(*pScrollWindow)(hWnd, XAmount, YAmount, lpRect, lpClipRect);
	IfTraceError();
	return res;
}

// commented out, too dangerous. Known side effects:
// 1) Recursion on HOT PATCH mode (or forever loop?)
// 2) blanked dialog boxes in Galapagos
// In any case, if useful somehow, it should not be hooked on GDImode != NONE condition
// P.s.so far, GetParent wrapping is useless and is eliminated.

#if 0
HWND WINAPI extGetParent(HWND hWnd)
{
	// Beware: can cause recursion on HOT PATCH mode
	HWND ret;

	ret = (*pGetParent)(hWnd);
	OutDebugDW("GetParent: hwnd=%#x ret=%#x\n", hWnd, ret);

	if(dxw.IsFullScreen()){
		if(ret == dxw.GethWnd()) {
			OutDebugDW("GetParent: setting desktop reached\n");
			ret = 0; // simulate reaching the desktop
		}
	}

	return ret;
}
#endif

BOOL WINAPI extInvalidateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
	// v2.05.92 fix: added proper scaling of invalidated rect region
	// fixes "Amy's Fantasies" scaled text progressive display
	ApiName("InvalidateRgn");

	OutTraceGDI("%s: hwnd=%#x hrgn=%#x erase=%#x\n", ApiRef, hWnd, hRgn, bErase);

	if(dxw.IsFullScreen()){
		if (dxw.IsRealDesktop(hWnd) && bErase) return true;
	}

	if(dxw.dwFlags11 & INVALIDATEFULLRECT) {
		hRgn = NULL;
		OutTraceGDI("%s: INVALIDATEFULLRECT fixed hrgn=NULL\n", ApiRef);
	}
	else
	if(dxw.IsFullScreen() && (dxw.GDIEmulationMode != GDIMODE_NONE)){
		HRGN hrgnScaled;
		hrgnScaled = dxw.MapRegion(ApiRef, hRgn);
		CombineRgn(hRgn, hrgnScaled, NULL, RGN_COPY);
		(*pDeleteObject)(hrgnScaled);
	} 

	return (*pInvalidateRgn)(hWnd, hRgn, bErase);
}

struct MYICON_INFO
{
    int     nWidth;
    int     nHeight;
    int     nBitsPerPixel;
};

static MYICON_INFO MyGetIconInfo(HICON hIcon)
{
    MYICON_INFO myinfo;
    ZeroMemory(&myinfo, sizeof(myinfo));

    ICONINFO info;
    ZeroMemory(&info, sizeof(info));

    BOOL bRes = FALSE;

    bRes = GetIconInfo(hIcon, &info);
    if(!bRes)
        return myinfo;

    BITMAP bmp;
    ZeroMemory(&bmp, sizeof(bmp));

    if(info.hbmColor){
        const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
        if(nWrittenBytes > 0){
            myinfo.nWidth = bmp.bmWidth;
            myinfo.nHeight = bmp.bmHeight;
            myinfo.nBitsPerPixel = bmp.bmBitsPixel;
        }
    }
    else if(info.hbmMask){
        // Icon has no color plane, image data stored in mask
        const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
        if(nWrittenBytes > 0){
            myinfo.nWidth = bmp.bmWidth;
            myinfo.nHeight = bmp.bmHeight / 2;
            myinfo.nBitsPerPixel = 1;
        }
    }

    if(info.hbmColor)
        DeleteObject(info.hbmColor);
    if(info.hbmMask)
        DeleteObject(info.hbmMask);

    return myinfo;
}

static BOOL StretchIcon(ApiArg, HDC hdc, int cx, int cy, int X, int Y, HICON hIcon) 
{
	BOOL ret;
	HRESULT res;
	dxw.MapClient(&X, &Y);
	// build a 1:1 DC to draw the icon
	HDC unscaledDC = CreateCompatibleDC(hdc);
	HBITMAP hbmp = (*pCreateCompatibleBitmap)(hdc, cx, cy);
	if(!hbmp){
		OutErrorGDI("%s: CreateCompatibleBitmap ERROR err=%d\n", ApiRef, GetLastError()); 
		return FALSE;
	}
	SelectObject(unscaledDC, hbmp);
	// copy the scaled background on this DC
	int sx = cx;
	int sy = cy;
	dxw.MapClient(&sx, &sy);
	OutTraceGDI("%s: fixed STRETCHED pos=(%d,%d) size=(%d,%d)\n", ApiRef, X, Y, sx, sy);
	(*pGDIStretchBlt)(unscaledDC, 0, 0, cx, cy, hdc, X, Y, sx, sy, SRCCOPY);
	// draw the icon on top of the DC
	ret = (*pDrawIcon)(unscaledDC, 0, 0, hIcon);
	if(!ret){
		OutErrorGDI("%s: DrawIcon ERROR err=%d\n", ApiRef, GetLastError()); 
		return FALSE;
	}
	// stretch from the 1:1 DC to the screen DC
	if(dxw.dwFlags15 & FORCEHALFTONE) {
		res=SetStretchBltMode(unscaledDC, HALFTONE);
		_if((!res) || (res==ERROR_INVALID_PARAMETER)) 
			OutErrorGDI("%s: SetStretchBltMode ERROR err=%d\n", ApiRef, GetLastError());
		// ignore the error
	}
	ret = (*pGDIStretchBlt)(hdc, X, Y, sx, sy, unscaledDC, 0, 0, cx, cy, SRCCOPY);
	if(!ret) OutErrorGDI("%s: StretchBlt ERROR err=%d\n", ApiRef, GetLastError()); 
	DeleteObject(hbmp);
	DeleteObject(unscaledDC);
	if(dxw.dwDFlags & MARKGDI32) {
		dxw.Mark(hdc, FALSE, RGB(255, 0, 0), X, Y, sx, sy);
	}
	return ret;
}

BOOL WINAPI extDrawIcon(HDC hdc, int X, int Y, HICON hIcon)
{
	BOOL res;
	ApiName("DrawIcon");

	OutTraceGDI("%s: hdcdest=%#x pos=(%d,%d) hicon=%#x\n", ApiRef, hdc, X, Y, hIcon);

	if(dxw.dwDFlags & NOICONS) {
		OutTraceGDI("%s: NOICONS\n", ApiRef);
		return TRUE;
	}

	MYICON_INFO ii = MyGetIconInfo(hIcon);
	int cx = ii.nWidth;
	int cy = ii.nHeight;

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC){
		//lpADC->AcquireDC();
		res=(*pDrawIcon)(lpADC->GetDC(hdc),  X, Y, hIcon);
		lpADC->ReleaseDC();
		return TRUE;
	}

	if(dxw.IsToRemap(hdc)){
		OutTraceGDI("%s: icon size=(%dx%d)\n", ApiRef, cx, cy);
		switch(dxw.GDIEmulationMode){
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&X, &Y);
				dxw.MapClient(&cx, &cy);
#if 1
				if(!StretchIcon(ApiRef, hdc, cx, cy, X, Y, hIcon)) return FALSE;
				OutTraceGDI("%s: fixed STRETCHED pos=(%d,%d) size=(%dx%d)\n", ApiRef, X, Y, cx, cy);
				return true;
#else
				{
				//HDC topdc = (*pGDIGetDC)(dxw.GethWnd());
				//hdc = topdc;
				// ensure it's a square icon
				cx = (cx > cy) ? cy : cx; cy = cx;
				OutTraceGDI("%s: fixed STRETCHED size=(%dx%d) pos=(%d,%d)\n", 
					ApiRef, cx, cy, X, Y);
				if(!pDrawIconEx) pDrawIconEx = DrawIconEx;
				//res=(*pDrawIconEx)(hdc, X, Y, hIcon, cx, cy, 0, NULL, 0);
				res=(*pDrawIcon)(hdc,  X, Y, hIcon);
				//res=(*pDrawIconEx)(hdc, X, Y, hIcon, cx, cy, 0, NULL, DI_NORMAL | DI_DEFAULTSIZE);
				//res=(*pDrawIconEx)(hdc, X, Y, hIcon, cx, cy, 0, NULL, DI_NORMAL | DI_COMPAT); 
				OutTraceGDI("%s: res=%#x err=%d\n", ApiRef, res, GetLastError()); 
				if(dxw.dwDFlags & MARKWING32) {
					dxw.Mark(hdc, FALSE, RGB(255, 0, 0), X, Y, cx, cy);
				}
				//(*pGDIReleaseDC)(dxw.GethWnd(), topdc);
				return res;
				}
#endif
				break;
			case GDIMODE_SHAREDDC: 
				{
				sdc.GetPrimaryDC(hdc);
				res=(*pDrawIcon)(sdc.GetHdc(),  X, Y, hIcon);
				sdc.PutPrimaryDC(hdc, TRUE, X, Y, cx, cy); 
				return res;
				}
				break;
		default:
			break;
		}
	}

	res = (*pDrawIcon)(hdc, X, Y, hIcon);
	IfTraceError();
	if(dxw.dwDFlags & MARKWING32) {
		//int cx = (*pGetSystemMetrics)(SM_CXICON);
		//int cy = (*pGetSystemMetrics)(SM_CYICON);
		dxw.Mark(hdc, FALSE, RGB(255, 0, 0), X, Y, cx, cy);
	}
	return res;
}

// not working in HOT PATCH mode
BOOL WINAPI extDrawIconEx( HDC hdc, int xLeft, int yTop, HICON hIcon, int cxWidth, int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
	BOOL res;
	ApiName("DrawIconEx");

	OutTraceGDI("%s: hdc=%#x pos=(%d,%d) hicon=%#x size=(%d,%d) istep=%#x flags=%#x\n",
		ApiRef, hdc, xLeft, yTop, hIcon, cxWidth, cyWidth, istepIfAniCur, diFlags);

	if(dxw.dwDFlags & NOICONS) {
		OutTraceGDI("%s: NOICONS\n", ApiRef);
		return TRUE;
	}

	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_STRETCHED: 
				dxw.MapClient(&xLeft, &yTop, &cxWidth, &cyWidth);
				OutTraceGDI("%s: fixed STRETCHED pos=(%d,%d) size=(%d,%d)\n", ApiRef, xLeft, yTop, cxWidth, cyWidth);
				break;
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				res=(*pDrawIconEx)(sdc.GetHdc(), xLeft, yTop, hIcon, cxWidth, cyWidth, istepIfAniCur, hbrFlickerFreeDraw, diFlags);
				sdc.PutPrimaryDC(hdc, TRUE, xLeft, yTop, cxWidth, cyWidth);
				return res;
				break;
		default:
			break;
		}
	}
	res = (*pDrawIconEx)(hdc, xLeft, yTop, hIcon, cxWidth, cyWidth, istepIfAniCur, hbrFlickerFreeDraw, diFlags);
	IfTraceError();
	return res;
}

BOOL WINAPI extDrawCaption(HWND hwnd, HDC hdc, LPCRECT lprc, UINT uFlags)
{
	BOOL res;
	ApiName("DrawCaption");

	OutTraceGDI("%s: hwnd=%#x hdc=%#x rect=(%d,%d)-(%d,%d) flags=%#x\n", 
		ApiRef, hwnd, hdc, lprc->left, lprc->top, lprc->right, lprc->bottom, uFlags);
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_STRETCHED: 
				dxw.MapClient((LPRECT)lprc);
				OutTraceGDI("%s: fixed STRETCHED rect=(%d,%d)-(%d,%d)\n", 
					ApiRef, lprc->left, lprc->top, lprc->right, lprc->bottom);
				break;
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				res=(*pDrawCaption)(hwnd, sdc.GetHdc(), lprc, uFlags);
				sdc.PutPrimaryDC(hdc, TRUE, lprc->left, lprc->top, lprc->right, lprc->bottom);
				return res;
				break;
		default:
			break;
		}
	}
	res = (*pDrawCaption)(hwnd, hdc, lprc, uFlags);
	IfTraceError();
	return res;
}

BOOL WINAPI extPaintDesktop(HDC hdc)
{
	BOOL res;
	ApiName("PaintDesktop");

	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);
	if(dxw.IsToRemap(hdc)){
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				res=(*pPaintDesktop)(sdc.GetHdc());
				sdc.PutPrimaryDC(hdc, TRUE);
				return res;
				break;
		default:
			break;
		}
	}
	res = (*pPaintDesktop)(hdc);
	IfTraceError();
	return res;
}

#ifndef DXW_NOTRACES
char *ExplainMouseMoveFlags(DWORD c)
{
	static char eb[256];
	unsigned int l;
	strcpy(eb,"MOUSEEVENTF_");
	if (c & MOUSEEVENTF_MOVE) strcat(eb, "MOVE+");
	if (c & MOUSEEVENTF_LEFTDOWN) strcat(eb, "LEFTDOWN+");
	if (c & MOUSEEVENTF_LEFTUP) strcat(eb, "LEFTUP+");
	if (c & MOUSEEVENTF_RIGHTDOWN) strcat(eb, "RIGHTDOWN+");
	if (c & MOUSEEVENTF_RIGHTUP) strcat(eb, "RIGHTUP+");
	if (c & MOUSEEVENTF_MIDDLEDOWN) strcat(eb, "MIDDLEDOWN+");
	if (c & MOUSEEVENTF_MIDDLEUP) strcat(eb, "MIDDLEUP+");
	if (c & MOUSEEVENTF_XDOWN) strcat(eb, "XDOWN+");
	if (c & MOUSEEVENTF_XUP) strcat(eb, "XUP+");
	if (c & MOUSEEVENTF_WHEEL) strcat(eb, "WHEEL+");
	if (c & MOUSEEVENTF_HWHEEL) strcat(eb, "HWHEEL+");
	if (c & MOUSEEVENTF_ABSOLUTE) strcat(eb, "ABSOLUTE+");
	l=strlen(eb);
	if (l>strlen("MOUSEEVENTF_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}
#endif

VOID WINAPI extmouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo)
{
	ApiName("mouse_event");

	OutTraceC("%s: flags=%#x(%s) xy=(%d,%d) data=%#x, extrainfo=%lx\n", 
		ApiRef, dwFlags, ExplainMouseMoveFlags(dwFlags), dx, dy, dwData, dwExtraInfo);
	
	if(dxw.dwFlags9 & NOMOUSEEVENTS) {
		OutTraceGDI("%s: SUPPRESS mouse event\n", ApiRef);
		return;
	}

	if((dwFlags & MOUSEEVENTF_MOVE) && (dxw.dwFlags2 & KEEPCURSORFIXED)) {
		OutTraceGDI("%s: SUPPRESS mouse move\n", ApiRef);
		return;
	}

	if(dxw.MustFixMouse){
		POINT cursor;
		cursor.x = dx;
		cursor.y = dy;
		if(dwFlags & MOUSEEVENTF_ABSOLUTE){
			// ???? untested ......
			//dxw.MapClient((int *)&dx, (int *)&dy);
			cursor = dxw.FixCursorPos(cursor);
		}
		else{
			dxw.MapClient(&cursor);
		}
		OutTraceGDI("%s: FIX MOUSEEVENTF_ABSOLUTE (%d,%d) -> (%d,%d)\n", ApiRef, dx, dy, cursor.x, cursor.y);
		dx = cursor.x;
		dy = cursor.y;
	}

	return (*pmouse_event)(dwFlags, dx, dy, dwData, dwExtraInfo);
}

BOOL WINAPI extShowScrollBar(HWND hWnd, int wBar, BOOL bShow)
{
	BOOL res;
	ApiName("ShowScrollBar");

	OutTraceGDI("%s: hwnd=%#x wBar=%#x show=%#x\n", ApiRef, hWnd, wBar, bShow);
	if(dxw.Windowize && dxw.IsRealDesktop(hWnd)) hWnd=dxw.GethWnd();
	res=(*pShowScrollBar)(hWnd, wBar, bShow);
	IfTraceError();
	return res;
}

BOOL WINAPI extDrawMenuBar(HWND hWnd)
{
	BOOL res;
	ApiName("DrawMenuBar");

	OutTraceGDI("%s: hwnd=%#x\n", ApiRef, hWnd);
	if(dxw.Windowize && dxw.IsRealDesktop(hWnd)) {
		OutTraceGDI("%s: moving hwnd %#x->%#x\n", ApiRef, hWnd, dxw.GethWnd());
		hWnd=dxw.GethWnd();
	}
	res=(*pDrawMenuBar)(hWnd);
	IfTraceError();
	return res;
}

#ifdef TRACESYSCALLS
BOOL WINAPI extTranslateMessage(MSG *pMsg)
{
	BOOL ret;
	ApiName("TranslateMessage");

	OutTraceGDI("%s: type=%#x pos=(%d,%d)\n", ApiRef, pMsg->message, pMsg->pt.x, pMsg->pt.y);
	ret=(*pTranslateMessage)(pMsg);
	OutTraceGDI("%s: ret=%#x(%s)\n", ApiRef, ret, ret ? "translated" : "not translated");
	return ret;
}
#endif

BOOL WINAPI extEnumDisplayDevicesA(LPCSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICE lpDisplayDevice, DWORD dwFlags)
{
	BOOL ret;
	ApiName("EnumDisplayDevicesA");

	OutTraceGDI("%s: device=%s devnum=%i flags=%#x\n", ApiRef, lpDevice, iDevNum, dwFlags);

	if((dxw.dwFlags2 & HIDEMULTIMONITOR) && (iDevNum > 0)) {
		OutTraceGDI("%s: HIDEMULTIMONITOR devnum=%i\n", ApiRef, iDevNum);
		return FALSE;
	}

	ret = (*pEnumDisplayDevicesA)(lpDevice, iDevNum, lpDisplayDevice, dwFlags);

#ifndef DXW_NOTRACES
	if(ret){
		OutTraceGDI("%s: cb=%#x devname=%s devstring=%s stateflags=%#x\n", 
			ApiRef, lpDisplayDevice->cb, lpDisplayDevice->DeviceName, lpDisplayDevice->DeviceString, lpDisplayDevice->StateFlags);
	}
	else{
		DWORD err = GetLastError();
		if(err == ERROR_INVALID_PARAMETER){
			OutTraceGDI("%s: INVALID_PARAMETER (no such device) devnum=%d\n", ApiRef, iDevNum);
		}
		else {
			OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
		}
	}
#endif // DXW_NOTRACES
	return ret;
}

BOOL WINAPI extEnumDisplayDevicesW(LPCWSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICEW lpDisplayDevice, DWORD dwFlags)
{
	BOOL ret;
	ApiName("EnumDisplayDevicesW");

	OutTraceGDI("%s: device=%ls devnum=%i flags=%#x\n", ApiRef, lpDevice, iDevNum, dwFlags);

	if((dxw.dwFlags2 & HIDEMULTIMONITOR) && (iDevNum > 0)) {
		OutTraceGDI("%s: HIDEMULTIMONITOR devnum=%i\n", ApiRef, iDevNum);
		return FALSE;
	}

	ret = (*pEnumDisplayDevicesW)(lpDevice, iDevNum, lpDisplayDevice, dwFlags);

#ifndef DXW_NOTRACES
	if(ret){
		OutTraceGDI("%s: cb=%#x devname=%ls devstring=%ls stateflags=%#x\n", 
			ApiRef, lpDisplayDevice->cb, lpDisplayDevice->DeviceName, lpDisplayDevice->DeviceString, lpDisplayDevice->StateFlags);
	}
	else{
		DWORD err = GetLastError();
		if(err == ERROR_INVALID_PARAMETER){
			OutTraceGDI("%s: INVALID_PARAMETER (no such device) devnum=%d\n", ApiRef, iDevNum);
		}
		else {
			OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
		}
	}
#endif // DXW_NOTRACES
	return ret;
}

INT_PTR WINAPI extDialogBoxIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	ApiName("DialogBoxIndirectParamA");

	OutTraceGDI("%s: hInstance=%#x pos=(%d,%d) size=(%dx%d) hWndParent=%#x, lpDialogFunc=%#x dwInitParam=%#x\n",
		ApiRef, hInstance, 
		hDialogTemplate->x, hDialogTemplate->y, hDialogTemplate->cx, hDialogTemplate->cy, 
		hWndParent, lpDialogFunc, dwInitParam);

#if 0
	if(dxw.dwFlags11 & CUSTOMLOCALE){
		// v2.05.62: necessary???
		INT_PTR ret = DialogBoxIndirectParamW(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
		return ret;
	}
#endif 

	return (*pDialogBoxIndirectParamA)(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR WINAPI extDialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	ApiName("DialogBoxIndirectParamW");

	OutTraceGDI("%s: hInstance=%#x pos=(%d,%d) size=(%dx%d) hWndParent=%#x, lpDialogFunc=%#x dwInitParam=%#x\n",
		ApiRef, hInstance, 
		hDialogTemplate->x, hDialogTemplate->y, hDialogTemplate->cx, hDialogTemplate->cy, 
		hWndParent, lpDialogFunc, dwInitParam);

	return (*pDialogBoxIndirectParamW)(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

BOOL WINAPI extEnumWindows(WNDENUMPROC lpEnumFunc, LPARAM lParam)
{
	ApiName("EnumWindows");

	OutTraceGDI("%s\n", ApiRef);
	if(dxw.dwFlags8 & WININSULATION){
		OutTraceGDI("%s: BYPASS\n", ApiRef);
		if(dxw.GethWnd()){
			if(!pEnumChildWindows) pEnumChildWindows = EnumChildWindows;
			lpEnumFunc(dxw.GethWnd(), lParam);
			(*pEnumChildWindows)(dxw.GethWnd(), lpEnumFunc, lParam); // to test .... 
		}
		return TRUE;
	}
	return (*pEnumWindows)(lpEnumFunc, lParam);
}

static void RedirectCoordinates(char *api, LPRECT lpRect)
{
	WINDOWPOS wp;
	dxw.CalculateWindowPos(NULL, dxw.GetScreenWidth(), dxw.GetScreenHeight(), &wp);
	lpRect->left = wp.x;
	lpRect->right = wp.x + wp.cx;
	lpRect->top = wp.y;
	lpRect->bottom = wp.y + wp.cy;
	OutTraceGDI("%s: FIX rect=(%d,%d)-(%d,%d)\n",
		api, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
}

BOOL WINAPI extAdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
	BOOL ret;
	ApiName("AdjustWindowRect");

#ifndef DXW_NOTRACES
	char sStyle[256];
	OutTraceGDI("%s: IN rect=(%d,%d)-(%d,%d) style=%#x(%s) menu=%#x\n",
		ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom,
		dwStyle, ExplainStyle(dwStyle, sStyle, 256), bMenu);
#endif // DXW:NOTRACES

	if(dxw.Windowize && (dxw.dwFlags8 & FIXADJUSTWINRECT)) RedirectCoordinates(ApiRef, lpRect);

	ret = pAdjustWindowRect(lpRect, dwStyle, bMenu);

	if(ret){
		OutTraceGDI("%s: OUT rect=(%d,%d)-(%d,%d)\n",
			ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	else{
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extAdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
	BOOL ret;
	ApiName("AdjustWindowRectEx");

#ifndef DXW_NOTRACES
	char sStyle[256];
	char sExStyle[256];
	OutTraceGDI("%s: IN rect=(%d,%d)-(%d,%d) style=%#x(%s) menu=%#x exstyle=%#x(%s)\n",
		ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom,
		dwStyle, ExplainStyle(dwStyle, sStyle, 256), bMenu, dwExStyle, ExplainExStyle(dwExStyle, sExStyle, 256));
#endif // DXW:NOTRACES

	if(dxw.Windowize && (dxw.dwFlags8 & FIXADJUSTWINRECT)) RedirectCoordinates(ApiRef, lpRect);

	ret = pAdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);

	if(ret){
		OutTraceGDI("%s: OUT rect=(%d,%d)-(%d,%d)\n",
			ApiRef, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	else{
		OutErrorGDI("%s ERROR: err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extValidateRgn(HWND hwnd, HRGN hrgn)
{
	BOOL ret;
	ApiName("ValidateRgn");

	OutTraceGDI("%s: hwnd=%#x hrgn=%#x\n", ApiRef, hwnd, hrgn);

	ret = (*pValidateRgn)(hwnd, hrgn);
	return ret;
}

UINT WINAPI extSendInput(UINT nInputs, LPINPUT pInputs, int cbSize)
{
	UINT ret;
	ApiName("SendInput");

	OutTraceGDI("%s: nInputs=%d cbSize=%d\n", ApiRef, nInputs, cbSize);
#ifndef DXW_NOTRACES
	if(IsDebugIN || IsDebugGDI){
		for(UINT i=0; i<nInputs; i++){
			PINPUT pi=&pInputs[i];
			switch(pi->type){
				case INPUT_MOUSE:
					OutTrace("%s: input=%d [Mouse:(%d,%d) data=%#x flags=%#x]\n", ApiRef, i, pi->mi.dx, pi->mi.dy, pi->mi.mouseData, pi->mi.dwFlags); break;
				case INPUT_KEYBOARD:
					OutTrace("%s: input=%d [Keybd:scan=%#x vk=%#x flags=%#x]\n", ApiRef, pi->ki.wScan, pi->ki.wVk, pi->ki.dwFlags); break;
				case INPUT_HARDWARE:
					OutTrace("%s: input=%d [Hardw:param h=%#x l=%#x]\n", ApiRef, pi->hi.wParamH, pi->hi.wParamL); break;
				default:
					OutTrace("%s: input=%d [Ubknown type=%#x]\n", ApiRef, pi->type); break;
			}
		}
	}
#endif
	if (dxw.dwFlags2 & KEEPCURSORFIXED){
		UINT nInputsFixed = 0;
		for(UINT i=0; i<nInputs; i++){
			PINPUT pi=&pInputs[i];
			PINPUT pi2=&pInputs[nInputsFixed];
			if(pi->type != INPUT_MOUSE){
				memcpy(pi2, pi, cbSize);
				nInputsFixed++;
			}
		}
		nInputs = nInputsFixed;
		OutTraceGDI("%s: KEEPCURSORFIXED updated nInputs=%d\n", ApiRef, nInputs);
		if(nInputs == 0) return 0;
	}

	if(dxw.IsFullScreen() & dxw.Windowize){
		for(UINT i=0; i<nInputs; i++){
			PINPUT pi=&pInputs[i];
			if(pi->type == INPUT_MOUSE){
				POINT pt;
				pt.x = pi->mi.dx; pt.y = pi->mi.dy;
				if(pi->mi.dwFlags & MOUSEEVENTF_ABSOLUTE)
					dxw.MapWindow(&pt);
				else {
					dxw.MapClient(&pt);
					// don't scale 1 or -1 relative movements
					if((pi->mi.dx == 1) || (pi->mi.dx == -1)) pt.x = pi->mi.dx;
					if((pi->mi.dy == 1) || (pi->mi.dy == -1)) pt.y = pi->mi.dy;
				}
				OutTraceGDI("%s: fixed mouse pt=(%d,%d) -> (%d,%d)\n", ApiRef, pi->mi.dx, pi->mi.dy, pt.x, pt.y);
				pi->mi.dx = pt.x; pi->mi.dy = pt.y;
			}
		}
	}
	ret = (*pSendInput)(nInputs, pInputs, cbSize);
	if(!ret){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

// To do:
// GrayStringA
// GrayStringW

#ifndef DXW_NOTRACES
static char *sRawDevFlags(DWORD c)
{
	static char eb[257];
	unsigned int l;
	strcpy(eb,"RIDEV_");
	if (c & RIDEV_REMOVE) strcat(eb, "REMOVE+"); 
	if (c & RIDEV_EXCLUDE) strcat(eb, "EXCLUDE+"); 
	if (c & RIDEV_PAGEONLY) strcat(eb, "PAGEONLY+"); 
	if (c & RIDEV_NOLEGACY) strcat(eb, "NOLEGACY+"); 
	if (c & RIDEV_INPUTSINK) strcat(eb, "INPUTSINK+"); 
	if (c & RIDEV_CAPTUREMOUSE) strcat(eb, "CAPTUREMOUSE+"); 
	if (c & RIDEV_NOHOTKEYS) strcat(eb, "NOHOTKEYS+"); 
	if (c & RIDEV_APPKEYS) strcat(eb, "APPKEYS+"); 
	if (c & RIDEV_EXINPUTSINK) strcat(eb, "EXINPUTSINK+"); 
	if (c & RIDEV_DEVNOTIFY) strcat(eb, "DEVNOTIFY+"); 
	l=strlen(eb);
	if (l>strlen("RIDEV_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}

static char *sRIMType(DWORD type)
{
#define RIM_TYPEBAD (RIM_TYPEHID+1)
	if(type > RIM_TYPEBAD) type = RIM_TYPEBAD;
	static char *sRIMTypes[] = {"MOUSE", "KEYBOARD", "HID", "invalid"};
	return sRIMTypes[type];
}

static char *sRawMouseFlags(USHORT c)
{
	static char eb[81];
	unsigned int l;
	strcpy(eb,"MOUSE_");
	if (c & MOUSE_MOVE_ABSOLUTE) strcat(eb, "MOVE_ABSOLUTE+"); 
	else strcat(eb, "MOVE_RELATIVE+"); 
	if (c & MOUSE_VIRTUAL_DESKTOP) strcat(eb, "VIRTUAL_DESKTOP+");
	if (c & MOUSE_ATTRIBUTES_CHANGED) strcat(eb, "ATTRIBUTES_CHANGED+");
	if (c & MOUSE_MOVE_NOCOALESCE) strcat(eb, "MOVE_NOCOALESCE+");
	l=strlen(eb);
	if (l>strlen("MOUSE_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}
#endif // DXW_NOTRACES

BOOL WINAPI extRegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize)
{
	BOOL res;
	ApiName("RegisterRawInputDevices");

#ifndef DXW_NOTRACES
	OutTraceC("%s: numdevs=%d size=%d\n", ApiRef, uiNumDevices, cbSize);
	for(UINT i=0; i<uiNumDevices; i++){
		OutTraceC("> dev[%d]: upage=%#x usage=%#x flags=%#x(%s) hwnd=%#x\n", i,
			pRawInputDevices[i].usUsagePage,
			pRawInputDevices[i].usUsage,
			pRawInputDevices[i].dwFlags, sRawDevFlags(pRawInputDevices[i].dwFlags),
			pRawInputDevices[i].hwndTarget
			);
	}
#endif // DXW_NOTRACES

	if(dxw.Windowize){
		for(UINT i=0; i<uiNumDevices; i++) (DWORD)(pRawInputDevices[i].dwFlags) &= ~RIDEV_CAPTUREMOUSE;
	}

	res=(*pRegisterRawInputDevices)(pRawInputDevices, uiNumDevices, cbSize);
	IfTraceError();
	return res;
}

UINT WINAPI extGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
	UINT ret;
	ApiName("GetRawInputData");

	OutTraceC("%s: hri=%#x cmd=%#x(%s) data=%#x sizehdr=%#x\n", 
		ApiRef, hRawInput, uiCommand, 
		(uiCommand==RID_INPUT) ? "INPUT" : (uiCommand==RID_HEADER) ? "HEADER" : "unknown",
		pData, cbSizeHeader);

	ret = (*pGetRawInputData)(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	if(ret == -1) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
		return ret;
	}

#ifndef DXW_NOTRACES
	if(IsTraceC){
		OutTrace("%s: size=%d\n", ApiRef, *pcbSize);
		if(pData) {
			RAWINPUTHEADER *phdr;
			RAWMOUSE *pmouse;
			phdr = (RAWINPUTHEADER *)pData;
			// this info retrieved in any case
			OutTrace("%s: size=%d type=%#x(%s) hdev=%#x wparam=%#x\n", 
				ApiRef,
				phdr->dwSize,
				phdr->dwType, sRIMType(phdr->dwType),
				phdr->hDevice,
				phdr->wParam);
			if(uiCommand == RID_INPUT){
				// this info retrieved only with RID_INPUT command
				phdr = (RAWINPUTHEADER *)pData;
				OutTrace("%s: size=%d type=%#x(%s) hdev=%#x wparam=%#x\n",
					ApiRef,
					phdr->dwSize,
					phdr->dwType, sRIMType(phdr->dwType),
					phdr->hDevice,
					phdr->wParam);
				switch (phdr->dwType) {
					case RIM_TYPEMOUSE:
						pmouse = &((LPRAWINPUT)pData)->data.mouse;
						OutTrace("%s: MOUSE flags=%#x(%s) xy=(%d,%d) bflags=%#x bdata=%#x extra=%#x\n", 
							ApiRef,
							pmouse->usFlags, sRawMouseFlags(pmouse->usFlags),
							pmouse->lLastX, pmouse->lLastY, 
							pmouse->usButtonFlags, pmouse->usButtonData, pmouse->ulExtraInformation);
						break;
					case RIM_TYPEKEYBOARD:
						OutTrace("%s: KEYBOARD\n", ApiRef);
						break;
					case RIM_TYPEHID:
						OutTrace("%s: HID\n", ApiRef);
						break;
				}
			}
		}
	}
#endif // DXW_NOTRACES

	if(pData && (uiCommand == RID_INPUT)) {
		if(dxw.dwFlags4 & RELEASEMOUSE) {
			RAWINPUTHEADER *phdr;
			RAWMOUSE *pmouse;
			phdr = (RAWINPUTHEADER *)pData;
			if(phdr->dwType == RIM_TYPEMOUSE){
				pmouse = &((LPRAWINPUT)pData)->data.mouse;
				if(pmouse->usFlags & MOUSE_MOVE_ABSOLUTE){
					RECT r = dxw.GetUnmappedScreenRect();
					if ((pmouse->lLastX < r.left) ||
						(pmouse->lLastX > r.right) ||
						(pmouse->lLastY < r.top) ||
						(pmouse->lLastX > r.bottom)){
						OutTraceC("%s: released mouse event pos=(%d,%d)\n", ApiRef, pmouse->lLastX, pmouse->lLastY);
						return -1;
					}
				}
			}
		}

		// mouse coordinate scaling
		if(dxw.dwFlags10 & FIXMOUSERAWINPUT) {
			RAWINPUTHEADER *phdr;
			RAWMOUSE *pmouse;
			phdr = (RAWINPUTHEADER *)pData;
			if(phdr->dwType == RIM_TYPEMOUSE){
				pmouse = &((LPRAWINPUT)pData)->data.mouse;
				POINT point, prev;
				point.x = pmouse->lLastX;
				point.y = pmouse->lLastY;
				prev=point;
				if(pmouse->usFlags & MOUSE_MOVE_ABSOLUTE){
					dxw.UnmapClient(&point);
				}
				else {
					// todo: reminders handling can't be done with a single static variable, since
					// you're supposed to be here because there are more than 1 single mouse device!
					dxw.ScaleRelMouse(ApiRef, &point);
				}
				pmouse->lLastX = point.x;
				pmouse->lLastY = point.y;
				OutTraceC("%s: FIXED pos=(%d,%d)->(%d,%d)\n", ApiRef, prev.x, prev.y, point.x, point.y);
			}
		}
	}

	return ret;
}

UINT WINAPI extGetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader)
{
	UINT ret;
	ApiName("GetRawInputBuffer");

	OutTraceC("%s: data=%#x psize=%#x sizehdr=%#x\n", ApiRef, pData, pcbSize, cbSizeHeader);
	ret = (*pGetRawInputBuffer)(pData, pcbSize, cbSizeHeader);
#ifndef DXW_NOTRACES
	if(ret) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	} else {
		OutTraceC("%s: size=%d\n", ApiRef, *pcbSize);
	}
#endif // DXW_NOTRACES
	return ret;
}

static char *sRawInfoCmd(UINT c)
{
	char *p = "invalid";
	switch(c){
		case RIDI_DEVICENAME: p = "DEVICENAME"; break;
		case RIDI_PREPARSEDDATA: p = "PREPARSEDDATA"; break;
		case RIDI_DEVICEINFO: p = "DEVICEINFO"; break;
	}
	return p;
}

static UINT WINAPI extGetRawInputDeviceInfo(ApiArg, GetRawInputDeviceInfo_Type pGetRawInputDeviceInfo, BOOL isWide, 
											HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize)
{
	UINT ret;
#ifndef DXW_NOTRACES
	if(pData){
		OutTraceGDI("%s: hdev=%#x cmd=%#x(%s) pdata=%#x size=%d\n", ApiRef, hDevice, uiCommand, sRawInfoCmd(uiCommand), pData, *pcbSize);
	} else {
		OutTraceGDI("%s: hdev=%#x cmd=%#x(%s) pdata=NULL\n", ApiRef, hDevice, uiCommand, sRawInfoCmd(uiCommand));
	}
#endif // DXW_NOTRACES

	ret = (*pGetRawInputDeviceInfo)(hDevice, uiCommand, pData, pcbSize);
	if(ret == -1) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
		return ret;
	}

#ifndef DXW_NOTRACES
	OutTraceGDI("%s: ret=%d size=%d\n", ApiRef, ret, *pcbSize);
	if(pData && IsTraceGDI){
		RID_DEVICE_INFO *info;
		switch(uiCommand){
			case RIDI_DEVICENAME: 
				if(isWide){
					OutTrace("%s: NAME name=%ls\n", ApiRef, (PTCHAR)pData);
				} else {
					OutTrace("%s: NAME name=%s\n", ApiRef, (PCHAR)pData);
				}
				break;
			case RIDI_DEVICEINFO: 
				info = (RID_DEVICE_INFO *)pData;
				RID_DEVICE_INFO_HID *pHid;
				RID_DEVICE_INFO_KEYBOARD *pKbd;
				RID_DEVICE_INFO_MOUSE *pMouse;
				OutTraceGDI("%s: INFO size=%d type=%#x(%s)\n", ApiRef, info->cbSize, info->dwType, sRIMType(info->dwType));
				switch (info->dwType) {
					case RIM_TYPEMOUSE:
						pMouse = &(info->mouse);
						OutTrace("> id=%#x\n", pMouse->dwId);
						OutTrace("> nbuttons=%d\n", pMouse->dwNumberOfButtons);
						OutTrace("> samplerate=%d\n", pMouse->dwSampleRate);
						OutTrace("> haswheel=%s\n", pMouse->fHasHorizontalWheel ? "yes" : "no");
						break;
					case RIM_TYPEKEYBOARD:
						pKbd = &(info->keyboard);
						OutTrace("> type=%#x\n", pKbd->dwType);
						OutTrace("> subtype=%#x\n", pKbd->dwSubType);
						OutTrace("> mode=%#x\n", pKbd->dwKeyboardMode);
						OutTrace("> fkeys=%d\n", pKbd->dwNumberOfFunctionKeys);
						OutTrace("> indicators=%d\n", pKbd->dwNumberOfIndicators);
						OutTrace("> ktotal=%d\n", pKbd->dwNumberOfKeysTotal);
						break;
					case RIM_TYPEHID:
						pHid = &(info->hid);
						OutTrace("> vendor=%#x\n", pHid->dwVendorId);
						OutTrace("> product=%#x\n", pHid->dwProductId);
						OutTrace("> version=%#x\n", pHid->dwVersionNumber);
						OutTrace("> usagepage=%#x\n", pHid->usUsagePage);
						OutTrace("> usage=%#x\n", pHid->usUsage);
						break;
				}				
				break;
		}
	}
#endif // DXW_NOTRACES

	return ret;
}

UINT WINAPI extGetRawInputDeviceInfoA(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize)
{ ApiName("GetRawInputDeviceInfoA"); return extGetRawInputDeviceInfo(ApiRef, pGetRawInputDeviceInfoA, FALSE, hDevice, uiCommand, pData, pcbSize); }
UINT WINAPI extGetRawInputDeviceInfoW(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize)
{ ApiName("GetRawInputDeviceInfoA"); return extGetRawInputDeviceInfo(ApiRef, pGetRawInputDeviceInfoW, TRUE, hDevice, uiCommand, pData, pcbSize); }

UINT WINAPI extGetRawInputDeviceList(PRAWINPUTDEVICELIST pRawInputDeviceList, PUINT puiNumDevices, UINT cbSize)
{
	UINT ret;
	ApiName("GetRawInputDeviceList");

	if(pRawInputDeviceList == NULL){
		OutTraceGDI("%s: list=NULL cbSize=%d\n", ApiRef, cbSize);
	} else {
		OutTraceGDI("%s: list=%#x puiNumDevices=%d cbSize=%d\n", ApiRef, pRawInputDeviceList, *puiNumDevices, cbSize);
	}

	ret = (*pGetRawInputDeviceList)(pRawInputDeviceList, puiNumDevices, cbSize);
	if(ret == -1) {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
		return ret;
	} 

	OutTraceC("%s: ret=%d numdevices=%d size=%d\n", ApiRef, ret, *puiNumDevices, cbSize);
	for(UINT i=0; i<ret; i++){
		OutTraceGDI("> dev[%d]: hid=%#x type=%#x(%s)\n", i,
			pRawInputDeviceList[i].hDevice,
			pRawInputDeviceList[i].dwType, sRIMType(pRawInputDeviceList[i].dwType));
	}

	return ret;
}

#ifdef TRACESYSCALLS
/*
ScrollWindowEx
*/

int WINAPI extSetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO lpsi, BOOL redraw)
{
	int ret;
	ApiName("SetScrollInfo");

	OutTraceGDI("%s: hwnd=%#x nbar=%#x redraw=%#x\n", ApiRef, hwnd, nBar, redraw);
	OutTraceGDI("%s: lpsi={size=%d mask=%#x scroll(min,max)=(%d,%d) page=%d pos=%d trackpos=%d}\n", 
		ApiRef, 
		lpsi->cbSize,
		lpsi->fMask,
		lpsi->nMin, lpsi->nMax,
		lpsi->nPage,
		lpsi->nPos,
		lpsi->nTrackPos);

	ret = (*pSetScrollInfo)(hwnd, nBar, lpsi, redraw);
	OutTraceGDI("%s: ret(current pos)=%d\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extGetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO lpsi)
{
	BOOL ret;
	ApiName("GetScrollInfo");

	OutTraceGDI("%s: hwnd=%#x nbar=%#x\n", ApiRef, hwnd, nBar);

	ret = (*pGetScrollInfo)(hwnd, nBar, lpsi);

	if(ret){
		OutTraceGDI("%s: lpsi={size=%d mask=%#x scroll(min,max)=(%d,%d) page=%d pos=%d trackpos=%d}\n", 
			ApiRef, 
			lpsi->cbSize,
			lpsi->fMask,
			lpsi->nMin, lpsi->nMax,
			lpsi->nPage,
			lpsi->nPos,
			lpsi->nTrackPos);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

int WINAPI extSetScrollPos(HWND hwnd, int nBar, int nPos, BOOL redraw)
{
	int ret;
	ApiName("SetScrollPos");

	OutTraceGDI("%s: hwnd=%#x nbar=%#x pos=%d redraw=%#x\n", ApiRef, hwnd, nBar, redraw);

	ret = (*pSetScrollPos)(hwnd, nBar, nPos, redraw);
	OutTraceGDI("%s: ret(current pos)=%d\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extGetScrollPos(HWND hwnd, int nBar)
{
	BOOL ret;
	ApiName("GetScrollPos");

	OutTraceGDI("%s: hwnd=%#x nbar=%#x\n", ApiRef, hwnd, nBar);

	ret = (*pGetScrollPos)(hwnd, nBar);

	OutTraceGDI("%s: pos=%d\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extGetScrollRange(HWND hwnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos)
{
	BOOL ret;
	ApiName("GetScrollRange");

	OutTraceGDI("%s: hwnd=%#x nbar=%#x\n", ApiRef, hwnd, nBar);

	ret = (*pGetScrollRange)(hwnd, nBar, lpMinPos, lpMaxPos);

	if(ret){
		OutTraceGDI("%s: range(min,max)=(%d,%d)\n", ApiRef, *lpMinPos, *lpMaxPos);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extSetScrollRange(HWND hwnd, int nBar, int nMinPos, int nMaxPos, BOOL bRedraw)
{
	BOOL ret;
	ApiName("SetScrollRange");

	OutTraceGDI("%s: hwnd=%#x nbar=%#x range(min,max)=(%d,%d) redraw=%#x\n", ApiRef, hwnd, nBar, nMinPos, nMaxPos, bRedraw);

	ret = (*pSetScrollRange)(hwnd, nBar, nMinPos, nMaxPos, bRedraw);

	if(ret){
		OutTraceGDI("%s: ok\n", ApiRef);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extIsDialogMessageA(HWND hDlg, LPMSG lpMsg)
{
	BOOL ret;
	ApiName("IsDialogMessageA");

#ifndef DXW_NOTRACES
	OutTrace("%s: hdlg=%#x lpmsg=%#x\n", ApiRef, hDlg, lpMsg);
	ExplainMsg(ApiRef, TRUE, lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
#endif // DXW_NOTRACES

	ret = (*pIsDialogMessageA)(hDlg, lpMsg);
	return ret;
}
#endif // TRACESYSCALLS

#define GET_X_PARAM(v) ((short)((DWORD)v & 0xFFFF))
#define GET_Y_PARAM(v) ((short)(((DWORD)v & 0xFFFF0000) >> 16))
#define MAKELPOINT(p)	((DWORD)(((short)p.x) | ((DWORD)(short)p.y << 16)))

DWORD WINAPI extGetMessagePos(void)
{
	DWORD ret;
	ApiName("GetMessagePos");

	ret = (*pGetMessagePos)();
	OutTraceC("%s: pos=(%d,%d)\n", ApiRef, GET_X_PARAM(ret), GET_Y_PARAM(ret));

	if(dxw.MustFixMouse){
		POINT pt;
		pt.x = GET_X_PARAM(ret);
		pt.y = GET_Y_PARAM(ret);
		dxw.UnmapWindow(&pt);
#ifdef PROVENTOBEUSEFUL
		if(dxw.dwFlags20 & CENTERONEXIT){
			if((pt.x < 0) || (pt.x > dxw.GetScreenWidth())) pt.x = dxw.GetScreenWidth() / 2;
			if((pt.y < 0) || (pt.y > dxw.GetScreenHeight())) pt.y = dxw.GetScreenHeight() / 2;
		}
#endif
		ret = MAKELPOINT(pt);
		OutTraceC("%s: FIXED pos=(%d,%d)\n", ApiRef, GET_X_PARAM(ret), GET_Y_PARAM(ret));
	}

	GetHookInfo()->MessageX=GET_X_PARAM(ret);
	GetHookInfo()->MessageY=GET_Y_PARAM(ret);

	return ret;
}

#ifdef TRACEWAITOBJECTS
#ifndef DXW_NOTRACES
char *ExplainWakeMask(DWORD c)
{
	static char eb[256];
	unsigned int l;
	strcpy(eb,"QS_");
	if (c & QS_KEY) strcat(eb, "KEY+");
	if (c & QS_MOUSEMOVE) strcat(eb, "MOUSEMOVE+");
	if (c & QS_MOUSEBUTTON) strcat(eb, "MOUSEBUTTON+");
	if (c & QS_POSTMESSAGE) strcat(eb, "POSTMESSAGE+");
	if (c & QS_TIMER) strcat(eb, "TIMER+");
	if (c & QS_PAINT) strcat(eb, "PAINT+");
	if (c & QS_SENDMESSAGE) strcat(eb, "SENDMESSAGE+");
	if (c & QS_HOTKEY) strcat(eb, "HOTKEY+");
	if (c & QS_ALLPOSTMESSAGE) strcat(eb, "ALLPOSTMESSAGE+");
	if (c & QS_RAWINPUT) strcat(eb, "RAWINPUT+");
	l=strlen(eb);
	if (l>strlen("QS_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}

char *ExplainMsgWaitRetcode(DWORD ret)
{
	static char buf[26];
	if (ret == WAIT_FAILED) return "WAIT_FAILED";
	if (ret == WAIT_TIMEOUT) return "WAIT_TIMEOUT";
	if (ret > WAIT_ABANDONED_0){
		sprintf(buf, "WAIT_ABANDONED_%d", (ret - WAIT_ABANDONED_0));
		return buf;
	}
	sprintf(buf, "WAIT_OBJECT_%d", (ret - WAIT_OBJECT_0 ));
	return buf;
}
#endif // DXW_NOTRACES

DWORD WINAPI extMsgWaitForMultipleObjects(DWORD nCount, const HANDLE *pHandles, BOOL fWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask)
{
	DWORD ret;
	ApiName("MsgWaitForMultipleObjects");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: count=%d waitall=%#x msec=%d wakemask=%#x(%s)\n", ApiRef, nCount, fWaitAll, dwMilliseconds, dwWakeMask, ExplainWakeMask(dwWakeMask));
	for (DWORD i=0; i<nCount; i++) {
		OutTraceGDI("> handle[%d]=%#x\n", i, pHandles[i]);
	}
#endif

	ret = (*pMsgWaitForMultipleObjects)(nCount, pHandles, fWaitAll, dwMilliseconds, dwWakeMask);

	OutTraceGDI("%s: ret=%#x(%s)\n", ApiRef, ret, ExplainMsgWaitRetcode(ret));
	return ret;
}
#endif // TRACEWAITOBJECTS

#ifdef HACKVIRTUALKEYS
#define VKMAPTONUMPAD TRUE
UINT WINAPI extMapVirtualKeyA(UINT uCode, UINT uMapType)
{
	UINT res;
	ApiName("MapVirtualKeyA");

	OutTraceIN("%s: code=%#x maptype=%#x\n", ApiRef, uCode, uMapType);
	res = (*pMapVirtualKeyA)(uCode, uMapType);
	OutTraceIN("%s: ret=%#x\n", ApiRef, res);
	return res;
}

UINT WINAPI extMapVirtualKeyW(UINT uCode, UINT uMapType)
{
	UINT res;
	ApiName("MapVirtualKeyW");

	OutTraceIN("%s: code=%#x maptype=%#x\n", ApiRef, uCode, uMapType);
	res = (*pMapVirtualKeyW)(uCode, uMapType);
	OutTraceIN("%s: ret=%#x\n", ApiRef, res);
	return res;
}
#endif // HACKVIRTUALKEYS

SHORT WINAPI extGetAsyncKeyState(int vKey)
{
	SHORT res;
	ApiName("GetAsyncKeyState");
	// beware: this call can be used quite frequently, it can easily flood the logfile.
	OutDebugIN("%s: vkey=%#x\n", ApiRef, vKey);

	if(dxw.dwFlags14 & FLUSHKEYSTATE){
		MSG msg;
		while(PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));
	}

	res = (*pGetAsyncKeyState)(vKey);
	if((dxw.dwFlags11 & FIXASYNCKEYSTATE) && (GetForegroundWindow() != dxw.GethWnd())){
#ifndef DXW_NOTRACES
		if(res) OutDebugDW("%s: suppressed vkey=%#x\n", ApiRef, res);
#endif // DXW_NOTRACES
		res = 0;
	}

	if((dxw.dwFlags16 & CONFIRMESCAPE) && (vKey == VK_ESCAPE) && res){
		OutTrace("ESC detected in %s\n", ApiRef);
		//if(MessageBox(0, "Do you confirm the ESC key?", "DxWnd", MB_OKCANCEL) != IDOK) res |= 0x80;
		res = 0;
	}

	_if(res) OutDebugIN("%s: vkey=%#x res=%#hx(%s%s)\n", 
		ApiRef, vKey, res, 
		res & 0x8000 ? "DOWN" : "UP",
		res & 0x0001 ? "+PRESS" : "");
	return res;
}

// --- NLS

LPSTR WINAPI extCharPrevA(LPCSTR lpStart, LPCSTR lpCurrentChar) 
{
	LPSTR ret;
	ApiName("CharPrevA");
	ret = CharPrevExA((WORD)dxw.CodePage, lpStart, lpCurrentChar, 0);
	OutTraceGDI("%s: start=\"%s\" currentchar=\"%.1s\", next=\"%.1s\"\n", ApiRef, lpStart, lpCurrentChar, ret);
	return ret;
}

LPSTR WINAPI extCharNextA(LPCSTR lpCurrentChar) 
{
	LPSTR ret;
	ApiName("CharNextA");
	ret = CharNextExA((WORD)dxw.CodePage, lpCurrentChar, 0);
	OutTraceGDI("%s: currentchar=\"%.1s\", next=\"%.1s\"\n", ApiRef, lpCurrentChar, ret);
	return ret;
}

BOOL WINAPI extSetWindowTextA(HWND hWnd, LPCSTR lpString)
{
	BOOL res;
	ApiName("SetWindowTextA");

#ifndef DXW_NOTRACES
	__try{
		OutTraceGDI("%s: hwnd=%#x(%s) str=\"%s\"\n", 
			ApiRef, hWnd, IsWindowUnicode(hWnd) ? "WIDE" : "ANSI", lpString);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTraceGDI("%s: hwnd=%#x(%s)\n", 
			ApiRef, hWnd, IsWindowUnicode(hWnd) ? "WIDE" : "ANSI");
	}
#endif // DXW_NOTRACES

	// v2.05.63: despite all attempts, it may happen that the target window wasn't created as a Unicode
	// window but is an ASCII window instead. In this case, better keep sending the ASCII text, this way
	// at least the ASCII part of the text will be correct.
	//if((dxw.dwFlags11 & CUSTOMLOCALE) && IsWindowUnicode(hWnd)) {
	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		LPWSTR wstr = NULL;
		int n;
		if (lpString) {
			int size = lstrlenA(lpString);
			wstr = (LPWSTR)malloc((size+1)<<1);
			n = MultiByteToWideChar(dxw.CodePage, 0, lpString, size, wstr, size);
			wstr[n] = L'\0'; // make tail ! 
		}
		res = (*pSetWindowTextW)(hWnd, wstr);
		if (wstr) free((LPVOID)wstr);
	}
	else {
		res = (*pSetWindowTextA)(hWnd, lpString);
	}
	IfTraceError();
	return res;
}

BOOL WINAPI extSetWindowTextW(HWND hWnd, LPCWSTR lpString)
{
	BOOL res;
	ApiName("SetWindowTextW");

	OutTraceGDI("%s: hwnd=%#x(%s) str=\"%ls\"\n", 
		ApiRef, hWnd, IsWindowUnicode(hWnd) ? "WIDE" : "ANSI", lpString);

	if((dxw.dwFlags11 & CUSTOMLOCALE) && IsWindowUnicode(hWnd)) {
#ifdef METHOD1
		res = DefWindowProcW(hWnd, WM_SETTEXT, NULL, (LPARAM)lpString);
#else
		LONG_PTR OrigWndP = GetWindowLongPtrW(hWnd, GWLP_WNDPROC);
		if(!pDefWindowProcW) pDefWindowProcW = DefWindowProcW;
		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)pDefWindowProcW);
		res = (*pSetWindowTextW)(hWnd, lpString);
		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)OrigWndP);
#endif
		IfTraceError();
		return res;
	}

	res = (*pSetWindowTextW)(hWnd, lpString);
	IfTraceError();
	return res;
}

int WINAPI extGetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
	int ret;
	ApiName("GetWindowTextA");

	OutTraceGDI("%s: hwnd=%#x count=%d\n", ApiRef, hWnd, nMaxCount);

	if((dxw.dwFlags11 & CUSTOMLOCALE) && IsWindowUnicode(hWnd)) {
		if(!pSendMessageW) pSendMessageW = SendMessageW;
		int len = (int)(*pSendMessageW)(hWnd, WM_GETTEXTLENGTH, 0, 0) + 1;
		LPWSTR lpStringW = (LPWSTR)malloc(len * sizeof(wchar_t));
		// see comments above for SetWindowTextA
#ifdef METHOD1
		ret = DefWindowProcW(hWnd, WM_GETTEXT, (WPARAM)len, (LPARAM)lpStringW);
#else
		LONG_PTR OrigWndP = GetWindowLongPtrW(hWnd, GWLP_WNDPROC);
		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)DefWindowProcW);
		ret = GetWindowTextW(hWnd, lpStringW, len);
		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)OrigWndP);
#endif
		if (ret > 0) {
			int size = WideCharToMultiByte(CP_ACP, 0, lpStringW, -1, lpString, nMaxCount, NULL, NULL);
			if (size > 0) ret = size - 1;
			else {
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
					lpString[nMaxCount - 1] = '\0'; ret = nMaxCount - 1;
				}
				else {
					lpString[0] = '\0'; ret = 0;
				}
			}
		}
		else {
			lpString[0] = '\0'; ret = 0;
		}
		free(lpStringW);
	}
	else {
		ret = (*pGetWindowTextA)(hWnd, lpString, nMaxCount);
	}

#if 0
	// purpose of this wrapped call is to clear the FPS indicator (format " ~ (%d FPS)") 
	// from the window title, if present. It crashes games such as "Panzer General 3 Scorched Earth"
	// when FPS on window title is activated.
	int ret;
	OutTraceGDI("GetWindowTextA: hwnd=%#x MaxCount=%d\n", hWnd, nMaxCount);
	ret=(*pGetWindowTextA)(hWnd, lpString, nMaxCount);
	if(ret) OutTraceGDI("GetWindowTextA: ret=%d String=\"%s\"\n", ret, lpString);
	if (ret && (dxw.dwFlags2 & SHOWFPS) && dxw.ishWndFPS(hWnd)){
		char *p;
		p=strstr(lpString, " ~ (");
		if(p){
			*p = NULL;
			ret = strlen(lpString);
			OutTraceGDI("GetWindowTextA: FIXED ret=%d String=\"%s\"\n", ret, lpString);
		}
	}
#endif

	OutTraceGDI("%s: lpstring=\"%s\" ret=%#x\n", ApiRef, lpString, ret);
	return ret;
}

static BOOL WINAPI intOemToCharBuffA(LPCSTR pSrc, LPSTR pDest, DWORD cchDestLen)
{
    WCHAR *bufW;
	int n;

	if (!pSrc || !pDest || !cchDestLen) return FALSE;

	bufW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, cchDestLen * sizeof(WCHAR));
	if(bufW){
		MultiByteToWideChar(CP_OEMCP, 0, pSrc, cchDestLen, bufW, cchDestLen );
		n=WideCharToMultiByte(CP_ACP, 0, bufW, cchDestLen, pDest, cchDestLen, NULL, NULL );
		pDest[n]=0;
		HeapFree(GetProcessHeap(), 0, bufW );
	}
    return TRUE;
}

BOOL WINAPI extOemToCharA(LPCSTR pSrc, LPSTR pDest)
{
	ApiName("OemToCharA");
	BOOL ret;

	OutTraceLOC("%s: src=%#x(\"%s\") dst=%x\n", ApiRef, pSrc, pSrc ? pSrc : "(NULL)", pDest);

	if(!pSrc || !pDest) return FALSE;

	if(dxw.dwFlags12 & PATHLOCALE){
		ret = intOemToCharBuffA(pSrc, pDest, strlen(pSrc) + 1);    
	}
	else {
		ret = (*pOemToCharA)(pSrc, pDest);
	}
	if(ret){
		OutTraceLOC("%s: OK dst=\"%s\"\n", ApiRef, pDest);
	}
	else {
		OutTraceLOC("%s: ERROR err=%d\n", pSrc, pDest, GetLastError);
	}
	return ret;
}

BOOL WINAPI extOemToCharBuffA(LPCSTR pSrc, LPSTR pDest, DWORD cchDestLen)
{
	BOOL ret;
	ApiName("OemToCharBuffA");

	if(dxw.dwFlags12 & PATHLOCALE){
		ret = intOemToCharBuffA(pSrc, pDest, cchDestLen);
	}
	else {
		ret = (*pOemToCharBuffA)(pSrc, pDest, cchDestLen);	
	}
	OutTraceLOC("%s: src=\"%s\" dst=\"%s\" len=%d ret=%#x\n", ApiRef, pSrc, pDest, cchDestLen, ret);
	return ret;
}

static BOOL WINAPI intCharToOemBuffA(LPCSTR pSrc, LPSTR pDest, DWORD cchDestLen)
{
    WCHAR *bufW;
	int n;

	if (!pSrc || !pDest || !cchDestLen) return FALSE;

	bufW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, cchDestLen * sizeof(WCHAR));
	if(bufW){
		MultiByteToWideChar(CP_ACP, 0, pSrc, cchDestLen, bufW, cchDestLen );
		n=WideCharToMultiByte(CP_OEMCP, 0, bufW, cchDestLen, pDest, cchDestLen, NULL, NULL );
		pDest[n]=0;
		HeapFree(GetProcessHeap(), 0, bufW );
	}
    return TRUE;
}

BOOL WINAPI extCharToOemA(LPCSTR pSrc, LPSTR pDest)
{
	ApiName("CharToOemA");
	BOOL ret;

	OutTraceLOC("%s: src=%#x(\"%s\") dst=%x\n", ApiRef, pSrc, pSrc ? pSrc : "(NULL)", pDest);
	
	if(!pSrc || !pDest) return FALSE;

	if(dxw.dwFlags12 & PATHLOCALE){
		ret = intCharToOemBuffA(pSrc, pDest, strlen(pSrc) + 1);    
	}
	else {
		ret = (*pCharToOemA)(pSrc, pDest);
	}
	if(ret){
		OutTraceLOC("%s: OK dst=\"%s\"\n", ApiRef, pDest);
	}
	else {
		OutTraceLOC("%s: ERROR err=%d\n", pSrc, pDest, GetLastError);
	}
	return ret;
}

BOOL WINAPI extCharToOemBuffA(LPCSTR pSrc, LPSTR pDest, DWORD cchDestLen)
{
	BOOL ret;
	ApiName("CharToOemBuffA");

	if(dxw.dwFlags12 & PATHLOCALE){
		ret = intCharToOemBuffA(pSrc, pDest, cchDestLen);
	}
	else {
		ret = (*pCharToOemBuffA)(pSrc, pDest, cchDestLen);	
	}
	OutTraceLOC("%s: src=\"%s\" dst=\"%s\" len=%d ret=%#x\n", ApiRef, pSrc, pDest, cchDestLen, ret);
	return ret;
}

extern char *ExplainRgnType(int);

int WINAPI extSetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw)
{
	int ret;
	ApiName("SetWindowRgn");

	OutTraceGDI("%s: hwnd=%#x hrgn=%#x redraw=%#x\n", ApiRef, hWnd, hRgn, bRedraw);   

	if(dxw.IsFullScreen() && (dxw.GDIEmulationMode != GDIMODE_NONE)){
		//(*pDeleteObject)(hrgnScaled); -- https://docs.microsoft.com/it-it/windows/win32/api/winuser/nf-winuser-setwindowrgn
		// After a successful call to SetWindowRgn, the system owns the region specified by the region handle hRgn. 
		// The system does not make a copy of the region. 
		// Thus, you should not make any further function calls with this region handle. 
		// In particular, do not delete this region handle. 
		HRGN hrgnScaled;
		hrgnScaled = dxw.MapRegion(ApiRef, hRgn);
		CombineRgn(hRgn, hrgnScaled, NULL, RGN_COPY);
		(*pDeleteObject)(hrgnScaled);
	} 

	ret = (*pSetWindowRgn)(hWnd, hRgn, bRedraw);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
	return ret; 
}

int WINAPI extGetWindowRgn(HWND hWnd, HRGN hRgn)
{
	int ret;
	ApiName("GetWindowRgn");

	OutTraceGDI("%s: hwnd=%#x hrgn=%#x\n", ApiRef, hWnd, hRgn);    

	ret = (*pGetWindowRgn)(hWnd, hRgn);
	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));

	// to do: leave owned hRgn unscaled !!!
	if(dxw.IsFullScreen() && (dxw.GDIEmulationMode != GDIMODE_NONE)){
		HRGN hrgnScaled;
		hrgnScaled = dxw.UnmapRegion(ApiRef, hRgn);
		CombineRgn(hRgn, hrgnScaled, NULL, RGN_COPY);
		(*pDeleteObject)(hrgnScaled);
	} 

	OutTraceGDI("%s: ret=%d(%s)\n", ApiRef, ret, ExplainRgnType(ret));
	return ret; 
}

// --------------------------------------------------------------------
// point trace wrappers
// --------------------------------------------------------------------

#ifdef TRACEPOINTS
BOOL WINAPI extPtInRect(CONST RECT *lprc, POINT pt)
{
	BOOL ret;
	ApiName("PtInRect");
	ret = (*pPtInRect)(lprc, pt);
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d) pt=(%d,%d) ret=%#x(%s)\n",
		ApiRef,
		lprc->left, lprc->top, lprc->right, lprc->bottom,
		pt.x, pt.y,
		ret, ret ? "IN" : "OUT");
	return ret;
}

BOOL WINAPI extSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
	BOOL ret;
	ApiName("SetRect");
	ret = (*pSetRect)(lprc, xLeft, yTop, xRight, yBottom);
	if(ret) {
		OutTraceGDI("%s: rect=(%d,%d)-(%d,%d)\n",
		ApiRef,
		lprc->left, lprc->top, lprc->right, lprc->bottom);
	} 
	else {
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extSetRectEmpty(LPRECT lprc)
{
	BOOL ret;
	ApiName("SetRectEmpty");
	ret = (*pSetRectEmpty)(lprc);
	if(ret) {
		OutTraceGDI("%s: rect=(%d,%d)-(%d,%d)\n",
		ApiRef,
		lprc->left, lprc->top, lprc->right, lprc->bottom);
	} 
	else {
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extOffsetRect(LPRECT lprc, int dx, int dy)
{
	BOOL ret;
	ApiName("OffsetRect");
	ret = (*pOffsetRect)(lprc, dx, dy);
	if(ret) {
		OutTraceGDI("%s: delta=(%d,%d) rect=(%d,%d)-(%d,%d)\n",
		ApiRef,
		dx, dy,
		lprc->left, lprc->top, lprc->right, lprc->bottom);
	} 
	else {
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extInflateRect(LPRECT lprc, int dx, int dy)
{
	BOOL ret;
	ApiName("InflateRect");
	ret = (*pInflateRect)(lprc, dx, dy);
	if(ret) {
		OutTraceGDI("%s: delta=(%d,%d) rect=(%d,%d)-(%d,%d)\n",
		ApiRef,
		dx, dy,
		lprc->left, lprc->top, lprc->right, lprc->bottom);
	} 
	else {
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extSubtractRect(LPRECT lprcDst, CONST RECT *lprcSrc1, CONST RECT *lprcSrc2)
{
	BOOL ret;
	ApiName("SubtractRect");
	OutTraceGDI("%s: rect1=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprcSrc1->left, lprcSrc1->top, lprcSrc1->right, lprcSrc1->bottom);
	OutTraceGDI("%s: rect2=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprcSrc2->left, lprcSrc2->top, lprcSrc2->right, lprcSrc2->bottom);
	ret = (*pSubtractRect)(lprcDst, lprcSrc1, lprcSrc2);
	if(ret) {
		OutTraceGDI("%s: dest=(%d,%d)-(%d,%d)\n", 
			ApiRef, lprcDst->left, lprcDst->top, lprcDst->right, lprcDst->bottom);
	} 
	else {
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extUnionRect(LPRECT lprcDst, CONST RECT *lprcSrc1, CONST RECT *lprcSrc2)
{
	BOOL ret;
	ApiName("UnionRect");
	OutTraceGDI("%s: rect1=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprcSrc1->left, lprcSrc1->top, lprcSrc1->right, lprcSrc1->bottom);
	OutTraceGDI("%s: rect2=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprcSrc2->left, lprcSrc2->top, lprcSrc2->right, lprcSrc2->bottom);
	ret = (*pUnionRect)(lprcDst, lprcSrc1, lprcSrc2);
	if(ret) {
		OutTraceGDI("%s: dest=(%d,%d)-(%d,%d)\n", 
			ApiRef, lprcDst->left, lprcDst->top, lprcDst->right, lprcDst->bottom);
	} 
	else {
		OutTraceGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extIntersectRect(LPRECT lprcDst, CONST RECT *lprcSrc1, CONST RECT *lprcSrc2)
{
	BOOL ret;
	ApiName("IntersectRect");
	OutTraceGDI("%s: rect1=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprcSrc1->left, lprcSrc1->top, lprcSrc1->right, lprcSrc1->bottom);
	OutTraceGDI("%s: rect2=(%d,%d)-(%d,%d)\n", 
		ApiRef, lprcSrc2->left, lprcSrc2->top, lprcSrc2->right, lprcSrc2->bottom);
	ret = (*pIntersectRect)(lprcDst, lprcSrc1, lprcSrc2);
	if(ret) {
		OutTrace("%s: dest=(%d,%d)-(%d,%d)\n", 
			ApiRef, lprcDst->left, lprcDst->top, lprcDst->right, lprcDst->bottom);
	} 
	else {
		OutTrace("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extIsRectEmpty(CONST RECT *lprc)
{
	BOOL ret;
	ApiName("IsRectEmpty");
	ret = (*pIsRectEmpty)(lprc);
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d) ret=%#x(%s)\n",
		ApiRef,
		lprc->left, lprc->top, lprc->right, lprc->bottom,
		ret, ret ? "YES" : "NO");
	return ret;
}

BOOL WINAPI extCopyRect(LPRECT lprcDst, CONST RECT *lprcSrc)
{
	BOOL ret;
	ApiName("CopyRect");
	ret = (*pCopyRect)(lprcDst, lprcSrc);
	OutTraceGDI("%s: rect=(%d,%d)-(%d,%d) ret=%#x(%s)\n",
		ApiRef,
		lprcSrc->left, lprcSrc->top, lprcSrc->right, lprcSrc->bottom,
		ret, ret ? "YES" : "NO");
	return ret;
}
#endif // TRACEPOINTS

#ifdef TRACEWINDOWS
BOOL WINAPI extEnableWindow(HWND hWnd, BOOL bEnable)
{
	BOOL ret;
	ApiName("EnableWindow");
	ret = (*pEnableWindow)(hWnd, bEnable);
	OutTraceGDI("%s: hwnd=%#x enable=%#x ret(prev state)=%#x\n",
		ApiRef, hWnd, bEnable, ret);
	return ret;
}
#endif // TRACEWINDOWS

#ifdef TRACEIMAGES
HBITMAP WINAPI extLoadBitmapA(HINSTANCE hInstance, LPCSTR lpBitmapName)
{
	HBITMAP ret;
	ApiName("LoadBitmapA");

	OutTraceGDI("%s: hinst=%#x name=%s\n", ApiRef, hInstance, ClassToStr(lpBitmapName));
	ret = (*pLoadBitmapA)(hInstance, lpBitmapName);
	if(ret){
		if(dxw.dwDFlags2 & DUMPBITMAPS) DumpBitmap(ApiRef, (HBITMAP)ret);
		OutTraceGDI("%s: hbitmap=%#x\n", ApiRef, ret);
	}
	else{
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

HBITMAP WINAPI extLoadBitmapW(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
	HBITMAP ret;
	ApiName("LoadBitmapW");

	OutTraceGDI("%s: hinst=%#x name=%ls\n", ApiRef, hInstance, ClassToWStr(lpBitmapName));
	ret = (*pLoadBitmapW)(hInstance, lpBitmapName);
	if(ret){
		if(dxw.dwDFlags2 & DUMPBITMAPS) DumpBitmap(ApiRef, (HBITMAP)ret);
		OutTraceGDI("%s: hbitmap=%#x\n", ApiRef, ret);
	}
	else{
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}
#endif // TRACEIMAGES

HANDLE WINAPI extLoadImageA(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad)
{
	HANDLE res;
	ApiName("LoadImageA");

	OutTraceGDI("%s: hinst=%#x name=\"%s\" type=%d xy=(%d,%d) load=%#x\n",
		ApiRef, hInst, ClassToStr(name), type, cx, cy, fuLoad);

	if(dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE)){
		if((fuLoad & LR_LOADFROMFILE) && name){
			name = dxwTranslatePathA(name, NULL);
			OutTraceGDI("%s: mapping path=\"%s\"\n", ApiRef, name);
		}
	}

	// v2.05.69: translate path to widechar only when not an ATOM
	if((dxw.dwFlags12 & PATHLOCALE) && ((DWORD)name & 0xFFFF0000) != 0){
		int size = strlen(name);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, name, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		if(pLoadImageW == NULL) pLoadImageW = LoadImageW;
		res=(*pLoadImageW)(hInst, lpFileNameW, type, cx, cy, fuLoad);
		free(lpFileNameW);
	}
	else {
		res=(*pLoadImageA)(hInst, name, type, cx, cy, fuLoad);
	}

#ifndef DXW_NOTRACES
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	else {
		if((dxw.dwDFlags2 & DUMPBITMAPS) && (type == IMAGE_BITMAP)) DumpBitmap(ApiRef, (HBITMAP)res);
		OutTraceGDI("%s: ret=%#x\n", ApiRef, res);
	}
#endif // DXW_NOTRACES

	// newt tweak/compat flag: W98LOADIMAGE ??
	if( !res && 
		(GetLastError() == ERROR_FILE_NOT_FOUND) && 
		((DWORD)name & 0xFFFF0000) && 
		!(fuLoad & LR_LOADFROMFILE)) 
	{
		// try again with LR_LOADFROMFILE
		OutTraceGDI("%s: retry with LR_LOADFROMFILE\n", ApiRef);
		res = extLoadImageA(hInst, name, type, cx, cy, fuLoad | LR_LOADFROMFILE);
	}

	return res;
}

HANDLE WINAPI extLoadImageW(HINSTANCE hInst, LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad)
{
	HANDLE res;
	ApiName("LoadImageW");

	OutTraceGDI("%s: hinst=%#x name=\"%ls\" type=%d xy=(%d,%d) load=%#x\n",
		ApiRef, hInst, ClassToWStr(name), type, cx, cy, fuLoad);

	if(dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE)){
		if((fuLoad & LR_LOADFROMFILE) && name){
			name = dxwTranslatePathW(name, NULL);
			OutTraceGDI("%s: mapping path=\"%ls\"\n", ApiRef, name);
		}
	}

	res=(*pLoadImageW)(hInst, name, type, cx, cy, fuLoad);
#ifndef DXW_NOTRACES
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	else {
		if((dxw.dwDFlags2 & DUMPBITMAPS) && (type == IMAGE_BITMAP)) DumpBitmap(ApiRef, (HBITMAP)res);
		OutTraceGDI("%s: ret=%#x\n", ApiRef, res);
	}
#endif // DXW_NOTRACES

	return res;
}

BOOL WINAPI extDrawEdge(HDC hdc, LPRECT qrc, UINT edge, UINT grfFlags)
{
	BOOL res;
	ApiName("DrawEdge");

	// MessageBox(0, ApiRef, "DxWnd", 0);
	OutTraceGDI("%s: hdc=%#x qrc=(%d,%d)-(%d,%d) edge=%d flags=%#x\n", 
		ApiRef, hdc,
		qrc->left, qrc->top, qrc->right, qrc->bottom,
		edge, grfFlags);

	if(dxw.IsToRemap(hdc)){
		RECT scaledqrc;
		switch(dxw.GDIEmulationMode){
			case GDIMODE_SHAREDDC: 
				sdc.GetPrimaryDC(hdc);
				res=(*pDrawEdge)(sdc.GetHdc(), qrc, edge, grfFlags);
				sdc.PutPrimaryDC(hdc, TRUE);
				return res;
				break;
			case GDIMODE_STRETCHED:
				scaledqrc = *qrc;
				dxw.MapClient(&scaledqrc);
				qrc = &scaledqrc;
				break;
			default:
				break;
		}
	}

	res = (*pDrawEdge)(hdc, qrc, edge, grfFlags);
	IfTraceError();
	return res;
}

HMENU WINAPI extGetMenu(HWND hwnd)
{
	HMENU hmenu;
	ApiName("GetMenu");
	hmenu = (*pGetMenu)(hwnd);
	if(hmenu && (dxw.dwFlags12 & SUPPRESSMENUS)){
		OutTraceGDI("%s: SUPPRESS hmenu=%#x whnd=%#x\n", ApiRef, hmenu, hwnd);
		if (!(*pSetMenu)(hwnd, NULL)) OutErrorGDI("SetMenu ERROR: err=%d\n", GetLastError());
		if (!DestroyMenu(hmenu)) OutErrorGDI("DestroyMenu ERROR: err=%d\n", GetLastError());
		hmenu = NULL;
	}
	OutTraceGDI("%s: hwnd=%#x hmenu=%#x\n", ApiRef, hwnd, hmenu);
	return hmenu;
}

BOOL WINAPI extSetMenu(HWND hwnd, HMENU hmenu)
{
	BOOL res;
	ApiName("SetMenu");
	OutTraceGDI("%s: hwnd=%#x hmenu=%#x\n", ApiRef, hwnd, hmenu);
	if(dxw.dwFlags12 & SUPPRESSMENUS){
		OutTraceGDI("%s: SUPPRESS hmenu=%#x whnd=%#x\n", ApiRef, hmenu, hwnd);
		hmenu = NULL;
	}
	res = (*pSetMenu)(hwnd, hmenu);
	if(res) {
		dxw.hWndMenu = hwnd;
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

LONG WINAPI extGetDialogBaseUnits()
{
	BOOL res;
	ApiName("GetDialogBaseUnits");
	res = (*pGetDialogBaseUnits)();
	OutTraceGDI("%s: res(hxw)=(%dx%d)\n", ApiRef, res>>16, res&0xFFFF);
	// v2.05.60: tweak LEGACYBASEUNITS to let "Aaron vs. Ruth" bypass the check
	if((dxw.dwFlags13 & LEGACYBASEUNITS) && ((res & 0xFFFF)> 8)) {
		OutTraceGDI("%s: TRIM res(hxw)=(16x8)\n", ApiRef, res);
		res = 0x00100008;
	}
	return res;
}

#ifdef TRACEMENUS
LONG WINAPI extGetMenuCheckMarkDimensions()
{
	LONG res;
	ApiName("GetMenuCheckMarkDimensions");
	res = (*pGetMenuCheckMarkDimensions)();
	OutTraceGDI("%s: ret(MenuCheckMarkDimensions)=%#x dim=(%dx%d)\n", ApiRef, res, res & 0xFFFF, res >> 16);
	return res;
}

HMENU WINAPI extCreateMenu(void)
{
	HMENU res;
	ApiName("CreateMenu");
	res = (*pCreateMenu)();
	OutTraceGDI("%s: hmenu=%#x\n", ApiRef, res);
	return res;
}

HMENU WINAPI extCreatePopupMenu(void)
{
	HMENU res;
	ApiName("CreatePopupMenu");
	res = (*pCreatePopupMenu)();
	OutTraceGDI("%s: hmenu=%#x\n", ApiRef, res);
	return res;
}

UINT WINAPI extGetMenuState(HMENU hMenu, UINT uId, UINT uFlags)
{
	UINT res;
	ApiName("GetMenuState");
	res = (*pGetMenuState)(hMenu, uId, uFlags);
	OutTraceGDI("%s: hmenu=%#x id=%d flags=%#x res=%#x\n", ApiRef, hMenu, uId, uFlags, res);
	return res;
}

BOOL WINAPI extGetMenuItemInfoA(HMENU hmenu, UINT item, BOOL fByPosition, LPMENUITEMINFOA lpmii)
{
	UINT res;
	ApiName("GetMenuItemInfoA");
	OutTraceGDI("%s: hmenu=%#x item=%d bypos=%d\n", ApiRef, hmenu, item, fByPosition);
	res = (*pGetMenuItemInfoA)(hmenu, item, fByPosition, lpmii);
	if(res){
		if(IsTraceGDI){
			OutTrace("> fMask=%#x\n", lpmii->fMask);
			OutTrace("> fType=%#x\n", lpmii->fType);
			OutTrace("> fState=%#x\n", lpmii->fState);
			OutTrace("> wID=%#x\n", lpmii->wID);
			OutTrace("> hSubMenu=%#x\n", lpmii->hSubMenu);
			OutTrace("> hbmpChecked=%#x\n", lpmii->hbmpChecked);
			OutTrace("> hbmpUnchecked=%#x\n", lpmii->hbmpUnchecked);
			if(lpmii->fMask && (lpmii->fType & MFT_STRING)) {
				OutTrace("> dwTypeData=\"%s\"\n", lpmii->dwTypeData);
			}
			else {
				OutTrace("> dwTypeData=%#x\n", lpmii->dwTypeData);
			}
			OutTrace("> cch=%d\n", lpmii->cch);
			OutTrace("> hbmpItem=%#x\n", lpmii->hbmpItem);		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extGetMenuItemInfoW(HMENU hmenu, UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii)
{
	UINT res;
	ApiName("GetMenuItemInfoW");
	OutTraceGDI("%s: hmenu=%#x item=%d bypos=%d\n", ApiRef, hmenu, item, fByPosition);
	res = (*pGetMenuItemInfoW)(hmenu, item, fByPosition, lpmii);
	if(res){
		if(IsTraceGDI){
			OutTrace("> fMask=%#x\n", lpmii->fMask);
			OutTrace("> fType=%#x\n", lpmii->fType);
			OutTrace("> fState=%#x\n", lpmii->fState);
			OutTrace("> wID=%#x\n", lpmii->wID);
			OutTrace("> hSubMenu=%#x\n", lpmii->hSubMenu);
			OutTrace("> hbmpChecked=%#x\n", lpmii->hbmpChecked);
			OutTrace("> hbmpUnchecked=%#x\n", lpmii->hbmpUnchecked);
			if(lpmii->fMask && (lpmii->fType & MFT_STRING)) {
				OutTrace("> dwTypeData=\"%ls\"\n", lpmii->dwTypeData);
			}
			else {
				OutTrace("> dwTypeData=%#x\n", lpmii->dwTypeData);
			}
			OutTrace("> cch=%d\n", lpmii->cch);
			OutTrace("> hbmpItem=%#x\n", lpmii->hbmpItem);
		}
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extRemoveMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
	BOOL res;
	ApiName("RemoveMenu");
	OutTraceGDI("%s: hmenu=%#x position=%d flags=%#x(%s)\n", 
		ApiRef, hMenu, uPosition, uFlags, (uFlags == MF_BYPOSITION) ? "BYPOSITION" : "BYCOMMAND");
	res = (*pRemoveMenu)(hMenu, uPosition, uFlags);
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extDeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
	BOOL res;
	ApiName("DeleteMenu");
	OutTraceGDI("%s: hmenu=%#x position=%d flags=%#x(%s)\n", 
		ApiRef, hMenu, uPosition, uFlags, (uFlags == MF_BYPOSITION) ? "BYPOSITION" : "BYCOMMAND");
	res = (*pDeleteMenu)(hMenu, uPosition, uFlags);
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extDestroyMenu(HMENU hMenu)
{
	BOOL res;
	ApiName("DestroyMenu");
	OutTraceGDI("%s: hmenu=%#x\n", ApiRef, hMenu);
	res = (*pDestroyMenu)(hMenu);
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extModifyMenuA(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem)
{
	BOOL res;
	ApiName("ModifyMenuA");
	OutTraceGDI("%s: hmenu=%#x pos=%d flags=%#x newItem=%s\n", 
		ApiRef, hMenu, uPosition, uFlags, lpNewItem ? lpNewItem : "");
	res = (*pModifyMenuA)(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	else {
		OutTraceGDI("%s: idNewItem=%#x\n", ApiRef, uIDNewItem);
	}
	return res;
}

BOOL WINAPI extSetMenuItemBitmaps(HMENU hMenu, UINT uPosition, UINT uFlags, HBITMAP hBitmapUnchecked, HBITMAP hBitmapChecked)
{
	BOOL ret;
	ApiName("SetMenuItemBitmaps");

	OutTraceSYS("%s: hmenu=%#x pos=%d flags=%#x bmp1=%#x bmp2=%#x\n", ApiRef, hMenu, uPosition, uFlags, hBitmapUnchecked, hBitmapChecked);

	if(dxw.dwDFlags2 & DUMPBITMAPS) {
		if(hBitmapUnchecked) DumpBitmap(ApiRef, (HBITMAP)hBitmapUnchecked);
		if(hBitmapChecked) DumpBitmap(ApiRef, (HBITMAP)hBitmapChecked);
	}

	ret = (*pSetMenuItemBitmaps)(hMenu, uPosition, uFlags, hBitmapUnchecked, hBitmapChecked);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}
#endif // TRACEMENUS

#ifndef DXW_NOTRACES

static void DumpMIIA(LPCMENUITEMINFOA lpmii)
{
	OutTrace("> cbSize=%#x\n", lpmii->cbSize);
	OutTrace("> fMask=%#x( %s%s%s%s%s%s%s%s%s)\n", lpmii->fMask,
		(lpmii->fMask & MIIM_STATE) ? "STATE " : "",
		(lpmii->fMask & MIIM_ID) ? "ID " : "",
		(lpmii->fMask & MIIM_SUBMENU) ? "SUBMENU " : "",
		(lpmii->fMask & MIIM_CHECKMARKS) ? "CHECKMARKS " : "",
		(lpmii->fMask & MIIM_TYPE) ? "TYPE " : "",
		(lpmii->fMask & MIIM_DATA) ? "DATA " : "",
		(lpmii->fMask & MIIM_STRING) ? "STRING " : "",
		(lpmii->fMask & MIIM_BITMAP) ? "BITMAP " : "",
		(lpmii->fMask & MIIM_FTYPE) ? "FTYPE " : ""
		);
	if(lpmii->fMask & (MIIM_FTYPE|MIIM_TYPE)) OutTrace("> fType=%#x\n", lpmii->fType);
	if(lpmii->fMask & MIIM_STATE) OutTrace("> fState=%#x\n", lpmii->fState);
	if(lpmii->fMask & MIIM_ID) OutTrace("> wID=%#x\n", lpmii->wID);
	if(lpmii->fMask & MIIM_SUBMENU) OutTrace("> hSubMenu=%#x\n", lpmii->hSubMenu);
	if(lpmii->fMask & MIIM_CHECKMARKS) {
		OutTrace("> hbmpChecked=%#x\n", lpmii->hbmpChecked);
		OutTrace("> hbmpUnchecked=%#x\n", lpmii->hbmpUnchecked);
	}
	if (((lpmii->fMask & (MIIM_TYPE|MIIM_STRING)) == (MIIM_TYPE|MIIM_STRING)) && 
		(lpmii->fType == MFT_STRING)) {
		OutTrace("> dwTypeData=\"%s\"\n", lpmii->dwTypeData);
	}
	OutTrace("> cch=%d\n", lpmii->cch);
	if(lpmii->fMask & MIIM_BITMAP) OutTrace("> hbmpItem=%#x\n", lpmii->hbmpItem);
}

#ifdef TRACEMENUS
static void DumpMIIW(LPCMENUITEMINFOW lpmii)
{
	OutTrace("> cbSize=%#x\n", lpmii->cbSize);
	OutTrace("> fMask=%#x( %s%s%s%s%s%s%s%s%s)\n", lpmii->fMask,
		(lpmii->fMask & MIIM_STATE) ? "STATE " : "",
		(lpmii->fMask & MIIM_ID) ? "ID " : "",
		(lpmii->fMask & MIIM_SUBMENU) ? "SUBMENU " : "",
		(lpmii->fMask & MIIM_CHECKMARKS) ? "CHECKMARKS " : "",
		(lpmii->fMask & MIIM_TYPE) ? "TYPE " : "",
		(lpmii->fMask & MIIM_DATA) ? "DATA " : "",
		(lpmii->fMask & MIIM_STRING) ? "STRING " : "",
		(lpmii->fMask & MIIM_BITMAP) ? "BITMAP " : "",
		(lpmii->fMask & MIIM_FTYPE) ? "FTYPE " : ""
		);
	if(lpmii->fMask & (MIIM_FTYPE|MIIM_TYPE)) OutTrace("> fType=%#x\n", lpmii->fType);
	if(lpmii->fMask & MIIM_STATE) OutTrace("> fState=%#x\n", lpmii->fState);
	if(lpmii->fMask & MIIM_ID) OutTrace("> wID=%#x\n", lpmii->wID);
	if(lpmii->fMask & MIIM_SUBMENU) OutTrace("> hSubMenu=%#x\n", lpmii->hSubMenu);
	if(lpmii->fMask & MIIM_CHECKMARKS) {
		OutTrace("> hbmpChecked=%#x\n", lpmii->hbmpChecked);
		OutTrace("> hbmpUnchecked=%#x\n", lpmii->hbmpUnchecked);
	}
	if (((lpmii->fMask & (MIIM_TYPE|MIIM_STRING)) == (MIIM_TYPE|MIIM_STRING)) && 
		(lpmii->fType == MFT_STRING)) {
		OutTrace("> dwTypeData=\"%ls\"\n", lpmii->dwTypeData);
	}
	OutTrace("> cch=%d\n", lpmii->cch);
	if(lpmii->fMask & MIIM_BITMAP) OutTrace("> hbmpItem=%#x\n", lpmii->hbmpItem);
}
#endif
#endif // DXW_NOTRACES

static WCHAR *ConvertMIIA(LPCMENUITEMINFOA lpmii, LPMENUITEMINFOW lpmiiw)
{
	lpmiiw->cbSize = sizeof(MENUITEMINFOW);
	lpmiiw->fMask = lpmii->fMask;
	lpmiiw->fType = lpmii->fType;
	lpmiiw->fState = lpmii->fState;
	lpmiiw->wID = lpmii->wID;
	lpmiiw->hSubMenu = lpmii->hSubMenu;
	lpmiiw->hbmpChecked = lpmii->hbmpChecked;
	lpmiiw->hbmpUnchecked = lpmii->hbmpUnchecked;
	lpmiiw->dwItemData = lpmii->dwItemData;
	lpmiiw->hbmpItem = lpmii->hbmpItem;
	int size = strlen(lpmii->dwTypeData);
	WCHAR *dwTypeDataW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
	int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpmii->dwTypeData, size, dwTypeDataW, size);
	dwTypeDataW[n] = (WCHAR)0;
	lpmiiw->dwTypeData = dwTypeDataW;
	lpmiiw->cch = n;
	return dwTypeDataW;
}

BOOL WINAPI extSetMenuItemInfoA(HMENU hmenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOA lpmii)
{
	UINT res;
	ApiName("SetMenuItemInfoA");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hmenu=%#x item=%d bypos=%d\n", ApiRef, hmenu, item, fByPosition);
	if(IsTraceGDI) DumpMIIA(lpmii);
#endif // DXW_NOTRACES
	// v2.05.67: handle case where lpmii->dwTypeData == NULL. Fixes @#@ "The Princess".
	// v2.06.07: fixed for case when string is in MIIM_DATA fMask. Fixes @#@ "SugoKano"
	// v2.06.07: can't do "lpmii->fType & MFT_STRING" because MFT_STRING is 0.
	if ((dxw.dwFlags11 & CUSTOMLOCALE) &&
		((lpmii->fMask & (MIIM_TYPE|MIIM_FTYPE)) && (lpmii->fType == MFT_STRING))) {
		OutTrace("%s: ANSI -> WIDE item\n", ApiRef);
		MENUITEMINFOW miiw;
		WCHAR *w = ConvertMIIA(lpmii, &miiw);
		res = SetMenuItemInfoW(hmenu, item, fByPosition, &miiw);
		if(w) free(w);
	}
	else {
		res = (*pSetMenuItemInfoA)(hmenu, item, fByPosition, lpmii);
	}
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extInsertMenuItemA(HMENU hmenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOA lpmii)
{
	UINT res;
	ApiName("InsertMenuItemA");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hmenu=%#x item=%d bypos=%d\n", ApiRef, hmenu, item, fByPosition);
	if(IsTraceGDI) DumpMIIA(lpmii);
#endif // DXW_NOTRACES
	// v2.05.67: handle case where lpmii->dwTypeData == NULL. Fixes @#@ "The Princess".
	// v2.06.07: fixed for case when string is in MIIM_DATA fMask. Fixes @#@ "SugoKano"
	// v2.06.07: can't do "lpmii->fType & MFT_STRING" because MFT_STRING is 0.
	if ((dxw.dwFlags11 & CUSTOMLOCALE) &&
		((lpmii->fMask & (MIIM_TYPE|MIIM_FTYPE)) && (lpmii->fType == MFT_STRING))) {
		OutTrace("%s: ANSI -> WIDE item\n", ApiRef);
		MENUITEMINFOW miiw;
		WCHAR *w = ConvertMIIA(lpmii, &miiw);
		res = InsertMenuItemW(hmenu, item, fByPosition, &miiw);
		if(w) free(w);
	}
	else {
		res = (*pInsertMenuItemA)(hmenu, item, fByPosition, lpmii);
	}
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extInsertMenuA(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem)
{
	UINT res;
	ApiName("InsertMenuA");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hmenu=%#x pos=%d flags=%#x item=\"%s\"\n", ApiRef, hMenu, uPosition, uFlags, (uFlags & MF_BITMAP) ? "(BITMAP)" : ((uFlags & MF_SEPARATOR) ? "(SEP)" : lpNewItem ));
#endif // DXW_NOTRACES
	// v2.05.67: handle case where lpmii->dwTypeData == NULL. Fixes @#@ "The Princess".
	// v2.06.07: fixed for case when string is in MIIM_DATA fMask. Fixes @#@ "SugoKano"
	// v2.06.07: can't do "lpmii->fType & MFT_STRING" because MFT_STRING is 0.
	if ((dxw.dwFlags11 & CUSTOMLOCALE) &&
		!(uFlags & (MF_BITMAP | MF_SEPARATOR))) {
		int size = strlen(lpNewItem);
		LPWSTR lpNewItemW = (LPWSTR)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpNewItem, size, lpNewItemW, size);
		lpNewItemW[n] = (WCHAR)0;
		OutTrace("%s: ANSI -> WIDE item\n", ApiRef);
		res = InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, (LPCWSTR)lpNewItemW);
		free(lpNewItemW);
	}
	else {
		res = (*pInsertMenuA)(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	}
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

#ifdef TRACEMENUS
BOOL WINAPI extInsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	UINT res;
	ApiName("InsertMenuW");
#ifndef DXW_NOTRACES
	OutTrace("!!! %s: flags=%#x\n", ApiRef, uFlags);
	OutTraceGDI("%s: hmenu=%#x pos=%d flags=%#x item=\"%ls\"\n", ApiRef, hMenu, uPosition, uFlags, (uFlags & MF_BITMAP) ? L"(BITMAP)" : ((uFlags & MF_SEPARATOR) ? L"(SEP)" : lpNewItem ));
#endif // DXW_NOTRACES
		
	res = (*pInsertMenuW)(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extInsertMenuItemW(HMENU hmenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmii)
{
	UINT res;
	ApiName("InsertMenuItemW");
#ifndef DXW_NOTRACES
	OutTraceGDI("%s: hmenu=%#x item=%d bypos=%d\n", ApiRef, hmenu, item, fByPosition);
	if(IsTraceGDI) DumpMIIW(lpmii);
#endif // DXW_NOTRACES
	res = (*pInsertMenuItemW)(hmenu, item, fByPosition, lpmii);
	if(!res){
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}
#endif

BOOL WINAPI extAppendMenuA(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem)
{
	ApiName("AppendMenuA");
	BOOL res;
	OutTraceGDI("%s: hmenu=%#x flags=%#x pNewItem=%#x NewItem=%s\n", 
		ApiRef, hMenu, uFlags, uIDNewItem, lpNewItem ? lpNewItem : "(NULL)");

	if((dxw.dwFlags11 & CUSTOMLOCALE) && lpNewItem){
		int size = strlen(lpNewItem);
		WCHAR *lpNewItemW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpNewItem, size, lpNewItemW, size);
		lpNewItemW[n] = (WCHAR)0;
		res = AppendMenuW(hMenu, uFlags, uIDNewItem, lpNewItemW);
		free(lpNewItemW);
	}
	else {
		res = (*pAppendMenuA)(hMenu, uFlags, uIDNewItem, lpNewItem);
	}
	return res;
}

#ifdef TRACEMENUS
BOOL WINAPI extAppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	ApiName("AppendMenuW");
	BOOL res;
	OutTraceGDI("%s: hmenu=%#x flags=%#x pNewItem=%#x NewItem=%ls\n", 
		ApiRef, hMenu, uFlags, uIDNewItem, lpNewItem ? lpNewItem : L"(NULL)");

	res = (*pAppendMenuW)(hMenu, uFlags, uIDNewItem, lpNewItem);
	return res;
}
#endif 

HMENU WINAPI extLoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName)
{
	ApiName("LoadMenuA");
	HMENU res;
	OutTraceGDI("%s: hinst=%#x name=%s\n", ApiRef, hInstance, sTemplateNameA(lpMenuName));
	if((dxw.dwFlags11 & CUSTOMLOCALE) && ((DWORD)lpMenuName >> 16)){
		// to be verified ....
		int size = strlen(lpMenuName);
		WCHAR *lpMenuNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpMenuName, size, lpMenuNameW, size);
		lpMenuNameW[n] = (WCHAR)0;
		res = LoadMenuW(hInstance, lpMenuNameW);
		free(lpMenuNameW);
	}
	else {
		res = (*pLoadMenuA)(hInstance, lpMenuName);
	}
	OutTraceGDI("%s: hmenu=%#x\n", ApiRef, res);
	return res;
}

HMENU WINAPI extLoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
	ApiName("LoadMenuW");
	HMENU res;
	OutTraceGDI("%s: hinst=%#x name=%ls\n", ApiRef, hInstance, sTemplateNameW(lpMenuName));
	res = (*pLoadMenuW)(hInstance, lpMenuName);
	OutTraceGDI("%s: hmenu=%#x\n", ApiRef, res);
	return res;
}

HMENU WINAPI extLoadMenuIndirectA(const MENUTEMPLATEA *lpMenuTemplate)
{
	ApiName("LoadMenuIndirectA");
	HMENU res;
	OutTraceGDI("%s: template=%#x\n", ApiRef, lpMenuTemplate);
	//if((dxw.dwFlags11 & CUSTOMLOCALE) && ((DWORD)lpMenuName >> 16)){
	//	// to be verified ....
	//	int size = strlen(lpMenuName);
	//	WCHAR *lpMenuNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
	//	int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpMenuName, size, lpMenuNameW, size);
	//	lpMenuNameW[n] = (WCHAR)0;
	//	res = LoadMenuW(hInstance, lpMenuNameW);
	//	free(lpMenuNameW);
	//}
	//else {Indirect
		res = (*pLoadMenuIndirectA)(lpMenuTemplate);
	//}
	OutTraceGDI("%s: hmenu=%#x\n", ApiRef, res);
	return res;
}

HMENU WINAPI extLoadMenuIndirectW(const MENUTEMPLATEA *lpMenuTemplate)
{
	ApiName("LoadMenuIndirectW");
	HMENU res;
	OutTraceGDI("%s: template=%#x\n", ApiRef, lpMenuTemplate);
	res = (*pLoadMenuIndirectA)(lpMenuTemplate);
	OutTraceGDI("%s: hmenu=%#x\n", ApiRef, res);
	return res;
}

#ifdef TRACEREGIONS
int WINAPI extGetWindowRgnBox(HWND hWnd, LPRECT lprc)
{
	int ret;
	ApiName("GetWindowRgnBox");
	ret = (*pGetWindowRgnBox)(hWnd, lprc);
	OutTrace("%s: hwnd=%#x rc=(%d,%d)-(%d,%d) ret=%d\n",
		ApiRef, hWnd, lprc->left, lprc->top, lprc->right, lprc->bottom, ret);
	return ret;
}
#endif // TRACEREGIONS

HCURSOR WINAPI extLoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName)
{
	ApiName("LoadCursorA");
	HCURSOR res;
	OutTraceGDI("%s: hinst=%#x name=%s\n", ApiRef, hInstance, sTemplateNameA(lpCursorName));
	if(dxw.dwFlags14 & LOCKCURSORICONS){
		OutTraceGDI("%s: LOCKCURSORICONS hcur=0\n", ApiRef);
		return (HCURSOR)NULL;
	}
	res = (*pLoadCursorA)(hInstance, lpCursorName);
	OutTraceGDI("%s: hcur=%#x\n", ApiRef, res);
	return res;
}

HCURSOR WINAPI extLoadCursorW(HINSTANCE hInstance, LPCWSTR lpCursorName)
{
	ApiName("LoadCursorW");
	HCURSOR res;
	OutTraceGDI("%s: hinst=%#x name=%ls\n", ApiRef, hInstance, sTemplateNameW(lpCursorName));
	if(dxw.dwFlags14 & LOCKCURSORICONS){
		OutTraceGDI("%s: LOCKCURSORICONS hcur=0\n", ApiRef);
		return (HCURSOR)NULL;
	}
	res = (*pLoadCursorW)(hInstance, lpCursorName);
	OutTraceGDI("%s: hcur=%#x\n", ApiRef, res);
	return res;
}

#ifdef TRACECURSOR
HCURSOR WINAPI extSetCursor(HCURSOR hCursor)
{
	ApiName("SetCursor");
	HCURSOR res;
	res = (*pSetCursor)(hCursor);
	OutTraceGDI("%s: hcur=%#x res=%#x\n", ApiRef, hCursor, res);
	return res;
}

HCURSOR WINAPI extCreateCursor(HINSTANCE hInst, int xHotSpot, int yHotSpot, int nWidth, int nHeight, const VOID *pvANDPlane, const VOID *pvXORPlane)
{
	ApiName("CreateCursor");
	HCURSOR res;
	OutTraceGDI("%s: hinst=%#x hotspot(x,y)=(%d,%d) size(wxh)=(%dx%d) planes(a,x)=(%x,%#x)\n",
		ApiRef, hInst, xHotSpot, yHotSpot, nWidth, nHeight, pvANDPlane, pvXORPlane);
	res = (*pCreateCursor)(hInst, xHotSpot, yHotSpot, nWidth, nHeight, pvANDPlane, pvXORPlane);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

BOOL WINAPI extDestroyCursor(HCURSOR hCursor)
{
	ApiName("DestroyCursor");
	BOOL res;
	OutTraceGDI("%s: hcur=%#x\n", ApiRef, hCursor);
	res = (*pDestroyCursor)(hCursor);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

HICON WINAPI extCreateIconIndirect(PICONINFO piconinfo)
{
	ApiName("CreateIconIndirect");
	HICON res;
#ifndef DXW_NOTRACES
	if(IsTraceGDI) {
		OutTrace("%s\n", ApiRef);
		OutTrace("> fIcon=%#x\n", piconinfo->fIcon);
		OutTrace("> hbmColor=%#x\n", piconinfo->hbmColor);
		OutTrace("> hbmMask=%#x\n", piconinfo->hbmMask);
		OutTrace("> xHotspot=(%d,%d)\n", piconinfo->xHotspot, piconinfo->yHotspot);
	}
#endif // DXW_NOTRACES
	res = (*pCreateIconIndirect)(piconinfo);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif // TRACECURSOR

#ifdef TRACESYSCALLS
int WINAPI extLoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax)
{
	int res;
	ApiName("LoadStringA");
	OutTraceGDI("%s: hinst=%#x uid=%d cchmax=%d lpbuf=%#x\n", ApiRef, hInstance, uID, cchBufferMax, lpBuffer);

#if 0
	if(dxw.dwFlags11 & CUSTOMLOCALE){
		OutTraceLOC("%s: WIDE processing locale\n", ApiRef);
		int size = cchBufferMax;
		if(size < 8) size = 4;
		LPWSTR lpBufferW = (LPWSTR)malloc((size + 1) * sizeof(WCHAR));
		res = LoadStringW(hInstance, uID, lpBufferW, cchBufferMax);
		OutTraceLOC("%s: WIDE ret=%d\n", ApiRef, res);
		if((cchBufferMax > 0) && (res > 0)) {
			lpBufferW[res] = 0;
			//OutTraceLOC("%s: WIDE string=\"%ls\"\n", ApiRef, lpBufferW);
			res = (*pWideCharToMultiByte)(dxw.CodePage, 0, lpBufferW, res, lpBuffer, res, NULL, FALSE);
			lpBuffer[res] = 0;
			OutTraceLOC("%s: ANSI string=\"%s\"\n", ApiRef, lpBuffer);
			free(lpBufferW);
		}
	}
	else {
		res = (*pLoadStringA)(hInstance, uID, lpBuffer, cchBufferMax);
	}
#else
	res = (*pLoadStringA)(hInstance, uID, lpBuffer, cchBufferMax);
#endif

	if(lpBuffer == NULL) {
		OutTraceGDI("%s: ret(len)=%d\n", ApiRef, res);
	}
	else {
		if(res > 0) {
			OutTraceGDI("%s: ret(len)=%d buf=\"%s\"\n", ApiRef, res, lpBuffer);
		}
		else {
			OutTraceGDI("%s: ret(len)=%d\n", ApiRef, res);
		}
	}
	return res;
}
#endif // TRACESYSCALLS

typedef BOOL (WINAPI *Monitorenumproc_Type)(HMONITOR, HDC, LPRECT, LPARAM);
typedef struct {
	Monitorenumproc_Type lpfnEnum;
	LPARAM dwData;
} Monitorenumproc_arg;

BOOL WINAPI MyMonitorenumproc(HMONITOR hmon, HDC hdc, LPRECT lprcClip, LPARAM dwData)
{
	if(IsTraceGDI){
		char sRect[81];
		if(lprcClip) sprintf(sRect, "(%d,%d)-(%d,%d)", lprcClip->left, lprcClip->top, lprcClip->right, lprcClip->bottom);
		else strcpy(sRect, "NULL");
		OutTrace("> hmon=%#x hdc=%#x rect=%s data=%#x\n", hmon, hdc, sRect, dwData);
	}
	Monitorenumproc_arg *arg = (Monitorenumproc_arg *)dwData;
	RECT Clip = *lprcClip;
	if(hdc == NULL){
		switch (dxw.dwFlags4 & (SUPPORTSVGA|SUPPORTHDTV)) {
			case SUPPORTSVGA:
				OutTraceGDI("> fixed cliprect=(0,0)-(800,600)\n");
				Clip.left = 0;
				Clip.top = 0;
				Clip.right = 800;
				Clip.bottom = 600;
				break;
			case SUPPORTHDTV:
				OutTraceGDI("> fixed cliprect=(0,0)-(1600,900)\n");
				Clip.left = 0;
				Clip.top = 0;
				Clip.right = 1600;
				Clip.bottom = 900;
				break;
		}
	}
	return (arg->lpfnEnum)(hmon, hdc, &Clip, arg->dwData);
}

BOOL WINAPI extEnumDisplayMonitors(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
	ApiName("EnumDisplayMonitors");
	BOOL res;
	if(IsTraceGDI){
		char sRect[81];
		if(lprcClip) sprintf(sRect, "(%d,%d)-(%d,%d)", lprcClip->left, lprcClip->top, lprcClip->right, lprcClip->bottom);
		else strcpy(sRect, "NULL");
		OutTrace("%s: hdc=%#x rect=%s fenum=%#x data=%#x\n", ApiRef, hdc, sRect, lpfnEnum, dwData);
	}
	if(dxw.dwFlags13 & FIXASPECTRATIO){
		Monitorenumproc_arg arg;
		arg.lpfnEnum = lpfnEnum;
		arg.dwData = dwData;
		res = (*pEnumDisplayMonitors)(hdc, lprcClip, MyMonitorenumproc, (LPARAM)&arg);
	}
	else {
		res = (*pEnumDisplayMonitors)(hdc, lprcClip, lpfnEnum, dwData);
	}
#ifndef DXW_NOTRACES 
	if(!res){
		DWORD err = GetLastError();
		if(err != ERROR_INVALID_PARAMETER) {
			OutErrorGDI("%s: ERROR err=%d\n", ApiRef, err);
		}
	}
#endif // DXW_NOTRACES
	return res;
}

HCURSOR WINAPI extLoadCursorFromFileA(LPCSTR lpFileName)
{
	ApiName("LoadCursorFromFileA");
	HCURSOR res;
	if((DWORD)lpFileName & 0xFFFF0000){
		OutTraceGDI("%s: file=\"%s\"\n", ApiRef, lpFileName);
	}
	else {
		OutTraceGDI("%s: file=%#x\n", ApiRef, lpFileName);
	}
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		lpFileName = dxwTranslatePathA(lpFileName, NULL);
	}
	res = (*pLoadCursorFromFileA)(lpFileName);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

HCURSOR WINAPI extLoadCursorFromFileW(LPCWSTR lpFileName)
{
	ApiName("LoadCursorFromFileW");
	HCURSOR res;
	if((DWORD)lpFileName & 0xFFFF0000){
		OutTraceGDI("%s: file=\"%ls\"\n", ApiRef, lpFileName);
	}
	else {
		OutTraceGDI("%s: file=%#x\n", ApiRef, lpFileName);
	}
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		lpFileName = dxwTranslatePathW(lpFileName, NULL);
	}
	res = (*pLoadCursorFromFileW)(lpFileName);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

DWORD WINAPI extGetSysColor(int nIndex)
{
	ApiName("GetSysColor");
	DWORD res;
	OutTraceGDI("%s: index=%d\n", ApiRef, nIndex);
	res = (*pGetSysColor)(nIndex);
	OutTraceGDI("%s: res=%#x\n", ApiRef, res);
	return res;
}

int WINAPI extGetKeyboardType(int nTypeFlag)
{
	int ret;
	ApiName("GetKeyboardType");
	OutTraceGDI("%s: flag=%#x(%s)\n", ApiRef, nTypeFlag, 
		nTypeFlag == 0 ? "TYPE" : (nTypeFlag == 1 ? "SUBTYPE" : "NKEYS"));
	ret = (*pGetKeyboardType)(nTypeFlag);
#ifndef DXW_NOTRACES
	if(nTypeFlag == 0){
		char *sType;
		switch(ret){
			case 0x4: sType = "Enhanced 101/102-key keyboard"; break;
			case 0x7: sType = "Japanese keyboard"; break;
			case 0x8: sType = "Korean keyboard"; break;
			case 0x51: sType = "Unknown type or HID keyboard"; break;
			default: sType = "???"; break;
		}
		OutTraceGDI("%s: ret=%#x(%s)\n", ApiRef, ret, sType);
	}
	else {
		OutTraceGDI("%s: ret=%#x\n", ApiRef, ret);
	}
#endif // DXW_NOTRACES
	if(dxw.dwFlags14 & FIXKEYBOARDTYPE){
		if (nTypeFlag == 0) {
			int oldRet = ret;
			switch(dxw.Locale){
				case 17:
				case 1041: ret = 0x7; break;
				case 18:
				case 1042: ret = 0x8; break;
				default: ret = 0x4; break; 
			}
			if(oldRet != ret) {
				OutTraceGDI("%s: FIXED ret=%#x\n", ApiRef, ret);
			}
		}
	}

	return ret;
}

int WINAPI extGetKeyNameTextA(LONG lParam, LPSTR lpString, int cchSize)
{
	int ret;
	ApiName("GetKeyNameTextA");
	OutTraceGDI("%s: lparam=%#x size=%d\n", ApiRef, lParam, cchSize);
	ret = (*pGetKeyNameTextA)(lParam, lpString, cchSize);
	if(ret || (0==GetLastError())){
		OutTraceGDI("%s: ret=%#x string=\"%s\"\n", ApiRef, ret, lpString);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

int WINAPI extGetKeyNameTextW(LONG lParam, LPWSTR lpString, int cchSize)
{
	int ret;
	ApiName("GetKeyNameTextW");
	OutTraceGDI("%s: lparam=%#x size=%d\n", ApiRef, lParam, cchSize);
	ret = (*pGetKeyNameTextW)(lParam, lpString, cchSize);
	if(ret || (0==GetLastError())){
		OutTraceGDI("%s: ret=%#x string=\"%ls\"\n", ApiRef, ret, lpString);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extEnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable)
{
	BOOL ret;
	ApiName("EnableMenuItem");

	OutTraceGDI("%s: hmenu=%#x item=%d enable=%d\n", ApiRef, hMenu, uIDEnableItem, uEnable);
	if((dxw.dwFlags14 & LOCKSYSTEMMENU) && (hMenu == GetSystemMenu(dxw.GethWnd(), FALSE))){
		OutTraceGDI("%s: LOCKSYSTEMMENU hMenu=%#x\n", ApiRef, hMenu);
		return TRUE;
	}
	ret = (*pEnableMenuItem)(hMenu, uIDEnableItem, uEnable);
	return ret;
}

HWND WINAPI extCreateMDIWindowA(LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HINSTANCE hInstance, LPARAM lParam)
{
	ApiName("CreateMDIWindowA");
	HWND res;

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char xString[20], yString[20], wString[20], hString[20];
		char sStyle[256];
		if (X==CW_USEDEFAULT) strcpy(xString,"CW_USEDEFAULT"); 
		else sprintf(xString,"%d", X);
		if (Y==CW_USEDEFAULT) strcpy(yString,"CW_USEDEFAULT"); 
		else sprintf(yString,"%d", Y);
		if (nWidth==CW_USEDEFAULT) strcpy(wString,"CW_USEDEFAULT"); 
		else sprintf(wString,"%d", nWidth);
		if (nHeight==CW_USEDEFAULT) strcpy(hString,"CW_USEDEFAULT"); 
		else sprintf(hString,"%d", nHeight);
		OutTrace("%s: class=\"%s\" wname=\"%s\" pos=(%s,%s) size=(%s,%s) Style=%#x(%s) hWndParent=%#x%s hInstance=%#x lParam=%#x\n",
			ApiRef, ClassToStr(lpClassName), lpWindowName, xString, yString, wString, hString, 
			dwStyle, ExplainStyle(dwStyle, sStyle, 256), 
			hWndParent, hWndParent==HWND_MESSAGE?"(HWND_MESSAGE)":"", 
			hInstance, lParam);
	}
	OutDebugGDI("%s: DEBUG fullscreen=%#x mainwin=%#x screen=(%d,%d)\n", 
		ApiRef, dxw.IsFullScreen(), dxw.GethWnd(), dxw.GetScreenWidth(), dxw.GetScreenHeight());
#endif

	res = (*pCreateMDIWindowA)(lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hInstance, lParam);
	if(res){
		OutTraceGDI("%s: created hwnd ret=%#x\n", ApiRef, res);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

HWND WINAPI extCreateMDIWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HINSTANCE hInstance, LPARAM lParam)
{
	ApiName("CreateMDIWindowW");
	HWND res;

#ifndef DXW_NOTRACES
	if(IsTraceGDI){
		char xString[20], yString[20], wString[20], hString[20];
		char sStyle[256];
		if (X==CW_USEDEFAULT) strcpy(xString,"CW_USEDEFAULT"); 
		else sprintf(xString,"%d", X);
		if (Y==CW_USEDEFAULT) strcpy(yString,"CW_USEDEFAULT"); 
		else sprintf(yString,"%d", Y);
		if (nWidth==CW_USEDEFAULT) strcpy(wString,"CW_USEDEFAULT"); 
		else sprintf(wString,"%d", nWidth);
		if (nHeight==CW_USEDEFAULT) strcpy(hString,"CW_USEDEFAULT"); 
		else sprintf(hString,"%d", nHeight);
		OutTrace("%s: class=\"%ls\" wname=\"%ls\" pos=(%s,%s) size=(%s,%s) Style=%#x(%s)%s hWndParent=%#x%s hInstance=%#x lParam=%#x\n",
			ApiRef, ClassToWStr(lpClassName), lpWindowName, xString, yString, wString, hString, 
			dwStyle, ExplainStyle(dwStyle, sStyle, 256), dwStyle & MDIS_ALLCHILDSTYLES ? "+MDIS_ALLCHILDSTYLES" : "",
			hWndParent, hWndParent==HWND_MESSAGE?"(HWND_MESSAGE)":"", 
			hInstance, lParam);
	}
	OutDebugGDI("%s: DEBUG fullscreen=%#x mainwin=%#x screen=(%d,%d)\n", 
		ApiRef, dxw.IsFullScreen(), dxw.GethWnd(), dxw.GetScreenWidth(), dxw.GetScreenHeight());
#endif

	res = (*pCreateMDIWindowW)(lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hInstance, lParam);
	if(res){
		OutTraceGDI("%s: created hwnd ret=%#x\n", ApiRef, res);
	}
	else {
		OutErrorGDI("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

#ifdef TRACEALL
BOOL WINAPI extEndDialog(HWND hwnd, INT_PTR nResult)
{
	ApiName("EndDialog");
	BOOL res;
	OutTraceGDI("%s: hwnd=%#x nResult=%#x\n", ApiRef, hwnd, nResult);
	//return TRUE;
	res = (*pEndDialog)(hwnd, nResult);
	OutTraceGDI("%s: hwnd=%#x nResult=%#x ret=%#x\n", ApiRef, hwnd, nResult, res);
	// commented: nResult can be -1 which is a bad pointer.
	//OutTraceGDI("%s: hwnd=%#x nResult=%d ret=%#x\n", ApiRef, hwnd, *(DWORD *)nResult, res);
	return res;
}
#endif // TRACEALL

BOOL WINAPI extSetSystemCursor(HCURSOR hcur, DWORD id)
{
	BOOL res;
	ApiName("SetSystemCursor");

	OutTraceGDI("%s: hcur=%#x id=%#x\n", ApiRef, hcur, id);
	
	if(dxw.dwFlags16 & LOCKSYSCURSORICONS) {
		OutTraceGDI("%s: BYPASS hcur=%#x id=%#x\n", ApiRef, hcur, id);
		return TRUE;
	}

	res = (*pSetSystemCursor)(hcur, id);
	return res;
}

typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
} WINHELP;
#if 0
#define HELP_CONTEXT      0x0001L  /* Display topic in ulTopic */
#define HELP_QUIT         0x0002L  /* Terminate help */
#define HELP_INDEX        0x0003L  /* Display index */
#define HELP_CONTENTS     0x0003L
#define HELP_HELPONHELP   0x0004L  /* Display help on using help */
#define HELP_SETINDEX     0x0005L  /* Set current Index for multi index help */
#define HELP_SETCONTENTS  0x0005L
#define HELP_CONTEXTPOPUP 0x0008L
#define HELP_FORCEFILE    0x0009L
#define HELP_KEY          0x0101L  /* Display topic for keyword in offabData */
#define HELP_COMMAND      0x0102L
#define HELP_PARTIALKEY   0x0105L
#define HELP_MULTIKEY     0x0201L
#define HELP_SETWINPOS    0x0203L
#define HELP_CONTEXTMENU  0x000a
#define HELP_FINDER       0x000b
#define HELP_WM_HELP      0x000c
#define HELP_SETPOPUP_POS 0x000d
#endif

#if 0
BOOL WINAPI extWinHelpA(HWND hWndMain, LPCSTR lpszHelp, UINT uCommand, ULONG_PTR dwData)
{
	BOOL res;
	ApiName("WinHelpA");

	OutTraceGDI("%s: hWndMain=%#x lpszHelp=%s uCommand=%d dwData=%#x\n", ApiRef, hWndMain, lpszHelp, uCommand, dwData);
	
	if(dxw.dwFlags17 & EMULATEWINHELP) {
		switch(uCommand){
			case HELP_QUIT:
				break;
			case HELP_CONTEXT:
			case HELP_CONTENTS: // ??? legacy
			case HELP_HELPONHELP:
			case HELP_KEY:
			case HELP_FINDER: // used by "The Adventures of Lomax"
				OutTraceGDI("%s: exec legacy helper\n", ApiRef);
				if(uCommand == HELP_HELPONHELP) lpszHelp = "Winhlp32.hlp";
				char cmdLine[256+1];
#if 0
				sprintf_s(cmdLine, 256, "c:\\Windows\\WinHelp\\winhlp32.exe %s", lpszHelp);
#else
				_snprintf(cmdLine, 256, "%s\\winhlp32.exe %s", GetDxWndPath(), lpszHelp);
#endif
				WinExec(cmdLine, SW_SHOWNORMAL);
				break;
			case HELP_COMMAND:
			case HELP_CONTEXTMENU:
			case HELP_CONTEXTPOPUP:
			case HELP_WM_HELP:
			case HELP_SETPOPUP_POS:
			default:
				// do nothing now ...
				break;
		}
		return TRUE;
	}

	res = (*pWinHelpA)(hWndMain, lpszHelp, uCommand, dwData);
	return res;
}

BOOL WINAPI extWinHelpW(HWND hWndMain, LPCWSTR lpszHelp, UINT uCommand, ULONG_PTR dwData)
{
	BOOL res;
	ApiName("WinHelpW");

	OutTraceGDI("%s: hWndMain=%#x lpszHelp=%ls uCommand=%d dwData=%#x\n", ApiRef, hWndMain, lpszHelp, uCommand, dwData);
	
	if(dxw.dwFlags17 & EMULATEWINHELP) {
		switch(uCommand){
			case HELP_QUIT:
				break;
			case HELP_CONTEXT:
			case HELP_CONTENTS: // ??? legacy
			case HELP_HELPONHELP:
			case HELP_KEY:
				OutTraceGDI("%s: exec legacy helper\n", ApiRef);
				if(uCommand == HELP_HELPONHELP) lpszHelp = L"Winhlp32.hlp";
				char cmdLine[256+1];
				sprintf_s(cmdLine, 256, "c:\\Windows\\WinHelp\\winhlp32.exe %ls", lpszHelp);
				WinExec(cmdLine, SW_SHOWNORMAL);
				break;
			case HELP_COMMAND:
			case HELP_CONTEXTMENU:
			case HELP_CONTEXTPOPUP:
			default:
				// do nothing now ...
				break;
		}
		return TRUE;
	}

	res = (*pWinHelpW)(hWndMain, lpszHelp, uCommand, dwData);
	return res;
}
#endif

BOOL WINAPI extDrawFrameControl(HDC hdc, LPRECT lprect, UINT icType, UINT icState)
{
	BOOL res;
	ApiName("DrawFrameControl");
	RECT rect;

	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d) type=%d state=%d\n", 
		ApiRef, hdc, lprect->left, lprect->top, lprect->right, lprect->bottom, icType, icState);

	switch(dxw.GDIEmulationMode){
		case GDIMODE_STRETCHED: 
			if(lprect){
				rect = *lprect;
				lprect = &rect;
			}
			dxw.MapClient(lprect);
			res = (*pDrawFrameControl)(hdc, lprect, icType, icState);
			return res;
			break;
	}
	res = (*pDrawFrameControl)(hdc, lprect, icType, icState);
	return res;
}

BOOL WINAPI extDrawFocusRect(HDC hdc, const RECT *lprect)
{
	BOOL res;
	ApiName("DrawFocusRect");
	RECT rect;

	OutTraceGDI("%s: hdc=%#x rect=(%d,%d)-(%d,%d)\n", 
		ApiRef, hdc, lprect->left, lprect->top, lprect->right, lprect->bottom);

	switch(dxw.GDIEmulationMode){
		case GDIMODE_STRETCHED: 
			if(lprect){
				rect = *lprect;
				lprect = &rect;
			}
			dxw.MapClient((LPRECT)lprect);
			res = (*pDrawFocusRect)(hdc, lprect);
			return res;
			break;
	}
	res = (*pDrawFocusRect)(hdc, lprect);
	return res;
}

#ifdef TRACEPROPS

HANDLE WINAPI extGetPropA(HWND hWnd, LPCSTR lpString)
{
	HANDLE res;
	ApiName("GetPropA");
	if((DWORD)lpString & 0xFFFF0000){
		OutTraceSYS("%s: hwnd=%#x str=\"%s\"\n", ApiRef, hWnd, lpString);
	}
	else {
		OutTraceSYS("%s: hwnd=%#x atom=%#x\n", ApiRef, hWnd, lpString);
	}
	res = (*pGetPropA)(hWnd, lpString);
	if(res){
		OutTraceSYS("%s: ret=%#x\n", ApiRef, res);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

HANDLE WINAPI extGetPropW(HWND hWnd, LPCWSTR lpString)
{
	HANDLE res;
	ApiName("GetPropW");
	if((DWORD)lpString & 0xFFFF0000){
		OutTraceSYS("%s: hwnd=%#x str=\"%ls\"\n", ApiRef, hWnd, lpString);
	}
	else {
		OutTraceSYS("%s: hwnd=%#x atom=%#x\n", ApiRef, hWnd, lpString);
	}
	res = (*pGetPropW)(hWnd, lpString);
	if(res){
		OutTraceSYS("%s: ret=%#x\n", ApiRef, res);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extSetPropA(HWND hWnd, LPCSTR lpString, HANDLE hData)
{
	BOOL res;
	ApiName("SetPropA");
	if((DWORD)lpString & 0xFFFF0000){
		OutTraceSYS("%s: hwnd=%#x str=\"%s\" data=%#x\n", ApiRef, hWnd, lpString, hData);
	}
	else {
		OutTraceSYS("%s: hwnd=%#x atom=%#x data=%#x\n", ApiRef, hWnd, lpString, hData);
	}
	res = (*pSetPropA)(hWnd, lpString, hData);
	if(!res){
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extSetPropW(HWND hWnd, LPCWSTR lpString, HANDLE hData)
{
	BOOL res;
	ApiName("SetPropW");
	if((DWORD)lpString & 0xFFFF0000){
		OutTraceSYS("%s: hwnd=%#x str=\"%ls\" data=%#x\n", ApiRef, hWnd, lpString, hData);
	}
	else {
		OutTraceSYS("%s: hwnd=%#x atom=%#x data=%#x\n", ApiRef, hWnd, lpString, hData);
	}
	res = (*pSetPropW)(hWnd, lpString, hData);
	if(!res){
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

#endif // TRACEPROPS

BOOL WINAPI extSetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString)
{
	int ret;
	ApiName("SetDlgItemTextA");

	OutTraceGDI("%s: hwnd=%#x item=%d Text=\"%s\"\n", ApiRef, hDlg, nIDDlgItem, lpString);

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextA(ApiRef, 0, 0, lpString, 0);

	if(dxw.dwFlags11 & CUSTOMLOCALE) {
		LPWSTR wstr = NULL;
		int n;
		int size = lstrlenA(lpString);
		if(size) {
			wstr = (LPWSTR)malloc((size+1)<<1);
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpString, size, wstr, size);
			wstr[n] = (WCHAR)0; // add terminator !!
			_if(n==0) OutTrace("!!! err=%d @%d\n", GetLastError(), __LINE__);
			if(!pSetDlgItemTextW) pSetDlgItemTextW = SetDlgItemTextW;
			ret = pSetDlgItemTextW(hDlg, nIDDlgItem, wstr);
			free(wstr);
			return ret;
		}
	}

	ret=(*pSetDlgItemTextA)(hDlg, nIDDlgItem, lpString);
	return ret;
}

BOOL WINAPI extSetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString)
{
	int ret;
	ApiName("SetDlgItemTextW");

	OutTraceGDI("%s: hwnd=%#x item=%d Text=\"%ls\"\n", ApiRef, hDlg, nIDDlgItem, lpString);

	if(dxw.dwFlags14 & DUMPTEXT) DumpTextW(ApiRef, 0, 0, lpString, 0);

	ret=(*pSetDlgItemTextW)(hDlg, nIDDlgItem, lpString);
	return ret;
}

#ifdef TRACEALL
BOOL WINAPI extTrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack)
{
	BOOL res;
	ApiName("TrackMouseEvent");
#ifndef DXW_NOTRACES
	if(IsTraceSYS){
		OutTrace("%s: lpEventTrack=%#x\n", ApiRef, lpEventTrack);
		OutTrace("> size=%d\n", lpEventTrack->cbSize);
		OutTrace("> flags=%#x\n", lpEventTrack->dwFlags);
		OutTrace("> hovertime=%d\n", lpEventTrack->dwHoverTime);
		OutTrace("> hwndTrack=%#x\n", lpEventTrack->hwndTrack);
	}
#endif // DXW_NOTRACES
	res = (*pTrackMouseEvent)(lpEventTrack);
	if(res){
		OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}
#endif // TRACEALL

BOOL WINAPI extWINNLSEnableIME(HWND hwnd, BOOL enable)
{
	ApiName("WINNLSEnableIME");
	OutTrace("%s: hwnd=%#x enable=%#x DISABLED\n", ApiRef, hwnd, enable);
	return TRUE;
}

HWND gProgramManagerHWND = 0;
HWND gIconTrayHWND = 0;

HWND WINAPI extFindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName)
{
	ApiName("FindWindowA");
	HWND res;
	res = (*pFindWindowA)(lpClassName, lpWindowName);
	OutTraceSYS("%s: lpClassName=%s lpWindowName=%s hwnd=%#x\n",
		ApiRef, lpClassName, lpWindowName, res);

	if(dxw.dwFlags8 & WININSULATION){
		if(lpClassName && lpWindowName){
			if(!strcmp(lpClassName, "Progman") && !strcmp(lpWindowName, "Program Manager")){
				OutTrace("%s: Program Manager hwnd=%#x\n", ApiRef, res);
				gProgramManagerHWND = res;
			}
		}
		if(lpClassName && !strcmp(lpClassName, "Shell_TrayWnd")){
			OutTrace("%s: Icon tray hwnd=%#x\n", ApiRef, res);
			gIconTrayHWND = res;
		}
	}
	return res;
}

HWND WINAPI extFindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
	ApiName("FindWindowW");
	HWND res;
	res = (*pFindWindowW)(lpClassName, lpWindowName);
	OutTraceSYS("%s: lpClassName=%ls lpWindowName=%ls hwnd=%#x\n",
		ApiRef, lpClassName, lpWindowName, res);
	if(dxw.dwFlags8 & WININSULATION){
		if(lpClassName && lpWindowName){
			if(!wcscmp(lpClassName, L"Progman") && !wcscmp(lpWindowName, L"Program Manager")){
				OutTrace("%s: Program Manager hwnd=%#x\n", ApiRef, res);
				gProgramManagerHWND = res;
			}
		}
		if(lpClassName && !wcscmp(lpClassName, L"Shell_TrayWnd")){
			OutTrace("%s: Icon tray hwnd=%#x\n", ApiRef, res);
			gIconTrayHWND = res;
		}
	}
	return res;
}

BOOL WINAPI extEnumChildWindows(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam)
{
	ApiName("EnumChildWindows");
	BOOL res;

	OutTraceSYS("%s: hWndParent=%#x lpEnumFunc=%#x lParam=%#x\n", ApiRef, hWndParent, lpEnumFunc, lParam);

	if((dxw.dwFlags8 & WININSULATION) && hWndParent){
		if(hWndParent == gProgramManagerHWND){
			OutTrace("%s: INSULATEWIN pretend there are no ProgMan childs\n", ApiRef);
			return TRUE;
		}
		if(hWndParent == gIconTrayHWND){
			OutTrace("%s: INSULATEWIN pretend there are no IconTray childs\n", ApiRef);
			return TRUE;
		}
	}

	res = (*pEnumChildWindows)(hWndParent, lpEnumFunc, lParam);
	return res;
}

LRESULT WINAPI extSendDlgItemMessageA(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	ApiName("SendDlgItemMessageA");
	LRESULT ret;
	OutTraceGDI("%s: hDlg=%#x item=%d msg=%#x(%s) w/lparam=%#x/%#x\n", 
		ApiRef, hDlg, nIDDlgItem, Msg, ExplainWinMessage(Msg), wParam, lParam);
	if((dxw.dwFlags11 & CUSTOMLOCALE) && (Msg == CB_ADDSTRING)){
		char *lpText = (char *)lParam;
		int size = strlen((const char *)lpText);
		WCHAR *lpTextW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpText, size, lpTextW, size);
		lpTextW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE text=\"%ls\"\n", ApiRef, lpTextW);
		ret = SendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, (LPARAM)lpTextW);
		free(lpTextW);
	}
	else {
		ret = (*pSendDlgItemMessageA)(hDlg, nIDDlgItem, Msg, wParam, lParam);
	}
	return ret;
}

HWND WINAPI extSetParent(HWND hWndChild, HWND hWndNewParent)
{
	ApiName("SetParent");
	HWND ret;
	OutTrace("%s: child=%#x parent=%#x\n", ApiRef, hWndChild, hWndNewParent);
	if(dxw.Windowize){
		if(dxw.IsDesktop(hWndNewParent)){
			OutTrace("%s: remap newParent %#x->%#x\n", ApiRef, hWndNewParent, dxw.GethWnd());
			hWndNewParent = dxw.GethWnd();
		}
	}
		
	ret = (*pSetParent)(hWndChild, hWndNewParent);
	OutTrace("%s: ret(hOldParent)=%#x\n", ApiRef, ret);
	return ret;
}

extern void mySetPalette(int, int, LPPALETTEENTRY); 

UINT WINAPI extUserRealizePalette(HDC hdc) 
{
	UINT ret;
	ApiName("UserRealizePalette");

	OutTraceGDI("%s: hdc=%#x\n", ApiRef, hdc);

	if(dxw.IsEmulated){
		PALETTEENTRY PalEntries[256];
		UINT cEntries;
		HPALETTE hPal;
		if(bFlippedDC) hdc = hFlippedDC;
		hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
		hPal = (*pGDISelectPalette)(hdc, hPal, FALSE);
		(*pGDISelectPalette)(hdc, hPal, FALSE);
		cEntries=(*pGetPaletteEntries)(hPal, 0, 256, PalEntries);
		OutTraceDW("%s: GetPaletteEntries on hdc=%#x ret=%d\n", ApiRef, hdc, cEntries);
		mySetPalette(0, 256, PalEntries); 
		if(IsDebugDW && cEntries) dxw.DumpPalette(cEntries, PalEntries);  
		ret=cEntries;
		(*pInvalidateRect)(dxw.GethWnd(), 0, FALSE);
	}
	else
		ret=(*pUserRealizePalette)(hdc);

	OutTraceGDI("%s: hdc=%#x nEntries=%d\n", ApiRef, hdc, ret);

	return ret;
}

BOOL WINAPI extUpdateLayeredWindow(HWND hWnd, HDC hdcDst, POINT *pptDst, SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags)
{
	BOOL ret;
	ApiName("UpdateLayeredWindow");

	OutTraceSYS("%s: hwnd=%#x hdcdest=%#x hdcsrc=%#x flags=%#x\n", ApiRef, hWnd, hdcDst, hdcSrc, dwFlags);

	if(dxw.dwFlags19 & DISABLELAYEREDWIN){
		OutTraceSYS("%s: BYPASS\n", ApiRef);
		return TRUE;
	}

	ret = (*pUpdateLayeredWindow)(hWnd, hdcDst, pptDst, psize, hdcSrc, pptSrc, crKey, pblend, dwFlags);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extSetLayeredWindowAttributes(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
	BOOL ret;
	ApiName("SetLayeredWindowAttributes");

	OutTrace("%s: hwnd=%#x colorref=%#x flags=%#x\n", ApiRef, hWnd, crKey, dwFlags);

	if(dxw.dwFlags19 & DISABLELAYEREDWIN){
		OutTraceSYS("%s: BYPASS\n", ApiRef);
		return TRUE;
	}

	ret = (*pSetLayeredWindowAttributes)(hWnd, crKey, bAlpha, dwFlags);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extUpdateLayeredWindowIndirect(HWND hWnd, const UPDATELAYEREDWINDOWINFO *pULWInfo)
{
	BOOL ret;
	ApiName("UpdateLayeredWindowIndirect");

	OutTrace("%s: hwnd=%#x\n", ApiRef, hWnd);

	if(dxw.dwFlags19 & DISABLELAYEREDWIN){
		OutTraceSYS("%s: BYPASS\n", ApiRef);
		return TRUE;
	}

	ret = (*pUpdateLayeredWindowIndirect)(hWnd, pULWInfo);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

#ifdef TRACEALL
HWND WINAPI extGetCapture()
{
	HWND ret;
	ApiName("GetCapture");

	ret = (*pGetCapture)();
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}
#endif

#ifdef TRACEALL
BOOL WINAPI extFlashWindow(HWND hwnd, BOOL invert)
{
	BOOL ret;
	ApiName("FlashWindow");

	ret = (*pFlashWindow)(hwnd, invert);
	OutTraceSYS("%s: hwnd=%#x bool=%#x ret=%#x\n", ApiRef, hwnd, invert, ret);
	return ret;
}
#endif
