#define  _CRT_SECURE_NO_WARNINGS
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "stdio.h"
#include "detours.h"

extern void clear_shim(HANDLE);

typedef DWORD (WINAPI *SuspendThread_Type)(HANDLE);
typedef DWORD (WINAPI *ResumeThread_Type)(HANDLE);
extern SuspendThread_Type pSuspendThread;
extern ResumeThread_Type pResumeThread;

#ifndef DXW_NOTRACES
static char *ExplainDebugEvent(DWORD ec)
{
	char *e;
	switch(ec){
		case EXCEPTION_DEBUG_EVENT: e="EXCEPTION"; break;
		case CREATE_THREAD_DEBUG_EVENT: e="CREATE_THREAD"; break;
		case CREATE_PROCESS_DEBUG_EVENT: e="CREATE_PROCESS"; break;
		case EXIT_THREAD_DEBUG_EVENT: e="EXIT_THREAD"; break;
		case EXIT_PROCESS_DEBUG_EVENT: e="EXIT_PROCESS"; break;
		case LOAD_DLL_DEBUG_EVENT: e="LOAD_DLL"; break;
		case UNLOAD_DLL_DEBUG_EVENT: e="UNLOAD_DLL"; break;
		case OUTPUT_DEBUG_STRING_EVENT: e="OUTPUT_DEBUG"; break;
		case RIP_EVENT: e="RIP"; break;
		default: e="unknown"; break;
	}
	return e;
}
#endif // DXW_NOTRACES

static BOOL DebugProc(ApiArg, LPPROCESS_INFORMATION lpProcessInformation)
{
	extern BOOL Inject(DWORD, const char *);
	DEBUG_EVENT debug_event ={0};
	char path[MAX_PATH];
	BOOL bContinueDebugging = TRUE;
	LPVOID LastExceptionPtr = 0;
#define LOCKINJECTIONTHREADS
#ifdef LOCKINJECTIONTHREADS
	DWORD StartingCode;
	LPVOID StartAddress = 0;
	extern LPVOID GetThreadStartAddress(HANDLE);
#endif
	while(bContinueDebugging)
	{ 
		if (!WaitForDebugEvent(&debug_event, INFINITE)) {
			OutErrorSYS("%s: WaitForDebugEvent error=%d\n", GetLastError());
			break;
		}
		OutDebugDW("%s: WaitForDebugEvent pid=%#x tid=%#x event=%#x(%s)\n", 
			ApiRef, debug_event.dwProcessId, debug_event.dwThreadId, debug_event.dwDebugEventCode, ExplainDebugEvent(debug_event.dwDebugEventCode));
		switch(debug_event.dwDebugEventCode){
			case EXIT_PROCESS_DEBUG_EVENT:
				bContinueDebugging=false;
				break;
			case CREATE_PROCESS_DEBUG_EVENT:
				GetModuleFileName(GetModuleHandle("dxwnd"), path, MAX_PATH);
				OutTraceDW("%s: injecting path=%s\n", ApiRef, path);
				StartAddress = debug_event.u.CreateProcessInfo.lpStartAddress;
				OutTraceDW("%s: start address=%#x\n", ApiRef, StartAddress);
				if(!Inject(lpProcessInformation->dwProcessId, path)){
					OutErrorSYS("%s: Injection ERROR pid=%#x dll=%s\n", ApiRef, lpProcessInformation->dwProcessId, path);
				}
#ifdef LOCKINJECTIONTHREADS
					HANDLE TargetHandle;
					extern LPVOID GetThreadStartAddress(HANDLE);
					DWORD EndlessLoop;
					EndlessLoop=0x9090FEEB; // assembly for JMP to here, NOP, NOP
					SIZE_T BytesCount;
					TargetHandle = OpenProcess(
						PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, 
						FALSE, 
						lpProcessInformation->dwProcessId);
					if(TargetHandle){
						StartAddress = GetThreadStartAddress(lpProcessInformation->hThread);
						//StartAddress = StartAddress;
						OutTraceSYS("%s: StartAddress=%#x\n", ApiRef, StartAddress);
						if(StartAddress){
							if(!ReadProcessMemory(lpProcessInformation->hProcess, StartAddress, &StartingCode, 4, &BytesCount)){ 
								OutTraceSYS("%s: ReadProcessMemory error=%d\n", ApiRef, GetLastError());
							}
							OutTrace("CreateProcess: StartCode=%#x\n", StartingCode);
							if(!WriteProcessMemory(lpProcessInformation->hProcess, StartAddress, &EndlessLoop, 4, &BytesCount)){
								OutTraceSYS("%s: WriteProcessMemory error=%d\n", ApiRef, GetLastError());
							}
						}
					}
#endif				
				OutTraceDW("%s: injection started\n", ApiRef);
				CloseHandle(debug_event.u.CreateProcessInfo.hFile);
				break;
			case EXCEPTION_DEBUG_EVENT:
				{
					LPEXCEPTION_DEBUG_INFO ei;
					ei=(LPEXCEPTION_DEBUG_INFO)&debug_event.u;
					OutErrorSYS("%s: EXCEPTION code=%#x flags=%#x addr=%#x firstchance=%#x\n", 
						ApiRef,
						ei->ExceptionRecord.ExceptionCode, 
						ei->ExceptionRecord.ExceptionFlags, 
						ei->ExceptionRecord.ExceptionAddress,
						debug_event.u.Exception.dwFirstChance);
					// exception twice in same address, then do not continue.
					if(LastExceptionPtr == ei->ExceptionRecord.ExceptionAddress) bContinueDebugging = FALSE;
					//if(ei->dwFirstChance == 0) bContinueDebugging = FALSE;
					LastExceptionPtr = ei->ExceptionRecord.ExceptionAddress;
					switch(ei->ExceptionRecord.ExceptionCode){
						case 0x80000003: // INT3 debugging exception, ignore it
						case 0xc0000008: // invalid handle - Better ignore. Ref. "Dungeon Keeper, Deeper Dungeons D3D" 
						case 0xe06d7363: // v2.05.05: from MSDN: The Visual C++ compiler uses exception code 0xE06D7363 for C++ exceptions. 
							bContinueDebugging=true;
							break;
						default:
							bContinueDebugging=false;
							break;
					}
				}
				break;
			case LOAD_DLL_DEBUG_EVENT:
				//OutTrace("CreateProcess: event=%#x(%s) dll=%s address=%#x\n", 
				//	debug_event.dwDebugEventCode, ExplainDebugEvent(debug_event.dwDebugEventCode),
				//	((LOAD_DLL_DEBUG_INFO *)&debug_event.u)->lpImageName, ((LOAD_DLL_DEBUG_INFO *)&debug_event.u)->lpBaseOfDll);
				CloseHandle(debug_event.u.LoadDll.hFile);
				break;
			case CREATE_THREAD_DEBUG_EVENT:
				OutDebugDW("%s: THREAD %#x\n", ApiRef, debug_event.u.CreateThread.hThread);
				break;
			case EXIT_THREAD_DEBUG_EVENT:
#ifdef LOCKINJECTIONTHREADS
				if(TargetHandle && StartAddress){
					if(dxw.dwDFlags & FREEZEINJECTEDSON){
						OutTraceDW("%s: FREEZEINJECTEDSON leaving son process in endless loop\n", ApiRef, GetLastError());
					}
					else if(!WriteProcessMemory(lpProcessInformation->hProcess, StartAddress, &StartingCode, 4, &BytesCount)){
							OutErrorSYS("%s: WriteProcessMemory error=%d\n", ApiRef, GetLastError());
					}
					CloseHandle((HANDLE)TargetHandle);
					OutTraceDW("%s: injection terminated\n", ApiRef);
				}
#endif
				OutDebugDW("%s: thread exit code=%#x\n", ApiRef, debug_event.u.ExitThread.dwExitCode);
				bContinueDebugging=false;
			default:
				break;
		}
		if(bContinueDebugging){
			ContinueDebugEvent(debug_event.dwProcessId, 
				debug_event.dwThreadId, 
				DBG_CONTINUE);
		}
		else{
			ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, DBG_CONTINUE); 
			if(!DebugSetProcessKillOnExit(FALSE)){
				OutErrorSYS("%s: DebugSetProcessKillOnExit ERROR err=%d\n", ApiRef, GetLastError());
			}
			if(!DebugActiveProcessStop(debug_event.dwProcessId)){
				OutErrorSYS("%s: DebugActiveProcessStop ERROR err=%d\n", ApiRef, GetLastError());
				MessageBox(NULL, "Error in DebugActiveProcessStop", "dxwnd", MB_OK);
			}
		}
	}
	OutTraceDW("%s: detached\n", ApiRef);
	return TRUE;
}

