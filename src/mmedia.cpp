#define _COMPONENT "IAMMultiMediaStream"

#include <dxdiag.h>
#include <dsound.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include <mmstream.h>
#include <amstream.h>

typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID, LPVOID *);
QueryInterface_Type pQueryInterfaceMMS;
HRESULT WINAPI extQueryInterfaceMMS(void *, REFIID, LPVOID *);
typedef HRESULT (WINAPI *MMOpenFile_Type)(void *, LPCWSTR, DWORD);
MMOpenFile_Type pMMOpenFile;
HRESULT WINAPI extMMOpenFile(void *, LPCWSTR, DWORD);

void HookAMMultiMediaStream(LPVOID obj)
{
	OutTrace("dxwnd.HookAMMediaStream\n");
	SetHook((void *)(**(DWORD **)obj +  0), extQueryInterfaceMMS, (void **)&pQueryInterfaceMMS, "QueryInterface");
	SetHook((void *)(**(DWORD **)obj +  64), extMMOpenFile, (void **)&pMMOpenFile, "OpenFile");
}

HRESULT WINAPI extQueryInterfaceMMS(void *obj, REFIID riid, LPVOID *obp)
{ 
	HRESULT res;
	ApiName("QueryInterface");
	OutTraceDSHOW("%s: obj=%#x REFIID=%s(%s)\n", ApiRef, obj, sRIID(riid), ExplainGUID((GUID *)&riid));
	res = (*pQueryInterfaceMMS)(obj, riid, obp);
	if(res) {
		OutErrorCOM("%s: ERROR ret=%#x\n", ApiRef, res);
	}
	else {
		OutTraceCOM("%s: OK obp=%#x ret=%#x\n", ApiRef, *obp);
	}
	return res;
}

HRESULT WINAPI extMMOpenFile(void *obj, LPCWSTR path, DWORD flags)
{
	HRESULT res;
	ApiName("OpenFile");
	OutTrace("%s: obj=%#x flags=%#x path=%ls\n", ApiRef, obj, flags, path);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		path = dxwTranslatePathW(path, NULL);
		OutTrace("%s: new path=%ls\n", ApiRef, path);
	}
	res = (*pMMOpenFile)(obj, path, flags);
	OutTrace("%s: res=%#x\n", ApiRef, res);
	return res;
}

