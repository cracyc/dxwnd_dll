#define _CRT_SECURE_NO_WARNINGS
#define INITGUID

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
//#define HOOKALLQUERIES

#ifdef HOOKALLQUERIES
void HookDirectShowClasses(LPVOID);
#endif

typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID, LPVOID *);
typedef HRESULT (WINAPI *Void_Type)(void *);

HRESULT CheckInterfaceDS(ApiArg, REFIID riid)
{
	if(dxw.dwFlags18 & SUPPRESSIVMR){
		switch(riid.Data1){
			case 0x0eb1088c: // IVMRWindowlessControl
			case 0xCE704FE7: // IVMRImagePresenter
			case 0x9f3a1c85: // IVMRImagePresenterConfig
			case 0x31ce832e: // IVMRSurfaceAllocator
				if(dxw.Windowize){
					OutTraceDSHOW("%s: SUPPRESS IVMR controls ret=E_NOINTERFACE\n", ApiRef);
					return E_NOINTERFACE;
				}
				break;
		}
	}
	return 0;
}

void HookInterfaceDS(ApiArg, void *obj, REFIID riid, LPVOID *obp)
{
	switch(riid.Data1){
		case 0x56a8689f: // IFilterGraph
			//if(IsEqualGUID(riid, IID_IFilterGraph)) 
				HookFilterGraph((IFilterGraph **)obp);
			break;
		case 0x36b73882: // IFilterGraph2
			//if(IsEqualGUID(riid, IID_IFilterGraph2)) 
				HookFilterGraph2((IFilterGraph2 **)obp);
			break;
		case 0x29840822: // ICreateDevEnum
			//if(IsEqualGUID(riid, IID_ICreateDevEnum)) 
				HookCreateDevEnum((ICreateDevEnum **)obp);
			break;
		case 0x56a868a9: // IGraphBuilder
			//if(IsEqualGUID(riid, IID_IGraphBuilder)) 
				HookGraphBuilder((IGraphBuilder **)obp);
			break;
		case 0x56a86893: // IEnumFilters
			//if(IsEqualGUID(riid, IID_IEnumFilters)) 
				HookEnumFilters((IEnumFilters **)obp);
			break;
		case 0x8E1C39A1: // IAMOpenProgress
			HookIAMOpenProgress((IAMOpenProgress **)obp);
			break;
		case 0x56a868b4: // IID_IVideoWindow
			HookVideoWindow((IVideoWindow **)obp);
			break;
		case 0x56a868b1: // IID_IMediaControl
			HookMediaControl((IMediaControl **)obp);
			break;
		case 0x56a868a2: // IID_MediaEventSink
			HookMediaEventSink((IMediaEventSink **)obp);
			break;
		case 0x56a86895: // IID_BaseFilter
			HookBaseFilter((IBaseFilter **)obp);
			break;
		case 0x56a86899: // IID_MediaFilter
			HookMediaFilter((IMediaFilter **)obp);
			break;
		case 0xa490b1e4: // IMFVideoDisplayControl
			HookIMFVideoDisplayControl(obp);
			break;
		case 0x6bc1cffa: 
			if(IsEqualGUID(riid, CLSID_VideoRendererDefault)) HookVideoRendererDefault(obp);
			break;
		case 0x56a868b5: // IID_IBasicVideo
		case 0x329bb360: // IID_IBasicVideo2 -- adds a last GetPreferredAspectRatio method
			if(IsEqualGUID(riid, IID_IBasicVideo)) HookBasicVideo((IBasicVideo **)obp);
			if(IsEqualGUID(riid, IID_IBasicVideo2)) HookBasicVideo((IBasicVideo **)obp);
			break;
		case 0x56a868c0: // IMediaEventEx
			if(IsEqualGUID(riid, IID_IMediaEventEx)) HookMediaEventEx((IMediaEventEx **)obp);
			break;
		case 0x56a868b6: // IMediaEvent
			if(IsEqualGUID(riid, IID_IMediaEvent)) HookMediaEvent((IMediaEvent **)obp);
			break;
		case 0x36b73880: // IMediaSeeking
			//if(IsEqualGUID(riid, IID_IMediaSeeking)) 
				HookMediaSeeking((IMediaSeeking **)obp);
			break;
		case 0xDFDFD197: // IMFVideoRenderer
			//if(IsEqualGUID(riid, IID_IMFVideoRenderer)) 
				HookIMFVideoRenderer(obp);
			break;
		case 0x0eb1088c: // "IVMRWindowlessControl"
			//if(IsEqualGUID(riid, IID_IVMRWindowlessControl)) 
				HookIVMRWindowlessControl((IVMRWindowlessControl **)obp);
			break;
		case 0xCE704FE7: // "IID_IVMRImagePresenter"
			//if(IsEqualGUID(riid, IID_IVMRImagePresenter)) 
				HookIVMRImagePresenter((IVMRImagePresenter **)obp);
			break;
		case 0x9f3a1c85: // IVMRImagePresenterConfig
			//if(IsEqualGUID(riid, IID_IVMRImagePresenterConfig)) 
				HookIVMRImagePresenterConfig((IVMRImagePresenterConfig **) obp);
			break;
		case 0x6411d54: // IVMRMonitorConfig
			//if(IsEqualGUID(riid, IID_IVMRMonitorConfig)) 
				HookIVMRMonitorConfig(obp);
			break;
		case 0x31ce832e: // "IVMRSurfaceAllocator"
			//if(IsEqualGUID(riid, IID_IVMRSurfaceAllocator)) 
				HookVMRSurfaceAllocator((IVMRSurfaceAllocator **)obp);
			break;
			
#ifdef HOOKALLQUERIES
		default:
			OutTraceDSHOW("%s: Hooking unknown DirectShow class REFIID=%s\n", ApiRef, sRIID(riid));
			//OutTraceDSHOW("%s: Hooking unknown DirectShow class REFIID=???\n", ApiRef);
			HookDirectShowClasses(obp);
			break;
#endif
	}
}

