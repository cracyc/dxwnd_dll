#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <d3d.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhook.h"
#include "syslibs.h"
#include "dxhelper.h"

//#define TRACEALL
//#define TRACETEXTURE
//#define DUMPEXECUTEBUFFER
//#define DUMPEXECUTEBUFFERINSTRUCTIONS
//#define DUMPEXECUTEBUFFERVERTEX
//#define TRACEMATERIAL

#ifdef TRACEALL
#define TRACETEXTURE
#define DUMPEXECUTEBUFFER
#define TRACEMATERIAL
#endif // TRACEALL

extern void LimitFrameCount(DWORD);
extern LPDIRECTDRAW lpPrimaryDD;
extern char *ExplainDDError(DWORD);
static void LegacyD3DResolutionHack(int);
LPDIRECT3DVIEWPORT lpCurrViewport = NULL;

void HookVertexBuffer3(LPDIRECT3DVERTEXBUFFER *);
void HookVertexBuffer7(LPDIRECT3DVERTEXBUFFER7 *);

typedef HRESULT (WINAPI *QueryInterface_Type)(void *, REFIID, LPVOID *);
extern HRESULT WINAPI extQueryInterfaceDX(char *, int, QueryInterface_Type, void *, REFIID, LPVOID *);
char *sExecuteBufferCaps(DWORD);
LPDIRECT3DVIEWPORT gViewport1 = NULL;
static void DumpEBData(LPDIRECT3DEXECUTEBUFFER);
extern BOOL isWithinCreateDevice;

//----------------------------------------------------------------------//
// Hookers typedefs, prototypes, pointers
//----------------------------------------------------------------------//

// exported API

typedef HRESULT (WINAPI *Direct3DCreateDevice_Type)(GUID FAR *, LPDIRECT3D, LPDIRECTDRAWSURFACE, LPDIRECT3D *, LPUNKNOWN);
typedef HRESULT (WINAPI *Direct3DCreate_Type)(UINT, LPDIRECT3D *, LPUNKNOWN);

Direct3DCreateDevice_Type pDirect3DCreateDevice = NULL;
Direct3DCreate_Type pDirect3DCreate = NULL;

HRESULT WINAPI extDirect3DCreateDevice(GUID FAR *, LPDIRECT3D, LPDIRECTDRAWSURFACE, LPDIRECT3D *, LPUNKNOWN);
HRESULT WINAPI extDirect3DCreate(UINT, LPDIRECT3D *, LPUNKNOWN);

// IDirect3D-n interfaces

