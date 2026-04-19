#define  _CRT_SECURE_NO_WARNINGS
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "stdio.h"

typedef DWORD (WINAPI *GetCurrentDirectoryA_Type)(DWORD, LPSTR);
extern GetCurrentDirectoryA_Type pGetCurrentDirectoryA;
typedef DWORD (WINAPI *GetModuleFileNameA_Type)(HMODULE, LPSTR, DWORD);
extern GetModuleFileNameA_Type pGetModuleFileNameA;

void PeekHotKeys()
{
	// peek/remove a WM_SYSKEYDOWN message to trigger the CD changer fkeys
	MSG msg;
	void HandleHotKeys(HWND, UINT, LPARAM, WPARAM);
	// v2.05.27: skip if PeekMessageA is unhooked
	if(pPeekMessageA && (*pPeekMessageA)(&msg, NULL, WM_SYSKEYDOWN, WM_SYSKEYDOWN, TRUE)){
		OutTrace("GetFakeDriverPath: WM_SYSKEYDOWN event hwnd=%#x msg=%#x l/wparam=%#x/%#x\n",
			msg.hwnd, msg.message, msg.lParam, msg.wParam);
		HandleHotKeys(msg.hwnd, msg.message, msg.lParam, msg.wParam);
	}
}

static void dxwGetModuleFileName(char *path, int dwIndex, char *InitPath)
{
	// v2.05.59 fix: do not use current folder, use file path
	//GetCurrentDirectory(MAX_PATH, sCurrentPath); 
	// v2.06.12 fix: in case of otvdmw emulation, get the path from the dxwnd.ini file
	char key[20+1];
	if(dxw.dwFlags19 & ISWIN16EXECUTABLE){
		sprintf_s(key, sizeof(key), "path%i", dwIndex);
		(*pGetPrivateProfileStringA)("target", key, NULL, path, MAX_PATH, InitPath);
	}
	else {
		(*pGetModuleFileNameA)(NULL, path, MAX_PATH);
	}
}

