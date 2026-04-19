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

extern DWORD HandleStopCommand();
extern DWORD TTMMSSToAbsSec(DWORD);
extern char *sTimeFormat(DWORD);
extern char *mmErrorString(MMRESULT);
extern HANDLE hPlayer;
extern int cdVolume;
extern void start_player();
extern BOOL gb_RefreshSemaphore;
extern BOOL checkGap(char *, unsigned int);

static char *ansiIdentity = NULL;
static char *ansiProduct = NULL;
static WCHAR *wideIdentity = NULL;

MCIERROR WINAPI emumciSendCommand(BOOL, MCIDEVICEID, UINT, DWORD, DWORD_PTR);

static LPCSTR sDevType(LPCSTR dev)
{
	LPCSTR s;
	switch((DWORD)dev){
		case MCI_ALL_DEVICE_ID: s = "MCI_ALL_DEVICE_ID"; break;
		case MCI_DEVTYPE_VCR: s = "MCI_DEVTYPE_VCR"; break;
		case MCI_DEVTYPE_VIDEODISC: s = "MCI_DEVTYPE_VIDEODISC"; break;
		case MCI_DEVTYPE_OVERLAY: s = "MCI_DEVTYPE_OVERLAY"; break;
		case MCI_DEVTYPE_CD_AUDIO: s = "MCI_DEVTYPE_CD_AUDIO"; break;
		case MCI_DEVTYPE_DAT: s = "MCI_DEVTYPE_DAT"; break;
		case MCI_DEVTYPE_SCANNER: s = "MCI_DEVTYPE_SCANNER"; break;
		case MCI_DEVTYPE_ANIMATION: s = "MCI_DEVTYPE_ANIMATION"; break;
		case MCI_DEVTYPE_DIGITAL_VIDEO: s = "MCI_DEVTYPE_DIGITAL_VIDEO"; break;
		case MCI_DEVTYPE_OTHER: s = "MCI_DEVTYPE_OTHER"; break;
		case MCI_DEVTYPE_WAVEFORM_AUDIO: s = "MCI_DEVTYPE_WAVEFORM_AUDIO"; break;
		case MCI_DEVTYPE_SEQUENCER: s = "MCI_DEVTYPE_SEQUENCER"; break;
		default: s = "???";
	}
	return s;
}

static LPCWSTR swDevType(LPCWSTR dev)
{
	LPCWSTR s;
	switch((DWORD)dev){
		case MCI_ALL_DEVICE_ID: s = L"MCI_ALL_DEVICE_ID"; break;
		case MCI_DEVTYPE_VCR: s = L"MCI_DEVTYPE_VCR"; break;
		case MCI_DEVTYPE_VIDEODISC: s = L"MCI_DEVTYPE_VIDEODISC"; break;
		case MCI_DEVTYPE_OVERLAY: s = L"MCI_DEVTYPE_OVERLAY"; break;
		case MCI_DEVTYPE_CD_AUDIO: s = L"MCI_DEVTYPE_CD_AUDIO"; break;
		case MCI_DEVTYPE_DAT: s = L"MCI_DEVTYPE_DAT"; break;
		case MCI_DEVTYPE_SCANNER: s = L"MCI_DEVTYPE_SCANNER"; break;
		case MCI_DEVTYPE_ANIMATION: s = L"MCI_DEVTYPE_ANIMATION"; break;
		case MCI_DEVTYPE_DIGITAL_VIDEO: s = L"MCI_DEVTYPE_DIGITAL_VIDEO"; break;
		case MCI_DEVTYPE_OTHER: s = L"MCI_DEVTYPE_OTHER"; break;
		case MCI_DEVTYPE_WAVEFORM_AUDIO: s = L"MCI_DEVTYPE_WAVEFORM_AUDIO"; break;
		case MCI_DEVTYPE_SEQUENCER: s = L"MCI_DEVTYPE_SEQUENCER"; break;
		default: s = L"???";
	}
	return s;
}

static BOOL IsCDAudioTrackA(LPCSTR arg)
{
	int len;
	if(!arg) return FALSE;
	len = strlen(arg);
	if(len < 4) return FALSE;
	//OutTrace("ElementName=%s tail=%s\n", arg, &arg[len-4]);
	return !strcmp(&arg[len-4], ".cda"); 
}

static BOOL IsCDAudioTrackW(LPCWSTR arg)
{
	int len;
	if(!arg) return FALSE;
	len = wcslen(arg);
	if(len < 4) return FALSE;
	//OutTrace("ElementName=%s tail=%s\n", arg, &arg[len-4]);
	return !wcscmp(&arg[len-4], L".cda"); 
}

