#define  _CRT_SECURE_NO_WARNINGS

#define _MODULE "kernel32"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "stdio.h"

#undef DXWEXTERN
#define DXWEXTERN
#include "heap.h"

//#define DXWFORCETRACE
#ifdef DXWFORCETRACE
#undef OutTraceDW
#undef OutTraceSYS
#undef OutDebugSys
#undef IsDebugSYS
#define OutTraceDW OutTrace
#define OutTraceSYS OutTrace
#define OutDebugSys OutTrace
#define IsDebugSYS TRUE
#endif // DXWFORCETRACE

// ---------------------------------------------------------------------
// Heap wrappers
// ---------------------------------------------------------------------

// FORCEVIRTUALHEAP TRUE will force going to malloc also for default heap, but then HeapFree, HeapDestroy 
// calls are at risk! 
// A proper usage of the flag is in combination with early-hook methods.
#define FORCEVIRTUALHEAP FALSE
#define HEAPSERIALIZE TRUE
#define HEAPPADSIZE 1000
//#define ASSERTDIALOGS

#ifdef ASSERTDIALOGS
BOOL newHeap(HANDLE hHeap)
{
	for(int i=0; i<dxw.nHeaps; i++) if(hHeap == dxw.pHeaps[i]) return TRUE;
	return FALSE;
}

void popHeap(HANDLE hHeap)
{
	for(int i=0; i<dxw.nHeaps; i++) if(hHeap == dxw.pHeaps[i]) dxw.pHeaps[i] = NULL;
}
#endif // ASSERTDIALOGS

static LPVOID VHeapMin = (LPVOID)0xFFFFFFFF;
static LPVOID VHeapMax = (LPVOID)0x00000000;
static LPVOID lpLast   = (LPVOID)0x00000000;
static DWORD iProg = 1;
static BOOL bRecursed = FALSE;

#define isVirtualHeap(hHeap) (dxw.dwFlags8 & VIRTUALHEAP) && ((DWORD)hHeap >= 0xDEADBEEF) && ((DWORD)hHeap <= 0xDEADBEEF + iProg)
#define isVirtualAddr(lpMem) (dxw.dwFlags8 & VIRTUALHEAP) && (lpMem >= VHeapMin) && (lpMem <= VHeapMax)

