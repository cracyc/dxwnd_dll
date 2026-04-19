#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IEnumPins"

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

void dxwEnumPins::Hook(IEnumPins *obj)
{
	//OutTraceDSHOW("dxwEnumPins.Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwEnumPins::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwEnumPins::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwEnumPins::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwEnumPins();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwEnumPins::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
	ApiName("Next");
	HRESULT res;
	//OutTraceDSHOW("%s: obj=%#x cPins=%d\n", ApiRef, m_this, cPins);
	res = m_this->Next(cPins, ppPins, pcFetched);
	if(res){
		if(res == S_FALSE){
			OutTraceDSHOW("%s: obj=%#x pins=%d STOP\n", ApiRef, m_this, cPins, res);
		}
		else {
			OutTraceDSHOW("%s: obj=%#x pins=%d ERROR res=%#x\n", ApiRef, m_this, cPins, res);
		}
	}
	else {
		if(ppPins && *ppPins){
			// v2.06.12 fix: the pFilter object must be released after usage!
			PIN_INFO pInfo;
			(*ppPins)->QueryPinInfo(&pInfo);
			OutTraceDSHOW("%s: res=OK pPin=%#x fetched=%d pin=%#x dir=%d \"%ls\"\n", 
				ApiRef, *ppPins, pcFetched ? *pcFetched : 0, *ppPins, pInfo.dir, pInfo.achName);
			pInfo.pFilter->Release();
		}
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwEnumPins::Skip(ULONG cFilters)
{
	ApiName("Skip");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x cFilters=%d\n", ApiRef, m_this, cFilters);
	res = m_this->Skip(cFilters);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwEnumPins::Reset()
{
	ApiName("Reset");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->Reset();
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwEnumPins::Clone(IEnumPins **ppEnum)
{
	ApiName("Clone");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->Clone(ppEnum);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

