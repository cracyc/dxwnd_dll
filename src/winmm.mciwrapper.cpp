#define _CRT_SECURE_NO_WARNINGS 
#define _CRT_NON_CONFORMING_SWPRINTFS

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
#include <mciavi.h>
#include "mciplayer.h"
#include "Digitalv.h"

HANDLE hPlayer = NULL;
int cdVolume = 100;

extern BOOL IsWithinMCICall;
extern MCIERROR WINAPI emumciSendCommand(BOOL, MCIDEVICEID, UINT, DWORD, DWORD_PTR);
extern BOOL gb_RefreshSemaphore;
extern int MapMsecToTTMMSS(int);

#ifndef MCI_SETAUDIO
#define MCI_SETAUDIO            0x0873
#endif

struct play_info movie_info = { 0, 0, 0, 0 };
extern char *mmErrorString(MMRESULT);

BOOL checkGap(char *api, unsigned int msec)
{
	unsigned int gap = 2000;
	BOOL ret;
	if(dxw.dwFlags11 & HACKMCIFRAMES){
		char hackPath[MAX_PATH];
		sprintf(hackPath, "%s\\Music\\mcihack.txt", dxw.cdAudioPath);
		gap = (*pGetPrivateProfileIntA)("msec", "gap", 2000, hackPath);
	}
	ret = (msec < gap);
	if(ret) OutTraceSND("%s: msec value OUTOFRANGE msec=%d gap=%d\n", api, msec, gap);
	return ret;
}

DWORD HandleStopCommand()
{
	player_set_status(MCI_STOP);
	if(dxw.dwFlags15 & EMULATEVISTAMCI){
		mciEmu.dwCurrentTrack = mciEmu.dwFirstTrack;
		mciEmu.dwSeekedTrack = mciEmu.dwFirstTrack;
	}

	if(dxw.dwFlags15 & EMULATEWIN9XMCI) {
		dprintf("> Emulate Win9X MCI\n"); 
		mciEmu.dwTargetTrack = mciEmu.dwLastTrack;
	}
	return 0; // no errors here
}

void start_player()
{
	if (hPlayer == NULL){
		// v2.04.99: CreateThread for CD player, the defauld stack size is not enough for "Speedboat Attack"
		// that crashes for stack overflow. 50K as stack size were ok, but to avoid risks we set 100K
        hPlayer = CreateThread(NULL, 100000, (LPTHREAD_START_ROUTINE)player_main, NULL, 0, NULL);
		if(!hPlayer) {
			OutTraceE("> Error starting player_main audio CD emulator\n");
		}
	}
}

DWORD TTMMSSToAbsSec(DWORD ttmmss)
{
	DWORD sec = 0;
	sec = tracks[TTMMSS_TRACKNO(ttmmss)].position;
	sec += TTMMSS_OFFSET(ttmmss);
	return sec;
}

char *ExplainMCICommands(DWORD c)
{
	switch(c){
		case MCI_OPEN: return "MCI_OPEN";
		case MCI_CLOSE: return "MCI_CLOSE";
		case MCI_ESCAPE: return "MCI_ESCAPE";
		case MCI_PLAY: return "MCI_PLAY";
		case MCI_SEEK: return "MCI_SEEK";
		case MCI_STOP: return "MCI_STOP";
		case MCI_PAUSE: return "MCI_PAUSE";
		case MCI_INFO: return "MCI_INFO";
		case MCI_GETDEVCAPS: return "MCI_GETDEVCAPS";
		case MCI_SPIN: return "MCI_SPIN";
		case MCI_SET: return "MCI_SET";
		case MCI_STEP: return "MCI_STEP";
		case MCI_RECORD: return "MCI_RECORD";
		case MCI_SYSINFO: return "MCI_SYSINFO";
		case MCI_BREAK: return "MCI_BREAK";
		case MCI_SAVE: return "MCI_SAVE";
		case MCI_STATUS: return "MCI_STATUS";
		case MCI_CUE: return "MCI_CUE";
		case MCI_REALIZE: return "MCI_REALIZE";
		case MCI_WINDOW: return "MCI_WINDOW";
		case MCI_PUT: return "MCI_PUT";
		case MCI_WHERE: return "MCI_WHERE";
		case MCI_FREEZE: return "MCI_FREEZE";
		case MCI_UNFREEZE: return "MCI_UNFREEZE";
		case MCI_LOAD: return "MCI_LOAD";
		case MCI_CUT: return "MCI_CUT";
		case MCI_COPY: return "MCI_COPY";
		case MCI_PASTE: return "MCI_PASTE";
		case MCI_UPDATE: return "MCI_UPDATE";
		case MCI_RESUME: return "MCI_RESUME";
		case MCI_DELETE: return "MCI_DELETE";
	}
	return "???";
}

