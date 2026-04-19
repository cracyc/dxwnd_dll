class dxwBaseFilter : public IBaseFilter
{
	public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &arg1,void **arg2);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);
		HRESULT STDMETHODCALLTYPE Stop(void);
		HRESULT STDMETHODCALLTYPE Pause(void);
		HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart);
		HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
		HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock);
	    HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock);

        HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins **ppEnum);
        HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, IPin **ppPin);
        HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO *pInfo);
        HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);
        HRESULT STDMETHODCALLTYPE QueryVendorInfo(LPWSTR *pVendorInfo);

		void Hook(IBaseFilter *);

	private:
		IBaseFilter *m_this;
};

class dxwEnumFilters : public IEnumFilters
{
    public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &arg1,void **arg2);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE Next(ULONG cFilters, IBaseFilter **ppFilter, ULONG *pcFetched);
		HRESULT STDMETHODCALLTYPE Skip(ULONG cFilters);
        HRESULT STDMETHODCALLTYPE Reset(void);
        HRESULT STDMETHODCALLTYPE Clone(IEnumFilters **ppEnum);
		void Hook(IEnumFilters *);
	private:
		IEnumFilters *m_this;
};

class dxwEnumPins : public IEnumPins
{
    public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &arg1,void **arg2);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
		HRESULT STDMETHODCALLTYPE Skip(ULONG cPins);
        HRESULT STDMETHODCALLTYPE Reset(void);
        HRESULT STDMETHODCALLTYPE Clone(IEnumPins **ppEnum);
		void Hook(IEnumPins *);
	private:
		IEnumPins *m_this;
};

class dxwFilterGraph : public IFilterGraph
{
    public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE AddFilter(IBaseFilter *pFilter, LPCWSTR pName);
        HRESULT STDMETHODCALLTYPE RemoveFilter(IBaseFilter *pFilter);
        HRESULT STDMETHODCALLTYPE EnumFilters(IEnumFilters **ppEnum);
        HRESULT STDMETHODCALLTYPE FindFilterByName(LPCWSTR pName, IBaseFilter **ppFilter);
        HRESULT STDMETHODCALLTYPE ConnectDirect(IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt);
        HRESULT STDMETHODCALLTYPE Reconnect(IPin *ppin);
        HRESULT STDMETHODCALLTYPE Disconnect(IPin *ppin);
        HRESULT STDMETHODCALLTYPE SetDefaultSyncSource(void);
        
		void Hook(IFilterGraph *);

	private:
		IFilterGraph *m_this;
};

//  MIDL_INTERFACE("36b73882-c2c8-11cf-8b46-00805f6cef60")

class dxwFilterGraph2 : public IFilterGraph2
{
    public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE AddFilter(IBaseFilter *pFilter, LPCWSTR pName);
        HRESULT STDMETHODCALLTYPE RemoveFilter(IBaseFilter *pFilter);
        HRESULT STDMETHODCALLTYPE EnumFilters(IEnumFilters **ppEnum);
        HRESULT STDMETHODCALLTYPE FindFilterByName(LPCWSTR pName, IBaseFilter **ppFilter);
        HRESULT STDMETHODCALLTYPE ConnectDirect(IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt);
        HRESULT STDMETHODCALLTYPE Reconnect(IPin *ppin);
        HRESULT STDMETHODCALLTYPE Disconnect(IPin *ppin);
        HRESULT STDMETHODCALLTYPE SetDefaultSyncSource(void);
		HRESULT STDMETHODCALLTYPE Connect(IPin *ppinOut, IPin *ppinIn);
		HRESULT STDMETHODCALLTYPE Render( IPin *ppinOut);
		HRESULT STDMETHODCALLTYPE RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList);
		HRESULT STDMETHODCALLTYPE AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter);
		HRESULT STDMETHODCALLTYPE SetLogFile(DWORD_PTR hFile);
		HRESULT STDMETHODCALLTYPE Abort(void);
		HRESULT STDMETHODCALLTYPE ShouldOperationContinue(void);
        HRESULT STDMETHODCALLTYPE AddSourceFilterForMoniker(IMoniker *pMoniker, IBindCtx *pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter);
        HRESULT STDMETHODCALLTYPE ReconnectEx(IPin *ppin, const AM_MEDIA_TYPE *pmt);
        HRESULT STDMETHODCALLTYPE RenderEx(IPin *pPinOut, DWORD dwFlags, DWORD *pvContext);
         
		void Hook(IFilterGraph2 *);

	private:
		IFilterGraph2 *m_this;
};

