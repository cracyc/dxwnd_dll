// TabTweaks.cpp : implementation file
//

#include "stdafx.h"
#include <afxtempl.h>
#include "dxwndhost.h"
#include "TargetDlg.h"
#include "TabTweaks.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabTweaks dialog

TweakEntry_Type *gAvailableTweaks;

CTabTweaks::CTabTweaks(CWnd* pParent /*=NULL*/)
	: CDialog(CTabTweaks::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabTweaks)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabTweaks::PreTranslateMessage(MSG *pMsg)
{
	//if(((pMsg->message == WM_KEYDOWN) || (pMsg->message == WM_KEYUP))
	if((pMsg->message == WM_KEYDOWN) 
		&& (pMsg->wParam == VK_RETURN)) {
			return TRUE;
	}
    // Added to enable keyboard navigation
    if(IsDialogMessage(pMsg) && (pMsg->wParam != VK_ESCAPE)) {
        return TRUE;
    }
	return CWnd::PreTranslateMessage(pMsg);
}

void CTabTweaks::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

#define offset(x) &((CTargetDlg *)NULL)->x

BOOL CTabTweaks::OnInitDialog()
{
	OutTrace("CTabTweaks::OnInitDialog\n");
	CListBox *ListAvail, *ListActive;
	CButton *pBtn;
	HICON hIcn;
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	int i;
	static TweakEntry_Type AvailableTweaks[] = {
		{ "FIXD3DFRAME",		"d3d:FixD3DWindowFrame", 0, offset(m_FixD3DFrame) },
		{ "NOWINDOWMOVE",		"d3d:NoD3DWindowMove", 0, offset(m_NoWindowMove) },
		{ "BACKBUFATTACH",		"ddraw:MakeBackbufferAttachable", 0, offset(m_BackBufAttach) },
		{ "BLITFROMBACKBUFFER", "ddraw:BlitFromBackbuffer", 0, offset(m_BlitFromBackBuffer) },
		{ "NOSYSMEMPRIMARY",	"ddraw:NoSystemMemoryOnPrimary", 0, offset(m_NoSysMemPrimary) },
		{ "NOSYSMEMBACKBUF",	"ddraw:NoSystemMemoryOnBackbuffer", 0, offset(m_NoSysMemBackBuf) },
		{ "ALLOWSYSMEMON3DDEV",	"ddraw:AllowSysMemoryOn3DDevices", 0, offset(m_AllowSysmemOn3DDev) },
		{ "RETURNNULLREF",		"ddraw:ReturnNULLReference", 0, offset(m_ReturnNullRef) },
		{ "FORCESHEL",			"ddraw:ForcesHEL", 0, offset(m_ForcesHEL) },
		{ "CACHED3DSESSION",	"d3d:CacheD3DSession", 0, offset(m_CacheD3DSession) },
		//{ "LOCKCOLORDEPTH",		"#ddraw:LockColorDepth", 0, offset(m_LockColorDepth) }, // obsolete
		{ "FIXPARENTWIN",		"hook:FixParentWindow", 0, offset(m_FixParentWin) },
		{ "MOUSEMOVEBYEVENT",	"input:MouseMoveByEvent", 0, offset(m_MouseMoveByEvent) },
		{ "NODESTROYWINDOW",	"win:NoMainWindowDestruction", 0, offset(m_NoDestroyWindow) },
		{ "ACTIVATEAPP",		"win:SendWM_ACTIVATEAPPMessage", 0, offset(m_ActivateApp) },
		{ "D3DRESOLUTIONHACK",	"d3d:LegacyD3DResolutionHack", 0, offset(m_D3DResolutionHack) },
		{ "FIXAILSOUNDLOCKS",	"sound:FixAILSoundLocks", 0, offset(m_FixAILSoundLocks) },
		{ "SUPPRESSIME",		"win:SuppressIME", 0, offset(m_SuppressIME) },
		{ "QUARTERBLT",			"win:FPSCountUpdatesBigger1/4Screen", 0, offset(m_QuarterBlt) },
		{ "MAKEWINVISIBLE",		"win:MakeQueriedWindowVisible", 0, offset(m_MakeWinVisible) },
		{ "FIXEMPIREOFS",		"patch:FixEmpireOfTheFadingSuns", 0, offset(m_FixEmpireOFS) },
		{ "KILLBLACKWIN",		"win:KillBlackWindows", 0, offset(m_KillBlackWin) },
		{ "ZERODISPLAYCOUNTER",	"cursor:ZeroDisplayCounter", 0, offset(m_ZeroDisplayCounter) },
		{ "REPLACEDIALOGS",		"win:ReplaceDialogs", 0, offset(m_ReplaceDialogs) },
		{ "HANDLEFOURCC",		"ddraw:ManageFourCCSurfaces", 0, offset(m_HandleFourCC) },
		{ "LOCKFPSCORNER",		"win:LockFPSCorner", 0, offset(m_LockFPSCorner) },
		{ "SETZBUFFER16BIT",	"ddraw:Forces16BitZBufferBitDepth", 0, offset(m_SetZBuffer16Bit) },
		{ "SETZBUFFER24BIT",	"ddraw:Forces24BitZBufferBitDepth", 0, offset(m_SetZBuffer24Bit) },
		//{ "LOCK24BITDEPTH",		"#ddraw:Lock24BitDepth", 0, offset(m_Lock24BitDepth) }, // obsolete
		{ "FULLPAINTRECT",		"win:FullPaintRect", 0, offset(m_FullPaintRect) },
		{ "PUSHACTIVEMOVIE",	"win:PushActiveMovieWindows", 0, offset(m_PushActiveMovie) },
		{ "FORCECLIPCHILDREN",	"win:ForceClipChildren", 0, offset(m_ForceClipChildren) },
		{ "PREVENTMINIMIZE",	"win:PreventMinimizeWindow", 0, offset(m_PreventMinimize) },
		{ "NOACCESSIBILITY",	"input:NoAccessibilityKeys", 0, offset(m_NoAccessibility) },
		{ "IGNOREDEBOUTPUT",	"shim:IgnoreDebugOutput", 0, offset(m_IgnoreDebOutput) },
		{ "NOOLEINITIALIZE",	"patch:NoOleInitialize", 0, offset(m_NoOleInitialize) },
		{ "CHAOSOVERLORDSFIX",	"patch:ChaosOverlordsFix", 0, offset(m_ChaosOverlordsFix) },
		{ "FIXFOLDERPATHS",		"shim:FixFolderPaths", 0, offset(m_FixFolderPaths) },
		{ "NOCOMPLEXMIPMAPS",	"ddraw:NoComplexMipmaps", 0, offset(m_NoComplexMipmaps) },
		{ "INVALIDATECLIENT",	"ddraw:InvalidateClient", 0, offset(m_InvalidateClient) },
		{ "CREATEDCHOOK",		"hook:CreateDCHook", 0, offset(m_CreateDCHook) },
		{ "SAFEPRIMLOCK",		"ddraw:SafePrimaryLock", 0, offset(m_SafePrimLock) },
		{ "D3D8MAXWINMODE",		"d3d:D3D8MaximizeWinModeHack", 0, offset(m_D3D8MaxWinMode) },
		{ "MUTEX4CRITSECTION",	"shim:MutexForCriticalSection", 0, offset(m_Mutex4CritSection) },
		{ "DELAYCRITSECTION",	"shim:DelayCriticalSection", 0, offset(m_DelayCritSection) },
		{ "REMAPNUMKEYPAD",		"input:RemapNumericKeypad", 0, offset(m_RemapNumKeypad) },
		{ "SETUSKEYDESCR",		"input:SetUSKeyDescr", 0, offset(m_SetUSKeyDescr) },
		{ "FORCESHAL",			"ddraw:ForcesHAL", 0, offset(m_ForcesHAL) },
		{ "FORCESNULL",			"ddraw:ForcesNULL", 0, offset(m_ForcesNULL) },
		{ "SMACKBUFFERNODEPTH",	"ddraw:SmackBufferNoDepth", 0, offset(m_SmackBufferNoDepth) },
		{ "LOCKSYSSETTINGS",	"patch:LockSystemSettings", 0, offset(m_LockSysSettings) },
		{ "PROJECTBUFFER",		"win:ProjectBuffer", 0, offset(m_ProjectBuffer) },
		{ "FORCERELAXIS",		"input:ForceRelativeAxis", 0, offset(m_ForceRelAxis) },
		{ "FORCEABSAXIS",		"input:ForceAbsoluteAxis", 0, offset(m_ForceAbsAxis) },
		{ "DIRECTXREPLACE",		"hook:ReplaceDirectXDLLs", 0, offset(m_DirectXReplace) },
		{ "W98OPAQUEFONT",		"shim:Win98OpaqueFont", 0, offset(m_W98OpaqueFont) },
		{ "FAKEGLOBALATOM",		"shim:FakeGlobalAtom", 0, offset(m_FakeGlobalAtom) },
		{ "REVERTDIBPALETTE",	"patch:RevertDIBPalette", 0, offset(m_RevertDIBPalette) },
		{ "FIXDCALREADYCREATED","ddraw:FixDCAlreadyCreated", 0, offset(m_FixDCAlreadyCreated) },
		{ "SUPPRESSMENUS",		"win:SuppressMenus", 0, offset(m_SuppressMenus) },
		{ "KILLDEADLOCKS",		"patch:KillDeadlocks", 0, offset(m_KillDeadlocks) },
		{ "FORCECOLORKEYOFF",	"d3d:ForceColorKeyOff", 0, offset(m_ForceColorKeyOff) },
		{ "SPONGEBOBHACK",		"patch:SpongeBobHack", 0, offset(m_SpongeBobHack) },
		{ "DSOUNDREPLACE",		"hook:ReplaceDSoundDLL", 0, offset(m_DSoundReplace) },
		{ "FIGHTINGFORCEFIX",	"patch:FightingForceFix", 0, offset(m_FightingForceFix) },
		{ "CENTERDIALOGS",		"win:CenterDialogs", 0, offset(m_CenterDialogs) },
		{ "DXVERSIONLIE",		"shim:DirectXVersionLie", 0, offset(m_DxVersionLie) },
		{ "LEGACYBASEUNITS",	"patch:LegacyBaseUnits", 0, offset(m_LegacyBaseUnits) },
		{ "PATCHMSVBVM",		"patch:PatchMSVBVM", 0, offset(m_PatchMSVBVM) },
		{ "XRAYTWEAK",			"tweak:XRayTweak", 0, offset(m_XRayTweak) },
		{ "DISABLEPSGP",		"d3d:DisablePSGP", 0, offset(m_DisablePSGP) },
		{ "DISABLED3DMMX",		"d3d:DisableMMX", 0, offset(m_DisableD3DMMX) },
		{ "DISABLED3DXPSGP",	"d3d:DisableD3DXPSGP", 0, offset(m_DisableD3DxPSGP) },
		{ "D3DXDONOTMUTE",		"d3d:D3DXDoNotMute", 0, offset(m_D3DXDoNotMute) },
		//{ "DISABLEDEP",			"hook:DisableDEP", 0, offset(m_DisableDEP) },
		{ "FIXDCCOLORDEPTH",	"patch:FixDCColorDepth", 0, offset(m_FixDCColorDepth) },
		{ "DEFUSEFLASHBOMB",	"patch:DefuseFlashBomb", 0, offset(m_DefuseFlashBomb) },
		{ "LOCKALLWINDOWS",		"win:LockAllWindows", 0, offset(m_LockAllWindows) },
		{ "IGNOREFSYSERRORS",	"patch:IgnoreFileSysErrors", 0, offset(m_IgnoreFSysErrors) },
		{ "DISABLECOLORKEY",	"ddraw:DisableColorKey", 0, offset(m_DisableColorKey) },
		{ "NOSHELLEXECUTE",		"hook:NoShellExecute", 0, offset(m_NoShellExecute) },
		{ "CUSTOMPERFCOUNT",	"time:CustomPerfCount", 0, offset(m_CustomPerfCount) },
		{ "MIDIOUTBYPASS",		"sound:MidiOutBypass", 0, offset(m_MidiOutBypass) },
		{ "FIXEDBITBLT",		"shim:FixedBitBlt", 0, offset(m_FixedBitBlt) },
		{ "MW2WAVEOUTFIX",		"patch:MW2WaveOutFix", 0, offset(m_MW2WaveOutFix) },
		{ "WIN16CREATEFILE",	"shim:Win16CreateFile", 0, offset(m_Win16CreateFile) },
		{ "WIN16FINDFILEFIX",	"shim:Win16FindFileFix", 0, offset(m_Win16FindFileFix) },
		{ "SLOWCDSTATUS",		"sound:SlowCDStatus", 0, offset(m_SlowCDStatus) },
		{ "WAVEOUTUSEPREFDEV",	"sound:WaveOutUsePreferredDevice", 0, offset(m_WaveOutUsePrefDev) },
		{ "FIXMACROMEDIAREG",	"patch:FixMacromediaRegistry", 0, offset(m_FixMacromediaReg) },
		{ "LOCKSYSTEMMENU",		"win:LockSystemMenu", 0, offset(m_LockSystemMenu) },
		{ "STICKYWINDOWS",		"win:StickyWindows", 0,offset(m_StickyWindows)  },
		{ "DUMPTEXT",			"patch:DumpText", 0, offset(m_DumpText) },
		{ "EMULATEFLOPPYDRIVE",	"patch:EmulateFloppyDrive", 0, offset(m_EmulateFloppyDrive) },
		{ "FILTERCOLORKEY",		"ddraw:FilterColorKey", 0, offset(m_FilterColorKey) },
		{ "FORCECOLORKEY",		"ddraw:ForceColorKey", 0, offset(m_ForceColorKey) },
		{ "COLORKEY2PALETTE",	"ddraw:ColorKey2Palette", 0, offset(m_ColorKey2Palette) },
		{ "FASTPRIMARYUPDATE",	"ddraw:FastPrimaryUpdate", 0, offset(m_FastPrimaryUpdate) },
		{ "DEFERREDWINPOS",		"d3d:DeferredWindowPos", 0, offset(m_DeferredWinPos) },
		{ "DISPELTWEAK",		"tweak:DispelTweak", 0, offset(m_DispelTweak) },
		{ "DIABLOTWEAK",		"tweak:DiabloTweak", 0, offset(m_DiabloTweak) },
		{ "REPLACEPRIVOPS",		"patch:ReplacePrivOpcodes", 0, offset(m_ReplacePrivOps) },
		{ "EASPORTSHACK",		"tweak:EASportsHack", 0, offset(m_EASportsHack) },
		{ "LIMITRESOURCES",		"tweak:LimitResources", 0, offset(m_LimitResources) },
		{ "FIXFILEDIALOG",		"patch:FixFileDialog", 0, offset(m_FixFileDialog) },
		{ "MIDISETINSTRUMENT",	"sound:MIDISetInstrument", 0, offset(m_MIDISetInstrument) },
		{ "IGNORESCHEDULER",	"shim:IgnoreScheduler", 0, offset(m_IgnoreScheduler) },
		{ "SLOWDOWNEXCEPTIONS",	"tweak:SlowDownExceptions", 0, offset(m_SlowDownExceptions) },
		{ "DUPLICATEHANDLEFIX",	"shim:DuplicateHandleFix", 0, offset(m_DuplicateHandleFix) },
		{ "EMULATEWIN95",		"shim:EmulateWin95", 0, offset(m_EmulateWin95) },
		{ "FORCEHALFTONETINY",	"tweak:ForceHalftoneOnTiny", 0, offset(m_ForceHalftoneTiny) },
		{ "FIXFULLWINSTYLE",	"ddraw:FixFullscreenWinStyle", 0, offset(m_FixFullWinStyle) },
		{ "HIDEDISPLAYMODES",	"shim:HideDisplayModes", 0, offset(m_HideDisplayModes) },
		{ "RAMP2RGBDEVICE",		"d3d:RampToRGBDevice", 0, offset(m_Ramp2RGBDevice) },
		{ "RAMP2MMXDEVICE",		"d3d:RampToMMXDevice", 0, offset(m_Ramp2MMXDevice) },
		{ "CULLMODENONE",		"d3d:CullModeNone", 0, offset(m_CullModeNone) },
		{ "NOSURFACENEW",		"ddraw:NoDirectDrawSurfaceNew", 0, offset(m_NoSurfaceNew) },
		//{ "EMULATEW9XHEAP",		"shim:EmulateWin9XHeap", 0, offset(m_EmulateWin9XHeap) }, -- moved to Tab
		{ "FORCED3DREFDEVICE",	"d3d:ForceD3DRefDevice", 0, offset(m_ForceD3DRefDevice) },
		{ "FIXCREATEFILEMAP",	"shim:FixCreateFileMapping", 0, offset(m_FixCreateFileMap) },
		{ "EMUGETCOMMANDLINE",	"shim:EmulateGetCommandLine", 0, offset(m_EmuGetCommandLine) },
		{ "MCINOTIFYTOPMOST",	"sound:MCINotifyTopmost", 0, offset(m_MCINotifyTopmost) },
		{ "FORCESIMPLEWINDOW",	"win:ForceSimpleWindow", 0, offset(m_ForceSimpleWindow) },
		{ "CONFIRMESCAPE",		"input:ConfirmEscape", 0, offset(m_ConfirmEscape) },
		{ "FORCED3D9ON12",		"d3d:ForceD3D9on12", 0, offset(m_ForceD3D9on12) },
		{ "FIXTNLRHW",			"d3d:FixT&LRhw", 0, offset(m_FixTnLRhw) },
		{ "MAPMEMB0000",		"shim:MapMemoryB0000", 0, offset(m_MapMemB0000) },
		{ "TRIMCHILDWINDOWS",	"win:TrimChildWindows", 0, offset(m_TrimChildWindows) },
		{ "NATIVEDIALOGS",		"win:NativeDialogs", 0, offset(m_NativeDialogs) },
		{ "TRIMMAINWINDOW",		"win:TrimMainWindow", 0, offset(m_TrimMainWindow) },
		{ "FIXAILBUG",			"sound:FixAILBug", 0, offset(m_FixAILBug) },
		{ "LOCKCURSORSHAPE",	"win:LockCursorShape", 0, offset(m_LockCursorShape) },
		{ "FIXOVERLAPPEDRESULT","patch:FixOverlappedResult", 0, offset(m_FixOverlappedResult) },
		{ "FORCEFILTERNEAREST",	"d3d:ForceFilterNearest", 0, offset(m_ForceFilterNearest) },
		{ "HIDEDIRECTDRAW",		"tweak:HideDirectDraw", 0, offset(m_HideDirectDraw) },
		{ "COMPENSATEOGLCOPY",	"tweak:CompensateOpenGLCopy", 0, offset(m_CompensateOpenGLCopy) },
		{ "EMUDDSYNCSHIM",		"shim:EmulateDirectDrawSync", 0, offset(m_EmuDDSyncShim) },
		{ "STRETCHBACKBUFFER",	"ddraw:StretchBackBuffer", 0, offset(m_StretchBackBuffer) },
		{ "LEGACYKERNEL32",		"patch:LegacyKernel32", 0, offset(m_LegacyKernel32) },
		{ "DDRAWLEGACYCALLBACK","ddraw:LegacyCallback", 0, offset(m_DDrawLegacyCallback) },
		{ "ASYNCWAVEOUTOPEN",	"sound:AsyncWaveOutOpen", 0, offset(m_AsyncWaveOutOpen) },
		{ "ASYNCWAVEOUTCLOSE",	"sound:AsyncWaveOutClose", 0, offset(m_AsyncWaveOutClose) },
		{ "TRANSPARENTDIALOG",	"win:TransparentDialog", 0, offset(m_TransparentDialog) },
		{ "EMULATEWINRESIZE",	"win:EmulateWinResize", 0, offset(m_EmulateWinResize) },
		{ "DISABLELAYEREDWIN",	"win:DisableLayeredWindows", 0, offset(m_DisableLayeredWin) },
		{ "SCALECHILDWIN",		"win:ScaleChildWindows", 0, offset(m_ScaleChildWin) },
		{ "TRIMTEXTYPOS",		"win:TrimTextYPos", 0, offset(m_TrimTextYPos) },
		{ "NODIRECTSOUND",		"sound:NoDirectSound", 0, offset(m_NoDirectSound) },
		{ "FORCEEMULDRIVER",	"sound:ForceEmulDriver", 0, offset(m_ForceEmulDriver) },
		{ "DEFMOVIESCOLOR",		"mmedia:DefMoviesColor", 0, offset(m_DefMoviesColor) },
		{ "BESTMOVIESCOLOR",	"mmedia:BestMoviesColor", 0, offset(m_BestMoviesColor) },
		{ "EMULATEXMIRRORING",	"ddraw:EmulateXMirroring", 0, offset(m_EmulateXMirroring) },
		{ "EMULATEYMIRRORING",	"ddraw:EmulateYMirroring", 0, offset(m_EmulateYMirroring) },
		{ "SUPPRESSBLTFX",		"ddraw:SuppressBltFX", 0, offset(m_SuppressBltFX) },
		{ NULL, NULL, NULL, NULL }  
	};
	// build the tweaks category list
	CComboBox *cTweaks = (CComboBox *)this->GetDlgItem(IDC_COMBO_TWEAKS);
	char category[256];
	cTweaks->Clear();
	cTweaks->AddString("ALL");
	for(i=0; ;i++){
		if(AvailableTweaks[i].Label == NULL) break;
		strcpy(category,  AvailableTweaks[i].Caption);
		strtok(category, ":");
		if(cTweaks->FindString(0, category) == CB_ERR) cTweaks->AddString(category);
	}

	// v2.04.30.fx1: the cTarget class is rebuilt, so each time all BOOL pointers must be reassigned!!!
	for(i=0; ;i++){
		if(AvailableTweaks[i].Label == NULL) break;
		AvailableTweaks[i].Flag = (BOOL *)((DWORD)cTarget + (DWORD)AvailableTweaks[i].Target);
	}
	AfxEnableControlContainer();
	ListAvail=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSAVAIL);
	ListActive=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSACTIVE);
	ListAvail->ResetContent();
	ListActive->ResetContent();
	for(i=0; ; i++) {
		if(AvailableTweaks[i].Label == NULL) break;
		//if(AvailableTweaks[i].Caption[0] == '#') continue; // quick way to comment out entries ....
		ListAvail->AddString(AvailableTweaks[i].Caption);
		if(*(AvailableTweaks[i].Flag)) {
			ListActive->AddString(AvailableTweaks[i].Caption);
			OutTrace("Added tweak \"%s\"\n", AvailableTweaks[i].Label);
		}
	}
	gAvailableTweaks = &AvailableTweaks[0];
	pBtn = (CButton *)this->GetDlgItem(IDC_TWEAK_ADD);
	pBtn->ModifyStyle(0, BS_ICON);
	hIcn = (HICON)LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_TWEAK_ADD), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	pBtn->SetIcon(hIcn);
	pBtn = (CButton *)this->GetDlgItem(IDC_TWEAK_REMOVE);
	pBtn->ModifyStyle(0, BS_ICON);
	hIcn = (HICON)LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_TWEAK_REMOVE), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	pBtn->SetIcon(hIcn);
	pBtn = (CButton *)this->GetDlgItem(IDC_TWEAK_HELP);
	pBtn->ModifyStyle(0, BS_ICON);
	hIcn = (HICON)LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_TWEAK_HELP), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	pBtn->SetIcon(hIcn);
	CDialog::OnInitDialog();
	return TRUE;
}

