#include "stdafx.h"
#include "dxwndhost.h"

#include <stddef.h>
#include <bcrypt.h>
#include "winternl.h"

extern BYTE APCProcx86[];

typedef struct _APCCONTEXT
{
	union {
		LPVOID lpStartAddress;
		BYTE bPadding1[8];
	} s;
	union {
		LPVOID lpParameter;
		BYTE bPadding2[8];
	} p;
	BYTE bExecuted;
} APCCONTEXT, *LPAPCCONTEXT;

typedef NTSTATUS(NTAPI *pLdrLoadDll)(PWCHAR, ULONG, PUNICODE_STRING, PHANDLE);
typedef NTSTATUS(NTAPI *pNtTerminateThread)(IN HANDLE ThreadHandle, IN NTSTATUS ExitStatus);
typedef NTSTATUS(*pLdrpInitializeProcess)(IN PCONTEXT Context OPTIONAL, IN PVOID SystemDllBase);

typedef struct _THREAD_DATA
{
	pLdrLoadDll fnLdrLoadDll;
	union
	{
		pNtTerminateThread fnNtTerminateThread;
		//PPS_POST_PROCESS_INIT_ROUTINE fnOrigPostProcessInit;
		DWORD fnOrigPostProcessInit;
		pLdrpInitializeProcess fnLdrpInitializeProcess;
	};
	UNICODE_STRING UnicodeString;
	WCHAR DllName[MAX_PATH];
	PWCHAR DllPath;
	ULONG Flags;
	LPTHREAD_START_ROUTINE OrigEIP;
	PVOID EIPParams;
	HANDLE ModuleHandle;
}THREAD_DATA, *PTHREAD_DATA;

#define ThreadProc ThreadProcx86
#define APCProc APCProcx86
#define PostProcProc PostProcProcx86
#define LdrpInitializeProcessProc LdrpInitializeProcessProcx86

BYTE apc_stub_x86[] =
"\xFC\x8B\x74\x24\x04\x55\x89\xE5\xE8\x89\x00\x00\x00\x60\x89\xE5"
"\x31\xD2\x64\x8B\x52\x30\x8B\x52\x0C\x8B\x52\x14\x8B\x72\x28\x0F"
"\xB7\x4A\x26\x31\xFF\x31\xC0\xAC\x3C\x61\x7C\x02\x2C\x20\xC1\xCF"
"\x0D\x01\xC7\xE2\xF0\x52\x57\x8B\x52\x10\x8B\x42\x3C\x01\xD0\x8B"
"\x40\x78\x85\xC0\x74\x4A\x01\xD0\x50\x8B\x48\x18\x8B\x58\x20\x01"
"\xD3\xE3\x3C\x49\x8B\x34\x8B\x01\xD6\x31\xFF\x31\xC0\xAC\xC1\xCF"
"\x0D\x01\xC7\x38\xE0\x75\xF4\x03\x7D\xF8\x3B\x7D\x24\x75\xE2\x58"
"\x8B\x58\x24\x01\xD3\x66\x8B\x0C\x4B\x8B\x58\x1C\x01\xD3\x8B\x04"
"\x8B\x01\xD0\x89\x44\x24\x24\x5B\x5B\x61\x59\x5A\x51\xFF\xE0\x58"
"\x5F\x5A\x8B\x12\xEB\x86\x5B\x80\x7E\x10\x00\x75\x3B\xC6\x46\x10"
"\x01\x68\xA6\x95\xBD\x9D\xFF\xD3\x3C\x06\x7C\x1A\x31\xC9\x64\x8B"
"\x41\x18\x39\x88\xA8\x01\x00\x00\x75\x0C\x8D\x93\xCF\x00\x00\x00"
"\x89\x90\xA8\x01\x00\x00\x31\xC9\x51\x51\xFF\x76\x08\xFF\x36\x51"
"\x51\x68\x38\x68\x0D\x16\xFF\xD3\xC9\xC2\x0C\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00";
int apc_stub_x86_size = sizeof(apc_stub_x86);

