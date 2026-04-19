#define _CRT_SECURE_NO_WARNINGS
#define SYSLIBNAMES_DEFINES

#define _COMPONENT "dxwCore"

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhelper.h"
#include "resource.h"
#include "hddraw.h"
#include "d3d9.h"
#include "dwmapi.h"
extern GetDC_Type pGetDCMethod();
extern ReleaseDC_Type pReleaseDC1;
extern HandleDDThreadLock_Type pReleaseDDThreadLock;
typedef DWORD (WINAPI *GetCurrentDirectoryA_Type)(DWORD, LPSTR);
extern GetCurrentDirectoryA_Type pGetCurrentDirectoryA;

/* ------------------------------------------------------------------ */
// External dependencies
/* ------------------------------------------------------------------ */

extern Blt_Type pBltMethod();
extern int iPrimarySurfaceVersion;

/* ------------------------------------------------------------------ */
// Internal function pointers
/* ------------------------------------------------------------------ */

typedef DWORD (*TimeShifter_Type)(DWORD, int);
typedef LARGE_INTEGER (*TimeShifter64_Type)(LARGE_INTEGER, int);


/* ------------------------------------------------------------------ */
// Constructor, destructor, initialization....
/* ------------------------------------------------------------------ */

dxwCore::dxwCore()
{
	// initialization stuff ....
	FullScreen=FALSE;
	SethWnd(NULL);
	SetScreenSize();
	hParentWnd = 0;
	hChildWnd = 0;
	bActive = TRUE;
	bDInputAbs = 0;
	TimeShift = 0;
	ResetEmulatedDC();
	MustShowOverlay=FALSE;
	TimerEvent.dwTimerType = TIMER_TYPE_NONE;
	// initialization of default vsync emulation array
	iRefreshDelays[0]=16;
	iRefreshDelays[1]=17;
	iRefreshDelayCount=2;
	TimeFreeze = FALSE;
	dwScreenWidth = 0;
	dwScreenHeight = 0;
	WindowStyle = WSTYLE_DEFAULT;
	SavedTimeShift = 0;
	TimeShiftStatus = TIMESHIFT_ON;
	TimeShiftRedeemTime = 0;
	DIBRGBQuadEntries = NULL;
	bEnableGammaControl = FALSE;
	dwCurrentFolderType = DXW_NO_FAKE;
	dwFindFileMapping = DXW_NO_FAKE;
	bIsRootFolder = FALSE;
	VirtualCDAudioDeviceId = 0xBEEF; // impossible id
	VirtualCDAuxDeviceId = 0x1; // second device after the mixer
	bIsSelectClipRgnRecursed = FALSE;
	lpFakePrimaryMonitorGUID = NULL;
	lpZombieSurface = NULL;
	trimmedGLExt = NULL;
	cdAudioPath = NULL;
	AddedTimeInMS.QuadPart = 0;
	AddedTimeInQPCTicks.QuadPart = 0;
	hStickyWindow = 0;
	hWndMenu = 0;
	hControlParentWnd = 0;
	CDADrive = '?';
	EmulateWin95 = FALSE;
	dwTransparentColor = 0xFFFFFFFF;
	hTable ht;
}

dxwCore::~dxwCore()
{
}

char * dxwCore::RectString(CONST RECT *r)
{
	static char s[40];
	if(r){
		sprintf(s, "(%d,%d)-(%d,%d)", r->left, r->top, r->right, r->bottom);
	}
	else{
		strcpy(s, "NULL");
	}
	return s;
}

void dxwCore::InitTextureLimits()
{
	ApiName("InitTextureLimits");
	char sPath[MAX_PATH];
	sprintf_s(sPath, MAX_PATH, "%s\\dxwnd.ini", GetDxWndPath());
	MinTexX=(*pGetPrivateProfileIntA)("Texture", "MinTexX", 0, sPath);
	MaxTexX=(*pGetPrivateProfileIntA)("Texture", "MaxTexX", 0, sPath);
	MinTexY=(*pGetPrivateProfileIntA)("Texture", "MinTexY", 0, sPath);
	MaxTexY=(*pGetPrivateProfileIntA)("Texture", "MaxTexY", 0, sPath);
	iTextureFileFormat = FORMAT_BMP;
	if(dxw.dwFlags8 & RAWFORMAT) iTextureFileFormat = FORMAT_RAW;
	if(dxw.dwFlags8 & DDSFORMAT) iTextureFileFormat = FORMAT_DDS;
	OutTrace("%s: size min=(%dx%d) max=(%dx%d)\n", ApiRef, MinTexX, MinTexY, MaxTexX, MaxTexY);
	sprintf_s(sPath, MAX_PATH, "%s\\texture.out", GetDxWndPath());
	CreateDirectory(sPath, NULL);
}

BOOL dxwCore::VerifyTextureLimits(int w, int h)
{
	static BOOL DoOnce = TRUE;
	if(DoOnce){
		dxw.InitTextureLimits();
		DoOnce = FALSE;
	}
	if((MinTexX && (w<MinTexX)) || (MinTexY && (h<MinTexY))) {
		return FALSE;
	}
	if((MaxTexX && (w>MaxTexX)) || (MaxTexY && (h>MaxTexY))) {
		return FALSE;
	}
	if(h==1){
		return FALSE;
	}
	return TRUE;
}

void dxwCore::SetFullScreen(BOOL fs) 
{
	ApiName("SetFullScreen");
	if(dxw.dwFlags3 & FORCEWINDOWING) fs=TRUE;
	OutTraceDW("%s: %s\n", ApiRef, fs?"FULLSCREEN":"WINDOWED");
	FullScreen=fs;
}

void dxwCore::ClearFullScreen() 
{
	ApiName("ClearFullScreen");
	OutTraceDW("%s: WINDOWED\n", ApiRef);
	FullScreen=FALSE;
}

BOOL dxwCore::IsFullScreen()
{
	return (Windowize && FullScreen);
}

BOOL dxwCore::IsToRemap(HDC hdc)
{
	if(!hdc) return TRUE;
	return (Windowize && FullScreen && (OBJ_DC == (*pGetObjectType)(hdc)));
}

void dxwCore::InitScreenResolutions()
{
	// Define the supported virual resolutions:
	extern SupportedRes_Type SupportedSVGARes[];
	extern SupportedRes_Type SupportedHDTVRes[];

	// v2.02.96	// 1) Build the array of all supported resolutions
	int j = 0;
	//if(dwFlags15 & HIDEDISPLAYMODES) ...
	if(dwFlags4 & SUPPORTSVGA) for(int i=0; SupportedSVGARes[i].w; i++, j++) SupportedRes[j] = SupportedSVGARes[i];
	if(dwFlags4 & SUPPORTHDTV) for(int i=0; SupportedHDTVRes[i].w; i++, j++) SupportedRes[j] = SupportedHDTVRes[i];
	if(dwFlags10 & CUSTOMRES){
		SupportedRes[j].w = iMaxW;
		SupportedRes[j].h = iMaxH;
		j++;
	}
	// 2) add a list terminator
	SupportedRes[j].w = 0;
	SupportedRes[j].h = 0;
	// 3) bubble sort
	for(BOOL ToSort = TRUE; ToSort; ){
		ToSort = FALSE;
		for(int i=1; SupportedRes[i].w; i++){
			if((SupportedRes[i].w*SupportedRes[i].h) < (SupportedRes[i-1].w*SupportedRes[i-1].h)){
				// bubble sort
				SupportedRes_Type res = SupportedRes[i];
				SupportedRes[i] = SupportedRes[i-1];
				SupportedRes[i-1] = res;
				ToSort = TRUE;
			}
		}
	}
	// 4) find limitations
#define HUGE 100000
	DWORD maxw, maxh;
	maxw=HUGE; maxh=HUGE; 
	DWORD minw, minh;
	minw=0; minh=0;
	if(dwFlags7 & MAXIMUMRES){
		if(iMaxW < maxw) maxw = iMaxW;
		if(iMaxH < maxh) maxh = iMaxH;
	}
	if(dwFlags20 & MINIMUMRES){
		if(minw < iMaxW) minw = iMaxW;
		if(minh < iMaxH) minh = iMaxH;
	}
	if(dwFlags15 & HIDEDISPLAYMODES){
		if(1024 < maxw) maxw = 1024;
		if(768 < maxh) maxh = 768;
		minw = 640;
		minh = 480;
	}
	if(maxw != HUGE){
		for(int i=0; SupportedRes[i].w; i++){
			if((SupportedRes[i].w > maxw) || (SupportedRes[i].h > maxh)){
				// place a new list terminator here ...
				SupportedRes[i].w = 0;
				SupportedRes[i].h = 0;
			}
		}
	}
	if(minw != 0){
		// shift to delete the low-res entries
		int j = 0;
		for(int i=0; SupportedRes[i].w; i++){
			if((SupportedRes[i].w >= minw) || (SupportedRes[i].h >= minh)){
				// place a new list terminator here ...
				SupportedRes[j].w = SupportedRes[i].w;
				SupportedRes[j].h = SupportedRes[i].h;
				SupportedRes[i].w = 0;
				SupportedRes[i].h = 0;
				j++;
			}
		}
	}
	// v2.05.88: if trimmed and greater, recover the custom res value.
	if((dwFlags10 & CUSTOMRES) && ((iMaxW > maxw) || (iMaxH > maxh))){
		int i;
		for(i=0; SupportedRes[i].w; i++); // find a free slot
		SupportedRes[i].w = iMaxW;
		SupportedRes[i].h = iMaxH;
		SupportedRes[i+1].w = 0;
		SupportedRes[i+1].h = 0;
	}
}

