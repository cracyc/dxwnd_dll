#define  _CRT_SECURE_NO_WARNINGS

#define HANDLEFAKERAWDEVICE
#define DXWESCAPEDINIENTRIES

#define _MODULE "kernel32"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "dxwlocale.h"
#include "stdio.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "kernel32.profiles.h"

#ifndef DXW_NOTRACES
#define TraceError() OutErrorSYS("%s: ERROR err=%d\n", ApiRef, GetLastError())
#define IfTraceError() if(!res) TraceError()
#else
#define TraceError()
#define IfTraceError()
#endif

#ifdef DXWESCAPEDINIENTRIES
extern BOOL GetTargetDirA(char *, int);
extern BOOL GetTargetDirW(WCHAR *, int);
extern BOOL isRelativePathA(LPCSTR);
extern BOOL isRelativePathW(LPCWSTR);
extern char *GetFakeWinDirA();
extern WCHAR *GetFakeWinDirW();

static BOOL unescapeA(char *s, LPCSTR lpFileName, UINT size)
{
	BOOL updated = FALSE;
	char buffer[MAX_PATH];
	GetTargetDirA(buffer, MAX_PATH);
	// do not process ini files outside the target folder
	if(_strnicmp(lpFileName, buffer, strlen(buffer))) return FALSE;
	// when run by WineVDM do not process the "otvdm.ini" file
	if(dxw.isWineControlled && strstr(lpFileName, "otvdm.ini")) return FALSE;

	char *se = (char *)malloc(size + MAX_PATH + 1);
	memset(se, 0, size); //
	char *pi = s;
	char *pe = se;
	for(UINT i=0; (i<strlen(s)) && (i<size); i++){
		if(*pi=='\\'){
			updated = TRUE;
			pi++;
			if(*pi == '{'){
				if(!strncmp(pi, "{path}", strlen("{path}"))) {
					char sTokenValue[MAX_PATH];
					GetTargetDirA(sTokenValue, MAX_PATH);
					int iTokenLength = strlen(sTokenValue);
					OutTraceDW("REPLACED {path} val=\"%s\" len=%d\n", sTokenValue, iTokenLength);
					strcpy(pe, sTokenValue);
					pe += iTokenLength;
					pi += strlen("{path}");
				}
				else if(!strncmp(pi, "{work}", strlen("{work}"))) {
					char sTokenValue[MAX_PATH];
					(*pGetCurrentDirectoryA)(MAX_PATH, sTokenValue);
					int iTokenLength = strlen(sTokenValue);
					OutTraceDW("REPLACED {work} val=\"%s\" len=%d\n", sTokenValue, iTokenLength);
					strcpy(pe, sTokenValue);
					pe += iTokenLength;
					pi += strlen("{work}");
				}
			}
			else {
				*pe = *pi;																																						;
				pe++;
				pi++;
			}
		}
		else {
			*pe = *pi;
			pe++;
			pi++;
		}
		if(strlen(se) > size) {
			break;
		}
	}
	*pe = 0;
	strncpy(s, se, size);
	free(se);
	return updated;
}

