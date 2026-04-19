#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <stdio.h>
#include <stdlib.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dsound.h"
#include "dxhelper.h"
#include "comvsound.h"

#define GLOBALFOCUSMASK (GLOBALFOCUSON | GLOBALFOCUSOFF)
//#define TRACEALL
#ifdef TRACEALL
#define TRACEALLSOUNDS
#endif

#ifndef DXW_NOTRACES
#define TraceDSError(e) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, e, ExplainDDError(e))
#else
#define TraceDSError(e)
#endif

//----------------------------------------------------------------------//
// Hookers typedefs, prototypes, pointers
//----------------------------------------------------------------------//

typedef HRESULT	(WINAPI *DirectSoundCreate_Type)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
typedef HRESULT (WINAPI *DirectSoundCreate8_Type)(LPCGUID, LPDIRECTSOUND8 *, LPUNKNOWN);
typedef HRESULT	(WINAPI *SetCooperativeLevel_Type)  (void *, HWND, DWORD);
typedef HRESULT (WINAPI *DSGetCaps_Type)(void *, LPDSCAPS);
typedef HRESULT (WINAPI *CreateSoundBuffer_Type) (void *, LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER *, LPUNKNOWN);
typedef HRESULT (WINAPI *DuplicateSoundBuffer_Type)(void *, LPDIRECTSOUNDBUFFER, LPDIRECTSOUNDBUFFER *);
typedef HRESULT (WINAPI *DirectSoundEnumerateA_Type)(LPDSENUMCALLBACKA, LPVOID);
typedef HRESULT (WINAPI *DirectSoundEnumerateW_Type)(LPDSENUMCALLBACKW, LPVOID);
typedef HRESULT (WINAPI *GetSpeakerConfig_Type)(void *, LPDWORD);
typedef HRESULT (WINAPI *SetSpeakerConfig_Type)(void *, DWORD);
typedef HRESULT (WINAPI *DSInitialize_Type)(void *, LPCGUID);
typedef HRESULT (WINAPI *Play_Type)(void *, DWORD, DWORD, DWORD);
typedef HRESULT (WINAPI *DSBSetCurrentPosition_Type)(void *, DWORD);
typedef HRESULT (WINAPI *DSBSetVolume_Type)(void *, LONG);
typedef HRESULT (WINAPI *Void_Type)(void *);
typedef HRESULT (WINAPI *DSBLock_Type)(void *, DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD);
typedef HRESULT (WINAPI *DSBUnlock_Type)(void *, LPVOID, DWORD, LPVOID, DWORD);
typedef HRESULT (WINAPI *DSBInitialize_Type)(void *, LPDIRECTSOUND, LPCDSBUFFERDESC1);
typedef HRESULT (WINAPI *DSBGetCaps_Type)(void *, LPDSBCAPS);
typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID, LPVOID *);
typedef HRESULT (WINAPI *DSCompact_Type)(void *);
typedef HRESULT (WINAPI *DSBGetCurrentPosition_Type)(void *, LPDWORD, LPDWORD);
typedef HRESULT (WINAPI *DSBGetFormat_Type)(void *, LPWAVEFORMATEX, DWORD, LPDWORD);
typedef HRESULT (WINAPI *DSBGetVolume_Type)(void *, LPLONG);
typedef HRESULT (WINAPI *DSBGetPan_Type)(void *, LPLONG);
typedef HRESULT (WINAPI *DSBGetFrequency_Type)(void *, LPDWORD);
typedef HRESULT (WINAPI *DSBGetStatus_Type)(void *, LPDWORD);
typedef HRESULT (WINAPI *DSBSetFormat_Type)(void *, LPCWAVEFORMATEX);
typedef HRESULT (WINAPI *DSBSetPan_Type)(void *, LONG);
typedef HRESULT (WINAPI *DSBSetFrequency_Type)(void *, DWORD);
typedef ULONG (WINAPI *DSRelease_Type)(void *);

// DirectSound API method pointers

DirectSoundCreate_Type pDirectSoundCreate = NULL;
DirectSoundCreate8_Type pDirectSoundCreate8 = NULL;
#ifdef TRACEALLSOUNDS
DirectSoundEnumerateA_Type pDirectSoundEnumerateA ;
DirectSoundEnumerateW_Type pDirectSoundEnumerateW;
#endif // TRACEALLSOUNDS

// IDirectSound method pointers 

SetCooperativeLevel_Type pDSSetCooperativeLevel = NULL;
CreateSoundBuffer_Type pDSCreateSoundBuffer = NULL;
DuplicateSoundBuffer_Type pDSDuplicateSoundBuffer = NULL;
DSGetCaps_Type pDSGetCaps = NULL;
#ifdef TRACEALLSOUNDS
QueryInterface_Type pDSQueryInterface;
GetSpeakerConfig_Type pDSGetSpeakerConfig = NULL;
SetSpeakerConfig_Type pDSSetSpeakerConfig = NULL;
DSInitialize_Type pDSInitialize = NULL;
DSCompact_Type pDSCompact;
DSRelease_Type pDSRelease;
#endif // TRACEALLSOUNDS

// IDirectSoundBuffer method pointers

Play_Type pDSSBPlay, pDSPBPlay;
DSBSetVolume_Type pDSSBSetVolume, pDSPBSetVolume;
DSBUnlock_Type pDSSBUnlock, pDSPBUnlock;
DSRelease_Type pDSPBRelease, pDSSBRelease;
DSBSetPan_Type pDSSBSetPan, pDSPBSetPan;
DSBGetCaps_Type pDSSBGetCaps, pDSPBGetCaps;
DSBGetPan_Type pDSSBGetPan, pDSPBGetPan;
#ifdef TRACEALLSOUNDS
Void_Type pDSSBStop, pDSPBStop;
Void_Type pDSSBRestore, pDSPBRestore;
DSBSetCurrentPosition_Type pDSSBSetCurrentPosition, pDSPBSetCurrentPosition;
DSBInitialize_Type pDSSBInitialize, pDSPBInitialize;
QueryInterface_Type pDSSBQueryInterface, pDSPBQueryInterface;
DSBGetCurrentPosition_Type pDSSBGetCurrentPosition, pDSPBGetCurrentPosition;
DSBGetFormat_Type pDSSBGetFormat, pDSPBGetFormat;
DSBGetVolume_Type pDSSBGetVolume, pDSPBGetVolume;
DSBGetFrequency_Type pDSSBGetFrequency, pDSPBGetFrequency;
DSBGetStatus_Type pDSSBGetStatus, pDSPBGetStatus;
DSBLock_Type pDSSBLock, pDSPBLock;
DSBSetFormat_Type pDSSBSetFormat, pDSPBSetFormat;
DSBSetFrequency_Type pDSSBSetFrequency, pDSPBSetFrequency;
#endif // TRACEALLSOUNDS

// IDirectSoundNotify methods

typedef HRESULT (WINAPI *SetNotificationPositions_Type)(DWORD, LPCDSBPOSITIONNOTIFY);
SetNotificationPositions_Type pDSNSetNotificationPositions;
HRESULT WINAPI extDSNSetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

// DirectSound API wrappers

