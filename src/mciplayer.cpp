// =========================================================================== //
// ===                          CD audio emulation                         === //
// =========================================================================== //

/*
* Copyright (c) 2012 Toni Spets <toni.spets@iki.fi>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
* Copyright (c) 2017 gho
*
* Yeah, same for me too!
*/

#define _CRT_SECURE_NO_WARNINGS 
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _MODULE "dxwnd"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "mciplayer.h"
#include <stdio.h>

struct play_info info = { 0, 0, 0, 0 };

mciEmuDescriptor mciEmu;
struct track_info tracks[MAX_TRACKS];

typedef int (*plr_pump_type)(void);
typedef void (*plr_stop_type)(void);
typedef int (*plr_play_type)(int);
typedef void (*plr_setvolume_type)(int);
typedef int (*plr_getvolume_type)(void);
typedef int (*plr_seek_type)(int);
typedef int (*plr_tell_type)(void);
typedef int (*plr_load_type)(track_info *, int);
typedef void (*plr_change_type)(int);
typedef int (*plr_setPath_type)(char *);
typedef int (*plr_rip_type)(char);
typedef int (*plr_readtoc_type)(char, track_info *, int);

int (*pplr_pump)(void) = NULL; // initialized to NULL as a bool to make the initial player loading
void (*pplr_stop)(void);
int (*pplr_play)(int);
void (*pplr_setvolume)(int);
int (*pplr_getvolume)(void);
int (*pplr_seek)(int);
int (*pplr_tell)(void);
int (*pplr_load)(track_info *, int);
void (*pplr_change)(int);
int (*pplr_setPath)(char *);
int (*pplr_rip)(char);
int (*pplr_readtoc)(char, track_info *, int);

static char FindCDADrive(void)
{
	ApiName("FindCDADrive");
	char rootPath[8];
	strcpy(rootPath, "?:\\");
	if(!pGetDriveTypeA) pGetDriveTypeA = GetDriveTypeA;
	for(char drive = 'D'; drive <= 'Z'; drive++){
		rootPath[0] = drive;
		UINT type = (*pGetDriveTypeA)(rootPath);
		if(type == DRIVE_CDROM) {
			OutDebugSND("%s: found drive %s type CDROM\n", ApiRef, rootPath);
			return drive;
		}
		else {
			OutDebugSND("%s: test drive %s type %d - skip\n", ApiRef, rootPath, type);
		}
	}
	OutDebugSND("%s: found no CDROM drive - return '?'\n", ApiRef);
	return '?';
}

