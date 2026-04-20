/*
DXWnd/dxwnd.cpp
DirectX Hook Module
Copyright(C) 2004-2021 SFB7/GHO

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _CRT_SECURE_NO_WARNINGS
#define _MODULE "dxwnd"

#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#ifdef DWX_ACL_ANYBODY_FULL_ACCESS
#include <aclapi.h>
#include <accctrl.h>
#endif // DWX_ACL_ANYBODY_FULL_ACCESS
#include "dxwnd.h"
#include "dxwcore.hpp"

#include "TlHelp32.h"

#ifdef _DEBUG
#define DEBUGSUFFIX ".debug" 
#else
#define DEBUGSUFFIX 
#endif

#ifdef DXW_NOTRACES
#define STREAMSUFFIX ".stream"
#else
#define STREAMSUFFIX
#endif
#ifdef TRACEALL
#define TRACESUFFIX ".trace" 
#else
#define TRACESUFFIX 
#endif
#define VERSION "v2.06.14.rc9" DEBUGSUFFIX STREAMSUFFIX TRACESUFFIX

#define DDTHREADLOCK 1

LRESULT CALLBACK HookProc(int ncode, WPARAM wparam, LPARAM lparam);

HINSTANCE hInst;
HHOOK hHook = NULL;
HHOOK hMouseHook = NULL;
HHOOK hKeyboardHook = NULL;
HANDLE hMapping;
TARGETMAP *pMapping;
DXWNDSTATUS *pStatus;
HANDLE hMutex;
HANDLE hTraceMutex;
HANDLE hLockMutex;
HANDLE hDDLockMutex;
HANDLE hKillMutex;
int HookStatus=DXW_IDLE;
static int TaskIndex=-1;
DXWNDSTATUS DxWndStatus;

void InjectHook(); 

/* ------------------------------------------------------------------ */
// filters table (to be sync with GUI!!)
/* ------------------------------------------------------------------ */

extern Blitter_Type  GDISyncBlitter;
extern Blitter_Type  GDIAsyncBlitter;
extern Blitter_Type  SDLSyncBlitter;
extern Blitter_Type  SDL2SyncBlitter;
extern Blitter_Type  OpenGLSyncBlitter;
extern Blitter_Type  OpenGLAsyncBlitter;
extern Blitter_Type  DDrawAsyncBlitter;
extern Blitter_Type  DDrawSyncBlitter;
extern Blitter_Type  D3D9AsyncBlitter;
extern Blitter_Type  D3D9SyncBlitter;
extern Blitter_Type PrimaryNotEmulated;

extern HRESULT WINAPI ColorConversionDDRAW(int, LPDIRECTDRAWSURFACE, RECT, LPDIRECTDRAWSURFACE *);
extern HRESULT WINAPI ColorConversionGDI(int, LPDIRECTDRAWSURFACE, RECT, LPDIRECTDRAWSURFACE *);
extern HRESULT WINAPI ColorConversionEmulated(int, LPDIRECTDRAWSURFACE, RECT, LPDIRECTDRAWSURFACE *);

extern void dx_ToggleMouseSpeed(BOOL);

#define DXW_MASK_EMU 0x00000001
#define DXW_MASK_OGL 0x00000002
#define DXW_MASK_GDI 0x00000004

dxw_Filter_Type dxwFilters[]={
	{"none",				DXW_FILTER_NONE,		0, 0, DXW_MASK_ALL},
	{"fast bilinear 2x",	DXW_FILTER_BILX2,		2, 2, DXW_MASK_EMU},
	{"HQ 2x",				DXW_FILTER_HQX2,		2, 2, DXW_MASK_EMU},
	{"HQ 3x",				DXW_FILTER_HQX3,		3, 3, DXW_MASK_EMU},
	{"HQ 4x",				DXW_FILTER_HQX4,		4, 4, DXW_MASK_EMU},
	{"deinterlace",			DXW_FILTER_DEINTERLACE,	0, 0, DXW_MASK_EMU|DXW_MASK_OGL},
	{"interlace",			DXW_FILTER_INTERLACE,	0, 0, DXW_MASK_EMU|DXW_MASK_OGL},
	{"PIX 2x",				DXW_FILTER_PIX2,		2, 2, DXW_MASK_EMU},
	{"PIX 3x",				DXW_FILTER_PIX3,		3, 3, DXW_MASK_EMU},
	{"PIX 4x",				DXW_FILTER_PIX4,		4, 4, DXW_MASK_EMU},
	{"Scale2X 2x",			DXW_FILTER_SCALE2X,		2, 2, DXW_MASK_EMU},
	{"Scale2X 3x",			DXW_FILTER_SCALE3X,		3, 3, DXW_MASK_EMU},	
	{"Scale2X 4x",			DXW_FILTER_SCALE4X,		4, 4, DXW_MASK_EMU},	
	{"Scale2K 2x",			DXW_FILTER_SCALE2K,		2, 2, DXW_MASK_EMU},	
	{"Scale2K 3x",			DXW_FILTER_SCALE3K,		3, 3, DXW_MASK_EMU},	
	{"Scale2K 4x",			DXW_FILTER_SCALE4K,		4, 4, DXW_MASK_EMU},
	{"Dithering",			DXW_FILTER_DITHER,		0, 0, DXW_MASK_EMU},
	{"HalfTone",			DXW_FILTER_HALFTONE,	0, 0, DXW_MASK_GDI},
	{"Rotate 90",			DXW_FILTER_ROTATE90,	0, 0, DXW_MASK_OGL},
	{"Rotate 180",			DXW_FILTER_ROTATE180,	0, 0, DXW_MASK_OGL},
	{"Rotate 270",			DXW_FILTER_ROTATE270,	0, 0, DXW_MASK_OGL},
	{"Blur",				DXW_FILTER_BLUR,		0, 0, DXW_MASK_OGL},
	{NULL, 0, 0, 0, 0}					// terminator
};

