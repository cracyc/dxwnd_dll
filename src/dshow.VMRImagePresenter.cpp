#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IVMRImagePresenter"

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

void dxwVMRImagePresenter::Hook(IVMRImagePresenter *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwVMRImagePresenter::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwVMRImagePresenter::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwVMRImagePresenter::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwVMRImagePresenter();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRImagePresenter::StartPresenting(DWORD_PTR dwUserID)
{
	ApiName("StartPresenting");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x userid=%#x\n", ApiRef, m_this, dwUserID);
	res = m_this->StartPresenting(dwUserID);
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRImagePresenter::StopPresenting(DWORD_PTR dwUserID)
{
	ApiName("StopPresenting");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x userid=%#x\n", ApiRef, m_this, dwUserID);
	res = m_this->StopPresenting(dwUserID);
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

void dumpPresInfo(VMRPRESENTATIONINFO *lpPresInfo)
{
	if(IsTraceDSHOW){
		OutTrace(
			"\t> dwFlags=%#x\n"
			"\t> lpSurf=%#x\n"
			"\t> szAspectRatio=%dx%d\n"
			"\t> rcSrc=(%d, %d)-(%d, %d)\n"
			"\t> rcDst=(%d, %d)-(%d, %d)\n"
			"\t> dwTypeSpecificFlags=%#x\n"
			"\t> dwInterlaceFlags=%#x\n",
			lpPresInfo->dwFlags,
			lpPresInfo->lpSurf,
			lpPresInfo->szAspectRatio.cx, lpPresInfo->szAspectRatio.cy,
			lpPresInfo->rcSrc.left, lpPresInfo->rcSrc.top, lpPresInfo->rcSrc.right, lpPresInfo->rcSrc.bottom,
			lpPresInfo->rcDst.left, lpPresInfo->rcDst.top, lpPresInfo->rcDst.right, lpPresInfo->rcDst.bottom,
			lpPresInfo->dwTypeSpecificFlags,
			lpPresInfo->dwInterlaceFlags);
	}
}
 
HRESULT STDMETHODCALLTYPE dxwVMRImagePresenter::PresentImage(DWORD_PTR dwUserID, VMRPRESENTATIONINFO *lpPresInfo)
{
	ApiName("PresentImage");
	ULONG res;
	OutTraceDSHOW("%s: obj=%#x userid=%#x presinfo=%#x\n", ApiRef, m_this, dwUserID, lpPresInfo);
	res = m_this->PresentImage(dwUserID, lpPresInfo);
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	if(!res && lpPresInfo) dumpPresInfo(lpPresInfo);
	return res;
}
