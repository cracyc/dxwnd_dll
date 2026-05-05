#define _CRT_SECURE_NO_WARNINGS 
#define _CRT_NON_CONFORMING_SWPRINTFS
// disable truncation warning when multiplying a BYTE for a float. The float is between 0.0 and 1.0.
#pragma warning( disable: 4244 )

#define _MODULE "winmm32"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "resource.h"
#include "player.h"

#include "MMSystem.h"
#include <stdio.h>
#include "mciplayer.h"

#include <intrin.h>
#pragma intrinsic(_ReturnAddress)

//#define TRACEALL
//#define TRACEMIXER
#ifdef TRACEALL
#define TRACEMIXER
#endif // TRACEALL

#define BYPASSWAVEOUT FALSE
#define SUPPRESSMCIERRORS FALSE
#define EMULATEJOY TRUE
#define INVERTJOYAXIS TRUE

extern BOOL vjGetJoy(char *, LPJOYINFO);
extern void vjGetCaps(LPJOYCAPS);

BOOL IsWithinMCICall = FALSE;
static int waveBits = -1;
static BOOL bMidiMessageHooked = FALSE;

typedef struct 
{
	HWAVEOUT hwo;
	LPWAVEHDR pwh;
	UINT cbwh;
} unprep;

//#include "logall.h" // comment when not debugging

MMRESULT WINAPI extauxGetDevCapsA(UINT_PTR, LPAUXCAPS, UINT);
typedef MMRESULT (WINAPI *auxGetDevCapsA_Type)(UINT_PTR, LPAUXCAPS, UINT);
auxGetDevCapsA_Type pauxGetDevCapsA;

typedef MMRESULT (WINAPI *auxOutMessage_Type)(UINT, UINT, DWORD_PTR, DWORD_PTR);
auxOutMessage_Type pauxOutMessage;
MMRESULT WINAPI extauxOutMessage(UINT, UINT, DWORD_PTR, DWORD_PTR);

typedef MMRESULT (WINAPI *timeGetDevCaps_Type)(LPTIMECAPS, UINT);
timeGetDevCaps_Type ptimeGetDevCaps = NULL;
MMRESULT WINAPI exttimeGetDevCaps(LPTIMECAPS, UINT);

typedef MCIDEVICEID (WINAPI *mciGetDeviceIDA_Type)(LPCTSTR);
mciGetDeviceIDA_Type pmciGetDeviceIDA = NULL;
MCIDEVICEID WINAPI extmciGetDeviceIDA(LPCTSTR);

typedef MCIDEVICEID (WINAPI *mciGetDeviceIDW_Type)(LPCWSTR);
mciGetDeviceIDW_Type pmciGetDeviceIDW = NULL;
MCIDEVICEID WINAPI extmciGetDeviceIDW(LPCWSTR);

typedef DWORD (WINAPI *joyGetNumDevs_Type)(void);
joyGetNumDevs_Type pjoyGetNumDevs = NULL;
DWORD WINAPI extjoyGetNumDevs(void);

typedef MMRESULT (WINAPI *joyGetDevCapsA_Type)(DWORD, LPJOYCAPS, UINT);
joyGetDevCapsA_Type pjoyGetDevCapsA = NULL;
MMRESULT WINAPI extjoyGetDevCapsA(DWORD, LPJOYCAPS, UINT);

typedef MMRESULT (WINAPI *joyGetPosEx_Type)(DWORD, LPJOYINFOEX);
joyGetPosEx_Type pjoyGetPosEx = NULL;
MMRESULT WINAPI extjoyGetPosEx(DWORD, LPJOYINFOEX);

typedef MMRESULT (WINAPI *joyGetPos_Type)(DWORD, LPJOYINFO);
joyGetPos_Type pjoyGetPos = NULL;
MMRESULT WINAPI extjoyGetPos(DWORD, LPJOYINFO);

typedef MMRESULT (WINAPI *joySetCapture_Type)(HWND, UINT, UINT, BOOL);
joySetCapture_Type pjoySetCapture;
MMRESULT WINAPI extjoySetCapture(HWND, UINT, UINT, BOOL);

typedef MMRESULT (WINAPI *joyReleaseCapture_Type)(UINT);
joyReleaseCapture_Type pjoyReleaseCapture;
MMRESULT WINAPI extjoyReleaseCapture(UINT);

typedef BOOL (WINAPI *mciGetErrorStringA_Type)(DWORD, LPCSTR, UINT);
mciGetErrorStringA_Type pmciGetErrorStringA;
BOOL WINAPI extmciGetErrorStringA(DWORD, LPCSTR, UINT);

typedef UINT (WINAPI *waveOutGetNumDevs_Type)(void);
waveOutGetNumDevs_Type pwaveOutGetNumDevs;
UINT WINAPI extwaveOutGetNumDevs(void);

typedef UINT (WINAPI *midiOutGetNumDevs_Type)(void);
midiOutGetNumDevs_Type pmidiOutGetNumDevs;
UINT WINAPI extmidiOutGetNumDevs(void);

typedef UINT (WINAPI *waveOutGetDevCapsA_Type)(DWORD, LPWAVEOUTCAPSA, UINT);
UINT WINAPI extwaveOutGetDevCapsA(DWORD, LPWAVEOUTCAPSA, UINT);
waveOutGetDevCapsA_Type pwaveOutGetDevCapsA;

typedef UINT (WINAPI *waveOutGetDevCapsW_Type)(DWORD, LPWAVEOUTCAPSW, UINT);
UINT WINAPI extwaveOutGetDevCapsW(DWORD, LPWAVEOUTCAPSW, UINT);
waveOutGetDevCapsW_Type pwaveOutGetDevCapsW;

typedef DWORD (WINAPI *wod32Message_Type)(UINT, UINT,  DWORD, DWORD, DWORD);
wod32Message_Type pwod32Message;
DWORD WINAPI extwod32Message(UINT, UINT,  DWORD, DWORD, DWORD);

typedef MMRESULT (WINAPI *midiOutOpen_Type)(LPHMIDIOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD);
midiOutOpen_Type pmidiOutOpen;
MMRESULT WINAPI extmidiOutOpen(LPHMIDIOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD);

typedef MMRESULT (WINAPI *midiOutGetDevCapsA_Type)(DWORD, LPMIDIOUTCAPS2A, UINT);
midiOutGetDevCapsA_Type pmidiOutGetDevCapsA;
MMRESULT WINAPI extmidiOutGetDevCapsA(DWORD, LPMIDIOUTCAPS2A, UINT);

typedef MMRESULT (WINAPI *midiOutGetDevCapsW_Type)(DWORD, LPMIDIOUTCAPS2W, UINT);
midiOutGetDevCapsW_Type pmidiOutGetDevCapsW;
MMRESULT WINAPI extmidiOutGetDevCapsW(DWORD, LPMIDIOUTCAPS2W, UINT);

typedef MMRESULT (WINAPI *midiOutShortMsg_Type)(HMIDIOUT, DWORD);
midiOutShortMsg_Type pmidiOutShortMsg;
MMRESULT WINAPI extmidiOutShortMsg(HMIDIOUT, DWORD);

typedef MMRESULT (WINAPI *midiOutLongMsg_Type)(HMIDIOUT, LPMIDIHDR, UINT);
midiOutLongMsg_Type pmidiOutLongMsg;
MMRESULT WINAPI extmidiOutLongMsg(HMIDIOUT, LPMIDIHDR, UINT);

typedef MMRESULT (WINAPI *waveOutClose_Type)(HWAVEOUT);
waveOutClose_Type pwaveOutClose;
MMRESULT WINAPI extwaveOutClose(HWAVEOUT);

typedef MMRESULT (WINAPI *midiOutClose_Type)(HMIDIOUT);
midiOutClose_Type pmidiOutClose;
MMRESULT WINAPI extmidiOutClose(HMIDIOUT);

typedef UINT (WINAPI *timeBeginPeriod_Type)(UINT);
timeBeginPeriod_Type ptimeBeginPeriod;
UINT WINAPI exttimeBeginPeriod(UINT);

typedef UINT (WINAPI *timeEndPeriod_Type)(UINT);
timeEndPeriod_Type ptimeEndPeriod;
UINT WINAPI exttimeEndPeriod(UINT);

typedef MMRESULT (WINAPI *waveOutReset_Type)(HWAVEOUT);
waveOutReset_Type pwaveOutReset;
MMRESULT WINAPI extwaveOutReset(HWAVEOUT);

typedef MMRESULT (WINAPI *waveOutPause_Type)(HWAVEOUT);
waveOutPause_Type pwaveOutPause;
MMRESULT WINAPI extwaveOutPause(HWAVEOUT);

typedef MMRESULT (WINAPI *waveOutRestart_Type)(HWAVEOUT);
waveOutRestart_Type pwaveOutRestart;
MMRESULT WINAPI extwaveOutRestart(HWAVEOUT);

typedef MMRESULT (WINAPI *waveOutOpen_Type)(LPHWAVEOUT, UINT_PTR, LPWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
waveOutOpen_Type pwaveOutOpen = NULL; // must be NULL !!!
MMRESULT WINAPI extwaveOutOpen(LPHWAVEOUT, UINT_PTR, LPWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);

typedef MMRESULT (WINAPI *waveOutPrepareHeader_Type)(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
waveOutPrepareHeader_Type pwaveOutPrepareHeader, pwaveOutUnprepareHeader;
MMRESULT WINAPI extwaveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh); 
MMRESULT WINAPI extwaveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);

typedef MMRESULT (WINAPI *waveOutWrite_Type)(HWAVEOUT, LPWAVEHDR, UINT);
waveOutWrite_Type pwaveOutWrite;
MMRESULT WINAPI extwaveOutWrite(HWAVEOUT, LPWAVEHDR, UINT);

typedef MMRESULT (WINAPI *waveOutGetPosition_Type)(HWAVEOUT, LPMMTIME, UINT);
waveOutGetPosition_Type pwaveOutGetPosition;
MMRESULT WINAPI extwaveOutGetPosition(HWAVEOUT, LPMMTIME, UINT);

typedef MMRESULT (WINAPI *auxSetVolume_Type)(UINT, DWORD);
auxSetVolume_Type pauxSetVolume;
MMRESULT WINAPI extauxSetVolume(UINT, DWORD);

typedef MMRESULT (WINAPI *auxGetVolume_Type)(UINT, LPDWORD);
auxGetVolume_Type pauxGetVolume;
MMRESULT WINAPI extauxGetVolume(UINT, LPDWORD);

typedef MMRESULT (WINAPI *waveOutSetVolume_Type)(HWAVEOUT, DWORD);
waveOutSetVolume_Type pwaveOutSetVolume;
MMRESULT WINAPI extwaveOutSetVolume(HWAVEOUT, DWORD);

typedef MMRESULT (WINAPI *waveOutGetVolume_Type)(HWAVEOUT, LPDWORD);
waveOutGetVolume_Type pwaveOutGetVolume;
MMRESULT WINAPI extwaveOutGetVolume(HWAVEOUT, LPDWORD);

typedef UINT (WINAPI *auxGetNumDevs_Type)(void);
UINT WINAPI extauxGetNumDevs(void);
auxGetNumDevs_Type pauxGetNumDevs; // used internally !!!

typedef MMRESULT (WINAPI *midiOutReset_Type)(HMIDIOUT);
MMRESULT WINAPI extmidiOutReset(HMIDIOUT);
midiOutReset_Type pmidiOutReset;

typedef MMRESULT (WINAPI *midiOutSetVolume_Type)(HMIDIOUT, DWORD);
MMRESULT WINAPI extmidiOutSetVolume(HMIDIOUT, DWORD);
midiOutSetVolume_Type pmidiOutSetVolume;

typedef HMMIO (WINAPI *mmioOpenA_Type)(LPCSTR, LPMMIOINFO, DWORD);
mmioOpenA_Type pmmioOpenA;
HMMIO WINAPI extmmioOpenA(LPCSTR, LPMMIOINFO, DWORD);

typedef HMMIO (WINAPI *mmioOpenW_Type)(LPCWSTR, LPMMIOINFO, DWORD);
mmioOpenW_Type pmmioOpenW;
HMMIO WINAPI extmmioOpenW(LPCWSTR, LPMMIOINFO, DWORD);

typedef MMRESULT (WINAPI *mmioDescend_Type)(HMMIO, LPMMCKINFO, const MMCKINFO *, UINT);
mmioDescend_Type pmmioDescend;
MMRESULT WINAPI extmmioDescend(HMMIO, LPMMCKINFO, const MMCKINFO *, UINT);

typedef LONG (WINAPI *mmioSeek_Type)(HMMIO, LONG, int);
mmioSeek_Type pmmioSeek;
LONG WINAPI extmmioSeek(HMMIO, LONG, int);

typedef LONG (WINAPI *mmioRead_Type)(HMMIO, HPSTR, LONG);
mmioRead_Type pmmioRead;
LONG WINAPI extmmioRead(HMMIO hmmio, HPSTR pch, LONG cch);

typedef MMRESULT (WINAPI *mmioSetInfo_Type)(HMMIO, LPCMMIOINFO, UINT);
mmioSetInfo_Type pmmioSetInfo;
MMRESULT WINAPI extmmioSetInfo(HMMIO, LPCMMIOINFO, UINT);

typedef MMRESULT (WINAPI *mmioGetInfo_Type)(HMMIO, LPMMIOINFO, UINT);
mmioGetInfo_Type pmmioGetInfo;
MMRESULT WINAPI extmmioGetInfo(HMMIO, LPMMIOINFO, UINT);

typedef MMRESULT (WINAPI *mmioClose_Type)(HMMIO, UINT);
mmioClose_Type pmmioClose;
MMRESULT WINAPI extmmioClose(HMMIO, UINT);

typedef BOOL (WINAPI *sndPlaySoundA_Type)(LPCSTR, UINT);
sndPlaySoundA_Type psndPlaySoundA;
BOOL WINAPI extsndPlaySoundA(LPCSTR, UINT);

typedef BOOL (WINAPI *sndPlaySoundW_Type)(LPCWSTR, UINT);
sndPlaySoundW_Type psndPlaySoundW;
BOOL WINAPI extsndPlaySoundW(LPCWSTR, UINT);

typedef BOOL (WINAPI *PlaySoundA_Type)(LPCSTR, HMODULE, UINT);
PlaySoundA_Type pPlaySoundA;
BOOL WINAPI extPlaySoundA(LPCSTR, HMODULE, UINT);

typedef BOOL (WINAPI *PlaySoundW_Type)(LPCWSTR, HMODULE, UINT);
PlaySoundW_Type pPlaySoundW;
BOOL WINAPI extPlaySoundW(LPCWSTR, HMODULE, UINT);

typedef MMRESULT (WINAPI *midiStreamOut_Type)(HMIDISTRM, LPMIDIHDR, UINT);
midiStreamOut_Type pmidiStreamOut;
MMRESULT WINAPI extmidiStreamOut(HMIDISTRM, LPMIDIHDR, UINT);

typedef MMRESULT (WINAPI *midiOutPrepareHeader_Type)(HMIDIOUT, LPMIDIHDR, UINT);
midiOutPrepareHeader_Type pmidiOutPrepareHeader = NULL;
MMRESULT WINAPI extmidiOutPrepareHeader(HMIDIOUT, LPMIDIHDR, UINT);

typedef MMRESULT (WINAPI *midiStreamPause_Type)(HMIDISTRM);
midiStreamPause_Type pmidiStreamPause;
MMRESULT WINAPI extmidiStreamPause(HMIDISTRM);

typedef MMRESULT (WINAPI *midiOutGetErrorTextA_Type)(MMRESULT, LPSTR, UINT);
midiOutGetErrorTextA_Type pmidiOutGetErrorTextA;
MMRESULT WINAPI extmidiOutGetErrorTextA(MMRESULT, LPSTR, UINT);

typedef MMRESULT (WINAPI *midiStreamClose_Type)(HMIDISTRM);
midiStreamClose_Type pmidiStreamClose;
MMRESULT WINAPI extmidiStreamClose(HMIDISTRM);

typedef MMRESULT (WINAPI *midiStreamOpen_Type)(LPHMIDISTRM, LPUINT, DWORD, DWORD_PTR, DWORD_PTR, DWORD);
midiStreamOpen_Type pmidiStreamOpen;
MMRESULT WINAPI extmidiStreamOpen(LPHMIDISTRM, LPUINT, DWORD, DWORD_PTR, DWORD_PTR, DWORD);

typedef MMRESULT (WINAPI *midiStreamStop_Type)(HMIDISTRM);
midiStreamStop_Type pmidiStreamStop;
MMRESULT WINAPI extmidiStreamStop(HMIDISTRM);

typedef MMRESULT (WINAPI *midiStreamRestart_Type)(HMIDISTRM);
midiStreamRestart_Type pmidiStreamRestart;
MMRESULT WINAPI extmidiStreamRestart(HMIDISTRM);

typedef MMRESULT (WINAPI *midiOutUnprepareHeader_Type)(HMIDIOUT, LPMIDIHDR, UINT);
midiOutPrepareHeader_Type pmidiOutUnprepareHeader;
MMRESULT WINAPI extmidiOutUnprepareHeader(HMIDIOUT, LPMIDIHDR, UINT);

typedef MMRESULT (WINAPI *midiStreamProperty_Type)(HMIDISTRM, LPBYTE, DWORD);
midiStreamProperty_Type pmidiStreamProperty;
MMRESULT WINAPI extmidiStreamProperty(HMIDISTRM, LPBYTE, DWORD);

typedef MMRESULT (WINAPI *midiStreamPosition_Type)(HMIDISTRM, LPMMTIME, UINT);
midiStreamPosition_Type pmidiStreamPosition;
MMRESULT WINAPI extmidiStreamPosition(HMIDISTRM, LPMMTIME, UINT);

typedef MMRESULT (WINAPI *midiConnect_Type)(HMIDI, HMIDIOUT, LPVOID);
midiConnect_Type pmidiConnect;
MMRESULT WINAPI extmidiConnect(HMIDI, HMIDIOUT, LPVOID);

typedef MMRESULT (WINAPI *mixerGetControlDetailsA_Type)(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD);
mixerGetControlDetailsA_Type pmixerGetControlDetailsA;
MMRESULT WINAPI extmixerGetControlDetailsA(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD);

typedef MMRESULT (WINAPI *mixerSetControlDetails_Type)(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD);
mixerSetControlDetails_Type pmixerSetControlDetails;
MMRESULT WINAPI extmixerSetControlDetails(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD);

typedef BOOL (WINAPI *DriverCallback_Type)(DWORD_PTR, DWORD, HDRVR, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);
DriverCallback_Type pDriverCallback;
BOOL WINAPI extDriverCallback(DWORD_PTR, DWORD, HDRVR, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);

#ifdef TRACEMIXER
typedef UINT (WINAPI *mixerGetNumDevs_Type)(void);
mixerGetNumDevs_Type pmixerGetNumDevs;
UINT WINAPI extmixerGetNumDevs(void);

typedef MMRESULT (WINAPI *mixerOpen_Type)(LPHMIXER, UINT, DWORD_PTR, DWORD_PTR, DWORD);
mixerOpen_Type pmixerOpen;
MMRESULT WINAPI extmixerOpen(LPHMIXER, UINT, DWORD_PTR, DWORD_PTR, DWORD);

typedef MMRESULT (WINAPI *mixerClose_Type)(HMIXER);
mixerClose_Type pmixerClose;
MMRESULT WINAPI extmixerClose(HMIXER);

typedef MMRESULT (WINAPI *mixerGetLineControlsA_Type)(HMIXEROBJ, LPMIXERLINECONTROLS, DWORD);
mixerGetLineControlsA_Type pmixerGetLineControlsA;
MMRESULT WINAPI extmixerGetLineControlsA(HMIXEROBJ, LPMIXERLINECONTROLS, DWORD);

typedef MMRESULT (WINAPI *mixerGetLineInfoA_Type)(HMIXEROBJ, LPMIXERLINEA, DWORD);
mixerGetLineInfoA_Type pmixerGetLineInfoA;
MMRESULT WINAPI extmixerGetLineInfoA(HMIXEROBJ, LPMIXERLINEA, DWORD);

typedef MMRESULT (WINAPI *mixerGetDevCapsA_Type)(UINT_PTR, LPMIXERCAPSA, UINT);
mixerGetDevCapsA_Type pmixerGetDevCapsA;
MMRESULT WINAPI extmixerGetDevCapsA(UINT_PTR, LPMIXERCAPSA, UINT);

typedef MMRESULT (WINAPI *mixerGetID_Type)(HMIXEROBJ, UINT *, DWORD);
mixerGetID_Type pmixerGetID;
MMRESULT WINAPI extmixerGetID(HMIXEROBJ, UINT *, DWORD);

