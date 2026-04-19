#define _MODULE "dxwnd"

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

typedef DWORD (WINAPI *SuspendThread_Type)(HANDLE);
typedef DWORD (WINAPI *ResumeThread_Type)(HANDLE);
extern SuspendThread_Type pSuspendThread;
extern ResumeThread_Type pResumeThread;

#define MAX_THREAD_ARRAY 40

// forward declarations
BOOL SlowCpuSpeed(DWORD, DWORD, int);
BOOL LimitCpuUsage(DWORD, DWORD, int);

typedef void (WINAPI *Nap_Type)(DWORD);
Nap_Type pNap = NULL;

static void WINAPI Nap(DWORD mSec)
{
	LARGE_INTEGER t, t0;
	QueryPerformanceCounter(&t0);
	t = t0;
	while((DWORD)(t.QuadPart - t0.QuadPart) < (100 * mSec)){
		QueryPerformanceCounter(&t);
	}
}

DWORD WINAPI CpuSlow(LPVOID lpThreadParameter) 
{
	ApiName("CpuSlow");
	int iSlowDownRatio = dxw.SlowRatio;
	DWORD dwOwnerPID = GetCurrentProcessId();
	DWORD dwOwnerThread = GetCurrentThreadId();
	OutTraceDW("%s: starting CPUSlow dwOwnerPID=%#x Ratio=1:%d %s\n", 
		ApiRef, dwOwnerPID, iSlowDownRatio, (dxw.dwFlags10 & PRECISETIMING) ? "PRECISE" : "SLEEP");
	pNap = (dxw.dwFlags10 & PRECISETIMING) ? Nap : Sleep;
	if(!dwOwnerPID) return FALSE;
	if(iSlowDownRatio < 1) return FALSE;
	Sleep(100);
	while(TRUE) SlowCpuSpeed(dwOwnerPID, dwOwnerThread, iSlowDownRatio);
	return TRUE;
} 

DWORD WINAPI CpuLimit(LPVOID lpThreadParameter) 
{
	ApiName("CpuLimit");
	int iSlowDownRatio = dxw.SlowRatio;
	DWORD dwOwnerPID = GetCurrentProcessId();
	DWORD dwOwnerThread = GetCurrentThreadId();
	OutTraceDW("%s: starting CPULimit dwOwnerPID=%#x Ratio=1:%d %s\n", 
		ApiRef, dwOwnerPID, iSlowDownRatio, (dxw.dwFlags10 & PRECISETIMING) ? "PRECISE" : "SLEEP");
	pNap = (dxw.dwFlags10 & PRECISETIMING) ? Nap : Sleep;
	if(!dwOwnerPID) return FALSE;
	if(iSlowDownRatio < 1) return FALSE;
	return LimitCpuUsage(dwOwnerPID, dwOwnerThread, iSlowDownRatio);
}

BOOL SlowCpuSpeed(DWORD dwOwnerPID, DWORD dwOwnerThread, int iSlowDownRatio) 
{ 
	ApiName("SlowCpuSpeed");
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
	THREADENTRY32 te32; 
	HANDLE SuspThreads[MAX_THREAD_ARRAY];
	int iThreadIndex, iNumThreads;

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
	if(hThreadSnap == INVALID_HANDLE_VALUE) {
		OutTraceDW("%s: CreateToolhelp32Snapshot ERROR err=%d\n", ApiRef, GetLastError());
		return FALSE;
	}

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32); 

	// Retrieve information about the first thread, and exit if unsuccessful
	if(!Thread32First(hThreadSnap, &te32)){
		OutTraceDW("%s: Thread32First ERROR: err=%d\n", ApiRef, GetLastError());  // Show cause of failure
		CloseHandle(hThreadSnap);     // Must clean up the snapshot object!
		return FALSE;
	}

	// Now walk the thread list of the system,
	// and stop each low-priority thread
	iThreadIndex = 0;
	do { 
		if( (te32.th32OwnerProcessID == dwOwnerPID) && 
			(te32.th32ThreadID != dwOwnerThread) &&
			(te32.tpBasePri < THREAD_PRIORITY_TIME_CRITICAL)) {

			HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
			(*pSuspendThread)(hThread);
			SuspThreads[iThreadIndex++] = hThread;
		}
	} while(Thread32Next(hThreadSnap, &te32) && (iThreadIndex<MAX_THREAD_ARRAY));
	iNumThreads = iThreadIndex;

	//  Don't forget to clean up the snapshot object.
	CloseHandle(hThreadSnap);

	(*pNap)(iSlowDownRatio);

	// Now resume all suspended threads
	for(iThreadIndex=0; iThreadIndex<iNumThreads; iThreadIndex++) {
		(*pResumeThread)(SuspThreads[iThreadIndex]);
		CloseHandle(SuspThreads[iThreadIndex]);
	}

	(*pNap)(1);
	return TRUE;
}

