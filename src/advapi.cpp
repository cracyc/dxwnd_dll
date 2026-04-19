#define _CRT_SECURE_NO_WARNINGS
#define DXW_REGISTRYDUMP

#define _MODULE "advapi32"

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
//#include "h_advapi32.h"
#include "dxhook.h"
#include "dxhelper.h"

typedef LONG (WINAPI *RegDeleteValueA_Type)(HKEY, LPCSTR);
RegDeleteValueA_Type pRegDeleteValueA;
LONG WINAPI extRegDeleteValueA(HKEY, LPCSTR);

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "RegOpenKeyA", (FARPROC)RegOpenKeyA, (FARPROC *)&pRegOpenKeyA, (FARPROC)extRegOpenKeyA},
	{HOOK_IAT_CANDIDATE, 0, "RegOpenKeyExA", (FARPROC)RegOpenKeyExA, (FARPROC *)&pRegOpenKeyExA, (FARPROC)extRegOpenKeyExA},
	{HOOK_IAT_CANDIDATE, 0, "RegCloseKey", (FARPROC)RegCloseKey, (FARPROC *)&pRegCloseKey, (FARPROC)extRegCloseKey},
	{HOOK_IAT_CANDIDATE, 0, "RegQueryValueA", (FARPROC)RegQueryValueA, (FARPROC *)&pRegQueryValueA, (FARPROC)extRegQueryValueA},
	{HOOK_IAT_CANDIDATE, 0, "RegQueryValueExA", (FARPROC)RegQueryValueExA, (FARPROC *)&pRegQueryValueExA, (FARPROC)extRegQueryValueExA},
	{HOOK_IAT_CANDIDATE, 0, "RegCreateKeyA", (FARPROC)RegCreateKeyA, (FARPROC *)&pRegCreateKeyA, (FARPROC)extRegCreateKeyA},
	{HOOK_IAT_CANDIDATE, 0, "RegCreateKeyExA", (FARPROC)RegCreateKeyExA, (FARPROC *)&pRegCreateKeyExA, (FARPROC)extRegCreateKeyExA},
	{HOOK_IAT_CANDIDATE, 0, "RegSetValueExA", (FARPROC)RegSetValueExA, (FARPROC *)&pRegSetValueExA, (FARPROC)extRegSetValueExA},
	{HOOK_IAT_CANDIDATE, 0, "RegFlushKey", (FARPROC)RegFlushKey, (FARPROC *)&pRegFlushKey, (FARPROC)extRegFlushKey},
	// v2.3.36
	{HOOK_IAT_CANDIDATE, 0, "RegEnumValueA", (FARPROC)RegEnumValueA, (FARPROC *)&pRegEnumValueA, (FARPROC)extRegEnumValueA},
	// v2.4.37
	{HOOK_IAT_CANDIDATE, 0, "RegEnumKeyA", (FARPROC)RegEnumKeyA, (FARPROC *)&pRegEnumKeyA, (FARPROC)extRegEnumKeyA},
	{HOOK_IAT_CANDIDATE, 0, "RegEnumKeyExA", (FARPROC)RegEnumKeyExA, (FARPROC *)&pRegEnumKeyExA, (FARPROC)extRegEnumKeyExA},
	// v2.4.97
	{HOOK_IAT_CANDIDATE, 0, "RegQueryInfoKeyA", (FARPROC)RegQueryInfoKeyA, (FARPROC *)&pRegQueryInfoKeyA, (FARPROC)extRegQueryInfoKeyA},
	// v2.5.48 - "Loony Labyrinth Pinball"
	{HOOK_IAT_CANDIDATE, 0, "RegSetValueA", (FARPROC)RegSetValueA, (FARPROC *)&pRegSetValueA, (FARPROC)extRegSetValueA},
	// v2.5.54 - "Return Fire"
	{HOOK_IAT_CANDIDATE, 0, "GetUserNameA", (FARPROC)GetUserNameA, (FARPROC *)&pGetUserNameA, (FARPROC)extGetUserNameA},
	// v2.05.76 - widechar calls
	{HOOK_IAT_CANDIDATE, 0, "RegOpenKeyW", (FARPROC)RegOpenKeyW, (FARPROC *)&pRegOpenKeyW, (FARPROC)extRegOpenKeyW},
	{HOOK_IAT_CANDIDATE, 0, "RegOpenKeyExW", (FARPROC)RegOpenKeyExW, (FARPROC *)&pRegOpenKeyExW, (FARPROC)extRegOpenKeyExW},
	{HOOK_IAT_CANDIDATE, 0, "RegQueryValueW", (FARPROC)RegQueryValueW, (FARPROC *)&pRegQueryValueW, (FARPROC)extRegQueryValueW},
	{HOOK_IAT_CANDIDATE, 0, "RegQueryValueExW", (FARPROC)RegQueryValueExW, (FARPROC *)&pRegQueryValueExW, (FARPROC)extRegQueryValueExW},
	{HOOK_IAT_CANDIDATE, 0, "RegCreateKeyW", (FARPROC)RegCreateKeyW, (FARPROC *)&pRegCreateKeyW, (FARPROC)extRegCreateKeyW},
	{HOOK_IAT_CANDIDATE, 0, "RegCreateKeyExW", (FARPROC)RegCreateKeyExW, (FARPROC *)&pRegCreateKeyExW, (FARPROC)extRegCreateKeyExW},
	{HOOK_IAT_CANDIDATE, 0, "RegSetValueW", (FARPROC)RegSetValueW, (FARPROC *)&pRegSetValueW, (FARPROC)extRegSetValueW},
	{HOOK_IAT_CANDIDATE, 0, "RegSetValueExW", (FARPROC)RegSetValueExW, (FARPROC *)&pRegSetValueExW, (FARPROC)extRegSetValueExW},
	{HOOK_IAT_CANDIDATE, 0, "RegEnumValueW", (FARPROC)RegEnumValueW, (FARPROC *)&pRegEnumValueW, (FARPROC)extRegEnumValueW},
	{HOOK_IAT_CANDIDATE, 0, "RegEnumKeyW", (FARPROC)RegEnumKeyW, (FARPROC *)&pRegEnumKeyW, (FARPROC)extRegEnumKeyW},
	{HOOK_IAT_CANDIDATE, 0, "RegEnumKeyExW", (FARPROC)RegEnumKeyExW, (FARPROC *)&pRegEnumKeyExW, (FARPROC)extRegEnumKeyExW},
	{HOOK_IAT_CANDIDATE, 0, "RegQueryInfoKeyW", (FARPROC)RegQueryInfoKeyW, (FARPROC *)&pRegQueryInfoKeyW, (FARPROC)extRegQueryInfoKeyW},
	{HOOK_IAT_CANDIDATE, 0, "GetUserNameW", (FARPROC)GetUserNameW, (FARPROC *)&pGetUserNameW, (FARPROC)extGetUserNameW},

	// RegGetValueA is not supported on Windows XP 32 bit
	{HOOK_IAT_CANDIDATE, 0, "RegGetValueA", (FARPROC)NULL, (FARPROC *)&pRegGetValueA, (FARPROC)extRegGetValueA},
	{HOOK_IAT_CANDIDATE, 0, "RegGetValueW", (FARPROC)NULL, (FARPROC *)&pRegGetValueW, (FARPROC)extRegGetValueW},
	{HOOK_IAT_CANDIDATE, 0, "RegOpenCurrentUser", (FARPROC)RegOpenCurrentUser, (FARPROC *)&pRegOpenCurrentUser, (FARPROC)extRegOpenCurrentUser},

	// v2.06.08
	{HOOK_IAT_CANDIDATE, 0, "RegDeleteValueA", (FARPROC)NULL, (FARPROC *)&pRegDeleteValueA, (FARPROC)extRegDeleteValueA},

	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type MacromediaHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "RegQueryValueExA", (FARPROC)RegQueryValueExA, (FARPROC *)&pRegQueryValueExA, (FARPROC)extRegQueryValueExA},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void MakeHot(HookEntryEx_Type *Hooks)
{
	for(; Hooks->APIName; Hooks++) 
		if((Hooks->HookStatus == HOOK_IAT_CANDIDATE) || (Hooks->HookStatus == HOOK_HOT_CANDIDATE))
			Hooks->HookStatus = HOOK_HOT_REQUIRED;
}

void HookAdvApi32(HMODULE module)
{
	if(	 dxw.dwFlags9 & HOTREGISTRY  ) {
		MakeHot(Hooks);
		MakeHot(MacromediaHooks);
	}
	// can't hot-patch calls twice, so be careful not to ever hook Hooks and 
	// MacromediaHooks contemporarily!
	if( (dxw.dwFlags3 & EMULATEREGISTRY) || 
		(dxw.dwFlags4 & OVERRIDEREGISTRY) || 
		(dxw.dwFlags6 & (WOW32REGISTRY|WOW64REGISTRY)) || 
		IsTraceR){
		HookLibraryEx(module, Hooks, "ADVAPI32.dll");
	}
	else if( dxw.dwFlags14 & FIXMACROMEDIAREG ) {
		HookLibraryEx(module, MacromediaHooks, "ADVAPI32.dll");
	}
}

FARPROC Remap_AdvApi32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if(!(dxw.dwFlags3 & EMULATEREGISTRY) || 
		(dxw.dwFlags4 & OVERRIDEREGISTRY) || 
		(dxw.dwFlags6 & (WOW32REGISTRY|WOW64REGISTRY)) || 
		IsTraceR) return NULL;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

#define HKEY_FAKE ((HKEY)0x7FFFFFFF)
#define HKEY_MASK 0x7FFFFF00
#define IsFake(hKey) (((DWORD)hKey & HKEY_MASK) == HKEY_MASK)

static FILE *OpenFakeRegistry();
static char *hKey2String(HKEY);

// int ReplaceVar(pData, lplpData, lpcbData): 
// extract the token name from pData beginning up to '}' delimiter
// calculates the value of the token. If the token is unknown, the value is null string.
// if *lplpData, copies the token value string to *lplpData and increments *lplpData
// if lpcbData, increments the key length of the token value length
// returns the length of token label to advance the parsing loop

typedef enum {
	LABEL_PATH = 0,
	LABEL_WORKDIR,
	LABEL_VOID,
	LABEL_END
};

static char *sTokenLabels[LABEL_END+1]={
	"path}",
	"work}",
	"}",
	NULL
};

#ifndef DXW_NOTRACES
static char *sType(DWORD t)
{
	char *s;
	switch(t){
		case REG_NONE:			s="NONE"; break;
		case REG_SZ:			s="SZ"; break;
		case REG_EXPAND_SZ:		s="EXPAND_SZ"; break;
		case REG_BINARY:		s="BINARY"; break;
		case REG_DWORD:			s="DWORD"; break;
		case REG_DWORD_BIG_ENDIAN: s="DWORD_BIG_ENDIAN"; break;
		case REG_LINK:			s="LINK"; break;
		case REG_MULTI_SZ:		s="MULTI_SZ"; break;
		case REG_RESOURCE_LIST: s="RESOURCE_LIST"; break;
		case REG_FULL_RESOURCE_DESCRIPTOR: s="FULL_RESOURCE_DESCRIPTOR"; break;
		case REG_RESOURCE_REQUIREMENTS_LIST: s="RESOURCE_REQUIREMENTS_LIST"; break;
		case REG_QWORD:			s="QWORD"; break;
		default:				s="???"; break;
	}
	return s;
}
#endif // DXW_NOTRACES

