#include <windows.h>
#include <dsound.h>

typedef struct {
	LPVOID Address;
	ULONG Size;
	ULONG BytesPerSecond;
	DWORD Flags;
	HANDLE hHeap;
	HANDLE NotifyThread;
} FAKEBUFFERSTATUS, *LPFAKEBUFFERSTATUS;

interface IFakeDirectSound:IDirectSound
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // IDirectSound methods
    HRESULT STDMETHODCALLTYPE CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter);
    HRESULT STDMETHODCALLTYPE GetCaps(LPDSCAPS pDSCaps);
    HRESULT STDMETHODCALLTYPE DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate);
    HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND hwnd, DWORD dwLevel);
    HRESULT STDMETHODCALLTYPE Compact(void);
    HRESULT STDMETHODCALLTYPE GetSpeakerConfig(LPDWORD pdwSpeakerConfig);
    HRESULT STDMETHODCALLTYPE SetSpeakerConfig(DWORD dwSpeakerConfig);
    HRESULT STDMETHODCALLTYPE Initialize(LPCGUID pcGuidDevice);

	IFakeDirectSound();

protected:
	int m_Ref;
};

interface IFakeDirectSound8:IFakeDirectSound
{
public:
    HRESULT STDMETHODCALLTYPE VerifyCertification(LPDWORD pdwCertified);
};

interface IFakeDirectSoundBuffer: IDirectSoundBuffer
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IDirectSoundBuffer methods
    HRESULT STDMETHODCALLTYPE GetCaps(LPDSBCAPS pDSBufferCaps);
    HRESULT STDMETHODCALLTYPE GetCurrentPosition(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor);
    HRESULT STDMETHODCALLTYPE GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten);
    HRESULT STDMETHODCALLTYPE GetVolume(LPLONG plVolume);
    HRESULT STDMETHODCALLTYPE GetPan(LPLONG plPan);
    HRESULT STDMETHODCALLTYPE GetFrequency(LPDWORD pdwFrequency);
    HRESULT STDMETHODCALLTYPE GetStatus(LPDWORD pdwStatus);
    HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc);
    HRESULT STDMETHODCALLTYPE Lock(DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE SetCurrentPosition(DWORD dwNewPosition);
    HRESULT STDMETHODCALLTYPE SetFormat(LPCWAVEFORMATEX pcfxFormat);
    HRESULT STDMETHODCALLTYPE SetVolume(LONG lVolume);
    HRESULT STDMETHODCALLTYPE SetPan(THIS_ LONG lPan);
    HRESULT STDMETHODCALLTYPE SetFrequency(THIS_ DWORD dwFrequency);
    HRESULT STDMETHODCALLTYPE Stop();
    HRESULT STDMETHODCALLTYPE Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2);
    HRESULT STDMETHODCALLTYPE Restore();

	IFakeDirectSoundBuffer();
	IFakeDirectSoundBuffer(ULONG, ULONG);
	void getBufferStatus(LPFAKEBUFFERSTATUS);
	void setBufferStatus(FAKEBUFFERSTATUS);
	ULONG m_Size;
	ULONG m_BytesPerSecond;
	DWORD m_Flags;
	LPVOID m_Address;
	HANDLE m_hHeap;
	HANDLE m_NotifyThread;

protected:
	int m_Ref;
	ULONG m_Locked;
	ULONG m_Write;
	ULONG m_Play;
	ULONG m_Delta;
	BOOL m_Looping;
	BOOL m_Playing;
	LONG m_Volume;
	LONG m_Pan;
};

interface IFakeDirectSoundBuffer8: IFakeDirectSoundBuffer
{
public:
    HRESULT STDMETHODCALLTYPE SetFX(DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes);
    HRESULT STDMETHODCALLTYPE AcquireResources(DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes);
    HRESULT STDMETHODCALLTYPE GetObjectInPath(REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID *ppObject);

