
#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IBasicVideo"

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

void dxwBasicVideo::Hook(IBasicVideo *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwBasicVideo::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwBasicVideo::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwBasicVideo();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetTypeInfoCount(UINT *pctinfo)
{return m_this->GetTypeInfoCount(pctinfo); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{return m_this->GetTypeInfo(iTInfo, lcid, ppTInfo); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{return m_this->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{return m_this->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_AvgTimePerFrame(REFTIME *pAvgTimePerFrame)
{return m_this->get_AvgTimePerFrame(pAvgTimePerFrame); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_BitRate(long *pBitRate)
{return m_this->get_BitRate(pBitRate); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_BitErrorRate(long *pBitErrorRate)
{return m_this->get_BitErrorRate(pBitErrorRate); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_VideoWidth(long *pVideoWidth)
{return m_this->get_VideoWidth(pVideoWidth); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_VideoHeight(long *pVideoHeight)
{return m_this->get_VideoHeight(pVideoHeight); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_SourceLeft(long SourceLeft)
{
	ApiName("put_SourceLeft");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, SourceLeft);
	res = m_this->put_SourceLeft(SourceLeft); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_SourceLeft(long *pSourceLeft)
{return m_this->get_SourceLeft(pSourceLeft); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_SourceWidth(long SourceWidth)
{
	ApiName("put_SourceWidth");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, SourceWidth);
	res = m_this->put_SourceWidth(SourceWidth); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_SourceWidth(long *pSourceWidth)
{return m_this->get_SourceWidth(pSourceWidth); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_SourceTop(long SourceTop)
{
	ApiName("put_SourceTop");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, SourceTop);
	res = m_this->put_SourceTop(SourceTop); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_SourceTop(long *pSourceTop)
{return m_this->get_SourceTop(pSourceTop); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_SourceHeight(long SourceHeight)
{
	ApiName("put_SourceHeight");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, SourceHeight);
	res = m_this->put_SourceHeight(SourceHeight); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_SourceHeight(long *pSourceHeight)
{return m_this->get_SourceHeight(pSourceHeight); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_DestinationLeft(long DestinationLeft)
{
	ApiName("put_DestinationLeft");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, DestinationLeft);
	res = m_this->put_DestinationLeft(DestinationLeft); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_DestinationLeft(long *pDestinationLeft)
{return m_this->get_DestinationLeft(pDestinationLeft); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_DestinationWidth(long DestinationWidth)
{
	ApiName("put_DestinationWidth");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, DestinationWidth);
	res = m_this->put_DestinationWidth(DestinationWidth); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_DestinationWidth(long *pDestinationWidth)
{return m_this->get_DestinationWidth(pDestinationWidth); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_DestinationTop(long DestinationTop)
{
	ApiName("put_DestinationTop");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, DestinationTop);
	res = m_this->put_DestinationTop(DestinationTop); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_DestinationTop(long *pDestinationTop)
{return m_this->get_DestinationTop(pDestinationTop); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::put_DestinationHeight(long DestinationHeight)
{
	ApiName("put_DestinationHeight");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x val=%d\n", ApiRef, m_this, DestinationHeight);
	res = m_this->put_DestinationHeight(DestinationHeight); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::get_DestinationHeight(long *pDestinationHeight)
{return m_this->get_DestinationHeight(pDestinationHeight); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::SetSourcePosition(long Left, long Top, long Width, long Height)
{
	ApiName("SetSourcePosition");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pos=(%d, %d) size=(%d x %d)\n", ApiRef, m_this, Left, Top, Width, Height);
	res = m_this->SetSourcePosition(Left, Top, Width, Height); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetSourcePosition(long *pLeft, long *pTop, long *pWidth, long *pHeight)
{return m_this->GetSourcePosition(pLeft, pTop, pWidth, pHeight); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::SetDefaultSourcePosition(void)
{return m_this->SetDefaultSourcePosition(); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::SetDestinationPosition(long Left, long Top, long Width, long Height)
{
	ApiName("SetDestinationPosition");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x pos=(%d, %d) size=(%d x %d)\n", ApiRef, m_this, Left, Top, Width, Height);
	res = m_this->SetDestinationPosition(Left, Top, Width, Height); 
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetDestinationPosition(long *pLeft, long *pTop, long *pWidth, long *pHeight)
{return m_this->GetDestinationPosition(pLeft, pTop, pWidth, pHeight); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::SetDefaultDestinationPosition(void)
{return m_this->SetDefaultDestinationPosition(); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetVideoSize(long *pWidth, long *pHeight)
{return m_this->GetVideoSize(pWidth, pHeight); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetVideoPaletteEntries(long StartIndex, long Entries, long *pRetrieved, long *pPalette)
{return m_this->GetVideoPaletteEntries(StartIndex, Entries, pRetrieved, pPalette); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::GetCurrentImage(long *pBufferSize, long *pDIBImage)
{return m_this->GetCurrentImage(pBufferSize, pDIBImage); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::IsUsingDefaultSource(void)
{return m_this->IsUsingDefaultSource(); }

HRESULT STDMETHODCALLTYPE dxwBasicVideo::IsUsingDefaultDestination(void)
{return m_this->IsUsingDefaultDestination(); }

