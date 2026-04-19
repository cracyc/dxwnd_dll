#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IVMRImagePresenterConfig"

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

void dxwVMRImagePresenterConfig::Hook(IVMRImagePresenterConfig *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwVMRImagePresenterConfig::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwVMRImagePresenterConfig::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwVMRImagePresenterConfig::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwVMRImagePresenterConfig();
	return res;
}

char *sRenderingPrefs(DWORD f, char *eb, int eblen)
{
	unsigned int l;
	*eb = 0;
	eblen -= 4;
	if (f & RenderPrefs_ForceOffscreen) strscat(eb, eblen, "ForceOffscreen+");
	if (f & RenderPrefs_ForceOverlays) strscat(eb, eblen, "RenderPrefs_ForceOverlays+");
	if (f & RenderPrefs_DoNotRenderColorKeyAndBorder) strscat(eb, eblen, "DoNotRenderColorKeyAndBorder+");
	if (f & RenderPrefs_Reserved) strscat(eb, eblen, "Reserved+");
	if (f & RenderPrefs_PreferAGPMemWhenMixing) strscat(eb, eblen, "PreferAGPMemWhenMixing+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>0) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}

HRESULT STDMETHODCALLTYPE dxwVMRImagePresenterConfig::SetRenderingPrefs(DWORD dwRenderFlags)
#ifdef BYPASS
{ return m_this->SetRenderingPrefs(dwRenderFlags); }
#else
{
	ApiName("SetRenderingPrefs");
	HRESULT res;
	char buf[80+1];
	OutTraceDSHOW("%s: obj=%#x mode=%#x(%s)\n", ApiRef, m_this, dwRenderFlags, sRenderingPrefs(dwRenderFlags, buf, 80));
#ifdef DSHOWHACKS
	if(dxw.IsEmulated){
		dwRenderFlags |= RenderPrefs_ForceOffscreen;
		dwRenderFlags &= ~RenderPrefs_ForceOverlays;
	}
#endif
	res = m_this->SetRenderingPrefs(dwRenderFlags);
	if(res) OutErrorDSHOW("%s: obj=%#x ERROR res=%#x\n", ApiRef, m_this, res);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRImagePresenterConfig::GetRenderingPrefs(DWORD *dwRenderFlags)
#ifdef BYPASS
{ return m_this->GetRenderingPrefs(dwRenderFlags); }
#else
{
	ApiName("GetRenderingPrefs");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x mode=%#x\n", ApiRef, m_this, dwRenderFlags);
	res = m_this->GetRenderingPrefs(dwRenderFlags);
	if(res) {
		OutErrorDSHOW("%s: obj=%#x ERROR res=%#x\n", ApiRef, m_this, res);
	}
	else {
		char buf[80+1];
		OutErrorDSHOW("%s: obj=%#x renderingflags=%#x\n", 
			ApiRef, m_this, res, *dwRenderFlags, sRenderingPrefs(*dwRenderFlags, buf, 80));
	}
	return res;
}
#endif