	IFakeDirectSoundBuffer8();
};

interface IFakeDirectSoundNotify: IDirectSoundNotify
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IDirectSoundNotify methods
    HRESULT STDMETHODCALLTYPE SetNotificationPositions(DWORD dwPositionNotifies, LPCDSBPOSITIONNOTIFY pcPositionNotifies);

	IFakeDirectSoundNotify();

protected:
	int m_Ref;
};

interface IFakeDirectSound3DBuffer: IDirectSound3DBuffer
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IDirectSound3DBuffer methods
    HRESULT STDMETHODCALLTYPE GetAllParameters(LPDS3DBUFFER pDs3dBuffer);
    HRESULT STDMETHODCALLTYPE GetConeAngles(LPDWORD pdwInsideConeAngle, LPDWORD pdwOutsideConeAngle);
    HRESULT STDMETHODCALLTYPE GetConeOrientation(D3DVECTOR* pvOrientation);
    HRESULT STDMETHODCALLTYPE GetConeOutsideVolume(LPLONG plConeOutsideVolume);
    HRESULT STDMETHODCALLTYPE GetMaxDistance(D3DVALUE* pflMaxDistance);
    HRESULT STDMETHODCALLTYPE GetMinDistance(D3DVALUE* pflMinDistance);
    HRESULT STDMETHODCALLTYPE GetMode(LPDWORD pdwMode);
    HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR* pvPosition);
    HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR* pvVelocity);
    HRESULT STDMETHODCALLTYPE SetAllParameters(LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetConeAngles(DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetConeOrientation(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetConeOutsideVolume(LONG lConeOutsideVolume, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetMaxDistance(D3DVALUE flMaxDistance, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetMinDistance(D3DVALUE flMinDistance, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetMode(DWORD dwMode, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetPosition(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
	
	IFakeDirectSound3DBuffer();

protected:
	int m_Ref;
	DS3DBUFFER m_Parameters;
};

interface IFakeDirectSound3DListener: IDirectSound3DListener{
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IDirectSound3DListener methods
    HRESULT STDMETHODCALLTYPE GetAllParameters(LPDS3DLISTENER pListener);
    HRESULT STDMETHODCALLTYPE GetDistanceFactor(D3DVALUE* pflDistanceFactor);
    HRESULT STDMETHODCALLTYPE GetDopplerFactor(D3DVALUE* pflDopplerFactor);
    HRESULT STDMETHODCALLTYPE GetOrientation(D3DVECTOR* pvOrientFront, D3DVECTOR* pvOrientTop);
    HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR* pvPosition);
    HRESULT STDMETHODCALLTYPE GetRolloffFactor(D3DVALUE* pflRolloffFactor);
    HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR* pvVelocity);
    HRESULT STDMETHODCALLTYPE SetAllParameters(LPCDS3DLISTENER pcListener, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetDistanceFactor(D3DVALUE flDistanceFactor, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetDopplerFactor(D3DVALUE flDopplerFactor, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetOrientation(THIS_ D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront,
                                               D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetPosition(THIS_ D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetRolloffFactor(D3DVALUE flRolloffFactor, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply);
    HRESULT STDMETHODCALLTYPE CommitDeferredSettings();

	IFakeDirectSound3DListener();

protected:
	int m_Ref;
};

interface IFakeIKsPropertySet {
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IKsPropertySet methods
    HRESULT STDMETHODCALLTYPE Get(void *, REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength, PULONG pulBytesReturned);
    HRESULT STDMETHODCALLTYPE Set(void *, REFGUID rguidPropSet, ULONG ulId, LPVOID pInstanceData, ULONG ulInstanceLength,
                                       LPVOID pPropertyData, ULONG ulDataLength);
    HRESULT STDMETHODCALLTYPE QuerySupport(void *, REFGUID rguidPropSet, ULONG ulId, PULONG pulTypeSupport);

protected:
	int m_Ref;
};