void dxwCore::InitializeMixer()
{
	ApiName("InitializeMixer");
	HMIXER        mixerHandle;
	MIXERCAPS     mixcaps;
	MIXERLINE     mixerline;
	MMRESULT	  err;	
	MIXERLINECONTROLS mixerLineControls;
	MIXERCONTROLDETAILS_UNSIGNED mixerVolume;
	MIXERCONTROLDETAILS_BOOLEAN mixerMute;
	HMODULE hmod;

	typedef MMRESULT (WINAPI *mixerOpen_Type)(LPHMIXER, UINT, DWORD_PTR, DWORD_PTR, DWORD);
	mixerOpen_Type pmixerOpen;
	typedef MMRESULT (WINAPI *mixerClose_Type)(HMIXER);
	mixerClose_Type pmixerClose;
	typedef MMRESULT (WINAPI *mixerGetDevCapsA_Type)(UINT_PTR, LPMIXERCAPSA, UINT);
	mixerGetDevCapsA_Type pmixerGetDevCapsA;
	typedef UINT (WINAPI *mixerGetNumDevs_Type)(void);
	mixerGetNumDevs_Type pmixerGetNumDevs;
	typedef MMRESULT (WINAPI *mixerGetLineInfoA_Type)(HMIXEROBJ, LPMIXERLINEA, DWORD);
	mixerGetLineInfoA_Type pmixerGetLineInfoA;
	typedef MMRESULT (WINAPI *mixerGetLineControlsA_Type)(HMIXEROBJ, LPMIXERLINECONTROLS, DWORD);
	mixerGetLineControlsA_Type pmixerGetLineControlsA;

	__try{
		OutTrace("%s\n", ApiRef);
		// defaults for Win7+
		MixerCDMuteID = 0x3;
		MixerCDVolumeID = 0x4;

		hmod=(*pLoadLibraryA)("winmm.dll");
		if(!hmod) return;
		pmixerOpen=(mixerOpen_Type)(*pGetProcAddress)(hmod,"mixerOpen");
		pmixerClose=(mixerClose_Type)(*pGetProcAddress)(hmod,"mixerClose");
		pmixerGetDevCapsA=(mixerGetDevCapsA_Type)(*pGetProcAddress)(hmod,"mixerGetDevCapsA");
		pmixerGetNumDevs=(mixerGetNumDevs_Type)(*pGetProcAddress)(hmod,"mixerGetNumDevs");
		pmixerGetLineInfoA=(mixerGetLineInfoA_Type)(*pGetProcAddress)(hmod,"mixerGetLineInfoA");
		pmixerGetLineControlsA=(mixerGetLineControlsA_Type)(*pGetProcAddress)(hmod,"mixerGetLineControlsA");
		FreeLibrary(hmod);
		if(!(pmixerOpen && pmixerClose && pmixerGetDevCapsA && pmixerGetNumDevs && pmixerGetLineInfoA && pmixerGetLineControlsA)) {
			OutTrace("%s: api missing: Open=%#x Close=%#x GetDevCaps=%#x GetNumDevs=%#x GetLineInfo=%#x GetLineControls=%#x\n", 
				ApiRef, pmixerOpen, pmixerClose, pmixerGetDevCapsA, pmixerGetNumDevs, pmixerGetLineInfoA, pmixerGetLineControlsA);
			return;
		}

		UINT mixCount = (*pmixerGetNumDevs)();
		if(mixCount < 1) {
			OutTrace("%s: no mixer devices\n", ApiRef);
			return; // no mixers ??
		}
		err = (*pmixerGetDevCapsA)(0, &mixcaps, sizeof(MIXERCAPS));
		if(err) {
			OutTrace("%s: mixerGetDevCaps ERROR err=%d\n", ApiRef, GetLastError());
			return;
		}
		err = (*pmixerOpen)(&mixerHandle, 0, 0, 0, MIXER_OBJECTF_MIXER);
		if(err) {
			OutTrace("%s: mixerOpen ERROR err=%d\n", ApiRef, GetLastError());
			return;
		}
		// Get info about a "CD" type of source line 
		memset(&mixerline, 0, sizeof(MIXERLINE));
		mixerline.cbStruct = sizeof(MIXERLINE);
		mixerline.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; 
		err = (*pmixerGetLineInfoA)((HMIXEROBJ)mixerHandle, &mixerline, MIXER_GETLINEINFOF_COMPONENTTYPE);
		if(err) {
			OutTrace("%s: mixerGetLineInfo ERROR err=%d\n", ApiRef, GetLastError());
			return;
		}
		OutTrace("%s: Mixer CD controls: LineID=%d\n", ApiRef, mixerline.dwLineID);
		memset(&mixerLineControls, 0, sizeof(MIXERLINECONTROLS));
		mixerLineControls.cbStruct = sizeof(MIXERLINECONTROLS);
		mixerLineControls.cControls = 1;
		mixerLineControls.dwLineID = mixerline.dwLineID;
		mixerLineControls.pamxctrl = (LPMIXERCONTROLA)&mixerVolume;
		mixerLineControls.cbmxctrl = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
		mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
		err = (*pmixerGetLineControlsA)((HMIXEROBJ)mixerHandle, &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
		if(!err) MixerCDVolumeID = mixerLineControls.dwLineID;
		mixerLineControls.pamxctrl = (LPMIXERCONTROLA)&mixerMute;
		mixerLineControls.cbmxctrl = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
		mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
		err = (*pmixerGetLineControlsA)((HMIXEROBJ)mixerHandle, &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
		if(!err) MixerCDMuteID = mixerLineControls.dwLineID;
		(*pmixerClose)(mixerHandle);
		OutTrace("%s: Mixer CD controls: Volume=%d Mute=%d\n", ApiRef, MixerCDVolumeID, MixerCDMuteID);

		// initialize variables
		char sProfilePath[MAX_PATH+1];
		sprintf(sProfilePath, "%s\\mixer.ini", GetDxWndPath());
		MixerCDMuteStatus=(*pGetPrivateProfileIntA)("CD", "MuteStatus", 0, sProfilePath);
		MixerCDVolumeLevel=(*pGetPrivateProfileIntA)("CD", "VolumeLevel", 0xFFFF, sProfilePath);
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
		OutTrace("%s: exception	caught\n", ApiRef);
	}		
}

void dxwCore::InitTarget(TARGETMAP *target)
{
	ApiName("InitTarget");
	dwIndex  = target->index; // order of program entry in dxwnd.ini file
	dwFlags1 = target->flags;
	dwFlags2 = target->flags2;
	dwFlags3 = target->flags3;
	dwFlags4 = target->flags4;
	dwFlags5 = target->flags5;
	dwFlags6 = target->flags6;
	dwFlags7 = target->flags7;
	dwFlags8 = target->flags8;
	dwFlags9 = target->flags9;
	dwFlags10= target->flags10;
	dwFlags11= target->flags11;
	dwFlags12= target->flags12;
	dwFlags13= target->flags13;
	dwFlags14= target->flags14;
	dwFlags15= target->flags15;
	dwFlags16= target->flags16;
	dwFlags17= target->flags17;
	dwFlags18= target->flags18;
	dwFlags19= target->flags19;
	dwFlags20= target->flags20;
	dwTFlags = target->tflags;
	dwTFlags2 = target->tflags2;
	dwDFlags = target->dflags;
	dwDFlags2= target->dflags2;
	iMaxW = target->resw;
	iMaxH = target->resh;
	*sMovieNickName = 0;
	*sAudioNickName = 0;

	hWnd = 0;
	// note: Windowize and LockedRes have the same initial value, but LockedRes remains constant
	// while Windowize can be made FALSE before entering a dialog and restored afterwards.
	Coordinates = target->coordinates;
	Windowize = (dwFlags2 & WINDOWIZE) ? TRUE : FALSE;
	LockedRes = Windowize; 
	isColorEmulatedMode = (Coordinates == DXW_COLOREMULATED); 
	//if(isColorEmulatedMode) Windowize = FALSE;
	isScaled = Windowize & !isColorEmulatedMode;
	isWineControlled = (dwFlags19 & ISWIN16EXECUTABLE) ? TRUE : FALSE;
	// in color emulation mode disable all remapping operations
	if(isColorEmulatedMode) {
		dwFlags1 &= ~CLIENTREMAPPING;
		dwFlags5 &= ~REMAPMCI;
		dwFlags18 &= ~SCALEDIRECTSHOW;
	}

	EmulateWin95 = (dwFlags15 & EMULATEWIN95) ? TRUE : FALSE;
	IsVisible = TRUE;
	memset(CurrDirectories, 0, sizeof(CurrDirectories));
	if(dwFlags3 & FORCEWINDOWING) FullScreen=TRUE;
	gsModules = target->module;
	MaxFPS = target->MaxFPS;
	CustomOpenGLLib = target->OpenGLLib;
	if(!strlen(CustomOpenGLLib)) CustomOpenGLLib=NULL;
	// bounds control
	dwTargetDDVersion = target->dxversion;
	MaxDdrawInterface = target->MaxDdrawInterface;
	if((MaxDdrawInterface < 1) || (MaxDdrawInterface > 7)) MaxDdrawInterface = 7;
	if(dwTargetDDVersion<0) dwTargetDDVersion=0;
	if(dwTargetDDVersion>12) dwTargetDDVersion=12;
	TimeShift = target->InitTS;
	if(TimeShift < -8) TimeShift = -8;
	if(TimeShift >  8) TimeShift =  8;
	FakeVersionId = target->FakeVersionId;
	CDAVolume = target->CDAVolume;
	MidiVolume = target->MidiVolume;
	WaveVolume = target->WaveVolume;
	GeneralVolume = target->GeneralVolume;
	lpddClipBack = 0;
	//MustFixCoordinates = ((dwFlags1 & MODIFYMOUSE) && !(dwFlags1 & MESSAGEPROC) && (dwFlags2 & WINDOWIZE)); // optimization
	//MustFixMouse = ((dwFlags1 & MODIFYMOUSE) && (dwFlags2 & WINDOWIZE));
	MustFixCoordinates = ((dwFlags1 & MODIFYMOUSE) && !(dwFlags1 & MESSAGEPROC)); // optimization
	MustFixMouse = ((dwFlags1 & MODIFYMOUSE) && isScaled);
	switch(target->SwapEffect){
		case 0: SwapEffect = D3DSWAPEFFECT_DISCARD; break;
		case 1: SwapEffect = D3DSWAPEFFECT_FLIP; break;
		case 2: SwapEffect = D3DSWAPEFFECT_COPY; break;
		case 3: SwapEffect = D3DSWAPEFFECT_OVERLAY; break;
		case 4: SwapEffect = D3DSWAPEFFECT_FLIPEX; break;
	}
	MustShowOverlay=((dwFlags2 & SHOWFPSOVERLAY) || (dwFlags4 & SHOWTIMESTRETCH));

	if(dwFlags16 & SETFOGCOLOR){
		char InitPath[MAX_PATH+1];
		sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
		fogColor = GetPrivateProfileIntA("window", "fogcolor", RGB(0xFF, 0x00, 0x00), InitPath);
		OutTrace("%s: custom fog color=%#x\n", ApiRef, fogColor);
	}

	// initialize timeshift values
	if(dwFlags2 & TIMESTRETCH){
		fMul[8]=1.0f;
		if(dwFlags13 & CUSTOMTIMESHIFT){
			char InitPath[MAX_PATH+1];
			char s_TimeShift[256+1];
			float fCustom;
			sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
			// GetPrivateProfileStringA is not hooked yet ...
			GetPrivateProfileStringA("debug", "timeshift", "1.0", s_TimeShift, 256, InitPath);
			sscanf(s_TimeShift, "%f", &fCustom);
			OutTrace("%s: custom timeshift=%f\n", ApiRef, fCustom);
			fMul[8]=fCustom;
		}
		float fTick = (dwFlags4 & FINETIMING) ? 1.10f : 1.50f;
		for (int i=9; i<=16; i++) fMul[i] = fMul[i-1] / fTick;
		for (int i=7; i>=0; i--) fMul[i] = fMul[i+1] * fTick;
	}

	////InitPos(target);
	//iMaxW = target->resw;
	//iMaxH = target->resh;
	// Check that custom resolution has some value ...
	if((iMaxW == 0) || (iMaxH == 0)) {
		dwFlags10 &= ~CUSTOMRES; 
	}
	// guessed initial screen resolution
	// v2.04.01.fx4: set default value ONLY when zero, because some program may initialize
	// them before creating a window that triggers second initialization, like "Spearhead"
	// through the Smack32 SmackSetSystemRes call
	if(!dwScreenWidth) dwScreenWidth = 800;
	if(!dwScreenHeight) dwScreenHeight = 600;

	SlowRatio = target->SlowRatio;
	ScanLine = target->ScanLine;

	// Window style
	WindowStyle = WSTYLE_DEFAULT;
	if (dwFlags2 & MODALSTYLE)   WindowStyle = WSTYLE_MODALSTYLE;
	if (dwFlags1 & FIXWINFRAME)  WindowStyle = WSTYLE_THICKFRAME;
	if (dwFlags9 & FIXTHINFRAME) WindowStyle = WSTYLE_THINFRAME;

	GDIEmulationMode = GDIMODE_NONE; // default
	if (dwFlags2 & GDISTRETCHED)	GDIEmulationMode = GDIMODE_STRETCHED;  
	if (dwFlags3 & GDIEMULATEDC)	GDIEmulationMode = GDIMODE_EMULATED; 
	if (dwFlags6 & SHAREDDC)		GDIEmulationMode = GDIMODE_SHAREDDC; 
	if (dwFlags15 & GDIASYNCDC)		GDIEmulationMode = GDIMODE_ASYNCDC; // temp

	if(dwDFlags & STRESSRESOURCES) dwFlags5 |= LIMITRESOURCES;

	// v2.05.81: decoupling renderer id from array index
	//Renderer = &dxwRenderers[target->RendererId];
	for(int i=0; dxwRenderers[i].name; i++) {
		Renderer = &dxwRenderers[i];
		if(Renderer->id == target->RendererId) break;
	}
	//RendererId = Renderer->id;
	IsEmulated = (Renderer->flags & DXWRF_EMULATED) ? TRUE : FALSE;
	SurfaceMode = PRIMARY_FULLSCREEN;
	if(IsEmulated || Windowize){
		SurfaceMode = (IsEmulated) ? 
			((dwFlags6 & HALFFLIPEMULATION) ? 
				PRIMARY_EMULATED : 
				PRIMARY_FLIPPABLE) : 
			PRIMARY_DIRECT;
	}

	typedef HRESULT (Blitter_Type)(int, Blt_Type, char *, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX, BOOL);
	extern Blitter_Type *pRendererBlt;
	pRendererBlt = (dxw.dwFlags15 & ASYNCBLITMODE) ? Renderer->pAsyncBlitter : Renderer->pSyncBlitter;
	if(!pRendererBlt) pRendererBlt = Renderer->pAsyncBlitter;
	if(!pRendererBlt) pRendererBlt = Renderer->pSyncBlitter;

	extern GetWindowLong_Type pGetWindowLong;
	extern SetWindowLong_Type pSetWindowLong;
	// made before hooking !!!
	// v2.05.44: deleted obsolete ANSIWIDE flag
	//pGetWindowLong = (dwFlags5 & ANSIWIDE) ? GetWindowLongW : GetWindowLongA;
	//pSetWindowLong = (dwFlags5 & ANSIWIDE) ? SetWindowLongW : SetWindowLongA;
	pGetWindowLong = GetWindowLongA;
	pSetWindowLong = SetWindowLongA;

	// hint system
	bHintActive = (dwFlags7 & SHOWHINTS) ? TRUE : FALSE;

	MonitorId = target->monitorid;

	// if specified, set the custom initial resolution
	if(dxw.dwFlags7 & INITIALRES) SetScreenSize(target->resw, target->resh);

	// v2.04.32: set emulated default video mode
	if(isScaled){
		// in window mode, set arbitrary default to 800x600
		dwDefaultScreenWidth = dwScreenWidth;
		dwDefaultScreenHeight = dwScreenHeight;
	} else {
		// v2.04.89: in native mode, set actual screen resolution (primary monitor)
		dwDefaultScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		dwDefaultScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	}
	if(dwFlags7 & INITIALRES) {
		dwDefaultScreenWidth = target->resw;
		dwDefaultScreenHeight = target->resh;
	}
	dwDefaultColorDepth = 32;
	if(dwFlags2 & INIT8BPP) dwDefaultColorDepth = 8;
	if(dwFlags2 & INIT16BPP) dwDefaultColorDepth = 16;
	if(dwFlags7 & INIT24BPP) dwDefaultColorDepth = 24;
	if(dwFlags7 & INIT32BPP) dwDefaultColorDepth = 32;
	if(dwFlags3 & FORCE16BPP) dwDefaultColorDepth = 16;

	FilterXScalingFactor = dxwFilters[target->FilterId].xfactor;
	FilterYScalingFactor = dxwFilters[target->FilterId].yfactor;
	FilterId = dxwFilters[target->FilterId].id;

	dwRealScreenWidth = 0;
	dwRealScreenHeight = 0;
	dxwLastDisplayWidth = 0;
	dxwLastDisplayHeight = 0;
	dxwLastDisplayBPP = 0;
	ReservedPaletteEntries = 20; // supposed initial value for a 8bit color desktop

	pInitialRamp = NULL;
	pCurrentRamp = NULL;
	bCustomKeyToggle = FALSE;
	if(dwFlags8 & LOADGAMMARAMP) bEnableGammaControl = TRUE;
	if(dwDFlags & STARTWITHTOGGLE) bCustomKeyToggle = TRUE;
	DIBRGBQuadEntries = NULL;
	bAsybcBlitStarted = NULL;
	lpBlitterSurface = NULL;

	// Fake device drives
	FakeHDDrive = target->FakeHDDrive;
	FakeCDDrive = target->FakeCDDrive;

	// Locale
	Locale = target->Locale;
	CodePage = target->CodePage;
	Country = target->Country;

	InitScreenResolutions();

	OutTrace("%s: __COMPAT_LAYER=\"%s\"\n", ApiRef, getenv("__COMPAT_LAYER"));

	if((dwFlags8 & VIRTUALCDAUDIO) && !(dwFlags15 & PLAYFROMCD)){
		if(dwFlags14 & SETCDAUDIOPATH){
			//cdAudioPath = (char *)malloc(strlen(GetDxWndPath()) + 1);
			//strcpy(cdAudioPath, GetDxWndPath());
			char key[12];
			char FakeHDPath[MAX_PATH];
			char InitPath[MAX_PATH];
			sprintf_s(key, sizeof(key), "fakecd%i", dwIndex);
			sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
			//OutTrace("InitPath=%s dxwndpath=%s key=%s\n", InitPath, GetDxWndPath(), key);
			GetPrivateProfileStringA("target", key, NULL, FakeHDPath, MAX_PATH, InitPath);
			// v2.05.91 fix: add handling of '?' wildcard for task folder replacement
			if(FakeHDPath[0]=='?'){
				HMODULE hshlwapi = LoadLibrary("shlwapi.dll");
				typedef BOOL (WINAPI *PathRemoveFileSpecA_Type)(LPCSTR);
				PathRemoveFileSpecA_Type pPathRemoveFileSpecA = (PathRemoveFileSpecA_Type)GetProcAddress(hshlwapi, "PathRemoveFileSpecA");
				char sTail[MAX_PATH+1];
				strcpy(sTail, &FakeHDPath[1]);
				// v2.05.59 fix: do not use current folder, use file path
				//GetCurrentDirectory(MAX_PATH, sCurrentPath); 
				GetModuleFileNameA(NULL, FakeHDPath, MAX_PATH);
				(*pPathRemoveFileSpecA)(FakeHDPath);
				FreeLibrary(hshlwapi);
				strncat(FakeHDPath, sTail, MAX_PATH);
			}
			cdAudioPath = (char *)malloc(strlen(FakeHDPath) + 1);
			strcpy(cdAudioPath, FakeHDPath);
		}
		else {
			cdAudioPath = ".";
		}
		OutTrace("%s: cdAudioPath=%s\n", ApiRef, cdAudioPath);
		/*
		char musicPath[MAX_PATH];
		sprintf(musicPath, "%s\\Music", cdAudioPath);
		DWORD attr = GetFileAttributesA(musicPath);
		if(attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)){
			OutTrace("no Music folder path=%s err=%d attr=%#x - disable VIRTUALCDAUDIO\n", 
				musicPath, GetLastError(), attr);
			dwFlags8 &= ~VIRTUALCDAUDIO;
		}
		*/
	}

#define UPTIMEMOREDAYS 45

	if(dwFlags14 & UPTIMECLEAR){
		// v2.05.75: recovered XP support deleting static link to GetTickCount64
		// LoadLibraryA and GetProcAddress are not hooked yet ...
		HMODULE hKernel32;
		ULONGLONG (WINAPI *pTick64)();
		typedef ULONGLONG (WINAPI *GetTickCount64_Type)();
		hKernel32=LoadLibraryA("kernel32.dll");
		pTick64 = (GetTickCount64_Type)GetProcAddress(hKernel32, "GetTickCount64");
		if(pTick64){
			LARGE_INTEGER Counter;
			AddedTimeInMS.QuadPart =  0L - (*pTick64)();
			if(QueryPerformanceCounter(&Counter)){
				AddedTimeInQPCTicks.QuadPart = 0L - Counter.QuadPart;
			}
		}
	}

	if(dwFlags14 & UPTIMESTRESS){
		LARGE_INTEGER Frequency;
		DWORD UpTimeMoreDays;
		char InitPath[MAX_PATH];
		sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
		UpTimeMoreDays = GetPrivateProfileIntA("window", "uptime", UPTIMEMOREDAYS, InitPath);
		AddedTimeInMS.QuadPart = (LONGLONG)UpTimeMoreDays * (LONGLONG)24 * (LONGLONG)3600 * (LONGLONG)1000;
		if(QueryPerformanceFrequency(&Frequency)){
			AddedTimeInQPCTicks.QuadPart = (LONGLONG)UpTimeMoreDays * (LONGLONG)24 * (LONGLONG)3600 * (LONGLONG)Frequency.QuadPart;
		}
	}

	if((dwFlags8 & VIRTUALHEAP) || (dxw.dwFlags14 & SAFEHEAP)){
		dxw.nHeaps = GetProcessHeaps(0, NULL);
		if(dxw.nHeaps != 0){
			dxw.pHeaps = (PHANDLE)malloc(dxw.nHeaps * sizeof(HANDLE));
			GetProcessHeaps(dxw.nHeaps, dxw.pHeaps);
			OutTrace("%s: Process heap=%#x\n", ApiRef, dxw.pHeaps[0]);
			for (int i=1; i<dxw.nHeaps; i++) OutTrace("Heap[%d]=%#x\n", i, dxw.pHeaps[i]);
		}
	}

	// error flags
	if(dwTFlags & LOGDEBUG) dwTFlags |= (LOGERRORS | LOGTRACE);
	if(dwTFlags & LOGTRACE) dwTFlags |= LOGERRORS;
	if(dwTFlags & LOGERRORS){
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DXWND] = dwTFlags2 & OUTDXWINTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DDRAW] = dwTFlags2 & OUTDDRAWTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_D3D] = dwTFlags2 & OUTD3DTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_WIN] = dwTFlags2 & OUTWINMESSAGES;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_CURSOR] = dwTFlags2 & OUTCURSORTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_IAT] = dwTFlags2 & OUTIMPORTTABLE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_REGISTRY] = dwTFlags2 & OUTREGISTRY;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_HOOK] = dwTFlags2 & TRACEHOOKS;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_WING32] = dwTFlags2 & OUTWINGTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_OPENGL] = dwTFlags2 & OUTOGLTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_SDL] = dwTFlags2 & OUTSDLTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_TIME] = dwTFlags2 & OUTTIMETRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_SOUND] = dwTFlags2 & OUTSOUNDTRACE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_INPUT] = dwTFlags2 & OUTINPUTS;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_SYS] = dwTFlags2 & OUTSYSLIBS;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_LOCALE] = dwTFlags2 & OUTLOCALE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_FPS] = dwTFlags2 & OUTFPS;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_IO] = dwTFlags2 & OUTFILEIO;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_GDI] = dwTFlags2 & OUTGDI;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_COM] = dwTFlags2 & OUTCOMTRACE;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_VGA] = dwTFlags2 & OUTVGA;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DSHOW] = dwTFlags2 & OUTDSHOW;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DRV] = dwTFlags2 & OUTDRV;	
	}
	// trace flags
	if(dwTFlags & LOGTRACE){
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_DXWND] = dwTFlags2 & OUTDXWINTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_DDRAW] = dwTFlags2 & OUTDDRAWTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_D3D] = dwTFlags2 & OUTD3DTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_WIN] = dwTFlags2 & OUTWINMESSAGES;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_CURSOR] = dwTFlags2 & OUTCURSORTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_IAT] = dwTFlags2 & OUTIMPORTTABLE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_REGISTRY] = dwTFlags2 & OUTREGISTRY;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_HOOK] = dwTFlags2 & TRACEHOOKS;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_WING32] = dwTFlags2 & OUTWINGTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_OPENGL] = dwTFlags2 & OUTOGLTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_SDL] = dwTFlags2 & OUTSDLTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_TIME] = dwTFlags2 & OUTTIMETRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_SOUND] = dwTFlags2 & OUTSOUNDTRACE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_INPUT] = dwTFlags2 & OUTINPUTS;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_SYS] = dwTFlags2 & OUTSYSLIBS;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_LOCALE] = dwTFlags2 & OUTLOCALE;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_FPS] = dwTFlags2 & OUTFPS;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_IO] = dwTFlags2 & OUTFILEIO;
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_GDI] = dwTFlags2 & OUTGDI;	
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_COM] = dwTFlags2 & OUTCOMTRACE;	
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_VGA] = dwTFlags2 & OUTVGA;	
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_DSHOW] = dwTFlags2 & OUTDSHOW;	
		bTFlags[DXTRACE_BASE_TRACE+DXTRACE_DRV] = dwTFlags2 & OUTDRV;	
	}
	// debug flags
	if(dwTFlags & LOGDEBUG){
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_DXWND] = dwTFlags2 & OUTDXWINTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_DDRAW] = dwTFlags2 & OUTDDRAWTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_D3D] = dwTFlags2 & OUTD3DTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_WIN] = dwTFlags2 & OUTWINMESSAGES;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_CURSOR] = dwTFlags2 & OUTCURSORTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_IAT] = dwTFlags2 & OUTIMPORTTABLE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_REGISTRY] = dwTFlags2 & OUTREGISTRY;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_HOOK] = dwTFlags2 & TRACEHOOKS;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_WING32] = dwTFlags2 & OUTWINGTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_OPENGL] = dwTFlags2 & OUTOGLTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_SDL] = dwTFlags2 & OUTSDLTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_TIME] = dwTFlags2 & OUTTIMETRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_SOUND] = dwTFlags2 & OUTSOUNDTRACE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_INPUT] = dwTFlags2 & OUTINPUTS;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_SYS] = dwTFlags2 & OUTSYSLIBS;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_LOCALE] = dwTFlags2 & OUTLOCALE;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_FPS] = dwTFlags2 & OUTFPS;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_IO] = dwTFlags2 & OUTFILEIO;
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_GDI] = dwTFlags2 & OUTGDI;	
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_COM] = dwTFlags2 & OUTCOMTRACE;	
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_VGA] = dwTFlags2 & OUTVGA;	
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_DSHOW] = dwTFlags2 & OUTDSHOW;	
		bTFlags[DXTRACE_BASE_DEBUG+DXTRACE_DRV] = dwTFlags2 & OUTDRV;	
	}
	if(dwTFlags & OUTALLERRORS){
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DXWND] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DDRAW] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_D3D] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_WIN] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_CURSOR] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_IAT] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_REGISTRY] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_HOOK] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_WING32] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_OPENGL] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_SDL] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_TIME] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_SOUND] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_INPUT] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_SYS] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_LOCALE] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_FPS] = TRUE;
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_IO] = TRUE;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_GDI] = TRUE;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_COM] = TRUE;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_VGA] = TRUE;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DSHOW] = TRUE;	
		bTFlags[DXTRACE_BASE_ERROR+DXTRACE_DRV] = TRUE;	
	}
	// automatically turn on HOTREGISTRY to allow d3d tweaks
	if(dwFlags13 & (DISABLED3DMMX | DISABLED3DXPSGP | DISABLEPSGP | D3DXDONOTMUTE))
		dwFlags9 |= HOTREGISTRY;

	CDADrive = target->CDADrive;
}

