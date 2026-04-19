/*
DxWnd v2.05.90

This code was made possible only thanks to Narzoul's support that very kindly 
explained how it is possible to enable W-based fog by hacking the fnUpdateWInfo
call in WDDM-compatible graphics drivers. Most of the code was adapter from
DDrawCompat 4.0 source code.

This implementation is not currently importing the whole d3dumddi.h data structures 
and relative dependencies, it re-defines only the relevant part of them to make
the code compatible with VS2008. In particular, the macro LPGENERIC(label) 
defines a generic 32-bit DWORD data with a "label" reminder of the actual data 
type that was not defined.

This implementation is currently hacking only the OpenAdapter call, it should be
completed with the hacking of GetPrivateDDITable as soon as I'll find a case for testing.
The schema summarizes the steps to get from OpenAdapter to fnUpdateWInfo:

	OpenAdapter->(D3DDDIARG_OPENADAPTER *)unnamedParam1;
    unnamedParam1->(D3DDDI_ADAPTERFUNCS *)pAdapterFuncs;
    pAdapterFuncs->(PFND3DDDI_CREATEDEVICE)pfnCreateDevice;
    pfnCreateDevice->(D3DDDI_DEVICEFUNCS *)pDeviceFuncs;
    pDeviceFuncs->(PFND3DDDI_UPDATEWINFO)fnUpdateWInfo;

// ----------------- DDrawCompat features --------------------------

HRESULT APIENTRY updateWInfo(HANDLE hDevice, const D3DDDIARG_WINFO* pData)
{
	if (pData && 1.0f == pData->WNear && 1.0f == pData->WFar){
		D3DDDIARG_WINFO wInfo = {};
		wInfo.WNear = 0.0f;
		wInfo.WFar = 1.0f;
		return getOrigVtable(hDevice).pfnUpdateWInfo(hDevice, &wInfo);
	}
	return getOrigVtable(hDevice).pfnUpdateWInfo(hDevice, pData);
}

*/

#define ZOOMEFFECT
#define CHECKDUPLICATESCALING
#define _MODULE "d3dumddi"

#include <windows.h>
#include <ddraw.h>
#include <d3dtypes.h>
#include <d3dumddi.h>
#include "dxwnd.h"
#include "dxhook.h"
#include "ddrawi.h"
#include "dxwcore.hpp"
#include "stdio.h" 
#include "hddraw.h"
#include "dxhelper.h"
#include "syslibs.h"

LPVOID pSelectedVertexBuffer = NULL;
UINT dSelectedStride = 0;
BOOL bUMBuffer;
BOOL bColorKeyDisable = TRUE;
D3DVALUE fZoom = 1.0;

void associate(HANDLE, LPVOID);
void unassociate(HANDLE);

#define MAXVBUFFERS 10

typedef struct _VTENTRY {
	HANDLE h;
	LPVOID p;
} VTENTRY;

VTENTRY vTable[MAXVBUFFERS];

static void associate(HANDLE h, LPVOID p)
{
	OutTraceDRV("associate: h=%#x p=%#x\n", h, p);
	for(int i=0; i<MAXVBUFFERS; i++)
		if(vTable[i].h == 0) {
			vTable[i].h = h;
			vTable[i].p = p;
			break;
		}
}

static void unassociate(HANDLE h)
{
	for(int i=0; i<MAXVBUFFERS; i++)
		if(vTable[i].h == h) {
			OutTraceDRV("unassociate: h=%#x\n", h);
			vTable[i].h = 0;
			vTable[i].p = 0;
			break;
		}
}

static LPVOID isassociated(HANDLE h)
{
	for(int i=0; i<MAXVBUFFERS; i++)
		if(vTable[i].h == h) {
			OutTraceDRV("isassociated: h=%#x p=%#x\n", h, vTable[i].p);
			return vTable[i].p;
		}
	return NULL;
}

#define LPGENERIC(type) LPVOID

typedef HRESULT (WINAPI *updateWInfo_Type)(HANDLE, const D3DDDIARG_WINFO *);
updateWInfo_Type pUpdateWInfo;

//typedef HRESULT (WINAPI *PFND3DDDI_CLEAR)(HANDLE, CONST D3DDDIARG_CLEAR *, UINT, CONST RECT *);
//PFND3DDDI_CLEAR pClear;
//PFND3DDDI_CLEAR pfnClear;

HRESULT APIENTRY updateWInfo(HANDLE hDevice, const D3DDDIARG_WINFO* pData)
{
	HRESULT res;
	ApiName("updateWInfo");
	if(pData) OutTraceDRV("%s: wnear=%f wfar=%f\n", ApiRef, pData->WNear, pData->WFar);
	else OutTraceDRV("%s: pData=NULL\n", ApiRef);
	if ((dxw.dwFlags19 & FORCEWBASEDFOGWDDM) && pData && 1.0f == pData->WNear && 1.0f == pData->WFar){
		D3DDDIARG_WINFO wInfo = {};
		wInfo.WNear = 0.0f;
		wInfo.WFar = 1.0f;
		res = (*pUpdateWInfo)(hDevice, &wInfo);
	}
	else {
		res = (*pUpdateWInfo)(hDevice, pData);
	}
	OutTraceDRV("%s: res=%#x\n", ApiRef, res);
	return res;
}

static char *ExplainPrimType(DWORD c)
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

static UINT getVertexCount(D3DPRIMITIVETYPE primitiveType, UINT primitiveCount)
{
	switch (primitiveType)
	{
	case D3DPT_POINTLIST:
		return primitiveCount;
	case D3DPT_LINELIST:
		return primitiveCount * 2;
	case D3DPT_LINESTRIP:
		return primitiveCount + 1;
	case D3DPT_TRIANGLELIST:
		return primitiveCount * 3;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_TRIANGLEFAN:
		return primitiveCount + 2;
	}
	return 0;
}

PFND3DDDI_DEPTHFILL pfnDepthFill;
PFND3DDDI_DRAWPRIMITIVE pfnDrawPrimitive;
PFND3DDDI_DRAWPRIMITIVE2 pfnDrawPrimitive2;
PFND3DDDI_DRAWINDEXEDPRIMITIVE pfnDrawIndexedPrimitive;
PFND3DDDI_DRAWINDEXEDPRIMITIVE2 pfnDrawIndexedPrimitive2;
PFND3DDDI_SETSTREAMSOURCEUM pfnSetStreamSourceUm; 
PFND3DDDI_SETSTREAMSOURCE pfnSetStreamSource; 
PFND3DDDI_CREATERESOURCE pfnCreateResource;
PFND3DDDI_CREATERESOURCE2 pfnCreateResource2;
PFND3DDDI_DESTROYRESOURCE pfnDestroyResource;
PFND3DDDI_CREATEDEVICE pCreateDevice;
PFND3DDDI_SETINDICES pfnSetIndices;
PFND3DDDI_SETINDICESUM pfnSetIndicesUm;
PFND3DDDI_CREATEVERTEXSHADERDECL pfnCreateVertexShaderDecl;
PFND3DDDI_DELETEVERTEXSHADERDECL pfnDeleteVertexShaderDecl;
PFND3DDDI_SETVERTEXSHADERDECL pfnSetVertexShaderDecl;
PFND3DDDI_DRAWRECTPATCH pfnDrawRectPatch;
PFND3DDDI_DRAWTRIPATCH pfnDrawTriPatch;
PFND3DDDI_SETTEXTURESTAGESTATE pfnSetTextureStageState;
PFND3DDDI_SETTEXTURE pfnSetTexture;
PFND3DDDI_CLEAR pfnClear;
PFND3DDDI_SETRENDERSTATE pfnSetRenderState;


