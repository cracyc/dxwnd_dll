#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "dplay.h"
//#include "dplayex.h"

#define DPLAYXBYPASS TRUE

 // statics

char *ExplainDPLayErr(HRESULT res)
{
	return "tbd";
}

// api
typedef HRESULT (WINAPI *DirectPlayCreate_Type)(LPGUID, LPDIRECTPLAY FAR *, IUnknown FAR *);
HRESULT WINAPI extDirectPlayCreate(LPGUID, LPDIRECTPLAY FAR *, IUnknown FAR *);
DirectPlayCreate_Type pDirectPlayCreate;

typedef HRESULT (WINAPI *DirectPlayEnumerate_Type)(LPDPENUMDPCALLBACK, LPVOID);
HRESULT WINAPI extDirectPlayEnumerate(LPDPENUMDPCALLBACK, LPVOID);
DirectPlayEnumerate_Type pDirectPlayEnumerate;

// dplay session
HRESULT WINAPI extDPXAddPlayerToGroup(void *, DPID, DPID);
HRESULT WINAPI extDPXClose(void *);
HRESULT WINAPI extDPXCreatePlayer(void *, LPDPID, LPSTR, LPSTR, LPHANDLE);
HRESULT WINAPI extDPXCreateGroup(void *, LPDPID, LPSTR, LPVOID, DWORD, DWORD);

static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "DirectPlayCreate", (FARPROC)NULL, (FARPROC *)&pDirectPlayCreate, (FARPROC)extDirectPlayCreate},
	{HOOK_IAT_CANDIDATE, 0, "DirectPlayEnumerate", (FARPROC)NULL, (FARPROC *)&pDirectPlayEnumerate, (FARPROC)extDirectPlayEnumerate},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookDPlayX(HMODULE module)
{
	HookLibraryEx(module, Hooks, "dplayx.dll");
}

FARPROC Remap_DPlayX_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	return NULL;
}

HRESULT WINAPI extDirectPlayCreate(LPGUID lpGUID, LPDIRECTPLAY FAR *lplpDP, IUnknown FAR *pUnk)
{
	ApiName("dplayx.DirectPlayCreate");
	OutTrace("%s: guid=%s\n", ApiRef, sGUID(lpGUID));
	return (*pDirectPlayCreate)(lpGUID, lplpDP, pUnk);
}

HRESULT WINAPI extDirectPlayEnumerate(LPDPENUMDPCALLBACK cbProc, LPVOID arg)
{
	ApiName("dplayx.DirectPlayEnumerate");
	OutTrace("%s: proc=%#x arg=%#x\n", ApiRef, cbProc, arg);
	return (*pDirectPlayEnumerate)(cbProc, arg);
}