static DWORD dxwLockMciDeviceIdA()
{
	MCI_OPEN_PARMSA mciOpenParms;

	__try {
		memset(&mciOpenParms, 0, sizeof(MCI_OPEN_PARMSA));
		mciOpenParms.lpstrDeviceType = "waveaudio";
		int MCIERRret;
		if (MCIERRret = (*pmciSendCommandA)(0, MCI_OPEN | MCI_WAIT, MCI_OPEN_TYPE, (DWORD)(LPVOID) &mciOpenParms)){
			// Failed to open wave device:
			OutTrace("CDA DeviceId locking failed - res=%#x mciId=0xBEEF\n", MCIERRret);
			return 0xBEEF;
		}
		else{
			// Opened wave device succesfully:
			OutTrace("CDA DeviceId locked - mciId=%#x\n", mciOpenParms.wDeviceID);
			return mciOpenParms.wDeviceID;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTrace("CDA DeviceId locking exception - mciId=0xBEEF\n");
		return 0xBEEF;
	}
}

static DWORD dxwLockMciDeviceIdW()
{
	MCI_OPEN_PARMSW mciOpenParms;

	__try {
		memset(&mciOpenParms, 0, sizeof(MCI_OPEN_PARMSW));
		mciOpenParms.lpstrDeviceType = L"waveaudio";
		int MCIERRret;
		if (MCIERRret = (*pmciSendCommandW)(0, MCI_OPEN | MCI_WAIT, MCI_OPEN_TYPE, (DWORD)(LPVOID) &mciOpenParms)){
			// Failed to open wave device:
			OutTrace("CDA DeviceId locking failed - res=%#x mciId=0xBEEF\n", MCIERRret);
			return 0xBEEF;
		}
		else{
			// Opened wave device succesfully:
			OutTrace("CDA DeviceId locked - mciId=%#x\n", mciOpenParms.wDeviceID);
			return mciOpenParms.wDeviceID;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTrace("CDA DeviceId locking exception - mciId=0xBEEF\n");
		return 0xBEEF;
	}
}

static void dxwUnlockMciDeviceIdA(DWORD mciid)
{
	OutTrace("Unlocking mciId=%#x\n", mciid);
	if(mciid != 0xBEEF) (*pmciSendCommandA)(mciid, MCI_CLOSE, 0, NULL);
}

static void dxwUnlockMciDeviceIdW(DWORD mciid)
{
	OutTrace("Unlocking mciId=%#x\n", mciid);
	if(mciid != 0xBEEF) (*pmciSendCommandW)(mciid, MCI_CLOSE, 0, NULL);
}


static char *sTrackType(DWORD type)
{
	char *ret = "???";
	switch(type){
		case MCI_CDA_TRACK_AUDIO: ret = "audio"; break;
		case MCI_CDA_TRACK_OTHER: ret = "other"; break;
	}
	return ret;
}

static char *sDeviceType(DWORD dt)
{
	char *s;
	switch(dt){
		case MCI_ALL_DEVICE_ID: s="ALL_DEVICE_ID"; break;
		case MCI_DEVTYPE_VCR: s="VCR"; break;
		case MCI_DEVTYPE_VIDEODISC: s="VIDEODISC"; break;
		case MCI_DEVTYPE_OVERLAY: s="OVERLAY"; break;
		case MCI_DEVTYPE_CD_AUDIO: s="CD_AUDIO"; break;
		case MCI_DEVTYPE_DAT: s="DAT"; break;
		case MCI_DEVTYPE_SCANNER: s="SCANNER"; break;
		case MCI_DEVTYPE_ANIMATION: s="ANIMATION"; break;
		case MCI_DEVTYPE_DIGITAL_VIDEO: s="DIGITAL_VIDEO"; break;
		case MCI_DEVTYPE_OTHER: s="OTHER"; break;
		case MCI_DEVTYPE_WAVEFORM_AUDIO: s="WAVEFORM_AUDIO"; break;
		case MCI_DEVTYPE_SEQUENCER: s="SEQUENCER"; break;
		default: s="unknown"; break;
	}
	return s;
}

static char *sStatusItem(DWORD dwItem)
{
	char *s;
	switch(dwItem){
		case MCI_STATUS_LENGTH:				s = "LENGTH"; break;
		case MCI_STATUS_POSITION:			s = "POSITION"; break;
		case MCI_STATUS_NUMBER_OF_TRACKS:	s = "NUMBER_OF_TRACKS"; break;
		case MCI_STATUS_MODE:				s = "MODE"; break;
		case MCI_STATUS_MEDIA_PRESENT:		s = "MEDIA_PRESENT"; break;
		case MCI_STATUS_TIME_FORMAT:		s = "TIME_FORMAT"; break;
		case MCI_STATUS_READY:				s = "READY"; break;
		case MCI_STATUS_CURRENT_TRACK:		s = "CURRENT_TRACK"; break;
			// v2.04.85: CDA extension found in "Sentinel Returns" 
		case MCI_CDA_STATUS_TYPE_TRACK:		s = "CDA_STATUS_TYPE_TRACK"; break;
		default:							s = "???"; break;
	}
	return s;
}

static char *sCapType(DWORD dt)
{
	char *s;
	switch(dt){
		case MCI_GETDEVCAPS_CAN_RECORD:		s="CAN_RECORD"; break;
		case MCI_GETDEVCAPS_HAS_AUDIO:		s="HAS_AUDIO"; break;
		case MCI_GETDEVCAPS_HAS_VIDEO:		s="HAS_VIDEO"; break;
		case MCI_GETDEVCAPS_DEVICE_TYPE:	s="DEVICE_TYPE"; break;
		case MCI_GETDEVCAPS_USES_FILES:		s="USES_FILES"; break;
		case MCI_GETDEVCAPS_COMPOUND_DEVICE: s="COMPOUND_DEVICE"; break;
		case MCI_GETDEVCAPS_CAN_EJECT:		s="CAN_EJECT"; break;
		case MCI_GETDEVCAPS_CAN_PLAY:		s="CAN_PLAY"; break;
		case MCI_GETDEVCAPS_CAN_SAVE:		s="CAN_SAVE"; break;
		case MCI_DGV_GETDEVCAPS_CAN_FREEZE:	s="CAN_FREEZE"; break;
		case MCI_DGV_GETDEVCAPS_CAN_LOCK:	s="CAN_LOCK"; break;
		case MCI_DGV_GETDEVCAPS_CAN_REVERSE:s="CAN_REVERSE"; break;
		case MCI_DGV_GETDEVCAPS_CAN_STR_IN:	s="CAN_STR_IN"; break;
		case MCI_DGV_GETDEVCAPS_CAN_STRETCH:s="CAN_STRETCH"; break;
		case MCI_DGV_GETDEVCAPS_CAN_TEST:	s="CAN_TEST"; break;
		case MCI_DGV_GETDEVCAPS_HAS_STILL:	s="HAS_STILL"; break;
		case MCI_DGV_GETDEVCAPS_MAX_WINDOWS:s="MAX_WINDOWS"; break;
		case MCI_DGV_GETDEVCAPS_MINIMUM_RATE:s="MINIMUM_RATE"; break;
		case MCI_DGV_GETDEVCAPS_PALETTES:	s="PALETTES"; break;
		default: s="unknown"; break;
	}
	return s;
}

char *sTimeFormat(DWORD tf)
{
	char *s;
	switch(tf){
		case MCI_FORMAT_MILLISECONDS: s="MILLISECONDS"; break;
		case MCI_FORMAT_HMS: s="HMS"; break;
		case MCI_FORMAT_MSF: s="MSF"; break;
		case MCI_FORMAT_FRAMES: s="FRAMES"; break;
		case MCI_FORMAT_SMPTE_24: s="SMPTE_24"; break;
		case MCI_FORMAT_SMPTE_25: s="SMPTE_25"; break;
		case MCI_FORMAT_SMPTE_30: s="SMPTE_30"; break;
		case MCI_FORMAT_SMPTE_30DROP: s="SMPTE_30DROP"; break;
		case MCI_FORMAT_BYTES: s="BYTES"; break;
		case MCI_FORMAT_SAMPLES: s="SAMPLES"; break;
		case MCI_FORMAT_TMSF: s="TMSF"; break;
		default: s="unknown"; break;
	}
	return s;
}

static char *ExplainMCICapability(DWORD item)
{
	char *labels[]={
		"???",
		"CAN_RECORD",
		"HAS_AUDIO",
		"HAS_VIDEO",
		"DEVICE_TYPE",
		"USES_FILES",
		"COMPOUND_DEVICE",
		"CAN_EJECT",
		"CAN_PLAY",
		"CAN_SAVE"
	};

	if((item >= MCI_GETDEVCAPS_CAN_RECORD) && (item <= MCI_GETDEVCAPS_CAN_SAVE)) return labels[item];
	return labels[0];
}

static DWORD DecodeSec(DWORD sec)
{
	DWORD res = 0;
	DWORD min, hours;
	switch (mciEmu.dwTimeFormat) {
		case MCI_FORMAT_MILLISECONDS:
			res = 1000 * sec;
			break;
		//case MCI_FORMAT_TMSF:
		//	min = sec / 60;
		//	sec = sec % 60;
		//	res = MCI_MAKE_TMSF(0, min, sec, 0);
		//	break;
		// v2.04.99: BEWARE! though the time format could be set to TMSF format, track lengths in that
		// case must be returned in MSF format. Fixes "Sentinel Returns" regression bug after v2.04.96
		case MCI_FORMAT_MSF:
		case MCI_FORMAT_TMSF:
			min = sec / 60;
			sec = sec % 60;
			res = MCI_MAKE_MSF(min, sec, 0);
			break;
		case MCI_FORMAT_HMS:
			hours = sec / 3600;
			sec = sec - (2600 * hours);
			min = sec / 60;
			sec = sec % 60;
			res = MCI_MAKE_HMS(hours, min, sec);
			break;
	}
	OutDebugSND("> DecodeSec(%d) -> format=%s res=%08.8X\n", sec, sTimeFormat(mciEmu.dwTimeFormat), res);
	return res;
}



#ifndef DXW_NOTRACES
static void DumpMciMessage(char *api, BOOL isAnsi, BOOL isOutput, DWORD IDDevice, UINT uMsg, DWORD dwFlags, DWORD_PTR dwParam)
{
	int iParamSize = 0;
	char sFlags[512+1];
	char sBuf[512+1];
	if(isOutput){
		sprintf(sBuf, "%s: < mciId=%#x msg=%#x(%s)", 
			api, IDDevice, uMsg, ExplainMCICommands(uMsg));
	}
	else {
		sprintf(sBuf, "%s: > mciId=%#x msg=%#x(%s) flags=%#x(%s) cback=%#x", 
			api, IDDevice, uMsg, ExplainMCICommands(uMsg), 
			dwFlags, ExplainMCIFlags(uMsg, dwFlags, sFlags, 512),
			dwParam ? ((LPMCI_GENERIC_PARMS)dwParam)->dwCallback : 0);
	}
	switch(uMsg){
		case MCI_BREAK: 
			{
				LPMCI_BREAK_PARMS lpBreak = (LPMCI_BREAK_PARMS)dwParam;
				OutTrace("%s virtkey=%d hwndbreak=%#x\n",
					sBuf, lpBreak->nVirtKey, lpBreak->hwndBreak);
				iParamSize = sizeof(MCI_BREAK_PARMS);
			}
			break;
		case MCI_INFO: 
			{
				LPMCI_INFO_PARMS lpInfo = (LPMCI_INFO_PARMS)dwParam;
				if(isOutput){
					int l = lpInfo->dwRetSize;
					if(isAnsi) {
						OutTrace("%s retsize=%#x str=\"%*.*s\"\n", sBuf, lpInfo->dwRetSize, l, l, lpInfo->lpstrReturn);
					}
					else {
						OutTrace("%s retsize=%#x str=\"%*.*ls\"\n", sBuf, lpInfo->dwRetSize, l, l, lpInfo->lpstrReturn);
					}
				}
				else {
					OutTrace("%s retsize=%#x\n", sBuf, lpInfo->dwRetSize);
				}
				iParamSize = sizeof(MCI_INFO_PARMS);
			}
			break;
		case MCI_PLAY: 
			{
				LPMCI_PLAY_PARMS lpPlay = (LPMCI_PLAY_PARMS)dwParam;
				if(lpPlay){
					char sFromTo[81];
					strcpy(sFromTo, ""); // initialize empty
					if(dwFlags & MCI_FROM) sprintf(sFromTo, " from=%#x", lpPlay->dwFrom);
					if(dwFlags & MCI_TO) sprintf(&sFromTo[strlen(sFromTo)], " to=%#x", lpPlay->dwTo);
					OutTrace("%s%s\n", sBuf, sFromTo);
					iParamSize = sizeof(MCI_PLAY_PARMS);
				}
				else {
					OutTrace("%s\n", sBuf);
					iParamSize = 0;
				}
			}
			break;
		case MCI_GETDEVCAPS: 
			{
				LPMCI_GETDEVCAPS_PARMS lpDevCaps = (LPMCI_GETDEVCAPS_PARMS)dwParam; 
				if(isOutput){
					OutTrace("%s item=%#x(%s) ret=%#x\n",
						sBuf, 
						lpDevCaps->dwItem, sCapType(lpDevCaps->dwItem), 
						lpDevCaps->dwReturn);
				}
				else {
					OutTrace("%s item=%#x(%s)\n",
						sBuf, 
						lpDevCaps->dwItem, sCapType(lpDevCaps->dwItem));
				}
				iParamSize = sizeof(MCI_GETDEVCAPS_PARMS);
			}
			break;
		case MCI_OPEN: 
			{
				// how to dump LPMCI_OPEN_PARMS strings without crash?
				if(isAnsi){
					LPMCI_OPEN_PARMSA lpOpen = (LPMCI_OPEN_PARMSA)dwParam;
					OutTrace("%s devid=%#x devtype=%s elementname=\"%s\" alias=\"%s\"\n",
						sBuf, 
						lpOpen->wDeviceID,
						(dwFlags & MCI_OPEN_TYPE) ? (
							(dwFlags & MCI_OPEN_TYPE_ID) ? 
								sDevType(lpOpen->lpstrDeviceType)
								:
								lpOpen->lpstrDeviceType
							) : "",
						(dwFlags & MCI_OPEN_ELEMENT) ? lpOpen->lpstrElementName : "",
						(dwFlags & MCI_OPEN_ALIAS) ? lpOpen->lpstrAlias : "");
					iParamSize = sizeof(MCI_OPEN_PARMSA);
				}
				else{
					LPMCI_OPEN_PARMSW lpOpen = (LPMCI_OPEN_PARMSW)dwParam;
					OutTrace("%s devid=%#x devtype=%ls elementname=\"%ls\" alias=\"%ls\"\n",
						sBuf,
						lpOpen->wDeviceID,
						(dwFlags & MCI_OPEN_TYPE) ? (
							(dwFlags & MCI_OPEN_TYPE_ID) ? 
								swDevType(lpOpen->lpstrDeviceType)
								:
								lpOpen->lpstrDeviceType
							) : L"", 
						(dwFlags & MCI_OPEN_ELEMENT) ? lpOpen->lpstrElementName : L"",
						(dwFlags & MCI_OPEN_ALIAS) ? lpOpen->lpstrAlias : L"");
					iParamSize = sizeof(MCI_OPEN_PARMSW);
				}
			}
			break;
		case MCI_STATUS:
			{
				LPMCI_STATUS_PARMS lpStatus = (LPMCI_STATUS_PARMS)dwParam;
				if(isOutput){
					OutTrace("%s item=%#x(%s) track=%#x ret=%#x\n",
						sBuf, lpStatus->dwItem, sStatusItem(lpStatus->dwItem), lpStatus->dwTrack, lpStatus->dwReturn);
				}
				else {
					OutTrace("%s item=%#x(%s) track=%#x\n",
						sBuf, lpStatus->dwItem, sStatusItem(lpStatus->dwItem), lpStatus->dwTrack);
				}
				iParamSize = sizeof(MCI_STATUS_PARMS);
			}
			break;
		case MCI_SYSINFO:
			{
				LPMCI_SYSINFO_PARMS lpSysInfo = (LPMCI_SYSINFO_PARMS)dwParam;
				if(isOutput){
					if(dwFlags & MCI_SYSINFO_QUANTITY){
						OutTrace("%s retsize=%#x retbuf=%d number=%#x devtype=%#x(%s)\n",
							sBuf, lpSysInfo->dwRetSize, 
							*(DWORD *)lpSysInfo->lpstrReturn, 
							lpSysInfo->dwNumber, lpSysInfo->wDeviceType, sDeviceType(lpSysInfo->wDeviceType));
					}
					else {
						OutTrace("%s retsize=%#x retbuf=%s number=%#x devtype=%#x(%s)\n",
							sBuf, lpSysInfo->dwRetSize, 
							lpSysInfo->lpstrReturn,
							lpSysInfo->dwNumber, lpSysInfo->wDeviceType, sDeviceType(lpSysInfo->wDeviceType));
					}
				}
				else {
					OutTrace("%s retsize=%#x devtype=%#x(%s)\n",
						sBuf,
						lpSysInfo->dwRetSize, 
						lpSysInfo->wDeviceType, sDeviceType(lpSysInfo->wDeviceType));
				}
				iParamSize = sizeof(MCI_SYSINFO_PARMS);
			}
			break;
		case MCI_SET:
			{
				LPMCI_SET_PARMS lpSetInfo = (LPMCI_SET_PARMS)dwParam;
				OutTrace("%s audio=%#x timeformat=%#x(%s)\n",
					sBuf, lpSetInfo->dwAudio, lpSetInfo->dwTimeFormat, sTimeFormat(lpSetInfo->dwTimeFormat));
				iParamSize = sizeof(MCI_SET_PARMS);
			}
			break;
		case MCI_SEEK:
			{
				// v2.05.00 added trace
				LPMCI_SEEK_PARMS lpSeekInfo = (LPMCI_SEEK_PARMS)dwParam;
				if(lpSeekInfo){
					OutTrace("%s to=%#x\n", sBuf, lpSeekInfo->dwTo);
				}
				else {
					OutTrace("%s seekinfo=(NULL)\n", sBuf);
				}
				iParamSize = sizeof(MCI_SEEK_PARMS);
			}
			break;
		case MCI_WHERE:
			{
				// v2.06.01 added trace
				LPMCI_GENERIC_PARMS lpWhereInfo = (LPMCI_GENERIC_PARMS)dwParam;
				if(isOutput){
					char sWhere[80+1];
					sWhere[0] = 0;
					if(dwFlags & (MCI_DGV_WHERE_WINDOW | MCI_DGV_WHERE_DESTINATION | MCI_DGV_WHERE_SOURCE)){
						LPMCI_DGV_RECT_PARMS lpRectParms = (LPMCI_DGV_RECT_PARMS)dwParam;
						sprintf(sWhere, " offset=(%d,%d) extent=(%dx%d)",
							lpRectParms->rc.left, lpRectParms->rc.top, lpRectParms->rc.right, lpRectParms->rc.bottom);
					}
					OutTrace("%s%s\n", sBuf, sWhere);
				}
				else {
					OutTrace("%s info=(NULL)\n", sBuf);
				}
				iParamSize = sizeof(MCI_DGV_RECT_PARMS);
			}
			break;
		case MCI_WINDOW:
			{
				LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)dwParam;
				if(lpGeneric)
					OutTrace("%s\n", sBuf);
				else
					OutTrace("%s params=(NULL)\n", sBuf);
				iParamSize = sizeof(MCI_GENERIC_PARMS);
			}
			break;
		case MCI_PUT:
			{
				LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)dwParam;
				if(lpGeneric)
					OutTrace("%s\n", sBuf);
				else
					OutTrace("%s params=(NULL)\n", sBuf);
				iParamSize = sizeof(MCI_GENERIC_PARMS);
			}
			break;
		default:
			{
				LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)dwParam;
				if(lpGeneric)
					OutTrace("%s\n", sBuf);
				else
					OutTrace("%s params=(NULL)\n", sBuf);
				iParamSize = sizeof(MCI_GENERIC_PARMS);
			}
			break;
	}
	OutHexSND((LPBYTE)dwParam, iParamSize);
}
#endif

static WCHAR *wideProduct = NULL;

MCIERROR WINAPI BypassmciSendCommand(ApiArg, BOOL isAnsi, mciSendCommand_Type pmciSendCommand, MCIDEVICEID IDDevice, UINT uMsg, DWORD dwFlags, DWORD_PTR dwParam)
{
	MCIERROR ret;
	MCI_STATUS_PARMS *sp;
	LPMCI_OPEN_PARMSA po;

	ret = 0;
	switch(uMsg){
		case MCI_OPEN:
			po = (MCI_OPEN_PARMSA *)dwParam;
			po->wDeviceID = 1;
			break;
		case MCI_STATUS:
			if(dwFlags & MCI_STATUS_ITEM){
				// fix for Tie Fighter 95: when bypassing, let the caller know you have no CD tracks
				// otherwise you risk an almost endless loop going through the unassigned returned 
				// number of ghost tracks
				// fix for "Emperor of the Fading Suns": the MCI_STATUS_ITEM is set in .or. with
				// MCI_TRACK
				sp = (MCI_STATUS_PARMS *)dwParam;
				switch(dwFlags){
					case MCI_TRACK:
						sp->dwReturn = 1;
						break;
					default:
						sp->dwTrack = 0;
						if(sp->dwItem == MCI_STATUS_CURRENT_TRACK) sp->dwTrack = 1;
						if(sp->dwItem == MCI_STATUS_NUMBER_OF_TRACKS) sp->dwTrack = 1;
						if(sp->dwItem == MCI_STATUS_LENGTH) sp->dwTrack = 200;
						if(sp->dwItem == MCI_STATUS_MEDIA_PRESENT) sp->dwTrack = 1;
						sp->dwReturn = 0;
						break;
				}
			}
			break;
		case MCI_CLOSE:
		default:
			break;
	}
#ifndef DXW_NOTRACES
	if(IsTraceSND) DumpMciMessage(ApiRef, isAnsi, TRUE, IDDevice, uMsg, dwFlags, dwParam);
#endif // DXW_NOTRACES
	return ret;
}

