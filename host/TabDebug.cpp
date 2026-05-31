// TabDebug.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabDebug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabDebug::CTabDebug(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabDebug::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabDebug)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabDebug::PreTranslateMessage(MSG *pMsg)
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

void CTabDebug::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	DDX_Check(pDX, IDC_ASSERT, cTarget->m_AssertDialog);
	DDX_Check(pDX, IDC_STARTWITHTOGGLE, cTarget->m_StartWithToggle);
	DDX_Check(pDX, IDC_MARKBLIT, cTarget->m_MarkBlit);
	DDX_Check(pDX, IDC_MARKLOCK, cTarget->m_MarkLock);
	DDX_Check(pDX, IDC_MARKWING32, cTarget->m_MarkWinG32);
	DDX_Check(pDX, IDC_MARKGDI32, cTarget->m_MarkGDI32);
	DDX_Check(pDX, IDC_MARKCLIPRGN, cTarget->m_MarkClipRgn);
	DDX_Check(pDX, IDC_DUMPDIBSECTION, cTarget->m_DumpDIBSection);
	DDX_Check(pDX, IDC_DUMPDEVCONTEXT, cTarget->m_DumpDevContext);
	DDX_Check(pDX, IDC_CAPTURESCREENS, cTarget->m_CaptureScreens);
	DDX_Check(pDX, IDC_DUMPSURFACES, cTarget->m_DumpSurfaces);
	DDX_Check(pDX, IDC_DUMPBLITSRC, cTarget->m_DumpBlitSrc);
	DDX_Check(pDX, IDC_DUMPBITMAPS, cTarget->m_DumpBitmaps);
	DDX_Check(pDX, IDC_FASTBLT, cTarget->m_FastBlt);
	DDX_Check(pDX, IDC_CAPMASK, cTarget->m_CapMask);
	DDX_Check(pDX, IDC_NODDRAWBLT, cTarget->m_NoDDRAWBlt);
	DDX_Check(pDX, IDC_NODDRAWFLIP, cTarget->m_NoDDRAWFlip);
	DDX_Check(pDX, IDC_NOGDIBLT, cTarget->m_NoGDIBlt);
	DDX_Check(pDX, IDC_NOICONS, cTarget->m_NoIcons);
	DDX_Check(pDX, IDC_DISABLEWINHOOK, cTarget->m_DisableWinHook);
	DDX_Check(pDX, IDC_FREEZEINJECTEDSON, cTarget->m_FreezeInjectedSon);
	DDX_Check(pDX, IDC_STRESSRESOURCES, cTarget->m_StressResources);
	DDX_Check(pDX, IDC_EXPERIMENTAL1, cTarget->m_Experimental);
	DDX_Check(pDX, IDC_EXPERIMENTAL2, cTarget->m_Experimental2);
	DDX_Check(pDX, IDC_EXPERIMENTAL3, cTarget->m_Experimental3);
	DDX_Check(pDX, IDC_EXPERIMENTAL4, cTarget->m_Experimental4);
	DDX_Check(pDX, IDC_EXPERIMENTAL5, cTarget->m_Experimental5);
	DDX_Check(pDX, IDC_EXPERIMENTAL6, cTarget->m_Experimental6);
	DDX_Check(pDX, IDC_EXPERIMENTAL7, cTarget->m_Experimental7);
	DDX_Check(pDX, IDC_EXPERIMENTAL8, cTarget->m_Experimental8);
	DDX_Check(pDX, IDC_CENTERTOWIN, cTarget->m_CenterToWin);
	DDX_Check(pDX, IDC_DISABLEDELETE, cTarget->m_DisableDelete);
	DDX_Check(pDX, IDC_DUMPDSHOWGRAPH, cTarget->m_DumpDShowGraph);
	DDX_Check(pDX, IDC_NOWINDOWHOOKS, cTarget->m_NoWindowHooks);
	DDX_Check(pDX, IDC_DISABLEWINHOOKS, cTarget->m_DisableWinHooks);
	DDX_Check(pDX, IDC_FIXRANDOMPALETTE, cTarget->m_FixRandomPalette);
	DDX_Check(pDX, IDC_ZBUFFERALWAYS, cTarget->m_ZBufferAlways);
	DDX_Check(pDX, IDC_FORCED3DCHECKOK, cTarget->m_ForceD3DCheckOK);
}

BEGIN_MESSAGE_MAP(CTabDebug, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers


