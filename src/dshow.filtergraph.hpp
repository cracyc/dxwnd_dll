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