static int ReplaceVar(char *pData, LPBYTE *lplpData, LPDWORD lpcbData)
{
	int iTokenLength;
	int iLabelLength;
	int iTokenIndex;
	char sTokenValue[MAX_PATH];
	char *p;
	// search for a matching token
	for(iTokenIndex=0; sTokenLabels[iTokenIndex]; iTokenIndex++){
		if(!_strnicmp(pData, sTokenLabels[iTokenIndex], strlen(sTokenLabels[iTokenIndex]))) break;
	}
	// set token label length
	iLabelLength = strlen(sTokenLabels[iTokenIndex]);
	// do replacement
	// v2.05.53: "path" vs. "work", made different
	switch(iTokenIndex){
		case LABEL_PATH:
			GetModuleFileNameA(NULL, sTokenValue, sizeof(sTokenValue));
			//PathRemoveFileSpecA(sTokenValue);
			p = sTokenValue + strlen(sTokenValue);
			while((*p != '\\') && (p > sTokenValue)) p--;
			if(*p == '\\') *p=0;
			break;
		case LABEL_WORKDIR:
			GetCurrentDirectory(MAX_PATH, sTokenValue);
			break;
		case LABEL_VOID:
		case LABEL_END:
			strcpy(sTokenValue, "");
			break;
	}
	// set output vars if not NULL
	iTokenLength = strlen(sTokenValue);
	OutTraceR("REPLACED token=%d val=\"%s\" len=%d\n", iTokenIndex, sTokenValue, iTokenLength);
	if(*lplpData) {
		strcpy((char *)*lplpData, sTokenValue);
		*lplpData += iTokenLength;
	}
	if(lpcbData) *lpcbData += iTokenLength;
	// return label length to advance parsing
	return iLabelLength;
}

static char *hKey2String(HKEY hKey)
{
	char *skey;
	static char sKey[MAX_PATH+1];
	static char skeybuf[10];
	if(IsFake(hKey)) {
		FILE *regf;
		char RegBuf[MAX_PATH+1];
		regf=OpenFakeRegistry();
		if(regf!=NULL){
			HKEY hLocalKey=HKEY_FAKE;
			fgets(RegBuf, 256, regf);
			while (!feof(regf)){
				if(RegBuf[0]=='['){
					if(hLocalKey == hKey){
						//OutTrace("building fake Key=\"%s\" hKey=%#x\n", sKey, hKey);
						fclose(regf);
						strcpy(sKey, &RegBuf[1]);
						//sKey[strlen(sKey)-2]=0; // get rid of "]"
						for(int i=strlen(sKey)-1; i; i--){
							if(sKey[i]==']'){
								sKey[i]=0;
								break;
							}
						}
						return sKey;
					}
					else {
						hLocalKey--;
					}
				}
				fgets(RegBuf, 256, regf);
			}
			fclose(regf);
		}
		return "HKEY_NOT_FOUND";	
	}
	switch((ULONG)hKey){
		case HKEY_CLASSES_ROOT:		skey="HKEY_CLASSES_ROOT"; break;
        case HKEY_CURRENT_CONFIG:	skey="HKEY_CURRENT_CONFIG"; break;
        case HKEY_CURRENT_USER:		skey="HKEY_CURRENT_USER"; break;
        case HKEY_LOCAL_MACHINE:	skey="HKEY_LOCAL_MACHINE"; break;
        case HKEY_USERS:			skey="HKEY_USERS"; break;
		default:					sprintf(skeybuf, "%#x", hKey); skey=skeybuf; break;
	}
	return skey;
}

static char *Unescape(char *s, char **dest)
{
	if(!*dest)	*dest=(char *)malloc(strlen(s)+100);
	else		*dest=(char *)realloc(*dest, strlen(s)+100); 
	char *t = *dest;
	for(; *s; s++){
		if((*s=='\\') && (*(s+1)=='n')){
			*t++ = '\n';
			s++;
		}
		else{
			*t++ = *s;
		}
	}
	*t=0;
	return *dest;
}

#ifdef DXW_REGISTRYDUMP
static FILE *OpenFakeRegistry()
{
    static BOOL bDumped = FALSE;
    char sSourcePath[MAX_PATH+1];
    FILE *ret;
    sprintf_s(sSourcePath, MAX_PATH, "%s\\dxwnd.reg", GetDxWndPath());
    ret = fopen(sSourcePath, "r");
#ifndef DXW_NOTRACES
    if(ret && IsTraceR && !bDumped){
        char sLine[256+2];
        int line = 1;
        bDumped = TRUE; // do it just once
        while(fgets(sLine, 256, ret)){
            char *nl = sLine + strlen(sLine) -1;
			// no final carriage return or line feed
            while((nl >= sLine) && ((*nl == 0x0D) || (*nl == 0x0A))) *nl-- = 0; 
            OutTrace("[%04d]: \"%s\"\n", line++, sLine);
        }
        fseek(ret, 0, SEEK_SET);
    }
#endif // DXW_NOTRACES
    return ret;
}
#else
static FILE *OpenFakeRegistry()
{
	char sSourcePath[MAX_PATH+1];
	sprintf_s(sSourcePath, MAX_PATH, "%s\\dxwnd.reg", GetDxWndPath());
	return fopen(sSourcePath, "r");
}
#endif // DXW_REGISTRYDUMP

static LONG SeekFakeKey(FILE *regf, HKEY hKey)
{
	LONG res;
	res = ERROR_FILE_NOT_FOUND;
	char RegBuf[MAX_PATH+1];
	HKEY hCurKey=HKEY_FAKE+1;
	fgets(RegBuf, 256, regf);
	while (!feof(regf)){
		if(RegBuf[0]=='['){
			hCurKey--;
		}
		if(hCurKey==hKey) {
			//OutTraceREGDebug("DEBUG: SeekFakeKey fount key at line=%s\n", RegBuf);
			res = ERROR_SUCCESS;
			break;
		}
		fgets(RegBuf, 256, regf);
	}
	return res;
}

static LONG SeekValueName(FILE *regf, LPCSTR lpValueName)
{
	LONG res;
	char RegBuf[MAX_PATH+1];
	long KeySeekPtr;
	res = ERROR_FILE_NOT_FOUND;
	// v2.04.01: fix to handle the '@' case properly
	if(lpValueName) if(!lpValueName[0]) lpValueName=NULL; 
	KeySeekPtr = ftell(regf);
	fgets(RegBuf, 256, regf);
	while (!feof(regf)){
		if(!lpValueName) {
			if((RegBuf[0]=='@') || (!strcmp(RegBuf,"\"\"="))){
				fseek(regf, KeySeekPtr, SEEK_SET);
				return ERROR_SUCCESS;
			}
		}
		else {
			if((RegBuf[0]=='"') &&
				!_strnicmp(lpValueName, &RegBuf[1], strlen(lpValueName)) &&
				(RegBuf[strlen(lpValueName)+1]=='"') &&
				(RegBuf[strlen(lpValueName)+2]=='='))
				{
				fseek(regf, KeySeekPtr, SEEK_SET);
				return ERROR_SUCCESS;
			}
		}
		// the next Key definition "[" can stop the search 
		if(RegBuf[0]=='[') return res;
		KeySeekPtr = ftell(regf);
		fgets(RegBuf, 256, regf);
	}
	return res;
}

static LONG SeekValueIndex(FILE *regf, DWORD dwIndex, LPCSTR lpValueName, LPDWORD lpcchValueName)
{
	LONG res;
	char RegBuf[MAX_PATH+1];
	long KeySeekPtr;
	res = ERROR_NO_MORE_ITEMS;
	KeySeekPtr = ftell(regf);
	fgets(RegBuf, 256, regf);
	dwIndex++;
	while (!feof(regf) && dwIndex){
		if(RegBuf[0]=='"') dwIndex--;
		if(dwIndex == 0){
			fseek(regf, KeySeekPtr, SEEK_SET);
			//sscanf(RegBuf, "\"%s\"=", lpValueName);
			strncpy((char *)lpValueName, strtok(&RegBuf[1], "\""), *lpcchValueName);
			*lpcchValueName = strlen(lpValueName);
			//OutTrace("DEBUG: lpValueName=%s len=%d\n", lpValueName, *lpcchValueName);
			return ERROR_SUCCESS;
		}
		if(RegBuf[0]=='[') return res;
		KeySeekPtr = ftell(regf);
		fgets(RegBuf, 256, regf);
	}
	return res;
}

#ifndef DXW_NOTRACES
#define IfLogKeyValue(a, b, c, d, e, f) if(IsTraceR) LogKeyValue(a, b, c, d, e, f)
static void LogKeyValue(char *ApiName, BOOL bWide, LONG res, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	char sInfo[1024+80+1];
	if(res) {
		OutTrace("%s: ERROR res=%d %s\n", // v2.06.05 fix
			ApiName, 
			res,
			(res == ERROR_MORE_DATA) ? "ERROR_MORE_DATA" : ((res == ERROR_FILE_NOT_FOUND) ? "ERROR_FILE_NOT_FOUND" : ""));
		return;
	}
	//v2.06.05 fix: eliminated possible crash condition
	strcpy(sInfo, "");
	if(lpType && lpData && lpcbData) {
		DWORD cbData = *lpcbData;
		if(cbData > 1024) cbData = 1024; // v2.05.78 fix: some keys can be REALLY long!
		if(bWide){
			switch(*lpType){
				case REG_SZ: sprintf(sInfo, "Data=\"%.*ls\"", cbData, lpData); break; 
				case REG_DWORD: sprintf(sInfo, "Data=%#x", *(DWORD *)lpData); break;
				case REG_BINARY: sprintf(sInfo, "Data=%s", hexdump((BYTE *)lpData, cbData)); break;
				case REG_NONE: sprintf(sInfo, "Data=\"%ls\"", lpData); break;
				default: sprintf(sInfo, "Data=???"); break;
			}
		}
		else {
			switch(*lpType){
				case REG_SZ: sprintf(sInfo, "Data=\"%.*s\"", cbData, lpData); break; 
				case REG_DWORD: sprintf(sInfo, "Data=%#x", *(DWORD *)lpData); break;
				case REG_BINARY: sprintf(sInfo, "Data=%s", hexdump((BYTE *)lpData, cbData)); break;
				case REG_NONE: sprintf(sInfo, "Data=\"%s\"", lpData); break;
				default: sprintf(sInfo, "Data=???"); break;
			}
		}
	}
	OutTrace("%s: res=SUCCESS size=%d type=%#x(%s) %s\n", 
		ApiName, lpcbData?*lpcbData:0, lpType?*lpType:0, lpType?ExplainRegType(*lpType):"none", 
		sInfo);
} 
#else
#define IfLogKeyValue(a, b, c, d, e, f) 
#define LogKeyValue(a, b, c, d, e, f)
#endif

