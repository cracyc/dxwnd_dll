// D3D8/9 API
#ifndef D3DADAPTER_IDENTIFIER8
typedef struct _D3DADAPTER_IDENTIFIER8
{
    char            Driver[MAX_DEVICE_IDENTIFIER_STRING];
    char            Description[MAX_DEVICE_IDENTIFIER_STRING];
    LARGE_INTEGER   DriverVersion;            /* Defined for 32 bit components */
    DWORD           VendorId;
    DWORD           DeviceId;
    DWORD           SubSysId;
    DWORD           Revision;
    GUID            DeviceIdentifier;
    DWORD           WHQLLevel;

} D3DADAPTER_IDENTIFIER8;
#endif 

typedef void* (WINAPI *Direct3DCreate8_Type)(UINT);
typedef void* (WINAPI *Direct3DCreate9_Type)(UINT);
typedef HRESULT (WINAPI *Direct3DCreate9Ex_Type)(UINT, IDirect3D9Ex **);
typedef HRESULT (WINAPI *CheckFullScreen_Type)(void);
typedef void (WINAPI * D3DPERF_SetOptions_Type)(DWORD);
typedef BOOL (WINAPI * DisableD3DSpy_Type)(void);

void*	WINAPI extDirect3DCreate8(UINT);
void*	WINAPI extDirect3DCreate9(UINT);
HRESULT WINAPI extDirect3DCreate9Ex(UINT, IDirect3D9Ex **);
HRESULT WINAPI extCheckFullScreen(void);
HRESULT WINAPI voidDirect3DShaderValidatorCreate9(void);
void	WINAPI voidDebugSetLevel(void);
void	WINAPI voidDebugSetMute(void);
BOOL	WINAPI voidDisableD3DSpy(void);
BOOL	WINAPI extDisableD3DSpy(void);
void	WINAPI extD3DPERF_SetOptions(DWORD);
BOOL    WINAPI voidDisableD3DSpy(void);

Direct3DCreate8_Type pDirect3DCreate8 = 0;
Direct3DCreate9_Type pDirect3DCreate9 = 0;
Direct3DCreate9Ex_Type pDirect3DCreate9Ex = 0;
CheckFullScreen_Type pCheckFullScreen = 0;
D3DPERF_SetOptions_Type pD3DPERF_SetOptions = 0;
DisableD3DSpy_Type pDisableD3DSpy = 0;

// IDirect3D8/9 methods 

typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID riid, void** ppvObj);
typedef ULONG	(WINAPI *ReleaseD3D_Type)(void *);
typedef UINT	(WINAPI *GetAdapterCount_Type)(void *);
typedef HRESULT (WINAPI *GetAdapterIdentifier9_Type)(void *, UINT, DWORD, D3DADAPTER_IDENTIFIER9 *);
typedef HRESULT (WINAPI *GetAdapterIdentifier8_Type)(void *, UINT, DWORD, D3DADAPTER_IDENTIFIER8 *);
typedef UINT	(WINAPI *GetAdapterModeCount8_Type)(void *, UINT);
typedef UINT	(WINAPI *GetAdapterModeCount9_Type)(void *, UINT, D3DFORMAT);
typedef HRESULT (WINAPI *EnumAdapterModes8_Type)(void *, UINT, UINT, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *EnumAdapterModes9_Type)(void *, UINT, D3DFORMAT ,UINT, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *GetAdapterDisplayMode_Type)(void *, UINT, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *CheckDeviceType_Type)(void *, UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, BOOL);
typedef HRESULT (WINAPI *CheckDeviceFormat_Type)(void *, UINT, D3DDEVTYPE, D3DFORMAT, DWORD, D3DRESOURCETYPE, D3DFORMAT);
typedef HRESULT (WINAPI *CheckDeviceMultiSampleType8_Type)(void *, UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE);
typedef HRESULT (WINAPI *CheckDeviceMultiSampleType9_Type)(void *, UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE, DWORD *);
typedef HRESULT (WINAPI *D3DGetDeviceCaps8_Type)(void *, UINT, D3DDEVTYPE, D3DCAPS8 *);
typedef HRESULT (WINAPI *D3DGetDeviceCaps9_Type)(void *, UINT, D3DDEVTYPE, D3DCAPS9 *);
typedef HMONITOR (WINAPI *GetAdapterMonitor_Type)(void *, UINT);
typedef HRESULT (WINAPI *CreateDevice_Type)(void *, UINT, D3DDEVTYPE, HWND, DWORD, void *, void **);
typedef HRESULT (WINAPI *CreateDeviceEx_Type)(void *, UINT, D3DDEVTYPE, HWND, DWORD, void *, D3DDISPLAYMODEEX *, void **);
typedef BOOL	(WINAPI *DisableD3DSpy_Type)(void);
typedef HRESULT (WINAPI *GetBackBuffer8_Type)(void *, UINT, D3DBACKBUFFER_TYPE, LPDIRECTDRAWSURFACE *);
typedef HRESULT (WINAPI *GetBackBuffer9_Type)(void *, UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9 **);

