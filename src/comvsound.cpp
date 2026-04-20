#define  _CRT_SECURE_NO_WARNINGS

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhelper.h"
#define GUID_ALLOCATE
#include "comvsound.h"
#include "stdio.h"

#define DXW_CREATE_COMMON_BUFFER

UINT gnEvents = 0;
LPDSBPOSITIONNOTIFY gpEvents = NULL;
BOOL gbEvents;

extern char *ExplainDSBFlags(DWORD, char *, int);

#define FDS_DEFAULT_DELAY 20

typedef struct {
	DWORD bps;
	DWORD size;
	BOOL looping;
} NOTIFIFICATIONARGS;

NOTIFIFICATIONARGS NotificationArgs;

static DWORD WINAPI FakeNotifyCallback(LPVOID lpParameter)
{
	NOTIFIFICATIONARGS *n = (NOTIFIFICATIONARGS *)lpParameter;
	ULONG bps = n->bps;
	ULONG size = n->size;
	BOOL looping = n->looping;
	OutTraceSND("dxwnd.FakeNotifyLooping: bps=%d size=%d\n", bps, size);
	while(TRUE){
		Sleep((gpEvents[0].dwOffset * 1000) / bps);
		for(UINT i=0; i<gnEvents-1; i++){
			OutTraceSND(">> event#%d SetEvent(%#x)\n", i, gpEvents[i].hEventNotify);
			if(gbEvents) SetEvent(gpEvents[i].hEventNotify);
			Sleep(((gpEvents[i+1].dwOffset - gpEvents[i].dwOffset) * 1000) / bps);
		}
		int last = gnEvents-1;
		OutTraceSND(">> event#%d SetEvent(%#x)\n", last, gpEvents[last].hEventNotify);
		if(gbEvents) SetEvent(gpEvents[last].hEventNotify);
		Sleep(((size - gpEvents[last].dwOffset) * 1000) / bps);
		if(!looping) break;
	} 
	ExitThread(0);
	return 0;
}

#ifdef DXW_CREATE_COMMON_BUFFER
LPVOID MakeCommonSpace(ULONG size)
{
	ApiName("dxwnd.MakeCommonSpace");
	static HANDLE hHeap = NULL;
	static ULONG HeapSize = 0;
	static LPVOID address;
	if(size == 0) return NULL;
	if(!hHeap){
		hHeap = HeapCreate(0, size, 0);
		if(!hHeap) OutTrace("%s: HeapCreate ERROR err=%d\n", ApiRef, GetLastError());
		address = HeapAlloc(hHeap, 0, size); 
		if(!address) OutTrace("%s: HeapAlloc ERROR err=%d\n", ApiRef, GetLastError());
		HeapSize = size;
	}
	if(size > HeapSize){
		address = HeapReAlloc(hHeap, 0, address, size); 
		if(!address) OutTrace("%s: HeapReAlloc ERROR err=%d\n", ApiRef, GetLastError());
		HeapSize = size;
		OutTraceSND("%s: address=%#x size=%d\n", ApiRef, address, size);
	}
	return address;
}
#endif

