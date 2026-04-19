#define _MODULE "ole32"
#define INITGUID

#include <dxdiag.h>
#include <dsound.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "comvsound.h"
#include "dshow.h"
#include "dshow.wrap.hpp"

extern void HookDirectSoundObj(LPDIRECTSOUND *);
extern void HookDirectSound8Obj(LPDIRECTSOUND8 *);
extern void HookDirectDrawFactory(void *);
extern void HookDlls(HMODULE);
extern void HookDirectPlay(REFIID, void *);
extern void HookAMMultiMediaStream(LPVOID);
extern void HookDirectInputSession(void **, int, BOOL);

#ifdef HOOKSHELL32
extern void HookShellLinkA(void *);
#endif // HOOKSHELL32

typedef HRESULT (WINAPI *CoInitializeEx_Type)(LPVOID, DWORD);
CoInitializeEx_Type pCoInitializeEx;
HRESULT WINAPI extCoInitializeEx(LPVOID, DWORD);

typedef HRESULT (WINAPI *CoGetClassObject_Type)(REFCLSID, DWORD, LPVOID, REFIID, LPVOID FAR *);
CoGetClassObject_Type pCoGetClassObject;
HRESULT WINAPI extCoGetClassObject(REFCLSID, DWORD, LPVOID, REFIID, LPVOID FAR *);

typedef HRESULT (WINAPI *LoadTypeLibEx_Type)(LPCOLESTR, REGKIND, ITypeLib **);
LoadTypeLibEx_Type pLoadTypeLibEx;
HRESULT WINAPI extLoadTypeLibEx(LPCOLESTR, REGKIND, ITypeLib **);

typedef HRESULT (WINAPI *CreateInstance_Type)(LPVOID, IUnknown *, REFIID, void **);
CreateInstance_Type pCreateInstance;
HRESULT WINAPI extCreateInstance(LPVOID, IUnknown *, REFIID, void **);

static BOOL bRecursedHook = FALSE;

static HookEntryEx_Type HooksOle32[]={
	{HOOK_HOT_CANDIDATE, 0, "CoCreateInstance", (FARPROC)CoCreateInstance, (FARPROC *)&pCoCreateInstance, (FARPROC)extCoCreateInstance},
	{HOOK_HOT_CANDIDATE, 0, "CoCreateInstanceEx", (FARPROC)CoCreateInstanceEx, (FARPROC *)&pCoCreateInstanceEx, (FARPROC)extCoCreateInstanceEx}, 
	{HOOK_HOT_CANDIDATE, 0, "CoInitialize", (FARPROC)CoInitialize, (FARPROC *)&pCoInitialize, (FARPROC)extCoInitialize}, 
	{HOOK_HOT_CANDIDATE, 0, "CoInitializeEx", (FARPROC)CoInitializeEx, (FARPROC *)&pCoInitializeEx, (FARPROC)extCoInitializeEx}, 
	{HOOK_HOT_CANDIDATE, 0, "OleInitialize", NULL, (FARPROC *)&pOleInitialize, (FARPROC)extOleInitialize}, 
	{HOOK_HOT_CANDIDATE, 0, "CoUninitialize", NULL, (FARPROC *)&pCoUninitialize, (FARPROC)extCoUninitialize}, 
	{HOOK_HOT_CANDIDATE, 0, "CoGetClassObject", (FARPROC)CoGetClassObject, (FARPROC *)&pCoGetClassObject, (FARPROC)extCoGetClassObject},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type HooksOleAut32[]={
	{HOOK_HOT_CANDIDATE, 0, "LoadTypeLibEx", (FARPROC)LoadTypeLibEx, (FARPROC *)&pLoadTypeLibEx, (FARPROC)extLoadTypeLibEx},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

extern HRESULT WINAPI extDirectDrawCreate(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);
extern HRESULT WINAPI extDirectDrawCreateEx(GUID FAR *, LPDIRECTDRAW FAR *, REFIID, IUnknown FAR *);

void HookOle32(HMODULE module)
{
	HookLibraryEx(module, HooksOle32, "ole32.dll");
}

void HookOleAut32(HMODULE module)
{
	HookLibraryEx(module, HooksOleAut32, "oleaut32.dll");
}

FARPROC Remap_Ole32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, HooksOle32)) return addr;
	return NULL;
}