static BOOL isAudioA(LPCSTR devType)
{
	if (LOWORD(devType) == MCI_DEVTYPE_CD_AUDIO) return TRUE;
	else
	if (devType && !_strnicmp(devType, "cdaudio", strlen("cdaudio"))) return TRUE;
	return FALSE;
}

static BOOL isAudioW(LPCWSTR devType)
{
	if (LOWORD(devType) == MCI_DEVTYPE_CD_AUDIO) return TRUE;
	else
	if (devType && !_wcsnicmp(devType, L"cdaudio", wcslen(L"cdaudio"))) return TRUE;
	return FALSE;
}

MCIERROR WINAPI extmciSendCommand(ApiArg, BOOL isAnsi, mciSendCommand_Type pmciSendCommand, MCIDEVICEID IDDevice, UINT uMsg, DWORD dwFlags, DWORD_PTR dwParam)
{
	RECT saverect = dxw.GetScreenRect(); // v2.06.08: initialize ...
	MCIERROR ret;
	MCI_OVLY_WINDOW_PARMSA *pwa = (MCI_OVLY_WINDOW_PARMSA *)dwParam;
	MCI_OVLY_WINDOW_PARMSW *pww = (MCI_OVLY_WINDOW_PARMSW *)dwParam;
	LPMCI_ANIM_RECT_PARMS pr;

#ifndef DXW_NOTRACES
	if(IsTraceSYS || IsTraceSND) DumpMciMessage(ApiRef, isAnsi, FALSE, IDDevice, uMsg, dwFlags, dwParam);
#endif

	if(uMsg == 0){
		OutTraceSND("%s: special msg=NULL\n", api);
		return 0;
	}

	if(dxw.dwFlags6 & BYPASSMCI){
		switch(uMsg){
			case MCI_OPEN:
			case MCI_CLOSE:
			case MCI_STATUS:
				return BypassmciSendCommand(ApiRef, isAnsi, pmciSendCommand, IDDevice, uMsg, dwFlags, dwParam);
				break;
		}
	}

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) { 
		if((uMsg == MCI_OPEN) && (dwFlags & MCI_OPEN_ELEMENT)){
			DWORD dtype = DXW_NO_FAKE;
			if(isAnsi){
				LPMCI_OPEN_PARMSA op = (LPMCI_OPEN_PARMSA)dwParam;
				if(op->lpstrElementName && !isAudioA(op->lpstrDeviceType)) {
					op->lpstrElementName = dxwTranslatePathA(op->lpstrElementName, &dtype);
					OutTraceSND("%s: translated path=\"%s\"\n", api, op->lpstrElementName);
				}
			}
			else {
				LPMCI_OPEN_PARMSW op = (LPMCI_OPEN_PARMSW)dwParam;
				if(op->lpstrElementName && !isAudioW(op->lpstrDeviceType)) {
					op->lpstrElementName = dxwTranslatePathW(op->lpstrElementName, &dtype);
					OutTraceSND("%s: translated path=\"%ls\"\n", api, op->lpstrElementName);
				}
			}
			// "Twisted Metal 2": MCI_WAIT+OPEN_SHAREABLE+OPEN_ELEMENT+OPEN_TYPE_ID+OPEN_TYPE
			// using Fake CD the CD drive becomes a disk folder that can't be opened with shareable
			// access, so better trim the flag down. Too bad it's not enough to fix all crashes ...
			if((dtype == DXW_FAKE_CD) && (dwFlags & MCI_OPEN_SHAREABLE)){
				OutTraceSND("%s: suppress MCI_OPEN_SHAREABLE on fake CD\n", api);
				dwFlags &= ~MCI_OPEN_SHAREABLE;
			}
		}
	}

	// v2.05.30: fix - suppress fullscreen movie play
	DWORD FullScreenModes = MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2;
	if(dxw.Windowize && (uMsg == MCI_PLAY) && (dwFlags & FullScreenModes)){
		OutTraceSND("%s: suppress MCI_PLAY FULLSCREEN flag\n", api);
		dwFlags &= ~FullScreenModes;
		dwFlags |= MCI_MCIAVI_PLAY_WINDOW;
	}

	if(dxw.dwFlags8 & VIRTUALCDAUDIO) {
		BOOL detour = FALSE;
		// v2.05.76: fix - mciSendCommand(MCI_SYSINFO) doesn't need to operate on specific devices,
		// so you'd better decide for detour value looking at the wDeviceType field that could
		// be set to MCI_DEVTYPE_CD_AUDIO.
		if (uMsg == MCI_SYSINFO && 
			(((LPMCI_SYSINFO_PARMSA)dwParam)->wDeviceType == MCI_DEVTYPE_CD_AUDIO)) {
			detour = TRUE;
			OutTrace("detour=TRUE\n");		
		}
		if (uMsg == MCI_OPEN){
			// v2.06.08 fix: alias name remains valid until next CD device MCI_CLOSE
			//strcpy(dxw.sAudioNickName, "");
			if(isAnsi){
				LPMCI_OPEN_PARMSA parms = (LPMCI_OPEN_PARMSA)dwParam;
				if (dwFlags & MCI_OPEN_TYPE) {
					if (dwFlags & MCI_OPEN_TYPE_ID){
						if (LOWORD(parms->lpstrDeviceType) == MCI_DEVTYPE_CD_AUDIO) detour=TRUE;
					}
					else {
						if (parms->lpstrDeviceType && !_strnicmp(parms->lpstrDeviceType, "cdaudio", strlen("cdaudio"))) detour=TRUE;
					}
				}
				if ((dwFlags & MCI_OPEN_ELEMENT) && IsCDAudioTrackA(parms->lpstrElementName)) // v2.04.95 - "Absolute Terror" 
					detour=TRUE;

				// v2.05.70: registering device alias name for mciSendString calls
				if(detour && (dwFlags & MCI_OPEN_ALIAS)) { 
					strncpy(dxw.sAudioNickName, parms->lpstrAlias, 80);
					dxw.sAudioNickName[80]=0;
					OutTraceSND("%s: MCI_OPEN_ALIAS alias=%s\n", api, dxw.sAudioNickName);
				}
			}
			else {
				LPMCI_OPEN_PARMSW parms = (LPMCI_OPEN_PARMSW)dwParam;
				if (dwFlags & MCI_OPEN_TYPE) {
					if (dwFlags & MCI_OPEN_TYPE_ID){
						if (LOWORD(parms->lpstrDeviceType) == MCI_DEVTYPE_CD_AUDIO) detour=TRUE;
					}
					else {
						if (parms->lpstrDeviceType && !_wcsnicmp(parms->lpstrDeviceType, L"cdaudio", wcslen(L"cdaudio"))) detour=TRUE;
					}
				}
				if ((dwFlags & MCI_OPEN_ELEMENT) && IsCDAudioTrackW(parms->lpstrElementName)) 
					detour=TRUE;
				// v2.05.70: registering device alias name for mciSendString calls
				if(detour && (dwFlags & MCI_OPEN_ALIAS)) { 
					wcstombs(dxw.sAudioNickName, parms->lpstrAlias, 80);
					dxw.sAudioNickName[80]=0;
					OutTraceSND("%s: MCI_OPEN_ALIAS alias=%s\n", api, dxw.sAudioNickName);
				}
			}
			if(detour && (dxw.dwFlags12 & SUPPRESSCDAUDIO)) {
				OutTraceSND("%s: suppress CDAUDIO ret=MCIERR_DEVICE_NOT_INSTALLED\n", api);
				ret = MCIERR_DEVICE_NOT_INSTALLED;
				return ret;
			}
			if(isAnsi) dxw.VirtualCDAudioDeviceId = dxwLockMciDeviceIdA();
			else dxw.VirtualCDAudioDeviceId = dxwLockMciDeviceIdW();
			OutTrace("detour=%d\n", detour);
		}
		else {
			if ((dxw.dwFlags8 & IGNOREMCIDEVID) ||
				// mciEmu.dwDevID is set in MCI_OPEN and cleared in MCI_CLOSE
				(IDDevice == mciEmu.dwDevID) || 
				(IDDevice == AUX_MAPPER)
			){
				switch (uMsg){
					case MCI_SET:
					case MCI_CLOSE:
					case MCI_SEEK:
					case MCI_PLAY:
					case MCI_STOP:
					case MCI_PAUSE:
					case MCI_RESUME: 
					case MCI_STATUS:
					case MCI_INFO:
					case MCI_SYSINFO: // v2.04.97: "Heavy Gear"
					case MCI_GETDEVCAPS: // v2.04.95: "Absolute Terror"
					case MCI_WHERE: // v2.04.95: "Absolute Terror"
						detour=TRUE;
						break;
				}
			}
		}
		if(detour) {
			if(mciEmu.dwDevID == 0xFFF0){
				switch (uMsg){
					case MCI_OPEN:
					case MCI_SYSINFO:
						break;
					default:
						OutTraceSND("%s: MCI session closed! ret=MCIERR_INVALID_DEVICE_ID\n", ApiRef);
						return MCIERR_INVALID_DEVICE_ID; 
				}
			}

			ret = emumciSendCommand(isAnsi, IDDevice, uMsg, dwFlags, dwParam);
			// v2.05.50: set / clear the mciEmu.dwDevID value to avoid routing to CD audio emulation
			// commands directed to other mci devices, like movie player. 
			// Fixes "Gooch Grundy's X-Decathlon" when setting CD audio emulation
			if(!ret) {
				if(uMsg == MCI_OPEN)  {
					mciEmu.dwDevID = ((MCI_OPEN_PARMS *)dwParam)->wDeviceID;
				}
				if(uMsg == MCI_CLOSE) {
					if(isAnsi) dxwUnlockMciDeviceIdA(mciEmu.dwDevID);
					else dxwUnlockMciDeviceIdW(mciEmu.dwDevID);
					mciEmu.dwDevID = 0xFFF0; // almost impossible value ...
					strcpy(dxw.sAudioNickName, "");
				}
			}
#ifndef DXW_NOTRACES
			if(IsTraceSYS || IsTraceSND) DumpMciMessage(ApiRef, isAnsi, TRUE, IDDevice, uMsg, dwFlags, dwParam);
#endif
			return ret;
		}
	}

	// v2.06.06: intercepting the error condition is needed to avoid crash in @#@ "Eryner" (Ko 1997)
	// beware: the IDDevice == 0 is a valid value when asking for global information like quantities
	if((IDDevice == 0) && (uMsg == MCI_SYSINFO) && !(dwFlags & (MCI_SYSINFO_QUANTITY | MCI_SYSINFO_OPEN))){
		OutTraceSND("%s: intercept iddev=0 ret=MCIERR_INVALID_DEVICE_ID\n", api);
		return MCIERR_INVALID_DEVICE_ID;
	}

	if((dxw.dwFlags5 & REMAPMCI) && (dxw.IsFullScreen())) {
		switch(uMsg){
			case MCI_WHERE:
				if(dxw.dwFlags6 & STRETCHMOVIES) {
					if(dwFlags & (MCI_OVLY_WHERE_DESTINATION | MCI_ANIM_WHERE_DESTINATION)){
						//MessageBox(0, "WHERE", "dxwnd", 0);
						LPMCI_OVLY_RECT_PARMS pOverlay = (LPMCI_OVLY_RECT_PARMS)dwParam;
						pOverlay->rc.left = 0;
						pOverlay->rc.top = 0;
						pOverlay->rc.right = dxw.iSizX;
						pOverlay->rc.bottom = dxw.iSizY;
						OutTraceDW("%s: MCI_WHERE fixed pos=(%d, %d) size=(%d x %d)\n", 
							ApiRef, pOverlay->rc.left, pOverlay->rc.top, pOverlay->rc.right, pOverlay->rc.bottom);
					}
				}
				break;
			case MCI_WINDOW:
				if(dxw.dwFlags6 & STRETCHMOVIES) {
					// Sends the window client coordinates before the MCI_WINDOW command,
					// just in case the application omits that.
					MCI_ANIM_RECT_PARMS aparam;
					pr = &aparam;
					uMsg = MCI_PUT;
					pr->dwCallback = 0;
					(*pGetClientRect)(dxw.GethWnd(), &pr->rc);
					dwFlags = MCI_ANIM_RECT | MCI_ANIM_PUT_DESTINATION;
					ret=(*pmciSendCommand)(IDDevice, uMsg, dwFlags, (DWORD_PTR)&aparam);
					_if(ret) OutTraceE("%s: MCI_PUT ERROR res=%d\n", api, ret);
					uMsg = MCI_WINDOW;
				}

				if(isAnsi){
					OutDebugDW("%s: hwnd=%#x CmdShow=%#x text=\"%s\"\n", 
						api, pwa->hWnd, pwa->nCmdShow, pwa->lpstrText);
					if(dxw.IsRealDesktop(pwa->hWnd)) {
						pwa->hWnd = dxw.GethWnd();
						OutDebugDW("%s: REDIRECT hwnd=%#x\n", api, pwa->hWnd);
					}
					if(dxw.dwFlags6 & STRETCHMOVIES) {
						dwFlags = MCI_ANIM_WINDOW_HWND | MCI_OVLY_WINDOW_ENABLE_STRETCH;
						pwa->lpstrText = 0;
						pwa->hWnd = dxw.GethWnd();
						pwa->nCmdShow = 0;
					}
				}
				else {
					OutDebugDW("%s: hwnd=%#x CmdShow=%#x text=\"%ls\"\n", 
						api, pww->hWnd, pww->nCmdShow, pww->lpstrText);
					if(dxw.IsRealDesktop(pww->hWnd)) {
						pww->hWnd = dxw.GethWnd();
						OutDebugDW("%s: REDIRECT hwnd=%#x\n", api, pww->hWnd);
					}
					if(dxw.dwFlags6 & STRETCHMOVIES) {
						dwFlags = MCI_ANIM_WINDOW_HWND | MCI_OVLY_WINDOW_ENABLE_STRETCH;
						pww->lpstrText = 0;
						pww->hWnd = dxw.GethWnd();
						pww->nCmdShow = 0;
					}
				}

				// attempt to stretch "Wizardry Chronicle" intro movie, but it doesn't work ...
				//if(1){
				//	dwFlags &= ~MCI_OVLY_WINDOW_DISABLE_STRETCH;
				//	dwFlags |= MCI_OVLY_WINDOW_ENABLE_STRETCH;
				//	dwFlags |= MCI_ANIM_WINDOW_HWND;
				//	OutDebugDW("mciSendCommand: STRETCH flags=%#x hwnd=%#x\n", dwFlags, pw->hWnd);
				//}
				break;
			case MCI_PUT:
				RECT client;
				pr = (MCI_ANIM_RECT_PARMS *)dwParam;
				OutTraceDW("%s: rect=(%d,%d),(%d,%d)\n", api, pr->rc.left, pr->rc.top, pr->rc.right, pr->rc.bottom);
				if(dwFlags & MCI_ANIM_PUT_DESTINATION){
					dwFlags |= MCI_ANIM_RECT;
					saverect=pr->rc;
					if(dxw.dwFlags6 & STRETCHMOVIES){
						pr->rc.top = 0;
						pr->rc.left = 0;
						pr->rc.right = dxw.iSizX;
						pr->rc.bottom = dxw.iSizY;
					}
					else {
						(*pGetClientRect)(dxw.GethWnd(), &client);
						pr->rc.top = (pr->rc.top * client.bottom) / dxw.GetScreenHeight();
						pr->rc.bottom = (pr->rc.bottom * client.bottom) / dxw.GetScreenHeight();
						pr->rc.left = (pr->rc.left * client.right) / dxw.GetScreenWidth();
						pr->rc.right = (pr->rc.right * client.right) / dxw.GetScreenWidth();
					}
					OutTraceDW("%s: fixed rect=(%d,%d),(%d,%d)\n", api, pr->rc.left, pr->rc.top, pr->rc.right, pr->rc.bottom);
				}
				break;
			case MCI_PLAY:
				if(dxw.dwFlags6 & NOMOVIES) return 0; // ???
				gb_RefreshSemaphore = TRUE;
				//OutTrace("set gb_RefreshSemaphore=%d\n", gb_RefreshSemaphore);
				break;
			case MCI_OPEN:
				if(dxw.dwFlags6 & NOMOVIES) return 275; // quite brutal, but working ....
				break;
			case MCI_STOP:
				if(dxw.dwFlags6 & NOMOVIES) return 0; // ???
				break;
			case MCI_CLOSE:
				if(dxw.dwFlags6 & NOMOVIES) return 0; // ???
				gb_RefreshSemaphore = FALSE;
				//OutTrace("set gb_RefreshSemaphore=%d\n", gb_RefreshSemaphore);
				break;
		}
	}
	else {
		if(uMsg == MCI_PUT){
			pr = (MCI_ANIM_RECT_PARMS *)dwParam;
			OutDebugDW("%s: rect=(%d,%d),(%d,%d)\n", api, pr->rc.left, pr->rc.top, pr->rc.right, pr->rc.bottom);
			// trim the rect width to 1 pixel less that screen width because you could 
			// get strange artifacts (see Julia intro movies).
			if(pr->rc.right == dxw.GetScreenWidth()) {
				pr->rc.right = dxw.GetScreenWidth() - 1;
				OutTraceSND("%s: fixed rect=(%d,%d),(%d,%d)\n", api, pr->rc.left, pr->rc.top, pr->rc.right, pr->rc.bottom);
			}
		}
	}

