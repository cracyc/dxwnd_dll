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
#include <dsound.h>
#include <Dbt.h>

extern void RecoverScreenMode();
extern void dx_FullScreenToggle(void);
extern void dx_DesktopToggle(BOOL);
extern void SDLScreenRefresh();
extern BOOL JoyProcessMouseWheelMessage(WPARAM, LPARAM);
extern BOOL gb_RefreshSemaphore;
extern void renewBackBufferClipper(HWND);

BOOL wasPlaying = FALSE;
static BOOL EscSemaphore = FALSE;

void dx_ToggleSound(BOOL active)
{
	ApiName("dx_ToggleSound");
	static LONG dSoundVolume = 0;
	extern LPDIRECTSOUNDBUFFER pDSPrimaryBuffer;
	HRESULT hres;
	OutTraceDW("%s: active=%d\n", ApiRef, active);
	if(active){
		// CD audio player
		if(wasPlaying){
				player_set_status(MCI_RESUME);
		}
		// dsound
		if(pDSPrimaryBuffer){
#ifdef USEVOLUMECONTROL
			hres = pDSPrimaryBuffer->SetVolume(dSoundVolume);
			if (!hres) {
				OutTraceSND("Toggle dsound volume ON vol=%#x\n", dSoundVolume);
			}
			else{
				OutTraceE("Toggle dsound volume ON ERROR err=%#x(%s)\n", hres, ExplainDDError(hres));
			}
#else
			hres = pDSPrimaryBuffer->Restore();
			if (hres) {
				OutTraceE("%s: Toggle dsound restore ON ERROR err=%#x(%s)\n", ApiRef, hres, ExplainDDError(hres));
			}
#endif
		}
	}
	else{
		// CD audio player
		if(mciEmu.playing) {
			player_set_status(MCI_PAUSE);
			wasPlaying = TRUE;
		}
		else{
			wasPlaying = FALSE;
		}
		// dsound
		if(pDSPrimaryBuffer){
#ifdef USEVOLUMECONTROL
			hres = pDSPrimaryBuffer->GetVolume(&dSoundVolume);
			if (!hres) {
				OutTraceSND("Toggle dsound GetVolume OFF vol=%#x\n", dSoundVolume);
			}
			else{
				OutTraceE("Toggle dsound GetVolume ERROR err=%#x(%s)\n", hres, ExplainDDError(hres));
			}
			hres = pDSPrimaryBuffer->SetVolume(DSBVOLUME_MIN);
			if (!hres) {
				OutTraceSND("Toggle dsound volume OFF\n");
			}
			else{
				OutTraceE("Toggle dsound volume OFF ERROR err=%#x(%s)\n", hres, ExplainDDError(hres));
			}		
#else
			hres = pDSPrimaryBuffer->Stop();
			if (hres) {
				OutTraceE("%s: Toggle dsound suspend ON ERROR err=%#x(%s)\n", ApiRef, hres, ExplainDDError(hres));
			}
#endif
		}
	}
}

void dx_ToggleMouseSpeed(BOOL active)
{
	ApiName("dx_ToggleMouseSpeed");
	extern SystemParametersInfo_Type pSystemParametersInfoA;
	static int iInitialMouseSpeed = 0;
	int iMouseSpeed;
	if(pSystemParametersInfoA == 0) pSystemParametersInfoA = SystemParametersInfoA;
	if(iInitialMouseSpeed == 0){
		(*pSystemParametersInfoA)(SPI_GETMOUSESPEED, 0, (PVOID)iInitialMouseSpeed, 0);
		OutTraceDW("%s: Initial mouse speed=%d\n", ApiRef, iInitialMouseSpeed);
		if ((iInitialMouseSpeed <= 0) || (iInitialMouseSpeed > 20)) iInitialMouseSpeed = 10;
	}

	if(active){
		iMouseSpeed = (dxw.iSizX * iInitialMouseSpeed) / dxw.GetScreenWidth();
		if(iMouseSpeed <= 0) iMouseSpeed = 1;
		if(iMouseSpeed > 20) iMouseSpeed = 20;
	}
	else {
		iMouseSpeed = iInitialMouseSpeed;
	}

	OutTraceDW("%S: Setting mouse speed=%d\n", ApiRef, iMouseSpeed);
	SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)iMouseSpeed, 0);
}