void dxwCore::SetScreenSize(void) 
{
	// if specified, use values registered in InitTarget
	if(dxw.dwFlags7 & INITIALRES) return;

	if(dxw.isScaled){
		SetScreenSize(800, 600); // set to default screen resolution
	}
	else{
		int sizx, sizy;
		sizx = GetSystemMetrics(SM_CXSCREEN);
		sizy = GetSystemMetrics(SM_CYSCREEN);
		SetScreenSize(sizx, sizy);
	}
}

void dxwCore::SetScreenSize(int x, int y) 
{
	ApiName("SetScreenSize");
	DXWNDSTATUS *p;
	OutTraceDW("%s: set screen size=(%d,%d)\n", ApiRef, x, y);
	if(x) dwScreenWidth=x; 
	if(y) dwScreenHeight=y;
	p = GetHookInfo();
	if(p) {
		p->Width = (short)dwScreenWidth;
		p->Height = (short)dwScreenHeight;
	}
	if(dxw.dwFlags7 & MAXIMUMRES){
		if(((long)p->Width > (int)dxw.iMaxW) || ((long)p->Height > (int)dxw.iMaxH)){
			OutTraceDW("%s: limit device size=(%d,%d)\n", ApiRef, dxw.iMaxW, dxw.iMaxH);
			// v2.03.90 setting new virtual desktop size 
			dwScreenWidth = p->Width = (short)dxw.iMaxW;
			dwScreenHeight= p->Height = (short)dxw.iMaxH;
		}
	}
}

void dxwCore::DumpDesktopStatus()
{
	ApiName("DumpDesktopStatus");
	HDC hDC;
	HWND hDesktop;
	RECT desktop;
	PIXELFORMATDESCRIPTOR pfd;
	int  iPixelFormat, iBPP;
	char ColorMask[32+1]; 

	// get the current pixel format index
	hDesktop = GetDesktopWindow();
	hDC = GetDC(hDesktop);
	::GetWindowRect(hDesktop, &desktop);
	iBPP = GetDeviceCaps(hDC, BITSPIXEL);
	iPixelFormat = GetPixelFormat(hDC); 
	if(!iPixelFormat) iPixelFormat=1; // why returns 0???
	// obtain a detailed description of that pixel format  
	if(!DescribePixelFormat(hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd)){
		OutTrace("%s: DescribePixelFormat ERROR: err=%d\n", ApiRef, GetLastError());
		return;
	}

	memset(ColorMask, ' ', 32); // blank fill
	ColorMask[32] = 0; // terminate
	if ((pfd.cRedShift+pfd.cRedBits <= 32) &&
		(pfd.cGreenShift+pfd.cGreenBits <= 32) &&
		(pfd.cBlueShift+pfd.cBlueBits <= 32) &&
		(pfd.cAlphaShift+pfd.cAlphaBits <= 32)){ // everything within the 32 bits ...
		for (int i=pfd.cRedShift; i<pfd.cRedShift+pfd.cRedBits; i++) ColorMask[i]='R';
		for (int i=pfd.cGreenShift; i<pfd.cGreenShift+pfd.cGreenBits; i++) ColorMask[i]='G';
		for (int i=pfd.cBlueShift; i<pfd.cBlueShift+pfd.cBlueBits; i++) ColorMask[i]='B';
		for (int i=pfd.cAlphaShift; i<pfd.cAlphaShift+pfd.cAlphaBits; i++) ColorMask[i]='A';
	}
	else
		strcpy(ColorMask, "???");
	OutTrace( 
		"%s: Desktop\n"
		"\tSize (W x H)=(%d x %d)\n" 
		"\tColor depth = %d (color bits = %d)\n"
		"\tPixel format = %d\n"
		"\tColor mask  (RGBA)= (%d,%d,%d,%d)\n" 
		"\tColor shift (RGBA)= (%d,%d,%d,%d)\n"
		"\tColor mask = \"%s\"\n"
		, 
		ApiRef,
		desktop.right, desktop.bottom, 
		iBPP, pfd.cColorBits,
		iPixelFormat,
		pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits,
		pfd.cRedShift, pfd.cGreenShift, pfd.cBlueShift, pfd.cAlphaShift,
		ColorMask
	);
}

void dxwCore::InitWindowPos(int x, int y, int w, int h)
{
	iPosX = x;
	iPosY = y; //v2.02.09
	iSizX = w;
	iSizY = h;
}

BOOL dxwCore::IsDesktop(HWND hwnd)
{
	return (
		(hwnd == 0)
		||
		(hwnd == (*pGetDesktopWindow)())
		||
		(hwnd == hWnd)
		);
}

BOOL dxwCore::IsRealDesktop(HWND hwnd)
{
	return (
		(hwnd == 0)
		||
		(hwnd == (*pGetDesktopWindow)())
		);
}

BOOL dxwCore::IsRealDesktopDC(HDC hdc)
{
	HWND hwnd = WindowFromDC(hdc);
	if(!hwnd) return FALSE;
	return (hwnd == (*pGetDesktopWindow)());
}

// v2.1.93: FixCursorPos completely revised to introduce a clipping tolerance in
// clipping regions as well as in normal operations

#define CLIP_TOLERANCE 4

POINT dxwCore::FixCursorPos(POINT prev)
{
	POINT curr;
	static BOOL IsWithin = TRUE;
	static POINT LastPos;

	curr=prev;

	// scale mouse coordinates
	// remember: rect from GetClientRect always start at 0,0!
	if(dxw.MustFixMouse){
		int w, h; // width, height and border

		// v2.04.88: use more reliable registered values instead of client window size
		w = iSizX;
		h = iSizY;

#ifdef PROVENTOBEUSEFUL
		if(dxw.dwFlags20 & CENTERONEXIT){
				if ((curr.x < 0) || (curr.y < 0) || (curr.x > w) || (curr.y > h)){
					curr.x = w / 2;
					curr.y = h / 2;
				}
		}
#endif

		if(dxw.dwFlags4 & RELEASEMOUSE){
			if ((curr.x < 0) || (curr.y < 0) || (curr.x > w) || (curr.y > h)){
				if(IsWithin){
					int RestX, RestY;
					RestX = w ? ((CLIP_TOLERANCE * w) / dwScreenWidth) + 2 : CLIP_TOLERANCE + 2;
					RestY = h ? ((CLIP_TOLERANCE * h) / dwScreenHeight) + 2 : CLIP_TOLERANCE + 2;
					if (curr.x < 0) curr.x = RestX;
					if (curr.y < 0) curr.y = RestY;
					if (curr.x > w) curr.x = w - RestX;
					if (curr.y > h) curr.y = h - RestY;
					LastPos = curr;
					IsWithin = FALSE;
				}
				else{
					curr = LastPos;
				}
			}
			else{
				IsWithin = TRUE;
				LastPos = curr;
			}
		}
		else {
			if (curr.x < 0) curr.x = 0;
			if (curr.y < 0) curr.y = 0;
			if (curr.x > w) curr.x = w;
			if (curr.y > h) curr.y = h;
		}

		if (w) curr.x = (curr.x * dwScreenWidth) / w;
		if (h) curr.y = (curr.y * dwScreenHeight) / h;

		if(dxw.dwFlags17 & FIXMOUSEBIASX) curr.x++; // v2.05.99 add-on
		if(dxw.dwFlags17 & FIXMOUSEBIASY) curr.y++;
	}

	// v2.06.10: INVERTMOUSE flags moved out of if condition above
	if(dwFlags11 & INVERTMOUSEXAXIS) curr.x = dwScreenWidth - curr.x;
	if(dwFlags11 & INVERTMOUSEYAXIS) curr.y = dwScreenHeight - curr.y; // v2.05.46 fix !!!
	FixCursorClipper(&curr);

	return curr;
}

void dxwCore::FixCursorClipper(LPPOINT p)
{
	extern LPRECT lpClipRegion;
	if((dxw.dwFlags1 & DISABLECLIPPING) && lpClipRegion){
		// v2.1.93:
		// in clipping mode, avoid the cursor position to lay outside the valid rect
		// note 1: the rect follow the convention and valid coord lay between left to righ-1,
		// top to bottom-1
		// note 2: CLIP_TOLERANCE is meant to handle possible integer divide tolerance errors
		// that may prevent reaching the clip rect borders. The smaller you shrink the window, 
		// the bigger tolerance is required
		if (p->x < lpClipRegion->left+CLIP_TOLERANCE) p->x=lpClipRegion->left;
		if (p->y < lpClipRegion->top+CLIP_TOLERANCE) p->y=lpClipRegion->top;
		if (p->x >= lpClipRegion->right-CLIP_TOLERANCE) p->x=lpClipRegion->right-1;
		if (p->y >= lpClipRegion->bottom-CLIP_TOLERANCE) p->y=lpClipRegion->bottom-1;
	}
	else{
		if (p->x < CLIP_TOLERANCE) p->x=0;
		if (p->y < CLIP_TOLERANCE) p->y=0;
		if (p->x >= (LONG)dwScreenWidth-CLIP_TOLERANCE) p->x=dwScreenWidth-1;
		if (p->y >= (LONG)dwScreenHeight-CLIP_TOLERANCE) p->y=dwScreenHeight-1;
	}
}

POINT dxwCore::ScreenToClient(POINT point)
{
	ApiName("ScreenToClient");
	// convert absolute screen coordinates to frame relative
	// v2.04.80: no error messages for initial 0 window handle
	if (hWnd && !(*pScreenToClient)(hWnd, &point)) {
		OutTraceE("%s: ERROR hwnd=%#x err=%d @%d\n", ApiRef, hWnd, GetLastError(), __LINE__);
		//point.x =0; point.y=0;
		point.x -= iPosX;
		point.y -= iPosY;
		if(point.x < 0) point.x = 0;
		if(point.y < 0) point.y = 0;
	}

	return point;
}

void dxwCore::FixNCHITCursorPos(LPPOINT lppoint)
{
	RECT rect;
	POINT point;

	point=*lppoint;
	(*pGetClientRect)(dxw.GethWnd(), &rect);
	(*pScreenToClient)(dxw.GethWnd(), &point);

	if (point.x < 0) return;
	if (point.y < 0) return;
	if (point.x > rect.right) return;
	if (point.y > rect.bottom) return;

	*lppoint=point;
	lppoint->x = (lppoint->x * dwScreenWidth) / rect.right; // v2.05.05.fx2 fix
	lppoint->y = (lppoint->y * dwScreenHeight) / rect.bottom;
	if(lppoint->x < CLIP_TOLERANCE) lppoint->x=0;
	if(lppoint->y < CLIP_TOLERANCE) lppoint->y=0;
	if(lppoint->x > (LONG)dwScreenWidth-CLIP_TOLERANCE) lppoint->x=dwScreenWidth-1;
	if(lppoint->y > (LONG)dwScreenHeight-CLIP_TOLERANCE) lppoint->y=dwScreenHeight-1;
}