typedef DWORD (WINAPI *mixerMessage_Type)(HMIXER, UINT, DWORD_PTR, DWORD_PTR);
mixerMessage_Type pmixerMessage;
DWORD WINAPI extmixerMessage(HMIXER, UINT, DWORD_PTR, DWORD_PTR);
#endif // TRACEMIXER

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "mciSendCommandA", NULL, (FARPROC *)&pmciSendCommandA, (FARPROC)extmciSendCommandA},
	{HOOK_IAT_CANDIDATE, 0, "mciSendCommandW", NULL, (FARPROC *)&pmciSendCommandW, (FARPROC)extmciSendCommandW},
	{HOOK_HOT_CANDIDATE, 0, "mciSendStringA", NULL, (FARPROC *)&pmciSendStringA, (FARPROC)extmciSendStringA},
	{HOOK_HOT_CANDIDATE, 0, "mciSendStringW", NULL, (FARPROC *)&pmciSendStringW, (FARPROC)extmciSendStringW},
	{HOOK_HOT_CANDIDATE, 0, "mciGetDeviceIDA", NULL, (FARPROC *)&pmciGetDeviceIDA, (FARPROC)extmciGetDeviceIDA},
	{HOOK_HOT_CANDIDATE, 0, "mciGetDeviceIDW", NULL, (FARPROC *)&pmciGetDeviceIDW, (FARPROC)extmciGetDeviceIDW},
	{HOOK_HOT_CANDIDATE, 0, "auxGetDevCapsA", NULL, (FARPROC *)&pauxGetDevCapsA, (FARPROC)extauxGetDevCapsA},
	{HOOK_HOT_CANDIDATE, 0, "auxSetVolume", NULL, (FARPROC *)&pauxSetVolume, (FARPROC)extauxSetVolume},
	{HOOK_HOT_CANDIDATE, 0, "waveOutOpen", NULL, (FARPROC *)&pwaveOutOpen, (FARPROC)extwaveOutOpen},
	{HOOK_HOT_CANDIDATE, 0, "waveOutSetVolume", NULL, (FARPROC *)&pwaveOutSetVolume, (FARPROC)extwaveOutSetVolume},
	{HOOK_HOT_CANDIDATE, 0, "waveOutWrite", NULL, (FARPROC *)&pwaveOutWrite, (FARPROC)extwaveOutWrite},
	{HOOK_IAT_CANDIDATE, 0, "waveOutGetPosition", NULL, (FARPROC *)&pwaveOutGetPosition, (FARPROC)extwaveOutGetPosition},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type FixIdHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetDevCapsA", NULL, (FARPROC *)&pmidiOutGetDevCapsA, (FARPROC)extmidiOutGetDevCapsA},
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetDevCapsW", NULL, (FARPROC *)&pmidiOutGetDevCapsW, (FARPROC)extmidiOutGetDevCapsW},
	{HOOK_HOT_CANDIDATE, 0, "midiOutOpen", NULL, (FARPROC *)&pmidiOutOpen, (FARPROC)extmidiOutOpen},
	{HOOK_IAT_CANDIDATE, 0, "waveOutGetDevCapsA", NULL, (FARPROC *)&pwaveOutGetDevCapsA, (FARPROC)extwaveOutGetDevCapsA},
	{HOOK_IAT_CANDIDATE, 0, "waveOutGetDevCapsW", NULL, (FARPROC *)&pwaveOutGetDevCapsW, (FARPROC)extwaveOutGetDevCapsW},
	{HOOK_HOT_CANDIDATE, 0, "wod32Message", NULL, (FARPROC *)&pwod32Message, (FARPROC)extwod32Message},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type CDEmuHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "auxSetVolume", NULL, (FARPROC *)&pauxSetVolume, (FARPROC)extauxSetVolume},
	{HOOK_HOT_CANDIDATE, 0, "auxGetVolume", NULL, (FARPROC *)&pauxGetVolume, (FARPROC)extauxGetVolume},
	{HOOK_HOT_CANDIDATE, 0, "auxGetNumDevs", NULL, (FARPROC *)&pauxGetNumDevs, (FARPROC)extauxGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "auxOutMessage", NULL, (FARPROC *)&pauxOutMessage, (FARPROC)extauxOutMessage},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type TimeHooks[]={
	//{HOOK_HOT_CANDIDATE, 0, "timeKillEvent", NULL, (FARPROC *)&ptimeKillEvent, (FARPROC)exttimeKillEvent},
	//{HOOK_HOT_CANDIDATE, 0, "timeSetEvent", NULL, (FARPROC *)&ptimeSetEvent, (FARPROC)exttimeSetEvent},
	{HOOK_HOT_CANDIDATE, 0, "timeGetDevCaps", NULL, (FARPROC *)&ptimeGetDevCaps, (FARPROC)exttimeGetDevCaps},
	{HOOK_HOT_CANDIDATE, 0, "timeBeginPeriod", NULL, (FARPROC *)&ptimeBeginPeriod, (FARPROC)exttimeBeginPeriod},
	{HOOK_HOT_CANDIDATE, 0, "timeEndPeriod", NULL, (FARPROC *)&ptimeEndPeriod, (FARPROC)exttimeEndPeriod},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type EventHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "timeKillEvent", NULL, (FARPROC *)&ptimeKillEvent, (FARPROC)exttimeKillEvent},
	{HOOK_HOT_CANDIDATE, 0, "timeSetEvent", NULL, (FARPROC *)&ptimeSetEvent, (FARPROC)exttimeSetEvent},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type UpTimeHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "timeGetTime", NULL, (FARPROC *)&ptimeGetTime, (FARPROC)exttimeGetTime},
	{HOOK_HOT_CANDIDATE, 0, "timeGetSystemTime", NULL, (FARPROC *)&ptimeGetSystemTime, (FARPROC)exttimeGetSystemTime},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type JoyHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "joyGetNumDevs", NULL, (FARPROC *)&pjoyGetNumDevs, (FARPROC)extjoyGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "joyGetDevCapsA", NULL, (FARPROC *)&pjoyGetDevCapsA, (FARPROC)extjoyGetDevCapsA},
	{HOOK_HOT_CANDIDATE, 0, "joyGetPosEx", NULL, (FARPROC *)&pjoyGetPosEx, (FARPROC)extjoyGetPosEx},
	{HOOK_HOT_CANDIDATE, 0, "joyGetPos", NULL, (FARPROC *)&pjoyGetPos, (FARPROC)extjoyGetPos},
	{HOOK_HOT_CANDIDATE, 0, "joySetCapture", NULL, (FARPROC *)&pjoySetCapture, (FARPROC)extjoySetCapture},
	{HOOK_HOT_CANDIDATE, 0, "joyReleaseCapture", NULL, (FARPROC *)&pjoyReleaseCapture, (FARPROC)extjoyReleaseCapture},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type AudioHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "waveOutGetNumDevs", NULL, (FARPROC *)&pwaveOutGetNumDevs, (FARPROC)extwaveOutGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "waveOutOpen", NULL, (FARPROC *)&pwaveOutOpen, (FARPROC)extwaveOutOpen},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type MidiHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "midiOutReset", NULL, (FARPROC *)&pmidiOutReset, (FARPROC)extmidiOutReset},
	{HOOK_HOT_CANDIDATE, 0, "midiOutSetVolume", NULL, (FARPROC *)&pmidiOutSetVolume, (FARPROC)extmidiOutSetVolume},
	{HOOK_HOT_CANDIDATE, 0, "midiOutPrepareHeader", NULL, (FARPROC *)&pmidiOutPrepareHeader, (FARPROC)extmidiOutPrepareHeader},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamOut", NULL, (FARPROC *)&pmidiStreamOut, (FARPROC)extmidiStreamOut},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
}; 

static HookEntryEx_Type MidiBypassHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "midiOutShortMsg", NULL, (FARPROC *)&pmidiOutShortMsg, (FARPROC)extmidiOutShortMsg},
	{HOOK_HOT_CANDIDATE, 0, "midiOutLongMsg", NULL, (FARPROC *)&pmidiOutLongMsg, (FARPROC)extmidiOutLongMsg},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamPause", NULL, (FARPROC *)&pmidiStreamPause, (FARPROC)extmidiStreamPause},
	{HOOK_HOT_CANDIDATE, 0, "midiOutGetErrorTextA", NULL, (FARPROC *)&pmidiOutGetErrorTextA, (FARPROC)extmidiOutGetErrorTextA},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamClose", NULL, (FARPROC *)&pmidiStreamClose, (FARPROC)extmidiStreamClose},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamOpen", NULL, (FARPROC *)&pmidiStreamOpen, (FARPROC)extmidiStreamOpen},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamStop", NULL, (FARPROC *)&pmidiStreamStop, (FARPROC)extmidiStreamStop},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamRestart", NULL, (FARPROC *)&pmidiStreamRestart, (FARPROC)extmidiStreamRestart},
	{HOOK_HOT_CANDIDATE, 0, "midiOutUnprepareHeader", NULL, (FARPROC *)&pmidiOutUnprepareHeader, (FARPROC)extmidiOutUnprepareHeader},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamProperty", NULL, (FARPROC *)&pmidiStreamProperty, (FARPROC)extmidiStreamProperty},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamPosition", NULL, (FARPROC *)&pmidiStreamPosition, (FARPROC)extmidiStreamPosition},
	{HOOK_IAT_CANDIDATE, 0, "midiOutClose", NULL, (FARPROC *)&pmidiOutClose, (FARPROC)extmidiOutClose},
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetDevCapsA", NULL, (FARPROC *)&pmidiOutGetDevCapsA, (FARPROC)extmidiOutGetDevCapsA},
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetDevCapsW", NULL, (FARPROC *)&pmidiOutGetDevCapsW, (FARPROC)extmidiOutGetDevCapsW},
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetNumDevs", NULL, (FARPROC *)&pmidiOutGetNumDevs, (FARPROC)extmidiOutGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "midiOutOpen", NULL, (FARPROC *)&pmidiOutOpen, (FARPROC)extmidiOutOpen},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
}; 

static HookEntryEx_Type PathHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "mmioOpenA", NULL, (FARPROC *)&pmmioOpenA, (FARPROC)extmmioOpenA},
	{HOOK_HOT_CANDIDATE, 0, "mmioOpenW", NULL, (FARPROC *)&pmmioOpenW, (FARPROC)extmmioOpenW},
	{HOOK_HOT_CANDIDATE, 0, "sndPlaySoundA", NULL, (FARPROC *)&psndPlaySoundA, (FARPROC)extsndPlaySoundA},
	{HOOK_HOT_CANDIDATE, 0, "sndPlaySoundW", NULL, (FARPROC *)&psndPlaySoundW, (FARPROC)extsndPlaySoundW},
	{HOOK_HOT_CANDIDATE, 0, "PlaySoundA", NULL, (FARPROC *)&pPlaySoundA, (FARPROC)extPlaySoundA},
	{HOOK_HOT_CANDIDATE, 0, "PlaySoundW", NULL, (FARPROC *)&pPlaySoundW, (FARPROC)extPlaySoundW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type PlaySoundHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "PlaySoundA", NULL, (FARPROC *)&pPlaySoundA, (FARPROC)extPlaySoundA},
	{HOOK_HOT_CANDIDATE, 0, "PlaySoundW", NULL, (FARPROC *)&pPlaySoundW, (FARPROC)extPlaySoundW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type MixerHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "mixerGetControlDetailsA", NULL, (FARPROC *)&pmixerGetControlDetailsA, (FARPROC)extmixerGetControlDetailsA},
	{HOOK_IAT_CANDIDATE, 0, "mixerSetControlDetails", NULL, (FARPROC *)&pmixerSetControlDetails, (FARPROC)extmixerSetControlDetails},
#ifdef TRACEMIXER
	{HOOK_IAT_CANDIDATE, 0, "mixerOpen", NULL, (FARPROC *)&pmixerOpen, (FARPROC)extmixerOpen},
	{HOOK_IAT_CANDIDATE, 0, "mixerClose", NULL, (FARPROC *)&pmixerClose, (FARPROC)extmixerClose},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetLineInfoA", NULL, (FARPROC *)&pmixerGetLineInfoA, (FARPROC)extmixerGetLineInfoA},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetLineControlsA", NULL, (FARPROC *)&pmixerGetLineControlsA, (FARPROC)extmixerGetLineControlsA},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetNumDevs", NULL, (FARPROC *)&pmixerGetNumDevs, (FARPROC)extmixerGetNumDevs},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetDevCapsA", NULL, (FARPROC *)&pmixerGetDevCapsA, (FARPROC)extmixerGetDevCapsA},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetID", NULL, (FARPROC *)&pmixerGetID, (FARPROC)extmixerGetID},
	{HOOK_IAT_CANDIDATE, 0, "mixerMessage", NULL, (FARPROC *)&pmixerMessage, (FARPROC)extmixerMessage},
#endif // TRACEMIXER
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type waveFixHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "waveOutUnprepareHeader", NULL, (FARPROC *)&pwaveOutUnprepareHeader, (FARPROC)extwaveOutUnprepareHeader},
	{HOOK_IAT_CANDIDATE, 0, "waveOutClose", NULL, (FARPROC *)&pwaveOutClose, (FARPROC)extwaveOutClose},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type DebugHooks[]={
	// commented to avoid recursive call and stack overflow
	//{HOOK_IAT_CANDIDATE, 0, "auxGetNumDevs", NULL, (FARPROC *)&pauxGetNumDevs, (FARPROC)extauxGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "auxGetVolume", NULL, (FARPROC *)&pauxGetVolume, (FARPROC)extauxGetVolume},
	{HOOK_IAT_CANDIDATE, 0, "mciGetErrorStringA", NULL, (FARPROC *)&pmciGetErrorStringA, (FARPROC)extmciGetErrorStringA},
	{HOOK_IAT_CANDIDATE, 0, "waveOutClose", NULL, (FARPROC *)&pwaveOutClose, (FARPROC)extwaveOutClose},
	{HOOK_IAT_CANDIDATE, 0, "waveOutGetDevCapsA", NULL, (FARPROC *)&pwaveOutGetDevCapsA, (FARPROC)extwaveOutGetDevCapsA},
	{HOOK_IAT_CANDIDATE, 0, "waveOutGetNumDevs", NULL, (FARPROC *)&pwaveOutGetNumDevs, (FARPROC)extwaveOutGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "waveOutGetVolume", NULL, (FARPROC *)&pwaveOutGetVolume, (FARPROC)extwaveOutGetVolume},
	{HOOK_HOT_CANDIDATE, 0, "waveOutOpen", NULL, (FARPROC *)&pwaveOutOpen, (FARPROC)extwaveOutOpen},
	{HOOK_IAT_CANDIDATE, 0, "waveOutPrepareHeader", NULL, (FARPROC *)&pwaveOutPrepareHeader, (FARPROC)extwaveOutPrepareHeader},
	{HOOK_IAT_CANDIDATE, 0, "waveOutReset", NULL, (FARPROC *)&pwaveOutReset, (FARPROC)extwaveOutReset},
	{HOOK_IAT_CANDIDATE, 0, "waveOutPause", NULL, (FARPROC *)&pwaveOutPause, (FARPROC)extwaveOutPause},
	{HOOK_IAT_CANDIDATE, 0, "waveOutRestart", NULL, (FARPROC *)&pwaveOutRestart, (FARPROC)extwaveOutRestart},
	{HOOK_IAT_CANDIDATE, 0, "waveOutUnprepareHeader", NULL, (FARPROC *)&pwaveOutUnprepareHeader, (FARPROC)extwaveOutUnprepareHeader},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetControlDetailsA", NULL, (FARPROC *)&pmixerGetControlDetailsA, (FARPROC)extmixerGetControlDetailsA},
	{HOOK_IAT_CANDIDATE, 0, "mixerSetControlDetails", NULL, (FARPROC *)&pmixerSetControlDetails, (FARPROC)extmixerSetControlDetails},
	{HOOK_IAT_CANDIDATE, 0, "DriverCallback", NULL, (FARPROC *)&pDriverCallback, (FARPROC)extDriverCallback},
#ifdef TRACEMIXER
	{HOOK_IAT_CANDIDATE, 0, "mixerOpen", NULL, (FARPROC *)&pmixerOpen, (FARPROC)extmixerOpen},
	{HOOK_IAT_CANDIDATE, 0, "mixerClose", NULL, (FARPROC *)&pmixerClose, (FARPROC)extmixerClose},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetLineInfoA", NULL, (FARPROC *)&pmixerGetLineInfoA, (FARPROC)extmixerGetLineInfoA},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetLineControlsA", NULL, (FARPROC *)&pmixerGetLineControlsA, (FARPROC)extmixerGetLineControlsA},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetNumDevs", NULL, (FARPROC *)&pmixerGetNumDevs, (FARPROC)extmixerGetNumDevs},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetDevCapsA", NULL, (FARPROC *)&pmixerGetDevCapsA, (FARPROC)extmixerGetDevCapsA},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetID", NULL, (FARPROC *)&pmixerGetID, (FARPROC)extmixerGetID},
	{HOOK_IAT_CANDIDATE, 0, "mixerMessage", NULL, (FARPROC *)&pmixerMessage, (FARPROC)extmixerMessage},
#endif // TRACEMIXER
	{HOOK_HOT_CANDIDATE, 0, "midiOutShortMsg", NULL, (FARPROC *)&pmidiOutShortMsg, (FARPROC)extmidiOutShortMsg},
	{HOOK_HOT_CANDIDATE, 0, "midiOutLongMsg", NULL, (FARPROC *)&pmidiOutLongMsg, (FARPROC)extmidiOutLongMsg},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamPause", NULL, (FARPROC *)&pmidiStreamPause, (FARPROC)extmidiStreamPause},
	{HOOK_HOT_CANDIDATE, 0, "midiOutGetErrorTextA", NULL, (FARPROC *)&pmidiOutGetErrorTextA, (FARPROC)extmidiOutGetErrorTextA},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamClose", NULL, (FARPROC *)&pmidiStreamClose, (FARPROC)extmidiStreamClose},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamOpen", NULL, (FARPROC *)&pmidiStreamOpen, (FARPROC)extmidiStreamOpen},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamStop", NULL, (FARPROC *)&pmidiStreamStop, (FARPROC)extmidiStreamStop},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamRestart", NULL, (FARPROC *)&pmidiStreamRestart, (FARPROC)extmidiStreamRestart},
	{HOOK_HOT_CANDIDATE, 0, "midiOutUnprepareHeader", NULL, (FARPROC *)&pmidiOutUnprepareHeader, (FARPROC)extmidiOutUnprepareHeader},
	{HOOK_HOT_CANDIDATE, 0, "midiStreamProperty", NULL, (FARPROC *)&pmidiStreamProperty, (FARPROC)extmidiStreamProperty},
	{HOOK_IAT_CANDIDATE, 0, "midiOutClose", NULL, (FARPROC *)&pmidiOutClose, (FARPROC)extmidiOutClose},
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetDevCapsA", NULL, (FARPROC *)&pmidiOutGetDevCapsA, (FARPROC)extmidiOutGetDevCapsA},
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetDevCapsW", NULL, (FARPROC *)&pmidiOutGetDevCapsW, (FARPROC)extmidiOutGetDevCapsW},
	{HOOK_IAT_CANDIDATE, 0, "midiOutGetNumDevs", NULL, (FARPROC *)&pmidiOutGetNumDevs, (FARPROC)extmidiOutGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "midiOutOpen", NULL, (FARPROC *)&pmidiOutOpen, (FARPROC)extmidiOutOpen},
	{HOOK_HOT_CANDIDATE, 0, "mmioDescend", NULL, (FARPROC *)&pmmioDescend, (FARPROC)extmmioDescend},
	{HOOK_HOT_CANDIDATE, 0, "mmioSeek", NULL, (FARPROC *)&pmmioSeek, (FARPROC)extmmioSeek},
	{HOOK_HOT_CANDIDATE, 0, "mmioRead", NULL, (FARPROC *)&pmmioRead, (FARPROC)extmmioRead},
	{HOOK_HOT_CANDIDATE, 0, "mmioSetInfo", NULL, (FARPROC *)&pmmioSetInfo, (FARPROC)extmmioSetInfo},
	{HOOK_HOT_CANDIDATE, 0, "mmioGetInfo", NULL, (FARPROC *)&pmmioGetInfo, (FARPROC)extmmioGetInfo},
	{HOOK_HOT_CANDIDATE, 0, "mmioClose", NULL, (FARPROC *)&pmmioClose, (FARPROC)extmmioClose},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookWinMM(HMODULE module)
{
	//char *libname = SysLibsTable[SYSLIBIDX_WINMM].name;
	char *libname = "winmm.dll";
	HookLibraryEx(module, Hooks, libname);
	if((dxw.dwFlags11 & FIXDEFAULTMCIID) || (dxw.dwFlags14 & WAVEOUTUSEPREFDEV) || (dxw.dwFlags19 & NOWAVEOUT)) 
		HookLibraryEx(module, FixIdHooks, libname);
	if(dxw.dwFlags8 & VIRTUALCDAUDIO) HookLibraryEx(module, CDEmuHooks, libname);
	if((dxw.dwFlags2 & TIMESTRETCH) && (dxw.dwFlags4 & STRETCHTIMERS)) HookLibraryEx(module, TimeHooks, libname);
	if(((dxw.dwFlags2 & TIMESTRETCH) && (dxw.dwFlags4 & STRETCHTIMERS)) || (dxw.dwFlags19 & HOOKLEGACYEVENTS))
		HookLibraryEx(module, EventHooks, libname);
	if((dxw.dwFlags2 & TIMESTRETCH) || (dxw.dwFlags14 & (UPTIMECLEAR | UPTIMESTRESS)))
		HookLibraryEx(module, UpTimeHooks, libname);
	if((dxw.dwFlags8 & FIXAUDIOPCM) || (dxw.dwFlags14 & MW2WAVEOUTFIX) || (dxw.dwFlags19 & NOWAVEOUT) || (dxw.dwFlags19 & HOOKLEGACYWAVE)) 
		HookLibraryEx(module, AudioHooks, libname);
	if(dxw.dwFlags13 & PLAYSOUNDFIX) HookLibraryEx(module, PlaySoundHooks, libname);
	HookLibraryEx(module, MidiHooks, libname);
	if((dxw.dwFlags14 & MIDIOUTBYPASS) || (dxw.dwFlags9 & LOCKVOLUME))  
		HookLibraryEx(module, MidiBypassHooks, libname);
	if((dxw.dwFlags6 & VIRTUALJOYSTICK) || (dxw.dwFlags9 & HIDEJOYSTICKS)) HookLibraryEx(module, JoyHooks, libname);
	if(IsDebugSND || (dxw.dwFlags11 & CUSTOMLOCALE) || (dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE))) 
		HookLibraryEx(module, PathHooks, libname);
	if(dxw.dwFlags13 & EMULATECDMIXER) HookLibraryEx(module, MixerHooks, libname);
	if(dxw.dwFlags19 & ASYNCWAVEOUTCLOSE) HookLibraryEx(module, waveFixHooks, libname);
	if(IsDebugSND) HookLibraryEx(module, DebugHooks, libname);

	extern void Hook_AIL_redbook(HMODULE);
	Hook_AIL_redbook(module);

}