static void dx_ToggleLogging()
{
	ApiName("dx_ToggleLogging");
	// toggle LOGGING
	if(dxw.dwTFlags & OUTTRACE){
		OutTraceDW("%s: Toggle logging OFF\n", ApiRef);
		dxw.dwTFlags &= ~OUTTRACE;
	}
	else {
		dxw.dwTFlags |= OUTTRACE;
		OutTraceDW("%s: Toggle logging ON\n", ApiRef);
	}
	GetHookInfo()->isLogging=(dxw.dwTFlags & OUTTRACE);
}

LRESULT CALLBACK extDialogWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	ApiName("extDialogWindowProc");
	static int i=0;
	static WINDOWPOS *wp;
	WNDPROC pWindowProc;
	LRESULT res;
	static int t = -1;
	static int iRecursion = 0;

	if(iRecursion) {
		iRecursion=0;
		return NULL;
	}
	iRecursion++;

	if (t == -1)
		t = (*pGetTickCount)();
	int tn = (*pGetTickCount)();

	OutTraceW("%s: DEBUG hwnd=%#x msg=[%#x]%s(%#x,%#x)\n", ApiRef, hwnd, message, ExplainWinMessage(message), wparam, lparam);

	// optimization: don't invalidate too often!
	// 200mSec seems a good compromise.
	if (tn-t > 200) {
		t=tn;
		(*pInvalidateRect)(hwnd, NULL, TRUE);
	}

	pWindowProc=dxwws.GetProc(hwnd);
	if(pWindowProc) {
		res =(*pCallWindowProcA)(pWindowProc, hwnd, message, wparam, lparam);
	}
	else {
		char *sMsg="ASSERT: DialogWinMsg pWindowProc=NULL !!!";
		OutTraceDW("%s: %s\n", ApiRef, sMsg);
		if (IsAssertEnabled) MessageBox(0, sMsg, "WindowProc", MB_OK | MB_ICONEXCLAMATION);
		res = NULL;
	}
	iRecursion=0;
	return res;
}

LRESULT CALLBACK extChildWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	ApiName("extChildWindowProc");
	static int i=0;
	static WINDOWPOS *wp;
	WNDPROC pWindowProc;

	OutTraceW("%s: DEBUG hwnd=%#x msg=[%#x]%s(%#x,%#x)\n", ApiRef, hwnd, message, ExplainWinMessage(message), wparam, lparam);

	if(dxw.Windowize){
		switch(message){
		// Cybermercs: it seems that all game menus are conveniently handled by the WindowProc routine,
		// while the action screen get messages processed by the ChildWindowProc, that needs some different
		// setting ..........
		// Beware: Cybermercs handles some static info about cursor position handling, so that if you resize
		// a menu it doesn't work correctly until you don't change screen.
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
			if(dxw.MustFixCoordinates){ // mouse processing  
				POINT prev, curr;
				// scale mouse coordinates
				prev.x = LOWORD(lparam);
				prev.y = HIWORD(lparam);
				curr = prev;
				if(message == WM_MOUSEWHEEL){ // v2.02.33 mousewheel fix
					POINT upleft={0,0};
					(*pClientToScreen)(dxw.GethWnd(), &upleft);
					curr = dxw.SubCoordinates(curr, upleft);
				}
				//OutTraceC("ChildWindowProc: hwnd=%#x pos XY prev=(%d,%d)\n", hwnd, prev.x, prev.y);
				curr=dxw.FixCursorPos(curr); // Warn! the correction must refer to the main window hWnd, not the current hwnd one !!!
				lparam = MAKELPARAM(curr.x, curr.y); 
				OutTraceC("%s: hwnd=%#x pos XY=(%d,%d)->(%d,%d)\n", ApiRef, hwnd, prev.x, prev.y, curr.x, curr.y);
			}
			break;	
		default:
			break;
		}
	}

	pWindowProc=dxwws.GetProc(hwnd);
	
	// v2.02.82: use CallWindowProc that handles WinProc handles
	if(pWindowProc) return(*pCallWindowProcA)(pWindowProc, hwnd, message, wparam, lparam);
	// should never get here ....
	OutTraceDW("%s: no WndProc for CHILD hwnd=%#x\n", ApiRef, hwnd);
	return DefWindowProc(hwnd, message, wparam, lparam);
}

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

static void dx_TogglePositionLock()
{
	ApiName("dx_TogglePositionLock");
	// toggle position locking
	if(dxw.dwFlags1 & LOCKWINPOS){
		// unlock
		OutTraceDW("%s: lock=OFF\n", ApiRef);
		dxw.dwFlags1 &= ~LOCKWINPOS;
	}
	else {
		OutTraceDW("%s: lock=ON\n", ApiRef);
		dxw.dwFlags1 |= LOCKWINPOS;
		dx_UpdatePositionLock(dxw.GethWnd());
	}
}