BYTE APCProcx86[] = {
// 52   : {
/* 00000 */	0x55,                           // push	 ebp
/* 00001 */	0x8b, 0xec,                     // mov	 ebp, esp
/* 00003 */	0x56,                           // push	 esi
// 53   : 	data->fnLdrLoadDll(data->DllPath, data->Flags, &data->UnicodeString, &data->ModuleHandle);
/* 00004 */	0x8b, 0x75, 0x08,               // mov	 esi, DWORD PTR _data$[ebp]
/* 00007 */	0x57,                           // push	 edi
/* 00008 */	0x8d, 0xbe, 0x28, 0x02, 0x00, 
/*       */	0x00,                           // lea	 edi, DWORD PTR [esi+552]
/* 0000e */	0x57,                           // push	 edi
/* 0000f */	0x8d, 0x46, 0x08,               // lea	 eax, DWORD PTR [esi+8]
/* 00012 */	0x50,                           // push	 eax
/* 00013 */	0xff, 0xb6, 0x1c, 0x02, 0x00, 
/*       */	0x00,                           // push	 DWORD PTR [esi+540]
/* 00019 */	0x8b, 0x06,                     // mov	 eax, DWORD PTR [esi]
/* 0001b */	0xff, 0xb6, 0x18, 0x02, 0x00, 
/*       */	0x00,                           // push	 DWORD PTR [esi+536]
/* 00021 */	0xff, 0xd0,                     // call	 eax
// 54   : 	if (data->OrigEIP) return (HANDLE)data->OrigEIP(data->EIPParams);
/* 00023 */	0x8b, 0x86, 0x20, 0x02, 0x00, 
/*       */	0x00,                           // mov	 eax, DWORD PTR [esi+544]
/* 00029 */	0x85, 0xc0,                     // test	 eax, eax
/* 0002b */	0x74, 0x0e,                     // je	 SHORT $LN2@APCProc
/* 0002d */	0xff, 0xb6, 0x24, 0x02, 0x00, 
/*       */	0x00,                           // push	 DWORD PTR [esi+548]
/* 00033 */	0xff, 0xd0,                     // call	 eax
/* 00035 */	0x5f,                           // pop	 edi
/* 00036 */	0x5e,                           // pop	 esi
// 56   : }
/* 00037 */	0x5d,                           // pop	 ebp
/* 00038 */	0xc2, 0x04, 0x00,               // ret	 4
/*LN2@APCProc:
 */	
// 55   : 	return data->ModuleHandle;
/* 0003b */	0x8b, 0x07,                     // mov	 eax, DWORD PTR [edi]
/* 0003d */	0x5f,                           // pop	 edi
/* 0003e */	0x5e,                           // pop	 esi
// 56   : }
/* 0003f */	0x5d,                           // pop	 ebp
/* 00040 */	0xc2, 0x04, 0x00                // ret	 4
};

const unsigned int APCProcx86Size = sizeof(APCProcx86);

//typedef NTSTATUS(NTAPI * NTQUEUEAPCTHREAD)(HANDLE hThreadHandle, LPVOID lpApcRoutine, LPVOID lpApcRoutineContext, LPVOID lpApcStatusBlock, LPVOID lpApcReserved);
typedef NTSTATUS(NTAPI * NTQUEUEAPCTHREAD)(HANDLE hThreadHandle, LPVOID lpApcRoutine, LPVOID lpApcRoutineContext, LPVOID lpApcStatusBlock, LPVOID lpApcReserved);

BOOL inject_via_apcthread(HANDLE hProcess, HANDLE hThread, LPVOID lpStartAddress, LPVOID lpParameter)
{
	LPVOID lpApcStub;
	LPVOID lpRemoteApcStub, lpRemoteApcContext;
	APCCONTEXT ctx = { 0 };
	DWORD dwApcStubLength = 0;
	NTQUEUEAPCTHREAD pNtQueueApcThread = (NTQUEUEAPCTHREAD)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "NtQueueApcThread");

	ctx.s.lpStartAddress = lpStartAddress;
	ctx.p.lpParameter = lpParameter;
	ctx.bExecuted = FALSE;

	// Get the architecture specific apc inject stub...
	lpApcStub = &apc_stub_x86;
	dwApcStubLength = sizeof(apc_stub_x86);

	if (!pNtQueueApcThread ||
		!(lpRemoteApcStub = VirtualAllocEx(hProcess, NULL, dwApcStubLength + sizeof(APCCONTEXT), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)))
		return FALSE;
	lpRemoteApcContext = ((BYTE *)lpRemoteApcStub + dwApcStubLength);
	if (!WriteProcessMemory(hProcess, lpRemoteApcStub, lpApcStub, dwApcStubLength, NULL) ||
		!WriteProcessMemory(hProcess, lpRemoteApcContext, (LPCVOID)&ctx, sizeof(APCCONTEXT), NULL))
		return FALSE;

	return pNtQueueApcThread(hThread, lpRemoteApcStub, lpRemoteApcContext, 0, 0) == ERROR_SUCCESS;
}