FARPROC Remap_OleAut32_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, HooksOleAut32)) return addr;
	return NULL;
}

// -------------------------------------------------------------------------------------
// Ole32 CoCreateInstance handling: you can create DirectDraw objects through it!
// utilized so far in a single game: "Axis & Allies"
// another one: "Crusaders of Might and Magic" ....
// -------------------------------------------------------------------------------------

extern void HookDDSession(LPDIRECTDRAW *, int); 

#ifndef DXW_NOTRACES
static char *ExplainCoCError(DWORD r)
{
	char *s;
	switch(r){
		case S_OK: s="OK"; break;
		case CO_E_NOTINITIALIZED: s="CO_E_NOTINITIALIZED"; break;
		case CLASS_E_NOAGGREGATION: s="CLASS_E_NOAGGREGATION"; break;
		case CLASS_E_CLASSNOTAVAILABLE: s="CLASS_E_CLASSNOTAVAILABLE"; break;
		case CLASS_E_NOTLICENSED: s="CLASS_E_NOTLICENSED"; break;
		case REGDB_E_CLASSNOTREG: s="REGDB_E_CLASSNOTREG"; break;
		case E_NOINTERFACE: s="E_NOINTERFACE"; break;
		case E_POINTER: s="E_POINTER"; break;
		case RPC_E_SERVERFAULT: s="RPC_E_SERVERFAULT"; break;
		case RPC_E_CHANGED_MODE: s="RPC_E_CHANGED_MODE"; break;
		case RPC_E_INVALIDMETHOD: s="RPC_E_INVALIDMETHOD"; break;
		case 0x80012: s="Interfaces not all implemented"; break;
		default: s="unknown"; break;
	}
	return s;
}
#endif

char *sRIID(REFIID riid)
{
	static char sRIIDBuffer[81];
	OLECHAR* guidString;
	StringFromCLSID(riid, &guidString);
	sprintf_s(sRIIDBuffer, 80, "%ls",  guidString);
	::CoTaskMemFree(guidString);
	return sRIIDBuffer;
}

#ifdef UNUSED
static void HookClassObject(LPVOID *ppv)
{
	ApiName("HookClassObject");

	OutTraceCOM("%s: clsid=%s\n", ApiRef, sRIID((const IID &)ppv));
	// now hook IClassFactory methods
	//SetHook((void *)(**(DWORD **)ppv),     extQueryInterfaceCF, (void **)&pQueryInterfaceCF, "QueryInterface(CF)");
	//SetHook((void *)(**(DWORD **)ppv + 8), extReleaseCF, (void **)&pReleaseCF, "Release(CF)");
	SetHook((void *)(**(DWORD **)ppv + 12), extCreateInstance, (void **)&pCreateInstance, "CreateInstance");
	//SetHook((void *)(**(DWORD **)ppv + 16), extLockServer, (void **)&pLockServer, "LockServer");
}
#endif