HRESULT WINAPI extQueryInterfaceD3D8(void *, REFIID, void **);
HRESULT WINAPI extReleaseD3D8(void *); 
UINT WINAPI extGetAdapterCount8(void *);
HRESULT WINAPI extGetAdapterIdentifier8(void *, UINT, DWORD, D3DADAPTER_IDENTIFIER8 *);
UINT WINAPI extGetAdapterModeCount8(void *, UINT);
HRESULT WINAPI extEnumAdapterModes8(void *, UINT, UINT , D3DDISPLAYMODE *);
HRESULT WINAPI extGetAdapterDisplayMode8(void *, UINT, D3DDISPLAYMODE *);
HRESULT WINAPI extCheckDeviceType8(void *, UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, BOOL);
HRESULT WINAPI extD3DGetDeviceCaps8(void *, UINT, D3DDEVTYPE, D3DCAPS8 *);
HRESULT WINAPI extCheckDeviceFormat8(void *, UINT, D3DDEVTYPE, D3DFORMAT, DWORD, D3DRESOURCETYPE, D3DFORMAT);
HRESULT WINAPI extCheckDeviceMultiSampleType8(void *, UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE);
HRESULT WINAPI extCheckDeviceMultiSampleType9(void *, UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE, DWORD *);
HMONITOR WINAPI extGetAdapterMonitor8(void *, UINT);
HRESULT WINAPI extCreateDevice8(void *, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS *, void **);

HRESULT WINAPI extQueryInterfaceD3D9(void *, REFIID, void **);
HRESULT WINAPI extReleaseD3D9(void *); 
UINT WINAPI extGetAdapterCount9(void *);
HRESULT WINAPI extGetAdapterIdentifier9(void *, UINT, DWORD, D3DADAPTER_IDENTIFIER9 *);
UINT WINAPI extGetAdapterModeCount9(void *, UINT, D3DFORMAT);
HRESULT WINAPI extEnumAdapterModes9(void *, UINT, D3DFORMAT, UINT , D3DDISPLAYMODE *);
HRESULT WINAPI extGetAdapterDisplayMode9(void *, UINT, D3DDISPLAYMODE *);
HRESULT WINAPI extCheckDeviceType9(void *, UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, BOOL);
HRESULT WINAPI extCheckDeviceFormat9(void *, UINT, D3DDEVTYPE, D3DFORMAT, DWORD, D3DRESOURCETYPE, D3DFORMAT);
HRESULT WINAPI extCheckDeviceMultiSampleType(void *, UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE, DWORD *);
HRESULT WINAPI extD3DGetDeviceCaps9(void *, UINT, D3DDEVTYPE, D3DCAPS9 *);
HMONITOR WINAPI extGetAdapterMonitor9(void *, UINT);
HRESULT WINAPI extCreateDevice9(void *, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS *, void **);
HRESULT WINAPI extCreateDeviceEx(void *, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS *, D3DDISPLAYMODEEX *, void **);

