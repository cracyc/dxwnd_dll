// dxTabCtrl.cpp : implementation file
//
/////////////////////////////////////////////////////
// This class is provided as is and Ben Hill takes no
// responsibility for any loss of any kind in connection
// to this code.
/////////////////////////////////////////////////////
// Is is meant purely as a educational tool and may
// contain bugs.
/////////////////////////////////////////////////////
// ben@shido.fsnet.co.uk
// http://www.shido.fsnet.co.uk
/////////////////////////////////////////////////////
// Thanks to a mystery poster in the C++ forum on 
// www.codeguru.com I can't find your name to say thanks
// for your Control drawing code. If you are that person 
// thank you very much. I have been able to use some of 
// you ideas to produce this sample application.
/////////////////////////////////////////////////////

#include "stdafx.h"
#include "dxTabCtrl.h"

#include "TabProgram.h"
#include "TabHook.h"
#include "TabCDA.h"
#include "TabDirectX.h"
#include "TabDirectX2.h"
#include "TabDirect3D.h"
#include "TabDirect3D2.h"
#include "TabWDDM.h"
#include "TabInput.h"
#include "Tab3D.h"
#include "TabMouse.h"
#include "TabMsgs.h"
#include "TabTiming.h"
#include "TabWindow.h"
#include "TabOpenGL.h"
#include "TabCompat.h"
#include "TabColor.h"
#include "TabLogs.h"
#include "TabRegistry.h"
#include "TabNotes.h"
#include "TabSysLibs.h"
#include "TabDebug.h"
#include "TabOpenGL.h"
#include "TabSDL.h"
#include "TabSound.h"
#include "TabTweaks.h"
#include "TabIO.h"
#include "TabHW.h"
#include "TabLocale.h"
#include "TabGDI.h"
#include "TabWine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL gbDebug;
extern BOOL gbExpertMode;
extern BOOL gbWineConfig;
extern DWORD GetDxWndCaps(void);
extern void OutTrace(const char *, ...);
extern BOOL IsWinXP();
extern BOOL QueryWDDM();

/////////////////////////////////////////////////////////////////////////////
// CDXTabCtrl

CDXTabCtrl::CDXTabCtrl()
{
	OutTrace("CDXTabCtrl::CDXTabCtrl: BEGIN\n");
}

CDXTabCtrl::~CDXTabCtrl()
{
	// commented to avoid crash when switching list video mode
	for(int nCount=0; nCount < m_nNumberOfPages; nCount++){
		m_tabPages[nCount] = NULL;
		m_tabHelpers[nCount] = NULL;
		//delete m_tabPages[nCount];
	}
}