#ifndef DXW_NOTRACES
	if((uMsg == MCI_OPEN) && (dwFlags & MCI_OPEN_ELEMENT)){
		if(isAnsi){
			if(IsTraceDW || IsTraceSND){
				LPMCI_OPEN_PARMSA lpOpenA = (LPMCI_OPEN_PARMSA)dwParam;
				if((dwFlags & MCI_OPEN_TYPE) && !(dwFlags & MCI_OPEN_TYPE_ID)) 
					OutTrace("%s: lpstrDeviceType=%s\n", ApiRef, lpOpenA->lpstrDeviceType);
				if(dwFlags & MCI_OPEN_TYPE_ID)
					OutTrace("%s: lpstrDeviceType=%#x (type=%s ord=%d)\n", 
					ApiRef, 
					lpOpenA->lpstrDeviceType, 
					sDeviceType((DWORD)(lpOpenA->lpstrDeviceType) & 0x0000FFFF),
					((DWORD)(lpOpenA->lpstrDeviceType) & 0xFFFF0000) >> 16
					);
				if((dwFlags & MCI_OPEN_ALIAS) && lpOpenA->lpstrAlias)
					OutTrace("%s: lpstrAlias=%s\n", ApiRef, lpOpenA->lpstrAlias);
				if(lpOpenA->lpstrElementName) OutTrace("%s: lpstrElementName=%s\n", ApiRef, lpOpenA->lpstrElementName);
			}
		}
		else {
			LPMCI_OPEN_PARMSW lpOpenW = (LPMCI_OPEN_PARMSW)dwParam;
			if(IsTraceDW || IsTraceSND){
				if((dwFlags & MCI_OPEN_TYPE) && !(dwFlags & MCI_OPEN_TYPE_ID)) 
					OutTrace("%s: lpstrDeviceType=%ls\n", ApiRef, lpOpenW->lpstrDeviceType);
				if(dwFlags & MCI_OPEN_TYPE_ID) 
					OutTrace("%s: lpstrDeviceType=%#x (type=%s ord=%d)\n", 
					ApiRef, 
					lpOpenW->lpstrDeviceType, 
					sDeviceType((DWORD)(lpOpenW->lpstrDeviceType) & 0x0000FFFF),
					((DWORD)(lpOpenW->lpstrDeviceType) & 0xFFFF0000) >> 16
					);
				if((dwFlags & MCI_OPEN_ALIAS) && lpOpenW->lpstrAlias) OutTrace("%s: lpstrAlias=%ls\n", ApiRef, lpOpenW->lpstrAlias);
				if(lpOpenW->lpstrElementName) OutTrace("%s: lpstrElementName=%ls\n", ApiRef, lpOpenW->lpstrElementName);
			}
		}
	}

	if((dxw.CDADrive >= 'D') && (dxw.CDADrive <= 'Z')) {
		BOOL remap = FALSE;
		if (uMsg == MCI_OPEN) { 
			if(isAnsi){
				LPMCI_OPEN_PARMSA parms = (LPMCI_OPEN_PARMSA)dwParam;
				if (dwFlags & MCI_OPEN_TYPE) {
					if (dwFlags & MCI_OPEN_TYPE_ID){
						if (LOWORD(parms->lpstrDeviceType) == MCI_DEVTYPE_CD_AUDIO) remap=TRUE;
					}
					else {
						if (!_strnicmp(parms->lpstrDeviceType, "cdaudio", strlen("cdaudio"))) remap=TRUE;
					}
				}
				// v2.05.70: registering device alias name for mciSendString calls
				if(remap && (dwFlags & MCI_OPEN_ALIAS)) { 
					strncpy(dxw.sAudioNickName, parms->lpstrAlias, 80);
					dxw.sAudioNickName[80]=0;
					OutTraceSND("%s: MCI_OPEN_ALIAS alias=%s\n", api, dxw.sAudioNickName);
				}
				if(remap){
					OutTraceSND("%s: remapping to drive %c:\n", api, dxw.CDADrive);
					static char sDrive[3];
					sprintf(sDrive, "%c:", dxw.CDADrive);
					dwFlags = (dwFlags & MCI_OPEN_ALIAS) | MCI_OPEN_TYPE_ID | MCI_OPEN_ELEMENT;
					parms->lpstrDeviceType = (LPCTSTR)MCI_DEVTYPE_CD_AUDIO;
					parms->lpstrElementName = sDrive;	
					if(IsTraceSYS || IsTraceSND) DumpMciMessage(ApiRef, isAnsi, FALSE, IDDevice, uMsg, dwFlags, dwParam);
				}
			}
			else {
				LPMCI_OPEN_PARMSW parms = (LPMCI_OPEN_PARMSW)dwParam;
				if (dwFlags & MCI_OPEN_TYPE) {
					if (dwFlags & MCI_OPEN_TYPE_ID){
						if (LOWORD(parms->lpstrDeviceType) == MCI_DEVTYPE_CD_AUDIO) remap=TRUE;
					}
					else {
						if (parms->lpstrDeviceType && !_wcsnicmp(parms->lpstrDeviceType, L"cdaudio", wcslen(L"cdaudio"))) remap=TRUE;
					}
				}
				// v2.05.70: registering device alias name for mciSendString calls
				if(remap && (dwFlags & MCI_OPEN_ALIAS)) { 
					wcstombs(dxw.sAudioNickName, parms->lpstrAlias, 80);
					dxw.sAudioNickName[80]=0;
					OutTraceSND("%s: MCI_OPEN_ALIAS alias=%s\n", api, dxw.sAudioNickName);
				}
				if(remap){
					OutTraceSND("%s: remapping to drive %c:\n", api, dxw.CDADrive);
					static WCHAR sDrive[3];
					swprintf(sDrive, L"%c:", dxw.CDADrive);
					dwFlags = (dwFlags & MCI_OPEN_ALIAS) | MCI_OPEN_TYPE_ID | MCI_OPEN_ELEMENT;
					parms->lpstrDeviceType = (LPCWSTR)MCI_DEVTYPE_CD_AUDIO;
					parms->lpstrElementName = sDrive;	
					if(IsTraceSYS || IsTraceSND) DumpMciMessage(ApiRef, isAnsi, FALSE, IDDevice, uMsg, dwFlags, dwParam);
				}
			}
		}
	}

