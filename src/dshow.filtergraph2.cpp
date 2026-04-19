#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IFilterGraph2"

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

void dxwFilterGraph2::Hook(IFilterGraph2 *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwFilterGraph2::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwFilterGraph2::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwFilterGraph2();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::AddFilter(IBaseFilter *pFilter, LPCWSTR pName)
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

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::RemoveFilter(IBaseFilter *pFilter)
{
	ApiName("RemoveFilter");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x filter=%#x\n", ApiRef, m_this, pFilter); 

	res = m_this->RemoveFilter(pFilter);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::EnumFilters(IEnumFilters **ppEnum)
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

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::FindFilterByName(LPCWSTR pName, IBaseFilter **ppFilter)
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

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::ConnectDirect(IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt)
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

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::Reconnect(IPin *ppin)
{
	ApiName("Reconnect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pin=%ls\n", ApiRef, m_this, ppin); 

	res = m_this->Reconnect(ppin);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::Disconnect(IPin *ppin)
{
	ApiName("Disconnect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pin=%ls\n", ApiRef, m_this, ppin); 

	res = m_this->Disconnect(ppin);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::SetDefaultSyncSource(void)
{
	ApiName("SetDefaultSyncSource");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this); 

	res = m_this->SetDefaultSyncSource();

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res); 
	return res;
}
        
HRESULT STDMETHODCALLTYPE dxwFilterGraph2::Connect(IPin *ppinOut, IPin *ppinIn)
#ifdef BYPASS
{ return m_this->Connect(ppinOut, ppinIn); }
#else
{
	HRESULT res;
	ApiName("Connect");
	OutTraceDSHOW("%s: pinOut=%#x pinIn=%#x\n", ApiRef, ppinOut, ppinIn);
	res =  m_this->Connect(ppinOut, ppinIn);
	OutTraceDSHOW("%s: res=%#x(%s)\n", ApiRef, res, res ? "ERROR" : "OK");
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::Render(IPin *ppinOut)
#ifdef BYPASS
{ return m_this->Render(ppinOut); }
#else
{
	HRESULT res;
	ApiName("Render");
	OutTraceDSHOW("%s: pinOut=%#x\n", ApiRef, ppinOut);
	res =  m_this->Render(ppinOut);
	OutTraceDSHOW("%s: res=%#x(%s)\n", ApiRef, res, res ? "ERROR" : "OK");
	if(dxw.dwDFlags & DUMPDSHOWGRAPH) GraphDump((IGraphBuilder*)m_this);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
#ifdef BYPASS
{ return m_this->RenderFile(lpcwstrFile, lpcwstrPlayList); }
#else
{
	HRESULT res;
	ApiName("RenderFile");
	OutTraceDSHOW("%s: obj=%#x path=%ls PlayList=%ls\n", ApiRef, m_this, 
		lpcwstrFile ? lpcwstrFile : L"",
		lpcwstrPlayList ? lpcwstrPlayList : L"");

	if(dxw.dwFlags6 & NOMOVIES) {
		// v2.06.05: curiously, a simple "return 0" won't do, it would cause an error.
		// instead, a replacement with an invalid path works better.
		// in @#@ "Crashday" the error is bypassed and the game goes to the main menu screen.
		lpcwstrFile = L"";
	}
	else
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		DWORD mapping;
		lpcwstrFile = dxwTranslatePathW(lpcwstrFile, &mapping);
		if(mapping != DXW_NO_FAKE){
			OutTraceDSHOW("%s: new path=%ls\n", ApiRef, lpcwstrFile);
		}
	}

	res = m_this->RenderFile(lpcwstrFile, lpcwstrPlayList);
	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	if(dxw.dwDFlags & DUMPDSHOWGRAPH) GraphDump((IGraphBuilder*)m_this);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter)
#ifdef BYPASS
{ return m_this->AddSourceFilter(lpcwstrFileName, lpcwstrFilterName, ppFilter); }
#else
{
	HRESULT res;
	ApiName("AddSourceFilter");
	OutTraceDSHOW("%s: filename=\"%ls\" filtername=\"%ls\"pinOut=%#x\n", ApiRef, lpcwstrFilterName);
	res =  m_this->AddSourceFilter(lpcwstrFileName, lpcwstrFilterName, ppFilter);
	OutTraceDSHOW("%s: res=%#x(%s)\n", ApiRef, res, res ? "ERROR" : "OK");
	if(res == 0) HookBaseFilter(ppFilter);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::SetLogFile(DWORD_PTR hFile)
{ return m_this->SetLogFile(hFile); }

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::Abort(void)
#ifdef BYPASS
{ return m_this->Abort(); }
#else
{
	HRESULT res;
	ApiName("Abort");
	OutTraceDSHOW("%s\n", ApiRef);
	res =  m_this->Abort();
	OutTraceDSHOW("%s: res=%#x(%s)\n", ApiRef, res, res ? "ERROR" : "OK");
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::ShouldOperationContinue(void)
#ifdef BYPASS
{ return m_this->ShouldOperationContinue(); }
#else
{
	HRESULT res;
	ApiName("ShouldOperationContinue");
	OutTraceDSHOW("%s\n", ApiRef);
	res =  m_this->ShouldOperationContinue();
	OutTraceDSHOW("%s: res=%#x(%s)\n", ApiRef, res, res ? "ERROR" : "OK");
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::AddSourceFilterForMoniker(IMoniker *pMoniker, IBindCtx *pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter)
{ return m_this->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter); }

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::ReconnectEx(IPin *ppin, const AM_MEDIA_TYPE *pmt)
{ return m_this->ReconnectEx(ppin, pmt); }

HRESULT STDMETHODCALLTYPE dxwFilterGraph2::RenderEx(IPin *pPinOut, DWORD dwFlags, DWORD *pvContext)
#ifdef BYPASS
{ return m_this->RenderEx(pPinOut, dwFlags, pvContext); }
#else
{
	HRESULT res;
	ApiName("RenderEx");
	OutTraceDSHOW("%s: pinOut=%#x flags=%#x context=%#x\n", ApiRef, pPinOut, dwFlags, pvContext);
	res =  m_this->RenderEx(pPinOut, dwFlags, pvContext);
	OutTraceDSHOW("%s: res=%#x(%s)\n", ApiRef, res, res ? "ERROR" : "OK");
	if(dxw.dwDFlags & DUMPDSHOWGRAPH) GraphDump((IGraphBuilder*)m_this);
	return res;
}
#endif

