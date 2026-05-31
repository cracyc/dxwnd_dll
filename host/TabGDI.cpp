// TabGDI.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabGDI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabLogs dialog

CTabGDI::CTabGDI(CWnd* pParent /*=NULL*/)
	: CDialog(CTabGDI::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabGDI)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabGDI::PreTranslateMessage(MSG *pMsg)
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

void CTabGDI::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	// GDI
	DDX_Check(pDX, IDC_CLIENTREMAPPING, cTarget->m_ClientRemapping);
	DDX_Radio(pDX, IDC_GDINONE, cTarget->m_DCEmulationMode);
	DDX_Check(pDX, IDC_FIXTEXTOUT, cTarget->m_FixTextOut);
	DDX_Check(pDX, IDC_FORCEHALFTONE, cTarget->m_ForceHalfTone);
	DDX_Check(pDX, IDC_QUALITYFONTS, cTarget->m_QualityFonts);
	DDX_Check(pDX, IDC_SHRINKFONTWIDTH, cTarget->m_ShrinkFontWidth);
	DDX_Check(pDX, IDC_NONANTIALIASEDFONTS, cTarget->m_NonAntialiasedFonts);
	DDX_Check(pDX, IDC_FIXCLIPPERAREA, cTarget->m_FixClipperArea);
	DDX_Check(pDX, IDC_SHAREDDCHYBRID, cTarget->m_SharedDCHybrid);
	DDX_Check(pDX, IDC_SYNCPALETTE, cTarget->m_SyncPalette);
	DDX_Check(pDX, IDC_NOWINERRORS, cTarget->m_NoWinErrors);
	DDX_Check(pDX, IDC_NODIALOGS, cTarget->m_NoDialogs);
	DDX_Check(pDX, IDC_STRETCHDIALOGS, cTarget->m_StretchDialogs);
	DDX_Check(pDX, IDC_INVALIDATEFULLRECT, cTarget->m_InvalidateFullRect);
	DDX_Check(pDX, IDC_NOSETPIXELFORMAT, cTarget->m_NoSetPixelFormat);
	DDX_Check(pDX, IDC_SCALECBTHOOK, cTarget->m_ScaleCBTHook);
	DDX_Check(pDX, IDC_WINAUTOREPAINT, cTarget->m_WinAutoRepaint);
	DDX_Check(pDX, IDC_FIXBITMAPCOLOR, cTarget->m_FixBitMapColor);
	DDX_Check(pDX, IDC_NOFILLRECT, cTarget->m_NoFillRect);

	// color management
	DDX_Check(pDX, IDC_DISABLEGAMMARAMP, cTarget->m_DisableGammaRamp);
	DDX_Check(pDX, IDC_LOCKSYSCOLORS, cTarget->m_LockSysColors);
	DDX_Check(pDX, IDC_LOCKRESERVEDPALETTE, cTarget->m_LockReservedPalette);
	DDX_Check(pDX, IDC_PALDIBEMULATION, cTarget->m_PALDIBEmulation);
	DDX_Check(pDX, IDC_REFRESHONREALIZE, cTarget->m_RefreshOnRealize);
}

BEGIN_MESSAGE_MAP(CTabGDI, CDialog)
	//{{AFX_MSG_MAP(CTabLogs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabLogs message handlers