class dxwGraphBuilder : public IGraphBuilder
{
	public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE AddFilter(IBaseFilter *pFilter, LPCWSTR pName);
        HRESULT STDMETHODCALLTYPE RemoveFilter(IBaseFilter *pFilter);
        HRESULT STDMETHODCALLTYPE EnumFilters(IEnumFilters **ppEnum);
        HRESULT STDMETHODCALLTYPE FindFilterByName(LPCWSTR pName, IBaseFilter **ppFilter);
        HRESULT STDMETHODCALLTYPE ConnectDirect(IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt);
        HRESULT STDMETHODCALLTYPE Reconnect(IPin *ppin);
        HRESULT STDMETHODCALLTYPE Disconnect(IPin *ppin);
        HRESULT STDMETHODCALLTYPE SetDefaultSyncSource(void);
		HRESULT STDMETHODCALLTYPE Connect(IPin *ppinOut, IPin *ppinIn);
		HRESULT STDMETHODCALLTYPE Render( IPin *ppinOut);
		HRESULT STDMETHODCALLTYPE RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList);
		HRESULT STDMETHODCALLTYPE AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter);
		HRESULT STDMETHODCALLTYPE SetLogFile(DWORD_PTR hFile);
		HRESULT STDMETHODCALLTYPE Abort(void);
		HRESULT STDMETHODCALLTYPE ShouldOperationContinue(void);
		void Hook(IGraphBuilder *);

	private:
		IGraphBuilder *m_this;
};

class dxwMediaControl : public IMediaControl
{
	public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
		HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
		HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
		HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
		HRESULT STDMETHODCALLTYPE Run(void);
		HRESULT STDMETHODCALLTYPE Pause(void);
		HRESULT STDMETHODCALLTYPE Stop(void);
		HRESULT STDMETHODCALLTYPE GetState(LONG msTimeout, OAFilterState *pfs);
		HRESULT STDMETHODCALLTYPE RenderFile(BSTR strFilename);
		HRESULT STDMETHODCALLTYPE AddSourceFilter(BSTR strFilename, IDispatch **ppUnk);
		HRESULT STDMETHODCALLTYPE get_FilterCollection(IDispatch **ppUnk);
		HRESULT STDMETHODCALLTYPE get_RegFilterCollection(IDispatch **ppUnk);
		HRESULT STDMETHODCALLTYPE StopWhenReady(void);
	    
		void Hook(IMediaControl *);

	private:
		IMediaControl *m_this;
		DWORD m_GDIMode;
		BOOL m_Windowize;
};

class dxwMediaFilter : public IMediaFilter
{
	public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &arg1,void **arg2);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);
		HRESULT STDMETHODCALLTYPE Stop(void);
		HRESULT STDMETHODCALLTYPE Pause(void);
		HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart);
		HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
		HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock);
	    HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock);

		void Hook(IMediaFilter *);

	private:
		IMediaFilter *m_this;
};

