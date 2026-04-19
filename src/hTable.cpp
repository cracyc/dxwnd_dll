#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
//#include "hTable.hpp"

//#define LOGGING
#ifdef LOGGING
#define LOG(f, ...) OutTrace(f, __VA_ARGS__)
#else
#define LOG(f, ...)
#endif

hTable::hTable()
{
	InitializeCriticalSection(&hTableLock);
	hTableBuffer = (HMODULE *)malloc(HTABLEDELTA * sizeof(HMODULE));
	memset(hTableBuffer, 0, HTABLEDELTA * sizeof(HMODULE));
	hTableSize = HTABLEDELTA;
	hTableMax = 0;
	LOG("hTable initialized\n");
}

BOOL hTable::hGet(HMODULE h)
{
	HMODULE *hp;
	int i = 0;
	EnterCriticalSection(&hTableLock);
	if(hTableMax == hTableSize){
		HMODULE *hTableNew;
		hTableSize += HTABLEDELTA;
		hTableNew = (HMODULE *)realloc((LPVOID)hTableBuffer, hTableSize*sizeof(HMODULE));
		memset(&hTableNew[hTableSize - HTABLEDELTA], 0, HTABLEDELTA * sizeof(HMODULE));
		hTableBuffer = hTableNew;
		LOG("hGet EXPAND size=%d\n", hTableSize);
	}
	for (i=0; i<hTableMax; i++){
		hp = &hTableBuffer[i];
		if(h == *hp) {
			LOG("hGet MATCH i=%d hdl=%#x\n", i, *hp);
			LeaveCriticalSection(&hTableLock);
			return TRUE;
		}
	}
	hp = &hTableBuffer[i];
	hTableMax++;
	*hp = h;
	LOG("hGet FILL i=%d hdl=%#x max=%d\n", i, *hp, hTableMax);
	LeaveCriticalSection(&hTableLock);
	return FALSE;
}

typedef DWORD (WINAPI *GetModuleFileNameA_Type)(HMODULE, LPSTR, DWORD);
void hTable::hClean(void)
{
	LOG("hClean\n");
	EnterCriticalSection(&hTableLock);
	extern GetModuleFileNameA_Type pGetModuleFileNameA;
	char sName[MAX_PATH];
	HMODULE *hp;
	for (int i=0; i<hTableMax; i++){
		hp = &hTableBuffer[i];
		LOG("hClean i=%d h=%#x\n", i, *hp);
		if(*hp != NULL) {
			if ((*pGetModuleFileNameA)(*hp, sName, MAX_PATH) == 0){ 
				LOG("hClean FREE h=%#x\n", *hp);
				*hp = NULL;
			}
			else {
				LOG("hClean VALID h=%#x path=%s\n", *hp, sName);
			}
		}
	}
	LeaveCriticalSection(&hTableLock);
}
