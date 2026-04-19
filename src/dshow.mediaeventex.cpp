#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IMediaEventEx"
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

#define DSHOWHACKS

void dxwMediaEventEx::Hook(IMediaEventEx *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwMediaEventEx::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwMediaEventEx::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwMediaEventEx();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::GetTypeInfoCount(UINT *pctinfo)
{ return m_this->GetTypeInfoCount(pctinfo); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
#ifdef BYPASS
{ return m_this->GetTypeInfo(iTInfo, lcid, ppTInfo); }
#else
{
	ApiName("GetTypeInfo");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x tInfo=%d lcid=%#x\n", ApiRef, m_this, iTInfo, lcid);
	
	res = m_this->GetTypeInfo(iTInfo, lcid, ppTInfo);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x info=%ls\n", ApiRef, m_this, *ppTInfo);
		HookTypeInfo(ppTInfo);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{ return m_this->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{ return m_this->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::GetEventHandle(OAEVENT *hEvent)
{ return m_this->GetEventHandle(hEvent); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::GetEvent(long *lEventCode, LONG_PTR *lParam1, LONG_PTR *lParam2, long msTimeout)
{ return m_this->GetEvent(lEventCode, lParam1, lParam2, msTimeout); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::WaitForCompletion(long msTimeout, long *pEvCode)
{ return m_this->WaitForCompletion(msTimeout, pEvCode); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::CancelDefaultHandling(long lEvCode)
{ return m_this->CancelDefaultHandling(lEvCode); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::RestoreDefaultHandling(long lEvCode)
{ return m_this->RestoreDefaultHandling(lEvCode); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::FreeEventParams(long lEvCode, LONG_PTR lParam1, LONG_PTR lParam2)
{ return m_this->FreeEventParams(lEvCode, lParam1, lParam2); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::SetNotifyWindow(OAHWND hwnd, long lMsg, LONG_PTR lInstanceData)
{ return SetNotifyWindow(hwnd, lMsg, lInstanceData); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::SetNotifyFlags(long lNoNotifyFlags)
{ return m_this->SetNotifyFlags(lNoNotifyFlags); }

HRESULT STDMETHODCALLTYPE dxwMediaEventEx::GetNotifyFlags(long *lplNoNotifyFlags)
{ return m_this->GetNotifyFlags(lplNoNotifyFlags); }

#undef _COMPONENT
#define _COMPONENT "IMediaEvent"

void dxwMediaEvent::Hook(IMediaEvent *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwMediaEvent::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwMediaEvent::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwMediaEvent::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwMediaEvent();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaEvent::GetTypeInfoCount(UINT *pctinfo)
{ return m_this->GetTypeInfoCount(pctinfo); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
#ifdef BYPASS
{ return m_this->GetTypeInfo(iTInfo, lcid, ppTInfo); }
#else
{
	ApiName("GetTypeInfo");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x tInfo=%d lcid=%#x\n", ApiRef, m_this, iTInfo, lcid);
	
	res = m_this->GetTypeInfo(iTInfo, lcid, ppTInfo);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x info=%ls\n", ApiRef, m_this, *ppTInfo);
		HookTypeInfo(ppTInfo);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwMediaEvent::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{ return m_this->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{ return m_this->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::GetEventHandle(OAEVENT *hEvent)
{ return m_this->GetEventHandle(hEvent); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::GetEvent(long *lEventCode, LONG_PTR *lParam1, LONG_PTR *lParam2, long msTimeout)
{ return m_this->GetEvent(lEventCode, lParam1, lParam2, msTimeout); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::WaitForCompletion(long msTimeout, long *pEvCode)
{ return m_this->WaitForCompletion(msTimeout, pEvCode); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::CancelDefaultHandling(long lEvCode)
{ return m_this->CancelDefaultHandling(lEvCode); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::RestoreDefaultHandling(long lEvCode)
{ return m_this->RestoreDefaultHandling(lEvCode); }

HRESULT STDMETHODCALLTYPE dxwMediaEvent::FreeEventParams(long lEvCode, LONG_PTR lParam1, LONG_PTR lParam2)
{ return m_this->FreeEventParams(lEvCode, lParam1, lParam2); }