QueryInterface_Type pQueryInterfaceD3D8 = 0;
QueryInterface_Type pQueryInterfaceD3D9 = 0;
ReleaseD3D_Type pReleaseD3D8, pReleaseD3D9;
GetAdapterCount_Type pGetAdapterCount8, pGetAdapterCount9;
GetAdapterIdentifier8_Type pGetAdapterIdentifier8; 
GetAdapterIdentifier9_Type pGetAdapterIdentifier9;
GetAdapterModeCount8_Type pGetAdapterModeCount8;
GetAdapterModeCount9_Type pGetAdapterModeCount9;
EnumAdapterModes8_Type pEnumAdapterModes8 = 0;
EnumAdapterModes9_Type pEnumAdapterModes9 = 0;
GetAdapterDisplayMode_Type pGetAdapterDisplayMode8 = 0;
GetAdapterDisplayMode_Type pGetAdapterDisplayMode9 = 0;
CheckDeviceType_Type pCheckDeviceType8, pCheckDeviceType9;
CheckDeviceFormat_Type pCheckDeviceFormat8, pCheckDeviceFormat9;
CheckDeviceMultiSampleType8_Type pCheckDeviceMultiSampleType8;
CheckDeviceMultiSampleType9_Type pCheckDeviceMultiSampleType9;
D3DGetDeviceCaps8_Type pD3DGetDeviceCaps8 = 0;
D3DGetDeviceCaps9_Type pD3DGetDeviceCaps9 = 0;
GetAdapterMonitor_Type pGetAdapterMonitor8, pGetAdapterMonitor9;
CreateDevice_Type pCreateDevice8, pCreateDevice9;
CreateDeviceEx_Type pCreateDeviceEx = 0;

// IDirect3DDevice8/9 methods

