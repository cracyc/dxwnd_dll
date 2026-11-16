#define _CRT_SECURE_NO_WARNINGS

#define _COMPONENT "dxwDirect3DQuery9"

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"

#include "D3DQuery9.wrap.hpp"

HRESULT STDMETHODCALLTYPE dxwDirect3DQuery9::QueryInterface(const IID &riid, void **obp)
{
	ApiName("QueryInterface");
	OutTraceD3D("%s\n", ApiRef);
	OutTraceD3D("%s: obj=%#x REFIID=%s(%s)\n", ApiRef, m_this, sRIID(riid), ExplainGUID((GUID *)&riid));
	return obp ? E_NOINTERFACE : E_POINTER;
}

ULONG STDMETHODCALLTYPE dxwDirect3DQuery9::AddRef()
{
	ApiName("AddRef");
	ULONG res = 0;
	OutTraceD3D("%s\n", ApiRef);
	res = ++m_ref;
	OutTraceD3D("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

ULONG STDMETHODCALLTYPE dxwDirect3DQuery9::Release()
{
	ApiName("Release");
	ULONG res = 0;
	OutTraceD3D("%s\n", ApiRef);
	res = --m_ref;
	OutTraceD3D("%s: obj=%#x res=%d\n", ApiRef, m_this, res);
	return res;
}

HRESULT STDMETHODCALLTYPE dxwDirect3DQuery9::GetDevice(IDirect3DDevice9** ppDevice)
{
	ApiName("GetDevice");
	OutTraceD3D("%s: pQ=%#x ppDevice=%#x\n", ApiRef, m_this, ppDevice);
	*ppDevice = m_Device;
	return D3D_OK;
}

D3DQUERYTYPE STDMETHODCALLTYPE dxwDirect3DQuery9::GetType()
{
	ApiName("GetType");
	OutTraceD3D("%s: pQ=%#x type=%d\n", ApiRef, m_this, Type);
	return Type;
}

DWORD STDMETHODCALLTYPE dxwDirect3DQuery9::GetDataSize()
{
	ApiName("GetDataSize");
	DWORD res = 0;
	OutTraceD3D("%s\n", ApiRef);
	switch(Type){
		case D3DQUERYTYPE_EVENT: res = 2; break;
	}
	OutTraceD3D("%s: pQ=%#x type=%d res(size)=%d\n", ApiRef, m_this, Type, res);
	return Type;
}

HRESULT STDMETHODCALLTYPE dxwDirect3DQuery9::Issue(DWORD dwIssueFlags)
{
	ApiName("Issue");
	char *sFlags = "";
	if(Type == D3DQUERYTYPE_OCCLUSION) sFlags = dwIssueFlags == 1 ? "END" : (dwIssueFlags == 2 ? "BEGIN" : "???");
	OutTraceD3D("%s: pQ=%#x flags=%#x(%s)\n", ApiRef, m_this, dwIssueFlags, sFlags);
	return D3D_OK;
}

HRESULT STDMETHODCALLTYPE dxwDirect3DQuery9::GetData(void* pData, DWORD dwSize, DWORD dwGetDataFlags)
{
	ApiName("GetData");
	OutTraceD3D("%s: pQ=%#x pdata=%#x size=%d flags=%#x(%s)\n", ApiRef, m_this, pData, dwSize, dwGetDataFlags,  
		dwGetDataFlags == D3DGETDATA_FLUSH ? "FLUSH" : (dwGetDataFlags == 0 ? "NULL" : "???"));
	if(dwSize) memset(pData, 0, dwSize);
	return D3D_OK;
} 

void STDMETHODCALLTYPE dxwDirect3DQuery9::Initialize(D3DQUERYTYPE type, DWORD obj, IDirect3DDevice9* device)
{
	ApiName("Initialize");
	OutTraceD3D("%s: new obj pQ=%#x type=%d\n", ApiRef, this, type);
	Type = type;
	m_this = obj;
	m_ref = 1;
	m_Device = device;
}