HRESULT WINAPI extDirectSoundCreate(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
HRESULT WINAPI extDirectSoundCreate8(LPCGUID, LPDIRECTSOUND8 *, LPUNKNOWN);
#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDirectSoundEnumerateA(LPDSENUMCALLBACKA, LPVOID);
HRESULT WINAPI extDirectSoundEnumerateW(LPDSENUMCALLBACKW, LPVOID);
#endif // TRACEALLSOUNDS

// IDirectSound method wrappers 

HRESULT WINAPI extDSSetCooperativeLevel(void *, HWND, DWORD);
HRESULT WINAPI extDSCreateSoundBuffer(void *, LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER *, LPUNKNOWN);
HRESULT WINAPI extDSDuplicateSoundBuffer(void *, LPDIRECTSOUNDBUFFER, LPDIRECTSOUNDBUFFER *);
HRESULT WINAPI extDSGetCaps(void *, LPDSCAPS);
#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSQueryInterface(void *, REFIID, LPVOID *);
HRESULT WINAPI extDSGetSpeakerConfig(void *, LPDWORD);
HRESULT WINAPI extDSSetSpeakerConfig(void *, DWORD);
HRESULT WINAPI extDSInitialize(void *, LPCGUID);
HRESULT WINAPI extDSCompact(void *);
ULONG	WINAPI extDSRelease(void *);
#endif // TRACEALLSOUNDS

// IDirectSoundBuffer method wrappers 

HRESULT WINAPI extDSPBPlay(void *, DWORD, DWORD, DWORD);
HRESULT WINAPI extDSSBPlay(void *, DWORD, DWORD, DWORD);
HRESULT WINAPI extDSSBSetVolume(void *, LONG);
HRESULT WINAPI extDSPBSetVolume(void *, LONG);
HRESULT WINAPI extDSSBUnlock(void *, LPVOID, DWORD, LPVOID, DWORD);
HRESULT WINAPI extDSPBUnlock(void *, LPVOID, DWORD, LPVOID, DWORD);
HRESULT WINAPI extDSSBSetPan(void *, LONG);
HRESULT WINAPI extDSPBSetPan(void *, LONG);
ULONG	WINAPI extDSPBRelease(void *);
ULONG	WINAPI extDSSBRelease(void *);
HRESULT WINAPI extDSSBGetCaps(void *, LPDSBCAPS);
HRESULT WINAPI extDSPBGetCaps(void *, LPDSBCAPS);
HRESULT WINAPI extDSSBGetPan(void *, LPLONG);
HRESULT WINAPI extDSPBGetPan(void *, LPLONG);

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSSBStop(void *);
HRESULT WINAPI extDSPBStop(void *);
HRESULT WINAPI extDSSBRestore(void *);
HRESULT WINAPI extDSPBRestore(void *);
HRESULT WINAPI extDSSBSetCurrentPosition(void *, DWORD);
HRESULT WINAPI extDSPBSetCurrentPosition(void *, DWORD);
HRESULT WINAPI extDSSBInitialize(void *, LPDIRECTSOUND, LPCDSBUFFERDESC1);
HRESULT WINAPI extDSPBInitialize(void *, LPDIRECTSOUND, LPCDSBUFFERDESC1);
HRESULT WINAPI extDSSBQueryInterface(void *, REFIID, LPVOID *);
HRESULT WINAPI extDSPBQueryInterface(void *, REFIID, LPVOID *);
HRESULT WINAPI extDSSBLock(void *, DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD);
HRESULT WINAPI extDSPBLock(void *, DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD);
HRESULT WINAPI extDSSBGetCurrentPosition(void *, LPDWORD, LPDWORD);
HRESULT WINAPI extDSPBGetCurrentPosition(void *, LPDWORD, LPDWORD);
HRESULT WINAPI extDSSBGetFormat(void *, LPWAVEFORMATEX, DWORD, LPDWORD);
HRESULT WINAPI extDSPBGetFormat(void *, LPWAVEFORMATEX, DWORD, LPDWORD);
HRESULT WINAPI extDSSBGetVolume(void *, LPLONG);
HRESULT WINAPI extDSPBGetVolume(void *, LPLONG);
HRESULT WINAPI extDSSBGetFrequency(void *, LPDWORD);
HRESULT WINAPI extDSPBGetFrequency(void *, LPDWORD);
HRESULT WINAPI extDSSBGetStatus(void *, LPDWORD);
HRESULT WINAPI extDSPBGetStatus(void *, LPDWORD);
HRESULT WINAPI extDSSBSetFormat(void *, LPCWAVEFORMATEX);
HRESULT WINAPI extDSPBSetFormat(void *, LPCWAVEFORMATEX);
HRESULT WINAPI extDSSBSetFrequency(void *, DWORD);
HRESULT WINAPI extDSPBSetFrequency(void *, DWORD);
#endif // TRACEALLSOUNDS

#ifndef DSSCL_NORMAL
#define DSSCL_NORMAL                0x00000001
#define DSSCL_PRIORITY              0x00000002
#define DSSCL_EXCLUSIVE             0x00000003
#define DSSCL_WRITEPRIMARY          0x00000004
#endif

LPDIRECTSOUNDBUFFER pDSPrimaryBuffer = NULL;
DWORD dsForcedCapability = 0;

//----------------------------------------------------------------------//
// Hooking procedures
//----------------------------------------------------------------------//

static HookEntryEx_Type Hooks[]={
	{HOOK_HOT_CANDIDATE, 0x0001, "DirectSoundCreate", (FARPROC)NULL, (FARPROC *)&pDirectSoundCreate, (FARPROC)extDirectSoundCreate},
	{HOOK_HOT_CANDIDATE, 0x000B, "DirectSoundCreate8", (FARPROC)NULL, (FARPROC *)&pDirectSoundCreate8, (FARPROC)extDirectSoundCreate8},
#ifdef TRACEALLSOUNDS
	{HOOK_HOT_CANDIDATE, 0x0002, "DirectSoundEnumerateA", (FARPROC)NULL, (FARPROC *)&pDirectSoundEnumerateA, (FARPROC)extDirectSoundEnumerateA},
	{HOOK_HOT_CANDIDATE, 0x0003, "DirectSoundEnumerateW", (FARPROC)NULL, (FARPROC *)&pDirectSoundEnumerateW, (FARPROC)extDirectSoundEnumerateW},
#endif // TRACEALLSOUNDS
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

FARPROC Remap_DSound_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

static char *libname = "dsound.dll";

void HookDirectSound(HMODULE hModule)
{
	HookLibraryEx(hModule, Hooks, "dsound.dll");

	if(dxw.dwFlags13 & DSOUNDREPLACE){
		char path[MAX_PATH];
		sprintf(path, "%salt.dll\\dsound.dll", GetDxWndPath());
		PinLibraryEx(Hooks, path);
		OutTrace("HookDirectSound: alt.dll loaded path=%s\n", path);
	}
}

void HookDirectSoundObj(LPDIRECTSOUND *lpds)
{
	// v2.03.99: check or DirectSound could be hooked through CoCreateInstance !!
	if(!(dxw.dwFlags7 & HOOKDIRECTSOUND)) return;
	// IDirectSound::SetCooperativeLevel
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)lpds +  0), extDSQueryInterface, (void **)&pDSQueryInterface, "QueryInterface");
	SetHook((void *)(**(DWORD **)lpds +  8), extDSRelease, (void **)&pDSRelease, "Release");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)lpds + 12), extDSCreateSoundBuffer, (void **)&pDSCreateSoundBuffer, "CreateSoundBuffer");
	SetHook((void *)(**(DWORD **)lpds + 16), extDSGetCaps, (void **)&pDSGetCaps, "GetCaps");
	SetHook((void *)(**(DWORD **)lpds + 20), extDSDuplicateSoundBuffer, (void **)&pDSDuplicateSoundBuffer, "DuplicateSoundBuffer");
	SetHook((void *)(**(DWORD **)lpds + 24), extDSSetCooperativeLevel, (void **)&pDSSetCooperativeLevel, "SetCooperativeLevel(DSound)");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)lpds + 28), extDSCompact, (void **)&pDSCompact, "Compact");
	SetHook((void *)(**(DWORD **)lpds + 32), extDSGetSpeakerConfig, (void **)&pDSGetSpeakerConfig, "GetSpeakerConfig(DSound)");
	SetHook((void *)(**(DWORD **)lpds + 36), extDSSetSpeakerConfig, (void **)&pDSSetSpeakerConfig, "SetSpeakerConfig(DSound)");
	SetHook((void *)(**(DWORD **)lpds + 40), extDSInitialize, (void **)&pDSInitialize, "Initialize(DSound)");
#endif // TRACEALLSOUNDS
}

void HookDirectSound8Obj(LPDIRECTSOUND8 *lpds)
{
	HookDirectSoundObj((LPDIRECTSOUND *)lpds);
	//    STDMETHOD(VerifyCertification)  (THIS_ LPDWORD pdwCertified) PURE;
	//	SetHook((void *)(**(DWORD **)lpds + 44), extDSVerifyCertification, (void **)&pDSVerifyCertification, "VerifyCertification(DSound)");
}

// naming: DSB: DirectSound Buffer, DSPB: DirectSound Primary Buffer, DSSB: DirectSound Secondary Buffer