FARPROC Remap_WinMM_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;

	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	if((dxw.dwFlags11 & FIXDEFAULTMCIID) || (dxw.dwFlags14 & WAVEOUTUSEPREFDEV)) if (addr=RemapLibraryEx(proc, hModule, FixIdHooks)) return addr;
	if(dxw.dwFlags8 & VIRTUALCDAUDIO) if (addr=RemapLibraryEx(proc, hModule, CDEmuHooks)) return addr;
	if((dxw.dwFlags2 & TIMESTRETCH) && (dxw.dwFlags4 & STRETCHTIMERS))
		if (addr=RemapLibraryEx(proc, hModule, TimeHooks)) return addr;
	if(((dxw.dwFlags2 & TIMESTRETCH) && (dxw.dwFlags4 & STRETCHTIMERS)) || (dxw.dwFlags19 & HOOKLEGACYEVENTS)) 
		if (addr=RemapLibraryEx(proc, hModule, EventHooks)) return addr;
	if((dxw.dwFlags2 & TIMESTRETCH) || (dxw.dwFlags14 & (UPTIMECLEAR | UPTIMESTRESS)))
		if (addr=RemapLibraryEx(proc, hModule, UpTimeHooks)) return addr;
	if((dxw.dwFlags8 & FIXAUDIOPCM) || (dxw.dwFlags14 & MW2WAVEOUTFIX) || (dxw.dwFlags19 & NOWAVEOUT) || (dxw.dwFlags19 & HOOKLEGACYWAVE)) 
		if (addr=RemapLibraryEx(proc, hModule, AudioHooks)) return addr;
	if(dxw.dwFlags13 & PLAYSOUNDFIX) if (addr=RemapLibraryEx(proc, hModule, PlaySoundHooks)) return addr;
	if (addr=RemapLibraryEx(proc, hModule, MidiHooks)) return addr;
	if(dxw.dwFlags14 & MIDIOUTBYPASS) if (addr=RemapLibraryEx(proc, hModule, MidiBypassHooks)) return addr;
	if((dxw.dwFlags6 & VIRTUALJOYSTICK) || (dxw.dwFlags9 & HIDEJOYSTICKS)) if (addr=RemapLibraryEx(proc, hModule, JoyHooks)) return addr;
	if(IsDebugSND || (dxw.dwFlags11 & CUSTOMLOCALE) || (dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE))) if (addr=RemapLibraryEx(proc, hModule, PathHooks)) return addr;
	if(dxw.dwFlags13 & EMULATECDMIXER) if (addr=RemapLibraryEx(proc, hModule, MixerHooks)) return addr;
	if(IsDebugDW) if (addr=RemapLibraryEx(proc, hModule, DebugHooks)) return addr;

	return NULL;
}

void HookWinMMInit()
{
	//if(dxw.dwFlags8 & VIRTUALCDAUDIO) HookLibInitEx(CDEmuHooks);
	// done with dynamic link not to create dependency with winmm
	if(dxw.dwFlags8 & VIRTUALCDAUDIO) {
		HMODULE hWinMM = (*pLoadLibraryA)("winmm.dll");
		if(!hWinMM) return;
		pauxGetNumDevs = (auxGetNumDevs_Type)(*pGetProcAddress)(hWinMM, "auxGetNumDevs");
	}
}

static void emuSetVolume(UINT nVolume)
{
	if(!pplr_pump) player_init();
	IsWithinMCICall = TRUE;
	(*pplr_setvolume)(nVolume);
	IsWithinMCICall = FALSE;
}

static DWORD MapVolume(int volume)
{
	// maps a volume in the range 0 - 100 to the double WORD calues
	// in the range 0x0000 - 0xFFFF accepted vy mci calls.
	// No volume balance at the moment ...
	DWORD dwVol;
	if(volume <= 0) return 0x00000000;
	if(volume >= 100) return 0xFFFFFFFF;
	dwVol = (volume * 0xFFFF) / 100;
	dwVol = dwVol | (dwVol << 16);
	return dwVol;
}

#ifndef DXW_NOTRACES
char *mmErrorString(MMRESULT err)
{
	char *syserr[] = {
		"NOERROR", "ERROR", "BADDEVICEID", "NOTENABLED", "ALLOCATED", "INVALHANDLE",
		"NODRIVER", "NOMEM", "NOTSUPPORTED", "BADERRNUM", "INVALFLAG", "INVALPARAM",
		"HANDLEBUSY", "INVALIDALIAS", "BADDB", "KEYNOTFOUND", "READERROR", "WRITEERROR",
		"DELETEERROR", "VALNOTFOUND", "NODRIVERCB", "MOREDATA"};
	char *waverr[] = {
		"BADFORMAT", "STILLPLAYING", "UNPREPARED", "SYNC"};
	char *miderr[] = {
		"UNPREPARED", "STILLPLAYING", "NOMAP", "NOTREADY", "NODEVICE", "INVALIDSETUP",
		"BADOPENMODE", "DONT_CONTINUE"};
	char *mixerr[] = {
		"INVALLINE", "INVALCONTROL", "INVALVALUE"};
	char *mcierr[] = {
		"BASE", "INVALID_DEVICE_ID", "?", "UNRECOGNIZED_KEYWORD", "?", "UNRECOGNIZED_COMMAND",
		"HARDWARE", "INVALID_DEVICE_NAME", "OUT_OF_MEMORY", "DEVICE_OPEN", "CANNOT_LOAD_DRIVER", "MISSING_COMMAND_STRING",
		"PARAM_OVERFLOW", "MISSING_STRING_ARGUMENT", "BAD_INTEGER", "PARSER_INTERNAL", "DRIVER_INTERNAL", "MISSING_PARAMETER",
		"UNSUPPORTED_FUNCTION", "FILE_NOT_FOUND", "DEVICE_NOT_READY", "INTERNAL", "DRIVER", "CANNOT_USE_ALL",
		"MULTIPLE", "EXTENSION_NOT_FOUND", "OUTOFRANGE", "?", "FLAGS_NOT_COMPATIBLE", "?",
		"FILE_NOT_SAVED", "DEVICE_TYPE_REQUIRED", "DEVICE_LOCKED", "DUPLICATE_ALIAS", "BAD_CONSTANT", "MUST_USE_SHAREABLE",
		"MISSING_DEVICE_NAME", "BAD_TIME_FORMAT", "NO_CLOSING_QUOTE", "DUPLICATE_FLAGS", "INVALID_FILE", "NULL_PARAMETER_BLOCK",
		"UNNAMED_RESOURCE", "NEW_REQUIRES_ALIAS", "NOTIFY_ON_AUTO_OPEN", "NO_ELEMENT_ALLOWED", "NONAPPLICABLE_FUNCTION", "ILLEGAL_FOR_AUTO_OPEN",
		"FILENAME_REQUIRED", "EXTRA_CHARACTERS", "DEVICE_NOT_INSTALLED", "GET_CD", "SET_CD", "SET_DRIVE",
		"DEVICE_LENGTH", "DEVICE_ORD_LENGTH", "NO_INTEGER", "?", "?", "?",
		"?", "?", "?", "?", "WAVE_OUTPUTSINUSE", "WAVE_SETOUTPUTINUSE",
		"WAVE_INPUTSINUSE", "WAVE_SETINPUTINUSE", "WAVE_OUTPUTUNSPECIFIED", "WAVE_INPUTUNSPECIFIED", "WAVE_OUTPUTSUNSUITABLE", "WAVE_SETOUTPUTUNSUITABLE",
		"WAVE_INPUTSUNSUITABLE", "WAVE_SETINPUTUNSUITABLE", "?", "?", "?", "?",
		"?", "?", "SEQ_DIV_INCOMPATIBLE", "SEQ_PORT_INUSE", "SEQ_PORT_NONEXISTENT", "SEQ_PORT_MAPNODEVICE",
		"SEQ_PORT_MISCERROR", "SEQ_TIMER", "SEQ_PORTUNSPECIFIED", "SEQ_NOMIDIPRESENT", "?", "?",
		"NO_WINDOW", "CREATEWINDOW", "FILE_READ", "FILE_WRITE", "NO_IDENTITY"};

	if((err >= MMSYSERR_NOERROR) && (err <= MMSYSERR_LASTERROR)) return syserr[err];
	if((err >= WAVERR_BASE) && (err <= WAVERR_LASTERROR)) return waverr[err-WAVERR_BASE];
	if((err >= MIDIERR_BASE) && (err <= MIDIERR_LASTERROR)) return miderr[err-MIDIERR_BASE];
	if((err >= MIXERR_BASE) && (err <= MIXERR_LASTERROR)) return mixerr[err-MIXERR_BASE];
	if(err == TIMERR_NOCANDO) return "NOCANDO";
	if(err == TIMERR_STRUCT) return "STRUCT";
	// BEWARE: MMIOERR_BASE and MCIERR_BASE are both equal to 256, messages could be confused!!
	if((err >= MCIERR_BASE) && (err <= MCIERR_NO_IDENTITY)) return mcierr[err-MCIERR_BASE];
	return "???";
}

char *mioErrorString(MMRESULT err)
{
	char *mioerr[] = {
		"BASE", "FILENOTFOUND", "OUTOFMEMORY", "CANNOTOPEN", "CANNOTCLOSE", "CANNOTREAD",
		"CANNOTWRITE", "CANNOTSEEK", "CANNOTEXPAND", "CHUNKNOTFOUND", "UNBUFFERED", "PATHNOTFOUND",
		"ACCESSDENIED", "SHARINGVIOLATION", "NETWORKERROR", "TOOMANYOPENFILES", "INVALIDFILE"};
	if((err >= MMIOERR_BASE) && (err <= MMIOERR_INVALIDFILE)) return mioerr[err-MMIOERR_BASE];
	return "???";
}

void dumpMmioInfo(LPCMMIOINFO pmmioinfo)
{
	OutTrace("> dwFlags=%#x\n> fccIOProc=%#x\n> pIOProc=%#x\n> wErrorRet=%#x\n> htask=%#x\n",
		pmmioinfo->dwFlags,
		pmmioinfo->fccIOProc,
		pmmioinfo->pIOProc,
		pmmioinfo->wErrorRet,
		pmmioinfo->htask);
	OutTrace("> cchBuffer=%#x\n> pchBuffer=%#x\n> pchNext=%#x\n> pchEndRead=%#x\n> pchEndWrite=%#x\n> lBufOffset=%#x\n",
		pmmioinfo->cchBuffer,
		pmmioinfo->pchBuffer,
		pmmioinfo->pchNext,
		pmmioinfo->pchEndRead,
		pmmioinfo->pchEndWrite,
		pmmioinfo->lBufOffset);
	OutTrace("> lDiskOffset=%#x\n> adwInfo=[%#x:%#x:%#x]\n> dwReserved1=%#x\n> dwReserved2=%#x\n> hmmio=%#x\n",
		pmmioinfo->lDiskOffset,
		pmmioinfo->adwInfo[0], pmmioinfo->adwInfo[1], pmmioinfo->adwInfo[2],
		pmmioinfo->dwReserved1,
		pmmioinfo->dwReserved2,
		pmmioinfo->hmmio);
}
#endif // DXW_NOTRACES

typedef void (__stdcall *PWINMM_TIMESETEVENT_CALLBACK)(int p1, int p2, int p3, int p4, int p5);

typedef struct 
{
    PWINMM_TIMESETEVENT_CALLBACK original_callback;
    int original_dwUser;
} _winmm_naked_callback_data;

#ifdef _DEBUG
#pragma runtime_checks ("s", off)
#endif
void __declspec(naked)
_winmm_naked_callback(int p1, int p2, _winmm_naked_callback_data *p3, int p4, int p5)
{
    __asm 
    {
        push    ebp     
        mov     ebp, esp

        push    ebx
        push    esi
        push    edi
    }

	OutDebugT("voidCallback: timerId=%d msg=%#x user=%#x dw1/2=%#x/%#x\n",
		p1, p2, p3->original_dwUser, p4, p5);

    p3->original_callback(p1, p2, p3->original_dwUser, p4, p5);

    __asm 
    {
        pop edi
        pop esi
        pop ebx

        mov esp, ebp       
        pop ebp            
        ret 20
    }
}
#ifdef _DEBUG
#pragma runtime_checks ("s", restore)
#endif