static void dx_ToggleFPS()
{
	ApiName("dx_ToggleFPS");
	if(dxw.dwFlags2 & SHOWFPSOVERLAY){
		dxw.dwFlags2 &= ~SHOWFPSOVERLAY;
		OutTrace("%s: SHOWFPSOVERLAY mode OFF\n", ApiRef);
	}
	else {
		dxw.dwFlags2 |= SHOWFPSOVERLAY;
		OutTrace("%s: SHOWFPSOVERLAY mode ON\n", ApiRef);
	}
}

typedef DWORD (WINAPI *GetCurrentDirectoryA_Type)(DWORD, LPSTR);
extern GetCurrentDirectoryA_Type pGetCurrentDirectoryA;
extern SetCurrentDirectoryA_Type pSetCurrentDirectoryA;

static void dx_CDChanger(int delta)
{
	ApiName("dx_CDChanger");
	char msg[81];
	int cdindex;
	int cdnext;
	DWORD res;
	DWORD mapping;
	char fullBuffer[MAX_PATH];
	char *lpPathName;

	// v2.05.59 fix:
	// when switcing CD you must also check if, by chance or game design, you're located 
	// on the fake CD drive (this happens in "Master of Dimensions"). In this case you
	// also have to change your current wworking path (see Get/SetCurrentDirectory calls)
	// to reflect the fact that you shoud expect different files in the local folder.
	// step 1: get the fake pathname of the current folder 
	res = (*pGetCurrentDirectoryA)(MAX_PATH, fullBuffer);
	OutDebugDW("%s: current path=\"%s\"\n", ApiRef, fullBuffer);
	lpPathName = (char *)dxwUntranslatePathA((LPCSTR)fullBuffer, &mapping);
	OutDebugDW("%s: logical path=\"%s\"\n", ApiRef, fullBuffer);
	// step 2: change the CD index
	cdindex = GetHookInfo()->CDIndex;
	cdnext = (cdindex + (dxw.MaxCDVolume + 1) + delta) % (dxw.MaxCDVolume + 1);
	OutDebugDW("%s: MaxCDVolume=%d index=%d next=%d\n", ApiRef, dxw.MaxCDVolume, cdindex, cdnext);
	GetHookInfo()->CDIndex = cdnext;
	// step 3: if the current folder was on fake CD, move to the same virtual folder but
	// now using the altered CD index
	if(mapping == DXW_FAKE_CD){
		lpPathName = (char *)dxwTranslatePathA(lpPathName, NULL);
		OutDebugDW("%s: remapped path=\"%s\"\n", ApiRef, lpPathName);
		(*pSetCurrentDirectoryA)(lpPathName);
	}
	// v2.06.04 fix: lpPathName is not malloc-ed inside dxwUntranslatePathA
	// if(mapping != DXW_NO_FAKE) free(lpPathName);

	// step 4: print a dialog in proxy mode or enable the CD changer icon.
	if(cdindex != cdnext) {
		if(dxw.dwIndex == -1){
			sprintf_s(msg, 80, "cd charger: switched from CD%d to CD%d\n", cdindex+1, cdnext+1);
			MessageBox(0, msg, "DxWnd", 0);
		}
		else {
			dxw.ShowCDChanger();
		}
	}

	char label = dxw.FakeCDDrive;
	if((label>='A') && (label<='Z')){
		DWORD unitmask = 0x1 << (label - 'A');
		DEV_BROADCAST_VOLUME dev;
		dev.dbcv_size = sizeof(dev);
		dev.dbcv_devicetype = DBT_DEVTYP_VOLUME;
		dev.dbcv_reserved = 0;
		dev.dbcv_flags = DBTF_MEDIA;
		dev.dbcv_unitmask = unitmask;
		SendMessageA(dxw.GethWnd(), WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)&dev);
	}
	OutTrace("%s: switched from CD%d to CD%d\n", ApiRef, cdindex+1, cdnext+1);
}