static BOOL unescapeW(WCHAR *s, LPCWSTR lpFileName, UINT size)
{
	BOOL updated = FALSE;
	WCHAR buffer[MAX_PATH];
	GetTargetDirW(buffer, MAX_PATH);
	// do not process ini files outside the target folder
	if(_wcsnicmp(lpFileName, buffer, wcslen(buffer))) return FALSE;
	// when run by WineVDM do not process the "otvdm.ini" file
	if(dxw.isWineControlled && wcsstr(lpFileName, L"otvdm.ini")) return FALSE;

	WCHAR *se = (WCHAR *)malloc((size + MAX_PATH + 1) * sizeof(WCHAR));
	memset(se, 0, size * sizeof(WCHAR)); //
	WCHAR *pi = s;
	WCHAR *pe = se;
	for(UINT i=0; (i<wcslen(s)) && (i<size); i++){
		if(*pi==L'\\'){
			updated = TRUE;
			pi++;
			if(*pi == L'{'){
				if(!wcsncmp(pi, L"{path}", wcslen(L"{path}"))) {
					WCHAR sTokenValue[MAX_PATH];
					GetTargetDirW(sTokenValue, MAX_PATH);
					int iTokenLength = wcslen(sTokenValue);
					OutTraceDW("REPLACED {path} val=\"%Ls\" len=%d\n", sTokenValue, iTokenLength);
					wcscpy(pe, sTokenValue);
					pe += iTokenLength;
					pi += wcslen(L"{path}");
				}
				else if(!wcsncmp(pi, L"{work}", wcslen(L"{work}"))) {
					WCHAR sTokenValue[MAX_PATH];
					(*pGetCurrentDirectoryW)(MAX_PATH, sTokenValue);
					int iTokenLength = wcslen(sTokenValue);
					OutTraceDW("REPLACED {work} val=\"%Ls\" len=%d\n", sTokenValue, iTokenLength);
					wcscpy(pe, sTokenValue);
					pe += iTokenLength;
					pi += wcslen(L"{work}");
				}
			}
			else {
				*pe = *pi;																																						;
				pe++;
				pi++;
			}
		}
		else {
			*pe = *pi;
			pe++;
			pi++;
		}
		if(wcslen(se) > size) {
			break;
		}
	}
	*pe = 0;
	wcsncpy(s, se, size);
	free(se);
	return updated;
}
#endif //DXWESCAPEDINIENTRIES

static char *RemapIniPathA(LPCSTR lpFileName, char *lpNewPath)
{
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
	return lpNewPath;
}

static WCHAR *RemapIniPathW(LPCWSTR lpFileName, WCHAR *lpNewPath)
{
	wcscpy(lpNewPath, GetFakeWinDirW());
	wcscat(lpNewPath, L"\\");
	// v2.06.08: neutralize registry remapping for Control.ini, System.ini and Winfile.ini
	if(!_wcsicmp(lpFileName, L"Control.ini")) wcscat(lpNewPath, L".Control.ini");
	else
	if(!_wcsicmp(lpFileName, L"System.ini")) wcscat(lpNewPath, L".System.ini");
	else
	if(!_wcsicmp(lpFileName, L"Winfile.ini")) wcscat(lpNewPath, L".Winfile.ini");
	else
	wcscat(lpNewPath, lpFileName ? lpFileName : L"Win.ini");
	return lpNewPath;
}

// ===== wrappers ===== //

UINT WINAPI extGetPrivateProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName)
{
	DWORD ret;
	ApiName("GetPrivateProfileIntA");
	char lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" default=%d filename=\"%s\"\n", ApiRef, lpAppName, lpKeyName, nDefault, lpFileName);
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathA(lpFileName)){
		lpFileName = RemapIniPathA(lpFileName, lpNewPath);
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
		lpFileName = RemapIniPathW(lpFileName, lpNewPath);
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

DWORD WINAPI extGetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
	DWORD ret;
	ApiName("GetPrivateProfileStringA");
	char lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=%s key=%s default=\"%s\" size=%d filename=\"%s\"\n", ApiRef, lpAppName, lpKeyName, lpDefault, nSize, lpFileName);
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathA(lpFileName)){
		lpFileName = RemapIniPathA(lpFileName, lpNewPath);
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpFileName);
	}
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpFileName) {
		lpFileName = dxwTranslatePathA(lpFileName, NULL);
	}
	ret = (*pGetPrivateProfileStringA)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
	OutTraceSYS("%s: ret=%d retstring=\"%s\"\n", ApiRef, ret, lpReturnedString);

#ifdef DXWESCAPEDINIENTRIES
	if((dxw.dwFlags20 & ESCAPEDINIFILES) && ret){
		if(strcmp(lpReturnedString, lpDefault)){
			if(unescapeA(lpReturnedString, lpFileName, nSize)){
				ret = strlen(lpReturnedString);
				OutTrace("%s: UNESCAPED ret=%d retstring=\"%s\"\n", ApiRef, ret, lpReturnedString);
			}
		}
	}