void CDXTabCtrl::Init()
{
	//OutTrace("CDXTabCtrl::Init: m_nNumberOfPages=%d\n", m_nNumberOfPages);
	// BEWARE: this part MUST be syncronized with TargetDld initialization in TargetDlg.cpp !!!
	int i;
	m_tabCurrent=0;

	// build helpers strings
	i=0;
	m_tabHelpers[i++]="Main";
	if (gbExpertMode) m_tabHelpers[i++]="Hook1";
	m_tabHelpers[i++]="Video";
	m_tabHelpers[i++]="Mouse";
	m_tabHelpers[i++]="Input";
	if (gbExpertMode) m_tabHelpers[i++]="3D";
	if (gbExpertMode) m_tabHelpers[i++]="Msgs";
	m_tabHelpers[i++]="DirectX";
	if (gbExpertMode) m_tabHelpers[i++]="DirectX2";
	m_tabHelpers[i++]="Direct3D";
	if (gbExpertMode) m_tabHelpers[i++]="Direct3D2";
	if (gbExpertMode && QueryWDDM())m_tabHelpers[i++]="WDDM";
	if (gbExpertMode) m_tabHelpers[i++]="Timing";
	if (gbExpertMode && (GetDxWndCaps() & DXWCAPS_CANLOG)) m_tabHelpers[i++]="Logs";
	if (gbExpertMode) m_tabHelpers[i++]="Libs";
	m_tabHelpers[i++]="GDI";
	if (gbExpertMode) m_tabHelpers[i++]="Compatibility";
	if (gbExpertMode) m_tabHelpers[i++]="Registry1";
	m_tabHelpers[i++]="Notes";
	if (gbExpertMode) m_tabHelpers[i++]="OpenGL";
	if (gbExpertMode) m_tabHelpers[i++]="SDL";
	if (gbExpertMode) m_tabHelpers[i++]="Sound";
	if (gbExpertMode) m_tabHelpers[i++]="CDAudio";
	if (gbExpertMode) m_tabHelpers[i++]="Tweaks1";
	if (gbExpertMode) m_tabHelpers[i++]="IOtweaks";
	if (gbExpertMode && gbDebug) m_tabHelpers[i++]=NULL;
	if (gbExpertMode && gbDebug) m_tabHelpers[i++]=NULL;
	if (gbExpertMode) m_tabHelpers[i++]="Locale";
	if (gbWineConfig) m_tabHelpers[i++]="Wine";

	// build tab control pages
	i=0;
	m_tabPages[i++]=new CTabProgram;
	if (gbExpertMode) m_tabPages[i++]=new CTabHook;
	m_tabPages[i++]=new CTabWindow;
	m_tabPages[i++]=new CTabMouse;
	m_tabPages[i++]=new CTabInput;
	if (gbExpertMode) m_tabPages[i++]=new CTab3D;
	if (gbExpertMode) m_tabPages[i++]=new CTabMsgs;
	m_tabPages[i++]=new CTabDirectX;
	if (gbExpertMode) m_tabPages[i++]=new CTabDirectX2;
	m_tabPages[i++]=new CTabDirect3D;
	if (gbExpertMode) m_tabPages[i++]=new CTabDirect3D2;
	if (gbExpertMode && QueryWDDM()) m_tabPages[i++]=new CTabWDDM;
	if (gbExpertMode) m_tabPages[i++]=new CTabTiming;
	if (gbExpertMode && (GetDxWndCaps() & DXWCAPS_CANLOG)) m_tabPages[i++]=new CTabLogs;
	if (gbExpertMode) m_tabPages[i++]=new CTabSysLibs;
	m_tabPages[i++]=new CTabGDI;
	if (gbExpertMode) m_tabPages[i++]=new CTabCompat;
	if (gbExpertMode) m_tabPages[i++]=new CTabRegistry;
	m_tabPages[i++]=new CTabNotes;
	if (gbExpertMode) m_tabPages[i++]=new CTabOpenGL;
	if (gbExpertMode) m_tabPages[i++]=new CTabSDL;
	if (gbExpertMode) m_tabPages[i++]=new CTabSound;
	if (gbExpertMode) m_tabPages[i++]=new CTabCDA;
	if (gbExpertMode) m_tabPages[i++]=new CTabTweaks;
	if (gbExpertMode) m_tabPages[i++]=new CTabIO;
	if (gbExpertMode && gbDebug) m_tabPages[i++]=new CTabDebug;
	if (gbExpertMode && gbDebug) m_tabPages[i++]=new CTabHW;
	if (gbExpertMode) m_tabPages[i++]=new CTabLocale;
	if (gbWineConfig)m_tabPages[i++]=new CTabWine;

	i=0;
	m_tabPages[i++]->Create(IDD_TAB_MAIN, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_HOOK, this);
	m_tabPages[i++]->Create(IDD_TAB_VIDEO, this);
	m_tabPages[i++]->Create(IDD_TAB_MOUSE, this);
	m_tabPages[i++]->Create(IDD_TAB_INPUT, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_3D, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_MESSAGES, this);
	m_tabPages[i++]->Create(IDD_TAB_DIRECTX, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_DIRECTX2, this);
	m_tabPages[i++]->Create(IDD_TAB_D3D, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_D3D2, this);
	if (gbExpertMode && QueryWDDM()) m_tabPages[i++]->Create(IDD_TAB_WDDM, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_TIMING, this);
	if (gbExpertMode && (GetDxWndCaps() & DXWCAPS_CANLOG)) m_tabPages[i++]->Create(IDD_TAB_LOG, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_SYSLIBS, this);
	m_tabPages[i++]->Create(IDD_TAB_GDI, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_COMPAT, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_REGISTRY, this);
	m_tabPages[i++]->Create(IDD_TAB_NOTES, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_OPENGL, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_SDL, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_SOUND, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_CDA, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_TWEAKS, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_IO, this);
	if (gbExpertMode && gbDebug) m_tabPages[i++]->Create(IDD_TAB_DEBUG, this);
	if (gbExpertMode && gbDebug) m_tabPages[i++]->Create(IDD_TAB_HW, this);
	if (gbExpertMode) m_tabPages[i++]->Create(IDD_TAB_LOCALE, this);
	if (gbWineConfig) m_tabPages[i++]->Create(IDD_TAB_WINE, this);

	m_nNumberOfPages = i;
	for(int nCount=0; nCount < m_nNumberOfPages; nCount++){
		m_tabPages[nCount]->ShowWindow(nCount ? SW_HIDE:SW_SHOW);
	}

	SetRectangle();

	// trick to invalidate and redraw initial rect - should be done better than this ....
	SwitchToTab(1);
	SwitchToTab(0);
}

#define DEFAULTDPI 96
#define PANELHEIGHT 424
#define FRAMEHEIGHT 524
#define BUTTONDISPL 20
#define InchToPixelsX(n) (((n)*dpiX)/100)
#define InchToPixelsY(n) (((n)*dpiY)/100)
#define PixelsToInchX(n) (((n)*100)/dpiX)
#define PixelsToInchY(n) (((n)*100)/dpiY)

#define TRACEVALUES

