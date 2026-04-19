#define  _CRT_SECURE_NO_WARNINGS
#define _MODULE "dxwnd"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include <psapi.h>

// thank to KuromeSan article here: https://gist.github.com/KuromeSan/56d8b724c0696b54f9f81994ae3591d1

static void DefuseFLashBomb(HMODULE flash)
{
	ApiName("DefuseFLashBomb");
	static BOOL defused = FALSE;
	if(defused) return;
	BYTE bomb[8] = {0x00, 0x00, 0x40, 0x46, 0x3E, 0x6F, 0x77, 0x42};
	BYTE nuke[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7F};

	unsigned char *start;
	DWORD dwSegSize;
	DWORD oldprot;

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
	(*pGetModuleInformation)(GetCurrentProcess(), flash, &ModuleInfo, sizeof(MODULEINFO));
	FreeLibrary(psapilib);
	OutTraceDW("%s: flash.ocx segment base=%#x len=%#x entry=%#x\n",
		ApiRef, 
		ModuleInfo.lpBaseOfDll,
		ModuleInfo.SizeOfImage,
		ModuleInfo.EntryPoint
		);

	start = (LPBYTE)ModuleInfo.lpBaseOfDll;
	dwSegSize = ModuleInfo.SizeOfImage;
	for(LPBYTE p=start; p < start + dwSegSize - 8; p++){
		if(!memcmp(p, bomb, 8)){
			if(!VirtualProtect((LPVOID)p, 8, PAGE_READWRITE, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR: target=%#x err=%d @%d\n", 
					ApiRef, p, GetLastError(), __LINE__);
				return; // error condition
			}
			memcpy(p, nuke, 8);
			if(!VirtualProtect((LPVOID)p, 8, oldprot, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR: target=%#x err=%d @%d\n", 
					ApiRef, p, GetLastError(), __LINE__);
				return; // error condition
			}
			if(!FlushInstructionCache(GetCurrentProcess(), p, 8)){
				OutTrace("%s: FlushInstructionCache ERROR target=%#x, err=%#x\n", 
					ApiRef, p, GetLastError());
				return; // error condition
			}
			defused = TRUE;
			//MessageBox(NULL, "Flash defused", "DxWnd", 0);
			break;
		}
	}
}

void HookFlash(HMODULE flash)
{
	if(dxw.dwFlags13 & DEFUSEFLASHBOMB) DefuseFLashBomb(flash);
}

