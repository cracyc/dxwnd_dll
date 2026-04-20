#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE 1
#define _MODULE "dxwnd" 

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <psapi.h>
#include <dbghelp.h>
#include <locale.h>
#include <time.h>
#include <lm.h>
//#include <wdm.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "shareddc.hpp"
#include "dxhook.h"
#include "glhook.h"
#include "glidehook.h"
#include "msvfwhook.h"
#define DXWDECLARATIONS 1
#include "syslibs.h"
#undef DXWDECLARATIONS
#include "dxhelper.h"
#include "Ime.h"
#include "Winnls32.h"
#include "Mmsystem.h"
#include "disasm.h"
#include "dwdisasm.h"
#include "MinHook.h" 
#include "asyncdc.h"

#define SKIPIMEWINDOW TRUE

dxwCore dxw;
dxwSStack dxwss;
dxwWStack dxwws;
dxwChildWStack dxwcws;
dxwSDC sdc;
AsyncDC adc;
dxwFStack fontdb;
GetWindowLong_Type pGetWindowLong;
SetWindowLong_Type pSetWindowLong;
HWND hTrayWnd;
AsyncDC *lpADC;
BOOL hookSemaphore = FALSE;

extern LRESULT CALLBACK MessageHook(int, WPARAM, LPARAM);
extern LRESULT CALLBACK KeyboardHook(int, WPARAM, LPARAM);
extern DWORD WINAPI CpuLimit(LPVOID); 
extern DWORD WINAPI CpuSlow(LPVOID); 
extern LONG CALLBACK Int3Handler(PEXCEPTION_POINTERS);
extern LONG WINAPI DxWExceptionHandler(LPEXCEPTION_POINTERS);
extern void PatchMSVBVM60();
extern void patcher();

static BOOL GetTextSegment(char *, unsigned char **, DWORD *);

DWORD dwB0000 = 0;

extern void *HotPatch(void *, const char *, void *);
extern void *IATPatchDefault(HMODULE, DWORD, char *, void *, const char *, void *);
extern void *IATPatchSequential(HMODULE, DWORD, char *, void *, const char *, void *);
extern void *IATPatchSeqHint(HMODULE, DWORD, char *, void *, const char *, void *);
extern void *IATPatchByFT(HMODULE, DWORD, char *, void *, const char *, void *);
typedef void * (*IATPatch_Type)(HMODULE, DWORD, char *, void *, const char *, void *);
#ifndef DXW_NOTRACES
extern void DumpImportTableDefault(HMODULE);
extern void DumpImportTableSequential(HMODULE);
extern void DumpImportTableSeqHint(HMODULE);
extern void DumpImportTableByFT(HMODULE);
typedef void (*DumpImportTable_Type)(HMODULE);
DumpImportTable_Type DumpImportTable;
#endif
IATPatch_Type IATPatch;
extern BOOL IsIATSequential(HMODULE);

DWORD WINAPI LockScreenMode(LPVOID);
void RecoverScreenMode();

extern HANDLE hTraceMutex;

CRITICAL_SECTION TraceCS; 
static DWORD dwTick0 = 0;
static BOOL CheckLocalProxy(char *);
static void HookHints(int);

#include <tlhelp32.h>
#include <Psapi.h>

DWORD getParentPID(DWORD pid)
{
    HANDLE h = NULL;
    PROCESSENTRY32 pe = { 0 };
    DWORD ppid = 0;
    pe.dwSize = sizeof(PROCESSENTRY32);
    h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if( Process32First(h, &pe)) {
        do  {
            if (pe.th32ProcessID == pid)  {
                ppid = pe.th32ParentProcessID;
                break;
            }
        } while( Process32Next(h, &pe));
    }
    CloseHandle(h);
    return (ppid);
}

int getProcessName(DWORD pid, char *fname, DWORD sz)
{
    HANDLE h = NULL;
    int e = 0;
    h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (h) {
		HMODULE hmod = (*LoadLibrary)("psapi.dll");
		if(!hmod) return 0;
		typedef DWORD (WINAPI *GetModuleFileNameExA_Type)(HANDLE, HMODULE, LPSTR, DWORD);
		GetModuleFileNameExA_Type pGetModuleFileNameExA = (GetModuleFileNameExA_Type)(*GetProcAddress)(hmod, "GetModuleFileNameExA");
		if(!pGetModuleFileNameExA) return 0;
        if ((*pGetModuleFileNameExA)(h, NULL, fname, sz) == 0)
            e = GetLastError();
        CloseHandle(h);
    }
    else{
        e = GetLastError();
    }
    return (e);
}

char *GetDxWndPath()
{
	static BOOL DoOnce = TRUE;
	static char sFolderPath[MAX_PATH];

	if(DoOnce){
		GetModuleFileName(GetModuleHandle("dxwnd"), sFolderPath, MAX_PATH);
		sFolderPath[strlen(sFolderPath)-strlen("dxwnd.dll")] = 0; // terminate the string just before "dxwnd.dll"
		DoOnce = FALSE;
	}

	return sFolderPath;
}

static void OutTraceHeader(FILE *fp, DWORD tflags, DWORD tflags2)
{
	SYSTEMTIME Time;
	int i;
	DWORD dword;
	GetLocalTime(&Time);
	fprintf(fp, "[ DxWnd %s log BEGIN: %02d-%02d-%04d %02d:%02d:%02d ]\n",
		GetDllVersion(), Time.wDay, Time.wMonth, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond);
	fprintf(fp, "[ Flags= ");
	for(i=0, dword=dxw.dwFlags1; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(0,i));
	for(i=0, dword=dxw.dwFlags2; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(1,i));
	for(i=0, dword=dxw.dwFlags3; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(2,i));
	for(i=0, dword=dxw.dwFlags4; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(3,i));
	for(i=0, dword=dxw.dwFlags5; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(4,i));
	for(i=0, dword=dxw.dwFlags6; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(5,i));
	for(i=0, dword=dxw.dwFlags7; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(6,i));
	for(i=0, dword=dxw.dwFlags8; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(7,i));
	for(i=0, dword=dxw.dwFlags9; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(8,i));
	for(i=0, dword=dxw.dwFlags10;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(9,i));
	for(i=0, dword=dxw.dwFlags11;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(10,i));
	for(i=0, dword=dxw.dwFlags12;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(11,i));
	for(i=0, dword=dxw.dwFlags13;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(12,i));
	for(i=0, dword=dxw.dwFlags14;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(13,i));
	for(i=0, dword=dxw.dwFlags15;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(14,i));
	for(i=0, dword=dxw.dwFlags16;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(15,i));
	for(i=0, dword=dxw.dwFlags17;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(16,i));
	for(i=0, dword=dxw.dwFlags18;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(17,i));
	for(i=0, dword=dxw.dwFlags19;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(18,i));
	for(i=0, dword=dxw.dwFlags20;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(19,i));
	for(i=0, dword=tflags      ; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(20,i));
	for(i=0, dword=tflags2     ; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(21,i));
	for(i=0, dword=dxw.dwDFlags; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(22,i));
	for(i=0, dword=dxw.dwDFlags2;i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", GetFlagCaption(23,i));
	fprintf(fp, "]\n");
	fprintf(fp, "[ %d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d ]\n",
		dxw.dwFlags1, dxw.dwFlags2, dxw.dwFlags3, dxw.dwFlags4,
		dxw.dwFlags5, dxw.dwFlags6, dxw.dwFlags7, dxw.dwFlags8,
		dxw.dwFlags9, dxw.dwFlags10, dxw.dwFlags11, dxw.dwFlags12,
		dxw.dwFlags13, dxw.dwFlags14, dxw.dwFlags15, dxw.dwFlags16,
		dxw.dwFlags17, dxw.dwFlags18, dxw.dwFlags19, dxw.dwFlags20);
}

#define DXWMAXLOGSIZE 4096
#define MAXLINECOUNT 50000

#ifndef DXW_NOTRACES
void OutTrace(const char *format, ...)
{
	va_list al;
	static char path[MAX_PATH];
	static FILE *fp=NULL; // GHO: thread safe???
	char sBuf[DXWMAXLOGSIZE+2];
	DWORD tFlags, tFlags2;
	static GetTickCount_Type pGetTick;
	static int iLineCount = 0;

	// check global log flag
	if(!(dxw.dwTFlags & (OUTTRACE|OUTDEBUGSTRING))) return;
	tFlags = dxw.dwTFlags;
	tFlags2 = dxw.dwTFlags2;
	dxw.dwTFlags = 0x0; // to avoid possible log recursion while loading C runtime libraries!!!

	WaitForSingleObject(hTraceMutex, INFINITE);

	if(tFlags & OUTCIRCULAR){
		iLineCount++;
		if(iLineCount > MAXLINECOUNT){
			// move current logfile to old
			char oldpath[MAX_PATH];
			if(tFlags & OUTONTEMPFOLDER)
				GetTempPathA(MAX_PATH, oldpath);
			else 
				GetCurrentDirectory(MAX_PATH, oldpath);
			char *p = oldpath + strlen(oldpath); 
			strcat(oldpath, "\\dxwnd.0.log");
			fclose(fp);
			_unlink(oldpath); // no error check, it may not exist
			rename(path, oldpath);
			// reset
			fp = NULL;
			iLineCount = 0;
		}
	}
	if (fp == NULL){
		char *OpenMode = (tFlags & ERASELOGFILE) ? "w+" : "a+";
		pGetTick = GetTickCount; // save function pointer
		if(tFlags & OUTONTEMPFOLDER)
			GetTempPathA(MAX_PATH, path);
		else 
			GetCurrentDirectory(MAX_PATH, path);
		if(tFlags & OUTSEPARATED){
			char *p = path + strlen(path); 
			strcat(path, "\\dxwnd.log");
			if(fp = fopen(path, "r")){
				fclose(fp);
				for (int i=1; ; i++){
					sprintf(p, "\\dxwnd(%d).log", i);
					if(fp = fopen(path, "r")) fclose(fp);
					else break;
				}
			}
		}
		else {
			strcat(path, "\\dxwnd.log");
		}
		fp = fopen(path, OpenMode);
		if (fp==NULL){ // in case of error (e.g. current dir on unwritable CD unit)... 
			strcpy(path, GetDxWndPath());
			strcat(path, "\\dxwnd.log");
			fp = fopen(path, OpenMode);
		}
		if (fp==NULL)
			return; // last chance: do not log... 
		else {
			// v2.05.92 fix: set dwTick0 only once, do not reset the timestamp on CIRCULAR option.
			if(dwTick0 == 0) dwTick0 = GetTickCount();
			OutTraceHeader(fp, tFlags, tFlags2);
		}
	}
	va_start(al, format);
	//vfprintf(fp, format, al);
	__try{
		vsprintf_s(sBuf, DXWMAXLOGSIZE, format, al);
	} 
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// v2.05.23: more info on excepted logs ...
		//strcpy(sBuf, "***\n");
		sprintf_s(sBuf, DXWMAXLOGSIZE, "*** >>> %s\n", format);
	}
	sBuf[DXWMAXLOGSIZE]=0; // just in case of log truncation
	va_end(al);
	if(tFlags & OUTTRACE) {
		if(tFlags & ADDTIMESTAMP) {
			DWORD tCount = (*pGetTick)() - dwTick0;
			if (tFlags & ADDRELATIVETIME){
				static DWORD tLastCount = 0;
				DWORD tNow;
				tNow = tCount;
				tCount = tLastCount ? (tCount - tLastCount) : 0;
				tLastCount = tNow;
			}
			//fprintf(fp, "%08.8d: ", tCount);
			fprintf(fp, "[%05.5d:%03.3d] ", tCount/1000, tCount%1000);
			// set elapsed time (sec) to status win
			GetHookInfo()->upTime = tCount/1000; 
		}
		if(tFlags & ADDTHREADID){
			fprintf(fp, "(%08.8X) ", GetCurrentThreadId());
		}
		fputs(sBuf, fp);
		fflush(fp); 
	}
	if(tFlags & OUTDEBUGSTRING) OutputDebugString(sBuf);
	ReleaseMutex(hTraceMutex);

	dxw.dwTFlags = tFlags; // restore settings
}

#ifndef NEWHEXLAYOUT
#define HEXCOLUMNS 16
void HexTrace(unsigned char *buf, int len)
{
	char line[3*HEXCOLUMNS + 50 + HEXCOLUMNS];
	char hex[6];
	char asciiBuf[HEXCOLUMNS+2];
	int count=0;
	char *out;
	int i, j;
	while(len){
		out = line;
		sprintf(line, "%04.4X: ", count);
		out += strlen(line);
		for(i=HEXCOLUMNS, j=0; i && len; i--, j++, len--, buf++){
			unsigned char c;
			__try {
				c = *buf;
			}
			__except(EXCEPTION_EXECUTE_HANDLER){
				strcpy(out, "<e>");
				len = 0;
				break;
			}
			sprintf(hex, "%02X.", c);
			strcpy(out, hex);
			out += 3;
			asciiBuf[j] = isprint(c) ? c : '.';
		}
		//asciiBuf[j++] = '\n';
		asciiBuf[j++] = 0;
		for(; i; i--) {
			strcpy(out, "__."); // align length
			out += 3;
		}
		OutTrace("%s - %s\n", line, asciiBuf);
		count += HEXCOLUMNS;
	}
}
#else
void HexTrace(unsigned char *buf, int len)
{
	char line[3*32 + 40];
	char hex[6];
	int count=0;
	while(len){
		sprintf(line,"%04X: ", count);
		for(int i=32; i && len; i--, len--, buf++){
			sprintf(hex, "%02X.", *buf);
			strcat(line, hex);
		}
		OutTrace("%s\n", line);
		count += 32;
	}
}
#endif
#endif

// from MSDN:
// GetVersionEx may be altered or unavailable for releases after Windows 8.1. Instead, use the Version Helper functions
//
// With the release of Windows 8.1, the behavior of the GetVersionEx API has changed in the value it will return for the 
// operating system version. The value returned by the GetVersionEx function now depends on how the application is manifested.
//
// Applications not manifested for Windows 8.1 or Windows 10 will return the Windows 8 OS version value (6.2). 
// Once an application is manifested for a given operating system version, GetVersionEx will always return the version 
// that the application is manifested for in future releases. 
// To manifest your applications for Windows 8.1 or Windows 10, refer to Targeting your application for Windows.