static void dx_Cornerize()
{
	ApiName("dx_Cornerize");
	static BOOL bCornerized = FALSE;
	static RECT WinRect = {0, 0, 0, 0};
	static DWORD OldStyle, OldExtStyle;
	HWND hwnd = dxw.GethWnd();

	if (bCornerized){ 	// toggle ....
		OutTraceDW("%s: exiting corner mode\n", ApiRef);
		(*pSetWindowLong)(hwnd, GWL_STYLE, OldStyle);
		(*pSetWindowLong)(hwnd, GWL_EXSTYLE, OldExtStyle);
		(*pMoveWindow)(hwnd, WinRect.left, WinRect.top, WinRect.right, WinRect.bottom, TRUE);
		memset(&WinRect, 0, sizeof(WinRect));
	}
	else {
		OutTraceDW("%s: entering corner mode\n", ApiRef);
		(*pGetWindowRect)(hwnd, &WinRect);
		OldStyle = (*pGetWindowLong)(hwnd, GWL_STYLE);
		OldExtStyle = (*pGetWindowLong)(hwnd, GWL_EXSTYLE);
		(*pSetWindowLong)(hwnd, GWL_STYLE, WS_VISIBLE|WS_CLIPSIBLINGS|WS_OVERLAPPED);
		(*pSetWindowLong)(hwnd, GWL_EXSTYLE, 0);
		(*pMoveWindow)(hwnd, 0, 0, dxw.GetScreenWidth(), dxw.GetScreenHeight(), TRUE);
	}
	bCornerized = !bCornerized; // switch toggle
	(*pUpdateWindow)(hwnd);
	dxw.ScreenRefresh();
}

LRESULT LastCursorPos;