static HRESULT dsQueryInterface(ApiArg, LPVOID obj, REFIID riid, void **ppv)
{
	OutTraceSND("%s: obj=%#x guid=%s\n", ApiRef, obj, ExplainGUID((GUID *)&riid));
	
	switch(riid.Data1){
		case 0x279afa83: { // IDirectSound
				IFakeDirectSound *FakeDSound = new(IFakeDirectSound);
				*(LPDIRECTSOUND *)ppv = (LPDIRECTSOUND)FakeDSound;
				OutTraceSND("%s: created virtual DirectSound ppDS=%#x\n", ApiRef, FakeDSound);
				return DS_OK;	
			}
			break;

		case 0xC50A7E93: { // IDirectSound8
				IFakeDirectSound8 *FakeDS8 = new(IFakeDirectSound8);
				*(LPDIRECTSOUND8 *)ppv = (LPDIRECTSOUND8)FakeDS8;
				OutTraceSND("%s: created virtual DirectSound8 ppDSBuffer=%#x\n", ApiRef, FakeDS8);
				return DS_OK;	
			}
			break;
		case 0x279AFA85: { // IDirectSoundBuffer
				FAKEBUFFERSTATUS s;
				IFakeDirectSoundBuffer *FakeDSoundBuffer = new(IFakeDirectSoundBuffer);
				*(LPDIRECTSOUNDBUFFER *)ppv = (LPDIRECTSOUNDBUFFER)FakeDSoundBuffer;
				((IFakeDirectSoundBuffer *)obj)->getBufferStatus(&s);
				IFakeDirectSoundBuffer *pbb = *(IFakeDirectSoundBuffer **)ppv;
				pbb->setBufferStatus(s);
				OutTraceSND("%s: created virtual DirectSoundBuffer ppDSBuffer=%#x addr=%#x size=%d flags=%#x\n", 
					ApiRef, FakeDSoundBuffer, s.Address, s.Size, s.Flags);
				return DS_OK;	
			}
			break;
		case 0x6825a449: { // IID_IDirectSoundBuffer8
				FAKEBUFFERSTATUS s;
				IFakeDirectSoundBuffer8 *FakeDSoundBuffer8 = new(IFakeDirectSoundBuffer8);
				*(LPDIRECTSOUNDBUFFER8 *)ppv = (LPDIRECTSOUNDBUFFER8)FakeDSoundBuffer8;
				((IFakeDirectSoundBuffer *)obj)->getBufferStatus(&s);
				IFakeDirectSoundBuffer8 *pbb8 = *(IFakeDirectSoundBuffer8 **)ppv;
				pbb8->setBufferStatus(s);
				OutTraceSND("%s: created virtual DirectSoundBuffer8 ppDSBuffer8=%#x addr=%#x size=%d flags=%#x\n", 
					ApiRef, FakeDSoundBuffer8, s.Address, s.Size, s.Flags);
				return DS_OK;	
			}
			break;
		case 0xb0210783: { // IDirectSoundNotify
				IFakeDirectSoundNotify *IFakeNotify = new(IFakeDirectSoundNotify);
				*(LPDIRECTSOUNDNOTIFY *)ppv = (LPDIRECTSOUNDNOTIFY)IFakeNotify;
				OutTraceSND("%s: created virtual DirectSoundNotify ppDSBuffer=%#x\n", ApiRef, IFakeNotify);
				return DS_OK;
			}
			break;
		case 0x279AFA86: { // IID_IDirectSound3DBuffer a.k.a IID_IDirectSound3DBuffer8
				IFakeDirectSound3DBuffer *IFake3DBuffer = new(IFakeDirectSound3DBuffer);
				*(LPDIRECTSOUNDNOTIFY *)ppv = (LPDIRECTSOUNDNOTIFY)IFake3DBuffer;
				OutTraceSND("%s: created virtual DirectSound3DBuffer ppDS3DBuffer=%#x\n", ApiRef, IFake3DBuffer);
				return DS_OK;
			}
			break;
		case 0x279afa84: { // IID_IDirectSound3DListener
			IFakeDirectSound3DListener *IFake3DListener = new(IFakeDirectSound3DListener);
				*(LPDIRECTSOUND3DLISTENER *)ppv = (LPDIRECTSOUND3DLISTENER)IFake3DListener;
				OutTraceSND("%s: created virtual DirectSound3DListener ppDS3DListener=%#x\n", ApiRef, IFake3DListener);
				return DS_OK;
			}
			break;
		case 0x31efac30: { // IID_IKsPropertySet
				IFakeIKsPropertySet *IFakePropertySet = new(IFakeIKsPropertySet);
				*ppv = IFakePropertySet;
				OutTraceSND("%s: created virtual IKsPropertySet ppDSPropertySet=%#x\n", ApiRef, IFakePropertySet);
				return DS_OK;
			}
			break;
	}
	*ppv = NULL;
	OutTraceSND("%s: riid.Data1=%#x UNSUPPORTED\n", ApiRef, riid.Data1);
	return DSERR_UNSUPPORTED; // others
}

// ========== IFakeDirectSound ==========

IFakeDirectSound::IFakeDirectSound() 
{
	OutTraceSND("IFakeDirectSound constructor: ds=%#x\n", this);
	m_Ref = 1;
}

HRESULT IFakeDirectSound::QueryInterface(REFIID riid, void **ppv)
{
	ApiName("IFakeDirectSound::QueryInterface");
	return dsQueryInterface(ApiRef, this, riid, ppv);
}