/*
 * Options for clearing
 */
#define D3DCLEAR_TARGET            0x00000001l  /* Clear target surface */
#define D3DCLEAR_ZBUFFER           0x00000002l  /* Clear target z buffer */
#define D3DCLEAR_STENCIL           0x00000004l  /* Clear stencil planes */

HRESULT WINAPI dxwClear(HANDLE hDevice, const D3DDDIARG_CLEAR *data, UINT numRect, const RECT *lpRect)
{
	ApiName("dxwClear");
	OutTraceDRV("%s:\n", ApiRef);
	bColorKeyDisable = TRUE;
	return (*pfnClear)(hDevice, data, numRect, lpRect);
}

HRESULT WINAPI dxwDepthFill(HANDLE hDevice, const D3DDDIARG_DEPTHFILL* data)
{
	ApiName("dxwDepthFill");
	HRESULT res;
	OutTraceDRV("%s: hDevice=%#x\n", ApiRef, hDevice);
	res = pfnDepthFill(hDevice, data);
	if(res) {
		OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
		return res;
	}
	//res = 0;
	//HANDLE resource = getResource(data->hResource);
	//HANDLE customResource = resource->getCustomResource();
	//int fi = getFormatInfo(resource->getFixedDesc().Format);	
	D3DDDIARG_CLEAR clear = {};
	clear.Flags = D3DCLEAR_ZBUFFER;
	//clear.FillDepth = getComponentAsFloat(data->Depth, fi.depth);
	clear.FillDepth = 1.0f;
	//if (0 != fi.stencil.bitCount)
	//{
		clear.Flags |= D3DCLEAR_STENCIL;
		//clear.FillStencil = getComponent(data->Depth, fi.stencil);
		clear.FillStencil = 0xFFFFFFFF;
	//}
	pfnClear(hDevice, &clear, 1, &data->DstRect);
	return res;
}

HRESULT WINAPI dxwSetTextureStageState(HANDLE hDevice, const D3DDDIARG_TEXTURESTAGESTATE* data)
{
	ApiName("dxwSetTextureStageState");
	static char *stateCaptions[]={
	"TEXTUREMAP",
    "COLOROP",
    "COLORARG1",
    "COLORARG2",
    "ALPHAOP",
    "ALPHAARG1",
    "ALPHAARG2",
    "BUMPENVMAT00",
    "BUMPENVMAT01",
    "BUMPENVMAT10",
    "BUMPENVMAT11",
    "TEXCOORDINDEX",
	"???",
    "ADDRESSU",
    "ADDRESSV",
    "BORDERCOLOR",
    "MAGFILTER",
    "MINFILTER",
    "MIPFILTER",
    "MIPMAPLODBIAS",
    "MAXMIPLEVEL",
    "MAXANISOTROPY",
    "BUMPENVLSCALE",
    "BUMPENVLOFFSET",
    "TEXTURETRANSFORMFLAGS",
    "ADDRESSW",
    "COLORARG0",
    "ALPHAARG0",
    "RESULTARG",
    "SRGBTEXTURE",
    "ELEMENTINDEX",
    "DMAPOFFSET",
    "CONSTANT",
    "DISABLETEXTURECOLORKEY",
    "TEXTURECOLORKEYVAL",
	"???"
	};

	OutTraceDRV("%s: hDevice=%#x data={stage=%d state=%#x(%s) val=%#x}\n", ApiRef, 
		hDevice, 
		data->Stage, 
		data->State, stateCaptions[data->State <= D3DDDITSS_TEXTURECOLORKEYVAL ? data->State : D3DDDITSS_TEXTURECOLORKEYVAL + 1], 
		data->Value);

	if (D3DDDITSS_TEXTURECOLORKEYVAL == data->State) bColorKeyDisable = FALSE;
	if (D3DDDITSS_DISABLETEXTURECOLORKEY == data->State) bColorKeyDisable = TRUE;

	return (*pfnSetTextureStageState)(hDevice, data);
}

HRESULT WINAPI dxwSetTexture(HANDLE hDevice,  UINT i,  HANDLE h)
{
	ApiName("dxwSetTexture");
	OutTraceDRV("%s: hDevice=%#x i=%#x h=%#x}\n", ApiRef, hDevice, i, h);
	return (*pfnSetTexture)(hDevice, i, h);
}

// ???
typedef enum _D3DDECLUSAGE
{
    D3DDECLUSAGE_POSITION = 0,
    D3DDECLUSAGE_BLENDWEIGHT,   // 1
    D3DDECLUSAGE_BLENDINDICES,  // 2
    D3DDECLUSAGE_NORMAL,        // 3
    D3DDECLUSAGE_PSIZE,         // 4
    D3DDECLUSAGE_TEXCOORD,      // 5
    D3DDECLUSAGE_TANGENT,       // 6
    D3DDECLUSAGE_BINORMAL,      // 7
    D3DDECLUSAGE_TESSFACTOR,    // 8
    D3DDECLUSAGE_POSITIONT,     // 9
    D3DDECLUSAGE_COLOR,         // 10
    D3DDECLUSAGE_FOG,           // 11
    D3DDECLUSAGE_DEPTH,         // 12
    D3DDECLUSAGE_SAMPLE,        // 13
} D3DDECLUSAGE;

HRESULT WINAPI dxwCreateVertexShaderDecl(HANDLE hDevice, D3DDDIARG_CREATEVERTEXSHADERDECL *pData, const D3DDDIVERTEXELEMENT *pVertexElements)
{
	HRESULT res;
	ApiName("CreateVertexShaderDecl");
	BOOL bHasTransformedVertices = FALSE;
	OutTraceDRV("%s: hDevice==%#x data=%x{NumVertexElements=%d} vertex=%#x\n", 
		ApiRef, hDevice, 
		pData, pData->NumVertexElements,
		pVertexElements);

	res = (*pfnCreateVertexShaderDecl)(hDevice, pData, pVertexElements);
	if(res) {
		OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	}
	else {
		OutTraceDRV("%s: ERROR ShaderHandle=%#x\n", ApiRef, pData->ShaderHandle);
		D3DDDIVERTEXELEMENT *ve = (D3DDDIVERTEXELEMENT *)pVertexElements;
		for(UINT i=0; i<pData->NumVertexElements; i++){
			if(ve->Usage == D3DDECLUSAGE_POSITIONT){
				OutTraceDRV("%s: has transformed vertices index=%d\n", ApiRef, i);
				bHasTransformedVertices = TRUE;
				break;
			}
		}
	}
	return res;
}

