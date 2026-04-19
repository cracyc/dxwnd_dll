#define _CRT_SECURE_NO_WARNINGS
#define INITGUID
//#define FULLHEXDUMP
#define _MODULE "d3dxof"

#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "dxhook.h"
#include "dxwcore.hpp"
#include "stdio.h" 
#include "dxhelper.h"
#include "syslibs.h"
#include "DXFile.h"

typedef HRESULT (WINAPI *DirectXFileCreate_Type)(LPDIRECTXFILE *);
DirectXFileCreate_Type pDirectXFileCreate;
HRESULT WINAPI extDirectXFileCreate(LPDIRECTXFILE *);

static HookEntryEx_Type HooksD3dxof[]={
	{HOOK_IAT_CANDIDATE, 0, "DirectXFileCreate", (FARPROC)NULL, (FARPROC *)&pDirectXFileCreate, (FARPROC)extDirectXFileCreate},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookD3dxof(HMODULE hModule)
{
	HookLibraryEx(hModule, HooksD3dxof, "D3dxof.dll");
}

FARPROC Remap_D3dxof_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, HooksD3dxof)) return addr;
	return NULL;
}

void HookDirectXFile(LPDIRECTXFILE *);
void HookDirectXFileEnumObject(LPDIRECTXFILEENUMOBJECT *);
void HookDirectXFileData(LPDIRECTXFILEDATA *);

HRESULT WINAPI extDirectXFileCreate(LPDIRECTXFILE *lplpDirectXFile)
{
	ApiName("DirectXFileCreate");
	HRESULT res;

	res = (*pDirectXFileCreate)(lplpDirectXFile);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: dxfile=%#x\n", ApiRef, *lplpDirectXFile);
		HookDirectXFile(lplpDirectXFile);
	}
	return res;
}

static char *sLoadFrom(DXFILELOADOPTIONS opt)
{
	switch(opt){
		case DXFILELOAD_FROMFILE: return "FILE";
		case DXFILELOAD_FROMRESOURCE: return "RESOURCE";
		case DXFILELOAD_FROMMEMORY: return "MEMORY";
		case DXFILELOAD_FROMSTREAM: return "STREAM";
		case DXFILELOAD_FROMURL: return "URL";
	}
	return "???";
}

typedef HRESULT (WINAPI *CreateEnumObject_Type) (LPVOID, LPVOID, DXFILELOADOPTIONS, LPDIRECTXFILEENUMOBJECT *);
CreateEnumObject_Type pCreateEnumObject;
HRESULT WINAPI extCreateEnumObject(LPVOID obj, LPVOID pvSource, DXFILELOADOPTIONS dwLoadOptions, LPDIRECTXFILEENUMOBJECT *ppEnumObj)
{
	ApiName("CreateEnumObject");
	HRESULT res;
	OutTrace("%s: obj=%#x source=%#x loadptions=%#x(%s)\n", ApiRef, obj, pvSource, dwLoadOptions, sLoadFrom(dwLoadOptions));

	res = (*pCreateEnumObject)(obj, pvSource, dwLoadOptions, ppEnumObj);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: EnumObj=%#x\n", ApiRef, *ppEnumObj);
		HookDirectXFileEnumObject(ppEnumObj);
	}
	return res;
}

static char *sFileFormat(DXFILEFORMAT ff)
{
	switch(ff){
		case DXFILEFORMAT_BINARY: return "BINARY";
		case DXFILEFORMAT_TEXT: return "TEXT";
		case DXFILEFORMAT_COMPRESSED: return "COMPRESSED";
	}
	return "???";
}