HANDLE WINAPI extGetProcessHeap(void)
{
	HANDLE ret;
	ApiName("GetProcessHeap");

	OutDebugSYS("%s\n", ApiRef);

	if(dxw.dwFlags11 & VIRTUALPROCHEAP) {
		ret = (HANDLE)0xDEADBEEF;
		OutDebugSYS("%s: ret=%#x(virtual)\n", ApiRef, ret);
	}
	else {
		ret = (*pGetProcessHeap)();
		OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	return ret;
}

DWORD WINAPI extGetProcessHeaps(DWORD NumberOfHeaps, PHANDLE ProcessHeaps)
{
	DWORD ret;
	ApiName("GetProcessHeaps");

	OutTraceSYS("%s: nheaps=%d ph+%#x\n", ApiRef, NumberOfHeaps, ProcessHeaps);
		
	ret = (*pGetProcessHeaps)(NumberOfHeaps, ProcessHeaps);
	if(ret == 0){
		OutErrorSYS("%s: ret=0 err=%d\n", ApiRef, GetLastError());
	}
	else {
		OutTraceSYS("%s: nheaps=%d\n", ApiRef, ret);
		int n = (NumberOfHeaps > ret) ? ret : NumberOfHeaps;
		for(int i=0; i<n; i++){
			OutDebugSYS("%s: heap[%d]=%#x\n", ApiRef, i, ProcessHeaps[i]);
		}
	}

	if((dxw.dwFlags11 & VIRTUALPROCHEAP) && ret && (NumberOfHeaps > 0)){
		OutTraceDW("%s: replace process heap %#x->%%x\n", 
			ApiRef, ProcessHeaps[0], 0xDEADBEEF);
		ProcessHeaps[0] = (HANDLE)0xDEADBEEF;
	}

	return ret;
}

LPVOID WINAPI extHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
	LPVOID ret;
	ApiName("HeapAlloc");

	// v2.05.76: recursion fix.
	//static BOOL bRecursed = FALSE;
	if(bRecursed) return (*pHeapAlloc)(hHeap, dwFlags, dwBytes);
	bRecursed = TRUE;

	OutDebugSYS("%s: heap=%#x flags=%#x bytes=%d\n", ApiRef, hHeap, dwFlags, dwBytes);

	// v2.05.71: added emulation of MS HeapPadAllocation shim
	if(dxw.dwFlags14 & HEAPPADALLOCATION) dwBytes += HEAPPADSIZE;

	if(HEAPSERIALIZE) dwFlags &= ~HEAP_NO_SERIALIZE;
	if(dxw.dwFlags19 & HEAPZEROMEMORY) dwFlags |= HEAP_ZERO_MEMORY;

	if(dxw.dwFlags14 & SAFEHEAP){
		ret = (*pHeapAlloc)(hHeap, dwFlags, dwBytes+12);
		if(ret){
			memcpy(ret, "AA", 2);
			memcpy((LPBYTE)ret+2, &dwBytes, sizeof(size_t));
			memcpy((LPBYTE)ret+6, &hHeap, sizeof(HANDLE));
			memcpy((LPBYTE)ret+dwBytes+10, "ZZ", 2);
			ret = (LPBYTE)ret + 10;
			OutDebugSYS("%s: (safe) ret=%#x\n", ApiRef, ret);
		}
		bRecursed = FALSE;
		return ret;
	}

	//if((dxw.dwFlags8 & VIRTUALHEAP) && ((DWORD)hHeap >= (DWORD)0xDEADBEEF) && ((DWORD)hHeap <= (DWORD)0xDEADBEEF + iProg)){
	if(isVirtualHeap(hHeap)){
		// v2.05.76: recursion fix. The alloc() call can call a HeapAlloc
		ret = malloc(dwBytes);
		lpLast = 0;
		if(ret){
			if(ret > VHeapMax) VHeapMax = ret;
			if(ret < VHeapMin) VHeapMin = ret;
		}
		if(ret) {
			OutDebugSYS("%s: (virtual) ret=%#x\n", ApiRef, ret);
		} 
		else {
			OutErrorSYS("%s: (virtual) ret=0 err=%d\n", ApiRef, GetLastError());
		}
	}
	else {
		ret = (*pHeapAlloc)(hHeap, dwFlags, dwBytes);
		OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	if(ret && (dxw.dwFlags9 & NOBAADFOOD)) memset(ret, 0, dwBytes);
	bRecursed = FALSE;
	return ret;
}

LPVOID WINAPI extHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
{
	LPVOID ret;
	ApiName("HeapReAlloc");

	// v2.05.76: recursion fix.
	//static BOOL bRecursed = FALSE;
	if(bRecursed) return (*pHeapReAlloc)(hHeap, dwFlags, lpMem, dwBytes);
	bRecursed = TRUE;

	OutDebugSYS("%s: heap=%#x flags=%#x mem=%#x bytes=%d\n", ApiRef, hHeap, dwFlags, lpMem, dwBytes);

	// v2.05.71: added emulation of MS HeapPadAllocation shim
	if(dxw.dwFlags14 & HEAPPADALLOCATION) dwBytes += HEAPPADSIZE;

	if(HEAPSERIALIZE) dwFlags &= ~HEAP_NO_SERIALIZE;

	if(dxw.dwFlags14 & SAFEHEAP){
		BOOL bCorrupted = FALSE;
		size_t origLen = (size_t)*(size_t *)((LPBYTE)lpMem-8);
		HANDLE origHdl = (HANDLE)*(HANDLE *)((LPBYTE)lpMem-4);

		if((origLen < 0) || (origLen > 10000000)) {
			OutErrorSYS("%s: SAFEHEAP suspicious len=%d - skip\n", ApiRef, origLen);
			bCorrupted = TRUE;
		}

		if(origHdl != hHeap){
			if(dxw.dwFlags18 & REPAIRHEAP){
				OutErrorSYS("%s: SAFEHEAP bad hHeap origHdl=%#x - REPAIR\n", ApiRef, origHdl);
				hHeap = origHdl;
			}
			else {
				OutErrorSYS("%s: SAFEHEAP bad hHeap origHdl=%#x - skip\n", ApiRef, origHdl);
				bCorrupted = TRUE;
			}
		}

		if(!bCorrupted && memcmp((LPBYTE)lpMem-10, "AA", 2)) {
			OutErrorSYS("%s: SAFEHEAP corrupted heap HEAD @%#x - skip\n", ApiRef, lpMem);
			HexTrace((LPBYTE)lpMem-10, 10);
			bCorrupted = TRUE;
		}

		if(!bCorrupted && memcmp((LPBYTE)lpMem+origLen, "ZZ", 2)) {
			OutErrorSYS("%s: SAFEHEAP corrupted heap TAIL @%#x - skip\n", ApiRef, lpMem);
			HexTrace((LPBYTE)lpMem+origLen, 2);
			bCorrupted = TRUE;
		}
#ifdef ASSERTDIALOGS
		if(bCorrupted && newHeap(hHeap)){
			MessageBox(NULL, "Warning: HeapRealloc on corrupted buffer", "DxWnd", 0);
		}
#endif // ASSERTDIALOGS
		if(bCorrupted){
			ret = (*pHeapAlloc)(hHeap, dwFlags, dwBytes+12); 
			if(ret){
				memcpy(ret, "AA", 2);
				memcpy((LPBYTE)ret+2, &dwBytes, sizeof(size_t));
				memcpy((LPBYTE)ret+6, &hHeap, sizeof(HANDLE));
				memcpy((LPBYTE)ret+dwBytes+10, "ZZ", 2);
				// try to move old data al long as possible safely
				UINT i;
				__try{
					for(i=0; i<dwBytes; i++) ((LPBYTE)ret)[10+i] = ((LPBYTE)lpMem)[i];
				}
				__except(EXCEPTION_EXECUTE_HANDLER) { i = dwBytes; }
				bRecursed = FALSE;
				ret = (LPBYTE)ret+10;
			}
		}
		else {
			ret = (*pHeapReAlloc)(hHeap, dwFlags, (LPBYTE)lpMem-10, dwBytes+12); 
			if(ret){
				memcpy(ret, "AA", 2);
				memcpy((LPBYTE)ret+2, &dwBytes, sizeof(size_t));
				memcpy((LPBYTE)ret+6, &hHeap, sizeof(HANDLE));
				memcpy((LPBYTE)ret+dwBytes+10, "ZZ", 2);
				bRecursed = FALSE;
				ret = (LPBYTE)ret+10;
			}
		}
		OutDebugSYS("%s: (safe) ret=%#x\n", ApiRef, ret);
		return ret;
	}

	//if((dxw.dwFlags8 & VIRTUALHEAP) && (lpMem >= VHeapMin) && (lpMem <= VHeapMax)){
	if(isVirtualAddr(lpMem)){
		// v2.05.76: recursion fix. The realloc() call can call a HeapReAlloc
		ret = realloc(lpMem, dwBytes);
		lpLast = 0;
		if(ret){
			if(ret > VHeapMax) VHeapMax = ret;
			if(ret < VHeapMin) VHeapMin = ret;
		}
		OutDebugSYS("%s: (virtual) ret=%#x\n", ApiRef, ret);
	}
	else {
		ret = (*pHeapReAlloc)(hHeap, dwFlags, lpMem, dwBytes);
		OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	bRecursed = FALSE;
	return ret;
}

BOOL WINAPI extHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
	BOOL ret;
	ApiName("HeapFree");

	// v2.05.76: recursion fix.
	if(bRecursed) return (*pHeapFree)(hHeap, dwFlags, lpMem);
	bRecursed = TRUE;

	OutDebugSYS("%s: heap=%#x flags=%#x mem=%#x\n", ApiRef, hHeap, dwFlags, lpMem);

	if(HEAPSERIALIZE) dwFlags &= ~HEAP_NO_SERIALIZE;

	if(dxw.dwFlags14 & HEAPLEAK) {
		//if(!HeapValidate(hHeap, 0, lpMem)){		
			OutTraceDW("%s: HEAPLEAK skipping HeapFree(%#x, %#x)\n", ApiRef, hHeap, lpMem);
			bRecursed = FALSE;
			return TRUE;
		//}
	}

	if(dxw.dwFlags14 & SAFEHEAP){
		BOOL bCorrupted = FALSE;
		if(lpMem == 0){
			// what does HeapFree(hHeap,dwFlags,0)? Maybe it frees the whole heap?
			ret = (*pHeapFree)(hHeap, dwFlags, 0);
			OutDebugSYS("%s: (safe) ret=%#x\n", ApiRef, ret);
			bRecursed = FALSE;
			return ret;
		}
		size_t origLen = (size_t)*(size_t *)((LPBYTE)lpMem-8);
		HANDLE origHdl = (HANDLE)*(HANDLE *)((LPBYTE)lpMem-4);
		OutDebugSYS("%s: origlen=%d orighdl=%#x\n", ApiRef, origLen, origHdl);

		if((origLen < 0) || (origLen > 10000000)) {
			OutErrorSYS("%s: SAFEHEAP suspicious len=%d - skip\n", ApiRef, origLen);
			bCorrupted = TRUE;
		} 
		if(origHdl != hHeap){
			if(dxw.dwFlags18 & REPAIRHEAP){
				OutErrorSYS("%s: SAFEHEAP bad hHeap origHdl=%#x - REPAIR\n", ApiRef, origHdl);
				hHeap = origHdl;
			}
			else {
				OutErrorSYS("%s: SAFEHEAP bad hHeap origHdl=%#x - skip\n", ApiRef, origHdl);
				bCorrupted = TRUE;
			}
		}
		if(!bCorrupted && memcmp((LPBYTE)lpMem-10, "AA", 2)) {
			if(!memcmp((LPBYTE)lpMem-10, "FF", 2)) {
				OutErrorSYS("%s: SAFEHEAP duplicate free @%#x - skip\n", ApiRef, lpMem);
//#ifdef ASSERTDIALOGS
//			if(newHeap(hHeap)){
//				MessageBox(NULL, "Warning: double freed buffer", "DxWnd", 0);
//			}
//#endif // ASSERTDIALOGS
			}
			else {
				OutErrorSYS("%s: SAFEHEAP corrupted heap HEAD @%#x - skip\n", ApiRef, lpMem);
				HexTrace((LPBYTE)lpMem-10, 10);
#ifdef ASSERTDIALOGS
				if(newHeap(hHeap)){
					MessageBox(NULL, "Warning: free on corrupted heap HEAD", "DxWnd", 0);
				}
#endif // ASSERTDIALOGS
			}
			bCorrupted = TRUE;
		}

		if(!bCorrupted && memcmp((LPBYTE)lpMem+origLen, "ZZ", 2)) {
			OutErrorSYS("%s: SAFEHEAP corrupted buffer TAIL @%#x - skip\n", ApiRef, lpMem);
			HexTrace((LPBYTE)lpMem+origLen, 2);
#ifdef ASSERTDIALOGS
			if(newHeap(hHeap)){
				MessageBox(NULL, "Warning: free on corrupted heap TAIL", "DxWnd", 0);
			}
#endif // ASSERTDIALOGS
			bCorrupted = TRUE;
		}

		ret = TRUE;
		if(!bCorrupted){
			memcpy((LPBYTE)lpMem-10, "FF", 2);
			__try {
				ret = (*pHeapFree)(hHeap, dwFlags, (LPBYTE)lpMem-10);
			}
			__except(EXCEPTION_EXECUTE_HANDLER){
				ret = TRUE;
				OutTrace("%s: (exception - skip) ret=%#x\n", ApiRef, ret);
				return ret;
			}
		}
		OutDebugSYS("%s: (safe) ret=%#x\n", ApiRef, ret);
		bRecursed = FALSE;
		return ret;
	}	

	if(lpLast == lpMem) {
		OutTraceDW("%s: (virtual) skip duplicate free\n", ApiRef);
		bRecursed = FALSE;
		return TRUE;
	} 

	char *sVirtual = "";
	__try { // v2.05.06: quick & dirty fix for HeapFree exceptions
		if(isVirtualAddr(lpMem)){
			sVirtual = "(virtual) ";
			// v2.05.76: recursion fix. The free() call can hide a HeapFree
			//bRecursed = TRUE;
			free(lpMem);
			//bRecursed = FALSE;
			lpLast = lpMem;
			ret = TRUE;
		}
		else {
			ret = (*pHeapFree)(hHeap, dwFlags, lpMem);
		}
	} __except(EXCEPTION_EXECUTE_HANDLER){
		OutErrorSYS("%s: %sexception\n", ApiRef, sVirtual);
		ret = TRUE;
	}

	OutDebugSYS("%s: %sret=%#x\n", ApiRef, sVirtual, ret);
	bRecursed = FALSE;
	return ret;
}

BOOL WINAPI extHeapValidate(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
	BOOL ret;
	ApiName("HeapValidate");

	// v2.05.76: recursion fix.
	if(bRecursed) return (*pHeapValidate)(hHeap, dwFlags, lpMem);
	bRecursed = TRUE;

	OutDebugSYS("%s: heap=%#x flags=%#x mem=%#x\n", ApiRef, hHeap, dwFlags, lpMem);

	if(HEAPSERIALIZE) dwFlags &= ~HEAP_NO_SERIALIZE;

	if(dxw.dwFlags14 & SAFEHEAP){
		BOOL corrupted = FALSE;
		if(lpMem == NULL){
			// validate the whole heap block
			ret = (*pHeapValidate)(hHeap, dwFlags, lpMem);
			bRecursed = FALSE;
			return ret; 
		}

		size_t origLen = (size_t)*(size_t *)((LPBYTE)lpMem-8);
		HANDLE origHdl = (HANDLE)*(HANDLE *)((LPBYTE)lpMem-4);
		if((origLen < 0) || (origLen > 10000000)) {
			corrupted = TRUE;
			OutErrorSYS("%s: SAFEHEAP suspicious len=%d\n", ApiRef, origLen);
#ifdef ASSERTDIALOGS
			if(newHeap(hHeap)){
				MessageBox(NULL, "Warning: suspicious len in HeapValidate", "DxWnd", 0);
			}
#endif // ASSERTDIALOGS
			bRecursed = FALSE;
			return TRUE; // pretend it was ok
		}
		if(origHdl != hHeap){
			OutErrorSYS("%s: SAFEHEAP bad hHeap origHdl=%#x - skip\n", ApiRef, origHdl);
			corrupted = TRUE;
			//OutErrorSYS("%s: SAFEHEAP bad hHeap origHdl=%#x - fix\n", ApiRef, origHdl);
			//hHeap = origHdl;
			return TRUE;
		}

		OutDebugSYS("%s: origlen=%d\n", ApiRef, origLen);
		if(memcmp((LPBYTE)lpMem-10, "AA", 2)) {
			corrupted = TRUE;
			if(!memcmp((LPBYTE)lpMem-10, "FF", 2)) {
				OutErrorSYS("%s: SAFEHEAP free block @%#x\n", ApiRef, lpMem);
//#ifdef ASSERTDIALOGS
//				if(newHeap(hHeap)){
//					MessageBox(NULL, "Warning: double freed buffer", "DxWnd", 0);
//				}
//#endif // ASSERTDIALOGS
			}
			else {
				OutErrorSYS("%s: SAFEHEAP corrupted heap HEAD @%#x\n", ApiRef, lpMem);
				HexTrace((LPBYTE)lpMem-10, 10);
#ifdef ASSERTDIALOGS
				if(newHeap(hHeap)){
					MessageBox(NULL, "Warning: free on corrupted heap HEAD", "DxWnd", 0);
				}
#endif // ASSERTDIALOGS
			}
		}

		if(memcmp((LPBYTE)lpMem+origLen, "ZZ", 2)) {
			corrupted = TRUE;
			OutErrorSYS("%s: SAFEHEAP corrupted buffer TAIL @%#x\n", ApiRef, lpMem);
			HexTrace((LPBYTE)lpMem+origLen, 2);
#ifdef ASSERTDIALOGS
			if(newHeap(hHeap)){
				MessageBox(NULL, "Warning: free on corrupted heap TAIL", "DxWnd", 0);
			}
#endif // ASSERTDIALOGS
		}

		OutDebugSYS("%s: (safe) ret=TRUE\n", ApiRef);
		bRecursed = FALSE;
		return TRUE;
	}	

	if(isVirtualAddr(lpMem)){
		ret = TRUE;
		OutDebugSYS("%s: (virtual) ret=%#x\n", ApiRef, ret);
	}
	else {
		ret = (*pHeapValidate)(hHeap, dwFlags, lpMem);
		OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	}
	return ret;
}

SIZE_T WINAPI extHeapCompact(HANDLE hHeap, DWORD dwFlags)
{
	SIZE_T ret;
	ApiName("HeapCompact");

	OutDebugSYS("%s: heap=%#x flags=%#x\n", ApiRef, hHeap, dwFlags);

	if(HEAPSERIALIZE) dwFlags &= ~HEAP_NO_SERIALIZE;

	char *label = "";
	if(isVirtualHeap(hHeap)){
		ret = 100000; // just a number .... to be fixed !!!
		label="(virtual) ";
	}
	else {
		ret = (*pHeapCompact)(hHeap, dwFlags);

		// decrement space for safe data
		if(dxw.dwFlags14 & SAFEHEAP) {
			label="(safe) ";
			ret = (ret > 8) ? ret - 8 : 0;
		}
		// decrement space for additional space
		if(dxw.dwFlags14 & HEAPPADALLOCATION) {
			ret = (ret > HEAPPADSIZE) ? ret - HEAPPADSIZE : 0;
		}

		OutDebugSYS("%s: %sret=%d\n", ApiRef, label, ret);
	}
	return ret;
}

HANDLE WINAPI extHeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize)
{
	HANDLE ret;
	ApiName("HeapCreate");

	OutDebugSYS("%s: flags=%#x size(init-max)=(%d-%d)\n", ApiRef, flOptions, dwInitialSize, dwMaximumSize);

	if(dxw.dwFlags8 & VIRTUALHEAP){
		ret = (HANDLE)(0xDEADBEEF + iProg++);
		OutDebugSYS("%s: (virtual) ret=%X\n", ApiRef, ret);
	}
	else {
		ret = (*pHeapCreate)(flOptions, dwInitialSize, dwMaximumSize);
		OutDebugSYS("%s: ret=%X\n", ApiRef, ret);
	}
	return ret;
}

