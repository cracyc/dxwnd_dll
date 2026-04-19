#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
//#include <winternl.h>

#define NTSTATUS DWORD 

BOOL IsWinXP()
{
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	return osvi.dwMajorVersion == 5;
}

void clear_shim(HANDLE hp)
{
    struct
    {
        DWORD64 Reserved1;
        DWORD64 PebBaseAddress;
        DWORD64 Reserved2[2];
        DWORD64 UniqueProcessId;
        DWORD64 Reserved3;
    } pbi64;

    struct
    {
        DWORD ExitStatus;
        DWORD PebBaseAddress;
        DWORD AffinityMask;
        DWORD BasePriority;
        DWORD UniqueProcessId;
        DWORD InheritedFromUniqueProcessId;
    } pbi;

    typedef BOOL (NTAPI *IsWow64Process_Type)(HANDLE, PBOOL);
    typedef NTSTATUS (NTAPI *NtWow64QueryInformationProcess64_Type) (HANDLE, DWORD, PVOID, ULONG, PULONG);
    typedef NTSTATUS (NTAPI *NtQueryInformationProcess_Type) (HANDLE, DWORD, PVOID, ULONG, PULONG);
    BOOL (NTAPI *pIsWow64Process)(HANDLE, PBOOL) = 0;
    NTSTATUS (NTAPI *pNtWow64QueryInformationProcess64) (HANDLE, DWORD, PVOID, ULONG, PULONG) = 0;
    NTSTATUS (NTAPI *pNtQueryInformationProcess) (HANDLE, DWORD, PVOID, ULONG, PULONG) = 0;

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll"); 
    const DWORD ProcessBasicInformation = 0;    
    BOOL bits32 = TRUE;
    DWORD count;
    DWORD64 zero = 0;

	if(IsWinXP()){
		pIsWow64Process = (IsWow64Process_Type)GetProcAddress(kernel32, "IsWow64Process");
		if(pIsWow64Process)
		{
			pNtWow64QueryInformationProcess64 = (NtWow64QueryInformationProcess64_Type)GetProcAddress(ntdll, "NtWow64QueryInformationProcess64");
			if(pNtWow64QueryInformationProcess64)
			{
				(*pIsWow64Process)(hp, &bits32);
				pNtWow64QueryInformationProcess64(hp, ProcessBasicInformation, &pbi64, sizeof(pbi64), NULL);
				// this depends on the peb64 being in 32bit address space, otherwise NtWow64WriteVirtualMemory64 is needed
				WriteProcessMemory(hp, (char *)(pbi64.PebBaseAddress) + 0x2d8, &zero, 8, &count);
			}
		}
		if(bits32)
		{
			pNtQueryInformationProcess = (NtQueryInformationProcess_Type)GetProcAddress(ntdll, "NtQueryInformationProcess");
			pNtQueryInformationProcess(hp, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
			WriteProcessMemory(hp, (char *)(pbi.PebBaseAddress) + 0x1e8, &zero, 4, &count);
		}
	}
	else {
		DWORD addr;
		pNtQueryInformationProcess = (NtQueryInformationProcess_Type)GetProcAddress(ntdll, "NtQueryInformationProcess");
		pNtQueryInformationProcess(hp, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
		ReadProcessMemory(hp, (char *)(pbi.PebBaseAddress) + 0x1e8, &addr, 4, &count);
		WriteProcessMemory(hp, (char *)addr + 0x210, &zero, 4, &count);
	}
}

#ifdef TESTMAIN
int main(int argc, char **argv)
{
    PROCESS_INFORMATION pi;
    char cmdline[1024];
    STARTUPINFOA sia = {0};
    sia.cb = sizeof(STARTUPINFOA);
    if(!CreateProcessA(NULL, argv[1], NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sia, &pi))
    {
        printf("CreateProcess failed\n");
        return -1;
    }

    clear_shim(pi.hProcess);

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
#endif