static BOOL CheckCompatibilityFlags()
{
	typedef DWORD (WINAPI *GetFileVersionInfoSizeA_Type)(LPCSTR, LPDWORD);
	typedef BOOL (WINAPI *GetFileVersionInfoA_Type)(LPCSTR, DWORD, DWORD, LPVOID);
	typedef BOOL (WINAPI *VerQueryValueA_Type)(LPCVOID, LPCSTR, LPVOID, PUINT);
	VerQueryValueA_Type pVerQueryValueA;
	GetFileVersionInfoA_Type pGetFileVersionInfoA;
	GetFileVersionInfoSizeA_Type pGetFileVersionInfoSizeA;

	HMODULE VersionLib;
	DWORD dwMajorVersion, dwMinorVersion;
	DWORD dwHandle = 0;
	int size;
	UINT len = 0;
    VS_FIXEDFILEINFO*   vsfi = NULL;
	OSVERSIONINFO vi;

	if(!(VersionLib=LoadLibrary("Version.dll"))) return FALSE;
	pGetFileVersionInfoA=(GetFileVersionInfoA_Type)GetProcAddress(VersionLib, "GetFileVersionInfoA");
	if(!pGetFileVersionInfoA) return FALSE;
	pGetFileVersionInfoSizeA=(GetFileVersionInfoSizeA_Type)GetProcAddress(VersionLib, "GetFileVersionInfoSizeA");
	if(!pGetFileVersionInfoSizeA) return FALSE;
	pVerQueryValueA=(VerQueryValueA_Type)GetProcAddress(VersionLib, "VerQueryValueA");
	if(!pVerQueryValueA) return FALSE;

	size = (*pGetFileVersionInfoSizeA)("kernel32.dll", &dwHandle);	
	BYTE* VersionInfo = new BYTE[size];
	(*pGetFileVersionInfoA)("kernel32.dll", dwHandle, size, VersionInfo);
    (*pVerQueryValueA)(VersionInfo, "\\", (void**)&vsfi, &len);
	dwMajorVersion=HIWORD(vsfi->dwProductVersionMS);
	dwMinorVersion=LOWORD(vsfi->dwProductVersionMS);
    delete[] VersionInfo;	
	vi.dwOSVersionInfoSize=sizeof(vi);
	GetVersionExA(&vi);
	if((vi.dwMajorVersion!=dwMajorVersion) || (vi.dwMinorVersion!=dwMinorVersion)) {
		ShowHint(HINT_FAKEOS);
		return TRUE;
	}
	return FALSE;
}

static int hex2char(unsigned char *dst, char *src)
{
	char buf[3];
	int val;
	int len = strlen(src);
	int retlen = 0;
	buf[2]=0; // string terminator
	for (; len; len-=2, retlen++){
		buf[0]=*src;
		buf[1]=*(src+1);
		sscanf(buf, "%X", &val);
		*dst = (unsigned char)val;
		src += 2;
		dst ++;
	}
	return retlen;
}

void LoadGammaRamp()
{
	ApiName("LoadGammaRamp");
	char gInitPath[MAX_PATH];
	BYTE GammaBuffer[256*3];
	char GammaString[256*2 + 16]; 
	sprintf(gInitPath, "%sdxwnd.ini", GetDxWndPath()); 
	OutTrace("%s: IniPath=%s\n", ApiRef, gInitPath);
	GetHookInfo()->GammaControl = (*pGetPrivateProfileIntA)("window", "gammaenabled", 0,  gInitPath);
	// red
	(*pGetPrivateProfileStringA)("window", "gammared", "", GammaString, 256*2+1,  gInitPath);
	OutTrace("%s: Gamma[red]=%s\n", ApiRef, GammaString);
	hex2char(&GammaBuffer[0], GammaString);
	// green
	(*pGetPrivateProfileStringA)("window", "gammagreen", "", GammaString, 256*2+1,  gInitPath);
	OutTrace("%s: Gamma[green]=%s\n", ApiRef, GammaString);
	hex2char(&GammaBuffer[256], GammaString);
	// blue
	(*pGetPrivateProfileStringA)("window", "gammablue", "", GammaString, 256*2+1,  gInitPath);
	OutTrace("%s: Gamma[blue]=%s\n", ApiRef, GammaString);
	hex2char(&GammaBuffer[512], GammaString);
	//GetHookInfo()->GammaControl = TRUE;
	memcpy(GetHookInfo()->GammaRamp, GammaBuffer, sizeof(GammaBuffer));
}

