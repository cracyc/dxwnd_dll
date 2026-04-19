#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "IMediaEventSink"

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

void dxwMediaEventSink::Hook(IMediaEventSink *obj)
{
	//OutTrace("Hook: obj=%#x\n", obj);
	m_this = obj;
}

HRESULT STDMETHODCALLTYPE dxwMediaEventSink::QueryInterface(const IID &riid, void **obp)
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

ULONG STDMETHODCALLTYPE dxwMediaEventSink::AddRef()
{
	ApiName("AddRef");
	ULONG res;
	res = m_this->AddRef();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwMediaEventSink::Release()
{
	ApiName("Release");
	ULONG res;
	res = m_this->Release();
	OutTraceDSHOW("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	//if(res == 0) this->Release();
	if(res == 0) this->~dxwMediaEventSink();
	return res;
}

char *sEventCode(long ec)
{
	char *ECList[] = 
	{	"COMPLETE", "USERABORT", "ERRORABORT", "TIME", "REPAINT", // 1 - 5
		"STREAM_ERROR_STOPPED", "STREAM_ERROR_STILLPLAYING", "ERROR_STILLPLAYING", "PALETTE_CHANGED", "VIDEO_SIZE_CHANGED", // 6 - 10
		"QUALITY_CHANGE", "SHUTTING_DOWN", "CLOCK_CHANGED", "PAUSED", "", // 11 - 15
		"OPENING_FILE", "BUFFERING_DATA", "FULLSCREEN_LOST", "ACTIVATE", "NEED_RESTART", // 16 - 20
		"WINDOW_DESTROYED", "DISPLAY_CHANGED", "STARVATION", "OLE_EVENT", "NOTIFY_WINDOW", // 21 - 25
		"STREAM_CONTROL_STOPPED", "STREAM_CONTROL_STARTED", "END_OF_SEGMENT", "SEGMENT_STARTED", "LENGTH_CHANGED", // 26 - 30
		"DEVICE_LOST", "SAMPLE_NEEDED", "PROCESSING_LATENCY", "SAMPLE_LATENCY", "SCRUB_TIME", // 31 - 35
		"STEP_COMPLETE", "", "", "", "", // 36 - 40
		"", "", "", "", "", // 41 - 45
		"", "", "TIMECODE_AVAILABLE", "EXTDEVICE_MODE_CHANGE", "STATE_CHANGE" // 46 - 50
	};

	//if((ec < 1) || (ec > EC_CODECAPI_EVENT)) return "???"; 
	if((ec < 1) || (ec > EC_STATE_CHANGE)) return "???"; 
	return ECList[ec];
}
HRESULT STDMETHODCALLTYPE dxwMediaEventSink::Notify(long EventCode, LONG_PTR EventParam1, LONG_PTR EventParam2)
{
	HRESULT res;
	ApiName("Notify");
	OutTraceDSHOW("%s: obj=%#x code=%d(%s) param1=%#x param2=%#x\n", ApiRef, m_this, EventCode, sEventCode(EventCode), EventParam1, EventParam2);

	res = m_this->Notify(EventCode, EventParam1, EventParam2);

	OutTraceDSHOW("%s: res=%#x\n", ApiRef, res);
	return res;
}