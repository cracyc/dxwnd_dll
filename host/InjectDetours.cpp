#include "stdafx.h"
#include "dxwndhost.h"
#include "detours.h"

extern void clear_shim(HANDLE);

void InjectDetours(char *exepath, char *dirpath, BOOL bSuspended, BOOL bCommitPage, TARGETMAP *tm, PRIVATEMAP *pm)
{
	char dllpath[MAX_PATH];
	FILE *fExe = NULL;
	BOOL bKillProcess = FALSE;
	HANDLE hEvent;
	char *eventName = "DxWndInjectCompleted";
	DWORD t0 = GetTickCount();
	LPSTR lpCommandLine = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFOA sinfo = {0};
	OutTrace("InjectDetours: exe=\"%s\" dir=\"%s\" commit=%x\n",exepath, dirpath, bCommitPage);

	hEvent = CreateEvent(NULL, FALSE, FALSE, eventName);
	if(!hEvent) {
		if(ERROR_ALREADY_EXISTS == GetLastError()){
			hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName);
		} 
		if(!hEvent) {
			OutTrace("CreateEvent %s failed: err=%d\n", eventName, GetLastError());
		}
	}

	// inject DLL payload into remote process
	GetFullPathName("dxwnd.dll", MAX_PATH, dllpath, NULL);
	sinfo.cb = sizeof(sinfo);
	// attempt to load the specified target
	// v2.05.21: setlpCommandLine together with lpApplicationName for games that requires it,
	// for instance "Star Wars Rogue Squadron 3D"
	// v2.05.23: made this configurable with SETCMDLINE flag
	// v2.05.53: made configurable by field copy and fixed argument order!!
	if(pm->cmdline[0])
		lpCommandLine = pm->cmdline;

	HANDLE h = NULL;
    SECURITY_ATTRIBUTES sa;
	DWORD flags = CREATE_SUSPENDED;
	if(tm->tflags2 & OUTSTDIO){
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;       
		h = CreateFile(_T("stdout.log"),
			FILE_WRITE_DATA, //FILE_APPEND_DATA,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			&sa,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );	
		sinfo.dwFlags |= STARTF_USESTDHANDLES;
		sinfo.hStdInput = NULL;
		sinfo.hStdError = h;
		sinfo.hStdOutput = h;	
		flags |= CREATE_NO_WINDOW;
	}

	if (!CreateProcess(exepath, lpCommandLine, 0, 0, TRUE, flags, NULL, dirpath, &sinfo, &pi)){
		char DebugMessage[MAX_PATH + 80];
		int err = GetLastError();
		char *errmsg;
		switch(err){
			case ERROR_DIRECTORY:
				errmsg="(invalid directory name)";
				break;
			case ERROR_FILE_NOT_FOUND:
				errmsg="(file not found)";
				break;
			default:
				errmsg="";
				break;
		}
		sprintf(DebugMessage,"CreateProcess \"%s\" \nerror=%d %s", exepath, err, errmsg);
		MessageBoxEx(0, DebugMessage, "Injection", MB_ICONEXCLAMATION|MB_OK, NULL);
		OutTrace("%s\n", DebugMessage);
		return;
	}

	// v2.06.10: thanks to Crazyc support, here shims can be cleared on the fly 
	// with no need to copy the executable file
	if(tm->flags7 & CLEARSHIMS){
		OutTrace("clearing shims on PEB hp=%#x\n", pi.hProcess);
		clear_shim(pi.hProcess);
	}

	LPCSTR szDll = dllpath;
	if (!DetourUpdateProcessWithDll(pi.hProcess, &szDll, 1) &&
		!DetourProcessViaHelperA(pi.dwProcessId,
								 dllpath,
								 CreateProcessA)) {
		TerminateProcess(pi.hProcess, ~0u);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return;
	}
	if (bSuspended){
		MessageBox(NULL, "Click when the debugger is attached", "Injection", 0);
	}

	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	// cleanup ....
	CloseHandle(hEvent); // FIX: Early hook synchronization
	if((tm->tflags2 & OUTSTDIO) && h) CloseHandle(h);

	// terminate the newly spawned process
	if(bKillProcess){
		if(!TerminateProcess( pi.hProcess, -1 )){
			OutTrace("failed to kill hproc=%x err=%d\n", pi.hProcess, GetLastError());
		}
	}
	OutTrace("InjectDetours done!\n");
	return;
}