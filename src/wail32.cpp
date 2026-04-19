#define  _CRT_SECURE_NO_WARNINGS

#define _MODULE "wail32"

#include <windows.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"

typedef void* HSAMPLE;        // Opaque handle to sample
typedef void (WINAPI *AIL_lock_Type)(void);
typedef void (WINAPI *AIL_sample_Type)(HSAMPLE);

AIL_lock_Type pAIL_lock, pAIL_unlock, pAIL_serve;
AIL_sample_Type pAIL_start_sample, pAIL_stop_sample, pAIL_resume_sample, pAIL_end_sample;
HANDLE ghAILMutex;

void WINAPI extAIL_lock(void);
void WINAPI extAIL_unlock(void);
void WINAPI extAIL_serve(void);
void WINAPI extAIL_start_sample(HSAMPLE);
void WINAPI extAIL_stop_sample(HSAMPLE);
void WINAPI extAIL_resume_sample(HSAMPLE);
void WINAPI extAIL_end_sample(HSAMPLE);

typedef void (WINAPI *MEM_free_lock_Type)(DWORD);
MEM_free_lock_Type pMEM_free_lock;
void WINAPI extMEM_free_lock(DWORD);

typedef void (WINAPI *AIL_send_channel_voice_message_Type)(DWORD, DWORD, DWORD, DWORD, DWORD);
AIL_send_channel_voice_message_Type pAIL_send_channel_voice_message;
void WINAPI extAIL_send_channel_voice_message(DWORD, DWORD, DWORD, DWORD, DWORD);

static HookEntryEx_Type Hooks[]={
	{HOOK_HOT_REQUIRED, 0x0000, "AIL_lock", (FARPROC)NULL, (FARPROC *)&pAIL_lock, (FARPROC)extAIL_lock},	
	{HOOK_HOT_REQUIRED, 0x0000, "AIL_unlock", (FARPROC)NULL, (FARPROC *)&pAIL_unlock, (FARPROC)extAIL_unlock},	
	{HOOK_HOT_REQUIRED, 0x0000, "AIL_serve", (FARPROC)NULL, (FARPROC *)&pAIL_serve, (FARPROC)extAIL_serve},	
	{HOOK_HOT_REQUIRED, 0x0000, "AIL_start_sample", (FARPROC)NULL, (FARPROC *)&pAIL_start_sample, (FARPROC)extAIL_start_sample},	
	{HOOK_HOT_REQUIRED, 0x0000, "AIL_stop_sample", (FARPROC)NULL, (FARPROC *)&pAIL_stop_sample, (FARPROC)extAIL_stop_sample},	
	{HOOK_HOT_REQUIRED, 0x0000, "AIL_resume_sample", (FARPROC)NULL, (FARPROC *)&pAIL_resume_sample, (FARPROC)extAIL_resume_sample},	
	{HOOK_HOT_REQUIRED, 0x0000, "AIL_end_sample", (FARPROC)NULL, (FARPROC *)&pAIL_end_sample, (FARPROC)extAIL_end_sample},	
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type Hooks2[]={
	{HOOK_HOT_REQUIRED, 0x0000, "_MEM_free_lock@4", (FARPROC)NULL, (FARPROC *)&pMEM_free_lock, (FARPROC)extMEM_free_lock},	
	{HOOK_HOT_REQUIRED, 0x0000, "_AIL_send_channel_voice_message@20", (FARPROC)NULL, (FARPROC *)&pAIL_send_channel_voice_message, (FARPROC)extAIL_send_channel_voice_message},	
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static char *libname = "WAIL32.DLL";

void HookWAIL32(HMODULE hModule)
{
	ghAILMutex = CreateMutex(NULL, FALSE, NULL);
	if(ghAILMutex == NULL){
		OutTraceE("HookWAIL32 ERROR: CreateMutex err=%d\n", GetLastError());
		return;
	}
	HookLibraryEx(hModule, Hooks, libname);
	HookLibraryEx(hModule, Hooks2, libname);
}

BOOL gLocked = FALSE;

#ifdef WAIL32TRACEONLY

void WINAPI extAIL_lock(void)
{
	OutTraceSYS(">> AIL_lock\n");
	WaitForSingleObject(ghAILMutex, INFINITE);
	if(gLocked) {
		OutTraceSYS(">- AIL_lock SKIP\n");
		ReleaseMutex(ghAILMutex);
		return;
	}
	gLocked = TRUE;
	ReleaseMutex(ghAILMutex);
	(*pAIL_lock)();
}

void WINAPI extAIL_unlock(void)
{
	OutTraceSYS(">> AIL_unlock\n");
	(*pAIL_unlock)();
	gLocked = FALSE;
}

void WINAPI extAIL_serve(void)
{
	OutTraceSYS(">> AIL_serve\n");
	WaitForSingleObject(ghAILMutex, INFINITE);
	if(gLocked) {
		OutTraceSYS(">- AIL_serve SKIP\n");
		ReleaseMutex(ghAILMutex);
		return;
	}
	gLocked = TRUE;
	ReleaseMutex(ghAILMutex);
	(*pAIL_serve)();
	gLocked = FALSE;
}

void WINAPI extAIL_start_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_start_sample s=%#x\n", s); 
	WaitForSingleObject(ghAILMutex, INFINITE);
	if(gLocked) {
		OutTraceSYS(">- AIL_start_sample SKIP\n");
		ReleaseMutex(ghAILMutex);
		return;
	}
	//gLocked = TRUE;
	ReleaseMutex(ghAILMutex);
	(*pAIL_start_sample)(s);
	//(*pAIL_unlock)();
}

void WINAPI extAIL_stop_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_stop_sample s=%#x\n", s);
	//(*pAIL_lock)();
	//(*pAIL_stop_sample)(s);
	//(*pAIL_unlock)();
}