BOOL injectLdrLoadDLL(HANDLE hProcess, HANDLE hThread, WCHAR *szDLL)
{
	THREAD_DATA data;
	PVOID pData, code, pProc;
	BOOL bRet = TRUE;
	ULONG ulSizeOfCode;
	HMODULE nt;
	static pLdrpInitializeProcess fnLdrpInitializeProcess = NULL;

	nt = GetModuleHandleW(L"ntdll.dll");

	ZeroMemory(&data, sizeof(data));
	data.fnLdrLoadDll = (pLdrLoadDll)GetProcAddress(nt, "LdrLoadDll");
	lstrcpyW(data.DllName, szDLL);

	pProc = APCProc;
	ulSizeOfCode = sizeof(APCProc) / sizeof(APCProc[0]);

	if (!(code = VirtualAllocEx(hProcess, NULL, ulSizeOfCode + sizeof(data), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)))
		return FALSE;
	pData = (LPBYTE)code + ulSizeOfCode;
	data.UnicodeString.Length = (USHORT)wcslen(data.DllName) * sizeof(WCHAR);
	data.UnicodeString.MaximumLength = sizeof(data.DllName);
	data.UnicodeString.Buffer = (PWSTR)((PBYTE)pData + offsetof(THREAD_DATA, DllName));
	if (!WriteProcessMemory(hProcess, pData, &data, sizeof(data), NULL) ||
		!WriteProcessMemory(hProcess, code, (PVOID)pProc, ulSizeOfCode, NULL)){
		OutTrace("inject_via_apcthread: error @%d\n", __LINE__);
		return FALSE;
	}
	return inject_via_apcthread(hProcess, hThread, code, pData);
}

void InjectAPC(char *exepath, char *dirpath, BOOL bSuspended, BOOL bCommitPage, TARGETMAP *tm, PRIVATEMAP *pm)
{
	STARTUPINFO sinfo;
	PROCESS_INFORMATION pinfo;
	char DebugMessage[1024];
	BOOL bKillProcess = FALSE;
	HANDLE hEvent;
	char *eventName = "DxWndInjectCompleted";
	DWORD t0 = GetTickCount();
	LPSTR lpCommandLine = NULL;
	BOOL APCret;

	OutTrace("InjectAPC: exe=\"%s\" dir=\"%s\" commit=%x\n",exepath, dirpath, bCommitPage);

	hEvent = CreateEvent(NULL, FALSE, FALSE, eventName);
	if(!hEvent) {
		if(ERROR_ALREADY_EXISTS == GetLastError()){
			hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName);
		} 
		if(!hEvent) {
			OutTrace("CreateEvent %s failed: err=%d\n", eventName, GetLastError());
		}
	}

	ZeroMemory(&sinfo, sizeof(sinfo));
	sinfo.cb = sizeof(sinfo);
	// attempt to load the specified target
	// v2.05.21: setlpCommandLine together with lpApplicationName for games that requires it,
	// for instance "Star Wars Rogue Squadron 3D"
	// v2.05.23: made this configurable with SETCMDLINE flag
	// v2.05.53: made configurable by field copy and fixed argument order!!
	if(pm->cmdline[0])
		lpCommandLine = pm->cmdline;

	if (!CreateProcess(exepath, lpCommandLine, 0, 0, false, CREATE_SUSPENDED, NULL, dirpath, &sinfo, &pinfo)){
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

	WCHAR dllpath[MAX_PATH];
	GetFullPathNameW(L"dxwnd.dll", MAX_PATH, dllpath, NULL);
	APCret = injectLdrLoadDLL(pinfo.hProcess, pinfo.hThread, dllpath);
	OutTrace("injectLdrLoadDLL: ret=%d\n", APCret);

	if(bSuspended){
		MessageBoxA(NULL, "Click when the debugger is attached", "debug", 0);
	}
	else {
        HMODULE hUser32 = GetModuleHandle("user32.dll");
        if (hUser32){
			typedef int (WINAPI *MessageBoxTimeoutA_Type)(HWND, LPCSTR, LPCSTR, UINT, WORD, DWORD);
			MessageBoxTimeoutA_Type pMessageBoxTimeoutA;
            pMessageBoxTimeoutA = (MessageBoxTimeoutA_Type)GetProcAddress(hUser32, "MessageBoxTimeoutA");
			if(pMessageBoxTimeoutA) (*pMessageBoxTimeoutA)(NULL, "wait ...", "DxWnd", 0, 0, 500);
		}
	}

	if(ResumeThread(pinfo.hThread)==(DWORD)-1){
		sprintf(DebugMessage,"ResumeThread error=%d at:%d", GetLastError(), __LINE__);
		MessageBoxEx(0, DebugMessage, "Injection", MB_ICONEXCLAMATION|MB_OK, NULL);
		OutTrace("%s\n", DebugMessage);
	}

	CloseHandle(pinfo.hThread);
	CloseHandle(pinfo.hProcess);

	CloseHandle(hEvent);
}