#endif // DXWESCAPEDINIENTRIES
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
		lpFileName = RemapIniPathW(lpFileName, lpNewPath);
		OutTraceDW("%s: remapped path=\"%ls\"\n", ApiRef, lpFileName);
	}
	// v2.05.19: safeguard against possibly NULL lpFileName
	if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) && lpFileName) {
		lpFileName = dxwTranslatePathW(lpFileName, NULL);
	}
	ret = (*pGetPrivateProfileStringW)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
	OutTraceSYS("%s: ret=%#x retstring=\"%ls\"\n", ApiRef, ret, lpReturnedString);

#ifdef DXWESCAPEDINIENTRIES
	if((dxw.dwFlags20 & ESCAPEDINIFILES) && ret){
		if(wcscmp(lpReturnedString, lpDefault)){
			if(unescapeW(lpReturnedString, lpFileName, nSize)){
				ret = wcslen(lpReturnedString);
				OutTrace("%s: UNESCAPED ret=%d retstring=\"%ls\"\n", ApiRef, ret, lpReturnedString);
			}
		}
	}
#endif // DXWESCAPEDINIENTRIES
	return ret;
}

UINT WINAPI extGetProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault)
{
	DWORD ret;
	ApiName("GetProfileIntA");
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" default=%d\n", ApiRef, lpAppName, lpKeyName, nDefault);
	if(dxw.dwFlags11 & REMAPSYSFOLDERS){
		char lpNewPath[MAX_PATH];
		RemapIniPathA(NULL, lpNewPath);
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpNewPath);
		ret = (*pGetPrivateProfileIntA)(lpAppName, lpKeyName, nDefault, lpNewPath);
	}
	else {
		ret = (*pGetProfileIntA)(lpAppName, lpKeyName, nDefault);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

UINT WINAPI extGetProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault)
{
	DWORD ret;
	ApiName("GetProfileIntW");
	if(dxw.dwFlags11 & REMAPSYSFOLDERS){
		WCHAR lpNewPath[MAX_PATH];
		RemapIniPathW(NULL, lpNewPath);
		OutTraceDW("%s: remapped path=\"%ls\"\n", ApiRef, lpNewPath);
		ret = (*pGetPrivateProfileIntW)(lpAppName, lpKeyName, nDefault, lpNewPath);
	}
	else {
		ret = (*pGetProfileIntW)(lpAppName, lpKeyName, nDefault);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

DWORD WINAPI extGetProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize)
{
	DWORD ret;
	ApiName("GetProfileStringA");
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" default=\"%s\" size=%d\n", ApiRef, lpAppName, lpKeyName, lpDefault, nSize);
	if(dxw.dwFlags11 & REMAPSYSFOLDERS){
		char lpNewPath[MAX_PATH];
		RemapIniPathA(NULL, lpNewPath);
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpNewPath);
		ret = (*pGetPrivateProfileStringA)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpNewPath);
#ifdef DXWESCAPEDINIENTRIES
		if((dxw.dwFlags20 & ESCAPEDINIFILES) && ret){
			if(strcmp(lpReturnedString, lpDefault)){
				if(unescapeA(lpReturnedString, lpNewPath, nSize)){
					ret = strlen(lpReturnedString);
					OutTrace("%s: UNESCAPED ret=%d retstring=\"%s\"\n", ApiRef, ret, lpReturnedString);
				}
			}
		}
#endif // DXWESCAPEDINIENTRIES
	}
	else {
		ret = (*pGetProfileStringA)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize);
	}
	OutTraceSYS("%s: ret=%#x retstring=\"%s\"\n", ApiRef, ret, lpReturnedString);

	return ret;
}

