#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IFilterGraph"

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

#define DSHOWHACKS

void dxwFilterGraph::Hook(IFilterGraph *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwFilterGraph::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwFilterGraph::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwFilterGraph();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::AddFilter(IBaseFilter *pFilter, LPCWSTR pName)
{
	ApiName("AddFilter");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x filter=%#x(%ls)\n", ApiRef, m_this, pFilter, pName); 

    if(dxw.dwFlags18 & IGNOREADDFILTER){
        OutTraceDSHOW("%s: Ignoring AddFilter\n", ApiRef);
		return S_OK;
    }

	res = m_this->AddFilter(pFilter, pName);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::RemoveFilter(IBaseFilter *pFilter)
{
	ApiName("RemoveFilter");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x filter=%#x\n", ApiRef, m_this, pFilter); 

	res = m_this->RemoveFilter(pFilter);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::EnumFilters(IEnumFilters **ppEnum)
{
	ApiName("EnumFilters");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x enum=%#x\n", ApiRef, m_this, ppEnum); 

	res = m_this->EnumFilters(ppEnum);

	if(res == S_OK){
		OutTraceDSHOW("%s: ppEnum=%#x\n", ApiRef, *ppEnum);
		dxwEnumFilters *filter = new(dxwEnumFilters);
		filter->Hook(*ppEnum);
		*ppEnum = filter;
	}

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::FindFilterByName(LPCWSTR pName, IBaseFilter **ppFilter)
{
	ApiName("FindFilterByName");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x name=%ls\n", ApiRef, m_this, pName); 

	res = m_this->FindFilterByName(pName, ppFilter);
	if(res == S_OK){
		// v2.06.12 fix: the pGraph object must be released after usage!
		FILTER_INFO fInfo;
		(*ppFilter)->QueryFilterInfo(&fInfo);
		OutTrace("%s: filter=%#x %ls\n", ApiRef, *ppFilter, fInfo.achName);
		fInfo.pGraph->Release();
	}

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::ConnectDirect(IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt)
{
	ApiName("ConnectDirect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pinOut=%#x pinIn=%#x\n", ApiRef, m_this, ppinOut, ppinIn); 

	res = m_this->ConnectDirect(ppinOut, ppinIn, pmt);
	if((res == S_OK) && IsTraceDSHOW){
		OutTraceDSHOW("> majortype=%s\n", sGUID((GUID *)&(pmt->majortype)));
		OutTraceDSHOW("> subtype=%s\n", sGUID((GUID *)&(pmt->subtype)));
		OutTraceDSHOW("> bFixedSizeSamples=%#x\n", pmt->bFixedSizeSamples);
		OutTraceDSHOW("> bTemporalCompression=%#x\n", pmt->bTemporalCompression);
		OutTraceDSHOW("> lSampleSize=%d\n", pmt->lSampleSize);
		OutTraceDSHOW("> formattype=%s\n", pmt->formattype);
		OutTraceDSHOW("> cbFormat=%d\n", pmt->cbFormat);
		//     BYTE *pbFormat;

	}

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::Reconnect(IPin *ppin)
{
	ApiName("Reconnect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pin=%ls\n", ApiRef, m_this, ppin); 

	res = m_this->Reconnect(ppin);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::Disconnect(IPin *ppin)
{
	ApiName("Disconnect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pin=%ls\n", ApiRef, m_this, ppin); 

	res = m_this->Disconnect(ppin);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph::SetDefaultSyncSource(void)
{
	ApiName("SetDefaultSyncSource");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this); 

	res = m_this->SetDefaultSyncSource();

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}
        
