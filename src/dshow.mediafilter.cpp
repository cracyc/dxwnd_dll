#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IMediaFilter"

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

void dxwMediaFilter::Hook(IMediaFilter *obj)
{
	//OutTraceDSHOW("dxwMediaFilter.Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwMediaFilter::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwMediaFilter::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwMediaFilter();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::GetClassID(CLSID *pClassID)
{
	ApiName("GetClassID");
	ULONG res;
	res = m_this->GetClassID(pClassID);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::Stop(void)
{
	ApiName("Stop");
	ULONG res;
	res = m_this->Stop();
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::Pause(void)
{
	ApiName("Pause");
	ULONG res;
	res = m_this->Pause();
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::Run(REFERENCE_TIME tStart)
{
	ApiName("Run");
	ULONG res;
	res = m_this->Run(tStart);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
	ApiName("GetState");
	ULONG res;
	res = m_this->GetState(dwMilliSecsTimeout, State);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::SetSyncSource(IReferenceClock *pClock)
{
	ApiName("SetSyncSource");
	ULONG res;
	res = m_this->SetSyncSource(pClock);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwMediaFilter::GetSyncSource(IReferenceClock **pClock)
{
	ApiName("GetSyncSource");
	ULONG res;
	res = m_this->GetSyncSource(pClock);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}