dxw_Renderer_Type dxwRenderers[]={
	{	"none",				
		DXW_RENDERER_NONE,			
		0,
		0,
		PrimaryNotEmulated, NULL, NULL
	},		
	{	"primary buffer",		
		DXW_RENDERER_EMULATEBUFFER,	
		DXWRF_BUILDREALSURFACES|DXWRF_BUILDLOCKBUFFER,
		0,
		DDrawSyncBlitter, DDrawAsyncBlitter, ColorConversionDDRAW // ???
	},
	{
		"locked surface",
		DXW_RENDERER_LOCKEDSURFACE, 
		DXWRF_BUILDLOCKSURFACE, 
		0,
		PrimaryNotEmulated, NULL, ColorConversionEmulated
	},	
	{
		"primary surface",
		DXW_RENDERER_PRIMSURFACE,
		DXWRF_EMULATED|DXWRF_BUILDREALSURFACES, 
		DXW_MASK_EMU,
		DDrawSyncBlitter, DDrawAsyncBlitter, ColorConversionEmulated
	},
	//{
	//	"hybrid",
	//	DXW_RENDERER_HYBRID,
	//	DXWRF_EMULATED|DXWRF_BUILDREALSURFACES, 
	//	0,
	//	DDrawSyncBlitter, DDrawAsyncBlitter, ColorConversionDDRAW
	//},
	{
		"GDI",
		DXW_RENDERER_GDI,
		DXWRF_EMULATED|DXWRF_PALETTEREFRESH|DXWRF_USEBACKBUFFER, 
		DXW_MASK_GDI,
		GDISyncBlitter, GDIAsyncBlitter, ColorConversionGDI
	},
	{
		"SDL",
		DXW_RENDERER_SDL,
		DXWRF_EMULATED|DXWRF_PALETTEREFRESH|DXWRF_USEBACKBUFFER, 
		0,
		SDLSyncBlitter, NULL, NULL
	},
	{
		"SDL2",
		DXW_RENDERER_SDL2,
		DXWRF_EMULATED|DXWRF_PALETTEREFRESH|DXWRF_USEBACKBUFFER,
		0,
		SDL2SyncBlitter, NULL, NULL
	},
	{
		"OpenGL",
		DXW_RENDERER_OPENGL,
		DXWRF_EMULATED|DXWRF_PALETTEREFRESH|DXWRF_USEBACKBUFFER, 
		DXW_MASK_OGL,
		OpenGLSyncBlitter, OpenGLAsyncBlitter, NULL
	},
	{
		"D3D9",
		DXW_RENDERER_D3D9,
		DXWRF_EMULATED|DXWRF_PALETTEREFRESH|DXWRF_USEBACKBUFFER, 
		0,
		D3D9SyncBlitter, D3D9AsyncBlitter, NULL
	},
	{NULL, 0, 0, 0, 0, 0}			// terminator
};

#ifdef DWX_ACL_ANYBODY_FULL_ACCESS
void SetSecurityForEverybody(SECURITY_ATTRIBUTES *lpsa)
{
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	PSID everyone_sid = NULL;
	AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 
	   0, 0, 0, 0, 0, 0, 0, &everyone_sid);

	EXPLICIT_ACCESS ea;
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName  = (LPTSTR)everyone_sid;

	PACL acl = NULL;
	SetEntriesInAcl(1, &ea, NULL, &acl);

	PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, 
									   SECURITY_DESCRIPTOR_MIN_LENGTH);
	InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE);

	lpsa->nLength = sizeof(SECURITY_ATTRIBUTES);
	lpsa->lpSecurityDescriptor = sd;
	lpsa->bInheritHandle = FALSE;
}
#endif // DWX_ACL_ANYBODY_FULL_ACCESS

BOOL APIENTRY DllMain( HANDLE hmodule, 
                       DWORD  dwreason, 
                       LPVOID preserved
					 )
{
	HANDLE hCurrentThread;

	if(dwreason == DLL_PROCESS_DETACH){
		if(dxw.dwFlags13 & EMULATECDMIXER){
			char sProfilePath[MAX_PATH+1];
			char sVolume[16];
			sprintf(sProfilePath, "%s\\mixer.ini", GetDxWndPath());
			sprintf(sVolume, "%d", dxw.MixerCDVolumeLevel);
			(*pWritePrivateProfileStringA)("CD", "MuteStatus", dxw.MixerCDMuteStatus ? "1" : "0", sProfilePath);	
			(*pWritePrivateProfileStringA)("CD", "VolumeLevel", sVolume, sProfilePath);
		}
		if(dxw.dwFlags16 & RECOVERSYSCURSORICONS) RestoreSystemCursors();
		if(dxw.dwFlags12 & ADAPTMOUSESPEED) dx_ToggleMouseSpeed(FALSE);
		if(pInvalidateRect) (*pInvalidateRect)(0, NULL, FALSE); // invalidate full desktop, no erase.
		// UnmapViewOfFile(pMapping); // v2.04.83
		UnmapViewOfFile(pStatus); // v2.04.81
		CloseHandle(hMapping);
	}

    if(dwreason != DLL_PROCESS_ATTACH) return TRUE;

	hCurrentThread = GetCurrentThread();
	SetThreadPriority(hCurrentThread, THREAD_PRIORITY_HIGHEST); // trick to reduce concurrency problems at program startup

	hInst = (HINSTANCE)hmodule;
	// optimization: disables DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications for the specified DLL
	DisableThreadLibraryCalls((HMODULE)hmodule);
#ifdef DWX_ACL_ANYBODY_FULL_ACCESS
	SECURITY_ATTRIBUTES sa;
	SetSecurityForEverybody(&sa);
	hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
		0, sizeof(DxWndStatus)+sizeof(TARGETMAP)*MAXTARGETS, "UniWind_TargetList");
#else
	hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
		0, sizeof(DxWndStatus)+sizeof(TARGETMAP)*MAXTARGETS, "UniWind_TargetList");