static void HookOLEClass(REFCLSID rclsid)
{
	char sSubKey[80];
	HKEY hKey, hSrvKey, hHdlKey;
	LSTATUS oleret;
	char sClassName[81];
	char sDLLPath[MAX_PATH+1];
	LONG len;
	HMODULE hMod;
	ApiName("HookOLEClass");

	OutTraceCOM("%s: clsid=%s\n", ApiRef, sRIID(rclsid));
	if(!pRegOpenKeyA) pRegOpenKeyA = RegOpenKeyA;
	sprintf_s(sSubKey, 80, "\\CLSID\\%s", sRIID(rclsid));
	oleret = (*pRegOpenKeyA)(HKEY_CLASSES_ROOT, sSubKey, &hKey);
	if(oleret != ERROR_SUCCESS) return;
	if(!pRegQueryValueA) pRegQueryValueA = RegQueryValueA;
	len = 80;
	oleret = (*pRegQueryValueA)(hKey, "", sClassName, &len);
	_if(oleret == ERROR_SUCCESS) OutTraceCOM("%s: class=%s\n", ApiRef, sClassName);

	oleret = (*pRegOpenKeyA)(hKey, "InProcServer32", &hSrvKey);
	if(oleret != ERROR_SUCCESS) hSrvKey = NULL;
	oleret = (*pRegOpenKeyA)(hKey, "InProcHandler32", &hHdlKey);
	if(oleret != ERROR_SUCCESS) hHdlKey = NULL;

	if(hSrvKey){
		len = MAX_PATH;
		oleret = (*pRegQueryValueA)(hSrvKey, "", sDLLPath, &len);
		if(oleret != ERROR_SUCCESS) return;
		OutTraceCOM("%s: InProcServer32 dllpath=\"%s\"\n", ApiRef, sDLLPath);
		if(dxw.dwTFlags & ASSERTDIALOG) {
			char msg[MAX_PATH * 2];
			sprintf_s(msg, MAX_PATH*2, "Hooking OLE server\nclass=%s\nGUID=%s\ndllpath=\"%s\".",
				sClassName, sRIID(rclsid), sDLLPath);
			MessageBox(0, msg, "DxWnd Assert", 0);
		}
		// v2.05.44 fix: don't hook the dll directly, this is ok only when handling a unknown module.
		// in case of known modules, a direct hook may bring recursion or improper hooking of dependencies.
		// better mimick a LoadLibrary operation that has all code to manage that.
		//hMod = (*pLoadLibraryA)(sDLLPath);
		//if(hMod) HookDlls(hMod);
		extern HMODULE WINAPI LoadLibraryExWrapper(LPVOID, BOOL, HANDLE, DWORD, ApiArg);
		LoadLibraryExWrapper(sDLLPath, FALSE, NULL, 0, ApiRef);
	}

	if(hHdlKey){
		len = MAX_PATH;
		oleret = (*pRegQueryValueA)(hHdlKey, "", sDLLPath, &len);
		if(oleret != ERROR_SUCCESS) return;
		OutTraceCOM("%s: InProcHandler32 dllpath=\"%s\"\n", ApiRef, sDLLPath);
		if(dxw.dwTFlags & ASSERTDIALOG) {
			char msg[MAX_PATH * 2];
			sprintf_s(msg, MAX_PATH*2, "Hooking OLE handler\nclass=%s\nGUID=%s\ndllpath=\"%s\".",
				sClassName, sRIID(rclsid), sDLLPath);
			MessageBox(0, msg, "DxWnd Assert", 0);
		}
		hMod = (*pLoadLibraryA)(sDLLPath);
		if(hMod) HookDlls(hMod);
	}

	if(!pRegCloseKey) pRegCloseKey = RegCloseKey;
	if(hSrvKey) (*pRegCloseKey)(hSrvKey);
	if(hHdlKey) (*pRegCloseKey)(hHdlKey);
	(*pRegCloseKey)(hKey);
}

typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID, LPVOID *);
QueryInterface_Type pQueryInterface;
HRESULT WINAPI extQueryInterface(void *, REFIID, LPVOID *);
typedef ULONG (WINAPI *AddRef_Type)(void *);
AddRef_Type pAddRef;
HRESULT WINAPI extAddRef(LPVOID obj);
typedef ULONG (WINAPI *Release_Type)(void *);
Release_Type pRelease;
ULONG WINAPI extRelease(LPVOID obj);

extern HRESULT WINAPI QueryInterfaceDS(ApiArg, QueryInterface_Type, void *, REFIID, LPVOID *);

HRESULT WINAPI extQueryInterface(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("I_Unknown::QueryInterface", pQueryInterface, obj, riid, obp); }

HRESULT WINAPI extAddRef(LPVOID obj)
{
	ApiName("I_Unknown::AddRef");
	OutTraceCOM("%s\n", ApiRef);
	return (*pAddRef)(obj);
}

ULONG WINAPI extRelease(LPVOID obj)
{
	ApiName("I_Unknown::Release");
	ULONG refCount;
	OutTraceCOM("%s\n", ApiRef);
	refCount = (*pRelease)(obj);
	OutTraceCOM("%s: ref=%d %s\n", ApiRef, refCount, refCount ? "" : "DESTROYED");
	return refCount;
}

static void HookUnknown(LPVOID obj)
{
	SetHook((void *)(**(DWORD **)obj), extQueryInterface, (void **)&pQueryInterface, "QueryInterface");
	//SetHook((void *)(**(DWORD **)obj +  4), extAddRef, (void **)&pAddRef, "AddRef");
	//SetHook((void *)(**(DWORD **)obj +  8), extRelease, (void **)&pRelease, "Release");
}