ULONG IFakeDirectSound::AddRef(void)
{
	ApiName("IFakeDirectSound::AddRef");
	m_Ref++;
	OutTraceSND("%s: lpds=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

ULONG IFakeDirectSound::Release(void)
{
	ApiName("IFakeDirectSound::Release");
	if(m_Ref) m_Ref--;
	OutTraceSND("%s: lpds=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

static DWORD getsize(LPCDSBUFFERDESC pcDSBufferDesc)
{
	DWORD dwSize;
	hookSemaphore = TRUE;
  __try  { dwSize = pcDSBufferDesc->dwSize; }
  __except(EXCEPTION_EXECUTE_HANDLER) { dwSize = 0; };
  	hookSemaphore = FALSE;
  return dwSize;
}

HRESULT IFakeDirectSound::CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) 
{
	ApiName("IFakeDirectSound::CreateSoundBuffer");
	DWORD dwSize = getsize(pcDSBufferDesc);
#ifndef DXW_NOTRACES
	if(IsTraceSND){
		char sBuf[128];
		if(dwSize < sizeof(DSBUFFERDESC1)){
			// @#@ "Rockman 8 FC" in early hook phase creates buffers with a bad buffer descriptor
			OutTraceSND("%s: lpds=%#x ppdsb=%#x desc={size=%d}\n", ApiRef, this, ppDSBuffer, dwSize);
		}
		else {
			OutTrace("%s: lpds=%#x ppdsb=%#x desc={size=%d flags=%#x(%s) BufferBytes=%d lpwfxFormat=%#x reserved=%#x}\n", 
				ApiRef, this, ppDSBuffer, 
				pcDSBufferDesc->dwSize,
				pcDSBufferDesc->dwFlags, ExplainDSBFlags(pcDSBufferDesc->dwFlags, sBuf, 128),
				pcDSBufferDesc->dwBufferBytes,
				pcDSBufferDesc->lpwfxFormat,
				pcDSBufferDesc->dwReserved);
			if(pcDSBufferDesc->lpwfxFormat){
				WAVEFORMATEX *p = pcDSBufferDesc->lpwfxFormat;
				OutTrace("%s: lpwfxFormat:\n", ApiRef);
				OutTrace("> wFormatTag: %#x\n", p->wFormatTag);
				OutTrace("> nChannels: %d\n", p->nChannels);
				OutTrace("> nSamplesPerSec: %d\n", p->nSamplesPerSec);
				OutTrace("> nAvgBytesPerSec: %d\n", p->nAvgBytesPerSec);
				OutTrace("> nBlockAlign: %d\n", p->nBlockAlign);
				OutTrace("> wBitsPerSample: %d\n", p->wBitsPerSample);
				OutTrace("> cbSize: %d\n", p->cbSize);
			}
			// dump guid3DAlgorithm if DIRECTSOUND_VERSION >= 0x0700
			if((pcDSBufferDesc->dwFlags & DSBCAPS_CTRL3D) && (pcDSBufferDesc->dwSize == sizeof(DSBUFFERDESC))){
				OutTraceSND("> 3Dalgorithm: %s\n", sGUID((GUID *)&(pcDSBufferDesc->guid3DAlgorithm)));
			}
		}
	}
#endif // DXW_NOTRACES

	ULONG bps = 0;
	ULONG size = 0;
	if((dwSize >= sizeof(DSBUFFERDESC1)) && !(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)){
		size = pcDSBufferDesc->dwBufferBytes;
		if(pcDSBufferDesc->lpwfxFormat) bps = pcDSBufferDesc->lpwfxFormat->nAvgBytesPerSec;
		if((size < DSBSIZE_MIN) || (size > DSBSIZE_MAX)){
			OutTraceSND("%s: invalid size=%d ret=DSERR_INVALIDPARAM\n", ApiRef, size);
			return DSERR_INVALIDPARAM;
		}
	}
	IFakeDirectSoundBuffer *FakeBuffer = new(IFakeDirectSoundBuffer)(size, bps);
	*ppDSBuffer = (LPDIRECTSOUNDBUFFER)FakeBuffer;
	OutTraceSND("%s: created virtual DirectSoundBuffer ppDSBuffer=%#x\n", ApiRef, FakeBuffer);
	return DS_OK;
}

static 	DSCAPS fakeCaps = {
		sizeof(DSCAPS),
		DSCAPS_CERTIFIED | DSCAPS_PRIMARY16BIT | DSCAPS_PRIMARY8BIT | DSCAPS_PRIMARYMONO | DSCAPS_PRIMARYSTEREO |
		DSCAPS_SECONDARY16BIT | DSCAPS_SECONDARY8BIT | DSCAPS_SECONDARYMONO | DSCAPS_SECONDARYSTEREO | DSCAPS_CONTINUOUSRATE,
		100, 200000, // dwMinSecondarySampleRate, dwMaxSecondarySampleRate
		1, // dwPrimaryBuffers
		1, // dwMaxHwMixingAllBuffers: Number of buffers that can be mixed in hardware. This member can be less than the sum of dwMaxHwMixingStaticBuffers and dwMaxHwMixingStreamingBuffers. Resource tradeoffs frequently occur.
		1, // dwMaxHwMixingStaticBuffers: Maximum number of static buffers.
		1, // dwMaxHwMixingStreamingBuffers: Maximum number of streaming sound buffers.
		0, // dwFreeHwMixingAllBuffers: Number of unallocated buffers. On WDM drivers, this includes dwFreeHw3DAllBuffers.
		0, // dwFreeHwMixingStaticBuffers: Number of unallocated static buffers.
		0, // dwFreeHwMixingStreamingBuffers: Number of unallocated streaming buffers.
		0, // dwMaxHw3DAllBuffers: Maximum number of 3D buffers.
		0, // dwMaxHw3DStaticBuffers: Maximum number of static 3D buffers.
		0, // dwMaxHw3DStreamingBuffers: Maximum number of streaming 3D buffers.
		0, // dwFreeHw3DAllBuffers: Number of unallocated 3D buffers.
		0, // dwFreeHw3DStaticBuffers: Number of unallocated static 3D buffers.
		0, // dwFreeHw3DStreamingBuffers: Number of unallocated streaming 3D buffers.
		0, // 1024*1024*40, // dwTotalHwMemBytes: Size, in bytes, of the amount of memory on the sound card that stores static sound buffers.
		0, // 1024*1024*40, // dwFreeHwMemBytes: Size, in bytes, of the free memory on the sound card.
		0, // 1024*1024*40, // dwMaxContigFreeHwMemBytes: Size, in bytes, of the largest contiguous block of free memory on the sound card.
		0, // 1024, // dwUnlockTransferRateHwBuffers
		0, // dwPlayCpuOverheadSwBuffers
		0, // dwReserved1
		0, // dwReserved2
	};

HRESULT IFakeDirectSound::GetCaps(LPDSCAPS pDSCaps)
{
	ApiName("IFakeDirectSound::GetCaps");
	OutTraceSND("%s: lpds=%#x pCaps=%#x\n", ApiRef, this, pDSCaps);
	if(!pDSCaps) return DSERR_INVALIDPARAM;
	*pDSCaps = fakeCaps;
	return DS_OK;
}

HRESULT IFakeDirectSound::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate)
{
	ApiName("IFakeDirectSound::DuplicateSoundBuffer");
	OutTraceSND("%s: lpds=%#x orig=%#x\n", ApiRef, this, pDSBufferOriginal);
	ULONG size = ((IFakeDirectSoundBuffer *)pDSBufferOriginal)->m_Size;
	ULONG bps =  ((IFakeDirectSoundBuffer *)pDSBufferOriginal)->m_BytesPerSecond;
	*ppDSBufferDuplicate = new IFakeDirectSoundBuffer(size, bps);
	OutTraceSND("%s: new=%#x\n", ApiRef, *ppDSBufferDuplicate);
	return DS_OK;
}

HRESULT IFakeDirectSound::SetCooperativeLevel(HWND hwnd, DWORD dwLevel)
{
	ApiName("IFakeDirectSound::SetCooperativeLevel");
	OutTraceSND("%s: lpds=%#x hwnd=%#x level=%d\n", ApiRef, this, hwnd, dwLevel);
	return DS_OK;
}

HRESULT IFakeDirectSound::Compact()
{
	ApiName("IFakeDirectSound::Compact");
	OutTraceSND("%s: lpds=%#x\n", ApiRef, this);
	return DS_OK;
}

HRESULT IFakeDirectSound::GetSpeakerConfig(LPDWORD pdwSpeakerConfig)
{
	ApiName("IFakeDirectSound::GetSpeakerConfig");
	OutTraceSND("%s: lpds=%#x\n", ApiRef, this);
	return DS_OK;
}

HRESULT IFakeDirectSound::SetSpeakerConfig(DWORD dwSpeakerConfig) 
{
	ApiName("IFakeDirectSound::SetSpeakerConfig");
	OutTraceSND("%s: lpds=%#x\n", ApiRef, this);
	return DS_OK;
}

HRESULT IFakeDirectSound::Initialize(LPCGUID pcGuidDevice)
{
	ApiName("IFakeDirectSound::Initialize");
	OutTraceSND("%s: lpds=%#x\n", ApiRef, this);
	return DS_OK;
}

HRESULT IFakeDirectSound8::VerifyCertification(LPDWORD pdwCertified)
{
	ApiName("IFakeDirectSound::VerifyCertification");
	OutTraceSND("%s: lpds=%#x\n", ApiRef, this);
	return DS_OK;
}

// ========== IFakeDirectSoundBuffer ==========

IFakeDirectSoundBuffer::IFakeDirectSoundBuffer()
{
	new IFakeDirectSoundBuffer(0, 0);
}

IFakeDirectSoundBuffer::IFakeDirectSoundBuffer(ULONG size, ULONG bps) 
{
	m_Ref = 1;
	m_Locked = 0;
	m_Write = size / 2;
	m_Play = 0;
	m_Size = size;
	m_Delta = size / 10;
	m_BytesPerSecond = bps;
	m_Volume = 0;
	m_Pan = 0;
	m_NotifyThread = NULL;
#ifdef DXW_CREATE_COMMON_BUFFER
	m_Address = 0;
	if(size) m_Address = MakeCommonSpace(size);
#endif
	OutTraceSND("IFakeDirectSoundBuffer constructor: lpdsb=%#x size=%d bps=%d addr=%#x\n", this, size, bps, m_Address);
}

void IFakeDirectSoundBuffer::getBufferStatus(LPFAKEBUFFERSTATUS s)
{
	s->Address = m_Address;
	s->Size = m_Size;
	s->BytesPerSecond = m_BytesPerSecond;
	s->Flags = m_Flags;
	s->hHeap = m_hHeap;
	s->NotifyThread = m_NotifyThread;
}

void IFakeDirectSoundBuffer::setBufferStatus(FAKEBUFFERSTATUS s)
{
	m_Address = s.Address;
	m_Size = s.Size;
	m_BytesPerSecond = s.BytesPerSecond;
	m_Flags = s.Flags;
	m_hHeap = s.hHeap;
	m_NotifyThread = s.NotifyThread;
}

HRESULT IFakeDirectSoundBuffer::QueryInterface(REFIID riid, void **ppv)
{
	ApiName("IFakeDirectSoundBuffer::QueryInterface");
	return dsQueryInterface(ApiRef, this, riid, ppv);
}

ULONG IFakeDirectSoundBuffer::AddRef(void)
{
	ApiName("IFakeDirectSoundBuffer::AddRef");
	m_Ref++;
	OutTraceSND("%s: lpdsb=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

ULONG IFakeDirectSoundBuffer::Release(void)
{
	ApiName("IFakeDirectSoundBuffer::Release");
	if(m_Ref) m_Ref--;
	OutTraceSND("%s: lpdsb=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

HRESULT IFakeDirectSoundBuffer::GetCaps(LPDSBCAPS pDSBufferCaps)
{
	ApiName("IFakeDirectSoundBuffer::GetCaps");
	OutTraceSND("%s: lpdsb=%#x lpCaps=%#x\n", ApiRef, this, pDSBufferCaps);
	if(pDSBufferCaps){
		pDSBufferCaps->dwSize = sizeof(DSBCAPS);
		pDSBufferCaps->dwFlags = m_Flags | DSBCAPS_LOCHARDWARE;
		pDSBufferCaps->dwPlayCpuOverhead = 0; // ??
		pDSBufferCaps->dwUnlockTransferRate = 0; // ??
		pDSBufferCaps->dwBufferBytes = m_Size;
		return (HRESULT)DS_OK;
	}
	else {
		return (HRESULT)DSERR_INVALIDPARAM;
	}
}

HRESULT IFakeDirectSoundBuffer::GetCurrentPosition(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
{
	ApiName("IFakeDirectSoundBuffer::GetCurrentPosition");
	m_Play += m_Delta;
	m_Play %= m_Size;
	m_Write += m_Delta;
	m_Write %= m_Size;
	if(pdwCurrentPlayCursor) *pdwCurrentPlayCursor = m_Play;
	if(pdwCurrentWriteCursor) *pdwCurrentWriteCursor = m_Write;
	OutTraceSND("%s: lpdsb=%#x delta=%d cursors play=%#x write=%#x\n", ApiRef, this, m_Delta,
		pdwCurrentPlayCursor ? *pdwCurrentPlayCursor : 0, 
		pdwCurrentWriteCursor ? *pdwCurrentWriteCursor : 0);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{
	ApiName("IFakeDirectSoundBuffer::GetFormat");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::GetVolume(LPLONG plVolume)
{
	ApiName("IFakeDirectSoundBuffer::GetVolume");
	if(plVolume){
		*plVolume = m_Volume;
		OutTraceSND("%s: lpdsb=%#x volume=%d\n", ApiRef, this, *plVolume);
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: lpdsb=%#x ERROR DSERR_INVALIDPARAM\n", ApiRef, this);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT IFakeDirectSoundBuffer::GetPan(LPLONG plPan)
{
	ApiName("IFakeDirectSoundBuffer::GetPan");
	if(plPan){
		*plPan = m_Pan;
		OutTraceSND("%s: lpdsb=%#x pan=%d\n", ApiRef, this, *plPan);
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: lpdsb=%#x ERROR DSERR_INVALIDPARAM\n", ApiRef, this);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT IFakeDirectSoundBuffer::GetFrequency(LPDWORD pdwFrequency)
{
	ApiName("IFakeDirectSoundBuffer::GetFrequency");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::GetStatus(LPDWORD pdwStatus)
{
	ApiName("IFakeDirectSoundBuffer::GetStatus");
	DWORD status;
	// @#@ "Rapanui" waits for the play completion sampling the GetStatus return code
	// @#@ "Irisu Syndrome" waits for the play start sampling the GetStatus return code
	if(pdwStatus){
		if(m_Playing){
			// pretend an infinite play speed, a non-looping play is always immediately consumed.
			// beware: a finished buffer has no dedicated macro, the status value is 0, not to
			// be confused with DSBSTATUS_TERMINATED
			status = m_Looping ? (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING) : 0; 
		}
		else {
			status = 0;
		}
		*pdwStatus = status;
		OutTraceSND("%s: lpdsb=%#x pdwStatus=%#x\n", ApiRef, this, pdwStatus ? *pdwStatus : 0);
		return DS_OK;
	}
	else {
		OutTraceSND("%s: lpdsb=%#x INVALIDPARAM\n", ApiRef, this);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT IFakeDirectSoundBuffer::Initialize(LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc)
{
	ApiName("IFakeDirectSoundBuffer::Initialize");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::Lock(DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
	ApiName("IFakeDirectSoundBuffer::Lock");

#ifndef DXW_NOTRACES
	extern char *ExplainDSLockFlags(DWORD, char *, int);
	char sBuf[81];
	OutTraceSND("%s: lpdsb=%#x offset=%#x bytes=%d flags=%#x(%s)\n", ApiRef, this, dwOffset, dwBytes, dwFlags, ExplainDSLockFlags(dwFlags, sBuf, 80));
#endif // DXW_NOTRACES
	OutTraceSND("> ptr1=%#x bytes1=%#x ptr2=%#x bytes2=%#x\n", ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);

#ifdef DXW_CREATE_COMMON_BUFFER
	if(m_Size < dwBytes){
		// locking a primary buffer, need to allocate some memory if necessary
		m_Address = MakeCommonSpace(dwBytes);
		m_Size = dwBytes;
	}
#endif
	if(ppvAudioPtr1 && pdwAudioBytes1){
#ifdef DXW_CREATE_COMMON_BUFFER
		if(dwFlags & DSBLOCK_FROMWRITECURSOR) {
			// to be done ...
			*ppvAudioPtr1 = m_Address;
			*pdwAudioBytes1 = dwBytes;
		}
		else if(dwFlags & DSBLOCK_ENTIREBUFFER){
			*ppvAudioPtr1 = m_Address;
			*pdwAudioBytes1 = m_Size;
		}
		else {
			*ppvAudioPtr1 = (LPVOID)((LPBYTE)m_Address + (dwOffset % m_Size));
			if((dwOffset + dwBytes) <= m_Size){
				*pdwAudioBytes1 = dwBytes;
			}
			else {
				*pdwAudioBytes1 = m_Size - dwOffset;
			}
		}
#else
		*ppvAudioPtr1 = 0;
		*pdwAudioBytes1 = 0;
#endif
	}
	if(ppvAudioPtr2 && pdwAudioBytes2){
#ifdef DXW_CREATE_COMMON_BUFFER
		if(dwFlags & DSBLOCK_FROMWRITECURSOR) {
			// to be done ...
			*ppvAudioPtr2 = NULL;
			*pdwAudioBytes2 = 0;
		}
		else if(dwFlags & DSBLOCK_ENTIREBUFFER){
			*ppvAudioPtr2 = NULL;
			*pdwAudioBytes2 = 0;
		}
		else {
			if((dwOffset + dwBytes) <= m_Size){
				*ppvAudioPtr2 = NULL;
				*pdwAudioBytes2 = 0;
			}
			else {
				*ppvAudioPtr2 = m_Address;
				*pdwAudioBytes2 = dwBytes - (m_Size - dwOffset);
			}
		}
#else
		*ppvAudioPtr2 = 0;
		*pdwAudioBytes2 = 0;
#endif
	}
	m_Locked = dwBytes;
	OutTraceSND("%s: lpdsb=%#x ptr1=%#x bytes1=%d ptr2=%#x bytes2=%d\n", ApiRef, this,
		ppvAudioPtr1 ? *ppvAudioPtr1 : 0, 
		pdwAudioBytes1 ? *pdwAudioBytes1 : 0, 
		ppvAudioPtr2 ? *ppvAudioPtr2 : 0, 
		pdwAudioBytes2 ? *pdwAudioBytes2: 0);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
{
	ApiName("IFakeDirectSoundBuffer::Play");
	OutTraceSND("%s: lpdsb=%#x pri=%d flags=%#x\n", ApiRef, this, dwPriority, dwFlags);
	m_Looping = dwFlags & DSBPLAY_LOOPING;
	m_Playing = TRUE;
	if((gnEvents > 0) && (m_BytesPerSecond > 0) && (m_Size > 0)){
		NotificationArgs.bps = m_BytesPerSecond;
		NotificationArgs.size = m_Size;
		NotificationArgs.looping = m_Looping;
		gbEvents = TRUE;
		m_NotifyThread = CreateThread(NULL, 0, FakeNotifyCallback, (LPVOID)&NotificationArgs, 0, NULL);
	}
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::SetCurrentPosition(DWORD dwNewPosition)
{
	ApiName("IFakeDirectSoundBuffer::SetCurrentPosition");
	OutTraceSND("%s: lpdsb=%#x position=%#x\n", ApiRef, this, dwNewPosition);
	m_Play = dwNewPosition;
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::SetFormat(LPCWAVEFORMATEX pcfxFormat)
{
	ApiName("IFakeDirectSoundBuffer::SetFormat");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::SetVolume(LONG lVolume)
{
	ApiName("IFakeDirectSoundBuffer::SetVolume");
	OutTraceSND("%s: lpdsb=%#x volume=%d\n", ApiRef, this, lVolume);
	m_Volume = lVolume;
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::SetPan(LONG lPan)
{
	ApiName("IFakeDirectSoundBuffer::SetPan");
	OutTraceSND("%s: lpdsb=%#x pan=%d\n", ApiRef, this, lPan);
	m_Pan = lPan;
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::SetFrequency(DWORD dwFrequency)
{
	ApiName("IFakeDirectSoundBuffer::SetFrequency");
	OutTraceSND("%s: lpdsb=%#x freq=%d\n", ApiRef, this, dwFrequency);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::Stop()
{
	ApiName("IFakeDirectSoundBuffer::Stop");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	if(m_NotifyThread) {
		TerminateThread(m_NotifyThread, 0);
	}
	m_Playing = FALSE;
	gbEvents = FALSE;
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
	ApiName("IFakeDirectSoundBuffer::Unlock");
	OutTraceSND("%s: lpdsb=%#x ptr1=%#x bytes1=%d ptr2=%#x bytes2=%d\n", ApiRef, this, pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
	// do nothing !!
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer::Restore()
{
	ApiName("IFakeDirectSoundBuffer::Restore");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

// ========== IFakeDirectSoundBuffer8 ==========

IFakeDirectSoundBuffer8::IFakeDirectSoundBuffer8() 
{
	OutTraceSND("IFakeDirectSoundBuffer8 constructor: lpdsb8=%#x\n", this);
}

HRESULT IFakeDirectSoundBuffer8::SetFX(DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes)
{
	ApiName("IFakeDirectSoundBuffer8::SetFX");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer8::AcquireResources(DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes)
{
	ApiName("IFakeDirectSoundBuffer8::AcquireResources");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT IFakeDirectSoundBuffer8::GetObjectInPath(REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID *ppObject)
{
	ApiName("IFakeDirectSoundBuffer8::GetObjectInPath");
	OutTraceSND("%s: lpdsb=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

// ========== IFakeDirectSoundNotify ==========

IFakeDirectSoundNotify::IFakeDirectSoundNotify() 
{
	OutTraceSND("IFakeDirectSoundNotify constructor: lpdsn=%#x\n", this);
	m_Ref = 1;
}

HRESULT IFakeDirectSoundNotify::QueryInterface(REFIID riid, void **ppv)
{
	ApiName("IFakeDirectSoundNotify::QueryInterface");
	return dsQueryInterface(ApiRef, this, riid, ppv);
}

ULONG IFakeDirectSoundNotify::AddRef(void)
{
	ApiName("IFakeDirectSoundNotify::AddRef");
	m_Ref++;
	OutTraceSND("%s: lpdsn=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

ULONG IFakeDirectSoundNotify::Release(void)
{
	ApiName("IFakeDirectSoundNotify::Release");
	m_Ref--;
	OutTraceSND("%s: lpdsn=%#x ref=%d\n", ApiRef, this, m_Ref);
	if(m_Ref == 0) {
		gnEvents = 0;
		free(gpEvents); 
	}
	return m_Ref;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSoundNotify::SetNotificationPositions(DWORD dwPositionNotifies, LPCDSBPOSITIONNOTIFY pcPositionNotifies)
{
	ApiName("IFakeDirectSoundNotify::SetNotificationPositions");
	OutTraceSND("%s: lpdsn=%#x PositionNotifies=%d \n", ApiRef, this, dwPositionNotifies);
	for(UINT i=0; i<dwPositionNotifies; i++){
		OutTraceSND("[%d] > offset=%#x hEvent=%#x\n", i, pcPositionNotifies[i].dwOffset, pcPositionNotifies[i].hEventNotify);
	}
	gnEvents = dwPositionNotifies;
	gpEvents = (LPDSBPOSITIONNOTIFY)malloc(dwPositionNotifies * sizeof(DSBPOSITIONNOTIFY));
	for(UINT i=0; i<dwPositionNotifies; i++){
		gpEvents[i]=pcPositionNotifies[i];
	}
	return (HRESULT)DS_OK;
}

// ========== IFakeDirectSound3DBuffer ==========

IFakeDirectSound3DBuffer::IFakeDirectSound3DBuffer() 
{
	OutTraceSND("IFakeDirectSound3DBuffer constructor: lpds3Db=%#x\n", this);
	memset(&m_Parameters, 0, sizeof(DS3DBUFFER));
	m_Ref = 1;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::QueryInterface(REFIID riid, LPVOID *ppv)
{
	ApiName("IFakeDirectSound3DBuffer::QueryInterface");
	return dsQueryInterface(ApiRef, this, riid, ppv);
}

ULONG STDMETHODCALLTYPE IFakeDirectSound3DBuffer::AddRef()
{
	ApiName("IFakeDirectSound3DBuffer::AddRef");
	m_Ref++;
	OutTraceSND("%s: lpds3Db=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

ULONG STDMETHODCALLTYPE IFakeDirectSound3DBuffer::Release()
{
	ApiName("IFakeDirectSoundNotify::Release");
	m_Ref--;
	OutTraceSND("%s: lpds3Db=%#x ref=%d\n", ApiRef, this, m_Ref);
	//if(m_Ref == 0) this->Release();
	return m_Ref;
}

// IDirectSound3DBuffer methods
HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetAllParameters(LPDS3DBUFFER pDs3dBuffer)
{
	ApiName("IFakeDirectSoundNotify::GetAllParameters");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pDs3dBuffer){
		*pDs3dBuffer = m_Parameters;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pDs3dBuffer ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetConeAngles(LPDWORD pdwInsideConeAngle, LPDWORD pdwOutsideConeAngle)
{
	ApiName("IFakeDirectSoundNotify::GetConeAngles");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pdwOutsideConeAngle){
		*pdwOutsideConeAngle = m_Parameters.dwOutsideConeAngle;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pdwOutsideConeAngle ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetConeOrientation(D3DVECTOR* pvOrientation)
{
	ApiName("IFakeDirectSoundNotify::GetConeOrientation");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pvOrientation){
		*pvOrientation = m_Parameters.vConeOrientation;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pvOrientation ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetConeOutsideVolume(LPLONG plConeOutsideVolume)
{
	ApiName("IFakeDirectSoundNotify::GetConeOutsideVolume");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(plConeOutsideVolume){
		*plConeOutsideVolume = m_Parameters.lConeOutsideVolume;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL plConeOutsideVolume ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetMaxDistance(D3DVALUE* pflMaxDistance)
{
	ApiName("IFakeDirectSoundNotify::GetMaxDistance");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pflMaxDistance){
		*pflMaxDistance = m_Parameters.flMaxDistance;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pflMaxDistance ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetMinDistance(D3DVALUE* pflMinDistance)
{
	ApiName("IFakeDirectSoundNotify::GetMinDistance");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pflMinDistance){
		*pflMinDistance = m_Parameters.flMinDistance;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pflMinDistance ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetMode(LPDWORD pdwMode)
{
	ApiName("IFakeDirectSoundNotify::GetMode");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pdwMode){
		*pdwMode = m_Parameters.dwMode;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pdwMode ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetPosition(D3DVECTOR* pvPosition)
{
	ApiName("IFakeDirectSoundNotify::GetPosition");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pvPosition){
		*pvPosition = m_Parameters.vPosition;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pvPosition ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::GetVelocity(D3DVECTOR* pvVelocity)
{
	ApiName("IFakeDirectSoundNotify::GetVelocity");
	OutTraceSND("%s: lpds3Db=%#x\n", ApiRef, this);
	if(pvVelocity){
		*pvVelocity = m_Parameters.vVelocity;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pvVelocity ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetAllParameters(LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetAllParameters");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	if(pcDs3dBuffer){
		m_Parameters = *pcDs3dBuffer;
		return (HRESULT)DS_OK;
	}
	else {
		OutTraceSND("%s: NULL pcDs3dBuffer ret=DSERR_INVALIDPARAM\n", ApiRef);
		return DSERR_INVALIDPARAM;
	}
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetConeAngles(DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetConeAngles");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.dwInsideConeAngle = dwInsideConeAngle;
	m_Parameters.dwOutsideConeAngle = dwOutsideConeAngle;
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetConeOrientation(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetConeOrientation");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.vConeOrientation.x = x;
	m_Parameters.vConeOrientation.y = y;
	m_Parameters.vConeOrientation.z = z;
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetConeOutsideVolume(LONG lConeOutsideVolume, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetConeOutsideVolume");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.lConeOutsideVolume = lConeOutsideVolume;
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetMaxDistance(D3DVALUE flMaxDistance, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetMaxDistance");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.flMaxDistance = flMaxDistance;
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetMinDistance(D3DVALUE flMinDistance, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetMinDistance");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.flMinDistance = flMinDistance;
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetMode(DWORD dwMode, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetMode");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.dwMode = dwMode;
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetPosition(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetPosition");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.vPosition.x = x;
	m_Parameters.vPosition.y = y;
	m_Parameters.vPosition.z = z;
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DBuffer::SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
	ApiName("IFakeDirectSoundNotify::SetVelocity");
	OutTraceSND("%s: lpds3Db=%#x apply=%s\n", ApiRef, this, dwApply ? "DEFERRED" : "IMMEDIATE");
	m_Parameters.vVelocity.x = x;
	m_Parameters.vVelocity.y = y;
	m_Parameters.vVelocity.z = z;
	return (HRESULT)DS_OK;
}

// ========== IFakeDirectSound3DListener ==========

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::QueryInterface(REFIID riid, LPVOID *ppv)
{
	ApiName("IFakeDirectSound3DListener::QueryInterface");
	return dsQueryInterface(ApiRef, this, riid, ppv);
}

ULONG STDMETHODCALLTYPE IFakeDirectSound3DListener::AddRef()
{
	ApiName("IFakeDirectSound3DListener::AddRef");
	m_Ref++;
	OutTraceSND("%s: lpds3Dl=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

ULONG STDMETHODCALLTYPE IFakeDirectSound3DListener::Release()
{
	ApiName("IFakeDirectSound3DListener::Release");
	m_Ref--;
	OutTraceSND("%s: lpds3Dl=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::GetAllParameters(LPDS3DLISTENER pListener)
{
	ApiName("IFakeDirectSound3DListener::GetAllParameters");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::GetDistanceFactor(D3DVALUE* pflDistanceFactor)
{
	ApiName("IFakeDirectSound3DListener::GetDistanceFactor");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::GetDopplerFactor(D3DVALUE* pflDopplerFactor)
{
	ApiName("IFakeDirectSound3DListener::GetDopplerFactor");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::GetOrientation(D3DVECTOR* pvOrientFront, D3DVECTOR* pvOrientTop)
{
	ApiName("IFakeDirectSound3DListener::GetOrientation");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::GetPosition(D3DVECTOR* pvPosition)
{
	ApiName("IFakeDirectSound3DListener::GetPosition");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::GetRolloffFactor(D3DVALUE* pflRolloffFactor)
{
	ApiName("IFakeDirectSound3DListener::GetRolloffFactor");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::GetVelocity(D3DVECTOR* pvVelocity)
{
	ApiName("IFakeDirectSound3DListener::GetVelocity");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::SetAllParameters(LPCDS3DLISTENER pcListener, DWORD dwApply)
{
	ApiName("IFakeDirectSound3DListener::SetAllParameters");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::SetDistanceFactor(D3DVALUE flDistanceFactor, DWORD dwApply)
{
	ApiName("IFakeDirectSound3DListener::SetDistanceFactor");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::SetDopplerFactor(D3DVALUE flDopplerFactor, DWORD dwApply)
{
	ApiName("IFakeDirectSound3DListener::SetDopplerFactor");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::SetOrientation(D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront,
                                           D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop, DWORD dwApply)
{
	ApiName("IFakeDirectSound3DListener::SetOrientation");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::SetPosition(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
	ApiName("IFakeDirectSound3DListener::SetPosition");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::SetRolloffFactor(D3DVALUE flRolloffFactor, DWORD dwApply)
{
	ApiName("IFakeDirectSound3DListener::SetRolloffFactor");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply)
{
	ApiName("IFakeDirectSound3DListener::SetVelocity");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

HRESULT STDMETHODCALLTYPE IFakeDirectSound3DListener::CommitDeferredSettings()
{
	ApiName("IFakeDirectSound3DListener::CommitDeferredSettings");
	OutTraceSND("%s: lpds3Dl=%#x\n", ApiRef, this);
	return (HRESULT)DS_OK;
}

IFakeDirectSound3DListener::IFakeDirectSound3DListener() 
{
	OutTraceSND("IFakeDirectSound3DBuffer constructor: lpds3Dl=%#x\n", this);
	m_Ref = 1;
}

// ========== IFakeIKsPropertySet ==========

HRESULT IFakeIKsPropertySet::QueryInterface(REFIID riid, void **ppv)
{
	ApiName("IFakeIKsPropertySet::QueryInterface");
	//OutTraceSND("%s: lpps=%#x\n", ApiRef, this);
	return dsQueryInterface(ApiRef, this, riid, ppv);
}

ULONG IFakeIKsPropertySet::AddRef(void)
{
	ApiName("IFakeIKsPropertySet::AddRef");
	m_Ref++;
	OutTraceSND("%s: lpps=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

ULONG IFakeIKsPropertySet::Release(void)
{
	ApiName("IFakeIKsPropertySet::Release");
	if(m_Ref) m_Ref--;
	OutTraceSND("%s: lpps=%#x ref=%d\n", ApiRef, this, m_Ref);
	return m_Ref;
}

HRESULT IFakeIKsPropertySet::Get(void *, REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength, PULONG pulBytesReturned)
{
	ApiName("IFakeIKsPropertySet::Get");
	OutTraceSND("%s: lpps=%#x ref=%d\n", ApiRef, this, m_Ref);
	return (HRESULT)DS_OK;
}

HRESULT IFakeIKsPropertySet::Set(void *, REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength)
{
	ApiName("IFakeIKsPropertySet::Set");
	OutTraceSND("%s: lpps=%#x ref=%d\n", ApiRef, this, m_Ref);
	return (HRESULT)DS_OK;
}

HRESULT IFakeIKsPropertySet::QuerySupport(void *, REFGUID rguidPropSet, ULONG ulId, PULONG pulTypeSupport)
{
	ApiName("IFakeIKsPropertySet::QuerySupport");
	OutTraceSND("%s: lpps=%#x ref=%d\n", ApiRef, this, m_Ref);
	return (HRESULT)DS_OK;
}
