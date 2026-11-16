#define  _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define _COMPONENT "dxwCore"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "stdio.h"

//#define DDD_FLAGS (DDD_NO_BROADCAST_SYSTEM | DDD_RAW_TARGET_PATH | DDD_LUID_BROADCAST_DRIVE)
//#define DDD_FLAGS (DDD_RAW_TARGET_PATH | DDD_LUID_BROADCAST_DRIVE)
//#define DDD_FLAGS DDD_RAW_TARGET_PATH
#define DDD_FLAGS 0

// -------------------- REMAPSYSFOLDERS hooks --------------------
//#define RELATIVESYSPATH

typedef DWORD (WINAPI *GetPrivateProfileStringA_Type)(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);
extern GetPrivateProfileStringA_Type pGetPrivateProfileStringA;
typedef DWORD (WINAPI *GetModuleFileNameA_Type)(HMODULE, LPSTR, DWORD);
extern GetModuleFileNameA_Type pGetModuleFileNameA;

static char *myPathA = NULL;

char *dxwCore::GetFakeWinDirA()
{
	ApiName("GetFakeWinDirA");
	if(myPathA) return myPathA;
	myPathA = (char *)malloc(MAX_PATH);

#ifdef RELATIVESYSPATH
	strcpy(myPathA, ".\\Windows"); 
	return myPathA;
#endif
	if(dxw.isWineControlled){
		char keyName[10];
		char sInitPath[MAX_PATH+1];
		_snprintf(sInitPath, MAX_PATH, "%sdxwnd.ini", GetDxWndPath()); 
		_snprintf(keyName, 10, "path%d", dxw.dwIndex);
		(*pGetPrivateProfileStringA)("target", keyName, "", myPathA, MAX_PATH, sInitPath);
	}
	else {
		(*pGetModuleFileNameA)(NULL, myPathA, MAX_PATH);
	}
	//PathRemoveFileSpecA(myPathA);
	char *p = myPathA + strlen(myPathA);
	while((*p != '\\') && (p > myPathA)) p--;

	if(dwFlags20 & USEDOSDEVICES){
		if(*p == '\\') *(p+1)=0;
		char rootPath[8];
		char driveLetter;
		if(gFakeWinDriveLetter != 0) driveLetter = gFakeWinDriveLetter;
		else {
			for(driveLetter='Z'; driveLetter>'C'; driveLetter--){
				sprintf(rootPath, "%c:\\", driveLetter);
				if((*pGetDriveTypeA)(rootPath) == DRIVE_NO_ROOT_DIR) {
					gFakeWinDriveLetter = driveLetter;
					OutTrace("%s: saving fake Windows drive=%c\n", ApiRef, gFakeWinDriveLetter);
					break;
				}
			}
			if(driveLetter > 'C'){
				sprintf(rootPath,"%c:", driveLetter); // no trailing backslash is allowed!
				OutTrace("%s: DefineDosDeviceA drive=%s folder=%s\n", ApiRef, rootPath, myPathA);
				if(!DefineDosDeviceA(DDD_FLAGS, rootPath, myPathA)){
					OutTrace("%s: DefineDosDeviceA ERROR drive=%s folder=%s err=%d\n", ApiRef, rootPath, myPathA, GetLastError());
					return "C:\\Windows";
				}
			}
			else {
				driveLetter = 'C';
			}
			sprintf(myPathA, "%c:\\Windows", driveLetter);
			OutTrace("%s: Windows folder=%s\n", ApiRef, myPathA);
		}
	}
	else {
		if(*p == '\\') *p=0;
		strcat_s(myPathA, MAX_PATH, "\\Windows");
	}
	return myPathA;
}

typedef DWORD (WINAPI *GetPrivateProfileStringW_Type)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
extern GetPrivateProfileStringW_Type pGetPrivateProfileStringW;
typedef DWORD (WINAPI *GetModuleFileNameW_Type)(HMODULE, LPWSTR, DWORD);
extern GetModuleFileNameW_Type pGetModuleFileNameW;

static WCHAR *myPathW = NULL;

WCHAR *dxwCore::GetFakeWinDirW()
{
	ApiName("GetFakeWinDirW");
	if(myPathW) return myPathW;
	myPathW = (WCHAR *)malloc(MAX_PATH * sizeof(WCHAR));

#ifdef RELATIVESYSPATH
	wcscpy(myPathW, L".\\Windows"); 
	return myPathW;
#endif
	if(dxw.isWineControlled){
		WCHAR keyName[10];
		WCHAR sInitPath[MAX_PATH+1];
		swprintf(sInitPath, MAX_PATH, L"%sdxwnd.ini", GetDxWndPath()); 
		swprintf(keyName, 10, L"path%d", dxw.dwIndex);
		(*pGetPrivateProfileStringW)(L"target", keyName, L"", myPathW, MAX_PATH, sInitPath);
	}
	else {
		(*pGetModuleFileNameW)(NULL, myPathW, MAX_PATH);
	}
	//PathRemoveFileSpecW(myPathW);
	WCHAR *p = myPathW + wcslen(myPathW);
	while((*p != L'\\') && (p > myPathW)) p--;

	if(dwFlags20 & USEDOSDEVICES){
		if(*p == L'\\') *(p+1)=0;
		WCHAR rootPath[8];
		char driveLetter;
		if(gFakeWinDriveLetter != 0) driveLetter = gFakeWinDriveLetter;
		else {
			for(driveLetter='Z'; driveLetter>'C'; driveLetter--){
				swprintf(rootPath, L"%c:\\", driveLetter);
				if((*pGetDriveTypeW)(rootPath) == DRIVE_NO_ROOT_DIR) {
					gFakeWinDriveLetter = driveLetter;
					OutTrace("%s: saving fake Windows drive=%c\n", ApiRef, gFakeWinDriveLetter);
					break;
				}
			}
			if(driveLetter > 'C'){
				swprintf(rootPath,L"%c:", driveLetter); // no trailing backslash is allowed!
				OutTrace("%s: DefineDosDeviceW drive=%ls folder=%ls\n", ApiRef, rootPath, myPathW);
				if(!DefineDosDeviceW(DDD_FLAGS, rootPath, myPathW)){
					OutTrace("%s: DefineDosDeviceW ERROR drive=%ls folder=%ls err=%d\n", ApiRef, rootPath, myPathW, GetLastError());
					return L"C:\\Windows";
				}
			}
			else {
				driveLetter = 'C';
			}
			swprintf(myPathW, L"%c:\\Windows", driveLetter);
			OutTrace("%s: Windows folder=%s\n", ApiRef, myPathW);
		}
	}
	else{
		if(*p == L'\\') *p=0;
		wcscat_s(myPathW, MAX_PATH, L"\\Windows");
	}
	return myPathW;
}