HRESULT WINAPI QueryInterfaceDS(ApiArg, QueryInterface_Type pQueryInterface, void *obj, REFIID riid, LPVOID *obp)
{
	HRESULT res;
	OutTraceDSHOW("%s: obj=%#x REFIID=%s(%s)\n", ApiRef, obj, sRIID(riid), ExplainGUID((GUID *)&riid));
	if(res = CheckInterfaceDS(ApiRef, riid)) return res;
	res = (*pQueryInterface)(obj, riid, obp);
	if(res) {
		OutErrorDSHOW("%s: ERROR ret=%#x\n", ApiRef, res);
	}
	else {
		OutTraceDSHOW("%s: OK obp=%#x ret=%#x\n", ApiRef, *obp);
		HookInterfaceDS(ApiRef, obj, riid, obp);
	}
	return res;
}

#ifdef HOOKALLQUERIES
QueryInterface_Type pQueryInterfaceGeneric;
HRESULT WINAPI extQueryInterfaceGeneric(void *, REFIID, LPVOID *);

void HookDirectShowClasses(LPVOID obj)
{
	OutTraceDSHOW("dxwnd.HookDirectShowClasses: obj=%#x\n", obj);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceGeneric, (void **)&pQueryInterfaceGeneric, "QueryInterface");
}

HRESULT WINAPI extQueryInterfaceGeneric(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("unknown::QueryInterface", pQueryInterfaceGeneric, obj, riid, obp); }
#endif

// ----- IFilterGraph -----
// MIDL_INTERFACE("56a8689f-0ad4-11ce-b03a-0020af0ba770")

void HookFilterGraph(IFilterGraph **obj)
{
	OutTraceDSHOW("dxwnd.HookFilterGraph: obj=%#x\n", *obj);
	dxwFilterGraph *fg = new(dxwFilterGraph);
	fg->Hook(*obj);
	*obj = fg;
}

// ----- IFilterGraph2 -----
// MIDL_INTERFACE("36b73882-c2c8-11cf-8b46-00805f6cef60")

void HookFilterGraph2(IFilterGraph2 **obj)
{
	OutTraceDSHOW("dxwnd.HookFilterGraph2: obj=%#x\n", *obj);
	dxwFilterGraph2 *fg2 = new(dxwFilterGraph2);
	fg2->Hook(*obj);
	*obj = fg2;
}

// ----- ICreateDevEnum -----
// MIDL_INTERFACE("29840822-5B84-11D0-BD3B-00A0C911CE86")

void HookCreateDevEnum(ICreateDevEnum **obj)
{
	OutTraceDSHOW("dxwnd.HookCreateDevEnum: obj=%#x\n", *obj);
	dxwCreateDevEnum *cde = new(dxwCreateDevEnum);
	cde->Hook(*obj);
	*obj = cde;
}