int player_init()
{
    unsigned int position = 0;

	memset(&mciEmu, 0, sizeof(mciEmu));
	mciEmu.dwFirstTrack = 0;
	mciEmu.dwLastTrack = 0;
	//mciEmu.dwSeekedTrack = 0;
	mciEmu.dwTargetTrack = 0;
	mciEmu.playing = 0;
	mciEmu.paused = 0;
	mciEmu.dooropened = 0;
	// v2.05.86 fix: initial state is begin of track#1 in any case
	mciEmu.dwCurrentTrack = TTMMSS_ENCODE(1, 0, 0); // begin of track #1
	mciEmu.dwSeekedTrack = TTMMSS_ENCODE(1, 0, 0); // begin of track #1

	// v2.04.97: strange, but ... true? 
	// The default time format is MCI_FORMAT_MSF after a mciSendString("open cdaudio") call,
	// but it is MCI_FORMAT_TMSF after a mciSendCommand(MCI_OPEN) call.
	mciEmu.dwTimeFormat = MCI_FORMAT_UNDEFINED; // v2.04.97: strange, but ... 

	dprintf("dxwplay: searching tracks...\n");

    memset(tracks, 0, sizeof tracks);

	HINSTANCE hplay;

	char inipath[MAX_PATH];
	GetModuleFileName(GetModuleHandle("dxwnd"), inipath, MAX_PATH);
	inipath[strlen(inipath)-strlen("dxwnd.dll")] = 0; // terminate the string just before "dxwnd.dll"
	strcat(inipath, "dxwplay.dll");
	hplay = (*pLoadLibraryA)((LPCSTR)inipath);

	if(!hplay) {
		OutErrorSND("dxwplay: error loading audio CD emulator path=%s err=%d\n", inipath, GetLastError());
		return 0;
	}
	pplr_load =	(plr_load_type)(*pGetProcAddress)(hplay, "plr_load");
	pplr_pump =	(plr_pump_type)(*pGetProcAddress)(hplay, "plr_pump");
	pplr_stop =	(plr_stop_type)(*pGetProcAddress)(hplay, "plr_stop");
	pplr_play =	(plr_play_type)(*pGetProcAddress)(hplay, "plr_play");
	pplr_setvolume =(plr_setvolume_type)(*pGetProcAddress)(hplay, "plr_setvolume");
	pplr_getvolume =(plr_getvolume_type)(*pGetProcAddress)(hplay, "plr_getvolume");
	pplr_seek =(plr_seek_type)(*pGetProcAddress)(hplay, "plr_seek");
	pplr_tell =(plr_tell_type)(*pGetProcAddress)(hplay, "plr_tell");
	pplr_change = (plr_change_type)(*pGetProcAddress)(hplay, "plr_change");
	pplr_setPath = (plr_setPath_type)(*pGetProcAddress)(hplay, "plr_setPath");
	pplr_rip = (plr_rip_type)(*pGetProcAddress)(hplay, "plr_rip");
	pplr_readtoc = (plr_readtoc_type)(*pGetProcAddress)(hplay, "plr_readtoc");
	// pplr_setPath, pplr_seek & pplr_tell: we can survive without ....
	if(!(pplr_load && pplr_pump && pplr_stop && pplr_play && pplr_setvolume && pplr_getvolume && pplr_change)) {
		OutErrorSND("dxwplay: error getting player audio CD functions\n");
		return 0;
	}
	if((dxw.dwFlags15 & CDAUTOMATICRIP) && !pplr_rip){
		OutErrorSND("dxwplay: error getting player plr_rip function\n");
		return 0;
	}
	else if((dxw.dwFlags15 & PLAYFROMCD) && !pplr_readtoc){
		OutErrorSND("dxwplay: error getting player plr_readtoc function\n");
		return 0;
	}

	// v2.05.72
	if(dxw.dwFlags14 & SETCDAUDIOPATH) {
		(*pplr_setPath)(dxw.cdAudioPath);
		OutTraceSND("dxwplay: set cd audio path=%s\n", dxw.cdAudioPath);
	}

	// v2.05.79
	if(dxw.dwFlags15 & CDAUTOMATICRIP){
		if(dxw.CDADrive == '?') dxw.CDADrive = FindCDADrive();
		OutTraceSND("dxwplay: rip CD drive %c:\n", dxw.CDADrive);
		mciEmu.dwNumTracks = (*pplr_rip)(dxw.CDADrive);
		if(mciEmu.dwNumTracks == -1){
			MessageBox(0, "Warning: Can't overwrite existing audio tracks", 
				"DxWnd", MB_OK | MB_ICONEXCLAMATION);
		}
		mciEmu.dwNumTracks = (*pplr_load)(tracks, MAX_TRACKS);
	}
	else if(dxw.dwFlags15 & PLAYFROMCD){
		if(dxw.CDADrive == '?') dxw.CDADrive = FindCDADrive();
		OutTraceSND("dxwplay: read TOC from CD drive %c:\n", dxw.CDADrive);
		mciEmu.dwNumTracks = (*pplr_readtoc)(dxw.CDADrive, tracks, MAX_TRACKS);
	}
	else {
		mciEmu.dwNumTracks = (*pplr_load)(tracks, MAX_TRACKS);
	}
	OutTraceSND("dxwplay: found %d tracks\n", mciEmu.dwNumTracks);

	// BEWARE: this track array is indexed from 1 instead than 0, so the first slot is unused
	// and all the cycles must go from 1 to dwNumTracks both included.
    for (DWORD i = 1; i <= mciEmu.dwNumTracks; i++) {
		if(tracks[i].type == MCI_AUDIO_TRACK) {
			if (mciEmu.dwFirstTrack == 0) mciEmu.dwFirstTrack = TTMMSS_ENCODE(i, 0, 0);	
			if (mciEmu.dwSeekedTrack == 0) mciEmu.dwSeekedTrack = mciEmu.dwFirstTrack;
			mciEmu.dwLastTrack = TTMMSS_ENCODE(i, tracks[i].length / 60, tracks[i].length % 60);
			mciEmu.dwTargetTrack = mciEmu.dwLastTrack;
		}
	}

    for (DWORD i = 1; i <= mciEmu.dwNumTracks; i++) {
		dprintf("> Track %02d: [%c] %02d:%02d (%04d sec) @ %d sec\n", 
			i, 
			(tracks[i].type == MCI_AUDIO_TRACK) ? 'A' : 'D', tracks[i].length / 60, 
			tracks[i].length % 60, 
			tracks[i].length,
			tracks[i].position);
	}

    dprintf("Emulating total of %d CD tracks. First=%02d.%02d.%02d Last=%02d.%02d.%02d\n", 
		mciEmu.dwNumTracks, 
		mciEmu.dwFirstTrack>>16, (mciEmu.dwFirstTrack>>8)&0xFF, mciEmu.dwFirstTrack&0xFF,
		mciEmu.dwLastTrack>>16,  (mciEmu.dwLastTrack>>8)&0xFF,  mciEmu.dwLastTrack&0xFF);

	if(dxw.dwFlags10 & SETCDVOLUME) (*pplr_setvolume)(dxw.CDAVolume);

	return mciEmu.dwNumTracks;
}