static DWORD GetKeyValue(
				FILE *regf,
				char *ApiName, 
				LPCSTR lpValueName, 
				LPDWORD lpType, // beware: could be NULL
				LPBYTE lpData,  // beware: could be NULL
				LPDWORD lpcbData,
				BOOL isWide)
{
	LONG res;
	LPBYTE lpb;
	char *pData;
	char RegBuf[MAX_PATH+1];
	DWORD cbData=0;
	DWORD dwType = 0;

	//OutTrace("GetKeyValue: ValueName=%s\n", lpValueName);
	fgets(RegBuf, 256, regf);
	if(RegBuf[0]=='@')
		pData=&RegBuf[2];
	else
		pData=&RegBuf[strlen(lpValueName)+3];
	lpb = lpData;
	if(lpcbData) {
		cbData = *lpcbData;
		*lpcbData=0;
	}
	do {
		if((*pData=='"') || (*pData=='(')) { // string value
			if(*pData=='('){
				sscanf(pData, "(%d)\"", &dwType);
				pData+=strlen("(n)\"");
			}
			else{
				dwType=REG_SZ;
				pData++;
			}
			while(*pData && (*pData != '"')){
				if(*pData=='\\') {
					pData++;
					switch(*pData){
						case '{':{
							pData++; // skip '{'
							pData += ReplaceVar(pData, &lpb, lpcbData);
							continue; // v2.04.13 fix
							}
							break;
						default: 
							break; // skip first '\'
					}
				}
				if(lpData && lpcbData) if(*lpcbData < cbData) *lpb++=*pData;
				pData++;
				if(lpcbData) (*lpcbData)++;
			}
			if(lpcbData) {
				(*lpcbData)++; // extra space for string terminator ?Get
				if(isWide) *lpcbData *= 2; // double the space for WIDE strings
			}
			if(lpData && lpcbData) {
				if(isWide){
					int size = strlen((char *)lpData);
					WCHAR *lpDataW = (WCHAR *)malloc((size + 1) * sizeof(WCHAR));
					int n = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpData, size, lpDataW, size);
					lpDataW[n]=0;
					//OutTrace("cbdata=%d n=%d data=%s wdata=%ls\n", *lpcbData, n, lpData, lpDataW);
					memcpy(lpData, lpDataW, *lpcbData);
					free(lpDataW);
				}
				else {
					if(*lpcbData <= cbData) *lpb = 0; // string terminator
				}
			}
			// v2.04.14 fix: ERROR_MORE_DATA should be returned only in case lpData is not NULL
			res=ERROR_SUCCESS;
			if(lpData && lpcbData)
				if (*lpcbData > cbData) res = ERROR_MORE_DATA;
			break;
		}
		if(!strncmp(pData,"dword:",strlen("dword:"))){ //dword value
			DWORD val;
			dwType=REG_DWORD;
			pData+=strlen("dword:");
			sscanf(pData, "%x", &val); // v2.05.19: fix - "%#x" -> "%x" !!!
			if(lpData) {
				if (cbData >= sizeof(DWORD)) {
					memcpy(lpData, &val, sizeof(DWORD));
					res=ERROR_SUCCESS;
				}
				else
					res=ERROR_MORE_DATA;
			}
			else 
				res=ERROR_SUCCESS; // data not needed
			if (lpcbData) *lpcbData=sizeof(DWORD);
			OutTraceR("%s: type=REG_DWORD cbData=%#x Data=%#x\n", 
				ApiName, lpcbData ? *lpcbData : 0, val);
			break;
		}
		if (!strncmp(pData,"hex",strlen("hex"))) { // default hex value REG_BINARY or custom hex value type hex(n)
			BYTE *p;
			p = (BYTE *)pData;
			if (!strncmp(pData,"hex(",strlen("hex("))) {
				sscanf(pData, "hex(%d):", &dwType);
				p+=strlen("hex(n):");
			}
			else {
				dwType=REG_BINARY;
				p+=strlen("hex:");
			}
			while(TRUE){
				p[strlen((char *)p)-1]=0; // eliminates \n at the end of line
				while(strlen((char *)p)>=3){
					if(lpcbData && (*lpcbData < cbData) && lpData){ // v2.05.54
						UINT c;
						sscanf((char *)p, "%x,", &c); // v2.05.27 fix - no # in sscanf format
						*lpb = (BYTE)c;
						lpb++;
					}
					p+=3;
					if(lpcbData) (*lpcbData)++;
				}
				if(*p=='\\'){
					fgets(RegBuf, 256, regf);
					pData = RegBuf;
					p = (BYTE *)pData;
					while (*p == ' ') p++; // v2.05.54: skip leading blanks
				}
				else break;
			}
			OutTraceR("%s: type=%s cbData=%d Data=%s\n", 
				ApiName, sType(dwType),
				lpcbData ? *lpcbData : 0, 
				lpData ? hexdump(lpData, *lpcbData) : "(NULL)");
			if(lpcbData) { // v2.05.54
				res=(*lpcbData > cbData) ? ERROR_MORE_DATA : ERROR_SUCCESS;
			}
			else {
				res = ERROR_SUCCESS;
			}
			break;
		}
	} while(FALSE);
	if(lpType) *lpType = dwType;
	IfLogKeyValue(ApiName, isWide, res, lpType, lpData, lpcbData);
	return res;
} 

static LONG myRegOpenKeyEx(
			    LPCSTR label,
				HKEY hKey,
				LPCSTR lpSubKey,
				PHKEY phkResult)
{
	FILE *regf;
	char sKey[MAX_PATH+1];
	char RegBuf[MAX_PATH+1];

	if(lpSubKey)
		sprintf(sKey,"%s\\%s", hKey2String(hKey), lpSubKey);
	else
		sprintf(sKey,"%s", hKey2String(hKey));

	OutTraceR("%s: searching for key=\"%s\"\n", label, sKey);

	regf=OpenFakeRegistry();
	if(regf!=NULL){
		if(phkResult) *phkResult=HKEY_FAKE;
		fgets(RegBuf, 256, regf);
		while (!feof(regf)){
			if(RegBuf[0]=='['){
				// beware: registry keys are case insensitive. Must use _strnicmp instead of strncmp
				if((!_strnicmp(&RegBuf[1],sKey,strlen(sKey))) && (RegBuf[strlen(sKey)+1]==']')){
					OutTraceR("%s: found fake Key=\"%s\" hkResult=%#x\n", label, sKey, phkResult ? *phkResult : 0);
					fclose(regf);
					return ERROR_SUCCESS;
				}
				else {
					if(phkResult) (*phkResult)--;
				}
			}
			fgets(RegBuf, 256, regf);
		}
		fclose(regf);
	}
	return ERROR_FILE_NOT_FOUND;
}

// v2.05.78:
// FixMacromediaRegistry: fixes a problem about buffer overflow in MacroMedia programs as described here:
// https://gaming.stackexchange.com/questions/339173/how-can-i-play-dcr-shockwave-games
// the procedure doesn't change the registry, it just replaces the returned data, so that no harm is possible.

LONG FixMacromediaRegistry(HKEY hKey, LPBYTE lpData, LPDWORD lpcbData)
{
	DWORD cbData = 0;
	char *lpDataPtr = (char *)lpData;
	char *keyName = "InstalledDisplayDrivers";
	LONG res;
	res=(*pRegQueryValueExA)(hKey, keyName, NULL, NULL, NULL, &cbData);
	if(res != ERROR_SUCCESS){
		return res;
	}
	OutTrace("FixMacromediaRegistry: cbdata=%d\n", cbData);
	char *keyBuf = (char *)malloc(cbData+1); 
	res=(*pRegQueryValueExA)(hKey, keyName, NULL, NULL, (LPBYTE)keyBuf, &cbData);
	if(res != ERROR_SUCCESS){
		free(keyBuf);
		return res;
	}
	char *p = keyBuf;
	cbData = 0;
	while(*p){
		int jump = strlen(p) + 1;
		char *t;
		char *q = p;
		t = q;
		q = strtok(p, "\\/");
		while(q) {
			t = q;
			q = strtok(NULL, "\\/");
		}
		if(!strcmp(t + strlen(t) - 4, ".dll")) t[strlen(t) - 4] = 0;
		OutTrace("> \"%s\"\n", t);
		cbData += strlen(t) + 1;
		if(lpData){
			strcpy(lpDataPtr, t);
			lpDataPtr = lpDataPtr + strlen(t) + 1;
			*lpDataPtr = 0;
		}
		p = p + jump;
	}
	if(lpcbData) *lpcbData = cbData + 1;
	free(keyBuf);
	if(lpData) for(char *p = (char *)lpData; *p; p+= strlen(p)+1) OutTrace("< \"%s\"\n", p);
	return res;
}

// ---------------------------------------------------------------------------------

LONG WINAPI extRegOpenKeyA( // so far, found only in "Moon Child"
				HKEY hKey,
				LPCSTR lpSubKey,
				PHKEY phkResult)
{
	LONG res;
	ApiName("RegOpenKeyA");

	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%s\"\n", ApiRef, hKey, hKey2String(hKey), lpSubKey);

	if(dxw.dwFlags4 & OVERRIDEREGISTRY){
		res = myRegOpenKeyEx(ApiRef, hKey, lpSubKey, phkResult);
		if(res == ERROR_SUCCESS) return res;
	}

	res=(*pRegOpenKeyA)(hKey, lpSubKey, phkResult);
	OutTraceR("%s: res=%#x phkResult=%#x\n", ApiRef, res, phkResult ? *phkResult : 0); 

	if(phkResult && (res == ERROR_SUCCESS) && (dxw.dwFlags13 & (DISABLED3DMMX|DISABLED3DXPSGP|DISABLEPSGP|D3DXDONOTMUTE))){
		if((hKey == HKEY_LOCAL_MACHINE) && !strcmp(lpSubKey, "Software\\Microsoft\\Direct3D")){
			OutTraceD3D("%s: D3D registry key=%#x\n", *phkResult);
			dxw.d3dRegistryKey = *phkResult;
		}
	}

	if((res==ERROR_SUCCESS) || !(dxw.dwFlags3 & EMULATEREGISTRY) || (dxw.dwFlags4 & OVERRIDEREGISTRY)) return res;
	
	return myRegOpenKeyEx(ApiRef, hKey, lpSubKey, phkResult);
}

LONG WINAPI extRegOpenKeyExA(
				HKEY hKey,
				LPCSTR lpSubKey,
				DWORD ulOptions,
				REGSAM samDesired,
				PHKEY phkResult)
{
	LONG res;
	ApiName("RegOpenKeyExA");

	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%s\" Options=%#x\n", 
		ApiRef, hKey, hKey2String(hKey), lpSubKey, ulOptions);

	if(dxw.dwFlags4 & OVERRIDEREGISTRY){
		res = myRegOpenKeyEx(ApiRef, hKey, lpSubKey, phkResult);
		if(res == ERROR_SUCCESS) return res;
	}

	if(dxw.dwFlags6 & WOW64REGISTRY){
		ulOptions &= ~KEY_WOW64_32KEY;
		ulOptions |= KEY_WOW64_64KEY;
	}
	if(dxw.dwFlags6 & WOW32REGISTRY){
		ulOptions &= ~KEY_WOW64_64KEY;
		ulOptions |= KEY_WOW64_32KEY;
	}

	res=(*pRegOpenKeyExA)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	OutTraceR("%s: res=%#x phkResult=%#x\n", ApiRef, res, phkResult ? *phkResult : 0); 

	if((res==ERROR_SUCCESS) || !(dxw.dwFlags3 & EMULATEREGISTRY) || (dxw.dwFlags4 & OVERRIDEREGISTRY)) return res;
	
	return myRegOpenKeyEx(ApiRef, hKey, lpSubKey, phkResult);
}

// extRegQueryValueA: legacy API, almost always replaced by extRegQueryValueExA but referenced
// in "Warhammer 40.000 Shadow of the Horned Rat"

