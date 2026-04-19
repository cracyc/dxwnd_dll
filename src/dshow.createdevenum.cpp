#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "ICreateDevEnum"

#include <dxdiag.h>
#include <dsound.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include <strmif.h> // includes "dshow.h"
#include <control.h> // includes "dshow.h"
#include <dshow.h>
#include <strmif.h>
#include <comdef.h> // for BSTR type manipulation
#include <evr.h> // for video control

#include "dshow.wrap.hpp"

void dxwCreateDevEnum::Hook(ICreateDevEnum *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwCreateDevEnum::QueryInterface(const IID &riid, void **obp)
{
	ApiName("QueryInterface");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x REFIID=%s(%s)\n", ApiRef, m_this, sRIID(riid), ExplainGUID((GUID *)&riid));
	if(res = CheckInterfaceDS(ApiRef, riid)) return res;
	res = m_this->QueryInterface(riid, obp);
	if(res) {
		OutTraceDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTraceDSHOW("%s: OK obp=%#x\n", ApiRef, *obp);
		HookInterfaceDS(ApiRef, m_this, riid, obp);
	}
	return res;
}

ULONG STDMETHODCALLTYPE dxwCreateDevEnum::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwCreateDevEnum::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwCreateDevEnum();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwCreateDevEnum::CreateClassEnumerator(REFCLSID clsidDeviceClass, IEnumMoniker **ppEnumMoniker, DWORD dwFlags)
{
	ApiName("CreateClassEnumerator");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x clsid=%s flags=%#x\n", ApiRef, m_this, sRIID(clsidDeviceClass), ppEnumMoniker, dwFlags);
	res = m_this->CreateClassEnumerator(clsidDeviceClass, ppEnumMoniker, dwFlags);
#ifdef TOBEDONE
	if(res == 0){
		OutTraceDSHOW("%s: pEnumMoniker=%#x\n", ApiRef, *ppEnumMoniker);
		dxwEnumMoniker *moniker = new(dxwEnumMoniker);
		moniker->Hook(*ppEnumMoniker);
		*ppEnumMoniker = moniker;
	}
#endif
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}