char *ExplainMCIFlags(DWORD cmd, DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb,"MCI_", eblen);
	// common flags
	if (c & MCI_NOTIFY) strscat(eb, eblen, "NOTIFY+");
	if (c & MCI_WAIT) strscat(eb, eblen, "WAIT+");
	if (c & MCI_FROM) strscat(eb, eblen, "FROM+");
	if (c & MCI_TO) strscat(eb, eblen, "TO+");
	if (c & MCI_TRACK) strscat(eb, eblen, "TRACK+");
	if (c & MCI_TEST) strscat(eb, eblen, "TEST+");
	switch(cmd){
	case MCI_OPEN:
		if (c & MCI_OPEN_SHAREABLE) strscat(eb, eblen, "OPEN_SHAREABLE+");
		if (c & MCI_OPEN_ELEMENT) strscat(eb, eblen, "OPEN_ELEMENT+");
		if (c & MCI_OPEN_ALIAS) strscat(eb, eblen, "OPEN_ALIAS+");
		if (c & MCI_OPEN_ELEMENT_ID) strscat(eb, eblen, "OPEN_ELEMENT_ID+");
		if (c & MCI_OPEN_TYPE_ID) strscat(eb, eblen, "OPEN_TYPE_ID+");
		if (c & MCI_OPEN_TYPE) strscat(eb, eblen, "OPEN_TYPE+");
		if (c & MCI_WAVE_OPEN_BUFFER) strscat(eb, eblen, "WAVE_OPEN_BUFFER+");
		break;
	case MCI_SEEK:
		if (c & MCI_TO) strscat(eb, eblen, "TO+");
		if (c & MCI_SEEK_TO_START) strscat(eb, eblen, "SEEK_TO_START+");
		if (c & MCI_SEEK_TO_END) strscat(eb, eblen, "SEEK_TO_END+");
		if (c & MCI_STATUS_START) strscat(eb, eblen, "STATUS_START+");
		if (c & MCI_VD_SEEK_REVERSE) strscat(eb, eblen, "VD_SEEK_REVERSE+");
		break;
	case MCI_STATUS:
		if (c & MCI_STATUS_ITEM) strscat(eb, eblen, "STATUS_ITEM+");
		if (c & MCI_STATUS_START) strscat(eb, eblen, "STATUS_START+");
		break;
	case MCI_INFO:
		if (c & MCI_INFO_PRODUCT) strscat(eb, eblen, "INFO_PRODUCT+");
		if (c & MCI_INFO_FILE) strscat(eb, eblen, "INFO_FILE+");
		if (c & MCI_INFO_MEDIA_UPC) strscat(eb, eblen, "INFO_MEDIA_UPC+");
		if (c & MCI_INFO_MEDIA_IDENTITY) strscat(eb, eblen, "INFO_MEDIA_IDENTITY+");
		if (c & MCI_INFO_NAME) strscat(eb, eblen, "INFO_NAME+");
		if (c & MCI_INFO_COPYRIGHT) strscat(eb, eblen, "INFO_COPYRIGHT+");
		break;
	case MCI_GETDEVCAPS:
		if (c & MCI_VD_GETDEVCAPS_CLV) strscat(eb, eblen, "VD_GETDEVCAPS_CLV+");
		if (c & MCI_VD_GETDEVCAPS_CAV) strscat(eb, eblen, "VD_GETDEVCAPS_CAV+");
		if (c & MCI_GETDEVCAPS_ITEM) strscat(eb, eblen, "GETDEVCAPS_ITEM+");
		break;
	case MCI_SYSINFO:
		if (c & MCI_SYSINFO_QUANTITY) strscat(eb, eblen, "SYSINFO_QUANTITY+");
		if (c & MCI_SYSINFO_OPEN) strscat(eb, eblen, "SYSINFO_OPEN+");
		if (c & MCI_SYSINFO_NAME) strscat(eb, eblen, "SYSINFO_NAME+");
		if (c & MCI_SYSINFO_INSTALLNAME) strscat(eb, eblen, "SYSINFO_INSTALLNAME+");
		break;
	case MCI_SET:
		if (c & MCI_SET_DOOR_OPEN) strscat(eb, eblen, "SET_DOOR_OPEN+");
		if (c & MCI_SET_DOOR_CLOSED) strscat(eb, eblen, "SET_DOOR_CLOSED+");
		if (c & MCI_SET_TIME_FORMAT) strscat(eb, eblen, "SET_TIME_FORMAT+");
		if (c & MCI_SET_AUDIO) strscat(eb, eblen, "SET_AUDIO+");
		if (c & MCI_SET_VIDEO) strscat(eb, eblen, "SET_VIDEO+");
		if (c & MCI_SET_ON) strscat(eb, eblen, "SET_ON+");
		if (c & MCI_SET_OFF) strscat(eb, eblen, "SET_OFF+");
		if (c & MCI_SEQ_SET_TEMPO) strscat(eb, eblen, "SEQ_SET_TEMPO+");
		if (c & MCI_SEQ_SET_PORT) strscat(eb, eblen, "SEQ_SET_PORT+");
		if (c & MCI_SEQ_SET_SLAVE) strscat(eb, eblen, "SEQ_SET_SLAVE+");
		if (c & MCI_SEQ_SET_MASTER) strscat(eb, eblen, "SEQ_SET_MASTER+");
		if (c & MCI_SEQ_SET_OFFSET) strscat(eb, eblen, "SEQ_SET_OFFSET+");
		break;
	case MCI_BREAK:
		if (c & MCI_BREAK_KEY) strscat(eb, eblen, "BREAK_KEY+");
		if (c & MCI_BREAK_HWND) strscat(eb, eblen, "BREAK_HWND+");
		if (c & MCI_BREAK_OFF) strscat(eb, eblen, "BREAK_OFF+");
		break;
	case MCI_RECORD:
		if (c & MCI_RECORD_INSERT) strscat(eb, eblen, "RECORD_INSERT+");
		if (c & MCI_RECORD_OVERWRITE) strscat(eb, eblen, "RECORD_OVERWRITE+");
		break;
	case MCI_SAVE:
		if (c & MCI_SAVE_FILE) strscat(eb, eblen, "SAVE_FILE+");
		break;
	case MCI_LOAD:
		if (c & MCI_LOAD_FILE) strscat(eb, eblen, "SAVE_FILE+");
		break;
	case MCI_PLAY:
		if (c & MCI_VD_PLAY_REVERSE) strscat(eb, eblen, "PLAY_REVERSE+");
		if (c & MCI_VD_PLAY_FAST) strscat(eb, eblen, "PLAY_FAST+");
		if (c & MCI_VD_PLAY_SPEED) strscat(eb, eblen, "PLAY_SPEED+");
		if (c & MCI_VD_PLAY_SCAN) strscat(eb, eblen, "PLAY_SCAN+");
		if (c & MCI_VD_PLAY_SLOW) strscat(eb, eblen, "PLAY_SLOW+");
		if (c & MCI_MCIAVI_PLAY_WINDOW) strscat(eb, eblen, "PLAY_WINDOW+");
		if (c & MCI_MCIAVI_PLAY_FULLSCREEN) strscat(eb, eblen, "PLAY_FULLSCREEN+");
		if (c & MCI_MCIAVI_PLAY_FULLBY2) strscat(eb, eblen, "PLAY_FULLBY2+");
		break;
	case MCI_STEP:
		if (c & MCI_VD_STEP_FRAMES) strscat(eb, eblen, "VD_STEP_FRAMES+");
		if (c & MCI_VD_STEP_REVERSE) strscat(eb, eblen, "VD_STEP_REVERSE+");
		break;
	case MCI_WINDOW:
		if (c & MCI_ANIM_WINDOW_HWND) strscat(eb, eblen, "WINDOW_HWND+");
		if (c & MCI_ANIM_WINDOW_STATE) strscat(eb, eblen, "WINDOW_STATE+");
		if (c & MCI_ANIM_WINDOW_TEXT) strscat(eb, eblen, "WINDOW_TEXT+");
		if (c & MCI_ANIM_WINDOW_ENABLE_STRETCH) strscat(eb, eblen, "WINDOW_ENABLE_STRETCH+");
		if (c & MCI_ANIM_WINDOW_DISABLE_STRETCH) strscat(eb, eblen, "WINDOW_DISABLE_STRETCH+");
		break;
	case MCI_PUT:
		if (c & MCI_ANIM_RECT) strscat(eb, eblen, "RECT+");
		if (c & MCI_ANIM_PUT_SOURCE) strscat(eb, eblen, "PUT_SOURCE+");
		if (c & MCI_ANIM_PUT_DESTINATION) strscat(eb, eblen, "PUT_DESTINATION+");
		break;
	case MCI_WHERE:
		if (c & MCI_ANIM_WHERE_SOURCE) strscat(eb, eblen, "WHERE_SOURCE+");
		if (c & MCI_ANIM_WHERE_DESTINATION) strscat(eb, eblen, "WHERE_DESTINATION+");
		if (c & MCI_DGV_WHERE_FRAME) strscat(eb, eblen, "WHERE_FRAME+");
		if (c & MCI_DGV_WHERE_VIDEO) strscat(eb, eblen, "WHERE_VIDEO+");
		if (c & MCI_DGV_WHERE_WINDOW) strscat(eb, eblen, "WHERE_WINDOW+");
		if (c & MCI_DGV_WHERE_MAX) strscat(eb, eblen, "WHERE_MAX+");
		break;
	case MCI_UPDATE:
		if (c & MCI_ANIM_UPDATE_HDC) strscat(eb, eblen, "ANIM_UPDATE_HDC+");
		break;
	}
	/*
	if (c & MCI_OVLY_OPEN_WS) strscat(eb, eblen, "OVLY_OPEN_WS+");
	if (c & MCI_OVLY_OPEN_PARENT) strscat(eb, eblen, "OVLY_OPEN_PARENT+");
	if (c & MCI_OVLY_STATUS_HWND) strscat(eb, eblen, "OVLY_STATUS_HWND+");
	if (c & MCI_OVLY_STATUS_STRETCH) strscat(eb, eblen, "OVLY_STATUS_STRETCH+");
	if (c & MCI_OVLY_INFO_TEXT) strscat(eb, eblen, "OVLY_INFO_TEXT+");
	if (c & MCI_OVLY_WINDOW_HWND) strscat(eb, eblen, "OVLY_WINDOW_HWND+");
	if (c & MCI_OVLY_WINDOW_STATE) strscat(eb, eblen, "OVLY_WINDOW_STATE+");
	if (c & MCI_OVLY_WINDOW_TEXT) strscat(eb, eblen, "OVLY_WINDOW_TEXT+");
	if (c & MCI_OVLY_WINDOW_ENABLE_STRETCH) strscat(eb, eblen, "OVLY_WINDOW_ENABLE_STRETCH+");
	if (c & MCI_OVLY_WINDOW_DISABLE_STRETCH) strscat(eb, eblen, "OVLY_WINDOW_DISABLE_STRETCH+");
	if (c & MCI_OVLY_WINDOW_DEFAULT) strscat(eb, eblen, "OVLY_WINDOW_DEFAULT+");
	if (c & MCI_OVLY_RECT) strscat(eb, eblen, "OVLY_RECT+");
	if (c & MCI_OVLY_PUT_SOURCE) strscat(eb, eblen, "OVLY_PUT_SOURCE+");
	if (c & MCI_OVLY_PUT_DESTINATION) strscat(eb, eblen, "OVLY_PUT_DESTINATION+");
	if (c & MCI_OVLY_PUT_FRAME) strscat(eb, eblen, "OVLY_PUT_FRAME+");
	if (c & MCI_OVLY_PUT_VIDEO) strscat(eb, eblen, "OVLY_PUT_VIDEO+");
	if (c & MCI_OVLY_WHERE_SOURCE) strscat(eb, eblen, "OVLY_WHERE_SOURCE+");
	if (c & MCI_OVLY_WHERE_DESTINATION) strscat(eb, eblen, "OVLY_WHERE_DESTINATION+");
	if (c & MCI_OVLY_WHERE_FRAME) strscat(eb, eblen, "OVLY_WHERE_FRAME+");
	if (c & MCI_OVLY_WHERE_VIDEO) strscat(eb, eblen, "OVLY_WHERE_VIDEO+");
	*/
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("MCI_")) eb[l-1]=0; // delete last '+' if any
		else strncpy(eb, "NULL", eblen);
	}
	return(eb);
}