void dxwCore::InitializeClipCursorState(void)
{
	ApiName("InitializeClipCursorState");
	RECT cliprect;
	BOOL clipret;
	clipret = (*pGetClipCursor)(&cliprect);
	// v2.04.06: you always get a clipper area. To tell that the clipper is NOT active for your window 
	// you can compare the clipper area with the whole desktop. If they are equivalent, you have no 
	// clipper (or you are in fullscreen mode, but that is equivalent).
	ClipCursorToggleState = TRUE;
	if (((cliprect.right - cliprect.left) == (*pGetSystemMetrics)(SM_CXVIRTUALSCREEN)) && 
		((cliprect.bottom - cliprect.top) == (*pGetSystemMetrics)(SM_CYVIRTUALSCREEN))) 
		ClipCursorToggleState = FALSE;
	OutTraceDW("%s: Initial clipper status=%#x\n", ApiRef, ClipCursorToggleState);
}

BOOL dxwCore::IsClipCursorActive(void)
{
	static BOOL bDoOnce = TRUE;
	if (bDoOnce) InitializeClipCursorState();
	return ClipCursorToggleState;
}

void dxwCore::SetClipCursor()
{
	ApiName("SetClipCursor");
	RECT Rect;
	POINT UpLeftCorner={0,0};

	OutTraceDW("%s:\n", ApiRef);
	if (hWnd==NULL) {
		OutTraceDW("%s: ASSERT hWnd==NULL\n", ApiRef);
		return;
	}

	// check for errors to avoid setting random clip regions
	//if((*pIsWindowVisible)(hWnd)){
	//	OutTraceE("SetClipCursor: not visible\n");
	//	return;
	//}

	if(!(*pGetClientRect)(hWnd, &Rect)){
		OutTraceE("%s: GetClientRect ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return;
	}
	if((Rect.right == 0) && (Rect.bottom == 0)){ 
		OutTraceE("%s: GetClientRect returns zero sized rect @%d\n", ApiRef, __LINE__);
		return;
	}
	if(!(*pClientToScreen)(hWnd, &UpLeftCorner)){
		OutTraceE("%s: ClientToScreen ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return ;
	}
	Rect.left+=UpLeftCorner.x;
	Rect.right+=UpLeftCorner.x;
	Rect.top+=UpLeftCorner.y;
	Rect.bottom+=UpLeftCorner.y;

	if(dwFlags8 & CLIPMENU) {
		// v2.04.11:
		// if flag set and the window has a menu, extend the mouse clipper area to allow reaching the manu
		// implementation is partial: doesn't take in account multi-lines menues or menus positioned
		// not on the top of the window client area, but it seems good for the most cases.
		if(GetMenu(hWnd)) Rect.top -= (*pGetSystemMetrics)(SM_CYMENU);
	}

	(*pClipCursor)(NULL);
	if((*pClipCursor)(&Rect)){
		ClipCursorToggleState = TRUE;
	}
	else{
		OutTraceE("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
	}

	OutTraceDW("%s: rect=(%d,%d)-(%d,%d)\n",
		ApiRef, Rect.left, Rect.top, Rect.right, Rect.bottom);
}

void dxwCore::EraseClipCursor()
{
	ApiName("EraseClipCursor");
	OutTraceDW("%s:\n", ApiRef);
	(*pClipCursor)(NULL);
	ClipCursorToggleState = FALSE;
}

void dxwCore::SethWnd(HWND hwnd) 
{
	ApiName("SethWnd");
	RECT WinRect;
	if(!pGetWindowRect) pGetWindowRect=::GetWindowRect;
	if(!pGDIGetDC)		pGDIGetDC=::GetDC;

	hWnd=hwnd; 
	hWndFPS=hwnd;	

	// v2.04.77: added hWnd tracking for desktop hider 
	// beware, SethWnd can be called before GetHookInfo initialization 
	DXWNDSTATUS *p = GetHookInfo();
	if(p) p->hWnd = hwnd;

	if(hwnd){
		(*pGetWindowRect)(hwnd, &WinRect);
		OutTraceDW("%s: setting main win=%#x pos=(%d,%d)-(%d,%d) res=(%dx%d)\n", 
			ApiRef, hwnd, WinRect.left, WinRect.top, WinRect.right, WinRect.bottom, 
			dxw.GetScreenWidth(), dxw.GetScreenHeight());
		if(dxw.dwFlags12 & DISABLEDWM) DisableDWM(hwnd);
	}
	else{
		OutTraceDW("%s: clearing main win\n", ApiRef);
	}
}

void dxwCore::DisableDWM(HWND hwnd)
{
	ApiName("DisableDWM");
	HRESULT res;
	HMODULE hmod;
	typedef HRESULT (WINAPI *DwmSetWindowAttribute_Type)(HWND, DWORD, LPCVOID, DWORD);
	DwmSetWindowAttribute_Type pDwmSetWindowAttribute;
	hmod=(*pLoadLibraryA)("Dwmapi.dll");
	if(!hmod) return;
	pDwmSetWindowAttribute=(DwmSetWindowAttribute_Type)(*pGetProcAddress)(hmod,"DwmSetWindowAttribute");
	if(!pDwmSetWindowAttribute) return;
	DWMNCRENDERINGPOLICY ncRenderingPolicy = DWMNCRP_DISABLED;
	res = (*pDwmSetWindowAttribute)(hwnd, DWMWA_NCRENDERING_POLICY,
		&ncRenderingPolicy, sizeof(ncRenderingPolicy));
	_if(res) OutTraceE("%s: DwmSetWindowAttribute(DWMWA_NCRENDERING_POLICY DWMNCRP_DISABLED) ERROR res=%#x\n", ApiRef, res);
	BOOL disableTransitions = TRUE;
	res = (*pDwmSetWindowAttribute)(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED,
		&disableTransitions, sizeof(disableTransitions));
	_if(res) OutTraceE("%s: DwmSetWindowAttribute(DWMWA_TRANSITIONS_FORCEDISABLED TRUE) ERROR res=%#x\n", ApiRef, res);
	FreeLibrary(hmod);
}

void dxwCore::SuppressIMEWindow()
{
	ApiName("SuppressIMEWindow");
	OutTraceDW("%s: SUPPRESS IME\n", ApiRef);
	typedef BOOL (WINAPI *ImmDisableIME_Type)(DWORD);
	ImmDisableIME_Type pImmDisableIME;
	HMODULE ImmLib;
	ImmLib=(*pLoadLibraryA)("Imm32");
	if(ImmLib){
		pImmDisableIME=(ImmDisableIME_Type)(*pGetProcAddress)(ImmLib,"ImmDisableIME");
		if(pImmDisableIME)(*pImmDisableIME)(-1);
		FreeLibrary(ImmLib);
	}
}

void dxwCore::AdjustWindowPos(HWND hwnd, DWORD width, DWORD height)
{
	ApiName("AdjustWindowPos");
	WINDOWPOS wp;
	DWORD lpWinCB;

	OutTraceDW("%s: hwnd=%#x size=(%d,%d)\n", ApiRef, hwnd, width, height);
	if(dwFlags10 & HIDEWINDOWCHANGES){
		lpWinCB = (*pGetWindowLong)(hwnd, GWL_WNDPROC);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, NULL);
	}
	CalculateWindowPos(hwnd, width, height, &wp);
	OutTraceDW("%s: hwnd=%#x fixed pos=(%d,%d) size=(%d,%d)\n", ApiRef, hwnd, wp.x, wp.y, wp.cx, wp.cy);

	RECT rect1, rect2;
	(*pGetWindowRect)(hwnd, &rect1);
	rect2.left = wp.x;
	rect2.right = wp.x + wp.cx;
	rect2.top = wp.y;
	rect2.bottom = wp.y + wp.cy;
	if(memcmp(&rect1, &rect2, sizeof(RECT))){
		if(!(*pSetWindowPos)(hwnd, 0, wp.x, wp.y, wp.cx, wp.cy, 0)){
			OutTraceE("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		}
	}
	else {
		OutDebugDW("%s: skip unnecessary move\n", ApiRef);
	}

	UpdateDesktopCoordinates();
	if(dxw.dwFlags2 & SUPPRESSIME) dxw.SuppressIMEWindow();
	if(dxw.dwFlags4 & HIDEDESKTOP) dxw.HideDesktop(hwnd);
	ShowBanner(hwnd);
	if(dwFlags10 & HIDEWINDOWCHANGES){
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, lpWinCB);
	}
}

BOOL dxwCore::ishWndFPS(HWND hwnd) 
{
	return (hwnd == hWndFPS);
}

void dxwCore::FixWorkarea(LPRECT workarea)
{
	int w, h, b; // width, height and border
	int rw, rh;

	rw = iRatioX;
	rh = iRatioY;
	if(dwFlags11 & ADAPTIVERATIO) { // ADAPTIVERATIO
		rw = dwScreenWidth;
		rh = dwScreenHeight;
	}

	w = workarea->right - workarea->left;
	h = workarea->bottom - workarea->top;
	if ((w * rh) > (h * rw)){
		b = (w - (h * rw / rh))/2;
		workarea->left += b;
		workarea->right -= b;
	}
	else {
		b = (h - (w * rh / rw))/2;
		workarea->top += b;
		workarea->bottom -= b;
	}
}

//#define DUMPALLPALETTECHANGES TRUE

void dxwCore::DumpPalette(DWORD dwcount, LPPALETTEENTRY lpentries)
{
	char sInfo[(14*256)+1];
	char sEntry[20+1];
	if(dwcount == 0) return;
	sInfo[0]=0;
	// "Spearhead" has a bug that sets 897 palette entries!
	if(dwcount > 256) dwcount=256;
	for(DWORD idx=0; idx<dwcount; idx++) {
		char flags[2+1];
		__try {
			switch(lpentries[idx].peFlags){
				case PC_RESERVED: strcpy(flags, "RE"); break;
				case PC_EXPLICIT: strcpy(flags, "EX"); break; 
				case PC_NOCOLLAPSE: strcpy(flags, "NC"); break; 
				default: sprintf(flags, "%02x", lpentries[idx].peFlags); break;
			}
			sprintf(sEntry, "(%02x.%02x.%02x:%s)",  
			lpentries[idx].peRed, lpentries[idx].peGreen, lpentries[idx].peBlue, flags);
			strcat(sInfo, sEntry);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){ break; }
	}
	strcat(sInfo,"\n");
	OutTrace(sInfo);

#ifdef DUMPALLPALETTECHANGES
	if(DUMPALLPALETTECHANGES){
		extern DXWNDSTATUS *pStatus;
		for(DWORD idx=0; idx<dwcount; idx++)  
			pStatus->Palette[idx]= lpentries[idx];
		Sleep(2000);
	}
#endif
}

#ifndef DXW_NOTRACE
void dxwCore::DumpSysPalette(void)
{
	ApiName("DumpSysPalette");
	char *sUse[] = {"ERROR", "STATIC", "NOSTATIC", "NOSTATIC256"};
	PALETTEENTRY PalEntries[256];
	HWND hDesktop = (*pGetDesktopWindow)();
	HDC hdc = (*pGDIGetDC)(hDesktop);
	UINT use = (*pGetSystemPaletteUse)(hdc);
	UINT count = (*pGDIGetSystemPaletteEntries)(hdc, 0, 256, PalEntries);
	(*pGDIReleaseDC)(hDesktop, hdc);
	OutTrace("%s: desktop SystemPaletteUse=%d(%s) count=%d\n", 
		ApiRef, 
		use, 
		sUse[use > SYSPAL_NOSTATIC256 ? 0 : use], 
		count);
	dxw.DumpPalette(count, PalEntries);
}
#endif // DXW_NOTRACE

void dxwCore::ScreenRefresh(void)
{
	// v2.04.30: fix for "Jane's Fighters Anthology"
	// by default, perform an optimized screen refresh, that is no more that once every 20 mSec.
	// This is no good for palette updates, that risk to leave the wrong colors to screen.
	ScreenRefresh(FALSE);
}

void dxwCore::ScreenRefresh(BOOL bForced)
{
	// optimization: don't blit too often!
	// 20mSec seems a good compromise.
	#define DXWREFRESHINTERVAL 20

	LPDIRECTDRAWSURFACE lpDDSPrim;
	LPDIRECTDRAWSURFACE lpDDSSource;
	extern HRESULT WINAPI extBlt(char *, int, Blt_Type, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);

	static int t = -1;
	if (t == -1)
		t = (*pGetTickCount)()-(DXWREFRESHINTERVAL+1); // V.2.1.69: trick - subtract 
	int tn = (*pGetTickCount)();

	if ((tn-t < DXWREFRESHINTERVAL) && !bForced) return;
	t = tn;

	// if not too early, refresh primary surface ....
	lpDDSPrim = dxwss.GetPrimarySurface();
	lpDDSSource = lpDDSPrim;
	if(dxw.dwFlags1 & BLITFROMBACKBUFFER) lpDDSSource = dxwss.GetBackBufferSurface();
	if (lpDDSPrim) {
		char *objname = "??";
		switch(iPrimarySurfaceVersion){
			case 1: objname = "IDirectDrawSurface::Blt"; break;
			case 2: objname = "IDirectDrawSurface2::Blt"; break;
			case 3: objname = "IDirectDrawSurface3::Blt"; break;
			case 4: objname = "IDirectDrawSurface4::Blt"; break;
			case 7: objname = "IDirectDrawSurface7::Blt"; break;
		}
		extBlt(objname, iPrimarySurfaceVersion, pBltMethod(), lpDDSPrim, NULL, lpDDSSource, NULL, 0, NULL); 
	}

	// v2.02.44 - used for what? Commenting out seems to fix the palette update glitches  
	// and make the "Palette updates don't blit" option useless....
	// v2.04.82 - invalidate the client area, makes "Gangsters Organized Crime" menu text visible
	if(dxw.dwFlags10 & INVALIDATECLIENT) (*pInvalidateRect)(hWnd, NULL, FALSE); 
}

void dxwCore::DoSlow(int delay)
{
	MSG uMsg;
	int t, tn;
	t = (*pGetTickCount)();

	uMsg.message=0; // initialize somehow...
	while((tn = (*pGetTickCount)())-t < delay){
		while (PeekMessage (&uMsg, NULL, 0, 0, PM_REMOVE) > 0){
			if(WM_QUIT == uMsg.message) break;
			TranslateMessage (&uMsg);
			DispatchMessage (&uMsg);
		}	
		(*pSleep)(1);
	}
}

// Remarks:
// If the target window is owned by the current process, GetWindowText causes a WM_GETTEXT message 
// to be sent to the specified window or control. If the target window is owned by another process 
// and has a caption, GetWindowText retrieves the window caption text. If the window does not have 
// a caption, the return value is a null string. This behavior is by design. It allows applications 
// to call GetWindowText without becoming unresponsive if the process that owns the target window 
// is not responding. However, if the target window is not responding and it belongs to the calling 
// application, GetWindowText will cause the calling application to become unresponsive. 

static void ShowFPSOnTitlebar(HWND hwnd, int FPSCount)
{
	static HWND LasthWnd = 0;
	static char sBuf[80+15+1]; // title + fps string + terminator
	char *fpss;
	if(hwnd != LasthWnd){
		GetWindowText(hwnd, sBuf, 80);
		LasthWnd = hwnd;
	}
	fpss=strstr(sBuf," ~ (");
	if(fpss==NULL) fpss=&sBuf[strlen(sBuf)];
	if(dxw.dwTFlags & ADDTIMESTAMP) {
		sprintf_s(fpss, 15, " ~ (fps:%d@%d)", FPSCount, GetHookInfo()->upTime);
	}
	else {
		sprintf_s(fpss, 15, " ~ (fps:%d)", FPSCount);
	}
	SetWindowText(hwnd, sBuf);
}

static void CountFPS(HWND hwnd)
{
	ApiName("CountFPS");
	static DWORD time = 0xFFFFFFFF;
	static DWORD FPSCount = 0;
	extern void SetFPS(int);
	//DXWNDSTATUS Status;
	DWORD tmp;
	tmp = (*pGetTickCount)();
	if((tmp - time) > 1000) {
		// log fps count
		// OutTrace("FPS: Delta=%#x FPSCount=%d\n", (tmp-time), FPSCount);
		// show fps count on status win
		GetHookInfo()->FPSCount = FPSCount; // for overlay display
		// show fps on win title bar
		if (dxw.dwFlags2 & SHOWFPS) ShowFPSOnTitlebar(hwnd, FPSCount);
		// trace FPS counters on log
		OutTraceFPS("%s: @%d fps=%d\n", ApiRef, tmp / 1000, FPSCount);
		// reset
		FPSCount=0;
		time = tmp;
	}
	else {
		FPSCount++;
		//OutDebugDW("FPS: Delta=%#x FPSCount++=%d\n", (tmp-time), FPSCount);
	}
}

void LimitFrameMSec(DWORD delay)
{
	ApiName("LimitFrameMSec");
	static DWORD oldtime = (*pGetTickCount)();
	DWORD newtime;
	newtime = (*pGetTickCount)();
	OutDebugDW("%s: old=%#x new=%#x delay=%d sleep=%d\n", 
		ApiRef, oldtime, newtime, delay, (oldtime+delay-newtime));
	// use '<' and not '<=' to avoid the risk of sleeping forever....
	if((newtime < oldtime+delay) && (newtime >= oldtime)) {
		do{
			OutDebugDW("%s: sleep=%d\n", ApiRef, oldtime+delay-newtime);
			(*pSleep)(oldtime+delay-newtime);
			newtime = (*pGetTickCount)();
			OutDebugDW("%s: newtime=%#x\n", ApiRef, newtime);
		} while(newtime < oldtime+delay);
	}
	oldtime += delay;
	if(oldtime < newtime-delay) oldtime = newtime-delay;
}

#define MAXCUSTOMDELAYCOUNT 60

void LimitFrameHz(DWORD hertz)
{
	ApiName("LimitFrameHz");
	static UINT *iRefreshDelays = NULL;

	if((hertz < 1) || (hertz > 800)) return; 

	if(!iRefreshDelays) {
		UINT Reminder=0;
		UINT iRefreshDelayCount = 0;
		iRefreshDelays = (UINT *)malloc((MAXCUSTOMDELAYCOUNT + 1) * sizeof(UINT));
		UINT *dwDelay = iRefreshDelays;
		do{
			*dwDelay=(1000+Reminder)/hertz;
			Reminder=(1000+Reminder)-(*dwDelay * hertz);
			iRefreshDelayCount++;
			dwDelay++;
		} while(Reminder && (iRefreshDelayCount<MAXCUSTOMDELAYCOUNT));
		*dwDelay = 0; // terminator
		if(IsTraceDW){
			char sInfo[MAXCUSTOMDELAYCOUNT * 10];
			strcpy(sInfo, "");
			for(UINT i=0; i<iRefreshDelayCount; i++) sprintf(sInfo, "%s%d ", sInfo, iRefreshDelays[i]);
			OutTraceDW("%s: Refresh rate=%d: delay=%s\n", ApiRef, hertz, sInfo);
		}
	}

	static DWORD time = 0;
	static BOOL step = 0;
	DWORD tmp;
	tmp = (*pGetTickCount)();
	if((time - tmp) > 1000) time = tmp;
	(*pSleep)(time - tmp);
	time += iRefreshDelays[step++];
	if(iRefreshDelays[step] == 0) step=0;
}

void LimitFrameCount(DWORD delay)
{
	if(dxw.dwFlags14 & LIMITFREQUENCY)
		LimitFrameHz(delay); // Hertz, actually
	else
		LimitFrameMSec(delay); // mSec
}

static BOOL SkipFrameCount(DWORD delay)
{
	static DWORD oldtime=(*pGetTickCount)();
	DWORD newtime;
	newtime = (*pGetTickCount)();
	if(newtime < oldtime+delay) return TRUE; // TRUE => skip the screen refresh
	oldtime = newtime;
	return FALSE; // don't skip, do the update
}

BOOL dxwCore::HandleFPS()
{
	//if(dwFlags2 & (SHOWFPS|SHOWFPSOVERLAY)) CountFPS(hWndFPS);
	CountFPS(hWndFPS);
	if(dwFlags2 & LIMITFPS)LimitFrameCount(dxw.MaxFPS);
	if(dwFlags2 & SKIPFPS) if(SkipFrameCount(dxw.MaxFPS)) return TRUE;
	return FALSE;
}

void dxwCore::HandleDIB()
{
	if(dwFlags10 & LIMITDIBOPERATIONS) LimitFrameCount(dxw.MaxFPS);
}

void dxwCore::SuspendFPS(void)
{
	dwFPSFlags = dwFlags2 & (SHOWFPS|SHOWFPSOVERLAY|LIMITFPS|SKIPFPS);
	dwFlags2 &= ~dwFPSFlags;
}

void dxwCore::ResumeFPS(void)
{
	dwFlags2 |= dwFPSFlags;
}

void dxwCore::LimitAsyncBlitter(void)
{
	if(dxw.dwFlags2 & LIMITFPS) LimitFrameCount(dxw.MaxFPS);
	else LimitFrameHz(60); // by default, limit to 60 FPS/Hz
}

// auxiliary functions ...

void dxwCore::SetVSyncDelays(UINT RefreshRate)
{
	ApiName("SetVSyncDelays");
	int Reminder;
	char sInfo[256];

	if(!(dxw.dwFlags1 & SAVELOAD)) return;
	if((RefreshRate < 10) || (RefreshRate > 100)) return; 

	gdwRefreshRate = RefreshRate;
	if(!gdwRefreshRate) return;
	iRefreshDelayCount=0;
	Reminder=0;
	do{
		iRefreshDelays[iRefreshDelayCount]=(1000+Reminder)/gdwRefreshRate;
		Reminder=(1000+Reminder)-(iRefreshDelays[iRefreshDelayCount]*gdwRefreshRate);
		iRefreshDelayCount++;
	} while(Reminder && (iRefreshDelayCount<MAXREFRESHDELAYCOUNT));
	if(IsTraceDW){
		strcpy(sInfo, "");
		for(int i=0; i<iRefreshDelayCount; i++) sprintf(sInfo, "%s%d ", sInfo, iRefreshDelays[i]);
		OutTraceDW("%s: Refresh rate=%d: delay=%s\n", ApiRef, gdwRefreshRate, sInfo);
	}
}

void dxwCore::VSyncWait()
{
	static DWORD time = 0;
	static BOOL step = 0;
	DWORD tmp;
	tmp = (*pGetTickCount)();
	if((time - tmp) > 32) time = tmp;
	(*pSleep)(time - tmp);
	time += iRefreshDelays[step++];
	if(step >= iRefreshDelayCount) step=0;
}

void dxwCore::VSyncWaitLine(DWORD ScanLine)
{
	ApiName("VSyncWaitLine");
	extern LPDIRECTDRAW lpPrimaryDD;
	static DWORD iLastScanLine = 0;
	DWORD iCurrentScanLine;
	if (!lpPrimaryDD) return;
	while(1){
		HRESULT res;
		if(res=lpPrimaryDD->GetScanLine(&iCurrentScanLine)) {
			OutTraceE("%s: GetScanLine ERROR res=%#x\n", ApiRef, res);
			iLastScanLine = 0;
			break; // error
		}
		if((iLastScanLine <= ScanLine) && (iCurrentScanLine > ScanLine)) {
			OutDebugDW("%s: line=%d last=%d\n", ApiRef, iCurrentScanLine, iLastScanLine);
			break;
		}
		iLastScanLine = iCurrentScanLine;
		(*pSleep)(1);
	}
}

// Windows sleep in 100ns units 
void nanosleep(LONGLONG ns){
    /* Declarations */
    static HANDLE timer = NULL;   /* Timer handle */
    LARGE_INTEGER li;   /* Time defintion */
    /* Create timer */
	if(!timer){
		if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL))) return;
	}
    /* Set timer properties */
    li.QuadPart = -ns;
    if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
        CloseHandle(timer);
		timer = NULL;
        return;
    }
    /* Start & wait for timer */
    WaitForSingleObject(timer, INFINITE);
    /* Clean resources */
    //CloseHandle(timer);
    /* Slept without problems */
    return;
}

