#define  _CRT_SECURE_NO_WARNINGS

#define _MODULE "kernel32"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "dxwlocale.h"
#include "hddraw.h"
#include "stdio.h"
#include "shlwapi.h"
// v2.05.20: added for DeviceIOControl interpretation about CDROM devices
#include "ntddcdrm.h"
#include "ntddscsi.h"
#include "heap.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifndef DXW_NOTRACES
#define TraceError() OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError())
#define IfTraceError() if(!res) TraceError()
#else
#define TraceError()
#define IfTraceError()
#endif

//#define ASSERTDIALOGS

// LOCKINJECTIONTHREADS enables the source code necessary to manage the Debug FREEZEINJECTEDSON option!
//#define LOCKINJECTIONTHREADS
#define TRYFATNAMES TRUE
//#define TRACEALL
//#define MONITORTHREADS
//#define NLSTRACE
//#define TRACERESOURCES
//#define TRACEHEAP

#ifdef TRACEALL
#define MONITORTHREADS
#define NLSTRACE
#define TRACERESOURCES
#define TRACEHEAP
#endif

extern HRESULT WINAPI extDirectDrawEnumerateA(LPDDENUMCALLBACK, LPVOID);
extern HRESULT WINAPI extDirectDrawEnumerateExA(LPDDENUMCALLBACKEX, LPVOID, DWORD);
extern void ReplaceCPUID(char *);
extern DirectDrawEnumerateA_Type pDirectDrawEnumerateA;
extern DirectDrawEnumerateExA_Type pDirectDrawEnumerateExA;
extern void HookModule(HMODULE);
extern void HookDlls(HMODULE);

HANDLE CDLockFile = 0;

typedef DWORD (WINAPI *WaitForSingleObject_Type)(HANDLE, DWORD);
DWORD WINAPI extWaitForSingleObject(HANDLE, DWORD);
WaitForSingleObject_Type pWaitForSingleObject;
typedef HGLOBAL (WINAPI *GlobalFree_Type)(HGLOBAL);
HGLOBAL WINAPI extGlobalFree(HGLOBAL);
GlobalFree_Type pGlobalFree = NULL;
typedef LPSTR (WINAPI *GetCommandLineA_Type)(void);
GetCommandLineA_Type pGetCommandLineA;
LPSTR WINAPI extGetCommandLineA();
typedef DWORD (WINAPI *GetModuleFileNameA_Type)(HMODULE, LPSTR, DWORD);
GetModuleFileNameA_Type pGetModuleFileNameA;
DWORD WINAPI extGetModuleFileNameA(HMODULE, LPSTR, DWORD);
typedef BOOL (WINAPI *GetOverlappedResult_Type)(HANDLE, LPOVERLAPPED, LPDWORD, BOOL);
GetOverlappedResult_Type pGetOverlappedResult;
BOOL WINAPI extGetOverlappedResult(HANDLE, LPOVERLAPPED, LPDWORD, BOOL);
typedef DWORD (WINAPI *GetFullPathNameA_Type)(LPCSTR, DWORD, LPSTR, LPSTR *);
GetFullPathNameA_Type pGetFullPathNameA;
DWORD WINAPI extGetFullPathNameA(LPCSTR, DWORD, LPSTR, LPSTR *);
//typedef DWORD (WINAPI *GetFullPathNameW_Type)(LPCWSTR, DWORD, LPWSTR, LPWSTR *);
//GetFullPathNameW_Type pGetFullPathNameW;
//DWORD WINAPI extGetFullPathNameW(LPCWSTR, DWORD, LPWSTR, LPWSTR *);

#ifdef TRACEALL
typedef int (WINAPI *GetDateFormatA_Type)(LCID, DWORD, const SYSTEMTIME *, LPCSTR, LPSTR, int);
GetDateFormatA_Type pGetDateFormatA;
int WINAPI extGetDateFormatA(LCID, DWORD, const SYSTEMTIME *, LPCSTR, LPSTR, int);
typedef int (WINAPI *GetDateFormatW_Type)(LCID, DWORD, const SYSTEMTIME *, LPCWSTR, LPWSTR, int);
GetDateFormatW_Type pGetDateFormatW;
int WINAPI extGetDateFormatW(LCID, DWORD, const SYSTEMTIME *, LPCWSTR, LPWSTR, int);
LPVOID WINAPI extGlobalLock(HGLOBAL);
HGLOBAL WINAPI extGlobalAlloc(UINT, SIZE_T);
HGLOBAL WINAPI extGlobalReAlloc(HGLOBAL, SIZE_T, UINT);
BOOL WINAPI extGlobalMemoryStatusEx(LPMEMORYSTATUSEX);
BOOL WINAPI extGetPhysicallyInstalledSystemMemory(PULONGLONG);
typedef LPVOID (WINAPI *GlobalLock_Type)(HGLOBAL);
typedef HGLOBAL (WINAPI *GlobalAlloc_Type)(UINT, SIZE_T);
typedef HGLOBAL (WINAPI *GlobalReAlloc_Type)(HGLOBAL, SIZE_T, UINT);
typedef BOOL (WINAPI *GlobalMemoryStatusEx_Type)(LPMEMORYSTATUSEX);
typedef BOOL (WINAPI *GetPhysicallyInstalledSystemMemory_Type)(PULONGLONG);
GlobalLock_Type pGlobalLock = NULL;
GlobalAlloc_Type pGlobalAlloc = NULL;
GlobalReAlloc_Type pGlobalReAlloc = NULL;
GlobalMemoryStatusEx_Type pGlobalMemoryStatusEx = NULL;
GetPhysicallyInstalledSystemMemory_Type pGetPhysicallyInstalledSystemMemory = NULL;
typedef HGLOBAL (WINAPI *GlobalHandle_Type)(LPCVOID);
GlobalHandle_Type pGlobalHandle;
HGLOBAL WINAPI extGlobalHandle(LPCVOID);
typedef HLOCAL (WINAPI *LocalAlloc_Type)(UINT, SIZE_T);
typedef HLOCAL (WINAPI *LocalFree_Type)(HLOCAL);
LocalAlloc_Type pLocalAlloc;
LocalFree_Type pLocalFree;
HLOCAL WINAPI extLocalAlloc(UINT, SIZE_T);
HLOCAL WINAPI extLocalFree(HLOCAL);
typedef DWORD (WINAPI *GetFileType_Type)(HANDLE);
GetFileType_Type pGetFileType;
DWORD WINAPI extGetFileType(HANDLE);
typedef HMODULE (WINAPI *GetModuleHandleA_Type)(LPCSTR);
GetModuleHandleA_Type pGetModuleHandleA;
HMODULE WINAPI extGetModuleHandleA(LPCSTR);
typedef UINT (WINAPI *SetHandleCount_Type)(UINT);
SetHandleCount_Type pSetHandleCount;
UINT WINAPI extSetHandleCount(UINT);
#endif

#ifdef MONITORTHREADS
HANDLE WINAPI extCreateThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
typedef HANDLE (WINAPI *CreateThread_Type)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
CreateThread_Type pCreateThread = NULL;
#endif // MONITORTHREADS

typedef UINT (WINAPI *GetWindowsDirectoryA_Type)(LPSTR, UINT);
GetWindowsDirectoryA_Type pGetWindowsDirectoryA = NULL;
UINT WINAPI extGetWindowsDirectoryA(LPSTR, UINT);
typedef UINT (WINAPI *GetWindowsDirectoryW_Type)(LPWSTR, UINT);
GetWindowsDirectoryW_Type pGetWindowsDirectoryW = NULL;
UINT WINAPI extGetWindowsDirectoryW(LPWSTR, UINT);
typedef DWORD (WINAPI *GetPrivateProfileStringA_Type)(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);
GetPrivateProfileStringA_Type pGetPrivateProfileStringA = NULL;
DWORD WINAPI extGetPrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);
typedef DWORD (WINAPI *GetPrivateProfileStringW_Type)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
GetPrivateProfileStringW_Type pGetPrivateProfileStringW = NULL;
DWORD WINAPI extGetPrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
typedef DWORD (WINAPI *WritePrivateProfileStringA_Type)(LPCSTR, LPCSTR, LPSTR, LPCSTR);
WritePrivateProfileStringA_Type pWritePrivateProfileStringA = NULL;
DWORD WINAPI extWritePrivateProfileStringA(LPCSTR, LPCSTR, LPSTR, LPCSTR);
typedef DWORD (WINAPI *GetProfileStringA_Type)(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD);
GetProfileStringA_Type pGetProfileStringA = NULL;
DWORD WINAPI extGetProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD);
typedef DWORD (WINAPI *WriteProfileStringA_Type)(LPCSTR, LPCSTR, LPSTR);
WriteProfileStringA_Type pWriteProfileStringA = NULL;
DWORD WINAPI extWriteProfileStringA(LPCSTR, LPCSTR, LPSTR);
typedef UINT (WINAPI *GetPrivateProfileIntA_Type)(LPCTSTR, LPCTSTR, INT, LPCTSTR);
GetPrivateProfileIntA_Type pGetPrivateProfileIntA = NULL;
UINT WINAPI extGetPrivateProfileIntA(LPCTSTR, LPCTSTR, INT, LPCTSTR);
typedef UINT (WINAPI *GetPrivateProfileIntW_Type)(LPCWSTR, LPCWSTR, INT, LPCWSTR);
GetPrivateProfileIntW_Type pGetPrivateProfileIntW = NULL;
UINT WINAPI extGetPrivateProfileIntW(LPCWSTR, LPCWSTR, INT, LPCWSTR);
typedef UINT (WINAPI *GetProfileIntA_Type)(LPCTSTR, LPCTSTR, INT);
GetProfileIntA_Type pGetProfileIntA = NULL;
UINT WINAPI extGetProfileIntA(LPCTSTR, LPCTSTR, INT);
typedef UINT (WINAPI *GetProfileIntW_Type)(LPCWSTR, LPCWSTR, INT);
GetProfileIntW_Type pGetProfileIntW = NULL;
UINT WINAPI extGetProfileIntW(LPCWSTR, LPCWSTR, INT);
typedef BOOL (WINAPI *DeviceIoControl_Type)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
DeviceIoControl_Type pDeviceIoControl = NULL;
BOOL WINAPI extDeviceIoControl(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *GetStringTypeA_Type)(LCID, DWORD, LPCSTR, int, LPDWORD);
GetStringTypeA_Type pGetStringTypeA = NULL;
BOOL WINAPI extGetStringTypeA(LCID, DWORD, LPCSTR, int, LPDWORD);
typedef int (WINAPI *LCMapStringA_Type)(LCID, DWORD, LPCSTR, int, LPSTR, int);
LCMapStringA_Type pLCMapStringA = NULL;
int WINAPI extLCMapStringA(LCID, DWORD, LPCSTR, int, LPSTR, int);
typedef int (WINAPI *LCMapStringW_Type)(LCID, DWORD, LPCWSTR, int, LPWSTR, int);
LCMapStringW_Type pLCMapStringW = NULL;
int WINAPI extLCMapStringW(LCID, DWORD, LPCWSTR, int, LPWSTR, int);
typedef BOOL (WINAPI *GetStringTypeExA_Type)(LCID, DWORD, LPCSTR, INT, LPWORD);
GetStringTypeExA_Type pGetStringTypeExA;
BOOL WINAPI extGetStringTypeExA(LCID, DWORD, LPCSTR, INT, LPWORD);
typedef BOOL (WINAPI *GetStringTypeExW_Type)(LCID, DWORD, LPCWCH, INT, LPWORD);
GetStringTypeExW_Type pGetStringTypeExW;
BOOL WINAPI extGetStringTypeExW(LCID, DWORD, LPCWCH, INT, LPWORD);

#ifdef NLSTRACE
typedef LPSTR (WINAPI *lstrcpynA_Type)(LPSTR, LPCSTR, int);
lstrcpynA_Type plstrcpynA;
LPSTR WINAPI extlstrcpynA(LPSTR, LPCSTR, int);
typedef LPSTR (WINAPI *lstrcpyA_Type)(LPSTR, LPCSTR);
lstrcpyA_Type plstrcpyA;
LPSTR WINAPI extlstrcpyA(LPSTR, LPCSTR);
typedef int (WINAPI *GetLocaleInfoA_Type)(LCID, LCTYPE, LPSTR, int);
GetLocaleInfoA_Type pGetLocaleInfoA;
int WINAPI extGetLocaleInfoA(LCID, LCTYPE, LPSTR, int);
typedef int (WINAPI *GetLocaleInfoW_Type)(LCID, LCTYPE, LPWSTR, int);
GetLocaleInfoW_Type pGetLocaleInfoW;
int WINAPI extGetLocaleInfoW(LCID, LCTYPE, LPWSTR, int);
typedef BOOL (WINAPI *WriteConsole_Type)(HANDLE, const VOID *, DWORD, LPDWORD, LPVOID);
WriteConsole_Type pWriteConsoleA, pWriteConsoleW;
BOOL WINAPI extWriteConsoleA(HANDLE, const VOID *, DWORD, LPDWORD, LPVOID);
BOOL WINAPI extWriteConsoleW(HANDLE, const VOID *, DWORD, LPDWORD, LPVOID);
#endif // NLSTRACE

#ifndef DXW_NOTRACES
// ex-TRACEIO group
typedef BOOL (WINAPI *WriteFile_Type)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
WriteFile_Type pWriteFile = NULL;
BOOL WINAPI extWriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *FindClose_Type)(HANDLE);
FindClose_Type pFindClose;
BOOL WINAPI extFindClose(HANDLE);
typedef UINT (WINAPI *_lread_Type)(HFILE, LPVOID, UINT);
_lread_Type p_lread;
UINT WINAPI ext_lread(HFILE, LPVOID, UINT);
typedef long (WINAPI *_hread_Type)(HFILE, LPVOID, long);
_hread_Type p_hread;
long WINAPI ext_hread(HFILE, LPVOID, long);
typedef UINT (WINAPI *_lwrite_Type)(HFILE, LPCCH, UINT);
_lwrite_Type p_lwrite, p_hwrite;
UINT WINAPI ext_lwrite(HFILE, LPCCH, UINT);
UINT WINAPI ext_hwrite(HFILE, LPCCH, UINT);
typedef DWORD (WINAPI *SetFilePointer_Type)(HANDLE, LONG, PLONG, DWORD);
SetFilePointer_Type pSetFilePointer;
extern DWORD WINAPI extSetFilePointer(HANDLE, LONG, PLONG, DWORD);
typedef LONG (WINAPI *_llseek_Type)(HFILE, LONG, int);
_llseek_Type p_llseek;
LONG WINAPI ext_llseek(HFILE, LONG, int);
typedef HFILE (WINAPI *_lclose_Type)(HFILE);
_lclose_Type p_lclose;
HFILE WINAPI ext_lclose(HFILE);
#endif // DXW_NOTRACES

typedef BOOL (WINAPI *GetStringTypeW_Type)(DWORD, LPCWCH, int, LPWORD);
GetStringTypeW_Type pGetStringTypeW = NULL;
BOOL WINAPI extGetStringTypeW(DWORD, LPCWCH, int, LPWORD);
typedef BOOL (WINAPI *GetCPInfo_Type)(UINT, LPCPINFO);
GetCPInfo_Type pGetCPInfo;
BOOL WINAPI extGetCPInfo(UINT, LPCPINFO);
typedef BOOL (WINAPI *GetCPInfoExA_Type)(UINT, DWORD, LPCPINFOEXA);
GetCPInfoExA_Type pGetCPInfoExA;
BOOL WINAPI extGetCPInfoExA(UINT, DWORD, LPCPINFOEXA);
typedef BOOL (WINAPI *GetCPInfoExW_Type)(UINT, DWORD, LPCPINFOEXW);
GetCPInfoExW_Type pGetCPInfoExW;
BOOL WINAPI extGetCPInfoExW(UINT, DWORD, LPCPINFOEXW);
typedef LCID (WINAPI *GetThreadLocale_Type)();
GetThreadLocale_Type pGetThreadLocale = NULL;
LCID WINAPI extGetThreadLocale();
typedef BOOL (WINAPI *SetThreadLocale_Type)(LCID);
SetThreadLocale_Type pSetThreadLocale = NULL;
BOOL WINAPI extSetThreadLocale(LCID);
typedef BOOL (WINAPI *IsDBCSLeadByte_Type)(BYTE);
IsDBCSLeadByte_Type pIsDBCSLeadByte;
BOOL WINAPI extIsDBCSLeadByte(BYTE);
typedef BOOL (WINAPI *IsDBCSLeadByteEx_Type)(UINT, BYTE);
IsDBCSLeadByteEx_Type pIsDBCSLeadByteEx = NULL; // must be initialized!!
BOOL WINAPI extIsDBCSLeadByteEx(UINT, BYTE);
typedef UINT (WINAPI *GetCP_Type)();
GetCP_Type pGetACP, pGetOEMCP;
UINT WINAPI extGetACP();
UINT WINAPI extGetOEMCP();
typedef int (WINAPI *MultiByteToWideChar_Type)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
MultiByteToWideChar_Type pMultiByteToWideChar = NULL;
int WINAPI extMultiByteToWideChar(UINT, DWORD, LPCSTR, int, LPWSTR, int);
typedef int (WINAPI *WideCharToMultiByte_Type)(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
WideCharToMultiByte_Type pWideCharToMultiByte = NULL;
int WINAPI extWideCharToMultiByte(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
typedef HRSRC (WINAPI *FindResourceA_Type)(HMODULE, LPCSTR, LPCSTR);
FindResourceA_Type pFindResourceA;
HRSRC WINAPI extFindResourceA(HMODULE, LPCSTR, LPCSTR);
typedef HRSRC (WINAPI *FindResourceExA_Type)(HMODULE, LPCSTR, LPCSTR, WORD);
FindResourceExA_Type pFindResourceExA = NULL;
HRSRC WINAPI extFindResourceExA(HMODULE, LPCSTR, LPCSTR, WORD);
LANGID WINAPI extGetUserDefaultUILanguage(void);
LANGID WINAPI extGetSystemDefaultUILanguage(void);
LANGID WINAPI extGetSystemDefaultLangID(void);
LANGID WINAPI extGetUserDefaultLangID(void);
LCID WINAPI extGetThreadLocale(void);
LCID WINAPI extGetSystemDefaultLCID(void);
LCID WINAPI extGetUserDefaultLCID(void);
typedef BOOL (WINAPI *IsValidCodePage_Type)(UINT);
IsValidCodePage_Type pIsValidCodePage;
BOOL WINAPI extIsValidCodePage(UINT);
typedef BOOL (WINAPI *IsValidLocale_Type)(LCID, DWORD);
IsValidLocale_Type pIsValidLocale;
BOOL WINAPI extIsValidLocale(LCID, DWORD);
typedef BOOL (WINAPI *EnumSystemLocalesA_Type)(LOCALE_ENUMPROCA, DWORD);
EnumSystemLocalesA_Type pEnumSystemLocalesA;
BOOL WINAPI extEnumSystemLocalesA(LOCALE_ENUMPROCA, DWORD);
typedef BOOL (WINAPI *VirtualFree_Type)(LPVOID, SIZE_T, DWORD);
VirtualFree_Type pVirtualFree;
BOOL WINAPI extVirtualFree(LPVOID, SIZE_T, DWORD);

// definitions for fake global atoms
typedef ATOM (WINAPI *GlobalDeleteAtom_Type)(ATOM);
typedef ATOM (WINAPI *GlobalAddAtomA_Type)(LPCSTR);
typedef ATOM (WINAPI *GlobalFindAtomA_Type)(LPCSTR);
typedef UINT (WINAPI *GlobalGetAtomNameA_Type)(ATOM, LPSTR, int);
GlobalDeleteAtom_Type pGlobalDeleteAtom = NULL;
GlobalAddAtomA_Type pGlobalAddAtomA = NULL;
GlobalFindAtomA_Type pGlobalFindAtomA = NULL;
GlobalGetAtomNameA_Type pGlobalGetAtomNameA = NULL;
ATOM WINAPI extGlobalDeleteAtom(ATOM);
ATOM WINAPI extGlobalAddAtomA(LPCSTR);
ATOM WINAPI extGlobalFindAtomA(LPCSTR);
UINT WINAPI extGlobalGetAtomNameA(ATOM, LPSTR, int);

typedef DWORD (WINAPI *GetCurrentDirectoryA_Type)(DWORD, LPSTR);
GetCurrentDirectoryA_Type pGetCurrentDirectoryA = NULL;
DWORD WINAPI extGetCurrentDirectoryA(DWORD, LPSTR);
typedef DWORD (WINAPI *GetCurrentDirectoryW_Type)(DWORD, LPWSTR);
GetCurrentDirectoryW_Type pGetCurrentDirectoryW = NULL;
DWORD WINAPI extGetCurrentDirectoryW(DWORD, LPWSTR);
typedef BOOL (WINAPI *CreateDirectoryA_Type)(LPCSTR, LPSECURITY_ATTRIBUTES);
CreateDirectoryA_Type pCreateDirectoryA = NULL;
BOOL WINAPI extCreateDirectoryA(LPCSTR, LPSECURITY_ATTRIBUTES);
typedef BOOL (WINAPI *RemoveDirectoryA_Type)(LPCSTR);
RemoveDirectoryA_Type pRemoveDirectoryA = NULL;
BOOL WINAPI extRemoveDirectoryA(LPCSTR);
typedef DWORD (WINAPI *GetFileSize_Type)(HANDLE, LPDWORD);
GetFileSize_Type pGetFileSize;
DWORD WINAPI extGetFileSize(HANDLE, LPDWORD);
typedef BOOLEAN (WINAPI *CreateSymbolicLinkA_Type)(LPCSTR, LPCSTR, DWORD);
CreateSymbolicLinkA_Type pCreateSymbolicLinkA;
BOOLEAN WINAPI extCreateSymbolicLinkA(LPCSTR, LPCSTR, DWORD);
typedef BOOLEAN (WINAPI *CreateSymbolicLinkW_Type)(LPCWSTR, LPCWSTR, DWORD);
CreateSymbolicLinkW_Type pCreateSymbolicLinkW;
BOOLEAN WINAPI extCreateSymbolicLinkW(LPCWSTR, LPCWSTR, DWORD);

typedef DWORD (WINAPI *SearchPathA_Type)(LPCSTR, LPCSTR, LPCSTR, DWORD, LPSTR, LPSTR *);
SearchPathA_Type pSearchPathA;
DWORD WINAPI extSearchPathA(LPCSTR, LPCSTR, LPCSTR, DWORD, LPSTR, LPSTR *);
typedef DWORD (WINAPI *SearchPathW_Type)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPWSTR, LPWSTR *);
SearchPathW_Type pSearchPathW;
DWORD WINAPI extSearchPathW(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPWSTR, LPWSTR *);

#ifdef TRACERESOURCES
typedef HGLOBAL (WINAPI *LoadResource_Type)(HMODULE, HRSRC);
LoadResource_Type pLoadResource;
HGLOBAL WINAPI extLoadResource(HMODULE, HRSRC);
typedef LPVOID (WINAPI *LockResource_Type)(HGLOBAL);
LockResource_Type pLockResource;
LPVOID WINAPI extLockResource(HGLOBAL);
#endif // TRACERESOURCES

typedef DWORD (WINAPI *SuspendThread_Type)(HANDLE);
typedef DWORD (WINAPI *ResumeThread_Type)(HANDLE);
SuspendThread_Type pSuspendThread;
ResumeThread_Type pResumeThread;
DWORD WINAPI extSuspendThread(HANDLE);
DWORD WINAPI extResumeThread(HANDLE);
typedef BOOL (WINAPI *SetThreadPriority_Type)(HANDLE, int);
SetThreadPriority_Type pSetThreadPriority;
BOOL WINAPI extSetThreadPriority(HANDLE, int);

typedef BOOL (WINAPI *DuplicateHandle_Type)(HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD);
DuplicateHandle_Type pDuplicateHandle;
BOOL WINAPI extDuplicateHandle(HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD);

typedef BOOL (WINAPI *CopyFileA_Type)(LPCSTR, LPCSTR, BOOL);
typedef BOOL (WINAPI *CopyFileW_Type)(LPCWSTR, LPCWSTR, BOOL);
CopyFileA_Type pCopyFileA;
CopyFileW_Type pCopyFileW;
BOOL WINAPI extCopyFileA(LPCSTR, LPCSTR, BOOL);
BOOL WINAPI extCopyFileW(LPCWSTR, LPCWSTR, BOOL);

typedef BOOL (WINAPI *ReleaseMutex_Type)(HANDLE);
ReleaseMutex_Type pReleaseMutex;
BOOL WINAPI extReleaseMutex(HANDLE);

//#define TLSDUMP
#ifdef TLSDUMP
typedef DWORD (WINAPI *TlsAlloc_Type)();
typedef BOOL (WINAPI *TlsFree_Type)(DWORD);
typedef LPVOID (WINAPI *TlsGetValue_Type)(DWORD);
typedef BOOL (WINAPI *TlsSetValue_Type)(DWORD, LPVOID);

TlsAlloc_Type pTlsAlloc;
TlsFree_Type pTlsFree;
TlsGetValue_Type pTlsGetValue;
TlsSetValue_Type pTlsSetValue;

DWORD WINAPI extTlsAlloc();
BOOL WINAPI extTlsFree(DWORD);
LPVOID WINAPI extTlsGetValue(DWORD);
BOOL WINAPI extTlsSetValue(DWORD, LPVOID);
#endif

typedef UINT (WINAPI *SetErrorMode_Type)(UINT);
SetErrorMode_Type pSetErrorMode;
UINT WINAPI extSetErrorMode(UINT);

// v2.02.96: the GetSystemInfo API is NOT hot patchable on Win7. This can cause problems because it can't be hooked by simply
// enabling hot patch. A solution is making all LoadLibrary* calls hot patchable, so that when loading the module, the call
// can be hooked by the IAT lookup. This fixes a problem after movie playing in Wind Fantasy SP.

static HookEntryEx_Type LauncherHooks[]={
	{HOOK_HOT_REQUIRED, 0, "CreateProcessA", (FARPROC)CreateProcessA, (FARPROC *)&pCreateProcessA, (FARPROC)extCreateProcessA},
	{HOOK_HOT_REQUIRED, 0, "CreateProcessW", (FARPROC)CreateProcessW, (FARPROC *)&pCreateProcessW, (FARPROC)extCreateProcessW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type Hooks[]={
	// hot forbidden on LoadLibraryEx* calls to avoid recursion on WinXP
	{HOOK_HOT_FORBIDDEN, 0, "LoadLibraryExA", (FARPROC)LoadLibraryExA, (FARPROC *)&pLoadLibraryExA, (FARPROC)extLoadLibraryExA},
	{HOOK_HOT_CANDIDATE, 0, "LoadLibraryA", (FARPROC)LoadLibraryA, (FARPROC *)&pLoadLibraryA, (FARPROC)extLoadLibraryA},
	{HOOK_HOT_FORBIDDEN, 0, "LoadLibraryExW", (FARPROC)LoadLibraryExW, (FARPROC *)&pLoadLibraryExW, (FARPROC)extLoadLibraryExW},
	{HOOK_HOT_CANDIDATE, 0, "LoadLibraryW", (FARPROC)LoadLibraryW, (FARPROC *)&pLoadLibraryW, (FARPROC)extLoadLibraryW},

	{HOOK_IAT_CANDIDATE, 0, "IsDebuggerPresent", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)extIsDebuggerPresent},
	{HOOK_IAT_CANDIDATE, 0, "CheckRemoteDebuggerPresent", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)extCheckRemoteDebuggerPresent},
	{HOOK_HOT_CANDIDATE, 0, "GetProcAddress", (FARPROC)GetProcAddress, (FARPROC *)&pGetProcAddress, (FARPROC)extGetProcAddress},

	{HOOK_IAT_CANDIDATE, 0, "GetDriveTypeA", (FARPROC)GetDriveTypeA, (FARPROC *)&pGetDriveTypeA, (FARPROC)extGetDriveTypeA},
	{HOOK_IAT_CANDIDATE, 0, "GetDriveTypeW", (FARPROC)GetDriveTypeW, (FARPROC *)&pGetDriveTypeW, (FARPROC)extGetDriveTypeW},
	{HOOK_IAT_CANDIDATE, 0, "GetLogicalDrives", (FARPROC)GetLogicalDrives, (FARPROC *)&pGetLogicalDrives, (FARPROC)extGetLogicalDrives},
	{HOOK_IAT_CANDIDATE, 0, "GetTempFileNameA", (FARPROC)GetTempFileNameA, (FARPROC *)&pGetTempFileNameA, (FARPROC)extGetTempFileNameA},
	{HOOK_IAT_CANDIDATE, 0, "GetTempFileNameW", (FARPROC)GetTempFileNameW, (FARPROC *)&pGetTempFileNameW, (FARPROC)extGetTempFileNameW},
	{HOOK_IAT_CANDIDATE, 0, "CreateProcessA", (FARPROC)CreateProcessA, (FARPROC *)&pCreateProcessA, (FARPROC)extCreateProcessA},
	{HOOK_IAT_CANDIDATE, 0, "CreateProcessW", (FARPROC)CreateProcessW, (FARPROC *)&pCreateProcessW, (FARPROC)extCreateProcessW},
	{HOOK_IAT_CANDIDATE, 0, "WinExec", (FARPROC)WinExec, (FARPROC *)&pWinExec, (FARPROC)extWinExec},
	{HOOK_HOT_CANDIDATE, 0, "GlobalUnlock", (FARPROC)GlobalUnlock, (FARPROC *)&pGlobalUnlock, (FARPROC)extGlobalUnlock},
	{HOOK_HOT_CANDIDATE, 0, "GlobalFree", (FARPROC)GlobalFree, (FARPROC *)&pGlobalFree, (FARPROC)extGlobalFree},	
	{HOOK_HOT_CANDIDATE, 0, "FreeLibrary", (FARPROC)FreeLibrary, (FARPROC *)&pFreeLibrary, (FARPROC)extFreeLibrary},
	{HOOK_HOT_CANDIDATE, 0, "IsProcessorFeaturePresent", (FARPROC)IsProcessorFeaturePresent, (FARPROC *)&pIsProcessorFeaturePresent, (FARPROC)extIsProcessorFeaturePresent},
	{HOOK_HOT_CANDIDATE, 0, "GetFullPathNameA", (FARPROC)GetFullPathNameA, (FARPROC *)&pGetFullPathNameA, (FARPROC)extGetFullPathNameA},
#ifdef TRACEALL
	{HOOK_HOT_CANDIDATE, 0, "RaiseException", (FARPROC)RaiseException, (FARPROC *)&pRaiseException, (FARPROC)extRaiseException},
	{HOOK_HOT_CANDIDATE, 0, "GlobalLock", (FARPROC)GlobalLock, (FARPROC *)&pGlobalLock, (FARPROC)extGlobalLock},
	{HOOK_HOT_CANDIDATE, 0, "GlobalAlloc", (FARPROC)GlobalAlloc, (FARPROC *)&pGlobalAlloc, (FARPROC)extGlobalAlloc},
	{HOOK_HOT_CANDIDATE, 0, "GlobalReAlloc", (FARPROC)GlobalReAlloc, (FARPROC *)&pGlobalReAlloc, (FARPROC)extGlobalReAlloc},
	{HOOK_HOT_CANDIDATE, 0, "GlobalHandle", (FARPROC)GlobalHandle, (FARPROC *)&pGlobalHandle, (FARPROC)extGlobalHandle},
	{HOOK_HOT_CANDIDATE, 0, "LocalAlloc", (FARPROC)LocalAlloc, (FARPROC *)&pLocalAlloc, (FARPROC)extLocalAlloc},
	{HOOK_HOT_CANDIDATE, 0, "LocalFree", (FARPROC)LocalFree, (FARPROC *)&pLocalFree, (FARPROC)extLocalFree},
	{HOOK_HOT_CANDIDATE, 0, "GlobalMemoryStatusEx", (FARPROC)GlobalMemoryStatusEx, (FARPROC *)&pGlobalMemoryStatusEx, (FARPROC)extGlobalMemoryStatusEx},
	{HOOK_HOT_CANDIDATE, 0, "GetPhysicallyInstalledSystemMemory", (FARPROC)NULL, (FARPROC *)&pGetPhysicallyInstalledSystemMemory, (FARPROC)extGetPhysicallyInstalledSystemMemory},
	{HOOK_HOT_REQUIRED, 0, "GetDateFormatA", (FARPROC)GetDateFormatA, (FARPROC *)pGetDateFormatA, (FARPROC)extGetDateFormatA},
	{HOOK_HOT_REQUIRED, 0, "GetDateFormatW", (FARPROC)GetDateFormatW, (FARPROC *)pGetDateFormatW, (FARPROC)extGetDateFormatW},
	{HOOK_HOT_CANDIDATE, 0, "SetHandleCount", (FARPROC)SetHandleCount, (FARPROC *)&pSetHandleCount, (FARPROC)extSetHandleCount},
#endif // TRACEALL
	{HOOK_HOT_CANDIDATE, 0, "DeviceIoControl", (FARPROC)DeviceIoControl, (FARPROC *)&pDeviceIoControl, (FARPROC)extDeviceIoControl},
#ifdef TRACERESOURCES
	{HOOK_IAT_CANDIDATE, 0, "LoadResource", (FARPROC)LoadResource, (FARPROC *)&pLoadResource, (FARPROC)extLoadResource},
	{HOOK_IAT_CANDIDATE, 0, "LockResource", (FARPROC)LockResource, (FARPROC *)&pLockResource, (FARPROC)extLockResource},
#endif // TRACERESOURCES
	{HOOK_IAT_CANDIDATE, 0, "DuplicateHandle", (FARPROC)DuplicateHandle, (FARPROC *)&pDuplicateHandle, (FARPROC)extDuplicateHandle},

	// moved here because of references in Virtual I/O calls, they MUST provide a reliable original address
	{HOOK_HOT_CANDIDATE, 0, "SetCurrentDirectoryA", (FARPROC)SetCurrentDirectoryA, (FARPROC *)&pSetCurrentDirectoryA, (FARPROC)extSetCurrentDirectoryA},
	{HOOK_HOT_CANDIDATE, 0, "SetCurrentDirectoryW", (FARPROC)SetCurrentDirectoryW, (FARPROC *)&pSetCurrentDirectoryW, (FARPROC)extSetCurrentDirectoryW},
	{HOOK_IAT_CANDIDATE, 0, "GetCurrentDirectoryA", (FARPROC)GetCurrentDirectoryA, (FARPROC *)&pGetCurrentDirectoryA, (FARPROC)extGetCurrentDirectoryA},
	{HOOK_IAT_CANDIDATE, 0, "GetCurrentDirectoryW", (FARPROC)GetCurrentDirectoryW, (FARPROC *)&pGetCurrentDirectoryW, (FARPROC)extGetCurrentDirectoryW},

	// v2.05.87: moved here because referenced in Windows9X emulation
	{HOOK_IAT_CANDIDATE, 0, "CreateFileMappingA", (FARPROC)NULL, (FARPROC *)&pCreateFileMappingA, (FARPROC)extCreateFileMappingA},
	{HOOK_IAT_CANDIDATE, 0, "CreateFileMappingW", (FARPROC)NULL, (FARPROC *)&pCreateFileMappingW, (FARPROC)extCreateFileMappingW},
	{HOOK_IAT_CANDIDATE, 0, "OpenFileMappingA", (FARPROC)NULL, (FARPROC *)&pOpenFileMappingA, (FARPROC)extOpenFileMappingA},
	{HOOK_IAT_CANDIDATE, 0, "OpenFileMappingW", (FARPROC)NULL, (FARPROC *)&pOpenFileMappingW, (FARPROC)extOpenFileMappingW},

	{HOOK_IAT_CANDIDATE, 0, "GetCommandLineA", (FARPROC)GetCommandLineA, (FARPROC *)&pGetCommandLineA, (FARPROC)extGetCommandLineA},
	{HOOK_IAT_CANDIDATE, 0, "GetModuleFileNameA", (FARPROC)GetModuleFileNameA, (FARPROC *)&pGetModuleFileNameA, (FARPROC)extGetModuleFileNameA},

#ifdef TLSDUMP
	{HOOK_IAT_CANDIDATE, 0, "TlsAlloc", (FARPROC)TlsAlloc, (FARPROC *)&pTlsAlloc, (FARPROC)extTlsAlloc},
	{HOOK_IAT_CANDIDATE, 0, "TlsFree", (FARPROC)TlsFree, (FARPROC *)&pTlsFree, (FARPROC)extTlsFree},
	{HOOK_IAT_CANDIDATE, 0, "TlsGetValue", (FARPROC)TlsGetValue, (FARPROC *)&pTlsGetValue, (FARPROC)extTlsGetValue},
	{HOOK_IAT_CANDIDATE, 0, "TlsSetValue", (FARPROC)TlsSetValue, (FARPROC *)&pTlsSetValue, (FARPROC)extTlsSetValue},
#endif
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0, "GetFileType", (FARPROC)GetFileType, (FARPROC *)&pGetFileType, (FARPROC)extGetFileType},
	{HOOK_IAT_CANDIDATE, 0, "GetModuleHandleA", (FARPROC)GetModuleHandleA, (FARPROC *)&pGetModuleHandleA, (FARPROC)extGetModuleHandleA},
#endif

	{HOOK_IAT_CANDIDATE, 0, "SetErrorMode", (FARPROC)SetErrorMode, (FARPROC *)&pSetErrorMode, (FARPROC)extSetErrorMode},

	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type OverlappedHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "GetOverlappedResult", (FARPROC)GetOverlappedResult, (FARPROC *)&pGetOverlappedResult, (FARPROC)extGetOverlappedResult},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type WaitHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "WaitForSingleObject", (FARPROC)WaitForSingleObject, (FARPROC *)pWaitForSingleObject, (FARPROC)extWaitForSingleObject},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type HeapHooks[]={
	{HOOK_HOT_FORBIDDEN, 0, "GetProcessHeap", (FARPROC)GetProcessHeap, (FARPROC *)&pGetProcessHeap, (FARPROC)extGetProcessHeap},
	{HOOK_HOT_FORBIDDEN, 0, "GetProcessHeaps", (FARPROC)GetProcessHeaps, (FARPROC *)&pGetProcessHeaps, (FARPROC)extGetProcessHeaps},
	{HOOK_HOT_FORBIDDEN, 0, "HeapCreate", (FARPROC)HeapCreate, (FARPROC *)&pHeapCreate, (FARPROC)extHeapCreate},
	{HOOK_HOT_FORBIDDEN, 0, "HeapAlloc", (FARPROC)HeapAlloc, (FARPROC *)&pHeapAlloc, (FARPROC)extHeapAlloc},
	{HOOK_HOT_FORBIDDEN, 0, "HeapReAlloc", (FARPROC)HeapReAlloc, (FARPROC *)&pHeapReAlloc, (FARPROC)extHeapReAlloc},
	{HOOK_HOT_FORBIDDEN, 0, "HeapValidate", (FARPROC)HeapValidate, (FARPROC *)&pHeapValidate, (FARPROC)extHeapValidate},
	{HOOK_HOT_FORBIDDEN, 0, "HeapCompact", (FARPROC)HeapCompact, (FARPROC *)&pHeapCompact, (FARPROC)extHeapCompact},
	{HOOK_HOT_FORBIDDEN, 0, "HeapDestroy", (FARPROC)HeapDestroy, (FARPROC *)&pHeapDestroy, (FARPROC)extHeapDestroy},
	{HOOK_HOT_FORBIDDEN, 0, "HeapSize", (FARPROC)HeapSize, (FARPROC *)&pHeapSize, (FARPROC)extHeapSize},
	{HOOK_HOT_FORBIDDEN, 0, "HeapLock", (FARPROC)HeapLock, (FARPROC *)&pHeapLock, (FARPROC)extHeapLock},
	{HOOK_HOT_FORBIDDEN, 0, "HeapUnlock", (FARPROC)HeapUnlock, (FARPROC *)&pHeapUnlock, (FARPROC)extHeapUnlock},
	{HOOK_HOT_FORBIDDEN, 0, "HeapQueryInformation", (FARPROC)HeapQueryInformation, (FARPROC *)&pHeapQueryInformation, (FARPROC)extHeapQueryInformation},
	{HOOK_HOT_FORBIDDEN, 0, "HeapSetInformation", (FARPROC)HeapSetInformation, (FARPROC *)&pHeapSetInformation, (FARPROC)extHeapSetInformation},
	{HOOK_HOT_FORBIDDEN, 0, "HeapWalk", (FARPROC)HeapWalk, (FARPROC *)&pHeapWalk, (FARPROC)extHeapWalk},
	{HOOK_HOT_FORBIDDEN, 0, "HeapFree", (FARPROC)HeapFree, (FARPROC *)&pHeapFree, (FARPROC)extHeapFree},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type AllocHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "VirtualAlloc", (FARPROC)VirtualAlloc, (FARPROC *)&pVirtualAlloc, (FARPROC)extVirtualAlloc},
	{HOOK_IAT_CANDIDATE, 0, "VirtualFree", (FARPROC)VirtualFree, (FARPROC *)&pVirtualFree, (FARPROC)extVirtualFree},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type FixIOHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "ReadFile", (FARPROC)ReadFile, (FARPROC *)&pReadFile, (FARPROC)extReadFile},
	{HOOK_IAT_CANDIDATE, 0, "CreateFileA", (FARPROC)CreateFileA, (FARPROC *)&pCreateFileA, (FARPROC)extCreateFileA},
	{HOOK_IAT_CANDIDATE, 0, "CreateFileW", (FARPROC)CreateFileW, (FARPROC *)&pCreateFileW, (FARPROC)extCreateFileW},
	{HOOK_HOT_FORBIDDEN, 0, "CloseHandle", (FARPROC)CloseHandle, (FARPROC *)&pCloseHandle, (FARPROC)extCloseHandle},
	{HOOK_IAT_CANDIDATE, 0, "SetFilePointer", (FARPROC)NULL, (FARPROC *)&pSetFilePointer, (FARPROC)extSetFilePointer},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type FixW16Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "CreateFileA", (FARPROC)CreateFileA, (FARPROC *)&pCreateFileA, (FARPROC)extCreateFileA},
	{HOOK_IAT_CANDIDATE, 0, "CreateFileW", (FARPROC)CreateFileW, (FARPROC *)&pCreateFileW, (FARPROC)extCreateFileW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

#ifndef DXW_NOTRACES
static HookEntryEx_Type TraceIOHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "WriteFile", (FARPROC)NULL, (FARPROC *)&pWriteFile, (FARPROC)extWriteFile},
	{HOOK_IAT_CANDIDATE, 0, "_lread", (FARPROC)NULL, (FARPROC *)&p_lread, (FARPROC)ext_lread},
	{HOOK_IAT_CANDIDATE, 0, "_hread", (FARPROC)NULL, (FARPROC *)&p_hread, (FARPROC)ext_hread},
	{HOOK_IAT_CANDIDATE, 0, "_lwrite", (FARPROC)NULL, (FARPROC *)&p_lwrite, (FARPROC)ext_lwrite},
	{HOOK_IAT_CANDIDATE, 0, "_hwrite", (FARPROC)NULL, (FARPROC *)&p_hwrite, (FARPROC)ext_hwrite},
	{HOOK_IAT_CANDIDATE, 0, "_llseek", (FARPROC)NULL, (FARPROC *)&p_llseek, (FARPROC)ext_llseek},
	{HOOK_IAT_CANDIDATE, 0, "_lclose", (FARPROC)NULL, (FARPROC *)&p_lclose, (FARPROC)ext_lclose},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};
#endif // DXW_NOTRACES

static HookEntryEx_Type RemapIOHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "CreateFileA", (FARPROC)CreateFileA, (FARPROC *)&pCreateFileA, (FARPROC)extCreateFileA},
	{HOOK_IAT_CANDIDATE, 0, "CreateFileW", (FARPROC)CreateFileW, (FARPROC *)&pCreateFileW, (FARPROC)extCreateFileW},
	{HOOK_IAT_CANDIDATE, 0, "DeleteFileA", (FARPROC)NULL, (FARPROC *)&pDeleteFileA, (FARPROC)extDeleteFileA},
	{HOOK_IAT_CANDIDATE, 0, "DeleteFileW", (FARPROC)NULL, (FARPROC *)&pDeleteFileW, (FARPROC)extDeleteFileW},
	{HOOK_IAT_CANDIDATE, 0, "CreateDirectoryA", (FARPROC)NULL, (FARPROC *)&pCreateDirectoryA, (FARPROC)extCreateDirectoryA},
	{HOOK_IAT_CANDIDATE, 0, "RemoveDirectoryA", (FARPROC)NULL, (FARPROC *)&pRemoveDirectoryA, (FARPROC)extRemoveDirectoryA},
	{HOOK_IAT_CANDIDATE, 0, "MoveFileA", (FARPROC)NULL, (FARPROC *)&pMoveFileA, (FARPROC)extMoveFileA},
	{HOOK_IAT_CANDIDATE, 0, "FindFirstFileA", (FARPROC)FindFirstFileA, (FARPROC *)&pFindFirstFileA, (FARPROC)extFindFirstFileA},
	{HOOK_HOT_REQUIRED, 0, "FindFirstFileExA", (FARPROC)FindFirstFileExA, (FARPROC *)&pFindFirstFileExA, (FARPROC)extFindFirstFileExA},
	{HOOK_IAT_CANDIDATE, 0, "FindNextFileA", (FARPROC)FindNextFileA, (FARPROC *)&pFindNextFileA, (FARPROC)extFindNextFileA},
	{HOOK_IAT_CANDIDATE, 0, "FindFirstFileW", (FARPROC)FindFirstFileW, (FARPROC *)&pFindFirstFileW, (FARPROC)extFindFirstFileW},
	{HOOK_HOT_REQUIRED, 0, "FindFirstFileExW", (FARPROC)FindFirstFileExW, (FARPROC *)&pFindFirstFileExW, (FARPROC)extFindFirstFileExW},
	{HOOK_IAT_CANDIDATE, 0, "FindNextFileW", (FARPROC)FindNextFileW, (FARPROC *)&pFindNextFileW, (FARPROC)extFindNextFileW},
	{HOOK_IAT_CANDIDATE, 0, "FindClose", (FARPROC)NULL, (FARPROC *)&pFindClose, (FARPROC)extFindClose},
	{HOOK_IAT_CANDIDATE, 0, "GetFileAttributesA", (FARPROC)NULL, (FARPROC *)&pGetFileAttributesA, (FARPROC)extGetFileAttributesA},
	{HOOK_IAT_CANDIDATE, 0, "GetVolumeInformationA", (FARPROC)GetVolumeInformationA, (FARPROC *)&pGetVolumeInformationA, (FARPROC)extGetVolumeInformationA},
	{HOOK_IAT_CANDIDATE, 0, "GetVolumeInformationW", (FARPROC)GetVolumeInformationW, (FARPROC *)&pGetVolumeInformationW, (FARPROC)extGetVolumeInformationW},
	{HOOK_IAT_CANDIDATE, 0, "GetLogicalDriveStringsA", (FARPROC)GetLogicalDriveStringsA, (FARPROC *)&pGetLogicalDriveStringsA, (FARPROC)extGetLogicalDriveStringsA},
	{HOOK_IAT_CANDIDATE, 0, "GetLogicalDriveStringsW", (FARPROC)GetLogicalDriveStringsW, (FARPROC *)&pGetLogicalDriveStringsW, (FARPROC)extGetLogicalDriveStringsW},
	{HOOK_IAT_CANDIDATE, 0, "_lopen", (FARPROC)_lopen, (FARPROC *)&p_lopen, (FARPROC)ext_lopen},
	{HOOK_IAT_CANDIDATE, 0, "_lcreat", (FARPROC)_lcreat, (FARPROC *)&p_lcreat, (FARPROC)ext_lcreat},
	{HOOK_HOT_CANDIDATE, 0, "OpenFile", (FARPROC)OpenFile, (FARPROC *)&pOpenFile, (FARPROC)extOpenFile},
	{HOOK_HOT_CANDIDATE, 0, "GetDiskFreeSpaceA", (FARPROC)GetDiskFreeSpaceA, (FARPROC *)&pGetDiskFreeSpaceA, (FARPROC)extGetDiskFreeSpaceA},
	{HOOK_HOT_CANDIDATE, 0, "GetDiskFreeSpaceW", (FARPROC)GetDiskFreeSpaceW, (FARPROC *)&pGetDiskFreeSpaceW, (FARPROC)extGetDiskFreeSpaceW},
	{HOOK_HOT_CANDIDATE, 0, "GetDiskFreeSpaceExA", (FARPROC)GetDiskFreeSpaceExA, (FARPROC *)&pGetDiskFreeSpaceExA, (FARPROC)extGetDiskFreeSpaceExA},
	{HOOK_HOT_CANDIDATE, 0, "GetDiskFreeSpaceExW", (FARPROC)GetDiskFreeSpaceExW, (FARPROC *)&pGetDiskFreeSpaceExW, (FARPROC)extGetDiskFreeSpaceExW},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileStringA", (FARPROC)GetPrivateProfileStringA, (FARPROC *)&pGetPrivateProfileStringA, (FARPROC)extGetPrivateProfileStringA},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileStringW", (FARPROC)GetPrivateProfileStringW, (FARPROC *)&pGetPrivateProfileStringW, (FARPROC)extGetPrivateProfileStringW},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileIntA", (FARPROC)GetPrivateProfileIntA, (FARPROC *)&pGetPrivateProfileIntA, (FARPROC)extGetPrivateProfileIntA},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileIntW", (FARPROC)GetPrivateProfileIntW, (FARPROC *)&pGetPrivateProfileIntW, (FARPROC)extGetPrivateProfileIntW},
	{HOOK_HOT_CANDIDATE, 0, "GetFileSize", (FARPROC)GetFileSize, (FARPROC *)&pGetFileSize, (FARPROC)extGetFileSize},
	// v2.05.70: erased CreateSymbolicLinkA/W direct references to keep WinXP compatibility 
	{HOOK_IAT_CANDIDATE, 0, "CreateSymbolicLinkA", (FARPROC)NULL, (FARPROC *)&pCreateSymbolicLinkA, (FARPROC)extCreateSymbolicLinkA},
	{HOOK_IAT_CANDIDATE, 0, "CreateSymbolicLinkW", (FARPROC)NULL, (FARPROC *)&pCreateSymbolicLinkW, (FARPROC)extCreateSymbolicLinkW},

	{HOOK_IAT_CANDIDATE, 0, "SearchPathA", (FARPROC)SearchPathA, (FARPROC *)&pSearchPathA, (FARPROC)extSearchPathA},
	{HOOK_IAT_CANDIDATE, 0, "SearchPathW", (FARPROC)SearchPathW, (FARPROC *)&pSearchPathW, (FARPROC)extSearchPathW},

	// missing: CopyFileExA/W
	{HOOK_IAT_CANDIDATE, 0, "CopyFileA", (FARPROC)CopyFileA, (FARPROC *)&pCopyFileA, (FARPROC)extCopyFileA},
	{HOOK_IAT_CANDIDATE, 0, "CopyFileW", (FARPROC)CopyFileW, (FARPROC *)&pCopyFileW, (FARPROC)extCopyFileW},

	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type LimitHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetDiskFreeSpaceA", (FARPROC)GetDiskFreeSpaceA, (FARPROC *)&pGetDiskFreeSpaceA, (FARPROC)extGetDiskFreeSpaceA},
	{HOOK_HOT_CANDIDATE, 0, "GetDiskFreeSpaceW", (FARPROC)GetDiskFreeSpaceW, (FARPROC *)&pGetDiskFreeSpaceW, (FARPROC)extGetDiskFreeSpaceW},
	{HOOK_HOT_CANDIDATE, 0, "GlobalMemoryStatus", (FARPROC)GlobalMemoryStatus, (FARPROC *)&pGlobalMemoryStatus, (FARPROC)extGlobalMemoryStatus},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type SysFolderHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetWindowsDirectoryA", (FARPROC)GetWindowsDirectoryA, (FARPROC *)&pGetWindowsDirectoryA, (FARPROC)extGetWindowsDirectoryA},
	{HOOK_HOT_CANDIDATE, 0, "GetWindowsDirectoryW", (FARPROC)GetWindowsDirectoryW, (FARPROC *)&pGetWindowsDirectoryW, (FARPROC)extGetWindowsDirectoryW},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileStringA", (FARPROC)GetPrivateProfileStringA, (FARPROC *)&pGetPrivateProfileStringA, (FARPROC)extGetPrivateProfileStringA},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileStringW", (FARPROC)GetPrivateProfileStringW, (FARPROC *)&pGetPrivateProfileStringW, (FARPROC)extGetPrivateProfileStringW},
	{HOOK_HOT_CANDIDATE, 0, "WritePrivateProfileStringA", (FARPROC)WritePrivateProfileStringA, (FARPROC *)&pWritePrivateProfileStringA, (FARPROC)extWritePrivateProfileStringA},
	{HOOK_HOT_CANDIDATE, 0, "GetProfileStringA", (FARPROC)GetProfileStringA, (FARPROC *)&pGetProfileStringA, (FARPROC)extGetProfileStringA},
	{HOOK_HOT_CANDIDATE, 0, "WriteProfileStringA", (FARPROC)WriteProfileStringA, (FARPROC *)&pWriteProfileStringA, (FARPROC)extWriteProfileStringA},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileIntA", (FARPROC)GetPrivateProfileIntA, (FARPROC *)&pGetPrivateProfileIntA, (FARPROC)extGetPrivateProfileIntA},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileIntW", (FARPROC)GetPrivateProfileIntW, (FARPROC *)&pGetPrivateProfileIntW, (FARPROC)extGetPrivateProfileIntW},
	{HOOK_HOT_CANDIDATE, 0, "GetProfileIntA", (FARPROC)GetProfileIntA, (FARPROC *)&pGetProfileIntA, (FARPROC)extGetProfileIntA},
	{HOOK_HOT_CANDIDATE, 0, "GetProfileIntW", (FARPROC)GetProfileIntW, (FARPROC *)&pGetProfileIntW, (FARPROC)extGetProfileIntW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type TimeHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetLocalTime", (FARPROC)GetLocalTime, (FARPROC *)&pGetLocalTime, (FARPROC)extGetLocalTime},
	{HOOK_HOT_CANDIDATE, 0, "GetSystemTime", (FARPROC)GetSystemTime, (FARPROC *)&pGetSystemTime, (FARPROC)extGetSystemTime},
	{HOOK_HOT_CANDIDATE, 0, "GetSystemTimeAsFileTime", (FARPROC)GetSystemTimeAsFileTime, (FARPROC *)&pGetSystemTimeAsFileTime, (FARPROC)extGetSystemTimeAsFileTime},
	{HOOK_HOT_CANDIDATE, 0, "Sleep", (FARPROC)Sleep, (FARPROC *)&pSleep, (FARPROC)extSleep},
	{HOOK_HOT_CANDIDATE, 0, "SleepEx", (FARPROC)SleepEx, (FARPROC *)&pSleepEx, (FARPROC)extSleepEx},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type UpTimeHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetTickCount", (FARPROC)GetTickCount, (FARPROC *)&pGetTickCount, (FARPROC)extGetTickCount},
// v2.05.75: recovered XP compatibility
//	{HOOK_HOT_CANDIDATE, 0, "GetTickCount64", (FARPROC)GetTickCount64, (FARPROC *)&pGetTickCount64, (FARPROC)extGetTickCount64},
	{HOOK_HOT_CANDIDATE, 0, "GetTickCount64", (FARPROC)NULL, (FARPROC *)&pGetTickCount64, (FARPROC)extGetTickCount64},
	{HOOK_HOT_CANDIDATE, 0, "QueryPerformanceCounter", (FARPROC)QueryPerformanceCounter, (FARPROC *)&pQueryPerformanceCounter, (FARPROC)extQueryPerformanceCounter},
	{HOOK_HOT_CANDIDATE, 0, "QueryPerformanceFrequency", (FARPROC)QueryPerformanceFrequency, (FARPROC *)&pQueryPerformanceFrequency, (FARPROC)extQueryPerformanceFrequency},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type VersionHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetVersion", (FARPROC)GetVersion, (FARPROC *)&pGetVersion, (FARPROC)extGetVersion},
	{HOOK_HOT_CANDIDATE, 0, "GetVersionExA", (FARPROC)GetVersionExA, (FARPROC *)&pGetVersionExA, (FARPROC)extGetVersionExA},
	{HOOK_HOT_CANDIDATE, 0, "GetVersionExW", (FARPROC)GetVersionExW, (FARPROC *)&pGetVersionExW, (FARPROC)extGetVersionExW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type OutDebHooks[]={
	{HOOK_HOT_REQUIRED,  0, "OutputDebugStringA", (FARPROC)OutputDebugStringA, (FARPROC *)&pOutputDebugStringA, (FARPROC)extOutputDebugStringA},
	{HOOK_HOT_REQUIRED,  0, "OutputDebugStringW", (FARPROC)OutputDebugStringW, (FARPROC *)&pOutputDebugStringW, (FARPROC)extOutputDebugStringW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type CritRegionHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "InitializeCriticalSection", (FARPROC)InitializeCriticalSection, (FARPROC *)&pInitializeCriticalSection, (FARPROC)extInitializeCriticalSection},
	{HOOK_IAT_CANDIDATE, 0, "EnterCriticalSection", (FARPROC)EnterCriticalSection, (FARPROC *)&pEnterCriticalSection, (FARPROC)extEnterCriticalSection},
	{HOOK_IAT_CANDIDATE, 0, "LeaveCriticalSection", (FARPROC)LeaveCriticalSection, (FARPROC *)&pLeaveCriticalSection, (FARPROC)extLeaveCriticalSection},
	{HOOK_IAT_CANDIDATE, 0, "DeleteCriticalSection", (FARPROC)DeleteCriticalSection, (FARPROC *)&pDeleteCriticalSection, (FARPROC)extDeleteCriticalSection},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type InternalHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileStringA", (FARPROC)GetPrivateProfileStringA, (FARPROC *)&pGetPrivateProfileStringA, (FARPROC)extGetPrivateProfileStringA},
	{HOOK_HOT_CANDIDATE, 0, "GetPrivateProfileIntA", (FARPROC)GetPrivateProfileIntA, (FARPROC *)&pGetPrivateProfileIntA, (FARPROC)extGetPrivateProfileIntA},
	{HOOK_HOT_CANDIDATE, 0, "WritePrivateProfileStringA", (FARPROC)WritePrivateProfileStringA, (FARPROC *)&pWritePrivateProfileStringA, (FARPROC)extWritePrivateProfileStringA},
	{HOOK_HOT_REQUIRED, 0, "CreateFileW", (FARPROC)CreateFileW, (FARPROC *)&pCreateFileW, (FARPROC)extCreateFileW},
	{HOOK_HOT_REQUIRED, 0, "DeleteFileW", (FARPROC)DeleteFileW, (FARPROC *)&pDeleteFileW, (FARPROC)extDeleteFileW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type NlsHooks[]={
	// IsDBCSLeadByte hook alone is enough to fix DiscStation's "Restaurant King" game ....
	{HOOK_HOT_REQUIRED,  0, "IsDBCSLeadByte", (FARPROC)IsDBCSLeadByte, (FARPROC *)&pIsDBCSLeadByte, (FARPROC)extIsDBCSLeadByte},
	{HOOK_HOT_REQUIRED,  0, "IsDBCSLeadByteEx", (FARPROC)IsDBCSLeadByteEx, (FARPROC *)NULL, (FARPROC)extIsDBCSLeadByteEx},
	{HOOK_HOT_REQUIRED,  0, "GetACP", (FARPROC)GetACP, (FARPROC *)&pGetACP, (FARPROC)extGetACP},
	{HOOK_HOT_REQUIRED,  0, "GetOEMCP", (FARPROC)GetOEMCP, (FARPROC *)&pGetOEMCP, (FARPROC)extGetOEMCP},
	{HOOK_HOT_REQUIRED,  0, "MultiByteToWideChar", (FARPROC)MultiByteToWideChar, (FARPROC *)&pMultiByteToWideChar, (FARPROC)extMultiByteToWideChar},
	{HOOK_HOT_REQUIRED,  0, "WideCharToMultiByte", (FARPROC)WideCharToMultiByte, (FARPROC *)&pWideCharToMultiByte, (FARPROC)extWideCharToMultiByte},
	{HOOK_HOT_REQUIRED,  0, "FindResourceA", (FARPROC)FindResourceA, (FARPROC *)&pFindResourceA, (FARPROC)extFindResourceA},
	{HOOK_HOT_REQUIRED,  0, "FindResourceExA", (FARPROC)FindResourceExA, (FARPROC *)&pFindResourceExA, (FARPROC)extFindResourceExA},
	{HOOK_HOT_REQUIRED,  0, "GetCPInfo", (FARPROC)GetCPInfo, (FARPROC *)&pGetCPInfo, (FARPROC)extGetCPInfo},
	{HOOK_HOT_REQUIRED,  0, "GetCPInfoExA", (FARPROC)GetCPInfoExA, (FARPROC *)&pGetCPInfoExA, (FARPROC)extGetCPInfoExA},
	{HOOK_HOT_REQUIRED,  0, "GetCPInfoExW", (FARPROC)GetCPInfoExW, (FARPROC *)&pGetCPInfoExW, (FARPROC)extGetCPInfoExW},
	// added to ntlea hooked calls !!!
	{HOOK_HOT_REQUIRED,  0, "LCMapStringA", (FARPROC)LCMapStringA, (FARPROC *)&pLCMapStringA, (FARPROC)extLCMapStringA},
	{HOOK_HOT_REQUIRED,  0, "LCMapStringW", (FARPROC)LCMapStringW, (FARPROC *)&pLCMapStringW, (FARPROC)extLCMapStringW},
	//{HOOK_HOT_REQUIRED,  0, "GetStringTypeA", (FARPROC)GetStringTypeA, (FARPROC *)&pGetStringTypeA, (FARPROC)extGetStringTypeA},
	//{HOOK_HOT_REQUIRED,  0, "GetStringTypeW", (FARPROC)GetStringTypeW, (FARPROC *)&pGetStringTypeW, (FARPROC)extGetStringTypeW},
	{HOOK_HOT_REQUIRED,  0, "GetStringTypeExA", (FARPROC)GetStringTypeExA, (FARPROC *)&pGetStringTypeExA, (FARPROC)extGetStringTypeExA},
	{HOOK_HOT_REQUIRED,  0, "GetStringTypeExW", (FARPROC)GetStringTypeExW, (FARPROC *)&pGetStringTypeExW, (FARPROC)extGetStringTypeExW},
	// all these requiring no original ptr since they are supposed to return a fixed val (is it correct?)
	{HOOK_HOT_REQUIRED,  0, "GetUserDefaultUILanguage", (FARPROC)GetUserDefaultUILanguage, (FARPROC *)NULL, (FARPROC)extGetUserDefaultUILanguage},
	{HOOK_HOT_REQUIRED,  0, "GetSystemDefaultUILanguage", (FARPROC)GetSystemDefaultUILanguage, (FARPROC *)NULL, (FARPROC)extGetSystemDefaultUILanguage},
	{HOOK_HOT_REQUIRED,  0, "GetSystemDefaultLangID", (FARPROC)GetSystemDefaultLangID, (FARPROC *)NULL, (FARPROC)extGetSystemDefaultLangID},
	{HOOK_HOT_REQUIRED,  0, "GetUserDefaultLangID", (FARPROC)GetUserDefaultLangID, (FARPROC *)NULL, (FARPROC)extGetUserDefaultLangID},
	{HOOK_HOT_REQUIRED,  0, "GetThreadLocale", (FARPROC)GetThreadLocale, (FARPROC *)NULL, (FARPROC)extGetThreadLocale},
	{HOOK_HOT_REQUIRED,  0, "SetThreadLocale", (FARPROC)SetThreadLocale, (FARPROC *)&pSetThreadLocale, (FARPROC)extSetThreadLocale},
	{HOOK_HOT_REQUIRED,  0, "GetSystemDefaultLCID", (FARPROC)GetSystemDefaultLCID, (FARPROC *)NULL, (FARPROC)extGetSystemDefaultLCID},
	{HOOK_HOT_REQUIRED,  0, "GetUserDefaultLCID", (FARPROC)GetUserDefaultLCID, (FARPROC *)NULL, (FARPROC)extGetUserDefaultLCID},
	{HOOK_HOT_REQUIRED,  0, "IsValidLocale", (FARPROC)IsValidLocale, (FARPROC *)&pIsValidLocale, (FARPROC)extIsValidLocale},
	{HOOK_HOT_REQUIRED,  0, "IsValidCodePage", (FARPROC)IsValidCodePage, (FARPROC *)&pIsValidCodePage, (FARPROC)extIsValidCodePage},
	{HOOK_IAT_CANDIDATE, 0, "EnumSystemLocalesA", (FARPROC)EnumSystemLocalesA, (FARPROC *)&pEnumSystemLocalesA, (FARPROC)extEnumSystemLocalesA},
#ifdef NLSTRACE
	{HOOK_IAT_CANDIDATE, 0, "lstrcpynA", (FARPROC)lstrcpynA, (FARPROC *)&plstrcpynA, (FARPROC)extlstrcpynA},
	{HOOK_IAT_CANDIDATE, 0, "lstrcpyA", (FARPROC)lstrcpyA, (FARPROC *)&plstrcpyA, (FARPROC)extlstrcpyA},
	{HOOK_IAT_CANDIDATE, 0, "GetLocaleInfoA", (FARPROC)GetLocaleInfoA, (FARPROC *)&pGetLocaleInfoA, (FARPROC)extGetLocaleInfoA},
	{HOOK_IAT_CANDIDATE, 0, "GetLocaleInfoW", (FARPROC)GetLocaleInfoW, (FARPROC *)&pGetLocaleInfoW, (FARPROC)extGetLocaleInfoW},
	{HOOK_HOT_REQUIRED,  0, "WriteConsoleA", (FARPROC)WriteConsoleA, (FARPROC *)NULL, (FARPROC)extWriteConsoleA},
	{HOOK_HOT_REQUIRED,  0, "WriteConsoleW", (FARPROC)WriteConsoleW, (FARPROC *)NULL, (FARPROC)extWriteConsoleW},
#endif // NLSTRACE
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

/*
missing:
GetStringTypeExW
*/

static HookEntryEx_Type AtomHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "GlobalAddAtomA", (FARPROC)GlobalAddAtomA, (FARPROC *)pGlobalAddAtomA, (FARPROC)extGlobalAddAtomA},
	{HOOK_IAT_CANDIDATE, 0, "GlobalDeleteAtom", (FARPROC)GlobalDeleteAtom, (FARPROC *)pGlobalDeleteAtom, (FARPROC)extGlobalDeleteAtom},
	{HOOK_IAT_CANDIDATE, 0, "GlobalFindAtomA", (FARPROC)GlobalFindAtomA, (FARPROC *)pGlobalFindAtomA, (FARPROC)extGlobalFindAtomA},
	{HOOK_IAT_CANDIDATE, 0, "GlobalGetAtomNameA", (FARPROC)GlobalGetAtomNameA, (FARPROC *)pGlobalGetAtomNameA, (FARPROC)extGlobalGetAtomNameA},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

#ifdef MONITORTHREADS
static HookEntryEx_Type ThreadHooks[]={
	{HOOK_HOT_REQUIRED, 0, "CreateThread", (FARPROC)CreateThread, (FARPROC *)pCreateThread, (FARPROC)extCreateThread},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};
#endif // MONITORTHREADS

static HookEntryEx_Type ThreadSelfHooks[]={
	{HOOK_HOT_REQUIRED, 0, "SetPriorityClass", (FARPROC)SetPriorityClass, (FARPROC *)&pSetPriorityClass, (FARPROC)extSetPriorityClass},
	{HOOK_HOT_REQUIRED, 0, "SetThreadPriority", (FARPROC)SetThreadPriority, (FARPROC *)&pSetThreadPriority, (FARPROC)extSetThreadPriority},
	{HOOK_HOT_REQUIRED, 0, "SuspendThread", (FARPROC)SuspendThread, (FARPROC *)&pSuspendThread, (FARPROC)extSuspendThread},
	{HOOK_HOT_REQUIRED, 0, "ResumeThread", (FARPROC)ResumeThread, (FARPROC *)&pResumeThread, (FARPROC)extResumeThread},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type EmuDDSyncHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "WaitForSingleObject", (FARPROC)WaitForSingleObject, (FARPROC *)pWaitForSingleObject, (FARPROC)extWaitForSingleObject},
	{HOOK_IAT_CANDIDATE, 0, "ReleaseMutex", (FARPROC)ReleaseMutex, (FARPROC *)pReleaseMutex, (FARPROC)extReleaseMutex},
	{HOOK_IAT_CANDIDATE, 0, "CloseHandle", (FARPROC)CloseHandle, (FARPROC *)pCloseHandle, (FARPROC)extCloseHandle},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

// v2.06.09: dummy exports to please GetProcAddress. Tested with @#@ "RoadRash" about ThunkConnect32
void _stdcall dummyMapSL(LONG p1) { OutTrace("dummyMapSL\n"); };
void _stdcall dummysMapLS() { OutTrace("dummysMapLS\n"); };
BOOL _stdcall dummyThunkConnect32(LPVOID p1, LPVOID p2, LPVOID p3, LPVOID p4, LPVOID p5, LPVOID p6) 
{ 
	OutTrace("dummyThunkConnect32\n"); 
	return FALSE; 
};

static HookEntryEx_Type LegacyHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "MapSL", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummyMapSL},
	{HOOK_IAT_CANDIDATE, 0, "SMapLS", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "SMapLS_IP_EBP_8", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "SMapLS_IP_EBP_12", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "SMapLS_IP_EBP_16", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "SUnMapLS", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "SUnMapLS_IP_EBP_8", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "SUnMapLS_IP_EBP_12", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "SUnMapLS_IP_EBP_16", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "FT_Exit8", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "FT_Exit12", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "FT_Thunk", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummysMapLS},
	{HOOK_IAT_CANDIDATE, 0, "ThunkConnect32", (FARPROC)NULL, (FARPROC *)NULL, (FARPROC)dummyThunkConnect32},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static char *libname = "kernel32.dll";
//static char *libname = "kernelbase.dll";

void HookLauncher(HMODULE module)
{
	HookLibraryEx(module, LauncherHooks, libname);
}

void HookKernelModule(HMODULE module, char *libname)
{
	HookLibraryEx(module, Hooks, libname);
#ifndef DXW_NOTRACES
	if( (dxw.dwFlags3 & BUFFEREDIOFIX) || 
		(dxw.dwFlags20 & HANDLECDLOCK) ||
		(dxw.dwTFlags2 & OUTFILEIO))
		HookLibraryEx(module, FixIOHooks, libname);
	if( (dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) || 
		(dxw.dwFlags11 & CUSTOMLOCALE) ||
		(dxw.dwTFlags2 & OUTFILEIO)) HookLibraryEx(module, RemapIOHooks, libname);
	if(dxw.dwTFlags2 & OUTFILEIO) HookLibraryEx(module, TraceIOHooks, libname);
#else
	if( (dxw.dwFlags3 & BUFFEREDIOFIX) || 
		(dxw.dwFlags20 & HANDLECDLOCK)) HookLibraryEx(module, FixIOHooks, libname);
	if( (dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) || 
		(dxw.dwFlags11 & CUSTOMLOCALE)) HookLibraryEx(module, RemapIOHooks, libname);
#endif // DXW_NOTRACES
	if(dxw.dwFlags14 & WIN16CREATEFILE) HookLibraryEx(module, FixW16Hooks, libname);
	if((dxw.dwFlags2 & LIMITRESOURCES) || (dxw.dwFlags15 & (LIMITFREEDISK|LIMITFREERAM))) 
		HookLibraryEx(module, LimitHooks, libname);
	if(dxw.dwFlags2 & TIMESTRETCH) HookLibraryEx(module, TimeHooks, libname);
	if((dxw.dwFlags2 & TIMESTRETCH) || (dxw.dwFlags14 & (UPTIMECLEAR | UPTIMESTRESS)))
		HookLibraryEx(module, UpTimeHooks, libname);
	if(dxw.dwFlags2 & FAKEVERSION) HookLibraryEx(module, VersionHooks, libname);
#ifdef TRACEHEAP
	HookLibraryEx(module, AllocHooks, libname);
	HookLibraryEx(module, HeapHooks, libname);
#else
	if(dxw.dwFlags6 & LEGACYALLOC) HookLibraryEx(module, AllocHooks, libname);
	if( (dxw.dwFlags8 & VIRTUALHEAP) || 
		(dxw.dwFlags14 & SAFEHEAP)) HookLibraryEx(module, HeapHooks, libname);
#endif // TRACEHEAP
	if(dxw.dwFlags4 & IGNOREDEBOUTPUT) HookLibraryEx(module, OutDebHooks, libname);
	if(dxw.dwFlags11 & (MUTEX4CRITSECTION|DELAYCRITSECTION)) HookLibraryEx(module, CritRegionHooks, libname);
	if(dxw.dwFlags11 & REMAPSYSFOLDERS) HookLibraryEx(module, SysFolderHooks, libname);
	if(dxw.dwFlags11 & CUSTOMLOCALE) HookLibraryEx(module, NlsHooks, libname); 
	if(dxw.dwFlags12 & FAKEGLOBALATOM) HookLibraryEx(module, AtomHooks, libname);
	if(dxw.dwFlags12 & KILLDEADLOCKS) HookLibraryEx(module, WaitHooks, libname);
	if(dxw.dwFlags15 & IGNORESCHEDULER) HookLibraryEx(module, ThreadSelfHooks, libname);
	if(dxw.dwFlags17 & FIXOVERLAPPEDRESULT) HookLibraryEx(module, OverlappedHooks, libname);;
	if(dxw.dwFlags18 & EMUDDSYNCSHIM) HookLibraryEx(module, EmuDDSyncHooks, libname);;

#ifdef MONITORTHREADS
	HookLibraryEx(module, ThreadHooks, libname);
#endif // MONITORTHREADS
}

void HookKernel32Init()
{
	HookLibInitEx(Hooks);
	HookLibInitEx(LauncherHooks);
	HookLibInitEx(WaitHooks);
	HookLibInitEx(HeapHooks);
	HookLibInitEx(AllocHooks);
	HookLibInitEx(FixIOHooks);
	HookLibInitEx(RemapIOHooks);
	HookLibInitEx(LimitHooks);
	HookLibInitEx(SysFolderHooks);
	HookLibInitEx(TimeHooks);
	HookLibInitEx(UpTimeHooks);
	HookLibInitEx(VersionHooks);
	HookLibInitEx(OutDebHooks);
	HookLibInitEx(CritRegionHooks);
	HookLibInitEx(InternalHooks); // for DxWnd internal use
	HookLibInitEx(NlsHooks);
	HookLibInitEx(AtomHooks); 
	HookLibInitEx(ThreadSelfHooks); 
	HookLibInitEx(EmuDDSyncHooks);
}

FARPROC Remap_kernel32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;

	if (dxw.dwFlags4 & NOPERFCOUNTER){
		if( !strcmp(proc, "QueryPerformanceCounter") ||
			!strcmp(proc, "QueryPerformanceFrequency")){
				OutTraceDW("GetProcAddress: HIDING proc=%s\n", proc);
			return NULL;
		}
	}
			
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;

#ifndef DXW_NOTRACES
	if((dxw.dwFlags3 & BUFFEREDIOFIX) || (dxw.dwFlags20 & HANDLECDLOCK) || (dxw.dwTFlags2 & OUTFILEIO))
		if (addr=RemapLibraryEx(proc, hModule, FixIOHooks)) return addr;

	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) || (dxw.dwFlags11 & CUSTOMLOCALE) || (dxw.dwTFlags2 & OUTFILEIO))
		if (addr=RemapLibraryEx(proc, hModule, RemapIOHooks)) return addr;

	if (dxw.dwTFlags2 & OUTFILEIO)
		if (addr=RemapLibraryEx(proc, hModule, TraceIOHooks)) return addr;
#else
	if(dxw.dwFlags3 & BUFFEREDIOFIX)
		if (addr=RemapLibraryEx(proc, hModule, FixIOHooks)) return addr;

	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) || (dxw.dwFlags11 & CUSTOMLOCALE))
		if (addr=RemapLibraryEx(proc, hModule, RemapIOHooks)) return addr;
#endif // DXW_NOTRACES

	if(dxw.dwFlags14 & WIN16CREATEFILE)
		if (addr=RemapLibraryEx(proc, hModule, FixW16Hooks)) return addr;

	if((dxw.dwFlags2 & LIMITRESOURCES) || (dxw.dwFlags15 & (LIMITFREEDISK|LIMITFREERAM)))
		if (addr=RemapLibraryEx(proc, hModule, LimitHooks)) return addr;

	if(dxw.dwFlags2 & TIMESTRETCH)
		if (addr=RemapLibraryEx(proc, hModule, TimeHooks)) return addr;

	if((dxw.dwFlags2 & TIMESTRETCH) || 
		(dxw.dwFlags14 & (UPTIMECLEAR | UPTIMESTRESS)))
		if (addr=RemapLibraryEx(proc, hModule, UpTimeHooks)) return addr;

	if(dxw.dwFlags2 & FAKEVERSION)
		if (addr=RemapLibraryEx(proc, hModule, VersionHooks)) return addr;

#ifdef TRACEHEAP
	if (addr=RemapLibraryEx(proc, hModule, AllocHooks)) return addr;
	if (addr=RemapLibraryEx(proc, hModule, HeapHooks)) return addr;
#else
	if(dxw.dwFlags6 & LEGACYALLOC)
		if (addr=RemapLibraryEx(proc, hModule, AllocHooks)) return addr;

	if((dxw.dwFlags8 & VIRTUALHEAP) || (dxw.dwFlags14 & SAFEHEAP))
		if (addr=RemapLibraryEx(proc, hModule, HeapHooks)) return addr;
#endif

	if(dxw.dwFlags4 & IGNOREDEBOUTPUT) 
		if (addr=RemapLibraryEx(proc, hModule, OutDebHooks)) return addr;

	if(dxw.dwFlags11 & (MUTEX4CRITSECTION|DELAYCRITSECTION)) 
		if (addr=RemapLibraryEx(proc, hModule, CritRegionHooks)) return addr;

	if(dxw.dwFlags11 & REMAPSYSFOLDERS) 
		if (addr=RemapLibraryEx(proc, hModule, SysFolderHooks)) return addr;

	if(dxw.dwFlags12 & FAKEGLOBALATOM) 
		if (addr=RemapLibraryEx(proc, hModule, AtomHooks)) return addr;

	if(dxw.dwFlags12 & KILLDEADLOCKS) 
		if (addr=RemapLibraryEx(proc, hModule, WaitHooks)) return addr;

	if(dxw.dwFlags15 & IGNORESCHEDULER)
		if (addr=RemapLibraryEx(proc, hModule, ThreadSelfHooks)) return addr;

	if(dxw.dwFlags17 & FIXOVERLAPPEDRESULT) 
		if (addr=RemapLibraryEx(proc, hModule, OverlappedHooks)) return addr;

	if(dxw.dwFlags18 & EMUDDSYNCSHIM) 
		if (addr=RemapLibraryEx(proc, hModule, EmuDDSyncHooks)) return addr;

#ifdef MONITORTHREADS
	if (addr=RemapLibraryEx(proc, hModule, ThreadHooks)) return addr;
#endif // MONITORTHREADS

	if(dxw.dwFlags19 & LEGACYKERNEL32) 
		if (addr=RemapLibraryEx(proc, hModule, LegacyHooks)) return addr;

	return NULL;
}

void HookKernel32(HMODULE module)
{
	OSVERSIONINFO osinfo;
	if(!pGetVersionExA) pGetVersionExA = GetVersionExA;
	(*pGetVersionExA)(&osinfo);
	if(osinfo.dwMajorVersion >= 6) HookKernelModule(module, "kernelbase.dll");
	HookKernelModule(module, "kernel32.dll");
}

/* ------------------------------------------------------------------------------ */
// auxiliary & static functions
/* ------------------------------------------------------------------------------ */

// v2.05.53: initialization of folder type for games that are supposed to start from CD folder
// allows a DxWnd-RIP for "Return Fire"
DWORD InitFolderType(void)
{
	char sFakePath[MAX_PATH+1];
	char sWorkPath[MAX_PATH+1];
	int MaxCount;
	//if(!pGetCurrentDirectoryA) pGetCurrentDirectoryA = GetCurrentDirectoryA;
	(*pGetCurrentDirectoryA)(MAX_PATH, sWorkPath);
	for(UINT i=0; i<strlen(sWorkPath); i++) sWorkPath[i]=toupper(sWorkPath[i]);
	OutTraceDW("InitFolderType: WorkPath=%s\n", sWorkPath);
	if(dxw.dwFlags10 & FAKECDDRIVE){
		strcpy(sFakePath, dxwGetFakeDriverPath(DXW_CD_PATH));
		for(UINT i=0; i<strlen(sFakePath); i++) sFakePath[i]=toupper(sFakePath[i]);
		MaxCount = strlen(sFakePath);
		if(!strncmp(sFakePath, sWorkPath, MaxCount)) {
			OutTraceDW("InitFolderType: matched FAKE_CD path=%s\n", sFakePath);
			return DXW_FAKE_CD;
		}
	}
	if(dxw.dwFlags10 & FAKEHDDRIVE){
		strcpy(sFakePath, dxwGetFakeDriverPath(DXW_HD_PATH));
		for(UINT i=0; i<strlen(sFakePath); i++) sFakePath[i]=toupper(sFakePath[i]);
		OutTraceDW("InitFolderType: fake HD path=%s\n", sFakePath);
		MaxCount = strlen(sFakePath);
		if(!strncmp(sFakePath, sWorkPath, MaxCount)) {
			OutTraceDW("InitFolderType: matched FAKE_HD path=%s\n", sFakePath);
			return DXW_FAKE_HD;
		}
	}
	OutTraceDW("InitFolderType: matched NO_FAKE\n");
	return DXW_NO_FAKE;
}

static DWORD GetFakeAttribute(DWORD attribute)
{
	// to do: check more cases ...
/*
#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_DEVICE               0x00000040  
#define FILE_ATTRIBUTE_NORMAL               0x00000080  
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100  
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800  
#define FILE_ATTRIBUTE_OFFLINE              0x00001000  
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000  
#define FILE_ATTRIBUTE_VIRTUAL              0x00010000  
*/
	DWORD fake = attribute;
	if(fake & FILE_ATTRIBUTE_DIRECTORY) {
		fake = FILE_ATTRIBUTE_DIRECTORY;
		if (dxw.dwFlags15 & EMULATEWIN95) fake = FILE_ATTRIBUTE_DIRECTORY;
		else fake = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY;
	}
	else {
		fake |= FILE_ATTRIBUTE_READONLY;
	}
	
	return fake;
}

char *dxwGetCDVolumeName()
{
	char InitPath[MAX_PATH+1];
	char key[20];
	static char s_CDLabel[256+1];
	//if(dxw.dwFlags11 & SLOWDEVPOLLING) Sleep(200);
	Sleep(100);
	sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
	sprintf_s(key, sizeof(key), "fakecdlabel%i", (dxw.dwIndex == -1) ? 0 : dxw.dwIndex); // v2.05.23 fix for proxy 
	//OutTrace("dxwGetCDVolumeName: key=%s\n", key);
	(*pGetPrivateProfileStringA)("target", key, "", s_CDLabel, 256, InitPath);
	OutDebugSYS("CD label path=%s key=%s val=\"%s\"\n", InitPath, key, s_CDLabel);
	// if there's the ';' separator, returns the n-th label value only
	if(strchr(s_CDLabel, ';')){
		char *pBegin, *pEnd;
		int cdindex = GetHookInfo()->CDIndex;
		pBegin = s_CDLabel;
		pEnd = strchr(pBegin, ';');
		if(pEnd) *pEnd = 0;
		for(int i=0; i<cdindex; i++){
			pBegin = pEnd+1;
			pEnd = strchr(pBegin, ';');
			if(pEnd) *pEnd = 0;
		}
		OutDebugSYS("CD%d label=\"%s\"\n", cdindex + 1, pBegin);
		return pBegin;
	}
	else {
		OutDebugSYS("CD label=\"%s\"\n", s_CDLabel);
		return s_CDLabel;
	}
}

// --- EmulateDirectDrawSync (EMUDDSYNCSHIM) support variables

// Enum used to tell our thread what to do
enum {sNone, sWaitForSingleObject, sReleaseMutex};

// Events we use to signal our thread to do work and wait until its done
HANDLE g_hWaitEvent;
HANDLE g_hDoneEvent;
HANDLE g_hThread = NULL;

//
// Parameters that are passed between the caller thread and our thread
// Access is synchronized with a critical section
//

CRITICAL_SECTION g_csSync;
DWORD g_dwWait;
DWORD g_dwWaitRetValue;
DWORD g_dwTime;
BOOL g_bRetValue;

// Store the DirectDraw mutex handle
HANDLE g_hDDMutex = 0;

// Thread tracking data so we can identify degenerate cases
DWORD g_dwMutexOwnerThreadId = 0;

// Find the DirectDraw mutex
DWORD g_dwFindMutexThread = 0;

BOOL FindMutex()
{
	ApiName("FindMutex");
    typedef VOID (WINAPI *_pfn_GetOLEThunkData)(ULONG_PTR dwOrdinal);
    HMODULE hMod;
    _pfn_GetOLEThunkData pfnGetOLEThunkData;

    hMod = GetModuleHandleA("ddraw");
    if (!hMod) {
		OutTraceDW("%s: DirectDraw not loaded\n", ApiRef);
        return FALSE;
    }

    pfnGetOLEThunkData = (_pfn_GetOLEThunkData) (*pGetProcAddress)(hMod, "GetOLEThunkData");
    if (!pfnGetOLEThunkData) {
		OutTraceDW("%s: Failed to get GetOLEThunkData API\n", ApiRef);
        return FALSE;
    }

    //
    // Now we plan to go and find the mutex by getting Ddraw to call 
    // ReleaseMutex.
    //

    EnterCriticalSection(&g_csSync); 

    //
    // Set the mutex to the current thread so it can be picked up in the 
    // ReleaseMutex hook
    //

    g_dwFindMutexThread = GetCurrentThreadId();

    //
    // Call to the hard-coded (in ddraw) ReleaseMutex hack which releases the 
    // mutex
    //

    pfnGetOLEThunkData(6);
    g_dwFindMutexThread = 0;
    LeaveCriticalSection(&g_csSync);
    return (g_hDDMutex != 0);
}

VOID WINAPI ThreadSyncMutex(LPVOID lpParameter)
{
	if(!pReleaseMutex) pReleaseMutex = ReleaseMutex;
	if(!pWaitForSingleObject) pWaitForSingleObject = WaitForSingleObject;
    while (1) {
        // Wait until we need to acquire or release the mutex
        (*pWaitForSingleObject)(g_hWaitEvent, INFINITE);
        
        if (g_dwWait == sWaitForSingleObject) {
            // WaitForSingleobject() has been called on the Mutex object 
            g_dwWaitRetValue = (*pWaitForSingleObject)(g_hDDMutex, g_dwTime);
        }  
        else if (g_dwWait == sReleaseMutex) {
            // ReleaseMutex has been called
            g_bRetValue = (*pReleaseMutex)(g_hDDMutex);
        }

        g_dwWait = sNone;

        ResetEvent(g_hWaitEvent);

        SetEvent(g_hDoneEvent);
    }
}

BOOL ddNotifyFunction()
{
	static BOOL initialized = FALSE;
	if(initialized) return TRUE;
	initialized = TRUE;
    //
    // We need the critical section all the time
    //
    InitializeCriticalSection(&g_csSync);
    //
    // Create Events that will be used for the thread synchronization, i.e 
    // to synchronize this thread and the one we will be creating ahead. We 
    // don't clean these up by design. We have to do this stuff here, rather
    // than in the process attach, since OpenGL apps and others do DirectX 
    // stuff in their dllmains.
    //
    g_hWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hWaitEvent) {
        OutTraceDW("Failed to create Event 1\n");
        return FALSE;
    }

    g_hDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hDoneEvent) {
        OutTraceDW("Failed to create Event 2\n");
        return FALSE;
    }

    // Create our thread
    g_hThread = CreateThread(NULL, 0, 
        (LPTHREAD_START_ROUTINE) ThreadSyncMutex, NULL, 0, 
        NULL);

    if (!g_hThread) {
        OutTraceDW("Failed to create Thread\n");
        return FALSE;
    }

    return TRUE;
}

/* ------------------------------------------------------------------------------ */
// kernel32.dll wrappers
/* ------------------------------------------------------------------------------ */

int WINAPI extIsDebuggerPresent(void)
{
	OutTraceDW("extIsDebuggerPresent: return FALSE\n");
	return FALSE;
}

static BOOL fixFakeSpace(ApiArg, char ansi_drive, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters)
{
	if((dxw.dwFlags10 & FAKECDDRIVE) && (ansi_drive == dxw.FakeCDDrive)){
		OutTraceDW("%s: fake CD detected\n", ApiRef);
		// copied from "Need for Speed 2" CD
		if(lpSectorsPerCluster)		*lpSectorsPerCluster = 1;
		if(lpBytesPerSector)		*lpBytesPerSector = 2048;
		if(lpNumberOfFreeClusters)	*lpNumberOfFreeClusters = 0;
		if(lpTotalNumberOfClusters)	*lpTotalNumberOfClusters = 266547;
		if(dxw.dwFlags17 & CDHACK){
			// v2.06.04: handling different geometry values
			OutTraceSYS("%s: CDHACK reading cdinfo.txt [geometry CD%d] values\n", ApiRef, dxw.DataCDIndex + 1);
			char room[20];
			strcpy(room, "geometry");
			dxw.DataCDIndex = GetHookInfo()->CDIndex;
			if(dxw.DataCDIndex > 0) sprintf(room, "geometry%d", dxw.DataCDIndex + 1);
			char *cdhack = ".\\cdinfo.txt";
			if(lpSectorsPerCluster) {
				*lpSectorsPerCluster = (*pGetPrivateProfileIntA)(room, "SectorsPerCluster", 1, cdhack);
				if(*lpSectorsPerCluster != 1) OutTraceDW("%s: fixed SectorsPerCluster=%d\n", ApiRef, *lpSectorsPerCluster);
			}
			if(lpBytesPerSector) {
				*lpBytesPerSector = (*pGetPrivateProfileIntA)(room, "BytesPerSector", 2048, cdhack);
				if(*lpBytesPerSector != 2048) OutTraceDW("%s: fixed BytesPerSector=%d\n", ApiRef, *lpBytesPerSector);
			}
			if(lpNumberOfFreeClusters) {
				*lpNumberOfFreeClusters = (*pGetPrivateProfileIntA)(room, "NumberOfFreeClusters", 0, cdhack);
				if(*lpNumberOfFreeClusters != 0) OutTraceDW("%s: fixed NumberOfFreeClusters=%d\n", ApiRef, *lpNumberOfFreeClusters);
			}
			if(lpTotalNumberOfClusters) {
				*lpTotalNumberOfClusters = (*pGetPrivateProfileIntA)(room, "TotalNumbrOfClusters", 266547, cdhack);
				if(*lpTotalNumberOfClusters != 266547) OutTraceDW("%s: fixed TotalNumberOfClusters=%d\n", ApiRef, *lpTotalNumberOfClusters);
			}
		}
		return TRUE;
	}
	if((dxw.dwFlags10 & FAKEHDDRIVE) && (ansi_drive == dxw.FakeHDDrive)){
		OutTraceDW("%s: fake HD detected\n", ApiRef);
		// copied from maximum limited resources
		if(lpSectorsPerCluster)		*lpSectorsPerCluster = 8;
		if(lpBytesPerSector)		*lpBytesPerSector = 512;
		if(lpNumberOfFreeClusters)	*lpNumberOfFreeClusters = 24414;
		if(lpTotalNumberOfClusters)	*lpTotalNumberOfClusters = 29296;
		return TRUE;
	}
	return FALSE;
}

static VOID fixLimitSpace(ApiArg, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters)
{
	if(dxw.EmulateWin95){
		OutTraceDW("%s: setting Win95 defaults\n", ApiRef);
		*lpSectorsPerCluster     = 0x00000040;
		*lpBytesPerSector        = 0x00000200;
		*lpNumberOfFreeClusters  = 0x0000FFF6;
		*lpTotalNumberOfClusters = 0x0000FFF6;
	}
	else {
		DWORD dwFreeBytes;
		DWORD BytesXCluster = *lpBytesPerSector * *lpSectorsPerCluster;
		char sInitPath[MAX_PATH+1];
		sprintf(sInitPath, "%sdxwnd.ini", GetDxWndPath()); 
		dwFreeBytes = GetPrivateProfileInt("window", "freedisk", 0, sInitPath);
		if(dwFreeBytes) {
			// try to define dwFreeBytes of free space in a hard disk 20% bigger
			if(BytesXCluster){
				*lpNumberOfFreeClusters = dwFreeBytes / BytesXCluster;
				//*lpTotalNumberOfClusters = ((dwFreeBytes / BytesXCluster) * 12) / 10;
				// rewritten to avoid integer overflow in the 20% increment
				*lpTotalNumberOfClusters = (*lpNumberOfFreeClusters * 6) / 5;
				OutTraceDW("%s: FIXED SectXCluster=%d BytesXSect=%d FreeClusters=%d TotalClusters=%d\n", ApiRef,
					*lpSectorsPerCluster, *lpBytesPerSector, *lpNumberOfFreeClusters, *lpTotalNumberOfClusters);
			}
		}
		else {
			// v2.06.05: @#@ "Cossacks" requires more than 100 MB free
			// try to define 120MB of free space in a 160MB hard disk
			if(BytesXCluster){
				*lpNumberOfFreeClusters = 120000000 / BytesXCluster;
				*lpTotalNumberOfClusters = 160000000 / BytesXCluster;
				OutTraceDW("%s: FIXED SectXCluster=%d BytesXSect=%d FreeClusters=%d TotalClusters=%d\n", ApiRef,
					*lpSectorsPerCluster, *lpBytesPerSector, *lpNumberOfFreeClusters, *lpTotalNumberOfClusters);
			}
		}
	}
}

BOOL WINAPI extGetDiskFreeSpaceA(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters)
{
	ApiName("GetDiskFreeSpaceA");
	BOOL ret;
	OutTraceSYS("%s: RootPathName=\"%s\"\n", ApiRef, lpRootPathName);
	ret=(*pGetDiskFreeSpaceA)(lpRootPathName, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
	if(ret) {
		OutTraceSYS("%s: SectXCluster=%d BytesXSect=%d FreeClusters=%d TotalClusters=%d\n", ApiRef,
			*lpSectorsPerCluster, *lpBytesPerSector, *lpNumberOfFreeClusters, *lpTotalNumberOfClusters);
	}
	else {
		TraceError();
	}

	if(ret && lpTotalNumberOfClusters && (lpNumberOfFreeClusters == 0)){
		// No free clusters probably means a CD/DVD drive (or maybe that you have a serious
		// problem with your HD!) so in this case it's better leave the values unchanged.
		OutTraceDW("%s: CD/DVD detected\n", ApiRef);
		return ret;
	}

	// v2.05.09 fix: GetDiskFreeSpace can accept a NULL value for lpRootPathName
	// to tell the root of the current disk. Found in "Byzantine the Betrayal".
	// t.b.d: handle case where the current disk is a fake device!
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpRootPathName){
		// v2.05.54: more strict and correct check: avoid mixing fake HD and CD logics
		char ansi_drive = toupper(lpRootPathName[0]);
		if(fixFakeSpace(ApiRef, ansi_drive, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters)){
			return TRUE;
		}
	}

	if(ret && ((dxw.dwFlags2 & LIMITRESOURCES) || (dxw.dwFlags15 & LIMITFREEDISK))){
		fixLimitSpace(ApiRef, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
		OutTraceSYS("%s: LIMIT SectXCluster=%d BytesXSect=%d FreeClusters=%d TotalClusters=%d\n", ApiRef,
			*lpSectorsPerCluster, *lpBytesPerSector, *lpNumberOfFreeClusters, *lpTotalNumberOfClusters);
	}
	return ret;
}

BOOL WINAPI extGetDiskFreeSpaceW(LPCWSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters)
{
	ApiName("GetDiskFreeSpaceW");
	BOOL ret;
	OutTraceSYS("%s: RootPathName=\"%ls\"\n", ApiRef, lpRootPathName);
	ret=(*pGetDiskFreeSpaceW)(lpRootPathName, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
	if(ret) {
		OutTraceSYS("%s: SectXCluster=%d BytesXSect=%d FreeClusters=%d TotalClusters=%d\n", ApiRef,
			*lpSectorsPerCluster, *lpBytesPerSector, *lpNumberOfFreeClusters, *lpTotalNumberOfClusters);
	}
	else {
		TraceError();
	}

	// v2.05.09 fix: GetDiskFreeSpace can accept a NULL value for lpRootPathName
	// to tell the root of the current disk. Found in "Byzantine the Betrayal".
	// t.b.d: handle case where the current disk is a fake device!
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpRootPathName){
		// beware: this is NOT a generic WIDE to ANSI conversion, but takes advantage that drive letters
		// can only be letters from "A" to "Z", so ANSI drive letter should be the first byte of the WIDE array
		char ansi_drive = toupper(((LPBYTE)lpRootPathName)[0]);
		// v2.05.54: more strict and correct check: avoid mixing fake HD and CD logics
		if(fixFakeSpace(ApiRef, ansi_drive, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters)){
			OutTraceSYS("%s: HACK SectXCluster=%d BytesXSect=%d FreeClusters=%d TotalClusters=%d\n", ApiRef,
				*lpSectorsPerCluster, *lpBytesPerSector, *lpNumberOfFreeClusters, *lpTotalNumberOfClusters);
			return TRUE;
		}
	}

	if(ret && ((dxw.dwFlags2 & LIMITRESOURCES) || (dxw.dwFlags15 & LIMITFREEDISK))){
		fixLimitSpace(ApiRef, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
		OutTraceSYS("%s: LIMIT SectXCluster=%d BytesXSect=%d FreeClusters=%d TotalClusters=%d\n", ApiRef,
			*lpSectorsPerCluster, *lpBytesPerSector, *lpNumberOfFreeClusters, *lpTotalNumberOfClusters);
	}
	return ret;
}

static BOOL fixSpaceEx(ApiArg, char ansi_drive, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
	if((dxw.dwFlags10 & FAKECDDRIVE) && (ansi_drive == dxw.FakeCDDrive)){
		OutTraceDW("%s: fake CD detected\n", ApiRef);
		// copied from "Need for Speed 2" CD
		if(lpFreeBytesAvailableToCaller) (*lpFreeBytesAvailableToCaller).QuadPart = 0LL;
		if(lpTotalNumberOfBytes) (*lpTotalNumberOfBytes).QuadPart = 683636736LL;
		if(lpTotalNumberOfFreeBytes) (*lpTotalNumberOfFreeBytes).QuadPart = 0LL;
		OutTraceDW("%s: FAKECD FreeAvailToCaller=%lld Total=%lld TotalFree=%lld\n", ApiRef,
			lpFreeBytesAvailableToCaller ? (*lpFreeBytesAvailableToCaller).QuadPart : 0LL, 
			lpTotalNumberOfBytes ? (*lpTotalNumberOfBytes).QuadPart : 0LL, 
			lpTotalNumberOfFreeBytes ? (*lpTotalNumberOfFreeBytes).QuadPart : 0LL);
		return TRUE;
	}
	if((dxw.dwFlags10 & FAKEHDDRIVE) && (ansi_drive == dxw.FakeHDDrive)){
		OutTraceDW("%s: fake HD detected\n", ApiRef);
		// copied from maximum limited resources
		if(lpFreeBytesAvailableToCaller) (*lpFreeBytesAvailableToCaller).QuadPart = 10000000000LL;
		if(lpTotalNumberOfBytes) (*lpTotalNumberOfBytes).QuadPart = 20000000000LL;
		if(lpTotalNumberOfFreeBytes) (*lpTotalNumberOfFreeBytes).QuadPart = 10000000000LL;
		OutTraceDW("%s: FAKEHD FreeAvailToCaller=%lld Total=%lld TotalFree=%lld\n", ApiRef,
			lpFreeBytesAvailableToCaller ? (*lpFreeBytesAvailableToCaller).QuadPart : 0LL, 
			lpTotalNumberOfBytes ? (*lpTotalNumberOfBytes).QuadPart : 0LL, 
			lpTotalNumberOfFreeBytes ? (*lpTotalNumberOfFreeBytes).QuadPart : 0LL);
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI extGetDiskFreeSpaceExA(LPCSTR lpRootPathName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
	ApiName("GetDiskFreeSpaceExA");
	BOOL ret;
	OutTraceSYS("%s: RootPathName=\"%s\"\n", ApiRef, lpRootPathName);
	ret=(*pGetDiskFreeSpaceExA)(lpRootPathName, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
	if(ret) {
		OutTraceSYS("%s: FreeAvailToCaller=%lld Total=%lld TotalFree=%lld\n", ApiRef,
			lpFreeBytesAvailableToCaller ? (*lpFreeBytesAvailableToCaller).QuadPart : 0LL, 
			lpTotalNumberOfBytes ? (*lpTotalNumberOfBytes).QuadPart : 0LL, 
			lpTotalNumberOfFreeBytes ? (*lpTotalNumberOfFreeBytes).QuadPart : 0LL);
	}
	else {
		TraceError();
	}

	if(ret && lpFreeBytesAvailableToCaller && ((*lpFreeBytesAvailableToCaller).QuadPart == 0LL)){
		// No free bytes probably means a CD/DVD drive (or maybe that you have a serious
		// problem with your HD!) so in this case it's better leave the values unchanged.
		OutTraceDW("%s: CD/DVD detected\n", ApiRef);
		return ret;
	}

	// v2.05.09 fix: GetDiskFreeSpace can accept a NULL value for lpRootPathName
	// to tell the root of the current disk. Found in "Byzantine the Betrayal".
	// t.b.d: handle case where the current disk is a fake device!
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpRootPathName){
		// v2.05.54: more strict and correct check: avoid mixing fake HD and CD logics
		char ansi_drive = toupper(lpRootPathName[0]);
		if (fixSpaceEx(ApiRef, ansi_drive, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes)) 
			return TRUE;
	}

	return ret;
}

BOOL WINAPI extGetDiskFreeSpaceExW(LPCWSTR lpRootPathName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
	ApiName("GetDiskFreeSpaceExW");
	BOOL ret;
	OutTraceSYS("%s: RootPathName=\"%ls\"\n", ApiRef, lpRootPathName);
	ret=(*pGetDiskFreeSpaceExW)(lpRootPathName, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
	if(ret) {
		OutTraceSYS("%s: FreeAvailToCaller=%lld Total=%lld TotalFree=%lld\n", ApiRef,
			lpFreeBytesAvailableToCaller ? (*lpFreeBytesAvailableToCaller).QuadPart : 0LL, 
			lpTotalNumberOfBytes ? (*lpTotalNumberOfBytes).QuadPart : 0LL, 
			lpTotalNumberOfFreeBytes ? (*lpTotalNumberOfFreeBytes).QuadPart : 0LL);
	}
	else {
		TraceError();
	}

	if(ret && lpFreeBytesAvailableToCaller && ((*lpFreeBytesAvailableToCaller).QuadPart == 0LL)){
		// No free bytes probably means a CD/DVD drive (or maybe that you have a serious
		// problem with your HD!) so in this case it's better leave the values unchanged.
		OutTraceDW("%s: CD/DVD detected\n", ApiRef);
		return ret;
	}

	// v2.05.09 fix: GetDiskFreeSpace can accept a NULL value for lpRootPathName
	// to tell the root of the current disk. Found in "Byzantine the Betrayal".
	// t.b.d: handle case where the current disk is a fake device!
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpRootPathName){
		// beware: this is NOT a generic WIDE to ANSI conversion, but takes advantage that drive letters
		// can only be letters from "A" to "Z", so ANSI drive letter should be the first byte of the WIDE array
		char ansi_drive = toupper(((LPBYTE)lpRootPathName)[0]);
		if (fixSpaceEx(ApiRef, ansi_drive, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes)) 
			return TRUE;
	}

	return ret;
}

/* -------------------------------------------------------------------------------

GlobalMemoryStatus: MSDN documents that on modern PCs that have more than DWORD
memory values the GlobalMemoryStatus sets the fields to -1 (0xFFFFFFFF) and you 
should use GlobalMemoryStatusEx instead. 
But in some cases the value is less that DWORD max, but greater that DWORD>>1, that
is the calling application may get a big value and see it as a signed negative
value, as it happened to Nocturne on my PC. That's why it's not adviseable to write: 
if(lpBuffer->dwTotalPhys== -1) ...
but this way:
if ((int)lpBuffer->dwTotalPhys < 0) ...
and also don't set 
BIGENOUGH 0x80000000 // possibly negative!!!
but:
BIGENOUGH 0x60000000 // surely positive !!!

v2.03.08: the "Jeff Gordon XS Racing demo" adds the dwAvailPhys to the dwAvailPageFile
value, so that the sum is negative. To avoid this, all available memory values are
divided by 2 (HALFBIG macro).
v2.05.59: oddly, the game "Master od Dimensions" doesn't like anything above 0x04000000
but this is big enough that it doesn't require a new flag.
/* ---------------------------------------------------------------------------- */

//#define BIGENOUGH 0x60000000
#define BIGENOUGH 0x40000000 // v2.05.59
#define HALFBIG (BIGENOUGH >> 1)
#define TOOBIG	  0xFFFFFFFF

void WINAPI extGlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
	ApiName("GlobalMemoryStatus");
	(*pGlobalMemoryStatus)(lpBuffer);
	OutTraceSYS("%s: Length=%d MemoryLoad=%d%c "
		"TotalPhys=%#x AvailPhys=%#x TotalPageFile=%#x AvailPageFile=%#x TotalVirtual=%#x AvailVirtual=%#x\n",
		ApiRef, lpBuffer->dwLength, lpBuffer->dwMemoryLoad, '%', lpBuffer->dwTotalPhys, lpBuffer->dwAvailPhys,
		lpBuffer->dwTotalPageFile, lpBuffer->dwAvailPageFile, lpBuffer->dwTotalVirtual, lpBuffer->dwAvailVirtual);
	if(lpBuffer->dwLength==sizeof(MEMORYSTATUS)){
		if ((dxw.dwFlags2 & LIMITRESOURCES) || (dxw.dwFlags15 & LIMITFREERAM)){
			char sInitPath[MAX_PATH+1];
			sprintf(sInitPath, "%sdxwnd.ini", GetDxWndPath()); 
			MEMORYSTATUS PrevMemoryStatus;
			memcpy(&PrevMemoryStatus, lpBuffer, sizeof(MEMORYSTATUS));
			DWORD dwBigEnough = GetPrivateProfileInt("window", "freeram", 0, sInitPath);
			DWORD dwHalfBig = 0;
			if(dwBigEnough == 0) {
				dwBigEnough = BIGENOUGH;
				dwHalfBig = HALFBIG;
			}
			else {
				dwHalfBig = dwBigEnough >> 1; // just half ...
			}
			if (((int)lpBuffer->dwTotalPhys < 0) || ((int)lpBuffer->dwTotalPhys > dwBigEnough)) lpBuffer->dwTotalPhys = dwBigEnough;
			if (((int)lpBuffer->dwAvailPhys < 0) || ((int)lpBuffer->dwAvailPhys > dwHalfBig)) lpBuffer->dwAvailPhys = dwHalfBig;
			if (((int)lpBuffer->dwTotalPageFile < 0) || ((int)lpBuffer->dwTotalPageFile > dwBigEnough)) lpBuffer->dwTotalPageFile = dwBigEnough;
			if (((int)lpBuffer->dwAvailPageFile < 0) || ((int)lpBuffer->dwAvailPageFile > dwHalfBig)) lpBuffer->dwAvailPageFile = dwHalfBig;
			if (((int)lpBuffer->dwTotalVirtual < 0) || ((int)lpBuffer->dwTotalVirtual > dwBigEnough)) lpBuffer->dwTotalVirtual = dwBigEnough;
			if (((int)lpBuffer->dwAvailVirtual < 0) || ((int)lpBuffer->dwAvailVirtual > dwHalfBig)) lpBuffer->dwAvailVirtual = dwHalfBig;
			if(dxw.dwDFlags & STRESSRESOURCES){
				lpBuffer->dwTotalPhys = TOOBIG;
				lpBuffer->dwAvailPhys = TOOBIG;
				lpBuffer->dwTotalPageFile = TOOBIG;
				lpBuffer->dwAvailPageFile = TOOBIG;
				lpBuffer->dwTotalVirtual = TOOBIG;
				lpBuffer->dwAvailVirtual = TOOBIG;
			}
			_if(memcmp(&PrevMemoryStatus, lpBuffer, sizeof(MEMORYSTATUS)))
				OutTraceDW("%s UPDATED: Length=%d MemoryLoad=%d%c "
					"TotalPhys=%#x AvailPhys=%#x TotalPageFile=%#x AvailPageFile=%#x TotalVirtual=%#x AvailVirtual=%#x\n",
					ApiRef, lpBuffer->dwLength, lpBuffer->dwMemoryLoad, '%', lpBuffer->dwTotalPhys, lpBuffer->dwAvailPhys,
					lpBuffer->dwTotalPageFile, lpBuffer->dwAvailPageFile, lpBuffer->dwTotalVirtual, lpBuffer->dwAvailVirtual);
		}
	}
	else{
		OutErrorSYS("%s WARNING: Length=%d sizeof(MEMORYSTATUS)=%d\n", ApiRef, lpBuffer->dwLength, sizeof(MEMORYSTATUS));
	}
}

#ifdef TRACEALL
BOOL WINAPI extGlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer)
{
	BOOL ret;
	ApiName("GlobalMemoryStatusEx");
	ret = (*pGlobalMemoryStatusEx)(lpBuffer);
	if(ret){
		if(lpBuffer->dwLength==sizeof(MEMORYSTATUSEX)){
			OutTraceSYS("%s: Length=%d MemoryLoad=%d%c "
				"TotalPhys=%llx AvailPhys=%llx TotalPageFile=%llx AvailPageFile=%llx TotalVirtual=%llx AvailVirtual=%llx\n",
				ApiRef, lpBuffer->dwLength, lpBuffer->dwMemoryLoad, '%', lpBuffer->ullTotalPhys, lpBuffer->ullAvailPhys,
				lpBuffer->ullTotalPageFile, lpBuffer->ullAvailPageFile, lpBuffer->ullTotalVirtual, lpBuffer->ullAvailVirtual);
			}
		else{
			OutErrorSYS("%s WARNING: Length=%d sizeof(LPMEMORYSTATUSEX)=%d\n", ApiRef, lpBuffer->dwLength, sizeof(LPMEMORYSTATUSEX));
		}
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

BOOL WINAPI extGetPhysicallyInstalledSystemMemory(PULONGLONG pTotalMemoryInKilobytes)
{
	BOOL ret;
	ApiName("GetPhysicallyInstalledSystemMemory");
	ret = (*pGetPhysicallyInstalledSystemMemory)(pTotalMemoryInKilobytes);
	if(ret){
		OutTraceDW("%s: TotalMemory(KBytes)=0x%llx\n", ApiRef, *pTotalMemoryInKilobytes);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}
#endif // TRACEALL

/*
From MSDN:
Operating system		Version number	dwMajorVersion	dwMinorVersion	Other
Windows 8				6.2		6		2		OSVERSIONINFOEX.wProductType == VER_NT_WORKSTATION
Windows Server 2012		6.2		6		2		OSVERSIONINFOEX.wProductType != VER_NT_WORKSTATION
Windows 7				6.1		6		1		OSVERSIONINFOEX.wProductType == VER_NT_WORKSTATION
Windows Server 2008 R2	6.1		6		1		OSVERSIONINFOEX.wProductType != VER_NT_WORKSTATION
Windows Server 2008		6.0		6		0		OSVERSIONINFOEX.wProductType != VER_NT_WORKSTATION
Windows Vista			6.0		6		0		OSVERSIONINFOEX.wProductType == VER_NT_WORKSTATION
Windows Server 2003 R2	5.2		5		2		GetSystemMetrics(SM_SERVERR2) != 0
Windows Home Server		5.2		5		2		OSVERSIONINFOEX.wSuiteMask & VER_SUITE_WH_SERVER
Windows Server 2003		5.2		5		2		GetSystemMetrics(SM_SERVERR2) == 0
Windows XP Pro x64 Ed.	5.2		5		2		(OSVERSIONINFOEX.wProductType == VER_NT_WORKSTATION) && (SYSTEM_INFO.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
Windows XP				5.1		5		1		Not applicable
Windows 2000			5.0		5		0		Not applicable
From http://delphi.about.com/cs/adptips2000/a/bltip1100_2.htm 
Windows 95				4.0		4		0
Windows 98/SE"			4.10	4		10		if osVerInfo.szCSDVersion[1] = 'A' then Windows98SE
Windows ME				4.90	4		90
*/

/*
Differences with older Windows versions
This function is provided even with older Windows versions with some significant differences than stated above:
The high order bit determins if it's NT based (NT, 2000, XP and newer) or not (Win 3.1, 95, 98, ME)
The remaining bits of the high order word specify the build number only on NT based Windows verions.

From older MSDN:
To distinguish between operating system platforms, use the high order bit and the low order byte, as shown in the following table:

Windows NT
    High order bit: 0
    Low order byte (major version number): 3 or 4
Windows 95 and Windows 98
    High order bit: 1
    Low order byte (major version number): 4
Win32s with Windows 3.1
    High order bit: 1
    Low order byte (major version number): 3

For Windows NT and Win32s, the remaining bits in the high order word specify the build number.
For Windows 95 and Windows 98, the remaining bits of the high order word are reserved.
*/

// v2.03.20: "Talonsoft's Operational Art of War II" checks the dwPlatformId field
// v2.03.20: list revised according to Microsoft compatibility settings

static struct {DWORD bMajor; DWORD bMinor; DWORD dwPlatformId; DWORD build; char *sName;} WinVersions[9]=
{
	{4, 0, VER_PLATFORM_WIN32_WINDOWS,	950,		"Windows 95"},
	{4,10, VER_PLATFORM_WIN32_WINDOWS,	67766446,	"Windows 98/SE"},
	{4,90, VER_PLATFORM_WIN32_WINDOWS,	0,			"Windows ME"},
//	{4, 0, VER_PLATFORM_WIN32_NT,		1381,		"Windows NT4.0(sp5)"},
	{5, 0, VER_PLATFORM_WIN32_NT,		2195,		"Windows 2000"},
//	{5, 1, VER_PLATFORM_WIN32_NT,		2600,		"Windows XP(sp2)"},
	{5, 1, VER_PLATFORM_WIN32_NT,		2600,		"Windows XP(sp3)"},
	{5, 2, VER_PLATFORM_WIN32_NT,		3790,		"Windows Server 2003(sp1)"},
//	{6, 0, VER_PLATFORM_WIN32_NT,		6001,		"Windows Server 2008(sp1)"},
//	{6, 0, VER_PLATFORM_WIN32_NT,		6000,		"Windows Vista"},
//	{6, 0, VER_PLATFORM_WIN32_NT,		6001,		"Windows Vista(sp1)"},
	{6, 0, VER_PLATFORM_WIN32_NT,		6002,		"Windows Vista(sp2)"},
	{6, 1, VER_PLATFORM_WIN32_NT,		7600,		"Windows 7"},
	{6, 2, VER_PLATFORM_WIN32_NT,		0,			"Windows 8"}
};

DWORD dxwGetMajor()
{
	return WinVersions[dxw.FakeVersionId].bMajor;
}

DWORD dxwGetMinor()
{
	return WinVersions[dxw.FakeVersionId].bMinor;
}

BOOL WINAPI extGetVersionExA(LPOSVERSIONINFOA lpVersionInfo)
{
	ApiName("GetVersionExA");
	BOOL ret;

	ret=(*pGetVersionExA)(lpVersionInfo);
	if(!ret) {
		TraceError();
		return ret;
	}

	OutTraceDW("%s: version=%d.%d platform=%#x build=(%d)\n", ApiRef,
		lpVersionInfo->dwMajorVersion, lpVersionInfo->dwMinorVersion, lpVersionInfo->dwPlatformId, lpVersionInfo->dwBuildNumber);

	if(dxw.dwFlags2 & FAKEVERSION) {
		// fake Win XP build 0
		lpVersionInfo->dwMajorVersion = WinVersions[dxw.FakeVersionId].bMajor;
		lpVersionInfo->dwMinorVersion = WinVersions[dxw.FakeVersionId].bMinor;
		lpVersionInfo->dwPlatformId = WinVersions[dxw.FakeVersionId].dwPlatformId;
		lpVersionInfo->dwBuildNumber = 0;
		OutTraceDW("%s: FIXED version=%d.%d platform=%#x build=(%d) os=\"%s\"\n", ApiRef,
			lpVersionInfo->dwMajorVersion, lpVersionInfo->dwMinorVersion, lpVersionInfo->dwPlatformId, lpVersionInfo->dwBuildNumber,
			WinVersions[dxw.FakeVersionId].sName);
	}
	return TRUE;
}

BOOL WINAPI extGetVersionExW(LPOSVERSIONINFOW lpVersionInfo)
{
	ApiName("GetVersionExW");
	BOOL ret;

	ret=(*pGetVersionExW)(lpVersionInfo);
	if(!ret) {
		TraceError();
		return ret;
	}

	OutTraceDW("%s: version=%d.%d platform=%#x build=(%d)\n", ApiRef,
		lpVersionInfo->dwMajorVersion, lpVersionInfo->dwMinorVersion, lpVersionInfo->dwPlatformId, lpVersionInfo->dwBuildNumber);

	if(dxw.dwFlags2 & FAKEVERSION) {
		// fake Win XP build 0
		lpVersionInfo->dwMajorVersion = WinVersions[dxw.FakeVersionId].bMajor;
		lpVersionInfo->dwMinorVersion = WinVersions[dxw.FakeVersionId].bMinor;
		lpVersionInfo->dwPlatformId = WinVersions[dxw.FakeVersionId].dwPlatformId;
		lpVersionInfo->dwBuildNumber = 0;
		OutTraceDW("%s: FIXED version=%d.%d platform=%#x build=(%d) os=\"%s\"\n", ApiRef,
			lpVersionInfo->dwMajorVersion, lpVersionInfo->dwMinorVersion, lpVersionInfo->dwPlatformId, lpVersionInfo->dwBuildNumber,
			WinVersions[dxw.FakeVersionId].sName);
	}
	return TRUE;
}

DWORD WINAPI extGetVersion(void)
{
	ApiName("GetVersion");
    DWORD dwVersion; 
    DWORD dwMajorVersion;
    DWORD dwMinorVersion; 
    DWORD dwBuild = 0;

    dwVersion = (*pGetVersion)();
 
    // Get the Windows version.

    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    // Get the build number.

    if (dwVersion < 0x80000000)              
        dwBuild = (DWORD)(HIWORD(dwVersion));

	OutTraceDW("%s: version=%d.%d build=(%d)\n", ApiRef, dwMajorVersion, dwMinorVersion, dwBuild);

	if(dxw.dwFlags2 & FAKEVERSION) {
		dwVersion = WinVersions[dxw.FakeVersionId].bMajor | (WinVersions[dxw.FakeVersionId].bMinor << 8);
		dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
		dwBuild = (DWORD)(HIWORD(dwVersion));
		if(WinVersions[dxw.FakeVersionId].bMajor == 4) dwVersion |= 0x80000000; // v2.03.11: fixes "Warhead"
		OutTraceDW("%s: FIXED version=%#x: Win%d.%d build=(%d) os=\"%s\"\n", ApiRef,
			dwVersion, dwMajorVersion, dwMinorVersion, dwBuild, WinVersions[dxw.FakeVersionId].sName);
	}

	return dwVersion;
}

/* -------------------------------------------------------------------------------

time related APIs

/* ---------------------------------------------------------------------------- */

DWORD WINAPI extGetTickCount(void)
{
	DWORD ret;
	ret=dxw.GetTickCount() + dxw.AddedTimeInMS.LowPart;
	OutDebugT("GetTickCount: ret=%#x\n", ret); 
	return ret;
}

ULONGLONG WINAPI extGetTickCount64(void)
{
	ULONGLONG ret;
	ret = (ULONGLONG)dxw.GetTickCount() + dxw.AddedTimeInMS.QuadPart;
	OutDebugT("GetTickCount64: ret=%#x\n", ret); 
	return ret;
}

void WINAPI extGetSystemTime(LPSYSTEMTIME lpSystemTime)
{
	dxw.GetSystemTime(lpSystemTime);
	OutTraceT("GetSystemTime: %02d:%02d:%02d.%03d\n", 
		lpSystemTime->wHour, lpSystemTime->wMinute, lpSystemTime->wSecond, lpSystemTime->wMilliseconds);
}

void WINAPI extGetLocalTime(LPSYSTEMTIME lpLocalTime)
{
	SYSTEMTIME SystemTime;
	dxw.GetSystemTime(&SystemTime);
	SystemTimeToTzSpecificLocalTime(NULL, &SystemTime, lpLocalTime);
	OutTraceT("GetLocalTime: %02d:%02d:%02d.%03d\n", 
		lpLocalTime->wHour, lpLocalTime->wMinute, lpLocalTime->wSecond, lpLocalTime->wMilliseconds);
}

VOID WINAPI extSleep(DWORD dwMilliseconds)
{
	DWORD dwNewDelay;
	dwNewDelay=dwMilliseconds;
	if ((dwMilliseconds!=INFINITE) && (dwMilliseconds!=0)){
		dwNewDelay = dxw.StretchTime(dwMilliseconds);
		if (dwNewDelay==0) dwNewDelay=1; // minimum allowed...
	}
	OutTraceT("Sleep: msec=%d->%d timeshift=%d\n", dwMilliseconds, dwNewDelay, dxw.TimeShift);
	//(*pSleep)(dwNewDelay);
	switch(dwNewDelay){
		case 0:
		case INFINITE:
			(*pSleep)(dwNewDelay);
			break;
		default:
			if(dxw.dwFlags13 & USENANOSLEEP)
				nanosleep(dwNewDelay * 10000); // nanosleep unit = 100 nSec (0.1 microSec)
			else
				(*pSleep)(dwNewDelay);
			break;
	}
	while(dxw.TimeFreeze)(*pSleep)(40);
}

DWORD WINAPI extSleepEx(DWORD dwMilliseconds, BOOL bAlertable)
{
	DWORD ret;
	DWORD dwNewDelay;
	dwNewDelay=dwMilliseconds;
	if ((dwMilliseconds!=INFINITE) && (dwMilliseconds!=0)){
		dwNewDelay = dxw.StretchTime(dwMilliseconds);
		if (dwNewDelay==0) dwNewDelay=1; // minimum allowed...
	}
	OutTraceT("SleepEx: msec=%d->%d alertable=%#x, timeshift=%d\n", dwMilliseconds, dwNewDelay, bAlertable, dxw.TimeShift);
	ret = (*pSleepEx)(dwNewDelay, bAlertable);
	while(dxw.TimeFreeze)(*pSleep)(40);
	return ret;
}

void WINAPI extGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	OutTraceT("GetSystemTimeAsFileTime\n");
	dxw.GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}

BOOL WINAPI extQueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
	BOOL ret;
	ApiName("QueryPerformanceCounter");

	if(dxw.dwFlags4 & NOPERFCOUNTER){
		ret=0;
		(*lpPerformanceCount).QuadPart = 0;
		OutTraceT("%s: ret=%#x count=%lld\n", ApiRef, ret, lpPerformanceCount->QuadPart);
		return 0;
	}

	LARGE_INTEGER CurrentInCount;
	ret=(*pQueryPerformanceCounter)(&CurrentInCount);
	if(dxw.dwFlags5 & NORMALIZEPERFCOUNT) {
		LARGE_INTEGER PerfFrequency;
		static LARGE_INTEGER StartCounter = {0LL};
		if (StartCounter.QuadPart == 0LL) StartCounter.QuadPart = CurrentInCount.QuadPart;
		(*pQueryPerformanceFrequency)(&PerfFrequency);
		CurrentInCount.QuadPart = ((CurrentInCount.QuadPart - StartCounter.QuadPart) * 1000000LL) / PerfFrequency.QuadPart;
	}
	if(dxw.dwFlags2 & TIMESTRETCH){
		lpPerformanceCount->QuadPart = dxw.StretchLargeCounter(CurrentInCount).QuadPart + dxw.AddedTimeInQPCTicks.QuadPart;
	}
	else {
		lpPerformanceCount->QuadPart = CurrentInCount.QuadPart;
	}

	OutTraceT("%s: ret=%#x count=%lld\n", ApiRef, ret, lpPerformanceCount->QuadPart);
	return ret;
}

BOOL WINAPI extQueryPerformanceFrequency(LARGE_INTEGER *lpPerformanceFrequency)
{
	BOOL ret;
	ApiName("QueryPerformanceFrequency");
	LARGE_INTEGER myPerfFrequency;

	ret = (*pQueryPerformanceFrequency)(&myPerfFrequency);
	if(!ret){
		OutErrorSYS("%s: ERROR ret=%d err=%d\n", ApiRef, ret, GetLastError());
		return ret;
	}
	lpPerformanceFrequency->QuadPart = myPerfFrequency.QuadPart;
	OutTraceT("%s: freq=%lld\n", ApiRef, myPerfFrequency.QuadPart);

	if(dxw.dwFlags12 & STRETCHPERFREQUENCY){
		*lpPerformanceFrequency = dxw.StretchTime(myPerfFrequency);
		OutTraceT("%s: STRETCH freq=%lld->%lld\n", ApiRef, myPerfFrequency.QuadPart, lpPerformanceFrequency->QuadPart);
		return ret;
	}

	if(dxw.dwFlags4 & NOPERFCOUNTER){
		lpPerformanceFrequency->QuadPart = 0LL;
		OutTraceT("%s: NOPERFCOUNTER freq=0\n", ApiRef);
		ret = 0;
	}

	if(dxw.dwFlags5 & NORMALIZEPERFCOUNT){
		if(dxw.dwFlags14 & CUSTOMPERFCOUNT) {
			LARGE_INTEGER Frequency;
			char InitPath[MAX_PATH];
			sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
			Frequency.QuadPart = (LONGLONG)GetPrivateProfileIntA("window", "perf_frequency", 1000000, InitPath);
			*lpPerformanceFrequency = Frequency;
			OutTraceT("%s: CUSTOM freq=%lld\n", ApiRef, Frequency.QuadPart);
		}
		else {
			lpPerformanceFrequency->QuadPart = 1000000LL;
			OutTraceT("%s: NORMALIZE freq=%lld\n", ApiRef, myPerfFrequency.QuadPart);
		}
		ret = TRUE;
	}

	OutTraceT("%s: ret=%#x freq=%lld\n", ApiRef, ret, lpPerformanceFrequency->QuadPart);
	return ret;
}

/* -------------------------------------------------------------------------------

LoadLibrary (hooking) related APIs

/* ---------------------------------------------------------------------------- */

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif

HMODULE WINAPI LoadLibraryExWrapper(LPVOID lpFileName, BOOL IsWidechar, HANDLE hFile, DWORD dwFlags, ApiArg)
{
	HMODULE libhandle;
	int idx;
	// recursion control: this is necessary so far only on WinXP while other OS like Win7,8,10 don't get into 
	// recursion problems, but in any case better to leave it here, you never know ....
	static BOOL Recursed = FALSE;

	// v2.06.01: load dlls on fake drives
	// v2.06.08: fix - do not prefix the fake drive path on relative filenames. Fixes @#@ "Nayu chan no Yuuutsu"
	// note: perhaps the bypass condition should apply to dlls on the system path?
	if ((dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE)) && !(dwFlags &LOAD_LIBRARY_SEARCH_SYSTEM32)){ 
		if(IsWidechar){
			BOOL isPath = wcschr((LPCWSTR)lpFileName, L'/') || wcschr((LPCWSTR)lpFileName, L'\\');
			if (isPath) lpFileName = (LPVOID)dxwTranslatePathW((LPCWSTR)lpFileName, NULL);
		}
		else {
			BOOL isPath = strchr((LPCSTR)lpFileName, '/') || strchr((LPCSTR)lpFileName, '\\');
			if (isPath) lpFileName = (LPVOID)dxwTranslatePathA((LPCSTR)lpFileName, NULL);
		}
	}

	// troublesome flag causing failure on 64 bit platforms - to be flagged??
	if(dwFlags & LOAD_LIBRARY_SEARCH_SYSTEM32){
		OutTraceSYS("%s: trimming LOAD_LIBRARY_SEARCH_SYSTEM32\n", ApiRef);
		dwFlags &= ~LOAD_LIBRARY_SEARCH_SYSTEM32;
	}

	// v2.04.82: Fix different behavior of LoadLibraryEx with LOAD_WITH_ALTERED_SEARCH_PATH flag on recent OS.
	// Fixes "Anachronox" as alternative fix as manually copying mss32.dll in anoxdata\PLUGIN subfolder
	if((dxw.dwFlags3 & FIXALTEREDPATH) && (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH)) {
		char LocalPath[MAX_PATH+1];
		(*pGetCurrentDirectoryA)(MAX_PATH, LocalPath);
		SetDllDirectory(LocalPath);
		OutTraceSYS("%s: fix LOAD_WITH_ALTERED_SEARCH_PATH path=\"%s\"\n", ApiRef, LocalPath);
	}

	if(IsWidechar){
		OutDebugSYS("%s: file=%ls flags=%#x\n", ApiRef, (LPCWSTR)lpFileName, dwFlags);
		libhandle=(*pLoadLibraryExW)((LPCWSTR)lpFileName, hFile, dwFlags);
	}
	else{
		OutDebugSYS("%s: file=%s flags=%#x\n", ApiRef, lpFileName, dwFlags);
		libhandle=(*pLoadLibraryExA)((LPCTSTR)lpFileName, hFile, dwFlags);
	}

	if(Recursed) {
		// v2.03.97.fx2: clear Recursed flag when exiting!
		Recursed = FALSE;
		OutTrace("%s: recursed\n", ApiRef);
		return libhandle;
	}
	Recursed = TRUE;

	while(TRUE) { // fake loop

		if(!libhandle && (dxw.dwFlags11 & REMAPSYSFOLDERS)){
			if(IsWidechar){
				if(!_wcsnicmp((WCHAR *)lpFileName, L"C:\\WINDOWS\\", wcslen(L"C:\\WINDOWS\\"))){
					WCHAR *lpLocalPath = (WCHAR *)malloc(wcslen((WCHAR *)lpFileName) * sizeof(WCHAR));
					wcscpy(lpLocalPath, L".\\Windows\\");
					wcscat(lpLocalPath, (WCHAR *)lpFileName+wcslen(L"C:\\WINDOWS\\"));
					OutDebugSYS("%s: file=%ls flags=%#x\n", ApiRef, lpLocalPath, dwFlags);
					libhandle=(*pLoadLibraryExW)(lpLocalPath, hFile, dwFlags);
					free(lpLocalPath);
				}
			}
			else {
				if(!_strnicmp((const char *)lpFileName, "C:\\WINDOWS\\", strlen("C:\\WINDOWS\\"))){
					char *lpLocalPath = (char *)malloc(strlen((char *)lpFileName));
					strcpy(lpLocalPath, ".\\Windows\\");
					strcat(lpLocalPath, (char *)lpFileName+strlen("C:\\WINDOWS\\"));
					OutDebugSYS("%s: file=%s flags=%#x\n", ApiRef, lpLocalPath, dwFlags);
					libhandle=(*pLoadLibraryExA)(lpLocalPath, hFile, dwFlags);
					free(lpLocalPath);
				}
			}
		}

		// found in "The Rage" (1996): loading a module with relative path after a SetCurrentDirectory may fail, though
		// the module is present in the current directory folder. To fix this problem in case of failure it is possible 
		// to retry the operation using a full pathname composed concatenating current dir and module filename.
		// V2.05.38 FIX: "The Rage" hides libraries within files with different extensions, so you can't add the ".dll"
		// extension if missing. Maybe you could check if there's an extension at all, but it seems wiser to alter the
		// path as little as possible, so better not to do it. Added a check only to avoid the useless operation of
		// adding a full path to another full path (comparison of 2nd char with ':', rough but working).
		// v2.06.09: added a string length check to avoid exceptions in @#@ "Counter-Strike: Condition Zero"
		if(!libhandle){
			__try {
				BOOL bIsAbsolutePath = FALSE;
				char lpBuffer[MAX_PATH+1];
				(*pGetCurrentDirectoryA)(MAX_PATH, lpBuffer);
				if(IsWidechar) {
					bIsAbsolutePath = (((WCHAR *)lpFileName)[1] == L':');
					if((strlen(lpBuffer) + wcslen((WCHAR *)lpFileName) + 1) > MAX_PATH){
						OutTrace("%s: ERROR path length overflow\n", ApiRef);
						libhandle = NULL;
						break;
					}
					sprintf_s(lpBuffer, MAX_PATH, "%s\\%ls", 
						lpBuffer, 
						lpFileName);
				}
				else {
					bIsAbsolutePath = (((char *)lpFileName)[1] == ':');
					if((strlen(lpBuffer) + strlen((char *)lpFileName) + 1) > MAX_PATH){
						OutTrace("%s: ERROR path length overflow\n", ApiRef);
						libhandle = NULL;
						break;
					}
					sprintf_s(lpBuffer, MAX_PATH, "%s\\%s", 
						lpBuffer, 
						lpFileName);
				}

				if(!bIsAbsolutePath){
					OutTraceDW("%s: RETRY fullpath=\"%s\"\n", ApiRef, lpBuffer);
					libhandle=(*pLoadLibraryExA)(lpBuffer, hFile, dwFlags);
				}
			} __except(EXCEPTION_EXECUTE_HANDLER){
				OutTraceDW("%s: EXCEPTION\n", ApiRef);
				libhandle = NULL;
				break;
			}
		}

#ifndef DXW_NOTRACES
		char sFlags[128];
		if(IsWidechar){
			OutTraceDW("%s: FileName=%ls hFile=%#x Flags=%#x(%s) hmodule=%#x\n", ApiRef, lpFileName, hFile, dwFlags, ExplainLoadLibFlags(dwFlags, sFlags, 128), libhandle);
		}
		else {
			OutTraceDW("%s: FileName=%s hFile=%#x Flags=%#x(%s) hmodule=%#x\n", ApiRef, lpFileName, hFile, dwFlags, ExplainLoadLibFlags(dwFlags, sFlags, 128), libhandle);
		}
#endif // DXW_NOTRACES

		if(!libhandle){
			if(IsWidechar){
				OutErrorSYS("%s: ERROR FileName=%ls err=%d\n", ApiRef, lpFileName, GetLastError());
			}
			else {
				OutErrorSYS("%s: ERROR FileName=%s err=%d\n", ApiRef, lpFileName, GetLastError());
			}
			Recursed = FALSE;
		
			// compatibility issue: some games (Nightmare Creatures) check for the ERROR_DLL_NOT_FOUND
			// errorcode or assume the library is there, hence the dialog box about a SDL.DLL file to delete.
			if((dxw.dwFlags8 & LOADLIBRARYERR) && (GetLastError()==ERROR_MOD_NOT_FOUND)) {
				OutTraceDW("%s: setting err=ERROR_DLL_NOT_FOUND\n", ApiRef);
				SetLastError(ERROR_DLL_NOT_FOUND);
			}
			break;
		}

		// when loaded with LOAD_LIBRARY_AS_DATAFILE or LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE flags, 
		// there's no symbol map, then it isn't possible to hook function calls.
		if(dwFlags & (LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE|LOAD_LIBRARY_AS_DATAFILE)) break;

		char *AnsiFileName;
		if(IsWidechar){	
			static char sFileName[256+1];
			wcstombs_s(NULL, sFileName, (LPCWSTR)lpFileName, 80);
			AnsiFileName = sFileName;
		}
		else
			AnsiFileName = (char *)lpFileName;

		// v2.05.51: "State of War 2" incredibly loads through a LoadLibraryExW call an executable
		// file, c:\\Windows\winhlp32.exe !! Though it's unclear to me why that for, the operation
		// followed by a hook attempt causes the game crash, so it must be bypassed.
		// Here DxWnd bypasses any attempt to load an exe file, you never know ...
		int pathlen = strlen(AnsiFileName);
		if((pathlen > 4) && (!strcmp(&AnsiFileName[pathlen-4], ".exe"))){
			OutTraceDW("%s: BYPASS exe hooking path=%s\n", ApiRef, AnsiFileName);
			break;
		}

		// v2.06.00: found in "Narar Superboard", skip lib hooking to avoid crash since there
		// is no symbol table
		if(dwFlags & DONT_RESOLVE_DLL_REFERENCES) break;

		idx=dxw.GetDLLIndex(AnsiFileName); // v2.04.66 !!! AnsiFileName vs. lpFileName !!!

		//if(idx == SYSLIBIDX_KERNEL32) { // v2.06.05 !!! ffdshow does this causing exceptions
		//	OutTrace("%s: BYPASS LoadLibrary(kernel32.dll)\n", ApiRef);
		//	return GetModuleHandle("kernel32");
		//}

		// handle custom OpenGL library
		if(!lstrcmpi(AnsiFileName,dxw.CustomOpenGLLib)){
			idx=SYSLIBIDX_OPENGL;
			SysLibsTable[idx].hmodule = libhandle;
		}

		if((dxw.dwFlags17 & HIDEDIRECTDRAW) && (idx == SYSLIBIDX_DIRECTDRAW)) {
			OutTraceDW("%s: HIDEDIRECTDRAW ret=NULL\n", ApiRef);
			SetLastError(ERROR_MOD_NOT_FOUND);
			libhandle = NULL;
			break;
		}

		if(dxw.dwFlags12 & SUPPRESSGLIDE){
			switch(idx){
				case SYSLIBIDX_GLIDE:
				case SYSLIBIDX_GLIDE2:
				case SYSLIBIDX_GLIDE3:
					OutTraceDW("%s: suppress glide support\n", ApiRef);
					(*pFreeLibrary)(libhandle);
					SetLastError(ERROR_MOD_NOT_FOUND);
					libhandle = NULL;
					break;
			}
		}
		if (idx == -1)  {
			OutTraceDW("%s: hooking lib=\"%s\" handle=%#x\n", ApiRef, AnsiFileName, libhandle);
			// HookModule(libhandle); -- useless and dangerous for WATCOM C/C++ libraries
			HookDlls(libhandle);
		}
		else {
			OutTraceDW("%s: push idx=%#x library=%s hdl=%#x\n", ApiRef, idx, AnsiFileName, libhandle);
			dxwLibsHookTable_Type *lpEntry = &SysLibsTable[idx];
			lpEntry->hmodule = libhandle;
			if(dxw.dwFlags12 & DIRECTXREPLACE) {
				switch(idx){
					case SYSLIBIDX_DIRECTDRAW:
					case SYSLIBIDX_DIRECT3D8:
					case SYSLIBIDX_DIRECT3D9:
						if(libhandle){
							char path[MAX_PATH];
							HMODULE newhandle;
							sprintf(path, "%salt.dll\\%s.dll", GetDxWndPath(), SysLibsTable[idx].name);
							newhandle = (*pLoadLibraryA)(path);
							OutTrace("%s: alt.dll path=%s hinst=%#x\n", ApiRef, path, newhandle);
							if(newhandle){
								// replace when a replacement dll is found
								// CloseHandle may be hooked or not ...
								CloseHandle(libhandle);
								libhandle = newhandle;
							}
						}
						break;
				}
			}
			if((lpEntry->flags & DXWHOOK_HOOK) && lpEntry->hookf) (*lpEntry->hookf)(libhandle);
			if(lpEntry->flags & DXWHOOK_EXTEND){
				OutTraceDW("%s: extend hooking lib=\"%s\" handle=%#x\n", ApiRef, AnsiFileName, libhandle);
				HookDlls(libhandle);
			}
		}
		break;
	}
	Recursed = FALSE;
	OutDebugSYS("%s: ret=%#x\n", ApiRef, libhandle);
	return libhandle;
}

HMODULE WINAPI extLoadLibraryA(LPCTSTR lpFileName)
{ ApiName("LoadLibraryA"); return LoadLibraryExWrapper((LPVOID)lpFileName, FALSE, NULL, 0, ApiRef); }

HMODULE WINAPI extLoadLibraryW(LPCWSTR lpFileName)
{ ApiName("LoadLibraryW"); return LoadLibraryExWrapper((LPVOID)lpFileName, TRUE, NULL, 0, ApiRef); }

HMODULE WINAPI extLoadLibraryExA(LPCTSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{ ApiName("LoadLibraryExA"); return LoadLibraryExWrapper((LPVOID)lpFileName, FALSE, hFile, dwFlags, ApiRef); }

HMODULE WINAPI extLoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{ ApiName("LoadLibraryExW"); return LoadLibraryExWrapper((LPVOID)lpFileName, TRUE, hFile, dwFlags, ApiRef); }

// v2.04.82: 
// FIXFREELIBRARY fixes a compatibility issue with w95 where multiple calls to 
// FreeLibrary are expected to end up with an error (error code 0) so that Heavy Gear loops
// the FreeLibrary call until it gets a 0 return value, while on recent OS the call always 
// return success (1)
// SKIPFREELIBRARY fixes a recursion problem in pre-patched executables (like GOG version of
// Disciples Gold) where FreeLibrary keeps calling itself ending up with a stack overflow

static HMODULE hSkipModule = NULL;
static HMODULE hLastModule = NULL;

BOOL WINAPI extFreeLibrary(HMODULE hModule)
{ 
	BOOL ret;
	ApiName("FreeLibrary");

	OutTraceSYS("%s: hModule=%#x\n", ApiRef, hModule);

	// early FreeLibrary fix: needed by "GOG Disciples Gold"
	if(dxw.dwFlags3 & SKIPFREELIBRARY) { // "Compat. > Skip FreeLibrary recursion"
		if (hModule == hSkipModule) {
			ret = (dxw.dwFlags7 & FIXFREELIBRARY) ? 0 : 1;
			OutTraceDW("%s: SKIPFREELIBRARY hack ret=%d\n", ApiRef, ret);
			return ret;
		}
		hSkipModule = hModule;
	}

	ret = (*pFreeLibrary)(hModule);

	if(ret){
		OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
		// late FreeeLibrary fix: needed by "Heavy Gear"
		if(dxw.dwFlags7 & FIXFREELIBRARY) {
			if(hModule == hLastModule) {
				OutTraceDW("%s: FIXFREELIBRARY hack ret=0\n", ApiRef);
				ret = 0;
			}
			hLastModule = hModule;
		}
		dxw.ht.hClean();
	}
	else {
		OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
	}

	//if(dxw.dwDFlags2 & EXPERIMENTAL) ReHook();

	OutTraceDW("%s: ret=%d\n", ApiRef, ret);
	return ret; 
}

extern DirectDrawCreate_Type pDirectDrawCreate;
extern DirectDrawCreateEx_Type pDirectDrawCreateEx;
extern HRESULT WINAPI extDirectDrawCreate(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);
extern HRESULT WINAPI extDirectDrawCreateEx(GUID FAR *, LPDIRECTDRAW FAR *, REFIID, IUnknown FAR *);
extern GetProcAddress_Type pGetProcAddress;
//extern HRESULT STDAPICALLTYPE extCoCreateInstance(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID FAR*);

DWORD WINAPI dxwRegisterServiceProcess(DWORD dwProcessID, DWORD dwType)
{
	OutTrace("FAKE kernel32.RegisterServiceProcess: pid=%#x type=%d(%s)\n",
		dwProcessID, dwType, dwType ? "RSPSIMPLESERVICE" : "RSPUNREGISTERSERVICE");
	// fake a success. When the function fails, it returns 0.
	return 0x1;
}

FARPROC WINAPI extGetProcAddress(HMODULE hModule, LPCSTR proc)
{
	FARPROC ret;
	int idx;
	ApiName("GetProcAddress");

	// WARNING: seems to be called with bad LPCSTR value....
	// from MSDN:
	// The function or variable name, or the function's ordinal value. 
	// If this parameter is an ordinal value, it must be in the low-order word; 
	// the high-order word must be zero.

	// v2.04.47: skipping processing of recursive calls.
	// recursion is caused by the use of some of these calls from within the stdlibC routines
	// used for logging, so no logging for these statements. Logging moved forward.
	// a better solution would be linking a private subset of std routines ...
	// fixes "Bruce Lee Remake" with suspend process hooking.

	for(idx=0; SysLibsTable[idx].name; idx++){
		if(SysLibsTable[idx].hmodule == hModule) break;
	}

	OutTraceDW("%s: hModule=%#x idx=%d(%s) proc=%s\n", ApiRef, hModule, idx, SysLibsTable[idx].name, ProcToString(proc));

	if((!SysLibsTable[idx].name) || (SysLibsTable[idx].flags == DXWHOOK_NULL)){
		ret=(*pGetProcAddress)(hModule, proc);
		OutTraceDW("%s: module=%#x proc=%s ret=%#x\n", ApiRef, hModule, ProcToString(proc), ret);
		return ret;
	}

	if(idx == SYSLIBIDX_GDI32){
		// avoid recursions
		if(!strncmp(proc, "D3DKMT", 6)) return(*pGetProcAddress)(hModule, proc);
	}

	if(idx == SYSLIBIDX_KERNEL32){
		// v2.06.03: @#@ "Abbie 2.2.0"
		if(!strcmp(proc, "RegisterServiceProcess")) return (FARPROC)dxwRegisterServiceProcess;
	}

	if((DWORD)proc & 0xFFFF0000){
		FARPROC remap = 0;
		if((SysLibsTable[idx].flags & DXWHOOK_HOOK) && (SysLibsTable[idx].remapf)){
			remap = (*SysLibsTable[idx].remapf)(proc, hModule);
			if(remap == (FARPROC)-1) {
				OutTraceDW("%s: FAKE ret=0\n", ApiRef);
				return 0; // pretend the call isn't there ....
			}
			if(remap) {
				OutTraceDW("%s: HOOK ret=%#x\n", ApiRef, remap);
				return remap;
			}
		}
	}
	else {
		switch(idx){
		case SYSLIBIDX_DIRECTDRAW:
			switch((DWORD)proc){
				case 0x0008: // DirectDrawCreate
					pDirectDrawCreate=(DirectDrawCreate_Type)(*pGetProcAddress)(hModule, proc);
					OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), pDirectDrawCreate);
					return (FARPROC)extDirectDrawCreate;
					break;
				case 0x000A: // DirectDrawCreateEx
					pDirectDrawCreateEx=(DirectDrawCreateEx_Type)(*pGetProcAddress)(hModule, proc);
					OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), pDirectDrawCreateEx);
					return (FARPROC)extDirectDrawCreateEx;
					break;
				case 0x000B: // DirectDrawEnumerateA
					pDirectDrawEnumerateA=(DirectDrawEnumerateA_Type)(*pGetProcAddress)(hModule, proc);
					OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, proc, pDirectDrawEnumerateA);
					return (FARPROC)extDirectDrawEnumerateA;
					break;
				case 0x000C: // DirectDrawEnumerateExA
					pDirectDrawEnumerateExA=(DirectDrawEnumerateExA_Type)(*pGetProcAddress)(hModule, proc);
					OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, proc, pDirectDrawEnumerateExA);
					return (FARPROC)extDirectDrawEnumerateExA;
					break;
			}
			break;
		case SYSLIBIDX_USER32:
			if ((DWORD)proc == 0x0020){ // ChangeDisplaySettingsA
				/* if (!pChangeDisplaySettingsA) */ pChangeDisplaySettingsA=(ChangeDisplaySettingsA_Type)(*pGetProcAddress)(hModule, proc);
				OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), pChangeDisplaySettingsA);
				return (FARPROC)extChangeDisplaySettingsA;
			}
			break;
#ifndef ANTICHEATING
		case SYSLIBIDX_KERNEL32:
			if ((DWORD)proc == 0x0305){ // "IsDebuggerPresent"
				OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), extIsDebuggerPresent);
				return (FARPROC)extIsDebuggerPresent;
			}
			if ((DWORD)proc == 0x0050){ // "CheckRemoteDebuggerPresent"
				OutTraceDW("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), extCheckRemoteDebuggerPresent);
				return (FARPROC)extCheckRemoteDebuggerPresent;
			}
#endif
		// v2.04.49: commented out because unreliable, ordinal numbers of OLE32 are not constant
		//case SYSLIBIDX_OLE32:
		//	if ((DWORD)proc == 0x0011){ // "CoCreateInstance"
		//		pCoCreateInstance=(CoCreateInstance_Type)(*pGetProcAddress)(hModule, proc);
		//		OutTraceDW("GetProcAddress: hooking proc=%s at addr=%#x\n", ProcToString(proc), pCoCreateInstance);
		//		return (FARPROC)extCoCreateInstance;
		//	}
		//	break;
		}
	}

	ret=(*pGetProcAddress)(hModule, proc);
	OutTraceDW("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

/* -------------------------------------------------------------------------------

I/O related APIs (together with GetDiskFreeSpace)

/* ---------------------------------------------------------------------------- */

UINT WINAPI extGetDriveTypeA(LPCSTR lpRootPathName)
{
	UINT ret;
	ApiName("GetDriveTypeA");
	char *labels[]={
		"DRIVE_UNKNOWN",
		"DRIVE_NO_ROOT_DIR",
		"DRIVE_REMOVABLE",
		"DRIVE_FIXED",
		"DRIVE_REMOTE",
		"DRIVE_CDROM",
		"DRIVE_RAMDISK"
	};

	if(dxw.bHintActive) ShowHint(HINT_CDCHECK);

	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpRootPathName);
	// v2.05.44 fix: it is possible to call GetDriveTypeA with NULL argument. Found in "Leadfoot Stadium Off-Road Racing"
	// v2.05.60 fix: returns DRIVE_FIXED for fake HD drives
	// v2.05.66 fix: getting to extGetDriveTypeA with pGetDriveTypeA==NULL. Added initialization in hook table
	if(lpRootPathName){
		if ((dxw.dwFlags10 & FAKECDDRIVE) && (dxw.FakeCDDrive == (char)toupper(lpRootPathName[0]))) {
			OutTraceSYS("%s: FAKECDDRIVE - fake CDROM %c: ret=5(DRIVE_CDROM)\n", ApiRef, dxw.FakeCDDrive); 
			return DRIVE_CDROM;
		}
		if ((dxw.dwFlags10 & FAKEHDDRIVE) && (dxw.FakeHDDrive == (char)toupper(lpRootPathName[0]))) {
			OutTraceSYS("%s: FAKEHDDRIVE - fake HDISK %c: ret=3(DRIVE_FIXED)\n", ApiRef, dxw.FakeCDDrive); 
			return DRIVE_FIXED;
		}
		if (dxw.dwFlags3 & CDROMDRIVETYPE) {
			OutTraceSYS("%s: CDROMDRIVETYPE - fake CDROM %c: ret=5(DRIVE_CDROM)\n", ApiRef, lpRootPathName[0]); 
			return DRIVE_CDROM;
		}
		if (dxw.dwFlags4 & HIDECDROMEMPTY){ 
			BOOL Vol;
			Vol = GetVolumeInformationA(lpRootPathName, NULL, NULL, NULL, 0, 0, 0, 0);
			OutTraceSYS("Vol=%#x\n", Vol);
			if(!Vol) {
				OutTraceSYS("%s: HIDECDROMEMPTY - hide CDROM %c:\n", ApiRef, lpRootPathName[0]); 
				return DRIVE_UNKNOWN;
			}
		}

		ret = (*pGetDriveTypeA)(lpRootPathName);

		if(dxw.dwFlags17 & HIDECDROMREAL){
			// @#@ "Geisters" (Ko 1997)
			// @#@ "Club Rapper" (Ko)
			// @#@ "Revival of a Myth Lazenca" (Ko 1997)
			if(ret == DRIVE_CDROM){
				OutTraceSYS("%s: HIDECDROMREAL - hide CDROM %c:\n", ApiRef, lpRootPathName[0]); 
				return DRIVE_UNKNOWN;
			}
		}
	}
	else {
		// v2.05.50: when passing NULL (current folder) use the fake driver specification
		ret = (*pGetDriveTypeA)(lpRootPathName);
		if (dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE)){
			if(dxw.dwCurrentFolderType == DXW_FAKE_HD) ret = DRIVE_FIXED;
			if(dxw.dwCurrentFolderType == DXW_FAKE_CD) ret = DRIVE_CDROM;
		}
	}

	OutTraceSYS("%s: ret=%#x(%s)\n", ApiRef, ret, ((ret>= DRIVE_UNKNOWN) && (ret <= DRIVE_RAMDISK)) ? labels[ret] : "unknown"); 
	return ret;
}

UINT WINAPI extGetDriveTypeW(LPCWSTR lpRootPathName)
{
	UINT ret;
	ApiName("GetDriveTypeA");
	char *labels[]={
		"DRIVE_UNKNOWN",
		"DRIVE_NO_ROOT_DIR",
		"DRIVE_REMOVABLE",
		"DRIVE_FIXED",
		"DRIVE_REMOTE",
		"DRIVE_CDROM",
		"DRIVE_RAMDISK"
	};

	if(dxw.bHintActive) ShowHint(HINT_CDCHECK);

	OutTraceSYS("%s: path=\"%ls\"\n", ApiRef, lpRootPathName);
	// v2.05.44 fix: it is possible to call GetDriveTypeW with NULL argument.
	// v2.05.60 fix: returns DRIVE_FIXED for fake HD drives
	if(lpRootPathName){
		if ((dxw.dwFlags10 & FAKECDDRIVE) && (dxw.FakeCDDrive == (char)toupper(lpRootPathName[0]))) {
			OutTraceSYS("%s: FAKECDDRIVE - fake CDROM %c: ret=5(DRIVE_CDROM)\n", ApiRef, dxw.FakeCDDrive); 
			return DRIVE_CDROM;
		}
		if ((dxw.dwFlags10 & FAKEHDDRIVE) && (dxw.FakeHDDrive == (char)toupper(lpRootPathName[0]))) {
			OutTraceSYS("%s: FAKEHDDRIVE - fake HDISK %c: ret=3(DRIVE_FIXED)\n", ApiRef, dxw.FakeCDDrive); 
			return DRIVE_FIXED;
		}
		if (dxw.dwFlags3 & CDROMDRIVETYPE) {
			OutTraceSYS("%s: CDROMDRIVETYPE - fake CDROM %c: ret=5(DRIVE_CDROM)\n", ApiRef, lpRootPathName[0]); 
			return DRIVE_CDROM;
		}
		if (dxw.dwFlags4 & HIDECDROMEMPTY){ 
			BOOL Vol;
			Vol = GetVolumeInformationW(lpRootPathName, NULL, NULL, NULL, 0, 0, 0, 0);
			OutTraceSYS("Vol=%#x\n", Vol);
			if(!Vol) {
				OutTraceSYS("%s: HIDECDROMEMPTY - hide CDROM %c:\n", ApiRef, (char)lpRootPathName[0]); 
				return DRIVE_UNKNOWN;
			}
		}

		ret = (*pGetDriveTypeW)(lpRootPathName);

		if(dxw.dwFlags17 & HIDECDROMREAL){
			if(ret == DRIVE_CDROM){
				OutTraceSYS("%s: HIDECDROMREAL - hide CDROM %lc:\n", ApiRef, lpRootPathName[0]); 
				return DRIVE_UNKNOWN;
			}
		}
	}
	else {
		// v2.05.50: when passing NULL (current folder) use the fake driver specification
		ret = (*pGetDriveTypeW)(lpRootPathName);
		if (dxw.dwFlags10 & (FAKECDDRIVE|FAKEHDDRIVE)){
			if(dxw.dwCurrentFolderType == DXW_FAKE_HD) ret = DRIVE_FIXED;
			if(dxw.dwCurrentFolderType == DXW_FAKE_CD) ret = DRIVE_CDROM;
		}
	}

	OutTraceSYS("%s: ret=%#x(%s)\n", ApiRef, ret, ((ret>= DRIVE_UNKNOWN) && (ret <= DRIVE_RAMDISK)) ? labels[ret] : "unknown"); 
	return ret;
}

#ifndef DXW_NOTRACES
static char *sDevArray(DWORD mask)
{
	static char sDevLetters[32+1];
	for(int i=0; i<32; i++){
		sDevLetters[i] = (mask & 0x1) ? 'A'+i : '-';
		mask >>= 1;
	}
	sDevLetters[32]=0;
	return sDevLetters;
}
#endif // DXW_NOTRACES

DWORD WINAPI extGetLogicalDrives(void)
{
	DWORD DevMask;
	OutTraceDW("GetLogicalDrives:\n");

	if(dxw.bHintActive) ShowHint(HINT_CDCHECK);

	DevMask = (*pGetLogicalDrives)();
	if (dxw.dwFlags4 & HIDECDROMEMPTY){ 
		for(int i=0; i<32; i++){
			DWORD DevBit;
			BOOL Vol;
			DevBit = 0x1 << i;
			if(DevMask & DevBit){
				char RootPathName[10];
				sprintf_s(RootPathName, 4, "%c:\\", 'A'+i);
				Vol = GetVolumeInformation(RootPathName, NULL, NULL, NULL, 0, 0, 0, 0);
				OutTraceSYS("Vol=%s status=%#x\n", RootPathName, Vol);
				if(!Vol) DevMask &= ~DevBit;
			}
		}
	}
	// v2.04.84: handling of fake HD drive when FAKEHDDRIVE
	if (dxw.dwFlags10 & FAKEHDDRIVE){
		int BitShift = dxw.FakeHDDrive - 'A';
		DevMask |= 0x1 << BitShift;
		OutTraceSYS("GetLogicalDrives: added virtual HD %c:\\\n", dxw.FakeHDDrive);
	}
	// v2.04.84: handling of fake CDROM drive when FAKECDDRIVE
	if (dxw.dwFlags10 & FAKECDDRIVE){
		int BitShift = dxw.FakeCDDrive - 'A';
		DevMask |= 0x1 << BitShift;
		OutTraceSYS("GetLogicalDrives: added virtual CDROM %c:\\\n", dxw.FakeCDDrive);
	}

	if(dxw.dwFlags15 & EMULATEFLOPPYDRIVE) DevMask |= 0x3; // 0x1 => A:, 0x3 => A: + B:

	OutTraceSYS("GetLogicalDrives: ret=%08.8X(%s)\n", DevMask, sDevArray(DevMask));
	return DevMask;
}

#ifndef DXW_NOTRACES
static char *ExplainFlagsAndAttributes(DWORD c)
{
	static char eb[256];
	unsigned int l;
	strcpy(eb,"");
	if (c & FILE_ATTRIBUTE_ARCHIVE) strcat(eb, "FILE_ATTRIBUTE_ARCHIVE+");
	if (c & FILE_ATTRIBUTE_ENCRYPTED) strcat(eb, "FILE_ATTRIBUTE_ENCRYPTED+");
	if (c & FILE_ATTRIBUTE_HIDDEN) strcat(eb, "FILE_ATTRIBUTE_HIDDEN+");
	if (c & FILE_ATTRIBUTE_NORMAL) strcat(eb, "FILE_ATTRIBUTE_NORMAL+");
	if (c & FILE_ATTRIBUTE_OFFLINE) strcat(eb, "FILE_ATTRIBUTE_OFFLINE+");
	if (c & FILE_ATTRIBUTE_READONLY) strcat(eb, "FILE_ATTRIBUTE_READONLY+");
	if (c & FILE_ATTRIBUTE_SYSTEM) strcat(eb, "FILE_ATTRIBUTE_SYSTEM+");
	if (c & FILE_ATTRIBUTE_TEMPORARY) strcat(eb, "FILE_ATTRIBUTE_TEMPORARY+");
	if (c & FILE_FLAG_BACKUP_SEMANTICS) strcat(eb, "FILE_FLAG_BACKUP_SEMANTICS+");
	if (c & FILE_FLAG_DELETE_ON_CLOSE) strcat(eb, "FILE_FLAG_DELETE_ON_CLOSE+");
	if (c & FILE_FLAG_NO_BUFFERING) strcat(eb, "FILE_FLAG_NO_BUFFERING+");
	if (c & FILE_FLAG_OPEN_NO_RECALL) strcat(eb, "FILE_FLAG_OPEN_NO_RECALL+");
	if (c & FILE_FLAG_OPEN_REPARSE_POINT) strcat(eb, "FILE_FLAG_OPEN_REPARSE_POINT+");
	if (c & FILE_FLAG_OVERLAPPED) strcat(eb, "FILE_FLAG_OVERLAPPED+");
	if (c & FILE_FLAG_POSIX_SEMANTICS) strcat(eb, "FILE_FLAG_POSIX_SEMANTICS+");
	if (c & FILE_FLAG_RANDOM_ACCESS) strcat(eb, "FILE_FLAG_RANDOM_ACCESS+");
	//if (c & FILE_FLAG_SESSION_AWARE) strcat(eb, "FILE_FLAG_SESSION_AWARE+");
	if (c & FILE_FLAG_SEQUENTIAL_SCAN) strcat(eb, "FILE_FLAG_SEQUENTIAL_SCAN+");
	if (c & FILE_FLAG_WRITE_THROUGH) strcat(eb, "FILE_FLAG_WRITE_THROUGH+");
	if (c & SECURITY_ANONYMOUS) strcat(eb, "SECURITY_ANONYMOUS+");
	if (c & SECURITY_CONTEXT_TRACKING) strcat(eb, "SECURITY_CONTEXT_TRACKING+");
	if (c & SECURITY_DELEGATION) strcat(eb, "SECURITY_DELEGATION+");
	if (c & SECURITY_EFFECTIVE_ONLY) strcat(eb, "SECURITY_EFFECTIVE_ONLY+");
	if (c & SECURITY_IDENTIFICATION) strcat(eb, "SECURITY_IDENTIFICATION+");
	if (c & SECURITY_IMPERSONATION) strcat(eb, "SECURITY_IMPERSONATION+");
	l=strlen(eb);
	if (l>strlen("")) eb[l-1]=0; // delete last '+' if any
	return(eb);
}

static char *ExplainDesiredAccess(DWORD c)
{
	static char eb[256];
	unsigned int l;
	strcpy(eb,"GENERIC_");
	if (c & GENERIC_READ) strcat(eb, "READ+");
	if (c & GENERIC_WRITE) strcat(eb, "WRITE+");
	if (c & GENERIC_EXECUTE) strcat(eb, "EXECUTE+");
	if (c & GENERIC_ALL) strcat(eb, "ALL+");
	l=strlen(eb);
	if (l>strlen("GENERIC_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}

static char *ExplainDisposition(DWORD c)
{
	static char *captions[] = {
		"NULL","CREATE_NEW","CREATE_ALWAYS","OPEN_EXISTING","OPEN_ALWAYS","TRUNCATE_EXISTING"
	};
	if(c > TRUNCATE_EXISTING) return "???";
	return captions[c];
}
#endif // DXW_NOTRACES

HANDLE WINAPI extCreateFileA(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
							LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
							DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	HANDLE ret;
	ApiName("CreateFileA");

	int err=0;
	OutTraceSYS("%s: FileName=\"%s\" DesiredAccess=%#x(%s) SharedMode=%#x Disposition=%#x(%s) Flags=%#x(%s)\n", 
		ApiRef, lpFileName, dwDesiredAccess, ExplainDesiredAccess(dwDesiredAccess), dwShareMode, 
		dwCreationDisposition, ExplainDisposition(dwCreationDisposition),
		dwFlagsAndAttributes, ExplainFlagsAndAttributes(dwFlagsAndAttributes));

	if((dxw.dwFlags3 & BUFFEREDIOFIX) && (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)){
		OutTraceDW("%s: suppress FILE_FLAG_NO_BUFFERING on Filename=\"%s\"\n", ApiRef, lpFileName); 
		dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;
	}

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		lpFileName = dxwTranslatePathA(lpFileName, &mapping);

		// if mapped on virtual CD and write access required, you should fake a no access error code
		if(mapping == DXW_FAKE_CD){
			if(dxw.dwFlags14 & WIN16CREATEFILE){
				if(dwDesiredAccess == (GENERIC_WRITE|GENERIC_READ)){
					OutTraceDW("%s: WIN16CREATEFILE use READ mode\n", ApiRef);
					dwDesiredAccess = GENERIC_READ;
				}
			}
			if(dwDesiredAccess & (GENERIC_WRITE|GENERIC_ALL)){
				OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
				// should set lasterror here?
				SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
				return INVALID_HANDLE_VALUE;
			}
		}
	}

	// v2.05.71 fix: Korean game "Zero" calls CreateFile with NULL path
	if((dxw.dwFlags12 & PATHLOCALE) && lpFileName){
		int size = strlen(lpFileName);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpFileName, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		ret=(*pCreateFileW)(lpFileNameW, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		free(lpFileNameW);
	}
	else {
		ret=(*pCreateFileA)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
	if(ret && (ret != INVALID_HANDLE_VALUE)) {
		err = 0;
		OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	else {
		err = GetLastError();
		OutTraceSYS("%s: ERROR err=%d fname=\"%s\"\n", ApiRef, GetLastError(), lpFileName);
	}

	if(dxw.dwFlags14 & WIN16CREATEFILE){
		if((err == 5) && (dwDesiredAccess == (GENERIC_WRITE|GENERIC_READ))){
			OutTraceDW("%s: WIN16CREATEFILE retry with READ mode\n", ApiRef);
			SetLastError(0);
			dwDesiredAccess = GENERIC_READ;
			ret=(*pCreateFileA)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
			if(ret && (ret != INVALID_HANDLE_VALUE)) {
				OutTraceDW("%s: WIN16CREATEFILE ret=%#x\n", ApiRef, ret);
			}
			else {
				OutTraceDW("%s: WIN16CREATEFILE err=%d fname=\"%s\"\n", ApiRef, GetLastError(), lpFileName);
			}
		}
	}

	return ret;
} 

HANDLE WINAPI extCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
							LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
							DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	HANDLE ret;
	ApiName("CreateFileW");

	int err=0;
	OutTraceSYS("%s: FileName=\"%ls\" DesiredAccess=%#x(%s) SharedMode=%#x Disposition=%#x(%s) Flags=%#x(%s)\n", 
		ApiRef, lpFileName, dwDesiredAccess, ExplainDesiredAccess(dwDesiredAccess), dwShareMode, 
		dwCreationDisposition, ExplainDisposition(dwCreationDisposition),
		dwFlagsAndAttributes, ExplainFlagsAndAttributes(dwFlagsAndAttributes));

	if((dxw.dwFlags3 & BUFFEREDIOFIX) && (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)){
		OutTraceDW("%s: suppress FILE_FLAG_NO_BUFFERING on Filename=\"%ls\"\n", ApiRef, lpFileName); 
		dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;
	}

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		lpFileName = dxwTranslatePathW(lpFileName, &mapping);

		// if mapped on virtual CD and write access required, you should fake a no access error code
		if(mapping == DXW_FAKE_CD){
			if(dxw.dwFlags14 & WIN16CREATEFILE){
				if(dwDesiredAccess == (GENERIC_WRITE|GENERIC_READ)){
					OutTraceDW("%s: WIN16CREATEFILE use READ mode\n", ApiRef);
					dwDesiredAccess = GENERIC_READ;
				}
			}
			if(dwDesiredAccess & (GENERIC_WRITE|GENERIC_ALL)){
				OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
				// should set lasterror here?
				SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
				return INVALID_HANDLE_VALUE;
			}
		}
	}

	ret=(*pCreateFileW)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if(ret && (ret != INVALID_HANDLE_VALUE)) {
		err = 0;
		OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	else {
		err = GetLastError();
		OutTraceSYS("%s ERROR: err=%d fname=\"%ls\"\n", ApiRef, GetLastError(), lpFileName);
	}

	if(dxw.dwFlags14 & WIN16CREATEFILE){
		if((err == 5) && (dwDesiredAccess == (GENERIC_WRITE|GENERIC_READ))){
			OutTraceDW("%s: WIN16CREATEFILE retry with READ mode\n", ApiRef);
			SetLastError(0);
			dwDesiredAccess = GENERIC_READ;
			ret=(*pCreateFileW)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
			if(ret && (ret != INVALID_HANDLE_VALUE)) {
				OutTraceDW("%s: WIN16CREATEFILE ret=%#x\n", ApiRef, ret);
			}
			else {
				OutTraceDW("%s: WIN16CREATEFILE err=%d fname=\"%s\"\n", ApiRef, GetLastError(), lpFileName);
			}
		}
	}

	return ret;
} 

BOOL WINAPI extDeleteFileA(LPCSTR lpFileName)
{
	BOOL ret;
	ApiName("DeleteFileA");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpFileName);

	if(dxw.dwDFlags & DISABLEDELETE) {
		OutTrace("%s: DISABLEDELETE path=\"%s\"\n", ApiRef, lpFileName);
		return TRUE;
	}

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) lpFileName = dxwTranslatePathA(lpFileName, &mapping);

	if(mapping == DXW_FAKE_CD){
		OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
		// should set lasterror here?
		SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
		return FALSE;
	}

	if(dxw.dwFlags12 & PATHLOCALE){
		int size = strlen(lpFileName);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpFileName, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		ret = (*pDeleteFileW)(lpFileNameW);
		free(lpFileNameW);
	}
	else {
		ret = (*pDeleteFileA)(lpFileName);
	}
	if(!ret){
		OutErrorSYS("%s ERROR: res=%#x err=%d\n", ApiRef, ret, GetLastError());
	}

	if(dxw.dwFlags13 & IGNOREFSYSERRORS) return true;
	return ret;
}

BOOL WINAPI extDeleteFileW(LPCWSTR lpFileName)
{
	BOOL ret;
	ApiName("DeleteFileW");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%ls\"\n", ApiRef, lpFileName);

	if(dxw.dwDFlags & DISABLEDELETE) {
		OutTrace("%s: DISABLEDELETE path=\"%ls\"\n", ApiRef, lpFileName);
		return TRUE;
	}

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) lpFileName = dxwTranslatePathW(lpFileName, &mapping);

	if(mapping == DXW_FAKE_CD){
		OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
		// should set lasterror here?
		SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
		return FALSE;
	}

	ret = (*pDeleteFileW)(lpFileName);
	if(!ret){
		OutErrorSYS("%s ERROR: res=%#x err=%d\n", ApiRef, ret, GetLastError());
	}

	if(dxw.dwFlags13 & IGNOREFSYSERRORS) return true;
	return ret;
}

BOOL WINAPI extCreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecAttr)
{
	BOOL ret;
	ApiName("CreateDirectoryA");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%s\" secattr=%#x\n", ApiRef, lpPathName, lpSecAttr);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) lpPathName = dxwTranslatePathA(lpPathName, &mapping);

	if(mapping == DXW_FAKE_CD){
		OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
		// should set lasterror here?
		SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
		return FALSE;
	}

	ret = (*pCreateDirectoryA)(lpPathName, lpSecAttr);
	if(!ret){
		OutErrorSYS("%s ERROR: res=%#x err=%d\n", ApiRef, ret, GetLastError());
	}

	return ret;
}

BOOL WINAPI extRemoveDirectoryA(LPCSTR lpPathName)
{
	BOOL ret;
	ApiName("RemoveDirectoryA");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpPathName);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) lpPathName = dxwTranslatePathA(lpPathName, &mapping);

	if(mapping == DXW_FAKE_CD){
		OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
		// should set lasterror here?
		SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
		return FALSE;
	}

	ret = (*pRemoveDirectoryA)(lpPathName);
	if(!ret){
		OutErrorSYS("%s ERROR: res=%#x err=%d\n", ApiRef, ret, GetLastError());
	}

	return ret;
}

BOOL WINAPI extMoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName)
{
	BOOL ret;
	ApiName("MoveFileA");
	DWORD mapping = DXW_NO_FAKE;
	BOOL IsaCD = FALSE;
	CHAR ExistingFileName[MAX_PATH+1];

	OutTraceSYS("%s: from=\"%s\" to=\"%s\"\n", ApiRef, lpExistingFileName, lpNewFileName);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpExistingFileName = dxwTranslatePathA(lpExistingFileName, &mapping);
		strncpy(ExistingFileName, lpExistingFileName, MAX_PATH);
		lpExistingFileName = ExistingFileName;
		if(mapping == DXW_FAKE_CD) IsaCD = TRUE;
		lpNewFileName = dxwTranslatePathA(lpNewFileName, &mapping);
		if(mapping == DXW_FAKE_CD) IsaCD = TRUE;
	}

	if(IsaCD){
		OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
		// should set lasterror here?
		SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
		return FALSE;
	}

	ret = (*pMoveFileA)(lpExistingFileName, lpNewFileName);
	if(!ret){
		OutErrorSYS("%s ERROR: res=%#x err=%d\n", ApiRef, ret, GetLastError());
	}

	return ret;
}

static void completeFindDataA(LPWIN32_FIND_DATAA fd) 
{
	if(strlen(fd->cFileName) <= 12){
		memcpy(fd->cAlternateFileName, fd->cFileName, sizeof(fd->cAlternateFileName));
		fd->cAlternateFileName[sizeof(fd->cAlternateFileName) - 1] = '\0';
	}
}

static void completeFindDataW(LPWIN32_FIND_DATAW fd) 
{
	if(wcslen(fd->cFileName) <= 12){
		memcpy(fd->cAlternateFileName, fd->cFileName, sizeof(fd->cAlternateFileName));
		fd->cAlternateFileName[sizeof(fd->cAlternateFileName) - 1] = '\0';
	}
}

//#undef OutTraceSYS
//#undef OutDebugSYS
//#define OutTraceSYS OutTrace
//#define OutDebugSYS OutTrace

HANDLE WINAPI extFindFirstFileA(LPCTSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
	ApiName("FindFirstFileA");
	HANDLE ret;
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpFileName);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpFileName = dxwTranslatePathA(lpFileName, &mapping);
		dxw.dwFindFileMapping = mapping;
	}

	if(dxw.dwFlags12 & PATHLOCALE){
		// v2.06.03: @#@ "Pinal Puff" (Jp)
		int size = strlen(lpFileName);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		WIN32_FIND_DATAW FindFileDataW;
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpFileName, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		//if(!pFindFirstFileW) pFindFirstFileW = FindFirstFileW;
		ret = (*pFindFirstFileW)(lpFileNameW, &FindFileDataW);
		// revert data to ANSI structure. v2.05.71 fix.
		OutTraceLOC("%s WIDE dFileName=\"%ls\" cAlternateFileName=\"%ls\"\n", ApiRef, lpFindFileData->cFileName, lpFindFileData->cAlternateFileName);
		n = (*pWideCharToMultiByte)(dxw.CodePage, 0, FindFileDataW.cFileName, lstrlenW(FindFileDataW.cFileName), lpFindFileData->cFileName, MAX_PATH, 0, 0);
		lpFindFileData->cFileName[n] = 0;
		if(FindFileDataW.cAlternateFileName == NULL){
			lpFindFileData->cAlternateFileName[0] = NULL;
		}
		else {
			(*pWideCharToMultiByte)(dxw.CodePage, 0, FindFileDataW.cAlternateFileName, 12, lpFindFileData->cAlternateFileName, 14, 0, 0);
		}
		lpFindFileData->dwFileAttributes = FindFileDataW.dwFileAttributes;
		lpFindFileData->dwReserved0 = FindFileDataW.dwReserved0;
		lpFindFileData->dwReserved1 = FindFileDataW.dwReserved1;
		lpFindFileData->ftCreationTime = FindFileDataW.ftCreationTime;
		lpFindFileData->ftLastAccessTime = FindFileDataW.ftLastAccessTime;
		lpFindFileData->ftLastWriteTime = FindFileDataW.ftLastWriteTime;
		lpFindFileData->nFileSizeHigh = FindFileDataW.nFileSizeHigh;
		lpFindFileData->nFileSizeLow = FindFileDataW.nFileSizeLow;
	}
	else {
		ret = (*pFindFirstFileA)(lpFileName, lpFindFileData);
	}
	if((ret == 0) || (ret == INVALID_HANDLE_VALUE)){
		OutErrorSYS("%s: ERROR res=%#x err=%d\n", ApiRef, ret, GetLastError());
		return ret;
	}

	// when making FindFirstFile on a root folder, the file sequence does NOT begins with the
	// two folders "." and "..", so if making the operation on a fake drive and on a fake root
	// you have to dkip these. 
	// Found in "Precision Skateboarding" when trying to emulate the game CD.
	if(dxw.bIsRootFolder){
		if(!strcmp(lpFindFileData->cFileName, ".")) {
			OutErrorSYS("%s: skip .\n", ApiRef);
			(*pFindNextFileA)(ret, lpFindFileData);
		}
		if(!strcmp(lpFindFileData->cFileName, "..")) {
			OutErrorSYS("%s: skip ..\n", ApiRef);
			(*pFindNextFileA)(ret, lpFindFileData);
		}
	}

	if(dxw.dwFindFileMapping == DXW_FAKE_CD){	
		OutTraceDW("%s: setting CD attributes for path=\"%s\"\n", ApiRef, lpFindFileData->cFileName);
		lpFindFileData->dwFileAttributes = GetFakeAttribute(lpFindFileData->dwFileAttributes);
		lpFindFileData->ftLastAccessTime.dwLowDateTime = 0;
		lpFindFileData->ftLastAccessTime.dwHighDateTime = 0;
		
		if ((dxw.dwFlags15 & EMULATEWIN95) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)){
	        // It's a directory: flip the read-only bit
			OutTraceDW("%s: Removing directory FILE_ATTRIBUTE_READONLY\n");
			lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
		}
	}

	if(dxw.dwFlags14 & WIN16FINDFILEFIX) completeFindDataA(lpFindFileData);

	OutDebugSYS("> filename=\"%s\"\n", lpFindFileData->cFileName);
	OutDebugSYS("> altname=\"%0.14s\"\n", lpFindFileData->cAlternateFileName);
	OutDebugSYS("> attributes=%#x\n", lpFindFileData->dwFileAttributes);
	OutDebugSYS("> filetime=%#x.%#x\n", lpFindFileData->ftCreationTime.dwLowDateTime, lpFindFileData->ftCreationTime.dwHighDateTime);
	OutDebugSYS("> lastaccesstime=%#x.%#x\n", lpFindFileData->ftLastAccessTime.dwLowDateTime, lpFindFileData->ftLastAccessTime.dwHighDateTime);
	OutDebugSYS("> lastwritetime=%#x.%#x\n", lpFindFileData->ftLastWriteTime.dwLowDateTime, lpFindFileData->ftLastWriteTime.dwHighDateTime);
	
	return ret;
}

HANDLE WINAPI extFindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	ApiName("FindFirstFileW");
	HANDLE ret;
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%ls\"\n", ApiRef, lpFileName);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpFileName = dxwTranslatePathW(lpFileName, &mapping);
		dxw.dwFindFileMapping = mapping;
	}

	ret = (*pFindFirstFileW)(lpFileName, lpFindFileData);

	if((ret == 0) || (ret == INVALID_HANDLE_VALUE)){
		OutErrorSYS("%s: ERROR res=%#x err=%d\n", ApiRef, ret, GetLastError());
		return ret;
	}

	// when making FindFirstFile on a root folder, the file sequence does NOT begins with the
	// two folders "." and "..", so if making the operation on a fake drive and on a fake root
	// you have to dkip these. 
	// Found in "Precision Skateboarding" when trying to emulate the game CD.
	if(dxw.bIsRootFolder){
		if(!wcscmp(lpFindFileData->cFileName, L".")) {
			OutErrorSYS("%s: skip .\n", ApiRef);
			(*pFindNextFileW)(ret, lpFindFileData);
		}
		if(!wcscmp(lpFindFileData->cFileName, L"..")) {
			OutErrorSYS("%s: skip ..\n", ApiRef);
			(*pFindNextFileW)(ret, lpFindFileData);
		}
	}

	if(dxw.dwFindFileMapping == DXW_FAKE_CD){	
		OutTraceDW("%s: setting CD attributes for path=\"%ls\"\n", ApiRef, lpFindFileData->cFileName);
		lpFindFileData->dwFileAttributes = GetFakeAttribute(lpFindFileData->dwFileAttributes);
		lpFindFileData->ftLastAccessTime.dwLowDateTime = 0;
		lpFindFileData->ftLastAccessTime.dwHighDateTime = 0;

		if ((dxw.dwFlags15 & EMULATEWIN95) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)){
	        // It's a directory: flip the read-only bit
			OutTraceDW("%s: Removing directory FILE_ATTRIBUTE_READONLY\n");
			lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
		}
	}

	if(dxw.dwFlags14 & WIN16FINDFILEFIX) completeFindDataW(lpFindFileData);

	OutDebugSYS("> filename=\"%ls\"\n", lpFindFileData->cFileName);
	OutDebugSYS("> altname=\"%0.14ls\"\n", lpFindFileData->cAlternateFileName);
	OutDebugSYS("> attributes=%#x\n", lpFindFileData->dwFileAttributes);
	OutDebugSYS("> filetime=%#x.%#x\n", lpFindFileData->ftCreationTime.dwLowDateTime, lpFindFileData->ftCreationTime.dwHighDateTime);
	OutDebugSYS("> lastaccesstime=%#x.%#x\n", lpFindFileData->ftLastAccessTime.dwLowDateTime, lpFindFileData->ftLastAccessTime.dwHighDateTime);
	OutDebugSYS("> lastwritetime=%#x.%#x\n", lpFindFileData->ftLastWriteTime.dwLowDateTime, lpFindFileData->ftLastWriteTime.dwHighDateTime);
	
	return ret;
}

/* 
BEWARE: wrong doc not aligned with WinBase.h !!!

FindExInfoStandard: 
	The FindFirstFileEx function retrieves a standard set of attribute information. 
	The data is returned in a WIN32_FIND_DATA structure.
FindExInfoBasic: 
	The FindFirstFileEx function does not query the short file name, improving overall enumeration speed. 
	The data is returned in a WIN32_FIND_DATA structure, and the cAlternateFileName member is always a NULL string.

Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported until Windows Server 2008 R2 and Windows 7.
FindExInfoMaxInfoLevel
This value is used for validation. Supported values are less than this value.

typedef enum _FINDEX_INFO_LEVELS {
  FindExInfoStandard,
  FindExInfoBasic,
  FindExInfoMaxInfoLevel
} FINDEX_INFO_LEVELS;
*/

HANDLE WINAPI extFindFirstFileExA(LPCSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPWIN32_FIND_DATAA lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags)
{
	ApiName("FindFirstFileExA");
	HANDLE ret;
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpFileName);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpFileName = dxwTranslatePathA(lpFileName, &mapping);
		dxw.dwFindFileMapping = mapping;
	}

	if(dxw.dwFlags12 & PATHLOCALE){
		// v2.06.03 fix
		int size = strlen(lpFileName);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		WIN32_FIND_DATAW FindFileDataW;
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpFileName, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		ret = (*pFindFirstFileExW)(lpFileNameW, fInfoLevelId, &FindFileDataW, fSearchOp, lpSearchFilter, dwAdditionalFlags);
		// revert data to ANSI structure. v2.05.71 fix.
		OutTraceLOC("%s WIDE dFileName=\"%ls\" cAlternateFileName=\"%ls\"\n", ApiRef, lpFindFileData->cFileName, lpFindFileData->cAlternateFileName);
		n = (*pWideCharToMultiByte)(dxw.CodePage, 0, FindFileDataW.cFileName, lstrlenW(FindFileDataW.cFileName), lpFindFileData->cFileName, MAX_PATH, 0, 0);
		lpFindFileData->cFileName[n] = 0;
		if(FindFileDataW.cAlternateFileName == NULL){
			lpFindFileData->cAlternateFileName[0] = NULL;
		}
		else {
			(*pWideCharToMultiByte)(dxw.CodePage, 0, FindFileDataW.cAlternateFileName, 12, lpFindFileData->cAlternateFileName, 14, 0, 0);
		}
		lpFindFileData->dwFileAttributes = FindFileDataW.dwFileAttributes;
		lpFindFileData->dwReserved0 = FindFileDataW.dwReserved0;
		lpFindFileData->dwReserved1 = FindFileDataW.dwReserved1;
		lpFindFileData->ftCreationTime = FindFileDataW.ftCreationTime;
		lpFindFileData->ftLastAccessTime = FindFileDataW.ftLastAccessTime;
		lpFindFileData->ftLastWriteTime = FindFileDataW.ftLastWriteTime;
		lpFindFileData->nFileSizeHigh = FindFileDataW.nFileSizeHigh;
		lpFindFileData->nFileSizeLow = FindFileDataW.nFileSizeLow;
	}
	else {
		ret = (*pFindFirstFileExA)(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
	}

	if((ret == 0) || (ret == INVALID_HANDLE_VALUE)){
		OutErrorSYS("%s: ERROR res=%#x err=%d\n", ApiRef, ret, GetLastError());
		return ret;
	}

#if 0
	// when making FindFirstFile on a root folder, the file sequence does NOT begins with the
	// two folders "." and "..", so if making the operation on a fake drive and on a fake root
	// you have to dkip these. 
	// Found in "Precision Skateboarding" when trying to emulate the game CD.
	if(dxw.bIsRootFolder){
		if(!strcmp(lpFindFileData->cFileName, ".")) {
			OutErrorSYS("%s: skip .\n", ApiRef);
			(*pFindNextFileA)(ret, lpFindFileData);
		}
		if(!strcmp(lpFindFileData->cFileName, "..")) {
			OutErrorSYS("%s: skip ..\n", ApiRef);
			(*pFindNextFileA)(ret, lpFindFileData);
		}
	}

	if(dxw.dwFindFileMapping == DXW_FAKE_CD){	
		OutTraceDW("%s: setting CD attributes for path=\"%s\"\n", ApiRef, lpFindFileData->cFileName);
		lpFindFileData->dwFileAttributes = GetFakeAttribute(lpFindFileData->dwFileAttributes);
		lpFindFileData->ftLastAccessTime.dwLowDateTime = 0;
		lpFindFileData->ftLastAccessTime.dwHighDateTime = 0;
		
		if ((dxw.dwFlags15 & EMULATEWIN95) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)){
	        // It's a directory: flip the read-only bit
			OutTraceDW("%s: Removing directory FILE_ATTRIBUTE_READONLY\n");
			lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
		}
	}

	if(dxw.dwFlags14 & WIN16FINDFILEFIX) completeFindDataA(lpFindFileData);

	OutDebugSYS("> filename=\"%s\"\n", lpFindFileData->cFileName);
	OutDebugSYS("> altname=\"%0.14s\"\n", lpFindFileData->cAlternateFileName);
	OutDebugSYS("> attributes=%#x\n", lpFindFileData->dwFileAttributes);
	OutDebugSYS("> filetime=%#x.%#x\n", lpFindFileData->ftCreationTime.dwLowDateTime, lpFindFileData->ftCreationTime.dwHighDateTime);
	OutDebugSYS("> lastaccesstime=%#x.%#x\n", lpFindFileData->ftLastAccessTime.dwLowDateTime, lpFindFileData->ftLastAccessTime.dwHighDateTime);
	OutDebugSYS("> lastwritetime=%#x.%#x\n", lpFindFileData->ftLastWriteTime.dwLowDateTime, lpFindFileData->ftLastWriteTime.dwHighDateTime);
#endif
	return ret;
}

HANDLE WINAPI extFindFirstFileExW(LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPWIN32_FIND_DATAW lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags)
{
	ApiName("FindFirstFileExW");
	HANDLE ret;
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%ls\"\n", ApiRef, lpFileName);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpFileName = dxwTranslatePathW(lpFileName, &mapping);
		dxw.dwFindFileMapping = mapping;
	}

	ret = (*pFindFirstFileExW)(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);

	if((ret == 0) || (ret == INVALID_HANDLE_VALUE)){
		OutErrorSYS("%s: ERROR res=%#x err=%d\n", ApiRef, ret, GetLastError());
		return ret;
	}

#if 0
	// when making FindFirstFile on a root folder, the file sequence does NOT begins with the
	// two folders "." and "..", so if making the operation on a fake drive and on a fake root
	// you have to dkip these. 
	// Found in "Precision Skateboarding" when trying to emulate the game CD.
	if(dxw.bIsRootFolder){
		if(!wcscmp(lpFindFileData->cFileName, L".")) {
			OutErrorSYS("%s: skip .\n", ApiRef);
			(*pFindNextFileW)(ret, lpFindFileData);
		}
		if(!wcscmp(lpFindFileData->cFileName, L"..")) {
			OutErrorSYS("%s: skip ..\n", ApiRef);
			(*pFindNextFileW)(ret, lpFindFileData);
		}
	}

	if(dxw.dwFindFileMapping == DXW_FAKE_CD){	
		OutTraceDW("%s: setting CD attributes for path=\"%ls\"\n", ApiRef, lpFindFileData->cFileName);
		lpFindFileData->dwFileAttributes = GetFakeAttribute(lpFindFileData->dwFileAttributes);
		lpFindFileData->ftLastAccessTime.dwLowDateTime = 0;
		lpFindFileData->ftLastAccessTime.dwHighDateTime = 0;

		if ((dxw.dwFlags15 & EMULATEWIN95) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)){
	        // It's a directory: flip the read-only bit
			OutTraceDW("%s: Removing directory FILE_ATTRIBUTE_READONLY\n");
			lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
		}
	}

	if(dxw.dwFlags14 & WIN16FINDFILEFIX) completeFindDataW(lpFindFileData);

	OutDebugSYS("> filename=\"%ls\"\n", lpFindFileData->cFileName);
	OutDebugSYS("> altname=\"%0.14ls\"\n", lpFindFileData->cAlternateFileName);
	OutDebugSYS("> attributes=%#x\n", lpFindFileData->dwFileAttributes);
	OutDebugSYS("> filetime=%#x.%#x\n", lpFindFileData->ftCreationTime.dwLowDateTime, lpFindFileData->ftCreationTime.dwHighDateTime);
	OutDebugSYS("> lastaccesstime=%#x.%#x\n", lpFindFileData->ftLastAccessTime.dwLowDateTime, lpFindFileData->ftLastAccessTime.dwHighDateTime);
	OutDebugSYS("> lastwritetime=%#x.%#x\n", lpFindFileData->ftLastWriteTime.dwLowDateTime, lpFindFileData->ftLastWriteTime.dwHighDateTime);
#endif
	return ret;
}

BOOL WINAPI extFindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
	BOOL ret;
	ApiName("FindNextFileA");

	OutTraceSYS("%s: hfile=%#x\n", ApiRef, hFindFile);

	ret=(*pFindNextFileA)(hFindFile, lpFindFileData);
	if(!ret) {
#ifndef DXW_NOTRACES
		int err = GetLastError();
		if(err == ERROR_NO_MORE_FILES){
			OutTraceSYS("%s: NO_MORE_FILES\n", ApiRef);
		}
		else {
			OutErrorSYS("%s: ERROR: err=%d\n", ApiRef, err);
		}
#endif // DXW_NOTRACES
		return ret;
	}

	if(dxw.dwFindFileMapping == DXW_FAKE_CD){	
		OutTraceDW("%s: setting CD attributes for path=\"%s\"\n", ApiRef, lpFindFileData->cFileName);
		lpFindFileData->dwFileAttributes = GetFakeAttribute(lpFindFileData->dwFileAttributes);
		lpFindFileData->ftLastAccessTime.dwLowDateTime = 0;
		lpFindFileData->ftLastAccessTime.dwHighDateTime = 0;

		if ((dxw.dwFlags15 & EMULATEWIN95) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)){
	        // It's a directory: flip the read-only bit
			OutTraceDW("%s: Removing directory FILE_ATTRIBUTE_READONLY\n");
			lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
		}
	}

	if(dxw.dwFlags14 & WIN16FINDFILEFIX) completeFindDataA(lpFindFileData);

	OutDebugSYS("> filename=\"%s\"\n", lpFindFileData->cFileName);
	OutDebugSYS("> altname=\"%0.14s\"\n", lpFindFileData->cAlternateFileName);
	OutDebugSYS("> attributes=%#x\n", lpFindFileData->dwFileAttributes);
	OutDebugSYS("> filetime=%#x.%#x\n", lpFindFileData->ftCreationTime.dwLowDateTime, lpFindFileData->ftCreationTime.dwHighDateTime);
	OutDebugSYS("> lastaccesstime=%#x.%#x\n", lpFindFileData->ftLastAccessTime.dwLowDateTime, lpFindFileData->ftLastAccessTime.dwHighDateTime);
	OutDebugSYS("> lastwritetime=%#x.%#x\n", lpFindFileData->ftLastWriteTime.dwLowDateTime, lpFindFileData->ftLastWriteTime.dwHighDateTime);

	return ret;
}

BOOL WINAPI extFindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	BOOL ret;
	ApiName("FindNextFileW");

	OutTraceSYS("%s: hfile=%#x\n", ApiRef, hFindFile);

	ret=(*pFindNextFileW)(hFindFile, lpFindFileData);
	if(!ret) {
#ifndef DXW_NOTRACES
		int err = GetLastError();
		if(err == ERROR_NO_MORE_FILES){
			OutTraceSYS("%s: NO_MORE_FILES\n", ApiRef);
		}
		else {
			OutErrorSYS("%s: ERROR: err=%d\n", ApiRef, err);
		}
#endif // DXW_NOTRACES
		return ret;
	}

	if(dxw.dwFindFileMapping == DXW_FAKE_CD){	
		OutTraceDW("%s: setting CD attributes for path=\"%ls\"\n", ApiRef, lpFindFileData->cFileName);
		lpFindFileData->dwFileAttributes = GetFakeAttribute(lpFindFileData->dwFileAttributes);
		lpFindFileData->ftLastAccessTime.dwLowDateTime = 0;
		lpFindFileData->ftLastAccessTime.dwHighDateTime = 0;

		if ((dxw.dwFlags15 & EMULATEWIN95) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)){
	        // It's a directory: flip the read-only bit
			OutTraceDW("%s: Removing directory FILE_ATTRIBUTE_READONLY\n");
			lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
		}
	}

	if(dxw.dwFlags14 & WIN16FINDFILEFIX) completeFindDataW(lpFindFileData);

	OutDebugSYS("> filename=\"%ls\"\n", lpFindFileData->cFileName);
	OutDebugSYS("> altname=\"%0.14ls\"\n", lpFindFileData->cAlternateFileName);
	OutDebugSYS("> attributes=%#x\n", lpFindFileData->dwFileAttributes);
	OutDebugSYS("> filetime=%#x.%#x\n", lpFindFileData->ftCreationTime.dwLowDateTime, lpFindFileData->ftCreationTime.dwHighDateTime);
	OutDebugSYS("> lastaccesstime=%#x.%#x\n", lpFindFileData->ftLastAccessTime.dwLowDateTime, lpFindFileData->ftLastAccessTime.dwHighDateTime);
	OutDebugSYS("> lastwritetime=%#x.%#x\n", lpFindFileData->ftLastWriteTime.dwLowDateTime, lpFindFileData->ftLastWriteTime.dwHighDateTime);

	return ret;
}

//#undef OutTraceSYS
//#undef OutDebugSYS
//#define OutTraceSYS(f, ...) if(IsTraceSYS) OutTrace(f, __VA_ARGS__)
//#define OutDebugSYS(f, ...) if(IsDebugSYS) OutTrace(f, __VA_ARGS__)

DWORD WINAPI extGetFileAttributesA(LPCSTR lpFileName)
{
	DWORD ret;
	ApiName("GetFileAttributesA");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpFileName);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpFileName = dxwTranslatePathA(lpFileName, &mapping);
		//if(mapping) {
		//	OutTraceDW("GetFileAttributesA: FAKE2REAL path=\"%s\"\n", lpFileName);
		//}
	}

	ret=(*pGetFileAttributesA)(lpFileName);
	if(ret == INVALID_FILE_ATTRIBUTES) {
		OutErrorSYS("%s: ERROR err=%d path=\"%s\"\n", ApiRef, GetLastError(), lpFileName);
		return ret;
	}

	// v2.04.91: curiously, folders in CD drives don't get the FILE_ATTRIBUTE_READONLY but only
	// keep FILE_ATTRIBUTE_DIRECTORY. Fixes "Scooby Doo Case File #1".
	OutTraceSYS("%s: attributes=%#x\n", ApiRef, ret);
	if(mapping == DXW_FAKE_CD) ret = GetFakeAttribute(ret);
	return ret;
}

static void strncpyx(char *dest, char *src, UINT len)
{
	for(; *src && len; dest++, src++, len--) *dest = *src;
	if(len) *dest = 0;
}

static char hex2i(char b)
{
	BYTE res = 0;
	if((b>='0') && (b<='9')) res = b-'0';
	if((b>='A') && (b<='F')) res = 10+(b-'A');
	if((b>='a') && (b<='f')) res = 10+(b-'a');
	return res;
}

BOOL WINAPI extGetVolumeInformationA(LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
	 LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
	BOOL ret;
	ApiName("GetVolumeInformationA");

	if(dxw.bHintActive) ShowHint(HINT_CDCHECK);

	OutTraceSYS("%s: root=\"%s\" vnamesize=%d fsnamesize=%d\n", 
		ApiRef, 
		lpRootPathName ? lpRootPathName : "(NULL)", 
		nVolumeNameSize, nFileSystemNameSize);
	ret = (*pGetVolumeInformationA)(lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,
		lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);

	// v2.05.51: skip control when lpRootPathName is NULL or there will be a crash. Fixes "Diggles".
	// v2.06.02: fixed CDHACK logic
	// @#@ "Boomerang Fighter" (Ko 2000)
	// v2.06.04: fixed logic when lpRootPathName is NULL but the current folder is a fake one
	// @#@ "Robot Commander" (Ch 1996)
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE))) {
		DWORD devType = DXW_NO_FAKE;
		if(lpRootPathName == NULL) {
			int res;
			res = (*pGetCurrentDirectoryA)(0, NULL);
			if(!res) {
				OutTraceSYS("%s: GetCurrentDirectoryA ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
				return 0;
			}
			char *fullBuffer = (char *)malloc(res + 1); // len + terminator
			char *lpPathName;
			res = (*pGetCurrentDirectoryA)(res, fullBuffer);
			OutTraceSYS("%s: actual res=%d buf=\"%s\"\n", ApiRef, ret, fullBuffer);
			lpPathName = (char *)dxwUntranslatePathA((LPCSTR)fullBuffer, &devType);
			free(fullBuffer);
		}
		else {
			// v2.06.04 fix: @#@ "SkullCracker"
			if((strlen(lpRootPathName) > 1) && (lpRootPathName[1]==':')){
				devType = dxwVirtualDriveTypeA(lpRootPathName);
			}
			else {
				lpRootPathName = (char *)dxwUntranslatePathA((LPCSTR)lpRootPathName, &devType);
			}
		}
		switch(devType){
			case DXW_FAKE_HD:
				// on fake HD return same info as C:
				OutTraceSYS("%s: getting C: volume info for fake HD drive %c:\n", ApiRef, dxw.FakeHDDrive);
				ret = (*pGetVolumeInformationA)("C:\\", lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,
					lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);
				ret = TRUE;
				break;
			case DXW_FAKE_CD:
				// lpMaximumComponentLength and lpFileSystemFlags borrowed from "Battle of Heroes"
				OutTraceSYS("%s: getting volume info for fake CD drive %c:\n", ApiRef, dxw.FakeCDDrive);
				//PeekHotKeys();
				// v2.06.00: it seems that the strncpy call is causing troubles here!
				//if(lpVolumeNameBuffer) strncpy(lpVolumeNameBuffer, dxwGetCDVolumeName(), nVolumeNameSize);
				if(lpVolumeNameBuffer) strncpyx(lpVolumeNameBuffer, dxwGetCDVolumeName(), nVolumeNameSize);
				//if(lpFileSystemNameBuffer) strncpy(lpFileSystemNameBuffer, "CDFS", nFileSystemNameSize);
				if(lpFileSystemNameBuffer) strncpyx(lpFileSystemNameBuffer, "CDFS", nFileSystemNameSize);
				if(lpMaximumComponentLength) *lpMaximumComponentLength = 110;
				if(lpFileSystemFlags) *lpFileSystemFlags = 0x1080005;
				if(lpVolumeSerialNumber) *lpVolumeSerialNumber = 0x1080005; // ????
				if(dxw.dwFlags17 & CDHACK){
					// v2.06.04: handling different volume values
					OutTraceSYS("%s: CDHACK reading cdinfo.txt [volume CD%d] values\n", ApiRef, dxw.DataCDIndex + 1);
					char room[20];
					strcpy(room, "volume");
					dxw.DataCDIndex = GetHookInfo()->CDIndex;
					if(dxw.DataCDIndex > 0) sprintf(room, "volume%d", dxw.DataCDIndex + 1);
					char *cdhack = ".\\cdinfo.txt";
					if(lpVolumeNameBuffer){
						char VolumeNameBuffer[80+1];
						char VolumeNameHexBuffer[80+1];
						char VolumeNameUTF8Buffer[40+1];
						(*pGetPrivateProfileStringA)(room, "volumeName", "-", VolumeNameBuffer, 80, cdhack);
						if(strcmp("-", VolumeNameBuffer)){
							strncpy(lpVolumeNameBuffer, VolumeNameBuffer, nVolumeNameSize);
						}
						(*pGetPrivateProfileStringA)(room, "volumeNameHex", "-", VolumeNameHexBuffer, 80, cdhack);
						if(strcmp("-", VolumeNameHexBuffer)){
							UINT i;
							for(i=0; i<strlen(VolumeNameHexBuffer); i+=2){
								VolumeNameUTF8Buffer[i/2] = (16 * hex2i(VolumeNameHexBuffer[i])) + hex2i(VolumeNameHexBuffer[i+1]);
							}
							VolumeNameUTF8Buffer[i/2] = 0; // terminator
							memcpy(lpVolumeNameBuffer, VolumeNameUTF8Buffer, i);
						}
						OutTraceDW("%s: fixed VolumeName=\"%s\"\n", ApiRef, lpVolumeNameBuffer);
					}
					if(lpFileSystemNameBuffer) {
						char FileSystemNameHexBuffer[80+1];
						char FileSystemNameUTF8Buffer[40+1];
						(*pGetPrivateProfileStringA)(room, "fileSystemName", "CDFS", lpFileSystemNameBuffer, nFileSystemNameSize, cdhack);
						(*pGetPrivateProfileStringA)(room, "fileSystemNameHex", "-", FileSystemNameHexBuffer, 80, cdhack);
						if(strcmp("-", FileSystemNameHexBuffer)){
							UINT i;
							for(i=0; i<strlen(FileSystemNameHexBuffer); i+=2){
								FileSystemNameUTF8Buffer[i/2] = (16 * hex2i(FileSystemNameHexBuffer[i])) + hex2i(FileSystemNameHexBuffer[i+1]);
							}
							FileSystemNameUTF8Buffer[i/2] = 0; // terminator
							strncpy(lpFileSystemNameBuffer, FileSystemNameUTF8Buffer, nFileSystemNameSize);
						}
						if(strcmp(lpFileSystemNameBuffer, "CDFS")) OutTraceDW("%s: fixed FileSystemName=\"%s\"\n", ApiRef, lpFileSystemNameBuffer);
					}
					if(lpVolumeSerialNumber) {
						*lpVolumeSerialNumber = (*pGetPrivateProfileIntA)(room, "serialNumber", 0x1080005, cdhack);
						if(*lpVolumeSerialNumber != 0x1080005) OutTraceDW("%s: fixed VolumeSerialNumber=%#x\n", ApiRef, *lpVolumeSerialNumber);
					}
					if(lpMaximumComponentLength) {
						*lpMaximumComponentLength = (*pGetPrivateProfileIntA)(room, "maxComponentLen", 110, cdhack);
						if(*lpMaximumComponentLength != 110) OutTraceDW("%s: fixed MaximumComponentLength=%d\n", ApiRef, *lpMaximumComponentLength);
					}
					if(lpFileSystemFlags) {
						*lpFileSystemFlags = (*pGetPrivateProfileIntA)(room, "fsFlags", 0x1080005, cdhack);
						if(*lpFileSystemFlags != 0x1080005) OutTraceDW("%s: fixed FileSystemFlags=%#x\n", ApiRef, *lpFileSystemFlags);
					}
				}
				ret = TRUE;
				break;
		}
	}

	if(!ret) {
		OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
		return ret;
	}

	if(lpVolumeNameBuffer) {
		OutTraceSYS("> vname=\"%s\"\n", lpVolumeNameBuffer);
	}
	if(lpFileSystemNameBuffer) {
		OutTraceSYS("> fsname=\"%s\"\n", lpFileSystemNameBuffer);
	}
	if(lpVolumeSerialNumber) {
		OutTraceSYS("> serial=%08X\n", *lpVolumeSerialNumber);
	}
	if(lpMaximumComponentLength) {
		OutTraceSYS("> maxcomplen=%d\n", *lpMaximumComponentLength);
	}
	if(lpFileSystemFlags) {
		OutTraceSYS("> fsflags=%08.0X\n", *lpFileSystemFlags);
	}
	return ret;
}

BOOL WINAPI extGetVolumeInformationW(LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
	 LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
	BOOL ret;
	ApiName("GetVolumeInformationW");

	if(dxw.bHintActive) ShowHint(HINT_CDCHECK);

	OutTraceSYS("%s: root=\"%ls\" vnamesize=%d fsnamesize=%d\n", 
		ApiRef, 
		lpRootPathName ? lpRootPathName : L"(NULL)", 
		nVolumeNameSize, nFileSystemNameSize);
	ret = (*pGetVolumeInformationW)(lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,
		lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);

	// v2.05.51: skip control when lpRootPathName is NULL or there will be a crash.
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE))) {
		DWORD devType = DXW_NO_FAKE;
		if(lpRootPathName == NULL){
			int res;
			res = (*pGetCurrentDirectoryW)(0, NULL);
			if(!res) {
				OutTraceSYS("%s: GetCurrentDirectoryW ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
				return 0;
			}
			WCHAR *fullBuffer = (WCHAR *)malloc((res + 1) * sizeof(WCHAR)); // len + terminator
			WCHAR *lpPathName;
			res = (*pGetCurrentDirectoryW)(res, fullBuffer);
			OutTraceSYS("%s: actual res=%d buf=\"%ls\"\n", ApiRef, ret, fullBuffer);
			lpPathName = (WCHAR *)dxwUntranslatePathW((LPWSTR)fullBuffer, &devType);
			free(fullBuffer);
		}
		else {
			// v2.06.04 fix: tentative WCHAR version of ANSI fix
			if((wcslen(lpRootPathName) > 1) && (lpRootPathName[1]==L':')){
				devType = dxwVirtualDriveTypeW(lpRootPathName);
			}
			else {
				lpRootPathName = (LPCWSTR)dxwUntranslatePathW((LPWSTR)lpRootPathName, &devType);
			}
		}
		switch(devType){
			case DXW_FAKE_HD:
				// on fake HD return same info as C:
				OutTraceSYS("%s: getting C: volume info for fake HD drive %c:\n", ApiRef, dxw.FakeHDDrive);
				ret = (*pGetVolumeInformationW)(L"C:\\", lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,
					lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);
				ret = TRUE;
				break;
			case DXW_FAKE_CD:
				// lpMaximumComponentLength and lpFileSystemFlags borrowed from "Battle of Heroes"
				OutTraceSYS("%s: getting volume info for fake CD drive %c:\n", ApiRef, dxw.FakeCDDrive);
				PeekHotKeys();
				if(lpVolumeNameBuffer) {
					char *voumeName = dxwGetCDVolumeName();
					WCHAR volumeNameW[256];
					mbstowcs(volumeNameW,voumeName,256);
					wcsncpy(lpVolumeNameBuffer, volumeNameW, nVolumeNameSize);
				}
				if(lpFileSystemNameBuffer) wcsncpy(lpFileSystemNameBuffer, L"CDFS", nFileSystemNameSize);
				if(lpMaximumComponentLength) *lpMaximumComponentLength = 110;
				if(lpFileSystemFlags) *lpFileSystemFlags = 0x1080005;

				if(dxw.dwFlags17 & CDHACK){
					// v2.06.04: handling different volume values
					OutTraceSYS("%s: CDHACK reading cdinfo.txt [volume CD%d] values\n", ApiRef, dxw.DataCDIndex + 1);
					WCHAR room[20];
					wcscpy(room, L"volume");
					dxw.DataCDIndex = GetHookInfo()->CDIndex;
					if(dxw.DataCDIndex > 0) swprintf(room, 20, L"volume%d", dxw.DataCDIndex + 1);
					WCHAR *cdhack = L".\\cdinfo.txt";
					if(lpVolumeNameBuffer){
						WCHAR VolumeNameBuffer[80+1];
						//WCHAR VolumeNameHexBuffer[80+1];
						//WCHAR VolumeNameUTF8Buffer[40+1];
						(*pGetPrivateProfileStringW)(room, L"volumeName", L"-", VolumeNameBuffer, 80, cdhack);
						if(wcscmp(L"-", VolumeNameBuffer)){
							wcsncpy(lpVolumeNameBuffer, VolumeNameBuffer, nVolumeNameSize);
						}
						//(*pGetPrivateProfileStringW)(room, L"volumeNameHex", L"-", VolumeNameHexBuffer, 80, cdhack);
						//if(wcscmp(L"-", VolumeNameHexBuffer)){
						//	UINT i;
						//	for(i=0; i<strlen(VolumeNameHexBuffer); i+=2){
						//		VolumeNameUTF8Buffer[i/2] = (16 * hex2i(VolumeNameHexBuffer[i])) + hex2i(VolumeNameHexBuffer[i+1]);
						//	}
						//	VolumeNameUTF8Buffer[i/2] = 0; // terminator
						//	memcpy(lpVolumeNameBuffer, VolumeNameUTF8Buffer, i);
						//}
						OutTraceDW("%s: fixed VolumeName=\"%ls\"\n", ApiRef, lpVolumeNameBuffer);
					}

					if(lpFileSystemNameBuffer) {
						//WCHAR FileSystemNameHexBuffer[80+1];
						//WCHAR FileSystemNameUTF8Buffer[40+1];
						(*pGetPrivateProfileStringW)(room, L"fileSystemName", L"CDFS", lpFileSystemNameBuffer, nFileSystemNameSize, cdhack);
						//(*pGetPrivateProfileStringW)(room, L"fileSystemNameHex", L"-", FileSystemNameHexBuffer, 80, wcdhack);
						//if(wcscmp(L"-", FileSystemNameHexBuffer)){
						//	UINT i;
						//	for(i=0; i<strlen(FileSystemNameHexBuffer); i+=2){
						//		FileSystemNameUTF8Buffer[i/2] = (16 * hex2i(FileSystemNameHexBuffer[i])) + hex2i(FileSystemNameHexBuffer[i+1]);
						//	}
						//	FileSystemNameUTF8Buffer[i/2] = 0; // terminator
						//	strncpy(lpFileSystemNameBuffer, FileSystemNameUTF8Buffer, nFileSystemNameSize);
						//}
						if(wcscmp(lpFileSystemNameBuffer, L"CDFS")) OutTraceDW("%s: fixed FileSystemName=\"%ls\"\n", ApiRef, lpFileSystemNameBuffer);
					}
					if(lpVolumeSerialNumber) {
						*lpVolumeSerialNumber = (*pGetPrivateProfileIntW)(room, L"serialNumber", 0x1080005, cdhack);
						if(*lpVolumeSerialNumber != 0x1080005) OutTraceDW("%s: fixed VolumeSerialNumber=%#x\n", ApiRef, *lpVolumeSerialNumber);
					}
					if(lpMaximumComponentLength) {
						*lpMaximumComponentLength = (*pGetPrivateProfileIntW)(room, L"maxComponentLen", 110, cdhack);
						if(*lpMaximumComponentLength != 110) OutTraceDW("%s: fixed MaximumComponentLength=%d\n", ApiRef, *lpMaximumComponentLength);
					}
					if(lpFileSystemFlags) {
						*lpFileSystemFlags = (*pGetPrivateProfileIntW)(room, L"fsFlags", 0x1080005, cdhack);
						if(*lpFileSystemFlags != 0x1080005) OutTraceDW("%s: fixed FileSystemFlags=%#x\n", ApiRef, *lpFileSystemFlags);
					}
				}

				ret = TRUE;
				break;
		}
	}

	if(!ret) {
		OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
		return ret;
	}

	if(lpVolumeNameBuffer) {
		OutTraceSYS("> vname=\"%ls\"\n", lpVolumeNameBuffer);
	}
	if(lpFileSystemNameBuffer) {
		OutTraceSYS("> fsname=\"%ls\"\n", lpFileSystemNameBuffer);
	}
	if(lpVolumeSerialNumber) {
		OutTraceSYS("> serial=%08X\n", *lpVolumeSerialNumber);
	}
	if(lpMaximumComponentLength) {
		OutTraceSYS("> maxcomplen=%d\n", *lpMaximumComponentLength);
	}
	if(lpFileSystemFlags) {
		OutTraceSYS("> fsflags=%08.0X\n", *lpFileSystemFlags);
	}
	return ret;
}

#define MAXDRIVESPACE ((('Z' - 'A' + 1) * 4) + 1)

static void deviceEraseA(char *p)
{
	char *pCurr = p;
	char *pNext = p + strlen(p) + 1;
	while(*pNext) {
		strcpy(pCurr, pNext);
		pNext += strlen(pNext) + 1;
		pCurr += strlen(pCurr) + 1;
	}
	*pCurr++ = 0;
	*pCurr = 0;
}

DWORD WINAPI extGetLogicalDriveStringsA(DWORD nBufferLength, LPCSTR lpBuffer)
{
	DWORD ret;
	char *p;
	ApiName("GetLogicalDriveStringsA");

	OutTraceSYS("%s: buflen=%d\n", ApiRef, nBufferLength);

	char *sTmpBuffer = (char *)malloc(MAXDRIVESPACE);
	ret=(*pGetLogicalDriveStringsA)(MAXDRIVESPACE, sTmpBuffer);
	if(!ret){
		OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
		free(sTmpBuffer);
		return ret;
	}
	else {
		OutDebugSYS("%s: actual ret=%d\n", ApiRef, ret);
	}

	if(dxw.dwFlags17 & HIDECDROMREAL){
		UINT driveType;
		p = (char *)sTmpBuffer;
		while(*p){
			driveType = (*pGetDriveTypeA)(p);
			//OutTrace("%s: drive %s type=%d\n", ApiRef, p, driveType);
			if(driveType == DRIVE_CDROM){
				OutTraceDW("%s: HIDECDROMREAL - hide real CD drive %s\n", ApiRef, p);
				// delete the drive from the list
				deviceEraseA(p);			}
			else {
				p += strlen(p) + 1;
			}
		}
	}

	if(dxw.dwFlags4 & HIDECDROMEMPTY){
		p = (char *)sTmpBuffer;
		while(*p){
			BOOL Vol;
			Vol = (*pGetVolumeInformationA)(p, NULL, NULL, NULL, 0, 0, 0, 0);
			if(!Vol) {
				OutTraceSYS("%s: HIDECDROMEMPTY - hide empty CDROM drive %s\n", ApiRef, p); 
				deviceEraseA(p);			}
			else {
				p += strlen(p) + 1;
			}
		}
	}

	// should insert virtual CD/HD drives here ...
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		BOOL bMustAddHD = (dxw.dwFlags10 & FAKEHDDRIVE);
		BOOL bMustAddCD = (dxw.dwFlags10 & FAKECDDRIVE);
		ret = 1; // to count for the final terminator
		// reach the end of the list and verify the need to add drives
		p = (char *)sTmpBuffer;
		while(*p){
			if(bMustAddHD && (toupper(*p) == dxw.FakeHDDrive)) bMustAddHD = FALSE;
			if(bMustAddCD && (toupper(*p) == dxw.FakeCDDrive)) bMustAddCD = FALSE;
			ret += strlen(p) + 1;
			p += strlen(p) + 1;
		}
		if(bMustAddHD){
			sprintf(p, "%c:\\", dxw.FakeHDDrive);
			ret += 4;
			p += 4;
		}
		if(bMustAddCD){
			sprintf(p, "%c:\\", dxw.FakeCDDrive);
			ret += 4;
			p += 4;
		}
		*p = 0;
	}

	p = (char *)sTmpBuffer;
	while(*p){
		OutTraceSYS("> %s\n", p);
		p += strlen(p) + 1;
	}

	// copy to destination buffer for available space only
	// v2.05.32: trim if user provided too high value! Fixes "Nox" crash.
	if(lpBuffer) memcpy((char *)lpBuffer, sTmpBuffer, (nBufferLength > MAXDRIVESPACE) ? MAXDRIVESPACE : nBufferLength);
	free(sTmpBuffer);
	OutTraceSYS("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

static void deviceEraseW(WCHAR *p)
{
	WCHAR *pCurr = p;
	WCHAR *pNext = p + wcslen(p) + 1;
	while(*pNext) {
		wcscpy(pCurr, pNext);
		pNext += wcslen(pNext) + 1;
		pCurr += wcslen(pCurr) + 1;
	}
	*pCurr++ = 0;
	*pCurr = 0;
}

DWORD WINAPI extGetLogicalDriveStringsW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	// v2.06.08: fix - corrected return value
	DWORD ret;
	WCHAR *p;
	ApiName("GetLogicalDriveStringsW");

	OutTraceSYS("%s: buflen=%d\n", ApiRef, nBufferLength);

	WCHAR *sTmpBuffer = (WCHAR *)malloc(MAXDRIVESPACE * sizeof(WCHAR));
	ret=(*pGetLogicalDriveStringsW)(MAXDRIVESPACE, sTmpBuffer);
	if(!ret){
		OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
		free(sTmpBuffer);
		return ret;
	}
	else {
		OutDebugSYS("%s: actual ret=%d\n", ApiRef, ret);
	}

	if(dxw.dwFlags17 & HIDECDROMREAL){
		UINT driveType;
		p = (WCHAR *)sTmpBuffer;
		while(*p){
			driveType = (*pGetDriveTypeW)(p);
			//OutTrace("%s: drive %ls type=%d\n", ApiRef, p, driveType);
			if(driveType == DRIVE_CDROM){
				OutTraceDW("%s: HIDECDROMREAL - hide real CD drive %Ls\n", ApiRef, p);
				// delete the drive from the list
				deviceEraseW(p);			}
			else {
				p += wcslen(p) + 1;
			}
		}
	}

	if(dxw.dwFlags4 & HIDECDROMEMPTY){
		p = (WCHAR *)sTmpBuffer;
		while(*p){
			BOOL Vol;
			Vol = (*pGetVolumeInformationW)(p, NULL, NULL, NULL, 0, 0, 0, 0);
			if(!Vol) {
				OutTraceSYS("%s: HIDECDROMEMPTY - hide empty CDROM drive %Ls\n", ApiRef, p); 
				deviceEraseW(p);			}
			else {
				p += wcslen(p) + 1;
			}
		}
	}

	// should insert virtual CD/HD drives here ...
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		BOOL bMustAddHD = (dxw.dwFlags10 & FAKEHDDRIVE);
		BOOL bMustAddCD = (dxw.dwFlags10 & FAKECDDRIVE);
		ret = 1 * sizeof(WCHAR); // to count for the final terminator
		// reach the end of the list and verify the need to add drives
		p = (WCHAR *)sTmpBuffer;
		while(*p){
			if(bMustAddHD && (toupper(*p) == dxw.FakeHDDrive)) bMustAddHD = FALSE;
			if(bMustAddCD && (toupper(*p) == dxw.FakeCDDrive)) bMustAddCD = FALSE;
			ret += (wcslen(p) + 1) * sizeof(WCHAR);
			p += wcslen(p) + 1;
		}
		if(bMustAddHD){
			swprintf(p, MAXDRIVESPACE * sizeof(WCHAR), L"%c:\\", dxw.FakeHDDrive);
			ret += 4 * sizeof(WCHAR);
			p += 4;
		}
		if(bMustAddCD){
			swprintf(p, MAXDRIVESPACE * sizeof(WCHAR), L"%c:\\", dxw.FakeCDDrive);
			ret += 4 * sizeof(WCHAR);
			p += 4;
		}
		*p = 0;
	}

	p = sTmpBuffer;
	while(*p){
		OutTraceSYS("> %ls\n", p);
		p += wcslen(p) + 1;
	}

	// copy to destination buffer for available space only
	// v2.05.32: trim if user provided too high value! Fixes "Nox" crash.
	if(lpBuffer) wcsncpy(lpBuffer, sTmpBuffer, (nBufferLength > MAXDRIVESPACE) ? MAXDRIVESPACE : nBufferLength);
	free(sTmpBuffer);
	OutTraceSYS("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

HFILE WINAPI ext_lopen(LPCSTR lpPathName, int iReadWrite)
{
	HFILE ret;
	ApiName("_lopen");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: lpPathName=\"%s\" rw=%#x\n", ApiRef, lpPathName, iReadWrite);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) lpPathName = dxwTranslatePathA(lpPathName, &mapping);

	if(mapping == DXW_FAKE_CD) {
		// tbd: fake error codes for write access on fake CD
		if(iReadWrite & (OF_READWRITE|OF_WRITE)) {
			OutTraceDW("%s: simulate HFILE_ERROR on CD\n", ApiRef);
			SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
			return HFILE_ERROR;
		}
		// necessary or a Wine bug???
		if(iReadWrite == 0){
			FILE *f = fopen(lpPathName, "r");
			if(f == NULL) {
				OutTraceDW("%s: simulate HFILE_ERROR on CD\n", ApiRef);
				SetLastError(ERROR_FILE_NOT_FOUND);
				return HFILE_ERROR;
			}
			fclose(f);
		}
	}

	// v2.06.02 fix: added path translation
	// @#@ "Mulk & Swank Great Adventure in Magic World"
	if((dxw.dwFlags12 & PATHLOCALE) && lpPathName){
		DWORD dwDesiredAccess;
		DWORD dwShareMode;
		DWORD dwCreationDisposition;
		DWORD dwFlagsAndAttributes;
		switch(iReadWrite & 0x3){ // t.b.d. all the remaining mask bits
			case OF_READ:
			default:
				dwDesiredAccess = GENERIC_READ;
				dwShareMode = FILE_SHARE_READ;
				dwCreationDisposition = OPEN_EXISTING;
				dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
				break;
			case OF_WRITE:
				dwDesiredAccess = GENERIC_WRITE;
				dwShareMode = FILE_SHARE_WRITE;
				dwCreationDisposition = CREATE_NEW; // the less dangerous ...
				dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
				break;
			case OF_READWRITE:
				dwDesiredAccess = GENERIC_ALL;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition = CREATE_NEW; // the less dangerous ...
				dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
				break;
		}
		int size = strlen(lpPathName); // ghogho
		OutTraceLOC("%s: ASCII path=%s len0%d\n", ApiRef, lpPathName, size);
		WCHAR *lpPathNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpPathName, size, lpPathNameW, size);
		lpPathNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=%ls\n", ApiRef, lpPathNameW);
		ret = (HFILE)CreateFileW(lpPathNameW, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
		if(ret == HFILE_ERROR) {
			OutTraceE("%s: CreateFileW ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
			ret=(*p_lopen)(lpPathName, iReadWrite);
		}
		free(lpPathNameW);
	}
	else {
		ret=(*p_lopen)(lpPathName, iReadWrite);
	}

	if(ret == HFILE_ERROR){
		OutErrorSYS("%s: ERROR path=\"%s\" rw=%#x err=%d\n", ApiRef, lpPathName, iReadWrite, GetLastError());
	} else {
		OutTraceSYS("%s: res=%#x\n", ApiRef, ret);
	}

	return ret;
}

HFILE WINAPI ext_lcreat(LPCSTR lpPathName, int iAttribute)
{
	HFILE ret;
	ApiName("_lcreat");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: lpPathName=\"%s\" attr=%#x\n", ApiRef, lpPathName, iAttribute);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) lpPathName = dxwTranslatePathA(lpPathName, &mapping);

	if(mapping == DXW_FAKE_CD) {
		OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
		SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	if((dxw.dwFlags12 & PATHLOCALE) && lpPathName){
		// If the file does not exist, _lcreat creates and opens a new file for writing. 
		// If the file does exist, _lcreat truncates the file size to zero and opens it for reading and writing.
		DWORD dwDesiredAccess;
		DWORD dwShareMode;
		DWORD dwCreationDisposition;
		DWORD dwFlagsAndAttributes;
		switch(iAttribute){ 
			case 0: // Normal. Can be read from or written to without restriction. 
				dwDesiredAccess = GENERIC_ALL;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition = OPEN_ALWAYS;
				dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
				break;
			case 1: // Read-only. Cannot be opened for write. 
				dwDesiredAccess = GENERIC_READ;
				dwShareMode = FILE_SHARE_READ;
				dwCreationDisposition = OPEN_ALWAYS; // the less dangerous ... ???
				dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
				break;
			case 2: // Hidden. Not found by directory search. 
			case 4: // System. Not found by directory search. 
			default:
				MessageBox(0, "_lcreat trap", "DxWnd", 0);
				OutTrace("%s: _lcreat TRAP: attribute=%d path=\"%s\"\n", ApiRef, iAttribute, lpPathName);
				break;
		}
		int size = strlen(lpPathName); // ghogho
		OutTraceLOC("%s: ASCII path=%s len0%d\n", ApiRef, lpPathName, size);
		WCHAR *lpPathNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpPathName, size, lpPathNameW, size);
		lpPathNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=%ls\n", ApiRef, lpPathNameW);
		ret = (HFILE)CreateFileW(lpPathNameW, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
		if(ret == (HFILE)INVALID_HANDLE_VALUE) {
			OutTraceE("%s: CreateFileW ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
			ret=(*p_lcreat)(lpPathName, iAttribute);
		}
		free(lpPathNameW);
	}
	else {
		ret=(*p_lcreat)(lpPathName, iAttribute);
	}

	if(ret == HFILE_ERROR){
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	} else {
		OutDebugSYS("%s: res=%#x\n", ApiRef, ret);
	}

	return ret;
}

HFILE WINAPI extOpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle)
{
	HFILE ret;
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("OpenFile: lpPathName=\"%s\" style=%#x\n", lpFileName, uStyle);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) lpFileName = dxwTranslatePathA(lpFileName, &mapping);

	if(mapping == DXW_FAKE_CD) {
		if(uStyle & (OF_CREATE|OF_DELETE|OF_READWRITE|OF_WRITE)) {
			OutTraceDW("OpenFile: simulate write access error on CD\n");
			SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
			return (HFILE)INVALID_HANDLE_VALUE;
		}
	}

	ret=(*pOpenFile)(lpFileName, lpReOpenBuff, uStyle);

	if(ret == HFILE_ERROR){
		OutErrorSYS("OpenFile ERROR: err=%d\n", GetLastError());
	} else {
		OutDebugSYS("OpenFile: res=%#x\n", ret);
	}

	return ret;
}

HANDLE WINAPI extCreateFileMappingA(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName)
{
	HANDLE ret;
	ApiName("CreateFileMappingA");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: hfile=%#x protect=%#x MaxHigh\\Low=%d\\%d lpName=\"%s\"\n", 
		ApiRef, hFile, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

	// v2.06.11: for some reason, pCreateFileMappingA could be NULL ... @#@ "Gangsters: Organized Crime"
	if(!pCreateFileMappingA) {
		OutTrace("%s ERROR: NULL hook, bypass returning INVALID_HANDLE_VALUE\n", ApiRef);
		return INVALID_HANDLE_VALUE;
	}

	// v2.05.19: lpName could be NULL, so bypass dxwTranslatePathA in this case
	if(lpName && (dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)))
		lpName = dxwTranslatePathA(lpName, &mapping);

	if(mapping == DXW_FAKE_CD) {
		// to do ....
		OutTrace("%s: TO DO @%d\n", ApiRef, __LINE__);
	}

	if((dxw.dwFlags11 & CUSTOMLOCALE) && (dxw.dwFlags12 & PATHLOCALE) && lpName){
		LPCWSTR lpNameW = MultiByteToWideCharInternal(lpName);
		if(!pCreateFileMappingW) pCreateFileMappingW = CreateFileMappingW;
		OutTraceSYS("%s: TRANSLATED lpName=\"%Ls\"\n", ApiRef, lpNameW);
		ret=(*pCreateFileMappingW)(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpNameW);
		FreeStringInternal((LPVOID)lpNameW);
		return ret;
	}

	ret=(*pCreateFileMappingA)(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

	// v2.06.07: @#@ "Demonworld"
	if(!ret && lpName){
		char fixedFileName[MAX_PATH+1];
		char *p;
		strncpy(fixedFileName, lpName, MAX_PATH);
		char *begin = fixedFileName;
		if(_strnicmp(fixedFileName, "Global\\", 7) == 0) begin +=7;
		if(_strnicmp(fixedFileName, "Local\\", 6) == 0) begin +=6;
		while (p=(char *)strchr(begin, '\\')) *p='_';
		OutTraceDW("%s: patched path=\"%s\"\n", ApiRef, fixedFileName);
		ret=(*pCreateFileMappingA)(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, fixedFileName);
	}

	if(!ret && (dxw.EmulateWin95 || (dxw.dwFlags16 & FIXCREATEFILEMAP))){
        DWORD flNewProtect = flProtect;
        if ((flProtect & SEC_NOCACHE) && 
            (!((flProtect & SEC_COMMIT) || (flProtect & SEC_RESERVE)))) {
            // Add the SEC_COMMIT flag
			OutTraceDW("%s: adding SEC_COMMIT flag\n", ApiRef);
            flNewProtect |= SEC_COMMIT;
        }
		
		char fixedFileName[MAX_PATH+1];
		char *p;
		strncpy(fixedFileName, lpName, MAX_PATH);
		char *begin = fixedFileName;
		if(_strnicmp(fixedFileName, "Global\\", 7) == 0) begin +=7;
		if(_strnicmp(fixedFileName, "Local\\", 6) == 0) begin +=6;
		while (p=(char *)strchr(begin, '\\')) *p='_';
		OutTraceDW("%s: patched path=\"%s\"\n", ApiRef, fixedFileName);
		ret=(*pCreateFileMappingA)(hFile, lpFileMappingAttributes, flNewProtect, dwMaximumSizeHigh, dwMaximumSizeLow, fixedFileName);
	}

	if(!ret){
		OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
	} else {
		OutDebugSYS("%s: res=%#x\n", ApiRef, ret);
	}

	return ret;
}

HANDLE WINAPI extCreateFileMappingW(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName)
{
	HANDLE ret;
	ApiName("CreateFileMappingW");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: hfile=%#x protect=%#x lpName=\"%ls\"\n", ApiRef, hFile, flProtect, lpName);

	// v2.05.19: lpName could be NULL, so bypass dxwTranslatePathA in this case
	if(lpName && (dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)))
		lpName = dxwTranslatePathW(lpName, &mapping);

	if(mapping == DXW_FAKE_CD) {
		// to do ....
	}

	ret=(*pCreateFileMappingW)(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

	if(!ret && (dxw.EmulateWin95 || (dxw.dwFlags16 & FIXCREATEFILEMAP))){
        DWORD flNewProtect = flProtect;
        if ((flProtect & SEC_NOCACHE) && 
            (!((flProtect & SEC_COMMIT) || (flProtect & SEC_RESERVE)))) {
            // Add the SEC_COMMIT flag
			OutTraceDW("%s: adding SEC_COMMIT flag\n", ApiRef);
            flNewProtect |= SEC_COMMIT;
        }
		
		WCHAR fixedFileName[MAX_PATH+1];
		WCHAR *p;
		wcsncpy(fixedFileName, lpName, MAX_PATH);
		WCHAR *begin = fixedFileName;
		if(_wcsnicmp(fixedFileName, L"Global\\", 7) == 0) begin +=7;
		if(_wcsnicmp(fixedFileName, L"Local\\", 6) == 0) begin +=6;
		while (p=(WCHAR *)wcschr(begin, L'\\')) *p='_';
		OutTraceDW("%s: patched path=\"%ls\"\n", ApiRef, fixedFileName);
		ret=(*pCreateFileMappingW)(hFile, lpFileMappingAttributes, flNewProtect, dwMaximumSizeHigh, dwMaximumSizeLow, fixedFileName);
	}

	if(!ret){
		OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
	} else {
		OutDebugSYS("%s: res=%#x\n", ApiRef, ret);
	}

	return ret;
}

HANDLE WINAPI extOpenFileMappingA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
{
	HANDLE ret;
	ApiName("OpenFileMappingA");

#ifndef DXW_NOTRACES
	if(lpName)
		OutTraceSYS("%s: access=%#x inherit=%#x name=\"%s\"\n", ApiRef, dwDesiredAccess, bInheritHandle, lpName);
	else
		OutTraceSYS("%s: access=%#x inherit=%#x name=(NULL)\n", ApiRef, dwDesiredAccess, bInheritHandle);
#endif DXW_NOTRACES

	ret = (*pOpenFileMappingA)(dwDesiredAccess, bInheritHandle, lpName);

	if(!ret && lpName){
		char fixedFileName[MAX_PATH+1];
		char *p;
		strncpy(fixedFileName, lpName, MAX_PATH);
		char *begin = fixedFileName;
		if(_strnicmp(fixedFileName, "Global\\", 7) == 0) begin +=7;
		if(_strnicmp(fixedFileName, "Local\\", 6) == 0) begin +=6;
		while (p=(char *)strchr(begin, '\\')) *p='_';
		OutTraceDW("%s: patched path=\"%s\"\n", ApiRef, fixedFileName);
		ret = (*pOpenFileMappingA)(dwDesiredAccess, bInheritHandle, fixedFileName);
	}
		
	if(!ret){
		OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
	} else {
		OutDebugSYS("%s: res=%#x\n", ApiRef, ret);
	}

	return ret;
}

HANDLE WINAPI extOpenFileMappingW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
{
	HANDLE ret;
	ApiName("OpenFileMappingW");

#ifndef DXW_NOTRACES
	if(lpName)
		OutTraceSYS("%s: access=%#x inherit=%#x name=\"%ls\"\n", ApiRef, dwDesiredAccess, bInheritHandle, lpName);
	else
		OutTraceSYS("%s: access=%#x inherit=%#x name=(NULL)\n", ApiRef, dwDesiredAccess, bInheritHandle);
#endif DXW_NOTRACES

	ret = (*pOpenFileMappingW)(dwDesiredAccess, bInheritHandle, lpName);

	if(!ret && lpName){
		WCHAR fixedFileName[MAX_PATH+1];
		WCHAR *p;
		wcsncpy(fixedFileName, lpName, MAX_PATH);
		WCHAR *begin = fixedFileName;
		if(_wcsnicmp(fixedFileName, L"Global\\", 7) == 0) begin +=7;
		if(_wcsnicmp(fixedFileName, L"Local\\", 6) == 0) begin +=6;
		while (p=(WCHAR *)wcschr(begin, L'\\')) *p=L'_';
		OutTraceDW("%s: patched path=\"%ls\"\n", ApiRef, fixedFileName);
		ret = (*pOpenFileMappingW)(dwDesiredAccess, bInheritHandle, fixedFileName);
	}
		
	if(!ret){
		OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
	} else {
		OutDebugSYS("%s: res=%#x\n", ApiRef, ret);
	}

	return ret;
}

BOOL WINAPI extCloseHandle(HANDLE hObject)
{
	BOOL ret;
	ApiName("CloseHandle");
	DWORD dwInfo;
	OutDebugDW("%s: hObject=%#x\n", ApiRef, hObject);

	if ((hObject == 0) || (hObject == (HANDLE)-1)){
		OutTraceDW("%s: BYPASS hObject=%#x\n", ApiRef, hObject);
		return TRUE;
	}

	if(dxw.dwFlags18 & EMUDDSYNCSHIM){
		if (g_hThread && (hObject == g_hDDMutex))
		{
			OutTraceDW("%s: EMUDDSYNCSHIM DDraw exclusive mode mutex closed\n", ApiRef);
			g_hDDMutex = 0;
		}
	}

	if(dxw.dwFlags20 & HANDLECDLOCK){
		if(hObject == CDLockFile){
			OutTraceDW("%s: clear CDLockFile hObject=%#x\n", ApiRef, hObject);
			CDLockFile = 0;
		}
	}

	if(!GetHandleInformation(hObject, &dwInfo)){
		OutTraceDW("%s: GetHandleInformation BYPASS hObject=%#x err=%d\n", ApiRef, hObject, GetLastError());
		return FALSE;
	}
	__try {
		ret=(*pCloseHandle)(hObject);
	} 
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTraceDW("%s: BYPASS EXCEPTION hObject=%#x\n", ApiRef, hObject);
		ret = TRUE;
	}
	return ret;
}

BOOL WINAPI extGetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode)
{
	BOOL res;
	ApiName("GetExitCodeProcess");

	OutTraceDW("%s: hProcess=%#x\n", ApiRef, hProcess);

	if(dxw.dwFlags4 & SUPPRESSCHILD) {
		OutTraceDW("%s: FAKE exit code=0\n", ApiRef);
		lpExitCode = 0;
		return TRUE;
	}

	res=(*pGetExitCodeProcess)(hProcess, lpExitCode);
	// v2.05.66 fix: no longer necessary
	//if(dxw.dwFlags5 & (INJECTSON|DEBUGSON)) {
	//	if(*lpExitCode != STILL_ACTIVE){
	//		OutTraceDW("%s: locking mutex\n", ApiRef);
	//		extern HANDLE hLockMutex;
	//		WaitForSingleObject(hLockMutex, 0);
	//	}
	//}
	// to do: should refresh some info into status descriptor!
	OutTraceDW("%s: hProcess=%#x ExitCode=%#x res=%#x\n", ApiRef, hProcess, *lpExitCode, res);
	return res;
}

BOOL WINAPI extCheckRemoteDebuggerPresent(HANDLE hProcess, PBOOL pbDebuggerPresent)
{
	BOOL ret;
	ApiName("CheckRemoteDebuggerPresent");
	if(pbDebuggerPresent) *pbDebuggerPresent = FALSE;
	ret= (hProcess==(HANDLE)0xFFFFFFFF) ? FALSE : TRUE;
	OutTraceDW("%s: hProcess=%#x ret=%#x\n", ApiRef, hProcess, ret);
	return ret;
}

UINT WINAPI extGetTempFileNameA(LPCSTR lpPathName, LPCSTR lpPrefixString, UINT uUnique, LPSTR lpTempFileName)
{
	UINT ret;
	ApiName("GetTempFileNameA");
	OutTraceDW("%s: PathName=\"%s\" PrefixString=%s Unique=%d\n", ApiRef, lpPathName, lpPrefixString, uUnique);

	//if(1) {
	//	lpTempFileName = ".\\Temp1";
	//	ret = 1;
	//	OutTraceDW("%s: TempFileName=\"%s\" ret=%d\n", ApiRef, lpTempFileName, ret);
	//	return ret;
	//}

	ret = (*pGetTempFileNameA)(lpPathName, lpPrefixString, uUnique, lpTempFileName);
	if(ret == 0){
		// GetTempFileName patch to make "Powerslide" working
		OutTraceDW("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		char sTmpDir[MAX_PATH+1];
		GetTempPathA(sizeof(sTmpDir), sTmpDir);
		ret = (*pGetTempFileNameA)(sTmpDir, lpPrefixString, uUnique, lpTempFileName);
		_if(ret == 0) OutTraceDW("%s: ERROR err=%d PathName=\"%s\" @%d\n", 
			ApiRef, GetLastError(), sTmpDir, __LINE__);
	}
	if(ret){
		OutTraceDW("%s: TempFileName=\"%s\" ret=%d\n", ApiRef, lpTempFileName, ret);
	}
	return ret;
}

UINT WINAPI extGetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName)
{
	UINT ret;
	ApiName("GetTempFileNameW");
	OutTraceDW("%s: PathName=\"%ls\" PrefixString=%ls Unique=%d\n", ApiRef, lpPathName, lpPrefixString, uUnique);

	//if(1) {
	//	lpTempFileName = L".\\Temp1";
	//	ret = 1;
	//	OutTraceDW("%s: TempFileName=\"%ls\" ret=%d\n", ApiRef, lpTempFileName, ret);
	//	return ret;
	//}

	ret = (*pGetTempFileNameW)(lpPathName, lpPrefixString, uUnique, lpTempFileName);
	if(ret == 0){
		// GetTempFileName patch, useless so far ...
		OutTraceDW("%s: ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		WCHAR sTmpDir[MAX_PATH+1];
		GetTempPathW(sizeof(sTmpDir), sTmpDir);
		ret = (*pGetTempFileNameW)(sTmpDir, lpPrefixString, uUnique, lpTempFileName);
		_if(ret == 0) OutTraceDW("%s: ERROR err=%d PathName=\"%ls\" @%d\n", 
			ApiRef, GetLastError(), sTmpDir, __LINE__);
	}
	if(ret){
		OutTraceDW("%s: TempFileName=\"%ls\" ret=%d\n", ApiRef, lpTempFileName, ret);
	}
	return ret;
}

LPVOID WINAPI extVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	// v2.03.20: handling of legacy memory segments.
	// Some games (Crusaders of Might and Magic, the demo and the GOG release) rely on the fact that the
	// program can VirtualAlloc-ate memory on certain tipically free segments (0x4000000, 0x5000000, 
	// 0x6000000, 0x7000000 and 0x8000000) but when the program is hooked by DxWnd these segments could
	// be allocated to extra dlls or allocated memory. 
	// The trick is ti pre-allocate this memory and free it upon error to make it certainly available to
	// the calling program.

	LPVOID ret;
	ApiName("VirtualAlloc");

	OutDebugSYS("%s: lpAddress=%#x size=%#x flag=%#x protect=%#x\n", ApiRef, lpAddress, dwSize, flAllocationType, flProtect);
	ret = (*pVirtualAlloc)(lpAddress, dwSize, flAllocationType, flProtect);
	if((ret == NULL) && lpAddress && (dxw.dwFlags6 & LEGACYALLOC)){
		OutErrorSYS("%s: RECOVERY lpAddress=%#x size=%#x flag=%#x protect=%#x\n", 
			ApiRef, lpAddress, dwSize, flAllocationType, flProtect);
		if (((DWORD)lpAddress & 0xF0FFFFFF) == 0){
			BOOL bret;
			bret = VirtualFree(lpAddress, 0x00000000, MEM_RELEASE);
			_if(!bret) OutErrorSYS("%s: VirtualFree MEM_RELEASE err=%d\n", ApiRef, GetLastError());
			ret = (*pVirtualAlloc)(lpAddress, dwSize, flAllocationType, flProtect);
			_if (ret == NULL) OutErrorSYS("%s: addr=%#x err=%d\n", ApiRef, lpAddress, GetLastError());
		}
		if (!ret) ret = (*pVirtualAlloc)((LPVOID)0x00000000, dwSize, flAllocationType, flProtect);
		_if(ret == NULL) OutErrorSYS("%s: addr=NULL err=%d\n", ApiRef, GetLastError());
	}
	OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extVirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
	BOOL ret;
	ApiName("VirtualFree");

	OutDebugSYS("%s: lpAddress=%#x size=%#x FreeType=%#x\n", ApiRef, lpAddress, dwSize, dwFreeType);
	ret = (*pVirtualFree)(lpAddress, dwSize, dwFreeType);
	if(!ret) {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

// WinExec: used by "Star Wars X-Wings Alliance" frontend, but fortunately it's not essential to hook it....
// used by "Daemonica", and in this case it WOULD be essential ...
UINT WINAPI extWinExec(LPCSTR lpCmdLine, UINT uCmdShow)
{
	UINT ret;
	ApiName("WinExec");

	OutTraceDW("%s: lpCmdLine=\"%s\" CmdShow=%#x\n", ApiRef, lpCmdLine, uCmdShow);

	if(dxw.bHintActive) ShowHint(HINT_PROCESS);

	if(dxw.dwFlags4 & SUPPRESSCHILD) {
		OutTraceDW("%s: SUPPRESS\n", ApiRef);
		// from MSDN: If the function succeeds, the return value is greater than 31.
		return 32;
	}

	// v2.06.06: added fake-HD/CD handling. Needed for @#@ "Mr. Moo"
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpCmdLine) {
		lpCmdLine = dxwTranslatePathA(lpCmdLine, NULL);
	}

	// v2.05.75 fix: use dxwWinExec also in case of INJECTSON. Needed for "Cabela's 4x4 Off-Road Adventure 2" launcher
	if((dxw.dwFlags13 & SHAREDHOOK)||(dxw.dwFlags5 & INJECTSON)){
		extern UINT dxwWinExec(LPCSTR, UINT);
		return dxwWinExec(lpCmdLine, uCmdShow);
	}

	ret=(*pWinExec)(lpCmdLine, uCmdShow);
	if(ret<31){
		OutErrorSYS("%s: ERROR err=%d ret=%#x\n", ApiRef, GetLastError(), ret);
		}
	else{
		OutErrorSYS("%s: OK ret=%#x\n", ApiRef, ret);
	}
	return ret;
}

BOOL WINAPI extSetThreadPriority(HANDLE hThread, int nPriority)  
{
	BOOL ret;
	static BOOL uninitialized = TRUE;
	ApiName("SetThreadPriority");

	if(dxw.dwFlags15 & IGNORESCHEDULER) {
		if(uninitialized){
			OutTraceDW("%s: IGNORESCHEDULER\n", ApiRef);
			nPriority = THREAD_PRIORITY_NORMAL;
			ret = (*pSetThreadPriority)(hThread, nPriority);
			uninitialized = FALSE;
		}
		return TRUE;
	}

	OutTraceDW("%s: hThread=%#x priority=%d\n", ApiRef, hThread, nPriority);
	ret = (*pSetThreadPriority)(hThread, nPriority);
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extSetPriorityClass(HANDLE hProcess, DWORD dwPriorityClass)
{
	BOOL ret;
	static BOOL uninitialized = TRUE;
	ApiName("SetPriorityClass");

	if(dxw.dwFlags15 & IGNORESCHEDULER) {
		if(uninitialized){
			OutTraceDW("%s: IGNORESCHEDULER\n", ApiRef);
			dwPriorityClass = THREAD_PRIORITY_NORMAL;
			ret = (*pSetPriorityClass)(hProcess, dwPriorityClass);
			uninitialized = FALSE;
		}
		return TRUE;
	}

	OutTraceDW("%s: hProcess=%#x class=%#x\n", ApiRef, hProcess, dwPriorityClass);
	ret = (*pSetPriorityClass)(hProcess, dwPriorityClass);
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

void WINAPI extOutputDebugStringA(LPCTSTR str)
{
	CHAR c = 0;
	if(str[strlen(str)-1] != '\n') c='\n';
	OutTraceSYS("OutputDebugStringA: %s%c", str, c);
}

void WINAPI extOutputDebugStringW(LPCWSTR str)
{
	CHAR c = 0;
	if(str[wcslen(str)-1] != L'\n') c='\n';
	OutTraceSYS("OutputDebugStringW: %Ls%c", str, c);
}

// 0: PF_FLOATING_POINT_PRECISION_ERRATA
// On a Pentium, a floating-point precision error can occur in rare circumstances.
//
// 1: PF_FLOATING_POINT_EMULATED
// Floating-point operations are emulated using a software emulator.
//
// 2: PF_COMPARE_EXCHANGE_DOUBLE
// The atomic compare and exchange operation (cmpxchg) is available.
//
// 3: PF_MMX_INSTRUCTIONS_AVAILABLE
// The MMX instruction set is available.
//
// 6: PF_XMMI_INSTRUCTIONS_AVAILABLE
// The SSE instruction set is available.
//
// 7: PF_3DNOW_INSTRUCTIONS_AVAILABLE
// The 3D-Now instruction set is available.
//
// 8: PF_RDTSC_INSTRUCTION_AVAILABLE
// The RDTSC instruction is available.
//
// 9: PF_PAE_ENABLED
// The processor is PAE-enabled. For more information, see Physical Address Extension.
// All x64 processors always return a nonzero value for this feature.
//
// 10: PF_XMMI64_INSTRUCTIONS_AVAILABLE
// The SSE2 instruction set is available.
// Windows 2000:  This feature is not supported.
//
// 12: PF_NX_ENABLED
// Data execution prevention is enabled.
// Windows XP/2000:  This feature is not supported until Windows XP with SP2 and Windows Server 2003 with SP1.
//
// 13: PF_SSE3_INSTRUCTIONS_AVAILABLE
// The SSE3 instruction set is available.
// Windows Server 2003 and Windows XP/2000:  This feature is not supported.
//
// 14: PF_COMPARE_EXCHANGE128
// The atomic compare and exchange 128-bit operation (cmpxchg16b) is available.
// Windows Server 2003 and Windows XP/2000:  This feature is not supported.
//
// 15: PF_COMPARE64_EXCHANGE128
// The atomic compare 64 and exchange 128-bit operation (cmp8xchg16) is available.
// Windows Server 2003 and Windows XP/2000:  This feature is not supported.
//
// 16: PF_CHANNELS_ENABLED
// The processor channels are enabled.
//
// 17: PF_XSAVE_ENABLED
// The processor implements the XSAVE and XRSTOR instructions.
//
// 18: PF_ARM_VFP_32_REGISTERS_AVAILABLE
// The VFP/Neon: 32 x 64bit register bank is present. This flag has the same meaning as PF_ARM_VFP_EXTENDED_REGISTERS.
//
// 20: PF_SECOND_LEVEL_ADDRESS_TRANSLATION
// Second Level Address Translation is supported by the hardware.
//
// 21: PF_VIRT_FIRMWARE_ENABLED
// Virtualization is enabled in the firmware.
//
// 22: PF_RDWRFSGSBASE_AVAILABLE
// RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE instructions are available.
//
// 23: PF_FASTFAIL_AVAILABLE
// _fastfail() is available.
// This function returns a nonzero value if floating-point operations are emulated; otherwise, it returns zero.
//
// 24: PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE
// The divide instructions are available.
//
// 25: PF_ARM_64BIT_LOADSTORE_ATOMIC
// The 64-bit load/store atomic instructions are available.
//
// 26: PF_ARM_EXTERNAL_CACHE_AVAILABLE
// The external cache is available.
//
// 27: PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE
// The floating-point multiply-accumulate instruction is available.

BOOL WINAPI extIsProcessorFeaturePresent(DWORD ProcessorFeature)
{
	BOOL res;
	ApiName("IsProcessorFeaturePresent");
	res = (*pIsProcessorFeaturePresent)(ProcessorFeature);
	OutTraceSYS("%s: feature=%#x ret=%#x\n", ApiRef, ProcessorFeature, res);
	return res;
}

DWORD WINAPI extGetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer)
{
	DWORD res;
	ApiName("GetCurrentDirectoryA");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: buflen=%d buf=%#x\n", ApiRef, nBufferLength, lpBuffer);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		res = (*pGetCurrentDirectoryA)(0, NULL);
		if(!res) {
			OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
			return 0;
		}
		char *fullBuffer = (char *)malloc(res + 1); // len + terminator
		char *lpPathName;
		res = (*pGetCurrentDirectoryA)(res, fullBuffer);
		OutTraceSYS("%s: actual ret=%d buf=\"%s\"\n", ApiRef, res, fullBuffer);
		lpPathName = (char *)dxwUntranslatePathA((LPCSTR)fullBuffer, &mapping);
		strncpy(lpBuffer, lpPathName, nBufferLength);
		res = strlen(lpBuffer);
		free(fullBuffer);
	}
	else {
		res = (*pGetCurrentDirectoryA)(nBufferLength, lpBuffer);
	}

	OutTraceSYS("%s: ret=%d buf=\"%s\"\n", ApiRef, res, lpBuffer);
	return res;
}

DWORD WINAPI extGetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	DWORD res;
	ApiName("GetCurrentDirectoryW");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: buflen=%d buf=%#x\n", ApiRef, nBufferLength, lpBuffer);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		res = (*pGetCurrentDirectoryW)(0, NULL);
		if(!res) {
			OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
			return 0;
		}
		LPWSTR fullBuffer = (LPWSTR)malloc(sizeof(WCHAR) * (res + 1)); // len + terminator, in WCHARs (2 bytes), fixed v2.05.58
		LPWSTR lpPathName;
		res = (*pGetCurrentDirectoryW)(res, fullBuffer);
		OutTraceSYS("%s: actual ret=%d buf=\"%ls\"\n", ApiRef, res, fullBuffer);
		lpPathName = (LPWSTR)dxwUntranslatePathW((LPWSTR)fullBuffer, &mapping);
		wcsncpy(lpBuffer, lpPathName, nBufferLength);
		res = wcslen(lpBuffer);
		free(fullBuffer);
	}
	else {
		res = (*pGetCurrentDirectoryW)(nBufferLength, lpBuffer);
	}

	OutTraceSYS("%s: ret=%d buf=\"%ls\"\n", ApiRef, res, lpBuffer);
	return res;
}

static int dirExistsA(const char *Path)
{
	ApiName("dirExists");
    struct stat info;
	int len = strlen(Path);
	int ret;
	char *path = (char *)malloc(len+2);
	strcpy(path, Path);
	// v2.06.04: delete multiple ending slashes. @#@ "Zaphie" (Ko 1998)
	//if((path[len-1]=='\\') || (path[len-1]=='/')) path[len-1] = 0;
	while(len && ((path[len-1]=='\\') || (path[len-1]=='/'))) {
		path[len-1] = 0;
		len--;
	}
	ret = stat( path, &info );
	free(path);
	if(ret != 0){
		OutTrace("%s: path=%s: err=%d\n", ApiRef, Path, GetLastError());
        return 0;
	}
	else if(info.st_mode & S_IFDIR){
 		OutTrace("%s: path=%s: ok\n", ApiRef, Path);
		return 1;
	}
	else{
		OutTrace("%s: path=%s: st_mode=%#x\n", ApiRef, Path, info.st_mode);
        return 0;
	}
}

static int dirExistsW(const WCHAR *Path)
{
	char path[MAX_PATH];
	sprintf(path, "%ls", Path);
	return dirExistsA(path);
}

BOOL WINAPI extSetCurrentDirectoryA(LPCSTR lpPathName)
{
	BOOL res;
	ApiName("SetCurrentDirectoryA");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpPathName);

	// v2.05.66: strip leading spaces, found in "Rayman Eveil"
	while(*lpPathName == ' ') lpPathName++;

	// fake drive handling
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		// save the current directory per each visited logical drive
		dxw.SaveCurrentDirectory();

		// try to understand if moving to a root folder BEFORE making the fake device mapping
		dxw.bIsRootFolder = FALSE;
		if(lpPathName){
			// case of "\" root folder
			if(!strcmp(lpPathName, "\\")) dxw.bIsRootFolder = TRUE;
			// case of "X:\" root folder
			if((strlen(lpPathName) == 3) && !strcmp(lpPathName+1, ":\\")) dxw.bIsRootFolder = TRUE;
		}
		lpPathName = dxwTranslatePathA(lpPathName, &mapping);
		if(mapping != DXW_NO_FAKE) OutTraceDW("%s: MAP %s\n", ApiRef, lpPathName);
	}

	if(dxw.dwFlags10 & FIXFOLDERPATHS){
		if(lpPathName[strlen(lpPathName)-1]=='.') {
			((UCHAR *)lpPathName)[strlen(lpPathName)-1]=0;
			OutTraceSYS("%s: fixed path=\"%s\"\n", ApiRef, lpPathName);
		}
		// v2.05.12: SetCurrentDirectory("") returns error 123 ERROR_INVALID_NAME - seen in "SpyCraft".
		if(strlen(lpPathName) == 0) {
			lpPathName=".";
			OutTraceSYS("%s: fixed path=\"%s\"\n", ApiRef, lpPathName);
		}
	}

	char sCurrentPath[MAX_PATH];
	(*pGetCurrentDirectoryA)(MAX_PATH, sCurrentPath);
	OutTraceSYS("%s: current=\"%s\" new=\"%s\"\n", ApiRef, sCurrentPath, lpPathName);
	if(mapping != DXW_NO_FAKE){
		// v2.05.79: check for folder existence and simulate err ERROR_FILE_NOT_FOUND if not found
		if(!dirExistsA(lpPathName)){
			OutTraceDW("%s: NO FOLDER path=\"%s\" ret=FALSE err=ERROR_FILE_NOT_FOUND\n", ApiRef, lpPathName);
			SetLastError(ERROR_FILE_NOT_FOUND);
			return FALSE;
		}
	}

	res = (*pSetCurrentDirectoryA)(lpPathName); 

	if(res && 
		((dxw.dwFlags5 & INJECTSON) || 
		(dxw.dwFlags7 & INJECTSUSPENDED)))
	{
		#define MAX_ENVPATH 4000
		char sPath[MAX_ENVPATH+1];
		GetEnvironmentVariableA("Path", sPath, MAX_ENVPATH);
		OutDebugDW("%s: ENV Path=%s\n", ApiRef, sPath);
		if(!strstr(sPath, ";.;")) {
			if(sPath[strlen(sPath)-1]==';')
				strncat(sPath, ".;", MAX_ENVPATH);
			else 
				strncat(sPath, ";.;", MAX_ENVPATH);
			SetEnvironmentVariableA("Path", sPath);
		}
	}

	if(mapping != DXW_NO_FAKE){
		OutTraceDW("%s: moved to virtual path=\"%s\"\n", ApiRef, lpPathName);
	}

	if(res) {
		dxw.dwCurrentFolderType = mapping;
		OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	}
	else {
		OutErrorSYS("%s: ERROR ret=0 path=\"%s\" err=%d\n", ApiRef, lpPathName, GetLastError());
	}
	return res;
}

BOOL WINAPI extSetCurrentDirectoryW(LPWSTR lpPathName)
{
	BOOL res;
	ApiName("SetCurrentDirectoryW");
	DWORD mapping = DXW_NO_FAKE;

	OutTraceSYS("%s: path=\"%ls\"\n", ApiRef, lpPathName);

	// strip leading spaces
	while(*lpPathName == L' ') lpPathName++;

	// fake drive handling
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		// save the current directory per each visited logical drive
		dxw.SaveCurrentDirectory();

		// try to understand if moving to a root folder BEFORE making the fake device mapping
		dxw.bIsRootFolder = FALSE;
		if(lpPathName){
			// case of "\" root folder
			if(!wcscmp(lpPathName, L"\\")) dxw.bIsRootFolder = TRUE;
			// case of "X:\" root folder
			if((wcslen(lpPathName) == 3) && !wcscmp(lpPathName+1, L":\\")) dxw.bIsRootFolder = TRUE;
		}
		lpPathName = (LPWSTR)dxwTranslatePathW(lpPathName, &mapping);
		if(mapping != DXW_NO_FAKE) OutTraceDW("%s: MAP %ls\n", ApiRef, lpPathName);
	}

	if(dxw.dwFlags10 & FIXFOLDERPATHS){
		if(lpPathName[wcslen(lpPathName)-1]==(WCHAR)'.') { 
			((WCHAR *)lpPathName)[wcslen(lpPathName)-1]=(WCHAR)0;
			OutTraceSYS("%s: fixed path=\"%ls\"\n", ApiRef, lpPathName);
		}
		// v2.05.12: SetCurrentDirectory("") returns error 123 ERROR_INVALID_NAME.
		if(wcslen(lpPathName) == 0) {
			lpPathName=L".";
			OutTraceSYS("%s: fixed path=\"%ls\"\n", ApiRef, lpPathName);
		}
	}

	WCHAR sCurrentPath[MAX_PATH];
	(*pGetCurrentDirectoryW)(MAX_PATH, sCurrentPath);
	OutTraceSYS("%s: current=\"%ls\" new=\"%ls\"\n", ApiRef, sCurrentPath, lpPathName);
	if(mapping != DXW_NO_FAKE){
		// v2.05.79: check for folder existence and simulate err ERROR_FILE_NOT_FOUND if not found
		if(!dirExistsW(lpPathName)){
			OutTraceDW("%s: NO FOLDER path=\"%ls\" ret=FALSE err=ERROR_FILE_NOT_FOUND\n", ApiRef, lpPathName);
			SetLastError(ERROR_FILE_NOT_FOUND);
			return FALSE;
		}
	}

	// if moving to fake drive, keep track of the drive type for later use
	res = (*pSetCurrentDirectoryW)(lpPathName);

	if(res && 
		((dxw.dwFlags5 & INJECTSON) || 
		(dxw.dwFlags7 & INJECTSUSPENDED))){
		#define MAX_ENVPATH 4000
		WCHAR sPath[MAX_ENVPATH+1];
		GetEnvironmentVariableW(L"Path", sPath, MAX_ENVPATH);
		OutDebugDW("%s: ENV Path=%ls\n", ApiRef, sPath);
		if(!wcsstr(sPath, L";.;")) {
			if(sPath[wcslen(sPath)-1]==L';')
				wcsncat(sPath, L".;", MAX_ENVPATH);
			else 
				wcsncat(sPath, L";.;", MAX_ENVPATH);
			SetEnvironmentVariableW(L"Path", sPath);
		}
	}

	if(mapping != DXW_NO_FAKE){
		OutTraceDW("%s: moved to virtual path=\"%ls\"\n", ApiRef, lpPathName);
	}

	if(res) {
		dxw.dwCurrentFolderType = mapping;
		OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	}
	else {
		OutErrorSYS("%s: ERROR ret=0 path=\"%ls\" err=%d\n", ApiRef, lpPathName, GetLastError());
	}
	return res;
}

// Critical Section API 

#define DELAYCYCLES		0x4000
#define DELAYACTIVITY	for(int i=0; i<DELAYCYCLES; i++) {DWORD j=(*pGetTickCount)();}

void WINAPI extInitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	ApiName("InitializeCriticalSection");
	OutTraceSYS("%s: lpcs=%#x\n", ApiRef, lpCriticalSection);

	if(dxw.dwFlags11 & MUTEX4CRITSECTION){
		char MutexName[21];
		sprintf(MutexName, "Mx%08.8X", lpCriticalSection);
		lpCriticalSection->LockCount = (LONG)CreateMutexA(NULL, FALSE, MutexName);
		OutTraceSYS("%s: mutex name=%s handle=%#x\n", ApiRef, MutexName, lpCriticalSection->LockCount);
	}
	else {
		OutTraceSYS("%s: cs={lockcount=%d spincount=%d reccount=%d locksem=%#x ownthread=%#x}\n", ApiRef, 
			lpCriticalSection->LockCount,
			lpCriticalSection->SpinCount,
			lpCriticalSection->RecursionCount,
			lpCriticalSection->LockSemaphore,
			lpCriticalSection->OwningThread);
		(*pInitializeCriticalSection)(lpCriticalSection);
	}

	OutTraceSYS("%s: terminated\n", ApiRef);
}

void WINAPI extEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	ApiName("EnterCriticalSection");
	OutDebugSYS("%s: lpcs=%#x\n", ApiRef, lpCriticalSection);

	if(dxw.dwFlags11 & DELAYCRITSECTION) DELAYACTIVITY;

	if(dxw.dwFlags11 & MUTEX4CRITSECTION){
		OutTraceSYS("%s: mutex handle=%#x\n", ApiRef, lpCriticalSection->LockCount);
		WaitForSingleObject((HANDLE)lpCriticalSection->LockCount, INFINITE);
		//WaitForSingleObject((HANDLE)lpCriticalSection->LockCount, 1);
	}
	else {
		OutDebugSYS("%s: cs={lockcount=%d spincount=%d reccount=%d locksem=%#x ownthread=%#x}\n", ApiRef, 
			lpCriticalSection->LockCount,
			lpCriticalSection->SpinCount,
			lpCriticalSection->RecursionCount,
			lpCriticalSection->LockSemaphore,
			lpCriticalSection->OwningThread);
		(*pEnterCriticalSection)(lpCriticalSection);
	}

	//if(dxw.dwFlags11 & DELAYCRITSECTION) DELAYACTIVITY;
	OutDebugSYS("%s: terminated\n", ApiRef);
}

void WINAPI extLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	ApiName("LeaveCriticalSection");
	OutDebugSYS("%s: lpcs=%#x\n", ApiRef, lpCriticalSection);

	if(dxw.dwFlags11 & DELAYCRITSECTION) DELAYACTIVITY;

	if(dxw.dwFlags11 & MUTEX4CRITSECTION){
		OutTraceSYS("%s: mutex handle=%#x\n", ApiRef, lpCriticalSection->LockCount);
		ReleaseMutex((HANDLE)lpCriticalSection->LockCount);
	}
	else {
		OutDebugSYS("%s: cs={lockcount=%d spincount=%d reccount=%d locksem=%#x ownthread=%#x}\n", ApiRef, 
			lpCriticalSection->LockCount,
			lpCriticalSection->SpinCount,
			lpCriticalSection->RecursionCount,
			lpCriticalSection->LockSemaphore,
			lpCriticalSection->OwningThread);
		(*pLeaveCriticalSection)(lpCriticalSection);
	}

	//if(dxw.dwFlags11 & DELAYCRITSECTION) DELAYACTIVITY;
	OutDebugSYS("%s: terminated\n", ApiRef);
}

void WINAPI extDeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	ApiName("DeleteCriticalSection");
	OutTraceSYS("%s: lpcs=%#x\n", ApiRef, lpCriticalSection);

	if(dxw.dwFlags11 & MUTEX4CRITSECTION){
		OutTraceSYS("%s: mutex handle=%#x\n", ApiRef, lpCriticalSection->LockCount);
		CloseHandle((HANDLE)lpCriticalSection->LockCount);
	}
	else {
		OutTraceSYS("%s: cs={lockcount=%d spincount=%d reccount=%d locksem=%#x ownthread=%#x}\n", ApiRef, 
			lpCriticalSection->LockCount,
			lpCriticalSection->SpinCount,
			lpCriticalSection->RecursionCount,
			lpCriticalSection->LockSemaphore,
			lpCriticalSection->OwningThread);
		(*pDeleteCriticalSection)(lpCriticalSection);
	}

	OutTraceSYS("%s: terminated\n", ApiRef);
}

// ===========================================================================
// Global memory handling
// ===========================================================================

BOOL WINAPI extGlobalUnlock(HGLOBAL hMem)
{
	BOOL res;
	ApiName("GlobalUnlock");
	OutDebugSYS("%s: hg=%#x\n", ApiRef, hMem);
	res=(*pGlobalUnlock)(hMem);
	int err = GetLastError();
	if((res == 0) && err){
		OutErrorSYS("%s: ERROR ret=%#x err=%d\n", ApiRef, res, err);
	}
	else {
		OutDebugSYS("%s: ret=%#x\n", ApiRef, res);
	}
	if((dxw.dwFlags7 & FIXGLOBALUNLOCK) && (res == 1)){
		static HGLOBAL hLastMem = NULL;
		if((hLastMem == NULL) || (hMem == hLastMem)){
			OutTraceDW("%s: FIXED RETCODE hMem=%#x\n", ApiRef, hMem);
			res = 0;
			SetLastError(NO_ERROR);
			hLastMem = NULL;
		}
		else {
			hLastMem = hMem;
		}
	}
	return res;
}

#ifdef TRACEALL
HLOCAL WINAPI extLocalAlloc(UINT uFlags, SIZE_T uBytes)
{
	HLOCAL res;
	ApiName("LocalAlloc");
	OutTrace("%s: flags=%#x size=%d\n", ApiRef, uFlags, uBytes);
	res=(*pLocalAlloc)(uFlags, uBytes);
	OutTrace("%s: ret=%#x\n", ApiRef, res);
	return res;
}

HLOCAL WINAPI extLocalFree(HLOCAL hMem)
{
	HLOCAL res;
	ApiName("LocalFree");
	OutTrace("%s: hMem=%#x\n", ApiRef, hMem);
	res=(*pLocalFree)(hMem);
	OutTrace("%s: ret=%#x\n", ApiRef, res);
	return res;
}

LPVOID WINAPI extGlobalLock(HGLOBAL hMem)
{	
	LPVOID res;
	ApiName("GlobalLock");
	OutTrace("%s: hg=%#x\n", ApiRef, hMem);
	res=(*pGlobalLock)(hMem);
	OutTrace("%s: ret=%#x\n", ApiRef, res);
	return res;
}

HGLOBAL WINAPI extGlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
	HGLOBAL res;
	ApiName("GlobalAlloc");
	OutTrace("%s: flags=%#x size=%d\n", ApiRef, uFlags, dwBytes);
	res=(*pGlobalAlloc)(uFlags, dwBytes);
	OutTrace("%s: ret=%#x\n", ApiRef, res);
	return res;
}

HGLOBAL WINAPI extGlobalReAlloc(HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags)
{
	HGLOBAL res;
	ApiName("GlobalRealloc");
	OutTrace("%s: hmem=%#x flags=%#x size=%d\n", ApiRef, hMem, uFlags, dwBytes);
	res=(*pGlobalReAlloc)(hMem, dwBytes, uFlags);
	OutTrace("%s: ret=%#x\n", ApiRef, res);
	return res;
}

VOID WINAPI extRaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nArgs, CONST ULONG_PTR *lpArguments)
{
	ApiName("RaiseException");
	OutTrace("%s: code=%Xh flags=%Xh args=%d\n", ApiRef, dwExceptionCode, dwExceptionFlags, nArgs);
	(*pRaiseException)(dwExceptionCode, dwExceptionFlags, nArgs, lpArguments);
}

HGLOBAL WINAPI extGlobalHandle(LPCVOID pMem)
{
	ApiName("GlobalHandle");
	HGLOBAL res;
	res=(*pGlobalHandle)(pMem);
	OutTrace("%s: pMem=%#x ret=%#x\n", ApiRef, pMem, res);
	return res;
}
#endif // TRACEALL

// v2.05.72: promoted to fixed hook
HGLOBAL WINAPI extGlobalFree(HGLOBAL hMem)
{
	HGLOBAL res;
	ApiName("GlobalFree");

	OutDebugSYS("%s: hmem=%#x\n", ApiRef, hMem);

	// v2.05.76: recursion fix.
	static BOOL bRecursed = FALSE;
	if(bRecursed) return (*pGlobalFree)(hMem);
	bRecursed = TRUE;

	if(dxw.dwFlags14 & HEAPLEAK) {
		//if(!HeapValidate(hHeap, 0, lpMem)){		
			OutTraceDW("%s: HEAPLEAK skipping GlobalFree(%#x)\n", ApiRef, hMem);
			bRecursed = FALSE;
			return (HGLOBAL)NULL;
		//}
	}

	__try {
		res = (*pGlobalFree)(hMem);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutErrorSYS("%s: EXCEPTION hmem=%#x\n", ApiRef, hMem);
		res = NULL; // fake a success
	}
	if(res) OutErrorSYS("%s: ret=%#x err=%d\n", ApiRef, res, GetLastError());
	return res;
}

// -------------------- REMAPSYSFOLDERS hooks --------------------

static char *GetFakeWinDirA()
{
	static char myPathA[MAX_PATH];
	GetModuleFileNameA(NULL, myPathA, MAX_PATH);
	//PathRemoveFileSpecA(myPathA);
	char *p = myPathA + strlen(myPathA);
	while((*p != '\\') && (p > myPathA)) p--;
	if(*p == '\\') *p=0;

	strcat_s(myPathA, MAX_PATH, "\\Windows");
	return myPathA;
}

static WCHAR *GetFakeWinDirW()
{
	static WCHAR myPathW[MAX_PATH];
	GetModuleFileNameW(NULL, myPathW, MAX_PATH);
	//PathRemoveFileSpecW(myPathW);
	WCHAR *p = myPathW + wcslen(myPathW);
	while((*p != L'\\') && (p > myPathW)) p--;
	if(*p == L'\\') *p=0;

	wcscat_s(myPathW, MAX_PATH, L"\\Windows");
	return myPathW;
}

static BOOL isRelativePathA(LPCSTR path)
{
	// v2.05.19: NULL path is to be considered as relative 
	if(path == NULL) return TRUE;

	char *lpFolder = GetFakeWinDirA();
	if(!strncmp(path, lpFolder, strlen(lpFolder))){
		return FALSE;
	}
	if(strlen(path) > 2){
		if((path[1]==':') && (path[2]=='\\')) return FALSE;
		// v2.06.03 @#@ "Cheonsang Soma Yeongung-jeon II" (Ko 1997)
		if((path[0]=='.') && (path[1]=='\\')) return FALSE;
	}
	return TRUE;
}

static BOOL isRelativePathW(LPCWSTR path)
{
	// v2.05.29: NULL path is to be considered as relative 
	if(path == NULL) return TRUE;

	WCHAR *lpFolder = GetFakeWinDirW();
	if(!wcsncmp(path, lpFolder, wcslen(lpFolder))){
		return FALSE;
	}
	if(wcslen(path) > 2){
		if((path[1]==L':') && (path[2]==L'\\')) return FALSE;
		// v2.06.03
		if((path[0]==L'.') && (path[1]==L'\\')) return FALSE;
	}
	return TRUE;
}

UINT WINAPI extGetWindowsDirectoryA(LPSTR lpBuffer, UINT uSize)
{
	UINT ret;
	ApiName("GetWindowsDirectoryA");
	char *sFakePath = GetFakeWinDirA();
	ret = strlen(sFakePath);
	strncpy(lpBuffer, sFakePath, uSize);
	OutTraceDW("%s: returning relative path\n", ApiRef);
	return ret;
}

UINT WINAPI extGetWindowsDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	UINT ret;
	ApiName("GetWindowsDirectoryW");
	WCHAR *sFakePath = GetFakeWinDirW();
	ret = wcslen(sFakePath);
	wcsncpy(lpBuffer, sFakePath, uSize);
	OutTraceDW("%s: returning relative path\n", ApiRef);
	return ret;
}

DWORD WINAPI extGetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
	DWORD ret;
	ApiName("GetPrivateProfileStringA");
	char lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=%s key=%s default=\"%s\" size=%d filename=\"%s\"\n", ApiRef, lpAppName, lpKeyName, lpDefault, nSize, lpFileName);
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathA(lpFileName)){
		strcpy(lpNewPath, GetFakeWinDirA());
		strcat(lpNewPath, "\\");
		// v2.06.08: neutralize registry remapping for Control.ini, System.ini and Winfile.ini
		if(!_stricmp(lpFileName, "Control.ini")) strcat(lpNewPath, ".Control.ini");
		else
		if(!_stricmp(lpFileName, "System.ini")) strcat(lpNewPath, ".System.ini");
		else
		if(!_stricmp(lpFileName, "Winfile.ini")) strcat(lpNewPath, ".Winfile.ini");
		else
		strcat(lpNewPath, lpFileName ? lpFileName : "Win.ini");
		lpFileName = lpNewPath;
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpFileName);
	}
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpFileName) {
		lpFileName = dxwTranslatePathA(lpFileName, NULL);
	}
	ret = (*pGetPrivateProfileStringA)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
	OutTraceSYS("%s: ret=%#x retstring=\"%s\"\n", ApiRef, ret, lpReturnedString);
	return ret;
}

DWORD WINAPI extGetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	DWORD ret;
	ApiName("GetPrivateProfileStringW");
	WCHAR lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%ls\" key=\"%ls\" default=\"%ls\" size=%d filename=\"%ls\"\n", ApiRef, lpAppName, lpKeyName, lpDefault, nSize, lpFileName);
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathW(lpFileName)){
		wcscpy(lpNewPath, GetFakeWinDirW());
		wcscat(lpNewPath, L"\\");
		wcscat(lpNewPath, lpFileName ? lpFileName : L"Win.ini");
		lpFileName = lpNewPath;
		OutTraceDW("%s: remapped path=\"%ls\"\n", ApiRef, lpFileName);
	}
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpFileName) {
		lpFileName = dxwTranslatePathW(lpFileName, NULL);
	}
	ret = (*pGetPrivateProfileStringW)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
	OutTraceSYS("%s: ret=%#x retstring=\"%ls\"\n", ApiRef, ret, lpReturnedString);
	return ret;
}

DWORD WINAPI extWritePrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPSTR lpString, LPCSTR lpFileName)
{
	DWORD ret;
	ApiName("WritePrivateProfileStringA");
	char lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" val=\"%s\" filename=\"%s\"\n", ApiRef, lpAppName, lpKeyName, lpString, lpFileName);
	if(isRelativePathA(lpFileName)){
		strcpy(lpNewPath, GetFakeWinDirA());
		strcat(lpNewPath, "\\");
		strcat(lpNewPath, lpFileName); // should I set to Win.ini if NULL??
		lpFileName = lpNewPath;
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpFileName);
	}
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		lpFileName = dxwTranslatePathA(lpFileName, &mapping);
		// what if we try to write on a CD ini file?
		if(mapping == DXW_FAKE_CD) return TRUE; 
	}
	ret = (*pWritePrivateProfileStringA)(lpAppName, lpKeyName, lpString, lpFileName);
	if(!ret) OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

DWORD WINAPI extGetProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize)
{
	DWORD ret;
	ApiName("GetProfileStringA");
	char sPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" default=\"%s\" size=%d\n", ApiRef, lpAppName, lpKeyName, lpDefault, nSize);
	strcpy_s(sPath, GetFakeWinDirA());
	strcat_s(sPath, "\\Win.ini");
	OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, sPath);
	ret = (*pGetPrivateProfileStringA)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, sPath);
	OutTraceSYS("%s: ret=%#x retstring=\"%s\"\n", ApiRef, ret, lpReturnedString);
	return ret;
}

DWORD WINAPI extWriteProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPSTR lpString)
{
	DWORD ret;
	ApiName("WriteProfileStringA");
	char sPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" val\"=%s\"\n", ApiRef, lpAppName, lpKeyName, lpString);
	strcpy_s(sPath, GetFakeWinDirA());
	strcat_s(sPath, "\\Win.ini");
	OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, sPath);
	ret = (*pWritePrivateProfileStringA)(lpAppName, lpKeyName, lpString, sPath);
	return ret;
}

UINT WINAPI extGetPrivateProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName)
{
	DWORD ret;
	ApiName("GetPrivateProfileIntA");
	char lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" default=%d filename=\"%s\"\n", ApiRef, lpAppName, lpKeyName, nDefault, lpFileName);
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathA(lpFileName)){
		strcpy(lpNewPath, GetFakeWinDirA());
		strcat(lpNewPath, "\\");
		strcat(lpNewPath, lpFileName ? lpFileName : "Win.ini");
		lpFileName = lpNewPath;
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpFileName);
	}
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpFileName) {
		lpFileName = dxwTranslatePathA(lpFileName, NULL);
	}
	ret = (*pGetPrivateProfileIntA)(lpAppName, lpKeyName, nDefault, lpFileName);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

UINT WINAPI extGetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName)
{
	DWORD ret;
	ApiName("GetPrivateProfileIntW");
	WCHAR lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%ls\" key=\"%ls\" default=%d filename=\"%ls\"\n", ApiRef, lpAppName, lpKeyName, nDefault, lpFileName);
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathW(lpFileName)){
		wcscpy(lpNewPath, GetFakeWinDirW());
		wcscat(lpNewPath, L"\\");
		wcscat(lpNewPath, lpFileName ? lpFileName : L"Win.ini");
		lpFileName = lpNewPath;
		OutTraceDW("%s: remapped path=\"%ls\"\n", ApiRef, lpFileName);
	}
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpFileName) {
		lpFileName = dxwTranslatePathW(lpFileName, NULL);
	}
	ret = (*pGetPrivateProfileIntW)(lpAppName, lpKeyName, nDefault, lpFileName);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

UINT WINAPI extGetProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault)
{
	DWORD ret;
	ApiName("GetProfileIntA");
	char sPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" default=%d\n", ApiRef, lpAppName, lpKeyName, nDefault);
	strcpy_s(sPath, MAX_PATH, GetFakeWinDirA());
	strcat_s(sPath, MAX_PATH, "\\Win.ini");
	OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, sPath);
	ret = (*pGetPrivateProfileIntA)(lpAppName, lpKeyName, nDefault, sPath);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

UINT WINAPI extGetProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault)
{
	DWORD ret;
	ApiName("GetProfileIntW");
	WCHAR sPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%ls\" key=\"%ls\" default=%d\n", ApiRef, lpAppName, lpKeyName, nDefault);
	wcscpy_s(sPath, MAX_PATH, GetFakeWinDirW());
	wcscat_s(sPath, MAX_PATH, L"\\Win.ini");
	OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, sPath);
	ret = (*pGetPrivateProfileIntW)(lpAppName, lpKeyName, nDefault, sPath);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

DWORD WINAPI extGetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR *lpFilePart)
{
	DWORD ret;
	ApiName("GetFullPathNameA");
	OutTraceSYS("%s: filename=\"%s\" buflen=%d buf=%#x filepart=%#x\n", ApiRef, lpFileName, nBufferLength, lpBuffer, lpFilePart);

	char sArgLine[MAX_PATH+1];
	// v2.05.98 fix: added EMULATEWIN95 fix
	if((dxw.dwFlags15 & EMULATEWIN95) && !lpFileName){
		// if the exe filename is not accessible, provide it through Argv[0]
		// fixes "Indicar Racing II"
		strncpy(sArgLine, (*pGetCommandLineA)(), MAX_PATH);
		sArgLine[MAX_PATH] = 0;
		char *p = &sArgLine[1];
		if(sArgLine[0] == '"') {
			for(; *p && (*p!='"'); p++);
			*p = 0;
			lpFileName = &sArgLine[1];
		}
		else {
			for(; *p && (*p!=' '); p++);
			*p = 0;
			lpFileName = sArgLine;
		}
		OutTraceDW("%s: EMULATEWIN95 replace lpFileName=NULL->\"%s\"\n", ApiRef, lpFileName);
	}

#if 0
	if(dxw.dwFlags12 & PATHLOCALE){
		int size = strlen(lpFileName);
		WCHAR *lpFileNameW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
		int n = (*pMultiByteToWideChar)(dxw.CodePage, 0, lpFileName, size, lpFileNameW, size);
		lpFileNameW[n] = L'\0'; // make tail ! 
		OutTraceLOC("%s: WIDE path=\"%ls\"\n", ApiRef, lpFileNameW);
		WCHAR *lpBufferW = (WCHAR *)malloc(nBufferLength * sizeof(WCHAR));
		WCHAR *lpFilePartW = NULL;
		ret = (*pGetFullPathNameW)(lpFileNameW, nBufferLength, lpBufferW, &lpFilePartW);
		// revert data to ANSI structure. v2.05.71 fix.
		OutTrace("!!! WIDE lpBufferW=\"%ls\"\n", lpBufferW);
		OutTrace("!!! WIDE lpFilePartW=\"%ls\"\n", lpFilePartW);
		(*pWideCharToMultiByte)(dxw.CodePage, 0, lpBufferW, n, lpBuffer, nBufferLength, 0, 0);
		//*lpFilePart = *lpFilePartW ? lpBuffer + ((*lpFilePartW - &lpBufferW) / sizeof(WCHAR)) : NULL;
		free(lpFileNameW);
		free(lpBufferW);
	}
	else {	
		ret = (*pGetFullPathNameA)(lpFileName, nBufferLength, lpBuffer, lpFilePart);
	}
#else
	ret = (*pGetFullPathNameA)(lpFileName, nBufferLength, lpBuffer, lpFilePart);
#endif

	if(ret){
		OutTraceSYS("%s: ret=%d fullpath=\"%s\" part=\"%s\"\n", ApiRef, ret, 
			(lpBuffer && nBufferLength) ? lpBuffer : "", 
			(lpFilePart && lpBuffer && nBufferLength) ? *lpFilePart : ""
		);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d", ApiRef, GetLastError()); 
	}
	return ret;
}

BOOL WINAPI extSetThreadLocale(LCID Locale)
{
	BOOL ret;
	ApiName("SetThreadLocale");
	OutTraceSYS("%s: locale=%#x\n", ApiRef, Locale);
	ret = (*pSetThreadLocale)(Locale);
	//ret = (*pSetThreadLocale)(dxw.Locale);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extIsDBCSLeadByte(BYTE testchar)
{
	BOOL ret;
	ApiName("IsDBCSLeadByte");
	if(!pIsDBCSLeadByteEx) pIsDBCSLeadByteEx = IsDBCSLeadByteEx; // v2.05.56 fix
	ret = (*pIsDBCSLeadByteEx)(dxw.CodePage, testchar);
	OutTraceLOC("%s: char=%#x ret=%#x\n", ApiRef, testchar, ret);
	return ret;
}

BOOL WINAPI extIsDBCSLeadByteEx(UINT CodePage, BYTE testchar)
{
	BOOL ret;
	ApiName("IsDBCSLeadByteEx");
	ret = IsDBCSLeadByteEx(dxw.CodePage, testchar);
	OutTraceLOC("%s: codepage=%d char=%#x ret=%#x\n", ApiRef, CodePage, testchar, ret);
	return ret;
}

UINT WINAPI extGetACP()
{
	UINT ret;
	ApiName("GetACP");
	ret = (*pGetACP)();
	OutTraceLOC("%s: cp=%d->%d\n", ApiRef, ret, dxw.CodePage);
	return dxw.CodePage;
}

UINT WINAPI extGetOEMCP()
{
	UINT ret;
	ApiName("GetOEMCP");
	ret = (*pGetOEMCP)();
	OutTraceLOC("%s: cp=%d->%d\n", ApiRef, ret, dxw.CodePage);
	return dxw.CodePage;
}

#ifndef DXW_NOTRACES
static char *ExplainLCFlags(DWORD f)
{
	static char eb[256];
	unsigned int l;
	eb[0]=0;
	if (f & MB_PRECOMPOSED) strcat(eb, "MB_PRECOMPOSED+");
	if (f & MB_COMPOSITE) strcat(eb, "MB_COMPOSITE+");
	if (f & MB_USEGLYPHCHARS) strcat(eb, "MB_USEGLYPHCHARS+");
	if (f & MB_ERR_INVALID_CHARS) strcat(eb, "MB_ERR_INVALID_CHARS+");
	if (f & WC_COMPOSITECHECK) strcat(eb, "WC_COMPOSITECHECK+");
	if (f & WC_DISCARDNS) strcat(eb, "WC_DISCARDNS+");
	if (f & WC_SEPCHARS) strcat(eb, "WC_SEPCHARS+");
	if (f & WC_DEFAULTCHAR) strcat(eb, "WC_DEFAULTCHAR+");
	if (f & WC_ERR_INVALID_CHARS) strcat(eb, "WC_ERR_INVALID_CHARS+");
	if (f & WC_NO_BEST_FIT_CHARS) strcat(eb, "WC_NO_BEST_FIT_CHARS+");
	l=strlen(eb);
	if (l) eb[l-1]=0; // delete last '+' if any
	return(eb);
}
#endif

int WINAPI extMultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
	int ret;
	ApiName("MultiByteToWideChar");
	UINT dwCodePage; 
	OutTraceLOC("%s: cp=%d flags=%#x(%s) cbMultiByte=%d cchWideChar=%d\n", 
		ApiRef, CodePage, dwFlags, ExplainLCFlags(dwFlags), cbMultiByte, cchWideChar);
	dwCodePage = (CodePage >= CP_UTF7/*support UTF-7 UTF-8*/) ? CodePage : dxw.CodePage;
	ret = (*pMultiByteToWideChar)(dwCodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
#ifndef DXW_NOTRACES
	if(IsDebugLOC){
		if(cbMultiByte == -1) {
			OutTrace("< mbstr=\"%s\"\n", lpMultiByteStr);
		}
		else {
			OutTrace("< mbstr=\"%.*s\"\n", cbMultiByte, lpMultiByteStr);
		}
		if(cchWideChar){
			OutTrace("> wcstr=\"%.*ls\"\n", ret, lpWideCharStr);
		}
		OutTrace("> ret(len)=%d\n", ret);
	}
#endif // DXW_NOTRACES
	return ret;
}

int WINAPI extWideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
	int ret;
	ApiName("WideCharToMultiByte");
	UINT dwCodePage; 
	OutTraceLOC("%s: cp=%d flags=%#x(%s) cchWideChar=%d cbMultiByte=%d\n",
		ApiRef, CodePage, dwFlags, ExplainLCFlags(dwFlags), cchWideChar, cbMultiByte);
	dwCodePage = (CodePage >= CP_UTF7/*support UTF-7 UTF-8*/) ? CodePage : dxw.CodePage;
	ret = (*pWideCharToMultiByte)(dwCodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
#ifndef DXW_NOTRACES
	if(IsDebugLOC){
		if(lpDefaultChar) OutTrace("< defchar=\"%c\"\n", *lpDefaultChar);
		if((DWORD)lpMultiByteStr){
			if(cchWideChar == -1)
				OutTrace("< wcstr=\"%ls\"\n", lpMultiByteStr);
			else
				OutTrace("< wcstr=\"%.*ls\"\n", cchWideChar, lpMultiByteStr);
		}
		if(cbMultiByte){
			OutTrace("> mbstr=\"%s\"\n", lpWideCharStr);
		}
		if(lpUsedDefaultChar) OutTrace("> useddefchar=\"%#x\"\n", *lpUsedDefaultChar);
		OutTrace("> ret(len)=%d\n", ret);
	}
#endif // DXW_NOTRACES
	return ret;
}

#ifndef DXW_NOTRACES
char *ResNames[]={
	"NULL", "CURSOR", "BITMAP", "ICON", "MENU", "DIALOG", "STRING",
	"FONTDIR", "FONT", "ACCELERATOR", "RCDATA", "MESSAGETABLE", "R12", "R13",
	"R14", "R15", "VERSION", "DLGINCLUDE", "R18", "PLUGPLAY", "VXD",
	"ANICURSOR", "ANIICON", "HTML"
};
#endif

HRSRC WINAPI extFindResourceA(HMODULE hModule, LPCSTR lpName, LPCSTR lpType)
{
	HRSRC hrsrc;
	ApiName("FindResourceA");

	BOOL isStrType = ((DWORD_PTR)lpType & WM_CLASSMASK);
	BOOL isStrName = ((DWORD_PTR)lpName & WM_CLASSMASK);

#ifndef DXW_NOTRACES
	if(IsTraceLOC){
		char hexType[10+1];
		char hexName[10+1];
		if(!isStrType) sprintf(hexType, "0x%04.4x", (DWORD)lpType);
		if(((DWORD)lpType) <= (DWORD)RT_HTML) strcpy(hexType, ResNames[(DWORD)lpType]); 
		if(!isStrName) sprintf(hexName, "0x%04.4x", (DWORD)lpName);
		OutTrace("%s: hmodule=%#x type=%s name=%s\n", 
			ApiRef, hModule, 
			isStrType ? lpType : hexType,
			isStrName ? lpName : hexName);
	}
#endif // DXW_NOTRACES

#if 0
	if(dxw.dwFlags11 & CUSTOMLOCALE){
		WORD wLanguage = (WORD)dxw.Locale;
		LPCWSTR lpTypeW = isStrType ? MultiByteToWideCharInternal(lpType) : (LPCWSTR)lpType;
		LPCWSTR lpNameW = isStrName ? MultiByteToWideCharInternal(lpName) : (LPCWSTR)lpName;
		hrsrc = FindResourceExW(hModule, lpTypeW, lpNameW, wLanguage);
		if (isStrName) FreeStringInternal((LPVOID)lpNameW);
		if (isStrType) FreeStringInternal((LPVOID)lpTypeW);
	}
	else {
		hrsrc = (*pFindResourceA)(hModule, lpName, lpType);
	}
#else
	hrsrc = (*pFindResourceA)(hModule, lpName, lpType);
#endif

	OutTraceLOC("%s: ret=%#x\n", ApiRef, hrsrc);
	return hrsrc;
}

HRSRC WINAPI extFindResourceExA(HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage)
{
	HRSRC hrsrc;
	ApiName("FindResourceExA");

	BOOL isStrType = ((DWORD_PTR)lpType & WM_CLASSMASK);
	BOOL isStrName = ((DWORD_PTR)lpName & WM_CLASSMASK);

#ifndef DXW_NOTRACES
	if(IsTraceSYS){
		char hexType[20+1];
		char hexName[20+1];
		if(!isStrType) sprintf(hexType, "0x%04.4x", (DWORD)lpType);
		if(((DWORD)lpType) <= (DWORD)RT_HTML) strcpy(hexType, ResNames[(DWORD)lpType]); 
		if(!isStrName) sprintf(hexName, "0x%04.4x", (DWORD)lpName);
		OutTrace("%s: hmodule=%#x type=%s name=%s lang=%d\n", 
			ApiRef, hModule, 
			isStrType ? lpType : hexType,
			isStrName ? lpName : hexName,
			wLanguage);
	}
#endif // DXW_NOTRACES

	if(!(isStrType || isStrName)) {
		hrsrc = (*pFindResourceExA)(hModule, lpType, lpName, wLanguage);
		OutTraceSYS("%s: ret(hrsrc)=%#x\n", ApiRef, hrsrc);
		return hrsrc;
	}

	// warning: better leave wLanguage unaltered when not set or some resources won't be found.
	// example: "Tang Poetry II"
	// v2.05.71 fix: don't touch wLanguage if it is 0: zero means all language is ok, so better leave 
	// it alone. Fixes Korean game "Zero" intro movies showing error because they couldn't find vids:IV50 codec.
	if((dxw.Locale) && wLanguage) wLanguage = (WORD)dxw.Locale;
	LPCWSTR lpTypeW = isStrType ? MultiByteToWideCharInternal(lpType) : (LPCWSTR)lpType;
	LPCWSTR lpNameW = isStrName ? MultiByteToWideCharInternal(lpName) : (LPCWSTR)lpName;
	hrsrc = FindResourceExW(hModule, lpTypeW, lpNameW, wLanguage);
	if (isStrName) FreeStringInternal((LPVOID)lpNameW);
	if (isStrType) FreeStringInternal((LPVOID)lpTypeW);
	OutTraceSYS("%s: ret(hrsrc)=%#x\n", ApiRef, hrsrc);
	return hrsrc;
}

// ??? for all the following 7 calls ntlea always returns the Locale ID 
// also when the Language ID is requested !!!

LANGID WINAPI extGetUserDefaultUILanguage(void)
{
	ApiName("GetUserDefaultUILanguage");
	OutTraceLOC("%s: ret=%d\n", ApiRef, dxw.Locale);
	return dxw.Locale;
}

LANGID WINAPI extGetSystemDefaultUILanguage(void)
{
	ApiName("GetSystemDefaultUILanguage");
	OutTraceLOC("%s: ret=%d\n", ApiRef, dxw.Locale);
	return dxw.Locale;
}

LANGID WINAPI extGetSystemDefaultLangID(void)
{
	ApiName("GetSystemDefaultLangID");
	OutTraceLOC("%s: ret=%d\n", ApiRef, dxw.Locale);
	return dxw.Locale;
}

LANGID WINAPI extGetUserDefaultLangID(void)
{
	ApiName("GetUserDefaultLangID");
	OutTraceLOC("%s: ret=%d\n", ApiRef, dxw.Locale);
	return dxw.Locale;
}

LCID WINAPI extGetThreadLocale(void)
{
	ApiName("GetThreadLocale");
	OutTraceLOC("%s: ret=%d\n", ApiRef, dxw.Locale);
	return dxw.Locale;
}

LCID WINAPI extGetSystemDefaultLCID(void)
{
	ApiName("GetSystemDefaultLCID");
	OutTraceLOC("%s: ret=%d\n", ApiRef, dxw.Locale);
	return dxw.Locale;
}

LCID WINAPI extGetUserDefaultLCID(void)
{
	ApiName("GetUserDefaultLCID");
	OutTraceLOC("%s: ret=%d\n", ApiRef, dxw.Locale);
	return dxw.Locale;
}

BOOL WINAPI extGetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPDWORD lpCharType)
{
	BOOL ret;
	ApiName("GetStringTypeA");
	OutTraceSYS("%s: locale=%d infotype=%#x str=(%d)\"%s\"\n", 
		ApiRef, Locale, dwInfoType, cchSrc, lpSrcStr);
	switch(Locale){
		case LOCALE_SYSTEM_DEFAULT:
		case LOCALE_USER_DEFAULT:
		case LOCALE_CUSTOM_DEFAULT:
		case LOCALE_CUSTOM_UI_DEFAULT:
		case LOCALE_CUSTOM_UNSPECIFIED:
			OutTraceLOC("%s: locale=%d->%d str=(%d)\"%s\"\n", 
				ApiRef, Locale, dxw.CodePage, cchSrc, lpSrcStr);
			Locale = dxw.Locale;
			break;
	}
	ret = (*pGetStringTypeA)(Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType);
	return ret;
}

BOOL WINAPI extGetStringTypeW(DWORD dwInfoType, LPCWCH lpSrcStr, int cchSrc, LPWORD lpCharType)
{
	BOOL ret;
	ApiName("GetStringTypeW");
	OutTraceSYS("%s: infotype=%#x str=(%d)\"%ls\"\n", 
		ApiRef, dwInfoType, cchSrc, lpSrcStr);
	//ret = (*pGetStringTypeExW)(dxw.CodePage, dwInfoType, lpSrcStr, cchSrc, lpCharType);
	ret = (*pGetStringTypeW)(dwInfoType, lpSrcStr, cchSrc, lpCharType);
	return ret;
}

BOOL WINAPI extGetStringTypeExA(LCID Locale, DWORD type, LPCSTR lpSrcStr, INT cchSrc, LPWORD chartype)
{
	BOOL ret;
	ApiName("GetStringTypeExA");
	OutTraceSYS("%s: loc=%d type=%#x str=\"%s\" count=%d\n", ApiRef, Locale, type, lpSrcStr, cchSrc);
	switch(Locale){
		case LOCALE_SYSTEM_DEFAULT:
		case LOCALE_USER_DEFAULT:
		case LOCALE_CUSTOM_DEFAULT:
		case LOCALE_CUSTOM_UI_DEFAULT:
		case LOCALE_CUSTOM_UNSPECIFIED:
			OutTraceLOC("%s: locale=%d->%d str=(%d)\"%s\"\n", 
				ApiRef, Locale, dxw.CodePage, cchSrc, lpSrcStr);
			Locale = dxw.Locale;
			break;
	}
	ret = (*pGetStringTypeExA)(Locale, type, lpSrcStr, cchSrc, chartype);
	OutTraceSYS("%s: ret=%#x chartype=%#x\n", ApiRef, ret, chartype ? 0 : *chartype);
	return ret;
}

BOOL WINAPI extGetStringTypeExW(LCID Locale, DWORD type, LPCWCH lpSrcStr, int cchSrc, LPWORD chartype)
{
	BOOL ret;
	ApiName("GetStringTypeExW");
	OutTraceSYS("%s: loc=%d type=%#x str=\"%ls\" count=%d\n", ApiRef, Locale, type, lpSrcStr, cchSrc);
	switch(Locale){
		case LOCALE_SYSTEM_DEFAULT:
		case LOCALE_USER_DEFAULT:
		case LOCALE_CUSTOM_DEFAULT:
		case LOCALE_CUSTOM_UI_DEFAULT:
		case LOCALE_CUSTOM_UNSPECIFIED:
			OutTraceLOC("%s: locale=%d->%d str=(%d)\"%s\"\n", 
				ApiRef, Locale, dxw.CodePage, cchSrc, lpSrcStr);
			Locale = dxw.Locale;
			break;
	}
	ret = (*pGetStringTypeExW)(Locale, type, lpSrcStr, cchSrc, chartype);
	OutTraceSYS("%s: ret=%#x chartype=%#x\n", ApiRef, ret, chartype ? 0 : *chartype);
	return ret;
}

int WINAPI extLCMapStringA(LCID Locale, DWORD dwMapFLags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest)
{
	int ret;
	ApiName("LCMapStringA");
	OutTraceSYS("%s: locale=%d flags=%#x str=(%d)\"%s\" dest=(%d)\n", 
		ApiRef, Locale, dwMapFLags, cchSrc, lpSrcStr, cchDest);
	switch(Locale){
		case LOCALE_SYSTEM_DEFAULT:
		case LOCALE_USER_DEFAULT:
		case LOCALE_CUSTOM_DEFAULT:
		case LOCALE_CUSTOM_UI_DEFAULT:
		case LOCALE_CUSTOM_UNSPECIFIED:
			OutTraceLOC("%s: locale=%d->%d str=(%d)\"%s\"\n", 
				ApiRef, Locale, dxw.Locale, cchSrc, lpSrcStr);
			Locale = dxw.Locale;
			break;
	}
	ret = (*pLCMapStringA)(Locale, dwMapFLags, lpSrcStr, cchSrc, lpDestStr, cchDest);
	return ret;
}

int WINAPI extLCMapStringW(LCID Locale, DWORD dwMapFLags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest)
{
	int ret;
	ApiName("LCMapStringW");
	OutTraceSYS("%s: locale=%d flags=%#x str=(%d)\"%ls\" dest=(%d)\n", 
		ApiRef, Locale, dwMapFLags, cchSrc, lpSrcStr, cchDest);
	switch(Locale){
		case LOCALE_SYSTEM_DEFAULT:
		case LOCALE_USER_DEFAULT:
		case LOCALE_CUSTOM_DEFAULT:
		case LOCALE_CUSTOM_UI_DEFAULT:
		case LOCALE_CUSTOM_UNSPECIFIED:
			OutTraceLOC("%s: locale=%d->%d str=(%d)\"%ls\"\n", 
				ApiRef, Locale, dxw.Locale, cchSrc, lpSrcStr);
			Locale = dxw.Locale;
			break;
	}
	ret = (*pLCMapStringW)(Locale, dwMapFLags, lpSrcStr, cchSrc, lpDestStr, cchDest);
	return ret;
}

#define ACCESS_FROM_CTL_CODE(c) (((DWORD)(c & 0x0000c000)) >> 14)
#define FUNCTION_FROM_CTL_CODE(c) (((DWORD)(c & 0x00003ffc)) >> 2)
#ifndef IOCTL_CDROM_READ_TOC
#define IOCTL_CDROM_READ_TOC 0x24000
#endif

#ifndef DXW_NOTRACES

// EXTENDED VALUES FROM https://github.com/secrary/ida-scripts/blob/master/IOCTL_decode.py
#ifndef FILE_DEVICE_MT_COMPOSITE
#define FILE_DEVICE_MT_COMPOSITE        0x00000042
#define FILE_DEVICE_MT_TRANSPORT        0x00000043
#define FILE_DEVICE_BIOMETRIC           0x00000044
#define FILE_DEVICE_PMI                 0x00000045
#define FILE_DEVICE_EHSTOR              0x00000046
#define FILE_DEVICE_DEVAPI              0x00000047
#define FILE_DEVICE_GPIO                0x00000048
#define FILE_DEVICE_USBEX               0x00000049
#define FILE_DEVICE_CONSOLE             0x00000050
#define FILE_DEVICE_NFP                 0x00000051
#define FILE_DEVICE_SYSENV              0x00000052
#define FILE_DEVICE_VIRTUAL_BLOCK       0x00000053
#define FILE_DEVICE_POINT_OF_SERVICE    0x00000054
#define FILE_DEVICE_STORAGE_REPLICATION 0x00000055
#define FILE_DEVICE_TRUST_ENV           0x00000056
#define FILE_DEVICE_UCM                 0x00000057
#define FILE_DEVICE_UCMTCPCI            0x00000058
#define FILE_DEVICE_PERSISTENT_MEMORY   0x00000059
#define FILE_DEVICE_NVDIMM              0x0000005a
#define FILE_DEVICE_HOLOGRAPHIC         0x0000005b
#define FILE_DEVICE_SDFXHCI             0x0000005c
#endif

static char *sDeviceType(DWORD dev)
{
	char *devices[] = {
		"BEEP",
		"CD_ROM",
		"CD_ROM_FILE_SYSTEM",
		"CONTROLLER",
		"DATALINK",
		"DFS",
		"DISK",
		"DISK_FILE_SYSTEM",
		"FILE_SYSTEM",
		"INPORT_PORT",
		"KEYBOARD",
		"MAILSLOT",
		"MIDI_IN",
		"MIDI_OUT",
		"MOUSE",
		"MULTI_UNC_PROVIDER",
		"NAMED_PIPE",
		"NETWORK",
		"NETWORK_BROWSER",
		"NETWORK_FILE_SYSTEM",
		"NULL",
		"PARALLEL_PORT",
		"PHYSICAL_NETCARD",
		"PRINTER",
		"SCANNER",
		"SERIAL_MOUSE_PORT",
		"SERIAL_PORT",
		"SCREEN",
		"SOUND",
		"STREAMS",
		"TAPE",
		"TAPE_FILE_SYSTEM",
		"TRANSPORT",
		"UNKNOWN",
		"VIDEO",
		"VIRTUAL_DISK",
		"WAVE_IN",
		"WAVE_OUT",
		"8042_PORT",
		"NETWORK_REDIRECTOR",
		"BATTERY",
		"BUS_EXTENDER",
		"MODEM",
		"VDM",
		"MASS_STORAGE",
		"SMB",
		"KS",
		"CHANGER",
		"SMARTCARD",
		"ACPI",
		"DVD",
		"FULLSCREEN_VIDEO",
		"DFS_FILE_SYSTEM",
		"DFS_VOLUME",
		"SERENUM",
		"TERMSRV",
		"KSEC",
		"FIPS",
		"INFINIBAND",
		"?", // 3C
		"?", // 3D
		"VMBUS",
		"CRYPT_PROVIDER",
		"WPD",
		"BLUETOOTH",
		"MT_COMPOSITE",
		"MT_TRANSPORT",
		"BIOMETRIC",
		"PMI",
		"EHSTOR",
		"DEVAPI",
		"GPIO",
		"USBEX",
		"?", // 4A
		"?", // 4B
		"?", // 4C
		"?", // 4D
		"?", // 4E
		"?", // 4F
		"CONSOLE",
		"NFP",
		"SYSENV",
		"VIRTUAL_BLOCK",
		"POINT_OF_SERVICE",
		"STORAGE_REPLICATION",
		"TRUST_ENV",
		"UCM",
		"UCMTCPCI",
		"PERSISTENT_MEMORY",
		"NVDIMM",
		"HOLOGRAPHIC",
		"SDFXHCI"
	};
	if((dev > 0) && (dev <= FILE_DEVICE_SDFXHCI)) return devices[dev-1];
	return ("unknown");
}

static char *sAccess(DWORD acc)
{
	char *accesses[] = {
		"ANY_ACCESS",
		"READ_ACCESS", 
		"WRITE_ACCESS",
		"READ+WRITE_ACCESS"
	};
	return accesses[acc & 0x3];
}

static char *sMethod(DWORD m)
{
	char *methods[] = {
		"BUFFERED",
		"IN_DIRECT",
		"OUT_DIRECT",
		"NEITHER"
	};
	return methods[m & 0x3];
}

static void DumpIOControlCode(ApiArg, DWORD ctrlCode)
{
	DWORD dwDeviceType, dwFunction, dwMethod, dwAccess;
	dwDeviceType = DEVICE_TYPE_FROM_CTL_CODE(ctrlCode);
	dwFunction = FUNCTION_FROM_CTL_CODE(ctrlCode);
	dwMethod = METHOD_FROM_CTL_CODE(ctrlCode);
	dwAccess = ACCESS_FROM_CTL_CODE(ctrlCode);
	OutTrace("%s: code {DeviceType=%#x(FILE_DEVICE_%s) Function=%#x Method=%#x(METHOD_%s) Access=%#x(%s)}\n", 
		ApiRef, 
		dwDeviceType, sDeviceType(dwDeviceType), 
		dwFunction, 
		dwMethod, sMethod(dwMethod), 
		dwAccess, sAccess(dwAccess));
}

static char *sKnownCode(DWORD code)
{
	char *ret = "";
	switch(code){
		case IOCTL_SCSI_PASS_THROUGH: ret="IOCTL_SCSI_PASS_THROUGH"; break;
		case IOCTL_SCSI_PASS_THROUGH_DIRECT: ret="IOCTL_SCSI_PASS_THROUGH_DIRECT"; break; // Heaven & Hell Securom
		case IOCTL_CDROM_READ_TOC: ret = "IOCTL_CDROM_READ_TOC"; break;
		case IOCTL_CDROM_STOP_AUDIO: ret = "IOCTL_CDROM_STOP_AUDIO"; break;
		case IOCTL_CDROM_PAUSE_AUDIO: ret = "IOCTL_CDROM_PAUSE_AUDIO"; break;
		case IOCTL_CDROM_RESUME_AUDIO: ret = "IOCTL_CDROM_RESUME_AUDIO"; break;
		case IOCTL_CDROM_GET_VOLUME: ret = "IOCTL_CDROM_GET_VOLUME"; break;
		case IOCTL_CDROM_PLAY_AUDIO_MSF: ret = "IOCTL_CDROM_PLAY_AUDIO_MSF"; break;
		case IOCTL_CDROM_SET_VOLUME: ret = "IOCTL_CDROM_SET_VOLUME"; break;
		case IOCTL_CDROM_READ_Q_CHANNEL: ret = "IOCTL_CDROM_READ_Q_CHANNEL"; break;
		case IOCTL_STORAGE_CHECK_VERIFY: ret = "IOCTL_STORAGE_CHECK_VERIFY"; break;
		case IOCTL_STORAGE_CHECK_VERIFY2: ret = "IOCTL_STORAGE_CHECK_VERIFY2"; break;
		case IOCTL_STORAGE_MEDIA_REMOVAL: ret = "IOCTL_STORAGE_MEDIA_REMOVAL"; break;
		case IOCTL_DISK_MEDIA_REMOVAL: ret = "IOCTL_DISK_MEDIA_REMOVAL"; break; // Rugrats Adventure Game
		case OBSOLETE_IOCTL_CDROM_GET_CONTROL: ret = "OBSOLETE_IOCTL_CDROM_GET_CONTROL"; break; 
		case IOCTL_CDROM_GET_LAST_SESSION: ret = "IOCTL_CDROM_GET_LAST_SESSION"; break; 
		case IOCTL_CDROM_RAW_READ: ret = "IOCTL_CDROM_RAW_READ"; break; 
		case IOCTL_CDROM_DISK_TYPE: ret = "IOCTL_CDROM_DISK_TYPE"; break; 
		case IOCTL_CDROM_GET_DRIVE_GEOMETRY: ret = "IOCTL_CDROM_GET_DRIVE_GEOMETRY"; break; 
		case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX: ret = "IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX"; break; 
		case IOCTL_CDROM_READ_TOC_EX: ret = "IOCTL_CDROM_READ_TOC_EX"; break; 
		case IOCTL_CDROM_GET_CONFIGURATION: ret = "IOCTL_CDROM_GET_CONFIGURATION"; break; 
		case IOCTL_CDROM_EXCLUSIVE_ACCESS: ret = "IOCTL_CDROM_EXCLUSIVE_ACCESS"; break; 
		case IOCTL_CDROM_SET_SPEED: ret = "IOCTL_CDROM_SET_SPEED"; break; 
		case IOCTL_CDROM_GET_INQUIRY_DATA: ret = "IOCTL_CDROM_GET_INQUIRY_DATA"; break; 
		case IOCTL_CDROM_ENABLE_STREAMING: ret = "IOCTL_CDROM_ENABLE_STREAMING"; break; 
		case IOCTL_CDROM_SEND_OPC_INFORMATION: ret = "IOCTL_CDROM_SEND_OPC_INFORMATION"; break; 
		case IOCTL_CDROM_GET_PERFORMANCE: ret = "IOCTL_CDROM_GET_PERFORMANCE"; break; 
	}
	return ret;
}
#endif // DXW_NOTRACES

BOOL WINAPI extDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	BOOL ret;
	ApiName("DeviceIoControl");
	OutTraceSYS("%s: hdevice=%#x code=%#x(%s) insize=%d outsize=%d\n", 
		ApiRef, hDevice, dwIoControlCode, sKnownCode(dwIoControlCode), nInBufferSize, nOutBufferSize);
#ifndef DXW_NOTRACES
	if(IsDebugSYS) {
		DumpIOControlCode(ApiRef, dwIoControlCode);
		if(IsTraceHex) HexTrace((LPBYTE)lpInBuffer, nInBufferSize);
	}
#endif // DXW_NOTRACES

	//if((dxw.dwFlags13 & SECUROMDETECT) && (dwIoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT)){
	//	MessageBox(0, "IOCTL_SCSI_PASS_THROUGH_DIRECT", "DxWnd", 0);
	//}

	if(dxw.dwFlags12 & LOCKCDTRAY){
		switch (dwIoControlCode){
			case IOCTL_STORAGE_EJECT_MEDIA:
				OutTraceDW("%s: prevent EJECT media\n", ApiRef);
				return TRUE;
				break;
			case IOCTL_STORAGE_LOAD_MEDIA:
				OutTraceDW("%s: prevent LOAD media\n", ApiRef);
				return TRUE;
				break;
			default:
				break;
		}
	}

	ret=(*pDeviceIoControl)(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
	if(ret){
		//OutTraceSYS("%s: OK retbytes=%d overlapped=%d\n", ApiRef, *lpBytesReturned, *lpOverlapped);
		OutTraceSYS("%s: OK outsize=%d\n", ApiRef, lpBytesReturned ? *lpBytesReturned : 0);
#ifndef DXW_NOTRACES
		if(IsDebugSYS && IsTraceHex) {
			if(lpBytesReturned && *lpBytesReturned) {
				OutTrace("> OutBuffer:\n");
				HexTrace((LPBYTE)lpOutBuffer, *lpBytesReturned);
			}
			if(lpOverlapped) {
				OutTrace("> Overlapped:\n");
				HexTrace((LPBYTE)lpOverlapped, sizeof(OVERLAPPED));
			}
		}
#endif // DXW_NOTRACES
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}

	if(ret && (dwIoControlCode == IOCTL_CDROM_READ_TOC)){
		PCDROM_TOC pToc = (PCDROM_TOC)lpOutBuffer;
		PTRACK_DATA pTrack = pToc->TrackData;
		int nTracks;
		OutTrace("TOC: len=%d Track:(first=%d last=%d)\n",
			(WORD)pToc->Length, pToc->FirstTrack, pToc->LastTrack);
		nTracks = pToc->LastTrack-pToc->FirstTrack;
		if(nTracks > 0) for(int i=0; i<nTracks; i++){
			OutTrace("Track %d: ctrl=%#x adr=%#x num=%d address=%#x\n",
				i+1, pTrack->Control, pTrack->Adr, pTrack->TrackNumber, pTrack->Address);
			pTrack++;
		}
	}

	// Rugrats Adventure Game - ???
	//if(!ret && (dwIoControlCode == IOCTL_DISK_MEDIA_REMOVAL)){
	//	// pretend it worked ...
	//	OutTraceDW("%s: simulate IOCTL_DISK_MEDIA_REMOVAL\n", ApiRef);
	//	ret = TRUE;
	//}

	return ret;
}

// ==== Fake Global Atom wrappers

UINT WINAPI extGlobalGetAtomNameA(ATOM nAtom, LPSTR lpBuffer, int nSize)
{
	UINT ret;
	ApiName("GlobalGetAtomNameA");
	OutTraceSYS("%s: atom=%#x size=%d\n", ApiRef, nAtom, nSize);
	__try {
		ret = (*pGlobalGetAtomNameA)(nAtom, lpBuffer, nSize);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutErrorSYS("%s: EXCEPTION err=%d\n", ApiRef, GetLastError());
		return 0;
	}
	if(ret) {
		OutTraceSYS("%s: buffer=\"%*.*s\"\n", ApiRef, ret, ret, lpBuffer);
		//OutTraceSYS("%s: buffer=%#x\n", ApiRef, ret, ret, lpBuffer);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return ret;
}

ATOM WINAPI extGlobalDeleteAtom(ATOM nAtom)
{
	ApiName("GlobalDeleteAtom");
	static int Kount = 0;
	OutTraceSYS("%s: atom=%#x\n", ApiRef, nAtom);
	if(dxw.dwFlags12 & FAKEGLOBALATOM) {
		ATOM ret = 0;
		if(Kount++) ret = (ATOM)0x1;
		OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
		return ret;
	}
	return (*pGlobalDeleteAtom)(nAtom);
}

ATOM WINAPI extGlobalAddAtomA(LPCSTR lpString)
{
	ApiName("GlobalAddAtomA");
	OutTraceSYS("%s: string=\"%s\"\n", ApiRef, lpString);
	if(dxw.dwFlags12 & FAKEGLOBALATOM) return (ATOM)0xBEEF;
	return (*pGlobalAddAtomA)(lpString);
}

ATOM WINAPI extGlobalFindAtomA(LPCSTR lpString)
{
	ApiName("GlobalFindAtomA");
	OutTraceSYS("%s: string=\"%s\"\n", ApiRef, lpString);
	if(dxw.dwFlags12 & FAKEGLOBALATOM) return (ATOM)0x0;
	return (*pGlobalFindAtomA)(lpString);
}

DWORD WINAPI extWaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
	ApiName("WaitForSingleObject");
	OutTraceSYS("%s: handle=%#x time=%#x\n", ApiRef, hHandle, dwMilliseconds);

    if ((dxw.dwFlags18 & EMUDDSYNCSHIM) && g_hThread) {
        //
        // Hack to find the DirectDraw mutex
        //
        if (!g_hDDMutex) FindMutex();
        if (g_hDDMutex && (hHandle == g_hDDMutex)) {
            //
            // Use our thread to acquire the mutex. We synchronize since we're
            // accessing globals to communicate with our thread.
            //
            DWORD dwRet;
			OutTraceDW("%s: EMUDDSYNCSHIM synchronizing ...\n", ApiRef);
            EnterCriticalSection(&g_csSync); 
            // Set globals to communicate with our thread
            g_dwTime = dwMilliseconds;
            g_dwWait = sWaitForSingleObject;
            ResetEvent(g_hDoneEvent);
            // Signal our thread to obtain the mutex
            SetEvent(g_hWaitEvent);
            // Wait until the state of the mutex has been determined
            (*pWaitForSingleObject)(g_hDoneEvent, INFINITE); 

            // Code to detect the degenerate
            if (g_dwWaitRetValue == WAIT_OBJECT_0) {
                g_dwMutexOwnerThreadId = GetCurrentThreadId();
            }
            dwRet = g_dwWaitRetValue;
            LeaveCriticalSection(&g_csSync);
            return dwRet;
        }
    }

	if((dxw.dwFlags12 & KILLDEADLOCKS) && (dwMilliseconds == INFINITE)) {
		OutTraceDW("%s: trimmed INFINITE time on hdl=%#x\n", ApiRef, hHandle);
		dwMilliseconds=1000;
	}

	if(!pWaitForSingleObject) {
		OutTraceDW("%s: missing hook\n", ApiRef);
		pWaitForSingleObject = WaitForSingleObject;
	}
	return (*pWaitForSingleObject)(hHandle, dwMilliseconds);
}

BOOL WINAPI extReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	BOOL ret;
	ApiName("ReadFile");
#ifndef DXW_NOTRACES
	if(IsTraceSYS){
		char sBuf[81];
		sBuf[0]=0; // empty string
		if(lpOverlapped) sprintf_s(sBuf, 80, "->(Offset=%#x OffsetHigh=%#x)", lpOverlapped->Offset, lpOverlapped->OffsetHigh);
		OutTrace("%s: hFile=%#x Buffer=%#x BytesToRead=%d Overlapped=%#x%s\n", ApiRef, hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, sBuf);
	}
#endif // DXW_NOTRACES

	ret = (*pReadFile)(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

	if((dxw.dwFlags20 & HANDLECDLOCK) && (*lpNumberOfBytesRead < nNumberOfBytesToRead) && (CDLockFile == hFile)){
		OutTrace("%s: pretending to read from file\n", ApiRef);
		memset(lpBuffer, 0, nNumberOfBytesToRead);
		if(!pSetFilePointer) pSetFilePointer = SetFilePointer;
		SetFilePointer(hFile, 0, NULL, 0);
		DWORD dummy;
		if(!(*pReadFile)(hFile, lpBuffer, 1, &dummy, NULL)){
			OutTrace("%s: CDLock emulation FAILED read=%d val=%c\n", ApiRef, dummy, ((char *)lpBuffer)[0]);
		}
		else{
			OutTrace("%s: CDLock emulation read=%d val=%#x\n", ApiRef, dummy, ((char *)lpBuffer)[0]);
		};
		*lpNumberOfBytesRead = nNumberOfBytesToRead;
		ret = TRUE;
	}

#ifndef DXW_NOTRACES
	if(ret){
		OutTraceSYS("%s: NumberOfBytesRead=%d\n", ApiRef, *lpNumberOfBytesRead);
		OutHexSYS((LPBYTE)lpBuffer, *lpNumberOfBytesRead);
	} else {
		OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
#endif // DXW_NOTRACES

	return ret;
}

#ifndef DXW_NOTRACES
UINT WINAPI ext_lread(HFILE hFile, LPVOID lpBuffer, UINT uBytes)
{
	UINT res;
	ApiName("_lread");
	OutTraceSYS("%s: hfile=%#x buf=%#x bytes=%d\n", ApiRef, hFile, lpBuffer, uBytes);
	res = (*p_lread)(hFile, lpBuffer, uBytes);
	OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	return res;
}

long WINAPI ext_hread(HFILE hFile, LPVOID lpBuffer, long lBytes)
{
	long res;
	ApiName("_hread");
	OutTraceSYS("%s: hfile=%#x buf=%#x bytes=%d\n", ApiRef, hFile, lpBuffer, lBytes);
	res = (*p_hread)(hFile, lpBuffer, lBytes);
	OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	return res;
}

BOOL WINAPI extWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	BOOL res;
	ApiName("WriteFile");
#ifndef DXW_NOTRACES
	if(IsTraceSYS){
		char sBuf[81];
		sBuf[0]=0; // empty string
		if(lpOverlapped) sprintf_s(sBuf, 80, "->(Offset=%#x OffsetHigh=%#x)", lpOverlapped->Offset, lpOverlapped->OffsetHigh);
		OutTrace("%s: hFile=%#x Buffer=%#x BytesToWrite=%d Overlapped=%#x%s\n", ApiRef, hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped, sBuf);
	}
#endif // DXW_NOTRACES

	res = (*pWriteFile)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
#ifndef DXW_NOTRACES
	if(res){
		OutTraceSYS("%s: BytesWritten=%d\n", ApiRef, *lpNumberOfBytesWritten);
		OutHexSYS((LPBYTE)lpBuffer, *lpNumberOfBytesWritten);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
#endif // DXW_NOTRACES
	return res;
}

BOOL WINAPI extFindClose(HANDLE hFindFile)
{
	BOOL res;
	ApiName("FindClose");
	OutTraceSYS("%s: hFindFile=%#x\n", ApiRef, hFindFile);
	if(!pFindClose) pFindClose = FindClose; // ???? 
	res = (*pFindClose)(hFindFile);
	if(res) {
		OutTraceSYS("%s: OK\n", ApiRef);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

UINT WINAPI ext_lwrite(HFILE hFile, LPCCH lpBuffer, UINT uBytes)
{
	UINT res;
	ApiName("_lwrite");
	OutTraceSYS("%s: hfile=%#x buf=%#x bytes=%d\n", ApiRef, hFile, lpBuffer, uBytes);
	res = (*p_lwrite)(hFile, lpBuffer, uBytes);
	OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	return res;
}

UINT WINAPI ext_hwrite(HFILE hFile, LPCCH lpBuffer, UINT uBytes)
{
	UINT res;
	ApiName("_hwrite");
	OutTraceSYS("%s: hfile=%#x buf=%#x bytes=%d\n", ApiRef, hFile, lpBuffer, uBytes);
	res = (*p_hwrite)(hFile, lpBuffer, uBytes);
	OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	return res;
}

LONG WINAPI ext_llseek(HFILE hFile, LONG lOffset, int iOrigin)
{
	LONG res;
	ApiName("_llseek");
	OutTraceSYS("%s: hfile=%#x offset=%#x orig=%d(%s)\n", ApiRef, hFile, lOffset, 
		iOrigin, iOrigin == 0 ? "BEGIN" : (iOrigin == 1 ? "CURRENT" : "END"));
	res = (*p_llseek)(hFile, lOffset, iOrigin);
	OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	return res;
}

HFILE WINAPI ext_lclose(HFILE hFile)
{
	HFILE res;
	ApiName("_lclose");
	OutTraceSYS("%s: hfile=%#x\n", ApiRef, hFile);
	res = (*p_lclose)(hFile);
	OutTraceSYS("%s: ret=%d\n", ApiRef, res);
	return res;
}

DWORD WINAPI extSetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	DWORD ret;
	ApiName("SetFilePointer");
	OutTraceSYS("%s: hFile=%#x DistanceToMove=0x%lx DistanceToMoveHigh=%#x MoveMethod=%#x\n", 
		ApiRef, hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
	ret = (*pSetFilePointer)(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
	if((ret == INVALID_SET_FILE_POINTER) && GetLastError()){
		OutErrorSYS("%s: ERROR ret=INVALID_SET_FILE_POINTER err=%d\n", ApiRef, GetLastError());
	}
	else {
		OutTraceSYS("%s: ret=%d\n", ApiRef, ret);
	}
	if((dxw.dwFlags20 & HANDLECDLOCK) && (lDistanceToMove > 645000000) && (dwMoveMethod == 0)){
		ret = lDistanceToMove;
		CDLockFile = hFile;
	}
	return ret;
}
#endif // DXW_NOTRACES

#ifdef MONITORTHREADS
HANDLE WINAPI extCreateThread(
  LPSECURITY_ATTRIBUTES   lpThreadAttributes,
  SIZE_T                  dwStackSize,
  LPTHREAD_START_ROUTINE  lpStartAddress,
  LPVOID				  lpParameter,
  DWORD                   dwCreationFlags,
  LPDWORD                 lpThreadId)
{
	HANDLE res;
	ApiName("CreateThread");
	DWORD ThreadId;
	OutTrace("%s: curthid=%#x stacksize=%d routine=%#x arg=%#x flags=%#x\n", 
		ApiRef, GetCurrentThreadId(), dwStackSize, lpStartAddress, lpParameter, dwCreationFlags);

	if(!pCreateThread) pCreateThread = CreateThread;
	res = (*pCreateThread)(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, &ThreadId);
	if(lpThreadId) *lpThreadId = ThreadId;

	OutTrace("%s: thid=%#x hdl(res)=%#x\n", ApiRef, ThreadId, res);
	return res;
}
#endif // MONITORTHREADS

DWORD WINAPI extGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
	DWORD ret;
	ApiName("GetFileSize");
	OutTraceSYS("%s: hFile=%#x lpFileSizeHigh=%#x\n", ApiRef, hFile, lpFileSizeHigh);
	ret = (*pGetFileSize)(hFile, lpFileSizeHigh);
	if(lpFileSizeHigh){
		OutTraceSYS("%s: Size=%#x SizeHigh=%#x\n", ApiRef, ret, *lpFileSizeHigh);
	}
	else {
		OutTraceSYS("%s: Size=%#x\n", ApiRef, ret);
	}
	return ret;
}

BOOL WINAPI extGetCPInfo(UINT CodePage, LPCPINFO lpCPInfo)
{
	BOOL res;
	ApiName("GetCPInfo");
	OutTraceSYS("%s: cp=%d -> %d\n", ApiRef, CodePage, dxw.CodePage);
	res = (*pGetCPInfo)(dxw.CodePage, lpCPInfo);
	if(res){
		OutTraceSYS("> MaxCharSize=%d\n", lpCPInfo->MaxCharSize);
		OutTraceSYS("> DefaultChar=%s\n", hexdump(lpCPInfo->DefaultChar, MAX_DEFAULTCHAR));
		OutTraceSYS("> LeadByte=%s\n", hexdump(lpCPInfo->LeadByte, MAX_LEADBYTES));	
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extGetCPInfoExA(UINT CodePage, DWORD dwFlags, LPCPINFOEXA lpCPInfo)
{
	BOOL res;
	ApiName("GetCPInfoExA");
	OutTraceSYS("%s: cp=%d -> %d\n", ApiRef, CodePage, dxw.CodePage);
	res = (*pGetCPInfoExA)(dxw.CodePage, dwFlags, lpCPInfo);
	if(res){
		OutTraceSYS("> MaxCharSize=%d\n", lpCPInfo->MaxCharSize);
		OutTraceSYS("> DefaultChar=%s\n", hexdump(lpCPInfo->DefaultChar, MAX_DEFAULTCHAR));
		OutTraceSYS("> LeadByte=%s\n", hexdump(lpCPInfo->LeadByte, MAX_LEADBYTES));	
		OutTraceSYS("> UnicodeDefaultChar=%#x\n", lpCPInfo->UnicodeDefaultChar);	
		OutTraceSYS("> CodePage=%d\n", lpCPInfo->CodePage);
		OutTraceSYS("> CodePageName=%s\n", lpCPInfo->CodePageName);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

BOOL WINAPI extGetCPInfoExW(UINT CodePage, DWORD dwFlags, LPCPINFOEXW lpCPInfo)
{
	BOOL res;
	ApiName("GetCPInfoExW");
	OutTraceSYS("%s: cp=%d -> %d\n", ApiRef, CodePage, dxw.CodePage);
	res = (*pGetCPInfoExW)(dxw.CodePage, dwFlags, lpCPInfo);
	if(res){
		OutTraceSYS("> MaxCharSize=%d\n", lpCPInfo->MaxCharSize);
		OutTraceSYS("> DefaultChar=%s\n", hexdump(lpCPInfo->DefaultChar, MAX_DEFAULTCHAR));
		OutTraceSYS("> LeadByte=%s\n", hexdump(lpCPInfo->LeadByte, MAX_LEADBYTES));	
		OutTraceSYS("> UnicodeDefaultChar=%#x\n", lpCPInfo->UnicodeDefaultChar);	
		OutTraceSYS("> CodePage=%d\n", lpCPInfo->CodePage);
		OutTraceSYS("> CodePageName=%ls\n", lpCPInfo->CodePageName);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	return res;
}

#ifdef NLSTRACE
LPSTR WINAPI extlstrcpynA(LPSTR lpString1, LPCSTR lpString2, int iMaxLength)
{
	LPSTR ret;
	ApiName("lstrcpynA");
	OutTraceSYS("%s: str=\"%s\" maxlen=%d\n", ApiRef, lpString2, iMaxLength);
	ret = (*plstrcpynA)(lpString1, lpString2, iMaxLength);
	OutTraceSYS("%s: ret=\"%s\"\n", ApiRef, ret);
	return ret;
}

LPSTR WINAPI extlstrcpyA(LPSTR lpString1, LPCSTR lpString2)
{
	LPSTR ret;
	ApiName("lstrcpyA");
	OutTraceSYS("%s: str=\"%s\" \n", ApiRef, lpString2);
	ret = (*plstrcpyA)(lpString1, lpString2);
	OutTraceSYS("%s: ret=\"%s\"\n", ApiRef, ret);
	return ret;
}

int WINAPI extGetLocaleInfoA(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData)
{
	int ret;
	ApiName("GetLocaleInfoA");
	OutTraceSYS("%s: loc=%d type=%#x len=%d\n", ApiRef, Locale, LCType, cchData);
	ret = (*pGetLocaleInfoA)(Locale, LCType, lpLCData, cchData);
	//ret = (*pGetLocaleInfoA)(dxw.Locale, LCType, lpLCData, cchData);
	if(cchData){
		OutTraceSYS("%s: data=\"%s\" ret=%#x\n", ApiRef, lpLCData, ret);
	}
	else{
		OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	return ret;
}

int WINAPI extGetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	int ret;
	ApiName("GetLocaleInfoW");
	OutTraceSYS("%s: loc=%d type=%#x len=%d\n", ApiRef, Locale, LCType, cchData);
	ret = (*pGetLocaleInfoW)(Locale, LCType, lpLCData, cchData);
	//ret = (*pGetLocaleInfoW)(dxw.Locale, LCType, lpLCData, cchData);
	if(cchData){
		OutTraceSYS("%s: data=\"%ls\" ret=%#x\n", ApiRef, lpLCData, ret);
	}
	else{
		OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	return ret;
}

BOOL WINAPI extWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	BOOL ret;
	ApiName("WriteConsoleA");
	OutTraceSYS("%s: hc=%#x buf=(%d)\"%.*s\"\n", ApiRef, nNumberOfCharsToWrite, nNumberOfCharsToWrite, lpBuffer);
	ret = (*pWriteConsoleA)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	if(lpNumberOfCharsWritten) {
		OutTraceSYS("%s: ret=%#x written=%d\n", ApiRef, ret, *lpNumberOfCharsWritten);
	}
	else {
		OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	return TRUE;
}

BOOL WINAPI extWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	BOOL ret;
	ApiName("WriteConsoleW");
	OutTraceSYS("%s: hc=%#x buf=(%d)\"%.*ls\"\n", ApiRef, nNumberOfCharsToWrite, nNumberOfCharsToWrite, lpBuffer);
	ret = (*pWriteConsoleW)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	if(lpNumberOfCharsWritten) {
		OutTraceSYS("%s: ret=%#x written=%d\n", ApiRef, ret, *lpNumberOfCharsWritten);
	}
	else {
		OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	return TRUE;
}
#endif // NLSTRACE

BOOL WINAPI extIsValidCodePage(UINT CodePage)
{
	BOOL ret;
	ApiName("IsValidCodePage");
	ret = (*pIsValidCodePage)(CodePage);
	OutTraceSYS("%s: codepage=%d ret=%d\n", ApiRef, CodePage, ret);
	if(CodePage == dxw.CodePage) ret = TRUE;
	return ret;
}

BOOL WINAPI extIsValidLocale(LCID Locale, DWORD dwFlags)
{
	BOOL ret;
	ApiName("IsValidLocale");
	ret = (*pIsValidLocale)(Locale, dwFlags);
	OutTraceSYS("%s: locale=%d flags=%#x ret=%d\n", ApiRef, Locale, dwFlags, ret);
	if(Locale == dxw.Locale) ret = TRUE;
	return ret;
}

BOOL WINAPI extEnumSystemLocalesA(LOCALE_ENUMPROCA lpLocaleEnumProc, DWORD dwFlags)
{
	ApiName("EnumSystemLocalesA");
	OutTrace("%s: flags=%#x\n", ApiRef, dwFlags);
	// make sure that the selected locale is enumerated
	// loading LCIDToLocaleName dynamically for WinXP compatibility
	WCHAR WName[LOCALE_NAME_MAX_LENGTH+1];
	CHAR Name[(2*LOCALE_NAME_MAX_LENGTH)+1];
	do { // fake loop
		typedef int (WINAPI *LCIDToLocaleName_Type)(LCID, LPWSTR, int, DWORD);
		HMODULE hKernel32 = LoadLibrary("Kernel32.dll");
		if(hKernel32 == NULL) break;
		LCIDToLocaleName_Type pLCIDToLocaleName = (LCIDToLocaleName_Type)GetProcAddress(hKernel32, "LCIDToLocaleName");
		if(pLCIDToLocaleName == NULL) break;
		if((*pLCIDToLocaleName)(dxw.Locale, WName, LOCALE_NAME_MAX_LENGTH, 0)){
			sprintf(Name, "%ls", WName);
			(*lpLocaleEnumProc)(Name);
		}
	} while (FALSE);
	// enumerate all the others
	(*pEnumSystemLocalesA)(lpLocaleEnumProc, dwFlags);
	return TRUE;
}

BOOLEAN WINAPI extCreateSymbolicLinkA(LPCSTR lpSymlinkFileName, LPCSTR lpTargetFileName, DWORD dwFlags)
{
	BOOLEAN ret;
	ApiName("CreateSymbolicLinkA");
	char lpNewLink[MAX_PATH+1];
	char lpNewPath[MAX_PATH+1];
	OutTraceSYS("%s: link=\"%s\" target=\"%s\" flags=%#x\n", ApiRef, lpSymlinkFileName, lpTargetFileName, dwFlags);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpSymlinkFileName = dxwTranslatePathA(lpSymlinkFileName, NULL);
		strcpy(lpNewLink, lpSymlinkFileName);
		lpTargetFileName = dxwTranslatePathA(lpTargetFileName, NULL);
		strcpy(lpNewPath, lpTargetFileName);
		ret = (*pCreateSymbolicLinkA)(lpNewLink, lpNewPath, dwFlags);
	}
	else {
		ret = (*pCreateSymbolicLinkA)(lpSymlinkFileName, lpTargetFileName, dwFlags);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOLEAN WINAPI extCreateSymbolicLinkW(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
	BOOLEAN ret;
	ApiName("CreateSymbolicLinkW");
	WCHAR lpNewLink[MAX_PATH+1];
	WCHAR lpNewPath[MAX_PATH+1];
	OutTraceSYS("%s: link=\"%ls\" target=\"%ls\" flags=%#x\n", ApiRef, lpSymlinkFileName, lpTargetFileName, dwFlags);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		lpSymlinkFileName = dxwTranslatePathW(lpSymlinkFileName, NULL);
		wcscpy(lpNewLink, lpSymlinkFileName);
		lpTargetFileName = dxwTranslatePathW(lpTargetFileName, NULL);
		wcscpy(lpNewPath, lpTargetFileName);
		ret = (*pCreateSymbolicLinkW)(lpNewLink, lpNewPath, dwFlags);
	}
	else {
		ret = (*pCreateSymbolicLinkW)(lpSymlinkFileName, lpTargetFileName, dwFlags);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

#ifdef TRACEALL
int WINAPI extGetDateFormatA(LCID Locale, DWORD dwFlags, const SYSTEMTIME *lpDate, LPCSTR lpFormat, LPSTR lpDateStr, int cchDate)
{
	ApiName("GetDateFormatA");
	int res;
	OutTrace("%s: lcid=%#x flags=%x format=\"%s\" cchdate=%d\n", 
		ApiRef, Locale, dwFlags, lpFormat, cchDate);
	res = (*pGetDateFormatA)(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);
	if(res) {
		if(cchDate){
			OutTrace("%s: res=%d datestr=\"%s\"\n", ApiRef, res, lpDateStr);
		}
		else {
			OutTrace("%s: res=%d\n", ApiRef, res);
		}
	}
	else {
		OutErrorSYS("%s: ERROR res=%d err=%d\n", ApiRef, res, GetLastError());
	}
	return res;
}

int WINAPI extGetDateFormatW(LCID Locale, DWORD dwFlags, const SYSTEMTIME *lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate)
{
	ApiName("GetDateFormatW");
	int res;
	OutTrace("%s: lcid=%#x flags=%x format=\"%ls\" cchdate=%d\n", 
		ApiRef, Locale, dwFlags, lpFormat, cchDate);
	res = (*pGetDateFormatW)(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);
	if(res) {
		if(cchDate){
			OutTrace("%s: res=%d datestr=\"%ls\"\n", ApiRef, res, lpDateStr);
		}
		else {
			OutTrace("%s: res=%d\n", ApiRef, res);
		}
	}
	else {
		OutErrorSYS("%s: ERROR res=%d err=%d\n", ApiRef, res, GetLastError());
	}
	return res;
}
#endif // TRACEALL

DWORD WINAPI extSearchPathA(LPCSTR lpPath, LPCSTR lpFileName, LPCSTR lpExtension, DWORD nBufferLength, LPSTR lpBuffer, LPSTR *lpFilePart)
{
	ApiName("SearchPathA");
	DWORD res;
	OutTraceSYS("%s: path=\"%s\" fname=\"%s\" ext=\"%s\" buflen=%d\n", 
		ApiRef, lpPath, lpFileName, lpExtension, nBufferLength);

	// v2.06.03 fix: lpPath can be NULL
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		if(lpPath){
			lpPath = dxwTranslatePathA(lpPath, &mapping);
			if(mapping != DXW_NO_FAKE) {
				OutTraceDW("%s: remapped path on fake %s\n", ApiRef, mapping == DXW_FAKE_HD ? "HD" : "CD");
			}
		}
		lpFileName = dxwTranslatePathA(lpFileName, &mapping);
		if(mapping != DXW_NO_FAKE) {
			OutTraceDW("%s: remapped %s lpFileName=%s\n", ApiRef, mapping == DXW_FAKE_HD ? "HD" : "CD", lpFileName);
		}
	}

	res = (*pSearchPathA)(lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
	if(res) {
		// v2.05.94: the *lpFilePart is not reliable: even if not NULL it can point to a 
		// bad string that causes axception
		//OutTrace("%s: res=%d buffer=\"%s\" filepart=\"%s\"\n", 
		//	ApiRef, res, lpBuffer ? lpBuffer : "(NULL)", 
		//	lpFilePart ? (*lpFilePart ? *lpFilePart : "(NULL)") : "(NULL)");
		OutTrace("%s: res=%d buffer=\"%s\" \n", 
			ApiRef, res, lpBuffer ? lpBuffer : "(NULL)");
	}
	else {
		OutErrorSYS("%s: ERROR res=%d err=%d\n", ApiRef, res, GetLastError());
	}
	return res;
}

DWORD WINAPI extSearchPathW(LPCWSTR lpPath, LPCWSTR lpFileName, LPCWSTR lpExtension, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart)
{
	ApiName("SearchPathW");
	DWORD res;
	OutTraceSYS("%s: path=\"%ls\" fname=\"%ls\" ext=\"%ls\" buflen=%d\n", 
		ApiRef, lpPath, lpFileName, lpExtension, nBufferLength);

	// v2.06.03 fix: lpPath can be NULL
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		if(lpPath) {
			lpPath = dxwTranslatePathW(lpPath, &mapping);
			if(mapping != DXW_NO_FAKE) {
				OutTraceDW("%s: remapped path on fake %s\n", ApiRef, mapping == DXW_FAKE_HD ? "HD" : "CD");
			}
		}
		lpFileName = dxwTranslatePathW(lpFileName, &mapping); // v2.06.11 - fix for "Barbie Fashion Designer" through winevdm
		if(mapping != DXW_NO_FAKE) {
			OutTraceDW("%s: remapped %s lpFileName=%ls\n", ApiRef, mapping == DXW_FAKE_HD ? "HD" : "CD", lpFileName);
		}
	}

	res = (*pSearchPathW)(lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
	if(res) {
		OutTrace("%s: res=%d buffer=\"%ls\" filepart=\"%ls\"\n", 
			ApiRef, res, lpBuffer, lpFilePart ? *lpFilePart : L"(NULL)");
	}
	else {
		OutErrorSYS("%s: ERROR res=%d err=%d\n", ApiRef, res, GetLastError());
	}
	return res;
}

#ifdef TRACERESOURCES
HGLOBAL WINAPI extLoadResource(HMODULE hModule, HRSRC hResInfo)
{
	ApiName("LoadResource");
	HGLOBAL res;
	OutTraceSYS("%s: hmod=%#x hResInfo=%#x\n", ApiRef, hModule, hResInfo);
	res = (*pLoadResource)(hModule, hResInfo);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

LPVOID WINAPI extLockResource(HGLOBAL hResData)
{
	ApiName("LockResource");
	LPVOID res;
	OutTraceSYS("%s: hResData=%#x\n", ApiRef, hResData);
	res = (*pLockResource)(hResData);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif // TRACERESOURCES

DWORD PortableGetThreadId(HANDLE hThread)
{
	static DWORD (WINAPI *pGetThreadId)(HANDLE) = NULL;
	if(pGetThreadId == NULL){
		HINSTANCE hinst;
		hinst=(*pLoadLibraryA)("kernel32.dll");
		if(!hinst) return 0;
		pGetThreadId = (DWORD (WINAPI *)(HANDLE))(*pGetProcAddress)(hinst, "GetThreadId");
		if(pGetThreadId == NULL) {
			OutErrorSYS("PortableGetThreadId: ERROR err=%d\n", GetLastError());
			return 0;
		}
	}
	return (*pGetThreadId)(hThread);
}

DWORD WINAPI extSuspendThread(HANDLE hThread)
{
	ApiName("SuspendThread");

	// v2.06.04: to cope with Mechwarrior 2 bugs it is better to avoid writing logs
	// so the OutTraceSYS statement is now in the "else" case of IGNORESCHEDULER

	if(dxw.dwFlags15 & IGNORESCHEDULER){
		// if we're trying to suspend our own thread, refuse
		if (PortableGetThreadId(hThread) == GetCurrentThreadId()) {
			OutErrorSYS("%s: BYPASS suspend on self hThread=%#x\n", ApiRef, hThread);
			return 0;
		}
	}
	else {
		OutTraceSYS("%s: hThread=%#x\n", ApiRef, hThread);
	}

	return (*pSuspendThread)(hThread);
}

DWORD WINAPI extResumeThread(HANDLE hThread)
{
	ApiName("ResumeThread");

	// v2.06.04: to cope with Mechwarrior 2 bugs it is better to avoid writing logs
	// so the OutTraceSYS statement is now in the "else" case of IGNORESCHEDULER

	if(dxw.dwFlags15 & IGNORESCHEDULER){
		// if we're trying to resume our own thread, refuse
		if (PortableGetThreadId(hThread) == GetCurrentThreadId()) {
			OutErrorSYS("%s: BYPASS resume on self hThread=%#x\n", ApiRef, hThread);
			return 0;
		}
	}
	else {
		OutTraceSYS("%s: hThread=%#x\n", ApiRef, hThread);
	}

	return (*pResumeThread)(hThread);
}

BOOL WINAPI extDuplicateHandle(
    HANDLE hSourceProcessHandle,  // handle to source process
    HANDLE hSourceHandle,         // handle to duplicate
    HANDLE hTargetProcessHandle,  // handle to target process
    LPHANDLE lpTargetHandle,      // duplicate handle
    DWORD dwDesiredAccess,        // requested access
    BOOL bInheritHandle,          // handle inheritance option
    DWORD dwOptions               // optional actions
    )
{
	ApiName("DuplicateHandle");
	BOOL ret; 

    // Save the original value
	// v2.05.83 fix. Mars3D calls DuplicateHandle(0). The origHandle assignment must be protected.
	HANDLE origHandle = lpTargetHandle ? *lpTargetHandle : 0;

    ret = (*pDuplicateHandle)(hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, lpTargetHandle, dwDesiredAccess, bInheritHandle, dwOptions);

    if (!ret && (dxw.dwFlags15 & DUPLICATEHANDLEFIX))  {
        DWORD dwLastError = GetLastError();
		OutErrorSYS("%s: ERROR err=%d - reverting *lpTargetHandle to previous value\n", ApiRef, GetLastError());
        *lpTargetHandle = origHandle;
    }

    return ret;
}

LPSTR WINAPI extGetCommandLineA()
{
	ApiName("GetCommandLineA");
	LPSTR ret = (*pGetCommandLineA)();
	static char *lpCommandLine = NULL;
	if(dxw.dwFlags16 & EMUGETCOMMANDLINE){
		if(lpCommandLine == NULL){
			lpCommandLine = (char *)malloc(strlen(ret)+1+2); // 2 extra, just in case ...
			char arg0[MAX_PATH];
			char shortName[MAX_PATH];
			char *tail;
			if(ret[0]=='"'){
				char *quote = strchr(&ret[1], '"');
				int len = (quote - ret) - 1;
				strncpy(arg0, &ret[1], len);
				arg0[len]=0;
				tail = &ret[len + 2];
			}
			else {
				char *space = strchr(ret, ' ');
				int len = (space - ret);
				strncpy(arg0, ret, len);
				arg0[len]=0;
				tail = &ret[len];
			}
			int len2 = GetShortPathNameA(arg0, shortName, MAX_PATH);
			if(!len2) {
				OutTrace("%s: GetShortPathNameA ERROR err=%d ret=\"%s\"\n", ApiRef, GetLastError(), ret);
				return ret; // in case of error, return the original value!
			}
			OutTrace("%s: SHORTED \"%s\"->\"%s\"\n", ApiRef, arg0, shortName);
			strcpy(lpCommandLine, shortName);
			strcat(lpCommandLine, tail);
		}
		ret = lpCommandLine;
	}
	OutTrace("%s: ret=\"%s\"\n", ApiRef, ret);
	return ret;
}

DWORD WINAPI extGetModuleFileNameA(
	HMODULE hModule,      // handle to module
    LPSTR   lpFilename,   // file name of module
    DWORD   nSize         // size of buffer
    )
{
	DWORD ret;
	ApiName("GetModuleFileNameA");
	OutTraceSYS("%s: hmod=%#x nSize=%d\n", ApiRef, hModule, nSize);
	ret = (*pGetModuleFileNameA)(hModule, lpFilename, nSize);
	if(dxw.dwFlags16 & EMUGETCOMMANDLINE){
		char fileName[MAX_PATH];
		char shortName[MAX_PATH];
		ret = (*pGetModuleFileNameA)(hModule, fileName, MAX_PATH);
		if (strchr(fileName, ' ') != NULL){
			int len = GetShortPathNameA(fileName, shortName, MAX_PATH);
			OutTraceSYS("%s: SHORTED \"%s\"->\"%s\"\n", ApiRef, fileName, shortName);
			strncpy(lpFilename, shortName, nSize);
			if(strlen(shortName)>=nSize) lpFilename[nSize-1]=0;
			ret = strlen(lpFilename);
		}
	}

	// Sierra games use the module filename to identify the corresponding configuration
	// file <modulename>.ini. The ".noshim" postfix should be trimmed out.
	// v2.06.12: the ".noshim" suffix is no longer used
	//if(dxw.dwFlags7 & CLEARSHIMS){
	//	char *p = strstr(lpFilename, ".noshim");
	//	if(p) *p=0;
	//}

	OutTraceSYS("%s: ret(len)=%d fname=\"%s\"\n", ApiRef, ret, lpFilename);
	return ret;
}

BOOL WINAPI extCopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists)
{
	BOOL ret;
	ApiName("CopyFileA");
	OutTraceSYS("%s: existingPath=\"%s\" newPaath=\"%s\" failIfExists=%d\n", 
		ApiRef, lpExistingFileName, lpNewFileName, bFailIfExists);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		lpExistingFileName = dxwTranslatePathA(lpExistingFileName, &mapping);
		if(mapping != DXW_NO_FAKE) {
			OutTraceDW("%s: remapped existing path on fake %s\n", ApiRef, mapping == DXW_FAKE_HD ? "HD" : "CD");
		}
		lpNewFileName = dxwTranslatePathA(lpNewFileName, &mapping);
		if(mapping == DXW_FAKE_HD) {
			OutTraceDW("%s: remapped copy path on fake HD path=\"%s\"\n", ApiRef, lpNewFileName);
		}
		else if(mapping == DXW_FAKE_CD) {
			OutTraceDW("%s: remapped copy path on fake CD path=\"%s\" - simulate error\n", ApiRef, lpNewFileName);
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
	}

	ret = (*pCopyFileA)(lpExistingFileName, lpNewFileName, bFailIfExists);
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

BOOL WINAPI extCopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
	BOOL ret;
	ApiName("CopyFileW");
	OutTraceSYS("%s: existingPath=\"%ls\" newPaath=\"%ls\" failIfExists=%d\n", 
		ApiRef, lpExistingFileName, lpNewFileName, bFailIfExists);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		lpExistingFileName = dxwTranslatePathW(lpExistingFileName, &mapping);
		if(mapping != DXW_NO_FAKE) {
			OutTraceDW("%s: remapped existing path on fake %s\n", ApiRef, mapping == DXW_FAKE_HD ? "HD" : "CD");
		}
		lpNewFileName = dxwTranslatePathW(lpNewFileName, &mapping);
		if(mapping == DXW_FAKE_HD) {
			OutTraceDW("%s: remapped copy path on fake HD path=\"%ls\"\n", ApiRef, lpNewFileName);
		}
		else if(mapping == DXW_FAKE_CD) {
			OutTraceDW("%s: remapped copy path on fake CD path=\"%ls\" - simulate error\n", ApiRef, lpNewFileName);
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
	}

	ret = (*pCopyFileW)(lpExistingFileName, lpNewFileName, bFailIfExists);
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

#ifdef TLSDUMP
DWORD WINAPI extTlsAlloc()
{
	DWORD ret;
	ApiName("TlsAlloc");
	OutTrace("%s\n", ApiRef);
	ret = (*pTlsAlloc)();
	if(ret == TLS_OUT_OF_INDEXES) {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	else {
		OutErrorSYS("%s: tls=%#x\n", ApiRef, ret);
	}
	return ret;
}

BOOL WINAPI extTlsFree(DWORD dwTlsIndex)
{
	BOOL ret;
	ApiName("TlsFree");
	OutTrace("%s: tlsindex=%#x\n", ApiRef, dwTlsIndex);
	ret = (*pTlsFree)(dwTlsIndex);
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

LPVOID WINAPI extTlsGetValue(DWORD dwTlsIndex)
{
	LPVOID ret;
	ApiName("TlsGetValue");
	OutTrace("%s: tlsindex=%#x\n", ApiRef, dwTlsIndex);
	SetLastError(0);
	ret = (*pTlsGetValue)(dwTlsIndex);
	if((ret == 0) && (GetLastError != ERROR_SUCCESS)){
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	else {
		OutErrorSYS("%s: tlsvalue=%#x -> %#x\n", 
			ApiRef, ret, ret ? *(DWORD *)ret : 0);
	}
	return ret;
}

BOOL WINAPI extTlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
	BOOL ret;
	ApiName("TlsSetValue");
	OutTrace("%s: tlsindex=%#x value=%#x -> %#x\n", 
		ApiRef, dwTlsIndex, lpTlsValue, lpTlsValue ? *(DWORD *)lpTlsValue : 0);
	ret = (*pTlsSetValue)(dwTlsIndex, lpTlsValue);
	if(!ret) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}
#endif

BOOL WINAPI extGetOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait)
{
	BOOL ret;
	ApiName("GetOverlappedResult");
	OutTraceSYS("%s: hfile=%#x overlapped={offset=%#x high=%#x event=%#x} wait=%#x\n", 
		ApiRef, hFile, lpOverlapped->Offset, lpOverlapped->OffsetHigh, lpOverlapped->hEvent, bWait);
	if(dxw.dwFlags17 & FIXOVERLAPPEDRESULT) bWait = TRUE;
	ret = (*pGetOverlappedResult)(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	if(ret) {
		OutTraceSYS("%s: bytesread=%d}\n", ApiRef, *lpNumberOfBytesTransferred);
	}
	else {
		OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	}
	if((dxw.dwFlags17 & FIXOVERLAPPEDRESULT) && (GetLastError() == ERROR_HANDLE_EOF)) {
		OutTraceDW("%s: FIXOVERLAPPEDRESULT ret=TRUE\n", ApiRef);
		*lpNumberOfBytesTransferred = 0;
		ret = TRUE;
	}
	return ret;
}

#ifdef TRACEALL
DWORD WINAPI extGetFileType(HANDLE hdl)
{
	DWORD res;
	ApiName("GetFileType");
	OutTraceSYS("%s: hdl=%#x\n", ApiRef, hdl);
	res = (*pGetFileType)(hdl);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

HMODULE WINAPI extGetModuleHandleA(LPCSTR lpPath)
{
	HMODULE res;
	ApiName("GetModuleHandleA");
	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, lpPath);
	res = (*pGetModuleHandleA)(lpPath);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif

BOOL WINAPI extReleaseMutex(HANDLE hMutex)
{
	ApiName("ReleaseMutex");
	OutDebugSYS("%s: hMutex=%#x\n", ApiRef, hMutex);

	BOOL bRet = FALSE;

	if(dxw.dwFlags18 & EMUDDSYNCSHIM){
		if (g_hThread && (g_dwFindMutexThread == GetCurrentThreadId())) {
			//
			// We're using our hack to find the DirectDraw mutex
			// 
			OutTraceDW("%s: EMUDDSYNCSHIM DDraw exclusive mode mutex found hMutex=%#x\n", ApiRef, hMutex);
			g_hDDMutex = hMutex;

			// Don't release it, since we never acquired it
			return TRUE;
		}

		//
		// First try to release it on the current thread. This will only succeed if 
		// it was obtained on this thread.
		//

		bRet = (*pReleaseMutex)(hMutex);

		if (!bRet && g_hThread && g_hDDMutex && (hMutex == g_hDDMutex)) {
			//
			// Use our thread to release the mutex. We synchronize since we're
			// accessing globals to communicate with our thread.
			//
			EnterCriticalSection(&g_csSync);
	    
			// Set globals to communicate with our thread
			g_dwWait = sReleaseMutex;

			ResetEvent(g_hDoneEvent);

			// Wait until our thread returns
			SetEvent(g_hWaitEvent);

			// Signal our thread to release the mutex
			(*pWaitForSingleObject)(g_hDoneEvent, INFINITE);

			// Detect degenerate case
			if (GetCurrentThreadId() != g_dwMutexOwnerThreadId) {
				OutTraceDW("%s: EMUDDSYNCSHIM DirectDraw synchronization error - correcting\n", ApiRef);
			}

			if (g_bRetValue) g_dwMutexOwnerThreadId = 0;
			bRet = g_bRetValue;			LeaveCriticalSection(&g_csSync);
		}
	}
	else {
		// pass through
		bRet = (*pReleaseMutex)(hMutex);
	}

    return bRet;
}

UINT WINAPI extSetErrorMode(UINT mode)
{
	ApiName("SetErrorMode");
	UINT ret;

	OutTraceSYS("%s: mode=%#x\n", ApiRef, mode);
	if((mode & (SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS)) == SEM_NOOPENFILEERRORBOX){
		mode |= SEM_FAILCRITICALERRORS;
		OutTraceDW("%s: fixed mode=%#x\n", ApiRef, mode);
	}
	ret = (*pSetErrorMode)(mode);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

#ifdef TRACEALL
UINT WINAPI extSetHandleCount(UINT uNumber)
{
	ApiName("SetHandleCount");
	UINT ret;

	ret = (*pSetHandleCount)(uNumber);

	OutTrace("%s: number=%d ret=%d\n", ApiRef, uNumber, ret);
	return ret;
}
#endif