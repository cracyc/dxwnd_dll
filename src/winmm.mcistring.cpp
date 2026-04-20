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

extern struct play_info movie_info;
extern DWORD HandleStopCommand();
extern char *sTimeFormat(DWORD);
extern DWORD TTMMSSToAbsSec(DWORD);
extern int StringToTTMMSS(char *);
extern BOOL checkGap(char *, unsigned int);
extern DWORD HandlePlayCommand(DWORD, DWORD);
extern BOOL IsWithinMCICall;
extern char *mmErrorString(MMRESULT);
extern int MapMsecToTTMMSS(int);

extern int cdVolume;

int StringToTTMMSS(char *s)
{
	int ttmmss;
	DWORD track, hour, min, sec, frame, tokens, msec;
	char *tformat;
	switch (mciEmu.dwTimeFormat) {
		case MCI_FORMAT_MILLISECONDS:
			sscanf(s, "%d", &msec);
			ttmmss = MapMsecToTTMMSS(msec);
			tformat = "msec";
			break;
		case MCI_FORMAT_TMSF:
			tokens = sscanf(s, "%d:%d:%d:%d", &track, &min, &sec, &frame);
			switch(tokens){
				case 1: ttmmss = track << 16; break;
				case 2: ttmmss = (track << 16) + (min << 8); break;
				// case 3 and 4 identical because we ignore the frames
				case 3: 
				case 4: ttmmss = (track << 16) + (min << 8) + sec; break;
			}
			// "Disney's Hercules" special case ...
			if(track > 99){ // is it the proper condition ???
				DWORD hexval = track;
				track = hexval & 0xFF;
				min = (hexval & 0xFF00) >> 8,
				sec = (hexval & 0xFF0000) >> 16,
				ttmmss = (track << 16) + (min << 8) + sec;
			}
			tformat = "tmsf";
			break;
		case MCI_FORMAT_MSF:
			tokens = sscanf(s, "%d:%d:%d", &min, &sec, &frame);
			sec = (60 * min) + sec; // we ignore frames ....
			for(track = 1; track <= mciEmu.dwNumTracks ; track++) if((int)tracks[track].position > sec) break;
			track--; // go to last track that begun before the given time
			if(track < 1) track = 1;
			if(track > mciEmu.dwNumTracks) track = mciEmu.dwNumTracks;
			sec -= tracks[track].position;
			min = sec / 60;
			sec = sec % 60;
			ttmmss = (track << 16) + (min << 8) + sec;
			tformat = "msf";
			break;
		case MCI_FORMAT_HMS:
			tokens = sscanf(s, "%d:%d:%d", &hour, &min, &sec);
			sec = (3600 * hour) + (60 * min) + sec; 
			for(track = 1; track <= mciEmu.dwNumTracks ; track++) if((int)tracks[track].position > sec) break;
			track--; // go to last track that begun before the given time
			if(track < 1) track = 1;
			if(track > mciEmu.dwNumTracks) track = mciEmu.dwNumTracks;
			sec -= tracks[track].position;
			min = sec / 60;
			sec = sec % 60;
			ttmmss = (track << 16) + (min << 8) + sec;
			tformat = "hms";
			break;
		default:
			tformat = "unknown";
			break;	
	}
	OutDebugSND("> StringToTTMMSS(\"%s\") = TMS#%d:%d:%d tformat=%s\n", 
		s, (ttmmss >> 16), ((ttmmss >> 8) & 0xFF), (ttmmss & 0xFF), tformat);
	return ttmmss;
}


static char *sMCIMode()
{
	char *pMode;
	if(mciEmu.playing) pMode = "playing";
	else if(mciEmu.paused) pMode = "paused";
	else pMode = "stopped";
	return pMode;
}

static int GetHackString(char *sRoom, int trackno, char *sBuf)
{
	char sKey[20];
	char hackPath[MAX_PATH];
	sprintf(hackPath, "%s\\Music\\mcihack.txt", dxw.cdAudioPath);
	if(trackno != 0) sprintf(sKey, "track%02d", trackno);
	else strcpy(sKey, "cd");
	return (*pGetPrivateProfileStringA)(sRoom, sKey, "", sBuf, 40, hackPath);
}

void dxwReplaceArg(LPCSTR Command, char *newCommand, int argIndex, char *newArg)
{
	int len = strlen(Command);
	char *copy = (char *)malloc(len+1);
	strcpy(copy, Command);
	strcpy(newCommand, "");
	for(int index=0; ; index++){
		char *token = strtok(index ? NULL : copy, " ");
		if(token == NULL) break;
		if(index == argIndex) strcat(newCommand, newArg);
		else strcat(newCommand, token);
		strcat(newCommand, " ");
	}
	free(copy);
}

static DWORD DoRetString(char *sAnswer, char *lpszReturnString, int cchReturn, char *label)
{
	if(strlen(sAnswer)<(size_t)cchReturn){
		strcpy(lpszReturnString, sAnswer);
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> %s: ret=%s\n", label, lpszReturnString); 
		}
		return MMSYSERR_NOERROR;
	}
	else {
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> %s: err=MCIERR_PARAM_OVERFLOW ret=%s\n", label, sAnswer); 
		}
		return MCIERR_PARAM_OVERFLOW;
	}
}

static int mciDurationToString(char *sAnswer, int iMaxLen, unsigned int duration)
{
	char sBuf[80+1];
	int ret;
	switch(mciEmu.dwTimeFormat){
		case MCI_FORMAT_MILLISECONDS:
			sprintf_s(sBuf, 80, "%d", duration * 1000);
			break;
		// beware: measures about durations are identical in tmsf and msf formats
		case MCI_FORMAT_MSF:
		case MCI_FORMAT_TMSF:
			sprintf_s(sBuf, 80, "%02d:%02d:00", duration / 60, duration % 60);
			break;
		case MCI_FORMAT_HMS:
			sprintf_s(sBuf, 80, "%02d:%02d:%02d", duration / 3600, (duration % 3600) / 60, duration % 60);
			break;
		case MCI_FORMAT_FRAMES:
			sprintf_s(sBuf, 80, "%d", duration * 75);
			break;
		default:
			return 290; // invalid parameter value
			break;
	}

	ret = 0;
	if(strlen(sBuf)<(size_t)iMaxLen){
		strcpy(sAnswer, sBuf);
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> retString=\"%s\"\n", sAnswer);
		}
	}
	else {
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> err=MCIERR_PARAM_OVERFLOW\n"); 
		}
		ret = MCIERR_PARAM_OVERFLOW;
	}
	return ret;
}