char *dxwGetFakeDriverPath(char DriverId)
{
	static BOOL FirstTimeThrough = TRUE;
	static char *pSuffix;
	static char *FakeHDPath = NULL;
	static char *FakeCDPath = NULL;
	char *ret;

	if(FirstTimeThrough){
		char InitPath[MAX_PATH+1];
		char key[12];
		int dwIndex;
		if(!pGetModuleFileNameA) pGetModuleFileNameA = GetModuleFileNameA;
		HMODULE hshlwapi = LoadLibrary("shlwapi.dll");
		if(!hshlwapi) return "";
		typedef BOOL (WINAPI *PathRemoveFileSpecA_Type)(LPCSTR);
		typedef BOOL (WINAPI *PathFileExistsA_Type)(LPCSTR);
		PathRemoveFileSpecA_Type pPathRemoveFileSpecA = (PathRemoveFileSpecA_Type)(*pGetProcAddress)(hshlwapi, "PathRemoveFileSpecA");
		PathFileExistsA_Type pPathFileExistsA = (PathFileExistsA_Type)(*pGetProcAddress)(hshlwapi, "PathFileExistsA");
		sprintf_s(InitPath, MAX_PATH, "%s\\dxwnd.%s", GetDxWndPath(), (dxw.dwIndex == -1) ? "dxw" : "ini");
		dwIndex = (dxw.dwIndex == -1) ? 0 : dxw.dwIndex;
		//OutTrace("debug: InitPath=%s\n", InitPath);
		if(dxw.dwFlags10 & FAKEHDDRIVE){
			FakeHDPath = (char *)malloc(MAX_PATH+1);
			sprintf_s(key, sizeof(key), "fakehd%i", dwIndex);
			(*pGetPrivateProfileStringA)("target", key, NULL, FakeHDPath, MAX_PATH, InitPath);
			// default for no string is current folder
			if(strlen(FakeHDPath)==0) {
				GetCurrentDirectory(MAX_PATH, FakeHDPath);
			} else {
				// in path, '?' in first position means current folder
				if(FakeHDPath[0]=='?'){
					char sTail[MAX_PATH+1];
					strcpy(sTail, &FakeHDPath[1]);
					dxwGetModuleFileName(FakeHDPath, dwIndex, InitPath);
					(*pPathRemoveFileSpecA)(FakeHDPath);
					strncat(FakeHDPath, sTail, MAX_PATH);
				}
			}
			OutTraceDW("GetFakeDriverPath: HD index=%d drive=%c: path=\"%s\"\n", dwIndex, dxw.FakeHDDrive, FakeHDPath);
		}
		if(dxw.dwFlags10 & FAKECDDRIVE){
			FakeCDPath = (char *)malloc(MAX_PATH+1);
			sprintf_s(key, sizeof(key), "fakecd%i", dwIndex);
			(*pGetPrivateProfileStringA)("target", key, NULL, FakeCDPath, MAX_PATH, InitPath);
			if(strlen(FakeCDPath)==0) {
				GetCurrentDirectory(MAX_PATH, FakeCDPath);
			} else {
				// in path, '?' in first position means current folder
				if(FakeCDPath[0]=='?'){
					char sTail[MAX_PATH+1];
					strcpy(sTail, &FakeCDPath[1]);
					dxwGetModuleFileName(FakeCDPath, dwIndex, InitPath);
					(*pPathRemoveFileSpecA)(FakeCDPath);
					strncat(FakeCDPath, sTail, MAX_PATH);
				}
			}
			OutTraceDW("GetFakeDriverPath: CD index=%d drive=%c:  path=\"%s\"\n", dwIndex, dxw.FakeCDDrive, FakeCDPath);
			pSuffix = &FakeCDPath[strlen(FakeCDPath)];
			dxw.MaxCDVolume = 0;
			if(hshlwapi){
				for(int i=2; i<10; i++){
					char sVolumePath[MAX_PATH+1];
					sprintf_s(sVolumePath, MAX_PATH, "%s%02d", FakeCDPath, i);
					if((*pPathFileExistsA)(sVolumePath))
						dxw.MaxCDVolume = i-1;
					else 
						break;
				}
				FreeLibrary(hshlwapi);
			}
			OutTraceDW("GetFakeDriverPath: CD last disk=CD%02d\n", dxw.MaxCDVolume+1);
		}

		FirstTimeThrough = FALSE;
	}

	switch(DriverId){
		case DXW_HD_PATH: 
			ret = FakeHDPath; 
			break;
		case DXW_CD_PATH: 
			if(dxw.DataCDIndex != GetHookInfo()->CDIndex){
				// v2.06.04: PeekHotKeys() moved inside if clause only when drive has changed
				PeekHotKeys();
				dxw.DataCDIndex = GetHookInfo()->CDIndex;
				// avoid switching to non existing disks
				if(dxw.DataCDIndex > dxw.MaxCDVolume){
					dxw.DataCDIndex = dxw.MaxCDVolume;
					GetHookInfo()->CDIndex = dxw.MaxCDVolume;
				}
				dxw.ShowCDChanger();
			}
			if(dxw.DataCDIndex > 0)
				sprintf(pSuffix, "%02d\\", dxw.DataCDIndex + 1);
			else
				sprintf(pSuffix, "\\");
			ret = FakeCDPath; 
			break;
	}
	// OutTrace("!!! DEBUG id=%d path=%s\n", DriverId, ret);
	return ret;
}