static BOOL CreateProcessDebugA(
	LPCTSTR lpApplicationName, 
	LPTSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	ApiName("CreateProcessA");
	BOOL res;

	OutTraceDW("%s: dwCreationFlags = %#x\n", ApiRef, dwCreationFlags);
	dwCreationFlags |= DEBUG_ONLY_THIS_PROCESS;
	dwCreationFlags &= ~CREATE_SUSPENDED;
	if(dxw.dwFlags7 & CLEARSHIMS) dwCreationFlags |= CREATE_SUSPENDED;
	res=(*pCreateProcessA)(
		lpApplicationName, lpCommandLine,
		lpProcessAttributes, lpThreadAttributes, bInheritHandles,
		dwCreationFlags, lpEnvironment,
		lpCurrentDirectory, lpStartupInfo, lpProcessInformation
	);
	if(dxw.dwFlags7 & CLEARSHIMS) {
		OutTrace("clearing shims on PEB hp=%#x\n", lpProcessInformation->hProcess);
		clear_shim(lpProcessInformation->hProcess);
		ResumeThread(lpProcessInformation->hThread);
	}
	OutTraceSYS("%s res=%#x\n", ApiRef, res);
	if(res) DebugProc(ApiRef, lpProcessInformation);
	return res;
}

static BOOL CreateProcessDebugW(
	LPCWSTR lpApplicationName, 
	LPWSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	ApiName("CreateProcessW");
	BOOL res;

	OutTraceDW("%s: dwCreationFlags = %#x\n", ApiRef, dwCreationFlags);
	dwCreationFlags |= DEBUG_ONLY_THIS_PROCESS;
	dwCreationFlags &= ~CREATE_SUSPENDED;
	if(dxw.dwFlags7 & CLEARSHIMS) dwCreationFlags |= CREATE_SUSPENDED;
	res=(*pCreateProcessW)(
		lpApplicationName, lpCommandLine,
		lpProcessAttributes, lpThreadAttributes, bInheritHandles,
		dwCreationFlags, lpEnvironment,
		lpCurrentDirectory, lpStartupInfo, lpProcessInformation
	);
	if(dxw.dwFlags7 & CLEARSHIMS) {
		OutTrace("clearing shims on PEB hp=%#x\n", lpProcessInformation->hProcess);
		clear_shim(lpProcessInformation->hProcess);
		ResumeThread(lpProcessInformation->hThread);
	}
	OutTraceSYS("%s res=%#x\n", ApiRef, res);
	if(res) DebugProc(ApiRef, lpProcessInformation);
	return res;
}