typedef ULONG	(WINAPI *AddRef_Type)(void *);
typedef ULONG	(WINAPI *Release_Type)(void *);
typedef HRESULT (WINAPI *TestCooperativeLevel_Type)(void *);
typedef UINT	(WINAPI *GetAvailableTextureMem_Type)(void *);
typedef HRESULT (WINAPI *GetDirect3D8_Type)(void *, void **);
typedef HRESULT (WINAPI *GetDirect3D9_Type)(void *, void **);
typedef HRESULT (WINAPI *D3DDGetDeviceCaps_Type)(void *, void *);
typedef HRESULT (WINAPI *GetDisplayMode8_Type)(void *, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *GetDisplayMode9_Type)(void *, UINT, D3DDISPLAYMODE *);
typedef void	(WINAPI *SetCursorPosition9_Type)(void *, int, int, DWORD);
typedef void	(WINAPI *SetCursorPosition8_Type)(void *, int, int, DWORD);
typedef BOOL	(WINAPI *ShowCursor8_Type)(void *, BOOL);
typedef BOOL	(WINAPI *ShowCursor9_Type)(void *, BOOL);
typedef HRESULT (WINAPI *CreateAdditionalSwapChain_Type)(void *, D3DPRESENT_PARAMETERS *, IDirect3DSwapChain9 **);
typedef HRESULT (WINAPI *GetSwapChain_Type)(void *, UINT, IDirect3DSwapChain9**);
typedef UINT	(WINAPI *GetNumberOfSwapChains_Type)(void *);
typedef HRESULT (WINAPI *Reset_Type)(void *, D3DPRESENT_PARAMETERS*);
typedef HRESULT (WINAPI *Present_Type)(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
typedef void	(WINAPI *SetGammaRamp8_Type)(void *, DWORD, D3DGAMMARAMP *);
typedef void	(WINAPI *SetGammaRamp9_Type)(void *, UINT, DWORD, D3DGAMMARAMP *);
typedef void	(WINAPI *GetGammaRamp8_Type)(void *, D3DGAMMARAMP *);
typedef void	(WINAPI *GetGammaRamp9_Type)(void *, UINT, D3DGAMMARAMP *);
typedef HRESULT (WINAPI *CreateTexture8_Type)(void *, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, void **);
typedef HRESULT (WINAPI *CreateTexture9_Type)(void *, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, void **, HANDLE *);
typedef ULONG	(WINAPI *CreateRenderTarget8_Type)(void *, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, BOOL, void**);
typedef ULONG	(WINAPI *CreateRenderTarget9_Type)(void *, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, BOOL, void**);
typedef ULONG	(WINAPI *GetRenderTarget8_Type)(void *, void **);
typedef ULONG	(WINAPI *GetRenderTarget9_Type)(void *, DWORD, void **);
typedef HRESULT (WINAPI *GetFrontBufferData_Type)(void *, UINT, LPDIRECTDRAWSURFACE);
typedef HRESULT (WINAPI *StretchRect_Type)(void *, IDirect3DSurface9 *, CONST RECT *, IDirect3DSurface9 *, CONST RECT *, D3DTEXTUREFILTERTYPE);
typedef ULONG	(WINAPI *BeginScene_Type)(void *);
typedef ULONG	(WINAPI *EndScene_Type)(void *);
// n.b. D3DVIEWPORT9 and D3DVIEWPORT8 are identical
typedef HRESULT (WINAPI *SetViewport_Type)(void *, CONST D3DVIEWPORT9 *);
typedef HRESULT (WINAPI *GetViewport_Type)(void *, D3DVIEWPORT9 *);
// n.b. prototypes and D3DLIGHT8/D3DLIGHT9 are identical, so 1 fits both D3D8 & D3D9
typedef HRESULT (WINAPI *SetLight_Type)(void *, DWORD Index, CONST D3DLIGHT9 *);
typedef HRESULT (WINAPI *GetLight_Type)(void *, DWORD Index, D3DLIGHT9 *);
typedef HRESULT (WINAPI *SetRenderState_Type)(void *, D3DRENDERSTATETYPE, DWORD);
typedef HRESULT (WINAPI *GetRenderState_Type)(void *, D3DRENDERSTATETYPE, LPDWORD );
typedef HRESULT (WINAPI *BeginStateBlock_Type)(void *);
typedef HRESULT (WINAPI *EndStateBlock8_Type)(void *, DWORD *);
typedef HRESULT (WINAPI *EndStateBlock9_Type)(void *, IDirect3DStateBlock9**);
typedef HRESULT (WINAPI *SetTexture8_Type)(void *, DWORD, void *);
typedef HRESULT (WINAPI *SetTexture9_Type)(void *, DWORD, void *);
typedef HRESULT	(WINAPI *CopyRects_Type)(void *, LPDIRECTDRAWSURFACE, CONST RECT *, UINT, LPDIRECTDRAWSURFACE, CONST POINT *);
typedef HRESULT (WINAPI *GetFrontBuffer_Type)(void *, LPDIRECTDRAWSURFACE);
typedef ULONG	(WINAPI *ReleaseDev_Type)(void *);
typedef HRESULT (WINAPI *GetDepthStencilSurface_Type)(void *, void**);
typedef HRESULT (WINAPI *CreateDepthStencilSurface8_Type)(void *, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, void **);
typedef HRESULT (WINAPI *Clear8_Type)(void *, DWORD, CONST D3DRECT *, DWORD, D3DCOLOR, float, DWORD);
typedef HRESULT (WINAPI *SetTransform_Type)(void *, D3DTRANSFORMSTATETYPE, CONST D3DMATRIX *);
typedef ULONG	(WINAPI *CreateImageSurface8_Type)(void *, UINT, UINT, D3DFORMAT, void**);
typedef HRESULT (WINAPI *CreateVertexBuffer8_Type)(void *, UINT, DWORD, DWORD, D3DPOOL, DWORD **);
typedef HRESULT (WINAPI *CreateVertexBuffer9_Type)(void *, UINT, DWORD, DWORD, D3DPOOL, DWORD **, HANDLE *);

HRESULT WINAPI extQueryInterfaceDev8(void *, REFIID, void **);
HRESULT WINAPI extQueryInterfaceDev9(void *, REFIID, void **);
ULONG	WINAPI extReleaseDev8(void *);
ULONG	WINAPI extReleaseDev9(void *);
HRESULT WINAPI extTestCooperativeLevel8(void *);
HRESULT WINAPI extTestCooperativeLevel9(void *);
UINT	WINAPI extGetAvailableTextureMem8(void *);
UINT	WINAPI extGetAvailableTextureMem9(void *);
HRESULT WINAPI extGetDirect3D8(void *, void **);
HRESULT WINAPI extGetDirect3D9(void *, void **);
HRESULT WINAPI extD3DDGetDeviceCaps8(void *, D3DCAPS8 *);
HRESULT WINAPI extD3DDGetDeviceCaps9(void *, D3DCAPS9 *);
HRESULT WINAPI extGetDisplayMode8(void *, D3DDISPLAYMODE *);
HRESULT WINAPI extGetDisplayMode9(void *, UINT, D3DDISPLAYMODE *);
void	WINAPI extSetCursorPosition9(void *, int, int, DWORD);
void	WINAPI extSetCursorPosition8(void *, int, int, DWORD);
BOOL	WINAPI extShowCursor8(void *, BOOL);
BOOL	WINAPI extShowCursor9(void *, BOOL);
HRESULT WINAPI extCreateAdditionalSwapChain8(void *, D3DPRESENT_PARAMETERS *, IDirect3DSwapChain9 **);
HRESULT WINAPI extCreateAdditionalSwapChain9(void *, D3DPRESENT_PARAMETERS *, IDirect3DSwapChain9 **);
HRESULT WINAPI extGetSwapChain(void *, UINT, IDirect3DSwapChain9**);
UINT	WINAPI extGetNumberOfSwapChains(void *);
HRESULT WINAPI extReset8(void *, D3DPRESENT_PARAMETERS *);
HRESULT WINAPI extReset9(void *, D3DPRESENT_PARAMETERS *);
HRESULT WINAPI extPresent8(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
HRESULT WINAPI extPresent9(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
HRESULT WINAPI extGetBackBuffer8(void *, UINT, D3DBACKBUFFER_TYPE, LPDIRECTDRAWSURFACE *);
HRESULT WINAPI extGetBackBuffer9(void *, UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9 **);
void	WINAPI extSetGammaRamp8(void *, DWORD, D3DGAMMARAMP *);
void	WINAPI extSetGammaRamp9(void *, UINT, DWORD, D3DGAMMARAMP *);
void	WINAPI extGetGammaRamp8(void *, D3DGAMMARAMP *);
void	WINAPI extGetGammaRamp9(void *, UINT, D3DGAMMARAMP *);
HRESULT WINAPI extCreateTexture8(void *, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, void **);
HRESULT WINAPI extCreateTexture9(void *, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, void **, HANDLE *);
ULONG	WINAPI extCreateRenderTarget8(void *, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, BOOL, void **);
ULONG	WINAPI extCreateRenderTarget9(void *, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, BOOL, void **);
ULONG	WINAPI extGetRenderTarget8(void *, void **);
ULONG	WINAPI extGetRenderTarget9(void *, DWORD, void **);
HRESULT WINAPI extGetFrontBufferData9(void *, UINT, LPDIRECTDRAWSURFACE);
HRESULT WINAPI extStretchRect9(void *, IDirect3DSurface9 *, CONST RECT *, IDirect3DSurface9 *, CONST RECT *, D3DTEXTUREFILTERTYPE);
ULONG	WINAPI extBeginScene8(void *);
ULONG	WINAPI extBeginScene9(void *);
ULONG	WINAPI extEndScene8(void *);
ULONG	WINAPI extEndScene9(void *);
#ifdef TRACEALL
HRESULT WINAPI extClear8(void *, DWORD, CONST D3DRECT *, DWORD, D3DCOLOR, float, DWORD);
#endif // TRACEALL
HRESULT WINAPI extSetTransform8(void *, D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
HRESULT WINAPI extSetTransform9(void *, D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
HRESULT WINAPI extSetViewport8(void *, CONST D3DVIEWPORT9 *);
HRESULT WINAPI extSetViewport9(void *, CONST D3DVIEWPORT9 *);
HRESULT WINAPI extGetViewport8(void *, D3DVIEWPORT9 *);
HRESULT WINAPI extGetViewport9(void *, D3DVIEWPORT9 *);
HRESULT WINAPI extSetLight(void *, DWORD Index, CONST D3DLIGHT9 *);
HRESULT WINAPI extGetLight(void *, DWORD Index, D3DLIGHT9 *);
HRESULT WINAPI extSetRenderState8(void *, D3DRENDERSTATETYPE, DWORD);
HRESULT WINAPI extSetRenderState9(void *, D3DRENDERSTATETYPE, DWORD);
HRESULT WINAPI extGetRenderState8(void *, D3DRENDERSTATETYPE, LPDWORD);
HRESULT WINAPI extGetRenderState9(void *, D3DRENDERSTATETYPE, LPDWORD);
HRESULT WINAPI extBeginStateBlock8(void *);
HRESULT WINAPI extBeginStateBlock9(void *);
HRESULT WINAPI extEndStateBlock8(void *, DWORD *);
HRESULT WINAPI extEndStateBlock9(void *, IDirect3DStateBlock9**);
ULONG	WINAPI extSetTexture8(void *, DWORD, void *);
ULONG	WINAPI extSetTexture9(void *, DWORD, void *);
// CopyRects prototype uses IDirect3DSurface8 *, but to avoid including d3d8.h better use a generic ptr as LPDIRECTDRAWSURFACE
HRESULT	WINAPI extCopyRects(void *, LPDIRECTDRAWSURFACE, CONST RECT *, UINT, LPDIRECTDRAWSURFACE, CONST POINT *);
HRESULT WINAPI extGetFrontBuffer(void *, LPDIRECTDRAWSURFACE);
#ifdef TRACEALL
HRESULT WINAPI extGetDepthStencilSurface8(void *, void**);
HRESULT WINAPI extCreateDepthStencilSurface8(void *, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, void **);
#endif
ULONG WINAPI extCreateImageSurface8(void *, UINT, UINT, D3DFORMAT, void **);
HRESULT WINAPI extCreateVertexBuffer8(void *, UINT, DWORD, DWORD, D3DPOOL, DWORD **);
HRESULT WINAPI extCreateVertexBuffer9(void *, UINT, DWORD, DWORD, D3DPOOL, DWORD **, HANDLE *);

// unreferenced
//AddRef_Type pAddRef9 = 0;
//Release_Type pRelease9 = 0;
QueryInterface_Type pQueryInterfaceDev8 = 0;
QueryInterface_Type pQueryInterfaceDev9 = 0;
GetAvailableTextureMem_Type pGetAvailableTextureMem8, pGetAvailableTextureMem9;
TestCooperativeLevel_Type pTestCooperativeLevel8, pTestCooperativeLevel9;
GetDirect3D8_Type pGetDirect3D8 = 0;
GetDirect3D9_Type pGetDirect3D9 = 0;
D3DDGetDeviceCaps_Type pD3DDGetDeviceCaps8, pD3DDGetDeviceCaps9;
GetDisplayMode8_Type pGetDisplayMode8 = 0;
GetDisplayMode9_Type pGetDisplayMode9 = 0;
SetCursorPosition9_Type pSetCursorPosition9 = 0;
SetCursorPosition8_Type pSetCursorPosition8 = 0;
ShowCursor8_Type pShowCursor8 = 0;
ShowCursor9_Type pShowCursor9 = 0;
CreateAdditionalSwapChain_Type pCreateAdditionalSwapChain8 = 0;
CreateAdditionalSwapChain_Type pCreateAdditionalSwapChain9 = 0;
GetSwapChain_Type pGetSwapChain = 0;
GetNumberOfSwapChains_Type pGetNumberOfSwapChains = 0;
BeginStateBlock_Type pBeginStateBlock8 = 0;
BeginStateBlock_Type pBeginStateBlock9 = 0;
EndStateBlock8_Type pEndStateBlock8 = 0;
EndStateBlock9_Type pEndStateBlock9 = 0;
CreateTexture8_Type pCreateTexture8 = 0;
CreateTexture9_Type pCreateTexture9 = 0;
CopyRects_Type pCopyRects = 0;
GetFrontBuffer_Type pGetFrontBuffer = 0;
ReleaseDev_Type pReleaseDev8, pReleaseDev9;
#ifdef TRACEALL
GetDepthStencilSurface_Type pGetDepthStencilSurface8;
CreateDepthStencilSurface8_Type pCreateDepthStencilSurface8;
Clear8_Type pClear8;
SetTransform_Type pSetTransform8, pSetTransform9;
#endif
GetBackBuffer8_Type pGetBackBuffer8 = 0;
GetBackBuffer9_Type pGetBackBuffer9 = 0;
GetFrontBufferData_Type pGetFrontBufferData9;
StretchRect_Type pStretchRect9;
Present_Type pPresent8 = 0;
Present_Type pPresent9 = 0;
SetRenderState_Type pSetRenderState8, pSetRenderState9;
GetRenderState_Type pGetRenderState8, pGetRenderState9;
GetViewport_Type pGetViewport8, pGetViewport9;
SetViewport_Type pSetViewport8, pSetViewport9;
SetGammaRamp8_Type pSetGammaRamp8;
SetGammaRamp9_Type pSetGammaRamp9;
GetGammaRamp8_Type pGetGammaRamp8;
GetGammaRamp9_Type pGetGammaRamp9;
SetLight_Type pSetLight;
GetLight_Type pGetLight;
CreateImageSurface8_Type pCreateImageSurface8 = 0;
CreateRenderTarget8_Type pCreateRenderTarget8 = 0;
CreateRenderTarget9_Type pCreateRenderTarget9 = 0;
GetRenderTarget8_Type pGetRenderTarget8 = 0;
GetRenderTarget9_Type pGetRenderTarget9 = 0;
BeginScene_Type pBeginScene8, pBeginScene9;
EndScene_Type pEndScene8, pEndScene9;
Reset_Type pReset8, pReset9;
SetTexture8_Type pSetTexture8 = 0;
SetTexture9_Type pSetTexture9 = 0;
CreateVertexBuffer8_Type pCreateVertexBuffer8;
CreateVertexBuffer9_Type pCreateVertexBuffer9;

// IDirect3DTexture8/9 methods

typedef HRESULT (WINAPI *LockRect_Type)(void *, UINT, D3DLOCKED_RECT *, CONST RECT *, DWORD);
typedef HRESULT (WINAPI *UnlockRect_Type)(void *, UINT);
typedef HRESULT (WINAPI *GetLevelDesc_Type)(void *, UINT, D3DSURFACE_DESC *);
typedef HRESULT (WINAPI *GetSurfaceLevel_Type)(void *, UINT, void **);
typedef HRESULT (WINAPI *GetDepthStencilSurface9_Type)(void *, IDirect3DSurface9**);
typedef HRESULT (WINAPI *SetDepthStencilSurface9_Type)(void *, IDirect3DSurface9*);

HRESULT WINAPI extLockRect8(void *, UINT, D3DLOCKED_RECT *, CONST RECT *, DWORD);
HRESULT WINAPI extLockRect9(void *, UINT, D3DLOCKED_RECT *, CONST RECT *, DWORD);
HRESULT WINAPI extUnlockRect8(void *, UINT);
HRESULT WINAPI extUnlockRect9(void *, UINT);
#ifdef TRACEALL
HRESULT WINAPI extGetLevelDesc8(void *, UINT, D3DSURFACE_DESC *);
HRESULT WINAPI extGetSurfaceLevel8(void *, UINT, void **);
HRESULT WINAPI extGetDepthStencilSurface9(void *, IDirect3DSurface9**);
HRESULT WINAPI extSetDepthStencilSurface9(void *, IDirect3DSurface9*);
#endif

LockRect_Type pLockRect8, pLockRect9;
UnlockRect_Type pUnlockRect8, pUnlockRect9;
#ifdef TRACEALL
GetLevelDesc_Type pGetLevelDesc8;
GetSurfaceLevel_Type pGetSurfaceLevel8;
GetDepthStencilSurface9_Type pGetDepthStencilSurface9;
SetDepthStencilSurface9_Type pSetDepthStencilSurface9;
#endif

// swap chain
typedef HRESULT (WINAPI *PresentSwC8_Type)(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
typedef HRESULT (WINAPI *PresentSwC9_Type)(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *, DWORD);
PresentSwC8_Type pPresentSwC8;
PresentSwC9_Type pPresentSwC9;
HRESULT WINAPI extPresentSwC8(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
HRESULT WINAPI extPresentSwC9(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *, DWORD);

// IDirect3D10 ....

typedef HRESULT (WINAPI *D3D10CreateDevice_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, ID3D10Device **);
typedef HRESULT (WINAPI *D3D10CreateDeviceAndSwapChain_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
typedef HRESULT (WINAPI *D3D10CreateDevice1_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, D3D10_FEATURE_LEVEL1, UINT, ID3D10Device **);
typedef HRESULT (WINAPI *D3D10CreateDeviceAndSwapChain1_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
typedef void	(WINAPI *RSSetViewports10_Type)(void *, UINT, D3D10_VIEWPORT *);

HRESULT WINAPI extD3D10CreateDevice(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, ID3D10Device **);
HRESULT WINAPI extD3D10CreateDeviceAndSwapChain(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
HRESULT WINAPI extD3D10CreateDevice1(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, D3D10_FEATURE_LEVEL1, UINT, ID3D10Device **);
HRESULT WINAPI extD3D10CreateDeviceAndSwapChain1(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
void	WINAPI extRSSetViewports10(void *, UINT, D3D10_VIEWPORT *);

D3D10CreateDevice_Type pD3D10CreateDevice = 0;
D3D10CreateDeviceAndSwapChain_Type pD3D10CreateDeviceAndSwapChain = 0;
D3D10CreateDevice1_Type pD3D10CreateDevice1 = 0;
D3D10CreateDeviceAndSwapChain1_Type pD3D10CreateDeviceAndSwapChain1 = 0;
RSSetViewports10_Type pRSSetViewports10 = 0;

// d3d8 surface

typedef HRESULT (WINAPI *LockRectS8_Type)(void *, D3DLOCKED_RECT *, CONST RECT *, DWORD);
typedef HRESULT (WINAPI *UnlockRectS8_Type)(void *);

HRESULT WINAPI extLockRectS8(void *, D3DLOCKED_RECT *, CONST RECT *, DWORD);
HRESULT WINAPI extUnlockRectS8(void *);

LockRectS8_Type pLockRectS8;
UnlockRectS8_Type pUnlockRectS8;