BEGIN_MESSAGE_MAP(CTabTweaks, CDialog)
	//{{AFX_MSG_MAP(CTabTweaks)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_TWEAK_ADD, &CTabTweaks::OnBnClickedTweakAdd)
	ON_BN_CLICKED(IDC_TWEAK_REMOVE, &CTabTweaks::OnBnClickedTweakRemove)
	ON_BN_CLICKED(IDC_TWEAK_HELP, &CTabTweaks::OnBnClickedTweakHelp)
	ON_LBN_DBLCLK(IDC_LIST_TWEAKSAVAIL, &CTabTweaks::OnLbnDblclkListTweaksavail)
	ON_LBN_DBLCLK(IDC_LIST_TWEAKSACTIVE, &CTabTweaks::OnLbnDblclkListTweaksactive)
	ON_CBN_SELCHANGE(IDC_COMBO_TWEAKS, &CTabTweaks::OnCbnSelchangeComboTweaks)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabTweaks message handlers


void CTabTweaks::OnBnClickedTweakAdd()
{
	CListBox *ListAvail, *ListActive;
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	char sBuffer[256];
	int i, k;
	//MessageBox("Add button clicked","click",0);
	ListAvail=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSAVAIL);
	ListActive=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSACTIVE);
	int nCount = ListAvail->GetSelCount();
	CArray<int,int> aryListBoxSel;
	aryListBoxSel.SetSize(nCount);
	ListAvail->GetSelItems(nCount, aryListBoxSel.GetData()); 
	for(int j=0; j<nCount; j++){
		//i = ListAvail->GetCurSel();
		i = aryListBoxSel.GetAt(j);
		if (i == LB_ERR) return;
		// search for a matching string - the ListAvail list is sorted!
		ListAvail->GetText(i, sBuffer);
		for(k=0; ; k++) {
			if(gAvailableTweaks[k].Label == NULL) return;
			if(!strcmp(gAvailableTweaks[k].Caption, sBuffer)) break;
		}
		//MessageBox(gAvailableTweaks[i].Caption,"select",0);
		if(!*(gAvailableTweaks[k].Flag)){
			//MessageBox(gAvailableTweaks[i].Caption,"add",0);
			*(gAvailableTweaks[k].Flag) = TRUE;
			ListActive->AddString(gAvailableTweaks[k].Caption);
			OutTrace("Added tweak \"%s\"\n", gAvailableTweaks[k].Label);
		}
	}
	ListAvail->SetSel(-1, FALSE); // deselect all ....
}