static char *OpenExeFromArgumentsA(LPCSTR lpApplicationName, LPSTR lpCommandLine)
{
	// v2.05.59 fix: add ".exe" extension if omitted
	char *pTry = NULL;
	char *sExe = NULL;
	FILE *fExe = NULL;
	char *argument;

	argument = (LPSTR)lpApplicationName; 
	if(argument == NULL) {
		argument = lpCommandLine; 
	} else {
		if(strlen(lpApplicationName) == 0) {
			argument = lpCommandLine;
		}
	}

	sExe = (LPTSTR)malloc(strlen(argument) + 5); // add space for an extension .exe and a string terminator
	strcpy(sExe, argument);
	// make it lowercase
	//for(char *p=sExe; *p; p++) *p = tolower(*p);
	_strlwr(sExe);
	if(sExe[0]=='"') {
		// if quoted, the exe path is surrounded by quotes
		char *path=&sExe[1];
		strtok(path, "\"");
		pTry = (char *)malloc(strlen(path) + 5);
		strcpy(pTry, path);
		//fExe = fopen(pTry, "rb");
		//if(fExe == NULL){
		//	strcat(pTry, ".exe");
		//	FILE *fExe = fopen(pTry, "rb");
		//	OutDebugDW(">> exepath=\"%s\" success=%x\n", pTry, fExe ? 1 : 0);
		//}
		//free(pTry);
		//*ppPath = pTry;
	}
	else {
		// if not quoted, the exe path is blank separated, but the path could hold blanks, so you must search ...
		char *pBlank = sExe;
		while(pBlank) {
			pBlank = strchr(pBlank, ' ');
			if(pBlank) *pBlank = 0;
			pTry = (char *)malloc(strlen(sExe) + 5);
			strcpy(pTry, sExe);
			if(pBlank) *pBlank = ' ';
			pBlank++;
			fExe = fopen(pTry, "rb");
			if(fExe == NULL){
				strcat(pTry, ".exe");
				fExe = fopen(pTry, "rb"); // v2.05.72 fix
			}
			OutDebugDW(">> exepath=\"%s\" success=%x\n", pTry, fExe ? 1 : 0);
			if(fExe) break;
			free(pTry);
			pTry = NULL;
		}
		//*ppPath = pTry;
	}
	if(sExe) free(sExe);
	if(fExe) fclose(fExe);
	return pTry;
}

static WCHAR *OpenExeFromArgumentsW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine)
{
	// v2.05.59 fix: add ".exe" extension if omitted
	WCHAR *pTry = NULL;
	WCHAR *sExe = NULL;
	FILE *fExe = NULL;
	WCHAR *argument;

	argument = (LPWSTR)lpApplicationName; 
	if(argument == NULL) {
		argument = lpCommandLine; 
	} else {
		if(wcslen(lpApplicationName) == 0) {
			argument = lpCommandLine;
		}
	}

	sExe = (LPWSTR)malloc((wcslen(argument) + 5) * sizeof(WCHAR)); // add space for an extension .exe and a string terminator
	wcscpy(sExe, argument);
	// make it lowercase
	_wcslwr(sExe);
	if(sExe[0]==L'"') {
		// if quoted, the exe path is surrounded by quotes
		WCHAR *path=&sExe[1];
		wcstok(path, L"\"");
		pTry = (WCHAR *)malloc((wcslen(path) + 5) * sizeof(WCHAR));
		wcscpy(pTry, path);
		//fExe = _wfopen(pTry, L"rb");
		//if(fExe == NULL){
		//	wcscat(pTry, L".exe");
		//	FILE *fExe = _wfopen(pTry, L"rb");
		//	OutDebugDW(">> exepath=\"%ls\" success=%x\n", pTry, fExe ? 1 : 0);
		//}
		//free(pTry);
		//*ppPath = pTry;
	}
	else {
		// if not quoted, the exe path is blank separated, but the path could hold blanks, so you must search ...
		WCHAR *pBlank = sExe;
		while(pBlank) {
			pBlank = wcschr(pBlank, L' ');
			if(pBlank) *pBlank = 0;
			pTry = (WCHAR *)malloc((wcslen(sExe) + 5) * sizeof(WCHAR));
			wcscpy(pTry, sExe);
			if(pBlank) *pBlank = L' ';
			pBlank++;
			fExe = _wfopen(pTry, L"rb");
			if(fExe == NULL){
				wcscat(pTry, L".exe");
				fExe = _wfopen(pTry, L"rb"); // v2.05.72 fix
			}
			OutDebugDW(">> exepath=\"%ls\" success=%x\n", pTry, fExe ? 1 : 0);
			if(fExe) break;
			free(pTry);
			pTry = NULL;
		}
		//*ppPath = pTry;
	}
	if(fExe) fclose(fExe);
	if(sExe) free(sExe);
	return pTry;
}

