#include <strmif.h>
#include <d3d9.h>

class dxwDirect3DQuery9 : public IDirect3DQuery9
{

	public:    
		/*** IUnknown methods ***/
		HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj);
		ULONG STDMETHODCALLTYPE AddRef(THIS);
		ULONG STDMETHODCALLTYPE Release(THIS);

		/*** IDirect3DQuery9 methods ***/
		HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice);
		D3DQUERYTYPE STDMETHODCALLTYPE GetType(THIS);
		DWORD STDMETHODCALLTYPE GetDataSize(THIS);
		HRESULT STDMETHODCALLTYPE Issue(THIS_ DWORD dwIssueFlags);
		HRESULT STDMETHODCALLTYPE GetData(THIS_ void* pData, DWORD dwSize, DWORD dwGetDataFlags);
	    
		void STDMETHODCALLTYPE Initialize(D3DQUERYTYPE, DWORD, IDirect3DDevice9*);
	private:
		D3DQUERYTYPE Type;
		DWORD m_this;
		ULONG m_ref;
		IDirect3DDevice9* m_Device;
};