static int mciPositionToString(char *sAnswer, int iMaxLen, int track, unsigned int sec)
{
	char sBuf[80+1];
	int ret;
	switch(mciEmu.dwTimeFormat){
		case MCI_FORMAT_MILLISECONDS:
			sprintf_s(sBuf, 80, "%d", sec * 1000);
			break;
		case MCI_FORMAT_TMSF:
			sec -= tracks[track].position;
			sprintf_s(sBuf, 80, "%02d:%02d:%02d:00", track, sec / 60, sec % 60);
			break;
		case MCI_FORMAT_MSF:
			sprintf_s(sBuf, 80, "%02d:%02d:00", sec / 60, sec % 60);
			break;
		case MCI_FORMAT_HMS:
			sprintf_s(sBuf, 80, "%02d:%02d:%02d", sec / 3600, (sec % 3600) / 60, sec % 60);
			break;
		case MCI_FORMAT_FRAMES:
			sprintf_s(sBuf, 80, "%d", sec * 75);
			break;
		default:
			return 290; // invalid parameter value
			break;
	}

	ret = 0;
	if(strlen(sBuf)<(size_t)iMaxLen){
		strcpy(sAnswer, sBuf);
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> retString=\"%s\"\n", sAnswer);
		}
	}
	else {
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> err=MCIERR_PARAM_OVERFLOW\n"); 
		}
		ret = MCIERR_PARAM_OVERFLOW;
	}
	return ret;
}

static int mciModeToString(char *sAnswer, int iMaxLen)
{
	int ret = 0;
	char *pMode;
	if(mciEmu.playing) pMode = "playing";
	else if((mciEmu.paused) && (dxw.dwFlags11 & CDPAUSECAPABILITY)) pMode = "paused";
	else pMode = "stopped";
	if((int)strlen(pMode) < iMaxLen) {
		strcpy(sAnswer, pMode);
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> status mode=%s\n", sAnswer);
		}
	}
	else {
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> err=MCIERR_PARAM_OVERFLOW\n"); 
		}
		ret = MCIERR_PARAM_OVERFLOW;
	}
	return ret;
}

static int mciTimeFormatToString(char *sAnswer, int iMaxLen)
{
	int ret = 0;
	char *pFormat;
	switch(mciEmu.dwTimeFormat){
		case MCI_FORMAT_MILLISECONDS:
			pFormat = "milliseconds";
			break;
		case MCI_FORMAT_MSF:
			pFormat = "msf";
			break;
		case MCI_FORMAT_TMSF:
			pFormat = "tmsf";
			break;
		case MCI_FORMAT_HMS:
			pFormat = "hms";
			break;
		case MCI_FORMAT_FRAMES:
			// unsupported by cdaudio devices !!!
			pFormat = "frames";
			break;
		default:
			pFormat = "unsupported";
			break;
	}
	if((int)strlen(pFormat) < iMaxLen) {
		strcpy(sAnswer, pFormat);
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> status mode=%s\n", sAnswer);
		}
	}
	else {
		if(IsTraceSYS || IsTraceSND){
			OutTrace("> err=MCIERR_PARAM_OVERFLOW\n"); 
		}
		ret = MCIERR_PARAM_OVERFLOW;
	}
	return ret;
}


MCIERROR mciSendMovieString(ApiArg, LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
	char NewCommand[256];
	RECT rect;
	char sTail[81];
	sTail[0]=0;

	// v2.06.09: capture cdaudio or movie callback
	if(!strncmp(lpszCommand, "play ", 5)) {
		movie_info.play_callback = (DWORD_PTR)hwndCallback;
	}

	if(dxw.dwFlags5 & REMAPMCI) {
		HWND wHandle;

		if(strstr(lpszCommand, " fullscreen")) {
			// eliminate "fullscreen specification"
			char *p = (char *)strstr(lpszCommand, " fullscreen");
			memcpy(NewCommand, lpszCommand, (p - lpszCommand));
			NewCommand[p - lpszCommand] = 0;
			// fails: needs some stretching capability?
			//char sNewPos[81];
			//sprintf(sNewPos, " at %d %d %d %d", dxw.iPosX, dxw.iPosY, dxw.iPosX+dxw.iSizX, dxw.iPosY+dxw.iSizY);
			//strcat(NewCommand, sNewPos);
			strcat(NewCommand, &p[strlen(" fullscreen")]);
			lpszCommand=NewCommand;
			OutTraceDW("%s: replaced Command=\"%s\"\n", api, lpszCommand);
			// now should stretch the mciEmu.dwCurrentTrack window ???
		}

		if (sscanf(lpszCommand, "put %s destination at %ld %ld %ld %ld %s", 
			dxw.sMovieNickName, &(rect.left), &(rect.top), &(rect.right), &(rect.bottom), sTail)>=5) {
			if(dxw.dwFlags6 & STRETCHMOVIES){
				rect.top = rect.left = 0;
				if(dxw.GDIEmulationMode == GDIMODE_NONE){
					rect.right = dxw.iSizX;
					rect.bottom = dxw.iSizY;
				}
				else {
					rect.right = dxw.GetScreenWidth();
					rect.bottom = dxw.GetScreenHeight();
				}
			}
			else {
				// v2.03.19 height / width fix
				rect.right += rect.left; // convert width to position
				rect.bottom += rect.top; // convert height to position
				rect=dxw.MapClientRect(&rect);
				rect.right -= rect.left; // convert position to width
				rect.bottom -= rect.top; // convert position to height
			}
			sprintf(NewCommand, "put %s destination at %d %d %d %d %s", 
				dxw.sMovieNickName, rect.left, rect.top, rect.right, rect.bottom, sTail);
			lpszCommand=NewCommand;
			OutTraceDW("%s: replaced Command=\"%s\"\n", api, lpszCommand);
		}

		// v2.05.82 - replace window handle forcing the handle of the main window
		// partially fixes the displacement of "Felix the Cat" Felix and Pinky intro movies.

		// @#@ two spaces found in "Age of Empires Gold Edition"
		if(! (dxw.dwFlags5 & PUSHACTIVEMOVIE)){
			if ((sscanf(lpszCommand, "window %s handle %d", dxw.sMovieNickName, &wHandle)==2) ||
				(sscanf(lpszCommand, "window %s  handle %d", dxw.sMovieNickName, &wHandle)==2)){ 
				//sprintf(NewCommand, "window %s handle %d state restore wait", dxw.sMovieNickName, dxw.GethWnd());
				char newHandle[20];
				sprintf(newHandle, "%d", dxw.GethWnd());
				//sprintf(newHandle, "%d", 0);
				dxwReplaceArg(lpszCommand, NewCommand, 3, newHandle);
				lpszCommand=NewCommand;
				OutTraceDW("%s: replaced Command=\"%s\"\n", api, lpszCommand);
			}
		}

		if (sscanf(lpszCommand, "put %s window client at %ld %ld %ld %ld %s", // found in "Twisted Metal 1"
			dxw.sMovieNickName, &(rect.left), &(rect.top), &(rect.right), &(rect.bottom), sTail)>=5){
			// v2.03.19 height / width fix
			rect.right += rect.left; // convert width to position
			rect.bottom += rect.top; // convert height to position
			rect=dxw.MapClientRect(&rect);
			rect.right -= rect.left; // convert position to width
			rect.bottom -= rect.top; // convert position to height
			sprintf(NewCommand, "put %s window client at %d %d %d %d %s", dxw.sMovieNickName, rect.left, rect.top, rect.right, rect.bottom, sTail);
			lpszCommand=NewCommand;
			OutTraceDW("%s: replaced Command=\"%s\"\n", api, lpszCommand);
		}
	}

	if(dxw.dwFlags6 & NOMOVIES) {
		if (!strncmp(lpszCommand, "play ", 5)) {
			OutTrace("%s: NOMOVIES skip play video command\n", ApiRef);
			if(movie_info.play_callback){
				LRESULT lres = (*pSendMessageA)((HWND)movie_info.play_callback, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, 0);
				OutTrace("%s: CLOSE: Sent MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message: hwnd=%#x, res=%#x\n", ApiRef, movie_info.play_callback, lres);
				movie_info.play_callback = 0;
			}
		}
		return 0;
	}

	return (*pmciSendStringA)(lpszCommand, lpszReturnString, cchReturn, hwndCallback);
}