static BOOL InjectSon(ApiArg, FILE *fExe, LPPROCESS_INFORMATION lpProcessInformation, HANDLE TargetHandle)
{
	extern BOOL Inject(DWORD, const char *);
	extern LPVOID GetThreadStartAddress(HANDLE);
	DWORD PEHeader[0x70];
	LPVOID StartAddress;
	//HANDLE TargetHandle;
	DWORD OldProt;
	char StartingCode[4];
	DWORD EndlessLoop;
	EndlessLoop=0x9090FEEB; // careful: it's BIG ENDIAN: EB FE 90 90
	DWORD BytesCount;
	char dllpath[MAX_PATH + 1];

	// read DOS header
	if(fread((void *)PEHeader, sizeof(DWORD), 0x10, fExe)!=0x10){
		OutErrorSYS("%s: fread DOSHDR error=%d\n", ApiRef, GetLastError());
		return 0;
	}
	OutDebugDW("%s: NT Header offset=%X\n", ApiRef, PEHeader[0xF]);
	fseek(fExe, PEHeader[0xF], 0);
	// read File header + Optional header
	if(fread((void *)PEHeader, sizeof(DWORD), 0x70, fExe)!=0x70){
		OutErrorSYS("%s: fread NTHDR error=%d\n", ApiRef, GetLastError());
		return 0;
	}

	// You can't just fetch the start address from the exe because it might be relocated 
	// due to ASLR (Address Space Layout Randomization) www.wikipedia.org/wiki/ASLR
	// GetThreadStartAddress fixes Injection run for bstone (windows port of "Blak Stone").
	StartAddress = GetThreadStartAddress(lpProcessInformation->hThread);
	OutDebugDW("%s: AddressOfEntryPoint=%#X\n", ApiRef, StartAddress);
	// patch the entry point with infinite loop
	if(!VirtualProtectEx(TargetHandle, StartAddress, 4, PAGE_EXECUTE_READWRITE, &OldProt )){
		OutErrorSYS("%s: VirtualProtectEx error=%d\n", ApiRef, GetLastError());
		return 0;
	}

	if(!ReadProcessMemory(TargetHandle, StartAddress, &StartingCode, 4, &BytesCount)){ 
		OutErrorSYS("%s: ReadProcessMemory error=%d\n", ApiRef, GetLastError());
		return 0;
	}

	if(!WriteProcessMemory(TargetHandle, StartAddress, &EndlessLoop, 4, &BytesCount)){
		OutErrorSYS("%s: WriteProcessMemory error=%d\n", ApiRef, GetLastError());
		return 0;
	}

	if(!FlushInstructionCache(TargetHandle, StartAddress, 4)){
		OutErrorSYS("%s: FlushInstructionCache ERROR err=%#x\n", ApiRef, GetLastError());
		return 0; // error condition
	}

	// resume the main thread
	if((*pResumeThread)(lpProcessInformation->hThread)==(DWORD)-1){
		OutErrorSYS("%s: ResumeThread error=%d at:%d\n", ApiRef, GetLastError(), __LINE__);
		return 0;
	}

	// wait until the thread stuck at entry point
	CONTEXT context;
	context.Eip = (DWORD)0; // initialize to impossible value
	for ( unsigned int i = 0; i < 80 && context.Eip != (DWORD)StartAddress; ++i ){
		// patience.
		Sleep(500);

		// read the thread context
		context.ContextFlags = CONTEXT_CONTROL;
		if(!GetThreadContext(lpProcessInformation->hThread, &context)){
			OutErrorSYS("%s: GetThreadContext error=%d\n", ApiRef, GetLastError());
			break;
		}
		OutDebugDW("wait cycle %d eip=%#x\n", i, context.Eip);
	}

	if (context.Eip != (DWORD)StartAddress){
		// wait timed out
		OutErrorSYS("%s: thread blocked eip=%#x addr=%#x\n", ApiRef, context.Eip, StartAddress);
		return 0;
	}

	// inject DLL payload into remote process
	// GetFullPathName("dxwnd.dll", MAX_PATH, dllpath, NULL);
	GetModuleFileNameA(GetModuleHandleA("dxwnd.dll"), dllpath, MAX_PATH);
	if(!Inject(lpProcessInformation->dwProcessId, dllpath)){
		// DXW_STRING_INJECTION
		OutErrorSYS("%s: Injection error: pid=%#x dll=%s\n", ApiRef, lpProcessInformation->dwProcessId, dllpath);
		return 0;
	}

	// pause 
	if((*pSuspendThread)(lpProcessInformation->hThread)==(DWORD)-1){
		OutErrorSYS("%s: SuspendThread error=%d\n", ApiRef, GetLastError());
		return 0;
	}

	// restore original entry point
	if(!WriteProcessMemory(TargetHandle, StartAddress, &StartingCode, 4, &BytesCount)){
		OutErrorSYS("%s: WriteProcessMemory error=%d\n", ApiRef, GetLastError());
		return 0;
	}

	if(!FlushInstructionCache(TargetHandle, StartAddress, 4)){
		OutErrorSYS("%s: FlushInstructionCache ERROR err=%#x\n", ApiRef, GetLastError());
		return 0;
	}

	// you are ready to go
	// pause and restore original entry point
	if((*pResumeThread)(lpProcessInformation->hThread)==(DWORD)-1){
		OutErrorSYS("%s: ResumeThread error=%d at:%d\n", ApiRef, GetLastError(), __LINE__);
		return 0;
	}

	return 1;
}