class dxwVideoWindow : public IVideoWindow
    {
    public:
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
        HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
        HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
		HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
		HRESULT STDMETHODCALLTYPE put_Caption(BSTR strCaption);
        HRESULT STDMETHODCALLTYPE get_Caption(BSTR *strCaption);
        HRESULT STDMETHODCALLTYPE put_WindowStyle(long WindowStyle);
        HRESULT STDMETHODCALLTYPE get_WindowStyle(long *WindowStyle);
        HRESULT STDMETHODCALLTYPE put_WindowStyleEx(long WindowStyleEx);
        HRESULT STDMETHODCALLTYPE get_WindowStyleEx(long *WindowStyleEx);
        HRESULT STDMETHODCALLTYPE put_AutoShow(long AutoShow);
        HRESULT STDMETHODCALLTYPE get_AutoShow(long *AutoShow);
        HRESULT STDMETHODCALLTYPE put_WindowState(long WindowState);
        HRESULT STDMETHODCALLTYPE get_WindowState(long *WindowState);
        HRESULT STDMETHODCALLTYPE put_BackgroundPalette(long BackgroundPalette);
        HRESULT STDMETHODCALLTYPE get_BackgroundPalette(long *pBackgroundPalette);
        HRESULT STDMETHODCALLTYPE put_Visible(long Visible);
        HRESULT STDMETHODCALLTYPE get_Visible(long *pVisible);
        HRESULT STDMETHODCALLTYPE put_Left(long Left);
        HRESULT STDMETHODCALLTYPE get_Left(long *pLeft);
        HRESULT STDMETHODCALLTYPE put_Width(long Width);
        HRESULT STDMETHODCALLTYPE get_Width(long *pWidth);
        HRESULT STDMETHODCALLTYPE put_Top(long Top);
        HRESULT STDMETHODCALLTYPE get_Top(long *pTop);
        HRESULT STDMETHODCALLTYPE put_Height(long Height);
        HRESULT STDMETHODCALLTYPE get_Height(long *pHeight);
        HRESULT STDMETHODCALLTYPE put_Owner(OAHWND Owner);
        HRESULT STDMETHODCALLTYPE get_Owner(OAHWND *Owner);
        HRESULT STDMETHODCALLTYPE put_MessageDrain(OAHWND Drain);
        HRESULT STDMETHODCALLTYPE get_MessageDrain(OAHWND *Drain);
        HRESULT STDMETHODCALLTYPE get_BorderColor(long *Color);
        HRESULT STDMETHODCALLTYPE put_BorderColor(long Color);
        HRESULT STDMETHODCALLTYPE get_FullScreenMode(long *FullScreenMode);
        HRESULT STDMETHODCALLTYPE put_FullScreenMode(long FullScreenMode);
        HRESULT STDMETHODCALLTYPE SetWindowForeground(long Focus);
        HRESULT STDMETHODCALLTYPE NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam);
        HRESULT STDMETHODCALLTYPE SetWindowPosition(long Left, long Top, long Width, long Height);
        HRESULT STDMETHODCALLTYPE GetWindowPosition(long *pLeft, long *pTop, long *pWidth, long *pHeight);
        HRESULT STDMETHODCALLTYPE GetMinIdealImageSize(long *pWidth, long *pHeight);     
        HRESULT STDMETHODCALLTYPE GetMaxIdealImageSize(long *pWidth, long *pHeight);
        HRESULT STDMETHODCALLTYPE GetRestorePosition(long *pLeft, long *pTop, long *pWidth, long *pHeight);
        HRESULT STDMETHODCALLTYPE HideCursor(long HideCursor);
        HRESULT STDMETHODCALLTYPE IsCursorHidden(long *CursorHidden);
		
		void Hook(IVideoWindow *);

	private:
		IVideoWindow *m_this;
};

class dxwCreateDevEnum : public ICreateDevEnum
{
    public:
 		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE CreateClassEnumerator(REFCLSID clsidDeviceClass, IEnumMoniker **ppEnumMoniker, DWORD dwFlags);
        
 		void Hook(ICreateDevEnum *);

	private:
		ICreateDevEnum *m_this;
};


class dxwVMRSurfaceAllocator : public IVMRSurfaceAllocator
// MIDL_INTERFACE("31ce832e-4484-458b-8cca-f4d7e3db0b52
{
    public:
  		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE AllocateSurface(DWORD_PTR dwUserID, VMRALLOCATIONINFO *lpAllocInfo, DWORD *lpdwActualBuffers, LPDIRECTDRAWSURFACE7 *lplpSurface);
		HRESULT STDMETHODCALLTYPE FreeSurface(DWORD_PTR dwID);
		HRESULT STDMETHODCALLTYPE PrepareSurface(DWORD_PTR dwUserID, LPDIRECTDRAWSURFACE7 lpSurface, DWORD dwSurfaceFlags);
		HRESULT STDMETHODCALLTYPE AdviseNotify(IVMRSurfaceAllocatorNotify *lpIVMRSurfAllocNotify);
        
 		void Hook(IVMRSurfaceAllocator *);

	private:
		IVMRSurfaceAllocator *m_this;
};

class dxwVMRImagePresenterConfig: IVMRImagePresenterConfig    
{
    // MIDL_INTERFACE("9f3a1c85-8555-49ba-935f-be5b5b29d178")

	public:
 		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE SetRenderingPrefs(DWORD dwRenderFlags);
        HRESULT STDMETHODCALLTYPE GetRenderingPrefs(DWORD *dwRenderFlags);
        
 		void Hook(IVMRImagePresenterConfig *);