static DWORD TimeShifter(DWORD val, int shift)
{
	float fVal;
	fVal = (float)val * dxw.fMul[shift+8];
	return (DWORD)fVal;
}

static LARGE_INTEGER TimeShifter64(LARGE_INTEGER val, int shift)
{
	float fVal;
	fVal = (float)val.LowPart * dxw.fMul[shift+8];
	val.HighPart = 0;
	val.LowPart = (DWORD)fVal;
	return val;	
}

DWORD dxwCore::GetTickCount(void)
{
	DWORD dwTick;
	static DWORD dwLastRealTick=0;
	static DWORD dwLastFakeTick=0;
	DWORD dwNextRealTick;
	static BOOL FirstTime = TRUE;
	static BOOL TimeStretch;

	if(FirstTime){
		dwLastRealTick=(*pGetTickCount)();
		dwLastFakeTick=dwLastRealTick;
		FirstTime=FALSE;
		TimeStretch = dwFlags2 & TIMESTRETCH;
	}
	dwNextRealTick=(*pGetTickCount)();
	dwTick=(dwNextRealTick-dwLastRealTick);
	if(TimeStretch){
		TimeShift=GetHookInfo()->TimeShift;
		dwTick = TimeShifter(dwTick, TimeShift);
	}
	if(TimeFreeze) dwTick=0;
	dwLastFakeTick += dwTick;
	dwLastRealTick = dwNextRealTick;
	return dwLastFakeTick;
}

DWORD dxwCore::StretchTime(DWORD dwTimer)
{
	TimeShift=GetHookInfo()->TimeShift;
	dwTimer = TimeShifter(dwTimer, -TimeShift);
	return dwTimer;
}

LARGE_INTEGER dxwCore::StretchTime(LARGE_INTEGER dwTimer)
{
	static int Reminder = 0;
	LARGE_INTEGER ret;
	TimeShift=GetHookInfo()->TimeShift;
	dwTimer.QuadPart += Reminder;
	ret = TimeShifter64(dwTimer, -TimeShift);
	Reminder = (ret.QuadPart==0) ? dwTimer.LowPart : 0;
	return ret;
}

DWORD dxwCore::StretchCounter(DWORD dwTimer)
{
	TimeShift=GetHookInfo()->TimeShift;
	dwTimer = TimeShifter(dwTimer, TimeShift);
	return (dxw.TimeFreeze) ? 0 : dwTimer;
}

LARGE_INTEGER dxwCore::StretchCounter(LARGE_INTEGER dwTimer)
{
	static int Reminder = 0;
	LARGE_INTEGER ret;
	LARGE_INTEGER zero = {0,0};
	TimeShift=GetHookInfo()->TimeShift;
	dwTimer.QuadPart += Reminder;
	ret = TimeShifter64(dwTimer, TimeShift);
	Reminder = (ret.QuadPart==0) ? dwTimer.LowPart : 0;
	return (dxw.TimeFreeze) ? zero : ret;
}

void ShiftSystemTime(SYSTEMTIME *t)
{
	ApiName("ShiftSystemTime");
	char gInitPath[MAX_PATH];
	char sFakeCalendarDate[80+1]; 
	sprintf(gInitPath, "%sdxwnd.ini", GetDxWndPath()); 
	GetPrivateProfileString("window", "fakedate", "1/1/1986", sFakeCalendarDate, 80, gInitPath);
	int day, month, year;
	sscanf(sFakeCalendarDate, "%d/%d/%d", &day, &month, &year);
	t->wYear = year;
	t->wMonth = month;
	t->wDay = day;
	t->wDayOfWeek = -1; // invalid value
	OutTrace("%s: setting %d/%d/%d %02d:%02d:%02d\n", ApiRef,
		t->wDay = day, t->wMonth = month, t->wYear = year,
		t->wHour, t->wMinute, t->wSecond);
}

void dxwCore::GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	DWORD dwTick;
	DWORD dwCurrentTick;
	FILETIME CurrFileTime;
	static DWORD dwStartTick=0;
	static FILETIME StartFileTime;

	if(dwStartTick==0) {
		SYSTEMTIME StartingTime;
		// first time through, initialize & return true time
		dwStartTick = (*pGetTickCount)();
		(*pGetSystemTime)(&StartingTime);
		if(dwFlags16 & FAKEDATE) ShiftSystemTime(&StartingTime);
		SystemTimeToFileTime(&StartingTime, &StartFileTime);
		*lpSystemTimeAsFileTime = StartFileTime;
	}
	else {
		dwCurrentTick=(*pGetTickCount)();
		dwTick=(dwCurrentTick-dwStartTick);
		TimeShift=GetHookInfo()->TimeShift;
		dwTick = TimeShifter(dwTick, TimeShift);
		if(dxw.TimeFreeze) dwTick=0;
		// From MSDN: Contains a 64-bit value representing the number of 
		// 100-nanosecond intervals since January 1, 1601 (UTC).
		// So, since 1mSec = 10.000 * 100nSec, you still have to multiply by 10.000.
		CurrFileTime.dwHighDateTime = StartFileTime.dwHighDateTime; // wrong !!!!
		CurrFileTime.dwLowDateTime = StartFileTime.dwLowDateTime + (10000 * dwTick); // wrong !!!!
		*lpSystemTimeAsFileTime=CurrFileTime;
		// reset to avoid time jumps on TimeShift changes...
		StartFileTime = CurrFileTime;
		dwStartTick = dwCurrentTick;
	}
}

void dxwCore::GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
	DWORD dwTick;
	DWORD dwCurrentTick;
	FILETIME CurrFileTime;
	static DWORD dwStartTick=0;
	static FILETIME StartFileTime;

	if(dwStartTick==0) {
		SYSTEMTIME StartingTime;
		// first time through, initialize & return true time
		dwStartTick = (*pGetTickCount)();
		(*pGetSystemTime)(&StartingTime);
		if(dwFlags16 & FAKEDATE) ShiftSystemTime(&StartingTime);
		SystemTimeToFileTime(&StartingTime, &StartFileTime);
		*lpSystemTime = StartingTime;
	}
	else {
		dwCurrentTick=(*pGetTickCount)();
		dwTick=(dwCurrentTick-dwStartTick);
		TimeShift=GetHookInfo()->TimeShift;
		dwTick = TimeShifter(dwTick, TimeShift);
		if(TimeFreeze) dwTick=0;
		// From MSDN: Contains a 64-bit value representing the number of 
		// 100-nanosecond intervals since January 1, 1601 (UTC).
		// So, since 1mSec = 10.000 * 100nSec, you still have to multiply by 10.000.
		CurrFileTime.dwHighDateTime = StartFileTime.dwHighDateTime; // wrong !!!!
		CurrFileTime.dwLowDateTime = StartFileTime.dwLowDateTime + (10000 * dwTick); // wrong !!!!
		FileTimeToSystemTime(&CurrFileTime, lpSystemTime);
		// reset to avoid time jumps on TimeShift changes...
		StartFileTime = CurrFileTime;
		dwStartTick = dwCurrentTick;
	}
}

void dxwCore::ShowOverlay()
{
	if (MustShowOverlay) {
		RECT rect;
		(*pGetClientRect)(hWnd, &rect);
		this->ShowOverlay(GetDC(hWnd), rect.right, rect.bottom);
	}
}

void dxwCore::ShowOverlay(LPDIRECTDRAWSURFACE lpdds)
{
	typedef HRESULT (WINAPI *GetDC_Type) (LPDIRECTDRAWSURFACE, HDC FAR *);
	typedef HRESULT (WINAPI *ReleaseDC_Type)(LPDIRECTDRAWSURFACE, HDC);
	extern GetDC_Type pGetDCMethod();
	extern ReleaseDC_Type pReleaseDCMethod();
	if (MustShowOverlay) {
		HDC hdc; // the working dc
		int h, w;
		if(!lpdds) return;
		if (FAILED((*pGetDCMethod())(lpdds, &hdc))) return; 
		w = this->dwScreenWidth;
		h = this->dwScreenHeight;
		if(dxw.FilterXScalingFactor) w *= dxw.FilterXScalingFactor;
		if(dxw.FilterYScalingFactor) h *= dxw.FilterYScalingFactor;
		this->ShowOverlay(hdc, w, h);
		(*pReleaseDCMethod())(lpdds, hdc);
	}
}

void dxwCore::ShowOverlay(HDC hdc)
{
	if(!hdc) return;
	RECT rect;
	(*pGetClientRect)(hWnd, &rect);
	this->ShowOverlay(hdc, rect.right, rect.bottom);
}

void dxwCore::ShowOverlay(HDC hdc, int w, int h)
{
	if(!hdc) return;
	if((dwFlags2 & SHOWFPSOVERLAY) || (dwFlags4 & SHOWTIMESTRETCH)) ShowTextOverlay(hdc, w, h);
	if (dwCDOverlayTimeStamp) ShowCDChangerIcon(hdc, w, h);
}

#define DXW_OVERLAY_W 120
#define DXW_OVERLAY_H 20
#define DXW_BORDER_X 10 
#define DXW_BORDER_Y 10 