typedef struct{
	DWORD tid;
	HANDLE hThread;
	FILETIME LastUsed;
	signed long DeltaUsed;
	BOOL Suspended;
} ThreadDesc_Type;

static FILETIME FTFTSUM(FILETIME a, FILETIME b)
{
	FILETIME s;
	s.dwHighDateTime = a.dwHighDateTime + b.dwHighDateTime;
	__try{ s.dwLowDateTime = a.dwLowDateTime + b.dwLowDateTime;}
	__except (EXCEPTION_EXECUTE_HANDLER){ s.dwHighDateTime += 1; }; // add reminder on overflow
	return s;
}

static FILETIME FTDWSUM(FILETIME a, DWORD b)
{
	FILETIME s;
	s.dwHighDateTime = a.dwHighDateTime;
	__try{ s.dwLowDateTime = a.dwLowDateTime + b;}
	__except (EXCEPTION_EXECUTE_HANDLER){ s.dwHighDateTime += 1; }; // add reminder on overflow
	return s;
}

static DWORD DWDIFF(FILETIME a, FILETIME b)
{
	// we suppose that the difference can't be greater to 2^sizeof(DWORD), so
	// dwHighDateTime values are either identical or 1 greater
	DWORD d;
	if(a.dwHighDateTime == a.dwHighDateTime)
		d = a.dwLowDateTime - b.dwLowDateTime;
	else
		d = b.dwLowDateTime - a.dwLowDateTime;
	return d;
}

//#define DEBUGTRACE 1

static BOOL RefreshThreadList(int *iNumThreads, ThreadDesc_Type ProcessThreads[], DWORD dwOwnerPID, DWORD dwOwnerThread)
{
	ApiName("LimitCpuUsage");
	THREADENTRY32 te32;
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
	int iThreadIndex;
	FILETIME CreationTime, ExitTime, KernelTime, UserTime;

	OutTraceDW("%s: refreshing thread list\n", ApiRef);
	te32.dwSize = sizeof(THREADENTRY32); 

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
	if(hThreadSnap == INVALID_HANDLE_VALUE) {
		OutErrorSYS("%s: CreateToolhelp32Snapshot ERROR err=%d\n", ApiRef, GetLastError());
		return FALSE;
	}

	// Retrieve information about the first thread, and exit if unsuccessful
	if(!Thread32First(hThreadSnap, &te32)){
		OutErrorSYS("%s: Thread32First ERROR err=%d\n", ApiRef, GetLastError());  // Show cause of failure
		CloseHandle(hThreadSnap);     // Must clean up the snapshot object!
		return FALSE;
	}

	// Now walk the thread list of the system threads,
	// and take a snapshot of each target low-priority thread
	do { 
		if( (te32.th32OwnerProcessID == dwOwnerPID) && 
			(te32.th32ThreadID != dwOwnerThread) &&
			(te32.tpBasePri < THREAD_PRIORITY_TIME_CRITICAL)) {

			// find threads already listed
			BOOL IsListed = FALSE;
			iThreadIndex = *iNumThreads;
			for(int j=0; j<(*iNumThreads); j++){
				if(ProcessThreads[j].tid == 0) iThreadIndex=j; // find a list hole, if existing
				if(te32.th32ThreadID == ProcessThreads[j].tid){
					IsListed = TRUE;
					break;
				}
			}

			// if not in the list, add
			if(!IsListed){
				HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION|THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
				if(!GetThreadTimes(hThread, &CreationTime, &ExitTime, &KernelTime, &UserTime)) continue;
				ProcessThreads[iThreadIndex].LastUsed = FTFTSUM(UserTime, KernelTime);
				ProcessThreads[iThreadIndex].hThread =hThread;
				ProcessThreads[iThreadIndex].tid = te32.th32ThreadID;
				ProcessThreads[iThreadIndex].Suspended = FALSE;
#ifdef DEBUGTRACE
				OutTrace("Tid[%d]:%#x init time=%d\n", iThreadIndex, te32.th32ThreadID, ProcessThreads[iThreadIndex].LastUsed);
#endif
				iThreadIndex++;
				if(iThreadIndex> *iNumThreads) *iNumThreads = iThreadIndex;
			}
		}
	} while(Thread32Next(hThreadSnap, &te32) && (*iNumThreads<MAX_THREAD_ARRAY-1));
#ifdef DEBUGTRACE
	OutTrace("Got %d threads\n", *iNumThreads);
#endif
	//  Don't forget to clean up the snapshot object.
	CloseHandle(hThreadSnap);
	return TRUE;
}