LONG WINAPI extRegQueryValueA(
				HKEY hKey, 
				LPCSTR lpSubKey, 
				LPSTR lpValue, 
				PLONG lpcbValue)
{
	LONG res;
	ApiName("RegQueryValueA");
	FILE *regf;
	char *skey;
	BOOL isRootKey;

	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%s\" lpcbData=%#x->(%d)\n", 
		ApiRef, hKey, hKey2String(hKey), lpSubKey, lpcbValue, lpcbValue ? *lpcbValue : 0);

	if(dxw.dwFlags4 & OVERRIDEREGISTRY){
		isRootKey = TRUE;
		switch((DWORD)hKey){
			case HKEY_CLASSES_ROOT:		skey="HKEY_CLASSES_ROOT"; break;
			case HKEY_CURRENT_CONFIG:	skey="HKEY_CURRENT_CONFIG"; break;
			case HKEY_CURRENT_USER:		skey="HKEY_CURRENT_USER"; break;
			case HKEY_LOCAL_MACHINE:	skey="HKEY_LOCAL_MACHINE"; break;
			case HKEY_USERS:			skey="HKEY_USERS"; break;
			default:					skey=""; isRootKey = FALSE; break;
		}

		if(isRootKey){
			regf=OpenFakeRegistry();
			if(regf==NULL) {
				OutTraceR("%s: error in OpenFakeRegistry err=%s\n", ApiRef, GetLastError());	
				return ERROR_FILE_NOT_FOUND;
			}
			HKEY hFakeKey;
			res = myRegOpenKeyEx(ApiRef, hKey, NULL, &hFakeKey);
			if(res == 0) hKey = hFakeKey;
			fclose(regf);
		}
	}

	if (!IsFake(hKey)){
		res=(*pRegQueryValueA)(hKey, lpSubKey, lpValue, lpcbValue);
		IfLogKeyValue(ApiRef, FALSE, res, 0, (LPBYTE)lpValue, (LPDWORD)lpcbValue);
		return res;
	}

	regf=OpenFakeRegistry();
	if(regf==NULL) {
		OutTraceR("%s: error in OpenFakeRegistry err=%s\n", ApiRef, GetLastError());	
		return ERROR_FILE_NOT_FOUND;
	}
	res = SeekFakeKey(regf, hKey);
	if(res != ERROR_SUCCESS) {
		OutTraceR("%s: error in SeekFakeKey res=%#x hKey=%#x\n", ApiRef, res, hKey);	
		return res;
	}
	res = SeekValueName(regf, lpSubKey);
	if(res != ERROR_SUCCESS) {
		OutTraceR("%s: error in SeekValueName res=%#x ValueName=%s\n", ApiRef, res, lpSubKey);	
		return res;
	}
	res = GetKeyValue(regf, ApiRef, lpSubKey, NULL, (LPBYTE)lpValue, (LPDWORD)lpcbValue, FALSE);
	fclose(regf);
	return res;
}

