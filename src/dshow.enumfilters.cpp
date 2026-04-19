#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IEnumFilters"
//#define BYPASS

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

void dxwEnumFilters::Hook(IEnumFilters *obj)
{
	//OutTraceDSHOW("dxwEnumFilters.Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwEnumFilters::QueryInterface(const IID &riid, void **obp)
{
#ifdef BYPASS
{ return m_this->QueryInterface(riid, obp); }
#else
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
#endif
}

ULONG STDMETHODCALLTYPE dxwEnumFilters::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwEnumFilters::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwEnumFilters();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwEnumFilters::Next(ULONG cFilters, IBaseFilter **ppFilter, ULONG *pcFetched)
{
#ifdef BYPASS
{ return m_this->Next(cFilters, ppFilter, pcFetched); }
#else
	ApiName("Next");
	HRESULT res;
	//OutTraceDSHOW("%s: obj=%#x cFilters=%d ppFilter=%#x pcFetched=%#x\n", ApiRef, m_this, cFilters, ppFilter, pcFetched);
	res = m_this->Next(cFilters, ppFilter, pcFetched);
	if(res){
		if(res == S_FALSE){
			OutTraceDSHOW("%s: obj=%#x cFilters=%d STOP\n", ApiRef, m_this, cFilters, res);
		}
		else {
			OutTraceDSHOW("%s: obj=%#x cFilters=%d ERROR res=%#x\n", ApiRef, m_this, cFilters, res);
		}
	}
	else {
		if(ppFilter && *ppFilter) {
			// v2.06.12 fix: the pGraph object must be released after usage!
			FILTER_INFO fInfo;
			(*ppFilter)->QueryFilterInfo(&fInfo);
			OutTraceDSHOW("%s: res=OK pFilter=%#x fetched=%d filter=%#x \"%ls\"\n", 
				ApiRef, *ppFilter, pcFetched ? *pcFetched : 0, *ppFilter, fInfo.achName);
			fInfo.pGraph->Release();
		}
	}
	return res;
#endif
}

HRESULT STDMETHODCALLTYPE dxwEnumFilters::Skip(ULONG cFilters)
{
	ApiName("Skip");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x cFilters=%d\n", ApiRef, m_this, cFilters);
	res = m_this->Skip(cFilters);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwEnumFilters::Reset()
{
	ApiName("Reset");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->Reset();
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwEnumFilters::Clone(IEnumFilters **ppEnum)
{
	ApiName("Clone");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->Clone(ppEnum);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

