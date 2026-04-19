
#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IAMOpenProgress"

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

void dxwAMOpenProgress::Hook(IAMOpenProgress *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwAMOpenProgress::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwAMOpenProgress::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwAMOpenProgress::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwAMOpenProgress();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwAMOpenProgress::QueryProgress(LONGLONG *pllTotal, LONGLONG *pllCurrent)
#ifdef BYPASS
{ return m_this->QueryProgress(pllTotal, pllCurrent); }
#else
{
	ApiName("QueryProgress");
	ULONG res;
	res = m_this->QueryProgress(pllTotal, pllCurrent);
	if(res){
		OutTraceDSHOW("%s: obj=%#x ERROR res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: obj=%#x total=%llu current=%llu\n", ApiRef, m_this, *pllTotal, *pllCurrent);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwAMOpenProgress::AbortOperation(void)
#ifdef BYPASS
{ return m_this->AbortOperation(); }
#else
{
	ApiName("AbortOperation");
	ULONG res;
	res = m_this->AbortOperation();
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res);
	return res;
}
#endif