typedef HRESULT (WINAPI *QueryInterfaceD3_Type)(void *, REFIID, LPVOID *);
typedef HRESULT (WINAPI *Initialize_Type)(void *);
typedef HRESULT (WINAPI *EnumDevices_Type)(void *, LPD3DENUMDEVICESCALLBACK, LPVOID);
typedef HRESULT (WINAPI *EnumDevices7_Type)(void *, LPD3DENUMDEVICESCALLBACK7, LPVOID);
typedef HRESULT (WINAPI *CreateLight_Type)(void *, LPDIRECT3DLIGHT *, IUnknown *);
#ifdef TRACEMATERIAL
typedef HRESULT (WINAPI *CreateMaterial1_Type)(void *, LPDIRECT3DMATERIAL *, IUnknown *);
typedef HRESULT (WINAPI *CreateMaterial2_Type)(void *, LPDIRECT3DMATERIAL2 *, IUnknown *);
typedef HRESULT (WINAPI *CreateMaterial3_Type)(void *, LPDIRECT3DMATERIAL3 *, IUnknown *);
#endif // TRACEMATERIAL
typedef HRESULT (WINAPI *CreateViewport1_Type)(void *, LPDIRECT3DVIEWPORT *, IUnknown *);
typedef HRESULT (WINAPI *CreateViewport2_Type)(void *, LPDIRECT3DVIEWPORT2 *, IUnknown *);
typedef HRESULT (WINAPI *CreateViewport3_Type)(void *, LPDIRECT3DVIEWPORT3 *, IUnknown *);
typedef HRESULT (WINAPI *FindDevice_Type)(void *, LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
typedef HRESULT (WINAPI *CreateDevice2_Type)(void *, REFCLSID, LPDIRECTDRAWSURFACE, LPDIRECT3DDEVICE2 *);
typedef HRESULT (WINAPI *CreateDevice3_Type)(void *, REFCLSID, LPDIRECTDRAWSURFACE4, LPDIRECT3DDEVICE3 *, LPUNKNOWN);
typedef HRESULT (WINAPI *CreateDevice7_Type)(void *, REFCLSID, LPDIRECTDRAWSURFACE7, LPDIRECT3DDEVICE7 *);
typedef HRESULT (WINAPI *CreateVertexBuffer3_Type)(void *, LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER *, DWORD, LPUNKNOWN);
typedef HRESULT (WINAPI *CreateVertexBuffer7_Type)(void *, LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER7 *, DWORD);
typedef HRESULT (WINAPI *EnumZBufferFormats_Type)(void *, REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
typedef HRESULT (WINAPI *EnumTextureFormats12_Type)(void *, LPD3DENUMTEXTUREFORMATSCALLBACK, LPVOID);
typedef HRESULT (WINAPI *EnumTextureFormats37_Type)(void *, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);

QueryInterfaceD3_Type pQueryInterfaceD31 = NULL;
QueryInterfaceD3_Type pQueryInterfaceD32 = NULL;
QueryInterfaceD3_Type pQueryInterfaceD33 = NULL;
QueryInterfaceD3_Type pQueryInterfaceD37 = NULL;
Initialize_Type pInitialize = NULL;
EnumDevices_Type pEnumDevices1 = NULL;
EnumDevices_Type pEnumDevices2 = NULL;
EnumDevices_Type pEnumDevices3 = NULL;
EnumDevices7_Type pEnumDevices7 = NULL;
CreateLight_Type pCreateLight1 = NULL;
CreateLight_Type pCreateLight2 = NULL;
CreateLight_Type pCreateLight3 = NULL;
#ifdef TRACEMATERIAL
CreateMaterial1_Type pCreateMaterial1 = NULL;
CreateMaterial2_Type pCreateMaterial2 = NULL;
CreateMaterial3_Type pCreateMaterial3 = NULL;
#endif // TRACEMATERIAL
CreateViewport1_Type pCreateViewport1 = NULL;
CreateViewport2_Type pCreateViewport2 = NULL;
CreateViewport3_Type pCreateViewport3 = NULL;
FindDevice_Type pFindDevice1, pFindDevice2, pFindDevice3;
CreateDevice2_Type pCreateDevice2 = NULL;
CreateDevice3_Type pCreateDevice3 = NULL;
CreateDevice7_Type pCreateDevice7 = NULL;
CreateVertexBuffer3_Type pCreateVertexBuffer3;
CreateVertexBuffer7_Type pCreateVertexBuffer7;
EnumZBufferFormats_Type pEnumZBufferFormats3 = NULL;
EnumZBufferFormats_Type pEnumZBufferFormats7 = NULL;

EnumTextureFormats12_Type pEnumTextureFormats1, pEnumTextureFormats2;
EnumTextureFormats37_Type pEnumTextureFormats3, pEnumTextureFormats7;

HRESULT WINAPI extQueryInterfaceD31(void *, REFIID, LPVOID *);
HRESULT WINAPI extQueryInterfaceD32(void *, REFIID, LPVOID *);
HRESULT WINAPI extQueryInterfaceD33(void *, REFIID, LPVOID *);
HRESULT WINAPI extQueryInterfaceD37(void *, REFIID, LPVOID *);
HRESULT WINAPI extEnumDevices1(void *, LPD3DENUMDEVICESCALLBACK, LPVOID);
HRESULT WINAPI extEnumDevices2(void *, LPD3DENUMDEVICESCALLBACK, LPVOID);
HRESULT WINAPI extEnumDevices3(void *, LPD3DENUMDEVICESCALLBACK, LPVOID);
HRESULT WINAPI extEnumDevices7(void *, LPD3DENUMDEVICESCALLBACK7, LPVOID);
HRESULT WINAPI extCreateLight1(void *, LPDIRECT3DLIGHT *, IUnknown *);
HRESULT WINAPI extCreateLight2(void *, LPDIRECT3DLIGHT *, IUnknown *);
HRESULT WINAPI extCreateLight3(void *, LPDIRECT3DLIGHT *, IUnknown *);

HRESULT WINAPI extEnumZBufferFormats3(void *, REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
HRESULT WINAPI extEnumZBufferFormats7(void *, REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);

HRESULT WINAPI extEnumTextureFormats1(void *, LPD3DENUMTEXTUREFORMATSCALLBACK, LPVOID);
HRESULT WINAPI extEnumTextureFormats2(void *, LPD3DENUMTEXTUREFORMATSCALLBACK, LPVOID);
HRESULT WINAPI extEnumTextureFormats3(void *, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
HRESULT WINAPI extEnumTextureFormats7(void *, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);

// Direct3DDevice-n interfaces

typedef ULONG   (WINAPI *ReleaseD3D_Type)(LPDIRECT3DDEVICE);
typedef HRESULT (WINAPI *QueryInterfaceD3D_Type)(void *, REFIID, LPVOID *);
typedef HRESULT (WINAPI *D3DInitialize_Type)(void *, LPDIRECT3D , LPGUID, LPD3DDEVICEDESC);
typedef HRESULT (WINAPI *D3DGetCaps_Type)(void *, LPD3DDEVICEDESC ,LPD3DDEVICEDESC);
typedef HRESULT (WINAPI *D3DGetCaps7_Type)(void *, LPD3DDEVICEDESC7);
typedef HRESULT (WINAPI *AddViewport1_Type)(void *, LPDIRECT3DVIEWPORT);
typedef HRESULT (WINAPI *AddViewport2_Type)(void *, LPDIRECT3DVIEWPORT2);
typedef HRESULT (WINAPI *AddViewport3_Type)(void *, LPDIRECT3DVIEWPORT3);
typedef HRESULT (WINAPI *Scene_Type)(void *); // BeginScene, EndScene
typedef HRESULT (WINAPI *SetRenderState3_Type)(void *, D3DRENDERSTATETYPE, DWORD);
typedef HRESULT (WINAPI *SetLightState_Type)(void *, D3DLIGHTSTATETYPE, DWORD);
typedef HRESULT (WINAPI *GetCurrentViewport2_Type)(void *, LPDIRECT3DVIEWPORT2 *);
typedef HRESULT (WINAPI *SetCurrentViewport2_Type)(void *, LPDIRECT3DVIEWPORT2);
typedef HRESULT (WINAPI *GetCurrentViewport3_Type)(void *, LPDIRECT3DVIEWPORT3 *);
typedef HRESULT (WINAPI *SetCurrentViewport3_Type)(void *, LPDIRECT3DVIEWPORT3);
typedef HRESULT (WINAPI *GetTexture3_Type)(void *, DWORD, LPDIRECT3DTEXTURE2 *);
typedef HRESULT (WINAPI *GetTexture7_Type)(void *, DWORD, LPDIRECTDRAWSURFACE7 *);
typedef HRESULT (WINAPI *SetTexture3_Type)(void *, DWORD, LPDIRECT3DTEXTURE2);
typedef HRESULT (WINAPI *SetTexture7_Type)(void *, DWORD, LPDIRECTDRAWSURFACE7);
typedef HRESULT (WINAPI *SwapTextureHandles_Type)(void *, LPDIRECT3DTEXTURE, LPDIRECT3DTEXTURE);
typedef HRESULT (WINAPI *SwapTextureHandles2_Type)(void *, LPDIRECT3DTEXTURE2, LPDIRECT3DTEXTURE2);
typedef HRESULT (WINAPI *SetLight7_Type)(void *, DWORD, LPD3DLIGHT7);
typedef HRESULT (WINAPI *GetViewport7_Type)(void *, LPD3DVIEWPORT7);
typedef HRESULT (WINAPI *SetViewport7_Type)(void *, LPD3DVIEWPORT7);
typedef HRESULT (WINAPI *SetTransform_Type)(void *, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);

typedef HRESULT (WINAPI *CreateMatrix_Type)(void *, LPD3DMATRIXHANDLE);
typedef HRESULT (WINAPI *SetMatrix_Type)(void *, D3DMATRIXHANDLE, const LPD3DMATRIX);
typedef HRESULT (WINAPI *GetMatrix_Type)(void *, D3DMATRIXHANDLE, LPD3DMATRIX);
typedef HRESULT (WINAPI *DeleteMatrix_Type)(void *, D3DMATRIXHANDLE);
typedef HRESULT (WINAPI *DrawPrimitive2_Type)(void *, D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, DWORD);
typedef HRESULT (WINAPI *DrawPrimitive37_Type)(void *, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
typedef HRESULT (WINAPI *ClearD7_Type)(void *, DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD);

#ifdef TRACEALL
typedef HRESULT (WINAPI *GetLight7_Type)(void *, DWORD, LPD3DLIGHT7);
typedef HRESULT (WINAPI *SetRenderTarget_Type)(void *, LPDIRECTDRAWSURFACE7, DWORD);
typedef HRESULT (WINAPI *GetRenderTarget_Type)(void *, LPDIRECTDRAWSURFACE7 *);
typedef HRESULT (WINAPI *GetTextureStageState_Type)(void *, DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD);
typedef HRESULT (WINAPI *SetTextureStageState_Type)(void *, DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
#endif // TRACEALL
typedef HRESULT (WINAPI *CreateExecuteBuffer_Type)(void *, LPD3DEXECUTEBUFFERDESC, LPDIRECT3DEXECUTEBUFFER *, IUnknown *);
typedef HRESULT (WINAPI *Execute_Type)(void *, LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD);

QueryInterfaceD3_Type pQueryInterfaceD3D1, pQueryInterfaceD3D2, pQueryInterfaceD3D3, pQueryInterfaceD3D7;
ReleaseD3D_Type pReleaseD3D1, pReleaseD3D2, pReleaseD3D3, pReleaseD3D7;
D3DInitialize_Type pD3DInitialize = NULL;
D3DGetCaps_Type pD3DGetCaps1, pD3DGetCaps2, pD3DGetCaps3;
D3DGetCaps7_Type pD3DGetCaps7;
AddViewport1_Type pAddViewport1 = NULL;
AddViewport2_Type pAddViewport2 = NULL;
AddViewport3_Type pAddViewport3 = NULL;
Scene_Type pBeginScene1 = NULL;
Scene_Type pBeginScene2 = NULL;
Scene_Type pBeginScene3 = NULL;
Scene_Type pBeginScene7 = NULL;
Scene_Type pEndScene1 = NULL;
Scene_Type pEndScene2 = NULL;
Scene_Type pEndScene3 = NULL;
Scene_Type pEndScene7 = NULL;
SetRenderState3_Type pSetRenderState2 = NULL;
SetRenderState3_Type pSetRenderState3 = NULL;
SetRenderState3_Type pSetRenderState7 = NULL;
SetLightState_Type pSetLightState3 = NULL;
GetCurrentViewport2_Type pGetCurrentViewport2 = NULL;
SetCurrentViewport2_Type pSetCurrentViewport2 = NULL;
GetCurrentViewport3_Type pGetCurrentViewport3 = NULL;
SetCurrentViewport3_Type pSetCurrentViewport3 = NULL;
SetTexture3_Type pSetTexture3 = NULL;
SetTexture7_Type pSetTexture7 = NULL;
SwapTextureHandles_Type pSwapTextureHandles = NULL;
SwapTextureHandles2_Type pSwapTextureHandles2 = NULL;
SetLight7_Type pSetLight7 = NULL;
GetViewport7_Type pGetViewport7 = NULL;
SetViewport7_Type pSetViewport7 = NULL;
SetTransform_Type pSetTransform2, pSetTransform3, pSetTransform7;

CreateMatrix_Type pCreateMatrix;
SetMatrix_Type pSetMatrix;
GetMatrix_Type pGetMatrix;
DeleteMatrix_Type pDeleteMatrix;
DrawPrimitive2_Type pDrawPrimitive2;
DrawPrimitive37_Type pDrawPrimitive3, pDrawPrimitive7;
ClearD7_Type pClearD7;

#ifdef TRACEALL
GetLight7_Type pGetLight7;
SetRenderTarget_Type pSetRenderTarget7;
GetRenderTarget_Type pGetRenderTarget7;
GetTexture3_Type pGetTexture3;
GetTexture7_Type pGetTexture7;
GetTextureStageState_Type pGetTextureStageState3, pGetTextureStageState7;
SetTextureStageState_Type pSetTextureStageState3, pSetTextureStageState7;
#endif // TRACEALL
CreateExecuteBuffer_Type pCreateExecuteBuffer = NULL;
Execute_Type pExecute = NULL;

// IDirect3DViewport-n interfaces

typedef HRESULT (WINAPI *InitializeVP_Type)(void *, LPDIRECT3D);
typedef HRESULT (WINAPI *Pick_Type)(void *, LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD, LPD3DRECT);
typedef HRESULT (WINAPI *SetViewport_Type)(void *, LPD3DVIEWPORT);
typedef HRESULT (WINAPI *GetViewport_Type)(void *, LPD3DVIEWPORT);
typedef HRESULT (WINAPI *GetViewport2_Type)(void *, LPD3DVIEWPORT);
typedef HRESULT (WINAPI *SetViewport2_Type)(void *, LPD3DVIEWPORT);
typedef HRESULT (WINAPI *GetViewport3_Type)(void *, LPD3DVIEWPORT);
typedef HRESULT (WINAPI *SetViewport3_Type)(void *, LPD3DVIEWPORT);
typedef HRESULT (WINAPI *GetViewport2_3_Type)(void *, LPD3DVIEWPORT2);
typedef HRESULT (WINAPI *SetViewport2_3_Type)(void *, LPD3DVIEWPORT2);
typedef HRESULT (WINAPI *DeleteViewport1_Type)(void *, LPDIRECT3DVIEWPORT);
typedef HRESULT (WINAPI *NextViewport1_Type)(void *, LPDIRECT3DVIEWPORT, LPDIRECT3DVIEWPORT *, DWORD);
typedef HRESULT (WINAPI *DeleteViewport2_Type)(void *, LPDIRECT3DVIEWPORT2);
typedef HRESULT (WINAPI *NextViewport2_Type)(void *, LPDIRECT3DVIEWPORT2, LPDIRECT3DVIEWPORT2 *, DWORD);
typedef HRESULT (WINAPI *ViewportClear_Type)(void *, DWORD, LPD3DRECT, DWORD);
#ifdef TRACEALL
typedef HRESULT (WINAPI *TransformVertices_Type)(void *, DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
typedef HRESULT (WINAPI *LightElements_Type)(void *, DWORD, LPD3DLIGHTDATA);
typedef HRESULT (WINAPI *SetBackground_Type)(void *, D3DMATERIALHANDLE);
typedef HRESULT (WINAPI *GetBackground_Type)(void *, LPD3DMATERIALHANDLE, LPBOOL);
typedef HRESULT (WINAPI *SetBackgroundDepth_Type)(void *, LPDIRECTDRAWSURFACE);
typedef HRESULT (WINAPI *GetBackgroundDepth_Type)(void *, LPDIRECTDRAWSURFACE *, LPBOOL);
typedef HRESULT (WINAPI *Clear_Type)(void *, DWORD, LPD3DRECT, DWORD);
typedef HRESULT (WINAPI *AddLight_Type)(void *, LPDIRECT3DLIGHT);
typedef HRESULT (WINAPI *DeleteLight_Type)(void *, LPDIRECT3DLIGHT);
typedef HRESULT (WINAPI *NextLight_Type)(void *, LPDIRECT3DLIGHT, LPDIRECT3DLIGHT *, DWORD);
typedef HRESULT (WINAPI *LightEnable_Type)(void *, DWORD, BOOL);
typedef HRESULT (WINAPI *GetLightEnable_Type)(void *, DWORD, BOOL *);
typedef HRESULT (WINAPI *SetClipPlane_Type)(void *, DWORD, D3DVALUE *);
typedef HRESULT (WINAPI *GetClipPlane_Type)(void *, DWORD, D3DVALUE *);
#endif // TRACEALL

#ifdef TRACEMATERIAL
// IDirect3DMaterial interfaces

typedef HRESULT (WINAPI *SetMaterial_Type)(void *, LPD3DMATERIAL);
typedef HRESULT (WINAPI *GetMaterial_Type)(void *, LPD3DMATERIAL);
typedef HRESULT (WINAPI *SetMaterial7_Type)(void *, LPD3DMATERIAL7);
typedef HRESULT (WINAPI *GetMaterial7_Type)(void *, LPD3DMATERIAL7);
#endif // TRACEMATERIAL

InitializeVP_Type pInitializeVP = NULL; // ???
DeleteViewport1_Type pDeleteViewport1 = NULL;
DeleteViewport2_Type pDeleteViewport2 = NULL;
NextViewport1_Type pNextViewport1 = NULL;
NextViewport2_Type pNextViewport2 = NULL;
ViewportClear_Type pViewportClear = NULL;
SetViewport_Type pSetViewport1 = NULL;
GetViewport_Type pGetViewport1 = NULL;
GetViewport2_Type pGetViewport2 = NULL;
SetViewport2_Type pSetViewport2 = NULL;
GetViewport3_Type pGetViewport3 = NULL;
SetViewport3_Type pSetViewport3 = NULL;
GetViewport2_3_Type pGetViewport2_2 = NULL;
SetViewport2_3_Type pSetViewport2_2 = NULL;
SetViewport2_3_Type pSetViewport2_3 = NULL;
GetViewport2_3_Type pGetViewport2_3 = NULL;
#ifdef TRACEALL
TransformVertices_Type pTransformVertices1, pTransformVertices2, pTransformVertices3;
LightElements_Type pLightElements1, pLightElements2, pLightElements3;
SetBackground_Type pSetBackground1, pSetBackground2, pSetBackground3;
GetBackground_Type pGetBackground1, pGetBackground2, pGetBackground3;
SetBackgroundDepth_Type pSetBackgroundDepth1, pSetBackgroundDepth2, pSetBackgroundDepth3;
GetBackgroundDepth_Type pGetBackgroundDepth1, pGetBackgroundDepth2, pGetBackgroundDepth3;
Clear_Type pClear1, pClear2, pClear3;
AddLight_Type pAddLight1, pAddLight2, pAddLight3;
DeleteLight_Type pDeleteLight1, pDeleteLight2, pDeleteLight3;
NextLight_Type pNextLight1, pNextLight2, pNextLight3;
LightEnable_Type pLightEnable7;
GetLightEnable_Type pGetLightEnable7;
SetClipPlane_Type pSetClipPlane7;
GetClipPlane_Type pGetClipPlane7;
#endif // TRACEALL

#ifdef TRACEMATERIAL
SetMaterial_Type pSetMaterial1, pSetMaterial2, pSetMaterial3;
GetMaterial_Type pGetMaterial1, pGetMaterial2, pGetMaterial3;
SetMaterial7_Type pSetMaterial7;
GetMaterial7_Type pGetMaterial7;
Pick_Type pPick = NULL;
#endif // TRACEMATERIAL

#ifdef TRACEMATERIAL
HRESULT WINAPI extCreateMaterial1(void *, LPDIRECT3DMATERIAL *, IUnknown *);
HRESULT WINAPI extCreateMaterial2(void *, LPDIRECT3DMATERIAL2 *, IUnknown *);
HRESULT WINAPI extCreateMaterial3(void *, LPDIRECT3DMATERIAL3 *, IUnknown *);
HRESULT WINAPI extPick(void *, LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD, LPD3DRECT);
HRESULT WINAPI extInitialize(void *);
#endif // TRACEMATERIAL
HRESULT WINAPI extCreateViewport1(void *, LPDIRECT3DVIEWPORT *, IUnknown *);
HRESULT WINAPI extCreateViewport2(void *, LPDIRECT3DVIEWPORT2 *, IUnknown *);
HRESULT WINAPI extCreateViewport3(void *, LPDIRECT3DVIEWPORT3 *, IUnknown *);
HRESULT WINAPI extFindDevice1(void *, LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
HRESULT WINAPI extFindDevice2(void *, LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
HRESULT WINAPI extFindDevice3(void *, LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
HRESULT WINAPI extCreateDevice2(void *, REFCLSID, LPDIRECTDRAWSURFACE, LPDIRECT3DDEVICE2 *);
HRESULT WINAPI extCreateDevice3(void *, REFCLSID, LPDIRECTDRAWSURFACE4, LPDIRECT3DDEVICE3 *, LPUNKNOWN);
HRESULT WINAPI extCreateDevice7(void *, REFCLSID, LPDIRECTDRAWSURFACE7, LPDIRECT3DDEVICE7 *);
HRESULT WINAPI extCreateVertexBuffer3(void *, LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER *, DWORD, LPUNKNOWN);
HRESULT WINAPI extCreateVertexBuffer7(void *, LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER7 *, DWORD);
HRESULT WINAPI extDeleteViewport1(void *, LPDIRECT3DVIEWPORT);
HRESULT WINAPI extNextViewport1(void *, LPDIRECT3DVIEWPORT, LPDIRECT3DVIEWPORT *, DWORD);
HRESULT WINAPI extDeleteViewport2(void *, LPDIRECT3DVIEWPORT2);
HRESULT WINAPI extNextViewport2(void *, LPDIRECT3DVIEWPORT2, LPDIRECT3DVIEWPORT2 *, DWORD);
HRESULT WINAPI extViewportClear(void *, DWORD, LPD3DRECT, DWORD);

HRESULT WINAPI extInitializeVP(void *, LPDIRECT3D);
HRESULT WINAPI extSetViewport1(void *, LPD3DVIEWPORT);
HRESULT WINAPI extGetViewport1(void *, LPD3DVIEWPORT);
HRESULT WINAPI extSetViewport2(void *, LPD3DVIEWPORT);
HRESULT WINAPI extGetViewport2(void *, LPD3DVIEWPORT);
HRESULT WINAPI extSetViewport3(void *, LPD3DVIEWPORT);
HRESULT WINAPI extGetViewport3(void *, LPD3DVIEWPORT);
#ifdef TRACEALL
HRESULT WINAPI extTransformVertices1(void *, DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
HRESULT WINAPI extTransformVertices2(void *, DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
HRESULT WINAPI extTransformVertices3(void *, DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
HRESULT WINAPI extLightElements1(void *, DWORD, LPD3DLIGHTDATA);
HRESULT WINAPI extLightElements2(void *, DWORD, LPD3DLIGHTDATA);
HRESULT WINAPI extLightElements3(void *, DWORD, LPD3DLIGHTDATA);
HRESULT WINAPI extSetBackground1(void *, D3DMATERIALHANDLE);
HRESULT WINAPI extSetBackground2(void *, D3DMATERIALHANDLE);
HRESULT WINAPI extSetBackground3(void *, D3DMATERIALHANDLE);
HRESULT WINAPI extGetBackground1(void *, LPD3DMATERIALHANDLE, LPBOOL);
HRESULT WINAPI extGetBackground2(void *, LPD3DMATERIALHANDLE, LPBOOL);
HRESULT WINAPI extGetBackground3(void *, LPD3DMATERIALHANDLE, LPBOOL);
HRESULT WINAPI extSetBackgroundDepth1(void *, LPDIRECTDRAWSURFACE);
HRESULT WINAPI extSetBackgroundDepth2(void *, LPDIRECTDRAWSURFACE);
HRESULT WINAPI extSetBackgroundDepth3(void *, LPDIRECTDRAWSURFACE);
HRESULT WINAPI extGetBackgroundDepth1(void *, LPDIRECTDRAWSURFACE *, LPBOOL);
HRESULT WINAPI extGetBackgroundDepth2(void *, LPDIRECTDRAWSURFACE *, LPBOOL);
HRESULT WINAPI extGetBackgroundDepth3(void *, LPDIRECTDRAWSURFACE *, LPBOOL);
HRESULT WINAPI extClear1(void *, DWORD, LPD3DRECT, DWORD);
HRESULT WINAPI extClear2(void *, DWORD, LPD3DRECT, DWORD);
HRESULT WINAPI extClear3(void *, DWORD, LPD3DRECT, DWORD);
HRESULT WINAPI extAddLight1(void *, LPDIRECT3DLIGHT);
HRESULT WINAPI extAddLight2(void *, LPDIRECT3DLIGHT);
HRESULT WINAPI extAddLight3(void *, LPDIRECT3DLIGHT);
HRESULT WINAPI extDeleteLight1(void *, LPDIRECT3DLIGHT);
HRESULT WINAPI extDeleteLight2(void *, LPDIRECT3DLIGHT);
HRESULT WINAPI extDeleteLight3(void *, LPDIRECT3DLIGHT);
HRESULT WINAPI extNextLight1(void *, LPDIRECT3DLIGHT, LPDIRECT3DLIGHT *, DWORD);
HRESULT WINAPI extNextLight2(void *, LPDIRECT3DLIGHT, LPDIRECT3DLIGHT *, DWORD);
HRESULT WINAPI extNextLight3(void *, LPDIRECT3DLIGHT, LPDIRECT3DLIGHT *, DWORD);
#endif // TRACEALL

#ifdef TRACEMATERIAL
HRESULT WINAPI extSetMaterial1(void *, LPD3DMATERIAL);
HRESULT WINAPI extGetMaterial1(void *, LPD3DMATERIAL);
HRESULT WINAPI extSetMaterial2(void *, LPD3DMATERIAL);
HRESULT WINAPI extGetMaterial2(void *, LPD3DMATERIAL);
HRESULT WINAPI extSetMaterial3(void *, LPD3DMATERIAL);
HRESULT WINAPI extGetMaterial3(void *, LPD3DMATERIAL);
HRESULT WINAPI extSetMaterial7(void *, LPD3DMATERIAL7);
HRESULT WINAPI extGetMaterial7(void *, LPD3DMATERIAL7);
#endif // TRACEMATERIAL

HRESULT WINAPI extQueryInterfaceD3(void *, REFIID, LPVOID *);
HRESULT WINAPI extQueryInterfaceD3D1(void *, REFIID, LPVOID *);
HRESULT WINAPI extQueryInterfaceD3D2(void *, REFIID, LPVOID *);
HRESULT WINAPI extQueryInterfaceD3D3(void *, REFIID, LPVOID *);
HRESULT WINAPI extQueryInterfaceD3D7(void *, REFIID, LPVOID *);

#ifdef TRACEALL
ULONG WINAPI extReleaseD3D1(LPDIRECT3DDEVICE);
ULONG WINAPI extReleaseD3D2(LPDIRECT3DDEVICE);
ULONG WINAPI extReleaseD3D3(LPDIRECT3DDEVICE);
ULONG WINAPI extReleaseD3D7(LPDIRECT3DDEVICE);
#endif // TRACEALL
HRESULT WINAPI extCreateMatrix(void *, LPD3DMATRIXHANDLE);
HRESULT WINAPI extSetMatrix(void *, D3DMATRIXHANDLE, const LPD3DMATRIX);
HRESULT WINAPI extGetMatrix(void *, D3DMATRIXHANDLE, LPD3DMATRIX);
HRESULT WINAPI extDeleteMatrix(void *, D3DMATRIXHANDLE);

HRESULT WINAPI extBeginScene1(void *);
HRESULT WINAPI extEndScene1(void *);
HRESULT WINAPI extBeginScene2(void *);
HRESULT WINAPI extEndScene2(void *);
HRESULT WINAPI extBeginScene3(void *);
HRESULT WINAPI extEndScene3(void *);
HRESULT WINAPI extBeginScene7(void *);
HRESULT WINAPI extEndScene7(void *);
HRESULT WINAPI extSetRenderState2(void *, D3DRENDERSTATETYPE, DWORD);
HRESULT WINAPI extSetRenderState3(void *, D3DRENDERSTATETYPE, DWORD);
HRESULT WINAPI extSetRenderState7(void *, D3DRENDERSTATETYPE, DWORD);
HRESULT WINAPI extD3DGetCaps1(void *, LPD3DDEVICEDESC, LPD3DDEVICEDESC);
HRESULT WINAPI extD3DGetCaps2(void *, LPD3DDEVICEDESC, LPD3DDEVICEDESC);
HRESULT WINAPI extD3DGetCaps3(void *, LPD3DDEVICEDESC, LPD3DDEVICEDESC);
HRESULT WINAPI extD3DGetCaps7(void *, LPD3DDEVICEDESC7);
HRESULT WINAPI extSetLightState3(void *d3dd, D3DLIGHTSTATETYPE d3dls, DWORD t);
HRESULT WINAPI extSetViewport3(void *, LPD3DVIEWPORT);
HRESULT WINAPI extGetViewport3(void *, LPD3DVIEWPORT);
HRESULT WINAPI extAddViewport1(void *, LPDIRECT3DVIEWPORT);
HRESULT WINAPI extAddViewport2(void *, LPDIRECT3DVIEWPORT2);
HRESULT WINAPI extAddViewport3(void *, LPDIRECT3DVIEWPORT3);
HRESULT WINAPI extGetViewport2(void *, LPD3DVIEWPORT);
HRESULT WINAPI extSetViewport2(void *, LPD3DVIEWPORT);
HRESULT WINAPI extGetViewport2_2(void *, LPD3DVIEWPORT2);
HRESULT WINAPI extSetViewport2_2(void *, LPD3DVIEWPORT2);
HRESULT WINAPI extGetViewport2_3(void *, LPD3DVIEWPORT2);
HRESULT WINAPI extSetViewport2_3(void *, LPD3DVIEWPORT2);
HRESULT WINAPI extSetCurrentViewport2(void *, LPDIRECT3DVIEWPORT2);
HRESULT WINAPI extGetCurrentViewport2(void *, LPDIRECT3DVIEWPORT2 *);
HRESULT WINAPI extSetCurrentViewport3(void *, LPDIRECT3DVIEWPORT3);
HRESULT WINAPI extGetCurrentViewport3(void *, LPDIRECT3DVIEWPORT3 *);
HRESULT WINAPI extSetViewport7(void *, LPD3DVIEWPORT7);
HRESULT WINAPI extGetViewport7(void *, LPD3DVIEWPORT7);
HRESULT WINAPI extSetTexture3(void *, DWORD, LPDIRECT3DTEXTURE2);
HRESULT WINAPI extSetTexture7(void *, DWORD, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI extSwapTextureHandles(void *, LPDIRECT3DTEXTURE, LPDIRECT3DTEXTURE);
HRESULT WINAPI extSwapTextureHandles2(void *, LPDIRECT3DTEXTURE2, LPDIRECT3DTEXTURE2);
HRESULT WINAPI extSetLight7(void *, DWORD, LPD3DLIGHT7);
HRESULT WINAPI extSetTransform3(void *, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
HRESULT WINAPI extSetTransform7(void *, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
HRESULT WINAPI extSetTransform2(void *, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
HRESULT WINAPI extDrawPrimitive2(void *, D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, DWORD);
HRESULT WINAPI extDrawPrimitive3(void *, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
HRESULT WINAPI extDrawPrimitive7(void *, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
HRESULT WINAPI extClearD7(void *, DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD);
#ifdef TRACEALL
HRESULT WINAPI extGetLight7(void *, DWORD, LPD3DLIGHT7);
HRESULT WINAPI extSetRenderTarget7(void *, LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI extGetRenderTarget7(void *, LPDIRECTDRAWSURFACE7 *);
HRESULT WINAPI extGetTexture3(void *, DWORD, LPDIRECT3DTEXTURE2 *);
HRESULT WINAPI extGetTexture7(void *, DWORD, LPDIRECTDRAWSURFACE7 *);
HRESULT WINAPI extGetTextureStageState3(void *, DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD);
HRESULT WINAPI extGetTextureStageState7(void *, DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD);
HRESULT WINAPI extSetTextureStageState3(void *, DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
HRESULT WINAPI extSetTextureStageState7(void *, DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
HRESULT WINAPI extLightEnable7(void *, DWORD, BOOL);
HRESULT WINAPI extGetLightEnable7(void *, DWORD, BOOL *);
HRESULT WINAPI extSetClipPlane7(void *, DWORD, D3DVALUE *);
HRESULT WINAPI extGetClipPlane7(void *, DWORD, D3DVALUE *);
#endif // TRACEALL

HRESULT WINAPI extCreateExecuteBuffer(void *, LPD3DEXECUTEBUFFERDESC, LPDIRECT3DEXECUTEBUFFER *, IUnknown *);
HRESULT WINAPI extExecute(void *, LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD);

// Texture

#ifdef TRACETEXTURE
typedef HRESULT (WINAPI *TexInitialize_Type)(void *, LPDIRECT3DDEVICE, LPDIRECTDRAWSURFACE);
typedef HRESULT (WINAPI *TexGetHandle_Type)(void *, LPDIRECT3DDEVICE, LPD3DTEXTUREHANDLE);
typedef HRESULT (WINAPI *TexPaletteChanged_Type)(void *, DWORD, DWORD);
typedef HRESULT (WINAPI *TexLoad_Type)(void *, LPDIRECT3DTEXTURE);
typedef HRESULT (WINAPI *TexUnload_Type)(void *);

TexInitialize_Type pTInitialize = NULL;
TexGetHandle_Type pTGetHandle1, pTGetHandle2;
TexPaletteChanged_Type pTPaletteChanged1, pTPaletteChanged2;
TexLoad_Type pTLoad1, pTLoad2;
TexUnload_Type pTUnload = NULL;

HRESULT WINAPI extTexInitialize(void *, LPDIRECT3DDEVICE, LPDIRECTDRAWSURFACE);
HRESULT WINAPI extTexGetHandle1(void *, LPDIRECT3DDEVICE, LPD3DTEXTUREHANDLE);
HRESULT WINAPI extTexGetHandle2(void *, LPDIRECT3DDEVICE2, LPD3DTEXTUREHANDLE);
HRESULT WINAPI extTexPaletteChanged1(void *, DWORD, DWORD);
HRESULT WINAPI extTexPaletteChanged2(void *, DWORD, DWORD);
HRESULT WINAPI extTexLoad1(void *, LPDIRECT3DTEXTURE);
HRESULT WINAPI extTexLoad2(void *, LPDIRECT3DTEXTURE);
HRESULT WINAPI extTexUnload(void *);
#endif // TRACETEXTURE


#ifdef TRACEALL
typedef HRESULT (WINAPI *Validate_Type)(void *, LPDWORD, LPD3DVALIDATECALLBACK, LPVOID, DWORD);
typedef HRESULT (WINAPI *EBInitialize_Type)(void *, LPDIRECT3DDEVICE, LPD3DEXECUTEBUFFERDESC);
Validate_Type pValidate;
EBInitialize_Type pEBInitialize;
HRESULT WINAPI extValidate(void *, LPDWORD, LPD3DVALIDATECALLBACK, LPVOID, DWORD);
HRESULT WINAPI extEBInitialize(void *, LPDIRECT3DDEVICE, LPD3DEXECUTEBUFFERDESC);
#endif // TRACEALL
typedef HRESULT (WINAPI *Optimize_Type)(void *, DWORD);
Optimize_Type pOptimize;
HRESULT WINAPI extOptimize(void *, DWORD);

typedef HRESULT (WINAPI *EBLock_Type)(void *, LPD3DEXECUTEBUFFERDESC);
typedef HRESULT (WINAPI *EBUnlock_Type)(void *);
typedef HRESULT (WINAPI *SetExecuteData_Type)(void *, LPD3DEXECUTEDATA);
typedef HRESULT (WINAPI *GetExecuteData_Type)(void *, LPD3DEXECUTEDATA);
typedef HRESULT (WINAPI *GetLightState2_Type)(void *, D3DLIGHTSTATETYPE, LPDWORD);
typedef HRESULT (WINAPI *SetLightState2_Type)(void *, D3DLIGHTSTATETYPE, DWORD);
EBLock_Type pEBLock;
EBUnlock_Type pEBUnlock;
SetExecuteData_Type pSetExecuteData;
GetExecuteData_Type pGetExecuteData;
GetLightState2_Type pGetLightState2;
SetLightState2_Type pSetLightState2;
HRESULT WINAPI extEBLock(void *, LPD3DEXECUTEBUFFERDESC);
HRESULT WINAPI extEBUnlock(void *);
HRESULT WINAPI extSetExecuteData(void *, LPD3DEXECUTEDATA);
HRESULT WINAPI extGetExecuteData(void *, LPD3DEXECUTEDATA);
HRESULT WINAPI extGetLightState2(void *, D3DLIGHTSTATETYPE, LPDWORD);
HRESULT WINAPI extSetLightState2(void *, D3DLIGHTSTATETYPE, DWORD);

//----------------------------------------------------------------------//
// Hooking procedures
//----------------------------------------------------------------------//

// v2.05.45: 
// BEWARE: since both these calls seem to be used internally by COM methods, QueryInterface in particular,
// a hot patching creates the risk of hooking these objects with the wrong interface version, then creating
// a huge mess. Better leave them patched at IAT level only, and maybe only enabling hot patching in
// exceptional cases. This fixes "WhiteWater Rapids" attempt to force 16 bit desktop through hot patching.

static HookEntryEx_Type d3dHooks[]={
	{HOOK_IAT_CANDIDATE, 0x00, "Direct3DCreate", (FARPROC)NULL, (FARPROC *)&pDirect3DCreate, (FARPROC)extDirect3DCreate},
	{HOOK_IAT_CANDIDATE, 0x00, "Direct3DCreateDevice", (FARPROC)NULL, (FARPROC *)&pDirect3DCreateDevice, (FARPROC)extDirect3DCreateDevice},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookDirect3D(HMODULE module, char *name){
	ApiName("dxwnd.HookDirect3D");
	LPDIRECT3D lpd3d=NULL;
	int version = dxw.dwTargetDDVersion;

	OutTraceDW("%s: module=%#x name=%s version=%d\n", ApiRef, module, name, version);

	HookLibraryEx(module, d3dHooks, name);

	if(dxw.dwFlags12 & DIRECTXREPLACE){
		//if(IsHookedBlock(d3dHooks)){ -- better force operation in any case?
			char path[MAX_PATH];
			sprintf(path, "%salt.dll\\%s", GetDxWndPath(), name);
			PinLibraryEx(d3dHooks, path);
			OutTrace("%s: alt.dll loaded path=%s\n", ApiRef, path);
		//}
	}
}

void HookDirect3D16(HMODULE module){
	HookDirect3D(module, "d3dim.dll");
}

void HookDirect3D7(HMODULE module){
	HookDirect3D(module, "d3dim700.dll");
}

FARPROC Remap_d3d7_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	ApiName("dxwnd.Remap_d3d7_ProcAddress");
	if (!strcmp(proc,"Direct3DCreate") && !pDirect3DCreate){
		pDirect3DCreate=(Direct3DCreate_Type)(*pGetProcAddress)(hModule, proc);
		OutTraceD3D("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), pDirect3DCreate);
		return (FARPROC)extDirect3DCreate;
	}
	if (!strcmp(proc,"Direct3DCreateDevice") && !pDirect3DCreateDevice){
		pDirect3DCreateDevice=(Direct3DCreateDevice_Type)(*pGetProcAddress)(hModule, proc);
		OutTraceD3D("%s: hooking proc=%s at addr=%#x\n", ApiRef, ProcToString(proc), pDirect3DCreateDevice);
		return (FARPROC)extDirect3DCreateDevice;
	}
	// NULL -> keep the original call address
	return NULL;
}

void HookDirect3DSession(LPDIRECTDRAW *lplpdd, int d3dversion)
{
	ApiName("dxwnd.HookDirect3DSession");
	OutTraceD3D("%s: d3d=%#x d3dversion=%d\n", ApiRef, *lplpdd, d3dversion);

	if(dxw.dwFlags9 & D3DRESOLUTIONHACK) LegacyD3DResolutionHack(d3dversion);

	switch(d3dversion){
	case 1:
		SetHook((void *)(**(DWORD **)lplpdd +   0), extQueryInterfaceD31, (void **)&pQueryInterfaceD31, "QueryInterface(D3S1)");
#ifdef TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  12), extInitialize, (void **)&pInitialize, "Initialize(1)");
#endif // TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  16), extEnumDevices1, (void **)&pEnumDevices1, "EnumDevices(1)");
		SetHook((void *)(**(DWORD **)lplpdd +  20), extCreateLight1, (void **)&pCreateLight1, "CreateLight(1)");
#ifdef TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  24), extCreateMaterial1, (void **)&pCreateMaterial1, "CreateMaterial(1)");
#endif // TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  28), extCreateViewport1, (void **)&pCreateViewport1, "CreateViewport(1)");
		SetHook((void *)(**(DWORD **)lplpdd +  32), extFindDevice1, (void **)&pFindDevice1, "FindDevice(1)");	
		break;
	case 2:
		SetHook((void *)(**(DWORD **)lplpdd +   0), extQueryInterfaceD32, (void **)&pQueryInterfaceD32, "QueryInterface(D3S2)");
		SetHook((void *)(**(DWORD **)lplpdd +  12), extEnumDevices2, (void **)&pEnumDevices2, "EnumDevices(2)");
		SetHook((void *)(**(DWORD **)lplpdd +  16), extCreateLight2, (void **)&pCreateLight2, "CreateLight(2)");
#ifdef TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  20), extCreateMaterial2, (void **)&pCreateMaterial2, "CreateMaterial(2)");
#endif // TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  24), extCreateViewport2, (void **)&pCreateViewport2, "CreateViewport(2)");
		SetHook((void *)(**(DWORD **)lplpdd +  28), extFindDevice2, (void **)&pFindDevice2, "FindDevice(2)");
		SetHook((void *)(**(DWORD **)lplpdd +  32), extCreateDevice2, (void **)&pCreateDevice2, "CreateDevice(D3D2)");
		break;
	case 3:
		SetHook((void *)(**(DWORD **)lplpdd +   0), extQueryInterfaceD33, (void **)&pQueryInterfaceD33, "QueryInterface(D3S3)");
		SetHook((void *)(**(DWORD **)lplpdd +  12), extEnumDevices3, (void **)&pEnumDevices3, "EnumDevices(3)");
		SetHook((void *)(**(DWORD **)lplpdd +  16), extCreateLight3, (void **)&pCreateLight3, "CreateLight(3)");
#ifdef TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  20), extCreateMaterial3, (void **)&pCreateMaterial3, "CreateMaterial(3)");
#endif // TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lplpdd +  24), extCreateViewport3, (void **)&pCreateViewport3, "CreateViewport(3)");
		SetHook((void *)(**(DWORD **)lplpdd +  28), extFindDevice3, (void **)&pFindDevice3, "FindDevice(3)");
		SetHook((void *)(**(DWORD **)lplpdd +  32), extCreateDevice3, (void **)&pCreateDevice3, "CreateDevice(D3D3)");
		SetHook((void *)(**(DWORD **)lplpdd +  36), extCreateVertexBuffer3, (void **)&pCreateVertexBuffer3, "CreateVertexBuffer(D3D3)");
		SetHook((void *)(**(DWORD **)lplpdd +  40), extEnumZBufferFormats3, (void **)&pEnumZBufferFormats3, "EnumZBufferFormats(D3D3)");
		break;
	case 7:
		SetHook((void *)(**(DWORD **)lplpdd +   0), extQueryInterfaceD37, (void **)&pQueryInterfaceD37, "QueryInterface(D3S7)");
		SetHook((void *)(**(DWORD **)lplpdd +  12), extEnumDevices7, (void **)&pEnumDevices7, "EnumDevices(7)");
		SetHook((void *)(**(DWORD **)lplpdd +  16), extCreateDevice7, (void **)&pCreateDevice7, "CreateDevice(D3D7)");
		SetHook((void *)(**(DWORD **)lplpdd +  20), extCreateVertexBuffer7, (void **)&pCreateVertexBuffer7, "CreateVertexBuffer(D3D7)");
		SetHook((void *)(**(DWORD **)lplpdd +  24), extEnumZBufferFormats7, (void **)&pEnumZBufferFormats7, "EnumZBufferFormats(D3D7)");
		break;
	}
} 

void HookDirect3DDevice(void **lpd3ddev, int d3dversion)
{
	ApiName("dxwnd.HookDirect3DDevice");
	OutTraceD3D("%s: d3ddev=%#x d3dversion=%d\n", ApiRef, *lpd3ddev, d3dversion);

	switch(d3dversion){
	case 1:
		SetHook((void *)(**(DWORD **)lpd3ddev +   0), extQueryInterfaceD3D1, (void **)&pQueryInterfaceD3D1, "QueryInterface(D3DD1)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +   8), extReleaseD3D1, (void **)&pReleaseD3D1, "ReleaseD3D(1)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  16), extD3DGetCaps1, (void **)&pD3DGetCaps1, "GetCaps(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  20), extSwapTextureHandles, (void **)&pSwapTextureHandles, "SwapTextureHandles(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  24), extCreateExecuteBuffer, (void **)&pCreateExecuteBuffer, "CreateExecuteBuffer(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  32), extExecute, (void **)&pExecute, "Execute(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  36), extAddViewport1, (void **)&pAddViewport1, "AddViewport(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  40), extDeleteViewport1, (void **)&pDeleteViewport1, "DeleteViewport(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  44), extNextViewport1, (void **)&pNextViewport1, "NextViewport(1)");
#ifdef TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lpd3ddev +  48), extPick, (void **)&pPick, "Pick(1)");
#endif // TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lpd3ddev +  56), extEnumTextureFormats1, (void **)&pEnumTextureFormats1, "EnumTextureFormats(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  60), extCreateMatrix, (void **)&pCreateMatrix, "CreateMatrix(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  64), extSetMatrix, (void **)&pSetMatrix, "SetMatrix(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  68), extGetMatrix, (void **)&pGetMatrix, "GetMatrix(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  72), extDeleteMatrix, (void **)&pDeleteMatrix, "DeleteMatrix(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  76), extBeginScene1, (void **)&pBeginScene1, "BeginScene(1)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  80), extEndScene1, (void **)&pEndScene1, "EndScene(1)");
		break;
	case 2:
		SetHook((void *)(**(DWORD **)lpd3ddev +   0), extQueryInterfaceD3D2, (void **)&pQueryInterfaceD3D2, "QueryInterface(D3DD2)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +   8), extReleaseD3D2, (void **)&pReleaseD3D2, "ReleaseD3D(2)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  12), extD3DGetCaps2, (void **)&pD3DGetCaps2, "GetCaps(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  16), extSwapTextureHandles2, (void **)&pSwapTextureHandles2, "SwapTextureHandles(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  24), extAddViewport2, (void **)&pAddViewport2, "AddViewport(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  28), extDeleteViewport2, (void **)&pDeleteViewport2, "DeleteViewport(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  32), extNextViewport2, (void **)&pNextViewport2, "NextViewport(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  36), extEnumTextureFormats2, (void **)&pEnumTextureFormats2, "EnumTextureFormats(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  40), extBeginScene2, (void **)&pBeginScene2, "BeginScene(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  44), extEndScene2, (void **)&pEndScene2, "EndScene(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  52), extSetCurrentViewport2, (void **)&pSetCurrentViewport2, "SetCurrentViewport(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  56), extGetCurrentViewport2, (void **)&pGetCurrentViewport2, "GetCurrentViewport(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  92), extSetRenderState2, (void **)&pSetRenderState2, "SetRenderState(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  96), extGetLightState2, (void **)&pGetLightState2, "GetLightState(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 100), extSetLightState2, (void **)&pSetLightState2, "SetLightState(2)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 104), extSetTransform2, (void **)&pSetTransform2, "SetTransform(2)"); // v2.04.78 fix
		SetHook((void *)(**(DWORD **)lpd3ddev + 116), extDrawPrimitive2, (void **)&pDrawPrimitive2, "DrawPrimitive(2)"); 
		if(pSetRenderState2){
			if(dxw.dwFlags2 & WIREFRAME)(*pSetRenderState2)(*lpd3ddev, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME); 		
			if(dxw.dwFlags4 & DISABLEFOGGING) (*pSetRenderState2)(*lpd3ddev, D3DRENDERSTATE_FOGENABLE, FALSE); 
			if(dxw.dwFlags13 & FORCECOLORKEYOFF) (*pSetRenderState2)(*lpd3ddev, D3DRENDERSTATE_COLORKEYENABLE , FALSE); 
			if(dxw.dwDFlags & ZBUFFERALWAYS) (*pSetRenderState2)(*lpd3ddev, D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
			if(dxw.dwFlags15 & CULLMODENONE)(*pSetRenderState2)(*lpd3ddev, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
		}		
		break;
	case 3:
		SetHook((void *)(**(DWORD **)lpd3ddev +   0), extQueryInterfaceD3D3, (void **)&pQueryInterfaceD3D3, "QueryInterface(D3DD3)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +   8), extReleaseD3D3, (void **)&pReleaseD3D3, "ReleaseD3D(3)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  12), extD3DGetCaps3, (void **)&pD3DGetCaps3, "GetCaps(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  20), extAddViewport3, (void **)&pAddViewport3, "AddViewport(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  32), extEnumTextureFormats3, (void **)&pEnumTextureFormats3, "EnumTextureFormats(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  36), extBeginScene3, (void **)&pBeginScene3, "BeginScene(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  40), extEndScene3, (void **)&pEndScene3, "EndScene(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  48), extSetCurrentViewport3, (void **)&pSetCurrentViewport3, "SetCurrentViewport(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  52), extGetCurrentViewport3, (void **)&pGetCurrentViewport3, "GetCurrentViewport(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  88), extSetRenderState3, (void **)&pSetRenderState3, "SetRenderState(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  96), extSetLightState3, (void **)&pSetLightState3, "SetLightState(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 100), extSetTransform3, (void **)&pSetTransform3, "SetTransform(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 112), extDrawPrimitive3, (void **)&pDrawPrimitive3, "DrawPrimitive(3)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev + 148), extGetTexture3, (void **)&pGetTexture3, "GetTexture(3)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev + 152), extSetTexture3, (void **)&pSetTexture3, "SetTexture(3)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev + 156), extGetTextureStageState3, (void **)&pGetTextureStageState3, "GetTextureStageState(3)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 160), extSetTextureStageState3, (void **)&pSetTextureStageState3, "SetTextureStageState(3)");
#endif // TRACEALL
		if(pSetRenderState3){
			if(dxw.dwFlags2 & WIREFRAME)(*pSetRenderState3)(*lpd3ddev, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME); 		
			if(dxw.dwFlags4 & DISABLEFOGGING) (*pSetRenderState3)(*lpd3ddev, D3DRENDERSTATE_FOGENABLE, FALSE); 
			if(dxw.dwFlags13 & FORCECOLORKEYOFF) (*pSetRenderState3)(*lpd3ddev, D3DRENDERSTATE_COLORKEYENABLE , FALSE); 
			if(dxw.dwDFlags & ZBUFFERALWAYS) (*pSetRenderState3)(*lpd3ddev, D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
			if(dxw.dwFlags15 & CULLMODENONE)(*pSetRenderState3)(*lpd3ddev, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
		}		
		break;
	case 7:
		SetHook((void *)(**(DWORD **)lpd3ddev +   0), extQueryInterfaceD3D7, (void **)&pQueryInterfaceD3D7, "QueryInterface(D3DD7)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +   8), extReleaseD3D7, (void **)&pReleaseD3D7, "ReleaseD3D(7)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  12), extD3DGetCaps7, (void **)&pD3DGetCaps7, "GetCaps(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  16), extEnumTextureFormats7, (void **)&pEnumTextureFormats7, "EnumTextureFormats(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  20), extBeginScene7, (void **)&pBeginScene7, "BeginScene(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  24), extEndScene7, (void **)&pEndScene7, "EndScene(7)");
		// 28: GetDirect3D
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  32), extSetRenderTarget7, (void **)&pSetRenderTarget7, "SetRenderTarget(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  36), extGetRenderTarget7, (void **)&pGetRenderTarget7, "GetRenderTarget(7)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  40), extClearD7, (void **)&pClearD7, "Clear(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  44), extSetTransform7, (void **)&pSetTransform7, "SetTransform(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  52), extSetViewport7, (void **)&pSetViewport7, "SetViewport(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  60), extGetViewport7, (void **)&pGetViewport7, "GetViewport(7)");
#ifdef TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lpd3ddev +  64), extSetMaterial7, (void **)&pSetMaterial7, "SetMaterial(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev +  68), extGetMaterial7, (void **)&pGetMaterial7, "GetMaterial(7)");
#endif // TRACEMATERIAL
		SetHook((void *)(**(DWORD **)lpd3ddev +  72), extSetLight7, (void **)&pSetLight7, "SetLight(7)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  76), extGetLight7, (void **)&pGetLight7, "GetLight(7)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev +  80), extSetRenderState7, (void **)&pSetRenderState7, "SetRenderState(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 100), extDrawPrimitive7, (void **)&pDrawPrimitive7, "DrawPrimitive(7)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev + 136), extGetTexture7, (void **)&pGetTexture7, "GetTexture(7)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev + 140), extSetTexture7, (void **)&pSetTexture7, "SetTexture(7)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpd3ddev + 144), extGetTextureStageState7, (void **)&pGetTextureStageState7, "GetTextureStageState(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 148), extSetTextureStageState7, (void **)&pSetTextureStageState7, "SetTextureStageState(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 176), extLightEnable7, (void **)&pLightEnable7, "LightEnable(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 180), extGetLightEnable7, (void **)&pGetLightEnable7, "GetLightEnable(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 184), extSetClipPlane7, (void **)&pSetClipPlane7, "SetClipPlane(7)");
		SetHook((void *)(**(DWORD **)lpd3ddev + 188), extGetClipPlane7, (void **)&pGetClipPlane7, "GetClipPlane(7)");
#endif // TRACEALL
		if(pSetRenderState7){
			if(dxw.dwFlags2 & WIREFRAME)(*pSetRenderState7)(*lpd3ddev, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME); 		
			if(dxw.dwFlags4 & DISABLEFOGGING) (*pSetRenderState7)(*lpd3ddev, D3DRENDERSTATE_FOGENABLE, FALSE); 
			if(dxw.dwFlags13 & FORCECOLORKEYOFF) (*pSetRenderState7)(*lpd3ddev, D3DRENDERSTATE_COLORKEYENABLE , FALSE); 
			if(dxw.dwDFlags & ZBUFFERALWAYS) (*pSetRenderState7)(*lpd3ddev, D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
			if(dxw.dwFlags15 & CULLMODENONE)(*pSetRenderState7)(*lpd3ddev, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
		}		
		break;
	}
} 

void HookViewport(LPDIRECT3DVIEWPORT *lpViewport, int d3dversion)
{
	ApiName("dxwnd.HookViewport");
	OutTraceD3D("%s: Viewport=%#x d3dversion=%d\n", ApiRef, *lpViewport, d3dversion);

 	switch(d3dversion){
	case 1:
		SetHook((void *)(**(DWORD **)lpViewport +  12), extInitializeVP, (void **)&pInitializeVP, "Initialize(VP1)");
		SetHook((void *)(**(DWORD **)lpViewport +  16), extGetViewport1, (void **)&pGetViewport1, "GetViewport(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  20), extSetViewport1, (void **)&pSetViewport1, "SetViewport(1)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpViewport +  24), extTransformVertices1, (void **)&pTransformVertices1, "TransformVertices(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  28), extLightElements1, (void **)&pLightElements1, "LightElements(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  32), extSetBackground1, (void **)&pSetBackground1, "SetBackground(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  36), extGetBackground1, (void **)&pGetBackground1, "GetBackground(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  40), extSetBackgroundDepth1, (void **)&pSetBackgroundDepth1, "SetBackgroundDepth(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  44), extGetBackgroundDepth1, (void **)&pGetBackgroundDepth1, "GetBackgroundDepth(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  48), extClear1, (void **)&pClear1, "Clear(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  52), extAddLight1, (void **)&pAddLight1, "AddLight(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  56), extDeleteLight1, (void **)&pDeleteLight1, "DeleteLight(1)");
		SetHook((void *)(**(DWORD **)lpViewport +  60), extNextLight1, (void **)&pNextLight1, "NextLight(1)");

		// to do: why Clear method crashes in "Forsaken" in emulation and GDI mode??? fixed ???
		SetHook((void *)(**(DWORD **)lpViewport +  48), extViewportClear, (void **)&pViewportClear, "Clear(1)");
#endif // TRACEALL
		break;
	case 2:
		SetHook((void *)(**(DWORD **)lpViewport +  12), extInitializeVP, (void **)&pInitializeVP, "Initialize(VP2)");
		SetHook((void *)(**(DWORD **)lpViewport +  16), extGetViewport2, (void **)&pGetViewport2, "GetViewport(2)");
		SetHook((void *)(**(DWORD **)lpViewport +  20), extSetViewport2, (void **)&pSetViewport2, "SetViewport(2)");
		SetHook((void *)(**(DWORD **)lpViewport +  64), extGetViewport2_2, (void **)&pGetViewport2_2, "GetViewport2(2)");
		SetHook((void *)(**(DWORD **)lpViewport +  68), extSetViewport2_2, (void **)&pSetViewport2_2, "SetViewport2(2)");
		break;
	case 3:
		SetHook((void *)(**(DWORD **)lpViewport +  12), extInitializeVP, (void **)&pInitializeVP, "Initialize(VP3)");
		SetHook((void *)(**(DWORD **)lpViewport +  16), extGetViewport3, (void **)&pGetViewport3, "GetViewport(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  20), extSetViewport3, (void **)&pSetViewport3, "SetViewport(3)");
#ifdef TRACEALL
		SetHook((void *)(**(DWORD **)lpViewport +  24), extTransformVertices3, (void **)&pTransformVertices3, "TransformVertices(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  28), extLightElements3, (void **)&pLightElements3, "LightElements(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  32), extSetBackground3, (void **)&pSetBackground3, "SetBackground(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  36), extGetBackground3, (void **)&pGetBackground3, "GetBackground(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  40), extSetBackgroundDepth3, (void **)&pSetBackgroundDepth3, "SetBackgroundDepth(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  44), extGetBackgroundDepth3, (void **)&pGetBackgroundDepth3, "GetBackgroundDepth(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  48), extClear3, (void **)&pClear3, "Clear(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  52), extAddLight3, (void **)&pAddLight3, "AddLight(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  56), extDeleteLight3, (void **)&pDeleteLight3, "DeleteLight(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  60), extNextLight3, (void **)&pNextLight3, "NextLight(3)");
#endif // TRACEALL
		SetHook((void *)(**(DWORD **)lpViewport +  64), extGetViewport2_3, (void **)&pGetViewport2_3, "GetViewport2(3)");
		SetHook((void *)(**(DWORD **)lpViewport +  68), extSetViewport2_3, (void **)&pSetViewport2_3, "SetViewport2(3)");
		break;
	case 7:
		break;
	}
}

#ifdef TRACEMATERIAL
void HookMaterial(LPDIRECT3DMATERIAL *lpMaterial, int d3dversion)
{
	ApiName("dxwnd.HookMaterial");
	OutTraceD3D("%s: Material=%#x d3dversion=%d\n", ApiRef, *lpMaterial, d3dversion);

 	switch(d3dversion){
	case 1:
		SetHook((void *)(**(DWORD **)lpMaterial +  16), extSetMaterial1, (void **)&pSetMaterial1, "SetMaterial(1)");
		SetHook((void *)(**(DWORD **)lpMaterial +  20), extGetMaterial1, (void **)&pGetMaterial1, "GetMaterial(1)");
		break;
	case 2:
		SetHook((void *)(**(DWORD **)lpMaterial +  12), extSetMaterial2, (void **)&pSetMaterial2, "SetMaterial(2)");
		SetHook((void *)(**(DWORD **)lpMaterial +  16), extGetMaterial2, (void **)&pGetMaterial2, "GetMaterial(2)");
		break;
	case 3:
		SetHook((void *)(**(DWORD **)lpMaterial +  12), extSetMaterial3, (void **)&pSetMaterial3, "SetMaterial(3)");
		SetHook((void *)(**(DWORD **)lpMaterial +  16), extGetMaterial3, (void **)&pGetMaterial3, "GetMaterial(3)");
		break;
	//default:
	//	SetHook((void *)(**(DWORD **)lpMaterial +  12), extSetMaterial, (void **)&pSetMaterial, "SetMaterial");
	//	SetHook((void *)(**(DWORD **)lpMaterial +  16), extGetMaterial, (void **)&pGetMaterial, "GetMaterial");
	//	break;
	}
}
#endif // TRACEMATERIAL

#ifdef TRACETEXTURE
typedef ULONG	(WINAPI *AddRef_Type)(void *);
typedef ULONG	(WINAPI *Release_Type)(void *);
QueryInterface_Type pQueryInterfaceD3DT1, pQueryInterfaceD3DT2;
AddRef_Type pAddRefD3DT1, pAddRefD3DT2;
Release_Type pReleaseD3DT1, pReleaseD3DT2;
HRESULT WINAPI extQueryInterfaceD3DT1(LPVOID, REFIID, void **);
HRESULT WINAPI extQueryInterfaceD3DT2(LPVOID, REFIID, void **);
ULONG WINAPI extAddRefD3DT1(LPVOID);
ULONG WINAPI extAddRefD3DT2(LPVOID);
ULONG WINAPI extReleaseD3DT1(LPVOID);
ULONG WINAPI extReleaseD3DT2(LPVOID);
#endif // TRACETEXTURE

void HookTexture(LPVOID *lpTexture, int version)
{
	ApiName("dxwnd.HookTexture");
	OutTraceD3D("%s: Texture=%#x version=%d\n", ApiRef, *lpTexture, version);
 	switch(version){
	case 1:
#ifdef TRACETEXTURE
		SetHook((void *)(**(DWORD **)lpTexture +   0), extQueryInterfaceD3DT1, (void **)&pQueryInterfaceD3DT1, "QueryInterface(D3DT1)");
		SetHook((void *)(**(DWORD **)lpTexture +   4), extAddRefD3DT1, (void **)&pAddRefD3DT1, "AddRef(D3DT1)");
		SetHook((void *)(**(DWORD **)lpTexture +   8), extReleaseD3DT1, (void **)&pReleaseD3DT1, "Release(D3DT1)");
		SetHook((void *)(**(DWORD **)lpTexture +  12), extTexInitialize, (void **)&pTInitialize, "Initialize(T1)");
		SetHook((void *)(**(DWORD **)lpTexture +  16), extTexGetHandle1, (void **)&pTGetHandle1, "GetHandle(T1)");
		SetHook((void *)(**(DWORD **)lpTexture +  20), extTexPaletteChanged1, (void **)&pTPaletteChanged1, "PaletteChanged(T1)");
		SetHook((void *)(**(DWORD **)lpTexture +  24), extTexLoad1, (void **)&pTLoad1, "Load(T1)");
		SetHook((void *)(**(DWORD **)lpTexture +  28), extTexUnload, (void **)&pTUnload, "Unload(T1)");
#endif // TRACETEXTURE
		break;
	case 2:
#ifdef TRACETEXTURE
		SetHook((void *)(**(DWORD **)lpTexture +   0), extQueryInterfaceD3DT2, (void **)&pQueryInterfaceD3DT2, "QueryInterface(D3DT2)");
		SetHook((void *)(**(DWORD **)lpTexture +   4), extAddRefD3DT2, (void **)&pAddRefD3DT2, "AddRef(D3DT2)");
		SetHook((void *)(**(DWORD **)lpTexture +   8), extReleaseD3DT2, (void **)&pReleaseD3DT2, "Release(D3DT2)");
		SetHook((void *)(**(DWORD **)lpTexture +  12), extTexGetHandle2, (void **)&pTGetHandle2, "GetHandle(T2)");
		SetHook((void *)(**(DWORD **)lpTexture +  16), extTexPaletteChanged2, (void **)&pTPaletteChanged2, "PaletteChanged(T2)");
		SetHook((void *)(**(DWORD **)lpTexture +  20), extTexLoad2, (void **)&pTLoad2, "Load(T2)");
#endif // TRACETEXTURE
		break;
	}
}

#ifdef TRACETEXTURE
static ULONG extAddRefD3DT(char *api, AddRef_Type pAddRef, LPVOID lpTex)
{ 
	ULONG ret;
	ret = (*pAddRef)(lpTex);
	OutTrace("%s: ref=%d\n", api, ret); 
	return ret;
}

ULONG WINAPI extAddRefD3DT1(LPVOID lpTex)
{ return extAddRefD3DT("IDirect3DTexture::AddRef", pAddRefD3DT1, lpTex); }
ULONG WINAPI extAddRefD3DT2(LPVOID lpTex)
{ return extAddRefD3DT("IDirect3DTexture2::AddRef", pAddRefD3DT2, lpTex); }

static ULONG extReleaseD3DT(char *api, Release_Type pRelease, LPVOID lpTex)
{ 
	ULONG ret;
	ret = (*pRelease)(lpTex);
	OutTrace("%s: ref=%d\n", api, ret); 
	return ret;
}

ULONG WINAPI extReleaseD3DT1(LPVOID lpTex)
{ return extReleaseD3DT("IDirect3DTexture::Release", pReleaseD3DT1, lpTex); }
ULONG WINAPI extReleaseD3DT2(LPVOID lpTex)
{ return extReleaseD3DT("IDirect3DTexture2::Release", pReleaseD3DT2, lpTex); }

HRESULT WINAPI extQueryInterfaceD3DT1(void *lpTex, REFIID riid, void **ppvObj)
{ return extQueryInterfaceDX("IDirect3DTexture::QueryInterface", 1, pQueryInterfaceD3DT1, lpTex, riid, ppvObj); }
HRESULT WINAPI extQueryInterfaceD3DT2(void *lpTex, REFIID riid, void **ppvObj)
{ return extQueryInterfaceDX("IDirect3DTexture2::QueryInterface", 2, pQueryInterfaceD3DT2, lpTex, riid, ppvObj); }
#endif // TRACETEXTURE

void HookExecuteBuffer(LPDIRECT3DEXECUTEBUFFER *lplpeb)
{
	OutTraceD3D("HookExecuteBuffer: lpeb=%#x\n", *lplpeb);

#ifdef TRACEALL
	SetHook((void *)(**(DWORD **)lplpeb +  12), extEBInitialize, (void **)&pEBInitialize, "Initialize(D3DEB)");
#endif // TRACEALL
	SetHook((void *)(**(DWORD **)lplpeb +  16), extEBLock, (void **)&pEBLock, "Lock(D3DEB)");
	SetHook((void *)(**(DWORD **)lplpeb +  20), extEBUnlock, (void **)&pEBUnlock, "Unlock(D3DEB)");
	SetHook((void *)(**(DWORD **)lplpeb +  24), extSetExecuteData, (void **)&pSetExecuteData, "SetExecuteData(D3DEB)");
	SetHook((void *)(**(DWORD **)lplpeb +  28), extGetExecuteData, (void **)&pGetExecuteData, "GetExecuteData(D3DEB)");
#ifdef TRACEALL
	SetHook((void *)(**(DWORD **)lplpeb +  32), extValidate, (void **)&pValidate, "Validate(D3DEB)");
#endif // TRACEALL
	SetHook((void *)(**(DWORD **)lplpeb +  36), extOptimize, (void **)&pOptimize, "Optimize(D3DEB)");
}

//----------------------------------------------------------------------//
// Auxiliary functions
//----------------------------------------------------------------------//

typedef struct {
	DWORD state;
	DWORD val;
} D3DSTATEX, *LPD3DSTATEX;

static int dxwForceState(LPDIRECT3DEXECUTEBUFFER lpeb, DWORD state, DWORD val)
{
	D3DEXECUTEBUFFERDESC ebd;
	D3DEXECUTEDATA ed;
	LPD3DINSTRUCTION lpInst;
	int fix = 0;
	HRESULT res;
	OutTraceDW("ForceState: state=%#x val=%#x\n", state, val);

	ed.dwSize = sizeof(D3DEXECUTEDATA);
	if(res=(*pGetExecuteData)(lpeb, &ed)) {
		OutErrorD3D("ForceState: GetExecuteData ERROR res=%#x(%s)\n", res, ExplainDDError(res));
		return 0;
	}
	ebd.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
	if(res=(*pEBLock)(lpeb, &ebd)) {
		OutErrorD3D("ForceState: Lock ERROR res=%#x(%s)\n", res, ExplainDDError(res));
		return 0;
	}

	int len;
	lpInst = (LPD3DINSTRUCTION)((LPBYTE)ebd.lpData + ed.dwInstructionOffset);
	len = (int)ed.dwInstructionLength;
	for(DWORD i=0; len>0; i++){
		if((lpInst->bOpcode < D3DOP_POINT) || (lpInst->bOpcode > D3DOP_SETSTATUS)) break;
		if(lpInst->bOpcode == D3DOP_EXIT) break;
		if(lpInst->bOpcode == D3DOP_STATERENDER){
			LPD3DSTATEX lpState = (LPD3DSTATEX)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION));
			for (int j=0; j<lpInst->wCount; j++){
				if(lpState->state == state) {
					lpState->val = val;
					fix ++;
				}
				lpState ++;
			}
		}
		len -= (lpInst->bSize * lpInst->wCount);
		lpInst = (LPD3DINSTRUCTION)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION) + (lpInst->bSize * lpInst->wCount));
	}
	if(res=(*pEBUnlock)(lpeb)) {
		OutErrorD3D("ForceState: Unlock ERROR res=%#x(%s)\n", res, ExplainDDError(res));
		return 0;
	}
	OutTraceDW("ForceState: eb=%#x state=%#x val=%#x fixed=%d\n", lpeb, state, val, fix);
	return fix;
}

static void dxwFakeWireFrame(LPDIRECT3DEXECUTEBUFFER lpeb)
{
	D3DEXECUTEBUFFERDESC ebd;
	D3DEXECUTEDATA ed;
	LPD3DINSTRUCTION lpInst;
	int fix = 0;

	ed.dwSize = sizeof(D3DEXECUTEDATA);
	if((*pGetExecuteData)(lpeb, &ed)) return;
	ebd.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
	if((*pEBLock)(lpeb, &ebd)) return; 

	int len;
	lpInst = (LPD3DINSTRUCTION)((LPBYTE)ebd.lpData + ed.dwInstructionOffset);
	len = (int)ed.dwInstructionLength;
	for(DWORD i=0; len>0; i++){
		if((lpInst->bOpcode < D3DOP_POINT) || (lpInst->bOpcode > D3DOP_SETSTATUS)) break;
		if(lpInst->bOpcode == D3DOP_EXIT) break;
		if(lpInst->bOpcode == D3DOP_TRIANGLE){
			lpInst->bOpcode = D3DOP_LINE;
			LPWORD lpVertex = (LPWORD)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION));
			for (int j=0; j<lpInst->wCount; j++){
				lpVertex[3]=lpVertex[2];
				lpVertex[2]=lpVertex[1];
				lpVertex+=4;
			}
			lpInst->wCount *= 2;
			lpInst->bSize /= 2;
		}
		len -= (lpInst->bSize * lpInst->wCount);
		lpInst = (LPD3DINSTRUCTION)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION) + (lpInst->bSize * lpInst->wCount));
	}
	(*pEBUnlock)(lpeb);
	//OutTraceD3D("ForceState: eb=%#x state=%#x val=%#x fixed=%d\n", lpeb, state, val, fix);
}

static void LegacyD3DResolutionHack(int d3dversion)
{
	HMODULE hD3D;
	char *module;
	extern void dxPatchModule(HMODULE, const BYTE *, int, const BYTE *, int, BOOL);
	OutTraceDW("LegacyD3DResolutionHack\n");
#if 0
	// first occurrence
	const BYTE wantedBytes[] =  { 0x75, 0xEC, 0xB8, 0x00, 0x08, 0x00, 0x00, 0x39 };
	const BYTE updatedBytes[] = { 0x75, 0xEC, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0x39 };
	// second occurrence
	//const BYTE wantedBytes[] = { 0x74, 0xDF, 0xB8, 0x00, 0x08, 0x00, 0x00, 0x39 };
	//const BYTE updatedBytes[] = { 0x74, 0xDF, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0x39 };
#else
	const BYTE wantedBytes[] =  { 0xB8, 0x00, 0x08, 0x00, 0x00, 0x39 };
	const BYTE updatedBytes[] = { 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0x39 };
#endif
	module = (d3dversion == 7) ? "d3dim700.dll" : "d3dim.dll";
	hD3D = GetModuleHandle(module);
	if(hD3D) {
		dxPatchModule(hD3D, wantedBytes, sizeof(wantedBytes), updatedBytes, sizeof(updatedBytes), TRUE);
	}
	else {
		OutErrorD3D("LegacyD3DResolutionHack: no module %s\n", module);
	}
}

static void InsertPatchingExecuteBuffer(void *d3dd, DWORD state, DWORD val)
{
	HRESULT res;
	D3DEXECUTEBUFFERDESC ebdesc;
	D3DEXECUTEDATA ed;
	LPDIRECT3DEXECUTEBUFFER lpeb;
	LPD3DINSTRUCTION lpInstruction;
	LPD3DSTATEX lpState;
	LPBYTE lpBuf;
	int dwSize;

	OutTraceDW("Patch: d3dd=%#x state=%#x val=%#x\n", d3dd, state, val);
	// reserve space for the following:
	// instruction D3DOP_STATERENDER with 1 D3DRENDERSTATE_ZFUNC state
	// instruction EXIT
	dwSize = (2 * sizeof(D3DINSTRUCTION)) + sizeof(D3DSTATE);

	memset(&ebdesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
	ebdesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
	ebdesc.dwFlags = D3DDEB_BUFSIZE;
	ebdesc.dwBufferSize = dwSize;
	res = (*pCreateExecuteBuffer)(d3dd, &ebdesc, &lpeb, NULL);
	if(res) {
		OutTraceDW("Patch: CreateExecuteBuffer error ret=%d(%s)\n", res, ExplainDDError(res));
		return;
	}
	res = (*pEBLock)(lpeb, &ebdesc);
	if(res) {
		OutErrorD3D("Patch: Lock error ret=%d(%s)\n", res, ExplainDDError(res));
		return;
	}
	lpBuf = (LPBYTE)ebdesc.lpData;
	memset(lpBuf, 0, dwSize);

	lpInstruction = (LPD3DINSTRUCTION)lpBuf;
	lpBuf += sizeof(D3DINSTRUCTION);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->wCount = 1;
	lpInstruction->bSize = sizeof(D3DSTATE);

	/* possible values
	D3DCMP_NEVER               = 1,
    D3DCMP_LESS                = 2,
    D3DCMP_EQUAL               = 3,
    D3DCMP_LESSEQUAL           = 4,
    D3DCMP_GREATER             = 5,
    D3DCMP_NOTEQUAL            = 6,
    D3DCMP_GREATEREQUAL        = 7,
    D3DCMP_ALWAYS              = 8,
	*/

	lpState = (LPD3DSTATEX)lpBuf;
	lpBuf += sizeof(LPD3DSTATEX);
	lpState->state = state;
	lpState->val = val;

	// the execute buffer must NOT be terminated with a D3DOP_EXIT instruction
	// or the effect won't be propagated to the following execute buffers!!!
	//lpInstruction = (LPD3DINSTRUCTION)lpBuf;
	//lpBuf += sizeof(D3DINSTRUCTION);
	//lpInstruction->bOpcode = D3DOP_EXIT;
	//lpInstruction->wCount = 0;
	//lpInstruction->bSize = 0;

	res = (*pEBUnlock)(lpeb);
	if(res) {
		OutErrorD3D("Patch: Unlock error ret=%#x(%s)\n", res, ExplainDDError(res));
		return;
	}
	memset(&ed, 0, sizeof(D3DEXECUTEDATA));
	ed.dwSize = sizeof(D3DEXECUTEDATA);
	ed.dwInstructionOffset = 0;
	ed.dwHVertexOffset = 0;
	ed.dwInstructionLength = dwSize;
	ed.dwVertexCount = 0;
	ed.dwVertexOffset = 0;
	// ed.dsStatus ???
	res = (*pSetExecuteData)(lpeb, &ed);
	if(res) {
		OutErrorD3D("Patch: SetExecuteData error ret=%#x(%s)\n", res, ExplainDDError(res));
		return;
	}
	res = (*pExecute)(d3dd, lpeb, lpCurrViewport, D3DEXECUTE_CLIPPED); // viewport? flags?
	if(res) {
		OutErrorD3D("Patch: Execute error ret=%#x(%s)\n", res, ExplainDDError(res));
		return;
	}
	(LPDIRECT3DEXECUTEBUFFER)lpeb->Release();
}

#define FORCEEBCAPS (FORCEHWVERTEXPROC|FORCESWVERTEXPROC|FORCEMXVERTEXPROC)

DWORD ForceExecuteBufferCaps(void)
{
	DWORD ebcaps;
	switch(dxw.dwFlags10 & FORCEEBCAPS){
		case FORCEHWVERTEXPROC:
			ebcaps = D3DDEBCAPS_VIDEOMEMORY;
			break;
		case FORCESWVERTEXPROC:
			ebcaps = D3DDEBCAPS_SYSTEMMEMORY;
			break;
		case FORCEMXVERTEXPROC:
			ebcaps = D3DDEBCAPS_VIDEOMEMORY|D3DDEBCAPS_SYSTEMMEMORY;
			break;
	}
	OutTraceDW("ForceExecuteBufferCaps: FORCE cap=%#x(%s)\n", ebcaps, sExecuteBufferCaps(ebcaps));
	return ebcaps;
}

#ifndef DXW_NOTRACES
static char *explainD3DTextureStageState(DWORD state)
{
	char *labels[] = {
	"???", "COLOROP", "COLORARG1", "COLORARG2", "ALPHAOP", "ALPHAARG1", "ALPHAARG2", "BUMPENVMAT00", // 0-7
	"BUMPENVMAT01", "BUMPENVMAT10", "BUMPENVMAT11", "TEXCOORDINDEX", "ADDRESS", "ADDRESSU", "ADDRESSV", "BORDERCOLOR", // 8-15
	"MAGFILTER", "MINFILTER", "MIPFILTER", "MIPMAPLODBIAS", "MAXMIPLEVEL", "MAXANISOTROPY", "BUMPENVLSCALE", "BUMPENVLOFFSET", // 16-23
	"TEXTURETRANSFORMFLAGS" // 24
	};
	if(state > D3DTSS_TEXTURETRANSFORMFLAGS) return "???";
	return labels[state];
}

static char *explainD3DTextureOp(DWORD state, DWORD op)
{
	char *val = "???";
	static char valBuffer[80];
	char *colorLabels[] = {
	"???", "DISABLE", "SELECTARG1", "SELECTARG2", "MODULATE", "MODULATE2X", "MODULATE4X", "ADD", // 0-7
	"ADDSIGNED", "ADDSIGNED2X", "SUBTRACT", "ADDSMOOTH", "BLENDDIFFUSEALPHA", "BLENDTEXTUREALPHA", "BLENDFACTORALPHA", "BLENDTEXTUREALPHAPM", // 8-15
	"BLENDCURRENTALPHA", "PREMODULATE", "MODULATEALPHA_ADDCOLOR", "MODULATECOLOR_ADDALPHA", "MODULATEINVALPHA_ADDCOLOR", "MODULATEINVCOLOR_ADDALPHA", "BUMPENVMAP", "BUMPENVMAPLUMINANCE", // 16-23
	"DOTPRODUCT3" // 24
	};
	char *d3dtaLabels[] = {
	"DIFFUSE", "CURRENT", "TEXTURE", "TFACTOR", "SPECULAR"
	};
	char *addrLabels[] = {
	"???", "WRAP", "MIRROR", "CLAMP", "BORDER"
	};
	char *magfilterLabels[] = {
	"???", "POINT", "LINEAR", "FLATCUBIC", "GAUSSIANCUBIC", "ANISOTROPIC"
	};

	switch(state){
		case D3DTSS_COLOROP:
		case D3DTSS_ALPHAOP:
			if(op <= D3DTOP_DOTPRODUCT3) val = colorLabels[op];
			break;
		case D3DTSS_COLORARG1:
		case D3DTSS_COLORARG2:
		case D3DTSS_ALPHAARG1:
		case D3DTSS_ALPHAARG2:
			op &= 0x7;
			if(op <= D3DTA_SPECULAR) val = d3dtaLabels[op];
			break;
		case D3DTSS_BUMPENVMAT00:
		case D3DTSS_BUMPENVMAT01:
		case D3DTSS_BUMPENVMAT10:
		case D3DTSS_BUMPENVMAT11:
		case D3DTSS_TEXCOORDINDEX:
		case D3DTSS_MIPMAPLODBIAS:
		case D3DTSS_BUMPENVLSCALE:
		case D3DTSS_BUMPENVLOFFSET:
		case D3DTSS_MAXANISOTROPY:
		case D3DTSS_MAXMIPLEVEL:
			val="dword";
			break;
		case D3DTSS_ADDRESS:
		case D3DTSS_ADDRESSU:
		case D3DTSS_ADDRESSV:
			if(op <= D3DTADDRESS_BORDER) val = addrLabels[op];
			break;
		case D3DTSS_BORDERCOLOR:
			val="color";
			break;
		case D3DTSS_MAGFILTER:
		case D3DTSS_MINFILTER:
		case D3DTSS_MIPFILTER:
			if(op <= D3DTFG_ANISOTROPIC) val = magfilterLabels[op];
			break;
		case D3DTSS_TEXTURETRANSFORMFLAGS:
			switch(op & 0x0F){
				case D3DTTFF_DISABLE: val = "DISABLE"; break;
				case D3DTTFF_COUNT1: val = "COUNT1"; break;
				case D3DTTFF_COUNT2: val = "COUNT2"; break;
				case D3DTTFF_COUNT3: val = "COUNT3"; break;
				case D3DTTFF_COUNT4: val = "COUNT4"; break;
			}
			if(op & D3DTTFF_PROJECTED) {
				sprintf(valBuffer, "PROJECTED+%s", val);
				val = valBuffer;
			}
			break;
	}
	return val;
}
#endif // DXW_NOTRACES

//----------------------------------------------------------------------//
// Tracing
//----------------------------------------------------------------------//

#ifndef DXW_NOTRACES

static char *sFourCC(DWORD fcc)
{
	static char sRet[5];
	char c;
	int i;
	char *t=&sRet[0];
	for(i=0; i<4; i++){
		c = fcc & (0xFF);
		*t++ = isprint(c) ? c : '.';
		c = c >> 8;
	}
	*t = 0;
	return sRet;
}

static char *ExplainBlendType(DWORD val)
{
	char *captions[] = {
		"ZERO", "ONE", "SRCCOLOR", "INVSRCCOLOR", "SRCALPHA", "INVSRCALPHA",
		"DESTALPHA", "INVDESTALPHA", "DESTCOLOR", "INVDESTCOLOR", "SRCALPHASAT",
		"BOTHSRCALPHA", "BOTHINVSRCALPHA"
	};
	if((val < D3DBLEND_ZERO) || (val > (D3DBLEND_BOTHINVSRCALPHA-D3DBLEND_ZERO))) return "???";
	return captions[val-D3DBLEND_ZERO];
}

static char *ExplainAntialiasMode(DWORD val)
{
	char *captions[] = {
	"NONE", "SORTDEPENDENT", "SORTINDEPENDENT"
	};
	if((val < D3DANTIALIAS_NONE) || (val > (D3DANTIALIAS_SORTINDEPENDENT-D3DANTIALIAS_NONE))) return "???";
	return captions[val-D3DANTIALIAS_NONE];
}

static char *ExplainShadeMode(DWORD val)
{
	char *captions[] = {
	"FLAT", "GOURAUD", "PHONG"
	};
	if((val < D3DSHADE_FLAT) || (val > (D3DSHADE_PHONG-D3DSHADE_FLAT))) return "???";
	return captions[val-D3DSHADE_FLAT];
}


static char *ExplainCompFuncType(DWORD val)
{
	char *captions[] = {
		"NEVER", "LESS", "EQUAL", "LESSEQUAL", 
		"GREATER", "NOTEQUAL", "GREATEREQUAL", "ALWAYS"
	};
	if((val < D3DCMP_NEVER) || (val > (D3DCMP_ALWAYS-D3DCMP_NEVER))) return "???";
	return captions[val-D3DCMP_NEVER];
}

static char *ExplainFillMode(DWORD val)
{
	char *captions[] = {
		"POINT", "WIREFRAME", "SOLID"
	};
	if((val < D3DFILL_POINT) || (val > (D3DFILL_SOLID-D3DFILL_POINT))) return "???";
	return captions[val-D3DFILL_POINT];
}

static char *ExplainD3DStencilOp(DWORD val)
{
	char *captions[] = {
		"KEEP", "ZERO", "REPLACE", "INCRSAT", "DECRSAT", "INVERT", "INCR", "DECR"
	};
	if((val < D3DSTENCILOP_KEEP) || (val > D3DSTENCILOP_DECR-D3DSTENCILOP_KEEP)) return "???";
	return captions[val-D3DSTENCILOP_KEEP];
}

static char *ExplainD3DTextureFilter(DWORD val)
{
	char *captions[] = {
		"NEAREST", "LINEAR", "MIPNEAREST", "MIPLINEAR", "LINEARMIPNEAREST", "LINEARMIPLINEAR"
	};
	if((val < D3DFILTER_NEAREST) || (val > D3DFILTER_LINEARMIPLINEAR-D3DFILTER_NEAREST)) return "???";
	return captions[val-D3DFILTER_NEAREST];
}

static char *ExplainD3DMapBlend(DWORD val)
{
	char *captions[] = {
		"DECAL", "MODULATE", "DECALALPHA", "MODULATEALPHA", "DECALMASK", "MODULATEMASK", "COPY", "ADD"
	};
	if((val < D3DTBLEND_DECAL) || (val > D3DTBLEND_ADD-D3DTBLEND_DECAL)) return "???";
	return captions[val-D3DTBLEND_DECAL];
}

static char *sTransformType(D3DTRANSFORMSTATETYPE tstype)
{
	char *s;
	switch(tstype){
		case D3DTRANSFORMSTATE_WORLD:		s = "WORLD"; break;
		case D3DTRANSFORMSTATE_VIEW:		s = "VIEW"; break;
		case D3DTRANSFORMSTATE_PROJECTION:	s = "PROJECTION"; break;
		case D3DTRANSFORMSTATE_WORLD1:		s = "WORLD1"; break;
		case D3DTRANSFORMSTATE_WORLD2:		s = "WORLD2"; break;
		case D3DTRANSFORMSTATE_WORLD3:		s = "WORLD3"; break;
		case D3DTRANSFORMSTATE_TEXTURE0:	s = "TEXTURE0"; break;
		case D3DTRANSFORMSTATE_TEXTURE1:	s = "TEXTURE1"; break;
		case D3DTRANSFORMSTATE_TEXTURE2:	s = "TEXTURE2"; break;
		case D3DTRANSFORMSTATE_TEXTURE3:	s = "TEXTURE3"; break;
		case D3DTRANSFORMSTATE_TEXTURE4:	s = "TEXTURE4"; break;
		case D3DTRANSFORMSTATE_TEXTURE5:	s = "TEXTURE5"; break;
		case D3DTRANSFORMSTATE_TEXTURE6:	s = "TEXTURE6"; break;
		case D3DTRANSFORMSTATE_TEXTURE7:	s = "TEXTURE7"; break;
		default:	s = "unknown"; break;
	}
	return s;
}

static char *sExecuteBufferFlags(DWORD c)
{
	static char eb[128];
	unsigned int l;
	strcpy(eb,"D3DDEB_");
	if (c & D3DDEB_BUFSIZE) strcat(eb, "BUFSIZE+");
	if (c & D3DDEB_CAPS) strcat(eb, "CAPS+");
	if (c & D3DDEB_LPDATA) strcat(eb, "LPDATA+");
	l=strlen(eb);
	if (l>strlen("D3DDEB_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}

static char *sExecuteBufferCaps(DWORD c)
{
	static char eb[128];
	unsigned int l;
	strcpy(eb,"D3DDEBCAPS_");
	if (c & D3DDEBCAPS_SYSTEMMEMORY) strcat(eb, "SYSTEMMEMORY+");
	if (c & D3DDEBCAPS_VIDEOMEMORY) strcat(eb, "VIDEOMEMORY+");
	l=strlen(eb);
	if (l>strlen("D3DDEBCAPS_")) eb[l-1]=0; // delete last '+' if any
	else eb[0]=0;
	return(eb);
}

#ifdef DUMPEXECUTEBUFFER

static char *sOpcode(BYTE op)
{
	char *ops[]={
		"NULL",
		"POINT",
		"LINE",
		"TRIANGLE",
		"MATRIXLOAD",
		"MATRIXMULTIPLY",
		"STATETRANSFORM",
		"STATELIGHT",
		"STATERENDER",
		"PROCESSVERTICES",
		"TEXTURELOAD",
		"EXIT",
		"BRANCHFORWARD",
		"SPAN",
		"SETSTATUS"
	};

	if(op <= D3DOP_SETSTATUS) return ops[op];
	return "unknown";
}

static char *sRenderStateCode(DWORD code)
{
	char *codes[]={
		"NULL",
		"TEXTUREHANDLE",
		"ANTIALIAS",
		"TEXTUREADDRESS",
		"TEXTUREPERSPECTIVE",
		"WRAPU",
		"WRAPV",
		"ZENABLE",
		"FILLMODE",
		"SHADEMODE",
		"LINEPATTERN", // 10
		"MONOENABLE",
		"ROP2",
		"PLANEMASK",
		"ZWRITEENABLE",
		"ALPHATESTENABLE",
		"LASTPIXEL",
		"TEXTUREMAG",
		"TEXTUREMIN",
		"SRCBLEND",
		"DESTBLEND", // 20
		"TEXTUREMAPBLEND",
		"CULLMODE",
		"ZFUNC",
		"ALPHAREF",
		"ALPHAFUNC",
		"DITHERENABLE",
		"ALPHABLENDENABLE",
		"FOGENABLE",
		"SPECULARENABLE",
		"ZVISIBLE", // 30
		"SUBPIXEL",
		"SUBPIXELX",
		"STIPPLEDALPHA",
		"FOGCOLOR",
		"FOGTABLEMODE",
		"FOGSTART",
		"FOGEND",
		"FOGDENSITY",
		"STIPPLEENABLE",
		"EDGEANTIALIAS", // 40
		"COLORKEYENABLE",
		"42???",
		"BORDERCOLOR",
		"TEXTUREADDRESSU",
		"TEXTUREADDRESSV",
		"MIPMAPLODBIAS",
		"ZBIAS",
		"RANGEFOGENABLE",
		"ANISOTROPY",
		"FLUSHBATCH", // 50
		"TRANSLUCENTSORTINDEPENDENT",
		"STENCILENABLE",
		"STENCILFAIL",
		"STENCILZFAIL",
		"STENCILPASS",
		"STENCILFUNC",
		"STENCILREF",
		"STENCILMASK",
		"STENCILWRITEMASK",
		"TEXTUREFACTOR" // 60
	};

	if(code <= D3DRENDERSTATE_TEXTUREFACTOR) return codes[code];
	return "unknown";
}

static char *s_D3DTransformStateType(DWORD t)
{
	char *st[] = {"WORLD", "VIEW", "PROJECTION", "WORLD1", "WORLD2", "WORLD3" };
	if((t >= D3DTRANSFORMSTATE_WORLD) && (t <= D3DTRANSFORMSTATE_WORLD3)) return st[t - D3DTRANSFORMSTATE_WORLD];
	return "???";
}

static void DumpEBData(LPDIRECT3DEXECUTEBUFFER lpeb)
{
	D3DEXECUTEBUFFERDESC ebd;
	D3DEXECUTEDATA ed;

	if(!IsDebugD3D) return;

	ed.dwSize = sizeof(D3DEXECUTEDATA);
	if((*pGetExecuteData)(lpeb, &ed)) return;
	ebd.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
	if((*pEBLock)(lpeb, &ebd)) return; 

	OutTrace("> ==== DumpEBData lpeb=%#x ====\n", lpeb);
	OutTrace("> Vertex: offset=%d count=%d hvertexoffset=%d\n", ed.dwVertexOffset, ed.dwVertexCount, ed.dwHVertexOffset);
	OutTrace("> Instr.: offset=%d len=%d\n", ed.dwInstructionOffset, ed.dwInstructionLength);

#ifdef DUMPEXECUTEBUFFERVERTEX
	LPD3DVERTEX lpVertex;
	lpVertex = (LPD3DVERTEX)((LPBYTE)ebd.lpData + ed.dwVertexOffset);
	OutHexD3D((LPBYTE)lpVertex, ed.dwVertexCount * sizeof(D3DVERTEX));
	for(DWORD i=0; i<ed.dwVertexCount; i++){
		// do not format normal y coordinate, it is often an invalid real (infinite?)
		//OutTraceD3D("> Vertex[%04.4d]: pos=(%f:%f:%f) n=(%f:%f:%f) tex=(%f:%f)\n", i, 
		OutTrace("> Vertex[%04.4d]: pos=(%f:%f:%f) tex=(%f:%f)\n", i, 
			lpVertex->x, lpVertex->y, lpVertex->z,
			//lpVertex->nx, lpVertex->ny, lpVertex->nz,
			lpVertex->tu, lpVertex->tv);
		lpVertex++;
	}
#endif // DUMPEXECUTEBUFFERVERTEX

#ifdef DUMPEXECUTEBUFFERINSTRUCTIONS
	LPD3DINSTRUCTION lpInst;
	int len;
	lpInst = (LPD3DINSTRUCTION)((LPBYTE)ebd.lpData + ed.dwInstructionOffset);
	//lpInst = (LPD3DINSTRUCTION)lpVertex;
	len = (int)ed.dwInstructionLength;
	OutHexD3D((LPBYTE)lpInst, len);
	for(DWORD i=0; len>0; i++){
		if((lpInst->bOpcode < D3DOP_POINT) || (lpInst->bOpcode > D3DOP_SETSTATUS)) {
			OutTraceD3D("> Instr.[%04.4d]: invalid op=%d\n", i, lpInst->bOpcode);
			break;
		}
		OutTrace("> Instr.[%04.4d]: op=%d(%s) size=%d count=%d\n", i, 
			lpInst->bOpcode, sOpcode(lpInst->bOpcode), lpInst->bSize, lpInst->wCount);
		OutHexD3D((LPBYTE)lpInst + sizeof(D3DINSTRUCTION), (lpInst->bSize * lpInst->wCount));
		switch(lpInst->bOpcode){
			case D3DOP_TRIANGLE: 
				{
					LPD3DTRIANGLE lpTriangle = (LPD3DTRIANGLE)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION));
					for (int j=0; j<lpInst->wCount; j++){
						OutTrace(">> [%04.4d]: vertices=(%d,%d,%d) flags=%#x\n", 
							j, lpTriangle->v1, lpTriangle->v2, lpTriangle->v3, lpTriangle->wFlags);
						// cheats ...
						//lpTriangle->wFlags = 0x300;
						//lpTriangle->v3 = lpTriangle->v1;
						lpTriangle ++;
					}
				}		
				break;
			case D3DOP_STATERENDER:
				{
					LPD3DSTATE lpState = (LPD3DSTATE)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION));
					for (int j=0; j<lpInst->wCount; j++){
						OutTrace(">> [%04.4d]: code=%#x(%s) val=%#x\n", 
							j, lpState->drstRenderStateType, sRenderStateCode(lpState->drstRenderStateType), lpState->dwArg[0]);
						//if(lpState->drstRenderStateType == D3DRENDERSTATE_ZENABLE) 
						//	lpState->dwArg[0] = 0;
						lpState ++;
					}
				}		
				break;
			case D3DOP_MATRIXLOAD:
				{
					LPD3DMATRIXHANDLE lpMatrix = (LPD3DMATRIXHANDLE)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION));
					OutTrace(">> load %#x from %#x\n", lpMatrix[0], lpMatrix[1]);
				}
				break;
			case D3DOP_STATETRANSFORM:
				{
					LPD3DSTATE lpState = (LPD3DSTATE)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION));
					for(UINT i=0; i<lpInst->wCount; i++){
						OutTrace(">> STATETRANSFORM %#x(%s) matrix=%#x\n", 
							lpState->dtstTransformStateType, 
							s_D3DTransformStateType((DWORD)lpState->dtstTransformStateType),
							lpState->dwArg[0]);
						lpState++;
					}
				}
				break;
		}
		len -= sizeof(D3DINSTRUCTION) + (lpInst->bSize * lpInst->wCount);
		if(lpInst->bOpcode == D3DOP_EXIT) break;
		lpInst = (LPD3DINSTRUCTION)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION) + (lpInst->bSize * lpInst->wCount));
	}
	if(len) OutTrace("> residual len=%d !!!\n", len);
#endif // DUMPEXECUTEBUFFERINSTRUCTIONS
	(*pEBUnlock)(lpeb);
	OutTrace("> ==== DumpEBData END ====\n");
}
#endif // DUMPEXECUTEBUFFER

#endif // DXW_NOTRACES

//----------------------------------------------------------------------//
// Proxies
//----------------------------------------------------------------------//

HRESULT WINAPI extDirect3DCreate(UINT SDKVersion, LPDIRECT3D *lplpd3d, LPUNKNOWN pUnkOuter)
{
	HRESULT res;
	ApiName("Direct3DCreate");
	UINT d3dversion; 

	d3dversion = 1;
	if(SDKVersion >= 0x0500) d3dversion = 2;
	if(SDKVersion >= 0x0600) d3dversion = 3;
	if(SDKVersion >= 0x0700) d3dversion = 7;

	OutTraceD3D("%s: SDKVersion=%#x(D3D%d) UnkOuter=%#x\n", ApiRef, SDKVersion, d3dversion, pUnkOuter);
	res=(*pDirect3DCreate)(SDKVersion, lplpd3d, pUnkOuter);

	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
		return res;
	}

	HookDirect3DSession((LPDIRECTDRAW *)lplpd3d, d3dversion);
	OutTraceD3D("%s: d3d=%#x\n", ApiRef, *lplpd3d);
	return res;
}

HRESULT WINAPI extDirect3DCreateDevice(GUID FAR *lpGUID, LPDIRECT3D lpd3ddevice, LPDIRECTDRAWSURFACE surf, LPDIRECT3D *lplpd3ddevice, LPUNKNOWN pUnkOuter)
{
	HRESULT res;
	ApiName("Direct3DCreateDevice");
	int d3dversion = 0;

	switch(lpGUID->Data1){
	case 0x64108800: // IID_IDirect3DDevice
		d3dversion = 1; break;
	case 0x93281501: // IID_IDirect3DDevice2
		d3dversion = 2; break;
	case 0xb0ab3b60: // IID_IDirect3DDevice3
		d3dversion = 3; break;
	case 0xf5049e79: // IID_IDirect3DDevice7
		d3dversion = 7; break;
	case 0x84e63de0: // IID_IDirect3DHALDevice;
		d3dversion = 7; break;
	default:
		d3dversion = 0; break;
	}

	OutTraceD3D("%s: guid=%#x(%s) version=%d d3ddevice=%#x dds=%#x%s UnkOuter=%#x\n",
		ApiRef, lpGUID, ExplainGUID(lpGUID), d3dversion, lpd3ddevice, surf, dxwss.ExplainSurfaceRole(surf), pUnkOuter);
	res=(*pDirect3DCreateDevice)(lpGUID, lpd3ddevice, surf, lplpd3ddevice, pUnkOuter);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: d3ddevice=%#x version=%d\n", ApiRef, *lplpd3ddevice, d3dversion);
		if(d3dversion) HookDirect3DDevice((void **)lplpd3ddevice, d3dversion);
	}
	return res;
}

HRESULT WINAPI extQueryInterfaceD31(void *lpd3d, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3D::QueryInterface", 1, pQueryInterfaceD31, lpd3d, riid, ppvObj); }
HRESULT WINAPI extQueryInterfaceD32(void *lpd3d, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3D2::QueryInterface", 2, pQueryInterfaceD32, lpd3d, riid, ppvObj); }
HRESULT WINAPI extQueryInterfaceD33(void *lpd3d, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3D3::QueryInterface", 3, pQueryInterfaceD33, lpd3d, riid, ppvObj); }
HRESULT WINAPI extQueryInterfaceD37(void *lpd3d, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3D7::QueryInterface", 7, pQueryInterfaceD37, lpd3d, riid, ppvObj); }

HRESULT WINAPI extQueryInterfaceD3D1(void *lpd3ddev, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3DDevice::QueryInterface", 1, pQueryInterfaceD3D1, lpd3ddev, riid, ppvObj); }
HRESULT WINAPI extQueryInterfaceD3D2(void *lpd3ddev, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3DDevice2::QueryInterface", 2, pQueryInterfaceD3D2, lpd3ddev, riid, ppvObj); }
HRESULT WINAPI extQueryInterfaceD3D3(void *lpd3ddev, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3DDevice3::QueryInterface", 3, pQueryInterfaceD3D3, lpd3ddev, riid, ppvObj); }
HRESULT WINAPI extQueryInterfaceD3D7(void *lpd3ddev, REFIID riid, LPVOID *ppvObj)
{ return extQueryInterfaceDX("IDirect3DDevice7::QueryInterface", 7, pQueryInterfaceD3D7, lpd3ddev, riid, ppvObj); }

#ifdef TRACEALL
ULONG WINAPI extReleaseD3D(ApiArg, int d3dversion, ReleaseD3D_Type pReleaseD3D, LPDIRECT3DDEVICE lpd3dd)
{
	ULONG ref;
	OutTraceD3D("Release(D3DD%d): d3ddev=%#x \n", d3dversion, lpd3dd);
	ref = (*pReleaseD3D)(lpd3dd);
	OutTraceD3D("Release(D3DD): ref=%d\n", ref);
	return ref;
}

ULONG WINAPI extReleaseD3D1(LPDIRECT3DDEVICE lpd3d)
{ return extReleaseD3D("IDirect3D::Release", 1, pReleaseD3D1, lpd3d); }
ULONG WINAPI extReleaseD3D2(LPDIRECT3DDEVICE lpd3d)
{ return extReleaseD3D("IDirect3D2::Release", 2, pReleaseD3D2, lpd3d); }
ULONG WINAPI extReleaseD3D3(LPDIRECT3DDEVICE lpd3d)
{ return extReleaseD3D("IDirect3D3::Release", 3, pReleaseD3D3, lpd3d); }
ULONG WINAPI extReleaseD3D7(LPDIRECT3DDEVICE lpd3d)
{ return extReleaseD3D("IDirect3D7::Release", 7, pReleaseD3D7, lpd3d); }
#endif // TRACEALL

#ifdef TRACEMATERIAL
HRESULT WINAPI extInitialize(void *lpd3d)
{
	HRESULT res;
	ApiName("IDirect3D::Initialize");

	// the Initialize method is present in D3D interface version 1 only...
	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3d);
	res=(*pInitialize)(lpd3d);
	if(res) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	else OutTraceD3D("%s: OK\n", ApiRef);
	return res;
}
#endif // TRACEMATERIAL

typedef struct {
	LPD3DENUMDEVICESCALLBACK *cb;
	LPVOID arg;
} CallbackArg;

typedef struct {
	LPD3DENUMDEVICESCALLBACK7 *cb;
	LPVOID arg;
} CallbackArg7;

#ifndef DXW_NOTRACES
static void DumpD3DDeviceDesc(LPD3DDEVICEDESC d3, char *label, char *dev)
{
	if(IsTraceD3D){
		if(d3){
			char *logLine = (char *)malloc(1024);
			sprintf(logLine, "%s: LPD3DDEVICEDESC dev=%s Size=%d Flags=%#x", label, dev, d3->dwSize, d3->dwFlags);
			if(d3->dwFlags & D3DDD_COLORMODEL) sprintf(logLine, "%s ColorModel=%#x", logLine, d3->dcmColorModel);
			if(d3->dwFlags & D3DDD_DEVCAPS) sprintf(logLine, "%s DevCaps=%#x", logLine, d3->dwDevCaps);
			if(d3->dwFlags & D3DDD_TRANSFORMCAPS) sprintf(logLine, "%s TransformCaps=%#x", logLine, d3->dtcTransformCaps.dwCaps);
			if(d3->dwFlags & D3DDD_BCLIPPING) sprintf(logLine, "%s Clipping=%#x", logLine, d3->bClipping);
			if(d3->dwFlags & D3DDD_LIGHTINGCAPS) sprintf(logLine, "%s LightingCaps=%#x", logLine, d3->dlcLightingCaps);
			if(d3->dwFlags & D3DDD_LINECAPS) sprintf(logLine, "%s LineCaps=%#x", logLine, d3->dpcLineCaps);
			if(d3->dwFlags & D3DDD_TRICAPS) sprintf(logLine, "%s TriCaps=%#x", logLine, d3->dpcTriCaps);
			if(d3->dwFlags & D3DDD_DEVICERENDERBITDEPTH) sprintf(logLine, "%s DeviceRenderBitDepth=%d", logLine, d3->dwDeviceRenderBitDepth);
			if(d3->dwFlags & D3DDD_DEVICEZBUFFERBITDEPTH) sprintf(logLine, "%s DeviceZBufferBitDepth=%d", logLine, d3->dwDeviceZBufferBitDepth);
			if(d3->dwFlags & D3DDD_MAXBUFFERSIZE) sprintf(logLine, "%s MaxBufferSize=%d", logLine, d3->dwMaxBufferSize);
			if(d3->dwFlags & D3DDD_MAXVERTEXCOUNT) sprintf(logLine, "%s MaxVertexCount=%d", logLine, d3->dwMaxVertexCount);
			if(d3->dwSize > 52){
				sprintf(logLine, "%s Texture min=(%dx%d) max=(%dx%d)", logLine, d3->dwMinTextureWidth, d3->dwMinTextureHeight, d3->dwMaxTextureWidth, d3->dwMaxTextureHeight);
				sprintf(logLine, "%s Stipple min=(%dx%d) max=(%dx%d)", logLine, d3->dwMinStippleWidth, d3->dwMinStippleHeight, d3->dwMaxStippleWidth, d3->dwMaxStippleHeight);
			}
			if(d3->dwSize > (52 + 32)){
				sprintf(logLine, "%s MaxTextureRepeat/ratio=%d,%d MaxAnisotropy=%d", logLine, d3->dwMaxTextureRepeat, d3->dwMaxTextureAspectRatio, d3->dwMaxAnisotropy);
				// Guard band that the rasterizer can accommodate
				// Screen-space vertices inside this space but outside the viewport
				// will get clipped properly.
				//D3DVALUE    dvGuardBandLeft;
				//D3DVALUE    dvGuardBandTop;
				//D3DVALUE    dvGuardBandRight;
				//D3DVALUE    dvGuardBandBottom;
				//D3DVALUE    dvExtentsAdjust;
				sprintf(logLine, "%s StencilCaps=%#x FVFCaps=%#x TextureOpCaps=%#x", logLine, d3->dwStencilCaps, d3->dwFVFCaps, d3->dwTextureOpCaps);
				//WORD        wMaxTextureBlendStages;
				//WORD        wMaxSimultaneousTextures;			
			}
			strcat(logLine, "\n");
			OutTrace(logLine);
			free(logLine);
			OutHexD3D((unsigned char *)d3, d3->dwSize);
		}
		else
			OutTrace("%s: LPD3DDEVICEDESC dev=%s ddesc=NULL\n", label, dev);
	}
}

static void DumpD3DPrimCaps(char *label, D3DPRIMCAPS *pc)
{
	OutTrace("%s={siz=%d Misc=%#x Raster=%#x ZCmp=%#x SrcBlend=%#x DestBlend=%#x AlphaCmp=%#x Shade=%#x Tex=%#x TexFil=%#x TexBlend=%#x TexAddr=%#x Stipple=(%dx%d)}\n", 
		label,
		pc->dwSize, pc->dwMiscCaps, pc->dwRasterCaps, pc->dwZCmpCaps, pc->dwSrcBlendCaps, pc->dwDestBlendCaps, pc->dwAlphaCmpCaps,
		pc->dwShadeCaps, pc->dwTextureCaps, pc->dwTextureFilterCaps, pc->dwTextureBlendCaps, pc->dwTextureAddressCaps,
		pc->dwStippleWidth, pc->dwStippleHeight);
}

char *explainDWordFlags(DWORD v, char *captions[]){
	static char sBuffer[1024];
	sBuffer[0] = 0;
	for(int i=0; i<32; i++){
		if(captions[i]==NULL) break;
		if(v & 0x00000001) {
			strcat(sBuffer, captions[i]);
			strcat(sBuffer, "+");
		}
		v >>= 1;
	}
	int len = strlen(sBuffer);
	if(len > 0) sBuffer[len-1] = 0; // delete last "+"
	return sBuffer;
}

char *sTexOpCaps[] = {
	"DISABLE", "SELECTARG1", "SELECTARG2", "MODULATE", "MODULATE2X", "MODULATE4X", "ADD", "ADDSIGNED",
	"ADDSIGNED2X", "SUBTRACT", "ADDSMOOTH", "BLENDDIFFUSEALPHA", "BLENDTEXTUREALPHA", "BLENDFACTORALPHA", "BLENDTEXTUREALPHAPM", "BLENDCURRENTALPHA",
	"PREMODULATE", "MODULATEALPHA_ADDCOLOR", "MODULATECOLOR_ADDALPHA", "BUMPENVMAP", "BUMPENVMAPLUMINANCE", "DOTPRODUCT3",
	NULL };

static char *explainTextureOpCaps(DWORD cap){
	return explainDWordFlags(cap, sTexOpCaps);
}

char *sDevCaps[] = {
	"FLOATTLVERTEX", "SORTINCREASINGZ", "SORTDECREASINGZ", "SORTEXACT",
	"EXECUTESYSTEMMEMORY", "EXECUTEVIDEOMEMORY", "TLVERTEXSYSTEMMEMORY", "TLVERTEXVIDEOMEMORY",
	"TEXTURESYSTEMMEMORY", "TEXTUREVIDEOMEMORY", "DRAWPRIMTLVERTEX", "CANRENDERAFTERFLIP",
	"TEXTURENONLOCALVIDMEM", "DRAWPRIMITIVES2", "SEPARATETEXTUREMEMORIES", "DRAWPRIMITIVES2EX",
	"HWTRANSFORMANDLIGHT", "CANBLTSYSTONONLOCAL", "?", "HWRASTERIZATION ",
	NULL };

static char *explainDevCaps(DWORD cap){
	return explainDWordFlags(cap, sDevCaps);
}

static void DumpD3DDeviceDesc7(LPD3DDEVICEDESC7 d3, char *label, char *dev)
{
	if(IsTraceD3D){
		if(d3){
			OutTrace("%s: LPD3DDEVICEDESC dev=%s GUID=%#x(%s)\n", label, dev, d3->deviceGUID.Data1, ExplainGUID(&d3->deviceGUID));
			OutTrace("> dwDevCaps=%#x(%s)\n", d3->dwDevCaps, explainDevCaps(d3->dwDevCaps));
			DumpD3DPrimCaps("> dpcLineCaps", &d3->dpcLineCaps);
			DumpD3DPrimCaps("> dpcTriCaps", &d3->dpcLineCaps);
			OutTrace("> dwTextureOpCaps=%#x(%s)\n", d3->dwTextureOpCaps, explainTextureOpCaps(d3->dwTextureOpCaps));
			OutTrace("> dwDeviceRenderBitDepth=%d\n", d3->dwDeviceRenderBitDepth);
			OutTrace("> dwDeviceZBufferBitDepth=%d\n", d3->dwDeviceZBufferBitDepth);
			OutTrace("> dwFVFCaps count=%d%s\n", 
				(d3->dwFVFCaps & D3DFVFCAPS_TEXCOORDCOUNTMASK),
				(d3->dwFVFCaps & D3DFVFCAPS_DONOTSTRIPELEMENTS) ? " DONOTSTRIPELEMENTS" : "");
			OutTrace("> Texture min=(%dx%d) max=(%dx%d)\n", d3->dwMinTextureWidth, d3->dwMinTextureHeight, d3->dwMaxTextureWidth, d3->dwMaxTextureHeight);
			// to be completed ....
			OutHexD3D((unsigned char *)d3, sizeof(D3DDEVICEDESC7));
		}
		else
			OutTrace("%s: LPD3DDEVICEDESC dev=%s ddesc=NULL\n", label, dev);
	}
}
#endif // DXW_NOTRACES

HRESULT WINAPI extDeviceProxy(GUID FAR *lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC lpd3ddd1, LPD3DDEVICEDESC lpd3ddd2, LPVOID arg)
{
	HRESULT res;
	ApiName("EnumDevices");
#ifndef DXW_NOTRACES
	OutTraceD3D("%s: CALLBACK GUID=%#x(%s) DeviceDescription=\"%s\", DeviceName=\"%s\", arg=%#x\n", ApiRef, lpGuid->Data1, ExplainGUID(lpGuid), lpDeviceDescription, lpDeviceName, ((CallbackArg *)arg)->arg);
	DumpD3DDeviceDesc(lpd3ddd1, ApiRef, "HWDEV");
	DumpD3DDeviceDesc(lpd3ddd2, ApiRef, "SWDEV");
#endif // DXW_NOTRACES

	// IID_IDirect3DRampDevice = 0xf2086b20
	if((dxw.dwFlags14 & NORAMPDEVICE) && (lpGuid->Data1 == 0xf2086b20)){
		OutTraceDW("%s: D3DRAMPDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DRGBDevice = 0xa4665c60
	if((dxw.dwFlags14 & NORGBDEVICE) && (lpGuid->Data1 == 0xa4665c60)){
		OutTraceDW("%s: D3DRGBDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DMMXDevice = 0x881949a1
	if((dxw.dwFlags14 & NOMMXDEVICE) && (lpGuid->Data1 == 0x881949a1)){
		OutTraceDW("%s: D3DMMXDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DHALDevice = 0x84e63de0....
	if((dxw.dwFlags8 & NOHALDEVICE) && (lpGuid->Data1 == 0x84e63de0)){
		OutTraceDW("%s: D3DHALDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DTnLHalDevice = 0xf5049e78
	if((dxw.dwFlags12 & NOTNLDEVICE) && (lpGuid->Data1 == 0xf5049e78)){
		OutTraceDW("%s: D3DTNLDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
 
	if((dxw.dwFlags4 & NOPOWER2FIX) || (dxw.dwFlags16 & (FOGVERTEXCAP|FOGTABLECAP))){ 
		D3DDEVICEDESC lpd3ddd1fix, lpd3ddd2fix;
		if(lpd3ddd1) memcpy(&lpd3ddd1fix, lpd3ddd1, sizeof(D3DDEVICEDESC));
		if(lpd3ddd2) memcpy(&lpd3ddd2fix, lpd3ddd2, sizeof(D3DDEVICEDESC));

		if(dxw.dwFlags4 & NOPOWER2FIX){
			lpd3ddd1fix.dpcLineCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
			lpd3ddd1fix.dpcTriCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
			lpd3ddd2fix.dpcLineCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
			lpd3ddd2fix.dpcTriCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
		}

		if(dxw.dwFlags16 & FOGVERTEXCAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGVERTEX @%d\n", ApiRef, __LINE__); 
			lpd3ddd2fix.dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			lpd3ddd2fix.dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
			lpd3ddd2fix.dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			lpd3ddd2fix.dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
		}

		if(dxw.dwFlags16 & FOGTABLECAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGTABLE @%d\n", ApiRef, __LINE__); 
			lpd3ddd2fix.dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			lpd3ddd2fix.dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
			lpd3ddd2fix.dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			lpd3ddd2fix.dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
		}

		res = (*(((CallbackArg *)arg)->cb))(lpGuid, lpDeviceDescription, lpDeviceName, lpd3ddd1?&lpd3ddd1fix:NULL, lpd3ddd2?&lpd3ddd2fix:NULL, ((CallbackArg *)arg)->arg);
	}
	else {
		res = (*(((CallbackArg *)arg)->cb))(lpGuid, lpDeviceDescription, lpDeviceName, lpd3ddd1, lpd3ddd2, ((CallbackArg *)arg)->arg);
	}
	OutTraceD3D("%s: CALLBACK ret=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extDeviceProxy7(LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC7 lpd3ddd, LPVOID arg)
{
	HRESULT res;
	ApiName("EnumDevices(D3D7)");
#ifndef DXW_NOTRACES
	OutTraceD3D("%s: CALLBACK DeviceDescription=\"%s\", DeviceName=\"%s\", arg=%#x\n", ApiRef, lpDeviceDescription, lpDeviceName, ((CallbackArg *)arg)->arg);
	DumpD3DDeviceDesc7(lpd3ddd, ApiRef, "DX7DEV");
#endif // DXW_NOTRACES
	
	// IID_IDirect3DRampDevice = 0xf2086b20
	if((dxw.dwFlags14 & NORAMPDEVICE) && (lpd3ddd->deviceGUID.Data1 == 0xf2086b20)){
		OutTraceDW("%s: D3DRAMPDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DRGBDevice = 0xa4665c60
	if((dxw.dwFlags14 & NORGBDEVICE) && (lpd3ddd->deviceGUID.Data1 == 0xa4665c60)){
		OutTraceDW("%s: D3DRGBDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DMMXDevice = 0x881949a1
	if((dxw.dwFlags14 & NOMMXDEVICE) && (lpd3ddd->deviceGUID.Data1 == 0x881949a1)){
		OutTraceDW("%s: D3DMMXDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DHALDevice = 0x84e63de0....
	if((dxw.dwFlags8 & NOHALDEVICE) && (lpd3ddd->deviceGUID.Data1 == 0x84e63de0)){
		OutTraceDW("%s: D3DHALDEVICE SKIP\n", ApiRef);
		return TRUE;
	}
	// IID_IDirect3DTnLHalDevice = 0xf5049e78
	if((dxw.dwFlags12 & NOTNLDEVICE) && (lpd3ddd->deviceGUID.Data1 == 0xf5049e78)){
		OutTraceDW("%s: D3DTNLDEVICE SKIP\n", ApiRef);
		return TRUE;
	}

	if(dxw.dwFlags11 & TRANSFORMANDLIGHT){
		// v2.05.12: this here to please "Will Rock"
		lpd3ddd->dwDevCaps |= D3DDEVCAPS_HWTRANSFORMANDLIGHT;
	}

	if (lpd3ddd && ((dxw.dwFlags4 & NOPOWER2FIX) || (dxw.dwFlags16 & (FOGVERTEXCAP|FOGTABLECAP)))){ 
		D3DDEVICEDESC7 lpd3dddfix;
		memcpy(&lpd3dddfix, lpd3ddd, sizeof(D3DDEVICEDESC7));

		if(dxw.dwFlags4 & NOPOWER2FIX){
			lpd3dddfix.dpcLineCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
			lpd3dddfix.dpcTriCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
		}

		if(dxw.dwFlags16 & FOGVERTEXCAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGVERTEX @%d\n", ApiRef, __LINE__); 
			lpd3dddfix.dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			lpd3dddfix.dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
			lpd3dddfix.dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			lpd3dddfix.dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
		}

		if(dxw.dwFlags16 & FOGTABLECAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGTABLE @%d\n", ApiRef, __LINE__); 
			lpd3dddfix.dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			lpd3dddfix.dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
			lpd3dddfix.dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			lpd3dddfix.dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
		}

		res = (*(((CallbackArg7 *)arg)->cb))(lpDeviceDescription, lpDeviceName, lpd3ddd?&lpd3dddfix:NULL, ((CallbackArg7 *)arg)->arg);
	}
	else {
		res = (*(((CallbackArg7 *)arg)->cb))(lpDeviceDescription, lpDeviceName, lpd3ddd, ((CallbackArg7 *)arg)->arg);
	}
	OutTraceD3D("%s: CALLBACK ret=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extEnumDevices(ApiArg, int d3dversion, EnumDevices_Type pEnumDevices, void *lpd3d, LPD3DENUMDEVICESCALLBACK cb, LPVOID arg)
{
	HRESULT res;
	CallbackArg Arg;

	OutTraceD3D("%s: d3d=%#x arg=%#x\n", ApiRef, lpd3d, arg);
	Arg.cb= &cb;
	Arg.arg=arg;
	res=(*pEnumDevices)(lpd3d, (LPD3DENUMDEVICESCALLBACK)extDeviceProxy, (LPVOID)&Arg);
	if(res) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	else OutTraceD3D("%s: OK\n", ApiRef);
	return res;
}

HRESULT WINAPI extEnumDevices1(void *lpd3d, LPD3DENUMDEVICESCALLBACK cb, LPVOID arg)
{ return extEnumDevices("IDirect3D::EnumDevices", 1, pEnumDevices1, lpd3d, cb, arg); }
HRESULT WINAPI extEnumDevices2(void *lpd3d, LPD3DENUMDEVICESCALLBACK cb, LPVOID arg)
{ return extEnumDevices("IDirect3D2::EnumDevices", 2, pEnumDevices2, lpd3d, cb, arg); }
HRESULT WINAPI extEnumDevices3(void *lpd3d, LPD3DENUMDEVICESCALLBACK cb, LPVOID arg)
{ return extEnumDevices("IDirect3D3::EnumDevices", 3, pEnumDevices3, lpd3d, cb, arg); }

HRESULT WINAPI extEnumDevices7(void *lpd3d, LPD3DENUMDEVICESCALLBACK7 cb, LPVOID arg)
{
	HRESULT res;
	ApiName("IDirect3D7::EnumDevices");
	CallbackArg7 Arg;

	OutTraceD3D("%s: d3d=%#x arg=%#x\n", ApiRef, lpd3d, arg);
	Arg.cb= &cb;
	Arg.arg=arg;
	res=(*pEnumDevices7)(lpd3d, (LPD3DENUMDEVICESCALLBACK7)extDeviceProxy7, (LPVOID)&Arg);
	if(res) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	else OutTraceD3D("%s: OK\n", ApiRef);
	return res;
}

HRESULT WINAPI extCreateLight(ApiArg, int d3dversion, CreateLight_Type pCreateLight, void *lpd3d, LPDIRECT3DLIGHT *lpLight, IUnknown *p0)
{
	HRESULT res;

	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3d);
	res=(*pCreateLight)(lpd3d, lpLight, p0);
	if(res) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	else OutTraceD3D("%s: OK\n", ApiRef);
	return res;
}

HRESULT WINAPI extCreateLight1(void *lpd3d, LPDIRECT3DLIGHT *lpLight, IUnknown *p0)
{ return extCreateLight("IDirect3D::CreateLight", 1, pCreateLight1, lpd3d, lpLight, p0); }
HRESULT WINAPI extCreateLight2(void *lpd3d, LPDIRECT3DLIGHT *lpLight, IUnknown *p0)
{ return extCreateLight("IDirect3D2::CreateLight", 2, pCreateLight2, lpd3d, lpLight, p0); }
HRESULT WINAPI extCreateLight3(void *lpd3d, LPDIRECT3DLIGHT *lpLight, IUnknown *p0)
{ return extCreateLight("IDirect3D3::CreateLight", 3, pCreateLight3, lpd3d, lpLight, p0); }

#ifdef TRACEMATERIAL
HRESULT WINAPI extCreateMaterial1(void *lpd3d, LPDIRECT3DMATERIAL *lpMaterial, IUnknown *p0)
{
	HRESULT res;
	ApiName("IDirect3D::CreateMaterial");

	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3d);
	res=(*pCreateMaterial1)(lpd3d, lpMaterial, p0);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
		HookMaterial(lpMaterial, 1);
	}
	return res;
}

HRESULT WINAPI extCreateMaterial2(void *lpd3d, LPDIRECT3DMATERIAL2 *lpMaterial, IUnknown *p0)
{
	HRESULT res;
	ApiName("IDirect3D2::CreateMaterial");

	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3d);
	res=(*pCreateMaterial2)(lpd3d, lpMaterial, p0);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
		HookMaterial((LPDIRECT3DMATERIAL *)lpMaterial, 2);
	}
	return res;
}

HRESULT WINAPI extCreateMaterial3(void *lpd3d, LPDIRECT3DMATERIAL3 *lpMaterial, IUnknown *p0)
{
	HRESULT res;
	ApiName("IDirect3D3::CreateMaterial");

	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3d);
	res=(*pCreateMaterial3)(lpd3d, lpMaterial, p0);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
		HookMaterial((LPDIRECT3DMATERIAL *)lpMaterial, 3);
	}
	return res;
}
#endif // TRACEMATERIAL

HRESULT WINAPI extCreateViewport1(void *lpd3d, LPDIRECT3DVIEWPORT *lpViewport, IUnknown *p0)
{
	HRESULT res;
	ApiName("IDirect3D::CreateViewport");

	OutTraceD3D("%s: d3d=%#x p0=%#x\n", ApiRef, lpd3d, p0);
	res=(*pCreateViewport1)(lpd3d, lpViewport, p0);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	} else {
		OutTraceD3D("%s: Viewport=%#x\n", ApiRef, *lpViewport);
		HookViewport(lpViewport, 1);
		gViewport1 = *lpViewport;
	}
	return res;
}

HRESULT WINAPI extCreateViewport2(void *lpd3d, LPDIRECT3DVIEWPORT2 *lpViewport, IUnknown *p0)
{
	HRESULT res;
	ApiName("IDirect3D2::CreateViewport");

	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3d);
	res=(*pCreateViewport2)(lpd3d, lpViewport, p0);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	} else {
		OutTraceD3D("%s: Viewport=%#x\n", ApiRef, *lpViewport);
		HookViewport((LPDIRECT3DVIEWPORT *)lpViewport, 2);
	}
	return res;
}

HRESULT WINAPI extCreateViewport3(void *lpd3d, LPDIRECT3DVIEWPORT3 *lpViewport, IUnknown *p0)
{
	HRESULT res;
	ApiName("IDirect3D3::CreateViewport");

	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3d);
	res=(*pCreateViewport3)(lpd3d, lpViewport, p0);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	} else {
		OutTraceD3D("%s: Viewport=%#x\n", ApiRef, *lpViewport);
		HookViewport((LPDIRECT3DVIEWPORT *)lpViewport, 3);
		//if(IsDebugD3D){
		//	HRESULT res2;
		//	D3DVIEWPORT2 vpdesc;
		//	vpdesc.dwSize = sizeof(D3DVIEWPORT2);
		//	res2=(*pGetViewport2_3)(*lpViewport, &vpdesc);
		//	if(res2) 
		//		if(res2 == D3DERR_VIEWPORTDATANOTSET){
		//			OutTrace("CreateViewport: Viewport data not set\n");
		//		} else {
		//			OutErrorD3D("CreateViewport GetViewport2 ERROR: err=%#x(%s) @%d\n", res2, ExplainDDError(res2), __LINE__);
		//		}
		//	else {
		//		OutTraceD3D("CreateViewport: size=%d pos=(%d,%d) dim=(%dx%d)\n",
		//			vpdesc.dwSize, vpdesc.dwX, vpdesc.dwY, vpdesc.dwWidth, vpdesc.dwHeight);
		//	}
		//}
	}

	return res;
}

static HRESULT WINAPI extFindDevice(ApiArg, int d3dversion, FindDevice_Type pFindDevice, void *lpd3d, LPD3DFINDDEVICESEARCH p1, LPD3DFINDDEVICERESULT p2)
{
	HRESULT res;

	OutTraceD3D("%s: d3d=%#x devsearch=%#x (size=%d flags=%#x caps=%#x primcaps=%#x colormodel=%#x hw=%#x guid=%#x) p2=%#x\n", 
		ApiRef, d3dversion, lpd3d, p1, p1->dwSize, p1->dwFlags, p1->dwCaps, p1->dpcPrimCaps, p1->dcmColorModel, p1->bHardware, p1->guid, p2);
	res=(*pFindDevice)(lpd3d, p1, p2);
	if(res) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	else {
#ifndef DXW_NOTRACES
		OutTraceD3D("%s: GUID=%#x.%#x.%#x.%#x\n", ApiRef, p2->guid.Data1, p2->guid.Data2, p2->guid.Data3, p2->guid.Data4);
		DumpD3DDeviceDesc(&(p2->ddHwDesc), ApiRef, "HWDEV");
		DumpD3DDeviceDesc(&(p2->ddSwDesc), ApiRef, "SWDEV");
#endif // DXW_NOTRACES
	}
	return res;
}

HRESULT WINAPI extFindDevice1(void *lpd3d, LPD3DFINDDEVICESEARCH p1, LPD3DFINDDEVICERESULT p2)
{ return extFindDevice("IDirect3D::FindDevice", 1, pFindDevice1, lpd3d, p1, p2); }
HRESULT WINAPI extFindDevice2(void *lpd3d, LPD3DFINDDEVICESEARCH p1, LPD3DFINDDEVICERESULT p2)
{ return extFindDevice("IDirect3D2::FindDevice", 2, pFindDevice2, lpd3d, p1, p2); }
HRESULT WINAPI extFindDevice3(void *lpd3d, LPD3DFINDDEVICESEARCH p1, LPD3DFINDDEVICERESULT p2)
{ return extFindDevice("IDirect3D3::FindDevice", 3, pFindDevice3, lpd3d, p1, p2); }

HRESULT WINAPI extInitializeVP(void *lpvp, LPDIRECT3D lpd3d)
{
	HRESULT res;

	OutTraceD3D("Initialize(VP): viewport=%#x d3d=%#x\n", lpvp, lpd3d);
	res=(*pInitializeVP)(lpvp, lpd3d);
	if(res) OutErrorD3D("Initialize(VP) ERROR: err=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	else OutTraceD3D("Initialize(VP): OK \n");
	return res;
}

#ifndef D3DERR_NOTAVAILABLE
#define _FACD3D  0x876
#define D3DERR_NOTAVAILABLE MAKE_HRESULT(1, _FACD3D, 2154)
#endif // D3DERR_NOTAVAILABLE

HRESULT WINAPI extCreateDevice2(void *lpd3d, REFCLSID Guid, LPDIRECTDRAWSURFACE lpdds, LPDIRECT3DDEVICE2 *lplpd3dd)
{
	HRESULT res;
	ApiName("IDirect3D2::CreateDevice");

	OutTraceD3D("%s: d3d=%#x GUID=%#x(%s) lpdds=%#x%s\n", 
		ApiRef, lpd3d, Guid.Data1, ExplainGUID((GUID *)&Guid), lpdds, dxwss.ExplainSurfaceRole((LPDIRECTDRAWSURFACE)lpdds));

	if((dxw.dwFlags8 & NOHALDEVICE) && (Guid.Data1 == 0x84e63de0)) {
		OutTraceDW("5S: D3DHALDEVICE SKIP\n", ApiRef);
		return D3DERR_NOTAVAILABLE;
	}

	if((Guid == IID_IDirect3DRampDevice) && dxw.bHintActive) ShowHint(HINT_RAMPDEV);

	if(dxw.dwFlags16 & FORCED3DREFDEVICE){
		OutTraceDW("%s: force IID_IDirect3DRefDevice GUID\n", ApiRef);
		res = (*pCreateDevice2)(lpd3d, IID_IDirect3DRefDevice, lpdds, lplpd3dd);
	}
	else if((dxw.dwFlags16 & (RAMP2RGBDEVICE|RAMP2MMXDEVICE)) && (Guid == IID_IDirect3DRampDevice)){
		OutTraceDW("%s: replacing IID_IDirect3DRampDevice with %s\n", 
			ApiRef, (dxw.dwFlags16 & RAMP2MMXDEVICE) ? "IID_IDirect3DMMXDevice" : "IID_IDirect3DRGBDevice");
		res = (*pCreateDevice2)(lpd3d, 
			(dxw.dwFlags16 & RAMP2MMXDEVICE) ? IID_IDirect3DMMXDevice : IID_IDirect3DRGBDevice, 
			lpdds, lplpd3dd);
	}
	else {
		res = (*pCreateDevice2)(lpd3d, Guid, lpdds, lplpd3dd);
	}

	if((res == D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY) && !(dxw.dwFlags17 & ZBUFONSYSMEMORY)){
		OutTraceDW("%s: retrying with SYTEMMEMORY zbuffer\n", ApiRef);
		dxw.dwFlags17 |= ZBUFONSYSMEMORY;
		return extCreateDevice2(lpd3d, Guid, lpdds, lplpd3dd);
	}
	if((res == D3DERR_ZBUFF_NEEDS_VIDEOMEMORY) && !(dxw.dwFlags19 & ZBUFONVIDMEMORY)){
		OutTraceDW("%s: retrying with VIDEOMEMORY zbuffer\n", ApiRef);
		dxw.dwFlags19 |= ZBUFONVIDMEMORY;
		return extCreateDevice2(lpd3d, Guid, lpdds, lplpd3dd);
	}

	if(res!=DD_OK) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else{
		OutTraceD3D("%s: lpd3d=%#x lpd3dd=%#x\n", ApiRef, lpd3d, *lplpd3dd);
		HookDirect3DDevice((void **)lplpd3dd, 2); 
	}

	if(dxw.dwFlags16 & FORCEWBASEDFOG){
		D3DMATRIX matrix = { 
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 1.0, 0.0};
		//(*lplpd3dd)->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matrix);
		(*pSetTransform2)(*lplpd3dd, D3DTRANSFORMSTATE_PROJECTION, &matrix);
	}
	return res;
}

HRESULT WINAPI extCreateDevice3(void *lpd3d, REFCLSID Guid, LPDIRECTDRAWSURFACE4 lpdds, LPDIRECT3DDEVICE3 *lplpd3dd, LPUNKNOWN unk)
{
	HRESULT res;
	ApiName("IDirect3D3::CreateDevice");

	OutTraceD3D("%s: d3d=%#x GUID=%#x(%s) lpdds=%#x%s\n", 
		ApiRef, lpd3d, Guid.Data1, ExplainGUID((GUID *)&Guid), lpdds, dxwss.ExplainSurfaceRole((LPDIRECTDRAWSURFACE)lpdds));

	if((dxw.dwFlags8 & NOHALDEVICE) && (Guid.Data1 == 0x84e63de0)) {
		OutTraceDW("%s: D3DHALDEVICE SKIP\n", ApiRef);
		return D3DERR_NOTAVAILABLE;
	}

	if((Guid == IID_IDirect3DRampDevice) && dxw.bHintActive) ShowHint(HINT_RAMPDEV);

	isWithinCreateDevice = TRUE;
	if(dxw.dwFlags16 & FORCED3DREFDEVICE){
		OutTraceDW("%s: force IID_IDirect3DRefDevice GUID\n", ApiRef);
		res = (*pCreateDevice3)(lpd3d, IID_IDirect3DRefDevice, lpdds, lplpd3dd, unk);
	}
	else if((dxw.dwFlags16 & (RAMP2RGBDEVICE|RAMP2MMXDEVICE)) && (Guid == IID_IDirect3DRampDevice)){
		OutTraceDW("%s: replacing IID_IDirect3DRampDevice with %s\n", 
			ApiRef, (dxw.dwFlags16 & RAMP2MMXDEVICE) ? "IID_IDirect3DMMXDevice" : "IID_IDirect3DRGBDevice");
		res = (*pCreateDevice3)(lpd3d, 
			(dxw.dwFlags16 & RAMP2MMXDEVICE) ? IID_IDirect3DMMXDevice : IID_IDirect3DRGBDevice, 
			lpdds, lplpd3dd, unk);
	}
	else {
		res = (*pCreateDevice3)(lpd3d, Guid, lpdds, lplpd3dd, unk);
	}
	isWithinCreateDevice = FALSE;

	if((res == D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY) && !(dxw.dwFlags17 & ZBUFONSYSMEMORY)){
		OutTraceDW("%s: retrying with SYTEMMEMORY zbuffer\n", ApiRef);
		dxw.dwFlags17 |= ZBUFONSYSMEMORY;
		return extCreateDevice3(lpd3d, Guid, lpdds, lplpd3dd, unk);
	}
	if((res == D3DERR_ZBUFF_NEEDS_VIDEOMEMORY) && !(dxw.dwFlags19 & ZBUFONVIDMEMORY)){
		OutTraceDW("%s: retrying with VIDEOMEMORY zbuffer\n", ApiRef);
		dxw.dwFlags19 |= ZBUFONVIDMEMORY;
		return extCreateDevice3(lpd3d, Guid, lpdds, lplpd3dd, unk);
	}

	if(res!=DD_OK) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else{
		OutTraceD3D("%s: lpd3d=%#x lpd3dd=%#x\n", ApiRef, lpd3d, *lplpd3dd);
		HookDirect3DDevice((void **)lplpd3dd, 3); 
	}

	if(dxw.dwFlags16 & FORCEWBASEDFOG){
		D3DMATRIX matrix = { 
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 1.0, 0.0};
		//(*lplpd3dd)->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matrix);
		(*pSetTransform3)(*lplpd3dd, D3DTRANSFORMSTATE_PROJECTION, &matrix);
	}
	return res;
}

HRESULT WINAPI extCreateDevice7(void *lpd3d, REFCLSID Guid, LPDIRECTDRAWSURFACE7 lpdds, LPDIRECT3DDEVICE7 *lplpd3dd)
{
	// v2.02.83: D3D CreateDevice (version 7? all versions?) internally calls the Release method upon the backbuffer
	// surface, and this has to be avoided since it causes a crash. 

	HRESULT res;
	ApiName("IDirect3D3::CreateDevice");

	OutTraceD3D("%s: d3d=%#x GUID=%#x(%s) lpdds=%#x%s\n", 
		ApiRef, lpd3d, Guid.Data1, ExplainGUID((GUID *)&Guid), lpdds, dxwss.ExplainSurfaceRole((LPDIRECTDRAWSURFACE)lpdds));

	if((dxw.dwFlags12 & NOTNLDEVICE) && (Guid.Data1 == 0xf5049e78)){
		OutTraceDW("%s: D3DTNLDEVICE SKIP\n", ApiRef);
		return D3DERR_NOTAVAILABLE;
	}
	if((dxw.dwFlags8 & NOHALDEVICE) && (Guid.Data1 == 0x84e63de0)) {
		OutTraceDW("%s: D3DHALDEVICE SKIP\n", ApiRef);
		return D3DERR_NOTAVAILABLE;
	}

	if((Guid == IID_IDirect3DRampDevice) && dxw.bHintActive) ShowHint(HINT_RAMPDEV);

	isWithinCreateDevice = TRUE;
	if(dxw.dwFlags16 & FORCED3DREFDEVICE){
		OutTraceDW("%s: force IID_IDirect3DRefDevice GUID\n", ApiRef);
		res = (*pCreateDevice7)(lpd3d, IID_IDirect3DRefDevice, lpdds, lplpd3dd);
	}
	else if((dxw.dwFlags16 & (RAMP2RGBDEVICE|RAMP2MMXDEVICE)) && (Guid == IID_IDirect3DRampDevice)){
		OutTraceDW("%s: replacing IID_IDirect3DRampDevice with %s\n", 
			ApiRef, (dxw.dwFlags16 & RAMP2MMXDEVICE) ? "IID_IDirect3DMMXDevice" : "IID_IDirect3DRGBDevice");
		res = (*pCreateDevice7)(lpd3d, 
			(dxw.dwFlags16 & RAMP2MMXDEVICE) ? IID_IDirect3DMMXDevice : IID_IDirect3DRGBDevice, 
			lpdds, lplpd3dd);
	}
	else {
		res = (*pCreateDevice7)(lpd3d, Guid, lpdds, lplpd3dd);
	}
	isWithinCreateDevice = FALSE;

	if((res == D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY) && !(dxw.dwFlags17 & ZBUFONSYSMEMORY)){
		OutTraceDW("%s: retrying with SYTEMMEMORY zbuffer\n", ApiRef);
		dxw.dwFlags17 |= ZBUFONSYSMEMORY;
		return extCreateDevice7(lpd3d, Guid, lpdds, lplpd3dd);
	}
	if((res == D3DERR_ZBUFF_NEEDS_VIDEOMEMORY) && !(dxw.dwFlags19 & ZBUFONVIDMEMORY)){
		OutTraceDW("%s: retrying with VIDEOMEMORY zbuffer\n", ApiRef);
		dxw.dwFlags19 |= ZBUFONVIDMEMORY;
		return extCreateDevice7(lpd3d, Guid, lpdds, lplpd3dd);
	}

	if(res == DD_OK){
		OutTraceD3D("%s: lpd3d=%#x lpd3dd=%#x\n", ApiRef, lpd3d, *lplpd3dd);
		HookDirect3DDevice((void **)lplpd3dd, 7); 
	}
	else
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);

	if(dxw.dwFlags16 & FORCEWBASEDFOG){
		D3DMATRIX matrix = { 
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 1.0, 0.0};
		//(*lplpd3dd)->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matrix);
		(*pSetTransform7)(*lplpd3dd, D3DTRANSFORMSTATE_PROJECTION, &matrix);
	}
	return res;
}

static char *ExplainD3DValue(DWORD dwState, DWORD dwValue)
{
	static char sBuf[80];
	char *s;
	switch(dwState){
		case D3DRENDERSTATE_ANTIALIAS:
			s = ExplainAntialiasMode(dwValue);
			break;
		case D3DRENDERSTATE_LINEPATTERN:
			sprintf(sBuf, "RepeatFactor=%d pattern=\"", dwValue & 0x0000FFFF);
			dwValue >>= 16;
			for(int i=0; i<16; i++, dwValue >>= 1) strcat(sBuf, dwValue & 0x00000001 ? "=" : " ");
			strcat(sBuf, "\"");
			s = sBuf;
			break;
		case D3DRENDERSTATE_CULLMODE:
			s = (dwValue == D3DCULL_NONE) ? "NONE" : 
				(dwValue == D3DCULL_CW) ? "CW" :
				(dwValue == D3DCULL_CCW) ? "CCW" : "??";
			break;
		case D3DRENDERSTATE_FOGTABLEMODE:
			s = (dwValue == D3DFOG_NONE) ? "NONE" : 
				(dwValue == D3DFOG_EXP) ? "EXP" :
				(dwValue == D3DFOG_EXP2) ? "EXP2" : 
				(dwValue == D3DFOG_LINEAR) ? "LINEAR" : "??";
			break;
		case D3DRENDERSTATE_SHADEMODE:
			s = ExplainShadeMode(dwValue);
			break;
		case D3DRENDERSTATE_SRCBLEND:
		case D3DRENDERSTATE_DESTBLEND:
			s = ExplainBlendType(dwValue);
			break;
		case D3DRENDERSTATE_ZFUNC:
		case D3DRENDERSTATE_ALPHAFUNC:
		case D3DRENDERSTATE_STENCILFUNC:
			s = ExplainCompFuncType(dwValue);
			break;
		case D3DRENDERSTATE_FILLMODE:
			s = ExplainFillMode(dwValue);
			break;
		case D3DRENDERSTATE_FOGDENSITY:
		case D3DRENDERSTATE_FOGSTART:
		case D3DRENDERSTATE_FOGEND:
			sprintf(sBuf, "%f", *(float *)&dwValue);
			s = sBuf;
			break;
		case D3DRENDERSTATE_STENCILFAIL:
		case D3DRENDERSTATE_STENCILZFAIL:
		case D3DRENDERSTATE_STENCILPASS:
			s = ExplainD3DStencilOp(dwValue);
			break;
		case D3DRENDERSTATE_TEXTUREMAG:
		case D3DRENDERSTATE_TEXTUREMIN:
			s = ExplainD3DTextureFilter(dwValue);
			break;
		case D3DRENDERSTATE_TEXTUREMAPBLEND:
			s = ExplainD3DMapBlend(dwValue);
			break;
		default:
			s = "";
			break;
	}
	//if(strlen(s)) {
	//	sprintf(sBuf, "(%s)", s);
	//	s = sBuf;
	//}
	return s;
}

HRESULT WINAPI extSetRenderState(ApiArg, int version, SetRenderState3_Type pSetRenderState, void *d3dd, D3DRENDERSTATETYPE dwState, DWORD dwValue)
{
	HRESULT res;
#ifndef DXW_NOTRACES
	if(IsTraceD3D){
		OutTrace("%s: d3dd=%#x dwState=%#x(%s) dwValue=%#x(%s)\n", 
			ApiRef, d3dd, dwState, ExplainD3DRenderState(dwState), 
			dwValue, ExplainD3DValue(dwState, dwValue));

	}
#endif // DXW_NOTRACES

	// v2.06.04: fix - trim invalid alpha values. @#@ "Glover" (1998)
	if((dwState == D3DRENDERSTATE_ALPHAREF) && (dwValue & 0xFF00)){
		OutTraceD3D("%s: FIXED dwState=ALPHAREF dwValue=%#x->%#x\n", ApiRef, dwValue & 0xFF);
		dwValue = dwValue & 0xFF;
	}

	if((dxw.dwFlags17 & FORCEDITHERING) && (dwState == D3DRENDERSTATE_DITHERENABLE)) {
		OutTraceD3D("%s: FIXED dwState=DITHER dwValue=%s->TRUE\n", ApiRef, dwValue ? "TRUE" : "FALSE");
		dwValue = TRUE;
	}
	if((dxw.dwFlags17 & CLEARDITHERING) && (dwState == D3DRENDERSTATE_DITHERENABLE)) {
		OutTraceD3D("%s: FIXED dwState=DITHER dwValue=%s->FALSE\n", ApiRef, dwValue ? "TRUE" : "FALSE");
		dwValue = FALSE;
	}
	if((dxw.dwFlags13 & FORCECOLORKEYOFF) && (dwState == (D3DRENDERSTATE_COLORKEYENABLE))) { // v2.05.55
		if(dwValue){
			OutTraceD3D("%s: FIXED dwState=COLORKEYENABLE dwValue=FALSE\n", ApiRef);
			dwValue = FALSE;
		}
	}
	if((dxw.dwFlags4 & NOTEXTURES) && (dwState == (D3DRENDERSTATE_TEXTUREHANDLE))) { // v2.04.88
		OutTraceD3D("%s: NOTEXTURES - skip TEXTUREHANDLE\n", ApiRef);
		return DD_OK;
	}
	if((dxw.dwDFlags & ZBUFFERALWAYS) && (dwState == D3DRENDERSTATE_ZFUNC)) {
		OutTraceD3D("%s: FIXED dwState=ZFUNC dwValue=%s->D3DCMP_ALWAYS\n", ApiRef, ExplainRenderstateValue(dwValue));
		dwValue = D3DCMP_ALWAYS;
	}
	if((dxw.dwFlags2 & WIREFRAME) && (dwState == D3DRENDERSTATE_FILLMODE)){
		OutTraceD3D("%s: FIXED dwState=FILLMODE dwValue=%#x->D3DFILL_WIREFRAME\n", ApiRef, dwValue);
		dwValue = D3DFILL_WIREFRAME;
	}
	if((dxw.dwFlags4 & DISABLEFOGGING) && (dwState == D3DRENDERSTATE_FOGENABLE)){
		OutTraceD3D("%s: FIXED dwState=FOGENABLE dwValue=%#x->FALSE\n", ApiRef, dwValue);
		dwValue = FALSE;
	}
	if((dxw.dwFlags16 & SETFOGCOLOR) && (dwState == D3DRENDERSTATE_FOGCOLOR)){
		OutTraceD3D("%s: FIXED State=FOGCOLOR Value=%#x->%#x\n", ApiRef, dwValue, dxw.fogColor);
		dwValue = dxw.fogColor;
	}
	//if((dxw.dwFlags5 & TEXTURETRANSP) && (dwState == D3DRENDERSTATE_ALPHABLENDENABLE)){
	//	OutTraceD3D("%s: FIXED dwState=ALPHABLENDENABLE dwValue=%#x->TRUE\n", ApiRef, dwValue);
	//	dwValue = TRUE;
	//}
	if((dxw.dwFlags15 & CULLMODENONE) && (dwState == D3DRENDERSTATE_CULLMODE)){
		OutTraceD3D("%s: FIXED dwState=D3DRENDERSTATE_CULLMODE: %d -> 1(D3DCULL_NONE)\n", ApiRef, dwValue);
		dwValue = D3DCULL_NONE;
	}
	if((dxw.dwFlags17 & FORCEFILTERNEAREST) && ((dwState == D3DRENDERSTATE_TEXTUREMIN) || (dwState == D3DRENDERSTATE_TEXTUREMAG))){
		OutTraceD3D("%s: FIXED dwState=D3DRENDERSTATE_TEXTURE%s: %d -> 1(D3DFILTER_NEAREST)\n", 
			ApiRef, (dwState == D3DRENDERSTATE_TEXTUREMIN) ? "MIN" : "MAG", dwValue);
		dwValue = D3DFILTER_NEAREST;
	}

	// beware!!! likely this code would work for interface version 3 only !!!
	if((dwState==D3DRENDERSTATE_ZWRITEENABLE) && (dwValue==TRUE) && (dxw.dwFlags8 & DYNAMICZCLEAN)){
	//if((dwState==D3DRENDERSTATE_ZWRITEENABLE) && (dwValue==FALSE) && (dxw.dwFlags8 & DYNAMICZCLEAN)){
	//if((dwState==D3DRENDERSTATE_ZWRITEENABLE) && (dxw.dwFlags8 & DYNAMICZCLEAN)){
		HRESULT res2;
		LPDIRECT3DVIEWPORT3 vp;
		D3DVIEWPORT vpd;
		res2=((LPDIRECT3DDEVICE3)d3dd)->GetCurrentViewport(&vp);
		if(!res2){
			D3DRECT d3dRect;
			vpd.dwSize=sizeof(D3DVIEWPORT);
			vp->GetViewport(&vpd);
			d3dRect.x1 = vpd.dwX; 
			d3dRect.y1 = vpd.dwY;
			d3dRect.x2 = vpd.dwX + vpd.dwWidth;
			d3dRect.y2 = vpd.dwY + vpd.dwHeight;
			OutTraceD3D("%s: d3dRect=(%d,%d)-(%d,%d)\n", ApiRef, d3dRect.x1, d3dRect.y1, d3dRect.x2, d3dRect.y2);
			if(dxw.dwFlags4 & ZBUFFERCLEAN ) vp->Clear2(1, &d3dRect, D3DCLEAR_ZBUFFER, 0, 1.0, 0);	
			if(dxw.dwFlags4 & ZBUFFER0CLEAN) vp->Clear2(1, &d3dRect, D3DCLEAR_ZBUFFER, 0, 0.0, 0);	
			if(dxw.dwFlags5 & CLEARTARGET) vp->Clear(1, &d3dRect, D3DCLEAR_TARGET);	
		}
	}

	// beware!!! this code would work for interface version 7 only !!!
	if(dxw.dwFlags13 & CONTROLFOGGING){
		char *fogCmd;
		float f;
		switch(dwState){
			/*
			case D3DRENDERSTATE_FOGCOLOR:
				{
					float ff;
					DWORD ir, ig, ib;
					ff = GetHookInfo()->FogFactor;
					ir = (DWORD)((float)(dwValue & 0x0000FF) * ff);
					ig = (DWORD)((float)((dwValue & 0x00FF00) >> 8) * ff);
					ib = (DWORD)((float)((dwValue & 0xFF0000) >> 16) * ff);
					if(ir > 255) ir=255;
					if(ig > 255) ig=255;
					if(ib > 255) ib=255;
					dwValue = ir | (ig << 8) | (ib << 16);		
				}
				break;
			*/
			case D3DRENDERSTATE_FOGDENSITY:
				f = *(float *)&dwValue;
				if(f == 0.0f) f = 1.0f; // make the fog not null
				f = f * GetHookInfo()->FogFactor;
				OutTraceD3D("%s: DENSITY=%f -> %f\n", ApiRef, *(float *)&dwValue, f);
				dwValue = *(DWORD *)&f;
				break;
			case D3DRENDERSTATE_FOGSTART:
			case D3DLIGHTSTATE_FOGEND:
				fogCmd = (dwState == D3DRENDERSTATE_FOGSTART) ? "FOGSTART" : "FOGEND";
				OutTraceD3D("%s: %s=%f\n", ApiRef, fogCmd, *(float *)&dwValue);
				f = *(float *)&dwValue;
				// try-catch to handle divide by zero or float overflow events
				__try {
					f = f / GetHookInfo()->FogFactor;
				}
				__except (EXCEPTION_EXECUTE_HANDLER){ 
					f = FLT_MAX;
				}; 
				OutTraceD3D("%s: %s=%f -> %f\n", ApiRef, fogCmd, *(float *)&dwValue, f);
				dwValue = *(DWORD *)&f;
				break;
		}
	}

	res=(*pSetRenderState)(d3dd, dwState, dwValue);
	_if(res) OutErrorD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extSetRenderState2(void *d3dd, D3DRENDERSTATETYPE State, DWORD Value)
{ return extSetRenderState("IDirect3DDevice2::SetRenderState", 2, pSetRenderState2, d3dd, State, Value); }
HRESULT WINAPI extSetRenderState3(void *d3dd, D3DRENDERSTATETYPE State, DWORD Value)
{ return extSetRenderState("IDirect3DDevice3::SetRenderState", 3, pSetRenderState3, d3dd, State, Value); }
HRESULT WINAPI extSetRenderState7(void *d3dd, D3DRENDERSTATETYPE State, DWORD Value)
{ return extSetRenderState("IDirect3DDevice7::SetRenderState", 7, pSetRenderState7, d3dd, State, Value); }

static HRESULT WINAPI dxwRestoreCallback(LPDIRECTDRAWSURFACE lpDDSurface, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext)
{
	HRESULT res;
	OutTrace("dxwRestoreCallback: ANALYZING lpdds=%#x\n", lpDDSurface);
	if(lpDDSurface->IsLost()){
		if(res=lpDDSurface->Restore()){
			OutTrace("dxwRestoreCallback: RESTORE FAILED lpdds=%#x err=%#x(%s)\n", lpDDSurface, res, ExplainDDError(res));
			return DDENUMRET_CANCEL;
		}
		OutTrace("dxwRestoreCallback: RESTORED lpdds=%#x\n", lpDDSurface);
	}
	return DDENUMRET_OK;
}

HRESULT WINAPI extBeginScene1(void *d3dd)
{
	HRESULT res;
	ApiName("IDirect3DDevice::BeginScene");

	OutTraceD3D("%s: d3dd=%#x\n", ApiRef, d3dd);
		
	if((dxw.dwFlags4 & (ZBUFFERCLEAN|ZBUFFER0CLEAN)) || (dxw.dwFlags5 & CLEARTARGET)){
		if(gViewport1){
			D3DRECT d3dRect;
			DWORD dwFlags;
			d3dRect.x1 = (LONG)0.0;
			d3dRect.y1 = (LONG)0.0;
			d3dRect.x2 = (LONG)dxw.GetScreenWidth();
			d3dRect.y2 = (LONG)dxw.GetScreenHeight();
			dwFlags = 0;
			if(dxw.dwFlags4 & (ZBUFFERCLEAN|ZBUFFER0CLEAN)) dwFlags = D3DCLEAR_ZBUFFER;
			if(dxw.dwFlags5 & CLEARTARGET) dwFlags |= D3DCLEAR_TARGET;
			if(gViewport1){
				res = gViewport1->Clear(1, &d3dRect, dwFlags);	
				_if(res) OutErrorD3D("%s: viewport Clear ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
			}
		}
	}

	if(dxw.dwFlags5 & LIMITBEGINSCENE) LimitFrameCount(dxw.MaxFPS);
	res=(*pBeginScene1)(d3dd);
	if(res == DDERR_SURFACELOST){
		OutTraceDW("%s: recovering from DDERR_SURFACELOST\n", ApiRef);
		lpPrimaryDD->EnumSurfaces(DDENUMSURFACES_DOESEXIST|DDENUMSURFACES_ALL, NULL, NULL, (LPDDENUMSURFACESCALLBACK)dxwRestoreCallback);
		res=(*pBeginScene1)(d3dd);
	}
	_if(res) OutErrorD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res), ApiRef);
	return res;
}

static HRESULT WINAPI dxwRestoreCallback2(LPDIRECTDRAWSURFACE4 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
	HRESULT res;
	OutTrace("dxwRestoreCallback2: ANALYZING lpdds=%#x\n", lpDDSurface);
	if(lpDDSurface->IsLost()){
		if(res=lpDDSurface->Restore()){
			OutTrace("dxwRestoreCallback2: RESTORE FAILED lpdds=%#x err=%#x(%s)\n", lpDDSurface, res, ExplainDDError(res));
			return DDENUMRET_CANCEL;
		}
		OutTrace("dxwRestoreCallback2: RESTORED lpdds=%#x\n", lpDDSurface);
	}
	return DDENUMRET_OK;
}

HRESULT WINAPI extBeginScene2(void *d3dd)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::BeginScene");

	OutTraceD3D("%s: d3dd=%#x\n", ApiRef, d3dd);
	if(dxw.dwFlags5 & LIMITBEGINSCENE) LimitFrameCount(dxw.MaxFPS);
	if((dxw.dwFlags4 & (ZBUFFERCLEAN|ZBUFFER0CLEAN)) || (dxw.dwFlags5 & CLEARTARGET)){
		LPDIRECT3DVIEWPORT2 vp;
		D3DVIEWPORT vpd;
		res=((LPDIRECT3DDEVICE2)d3dd)->GetCurrentViewport(&vp);
		if(!res){
			D3DRECT d3dRect;
			DWORD dwFlags;
			vpd.dwSize=sizeof(D3DVIEWPORT);
			vp->GetViewport(&vpd);
			d3dRect.x1 = vpd.dwX; 
			d3dRect.y1 = vpd.dwY;
			d3dRect.x2 = vpd.dwX + vpd.dwWidth;
			d3dRect.y2 = vpd.dwY + vpd.dwHeight;
			OutTraceD3D("%s: d3dRect=(%d,%d)-(%d,%d)\n", ApiRef, d3dRect.x1, d3dRect.y1, d3dRect.x2, d3dRect.y2);
			dwFlags = 0;
			if(dxw.dwFlags4 & (ZBUFFERCLEAN|ZBUFFER0CLEAN)) dwFlags = D3DCLEAR_ZBUFFER;
			if(dxw.dwFlags5 & CLEARTARGET) dwFlags |= D3DCLEAR_TARGET;
			if(gViewport1){ // v2.05.58 fix crash in "Snowboard Racer"
				res = gViewport1->Clear(1, &d3dRect, dwFlags);	
				_if(res) OutErrorD3D("%s: viewport Clear ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
			}
		}
	}
	res=(*pBeginScene2)(d3dd);
	if(res == DDERR_SURFACELOST){
		OutTraceDW("%s: recovering from DDERR_SURFACELOST\n", ApiRef);
		lpPrimaryDD->EnumSurfaces(DDENUMSURFACES_DOESEXIST|DDENUMSURFACES_ALL, NULL, NULL, (LPDDENUMSURFACESCALLBACK)dxwRestoreCallback2);
		res=(*pBeginScene2)(d3dd);
	}
	_if(res) OutErrorD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extBeginScene3(void *d3dd)
{
	HRESULT res;
	ApiName("IDirect3DDevice3::BeginScene");

	OutTraceD3D("%s: d3dd=%#x\n", ApiRef, d3dd);
	if(dxw.dwFlags5 & LIMITBEGINSCENE) LimitFrameCount(dxw.MaxFPS);
	if((dxw.dwFlags4 & (ZBUFFERCLEAN|ZBUFFER0CLEAN)) || (dxw.dwFlags5 & CLEARTARGET)){
		LPDIRECT3DVIEWPORT3 vp;
		D3DVIEWPORT vpd;
		res=((LPDIRECT3DDEVICE3)d3dd)->GetCurrentViewport(&vp);
		if(!res){
			D3DRECT d3dRect;
			vpd.dwSize=sizeof(D3DVIEWPORT);
			vp->GetViewport(&vpd);
			d3dRect.x1 = vpd.dwX; 
			d3dRect.y1 = vpd.dwY;
			d3dRect.x2 = vpd.dwX + vpd.dwWidth;
			d3dRect.y2 = vpd.dwY + vpd.dwHeight;
			OutTraceD3D("%s: d3dRect=(%d,%d)-(%d,%d)\n", ApiRef, d3dRect.x1, d3dRect.y1, d3dRect.x2, d3dRect.y2);
			if(dxw.dwFlags4 & ZBUFFERCLEAN )vp->Clear2(1, &d3dRect, D3DCLEAR_ZBUFFER, 0, 1.0, 0);	
			if(dxw.dwFlags4 & ZBUFFER0CLEAN)vp->Clear2(1, &d3dRect, D3DCLEAR_ZBUFFER, 0, 0.0, 0);	
			if(dxw.dwFlags5 & CLEARTARGET) vp->Clear(1, &d3dRect, D3DCLEAR_TARGET);	
		}
	}
	res=(*pBeginScene3)(d3dd);
	if(res == DDERR_SURFACELOST){
		OutTraceDW("%s: recovering from DDERR_SURFACELOST\n", ApiRef);
		lpPrimaryDD->EnumSurfaces(DDENUMSURFACES_DOESEXIST|DDENUMSURFACES_ALL, NULL, NULL, (LPDDENUMSURFACESCALLBACK)dxwRestoreCallback2);
		res=(*pBeginScene3)(d3dd);
	}
	_if(res) OutErrorD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

static HRESULT WINAPI dxwRestoreCallback7(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
	HRESULT res;
	OutTrace("dxwRestoreCallback7: ANALYZING lpdds=%#x\n", lpDDSurface);
	if(lpDDSurface->IsLost()){
		if(res=lpDDSurface->Restore()){
			OutTrace("dxwRestoreCallback7: RESTORE FAILED lpdds=%#x err=%#x(%s)\n", lpDDSurface, res, ExplainDDError(res));
			return DDENUMRET_CANCEL;
		}
		OutTrace("dxwRestoreCallback7: RESTORED lpdds=%#x\n", lpDDSurface);
	}
	return DDENUMRET_OK;
}

HRESULT WINAPI extBeginScene7(void *d3dd)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::BeginScene");

	OutTraceD3D("%s: d3dd=%#x\n", ApiRef, d3dd);
	
	if(dxw.dwFlags5 & LIMITBEGINSCENE) LimitFrameCount(dxw.MaxFPS);

	// there is no Clear method for Viewport object in D3D7 !!!

	res=(*pBeginScene7)(d3dd);
	if(res == DDERR_SURFACELOST){
		OutTraceDW("%s: recovering from DDERR_SURFACELOST\n", ApiRef);
		lpPrimaryDD->EnumSurfaces(DDENUMSURFACES_DOESEXIST|DDENUMSURFACES_ALL, NULL, NULL, (LPDDENUMSURFACESCALLBACK)dxwRestoreCallback7);
		res=(*pBeginScene7)(d3dd);
	}
	_if(res) OutErrorD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

static HRESULT WINAPI extEndScene(ApiArg, int d3dversion, Scene_Type pEndScene, void *d3dd)
{
	HRESULT res;
	OutTraceD3D("%s: d3dd=%#x\n", ApiRef, d3dd);
	res=(*pEndScene)(d3dd);
	//dxw.ShowOverlay();
	_if(res) OutErrorD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extEndScene1(void *d3dd)
{ return extEndScene("IDirect3DDevice::EndScene", 1, pEndScene1, d3dd); }
HRESULT WINAPI extEndScene2(void *d3dd)
{ return extEndScene("IDirect3DDevice2::EndScene", 2, pEndScene2, d3dd); }
HRESULT WINAPI extEndScene3(void *d3dd)
{ return extEndScene("IDirect3DDevice3::EndScene", 3, pEndScene3, d3dd); }
HRESULT WINAPI extEndScene7(void *d3dd)
{ return extEndScene("IDirect3DDevice7::EndScene", 7, pEndScene7, d3dd); }

HRESULT WINAPI extD3DGetCaps(ApiArg, int d3dversion, D3DGetCaps_Type pD3DGetCaps, void *d3dd, LPD3DDEVICEDESC hd, LPD3DDEVICEDESC sd)
{
	HRESULT res;

	OutTraceD3D("%s: d3dd=%#x hd=%#x sd=%#x\n", ApiRef, d3dd, hd, sd);
	res=(*pD3DGetCaps)(d3dd, hd, sd);
	if(res) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	}

#ifndef DXW_NOTRACES
	DumpD3DDeviceDesc(hd, ApiRef, "HWDEV");
	DumpD3DDeviceDesc(sd, ApiRef, "SWDEV");
#endif // DXW_NOTRACES

	if(hd) {
		if(dxw.dwFlags4 & NOPOWER2FIX){
			OutTraceDW("%s: Fixing NOPOWER2FIX hw caps\n", ApiRef);
            hd->dpcLineCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
            hd->dpcTriCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
		}
		if(dxw.dwFlags16 & FOGVERTEXCAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGVERTEX @%d\n", ApiRef, __LINE__); 
			hd->dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			hd->dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
			hd->dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			hd->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
		}
		if(dxw.dwFlags16 & FOGTABLECAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGTABLE @%d\n", ApiRef, __LINE__); 
			hd->dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			hd->dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
			hd->dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			hd->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
		}
    }
    if(sd) {
		if(dxw.dwFlags4 & NOPOWER2FIX){
			OutTraceDW("%s: Fixing NOPOWER2FIX sw caps\n", ApiRef);
            sd->dpcLineCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
            sd->dpcTriCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
		}
		if(dxw.dwFlags16 & FOGVERTEXCAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGVERTEX @%d\n", ApiRef, __LINE__); 
			sd->dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			sd->dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
			sd->dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			sd->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
		}
		if(dxw.dwFlags16 & FOGTABLECAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGTABLE @%d\n", ApiRef, __LINE__); 
			sd->dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			sd->dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
			sd->dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			sd->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
        }
    }

	return res;
}

HRESULT WINAPI extD3DGetCaps1(void *d3dd, LPD3DDEVICEDESC hd, LPD3DDEVICEDESC sd)
{ return extD3DGetCaps("IDirect3DDevice::GetCaps", 1, pD3DGetCaps1, d3dd, hd, sd); }
HRESULT WINAPI extD3DGetCaps2(void *d3dd, LPD3DDEVICEDESC hd, LPD3DDEVICEDESC sd)
{ return extD3DGetCaps("IDirect3DDevice2::GetCaps", 2, pD3DGetCaps2, d3dd, hd, sd); }
HRESULT WINAPI extD3DGetCaps3(void *d3dd, LPD3DDEVICEDESC hd, LPD3DDEVICEDESC sd)
{ return extD3DGetCaps("IDirect3DDevice3::GetCaps", 3, pD3DGetCaps3, d3dd, hd, sd); }

HRESULT WINAPI extD3DGetCaps7(void *d3dd, LPD3DDEVICEDESC7 lpd3ddd)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::GetCaps");

	OutTraceD3D("%s: d3dd=%#x lpd3ddd=%#x\n", ApiRef, d3dd, lpd3ddd);
	res=(*pD3DGetCaps7)(d3dd, lpd3ddd);
	if(res) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	}

#ifndef DXW_NOTRACES
	DumpD3DDeviceDesc7(lpd3ddd, ApiRef, "DEV7");
#endif // DXW_NOTRACES

	if(lpd3ddd) {
		if(dxw.dwFlags4 & NOPOWER2FIX){
			OutTraceDW("%s: Fixing NOPOWER2FIX hw caps\n", ApiRef);
            lpd3ddd->dpcLineCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
            lpd3ddd->dpcTriCaps.dwTextureCaps|=D3DPTEXTURECAPS_NONPOW2CONDITIONAL|D3DPTEXTURECAPS_POW2;
        }

		if(dxw.dwFlags16 & FOGVERTEXCAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGVERTEX @%d\n", ApiRef, __LINE__); 
			lpd3ddd->dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			lpd3ddd->dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
			lpd3ddd->dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
			lpd3ddd->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
		}

		if(dxw.dwFlags16 & FOGTABLECAP){
			OutTrace("%s: force D3DPRASTERCAPS_FOGTABLE @%d\n", ApiRef, __LINE__); 
			lpd3ddd->dpcLineCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			lpd3ddd->dpcLineCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
			lpd3ddd->dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_FOGTABLE;
			lpd3ddd->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGVERTEX;
		}

		if(dxw.dwFlags11 & TRANSFORMANDLIGHT){
			OutTraceDW("%s: Adding HWTRANSFORMANDLIGHT caps\n", ApiRef);
            lpd3ddd->dwDevCaps|=D3DDEVCAPS_HWTRANSFORMANDLIGHT;
        }

		if(dxw.dwFlags12 & NOTNLDEVICE){
			OutTraceDW("%s: Removing HWTRANSFORMANDLIGHT caps\n", ApiRef);
            lpd3ddd->dwDevCaps&=~D3DDEVCAPS_HWTRANSFORMANDLIGHT;
        }
	}
	return res;
}

HRESULT WINAPI extGetLightState2(void *lpd3dd, D3DLIGHTSTATETYPE dwState, LPDWORD pdwValue)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::GetLightState");

	OutTraceD3D("%s: lpd3dd=%#x state=%#x\n", ApiRef, lpd3dd, dwState);
	res = (*pGetLightState2)(lpd3dd, dwState, pdwValue);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: lpd3dd=%#x state=%#x value=%#x\n", ApiRef, lpd3dd, dwState, *pdwValue);
	}
	return res;
}

HRESULT WINAPI extSetLightState(ApiArg, SetLightState_Type pSetLightState, void *lpd3dd, D3DLIGHTSTATETYPE dwState, DWORD dwValue)
{
	HRESULT res;
#ifndef DXW_NOTRACES
	char *sLightModes[] = {"???", "MATERIAL", "AMBIENT", "COLORMODEL", 
		"FOGMODE", "FOGSTART", "FOGEND", "FOGDENSITY", "COLORVERTEX", "???"};
	OutTraceD3D("%s: lpd3dd=%#x state=%d(%s) value=%#x\n", ApiRef, lpd3dd, dwState, 
		sLightModes[dwState <= D3DLIGHTSTATE_COLORVERTEX ? dwState : D3DLIGHTSTATE_COLORVERTEX+1],
		dwValue);
#endif // DXW_NOTRACES
	if(dxw.dwFlags13 & CONTROLFOGGING){
		char *fogCmd;
		float f;
		switch(dwState){
			case D3DLIGHTSTATE_FOGDENSITY:
				f = *(float *)&dwValue;
				if(f == 0.0f) f = 1.0f; // make the fog not null
				f = f * GetHookInfo()->FogFactor;
				OutTraceD3D("%s: DENSITY=%f -> %f\n", ApiRef, *(float *)&dwValue, f);
				dwValue = *(DWORD *)&f;
				break;
			case D3DLIGHTSTATE_FOGMODE:
#ifndef DXW_NOTRACES
				{
				char *sFogModes[] = {"NONE", "EXP", "EXP2", "LINEAR", "unknown"};
				OutTraceD3D("%s: fogmode=%d(%s)\n", ApiRef, dwValue, 
					sFogModes[dwValue > D3DFOG_LINEAR ? D3DFOG_LINEAR+1 : dwValue]);
				}
#endif // DXW_NOTRACES
				break;
			case D3DLIGHTSTATE_FOGSTART:
			case D3DLIGHTSTATE_FOGEND:
				fogCmd = (dwState == D3DRENDERSTATE_FOGSTART) ? "FOGSTART" : "FOGEND";
				OutTraceD3D("%s: %s=%f\n", ApiRef, fogCmd, *(float *)&dwValue);
				f = *(float *)&dwValue;
				// try-catch to handle divide by zero or float overflow events
				__try {
					f = f / GetHookInfo()->FogFactor;
				}
				__except (EXCEPTION_EXECUTE_HANDLER){ 
					f = FLT_MAX;
				}; 
				OutTraceD3D("%s: %s=%f -> %f\n", ApiRef, fogCmd, *(float *)&dwValue, f);
				dwValue = *(DWORD *)&f;
				break;
		}
	}
	res = (*pSetLightState)(lpd3dd, dwState, dwValue);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", api, res, ExplainDDError(res));
	}
	return res;
}

HRESULT WINAPI extSetLightState2(void *d3dd, D3DLIGHTSTATETYPE d3dls, DWORD t)
{ return extSetLightState("IDirect3DDevice2::SetLightState", pSetLightState2, d3dd, d3dls, t); }
HRESULT WINAPI extSetLightState3(void *d3dd, D3DLIGHTSTATETYPE d3dls, DWORD t)
{ return extSetLightState("IDirect3DDevice3::SetLightState", pSetLightState3, d3dd, d3dls, t); }

HRESULT WINAPI extAddViewport1(void *d3dd, LPDIRECT3DVIEWPORT lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice::AddViewport");

	static VOID *LastDevice = 0;
	OutTraceD3D("%s: d3d=%#x d3dvp=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pAddViewport1)(d3dd, lpd3dvp);
	if((res == DDERR_INVALIDPARAMS) && LastDevice) {
		// going through here fixes "Die hard trilogy" "DirectX error 15" caused by an AddViewport failure
		OutErrorD3D("%s: DDERR_INVALIDPARAMS: try to unlink from d3dd=%#x\n", ApiRef, LastDevice);
		res=((LPDIRECT3DDEVICE)LastDevice)->DeleteViewport(lpd3dvp);
		_if(res) OutTrace("%s: DeleteViewport ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		res=(*pAddViewport1)(d3dd, lpd3dvp);
	}
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		if(dxw.dwFlags1 & SUPPRESSDXERRORS) res=DD_OK;
	}
	else{
		LastDevice = d3dd;
		lpCurrViewport = lpd3dvp;
	}
	return res;
}

HRESULT WINAPI extAddViewport2(void *d3dd, LPDIRECT3DVIEWPORT2 lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::AddViewport");

	static VOID *LastDevice = 0;
	OutTraceD3D("%s: d3d=%#x d3dvp=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pAddViewport2)(d3dd, lpd3dvp);
	if((res == DDERR_INVALIDPARAMS) && LastDevice) {
		OutErrorD3D("%s: DDERR_INVALIDPARAMS: try to unlink from d3dd=%#x\n", ApiRef, LastDevice);
		res=((LPDIRECT3DDEVICE2)LastDevice)->DeleteViewport(lpd3dvp);
		_if(res) OutTrace("%s: DeleteViewport ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		res=(*pAddViewport2)(d3dd, lpd3dvp);
	}
	if(res) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		if(dxw.dwFlags1 & SUPPRESSDXERRORS) res=DD_OK;
	}
	else
		LastDevice = d3dd;
	return res;
}

HRESULT WINAPI extAddViewport3(void *d3dd, LPDIRECT3DVIEWPORT3 lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice3::AddViewport");

	static VOID *LastDevice = 0;
	OutTraceD3D("%s: d3d=%#x d3dvp=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pAddViewport3)(d3dd, lpd3dvp);
	if((res == DDERR_INVALIDPARAMS) && LastDevice) {
		OutErrorD3D("%s: DDERR_INVALIDPARAMS: try to unlink from d3dd=%#x\n", ApiRef, LastDevice);
		res=((LPDIRECT3DDEVICE3)LastDevice)->DeleteViewport(lpd3dvp);
		_if(res) OutTrace("%s: DeleteViewport ERROR: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		res=(*pAddViewport3)(d3dd, lpd3dvp);
	}
	if(res) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		if(dxw.dwFlags1 & SUPPRESSDXERRORS) res=DD_OK;
	}
	else
		LastDevice = d3dd;
	return res;
}

HRESULT WINAPI extSetViewport2_2(void *lpvp, LPD3DVIEWPORT2 vpd)
{
	HRESULT res;
	ApiName("IDirect3DViewport2::SetViewport2");

	OutTraceD3D("%s: viewport=%#x viewportd=%#x size=%d pos=(%d,%d) dim=(%dx%d) clippos=(%f:%f) clipsize=(%f:%f) Z=(%f:%f)\n", 
		ApiRef, lpvp, vpd, vpd->dwSize, vpd->dwX, vpd->dwY, vpd->dwWidth, vpd->dwHeight, 
		vpd->dvClipX, vpd->dvClipY, vpd->dvClipWidth, vpd->dvClipHeight,
		vpd->dvMinZ, vpd->dvMaxZ);

	res=(*pSetViewport2_2)(lpvp, vpd);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

HRESULT WINAPI extGetViewport2_2(void *lpvp, LPD3DVIEWPORT2 vpd)
{
	HRESULT res;
	ApiName("IDirect3DViewport2::GetViewport2");

	OutTraceD3D("%s: viewport=%#x viewportd=%#x\n", ApiRef, lpvp, vpd);
	res=(*pGetViewport2_2)(lpvp, vpd);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK size=%d pos=(%d,%d) dim=(%dx%d)\n",
			ApiRef, vpd->dwSize, vpd->dwX, vpd->dwY, vpd->dwWidth, vpd->dwHeight);	
	}
	return res;
}

HRESULT WINAPI extSetViewport2_3(void *lpvp, LPD3DVIEWPORT2 vpd)
{
	HRESULT res;
	ApiName("IDirect3DViewport3::SetViewport2");

	OutTraceD3D("%s: viewport=%#x viewportd=%#x size=%d pos=(%d,%d) dim=(%dx%d) Z=(%f-%f)\n", 
		ApiRef, lpvp, vpd, vpd->dwSize, vpd->dwX, vpd->dwY, vpd->dwWidth, vpd->dwHeight, vpd->dvMinZ, vpd->dvMaxZ);
	res=(*pSetViewport2_3)(lpvp, vpd);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

HRESULT WINAPI extGetViewport2_3(void *lpvp, LPD3DVIEWPORT2 vpd)
{
	HRESULT res;
	ApiName("IDirect3DViewport3::GetViewport2");

	OutTraceD3D("%s: viewport=%#x viewportd=%#x\n", ApiRef, lpvp, vpd);
	res=(*pGetViewport2_3)(lpvp, vpd);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK size=%d pos=(%d,%d) dim=(%dx%d)\n",
			ApiRef, vpd->dwSize, vpd->dwX, vpd->dwY, vpd->dwWidth, vpd->dwHeight);	
	}
	return res;
}

HRESULT WINAPI extGetCurrentViewport2(void *d3dd, LPDIRECT3DVIEWPORT2 *lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::GetCurrentViewport");

	OutTraceD3D("%s: d3dd=%#x viewportd=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pGetCurrentViewport2)(d3dd, lpd3dvp);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
		HookViewport((LPDIRECT3DVIEWPORT *)lpd3dvp, 2);
	}
	return res;
}

HRESULT WINAPI extSetCurrentViewport2(void *d3dd, LPDIRECT3DVIEWPORT2 lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::SetCurrentViewport");

	OutTraceD3D("%s: d3dd=%#x viewportd=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pSetCurrentViewport2)(d3dd, lpd3dvp);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

HRESULT WINAPI extGetCurrentViewport3(void *d3dd, LPDIRECT3DVIEWPORT3 *lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice3::GetCurrentViewport");

	OutTraceD3D("%s: d3dd=%#x viewportd=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pGetCurrentViewport3)(d3dd, lpd3dvp);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
		return res;
	}
	HookViewport((LPDIRECT3DVIEWPORT *)lpd3dvp, 3);
	//if(IsDebugD3D){
	//	HRESULT res2;
	//	D3DVIEWPORT2 vpdesc;
	//	vpdesc.dwSize = sizeof(D3DVIEWPORT2);
	//	res2=(*pGetViewport2_3)(lpd3dvp, &vpdesc);
	//	if(res2) 
	//		OutErrorD3D("GetCurrentViewport(D3DD3) GetViewport2 ERROR: err=%#x(%s) @%d\n", res2, ExplainDDError(res2), __LINE__);
	//	else
	//		OutTraceD3D("GetCurrentViewport(D3DD3): size=%d pos=(%d,%d) dim=(%dx%d)\n",
	//			vpdesc.dwSize, vpdesc.dwX, vpdesc.dwY, vpdesc.dwWidth, vpdesc.dwHeight);
	//}
	return res;
}

HRESULT WINAPI extSetCurrentViewport3(void *lpd3dd, LPDIRECT3DVIEWPORT3 lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice3::SetCurrentViewport");

	OutTraceD3D("%s: d3dd=%#x viewportd=%#x\n", ApiRef, lpd3dd, lpd3dvp);
	res=(*pSetCurrentViewport3)(lpd3dd, lpd3dvp);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
		return res;
	}
	OutTraceD3D("%s: OK\n", ApiRef);
	//if(IsDebugD3D){
	//	HRESULT res2;
	//	D3DVIEWPORT2 vpdesc;
	//	vpdesc.dwSize = sizeof(D3DVIEWPORT2);
	//	res2=(*pGetViewport2_3)(lpd3dvp, &vpdesc);
	//	if(res2) 
	//		OutErrorD3D("SetCurrentViewport(D3DD3) GetViewport2 ERROR: err=%#x(%s) @%d\n", res2, ExplainDDError(res2), __LINE__);
	//	else
	//		OutTraceD3D("SetCurrentViewport(D3DD3): size=%d pos=(%d,%d) dim=(%dx%d)\n",
	//			vpdesc.dwSize, vpdesc.dwX, vpdesc.dwY, vpdesc.dwWidth, vpdesc.dwHeight);
	//}
	return res;
}

HRESULT WINAPI extDeleteViewport1(void *d3dd, LPDIRECT3DVIEWPORT lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice::DeletetViewport");

	OutTraceD3D("%s: d3dd=%#x viewport=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pDeleteViewport1)(d3dd, lpd3dvp);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

HRESULT WINAPI extNextViewport1(void *d3dd, LPDIRECT3DVIEWPORT lpd3dvp, LPDIRECT3DVIEWPORT *vpnext, DWORD dw)
{
	HRESULT res;
	ApiName("IDirect3DDevice::NextViewport");

	OutTraceD3D("%s: d3dd=%#x viewport=%#x dw=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pNextViewport1)(d3dd, lpd3dvp, vpnext, dw);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

HRESULT WINAPI extDeleteViewport2(void *d3dd, LPDIRECT3DVIEWPORT2 lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::DeletetViewport");

	OutTraceD3D("%s: d3dd=%#x viewport=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pDeleteViewport2)(d3dd, lpd3dvp);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

HRESULT WINAPI extNextViewport2(void *d3dd, LPDIRECT3DVIEWPORT2 lpd3dvp, LPDIRECT3DVIEWPORT2 *vpnext, DWORD dw)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::NextViewport");

	OutTraceD3D("%s: d3dd=%#x viewport=%#x dw=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pNextViewport2)(d3dd, lpd3dvp, vpnext, dw);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

HRESULT WINAPI extSetTexture3(void *d3dd, DWORD stage, LPDIRECT3DTEXTURE2 lptex)
{
	HRESULT res;
	ApiName("IDirect3DDevice3::SetTexture");

	OutTraceD3D("%s: d3dd=%#x stage=%d tex=%#x\n", ApiRef, d3dd, stage, lptex);
	if (dxw.dwFlags4 & NOTEXTURES) return DD_OK;

	res=(*pSetTexture3)(d3dd, stage, lptex);
	_if(res) OutTraceD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extSetTexture7(void *d3dd, DWORD stage, LPDIRECTDRAWSURFACE7 lptex)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::SetTexture");

	OutTraceD3D("%s: d3dd=%#x stage=%d tex=%#x\n", ApiRef, d3dd, stage, lptex);
	if (dxw.dwFlags4 & NOTEXTURES) return DD_OK;

	res=(*pSetTexture7)(d3dd, stage, lptex);
	_if(res) OutTraceD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

#ifdef TRACEMATERIAL
HRESULT WINAPI extSetMaterial(ApiArg, int version, SetMaterial_Type pSetMaterial, void *d3dm, LPD3DMATERIAL lpMaterial)
{
	HRESULT res;

	OutTraceD3D("%s: d3dd=%#x material=%#x\n", ApiRef, d3dm, lpMaterial);
	if(lpMaterial && IsDebugD3D){
		OutTraceD3D("%s: Size=%d Texture=%#x diffuse=(%f,%f,%f,%f) ambient=(%f,%f,%f,%f) specular=(%f,%f,%f,%f) emissive=(%f,%f,%f,%f) power=%f\n", 
			ApiRef,
			lpMaterial->dwSize, lpMaterial->hTexture, 
			lpMaterial->diffuse.a, lpMaterial->diffuse.r, lpMaterial->diffuse.g, lpMaterial->diffuse.b,
			lpMaterial->ambient.a, lpMaterial->ambient.r, lpMaterial->ambient.g, lpMaterial->ambient.b,
			lpMaterial->specular.a, lpMaterial->specular.r, lpMaterial->specular.g, lpMaterial->specular.b,
			lpMaterial->emissive.a, lpMaterial->emissive.r, lpMaterial->emissive.g, lpMaterial->emissive.b,
			lpMaterial->power
		);
	}
	res=(*pSetMaterial)(d3dm, lpMaterial);
	if(res) OutTraceD3D("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extSetMaterial1(void *d3dm, LPD3DMATERIAL lpMaterial)
{ return extSetMaterial("IDirect3DMaterial::SetMaterial", 1, pSetMaterial1, d3dm, lpMaterial); }
HRESULT WINAPI extSetMaterial2(void *d3dm, LPD3DMATERIAL lpMaterial)
{ return extSetMaterial("IDirect3DMaterial2::SetMaterial", 2, pSetMaterial2, d3dm, lpMaterial); }
HRESULT WINAPI extSetMaterial3(void *d3dm, LPD3DMATERIAL lpMaterial)
{ return extSetMaterial("IDirect3DMaterial3::SetMaterial", 3, pSetMaterial3, d3dm, lpMaterial); }

HRESULT WINAPI extSetMaterial7(void *d3dd, LPD3DMATERIAL7 lpMaterial)
{
	HRESULT res;
	ApiName("IDirect3DMaterial7::SetMaterial");

	OutTraceD3D("%s: d3dd=%#x material=%#x\n", ApiRef, d3dd, lpMaterial);
	if(lpMaterial && IsDebugD3D){
		OutTraceD3D("%s: diffuse=(%f,%f,%f,%f) ambient=(%f,%f,%f,%f) specular=(%f,%f,%f,%f) emissive=(%f,%f,%f,%f) power=%f\n", 
			ApiRef,
			lpMaterial->diffuse.a, lpMaterial->diffuse.r, lpMaterial->diffuse.g, lpMaterial->diffuse.b,
			lpMaterial->ambient.a, lpMaterial->ambient.r, lpMaterial->ambient.g, lpMaterial->ambient.b,
			lpMaterial->specular.a, lpMaterial->specular.r, lpMaterial->specular.g, lpMaterial->specular.b,
			lpMaterial->emissive.a, lpMaterial->emissive.r, lpMaterial->emissive.g, lpMaterial->emissive.b,
			lpMaterial->power
		);
	}

	res=(*pSetMaterial7)(d3dd, lpMaterial);
	if(res) OutTraceD3D("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extGetMaterial(ApiArg, int version, GetMaterial_Type pGetMaterial, void *d3dm, LPD3DMATERIAL lpMaterial)
{
	HRESULT res;

	res=(*pGetMaterial)(d3dm, lpMaterial);
	OutTraceD3D("%s: d3dm=%#x material=%#x res=%#x\n", ApiRef, d3dm, lpMaterial, res);
	if(lpMaterial && IsDebugD3D && (res==DD_OK)){
		OutTraceD3D("%s: Size=%d diffuse=(%f,%f,%f,%f) ambient=(%f,%f,%f,%f) specular=(%f,%f,%f,%f) emissive=(%f,%f,%f,%f) power=%f\n", 
			ApiRef,
			lpMaterial->dwSize, 
			lpMaterial->diffuse.a, lpMaterial->diffuse.r, lpMaterial->diffuse.g, lpMaterial->diffuse.b,
			lpMaterial->ambient.a, lpMaterial->ambient.r, lpMaterial->ambient.g, lpMaterial->ambient.b,
			lpMaterial->specular.a, lpMaterial->specular.r, lpMaterial->specular.g, lpMaterial->specular.b,
			lpMaterial->emissive.a, lpMaterial->emissive.r, lpMaterial->emissive.g, lpMaterial->emissive.b,
			lpMaterial->power
		);
	}
	return res;
}

HRESULT WINAPI extGetMaterial1(void *d3dm, LPD3DMATERIAL lpMaterial)
{ return extGetMaterial("IDirect3DMaterial::GetMaterial", 1, pGetMaterial1, d3dm, lpMaterial); }
HRESULT WINAPI extGetMaterial2(void *d3dm, LPD3DMATERIAL lpMaterial)
{ return extGetMaterial("IDirect3DMaterial2::GetMaterial", 2, pGetMaterial2, d3dm, lpMaterial); }
HRESULT WINAPI extGetMaterial3(void *d3dm, LPD3DMATERIAL lpMaterial)
{ return extGetMaterial("IDirect3DMaterial3::GetMaterial", 3, pGetMaterial3, d3dm, lpMaterial); }

HRESULT WINAPI extGetMaterial7(void *d3dd, LPD3DMATERIAL7 lpMaterial)
{
	HRESULT res;
	ApiName("IDirect3DMaterial7::GetMaterial");

	res=(*pGetMaterial7)(d3dd, lpMaterial);
	OutTraceD3D("%s: d3dd=%#x material=%#x res=%#x\n", ApiRef, d3dd, lpMaterial, res);
	if(lpMaterial && IsDebugD3D && (res==DD_OK)){
		OutTraceD3D("%s: diffuse=(%f,%f,%f,%f) ambient=(%f,%f,%f,%f) specular=(%f,%f,%f,%f) emissive=(%f,%f,%f,%f) power=%f\n", 
			ApiRef,
			lpMaterial->diffuse.a, lpMaterial->diffuse.r, lpMaterial->diffuse.g, lpMaterial->diffuse.b,
			lpMaterial->ambient.a, lpMaterial->ambient.r, lpMaterial->ambient.g, lpMaterial->ambient.b,
			lpMaterial->specular.a, lpMaterial->specular.r, lpMaterial->specular.g, lpMaterial->specular.b,
			lpMaterial->emissive.a, lpMaterial->emissive.r, lpMaterial->emissive.g, lpMaterial->emissive.b,
			lpMaterial->power
		);
	}
	return res;
}
#endif // TRACEMATERIAL

HRESULT WINAPI extSwapTextureHandles(void *d3dd, LPDIRECT3DTEXTURE t1, LPDIRECT3DTEXTURE t2)
{
	HRESULT res;
	ApiName("IDirect3DDevice::SwapTextureHandles");

	OutTraceD3D("%s: d3dd=%#x t1=%#x t2=%#x\n", ApiRef, d3dd, t1, t2);
	if (dxw.dwFlags4 & NOTEXTURES) return DD_OK;
	
	res=(*pSwapTextureHandles)(d3dd, t1, t2);
	_if(res) OutTraceD3D("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extSwapTextureHandles2(void *d3dd, LPDIRECT3DTEXTURE2 t1, LPDIRECT3DTEXTURE2 t2)
{
	HRESULT res;
	ApiName("IDirect3DDevice2::SwapTextureHandles");

	OutTraceD3D("%s: d3dd=%#x t1=%#x t2=%#x\n", ApiRef, d3dd, t1, t2);
	if (dxw.dwFlags4 & NOTEXTURES) return DD_OK;
	
	res=(*pSwapTextureHandles2)(d3dd, t1, t2);
	_if(res) OutTraceD3D("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

#ifdef TRACETEXTURE
HRESULT WINAPI extTexInitialize(void *t, LPDIRECT3DDEVICE lpd3dd, LPDIRECTDRAWSURFACE lpdds)
{
	HRESULT ret;
	ApiName("IDirect3DTexture::Initialize");
	OutTraceD3D("%s: lpt=%#x lpd3dd=%#x lpdds=%#x\n", ApiRef, t, lpd3dd, lpdds);
	ret = (*pTInitialize)(t, lpd3dd, lpdds);
	if(ret) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, ret, ExplainDDError(ret));
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return ret;
}

static HRESULT WINAPI extTexGetHandle(ApiArg, int version, TexGetHandle_Type pTGetHandle, void *t, LPDIRECT3DDEVICE lpd3dd, LPD3DTEXTUREHANDLE lpth)
{
	HRESULT ret;
	OutTraceD3D("%s lpt=%#x lpd3dd=%#x lpth=%#x\n", ApiRef, t, lpd3dd, lpth);
	ret = (*pTGetHandle)(t, lpd3dd, lpth);
	_if(ret) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, ret, ExplainDDError(ret));
	}
	else {
		OutTraceD3D("%s: OK lpt=%#x lpd3dd=%#x th=%#x\n", ApiRef, t, lpd3dd, *lpth);
	}
	return ret;
}

HRESULT WINAPI extTexGetHandle1(void *t, LPDIRECT3DDEVICE lpd3dd, LPD3DTEXTUREHANDLE lpth)
{ return extTexGetHandle("IDirect3DTexture::GetHandle", 1, pTGetHandle1, t, lpd3dd, lpth); }
HRESULT WINAPI extTexGetHandle2(void *t, LPDIRECT3DDEVICE2 lpd3dd, LPD3DTEXTUREHANDLE lpth)
{ return extTexGetHandle("IDirect3DTexture2::GetHandle", 2, pTGetHandle2, t, (LPDIRECT3DDEVICE)lpd3dd, lpth); }


static HRESULT WINAPI extTexPaletteChanged(ApiArg, int version, TexPaletteChanged_Type pTPaletteChanged, void *t, DWORD dw1, DWORD dw2)
{
	HRESULT ret;
	OutTraceD3D("%s: lpt=%#x dw1=%#x dw2=%#x\n", ApiRef, t, dw1, dw2);
	ret = (*pTPaletteChanged)(t, dw1, dw2);
	_if(ret) OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, ret, ExplainDDError(ret));
	return ret;
}

HRESULT WINAPI extTexPaletteChanged1(void *t, DWORD dw1, DWORD dw2)
{ return extTexPaletteChanged("IDirect3DTexture::PaletteChanged", 1, pTPaletteChanged1, t, dw1, dw2); }
HRESULT WINAPI extTexPaletteChanged2(void *t, DWORD dw1, DWORD dw2)
{ return extTexPaletteChanged("IDirect3DTexture2::PaletteChanged", 2, pTPaletteChanged2, t, dw1, dw2); }

HRESULT WINAPI extTexLoad(ApiArg, int version, TexLoad_Type pTLoad, void *t, LPDIRECT3DTEXTURE lpt)
{
	HRESULT ret;
	OutTraceD3D("%s: lpt=%#x lpd3dt=%#x\n", ApiRef, t, lpt);
	ret = (*pTLoad)(t, lpt);
	if(ret) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, ret, ExplainDDError(ret));
	}
	else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return ret;
}

HRESULT WINAPI extTexLoad1(void *t, LPDIRECT3DTEXTURE lpt)
{ return extTexLoad("IDirect3DTexture::Load", 1, pTLoad1, t, lpt); }
HRESULT WINAPI extTexLoad2(void *t, LPDIRECT3DTEXTURE lpt)
{ return extTexLoad("IDirect3DTexture2::Load", 2, pTLoad2, t, lpt); }

HRESULT WINAPI extTexUnload(void *t)
{
	HRESULT ret;
	ApiName("IDirect3DTexture::Unload");

	OutTraceD3D("%s: lpt=%#x\n", ApiRef, t);
	ret = (*pTUnload)(t);
	_if(ret) OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, ret, ExplainDDError(ret));
	return ret;
}
#endif // TRACETEXTURE

typedef struct {
	VOID *cb;
	LPVOID arg;
} CallbackPixFmtArg;

HRESULT WINAPI extZBufferProxy(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
	HRESULT res;
#ifndef DXW_NOTRACES
	char sPFormat[1024];
	OutTraceD3D("EnumZBufferFormats: CALLBACK context=%#x %s \n", ((CallbackPixFmtArg *)lpContext)->arg, 
		ExplainPixelFormat(lpDDPixFmt, sPFormat, 1024));
#endif // DXW_NOTRACES
	res = (*(LPD3DENUMPIXELFORMATSCALLBACK)(((CallbackPixFmtArg *)lpContext)->cb))(lpDDPixFmt, ((CallbackPixFmtArg *)lpContext)->arg);
	OutTraceD3D("EnumZBufferFormats: CALLBACK ret=%#x\n", res);
	return res;
}

#define MAXPFTABLESIZE 50
#define MAXTRIMMEDENTRIES 3

typedef struct {
	int nEntries;
//	LPDDPIXELFORMAT lpPixelFormatEntries;
	DDPIXELFORMAT lpPixelFormatEntries[MAXPFTABLESIZE];
} PixelFormatTable_Type;

HRESULT WINAPI FillPixelFormatTable(LPDDPIXELFORMAT lpDDPixFmt, LPVOID Arg)
{
	PixelFormatTable_Type *lpPixelFormatTable = (PixelFormatTable_Type *)Arg;
#ifndef DXW_NOTRACES
	char sPFormat[1024];
	OutTraceD3D("EnumZBufferFormats: FILL CALLBACK entry=%d %s\n", lpPixelFormatTable->nEntries, ExplainPixelFormat(lpDDPixFmt, sPFormat, 1024));
#endif // DXW_NOTRACES
	if(lpPixelFormatTable->nEntries >= MAXPFTABLESIZE) return FALSE;
	memcpy((LPVOID)&(lpPixelFormatTable->lpPixelFormatEntries[lpPixelFormatTable->nEntries]), (LPVOID)lpDDPixFmt, sizeof(DDPIXELFORMAT));
	lpPixelFormatTable->nEntries ++;
	//lpPixelFormatTable->lpPixelFormatEntries = (LPDDPIXELFORMAT)realloc((LPVOID)(lpPixelFormatTable->lpPixelFormatEntries), lpPixelFormatTable->nEntries * sizeof(DDPIXELFORMAT));
	//OutTraceD3D("lp=%#x err=%s\n", lpPixelFormatTable->lpPixelFormatEntries, GetLastError());
	return TRUE;
}

static HRESULT WINAPI extEnumZBufferFormats(ApiArg, int d3dversion, EnumZBufferFormats_Type pEnumZBufferFormats, void *lpd3d, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
	HRESULT ret;
	OutTraceD3D("%s: d3d=%#x clsid=%#x context=%#x\n", ApiRef, lpd3d, riidDevice.Data1, lpContext);

	if(dxw.dwFlags8 & TRIMTEXTUREFORMATS){
		int iIndex;
		int iEnumerated;
		PixelFormatTable_Type PixelFormatTable;
		PixelFormatTable.nEntries = 0;
		//PixelFormatTable.lpPixelFormatEntries = (LPDDPIXELFORMAT)malloc(sizeof(DDPIXELFORMAT));
		ret = (*pEnumZBufferFormats)(lpd3d, riidDevice, (LPD3DENUMPIXELFORMATSCALLBACK)FillPixelFormatTable, (LPVOID)&PixelFormatTable);
		OutTraceD3D("%s: collected entries=%d\n", ApiRef, PixelFormatTable.nEntries);
		// bubble sorting;
		while(TRUE){
			BOOL bSorted = FALSE;
			for(iIndex=0; iIndex<PixelFormatTable.nEntries-1; iIndex++){
				if(PixelFormatTable.lpPixelFormatEntries[iIndex].dwRGBBitCount > PixelFormatTable.lpPixelFormatEntries[iIndex+1].dwRGBBitCount){
					DDPIXELFORMAT tmp;
					tmp = PixelFormatTable.lpPixelFormatEntries[iIndex];
					PixelFormatTable.lpPixelFormatEntries[iIndex] = PixelFormatTable.lpPixelFormatEntries[iIndex+1];
					PixelFormatTable.lpPixelFormatEntries[iIndex+1] = tmp;
					bSorted = TRUE;
				}
			}
			if(!bSorted) break;
		}
		for(iIndex=0, iEnumerated=0; (iIndex < PixelFormatTable.nEntries) && (iEnumerated < MAXTRIMMEDENTRIES); iIndex++){
			if(PixelFormatTable.lpPixelFormatEntries[iIndex].dwRGBBitCount >= 32) break;
			if((dxw.dwFlags7 & CLEARTEXTUREFOURCC) && (PixelFormatTable.lpPixelFormatEntries[iIndex].dwFlags & DDPF_FOURCC)) continue;
			ret = (*lpEnumCallback)(&(PixelFormatTable.lpPixelFormatEntries[iIndex]), lpContext);
#ifndef DXW_NOTRACES
			char sPFormat[1024];
			OutTraceD3D("%s: CALLBACK entry=%d ret=%#x %s\n", ApiRef, iIndex, ret, 
				ExplainPixelFormat(&PixelFormatTable.lpPixelFormatEntries[iIndex], sPFormat, 1024));
#endif // DXW_NOTRACES
			if(!ret) break;
			iEnumerated++;
		}
		//free((LPVOID)(PixelFormatTable.lpPixelFormatEntries));
		ret = DD_OK;
	}
	else {
		CallbackPixFmtArg Arg;
		Arg.cb= lpEnumCallback; // v2.04.71.fx1: fix (deleted &) - tested with "Choanikimura"
		Arg.arg=lpContext;
		ret = (*pEnumZBufferFormats)(lpd3d, riidDevice, (LPD3DENUMPIXELFORMATSCALLBACK)extZBufferProxy, (LPVOID)&Arg);
	}
	_if(ret) OutErrorD3D("%s: res=%#x(%s)\n", ApiRef, ret, ExplainDDError(ret));
	return ret;
}

HRESULT WINAPI extEnumZBufferFormats3(void *lpd3d, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{ return extEnumZBufferFormats("IDirect3D3::EnumZBufferFormats", 3, pEnumZBufferFormats3, lpd3d, riidDevice, lpEnumCallback, lpContext); }
HRESULT WINAPI extEnumZBufferFormats7(void *lpd3d, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{ return extEnumZBufferFormats("IDirect3D7::EnumZBufferFormats", 7, pEnumZBufferFormats7, lpd3d, riidDevice, lpEnumCallback, lpContext); }

// Beware: using service surfaces with DDSCAPS_SYSTEMMEMORY capability may lead to crashes in D3D operations
// like Vievport::Clear() in "Forsaken" set in emulation AERO-friendly mode. To avoid the problem, you can 
// suppress the offending cap by use of the NOSYSMEMPRIMARY or NOSYSMEMBACKBUF flags

#ifdef TRACEALL
HRESULT WINAPI extViewportClear(void *lpd3dvp, DWORD dwNRect, LPD3DRECT lpRect, DWORD dwFlags)
{
	HRESULT ret;
	ApiName("IDirect3D::ViewportClear");

#ifndef DXW_NOTRACES
	if(IsTraceD3D){
		OutTrace("%s: lpd3dvp=%#x nrect=%#x flags=%#x\n", ApiRef, lpd3dvp, dwNRect, dwFlags);
		if(IsDebugD3D){
			for(DWORD i=0; i<dwNRect; i++){
				OutTrace("> rect[%d]=(%d,%d)-(%d,%d)\n", 
					i, lpRect[i].x1, lpRect[i].y1, lpRect[i].x2, lpRect[i].y2);
			}
		}
	}
#endif // DXW_NOTRACES

	// proxying the call ....
	ret = (*pViewportClear)(lpd3dvp, dwNRect, lpRect, dwFlags);

	OutTraceD3D("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}
#endif // TRACEALL

static HRESULT CALLBACK lpTextureTrimmer12(LPDDSURFACEDESC lpDdsd, LPVOID lpContext)
{
	HRESULT res;
	BOOL bSkip = FALSE;
	if((dxw.dwFlags7 & CLEARTEXTUREFOURCC) && (lpDdsd->ddpfPixelFormat.dwFlags & DDPF_FOURCC)) bSkip = TRUE;
	if((dxw.dwFlags18 & FILTERRGBTEXTURES) && !(lpDdsd->ddpfPixelFormat.dwFlags & DDPF_RGB)) bSkip = TRUE;
#ifndef DXW_NOTRACES
	char sPFormat[1024];
	OutTraceD3D("EnumTextureFormats: %s context=%#x %s \n", 
		bSkip ? "SKIP" : "CALLBACK", 
		lpContext, ExplainPixelFormat(&(lpDdsd->ddpfPixelFormat), sPFormat, 1024));
#endif // DXW_NOTRACES
	if(bSkip)
		res = TRUE; // v2.05.44 fix
	else
		res = (*(LPD3DENUMTEXTUREFORMATSCALLBACK)(((CallbackPixFmtArg *)lpContext)->cb))(lpDdsd, ((CallbackPixFmtArg *)lpContext)->arg);
	return res;
}

static HRESULT CALLBACK lpTextureTrimmer37(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
	HRESULT res;
	BOOL bSkip = FALSE;
	if((dxw.dwFlags7 & CLEARTEXTUREFOURCC) && (lpDDPixFmt->dwFlags & DDPF_FOURCC)) bSkip = TRUE;
	if((dxw.dwFlags18 & FILTERRGBTEXTURES) && !(lpDDPixFmt->dwFlags & DDPF_RGB)) bSkip = TRUE;
#ifndef DXW_NOTRACES
	char sPFormat[1024];
	OutTraceD3D("EnumTextureFormats: %s context=%#x %s \n", 
		bSkip ? "SKIP" : "CALLBACK", 
		lpContext, ExplainPixelFormat(lpDDPixFmt, sPFormat, 1024));
#endif // DXW_NOTRACES
	if(bSkip)
		res = TRUE; // v2.05.44 fix
	else
		res = (*(LPD3DENUMPIXELFORMATSCALLBACK)(((CallbackPixFmtArg *)lpContext)->cb))(lpDDPixFmt, ((CallbackPixFmtArg *)lpContext)->arg);
	return res;
}

HRESULT WINAPI extEnumTextureFormats12(ApiArg, int d3dversion, EnumTextureFormats12_Type pEnumTextureFormats, void *lpd3dd, LPD3DENUMTEXTUREFORMATSCALLBACK lptfcallback, LPVOID arg)
{
	HRESULT res;
	OutTraceD3D("%s: lpd3dd=%#x cb=%#x arg=%#x\n", ApiRef, lpd3dd, lptfcallback, arg);

	if(IsDebugD3D || (dxw.dwFlags7 & CLEARTEXTUREFOURCC) || (dxw.dwFlags18 & FILTERRGBTEXTURES)){
		CallbackPixFmtArg Arg;
		Arg.cb= (LPD3DENUMTEXTUREFORMATSCALLBACK)lptfcallback; // v2.04.77 fixed
		Arg.arg=arg;
		res = (*pEnumTextureFormats)(lpd3dd, lpTextureTrimmer12, (LPVOID)&Arg);
	}
	else{
		res = (*pEnumTextureFormats)(lpd3dd, lptfcallback, arg);
	}
	_if(res) OutTraceD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extEnumTextureFormats37(ApiArg, int d3dversion, EnumTextureFormats37_Type pEnumTextureFormats, void *lpd3dd, LPD3DENUMPIXELFORMATSCALLBACK lptfcallback, LPVOID arg)
{
	HRESULT res;
	OutTraceD3D("%s: lpd3dd=%#x cb=%#x arg=%#x\n", ApiRef, lpd3dd, lptfcallback, arg);

	if(IsDebugD3D || (dxw.dwFlags7 & CLEARTEXTUREFOURCC) || (dxw.dwFlags18 & FILTERRGBTEXTURES)){
		CallbackPixFmtArg Arg;
		Arg.cb= (LPD3DENUMPIXELFORMATSCALLBACK)lptfcallback; // v2.04.77 fixed
		Arg.arg=arg;
		res = (*pEnumTextureFormats)(lpd3dd, lpTextureTrimmer37, (LPVOID)&Arg);
	}
	else{
		res = (*pEnumTextureFormats)(lpd3dd, lptfcallback, arg);
	}
	_if(res) OutTraceD3D("%s: res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extEnumTextureFormats1(void *lpd3dd, LPD3DENUMTEXTUREFORMATSCALLBACK lptfcallback, LPVOID arg)
{ return extEnumTextureFormats12("IDirect3DDevice::EnumTextureFormats", 1, pEnumTextureFormats1, lpd3dd, lptfcallback, arg); }
HRESULT WINAPI extEnumTextureFormats2(void *lpd3dd, LPD3DENUMTEXTUREFORMATSCALLBACK lptfcallback, LPVOID arg)
{ return extEnumTextureFormats12("IDirect3DDevice2::EnumTextureFormats", 2, pEnumTextureFormats2, lpd3dd, lptfcallback, arg); }
HRESULT WINAPI extEnumTextureFormats3(void *lpd3dd, LPD3DENUMPIXELFORMATSCALLBACK lptfcallback, LPVOID arg)
{ return extEnumTextureFormats37("IDirect3DDevice3::EnumTextureFormats", 3, pEnumTextureFormats3, lpd3dd, lptfcallback, arg); }
HRESULT WINAPI extEnumTextureFormats7(void *lpd3dd, LPD3DENUMPIXELFORMATSCALLBACK lptfcallback, LPVOID arg)
{ return extEnumTextureFormats37("IDirect3DDevice7::EnumTextureFormats", 7, pEnumTextureFormats7, lpd3dd, lptfcallback, arg); }


HRESULT WINAPI extSetLight7(void *lpd3dd, DWORD Index, D3DLIGHT7 *light)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::SetLight");
	extern void AdjustLight(D3DCOLORVALUE *, BYTE *);

#ifndef DXW_NOTRACES
	OutTraceD3D("%s: d3d=%#x index=%d\n", ApiRef, lpd3dd, Index);
	if(IsDebugD3D){
		OutTrace("> diffuse={%f,%f,%f,%f}\n", light->dcvDiffuse.r, light->dcvDiffuse.g, light->dcvDiffuse.b, light->dcvDiffuse.a);
		OutTrace("> specular={%f,%f,%f,%f}\n", light->dcvSpecular.r, light->dcvSpecular.g, light->dcvSpecular.b, light->dcvSpecular.a);
		OutTrace("> ambient={%f,%f,%f,%f}\n", light->dcvAmbient.r, light->dcvAmbient.g, light->dcvAmbient.b, light->dcvAmbient.a);
		OutTrace("> attenuation={%f,%f,%f}\n", light->dvAttenuation0, light->dvAttenuation1, light->dvAttenuation2);
	}
#endif // DXW_NOTRACES

	if(GetHookInfo()->GammaControl){
		BYTE *Gamma;
		D3DLIGHT7 newlight = *light;
		Gamma = GetHookInfo()->GammaRamp;
		AdjustLight(&newlight.dcvDiffuse, Gamma);
		AdjustLight(&newlight.dcvSpecular, Gamma);
		AdjustLight(&newlight.dcvAmbient, Gamma);
		res = (*pSetLight7)(lpd3dd, Index, &newlight);
	}
	else {
		res = (*pSetLight7)(lpd3dd, Index, light);
	}

	_if(res) OutErrorD3D("%s: ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

#define D3DSETSTATUS_STATUS     0x00000001L
#define D3DSETSTATUS_EXTENTS        0x00000002L
#define D3DSETSTATUS_ALL    (D3DSETSTATUS_STATUS | D3DSETSTATUS_EXTENTS)

HRESULT WINAPI extSetExecuteData(void *lpeb, LPD3DEXECUTEDATA lped)
{
	HRESULT res;
	ApiName("IDirect3DExecuteBuffer::SetExecuteData");

	OutTraceD3D("%s: lpeb=%#x lped=%#x\n", ApiRef, lpeb, lped);
	OutDebugD3D("%s: size=%d vertexoffset=%d vertexcount=%d instroffset=%d instrlen=%d hvertexoffset=%d "
		"d3dstatus={flags=%#x status=%#x(%s) extent=%#x}\n",
		ApiRef, 
		lped->dwSize, lped->dwVertexOffset, lped->dwVertexCount, 
		lped->dwInstructionOffset, lped->dwInstructionLength, lped->dwHVertexOffset, 
		lped->dsStatus.dwFlags, 
		lped->dsStatus.dwStatus, lped->dsStatus.dwStatus == D3DSETSTATUS_STATUS ? "STATUS" : (lped->dsStatus.dwStatus == D3DSETSTATUS_EXTENTS ? "EXTENTS" : "??"), 
		lped->dsStatus.drExtent);

	res = (*pSetExecuteData)(lpeb, lped);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} 

	//if(dxw.dwFlags2 & WIREFRAME) dxwForceState((LPDIRECT3DEXECUTEBUFFER)lpeb, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);
	return res;
}

HRESULT WINAPI extGetExecuteData(void *lpeb, LPD3DEXECUTEDATA lped)
{
	HRESULT res;
	ApiName("IDirect3DExecuteBuffer::GetExecuteData");

	OutTraceD3D("%s: lpeb=%#x lped=%#x\n", ApiRef, lpeb, lped);
	res = (*pGetExecuteData)(lpeb, lped);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	} 

	OutDebugD3D("%s: size=%d vertexoffset=%d vertexcount=%d instroffset=%d instrlen=%d hvertexoffset=%d "
		"d3dstatus={flags=%#x status=%#x(%s) extent=%#x}\n",
		ApiRef, 
		lped->dwSize, lped->dwVertexOffset, lped->dwVertexCount, 
		lped->dwInstructionOffset, lped->dwInstructionLength, lped->dwHVertexOffset, 
		lped->dsStatus.dwFlags, 
		lped->dsStatus.dwStatus, lped->dsStatus.dwStatus == D3DSETSTATUS_STATUS ? "STATUS" : (lped->dsStatus.dwStatus == D3DSETSTATUS_EXTENTS ? "EXTENTS" : "??"), 
		lped->dsStatus.drExtent);

	return res;
}

HRESULT WINAPI extEBLock(void *lpeb, LPD3DEXECUTEBUFFERDESC lpebd)
{
	HRESULT res;
	ApiName("IDirect3DExecuteBuffer::Lock");

	OutTraceD3D("%s: lpeb=%#x ebdesc={size=%d bufsize=%d caps=%#x(%s) flags=%#x(%s) data=%#x}\n", 
		ApiRef,
		lpeb, lpebd->dwSize, lpebd->dwBufferSize,
		lpebd->dwCaps, sExecuteBufferCaps(lpebd->dwCaps),
		lpebd->dwFlags, sExecuteBufferFlags(lpebd->dwFlags),
		lpebd->lpData);

	res = (*pEBLock)(lpeb, lpebd);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	} 

	OutTraceD3D("%s: OK lpeb=%#x ebdesc={size=%d bufsize=%d caps=%#x(%s) flags=%#x(%s) data=%#x}\n", 
		ApiRef,
		lpeb, lpebd->dwSize, lpebd->dwBufferSize,
		lpebd->dwCaps, sExecuteBufferCaps(lpebd->dwCaps),
		lpebd->dwFlags, sExecuteBufferFlags(lpebd->dwFlags),
		lpebd->lpData);

	return res;
}

HRESULT WINAPI extEBUnlock(void *lpeb)
{
	HRESULT res;
	ApiName("IDirect3DExecuteBuffer::Unlock");

	OutTraceD3D("%s: lpeb=%#x\n", ApiRef, lpeb);

	res = (*pEBUnlock)(lpeb);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} else {
		OutTraceD3D("%s: OK\n", ApiRef);
	}
	return res;
}

void dxwTrimPoints(LPDIRECT3DEXECUTEBUFFER lpeb)
{
	ApiName("dxwTrimPoints");

	OutTraceD3D("%s: lpeb=%#x\n", ApiRef, lpeb);

	D3DEXECUTEBUFFERDESC ebd;
	D3DEXECUTEDATA ed;

	ed.dwSize = sizeof(D3DEXECUTEDATA);
	if((*pGetExecuteData)(lpeb, &ed)) return;
	ebd.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
	if((*pEBLock)(lpeb, &ebd)) return; 

	LPD3DINSTRUCTION lpInst;
	int len;
	lpInst = (LPD3DINSTRUCTION)((LPBYTE)ebd.lpData + ed.dwInstructionOffset);
	len = (int)ed.dwInstructionLength;
	for(DWORD i=0; len>0; i++){
		if((lpInst->bOpcode < D3DOP_POINT) || (lpInst->bOpcode > D3DOP_SETSTATUS)) {
			OutTraceD3D("> Instr.[%04.4d]: invalid op=%d\n", i, lpInst->bOpcode);
			break;
		}
		switch(lpInst->bOpcode){
			case D3DOP_POINT:
				if(lpInst->wCount > 1){
					OutTrace("%s: critical situation D3DOP_POINT wCount=%d\n", ApiRef, lpInst->wCount);
					int toFill = lpInst->wCount - 1;
					lpInst->wCount = 1;
					LPD3DINSTRUCTION lpNext = lpInst + 2; // sizeof(D3DINSTRUCTION) == sizeof(D3DPOINT)
					for(int i=0; i<toFill; i++){
						lpNext->bOpcode = D3DOP_STATERENDER;
						lpNext->wCount = 0;
						lpNext->bSize = sizeof(D3DSTATE);
						lpNext++;
					}
				};
				break;
		}
		len -= sizeof(D3DINSTRUCTION) + (lpInst->bSize * lpInst->wCount);
		if(lpInst->bOpcode == D3DOP_EXIT) break;
		lpInst = (LPD3DINSTRUCTION)((LPBYTE)lpInst + sizeof(D3DINSTRUCTION) + (lpInst->bSize * lpInst->wCount));
	}
	if(len) OutTrace("%s: residual len=%d\n", ApiRef, len);
	(*pEBUnlock)(lpeb);
}

HRESULT WINAPI extExecute(void *lpd3d, LPDIRECT3DEXECUTEBUFFER lpeb, LPDIRECT3DVIEWPORT vp, DWORD flags)
{
	HRESULT ret;
	ApiName("IDirect3DDevice::Execute");

	OutTraceD3D("%s: lpd3d=%#x eb=%#x vp=%#x flags=%#x\n", ApiRef, lpd3d, lpeb, vp, flags);

#ifdef DUMPEXECUTEBUFFER
#ifndef DXW_NOTRACES
	DumpEBData(lpeb);
#endif // DXW_NOTRACES
#endif // DUMPEXECUTEBUFFER

	if(dxw.dwFlags17 & PATCHEXECUTEBUFFER) InsertPatchingExecuteBuffer(lpd3d, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL); // trytry ....
	if(dxw.dwFlags2 & WIREFRAME) InsertPatchingExecuteBuffer(lpd3d, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME); // trytry ....
	if(dxw.dwFlags17 & FORCEZBUFFERON) InsertPatchingExecuteBuffer(lpd3d, D3DRENDERSTATE_ZENABLE, TRUE);
	if(dxw.dwFlags17 & FORCEZBUFFEROFF) InsertPatchingExecuteBuffer(lpd3d, D3DRENDERSTATE_ZENABLE, FALSE);

	if(dxw.dwFlags4 & NOTEXTURES) dxwForceState(lpeb, D3DRENDERSTATE_TEXTUREHANDLE, NULL);
	if(dxw.dwDFlags & ZBUFFERALWAYS) dxwForceState(lpeb, D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
	if(dxw.dwFlags17 & FORCEZBUFFERON) dxwForceState(lpeb, D3DRENDERSTATE_ZENABLE, TRUE);
	if(dxw.dwFlags17 & FORCEZBUFFEROFF) dxwForceState(lpeb, D3DRENDERSTATE_ZENABLE, FALSE);
	//if(dxw.dwFlags2 & WIREFRAME) dxwFakeWireFrame(lpeb);
	if(dxw.dwFlags2 & WIREFRAME) dxwForceState(lpeb, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);
	if(dxw.dwFlags17 & PATCHEXECUTEBUFFER) dxwForceState(lpeb, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL); 
	if(dxw.dwFlags15 & CULLMODENONE) {
		if(!dxwForceState(lpeb, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE)){
			InsertPatchingExecuteBuffer(lpd3d, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
		}
	}
	if(dxw.dwFlags19 & TRIMD3DPOINTS) dxwTrimPoints(lpeb);

	ret=(*pExecute)(lpd3d, lpeb, vp, flags);
	_if (ret) OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, ret, ExplainDDError(ret));
	return DD_OK;
}

HRESULT WINAPI extCreateExecuteBuffer(void *lpd3dd, LPD3DEXECUTEBUFFERDESC lpebd, LPDIRECT3DEXECUTEBUFFER *lplpeb, IUnknown *unk)
{
	HRESULT res;
	ApiName("IDirect3DDevice::CreateExecuteBuffer");

	OutTraceD3D("%s: d3d=%#x ebdesc={size=%d bufsize=%d caps=%#x(%s) flags=%#x(%s)}\n", 
		ApiRef,
		lpd3dd, lpebd->dwSize, lpebd->dwBufferSize,
		lpebd->dwCaps, sExecuteBufferCaps(lpebd->dwCaps),
		lpebd->dwFlags, sExecuteBufferFlags(lpebd->dwFlags));

	if(dxw.dwFlags10 & FORCEEBCAPS) {
		lpebd->dwFlags |= D3DDEB_CAPS;
		lpebd->dwCaps = ForceExecuteBufferCaps();
	}

	res = (*pCreateExecuteBuffer)(lpd3dd, lpebd, lplpeb, unk);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} else {
		OutTraceD3D("%s: lpeb=%#x\n", ApiRef, *lplpeb);
		HookExecuteBuffer(lplpeb);
	}
	return res;
}

HRESULT WINAPI extSetViewport(ApiArg, int dxversion, SetViewport_Type pSetViewport, void *lpvp, LPD3DVIEWPORT vpd)
{
	HRESULT res;

	OutTraceD3D("%s: viewport=%#x viewportd=%#x size=%d pos=(%d,%d) dim=(%dx%d) scale=(%fx%f) maxXYZ=(%f,%f,%f) minZ=%f\n", 
		ApiRef, lpvp, vpd, vpd->dwSize, vpd->dwX, vpd->dwY, vpd->dwWidth, vpd->dwHeight, vpd->dvScaleX, vpd->dvScaleY, 
		vpd->dvMaxX, vpd->dvMaxY, vpd->dvMaxZ, vpd->dvMinZ);

	// v2.03.48: scaled dvScaleX/Y fields. Fixes "Dark Vengeance" viewport size when using D3D interface.
	// no.... see Forsaken
	// no good (useless) also for "Spearhead"
	//if(dxw.Windowize){
	//	dxw.MapClient(&vpd->dvScaleX, &vpd->dvScaleY);
	//	OutTraceDW("SetViewport: FIXED scale=(%fx%f)\n", vpd->dvScaleX, vpd->dvScaleY);
	//	int w = (int) vpd->dwWidth;
	//	int h = (int) vpd->dwHeight;
	//	dxw.MapClient(&w, &h);
	//	vpd->dwWidth = (DWORD) w;
	//	vpd->dwHeight = (DWORD) h;
	//	OutTraceDW("SetViewport: FIXED scale=(%dx%d)\n", vpd->dwWidth, vpd->dwHeight);
	//}

	res=(*pSetViewport)(lpvp, vpd);
	if(res) OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	else OutTraceD3D("%s: OK\n", ApiRef);
	return res;
}

HRESULT WINAPI extSetViewport1(void *lpvp, LPD3DVIEWPORT vpd)
{ return extSetViewport("IDirect3DViewport::SetViewport", 1, pSetViewport1, lpvp, vpd); }
HRESULT WINAPI extSetViewport2(void *lpvp, LPD3DVIEWPORT vpd)
{ return extSetViewport("IDirect3DViewport2::SetViewport", 2, pSetViewport2, lpvp, vpd); }
HRESULT WINAPI extSetViewport3(void *lpvp, LPD3DVIEWPORT vpd)
{ return extSetViewport("IDirect3DViewPort3::SetViewport", 3, pSetViewport3, lpvp, vpd); }

HRESULT WINAPI extGetViewport(ApiArg, int d3dversion, GetViewport_Type pGetViewport, void *lpvp, LPD3DVIEWPORT vpd)
{
	HRESULT res;

	OutTraceD3D("%s: viewport=%#x viewportd=%#x\n", ApiRef, lpvp, vpd);
	res=(*pGetViewport)(lpvp, vpd);
	// v2.03.48: should the dvScaleX/Y fields be unscaled? 
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
	}
	else {
		OutTraceD3D("%s: OK size=%d pos=(%d,%d) dim=(%dx%d) scale=(%fx%f) maxXYZ=(%f,%f,%f) minZ=%f\n",
			ApiRef, 
			vpd->dwSize, vpd->dwX, vpd->dwY, vpd->dwWidth, vpd->dwHeight, vpd->dvScaleX, vpd->dvScaleY, 
			vpd->dvMaxX, vpd->dvMaxY, vpd->dvMaxZ, vpd->dvMinZ);
	}
	return res;
}

HRESULT WINAPI extGetViewport1(void *lpvp, LPD3DVIEWPORT vpd)
{ return extGetViewport("IDirect3DViewport::GetViewport", 1, pGetViewport1, lpvp, vpd); }
HRESULT WINAPI extGetViewport2(void *lpvp, LPD3DVIEWPORT vpd)
{ return extGetViewport("IDirect3DViewport2::GetViewport", 2, pGetViewport2, lpvp, vpd); }
HRESULT WINAPI extGetViewport3(void *lpvp, LPD3DVIEWPORT vpd)
{ return extGetViewport("IDirect3DViewport3::GetViewport", 3, pGetViewport3, lpvp, vpd); }

#ifdef TRACEALL
HRESULT WINAPI extTransformVertices(ApiArg, int d3dver, TransformVertices_Type pTransformVertices, void *lpvp, DWORD dw, LPD3DTRANSFORMDATA lpd3dtd, DWORD dw2, LPDWORD lpdw)
{
	HRESULT res;

	OutTrace("%s: lpvp=%#x dw=%#x dw2=%#x\n", ApiRef, lpvp, dw, dw2);
	res = (*pTransformVertices)(lpvp, dw, lpd3dtd, dw2, lpdw);
	_if(res) OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

// IDirect3DViewport::TransformVertices
HRESULT WINAPI extTransformVertices1(void *lpvp, DWORD dw, LPD3DTRANSFORMDATA lpd3dtd, DWORD dw2, LPDWORD lpdw)
{ return extTransformVertices("IDirect3DViewport::TransformVertices", 1, pTransformVertices1, lpvp, dw, lpd3dtd, dw2, lpdw); }
HRESULT WINAPI extTransformVertices2(void *lpvp, DWORD dw, LPD3DTRANSFORMDATA lpd3dtd, DWORD dw2, LPDWORD lpdw)
{ return extTransformVertices("IDirect3DViewport2::TransformVertices", 2, pTransformVertices2, lpvp, dw, lpd3dtd, dw2, lpdw); }
HRESULT WINAPI extTransformVertices3(void *lpvp, DWORD dw, LPD3DTRANSFORMDATA lpd3dtd, DWORD dw2, LPDWORD lpdw)
{ return extTransformVertices("IDirect3DViewport3::TransformVertices", 3, pTransformVertices3, lpvp, dw, lpd3dtd, dw2, lpdw); }

HRESULT WINAPI extLightElements(ApiArg, int d3dver, LightElements_Type pLightElements, void *lpvp, DWORD dwCount, LPD3DLIGHTDATA lpd3dld)
{
	HRESULT res;

	OutTrace("%s(VP%d): lpvp=%#x count=%#x\n", ApiRef, d3dver, lpvp, dwCount);
	res = (*pLightElements)(lpvp, dwCount, lpd3dld);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}

HRESULT WINAPI extLightElements1(void *lpvp, DWORD dwCount, LPD3DLIGHTDATA lpd3dld)
{ return extLightElements("IDirect3DViewport::LightElements", 1, pLightElements1, lpvp, dwCount, lpd3dld); }
HRESULT WINAPI extLightElements2(void *lpvp, DWORD dwCount, LPD3DLIGHTDATA lpd3dld)
{ return extLightElements("IDirect3DViewport2::LightElements", 2, pLightElements2, lpvp, dwCount, lpd3dld); }
HRESULT WINAPI extLightElements3(void *lpvp, DWORD dwCount, LPD3DLIGHTDATA lpd3dld)
{ return extLightElements("IDirect3DViewport3::LightElements", 3, pLightElements3, lpvp, dwCount, lpd3dld); }

HRESULT WINAPI extSetBackground(ApiArg, int d3dver, SetBackground_Type pSetBackground, void *lpvp, D3DMATERIALHANDLE d3dmh)
{
	HRESULT res;

	OutTrace("%s: lpvp=%#x mhandle=%#x\n", ApiRef, lpvp, d3dmh);
	res = (*pSetBackground)(lpvp, d3dmh);
	_if(res) OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extSetBackground1(void *lpvp, D3DMATERIALHANDLE d3dmh)
{ return extSetBackground("IDirect3DViewport::SetBackground", 1, pSetBackground1, lpvp, d3dmh); }
HRESULT WINAPI extSetBackground2(void *lpvp, D3DMATERIALHANDLE d3dmh)
{ return extSetBackground("IDirect3DViewport2::SetBackground", 2, pSetBackground2, lpvp, d3dmh); }
HRESULT WINAPI extSetBackground3(void *lpvp, D3DMATERIALHANDLE d3dmh)
{ return extSetBackground("IDirect3DViewport3::SetBackground", 3, pSetBackground3, lpvp, d3dmh); }

HRESULT WINAPI extGetBackground(ApiArg, int d3dver, GetBackground_Type pGetBackground, void *lpvp, LPD3DMATERIALHANDLE lpd3dmh, LPBOOL lpb)
{
	HRESULT res;

	OutTrace("%s: lpvp=%#x\n", ApiRef, lpvp);
	res = (*pGetBackground)(lpvp, lpd3dmh, lpb);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	}
	OutTrace("%s: mh=%#x bool=%#x\n", ApiRef, *lpd3dmh, *lpb);
	return res;
}

HRESULT WINAPI extGetBackground1(void *lpvp, LPD3DMATERIALHANDLE lpd3dmh, LPBOOL lpb)
{ return extGetBackground("IDirect3DViewport::GetBackground", 1, pGetBackground1, lpvp, lpd3dmh, lpb); }
HRESULT WINAPI extGetBackground2(void *lpvp, LPD3DMATERIALHANDLE lpd3dmh, LPBOOL lpb)
{ return extGetBackground("IDirect3DViewport2::GetBackground", 2, pGetBackground2, lpvp, lpd3dmh, lpb); }
HRESULT WINAPI extGetBackground3(void *lpvp, LPD3DMATERIALHANDLE lpd3dmh, LPBOOL lpb)
{ return extGetBackground("IDirect3DViewport3::GetBackground", 3, pGetBackground3, lpvp, lpd3dmh, lpb); }

HRESULT WINAPI extSetBackgroundDepth(ApiArg, int d3dver, SetBackgroundDepth_Type pSetBackgroundDepth, void *lpvp, LPDIRECTDRAWSURFACE lpdds)
{
	HRESULT res;
	OutTrace("%s: lpvp=%#x lpdds=%#x\n", ApiRef, d3dver, lpvp, lpdds);
	res = (*pSetBackgroundDepth)(lpvp, lpdds);
	_if(res) OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extSetBackgroundDepth1(void *lpvp, LPDIRECTDRAWSURFACE lpdds)
{ return extSetBackgroundDepth("IDirect3DViewport::SetBackgroundDepth", 1, pSetBackgroundDepth1, lpvp, lpdds); }
HRESULT WINAPI extSetBackgroundDepth2(void *lpvp, LPDIRECTDRAWSURFACE lpdds)
{ return extSetBackgroundDepth("IDirect3DViewport2::SetBackgroundDepth", 2, pSetBackgroundDepth2, lpvp, lpdds); }
HRESULT WINAPI extSetBackgroundDepth3(void *lpvp, LPDIRECTDRAWSURFACE lpdds)
{ return extSetBackgroundDepth("IDirect3DViewport3::SetBackgroundDepth", 3, pSetBackgroundDepth3, lpvp, lpdds); }

HRESULT WINAPI extGetBackgroundDepth(ApiArg, int d3dver, GetBackgroundDepth_Type pGetBackgroundDepth, void *lpvp, LPDIRECTDRAWSURFACE *lplpdds, LPBOOL lpb)
{
	HRESULT res;

	OutTrace("%s: lpvp=%#x\n", ApiRef, lpvp);
	res = (*pGetBackgroundDepth)(lpvp, lplpdds, lpb);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	}
	OutTrace("%s: lpdds=%#x bool=%#x\n", ApiRef, *lplpdds, *lpb);
	return res;
}

HRESULT WINAPI extGetBackgroundDepth1(void *lpvp, LPDIRECTDRAWSURFACE *lplpdds, LPBOOL lpb)
{ return extGetBackgroundDepth("IDirect3DViewport::GetBackgroundDepth", 1, pGetBackgroundDepth1, lpvp, lplpdds, lpb); }
HRESULT WINAPI extGetBackgroundDepth2(void *lpvp, LPDIRECTDRAWSURFACE *lplpdds, LPBOOL lpb)
{ return extGetBackgroundDepth("IDirect3DViewport2::GetBackgroundDepth", 2, pGetBackgroundDepth2, lpvp, lplpdds, lpb); }
HRESULT WINAPI extGetBackgroundDepth3(void *lpvp, LPDIRECTDRAWSURFACE *lplpdds, LPBOOL lpb)
{ return extGetBackgroundDepth("IDirect3DViewport3::GetBackgroundDepth", 3, pGetBackgroundDepth3, lpvp, lplpdds, lpb); }

HRESULT WINAPI extClear(ApiArg, int d3dver, Clear_Type pClear, void *lpvp, DWORD dwNRect, LPD3DRECT lpRect, DWORD dwFlags)
{
	HRESULT res;

#ifndef DXW_NOTRACES
	if(IsTraceD3D){
		OutTrace("%s: lpvp=%#x nrect=%#x flags=%#x\n", ApiRef, lpvp, dwNRect, dwFlags);
		if(IsDebugD3D){
			for(DWORD i=0; i<dwNRect; i++){
				OutTrace("> rect[%d]=(%d,%d)-(%d,%d)\n", 
					i, lpRect[i].x1, lpRect[i].y1, lpRect[i].x2, lpRect[i].y2);
			}
		}
	}
#endif // DXW_NOTRACES
	res = (*pClear)(lpvp, dwNRect, lpRect, dwFlags);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}

HRESULT WINAPI extClear1(void *lpvp, DWORD dwNRect, LPD3DRECT lpRect, DWORD dwFlags)
{ return extClear("IDirect3DViewport::Clear", 1, pClear1, lpvp, dwNRect, lpRect, dwFlags); }
HRESULT WINAPI extClear2(void *lpvp, DWORD dwNRect, LPD3DRECT lpRect, DWORD dwFlags)
{ return extClear("IDirect3DViewport2::Clear", 2, pClear2, lpvp, dwNRect, lpRect, dwFlags); }
HRESULT WINAPI extClear3(void *lpvp, DWORD dwNRect, LPD3DRECT lpRect, DWORD dwFlags)
{ return extClear("IDirect3DViewport3::Clear", 3, pClear3, lpvp, dwNRect, lpRect, dwFlags); }

HRESULT WINAPI extAddLight(ApiArg, int d3dver, AddLight_Type pAddLight, void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{
	HRESULT res;

	OutTrace("%s: lpvp=%#x light=%#x\n", ApiRef, lpvp, lpd3dl);
	res = (*pAddLight)(lpvp, lpd3dl);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}

HRESULT WINAPI extAddLight1(void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{ return extAddLight("IDirect3DViewport::AddLight", 1, pAddLight1, lpvp, lpd3dl); }
HRESULT WINAPI extAddLight2(void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{ return extAddLight("IDirect3DViewport2::AddLight", 2, pAddLight2, lpvp, lpd3dl); }
HRESULT WINAPI extAddLight3(void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{ return extAddLight("IDirect3DViewport3::AddLight", 3, pAddLight3, lpvp, lpd3dl); }

HRESULT WINAPI extDeleteLight(ApiArg, int d3dver, DeleteLight_Type pDeleteLight, void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{
	HRESULT res;

	OutTrace("%s: lpvp=%#x light=%#x\n", ApiRef, lpvp, lpd3dl);
	res = (*pDeleteLight)(lpvp, lpd3dl);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}

HRESULT WINAPI extDeleteLight1(void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{ return extDeleteLight("IDirect3DViewport::DeleteLight", 1, pDeleteLight1, lpvp, lpd3dl); }
HRESULT WINAPI extDeleteLight2(void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{ return extDeleteLight("IDirect3DViewport2::DeleteLight", 2, pDeleteLight2, lpvp, lpd3dl); }
HRESULT WINAPI extDeleteLight3(void *lpvp, LPDIRECT3DLIGHT lpd3dl)
{ return extDeleteLight("IDirect3DViewport3::DeleteLight", 3, pDeleteLight3, lpvp, lpd3dl); }

HRESULT WINAPI extNextLight(ApiArg, int d3dver, NextLight_Type pNextLight, void *lpvp, LPDIRECT3DLIGHT lpd3dl, LPDIRECT3DLIGHT *lpd3dnextl, DWORD dw)
{
	HRESULT res;

	OutTrace("%s: lpvp=%#x light=%#x dx=%#x\n", ApiRef, lpvp, lpd3dl, dw);
	res = (*pNextLight)(lpvp, lpd3dl, lpd3dnextl, dw);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
		return res;
	}
	OutTrace("%s: nextlight=%#x\n", ApiRef, *lpd3dnextl);
	return res;
}

HRESULT WINAPI extNextLight1(void *lpvp, LPDIRECT3DLIGHT lpd3dl, LPDIRECT3DLIGHT *lpd3dnextl, DWORD dw)
{ return extNextLight("IDirect3DViewport::NextLight", 1, pNextLight1, lpvp, lpd3dl, lpd3dnextl, dw); }
HRESULT WINAPI extNextLight2(void *lpvp, LPDIRECT3DLIGHT lpd3dl, LPDIRECT3DLIGHT *lpd3dnextl, DWORD dw)
{ return extNextLight("IDirect3DViewport2::NextLight", 2, pNextLight2, lpvp, lpd3dl, lpd3dnextl, dw); }
HRESULT WINAPI extNextLight3(void *lpvp, LPDIRECT3DLIGHT lpd3dl, LPDIRECT3DLIGHT *lpd3dnextl, DWORD dw)
{ return extNextLight("IDirect3DViewport3::NextLight", 3, pNextLight3, lpvp, lpd3dl, lpd3dnextl, dw); }
#endif // TRACEALL

HRESULT WINAPI extSetViewport7(void *d3dd, LPD3DVIEWPORT7 lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::SetViewport");

	OutTraceD3D("%s: d3dd=%#x d3dvp=%#x\n", ApiRef, d3dd, lpd3dvp);
	if(lpd3dvp && IsDebugD3D){
		OutTraceD3D("> pos(x,y) : %d,%d\n", lpd3dvp->dwX, lpd3dvp->dwY);
		OutTraceD3D("> size(w,h): %d,%d\n", lpd3dvp->dwWidth, lpd3dvp->dwHeight);
		OutTraceD3D("> z(min,max): %f,%f\n", lpd3dvp->dvMinZ, lpd3dvp->dvMaxZ);
	}
	res=(*pSetViewport7)(d3dd, lpd3dvp);
	_if(res) OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extGetViewport7(void *d3dd, LPD3DVIEWPORT7 lpd3dvp)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::GetViewport");

	OutTraceD3D("%s: d3dd=%#x d3dvp=%#x\n", ApiRef, d3dd, lpd3dvp);
	res=(*pGetViewport7)(d3dd, lpd3dvp);
	_if(res) OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	if(!res && lpd3dvp && IsDebugD3D){
		OutTraceD3D("> pos(x,y) : %d,%d\n", lpd3dvp->dwX, lpd3dvp->dwY);
		OutTraceD3D("> size(w,h): %d,%d\n", lpd3dvp->dwWidth, lpd3dvp->dwHeight);
		OutTraceD3D("> z(min,max): %f,%f\n", lpd3dvp->dvMinZ, lpd3dvp->dvMaxZ);
	}

	return res;
}

static HRESULT WINAPI SetTransform(ApiArg, int d3dversion, SetTransform_Type pSetTransform, void *lpd3dd, D3DTRANSFORMSTATETYPE tstype, LPD3DMATRIX matrix)
{
	HRESULT res;

	OutTraceD3D("%s: lpd3dd=%#x tstype=%#x(%s) matrix={\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n}\n", 
		ApiRef, lpd3dd, tstype, sTransformType(tstype),
		matrix->_11, matrix->_12, matrix->_13, matrix->_14,
		matrix->_21, matrix->_22, matrix->_23, matrix->_24,
		matrix->_31, matrix->_32, matrix->_33, matrix->_34,
		matrix->_41, matrix->_42, matrix->_43, matrix->_44);

	res = (*pSetTransform)(lpd3dd, tstype, matrix);
	_if(res) OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extSetTransform2(void *lpd3dd, D3DTRANSFORMSTATETYPE tstype, LPD3DMATRIX matrix)
{ return SetTransform("IDirect3DDevice2::SetTransform", 2, pSetTransform2, lpd3dd, tstype, matrix); }
HRESULT WINAPI extSetTransform3(void *lpd3dd, D3DTRANSFORMSTATETYPE tstype, LPD3DMATRIX matrix)
{ return SetTransform("IDirect3DDevice3::SetTransform", 3, pSetTransform3, lpd3dd, tstype, matrix); }
HRESULT WINAPI extSetTransform7(void *lpd3dd, D3DTRANSFORMSTATETYPE tstype, LPD3DMATRIX matrix)
{ return SetTransform("IDirect3DDevice7::SetTransform", 7, pSetTransform7, lpd3dd, tstype, matrix); }

// ====== tracing proxies ==========

#ifdef TRACEALL

HRESULT WINAPI extGetLight7(void *lpd3dd, DWORD Index, D3DLIGHT7 *light)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::GetLight");

	OutTraceD3D("%s: d3d=%#x index=%d\n", ApiRef, lpd3dd, Index);

	res = (*pGetLight7)(lpd3dd, Index, light);
	_if(res) OutErrorD3D("%s: ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extSetRenderTarget7(void *lpd3dd, LPDIRECTDRAWSURFACE7 lpdds, DWORD dwFlags)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::SetRenderTarget");

	OutTraceD3D("%s: d3d=%#x lpdds=%#x flags=%#x\n", ApiRef, lpd3dd, lpdds, dwFlags);

	res = (*pSetRenderTarget7)(lpd3dd, lpdds, dwFlags);
	_if(res) OutErrorD3D("%s: ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extGetRenderTarget7(void *lpd3dd, LPDIRECTDRAWSURFACE7 *lplpdds)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::GetRenderTarget");

	OutTraceD3D("%s: d3d=%#x\n", ApiRef, lpd3dd);

	res = (*pGetRenderTarget7)(lpd3dd, lplpdds);
	if(res) {
		OutErrorD3D("%s: ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} else {
		OutTraceD3D("%s: lpdds=%#x\n", ApiRef, *lplpdds);
	}
	return res;
}
#endif // TRACEALL

#ifndef DXW_NOTRACES
char *sClearFlags(DWORD c, char *eb, int eblen)
{
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "D3DCLEAR_", eblen);
	if (c & D3DCLEAR_TARGET) strscat(eb, eblen, "TARGET+");
	if (c & D3DCLEAR_ZBUFFER) strscat(eb, eblen, "ZBUFFER+");
	if (c & D3DCLEAR_STENCIL) strscat(eb, eblen, "STENCIL+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("D3DCLEAR_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}
#endif

HRESULT WINAPI extClearD7(void *lpd3dd, DWORD dw0, LPD3DRECT rect, DWORD dwFlags, D3DCOLOR color, D3DVALUE depth, DWORD dw2)
{
	ApiName("IDirect3DDevice7::Clear");
	HRESULT res;

#ifndef DXW_NOTRACES
	if(IsTraceD3D){
		char rectBuf[80];
		char sFlags[80];
		if(rect) sprintf(rectBuf, "(%ld,%ld - %ld,%ld)", rect->x1, rect->y1, rect->x2, rect->y2);
		else strcpy(rectBuf, "(NULL)");
		OutTrace("%s: d3d=%#x dw0=%#x rect=%s flags=%#x(%s) color=%#x depth=%f dw2=%#x\n", 
			ApiRef, lpd3dd, dw0, rectBuf, dwFlags, sClearFlags(dwFlags, sFlags, 80), color, depth, dw2);
	}
#endif


	res = (*pClearD7)(lpd3dd, dw0, rect, dwFlags, color, depth, dw2);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} 

	return res;
}

#ifdef TRACEALL
HRESULT WINAPI extEBInitialize(void *lpeb, LPDIRECT3DDEVICE lpd3dd, LPD3DEXECUTEBUFFERDESC lpebd)
{
	HRESULT res;
	ApiName("IDirect3DExecuteBuffer::Initialize");

	OutTraceD3D("%s: lpeb=%#x lpd3dd=%#x lpebd=%#x\n", ApiRef, lpeb, lpd3dd, lpebd);
	res = (*pEBInitialize)(lpeb, lpd3dd, lpebd);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} 
	return res;
}

HRESULT WINAPI extValidate(void *lpeb, LPDWORD offset, LPD3DVALIDATECALLBACK cb, LPVOID ctx, DWORD reserved)
{
	HRESULT res;
	ApiName("IDirect3DExecuteBuffer::Validate");

	OutTraceD3D("%s: lpeb=%#x offset=%#x cb=%#x ctx=%#x resvd=%#x\n", ApiRef, lpeb, offset, cb, ctx, reserved);
	res = (*pValidate)(lpeb, offset, cb, ctx, reserved);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} 
	return res;
}
#endif // TRACEALL

HRESULT WINAPI extOptimize(void *lpeb, DWORD dwFlags)
{
	HRESULT res;
	ApiName("IDirect3DExecuteBuffer::Optimize");

	OutTraceD3D("%s: lpeb=%#x flags=%#x\n", ApiRef, lpeb, dwFlags);
	if(dxw.dwFlags18 & NOD3DOPTIMIZE) {
		OutTraceD3D("%s: BYPASS\n", ApiRef);
		return D3D_OK;
	}

	res = (*pOptimize)(lpeb, dwFlags);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} 
	return res;
}

#ifndef DXW_NOTRACES
char *ExplainPrimType(DWORD c)
{
	char *p;
	switch(c){
		case D3DPT_POINTLIST:		p="POINTLIST"; break;
    	case D3DPT_LINELIST:		p="LINELIST"; break;
    	case D3DPT_LINESTRIP:		p="LINESTRIP"; break;
    	case D3DPT_TRIANGLELIST:	p="TRIANGLELIST"; break;
    	case D3DPT_TRIANGLESTRIP:	p="TRIANGLESTRIP"; break;
    	case D3DPT_TRIANGLEFAN:		p="TRIANGLEFAN"; break;
		default:					p="???"; break;
	}
	return p;
}

char *ExplainVType(DWORD c)
{
	char *p;
	switch(c){
		case D3DVT_VERTEX:			p="VERTEX"; break;
    	case D3DVT_LVERTEX:			p="LVERTEX"; break;
    	case D3DVT_TLVERTEX:		p="TLVERTEX"; break;
		default:					p="???"; break;
	}
	return p;
}

char *ExplainDrawPrimFlags(DWORD c, char*eb, int eblen)
{
#ifndef D3DDP_OUTOFORDER
#define D3DDP_OUTOFORDER 0x00000002
#endif // D3DDP_OUTOFORDER
	unsigned int l;
	eblen -= 4;
	strncpy(eb, "D3DDP_", eblen);
	if (c & D3DDP_WAIT) strscat(eb, eblen, "WAIT+");
	if (c & D3DDP_OUTOFORDER) strscat(eb, eblen, "OUTOFORDER+");
	if (c & D3DDP_DONOTCLIP) strscat(eb, eblen, "DONOTCLIP+");
	if (c & D3DDP_DONOTUPDATEEXTENTS) strscat(eb, eblen, "DONOTUPDATEEXTENTS+");
	if (c & D3DDP_DONOTLIGHT) strscat(eb, eblen, "DONOTLIGHT+");
	l=strlen(eb);
	if(l == eblen) strscat(eb, eblen+3, "...");
	else {
		if (l>strlen("D3DDP_")) eb[l-1]=0; // delete last '+' if any
		else eb[0]=0;
	}
	return(eb);
}
#endif // DXW_NOTRACES

HRESULT WINAPI extDrawPrimitive(ApiArg, int d3dversion, DrawPrimitive37_Type pDrawPrimitive, void *lpd3d, D3DPRIMITIVETYPE ptype, DWORD vtype, LPVOID pVerts, DWORD nVerts, DWORD dwFlags)
{
	HRESULT res;
#ifndef DXW_NOTRACES
	char sFlags[80];
#endif // DXW_NOTRACES
	OutTraceD3D("%s: lpd3d=%#x ptype=%#x(%s) vtype=%#x(%s) verts=%#x nverts=%d flags=%#x(%s)\n", 
		ApiRef, lpd3d, ptype, ExplainPrimType(ptype), vtype, ExplainVType(vtype), pVerts, nVerts, 
		dwFlags, ExplainDrawPrimFlags(dwFlags, sFlags, 80));
#ifdef DUMPEXECUTEBUFFER
	LPD3DHVERTEX pv;
	LPD3DLVERTEX plv;
	LPD3DTLVERTEX ptlv;
	switch(vtype){
		case D3DVT_VERTEX: 
			pv=(LPD3DHVERTEX)pVerts;
			for(DWORD i=0; i<nVerts; i++, pv++) 
				OutTrace("> v[%d] p=(%f,%f,%f} flags=%#x\n",
					i, pv->hx, pv->hy, pv->hz, pv->dwFlags
					);
			break;
		case D3DVT_LVERTEX: 
			plv=(LPD3DLVERTEX)pVerts;
			for(DWORD i=0; i<nVerts; i++, plv++) 
				OutTrace("> v[%d] p=(%f,%f,%f} tex=(%f,%f) color=%#x specular=%#x\n",
					i, plv->x, plv->y, plv->z, plv->tu, plv->tv, plv->color, plv->specular
					);
			break;
		case D3DVT_TLVERTEX: 
			ptlv=(LPD3DTLVERTEX)pVerts;
			for(DWORD i=0; i<nVerts; i++, ptlv++) 
				OutTrace("> v[%d] p=(%f,%f,%f} tex=(%f,%f) color=%#x specular=%#x\n",
					i, ptlv->sx, ptlv->sy, ptlv->sz, ptlv->tu, ptlv->tv, ptlv->color, ptlv->specular
					);
			break;
	}
#endif // DUMPEXECUTEBUFFER

	if((dxw.dwFlags16 & FIXTNLRHW) && (vtype == D3DVT_TLVERTEX)){
		OutTrace("%s: fixing rhw\n", ApiRef);
		LPD3DTLVERTEX ptlv=(LPD3DTLVERTEX)pVerts;
		for(DWORD i=0; i<nVerts; i++, ptlv++) ptlv->rhw = 1.0;
	}

#ifdef NOTATD3DUMDDILEVEL
	// v2.06.06: implements the alternate pixel center correction (adding -0.5f offset to both x and y coord,)
	// fixes @#@ "Beast Wars Transformers". 
	// See https://learn.microsoft.com/en-us/windows/win32/direct3d9/directly-mapping-texels-to-pixels
	if(dxw.dwFlags18 & ALTPIXELCENTER){
		OutTrace("%s: fixing AlternatePixelCenter\n", ApiRef);
		switch (vtype){
			case D3DVT_VERTEX:
			{
				LPD3DVERTEX ptlv=(LPD3DVERTEX)pVerts;
				for(DWORD i=0; i<nVerts; i++, ptlv++) {
					ptlv->x += -0.5f;
					ptlv->y += -0.5f;
				}
			}
			break;
			case D3DVT_LVERTEX:
			{
				LPD3DLVERTEX ptlv=(LPD3DLVERTEX)pVerts;
				for(DWORD i=0; i<nVerts; i++, ptlv++) {
					ptlv->x += -0.5f;
					ptlv->y += -0.5f;
				}
			}
			break;
			case D3DVT_TLVERTEX:
			{
				LPD3DTLVERTEX ptlv=(LPD3DTLVERTEX)pVerts;
				for(DWORD i=0; i<nVerts; i++, ptlv++) {
					ptlv->sx += -0.5f;
					ptlv->sy += -0.5f;
				}
				break;
			}
		}
	}
#endif //NOTATD3DUMDDILEVEL

	res = (*pDrawPrimitive)(lpd3d, ptype, vtype, pVerts, nVerts, dwFlags);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} 
	return res;
}

HRESULT WINAPI extDrawPrimitive2(void *lpd3d, D3DPRIMITIVETYPE ptype, D3DVERTEXTYPE vtype, LPVOID pVerts, DWORD nVerts, DWORD dwFlags)
{ return extDrawPrimitive("IDirect3DDevice2::DrawPrimitive", 2, (DrawPrimitive37_Type)pDrawPrimitive2, lpd3d, ptype, (DWORD)vtype, pVerts, nVerts, dwFlags); }
HRESULT WINAPI extDrawPrimitive3(void *lpd3d, D3DPRIMITIVETYPE ptype, DWORD vtype, LPVOID pVerts, DWORD nVerts, DWORD dwFlags)
{ return extDrawPrimitive("IDirect3DDevice3::DrawPrimitive", 3, (DrawPrimitive37_Type)pDrawPrimitive3, lpd3d, ptype, vtype, pVerts, nVerts, dwFlags); }
HRESULT WINAPI extDrawPrimitive7(void *lpd3d, D3DPRIMITIVETYPE ptype, DWORD vtype, LPVOID pVerts, DWORD nVerts, DWORD dwFlags)
{ return extDrawPrimitive("IDirect3DDevice7::DrawPrimitive", 7, (DrawPrimitive37_Type)pDrawPrimitive7, lpd3d, ptype, vtype, pVerts, nVerts, dwFlags); }

#ifdef TRACEMATERIAL
HRESULT WINAPI extPick(void *lpd3dd, LPDIRECT3DEXECUTEBUFFER lpeb, LPDIRECT3DVIEWPORT lpvp, DWORD dw, LPD3DRECT lprect)
{
	HRESULT res;
	ApiName("IDirect3DDevice::Pick");

	OutTraceD3D("%s: d3d=%#x eb=%#x viewport=%#x dw=%#x rect=%#x\n", ApiRef, lpd3dd, lpeb, lpvp, dw, lprect);

	res = (*pPick)(lpd3dd, lpeb, lpvp, dw, lprect);
	if(res) {
		OutErrorD3D("%s: ERROR ret=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	} 
	return res;
}
#endif // TRACEMATERIAL

#ifdef TRACEALL
static HRESULT WINAPI extGetTextureStageState(ApiArg, GetTextureStageState_Type pGetTextureStageState, void *lpd3dd, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	HRESULT res;
	OutTraceD3D("%s: lpd3dd=%#x stage=%d state=%#x\n", ApiRef, lpd3dd, dwStage, dwState);
	res = (*pGetTextureStageState)(lpd3dd, dwStage, dwState, lpdwValue);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: value=%#x(%s)\n", ApiRef, *lpdwValue, explainD3DTextureStageState(*lpdwValue));
	}
	return res;
}

HRESULT WINAPI extGetTextureStageState3(void *lpd3dd, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{ return extGetTextureStageState("IDirect3DDevice3::GetTextureStageState", pGetTextureStageState3, lpd3dd, dwStage, dwState, lpdwValue); }
HRESULT WINAPI extGetTextureStageState7(void *lpd3dd, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{ return extGetTextureStageState("IDirect3DDevice7::GetTextureStageState", pGetTextureStageState7, lpd3dd, dwStage, dwState, lpdwValue); }

static HRESULT WINAPI extSetTextureStageState(ApiArg, SetTextureStageState_Type pSetTextureStageState, void *lpd3dd, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	HRESULT res;
	OutTraceD3D("%s: lpd3dd=%#x stage=%d state=%#x(%s) value=%#x(%s)\n", 
		ApiRef, lpd3dd, dwStage, dwState, explainD3DTextureStageState(dwState), dwValue, explainD3DTextureOp(dwState, dwValue));
	res = (*pSetTextureStageState)(lpd3dd, dwStage, dwState, dwValue);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res; 
}

HRESULT WINAPI extSetTextureStageState3(void *lpd3dd, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{ return extSetTextureStageState("IDirect3DDevice3::SetTextureStageState", pSetTextureStageState3, lpd3dd, dwStage, dwState, dwValue); }
HRESULT WINAPI extSetTextureStageState7(void *lpd3dd, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{ return extSetTextureStageState("IDirect3DDevice7::SetTextureStageState", pSetTextureStageState7, lpd3dd, dwStage, dwState, dwValue); }

HRESULT WINAPI extLightEnable7(void *lpd3dd, DWORD index, BOOL enable)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::LightEnable");
	OutTraceD3D("%s: lpd3dd=%#x index=%d value=%#x\n", ApiRef, lpd3dd, index, enable);
	res = (*pLightEnable7)(lpd3dd, index, enable);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res; 
}

HRESULT WINAPI extGetLightEnable7(void *lpd3dd, DWORD index, BOOL *enable)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::GetLightEnable");
	OutTraceD3D("%s: lpd3dd=%#x index=%d\n", ApiRef, lpd3dd, index);
	res = (*pGetLightEnable7)(lpd3dd, index, enable);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: enable=%#x\n", ApiRef, *enable);
	}
	return res; 
}

HRESULT WINAPI extSetClipPlane7(void *lpd3dd, DWORD index, D3DVALUE *pPlane)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::SetClipPlane");
	OutTraceD3D("%s: lpd3dd=%#x index=%d value=%f\n", ApiRef, lpd3dd, index, *pPlane);
	res = (*pSetClipPlane7)(lpd3dd, index, pPlane);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res; 
}

HRESULT WINAPI extGetClipPlane7(void *lpd3dd, DWORD index, D3DVALUE *pPlane)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::GetClipPlane");
	OutTraceD3D("%s: lpd3dd=%#x index=%d value=%f\n", ApiRef, lpd3dd, index, *pPlane);
	res = (*pSetClipPlane7)(lpd3dd, index, pPlane);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: lpd3dd=%#x index=%d value=%f\n", ApiRef, lpd3dd, index, *pPlane);
	}
	return res; 
}

HRESULT WINAPI extGetTexture3(void *d3dd, DWORD stage, LPDIRECT3DTEXTURE2 *lptex)
{
	HRESULT res;
	ApiName("IDirect3DDevice3::GetTexture");

	OutTraceD3D("%s: d3dd=%#x stage=%d\n", ApiRef, d3dd, stage);

	res=(*pGetTexture3)(d3dd, stage, lptex);
	if(res) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: OK tex=%#x\n", ApiRef, *lptex);
	}
	return res;
}

HRESULT WINAPI extGetTexture7(void *d3dd, DWORD stage, LPDIRECTDRAWSURFACE7 *lptex)
{
	HRESULT res;
	ApiName("IDirect3DDevice7::GetTexture");

	OutTraceD3D("%s: d3dd=%#x stage=%d\n", ApiRef, d3dd, stage);

	res=(*pGetTexture7)(d3dd, stage, lptex);
	if(res) {
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: OK tex=%#x\n", ApiRef, *lptex);
	}
	return res;

}
#endif // TRACEALL

HRESULT WINAPI extCreateMatrix(void *lpd3dd, LPD3DMATRIXHANDLE lpmh)
{
	HRESULT res;
	ApiName("IDirect3DDevice::CreateMatrix");
	OutTraceD3D("%s: lpd3d=%#x lpmh=%#x\n", ApiRef, lpd3dd, lpmh);
	res=(*pCreateMatrix)(lpd3dd, lpmh);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: *lpmh=%#x\n", ApiRef, *lpmh);
	}
	return res;
}

HRESULT WINAPI extSetMatrix(void *lpd3dd, D3DMATRIXHANDLE mh, const LPD3DMATRIX lpmatrix)
{
	HRESULT res;
	ApiName("IDirect3DDevice::SetMatrix");
	OutTraceD3D("%s: lpd3d=%#x mh=%#x\n", ApiRef, lpd3dd, mh);
	float *m = (float *)lpmatrix;
	OutTraceD3D("> %f, %f, %f, %f\n", m[ 0], m[ 1], m[ 2], m[ 3]);
	OutTraceD3D("> %f, %f, %f, %f\n", m[ 4], m[ 5], m[ 6], m[ 7]);
	OutTraceD3D("> %f, %f, %f, %f\n", m[ 8], m[ 9], m[10], m[11]);
	OutTraceD3D("> %f, %f, %f, %f\n", m[12], m[13], m[14], m[15]);
	res=(*pSetMatrix)(lpd3dd, mh, lpmatrix);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}

HRESULT WINAPI extGetMatrix(void *lpd3dd, D3DMATRIXHANDLE mh, LPD3DMATRIX lpmatrix)
{
	HRESULT res;
	ApiName("IDirect3DDevice::GetMatrix");
	OutTraceD3D("%s: lpd3d=%#x mh=%#x\n", ApiRef, lpd3dd, mh);
	res=(*pGetMatrix)(lpd3dd, mh, lpmatrix);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		float *m = (float *)lpmatrix;
		OutTraceD3D("> %f, %f, %f, %f\n", m[ 0], m[ 1], m[ 2], m[ 3]);
		OutTraceD3D("> %f, %f, %f, %f\n", m[ 4], m[ 5], m[ 6], m[ 7]);
		OutTraceD3D("> %f, %f, %f, %f\n", m[ 8], m[ 9], m[10], m[11]);
		OutTraceD3D("> %f, %f, %f, %f\n", m[12], m[13], m[14], m[15]);
	}
	return res;
}

HRESULT WINAPI extDeleteMatrix(void *lpd3dd, D3DMATRIXHANDLE mh)
{
	HRESULT res;
	ApiName("IDirect3DDevice::DeleteMatrix");
	OutTraceD3D("%s: lpd3d=%#x mh=%#x\n", ApiRef, lpd3dd, mh);
	res=(*pDeleteMatrix)(lpd3dd, mh);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}

#ifndef DXW_NOTRACES
char *explainVBCaps(DWORD caps, char *capsBuf)
{
	capsBuf[0]=0;
	if(caps & D3DVBCAPS_DONOTCLIP) strcat(capsBuf, "DONOTCLIP+");
	if(caps & D3DVBCAPS_SYSTEMMEMORY) strcat(capsBuf, "SYSTEMMEMORY+");
	if(caps & D3DVBCAPS_OPTIMIZED) strcat(capsBuf, "OPTIMIZED+");
	if(caps & D3DVBCAPS_WRITEONLY) strcat(capsBuf, "WRITEONLY+");
	if(strlen(capsBuf)>1) capsBuf[strlen(capsBuf)-1]=0;
	return capsBuf;
}

void dumpVertexBufferDesc(LPD3DVERTEXBUFFERDESC lpvbdesc)
{
	char caps[81];
	OutTrace("> dwSize=%d\n", lpvbdesc->dwSize);
	OutTrace("> dwCaps=%#x(%s)\n", lpvbdesc->dwCaps, explainVBCaps(lpvbdesc->dwCaps, caps));
    OutTrace("> dwFVF=%#x\n", lpvbdesc->dwFVF);
	OutTrace("> dwNumVertices=%d\n", lpvbdesc->dwNumVertices);
}
#endif // DXW_NOTRACES

HRESULT WINAPI extCreateVertexBuffer3(void *lpd3d, LPD3DVERTEXBUFFERDESC lpvbdesc, LPDIRECT3DVERTEXBUFFER *lplpvb, DWORD flags, LPUNKNOWN unk)
{
	HRESULT res;
	ApiName("IDirect3D3::CreateVertexBuffer");
#ifndef DXW_NOTRACES
	OutTraceD3D("%s: lpd3d=%#x lpvbdesc=%#x flags=%#x unk=%#x\n", 
		ApiRef, lpd3d, lpvbdesc, lplpvb, flags, unk);
	if(IsDebugD3D) dumpVertexBufferDesc(lpvbdesc);
#endif // DXW_NOTRACES
	if(dxw.dwFlags17 & FORCEVBUFONSYSMEM){
		OutTraceD3D("%s: setting SYSTEMMEMORY\n", ApiRef);
		lpvbdesc->dwCaps |= D3DVBCAPS_SYSTEMMEMORY;
	}
	res = (*pCreateVertexBuffer3)(lpd3d, lpvbdesc, lplpvb, flags, unk);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: ret=OK lpvb=%#x\n", ApiRef, *lplpvb);
		HookVertexBuffer3(lplpvb);
	}
	return res;
}

HRESULT WINAPI extCreateVertexBuffer7(void *lpd3d, LPD3DVERTEXBUFFERDESC lpvbdesc, LPDIRECT3DVERTEXBUFFER7 *lplpvb, DWORD flags)
{
	HRESULT res;
	ApiName("IDirect3D7::CreateVertexBuffer");
#ifndef DXW_NOTRACES
	OutTraceD3D("%s: lpd3d=%#x lpvbdesc=%#x flags=%#x\n", 
		ApiRef, lpd3d, lpvbdesc, lplpvb, flags);
	if(IsDebugD3D) dumpVertexBufferDesc(lpvbdesc);
#endif // DXW_NOTRACES
	if(dxw.dwFlags17 & FORCEVBUFONSYSMEM){
		OutTraceD3D("%s: setting SYSTEMMEMORY\n", ApiRef);
		lpvbdesc->dwCaps |= D3DVBCAPS_SYSTEMMEMORY;
	}
	res = (*pCreateVertexBuffer7)(lpd3d, lpvbdesc, lplpvb, flags);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	else {
		OutTraceD3D("%s: ret=OK lpvb=%#x\n", ApiRef, *lplpvb);
		HookVertexBuffer7(lplpvb);
	}
	return res;
}

//----------------------------------------------------------------------//
// Vertex Buffers
//----------------------------------------------------------------------//

typedef HRESULT (WINAPI *OptimizeVB3_Type)(IDirect3DVertexBuffer *, IDirect3DDevice3 *, DWORD);
OptimizeVB3_Type pOptimizeVB3;
HRESULT WINAPI extOptimizeVB3(IDirect3DVertexBuffer *, IDirect3DDevice3 *, DWORD);
typedef HRESULT (WINAPI *OptimizeVB7_Type)(IDirect3DVertexBuffer7 *, IDirect3DDevice7 *, DWORD);
OptimizeVB7_Type pOptimizeVB7;
HRESULT WINAPI extOptimizeVB7(IDirect3DVertexBuffer7 *, IDirect3DDevice7 *, DWORD);

void HookVertexBuffer3(LPDIRECT3DVERTEXBUFFER *obj)
{
	ApiName("dxwnd.HookVertexBuffer3");
	OutTraceD3D("%s: vb=%#x\n", ApiRef, *obj);

	//  0: QueryInterface
	//  4: AddRef
	//  8: Release
	// 12: Lock
	// 16: Unlock
	// 20: ProcessVertices
	// 24: GetVertexBufferDesc
	SetHook((void *)(**(DWORD **)obj +  28), extOptimizeVB3, (void **)&pOptimizeVB3, "Optimize");
}

void HookVertexBuffer7(LPDIRECT3DVERTEXBUFFER7 *obj)
{
	ApiName("dxwnd.HookVertexBuffer7");
	OutTraceD3D("%s: vb=%#x\n", ApiRef, *obj);

	//  0: QueryInterface
	//  4: AddRef
	//  8: Release
	// 12: Lock
	// 16: Unlock
	// 20: ProcessVertices
	// 24: GetVertexBufferDesc
	SetHook((void *)(**(DWORD **)obj +  28), extOptimizeVB7, (void **)&pOptimizeVB7, "Optimize");
	// 32: ProcessVerticesStrided
}

HRESULT WINAPI extOptimizeVB3(IDirect3DVertexBuffer *iface, IDirect3DDevice3 *lpd3dd, DWORD flags)
{
	HRESULT res;
	ApiName("IDirect3DVertexBuffer::Optimize");
	OutTraceD3D("%s: vb=%#x lpd3dd=%#x flags=%#x\n", ApiRef, iface, lpd3dd, flags);

	if(dxw.dwFlags18 & NOD3DOPTIMIZE) {
		OutTrace("%s: BYPASS\n", ApiRef);
		return D3D_OK;
	}

	res=(*pOptimizeVB3)(iface, lpd3dd, flags);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}

HRESULT WINAPI extOptimizeVB7(IDirect3DVertexBuffer7 *iface, IDirect3DDevice7 *lpd3dd, DWORD flags)
{
	HRESULT res;
	ApiName("IDirect3DVertexBuffer7::Optimize");
	OutTraceD3D("%s: vb=%#x lpd3dd=%#x flags=%#x\n", ApiRef, iface, lpd3dd, flags);

	if(dxw.dwFlags18 & NOD3DOPTIMIZE) {
		OutTrace("%s: BYPASS\n", ApiRef);
		return D3D_OK;
	}

	res=(*pOptimizeVB7)(iface, lpd3dd, flags);
	if(res){
		OutErrorD3D("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDDError(res));
	}
	return res;
}