// === EnumFilters ===
//    MIDL_INTERFACE("56a86893-0ad4-11ce-b03a-0020af0ba770")
//    IEnumFilters : public IUnknown

void HookEnumFilters(IEnumFilters **obj)
{
	OutTraceDSHOW("dxwnd.HookEnumFilters: obj=%#x\n", *obj);
	dxwEnumFilters *ef = new(dxwEnumFilters);
	ef->Hook(*obj);
	*obj = ef;
}

// === EnumPins ===

void HookEnumPins(IEnumPins **obj)
{
	OutTraceDSHOW("dxwnd.HookEnumPins: obj=%#x\n", *obj);
	dxwEnumPins *ep = new(dxwEnumPins);
	ep->Hook(*obj);
	*obj = ep;
}


// ===== IID_GraphBuilder =====

//    MIDL_INTERFACE("56a868a9-0ad4-11ce-b03a-0020af0ba770")
//    IGraphBuilder : public IFilterGraph

void HookGraphBuilder(IGraphBuilder **obj)
{
	OutTraceDSHOW("dxwnd.HookGraphBuilder: obj=%#x\n", *obj);
	dxwGraphBuilder *gb = new(dxwGraphBuilder);
	gb->Hook(*obj);
	*obj = (IGraphBuilder *)gb;
}

// ======= IID_IMediaFilter =======

//     MIDL_INTERFACE("56a86899-0ad4-11ce-b03a-0020af0ba770")
//	   IMediaFilter : public IPersist

void HookMediaFilter(IMediaFilter **obj)
{
	OutTraceDSHOW("Dxwnd.HookMediaFilter: obj=%#x\n", *obj);
	dxwMediaFilter *mf = new(dxwMediaFilter);
	mf->Hook(*obj);
	*obj = mf;
}

// ======= IID_IBaseFilter =======

//     MIDL_INTERFACE("56a86895-0ad4-11ce-b03a-0020af0ba770")
//	   IBaseFilter : public IMediaFilter

void HookBaseFilter(IBaseFilter **obj)
{
	OutTraceDSHOW("Dxwnd.HookBaseFilter: obj=%#x\n", *obj);
	dxwBaseFilter *bf = new(dxwBaseFilter);
	bf->Hook(*obj);
	*obj = bf;
}

// === IVMRSurfaceAllocator ===

void HookVMRSurfaceAllocator(IVMRSurfaceAllocator **obj)
{
	OutTraceDSHOW("Dxwnd.HookVMRSurfaceAllocator: obj=%#x\n", *obj);
	dxwVMRSurfaceAllocator *sa = new(dxwVMRSurfaceAllocator);
	sa->Hook(*obj);
	*obj = sa;
}

// ======= IID_IMediaEventSink =======

void HookMediaEventSink(IMediaEventSink **obj)
{
	OutTraceDSHOW("dxwnd.HookMediaEventSink: obj=%#x\n", *obj);
	dxwMediaEventSink *mes = new(dxwMediaEventSink);
	mes->Hook(*obj);
	*obj = mes;
}

// ========== IMediaControl ==========

void HookMediaControl(IMediaControl **obj)
{
	OutTraceDSHOW("dxwnd.HookMediaControl: obj=%#x\n", *obj);
	dxwMediaControl *mc = new(dxwMediaControl);
	mc->Hook(*obj);
	*obj = mc;
}

// ======= IVideoWindow =======

void HookVideoWindow(IVideoWindow **obj)
{
	OutTraceDSHOW("dxwnd.HookVideoWindow: obj=%#x\n", *obj);
	dxwVideoWindow *vw = new(dxwVideoWindow);
	vw->Hook(*obj);
	*obj = vw;
}

// ----- IVMRImagePresenterConfig -----
//    MIDL_INTERFACE("9f3a1c85-8555-49ba-935f-be5b5b29d178")

void HookIVMRImagePresenterConfig(IVMRImagePresenterConfig **obj)
{
	OutTraceDSHOW("dxwnd.HookVMRImagePresenterConfig: obj=%#x\n", *obj);
	dxwVMRImagePresenterConfig *ipc = new(dxwVMRImagePresenterConfig);
	ipc->Hook(*obj);
	*obj = (IVMRImagePresenterConfig *)ipc;
};

// ----- IBasicAudio -----

void HookBasicAudio(IBasicAudio **obj)
{
	OutTraceDSHOW("dxwnd.HookBasicAudio: obj=%#x\n", *obj);
	dxwBasicAudio *ba = new(dxwBasicAudio);
	ba->Hook(*obj);
	*obj = (IBasicAudio *)ba;
}

