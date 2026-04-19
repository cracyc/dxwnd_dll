#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE 1

#define _MODULE "dxwnd"

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhelper.h"
#include "mciplayer.h"
#include "hddraw.h"
#include <dsound.h>
#include <Dbt.h>

extern void dxwFixMinMaxInfo(char *, HWND, LPARAM);
extern void renewBackBufferClipper(HWND);
extern void dxwFixWindowPos(char *, HWND, LPARAM);
extern void dx_ToggleMouseSpeed(BOOL);
extern void dx_ToggleSound(BOOL);
extern void SDLScreenRefresh();
extern void SetIdlePriority(BOOL);
extern BOOL JoyProcessMouseWheelMessage(WPARAM, LPARAM);
extern void RecoverScreenMode();
extern void gShowHideTaskBar(BOOL);
extern void HandleHotKeys(HWND, UINT, LPARAM, WPARAM);
extern LPRECT lpClipRegion;

static void dx_UpdatePositionLock(HWND hwnd)
{
	ApiName("dx_UpdatePositionLock");
	RECT rect;
	POINT p={0,0};
	(*pGetClientRect)(hwnd,&rect);
	(*pClientToScreen)(hwnd,&p);
	dxw.dwFlags1 |= LOCKWINPOS;
	OutTraceDW("%s: Toggle position lock ON\n", ApiRef);
	dxw.InitWindowPos(p.x, p.y, rect.right-rect.left, rect.bottom-rect.top);
}


// returns:
// FALSE	: skip the window procedure and return *pResult
// TRUE		: run the window procedure 