LPCSTR dxwTranslatePathA(LPCSTR lpFileName, DWORD *mapping)
{
	static char sNewPath[MAX_PATH+1];
	char sRootPath[4];
	BOOL hasDriveLetter = FALSE;
	// v2.05.29: bug fix (exception when mapping==NULL & relative & FAKE)
	DWORD dwMapping;

	dwMapping = DXW_NO_FAKE;

	// safeguard
	if(lpFileName == NULL) return NULL;

	if(strlen(lpFileName) > 1)
		if(lpFileName[1]==':') hasDriveLetter = TRUE;

	sNewPath[MAX_PATH]=0;
	if(hasDriveLetter && (strlen(lpFileName) == 2)){
		// implicit, must use the last saved path for that drive
		do{
			char drive = toupper(lpFileName[0]);
			if(drive < 'A') break;
			if(drive > 'Z') break;
			int index = drive - 'A';
			char *p = dxw.CurrDirectories[index];
			//OutTrace("!!! drive=%c path=%s\n", drive, p);
			if(p){
				lpFileName = p;
			}
			else {
				sprintf(sRootPath, "%c:\\", drive);
				lpFileName = sRootPath;
			}
			OutTraceDW("TranslatePath: restored folder=\"%s\"\n", lpFileName);
		} while(FALSE);
	}

	do { // fake loop
		// v2.04.84: find matching for drive name. Do not look for 3 chars (like "E:\") because
		// some games omit the backslash. For instance, "Need for Speed II" looks for "X:NFSW.EXE"
		// Also, take care that drive letters could be in lowercase
		if((dxw.dwFlags10 & FAKEHDDRIVE) && hasDriveLetter && ((char)toupper(lpFileName[0])==dxw.FakeHDDrive)){
			char CurrentPath[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, CurrentPath);
			if(!_strnicmp(lpFileName, CurrentPath, strlen(CurrentPath))) break;
			strncpy(sNewPath, dxwGetFakeDriverPath(DXW_HD_PATH), MAX_PATH);
			strncat(sNewPath, &lpFileName[2], MAX_PATH-strlen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_HD;
			OutTraceDW("TranslatePath: fake HD new FileName=\"%s\"\n", lpFileName);
			break;
		}
		if((dxw.dwFlags10 & FAKEHDDRIVE) && !strncmp(lpFileName, "\\\\.\\", 4)){
			strncpy(sNewPath, dxwGetFakeDriverPath(DXW_HD_PATH), MAX_PATH);
			strncat(sNewPath, &lpFileName[3], MAX_PATH-strlen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_HD;
			OutTraceDW("TranslatePath: fake HD new FileName=\"%s\"\n", lpFileName);
			break;
		}
		if((dxw.dwFlags10 & FAKEHDDRIVE) && (lpFileName[0]=='\\') && (dxw.dwFlags19 & REMAPROOTFOLDER)){
			strncpy(sNewPath, dxwGetFakeDriverPath(DXW_HD_PATH), MAX_PATH);
			strncat(sNewPath, lpFileName, MAX_PATH-strlen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_HD;
			OutTraceDW("TranslatePath: fake HD new FileName=\"%s\"\n", lpFileName);
			break;
		}
		if((dxw.dwFlags10 & FAKECDDRIVE) && hasDriveLetter && ((char)toupper(lpFileName[0])==dxw.FakeCDDrive)){
			strncpy(sNewPath, dxwGetFakeDriverPath(DXW_CD_PATH), MAX_PATH);
			strncat(sNewPath, &lpFileName[2], MAX_PATH-strlen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_CD;
			OutTraceDW("TranslatePath: fake CD new FileName=\"%s\"\n", lpFileName);
			break;
		}
		if((dxw.dwCurrentFolderType != DXW_NO_FAKE) && !hasDriveLetter){
			// v2.05.79: fixed relative path translation on fake drives: the current directory could be
			// a subfolder of the current fake driver, so it's not correct to join the drive letter plus
			// the relative path, you should join the current directory with the relative path!
			// fixes fake CD in "Lords of the Realm".
			//sprintf(sNewPath, "%s\\%s", 
			//	dxwGetFakeDriverPath((dxw.dwCurrentFolderType == DXW_FAKE_HD) ? DXW_HD_PATH : DXW_CD_PATH), 
			//	lpFileName);
			char sCurrentDirectory[MAX_PATH];
			int len = (*pGetCurrentDirectoryA)(MAX_PATH, sCurrentDirectory);
			// v2.05.92 fix: avoid putting the backslash twice: the SetCurrentDirectory operation on a path
			// that terminates with double-backslash fails with errno=2. Found in "3D Scooter Racing".
			// v2.05.93 fix: what happens with backslash should be done also for slash "/"). Found in 
			// "Token For Yama", a Chinese game. 
			if(lpFileName && ((lpFileName[0]=='\\') || (lpFileName[0]=='/'))){
				char driveType = (dxw.dwCurrentFolderType == DXW_FAKE_CD) ? DXW_CD_PATH : DXW_HD_PATH;
				sprintf(sNewPath, "%s%s", dxwGetFakeDriverPath(driveType), lpFileName); 
			}
			else {
				sprintf(sNewPath, "%s\\%s", sCurrentDirectory, lpFileName); 
			}
			lpFileName = sNewPath;
			dwMapping = dxw.dwCurrentFolderType;
			OutTraceDW("TranslatePath: fake CD relative FileName=\"%s\"\n", lpFileName);
		}
	} while(FALSE);
	//OutDebugDW("TranslatePath: mapping=%d FileName=\"%s\"\n", 
	//	mapping ? *mapping : 0,
	//	lpFileName);
	if(mapping) *mapping = dwMapping;
	return lpFileName;
}

LPCWSTR dxwTranslatePathW(LPCWSTR lpFileName, DWORD *mapping)
{
	static WCHAR sNewPath[MAX_PATH+1];
	WCHAR sRootPath[MAX_PATH+1];
	BOOL hasDriveLetter = FALSE;
	size_t converted = 0;
	// v2.05.29: alignement with ASCII routine
	DWORD dwMapping;

	dwMapping = DXW_NO_FAKE;

	// safeguard
	if(lpFileName == NULL) return NULL;

	if(wcslen(lpFileName) > 1)
		if(lpFileName[1]==L':') hasDriveLetter = TRUE;

	sNewPath[MAX_PATH]=0;
	if(hasDriveLetter && (wcslen(lpFileName) == 2)){
		// implicit, must use the last saved path for that drive
		do{
			WCHAR drive = towupper(lpFileName[0]);
			if(drive < L'A') break;
			if(drive > L'Z') break;
			int index = drive - L'A';
			char *p = dxw.CurrDirectories[index];
			//OutTrace("!!! drive=%c path=%s\n", drive, p);
			if(p){
				swprintf(sRootPath, MAX_PATH, L"%s:\\", p);
			}
			else {
				swprintf(sRootPath, 4, L"%c:\\", drive);
			}
			lpFileName = sRootPath;
			OutTraceDW("TranslatePath: restored folder=\"%ls\"\n", lpFileName);
		} while(FALSE);
	}

	do { // fake loop
		// v2.04.84: find matching for drive name. Do not look for 3 chars (like "E:\") because
		// some games omit the backslash. For instance, "Need for Speed II" looks for "X:NFSW.EXE"
		// Also, take care that drive letters could be in lowercase
		if((dxw.dwFlags10 & FAKEHDDRIVE) && (lpFileName[1]==L':') && ((char)toupper((char)lpFileName[0])==dxw.FakeHDDrive)){
			mbstowcs_s(&converted, sNewPath, MAX_PATH, dxwGetFakeDriverPath(DXW_HD_PATH), _TRUNCATE); 
			wcsncat(sNewPath, &lpFileName[2], MAX_PATH-wcslen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_HD;
			OutTraceDW("TranslatePath: fake HD new FileName=\"%ls\"\n", lpFileName);
			break;
		}
		if((dxw.dwFlags10 & FAKEHDDRIVE) && !wcsncmp(lpFileName, L"\\\\.\\", 4)){
			mbstowcs_s(&converted, sNewPath, MAX_PATH, dxwGetFakeDriverPath(DXW_HD_PATH), _TRUNCATE); 
			wcsncat(sNewPath, &lpFileName[3], MAX_PATH-wcslen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_HD;
			OutTraceDW("TranslatePath: fake HD new FileName=\"%ls\"\n", lpFileName);
			break;
		}
		if((dxw.dwFlags10 & FAKEHDDRIVE) && (lpFileName[0]==L'\\') && (dxw.dwFlags19 & REMAPROOTFOLDER)){
			mbstowcs_s(&converted, sNewPath, MAX_PATH, dxwGetFakeDriverPath(DXW_HD_PATH), _TRUNCATE); 
			wcsncat(sNewPath, &lpFileName[3], MAX_PATH-wcslen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_HD;
			OutTraceDW("TranslatePath: fake HD new FileName=\"%ls\"\n", lpFileName);
			break;
		}
		if((dxw.dwFlags10 & FAKECDDRIVE) && (lpFileName[1]==L':') && ((char)toupper((char)lpFileName[0])==dxw.FakeCDDrive)){
			mbstowcs_s(&converted, sNewPath, MAX_PATH, dxwGetFakeDriverPath(DXW_CD_PATH), _TRUNCATE); 
			wcsncat(sNewPath, &lpFileName[2], MAX_PATH-wcslen(sNewPath));
			lpFileName = sNewPath;
			dwMapping = DXW_FAKE_CD;
			OutTraceDW("TranslatePath: fake CD new FileName=\"%ls\"\n", lpFileName);
			break;
		}
		if((dxw.dwCurrentFolderType != DXW_NO_FAKE) && !hasDriveLetter){
			// v2.05.79: fixed relative path translation on fake drives: the current directory could be
			// a subfolder of the current fake driver, so it's not correct to join the drive letter plus
			// the relative path, you should join the current directory with the relative path!
			//mbstowcs_s(&converted, sNewPath, MAX_PATH, dxwGetFakeDriverPath((dxw.dwCurrentFolderType == DXW_FAKE_HD) ? DXW_HD_PATH : DXW_CD_PATH), _TRUNCATE); 
			//wcsncat(sNewPath, lpFileName, MAX_PATH-wcslen(sNewPath)); // v2.05.53 fix (ref. "Jack Carlton's Soccer Nation")
			typedef DWORD (WINAPI *GetCurrentDirectoryW_Type)(DWORD, LPWSTR);
			extern GetCurrentDirectoryW_Type pGetCurrentDirectoryW;
			WCHAR sCurrentDirectory[MAX_PATH];
			int len = (*pGetCurrentDirectoryW)(MAX_PATH, sCurrentDirectory);
			// v2.05.92 fix: avoid putting the backslash twice: the SetCurrentDirectory operation on a path
			// that terminates with double-backslash fails with errno=2. 
			// v2.05.93 fix: same fix as in ASCII version dxwTranslatePathA
			if(lpFileName && ((lpFileName[0]==L'\\') || (lpFileName[0]==L'/'))){
				char driveType = (dxw.dwCurrentFolderType == DXW_FAKE_CD) ? DXW_CD_PATH : DXW_HD_PATH;
				wsprintfW(sNewPath, L"%s%s", dxwGetFakeDriverPath(driveType), lpFileName); 
			}
			else
				wsprintfW(sNewPath, L"%s\\%s", sCurrentDirectory, lpFileName); 
			lpFileName = sNewPath;
			dwMapping = dxw.dwCurrentFolderType;
			OutTraceDW("TranslatePath: fake CD relative FileName=\"%ls\"\n", lpFileName);
		}
	} while(FALSE);
	//OutDebugDW("TranslatePath: mapping=%d FileName=\"%ls\"\n", 
	//	mapping ? *mapping : 0,
	//	lpFileName);
	if(mapping) *mapping = dwMapping;
	return lpFileName;
}

LPCSTR dxwUntranslatePathA(LPCSTR lpFileName, DWORD *mapping)
{
	static char sNewPath[MAX_PATH+1];
	BOOL hasDriveLetter = FALSE;
	DWORD dwMapping;

	dwMapping = DXW_NO_FAKE;

	// safeguard
	if(lpFileName == NULL) return NULL;

	if(strlen(lpFileName) > 1)
		if(lpFileName[1]==':') hasDriveLetter = TRUE;

	sNewPath[MAX_PATH]=0;
	do { // fake loop
		// find matching for drive name. Here you can look for 3 chars (like "E:\") because
		// a translated path always has it.
		// make a string compare for 1 character less becuse of the ending backslash '\'
		if((dxw.dwFlags10 & FAKEHDDRIVE) && hasDriveLetter){
			char *lpFakeHDPath = dxwGetFakeDriverPath(DXW_HD_PATH);
			if(strncmp(lpFileName, lpFakeHDPath, strlen(lpFakeHDPath)-1) == 0){
				sprintf(sNewPath, "%c:\\", dxw.FakeHDDrive);
				if(strlen(lpFakeHDPath) < strlen(lpFileName)) strcat(sNewPath, lpFileName + strlen(lpFakeHDPath)); // v2.05.59 fix
				lpFileName = sNewPath;
				dwMapping = DXW_FAKE_HD;
				OutTraceDW("UntranslatePath: fake HD new FileName=\"%s\"\n", lpFileName);
			}
			break;
		}
		if((dxw.dwFlags10 & FAKECDDRIVE) && hasDriveLetter){
			char *lpFakeCDPath = dxwGetFakeDriverPath(DXW_CD_PATH);
			if(strncmp(lpFileName, lpFakeCDPath, strlen(lpFakeCDPath)-1) == 0){
				sprintf(sNewPath, "%c:\\", dxw.FakeCDDrive);
				if(strlen(lpFakeCDPath) < strlen(lpFileName)) strcat(sNewPath, lpFileName + strlen(lpFakeCDPath)); // v2.05.59 fix
				lpFileName = sNewPath;
				dwMapping = DXW_FAKE_CD;
				OutTraceDW("UntranslatePath: fake CD new FileName=\"%s\"\n", lpFileName);
			}
		}
	} while(FALSE);
	//OutDebugDW("TranslatePathA: mapping=%d FileName=\"%s\"\n", 
	//	mapping ? *mapping : 0,
	//	lpFileName);
	if(mapping) *mapping = dwMapping;
	return lpFileName;
}

LPWSTR dxwUntranslatePathW(LPWSTR lpFileName, DWORD *mapping)
{
	static WCHAR sNewPath[MAX_PATH+1];
	BOOL hasDriveLetter = FALSE;
	DWORD dwMapping;

	dwMapping = DXW_NO_FAKE;

	// safeguard
	if(lpFileName == NULL) return NULL;

	if(wcslen(lpFileName) > 1)
		if(lpFileName[1]==L':') hasDriveLetter = TRUE;

	sNewPath[MAX_PATH]=0;
	do { // fake loop
		// v2.04.84: find matching for drive name. Here you can look for 3 chars (like "E:\") because
		// a translated path always has it
		if((dxw.dwFlags10 & FAKEHDDRIVE) && hasDriveLetter){
			char *lpFakeHDPath = dxwGetFakeDriverPath(DXW_HD_PATH);
			WCHAR FakeHDPathW[MAX_PATH+1];
			size_t len;
			mbstowcs_s(&len, (LPWSTR)FakeHDPathW, strlen(lpFakeHDPath), lpFakeHDPath, MAX_PATH);
			if(wcsncmp(lpFileName, (LPWSTR)FakeHDPathW, strlen(lpFakeHDPath)-1) == 0){
				swprintf((LPWSTR)sNewPath, MAX_PATH, L"%c:\\", dxw.FakeHDDrive);
				wcscat((LPWSTR)sNewPath, lpFileName + strlen(lpFakeHDPath));
				lpFileName = (LPWSTR)sNewPath;
				dwMapping = DXW_FAKE_HD;
				OutTraceDW("UntranslatePath: fake HD new FileName=\"%ls\"\n", lpFileName);
			}
			break;
		}
		if((dxw.dwFlags10 & FAKECDDRIVE) && hasDriveLetter){
			char *lpFakeCDPath = dxwGetFakeDriverPath(DXW_CD_PATH);
			WCHAR FakeCDPathW[MAX_PATH+1];
			size_t len;
			// v2.05.58 fixes to avoid crash. Found & fixed for "Petka 3"
			//mbstowcs_s(&len, (LPWSTR)&FakeCDPathW[0], strlen(lpFakeCDPath), lpFakeCDPath, MAX_PATH);
			mbstowcs_s(&len, (LPWSTR)FakeCDPathW, MAX_PATH, lpFakeCDPath, _TRUNCATE);
			if(wcsncmp(lpFileName, (LPWSTR)FakeCDPathW, strlen(lpFakeCDPath)-1) == 0){ // v2.06.13 fix
				swprintf((LPWSTR)sNewPath, MAX_PATH, L"%c:\\", dxw.FakeCDDrive);
				wcscat((LPWSTR)sNewPath, lpFileName + strlen(lpFakeCDPath));
				lpFileName = (LPWSTR)sNewPath;
				dwMapping = DXW_FAKE_CD;
				OutTraceDW("UntranslatePath: fake CD new FileName=\"%ls\"\n", lpFileName);
			}
		}
	} while(FALSE);
	//OutDebugDW("UntranslatePathW: mapping=%d FileName=\"%ls\"\n", 
	//	mapping ? *mapping : 0,
	//	lpFileName);
	if(mapping) *mapping = dwMapping;
	return lpFileName;
}

DWORD dxwVirtualDriveTypeA(LPCSTR lpPath)
{
	char DriveLetter = toupper(lpPath[0]);
	if(DriveLetter==dxw.FakeHDDrive) return DXW_FAKE_HD;
	if(DriveLetter==dxw.FakeCDDrive) return DXW_FAKE_CD;
	return DXW_NO_FAKE;
}

DWORD dxwVirtualDriveTypeW(LPCWSTR lpPath)
{
	char DriveLetter;
	wctomb(&DriveLetter, lpPath[0]);
	if(DriveLetter==dxw.FakeHDDrive) return DXW_FAKE_HD;
	if(DriveLetter==dxw.FakeCDDrive) return DXW_FAKE_CD;
	return DXW_NO_FAKE;
}
