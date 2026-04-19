
#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IBasicAudio"

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
#define EXPHACKS 

void dxwBasicAudio::Hook(IBasicAudio *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwBasicAudio::QueryInterface(const IID &riid, void **obp)
{
	ApiName("QueryInterface");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x REFIID=%s(%s)\n", ApiRef, m_this, sRIID(riid), ExplainGUID((GUID *)&riid));
	if(res = CheckInterfaceDS(ApiRef, riid)) return res;
	res = m_this->QueryInterface(riid, obp);
	if(res == 0) HookInterfaceDS(ApiRef, m_this, riid, obp);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwBasicAudio::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwBasicAudio::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwBasicAudio();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicAudio::GetTypeInfoCount(UINT *pctinfo)
{return m_this->GetTypeInfoCount(pctinfo); }

HRESULT STDMETHODCALLTYPE dxwBasicAudio::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{return m_this->GetTypeInfo(iTInfo, lcid, ppTInfo); }

HRESULT STDMETHODCALLTYPE dxwBasicAudio::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{return m_this->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId); }

HRESULT STDMETHODCALLTYPE dxwBasicAudio::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{return m_this->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr); }

HRESULT STDMETHODCALLTYPE dxwBasicAudio::put_Volume(long lVolume)
{return m_this->put_Volume(lVolume); }

HRESULT STDMETHODCALLTYPE dxwBasicAudio::get_Volume(long *plVolume)
{return m_this->get_Volume(plVolume); }

HRESULT STDMETHODCALLTYPE dxwBasicAudio::put_Balance(long lBalance)
{return m_this->put_Balance(lBalance); }

HRESULT STDMETHODCALLTYPE dxwBasicAudio::get_Balance(long *plBalance)
{return m_this->get_Balance(plBalance); }