	private:
		IVMRImagePresenterConfig *m_this;
};

class dxwVMRImagePresenter: IVMRImagePresenter    
{
    // 

	public:
 		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE StartPresenting(DWORD_PTR dwUserID);
        HRESULT STDMETHODCALLTYPE StopPresenting(DWORD_PTR dwUserID);
        HRESULT STDMETHODCALLTYPE PresentImage(DWORD_PTR dwUserID, VMRPRESENTATIONINFO *lpPresInfo);
        
 		void Hook(IVMRImagePresenter *);

	private:
		IVMRImagePresenter *m_this;
};

class dxwVMRWindowlessControl : public IVMRWindowlessControl
{
    //MIDL_INTERFACE("0eb1088c-4dcd-46f0-878f-39dae86a51b7")

	public:
  		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth,LONG *lpARHeight);
        HRESULT STDMETHODCALLTYPE GetMinIdealVideoSize(LONG *lpWidth, LONG *lpHeight);
        HRESULT STDMETHODCALLTYPE GetMaxIdealVideoSize(LONG *lpWidth, LONG *lpHeight);
        HRESULT STDMETHODCALLTYPE SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
        HRESULT STDMETHODCALLTYPE GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect);
        HRESULT STDMETHODCALLTYPE GetAspectRatioMode(DWORD *lpAspectRatioMode);
        HRESULT STDMETHODCALLTYPE SetAspectRatioMode(DWORD AspectRatioMode);
        HRESULT STDMETHODCALLTYPE SetVideoClippingWindow(HWND hwnd);
        HRESULT STDMETHODCALLTYPE RepaintVideo(HWND hwnd, HDC hdc);
        HRESULT STDMETHODCALLTYPE DisplayModeChanged( void);
        HRESULT STDMETHODCALLTYPE GetCurrentImage(BYTE **lpDib);
        HRESULT STDMETHODCALLTYPE SetBorderColor(COLORREF Clr);
        HRESULT STDMETHODCALLTYPE GetBorderColor(COLORREF *lpClr);
        HRESULT STDMETHODCALLTYPE SetColorKey(COLORREF Clr);
        HRESULT STDMETHODCALLTYPE GetColorKey(COLORREF *lpClr); 
        
 		void Hook(IVMRWindowlessControl *);

	private:
		IVMRWindowlessControl *m_this;
		LPRECT m_DSTRect;
};

class dxwVMRSurfaceAllocatorNotify : public IVMRSurfaceAllocatorNotify
{
    // MIDL_INTERFACE("aada05a8-5a4e-4729-af0b-cea27aed51e2")

    public:
  		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        virtual HRESULT STDMETHODCALLTYPE AdviseSurfaceAllocator(DWORD_PTR dwUserID, IVMRSurfaceAllocator *lpIVMRSurfaceAllocator);
        virtual HRESULT STDMETHODCALLTYPE SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor);
        virtual HRESULT STDMETHODCALLTYPE ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor);
        virtual HRESULT STDMETHODCALLTYPE RestoreDDrawSurfaces(void);
        virtual HRESULT STDMETHODCALLTYPE NotifyEvent(LONG EventCode, LONG_PTR Param1, LONG_PTR Param2);
        virtual HRESULT STDMETHODCALLTYPE SetBorderColor(COLORREF clrBorder);
        
 		void Hook(IVMRSurfaceAllocatorNotify *);

	private:
		IVMRSurfaceAllocatorNotify *m_this;
};

class dxwMediaEventSink : public IMediaEventSink
{
    // MIDL_INTERFACE("56a868a2-0ad4-11ce-b03a-0020af0ba770")
    public:
  		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE Notify(long EventCode, LONG_PTR EventParam1, LONG_PTR EventParam2);
        
 		void Hook(IMediaEventSink *);

	private:
		IMediaEventSink *m_this;
};
 
class dxwAMOpenProgress : public IAMOpenProgress
{
    // MIDL_INTERFACE("8E1C39A1-DE53-11cf-AA63-0080C744528D")
    public:
  		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
        HRESULT STDMETHODCALLTYPE QueryProgress(LONGLONG *pllTotal, LONGLONG *pllCurrent);
        HRESULT STDMETHODCALLTYPE AbortOperation(void);
        
 		void Hook(IAMOpenProgress *);

	private:
		IAMOpenProgress *m_this;
};

//======

