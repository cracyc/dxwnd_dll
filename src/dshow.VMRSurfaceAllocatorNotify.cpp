#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IVMRSurfaceAllocatorNotify"

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
//#define DSHOWHACK2

void dxwVMRSurfaceAllocatorNotify::Hook(IVMRSurfaceAllocatorNotify *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwVMRSurfaceAllocatorNotify();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::AdviseSurfaceAllocator(DWORD_PTR dwUserID, IVMRSurfaceAllocator *lpIVMRSurfaceAllocator)
{
	ApiName("AdviseSurfaceAllocator");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x userid=%#x\n", ApiRef, m_this, dwUserID);
	res = m_this->AdviseSurfaceAllocator(dwUserID, lpIVMRSurfaceAllocator);
	if(res == 0) {
		OutTraceDSHOW("%s: pAllocator=%#x\n", ApiRef, *lpIVMRSurfaceAllocator);
		dxwVMRSurfaceAllocator *allocator = new(dxwVMRSurfaceAllocator);
		allocator->Hook(lpIVMRSurfaceAllocator);
		*lpIVMRSurfaceAllocator = *allocator;
	}
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res); // ???
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor)
{
	ApiName("SetDDrawDevice");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x lpdd7=%#x hmon=%#x\n", ApiRef, m_this, lpDDrawDevice, hMonitor);
	res = m_this->SetDDrawDevice(lpDDrawDevice, hMonitor);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor)
{
	ApiName("ChangeDDrawDevice");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x lpdd7=%#x hmon=%#x\n", ApiRef, m_this, lpDDrawDevice, hMonitor);
	res = m_this->ChangeDDrawDevice(lpDDrawDevice, hMonitor);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::RestoreDDrawSurfaces(void)
{
	ApiName("RestoreDDrawSurfaces");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->RestoreDDrawSurfaces();
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::NotifyEvent(LONG EventCode, LONG_PTR Param1, LONG_PTR Param2)
{
	ApiName("NotifyEvent");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x eventcode=%#x par1=%#x par2=%#x\n", ApiRef, m_this, EventCode, Param1, Param2);
	res = m_this->NotifyEvent(EventCode, Param1, Param2);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res); 
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRSurfaceAllocatorNotify::SetBorderColor(COLORREF clrBorder)
{
	ApiName("SetBorderColor");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x color=%#x\n", ApiRef, m_this, clrBorder);
	res = m_this->SetBorderColor(clrBorder);
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res); 
	return res;
}