BOOL FixWindowProc(ApiArg, HWND hwnd, UINT Msg, WPARAM *wpParam, LPARAM *lpParam, HRESULT *pResult)
{
	LPARAM lParam;
	WPARAM wParam;
	static BOOL EscSemaphore = FALSE;
	static BOOL IsToBeLocked;
	static int LastTimeShift;
	static BOOL DoOnce = TRUE;
	POINT prev, curr;
	RECT rect;

	if(DoOnce){
		DoOnce=FALSE;
		IsToBeLocked=(dxw.dwFlags1 & LOCKWINPOS);
		LastTimeShift=dxw.TimeShift;
	}

	lParam = *lpParam;
	wParam = *wpParam;
	//OutTraceW("%s: hwnd=%#x msg=[%#x]%s(%#x,%#x)\n",
	//	ApiRef, hwnd, Msg, ExplainWinMessage(Msg), wParam, lParam);

	if(LastTimeShift != dxw.TimeShift){
		extern void SetVSyncDelays(int, LPDIRECTDRAW);
		extern LPDIRECTDRAW lpPrimaryDD;
		extern int iPrimarySurfaceVersion;
		if(dxw.dwFlags4 & STRETCHTIMERS) dxw.RenewTimers();
		if(lpPrimaryDD) SetVSyncDelays(iPrimarySurfaceVersion, lpPrimaryDD);
		LastTimeShift=dxw.TimeShift;
	}

	switch(Msg){
	case WM_NCDESTROY:
	case WM_DESTROY:
		if(dxw.dwFlags10 & FORCED3DGAMMARAMP) (*pGDISetDeviceGammaRamp)((*pGDIGetDC)(NULL), dxw.pInitialRamp);
		break;

	case WM_GETMINMAXINFO: 
		if(dxw.dwFlags1 & LOCKWINPOS) dxwFixMinMaxInfo(ApiRef, hwnd, lParam);
		break;

	case WM_NCCALCSIZE:
	case WM_NCPAINT:
		// v2.02.30: don't alter child and other windows....
		// v2.04.09: comment inconsistent with code. Added '!' to if expression ....
		if(!((dxw.dwFlags1 & LOCKWINPOS) && (hwnd == dxw.GethWnd()) && dxw.IsFullScreen())){ 
			OutTraceDW("%s: %s wparam=%#x\n", ApiRef, ExplainWinMessage(Msg), wParam);
			*pResult = (*pDefWindowProcA)(hwnd, Msg, wParam, lParam);
			return FALSE;
		}
		break;

	case WM_NCCREATE:
		if(dxw.dwFlags2 & SUPPRESSIME) dxw.SuppressIMEWindow();
		break;
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_CONTROL:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
	case WM_IME_CHAR:
	case WM_IME_REQUEST:
	case WM_IME_KEYDOWN:
	case WM_IME_KEYUP:
		if(dxw.dwFlags2 & SUPPRESSIME){
			OutTraceDW("%s[%#x]: SUPPRESS IME WinMsg=[%#x]%s(%#x,%#x)\n", ApiRef, hwnd, Msg, ExplainWinMessage(Msg), wParam, lParam);
			*pResult = 0;
			return FALSE;
		}
		break;

	case WM_NCHITTEST:
		if((dxw.dwFlags2 & FIXNCHITTEST) && (dxw.MustFixMouse)){ // mouse processing 
			POINT cursor;
			*pResult=(*pDefWindowProcA)(hwnd, Msg, wParam, lParam);
			if (*pResult==HTCLIENT) {
				cursor.x=LOWORD(lParam);
				cursor.y=HIWORD(lParam);
				dxw.FixNCHITCursorPos(&cursor);
				*lpParam = MAKELPARAM(cursor.x, cursor.y); 
				OutTraceW("%s[%#x]: fixed WM_NCHITTEST pt=(%d,%d)\n", ApiRef, hwnd, cursor.x, cursor.y);
				GetHookInfo()->WinProcX=(short)cursor.x;
				GetHookInfo()->WinProcY=(short)cursor.y;
			}
			else {
				// v2.06.05: when TRIMMOUSEPOSITION always pretend that the cursor is within the client area
				if(dxw.dwFlags1 & TRIMMOUSEPOSITION) *pResult = HTCLIENT;
#ifdef PROVENTOBEUSEFUL
				if((dxw.dwFlags20 & CENTERONEXIT) && dxw.MustFixMouse){
					cursor.x = dxw.GetScreenWidth() / 2;
					cursor.y = dxw.GetScreenHeight() / 2;
					*lpParam = MAKELPARAM(cursor.x, cursor.y); 
					OutTraceW("%s[%#x]: fixed WM_NCHITTEST pt=(%d,%d)\n", ApiRef, hwnd, cursor.x, cursor.y);
					GetHookInfo()->WinProcX=(short)cursor.x;
					GetHookInfo()->WinProcY=(short)cursor.y;
				}
#endif
			}
			OutTraceW("%s: NCHITTEST result=%#x(%s)\n", ApiRef, *pResult, ExplainNChitTest(*pResult));
			return FALSE;
		}
		break;

	case WM_ERASEBKGND:
		// v2.03.97: fix for Adrenix lost backgrounds, thanks to Riitaoja hunt!
		if(dxw.Windowize && dxw.IsRealDesktop(hwnd)){ 
			OutTraceDW("%s: WM_ERASEBKGND(%#x,%#x) - suppressed\n", ApiRef, wParam, lParam);
			*pResult = 1;
			return FALSE;
		}
		break;

	case WM_DISPLAYCHANGE:
		// v2.06.13 fix: consider dxw.isColorEmulatedMode
		if (dxw.isScaled && (dxw.dwFlags1 & LOCKWINPOS) && dxw.IsFullScreen()){
			OutTraceDW("%s: prevent WM_DISPLAYCHANGE depth=%d size=(%d,%d)\n",
				ApiRef, wParam, LOWORD(lParam), HIWORD(lParam));
			// v2.02.43: unless emulation is set, lock the screen resolution only, but not the color depth!
			if(dxw.IsEmulated) *pResult = 0;
			// let rparam (color depth) change, but override lparam (screen width & height.)
			*lpParam = MAKELPARAM((LONG)dxw.GetScreenWidth(), (LONG)dxw.GetScreenHeight());
		}
		break;

	case WM_STYLECHANGING:
	case WM_STYLECHANGED:
		if(dxw.dwFlags1 & LOCKWINSTYLE) {
			OutTraceDW("%s: %s - suppressed\n", ApiRef, Msg==WM_STYLECHANGING ? "WM_STYLECHANGING" : "WM_STYLECHANGED");
			*pResult = 1;
			return FALSE;
		}
		break;

	case WM_DWMNCRENDERINGCHANGED:
		if(dxw.Windowize) renewBackBufferClipper(hwnd);
		break;

	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
		if(dxw.Windowize) renewBackBufferClipper(hwnd);
		if(dxw.Windowize && dxw.IsFullScreen()){
			LPWINDOWPOS wp = (LPWINDOWPOS)lParam;
			if(dxw.dwFlags5 & NOWINPOSCHANGES){
				OutTraceDW("%s: %s - suppressed\n", ApiRef, Msg==WM_WINDOWPOSCHANGED ? "WM_WINDOWPOSCHANGED" : "WM_WINDOWPOSCHANGING");
				*pResult = 0;
				return FALSE; 
			}
			wp = (LPWINDOWPOS)lParam;
			dxwFixWindowPos(ApiRef, hwnd, lParam);
			OutTraceDW("%s: %s fixed size=(%d,%d)\n", 
				ApiRef, (Msg == WM_WINDOWPOSCHANGED) ? "WM_WINDOWPOSCHANGED" : "WM_WINDOWPOSCHANGING", wp->cx, wp->cy);
			if(Msg==WM_WINDOWPOSCHANGED) {
				// try to lock main wind & control parent together
				if(dxw.IsDesktop(hwnd) && dxw.hControlParentWnd) {
					POINT fo = dxw.GetFrameOffset();
					(*pMoveWindow)(dxw.hControlParentWnd, wp->x+fo.x, wp->y+fo.y, wp->cx, wp->cy, TRUE);
				}
				// v2.03.30: in window mode, it seems that the WM_ACTIVATEAPP message is not sent to the main win.
				// this PostMessage call recovers "Thorgal" block at the end of intro movie and "Championship Manager 03 04" cursor
				if (dxw.dwFlags6 & ACTIVATEAPP){
					PostMessage(hwnd, WM_ACTIVATEAPP, 1, 0);
				}
				// v2.03.91.fx4: keep position coordinates updated!
				if(!(wp->flags & (SWP_NOMOVE|SWP_NOSIZE))) dxw.UpdateDesktopCoordinates();
			}
			if((dxw.dwFlags2 & LOCKEDSIZE) || (dxw.dwFlags7 & ANCHORED)){
				// return TRUE to bypass default message processing that may alter the DxWnd settings!
				*pResult = TRUE;
				return FALSE;
			}
			if((dxw.GDIEmulationMode == GDIMODE_STRETCHED) && dxw.isScaled){
				(*pSetViewportOrgEx)((*pGDIGetDC)(hwnd), 0, 0, NULL);
				OutTraceDW("%s: fixed viewport org=(0,0)\n", ApiRef);
			}
		}
		break;

	case WM_ENTERSIZEMOVE:
		if(IsToBeLocked){
			dxw.dwFlags1 &= ~LOCKWINPOS;
		}
		while((*pShowCursor)(1) < 0);
		if (
			((dxw.dwFlags1 & CLIPCURSOR) && !(dxw.dwFlags8 & CLIPLOCKED)) ||
			(dxw.dwFlags1 & DISABLECLIPPING)) {
			dxw.EraseClipCursor();
		}
		if(dxw.dwFlags12 & ADAPTMOUSESPEED) dx_ToggleMouseSpeed(dxw.bActive);
		break;

	case WM_EXITSIZEMOVE:
		if(IsToBeLocked){
			dxw.dwFlags1 |= LOCKWINPOS;
			dx_UpdatePositionLock(hwnd);
		}
		if(dxw.Windowize) renewBackBufferClipper(hwnd);
		if((dxw.dwFlags1 & HIDEHWCURSOR) && dxw.IsFullScreen()) while((*pShowCursor)(0) >= 0);
		if(dxw.dwFlags2 & SHOWHWCURSOR) while((*pShowCursor)(1) < 0);
		if(dxw.dwFlags1 & DISABLECLIPPING) extClipCursor(lpClipRegion);
		if(dxw.dwFlags2 & REFRESHONRESIZE) dxw.ScreenRefresh();
		if(dxw.dwFlags9 & SDLEMULATION) SDLScreenRefresh();
		if(dxw.dwFlags4 & HIDEDESKTOP) dxw.HideDesktop(dxw.GethWnd());
		if(dxw.dwDFlags & CENTERTOWIN) {
			HDC thdc;
			HWND w = dxw.GethWnd();
			RECT client;
			(*pGetClientRect)(w, &client);
			thdc=(*pGDIGetDC)(w);
			if(thdc) (*pGDIBitBlt)(thdc, client.left, client.top, client.right, client.bottom, 0, 0, 0, BLACKNESS);
		}
		if(dxw.isScaled) dxw.UpdateDesktopCoordinates();
		if(dxw.dwFlags12 & ADAPTMOUSESPEED) dx_ToggleMouseSpeed(dxw.bActive);
		break;

	case WM_ACTIVATE:
		// turn DirectInput bActive flag on & off .....
		dxw.bActive = (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) ? 1 : 0;
	case WM_NCACTIVATE:
		// turn DirectInput bActive flag on & off .....
		if(Msg == WM_NCACTIVATE) dxw.bActive = wParam;
		if(dxw.bActive) (*pSetWindowPos)(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		if(dxw.dwFlags14 & STOPSOUND) dx_ToggleSound(dxw.bActive);
		if(dxw.dwFlags6 & UNACQUIRE) ToggleAcquiredDevices(dxw.bActive);
		// v2.04.86: make BACKGROUNDPRIORITY work also when UNNOTIFY is set
		if (dxw.dwFlags8 & BACKGROUNDPRIORITY) SetIdlePriority(!dxw.bActive);
		// v2.05.41: adaptive mouse speed
		if(dxw.dwFlags12 & ADAPTMOUSESPEED) dx_ToggleMouseSpeed(dxw.bActive);
		if ((dxw.dwFlags1 & UNNOTIFY) ||							// UNNOTIFY, always bypasses
			((dxw.dwFlags11 & UNNOTIFYINACTIVE) && !dxw.bActive)){	// bypass only in case of deactivation
			(*pDefWindowProcA)(hwnd, Msg, wParam, lParam);
			OutTrace("%s: UNNOTIFY\n", ApiRef);
			*pResult = 0;
			return FALSE;
		}
		break;

	case WM_NCMOUSEMOVE:
		// Posted to a window when the cursor is moved within the nonclient area of the window. 
		// This message is posted to the window that contains the cursor. 
		// If a window has captured the mouse, this message is not posted.
		// V2.1.90: on nonclient areas the cursor is always shown.
		while((*pShowCursor)(1) < 0);
		break;
	case WM_MOUSEMOVE:
		if((dxw.dwFlags10 & MOUSESHIELD) && !dxw.bActive){
			*pResult = 0;
			return FALSE;
		}
		if(dxw.Windowize){
			prev.x = LOWORD(lParam);
			prev.y = HIWORD(lParam);
			if(dxw.IsFullScreen()){
				if (dxw.dwFlags1 & HIDEHWCURSOR){
					(*pGetClientRect)(hwnd, &rect);
					if(prev.x >= 0 && prev.x < rect.right && prev.y >= 0 && prev.y < rect.bottom)
						while((*pShowCursor)(0) >= 0);
					else
						while((*pShowCursor)(1) < 0);
				}
				if (dxw.dwFlags1 & SHOWHWCURSOR){
					while((*pShowCursor)(1) < 0);
				}
			}
			if(dxw.MustFixCoordinates){ // mouse processing  
				// scale mouse coordinates
				curr=dxw.FixCursorPos(prev); //v2.02.30
				lParam = MAKELPARAM(curr.x, curr.y); 
				*lpParam = lParam;
				OutTraceC("%s: hwnd=%#x pos XY=(%d,%d)->(%d,%d)\n", ApiRef, hwnd, prev.x, prev.y, curr.x, curr.y);
			}
			GetHookInfo()->WinProcX=LOWORD(lParam);
			GetHookInfo()->WinProcY=HIWORD(lParam);
		}
		else {
			// v2.05.93 add: handling of invert mouse axis also in real fullscreen mode. Needed in "Sherlock Holmes the 
			// Mystery of the Mummy" if configured in non-windowed mode.
			if(dxw.dwFlags11 & (INVERTMOUSEXAXIS | INVERTMOUSEYAXIS)){
				prev.x = LOWORD(lParam);
				prev.y = HIWORD(lParam);
				if(dxw.dwFlags11 & INVERTMOUSEXAXIS) curr.x = dxw.GetScreenWidth() - prev.x;
				if(dxw.dwFlags11 & INVERTMOUSEYAXIS) curr.y = dxw.GetScreenHeight() - prev.y; // v2.05.93 fix !!!
				*lpParam = MAKELPARAM(curr.x, curr.y); 
				OutTraceC("%s: hwnd=%#x pos XY=(%d,%d)->(%d,%d)\n", ApiRef, hwnd, prev.x, prev.y, curr.x, curr.y);
			}
		}
		break;	

	// fall through cases:
	case WM_MOUSEWHEEL:
		if(dxw.dwFlags6 & VIRTUALJOYSTICK) {
			if(dxw.Windowize && (dxw.dwFlags1 & CLIPCURSOR) && !dxw.IsClipCursorActive()) dxw.SetClipCursor();
			if (JoyProcessMouseWheelMessage(wParam, lParam)) {
				*pResult = 0;
				return FALSE;	
			}
		} // fall through
		if(dxw.dwFlags18 & ENABLEZOOMING){
			extern D3DVALUE fZoom;
			short wDirection = (short)HIWORD(wParam);
			if(wDirection > 0) fZoom *= (D3DVALUE) 1.1;
			if((wDirection < 0) && (fZoom > (D3DVALUE)1.0)) fZoom /= (D3DVALUE) 1.1;
		}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		if(dxw.Windowize){
			if((dxw.dwFlags1 & CLIPCURSOR) && !dxw.IsClipCursorActive()) {
				dxw.SetClipCursor();
			}
			if(dxw.MustFixCoordinates){ // mouse processing  
				// scale mouse coordinates
				prev.x = LOWORD(lParam);
				prev.y = HIWORD(lParam);
				curr = prev;
				if(Msg == WM_MOUSEWHEEL){ // v2.02.33 mousewheel fix
					POINT upleft={0,0};
					(*pClientToScreen)(dxw.GethWnd(), &upleft);
					curr = dxw.SubCoordinates(curr, upleft);
				}
				curr=dxw.FixCursorPos(curr); //v2.02.30
				lParam = MAKELPARAM(curr.x, curr.y); 
				*lpParam = lParam;
				OutTraceC("%s: hwnd=%#x pos XY=(%d,%d)->(%d,%d)\n", ApiRef, hwnd, prev.x, prev.y, curr.x, curr.y);
			}
			GetHookInfo()->WinProcX=LOWORD(lParam);
			GetHookInfo()->WinProcY=HIWORD(lParam);
		}
		break;	

	case WM_SETCURSOR:
		if(dxw.dwFlags17 & LOCKCURSORSHAPE) {
			*pResult = TRUE;
			return FALSE;
		}
		break;

	case WM_SYSCOMMAND:
		// v2.03.56.fix1 by FunkyFr3sh: ensure that "C&C Red Alert 2" receives the WM_SYSCOMMAND / SC_CLOSE message
		// that likely is filtered by the application logic
		// v2.03.91: from msdn - In WM_SYSCOMMAND messages, the four low-order bits of the wParam parameter are used 
		// internally by the system. To obtain the correct result when testing the value of wParam, an application 
		// must combine the value 0xFFF0 with the wParam value by using the bitwise AND operator. 
		if(dxw.Windowize && ((wParam & 0xFFF0)== SC_CLOSE) && (dxw.dwFlags6 & TERMINATEONCLOSE)) {
			*pResult = (*pDefWindowProcA)(hwnd, Msg, wParam, lParam);
			return FALSE;
		}
		if(dxw.isScaled){
			static int iLastX, iLastY, iLastW, iLastH;
			switch(wParam & 0xFFF0){
				case SC_MINIMIZE:
					dxw.IsVisible = FALSE;
					iLastX = dxw.iPosX; iLastY = dxw.iPosY;
					iLastW = dxw.iSizX; iLastH = dxw.iSizY;
					dxw.iPosX = dxw.iPosY = dxw.iSizX = dxw.iSizY = 0;
					break;
				case SC_RESTORE:
					dxw.IsVisible = TRUE;
					dxw.iPosX = iLastX; dxw.iPosY = iLastY;
					dxw.iSizX = iLastW; dxw.iSizY = iLastH;
					break;
				case SC_MAXIMIZE:
					dxw.IsVisible = TRUE;
					break;
			}
		}
		break;

	case WM_CLOSE:
		// Beware: closing main window does not always mean that the program is about to terminate!!!
		if(dxw.dwFlags6 & CONFIRMONCLOSE){
			OutTraceDW("%s: WM_CLOSE - terminating process?\n", ApiRef);
			if (MessageBoxA(NULL, "Do you really want to exit the game?", "DxWnd", MB_YESNO | MB_TASKMODAL) != IDYES) return FALSE;
		}			
		if(dxw.dwFlags6 & HIDETASKBAR) gShowHideTaskBar(FALSE);
		if(dxw.dwFlags3 & FORCE16BPP) RecoverScreenMode();
		if(dxw.dwFlags6 & TERMINATEONCLOSE) TerminateProcess(GetCurrentProcess(),0);
		break;

	case WM_SYSKEYDOWN:
		if ((dxw.dwFlags1 & HANDLEALTF4) && (wParam == VK_F4)) {
			OutTraceDW("%s: WM_SYSKEYDOWN(ALT-F4) - terminating process\n", ApiRef);
			TerminateProcess(GetCurrentProcess(),0);
		}
		// fall through
	case WM_KEYDOWN:
		if(dxw.dwFlags4 & ENABLEHOTKEYS){
			OutTraceW("%s: event %s wparam=%#x lparam=%#x\n", 
				ApiRef, (Msg==WM_SYSKEYDOWN)?"WM_SYSKEYDOWN":"WM_KEYDOWN", wParam, lParam);
			HandleHotKeys(hwnd, Msg, lParam, wParam);
		}
		break;

	case WM_MOVING:
	case WM_MOVE:
		if(dxw.dwFlags5 & NOWINPOSCHANGES) {
			*pResult = TRUE;
			return FALSE;
		}
		break;

	case WM_SETFOCUS: 
		if(dxw.dwFlags1 & CLIPCURSOR) dxw.SetClipCursor(); 
		break;

	case WM_KILLFOCUS: 
		if(dxw.dwFlags1 & CLIPCURSOR) dxw.EraseClipCursor(); 
		break; // v2.04.14: forgotten case ....

	case WM_SETFONT:
		if(dxw.dwFlags1 & FIXTEXTOUT) {
		*wpParam = (WPARAM)fontdb.GetScaledFont((HFONT)wParam);
		OutTraceGDI("%s: replaced scaled font hfnt=%#x\n", ApiRef, wParam);
	}

	case WM_SETTEXT:
		if(dxw.dwFlags14 & NOSETTEXT){
			OutTraceGDI("%s: SUPPRESS WM_SETTEXT\n", ApiRef);
			*pResult = TRUE;
			*lpParam = 0;
		}

	default:
		break;
	}

	// marker to run hooked function
	return TRUE;
}