BOOL WINAPI extHeapDestroy(HANDLE hHeap)
{
	BOOL ret;
	ApiName("HeapDestroy");

	OutDebugSYS("%s: heap=%#x\n", ApiRef, hHeap);

	if(dxw.dwFlags14 & SAFEHEAP){
		// validate the whole heap block
		bRecursed = TRUE;
		ret = (*pHeapValidate)(hHeap, 0, 0);
		bRecursed = FALSE;
		if(!ret){
			OutDebugSYS("%s: (safe) ret=TRUE\n", ApiRef);
			return TRUE;
		}
	}

	if(isVirtualHeap(hHeap))
		ret = TRUE;
	else {
		ret = (*pHeapDestroy)(hHeap);
#ifdef ASSERTDIALOGS
		popHeap(hHeap);
#endif // ASSERTDIALOGS
	}
	OutDebugSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

SIZE_T WINAPI extHeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem)
{
	SIZE_T ret;
	ApiName("HeapSize");

	OutDebugSYS("%s: heap=%#x flags=%#x mem=%#x\n", ApiRef, hHeap, dwFlags, lpMem);

	if(HEAPSERIALIZE) dwFlags &= ~HEAP_NO_SERIALIZE;

	if(dxw.dwFlags14 & SAFEHEAP){
		ret = (*pHeapSize)(hHeap, dwFlags, (LPBYTE)lpMem-6);
		if(ret == -1) {
			OutDebugSYS("%s: (safe) ret(size)=%d\n", ApiRef, ret);
			return ret;
		}
		if(ret < 8) {
			OutDebugSYS("%s: (safe) suspicious ret(size)=%d\n", ApiRef, ret);
		}
		ret = (ret >= 8) ? ret - 8 : 0;

		// decrement space for additional space
		if(dxw.dwFlags14 & HEAPPADALLOCATION) {
			ret = (ret > HEAPPADSIZE) ? ret - HEAPPADSIZE : 0;
		}
		OutDebugSYS("%s: (safe) ret(size)=%d\n", ApiRef, ret);
		return ret;
	}

	if(isVirtualHeap(hHeap)){
		// malloc-ed areas can't have an easily measured size, try returning
		// the size of default process heap.
		// no good for "Crime Cities" - maybe it expects a growing size?
		//ret = (*pHeapSize)((*pGetProcessHeap)(), dwFlags, lpMem);
		OutErrorSYS("%s: UNIMPLEMENTED\n", ApiRef);
		ret = 400000;
	}
	else {
		ret = (*pHeapSize)(hHeap, dwFlags, lpMem);
	}
	OutDebugSYS("%s: ret(size)=%d\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extHeapLock(HANDLE hHeap)
{
	BOOL ret;
	ApiName("HeapLock");

	OutTraceSYS("%s: heap=%#x\n", ApiRef, hHeap);
	if(isVirtualHeap(hHeap)){
		OutErrorSYS("%s: UNIMPLEMENTED\n", ApiRef);
		ret = TRUE;
	}
	else {
		ret = (*pHeapLock)(hHeap);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extHeapUnlock(HANDLE hHeap)
{
	BOOL ret;
	ApiName("HeapUnlock");

	OutTraceSYS("%s: heap=%#x\n", ApiRef, hHeap);
	if(isVirtualHeap(hHeap)){
		OutErrorSYS("%s: UNIMPLEMENTED\n", ApiRef);
		ret = TRUE;
	}
	else {
		ret = (*pHeapUnlock)(hHeap);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extHeapQueryInformation(HANDLE hHeap, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength, PSIZE_T ReturnLength)
{
	BOOL ret;
	ApiName("HeapQueryInformation");

	OutTraceSYS("%s: heap=%#x\n", ApiRef, hHeap);
	if(isVirtualHeap(hHeap)){
		OutErrorSYS("%s: UNIMPLEMENTED\n", ApiRef);
		ret = TRUE;
	}
	else {
		ret = (*pHeapQueryInformation)(hHeap, HeapInformationClass, HeapInformation, HeapInformationLength, ReturnLength);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extHeapSetInformation(HANDLE hHeap, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength)
{
	BOOL ret;
	ApiName("HeapSetInformation");

	OutTraceSYS("%s: heap=%#x\n", ApiRef, hHeap);
	if(isVirtualHeap(hHeap)){
		OutErrorSYS("%s: UNIMPLEMENTED\n", ApiRef);
		ret = TRUE;
	}
	else {
		ret = (*pHeapSetInformation)(hHeap, HeapInformationClass, HeapInformation, HeapInformationLength);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

BOOL WINAPI extHeapWalk(HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry)
{
	BOOL ret;
	ApiName("HeapWalk");

#ifndef DXW_NOTRACES
	OutTraceSYS("%s: heap=%#x\n", ApiRef, hHeap);
	if(IsDebugSYS){
		OutTrace("> lpData=%#x\n", lpEntry->lpData);
		OutTrace("> cbData=%d\n", lpEntry->cbData);
		OutTrace("> cbOverhead=%d\n", lpEntry->cbOverhead);
		OutTrace("> iRegionIndex=%d\n", lpEntry->iRegionIndex);
		OutTrace("> wFlags=%d\n", lpEntry->wFlags);
		OutTrace("> cbData=%d\n", lpEntry->cbData);
		OutTrace("> cbData=%#x\n", lpEntry->cbData);
		if(lpEntry->wFlags & PROCESS_HEAP_REGION){
			OutTrace("> dwCommittedSize=%#x\n", lpEntry->Region.dwCommittedSize);
			OutTrace("> dwUnCommittedSize=%#x\n", lpEntry->Region.dwUnCommittedSize);
			OutTrace("> lpFirstBlock=%#x\n", lpEntry->Region.lpFirstBlock);
			OutTrace("> lpLastBlock=%#x\n", lpEntry->Region.lpLastBlock);
		}
		else if((lpEntry->wFlags & PROCESS_HEAP_ENTRY_BUSY) && (lpEntry->wFlags & PROCESS_HEAP_ENTRY_MOVEABLE)) {
			OutTrace("> hMem=%#x\n", lpEntry->Block.hMem);
		}
	}
#endif // DXW_NOTRACES

	if(isVirtualHeap(hHeap)){
		OutErrorSYS("%s: UNIMPLEMENTED\n", ApiRef);
		ret = FALSE;
	}
	else {
		//if(dxw.dwFlags14 & SAFEHEAP) MessageBox(NULL, "HeapWalk", "DxWnd", 0);
		ret = (*pHeapWalk)(hHeap, lpEntry);
	}
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

// -- end of Heap wrappers