void HookSoundBuffer(LPDIRECTSOUNDBUFFER *ppDSBuffer)
{
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer +  0), extDSSBQueryInterface, (void **)&pDSSBQueryInterface, "QueryInterface");
	SetHook((void *)(**(DWORD **)ppDSBuffer +  8), extDSSBRelease, (void **)&pDSSBRelease, "Release");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 12), extDSSBGetCaps, (void **)&pDSSBGetCaps, "GetCaps");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 16), extDSSBGetCurrentPosition, (void **)&pDSSBGetCurrentPosition, "GetCurrentPosition");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 20), extDSSBGetFormat, (void **)&pDSSBGetFormat, "GetFormat");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 24), extDSSBGetVolume, (void **)&pDSSBGetVolume, "GetVolume");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 28), extDSSBGetPan, (void **)&pDSSBGetPan, "GetPan");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 32), extDSSBGetFrequency, (void **)&pDSSBGetFrequency, "GetFrequency");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 36), extDSSBGetStatus, (void **)&pDSSBGetStatus, "GetStatus");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 40), extDSSBInitialize, (void **)&pDSSBInitialize, "Initialize");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 44), extDSSBLock, (void **)&pDSSBLock, "Lock");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 48), extDSSBPlay, (void **)&pDSSBPlay, "Play");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 52), extDSSBSetCurrentPosition, (void **)&pDSSBSetCurrentPosition, "SetCurrentPosition");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 56), extDSSBSetFormat, (void **)&pDSSBSetFormat, "SetFormat");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 60), extDSSBSetVolume, (void **)&pDSSBSetVolume, "SetVolume");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 64), extDSSBSetPan, (void **)&pDSSBSetPan, "SetPan");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 68), extDSSBSetFrequency, (void **)&pDSSBSetFrequency, "SetFrequency");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 72), extDSSBStop, (void **)&pDSSBStop, "Stop");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 76), extDSSBUnlock, (void **)&pDSSBUnlock, "Unlock");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 80), extDSSBRestore, (void **)&pDSSBRestore, "Restore");
#endif // TRACEALLSOUNDS
}
void HookPrimaryBuffer(LPDIRECTSOUNDBUFFER *ppDSBuffer)
{
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer +  0), extDSPBQueryInterface, (void **)&pDSPBQueryInterface, "QueryInterface");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer +  8), extDSPBRelease, (void **)&pDSPBRelease, "Release");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 12), extDSPBGetCaps, (void **)&pDSPBGetCaps, "GetCaps");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 16), extDSPBGetCurrentPosition, (void **)&pDSPBGetCurrentPosition, "GetCurrentPosition");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 20), extDSPBGetFormat, (void **)&pDSPBGetFormat, "GetFormat");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 24), extDSPBGetVolume, (void **)&pDSPBGetVolume, "GetVolume");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 28), extDSPBGetPan, (void **)&pDSPBGetPan, "GetPan");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 32), extDSPBGetFrequency, (void **)&pDSPBGetFrequency, "GetFrequency");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 36), extDSPBGetStatus, (void **)&pDSPBGetStatus, "GetStatus");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 40), extDSPBInitialize, (void **)&pDSPBInitialize, "Initialize");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 44), extDSPBLock, (void **)&pDSPBLock, "Lock");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 48), extDSPBPlay, (void **)&pDSPBPlay, "Play");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 52), extDSPBSetCurrentPosition, (void **)&pDSPBSetCurrentPosition, "SetCurrentPosition");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 56), extDSPBSetFormat, (void **)&pDSPBSetFormat, "SetFormat");
#endif
	SetHook((void *)(**(DWORD **)ppDSBuffer + 60), extDSPBSetVolume, (void **)&pDSPBSetVolume, "SetVolume");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 64), extDSPBSetPan, (void **)&pDSPBSetPan, "SetPan");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 68), extDSPBSetFrequency, (void **)&pDSPBSetFrequency, "SetFrequency");
	SetHook((void *)(**(DWORD **)ppDSBuffer + 72), extDSPBStop, (void **)&pDSPBStop, "Stop");
#endif // TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 76), extDSPBUnlock, (void **)&pDSPBUnlock, "Unlock");
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSBuffer + 80), extDSPBRestore, (void **)&pDSPBRestore, "Restore");
#endif // TRACEALLSOUNDS
}

void HookSoundNotify(LPDIRECTSOUNDNOTIFY *ppDSNotify)
{
#ifdef TRACEALLSOUNDS
	SetHook((void *)(**(DWORD **)ppDSNotify + 12), extDSNSetNotificationPositions, (void **)&pDSNSetNotificationPositions, "SetNotificationPositions");
#endif // TRACEALLSOUNDS
}

//----------------------------------------------------------------------//
// Auxiliary functions
//----------------------------------------------------------------------//

static LONG gPan = DSBPAN_CENTER;

//----------------------------------------------------------------------//
// Tracing
//----------------------------------------------------------------------//

#ifndef DXW_NOTRACES
extern void strscat(char *, int, char *);

static char *ExplainLevel(DWORD lev)
{
	char *s;
	switch(lev){
		case DSSCL_NORMAL: s="NORMAL"; break;
		case DSSCL_PRIORITY: s="PRIORITY"; break;
		case DSSCL_EXCLUSIVE: s="EXCLUSIVE"; break;
		case DSSCL_WRITEPRIMARY: s="WRITEPRIMARY"; break;
		default: s="invalid"; break;
	}
	return s;
}