typedef HRESULT (WINAPI *CreateSaveObject_Type) (LPVOID, LPCSTR, DXFILEFORMAT, LPDIRECTXFILESAVEOBJECT *);
CreateSaveObject_Type pCreateSaveObject;
HRESULT WINAPI extCreateSaveObject(LPVOID obj, LPCSTR szFileName, DXFILEFORMAT dwFileFormat, LPDIRECTXFILESAVEOBJECT *p3)
{
	ApiName("CreateSaveObject");
	HRESULT res;
	OutTrace("%s: obj=%#x fname=\"%s\" format=%#x(%s)\n", ApiRef, obj, szFileName, dwFileFormat, sFileFormat(dwFileFormat));

	res = (*pCreateSaveObject)(obj, szFileName, dwFileFormat, p3);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

typedef HRESULT (WINAPI *RegisterTemplates_Type) (LPVOID, LPVOID, DWORD);
RegisterTemplates_Type pRegisterTemplates;
HRESULT WINAPI extRegisterTemplates(LPVOID obj, LPVOID pvData, DWORD cbSize)
{
	ApiName("RegisterTemplates");
	HRESULT res;
	OutTrace("%s: obj=%#x data=%#x size=%d\n", ApiRef, obj, pvData, cbSize);

	res = (*pRegisterTemplates)(obj, pvData, cbSize);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

void HookDirectXFile(LPDIRECTXFILE *obj)
{
	ApiName("HookDirectXFile");
	// SetHook((void *)(**(DWORD **)obj +  0), extQueryInterface, (void **)&pQueryInterface, "QueryInterface");
	// 4: AddRef
	// 8: Release
	SetHook((void *)(**(DWORD **)obj + 12), extCreateEnumObject, (void **)&pCreateEnumObject, "CreateEnumObject");
	SetHook((void *)(**(DWORD **)obj + 16), extCreateSaveObject, (void **)&pCreateSaveObject, "CreateSaveObject");
	SetHook((void *)(**(DWORD **)obj + 20), extRegisterTemplates, (void **)&pRegisterTemplates, "RegisterTemplates");
}	

typedef HRESULT (WINAPI *GetNextDataObject_Type)(LPVOID, LPDIRECTXFILEDATA *);
GetNextDataObject_Type pGetNextDataObject;
HRESULT WINAPI extGetNextDataObject(LPVOID obj, LPDIRECTXFILEDATA *ppDataObj)
{
	ApiName("GetNextDataObject");
	HRESULT res;
	OutTrace("%s: obj=%#x\n", ApiRef, obj);

	res = (*pGetNextDataObject)(obj, ppDataObj);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: DataObj=%#x\n", ApiRef, *ppDataObj);
		HookDirectXFileData(ppDataObj);
	}
	return res;
}


typedef HRESULT (WINAPI *GetDataObjectById_Type)(LPVOID, REFGUID, LPDIRECTXFILEDATA *);
GetDataObjectById_Type pGetDataObjectById;
HRESULT WINAPI extGetDataObjectById(LPVOID obj, REFGUID rguid, LPDIRECTXFILEDATA *ppDataObj)
{
	ApiName("GetDataObjectById");
	HRESULT res;
	OutTrace("%s: obj=%#x guid=\"%s\"\n", ApiRef, obj, sGUID(&(GUID)rguid));

	res = (*pGetDataObjectById)(obj, rguid, ppDataObj);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: DataObj=%#x\n", ApiRef, *ppDataObj);
		HookDirectXFileData(ppDataObj);
	}
	return res;
}

typedef HRESULT (WINAPI *GetDataObjectByName_Type)(LPVOID, LPCSTR, LPDIRECTXFILEDATA *);
GetDataObjectByName_Type pGetDataObjectByName;
HRESULT WINAPI extGetDataObjectByName(LPVOID obj, LPCSTR szName, LPDIRECTXFILEDATA *ppDataObj)
{
	ApiName("GetDataObjectByName");
	HRESULT res;
	OutTrace("%s: obj=%#x name=\"%s\"\n", ApiRef, obj, szName);

	res = (*pGetDataObjectByName)(obj, szName, ppDataObj);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: DataObj=%#x\n", ApiRef, *ppDataObj);
		HookDirectXFileData(ppDataObj);
	}
	return res;
}

void HookDirectXFileEnumObject(LPDIRECTXFILEENUMOBJECT *obj)
{
	ApiName("HookDirectXFileEnumObject");
	// SetHook((void *)(**(DWORD **)obj +  0), extQueryInterface, (void **)&pQueryInterface, "QueryInterface");
	// 4: AddRef
	// 8: Release
	SetHook((void *)(**(DWORD **)obj + 12), extGetNextDataObject, (void **)&pGetNextDataObject, "GetNextDataObject");
	SetHook((void *)(**(DWORD **)obj + 16), extGetDataObjectById, (void **)&pGetDataObjectById, "GetDataObjectById");
	SetHook((void *)(**(DWORD **)obj + 20), extGetDataObjectByName, (void **)&pGetDataObjectByName, "GetDataObjectByName");
}

typedef HRESULT (WINAPI *GetData_Type)(LPVOID, LPCSTR, DWORD *, LPVOID *);
GetData_Type pGetData;
HRESULT WINAPI extGetData(LPVOID obj, LPCSTR szMember, DWORD  *pcbSize, LPVOID *ppvData)
{
	ApiName("GetData");
	HRESULT res;
	OutTrace("%s: obj=%#x member=\"%s\"\n", ApiRef, obj, szMember);

	res = (*pGetData)(obj, szMember, pcbSize, ppvData);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: size=%d Data=%#x\n", ApiRef, *pcbSize, *ppvData);
	}
	return res;
}

