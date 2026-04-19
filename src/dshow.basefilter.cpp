#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IBaseFilter"

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

void dxwBaseFilter::Hook(IBaseFilter *obj)
{
	OutTraceDSHOW("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwBaseFilter::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwBaseFilter::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwBaseFilter();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::GetClassID(CLSID *pClassID)
{
	ApiName("GetClassID");
	ULONG res;
	res = m_this->GetClassID(pClassID);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x clsid=%s\n", ApiRef, m_this, sGUID(pClassID));
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::Stop(void)
{
	ApiName("Stop");
	ULONG res;
	res = m_this->Stop();
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x\n", ApiRef, m_this);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::Pause(void)
{
	ApiName("Pause");
	ULONG res;
	res = m_this->Pause();
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x\n", ApiRef, m_this);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::Run(REFERENCE_TIME tStart)
{
	ApiName("Run");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x start=%lld\n", ApiRef, m_this, tStart);
	if(dxw.dwFlags6 & NOMOVIES) {
		OutTraceDW("%s: SUPPRESSED (bypassing method)\n", ApiRef);
		res = m_this->Stop();
		return S_OK;
	}
	res = m_this->Run(tStart);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x\n", ApiRef, m_this);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
	ApiName("GetState");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x timeout[msec]=%d\n", ApiRef, m_this, dwMilliSecsTimeout);
	res = m_this->GetState(dwMilliSecsTimeout, State);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x state=%d(%s)\n", ApiRef, m_this, *State,
			*State == State_Running ? "Running" : (*State == State_Paused ? "Paused" : "Stopped"));
	}

	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::SetSyncSource(IReferenceClock *pClock)
{
	ApiName("SetSyncSource");
	ULONG res;
	OutTraceDSHOW("%s: OK obj=%#x clock=%#x\n", ApiRef, m_this, pClock);
	res = m_this->SetSyncSource(pClock);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x\n", ApiRef, m_this);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::GetSyncSource(IReferenceClock **pClock)
{
	ApiName("GetSyncSource");
	ULONG res;
	res = m_this->GetSyncSource(pClock);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x clock=%#x\n", ApiRef, m_this, *pClock);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBaseFilter::EnumPins(IEnumPins **ppEnum)
#ifdef BYPASS
{ return m_this->EnumPins(ppEnum); }
#else
{
	ApiName("EnumPins");
	ULONG res;
	res = m_this->EnumPins(ppEnum);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		HookEnumPins(ppEnum);
		OutTraceDSHOW("%s: OK obj=%#x enumPins=%#x\n", ApiRef, m_this, *ppEnum);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwBaseFilter::FindPin(LPCWSTR Id, IPin **ppPin)
#ifdef BYPASS
{ return m_this->FindPin(Id, ppPin); }
#else
{
	ApiName("FindPin");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x id=%ls\n", ApiRef, m_this, Id);
	res = m_this->FindPin(Id, ppPin);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x pin=%#x\n", ApiRef, m_this, *ppPin);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwBaseFilter::QueryFilterInfo(FILTER_INFO *pInfo)
#ifdef BYPASS
{ return m_this->QueryFilterInfo(pInfo); }
#else
{
	ApiName("QueryFilterInfo");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->QueryFilterInfo(pInfo);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x info=%ls\n", ApiRef, m_this, pInfo->achName);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwBaseFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
#ifdef BYPASS
{ return m_this->JoinFilterGraph(pGraph, pName); }
#else
{
	ApiName("JoinFilterGraph");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x name=%ls\n", ApiRef, m_this, pName);
	res = m_this->JoinFilterGraph(pGraph, pName);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x\n", ApiRef, m_this);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwBaseFilter::QueryVendorInfo(LPWSTR *pVendorInfo)
#ifdef BYPASS
{ return m_this->QueryVendorInfo(pVendorInfo); }
#else
{
	ApiName("QueryVendorInfo");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->QueryVendorInfo(pVendorInfo);
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: OK obj=%#x info=%ls\n", ApiRef, m_this, *pVendorInfo);
	}
	return res;
}
#endif