LONG WINAPI extRegQueryValueExA(
				HKEY hKey, 
				LPCSTR lpValueName, 
				LPDWORD lpReserved, 
				LPDWORD lpType, // beware: could be NULL
				LPBYTE lpData,  // beware: could be NULL
				LPDWORD lpcbData) // beware: could be NULL
{
	LONG res;
	ApiName("RegQueryValueExA");
	FILE *regf;
	DWORD cbData=0;

	OutTraceR("%s: hKey=%#x(\"%s\") ValueName=\"%s\" Reserved=%#x lpType=%#x lpData=%#x lpcbData=%#x->(%d)\n", 
		ApiRef, hKey, hKey2String(hKey), lpValueName, lpReserved, lpType, 
		lpData, lpcbData, lpcbData ? *lpcbData : 0);

	if(dxw.dwFlags13 & (DISABLED3DMMX|DISABLED3DXPSGP|DISABLEPSGP|D3DXDONOTMUTE)){
		if(dxw.dwFlags13 & DISABLEPSGP) { // 
			if((hKey == dxw.d3dRegistryKey) && !strcmp(lpValueName, "DisablePSGP")){
				*(DWORD *)lpData = (DWORD)1;
				*lpcbData = sizeof(DWORD);
				*lpType = REG_DWORD;
				OutTrace("%s: DisablePSGP tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
		if(dxw.dwFlags13 & DISABLED3DXPSGP) { // 
			if((hKey == dxw.d3dRegistryKey) && !strcmp(lpValueName, "DisableD3DXPSGP")){
				*(DWORD *)lpData = (DWORD)1;
				*lpcbData = sizeof(DWORD);
				*lpType = REG_DWORD;
				OutTrace("%s: DisableD3DXPSGP tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
		if(dxw.dwFlags13 & DISABLED3DMMX) { // 
			if((hKey == dxw.d3dRegistryKey) && !strcmp(lpValueName, "DisableMMX")){
				*(DWORD *)lpData = (DWORD)1;
				*lpcbData = sizeof(DWORD);
				*lpType = REG_DWORD;
				OutTrace("%s: DisableD3DMMX tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
		if(dxw.dwFlags13 & D3DXDONOTMUTE) { // 
			if((hKey == dxw.d3dRegistryKey) && !strcmp(lpValueName, "D3DXDoNotMute")){
				*(DWORD *)lpData = (DWORD)1;
				*lpcbData = sizeof(DWORD);
				*lpType = REG_DWORD;
				OutTrace("%s: D3DXDoNotMute tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
	}

	if((dxw.dwFlags14 & FIXMACROMEDIAREG) && lpValueName && !strcmp(lpValueName, "InstalledDisplayDrivers")){
		res = FixMacromediaRegistry(hKey, lpData, lpcbData);
		IfLogKeyValue(ApiRef, FALSE, res, lpType, lpData, lpcbData);
		return res;
	}

	if (!IsFake(hKey)){
		res=(*pRegQueryValueExA)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
		IfLogKeyValue(ApiRef, FALSE, res, lpType, lpData, lpcbData);
		return res;
	}

	regf=OpenFakeRegistry();
	if(regf==NULL) {
		OutTraceR("%s: error in OpenFakeRegistry err=%s\n", ApiRef, GetLastError());	
		return ERROR_FILE_NOT_FOUND;
	}
	res = SeekFakeKey(regf, hKey);
	if(res != ERROR_SUCCESS) {
		OutTraceR("%s: error in SeekFakeKey res=%#x hKey=%#x\n", ApiRef, res, hKey);	
		return res;
	}
	res = SeekValueName(regf, lpValueName);
	if(res != ERROR_SUCCESS) {
		OutTraceR("%s: error in SeekValueName res=%#x ValueName=%s\n", ApiRef, res, lpValueName);	
		return res;
	}
	res = GetKeyValue(regf, ApiRef, lpValueName, lpType, lpData, lpcbData, FALSE);
	fclose(regf);
	return res;
}

LONG WINAPI extRegCloseKey(HKEY hKey)
{
	OutTraceR("RegCloseKey: hKey=%#x\n", hKey);
	if (IsFake(hKey)) return ERROR_SUCCESS;
	return (*pRegCloseKey)(hKey);
}

LONG WINAPI extRegFlushKey(HKEY hKey)
{
	OutTraceR("RegFlushKey: hKey=%#x\n", hKey);
	if (IsFake(hKey)) return ERROR_SUCCESS;
	return (*pRegFlushKey)(hKey);
}

LONG WINAPI extRegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData)
{
	ApiName("RegSetValueExA");
#ifndef DXW_NOTRACES
	if (IsTraceR){
		char sInfo[1024];
		sprintf(sInfo, "%s: hKey=%#x ValueName=\"%s\" Type=%#x(%s) cbData=%d", ApiRef, hKey, lpValueName, dwType, ExplainRegType(dwType), cbData);
		switch(dwType){
			case REG_DWORD: OutTrace("%s Data=%#x\n", sInfo, *(DWORD *)lpData); break;
			case REG_NONE: OutTrace("%s Data=\"%s\"\n", sInfo, lpData); break;
			case REG_BINARY: OutTrace("%s Data=%s\n", sInfo, hexdump((BYTE *)lpData, cbData)); break;
			case REG_SZ: OutTrace("%s Data=\"%.*s\"\n", sInfo, cbData, lpData); break;
			default: OutTrace("%s\n", sInfo);
		}
	}
#endif
	if(IsFake(hKey) && (dxw.dwFlags3 & EMULATEREGISTRY)) {
		OutTraceR("%s: SUPPRESS registry key set\n", ApiRef);
		return ERROR_SUCCESS;
	}
	return (*pRegSetValueExA)(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}

LONG WINAPI extRegSetValueA(HKEY hKey, LPCSTR lpSubKey, DWORD dwType, LPCSTR lpData, DWORD cbData)
{
	ApiName("RegSetValueA");

#ifndef DXW_NOTRACES
	if (IsTraceR){
		char sInfo[1024];
		sprintf(sInfo, "%s: hKey=%#x(\"%s\") subkey=\"%s\" type=%d(%s) cbdata=%d ", 
			ApiRef, hKey, hKey2String(hKey), lpSubKey, dwType, ExplainRegType(dwType), cbData);
		switch(dwType){
			case REG_DWORD: OutTrace("%s Data=%#x\n", sInfo, *(DWORD *)lpData); break;
			case REG_NONE: OutTrace("%s Data=\"%s\"\n", sInfo, lpData); break;
			case REG_BINARY: OutTrace("%s Data=%s\n", sInfo, hexdump((BYTE *)lpData, cbData)); break;
			case REG_SZ: OutTrace("%s Data=\"%.*s\"\n", sInfo, cbData, lpData); break;
			default: OutTrace("%s\n", sInfo);
		}
	}
#endif
	if(IsFake(hKey) && (dxw.dwFlags3 & EMULATEREGISTRY)) {
		OutTraceR("%s: SUPPRESS registry key set\n", ApiRef);
		return ERROR_SUCCESS;
	}
	return (*pRegSetValueA)(hKey, lpSubKey, dwType, lpData, cbData);
}

// RegCreateKey/Ex calls will open an existing key or will create a new one if not found.

LONG WINAPI extRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired,
				LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	ApiName("RegCreateKeyExA");
	LONG ret;
	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%s\" Class=%#x\n", ApiRef, hKey, hKey2String(hKey), lpSubKey, lpClass);
	// v2.05.60: a fake key could be below an existing root key, should check in any case. 
	// found in "Beast Wars Transformers"
	if (dxw.dwFlags3 & EMULATEREGISTRY){
		*phkResult = HKEY_FAKE;
		// V2.3.12: return existing fake key if any ....
		ret = myRegOpenKeyEx(ApiRef, hKey, lpSubKey, phkResult);
		if(ret == ERROR_SUCCESS){
			if(lpdwDisposition) *lpdwDisposition=REG_OPENED_EXISTING_KEY;
			OutTraceR("%s: ret=%#x hkey=%#x disp=%#x\n", ApiRef, ret, *phkResult, lpdwDisposition ? *lpdwDisposition : 0);
			return ERROR_SUCCESS;
		}
		else {
			if(IsFake(hKey)) {
				if (dxw.dwFlags4 & OVERRIDEREGISTRY){ // v2.05.73
					if(lpdwDisposition) *lpdwDisposition=REG_CREATED_NEW_KEY; 
					return ERROR_SUCCESS;
				}
				OutErrorR("%s: ret=%#x\n", ApiRef, ret);
				return ret;
			}
			// else fall through ...
		}
	}
		
	ret = (*pRegCreateKeyExA)(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
				lpSecurityAttributes, phkResult, lpdwDisposition);

	if(ret == ERROR_SUCCESS) {
		OutTraceR("%s: ret=%#x hkey=%#x disp=%#x\n", ApiRef, ret, *phkResult, lpdwDisposition ? *lpdwDisposition : 0);
	}
	else{
		OutErrorR("%s: ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	return ret;
}

LONG WINAPI extRegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
	ApiName("RegCreateKeyA");
	LONG ret;
	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%s\"\n", ApiRef, hKey, hKey2String(hKey), lpSubKey);
	if (dxw.dwFlags3 & EMULATEREGISTRY){
		*phkResult = HKEY_FAKE;
		// V2.3.12: return existing fake key if any ....
		ret = myRegOpenKeyEx(ApiRef, hKey, lpSubKey, phkResult);
		if(ret == ERROR_SUCCESS){
			OutTraceR("%s: ret=%#x hkey=%#x\n", ApiRef, ret, *phkResult);
			return ERROR_SUCCESS;
		}
		else {
			if(IsFake(hKey)) {
				OutErrorR("%s: ret=%#x\n", ApiRef, ret);
				return ret;
			}
			// else fall through ...
		}
	}

	ret = (*pRegCreateKeyA)(hKey, lpSubKey, phkResult);
	if(ret == ERROR_SUCCESS) {
		OutTraceR("%s: ret=%#x hkey=%#x\n", ApiRef, ret, *phkResult);
	}
	else{
		OutErrorR("%s: ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	return ret;
}

LONG WINAPI extRegEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	LONG res;
	ApiName("RegEnumValueA");

	OutTraceR("%s: hKey=%#x(\"%s\") index=%d cchValueName=%d Reserved=%#x lpType=%#x lpData=%#x lpcbData=%#x\n", 
		ApiRef, hKey, hKey2String(hKey), dwIndex, *lpcchValueName, lpReserved, lpType, lpData, lpcbData);
	if (!IsFake(hKey)){
		res=(*pRegEnumValueA)(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
		IfLogKeyValue(ApiRef, FALSE, res, lpType, lpData, lpcbData);
		return res;
	}

	// try emulated registry
	FILE *regf;
	regf=OpenFakeRegistry();
	if(regf==NULL) return ERROR_FILE_NOT_FOUND;
	res = SeekFakeKey(regf, hKey);
	if(res == ERROR_SUCCESS) {
		res = SeekValueIndex(regf, dwIndex, lpValueName, lpcchValueName);
	}
	if(res == ERROR_SUCCESS) {
		res = GetKeyValue(regf, ApiRef, lpValueName, lpType, lpData, lpcbData, FALSE);
	}
	fclose(regf);
	return res;
}

LONG WINAPI extRegEnumKeyA(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cchName)
{
	LONG res;
	ApiName("RegEnumKeyA");

	OutTraceR("%s: hKey=%#x(\"%s\") index=%d cchName=%d\n", 
		ApiRef, hKey, hKey2String(hKey), dwIndex, cchName);
	if (!IsFake(hKey)){
		res=(*pRegEnumKeyA)(hKey, dwIndex, lpName, cchName);
		OutTraceR("%s: res=%#x name=%s\n",
			ApiRef, res, (res==ERROR_SUCCESS) ? lpName : "(???)");
		return res;
	}
	
	// try emulated registry
	FILE *regf;
	char sUpKey[MAX_PATH];
	char sSubKey[MAX_PATH];
	UINT len;
	regf=OpenFakeRegistry();
	if(regf==NULL) return ERROR_FILE_NOT_FOUND;
	res = SeekFakeKey(regf, hKey - dwIndex);
	if(res == ERROR_SUCCESS) {
		strcpy(sUpKey, hKey2String(hKey)); 
		strcpy(sSubKey, hKey2String(hKey - dwIndex -1)); 
		len = strlen(sUpKey);
		if(!strncmp(sSubKey, sUpKey, len)){
			strncpy(lpName, sSubKey + len + 1, cchName); 
			// to do: handling of lpClass and lpcClass when not NULL
			res = ERROR_SUCCESS;
			OutTraceR("%s: enum keynum=%d name=%s\n", ApiRef, dwIndex, lpName);
		}
		else {
			res = ERROR_NO_MORE_ITEMS;
		}
	}
	fclose(regf);
	OutTraceR("%s: res=%#x\n", ApiRef, res);
	return res;}

LONG WINAPI extRegEnumKeyExA(HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, 
							 LPSTR lpClass, LPDWORD lpcClass, PFILETIME lpftLastWriteTime)
{
	LONG res;
	ApiName("RegEnumKeyExA");

	OutTraceR("%s: hKey=%#x(\"%s\") index=%d cName=%d cClass=%d\n", 
		ApiRef, hKey, hKey2String(hKey), dwIndex, lpcName ? *lpcName : 0, lpcClass ? *lpcClass : 0);
	if (!IsFake(hKey)){
		res=(*pRegEnumKeyExA)(hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
		if(res == ERROR_SUCCESS){
			OutTraceR("%s: res=SUCCESS Name=%s cName=%d Class=%s cClass=%d\n",
				ApiRef, lpName, lpcName ? *lpcName : 0, lpClass ? lpClass : "(NULL)", lpcClass ? *lpcClass : 0);
		}
		else {
			OutTraceR("%s: res=%#x\n", ApiRef, res);
		}
		return res;
	}

	// try emulated registry
	FILE *regf;
	char sUpKey[MAX_PATH];
	char sSubKey[MAX_PATH];
	UINT len;
	regf=OpenFakeRegistry();
	if(regf==NULL) return ERROR_FILE_NOT_FOUND;
	res = SeekFakeKey(regf, hKey - dwIndex);
	if(res == ERROR_SUCCESS) {
		strcpy(sUpKey, hKey2String(hKey)); 
		strcpy(sSubKey, hKey2String(hKey - dwIndex -1)); 
		len = strlen(sUpKey);
		if(!strncmp(sSubKey, sUpKey, len)){
			strncpy(lpName, sSubKey + len + 1, *lpcName); 
			*lpcName = strlen(lpName);
			// to do: handling of lpClass and lpcClass when not NULL
			res = ERROR_SUCCESS;
			OutTraceR("%s: enum keynum=%d name=%s cName=%d\n", ApiRef, dwIndex, lpName, *lpcName);
		}
		else {
			res = ERROR_NO_MORE_ITEMS;
		}
	}
	fclose(regf);
	OutTraceR("%s: res=%#x\n", ApiRef, res);
	return res;
}

BOOL WINAPI extGetUserNameA(LPSTR lpBuffer, LPDWORD pcbBuffer)
{
	BOOL res;
	ApiName("GetUserNameA");

	OutTraceR("%s: maxlen=%d\n", ApiRef, *pcbBuffer);
	res = (*pGetUserNameA)(lpBuffer, pcbBuffer);
	if(res){
		OutTraceR("%s: user=\"%s\" len=%d\n", ApiRef, lpBuffer, *pcbBuffer);
	}
	else {
		OutErrorR("%s: err=%d\n", ApiRef, GetLastError());
	}

	return res;
}

#define IsQueried(p) ((p==NULL) ? "(NULL)": "Yes")
LONG WINAPI extRegQueryInfoKeyA(
	HKEY hKey, 
	LPSTR lpClass, 
	LPDWORD lpcchClass, 
	LPDWORD lpReserved, 
	LPDWORD lpcSubKeys, 
	LPDWORD lpcbMaxSubKeyLen, 
	LPDWORD lpcbMaxClassLen, 
	LPDWORD lpcValues, 
	LPDWORD lpcbMaxValueNameLen, 
	LPDWORD lpcbMaxValueLen, 
	LPDWORD lpcbSecurityDescriptor, 
	PFILETIME lpftLastWriteTime)
{
	// seen in "Fallen Haven"
	LONG res;
	ApiName("RegQueryInfoKeyA");

	OutTraceR("%s: hKey=%#x(\"%s\") cchClass=%d QUERY: Class=%s SubKeys=%s MaxSubKeyLen=%s MaxClassLen=%s "
		"Values=%s ValueNameLen=%s MaxValueLen=%s SecurityDescr=%s LastWriteTime=%s\n", 
		ApiRef,
		hKey, hKey2String(hKey), 
		lpcchClass ? *lpcchClass : 0,
		IsQueried(lpClass),
		IsQueried(lpcSubKeys),
		IsQueried(lpcbMaxSubKeyLen),
		IsQueried(lpcbMaxClassLen),
		IsQueried(lpcValues),
		IsQueried(lpcbMaxValueNameLen),
		IsQueried(lpcbMaxValueLen),
		IsQueried(lpcbSecurityDescriptor),
		IsQueried(lpftLastWriteTime)
		);

	if (!IsFake(hKey)){
		res=(*pRegQueryInfoKeyA)(
			hKey, 
			lpClass, 
			lpcchClass, 
			lpReserved, 
			lpcSubKeys, 
			lpcbMaxSubKeyLen, 
			lpcbMaxClassLen,
			lpcValues, 
			lpcbMaxValueNameLen, 
			lpcbMaxValueLen, 
			lpcbSecurityDescriptor, 
			lpftLastWriteTime);
	}
	else {
		FILE *regf;
		regf=OpenFakeRegistry();
		if(regf==NULL) {
			OutTraceR("%s: error in OpenFakeRegistry err=%s\n", ApiRef, GetLastError());	
			return ERROR_FILE_NOT_FOUND;
		}

		res = ERROR_FILE_NOT_FOUND;
		char RegBuf[MAX_PATH+1];
		HKEY hCurKey=HKEY_FAKE+1;
		fgets(RegBuf, 256, regf);
		while (!feof(regf)){
			if(RegBuf[0]=='['){
				hCurKey--;
			}
			if(hCurKey==hKey) {
				//OutTraceREGDebug("DEBUG: SeekFakeKey fount key at line=%s\n", RegBuf);
				res = ERROR_SUCCESS;
				if(lpcchClass){
					// No support for custom keys at the moment ...
					//if(lpClass && (*lpcchClass > strlen(RegBuf))) strncpy(lpClass, RegBuf, *lpcchClass);			
					//*lpcchClass = strlen(RegBuf);
					if(lpClass) *lpClass = 0;
					if(lpcchClass) *lpcchClass = 0;
				}
				break;
			}
			fgets(RegBuf, 256, regf);
		}

		if(res == ERROR_SUCCESS) {
			int iValueNameLen, iMaxValueNameLen, iValues;
			if (lpcSubKeys) *lpcSubKeys = 0;
			if (lpcbMaxSubKeyLen) *lpcbMaxSubKeyLen = 0; // ???
			if (lpcbMaxClassLen) *lpcbMaxClassLen = 0; // ???
			if (lpcSubKeys) *lpcSubKeys = 0; // ???

			iValues = 0; 
			iMaxValueNameLen = 0;

			fgets(RegBuf, 256, regf);
			while (!feof(regf)){
				if (RegBuf[0] == '[') break;
				if(RegBuf[0]=='"') {
					iValues++;
					if (lpcbMaxValueNameLen){
						int i;
						for(i=1; RegBuf[i]; i++) if(RegBuf[i]=='"') break;
						iValueNameLen = i-1;
						if(iValueNameLen > iMaxValueNameLen) iMaxValueNameLen = iValueNameLen; // save maximum value
					}
				}
				fgets(RegBuf, 256, regf);
			}
			OutDebugR("> Values=%d\n", iValues);	
			OutDebugR("> MaxValueNameLen=%d\n", iMaxValueNameLen);	
			if (lpcValues) *lpcValues = iValues;
			if (lpcbMaxValueNameLen) *lpcbMaxValueNameLen = iMaxValueNameLen;
		}
	}

	if(res == ERROR_SUCCESS){
		char sTime[81];
		SYSTEMTIME st;
		strcpy(sTime, "");
		if (lpftLastWriteTime) {
			if(FileTimeToSystemTime(lpftLastWriteTime, &st)){
				sprintf(sTime, "%02.2d/%02.2d/%04.4d %02.2d:%02.2d:%02.2d",
					st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			}
		}
		OutTraceR("%s: res=SUCCESS hKey=%#x(\"%s\") "
			"Class=%s cchClass=%d SubKeys=%d MaxSubKeyLen=%d "
			"MaxClassLen=%d Values=%d MaxValueNameLen=%d SecDescr=%s LastWriteTime=%s\n", 
			ApiRef,
			hKey, hKey2String(hKey), 
			lpClass ? lpClass : "(NULL)",
			lpcchClass ? *lpcchClass : 0,
			lpcSubKeys ? *lpcSubKeys : 0,
			lpcbMaxSubKeyLen ? *lpcbMaxSubKeyLen : 0,
			lpcbMaxClassLen ? *lpcbMaxClassLen : 0,
			lpcValues ? *lpcValues : 0,
			lpcbMaxValueNameLen ? *lpcbMaxValueNameLen : 0,
			lpcbSecurityDescriptor ? "(NULL)" : "(tbd)",
			lpftLastWriteTime ? sTime : "(NULL)"
			);
	}
	else {
		OutTraceR("%s: res=%#x err=%d\n", ApiRef, res, GetLastError());
	}	
	return res;
}

#ifdef TOBEDONE
LONG WINAPI RegQueryInfoKey(
  _In_        HKEY      hKey,
  _Out_opt_   LPSTR    lpClass,
  _Inout_opt_ LPDWORD   lpcClass,
  _Reserved_  LPDWORD   lpReserved,
  _Out_opt_   LPDWORD   lpcSubKeys,
  _Out_opt_   LPDWORD   lpcMaxSubKeyLen,
  _Out_opt_   LPDWORD   lpcMaxClassLen,
  _Out_opt_   LPDWORD   lpcValues,
  _Out_opt_   LPDWORD   lpcMaxValueNameLen,
  _Out_opt_   LPDWORD   lpcMaxValueLen,
  _Out_opt_   LPDWORD   lpcbSecurityDescriptor,
  _Out_opt_   PFILETIME lpftLastWriteTime
);
#endif

LONG WINAPI extRegOpenKeyW( 
				HKEY hKey,
				LPCWSTR lpSubKey,
				PHKEY phkResult)
{
	LONG res;
	ApiName("RegOpenKeyW");

	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%ls\"\n", ApiRef, hKey, hKey2String(hKey), lpSubKey);

	if(dxw.dwFlags4 & OVERRIDEREGISTRY){
		size_t len = wcslen(lpSubKey);
		LPCSTR lpszSubKeyA = (LPCSTR)malloc((2*len)+1);
		_wcstombs_s_l(&len, (char *)lpszSubKeyA, 2*len, lpSubKey, _TRUNCATE, NULL);
		res = myRegOpenKeyEx(ApiRef, hKey, lpszSubKeyA, phkResult);
		free((LPVOID)lpszSubKeyA);
		if(res == ERROR_SUCCESS) return res;
	}

	res=(*pRegOpenKeyW)(hKey, lpSubKey, phkResult);
	OutTraceR("%s: res=%#x phkResult=%#x\n", ApiRef, res, phkResult ? *phkResult : 0); 

	if(phkResult && (res == ERROR_SUCCESS) && (dxw.dwFlags13 & (DISABLED3DMMX|DISABLED3DXPSGP|DISABLEPSGP|D3DXDONOTMUTE))){
		if((hKey == HKEY_LOCAL_MACHINE) && !wcscmp(lpSubKey, L"Software\\Microsoft\\Direct3D")){
			OutTraceD3D("%s: D3D registry key=%#x\n", *phkResult);
			dxw.d3dRegistryKey = *phkResult;
		}
	}

	if((res==ERROR_SUCCESS) || !(dxw.dwFlags3 & EMULATEREGISTRY) || (dxw.dwFlags4 & OVERRIDEREGISTRY)) return res;
	
	size_t len = wcslen(lpSubKey);
	LPCSTR lpszSubKeyA = (LPCSTR)malloc((2*len)+1);
	_wcstombs_s_l(&len, (char *)lpszSubKeyA, 2*len, lpSubKey, _TRUNCATE, NULL);
	res = myRegOpenKeyEx(ApiRef, hKey, lpszSubKeyA, phkResult);
	free((LPVOID)lpszSubKeyA);
	return res;
}

LONG WINAPI extRegOpenKeyExW(
				HKEY hKey,
				LPCWSTR lpSubKey,
				DWORD ulOptions,
				REGSAM samDesired,
				PHKEY phkResult)
{
	LONG res;
	ApiName("RegOpenKeyExW");

	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%ls\" Options=%#x\n", 
		ApiRef, hKey, hKey2String(hKey), lpSubKey, ulOptions);

	if(dxw.dwFlags4 & OVERRIDEREGISTRY){
		size_t len = wcslen(lpSubKey);
		LPCSTR lpszSubKeyA = (LPCSTR)malloc((2*len)+1);
		_wcstombs_s_l(&len, (char *)lpszSubKeyA, 2*len, lpSubKey, _TRUNCATE, NULL);
		res = myRegOpenKeyEx(ApiRef, hKey, lpszSubKeyA, phkResult);
		free((LPVOID)lpszSubKeyA);
		if(res == ERROR_SUCCESS) return res;
	}

	if(dxw.dwFlags6 & WOW64REGISTRY){
		ulOptions &= ~KEY_WOW64_32KEY;
		ulOptions |= KEY_WOW64_64KEY;
	}
	if(dxw.dwFlags6 & WOW32REGISTRY){
		ulOptions &= ~KEY_WOW64_64KEY;
		ulOptions |= KEY_WOW64_32KEY;
	}

	res=(*pRegOpenKeyExW)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	OutTraceR("%s: res=%#x phkResult=%#x\n", ApiRef, res, phkResult ? *phkResult : 0); 

	if((res==ERROR_SUCCESS) || !(dxw.dwFlags3 & EMULATEREGISTRY) || (dxw.dwFlags4 & OVERRIDEREGISTRY)) return res;
	
	size_t len = wcslen(lpSubKey);
	LPCSTR lpszSubKeyA = (LPCSTR)malloc((2*len)+1);
	_wcstombs_s_l(&len, (char *)lpszSubKeyA, 2*len, lpSubKey, _TRUNCATE, NULL);
	res = myRegOpenKeyEx(ApiRef, hKey, lpszSubKeyA, phkResult);
	free((LPVOID)lpszSubKeyA);
	return res;
}

LONG WINAPI extRegQueryValueW(
				HKEY hKey, 
				LPCWSTR lpSubKey, 
				LPWSTR lpValue, 
				PLONG lpcbValue)
{
	LONG res;
	ApiName("RegQueryValueW");
	FILE *regf;

	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%ls\" lpcbData=%#x->(%d)\n", 
		ApiRef, hKey, hKey2String(hKey), lpSubKey, lpcbValue, lpcbValue ? *lpcbValue : 0);
	
	if (!IsFake(hKey)){
		res=(*pRegQueryValueW)(hKey, lpSubKey, lpValue, lpcbValue);
		IfLogKeyValue(ApiRef, TRUE, res, 0, (LPBYTE)lpValue, (LPDWORD)lpcbValue);
		return res;
	}

	regf=OpenFakeRegistry();
	if(regf==NULL) {
		OutTraceR("%s: error in OpenFakeRegistry err=%s\n", ApiRef, GetLastError());	
		return ERROR_FILE_NOT_FOUND;
	}
	res = SeekFakeKey(regf, hKey);
	if(res != ERROR_SUCCESS) {
		OutTraceR("%s: error in SeekFakeKey res=%#x hKey=%#x\n", ApiRef, res, hKey);	
		return res;
	}

	size_t len = wcslen(lpSubKey);
	LPCSTR lpszSubKeyA = (LPCSTR)malloc((2*len)+1);
	_wcstombs_s_l(&len, (char *)lpszSubKeyA, 2*len, lpSubKey, _TRUNCATE, NULL);
	res = SeekValueName(regf, lpszSubKeyA);
	if(res != ERROR_SUCCESS) {
		free((LPVOID)lpszSubKeyA);
		OutTraceR("%s: error in SeekValueName res=%#x ValueName=%ls\n", ApiRef, res, lpSubKey);	
		return res;
	}
	res = GetKeyValue(regf, ApiRef, lpszSubKeyA, NULL, (LPBYTE)lpValue, (LPDWORD)lpcbValue, TRUE);
	fclose(regf);
	free((LPVOID)lpszSubKeyA);
	return res;
}

LONG WINAPI extRegQueryValueExW(
				HKEY hKey, 
				LPCWSTR lpValueName, 
				LPDWORD lpReserved, 
				LPDWORD lpType, // beware: could be NULL
				LPBYTE lpData,  // beware: could be NULL
				LPDWORD lpcbData) // beware: could be NULL
{
	LONG res;
	ApiName("RegQueryValueExW");
	FILE *regf;
	DWORD cbData=0;

	OutTraceR("%s: hKey=%#x(\"%s\") ValueName=\"%ls\" Reserved=%#x lpType=%#x lpData=%#x lpcbData=%#x->(%d)\n", 
		ApiRef, hKey, hKey2String(hKey), lpValueName, lpReserved, lpType, 
		lpData, lpcbData, lpcbData ? *lpcbData : 0);

	if(dxw.dwFlags13 & (DISABLED3DMMX|DISABLED3DXPSGP|DISABLEPSGP|D3DXDONOTMUTE)){
		if(dxw.dwFlags13 & DISABLEPSGP) { // 
			if((hKey == dxw.d3dRegistryKey) && !wcscmp(lpValueName, L"DisablePSGP")){
				if(lpData) *(DWORD *)lpData = (DWORD)1;
				if(lpcbData) *lpcbData = sizeof(DWORD);
				if(lpType) *lpType = REG_DWORD;
				OutTrace("%s: DisablePSGP tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
		if(dxw.dwFlags13 & DISABLED3DXPSGP) { // 
			if((hKey == dxw.d3dRegistryKey) && !wcscmp(lpValueName, L"DisableD3DXPSGP")){
				if(lpData) *(DWORD *)lpData = (DWORD)1;
				if(lpcbData) *lpcbData = sizeof(DWORD);
				if(lpType) *lpType = REG_DWORD;
				OutTrace("%s: DisableD3DXPSGP tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
		if(dxw.dwFlags13 & DISABLED3DMMX) { // 
			if((hKey == dxw.d3dRegistryKey) && !wcscmp(lpValueName, L"DisableMMX")){
				if(lpData) *(DWORD *)lpData = (DWORD)1;
				if(lpcbData) *lpcbData = sizeof(DWORD);
				if(lpType) *lpType = REG_DWORD;
				OutTrace("%s: DisableD3DMMX tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
		if(dxw.dwFlags13 & D3DXDONOTMUTE) { // 
			if((hKey == dxw.d3dRegistryKey) && !wcscmp(lpValueName, L"D3DXDoNotMute")){
				if(lpData) *(DWORD *)lpData = (DWORD)1;
				if(lpcbData) *lpcbData = sizeof(DWORD);
				if(lpType) *lpType = REG_DWORD;
				OutTrace("%s: D3DXDoNotMute tweak\n", ApiRef);
				return ERROR_SUCCESS;
			}
		}
	}

	if (!IsFake(hKey)){
		res=(*pRegQueryValueExW)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
		IfLogKeyValue(ApiRef, TRUE, res, lpType, lpData, lpcbData);
		return res;
	}

	regf=OpenFakeRegistry();
	if(regf==NULL) {
		OutTraceR("%s: error in OpenFakeRegistry err=%s\n", ApiRef, GetLastError());	
		return ERROR_FILE_NOT_FOUND;
	}
	res = SeekFakeKey(regf, hKey);
	if(res != ERROR_SUCCESS) {
		OutTraceR("%s: error in SeekFakeKey res=%#x hKey=%#x\n", ApiRef, res, hKey);	
		return res;
	}
	
	size_t len = wcslen(lpValueName);
	LPCSTR lpValueNameA = (LPCSTR)malloc((2*len)+1);
	_wcstombs_s_l(&len, (char *)lpValueNameA, 2*len, lpValueName, _TRUNCATE, NULL);
	res = SeekValueName(regf, lpValueNameA);
	if(res != ERROR_SUCCESS) {
		free((LPVOID)lpValueNameA);
		OutTraceR("%s: error in SeekValueName res=%#x ValueName=%ls\n", ApiRef, res, lpValueName);	
		return res;
	}
	res = GetKeyValue(regf, ApiRef, lpValueNameA, lpType, (LPBYTE)lpData, (LPDWORD)lpcbData, TRUE);
	fclose(regf);
	free((LPVOID)lpValueNameA);
	return res;
}

LONG WINAPI extRegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
	ApiName("RegCreateKeyW");
	LONG ret;
	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%ls\"\n", ApiRef, hKey, hKey2String(hKey), lpSubKey);
	if (dxw.dwFlags3 & EMULATEREGISTRY){
		*phkResult = HKEY_FAKE;
		// V2.3.12: return existing fake key if any ....
		size_t len = wcslen(lpSubKey);
		LPCSTR lpSubKeyA = (LPCSTR)malloc((2*len)+1);
		_wcstombs_s_l(&len, (char *)lpSubKeyA, 2*len, lpSubKey, _TRUNCATE, NULL);
		ret = myRegOpenKeyEx(ApiRef, hKey, lpSubKeyA, phkResult);
		free((LPVOID)lpSubKeyA);
		if(ret == ERROR_SUCCESS){
			OutTraceR("%s: ret=%#x hkey=%#x\n", ApiRef, ret, *phkResult);
			return ERROR_SUCCESS;
		}
		else {
			if(IsFake(hKey)) {
				OutErrorR("%s: ret=%#x\n", ApiRef, ret);
				return ret;
			}
			// else fall through ...
		}
	}

	ret = (*pRegCreateKeyW)(hKey, lpSubKey, phkResult);
	if(ret == ERROR_SUCCESS) {
		OutTraceR("%s: ret=%#x hkey=%#x\n", ApiRef, ret, *phkResult);
	}
	else{
		OutErrorR("%s: ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	return ret;
}

LONG WINAPI extRegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired,
				LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	ApiName("RegCreateKeyExW");
	LONG ret;
	OutTraceR("%s: hKey=%#x(%s) SubKey=\"%ls\" Class=%#x\n", ApiRef, hKey, hKey2String(hKey), lpSubKey, lpClass);
	// v2.05.60: a fake key could be below an existing root key, should check in any case. 
	// found in "Beast Wars Transformers"
	if (dxw.dwFlags3 & EMULATEREGISTRY){
		*phkResult = HKEY_FAKE;
		// V2.3.12: return existing fake key if any ....
		size_t len = wcslen(lpSubKey);
		LPCSTR lpSubKeyA = (LPCSTR)malloc((2*len)+1);
		_wcstombs_s_l(&len, (char *)lpSubKeyA, 2*len, lpSubKey, _TRUNCATE, NULL);
		ret = myRegOpenKeyEx(ApiRef, hKey, lpSubKeyA, phkResult);
		free((LPVOID)lpSubKeyA);
		if(ret == ERROR_SUCCESS){
			if(lpdwDisposition) *lpdwDisposition=REG_OPENED_EXISTING_KEY;
			OutTraceR("%s: ret=%#x hkey=%#x disp=%#x\n", ApiRef, ret, *phkResult, lpdwDisposition ? *lpdwDisposition : 0);
			return ERROR_SUCCESS;
		}
		else {
			if(IsFake(hKey)) {
				if (dxw.dwFlags4 & OVERRIDEREGISTRY){ // v2.05.73
					if(lpdwDisposition) *lpdwDisposition=REG_CREATED_NEW_KEY; 
					return ERROR_SUCCESS;
				}
				OutErrorR("%s: ret=%#x\n", ApiRef, ret);
				return ret;
			}
			// else fall through ...
		}
	}
		
	ret = (*pRegCreateKeyExW)(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
				lpSecurityAttributes, phkResult, lpdwDisposition);

	if(ret == ERROR_SUCCESS) {
		OutTraceR("%s: ret=%#x hkey=%#x disp=%#x\n", ApiRef, ret, *phkResult, lpdwDisposition ? *lpdwDisposition : 0);
	}
	else{
		OutErrorR("%s: ret=%#x err=%d\n", ApiRef, ret, GetLastError());
	}
	return ret;
}

LONG WINAPI extRegSetValueW(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType, LPCWSTR lpData, DWORD cbData)
{
	ApiName("RegSetValueW");

#ifndef DXW_NOTRACES
	if (IsTraceR){
		char sInfo[1024];
		sprintf(sInfo, "%s: hKey=%#x(\"%s\") subkey=\"%ls\" type=%d(%s) cbdata=%d ", 
			ApiRef, hKey, hKey2String(hKey), lpSubKey, dwType, ExplainRegType(dwType), cbData);
		switch(dwType){
			case REG_DWORD: OutTrace("%s Data=%#x\n", sInfo, *(DWORD *)lpData); break;
			case REG_NONE: OutTrace("%s Data=\"%ls\"\n", sInfo, lpData); break;
			case REG_BINARY: OutTrace("%s Data=%s\n", sInfo, hexdump((BYTE *)lpData, cbData)); break;
			case REG_SZ: OutTrace("%s Data=\"%.*s\"\n", sInfo, cbData, lpData); break;
			default: OutTrace("%s\n", sInfo);
		}
	}
#endif
	if(IsFake(hKey) && (dxw.dwFlags3 & EMULATEREGISTRY)) {
		OutTraceR("%s: SUPPRESS registry key set\n", ApiRef);
		return ERROR_SUCCESS;
	}
	return (*pRegSetValueW)(hKey, lpSubKey, dwType, lpData, cbData);
}

LONG WINAPI extRegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData)
{
	ApiName("RegSetValueExW");
#ifndef DXW_NOTRACES
	if (IsTraceR){
		char sInfo[1024];
		sprintf(sInfo, "%s: hKey=%#x ValueName=\"%ls\" Type=%#x(%s) cbData=%d", ApiRef, hKey, lpValueName, dwType, ExplainRegType(dwType), cbData);
		switch(dwType){
			case REG_DWORD: OutTrace("%s Data=%#x\n", sInfo, *(DWORD *)lpData); break;
			case REG_NONE: OutTrace("%s Data=\"%ls\"\n", sInfo, lpData); break;
			case REG_BINARY: OutTrace("%s Data=%s\n", sInfo, hexdump((BYTE *)lpData, cbData)); break;
			case REG_SZ: OutTrace("%s Data=\"%.*ls\"\n", sInfo, cbData, lpData); break;
			default: OutTrace("%s\n", sInfo);
		}
	}
#endif
	if(IsFake(hKey) && (dxw.dwFlags3 & EMULATEREGISTRY)) {
		OutTraceR("%s: SUPPRESS registry key set\n", ApiRef);
		return ERROR_SUCCESS;
	}
	return (*pRegSetValueExW)(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}

LONG WINAPI extRegEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	LONG res;
	ApiName("RegEnumValueW");

	OutTraceR("%s: hKey=%#x(\"%s\") index=%d cchValueName=%d Reserved=%#x lpType=%#x lpData=%#x lpcbData=%#x\n", 
		ApiRef, hKey, hKey2String(hKey), dwIndex, *lpcchValueName, lpReserved, lpType, lpData, lpcbData);
	if (!IsFake(hKey)){
		res=(*pRegEnumValueW)(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
		IfLogKeyValue(ApiRef, TRUE, res, lpType, lpData, lpcbData);
		return res;
	}

	// try emulated registry
	char *lpValueNameA = NULL;
	FILE *regf;
	regf=OpenFakeRegistry();
	if(regf==NULL) return ERROR_FILE_NOT_FOUND;
	res = SeekFakeKey(regf, hKey);
	if(res == ERROR_SUCCESS) {
		lpValueNameA = (char *)malloc(*lpcchValueName + 1);
		res = SeekValueIndex(regf, dwIndex, lpValueNameA, lpcchValueName);
		wcstombs(lpValueNameA, lpValueName, *lpcchValueName);
	}
	if(res == ERROR_SUCCESS) {
		res = GetKeyValue(regf, ApiRef, lpValueNameA, lpType, lpData, lpcbData, TRUE);
	}
	fclose(regf);
	if(lpValueNameA) free((LPVOID)lpValueNameA);
	return res;
}

LONG WINAPI extRegEnumKeyW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cchName)
{
	LONG res;
	ApiName("RegEnumKeyW");

	OutTraceR("%s: hKey=%#x(\"%s\") index=%d cchName=%d\n", 
		ApiRef, hKey, hKey2String(hKey), dwIndex, cchName);
	if (!IsFake(hKey)){
		res=(*pRegEnumKeyW)(hKey, dwIndex, lpName, cchName);
		OutTraceR("%s: res=%#x name=%ls\n",
			ApiRef, res, (res==ERROR_SUCCESS) ? lpName : L"(???)");
		return res;
	}
	
	// try emulated registry
	FILE *regf;
	char sUpKey[MAX_PATH];
	char sSubKey[MAX_PATH];
	UINT len;
	regf=OpenFakeRegistry();
	if(regf==NULL) return ERROR_FILE_NOT_FOUND;
	res = SeekFakeKey(regf, hKey - dwIndex);
	if(res == ERROR_SUCCESS) {
		strcpy(sUpKey, hKey2String(hKey)); 
		strcpy(sSubKey, hKey2String(hKey - dwIndex -1)); 
		len = strlen(sUpKey);
		if(!strncmp(sSubKey, sUpKey, len)){
			mbstowcs(lpName, sSubKey + len + 1, cchName);
			// to do: handling of lpClass and lpcClass when not NULL
			res = ERROR_SUCCESS;
			OutTraceR("%s: enum keynum=%d name=%ls\n", ApiRef, dwIndex, lpName);
		}
		else {
			res = ERROR_NO_MORE_ITEMS;
		}
	}
	fclose(regf);
	OutTraceR("%s: res=%#x\n", ApiRef, res);
	return res;
}

LONG WINAPI extRegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcName, LPDWORD lpReserved, 
							 LPWSTR lpClass, LPDWORD lpcClass, PFILETIME lpftLastWriteTime)
{
	LONG res;
	ApiName("RegEnumKeyExW");

	OutTraceR("%s: hKey=%#x(\"%s\") index=%d cName=%d cClass=%d\n", 
		ApiRef, hKey, hKey2String(hKey), dwIndex, lpcName ? *lpcName : 0, lpcClass ? *lpcClass : 0);
	if (!IsFake(hKey)){
		res=(*pRegEnumKeyExW)(hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
		if(res == ERROR_SUCCESS){
			OutTraceR("%s: res=SUCCESS Name=%s cName=%d Class=%ls cClass=%d\n",
				ApiRef, lpName, lpcName ? *lpcName : 0, lpClass ? lpClass : L"(NULL)", lpcClass ? *lpcClass : 0);
		}
		else {
			OutTraceR("%s: res=%#x\n", ApiRef, res);
		}
		return res;
	}

	// try emulated registry
	FILE *regf;
	char sUpKey[MAX_PATH];
	char sSubKey[MAX_PATH];
	UINT len;
	regf=OpenFakeRegistry();
	if(regf==NULL) return ERROR_FILE_NOT_FOUND;
	res = SeekFakeKey(regf, hKey - dwIndex);
	if(res == ERROR_SUCCESS) {
		strcpy(sUpKey, hKey2String(hKey)); 
		strcpy(sSubKey, hKey2String(hKey - dwIndex -1)); 
		len = strlen(sUpKey);
		if(!strncmp(sSubKey, sUpKey, len)){
			mbstowcs(lpName, sSubKey + len + 1, *lpcName); 
			*lpcName = wcslen(lpName);
			// to do: handling of lpClass and lpcClass when not NULL
			res = ERROR_SUCCESS;
			OutTraceR("%s: enum keynum=%d name=%ls cName=%d\n", ApiRef, dwIndex, lpName, *lpcName);
		}
		else {
			res = ERROR_NO_MORE_ITEMS;
		}
	}
	fclose(regf);
	OutTraceR("%s: res=%#x\n", ApiRef, res);
	return res;
}

LONG WINAPI extRegQueryInfoKeyW(
	HKEY hKey, 
	LPWSTR lpClass, 
	LPDWORD lpcchClass, 
	LPDWORD lpReserved, 
	LPDWORD lpcSubKeys, 
	LPDWORD lpcbMaxSubKeyLen, 
	LPDWORD lpcbMaxClassLen, 
	LPDWORD lpcValues, 
	LPDWORD lpcbMaxValueNameLen, 
	LPDWORD lpcbMaxValueLen, 
	LPDWORD lpcbSecurityDescriptor, 
	PFILETIME lpftLastWriteTime)
{
	// seen in "Fallen Haven"
	LONG res;
	ApiName("RegQueryInfoKeyW");

	OutTraceR("%s: hKey=%#x(\"%s\") cchClass=%d QUERY: Class=%s SubKeys=%s MaxSubKeyLen=%s MaxClassLen=%s "
		"Values=%s ValueNameLen=%s MaxValueLen=%s SecurityDescr=%s LastWriteTime=%s\n", 
		ApiRef,
		hKey, hKey2String(hKey), 
		lpcchClass ? *lpcchClass : 0,
		IsQueried(lpClass),
		IsQueried(lpcSubKeys),
		IsQueried(lpcbMaxSubKeyLen),
		IsQueried(lpcbMaxClassLen),
		IsQueried(lpcValues),
		IsQueried(lpcbMaxValueNameLen),
		IsQueried(lpcbMaxValueLen),
		IsQueried(lpcbSecurityDescriptor),
		IsQueried(lpftLastWriteTime)
		);

	if (!IsFake(hKey)){
		res=(*pRegQueryInfoKeyW)(
			hKey, 
			lpClass, 
			lpcchClass, 
			lpReserved, 
			lpcSubKeys, 
			lpcbMaxSubKeyLen, 
			lpcbMaxClassLen,
			lpcValues, 
			lpcbMaxValueNameLen, 
			lpcbMaxValueLen, 
			lpcbSecurityDescriptor, 
			lpftLastWriteTime);
	}
	else {
		FILE *regf;
		regf=OpenFakeRegistry();
		if(regf==NULL) {
			OutTraceR("%s: error in OpenFakeRegistry err=%s\n", ApiRef, GetLastError());	
			return ERROR_FILE_NOT_FOUND;
		}

		res = ERROR_FILE_NOT_FOUND;
		char RegBuf[MAX_PATH+1];
		HKEY hCurKey=HKEY_FAKE+1;
		fgets(RegBuf, 256, regf);
		while (!feof(regf)){
			if(RegBuf[0]=='['){
				hCurKey--;
			}
			if(hCurKey==hKey) {
				//OutTraceREGDebug("DEBUG: SeekFakeKey fount key at line=%s\n", RegBuf);
				res = ERROR_SUCCESS;
				if(lpcchClass){
					// No support for custom keys at the moment ...
					//if(lpClass && (*lpcchClass > strlen(RegBuf))) strncpy(lpClass, RegBuf, *lpcchClass);			
					//*lpcchClass = strlen(RegBuf);
					if(lpClass) *lpClass = 0;
					if(lpcchClass) *lpcchClass = 0;
				}
				break;
			}
			fgets(RegBuf, 256, regf);
		}

		if(res == ERROR_SUCCESS) {
			int iValueNameLen, iMaxValueNameLen, iValues;
			if (lpcSubKeys) *lpcSubKeys = 0;
			if (lpcbMaxSubKeyLen) *lpcbMaxSubKeyLen = 0; // ???
			if (lpcbMaxClassLen) *lpcbMaxClassLen = 0; // ???
			if (lpcSubKeys) *lpcSubKeys = 0; // ???

			iValues = 0; 
			iMaxValueNameLen = 0;

			fgets(RegBuf, 256, regf);
			while (!feof(regf)){
				if (RegBuf[0] == '[') break;
				if(RegBuf[0]=='"') {
					iValues++;
					if (lpcbMaxValueNameLen){
						int i;
						for(i=1; RegBuf[i]; i++) if(RegBuf[i]=='"') break;
						iValueNameLen = i-1;
						if(iValueNameLen > iMaxValueNameLen) iMaxValueNameLen = iValueNameLen; // save maximum value
					}
				}
				fgets(RegBuf, 256, regf);
			}
			OutDebugR("> Values=%d\n", iValues);	
			OutDebugR("> MaxValueNameLen=%d\n", iMaxValueNameLen);	
			if (lpcValues) *lpcValues = iValues;
			if (lpcbMaxValueNameLen) *lpcbMaxValueNameLen = iMaxValueNameLen;
		}
	}

	if(res == ERROR_SUCCESS){
		char sTime[81];
		SYSTEMTIME st;
		strcpy(sTime, "");
		if (lpftLastWriteTime) {
			if(FileTimeToSystemTime(lpftLastWriteTime, &st)){
				sprintf(sTime, "%02.2d/%02.2d/%04.4d %02.2d:%02.2d:%02.2d",
					st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			}
		}
		OutTraceR("%s: res=SUCCESS hKey=%#x(\"%s\") "
			"Class=%ls cchClass=%d SubKeys=%d MaxSubKeyLen=%d "
			"MaxClassLen=%d Values=%d MaxValueNameLen=%d SecDescr=%s LastWriteTime=%s\n", 
			ApiRef,
			hKey, hKey2String(hKey), 
			lpClass ? lpClass : L"(NULL)",
			lpcchClass ? *lpcchClass : 0,
			lpcSubKeys ? *lpcSubKeys : 0,
			lpcbMaxSubKeyLen ? *lpcbMaxSubKeyLen : 0,
			lpcbMaxClassLen ? *lpcbMaxClassLen : 0,
			lpcValues ? *lpcValues : 0,
			lpcbMaxValueNameLen ? *lpcbMaxValueNameLen : 0,
			lpcbSecurityDescriptor ? "(NULL)" : "(tbd)",
			lpftLastWriteTime ? sTime : "(NULL)"
			);
	}
	else {
		OutTraceR("%s: res=%#x err=%d\n", ApiRef, res, GetLastError());
	}	
	return res;
}

BOOL WINAPI extGetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer)
{
	BOOL res;
	ApiName("GetUserNameW");

	OutTraceR("%s: maxlen=%d\n", ApiRef, *pcbBuffer);
	res = (*pGetUserNameW)(lpBuffer, pcbBuffer);
	if(res){
		OutTraceR("%s: user=\"%ls\" len=%d\n", ApiRef, lpBuffer, *pcbBuffer);
	}
	else {
		OutErrorR("%s: err=%d\n", ApiRef, GetLastError());
	}

	return res;
}

  //[in]                HKEY    hkey,
  //[in, optional]      LPCSTR  lpSubKey,
  //[in, optional]      LPCSTR  lpValue,
  //[in, optional]      DWORD   dwFlags,
  //[out, optional]     LPDWORD pdwType,
  //[out, optional]     PVOID   pvData,
  //[in, out, optional] LPDWORD pcbData

LONG WINAPI extRegGetValueA(HKEY hKey, LPCSTR lpSubKey, LPCSTR lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
{
	LONG res;
	ApiName("RegGetValueA");

	OutTraceR("%s: hKey=%#x(\"%s\") SubKeys=%s Value=%s flags=%#x cbData=%d\n", 
		ApiRef,
		hKey, hKey2String(hKey), 
		lpSubKey,
		lpValue,
		dwFlags, 
		*pcbData);

	res = (*pRegGetValueA)(hKey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
	if(res == ERROR_SUCCESS){
		OutTraceR("%s: Value=\"%s\" len=%d type=%#x(%s)\n", ApiRef, pvData, *pcbData, *pdwType, sType(*pdwType));
	}
	else {
		OutErrorR("%s: err=%d\n", ApiRef, GetLastError());
	}

	return res;
}

LONG WINAPI extRegGetValueW(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
{
	LONG res;
	ApiName("RegGetValueW");

	OutTraceR("%s: hKey=%#x(\"%s\") SubKeys=%ls Value=%ls flags=%#x cbData=%d\n", 
		ApiRef,
		hKey, hKey2String(hKey), 
		lpSubKey,
		lpValue,
		dwFlags, 
		*pcbData);

	res = (*pRegGetValueW)(hKey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
	if(res == ERROR_SUCCESS){
		OutTraceR("%s: Value=\"%ls\" len=%d type=%#x(%s)\n", ApiRef, pvData, *pcbData, *pdwType, sType(*pdwType));
	}
	else {
		OutErrorR("%s: err=%d\n", ApiRef, GetLastError());
	}

	return res;
}

LONG WINAPI extRegOpenCurrentUser(REGSAM samDesired, PHKEY phkResult)
{
	LONG res;
	ApiName("RegOpenCurrentUser");

	OutTraceR("%s: samDesired=%#x\n", ApiRef, samDesired);

	res = (*pRegOpenCurrentUser)(samDesired, phkResult);
	if(res == ERROR_SUCCESS){
		OutTraceR("%s: result(hKey)=%#x\n", ApiRef, *phkResult);
	}
	else {
		OutErrorR("%s: err=%d\n", ApiRef, GetLastError());
	}

	return res;
}

LONG WINAPI extRegDeleteValueA(HKEY hKey, LPCSTR lpValueName)
{
	LONG res;
	ApiName("RegDeleteValueA");

	OutTraceR("%s: hKey=%#x name=%s\n", ApiRef, hKey, lpValueName);

	if (IsFake(hKey)){
		OutTraceR("%s: pretending SUCCESS\n", ApiRef);
		return ERROR_SUCCESS;
	}

	res = (*pRegDeleteValueA)(hKey, lpValueName);
	if(res != ERROR_SUCCESS){
		OutErrorR("%s: err=%d\n", ApiRef, GetLastError());
	}

	return res;
}