MMRESULT WINAPI exttimeGetDevCaps(LPTIMECAPS ptc, UINT cbtc)
{
	MMRESULT res;
	ApiName("timeGetDevCaps");
	res = (*ptimeGetDevCaps)(ptc, cbtc);
	if(res) {
		OutErrorSYS("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));
	}
	else {
		OutTraceSYS("%s: period min=%d max=%d\n", ApiRef, ptc->wPeriodMin, ptc->wPeriodMax);
	}
	return MMSYSERR_NOERROR;
}

DWORD WINAPI exttimeGetTime(void)
{
	DWORD ret;
	ApiName("timeGetTime");
	ret = dxw.GetTickCount() + dxw.AddedTimeInMS.LowPart;
	OutTraceT("%s: time=%#x\n", ApiRef, ret);
	return ret;
}

DWORD WINAPI exttimeGetSystemTime(LPMMTIME pmmt, UINT cbmmt)
{
	DWORD ret;
	ApiName("timeGetSystemTime");
	if(cbmmt >= sizeof(MMTIME)){
		pmmt->wType = TIME_MS;
		pmmt->u.ms = dxw.GetTickCount() + dxw.AddedTimeInMS.LowPart;
		ret = TIMERR_NOERROR;
		OutTraceT("%s: time=%#x\n", ApiRef, ret);
	}
	else {
		ret = TIMERR_STRUCT;
		OutTraceT("%s: ERROR TIMERR_STRUCT\n", ApiRef);
	}
	return ret;
}

MMRESULT WINAPI exttimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
{
	MMRESULT res;
	ApiName("timeSetEvent");
	UINT NewDelay;
	OutTraceT("%s: Delay=%d Resolution=%d Event=%#x(%s+%s) timeproc=%#x\n", 
		ApiRef, uDelay, uResolution, 
		fuEvent, 
		(fuEvent & TIME_PERIODIC) ? "PERIODIC" : "ONESHOT",
		(fuEvent & TIME_CALLBACK_EVENT_PULSE) ? "EVENT_PULSE" : ((fuEvent & TIME_CALLBACK_EVENT_SET) ? "EVENT_SET" : "FUNCTION"),
		lpTimeProc);

	if(dxw.dwFlags19 & HOOKLEGACYEVENTS) {
		OutTraceT("%s: routing callback to connectorCallback\n", ApiRef);
		_winmm_naked_callback_data *c = (_winmm_naked_callback_data *)malloc(sizeof(_winmm_naked_callback_data));
		c->original_callback = (PWINMM_TIMESETEVENT_CALLBACK)lpTimeProc;
		c->original_dwUser = dwUser;
		dwUser = (DWORD_PTR)c; 
		lpTimeProc = (LPTIMECALLBACK)_winmm_naked_callback;
	}

	// v2.05.90: fix
	if((dxw.dwFlags4 & STRETCHTIMERS) && (dxw.dwFlags2 & TIMESTRETCH)) {
		NewDelay = dxw.StretchTime(uDelay);
		res=(*ptimeSetEvent)(NewDelay, uResolution, lpTimeProc, dwUser, fuEvent);
		if(res) dxw.PushTimer(res, uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
	}
	else {
		res=(*ptimeSetEvent)(uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
	}

	if(res) {
		OutTraceT("%s: res(hTimer)=%#x\n", ApiRef, res);
	}
	else {
		OutErrorT("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

MMRESULT WINAPI exttimeKillEvent(UINT uTimerID)
{
	MMRESULT res;
	ApiName("timeKillEvent");
	OutTrace("%s: TimerID=%#x\n", ApiRef, uTimerID);

	res=(*ptimeKillEvent)(uTimerID);

	// v2.05.90: fix
	if((res==TIMERR_NOERROR) && (dxw.dwFlags4 & STRETCHTIMERS)) dxw.PopTimer(uTimerID);
	_if(res) OutTrace("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));
	return res;
}

MMRESULT WINAPI exttimeBeginPeriod(UINT uPeriod)
{
	MMRESULT res;
	ApiName("timeBeginPeriod");
	OutTraceT("%s: period=%d\n", ApiRef, uPeriod);
	res=(*ptimeBeginPeriod)(uPeriod);
	OutTraceT("%s: ret=%#x\n", ApiRef, res);
	return res;
}

MMRESULT WINAPI exttimeEndPeriod(UINT uPeriod)
{
	MMRESULT res;
	ApiName("timeEndPeriod");
	OutTraceT("%s: period=%d\n", ApiRef, uPeriod);
	res=(*ptimeEndPeriod)(uPeriod);
	OutTraceT("%s: ret=%#x\n", ApiRef, res);
	return res;
}

/* MCI_DGV_PUT_FRAME

    The rectangle defined for MCI_DGV_RECT applies to the frame rectangle. 
	The frame rectangle specifies the portion of the frame buffer used as the destination of the video images obtained from the video rectangle. 
	The video should be scaled to fit within the frame buffer rectangle.
    The rectangle is specified in frame buffer coordinates. 
	The default rectangle is the full frame buffer. 
	Specifying this rectangle lets the device scale the image as it digitizes the data. 
	Devices that cannot scale the image reject this command with MCIERR_UNSUPPORTED_FUNCTION. 
	You can use the MCI_GETDEVCAPS_CAN_STRETCH flag with the MCI_GETDEVCAPS command to determine if a device scales the image. A device returns FALSE if it cannot scale the image.
*/

MCIDEVICEID WINAPI extmciGetDeviceIDA(LPCTSTR lpszDevice)
{
	MCIDEVICEID ret;
	ApiName("mciGetDeviceIDA");
	OutTraceDW("%s: device=\"%s\"\n", ApiRef, lpszDevice);
	ret = (*pmciGetDeviceIDA)(lpszDevice);
	if(dxw.dwFlags8 & VIRTUALCDAUDIO) {
		if(!strcmp(lpszDevice, "cd") || !strcmp(lpszDevice, "cdaudio")) {
			OutTraceSND("%s: detected cdaudio device\n", ApiRef);
			ret = dxw.VirtualCDAudioDeviceId;
		}
	}
	OutTraceDW("%s: device=\"%s\" ret=%#x\n", ApiRef, lpszDevice, ret);
	return ret;
}

MCIDEVICEID WINAPI extmciGetDeviceIDW(LPCWSTR lpszDevice)
{
	MCIDEVICEID ret;
	ApiName("mciGetDeviceIDW");
	OutTraceDW("%s: device=\"%ls\"\n", ApiRef, lpszDevice);
	ret = (*pmciGetDeviceIDW)(lpszDevice);
	if(dxw.dwFlags8 & VIRTUALCDAUDIO) {
		if(!wcscmp(lpszDevice, L"cd") || !wcscmp(lpszDevice, L"cdaudio")) {
			OutTraceSND("%s: detected cdaudio device\n", ApiRef);
			ret = dxw.VirtualCDAudioDeviceId;
		}
	}
	OutTraceDW("%s: device=\"%ls\" ret=%#x\n", ApiRef, lpszDevice, ret);
	return ret;
}

DWORD WINAPI extjoyGetNumDevs(void)
{
	DWORD ret;

	if(dxw.dwFlags9 & HIDEJOYSTICKS) {
		OutTraceDW("joyGetNumDevs: hide joystick ret=0\n");
		return 0;
	}
	if(dxw.dwFlags6 & VIRTUALJOYSTICK) {
		OutTraceDW("joyGetNumDevs: emulate joystick ret=1\n");
		return 1;
	}
	ret = (*pjoyGetNumDevs)();
	OutTraceDW("joyGetNumDevs: ret=%d\n", ret);
	return ret;
}


MMRESULT WINAPI extjoyGetDevCapsA(DWORD uJoyID, LPJOYCAPSA pjc, UINT cbjc)
{
	MMRESULT ret;
	ApiName("joyGetDevCaps");

	OutTraceDW("%s: joyid=%d size=%d\n", ApiRef, uJoyID, cbjc);

	if(dxw.dwFlags9 & HIDEJOYSTICKS) {
		OutTraceDW("%s: hide joystick ret=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}

	if(dxw.dwFlags6 & VIRTUALJOYSTICK) {
		if((uJoyID != -1) && (uJoyID != 0)) {
			OutTraceDW("%s: ERROR joyid=%d ret=MMSYSERR_NODRIVER\n", ApiRef, uJoyID, cbjc);
			return MMSYSERR_NODRIVER;
		}
		if(cbjc != sizeof(JOYCAPS)) {
			OutTraceDW("%s: ERROR joyid=%d size=%d ret=MMSYSERR_INVALPARAM\n", ApiRef, uJoyID, cbjc);
			return MMSYSERR_INVALPARAM;
		}
		// set Joystick capability structure
		vjGetCaps(pjc);
		ret = JOYERR_NOERROR;
	}
	else {
		ret = (*pjoyGetDevCapsA)(uJoyID, pjc, cbjc);
	}

	if(ret != JOYERR_NOERROR) {
		OutTraceE("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

	OutTraceDW("joyGetDevCaps: caps={"
		"manif.id=%#x prod.id=%#x name=\"%s\" "
		"pos(x:y:z)(min/max)=(%d/%d:%d/%d:%d/%d) "
		"num.buttons=%d period(min/max)=(%d/%d)"
		" caps=%#x maxaxes=%d numaxes=%d maxbtns=%d "
		"pos(r:u:v)(min/max)=(%d/%d:%d/%d:%d/%d) "
		"regkey=\"%s\" oem=\"%s\""
		"}\n", 
		pjc->wMid, pjc->wPid, pjc->szPname, 
		pjc->wXmin, pjc->wXmax, pjc->wYmin, pjc->wYmax, pjc->wZmin, pjc->wZmax, 
		pjc->wNumButtons, pjc->wPeriodMin, pjc->wPeriodMax
		, pjc->wCaps, pjc->wMaxAxes, pjc->wNumAxes, pjc->wMaxButtons,
		pjc->wRmin, pjc->wRmax, pjc->wUmin, pjc->wUmax, pjc->wVmin, pjc->wVmax,
		pjc->szRegKey, pjc->szOEMVxD
		);

	return JOYERR_NOERROR;
}

BOOL JoyProcessMouseWheelMessage(WPARAM wParam, LPARAM lParam)
{
	int zDelta;
	DWORD dwSensivity = GetHookInfo()->VJoySensivity;
	DWORD dwJoyStatus =	GetHookInfo()->VJoyStatus;

	if(!(dwJoyStatus & VJMOUSEWHEEL)) return FALSE;
	
	if(!dwSensivity) dwSensivity=100;
	//fwKeys = GET_KEYSTATE_WPARAM(wParam);
	zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if(zDelta >  4 * WHEEL_DELTA) zDelta =  4 * WHEEL_DELTA;
	if(zDelta < -4 * WHEEL_DELTA) zDelta = -4 * WHEEL_DELTA;
	if(zDelta > 0) dwSensivity = (dwSensivity * 110 *  zDelta) / (100 * WHEEL_DELTA);
	if(zDelta < 0) dwSensivity = (dwSensivity * 100 * -zDelta) / (110 * WHEEL_DELTA);
	if(dwSensivity < 32) dwSensivity = 32;
	if(dwSensivity > 250) dwSensivity = 250;
	GetHookInfo()->VJoySensivity = dwSensivity;
	return TRUE;
}

MMRESULT WINAPI extjoyGetPosEx(DWORD uJoyID, LPJOYINFOEX pji)
{
	JOYINFO jinfo;
	ApiName("joyGetPosEx");
	OutDebugIN("%s: joyid=%#x\n", ApiRef, uJoyID);

	if(!(dxw.dwFlags6 & VIRTUALJOYSTICK)) {
		return (*pjoyGetPosEx)(uJoyID, pji);
	}

	if(uJoyID != 0) return JOYERR_UNPLUGGED;

	vjGetJoy("joyGetPosEx", &jinfo);

	// set Joystick JOYINFOEX info structure
	memset(pji, 0, sizeof(JOYINFOEX));
	pji->dwSize = sizeof(JOYINFOEX);
	pji->dwFlags = 0;
	pji->dwXpos = jinfo.wXpos;
	pji->dwYpos = jinfo.wYpos;
	pji->dwButtons = jinfo.wButtons;
	pji->dwFlags = JOY_RETURNX|JOY_RETURNY|JOY_RETURNBUTTONS;

	return JOYERR_NOERROR;
}

MMRESULT WINAPI extjoyGetPos(DWORD uJoyID, LPJOYINFO pji)
{
	ApiName("joyGetPos");
	OutDebugIN("%s: joyid=%#x\n", ApiRef, uJoyID);

	if(!(dxw.dwFlags6 & VIRTUALJOYSTICK)) {
		return (*pjoyGetPos)(uJoyID, pji);
	}

	if(uJoyID != 0) return JOYERR_UNPLUGGED;
	vjGetJoy(ApiRef, pji);
	return JOYERR_NOERROR;
}

MMRESULT WINAPI extjoySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged)
{
	ApiName("joySetCapture");
	OutDebugIN("%s: hwnd=%#x joyid=%#x period=%d changed=%#x\n", 
		ApiRef, hwnd, uJoyID, uPeriod, fChanged);

	if(!(dxw.dwFlags6 & VIRTUALJOYSTICK)) {
		return (*pjoySetCapture)(hwnd, uJoyID, uPeriod, fChanged);
	}

	if(uJoyID != 0) return JOYERR_UNPLUGGED;
	return JOYERR_NOERROR;
}

MMRESULT WINAPI extjoyReleaseCapture(UINT uJoyID)
{
	ApiName("joyReleaseCapture");
	OutDebugIN("%s: joyid=%#x\n", ApiRef, uJoyID);

	if(!(dxw.dwFlags6 & VIRTUALJOYSTICK)) {
		return (*pjoyReleaseCapture)(uJoyID);
	}

	if(uJoyID != 0) return JOYERR_UNPLUGGED;
	return JOYERR_NOERROR;
}

BOOL WINAPI extmciGetErrorStringA(DWORD fdwError, LPCSTR lpszErrorText, UINT cchErrorText)
{
	BOOL ret;
	ret = (*pmciGetErrorStringA)(fdwError, lpszErrorText, cchErrorText);
	OutTraceSND("mciGetErrorStringA: ret=%#x err=%d text=(%d)\"%.*s\"\n", ret, fdwError, cchErrorText, cchErrorText, lpszErrorText);
	return ret;
}

#ifndef DXW_NOTRACES
static char *sSupport(DWORD c)
{
	static char eb[128];
	unsigned int l;
	strcpy(eb,"WAVECAPS_");
	if (c & WAVECAPS_PITCH) strcat(eb, "PITCH+");
	if (c & WAVECAPS_PLAYBACKRATE) strcat(eb, "PLAYBACKRATE+");
	if (c & WAVECAPS_VOLUME) strcat(eb, "VOLUME+");
	if (c & WAVECAPS_LRVOLUME) strcat(eb, "LRVOLUME+");
	if (c & WAVECAPS_SYNC) strcat(eb, "SYNC+");
	if (c & WAVECAPS_SAMPLEACCURATE) strcat(eb, "SAMPLEACCURATE+");
	l=strlen(eb);
	if (l>strlen("WAVECAPS_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}
#endif

MMRESULT WINAPI extwaveOutGetDevCapsA(DWORD uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc)
{
	MMRESULT ret;
	ApiName("waveOutGetDevCapsA");
	OutTraceSND("%s: dev=%#x siz=%d\n", ApiRef, uDeviceID, cbwoc);

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}

	if((dxw.dwFlags11 & FIXDEFAULTMCIID) && (uDeviceID == 0x0000FFFF)) {
		uDeviceID = WAVE_MAPPER;
		OutTraceSND("%s: fixing default dev=%#x\n", ApiRef, uDeviceID);
	}

	if(dxw.dwFlags14 & WAVEOUTUSEPREFDEV) {
		uDeviceID = WAVE_MAPPER;
		OutTraceSND("%s: using default dev=%#x\n", ApiRef, uDeviceID);
	}

	ret = (*pwaveOutGetDevCapsA)(uDeviceID, pwoc, cbwoc);
	if(ret){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	} 

	OutTraceSND("> Mid = %u\n", pwoc->wMid); 
	OutTraceSND("> Pid = %u\n", pwoc->wPid); 
	OutTraceSND("> DriverVersion = %u.%u\n", pwoc->vDriverVersion/256, pwoc->vDriverVersion%256); 
	OutTraceSND("> Pname = %s\n", pwoc->szPname); 
	OutTraceSND("> Formats = %#x\n", pwoc->dwFormats); 
	OutTraceSND("> Channels = %u\n", pwoc->wChannels); 
	OutTraceSND("> Reserved1 = %#x\n", pwoc->wReserved1); 
	OutTraceSND("> Support = %#x(%s)\n", pwoc->dwSupport, sSupport(pwoc->dwSupport)); 
	return ret;
}

MMRESULT WINAPI extwaveOutGetDevCapsW(DWORD uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc)
{
	MMRESULT ret;
	ApiName("waveOutGetDevCapsW");
	OutTraceSND("%s: dev=%#x siz=%d\n", ApiRef, uDeviceID, cbwoc);

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}

	if((dxw.dwFlags11 & FIXDEFAULTMCIID) && (uDeviceID == 0x0000FFFF)) {
		uDeviceID = WAVE_MAPPER;
		OutTraceSND("%s: fixing default dev=%#x\n", ApiRef, uDeviceID);
	}

	if(dxw.dwFlags14 & WAVEOUTUSEPREFDEV) {
		uDeviceID = WAVE_MAPPER;
		OutTraceSND("%s: using default dev=%#x\n", ApiRef, uDeviceID);
	}

	ret = (*pwaveOutGetDevCapsW)(uDeviceID, pwoc, cbwoc);
	if(ret){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	} 

	OutTraceSND("> Mid = %u\n", pwoc->wMid); 
	OutTraceSND("> Pid = %u\n", pwoc->wPid); 
	OutTraceSND("> DriverVersion = %u.%u\n", pwoc->vDriverVersion/256, pwoc->vDriverVersion%256); 
	OutTraceSND("> Pname = %ls\n", pwoc->szPname); 
	OutTraceSND("> Formats = %#x\n", pwoc->dwFormats); 
	OutTraceSND("> Channels = %u\n", pwoc->wChannels); 
	OutTraceSND("> Reserved1 = %#x\n", pwoc->wReserved1); 
	OutTraceSND("> Support = %#x(%s)\n", pwoc->dwSupport, sSupport(pwoc->dwSupport)); 
	return ret;
}

/*
  Catch the 16 bit applications, WOW calls this routine for 16 bit apps.
*/

#define WODM_GETDEVCAPS         4
#define WODM_OPEN               5

DWORD WINAPI extwod32Message(UINT uDeviceID, UINT uMessage,  DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	MMRESULT ret;
	ApiName("wod32Message");
	OutTraceSND("%s: dev=%#x msg=%#x inst=%#x param1=%#x param2=%#x\n", 
		ApiRef, uDeviceID, uMessage, dwInstance, dwParam1, dwParam2);

	if(dxw.dwFlags14 & WAVEOUTUSEPREFDEV) {
		// Change device 0 to WAVE_MAPPER for Open and GetDevCaps
		if (uDeviceID == 0) {
			if (uMessage == WODM_OPEN ||
				uMessage == WODM_GETDEVCAPS) {
				uDeviceID = WAVE_MAPPER; // Force device to WAVE_MAPPER
				OutTraceSND("%s: using default dev=%#x\n", ApiRef, uDeviceID);
			}
		}
	}

    ret = (*pwod32Message)(uDeviceID, uMessage, dwInstance, dwParam1, dwParam2);
    return ret;
}

#ifndef DXW_NOTRACES
//#define MOD_MIDIPORT    1  /* output port */
//#define MOD_SYNTH       2  /* generic internal synth */
//#define MOD_SQSYNTH     3  /* square wave internal synth */
//#define MOD_FMSYNTH     4  /* FM internal synth */
//#define MOD_MAPPER      5  /* MIDI mapper */
//#define MOD_WAVETABLE   6  /* hardware wavetable synth */
//#define MOD_SWSYNTH     7  /* software synth */

static char *sMidiTechnology(DWORD s)
{
	char *captions[]={"MIDIPORT", "SYNTH", "SQSYNTH", "FMSYNTH", "MAPPER", "WAVETABLE", "SWSYNTH"};

	if((s>0) && (s<8)) return captions[s-1];
	return "unknown";
}
#endif

MMRESULT WINAPI extmidiOutGetDevCapsA(DWORD uDeviceID, LPMIDIOUTCAPS2A pmoc, UINT cbmoc)
{
	UINT ret;
	ApiName("midiOutGetDevCapsA");
	OutTraceSND("%s: dev=%#x siz=%d(%s)\n", 
		ApiRef, uDeviceID, cbmoc, 
		cbmoc == sizeof(MIDIOUTCAPSA) ? "v1" : (cbmoc == sizeof(MIDIOUTCAPS2A) ? "v2" : "??"));

	if(dxw.dwFlags14 & MIDIOUTBYPASS) {
		// .. duly copied from VirtualMidiSynth defaults ...
		pmoc->wMid = 1; // 2
		pmoc->wPid = 1; // 2
		pmoc->vDriverVersion = 0x00000500; // UINT (4 ?)
		strcpy(pmoc->szPname, "Microsoft MIDI Mapper"); // 32
		pmoc->wTechnology = MOD_MAPPER;
		pmoc->wVoices = 0; // 2
		pmoc->wNotes = 0; // 2
		pmoc->wChannelMask = 0xFFFF; // 2
		pmoc->dwSupport = WAVECAPS_PITCH | WAVECAPS_LRVOLUME; // 2

		if(cbmoc == sizeof(MIDIOUTCAPS2A)){
			//pmoc->ManufacturerGuid = 0;
		 //   pmoc->ProductGuid = 0;
		 //   pmoc->NameGuid = 0;
		}

		return MMSYSERR_NOERROR;
	}

	if((dxw.dwFlags11 & FIXDEFAULTMCIID) && (uDeviceID == 0x0000FFFF)) {
		uDeviceID = 0xFFFFFFFF;
		OutTraceSND("%s: fixing default dev=%#x\n", ApiRef, uDeviceID);
	}

	ret = (*pmidiOutGetDevCapsA)(uDeviceID, pmoc, cbmoc);
	if(ret){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	} 

	OutTraceSND("> Mid = %u\n", pmoc->wMid); 
	OutTraceSND("> Pid = %u\n", pmoc->wPid); 
	OutTraceSND("> DriverVersion = %u.%u\n", pmoc->vDriverVersion/256, pmoc->vDriverVersion%256); 
	OutTraceSND("> Pname = %s\n", pmoc->szPname); 
	OutTraceSND("> Technology = %#x(%s)\n", pmoc->wTechnology, sMidiTechnology(pmoc->wTechnology)); 
	OutTraceSND("> Voices = %d\n", pmoc->wVoices); 
	OutTraceSND("> Notes = %d\n", pmoc->wNotes); 
	OutTraceSND("> ChannelMask = %#x\n", pmoc->wChannelMask); 
	OutTraceSND("> Support = %#x(%s)\n", pmoc->dwSupport, sSupport(pmoc->dwSupport)); 

	return ret;
}

MMRESULT WINAPI extmidiOutGetDevCapsW(DWORD uDeviceID, LPMIDIOUTCAPS2W pmoc, UINT cbmoc)
{
	UINT ret;
	ApiName("midiOutGetDevCapsW");
	OutTraceSND("%s: dev=%#x siz=%d(%s)\n", 
		ApiRef, uDeviceID, cbmoc, 
		cbmoc == sizeof(MIDIOUTCAPSA) ? "v1" : (cbmoc == sizeof(MIDIOUTCAPS2A) ? "v2" : "??"));

	if(dxw.dwFlags14 & MIDIOUTBYPASS) {
		// .. duly copied from VirtualMidiSynth defaults ...
		pmoc->wMid = 1; // 2
		pmoc->wPid = 1; // 2
		pmoc->vDriverVersion = 0x00000500; // UINT (4 ?)
		wcscpy(pmoc->szPname, L"Microsoft MIDI Mapper"); // 32
		pmoc->wTechnology = MOD_MAPPER;
		pmoc->wVoices = 0; // 2
		pmoc->wNotes = 0; // 2
		pmoc->wChannelMask = 0xFFFF; // 2
		pmoc->dwSupport = WAVECAPS_PITCH | WAVECAPS_LRVOLUME; // 2

		if(cbmoc == sizeof(MIDIOUTCAPS2W)){
			//pmoc->ManufacturerGuid = 0;
		 //   pmoc->ProductGuid = 0;
		 //   pmoc->NameGuid = 0;
		}

		return MMSYSERR_NOERROR;
	}

	if((dxw.dwFlags11 & FIXDEFAULTMCIID) && (uDeviceID == 0x0000FFFF)) {
		uDeviceID = 0xFFFFFFFF;
		OutTraceSND("%s: fixing default dev=%#x\n", ApiRef, uDeviceID);
	}

	ret = (*pmidiOutGetDevCapsW)(uDeviceID, pmoc, cbmoc);
	if(ret){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	} 

	OutTraceSND("> Mid = %u\n", pmoc->wMid); 
	OutTraceSND("> Pid = %u\n", pmoc->wPid); 
	OutTraceSND("> DriverVersion = %u.%u\n", pmoc->vDriverVersion/256, pmoc->vDriverVersion%256); 
	OutTraceSND("> Pname = %ls\n", pmoc->szPname); 
	OutTraceSND("> Technology = %#x(%s)\n", pmoc->wTechnology, sMidiTechnology(pmoc->wTechnology)); 
	OutTraceSND("> Voices = %d\n", pmoc->wVoices); 
	OutTraceSND("> Notes = %d\n", pmoc->wNotes); 
	OutTraceSND("> ChannelMask = %#x\n", pmoc->wChannelMask); 
	OutTraceSND("> Support = %#x(%s)\n", pmoc->dwSupport, sSupport(pmoc->dwSupport)); 

	return ret;
}

#ifndef DXW_NOTRACES
static char *swoCallback(DWORD c)
{
	char *s;
	switch(c & CALLBACK_TYPEMASK){
		case CALLBACK_NULL: s="CALLBACK_NULL"; break;
		case CALLBACK_WINDOW: s="CALLBACK_WINDOW"; break;
		case CALLBACK_TASK: s="CALLBACK_TASK"; break;
		case CALLBACK_FUNCTION: s="CALLBACK_FUNCTION"; break;
		case CALLBACK_EVENT: s="CALLBACK_EVENT"; break;
		default: s="???"; break;
	}
	return s;
}
#endif

MMRESULT WINAPI extmidiOutOpen(LPHMIDIOUT phmo, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{
	MMRESULT ret;
	ApiName("midiOutOpen");

#ifndef DXW_NOTRACES
	if(uDeviceID == MIDI_MAPPER) {
		OutTraceSND("%s: devid=MIDI_MAPPER callback=%#x inst=%#x flags=%#x(%s)\n", ApiRef, dwCallback, dwInstance, fdwOpen, swoCallback(fdwOpen));
	} else {
		OutTraceSND("%s: devid=%#x callback=%#x inst=%#x flags=%#x(%s)\n", ApiRef, uDeviceID, dwCallback, dwInstance, fdwOpen, swoCallback(fdwOpen));
	}
#endif // DXW_NOTRACES

	if(dxw.dwFlags14 & MIDIOUTBYPASS){
		*phmo = (HMIDIOUT)0xDEADBEEF;
		OutTraceSND("%s: BYPASS hmo=0xDEADBEEF\n", ApiRef);
		return 0;
	}

	if((dxw.dwFlags11 & FIXDEFAULTMCIID) && (uDeviceID == 0x0000FFFF)) {
		uDeviceID = 0xFFFFFFFF;
		OutTraceSND("%s: fixing default dev=%#x\n", ApiRef, uDeviceID);
	}

	ret = (*pmidiOutOpen)(phmo, uDeviceID, dwCallback, dwInstance, fdwOpen);

	if(ret == MCIERR_DEVICE_LOCKED){
		for (int retry=0; retry<4; retry++){
			(*pSleep)(4000);
			ret = (*pmidiOutOpen)(phmo, uDeviceID, dwCallback, dwInstance, fdwOpen);
			if(ret == MMSYSERR_NOERROR) break;
		}
		OutTraceSND("%s: LOCKED ret=%d @%d\n", ApiRef, ret, __LINE__);
	}

	if(ret){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

	OutTraceSND("%s: hmo=%#x\n", ApiRef, *phmo);
	return ret;
}

UINT WINAPI extmidiOutGetNumDevs()
{
	UINT ret;
	ApiName("midiOutGetNumDevs");

	//if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;
	if(dxw.dwFlags14 & MIDIOUTBYPASS) {
		OutTraceSND("%s: MIDIOUTBYPASS numdevs=1\n", ApiRef);
		return 1;
	}

	ret = (*pmidiOutGetNumDevs)();
	OutTraceSND("%s: numdevs=%d\n", ApiRef, ret);
	return ret;
}

DWORD WINAPI waveoutclose_wait(LPVOID param)
{
	HWAVEOUT hwo = *(HWAVEOUT *)param;
	HeapFree(GetProcessHeap(), 0, param);
	(*pwaveOutClose)(hwo);
	return 0;
}

MMRESULT WINAPI extwaveOutClose(HWAVEOUT hwo)
{
	MMRESULT ret;
	ApiName("waveOutClose");
	OutTraceSND("%s: hwo=%#x\n", ApiRef, hwo);

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}

#ifdef ASYNCOPENONLY
	if(dxw.dwFlags19 & ASYNCWAVEOUTCLOSE){
		HWAVEOUT *phwo = (HWAVEOUT *)HeapAlloc(GetProcessHeap(), 0, sizeof(HWAVEOUT *));
		*phwo = hwo;
		CreateThread(NULL, 0, waveoutclose_wait, phwo, 0, NULL);
		return MMSYSERR_NOERROR;
	}
#endif // ASYNCOPENONLY

	ret = (*pwaveOutClose)(hwo);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));

	return ret;
}

MMRESULT WINAPI extmidiOutClose(HMIDIOUT hmo)
{
	MMRESULT ret;
	ApiName("midiOutClose");
	OutTraceSND("%s: hmo=%#x\n", ApiRef, hmo);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiOutClose)(hmo);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

extern void DumpMidiMessage(LPBYTE, int);
static int MidiInstrument = -1;

MMRESULT WINAPI extmidiOutLongMsg(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh)
{
	MMRESULT ret;
	ApiName("midiOutLongMsg");

	if(IsTraceSND){
		OutTrace("%s: hmo=%#x cbmh=%d\n", ApiRef, hmo, cbmh);
		OutTrace("> lpData=%#x\n", pmh->lpData);
		OutTrace("> dwBufferLength=%d\n", pmh->dwBufferLength);
		OutTrace("> dwBytesRecorded=%d\n", pmh->dwBytesRecorded);
		OutTrace("> dwUser=%#x\n", pmh->dwUser);
		OutTrace("> dwFlags=%#x\n", pmh->dwFlags);
		if(IsTraceHex) HexTrace((LPBYTE)pmh->lpData, pmh->dwBufferLength);
		if(IsDebugSND) DumpMidiMessage((LPBYTE)pmh->lpData, pmh->dwBufferLength);
	}

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	if ((dxw.dwFlags9 & LOCKVOLUME) && pmh && pmh->lpData) {
		OutTraceSND("> midi volume LOCKED vol=%d%%\n", dxw.MidiVolume);
		bMidiMessageHooked = TRUE;
		float midiVol = (float)dxw.MidiVolume / (float)100;
		for (DWORD i = 0; i < pmh->dwBufferLength; ) {
			BYTE command = pmh->lpData[i] & 0xF0;
			int bytecount;
			switch (command) {
				case 0x80: bytecount = 2; break;  // Note Off
				case 0x90: bytecount = 2; break;  // Note On
				case 0xA0: bytecount = 2; break;  // Aftertouch
				case 0xB0: 
					bytecount = 2; 
					if(pmh->lpData[i+1] == 0x07){
						OutTrace("%s: midi volume level %d\n", ApiRef, pmh->lpData[i+2]); 
						pmh->lpData[i+2] *= midiVol;
					}
					break;  // Continuous Controller
				case 0xC0: bytecount = 1; 
					if(dxw.dwFlags15 & MIDISETINSTRUMENT) {
						if(MidiInstrument == -1) {
							char InitPath[MAX_PATH];
							sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
							MidiInstrument = (*pGetPrivateProfileIntA)("sound", "instrument", 0, InitPath);
						}
						pmh->lpData[i+1] = MidiInstrument;
					}
					break;  // Patch Change
				case 0xD0: bytecount = 1; break;  // Channel Pressure
				case 0xE0: bytecount = 2; break;  // Pitch Bend
				case 0xF0:
				default:
					bytecount = pmh->dwBufferLength; // to break the loop
					break;
			}
			i += (bytecount + 1);
		}
	}

	ret = (*pmidiOutLongMsg)(hmo, pmh, cbmh);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiOutShortMsg(HMIDIOUT hmo, DWORD dwMsg)
{
	MMRESULT ret;
	ApiName("midiOutShortMsg");
	OutTraceSND("%s: hmo=%#x msg=%#x\n", ApiRef, hmo, dwMsg);
	if(IsDebugSND) DumpMidiMessage((LPBYTE)&dwMsg, 0);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	if ((dxw.dwFlags9 & LOCKVOLUME) && 
		((dwMsg & 0x0000FFF0) == 0x000007B0)){
		bMidiMessageHooked = TRUE;
		OutTraceSND("> midi volume LOCKED vol=%d%%\n", dxw.MidiVolume);
		DWORD volume = ((DWORD)dxw.MidiVolume * 127) / 100;
		if(volume >= 128) volume = 127;

		volume <<= 16;
		OutTrace("vol=%#x\n", volume);
		dwMsg &= 0xFF00FFFF;
		dwMsg |= volume;
	}

	if ((dxw.dwFlags15 & MIDISETINSTRUMENT) &&
		((dwMsg & 0x000000F0) == 0x000000C0)){
		if(MidiInstrument == -1) {
			char InitPath[MAX_PATH];
			sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
			MidiInstrument = (*pGetPrivateProfileIntA)("sound", "instrument", 0, InitPath);
		}
		OutTrace("instrument=%#x\n", MidiInstrument);
		dwMsg &= 0xFFFF00FF;
		dwMsg |= (MidiInstrument << 8);
	}

	ret = (*pmidiOutShortMsg)(hmo, dwMsg);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiOutPrepareHeader(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh)
{
	MMRESULT ret;
	ApiName("midiOutPrepareHeader");
	OutTraceSND("%s: hmo=%#x pmh=%#x cbmh=%d\n", ApiRef, hmo, pmh, cbmh);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiOutPrepareHeader)(hmo, pmh, cbmh);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiStreamOut(HMIDISTRM hms, LPMIDIHDR pmh, UINT cbmh)
{
	MMRESULT ret;
	ApiName("midiStreamOut");
	OutTraceSND("%s: hms=%#x pmh=%#x cbmh=%d\n", ApiRef, pmh, cbmh);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	if ((dxw.dwFlags15 & EMULATEVOLUME) && pmh && pmh->lpData) {
		float midiVol = (float)dxw.MidiVolume / (float)100;
		for (int i = 0, j = pmh->dwBytesRecorded; i < j; i += sizeof(DWORD)*3) {
			MIDIEVENT *pe = (MIDIEVENT *)(pmh->lpData + i);
			if (pe->dwEvent & MEVT_F_LONG) {
				/* DWORD padding is required */
				i += (MEVT_EVENTPARM(pe->dwEvent) + 3) & 0xFFFFFC;
			} else if (MEVT_EVENTTYPE(pe->dwEvent) == MEVT_SHORTMSG) {
				char *pv = (char *)&pe->dwEvent; 
				if (!(pe->dwEvent&0x80)) {
					/* running status */
					pv[1] *= midiVol;
				} else if ((pe->dwEvent&0xF0) < 0xB0) {
					/* new voice status */
					pv[2] *= midiVol;
				}
			}
		}
	}

	ret = (*pmidiStreamOut)(hms, pmh, cbmh);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	if((ret == MIDIERR_UNPREPARED) && (dxw.dwFlags13 & MIDIAUTOREPAIR) && pmidiOutPrepareHeader){
		ret=(*pmidiOutPrepareHeader)((HMIDIOUT)hms, pmh, pmh->dwBufferLength);
		OutTraceDW("%s: MIDIAUTOREPAIR midiOutPrepareHeader res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		ret = (*pmidiStreamOut)(hms, pmh, cbmh);
		_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	}
	return ret;
}

MMRESULT WINAPI extmidiStreamPause(HMIDISTRM hms)
{
	MMRESULT ret;
	ApiName("midiStreamPause");
	OutTraceSND("%s: hms=%#x\n", ApiRef, hms);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiStreamPause)(hms);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiOutGetErrorTextA(MMRESULT mmError, LPSTR pszText, UINT cchText)
{
	MMRESULT ret;
	ApiName("midiOutGetErrorTextA");
	OutTraceSND("%s: mmerr=%#x cch=%d\n", ApiRef, mmError, cchText);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiOutGetErrorTextA)(mmError, pszText, cchText);
	if(ret){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

	OutTraceSND("%s: text=\"%s\"\n", ApiRef, pszText);
	return ret;
}

MMRESULT WINAPI extmidiStreamClose(HMIDISTRM hms)
{
	MMRESULT ret;
	ApiName("midiStreamClose");
	OutTraceSND("%s: hms=%#x\n", ApiRef, hms);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiStreamClose)(hms);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiStreamOpen(LPHMIDISTRM phms, LPUINT puDeviceID, DWORD cMidi, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{
	MMRESULT ret;
	ApiName("midiStreamOpen");
	char *fdwOpenStrings[] = {"NULL", "WINDOW", "TASK", "FUNCTION", "", "EVENT", "", ""};
	OutTraceSND("%s: phms=%#x devid=%#x cmidi=%#x cback=%#x instance=%#x fdwopen=%#x(%s)\n", 
		ApiRef, phms, 
		puDeviceID ? *puDeviceID : NULL,
		cMidi,
		dwCallback,
		dwInstance,
		fdwOpen, 
		fdwOpenStrings[(fdwOpen & CALLBACK_TYPEMASK) >> 16]);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiStreamOpen)(phms, puDeviceID, cMidi, dwCallback, dwInstance, fdwOpen);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiStreamStop(HMIDISTRM hms)
{
	MMRESULT ret;
	ApiName("midiStreamStop");
	OutTraceSND("%s: hms=%#x\n", ApiRef, hms);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiStreamStop)(hms);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiStreamRestart(HMIDISTRM hms)
{
	MMRESULT ret;
	ApiName("midiStreamRestart");
	OutTraceSND("%s: hms=%#x\n", ApiRef, hms);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiStreamRestart)(hms);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiOutUnprepareHeader(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh)
{
	MMRESULT ret;
	ApiName("midiOutUnprepareHeader");
	OutTraceSND("%s: hmo=%#x pmh=%#x cbmh=%d\n", ApiRef, pmh, cbmh);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiOutUnprepareHeader)(hmo, pmh, cbmh);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiStreamProperty(HMIDISTRM hms, LPBYTE lppropdata, DWORD dwProperty)
{
	MMRESULT ret;
	ApiName("midiStreamProperty");
	OutTraceSND("%s: hms=%#x prop=%#x(%s%s+%s%s)\n", ApiRef, hms, 
		dwProperty, 
		dwProperty & MIDIPROP_GET ? "GET" : "",
		dwProperty & MIDIPROP_SET ? "SET" : "",
		dwProperty & MIDIPROP_TIMEDIV ? "TIMEDIV" : "",
		dwProperty & MIDIPROP_TEMPO ? "TEMPO" : ""
		);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiStreamProperty)(hms, lppropdata, dwProperty);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extmidiStreamPosition(HMIDISTRM hms, LPMMTIME lpmmt, UINT cbmmt)
{
	MMRESULT ret;
	ApiName("midiStreamPosition");
	OutTraceSND("%s: hms=%#x mmt.wTime=%d bbmmt=%d\n", ApiRef, hms, lpmmt->wType, cbmmt);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) {
		lpmmt->u.ticks = 0;
		return MMSYSERR_NOERROR;
	}

	ret = (*pmidiStreamPosition)(hms, lpmmt, cbmmt);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	OutTraceSND("%s: hms=%#x mmt.wTime=%d mmt.u=%#x\n", ApiRef, hms, lpmmt->wType, lpmmt->u.ticks);
	return ret;
}

MMRESULT WINAPI extmidiConnect(HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved)
{
	MMRESULT ret;
	ApiName("midiConnect");
	OutTraceSND("%s: hmi=%#x hmo=%#x reserved=%#x\n", ApiRef, hmi, hmo, pReserved); 

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	ret = (*pmidiConnect)(hmi, hmo, pReserved);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

#ifndef WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE
#define WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE 0x10
#endif 


///* flags for dwFlags field of WAVEHDR */
//#define WHDR_DONE       0x00000001  /* done bit */
//#define WHDR_PREPARED   0x00000002  /* set if this header has been prepared */
//#define WHDR_BEGINLOOP  0x00000004  /* loop start block */
//#define WHDR_ENDLOOP    0x00000008  /* loop end block */
//#define WHDR_INQUEUE    0x00000010  /* reserved for driver */

void dumpHWaveHdr(LPWAVEHDR lpwh)
{
	OutTrace("> lpData: %#x\n", lpwh->lpData);
	OutTrace("> dwBufferLength: %d\n", lpwh->dwBufferLength);
	OutTrace("> dwBytesRecorded: %d\n", lpwh->dwBytesRecorded);
	OutTrace("> dwUser: %#x\n", lpwh->dwUser);
	OutTrace("> dwFlags: %#x( %s%s%s%s%s)\n", lpwh->dwFlags,
		lpwh->dwFlags & WHDR_DONE ? "WHDR_DONE " : "",
		lpwh->dwFlags & WHDR_PREPARED ? "WHDR_PREPARED " : "",
		lpwh->dwFlags & WHDR_BEGINLOOP ? "WHDR_BEGINLOOP " : "",
		lpwh->dwFlags & WHDR_ENDLOOP ? "WHDR_ENDLOOP " : "",
		lpwh->dwFlags & WHDR_INQUEUE ? "WHDR_INQUEUE " : ""
		);
	OutTrace("> dwLoops: %#x\n", lpwh->dwLoops);
	OutTrace("> lpNext: %#x\n", lpwh->lpNext);
	OutTrace("> reserved: %#x\n", lpwh->reserved);
}

void CALLBACK dxwWaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	OutTrace("waveOutProc: hwo=%#x msg=%#x(%s) instance=%#x param1=%#x param2=%#x\n",
		hwo, 
		// http://msdn.microsoft.com/en-us/library/dd743869(VS.85).aspx
		uMsg, uMsg == WOM_CLOSE ? "WOM_CLOSE" : (uMsg == WOM_OPEN ? "WOM_OPEN" : "WOM_DONE"),
		dwInstance, dwParam1, dwParam2);
#ifndef DXW_NOTRACES
	if(IsDebugSND){
		if(uMsg == WOM_DONE) dumpHWaveHdr((LPWAVEHDR)dwParam1);
	}
#endif // DXW_NOTRACES
}

typedef void (CALLBACK *waveOutProc_Type)(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
waveOutProc_Type pwaveOutProc;

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	OutTrace("waveOutProc: hwo=%#x msg=%d(%s) instance=%#x p1=%#x p2=%#x\n",
		hwo, uMsg, 
		uMsg == WOM_OPEN ? "WOM_OPEN" : (uMsg == WOM_CLOSE ? "WOM_CLOSE" : (uMsg == WOM_DONE ? "WOM_DONE" : "???")),
		dwInstance, dwParam1, dwParam2);
	(*pwaveOutProc)(hwo, uMsg, dwInstance, dwParam1, dwParam2);
	OutTrace("waveOutProc: end\n");
}

typedef struct {
	HWAVEOUT hwo;
	UINT uMsg;
	DWORD_PTR dwInstance;
	DWORD_PTR arg1;
	DWORD_PTR arg2;
} waveoutargs;

DWORD WINAPI waveoutasync_wait(LPVOID p)
{
	waveoutargs *args = (waveoutargs *)p;
	(*pwaveOutProc)(args->hwo, args->uMsg, args->dwInstance, args->arg1, args->arg2);
	free(args);
	return 0;
}

void CALLBACK waveOutProcAsync(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	OutDebugSND("waveOutProc: hwo=%#x msg=%d(%s) instance=%#x p1=%#x p2=%#x\n",
		hwo, uMsg, 
		uMsg == WOM_OPEN ? "WOM_OPEN" : (uMsg == WOM_CLOSE ? "WOM_CLOSE" : (uMsg == WOM_DONE ? "WOM_DONE" : "???")),
		dwInstance, dwParam1, dwParam2);
	if(uMsg == WOM_OPEN){
		waveoutargs *args = (waveoutargs *)malloc(sizeof(waveoutargs));
		args->hwo  = hwo;
		args->uMsg = uMsg;
		args->dwInstance = dwInstance;
		args->arg1 = dwParam1;
		args->arg2 = dwParam2;
		CreateThread(NULL, 0, waveoutasync_wait, (LPVOID)args, 0, NULL);
		Sleep(10);
	}
	else {
		(*pwaveOutProc)(hwo, uMsg, dwInstance, dwParam1, dwParam2);
	}
}

DWORD WINAPI waveout_thread(LPVOID param)
{
	MSG msg;
	BOOL ret;
	void (WINAPI *waveoutproc)(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
	waveoutproc = (void (WINAPI *)(HWAVEOUT, UINT, DWORD, DWORD, DWORD))param;
	while(ret = GetMessageA(&msg, NULL, 0, 0)){
	   if(ret == -1) return -1;
	   switch(msg.message){
		   case MM_WOM_OPEN:
		   case MM_WOM_CLOSE:
		   case MM_WOM_DONE:
			   waveoutproc((HWAVEOUT)msg.wParam, msg.message, 0, msg.lParam, 0);
			   if(msg.message == MM_WOM_CLOSE) return 0;
			   break;
		   case WM_USER+10:
			   return 0;
	   }
	}
	return 0;
}

MMRESULT WINAPI extwaveOutOpen(LPHWAVEOUT phwo, UINT_PTR uDeviceID, LPWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwCallbackInstance, DWORD fdwOpen)
{
	MMRESULT ret;
	DWORD threadid = 0;
	ApiName("waveOutOpen");

	OutTraceSND("%s: callback=%#x instance=%#x flags=%#x(%s)\n", 
		ApiRef, dwCallback, dwCallbackInstance, fdwOpen, swoCallback(fdwOpen));
	if(uDeviceID == WAVE_MAPPER) {
		OutTraceSND("%s: devid=WAVE_MAPPER\n", ApiRef);
	} else {
		OutTraceSND("%s: devid=%#x\n", ApiRef, uDeviceID);
	}

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}

	if(BYPASSWAVEOUT) {
		if(phwo) *phwo = (HWAVEOUT)0xDEADBEEF;
		return 0;
	}

	if((dxw.dwFlags19 & HOOKLEGACYWAVE) && ((fdwOpen & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION)) {
		OutTraceT("%s: routing callback to connectorCallback\n", ApiRef);
		_winmm_naked_callback_data *c = (_winmm_naked_callback_data *)malloc(sizeof(_winmm_naked_callback_data));
		c->original_callback = (PWINMM_TIMESETEVENT_CALLBACK)dwCallback;
		c->original_dwUser = dwCallbackInstance;
		dwCallbackInstance = (DWORD_PTR)c; 
		dwCallback = (DWORD_PTR)_winmm_naked_callback;
	}

	// fixes for the callback function
	if((fdwOpen & (CALLBACK_TYPEMASK | WAVE_FORMAT_QUERY)) == CALLBACK_FUNCTION){
		if(dxw.dwFlags19 & ASYNCWAVEOUTOPEN){ // fixes @#@ "CandyLand Adventure"
#ifdef ASYNCOPENONLY
			pwaveOutProc = (waveOutProc_Type)dwCallback;
			dwCallback = (DWORD_PTR)waveOutProcAsync;
#else
			CloseHandle(CreateThread(NULL, 0, waveout_thread, (LPVOID)dwCallback, 0, &threadid));
			fdwOpen = (fdwOpen & ~CALLBACK_TYPEMASK) | CALLBACK_THREAD;
			dwCallback = threadid;
#endif
		}
		else if(dxw.dwFlags14 & MW2WAVEOUTFIX){ // discouraged !!!
			fdwOpen = CALLBACK_NULL;
			dwCallback = (DWORD_PTR)dxwWaveOutProc;
			OutTraceSND("%s: MW2 fix\n", ApiRef);
		}
		else if(IsDebugSND){
			pwaveOutProc = (waveOutProc_Type)dwCallback;
			dwCallback = (DWORD_PTR)waveOutProc;
		}
	}

	OutTraceSND("%s: waveformat=(Tag=%#x Channels=%d AvgBytesPerSec=%d BlockAlign=%d BitsXSample=%d SamplesXSec=%d Size=%d)\n", 
		ApiRef,
		pwfx->wFormatTag,
		pwfx->nChannels,
		pwfx->nAvgBytesPerSec,
		pwfx->nBlockAlign,
		pwfx->wBitsPerSample,
		pwfx->nSamplesPerSec,
		pwfx->cbSize);

	// for later use, in midi/wave volume emulation
	/* FIXME: waveOutOpen can be called multiple times with different sample bits for sound mixing */
	/* However, it is almost always to be 16-bits; extremely rarely to be 8-bits or of a mixed-up */
	if (pwfx) waveBits = pwfx->wBitsPerSample;

	if((dxw.dwFlags11 & FIXDEFAULTMCIID) && (uDeviceID == 0x0000FFFF)) {
		uDeviceID = WAVE_MAPPER;
		OutTraceSND("%s: fixing default dev=%#x\n", ApiRef, uDeviceID);
	}

	if(dxw.dwFlags14 & WAVEOUTUSEPREFDEV) {
		uDeviceID = WAVE_MAPPER; 
		OutTraceSND("%s: using default dev=%#x\n", ApiRef, uDeviceID);
	}

	ret = (*pwaveOutOpen)(phwo, uDeviceID, pwfx, dwCallback, dwCallbackInstance, fdwOpen);

	if((ret == WAVERR_BADFORMAT) && (pwfx->nChannels > 1)){
		OutTraceSND("%s: bad format, try recovey with MONO channels\n", ApiRef);

		//pwfx->cbSize = 0;
		//pwfx->wFormatTag = WAVE_FORMAT_PCM;
		pwfx->nChannels = 1; // mono
		//pwfx->wBitsPerSample = 16; // 16 bit
		//pwfx->nSamplesPerSec = 8000; // 8 kHz
		//pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		//pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		ret = (*pwaveOutOpen)(phwo, uDeviceID, pwfx, dwCallback, dwCallbackInstance, fdwOpen);
		OutTraceSND("%s: MONO ret=%d @%d\n", ApiRef, ret, __LINE__);
	}

	if((ret == WAVERR_BADFORMAT) && (pwfx->wBitsPerSample == 4)){
		// v2.05.87 fix: fixes The Way Things Work Pinball Science (1998)
		// beware! the fix is not fully correct or working, but the program doesn't really use 4bit sound
		// so the fix helps only to skip an error that blocked the program. The program aoudio is then
		// reinitialized in a more decent 8 bit samples mode.

		pwfx->wBitsPerSample = 8; // 8 bit
		pwfx->wFormatTag = WAVE_FORMAT_PCM; // 16 bit 
		//pwfx->nSamplesPerSec = 8000; // 8 kHz
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		ret = (*pwaveOutOpen)(phwo, uDeviceID, pwfx, dwCallback, dwCallbackInstance, fdwOpen);
		OutTraceSND("%s: 4->8 bit samples ret=%d @%d\n", ApiRef, ret, __LINE__);
	}

	if(ret == MCIERR_DEVICE_LOCKED){
		for (int retry=0; retry<4; retry++){
			(*pSleep)(4000);
			ret = (*pwaveOutOpen)(phwo, uDeviceID, pwfx, dwCallback, dwCallbackInstance, fdwOpen);
			if(ret == MMSYSERR_NOERROR) break;
		}
		OutTraceSND("%s: LOCKED ret=%d @%d\n", ApiRef, ret, __LINE__);
	}

	//// v2.05.76: "MechWarrior 2" fix -- does NOT fix!
	//if(ret == MMSYSERR_BADDEVICEID){
	//	ret = (*pwaveOutOpen)(phwo, WAVE_MAPPER, pwfx, dwCallback, dwCallbackInstance, fdwOpen);
	//	OutTraceSND("%s: WAVE_MAPPER ret=%d @%d\n", ApiRef, ret, __LINE__);
	//}

	if(ret){
		if (threadid) PostThreadMessageA(threadid, WM_USER+10, 0, 0);
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

	// v2.05.15: phwho could be NULL!!
	OutTraceSND("%s: hwo=%#x\n", ApiRef, phwo ? *phwo : NULL);

	return ret;
}

MMRESULT WINAPI extwaveOutReset(HWAVEOUT hwo)
{
	MMRESULT ret;
	ApiName("waveOutReset");
	OutTraceSND("%s: hwo=%#x\n", ApiRef, hwo);

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}
	if(BYPASSWAVEOUT) return 0;

	ret = (*pwaveOutReset)(hwo);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extwaveOutRestart(HWAVEOUT hwo)
{
	MMRESULT ret;
	ApiName("waveOutRestart");
	OutTraceSND("%s: hwo=%#x\n", ApiRef, hwo);

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}
	if(BYPASSWAVEOUT) return 0;

	ret = (*pwaveOutRestart)(hwo);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

MMRESULT WINAPI extwaveOutPause(HWAVEOUT hwo)
{
	MMRESULT ret;
	ApiName("waveOutPause");
	OutTraceSND("%s: hwo=%#x\n", ApiRef, hwo);
		
	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}
	if(BYPASSWAVEOUT) return 0;

	ret = (*pwaveOutPause)(hwo);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

UINT WINAPI extwaveOutGetNumDevs()
{
	UINT ret;
	ApiName("waveOutGetNumDevs");
		
	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=0\n", ApiRef);
		return 0;
	}
	if(BYPASSWAVEOUT) return 1;

	ret = (*pwaveOutGetNumDevs)();
	OutTraceSND("%s: numdevs=%d\n", ApiRef, ret);
	return ret;
}

MMRESULT WINAPI extwaveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
{
	MMRESULT ret;
	ApiName("waveOutPrepareHeader");
	OutTraceSND("%s: hwo=%#x cbwh=%#x\n", ApiRef, hwo, cbwh);

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}
	if(BYPASSWAVEOUT) return 0;

	ret = (*pwaveOutPrepareHeader)(hwo, pwh, cbwh);
#ifndef DXW_NOTRACES
	if(IsDebugSND) dumpHWaveHdr(pwh);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
#endif // DXW_NOTRACES
	return ret;
}

DWORD WINAPI waveoutunprepareheader_wait(LPVOID param)
{
	unprep *unp = (unprep *)param;
	HWAVEOUT hwo = unp->hwo;
	LPWAVEHDR pwh = unp->pwh;
	UINT cbwh = unp->cbwh;
	HeapFree(GetProcessHeap(), 0, param);
	(*pwaveOutUnprepareHeader)(hwo, pwh, cbwh);
	return 0;
}

MMRESULT WINAPI extwaveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
{
	MMRESULT ret;
	ApiName("waveOutUnprepareHeader");
	OutTraceSND("%s: hwo=%#x cbwh=%#x\n", ApiRef, hwo, cbwh);
#ifndef DXW_NOTRACES
	if(IsDebugSND) dumpHWaveHdr(pwh);
#endif // DXW_NOTRACES

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}
	if(BYPASSWAVEOUT) return 0;

	if(dxw.dwFlags19 & ASYNCWAVEOUTCLOSE){
		unprep *unp = (unprep *)HeapAlloc(GetProcessHeap(), 0, sizeof(unprep));
		unp->hwo = hwo;
		unp->pwh = pwh;
		unp->cbwh = cbwh; 
		CreateThread(NULL, 0, waveoutunprepareheader_wait, unp, 0, NULL);
		return MMSYSERR_NOERROR;
	}

	ret = (*pwaveOutUnprepareHeader)(hwo, pwh, cbwh);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

/* on Windows Vista and later, waveOutSetVolume is always tied to master volume */
MMRESULT WINAPI extwaveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
{
	MMRESULT ret;
	ApiName("waveOutWrite");
	OutTraceSND("%s: hwo=%#x cbwh=%#x\n", ApiRef, hwo, cbwh);
#ifndef DXW_NOTRACES
	if(IsDebugSND) dumpHWaveHdr(pwh);
#endif // DXW_NOTRACES

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}
	if(BYPASSWAVEOUT) return 0;

	if((dxw.dwFlags15 & EMULATEVOLUME) && pwh && pwh->lpData) {
		/* Windows is f**ked up. MIDI synth driver converts MIDI to WAVE and then calls winmm.waveOutWrite!!! */
		void *addr = _ReturnAddress();
		char caller[MAX_PATH];
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
		GetModuleFileName((HMODULE)mbi.AllocationBase, caller, MAX_PATH);
		OutTrace("%s: caller=%s\n", ApiRef, caller);

		float vol = -1.0;
		char *pos = strrchr(caller, '\\');
		/* Mixer: msacm32.drv */
		if (strstr(pos, "wdmaud.drv")) {
			OutTrace("%s: emulate midi volume=%d%% bits=%d\n", 
				ApiRef, dxw.MidiVolume, waveBits);
			vol = (float)dxw.MidiVolume / (float)100;
			// if midiOutShort/LongMsg calls were hooked, don't scale the volume twice!
			if(bMidiMessageHooked) vol = -1.0; 
		}
		else if (!strstr(pos, ".drv")) {
			OutTrace("%s: emulate wave volume=%d%% bits=%d\n", 
				ApiRef, dxw.WaveVolume, waveBits);
			vol = (float)dxw.WaveVolume / (float)100;
		}
		else {
			OutTrace("%s: unknown audio driver\n", ApiRef);
		}

		if(vol > 1.0) vol = 1.0;
		if (vol >= 0) { 
			short *wave16;
			unsigned char *wave8;
			switch (waveBits) {
				case 16:
					wave16 = (short *)pwh->lpData;
					for (int i = 0, j = pwh->dwBufferLength/2; i < j; i++) {
						wave16[i] *= vol;
					}
					break;
				case 8:
					wave8 = (unsigned char *)pwh->lpData;
					for (int i = 0, j = pwh->dwBufferLength; i < j; i++) {
						wave8[i] *= vol;
					}
					break;
				default:
					OutTrace("%s: unsupported waveBits=%d\n", ApiRef, waveBits);
					break; /* WTF? */
			}
		}
	}	

	ret = (*pwaveOutWrite)(hwo, pwh, cbwh);
	_if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	return ret;
}

#ifndef DXW_NOTRACES
char *sWType(DWORD wtype)
{
	char *s = "???";
	switch(wtype){
		case TIME_MS: s = "msec"; break;
		case TIME_SAMPLES: s = "samples"; break;
		case TIME_BYTES: s = "bytes"; break;
		case TIME_SMPTE: s = "smpte"; break;
		case TIME_MIDI: s = "midi"; break;
		case TIME_TICKS: s = "ticks"; break;
	}
	return s;
}
#endif // DXW_NOTRACES

MMRESULT WINAPI extwaveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt)
{
	MMRESULT ret;
	ApiName("waveOutGetPosition");
	OutTraceSND("%s: hwo=%#x cbmmt=%d\n", ApiRef, hwo, cbmmt);
#ifndef DXW_NOTRACES
	if(IsTraceSND){
		OutTrace("> wTime=%d(%s)\n", pmmt->wType, sWType(pmmt->wType));
		if(pmmt->wType == TIME_SMPTE)
			OutTrace("> smpte=0sd:%02d:%02d.%02d fps=%02d\n",
				pmmt->u.smpte.hour,
				pmmt->u.smpte.min,
				pmmt->u.smpte.sec,
				pmmt->u.smpte.frame,
				pmmt->u.smpte.fps);
		else
			OutTrace("> val=%#x\n", pmmt->u.ticks);
	}
#endif // DXW_NOTRACES

	if(dxw.dwFlags19 & NOWAVEOUT) {
		OutTrace("%s: waveOut disabled res=MMSYSERR_NODRIVER\n", ApiRef);
		return MMSYSERR_NODRIVER;
	}

	if(dxw.dwFlags13 & BYPASSWAVEOUTPOS){
		OutTraceDW("%s: skip call\n", ApiRef);
		pmmt->u.ms = 0;
		return 0;
	}
	ret = (*pwaveOutGetPosition)(hwo, pmmt, cbmmt);
	if(ret) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
#ifndef DXW_NOTRACES
	else {
		if(IsTraceSND){
		OutTrace("< wTime=%d(%s)\n", pmmt->wType, sWType(pmmt->wType));
		if(pmmt->wType == TIME_SMPTE)
			OutTrace("< smpte=0sd:%02d:%02d.%02d fps=%02d\n",
				pmmt->u.smpte.hour,
				pmmt->u.smpte.min,
				pmmt->u.smpte.sec,
				pmmt->u.smpte.frame,
				pmmt->u.smpte.fps);
		else
			OutTrace("< val=%#x\n", pmmt->u.ticks);
		}
	}
#endif // DXW_NOTRACES
	return ret;
}

extern MCIERROR WINAPI extmciSendString(char *, LPCTSTR, LPTSTR, UINT, HANDLE);

MCIERROR WINAPI extmciSendStringA(LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{ 
	ApiName("mciSendStringA");
	if(IsWithinMCICall) return(*pmciSendStringA)(lpszCommand, lpszReturnString, cchReturn, hwndCallback); // just proxy ...

	if(!strcmp(lpszCommand, "play music notify")) return 0;

	MCIERROR ret;
	LPCSTR lpszCommandA;
	size_t len;
	len = strlen(lpszCommand);
	lpszCommandA = (LPCSTR)malloc(len+1);
	strcpy((LPSTR)lpszCommandA, lpszCommand);
	// v2.04.96: "Fallen Haven" sends uppercase strings ...
	for(char *p=(char *)lpszCommandA; *p; p++) *p = tolower(*p);
	ret = extmciSendString(ApiRef, lpszCommandA, lpszReturnString, cchReturn, hwndCallback); 
	free((LPVOID)lpszCommandA);
	return ret;
}

MCIERROR WINAPI extmciSendStringW(LPCWSTR lpszCommand, LPWSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
	ApiName("mciSendStringW");
	if(IsWithinMCICall) return(*pmciSendStringW)(lpszCommand, lpszReturnString, cchReturn, hwndCallback); // just proxy ...

	MCIERROR ret;
	LPCSTR lpszCommandA;
	LPCSTR lpszReturnStringA; 
	size_t len;
	BOOL returnsDWord = FALSE;
	len = wcslen(lpszCommand);
	lpszCommandA = (LPCSTR)malloc((2*len)+1);
	if(lpszReturnString){
		lpszReturnStringA = (LPCSTR)malloc((2*cchReturn)+1);
		(char)lpszReturnStringA[0] = 0;
	}
	else {
		lpszReturnStringA = NULL;
	}
	_wcstombs_s_l(&len, (char *)lpszCommandA, 2*len, lpszCommand, _TRUNCATE, NULL);
	for(char *p=(char *)lpszCommandA; *p; p++) *p = tolower(*p);

	// decides whether to convert a string or just copy a DWORD value
	if (!strncmp(lpszCommandA, "window", strlen("window")) ||
		!strncmp(lpszCommandA, "set", strlen("set")) ||	
		!strncmp(lpszCommandA, "status", strlen("status")) ||	
		!strncmp(lpszCommandA, "configure", strlen("configure")) ||	
		!strncmp(lpszCommandA, "put", strlen("put")) 
		) returnsDWord = TRUE;

	// v2.05.92: beware! the "status" command can return either a string or a DWORD depending on the 
	// status specification. "start position" seems to return a string with a number (isn't it crazy?).
	// fixes F/A 18 Korea".
	if(!strncmp(lpszCommandA, "status", strlen("status")  && strstr(lpszCommandA, "start position"))) returnsDWord = FALSE;

	ret = extmciSendString(ApiRef, lpszCommandA, (LPTSTR)lpszReturnStringA, cchReturn, hwndCallback); 
	// v2.04.54 fix: not all retstrings are strings!!
	// v2.05.92 fix: do not try to convert a null return value.
	if (lpszReturnStringA) {
		if(returnsDWord) {
			memcpy (lpszReturnString, lpszReturnStringA, cchReturn);
		}
		else {
			mbstowcs (lpszReturnString, lpszReturnStringA, cchReturn);
		}
	}
	free((LPVOID)lpszCommandA);
	free((LPVOID)lpszReturnStringA);
	return ret;
}

UINT WINAPI extauxGetNumDevs(void)
{
	ApiName("auxGetNumDevs");
	UINT ret;

	ret = (*pauxGetNumDevs)();
	OutTraceSND("%s: ret=%d\n", ApiRef, ret);
	if((dxw.dwFlags8 & VIRTUALCDAUDIO) && (dxw.dwFlags13 & EMULATECDAUX)){
		// v2.05.36: pretend there's ALWAYS a first device id = 0 for the audio mixer
		if (ret == 0) {
			ret = 1;
			OutTraceSND("%s: adding fake MIXER aux id=0\n", ApiRef);
		}
		// now adding the CD aux
		dxw.VirtualCDAuxDeviceId = ret;
		ret++;
		OutTraceSND("%s: adding fake CD aux id=%d ret=%d\n", 
			ApiRef, dxw.VirtualCDAuxDeviceId, ret);
	}

	return ret;
}

MMRESULT WINAPI extauxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT cbCaps)
{
	MMRESULT ret;
	ApiName("auxGetDevCapsA");
	OutTraceSND("%s: uDeviceID=%#x cbCaps=%d\n", ApiRef, uDeviceID, cbCaps);
#ifndef DXW_NOTRACES
	if(cbCaps != sizeof(AUXCAPSA)) dprintf("%s: mismatched cbCaps expected=%d\n", ApiRef, sizeof(AUXCAPSA));
#endif

	ret = (*pauxGetDevCapsA)(uDeviceID, lpCaps, cbCaps);

	if((dxw.dwFlags8 & VIRTUALCDAUDIO) && (ret == ERROR_FILE_NOT_FOUND)) {
		if ((dxw.dwFlags13 & EMULATECDAUX) && (uDeviceID == dxw.VirtualCDAuxDeviceId)) {
			lpCaps->wMid = 2 /*MM_CREATIVE*/;
			lpCaps->wPid = 401 /*MM_CREATIVE_AUX_CD*/;
			lpCaps->vDriverVersion = 1;
			strcpy(lpCaps->szPname, "dxwnd virtual CD");
			lpCaps->wTechnology = AUXCAPS_CDAUDIO;
			lpCaps->dwSupport = AUXCAPS_VOLUME|AUXCAPS_LRVOLUME;
			lpCaps->wReserved1 = 0;

			ret = MMSYSERR_NOERROR;
		}
		else
		if((dxw.dwFlags13 & EMULATECDMIXER) && ((uDeviceID == 0) || (uDeviceID == 0xFFFFFFFF))){
			lpCaps->wMid = 2 /*MM_CREATIVE*/;
			lpCaps->wPid = 409 /*MM_CREATIVE_SB16_MIXER*/;
			lpCaps->vDriverVersion = 1;
			strcpy(lpCaps->szPname, "dxwnd virtual mixer");
			lpCaps->wTechnology = AUXCAPS_AUXIN;
			lpCaps->dwSupport = AUXCAPS_VOLUME|AUXCAPS_LRVOLUME;
			lpCaps->wReserved1 = 0;

			ret = MMSYSERR_NOERROR;		
		}
	}

	if(ret){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

#ifndef DXW_NOTRACES
	if(IsTraceSND){
		OutTrace("> Mid = %#x\n", lpCaps->wMid);
		OutTrace("> Pid = %#x\n", lpCaps->wPid);
		OutTrace("> DriverVersion = %#x\n", lpCaps->vDriverVersion);
		OutTrace("> Pname = %s\n", lpCaps->szPname);
		OutTrace("> Technology = %#x\n", lpCaps->wTechnology);
		OutTrace("> Reserved1 = %#x\n", lpCaps->wReserved1);
		OutTrace("> Support = %#x\n", lpCaps->dwSupport);
	}
#endif // DXW_NOTRACES
	return ret;
}

MMRESULT WINAPI extauxGetVolume(UINT uDeviceID, LPDWORD lpdwVolume)
{
	MMRESULT res;
	ApiName("auxGetVolume");

	OutTraceSND("%s: uDeviceId=%#x\n", ApiRef, uDeviceID);

	if((dxw.dwFlags8 & VIRTUALCDAUDIO) && (uDeviceID == dxw.VirtualCDAuxDeviceId)){
		// "Outlaws" calls auxGetVolume before any mciSendCommand call, 
		// so you have to initialize also here.
		if(!pplr_pump) player_init();

		int volume;
		volume = (*pplr_getvolume)();

		dprintf("> volume=%d%%\n", volume);
		int lvol = (volume * 0xFFFF) / 100;
		*lpdwVolume = (lvol & 0xFFFF) | (lvol & 0xFFFF)<<16;
		dprintf("> volume=%#x\n", *lpdwVolume);
    
		return MMSYSERR_NOERROR;
	}

	res = (*pauxGetVolume)(uDeviceID, lpdwVolume);
	_if(res) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));
	return res;
}

MMRESULT WINAPI extauxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	MMRESULT res;
	ApiName("auxSetVolume");

	OutTraceSND("%s: uDeviceId=%#x dwVolume=%#x\n", ApiRef, uDeviceID, dwVolume);

	unsigned short left = LOWORD(dwVolume);
	unsigned short right = HIWORD(dwVolume);
	int volume;

	if((dxw.dwFlags8 & VIRTUALCDAUDIO) && (uDeviceID == dxw.VirtualCDAuxDeviceId)){

		// "Outlaws" calls auxSetVolume before any mciSendCommand call, 
		// so you have to initialize also here.
		if(!pplr_pump) player_init();

		dprintf("> left : %ud (%04X)\n", left, left);
		dprintf("> right: %ud (%04X)\n", right, right);

		if(dxw.dwFlags9 & LOCKVOLUME) {
			OutTraceSND("> CD volume LOCKED vol=%d%%\n", dxw.CDAVolume);
			volume = dxw.CDAVolume;
		}
		else {
			// set the average between left and right, the emulated volume has no balance yet!
			volume = (((unsigned int)left * 50) + ((unsigned int)right * 50)) / 0xFFFF;
			OutTraceSND("> CD volume vol=%d%%\n", volume);
		}

		emuSetVolume(volume);
		return MMSYSERR_NOERROR;
	}

	if(dxw.dwFlags9 & LOCKVOLUME) {
		OutTraceSND("> aux volume LOCKED vol=%d%%\n", dxw.WaveVolume);
		dwVolume = MapVolume(dxw.WaveVolume);
	}
	else {
		OutTraceSND("> aux volume vol(L,R)=%d%%, %d%%\n", (left * 100) / 0xFFFF, (right * 100) / 0xFFFF);
	}

	res = (*pauxSetVolume)(uDeviceID, dwVolume);
	_if(res) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));
	return res;
}

MMRESULT WINAPI extauxOutMessage(UINT uDeviceID, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
{
	MMRESULT res;
	ApiName("auxOutMessage");

	OutTraceSND("%s: uDeviceId=%#x msg=%#x args=(%#x, %#x)\n", ApiRef, uMsg, dw1, dw2);

	res = (*pauxOutMessage)(uDeviceID, uMsg, dw1, dw2);
	_if(res) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));

	return res;
}

MMRESULT WINAPI extwaveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume)
{
	MMRESULT res;
	ApiName("waveOutSetVolume");

	OutTraceSND("%s: hwo=%#x vol=%#x\n", ApiRef, hwo, dwVolume);

	if(dxw.dwFlags9 & LOCKVOLUME) {
		OutTraceSND("> wave volume LOCKED vol=%d%%\n", dxw.WaveVolume);
		dwVolume = MapVolume(dxw.WaveVolume);
	}

	// in case of volume emulation, set the maximum possible volume and let
	// the control to be done in waveOutWrite wrapper.
	if((dxw.dwFlags15 & EMULATEVOLUME) && dwVolume) dwVolume = 0xFFFFFFFF; 

	res = (*pwaveOutSetVolume)(hwo, dwVolume);
	_if(res) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));
	return res;
}

MMRESULT WINAPI extwaveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume)
{
	MMRESULT res;
	ApiName("waveOutGetVolume");

	OutTraceSND("%s: hwo=%#x\n", ApiRef, hwo);
	res = (*pwaveOutGetVolume)(hwo, pdwVolume);
	if(res != MMSYSERR_NOERROR){
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));
		return res;
	}

	OutTraceSND("%s: vol=%#x\n", ApiRef, *pdwVolume);
	return res;
}

