#define  _CRT_SECURE_NO_WARNINGS
//#include "dxwnd.h"
//#include "dxwcore.hpp"
//#include "syslibs.h"
//#include "dxhook.h"
#include <windows.h>
#include "injector32.h"
#include "stdio.h"

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

typedef NTSTATUS(NTAPI * NTQUEUEAPCTHREAD)(HANDLE hThreadHandle, LPVOID lpApcRoutine, LPVOID lpApcRoutineContext, LPVOID lpApcStatusBlock, LPVOID lpApcReserved);

BOOL inject_via_apcthread(HANDLE hProcess, HANDLE hThread, LPVOID lpStartAddress, LPVOID lpParameter)
{
	LPVOID lpApcStub;
	LPVOID lpRemoteApcStub, lpRemoteApcContext;
	APCCONTEXT ctx = { 0 };
	DWORD dwApcStubLength = 0;
	NTQUEUEAPCTHREAD pNtQueueApcThread = (NTQUEUEAPCTHREAD)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueueApcThread");

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