DWORD WINAPI extGetProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize)
{
	DWORD ret;
	ApiName("GetProfileStringW");
	OutTraceSYS("%s: app=\"%ls\" key=\"%ls\" default=\"%ls\" size=%d\n", ApiRef, lpAppName, lpKeyName, lpDefault, nSize);
	if(dxw.dwFlags11 & REMAPSYSFOLDERS){
		WCHAR lpNewPath[MAX_PATH];
		RemapIniPathW(NULL, lpNewPath);
		OutTraceDW("%s: remapped path=\"%ls\"\n", ApiRef, lpNewPath);
		ret = (*pGetPrivateProfileStringW)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpNewPath);
#ifdef DXWESCAPEDINIENTRIES
		if((dxw.dwFlags20 & ESCAPEDINIFILES) && ret){
			if(wcscmp(lpReturnedString, lpDefault)){
				if(unescapeW(lpReturnedString, lpNewPath, nSize)){
					ret = wcslen(lpReturnedString);
					OutTrace("%s: UNESCAPED ret=%d retstring=\"%ls\"\n", ApiRef, ret, lpReturnedString);
				}
			}
		}
#endif // DXWESCAPEDINIENTRIES
	}
	else {
		ret = (*pGetProfileStringW)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize);
	}
	OutTraceSYS("%s: ret=%#x retstring=\"%ls\"\n", ApiRef, ret, lpReturnedString);
	return ret;
}

DWORD WINAPI extWritePrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPSTR lpString, LPCSTR lpFileName)
{
	DWORD ret;
	ApiName("WritePrivateProfileStringA");
	char lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%s\" key=\"%s\" val=\"%s\" filename=\"%s\"\n", ApiRef, lpAppName, lpKeyName, lpString, lpFileName);
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathA(lpFileName)){
		lpFileName = RemapIniPathA(lpFileName, lpNewPath);
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpFileName);
	}
	//if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
	//	DWORD mapping;
	//	lpFileName = dxwTranslatePathA(lpFileName, &mapping);
	//	// what if we try to write on a CD ini file?
	//	if(mapping == DXW_FAKE_CD) return TRUE; 
	//}
	ret = (*pWritePrivateProfileStringA)(lpAppName, lpKeyName, lpString, lpFileName);
	if(!ret) OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

DWORD WINAPI extWritePrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPWSTR lpString, LPCWSTR lpFileName)
{
	DWORD ret;
	ApiName("WritePrivateProfileStringW");
	WCHAR lpNewPath[MAX_PATH];
	OutTraceSYS("%s: app=\"%ls\" key=\"%ls\" val=\"%ls\" filename=\"%ls\"\n", ApiRef, lpAppName, lpKeyName, lpString, lpFileName);
	if((dxw.dwFlags11 & REMAPSYSFOLDERS) && isRelativePathW(lpFileName)){
		lpFileName = RemapIniPathW(lpFileName, lpNewPath);
		OutTraceDW("%s: remapped path=\"%ls\"\n", ApiRef, lpFileName);
	}
	//if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
	//	DWORD mapping;
	//	lpFileName = dxwTranslatePathW(lpFileName, &mapping);
	//	// what if we try to write on a CD ini file?
	//	if(mapping == DXW_FAKE_CD) return TRUE; 
	//}
	ret = (*pWritePrivateProfileStringW)(lpAppName, lpKeyName, lpString, lpFileName);
	if(!ret) OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

DWORD WINAPI extWriteProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPSTR lpString)
{
	DWORD ret;
	ApiName("WriteProfileStringA");

	if(dxw.dwFlags11 & REMAPSYSFOLDERS){
		char lpNewPath[MAX_PATH];
		RemapIniPathA(NULL, lpNewPath);
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpNewPath);
		ret = (*pWritePrivateProfileStringA)(lpAppName, lpKeyName, lpString, lpNewPath);
	}
	else {
		ret = (*pWriteProfileStringA)(lpAppName, lpKeyName, lpString);
	}
	if(!ret) OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

DWORD WINAPI extWriteProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPWSTR lpString)
{
	DWORD ret;
	ApiName("WriteProfileStringW");

	if(dxw.dwFlags11 & REMAPSYSFOLDERS){
		WCHAR lpNewPath[MAX_PATH];
		RemapIniPathW(NULL, lpNewPath);
		OutTraceDW("%s: remapped path=\"%s\"\n", ApiRef, lpNewPath);
		ret = (*pWritePrivateProfileStringW)(lpAppName, lpKeyName, lpString, lpNewPath);
	}
	else {
		ret = (*pWriteProfileStringW)(lpAppName, lpKeyName, lpString);
	}
	if(!ret) OutTraceSYS("%s: ERROR err=%d\n", ApiRef, GetLastError());
	return ret;
}