void CDXTabCtrl::SetRectangle()
{
	CRect tabRect, itemRect;
	int nX, nY, nXc, nYc;
	RECT PanelRect, FrameRect;
	CWnd *pWnd;

	// get dpi scaling factors. 
	// WARNING: default dpi value for 100% scaling is 96 dpi
	int dpiX = this->GetDC()->GetDeviceCaps(LOGPIXELSX);
	int dpiY = this->GetDC()->GetDeviceCaps(LOGPIXELSY);
#ifdef TRACEVALUES
	OutTrace("dpiX=%d dpiY=%d\n", dpiX, dpiY);
	OutTrace("scaling=%d%% x %d%%\n", dpiX*100/DEFAULTDPI, dpiY*100/DEFAULTDPI);
#endif

	// get info for tabs height measure. 
	// Beware: the value is valid only if following window scaling won't alter the form width.
	// TabHeight & TabWidth expressed in pixels
	GetClientRect(&tabRect);
	if(!GetItemRect(0, &itemRect)){
		OutTrace("GetItemRect ERROR err=%d\n", GetLastError());
	}

	nX=itemRect.left;
	nY=itemRect.bottom+1;
	nXc=tabRect.right-itemRect.left-1;
	nYc=tabRect.bottom-nY-1 ;

	int TabHeight =  (itemRect.top - tabRect.top);
	int TabWidth =  (itemRect.right - tabRect.left);
#ifdef TRACEVALUES
	OutTrace("tab size=(%d x %d)\n", TabWidth, TabHeight);
#endif

	// recovery ....
	if(TabHeight < 0) TabHeight = 60;

	// resize external frame
	CWnd *Frame = m_tabPages[0]->GetParent()->GetParent();	// Frame = external window
	Frame->GetWindowRect(&FrameRect);
	Frame->SetWindowPos(&wndTop, 0, 0, FrameRect.right-FrameRect.left, (FRAMEHEIGHT*dpiY/100)+TabHeight, SWP_SHOWWINDOW|SWP_NOMOVE);
	//Frame->SetWindowPos(&wndTop, 0, 0, FrameRect.right-FrameRect.left, FRAMEHEIGHT+TabHeight, SWP_SHOWWINDOW|SWP_NOMOVE);

	// resize tabs frame (must grow in height)
	pWnd = Frame->GetDlgItem(IDC_TABPANEL);
	pWnd->GetClientRect(&PanelRect);
	pWnd->SetWindowPos(&wndTop, 0, 0, PanelRect.right-PanelRect.left, InchToPixelsY(PANELHEIGHT)+TabHeight, SWP_SHOWWINDOW|SWP_NOMOVE);

	// move buttons
	RECT BtnRect;
	int BtnY = InchToPixelsY(PANELHEIGHT+BUTTONDISPL)+TabHeight;

	pWnd = Frame->GetDlgItem(IDTRY);
	pWnd->GetWindowRect(&BtnRect);
	pWnd->SetWindowPos(&wndTop, BtnRect.left-FrameRect.left, BtnY, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);

	pWnd = Frame->GetDlgItem(IDKILL);
	pWnd->GetWindowRect(&BtnRect);
	pWnd->SetWindowPos(&wndTop, BtnRect.left-FrameRect.left, BtnY, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);

	pWnd = Frame->GetDlgItem(IDCONTEXTHELP);
	pWnd->GetWindowRect(&BtnRect);
	pWnd->SetWindowPos(&wndTop, BtnRect.left-FrameRect.left, BtnY, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);

	pWnd = Frame->GetDlgItem(IDCANCEL);
	pWnd->GetWindowRect(&BtnRect);
	pWnd->SetWindowPos(&wndTop, BtnRect.left-FrameRect.left, BtnY, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);

	pWnd = Frame->GetDlgItem(IDOK);
	pWnd->GetWindowRect(&BtnRect);
	pWnd->SetWindowPos(&wndTop, BtnRect.left-FrameRect.left, BtnY, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);

	m_tabPages[0]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	for(int nCount=1; nCount < m_nNumberOfPages; nCount++){
		m_tabPages[nCount]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);
	}
}

BEGIN_MESSAGE_MAP(CDXTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CDXTabCtrl)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXTabCtrl message handlers

void CDXTabCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTabCtrl::OnLButtonDown(nFlags, point);

	if(m_tabCurrent != GetCurFocus()){
		m_tabPages[m_tabCurrent]->ShowWindow(SW_HIDE);
		m_tabCurrent=GetCurFocus();
		m_tabPages[m_tabCurrent]->ShowWindow(SW_SHOW);
		m_tabPages[m_tabCurrent]->SetFocus();
	}
}

void CDXTabCtrl::SwitchToTab(int pos)
{
	m_tabPages[m_tabCurrent]->ShowWindow(SW_HIDE);
	SetCurSel(pos);
	m_tabPages[pos]->ShowWindow(SW_SHOW);
	m_tabPages[pos]->SetFocus();
	m_tabCurrent=GetCurFocus();
}

void CDXTabCtrl::OnOK()
{
	for(int nCount=0; nCount < m_nNumberOfPages; nCount++){
		m_tabPages[nCount]->UpdateData(TRUE);
	}
}