HRESULT WINAPI dxwDeleteVertexShaderDecl(HANDLE hDevice, HANDLE hShaderHandle)
{
	HRESULT res;
	ApiName("DeleteVertexShaderDecl");
	OutTraceDRV("%s: hDevice==%#x hShaderHandle=%x\n", 
		ApiRef, hDevice, hShaderHandle);

	res = (*pfnDeleteVertexShaderDecl)(hDevice, hShaderHandle);
	if(res) {
		OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

HRESULT WINAPI dxwSetVertexShaderDecl(HANDLE hDevice, HANDLE hShaderHandle)
{
	HRESULT res;
	ApiName("SetVertexShaderDecl");
	OutTraceDRV("%s: hDevice==%#x hShaderHandle=%x\n", 
		ApiRef, hDevice, hShaderHandle);

	res = (*pfnSetVertexShaderDecl)(hDevice, hShaderHandle);
	if(res) {
		OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	}
	return res;
}

static void fixDraw(HANDLE hDevice){
	if(dxw.dwFlags18 & FIXCOLORKEY) {
		D3DDDIARG_RENDERSTATE data;
		data.State = D3DDDIRS_COLORKEYENABLE;
		data.Value = bColorKeyDisable ? 0 : 1;	
		pfnSetRenderState(hDevice, (const D3DDDIARG_RENDERSTATE *)&data);
	}
}

HRESULT WINAPI dxwDrawPrimitive(HANDLE hDevice, const D3DDDIARG_DRAWPRIMITIVE *pData, const UINT *n)
{
	HRESULT res;
	ApiName("DrawPrimitive");
	OutTraceDRV("%s: hDevice==%#x primitive={type=%d(%s) start=%d count=%d} flag=%#x\n", 
		ApiRef, hDevice, 
		pData->PrimitiveType, ExplainPrimType(pData->PrimitiveType), pData->VStart, pData->PrimitiveCount,
		n ? *n : 0);

	fixDraw(hDevice);

	if((pData->PrimitiveType >= D3DPT_TRIANGLELIST) && pSelectedVertexBuffer){
#ifdef CHECKDUPLICATESCALING
		char *match;
		void *pVertexCopy;
#endif
		UINT vsize = getVertexCount(pData->PrimitiveType, pData->PrimitiveCount);
		if(dxw.dwFlags18 & ENABLEZOOMING){
#ifdef CHECKDUPLICATESCALING
			// initialize the match array for duplicate scaling detection 
			match = (char *)malloc(vsize);
			memset(match, 0, vsize);
			// save a copy of all vertices that will be scaled
			pVertexCopy = malloc(vsize * dSelectedStride);
			memcpy(
				pVertexCopy, 
				(char *)pSelectedVertexBuffer + (pData->VStart * dSelectedStride), 
				vsize * dSelectedStride);
#endif
			OutTraceDRV("zoom=%f\n", fZoom);
		}
		for(UINT i=0; i<vsize; i++){
			D3DVERTEX *v = (D3DVERTEX *)((char *)pSelectedVertexBuffer + ((pData->VStart + i)  * dSelectedStride));
			if(dxw.dwFlags18 & ENABLEZOOMING){
#ifdef CHECKDUPLICATESCALING
				if(match[i] == 0){
					v->x = v->x * fZoom;
					v->y = v->y * fZoom;
					match[i] = 1;
				}
#else
				v->x = v->x * fZoom;
				v->y = v->y * fZoom;
#endif
			}
			if(dxw.dwFlags18 & TRIMVERTEXBUFFER){
				if(v->z > 1.0) {
					OutTraceDRV("%s: trimming index=%d v=%f\n", ApiRef, pData->VStart + i, v->z);
					v->z = 1.0;
				}
				if(v->z < 0.0) {
					OutTraceDRV("%s: trimming index=%d v=%f\n", ApiRef, pData->VStart + i, v->z);
					v->z = 0.0;
				}
			}
			if(dxw.dwFlags18 & ALTPIXELCENTER){
				v->x += -0.5f;
				v->y += -0.5f;
			}
			v++;
		}
#ifdef CHECKDUPLICATESCALING
		if(dxw.dwFlags18 & ENABLEZOOMING) {
			res = (*pfnDrawPrimitive)(hDevice, pData, n);
			memcpy(
				(char *)pSelectedVertexBuffer + (pData->VStart * dSelectedStride), 
				pVertexCopy, 
				vsize * sizeof(D3DVERTEX));
			free(match);
			free(pVertexCopy);
			return res;
		}
#endif
	}

	res = (*pfnDrawPrimitive)(hDevice, pData, n);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI dxwDrawPrimitive2(HANDLE hDevice, const D3DDDIARG_DRAWPRIMITIVE2 *pData)
{
	HRESULT res;
	ApiName("DrawPrimitive2");
	OutTraceDRV("%s: hDevice==%#x primitive={type=%d(%s) offset=%#x count=%d}\n", 
		ApiRef, hDevice, 
		pData->PrimitiveType, ExplainPrimType(pData->PrimitiveType), pData->FirstVertexOffset, pData->PrimitiveCount);

	// ever seen ?????
	MessageBox(0, ApiRef, "dxwnd", 0);

	res = (*pfnDrawPrimitive2)(hDevice, pData);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI dxwDrawIndexedPrimitive(HANDLE hDevice,  const D3DDDIARG_DRAWINDEXEDPRIMITIVE *pData)
{
	HRESULT res;
	ApiName("DrawIndexedPrimitive");
	OutTraceDRV("%s: hDevice==%#x primitive={type=%d(%s) base=%d min=%d numV=%d start=%d count=%d}\n", 
		ApiRef, hDevice, 
		pData->PrimitiveType, ExplainPrimType(pData->PrimitiveType), 
		pData->BaseVertexIndex,
		pData->MinIndex,
		pData->NumVertices,
		pData->StartIndex,
		pData->PrimitiveCount);

	// ever seen ?????
	MessageBox(0, ApiRef, "dxwnd", 0);

	res = (*pfnDrawIndexedPrimitive)(hDevice, pData);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}


HRESULT WINAPI dxwDrawIndexedPrimitive2(HANDLE hDevice,  const D3DDDIARG_DRAWINDEXEDPRIMITIVE2 *pData, UINT dwIndicesSize, const VOID *pIndexBuffer, const UINT *pFlagBuffer)
{
	HRESULT res;
	ApiName("DrawIndexedPrimitive2");
	OutTraceDRV("%s: hDevice==%#x primitive={type=%d(%s) BaseVertexOffset=%d minIndex=%d NumVertices=%d StartIndexOffset=%d PrimitiveCount=%d} IndicesSize=%d pIndexBuffer=%#x pFlagBuffer=%#x\n", 
		ApiRef, hDevice, 
		pData->PrimitiveType, ExplainPrimType(pData->PrimitiveType), 
		pData->BaseVertexOffset,
		pData->MinIndex,
		pData->NumVertices,
		pData->StartIndexOffset,
		pData->PrimitiveCount,
		dwIndicesSize,
		pIndexBuffer,
		pFlagBuffer);

	fixDraw(hDevice);

	if((pData->PrimitiveType >= D3DPT_TRIANGLELIST) && pSelectedVertexBuffer){
		if(dwIndicesSize != sizeof(USHORT)){
			OutErrorDRV("%s: bad dwIndicesSize=%d\n", ApiRef, dwIndicesSize);
		}
#ifdef CHECKDUPLICATESCALING
		char *match;
		void *pVertexCopy;
#endif
		D3DVERTEX *vbase = (D3DVERTEX *)((char *)pSelectedVertexBuffer + pData->BaseVertexOffset);
		UINT vsize = getVertexCount(pData->PrimitiveType, pData->PrimitiveCount);
		LPVOID pRelocatedIndexBuffer = (LPVOID)((char *)pIndexBuffer + pData->StartIndexOffset);
		// re-calculate pData->NumVertices and pData->MinIndex
		UINT iMinIndex = 0xFFFFFFFF;
		UINT iMaxIndex = 0;
		for(UINT i=0; i<vsize; i++){
			USHORT index=((USHORT *)pRelocatedIndexBuffer)[i];
			if(index < iMinIndex) iMinIndex = index;
			if(index > iMaxIndex) iMaxIndex = index;
		}
		OutTraceDRV("%s: recalculated minIndex=%d->%d maxIndex=%d->%d\n", ApiRef, 
			pData->MinIndex, iMinIndex, 
			pData->NumVertices + pData->MinIndex -1, iMaxIndex);
		UINT numVertices = iMaxIndex - iMinIndex + 1;
		if(dxw.dwFlags18 & ENABLEZOOMING){
#ifdef CHECKDUPLICATESCALING
			// initialize the match array for duplicate scaling detection 
			match = (char *)malloc(numVertices);
			memset(match, 0, numVertices);
			// save a copy of all vertices that will be scaled
			pVertexCopy = malloc(numVertices * dSelectedStride);
			memcpy(
				pVertexCopy, 
				(char *)pSelectedVertexBuffer + (iMinIndex * dSelectedStride), 
				numVertices * dSelectedStride);
#endif
			OutTraceDRV("zoom=%f\n", fZoom);
		}
		//for(UINT i=pData->MinIndex; i<(pData->MinIndex + vsize); i++){
		for(UINT i=0; i<vsize; i++){
			UINT vertexIndex = (UINT)((USHORT *)pRelocatedIndexBuffer)[i];
			if(vertexIndex > numVertices) continue;
			D3DVERTEX *v = (D3DVERTEX *)((char *)vbase + (vertexIndex * dSelectedStride));
			if(dxw.dwFlags18 & ENABLEZOOMING){
#ifdef CHECKDUPLICATESCALING
				// scale the vertex if not done already
				if(match[vertexIndex-iMinIndex] == 0){
					v->x = v->x * fZoom;
					v->y = v->y * fZoom;
					match[vertexIndex-iMinIndex] = 1;
				}
#else
				v->x = v->x * fZoom;
				v->y = v->y * fZoom;
#endif
			}
			if(dxw.dwFlags18 & TRIMVERTEXBUFFER){
				// vertex z coordinate trimming in the range 0.0 - 1.0
				if(v->z > 1.0) {
					OutTraceDRV("%s: trimming index=%d v=%f\n", ApiRef, ((USHORT *)pRelocatedIndexBuffer)[i], v->z);
					v->z = 1.0;
				}
				if(v->z < 0.0) {
					OutTraceDRV("%s: trimming index=%d v=%f\n", ApiRef, ((USHORT *)pRelocatedIndexBuffer)[i], v->z);
					v->z = 0.0;
				}
			}
			if(dxw.dwFlags18 & ALTPIXELCENTER){
				v->x += -0.5f;
				v->y += -0.5f;
			}
		}
		if(dxw.dwFlags18 & ENABLEZOOMING) {
			res = (*pfnDrawIndexedPrimitive2)(hDevice, pData, dwIndicesSize, pIndexBuffer, pFlagBuffer);
#ifdef CHECKDUPLICATESCALING
			memcpy(
				(char *)pSelectedVertexBuffer + (iMinIndex * dSelectedStride), 
				pVertexCopy, 
				numVertices * sizeof(D3DVERTEX));
			free(match);
			free(pVertexCopy);
#endif
			return res;
		}
	}

	res = (*pfnDrawIndexedPrimitive2)(hDevice, pData, dwIndicesSize, pIndexBuffer, pFlagBuffer);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI dxwDrawRectPatch(HANDLE hDevice, CONST D3DDDIARG_DRAWRECTPATCH *pData, CONST D3DDDIRECTPATCH_INFO *pInfo, CONST FLOAT *pPatch)
{
	HRESULT res;
	ApiName("DrawRectPatch");
	OutTraceDRV("%s: hDevice=%#x pData=%#x{handle=%#x} pInfo=%#x{StartVertexOffset=%dx%d wxh=%dx%d stride=%d} pPatch=%#x\n", 
		ApiRef, hDevice, 
		pData, pData->Handle,
		pInfo, pInfo->StartVertexOffsetWidth, pInfo->StartVertexOffsetHeight, pInfo->Width, pInfo->Height, pInfo->Stride,
		pPatch);

	// ever seen ?????
	MessageBox(0, ApiRef, "dxwnd", 0);

	res = (*pfnDrawRectPatch)(hDevice, pData, pInfo, pPatch);

	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}
    UINT                StartVertexOffset;
    UINT                NumVertices;

HRESULT WINAPI dxwDrawTriPatch(HANDLE hDevice, CONST D3DDDIARG_DRAWTRIPATCH *pData, CONST D3DDDITRIPATCH_INFO *pInfo, CONST FLOAT *pPatch)
{
	HRESULT res;
	ApiName("DrawTriPatch");
	OutTraceDRV("%s: hDevice=%#x pData=%#x{handle=%#x} pInfo=%#x{StartVertexOffset=%d NumVertices=%d} pPatch=%#x\n", 
		ApiRef, hDevice, 
		pData, pData->Handle,
		pInfo, pInfo->StartVertexOffset, pInfo->NumVertices,
		pPatch);

	// ever seen ?????
	MessageBox(0, ApiRef, "dxwnd", 0);

	res = (*pfnDrawTriPatch)(hDevice, pData, pInfo, pPatch);

	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI dxwSetStreamSource(HANDLE hDevice, const D3DDDIARG_SETSTREAMSOURCE *pData)
{
	HRESULT res;
	ApiName("SetStreamSource");
	OutTraceDRV("%s: hDevice=%#x pData=%#x{stream=%d hVertexBuffer=%#x offset=%d stride=%d}\n", 
		ApiRef, hDevice, 
		pData, pData->Stream, pData->hVertexBuffer, pData->Offset, pData->Stride);

	LPVOID ptr;
	if(ptr = isassociated(pData->hVertexBuffer)){
		OutTraceDRV("MATCH!\n");
		pSelectedVertexBuffer = ptr;
		dSelectedStride = pData->Stride;
		bUMBuffer = FALSE;
	}

	res = (*pfnSetStreamSource)(hDevice, pData);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI dxwSetIndices(HANDLE hDevice, CONST D3DDDIARG_SETINDICES *pData)
{
	HRESULT res;
	ApiName("SetIndices");
	OutTraceDRV("%s: hDevice=%#x pData=%#x{hIndexBuffer=%#x stride=%d}\n", 
		ApiRef, hDevice, pData, pData->hIndexBuffer, pData->Stride);

	res = (*pfnSetIndices)(hDevice, pData);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI dxwSetIndicesUm(HANDLE hDevice, UINT indexSize, CONST VOID *puMBbuffer)
{
	HRESULT res;
	ApiName("SetIndicesUm");
	OutTraceDRV("%s: hDevice=%#x indexSize=%d puMBbuffer=%#x\n", 
		ApiRef, hDevice, indexSize, puMBbuffer);

	res = (*pfnSetIndicesUm)(hDevice, indexSize, puMBbuffer);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI dxwSetStreamSourceUm(HANDLE hDevice, const D3DDDIARG_SETSTREAMSOURCEUM *pData, const VOID *pUMBuffer)
{
	HRESULT res;
	ApiName("SetStreamSourceUm");
	OutTraceDRV("SetStreamSourceUm: hDevice=%#x pData=%#x{stream=%d stride=%d} pUMBuffer=%#x\n", 
		hDevice, 
		pData, pData->Stream, pData->Stride,
		pUMBuffer);

	pSelectedVertexBuffer = (LPVOID)pUMBuffer;
	bUMBuffer = TRUE;

	res = (*pfnSetStreamSourceUm)(hDevice, pData, pUMBuffer);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

static char *sRotation(DWORD rotation)
{
	char *s = "???";
	switch(rotation){
		case D3DDDI_ROTATION_IDENTITY: s = "IDENTITY"; break;
		case D3DDDI_ROTATION_90: s = "90"; break;
		case D3DDDI_ROTATION_180: s = "180"; break;
		case D3DDDI_ROTATION_270: s = "270"; break;
	}
	return s;
}

static char *sPool(DWORD pool)
{
	char *s = "???";
	switch(pool){
		case D3DDDIPOOL_SYSTEMMEM: s = "SYSTEMMEM"; break;
		case D3DDDIPOOL_VIDEOMEMORY: s = "VIDEOMEMORY"; break;
		case D3DDDIPOOL_LOCALVIDMEM: s = "LOCALVIDMEM"; break;
		case D3DDDIPOOL_NONLOCALVIDMEM: s = "NONLOCALVIDMEM"; break;
		case D3DDDIPOOL_STAGINGMEM: s = "STAGINGMEM"; break;
	}
	return s;
}

HRESULT WINAPI dxwCreateResource(HANDLE hDevice, D3DDDIARG_CREATERESOURCE *pResource)
{
	HRESULT res;
	ApiName("CreateResource");
	OutTraceDRV("%s: hDevice=%#x pResource=%#x{format=%d(%s) pool=%s SurfCount=%d MipLevels=%d fvf=%#x handle=%#x flags=%#x rot=%s}\n", ApiRef, hDevice, pResource,
		pResource->Format, ExplainD3DSurfaceFormat(pResource->Format),
		sPool(pResource->Pool),
		pResource->SurfCount,
		pResource->MipLevels,
		pResource->Fvf,
		pResource->hResource,
		pResource->Flags,
		sRotation(pResource->Rotation)
		);

	if((pResource->Pool != D3DDDIPOOL_SYSTEMMEM) && (pResource->Flags.MightDrawFromLocked || pResource->Flags.VertexBuffer)){
		OutTraceDRV("%s: suppress implicit vertex buffer on %s\n", ApiRef, sPool(pResource->Pool));
		return E_FAIL;
	}

	res = (*pfnCreateResource)(hDevice, pResource);
	if(res) {
		OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
		return res;
	}
	else {
		OutTraceDRV("> handle=%#x\n", pResource->hResource);
	}

	if( (pResource->Format == D3DDDIFMT_VERTEXDATA) &&
		(pResource->Pool == D3DDDIPOOL_SYSTEMMEM) &&
		(pResource->Flags.VertexBuffer || pResource->Flags.MightDrawFromLocked)){
		OutTraceDRV("%s: HOOK SurfCount=%d handle=%#x address=%#x\n", ApiRef, pResource->SurfCount, pResource->hResource, pResource->pSurfList[0].pSysMem);
		associate(pResource->hResource, (LPVOID)pResource->pSurfList[0].pSysMem);
	}

	return res;
}

HRESULT WINAPI dxwCreateResource2(HANDLE hDevice, D3DDDIARG_CREATERESOURCE2 *pResource)
{
	HRESULT res;
	ApiName("CreateResource2");
	OutTraceDRV("%s: hDevice=%#x pResource=%#x{format=%d(%s) pool=%s SurfCount=%d MipLevels=%d fvf=%#x handle=%#x flags=%#x rot=%s}\n", ApiRef, hDevice, pResource,
		pResource->Format, ExplainD3DSurfaceFormat(pResource->Format),
		sPool(pResource->Pool),
		pResource->SurfCount,
		pResource->MipLevels,
		pResource->Fvf,
		pResource->hResource,
		pResource->Flags,
		sRotation(pResource->Rotation)
		);

	if((pResource->Pool != D3DDDIPOOL_SYSTEMMEM) && (pResource->Flags.MightDrawFromLocked || pResource->Flags.VertexBuffer)){
		OutTraceDRV("%s: suppress implicit vertex buffer on %s\n", ApiRef, sPool(pResource->Pool));
		return E_FAIL;
	}

	res = (*pfnCreateResource2)(hDevice, pResource);
	if(res) {
		OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
		return res;
	}
	else {
		OutTraceDRV("> handle=%#x\n", pResource->hResource);
	}

	if( (pResource->Format == D3DDDIFMT_VERTEXDATA) &&
		(pResource->Pool == D3DDDIPOOL_SYSTEMMEM) &&
		(pResource->Flags.VertexBuffer || pResource->Flags.MightDrawFromLocked)){
		OutTraceDRV("%s: HOOK SurfCount=%d handle=%#x address=%#x\n", ApiRef, pResource->SurfCount, pResource->hResource, pResource->pSurfList[0].pSysMem);
		associate(pResource->hResource, (LPVOID)pResource->pSurfList[0].pSysMem);
	}

	return res;
}

HRESULT WINAPI dxwDestroyResource(HANDLE hDevice, HANDLE hResource)
{
	HRESULT res;
	ApiName("Destroyresource");
	OutTraceDRV("%s: hDevice=%#x hResource=%#x\n", ApiRef, hDevice, hResource);

	unassociate(hResource);

	res = (*pfnDestroyResource)(hDevice, hResource);
	if(res) OutTraceDRV("%s: ERROR res=%#x\n", ApiRef, res);
	return res;
}

HRESULT WINAPI extCreateDevice(HANDLE adapter, D3DDDIARG_CREATEDEVICE *args)
{
	HRESULT res;
	ApiName("pfnCreateDevice");
	OutTraceDRV("%s: adapter=%#x\n", ApiRef, adapter);

	res = (*pCreateDevice)(adapter, args);
	if(dxw.dwFlags19 & FORCEWBASEDFOGWDDM){
		if(args->pDeviceFuncs->pfnUpdateWInfo != updateWInfo){
			pUpdateWInfo = args->pDeviceFuncs->pfnUpdateWInfo;
			args->pDeviceFuncs->pfnUpdateWInfo = updateWInfo;
		}
	}

	if(dxw.dwFlags18 & (DEPTHBUFZCLEAN)){
		if(args->pDeviceFuncs->pfnDepthFill != dxwDepthFill){
			pfnDepthFill = args->pDeviceFuncs->pfnDepthFill;
			args->pDeviceFuncs->pfnDepthFill = dxwDepthFill;
		}
	}

	if(dxw.dwFlags18 & FIXCOLORKEY){
		// hook SetTextureStageState
		if(args->pDeviceFuncs->pfnSetTextureStageState != dxwSetTextureStageState){
			OutTraceDRV("%s: hooking SetTextureStageState=%#x->%#x\n", 
				ApiRef, args->pDeviceFuncs->pfnSetTextureStageState, dxwSetTextureStageState);
			pfnSetTextureStageState = args->pDeviceFuncs->pfnSetTextureStageState;
			args->pDeviceFuncs->pfnSetTextureStageState = dxwSetTextureStageState;
		}

		pfnSetRenderState = args->pDeviceFuncs->pfnSetRenderState;
	}

	//if(dxw.dwDFlags2 & EXPERIMENTAL){
	//	// hook SetTexture
	//	if(args->pDeviceFuncs->pfnSetTexture != dxwSetTexture){
	//		OutTraceDRV("%s: hooking SetTexture=%#x->%#x\n", 
	//			ApiRef, args->pDeviceFuncs->pfnSetTexture, dxwSetTexture);
	//		pfnSetTexture = args->pDeviceFuncs->pfnSetTexture;
	//		args->pDeviceFuncs->pfnSetTexture = dxwSetTexture;
	//	}
	//}

	if(dxw.dwFlags18 & (TRIMVERTEXBUFFER | ENABLEZOOMING | ALTPIXELCENTER | FIXCOLORKEY)) {
		// hook DrawPrimitive function
		if(args->pDeviceFuncs->pfnDrawPrimitive != dxwDrawPrimitive){
			pfnDrawPrimitive = args->pDeviceFuncs->pfnDrawPrimitive;
			args->pDeviceFuncs->pfnDrawPrimitive = dxwDrawPrimitive;
		}

		if(args->pDeviceFuncs->pfnDrawPrimitive2 != dxwDrawPrimitive2){
			pfnDrawPrimitive2 = args->pDeviceFuncs->pfnDrawPrimitive2;
			args->pDeviceFuncs->pfnDrawPrimitive2 = dxwDrawPrimitive2;
		}

		if(args->pDeviceFuncs->pfnDrawIndexedPrimitive && args->pDeviceFuncs->pfnDrawIndexedPrimitive != dxwDrawIndexedPrimitive){
			pfnDrawIndexedPrimitive = args->pDeviceFuncs->pfnDrawIndexedPrimitive;
			args->pDeviceFuncs->pfnDrawIndexedPrimitive = dxwDrawIndexedPrimitive;
		}

		if(args->pDeviceFuncs->pfnDrawIndexedPrimitive2 && args->pDeviceFuncs->pfnDrawIndexedPrimitive2 != dxwDrawIndexedPrimitive2){
			pfnDrawIndexedPrimitive2 = args->pDeviceFuncs->pfnDrawIndexedPrimitive2;
			args->pDeviceFuncs->pfnDrawIndexedPrimitive2 = dxwDrawIndexedPrimitive2;
		}

		// hook user mode vertex source
		if(args->pDeviceFuncs->pfnSetStreamSourceUm != dxwSetStreamSourceUm){
			pfnSetStreamSourceUm = args->pDeviceFuncs->pfnSetStreamSourceUm;
			args->pDeviceFuncs->pfnSetStreamSourceUm = dxwSetStreamSourceUm;
		}

		if(args->pDeviceFuncs->pfnSetStreamSource != dxwSetStreamSource){
			pfnSetStreamSource = args->pDeviceFuncs->pfnSetStreamSource;
			args->pDeviceFuncs->pfnSetStreamSource = dxwSetStreamSource;
		}

		if(args->pDeviceFuncs->pfnSetIndices && args->pDeviceFuncs->pfnSetIndices != dxwSetIndices){
			pfnSetIndices = args->pDeviceFuncs->pfnSetIndices;
			args->pDeviceFuncs->pfnSetIndices = dxwSetIndices;
		}

		if(args->pDeviceFuncs->pfnSetIndicesUm && args->pDeviceFuncs->pfnSetIndicesUm != dxwSetIndicesUm){
			pfnSetIndicesUm = args->pDeviceFuncs->pfnSetIndicesUm;
			args->pDeviceFuncs->pfnSetIndicesUm = dxwSetIndicesUm;
		}

		// hook vertex buffers creation
		if(args->pDeviceFuncs->pfnCreateResource && (args->pDeviceFuncs->pfnCreateResource != dxwCreateResource)){
			pfnCreateResource = args->pDeviceFuncs->pfnCreateResource;
			args->pDeviceFuncs->pfnCreateResource = dxwCreateResource;
		}

		if(args->pDeviceFuncs->pfnCreateResource2 && (args->pDeviceFuncs->pfnCreateResource2 != dxwCreateResource2)){
			pfnCreateResource2 = args->pDeviceFuncs->pfnCreateResource2;
			args->pDeviceFuncs->pfnCreateResource2 = dxwCreateResource2;
		}

		if(args->pDeviceFuncs->pfnDestroyResource && (args->pDeviceFuncs->pfnDestroyResource != dxwDestroyResource)){
			pfnDestroyResource = args->pDeviceFuncs->pfnDestroyResource;
			args->pDeviceFuncs->pfnDestroyResource = dxwDestroyResource;
		}

		if(args->pDeviceFuncs->pfnCreateVertexShaderDecl && (args->pDeviceFuncs->pfnCreateVertexShaderDecl != dxwCreateVertexShaderDecl)){
			pfnCreateVertexShaderDecl = args->pDeviceFuncs->pfnCreateVertexShaderDecl;
			args->pDeviceFuncs->pfnCreateVertexShaderDecl = dxwCreateVertexShaderDecl;
		}

		if(args->pDeviceFuncs->pfnDeleteVertexShaderDecl && (args->pDeviceFuncs->pfnDeleteVertexShaderDecl != dxwDeleteVertexShaderDecl)){
			pfnDeleteVertexShaderDecl = args->pDeviceFuncs->pfnDeleteVertexShaderDecl;
			args->pDeviceFuncs->pfnDeleteVertexShaderDecl = dxwDeleteVertexShaderDecl;
		}

		if(args->pDeviceFuncs->pfnSetVertexShaderDecl && (args->pDeviceFuncs->pfnSetVertexShaderDecl != dxwSetVertexShaderDecl)){
			pfnSetVertexShaderDecl = args->pDeviceFuncs->pfnSetVertexShaderDecl;
			args->pDeviceFuncs->pfnSetVertexShaderDecl = dxwSetVertexShaderDecl;
		}

		if(args->pDeviceFuncs->pfnDrawRectPatch && (args->pDeviceFuncs->pfnDrawRectPatch != dxwDrawRectPatch)){
			pfnDrawRectPatch = args->pDeviceFuncs->pfnDrawRectPatch;
			args->pDeviceFuncs->pfnDrawRectPatch = dxwDrawRectPatch;
		}

		if(args->pDeviceFuncs->pfnDrawTriPatch && (args->pDeviceFuncs->pfnDrawTriPatch != dxwDrawTriPatch)){
			pfnDrawTriPatch = args->pDeviceFuncs->pfnDrawTriPatch;
			args->pDeviceFuncs->pfnDrawTriPatch = dxwDrawTriPatch;
		}
	}

	if(args->pDeviceFuncs->pfnClear != dxwClear){
		pfnClear = args->pDeviceFuncs->pfnClear;
		args->pDeviceFuncs->pfnClear = dxwClear;
	}
	//pfnClear = args->pDeviceFuncs->pfnClear;

	OutTraceDRV("%s: res=%#x\n", ApiRef, res);
	return res;
}

typedef HRESULT (WINAPI *PFND3DDDI_GETCAPS)(HANDLE, const D3DDDIARG_GETCAPS *);
PFND3DDDI_GETCAPS pfnGetCaps;

static char *sCapsType(DWORD type)
{
	char *t = "???";
	char *val[] = {
		"DDRAW", // = 1
		"DDRAW_MODE_SPECIFIC", 
		"GETFORMATCOUNT", 
		"GETFORMATDATA", 
		"GETMULTISAMPLEQUALITYLEVELS",
		"GETD3DQUERYCOUNT", 
		"GETD3DQUERYDATA", 
		"GETD3D3CAPS", 
		"GETD3D5CAPS", 
		"GETD3D6CAPS", 
		"GETD3D7CAPS", 
		"GETD3D8CAPS",
		"GETD3D9CAPS", 
		"GETDECODEGUIDCOUNT", 
		"GETDECODEGUIDS",
		"GETDECODERTFORMATCOUNT", 
		"GETDECODERTFORMATS", 
		"GETDECODECOMPRESSEDBUFFERINFOCOUNT", 
		"GETDECODECOMPRESSEDBUFFERINFO", 
		"GETDECODECONFIGURATIONCOUNT", // = 20
		"GETDECODECONFIGURATIONS", 
		"GETVIDEOPROCESSORDEVICEGUIDCOUNT", 
		"GETVIDEOPROCESSORDEVICEGUIDS", 
		"GETVIDEOPROCESSORRTFORMATCOUNT", 
		"GETVIDEOPROCESSORRTFORMATS",
		"GETVIDEOPROCESSORRTSUBSTREAMFORMATCOUNT", 
		"GETVIDEOPROCESSORRTSUBSTREAMFORMATS", 
		"GETVIDEOPROCESSORCAPS", 
		"GETPROCAMPRANGE", 
		"FILTERPROPERTYRANGE", // = 30
		"GETEXTENSIONGUIDCOUNT", 
		"GETEXTENSIONGUIDS", 
		"GETEXTENSIONCAPS", 
		"GETGAMMARAMPCAPS", 
		"CHECKOVERLAYSUPPORT", // = 35
		"DXVAHD_GETVPDEVCAPS", 
		"DXVAHD_GETVPOUTPUTFORMATS", 
		"DXVAHD_GETVPINPUTFORMATS", 
		"DXVAHD_GETVPCAPS", 
		"DXVAHD_GETVPCUSTOMRATES", 
		"DXVAHD_GETVPFILTERRANGE", 
		"GETCONTENTPROTECTIONCAPS", 
		"GETCERTIFICATESIZE", 
		"GETCERTIFICATE", 
		"GET_ARCHITECTURE_INFO", 
		"GET_SHADER_MIN_PRECISION_SUPPORT", 
		"GET_MULTIPLANE_OVERLAY_CAPS", 
		"GET_MULTIPLANE_OVERLAY_FILTER_RANGE", 
		"GET_MULTIPLANE_OVERLAY_GROUP_CAPS", 
		"GET_SIMPLE_INSTANCING_SUPPORT",
		"GET_MARKER_CAPS" // = 51
		};
	if((type > 0) && (type <= D3DDDICAPS_GET_MARKER_CAPS)) t = val[type-1];
	return t;
}

#define OVERLAYLAYERCAPS \
	(DDCAPS_OVERLAY|DDCAPS_OVERLAYCANTCLIP|\
	DDCAPS_OVERLAYFOURCC|DDCAPS_OVERLAYSTRETCH)
#define OVERLAYKEYCAPS \
	(DDCKEYCAPS_DESTOVERLAY|DDCKEYCAPS_DESTOVERLAYYUV|\
	DDCKEYCAPS_SRCOVERLAY|DDCKEYCAPS_SRCOVERLAYYUV|\
	DDCKEYCAPS_SRCOVERLAYCLRSPACE|DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV)

HRESULT WINAPI extGetCaps(HANDLE hAdapter, const D3DDDIARG_GETCAPS *caps)
{
	HRESULT res;
	ApiName("GetCaps");
	OutTraceDRV("%s: hAdapter=%#x caps={size=%d type=%d(%s)}\n", 
		ApiRef, hAdapter, caps->DataSize, caps->Type, sCapsType(caps->Type));

	res = (*pfnGetCaps)(hAdapter, caps);
	if(res){
		OutTraceDRV("%s: res=%#x\n", ApiRef, res);
		return res;
	}

	if(dxw.dwFlags14 & DISABLECOLORKEY){
		if(caps->Type == D3DDDICAPS_DDRAW){
			DDRAW_CAPS *ddCaps = (DDRAW_CAPS *)(caps->pData);
			ddCaps->CKeyCaps &= ~DDRAW_CKEYCAPS_SRCBLT; 
			OutTraceDRV("%s: removed DDRAW_CKEYCAPS_SRCBLT capability\n", ApiRef);
		}
	}
	if(dxw.dwFlags19 & WDDMNOOVERLAY){
		if(caps->Type == D3DDDICAPS_DDRAW){
			DDRAW_CAPS *ddCaps = (DDRAW_CAPS *)(caps->pData);
			//ddCaps->Caps &= ~DDCAPS_OVERLAY; 
			ddCaps->Caps &= ~OVERLAYLAYERCAPS;
			ddCaps->CKeyCaps  &= ~OVERLAYKEYCAPS;
			OutTraceDRV("%s: removed overlay capabilities\n", ApiRef);
		}
		if(caps->Type == D3DDDICAPS_CHECKOVERLAYSUPPORT){
			// D3DDDICAPS_CHECKOVERLAYSUPPORT
			// The driver receives a pointer to a D3DOVERLAYCAPS structure that contains information about the capabilities of a particular overlay. 
			// The attributes of the overlay and the display mode in which the calling application wants to use the overlay are specified in a 
			// DDICHECKOVERLAYSUPPORTINPUT structure that is pointed to by pInfo. 
			// If the driver supports the overlay, the driver sets the members of the D3DOVERLAYCAPS; otherwise, the driver fails the call to its 
			// PFND3DDDI_GETCAPS function with either D3DDDIERR_UNSUPPORTEDOVERLAYFORMAT or D3DDDIERR_UNSUPPORTEDOVERLAY depending on whether the 
			// lack of support was based on the overlay format. D3DOVERLAYCAPS is described in the DirectXSDK documentation.
			OutTraceDRV("%s: removed overlay support\n", ApiRef);
			return D3DDDIERR_UNSUPPORTEDOVERLAY;
		}
	}

	OutTraceDRV("%s: res=%#x\n", ApiRef, res);
	return res;
}

struct D3D9ON12_PRIVATE_DDI_TABLE
{
	LPGENERIC(PFND3D9ON12_OPENADAPTER) pfnOpenAdapter;
	FARPROC pfnGetSharedGDIHandle;
	FARPROC pfnCreateSharedNTHandle;
	FARPROC pfnGetDeviceState;
	LPGENERIC(PFND3D9ON12_KMTPRESENT) pfnKMTPresent;
};

typedef HRESULT (WINAPI *OpenAdapter_Type)(D3DDDIARG_OPENADAPTER *);
OpenAdapter_Type pOpenAdapter;

typedef void (WINAPI *GetPrivateDDITable_Type)(D3D9ON12_PRIVATE_DDI_TABLE *);
GetPrivateDDITable_Type pGetPrivateDDITable;

HRESULT WINAPI extOpenAdapter(D3DDDIARG_OPENADAPTER *args)
{
	HRESULT res; 
	ApiName("OpenAdapter");
	OutTraceDRV("%s: hAdapter=%#x interface=%i version=%d\n", 
		ApiRef, args->hAdapter, args->Interface, args->Version);

	res = (*pOpenAdapter)(args);
	OutTraceDRV("%s: res=%#x hAdapter=%#x DriverVersion=%i\n", 
		ApiRef, res, args->hAdapter, args->DriverVersion);
	
	if(args->pAdapterFuncs->pfnCreateDevice != extCreateDevice){
		pCreateDevice = args->pAdapterFuncs->pfnCreateDevice;
		args->pAdapterFuncs->pfnCreateDevice = extCreateDevice;
	}

	if(args->pAdapterFuncs->pfnGetCaps != extGetCaps){
		pfnGetCaps = args->pAdapterFuncs->pfnGetCaps;
		args->pAdapterFuncs->pfnGetCaps = extGetCaps;
	}

	return res;
}

void WINAPI extGetPrivateDDITable(D3D9ON12_PRIVATE_DDI_TABLE *pPrivateDDITable)
{ 
	ApiName("GetPrivateDDITable");
	OutTraceDRV("%s: pPrivateDDITable=%#x\n", 
		ApiRef, pPrivateDDITable);

	(*pGetPrivateDDITable)(pPrivateDDITable);
	OutTraceDRV("%s: pfnOpenAdapter=%#x pfnGetSharedGDIHandle=%#x pfnGetDeviceState=%#x pfnKMTPresent=%#x\n", 
		ApiRef,
		pPrivateDDITable->pfnOpenAdapter,
		pPrivateDDITable->pfnGetSharedGDIHandle,
		pPrivateDDITable->pfnGetDeviceState,
		pPrivateDDITable->pfnKMTPresent
	);
	
	if(pPrivateDDITable->pfnOpenAdapter != extOpenAdapter){
		pOpenAdapter = (OpenAdapter_Type)pPrivateDDITable->pfnOpenAdapter;
		pPrivateDDITable->pfnOpenAdapter = extOpenAdapter;
	}
}

FARPROC WINAPI DxGetProcAddress(HMODULE hmod, LPCSTR lpProcName)
{
	ApiName("GetProcAddress");
	OutTraceDRV("%s: GetProcAddress(hmod=%#x procname=%s)\n", ApiRef, hmod, lpProcName);
	if(!strcmp("OpenAdapter", lpProcName)){
		pOpenAdapter = (OpenAdapter_Type)(*pGetProcAddress)(hmod, lpProcName);
		return (FARPROC)extOpenAdapter;
	}
	if(!strcmp("GetPrivateDDITable", lpProcName)){
		pGetPrivateDDITable = (GetPrivateDDITable_Type)(*pGetProcAddress)(hmod, lpProcName);
		return (FARPROC)extGetPrivateDDITable;
	}
	return (*pGetProcAddress)(hmod, lpProcName);
}

#ifndef D3DDDIFORMAT 
#define D3DDDIFORMAT DWORD
#endif
#ifndef D3DKMT_CREATEDCFROMMEMORY
typedef struct _D3DKMT_CREATEDCFROMMEMORY
{
    VOID*                           pMemory;       // in: memory for DC
    D3DDDIFORMAT                    Format;        // in: Memory pixel format
    UINT                            Width;         // in: Memory Width
    UINT                            Height;        // in: Memory Height
    UINT                            Pitch;         // in: Memory pitch
    HDC                             hDeviceDc;     // in: DC describing the device
    PALETTEENTRY*                   pColorTable;   // in: Palette
    HDC                             hDc;           // out: HDC
    HANDLE                          hBitmap;       // out: Handle to bitmap
} D3DKMT_CREATEDCFROMMEMORY;
#define D3DDDIFMT_P8 41
#endif 

typedef DWORD (WINAPI *CreateDCFromMemory_Type)(D3DKMT_CREATEDCFROMMEMORY *);
CreateDCFromMemory_Type pCreateDCFromMemory = 0;

PALETTEENTRY palette[256];

long WINAPI DxCreateDCFromMemory(D3DKMT_CREATEDCFROMMEMORY *pData)
{
	DWORD ret;
	ApiName("CreateDCFromMemory");
	extern long D3DKMTCreateDCFromMemory(D3DKMT_CREATEDCFROMMEMORY *);
	PALETTEENTRY *origColorTable = pData->pColorTable;
	DWORD origFormat = pData->Format;
	if(pCreateDCFromMemory == 0){
		HINSTANCE hinst=(*pLoadLibraryA)("gdi32.dll");
		if(!hinst){
			OutTraceDRV("%s: LoadLibrary ddraw.dll ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
			return 0;
		}
		pCreateDCFromMemory = (CreateDCFromMemory_Type)(*pGetProcAddress)(hinst, "D3DKMTCreateDCFromMemory");
		(*pFreeLibrary)(hinst);
	}

	if(!pCreateDCFromMemory) return 0;
	if(D3DDDIFMT_P8 == pData->Format){
		OutTraceDRV("%s: 8bit DC\n", ApiRef);
		extern DWORD PaletteEntries[256];
		for(int i=0; i<256; i++){
			palette[i].peBlue = (PaletteEntries[i] & 0x0000FF);
			palette[i].peGreen = (PaletteEntries[i] & 0x0FF00) >> 8;
			palette[i].peRed = (byte)((PaletteEntries[i] & 0xFF0000) >> 16);
			palette[i].peFlags = 0;
		}
		pData->pColorTable = palette;
	}
	ret = (*pCreateDCFromMemory)(pData);
	pData->pColorTable = origColorTable;
	pData->Format = origFormat;
	return ret;
}

PIMAGE_NT_HEADERS getImageNtHeaders(HMODULE module)
{
	PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	if (IMAGE_DOS_SIGNATURE != dosHeader->e_magic) return NULL;

	PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<char*>(dosHeader) + dosHeader->e_lfanew);
	if (IMAGE_NT_SIGNATURE != ntHeaders->Signature) return NULL;

	return ntHeaders;
}

FARPROC* findProcAddressInIat(HMODULE module, const char* procName)
{
	if (!module || !procName) return NULL;

	PIMAGE_NT_HEADERS ntHeaders = getImageNtHeaders(module);
	if (!ntHeaders) return NULL;

	char* moduleBase = reinterpret_cast<char*>(module);
	PIMAGE_IMPORT_DESCRIPTOR importDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase +
		ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (PIMAGE_IMPORT_DESCRIPTOR desc = importDesc;
		0 != desc->Characteristics && 0xFFFF != desc->Name;
		++desc){
		PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(moduleBase + desc->FirstThunk);
		PIMAGE_THUNK_DATA origThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(moduleBase + desc->OriginalFirstThunk);
		while (0 != thunk->u1.AddressOfData && 0 != origThunk->u1.AddressOfData){
			PIMAGE_IMPORT_BY_NAME origImport = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
				moduleBase + origThunk->u1.AddressOfData);

			if (0 == strcmp((char *)origImport->Name, procName)){
				return reinterpret_cast<FARPROC*>(&thunk->u1.Function);
			}

			++thunk;
			++origThunk;
		}
	}

	return NULL;
}

void hookIatFunction(HMODULE module, const char* funcName, void* newFuncPtr)
{
	FARPROC *func = findProcAddressInIat(module, funcName);
	if (func){
		OutTraceDRV("Hooking function via IAT: %s (%#x)\n", funcName, (DWORD)(*func));;
		DWORD oldProtect = 0;
		VirtualProtect(func, sizeof(func), PAGE_READWRITE, &oldProtect);
		*func = static_cast<FARPROC>(newFuncPtr);
		DWORD dummy = 0;
		VirtualProtect(func, sizeof(func), oldProtect, &dummy);
	}
}

void ddHookVideoAdapter()
{
	HINSTANCE hinst;
	ApiName("ddHookVideoAdapter");

	hinst=(*pLoadLibraryA)("ddraw.dll");
	if(!hinst){
		OutTraceDRV("%s: LoadLibrary ddraw.dll ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return;
	}
	OutTraceDRV("%s: hinst=%#x\n", ApiRef, hinst);
	hookIatFunction(hinst, "GetProcAddress", DxGetProcAddress);
	(*pFreeLibrary)(hinst);
}

void ddFixCreatedDC()
{
	HINSTANCE hinst;
	ApiName("ddFixCreatedDC");

	hinst=(*pLoadLibraryA)("ddraw.dll");
	if(!hinst){
		OutTraceDRV("%s: LoadLibrary ddraw.dll ERROR err=%d @%d\n", ApiRef, GetLastError(), __LINE__);
		return;
	}
	OutTraceDRV("%s: hinst=%#x\n", ApiRef, hinst);
	hookIatFunction(hinst, "D3DKMTCreateDCFromMemory", DxCreateDCFromMemory);
	(*pFreeLibrary)(hinst);
}