#endif // DXW_NOTRACES
#if 0
	if((dxw.dwFlags11 & CUSTOMLOCALE) && (uMsg == MCI_OPEN) && (dwFlags & MCI_OPEN_ELEMENT)){
		LPMCI_OPEN_PARMSA lpOpenA = (LPMCI_OPEN_PARMS)dwParam;
		MCI_OPEN_PARMSW OpenW;
		int n, size;
		typedef int (WINAPI *MultiByteToWideChar_Type)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
		extern MultiByteToWideChar_Type pMultiByteToWideChar;
		OpenW.dwCallback = lpOpenA->dwCallback;
		OpenW.wDeviceID = lpOpenA->wDeviceID;
		LPWSTR lpDeviceTypeW = NULL;
		if(lpOpenA->lpstrDeviceType){
			size = strlen(lpOpenA->lpstrDeviceType);
			lpDeviceTypeW = (LPWSTR)malloc((size + 1) * 2);
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpOpenA->lpstrDeviceType, size, lpDeviceTypeW, size);
			lpDeviceTypeW[n]=0;
			OpenW.lpstrDeviceType = lpDeviceTypeW;
			OutTrace("%s: WIDE lpDeviceType=%ls\n", lpDeviceTypeW);
		}
		LPWSTR lpstrAliasW = NULL;
		if(lpOpenA->lpstrAlias){
			size = strlen(lpOpenA->lpstrAlias);
			LPWSTR lpstrAliasW = (LPWSTR)malloc((size + 1) * 2);
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpOpenA->lpstrAlias, size, lpstrAliasW, size);
			lpstrAliasW[n]=0;
			OpenW.lpstrAlias = lpstrAliasW;
			OutTrace("%s: WIDE lpstrAlias=%ls\n", lpstrAliasW);
		}
		LPWSTR lpstrElementNameW = NULL;
		if(lpOpenA->lpstrElementName) {
			size = strlen(lpOpenA->lpstrElementName);
			LPWSTR lpstrElementNameW = (LPWSTR)malloc((size + 1) * 2);
			n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpOpenA->lpstrElementName, size, lpstrElementNameW, size);
			lpstrElementNameW[n]=0;
			OpenW.lpstrElementName = lpstrElementNameW;
			OutTrace("%s: WIDE lpstrElementName=%ls\n", lpstrElementNameW);
		}
		if(!pmciSendCommandW){
			HMODULE hMci = LoadLibrary("winmm.dll");
			pmciSendCommandW = (mciSendCommand_Type)(*pGetProcAddress)(hMci, "mciSendCommandW");
		}
		ret = (*pmciSendCommandW)(IDDevice, uMsg, dwFlags, (DWORD_PTR)&OpenW);
		if(lpDeviceTypeW) free(lpDeviceTypeW);
		if(lpstrAliasW) free(lpstrAliasW);
		if(lpstrElementNameW) free(lpstrElementNameW);
	}
	else {
		ret = (*pmciSendCommand)(IDDevice, uMsg, dwFlags, dwParam);
	}
#else
	ret = (*pmciSendCommand)(IDDevice, uMsg, dwFlags, dwParam);
#endif

	// v2.05.71: can't understand why, but this happens and that fixes ...
	if((ret == MCIERR_CREATEWINDOW) && (dwFlags & MCI_WAVE_OPEN_BUFFER)) {
		OutTraceSND("%s: trimming MCI_WAVE_OPEN_BUFFER on MCIERR_CREATEWINDOW error\n", api);
		dwFlags &= ~MCI_WAVE_OPEN_BUFFER;
		ret = (*pmciSendCommand)(IDDevice, uMsg, dwFlags, dwParam);
	}

	// restore original (unscaled) RECT data
	// v2.05.60: fix - do it only when REMAPMCI flag is set
	if(dxw.IsFullScreen() && (uMsg==MCI_PUT) && (dxw.dwFlags5 & REMAPMCI)) pr->rc=saverect;

	if (ret) {
		OutTraceE("%s: ERROR res=%#x(%s)\n", api, ret, mmErrorString(ret));
		return ret;
	}

#ifndef DXW_NOTRACES
	if(IsTraceSND || IsTraceSYS) DumpMciMessage(ApiRef, isAnsi, TRUE, IDDevice, uMsg, dwFlags, dwParam);
#endif // DXW_NOTRACES
	return ret;
}

MCIERROR WINAPI extmciSendCommandA(MCIDEVICEID IDDevice, UINT uMsg, DWORD dwFlags, DWORD_PTR dwParam)
{ ApiName("mciSendCommandA"); return extmciSendCommand(ApiRef, TRUE, pmciSendCommandA, IDDevice, uMsg, dwFlags, dwParam); }

MCIERROR WINAPI extmciSendCommandW(MCIDEVICEID IDDevice, UINT uMsg, DWORD dwFlags, DWORD_PTR dwParam)
{ ApiName("mciSendCommandW"); return extmciSendCommand(ApiRef, FALSE, pmciSendCommandW, IDDevice, uMsg, dwFlags, dwParam); }

static int MapSecondsToTTMMSS(unsigned int seconds)
{
	// v2.05.55 fix: set to end of tracks when sec is greater or equal to disk length
	int ttmmss = mciEmu.dwLastTrack;
	//OutTrace("DEBUG: dwLastTrack=%#x ttmmss=%#x @%d\n", mciEmu.dwLastTrack, ttmmss, __LINE__);
    for (DWORD i = 1; i <= mciEmu.dwNumTracks; i++) {
		DWORD trackend = tracks[i].position + tracks[i].length;
		//OutTrace("DEBUG: i=%d seconds=%d trackend=%d\n", i, seconds, trackend);
		// v2.08.89: fix: the if statement requires the "<=" condition instead of "<".
		// In case of seconds == the end of a track, the "<" criteria wouldn't be satisfied for
		// the correct track and will examine the next one, but with a delta == -1, negative
		// value that will flood the whole DWORD giving a ttmmss result of (DWORD)-1!
		if(seconds <= trackend){
			int delta = seconds - tracks[i].position;
			//OutTrace("DEBUG: i=%d delta=%d delta/60=%d delta%%60=%d\n", i, delta, delta/60, delta % 60);
			ttmmss = TTMMSS_ENCODE(i, delta/60, delta % 60);
			//OutTrace("DEBUG: ttmmss=%#x @%d\n", ttmmss, __LINE__);
			break;
		}
    }
	return ttmmss;
}

int MapMsecToTTMMSS(int msec)
{
	return MapSecondsToTTMMSS(msec / 1000);
}

static DWORD EncodeTTMMSS(DWORD t)
{
	DWORD ttmmss = 0;
	DWORD sec;
	switch (mciEmu.dwTimeFormat) {
		case MCI_FORMAT_MILLISECONDS:
			ttmmss = MapMsecToTTMMSS(t);
			break;
		case MCI_FORMAT_TMSF:
			ttmmss = (MCI_TMSF_TRACK(t)<<16) | (MCI_TMSF_MINUTE(t)<<8) | (MCI_TMSF_SECOND(t));
			break;
		case MCI_FORMAT_MSF:
			sec = (MCI_MSF_MINUTE(t) * 60) + MCI_MSF_SECOND(t);
			ttmmss = MapSecondsToTTMMSS(sec);
			break;
		case MCI_FORMAT_HMS:
			sec = (MCI_HMS_HOUR(t) * 3600) + (MCI_HMS_MINUTE(t) * 60) + MCI_HMS_SECOND(t);
			ttmmss = MapSecondsToTTMMSS(sec);
			break;
	}
	OutDebugSND("> EncodeTTMMSS(%08.8X) -> TMS#%d:%d:%d\n", t, (ttmmss >> 16), ((ttmmss >> 8) & 0xFF), (ttmmss & 0xFF));
	return ttmmss;
}

static DWORD DecodeTTMMSS(DWORD ttmmss)
{
	DWORD res = 0;
	int track, min, sec, hours;
	track = TTMMSS_TRACKNO(ttmmss);
	min = TTMMSS_MINUTES(ttmmss);
	sec = TTMMSS_SECONDS(ttmmss); 
	switch (mciEmu.dwTimeFormat) {
		case MCI_FORMAT_MILLISECONDS:
			sec = tracks[track].position + (60 * min) + sec;
			res = 1000 * sec;
			break;
		case MCI_FORMAT_TMSF:
			res = MCI_MAKE_TMSF(track, min, sec, 0);
			break;
		case MCI_FORMAT_MSF:
			sec = tracks[track].position + (60 * min) + sec;
			min = sec / 60;
			sec = sec % 60;
			res = MCI_MAKE_MSF(min, sec, 0);
			break;
		case MCI_FORMAT_HMS:
			sec = tracks[track].position + (60 * min) + sec;
			hours = sec / 3600;
			sec = sec % 3600;
			min = sec / 60;
			sec = sec % 60;
			res = MCI_MAKE_HMS(hours, min, sec);
			break;
	}
	OutDebugSND("> DecodeTTMMSS(TMS#%d:%d:%d) -> res=%08.8X\n", (ttmmss >> 16), ((ttmmss >> 8) & 0xFF), (ttmmss & 0xFF), res);
	return res;
}

// v2.05.78: added IGNOREMCIDEVID handling in mciSendString wrapper
static BOOL bCheckDevice(char *sDevice)
{
	return (!strcmp(sDevice, "cdaudio") || 
		!strcmp(sDevice, dxw.sAudioNickName) ||
		(dxw.dwFlags8 & IGNOREMCIDEVID)
		);
}

DWORD HandlePlayCommand(DWORD from, DWORD to)
{
	DWORD ret = 0;
	int syntax = 0;
	dprintf("< from=%#x to=%#x status=%s\n", from, to, player_get_status());
	start_player();
	if(from != UNKNOWNTRACK) syntax += 1;
	if(to != UNKNOWNTRACK) syntax += 2;
	switch(syntax){
		case 0: // no args
			if(mciEmu.playing){
				dprintf(">> from,to=NIL playing: ignore\n");
				return 0;
				break;
			}
			else if(mciEmu.paused){
				dprintf(">> from,to=NIL paused: resume\n");
				player_set_status(MCI_RESUME);
			}
			else {
				// else, stopped ...
				dprintf(">> from,to=NIL stopped: start from current to end\n");
				from = mciEmu.dwCurrentTrack;
				if(from < mciEmu.dwFirstTrack) from = mciEmu.dwFirstTrack;
				info.first = from;
				info.last = mciEmu.dwLastTrack;
				mciEmu.dwTargetTrack = mciEmu.dwLastTrack;
				player_set_status(MCI_PLAY);
			}
			break;
		case 1: // "from"
			dprintf(">> from=%#x\n", from);
			// beware: the "from" only syntax has a simpler behavior then the "to" only: it always resets
			// the target to the end of the disk, clearing any previous disposition.
			// v2.05.87: allow start from data track
			if((from > mciEmu.dwLastTrack) || (from < TTMMSS_ENCODE(1, 0, 0))){
				dprintf(">> out of range: error MCIERR_OUTOFRANGE @%d\n", __LINE__);
				player_set_status(MCI_STOP);
				ret = MCIERR_OUTOFRANGE; // The specified parameter value is out of range for the specified MCI command.
				break;
			}
			// v2.05.87: skip data tracks if any ...
			if(from < mciEmu.dwFirstTrack) from = mciEmu.dwFirstTrack;
			to = mciEmu.dwLastTrack;
			info.first = from;
			info.last = to;
			mciEmu.dwCurrentTrack = from; 
			mciEmu.dwTargetTrack = to;
			player_set_status(MCI_PLAY);
			break;
		case 2: // "to"
			dprintf(">> to=%#x\n", to);
			if((to > mciEmu.dwLastTrack) || (to < mciEmu.dwFirstTrack)){
				dprintf(">> out of range: error MCIERR_OUTOFRANGE @%d\n", __LINE__);
				player_set_status(MCI_STOP);
				ret = MCIERR_OUTOFRANGE; // The specified parameter value is out of range for the specified MCI command.
				break;
			}
			if(mciEmu.playing || mciEmu.paused){
				from = mciEmu.dwCurrentTrack;
				if(to < from) {
					dprintf(">> out of range: error MCIERR_OUTOFRANGE @%d\n", __LINE__);
					player_set_status(MCI_STOP);
					ret = MCIERR_OUTOFRANGE; // The specified parameter value is out of range for the specified MCI command.
					break;
				}
				info.first = from;
				info.last = to;
				mciEmu.dwTargetTrack = to;
				if(mciEmu.playing) player_set_status(MCI_PLAY);
				if(mciEmu.paused) player_set_status(MCI_RESUME);
			}
			else { // stopped
				//info.first = mciEmu.dwFirstTrack;
				from = mciEmu.dwSeekedTrack;
				if(to < from) {
					dprintf(">> out of range: error MCIERR_OUTOFRANGE @%d\n", __LINE__);
					player_set_status(MCI_STOP);
					ret = MCIERR_OUTOFRANGE; // The specified parameter value is out of range for the specified MCI command.
					break;
				}
				from = mciEmu.dwCurrentTrack; // Added to fix resume with MCI_PLAY + MCI_TO
				// v2.05.94 fix suggested by Dippy Dipper:
				// be sure to set the start position to a valid audio track position
				if(from < mciEmu.dwFirstTrack) from = mciEmu.dwFirstTrack; 
				info.first = from;
				info.last = to;
				mciEmu.dwCurrentTrack = from;
				mciEmu.dwTargetTrack = to;
				player_set_status(MCI_PLAY);
			}	
			break;
		case 3: // both "from" and "to"
			dprintf(">> from=%#x to=%#x\n", from, to);
			// v2.05.87: allow start from data track
			if( (from < TTMMSS_ENCODE(1, 0, 0)) ||
				(to > mciEmu.dwLastTrack) ||
				(from > to)){
				dprintf(">> out of range: error MCIERR_OUTOFRANGE @%d\n", __LINE__);
				player_set_status(MCI_STOP);
				ret = MCIERR_OUTOFRANGE; // The specified parameter value is out of range for the specified MCI command.
				break;
			}
			// v2.05.87: skip data tracks if any ...
			if(from < mciEmu.dwFirstTrack) from = mciEmu.dwFirstTrack;
			info.first = from;
			info.last = to;
			mciEmu.dwCurrentTrack = from; 
			mciEmu.dwTargetTrack = to;
			player_set_status(MCI_PLAY);
			break;
	}
	dprintf("< first=%#x last=%#x ret=%d status=%s\n", info.first, info.last, ret, player_get_status());
	return ret;
}