MCIERROR mciSendAudioString(ApiArg, LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
		char sNickName[80+1];
		char sCommand[80+1];
		char *sCmdTarget;
		DWORD dwCommand;
		static char cDriveLetter = 0;

	// v2.06.09: handle cdaudio callback on close
	if(!strncmp(lpszCommand, "close ", 6)) {
		if(info.play_callback){
			LRESULT lres = (*pSendMessageA)((HWND)info.play_callback, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, 0);
			OutTrace("%s: CLOSE: Sent MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message: hwnd=%#x, res=%#x\n", ApiRef, info.play_callback, lres);
			info.play_callback = 0;
		}
	}
	if(!strncmp(lpszCommand, "play ", 5)) {
		info.play_callback = (DWORD_PTR)hwndCallback;
	}

	if(!pplr_pump) player_init();
	player_change();
	if(mciEmu.dwTimeFormat == MCI_FORMAT_UNDEFINED) mciEmu.dwTimeFormat = MCI_FORMAT_MSF;

	if(sscanf(lpszCommand, "%s %s", sCommand, dxw.sAudioNickName) != 2) {
		OutTraceE("%s: bad syntax on \"%s\"\n", api, lpszCommand);
		return (*pmciSendStringA)((LPCSTR)lpszCommand, lpszReturnString, cchReturn, (HWND)hwndCallback);
	}

	if(!strcmp(sCommand, "open")) dwCommand = MCI_OPEN; else
	if(!strcmp(sCommand, "close")) dwCommand = MCI_CLOSE; else
	if(!strcmp(sCommand, "stop")) dwCommand = MCI_STOP; else
	if(!strcmp(sCommand, "pause")) dwCommand = MCI_PAUSE; else
	if(!strcmp(sCommand, "resume")) dwCommand = MCI_RESUME; else
	if(!strcmp(sCommand, "set")) dwCommand = MCI_SET; else
	if(!strcmp(sCommand, "status")) dwCommand = MCI_STATUS; else
	if(!strcmp(sCommand, "play")) dwCommand = MCI_PLAY; else
	if(!strcmp(sCommand, "seek")) dwCommand = MCI_SEEK; else
	if(!strcmp(sCommand, "capability")) dwCommand = MCI_GETDEVCAPS; else
	if(!strcmp(sCommand, "setaudio")) dwCommand = MCI_SETAUDIO; else
	if(!strcmp(sCommand, "info")) dwCommand = MCI_INFO; else
	dwCommand = 0; 
	OutDebugSND("> command=%d device=%s\n", dwCommand, dxw.sAudioNickName);
	
	if(dwCommand && (dwCommand != MCI_OPEN)){
		// don't try to parse unknown commands, nor open command that
		// doesn't necessarily have extra arguments
		sCmdTarget = (char *)lpszCommand;
		while (*sCmdTarget && *sCmdTarget != ' ') sCmdTarget++; // skip command
		while (*sCmdTarget && *sCmdTarget == ' ') sCmdTarget++; // skip first separator
		while (*sCmdTarget && *sCmdTarget != ' ') sCmdTarget++; // skip deviceid
		while (*sCmdTarget && *sCmdTarget == ' ') sCmdTarget++; // skip second separator
	}

	switch (dwCommand) {

		case MCI_OPEN:
			{
				BOOL bVirtual = FALSE;
				char sDriveString[32+1];
				int iDriveLetter;

				if (!bVirtual && (sscanf(lpszCommand, "open cdaudio alias %s", sNickName) == 1)) {
					strcpy(dxw.sAudioNickName, sNickName);
					OutTraceSND("> registered alias=%s for cdaudio\n", dxw.sAudioNickName);
					bVirtual = TRUE;
				}

				// v2.06.14: @#@ found in "Sudden Strike"
				if (!bVirtual && (sscanf(lpszCommand, "open cdaudio shareable alias %s", sNickName) == 1)) {
					strcpy(dxw.sAudioNickName, sNickName);
					OutTraceSND("> registered alias=%s for cdaudio\n", dxw.sAudioNickName);
					bVirtual = TRUE;
				}

				// v2.06.04 @#@ found in "Nakoruru ~Ano Hito kara no Okurimono~" (Jap 2001)
				if (!bVirtual && (sscanf(lpszCommand, "open cdaudio!%s alias %s", sDriveString, sNickName) == 2)) {
					strcpy(dxw.sAudioNickName, sNickName);
					OutTraceSND("> registered alias=%s for cdaudio!%s\n", dxw.sAudioNickName, sDriveString);
					bVirtual = TRUE;
				}

				// v2.05.05 @#@ found in HoMM1 Win32 version
				// v2.05.78 fixed command parsing
				// v2.06.00 fixed command parsing again for "Super Ball King"
				if (!bVirtual && 
					(sscanf(lpszCommand, "open %c: type cdaudio alias %80s", &iDriveLetter, sNickName) == 2)
					) {
					OutDebugSND("> sDriveLetter=\"%c\"\n", iDriveLetter);
					cDriveLetter = (char)tolower(iDriveLetter);
					if((cDriveLetter >= 'a') && (cDriveLetter <= 'z')) {
						strcpy(dxw.sAudioNickName, sNickName);
						OutTraceSND("> registered alias=%s for drive %c:\n", dxw.sAudioNickName, cDriveLetter); 
						bVirtual = TRUE;
					}
				}

				// v2.05.75 @#@ found in "Frogger: He's Back"
				// v2.05.78 fixed command parsing
				if (!bVirtual && 
					(sscanf(lpszCommand, "open %32s: type cdaudio", sDriveString) == 1) &&
					(strlen(sDriveString) == 2)
					) {
					cDriveLetter = (char)tolower(sDriveString[0]);
					if((cDriveLetter >= 'a') && (cDriveLetter <= 'z') && sDriveString[1] == ':') {
						strcpy(dxw.sAudioNickName, sDriveString);
						OutTraceSND("> registered alias=%s for drive %c:\n", dxw.sAudioNickName, cDriveLetter); 
						bVirtual = TRUE;
					}
				}

				// v2.05.42 fix: "open cdaudio" must work as any other "open cdaudio ..." command
				// found in "Fighting Force".
				if (!bVirtual && (!strncmp(lpszCommand, "open cdaudio", strlen("open cdaudio")))){
					OutTraceSND("> handling cdaudio device\n"); 
					bVirtual = TRUE;
				}

				if(bVirtual){
					if(dxw.dwFlags12 & SUPPRESSCDAUDIO) {
						OutTraceSND("%s: suppress CDAUDIO ret=MCIERR_DEVICE_NOT_INSTALLED\n", api);
						return MCIERR_DEVICE_NOT_INSTALLED;
					}

					if (cchReturn>5) {
						// v2.05.25: replaced 0xBEEF with correct value
						sprintf(lpszReturnString, "%d", dxw.VirtualCDAudioDeviceId);
						OutTraceSND("> ret=%s\n", lpszReturnString);
					}
					// do not return error here if cchReturn is 0: "Target" does it and
					// expects the function not to fail.

					player_set_status(MCI_OPEN);
					return 0;
				}
			}
			break;

		case MCI_SYSINFO: 
			{
				// v2.04.93: fix, the correct return string is "cdaudio", not "cd"
				// thanks to "dippy dipper", fixes "Pandemonium!" sound emulation
				return DoRetString("cdaudio", lpszReturnString, cchReturn, sCmdTarget);
			}
			break;

		case MCI_INFO: 
			{
				char sBuffer[81];
				char hackPath[MAX_PATH];
				sprintf(hackPath, "%s\\Music\\mcihack.txt", dxw.cdAudioPath);
				if(strstr(sCmdTarget, "identity")){
					(*pGetPrivateProfileStringA)("info", "identity", "fakecd", sBuffer, 80, hackPath);
					if(!strlen(sBuffer)){
						OutTraceSND("> ret=350(MCIERR_NO_IDENTITY)\n");
						return MCIERR_NO_IDENTITY;
					}
					return DoRetString(sBuffer, lpszReturnString, cchReturn, sCmdTarget);
				}
				if(strstr(sCmdTarget, "upc")){
					// v2.05.05: seen in "Heroes of Might and Magic", Win32 CD version.
					// as alternative, return MCIERR_NO_IDENTITY error ?
					(*pGetPrivateProfileStringA)("info", "product", "fakecd", sBuffer, 80, hackPath);
					if(!strlen(sBuffer)){
						OutTraceSND("> ret=350(MCIERR_NO_IDENTITY)\n");
						return MCIERR_NO_IDENTITY;
					}
					return DoRetString(sBuffer, lpszReturnString, cchReturn, sCmdTarget);
				}
			}
			break;

		case MCI_STOP:
			// v2.05.85: added handling of "stop all" command.
			// note: the command could be followed by "notify" and/or "wait".
			if(!strncmp(lpszCommand, "stop all", strlen("stop all"))){
				MCIERROR ret = 0;
				// stop the emulated CD first and then ...
				HandleStopCommand();
				// stop everything else. Hopefully, theis call will take care of "wait" and "notify" options.
				if(pmciSendStringA) ret = (*pmciSendStringA)(sCommand, lpszReturnString, cchReturn, (HWND)hwndCallback);
				if(ret) OutTraceE("%s: ERROR err=%#x\n", ApiRef, ret);
				return ret;
			}
			HandleStopCommand();
			return 0;
			break;

		case MCI_PAUSE:
			{
				// v2.05.94: Fixed pause cdaudio logic
				if((dxw.dwFlags11 & CDPAUSECAPABILITY))player_set_status(MCI_PAUSE);
				else player_set_status(MCI_STOP);
				return 0;
			}
			break;
		case MCI_RESUME:
			{
				player_set_status(dwCommand);
				return 0;
			}
			break;

		case MCI_CLOSE:
			{
				// Beware: from MSDN examples here https://msdn.microsoft.com/en-us/library/windows/desktop/dd797881(v=vs.85).aspx
				// and from commands sent to "Fighting Force" it seems evident that "close cdaudio" does NOT stop the music play.
				if (cchReturn>0) strcpy(lpszReturnString, "");
				// v2.05.00: MCI_CLOSE resets time formats
				mciEmu.dwTimeFormat = MCI_FORMAT_UNDEFINED;
				info.play_callback = 0;
				info.stop_callback = 0;
				OutTraceSND("> ret=0(MCIERR_NOERROR) from device=%s\n", dxw.sAudioNickName);
				return 0;
		}
			break;

		case MCI_SET:
			{
				// v2.05.01: better argument control, fixed bug with "tmsf" and "msf" confused with "ms"
				DWORD dwNewTimeFormat = -1;
				if (strstr(sCmdTarget, "time format")){
					char format[81];
					strcpy(format, "");
					sscanf(sCmdTarget, "time format %s", format);
					if (!strcmp(format, "tmsf"))
						dwNewTimeFormat = MCI_FORMAT_TMSF;
					else if (!strcmp(format, "msf"))
						dwNewTimeFormat = MCI_FORMAT_MSF;
					else if (!strcmp(format, "milliseconds"))
						dwNewTimeFormat = MCI_FORMAT_MILLISECONDS;
					else if  (!strcmp(format, "frames"))
						dwNewTimeFormat = MCI_FORMAT_FRAMES;
					else if  (!strcmp(format, "hms"))
						dwNewTimeFormat = MCI_FORMAT_HMS;
					else if  (!strcmp(format, "ms"))
						dwNewTimeFormat = MCI_FORMAT_MILLISECONDS;

					if(dwNewTimeFormat == -1){
						OutTraceE("%s: unknown time format \"%s\"\n", api, lpszCommand);
						return 290;
					}
					else {
						OutTraceSND("%s: set time format %#x(%s)\n", api, dwNewTimeFormat, sTimeFormat(dwNewTimeFormat));
						mciEmu.dwTimeFormat = dwNewTimeFormat;
						return 0;	
					}
				}
				else
				if (strstr(sCmdTarget, "door open")){
					player_set_status(MCIEMU_DOOR_OPEN);
					return 0;
				}
				else
				if (strstr(sCmdTarget, "door closed")){
					player_set_status(MCIEMU_DOOR_CLOSED);
					return 0;
				}
				else
				if (strstr(sCmdTarget, "audio all off") ||
					strstr(sCmdTarget, "audio left off") ||
					strstr(sCmdTarget, "audio right off")){
					cdVolume = (*pplr_getvolume)();
					(*pplr_setvolume)(0);
					return 0;
				}
				else
				if (strstr(sCmdTarget, "audio all on") ||
					strstr(sCmdTarget, "audio left on") ||
					strstr(sCmdTarget, "audio right on")){
					(*pplr_setvolume)(cdVolume);
					return 0;
				}
			}
			break;

		case MCI_STATUS:
			{
				// v2.05.59 fix:
				// because of the separate player thread, it may take some time before the variables are set
				//"Rockman Strategy" polls the CD status immediately and furiously, with the result of blocking
				// the audio play. A small delay (sleep 100 nSec) simulated the CD hardware delay and fixes
				// the problem.
				if(dxw.dwFlags14 & SLOWCDSTATUS) Sleep(100);

				char sAnswer[81]; // big more than enough
				if(strstr(sCmdTarget, "number of tracks")){
					_itoa(mciEmu.dwNumTracks, sAnswer, 10);
					return DoRetString(sAnswer, lpszReturnString, cchReturn, sCmdTarget);
				}
				else
				if(strstr(sCmdTarget, "media present")){
					// v2.04.81: when CDAUDIO emulation is on, always pretend the CDROM is in the caddy
					// found in "Terracide"
					return DoRetString(mciEmu.dooropened ? "false" : "true", lpszReturnString, cchReturn, sCmdTarget);
				}
				else
				if(strstr(sCmdTarget, "type track")){
					// v2.04.81: "status cdaudio type track <n>" emulation: 
					// first track is data track, all other tracks are supposed to be audio.
					// found in "Terracide"
					int iTrackNo;
					sscanf(sCmdTarget, "type track %d", &iTrackNo);
					strcpy(sAnswer, iTrackNo == 1 ? "other" : "audio");
					return DoRetString(sAnswer, lpszReturnString, cchReturn, sCmdTarget);
				}
				else					
				if(strstr(sCmdTarget, "length track")){
					// "status cdaudio length track <num>" 
					// found in "The Fifth Element"
					UINT trackno = 0; // avoid exceptions ...
					if (sscanf(sCmdTarget, "length track %d", &trackno) == 1){
						if((trackno < 1) || (trackno > TTMMSS_TRACKNO(mciEmu.dwLastTrack))) return MCIERR_OUTOFRANGE;
						if(dxw.dwFlags11 & HACKMCIFRAMES){
							char *sRoom = NULL;
							switch(mciEmu.dwTimeFormat){ // v2.06.07 fix
								case MCI_FORMAT_FRAMES: sRoom="frames"; break;
								case MCI_FORMAT_MILLISECONDS: sRoom="msec"; break; // @#@ "Breakdown"
								case MCI_FORMAT_TMSF: sRoom="tmsf"; break;
								case MCI_FORMAT_MSF: sRoom="msf"; break;
							}
							if(sRoom){
								char sBuf[40+1];
								int len = GetHackString(sRoom, trackno, sBuf);
								if(len > 0) {
									dprintf("> MCI_STATUS_LENGTH hacked value=\"%s\"\n", sBuf);
									return DoRetString(sBuf, lpszReturnString, cchReturn, sCmdTarget);
								}
							}
							else {
								dprintf("> missing hacked value: room=%s track=%d\n", sRoom, trackno);
							}
						}
						return mciDurationToString(lpszReturnString, cchReturn, tracks[trackno].length);
					}
				}
				else					
				if(strstr(sCmdTarget, "position track")){
					// "status cdaudio position track <num>" 
					// "status cdaudio position track <num>" found in "The Fifth Element"
					// found in codeproject CD player demo:  https://www.codeproject.com/Articles/3549/MCI-CD-Player
					// "status cdaudio position" found in "Rockman Strategy"
					UINT trackno = 0; // avoid exceptions ...
					if (sscanf(sCmdTarget, "position track %d", &trackno) == 1){
						if((trackno < 1) || (trackno > TTMMSS_TRACKNO(mciEmu.dwLastTrack))) return MCIERR_OUTOFRANGE;
						if(dxw.dwFlags11 & HACKMCIFRAMES){
							char *sRoom = NULL;
							switch(mciEmu.dwTimeFormat){
								case MCI_FORMAT_FRAMES: sRoom="start_frames_s"; break;
								case MCI_FORMAT_MILLISECONDS: sRoom="start_msec_s"; break;
								case MCI_FORMAT_TMSF: sRoom="start_tmsf_s"; break;
								case MCI_FORMAT_MSF: sRoom="start_msf_s"; break;
							}
							if(sRoom){
								char sBuf[40+1];
								int len = GetHackString(sRoom, trackno, sBuf);
								if(len) {
									dprintf("> MCI_STATUS_LENGTH hacked value=%#x\n", sBuf);
									return DoRetString(sBuf, lpszReturnString, cchReturn, sCmdTarget);
								}
							}
						}
						return mciPositionToString(lpszReturnString, cchReturn, trackno, tracks[trackno].position);
					}
					else {
						// v2.05.87 fix:
						//return mciPositionToString(lpszReturnString, cchReturn, 0, mciEmu.dwCurrentTrack);
						// err 270:  An integer in the command was invalid or missing.
						return 270;
					}
				}
				else 
				if(strstr(sCmdTarget, "type track")){
					// "status cdaudio type track <num>" found in "Terracide"
					UINT trackno = 0; // avoid exceptions ...
					if (sscanf(lpszCommand, "type track %d", &trackno) == 1){
						if((trackno < 1) || (trackno > TTMMSS_TRACKNO(mciEmu.dwLastTrack))) return MCIERR_OUTOFRANGE;
						if(tracks[trackno].type == MCI_AUDIO_TRACK) 
							strcpy(sAnswer, "audio");
						else
							strcpy(sAnswer, "other");
						return DoRetString(sAnswer, lpszReturnString, cchReturn, sCmdTarget);
					}
				}
				else
				if(strstr(sCmdTarget, "mode")){
					// v2.04.96: "status cdaudio mode" (uppercase) found in "Fallen Haven"
					// the virtual CD player can't be "not ready" or elsewhere ...
					return DoRetString(sMCIMode(), lpszReturnString, cchReturn, sCmdTarget);
				}
				else
				if(strstr(sCmdTarget, "current track")){
					sprintf(sAnswer, "%d", TTMMSS_TRACKNO(mciEmu.dwCurrentTrack));
					return DoRetString(sAnswer, lpszReturnString, cchReturn, sCmdTarget);
				}
				else
				if (strstr(sCmdTarget, "track")){
					UINT trackno = 0; // avoid exceptions ...
					char spec[81];
					int tag;
					int ret = 0;
					char *sInfo = "???";
					strcpy(spec, "");
					if (sscanf(sCmdTarget, "track %d %s", &trackno, spec) == 2){
						if((trackno < 1) || (trackno > TTMMSS_TRACKNO(mciEmu.dwLastTrack))) return MCIERR_OUTOFRANGE;
						if(!strcmp(spec, "position")){
							OutTraceSND("> track %d position", trackno);
							return mciPositionToString(lpszReturnString, cchReturn, trackno, tracks[trackno].position);
						}
						if(!strcmp(spec, "length")){
							OutTraceSND("> track %d length", trackno);
							return mciDurationToString(lpszReturnString, cchReturn, tracks[trackno].length);
						}
					}
					if (sscanf(sCmdTarget, "track %d %d", &trackno, &tag) == 2){
						if((trackno < 1) || (trackno > TTMMSS_TRACKNO(mciEmu.dwLastTrack))) return MCIERR_OUTOFRANGE;
						switch (tag) {
							case 1:
								ret = mciDurationToString(lpszReturnString, cchReturn, tracks[trackno].position);
								sInfo = "length";
								break;
							case 2:
								// seen in "Fallen Haven" !
								ret = mciPositionToString(lpszReturnString, cchReturn, trackno, tracks[trackno].position);
								sInfo = "pos"; 
								break;
							case 3:
								snprintf(lpszReturnString, cchReturn, "%d", mciEmu.dwNumTracks);
								sInfo = "numtracks"; 
								break;
							case 4:
								ret = mciModeToString(lpszReturnString, cchReturn);
								sInfo = "mode"; 
								break;
							case 5:
								// ??? undocumented
								strncpy(lpszReturnString, "true", cchReturn);
								break;
							case 6:
								ret = mciTimeFormatToString(lpszReturnString, cchReturn);
								sInfo = "timeformat";
								break;
							case 7:
								// ??? undocumented
								strncpy(lpszReturnString, "true", cchReturn);
								break;
							case 8:
								// undocumented. Thanks to huh researches, it seems the last seeked track, track 1 included ....
								snprintf(lpszReturnString, cchReturn, "%d", TTMMSS_TRACKNO(mciEmu.dwSeekedTrack));
								sInfo = "seekedtrack"; 
								break;
							default:
								ret = 274; // The MCI device the system is using does not support the specified command
								break;
						}
						OutTraceSND("> track %d %d(%s): ret=%d:\"%s\"\n", trackno, tag, sInfo, ret, lpszReturnString); 
						return ret;
					}
				}
				else
				if(strstr(sCmdTarget, "position")){ // "position track" and "track position" intercepted above
					// v2.05.11: must not return a duration but the current position: fixes "The House of the Dead" audio stop / resume 
					int ret = mciPositionToString(lpszReturnString, cchReturn, TTMMSS_TRACKNO(mciEmu.dwCurrentTrack), TTMMSSToAbsSec(mciEmu.dwCurrentTrack));
					OutTraceSND("> ret=%d:\"%s\"\n", ret, lpszReturnString); 
					return ret;
				}
				else					
				if(strstr(sCmdTarget, "length")){ // "length track" and "track length" intercepted above
					// v2.05.01: fixed "status cdaudio length" 
					int ret = mciDurationToString(lpszReturnString, cchReturn, TTMMSSToAbsSec(mciEmu.dwLastTrack));
					OutTraceSND("> ret=%d:\"%s\"\n", ret, lpszReturnString); 
					return ret;
				}
				else
				if(strstr(sCmdTarget, "ready")){ // should answer "true" if media is present or "false" otherwise
					// v2.05.42: fixed "status cdaudio ready" found in "Disney's Hercules"
					int ret = 0;
					strncpy(lpszReturnString, "true", cchReturn);
					OutTraceSND("> ret=%d:\"%s\"\n", ret, lpszReturnString); 
					return ret;
				}
				else
				if(strstr(sCmdTarget, "time format")){
					if(mciEmu.dwTimeFormat==MCI_FORMAT_MILLISECONDS){
						strcpy(sAnswer, "milliseconds");
						return DoRetString(sAnswer, lpszReturnString, cchReturn, sCmdTarget);
					}
					if(mciEmu.dwTimeFormat==MCI_FORMAT_TMSF){
						strcpy(sAnswer, "tmsf");
						return DoRetString(sAnswer, lpszReturnString, cchReturn, sCmdTarget);
					}
					if(mciEmu.dwTimeFormat==MCI_FORMAT_MSF){
						strcpy(sAnswer, "msf");
						return DoRetString(sAnswer, lpszReturnString, cchReturn, sCmdTarget);
					}
				}						
				else {
					OutTraceSND("> unrecognized command\n"); 
				}
			}
			break;

		case MCI_PLAY:
			{
				char sFrom[81];
				char sTo[81];
				if(mciEmu.dwTimeFormat == MCI_FORMAT_UNDEFINED) mciEmu.dwTimeFormat = MCI_FORMAT_MSF;
				// v2.05.38 fix: compare with "cdaudio" or nick to avoid confusion with movies or midi files
				// fixes "3-D Ultra Pinball" playing CD tracks of another game instead of its own midi file
				// by default, play the whole disk
				DWORD from = UNKNOWNTRACK;
				DWORD to = UNKNOWNTRACK;

				// v2.05.54: initialize all callbacks
				info.play_callback = 0;
				info.stop_callback = 0;
				// v2.05.60: "notify can be in different places of the command line ...
				// ... in the end ...
				if(!strcmp("notify", &sCmdTarget[strlen(sCmdTarget)-strlen("notify")])){
					OutTraceSND("> notify option hwnd=%#x\n", hwndCallback);
					info.play_callback = (DWORD_PTR)hwndCallback;
				}
				// ... or before the from and to spec. (see "Super Ball King")
				if(!strncmp(sCmdTarget, "notify ", strlen("notify "))){
					sCmdTarget += strlen("notify ");
					OutTraceSND("> notify option hwnd=%#x\n", hwndCallback);
					info.play_callback = (DWORD_PTR)hwndCallback;
				}

				// v2.05.86: full revision of play command

				if (sscanf(sCmdTarget, "from %s to %s", sFrom, sTo) == 2){
					from = StringToTTMMSS(sFrom);
					to = StringToTTMMSS(sTo);
				}
				else
				if (sscanf(sCmdTarget, "from %s", sFrom) == 1){ 
					from = StringToTTMMSS(sFrom);
					to = UNKNOWNTRACK;
				}
				else
				if (sscanf(sCmdTarget, "to %s", sTo) == 1){ 
					from = UNKNOWNTRACK;
					to = StringToTTMMSS(sTo);
				}
				else {
					from = UNKNOWNTRACK;
					to = UNKNOWNTRACK;
				}
				if((mciEmu.dwTimeFormat == MCI_FORMAT_MILLISECONDS) && (from != UNKNOWNTRACK)) {
					if(checkGap(ApiRef, atoi(sFrom))) return MCIERR_OUTOFRANGE;
				}
				return HandlePlayCommand(from, to);
			}
			break;

		case MCI_SEEK:
			{
				char sTarget[81];
				if (sscanf(sCmdTarget, "to %s", sTarget) == 1){
					if(!strcmp(sTarget, "start"))
						mciEmu.dwSeekedTrack = mciEmu.dwFirstTrack;
					else
					if(!strcmp(sTarget, "end"))
						mciEmu.dwSeekedTrack = mciEmu.dwLastTrack;
					else {
						// v2.05.01: fixed for all time formats
						mciEmu.dwSeekedTrack = StringToTTMMSS(sTarget);
					}
					// v2.05.86: seeked position becomes the current position. The player stop.
					mciEmu.dwCurrentTrack = mciEmu.dwSeekedTrack;
					player_set_status(MCI_STOP);
					return 0;
				}
			}
			break;

		case MCI_GETDEVCAPS:
			{
				char sTarget[81];
				if (!strcmp(sCmdTarget, "device type")){
					return DoRetString("cdaudio", lpszReturnString, cchReturn, sCmdTarget);
				}
				else
				if (sscanf(sCmdTarget, "can %s", sTarget) == 1){
					if(!strcmp(sTarget, "eject")) return DoRetString("true", lpszReturnString, cchReturn, sCmdTarget);
					else
					if(!strcmp(sTarget, "play")) return DoRetString("true", lpszReturnString, cchReturn, sCmdTarget);
					else
					return DoRetString("false", lpszReturnString, cchReturn, sCmdTarget);
				}
				else
				if (sscanf(sCmdTarget, "has %s", sTarget) == 1){
					if(!strcmp(sTarget, "audio")) return DoRetString("true", lpszReturnString, cchReturn, sCmdTarget);
					else
					return DoRetString("false", lpszReturnString, cchReturn, sCmdTarget);
				}
			}
			break;

		case MCI_SETAUDIO:
			{
				UINT nVolume;
				if (sscanf(sCmdTarget, "volume to %d", &nVolume) == 1){
					(*pplr_setvolume)(nVolume);
					OutTraceSND("> Volume=%d\n", nVolume);
				}
				return DoRetString("", lpszReturnString, cchReturn, sCmdTarget);
			}
			break;

	}

	if((dxw.CDADrive >= 'D') && (dxw.CDADrive <= 'Z')) {
		if(!_strnicmp(lpszCommand, "open cdaudio", strlen("open cdaudio"))){
			char sCommand[256+1];
			MCIERROR ret;
			sprintf(sCommand, "open %c: type cdaudio alias cdaudio%s", dxw.CDADrive, &lpszCommand[strlen("open cdaudio")]);
			OutTraceDW("%s: forced open device=%c: command=\"%s\"\n", api, dxw.CDADrive, lpszReturnString);
			IsWithinMCICall=TRUE;
			ret=(*pmciSendStringA)((LPCTSTR)lpszCommand, lpszReturnString, cchReturn, (HWND)hwndCallback);
			IsWithinMCICall=FALSE;
			_if(ret) OutTraceDW("%s ERROR: ret=%#x(%s)\n", api, ret, mmErrorString(ret));
			OutTraceDW("%s: RetString=\"%s\"\n", api, lpszReturnString);
			return ret;
		}
	}

	return 0;
}