void dxwCore::ShowTextOverlay(HDC xdc, int w, int h)
{
	char sBuf[81];
	static int LastCorner;
	static DWORD dwTimer = 0;
	int corner;
	static int x, y;
	static int iOverlayPos;
	static int iOverlayStyle;
	static HFONT hFont = NULL;
	DWORD dwBkColor, dwTextColor, dwROP;
	BOOL rAlign, bAlign, cAlign;
	SIZE Size;

	// initializations
	if(dwTimer == 0){
		char inipath[MAX_PATH];
		GetModuleFileName(GetModuleHandle("dxwnd"), inipath, MAX_PATH);
		inipath[strlen(inipath)-strlen("dxwnd.dll")] = 0; // terminate the string just before "dxwnd.dll"
		strcat(inipath, "dxwnd.ini");
		iOverlayPos = GetPrivateProfileInt("window", "overlaypos", 0, inipath);
		iOverlayStyle = GetPrivateProfileInt("window", "overlaystyle", 0, inipath);
	}
	if(!hFont) {
		if(!pGDICreateFontA) pGDICreateFontA = CreateFontA;
		hFont = (*pGDICreateFontA)(16, 6, 
			0, 0, 600, // weight 600 = almost bold
			0, 0, 0, 0, 0, 0, 
			CLEARTYPE_NATURAL_QUALITY, 
			0, "Arial");
	}

	// set position
	if(iOverlayPos == 0) {
		if((*pGetTickCount)()-dwTimer > 8000){
			dwTimer = (*pGetTickCount)();
			do {
				corner = rand() % 4;
			} while(corner == LastCorner);
			LastCorner = corner;
		}
	}
	else {
		corner = iOverlayPos - 1;
	}

	rAlign = bAlign = cAlign = FALSE;
	switch (corner) {
		// the 4 corners, from upper-left clockwise
		case 0: x=DXW_BORDER_X; y=DXW_BORDER_Y; break;
		case 1: x=w-DXW_BORDER_X; y=DXW_BORDER_Y; rAlign = TRUE; break;
		case 2: x=w-DXW_BORDER_X; y=h-DXW_BORDER_Y; rAlign = TRUE; bAlign = TRUE; break;
		case 3: x=DXW_BORDER_X; y=h-DXW_BORDER_Y; bAlign = TRUE;  break;
		// centered
		case 4: x = w / 2; y = h / 2; cAlign = TRUE; break;
	}

	strcpy_s(sBuf, 80, "");
	if (dwFlags2 & SHOWFPSOVERLAY) {
		if (dwTFlags & ADDTIMESTAMP)
			sprintf_s(sBuf, 80, " fps:%d@%d ", GetHookInfo()->FPSCount, GetHookInfo()->upTime);
		else
			sprintf_s(sBuf, 80, " fps:%d ", GetHookInfo()->FPSCount);
	}
	if (dwFlags4 & SHOWTIMESTRETCH) sprintf_s(sBuf, 80, "%s t%s ", sBuf, dxw.GetTSCaption());

	// warn: over palettized 8bpp DC you should not set colors not in the palette.
	// full black, white or primary colors usually are ok.
	switch(iOverlayStyle){
		case 0: // legacy
			dwBkColor = 0x00FFFFFF; // white
			dwTextColor = 0x00FF0000; // blue
			dwROP = SRCCOPY;
			break;
		case 1:
			dwBkColor = 0x00FFFFFF; // white
			dwTextColor = 0x00FF0000; // blue
			dwROP = SRCCOPY;
			break;
		case 2:
			dwBkColor = 0x00000000; // black
			//dwTextColor = 0x00C0C0C0; // light gray
			dwTextColor = 0x00FFFFFF; // white
			dwROP = SRCINVERT;
			break;
		case 3:
			dwBkColor = 0x00000000; // black
			dwTextColor = 0x00FFFFFF; // white
			dwROP = SRCPAINT;
			break;
	}

	// GetTextExtentPoint32 call should not be placed before we select the font or the
	// calculations won't be accurate!
	if(!pSetTextColor) pSetTextColor = SetTextColor;
	if(iOverlayStyle == 0){
		(*pSetTextColor)(xdc,dwTextColor);
		SetBkMode(xdc, OPAQUE);
		GetTextExtentPoint32(xdc, sBuf, strlen(sBuf), &Size);
		if(rAlign) x -= Size.cx; // if positioned to right, right-align.
		if(bAlign) y -= Size.cy; // if positioned to bottom, bottom-align.
		if(cAlign){
			x -= Size.cx / 2;
			y -= Size.cy / 2; 
		}
		TextOut(xdc, x, y, sBuf, strlen(sBuf));
	}
	else {
		HDC hdc;
		HBITMAP hbmp, holdbmp;
		HFONT hPrevFont;
		hdc=CreateCompatibleDC(xdc);
		hbmp=CreateCompatibleBitmap(xdc, DXW_OVERLAY_W, DXW_OVERLAY_H);
		holdbmp=(HBITMAP)SelectObject(hdc, (HGDIOBJ)hbmp);
		SetBkColor(hdc,dwBkColor); 
		(*pSetTextColor)(hdc, dwTextColor); 
		SetBkMode(hdc, OPAQUE);
		hPrevFont = (HFONT)SelectObject(hdc, (HGDIOBJ)hFont);
		TextOut(hdc, 0, 0, sBuf, strlen(sBuf));
		GetTextExtentPoint32(hdc, sBuf, strlen(sBuf), &Size);
		if(rAlign) x -= Size.cx; // if positioned to right, right-align.
		if(bAlign) y -= Size.cy; // if positioned to bottom, bottom-align.
		if(cAlign){
			x -= Size.cx / 2;
			y -= Size.cy / 2; 
		}
		BitBlt(xdc, x, y, Size.cx, Size.cy, hdc, 0, 0, dwROP);
		DeleteObject(hbmp);
		DeleteObject(hdc);
	}
}

void dxwCore::ShowCDChanger()
{
	dxw.dwCDOverlayTimeStamp = (*pGetTickCount)()+4000;
	dxw.MustShowOverlay = TRUE;
}

void dxwCore::ShowCDChangerIcon(HDC xdc, int w, int h)
{
	BOOL ret;
	char sBuf[81];
	static HBITMAP CDIcon = NULL;
	BITMAP bm;
	POINT PrevViewPort;
	if((*pGetTickCount)() > dwCDOverlayTimeStamp){
		dwCDOverlayTimeStamp = 0;
		// recover original value
		MustShowOverlay=((dwFlags2 & SHOWFPSOVERLAY) || (dwFlags4 & SHOWTIMESTRETCH));
		return;
	}
	if(!CDIcon) {
		extern HMODULE hInst;
		//if(!hInst)MessageBox(NULL, "bad hInst", "", 0);
		CDIcon = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CDICON64));
		//if(!CDIcon)MessageBox(NULL, "LoadBitmap fail", "", 0);
	}
    HDC hdcMem = CreateCompatibleDC(xdc);
	//if(!hdcMem)MessageBox(NULL, "CreateCompatibleDC fail", "", 0);
    HBITMAP hbmOld = (HBITMAP)(*pSelectObject)(hdcMem, CDIcon);
	GetObject(CDIcon, sizeof(bm), &bm);
	(*pSetViewportOrgEx)(xdc, 0, 0, &PrevViewPort);
	ret=(*pGDIBitBlt)(xdc, 0, 0, 64, 64, hdcMem, 0, 0, SRCPAINT);
	if(!ret)MessageBox(NULL, "BitBlt fail", "", 0);
	if(!pSetTextColor) pSetTextColor = SetTextColor;
	(*pSetTextColor)(xdc, 0xFF0000); // blue
	SetBkMode(xdc, TRANSPARENT);
	sprintf_s(sBuf, 80, "CD%d", GetHookInfo()->CDIndex+1);
	TextOut(xdc, 18, 6, sBuf, strlen(sBuf));
	(*pSetViewportOrgEx)(xdc, PrevViewPort.x, PrevViewPort.y, NULL);
    (*pSelectObject)(hdcMem, hbmOld);
	(*pGDIReleaseDC)(dxw.GethWnd(), xdc); // v2.04.97: add to avoid DC leakage
    DeleteDC(hdcMem);
}

char *dxwCore::GetTSCaption(int shift)
{
	static char *sTSCaptionCoarse[17]={
		"*16","*12","*8","*6",
		"*4","*3","*2","*1.5",
		"*1",
		"/1.5","/2","/3","/4",
		"/6","/8","/12","/16"};
	static char *sTSCaptionFine[17]={
		"*2.14","*1.95","*1.77","*1.61",
		"*1.46","*1.33","*1.21","*1.10",
		"*1.00",
		"/1.10","/1.21","/1.33","/1.46",
		"/1.61","/1.77","/1.95","/2.14"};
	if(TimeFreeze) return "x0";
	if (shift<(-8) || shift>(+8)) return "???";
	shift += 8;
	return (dxw.dwFlags4 & FINETIMING) ? sTSCaptionFine[shift] : sTSCaptionCoarse[shift];
}

char *dxwCore::GetTSCaption(void)
{
	return GetTSCaption(TimeShift);
}

