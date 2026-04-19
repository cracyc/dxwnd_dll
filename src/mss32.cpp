#define  _CRT_SECURE_NO_WARNINGS

#define _MODULE "mss32"

#include <windows.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"

typedef DWORD (WINAPI *AIL_redbook_track_info_Type)(DWORD, DWORD, DWORD *, DWORD *);
AIL_redbook_track_info_Type pAIL_redbook_track_info;
DWORD WINAPI extAIL_redbook_track_info(DWORD, DWORD, DWORD *, DWORD *);

typedef DWORD (WINAPI *AIL_redbook_tracks_Type)(DWORD);
AIL_redbook_tracks_Type pAIL_redbook_tracks;
DWORD WINAPI extAIL_redbook_tracks(DWORD);

typedef DWORD (WINAPI *AIL_redbook_status_Type)(DWORD);
AIL_redbook_status_Type pAIL_redbook_status;
DWORD WINAPI extAIL_redbook_status(DWORD);

typedef DWORD (WINAPI *AIL_redbook_play_Type)(DWORD, DWORD, DWORD);
AIL_redbook_play_Type pAIL_redbook_play;
DWORD WINAPI extAIL_redbook_play(DWORD, DWORD, DWORD);

typedef DWORD (WINAPI *AIL_redbook_set_volume_Type)(DWORD, DWORD);
AIL_redbook_set_volume_Type pAIL_redbook_set_volume;
DWORD WINAPI extAIL_redbook_set_volume(DWORD, DWORD);

typedef DWORD (WINAPI *AIL_set_digital_master_volume_Type)(DWORD, DWORD);
AIL_set_digital_master_volume_Type pAIL_set_digital_master_volume;
DWORD WINAPI extAIL_set_digital_master_volume(DWORD, DWORD);

static HookEntryEx_Type HooksRedbook[]={
	{HOOK_IAT_CANDIDATE, 0x0000, "_AIL_redbook_tracks@4", (FARPROC)NULL, (FARPROC *)&pAIL_redbook_tracks, (FARPROC)extAIL_redbook_tracks},
	{HOOK_IAT_CANDIDATE, 0x0000, "_AIL_redbook_status@4", (FARPROC)NULL, (FARPROC *)&pAIL_redbook_status, (FARPROC)extAIL_redbook_status},
	{HOOK_IAT_CANDIDATE, 0x0000, "_AIL_redbook_track_info@16", (FARPROC)NULL, (FARPROC *)&pAIL_redbook_track_info, (FARPROC)extAIL_redbook_track_info},
	{HOOK_IAT_CANDIDATE, 0x0000, "_AIL_redbook_play@12", (FARPROC)NULL, (FARPROC *)&pAIL_redbook_play, (FARPROC)extAIL_redbook_play},
	{HOOK_IAT_CANDIDATE, 0x0000, "_AIL_redbook_set_volume@8", (FARPROC)NULL, (FARPROC *)&pAIL_redbook_set_volume, (FARPROC)extAIL_redbook_set_volume},
	{HOOK_IAT_CANDIDATE, 0x0000, "_AIL_set_digital_master_volume@8", (FARPROC)NULL, (FARPROC *)&pAIL_set_digital_master_volume, (FARPROC)extAIL_set_digital_master_volume},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

DWORD gStart, gEnd, dTrack;

DWORD WINAPI extAIL_redbook_status(DWORD id)
{
	DWORD ret;
	ApiName("AIL_redbook_status");

	OutTrace("%s: id=%#x\n", ApiRef, id);
	ret = (*pAIL_redbook_status)(id);
#ifndef DXW_NOTRACES
	char *sStatus[] = {"ERROR", "PLAYING", "PAUSED", "STOPPED", "???"};
	OutTrace("%s: ret(status)=%d(%s)\n", ApiRef, ret, (ret > 3) ? sStatus[4] : sStatus[ret]);
#endif // DXW_NOTRACES
	return ret;
}

DWORD WINAPI extAIL_redbook_tracks(DWORD id)
{
	DWORD ret;
	ApiName("AIL_redbook_tracks");

	OutTrace("%s: id=%#x\n", ApiRef, id);
	ret = (*pAIL_redbook_tracks)(id);
	OutTrace("%s: ret(tracks)=%d\n", ApiRef, ret);
	return ret;
}

DWORD WINAPI extAIL_redbook_track_info(DWORD id, DWORD dwTrack, DWORD *start, DWORD *end)
{
	DWORD ret;
	ApiName("AIL_redbook_track_info");

	OutTrace("%s: id=%#x track=%d\n", ApiRef, id, dwTrack);
	ret = (*pAIL_redbook_track_info)(id, dwTrack, start, end);
	OutTrace("%s: ret=%#x start=%#x end=%#x\n", ApiRef, ret, *start, *end);
	if(dxw.dwFlags17 & FIXAILBUG){
		gStart = *start;
		gEnd = *end;
		OutTrace("%s: FIXAILBUG REGISTERED start=%#x end=%#x\n", ApiRef, start, end);
	}
	return ret;
}

DWORD WINAPI extAIL_redbook_play(DWORD id, DWORD start, DWORD end)
{
	DWORD ret;
	ApiName("AIL_redbook_play");

	OutTrace("%s: id=%#x start=%#x end=%#x\n", ApiRef, id, start, end);
	if(dxw.dwFlags17 & FIXAILBUG){
		start = gStart;
		end = gEnd;
		OutTrace("%s: FIXAILBUG FIXED start=%#x end=%#x\n", ApiRef, start, end);
	}
	ret = (*pAIL_redbook_play)(id, start, end);
	OutTrace("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

extern void (*pplr_setvolume)(int);
DWORD WINAPI extAIL_redbook_set_volume(DWORD mssHandle, DWORD volume)
{
	DWORD ret;
	ApiName("AIL_redbook_set_volume");

	OutTrace("%s: h=%#x vol=%#x\n", ApiRef, mssHandle, volume);
	if(dxw.dwFlags8 & VIRTUALCDAUDIO){
		int cdaVolume;
		if(dxw.dwFlags9 & LOCKVOLUME) {
			cdaVolume = dxw.CDAVolume;
			OutTraceSND("%s: LOCKVOLUME CDA volume=%#x(%d%%)\n", ApiRef, volume, cdaVolume);
			return volume;
		}
		// max volume = 0x7F
		// max volume = 0x3F
		if(volume > 0x3F) volume = 0x3F;
		cdaVolume = (volume * 100) / 0x3F;
		OutTraceSND("%s: setting CDA volume=%#x(%d%%)\n", ApiRef, volume, cdaVolume);
		if(!pplr_setvolume) return 0;
		(*pplr_setvolume)(cdaVolume);
		return volume;
	}
	ret = (*pAIL_redbook_set_volume)(mssHandle, volume);
	OutTrace("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

DWORD WINAPI extAIL_set_digital_master_volume(DWORD mssHandle, DWORD volume)
{
	DWORD ret;
	ApiName("AIL_set_digital_master_volume");

	OutTrace("%s: h=%#x vol=%#x\n", ApiRef, mssHandle, volume);
	ret = (*pAIL_set_digital_master_volume)(mssHandle, volume);
	OutTrace("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

void Hook_AIL_redbook(HMODULE hModule)
{
	HookLibraryEx(hModule, HooksRedbook, "mss32.dll");
}