void CTabTweaks::OnBnClickedTweakRemove()
{
	CListBox *ListActive;
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	char sBuffer[256];
	int i, k;
	//ListAvail=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSAVAIL);
	ListActive=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSACTIVE);
	int nCount = ListActive->GetSelCount();
	CArray<int,int> aryListBoxSel;
	aryListBoxSel.SetSize(nCount);
	ListActive->GetSelItems(nCount, aryListBoxSel.GetData()); 
	for(int j=0; j<nCount; j++){
		//i = ListActive->GetCurSel();
		i = aryListBoxSel.GetAt(j);
		if (i == LB_ERR) return;
		ListActive->GetText(i, sBuffer);
		//ListActive->DeleteString(i);
		//MessageBox(sBuffer,"delete",0);
		for(k=0; ; k++) {
			if(gAvailableTweaks[k].Label == NULL) break;
			if(!strcmp(gAvailableTweaks[k].Caption, sBuffer)){
				*(gAvailableTweaks[k].Flag) = FALSE;
				OutTrace("Removed tweak \"%s\"\n", gAvailableTweaks[k].Label);
			}
		}
	}
	ListActive->SetSel(-1, FALSE); // deselect all ....
	// rebuild the whole list!
	ListActive->ResetContent();
	for(i=0; ; i++) {
		if(gAvailableTweaks[i].Label == NULL) break;
		//if(gAvailableTweaks[i].Caption[0] == '#') continue; // quick way to comment out entries ....
		if(*(gAvailableTweaks[i].Flag)) {
			ListActive->AddString(gAvailableTweaks[i].Caption);
			OutTrace("Added tweak \"%s\"\n", gAvailableTweaks[i].Label);
		}
	}
}

