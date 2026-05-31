// TabGDI.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabSysLibs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabLogs dialog

CTabSysLibs::CTabSysLibs(CWnd* pParent /*=NULL*/)
	: CDialog(CTabSysLibs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabSysLibs)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabSysLibs::PreTranslateMessage(MSG *pMsg)
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

void CTabSysLibs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	// MCI
	DDX_Check(pDX, IDC_REMAPMCI, cTarget->m_RemapMCI);
	DDX_Check(pDX, IDC_NOMOVIES, cTarget->m_NoMovies);
	DDX_Check(pDX, IDC_STRETCHMOVIES, cTarget->m_StretchMovies);
	DDX_Check(pDX, IDC_FIXMOVIESCOLOR, cTarget->m_FixMoviesColor);
	DDX_Check(pDX, IDC_BYPASSMCI, cTarget->m_BypassMCI);
	DDX_Check(pDX, IDC_FIXPCMAUDIO, cTarget->m_FixPCMAudio);
	DDX_Check(pDX, IDC_FIXSMACKLOOP, cTarget->m_FixSmackLoop);
	DDX_Check(pDX, IDC_BYPASSACTIVEMOVIE, cTarget->m_BypassActiveMovie);

	// additional hooks
	DDX_Check(pDX, IDC_HOOKWING32, cTarget->m_HookWinG32);
	DDX_Check(pDX, IDC_HOOKGLIDE, cTarget->m_HookGlide);
	DDX_Check(pDX, IDC_SUPPRESSGLIDE, cTarget->m_SuppressGlide);
	DDX_Check(pDX, IDC_HOOKSMACKW32, cTarget->m_HookSmackW32);
	DDX_Check(pDX, IDC_HOOKBINKW32, cTarget->m_HookBinkW32);

	// Kernel32 heap
	DDX_Check(pDX, IDC_FIXGLOBALUNLOCK, cTarget->m_FixGlobalUnlock);
	DDX_Check(pDX, IDC_NOBAADFOOD, cTarget->m_NoBAADFOOD);
	DDX_Check(pDX, IDC_HEAPLEAK, cTarget->m_HeapLeak);
	DDX_Check(pDX, IDC_SAFEHEAP, cTarget->m_SafeHeap);
	DDX_Check(pDX, IDC_REPAIRHEAP, cTarget->m_RepairHeap);
	DDX_Check(pDX, IDC_EMULATEWIN9XHEAP, cTarget->m_EmulateWin9XHeap);
	DDX_Check(pDX, IDC_HEAPPADALLOCATION, cTarget->m_HeapPadAllocation);
	DDX_Check(pDX, IDC_HEAPZEROMEMORY, cTarget->m_HeapZeroMemory);

	// DirectShow
	DDX_Check(pDX, IDC_HOOKDIRECTSHOW, cTarget->m_HookDirectShow);
	DDX_Check(pDX, IDC_SCALEDIRECTSHOW, cTarget->m_ScaleDirectShow);
	DDX_Check(pDX, IDC_IGNOREADDFILTER, cTarget->m_IgnoreAddFilter);
	DDX_Check(pDX, IDC_SUPPRESSIVMR, cTarget->m_SuppressIVMR);
	DDX_Check(pDX, IDC_IGNOREGRAPHERRORS, cTarget->m_IgnoreGraphErrors);
	DDX_Check(pDX, IDC_NOLAVFILTERS, cTarget->m_NoLAVFilters);

}

BEGIN_MESSAGE_MAP(CTabSysLibs, CDialog)
	//{{AFX_MSG_MAP(CTabLogs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabLogs message handlers