void SetIdlePriority(BOOL idle)
{
	ApiName("SetIdlePriority");
	OutTrace("%s: Setting priority class to %s\n", ApiRef, idle ? "IDLE_PRIORITY_CLASS" : "NORMAL_PRIORITY_CLASS");
#ifndef DXW_NOTRACES
	if(!SetPriorityClass(GetCurrentProcess(), idle ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS))
		OutTraceE("%s: SetPriorityClass ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#else
	SetPriorityClass(GetCurrentProcess(), idle ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
#endif
}

char *sMCIReason(WPARAM w, char *r)
{
	unsigned int l;
	strcpy(r, "MCI_NOTIFY_");
	if(w & MCI_NOTIFY_ABORTED) strcat(r, "ABORTED+");
	if(w & MCI_NOTIFY_FAILURE) strcat(r, "FAILURE+");
	if(w & MCI_NOTIFY_SUCCESSFUL) strcat(r, "SUCCESSFUL+");
	if(w & MCI_NOTIFY_SUPERSEDED) strcat(r, "SUPERSEDED+");
	l=strlen(r);
	if (l>strlen("DDSD_")) r[l-1]=0; // delete last '+' if any
	else r[0]=0;
	return(r);
}

#ifndef DXW_NOTRACES
void ExplainMsg(char *ApiName, BOOL isAnsi, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static char *modes[5]={"RESTORED", "MINIMIZED", "MAXIMIZED", "MAXSHOW", "MAXHIDE"};
	char sPos[512+1];
	char sFlags[128];
	sPos[320]=0;
	sPos[0]=0;
	switch(Msg){
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
		LPWINDOWPOS wp;
		wp = (LPWINDOWPOS)lParam;
		sprintf_s(sPos, 512, " pos=(%d,%d) size=(%dx%d) flags=%#x(%s)", wp->x, wp->y, wp->cx, wp->cy, wp->flags, ExplainWPFlags(wp->flags, sFlags, 128));
		break;
	case WM_MOVE:
		sprintf_s(sPos, 512, " pos=(%d,%d)", LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_STYLECHANGING:
	case WM_STYLECHANGED:
		LPSTYLESTRUCT style;
		style = (LPSTYLESTRUCT)lParam;
		if(wParam == GWL_STYLE) {
			char sStyle[256];
			sprintf_s(sPos, 512, "style=%#x(%s)", style->styleOld, ExplainStyle(style->styleOld, sStyle, 256));
			sprintf_s(sPos+strlen(sPos), 512-strlen(sPos), "->%#x(%s)", style->styleNew, ExplainStyle(style->styleNew, sStyle, 256));
		}
		if(wParam == GWL_EXSTYLE) {
			char sExStyle[256];
			sprintf_s(sPos, 512, "exstyle=%#x(%s)", style->styleOld, ExplainExStyle(style->styleOld, sExStyle, 256));
			sprintf_s(sPos+strlen(sPos), 512-strlen(sPos), "->%#x(%s)", style->styleNew, ExplainExStyle(style->styleNew, sExStyle, 256));
		}
		break;
	case WM_SETTEXT:
		__try{
			if(isAnsi)
				sprintf_s(sPos, 512, "text=\"%s\"", lParam);
			else
				sprintf_s(sPos, 512, "text=\"%ls\"", lParam);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			sprintf_s(sPos, 512, "text=***");
		}
		break;
	case WM_SIZE:
		sprintf_s(sPos, 512, " mode=SIZE_%s size=(%dx%d)", modes[wParam % 5], LOWORD(lParam), HIWORD(lParam));
		break;	
	case WM_TIMER:
		sprintf_s(sPos, 512, " timer=%#x cback=%#x", wParam, lParam);
		break;	
	case MM_MCINOTIFY:
		sprintf_s(sPos, 512, " devid=%#x reason=%s", lParam, sMCIReason(wParam, sFlags));
		break;	
	}
	OutTrace("%s[%#x]: WinMsg=[%#x]%s(%#x,%#x) %s\n", ApiName, hwnd, Msg, ExplainWinMessage(Msg), wParam, lParam, sPos);
}
#endif

static BOOL IsWindowMovingMessage(int msg, WPARAM wparam)
{
	switch(msg){
		// minimum set for win move/resize on Win10
		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONUP:
		case WM_WINDOWPOSCHANGED:
		case WM_WINDOWPOSCHANGING:
		case WM_STYLECHANGING: 
		case WM_STYLECHANGED: 
		// case WM_TIMER: ???
			return TRUE;
			break;
		case WM_SYSCOMMAND:
			// v2.05.78: added WS_SYSCOMMAND to messages that request a window position change
			// from msdn pages:
			// In WM_SYSCOMMAND messages, the four low-order bits of the wParam parameter are used internally 
			// by the system. To obtain the correct result when testing the value of wParam, an application 
			// must combine the value 0xFFF0 with the wParam value by using the bitwise AND operator.
			if((wparam & 0xFFF0) == SC_MOVE) return TRUE;
			if((wparam & 0xFFF0) == SC_SIZE) return TRUE;
			break;
	}
	return FALSE;
}

void HandleHotKeys(HWND hwnd, UINT message, LPARAM lparam, WPARAM wparam)
{
	ApiName("HandleHotKeys");
	UINT DxWndKey;
	static BOOL TimeShiftToggle=TRUE;
	static int SaveTimeShift;
	static DWORD dwLastKeyDown = 0;
	DWORD dwCurKeyDown = (*pGetTickCount)();
	extern float fZoom;

	if((dwCurKeyDown-dwLastKeyDown) < 500) return; // ignore if closer than 500 mSec

	if ((dxw.dwFlags1 & HANDLEALTF4) && (message == WM_SYSKEYDOWN) && (wparam == VK_F4)) {
		OutTraceDW("%s: WM_SYSKEYDOWN(ALT-F4) - terminating process\n", ApiRef);
		TerminateProcess(GetCurrentProcess(),0);
	}

	DxWndKey=dxw.MapKeysConfig(message, lparam, wparam);
	// v2.05.11 fix: set dwLastKeyDown only when you got a real fkey
	if (DxWndKey != DXVK_NONE) dwLastKeyDown = dwCurKeyDown;

	switch (DxWndKey){
	case DXVK_CLIPTOGGLE:
		OutTraceDW("%s: WM_SYSKEYDOWN key=%#x clipper=%#x\n", ApiRef, wparam, dxw.IsClipCursorActive());
		dxw.IsClipCursorActive() ? dxw.EraseClipCursor() : dxw.SetClipCursor();
		break;
	case DXVK_REFRESH:
		dxw.ScreenRefresh();
		break;
	case DXVK_LOGTOGGLE:
		dx_ToggleLogging();
		break;
	case DXVK_PLOCKTOGGLE:
		dx_TogglePositionLock();
		break;
	case DXVK_FPSTOGGLE:
		dx_ToggleFPS();
		break;
	case DXVK_FREEZETIME:
		dxw.ToggleFreezedTime();
		break;
	case DXVK_TIMEFAST:
	case DXVK_TIMESLOW:
		if (dxw.dwFlags2 & TIMESTRETCH) {
			if (DxWndKey == DXVK_TIMESLOW && (dxw.TimeShift <  8)) dxw.TimeShift++;
			if (DxWndKey == DXVK_TIMEFAST && (dxw.TimeShift > -8)) dxw.TimeShift--;
			GetHookInfo()->TimeShift=dxw.TimeShift;
			OutTrace("%s: Time Stretch - shift=%d speed=%s\n", ApiRef, dxw.TimeShift, dxw.GetTSCaption());
		}
		break;
	case DXVK_TIMETOGGLE:
		if (dxw.dwFlags2 & TIMESTRETCH) {
			if(TimeShiftToggle){
				SaveTimeShift=dxw.TimeShift;
				dxw.TimeShift=0;
			}
			else{
				dxw.TimeShift=SaveTimeShift;
			}
			TimeShiftToggle = !TimeShiftToggle;
			GetHookInfo()->TimeShift=dxw.TimeShift;
	}
		break;
	case DXVK_ALTF4:
		dxw.EraseClipCursor(); // v2.05.26 clear clipper area before exiting
		dxw.dwFlags1 &= ~CLIPCURSOR; // v2.05.26 disable setting the clipper again !!
		if (dxw.dwFlags1 & HANDLEALTF4) {
			OutTraceDW("%s: WM_SYSKEYDOWN(virtual Alt-F4) - terminating process\n", ApiRef);
			TerminateProcess(GetCurrentProcess(),0);
		}
		break;
	case DXVK_PRINTSCREEN:
		dxw.ScreenShot();
		break;
	case DXVK_CORNERIZE:
		dx_Cornerize();
		break;
	case DXVK_FULLSCREEN:
		dx_FullScreenToggle();
		break;
	case DXVK_FAKEDESKTOP:
		dx_DesktopToggle(FALSE);
		break;
	case DXVK_FAKEWORKAREA:
		dx_DesktopToggle(TRUE);
	case DXVK_CUSTOM:
		dxw.bCustomKeyToggle = !dxw.bCustomKeyToggle;
		break; // v2.05.61
	case DXVK_CDNEXT:
	case DXVK_CDPREV:
		dx_CDChanger((DxWndKey == DXVK_CDPREV) ? -1 : +1);
		break;
	case DXVK_ZOOMIN: 
		fZoom *= (D3DVALUE)1.1;
		OutTraceDW("%s: zoom=%f\n", ApiRef, fZoom);
		break;
	case DXVK_ZOOMOUT:
		if(fZoom > (D3DVALUE)1.0) fZoom /= (D3DVALUE)1.1;
		OutTraceDW("%s: zoom=%f\n", ApiRef, fZoom);
		break;
	default:
		break;
	}
}

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
static BOOL ncMoving = FALSE;
static int ncMouseX;
static int ncMouseY;
static RECT wr;
static POINT ncAttackPoint;
static int ncLastHit = 0;

static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	int x, y, cx, cy;
	if(dxwcws.Get(hwnd, &x, &y, &cx, &cy)){
		RECT pr;
		(*pGetClientRect)((HWND)lParam, &pr);
		x = (x * pr.right) / dxw.GetScreenWidth();
		y = (y * pr.bottom) / dxw.GetScreenHeight();
		cx = (cx  * pr.right) / dxw.GetScreenWidth();
		cy = (cy * pr.bottom) / dxw.GetScreenHeight();
		(*pMoveWindow)(hwnd, x, y, cx, cy, 0);
		(*pInvalidateRect)(hwnd, 0, TRUE);
		OutTrace("EnumChildProc hwnd=%#x pos=(%d,%d) size=(%dx%d)\n", hwnd, x, y, cx, cy);
	}
	return TRUE;
}

static BOOL ncCalcSizeEmu(HWND hwnd, int message, LPARAM lparam, WPARAM wparam)
{
	int x, y, cx, cy;
	switch(message){
	case WM_LBUTTONUP:
		//OutTrace(">>> WM_LBUTTONUP\n");
		ncMoving = FALSE;
		ReleaseCapture();
		break;

	case WM_NCLBUTTONDOWN:
		ncMouseX = GET_X_LPARAM(lparam);
		ncMouseY = GET_Y_LPARAM(lparam);
		(*pGetWindowRect)(hwnd, &wr);
		switch(wparam){
			case HTLEFT:
			case HTRIGHT:
			case HTTOP:
			case HTBOTTOM:
			case HTTOPLEFT:
			case HTTOPRIGHT:
			case HTBOTTOMLEFT:
			case HTBOTTOMRIGHT:
			case HTCAPTION:
				OutTrace(">>> WM_NCLBUTTONDOWN hit=%d pos=(%d, %d)\n", wparam, lparam & 0xFFFF, (lparam >> 16) & 0xFFFF);
				ncMoving = TRUE;
				SetCapture(hwnd);
				ncLastHit = wparam;
				ncAttackPoint.x = ncMouseX;
				ncAttackPoint.y = ncMouseY;
				break;
		}
		break;

	case WM_NCLBUTTONUP:
		//OutTrace(">>> WM_NCLBUTTONUP hit=%d pos=(%d, %d)\n", wparam, lparam & 0xFFFF, (lparam >> 16) & 0xFFFF);
		ncMoving = FALSE;
		break;

	case WM_MOUSEMOVE:
		//OutTrace(">>> WM_MOUSEMOVE hit=%d pos=(%d, %d)\n", ncLastHit, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		if(ncMoving && (wparam & MK_LBUTTON) && ncLastHit){
			x = wr.left;
			y = wr.top;
			cx = wr.right - wr.left;
			cy = wr.bottom - wr.top;
			POINT p;
			(*pGetCursorPos)(&p);
			switch(ncLastHit){
				case HTLEFT:
					x = p.x - GetSystemMetrics(SM_CXSIZEFRAME);
					cx = wr.right - x;
					break;
				case HTRIGHT:
					x = wr.left;
					cx = p.x - wr.left + GetSystemMetrics(SM_CXSIZEFRAME);
					break;
				case HTTOP:
					y = p.y;
					cy = wr.bottom - y;
					break;
				case HTBOTTOM:
					y = wr.top;
					cy = p.y - wr.top + GetSystemMetrics(SM_CYSIZEFRAME);
					break;
				case HTTOPLEFT:
					x = p.x - GetSystemMetrics(SM_CXSIZEFRAME);
					y = p.y;
					cx = wr.right - x;
					cy = wr.bottom - y;
					break;
				case HTTOPRIGHT:
					x = wr.left;
					y = p.y;
					cx = p.x - wr.left + GetSystemMetrics(SM_CXSIZEFRAME);
					cy = wr.bottom - y;
					break;
				case HTBOTTOMLEFT:
					x = p.x - GetSystemMetrics(SM_CXSIZEFRAME);
					y = wr.top;
					cx = wr.right - x;
					cy = p.y - wr.top + GetSystemMetrics(SM_CYSIZEFRAME);
					break;
				case HTBOTTOMRIGHT:
					x = wr.left;
					y = wr.top;
					cx = p.x - wr.left + GetSystemMetrics(SM_CXSIZEFRAME);
					cy = p.y - wr.top + GetSystemMetrics(SM_CYSIZEFRAME);
					break;
				case HTCAPTION:
					x = wr.left + p.x - ncAttackPoint.x;
					y = wr.top + p.y - ncAttackPoint.y;
					break;
			}
			//OutTrace("--- (%d,%d) - (%d,%d)\n", x, y, cx, cy);
			(*pMoveWindow)(hwnd, x, y, cx, cy, 0);
			// try not to interfere with the window handling: 
			// do the refresh only if requested (dxw.dwFlags2 & REFRESHONRESIZE)
			// and if necessary (ncLastHit != HTCAPTION).
			if((dxw.dwFlags2 & REFRESHONRESIZE) &&
				(ncLastHit != HTCAPTION))
			{
				(*pInvalidateRect)(hwnd, 0, TRUE);
				if(dxw.dwFlags19 & SCALECHILDWIN) 
					EnumChildWindows(hwnd, EnumChildProc, (LPARAM)hwnd);
			}
			return 0;
		}
		break;
	}
	return 1;
}

LRESULT CALLBACK extWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	ApiName("WindowProc");
	static int i=0;
	WNDPROC pWindowProc;
	extern void dxwFixWindowPos(char *, HWND, LPARAM);
	extern LPRECT lpClipRegion;

#ifndef DXW_NOTRACES
	if ((IsTraceW) ||
		(IsTraceSND && (message == MM_MCINOTIFY)))
		ExplainMsg(ApiRef, TRUE, hwnd, message, wparam, lparam);
#endif

	if(dxw.dwFlags19 & EMULATEWINRESIZE){
		BOOL ret = ncCalcSizeEmu(hwnd, message, lparam, wparam);
		if(!ret) return 0;
	}

	if(dxw.dwFlags3 & (FILTERMESSAGES|DEFAULTMESSAGES)){
		switch(message){
		case WM_NCMOUSELEAVE: 
		case WM_NCMOUSEMOVE:
		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONUP:
		case WM_NCLBUTTONDBLCLK:
		case WM_NCRBUTTONDOWN:
		case WM_NCRBUTTONUP:
		case WM_NCRBUTTONDBLCLK:
		case WM_NCMBUTTONDOWN:
		case WM_NCMBUTTONUP:
		case WM_NCMBUTTONDBLCLK:
		case WM_MOVE:
		case WM_MOVING:
			if(dxw.dwFlags3 & FILTERMESSAGES){
				OutTraceDW("%s[%#x]: SUPPRESS WinMsg=[%#x]%s(%#x,%#x)\n", ApiRef, hwnd, message, ExplainWinMessage(message), wparam, lparam);
				return 0;
			}
			else {
				OutTraceDW("%s[%#x]: DEFAULT WinMsg=[%#x]%s(%#x,%#x)\n", ApiRef, hwnd, message, ExplainWinMessage(message), wparam, lparam);
				return (*pDefWindowProcA)(hwnd, message, wparam, lparam);
			}
		}
	}

	// v2.04.11: the processing of at least WM_WINPOSCHANG-ING/ED for "Man TT Superbike" must be placed
	// here, before and avoiding the call to the original callback because of the AdjustWindowRect that
	// is inside the callback and would keep the position fixed.
	if(dxw.dwFlags2 & FORCEWINRESIZE){ 
		if(IsWindowMovingMessage(message, wparam)){
			LRESULT ret;
			ret = (*pDefWindowProcA)(hwnd, message, wparam, lparam);
			// v2.04.55: in case of resizing & stretched gdi mode, viewport origins must be updated
			if(dxw.GDIEmulationMode == GDIMODE_STRETCHED){
				(*pSetViewportOrgEx)((*pGDIGetDC)(hwnd), 0, 0, NULL);
				OutTraceDW("%s: fixed viewport org=(0, 0)\n", ApiRef);
			}
			return ret;
		}
	}

	HRESULT Result;
	BOOL action = FixWindowProc(ApiRef, hwnd, message, &wparam, &lparam, &Result);
	if(!action) return Result;

	// v2.06.05 fix: suspend AUTOREFRESH during mci movie play
	// fixes @#@ "Cossacks" intro movie flickering
	if ((dxw.dwFlags1 & AUTOREFRESH) && !gb_RefreshSemaphore) dxw.ScreenRefresh();

	pWindowProc=dxwws.GetProc(hwnd);
	//OutDebugDW("WindowProc: pWindowProc=%#x extWindowProc=%#x message=%#x(%s) wparam=%#x lparam=%#x\n", 
	//	(*pWindowProc), extWindowProc, message, ExplainWinMessage(message), wparam, lparam);
	if(pWindowProc) {
		LRESULT ret;

		// v2.02.36: use CallWindowProc that handles WinProc handles
		ret=(*pCallWindowProcA)(pWindowProc, hwnd, message, wparam, lparam);

		switch(message){
			case WM_SIZE:
			//case WM_WINDOWPOSCHANGED: - no good!!!!
				// update new coordinates
				if (dxw.IsFullScreen()) dxw.UpdateDesktopCoordinates();
				break;
			case WM_NCHITTEST:
				// save last NCHITTEST cursor position for use with KEEPASPECTRATIO scaling
				LastCursorPos=ret;
				break;
		}

		// v2.01.89: if FORCEWINRESIZE add standard processing for the missing WM_NC* messages
		// v2.04.69: added WM_NCPAINT and WM_NCACTIVATE to defaulted messages to ensure non-client area 
		// repaint after Win+D and return events
		if(dxw.dwFlags2 & FORCEWINRESIZE){ 
			switch(message){
			//case WM_NCHITTEST:
			case WM_NCPAINT:
			//case WM_NCMOUSEMOVE:
			//case WM_NCCALCSIZE:
			case WM_NCACTIVATE:
			case WM_SETCURSOR:		// shows a different cursor when moving on borders
			case WM_NCLBUTTONDOWN:	// intercepts mouse down on borders
			case WM_NCLBUTTONUP:	// intercepts mouse up on borders
				ret=(*pDefWindowProcA)(hwnd, message, wparam, lparam);
				break;
			}
		}

		return ret;
	}

	//OutTraceDW("ASSERT: WindowProc mismatch hwnd=%#x\n", hwnd);
	// ??? maybe it's a normal condition, whenever you don't have a WindowProc routine
	// like in Commandos 2. Flag it?
	char sMsg[81];
	sprintf(sMsg,"ASSERT: WindowProc mismatch hwnd=%#x", hwnd);
	OutTraceDW("%s: %s\n", ApiRef, sMsg);
	if (IsAssertEnabled) MessageBox(0, sMsg, "WindowProc", MB_OK | MB_ICONEXCLAMATION);	
	return (*pDefWindowProcA)(hwnd, message, wparam, lparam);
}