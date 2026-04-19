#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IMediaControl"

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

void dxwMediaControl::Hook(IMediaControl *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwMediaControl::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwMediaControl::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwMediaControl::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwMediaControl();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaControl::GetTypeInfoCount(UINT *pctinfo)
{return m_this->GetTypeInfoCount(pctinfo); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{return m_this->GetTypeInfo(iTInfo, lcid, ppTInfo); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{return m_this->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{return m_this->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::Run(void)
{
	HRESULT res;
	ApiName("Run");
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	if(dxw.dwFlags6 & NOMOVIES) {
		OutTraceDW("%s: SUPPRESSED (bypassing method)\n", ApiRef);
		return S_OK;
	}
	if(dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		OutTraceDW("%s: SUPPRESSED (GDI scaling)\n", ApiRef);
		m_GDIMode = dxw.GDIEmulationMode;
		dxw.GDIEmulationMode = GDIMODE_NONE;
	}
	if(dxw.dwFlags5 & PUSHACTIVEMOVIE) {
		OutTraceDSHOW("%s: execute PUSHACTIVEMOVIE\n", ApiRef);
		HWND hwnd = dxw.GethWnd();
		(*pSetWindowPos)(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE);
	}
	res = m_this->Run();
	OutTraceDSHOW("%s: obj=%#x END res=%#x\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaControl::Pause(void)
{
	HRESULT res;
	ApiName("Pause");
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->Pause();
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaControl::Stop(void)
{
	HRESULT res;
	ApiName("Stop");
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->Stop();
	if(dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		dxw.GDIEmulationMode = m_GDIMode;
	}
	if(dxw.dwFlags5 & PUSHACTIVEMOVIE) {
		OutTraceDSHOW("%s: recover PUSHACTIVEMOVIE\n", ApiRef);
		HWND hwnd = dxw.GethWnd();
		(*pSetWindowPos)(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE);
	}
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaControl::GetState(LONG msTimeout, OAFilterState *pfs)
{return m_this->GetState(msTimeout, pfs); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::RenderFile(BSTR strFilename)
#ifdef BYPASS
{return m_this->RenderFile(strFilename); }
#else
{
	HRESULT res;
	ApiName("RenderFile");
	OutTraceDSHOW("%s: obj=%#x path=%ls\n", ApiRef, m_this, strFilename ? strFilename : L"");

	if(dxw.dwFlags6 & NOMOVIES) {
		OutTraceDSHOW("%s: NOMOVIES - setting empty path\n", ApiRef); 
		// v2.06.05: curiously, a simple "return 0" won't do, it would cause an error.
		// instead, a replacement with an invalid path works better.
		// in @#@ "Crashday" the error is bypassed and the game goes to the main menu screen.
		strFilename = (BSTR)L"";
	}
	else
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)){
		DWORD mapping;
		strFilename = (BSTR)dxwTranslatePathW((LPCWSTR)strFilename, &mapping);
		if(mapping != DXW_NO_FAKE){
			OutTraceDSHOW("%s: new path=%ls\n", ApiRef, strFilename);
		}
	}

	res = m_this->RenderFile(strFilename);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwMediaControl::AddSourceFilter(BSTR strFilename, IDispatch **ppUnk)
{return m_this->AddSourceFilter(strFilename, ppUnk); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::get_FilterCollection(IDispatch **ppUnk)
{return m_this->get_FilterCollection(ppUnk); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::get_RegFilterCollection(IDispatch **ppUnk)
{return m_this->get_RegFilterCollection(ppUnk); }

HRESULT STDMETHODCALLTYPE dxwMediaControl::StopWhenReady(void)
#ifdef BYPASS
{return m_this->StopWhenReady(); }
#else
{
	HRESULT res;
	ApiName("StopWhenReady");

	res = m_this->StopWhenReady();
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}
#endif