void HookDlls(HMODULE module)
{
	ApiName("HookDlls");
	PIMAGE_NT_HEADERS pnth;
	PIMAGE_IMPORT_DESCRIPTOR pidesc;
	DWORD base, rva;
	PSTR impmodule;
	PIMAGE_THUNK_DATA ptname;
	static int depth = 0;

	depth++;
#ifndef DXW_NOTRACES
	if(IsDebugH){
		char sPath[MAX_PATH+1];
		GetModuleFileName(module, sPath, MAX_PATH);
		//OutDebugH("HookDlls: base=%#x depth=%d path=\"%s\"\n", base, depth, sPath);
		OutTrace("%s(%d): hmod=%#x path=\"%s\"\n", ApiRef, depth, module, sPath);
	}
#endif // DXW_NOTRACES

	if(dxw.ht.hGet(module) && (depth > 1)) { // already visited, skip and stop recursion
		depth--;
		OutDebugH("%s(%d): visited\n", ApiRef, depth);
		return;
	}

	base=(DWORD)module;
	__try{
		pnth = PIMAGE_NT_HEADERS(PBYTE(base) + PIMAGE_DOS_HEADER(base)->e_lfanew);
		if(!pnth) {
			OutTraceH("%s(%d): ERROR no pnth @%d\n", ApiRef, depth, __LINE__);
			depth--;
			return;
		}
		rva = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		if(!rva) {
			OutTraceH("%s(%d): ERROR no rva @%d\n", ApiRef, depth, __LINE__);
			depth--;
			return;
		}

		for(pidesc = (PIMAGE_IMPORT_DESCRIPTOR)(base + rva); pidesc->FirstThunk; pidesc++){
			HMODULE DllBase;
			int idx;

			impmodule = (PSTR)(base + pidesc->Name);

			// skip dxwnd and system dll
			if(!lstrcmpi(impmodule, "DxWnd")) continue; 
			if(dxw.dwFlags19 & STARTDETOURS){
				if(strstr(impmodule, "dxwnd.dll")) continue;
			}
			idx=dxw.GetDLLIndex(impmodule);
			if(!lstrcmpi(impmodule,dxw.CustomOpenGLLib))idx=SYSLIBIDX_OPENGL;
			if(idx != -1) {
				dxwLibsHookTable_Type *lpEntry = &SysLibsTable[idx];
				DllBase=GetModuleHandle(impmodule);
				lpEntry->hmodule = DllBase;
				OutTraceH("%s(%d): system module %s @%#x\n", ApiRef, depth, impmodule, DllBase);

				// v2.05.35: bring the hook task here and get rid of HookModule unconditioned logic!!
				if((lpEntry->flags & DXWHOOK_HOOK) && lpEntry->hookf) (*lpEntry->hookf)(module);

				// v2.04.54: New! Capability to hook a module AND its dependencies.
				// It is required in SmackW32.dll, maybe others?
				if(!(lpEntry->flags & DXWHOOK_EXTEND)) continue;

				if(dxw.bHintActive) HookHints(idx);
			}

			OutTraceH("%s(%d): ENTRY timestamp=%#x module=%s forwarderchain=%#x\n", 
				ApiRef, depth, pidesc->TimeDateStamp, impmodule, pidesc->ForwarderChain);
			if(pidesc->OriginalFirstThunk) {
				ptname = (PIMAGE_THUNK_DATA)(base + (DWORD)pidesc->OriginalFirstThunk);
			}
			else{
				ptname = 0;
				OutTraceH("%s(%d): no PE OFTs - stripped module=%s\n", 
					ApiRef, depth, impmodule);
			}

			DllBase=GetModuleHandle(impmodule);
			if(DllBase) {
				HookDlls(DllBase); // recursion !!!
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{       
		OutTraceDW("%s: EXCEPTION\n", ApiRef);
	}
	OutDebugH("%s(%d): base=%#x END\n", ApiRef, depth, base);
	depth--;
	return;
}

// CheckImportTable: a good enough criteria to detect obfuscated executables is to count the entries in the most common
// and somehow mandatory system dlls such as kernel32.dll, user32.dll and gdi32.dll
// the routine counsts the kernel32.dll overall entries (they could be split in different sections!) and if lesser than 3
// a warning message is shown.

void CheckImportTable(HMODULE module)
{
	ApiName("CheckImportTable");
	PIMAGE_NT_HEADERS pnth;
	PIMAGE_IMPORT_DESCRIPTOR pidesc;
	DWORD base, rva;
	PSTR impmodule;
	PIMAGE_THUNK_DATA ptaddr;
	int Kernel32Count = 0;

	base=(DWORD)module;
	__try{
		pnth = PIMAGE_NT_HEADERS(PBYTE(base) + PIMAGE_DOS_HEADER(base)->e_lfanew);
		if(!pnth) {
			return;
		}
		rva = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		if(!rva) {
			return;
		}
		pidesc = (PIMAGE_IMPORT_DESCRIPTOR)(base + rva);

		while(pidesc->FirstThunk){
			impmodule = (PSTR)(base + pidesc->Name);
			if(!_stricmp (impmodule, "kernel32.dll")){
				ptaddr = (PIMAGE_THUNK_DATA)(base + (DWORD)pidesc->FirstThunk);
				while(ptaddr->u1.Function){
					ptaddr ++;
					Kernel32Count++;
				}
			}
			// warning: do not confuse "dplayerx.dll" (SafeDisk) with "dplayx.dll" (DirectPlay)!
			if(!_stricmp (impmodule, "dplayerx.dll")) ShowHint(HINT_SAFEDISC);
			if(!_stricmp (impmodule, "cms_95.dll")) ShowHint(HINT_SECUROM);
			if(!_stricmp (impmodule, "cms_NT.dll")) ShowHint(HINT_SECUROM);
			if(!_stricmp (impmodule, "cms16.dll"))  ShowHint(HINT_SECUROM);
			pidesc ++;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{       
		OutTraceDW("%s: EXCEPTION\n", ApiRef);
	}

	OutTraceDW("%s: found %d entries for kernel32.dll\n", ApiRef, Kernel32Count);
	if(Kernel32Count <= 3) ShowHint(HINT_OBFUSCATED);
	return;
}

void SetHook(void *target, void *hookproc, void **hookedproc, char *hookname)
{
	ApiName("SetHook");
	void *tmp;
	char msg[201];
	DWORD dwTmp, oldprot;

	// v2.04.86: Very tricky. COM objects can be created with copied pointers, so you may have an object whose pointers
	// are identical to the ones you're about to hook. In this case the first two checks for "target already hooked" and
	// "hook already hooked" are enough to prevent a double redirection. But in some rare cases (e.g. "Darkstone") the
	// object version could be different so that the pointers don't match. In this case it is necessary to compare the
	// function pointer with the address range of all dxwnd.dll segment to detect a secure double redirection and bypass
	// it.

	static DWORD minaddr = 0, maxaddr;
	if(!minaddr){
		MODULEINFO ModuleInfo;
		typedef BOOL (WINAPI *GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
		HMODULE psapilib;	
		GetModuleInformation_Type pGetModuleInformation;
		// getting segment size
		psapilib=(*pLoadLibraryA)("psapi.dll");
		if(!psapilib) {
			OutTraceDW("%s: Load lib=\"%s\" failed err=%d\n", "psapi.dll", ApiRef, GetLastError());
			minaddr = -1;
			maxaddr = -1;
		}
		else {
			pGetModuleInformation=(GetModuleInformation_Type)(*pGetProcAddress)(psapilib, "GetModuleInformation");
			minaddr=(DWORD)GetModuleHandle("dxwnd");
			(*pGetModuleInformation)(GetCurrentProcess(), (HMODULE)minaddr, &ModuleInfo, sizeof(MODULEINFO));
			minaddr=(DWORD)ModuleInfo.lpBaseOfDll;
			maxaddr=minaddr+ModuleInfo.SizeOfImage;
			FreeLibrary(psapilib);
			OutTraceH("%s: dxwnd address range min/max=%#x/%#x\n", ApiRef, minaddr, maxaddr);
		}
	}
	
	OutTraceH("%s: DEBUG target=%#x, proc=%#x name=%s\n", ApiRef, target, hookproc, hookname);
	// keep track of hooked call range to avoid re-hooking of hooked addresses !!!
	dwTmp = *(DWORD *)target;
	if(dwTmp == (DWORD)hookproc) {
		OutTraceH("%s: target already hooked\n", ApiRef);
		return; // already hooked
	}
	if(hookedproc){
		if(*(DWORD *)hookedproc == (DWORD)hookproc) {
			OutTraceH("%s: hook already hooked\n", ApiRef);
			return; // already hooked
		}
		if((*(DWORD *)hookedproc > minaddr) && (*(DWORD *)hookedproc < maxaddr)){
			OutTraceH("%s: hook in dxwnd range\n", ApiRef);
			return; // already hooked
		}
	}
	if(dwTmp == 0){
		sprintf(msg,"ERROR - NULL target for %s", hookname);
		*hookedproc = NULL; // v2.06.05
		OutTraceDW("%s: %s\n", ApiRef, msg);
		if (IsAssertEnabled) MessageBox(0, msg, "SetHook", MB_OK | MB_ICONEXCLAMATION);
		return; // error condition
	} 
	if(!VirtualProtect(target, 4, PAGE_READWRITE, &oldprot)) {
		sprintf(msg,"ERROR - target=%#x err=%d", target, GetLastError());
		OutTraceDW("%s: %s\n", ApiRef, msg);
		if (IsAssertEnabled) MessageBox(0, msg, "SetHook", MB_OK | MB_ICONEXCLAMATION);
		return; // error condition
	}
	*(DWORD *)target = (DWORD)hookproc;
	if(!VirtualProtect(target, 4, oldprot, &oldprot)){
		OutTrace("%s: VirtualProtect ERROR target=%#x, err=%#x\n", ApiRef, target, GetLastError());
		return; // error condition
	}
	if(!FlushInstructionCache(GetCurrentProcess(), target, 4)){
		OutTrace("%s: FlushInstructionCache ERROR target=%#x, err=%#x\n", ApiRef, target, GetLastError());
		return; // error condition
	}
	tmp=(void *)dwTmp;

	if(hookedproc){
		__try {
			if (*hookedproc && (*hookedproc)!=tmp) {
				// here we find an original address different from the previously registered one!!
				sprintf(msg,"proc=%s oldhook=%#x->%#x newhook=%#x", hookname, hookedproc, *(DWORD *)hookedproc, tmp);
				OutTraceDW("%s: %s\n", ApiRef, msg);
				if (IsAssertEnabled) MessageBox(0, msg, "SetHook", MB_OK | MB_ICONEXCLAMATION);
				if (dxw.bHintActive) ShowHint(HINT_HOOKUPDATE);
				// v2.03.83: updating the pointer sometimes is good, sometimes is bad!
				if(dxw.dwFlags7 & HOOKDOUPDATE) *hookedproc = tmp;
			}
			else {
				*hookedproc = tmp;
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
				OutTrace("%s: %s exception\n", ApiRef, hookname);
		};
	}
	OutTraceH("%s: *hookedproc=%#x, name=%s\n", ApiRef, tmp, hookname);
}

// v2.02.53: thorough scan - the IAT is scanned considering the possibility to have each dll module 
// replicated also many times. It may depend upon the compiling environment? 
// So far, it makes the difference for Dungeon Odissey

void *HookAPI(HMODULE module, char *dll, void *apiproc, const char *apiname, void *hookproc)
{
	ApiName("HookAPI");
#ifndef DXW_NOTRACES
	if(dxw.dwTFlags2 & OUTIMPORTTABLE) OutTrace("%s: module=%#x dll=%s apiproc=%#x apiname=%s hookproc=%#x\n", 
		ApiRef, module, dll, apiproc, apiname, hookproc);
#endif // DXW_NOTRACES

	if(!*apiname) { // check
#ifndef DXW_NOTRACES
		char *sMsg="NULL api name";
		OutTraceE("%s: %s\n", ApiRef, sMsg);
		if (IsAssertEnabled) MessageBox(0, sMsg, "HookAPI", MB_OK | MB_ICONEXCLAMATION);
#endif // DXW_NOTRACES
		return 0;
	}

	if(dxw.dwFlags15 & HOTPATCHALWAYS) {
		void *orig;
		orig=HotPatch(apiproc, apiname, hookproc);
		if(orig) return orig;
	}

	return IATPatch(module, 0, dll, apiproc, apiname, hookproc);
}

DEVMODE InitDevMode;

static void SaveScreenMode()
{
	ApiName("SaveScreenMode");
	static BOOL DoOnce=FALSE;
	if(DoOnce) return;
	DoOnce=TRUE;
	(*pEnumDisplaySettingsA)(NULL, ENUM_CURRENT_SETTINGS, &InitDevMode);
	OutTraceDW("%s: Initial display mode WxH=(%dx%d) BitsPerPel=%d\n", 
		ApiRef, InitDevMode.dmPelsWidth, InitDevMode.dmPelsHeight, InitDevMode.dmBitsPerPel);
	if(dxw.isColorEmulatedMode){
		dxw.iPos0X = dxw.iPos0Y = dxw.iPosX = dxw.iPosY = 0;
		dxw.iSiz0X = dxw.iSizX = InitDevMode.dmPelsWidth;
		dxw.iSiz0Y = dxw.iSizY = InitDevMode.dmPelsHeight;
		dxw.iMaxW = InitDevMode.dmPelsWidth;
		dxw.iMaxH =  InitDevMode.dmPelsHeight;
	}

}

DWORD WINAPI LockScreenMode(LPVOID lpData)
{
	ApiName("LockScreenMode");
	DEVMODE CurrentDevMode, DefaultDevMode;
	BOOL res;
	(*pEnumDisplaySettingsA)(NULL, ENUM_REGISTRY_SETTINGS, &DefaultDevMode);
	OutTraceDW("%s: default display mode WxH=(%dx%d) BitsPerPel=%d\n", 
		ApiRef, DefaultDevMode.dmPelsWidth, DefaultDevMode.dmPelsHeight, DefaultDevMode.dmBitsPerPel);
	while(TRUE){
		(*pEnumDisplaySettingsA)(0, ENUM_CURRENT_SETTINGS, &CurrentDevMode);
		if ((CurrentDevMode.dmPelsWidth != DefaultDevMode.dmPelsWidth) ||
			(CurrentDevMode.dmPelsHeight != DefaultDevMode.dmPelsHeight) ||
			(CurrentDevMode.dmBitsPerPel != DefaultDevMode.dmBitsPerPel)){
			OutTraceDW("%s: recover video mode res=(%dx%d) bpp=%d\n",
				ApiRef, DefaultDevMode.dmPelsWidth, DefaultDevMode.dmPelsHeight, DefaultDevMode.dmBitsPerPel);
			// using NULL, 0 ... as parameters works also on multi-monitor configuration
			res=(*pChangeDisplaySettingsExA)(NULL, 0, NULL, 0, NULL);
#ifndef DXW_NOTRACES
			if(res != DISP_CHANGE_SUCCESSFUL) {
				OutTraceE("%s: ChangeDisplaySettings: res=%d ERROR err=%d @%d\n", 
				ApiRef, res, GetLastError(), __LINE__);
			}
#endif // DXW_NOTRACES
		}
		// pause ...
		(*pSleep)(200);
	}
}

void RecoverScreenMode()
{
	ApiName("RecoverScreenMode");
	DEVMODE CurrentDevMode;
	BOOL res;
	(*pEnumDisplaySettingsA)(NULL, ENUM_CURRENT_SETTINGS, &CurrentDevMode);
	OutTraceDW("%s: recover CURRENT WxH=(%dx%d) BitsPerPel=%d TARGET WxH=(%dx%d) BitsPerPel=%d\n", 
		ApiRef, CurrentDevMode.dmPelsWidth, CurrentDevMode.dmPelsHeight, CurrentDevMode.dmBitsPerPel,
		InitDevMode.dmPelsWidth, InitDevMode.dmPelsHeight, InitDevMode.dmBitsPerPel);
	InitDevMode.dmFields = (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT);
	res=(*pChangeDisplaySettingsExA)(NULL, &InitDevMode, NULL, 0, NULL);
#ifndef DXW_NOTRACES
	if(res) OutTraceE("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
}

void SwitchTo16BPP()
{
	ApiName("SwitchTo16BPP");
	DEVMODE CurrentDevMode;
	BOOL res;
	(*pEnumDisplaySettingsA)(NULL, ENUM_CURRENT_SETTINGS, &CurrentDevMode);
	OutTraceDW("%s: CURRENT wxh=(%dx%d) BitsPerPel=%d -> 16\n", 
		ApiRef, CurrentDevMode.dmPelsWidth, CurrentDevMode.dmPelsHeight, CurrentDevMode.dmBitsPerPel);
	CurrentDevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	CurrentDevMode.dmBitsPerPel = 16;
	res=(*pChangeDisplaySettingsExA)(NULL, &CurrentDevMode, NULL, CDS_UPDATEREGISTRY, NULL);
#ifndef DXW_NOTRACES
	if(res) OutTraceE("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
#endif // DXW_NOTRACES
}

static BOOL CheckLocalProxy(char *dllpath)
{
	char path[MAX_PATH];
	sprintf_s(path, MAX_PATH, ".\\%s.dll", dllpath);
	FILE *testFp = fopen(path, "r");
	if(testFp){
		fclose(testFp);
		return true;
	}
	return false;
}

static void HookHints(int idx)
{
	switch(idx){
		case SYSLIBIDX_DIRECTDRAW:	ShowHint(HINT_DDRAW);	break;
		case SYSLIBIDX_DIRECT3D8:	ShowHint(HINT_D3D8);	break;
		case SYSLIBIDX_DIRECT3D9:	ShowHint(HINT_D3D9);	break;
		// DirectX10 & 11 are loaded by some system module, so better not to tell
		//case SYSLIBIDX_DIRECT3D10:	
		//case SYSLIBIDX_DIRECT3D10_1:ShowHint(HINT_D3D10);	break;
		//case SYSLIBIDX_DIRECT3D11:	ShowHint(HINT_D3D11);	break;
		case SYSLIBIDX_OPENGL:		ShowHint(HINT_OPENGL);	break;
		case SYSLIBIDX_DSOUND:		ShowHint(HINT_DSOUND);	break;
		case SYSLIBIDX_DINPUT:		ShowHint(HINT_DINPUT);	break;
		case SYSLIBIDX_DINPUT8:		ShowHint(HINT_DINPUT8);	break;
		case SYSLIBIDX_XINPUT11:	ShowHint(HINT_XINPUT);	break;
		case SYSLIBIDX_XINPUT12:	ShowHint(HINT_XINPUT);	break;
		case SYSLIBIDX_XINPUT13:	ShowHint(HINT_XINPUT);	break;
		case SYSLIBIDX_XINPUT14:	ShowHint(HINT_XINPUT);	break;
		case SYSLIBIDX_XINPUT910:	ShowHint(HINT_XINPUT);	break;
		case SYSLIBIDX_MSVFW:		
		case SYSLIBIDX_WINMM:		
		case SYSLIBIDX_AVIFIL32:	ShowHint(HINT_MOVIES);	break;
		case SYSLIBIDX_DIRECT3D:
		case SYSLIBIDX_DIRECT3D700:	ShowHint(HINT_D3D);		break;
		case SYSLIBIDX_IMAGEHLP:	ShowHint(HINT_IHLP);	break;
	}
	if(CheckLocalProxy(SysLibsTable[idx].name)) ShowHint(HINT_PROXY);
}

static void InitModuleHooks()
{
	for (int i=0; SysLibsTable[i].name; i++) SysLibsTable[i].hmodule=NULL;
}

void HookModule(HMODULE base)
{
	for(int i = 0; SysLibsTable[i].name; i++){
		dxwLibsHookTable_Type *lpEntry = &SysLibsTable[i];
		if((lpEntry->flags & DXWHOOK_HOOK) && lpEntry->hookf) (*lpEntry->hookf)(base);
	}
}

void SetModuleHooks()
{
	ApiName("SetModuleHooks");
	int i;
	HMODULE hModule;

	for (i=0; SysLibsTable[i].name; i++){
		// v2.05.85 fix: the "flags" field is a bit mask, so don't compare it with ==
		// but use & instead. Fixes "Total Annihilation" missing hook in patched version.
		if((SysLibsTable[i].hmodule==NULL) && (SysLibsTable[i].flags & DXWHOOK_HOOK)){
			hModule = GetModuleHandle(SysLibsTable[i].name);
			if(hModule) {
				SysLibsTable[i].hmodule = hModule;
				OutTraceDW("%s: lib=%s hmodule=%#x\n", ApiRef, SysLibsTable[i].name, hModule);
				if(dxw.bHintActive) HookHints(i);
			}
		}
	}
}

void ClearModuleHooks()
{
	ApiName("ClearModuleHooks");
	int i, count;

	count = 0;
	for (i=0; SysLibsTable[i].name; i++){
		if(SysLibsTable[i].hmodule) {
			SysLibsTable[i].hmodule = NULL;
			count++;
		}
	}
	OutTrace("%s: cleared=%d\n", ApiRef, count);
}

#define USEWINNLSENABLE

void DisableIME()
{
	ApiName("DisableIME");
	BOOL res;
	HMODULE hm;
	hm=GetModuleHandle("User32");
	if(hm==NULL){
		OutTrace("%s: GetModuleHandle(User32) ERROR err=%d\n", ApiRef, GetLastError());
		return;
	}
	// here, GetProcAddress may be not hooked yet!
	if(!pGetProcAddress) pGetProcAddress=GetProcAddress;
#ifdef USEWINNLSENABLE
	typedef BOOL (WINAPI *WINNLSEnableIME_Type)(HWND, BOOL);
	WINNLSEnableIME_Type pWINNLSEnableIME;
	pWINNLSEnableIME=(WINNLSEnableIME_Type)(*pGetProcAddress)(hm, "WINNLSEnableIME");
	OutTrace("%s: GetProcAddress(WINNLSEnableIME)=%#x\n", ApiRef, pWINNLSEnableIME);
	if(!pWINNLSEnableIME) return;
	SetLastError(0);
	res=(*pWINNLSEnableIME)(NULL, FALSE);
	OutTrace("%s: IME previous state=%#x error=%d\n", ApiRef, res, GetLastError());
#else
	typedef LRESULT (WINAPI *SendIMEMessage_Type)(HWND, LPARAM);
	SendIMEMessage_Type pSendIMEMessage;
	//pSendIMEMessage=(SendIMEMessage_Type)(*pGetProcAddress)(hm, "SendIMEMessage");
	pSendIMEMessage=(SendIMEMessage_Type)(*pGetProcAddress)(hm, "SendIMEMessageExA");
	OutTrace("DisableIME: GetProcAddress(SendIMEMessage)=%#x\n", pSendIMEMessage);
	if(!pSendIMEMessage) return;
	HGLOBAL imeh;
	IMESTRUCT *imes;
	imeh=GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE, sizeof(IMESTRUCT));
	imes=(IMESTRUCT *)imeh;
	//imes->fnc=IME_SETLEVEL;
	imes->fnc=7;
	imes->wParam=1;
	SetLastError(0);
	res=(*pSendIMEMessage)(dxw.GethWnd(), (LPARAM)imeh);
	OutTrace("res=%#x error=%d\n", res, GetLastError());
#endif
}

void SetSingleProcessAffinity(BOOL first)
{
	ApiName("SetSingleProcessAffinity");
	int i;
	BOOL res;
	DWORD ProcessAffinityMask, SystemAffinityMask;
#ifndef DXW_NOTRACES
	if(!GetProcessAffinityMask(GetCurrentProcess(), &ProcessAffinityMask, &SystemAffinityMask))
		OutTraceE("%s: GetProcessAffinityMask ERROR err=%d\n", ApiRef, GetLastError());
#else
	GetProcessAffinityMask(GetCurrentProcess(), &ProcessAffinityMask, &SystemAffinityMask);
#endif // DXW_NOTRACES
	OutTraceDW("%s: Process affinity=%#x\n", ApiRef, ProcessAffinityMask);
	if(first){
		for (i=0; i<(8 * sizeof(DWORD)); i++){
			if (ProcessAffinityMask & 0x1) break;
			ProcessAffinityMask >>= 1;
		}
		OutTraceDW("%s: First process affinity bit=%d\n", ApiRef, i);
		ProcessAffinityMask = 0x1;
		for (; i; i--) ProcessAffinityMask <<= 1;
		OutTraceDW("%s: Process affinity=%#x\n", ApiRef, ProcessAffinityMask);
	}
	else {
		for (i=0; i<(8 * sizeof(DWORD)); i++){
			if (ProcessAffinityMask & 0x80000000) break;
			ProcessAffinityMask <<= 1;
		}
		i = 31 - i;
		OutTraceDW("%s: Last process affinity bit=%d\n", ApiRef, i);
		ProcessAffinityMask = 0x1;
		for (; i; i--) ProcessAffinityMask <<= 1;
		OutTraceDW("%s: Process affinity=%#x\n", ApiRef, ProcessAffinityMask);
	}

	res = SetProcessAffinityMask(GetCurrentProcess(), ProcessAffinityMask);
	_if(!res) OutTraceE("%s: SetProcessAffinityMask ERROR err=%d\n", ApiRef, GetLastError());
}

#if 0
static BOOL GetTextSegment(char *module, unsigned char **start, DWORD *len)
{
	typedef BOOL (WINAPI *GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
	MODULEINFO mi;
	HMODULE psapilib;	
	GetModuleInformation_Type pGetModuleInformation;
	// getting segment size
	psapilib=(*pLoadLibraryA)("psapi.dll");
	if(!psapilib) {
		OutTraceDW("DXWND: Load lib=\"%s\" failed err=%d\n", "psapi.dll", GetLastError());
		return FALSE;
	}

	pGetModuleInformation=(GetModuleInformation_Type)(*pGetProcAddress)(psapilib, "GetModuleInformation");
	(*pGetModuleInformation)(GetCurrentProcess(), GetModuleHandle(NULL), &mi, sizeof(mi));
	FreeLibrary(psapilib);

	typedef IMAGE_NT_HEADERS *(WINAPI *ImageNtHeader_Type)(PVOID);
	ImageNtHeader_Type pImageNtHeader = NULL;
	HMODULE hDbgLib = LoadLibrary("dbghelp.dll");
	if(!hDbgLib) {
		OutTraceDW("DXWND: Load lib=\"%s\" failed err=%d\n", "dbghelp.dll", GetLastError());
		return FALSE;
	}
	pImageNtHeader = (ImageNtHeader_Type)GetProcAddress(hDbgLib, "ImageNtHeader");
	if (!pImageNtHeader) return FALSE;

	*start = NULL;
	*len = 0;
	IMAGE_NT_HEADERS *pNtHdr = (*pImageNtHeader)(GetModuleHandle(module));
	IMAGE_SECTION_HEADER *pSectionHdr = (IMAGE_SECTION_HEADER *)(pNtHdr+1);
	OutTrace("sections=%d\n", pNtHdr->FileHeader.NumberOfSections);
	for(int i=0; i<pNtHdr->FileHeader.NumberOfSections; i++){
		char *name = (char *)pSectionHdr->Name;
		if ((memcmp(name, ".text", 5) == 0) || (memcmp(name, "CODE", 4) == 0)){
			*start = (unsigned char *)mi.lpBaseOfDll + pSectionHdr->VirtualAddress;
			*len = pSectionHdr->SizeOfRawData;
			break;
		}
	} 
	OutTrace("GetTextSegment: module=%s hmod=%#x base=%#x size=%#x\n", 
		module, pNtHdr, *start, *len);
	FreeLibrary(hDbgLib);
	return (*start != NULL);
}
#else
//v2.04.30: much simpler version ....
static BOOL GetTextSegment(char *module, unsigned char **start, DWORD *len)
{
	ApiName("GetTextSegment");
	HMODULE hmod;
	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_NT_HEADERS pNtHeader;
	DWORD dwCodeBase;
	DWORD dwCodeSize;

	hmod = GetModuleHandle(module);
	if(!hmod) return FALSE;
	pDosHeader = (PIMAGE_DOS_HEADER)hmod;
	pNtHeader = (PIMAGE_NT_HEADERS)((char *)pDosHeader + pDosHeader->e_lfanew);
	dwCodeBase = (DWORD)hmod + pNtHeader->OptionalHeader.BaseOfCode;
	dwCodeSize = pNtHeader->OptionalHeader.SizeOfCode;

	OutTrace("%s: module=%s hmod=%#x base=%#x size=%#x\n", 
		ApiRef, module, hmod, dwCodeBase, dwCodeSize);

	*start = (unsigned char *)dwCodeBase;
	*len = dwCodeSize;
	return TRUE;
}
#endif

static void ReplaceRDTSC()
{
	ApiName("ReplaceRDTSC");
	typedef BOOL (WINAPI *GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
	HMODULE disasmlib;
	unsigned char *opcodes;
	t_disasm da;
	DWORD dwSegSize;
	DWORD oldprot;

	if (!(disasmlib=LoadDisasm())) return;
	(*pPreparedisasm)();

	if(!GetTextSegment(NULL, &opcodes, &dwSegSize)) return;

	unsigned int offset = 0;
	BOOL cont = TRUE;
	OutTraceDW("%s: replace RDTSC starting at addr=%#x size=%#x\n", ApiRef, opcodes, dwSegSize);
	while (cont) {
		int cmdlen = 0;
		__try{
			cmdlen=(*pDisasm)(opcodes+offset,20,offset,&da,0,NULL,NULL);
			//OutTrace("offset=%#x opcode=%#x\n", offset, *(opcodes+offset));
		}
		__except (EXCEPTION_EXECUTE_HANDLER){
			OutTrace("%s: exception at offset=%#x\n", ApiRef, offset);
			cont=FALSE;
		}		
		if (cmdlen==0) break;
		// search for RDTSC opcode 0x0F31
		if((*(opcodes+offset) == 0x0F) && (*(opcodes+offset+1) == 0x31)){
			OutTraceDW("%s: RDTSC opcode found at addr=%#x\n", ApiRef, (opcodes+offset));
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, PAGE_READWRITE, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR target=%#x err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
			*(opcodes+offset) = 0xCC;	// __asm INT3
			*(opcodes+offset+1) = 0x90; // __asm NOP
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, oldprot, &oldprot)){
				OutTrace("%s: VirtualProtect ERROR target=%#x, err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
		}
		// search for RDTSCP opcode 0x0F01F9
		if((*(opcodes+offset) == 0x0F) && (*(opcodes+offset+1) == 0x01) && (*(opcodes+offset+2) == 0xF9)){
			OutTraceDW("%s: RDTSCP opcode found at addr=%#x\n", ApiRef, (opcodes+offset));
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, PAGE_READWRITE, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR target=%#x err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
			*(opcodes+offset) = 0xCC;	// __asm INT3
			*(opcodes+offset+1) = 0x90; // __asm NOP
			*(opcodes+offset+2) = 0x90; // __asm NOP
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, oldprot, &oldprot)){
				OutTrace("%s: VirtualProtect ERROR target=%#x, err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
		}
		offset+=cmdlen; 
		if((offset+0x10) > dwSegSize) break; // skip last 16 bytes, just in case....
	}

	return;
	(*pFinishdisasm)();
	FreeLibrary(disasmlib);
}

void ReplaceCPUID(char *module)
{
	ApiName("ReplaceCPUID");
	typedef BOOL (WINAPI *GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
	HMODULE disasmlib;
	unsigned char *opcodes;
	t_disasm da;
	DWORD dwSegSize;
	DWORD oldprot;

	if (!(disasmlib=LoadDisasm())) return;
	(*pPreparedisasm)();

	if(!GetTextSegment(module, &opcodes, &dwSegSize)) return;

	unsigned int offset = 0;
	BOOL cont = TRUE;
	OutTraceDW("%s: replace CPUID starting at addr=%#x size=%#x\n", ApiRef, opcodes, dwSegSize);
	while (cont) {
		int cmdlen = 0;
		__try{
			cmdlen=(*pDisasm)(opcodes+offset,20,offset,&da,0,NULL,NULL);
			//OutTrace("offset=%#x opcode=%#x\n", offset, *(opcodes+offset));
		}
		__except (EXCEPTION_EXECUTE_HANDLER){
			OutTrace("%s: exception at offset=%#x\n", ApiRef, offset);
			cont=FALSE;
		}		
		if (cmdlen==0) break;
		// search for CPUID opcode 0x0FA2
		if((*(opcodes+offset) == 0x0F) && (*(opcodes+offset+1) == 0xA2)){
			OutTraceDW("%s: CPUID opcode found at addr=%#x\n", ApiRef, (opcodes+offset));
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, PAGE_READWRITE, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR target=%#x err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
			*(opcodes+offset) = 0xCC;	// __asm INT3
			*(opcodes+offset+1) = 0xCC; // __asm INT3 not to be executed, just to tell the difference with RDTSC case
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, oldprot, &oldprot)){
				OutTrace("%s: VirtualProtect ERROR target=%#x, err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
		}
		offset+=cmdlen; 
		if((offset+0x10) > dwSegSize) break; // skip last 16 bytes, just in case....
	}

	return;
	//(*pFinishdisasm)();
	//FreeLibrary(disasmlib);
}

static void PatchEFS()
{
	ApiName("PatchEFS");
	DWORD oldprot;
	OutTraceDW("%s: Patching \"Empire of the Fading Suns\"\n", ApiRef);

	unsigned char *start;
	DWORD len;
	unsigned int hash;
	extern unsigned int HashBuffer(BYTE *, int);
	GetTextSegment(NULL, &start, &len);
	OutTraceDW("%s: Empire of the Fading Suns start=%#x len=%#x\n", ApiRef, start, len);
	hash = HashBuffer(start, len);
	OutTraceDW("%s: Empire of the Fading Suns hash=%#x\n", ApiRef, hash);

	//static LPVOID lpTarget = (LPVOID)0x5A3698; // demo
	static LPVOID lpTarget; // full game
	switch(hash){
		case 0xb88c4fc3: 
			lpTarget = (LPVOID)0x5B91F0; break; // full game unpatched
		case 0xced22a1d: 
			lpTarget = (LPVOID)0x5A3698; break; // FilePlanet demo
		case 0x7df9a450: 
			lpTarget = (LPVOID)0x5A3694; break; // original unpatched exe from CD
		default: 
			OutTrace("%s: unknown EFS.EXE version\n", ApiRef);
			return; break;
	}
			
	if(!VirtualProtect(lpTarget, 4, PAGE_READWRITE, &oldprot)) {
		OutTrace("%s: VirtualProtect ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return; // error condition
	}
	*(LPDWORD)lpTarget = 0x0;	// 0
	if(!VirtualProtect(lpTarget, 4, oldprot, &oldprot)){
		OutTrace("%s: VirtualProtect ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return; // error condition
	}
}

static void ReplacePrivilegedOps()
{
	ApiName("ReplacePrivilegedOps");
	HMODULE disasmlib;
	unsigned char *opcodes;
	t_disasm da;
	DWORD dwSegSize;
	DWORD oldprot;
	static BOOL bDoOnce=FALSE;

	if(bDoOnce) return;
	bDoOnce = TRUE;

	if (!(disasmlib=LoadDisasm())) return;
	(*pPreparedisasm)();

	MODULEINFO ModuleInfo;
	typedef BOOL (WINAPI *GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
	HMODULE psapilib;	
	GetModuleInformation_Type pGetModuleInformation;
	// getting segment size
	psapilib=(*pLoadLibraryA)("psapi.dll");
	if(!psapilib) {
		OutTraceDW("%s: LoadLibrary ERROR lib=\"%s\" err=%d\n", ApiRef, "psapi.dll", GetLastError());
		return;
	}

	pGetModuleInformation=(GetModuleInformation_Type)(*pGetProcAddress)(psapilib, "GetModuleInformation");
	(*pGetModuleInformation)(GetCurrentProcess(), (HMODULE)GetModuleHandle(NULL), &ModuleInfo, sizeof(MODULEINFO));
	FreeLibrary(psapilib);
	OutTraceDW("%s: segment base=%#x len=%#x entry=%#x\n",
		ApiRef,
		ModuleInfo.lpBaseOfDll,
		ModuleInfo.SizeOfImage,
		ModuleInfo.EntryPoint
		);
	// from entry point to end of segment
	//opcodes=(unsigned char *)ModuleInfo.EntryPoint;
	//dwSegSize = ((LPBYTE)ModuleInfo.lpBaseOfDll + ModuleInfo.SizeOfImage) - (LPBYTE)ModuleInfo.EntryPoint;
	// from base address to entry point
	//opcodes=(unsigned char *)ModuleInfo.lpBaseOfDll;
	//dwSegSize = (LPBYTE)ModuleInfo.EntryPoint - (LPBYTE)ModuleInfo.lpBaseOfDll;
	// whole segment
	opcodes=(unsigned char *)ModuleInfo.lpBaseOfDll;
	dwSegSize=ModuleInfo.SizeOfImage;

	unsigned int offset = 0;
	BOOL cont = TRUE;
	OutTraceDW("%s: starting at addr=%#x size=%#x\n", ApiRef, opcodes, dwSegSize);
	while (cont) {
		int cmdlen = 0;
		char *sOpcode;
		BOOL bPriv;
		__try{
			cmdlen=(*pDisasm)(opcodes+offset,20,offset,&da,0,NULL,NULL);
			//OutTrace("offset=%#x opcode=%#x\n", offset, *(opcodes+offset));
		}
		__except (EXCEPTION_EXECUTE_HANDLER){
			OutTrace("%s: exception at offset=%#x\n", ApiRef, offset);
			cont=FALSE;
		}		
		if (cmdlen==0) break;
		// search for following opcodes:
		// 0xEC (IN AL, DX)
		// 0x6D (INS DWORD PTR ES:[EDI],DX)
		// 0x6E (OUTS DX,BYTE PTR DS:[ESI])  
		// 0x0F20xx (MOV xx CRx) // 0x0F20E0 = MOV EAX CR4 in Crime Cities 
		bPriv = FALSE;
		switch(*(opcodes+offset)){
			case 0xE4: sOpcode = "IN AL xx";			bPriv=TRUE; break;
			case 0xE5: sOpcode = "IN EAX xx";			bPriv=TRUE; break;
			case 0xE6: sOpcode = "OUT xx AL";			bPriv=TRUE; break;
			case 0xE7: sOpcode = "OUT xx EAX";			bPriv=TRUE; break;
			case 0xEC: sOpcode = "IN AL DX";			bPriv=TRUE; break; 
			case 0xED: sOpcode = "IN EAX DX";			bPriv=TRUE; break; 
			case 0xEE: sOpcode = "OUT DX AL";			bPriv=TRUE; break;
			case 0xEF: sOpcode = "OUT DX EAX";			bPriv=TRUE; break;
			case 0x6C: sOpcode = "INS EDI:[EDI] DX";	bPriv=TRUE; break; 
			case 0x6D: sOpcode = "INS EDI:[EDI] DX";	bPriv=TRUE; break; 
			case 0x6E: sOpcode = "OUTS DX DS:[ESI]";	bPriv=TRUE; break; 
			case 0x6F: sOpcode = "OUTS DX DS:[ESI]";	bPriv=TRUE; break; 
			// v2.04.30: CLI, STI opcodes found in "Jane's Fighters Anthology"
			case 0xFA: sOpcode = "CLI";					bPriv=TRUE; break; 
			case 0xFB: sOpcode = "STI";					bPriv=TRUE; break; 
			case 0x0F: if((*(opcodes+offset+1) == 0x20) && (cmdlen == 3)){
							sOpcode = "MOV xx CRx";
							bPriv=TRUE; 
					   }
					   break;
		}
		if(bPriv){
			//OutTraceDW("DXWND: %s opcode found at addr=%#x command=%s dump=%s\n", sOpcode, (opcodes+offset), da.result, da.dump);
			OutTraceDW("%s: \"%s\" opcode found at addr=%#x\n", ApiRef, sOpcode, (opcodes+offset));
			if(!VirtualProtect((LPVOID)(opcodes+offset), 8, PAGE_READWRITE, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR target=%#x err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
			*(opcodes+offset) = 0x90;	// __asm NOP
			if((*(opcodes+offset+1) == 0xA8) && 
				((*(opcodes+offset+3) == 0x75) || (*(opcodes+offset+3) == 0x74))) { // both JNZ and JZ
				OutTraceDW("%s: IN loop found at addr=%#x\n", ApiRef, (opcodes+offset));
				memset((opcodes+offset+1), 0x90, 4); // Ubik I/O loop
				offset+=4;
			}
			if(!VirtualProtect((LPVOID)(opcodes+offset), 8, oldprot, &oldprot)){
				OutTrace("%s: VirtualProtect ERROR target=%#x, err=%d @%d\n", 
					ApiRef, opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
		}
		offset+=cmdlen; 
		if((offset+0x10) > dwSegSize) break; // skip last 16 bytes, just in case....
	}

	return;
	//(*pFinishdisasm)();
	//FreeLibrary(disasmlib);
}

#if 0
// from https://www.winehq.org/pipermail/wine-users/2002-April/007910.html 
//
// There is no publicaly available version numbering for SafeDisc. However, it 
// seems that the version number is stored in the executable as 3 unsigned 32-bit 
// integers. Using an hexadecimal editor, locate the following byte pattern in 
// the wrapper (game.exe)
//
// > 426f475f 202a3930 2e302621 21202059   BoG_ *90.0&!!  Y
// > 793e0000                              y>..
//
// There should be 3 unsigned integers right after that, which are respectively 
// the version, subversion an revision number.
//
// On some versions of SafeDisc there are 3 null integers following the pattern, 
// before the version number. You'll then have to look at the 3 unsigned 32-bit 
// integers right after
//
// > 426f475f 202a3930 2e302621 21202059   BoG_ *90.0&!!  Y
// > 793e0000 00000000 00000000 00000000   y>..............

static void CheckSafeDiscVersion()
{
	unsigned char *opcode;
	DWORD dwSegSize;
	static BOOL bDoOnce=FALSE;
	DWORD dwVersion, dwSubversion, dwRevision;

	if(bDoOnce) return;
	bDoOnce = TRUE;

	if(!GetTextSegment(NULL, &opcode, &dwSegSize)) return;

	unsigned int offset = 0;
	BOOL cont = TRUE;
	OutTraceDW("DXWND: CheckSafeDiscVersion starting at addr=%#x size=%#x\n", opcode, dwSegSize);
	for(; dwSegSize > 40;) {
		// fast way to make 20 char comparisons .....
		if(*(DWORD *)opcode     ==0x5F476F42)
		if(*(DWORD *)(opcode+4) ==0x30392A20)
		if(*(DWORD *)(opcode+8) ==0x2126302E)
		if(*(DWORD *)(opcode+12)==0x59202021)
		if(*(DWORD *)(opcode+16)==0x00003E79){
			dwVersion = *(DWORD *)(opcode+20);
			dwSubversion = *(DWORD *)(opcode+24);
			dwRevision = *(DWORD *)(opcode+28);
			if(dwVersion == 0){
				dwVersion = *(DWORD *)(opcode+32);
				dwSubversion = *(DWORD *)(opcode+36);
				dwRevision = *(DWORD *)(opcode+40);	
			}
			OutTrace("Safedisk %d.%d.%d detected\n");
			ShowHint(HINT_SAFEDISC);
			break;
		}
		dwSegSize -= 4;
		opcode += 4;
	}
}
#endif

HWND hDesktopWindow = NULL;

// Message poller: its only purpose is to keep sending messages to the main window
// so that the message loop is kept running. It is a trick necessary to play 
// smack videos with smackw32.dll and AUTOREFRESH mode set
BOOL gb_RefreshSemaphore = FALSE;

DWORD WINAPI MessagePoller(LPVOID lpParameter)
{
	#define DXWREFRESHINTERVAL 20
	while(TRUE){
		Sleep(DXWREFRESHINTERVAL);
		// test gb_RefreshSemaphore to suspend refresh during mci movie play
		if(gb_RefreshSemaphore) continue;
		if(dxw.dwFlags2 & INDEPENDENTREFRESH)
			dxw.ScreenRefresh();
		else
			SendMessage(dxw.GethWnd(), WM_NCHITTEST, 0, 0);
	}
    return 0;
}

DWORD WINAPI WindowRepainter(LPVOID lpParameter)
{
	#define GDIREFRESHINTERVAL 200
	while(TRUE){
		Sleep(GDIREFRESHINTERVAL);
		if(gb_RefreshSemaphore) continue;
		if(dxw.GethWnd()) (*pInvalidateRect)(dxw.GethWnd(), NULL, FALSE);
	}
    return 0;
}

DWORD WINAPI TimeFreezePoller(LPVOID lpParameter)
{
	#define DXWREFRESHINTERVAL 20
	extern UINT VKeyConfig[];
	UINT FreezeToggleKey;
	FreezeToggleKey = VKeyConfig[DXVK_FREEZETIME];
	while(TRUE){
		Sleep(DXWREFRESHINTERVAL);
		if(GetAsyncKeyState(FreezeToggleKey) & 0xF000) dxw.ToggleFreezedTime();
	}
    return 0;
}

static void MemoryReserveB0000()
{
	dwB0000 = (DWORD)VirtualAlloc((LPVOID)0xB0000, 0x10000, MEM_COMMIT, PAGE_READWRITE);
}
	
static void MemoryReserve()
{
	VirtualAlloc((LPVOID)0x4000000, 0x04000000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x5000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x6000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x7000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x8000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
}

static void SignalHookEvent()
{
	ApiName("SignalHookEvent");
	HANDLE hEvent;
	char *eventName = "DxWndInjectCompleted";

	hEvent = CreateEvent(NULL, FALSE, FALSE, eventName);
	if(!hEvent) {
		if(ERROR_ALREADY_EXISTS == GetLastError()){
			hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName);
		} 
		if(!hEvent) {
			OutTraceE("%s: CreateEvent ERROR event=\"%s\" err=%d\n", ApiRef, eventName, GetLastError());
			return;
		}
	}
    if (!SetEvent(hEvent)) {
		DWORD gle = GetLastError();
		CloseHandle(hEvent); // FIX: Early hook synchronization
		OutTraceE("%s: SetEvent ERROR err=%d\n", ApiRef, GetLastError());
		return;
    }
	CloseHandle(hEvent); // FIX: Early hook synchronization
	OutTraceDW("%s: SetEvent OK\n", ApiRef);
}

#ifndef PROCESS_DPI_AWARENESS
	typedef enum _PROCESS_DPI_AWARENESS { 
		PROCESS_DPI_UNAWARE            = 0,
		PROCESS_SYSTEM_DPI_AWARE       = 1,
		PROCESS_PER_MONITOR_DPI_AWARE  = 2
	} PROCESS_DPI_AWARENESS;
#endif

static void SetDPIAware()
{
	// Kind courtesy of FunlyFr3sh & Toni Spets from C&C wrapper
	// https://github.com/CnCNet/cnc-ddraw/blob/308413323cad6f3aad1b4e40eaacd72d95faaa7d/src/main.c#L61
	// n.b. this routine is only called once and before the system call hooking, so it is possible 
	// to call the system call names directly.

	BOOL setDpiAware = FALSE;

	HMODULE hShcore = GetModuleHandle("shcore.dll");
	typedef HRESULT (WINAPI *SetProcessDpiAwareness_)(PROCESS_DPI_AWARENESS);
	if(hShcore){
		SetProcessDpiAwareness_ setProcessDpiAwareness = (SetProcessDpiAwareness_)GetProcAddress(hShcore, "SetProcessDpiAwareness");
		if (setProcessDpiAwareness){
			HRESULT result = setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
			setDpiAware = (result == S_OK || result == E_ACCESSDENIED);
		}   
	}
	if (!setDpiAware){
		HMODULE hUser32 = GetModuleHandle("user32.dll");
		typedef BOOL (WINAPI *SetProcessDPIAware_)();
		if(hUser32){
			SetProcessDPIAware_ setProcessDPIAware = (SetProcessDPIAware_)GetProcAddress(hUser32, "SetProcessDPIAware");
			if (setProcessDPIAware) 
				setProcessDPIAware();
		}
	}
}

static void NoAccessibilityShortcutKeys()
{
	STICKYKEYS g_StartupStickyKeys = {sizeof(STICKYKEYS), 0};
	TOGGLEKEYS g_StartupToggleKeys = {sizeof(TOGGLEKEYS), 0};
	FILTERKEYS g_StartupFilterKeys = {sizeof(FILTERKEYS), 0};    
    SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
    SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
    SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);

	// Disable StickyKeys/etc shortcuts but if the accessibility feature is on, 
	// then leave the settings alone as its probably being usefully used

	STICKYKEYS skOff = g_StartupStickyKeys;
    if( (skOff.dwFlags & SKF_STICKYKEYSON) == 0 ){
        // Disable the hotkey and the confirmation
        skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
        skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;
        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
    }

    TOGGLEKEYS tkOff = g_StartupToggleKeys;
    if( (tkOff.dwFlags & TKF_TOGGLEKEYSON) == 0 ){
        // Disable the hotkey and the confirmation
        tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
        tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;

        SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);
    }

    FILTERKEYS fkOff = g_StartupFilterKeys;
    if( (fkOff.dwFlags & FKF_FILTERKEYSON) == 0 ){
        // Disable the hotkey and the confirmation
        fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
        fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;

        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
    }
}

static void HookFrontend()
{
	ApiName("HookFrontend");
	HMODULE base;
	//MessageBox(NULL, "FRONTEND", "DxWnd", 0);
	SetDllDirectory(GetDxWndPath());
	InitModuleHooks();
	// initialize function pointers used by DxWnd itself
	HookKernel32Init();
	base=GetModuleHandle(NULL);
	IATPatch = IATPatchDefault;
	HookLauncher(base);
	if(IsTraceR) HookAdvApi32(base);
	OutTrace("%s: Launcher mode OK\n", ApiRef); 
}

static void DisableDEP()
{
	ApiName("DisableDEP");
	HMODULE hK = GetModuleHandle("KERNEL32.DLL");
	BOOL (WINAPI *pSetProcessDEPPolicy)(DWORD);
	*(FARPROC *) &pSetProcessDEPPolicy = GetProcAddress(hK, "SetProcessDEPPolicy");
	if (pSetProcessDEPPolicy) {
		BOOL ret = (*pSetProcessDEPPolicy)(0);
		OutTrace("%s: SetProcessDEPPolicy ret=%#x\n", ApiRef, ret); 
	}
	else {
		OutTrace("%s: no SetProcessDEPPolicy call\n", ApiRef); 
	}
}

static char *WindowsName(OSVERSIONINFOEXW i)
{
	char *s = "";
	switch(i.dwMajorVersion){
		case 5:
			switch(i.dwMinorVersion){
				case 0:
					s = "Windows 2000";
					break;
				case 1:
					s = "Windows XP";
					break;
				case 2:
					//if(i.wProductType == VER_NT_WORKSTATION) s = "Windows XP Professional x64 Edition";
					//else if (i.wSuiteMask == VER_SUITE_WH_SERVER) s = "Windows Home Server";
					//else s = "Windows Server 2003";
					s = "Windows Server 2003";
					break;
			}
			break;
		case 6:
			switch(i.dwMinorVersion){
				case 0:
					if(i.wProductType == VER_NT_WORKSTATION) s = "Windows Vista";
					else s = "Windows Server 2008";
					break;
				case 1:
					if(i.wProductType == VER_NT_WORKSTATION) s = "Windows 7";
					else s = "Windows Server 2008 R2";
					break;
				case 2:
					if(i.wProductType == VER_NT_WORKSTATION) s = "Windows 8";
					else s = "Windows Server 2012";
					break;
				case 3:
					if(i.wProductType == VER_NT_WORKSTATION) s = "Windows 8.1";
					else s = "Windows Server 2012 R2";
					break;

			}
			break;
		case 10:
			if(i.wProductType == VER_NT_WORKSTATION) {
				if(i.dwBuildNumber < 22000) s = "Windows 10";
				else s = "Windows 11";
			}
			else {
				s = "Windows Server 2019/2022";
			};
			break;			
	}
	return s;
}

static void PrintExactOSVersion()
{
	ApiName("PrintExactOSVersion");
#ifdef OSVERSIONUSENETAPI
	typedef DWORD (WINAPI *NetApiBufferFree_Type)(LPVOID);
	typedef DWORD (WINAPI *NetWkstaGetInfo_Type)(LMSTR, DWORD, LPBYTE *);
    HMODULE lib = LoadLibraryA("netapi32.dll");
	if(!lib) return;
    LPWKSTA_INFO_100 pBuf = NULL;
    NET_API_STATUS nStatus;
	NetApiBufferFree_Type pNetApiBufferFree = (NetApiBufferFree_Type)GetProcAddress(lib, "NetApiBufferFree");
	NetWkstaGetInfo_Type pNetWkstaGetInfo = (NetWkstaGetInfo_Type)GetProcAddress(lib, "NetWkstaGetInfo");
	if((pNetApiBufferFree == NULL) || (pNetWkstaGetInfo == NULL)){
		FreeLibrary(lib);
		return;
	}

    // Call the NetWkstaGetInfo function, specifying level 100.
    nStatus = (*pNetWkstaGetInfo)(NULL, 100, (LPBYTE *)&pBuf);

    // If the call is successful, print the workstation data.
    if(nStatus == NERR_Success)
		OutTrace("net OS=(%d.%d)\n", pBuf->wki100_ver_major, pBuf->wki100_ver_minor);

    // Free the allocated memory.
    if(pBuf != NULL) (*pNetApiBufferFree)(pBuf);
	FreeLibrary(lib);
#else
	HMODULE lib = LoadLibraryA("ntdll.dll");
	if(!lib) {
		OutTrace("%s: LoadLibrary(NtosKrnl.exe) ERROR err=%d\n", ApiRef, GetLastError());
		return;
	}
	typedef NTSTATUS (WINAPI *RtlGetVersion_Type)(PRTL_OSVERSIONINFOW);
	RtlGetVersion_Type pRtlGetVersion = (RtlGetVersion_Type)GetProcAddress(lib, "RtlGetVersion");
	if(pRtlGetVersion == NULL){
		OutTrace("%s: GetProcAddress(RtlGetVersion) ERROR err=%d\n", ApiRef, GetLastError());
		FreeLibrary(lib);
		return;
	}
	OSVERSIONINFOEXW VersionInformationEx;
	memset(&VersionInformationEx, 0, sizeof(OSVERSIONINFOEXW));
	VersionInformationEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	if((*pRtlGetVersion)((PRTL_OSVERSIONINFOW)&VersionInformationEx) == 0){
		OutTrace("%s: rtl OS=(%d.%d) build=%d platform=%d sp=(%d.%d) version=\"%ls\" suitemask=%#x name=\"%s\"\n", 
			ApiRef,
			VersionInformationEx.dwMajorVersion, VersionInformationEx.dwMinorVersion,
			VersionInformationEx.dwBuildNumber,
			VersionInformationEx.dwPlatformId,
			VersionInformationEx.wServicePackMajor, VersionInformationEx.wServicePackMinor, 
			VersionInformationEx.szCSDVersion,
			VersionInformationEx.wSuiteMask,
			WindowsName(VersionInformationEx));
	}
	else{
		memset(&VersionInformationEx, 0, sizeof(OSVERSIONINFOEXW));
		VersionInformationEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
		if((*pRtlGetVersion)((PRTL_OSVERSIONINFOW)&VersionInformationEx) == 0){
			OutTrace("%s: rtl OS=(%d.%d) build=%d platform=%d version=\"%ls\"\n", 
				ApiRef,
				VersionInformationEx.dwMajorVersion, VersionInformationEx.dwMinorVersion,
				VersionInformationEx.dwBuildNumber,
				VersionInformationEx.dwPlatformId,
				VersionInformationEx.szCSDVersion);
		}	
	}
	FreeLibrary(lib);
#endif
}

extern HHOOK hMouseHook;
extern HHOOK hKeyboardHook;
extern void HackLocale();

void InitScreenParameters()
{
	ApiName("InitScreenParameters");
	extern void FixPixelFormat(int , DDPIXELFORMAT *);
	extern void SetBltTransformations();
	DEVMODE CurrDevMode;
	static int DoOnce = FALSE;
	DWORD dwVJoyStatus;

	if(DoOnce) return; // useless? It's called only from HookInit()
	DoOnce = TRUE;

	// v2.04.78: set screen initial resolution
	// v2.06.09: fixed for unwindowed and color emulation
	if(dxw.isScaled){
		if(dxw.dwFlags7 & INITIALRES) 
			dxw.SetScreenSize(dxw.iMaxW, dxw.iMaxH); 
		else 
			dxw.SetScreenSize(); // 800 x 600 by default
	}
	else {
		dxw.SetScreenSize(InitDevMode.dmPelsWidth, InitDevMode.dmPelsHeight);
	}

	if(!(*pEnumDisplaySettingsA)(NULL, ENUM_CURRENT_SETTINGS, &CurrDevMode)){
		OutErrorGDI("%s: EnumDisplaySettings ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return;
	}
	memset(&dxw.ActualPixelFormat, 0, sizeof(DDPIXELFORMAT));
	// initialize to default null values, but dwRGBBitCount
	dxw.ActualPixelFormat.dwRGBBitCount=CurrDevMode.dmBitsPerPel;
	dxw.VirtualPixelFormat.dwRGBBitCount=CurrDevMode.dmBitsPerPel; // until set differently
	if(dxw.dwFlags2 & INIT8BPP) FixPixelFormat(8, &dxw.VirtualPixelFormat);
	if(dxw.dwFlags2 & INIT16BPP) FixPixelFormat(16, &dxw.VirtualPixelFormat);
	if(dxw.dwFlags7 & INIT24BPP) FixPixelFormat(24, &dxw.VirtualPixelFormat);
	if(dxw.dwFlags7 & INIT32BPP) FixPixelFormat(32, &dxw.VirtualPixelFormat);

	GetHookInfo()->Height=(short)dxw.GetScreenHeight();
	GetHookInfo()->Width=(short)dxw.GetScreenWidth();
	GetHookInfo()->ColorDepth=0; // unknown
	GetHookInfo()->DXVersion=0; // unknown
	GetHookInfo()->isLogging=(dxw.dwTFlags & OUTTRACE);
	
	dwVJoyStatus = GetHookInfo()->VJoyStatus;
	dwVJoyStatus &= ~VJOYPRESENT;
	if(dxw.dwFlags6 & VIRTUALJOYSTICK) dwVJoyStatus |= VJOYPRESENT;
	GetHookInfo()->VJoyStatus = dwVJoyStatus;

	SetBltTransformations();
	OutTraceDW("%s: size(WxH)=(%dx%d) RGBBitCount=%d\n", 
		ApiRef, dxw.GetScreenWidth(), dxw.GetScreenHeight(), CurrDevMode.dmBitsPerPel);
	return;
}

void ReHook()
{
	ClearModuleHooks();
	SetModuleHooks();
}

void HookInit(TARGETMAP *target, HWND hwnd)
{
	ApiName("HookInit");
	extern HINSTANCE hInst;
	typedef HHOOK (WINAPI *SetWindowsHookEx_Type)(int, HOOKPROC, HINSTANCE, DWORD);
	extern SetWindowsHookEx_Type pSetWindowsHookExA;
	static BOOL DoOnce = TRUE;
	HMODULE base;
	char *sModule;
	char sModuleBuf[60+1];
	static char *GOGWinMM = "win32";
	static char *dxversions[14]={
		"Automatic", "DirectX1~6", "", "", "", "", "", 
		"DirectX7", "DirectX8", "DirectX9", "DirectX10", "DirectX11", "None", ""
	};

	if(DoOnce) { // don't do that twice!
		dxw.InitTarget(target);

		if(dxw.dwFlags6 & FRONTEND){
			HookFrontend();
			return;
		}

#ifdef GHOSTTEXTURE
		extern void InitGhostGUID(void);
		InitGhostGUID();
#endif 

		// reserve legacy memory segments
		// no logs here also to avoid other interfering memory allocations
		if(dxw.dwFlags16 & MAPMEMB0000) MemoryReserveB0000();
		if(dxw.dwFlags6 & LEGACYALLOC) MemoryReserve();
		if(dxw.dwFlags1 & SETDPIAWARE) SetDPIAware();
		if(dxw.dwFlags5 & NOACCESSIBILITY) NoAccessibilityShortcutKeys();
		if(dxw.dwFlags16 & RECOVERSYSCURSORICONS) SaveSystemCursors();
		if(dxw.dwFlags20 & DISABLEGHOSTING) DisableProcessWindowsGhosting();

		// add the DxWnd install dir to the search path, to make all included dll linkable
		//SetDllDirectory(GetDxWndPath());
		// v2.05.70 fix: replaced SetDllDirectory with update to environment variable.
		// SetDllDirectory changes the dll search path and other details 
		// see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setdlldirectorya
		// and this interferes with some program (found in "Max Payne 2")
		#define MAX_ENVPATH 32767 // from MSDN
		char *sPath = (char *)malloc(MAX_ENVPATH);
		GetEnvironmentVariableA("Path", sPath, MAX_ENVPATH);
		if(sPath[strlen(sPath)-1]!=';') strncat(sPath, ";", MAX_ENVPATH);
		strncat(sPath, GetDxWndPath(), MAX_ENVPATH);
		SetEnvironmentVariableA("Path", sPath);
		free(sPath);

		if(dxw.dwFlags16 & EMULATEWIN9XHEAP){
			HMODULE heaplib = LoadLibraryA("9xheap.dll");
			if(!heaplib) {
				dxw.dwFlags16 &= ~EMULATEWIN9XHEAP;
				OutTrace("%s: 9xheap not loaded err=%d\n", ApiRef, GetLastError());
				MessageBox(0, "9xheap.dll missing", "DxWnd", 0);
			}
			else OutTrace("%s: 9xheap loaded\n", ApiRef);
		}

		OutTraceSYS("%s: CommandLine=\"%s\"\n", ApiRef, GetCommandLineA());
		if(dxw.dwFlags16 & MAPMEMB0000) OutTrace("%s: Memory 0xB0000=%#x\n", ApiRef, dwB0000);

		// v2.05.87 fix: showing a pop-up dialog during a window creation may recurse DefWindowProcA wrapper
		// and violate the stack area. Found when enabling hints in "3D Scooter Racing (NO3D)".
		//if(dxw.bHintActive) ShowHint(HINT_HINT);
		if(dxw.bHintActive && hwnd) ShowHint(HINT_HINT);

		InitModuleHooks();
		// initialize function pointers used by DxWnd itself
		HookKernel32Init();
		HookUser32Init();
		HookGDI32Init();
		HookWinMMInit();

		dxw.VirtualDesktop.left		= GetSystemMetrics(SM_XVIRTUALSCREEN);
		dxw.VirtualDesktop.top		= GetSystemMetrics(SM_YVIRTUALSCREEN);
		dxw.VirtualDesktop.right	= dxw.VirtualDesktop.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		dxw.VirtualDesktop.bottom	= dxw.VirtualDesktop.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
		OutTraceDW("%s: Virtual Desktop: monitors=%d area=(%d,%d)-(%d,%d)\n", 
			ApiRef, GetSystemMetrics(SM_CMONITORS),
			dxw.VirtualDesktop.left, dxw.VirtualDesktop.top, dxw.VirtualDesktop.right, dxw.VirtualDesktop.bottom);

#ifndef DXW_NOTRACES
		if(IsDebugDW){
			if(dxw.dwFlags4 & (SUPPORTSVGA|SUPPORTHDTV)){
				for(int i=0; SupportedRes[i].w; i++) 
					OutTrace("%s: Virtual mode[%d] = %d x %d\n", ApiRef, i, SupportedRes[i].w, SupportedRes[i].h);
			}
		}
#endif
		dxw.InitPos(target);
#ifndef DXW_NOTRACES
		if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) dxw.SaveCurrentDirectory();
		if(dxw.dwFlags10 & FAKEHDDRIVE) OutTraceDW("%s: Virtual HD drive %c:\n", ApiRef, dxw.FakeHDDrive);
		if(dxw.dwFlags10 & FAKECDDRIVE) OutTraceDW("%s: Virtual CD drive %c:\n", ApiRef, dxw.FakeCDDrive);
		if(dxw.CDADrive != ' ')			OutTraceDW("%s: Forced CDA drive %c:\n", ApiRef, dxw.CDADrive);
#endif // DXW_NOTRACES
		if(dxw.bHintActive) {
			CheckCompatibilityFlags();	// v2.02.83: Check for change of OS release
			// CheckSafeDiscVersion();		// v2.03.78: Detects SafeDisk references and version - moved to DxWnd.exe 
		}
		hTrayWnd = FindWindow("Shell_TrayWnd", NULL);
		 //moved here to load virtual keys definition also on proxy mode
		if(dxw.dwFlags4 & ENABLEHOTKEYS) dxw.MapKeysInit(); 
		if(dxw.dwFlags11 & CUSTOMLOCALE) HackLocale();

		DXWNDSTATUS *p = GetHookInfo();
		p->CursorX = p->CursorY = (short)-1;
		p->WinProcX = p->WinProcY = (short)-1;
		p->MsgHookX = p->MsgHookY = (short)-1;
		p->MessageX = p->MessageY = (short)-1;
		p->DInputX = p->DInputY = (short)-1;
		p->HDLabel = (dxw.dwFlags10 & FAKEHDDRIVE) ? dxw.FakeHDDrive : ' ';
		p->CDLabel = (dxw.dwFlags10 & FAKECDDRIVE) ? dxw.FakeCDDrive : ' ';
		if(dxw.dwFlags18 & RESETMULTIVOLUME) p->CDIndex = 0;
	}

	if(hwnd){
		// v2.04.43: do not elect to main window any window smaller than fullscreen.
		RECT rect;
		if(!pGetClientRect) pGetClientRect=GetClientRect;
		if((*pGetClientRect)(hwnd, &rect)){
			OutTraceDW("%s: window size check (%d,%d)\n", ApiRef, rect.right, rect.bottom);
			if ((rect.right < dxw.iSiz0X) || (rect.bottom < dxw.iSiz0Y)) hwnd = 0;
		}
	}

	if(hwnd){ // v2.02.32: skip this when in code injection mode.
		// v2.1.75: is it correct to set hWnd here?
		//dxw.SethWnd(hwnd);
		dxw.hParentWnd=GetParent(hwnd);
		dxw.hChildWnd=hwnd;
		// v2.02.31: set main win either this one or the parent!
		dxw.SethWnd((dxw.dwFlags1 & FIXPARENTWIN) ? GetParent(hwnd) : hwnd);
		//if(dxw.dwFlags4 & ENABLEHOTKEYS) dxw.MapKeysInit();
	}

	if(IsTraceDW){
		char sInfo[1024];
		OSVERSIONINFO osinfo;
		strcpy(sInfo, "");
		if(hwnd) sprintf(sInfo, " hWnd=%#x ParentWnd=%#x desktop=%#x", hwnd, dxw.hParentWnd, GetDesktopWindow());
		OutTrace(
			"%s: id=%d path=\"%s\" module=\"%s\" dxversion=%s pos=(%d,%d) size=(%d,%d) init-max=(%d,%d) "
			"monitor=%d%s renderer=%d(\"%s\") filter=\"%s\" scale=(%dx%d) timestretch=%d\n", 
			ApiRef, target->index,
			target->path, target->module, dxversions[dxw.dwTargetDDVersion], 
			//target->posx, target->posy, target->sizx, target->sizy, 
			dxw.iPos0X, dxw.iPos0Y, dxw.iSiz0X, dxw.iSiz0Y,
			target->resw, target->resh, 
			target->monitorid, sInfo,
			dxw.Renderer->id, dxw.Renderer->name,
			dxwFilters[target->FilterId].name,
			dxwFilters[target->FilterId].xfactor,
			dxwFilters[target->FilterId].yfactor,
			target->InitTS
			);
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(GetVersionEx(&osinfo)){
			OutTrace("%s: OS=(%d.%d) build=%d platform=%d service pack=%s\n", 
				ApiRef,
				osinfo.dwMajorVersion, 
				osinfo.dwMinorVersion, 
				osinfo.dwPlatformId, 
				osinfo.dwPlatformId, 
				osinfo.szCSDVersion);
		}
		PrintExactOSVersion();
#ifndef DXW_NOTRACES
		if (dxw.dwFlags7 & MAXIMUMRES) OutTrace("%s: max resolution=(%dx%d)\n", ApiRef, dxw.iMaxW, dxw.iMaxH);
		if (dxw.dwFlags7 & LIMITDDRAW) OutTrace("%s: max supported IDidrectDrawInterface=%d\n", ApiRef, dxw.MaxDdrawInterface);
		if (dxw.dwFlags7 & CPUSLOWDOWN) OutTrace("%s: CPU slowdown ratio 1:%d\n", ApiRef, dxw.SlowRatio);
		if (dxw.dwFlags7 & CPUMAXUSAGE) OutTrace("%s: CPU maxusage ratio 1:%d\n", ApiRef, dxw.SlowRatio);
		if (dxw.dwFlags8 & VSYNCSCANLINE) OutTrace("%s: VSync Scanline=%d\n", ApiRef, dxw.ScanLine);
#endif // DXW_NOTRACES
	}

	if(dxw.dwFlags6 & HIDETASKBAR) gShowHideTaskBar(TRUE);

	if (hwnd && IsDebugDW){
		DWORD dwStyle, dwExStyle;
		char ClassName[81];
		char WinText[81];
		char sStyle[256];
		char sExStyle[256];
		dwStyle=GetWindowLong(dxw.GethWnd(), GWL_STYLE);
		dwExStyle=GetWindowLong(dxw.GethWnd(), GWL_EXSTYLE);
		GetClassName(dxw.GethWnd(), ClassName, sizeof(ClassName));
		GetWindowText(dxw.GethWnd(), WinText, sizeof(WinText));
		OutTrace("%s: dxw.hChildWnd=%#x class=\"%s\" text=\"%s\" style=%#x(%s) exstyle=%#x(%s)\n", 
			ApiRef, dxw.hChildWnd, ClassName, WinText, 
			dwStyle, ExplainStyle(dwStyle, sStyle, 256), 
			dwExStyle, ExplainExStyle(dwExStyle, sExStyle, 256));
		dwStyle=GetWindowLong(dxw.hParentWnd, GWL_STYLE);
		dwExStyle=GetWindowLong(dxw.hParentWnd, GWL_EXSTYLE);
		GetClassName(dxw.hParentWnd, ClassName, sizeof(ClassName));
		GetWindowText(dxw.hParentWnd, WinText, sizeof(WinText));
		OutTrace("%s: dxw.hParentWnd=%#x class=\"%s\" text=\"%s\" style=%#x(%s) exstyle=%#x(%s)\n", 
			ApiRef, dxw.hParentWnd, ClassName, WinText, 
			dwStyle, ExplainStyle(dwStyle, sStyle, 256), 
			dwExStyle, ExplainExStyle(dwExStyle, sExStyle, 256));
		OutTrace("%s: target window pos=(%d,%d) size=(%d,%d)\n", 
			ApiRef, dxw.iPosX, dxw.iPosY, dxw.iSizX, dxw.iSizY);
		dxw.DumpDesktopStatus();
		typedef HRESULT (WINAPI *DwmIsCompositionEnabled_Type)(BOOL *);
		DwmIsCompositionEnabled_Type pDwmIsCompositionEnabled = NULL;
		HMODULE DwnApiHdl;
		DwnApiHdl = LoadLibrary("Dwmapi.dll");
		if (DwnApiHdl) pDwmIsCompositionEnabled = (DwmIsCompositionEnabled_Type)GetProcAddress(DwnApiHdl, "DwmIsCompositionEnabled");
		char *sRes;
		if(pDwmIsCompositionEnabled){
			HRESULT res;
			BOOL val;
			res = (*pDwmIsCompositionEnabled)(&val);
			if(res==S_OK) sRes = val ? "ENABLED" : "DISABLED";
			else sRes = "ERROR";
		}
		else sRes = "Unknown";
		OutTrace("%s: DWMComposition %s\n", ApiRef, sRes);
	}

	if (hwnd) {
		char ClassName[8+1];
		GetClassName(hwnd, ClassName, sizeof(ClassName));
		if(!strcmp(ClassName, "IME")){
			dxw.hChildWnd=GetParent(hwnd);
			dxw.hParentWnd=GetParent(dxw.hChildWnd);
			if (dxw.dwFlags2 & SUPPRESSIME) DestroyWindow(hwnd);
			// v2.02.31: set main win either this one or the parent!
			dxw.SethWnd((dxw.dwFlags1 & FIXPARENTWIN) ? dxw.hParentWnd : dxw.hChildWnd);
			hwnd = dxw.GethWnd();
#ifndef DXW_NOTRACES
			if(hwnd) OutTraceDW("%s: skipped IME window. current hWnd=%#x(hdc=%#x) dxw.hParentWnd=%#x(hdc=%#x)\n", 
				ApiRef, hwnd, GetDC(hwnd), dxw.hParentWnd, GetDC(dxw.hParentWnd));		
#endif // DXW_NOTRACES
		}
	}

	// enable optional modules
	if((dxw.dwFlags4 & HOOKGLIDE) || (dxw.dwFlags12 & SUPPRESSGLIDE)) {
		dxw.SetDLLFlags(SYSLIBIDX_GLIDE, DXWHOOK_HOOK);
		dxw.SetDLLFlags(SYSLIBIDX_GLIDE2, DXWHOOK_HOOK);
		dxw.SetDLLFlags(SYSLIBIDX_GLIDE3, DXWHOOK_HOOK);
	}
	if(dxw.dwFlags7 & HOOKSMACKW32) dxw.SetDLLFlags(SYSLIBIDX_SMACKW32, DXWHOOK_HOOK|DXWHOOK_EXTEND); // SMACKW32.DLL	
	if(dxw.dwFlags7 & HOOKSMACKW32) dxw.SetDLLFlags(SYSLIBIDX_SMKWAI32, DXWHOOK_HOOK|DXWHOOK_EXTEND); // v2.05.05: SMKWAI32.DLL	
	if(dxw.dwFlags5 & HOOKBINKW32) 	dxw.SetDLLFlags(SYSLIBIDX_BINKW32, DXWHOOK_HOOK|DXWHOOK_EXTEND); // BINKW32.DLL	
	if(dxw.dwFlags5 & HOOKBINKW32) 	dxw.SetDLLFlags(SYSLIBIDX_BINK2W32, DXWHOOK_HOOK|DXWHOOK_EXTEND); // BINK2W32.DLL	
	if(dxw.dwFlags8 & HOOKWING32) dxw.SetDLLFlags(SYSLIBIDX_WING32, DXWHOOK_HOOK);	// WING32.DLL	
	if(dxw.dwFlags9 & HOOKSDLLIB) dxw.SetDLLFlags(SYSLIBIDX_SDL, (dxw.dwFlags11 & EXTENDSDLHOOK) ? DXWHOOK_HOOK|DXWHOOK_EXTEND : DXWHOOK_HOOK);	// SDL.DLL	
	if(dxw.dwFlags9 & HOOKSDL2LIB) dxw.SetDLLFlags(SYSLIBIDX_SDL2, (dxw.dwFlags11 & EXTENDSDLHOOK) ? DXWHOOK_HOOK|DXWHOOK_EXTEND : DXWHOOK_HOOK); // SDL2.DLL
	if(dxw.dwFlags7 & HOOKDIRECTSOUND) dxw.SetDLLFlags(SYSLIBIDX_DSOUND, DXWHOOK_HOOK); // DSOUND.DLL
	if(dxw.dwFlags6 & HOOKGOGLIBS) SysLibsTable[SYSLIBIDX_WINMM].name = GOGWinMM; // SYSLIBIDX_WINMM
	if(dxw.dwFlags11& SAFEALLOCS) dxw.SetDLLFlags(SYSLIBIDX_MSVCRT, DXWHOOK_HOOK); // MSVCRT.DLL	
	if(dxw.dwFlags10 & (FAKECDDRIVE | FAKEHDDRIVE)) dxw.SetDLLFlags(SYSLIBIDX_MSVCRT, DXWHOOK_HOOK); // MSVCRT.DLL	

	if(dxw.dwFlags11& CUSTOMLOCALE) dxw.SetDLLFlags(SYSLIBIDX_NTDLL, DXWHOOK_HOOK); //NTDLL.DLL	

	base=GetModuleHandle(NULL);
	// set IAT navigators
	IATPatch = IATPatchDefault;
#ifndef DXW_NOTRACES
	DumpImportTable = DumpImportTableDefault;
#endif // DXW_NOTRACES
	if(dxw.dwFlags10 & PEFILEHOOK){
		OutTraceDW("HookInit: setting PE browsing navigation\n"); 
#ifndef DXW_NOTRACES
		DumpImportTable = DumpImportTableByFT;
#endif // DXW_NOTRACES
		IATPatch = IATPatchByFT;
	}
	if(!(dxw.dwFlags10 & PEFILEHOOK) && IsIATSequential(base)){
		OutTraceDW("%s: setting sequential IAT navigation\n", ApiRef); 
#ifndef DXW_NOTRACES
		DumpImportTable = DumpImportTableSequential;
#endif // DXW_NOTRACES
		IATPatch = IATPatchSequential;
		if(dxw.dwFlags3 & SKIPIATHINT){
#ifndef DXW_NOTRACES
			DumpImportTable = DumpImportTableSeqHint;
#endif // DXW_NOTRACES
			IATPatch = IATPatchSeqHint;
		}
	}

	if (dxw.dwFlags3 & SINGLEPROCAFFINITY) SetSingleProcessAffinity(TRUE);
	if (dxw.dwFlags5 & USELASTCORE) SetSingleProcessAffinity(FALSE);
	if (DoOnce){
		if(dxw.dwFlags13 & DISABLEDEP) DisableDEP();
		// Warning: the two exception handlers must be activated in this order or otherwise
		// the INT3 exception used to make fake instruction behaviour will be swallowed by 
		// the suppressor of bad instructions
		if (dxw.dwFlags1 & HANDLEEXCEPTIONS)
			AddVectoredExceptionHandler(1, DxWExceptionHandler);
		if ((dxw.dwFlags4 & INTERCEPTRDTSC) || 
			(dxw.dwFlags5 & DISABLEMMX) || 
			(dxw.dwFlags17 & FAKEPENTIUM3) || 
			(dxw.dwDFlags & CPUALLFLAGSMASK)) 
			AddVectoredExceptionHandler(1, Int3Handler); // 1 = first call, 0 = call last
		if (dxw.dwFlags4 & INTERCEPTRDTSC) ReplaceRDTSC();
		if (dxw.dwFlags5 & REPLACEPRIVOPS) ReplacePrivilegedOps();
		if ((dxw.dwFlags5 & DISABLEMMX) || 
			(dxw.dwFlags17 & FAKEPENTIUM3) || 
			(dxw.dwDFlags & CPUALLFLAGSMASK)) 
			ReplaceCPUID(NULL);
		// mixer
		if(dxw.dwFlags13 & EMULATECDMIXER) dxw.InitializeMixer();
		// visual basic
		if(dxw.dwFlags13 & PATCHMSVBVM) PatchMSVBVM60();
	}
#ifndef DXW_NOTRACES
	if (dxw.dwTFlags2 & OUTIMPORTTABLE) DumpImportTable(base);
#endif // DXW_NOTRACES
	if (dxw.dwFlags2 & SUPPRESSIME) DisableIME();
	if (dxw.dwFlags9 & FIXAILSOUNDLOCKS) HookWAIL32(NULL);

	if(dxw.bHintActive) CheckImportTable(base);

	// make InitPosition used for both DInput and DDraw
	if(dxw.isScaled) dxw.InitWindowPos(target->posx, target->posy, target->sizx, target->sizy);
	
	OutDebugH("%s: base hmodule=%#x\n", ApiRef, base);
	SetModuleHooks();

	HookDlls(base);

#if 1
	if(dxw.gsModules){
		strncpy(sModuleBuf, dxw.gsModules, 60);
		sModule=strtok(sModuleBuf," ;");
		while (sModule) {
			base=(*pLoadLibraryA)(sModule);
			if(!base){
				OutTraceE("%s: LoadLibrary ERROR module=%s err=%d\n", ApiRef, sModule, GetLastError());
			}
			else {
				OutTraceDW("%s: hooking additional module=%s base=%#x\n", ApiRef, sModule, base);
	#ifndef DXW_NOTRACES
				if (dxw.dwTFlags2 & OUTIMPORTTABLE) DumpImportTable(base);
	#endif // DXW_NOTRACES
				HookDlls(base);
			}
			sModule=strtok(NULL," ;");
		}
	}
#endif
	IATPatch(NULL, 0, NULL, NULL, NULL, NULL); // free resources

	SaveScreenMode();
	if(dxw.dwFlags3 & FORCE16BPP) SwitchTo16BPP();

	// v2.05.39: This is really tricky. In theory, the SetWindowsHookEx operation should be done only once,
	// so you should protect the block below with the DoOnce semaphore. But there's a caveat!
	// Obfuscated executables that uncompress their own code somehow invalidate the existing hooks, so that
	// the operation should be repeated. But you can't do that twice if the hook is still valid.
	// The only way to fix the problem is to tentativelu unhook the previous hook handle. If the operation
	// succeeds, the hook was still valid and we're doing a unnecessary operation, but we recover.
	// Instead, if the UnhookWindowsHookEx fails, this is the proof that the operation had to be done again!!
	// this fixes (finally!) the obfuscated "TieuNgaoGiangHo.exe".
	if (dxw.dwFlags1 & MESSAGEPROC){
		BOOL hRet;
		if(hMouseHook) hRet=(*pUnhookWindowsHookEx)(hMouseHook);
#ifndef DXW_NOTRACES
		if(!hRet) OutTraceE("%s: UnhookWindowsHookEx WH_GETMESSAGE failed: hhook=%#x error=%d\n", 
			ApiRef, hMouseHook, GetLastError());
#endif // DXW_NOTRACES
		hMouseHook =(*pSetWindowsHookExA)(WH_GETMESSAGE, MessageHook, hInst, GetCurrentThreadId());
#ifndef DXW_NOTRACES
		if(hMouseHook == NULL) OutTraceE("%s: SetWindowsHookEx WH_GETMESSAGE failed: error=%d\n", 
			ApiRef, GetLastError());
		else OutTraceDW("%s: SetWindowsHookEx WH_GETMESSAGE hhook=%x\n", ApiRef, hMouseHook);
#endif // DXW_NOTRACES
	}

	// v2.05.25: initialize a sequential fake device id for the CD emulator
	// v2.05.57: since now we use dxwLock/UnlockMciDeviceId() to pick a valid number, initializing the value is useless
	// unless wi miss the MCI_OPEN message. In that case, 1 seems the most likely good value.
	dxw.VirtualCDAudioDeviceId = 1;

	if(dxw.dwFlags5 & DISABLEALTTAB){
		OutTraceDW("%s: SetWindowsHookEx WH_KEYBOARD_LL\n", ApiRef);
		hKeyboardHook =(*pSetWindowsHookExA)(WH_KEYBOARD_LL, KeyboardHook, GetModuleHandle(NULL), 0);
#ifndef DXW_NOTRACES
		if(hKeyboardHook==NULL) OutTraceE("%s: SetWindowsHookEx WH_KEYBOARD_LL failed: error=%d\n", 
			ApiRef, GetLastError());
#endif // DXW_NOTRACES
	}

	InitScreenParameters(); 
	if(hwnd) dxw.HookWindowProc(hwnd);
	// in fullscreen mode, messages seem to reach and get processed by the parent window
	if((!dxw.Windowize) && hwnd) dxw.HookWindowProc(dxw.hParentWnd);

	// initialize window: if
	// 1) not in injection mode (hwnd != 0) and
	// 2) in Windowed mode and
	// 3) supposedly in fullscreen mode (dxw.IsFullScreen()) and
	// 4) configuration may ask for a window style update

	if (hwnd && dxw.isScaled && dxw.IsFullScreen()) {
		dxw.FixWindowFrame(dxw.hChildWnd); // v2.04.44: checks inside ...
		dxw.AdjustWindowPos(dxw.hChildWnd, target->sizx, target->sizy);
		if(dxw.dwFlags1 & FIXPARENTWIN) {
			dxw.FixWindowFrame(dxw.hParentWnd); // v2.04.44: checks inside ...
			dxw.AdjustWindowPos(dxw.hParentWnd, target->sizx, target->sizy);
		}
		if(dxw.dwFlags9 & LOCKTOPZORDER) 
			(*pSetWindowPos)(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
	}

	if (dxw.dwFlags1 & AUTOREFRESH) 
		CreateThread(NULL, 0, MessagePoller, NULL, 0, NULL);

	// @#@ "Sangokushi Koumeiden" (Jp 1996)
	if (dxw.dwFlags17 & WINAUTOREPAINT) 
		CreateThread(NULL, 0, WindowRepainter, NULL, 0, NULL);
	
	if (dxw.dwFlags4 & ENABLETIMEFREEZE)
		CreateThread(NULL, 0, TimeFreezePoller, NULL, 0, NULL);

	if(dxw.dwFlags7 & CPUSLOWDOWN)
		CreateThread(NULL, 0, CpuSlow, NULL, 0, NULL);
	else
	if(dxw.dwFlags7 & CPUMAXUSAGE)
		CreateThread(NULL, 0, CpuLimit, NULL, 0, NULL);

	if(dxw.dwFlags2 & TIMESTRETCH){ // v2.04.41: necessary for proxy hooking
		extern DXWNDSTATUS *pStatus;
		pStatus->TimeShift = dxw.TimeShift;
	}

	if(DoOnce){
		// 2.06.13: patch moved inside the DoOnce check to avoid fake version detections
		if(dxw.dwFlags9 & FIXEMPIREOFS) PatchEFS();

		if(dxw.dwFlags8 & LOADGAMMARAMP) LoadGammaRamp();

		if(dxw.dwFlags2 & RECOVERSCREENMODE)
			CreateThread(NULL, 0, LockScreenMode, NULL, 0, NULL);

		// moved to the CreateWindow wrappers
		//if(dxw.dwFlags17 & XBOX2KEYBOARD){
		//	if(!(*pLoadLibraryA)("xbox2kbd.dll")) 
		//		OutTrace("%s: ERROR LoadLibrary(xbox2kbd.dll) err=%d\n", ApiRef, GetLastError());
		//}

		if(dxw.dwFlags7 & INJECTSUSPENDED) SignalHookEvent();

		extern PALETTEENTRY DefaultSystemPalette[256];
		extern void mySetPalette(int, int, LPPALETTEENTRY);
		mySetPalette(0, 256, DefaultSystemPalette); 
#ifndef DXW_NOTRACE
		if((dxw.ActualPixelFormat.dwRGBBitCount == 8) && IsDebugSYS) dxw.DumpSysPalette();
#endif // DXW_NOTRACE

		// v2.05.53: folder type initialization
		extern DWORD InitFolderType(void);
		if(dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE)) dxw.dwCurrentFolderType = InitFolderType();

		if(dxw.dwFlags15 & SETPRIORITYLOW) (*pSetPriorityClass)(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

		if(dxw.dwDFlags2 & DISABLEWINHOOK) MessageBox(0, "Injected hook completed", "DxWnd", 0);

	}

	if(dxw.GDIEmulationMode == GDIMODE_ASYNCDC) lpADC = &adc;

	if(dxw.dwFlags16 & RUNBYDXWND) {
		char fname[MAX_PATH+1];
		bool bOk = FALSE;
		DWORD parentPid = getParentPID(GetCurrentProcessId());
		if(getProcessName(parentPid, fname, MAX_PATH) == 0){
			OutTrace("%s: parent pid=%#x path=%s\n", ApiRef, parentPid, fname);
			for(char *p=fname; *p; p++) *p = tolower(*p);
			if(strlen(fname) >= strlen("dxwnd.exe")) {
				if(!strcmp(&fname[strlen(fname)-strlen("dxwnd.exe")], "dxwnd.exe")) bOk = TRUE;
			}
			if(!bOk){
				MessageBox(0, "This program must be run by DxWnd!", "DxWnd", 0);
				exit(0);
			}
		}
	}

	if(dxw.dwFlags18 & ENABLEPATCHER) {
		OutTrace("%s: patcher invocation\n", ApiRef);
		patcher();
	}

	DoOnce = FALSE;
	OutTrace("%s: hook completed!\n", ApiRef);
}

LPCSTR ProcToString(LPCSTR proc)
{
	static char sBuf[24+1];
	if(((DWORD)proc & 0xFFFF0000) == 0){
		sprintf_s(sBuf, 24, "#%#x", proc);
		return sBuf;
	}
	else
		return proc;
}

FARPROC RemapLibraryEx(LPCSTR proc, HMODULE hModule, HookEntryEx_Type *Hooks)
{
	ApiName("RemapLibraryEx");
	void *remapped_addr;
	for(; Hooks->APIName; Hooks++){
		if (!strcmp(proc, Hooks->APIName)){
			if  ((Hooks->HookStatus == HOOK_HOT_REQUIRED) ||
				((dxw.dwFlags4 & HOTPATCH) && (Hooks->HookStatus == HOOK_HOT_CANDIDATE)) ||  // hot patch candidate still to process - or
				((dxw.dwFlags15 & HOTPATCHALWAYS) && (Hooks->HookStatus != HOOK_HOT_LINKED))){ // force hot patch and not already hooked

				if(!Hooks->OriginalAddress) {
					Hooks->OriginalAddress=(*pGetProcAddress)(hModule, Hooks->APIName);
					if(!Hooks->OriginalAddress) continue;
				}

				remapped_addr = HotPatch(Hooks->OriginalAddress, Hooks->APIName, Hooks->HookerAddress);
				if(remapped_addr == (void *)1) { // should never go here ...
					Hooks->HookStatus = HOOK_HOT_LINKED;
					continue; // already hooked
				}
				if(remapped_addr) {
					Hooks->HookStatus = HOOK_HOT_LINKED;
					if(Hooks->StoreAddress) *(Hooks->StoreAddress) = (FARPROC)remapped_addr;
				}			
			}
			if(Hooks->HookStatus == HOOK_HOT_LINKED) {
				OutTraceDW("%s: hot patched proc=%s addr=%#x\n", ApiRef, ProcToString(proc), Hooks->HookerAddress);
				return Hooks->HookerAddress;
			}
			if (Hooks->StoreAddress) *(Hooks->StoreAddress)=(*pGetProcAddress)(hModule, proc);
			OutTraceDW("%s: hooking proc=%s addr=%#x->%#x\n", 
				ApiRef,
				ProcToString(proc), 
				(Hooks->StoreAddress) ? *(Hooks->StoreAddress) : 0, 
				Hooks->HookerAddress);
			return Hooks->HookerAddress;
		}
	}
	MH_EnableHook(MH_ALL_HOOKS);

	return NULL;
}

// v2.05.83: similar to RemapLibraryEx but handling libs with optional calls
// that must return NULL in case the original pointer is absent.
// Fixes GLMars3D.exe that searches for wglGetExtensionsStringEXT that is not
// mandatory in all opengl32 releases.
// The Hot Patch logic was stripped here: there are no hot patched calls.
// v2.05.84: to be used for dxgi.dll as well. Maybe we should recover the 
// Hot Patch logic?
FARPROC RemapLibraryOpt(LPCSTR proc, HMODULE hModule, HookEntryEx_Type *Hooks)
{
	ApiName("RemapLibraryOpt");
	for(; Hooks->APIName; Hooks++){
		if (!strcmp(proc, Hooks->APIName)){
			if(Hooks->HookStatus == HOOK_HOT_LINKED) {
				OutTraceDW("%s: hot patched proc=%s addr=%#x\n", ApiRef, ProcToString(proc), Hooks->HookerAddress);
				return Hooks->HookerAddress;
			}
			if (Hooks->StoreAddress) *(Hooks->StoreAddress)=(*pGetProcAddress)(hModule, proc);
			OutTraceDW("%s: hooking proc=%s addr=%#x->%#x\n", 
				ApiRef,
				ProcToString(proc), 
				(Hooks->StoreAddress) ? *(Hooks->StoreAddress) : 0, 
				Hooks->HookerAddress);
			return *(Hooks->StoreAddress) ? Hooks->HookerAddress : NULL;
		}
	}
	return NULL;
}

void HookLibraryEx(HMODULE hModule, HookEntryEx_Type *Hooks, char *DLLName)
{
	ApiName("HookLibraryEx");
	HMODULE hDLL = NULL;

	//OutTrace("HookLibrary: hModule=%#x dll=%s\n", hModule, DLLName);

	for(; Hooks->APIName; Hooks++){
		void *remapped_addr;
		if(Hooks->HookStatus == HOOK_HOT_LINKED) continue; // skip any hot-linked entry
		if(((Hooks->HookStatus == HOOK_HOT_REQUIRED) ||
			((dxw.dwFlags4 & HOTPATCH) && (Hooks->HookStatus == HOOK_HOT_CANDIDATE)) ||  // hot patch candidate still to process - or
			((dxw.dwFlags15 & HOTPATCHALWAYS) && (Hooks->HookStatus <= HOOK_HOT_REQUIRED))) // force hot patch and not already hooked
			&&
			Hooks->StoreAddress){							 // and save ptr available
				OutDebugH("%s: trying hot-patch on %s\n", ApiRef, Hooks->APIName);
			// Hot Patch - beware! This way you're likely to hook unneeded libraries.
			// commented out this line: hot patch original addresses MUST be refreshed if
			// set by initializers
			//if(!Hooks->OriginalAddress) {
			if(!hDLL) {
				hDLL = (*pLoadLibraryA)(DLLName);
				if(!hDLL) {
					OutTrace("%s: LoadLibrary failed on DLL=%s err=%#x\n", ApiRef, DLLName, GetLastError());
					continue;
				}
			}
			Hooks->OriginalAddress=(*pGetProcAddress)(hDLL, Hooks->APIName);
			if(!Hooks->OriginalAddress) {
				OutDebugH("%s: GetProcAddress failed on API=%s err=%#x\n", ApiRef, Hooks->APIName, GetLastError());
				continue;
			}
			//}
			remapped_addr = HotPatch(Hooks->OriginalAddress, Hooks->APIName, Hooks->HookerAddress);
			switch((DWORD)remapped_addr){
				case 0:
					// v2.05.14: fix for duplicated HOT_CANDITATE or HOT_REQUIRED entries:
					// when HotPatch returns 0 it could mean that the entry is duplicated and the call was 
					// hooked already, so mark it as HOT LINKED anyway, just avoid to update the StoreAddress
					OutDebugH("%s: skipping hot hooked API=%s\n", ApiRef, Hooks->APIName);
					break;
				case 1:
					// should never go here ...
					OutDebugH("%s: skipping retcode=1\n", ApiRef);
					break;
				default:
					// successful case, save the address
					//OutTraceH("HookLibrary: HOT LINK on API=%s addr=%#x->%#x\n", 
					//	Hooks->APIName, Hooks->OriginalAddress, remapped_addr);
					*(Hooks->StoreAddress) = (FARPROC)remapped_addr;
					break;
			}

			Hooks->HookStatus = HOOK_HOT_LINKED;
			continue;
		}

		remapped_addr = IATPatch(hModule, Hooks->ordinal, DLLName, Hooks->OriginalAddress, Hooks->APIName, Hooks->HookerAddress);
		if(remapped_addr){
			Hooks->HookStatus = HOOK_IAT_LINKED;
			if (Hooks->StoreAddress) *(Hooks->StoreAddress) = (FARPROC)remapped_addr;
		}
	}
	// IATPatch(NULL, 0, NULL, NULL, NULL, NULL); // free resources - not working yet ....
	MH_EnableHook(MH_ALL_HOOKS);

}

void PinLibraryEx(HookEntryEx_Type *Hooks, char *DLLName)
{
	ApiName("PinLibraryEx");
	HMODULE hModule = NULL;
	hModule = (*pLoadLibraryA)(DLLName);
	if(!hModule) {
		OutTrace("%s: LoadLibrary failed on DLL=%s err=%#x\n", ApiRef, DLLName, GetLastError());
		return;
	}
	for(; Hooks->APIName; Hooks++){
		if (Hooks->StoreAddress) *(Hooks->StoreAddress) = (*pGetProcAddress)(hModule, Hooks->APIName);
	}
}

// tells wether at least one pointer was hooked here
BOOL IsHookedBlock(HookEntryEx_Type *Hooks)
{
	for(; Hooks->APIName; Hooks++){
		if (Hooks->StoreAddress) if(*(Hooks->StoreAddress)) return TRUE;
	}
	return FALSE;
}

BOOL IsHotPatchedEx(HookEntryEx_Type *Hooks, char *ApiName)
{
	for(; Hooks->APIName; Hooks++){
		if(!strcmp(Hooks->APIName, ApiName)) return (Hooks->HookStatus == HOOK_HOT_LINKED);
	}
	return FALSE;
}

void HookLibInitEx(HookEntryEx_Type *Hooks)
{
	for(; Hooks->APIName; Hooks++)
		if (Hooks->StoreAddress && Hooks->OriginalAddress) *(Hooks->StoreAddress) = Hooks->OriginalAddress;
}