void dxwCore::ShowBanner(HWND hwnd)
{
	ApiName("ShowBanner");
	static BOOL JustOnce=FALSE;
	extern HMODULE hInst;
	BITMAP bm;
	HDC hClientDC;
	HBITMAP g_hbmBall;
	RECT client;
	RECT win;
	POINT PrevViewPort;
	int StretchMode;
	int nFrames;
	char sBanner[MAX_PATH];
	BOOL bDebugMode = FALSE;

	hClientDC=(*pGDIGetDC)(hwnd); 
	(*pGetClientRect)(hwnd, &client);
	(*pInvalidateRect)(hwnd, NULL, FALSE); // invalidate virtual desktop, no erase.
	(*pGDIBitBlt)(hClientDC, 0, 0,  client.right, client.bottom, NULL, 0, 0, BLACKNESS);

	if(JustOnce || (dwFlags2 & NOBANNER)) return;
	JustOnce=TRUE;

	bDebugMode = dxw.dwDFlags || dxw.dwDFlags2;

	g_hbmBall = NULL;
	if(!bDebugMode){
		sprintf(sBanner, "%s\\dxwanim.bmp", GetDxWndPath());
		nFrames = 16;
		g_hbmBall = (HBITMAP)LoadImage(NULL, sBanner, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	if(!g_hbmBall) {
		nFrames = 1;
		g_hbmBall = LoadBitmap(hInst, MAKEINTRESOURCE(bDebugMode ? IDB_DEBUGBANNER : IDB_BANNER));
	}

    HDC hdcMem = CreateCompatibleDC(hClientDC);
    HBITMAP hbmOld = (HBITMAP)(*pSelectObject)(hdcMem, g_hbmBall);
    GetObject(g_hbmBall, sizeof(bm), &bm);

	(*pGetWindowRect)(hwnd, &win);
	OutTraceDW("%s: hwnd=%#x win=(%d,%d)-(%d,%d) banner size=(%dx%d)\n", 
		ApiRef, hwnd, win.left, win.top, win.right, win.bottom, bm.bmWidth, bm.bmHeight);

	//if(!pSetViewportOrgEx) pSetViewportOrgEx=SetViewportOrgEx;
	(*pSetViewportOrgEx)(hClientDC, 0, 0, &PrevViewPort);
	StretchMode=GetStretchBltMode(hClientDC);
	SetStretchBltMode(hClientDC, HALFTONE);
	for (int i=1; i<=16; i++){
		BOOL ret;
		int w, h;
		w=(bm.bmWidth*i)/(8*nFrames);
		h=(bm.bmHeight*i)/8;
		ret=(*pGDIStretchBlt)(hClientDC, (client.right-w)/2, (client.bottom-h)/2, w, h, hdcMem, 0, 0, (bm.bmWidth/nFrames), bm.bmHeight, SRCCOPY);
		(*pInvalidateRect)(hwnd, NULL, FALSE);
		Sleep(20);
	}
	if(nFrames == 16) for (int i=0; i<16; i++){
		int w, h;
		w=bm.bmWidth/8;
		h=bm.bmHeight*2;
		(*pGDIBitBlt)(hClientDC, 0, 0,  client.right, client.bottom, NULL, 0, 0, BLACKNESS);
		(*pGDIStretchBlt)(hClientDC, (client.right-w)/2, (client.bottom-h)/2, w, h, hdcMem, (bm.bmWidth/16)*i, 0, (bm.bmWidth/16), bm.bmHeight, SRCCOPY);
		(*pInvalidateRect)(hwnd, NULL, FALSE);
		Sleep(20);
	}
	if(bDebugMode) Sleep(120);
	for (int i=16; i>=4; i--){
		int w, h;
		w=(bm.bmWidth*i)/(8*nFrames);
		h=(bm.bmHeight*i)/8;
		(*pGDIBitBlt)(hClientDC, 0, 0,  client.right, client.bottom, NULL, 0, 0, BLACKNESS);
		(*pGDIStretchBlt)(hClientDC, (client.right-w)/2, (client.bottom-h)/2, w, h, hdcMem, 0, 0, (bm.bmWidth/nFrames), bm.bmHeight, SRCCOPY);
		(*pInvalidateRect)(hwnd, NULL, FALSE);
		Sleep(20);
	}

	if(dxw.dwFlags3 & LOCKFPSCORNER){
		DWORD confbuf[10];
		char *speedrun;
		confbuf[0]= dxw.dwFlags1;
		confbuf[1]= dxw.dwFlags2;
		confbuf[2]= dxw.dwFlags3;
		confbuf[3]= dxw.dwFlags4;
		confbuf[4]= dxw.dwFlags5;
		confbuf[5]= dxw.dwFlags6;
		confbuf[6]= dxw.dwFlags7;
		confbuf[7]= dxw.dwFlags8;
		confbuf[8]= dxw.dwFlags9;
		confbuf[9]= dxw.dwFlags10;
		speedrun = ((dxw.dwFlags2 & (TIMESTRETCH | LIMITFPS)) || (dxw.dwFlags4 & STRETCHTIMERS)) ? "SpeedRun KO" : "SpeedRun OK";
		
		if(!pSetTextColor) pSetTextColor = SetTextColor;
		(*pSetTextColor)(hClientDC, 0x0); // black
		SetBkMode(hClientDC, OPAQUE);
		TextOut(hClientDC, 10, 10, hexdump((LPBYTE)&confbuf[0], sizeof(confbuf)/2), 60);
		TextOut(hClientDC, 10, 30, hexdump((LPBYTE)&confbuf[5], sizeof(confbuf)/2), 60);
		TextOut(hClientDC, 10, 50, speedrun, strlen(speedrun));
		(*pInvalidateRect)(hwnd, NULL, FALSE);
		Sleep(2500);
	}

	SetStretchBltMode(hClientDC, StretchMode);
	(*pSetViewportOrgEx)(hClientDC, PrevViewPort.x, PrevViewPort.y, NULL);
    (*pSelectObject)(hdcMem, hbmOld);
    DeleteDC(hdcMem);
	(*pGDIReleaseDC)(hwnd, hClientDC); 

	Sleep(200);
}

void dxwCore::SetDLLFlags(int idx, DWORD flags)
{
	//SysLibsTable[idx].name = lpName; // add entry
	SysLibsTable[idx].flags = flags; // add entry
}

int dxwCore::GetDLLIndex(char *lpFileName)
{
	ApiName("GetDLLIndex");
	int idx;
	char *lpName, *lpNext;

	// v2.04.41: consider also pathnames with mixed slash '/' and backslash '\' folder indicator
	// fixes "Crime Cities" OpenGL path "driver/tcd3d/opengl32.dll"
	lpName=lpFileName;
	do {
		lpNext=strchr(lpName,'\\');
		if(!lpNext) lpNext=strchr(lpName,'/');
		if(lpNext) lpName=lpNext+1;
	} while(lpNext);

	for(idx=0; SysLibsTable[idx].name; idx++){
		char SysNameExt[81];
		strcpy(SysNameExt, SysLibsTable[idx].name);
		strcat(SysNameExt, ".dll");
		if(SysLibsTable[idx].prefixed){
			if(!_strnicmp(lpName, SysLibsTable[idx].name, strlen(SysLibsTable[idx].name))){
				OutTraceDW("%s: Registered wildcarded DLL FileName=%s match=%s*\n", ApiRef, lpFileName, SysLibsTable[idx].name);
				break;
			}
		} else {
			if(
				(!lstrcmpi(lpName, SysLibsTable[idx].name)) ||
				(!lstrcmpi(lpName, SysNameExt))
			){
				OutTraceDW("%s: Registered DLL FileName=%s\n", ApiRef, lpFileName);
				break;
			}
		}
	}
	if (!SysLibsTable[idx].name) return -1;	// module unmatched
	if (SysLibsTable[idx].flags == DXWHOOK_NULL) return -1; // module disabled
	return idx;
}

DWORD dxwCore::FixWinStyle(DWORD dwStyle)
{
	switch(dxw.Coordinates){
		case DXW_SET_COORDINATES:
		case DXW_DESKTOP_CENTER:
			switch(dxw.WindowStyle){
				case WSTYLE_DEFAULT: 
					break;
				case WSTYLE_MODALSTYLE: 
					dwStyle &= ~(WS_BORDER | WS_CAPTION | WS_DLGFRAME | WS_SYSMENU | WS_THICKFRAME);
					break;
				case WSTYLE_THICKFRAME:
				case WSTYLE_THINFRAME:
					// v2.05.13: better control of flags trimmed up / down
					// this code is equivalent to former one but the SCROLL flags, the TABSTOP controls 
					// and for all low-word bits
					dwStyle &= ~(
						WS_OVERLAPPED | 
						WS_POPUP | 
						WS_MAXIMIZE | 
						WS_DLGFRAME | 
						WS_BORDER | 
						// WS_HSCROLL |
						// WS_VSCROLL |
						WS_SYSMENU |
						// WS_GROUP |
						//WS_TABSTOP |
						WS_THICKFRAME
						);
					dwStyle |= (dxw.dwFlags9 & FIXTHINFRAME) ? WS_OVERLAPPEDTHIN : WS_OVERLAPPEDWINDOW;
					break;
			}
			break;
		case DXW_DESKTOP_WORKAREA:
		case DXW_DESKTOP_FULL:
		case DXW_COLOREMULATED:
			dwStyle &= ~(WS_BORDER | WS_CAPTION | WS_DLGFRAME | WS_SYSMENU | WS_THICKFRAME);
			break;
	}
	// v2.05.19: WS_CLIPCHILDREN would clear the effect of INVALIDATECLIENT tweak, clear it when the tweak is set
	// recovers the "Gangsters: Organized crime" menu texts
	// v2.06.06: logic for INVALIDATECLIENT moved from AdjustWindowFrame to here
	if(dxw.dwFlags10 & INVALIDATECLIENT) dwStyle &= ~WS_CLIPCHILDREN;
	return dwStyle;
}

DWORD dxwCore::FixWinExStyle(DWORD dwExStyle)
{
	switch(dxw.Coordinates){
		case DXW_SET_COORDINATES:
		case DXW_DESKTOP_CENTER:
			dwExStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
			break;
		case DXW_DESKTOP_WORKAREA:
		case DXW_DESKTOP_FULL:
		case DXW_COLOREMULATED:
			dwExStyle = 0;
			break;
	}

	if(dxw.dwFlags5 & UNLOCKZORDER) dwExStyle &= ~WS_EX_TOPMOST;

	return dwExStyle;
}

void dxwCore::FixWindowFrame(HWND hwnd)
{
	DWORD nStyle, nExStyle;
	ApiName("FixWindowFrame");

	OutTraceDW("%s: hwnd=%#x foreground=%#x style=%d\n", 
		ApiRef, hwnd, GetForegroundWindow(), dxw.WindowStyle);

	if(WindowStyle == WSTYLE_DEFAULT) return;

	// beware: 0 is a valid return code!
	nStyle=(*pGetWindowLong)(hwnd, GWL_STYLE);
	nExStyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE);

#ifndef DXW_NOTRACES
	char sStyle[256];
	char sExStyle[256];
	OutTraceDW("%s: style=%#x(%s) exstyle=%#x(%s)\n",
		ApiRef,
		nStyle, ExplainStyle(nStyle, sStyle, 256),
		nExStyle, ExplainExStyle(nExStyle, sExStyle, 256));
#endif // DXW_NOTRACES

	nStyle=FixWinStyle(nStyle);
	nExStyle=FixWinExStyle(nExStyle);

	// v2.04.43: no error checks, 0 could be the previous value returned correctly
	SetWindowLong_Type pSetWindowLong = (IsWindowUnicode(hwnd)) ? pSetWindowLongW : pSetWindowLongA;
	(*pSetWindowLong)(hwnd, GWL_STYLE, nStyle);
	(*pSetWindowLong)(hwnd, GWL_EXSTYLE, nExStyle);

	(*pSetWindowPos)(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);

#ifndef DXW_NOTRACES
	OutTraceDW("%s: FIXED style=%#x(%s) exstyle=%#x(%s)\n",
		ApiRef,
		nStyle, ExplainStyle(nStyle, sStyle, 256),
		nExStyle, ExplainExStyle(nExStyle, sExStyle, 256));
#endif // DXW_NOTRACES

	// ShowWindow retcode means in no way an error code! Better ignore it.
	(*pShowWindow)(hwnd, SW_RESTORE);
	return;
}

void dxwCore::HookWindowProc(HWND hwnd)
{
	WNDPROC pWindowProc;
	ApiName("HookWindowProc");

	if(dxw.dwDFlags2 & NOWINDOWHOOKS) return;

	pWindowProc = (WNDPROC)(*pGetWindowLong)(hwnd, GWL_WNDPROC);
	
	// don't hook twice ....	
	if ((pWindowProc == extWindowProc) ||
		(pWindowProc == extChildWindowProc) ||
		(pWindowProc == extDialogWindowProc) ||
		(pWindowProc == dw_Hider_Message_Handler)){
		// hooked already !!!
		OutTraceDW("%s: hwnd=%#x WindowProc HOOK already in place\n", ApiRef, hwnd);
		return;
	}

	// v2.03.22: don't remap WindowProc in case of special address 0xFFFFnnnn. 
	// This makes "The Hulk demo" work avoiding WindowProc recursion and stack overflow
	// v2.03.99.rc1: commented out, issue fixed by addition below!
	//if (((DWORD)pWindowProc & 0xFFFF0000) == 0xFFFF0000){
	//	OutTraceDW("GetWindowLong: hwnd=%#x WindowProc HOOK %#x not updated\n", hwnd, pWindowProc);
	//	return;
	//}

	// v2.03.99.rc1: always remap  WindowProc, but push to call stack the previous value 
	// depending on whether the window was hooked already or not!
	long lres;
	if(lres=(long)dxwws.GetProc(hwnd))
		dxwws.PutProc(hwnd, (WNDPROC)lres);
	else 
		dxwws.PutProc(hwnd, pWindowProc);

	// v2.05.66: avoid recursion in "ICE-Land"
	SetWindowLong_Type pSetWindowLong = (IsWindowUnicode(hwnd)) ? pSetWindowLongW : pSetWindowLongA;
	lres=(*pSetWindowLong)(hwnd, GWL_WNDPROC, (LONG)extWindowProc);
	OutTraceDW("%s: HOOK hwnd=%#x WindowProc=%#x->%#x\n", ApiRef, hwnd, lres, (LONG)extWindowProc);
}

void dxwCore::AdjustWindowFrame(HWND hwnd, DWORD width, DWORD height)
{
	HRESULT res=0;
	LONG style, exstyle;
	ApiName("AdjustWindowFrame");

	OutTraceDW("%s: hwnd=%#x, size=(%d,%d) coord=%d\n", ApiRef, hwnd, width, height, dxw.Coordinates); 

	//dxw.SetScreenSize(width, height); these values include the win border !!!
	if (hwnd==NULL) return;

	// v2.06.06: if WSTYLE_DEFAULT skip all style changes
	// v2.06.13: if color emulated mode skip all style changes
	if((dxw.WindowStyle != WSTYLE_DEFAULT) && !dxw.isColorEmulatedMode){
		// v2.05.13: window frame now updated taking in proper account the former window style/exstyle flags
		// in particular, keeps the WS_CLIPCHILDREN/SIBLINGS flags necessary in many situations.
		// Improves "Minigolf Masters Miniverse".
		GetWindowLong_Type pGetWindowLong = (IsWindowUnicode(hwnd)) ? pGetWindowLongW : pGetWindowLongA;
		style=(*pGetWindowLong)(hwnd, GWL_STYLE);
		exstyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE); 

		// v2.05.39: to propagate all flags is no good for DPI scaling (see 
		// https://sourceforge.net/p/dxwnd/discussion/general/thread/f261d3f2dd/#6c4c about "Age of Empires"
		// fixed returning to v2.05.12 logic + propagation of WS_CLIPCHILDREN and WS_CLIPSIBLINGS flags.
		// v2.05.48: "Command & Conquer Red Alert 2" needs the clipping flags, so let's comment the line below:
		//style &= ~(WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

		style = dxw.FixWinStyle(style);
		exstyle = dxw.FixWinExStyle(exstyle);

		SetWindowLong_Type pSetWindowLong = (IsWindowUnicode(hwnd)) ? pSetWindowLongW : pSetWindowLongA;
		(*pSetWindowLong)(hwnd, GWL_STYLE, style);
		(*pSetWindowLong)(hwnd, GWL_EXSTYLE, exstyle); 

		OutTraceDW("%s: hwnd=%#x, set style=%s extstyle=0\n", ApiRef, hwnd, (style == 0) ? "0" : "WS_OVERLAPPEDWINDOW"); 
	}

	(*pShowWindow)(hwnd, SW_SHOWNORMAL);

	//if (dxw.isScaled) dxw.AdjustWindowPos(hwnd, width, height);
	dxw.AdjustWindowPos(hwnd, width, height);

	// fixing windows message handling procedure
	dxw.HookWindowProc(hwnd);

	// fixing cursor view and clipping region

	if ((dxw.dwFlags1 & HIDEHWCURSOR) && dxw.IsFullScreen()) while ((*pShowCursor)(0) >= 0);
	if (dxw.dwFlags2 & SHOWHWCURSOR) while((*pShowCursor)(1) < 0);
	if (dxw.dwFlags1 & CLIPCURSOR) {
		OutTraceDW("%s: setting clip region\n", ApiRef);
		dxw.SetClipCursor();
	}

	(*pInvalidateRect)(hwnd, NULL, TRUE);
}

void dxwCore::FixWindow(HWND hwnd, DWORD dwStyle, DWORD dwExStyle, int x, int y, int nWidth, int nHeight)
{
	DWORD lpWinCB;
	if(dwFlags10 & HIDEWINDOWCHANGES){
		lpWinCB = (*pGetWindowLong)(hwnd, GWL_WNDPROC);
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, NULL);
	}
	if(dxw.WindowStyle != WSTYLE_DEFAULT){
		(*pSetWindowLong)(hwnd, GWL_STYLE, dwStyle);
		(*pSetWindowLong)(hwnd, GWL_EXSTYLE, dwExStyle);
	}
	(*pMoveWindow)(hwnd, x, y, nWidth, nHeight, FALSE);
	if(dwFlags9 & LOCKTOPZORDER) (*pSetWindowPos)(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);

	if(dwFlags10 & HIDEWINDOWCHANGES){
		(*pSetWindowLong)(hwnd, GWL_WNDPROC, lpWinCB);
	}
}

void dxwCore::FixStyle(char *ApiName, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPSTYLESTRUCT lpSS;
	lpSS = (LPSTYLESTRUCT) lParam;
	DWORD dwStyle;
#ifndef DXW_NOTRACES
	char sOldStyle[256];
	char sNewStyle[256];
	char sStyle[256];
#endif // DXW_NOTRACES

	switch (wParam) {
	case GWL_STYLE:
		dwStyle = lpSS->styleNew;
		lpSS->styleNew = FixWinStyle(lpSS->styleNew);
#ifndef DXW_NOTRACES
		OutDebugDW("%s: GWL_STYLE StyleOld=%#x(%s) StyleNew=%#x(%s)->%#x(%s)\n", 
			ApiName, 
			lpSS->styleOld, ExplainStyle(lpSS->styleOld, sOldStyle, 256),
			dwStyle, ExplainStyle(dwStyle, sStyle, 256),
			lpSS->styleNew, ExplainStyle(lpSS->styleNew, sNewStyle, 256));
#endif // DXW_NOTRACES
		break;
	case GWL_EXSTYLE:
		dwStyle = lpSS->styleNew;
		lpSS->styleNew = FixWinExStyle(lpSS->styleNew);
#ifndef DXW_NOTRACES
		OutDebugDW("%s: GWL_EXSTYLE StyleOld=%#x(%s) StyleNew=%#x(%s)->%#x(%s)\n", 
			ApiName, 
			lpSS->styleOld, ExplainExStyle(lpSS->styleOld, sOldStyle, 256),
			dwStyle, ExplainStyle(dwStyle, sStyle, 256),
			lpSS->styleNew, ExplainExStyle(lpSS->styleNew, sNewStyle, 256));
#endif // DXW_NOTRACES
		break;		
	default:
		break;
	}
}

void dxwCore::PushTimer(UINT uTimerId, UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
{
		// save current timer
		TimerEvent.dwTimerType = TIMER_TYPE_WINMM;
		TimerEvent.t.uTimerId = uTimerId;
		TimerEvent.t.uDelay = uDelay;
		TimerEvent.t.uResolution = uResolution;
		TimerEvent.t.lpTimeProc = lpTimeProc;
		TimerEvent.t.dwUser = dwUser;
		TimerEvent.t.fuEvent = fuEvent;
}

void dxwCore::PushTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
		// save current timer
		TimerEvent.dwTimerType = TIMER_TYPE_USER32;
		TimerEvent.t.hWnd = hWnd;
		TimerEvent.t.nIDEvent = nIDEvent;
		TimerEvent.t.uElapse = uElapse;
		TimerEvent.t.lpTimerFunc = lpTimerFunc;
}

void dxwCore::PopTimer(UINT uTimerId)
{
	ApiName("PopTimer");
	// clear current timer
	if(TimerEvent.dwTimerType != TIMER_TYPE_WINMM) {
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"TimerType=%#x last=%#x\n", TIMER_TYPE_WINMM, TimerEvent.dwTimerType);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE("%s: %s\n", ApiRef, msg);
		return;
	}
	if(uTimerId != TimerEvent.t.uTimerId){
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"TimerId=%#x last=%#x\n", uTimerId, TimerEvent.t.uTimerId);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE("%s: %s\n", ApiRef, msg);
		return;
	}
	TimerEvent.dwTimerType = TIMER_TYPE_NONE;
}

void dxwCore::PopTimer(HWND hWnd, UINT_PTR nIDEvent)
{
	ApiName("PopTimer");
	// clear current timer
	if(TimerEvent.dwTimerType != TIMER_TYPE_USER32) {
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"TimerType=%#x last=%#x\n", TIMER_TYPE_WINMM, TimerEvent.dwTimerType);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE("%s: %s\n", ApiRef, msg);
		return;
	}
	if(nIDEvent != TimerEvent.t.nIDEvent){
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"TimerId=%#x last=%#x\n", nIDEvent, TimerEvent.t.nIDEvent);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE("%s: %s\n", ApiRef, msg);
		return;
	}
	TimerEvent.dwTimerType = TIMER_TYPE_NONE;
}

void dxwCore::RenewTimers()
{
	ApiName("RenewTimers");
	OutTraceE("%s: RenewTimers type=%#x\n", ApiRef, TimerEvent.dwTimerType);
	switch(TimerEvent.dwTimerType){
	case TIMER_TYPE_NONE:
		OutTraceDW("%s: RenewTimers type=NONE\n", ApiRef);
		break;
	case TIMER_TYPE_USER32:
		if(pSetTimer && pKillTimer){
			UINT uElapse;
			UINT_PTR res;
			res=(*pKillTimer)(TimerEvent.t.hWnd, TimerEvent.t.nIDEvent);
#ifndef DXW_NOTRACES
			if(!res) OutTraceE("%s: KillTimer ERROR hWnd=%#x IDEvent=%#x err=%d\n", 
				ApiRef, TimerEvent.t.hWnd, TimerEvent.t.nIDEvent, GetLastError());
#endif // DXW_NOTRACES
			uElapse = dxw.StretchTime(TimerEvent.t.uElapse);
			res=(*pSetTimer)(TimerEvent.t.hWnd, TimerEvent.t.nIDEvent, uElapse, TimerEvent.t.lpTimerFunc);
			TimerEvent.t.nIDEvent = res;
#ifndef DXW_NOTRACES
			if(res){
				OutTraceDW("%s: RenewTimers type=USER32 Elsapse=%d IDEvent=%#x\n", ApiRef, uElapse, res);
			}
			else {
				OutTraceE("%s: SetTimer ERROR hWnd=%#x Elapse=%d TimerFunc=%#x err=%d\n", 
					ApiRef, TimerEvent.t.hWnd, uElapse, TimerEvent.t.lpTimerFunc, GetLastError());
			}
#endif // DXW_NOTRACES
		}
		break;
	case TIMER_TYPE_WINMM:
		if(ptimeKillEvent && ptimeSetEvent){
			UINT NewDelay;
			MMRESULT res;
			(*ptimeKillEvent)(TimerEvent.t.uTimerId);
			NewDelay = dxw.StretchTime(TimerEvent.t.uDelay);
			res=(*ptimeSetEvent)(NewDelay, TimerEvent.t.uResolution, TimerEvent.t.lpTimeProc, TimerEvent.t.dwUser, TimerEvent.t.fuEvent);
			TimerEvent.t.uTimerId = res;
			OutTraceDW("%s: RenewTimers type=WINMM Delay=%d TimerId=%#x\n", ApiRef, NewDelay, res);
		}
		break;
	default:
		OutTraceE("%s: RenewTimers type=%#x(UNKNOWN)\n", ApiRef, TimerEvent.dwTimerType);
		break;
	}
}