typedef HRESULT (WINAPI *GetType_Type)(LPVOID, const GUID **);
GetType_Type pGetType;
HRESULT WINAPI extGetType(LPVOID obj, const GUID **ppguid)
{
	ApiName("GetType");
	HRESULT res;
	OutTrace("%s: obj=%#x\n", ApiRef, obj);

	res = (*pGetType)(obj, ppguid);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: guid=%s\n", ApiRef, sGUID((GUID *)*ppguid));
	}
	return res;
}

typedef HRESULT (WINAPI *GetNextObject_Type)(LPVOID, LPDIRECTXFILEOBJECT *);
GetNextObject_Type pGetNextObject;
HRESULT WINAPI extGetNextObject(LPVOID obj, LPDIRECTXFILEOBJECT *ppChildObj)
{
	ApiName("GetNextObject");
	HRESULT res;
	OutTrace("%s: obj=%#x\n", ApiRef, obj);

	res = (*pGetNextObject)(obj, ppChildObj);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTrace("%s: child=%#x\n", ApiRef, *ppChildObj);
	}
	return res;
}

typedef HRESULT (WINAPI *AddDataObject_Type)(LPVOID, LPDIRECTXFILEDATA);
AddDataObject_Type pAddDataObject;
HRESULT WINAPI extAddDataObject(LPVOID obj, LPDIRECTXFILEDATA pDataObj)
{
	ApiName("AddDataObject");
	HRESULT res;
	OutTrace("%s: obj=%#x data=%#x\n", ApiRef, obj, pDataObj);

	res = (*pAddDataObject)(obj, pDataObj);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

typedef HRESULT (WINAPI *AddDataReference_Type)(LPVOID, LPCSTR, const GUID *);
AddDataReference_Type pAddDataReference;
HRESULT WINAPI extAddDataReference(LPVOID obj, LPCSTR szRef, const GUID *pguidRef)
{
	ApiName("AddDataReference");
	HRESULT res;
	OutTrace("%s: obj=%#x ref=\"%s\" guid=%s\n", ApiRef, obj, szRef, sGUID((GUID *)pguidRef));

	res = (*pAddDataReference)(obj, szRef, pguidRef);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

typedef HRESULT (WINAPI *AddBinaryObject_Type)(LPVOID, LPCSTR, const GUID *, LPCSTR, LPVOID, DWORD);
AddBinaryObject_Type pAddBinaryObject;
HRESULT WINAPI extAddBinaryObject(LPVOID obj, LPCSTR szName, const GUID *pguid, LPCSTR szMimeType, LPVOID pvData, DWORD cbSize)
{
	ApiName("AddBinaryObject");
	HRESULT res;
	OutTrace("%s: obj=%#x name=\"%s\" guid=%s mime=%s data=%#x size=%s\n", 
		ApiRef, obj, szName, sGUID((GUID *)pguid), szMimeType, pvData, cbSize);

	res = (*pAddBinaryObject)(obj, szName, pguid, szMimeType, pvData, cbSize);

	if(res){
		OutTrace("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

void HookDirectXFileData(LPDIRECTXFILEDATA *obj)
{
	ApiName("HookDirectXFileData");
	// SetHook((void *)(**(DWORD **)obj +  0), extQueryInterface, (void **)&pQueryInterface, "QueryInterface");
	// 4: AddRef
	// 8: Release
	//SetHook((void *)(**(DWORD **)obj + 12), extGetName, (void **)&pGetName, "GetName");
	//SetHook((void *)(**(DWORD **)obj + 16), extGetId, (void **)&pGetId, "GetId");
	SetHook((void *)(**(DWORD **)obj + 20), extGetData, (void **)&pGetData, "GetData");
	SetHook((void *)(**(DWORD **)obj + 24), extGetType, (void **)&pGetType, "GetType");
	SetHook((void *)(**(DWORD **)obj + 28), extGetNextObject, (void **)&pGetNextObject, "GetNextObject");
	SetHook((void *)(**(DWORD **)obj + 32), extAddDataObject, (void **)&pAddDataObject, "AddDataObject");
	SetHook((void *)(**(DWORD **)obj + 36), extAddDataReference, (void **)&pAddDataReference, "AddDataReference");
	SetHook((void *)(**(DWORD **)obj + 40), extAddBinaryObject, (void **)&pAddBinaryObject, "AddBinaryObject");
}