#endif // DWX_ACL_ANYBODY_FULL_ACCESS
	if(!hMapping) {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		return false;
	}
	// v2.0.2.75: beware: some tasks (namely, Flash player) get dxwnd.dll loaded, but can't create the file mapping
	// this situation has to be intercepted, or it can cause the dll to cause faults that may crash the program.
	pStatus = (DXWNDSTATUS *)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DXWNDSTATUS)+sizeof(TARGETMAP)*MAXTARGETS);
	if(!pStatus) return false;
	pMapping = (TARGETMAP *)((char *)pStatus + sizeof(DXWNDSTATUS));
	hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "UniWind_Mutex");
	if(!hMutex) hMutex = CreateMutex(0, FALSE, "UniWind_Mutex");
	hTraceMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "Trace_Mutex");
	if(!hTraceMutex) hTraceMutex = CreateMutex(0, FALSE, "Trace_Mutex");
	hLockMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "Lock_Mutex");
	if(!hLockMutex) hLockMutex = CreateMutex(0, FALSE, "Lock_Mutex");
	if(DDTHREADLOCK){
		hDDLockMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "DDLock_Mutex");
		if(!hDDLockMutex) hDDLockMutex = CreateMutex(0, FALSE, "DDLock_Mutex");
	}
	InjectHook();

	SetThreadPriority(hCurrentThread, THREAD_PRIORITY_NORMAL);
	CloseHandle(hCurrentThread);
	return true;
}

dxw_Filter_Type *GetFilterList()
{
	return dxwFilters;
}

dxw_Renderer_Type *GetRendererList()
{
	return dxwRenderers;
}

DWORD GetDxWndCaps()
{
	// default: all ON
	DWORD dxwCaps = 0xFFFFFFFF;
#ifdef DXW_NOTRACES
	dxwCaps &= ~DXWCAPS_CANLOG;
#endif
	return dxwCaps;
}

static BOOL GetMultiTaskEnabling(){
	char inipath[MAX_PATH];
	GetModuleFileName(GetModuleHandle("dxwnd"), inipath, MAX_PATH);
	inipath[strlen(inipath)-strlen("dxwnd.dll")] = 0; // terminate the string just before "dxwnd.dll"
	strcat(inipath, "dxwnd.ini");
	return GetPrivateProfileInt("window", "multiprocesshook", 0, inipath);
}

int SetTarget(DXWNDSTATUS *statusmap, TARGETMAP *targets){
	int i;
	char path[MAX_PATH+1];

	WaitForSingleObject(hMutex, INFINITE);
	pStatus->Status=DXW_IDLE;
	pStatus->IsFullScreen=FALSE;
	pStatus->TaskIdx=0;
	pStatus->hWnd=NULL;
	pStatus->ColorDepth=0;
	memset((void *)&(pStatus->pfd), 0, sizeof(DDPIXELFORMAT));
	pStatus->Height = pStatus->Width = 0;
	pStatus->DXVersion = 0;
	pStatus->AllowMultiTask=GetMultiTaskEnabling();
	//pStatus->VJoyStatus=VJOYENABLED|CROSSENABLED|INVERTYAXIS;
	pStatus->FogFactor = 1.0f;
	pStatus->LightFactor = 1.0f;
	if(statusmap) {
		pStatus->VJoyStatus=statusmap->VJoyStatus;
		pStatus->VJoySensivity=statusmap->VJoySensivity;
		pStatus->VJoyDeadZone=statusmap->VJoyDeadZone;
		pStatus->FogFactor = statusmap->FogFactor;
		pStatus->LightFactor = statusmap->LightFactor;
	}
	for(i = 0; targets[i].path[0] && (i < MAXTARGETS); i ++){
		char *c;
		pMapping[i] = targets[i];
		c = targets[i].path;
		if(*c == '*')
			strcpy(path, targets[i].path);
		else
			GetFullPathName(targets[i].path, MAX_PATH, path, NULL);
		for(c = path; *c; c++) *c = tolower(*c);
		strcpy(pMapping[i].path, path);
	}
	if(i < MAXTARGETS) pMapping[i].path[0] = 0;
	ReleaseMutex(hMutex);
	return i;
}

int StartHook(void)
{
	hHook = SetWindowsHookEx(WH_CALLWNDPROC, HookProc, hInst, 0);
	HookStatus=DXW_ACTIVE;
	return 0;
}

int EndHook(void)
{
	if (hHook) UnhookWindowsHookEx(hHook);
	if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	if (hKeyboardHook) UnhookWindowsHookEx(hKeyboardHook);
	HookStatus=DXW_IDLE;
	return 0;
}

char *GetDllVersion(void)
{
	return VERSION;
}

int GetHookStatus(DXWNDSTATUS *s)
{
	DWORD ret;
	ret=WaitForSingleObject(hLockMutex, 0);
	ReleaseMutex(hLockMutex);
	if(ret==WAIT_TIMEOUT) {
		if (s) *s = *pStatus;
		return DXW_RUNNING;
	}
	return HookStatus;
}

DXWNDSTATUS *GetHookInfo()
{
	return pStatus;
}

void SetFPS(int fps)
{
	pStatus->FPSCount=fps;
}

static DWORD WINAPI StatusLockThread(LPVOID hLockMutex) 
{					
	if(WaitForSingleObject((HANDLE)hLockMutex, 0)==WAIT_TIMEOUT){
		exit(0);
	}
	while(1) Sleep(INFINITE);
	return 0;
}

static DWORD WINAPI StatusMultiLockThread(LPVOID hLockMutex) 
{					
	WaitForSingleObject((HANDLE)hLockMutex, 0);
	while(1) Sleep(INFINITE);
	return 0;
}