static BOOL CreateProcessSuspendedA(
	LPCTSTR lpApplicationName, 
	LPTSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPSTR lpExePath
)
{
	ApiName("CreateProcessA");
	BOOL res;
	HANDLE TargetHandle;
	FILE *fExe = NULL;
	BOOL bKillProcess = FALSE;

	OutTraceDW("%s: appname=\"%s\" commandline=\"%s\" dir=\"%s\"\n", 
		ApiRef, lpApplicationName, lpCommandLine, lpCurrentDirectory);
	// attempt to load the specified target
	res=(*pCreateProcessA)(
		lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, false, 
		dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if (!res){
		OutErrorSYS("%s(CREATE_SUSPENDED) ERROR: err=%d\n", ApiRef, GetLastError());
		res=(*pCreateProcessA)(NULL, lpCommandLine, 0, 0, false, dwCreationFlags, NULL, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		if(!res){ 
			OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
		}
		return res;
	}
	if(dxw.dwFlags7 & CLEARSHIMS) {
		OutTrace("clearing shims on PEB hp=%#x\n", lpProcessInformation->hProcess);
		clear_shim(lpProcessInformation->hProcess);
		ResumeThread(lpProcessInformation->hThread);
	}

	while(TRUE){ // fake loop
		bKillProcess = TRUE;

		// locate the entry point
		TargetHandle = OpenProcess(
			PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_SUSPEND_RESUME, 
			FALSE, 
			lpProcessInformation->dwProcessId);

		fExe = fopen(lpExePath, "rb");
		if(fExe == NULL){
			OutErrorSYS("%s: ERROR fopen(\"%s\") err=%d\n", ApiRef, lpExePath, GetLastError());
			break;
		}

		bKillProcess = !InjectSon(ApiRef, fExe, lpProcessInformation, TargetHandle);
		break; // exit fake loop
	}

	// cleanup ....
	if(fExe) fclose(fExe);
	if(TargetHandle) CloseHandle(TargetHandle);
	// terminate the newly spawned process
	if(bKillProcess){
		OutTraceDW("%s: Kill son process hproc=%#x pid=%#x\n", ApiRef, lpProcessInformation->hProcess, lpProcessInformation->dwProcessId);
		if(!TerminateProcess( lpProcessInformation->hProcess, -1 )){
			OutErrorSYS("%s: failed to kill hproc=%#x err=%d\n", ApiRef, lpProcessInformation->hProcess, GetLastError());
		}
	}
	OutDebugDW("%s: resumed\n", ApiRef);
	return res;
}

static BOOL CreateProcessSuspendedW(
	LPCWSTR lpApplicationName, 
	LPWSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPWSTR lpExePath
)
{
	ApiName("CreateProcessW");
	BOOL res;
	HANDLE TargetHandle;
	FILE *fExe = NULL;
	BOOL bKillProcess = FALSE;

	OutTraceDW("%s: appname=\"%ls\" commandline=\"%ls\" dir=\"%ls\"\n", 
		ApiRef, lpApplicationName, lpCommandLine, lpCurrentDirectory);
	// attempt to load the specified target
	res=(*pCreateProcessW)(
		lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, false, 
		dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if (!res){
		OutErrorSYS("%s(CREATE_SUSPENDED) ERROR: err=%d\n", ApiRef, GetLastError());
		res=(*pCreateProcessW)(NULL, lpCommandLine, 0, 0, false, dwCreationFlags, NULL, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		if(!res){ 
			OutErrorSYS("%s ERROR: err=%d\n", ApiRef, GetLastError());
		}
		return res;
	}
	if(dxw.dwFlags7 & CLEARSHIMS) {
		OutTrace("clearing shims on PEB hp=%#x\n", lpProcessInformation->hProcess);
		clear_shim(lpProcessInformation->hProcess);
		ResumeThread(lpProcessInformation->hThread);
	}

	while(TRUE){ // fake loop
		bKillProcess = TRUE;

		// locate the entry point
		TargetHandle = OpenProcess(
			PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_SUSPEND_RESUME, 
			FALSE, 
			lpProcessInformation->dwProcessId);

		fExe = _wfopen(lpExePath, L"rb");
		if(fExe == NULL){
			OutErrorSYS("%s: ERROR fopen(\"%ls\") err=%d\n", ApiRef, lpExePath, GetLastError());
			break;
		}

		bKillProcess = !InjectSon(ApiRef, fExe, lpProcessInformation, TargetHandle);
		break; // exit fake loop
	}

	// cleanup ....
	if(fExe) fclose(fExe);
	if(TargetHandle) CloseHandle(TargetHandle);
	// terminate the newly spawned process
	if(bKillProcess){
		OutTraceDW("%s: Kill son process hproc=%#x pid=%#x\n", ApiRef, lpProcessInformation->hProcess, lpProcessInformation->dwProcessId);
		if(!TerminateProcess( lpProcessInformation->hProcess, -1 )){
			OutErrorSYS("%s: failed to kill hproc=%#x err=%d\n", ApiRef, lpProcessInformation->hProcess, GetLastError());
		}
	}
	OutDebugDW("%s: resumed\n", ApiRef);
	return res;
}

static BOOL CreateProcessDetoursA(
	LPCTSTR lpApplicationName, 
	LPTSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPSTR lpExePath
)
{
	ApiName("CreateProcessA");
	BOOL res;
	char dllpath[MAX_PATH+1];
	if(dxw.dwFlags7 & CLEARSHIMS){
		if (!CreateProcess(lpExePath, lpCommandLine, 0, 0, false, CREATE_SUSPENDED, NULL, lpCurrentDirectory, lpStartupInfo, lpProcessInformation)){
			OutTraceSYS("%s: CreateProcess failed err=%d\n", ApiRef, GetLastError());
			return FALSE;
		}
		OutTrace("%s: clearing shims on PEB hp=%#x\n", ApiRef, lpProcessInformation->hProcess);
		clear_shim(lpProcessInformation->hProcess);
		LPCSTR szDll = dllpath;
		if (!DetourUpdateProcessWithDll(lpProcessInformation->hProcess, &szDll, 1) &&
			!DetourProcessViaHelperA(lpProcessInformation->dwProcessId,
									 dllpath,
									 CreateProcessA)) {
			OutTrace("%s: InjectDetours process failed to resume\n", ApiRef);
			TerminateProcess(lpProcessInformation->hProcess, ~0u);
			return FALSE;
		}
		ResumeThread(lpProcessInformation->hThread);
	}
	else {
		GetModuleFileNameA(GetModuleHandleA("dxwnd.dll"), dllpath, MAX_PATH);
		if(res = DetourCreateProcessWithDllExA(lpExePath, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
			bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation, 
			dllpath, NULL)){
				OutTrace("%s: InjectDetours process started pi=%#x\n", ApiRef, lpProcessInformation->dwProcessId);
		}
		else {
			OutTrace("%s: InjectDetours process failed to start\n", ApiRef);
		}
	}
	return res;
}

static BOOL CreateProcessDetoursW(
	LPCWSTR lpApplicationName, 
	LPWSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPWSTR lpExePath
)
{
	ApiName("CreateProcessW");
	BOOL res;
	char dllpath[MAX_PATH+1];
	// GetFullPathName("dxwnd.dll", MAX_PATH, dllpath, NULL);
	GetModuleFileNameA(GetModuleHandleA("dxwnd.dll"), dllpath, MAX_PATH);
	if(res = DetourCreateProcessWithDllExW(lpExePath, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
		bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation, 
		dllpath, NULL)){
			OutTrace("%s: InjectDetours process started pi=%#x\n", ApiRef, lpProcessInformation->dwProcessId);
	}
	else {
		OutTrace("%s: InjectDetours process failed to start\n", ApiRef);
	}
	return res;
}

static BOOL CreateProcessLateHookA(
	LPCTSTR lpApplicationName, 
	LPTSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	ApiName("CreateProcessA");
	BOOL res;
	if(dxw.dwFlags7 & CLEARSHIMS) dwCreationFlags |= CREATE_SUSPENDED;
	res=(*pCreateProcessA)(
		lpApplicationName, 
		lpCommandLine, 
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);
	if(dxw.dwFlags7 & CLEARSHIMS){
		OutTrace("clearing shims on PEB hp=%#x\n", lpProcessInformation->hProcess);
		clear_shim(lpProcessInformation->hProcess);
		ResumeThread(lpProcessInformation->hThread);
	}
	return res;
}

#define PRIORITYMASK (ABOVE_NORMAL_PRIORITY_CLASS | BELOW_NORMAL_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | IDLE_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS)

static BOOL CreateProcessLateHookW(
	LPCWSTR lpApplicationName, 
	LPWSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	ApiName("CreateProcessW");
	BOOL res;

	if(dxw.dwFlags7 & CLEARSHIMS) dwCreationFlags |= CREATE_SUSPENDED;
	res=(*pCreateProcessW)(
		lpApplicationName, 
		lpCommandLine, 
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);
	if(dxw.dwFlags7 & CLEARSHIMS){
		OutTrace("clearing shims on PEB hp=%#x\n", lpProcessInformation->hProcess);
		clear_shim(lpProcessInformation->hProcess);
		ResumeThread(lpProcessInformation->hThread);
	}
	return res;
}

BOOL WINAPI extCreateProcessA(
	LPCSTR lpApplicationName, 
	LPSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCSTR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	ApiName("CreateProcessA");
	BOOL res;
	char *exePath = NULL;

#ifndef DXW_NOTRACES
	char *mode = 
		(dxw.dwFlags4 & SUPPRESSCHILD) ? "SUPPRESS" :
		(dxw.dwFlags5 & DEBUGSON) ? "DEBUG" :
		(dxw.dwFlags5 & INJECTSON) ? "INJECT" :
		(dxw.dwFlags19 & DETOURSSON) ? "DETOURS" :
		"DEFAULT";
	OutTraceDW("%s: ApplicationName=\"%s\" CommandLine=\"%s\" CreationFlags=%#x CurrentDir=\"%s\" mode=%s\n", 
		ApiRef,
		lpApplicationName ? lpApplicationName : "NULL", 
		lpCommandLine ? lpCommandLine : "NULL", 
		dwCreationFlags, 
		lpCurrentDirectory ? lpCurrentDirectory : "NULL",
		mode);
	if(IsDebugSYS){
		if(lpProcessAttributes){
			OutTrace("> lpProcessAttributes=%#x\n", lpProcessAttributes);
			OutTrace(">> nLength=%d\n", lpProcessAttributes->nLength);
			OutTrace(">> lpSecurityDescriptor=%#x\n", lpProcessAttributes->lpSecurityDescriptor);
			OutTrace(">> bInheritHandle=%d\n", lpProcessAttributes->bInheritHandle);
		}
		else {
			OutTrace("> lpProcessAttributes=NULL\n");
		}
		if(lpThreadAttributes){
			OutTrace("> lpThreadAttributes=%#x\n", lpThreadAttributes);
			OutTrace(">> nLength=%d\n", lpThreadAttributes->nLength);
			OutTrace(">> lpSecurityDescriptor=%#x\n", lpThreadAttributes->lpSecurityDescriptor);
			OutTrace(">> bInheritHandle=%d\n", lpThreadAttributes->bInheritHandle);
		}
		else {
			OutTrace("> lpThreadAttributes=NULL\n");
		}
		OutTrace("> bInheritHandles=%#x\n", bInheritHandles);
		OutTrace("> lpEnvironment=%#x\n", lpEnvironment);
		if(lpStartupInfo){
			OutTrace("> lpStartupInfo=%#x\n", lpStartupInfo);
			OutTrace(">> cb=%d\n", lpStartupInfo->cb);
			OutTrace(">> lpReserved=%#x\n", lpStartupInfo->lpReserved);
			OutTrace(">> lpDesktop=%#x\n", lpStartupInfo->lpDesktop);
			OutTrace(">> lpTitle=%#x\n", lpStartupInfo->lpTitle);
			OutTrace(">> dwX,dwY=%d,%d\n", lpStartupInfo->dwX, lpStartupInfo->dwY);
			OutTrace(">> dwXSize,dwYSize=%d,%d\n", lpStartupInfo->dwXSize, lpStartupInfo->dwYSize);
			OutTrace(">> dwXCountChars,dwYCountChars=%d,%d\n", lpStartupInfo->dwXCountChars, lpStartupInfo->dwYCountChars);
			OutTrace(">> dwFillAttribute=%#x\n", lpStartupInfo->dwFillAttribute);
			OutTrace(">> dwFlags=%#x\n", lpStartupInfo->dwFlags);
			OutTrace(">> wShowWindow=%#x\n", lpStartupInfo->wShowWindow);
			OutTrace(">> hStdInput/hStdOutput/hStdError=%#x/%#x/%#x\n", lpStartupInfo->hStdInput, lpStartupInfo->hStdOutput, lpStartupInfo->hStdError);
		}
		else {
			OutTrace("> lpStartupInfo=NULL\n");
		}
	}
#endif // DXW_NOTRACES

	if(dxw.bHintActive) ShowHint(HINT_PROCESS);

	if(dxw.dwFlags4 & SUPPRESSCHILD) {
		OutTraceDW("%s: SUPPRESS\n", ApiRef);
		return TRUE;
	}

	// TBT: to be tested
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		if(lpApplicationName) {
			// v2.06.08: translate exe path when not NULL: @#@ "Graduation"
			lpApplicationName = dxwTranslatePathA(lpApplicationName, &mapping);
			if(mapping != DXW_NO_FAKE) OutTraceDW("%s: MAP appname=%s\n", ApiRef, lpCommandLine);
		}
		if(lpCommandLine) {
			// v2.06.03: @#@ "Euratchacha Mu Daeri" (Ko 1997)
			lpCommandLine = (LPSTR)dxwTranslatePathA((LPCSTR)lpCommandLine, &mapping);
			if(mapping != DXW_NO_FAKE) OutTraceDW("%s: MAP cmdline=%s\n", ApiRef, lpCommandLine);
		}
	}
	
	if((dxw.dwFlags5 & (INJECTSON|DEBUGSON)) || (dxw.dwFlags19 & DETOURSSON)) {
		// v2.05.66 fix: instead of releasing the mutex, set the multitask
		// flag dinamically. This works much better for instance with 
		// "Space Clash"
		extern HANDLE hLockMutex;
		ReleaseMutex(hLockMutex);
		GetHookInfo()->AllowMultiTask = TRUE;
	}

	if((dxw.dwFlags15 & IGNORESCHEDULER) && (dwCreationFlags & PRIORITYMASK)) {
		OutTraceDW("%s: IGNORESCHEDULER\n", ApiRef);
        dwCreationFlags &= ~PRIORITYMASK;
        dwCreationFlags |= NORMAL_PRIORITY_CLASS;
	}

	if((dxw.dwFlags13 & SHAREDHOOK) || (dxw.dwFlags5 & INJECTSON)) {
		exePath = OpenExeFromArgumentsA(lpApplicationName, lpCommandLine);
		if(exePath == NULL){
			OutErrorSYS("%s: ERROR no exe from arguments error=%d\n", ApiRef, GetLastError());
			return 0;
		}
	}

	if(dxw.dwFlags13 & SHAREDHOOK){
		char fullPath[MAX_PATH+1];
		int index = GetHookInfo()->TaskIdx;
		GetFullPathNameA(exePath, MAX_PATH, fullPath, NULL);
		for(int i = 0; fullPath[i]; i++) fullPath[i] = tolower(fullPath[i]);
		OutTraceDW("%s: SHAREDHOOK path=\"%s\" full=\"%s\" taskidx=%d\n", ApiRef, exePath, fullPath, index);
		TARGETMAP *pMapping = (TARGETMAP *)((char *)GetHookInfo() + sizeof(DXWNDSTATUS) + (index * sizeof(TARGETMAP)));
		OutTraceDW("%s: SHAREDHOOK BEFORE path=\"%s\"\n", ApiRef, pMapping->path);
		strncpy(pMapping->path, fullPath, MAX_PATH);
		OutTraceDW("%s: SHAREDHOOK AFTER  path=\"%s\"\n", ApiRef, pMapping->path);
		if (!FlushViewOfFile(pMapping->path, MAX_PATH))
			OutErrorSYS("%s: FlushViewOfFile ERROR err=%d\n", ApiRef, GetLastError());
	}

	if(dxw.dwFlags5 & DEBUGSON) {
		res=CreateProcessDebugA(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation
		);
	}
	else
	if(dxw.dwFlags5 & INJECTSON) {
		res=CreateProcessSuspendedA(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation,
			exePath
		);
	}
	else 
	if(dxw.dwFlags19 & DETOURSSON) {
		res=CreateProcessDetoursA(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation,
			exePath
		);
	}
	else{
		res = CreateProcessLateHookA(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation
		);
	}

	if(exePath) free(exePath);
	_if(!res) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}

BOOL WINAPI extCreateProcessW(
	LPCWSTR lpApplicationName, 
	LPWSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	ApiName("CreateProcessW");
	BOOL res;
	LPWSTR exePath = NULL;

#ifndef DXW_NOTRACES
	char *mode = 
		(dxw.dwFlags4 & SUPPRESSCHILD) ? "SUPPRESS" :
		(dxw.dwFlags5 & DEBUGSON) ? "DEBUG" :
		(dxw.dwFlags5 & INJECTSON) ? "INJECT" :
		(dxw.dwFlags19 & DETOURSSON) ? "DETOURS" :
		"DEFAULT";
	OutTraceDW("%s: ApplicationName=\"%ls\" CommandLine=\"%ls\" CreationFlags=%#x CurrentDir=\"%ls\" mode=%s\n", 
		ApiRef,
		lpApplicationName ? lpApplicationName : L"NULL", 
		lpCommandLine ? lpCommandLine : L"NULL", 
		dwCreationFlags, 
		lpCurrentDirectory ? lpCurrentDirectory : L"NULL",
		mode);
#endif // DXW_NOTRACES

	if(dxw.bHintActive) ShowHint(HINT_PROCESS);

	if(dxw.dwFlags4 & SUPPRESSCHILD) {
		OutTraceDW("%s: SUPPRESS\n", ApiRef);
		return TRUE;
	}

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		// v2.06.03: @#@ untested
		DWORD mapping;
		lpCommandLine = (LPWSTR)dxwTranslatePathW((LPCWSTR)lpCommandLine, &mapping);
		if(mapping != DXW_NO_FAKE) OutTraceDW("%s: MAP %ls\n", ApiRef, lpCommandLine);
	}

	if((dxw.dwFlags5 & (INJECTSON|DEBUGSON)) || (dxw.dwFlags19 & DETOURSSON)){
		// v2.05.66 fix: instead of releasing the mutex, set the multitask
		// flag dinamically.
		//
		extern HANDLE hLockMutex;
		ReleaseMutex(hLockMutex);
		GetHookInfo()->AllowMultiTask = TRUE;
	}
	
	if((dxw.dwFlags15 & IGNORESCHEDULER) && (dwCreationFlags & PRIORITYMASK)) {
		OutTraceDW("%s: IGNORESCHEDULER\n", ApiRef);
        dwCreationFlags &= ~PRIORITYMASK;
        dwCreationFlags |= NORMAL_PRIORITY_CLASS;
	}

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		lpApplicationName = dxwTranslatePathW(lpApplicationName, NULL);
	}

	if((dxw.dwFlags13 & SHAREDHOOK) || (dxw.dwFlags5 & INJECTSON)) {
		exePath = OpenExeFromArgumentsW(lpApplicationName, lpCommandLine);
		if(exePath == NULL){
			OutErrorSYS("%s: ERROR no exe from arguments error=%d\n", ApiRef, GetLastError());
			return 0;
		}
	}

	if(dxw.dwFlags13 & SHAREDHOOK){
		WCHAR fullPath[MAX_PATH+1];
		int index = GetHookInfo()->TaskIdx;
		GetFullPathNameW(exePath, MAX_PATH, fullPath, NULL);
		OutTraceDW("%s: SHAREDHOOK path=\"%ls\" full=\"%ls\"\n", ApiRef, exePath, fullPath);
		TARGETMAP *pMapping = (TARGETMAP *)((char *)GetHookInfo() + sizeof(DXWNDSTATUS));
		int len = wcslen(fullPath);
		char *fullPathA = (char *)malloc(len+1);
		WideCharToMultiByte(0, 0, fullPath, len, fullPathA, len, NULL, NULL);
		fullPathA[len]=0;
		OutTraceDW("%s: SHAREDHOOK BEFORE path=\"%s\"\n", ApiRef, pMapping->path);
		strncpy(pMapping->path, fullPathA, MAX_PATH);
		OutTraceDW("%s: SHAREDHOOK AFTER  path=\"%s\"\n", ApiRef, pMapping->path);
		if (!FlushViewOfFile(pMapping->path, MAX_PATH))
			OutErrorSYS("%s: FlushViewOfFile ERROR err=%d\n", ApiRef, GetLastError());
		free(fullPathA);
	}

	if(dxw.dwFlags5 & DEBUGSON) {
		res=CreateProcessDebugW(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation
		);
	}
	else
	if(dxw.dwFlags5 & INJECTSON) {
		res=CreateProcessSuspendedW(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation,
			exePath
		);
	}
	else
	if(dxw.dwFlags19 & DETOURSSON) {
		res=CreateProcessDetoursW(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation,
			exePath
		);
	}
	else{
		res = CreateProcessLateHookW(
			lpApplicationName, 
			lpCommandLine, 
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpCurrentDirectory,
			lpStartupInfo,
			lpProcessInformation
		);
	}

	_if(!res) OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return res;
}