void dxwCore::ClearFakeWinDirMapping()
{
	ApiName("ClearFakeWinDirMapping");
	if(!(dxw.dwFlags20 & USEDOSDEVICES)) return;

	if(gFakeWinDriveLetter != 0){
		char rootPath[8];
		sprintf(rootPath, "%c:", gFakeWinDriveLetter);
		OutTraceDW("%s: drive=%s\n", ApiRef, rootPath);
		if(!DefineDosDeviceA(DDD_REMOVE_DEFINITION, rootPath, NULL)){
			OutTrace("%s: DefineDosDeviceA ERROR drive=%s err=%d\n", ApiRef, rootPath, GetLastError());
		}
	}
	if(dxw.FakeCDDrive && (dwFlags10 & FAKECDDRIVE)){
		char rootPath[8];
		sprintf(rootPath, "%c:", dxw.FakeCDDrive);
		OutTraceDW("%s: drive=%s\n", ApiRef, rootPath);
		if(!DefineDosDeviceA(DDD_REMOVE_DEFINITION, rootPath, NULL)){
			OutTrace("%s: DefineDosDeviceA ERROR drive=%s err=%d\n", ApiRef, rootPath, GetLastError());
		}
	}
	if(dxw.FakeHDDrive && (dwFlags10 & FAKEHDDRIVE)){
		char rootPath[8];
		sprintf(rootPath, "%c:", dxw.FakeHDDrive);
		OutTraceDW("%s: drive=%s\n", ApiRef, rootPath);
		if(!DefineDosDeviceA(DDD_REMOVE_DEFINITION, rootPath, NULL)){
			OutTrace("%s: DefineDosDeviceA ERROR drive=%s err=%d\n", ApiRef, rootPath, GetLastError());
		}
	}
}

void dxwCore::CreateDOSDrive(char drive, int type)
{
	ApiName("CreateDOSDrive");
	OutTraceDW("%s: drive letter=%c type=%s\n", ApiRef, drive, type == DXW_FAKE_CD ? "CD" : "HD");
	if(drive != 0){
		char rootPath[8];
		char key[12];
		char FakePath[MAX_PATH];
		char InitPath[MAX_PATH];
		sprintf(rootPath, "%c:", drive);
		sprintf_s(key, sizeof(key), type == DXW_FAKE_CD ? "fakecd%i" : "fakehd%i", dwIndex);
		sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
		//OutTrace("InitPath=%s dxwndpath=%s key=%s\n", InitPath, GetDxWndPath(), key);
		GetPrivateProfileStringA("target", key, NULL, FakePath, MAX_PATH, InitPath);
		if(FakePath[0]=='?'){
			HMODULE hshlwapi = LoadLibrary("shlwapi.dll");
			typedef BOOL (WINAPI *PathRemoveFileSpecA_Type)(LPCSTR);
			PathRemoveFileSpecA_Type pPathRemoveFileSpecA = (PathRemoveFileSpecA_Type)GetProcAddress(hshlwapi, "PathRemoveFileSpecA");
			char sTail[MAX_PATH+1];
			strcpy(sTail, &FakePath[1]);
			sprintf_s(key, sizeof(key), "path%i", dwIndex);
			GetPrivateProfileStringA("target", key, NULL, FakePath, MAX_PATH, InitPath);
			(*pPathRemoveFileSpecA)(FakePath);
			FreeLibrary(hshlwapi);
			strncat(FakePath, sTail, MAX_PATH);
		}
		OutTraceDW("%s: drive=%s type=%s path=\"%s\"\n", ApiRef, rootPath, type == DXW_FAKE_CD ? "CD" : "HD", FakePath);
		if(!DefineDosDeviceA(DDD_FLAGS, rootPath, FakePath)){
			OutTrace("%s: DefineDosDeviceA ERROR drive=%s err=%d\n", ApiRef, rootPath, GetLastError());
		}
		//sprintf(rootPath, "%c:\\", drive);
		//char volume[8];
		//sprintf(volume, "disk%c", drive);
		//if(!SetVolumeMountPoint (rootPath, volume)){
		//	OutTrace("%s: SetVolumeMountPoint ERROR drive=%s volume=%s err=%d\n", ApiRef, rootPath, volume, GetLastError());
		//}
	}
}