// ----- IMFVideoDisplayControl -----

QueryInterface_Type pQueryInterfaceMFVDC;
HRESULT WINAPI extQueryInterfaceMFVDC(void *, REFIID, LPVOID *);

void HookIMFVideoDisplayControl(LPVOID obj)
{
	OutTraceDSHOW("dxwnd.HookIMFVideoDisplayControl: obj=%#x\n", obj);
	MessageBox(0, "HookIMFVideoDisplayControl", "dxwnd", 0);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceMFVDC, (void **)&pQueryInterfaceMFVDC, "QueryInterface");
	// 1: AddRef
	// 2: Release
}

HRESULT WINAPI extQueryInterfaceMFVDC(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("IMFVideoDisplayControl::QueryInterface", pQueryInterfaceMFVDC, obj, riid, obp); }

// ----- IMFVideoRenderer -----

//MIDL_INTERFACE("DFDFD197-A9CA-43d8-B341-6AF3503792CD")
//IMFVideoRenderer : public IUnknown

QueryInterface_Type pQueryInterfaceMFVR;
HRESULT WINAPI extQueryInterfaceMFVR(void *, REFIID, LPVOID *);

void HookIMFVideoRenderer(LPVOID obj)
{
	OutTraceDSHOW("dxwnd.HookIMFVideoRenderer: obj=%#x\n", obj);
	//MessageBox(0, "HookIMFVideoRenderer", "dxwnd", 0);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceMFVR, (void **)&pQueryInterfaceMFVR, "QueryInterface");
	// 1: AddRef
	// 2: Release
}

HRESULT WINAPI extQueryInterfaceMFVR(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("HookIMFVideoRenderer::QueryInterface", pQueryInterfaceMFVR, obj, riid, obp); }

// ===== IVMRWindowlessControl =====

void HookIVMRWindowlessControl(IVMRWindowlessControl **obj)
{
	OutTraceDSHOW("dxwnd.HookIVMRWindowlessControl: obj=%#x\n", *obj);
	dxwVMRWindowlessControl *vw = new(dxwVMRWindowlessControl);
	vw->Hook(*obj);
	*obj = (IVMRWindowlessControl *)vw;
}

// ===== IVMRImagePresenter =====

void HookIVMRImagePresenter(IVMRImagePresenter **obj)
{
	OutTraceDSHOW("dxwnd.HookIVMRImagePresenter: obj=%#x\n", *obj);
	dxwVMRImagePresenter *ip = new(dxwVMRImagePresenter);
	ip->Hook(*obj);
	*obj = (IVMRImagePresenter *)ip;
}

// ----- IID_IBasicVideo -----

void HookBasicVideo(IBasicVideo **obj)
{
	OutTraceDSHOW("dxwnd.HookBasicVideo: obj=%#x\n", *obj);
	dxwBasicVideo *ip = new(dxwBasicVideo);
	ip->Hook(*obj);
	*obj = (IBasicVideo *)ip;
}

// ----- IAMOpenProgress -----
// MIDL_INTERFACE("8E1C39A1-DE53-11cf-AA63-0080C744528D")

void HookIAMOpenProgress(IAMOpenProgress **obj)
{
	OutTraceDSHOW("dxwnd.HookIAMOpenProgress: obj=%#x\n", *obj);
	dxwAMOpenProgress *vw = new(dxwAMOpenProgress);
	vw->Hook(*obj);
	*obj = (IAMOpenProgress *)vw;
}

// ----- VideoRendererDefault -----

    //helpstring("Default Video Renderer"),
    //threading(both),
    //uuid(6bc1cffa-8fc1-4261-ac22-cfb4cc38db50)

QueryInterface_Type pQueryInterfaceIVRD;
HRESULT WINAPI extQueryInterfaceIVRD(void *, REFIID, LPVOID *);

void HookVideoRendererDefault(void *obj)
{
	OutTraceDSHOW("dxwnd.HookVideoRendererDefault: obj=%#x\n", obj);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceIVRD, (void **)&pQueryInterfaceIVRD, "QueryInterface");
	// 4: AddRef
	// 8: Release
}

HRESULT WINAPI extQueryInterfaceIVRD(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("VideoRendererDefault::QueryInterface", pQueryInterfaceIVRD, obj, riid, obp); }

// ----- HookIVMRMonitorConfig -----