#if 0
     STDMETHOD(AddPlayerToGroup)(THIS_ DPID idGroup, DPID idPlayer) PURE;
     STDMETHOD(Close)(THIS) PURE;
     STDMETHOD(CreateGroup)(THIS_ LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     STDMETHOD(CreatePlayer)(THIS_ LPDPID lpidPlayer, LPDPNAME lpPlayerName, HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     STDMETHOD(DeletePlayerFromGroup)(THIS_ DPID idGroup, DPID idPlayer) PURE;
     STDMETHOD(DestroyGroup)(THIS_ DPID idGroup) PURE;
     STDMETHOD(DestroyPlayer)(THIS_ DPID idPlayer) PURE;
     STDMETHOD(EnumGroupPlayers)(THIS_ DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
     STDMETHOD(EnumGroups)(THIS_ LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
     STDMETHOD(EnumPlayers)(THIS_ LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
     STDMETHOD(EnumSessions)(THIS_ LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
     STDMETHOD(GetCaps)(THIS_ LPDPCAPS lpDPCaps, DWORD dwFlags) PURE;
     STDMETHOD(GetGroupData)(THIS_ DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags) PURE;
     STDMETHOD(GetGroupName)(THIS_ DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     STDMETHOD(GetMessageCount)(THIS_ DPID idPlayer, LPDWORD lpdwCount) PURE;
     STDMETHOD(GetPlayerAddress)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     STDMETHOD(GetPlayerCaps)(THIS_ DPID idPlayer, LPDPCAPS lpPlayerCaps, DWORD dwFlags) PURE;
     STDMETHOD(GetPlayerData)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags) PURE;
     STDMETHOD(GetPlayerName)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     STDMETHOD(GetSessionDesc)(THIS_ LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     STDMETHOD(Initialize)(THIS_ LPGUID lpGUID) PURE;
     STDMETHOD(Open)(THIS_ LPDPSESSIONDESC2 lpsd, DWORD dwFlags) PURE;
     STDMETHOD(Receive)(THIS_ LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     STDMETHOD(Send)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize) PURE;
     STDMETHOD(SetGroupData)(THIS_ DPID idGroup, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     STDMETHOD(SetGroupName)(THIS_ DPID idGroup, LPDPNAME lpGroupName, DWORD dwFlags) PURE;
     STDMETHOD(SetPlayerData)(THIS_ DPID idPlayer, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     STDMETHOD(SetPlayerName)(THIS_ DPID idPlayer, LPDPNAME lpPlayerName, DWORD dwFlags) PURE;
     STDMETHOD(SetSessionDesc)(THIS_ LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags) PURE;
     /*** IDirectPlay3 methods ***/
     STDMETHOD(AddGroupToGroup)(THIS_ DPID idParentGroup, DPID idGroup) PURE;
     STDMETHOD(CreateGroupInGroup)(THIS_ DPID idParentGroup, LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     STDMETHOD(DeleteGroupFromGroup)(THIS_ DPID idParentGroup, DPID idGroup) PURE;
     STDMETHOD(EnumConnections)(THIS_ LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags) PURE;
     STDMETHOD(EnumGroupsInGroup)(THIS_ DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumCallback, LPVOID lpContext, DWORD dwFlags) PURE;
     STDMETHOD(GetGroupConnectionSettings)(THIS_ DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     STDMETHOD(InitializeConnection)(THIS_ LPVOID lpConnection, DWORD dwFlags) PURE;
     STDMETHOD(SecureOpen)(THIS_ LPCDPSESSIONDESC2 lpsd, DWORD dwFlags, LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials) PURE;
     STDMETHOD(SendChatMessage)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPDPCHAT lpChatMessage) PURE;
     STDMETHOD(SetGroupConnectionSettings)(THIS_ DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConnection) PURE;
     STDMETHOD(StartSession)(THIS_ DWORD dwFlags, DPID idGroup) PURE;
     STDMETHOD(GetGroupFlags)(THIS_ DPID idGroup, LPDWORD lpdwFlags) PURE;
     STDMETHOD(GetGroupParent)(THIS_ DPID idGroup, LPDPID lpidParent) PURE;
     STDMETHOD(GetPlayerAccount)(THIS_ DPID idPlayer, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     STDMETHOD(GetPlayerFlags)(THIS_ DPID idPlayer, LPDWORD lpdwFlags) PURE;
#endif

HRESULT (WINAPI *pDPXAddPlayerToGroup)(void *, DPID, DPID);
HRESULT (WINAPI *pDPXClose)(void *);
HRESULT (WINAPI *pDPXCreatePlayer)(void *, LPDPID, LPSTR, LPSTR, LPHANDLE);
HRESULT (WINAPI *pDPXCreateGroup)(void *, LPDPID, LPSTR, LPVOID, DWORD, DWORD);
HRESULT (WINAPI *pDPXEnumGroupPlayers)(void *, DPID, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD);
HRESULT (WINAPI *pDPXEnumGroups)(void *, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD);
HRESULT (WINAPI *pDPXEnumPlayers)(void *, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD);
HRESULT (WINAPI *pDPXEnumSessions)(void *, LPDPSESSIONDESC2, DWORD, LPDPENUMSESSIONSCALLBACK2, LPVOID, DWORD);
HRESULT (WINAPI *pDPXGetCaps)(void *, LPDPCAPS, DWORD);
HRESULT (WINAPI *pDPXInitialize)(void *, LPGUID);
HRESULT (WINAPI *pDPXEnumConnections)(void *, LPCGUID, LPDPENUMCONNECTIONSCALLBACK, LPVOID, DWORD);

// methods
HRESULT WINAPI extDPXAddPlayerToGroup(void *dpx, DPID dp1, DPID dp2)
{ STEP; OutTrace("IDirectPlay::AddPlayerToGroup: dpx=%#x dp1=%#x dp2=%#x\n", dpx, dp1, dp2); return 0; }
HRESULT WINAPI extDPXClose(void *dpx)
{ STEP; OutTrace("IDirectPlay::Close: dpx=%#x\n", dpx); return 0; }
HRESULT WINAPI extDPXCreatePlayer(void *dpx, LPDPID lpDpid, LPSTR str1, LPSTR str2, LPHANDLE hdl)
{ STEP; OutTrace("IDirectPlay::CreatePlayer: dpx=%#x str1=%s str2=%s\n", dpx, str1, str2); return 0; }
HRESULT WINAPI extDPXCreateGroup(void *dpx, LPDPID lpidGroup, LPSTR lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags)
{ STEP; OutTrace("IDirectPlay::CreateGroup: dpx=%#x groupname=%s\n", dpx, lpGroupName); return 0; }
HRESULT WINAPI extDPXEnumGroupPlayers(void *dpx, DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags)
{ STEP; OutTrace("IDirectPlay::EnumGroupPlayers: dpx=%#x\n", dpx); return 0; }
HRESULT WINAPI extDPXEnumGroups(void *dpx, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags)
{ STEP; OutTrace("IDirectPlay::EnumGroups: dpx=%#x\n", dpx); return 0; }
HRESULT WINAPI extDPXEnumPlayers(void *dpx, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags)
{ STEP; OutTrace("IDirectPlay::EnumPlayers: dpx=%#x\n", dpx); return 0; }
HRESULT WINAPI extDPXEnumSessions(void *dpx, LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2, LPVOID lpContext, DWORD dwFlags)
{ STEP; OutTrace("IDirectPlay::EnumSessions: dpx=%#x\n", dpx); return 0; }
HRESULT WINAPI extDPXGetCaps(void *dpx, LPDPCAPS lpDPCaps, DWORD dwFlags)
{ STEP; OutTrace("IDirectPlay::GetCaps: dpx=%#x\n", dpx); return 0; }
HRESULT WINAPI extDPXInitialize(void *dpx, LPGUID lpGUID)
{ STEP; OutTrace("IDirectPlay::Initialize: dpx=%#x\n", dpx); return 0; }

HRESULT WINAPI extDPXEnumConnections(void *dpx, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags)
{ 
	HRESULT res;
	ApiName("IDirectPlay::EnumConnections");
	OutTrace("%s: dpx=%#x\n", ApiRef, dpx); 
	if(DPLAYXBYPASS) return S_OK;
	res = (*pDPXEnumConnections)(dpx, lpguidApplication, lpEnumCallback, lpContext, dwFlags);
	if(res) OutTraceE("%s: ERROR res=%#x(%s)\n", ApiRef, res, ExplainDPLayErr(res));
	return res; 
}

void HookDirectPlay(REFIID riid, void *lplpdx)
{
	ApiName("dxwnd.HookDirectPlay");
	OutTrace("%s: riid=%#x lplpdx=%#x\n", ApiRef, riid, lplpdx);
	int version = 0;

	switch (*(DWORD *)&riid){
		case 0x5454e9a0: // IID_DirectPLay
			OutTrace("%s: version=1\n", ApiRef);
			SetHook((void *)(**(DWORD **)lplpdx + 12), extDPXAddPlayerToGroup, (void **)&pDPXAddPlayerToGroup, "AddPlayerToGroup");
			//SetHook((void *)(**(DWORD **)lplpdx + 16), extDPXClose, (void **)&pDPXClose, "Close");
			//SetHook((void *)(**(DWORD **)lplpdx + 20), extDPXCreatePlayer, (void **)&pDPXCreatePlayer, "CreatePlayer");
			//SetHook((void *)(**(DWORD **)lplpdx + 12), extDPXAddPlayerToGroup, (void **)NULL, "AddPlayerToGroup");
			SetHook((void *)(**(DWORD **)lplpdx + 16), extDPXClose, (void **)NULL, "Close");
			SetHook((void *)(**(DWORD **)lplpdx + 20), extDPXCreatePlayer, (void **)NULL, "CreatePlayer");
			SetHook((void *)(**(DWORD **)lplpdx + 24), extDPXCreateGroup, (void **)NULL, "CreateGroup");
			break;
		case 0x2b74f7c0: // IID_DirectPLay2
		case 0x9d460580: // IID_DirectPLay2A
			OutTrace("%s: version=2\n", ApiRef);
			break;
		case 0x133efe40: // IID_DirectPLay3
		case 0x133efe41: // IID_DirectPLay3A
			OutTrace("%s: version=3\n", ApiRef);
			SetHook((void *)(**(DWORD **)lplpdx + 12), extDPXAddPlayerToGroup, (void **)&pDPXAddPlayerToGroup, "AddPlayerToGroup");
			//SetHook((void *)(**(DWORD **)lplpdx + 12), extDPXAddPlayerToGroup, (void **)NULL, "AddPlayerToGroup");
			SetHook((void *)(**(DWORD **)lplpdx + 16), extDPXClose, (void **)&pDPXClose, "Close");
			SetHook((void *)(**(DWORD **)lplpdx + 20), extDPXCreateGroup, (void **)&pDPXCreateGroup, "CreateGroup");
			SetHook((void *)(**(DWORD **)lplpdx + 24), extDPXCreatePlayer, (void **)&pDPXCreatePlayer, "CreatePlayer");
     //STDMETHOD(DeletePlayerFromGroup)(THIS_ DPID idGroup, DPID idPlayer) PURE;
     //STDMETHOD(DestroyGroup)(THIS_ DPID idGroup) PURE;
     //STDMETHOD(DestroyPlayer)(THIS_ DPID idPlayer) PURE;
			SetHook((void *)(**(DWORD **)lplpdx + 40), extDPXEnumGroupPlayers, (void **)&pDPXEnumGroupPlayers, "EnumGroupPlayers");
			SetHook((void *)(**(DWORD **)lplpdx + 44), extDPXEnumGroups, (void **)&pDPXEnumGroups, "EnumGroups");
			SetHook((void *)(**(DWORD **)lplpdx + 48), extDPXEnumPlayers, (void **)&pDPXEnumPlayers, "EnumPlayers");
     //STDMETHOD(EnumSessions)(THIS_ LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
			SetHook((void *)(**(DWORD **)lplpdx + 52), extDPXEnumSessions, (void **)&pDPXEnumSessions, "EnumSessions");
     //STDMETHOD(GetCaps)(THIS_ LPDPCAPS lpDPCaps, DWORD dwFlags) PURE;
			SetHook((void *)(**(DWORD **)lplpdx + 56), extDPXGetCaps, (void **)&pDPXGetCaps, "GetCaps");
     //60 STDMETHOD(GetGroupData)(THIS_ DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags) PURE;
     //64 STDMETHOD(GetGroupName)(THIS_ DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     //68 STDMETHOD(GetMessageCount)(THIS_ DPID idPlayer, LPDWORD lpdwCount) PURE;
     //72 STDMETHOD(GetPlayerAddress)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     //76 STDMETHOD(GetPlayerCaps)(THIS_ DPID idPlayer, LPDPCAPS lpPlayerCaps, DWORD dwFlags) PURE;
     //80 STDMETHOD(GetPlayerData)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags) PURE;
     //84 STDMETHOD(GetPlayerName)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     //88 STDMETHOD(GetSessionDesc)(THIS_ LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     //92 STDMETHOD(Initialize)(THIS_ LPGUID lpGUID) PURE;
			SetHook((void *)(**(DWORD **)lplpdx + 92), extDPXInitialize, (void **)&pDPXInitialize, "Initialize");
     //96 STDMETHOD(Open)(THIS_ LPDPSESSIONDESC2 lpsd, DWORD dwFlags) PURE;
     //100 STDMETHOD(Receive)(THIS_ LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     //104 STDMETHOD(Send)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize) PURE;
     //108 STDMETHOD(SetGroupData)(THIS_ DPID idGroup, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     //112 STDMETHOD(SetGroupName)(THIS_ DPID idGroup, LPDPNAME lpGroupName, DWORD dwFlags) PURE;
     //116 STDMETHOD(SetPlayerData)(THIS_ DPID idPlayer, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     //120 STDMETHOD(SetPlayerName)(THIS_ DPID idPlayer, LPDPNAME lpPlayerName, DWORD dwFlags) PURE;
     //124 STDMETHOD(SetSessionDesc)(THIS_ LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags) PURE;
     ///*** IDirectPlay3 methods ***/
     //128 STDMETHOD(AddGroupToGroup)(THIS_ DPID idParentGroup, DPID idGroup) PURE;
     //132 STDMETHOD(CreateGroupInGroup)(THIS_ DPID idParentGroup, LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
     //136 STDMETHOD(DeleteGroupFromGroup)(THIS_ DPID idParentGroup, DPID idGroup) PURE;
			SetHook((void *)(**(DWORD **)lplpdx + 140), extDPXEnumConnections, (void **)&pDPXEnumConnections, "EnumConnections");
     //STDMETHOD(EnumGroupsInGroup)(THIS_ DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumCallback, LPVOID lpContext, DWORD dwFlags) PURE;
     //STDMETHOD(GetGroupConnectionSettings)(THIS_ DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     //STDMETHOD(InitializeConnection)(THIS_ LPVOID lpConnection, DWORD dwFlags) PURE;
     //STDMETHOD(SecureOpen)(THIS_ LPCDPSESSIONDESC2 lpsd, DWORD dwFlags, LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials) PURE;
     //STDMETHOD(SendChatMessage)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPDPCHAT lpChatMessage) PURE;
     //STDMETHOD(SetGroupConnectionSettings)(THIS_ DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConnection) PURE;
     //STDMETHOD(StartSession)(THIS_ DWORD dwFlags, DPID idGroup) PURE;
     //STDMETHOD(GetGroupFlags)(THIS_ DPID idGroup, LPDWORD lpdwFlags) PURE;
     //STDMETHOD(GetGroupParent)(THIS_ DPID idGroup, LPDPID lpidParent) PURE;
     //STDMETHOD(GetPlayerAccount)(THIS_ DPID idPlayer, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
     //STDMETHOD(GetPlayerFlags)(THIS_ DPID idPlayer, LPDWORD lpdwFlags) PURE;			
			break;
		case 0xab1c530: // IID_DirectPLay4
		case 0xab1c531: // IID_DirectPLay4A
			OutTrace("%s: version=4\n", ApiRef);
			break;
		default:
			OutTrace("%s: unmanaged riid=%#x\n", ApiRef, *(DWORD *)&riid);
			break;
	}
}