MCIERROR WINAPI emumciSendCommand(BOOL isAnsi, MCIDEVICEID IDDevice, UINT uMsg, DWORD dwFlags, DWORD_PTR dwParam)
{
	if(!pplr_pump) player_init();
	player_change();
	if(mciEmu.dwTimeFormat == MCI_FORMAT_UNDEFINED) mciEmu.dwTimeFormat = MCI_FORMAT_TMSF;

	switch(uMsg){
		case MCI_OPEN:
			if (isAnsi){
				LPMCI_OPEN_PARMSA parms = (LPMCI_OPEN_PARMSA)dwParam;

				if (dwFlags & MCI_OPEN_TYPE_ID){
					if (LOWORD(parms->lpstrDeviceType) == MCI_DEVTYPE_CD_AUDIO){
						dprintf("> Returning device id=%d for MCI_DEVTYPE_CD_AUDIO\n", dxw.VirtualCDAudioDeviceId);
						parms->wDeviceID = dxw.VirtualCDAudioDeviceId;
						player_set_status(MCI_OPEN);
						return 0;
					}
				}

				if (dwFlags & MCI_OPEN_TYPE && !(dwFlags & MCI_OPEN_TYPE_ID)){
					dprintf("> MCI_OPEN_TYPE -> %s\n", parms->lpstrDeviceType);
					if (_stricmp(parms->lpstrDeviceType, "cdaudio") == 0){
						dprintf("> Returning magic device id=%d for MCI_OPEN_TYPE_ID cdaudio\n", dxw.VirtualCDAudioDeviceId);
						parms->wDeviceID = dxw.VirtualCDAudioDeviceId;
						player_set_status(MCI_OPEN);
						return 0;
					}
				}

				if (dwFlags & MCI_OPEN_ELEMENT) {
					char *p;
					int iTrackNo;
					dprintf("> MCI_OPEN_ELEMENT -> %s\n", parms->lpstrElementName);
					p = (char *)strstr(parms->lpstrElementName, "\\track");
					if(sscanf(p, "\\track%d.cda", &iTrackNo) == 1) {
						iTrackNo ++;
						dprintf("> Track number = %d\n", iTrackNo);
					}

					info.first = (MCI_TMSF_TRACK(iTrackNo)<<16) | (MCI_TMSF_MINUTE(0)<<8) | (MCI_TMSF_SECOND(0));
					info.last = (MCI_TMSF_TRACK(iTrackNo+1)<<16) | (MCI_TMSF_MINUTE(0)<<8) | (MCI_TMSF_SECOND(0));

					dprintf("> mapped tracks First=%02d.%02d.%02d Last=%02d.%02d.%02d\n", 
						info.first>>16, (info.first>>8)&0xFF, info.first&0xFF,
						info.last>>16,  (info.last>>8)&0xFF,  info.last&0xFF);

					mciEmu.dwSeekedTrack = iTrackNo;
					mciEmu.dwCurrentTrack = iTrackNo;
					start_player();
					player_set_status(MCI_PLAY);

					parms->wDeviceID = dxw.VirtualCDAudioDeviceId;
				}	
			}
			else {
				LPMCI_OPEN_PARMSW parms = (LPMCI_OPEN_PARMSW)dwParam;

				if (dwFlags & MCI_OPEN_TYPE_ID){
					if (LOWORD(parms->lpstrDeviceType) == MCI_DEVTYPE_CD_AUDIO){
						dprintf("> Returning magic device id=%d for MCI_DEVTYPE_CD_AUDIO\n", dxw.VirtualCDAudioDeviceId);
						parms->wDeviceID = dxw.VirtualCDAudioDeviceId;
						player_set_status(MCI_OPEN);
						return 0;
					}
				}

				if (dwFlags & MCI_OPEN_TYPE && !(dwFlags & MCI_OPEN_TYPE_ID)){
					dprintf("> MCI_OPEN_TYPE -> %ls\n", parms->lpstrDeviceType);
					if (parms->lpstrDeviceType && (_wcsicmp (parms->lpstrDeviceType, L"cdaudio") == 0)){
						dprintf("> Returning device id=%d for MCI_OPEN_TYPE_ID cdaudio\n", dxw.VirtualCDAudioDeviceId);
						parms->wDeviceID = dxw.VirtualCDAudioDeviceId;
						player_set_status(MCI_OPEN);
						return 0;
					}
				}

				if (dwFlags & MCI_OPEN_ELEMENT) {
					WCHAR *p;
					int iTrackNo;
					dprintf("> MCI_OPEN_ELEMENT -> %ls\n", parms->lpstrElementName);
					p = (WCHAR *)wcsstr(parms->lpstrElementName, L"\\track");
					if(swscanf(p, L"\\track%d.cda", &iTrackNo) == 1) {
						iTrackNo ++;
						dprintf("> Track number = %d\n", iTrackNo);
					}

					info.first = (MCI_TMSF_TRACK(iTrackNo)<<16) | (MCI_TMSF_MINUTE(0)<<8) | (MCI_TMSF_SECOND(0));
					info.last = (MCI_TMSF_TRACK(iTrackNo+1)<<16) | (MCI_TMSF_MINUTE(0)<<8) | (MCI_TMSF_SECOND(0));

					dprintf("> mapped tracks First=%02d.%02d.%02d Last=%02d.%02d.%02d\n", 
						info.first>>16, (info.first>>8)&0xFF, info.first&0xFF,
						info.last>>16,  (info.last>>8)&0xFF,  info.last&0xFF);

					mciEmu.dwSeekedTrack = iTrackNo;
					mciEmu.dwCurrentTrack = iTrackNo;
					start_player();
					player_set_status(MCI_PLAY);

					parms->wDeviceID = dxw.VirtualCDAudioDeviceId;
			}	
		}
		break;

		case MCI_GETDEVCAPS: { // v2.04.95: "Absolute Terror"
			LPMCI_GETDEVCAPS_PARMS parms = (LPMCI_GETDEVCAPS_PARMS)dwParam;
			dprintf("> MCI_GETDEVCAPS item=%#x(%s)\n", parms->dwItem, ExplainMCICapability(parms->dwItem));
			parms->dwReturn = 0; // has no capabilities
			switch(parms->dwItem){
				case MCI_GETDEVCAPS_CAN_PLAY:
				case MCI_GETDEVCAPS_CAN_EJECT:
				case MCI_GETDEVCAPS_HAS_AUDIO: // v2.05.72 - Pitfall
					parms->dwReturn = TRUE;
					dprintf("> MCI_GETDEVCAPS val=TRUE\n");
					break;
				case MCI_GETDEVCAPS_DEVICE_TYPE:
					parms->dwReturn = MCI_DEVTYPE_CD_AUDIO;
					dprintf("> MCI_GETDEVCAPS val=MCI_DEVTYPE_CD_AUDIO\n");
					break;
				default:
					parms->dwReturn = FALSE;
					dprintf("> MCI_GETDEVCAPS val=FALSE\n");
					break;
			}
			dprintf("> params->dwReturn=%d(%#x)\n", parms->dwReturn, parms->dwReturn);
			return 0;
		}
		break;

		case MCI_SET: {
			LPMCI_SET_PARMS parms = (LPMCI_SET_PARMS)dwParam;
			OutDebugSND("> MCI_SET_PARMS={dwCallback=%#x dwTimeFormat=%#x dwAudio=%#x}\n",
				parms->dwCallback,
				parms->dwTimeFormat,
				parms->dwAudio);
			if (dwFlags & MCI_SET_TIME_FORMAT){
				mciEmu.dwTimeFormat = parms->dwTimeFormat;
				dprintf("> MCI_SET_TIME_FORMAT format=%s\n", sTimeFormat(mciEmu.dwTimeFormat));
			}
			else
			if (dwFlags & MCI_SET_DOOR_OPEN){
				player_set_status(MCIEMU_DOOR_OPEN);
				dprintf("> MCI_SET_DOOR_OPEN\n");
			}
			else
			if (dwFlags & MCI_SET_DOOR_CLOSED){
				player_set_status(MCIEMU_DOOR_CLOSED);
				dprintf("> MCI_SET_DOOR_CLOSED\n");
			}
			else
			// v2.05.85: tentative handling of MCI_SET_AUDIO + MCI_SET_ON/OFF flags
			if (dwFlags & MCI_SET_AUDIO){
				dprintf("> MCI_SET_AUDIO dwAudio=%#x\n", parms->dwAudio);
				// DxWnd can trace what commands was issued, but can't handle a single left or right
				// channel, so in any case it will act on both.
				if(dwFlags & MCI_SET_AUDIO_ALL | MCI_SET_AUDIO_LEFT | MCI_SET_AUDIO_RIGHT){
					if(dwFlags & MCI_SET_AUDIO_ALL) dprintf(">> MCI_SET_AUDIO_ALL\n");
					if(dwFlags & MCI_SET_AUDIO_LEFT) dprintf(">> MCI_SET_AUDIO_LEFT\n");
					if(dwFlags & MCI_SET_AUDIO_RIGHT) dprintf(">> MCI_SET_AUDIO_RIGHT\n");
				}
				if(dwFlags & MCI_SET_ON){
					dprintf(">> MCI_SET_ON\n");
					(*pplr_setvolume)(cdVolume);
				}
				if(dwFlags & MCI_SET_OFF){
					dprintf(">> MCI_SET_OFF\n");
					cdVolume = (*pplr_getvolume)();
					(*pplr_setvolume)(0);
				}
			}
			// v2.05.62 fix: added MCI_NOTIFY handling. Needed for "Little Witch Parfait".
			if(dwFlags & MCI_NOTIFY) {
				LRESULT lres;
				lres = (*pSendMessageA)((HWND)(parms->dwCallback), MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, mciEmu.dwDevID);
				dprintf("> MCI_SET: Sent MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message: hwnd=%#x, res=%#x\n", parms->dwCallback, lres);
			}
		}
		break;

		case MCI_WHERE: {
			LPMCI_ANIM_RECT_PARMS parms = (LPMCI_ANIM_RECT_PARMS)dwParam;
			if (dwFlags & MCI_ANIM_WHERE_SOURCE){
				dprintf("> MCI_ANIM_WHERE_SOURCE rect=(%d,%d)-(%d,%d)\n",
					parms->rc.left, parms->rc.top, parms->rc.right, parms->rc.bottom);
				parms->rc.left = 0;
				parms->rc.right = 0;
				parms->rc.top = 0;
				parms->rc.bottom = 0;
				dprintf("< MCI_ANIM_WHERE_SOURCE rect=(%d,%d)-(%d,%d) ret=MCIERR_UNSUPPORTED_FUNCTION\n",
					parms->rc.left, parms->rc.top, parms->rc.right, parms->rc.bottom);
				return MCIERR_UNSUPPORTED_FUNCTION;
			}
			if (dwFlags & MCI_ANIM_WHERE_DESTINATION){
				dprintf("> MCI_ANIM_WHERE_DESTINATION rect=(%d,%d)-(%d,%d)\n",
					parms->rc.left, parms->rc.top, parms->rc.right, parms->rc.bottom);
				parms->rc.left = 0;
				parms->rc.right = 0;
				parms->rc.top = 0;
				parms->rc.bottom = 0;
				dprintf("< MCI_ANIM_WHERE_DESTINATION rect=(%d,%d)-(%d,%d) ret=MCIERR_UNSUPPORTED_FUNCTION\n",
					parms->rc.left, parms->rc.top, parms->rc.right, parms->rc.bottom);
				return MCIERR_UNSUPPORTED_FUNCTION;
			}
		}
		break;

		case MCI_CLOSE: {
			if (hPlayer) player_set_status(MCI_CLOSE);
			// v2.05.00: MCI_CLOSE resets time formats
			mciEmu.dwTimeFormat = MCI_FORMAT_UNDEFINED;
			// v2.05.78: MCI_CLOSE resets also callbacks
			info.play_callback = 0;
			info.stop_callback = 0;
			return 0;
		}
		break;

		case MCI_SEEK: {
			LPMCI_SEEK_PARMS parms = (LPMCI_SEEK_PARMS)dwParam;
			if (dwFlags & MCI_TO) {
				dprintf("> dwTo: %d\n", parms->dwTo);
				// v2.05.86: fix - a MCI_SEEK to 0 is treated as MCI_SEEK_TO_START. Seen in "Time Warriors".
				if(parms->dwTo == 0){
					mciEmu.dwSeekedTrack = mciEmu.dwFirstTrack;
				}
				else {
					mciEmu.dwSeekedTrack = EncodeTTMMSS(parms->dwTo);
				}
			}
			if (dwFlags & MCI_SEEK_TO_START) {
				dprintf("> Seek to Start\n");
				mciEmu.dwSeekedTrack = mciEmu.dwFirstTrack;
			}
			if (dwFlags & MCI_SEEK_TO_END) {
				dprintf("> Seek to End\n");
				mciEmu.dwSeekedTrack = mciEmu.dwLastTrack;
			}
			// v2.05.86: seeked position becomes the current position. The player stops.
			mciEmu.dwCurrentTrack = mciEmu.dwSeekedTrack;
			player_set_status(MCI_STOP);
			// v2.05.60 fix: added MCI_NOTIFY handling. Needed for "Beast Wars Transformers".
			if(dwFlags & MCI_NOTIFY) {
				LRESULT lres;
				lres = (*pSendMessageA)((HWND)parms->dwCallback, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, mciEmu.dwDevID);
				dprintf("> MCI_SEEK: Sent MM_MCINOTIFY MCI_NOTIFY_SUCCESSFUL message: hwnd=%#x, res=%#x\n", parms->dwCallback, lres);
				Sleep(100); // needs some delay to avoid interpreting the notify as if sent from the following message
			}
		}
		break;

		case MCI_PLAY: { 
			// v2.05.77: fixed handling of MCI_PLAY to resume and MCI_NOTIFY flag
			LPMCI_PLAY_PARMS parms = (LPMCI_PLAY_PARMS)dwParam;
			OutDebugSND("> LPMCI_PLAY_PARMS={dwCallback=%#x dxFrom=%#x dxTo=%#x}\n", 
				parms->dwCallback,
				parms->dwFrom,
				parms->dwTo);
			dprintf("> timeformat=%s\n", sTimeFormat(mciEmu.dwTimeFormat));

			if ((mciEmu.paused || mciEmu.playing) && info.play_callback){
				// v2.05.77: abort the previous notification request
				LRESULT lres;
				HWND hwnd = (HWND)info.play_callback;
				info.play_callback = 0;
				// according to https://learn.microsoft.com/en-us/windows/win32/multimedia/mm-mcinotify it is not
				// too clear what should be sent in this case, but it seems that MCI_NOTIFY_ABORTED is what happens
				// in real cases.
				//lres = (*pSendMessageA)(hwnd, MM_MCINOTIFY, MCI_NOTIFY_SUPERSEDED, mciEmu.dwDevID);
				//dprintf("> MCI_PLAY: Sent MM_MCINOTIFY MCI_NOTIFY_SUPERSEDED message: hwnd=%#x, res=%#x\n", hwnd, lres);
				lres = (*pSendMessageA)(hwnd, MM_MCINOTIFY, MCI_NOTIFY_ABORTED, mciEmu.dwDevID);
				dprintf("> MCI_PLAY: Sent MM_MCINOTIFY MCI_NOTIFY_ABORTED message: hwnd=%#x, res=%#x\n", hwnd, lres);
			}

			if(dwFlags & MCI_NOTIFY){
				dprintf("> dwCallback: %#x\n", parms->dwCallback);
				info.play_callback = parms->dwCallback;
				if(dxw.dwFlags16 & MCINOTIFYTOPMOST) {
					info.play_callback = (DWORD_PTR)0xFFFF;
					dprintf("> Setting TOPMOST play_callback hWnd=%#x\n", info.play_callback);
				}
				if(!info.play_callback) {
					info.play_callback = (DWORD_PTR)dxw.GethWnd(); // v2.05.89 Messiah fix ???
					dprintf("> Replacing NULL play_callback hWnd=%#x\n", info.play_callback);
				}
			}
			else 
				info.play_callback = 0;

			// v2.05.86: fully revised handling of void MCI_PLAY command
			DWORD from = UNKNOWNTRACK;
			DWORD to = UNKNOWNTRACK;
			if(dwFlags & MCI_FROM) {
				if(mciEmu.dwTimeFormat == MCI_FORMAT_MILLISECONDS) {
					if(checkGap("mciSendCommand", parms->dwFrom)) return MCIERR_OUTOFRANGE;
				}
				from = EncodeTTMMSS(parms->dwFrom);
			}
			if(dwFlags & MCI_TO) {
				to = EncodeTTMMSS(parms->dwTo);
			}
			return HandlePlayCommand(from, to);
		}
		break;

		case MCI_STOP: {
			LPMCI_GENERIC_PARMS parms = (LPMCI_GENERIC_PARMS)dwParam;
			if(dwFlags & MCI_NOTIFY) {
				info.stop_callback = parms->dwCallback;				
				if(dxw.dwFlags16 & MCINOTIFYTOPMOST) {
					info.stop_callback = (DWORD_PTR)0xFFFF;
					dprintf("> Setting TOPMOST stop_callback hWnd=%#x\n", info.play_callback);
				}
				if(!info.stop_callback) {
					info.stop_callback = (DWORD_PTR)dxw.GethWnd(); // v2.05.89 Messiah fix ???
					dprintf("> Replacing NULL stop_callback hWnd=%#x\n", info.stop_callback);
				}
			}
			else info.stop_callback = 0;
			HandleStopCommand();
		}
		break;

		// v2.04.57: added MCI_PAUSE case
		case MCI_PAUSE: {
			LPMCI_GENERIC_PARMS parms = (LPMCI_GENERIC_PARMS)dwParam;
			if(dwFlags & MCI_NOTIFY){
				dprintf("> dwCallback: %#x\n", parms->dwCallback);
				info.play_callback = parms->dwCallback;
				if(dxw.dwFlags16 & MCINOTIFYTOPMOST) {
					info.play_callback = (DWORD_PTR)0xFFFF;
					dprintf("> Setting TOPMOST play_callback hWnd=%#x\n", info.play_callback);
				}
				if(!info.play_callback) {
					info.play_callback = (DWORD_PTR)dxw.GethWnd(); // v2.05.89 Messiah fix ???
					dprintf("> Replacing NULL play_callback hWnd=%#x\n", info.play_callback);
				}
			}
			else {
				info.play_callback = 0;
			}
			// v2.05.94: Fixed pause cdaudio logic
            // v2.06.04: fix player_set_status moved outside the else block
			if(dxw.dwFlags11 & CDPAUSECAPABILITY) player_set_status(MCI_PAUSE);
			else player_set_status(MCI_STOP);
		}
		break;

		// v2.04.57: added MCI_RESUME case
		case MCI_RESUME: 
			player_set_status(uMsg);
		break;

		case MCI_INFO: {
			char hackPath[MAX_PATH];
			sprintf(hackPath, "%s\\Music\\mcihack.txt", dxw.cdAudioPath);
			if(isAnsi){
				// v2.04.97: use bitwise AND to compare instead of numeric identity
				// because both MCI_INFO_MEDIA_IDENTITY and MCI_INFO_PRODUCT could be
				// associated to other flags, es. MCI_WAIT - fixes "Sentinel Returns"
				dprintf("> MCI_INFO command = %#x\n", dwFlags);
				LPMCI_INFO_PARMS lpInfo = (LPMCI_INFO_PARMS)dwParam;
				if(dwFlags & MCI_INFO_MEDIA_IDENTITY) {
					// v2.05.32: return default "fakecd" string. Fixes "Sentinel Returns"
					if(!ansiIdentity){
						ansiIdentity = (char *)malloc(80+1); 
						(*pGetPrivateProfileStringA)("info", "identity", "fakecd", ansiIdentity, 80, hackPath);
					}
					lpInfo->lpstrReturn = ansiIdentity;
					lpInfo->dwRetSize = strlen(lpInfo->lpstrReturn);
					if(lpInfo->dwRetSize == 0){
						OutTraceSND("> ret=350(MCIERR_NO_IDENTITY)\n");
						return MCIERR_NO_IDENTITY;
					}
				}
				if(dwFlags & MCI_INFO_PRODUCT) { 
					// v2.05.32: return default "fakecd" string. 
					if(!ansiProduct){
						ansiProduct = (char *)malloc(80+1); 
						(*pGetPrivateProfileStringA)("info", "product", "fakecd", ansiProduct, 80, hackPath);
					}
					lpInfo->lpstrReturn = ansiProduct;
					lpInfo->dwRetSize = strlen(lpInfo->lpstrReturn);
					if(lpInfo->dwRetSize == 0){
						OutTraceSND("> ret=350(MCIERR_NO_IDENTITY)\n");
						return MCIERR_NO_IDENTITY;
					}
				}
				dprintf("> MCI_INFO strret=%s retsize=%d\n", lpInfo->lpstrReturn, lpInfo->dwRetSize);
			}
			else { // v2.05.54: widechar case
				dprintf("> MCI_INFO command = %#x\n", dwFlags);
				LPMCI_INFO_PARMSW lpInfo = (LPMCI_INFO_PARMSW)dwParam;
				if(dwFlags & MCI_INFO_MEDIA_IDENTITY) {
					// v2.05.32: return default "fakecd" string. Fixes "Sentinel Returns"
					if(!ansiIdentity){
						ansiIdentity = (char *)malloc(80+1); 
						(*pGetPrivateProfileStringA)("info", "identity", "fakecd", ansiIdentity, 80, hackPath);
					}
					if(!wideIdentity){
						wideIdentity = (WCHAR *)malloc((strlen(ansiIdentity)+1) * sizeof(WCHAR)); 
						mbstowcs(wideIdentity, ansiIdentity, strlen(ansiIdentity));
					}
					lpInfo->lpstrReturn = wideIdentity;
					lpInfo->dwRetSize = wcslen(wideIdentity);
					if(lpInfo->dwRetSize == 0){
						OutTraceSND("> ret=350(MCIERR_NO_IDENTITY)\n");
						return MCIERR_NO_IDENTITY;
					}
				}
				if(dwFlags & MCI_INFO_PRODUCT) { 
					// v2.05.32: return default "fakecd" string. 
					if(!ansiProduct){
						ansiProduct = (char *)malloc(80+1); 
						(*pGetPrivateProfileStringA)("info", "product", "fakecd", ansiProduct, 80, hackPath);
					}
					if(!wideIdentity){
						wideIdentity = (WCHAR *)malloc((strlen(ansiProduct)+1) * sizeof(WCHAR)); 
						mbstowcs(wideProduct, ansiProduct, strlen(ansiProduct));
					}
					lpInfo->lpstrReturn = wideProduct;
					lpInfo->dwRetSize = wcslen(wideProduct);
					if(lpInfo->dwRetSize == 0){
						OutTraceSND("> ret=350(MCIERR_NO_IDENTITY)\n");
						return MCIERR_NO_IDENTITY;
					}
				}
				dprintf("> MCI_INFO strret=%s retsize=%d\n", lpInfo->lpstrReturn, lpInfo->dwRetSize);
			}
		}
		break;

		case MCI_SYSINFO: {
			MCIERROR ret;
			if(isAnsi) {
				// tbd: found in "Heavy Gear":
				// also found in "Interstate 76 Nitro Pack"
				// MCI_SYSINFO_QUANTITY devtype=CDAUDIO, better return 1

				LPMCI_SYSINFO_PARMSA lpInfo = (LPMCI_SYSINFO_PARMSA)dwParam;

				dprintf("> MCI_SYSINFO command = %#x\n", dwFlags);

				ret = (*pmciSendCommandA)(IDDevice, uMsg, dwFlags, dwParam);
				OutTraceSND("> actual ret=%#x\n", ret);

				if( (dwFlags & MCI_SYSINFO_QUANTITY) && 
					(lpInfo->wDeviceType == MCI_DEVTYPE_CD_AUDIO)){
					dprintf("> MCI_SYSINFO MCI_SYSINFO_QUANTITY MCI_dev=DEVTYPE_CD_AUDIO force return 1\n");
					DWORD num = 1;
					// MSDN: Pointer to a user-supplied buffer for the return string. 
					// It is also used to return a DWORD value when the MCI_SYSINFO_QUANTITY flag is used.
					memcpy((LPVOID)(lpInfo->lpstrReturn), (LPVOID)&num, sizeof(DWORD));
					// MSDN: Size, in bytes, of return buffer.
					lpInfo->dwRetSize = sizeof(DWORD);
					// MSDN: Number indicating the device position in the MCI device table or in the list 
					// of open devices if the MCI_SYSINFO_OPEN flag is set.
					lpInfo->dwNumber = dxw.VirtualCDAudioDeviceId; // v2.05.36
					ret = 0; // v2.05.36: pretend it's OK
				}

				if( (dwFlags & (MCI_SYSINFO_NAME|MCI_SYSINFO_INSTALLNAME)) && 
					(lpInfo->wDeviceType == MCI_DEVTYPE_CD_AUDIO)){
					// v2.05.76 return the CDAudio name
					dprintf("> MCI_SYSINFO MCI_SYSINFO_NAME MCI_dev=DEVTYPE_CD_AUDIO force return \"CDAudio\"\n");
					sprintf(lpInfo->lpstrReturn, "%.*s", lpInfo->dwRetSize, "CDAudio");

					dprintf("> MCI_SYSINFO callback=%#x ret=\"%s\" retsize=%d num=%d devtype=%#x\n", 
						lpInfo->dwCallback,
						lpInfo->lpstrReturn, 
						lpInfo->dwRetSize,
						lpInfo->dwNumber,
						lpInfo->wDeviceType);
					ret = 0; // v2.05.76: pretend it's OK
				}
				else {
					dprintf("> MCI_SYSINFO callback=%#x ret=%d retsize=%d num=%d devtype=%#x\n", 
						lpInfo->dwCallback,
						*(DWORD *)lpInfo->lpstrReturn, 
						lpInfo->dwRetSize,
						lpInfo->dwNumber,
						lpInfo->wDeviceType);
					// v2.06.06. fix - in case of error, don't clear the return code! - required for @#@ "Eryner"
					return ret;
				}
			}
			else { // 2.05.54: widechar case
				LPMCI_SYSINFO_PARMSW lpInfo = (LPMCI_SYSINFO_PARMSW)dwParam;

				dprintf("> MCI_SYSINFO command = %#x\n", dwFlags);

				ret = (*pmciSendCommandW)(IDDevice, uMsg, dwFlags, dwParam);
				OutTraceSND("> actual ret=%#x\n", ret);

				if( (dwFlags & MCI_SYSINFO_QUANTITY) && 
					(lpInfo->wDeviceType == MCI_DEVTYPE_CD_AUDIO)){
					dprintf("> MCI_SYSINFO MCI_SYSINFO_QUANTITY MCI_dev=DEVTYPE_CD_AUDIO force return 1\n");
					DWORD num = 1;
					// MSDN: Pointer to a user-supplied buffer for the return string. 
					// It is also used to return a DWORD value when the MCI_SYSINFO_QUANTITY flag is used.
					memcpy((LPVOID)(lpInfo->lpstrReturn), (LPVOID)&num, sizeof(DWORD));
					// MSDN: Size, in bytes, of return buffer.
					lpInfo->dwRetSize = sizeof(DWORD);
					// MSDN: Number indicating the device position in the MCI device table or in the list 
					// of open devices if the MCI_SYSINFO_OPEN flag is set.
					lpInfo->dwNumber = dxw.VirtualCDAudioDeviceId; // v2.05.36
					ret = 0; // v2.05.36: pretend it's OK
				}

				if( (dwFlags & (MCI_SYSINFO_NAME|MCI_SYSINFO_INSTALLNAME)) && 
					(lpInfo->wDeviceType == MCI_DEVTYPE_CD_AUDIO)){
					// v2.05.76 return the CDAudio name
					dprintf("> MCI_SYSINFO MCI_SYSINFO_NAME MCI_dev=DEVTYPE_CD_AUDIO force return \"CDAudio\"\n");
					swprintf(lpInfo->lpstrReturn, L"%.*ls", lpInfo->dwRetSize, L"CDAudio");

					dprintf("> MCI_SYSINFO callback=%#x ret=\"%ls\" retsize=%d num=%d devtype=%#x\n", 
						lpInfo->dwCallback,
						lpInfo->lpstrReturn, 
						lpInfo->dwRetSize,
						lpInfo->dwNumber,
						lpInfo->wDeviceType);
					ret = 0;
				}
				else {
					dprintf("> MCI_SYSINFO callback=%#x ret=%d retsize=%d num=%d devtype=%#x\n", 
						lpInfo->dwCallback,
						*(DWORD *)lpInfo->lpstrReturn, 
						lpInfo->dwRetSize,
						lpInfo->dwNumber,
						lpInfo->wDeviceType);
				}
			}
			_if(ret) {
				OutTraceE("%> ERROR ret=%#x(%s)\n", ret, mmErrorString(ret));
				// v2.06.06. fix - in case of error, don't don't clear the return code!
				return ret;
			}
		}
		break;

		case MCI_STATUS: {
			// v2.05.76 fix:
			// because of the separate player thread, it may take some time before the variables are set
			//"Mechwarrior 2" polls the CD status immediately and furiously, with the result of blocking
			// the audio play. A small delay (sleep 100 mSec) simulated the CD hardware delay and fixes
			// the problem.
			if(dxw.dwFlags14 & SLOWCDSTATUS) Sleep(100);

			dprintf("> MCI_STATUS\n");
			LPMCI_STATUS_PARMS parms = (LPMCI_STATUS_PARMS)dwParam;

			parms->dwReturn = 0;

			if (dwFlags & MCI_TRACK){
				dprintf("> MCI_TRACK dwTrack = %d\n", parms->dwTrack);
			}

			if (dwFlags & MCI_STATUS_START){
				dprintf("> MCI_STATUS_START\n");
			}

			if (dwFlags & MCI_STATUS_ITEM) {
				dprintf("> MCI_STATUS_ITEM\n");

				if (parms->dwItem == MCI_STATUS_CURRENT_TRACK) {
					// v2.05.59 fix: from https://docs.microsoft.com/en-us/previous-versions/ms711491(v=vs.85)
					// MCI_STATUS_CURRENT_TRACK 
					// The dwReturn member is set to the current track number. 
					// MCI uses continuous track numbers.
					DWORD currentTrack = mciEmu.dwCurrentTrack;
					if(currentTrack < mciEmu.dwFirstTrack) currentTrack = mciEmu.dwFirstTrack;
					dprintf("> MCI_STATUS_CURRENT_TRACK track=%d\n", TTMMSS_TRACKNO(currentTrack));
					// v2.05.74 fix: when not initialized yet, the mciEmu.dwCurrentTrack is 0 that is NOT a
					// valid track number. Better use dwSeekedTrack. Fixes "War of the Worlds".
					//parms->dwReturn = TTMMSS_TRACKNO(mciEmu.dwCurrentTrack);
					parms->dwReturn = TTMMSS_TRACKNO(currentTrack);
				}

				if (parms->dwItem == MCI_STATUS_LENGTH){
					int seconds; 
					dprintf("> MCI_STATUS_LENGTH tf=%s\n", sTimeFormat(mciEmu.dwTimeFormat));
					if(dwFlags & MCI_TRACK){
						// get length of specified track
						seconds = tracks[parms->dwTrack].length;
					}
					else {
						// v2.05.01: get total length of CD (fixed)
						seconds = TTMMSSToAbsSec(mciEmu.dwLastTrack); 
					}
					if (seconds == 0) seconds = 4; // assume no track is less than 4 seconds
					parms->dwReturn = DecodeSec(seconds);
					if(dxw.dwFlags11 & HACKMCIFRAMES){
						char *sRoom = NULL;
						switch(mciEmu.dwTimeFormat){
							case MCI_FORMAT_FRAMES: sRoom="frames"; break;
							case MCI_FORMAT_MILLISECONDS: sRoom="msec"; break;
							case MCI_FORMAT_TMSF: sRoom="tmsf"; break;
							case MCI_FORMAT_MSF: sRoom="msf"; break;
						}
						if(sRoom){
							DWORD val;
							char sKey[20];
							char hackPath[MAX_PATH];
							sprintf(hackPath, "%s\\Music\\mcihack.txt", dxw.cdAudioPath);
							if(dwFlags & MCI_TRACK) sprintf(sKey, "track%02d", parms->dwTrack);
							else strcpy(sKey, "cd");
							val = (*pGetPrivateProfileIntA)(sRoom, sKey, 0, hackPath);
							if(val) {
								parms->dwReturn = val;
								dprintf("> MCI_STATUS_LENGTH hacked value=%#x\n", parms->dwReturn);
							}
						}
					}
				}

				if (parms->dwItem == MCI_STATUS_POSITION){
					if (dwFlags & MCI_TRACK){
						dprintf("> MCI_STATUS_POSITION MCI_TRACK=%d tf=%s\n", parms->dwTrack, sTimeFormat(mciEmu.dwTimeFormat));
						// from MSDN: https://docs.microsoft.com/en-us/windows/desktop/multimedia/mci-status
						// When used with MCI_STATUS_POSITION, MCI_TRACK obtains the starting position of the specified track. 
						if(parms->dwTrack < 1) parms->dwTrack = 1;
						if(parms->dwTrack > MAX_TRACKS) parms->dwTrack = MAX_TRACKS;
						parms->dwReturn = DecodeTTMMSS(TTMMSS_ENCODE(parms->dwTrack, 0, 0)); 
						// v2.05.88: added HACKMCIFRAME for track start
						if(dxw.dwFlags11 & HACKMCIFRAMES){
							char *sRoom = NULL;
							switch(mciEmu.dwTimeFormat){
								case MCI_FORMAT_FRAMES: sRoom="start_frames"; break;
								case MCI_FORMAT_MILLISECONDS: sRoom="start_msec"; break;
								case MCI_FORMAT_TMSF: sRoom="start_tmsf"; break;
								case MCI_FORMAT_MSF: sRoom="start_msf"; break;
							}
							if(sRoom){
								DWORD val;
								char sKey[20];
								char hackPath[MAX_PATH];
								sprintf(hackPath, "%s\\Music\\mcihack.txt", dxw.cdAudioPath);
								if(dwFlags & MCI_TRACK) sprintf(sKey, "track%02d", parms->dwTrack);
								else strcpy(sKey, "cd");
								val = (*pGetPrivateProfileIntA)(sRoom, sKey, 0, hackPath);
								if(val) {
									parms->dwReturn = val;
									dprintf("> MCI_STATUS_LENGTH hacked value=%#x\n", parms->dwReturn);
								}
							}
						}
						// end of HACKMCIFRAMES
					}
					else {
						dprintf("> MCI_STATUS_POSITION tf=%s\n", sTimeFormat(mciEmu.dwTimeFormat));
						// v2.05.00/01: from MSDN - the dwReturn member is set to the current position
						parms->dwReturn = DecodeTTMMSS(mciEmu.dwCurrentTrack);
					}
				}

				if (parms->dwItem == MCI_CDA_STATUS_TYPE_TRACK) {
					// v2.05.91 fix: handle MCIERR_OUTOFRANGE errors - needed for WipeoutXL CD check
					if((parms->dwTrack == 0) || (parms->dwTrack > mciEmu.dwNumTracks)){
						dprintf("> MCI_CDA_STATUS_TYPE_TRACK ret=MCIERR_OUTOFRANGE track=%d\n", parms->dwTrack);
						return MCIERR_OUTOFRANGE;
					}
					// ref. by WinQuake, Sentinel Returns
					parms->dwReturn = (tracks[parms->dwTrack].type == MCI_AUDIO_TRACK) ? MCI_CDA_TRACK_AUDIO : MCI_CDA_TRACK_OTHER;
					dprintf("> MCI_CDA_STATUS_TYPE_TRACK=%s\n", sTrackType(parms->dwReturn));
				}

				if (parms->dwItem == MCI_STATUS_MEDIA_PRESENT) {
					parms->dwReturn = mciEmu.dwLastTrack > 0;
					dprintf("> MCI_STATUS_MEDIA_PRESENT=%#x\n", parms->dwReturn);
				}

				if (parms->dwItem == MCI_STATUS_NUMBER_OF_TRACKS){
					dprintf("> MCI_STATUS_NUMBER_OF_TRACKS\n");
					parms->dwReturn = mciEmu.dwNumTracks;
				}

				if (parms->dwItem == MCI_STATUS_MODE){
					dprintf("> MCI_STATUS_MODE: %s\n", mciEmu.playing ? "playing" : (mciEmu.paused ? "paused" : "stop"));
					parms->dwReturn = mciEmu.playing ? MCI_MODE_PLAY : MCI_MODE_STOP;
					// v2.04.98: the MCI_MODE_PAUSE state prevents "Speedboat Attack" to send
					// a MCI_RESUME command afterwards. That should depend on some driver's capability
					if((mciEmu.paused) && (dxw.dwFlags11 & CDPAUSECAPABILITY)) parms->dwReturn = MCI_MODE_PAUSE;
					// v2.05.75: when pausing the sound because of a focus lost, better pretend that the track
					// is still playing, or the program may try to start it again. Fixes "Asghan".
					extern BOOL wasPlaying;
					if((mciEmu.paused) && (dxw.dwFlags14 & STOPSOUND) && wasPlaying) parms->dwReturn = MCI_MODE_PLAY;
				}

				if (parms->dwItem == MCI_STATUS_READY) {
					// referenced by Quake/cd_win.c
					dprintf("> MCI_STATUS_READY\n");
					parms->dwReturn = TRUE; // TRUE=ready, FALSE=not ready
				}

				if (parms->dwItem == MCI_STATUS_TIME_FORMAT) {
					// untested
					dprintf("> MCI_STATUS_TIME_FORMAT format=%s\n", sTimeFormat(mciEmu.dwTimeFormat));
					parms->dwReturn = mciEmu.dwTimeFormat;
				}

				// v2.05.55 fix: MCI_STATUS_START is used on dwFlags, not on dwItem!!
				//_if (parms->dwItem == MCI_STATUS_START) dprintf("> MCI_STATUS_START\n");
			}

			dprintf("> params->dwReturn=%d(%#x)\n", parms->dwReturn, parms->dwReturn);
		}
		break;
	}

    return 0; 
}