// -- SAFEMIDIOUT ---

MMRESULT WINAPI extmidiOutReset(HMIDIOUT hmo)
{
	MMRESULT res;
	ApiName("midiOutReset");

	OutTraceSND("%s: hmo=%#x\n", ApiRef, hmo);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	if(dxw.dwFlags11 & SAFEMIDIOUT) {
		OutTraceSND("%s: SAFEMIDIOUT skip call\n", ApiRef);
		return MMSYSERR_NOERROR;
	}
	res = (*pmidiOutReset)(hmo);
	_if(res) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));

	return res;
}

MMRESULT WINAPI extmidiOutSetVolume(HMIDIOUT hmo, DWORD dwVolume)
{
	MMRESULT res;
	ApiName("midiOutSetVolume");
	OutTraceSND("%s: hmo=%#x vol=%04.4x~%04.4x\n", ApiRef, hmo, (dwVolume>>8) & 0xFFFF, dwVolume & 0xFFFF);

	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMSYSERR_NOERROR;

	if(dxw.dwFlags11 & SAFEMIDIOUT) return MMSYSERR_NOERROR;

	if(dxw.dwFlags9 & LOCKVOLUME) {
		OutTraceSND("> midi volume LOCKED vol=%d%%\n", dxw.MidiVolume);
		dwVolume = MapVolume(dxw.WaveVolume);
	}

	// in case of volume emulation, set the maximum possible volume and let
	// the control to be done in midiStreamOut wrapper.
	if((dxw.dwFlags15 & EMULATEVOLUME) && dwVolume) dwVolume = 0xFFFFFFFF; 

	res = (*pmidiOutSetVolume)(hmo, dwVolume);
	_if(res) OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, res, mmErrorString(res));
	return res;
}