static char *ExplainDSFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DSCAPS_", eblen);
	if (c & DSCAPS_PRIMARYMONO) strscat(eb, eblen, "PRIMARYMONO+");
	if (c & DSCAPS_PRIMARYSTEREO) strscat(eb, eblen, "PRIMARYSTEREO+");
	if (c & DSCAPS_PRIMARY8BIT) strscat(eb, eblen, "PRIMARY8BIT+");
	if (c & DSCAPS_PRIMARY16BIT) strscat(eb, eblen, "PRIMARY16BIT+");
	if (c & DSCAPS_CONTINUOUSRATE) strscat(eb, eblen, "CONTINUOUSRATE+");
	if (c & DSCAPS_EMULDRIVER) strscat(eb, eblen, "EMULDRIVER+");
	if (c & DSCAPS_CERTIFIED) strscat(eb, eblen, "CERTIFIED+");
	if (c & DSCAPS_SECONDARYMONO) strscat(eb, eblen, "SECONDARYMONO+");
	if (c & DSCAPS_SECONDARYSTEREO) strscat(eb, eblen, "SECONDARYSTEREO+");
	if (c & DSCAPS_SECONDARY8BIT) strscat(eb, eblen, "SECONDARY8BIT+");
	if (c & DSCAPS_SECONDARY16BIT) strscat(eb, eblen, "SECONDARY16BIT+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DSBCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDSBFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DSBCAPS_", eblen);
	if (c & DSBCAPS_PRIMARYBUFFER) strscat(eb, eblen,  "PRIMARYBUFFER+");
	if (c & DSBCAPS_STATIC) strscat(eb, eblen,  "STATIC+");
	if (c & DSBCAPS_LOCHARDWARE) strscat(eb, eblen,  "LOCHARDWARE+");
	if (c & DSBCAPS_LOCSOFTWARE) strscat(eb, eblen,  "LOCSOFTWARE+");
	if (c & DSBCAPS_CTRL3D) strscat(eb, eblen,  "CTRL3D+");
	if (c & DSBCAPS_CTRLFREQUENCY) strscat(eb, eblen,  "CTRLFREQUENCY+");
	if (c & DSBCAPS_CTRLPAN) strscat(eb, eblen,  "CTRLPAN+");
	if (c & DSBCAPS_CTRLVOLUME) strscat(eb, eblen,  "CTRLVOLUME+");
	if (c & DSBCAPS_CTRLPOSITIONNOTIFY) strscat(eb, eblen,  "CTRLPOSITIONNOTIFY+");
	if (c & DSBCAPS_CTRLFX) strscat(eb, eblen,  "CTRLFX+");
	if (c & DSBCAPS_STICKYFOCUS) strscat(eb, eblen,  "STICKYFOCUS+");
	if (c & DSBCAPS_GLOBALFOCUS) strscat(eb, eblen,  "GLOBALFOCUS+");
	if (c & DSBCAPS_GETCURRENTPOSITION2) strscat(eb, eblen,  "GETCURRENTPOSITION2+");
	if (c & DSBCAPS_MUTE3DATMAXDISTANCE) strscat(eb, eblen,  "MUTE3DATMAXDISTANCE+");
	if (c & DSBCAPS_LOCDEFER) strscat(eb, eblen,  "LOCDEFER+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DSBCAPS_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

static char *ExplainPlayFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DSBPLAY_", eblen);
	if (c & DSBPLAY_LOOPING) strscat(eb, eblen, "LOOPING+");
	if (c & DSBPLAY_LOCHARDWARE) strscat(eb, eblen, "LOCHARDWARE+");
	if (c & DSBPLAY_LOCSOFTWARE) strscat(eb, eblen, "LOCSOFTWARE+");
	if (c & DSBPLAY_TERMINATEBY_TIME) strscat(eb, eblen, "TERMINATEBY_TIME+");
	if (c & DSBPLAY_TERMINATEBY_DISTANCE) strscat(eb, eblen, "TERMINATEBY_DISTANCE+");
	if (c & DSBPLAY_TERMINATEBY_PRIORITY) strscat(eb, eblen, "TERMINATEBY_PRIORITY+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DSBPLAY_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

char *ExplainDSLockFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "DSBLOCK_", eblen);
	if (c & DSBLOCK_FROMWRITECURSOR) strscat(eb, eblen, "FROMWRITECURSOR+");
	if (c & DSBLOCK_ENTIREBUFFER) strscat(eb, eblen, "ENTIREBUFFER+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("DSBLOCK_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

#endif // DXW_NOTRACES

//----------------------------------------------------------------------//
// Service
//----------------------------------------------------------------------//

void dxwInitVolume(ApiArg)
{
	typedef MMRESULT (WINAPI *waveOutSetVolume_Type)(HWAVEOUT, DWORD);
	waveOutSetVolume_Type pwaveOutSetVolume;
	DWORD volume;
	HMODULE hWinMM = (*pLoadLibraryA)("winmm.dll");
	pwaveOutSetVolume = (waveOutSetVolume_Type)(*pGetProcAddress)(hWinMM, "waveOutSetVolume");
	volume = (dxw.GeneralVolume * 0xFFFF) / 100; // scale in range 0 - 0xFFFF
	volume |= (volume << 16); // set left+right channels
	//(*pwaveOutSetVolume)(NULL, 0x8FFF8FFF); // initialize at 50% volume
	OutTraceSND("%s: InitVolume vol=%d%%->%x\n", ApiRef, dxw.GeneralVolume, volume);
	(*pwaveOutSetVolume)(NULL, volume); 
}

//----------------------------------------------------------------------//
// Proxies
//----------------------------------------------------------------------//

HRESULT WINAPI extDirectSoundCreate(LPGUID guid, LPDIRECTSOUND *ppds, LPUNKNOWN unk)
{
	HRESULT res;
	ApiName("dsound.DirectSoundCreate");

	if(dxw.dwFlags19 & NODIRECTSOUND) {
		OutTrace("%s: NODIRECTSOUND guid=%#x\n", ApiRef, guid);
		return DSERR_UNSUPPORTED;
	}

	if(dxw.dwFlags18 & BYPASSDSOUND){
		IFakeDirectSound *FakeDS = new(IFakeDirectSound);
		*ppds = (LPDIRECTSOUND)FakeDS;
		OutTrace("%s: created virtual DirectSound ppds=%#x->%#x\n", ApiRef, FakeDS, *FakeDS);
		return DS_OK;
	}

	OutTraceSND("%s: guid=%#x\n", ApiRef, guid);
	res = (*pDirectSoundCreate)(guid, ppds, unk);
	if(res){
		TraceDSError(res);
	}
	else {
		OutTraceSND("%s: ppDS=%#x\n", ApiRef, *ppds);
		HookDirectSoundObj(ppds);
	}
	return res;
}

HRESULT WINAPI extDirectSoundCreate8(LPCGUID lpcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter)
{
	HRESULT res;
	ApiName("dsound.DirectSoundCreate8");

	if(dxw.dwFlags19 & NODIRECTSOUND) {
		OutTrace("%s: NODIRECTSOUND guid=%#x\n", ApiRef, lpcGuidDevice);
		return DSERR_UNSUPPORTED;
	}

	if(dxw.dwFlags18 & BYPASSDSOUND){
		IFakeDirectSound8 *FakeDS = new(IFakeDirectSound8);
		*ppDS8 = (LPDIRECTSOUND8)FakeDS;
		OutTrace("%s: created virtual DirectSound8 lpds=%#x\n", ApiRef, FakeDS);
		return DS_OK;
	}

	OutTraceSND("%s: guid=%#x\n", ApiRef, lpcGuidDevice);
	res = (*pDirectSoundCreate8)(lpcGuidDevice, ppDS8, pUnkOuter);
	if(res){
		TraceDSError(res);
	}
	else {
		OutTraceSND("%s: ppDS8=%#x\n", ApiRef, *ppDS8);
		HookDirectSoundObj((LPDIRECTSOUND *)ppDS8);
	}
	return res;
}

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSQueryInterface(void *lpds, REFIID riid, LPVOID *ppvObj)
{
	HRESULT res;
	ApiName("IDirectSound::QueryInterface");

	OutTraceSND("%s: lpdds=%#x REFIID=%#x(%s)\n", ApiRef, lpds, riid.Data1, ExplainGUID((GUID *)&riid));	

	res = (*pDSQueryInterface)(lpds, riid, ppvObj);

	switch(riid.Data1){
		case 0x279afa83:
			HookDirectSoundObj((LPDIRECTSOUND *)ppvObj); break;
		case 0x279AFA85:
			HookSoundBuffer((LPDIRECTSOUNDBUFFER *)ppvObj); break;
		case 0xB0210783:
			HookSoundNotify((LPDIRECTSOUNDNOTIFY *)ppvObj); break;
		default: break;
	}

	_if(res) TraceDSError(res);
	return res;
}
#endif // TRACEALLSOUNDS

HRESULT WINAPI extDSSetCooperativeLevel(void *lpds, HWND hwnd, DWORD dwLevel)
{
	HRESULT res;
	ApiName("IDirectSound::SetCooperativeLevel");

	OutTraceSND("%s: lpds=%#x hwnd=%#x level=%#x(%s)\n", ApiRef, lpds, hwnd, dwLevel, ExplainLevel(dwLevel));
	if(dwLevel == DSSCL_EXCLUSIVE) {
		dwLevel=DSSCL_PRIORITY; // Arxel Tribe patch
		OutTraceSND("%s: PATCH level=DSSCL_PRIORITY hwnd=%#x\n", ApiRef, hwnd);
	}
	if((dwLevel == DSSCL_PRIORITY) && (hwnd == 0)){
		if(dxw.GethWnd()) hwnd=dxw.GethWnd(); // v2.04.69: "Ashgan" D3D version
		else dwLevel = DSSCL_NORMAL; // v2.05.15: "Wheel of Time" ...
		OutTraceSND("%s: PATCH level=%s hwnd=%#x\n", ApiRef, ExplainLevel(dwLevel), hwnd);
	}
	// v2.05.38 fix: "Championship Manager 2006"
	if((dwLevel == DSSCL_NORMAL) && (hwnd == 0)){
		if(dxw.GethWnd()) hwnd=dxw.GethWnd(); // 
		// else dwLevel = DSSCL_EXCLUSIVE; // else ???
		OutTraceSND("%s: PATCH level=%s hwnd=%#x\n", ApiRef, ExplainLevel(dwLevel), hwnd);
	}

	res = (*pDSSetCooperativeLevel)(lpds, hwnd, dwLevel);
	_if(res) TraceDSError(res);

	if(dxw.dwFlags12 & DSINITVOLUME) dxwInitVolume(ApiRef);

	return res;
}

HRESULT WINAPI extDSGetCaps(void *lpds, LPDSCAPS pDSCaps)
{
	HRESULT res;
	ApiName("IDirectSound::GetCaps");

	OutTraceSND("%s: lpds=%#x\n", ApiRef, lpds);
	res = (*pDSGetCaps)(lpds, pDSCaps);
	if(res) {
		TraceDSError(res);
		return res;
	}

	if(IsDebugSND){
		char sBuf[128];
		OutTrace("%s: caps={size=%d flags=%#x(%s) SecSampleRate(min:max)=(%d:%d) primbuffers=%d "
			"MaxHwMixingBuffers(All:Static:Streaming)=(%d:%d:%d) "
			"FreeHwMixingBuffers(All:Static:Streaming)=(%d:%d:%d) "
			"MaxHw3DBuffers(All:Static:Streaming)=(%d:%d:%d) "
			"FreeHw3DBuffers(All:Static:Streaming)=(%d:%d:%d) "
			"TotalHwMemBytes=%d FreeHwMemBytes=%d MaxContigFreeHwMemBytes=%d UnlockTransferRateHwBuffers=%d PlayCpuOverheadSwBuffers=%d}\n", 
			ApiRef,
			pDSCaps->dwSize, pDSCaps->dwFlags, ExplainDSFlags(pDSCaps->dwFlags, sBuf, 128), 
			pDSCaps->dwMinSecondarySampleRate, pDSCaps->dwMaxSecondarySampleRate,
			pDSCaps->dwPrimaryBuffers,
			pDSCaps->dwMaxHwMixingAllBuffers, pDSCaps->dwMaxHwMixingStaticBuffers, pDSCaps->dwMaxHwMixingStreamingBuffers,
			pDSCaps->dwFreeHwMixingAllBuffers, pDSCaps->dwFreeHwMixingStaticBuffers, pDSCaps->dwFreeHwMixingStreamingBuffers,
			pDSCaps->dwMaxHw3DAllBuffers ,pDSCaps->dwMaxHw3DStaticBuffers ,pDSCaps->dwMaxHw3DStreamingBuffers,
			pDSCaps->dwFreeHw3DAllBuffers ,pDSCaps->dwFreeHw3DStaticBuffers ,pDSCaps->dwFreeHw3DStreamingBuffers,
			pDSCaps->dwTotalHwMemBytes ,pDSCaps->dwFreeHwMemBytes, pDSCaps->dwMaxContigFreeHwMemBytes, 
			pDSCaps->dwUnlockTransferRateHwBuffers, pDSCaps->dwPlayCpuOverheadSwBuffers);
	}

	if((dxw.dwFlags19 & FORCEEMULDRIVER) && !(pDSCaps->dwFlags & DSCAPS_EMULDRIVER)){
		OutTraceSND("%s: force DSCAPS_EMULDRIVER\n", ApiRef);
		pDSCaps->dwFlags |= DSCAPS_EMULDRIVER;
	}

	return res;
}

static DWORD getsize(LPCDSBUFFERDESC pcDSBufferDesc)
{
	DWORD dwSize;
  __try  { dwSize = pcDSBufferDesc->dwSize; }
  __except(EXCEPTION_EXECUTE_HANDLER) { dwSize = 0; };
  return dwSize;
}

static char *sTag(DWORD tag)
{
	return (tag == WAVE_FORMAT_PCM) ? "WAVE_FORMAT_PCM" : "???";
}

HRESULT WINAPI extDSCreateSoundBuffer(void *lpds, LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter)
{
	HRESULT res;
	ApiName("IDirectSound::CreateSoundBuffer");
	DWORD dwSize = getsize(pcDSBufferDesc);
	dsForcedCapability = 0;

#ifndef DXW_NOTRACES
	if(IsTraceSND){
		char sBuf[128];
		if(dwSize < sizeof(DSBUFFERDESC1)){
			// @#@ "Rockman 8 FC" in early hook phase creates buffers with a bad buffer descriptor
			OutTraceSND("%s: lpds=%#x ppdsb=%#x desc={size=%d}\n", 
				ApiRef, lpds, ppDSBuffer, dwSize);
		}
		else {
			OutTrace("%s: lpds=%#x ppdsb=%#x desc={size=%d flags=%#x(%s) BufferBytes=%d lpwfxFormat=%#x reserved=%#x}\n", 
				ApiRef, lpds, ppDSBuffer, 
				pcDSBufferDesc->dwSize,
				pcDSBufferDesc->dwFlags, ExplainDSBFlags(pcDSBufferDesc->dwFlags, sBuf, 128),
				pcDSBufferDesc->dwBufferBytes,
				pcDSBufferDesc->lpwfxFormat,
				pcDSBufferDesc->dwReserved);
			if(pcDSBufferDesc->lpwfxFormat){
				WAVEFORMATEX *p = pcDSBufferDesc->lpwfxFormat;
				OutTrace("%s: lpwfxFormat:\n", ApiRef);
				OutTrace("> wFormatTag: %#x(%s)\n", p->wFormatTag, sTag(p->wFormatTag)); 
				OutTrace("> nChannels: %d\n", p->nChannels);
				OutTrace("> nSamplesPerSec: %d\n", p->nSamplesPerSec);
				OutTrace("> nAvgBytesPerSec: %d\n", p->nAvgBytesPerSec);
				OutTrace("> nBlockAlign: %d\n", p->nBlockAlign);
				OutTrace("> wBitsPerSample: %d\n", p->wBitsPerSample);
				OutTrace("> cbSize: %d\n", p->cbSize);
			}
			// dump guid3DAlgorithm if DIRECTSOUND_VERSION >= 0x0700
			if((pcDSBufferDesc->dwFlags & DSBCAPS_CTRL3D) && (pcDSBufferDesc->dwSize == sizeof(DSBUFFERDESC))){
				OutTraceSND("> 3Dalgorithm: %s\n", sGUID((GUID *)&(pcDSBufferDesc->guid3DAlgorithm)));
			}
		}
	}
#endif // DXW_NOTRACES
	if(dxw.dwFlags19 & FIXDSOUNDBUFFER){
		if((dwSize >= sizeof(DSBUFFERDESC1)) && pcDSBufferDesc->lpwfxFormat) {
			WAVEFORMATEX *p = pcDSBufferDesc->lpwfxFormat;
			DWORD orig = p->nAvgBytesPerSec;
			p->nAvgBytesPerSec = (p->nSamplesPerSec * p->nChannels * p->wBitsPerSample) / 8; 
			if(orig != p->nAvgBytesPerSec) {
				OutTraceSND("%s: fixed nAvgBytesPerSec %d->%d\n", ApiRef, orig, p->nAvgBytesPerSec);
			}
		}
	}

	if((dxw.dwFlags18 & EMULATESOUNDPAN) && (pcDSBufferDesc->dwFlags & DSBCAPS_CTRLPAN)) {
		OutTraceSND("%s: EMULATESOUNDPAN - remove DSBCAPS_CTRLPAN capability\n", ApiRef);
		((DSBUFFERDESC1 *)pcDSBufferDesc)->dwFlags &=  ~DSBCAPS_CTRLPAN;
		dsForcedCapability = DSBCAPS_CTRLPAN;
	}

	if(dwSize >= sizeof(DSBUFFERDESC1)){
		switch(dxw.dwFlags5 & GLOBALFOCUSMASK){
			case 0: break;
			case GLOBALFOCUSON:
				if(!(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)){
					((DSBUFFERDESC1 *)pcDSBufferDesc)->dwFlags |= DSBCAPS_GLOBALFOCUS;
				}
				break;
			case GLOBALFOCUSOFF:
				if(!(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)){
					((DSBUFFERDESC1 *)pcDSBufferDesc)->dwFlags &= ~DSBCAPS_GLOBALFOCUS;
				}
				break;
		}
	}

	res = (*pDSCreateSoundBuffer)(lpds, pcDSBufferDesc, ppDSBuffer, pUnkOuter);

	// v2.05.53: capability check: DSBCAPS_LOCHARDWARE or DSBCAPS_LOCSOFTWARE could be requested without
	// that GetCaps lists these capabilities. In this case, you can try trimming them off.
	// Fixes "KA52 Team Alligator" missing sound.
	if((res == DSERR_UNSUPPORTED) && (pcDSBufferDesc->dwFlags & (DSBCAPS_LOCHARDWARE | DSBCAPS_LOCSOFTWARE))){
		OutTraceSND("%s: DSERR_UNSUPPORTED try clearing LOC caps\n", ApiRef);
		dsForcedCapability |= (pcDSBufferDesc->dwFlags & (DSBCAPS_LOCHARDWARE | DSBCAPS_LOCSOFTWARE));
		(DWORD)(pcDSBufferDesc->dwFlags) = pcDSBufferDesc->dwFlags & ~(DSBCAPS_LOCHARDWARE | DSBCAPS_LOCSOFTWARE);
		res = (*pDSCreateSoundBuffer)(lpds, pcDSBufferDesc, ppDSBuffer, pUnkOuter);
	}

	if(res) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	}

	// Play method is different for primary buffer?
	if(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER) {
		pDSPrimaryBuffer = *ppDSBuffer;
		HookPrimaryBuffer(ppDSBuffer);
	} else {
		HookSoundBuffer(ppDSBuffer);
	}
	
	OutTraceSND("%s: lpbuf=%#x\n", ApiRef, *ppDSBuffer);
	return res;
}

HRESULT WINAPI extDSDuplicateSoundBuffer (void *lpds, LPDIRECTSOUNDBUFFER pDSBuffer, LPDIRECTSOUNDBUFFER *ppDSBuffer)
{
	HRESULT res;
	ApiName("IDirectSound::DuplicateSoundBuffer");

	OutTraceSND("%s: lpds=%#x soundbuffer=%#x\n", ApiRef, lpds, pDSBuffer);
	res = (*pDSDuplicateSoundBuffer)(lpds, pDSBuffer, ppDSBuffer);
	if(res) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	}

	HookSoundBuffer(ppDSBuffer);
	OutTraceSND("%s: lpbuf=%#x\n", ApiRef, *ppDSBuffer);
	return res;
}

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSCompact(void *lpds)
{
	HRESULT res;
	ApiName("IDirectSound::Compact");

	OutTraceSND("%s: lpds=%#x\n", ApiRef, lpds);
	res = (*pDSCompact)(lpds);
	_if(res) TraceDSError(res);
	return res;
}

