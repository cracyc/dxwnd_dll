#define _MODULE "shell32"

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include <shellapi.h>
#include <shlobj.h>
#include "shobjidl.h"

//#define TRACEALL
//#define EXPERIMENTAL_ENABLED
//#define ENABLE_MARKERS

extern void HookDlls(HMODULE);

#ifndef PIDLIST_ABSOLUTE
#define PIDLIST_ABSOLUTE VOID
#endif

typedef HINSTANCE (WINAPI *ShellExecuteA_Type)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
ShellExecuteA_Type pShellExecuteA;
HINSTANCE WINAPI extShellExecuteA(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
typedef HINSTANCE (WINAPI *ShellExecuteW_Type)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
ShellExecuteW_Type pShellExecuteW;
HINSTANCE WINAPI extShellExecuteW(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
typedef BOOL (WINAPI *ShellExecuteExA_Type)(SHELLEXECUTEINFOA *);
ShellExecuteExA_Type pShellExecuteExA;
BOOL WINAPI extShellExecuteExA(SHELLEXECUTEINFOA *);
typedef BOOL (WINAPI *ShellExecuteExW_Type)(SHELLEXECUTEINFOW *);
ShellExecuteExW_Type pShellExecuteExW = 0;
BOOL WINAPI extShellExecuteExW(SHELLEXECUTEINFOW *);

#ifdef TRACEALL
typedef HRESULT (WINAPI *SHCoCreateInstance_Type)(PCWSTR, const CLSID *, IUnknown *, REFIID, void **);
SHCoCreateInstance_Type pSHCoCreateInstance;
HRESULT WINAPI extSHCoCreateInstance(PCWSTR, const CLSID *, IUnknown *, REFIID, void **);
typedef int (WINAPI *SHFileOperationA_Type)(LPSHFILEOPSTRUCTA);
SHFileOperationA_Type pSHFileOperationA;
int WINAPI extSHFileOperationA(LPSHFILEOPSTRUCTA);
typedef HRESULT (WINAPI *SHGetSpecialFolderLocation_Type)(HWND, int, PIDLIST_ABSOLUTE *);
SHGetSpecialFolderLocation_Type pSHGetSpecialFolderLocation;
HRESULT WINAPI extSHGetSpecialFolderLocation(HWND, int, PIDLIST_ABSOLUTE *);
typedef PIDLIST_ABSOLUTE (WINAPI *SHBrowseForFolderA_Type)(LPBROWSEINFOA);
SHBrowseForFolderA_Type pSHBrowseForFolderA;
PIDLIST_ABSOLUTE WINAPI extSHBrowseForFolderA(LPBROWSEINFOA);
typedef HINSTANCE (WINAPI *FindExecutableA_Type)(LPCSTR, LPCSTR, LPSTR);
FindExecutableA_Type pFindExecutableA;
HINSTANCE WINAPI extFindExecutableA(LPCSTR, LPCSTR, LPSTR);
#endif // TRACEALL

static HookEntryEx_Type Hooks[]={
#ifdef TRACEALL
	{HOOK_HOT_CANDIDATE, 0, "SHCoCreateInstance", (FARPROC)NULL, (FARPROC *)&pSHCoCreateInstance, (FARPROC)extSHCoCreateInstance},
	{HOOK_HOT_CANDIDATE, 0, "SHFileOperationA", (FARPROC)NULL, (FARPROC *)&pSHFileOperationA, (FARPROC)extSHFileOperationA},
	{HOOK_HOT_CANDIDATE, 0, "SHGetSpecialFolderLocation", (FARPROC)NULL, (FARPROC *)&pSHGetSpecialFolderLocation, (FARPROC)extSHGetSpecialFolderLocation},
	{HOOK_HOT_CANDIDATE, 0, "SHBrowseForFolderA", (FARPROC)NULL, (FARPROC *)&pSHBrowseForFolderA, (FARPROC)extSHBrowseForFolderA},
	{HOOK_HOT_CANDIDATE, 0, "FindExecutableA", (FARPROC)NULL, (FARPROC *)&pFindExecutableA, (FARPROC)extFindExecutableA},
#endif // TRACEALL
	{HOOK_HOT_CANDIDATE, 0, "ShellExecuteA", (FARPROC)NULL, (FARPROC *)&pShellExecuteA, (FARPROC)extShellExecuteA},
	{HOOK_HOT_CANDIDATE, 0, "ShellExecuteW", (FARPROC)NULL, (FARPROC *)&pShellExecuteW, (FARPROC)extShellExecuteW},
	{HOOK_HOT_CANDIDATE, 0, "ShellExecuteExA", (FARPROC)NULL, (FARPROC *)&pShellExecuteExA, (FARPROC)extShellExecuteExA},
	{HOOK_HOT_CANDIDATE, 0, "ShellExecuteExW", (FARPROC)NULL, (FARPROC *)&pShellExecuteExW, (FARPROC)extShellExecuteExW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookShell32(HMODULE module)
{
	HookLibraryEx(module, Hooks, "shell32.dll");
}

FARPROC Remap_Shell32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

#define SHELL_RETCODE_OK (SE_ERR_DLLNOTFOUND+1)

static BOOL isAbsolutePathA(LPCSTR path)
{
	// boolean, returns true if the pathname is absolute looking at the 
	// initial slash/backslash or the presence of a drive letter "x:"
	int len;
	char slash;
	if(!path) return FALSE;
	len = strlen(path);
	if(len == 0) return FALSE;
	slash = path[0];
	if((slash == '\\') || (slash == '/')) return TRUE;
	if((len >= 3) && (path[1] == ':')){
		slash = path[2];
		if((slash == '\\') || (slash == '/')) return TRUE;
	}
	return FALSE;
}

typedef int (WINAPI *MultiByteToWideChar_Type)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
extern MultiByteToWideChar_Type pMultiByteToWideChar;

LPCWSTR Ascii2Wide(LPCSTR lpstr)
{
	if(!lpstr) return NULL;
	int size = strlen(lpstr);
	if(size == 0) return NULL;
	WCHAR *lpstrW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
	int n = (*pMultiByteToWideChar)(dxw.CodePage, MB_PRECOMPOSED , lpstr, size, lpstrW, size);
	if(n == 0) OutTrace("Ascii2Wide str=\"%s\" size=%d n=%d error=%d\n", lpstr, size, n, GetLastError());
	lpstrW[n] = L'\0'; // make tail ! 
	return lpstrW;
}

HINSTANCE WINAPI extShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	HINSTANCE res;
	ApiName("ShellExecuteA");
	if(IsTraceSYS){
		OutTrace("%s:\n", ApiRef);
		OutTrace("> hwnd=%#x\n", hwnd);
		OutTrace("> operation=%s\n", lpOperation);
		OutTrace("> file=%s\n", lpFile);
		OutTrace("> params=%s\n", lpParameters);
		OutTrace("> directory=%s\n", lpDirectory);
		OutTrace("> show=%d\n", nShowCmd);
	}
	if(dxw.dwFlags14 & NOSHELLEXECUTE) {
		OutTraceSYS("%s: NOSHELLEXECUTE res=%#x\n", ApiRef, SHELL_RETCODE_OK);
		return (HINSTANCE)SHELL_RETCODE_OK;
	}

	if((dxw.dwFlags11 & CUSTOMLOCALE) && (dxw.dwFlags12 & PATHLOCALE)){
		LPCWSTR lpOperationW, lpFileW, lpParametersW, lpDirectoryW;
		lpOperationW = Ascii2Wide(lpOperation);
		lpFileW = Ascii2Wide(lpFile);
		lpParametersW = Ascii2Wide(lpParameters);
		lpDirectoryW = Ascii2Wide(lpDirectory);
		if(!pShellExecuteW) pShellExecuteW = ShellExecuteW;
		res = (*pShellExecuteW)(hwnd, lpOperationW, lpFileW, lpParametersW, lpDirectoryW, nShowCmd);
		if(lpOperationW) free((LPVOID)lpOperationW);
		if(lpFileW) free((LPVOID)lpFileW);
		if(lpParametersW) free((LPVOID)lpParametersW);
		if(lpDirectoryW) free((LPVOID)lpDirectoryW);
		OutTraceSYS("%s: res=%#x\n", ApiRef, res);
		return res;
	}

	if ((dxw.dwFlags13 & SHAREDHOOK) &&
		((lpOperation == NULL)) || (lpOperation && !strcmp(lpOperation, "open")) &&
		(!_stricmp(&lpFile[strlen(lpFile)-4], ".exe")))
	{
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		char *sArgLine;
		char *sFile;
		OutTraceDW("%s: mapping to extCreateProcessA\n", ApiRef);

		// processing argument line adding parameters to the end if any
		int iArgLen = 2; // 1 for possible space + terminator
		if(lpFile) iArgLen += strlen(lpFile);
		if(lpParameters) iArgLen += strlen(lpParameters);
		sArgLine = (char *)malloc(iArgLen);  
		*sArgLine = 0;
		if(lpFile) strcat_s(sArgLine, iArgLen, lpFile);
		if(lpFile && lpParameters && (strlen(lpParameters)>0)) strcat_s(sArgLine, iArgLen, " ");
		if(lpParameters)strcat_s(sArgLine, iArgLen, lpParameters);
		// v2.06.07: if the target pathname has spaces, it must be surrounded by double quotes.
		BOOL hasSpaces = FALSE;
		if(lpFile) hasSpaces = (BOOL)strstr(lpFile, " ");

		// processing of task pathname adding the folder at the beginning (plus slash if missing) if necessary 
		if(isAbsolutePathA(lpFile)){
			int iFileLen = 1; // 1 for terminator
			if(hasSpaces) iFileLen += 2; // 1 for quotes
			if(lpFile) iFileLen += strlen(lpFile);
			sFile = (char *)malloc(iFileLen);
			*sFile = 0;
			if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
			if(lpFile) strcat_s(sFile, iFileLen, lpFile);
			if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
		}
		else {
			int iFileLen = 2; // 1 for possible slash + terminator
			if(hasSpaces) iFileLen += 2; // 1 for quotes
			if(lpFile) iFileLen += strlen(lpFile) + 1;
			if(lpDirectory) iFileLen += strlen(lpDirectory);
			sFile = (char *)malloc(iFileLen);
			*sFile = 0;
			if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
			if(lpDirectory && (strlen(lpDirectory) > 0)) {
				strcat_s(sFile, iFileLen, lpDirectory);
				char last = sFile[strlen(sFile)-1];
				if((last != '/') && (last != '\\')) strcat_s(sFile, iFileLen, "/");
			}
			if(lpFile) strcat_s(sFile, iFileLen, lpFile);
			if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
		}

		memset(&si, 0, sizeof(si)); 
		si.cb = sizeof(si);
		
		if(!extCreateProcessA(sFile, sArgLine, 0, 0, FALSE, 0, NULL, lpDirectory, &si, &pi)){
			OutErrorSYS("%s: extCreateProcessA ERROR err=%d\n", ApiRef, GetLastError());
		}

		free(sArgLine);
		free(sFile);
		return (HINSTANCE)GetModuleHandle(NULL);
	}

	res = (*pShellExecuteA)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res; 
}

HINSTANCE WINAPI extShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	HINSTANCE res;
	ApiName("ShellExecuteW");
	if(IsTraceSYS){
		OutTrace("%s:\n", ApiRef);
		OutTrace("> hwnd=%#x\n", hwnd);
		OutTrace("> operation=%ls\n", lpOperation);
		OutTrace("> file=%ls\n", lpFile);
		OutTrace("> params=%ls\n", lpParameters);
		OutTrace("> directory=%ls\n", lpDirectory);
		OutTrace("> show=%d\n", nShowCmd);
	}

	if(dxw.dwFlags14 & NOSHELLEXECUTE) {
		OutTraceSYS("%s: NOSHELLEXECUTE res=%#x\n", ApiRef, SHELL_RETCODE_OK);
		return (HINSTANCE)SHELL_RETCODE_OK;
	}

	res = (*pShellExecuteW)(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res; 
}

BOOL WINAPI extShellExecuteExA(SHELLEXECUTEINFOA *pExecInfo)
{
	BOOL res;
	ApiName("ShellExecuteExA");

	OutTraceSYS("%s: info={size=%d fmask=%#x hwnd=%#x verb=\"%s\" file=\"%s\" params=\"%s\" directory=\"%s\" show=%d hInstApp=%#x}\n", 
		ApiRef,
		pExecInfo->cbSize, 
		pExecInfo->fMask,
		pExecInfo->hwnd,
        pExecInfo->lpVerb,
        pExecInfo->lpFile,
        pExecInfo->lpParameters,
        pExecInfo->lpDirectory,
        pExecInfo->nShow,
		pExecInfo->hInstApp);

	if(dxw.dwFlags14 & NOSHELLEXECUTE) {
		OutTraceSYS("%s: NOSHELLEXECUTE res=%#x\n", ApiRef, TRUE);
		return TRUE;
	}

	if((dxw.dwFlags11 & CUSTOMLOCALE) && (dxw.dwFlags12 & PATHLOCALE)){
		SHELLEXECUTEINFOW ExecInfoW;
		SHELLEXECUTEINFOW *pExecInfoW = &ExecInfoW;
		memcpy(pExecInfoW, pExecInfo, sizeof(SHELLEXECUTEINFOW)); // because the two structures have the same size and shape
		pExecInfoW->lpVerb = Ascii2Wide(pExecInfo->lpVerb);
		pExecInfoW->lpFile = Ascii2Wide(pExecInfo->lpFile);
		pExecInfoW->lpParameters = Ascii2Wide(pExecInfo->lpParameters);
		pExecInfoW->lpDirectory = Ascii2Wide(pExecInfo->lpDirectory);
		if(!pShellExecuteExW) pShellExecuteExW = ShellExecuteExW;
		res = (*pShellExecuteExW)(pExecInfoW);
		OutTraceSYS("%s: ret=%d info={size=%d fmask=%#x hwnd=%#x verb=\"%Ls\" file=\"%Ls\" params=\"%Ls\" directory=\"%Ls\" show=%d hInstApp=%#x}\n", 
			ApiRef, res,
			pExecInfo->cbSize, 
			pExecInfo->fMask,
			pExecInfo->hwnd,
			pExecInfo->lpVerb,
			pExecInfo->lpFile,
			pExecInfo->lpParameters,
			pExecInfo->lpDirectory,
			pExecInfo->nShow,
			pExecInfo->hInstApp);		
		if(pExecInfoW->lpVerb) free((LPVOID)pExecInfoW->lpVerb);
		if(pExecInfoW->lpFile) free((LPVOID)pExecInfoW->lpFile);
		if(pExecInfoW->lpParameters) free((LPVOID)pExecInfoW->lpParameters);
		if(pExecInfoW->lpDirectory) free((LPVOID)pExecInfoW->lpDirectory);
		pExecInfo->hInstApp = pExecInfoW->hInstApp;
		return res;
	}

	if (dxw.dwFlags13 & SHAREDHOOK) {
		char *lpFile = (char *)pExecInfo->lpFile;
		char *lpParameters = (char *)pExecInfo->lpParameters;
		char *lpDirectory = (char *)pExecInfo->lpDirectory;
		BOOL ret;
		if (((pExecInfo->lpVerb == NULL)) || (pExecInfo->lpVerb && !strcmp(pExecInfo->lpVerb, "open")) &&
		(!_stricmp(&lpFile[strlen(lpFile)-4], ".exe"))) {
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			char *sArgLine;
			char *sFile;
			OutTraceDW("%s: mapping to extCreateProcessA\n", ApiRef);

			// replace null string with NULL pointer, fixes Chinese FPS @#@ "Dynasty"
			if(lpDirectory && (strlen(lpDirectory) == 0)) lpDirectory = NULL;
			// processing argument line adding parameters to the end if any
			int iArgLen = 2; // 1 for possible space + terminator
			if(lpFile) iArgLen += strlen(lpFile);
			if(lpParameters) iArgLen += strlen(lpParameters);
			sArgLine = (char *)malloc(iArgLen);  
			*sArgLine = 0;
			if(lpFile) strcat_s(sArgLine, iArgLen, lpFile);
			if(lpFile && lpParameters && (strlen(lpParameters)>0)) strcat_s(sArgLine, iArgLen, " ");
			if(lpParameters)strcat_s(sArgLine, iArgLen, lpParameters);
			// v2.06.07: if the target pathname has spaces, it must be surrounded by double quotes.
			BOOL hasSpaces = FALSE;
			if(lpFile) hasSpaces = (BOOL)strstr(lpFile, " ");

			// processing of task pathname adding the folder at the beginning (plus slash if missing) if necessary 
			if(isAbsolutePathA(lpFile)){
				int iFileLen = 1; // 1 for terminator
				if(hasSpaces) iFileLen += 2; // 1 for quotes
				if(lpFile) iFileLen += strlen(lpFile);
				sFile = (char *)malloc(iFileLen);
				*sFile = 0;
				if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
				if(lpFile) strcat_s(sFile, iFileLen, lpFile);
				if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
			}
			else {
				int iFileLen = 2; // 1 for possible slash + terminator
				if(hasSpaces) iFileLen += 2; // 1 for quotes
				if(lpFile) iFileLen += strlen(lpFile) + 1;
				if(lpDirectory) iFileLen += strlen(lpDirectory);
				sFile = (char *)malloc(iFileLen);
				*sFile = 0;
				if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
				if(lpDirectory && (strlen(lpDirectory) > 0)) {
					strcat_s(sFile, iFileLen, lpDirectory);
					char last = sFile[strlen(sFile)-1];
					if((last != '/') && (last != '\\')) strcat_s(sFile, iFileLen, "/");
				}
				if(lpFile) strcat_s(sFile, iFileLen, lpFile);
				if(hasSpaces) strcat_s(sFile, iFileLen, "\"");
			}

			memset(&si, 0, sizeof(si)); 
			si.cb = sizeof(si);
			
			ret = extCreateProcessA(sFile, sArgLine, 0, 0, FALSE, 0, NULL, lpDirectory, &si, &pi);
			if(ret) {
				//pExecInfo->hInstApp = (HINSTANCE)0x2A;
				//pExecInfo->hInstApp = (HINSTANCE)33;
			}
			else {
				OutErrorSYS("%s: extCreateProcessA ERROR err=%d\n", ApiRef, GetLastError());
			}

			free(sArgLine);
			free(sFile);
			return TRUE;
		}
	}

	res = (*pShellExecuteExA)(pExecInfo);
	OutTraceSYS("%s: ret=%d info={size=%d fmask=%#x hwnd=%#x verb=\"%s\" file=\"%s\" params=\"%s\" directory=\"%s\" show=%d hInstApp=%#x}\n", 
		ApiRef, res,
		pExecInfo->cbSize, 
		pExecInfo->fMask,
		pExecInfo->hwnd,
        pExecInfo->lpVerb,
        pExecInfo->lpFile,
        pExecInfo->lpParameters,
        pExecInfo->lpDirectory,
        pExecInfo->nShow,
		pExecInfo->hInstApp);

	return res;
}

BOOL WINAPI extShellExecuteExW(SHELLEXECUTEINFOW *pExecInfo)
{
	BOOL res;
	ApiName("ShellExecuteExW");

	OutTraceSYS("%s: info={size=%d fmask=%#x hwnd=%#x verb=\"%Ls\" file=\"%Ls\" params=\"%Ls\" directory=\"%Ls\" show=%d hInstApp=%#x}\n", 
		ApiRef,
		pExecInfo->cbSize, 
		pExecInfo->fMask,
		pExecInfo->hwnd,
        pExecInfo->lpVerb,
        pExecInfo->lpFile,
        pExecInfo->lpParameters,
        pExecInfo->lpDirectory,
        pExecInfo->nShow,
		pExecInfo->hInstApp);

	if(dxw.dwFlags14 & NOSHELLEXECUTE) {
		OutTraceSYS("%s: NOSHELLEXECUTE hInstApp=%#x\n", ApiRef, 33);
		pExecInfo->hInstApp = (HINSTANCE)33;
		return TRUE;
	}

	// to do: CUSTOMLOCALE handling
	// to do: SHAREDHOOK handling

	res = (*pShellExecuteExW)(pExecInfo);
	OutTraceSYS("%s: ret=%d info={size=%d fmask=%#x hwnd=%#x verb=\"%s\" file=\"%s\" params=\"%s\" directory=\"%s\" show=%d hInstApp=%#x}\n", 
		ApiRef, res,
		pExecInfo->cbSize, 
		pExecInfo->fMask,
		pExecInfo->hwnd,
        pExecInfo->lpVerb,
        pExecInfo->lpFile,
        pExecInfo->lpParameters,
        pExecInfo->lpDirectory,
        pExecInfo->nShow,
		pExecInfo->hInstApp);	
	return res; 
}

#ifdef TRACEALL
HRESULT WINAPI extSHCoCreateInstance(PCWSTR pszCLSID, const CLSID *pclsid, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
	HRESULT res;
	ApiName("SHCoCreateInstance");
	if(IsTraceSYS){
		OutTrace("%s:\n", ApiRef);
		OutTrace("> CLSID=%ls\n", pszCLSID);
	}
	res = (*pSHCoCreateInstance)(pszCLSID, pclsid, pUnkOuter, riid, ppv);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

int WINAPI extSHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp)
{
	int res;
	ApiName("SHFileOperationA");
	if(IsTraceSYS){
		OutTrace("%s:\n", ApiRef);
		OutTrace("> hwnd=%#x\n", lpFileOp->hwnd);
		OutTrace("> wFunc=%d\n", lpFileOp->wFunc);
		OutTrace("> from=%s\n", lpFileOp->pFrom);
		OutTrace("> to=%s\n", lpFileOp->pTo);
		OutTrace("> flags=%#x\n", lpFileOp->fFlags);
		OutTrace("> fAnyOperationsAborted=%d\n", lpFileOp->fAnyOperationsAborted);
	}
	res = (*pSHFileOperationA)(lpFileOp);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extSHGetSpecialFolderLocation(HWND hwnd, int csidl, PIDLIST_ABSOLUTE *ppidl)
{
	HRESULT res;
	ApiName("SHGetSpecialFolderLocation");
	if(IsTraceSYS){
		OutTrace("%s:\n", ApiRef);
		OutTrace("> hwnd=%#x\n", hwnd);
		OutTrace("> csidl=%d\n", csidl);
	}
	res = (*pSHGetSpecialFolderLocation)(hwnd, csidl, ppidl);
	if(res == S_OK){
		USHORT *w = (USHORT *)*ppidl;
		UINT i = 0;
		while (*w && (i<2)){
			OutTrace("> idl[%d]:\n", i++); 
			HexTrace((BYTE *)w + sizeof(USHORT), *w - sizeof(USHORT));
			//HexTrace((BYTE *)w, *w);
			w += *w / sizeof(USHORT);
		}
		//HexTrace((BYTE *)*ppidl, 80);
	}
	else {
		OutErrorSYS("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

PIDLIST_ABSOLUTE WINAPI extSHBrowseForFolderA(LPBROWSEINFOA lpbi)
{
	PIDLIST_ABSOLUTE res;
	ApiName("SHBrowseForFolderA");
	if(IsTraceSYS){
		OutTrace("%s:\n", ApiRef);
	}
	res = (*pSHBrowseForFolderA)(lpbi);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

HINSTANCE WINAPI extFindExecutableA(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
	HINSTANCE res;
	ApiName("FindExecutableA");
	if(IsTraceSYS){
		OutTrace("%s:\n", ApiRef);
		OutTrace("> file=%s\n", lpFile);
		OutTrace("> directory=%s\n", lpDirectory);
	}
	res = (*pFindExecutableA)(lpFile, lpDirectory, lpResult);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	OutTrace("> result=%s\n", lpResult);
	return res;
}
#endif // TRACEALL

#ifdef EXPERIMENTAL_ENABLED
#if 0
		in ShObjIdl.h

        virtual HRESULT STDMETHODCALLTYPE GetPath( 
            /* [size_is][string][out] */ __RPC__out_ecount_full_string(cch) LPSTR pszFile,
            /* [in] */ int cch,
            /* [unique][out][in] */ __RPC__inout_opt WIN32_FIND_DATAA *pfd,
            /* [in] */ DWORD fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIDList( 
            /* [out] */ __RPC__deref_out_opt PIDLIST_ABSOLUTE *ppidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIDList( 
            /* [in] */ __RPC__in PCIDLIST_ABSOLUTE pidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [size_is][string][out] */ __RPC__out_ecount_full_string(cch) LPSTR pszName,
            /* [in] */ int cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDescription( 
            /* [string][in] */ __RPC__in LPCSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWorkingDirectory( 
            /* [size_is][string][out] */ __RPC__out_ecount_full_string(cch) LPSTR pszDir,
            /* [in] */ int cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWorkingDirectory( 
            /* [string][in] */ __RPC__in LPCSTR pszDir) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetArguments( 
            /* [size_is][string][out] */ __RPC__out_ecount_full_string(cch) LPSTR pszArgs,
            /* [in] */ int cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetArguments( 
            /* [string][in] */ __RPC__in LPCSTR pszArgs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHotkey( 
            /* [out] */ __RPC__out WORD *pwHotkey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHotkey( 
            /* [in] */ WORD wHotkey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetShowCmd( 
            /* [out] */ __RPC__out int *piShowCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetShowCmd( 
            /* [in] */ int iShowCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIconLocation( 
            /* [size_is][string][out] */ __RPC__out_ecount_full_string(cch) LPSTR pszIconPath,
            /* [in] */ int cch,
            /* [out] */ __RPC__out int *piIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIconLocation( 
            /* [string][in] */ __RPC__in LPCSTR pszIconPath,
            /* [in] */ int iIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRelativePath( 
            /* [string][in] */ __RPC__in LPCSTR pszPathRel,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resolve( 
            /* [unique][in] */ __RPC__in_opt HWND hwnd,
            /* [in] */ DWORD fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPath( 
            /* [string][in] */ __RPC__in LPCSTR pszFile) = 0;
#endif

void HookUCOMIPersistFile(void *);

HRESULT (WINAPI *pSLQueryInterface)(void *, REFIID, LPVOID *);
HRESULT WINAPI extSLQueryInterface(void *obj, REFIID riid, LPVOID *obp)
{
	HRESULT res;
	ApiName("IShellLinkA::QueryInterface");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: obj=%#x\n", ApiRef, obj);
	OutTraceSYS("> riid=%s\n", sGUID((GUID *)&riid));
	res = (*pSLQueryInterface)(obj, riid, obp);
	switch(riid.Data1){
		case 0x0000010B:
			HookUCOMIPersistFile((void *)obp);
			break;
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, res);
	return res;
}

HRESULT (WINAPI *pGetPath)(void *, LPSTR, int, WIN32_FIND_DATAA *, DWORD);
HRESULT WINAPI extGetPath(void *obj, LPSTR pszFile, int cch, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
	HRESULT res;
	ApiName("IShellLinkA::GetPath");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: obj=%#x\n", ApiRef, obj);
	OutTraceSYS("> file=%s\n", pszFile);
	res = (*pGetPath)(obj, pszFile, cch, pfd, fFlags);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, res);
	return res;
}

HRESULT (WINAPI *pSetPath)(void *, LPSTR);
HRESULT WINAPI extSetPath(void *obj, LPSTR pszFile)
{
	HRESULT res;
	ApiName("IShellLinkA::SetPath");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: obj=%#x\n", ApiRef, obj);
	OutTraceSYS("> file=%s\n", pszFile);
	res = (*pSetPath)(obj, pszFile);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, res);
	return res;
}

//HRESULT (WINAPI *pGetIDList)(void *, PIDLIST_ABSOLUTE *);
typedef HRESULT (WINAPI *GetIDList_Type)(void *, PIDLIST_ABSOLUTE *);
GetIDList_Type pGetIDList;
HRESULT WINAPI extGetIDList(void *obj, PIDLIST_ABSOLUTE *ppidl)
{
	HRESULT res;
	ApiName("IShellLinkA::GetIDList");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: obj=%#x\n", ApiRef, obj);
	res = (*pGetIDList)(obj, ppidl);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, res);
	return res;
}

#ifdef ENABLE_MARKERS
HRESULT WINAPI marker()
{
	OutTrace("Marker\n");
	MessageBoxA(0, "Marker", "DxWnd", 0);
	return 0;
}
#endif

void HookShellLinkA(void *obj)
{
	OutTraceDW("Hooking IID_IShellLinkA obj=%#x\n", obj);

	SetHook((void *)(**(DWORD **)obj +  0), extSLQueryInterface, (void **)&pSLQueryInterface, "QueryInterface");
	//SetHook((void *)(**(DWORD **)obj +  4), extSLAddRef, (void **)&pSLAddRef, "AddRef");
	//SetHook((void *)(**(DWORD **)obj +  8), extSLRelease, (void **)&pSLRelease, "Release");
	SetHook((void *)(**(DWORD **)obj + 12), extGetPath, (void **)&pGetPath, "GetPath");
	SetHook((void *)(**(DWORD **)obj + 16), extGetIDList, (void **)&pGetIDList, "GetIDList");
	//SetHook((void *)(**(DWORD **)obj + 20), extSetIDList, (void **)&pSetIDList, "SetIDList");
	//SetHook((void *)(**(DWORD **)obj + 24), extGetDescription, (void **)&pGetDescription, "GetDescription");
	//SetHook((void *)(**(DWORD **)obj + 28), extSetDescription, (void **)&pSetDescription, "SetDescription");
	//SetHook((void *)(**(DWORD **)obj + 32), extGetWorkingDirectory, (void **)&pGetWorkingDirectory, "GetWorkingDirectory");
	//SetHook((void *)(**(DWORD **)obj + 36), extSetWorkingDirectory, (void **)&pSetWorkingDirectory, "SetWorkingDirectory");
	//SetHook((void *)(**(DWORD **)obj + 40), extGetArguments, (void **)&pGetArguments, "GetArguments");
	//SetHook((void *)(**(DWORD **)obj + 44), extSetArguments, (void **)&pSetArguments, "SetArguments");
	//SetHook((void *)(**(DWORD **)obj + 48), extGetHotkey, (void **)&pGetHotkey, "GetHotkey");
	//SetHook((void *)(**(DWORD **)obj + 52), extSetHotkey, (void **)&pSetHotkey, "SetHotkey");
	//SetHook((void *)(**(DWORD **)obj + 56), extGetShowCmd, (void **)&pGetShowCmd, "GetShowCmd");
	//SetHook((void *)(**(DWORD **)obj + 60), extSetShowCmd, (void **)&pSetShowCmd, "SetShowCmd");
	//SetHook((void *)(**(DWORD **)obj + 64), extGetIconLocation, (void **)&pGetIconLocation, "GetIconLocation");
	//SetHook((void *)(**(DWORD **)obj + 68), extSetIconLocation, (void **)&pSetIconLocation, "SetIconLocation");
	//SetHook((void *)(**(DWORD **)obj + 72), extSetRelativePath, (void **)&pSetRelativePath, "SetRelativePath");
	//SetHook((void *)(**(DWORD **)obj + 76), extResolve, (void **)&pResolve, "Resolve");

#ifdef ENABLE_MARKERS
	//SetHook((void *)(**(DWORD **)obj +  0), marker, NULL, "QueryInterface");
	SetHook((void *)(**(DWORD **)obj +  4), marker, NULL, "SetIDList");
	SetHook((void *)(**(DWORD **)obj +  8), marker, NULL, "SetIDList");
	//SetHook((void *)(**(DWORD **)obj + 12), marker, NULL, "SetIDList");
	//SetHook((void *)(**(DWORD **)obj + 16), marker, NULL, "SetIDList");
	SetHook((void *)(**(DWORD **)obj + 20), marker, NULL, "SetIDList");
	SetHook((void *)(**(DWORD **)obj + 24), marker, NULL, "GetDescription");
	SetHook((void *)(**(DWORD **)obj + 28), marker, NULL, "SetDescription");
	SetHook((void *)(**(DWORD **)obj + 32), marker, NULL, "GetWorkingDirectory");
	SetHook((void *)(**(DWORD **)obj + 36), marker, NULL, "SetWorkingDirectory");
	SetHook((void *)(**(DWORD **)obj + 40), marker, NULL, "GetArguments");
	SetHook((void *)(**(DWORD **)obj + 44), marker, NULL, "SetArguments");
	SetHook((void *)(**(DWORD **)obj + 48), marker, NULL, "GetHotkey");
	SetHook((void *)(**(DWORD **)obj + 52), marker, NULL, "SetHotkey");
	SetHook((void *)(**(DWORD **)obj + 56), marker, NULL, "GetShowCmd");
	SetHook((void *)(**(DWORD **)obj + 60), marker, NULL, "SetShowCmd");
	SetHook((void *)(**(DWORD **)obj + 64), marker, NULL, "GetIconLocation");
	SetHook((void *)(**(DWORD **)obj + 68), marker, NULL, "SetIconLocation");
	SetHook((void *)(**(DWORD **)obj + 72), marker, NULL, "SetRelativePath");
	SetHook((void *)(**(DWORD **)obj + 76), marker, NULL, "Resolve");
#endif

	SetHook((void *)(**(DWORD **)obj + 80), extSetPath, (void **)&pSetPath, "SetPath");
}

#if 0
	in Objdl.h:

    typedef struct IPersistFileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPersistFile * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPersistFile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPersistFile * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassID )( 
            IPersistFile * This,
            /* [out] */ __RPC__out CLSID *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE *IsDirty )( 
            IPersistFile * This);
        
        HRESULT ( STDMETHODCALLTYPE *Load )( 
            IPersistFile * This,
            /* [in] */ __RPC__in LPCOLESTR pszFileName,
            /* [in] */ DWORD dwMode);
        
        HRESULT ( STDMETHODCALLTYPE *Save )( 
            IPersistFile * This,
            /* [unique][in] */ __RPC__in_opt LPCOLESTR pszFileName,
            /* [in] */ BOOL fRemember);
        
        HRESULT ( STDMETHODCALLTYPE *SaveCompleted )( 
            IPersistFile * This,
            /* [unique][in] */ __RPC__in_opt LPCOLESTR pszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurFile )( 
            IPersistFile * This,
            /* [out] */ __RPC__deref_out_opt LPOLESTR *ppszFileName);
        
        END_INTERFACE
    } IPersistFileVtbl;
#endif

HRESULT (WINAPI *pIsDirty)();
HRESULT WINAPI extIsDirty()
{
	HRESULT res;
	ApiName("IPersistFile::IsDirty");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	res = (*pIsDirty)();
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT (WINAPI *pLoad)(void *, LPCOLESTR, DWORD);
HRESULT WINAPI extLoad(void *obj, LPCOLESTR pszFileName, DWORD dwMode)
{
	HRESULT res;
	ApiName("IPersistFile::Load");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: IPersistFile=%#x pszFileName=%ls dwMode=%#x\n", ApiRef, obj, pszFileName, dwMode);
	res = (*pLoad)(obj, pszFileName, dwMode);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT (WINAPI *pSave)(LPCOLESTR, BOOL);
HRESULT WINAPI extSave(LPCOLESTR pszFileName, BOOL fRemember)
{
	HRESULT res;
	ApiName("IPersistFile::Save");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: filename=%ls remember=%d\n", ApiRef, pszFileName, fRemember);
	res = (*pSave)(pszFileName, fRemember);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT (WINAPI *pSaveCompleted)(LPCOLESTR);
HRESULT WINAPI extSaveCompleted(LPCOLESTR pszFileName)
{
	HRESULT res;
	ApiName("IPersistFile::SaveCompleted");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: filename=%ls\n", ApiRef, pszFileName);
	res = (*pSaveCompleted)(pszFileName);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT (WINAPI *pGetCurFile)(void *, LPOLESTR *);
HRESULT WINAPI extGetCurFile(void *obj, LPOLESTR *ppszFileName)
{
	HRESULT res;
	ApiName("IPersistFile::GetCurFile");
	//MessageBox(0, ApiRef, "dxwnd", 0);
	OutTraceSYS("%s: IPersistFile=%#x\n", ApiRef, obj);
	res = (*pGetCurFile)(obj, ppszFileName);
	OutTraceSYS("%s: res=%#x\n", ApiRef, res);
	if(!res) OutTraceSYS("%s: FileName=%ls\n", ApiRef, *ppszFileName);
	return res;
}

void HookUCOMIPersistFile(void *obj)
{
	OutTraceDW("Hooking IPersistFile obj=%#x\n", obj);

	SetHook((void *)(**(DWORD **)obj +  0), extSLQueryInterface, (void **)&pSLQueryInterface, "QueryInterface");
	SetHook((void *)(**(DWORD **)obj + 16), extIsDirty, (void **)&pIsDirty, "IsDirty");
	SetHook((void *)(**(DWORD **)obj + 20), extLoad, (void **)&pLoad, "Load");
	SetHook((void *)(**(DWORD **)obj + 24), extSave, (void **)&pSave, "Save");
	SetHook((void *)(**(DWORD **)obj + 28), extSaveCompleted, (void **)&pSaveCompleted, "SaveCompleted");
	SetHook((void *)(**(DWORD **)obj + 32), extGetCurFile, (void **)&pGetCurFile, "GetCurFile");

	//SetHook((void *)(**(DWORD **)obj + 16), marker, NULL, "SetIDList");
	//SetHook((void *)(**(DWORD **)obj + 24), marker, NULL, "SetIDList");
	//SetHook((void *)(**(DWORD **)obj + 28), marker, NULL, "SetIDList");
	//SetHook((void *)(**(DWORD **)obj + 32), marker, NULL, "SetIDList");

#ifdef ENABLE_MARKERS
	//SetHook((void *)(**(DWORD **)obj +  4), marker, NULL, "");
	//SetHook((void *)(**(DWORD **)obj +  8), marker, NULL, "");
	SetHook((void *)(**(DWORD **)obj + 12), marker, NULL, "");
	SetHook((void *)(**(DWORD **)obj + 24), marker, NULL, "");
	SetHook((void *)(**(DWORD **)obj + 28), marker, NULL, "");
#endif
}
#endif // EXPERIMENTAL_ENABLED