static const GUID DX1VBGUID = {0xE7FF1300,0x96A5,0x11D3,{0xAC, 0x85, 0x00, 0xC0, 0x4F, 0xC2, 0xC6, 0x02}}; // ???
static const GUID DX7VBGUID = {0xE1211242,0x8E94,0x11D1,{0x88, 0x08, 0x00, 0xC0, 0x4F, 0xC2, 0xC6, 0x02}}; // DX7VB.DLL
static const GUID DX8VBGUID = {0xE1211242,0x8E94,0x11D1,{0x88, 0x08, 0x00, 0xC0, 0x4F, 0xC2, 0xC6, 0x03}}; // DX8VB.DLL

static const GUID IID_IDirectDraw3 = {0x618f8ad4,0x8b7a,0x11d0,{0x8f, 0xcc, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D}}; // {618F8AD4-8B7A-11d0-8FCC-00C04FD9189D} 

// used in "DOVE" with dx7vb.dll
// E1211353-8E94-11D1-8808-00C04FC2C602 IDirectX6.0 ??
// FAFA3599-8B72-11D2-90B2-00C04FC2C602 IDirectX7 ??
// 7FD52380-4E07-101B-AE2D-08002B2EC713 IID_IPersistStreamInit
// 37D84F60-42CB-11CE-8135-00AA004BB851 IPersistPropertyBag

static HRESULT FilterGUID(ApiArg, REFCLSID rclsid, REFIID riid)
{
	HRESULT res = 0;
	switch (rclsid.Data1) {
		case 0x99D54F63: // {99D54F63-1A69-41AE-AA4D-C976EB3F0713}(VMR Allocator Presenter)
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				if(res = CheckInterfaceDS(ApiRef, riid)) {
					OutTraceDW("%s: DirectShow class filtered res=E_NOINTERFACE\n", ApiRef);
					return res;
				}
			}
			break;
		case 0xB98D13E7:
			//B98D13E7-55DB-4385-A33D-09FD1BA26338
			if(dxw.dwFlags18 & NOLAVFILTERS){
				OutTraceDW("%s: LAV filters class filtered res=E_NOINTERFACE\n", ApiRef);
				return E_NOINTERFACE;
			}
			break;
		case 0x47d4d946: // CLSID_DirectSound
		case 0x3901cc3f: // "CLSID_DirectSound8"
			if(dxw.dwFlags19 & NODIRECTSOUND) {
				OutTrace("%s: NODIRECTSOUND guid=%#x\n", ApiRef, rclsid.Data1);
				return E_NOINTERFACE;
			}
	}
	return res;
}