void player_change(void)
{
	if(dxw.AudioCDIndex == GetHookInfo()->CDIndex) return;
	if(!pplr_change) return;
	dxw.AudioCDIndex = GetHookInfo()->CDIndex;
	// set a layout permanence of 2 seconds ...
	dxw.ShowCDChanger();
	dprintf("dxwplay: changing to CD%02d\n", dxw.AudioCDIndex+1);
	(*pplr_change)(dxw.AudioCDIndex);
	memset(&mciEmu, 0, sizeof(mciEmu));
	mciEmu.dwFirstTrack = 0;
	mciEmu.dwLastTrack = 0;
	mciEmu.dwSeekedTrack = 0;
	mciEmu.dwTargetTrack = 0;
	mciEmu.playing = 0;
	mciEmu.paused = 0;
	mciEmu.dooropened = 0;

	dprintf("dxwplay: searching tracks...\n");

    memset(tracks, 0, sizeof tracks);
	mciEmu.dwNumTracks = (*pplr_load)(tracks, MAX_TRACKS);
	OutTraceSND("dxwplay: found %d tracks\n", mciEmu.dwNumTracks);
    for (DWORD i = 1; i <= mciEmu.dwNumTracks; i++) {
		if(tracks[i].type == MCI_AUDIO_TRACK) {
			if (mciEmu.dwFirstTrack == 0) mciEmu.dwFirstTrack = TTMMSS_ENCODE(i, 0, 0);	
			if (mciEmu.dwSeekedTrack == 0) mciEmu.dwSeekedTrack = mciEmu.dwFirstTrack;
			mciEmu.dwLastTrack = TTMMSS_ENCODE(i, tracks[i].length / 60, tracks[i].length % 60);
			mciEmu.dwTargetTrack = mciEmu.dwLastTrack;
		}
	}

    for (DWORD i = 1; i <= mciEmu.dwNumTracks; i++) {
		dprintf("Track %02d: [%c] %02d:%02d (%04d sec) @ %d sec\n", 
			i, 
			(tracks[i].type == MCI_AUDIO_TRACK) ? 'A' : 'D', tracks[i].length / 60, 
			tracks[i].length % 60, 
			tracks[i].length,
			tracks[i].position);
	}

    dprintf("Emulating total of %d CD tracks. First=%02d.%02d.%02d Last=%02d.%02d.%02d\n", 
		mciEmu.dwNumTracks, 
		mciEmu.dwFirstTrack>>16, (mciEmu.dwFirstTrack>>8)&0xFF, mciEmu.dwFirstTrack&0xFF,
		mciEmu.dwLastTrack>>16,  (mciEmu.dwLastTrack>>8)&0xFF,  mciEmu.dwLastTrack&0xFF);

}

#define LOOP_TRACKER dprintf("DEBUG@%d: first=%#x play=%#x pause=%#x last=%#x mciEmu.dwCurrentTrack=%#x update=%#x\n", __LINE__, mciEmu.playing, mciEmu.paused, first, last, mciEmu.dwCurrentTrack, mciEmu.bUpdateTrack)