class dxwBasicVideo : public IBasicVideo
{
    // MIDL_INTERFACE("56a868b5-0ad4-11ce-b03a-0020af0ba770")

	public:
  		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
		HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
		HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
		HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
        HRESULT STDMETHODCALLTYPE get_AvgTimePerFrame(REFTIME *pAvgTimePerFrame);
        HRESULT STDMETHODCALLTYPE get_BitRate(long *pBitRate);
        HRESULT STDMETHODCALLTYPE get_BitErrorRate(long *pBitErrorRate);
        HRESULT STDMETHODCALLTYPE get_VideoWidth(long *pVideoWidth);
        HRESULT STDMETHODCALLTYPE get_VideoHeight(long *pVideoHeight);
        HRESULT STDMETHODCALLTYPE put_SourceLeft(long SourceLeft);
        HRESULT STDMETHODCALLTYPE get_SourceLeft(long *pSourceLeft);
        HRESULT STDMETHODCALLTYPE put_SourceWidth(long SourceWidth);
        HRESULT STDMETHODCALLTYPE get_SourceWidth(long *pSourceWidth);
        HRESULT STDMETHODCALLTYPE put_SourceTop(long SourceTop);
        HRESULT STDMETHODCALLTYPE get_SourceTop(long *pSourceTop);
        HRESULT STDMETHODCALLTYPE put_SourceHeight(long SourceHeight);
        HRESULT STDMETHODCALLTYPE get_SourceHeight(long *pSourceHeight);
        HRESULT STDMETHODCALLTYPE put_DestinationLeft(long DestinationLeft);
        HRESULT STDMETHODCALLTYPE get_DestinationLeft(long *pDestinationLeft);
        HRESULT STDMETHODCALLTYPE put_DestinationWidth(long DestinationWidth);
        HRESULT STDMETHODCALLTYPE get_DestinationWidth(long *pDestinationWidth);
        HRESULT STDMETHODCALLTYPE put_DestinationTop(long DestinationTop);
        HRESULT STDMETHODCALLTYPE get_DestinationTop(long *pDestinationTop);
        HRESULT STDMETHODCALLTYPE put_DestinationHeight(long DestinationHeight);
        HRESULT STDMETHODCALLTYPE get_DestinationHeight(long *pDestinationHeight);
        HRESULT STDMETHODCALLTYPE SetSourcePosition(long Left, long Top, long Width, long Height);
        HRESULT STDMETHODCALLTYPE GetSourcePosition(long *pLeft, long *pTop, long *pWidth, long *pHeight);
        HRESULT STDMETHODCALLTYPE SetDefaultSourcePosition(void);
        HRESULT STDMETHODCALLTYPE SetDestinationPosition(long Left, long Top, long Width, long Height);
        HRESULT STDMETHODCALLTYPE GetDestinationPosition(long *pLeft, long *pTop, long *pWidth, long *pHeight);
        HRESULT STDMETHODCALLTYPE SetDefaultDestinationPosition(void);
        HRESULT STDMETHODCALLTYPE GetVideoSize(long *pWidth, long *pHeight);
        HRESULT STDMETHODCALLTYPE GetVideoPaletteEntries(long StartIndex, long Entries, long *pRetrieved, long *pPalette);
        HRESULT STDMETHODCALLTYPE GetCurrentImage(long *pBufferSize, long *pDIBImage);
        HRESULT STDMETHODCALLTYPE IsUsingDefaultSource(void);
        HRESULT STDMETHODCALLTYPE IsUsingDefaultDestination(void);
 		void Hook(IBasicVideo *);

	private:
		IBasicVideo *m_this;
};
    
class dxwBasicAudio : public IBasicAudio
{
    // MIDL_INTERFACE("56a868b3-0ad4-11ce-b03a-0020af0ba770")

	public:
  		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
		HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
		HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
		HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
        HRESULT STDMETHODCALLTYPE put_Volume(long lVolume);
        HRESULT STDMETHODCALLTYPE get_Volume(long *plVolume);
        HRESULT STDMETHODCALLTYPE put_Balance(long lBalance);
        HRESULT STDMETHODCALLTYPE get_Balance(long *plBalance);
 		void Hook(IBasicAudio *);

	private:
		IBasicAudio *m_this;
};

class dxwMediaEventEx : public IMediaEventEx
{
	// MIDL_INTERFACE("56a868c0-0ad4-11ce-b03a-0020af0ba770")