#ifndef DXW_NOTRACES
static void dumpmmioInfo(LPMMIOINFO pmmioinfo)
{
		if(!pmmioinfo){
			OutTrace("> pmmioinfo: NULL\n");
			return;
		}
		OutTrace("> dwFlags: %#x\n", pmmioinfo->dwFlags);
		OutTrace("> fccIOProc: %#x(%s)\n", pmmioinfo->fccIOProc, sFourCC(pmmioinfo->fccIOProc));
		OutTrace("> pIOProc: %#x\n", pmmioinfo->pIOProc);
		OutTrace("> wErrorRet: %d\n", pmmioinfo->wErrorRet);
		OutTrace("> htask: %#x\n", pmmioinfo->htask);
		OutTrace("> cchBuffer: %d\n", pmmioinfo->cchBuffer);
		OutTrace("> lBufOffset: %d\n", pmmioinfo->lBufOffset);
		OutTrace("> lDiskOffset: %d\n", pmmioinfo->lDiskOffset);
		OutTrace("> adwInfo: {%#x, %#x, %#x}\n", pmmioinfo->adwInfo[0], pmmioinfo->adwInfo[1], pmmioinfo->adwInfo[2]);
		OutTrace("> dwReserved1,2: %#x, %#x\n", pmmioinfo->dwReserved1, pmmioinfo->dwReserved2);
		OutTrace("> hmmio: %#x\n", pmmioinfo->hmmio);
}
#endif // DXW_NOTRACES

HMMIO WINAPI extmmioOpenA(LPCSTR lpFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen)
{
	HMMIO res;
	ApiName("mmioOpenA");
	OutTraceSYS("%s: filename=\"%s\" ioinfo=%#x open=%#x\n", ApiRef, lpFileName, pmmioinfo, fdwOpen);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		// BEWARE: NULL pointer and path containing a '+' character are special mmioOpen cases!
		if(lpFileName && !strchr(lpFileName, '+')){
			lpFileName = dxwTranslatePathA(lpFileName, NULL);
		}
	}

	// v2.06.03: bypass for NULL lpFileName
	// @#@ "First Queen 1" (Jp 2001)
	if((dxw.dwFlags12 & PATHLOCALE) && lpFileName){
		typedef int (WINAPI *MultiByteToWideChar_Type)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
		extern MultiByteToWideChar_Type pMultiByteToWideChar;
		int size = strlen(lpFileName);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpFileName, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		if(!pmmioOpenW){
			// unhooked call, must get address
			HMODULE hmod = (*pLoadLibraryA)("winmm.dll");
			_if(!hmod) OutTraceE("%s: unexpected LoadLibrary ERROR err=%d @%d\n", GetLastError(), __LINE__);
			pmmioOpenW = (mmioOpenW_Type)(*pGetProcAddress)(hmod, "mmioOpenW");
			_if(!pmmioOpenW) OutTraceE("%s: unexpected GetProcAddress ERROR err=%d @%d\n", GetLastError(), __LINE__);
		}
		res = (*pmmioOpenW)(lpFileNameW, pmmioinfo, fdwOpen);
		free(lpFileNameW);
	}
	else {
		res = (*pmmioOpenA)(lpFileName, pmmioinfo, fdwOpen);
	}