// this is really ugly but for christ sake why did anyone use it?!
MCIERROR WINAPI extmciSendString(ApiArg, LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
#ifndef DXW_NOTRACES
	if(IsTraceSYS || IsTraceSND){
		OutTrace("%s: Command=\"%s\" retString=%#x retLength=%d Callback=%#x\n", api, lpszCommand, lpszReturnString, cchReturn, hwndCallback);
	}
#endif // DXW_NOTRACES

	MCIERROR ret;
	char sNickName[81];
	BOOL isCDAudio = FALSE;
	BOOL isMovie = FALSE;

	// v2.06.03: strip leading spaces
	// @#@ ???
	for(; *lpszCommand == ' '; lpszCommand++) ;
	// v2.06.07: strip trailing spaces
	// @#@ "The Queen of Hearts '99"
	//char *p = (char *)&(lpszCommand[strlen(lpszCommand)-1]);
	//for(; *p == ' '; *(p--) = 0) ;
	//OutTrace("%s: Command=\"%s\"\n", api, lpszCommand);

	// v2.06.09: tell whether cdaudio or movie ...
	if(strncmp(lpszCommand, "open ", 5)){ // for all commands except "open"
		char sCommand[81];
		if(sscanf(lpszCommand, "%s %s", sCommand, sNickName) == 2) {
			if(!strcmp(sNickName, "cdaudio")) isCDAudio = TRUE;

			if(!strcmp(sNickName, dxw.sAudioNickName)) isCDAudio = TRUE;
			if(!strcmp(sNickName, dxw.sMovieNickName)) isMovie = TRUE;

			// some commands imply a device type
			if(!(isCDAudio | isMovie)){
				if(!strcmp(sCommand, "window")) {
					isMovie = TRUE;
					strcpy(dxw.sMovieNickName, sNickName);
				}
			}
		}
	}
	else { // for "open" command
		if (strstr(lpszCommand, " type avivideo") || 
			strstr(lpszCommand, " type mpegvideo") ||
			!strcmp(lpszCommand, "open avivideo") || // @#@ Wizardry GOLD
			strstr(lpszCommand, ".avi ") ||
			strstr(lpszCommand, ".avi\"")){ // v2.06.10 @#@ "Hobie in de Jungle"
			isMovie = TRUE;
			char *alias = (char *)strstr(lpszCommand, " alias ");
			if(alias){
				sscanf(alias, " alias %s", dxw.sMovieNickName);
				OutTrace("%s: movie alias=%s\n", ApiRef, dxw.sMovieNickName);
			}
		}
		if(strstr(lpszCommand, " type cdaudio") ||
			strstr(lpszCommand, "open cdaudio ")||  // @#@ "Terracide"
			!strcmp(lpszCommand, "open cdaudio")) { // @#@ "The Adventures of Lomax"
			isCDAudio = TRUE;
			char *alias = (char *)strstr(lpszCommand, " alias ");
			if(alias){
				sscanf(alias, " alias %s", dxw.sAudioNickName);
			}
			else {
				strcpy(dxw.sAudioNickName, "cdaudio");
			}
			OutTrace("%s: audio alias=%s\n", ApiRef, dxw.sAudioNickName);
		}
	}

	// v2.05.47: remapping of "open path ..." commands with fake device. 
	// Used for "Goman" fake CD configuration.
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && (!strncmp(lpszCommand, "open ", 5))) {
		// v2.05.60: do not replace the path when the command is for a cdaudio device. 
		if(!strstr(lpszCommand, " type cdaudio") &&
			!strstr(lpszCommand, "open cdaudio")){ // naked "open cdaudio" command found in "XTom 3D"
			BOOL hasQuotes, needsQuotes;
			char *pathend, *pathbegin, *newpath; 
			char pathCopy[MAX_PATH+1];
			char stringCopy[2 * MAX_PATH];
			char *path = (char *)&lpszCommand[strlen("open ")];
			char *prefix;
			// v2.06.08: handling of <devtype>!<path> format
			if(strchr(path, '!')){
				path = strchr(path, '!') + 1;
			}
			pathbegin = path;
			prefix = path;
			if(*path=='\"') {
				path++; // v2.05.54 fix: path with double-quotes in Imperialism
				pathbegin = path;
				hasQuotes = TRUE;
				pathend = strchr(pathbegin, '\"');
			}
			else {
				pathbegin = path;
				hasQuotes = FALSE;
				pathend = strchr(pathbegin, ' ');
				if(!pathend) pathend = pathbegin + strlen(pathbegin); // if no further args ...
			}

			// copy the path
			char *pfrom, *pto;
			for(pfrom = pathbegin, pto=pathCopy; pfrom < pathend; pfrom++) *pto++ = *pfrom;
			*pto = 0;
			OutTrace("path=\"%s\" hasQuotes=%d\n", pathCopy, hasQuotes);
			newpath = (char *)dxwTranslatePathA(pathCopy, NULL);
			OutTrace("newpath=\"%s\" hasQuotes=%d\n", newpath, hasQuotes);
			needsQuotes = (strchr(newpath, ' ') != NULL); 
			if(hasQuotes) needsQuotes = TRUE; // leave quotes where they are, even if useless

			// now rebuild a new command string
			strncpy(stringCopy, lpszCommand, (prefix - lpszCommand));
			stringCopy[prefix - lpszCommand] = 0;
			if(needsQuotes) strcat(stringCopy, "\"");
			strcat(stringCopy, newpath);
			//free(newpath); // 2.06.00 fix
			if(needsQuotes & !hasQuotes) strcat(stringCopy, "\""); // v2.05.60 fix
			if(*pathend) strcat(stringCopy, pathend);
			OutTrace("newcommand=\"%s\"\n", stringCopy);
			IsWithinMCICall=TRUE;
			ret=(*pmciSendStringA)((LPCTSTR)stringCopy, lpszReturnString, cchReturn, (HWND)hwndCallback);
			IsWithinMCICall=FALSE;
			_if(ret) OutTraceDW("%s ERROR: ret=%#x\n", api, ret);
			OutTraceDW("%s: RetString=\"%s\"\n", api, lpszReturnString);
			return ret;
		}
	}

	IsWithinMCICall=TRUE;
	if(isMovie) 
		ret = mciSendMovieString(ApiRef, lpszCommand, lpszReturnString, cchReturn, (HWND)hwndCallback);
	else if(isCDAudio && (dxw.dwFlags8 & VIRTUALCDAUDIO)) 
		ret = mciSendAudioString(ApiRef, lpszCommand, lpszReturnString, cchReturn, (HWND)hwndCallback);
	else {
		OutTraceDW("%s: no audio/video mapping\n", ApiRef);
		ret=(*pmciSendStringA)(lpszCommand, lpszReturnString, cchReturn, (HWND)hwndCallback);
	}
	IsWithinMCICall=FALSE;

	BOOL returnsDWord = FALSE;
	// decides whether to log a string or a DWORD number
	if (!_strnicmp(lpszCommand, "window", strlen("window")) ||
		!_strnicmp(lpszCommand, "set", strlen("set")) ||	
		//!_strnicmp(lpszCommand, "status", strlen("status")) ||	
		!_strnicmp(lpszCommand, "configure", strlen("configure")) ||	
		!_strnicmp(lpszCommand, "seek", strlen("seek")) ||	
		!_strnicmp(lpszCommand, "put", strlen("put")) 
		) returnsDWord = TRUE;

	if(returnsDWord) {
		OutTraceSYS("%s: %s ret=%#x(%s) RetString=%#x\n", 
			api, ret ? "ERROR" : "OK", ret, mmErrorString(ret), lpszReturnString);
	}
	else {
		OutTraceSYS("%s: %s ret=%#x(%s) RetString=%s\n", 
			api, ret ? "ERROR" : "OK", ret, mmErrorString(ret), lpszReturnString ? lpszReturnString : "(NULL)");
	}

	if(dxw.dwFlags10 & CDROMPRESENT){
		// v2.04.81: tested with "Terracide" CDROM present check
		char sNickName[80+1];
		if (sscanf(lpszCommand, "status %s media present", sNickName) == 1){
			OutTraceDW("%s BYPASS: pretend CDROM is closed\n", api);
			if(cchReturn > strlen("true")) strcpy(lpszReturnString, "true");
		}
	}

	return ret;
}