static void player_play()
{
	// v2.05.77 fix: replaced dwSeekedTrack with dwCurrentTrack
	// fixes play resume 
	int offset;
	int trackNo;
	trackNo = TTMMSS_TRACKNO(mciEmu.dwCurrentTrack);
	offset = TTMMSS_OFFSET(mciEmu.dwCurrentTrack);
	dprintf("mciplayer: dwCurrentTrack=%#x {Track=%02d Offset=%02d:%02d}\n", mciEmu.dwCurrentTrack, trackNo, offset / 60, offset % 60);
	mciEmu.playing = (*pplr_play)(trackNo);
	if(pplr_seek && offset) pplr_seek(offset);
}

int player_main()
{
    DWORD first;
    DWORD last;
	bool repeat = (dxw.dwFlags8 & FORCETRACKREPEAT) ? TRUE : FALSE;
	DWORD begin_timer, elapsed_timer;
	DXWNDSTATUS *pStatus = GetHookInfo();

	pStatus->PlayerStatus = DXW_PLAYER_STOPPED;
	pStatus->TrackNo = 0;
	pStatus->TracksNo = TTMMSS_TRACKNO(mciEmu.dwLastTrack);
	//short Volume;
	pStatus->TimeElapsed = 0;

	if(pSendMessageA == NULL) pSendMessageA = SendMessageA;

	dprintf("Begin of player thread\n");
    while (1) {
		if(!mciEmu.playing) {
			pStatus->PlayerStatus = DXW_PLAYER_STOPPED;
			Sleep(50); // not too big, it could emulate hardware inertia ...
			// v2.06.06: reduced from 100 to 50: the value can't be comparable to the MCI_SEEK delay.
			// fixes @#@ "Beast Wars Transformers"
			continue;
		}

		//set track info
        if (mciEmu.bUpdateTrack) {
			int track;
			int offset;
            first = info.first;
            last = info.last;
            mciEmu.dwCurrentTrack = first;
            mciEmu.bUpdateTrack = 0;
			track = TTMMSS_TRACKNO(mciEmu.dwCurrentTrack);
			offset = TTMMSS_OFFSET(mciEmu.dwCurrentTrack);
			pStatus->TrackNo = track;
			pStatus->TimeElapsed = offset;
			pStatus->TrackLength = tracks[track].length;
			if(pplr_seek && offset) pplr_seek(offset);
			elapsed_timer = 1000 * offset;
			dprintf("seeking to track=%d offset=%d\n", track, offset);
        }

        //stop if at end of 'playlist'
        //note "last" track is NON-inclusive
		if (mciEmu.dwCurrentTrack >= last){
			dprintf("Reached last track: %02d\n", TTMMSS_TRACKNO(mciEmu.dwCurrentTrack));
			if(repeat) {
				// replay tracks from beginning
				mciEmu.dwCurrentTrack = first & 0xFF0000; // sets track to beginning 
				pStatus->TrackNo = TTMMSS_TRACKNO(mciEmu.dwCurrentTrack);
				pStatus->TimeElapsed = TTMMSS_OFFSET(mciEmu.dwCurrentTrack);
				pStatus->TrackLength = tracks[pStatus->TrackNo].length;
				player_play();
			}
			else {
				mciEmu.playing = 0; 
				if (info.play_callback){
					LRESULT lres;
					// v2.05.78: never clear info.play_callback AFTER the message shipment 
					// because you could clear a new callback specification. 
					// Fixes @#@ "Frogger 2 Swampy's revenge" interrupted track repetition.
					HWND hwnd = (HWND)info.play_callback;
					info.play_callback = NULL;
					dprintf("Sending MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message on EOT devid=%#x hwnd=%#x\n", 
						mciEmu.dwDevID, hwnd);
					lres = (*pSendMessageA)(hwnd, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, mciEmu.dwDevID);
					if(lres){
						dprintf("error while sending MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message: hwnd=%#x, res=%#x err=%d\n", 
							hwnd, lres, GetLastError());
					}
					else {
						dprintf("Sent MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message: hwnd=%#x, res=%#x\n", hwnd, lres);
					}
					continue;
				}
			}
		}
		else {
			// play mciEmu.dwCurrentTrack song
			pStatus->TrackNo = TTMMSS_TRACKNO(mciEmu.dwCurrentTrack);
			pStatus->TimeElapsed = TTMMSS_OFFSET(mciEmu.dwCurrentTrack);
			pStatus->TrackLength = tracks[pStatus->TrackNo].length;
			player_play();
		}

		begin_timer = (*pGetTickCount)() - (1000 * TTMMSS_OFFSET(mciEmu.dwCurrentTrack));
        while (1) {
            //check control signals
			if(pplr_tell){
				elapsed_timer = (*pplr_tell)();
			}
			else{
				elapsed_timer = ((*pGetTickCount)() - begin_timer) / 1000;
			}
			pStatus->TimeElapsed = elapsed_timer;
			mciEmu.dwCurrentTrack = TTMMSS_ENCODE(TTMMSS_TRACKNO(mciEmu.dwCurrentTrack), elapsed_timer / 60, elapsed_timer % 60);

			if(mciEmu.paused){ // MCI_PAUSE
				if(mciEmu.bUpdateTrack) break;
				pStatus->PlayerStatus = DXW_PLAYER_PAUSED;
				Sleep(100);
				continue;
			}

			if (mciEmu.playing == 0) { //MCI_STOP
				(*pplr_stop)(); //end playback
				LRESULT lres;
				HWND hwnd;
				if (info.stop_callback){ // v2.05.38 fix: send MM_MCINOTIFY only if requested
					hwnd = (HWND)info.stop_callback;
					info.stop_callback = NULL;
					dprintf("Sending MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message on STOP devid=%#x hwnd=%#x\n", 
						mciEmu.dwDevID, hwnd);
					lres = (*pSendMessageA)(hwnd, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, mciEmu.dwDevID);
					dprintf("Sent MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message: hwnd=%#x, res=%#x\n", hwnd, lres);
				}
				else if (info.play_callback){ // v2.05.38 fix: send MM_MCINOTIFY only if requested
					hwnd = (HWND)info.play_callback;
					info.play_callback = NULL;
					dprintf("Sending MM_MCINOTIFY MCI_NOTIFY_ABORTED message on STOP devid=%#x hwnd=%#x\n", 
						mciEmu.dwDevID, hwnd);
					lres = (*pSendMessageA)(hwnd, MM_MCINOTIFY, MCI_NOTIFY_ABORTED, mciEmu.dwDevID);
					dprintf("Sent MM_MCINOTIFY MCI_NOTIFY_ABORTED message: hwnd=%#x, res=%#x\n", hwnd, lres);
				}
				break;
			}

			if ((*pplr_pump)() == 0) {
				//done mciEmu.playing song
                break;
			}

			if (mciEmu.bUpdateTrack) { //MCI_PLAY
                break;
			}

			pStatus->PlayerStatus = DXW_PLAYER_PLAYING;
			pStatus->Volume = (*pplr_getvolume)();
			Sleep(10); // to avoid brakeless loop
        }

		if(mciEmu.playing) {
			mciEmu.dwCurrentTrack = TTMMSS_ENCODE(TTMMSS_TRACKNO(mciEmu.dwCurrentTrack)+1, 0, 0);
			// dprintf("curtrack=%#x @%d\n", mciEmu.dwCurrentTrack, __LINE__);
			pStatus->TrackNo = TTMMSS_TRACKNO(mciEmu.dwCurrentTrack)+1;
			pStatus->TimeElapsed = 0;
		}
    }
	pStatus->PlayerStatus = DXW_PLAYER_STOPPED;
    (*pplr_stop)();

    mciEmu.playing = 0;
	dprintf("End of player thread\n");
    return 0;
}

char* player_get_status()
{
	if(mciEmu.playing) return "PLAY";
	if(mciEmu.dooropened) return "DOOR_OPEN";
	if(mciEmu.paused) return "PAUSE";
	return "STOP";
}

void player_set_status(DWORD status)
{
	switch(status){
		case MCI_STOP:
			mciEmu.playing = 0;
			mciEmu.paused = 0;
			break;
		case MCI_PAUSE:
			mciEmu.paused = 1;
			mciEmu.playing = 0;
			break;
		case MCI_RESUME:
			mciEmu.paused = 0;
			mciEmu.playing = 1;
			break;
		case MCI_PLAY:
            mciEmu.bUpdateTrack = 1;
            mciEmu.playing = 1;
			mciEmu.paused = 0;
			break;
		case MCIEMU_DOOR_OPEN:
			mciEmu.playing = 0;
			mciEmu.paused = 0;
			mciEmu.dooropened = 1;
			break;
		case MCIEMU_DOOR_CLOSED:
			mciEmu.dooropened = 0;
			break;
	}
}