void dxwCore::SuspendTimeStretch(int action)
{
	ApiName("SuspendTimeStretch");
	char *status[3]={"off", "on", "redeem"};
	switch(action){
		case TIMESHIFT_OFF:
			if(TimeShiftStatus == TIMESHIFT_ON){
				SavedTimeShift = GetHookInfo()->TimeShift;
				GetHookInfo()->TimeShift = 0;
				OutTrace("%s: save tshift %d\n", ApiRef, SavedTimeShift);
			}
			else{
				OutTrace("%s: tshift already saved %d\n", ApiRef, SavedTimeShift);
			}
			break;
		case TIMESHIFT_ON:
			if((TimeShiftStatus == TIMESHIFT_OFF) || (TimeShiftStatus == TIMESHIFT_REDEEM)){
				OutTrace("%s: recover time shift %d\n", ApiRef, SavedTimeShift);
				GetHookInfo()->TimeShift = SavedTimeShift;
				SavedTimeShift = 0;
			}
			else{
				OutTrace("%s: tshift already on %d\n", ApiRef, GetHookInfo()->TimeShift);
			}
			break;
		case TIMESHIFT_REDEEM:
			TimeShiftRedeemTime = (*pGetTickCount)() + 1000; // one second more ...
			break;
		default:
			OutTrace("%s: invalid action=%d\n", ApiRef, action);
			return;
			break;
	}
	OutTrace("%s: tshift %s->%s\n", ApiRef, status[TimeShiftStatus], status[action]);
	TimeShiftStatus = action;
}

LARGE_INTEGER dxwCore::StretchLargeCounter(LARGE_INTEGER CurrentInCount)
{
	ApiName("StretchLargeCounter");
	LARGE_INTEGER CurrentOutPerfCount;
	static LARGE_INTEGER LastInPerfCount;
	static LARGE_INTEGER LastOutPerfCount;
	static BOOL FirstTime = TRUE;
	LARGE_INTEGER ElapsedCount;
	static BOOL TimeStretch;

	if(FirstTime){
		// first time through, initialize both inner and output per counters with real values
		LastInPerfCount.QuadPart = CurrentInCount.QuadPart;
		LastOutPerfCount.QuadPart = CurrentInCount.QuadPart;
		FirstTime=FALSE;
		TimeStretch = dwFlags2 & TIMESTRETCH;
	}

	ElapsedCount.QuadPart = CurrentInCount.QuadPart - LastInPerfCount.QuadPart;
	if(TimeStretch) ElapsedCount = dxw.StretchCounter(ElapsedCount);
	CurrentOutPerfCount.QuadPart = LastOutPerfCount.QuadPart + ElapsedCount.QuadPart;
	LastInPerfCount = CurrentInCount;
	LastOutPerfCount = CurrentOutPerfCount;

	OutDebugDW("%s: Count=[%#x-%#x]\n", ApiRef, CurrentOutPerfCount.HighPart, CurrentOutPerfCount.LowPart);
	return CurrentOutPerfCount;
}

UINT VKeyConfig[DXVK_SIZE];

static char *VKeyLabels[DXVK_SIZE]={
	"none",
	"cliptoggle", 
	"refresh", 
	"logtoggle", 
	"plocktoggle",
	"fpstoggle", 
	"timefast", 
	"timeslow", 
	"timetoggle", 
	"altf4",
	"printscreen",
	"corner",
	"freezetime",
	"fullscreen",
	"workarea",
	"desktop",
	"custom",
	"cdprev",
	"cdnext",
	"zoomin",
	"zoomout"
};

void dxwCore::MapKeysInit()
{
	ApiName("MapKeysInit");
	char InitPath[MAX_PATH];
	int KeyIdx;
	// v2.05.10 fix: when loaded by proxy, hot key definitions must be searched in dxwnd.dxw
	sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dwIndex == -1) ? "dxw" : "ini");
	VKeyConfig[DXVK_NONE]=DXVK_NONE;
	for(KeyIdx=1; KeyIdx<DXVK_SIZE; KeyIdx++){
		VKeyConfig[KeyIdx]=(*pGetPrivateProfileIntA)("keymapping", VKeyLabels[KeyIdx], KeyIdx==DXVK_ALTF4 ? 0x73 : 0x00, InitPath);
#ifndef DXW_NOTRACES
		if(VKeyConfig[KeyIdx]) OutTrace("%s: keymapping[%d](%s)=%#x\n", ApiRef, KeyIdx, VKeyLabels[KeyIdx], VKeyConfig[KeyIdx]);
#endif // DXW_NOTRACES
	}
}

UINT dxwCore::MapKeysConfig(UINT message, LPARAM lparam, WPARAM wparam)
{
	ApiName("MapKeysConfig");
	if(message!=WM_SYSKEYDOWN) return DXVK_NONE;
	for(int idx=1; idx<DXVK_SIZE; idx++)
		if(VKeyConfig[idx]==wparam) {
			OutTrace("%s: keymapping GOT=0x%02X idx=%d(%s)\n", ApiRef, wparam, idx, VKeyLabels[idx]);
			return idx;
		}
	return DXVK_NONE;
}

void dxwCore::ToggleFreezedTime()
{
	ApiName("ToggleFreezedTime");
	static DWORD dwLastTime = 0;
	if(((*pGetTickCount)() - dwLastTime) < 1000) return;
	TimeFreeze = !TimeFreeze;
	dwLastTime = (*pGetTickCount)();
	OutTraceDW("%s: time is %s\n", ApiRef, dxw.TimeFreeze ? "freezed" : "unfreezed");
}

void dxwCore::MessagePump()
{
	ApiName("MessagePump");
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
		OutTraceW("%s: msg=%#x l-wParam=(%#x,%#x)\n", ApiRef, msg.message, msg.lParam, msg.wParam);
		if((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST)) break; // do not consume keyboard inputs
		if((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST)) break; // do not consume mouse inputs
		if(msg.message == WM_QUIT) break; // v2.06.03: do not ignore WM_QUIT !!! @#@ "Wacky races"
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void dxwCore::Mark(HDC hdc, BOOL scale, COLORREF color, int x, int y, int cx, int cy)
{
	RECT frame;
	HBRUSH brush = CreateSolidBrush(color);
	frame.left = x; frame.top = y;
	frame.right = x+cx; frame.bottom = y+cy;
	if(scale) dxw.MapClient(&frame);
	(*pFrameRect)(hdc, &frame, brush);
	DeleteObject(brush);
}

void dxwCore::Mark(HDC hdc, BOOL scale, COLORREF color, RECT frame)
{
	HBRUSH brush = CreateSolidBrush(color);
	if(scale) dxw.MapClient(&frame);
	(*pFrameRect)(hdc, &frame, brush);
	DeleteObject(brush);
}

void dxwCore::ScreenShot(void)
{
	ApiName("ScreenShot");
	extern void DumpHDC(HDC, int, int, int, int);
	static BOOL DoOnce = TRUE;
	static int hash = 0;
	char pszFile[MAX_PATH];

	if(DoOnce){
		sprintf_s(pszFile, MAX_PATH, "%s\\screenshot.out", GetDxWndPath());
		CreateDirectory(pszFile, NULL);
		while(TRUE){	
			sprintf_s(pszFile, MAX_PATH, "%s\\screenshot.out\\shot.%08d.bmp", GetDxWndPath(), hash);
			if(GetFileAttributes(pszFile) == INVALID_FILE_ATTRIBUTES) break;
			hash++;
		}
		DoOnce = FALSE;
	}

	if(iPrimarySurfaceVersion > 0){
		// V2.05.10 fix: use iPrimarySurfaceVersion intead of dxw.dwDDVersion !!
		// fixes "Dreams to Raality" screenshots. The game created a DirectDraw1
		// session but after that queryes tto DirectDraw2 before creating the 
		// primary surface
		extern void DDrawScreenShot(int, int);
		OutTrace("%s: DDrawScreenShot(%d) hash=%d\n", ApiRef, iPrimarySurfaceVersion, hash);
		DDrawScreenShot(iPrimarySurfaceVersion, hash++);
	}
	else if(dxw.GDIEmulationMode == GDIMODE_EMULATED) {
		extern int DCScreenShot(HDC, int);
		HWND hwnd = dxw.GethWnd();
		HDC hdc = dxw.AcquireEmulatedDC(hwnd);
		OutTrace("%s: DCScreenShot(%#x) hash=%d\n", ApiRef, hdc, hash);
		DCScreenShot(hdc, hash++);
		dxw.ReleaseEmulatedDC(hwnd);
	}
	else {
		// when no ddraw surface is available, this routine takes a window screenshot
		// pretty much like Alt-PrintScreen, so with the desktop color depth, no virtual
		// palette and real window size.
		extern int WinScreenShot(HWND, int);
		OutTrace("%s: WinScreenShot(%#x) hash=%d\n", ApiRef, GethWnd(), hash);
		WinScreenShot(GethWnd(), hash++);
	}
}

void dxwCore::Project(HDC hdc)
{
	ApiName("Project");
	HWND hwnd, wdesktop;
	RECT rect;
	HDC hdesktop;
	OutTraceDW("%s: hdc=%#x\n", ApiRef, hdc);
	if(!(pGetClientRect && pGetDesktopWindow && pGDIGetDC && pGDIStretchBlt && pGDIReleaseDC)) return;
	hwnd=WindowFromDC(hdc);
	(*pGetClientRect)(hwnd, &rect);
	wdesktop = (*pGetDesktopWindow)();
	hdesktop = (*pGDIGetDC)(wdesktop);
	OutTraceDW("%s: hwnd=%#x rect=(%d,%d)-(%d,%d) hdesktop=%#x\n", ApiRef, hdc, rect.left, rect.top, rect.right, rect.bottom, hdesktop);
	if(!(*pGDIStretchBlt)(hdesktop, dxw.iPosX, dxw.iPosY, dxw.iSizX, dxw.iSizY, hdc, 0, 0, rect.right, rect.bottom, SRCCOPY)){
		OutTraceDW("%s: StretchBlt ERROR err=%d\n", ApiRef, GetLastError());
	}
	(*pGDIReleaseDC)(wdesktop, hdesktop);
}

void dxwCore::Project()
{
	ApiName("Project");
	HWND hwnd, wdesktop;
	RECT rect;
	HDC hdc, hdesktop;
	OutTraceDW("%d: (void)\n", ApiRef);
	if(!(pGetClientRect && pGetDesktopWindow && pGDIGetDC && pGDIStretchBlt && pGDIReleaseDC)) return;
	if(!(hwnd=dxw.GethWnd())) return;
	(*pGetClientRect)(hwnd, &rect);
	wdesktop = (*pGetDesktopWindow)();
	hdesktop = (*pGDIGetDC)(wdesktop);
	hdc = (*pGDIGetDC)(hwnd);
	OutTraceDW("%s: hwnd=%#x rect=(%d,%d)-(%d,%d) hdesktop=%#x\n", ApiRef, hdc, rect.left, rect.top, rect.right, rect.bottom, hdesktop);
	if(!(*pGDIStretchBlt)(hdesktop, dxw.iPosX, dxw.iPosY, dxw.iSizX, dxw.iSizY, hdc, 0, 0, rect.right, rect.bottom, SRCCOPY)){
		OutTraceDW("%s: StretchBlt ERROR err=%d\n", ApiRef, GetLastError());
	}
	(*pGDIReleaseDC)(hwnd, hdc);
	(*pGDIReleaseDC)(wdesktop, hdesktop);
}

BOOL WINAPI EnumCB(GUID FAR *lpGuid, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm)
{
	ApiName("EnumCB");
	int DevId = (int)lpContext;
	CHAR MatchDev[20];
	sprintf(MatchDev, "DISPLAY%d", DevId+1);
	if(strstr(lpDriverName, MatchDev) != NULL){
		dxw.lpFakePrimaryMonitorGUID = (LPGUID)malloc(sizeof(GUID));
		memcpy((LPVOID)dxw.lpFakePrimaryMonitorGUID, (LPVOID)lpGuid, sizeof(GUID));
		OutTrace("%s: DDRAW forced DISPLAY%d GUID=%s hmon=%#x\n", ApiRef, DevId+1, sGUID(lpGuid), hm);
		return FALSE; // stop here
	}

	return TRUE; // continue
}

LPGUID dxwCore::GetForcedMonitorGUID(void)
{
	extern DirectDrawEnumerateExA_Type pDirectDrawEnumerateExA;
	if(!lpFakePrimaryMonitorGUID){
		if(!pDirectDrawEnumerateExA) {
			HMODULE DDrawLib;
			DDrawLib=(*pLoadLibraryA)("ddraw");
			if(!DDrawLib) return NULL;
			pDirectDrawEnumerateExA=(DirectDrawEnumerateExA_Type)(*pGetProcAddress)(DDrawLib,"DirectDrawEnumerateExA");
		}
		(*pDirectDrawEnumerateExA)((LPDDENUMCALLBACKEXA)EnumCB, (LPVOID)MonitorId, DDENUM_ATTACHEDSECONDARYDEVICES);
	}
	return lpFakePrimaryMonitorGUID;
}

#ifndef D3DGAMMARAMP
#define D3DGAMMARAMP WORD[256*3]
#endif

void dxwCore::InitGammaRamp()
{
	HDC hDCDesktop = (*pGDIGetDC)(NULL);
	dxw.pInitialRamp = malloc(sizeof(D3DGAMMARAMP));
	dxw.pCurrentRamp = malloc(sizeof(D3DGAMMARAMP));
	if(!pGDIGetDeviceGammaRamp) pGDIGetDeviceGammaRamp = GetDeviceGammaRamp;
	(*pGDIGetDeviceGammaRamp)(hDCDesktop, dxw.pInitialRamp);
	(*pGDIGetDeviceGammaRamp)(hDCDesktop, dxw.pCurrentRamp);
	(*pGDIReleaseDC)(NULL, hDCDesktop);
}

void dxwCore::SaveCurrentDirectory(void)
{
	ApiName("SaveCurrentDirectory");
	do { // fake loop
		DWORD mapping;
		int len = (*pGetCurrentDirectoryA)(0, NULL);
		if(len == 0) break;
		char *fullBuffer = (char *)malloc(len + 1); // len + terminator
		len = (*pGetCurrentDirectoryA)(len, fullBuffer);
		if(len == 0) break;
		//OutTraceDW("%s: GetCurrentDirectory actual ret=%d buf=\"%s\"\n", ApiRef, len, fullBuffer);
		char *lpPathName = (char *)dxwUntranslatePathA((LPCSTR)fullBuffer, &mapping);
		if(strlen(lpPathName) == 0) break;
		char drive = toupper(lpPathName[0]);
		if(drive < 'A') break;
		if(drive > 'Z') break;
		int index = drive - 'A';
		char *p = dxw.CurrDirectories[index];
		if(p) free(p);
		p = (char *)malloc(strlen(lpPathName)+1);
		strcpy(p, lpPathName);
		dxw.CurrDirectories[index] = p;
		OutTraceDW("%s: saved current folder for drive %c: path=\"%s\"\n", ApiRef, drive, lpPathName);
		free(fullBuffer);
		//for(int c='A';  c<='Z'; c++) OutTrace("dump[%c]=%s\n", c, dxw.CurrDirectories[c-'A']);
	} while(FALSE);
}