BOOL LimitCpuUsage(DWORD dwOwnerPID, DWORD dwOwnerThread, int iSlowDownRatio) 
{ 
	ApiName("LimitCpuUsage");
	ThreadDesc_Type ProcessThreads[MAX_THREAD_ARRAY];
	int iThreadIndex, iNumThreads;
	int cycle;
	int iResumeCount;
	DWORD iTimeLimit = 100000;
	DWORD iTimeSlot = 100000 / iSlowDownRatio;
	FILETIME CreationTime, ExitTime, KernelTime, UserTime;

	OutTraceDW("%s: TimeLimit=%ld TimeSlot=%ld\n", ApiRef, iTimeLimit, iTimeSlot);

	// Fill in the size of the structure before using it. 
	for(int i=0; i<MAX_THREAD_ARRAY; i++) {
		ProcessThreads[i].DeltaUsed = 0;
		ProcessThreads[i].Suspended = FALSE;
		ProcessThreads[i].tid = 0;
	}

	iNumThreads = 0;
	for(cycle=0; TRUE; cycle++){
		if(cycle == 0) if(!RefreshThreadList(&iNumThreads, ProcessThreads, dwOwnerPID, dwOwnerThread)) break;

		if(cycle > 100) cycle=0; // every 100 cyces forces a thread list refresh

		for(iThreadIndex=0; iThreadIndex<iNumThreads; iThreadIndex++) {
			ThreadDesc_Type *t = &ProcessThreads[iThreadIndex];
			if (t->DeltaUsed > (signed long)iTimeLimit) {
#ifdef DEBUGTRACE
				OutTrace("Tid[%d]:%#x delta=%d stopped @%d\n", iThreadIndex, t->tid, t->DeltaUsed,  __LINE__);
#endif
				if ((iResumeCount=(*pSuspendThread)(t->hThread))== -1) {
					t->tid = NULL;
					CloseHandle(t->hThread);
					continue;
				}
				t->Suspended = TRUE;
				t->DeltaUsed -= iTimeSlot;
			}
		}

		(*pNap)(iSlowDownRatio);

		for(iThreadIndex=0; iThreadIndex<iNumThreads; iThreadIndex++) {
			ThreadDesc_Type *t = &ProcessThreads[iThreadIndex];

			if (t->tid == NULL) continue; // skip terminated ones

			if (t->Suspended) {
#ifdef DEBUGTRACE
				OutTrace("Tid[%d]=%#x delta=%d started @%d\n", iThreadIndex, t->tid, t->DeltaUsed, __LINE__);
#endif
				if ((iResumeCount=(*pResumeThread)(t->hThread))== -1) {
					t->tid = NULL;
					CloseHandle(t->hThread);
					continue;
				}
				t->Suspended = FALSE;
				t->DeltaUsed -= iTimeSlot;
			}
			else {
				if(!GetThreadTimes(t->hThread, &CreationTime, &ExitTime, &KernelTime, &UserTime)) {
					t->tid = NULL;
					CloseHandle(t->hThread);
					continue;
				}	
				FILETIME tmp = t->LastUsed;
				t->LastUsed = FTFTSUM(UserTime, KernelTime);
				t->DeltaUsed = t->DeltaUsed + DWDIFF(t->LastUsed, tmp);
#ifdef DEBUGTRACE
				OutTrace("Tid[%d]:%#x delta=%d measured @%d\n", iThreadIndex, t->tid, t->DeltaUsed, __LINE__);
#endif
			}

		}
		(*pNap)(1);
	}

	// should never go here, but in case, awake all suspended threads
	for(iThreadIndex=0; iThreadIndex<iNumThreads; iThreadIndex++) {
		ThreadDesc_Type *t = &ProcessThreads[iThreadIndex];
		if (t->tid && t->Suspended) {
			(*pResumeThread)(t->hThread);
		}
	}
	return TRUE;
}