#ifndef DXW_NOTRACES
	if(res) {
		OutTraceSYS("%s: ret(hmmio)=%#x\n", ApiRef, res);
		if(IsTraceSYS) dumpmmioInfo(pmmioinfo);
	}
	else {
		if(pmmioinfo){
			OutErrorSYS("%s: ERROR wErrorRet=%#x err=%d\n", ApiRef, pmmioinfo->wErrorRet, GetLastError());
		} 
		else {
			OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
		}
	}
#endif // DXW_NOTRACES

	return res;
}

HMMIO WINAPI extmmioOpenW(LPCWSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen)
{
	HMMIO res;
	ApiName("mmioOpenW");
	OutTraceSYS("%s: filename=\"%ls\" ioinfo=%#x open=%#x\n", ApiRef, pszFileName, pmmioinfo, fdwOpen);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		// BEWARE: NULL pointer and path containing a '+' character are special mmioOpen cases!
		if(pszFileName && !wcschr(pszFileName, L'+')){
			pszFileName = dxwTranslatePathW(pszFileName, NULL);
		}
	}
	res = (*pmmioOpenW)(pszFileName, pmmioinfo, fdwOpen);
	if(res) {
		OutTraceSYS("%s: ret(hmmio)=%#x\n", ApiRef, res);
		if(IsTraceSYS) dumpmmioInfo(pmmioinfo);
	}
	else {
		if(pmmioinfo){
			OutErrorSYS("%s: ERROR wErrorRet=%#x err=%d\n", ApiRef, pmmioinfo->wErrorRet, GetLastError());
		} 
		else {
			OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
		}
	}
	return res;
}