LPDSENUMCALLBACKA pDSEnumCallbackPtrA;
int WINAPI dxwDSEnumCallbackA(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
	ApiName("DSEnumCallbackA");
	int ret;
	OutTrace("%s: lpGuid=%s descr=\"%s\" module=\"%s\", context=%#x\n", ApiRef, sGUID(lpGuid), lpcstrDescription, lpcstrModule, lpContext);
	ret = (*pDSEnumCallbackPtrA)(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
	OutTrace("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

HRESULT WINAPI extDirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext)
{
	HRESULT res;
	ApiName("dsound.DirectSoundEnumerateA");

	OutTraceSND("%s\n", ApiRef);
	if(IsDebugSND){
		pDSEnumCallbackPtrA = pDSEnumCallback;
		res = (*pDirectSoundEnumerateA)(dxwDSEnumCallbackA, pContext);
	}
	else {
		res = (*pDirectSoundEnumerateA)(pDSEnumCallback, pContext);
	}
	_if(res) TraceDSError(res);
	return res;
}

LPDSENUMCALLBACKW pDSEnumCallbackPtrW;
int WINAPI dxwDSEnumCallbackW(LPGUID lpGuid, LPCWSTR lpcstrDescription, LPCWSTR lpcstrModule, LPVOID lpContext)
{
	ApiName("DSEnumCallbackW");
	int ret;
	OutTrace("%s: lpGuid=%s descr=\"%ls\" module=\"%ls\", context=%#x\n", ApiRef, sGUID(lpGuid), lpcstrDescription, lpcstrModule, lpContext);
	ret = (*pDSEnumCallbackPtrW)(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
	OutTrace("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

HRESULT WINAPI extDirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext)
{
	HRESULT res;
	ApiName("dsound.DirectSoundEnumerateW");

	OutTraceSND("%s\n", ApiRef);
	if(IsDebugSND){
		pDSEnumCallbackPtrW = pDSEnumCallback;
		res = (*pDirectSoundEnumerateW)(dxwDSEnumCallbackW, pContext);
	}
	else {
		res = (*pDirectSoundEnumerateW)(pDSEnumCallback, pContext);
	}
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSGetSpeakerConfig(void *lpds, LPDWORD pdwSpeakerConfig)
{
	HRESULT res;
	ApiName("IDirectSound::GetSpeakerConfig");

	OutTraceSND("%s\n", ApiRef);
	res = (*pDSGetSpeakerConfig)(lpds, pdwSpeakerConfig);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSetSpeakerConfig(void *lpds, DWORD pdwSpeakerConfig)
{
	HRESULT res;
	ApiName("IDirectSound::SetSpeakerConfig");

	OutTraceSND("%s\n", ApiRef);
	res = (*pDSSetSpeakerConfig)(lpds, pdwSpeakerConfig);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSInitialize(void *lpds, LPCGUID pcGuidDevice)
{
	HRESULT res;
	ApiName("IDirectSound::Initialize");

	OutTraceSND("%s\n", ApiRef);
	res = (*pDSInitialize)(lpds, pcGuidDevice);
	_if(res) TraceDSError(res);
	return res;
}
#endif // TRACEALLSOUNDS

HRESULT WINAPI extPlay(Play_Type pPlay, void *lpb, DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::Play");

#ifndef DXW_NOTRACES
	if(IsTraceSND){
		char sBuf[128];
		OutTrace("%s: lpbuf=%#x reserved=%#x priority=%#x flags=%#x(%s)\n", 
			ApiRef, lpb, dwReserved1, dwPriority, 
			dwFlags, ExplainPlayFlags(dwFlags, sBuf, 128));
	}
#endif // DXW_NOTRACES

	if(dxw.dwFlags9 & SOUNDMUTE) {
		OutTraceSND("%s: MUTE\n", ApiRef);
		return S_OK;
	}
	if((dxw.dwFlags14 & NOSOUNDLOOP) && (dwFlags & DSBPLAY_LOOPING)) {
		dwFlags &= ~DSBPLAY_LOOPING;
		OutTraceSND("%s: SUPPRESS DSBPLAY_LOOPING\n", ApiRef);
	}

	res = (*pPlay)(lpb, dwReserved1, dwPriority, dwFlags);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBPlay(void *lpb, DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
{ return extPlay(pDSSBPlay, lpb, dwReserved1, dwPriority, dwFlags); }
HRESULT WINAPI extDSPBPlay(void *lpb, DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
{ return extPlay(pDSPBPlay, lpb, dwReserved1, dwPriority, dwFlags); }

HRESULT WINAPI extDSBSetVolume(DSBSetVolume_Type pSetVolume, void *lpb, LONG lVolume)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::SetVolume");

	OutTraceSND("%s: lpbuf=%#x volume=%ld\n", ApiRef, lpb, lVolume);
	if(dxw.dwFlags9 & LOCKVOLUME) {
		OutTraceSND("%s: LOCK\n", ApiRef);
		return S_OK;
	}
	res = (*pSetVolume)(lpb, lVolume);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBSetVolume(void *lpb, LONG lVolume)
{ return extDSBSetVolume(pDSSBSetVolume, lpb, lVolume); }
HRESULT WINAPI extDSPBSetVolume(void *lpb, LONG lVolume)
{ return extDSBSetVolume(pDSPBSetVolume, lpb, lVolume); }

HRESULT WINAPI extDSBSetPan(DSBSetPan_Type pSetPan, void *lpb, LONG lPan)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::SetPan");

	OutTraceSND("%s: lpbuf=%#x pan=%ld\n", ApiRef, lpb, lPan);

	if(dxw.dwFlags18 & EMULATESOUNDPAN) {
		OutTraceSND("%s: EMULATESOUNDPAN - bypass\n", ApiRef);
		if((lPan < DSBPAN_LEFT) || (lPan > DSBPAN_RIGHT)) return DSERR_INVALIDPARAM;
		return S_OK;
	}

	res = (*pSetPan)(lpb, lPan);
	_if(res) TraceDSError(res);
	return res;
}
HRESULT WINAPI extDSSBSetPan(void *lpb, LONG lPan)
{ return extDSBSetPan(pDSSBSetPan, lpb, lPan); }
HRESULT WINAPI extDSPBSetPan(void *lpb, LONG lPan)
{ return extDSBSetPan(pDSPBSetPan, lpb, lPan); }

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBSetFrequency(DSBSetFrequency_Type pSetFrequency, void *lpb, DWORD dwFrequency)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::SetFrequency");

	OutTraceSND("%s: lpbuf=%#x frequency=%ld\n", ApiRef, lpb, dwFrequency);
	res = (*pSetFrequency)(lpb, dwFrequency);
	_if(res) TraceDSError(res);
	return res;
}
HRESULT WINAPI extDSSBSetFrequency(void *lpb, DWORD dwFrequency)
{ return extDSBSetFrequency(pDSSBSetFrequency, lpb, dwFrequency); }
HRESULT WINAPI extDSPBSetFrequency(void *lpb, DWORD dwFrequency)
{ return extDSBSetFrequency(pDSPBSetFrequency, lpb, dwFrequency); }
#endif // TRCEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBStop(Void_Type pStop, void *lpb)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::Stop");

	OutTraceSND("%s: lpbuf=%#x\n", ApiRef, lpb);
	res = (*pStop)(lpb);
	_if(res) TraceDSError(res);
	return res;
}
HRESULT WINAPI extDSSBStop(void *lpb)
{ return extDSBStop(pDSSBStop, lpb); }
HRESULT WINAPI extDSPBStop(void *lpb)
{ return extDSBStop(pDSPBStop, lpb); }
#endif // TRCEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBRestore(Void_Type pRestore, void *lpb)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::Restore");

	OutTraceSND("%s: lpbuf=%#x\n", ApiRef, lpb);
	res = (*pRestore)(lpb);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBRestore(void *lpb)
{ return extDSBRestore(pDSSBRestore, lpb); }
HRESULT WINAPI extDSPBRestore(void *lpb)
{ return extDSBRestore(pDSPBRestore, lpb); }
#endif // TRCEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBSetCurrentPosition(DSBSetCurrentPosition_Type pSetCurrentPosition, void *lpb, DWORD dwNewPosition)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::SetCurrentPosition");

	OutTraceSND("%s: lpbuf=%#x position=%#x\n", ApiRef, lpb, dwNewPosition);
	res = (*pSetCurrentPosition)(lpb, dwNewPosition);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBSetCurrentPosition(void *lpb, DWORD dwNewPosition)
{ return extDSBSetCurrentPosition(pDSSBSetCurrentPosition, lpb, dwNewPosition); }
HRESULT WINAPI extDSPBSetCurrentPosition(void *lpb, DWORD dwNewPosition)
{ return extDSBSetCurrentPosition(pDSPBSetCurrentPosition, lpb, dwNewPosition); }
#endif // TRCEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBSetFormat(DSBSetFormat_Type pSetFormat, void *lpb, LPCWAVEFORMATEX pcfxFormat)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::SetFormat");

	OutTraceSND("%s: lpbuf=%#x\n", ApiRef, lpb);
	OutTraceSND("> wFormatTag=%#x(%s)\n", pcfxFormat->wFormatTag, sTag( pcfxFormat->wFormatTag));
	OutTraceSND("> nChannels=%d\n", pcfxFormat->nChannels);
	OutTraceSND("> nSamplesPerSec=%d\n", pcfxFormat->nSamplesPerSec);
	OutTraceSND("> nAvgBytesPerSec=%d\n", pcfxFormat->nAvgBytesPerSec);
	OutTraceSND("> nBlockAlign=%d\n", pcfxFormat->nBlockAlign);
	OutTraceSND("> wBitsPerSample=%d\n", pcfxFormat->wBitsPerSample);
	OutTraceSND("> cbSize=%d\n", pcfxFormat->cbSize);

	res = (*pSetFormat)(lpb, pcfxFormat);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBSetFormat(void *lpb, LPCWAVEFORMATEX pcfxFormat)
{ return extDSBSetFormat(pDSSBSetFormat, lpb, pcfxFormat); }
HRESULT WINAPI extDSPBSetFormat(void *lpb, LPCWAVEFORMATEX pcfxFormat)
{ return extDSBSetFormat(pDSPBSetFormat, lpb, pcfxFormat); }
#endif // TRCEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBLock(DSBLock_Type pLock, void *lpb, DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::Lock");

#ifndef DXW_NOTRACES
	char sBuf[81];
	OutTraceSND("%s: lpbuf=%#x offset=%d bytes=%d flags=%#x(%s)\n", 
		ApiRef, lpb, dwOffset, dwBytes, dwFlags, ExplainDSLockFlags(dwFlags, sBuf, 80));
#endif // DXW_NOTRACES

	res = (*pLock)(lpb, dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBLock(void *lpb, DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{ return extDSBLock(pDSSBLock, lpb, dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags); }
HRESULT WINAPI extDSPBLock(void *lpb, DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{ return extDSBLock(pDSPBLock, lpb, dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags); }
#endif // TRCEALLSOUNDS

HRESULT WINAPI extDSBUnlock(DSBUnlock_Type pUnlock, void *lpb, LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::Unlock");

	OutTraceSND("%s: lpbuf=%#x ptr1=%#x len1=%d ptr2=%#x len2=%d\n", ApiRef, lpb, pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
	if(IsDebugSND && IsTraceHex) {
		if(dwAudioBytes1) HexTrace((LPBYTE)pvAudioPtr1, dwAudioBytes1);
		if(dwAudioBytes2) HexTrace((LPBYTE)pvAudioPtr2, dwAudioBytes2);
	}
	// this trick seems the only way to stop a background music loaded to primary/setvol buffers and 
	// playing apparently without issuing the Play method.
	if(dxw.dwFlags9 & SOUNDMUTE) {
		OutTraceSND("%s: MUTE\n", ApiRef);
		dwAudioBytes1 = 0;
		dwAudioBytes2 = 0;
	}
	res = (*pUnlock)(lpb, pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBUnlock(void *lpb, LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{ return extDSBUnlock(pDSSBUnlock, lpb, pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2); }
HRESULT WINAPI extDSPBUnlock(void *lpb, LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{ return extDSBUnlock(pDSPBUnlock, lpb, pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2); }

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBInitialize(DSBInitialize_Type pInitialize, void *lpb, LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC1 pcDSBufferDesc)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::Initialize");

#ifndef DXW_NOTRACES
	if(IsTraceSND){
		char sBuf[128];
		OutTrace("%s: lpbuf=%#x lpds=%#x desc={flags=%#x(%s) BufferBytes=%d}\n", 
			ApiRef, lpb, pDirectSound, pcDSBufferDesc->dwFlags, 
			ExplainDSBFlags(pcDSBufferDesc->dwFlags, sBuf, 128), 
			pcDSBufferDesc->dwBufferBytes);
	}
#endif // DXW_NOTRACES
	res = (*pDSSBInitialize)(lpb, pDirectSound, pcDSBufferDesc);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBInitialize(void *lpb, LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC1 pcDSBufferDesc)
{return extDSBInitialize(pDSSBInitialize, lpb, pDirectSound, pcDSBufferDesc); }
HRESULT WINAPI extDSPBInitialize(void *lpb, LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC1 pcDSBufferDesc)
{return extDSBInitialize(pDSPBInitialize, lpb, pDirectSound, pcDSBufferDesc); }
#endif // TRACEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBQueryInterface(QueryInterface_Type pQueryInterface, LPVOID lpds, REFIID riid, LPVOID *ppvObj)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::QueryInterface");

	OutTraceSND("%s: lpdds=%#x REFIID=%#x(%s)\n", ApiRef, lpds, riid.Data1, ExplainGUID((GUID *)&riid));	

	res = (*pQueryInterface)(lpds, riid, ppvObj);
	_if(res) TraceDSError(res);
	return res;
}

HRESULT WINAPI extDSSBQueryInterface(LPVOID lpds, REFIID riid, LPVOID *ppvObj)
{ return extDSBQueryInterface(pDSSBQueryInterface, lpds, riid, ppvObj); }
HRESULT WINAPI extDSPBQueryInterface(LPVOID lpds, REFIID riid, LPVOID *ppvObj)
{ return extDSBQueryInterface(pDSPBQueryInterface, lpds, riid, ppvObj); }
#endif // TRACEALLSOUNDS

HRESULT WINAPI extDSBGetCaps(DSBGetCaps_Type pGetCaps, void *lpb, LPDSBCAPS pDSBufferCaps)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::GetCaps");

	OutTraceSND("%s: lpds=%#x\n", ApiRef, lpb);
	res = (*pGetCaps)(lpb, pDSBufferCaps);
	if(res) {
		TraceDSError(res);
		return res;
	}

#ifndef DXW_NOTRACES
	if(IsTraceSND){
		char sBuf[128];
		OutTrace("%s: caps={Size=%d Flags=%#x(%s) BufferBytes=%d UnlockTransferRate=%#x PlayCpuOverhead=%#x}\n", 
			ApiRef, pDSBufferCaps->dwSize, pDSBufferCaps->dwFlags, 
			ExplainDSBFlags(pDSBufferCaps->dwFlags, sBuf, 128), 
			pDSBufferCaps->dwBufferBytes, 
			pDSBufferCaps->dwUnlockTransferRate, 
			pDSBufferCaps->dwPlayCpuOverhead);
	}
#endif // DXW_NOTRACES

	if(dsForcedCapability & DSBCAPS_LOCHARDWARE){
		OutTraceSND("%s: pretend LOCHARDWARE capability\n", ApiRef);
		pDSBufferCaps->dwFlags &= ~DSBCAPS_LOCSOFTWARE;
		pDSBufferCaps->dwFlags |= DSBCAPS_LOCHARDWARE;
	}
	if(dsForcedCapability & DSBCAPS_LOCSOFTWARE){
		OutTraceSND("%s: pretend LOCSOFTWARE capability\n", ApiRef);
		pDSBufferCaps->dwFlags |= DSBCAPS_LOCSOFTWARE;
		pDSBufferCaps->dwFlags &= ~DSBCAPS_LOCHARDWARE;
	}
	if(dsForcedCapability & DSBCAPS_CTRLPAN){
		OutTraceSND("%s: pretend CTRLPAN capability\n", ApiRef);
		pDSBufferCaps->dwFlags |= DSBCAPS_CTRLPAN;
	}
	return res;
}

HRESULT WINAPI extDSSBGetCaps(void *lpb, LPDSBCAPS pDSBufferCaps)
{ return extDSBGetCaps(pDSSBGetCaps, lpb, pDSBufferCaps); }
HRESULT WINAPI extDSPBGetCaps(void *lpb, LPDSBCAPS pDSBufferCaps)
{ return extDSBGetCaps(pDSPBGetCaps, lpb, pDSBufferCaps); }

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBGetCurrentPosition(DSBGetCurrentPosition_Type pGetCurrentPosition, void *lpdsb, LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::GetCurrentPosition");

	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, lpdsb);
	res = (*pGetCurrentPosition)(lpdsb, pdwCurrentPlayCursor, pdwCurrentWriteCursor);
	if(res) {
		TraceDSError(res);
		return res;
	}

	OutTraceSND("%s: play=%d write=%d\n", ApiRef, 
		pdwCurrentPlayCursor ? *pdwCurrentPlayCursor : 0, 
		pdwCurrentWriteCursor ? *pdwCurrentWriteCursor : 0);
	return res;
}

HRESULT WINAPI extDSSBGetCurrentPosition(void *lpdsb, LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
{ return extDSBGetCurrentPosition(pDSSBGetCurrentPosition, lpdsb, pdwCurrentPlayCursor, pdwCurrentWriteCursor); }
HRESULT WINAPI extDSPBGetCurrentPosition(void *lpdsb, LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
{ return extDSBGetCurrentPosition(pDSPBGetCurrentPosition, lpdsb, pdwCurrentPlayCursor, pdwCurrentWriteCursor); }
#endif // TRACEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBGetFormat(DSBGetFormat_Type pGetFormat, void *lpdsb, LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::GetFormat");

	OutTraceSND("%s: lpdsb=%#x allocated=%d\n", ApiRef, lpdsb, dwSizeAllocated);
	res = (*pGetFormat)(lpdsb, pwfxFormat, dwSizeAllocated, pdwSizeWritten);
	if(res) {
		TraceDSError(res);
		return res;
	}

	OutTraceSND("> written=%d\n", pdwSizeWritten ? *pdwSizeWritten : 0);
	OutTraceSND("> wFormatTag=%#x\n", pwfxFormat->wFormatTag);
	OutTraceSND("> nChannels=%d\n", pwfxFormat->nChannels);
	OutTraceSND("> nSamplesPerSec=%d\n", pwfxFormat->nSamplesPerSec);
	OutTraceSND("> nAvgBytesPerSec=%d\n", pwfxFormat->nAvgBytesPerSec);
	OutTraceSND("> nBlockAlign=%d\n", pwfxFormat->nBlockAlign);
	OutTraceSND("> wBitsPerSample=%d\n", pwfxFormat->wBitsPerSample);
	OutTraceSND("> cbSize=%d\n", pwfxFormat->cbSize);
	return res;
}

HRESULT WINAPI extDSSBGetFormat(void *lpdsb, LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{ return extDSBGetFormat(pDSSBGetFormat, lpdsb, pwfxFormat, dwSizeAllocated, pdwSizeWritten); }
HRESULT WINAPI extDSPBGetFormat(void *lpdsb, LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{ return extDSBGetFormat(pDSPBGetFormat, lpdsb, pwfxFormat, dwSizeAllocated, pdwSizeWritten); }
#endif // TRACEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBGetVolume(DSBGetVolume_Type pGetVolume, void *lpdsb, LPLONG plVolume)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::GetVolume");

	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, lpdsb);
	res = (*pGetVolume)(lpdsb, plVolume);
	if(res) {
		TraceDSError(res);
		return res;
	}

	OutTraceSND("%s: volume=%d\n", ApiRef, *plVolume);
	return res;
}

HRESULT WINAPI extDSSBGetVolume(void *lpdsb, LPLONG plVolume)
{ return extDSBGetVolume(pDSSBGetVolume, lpdsb, plVolume); }
HRESULT WINAPI extDSPBGetVolume(void *lpdsb, LPLONG plVolume)
{ return extDSBGetVolume(pDSPBGetVolume, lpdsb, plVolume); }
#endif // TRACEALLSOUNDS

HRESULT WINAPI extDSBGetPan(DSBGetPan_Type pGetPan, void *lpdsb, LPLONG plPan)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::GetPan");

	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, lpdsb);

	if(dxw.dwFlags18 & EMULATESOUNDPAN){
		OutTraceSND("%s: EMULATESOUNDPAN pan=%d\n", ApiRef, gPan);
		*plPan = gPan;
		return DS_OK;
	}

	res = (*pGetPan)(lpdsb, plPan);
	if(res) {
		TraceDSError(res);
		return res;
	}

	OutTraceSND("%s: pan=%d\n", ApiRef, *plPan);
	return res;
}

HRESULT WINAPI extDSSBGetPan(void *lpdsb, LPLONG plPan)
{ return extDSBGetPan(pDSSBGetPan, lpdsb, plPan); }
HRESULT WINAPI extDSPBGetPan(void *lpdsb, LPLONG plPan)
{ return extDSBGetPan(pDSPBGetPan, lpdsb, plPan); }

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBGetFrequency(DSBGetFrequency_Type pGetFrequency, void *lpdsb, LPDWORD pdwFrequency)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::GetFrequency");

	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, lpdsb);
	res = (*pGetFrequency)(lpdsb, pdwFrequency);
	if(res) {
		TraceDSError(res);
		return res;
	}

	OutTraceSND("%s: frequency=%#x\n", ApiRef, *pdwFrequency);
	return res;
}

HRESULT WINAPI extDSSBGetFrequency(void *lpdsb, LPDWORD pdwFrequency)
{return extDSBGetFrequency(pDSSBGetFrequency, lpdsb, pdwFrequency); }
HRESULT WINAPI extDSPBGetFrequency(void *lpdsb, LPDWORD pdwFrequency)
{return extDSBGetFrequency(pDSPBGetFrequency, lpdsb, pdwFrequency); }
#endif // TRACEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSBGetStatus(DSBGetStatus_Type pGetStatus, void *lpdsb, LPDWORD pdwStatus)
{
	HRESULT res;
	ApiName("IDirectSoundBuffer::GetStatus");

	OutTraceSND("%s: lpdsb=%#x pstatus=%#x\n", ApiRef, lpdsb, pdwStatus);
	res = (*pGetStatus)(lpdsb, pdwStatus);
	if(res) {
		TraceDSError(res);
		return res;
	}

	OutTraceSND("%s: status=%#x\n", ApiRef, *pdwStatus);
	return res;
}
HRESULT WINAPI extDSSBGetStatus(void *lpdsb, LPDWORD pdwStatus)
{ return extDSBGetStatus(pDSSBGetStatus, lpdsb, pdwStatus); }
HRESULT WINAPI extDSPBGetStatus(void *lpdsb, LPDWORD pdwStatus)
{ return extDSBGetStatus(pDSPBGetStatus, lpdsb, pdwStatus); }
#endif // TRACEALLSOUNDS

ULONG WINAPI extDSPBRelease(void *lpdsb)
{
	ULONG ret;
	ret = (*pDSPBRelease)(lpdsb);
	OutTraceSND("IDirectSoundBuffer::Release: PRIMARY lpdsb=%#x ret=%d\n", lpdsb, ret);
	if(ret == 0) pDSPrimaryBuffer = NULL;
	return ret;
}

ULONG WINAPI extDSSBRelease(void *lpdsb)
{
	ULONG ret;
	ret = (*pDSSBRelease)(lpdsb);
	OutTraceSND("IDirectSoundBuffer::Release: lpdsb=%#x ret=%d\n", lpdsb, ret);
	return ret;
}

#ifdef TRACEALLSOUNDS
ULONG WINAPI extDSRelease(void *lpdsb)
{
	ULONG ret;
	ret = (*pDSRelease)(lpdsb);
	OutTraceSND("IDirectSound::Release: lpdsb=%#x ret=%d\n", lpdsb, ret);
	return ret;
}
#endif // TRACEALLSOUNDS

#ifdef TRACEALLSOUNDS
HRESULT WINAPI extDSNSetNotificationPositions(DWORD dwPositionNotifies, LPCDSBPOSITIONNOTIFY pcPositionNotifies)
{
	HRESULT res;
	ApiName("IDirectSoundNotify::SetNotificationPositions");

	OutTraceSND("%s: PositionNotifies=%d \n", ApiRef, dwPositionNotifies);
	for(UINT i=0; i<dwPositionNotifies; i++){
		OutTraceSND("[%d] > offset=%#x hEvent=%#x\n", i, pcPositionNotifies[i].dwOffset, pcPositionNotifies[i].hEventNotify);
	}
	res = (*pDSNSetNotificationPositions)(dwPositionNotifies, pcPositionNotifies);
	return res;
}
#endif // TRACEALLSOUNDS