QueryInterface_Type pQueryInterfaceIVMR;
HRESULT WINAPI extQueryInterfaceIVMR(void *, REFIID, LPVOID *);

void HookIVMRMonitorConfig(void *obj)
{
	OutTraceDSHOW("dxwnd.HookIVMRMonitorConfig: obj=%#x\n", obj);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceIVMR, (void **)&pQueryInterfaceIVMR, "QueryInterface");
	// 4: AddRef
	// 8: Release
}

HRESULT WINAPI extQueryInterfaceIVMR(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("IVMRMonitorConfig::QueryInterface", pQueryInterfaceIVMR, obj, riid, obp); }

// ----- IMediaEventEx -----

// MIDL_INTERFACE("56a868c0-0ad4-11ce-b03a-0020af0ba770")
// Beware: for some mysterious reason, the object hook through redirection doesn't work!!
// leave #if 0 until found and fixed.

#if 0
void HookMediaEventEx(IMediaEventEx **obj)
{
	OutTraceDSHOW("dxwnd.HookMediaEventEx: obj=%#x\n", *obj);
	dxwMediaEventEx *meex = new(dxwMediaEventEx);
	meex->Hook(*obj);
	*obj = (IMediaEventEx *)meex;
}
#else
QueryInterface_Type pQueryInterfaceMEX;
HRESULT WINAPI extQueryInterfaceMEX(void *, REFIID, LPVOID *);

void HookMediaEventEx(IMediaEventEx **arg)
{ 
	LPVOID obj = (LPVOID)arg;
	OutTraceDSHOW("dxwnd.HookMediaEventEx: obj=%#x\n", obj);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceMEX, (void **)&pQueryInterfaceMEX, "QueryInterface");
	// 1: AddRef
	// 2: Release
}

HRESULT WINAPI extQueryInterfaceMEX(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("IMediaEventEx::QueryInterface", pQueryInterfaceMEX, obj, riid, obp); }
#endif

#if 0
void HookMediaEvent(IMediaEvent **obj)
{
	OutTraceDSHOW("dxwnd.HookMediaEvent: obj=%#x\n", *obj);
	dxwMediaEvent *me = new(dxwMediaEvent);
	me->Hook(*obj);
	*obj = (IMediaEvent *)me;
}
#else
QueryInterface_Type pQueryInterfaceME;
HRESULT WINAPI extQueryInterfaceME(void *, REFIID, LPVOID *);

void HookMediaEvent(IMediaEvent **arg)
{ 
	LPVOID obj = (LPVOID)arg;
	OutTraceDSHOW("dxwnd.HookMediaEvent: obj=%#x\n", obj);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceME, (void **)&pQueryInterfaceME, "QueryInterface");
	// 1: AddRef
	// 2: Release
}

HRESULT WINAPI extQueryInterfaceME(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("IMediaEvent::QueryInterface", pQueryInterfaceME, obj, riid, obp); }
#endif

QueryInterface_Type pQueryInterfaceMS;
HRESULT WINAPI extQueryInterfaceMS(void *, REFIID, LPVOID *);

void HookMediaSeeking(IMediaSeeking **pms)
{
	LPVOID obj = (LPVOID)pms;
	OutTraceDSHOW("dxwnd.HookMediaSeeking: obj=%#x\n", obj);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceMS, (void **)&pQueryInterfaceMS, "QueryInterface");
	// 1: AddRef
	// 2: Release
}

HRESULT WINAPI extQueryInterfaceMS(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("HookMediaSeeking::QueryInterface", pQueryInterfaceMS, obj, riid, obp); }

QueryInterface_Type pQueryInterfaceTI;
HRESULT WINAPI extQueryInterfaceTI(void *, REFIID, LPVOID *);

void HookTypeInfo(ITypeInfo **ti)
{
	LPVOID obj = (LPVOID)ti;
	OutTraceDSHOW("dxwnd.HookTypeInfo: obj=%#x\n", obj);
	SetHook((void *)(**(DWORD **)obj), extQueryInterfaceTI, (void **)&pQueryInterfaceTI, "QueryInterface");
	// 1: AddRef
	// 2: Release
}

HRESULT WINAPI extQueryInterfaceTI(void *obj, REFIID riid, LPVOID *obp)
{ return QueryInterfaceDS("HookTypeInfo::QueryInterface", pQueryInterfaceTI, obj, riid, obp); }

// still to hook fully:
// IMFVideoDisplayControl
// IMFVideoRenderer ??
// IMFVideoPresenter ??