#ifndef DXW_NOTRACES
static char *sDescend(UINT f, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"MMIO_", eblen);
	if (f & MMIO_FINDCHUNK) strscat(eb, eblen, "FINDCHUNK+");
	if (f & MMIO_FINDRIFF) strscat(eb, eblen, "FINDRIFF+");
	if (f & MMIO_FINDLIST) strscat(eb, eblen, "FINDLIST+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("MMIO_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

static void dumpMMCkInfo(LPMMCKINFO lpmmcki)
{
	if(!lpmmcki){
		OutTrace("> NULL\n");
		return;
	}
	OutTrace("> ckid: %x(%s)\n", lpmmcki->ckid, sFourCC(lpmmcki->ckid));
	OutTrace("> cksize: %d\n", lpmmcki->cksize);
	OutTrace("> fccType: %x(%s)\n", lpmmcki->fccType, sFourCC(lpmmcki->fccType));
	OutTrace("> dwDataOffset: %d\n", lpmmcki->dwDataOffset);
	OutTrace("> dwFlags: %#x\n", lpmmcki->dwFlags);
}
#endif // DXW_NOTRACES

MMRESULT WINAPI extmmioDescend(HMMIO hmmio, LPMMCKINFO pmmcki, const MMCKINFO *pmmckiParent, UINT fuDescend)
{
	MMRESULT ret;
	ApiName("mmioDescend");
	//MMIO_FINDCHUNK 	Searches for a chunk with the specified chunk identifier.
	//MMIO_FINDLIST 	Searches for a chunk with the chunk identifier "LIST" and with the specified form type.
	//MMIO_FINDRIFF 	Searches for a chunk with the chunk identifier "RIFF" and with the specified form type.
#ifndef DXW_NOTRACES
	if(IsTraceSYS){
		char sBuf[80];
		OutTrace("%s: hmmio=%#x descend=%d(%s)\n", ApiRef, hmmio, fuDescend, sDescend(fuDescend, sBuf, 80));
	}
#endif // DXW_NOTRACES

 	if(dxw.dwFlags14 & MIDIOUTBYPASS) return MMIOERR_CHUNKNOTFOUND;

	ret = (*pmmioDescend)(hmmio, pmmcki, pmmckiParent, fuDescend);
#ifndef DXW_NOTRACES
	if(ret != MMSYSERR_NOERROR) {
		OutErrorSYS("%s: ERROR ret=%d\n", ApiRef, ret);
	}
	else {
		if(IsTraceSYS){
			OutTrace("pmmcki:\n");
			dumpMMCkInfo(pmmcki);
			OutTrace("pmmckiParent:\n");
			dumpMMCkInfo((LPMMCKINFO)pmmckiParent);
		}
	}
#endif // DXW_NOTRACES
	return ret;
}

LONG WINAPI extmmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin)
{
	LONG ret;
	ApiName("mmioSeek");
	OutTraceSYS("%s: hmmio=%#x offset=%d origin=%d\n", ApiRef, hmmio, lOffset, iOrigin);
 
	ret = (*pmmioSeek)(hmmio, lOffset, iOrigin);
	OutTraceSYS("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

LONG WINAPI extmmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
	LONG ret;
	ApiName("mmioRead");
	OutTraceSYS("%s: hmmio=%#x pch=%#x cch=%d\n", ApiRef, hmmio, pch, cch);
 
	ret = (*pmmioRead)(hmmio, pch, cch);
	OutTraceSYS("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extsndPlaySoundA(LPCSTR lpszSound, UINT fuSound)
{
	BOOL ret;
	ApiName("sndPlaySoundA");
	OutTraceSND("%s: path=\"%s\" sound=%#x\n", ApiRef, lpszSound, fuSound);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		if(lpszSound) lpszSound = dxwTranslatePathA(lpszSound, NULL);
	}

	// v2.06.03: Path locale translation
	if((dxw.dwFlags12 & PATHLOCALE) && lpszSound && psndPlaySoundW){
		typedef int (WINAPI *MultiByteToWideChar_Type)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
		extern MultiByteToWideChar_Type pMultiByteToWideChar;
		int size = strlen(lpszSound);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpszSound, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		ret = (*psndPlaySoundW)(lpFileNameW, fuSound);
		free(lpFileNameW);
	}
	else {
		ret = (*psndPlaySoundA)(lpszSound, fuSound);
	}

	_if(!ret) OutErrorSND("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extsndPlaySoundW(LPCWSTR lpszSound, UINT fuSound)
{
	BOOL ret;
	ApiName("sndPlaySoundW");
	OutTraceSND("%s: path=\"%ls\" sound=%#x\n", ApiRef, lpszSound, fuSound);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		if(lpszSound) lpszSound = dxwTranslatePathW(lpszSound, NULL);
	}
	ret = (*psndPlaySoundW)(lpszSound, fuSound);
	_if(!ret) OutErrorSND("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

#ifndef DXW_NOTRACES
static char *sfuSound(UINT c)
{
	static char eb[81];
	unsigned int l;
	strcpy(eb,"SND_");
	if (c & SND_ASYNC) strcat(eb, "ASYNC+"); else strcat(eb, "SYNC+");
	if (c & SND_NODEFAULT) strcat(eb, "NODEFAULT+");
	if (c & SND_MEMORY) strcat(eb, "MEMORY+");
	if (c & SND_LOOP) strcat(eb, "LOOP+");
	if (c & SND_NOSTOP) strcat(eb, "NOSTOP+");
	if (c & SND_NOWAIT) strcat(eb, "NOWAIT+");
	if (c & SND_ALIAS) strcat(eb, "ALIAS+");
	//if (c & SND_ALIAS_ID) strcat(eb, "ALIAS_ID+");
	if (c & 0x00100000L) strcat(eb, "ALIAS_ID+"); // 0x00100000L = SND_ALIAS_ID & ~SND_ALIAS
	if (c & SND_FILENAME) strcat(eb, "FILENAME+");
	//if (c & SND_RESOURCE) strcat(eb, "RESOURCE+");
	if (c & 0x00040000L) strcat(eb, "RESOURCE+"); // 0x00040000L = SND_RESOURCE & ~SND_MEMORY
	if (c & SND_SENTRY) strcat(eb, "SENTRY+");
	if (c & SND_PURGE) strcat(eb, "PURGE+");
	if (c & SND_APPLICATION) strcat(eb, "APPLICATION+");
	if (c & SND_SENTRY) strcat(eb, "SENTRY+");
	if (c & SND_SYSTEM) strcat(eb, "SYSTEM+");
	l=strlen(eb);
	if (l>strlen("SND_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}
#endif // DXW_NOTRACES

BOOL WINAPI extPlaySoundA(LPCSTR lpszSound, HMODULE hmod, UINT fuSound)
{
	BOOL ret;
	ApiName("PlaySoundA");
#ifndef DXW_NOTRACES
	if(fuSound & SND_RESOURCE){
		OutTraceSND("%s: atom=%#x hmod=%#x sound=%#x(%s)\n", 
			ApiRef, lpszSound, hmod, fuSound, sfuSound(fuSound));
	}
	else {
		OutTraceSND("%s: path=\"%s\" hmod=%#x sound=%#x(%s)\n", 
			ApiRef, lpszSound, hmod, fuSound, sfuSound(fuSound));
	}
#endif // DXW_NOTRACES
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && (fuSound & SND_FILENAME)){
		if(lpszSound) lpszSound = dxwTranslatePathA(lpszSound, NULL);
	}
	ret = (*pPlaySoundA)(lpszSound, hmod, fuSound);
	_if(!ret) OutErrorSND("%s: ERROR err=%d\n", ApiRef, GetLastError());

	if((SND_RESOURCE & fuSound) && (ERROR_RESOURCE_TYPE_NOT_FOUND == GetLastError())) {
		OutTraceSND("%s: SUPPRESS legacy error ERROR_RESOURCE_TYPE_NOT_FOUND\n", ApiRef);
		ret = TRUE;
	}
	return ret;
}

BOOL WINAPI extPlaySoundW(LPCWSTR lpszSound, HMODULE hmod, UINT fuSound)
{
	BOOL ret;
	ApiName("PlaySoundW");
#ifndef DXW_NOTRACES
	if(fuSound & SND_RESOURCE){
		OutTraceSND("%s: atom=%#x hmod=%#x sound=%#x(%s)\n", 
			ApiRef, lpszSound, hmod, fuSound, sfuSound(fuSound));
	}
	else {
		OutTraceSND("%s: path=\"%ls\" hmod=%#x sound=%#x(%s)\n", 
			ApiRef, lpszSound, hmod, fuSound, sfuSound(fuSound));
	}
#endif // DXW_NOTRACES
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && (fuSound & SND_FILENAME)){
		if(lpszSound) lpszSound = dxwTranslatePathW(lpszSound, NULL);
	}
	ret = (*pPlaySoundW)(lpszSound, hmod, fuSound);
	_if(!ret) OutErrorSND("%s: ERROR err=%d\n", ApiRef, GetLastError());

	if((SND_RESOURCE & fuSound) && (ERROR_RESOURCE_TYPE_NOT_FOUND == GetLastError())) {
		OutTraceSND("%s: SUPPRESS legacy error ERROR_RESOURCE_TYPE_NOT_FOUND\n", ApiRef);
		ret = TRUE;
	}
	return ret;
}

#ifdef TRACEMIXER
UINT WINAPI extmixerGetNumDevs(void)
{
	UINT ret;
	ApiName("mixerGetNumDevs");
	ret = (*pmixerGetNumDevs)();
	OutTraceSND("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

MMRESULT WINAPI extmixerOpen(LPHMIXER phmx, UINT uMxId, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{
	MMRESULT ret;
	ApiName("mixerOpen");
	OutTraceSND("%s: mxid=%#x cback=%#x fdwOpen=%#x\n", ApiRef, uMxId, dwCallback, fdwOpen);
	ret = (*pmixerOpen)(phmx, uMxId, dwCallback, dwInstance, fdwOpen);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}
	OutTraceSND("%s: hmixer=%#x\n", ApiRef, *phmx);
	return ret;
}

MMRESULT WINAPI extmixerClose(HMIXER hMixer)
{
	MMRESULT ret;
	ApiName("mixerClose");
	OutTraceSND("%s: hmixer=%#x\n", ApiRef, hMixer);
	ret = (*pmixerClose)(hMixer);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}
	return ret;
}

#ifndef DXW_NOTRACES
/* fdwInfo:
#define MIXER_GETLINEINFOF_DESTINATION      0x00000000L
#define MIXER_GETLINEINFOF_SOURCE           0x00000001L
#define MIXER_GETLINEINFOF_LINEID           0x00000002L
#define MIXER_GETLINEINFOF_COMPONENTTYPE    0x00000003L
#define MIXER_GETLINEINFOF_TARGETTYPE       0x00000004L
#define MIXER_OBJECTF_HANDLE    0x80000000L
#define MIXER_OBJECTF_MIXER     0x00000000L
#define MIXER_OBJECTF_HMIXER    (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIXER)
#define MIXER_OBJECTF_WAVEOUT   0x10000000L
#define MIXER_OBJECTF_HWAVEOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEOUT)
#define MIXER_OBJECTF_WAVEIN    0x20000000L
#define MIXER_OBJECTF_HWAVEIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEIN)
#define MIXER_OBJECTF_MIDIOUT   0x30000000L
#define MIXER_OBJECTF_HMIDIOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIOUT)
#define MIXER_OBJECTF_MIDIIN    0x40000000L
#define MIXER_OBJECTF_HMIDIIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIIN)
#define MIXER_OBJECTF_AUX       0x50000000L
*/
static char *sfdwInfo(DWORD f)
{
	static char s[81];
	switch(f & 0x0000000F){
		case MIXER_GETLINEINFOF_DESTINATION: strcpy(s, "DESTINATION+"); break;
		case MIXER_GETLINEINFOF_SOURCE: strcpy(s, "SOURCE+"); break;
		case MIXER_GETLINEINFOF_LINEID: strcpy(s, "LINEID+"); break;
		case MIXER_GETLINEINFOF_COMPONENTTYPE: strcpy(s, "COMPONENTTYPE+"); break;
		case MIXER_GETLINEINFOF_TARGETTYPE: strcpy(s, "TARGETTYPE+"); break;
		default: strcpy(s,"?+"); break;
	}
	if(f & MIXER_OBJECTF_HANDLE) strcat(s, "HANDLE+");
	switch(f & 0x70000000){
		case MIXER_OBJECTF_MIXER: strcat(s, "MIXER"); break;
		case MIXER_OBJECTF_WAVEOUT: strcat(s, "WAVEOUT"); break;
		case MIXER_OBJECTF_WAVEIN: strcat(s, "WAVEIN"); break;
		case MIXER_OBJECTF_MIDIOUT: strcat(s, "MIDIOUT"); break;
		case MIXER_OBJECTF_MIDIIN: strcat(s, "MIDIIN"); break;
		default: strcat(s, "?");
	}
	return s;
}

/* dwComponentType:
#define MIXERLINE_COMPONENTTYPE_DST_FIRST       0x00000000L
#define MIXERLINE_COMPONENTTYPE_DST_UNDEFINED   (MIXERLINE_COMPONENTTYPE_DST_FIRST + 0)
#define MIXERLINE_COMPONENTTYPE_DST_DIGITAL     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 1)
#define MIXERLINE_COMPONENTTYPE_DST_LINE        (MIXERLINE_COMPONENTTYPE_DST_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_DST_MONITOR     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 3)
#define MIXERLINE_COMPONENTTYPE_DST_SPEAKERS    (MIXERLINE_COMPONENTTYPE_DST_FIRST + 4)
#define MIXERLINE_COMPONENTTYPE_DST_HEADPHONES  (MIXERLINE_COMPONENTTYPE_DST_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_DST_TELEPHONE   (MIXERLINE_COMPONENTTYPE_DST_FIRST + 6)
#define MIXERLINE_COMPONENTTYPE_DST_WAVEIN      (MIXERLINE_COMPONENTTYPE_DST_FIRST + 7)
#define MIXERLINE_COMPONENTTYPE_DST_VOICEIN     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)
#define MIXERLINE_COMPONENTTYPE_DST_LAST        (MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)

#define MIXERLINE_COMPONENTTYPE_SRC_FIRST       0x00001000L
#define MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 0)
#define MIXERLINE_COMPONENTTYPE_SRC_DIGITAL     (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 1)
#define MIXERLINE_COMPONENTTYPE_SRC_LINE        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE  (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 3)
#define MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 4)
#define MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 6)
#define MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 7)
#define MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT     (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 8)
#define MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 9)
#define MIXERLINE_COMPONENTTYPE_SRC_ANALOG      (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)
#define MIXERLINE_COMPONENTTYPE_SRC_LAST        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)
*/
char *sComponentType(DWORD c)
{
	static char sBuf[22];
	char *p = NULL;
	char *dst[] = {
		"UNDEFINED", "DIGITAL", "LINE", "MONITOR", "SPEAKERS", 
		"HEADPHONES", "TELEPHONE", "WAVEIN", "VOICEIN"};
	char *src[] = {
		"UNDEFINED", "DIGITAL", "LINE", "MICROPHONE", "SYNTHESIZER", 
		"COMPACTDISC", "TELEPHONE", "PCSPEAKER", "WAVEOUT",
		"AUXILIARY", "ANALOG"};
	if((c>=MIXERLINE_COMPONENTTYPE_DST_FIRST) && (c<=MIXERLINE_COMPONENTTYPE_DST_LAST))
	{
		strcpy(sBuf, "DST_");
		p = dst[c-MIXERLINE_COMPONENTTYPE_DST_FIRST];
	}
	else
	if((c>=MIXERLINE_COMPONENTTYPE_SRC_FIRST) && (c<=MIXERLINE_COMPONENTTYPE_SRC_LAST))
	{
		strcpy(sBuf, "SRC_");
		p = dst[c-MIXERLINE_COMPONENTTYPE_SRC_FIRST];
	}
	if(p){
		strcat(sBuf, p);
		return sBuf;
	}
	else return "???";
}

/* fdwLine:
#define MIXERLINE_LINEF_ACTIVE              0x00000001L
#define MIXERLINE_LINEF_DISCONNECTED        0x00008000L
#define MIXERLINE_LINEF_SOURCE              0x80000000L
*/
#endif // DXW_NOTRACES

MMRESULT WINAPI extmixerGetLineInfoA(HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo)
{
	MMRESULT ret;
	ApiName("mixerGetLineInfoA");

	OutTraceSND("%s: hmxobj=%#x info=%#x(%s)\n", ApiRef, hmxobj, fdwInfo, sfdwInfo(fdwInfo));

#ifndef DXW_NOTRACES
	if(IsTraceSND) {
		OutTrace("< cbStruct=%d\n", pmxl->cbStruct);
		switch(fdwInfo){
			case MIXER_GETLINEINFOF_DESTINATION:
				OutTrace("< dwDestination=%d\n", pmxl->dwDestination);
				break;
			case MIXER_GETLINEINFOF_SOURCE:
				OutTrace("< dwDestination=%d\n", pmxl->dwDestination);
				OutTrace("< dwSource=%d\n", pmxl->dwSource);
				break;
			case MIXER_GETLINEINFOF_LINEID:
				OutTrace("< dwLineID=%#x\n", pmxl->dwLineID);
				break;
			case MIXER_GETLINEINFOF_COMPONENTTYPE:
				OutTrace("< dwComponentType=%#x(%s)\n", 
					pmxl->dwComponentType, sComponentType(pmxl->dwComponentType));
				break;
			case MIXER_GETLINEINFOF_TARGETTYPE:
				OutTrace("< TARGETTYPE?\n");
				break;
		}
	}
#endif // DXW_NOTRACES

	ret = (*pmixerGetLineInfoA)(hmxobj, pmxl, fdwInfo);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

#ifndef DXW_NOTRACES
	if(IsDebugSND){
		OutTrace("> dwDestination=%d\n", pmxl->dwDestination);
		OutTrace("> dwSource=%d\n", pmxl->dwSource);
		OutTrace("> dwLineID=%#x\n", pmxl->dwLineID);
		OutTrace("> fdwLine=%#x(%s/%s/%s)\n", pmxl->fdwLine, 
			pmxl->fdwLine & MIXERLINE_LINEF_ACTIVE ? "ACTIVE" : "",
			pmxl->fdwLine & MIXERLINE_LINEF_DISCONNECTED ? "DISCONNECTED" : "",
			pmxl->fdwLine & MIXERLINE_LINEF_SOURCE ? "SOURCE" : ""
			);
		OutTrace("> dwComponentType=%#x(%s)\n", pmxl->dwComponentType, sComponentType(pmxl->dwComponentType));
		OutTrace("> cChannels=%d\n", pmxl->cChannels);
		OutTrace("> cConnections=%d\n", pmxl->cConnections);

		OutTrace("> cControls=%d\n", pmxl->cControls);
		OutTrace("> szShortName=\"%s\"\n", pmxl->szShortName);
		OutTrace("> szName=\"%s\"\n", pmxl->szName);
	}
#endif // DXW_NOTRACES

	return ret;
}

#ifndef DXW_NOTRACES
/*
fdwControls:
#define MIXER_GETLINECONTROLSF_ALL          0x00000000L
#define MIXER_GETLINECONTROLSF_ONEBYID      0x00000001L
#define MIXER_GETLINECONTROLSF_ONEBYTYPE    0x00000002L
#define MIXER_OBJECTF_HANDLE    0x80000000L
#define MIXER_OBJECTF_MIXER     0x00000000L
#define MIXER_OBJECTF_HMIXER    (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIXER)
#define MIXER_OBJECTF_WAVEOUT   0x10000000L
#define MIXER_OBJECTF_HWAVEOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEOUT)
#define MIXER_OBJECTF_WAVEIN    0x20000000L
#define MIXER_OBJECTF_HWAVEIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEIN)
#define MIXER_OBJECTF_MIDIOUT   0x30000000L
#define MIXER_OBJECTF_HMIDIOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIOUT)
#define MIXER_OBJECTF_MIDIIN    0x40000000L
#define MIXER_OBJECTF_HMIDIIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIIN)
#define MIXER_OBJECTF_AUX       0x50000000L
*/
static char *sfdwControls(DWORD f)
{
	static char s[81];
	switch(f & 0x0000000F){
		case MIXER_GETLINECONTROLSF_ALL: strcpy(s, "ALL+"); break;
		case MIXER_GETLINECONTROLSF_ONEBYID: strcpy(s, "ONEBYID+"); break;
		case MIXER_GETLINECONTROLSF_ONEBYTYPE: strcpy(s, "ONEBYTYPE+"); break;
		default: strcpy(s,"?+"); break;
	}
	if(f & MIXER_OBJECTF_HANDLE) strcat(s, "HANDLE+");
	switch(f & 0x70000000){
		case MIXER_OBJECTF_MIXER: strcat(s, "MIXER"); break;
		case MIXER_OBJECTF_WAVEOUT: strcat(s, "WAVEOUT"); break;
		case MIXER_OBJECTF_WAVEIN: strcat(s, "WAVEIN"); break;
		case MIXER_OBJECTF_MIDIOUT: strcat(s, "MIDIOUT"); break;
		case MIXER_OBJECTF_MIDIIN: strcat(s, "MIDIIN"); break;
		default: strcat(s, "?");
	}
	return s;
}

static char *sControlType(DWORD t)
{
	char *p;
	switch(t){
		case MIXERCONTROL_CONTROLTYPE_CUSTOM: p="CUSTOM"; break;
		case MIXERCONTROL_CONTROLTYPE_BOOLEANMETER: p="BOOLEANMETER"; break;
		case MIXERCONTROL_CONTROLTYPE_SIGNEDMETER: p="SIGNEDMETER"; break;
		case MIXERCONTROL_CONTROLTYPE_PEAKMETER: p="PEAKMETER"; break;
		case MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER: p="UNSIGNEDMETER"; break;
		case MIXERCONTROL_CONTROLTYPE_BOOLEAN: p="BOOLEAN"; break;
		case MIXERCONTROL_CONTROLTYPE_ONOFF: p="ONOFF"; break;
		case MIXERCONTROL_CONTROLTYPE_MUTE: p="MUTE"; break;
		case MIXERCONTROL_CONTROLTYPE_MONO: p="MONO"; break;
		case MIXERCONTROL_CONTROLTYPE_LOUDNESS: p="LOUDNESS"; break;
		case MIXERCONTROL_CONTROLTYPE_STEREOENH: p="STEREOENH"; break;
		case MIXERCONTROL_CONTROLTYPE_BASS_BOOST: p="BASS_BOOST"; break;
		case MIXERCONTROL_CONTROLTYPE_BUTTON: p="BUTTON"; break;
		case MIXERCONTROL_CONTROLTYPE_DECIBELS: p="DECIBELS"; break;
		case MIXERCONTROL_CONTROLTYPE_SIGNED: p="SIGNED"; break;
		case MIXERCONTROL_CONTROLTYPE_UNSIGNED: p="UNSIGNED"; break;
		case MIXERCONTROL_CONTROLTYPE_PERCENT: p="PERCENT"; break;
		case MIXERCONTROL_CONTROLTYPE_SLIDER: p="SLIDER"; break;
		case MIXERCONTROL_CONTROLTYPE_PAN: p="PAN"; break;
		case MIXERCONTROL_CONTROLTYPE_QSOUNDPAN: p="QSOUNDPAN"; break;
		case MIXERCONTROL_CONTROLTYPE_FADER: p="FADER"; break;
		case MIXERCONTROL_CONTROLTYPE_VOLUME: p="VOLUME"; break;
		case MIXERCONTROL_CONTROLTYPE_BASS: p="BASS"; break;
		case MIXERCONTROL_CONTROLTYPE_TREBLE: p="TREBLE"; break;
		case MIXERCONTROL_CONTROLTYPE_EQUALIZER: p="EQUALIZER"; break;
		case MIXERCONTROL_CONTROLTYPE_SINGLESELECT: p="SINGLESELECT"; break;
		case MIXERCONTROL_CONTROLTYPE_MUX: p="MUX"; break;  
		case MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT: p="MULTIPLESELECT"; break;
		case MIXERCONTROL_CONTROLTYPE_MIXER: p="MIXER"; break;
		case MIXERCONTROL_CONTROLTYPE_MICROTIME: p="MICROTIME"; break;
		case MIXERCONTROL_CONTROLTYPE_MILLITIME: p="MILLITIME"; break;
		default: p="?"; break;
	}
	return p;
}
#endif // DXW_NOTRACES

MMRESULT WINAPI extmixerGetLineControlsA(HMIXEROBJ hmxobj, LPMIXERLINECONTROLS pmxlc, DWORD fdwControls)
{
	MMRESULT ret;
	ApiName("mixerGetLineControlsA");
	OutTraceSND("%s: hmxobj=%#x fdwControls=%#x(%s)\n", ApiRef, hmxobj, fdwControls, sfdwControls(fdwControls));
#ifndef DXW_NOTRACES
	if(IsDebugSND){
		OutTrace("< cbStruct=%d\n", pmxlc->cbStruct);
		OutTrace("< dwLineID=%#x\n", pmxlc->dwLineID);
		OutTrace("< cControls=%d\n", pmxlc->cControls);
		if(fdwControls & MIXER_GETLINECONTROLSF_ONEBYTYPE){
			OutTrace("< dwControlType=%#x(%s)\n", 
				pmxlc->dwControlType, sControlType(pmxlc->dwControlType));
		} 
		else if(fdwControls & MIXER_GETLINECONTROLSF_ONEBYID){
			OutTrace("< dwControlID =%#x\n", pmxlc->dwControlID);
		}
		else {
			OutTrace("< dwControlID=%#x\n", pmxlc->dwControlID);
		}
	}
#endif // DXW_NOTRACES

	ret = (*pmixerGetLineControlsA)(hmxobj, pmxlc, fdwControls);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

#ifndef DXW_NOTRACES
	if(IsDebugSND){
		MIXERCONTROL *mc;
		UINT count;
		OutTrace("> dwLineID=%#x\n", pmxlc->dwLineID);
		if(fdwControls & MIXER_GETLINECONTROLSF_ONEBYTYPE){
			OutTrace("> dwControlType=%#x(%s)\n", 
				pmxlc->dwControlType, sControlType(pmxlc->dwControlType));
		}
		else if(fdwControls & MIXER_GETLINECONTROLSF_ONEBYID){
			OutTrace("> dwControlID =%#x\n", pmxlc->dwControlID);
		}
		else {
			OutTrace("> dwControlID=%#x\n", pmxlc->dwControlID);
		}
		OutTrace("> cControls=%d\n", pmxlc->cControls);
		OutTrace("> cbmxctrl=%d\n", pmxlc->cbmxctrl);
		mc = pmxlc->pamxctrl;
		count = pmxlc->cControls;
		if((fdwControls & (MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_GETLINECONTROLSF_ONEBYID)) && (count > 1)) count = 1;
		for(UINT i=0; i<count; i++){
			OutTrace("> [%d]\n", i);
			OutTrace(">> cbStruct=%d\n", mc->cbStruct);
			OutTrace(">> dwControlID=%#x\n", mc->dwControlID);
			OutTrace(">> dwControlType=%#x(%s)\n", mc->dwControlType, sControlType(mc->dwControlType));
			OutTrace(">> fdwControl=%#x\n", mc->fdwControl);
			OutTrace(">> cMultipleItems=%d\n", mc->cMultipleItems);
			OutTrace(">> szShortName=\"%s\"\n", mc->szShortName);
			OutTrace(">> szName=\"%s\"\n", mc->szName);
			OutTrace(">> Bounds(Min/Max)=(%d/%d)\n", mc->Bounds.lMinimum, mc->Bounds.lMaximum);
			mc = (MIXERCONTROL *)((LPBYTE)mc + pmxlc->cbmxctrl);
		}
	}
#endif // DXW_NOTRACES

	// v2.05.57 fix: "Fighting Force" has a peculiar way to access the CD volume control:
	// it loops through all the line controls until it finds a match for the shortname with
	// the string "CD Vol", probably a legacy value that on newer Windows became the current
	// string value "Volume". This hack replaces the string so that the game can match and use it.
	if(dxw.dwFlags13 & FIGHTINGFORCEFIX){
		MIXERCONTROL *mc = pmxlc->pamxctrl;
		for(int i=0; i<(int)pmxlc->cControls; i++){
			if(mc->dwControlID == dxw.MixerCDVolumeID) {
				OutTraceSND("%s: FIGHTINGFORCEFIX szShortname: \"%s\" -> \"CD Vol\"\n", ApiRef, mc->szShortName);
				strcpy(mc->szShortName, "CD Vol");
				OutTraceSND("%s: FIGHTINGFORCEFIX szName: \"%s\" -> \"CD Audio Volume Level\"\n", ApiRef, mc->szName);
				strcpy(mc->szName, "CD Audio Volume Level");
			}
			if(mc->dwControlID == dxw.MixerCDMuteID) {
				// to do:
				//OutTraceSND("%s: FIGHTINGFORCEFIX szShortname: \"%s\" -> \"CD Mut\"\n", ApiRef, mc->szShortName);
				//strcpy(mc->szShortName, "CD Vol");
				//OutTraceSND("%s: FIGHTINGFORCEFIX szName: \"%s\" -> \"CD Audio Mute\"\n", ApiRef, mc->szName);
				//strcpy(mc->szName, "CD Audio Volume Level");
			}
			mc = (MIXERCONTROL *)((LPBYTE)mc + pmxlc->cbmxctrl);
		}
	}

	if(dxw.dwFlags13 & HIDEMUTECONTROLS) {
		MIXERCONTROLA *mc;
		UINT cNewControls = pmxlc->cControls;
		mc = pmxlc->pamxctrl;
		for(UINT i=0; i<pmxlc->cControls; i++){
			if (mc->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE){
				OutTraceSND("%s: hiding mute control %#x\n", ApiRef, mc->dwControlID);
				UINT toMove = pmxlc->cControls - i - 1;
				// shifting memory blocks 
				memcpy(mc, (LPBYTE)mc + pmxlc->cbmxctrl, (pmxlc->cbmxctrl * toMove)); 
				pmxlc->cControls--;
			}
			else {
				mc = (MIXERCONTROL *)((LPBYTE)mc + pmxlc->cbmxctrl);
			}
		}
		pmxlc->cControls = cNewControls;
	}

	return ret;
}

MMRESULT WINAPI extmixerGetDevCapsA(UINT_PTR uMxId, LPMIXERCAPSA pmxcaps, UINT cbmxcaps)
{
	MMRESULT ret;
	ApiName("mixerGetDevCapsA");

	OutTraceSND("%s: mixid=%#x cbmxcaps=%d\n", ApiRef, uMxId, cbmxcaps);

	ret = (*pmixerGetDevCapsA)(uMxId, pmxcaps, cbmxcaps);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

#if 0
	if((ret == MMSYSERR_BADDEVICEID) && (uDeviceID == 0)) {
		// pretend a mixer device exists ....
		lpCaps->wMid = 2 /*MM_CREATIVE*/;
		lpCaps->wPid = 409 /*MM_CREATIVE_SB16_MIXER*/;
		lpCaps->vDriverVersion = 1;
		strcpy(lpCaps->szPname, "dxwnd virtual mixer");
		lpCaps->wTechnology = AUXCAPS_CDAUDIO;
		lpCaps->dwSupport = AUXCAPS_VOLUME|AUXCAPS_LRVOLUME;
		lpCaps->wReserved1 = 0;

		ret = MMSYSERR_NOERROR;
	}
#endif 

#ifndef DXW_NOTRACES
	if(IsTraceSND){
		OutTrace("> wMid=%#x\n", pmxcaps->wMid);
		OutTrace("> wPid=%#x\n", pmxcaps->wPid);
		OutTrace("> vDriverVersion=%#x\n", pmxcaps->vDriverVersion);
		OutTrace("> szPname=%s\n", pmxcaps->szPname);
		OutTrace("> fdwSupport=%#x\n", pmxcaps->fdwSupport);
		OutTrace("> cDestinations=%d\n", pmxcaps->cDestinations);
	}
#endif // DXW_NOTRACES

	return ret;
}

MMRESULT WINAPI extmixerGetID(HMIXEROBJ hmxobj, UINT *puMxId, DWORD fdwId)
{
	MMRESULT ret;
	ApiName("mixerGetID");

	OutTraceSND("%s: hmxobj=%#x fdwId=%#x(%s)\n", ApiRef, hmxobj, fdwId, sfdwInfo(fdwId));

	ret = (*pmixerGetID)(hmxobj, puMxId, fdwId);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

	OutTraceSND("%s: puMxId=%#x\n", ApiRef, *puMxId);
	return ret;
}

DWORD WINAPI extmixerMessage(HMIXER hmxobj, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	MMRESULT ret;
	ApiName("mixerMessage");

	OutTraceSND("%s: hmxobj=%#x msg=%d param1=%#x param2=%#x\n", ApiRef, hmxobj, uMsg, dwParam1, dwParam2);

	ret = (*pmixerMessage)(hmxobj, uMsg, dwParam1, dwParam2);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	}
	return ret;
}
#endif // TRACEMIXER

#ifndef DXW_NOTRACES
static void dumpMixerControlDetails(LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
	OutTrace("> cbStruct=%d\n", pmxcd->cbStruct);
	OutTrace("> dwControlID=%#x\n", pmxcd->dwControlID);
	OutTrace("> cChannels=%d\n", pmxcd->cChannels);
	if(fdwDetails & MIXER_SETCONTROLDETAILSF_CUSTOM){
		OutTrace("> hwndOwner=%#x\n", pmxcd->hwndOwner);
	}
	else {
		OutTrace("> cMultipleItems=%d\n", pmxcd->cMultipleItems);
	}
	OutTrace("> cbDetails=%d\n", pmxcd->cbDetails);
	for(DWORD i=0; i<pmxcd->cbDetails; i++){
		LPMIXERCONTROLDETAILS_LISTTEXTA p = (LPMIXERCONTROLDETAILS_LISTTEXTA)pmxcd->paDetails;
		// note: p->szName holds a clean string only when MIXER_GETCONTROLDETAILSF_LISTTEXT flag is set
		if(MIXER_GETCONTROLDETAILSF_LISTTEXT & fdwDetails)
			OutTrace(">> Detail[%d]: %#x %#x \"%s\"\n", i, p->dwParam1, p->dwParam2, p->szName);
		else
			OutTrace(">> Detail[%d]: %#x %#x\n", i, p->dwParam1, p->dwParam2);
		p++;
	}
}
#endif // DXW_NOTRACES

MMRESULT WINAPI extmixerGetControlDetailsA(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
	MMRESULT ret;
	ApiName("mixerGetControlDetailsA");

#ifndef DXW_NOTRACES
	char *sDetails[] = {"MIXER", "WAVEOUT", "WAVEIN", "MIDIOUT", "MIDIIN", "AUX", "", ""};
	OutTraceSND("%s: hmxobj=%#x details=%#x(%s:%s%s)\n", 
		ApiRef, hmxobj, fdwDetails, 
		fdwDetails & MIXER_SETCONTROLDETAILSF_CUSTOM ? "CUSTOM" : "VALUE",
		fdwDetails & MIXER_OBJECTF_HANDLE ? "H" : "",
		sDetails[(fdwDetails & 0x70000000) >> 28]
		);
	if(IsTraceSND){
		OutTrace("< cbStruct=%d\n", pmxcd->cbStruct);
		OutTrace("< dwControlID=%#x\n", pmxcd->dwControlID);
		OutTrace("< cChannels=%d\n", pmxcd->cChannels);
		if(fdwDetails & MIXER_SETCONTROLDETAILSF_CUSTOM){
			OutTrace("< hwndOwner=%#x\n", pmxcd->hwndOwner);
		}
		else {
			OutTrace("< cMultipleItems=%d\n", pmxcd->cMultipleItems);
		}
	}
	OutTrace("< cbDetails=%d\n", pmxcd->cbDetails);
#endif // DXW_NOTRACES

	if((dxw.dwFlags13 & EMULATECDMIXER) && 
		((fdwDetails == 0x80000000) || (fdwDetails == 0x00000000))){
		if(pmxcd->dwControlID == dxw.MixerCDMuteID){
			*(DWORD *)(pmxcd->paDetails) = (DWORD)dxw.MixerCDMuteStatus;
			OutTraceSND("%s: EMULATE CD GET MUTE %d\n", ApiRef, dxw.MixerCDMuteStatus);
			return MMSYSERR_NOERROR;
		}
		if(pmxcd->dwControlID == dxw.MixerCDVolumeID){
			*(DWORD *)(pmxcd->paDetails) = dxw.MixerCDVolumeLevel;
			DWORD nVolume = (dxw.MixerCDVolumeLevel * 100) / 0xFFFF;
			OutTraceSND("%s: EMULATE CD GET VOLUME %d%%\n", ApiRef, nVolume);
			return MMSYSERR_NOERROR;
		}
	}

	ret = (*pmixerGetControlDetailsA)(hmxobj, pmxcd, fdwDetails);
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
		return ret;
	}

#ifndef DXW_NOTRACES
	if(IsDebugSND) dumpMixerControlDetails(pmxcd, fdwDetails);
#endif // DXW_NOTRACES

	return ret;
}

MMRESULT WINAPI extmixerSetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
	MMRESULT ret;
	ApiName("mixerSetControlDetails");

#ifndef DXW_NOTRACES
	char *sDetails[] = {"MIXER", "WAVEOUT", "WAVEIN", "MIDIOUT", "MIDIIN", "AUX", "", ""};
	OutTraceSND("%s: hmxobj=%#x details=%#x(%s:%s%s)\n", 
		ApiRef, hmxobj, fdwDetails, 
		fdwDetails & MIXER_SETCONTROLDETAILSF_CUSTOM ? "CUSTOM" : "VALUE",
		fdwDetails & MIXER_OBJECTF_HANDLE ? "H" : "",
		sDetails[(fdwDetails & 0x70000000) >> 28]
		);
	if(IsDebugSND) dumpMixerControlDetails(pmxcd, fdwDetails);
#endif // DXW_NOTRACES

	if((dxw.dwFlags13 & EMULATECDMIXER) && 
		((fdwDetails == 0x80000000) || (fdwDetails == 0x00000000))){
		if(pmxcd->dwControlID == dxw.MixerCDMuteID){
			BOOL bMute = (BOOL)*(DWORD *)(pmxcd->paDetails);
			OutTraceSND("%s: EMULATE CD SET MUTE %d\n", ApiRef, bMute);
			dxw.MixerCDMuteStatus = bMute;
			if(bMute) {
				emuSetVolume(0); 
			}
			else {
				UINT nVolume;
				nVolume = (dxw.MixerCDVolumeLevel * 100) / 0xFFFF;
				emuSetVolume(nVolume); 
			}
			return MMSYSERR_NOERROR;
		}
		if(pmxcd->dwControlID == dxw.MixerCDVolumeID){
			dxw.MixerCDVolumeLevel = *(DWORD *)(pmxcd->paDetails);
			if(!dxw.MixerCDMuteStatus){
				DWORD nVolume = (dxw.MixerCDVolumeLevel * 100) / 0xFFFF;
				OutTraceSND("%s: EMULATE CD SET VOLUME %d%%\n", ApiRef, nVolume);
				emuSetVolume(nVolume);
			}
			return MMSYSERR_NOERROR;
		}
	}

	ret = (*pmixerSetControlDetails)(hmxobj, pmxcd, fdwDetails);
#ifndef DXW_NOTRACES
	if(ret) {
		OutErrorSND("%s: ERROR res=%#x(%s)\n", ApiRef, ret, mmErrorString(ret));
	}
#endif
	return ret;
}

BOOL WINAPI extDriverCallback(DWORD_PTR dwCallback, DWORD dwFlags, HDRVR hDevice, DWORD dwMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	MMRESULT ret;
	ApiName("DriverCallback");

#ifndef DXW_NOTRACES
	OutTraceSND("%s: callback=%#x flags=%#x device=%#x msg=%#x user=%#x params=%#x,%#x\n",
		ApiRef, 
		dwCallback, dwFlags, hDevice, dwMsg, dwUser, dwParam1, dwParam2
		);
#endif // DXW_NOTRACES
	ret = (*pDriverCallback)(dwCallback, dwFlags, hDevice, dwMsg, dwUser, dwParam1, dwParam2);
#ifndef DXW_NOTRACES
	if(!ret){
		OutErrorSND("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
#endif
	return ret;
}

MMRESULT WINAPI extmmioSetInfo(HMMIO hmmio, LPCMMIOINFO pmmioinfo, UINT fuInfo)
{
	MMRESULT ret;
	ApiName("mmioSetInfo");

#ifndef DXW_NOTRACES
	OutTraceSND("%s: hmmio=%#x fuInfo=%#x pmmioinfo=%#x\n", ApiRef, hmmio, fuInfo, pmmioinfo);
#endif // DXW_NOTRACES
	ret = (*pmmioSetInfo)(hmmio, pmmioinfo, fuInfo);
#ifndef DXW_NOTRACES
	if(ret != 0){
		OutErrorSND("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	else {
		if(IsDebugSND && pmmioinfo) dumpMmioInfo(pmmioinfo);
	}
#endif
	return ret;
}

MMRESULT WINAPI extmmioGetInfo(HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuInfo)
{
	MMRESULT ret;
	ApiName("mmioGetInfo");

#ifndef DXW_NOTRACES
	OutTraceSND("%s: hmmio=%#x fuInfo=%#x pmmioinfo=%#x\n", ApiRef, hmmio, fuInfo, pmmioinfo);
#endif // DXW_NOTRACES
	ret = (*pmmioGetInfo)(hmmio, pmmioinfo, fuInfo);
#ifndef DXW_NOTRACES
	if(ret != 0){
		OutErrorSND("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	else {
		if(IsDebugSND && pmmioinfo) dumpMmioInfo((LPCMMIOINFO)pmmioinfo);
	}
#endif
	return ret;
}

MMRESULT WINAPI extmmioClose(HMMIO hmmio, UINT  fuClose)
{
	MMRESULT ret;
	ApiName("mmioClose");

#ifndef DXW_NOTRACES
	OutTraceSND("%s: hmmio=%#x fuClose=%#x\n", ApiRef, hmmio, fuClose);
#endif // DXW_NOTRACES
	ret = (*pmmioClose)(hmmio, fuClose);
#ifndef DXW_NOTRACES
	if(ret != 0){
		OutErrorSND("%s: ERROR ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
#endif
	return ret;
}