void WINAPI extAIL_resume_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_resume_sample s=%#x\n", s); 
	//(*pAIL_lock)();
	//(*pAIL_resume_sample)(s);
	//(*pAIL_unlock)();
}

void WINAPI extAIL_end_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_end_sample s=%#x\n", s); 
	//(*pAIL_lock)();
	//(*pAIL_end_sample)(s);
	//(*pAIL_unlock)();
}

#else

void WINAPI extAIL_lock(void)
{
	OutTraceSYS(">> AIL_lock\n");
	(*pAIL_lock)();
	OutTraceSYS("<< AIL_lock\n");
}

void WINAPI extAIL_unlock(void)
{
	OutTraceSYS(">> AIL_unlock\n");
	(*pAIL_unlock)();
	OutTraceSYS("<< AIL_unlock\n");
}

void WINAPI extAIL_serve(void)
{
	OutTraceSYS(">> AIL_serve\n");
	(*pAIL_serve)();
	OutTraceSYS("<< AIL_serve\n");
}

void WINAPI extAIL_start_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_start_sample s=%#x\n", s); 
	(*pAIL_start_sample)(s);
	OutTraceSYS("<< AIL_start_sample s=%#x\n", s); 
}

void WINAPI extAIL_stop_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_stop_sample s=%#x\n", s);
	(*pAIL_stop_sample)(s);
	OutTraceSYS("<< AIL_stop_sample s=%#x\n", s);
}

void WINAPI extAIL_resume_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_resume_sample s=%#x\n", s); 
	(*pAIL_resume_sample)(s);
	OutTraceSYS("<< AIL_resume_sample s=%#x\n", s); 
}

void WINAPI extAIL_end_sample(HSAMPLE s)
{ 
	OutTraceSYS(">> AIL_end_sample s=%#x\n", s); 
	(*pAIL_end_sample)(s);
	OutTraceSYS("<< AIL_end_sample s=%#x\n", s); 
}
#endif

void WINAPI extMEM_free_lock(DWORD heap)
{
	ApiName("_MEM_free_lock@4");
	OutTrace("%s: heap=%#x\n", ApiRef, heap);

	if(dxw.dwFlags14 & HEAPLEAK) {
		OutTrace("%s: HEAPLEAK\n", ApiRef);
		return;
	}

	__try{
		(*pMEM_free_lock)(heap);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTrace("%s: exception caught\n", ApiRef);
	}
}

void WINAPI extAIL_send_channel_voice_message(DWORD p1, DWORD p2, DWORD p3, DWORD p4, DWORD p5)
{
	OutTrace("AIL_send_channel_voice_message: BYPASS #%x #%x #%x #%x #%x\n", p1, p2, p3, p4, p5);
	//return 0;
}
