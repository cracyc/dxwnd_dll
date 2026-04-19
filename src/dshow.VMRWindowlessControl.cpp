#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IVMRWindowlessControl"

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

void dxwVMRWindowlessControl::Hook(IVMRWindowlessControl *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwVMRWindowlessControl::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwVMRWindowlessControl::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwVMRWindowlessControl();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth,LONG *lpARHeight)
#ifdef BYPASS
{ return m_this->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight); }
#else
{
	ApiName("GetNativeVideoSize");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
	if(res){
		OutErrorDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: obj=%#x w,h=(%d,%d) ARw,h=(%d,%d)\n", ApiRef, m_this, 
			lpWidth ? *lpWidth : 0,
			lpHeight ? *lpHeight : 0, 
			lpARWidth ? *lpARWidth : 0, 
			lpARHeight ? *lpARHeight : 0);
	}
#ifdef NOEXPHACKS
		*lpHeight = 600;
		*lpWidth = 800;
#endif
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetMinIdealVideoSize(LONG *lpWidth, LONG *lpHeight)
#ifdef BYPASS
{ return m_this->GetMinIdealVideoSize(lpWidth, lpHeight); }
#else
{
	ApiName("GetMinIdealVideoSize");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetMinIdealVideoSize(lpWidth, lpHeight);
	if(res){
		OutErrorDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: obj=%#x w,h=(%d,%d)\n", ApiRef, m_this, 
			lpWidth ? *lpWidth : 0, 
			lpHeight ? *lpHeight : 0);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetMaxIdealVideoSize(LONG *lpWidth, LONG *lpHeight)
#ifdef BYPASS
{ return m_this->GetMaxIdealVideoSize(lpWidth, lpHeight); }
#else
{
	ApiName("GetMaxIdealVideoSize");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetMaxIdealVideoSize(lpWidth, lpHeight);
	if(res){
		OutErrorDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: obj=%#x w,h=(%d,%d)\n", ApiRef, m_this, 
			lpWidth ? *lpWidth : 0, 
			lpHeight ? *lpHeight : 0);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect)
#ifdef BYPASS
{ return m_this->SetVideoPosition(lpSRCRect, lpDSTRect); }
#else
{
	ApiName("SetVideoPosition");
	HRESULT res;
#ifndef DXW_NOTRACES
	if(IsTraceDSHOW){
		char sSrcRect[80];
		char sDstRect[80];
		if(lpSRCRect) sprintf_s(sSrcRect, 80, "(%d,%d)-(%d,%d)", lpSRCRect->left, lpSRCRect->top, lpSRCRect->right, lpSRCRect->bottom);
		else strcpy_s(sSrcRect, 80, "NULL");
		if(lpDSTRect) sprintf_s(sDstRect, 80, "(%d,%d)-(%d,%d)", lpDSTRect->left, lpDSTRect->top, lpDSTRect->right, lpDSTRect->bottom);
		else strcpy_s(sDstRect, 80, "NULL");
		OutTrace("%s: obj=%#x srcRect=%s dstRect=%s\n", ApiRef, m_this, sSrcRect, sDstRect);
	}
#endif // DXW_NOTRACES
#ifdef DSHOWHACKS
	BOOL isEmulatedOverlay = (dxw.dwFlags16 & (EMULATEOVERLAY | TRANSPARENTOVERLAY));

	if (dxw.IsFullScreen() && dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)) {
		RECT newRect;
		if(isEmulatedOverlay){
			newRect = dxw.GetScreenRect();
		}
		else {
			newRect.left = newRect.top = 0;
			newRect.right = dxw.iSizX;
			newRect.bottom = dxw.iSizY;
		}
		m_DSTRect = lpDSTRect;
		OutTraceDSHOW("%s: fixed dstRect=(%d,%d)-(%d,%d)\n", ApiRef, newRect.left, newRect.top, newRect.right, newRect.bottom);
		res = m_this->SetVideoPosition(lpSRCRect, (const LPRECT)&newRect);
	}
	else {
		res = m_this->SetVideoPosition(lpSRCRect, lpDSTRect);	
	}
#else
	res = m_this->SetVideoPosition(lpSRCRect, lpDSTRect);	
#endif
	OutTraceDSHOW("%s: obj=%#x res=%#x\n", ApiRef, m_this, res); 
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
#ifdef BYPASS
{ return m_this->GetVideoPosition(lpSRCRect, lpDSTRect); }
#else
{
	ApiName("GetVideoPosition");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetVideoPosition(lpSRCRect, lpDSTRect);
#ifndef DXW_NOTRACES
	if(!res && IsTraceDSHOW){
		char sSrcRect[80];
		char sDstRect[80];
		if(lpSRCRect) sprintf_s(sSrcRect, 80, "(%d,%d)-(%d,%d)", lpSRCRect->left, lpSRCRect->top, lpSRCRect->right, lpSRCRect->bottom);
		else strcpy_s(sSrcRect, 80, "NULL");
		if(lpDSTRect) sprintf_s(sDstRect, 80, "(%d,%d)-(%d,%d)", lpDSTRect->left, lpDSTRect->top, lpDSTRect->right, lpDSTRect->bottom);
		else strcpy_s(sDstRect, 80, "NULL");
		OutTrace("%s: obj=%#x srcRect=%s dstRect=%s\n", ApiRef, m_this, sSrcRect, sDstRect);
	}
	else {
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res); 
	}
#endif // DXW_NOTRACES
#ifdef DSHOWHACKS
	if(dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		OutTraceDSHOW("%s: fixed dstRect\n", ApiRef);
		lpDSTRect = m_DSTRect;	
	}
#endif // DSHOWHACKS
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetAspectRatioMode(DWORD *lpAspectRatioMode)
#ifdef BYPASS
{ return m_this->GetAspectRatioMode(lpAspectRatioMode); }
#else
{
	ApiName("GetAspectRatioMode");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetAspectRatioMode(lpAspectRatioMode);
	if(res){
		OutErrorDSHOW("%s: obj=%#x ERROR res=%#x\n", ApiRef, m_this, res);
	}
	else {
		OutTraceDSHOW("%s: obj=%#x mode=%#x\n", ApiRef, m_this, *lpAspectRatioMode);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::SetAspectRatioMode(DWORD AspectRatioMode)
#ifdef BYPASS
{ return m_this->SetAspectRatioMode(AspectRatioMode); }
#else
{
	ApiName("SetAspectRatioMode");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x mode=%#x\n", ApiRef, m_this, AspectRatioMode);
#ifdef DSHOWHACKS
	if(dxw.Windowize){
		OutTraceDSHOW("%s: obj=%#x FIXED mode=VMR_ARMODE_LETTER_BOX\n", ApiRef, m_this);
		AspectRatioMode = VMR_ARMODE_LETTER_BOX;
	}
#endif
	res = m_this->SetAspectRatioMode(AspectRatioMode);
	if(res) OutErrorDSHOW("%s: obj=%#x ERROR res=%#x\n", ApiRef, m_this, res);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::SetVideoClippingWindow(HWND hwnd)
#ifdef BYPASS
{ return m_this->SetVideoClippingWindow(hwnd); }
#else
{
	ApiName("SetVideoClippingWindow");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x hwnd=%#x\n", ApiRef, m_this, hwnd);
#ifdef DSHOWHACKS
	if(dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		hwnd = dxw.GethWnd();
		OutTraceDSHOW("%s: obj=%#x FIXED hwnd=%#x\n", ApiRef, m_this, hwnd);
	}
#endif // DSHOWHACK
	res = m_this->SetVideoClippingWindow(hwnd);
	if(res){
		OutErrorDSHOW("%s: obj=%#x ERROR res=%#x\n", ApiRef, m_this, res);
	}
#ifdef DSHOWHACKS
	if(dxw.Windowize && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		RECT newRect = dxw.GetUnmappedScreenRect();
		//RECT newRect;
		//newRect.top = newRect.left = 0;
		//newRect.right = dxw.iSizX;
		//newRect.bottom = dxw.iSizY;
		OutTraceDSHOW("%s: fixed dstRect=(%d,%d)-(%d,%d)\n", ApiRef, newRect.left, newRect.top, newRect.right, newRect.bottom);
		res = m_this->SetVideoPosition(NULL, (const LPRECT)&newRect);
		if(res){
			OutErrorDSHOW("%s: obj=%#x ERROR res=%#x\n", ApiRef, m_this, res);
		}
	}
#endif
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::RepaintVideo(HWND hwnd, HDC hdc)
#ifdef BYPASS
{ return m_this->RepaintVideo(hwnd, hdc); }
#else
{
	ApiName("RepaintVideo");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->RepaintVideo(hwnd, hdc);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::DisplayModeChanged( void)
#ifdef BYPASS
{ return m_this->DisplayModeChanged(); }
#else
{
	ApiName("DisplayModeChanged");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->DisplayModeChanged();
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetCurrentImage(BYTE **lpDib)
#ifdef BYPASS
{ return m_this->GetCurrentImage(lpDib); }
#else
{
	ApiName("GetCurrentImage");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetCurrentImage(lpDib);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::SetBorderColor(COLORREF Clr)
#ifdef BYPASS
{ return m_this->SetBorderColor(Clr); }
#else
{
	ApiName("SetBorderColor");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->SetBorderColor(Clr);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetBorderColor(COLORREF *lpClr)
#ifdef BYPASS
{ return m_this->GetBorderColor(lpClr); }
#else
{
	ApiName("GetBorderColor");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetBorderColor(lpClr);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::SetColorKey(COLORREF Clr)
#ifdef BYPASS
{ return m_this->SetColorKey(Clr); }
#else
{
	ApiName("SetColorKey");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->SetColorKey(Clr);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVMRWindowlessControl::GetColorKey(COLORREF *lpClr) 
#ifdef BYPASS
{ return m_this->GetColorKey(lpClr); }
#else
{
	ApiName("GetColorKey");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->GetColorKey(lpClr);
	return res;
}
#endif