	public:
   		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
		HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
		HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
		HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
		HRESULT STDMETHODCALLTYPE GetEventHandle(OAEVENT *hEvent);
        HRESULT STDMETHODCALLTYPE GetEvent(long *lEventCode, LONG_PTR *lParam1, LONG_PTR *lParam2, long msTimeout);
        HRESULT STDMETHODCALLTYPE WaitForCompletion(long msTimeout, long *pEvCode);
        HRESULT STDMETHODCALLTYPE CancelDefaultHandling(long lEvCode);
        HRESULT STDMETHODCALLTYPE RestoreDefaultHandling(long lEvCode);
        HRESULT STDMETHODCALLTYPE FreeEventParams(long lEvCode, LONG_PTR lParam1, LONG_PTR lParam2);
		HRESULT STDMETHODCALLTYPE SetNotifyWindow(OAHWND hwnd, long lMsg, LONG_PTR lInstanceData);
        HRESULT STDMETHODCALLTYPE SetNotifyFlags(long lNoNotifyFlags);
        HRESULT STDMETHODCALLTYPE GetNotifyFlags(long *lplNoNotifyFlags);
  		void Hook(IMediaEventEx *);

	private:
		IMediaEventEx *m_this;       
    };

class dxwMediaEvent: public IMediaEvent
{
	//  MIDL_INTERFACE("56a868b6-0ad4-11ce-b03a-0020af0ba770")

public:
   		HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,void **obp);
		ULONG STDMETHODCALLTYPE AddRef(void);
		ULONG STDMETHODCALLTYPE Release(void);
		HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
		HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
		HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
		HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
		HRESULT STDMETHODCALLTYPE GetEventHandle(OAEVENT *hEvent);
        HRESULT STDMETHODCALLTYPE GetEvent(long *lEventCode, LONG_PTR *lParam1, LONG_PTR *lParam2, long msTimeout);
        HRESULT STDMETHODCALLTYPE WaitForCompletion(long msTimeout, long *pEvCode);
        HRESULT STDMETHODCALLTYPE CancelDefaultHandling(long lEvCode);
        HRESULT STDMETHODCALLTYPE RestoreDefaultHandling(long lEvCode);
        HRESULT STDMETHODCALLTYPE FreeEventParams(long lEvCode, LONG_PTR lParam1, LONG_PTR lParam2);
  		void Hook(IMediaEvent *);

	private:
		IMediaEvent *m_this;       
    };

//======

extern void HookInterfaceDS(ApiArg, void *obj, REFIID riid, LPVOID *obp);
extern HRESULT CheckInterfaceDS(ApiArg, REFIID riid);
extern void GraphDump(IGraphBuilder *pGraph);
extern void HookDDSurface(LPDIRECTDRAWSURFACE *, int, BOOL);

void HookCreateDevEnum(ICreateDevEnum **);
void HookGraphBuilder(IGraphBuilder **);
void HookEnumFilters(IEnumFilters **);
void HookEnumPins(IEnumPins **);
void HookVideoWindow(IVideoWindow **);
void HookMediaControl(IMediaControl **);
void HookMediaEventSink(IMediaEventSink **);
void HookBaseFilter(IBaseFilter **);
void HookMediaFilter(IMediaFilter **);
void HookIMFVideoDisplayControl(LPVOID);
void HookIMFVideoRenderer(LPVOID);
void HookIVMRWindowlessControl(IVMRWindowlessControl **);
void HookIVMRImagePresenter(IVMRImagePresenter **);
void HookBasicVideo(IBasicVideo **);
void HookBasicVideo2(IBasicVideo2 **);
void HookBasicAudio(IBasicAudio **);
void HookIAMOpenProgress(IAMOpenProgress **);
void HookVideoRendererDefault(LPVOID);
void HookVMRSurfaceAllocator(IVMRSurfaceAllocator **);
void HookMediaEventSink(IMediaEventSink **);
void HookIVMRImagePresenterConfig(IVMRImagePresenterConfig **);
void HookIVMRMonitorConfig(LPVOID);
void HookMediaEventEx(IMediaEventEx **);
void HookMediaEvent(IMediaEvent **);
void HookMediaSeeking(IMediaSeeking **);
void HookFilterGraph(IFilterGraph **);
void HookFilterGraph2(IFilterGraph2 **);
void HookTypeInfo(ITypeInfo **);