LRESULT CALLBACK HookProc(int ncode, WPARAM wparam, LPARAM lparam)
{
	char name[MAX_PATH+1];
	HWND hwnd;
	int i;
	static int DoOnce = FALSE;

	// don't do more than once per process
	if(DoOnce) return CallNextHookEx(hHook, ncode, wparam, lparam);

	// take care here: if anything stops or delays the execution logic, the whole
	// operating system hangs, since it can't activate new windows!

	// could use WM_NCCREATE instead of WM_CREATE. Are there differences?
	hwnd = ((CWPSTRUCT *)lparam)->hwnd;
	if(((CWPSTRUCT *)lparam)->message == WM_CREATE){
		int iNameLength;
		name[MAX_PATH]=0; // string terminator
		GetModuleFileName(0, name, MAX_PATH);
		for(i = 0; name[i]; i ++) name[i] = tolower(name[i]);
		iNameLength = strlen(name);
		WaitForSingleObject(hMutex, INFINITE);
		for(i = 0; pMapping[i].path[0] && (i<MAXTARGETS); i++){
			register BOOL bMatched;
			if (!(pMapping[i].flags3 & HOOKENABLED)) continue;
			if(pMapping[i].path[0]=='*')
				bMatched=!strcmp(&name[iNameLength-strlen(pMapping[i].path)+1], &pMapping[i].path[1]);
			else
				bMatched=!strcmp(name, pMapping[i].path);
			if(bMatched)
			{
				// V.68 late fix:
				// check for locking thread (and hook) just once per process.
				// This callback is invoked per each process' thread.

				DoOnce = TRUE;
				extern HHOOK hHook;
				if(hHook) UnhookWindowsHookEx(hHook);
				// ??
				//if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
				//if (hKeyboardHook) UnhookWindowsHookEx(hKeyboardHook);

				// V.68: concurrency check. One game at a time, or exiting.
				// no good trying to insert fancy dialog boxes: the window
				// isn't ready yet, and the operation fails.

				// V2.03.07: allow multiple process hooking depending on config
				// v2.05.40: the mutex lock is moved in a separate, independent thread. 
				// This because some games can be hooked in a temporary thread that will
				// die after a while, then releasing the mutex signaled status.
				CreateThread(NULL, 0, 
					pStatus->AllowMultiTask ? StatusMultiLockThread : StatusLockThread, 
					(LPVOID)hLockMutex, 0, 0);

				pStatus->Status=DXW_RUNNING;
				pStatus->TaskIdx=i;
				pStatus->OrigIdx=pMapping[i].index;
				pStatus->IsFullScreen=FALSE;
				pStatus->hWnd=hwnd;
				pStatus->dwPid=GetProcessId(GetCurrentProcess());
				pStatus->TimeShift=pMapping[i].InitTS;
				pStatus->CursorX = pStatus->CursorY = (short)-1;
				pStatus->MessageX = pStatus->MessageY = (short)-1;
				pStatus->WinProcX = pStatus->WinProcY = (short)-1;
				pStatus->MsgHookX = pStatus->MsgHookY = (short)-1;
				pStatus->PlayerStatus = DXW_PLAYER_STOPPED;
				pStatus->TrackNo = 0;
				memset((void *)&(pStatus->pfd), 0, sizeof(DDPIXELFORMAT));
				DxWndStatus = *pStatus;
				HookInit(&pMapping[i], hwnd);
			}
		}
		ReleaseMutex(hMutex);
	}
	return CallNextHookEx(hHook, ncode, wparam, lparam);
}

void UnhookProc()
{
	// used to unhook DxWnd from the current process and allow another one (a son) to be managed
	//ReleaseMutex(hMutex);
	ReleaseMutex(hLockMutex);
	// UnmapViewOfFile(pMapping); v2.04.83
	UnmapViewOfFile(pStatus); // v2.04.81
	CloseHandle(hMapping);
}

void InjectHook()
{
	ApiName("InjectHook");
	char name[MAX_PATH+1];
	int i;
	GetModuleFileName(0, name, MAX_PATH);
	name[MAX_PATH]=0; // terminator
	for(char *c = name; *c; c++) *c = tolower(*c);
	for(i = 0; pMapping[i].path[0] && (i < MAXTARGETS); i ++){
		if(pMapping[i].flags3 & HOOKENABLED){
			if(!strncmp(name, pMapping[i].path, strlen(name))){
				HookInit(&pMapping[i],NULL);
				// beware: logging is possible only AFTER HookInit execution
				OutTrace("%s: hooked target=\"%s\"\n", ApiRef, name);
				break;
			}
		}
	}
}