#define MAX_HELP 1024

void CTabTweaks::OnBnClickedTweakHelp()
{
	char sHelp[MAX_HELP];
	char sHelpBuffer[MAX_HELP];
	char sHelpPath[MAX_PATH];
	CListBox *ListAvail;
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	char sBuffer[256];
	char *Title="tweaks help";
	int i, k;
	char *p;
	ListAvail=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSAVAIL);
	int nCount = ListAvail->GetSelCount();
	if(nCount==0){
		MessageBox("Please, select one item in available tweaks list", Title, 0);
		return;
	}
	if(nCount>1){
		MessageBox("Please, select only ONE item in available tweaks list", Title, 0);
		return;
	}
	int aryListBoxSel[1];
	ListAvail->GetSelItems(nCount, aryListBoxSel); 
	i = aryListBoxSel[0];
	if (i == LB_ERR) return;
	// search for a matching string - the ListAvail list is sorted!
	ListAvail->GetText(i, sBuffer);
	for(k=0; ; k++) {
		if(gAvailableTweaks[k].Label == NULL) return;
		if(!strcmp(gAvailableTweaks[k].Caption, sBuffer)) break;
	}
	p = strstr(sBuffer, ":") + 1;
	//sprintf(sHelpPath, "%s\\tweaks.ini", gInitPath);
	// v2.04.41: fixed path for tweaks.ini help file
	strncpy(sHelpPath, gInitPath, MAX_PATH);
	char *q = sHelpPath + (strlen(sHelpPath) - strlen("dxwnd.ini"));
	strcpy(q, "tweaks.ini");
	GetPrivateProfileString(p, "help", "help unavailable", sHelp, MAX_HELP, sHelpPath);
	sprintf(sHelpBuffer, "%s\n%s\n\n%s", p, gAvailableTweaks[k].Label, sHelp);
	MessageBox(sHelpBuffer, Title, 0);
}

void CTabTweaks::OnLbnDblclkListTweaksavail()
{
	this->OnBnClickedTweakAdd();
}

void CTabTweaks::OnLbnDblclkListTweaksactive()
{
	this->OnBnClickedTweakRemove();
}

void CTabTweaks::OnCbnSelchangeComboTweaks()
{
	char topic[20+2];
	CListBox *ListAvail=(CListBox *)this->GetDlgItem(IDC_LIST_TWEAKSAVAIL);
	CComboBox *cTweaks = (CComboBox *)this->GetDlgItem(IDC_COMBO_TWEAKS);
	ListAvail->ResetContent();
	int iTopic = cTweaks->GetCurSel();
	cTweaks->GetLBText(iTopic, topic);
	strcat(topic, ":");
	for(int i=0; ; i++) {
		if(gAvailableTweaks[i].Label == NULL) break;
		//if(AvailableTweaks[i].Caption[0] == '#') continue; // quick way to comment out entries ....
		if ((iTopic == 0) || (!strncmp(topic, gAvailableTweaks[i].Caption, strlen(topic)))) 
			ListAvail->AddString(gAvailableTweaks[i].Caption);
	}
}
