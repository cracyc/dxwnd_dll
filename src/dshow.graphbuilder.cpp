#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IGraphBuilder"

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

//#define DSHOWHACKS
//#define BYPASS 

void dxwGraphBuilder::Hook(IGraphBuilder *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwGraphBuilder::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwGraphBuilder::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwGraphBuilder();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::AddFilter(IBaseFilter *pFilter, LPCWSTR pName)
#ifdef BYPASS
{ return m_this->AddFilter(pFilter, pName); }
#else
{
	ApiName("AddFilter");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x filter=%#x(%ls)\n", ApiRef, m_this, pFilter, pName); 

    if(dxw.dwFlags18 & IGNOREADDFILTER){
        OutTraceDSHOW("%s: Ignoring AddFilter\n", ApiRef);
		return S_OK;
    }

	res = m_this->AddFilter(pFilter, pName);

	if(res && (dxw.dwFlags18 & IGNOREGRAPHERRORS)){
		OutTraceDSHOW("%s: obj=%#x IGNORING ERROR res=%#x\n", ApiRef, m_this, res); 
		res = 0;
	}
	if(res) {
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x filter=%#x\n", ApiRef, m_this, *pFilter);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::RemoveFilter(IBaseFilter *pFilter)
{
#ifdef BYPASS
{ return m_this->RemoveFilter(pFilter); }
#else
	ApiName("RemoveFilter");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x filter=%#x\n", ApiRef, m_this, pFilter); 

	res = m_this->RemoveFilter(pFilter);

	if(res && (dxw.dwFlags18 & IGNOREGRAPHERRORS)){
		OutTraceDSHOW("%s: obj=%#x IGNORING ERROR res=%#x\n", ApiRef, m_this, res); 
		res = 0;
	}
	if(res) {
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x\n", ApiRef, m_this);
	}
	return res;
#endif
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::EnumFilters(IEnumFilters **ppEnum)
{
#ifdef BYPASS
{ return m_this->EnumFilters(ppEnum); }
#else
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
#endif
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::FindFilterByName(LPCWSTR pName, IBaseFilter **ppFilter)
{
	ApiName("FindFilterByName");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x name=%ls\n", ApiRef, m_this, pName); 

	res = m_this->FindFilterByName(pName, ppFilter);
	if(res == S_OK){
		// v2.06.12 fix: the pGraph object must be released after usage!
		FILTER_INFO fInfo;
		(*ppFilter)->QueryFilterInfo(&fInfo);
		OutTrace("%s: OK obj=%#x filter=%#x %ls\n", ApiRef, m_this, *ppFilter, fInfo.achName);
		fInfo.pGraph->Release();
	}
	else {
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res); 
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::ConnectDirect(IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt)
{
	ApiName("ConnectDirect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pinOut=%#x pinIn=%#x\n", ApiRef, m_this, ppinOut, ppinIn); 

	res = m_this->ConnectDirect(ppinOut, ppinIn, pmt);
	if(res == S_OK){
		OutTraceDSHOW("%s: obj+%#x OK\n", ApiRef, m_this); 
		if(IsTraceDSHOW){
			OutTrace("> majortype=%s\n", sGUID((GUID *)&(pmt->majortype)));
			OutTrace("> subtype=%s\n", sGUID((GUID *)&(pmt->subtype)));
			OutTrace("> bFixedSizeSamples=%#x\n", pmt->bFixedSizeSamples);
			OutTrace("> bTemporalCompression=%#x\n", pmt->bTemporalCompression);
			OutTrace("> lSampleSize=%d\n", pmt->lSampleSize);
			OutTrace("> formattype=%s\n", pmt->formattype);
			OutTrace("> cbFormat=%d\n", pmt->cbFormat);
		}
		//     BYTE *pbFormat;

	}
	else {
		OutTraceDSHOW("%s: ERROR res=%#x\n", ApiRef, res); 
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::Reconnect(IPin *ppin)
{
	ApiName("Reconnect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pin=%ls\n", ApiRef, m_this, ppin); 

	res = m_this->Reconnect(ppin);

	if(res && (dxw.dwFlags18 & IGNOREGRAPHERRORS)){
		OutTraceDSHOW("%s: obj=%#x IGNORING ERROR res=%#x\n", ApiRef, m_this, res); 
		res = 0;
	}
	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::Disconnect(IPin *ppin)
{
	ApiName("Disconnect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pin=%ls\n", ApiRef, m_this, ppin); 

	res = m_this->Disconnect(ppin);

	if(res && (dxw.dwFlags18 & IGNOREGRAPHERRORS)){
		OutTraceDSHOW("%s: obj=%#x IGNORING ERROR res=%#x\n", ApiRef, m_this, res); 
		res = 0;
	}
	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::SetDefaultSyncSource(void)
{
	ApiName("SetDefaultSyncSource");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this); 

	res = m_this->SetDefaultSyncSource();

	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	return res;
}
        
HRESULT STDMETHODCALLTYPE dxwGraphBuilder::Connect(IPin *ppinOut, IPin *ppinIn)
#ifdef BYPASS
{return m_this->Connect(ppinOut, ppinIn); }
#else
{
	ApiName("Connect");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pinOut=%#x pinIn=%#x\n", ApiRef, m_this, ppinOut, ppinIn); 

	res = m_this->Connect(ppinOut, ppinIn);

	if(res && (dxw.dwFlags18 & IGNOREGRAPHERRORS)){
		OutTraceDSHOW("%s: obj=%#x IGNORING ERROR res=%#x\n", ApiRef, m_this, res); 
		res = 0;
	}
	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::Render(IPin *ppinOut)
{
	ApiName("Render");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pin=%#x\n", ApiRef, m_this, ppinOut); 

	// not working .... why?
	//if(dxw.dwFlags6 & NOMOVIES) {
	//	OutTraceDSHOW("%s: NOMOVIES\n", ApiRef); 
	//	return 0;
	//}

	res = m_this->Render(ppinOut);

	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	if(dxw.dwDFlags & DUMPDSHOWGRAPH) GraphDump((IGraphBuilder*)m_this);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
#ifdef BYPASS
{return m_this->RenderFile(lpcwstrFile, lpcwstrPlayList); }
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

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter)
#ifdef BYPASS
{return m_this->AddSourceFilter(lpcwstrFileName, lpcwstrFilterName, ppFilter); }
#else
{
	ApiName("AddSourceFilter");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x filename=%ls filtername=%ls\n", ApiRef, m_this, lpcwstrFileName, lpcwstrFilterName); 

	res = m_this->AddSourceFilter(lpcwstrFileName, lpcwstrFilterName, ppFilter);

	if(res == 0){
		dxwBaseFilter *bf = new(dxwBaseFilter);
		bf->Hook(*ppFilter);
		*ppFilter = bf;
	}
	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::SetLogFile(DWORD_PTR hFile)
#ifdef BYPASS
{return m_this->SetLogFile(hFile); }
#else
{
	ApiName("SetLogFile");
	HRESULT res;

	res = m_this->SetLogFile(hFile);

	OutTraceDSHOW("%s: obj=%#x hFile=%#x res=%#x\n", ApiRef, m_this, hFile, res); 
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::Abort(void)
#ifdef BYPASS
{return m_this->Abort(); }
#else
{
	ApiName("Abort");
	HRESULT res;

	res = m_this->Abort();

	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwGraphBuilder::ShouldOperationContinue(void)
#ifdef BYPASS
{return m_this->ShouldOperationContinue(); }
#else
{
	ApiName("ShouldOperationContinue");
	HRESULT res;

	res = m_this->ShouldOperationContinue();

	OutTraceDSHOW("%s: obj=%#x %s res=%#x\n", ApiRef, m_this, res ? "ERROR" : "OK", res); 
	return res;
}
#endif