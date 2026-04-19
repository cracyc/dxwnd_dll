#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IVideoWindow"

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

void dxwVideoWindow::Hook(IVideoWindow *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwVideoWindow::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwVideoWindow::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwVideoWindow();
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::GetTypeInfoCount(UINT *pctinfo)
{return m_this->GetTypeInfoCount(pctinfo); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{return m_this->GetTypeInfo(iTInfo, lcid, ppTInfo); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{return m_this->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{return m_this->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_Caption(BSTR Caption)
{
	HRESULT res;
	ApiName("put_Caption");
	if (Caption) {
		OutTraceDSHOW("%s: obj=%#x val=%ls\n", ApiRef, m_this, *Caption);
	}
	else {
		OutTraceDSHOW("%s: obj=%#x val=NULL\n", ApiRef, m_this);
	}
	res = m_this->put_Caption(Caption);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_Caption(BSTR *strCaption)
{return m_this->get_Caption(strCaption); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_WindowStyle(long WindowStyle)
{
	HRESULT res;
	ApiName("put_WindowStyle");
	char buf[80+1];
	OutTraceDSHOW("%s: obj=%#x val=%#x(%s)\n", ApiRef, m_this, WindowStyle, ExplainStyle(WindowStyle, buf, 80));
	res = m_this->put_WindowStyle(WindowStyle);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_WindowStyle(long *WindowStyle)
#ifdef BYPASS
{return m_this->get_WindowStyle(WindowStyle); }
#else
{
	ApiName("get_WindowStyle");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_WindowStyle(WindowStyle); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK wstyle=%#x\n", ApiRef, m_this, *WindowStyle);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_WindowStyleEx(long WindowStyleEx)
{
	HRESULT res;
	ApiName("put_WindowStyleEx");
	char buf[80+1];
	OutTraceDSHOW("%s: obj=%#x val=%#x(%s)\n", ApiRef, m_this, WindowStyleEx, ExplainStyle(WindowStyleEx, buf, 80));
	res = m_this->put_WindowStyleEx(WindowStyleEx);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_WindowStyleEx(long *WindowStyleEx)
#ifdef BYPASS
{return m_this->get_WindowStyleEx(WindowStyleEx); }
#else
{
	ApiName("get_WindowStyleEx");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_WindowStyleEx(WindowStyleEx); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK wstyleEx=%#x\n", ApiRef, m_this, *WindowStyleEx);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_AutoShow(long AutoShow)
{
	HRESULT res;
	ApiName("put_AutoShow");
	OutTraceDSHOW("%s: obj=%#x val=%#x\n", ApiRef, m_this, AutoShow);
	res = m_this->put_AutoShow(AutoShow);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_AutoShow(long *AutoShow)
#ifdef BYPASS
{return m_this->get_AutoShow(AutoShow); }
#else
{
	ApiName("get_AutoShow");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_AutoShow(AutoShow); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK autoshow=%#x\n", ApiRef, m_this, *AutoShow);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_WindowState(long WindowState)
{
	HRESULT res;
	ApiName("put_WindowState");
	OutTraceDSHOW("%s: obj=%#x val=%#x\n", ApiRef, m_this, WindowState);
#ifdef DSHOWHACKS
	if(dxw.Windowize && (SW_MAXIMIZE == WindowState) && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		RECT r = dxw.GetUnmappedScreenRect();
		OutTraceDSHOW("%s: FORCE window pos=(%d, %d)-(%d, %d)\n", ApiRef, r.left, r.top, r.right, r.bottom);
		return m_this->SetWindowPosition(r.left, r.top, r.right, r.bottom);
	}
#endif
	res = m_this->put_WindowState(WindowState);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_WindowState(long *WindowState)
#ifdef BYPASS
{return m_this->get_WindowState(WindowState); }
#else
{
	ApiName("get_WindowState");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_WindowState(WindowState); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK visible=%#x\n", ApiRef, m_this, *WindowState);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_BackgroundPalette(long BackgroundPalette)
{return m_this->put_BackgroundPalette(BackgroundPalette); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_BackgroundPalette(long *pBackgroundPalette)
{return m_this->get_BackgroundPalette(pBackgroundPalette); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_Visible(long Visible)
{
	HRESULT res;
	ApiName("put_Visible");
	OutTraceDSHOW("%s: obj=%#x val=%#x\n", ApiRef, m_this, Visible);
	res = m_this->put_Visible(Visible);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_Visible(long *pVisible)
#ifdef BYPASS
{return m_this->get_Visible(pVisible); }
#else
{
	ApiName("get_Visible");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_Visible(pVisible); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK visible=%d\n", ApiRef, m_this, *pVisible);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_Left(long Left)
{
	HRESULT res;
	ApiName("put_Left");
	OutTraceDSHOW("%s: obj=%#x left=%d\n", ApiRef, m_this, Left);
#ifdef DSHOWHACKS
	if((dxw.dwFlags6 & STRETCHMOVIES) && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		RECT screen = dxw.GetUnmappedScreenRect();
		Left = screen.left;
		OutTraceDW("%s: REMAPPED video obj=%#x left=%d\n", ApiRef, m_this, Left);
	}
#endif
	res = m_this->put_Left(Left);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_Left(long *pLeft)
#ifdef BYPASS
{return m_this->get_Left(pLeft); }
#else
{
	ApiName("get_Left");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_Left(pLeft); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK left=%d\n", ApiRef, m_this, *pLeft);
	}
	return res;
}
#endif


HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_Width(long Width)
{
	HRESULT res;
	ApiName("put_Width");
	OutTraceDSHOW("%s: obj=%#x width=%d\n", ApiRef, m_this, Width);
#ifdef DSHOWHACKS
	if((dxw.dwFlags6 & STRETCHMOVIES) && (dxw.dwFlags18 & SCALEDIRECTSHOW)) {
		RECT screen = dxw.GetUnmappedScreenRect();
		Width = screen.right - screen.left;
		OutTraceDW("%s: REMAPPED video obj=%#x width=%d\n", ApiRef, m_this, Width);
	}
#endif
	res = m_this->put_Width(Width);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_Width(long *pWidth)
#ifdef BYPASS
{return m_this->get_Width(pWidth); }
#else
{
	ApiName("get_Width");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_Width(pWidth); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK width=%d\n", ApiRef, m_this, *pWidth);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_Top(long Top)
{
	HRESULT res;
	ApiName("put_Top");
	OutTraceDSHOW("%s: obj=%#x top=%d\n", ApiRef, m_this, Top);
#ifdef DSHOWHACKS
	if((dxw.dwFlags6 & STRETCHMOVIES) && (dxw.dwFlags18 & SCALEDIRECTSHOW)) {
		RECT screen = dxw.GetUnmappedScreenRect();
		Top = screen.top;
		OutTraceDW("%s: REMAPPED video obj=%#x top=%d\n", ApiRef, m_this, Top);
	}
#endif
	res = m_this->put_Top(Top);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_Top(long *pTop)
#ifdef BYPASS
{return m_this->get_Top(pTop); }
#else
{
	ApiName("get_Top");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_Top(pTop); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK top=%d\n", ApiRef, m_this, *pTop);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_Height(long Height)
{
	HRESULT res;
	ApiName("put_Height");
	OutTraceDSHOW("%s: obj=%#x height=%d\n", ApiRef, m_this, Height);
#ifdef DSHOWHACKS
	if((dxw.dwFlags6 & STRETCHMOVIES) && (dxw.dwFlags18 & SCALEDIRECTSHOW)) {
		RECT screen = dxw.GetUnmappedScreenRect();
		Height = screen.bottom - screen.top;
		OutTraceDW("%s: REMAPPED video obj=%#x height=%d\n", ApiRef, m_this, Height);
	}
#endif
	res = m_this->put_Height(Height);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_Height(long *pHeight)
#ifdef BYPASS
{return m_this->get_Height(pHeight); }
#else
{
	ApiName("get_Height");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_Height(pHeight); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK height=%d\n", ApiRef, m_this, *pHeight);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_Owner(OAHWND Owner)
{
	ApiName("put_Owner");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x hwnd=%#x\n", ApiRef, m_this, Owner);
	res = m_this->put_Owner(Owner); 
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_Owner(OAHWND *Owner)
#ifdef BYPASS
{return m_this->get_Owner(Owner); }
#else
{
	ApiName("get_Owner");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_Owner(Owner); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK owner=%#x\n", ApiRef, m_this, *Owner);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_MessageDrain(OAHWND Drain)
{
	ApiName("put_MessageDrain");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x hwnd=%#x\n", ApiRef, m_this, Drain);
	res = m_this->put_MessageDrain(Drain); 
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_MessageDrain(OAHWND *Drain)
#ifdef BYPASS
{return m_this->get_MessageDrain(Drain); }
#else
{
	ApiName("get_MessageDrain");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
	res = m_this->get_MessageDrain(Drain); 
	if(res){
		OutTraceDSHOW("%s: ERROR obj=%#x res=%#x\n", ApiRef, m_this, res);
	} else {
		OutTraceDSHOW("%s: OK drain=%#x\n", ApiRef, m_this, *Drain);
	}
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_BorderColor(long *Color)
{return m_this->get_BorderColor(Color); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_BorderColor(long Color)
{return m_this->put_BorderColor(Color); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::get_FullScreenMode(long *FullScreenMode)
{
	ApiName("get_FullScreenMode");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x\n", ApiRef, m_this);
#ifdef DSHOWHACKS
	if(dxw.Windowize && dxw.IsFullScreen()){
		OutTraceDSHOW("%s: pretend fullscreen ON\n", ApiRef);
		if(FullScreenMode) *FullScreenMode = OATRUE;
		return S_OK;
	}
#endif
	res = m_this->get_FullScreenMode(FullScreenMode);
	OutTraceDSHOW("%s: res=%#x fullscreenmode=%#x\n", ApiRef, res, *FullScreenMode);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::put_FullScreenMode(long FullScreenMode)
{
	ApiName("put_FullScreenMode");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x fullscreenmode=%#x\n", ApiRef, m_this, FullScreenMode);
#ifdef DSHOWHACKS
	if(dxw.Windowize && FullScreenMode){
		OutTraceDSHOW("%s: set fullscreen OFF\n", ApiRef);
		FullScreenMode = OAFALSE;
	}
#endif
	res = m_this->put_FullScreenMode(FullScreenMode);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::SetWindowForeground(long Focus)
{
	ApiName("SetWindowForeground");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x focus=%#x\n", ApiRef, m_this, Focus);
	res = m_this->SetWindowForeground(Focus); 
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam)
#ifdef BYPASS
{return m_this->NotifyOwnerMessage(hwnd, uMsg, wParam, lParam); }
#else
{
	ApiName("NotifyOwnerMessage");
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x hwnd=%#x msg=%#x(%s) wparam=%#x lparam=%#x\n", 
		ApiRef, m_this, hwnd, uMsg, ExplainWinMessage(uMsg), wParam, lParam);
	res = m_this->NotifyOwnerMessage(hwnd, uMsg, wParam, lParam);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}
#endif

HRESULT STDMETHODCALLTYPE dxwVideoWindow::SetWindowPosition(long Left, long Top, long Width, long Height)
{
	HRESULT res;
	ApiName("SetWindowPosition");
	OutTraceDSHOW("%s: obj=%#x left=%d top=%d width=%d height=%d\n", ApiRef, m_this, Left, Top, Width, Height);
#ifdef noDSHOWHACKS
	RECT screen = dxw.GetUnmappedScreenRect();
	Left = screen.left;
	Top = screen.top;
	if(dxw.dwFlags6 & STRETCHMOVIES) {
		//m_this->put_FullScreenMode(OAFALSE);
		RECT screen = dxw.GetUnmappedScreenRect();
		//Width = screen.right - screen.left;
		//Height = screen.bottom - screen.top;
		OutTraceDW("%s: REMAPPED video obj=%#x left=%d top=%d width=%d height=%d\n", ApiRef, m_this, Left, Top, Width, Height);
	}
#endif
#ifdef DSHOWHACKS
	if((dxw.dwFlags6 & STRETCHMOVIES) && (dxw.dwFlags18 & SCALEDIRECTSHOW)){
		Width = dxw.GetScreenWidth();
		Height = dxw.GetScreenHeight();
		OutTraceDW("%s: REMAPPED video obj=%#x left=%d top=%d width=%d height=%d\n", ApiRef, m_this, Left, Top, Width, Height);
	}
#endif
	res = m_this->SetWindowPosition(Left, Top, Width, Height);
	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwVideoWindow::GetWindowPosition(long *pLeft, long *pTop, long *pWidth, long *pHeight)
{return m_this->GetWindowPosition(pLeft, pTop, pWidth, pHeight); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::GetMinIdealImageSize(long *pWidth, long *pHeight)     
{return m_this->GetMinIdealImageSize(pWidth, pHeight); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::GetMaxIdealImageSize(long *pWidth, long *pHeight)
{return m_this->GetMaxIdealImageSize(pWidth, pHeight); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::GetRestorePosition(long *pLeft, long *pTop, long *pWidth, long *pHeight)
{return m_this->GetRestorePosition(pLeft, pTop, pWidth, pHeight); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::HideCursor(long HideCursor)
{return m_this->HideCursor(HideCursor); }

HRESULT STDMETHODCALLTYPE dxwVideoWindow::IsCursorHidden(long *CursorHidden)
{return m_this->IsCursorHidden(CursorHidden); }