#ifndef DXW_NOTRACES
static char *FlagNames[24][32] ={{
	// Flags1
	//					+					+					+
	"UNNOTIFY",			"SETDPIAWARE",		"CLIPCURSOR",		"NEEDADMINCAPS",
	"HOOKDI",			"MODIFYMOUSE",		"HANDLEEXCEPTIONS", "SAVELOAD",
	"HOOKXINPUT",		"HOOKDI8",			"BLITFROMBACKBUFFER","SUPPRESSCLIPPING",
	"AUTOREFRESH",		"FIXWINFRAME",		"HIDEHWCURSOR",		"SLOWDOWN",
	"DISABLECLIPPING",	"LOCKWINSTYLE",		"MAPGDITOPRIMARY",	"FIXTEXTOUT",
	"TRIMMOUSEPOSITION", "USERGB565",		"SUPPRESSDXERRORS", "PREVENTMAXIMIZE",
	"LOCK24BITDEPTH",	"FIXPARENTWIN",		"SWITCHVIDEOMEMORY","CLIENTREMAPPING",
	"HANDLEALTF4",		"LOCKWINPOS",		"HOOKCHILDWIN",		"MESSAGEPROC"
	},{
	// Flags2
	"RECOVERSCREENMODE","REFRESHONRESIZE",	"BACKBUFATTACH",	"MODALSTYLE",
	"KEEPASPECTRATIO",  "INIT8BPP",			"FORCEWINRESIZE",	"INIT16BPP",
	"KEEPCURSORFIXED",  "DISABLEGAMMARAMP",	"INDEPENDENTREFRESH","FIXNCHITTEST",
	"LIMITFPS",			"SKIPFPS",			"SHOWFPS",			"HIDEMULTIMONITOR",
	"TIMESTRETCH",		"HOOKOPENGL",		"LOCKEDSIZE",		"SHOWHWCURSOR",
	"GDISTRETCHED",		"SHOWFPSOVERLAY",	"FAKEVERSION",		"PALDIBEMULATION",
	"NOPALETTEUPDATE",	"SUPPRESSIME",		"NOBANNER",			"WINDOWIZE",
	"LIMITRESOURCES",	"STARTDEBUG",		"SETCOMPATIBILITY", "WIREFRAME",
	},{
	// Flags3
	"FORCEHOOKOPENGL",	"REFRESHONREALIZE",	"HOOKDLLS",			"SUPPRESSD3DEXT",
	"HOOKENABLED",		"FIXD3DFRAME",		"FORCE16BPP",		"BLACKWHITE",
	"SKIPIATHINT",		"SINGLEPROCAFFINITY","EMULATEREGISTRY", "CDROMDRIVETYPE",
	"NOWINDOWMOVE",		"FORCECLIPPER",		"LOCKSYSCOLORS",	"GDIEMULATEDC",
	"FORCEWINDOWING",	"FONTBYPASS",		"MINIMALCAPS",		"DEFAULTMESSAGES",
	"BUFFEREDIOFIX",	"FILTERMESSAGES",	"PEEKALLMESSAGES",	"SETZBUFFER16BIT",
	"SETZBUFFER24BIT",	"FORCESHEL",		"SKIPFREELIBRARY",	"COLORFIX",
	"FULLPAINTRECT",	"FIXALTEREDPATH",	"LOCKFPSCORNER",	"NOPIXELFORMAT",
	},{
	// Flags4
	"NOALPHACHANNEL",	"SUPPRESSCHILD",	"FIXREFCOUNTER",	"SHOWTIMESTRETCH",
	"ZBUFFERCLEAN",		"ZBUFFER0CLEAN",	"FORCECLIPCHILDREN","DISABLEFOGGING",
	"NOPOWER2FIX",		"NOPERFCOUNTER",	"PREVENTMINIMIZE",	"INTERCEPTRDTSC",
	"---",				"NOFILLRECT",		"HOOKGLIDE",		"HIDEDESKTOP",
	"STRETCHTIMERS",	"FULLFLIPEMULATION","NOTEXTURES",		"RETURNNULLREF",
	"FINETIMING",		"NATIVERES",		"SUPPORTSVGA",		"SUPPORTHDTV",
	"RELEASEMOUSE",		"ENABLETIMEFREEZE", "HOTPATCH",			"ENABLEHOTKEYS",
	"IGNOREDEBOUTPUT",	"NOD3DRESET",		"OVERRIDEREGISTRY", "HIDECDROMEMPTY",
	},{
	// Flags5
	"DIABLOTWEAK",		"CLEARTARGET",		"NOWINPOSCHANGES",	"MAXCLIPPER", 
	"LIMITBEGINSCENE",	"USELASTCORE",		"SWALLOWMOUSEMOVE",	"AEROBOOST",
	"QUARTERBLT",		"NOIMAGEHLP",		"BILINEARFILTER",	"REPLACEPRIVOPS",
	"REMAPMCI",			"TEXTUREHIGHLIGHT",	"TEXTUREDUMP",		"TEXTUREHACK",
	"TEXTURETRANSP",	"NORMALIZEPERFCOUNT","DISABLEMMX",		"NOACCESSIBILITY",
	"INJECTSON",		"DEBUGSON",			"DISABLEALTTAB",	"HOOKBINKW32",
	"GLOBALFOCUSON",	"GLOBALFOCUSOFF",	"MESSAGEPUMP",		"TEXTUREFORMAT", 
	"PUSHACTIVEMOVIE",	"LOCKRESERVEDPALETTE","UNLOCKZORDER",	"EASPORTSHACK",
	},{
	// Flags6
	"FORCESWAPEFFECT",	"LEGACYALLOC",		"NODESTROYWINDOW",	"NOMOVIES",
	"SUPPRESSRELEASE",	"FIXMOVIESCOLOR",	"WOW64REGISTRY",	"DISABLEMAXWINMODE",
	"FIXPITCH",			"POWER2WIDTH",		"HIDETASKBAR",		"ACTIVATEAPP",
	"NOSYSMEMPRIMARY",	"NOSYSMEMBACKBUF",	"CONFIRMONCLOSE",	"TERMINATEONCLOSE",
	"HALFFLIPEMULATION","SETZBUFFERBITDEPTHS", "SHAREDDC",		"WOW32REGISTRY",
	"STRETCHMOVIES",	"BYPASSMCI",		"FIXPIXELZOOM",		"SCALERELMOUSE",
	"CREATEDESKTOP",	"FRONTEND",			"SYNCPALETTE",		"VIRTUALJOYSTICK",
	"UNACQUIRE",		"HOOKGOGLIBS",		"BYPASSGOGLIBS",	"EMULATERELMOUSE",
	},{
	// Flags7
	"LIMITDDRAW",		"NODISABLEALTTAB",	"FIXCLIPPERAREA",	"HOOKDIRECTSOUND",
	"HOOKSMACKW32",		"BLOCKPRIORITYCLASS","CPUSLOWDOWN",		"CPUMAXUSAGE",
	"NOWINERRORS",		"SUPPRESSOVERLAY",	"INIT24BPP",		"INIT32BPP",
	"FIXGLOBALUNLOCK",	"SHOWHINTS",		"SKIPDEVTYPEHID",	"INJECTSUSPENDED",
	"SUPPRESSDIERRORS", "HOOKNORUN",		"FIXBINDTEXTURE",	"ENUM16BITMODES",
	"SHAREDKEYBOARD",	"HOOKDOUPDATE",		"HOOKGLUT32",		"INITIALRES",
	"MAXIMUMRES",		"LOCKCOLORDEPTH",	"FIXSMACKLOOP",		"FIXFREELIBRARY",
	"ANCHORED",			"CLEARTEXTUREFOURCC","NODDEXCLUSIVEMODE","CLEARSHIMS",
	},{
	// Flags8
	"FORCEWAIT",		"FORCENOWAIT",		"FORCEVSYNC",		"FORCENOVSYNC",
	"VSYNCSCANLINES",	"TRIMTEXTUREFORMATS","NOHALDEVICE",		"CLIPLOCK",
	"PRETENDVISIBLE",	"RAWFORMAT",		"WININSULATION",	"FIXMOUSEHOOK",
	"DDSFORMAT",		"HOOKWING32",		"FIXAUDIOPCM",		"D3D8BACK16",
	"VIRTUALCDAUDIO",	"DYNAMICZCLEAN",	"FORCETRACKREPEAT",	"IGNOREMCIDEVID",
	"LOADGAMMARAMP",	"QUALITYFONTS",		"ALLOWSYSMEMON3DDEV","CLIPMENU",
	"BACKGROUNDPRIORITY","OFFSCREENZBUFFER","VIRTUALHEAP",		"ZBUFFERHARDCLEAN",
	"LOADLIBRARYERR",	"SHAREDDCHYBRID",	"FIXADJUSTWINRECT", "HOOKDLGWIN",
	},{
	// Flags9
	"FIXTHINFRAME",		"NOMOUSEEVENTS",	"IATWORDALIGNED",	"IATBYTEALIGNED",
	"WRITEON000B0000",	"NODIALOGS",		"SAFEPALETTEUSAGE", "LOCKFULLSCREENCOOP",
	"NOBAADFOOD",		"HOTREGISTRY",		"NOIATSCAN",		"HOOKSDLLIB",
	"SDLEMULATION",		"HIDEJOYSTICKS",	"HOOKSDL2LIB",		"SDLFORCESTRETCH",
	"MOUSEMOVEBYEVENT", "D3DRESOLUTIONHACK","FIXAILSOUNDLOCKS", "LOCKTOPZORDER",
	"EMULATEMAXIMIZE",	"MAKEWINVISIBLE",	"FIXEMPIREOFS",		"SCALEGLBITMAPS",
	"HOOKWGLCONTEXT",	"NOTASKBAROVERLAP", "CACHED3DSESSION",	"SLOWSCROLLING",
	"KILLBLACKWIN",		"ZERODISPLAYCOUNTER","SOUNDMUTE",		"LOCKVOLUME",	
	},{
	// Flags10
	"FORCEHWVERTEXPROC","FORCESWVERTEXPROC","FORCEMXVERTEXPROC","PRECISETIMING",
	"REPLACEDIALOGS",	"FAKEHDDRIVE",		"FAKECDDRIVE",		"LIGHTGAMMARAMP",
	"FORCED3DGAMMARAMP","HANDLEFOURCC",		"SUSPENDTIMESTRETCH","SLOWWINPOLLING",
	"NOOLEINITIALIZE",	"HWFOURCC",			"SWFOURCC",			"LIMITDIBOPERATIONS",
	"FIXMOUSERAWINPUT", "SETCDVOLUME",		"CUSTOMRES",		"CHAOSOVERLORDSFIX",
	"FIXFOLDERPATHS",	"NOCOMPLEXMIPMAPS", "CDROMPRESENT",		"SUPPRESSFOURCCBLT",
	"INVALIDATECLIENT", "MOUSESHIELD",		"CREATEDCHOOK",		"NOZBUFATTACH",
	"SAFEPRIMLOCK",		"PEFILEHOOK",		"HIDEWINDOWCHANGES","STRETCHDIALOGS",
	},{
	// Flags11
	"EXTENDSDLHOOK",	"D3D8MAXWINMODE",	"VIRTUALPROCHEAP",	"MUTEX4CRITSECTION",
	"DELAYCRITSECTION",	"HACKMCIFRAMES",	"UNNOTIFYINACTIVE",	"REMAPNUMKEYPAD",
	"SETUSKEYDESCR",	"HOOKEARSOUND",		"CDPAUSECAPABILITY","ADAPTIVERATIO",
	"REMAPSYSFOLDERS",	"SCALEMAINVIEWPORT","FORCESHAL",		"FORCESNULL",
	"USESHORTPATH",		"INVERTMOUSEXAXIS",	"INVERTMOUSEYAXIS",	"SMACKBUFFERNODEPTH",
	"LOCKSYSSETTINGS",	"INVALIDATEFULLRECT","FIXMESSAGEHOOK",	"NODISABLEPRINT",
	"SHRINKFONTWIDTH",	"SAFEMIDIOUT",		"MERGEMULTIPLECD",	"SAFEALLOCS",
	"FIXASYNCKEYSTATE",	"TRANSFORMANDLIGHT","CUSTOMLOCALE",		"FIXDEFAULTMCIID",
	},{
	// Flags12
	"LOCKCDTRAY",		"LOCKGLVIEWPORT",	"NOTNLDEVICE",		"CLASSLOCALE",
	"PROJECTBUFFER",	"COMMITPAGE",		"SETCMDLINE",		"FORCERELAXIS",
	"FORCEABSAXIS",		"LIMITFLIPONLY",	"DIRECTXREPLACE",	"W98OPAQUEFONT",
	"SUPPRESSGLIDE",	"NOSETPIXELFORMAT",	"GLEXTENSIONSLIE",	"FIXMOUSELPARAM",
	"FAKEGLOBALATOM",	"KILLVSYNC",		"STRETCHPERFREQUENCY","INJECTPROXY",
	"DSINITVOLUME",		"SCALECBTHOOK",		"REVERTDIBPALETTE",	"FIXDCALREADYCREATED",
	"SUPPRESSMENUS",	"ADAPTMOUSESPEED",	"KILLDEADLOCKS",	"BLUREFFECT",
	"TEXTUREPALETTE",	"DISABLEDWM",		"SUPPRESSCDAUDIO",	"PATHLOCALE",
	},{
	// Flags13
	"GLFIXCLAMP",		"FORCECOLORKEYOFF",	"SPONGEBOBHACK",	"DSOUNDREPLACE",
	"EMULATECDMIXER",	"PLAYSOUNDFIX",		"FIGHTINGFORCEFIX",	"EMULATECDAUX",
	"HIDEMUTECONTROLS",	"DXVERSIONLIE",		"CENTERDIALOGS",	"MIDIAUTOREPAIR",
	"LEGACYBASEUNITS",	"PATCHMSVBVM",		"XRAYTWEAK",		"FIXDCCOLORDEPTH",
	"DISABLED3DMMX",	"DISABLED3DXPSGP",	"DISABLEPSGP",		"D3DXDONOTMUTE",
	"DISABLEDEP",		"GLEXTENSIONSTRIM",	"FIXASPECTRATIO",	"CONTROLFOGGING",
	"CONTROLLIGHTS",	"USENANOSLEEP",		"CUSTOMTIMESHIFT",	"DEFUSEFLASHBOMB",
	"LOCKALLWINDOWS",	"SHAREDHOOK",		"BYPASSWAVEOUTPOS",	"IGNOREFSYSERRORS",
	},{
	// Flags14
	"NOSETTEXT",		"STRETCHFULLRECT",	"FLUSHKEYSTATE",	"LIMITFREQUENCY",
	"HEAPPADALLOCATION","DISABLECOLORKEY",	"NOSHELLEXECUTE",	"SAMPLE2COVERAGE",
	"LOCKCURSORICONS",	"SETCDAUDIOPATH",	"UPTIMECLEAR",		"UPTIMESTRESS",
	"NORAMPDEVICE",		"NORGBDEVICE",		"NOMMXDEVICE",		"FIXKEYBOARDTYPE",
	"STOPSOUND",		"NOSOUNDLOOP",		"CUSTOMPERFCOUNT",	"MIDIOUTBYPASS",
	"FIXEDBITBLT",		"HEAPLEAK",			"MW2WAVEOUTFIX",	"SAFEHEAP",
	"WIN16CREATEFILE",	"WIN16FINDFILEFIX",	"SLOWCDSTATUS",		"WAVEOUTUSEPREFDEV",
	"FIXMACROMEDIAREG",	"LOCKSYSTEMMENU",	"STICKYWINDOWS",	"DUMPTEXT",
	},{
	// Flags15
	"EMULATEWIN9XMCI",	"EMULATEFLOPPYDRIVE","PLAYFROMCD",		"CDAUTOMATICRIP",
	"EMULATEVISTAMCI",	"SETPRIORITYLOW",	"FASTPRIMARYUPDATE","LIMITFREEDISK",
	"LIMITFREERAM",		"LIMITVIDEORAM",	"LIMITTEXTURERAM",	"DEFERREDWINPOS",
	"DISPELTWEAK",		"ASYNCBLITMODE",	"FIXFILEDIALOG",	"EMULATEVOLUME",
	"HOOKRECOVER",		"MIDISETINSTRUMENT","IGNORESCHEDULER",	"SLOWDOWNEXCEPTIONS",
	"DUPLICATEHANDLEFIX","EMULATEWIN95",	"IGNOREVBOVERFLOW",	"INJECTAPC",
	"DISABLESTICKYKEYS","HOTPATCHALWAYS",	"FORCEHALFTONE",	"FORCEHALFTONETINY",
	"FIXFULLWINSTYLE",	"HIDEDISPLAYMODES",	"GDIASYNCDC",		"CULLMODENONE",
	},{
	// Flags16
	"RAMP2RGBDEVICE",	"RAMP2MMXDEVICE",	"NOSURFACENEW",		"EMULATEWIN9XHEAP",
	"FORCED3DREFDEVICE","FOGVERTEXCAP",		"FOGTABLECAP",		"SETFOGCOLOR",
	"COLORKEYTOALPHA",	"FIXCREATEFILEMAP",	"EMUGETCOMMANDLINE","FAKEDATE",
	"MCINOTIFYTOPMOST",	"FORCESIMPLEWINDOW","CONFIRMESCAPE",	"FORCEWBASEDFOG",
	"FORCED3D9ON12",	"HOOKVIDEOADAPTER",	"LOCKSYSCURSORICONS","RECOVERSYSCURSORICONS",
	"FIXTNLRHW",		"EMULATEOVERLAY",	"JOYSTICKEFFECTS",	"TRANSPARENTOVERLAY",
	"MAPMEMB0000",		"RUNBYDXWND",		"TRIMCHILDWINDOWS",	"NATIVEDIALOGS",
	"TRIMMAINWINDOW",	"FORCENOPS",		"FULLRECTBLT",		"FIXDCPALETTE",
	},{
	// Flags17
	"FIXAILBUG",		"EMULATEWINHELP",	"LOCKCURSORSHAPE",	"MERGEFRONTBUFFERS",	
	"FORCEDEVTYPE1",	"FORCEDEVTYPE2",	"FORCEDEVTYPE3",	"FIXOVERLAPPEDRESULT",	
	"FORCEZBUFFERON",	"FORCEZBUFFEROFF",	"FORCEDITHERING",	"CLEARDITHERING",	
	"PATCHEXECUTEBUFFER","POSSIBLYPROTECTED","NOMULTISAMPLE",	"RECOVEREXCLUSIVEERR",	
	"FORCEVBUFONSYSMEM","FORCEFILTERNEAREST","LOCKPIXELFORMAT",	"FIXMOUSEBIASX",	
	"FIXMOUSEBIASY",	"ZBUFONSYSMEMORY",	"SUPPRESSDEP",		"FAKEPENTIUM3",	
	"HIDEDIRECTDRAW",	"COMPENSATEOGLCOPY","CDHACK",			"HIDECDROMREAL",	
	"SHAREDMOUSE",		"XBOX2KEYBOARD",	"WINAUTOREPAINT",	"HOOKDIRECTSHOW",	
	},{
	// Flags18
	"PACKMOUSEDATA",	"BYPASSDSOUND",		"REPAIRHEAP",		"SAFEDISCSHIM",	
	"EMUDDSYNCSHIM",	"RESETMULTIVOLUME",	"TRIMVERTEXBUFFER",	"ENABLEZOOMING",	
	"NOSCISSORTEST",	"SDLFIXMOUSE",		"FORCEDX1HOOK",		"ENABLEPATCHER",	
	"SURFACEUNLOCK",	"PLAINBACKBUFFER",	"EMULATESOUNDPAN",	"FILTERRGBTEXTURES",
	"NOD3DOPTIMIZE",	"LOCKNORMALCOOP",	"LOCKEXCLUSIVECOOP","ALTPIXELCENTER",	
	"SCALEDIRECTSHOW",	"IGNOREADDFILTER",	"SUPPRESSIVMR",		"IGNOREGRAPHERRORS",	
	"FILTERCOLORKEY",	"FORCECOLORKEY",	"COLORKEY2PALETTE",	"NOLAVFILTERS",	
	"FIXCOLORKEY",		"DEPTHBUFZCLEAN",	"STRETCHBACKBUFFER","BYPASSACTIVEMOVIE",	
	},{
	// Flags19
	"NOWAVEOUT",		"EMULATECLIPPER",	"WDDMNOOVERLAY",	"TRIMD3DPOINTS",	
	"HOOKLEGACYEVENTS",	"HOOKLEGACYWAVE",	"LEGACYKERNEL32",	"STARTDETOURS",	
	"DETOURSSON",		"FIXDSOUNDBUFFER",	"HEAPZEROMEMORY",	"DDRAWLEGACYCALLBACK",	
	"ASYNCWAVEOUTOPEN",	"ASYNCWAVEOUTCLOSE","ISWIN16EXECUTABLE","ISDOSEXECUTABLE",	
	"TRANSPARENTDIALOG","EMULATEWINRESIZE",	"DISABLELAYEREDWIN","SCALECHILDWIN",	
	"TRIMTEXTYPOS",		"FORCEWBASEDFOGWDDM","USEBLTFAST",		"NODIRECTSOUND",	
	"FORCEEMULDRIVER",	"DEFMOVIESCOLOR",	"BESTMOVIESCOLOR",	"SETRESOLUTION",	
	"FIXBITMAPCOLOR",	"DIBPALETTE",		"REMAPROOTFOLDER",	"ZBUFONVIDMEMORY",	
	},{
	// Flags20
	"CENTERONEXIT",		"HANDLECDLOCK",		"DISABLEGHOSTING",	"MINIMUMRES",	
	"ALTERNATEVRETRACE","SUPPRESSBLTFX",	"EMULATEXMIRRORING","EMULATEYMIRRORING",	
	"GETALLMESSAGES",	"",					"",					"",	
	"",					"",					"",					"",	
	"",					"",					"",					"",	
	"",					"",					"",					"",	
	"",					"",					"",					"",	
	"",					"",					"",					"",	
	},{
	// TFlags
	"OUTTRACE",			"OUTALLERRORS",		"",					"",
	"OUTSEPARATED",		"OUTCIRCULAR",		"ASSERTDIALOG",		"",
	"LOGDEBUG",			"",					"",					"",
	"",					"",					"",					"OUTHEXTRACE",
	"",					"",					"",					"",		
	"",					"",					"",					"",		
	"LOGTRACE",			"LOGERRORS",		"OUTONTEMPFOLDER",	"ADDRELATIVETIME",
	"ADDTHREADID",		"ADDTIMESTAMP",		"OUTDEBUGSTRING",	"ERASELOGFILE", 
	},{
	// TFlags2
	"",					"OUTDDRAWTRACE",	"OUTWINMESSAGES",	"OUTCURSORTRACE",
	"",					"",					"",					"OUTIMPORTTABLE",
	"",					"OUTREGISTRY",		"TRACEHOOKS",		"OUTD3DTRACE",
	"OUTDXWINTRACE",	"OUTWINGTRACE",		"OUTOGLTRACE",		"OUTCOMTRACE",
	"OUTSDLTRACE",		"OUTTIMETRACE",		"OUTSOUNDTRACE",	"OUTINPUTS",		
	"OUTSYSLIBS",		"OUTLOCALE",		"OUTFPS",			"OUTFILEIO",
	"OUTGDI",			"",					"",					"",		
	"",					"",					"",					"",		
	},{
	// DFlags
	"CPUIDBIT1",		"CPUIDBIT2",		"CPUIDBIT3",		"CPUDISABLECPUID",
	"CPUDISABLEMMX",	"CPUDISABLESSE",	"CPUDISABLESSE2",	"CAPTURESCREENS",
	"DUMPDIBSECTION",	"DUMPDEVCONTEXT",	"DUMPCPUID",		"MARKBLIT",
	"MARKLOCK",			"MARKWING32",		"MARKGDI32",		"DOFASTBLT",
	"CENTERTOWIN",		"DUMPSURFACES",		"NODDRAWBLT",		"NODDRAWFLIP",
	"NOGDIBLT",			"STRESSRESOURCES",	"CAPMASK",			"DISABLEDELETE",
	"ZBUFFERALWAYS",	"MARKCLIPRGN",		"FREEZEINJECTEDSON","STARTWITHTOGGLE",
	"CAPTURESCREENS",	"DUMPDSHOWGRAPH",	"",					"", 
	},{
	// Dflags2
	"NOWINDOWHOOKS",	"FIXRANDOMPALETTE",	"DISABLEWINHOOKS",	"DUMPBITMAPS",
	"DUMPBLITSRC",		"",					"DISABLEWINHOOK",	"FORCED3DCHECKOK",
	"",					"",					"",					"",							
	"",					"",					"",					"",
	"",					"",					"",					"",
	"",					"",					"",					"",
	"EXPERIMENTAL",		"EXPERIMENTAL2",	"EXPERIMENTAL3",	"EXPERIMENTAL4",
	"EXPERIMENTAL5",	"EXPERIMENTAL6",	"EXPERIMENTAL7",	"EXPERIMENTAL8",
	}};

LPCSTR GetFlagCaption(int flag, int bit)
{
	//if((flag<0) || (flag>(9*32))) return "";
	//return FlagNames[flag >> 5][flag & 0x1F];
	if((flag<0) || (flag>23)) return "";
	if((bit<0) || (bit>31)) return "";
	return FlagNames[flag][bit];
}
#else
LPCSTR GetFlagCaption(int flag, int bit)
{
	return "";
}
#endif // DXW_NOTRACES