static void HookGUIDObject(ApiArg, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv)
{
	HRESULT res;

	HookOLEClass(rclsid);

	switch (rclsid.Data1) {
		case 0xD7B70EE0: // CLSID_DirectDraw:
		// v2.05.14: added rclsid==CLSID_DirectDraw7 case found in "Matatiki"
		case 0x3C305196: // CLSID_DirectDraw7:
			// v2.03.18: fixed
			OutTraceDW("%s: CLSID_DirectDraw object\n", ApiRef);
			switch (*(DWORD *)&riid){
				LPDIRECTDRAW lpOldDDraw;
				case 0x6C14DB80:
					// must go through DirectDrawCreate: needed for "Darius Gaiden"
					OutTraceDW("%s: IID_DirectDraw RIID\n", ApiRef);
					res=extDirectDrawCreate(NULL, &lpOldDDraw, 0);
					_if(res) OutTraceDW("%s: DirectDrawCreate res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					res=lpOldDDraw->QueryInterface(IID_IDirectDraw, (LPVOID *)ppv);
					_if(res) OutTraceDW("%s: QueryInterface res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					lpOldDDraw->Release();
					break;
				case 0xB3A6F3E0:
					OutTraceDW("%s: IID_DirectDraw2 RIID\n", ApiRef);
					res=extDirectDrawCreate(NULL, &lpOldDDraw, 0);
					_if(res) OutTraceDW("%s: DirectDrawCreate res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					res=lpOldDDraw->QueryInterface(IID_IDirectDraw2, (LPVOID *)ppv);
					_if(res) OutTraceDW("%s: QueryInterface res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					lpOldDDraw->Release();
					break;
				case 0x618f8ad4:
					OutTraceDW("%s: IID_DirectDraw3 RIID\n", ApiRef);
					res=extDirectDrawCreate(NULL, &lpOldDDraw, 0);
					_if(res) OutTraceDW("%s: DirectDrawCreate res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					res=lpOldDDraw->QueryInterface(IID_IDirectDraw3, (LPVOID *)ppv);
					_if(res) OutTraceDW("%s: QueryInterface res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					lpOldDDraw->Release();
					break;
				case 0x9C59509A:
					OutTraceDW("%s: IID_DirectDraw4 RIID\n", ApiRef);
					res=extDirectDrawCreate(NULL, &lpOldDDraw, 0);
					_if(res) OutTraceDW("%s: DirectDrawCreate res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					res=lpOldDDraw->QueryInterface(IID_IDirectDraw4, (LPVOID *)ppv);
					_if(res) OutTraceDW("%s: QueryInterface res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					lpOldDDraw->Release();
					break;
				case 0x15E65EC0:
					OutTraceDW("%s: IID_DirectDraw7 RIID\n", ApiRef);
					res=extDirectDrawCreateEx(NULL, (LPDIRECTDRAW *)ppv, IID_IDirectDraw7, 0);
					_if(res) OutTraceDW("%s: DirectDrawCreateEx res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
					break;
				case 0xE436EBB3:
					break;
			}
			break;
		// DirectInput
		//CLSID_DirectInput           equ 25E609E0-B259-11CF-BFC7-444553540000
		//CLSID_DirectInputDevice     equ 25E609E1-B259-11CF-BFC7-444553540000
		//DEFINE_GUID(IID_IDirectInputA,     0x89521360,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00 ,0x00);
		//DEFINE_GUID(IID_IDirectInputW,     0x89521361,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
		//DEFINE_GUID(IID_IDirectInput2A,    0x5944E662,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
		//DEFINE_GUID(IID_IDirectInput2W,    0x5944E663,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
		//DEFINE_GUID(IID_IDirectInput7A,    0x9A4CB684,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
		//DEFINE_GUID(IID_IDirectInput7W,    0x9A4CB685,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
		//DEFINE_GUID(IID_IDirectInput8A,    0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
		//DEFINE_GUID(IID_IDirectInput8W,    0xBF798031,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
#ifndef ISWIDEC
#define ISASCII TRUE
#define ISWIDEC FALSE
#endif
		case 0x25E609E0:
			if(dxw.dwFlags1 & HOOKDI){
				switch(*(DWORD *)&riid){
					case 0x89521360: HookDirectInputSession(ppv, 1, ISASCII); break;
					case 0x89521361: HookDirectInputSession(ppv, 1, ISWIDEC); break;
					case 0x5944E662: HookDirectInputSession(ppv, 2, ISASCII); break;
					case 0x5944E663: HookDirectInputSession(ppv, 2, ISWIDEC); break;
					case 0x9A4CB684: HookDirectInputSession(ppv, 7, ISASCII); break;
					case 0x9A4CB685: HookDirectInputSession(ppv, 7, ISWIDEC); break;
					case 0xBF798030: HookDirectInputSession(ppv, 8, ISASCII); break;
					case 0xBF798031: HookDirectInputSession(ppv, 8, ISWIDEC); break;
					default: OutErrorCOM("%s: unhooked RIID!\n"); break;
				}
			}
			break;

		case 0xA65B8071: // CLSID_DxDiagProvider:
			if ((rclsid.Data2==0x3BFE) && (rclsid.Data3 == 0x4213)){
			OutTraceDW("%s: CLSID_DxDiagProvider object\n", ApiRef);
				res=HookDxDiag(riid, ppv);
			}
			break;

		case 0x47d4d946: // CLSID_DirectSound
			OutTraceDW("%s: CLSID_DirectSound object\n", ApiRef);
			if(dxw.dwFlags18 & BYPASSDSOUND)
				*ppv = new(IFakeDirectSound);
			else
				HookDirectSoundObj((LPDIRECTSOUND *)ppv); 
			break;

		case 0x3901cc3f: // "CLSID_DirectSound8"
			OutTraceDW("%s: CLSID_DirectSound8 object\n", ApiRef);
			if(dxw.dwFlags18 & BYPASSDSOUND)
				*ppv = new(IFakeDirectSound8);
			else
				HookDirectSound8Obj((LPDIRECTSOUND8 *)ppv); 
			break;

		case 0x4fd2a832: // CLSID_DirectDrawFactory 
			if ((rclsid.Data2==0x86c8) && (rclsid.Data3 == 0x11d0)){
				OutTraceDW("%s: CLSID_DirectDrawFactory object\n", ApiRef);
				HookDirectDrawFactory((void *)ppv);
			}
			break;

		// case 0xD1EB6D20: // CLSID_DirectPlay
			// HookDirectPlay(riid, (void *)ppv);
			// break;

#ifdef HOOKSHELL32
		case 0x00021401: 
			OutTraceDW("%s: CLSID_ShellLink class\n", ApiRef);
			switch (*(DWORD *)&riid){
				case 0x000214EE:
					OutTraceDW("%s: IID_IShellLinkA object\n", ApiRef);
					HookShellLinkA((void *)ppv);
					break;
			}
			break;
#endif // HOOKSHELL32

		// ??? this is IAMMultiMediaStream and not a class !!!
		case 0x49C47CE5: // 49C47CE5-9BA4-11D0-8212-00C04FC32C4: AMMultiMediaStream
			if((dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) || (dxw.dwFlags17 & HOOKDIRECTSHOW)){
				OutTraceDW("%s: AMMultiMediaStream class\n", ApiRef);
				switch (*(DWORD *)&riid){
					case 0x00000000: // I_Unknown
						OutTraceDW("%s: I_Unknown object\n", ApiRef);
						HookUnknown((void *)ppv);
						break;
					case 0xBEBE595C:
						OutTraceDW("%s: IID_IAMMultiMediaStream object\n", ApiRef);
						HookAMMultiMediaStream((void *)ppv);
						break;
				}
			}
			break;

		case 0xe436ebb2: // e436ebb2-524f-11ce-9f53-0020af0ba770 Filter Mapper
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_FilterMapper class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_FilterMapper)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		case 0xcda42200: // (0xcda42200, 0xbd88, 0x11d0, 0xbd, 0x4e, 0x0, 0xa0, 0xc9, 0x11, 0xce, 0x86) CLSID_FilterMapper2
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_FilterMapper2 class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_FilterMapper2)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		case 0x79376820: // {79376820-07D0-11CF-A24D-0020AFD79767}(CLSID_DSoundRender)
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_DSoundRender class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_DSoundRender)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		case 0xE436EBB3: // {E436EBB3-524F-11CE-9F53-0020AF0BA770}(CLSID_FilterGraph)
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_FilterGraph class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_FilterGraph)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		case 0xE436EBB5: // CLSID_AsyncReader class
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_AsyncReader class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_AsyncReader)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		case 0x99D54F63: // {99D54F63-1A69-41AE-AA4D-C976EB3F0713}(VMR Allocator Presenter)
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_AllocPresenter class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_AllocPresenter)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		case 0x6BC1CFFA: // {6BC1CFFA-8FC1-4261-AC22-CFB4CC38DB50}(CLSID_VideoRendererDefault)
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_VideoRendererDefault class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_VideoRendererDefault)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		case 0x4315D437: // {0x4315D437,0x5B8C,0x11d0,0xBD,0x3B,0x00,0xA0,0xC9,0x11,0xCE,0x86)
			if(dxw.dwFlags17 & HOOKDIRECTSHOW){
				OutTraceDW("%s: CLSID_CDeviceMoniker class\n", ApiRef);
				if(IsEqualGUID(rclsid, CLSID_CDeviceMoniker)) 
					HookInterfaceDS(ApiRef, NULL, riid, ppv);
			}
			break;

		default:
			// either here or above ... ?
			//HookOLEClass(rclsid);
			break;
	}
}

HRESULT STDAPICALLTYPE extCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv)
{
	HRESULT res;
	ApiName("CoCreateInstance");
#ifndef DXW_NOTRACES
	if (IsTraceCOM) {
		char sGUID1[80], sGUID2[80];
		strcpy_s(sGUID1, 80, sRIID(rclsid));
		strcpy_s(sGUID2, 80, sRIID(riid));
		OutTraceCOM("%s: rclsid=%s(%s) UnkOuter=%#x ClsContext=%#x refiid=%s(%s)\n",
			ApiRef, sGUID1, ExplainGUID((GUID *)&rclsid), 
			pUnkOuter, dwClsContext, 
			sGUID2, ExplainGUID((GUID *)&riid));
	}
#endif

	if(res = FilterGUID(ApiRef, rclsid, riid)) return res;

	bRecursedHook = TRUE;
	// v2.04.49: added try-except condition to handle the case where the pCoCreateInstance pointer read from IAT
	// is no longer valid when later used. It happens with "Gods - Lands of Infinity Special Edition" and could
	// be avoided also by using hot patching hooks, but this way is better.
	__try {
		res=(*pCoCreateInstance)(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
		void *prevhook = (void *)pCoCreateInstance;
		HMODULE hOle32 = (*pLoadLibraryA)("ole32.dll"); 
		pCoCreateInstance = (CoCreateInstance_Type)(*pGetProcAddress)(hOle32, "CoCreateInstance");
		OutTraceDW("%s: renewed hook ptr=%#x->%#x\n", ApiRef, prevhook, pCoCreateInstance);
		res=(*pCoCreateInstance)(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	}
	bRecursedHook = FALSE;
	if(res) {
		OutErrorCOM("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainCoCError(res));
		return res;
	}
	
	HookGUIDObject(ApiRef, rclsid, riid, ppv);

	OutTraceDW("%s: ppv=%#x->%#x\n", ApiRef, *ppv, *(DWORD *)*ppv);
	return S_OK;
}

HRESULT STDAPICALLTYPE extCoCreateInstanceEx(REFCLSID rclsid, IUnknown *punkOuter, DWORD dwClsCtx, COSERVERINFO *pServerInfo, DWORD dwCount, MULTI_QI *pResults)
{
	HRESULT res;
	ApiName("CoCreateInstanceEx");

	OutTraceDW("%s: rclsid=%s(%s) UnkOuter=%#x ClsContext=%#x Count=%d\n",
		ApiRef, sRIID(rclsid), ExplainGUID((GUID *)&rclsid), punkOuter, dwClsCtx, dwCount);
	for(DWORD i=0; i<dwCount; i++){
		IID riid=*pResults[i].pIID;
		OutTraceDW("> riid[%d]=%s\n", i, sRIID(riid));			
	}

	// weird error codes ....
	// 0x80012 	Not all the requested interfaces were available
	// 0x80013 	The specified machine name was not found in the cache.

	res=(*pCoCreateInstanceEx)(rclsid, punkOuter, dwClsCtx, pServerInfo, dwCount, pResults);
	if(res) {
		OutErrorCOM("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainCoCError(res));
		if (res != 0x80012) return res;
	}
	if(bRecursedHook) return res;

	for(DWORD i=0; i<dwCount; i++){
		LPVOID FAR* ppv;
		IID riid=*pResults[i].pIID;

		if(res = FilterGUID(ApiRef, rclsid, riid)) return 0x80012;

		if(pResults[i].hr) {
			HRESULT res = pResults[i].hr;
			OutErrorCOM("%s: SKIP riid[%d]=%s res=%#x(%s)\n", 
				ApiRef, i, sRIID(riid), res, ExplainCoCError(res));
		}
		else {
			ppv=(LPVOID *)pResults[i].pItf;
			OutTraceDW("%s: SCAN riid[%d]=%s ppv=%#x\n", 
				ApiRef, i, sRIID(riid), ppv);			
			HookGUIDObject(ApiRef, rclsid, riid, ppv);
		}
	}


	OutTraceCOM("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extCoInitialize(LPVOID pvReserved)
{
	HRESULT res;
	ApiName("CoInitialize");
	OutTraceCOM("%s: Reserved=%#x\n", ApiRef, pvReserved);
	// v2.04.74: suppress the potential cause of RPC_E_CHANGED_MODE error. Fixes "Rubik's Games" audio.
	if(dxw.dwFlags10 & NOOLEINITIALIZE) {
		OutTraceDW("%s: SUPPRESS\n", ApiRef);
		return S_OK;
	}
	res=(*pCoInitialize)(pvReserved);
	OutTraceCOM("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extOleInitialize(LPVOID pvReserved)
{
	HRESULT res;
	ApiName("OleInitialize");
	OutTraceCOM("%s: Reserved=%#x\n", ApiRef, pvReserved);
	// v2.04.74: suppress the potential cause of RPC_E_CHANGED_MODE error. Fixes "Rubik's Games" audio.
	if(dxw.dwFlags10 & NOOLEINITIALIZE) {
		OutTraceDW("%s: SUPPRESS\n", ApiRef);
		return S_OK;
	}
	res=(*pOleInitialize)(pvReserved);
	OutTraceCOM("%s: res=%#x\n", ApiRef, res);
	return res;
}

void WINAPI extCoUninitialize(void)
{
	ApiName("CoUninitialize");
	OutTraceCOM("%s\n", ApiRef);
	if(dxw.dwFlags10 & NOOLEINITIALIZE) {
		OutTraceDW("%s: SUPPRESS\n", ApiRef);
		return;
	}
	(*pCoUninitialize)();
}

HRESULT WINAPI extCoInitializeEx(LPVOID pvReserved, DWORD dwCoInit)
{
	HRESULT res;
	ApiName("CoInitializeEx");
	OutTraceCOM("%s: init=%#x\n", ApiRef, dwCoInit);
	if(dxw.dwFlags10 & NOOLEINITIALIZE) {
		OutTraceDW("%s: SUPPRESS\n", ApiRef);
		return S_OK;
	}
	res = (*pCoInitializeEx)(pvReserved, dwCoInit);
	OutTraceCOM("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extCoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, LPVOID pvReserved, REFIID riid, LPVOID FAR* ppv)
{
	HRESULT res;
	ApiName("CoGetClassObject");
#ifndef DXW_NOTRACES
	if (IsTraceCOM) {
		char sGUID1[80], sGUID2[80];
		strcpy_s(sGUID1, 80, sRIID(rclsid));
		strcpy_s(sGUID2, 80, sRIID(riid));
		OutTrace("%s: rclsid=%s(%s) context=%#x reserved=%#x refiid=%s(%s) ppv=%#x\n", 
			ApiRef, 
			sGUID1, ExplainGUID((GUID *)&rclsid), 
			dwClsContext, 
			pvReserved, 
			sGUID2, ExplainGUID((GUID *)&riid), 
			ppv);
	}
#endif
	if(res = FilterGUID(ApiRef, rclsid, riid)) return res;

	res = (*pCoGetClassObject)(rclsid, dwClsContext, pvReserved, riid, ppv);
	
	if(!res) HookGUIDObject(ApiRef, rclsid, riid, ppv);

	OutTraceCOM("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extLoadTypeLibEx(LPCOLESTR szFile, REGKIND regkind, ITypeLib **pptlib)
{
	HRESULT res;
	ApiName("LoadTypeLibEx");
	OutTraceCOM("%s: file=\"%s\" regkind=%#x\n", ApiRef, szFile, regkind);
	res = (*pLoadTypeLibEx)(szFile, regkind, pptlib);
	OutTraceCOM("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extCreateInstance(LPVOID obj, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
	HRESULT res;
	ApiName("CreateInstance");
#ifndef DXW_NOTRACES
	if (IsTraceDW) {
		char sGUID[80];
		strcpy_s(sGUID, 80, sRIID(riid));
		OutTraceCOM("%s: obj=%#x UnkOuter=%#x refiid=%s(%s)\n",
			ApiRef, obj, pUnkOuter, sGUID, ExplainGUID((GUID *)&riid));
	}
#endif
	//if(res = FilterGUID(ApiRef, rclsid, riid)) return res;

	bRecursedHook = TRUE;
	res=(*pCreateInstance)(obj, pUnkOuter, riid, ppv);
	bRecursedHook = FALSE;

	if(res) {
		OutErrorCOM("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainCoCError(res));
		return res;
	}
	
	if(ppv) {
		HookGUIDObject(ApiRef, (const IID &)pUnkOuter, riid, ppv);
		OutTraceDW("%s: ppv=%#x->%#x\n", ApiRef, *ppv, *(DWORD *)*ppv);
	}

	return S_OK;
}

