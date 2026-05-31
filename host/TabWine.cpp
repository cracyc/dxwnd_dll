// TabWine.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabWine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void OutTrace(const char *, ...);
extern BOOL gbWineConfig;

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabWine::CTabWine(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabWine::IDD, pParent)
{
	//OutTrace("CDialog::CTabWine\n");
	//{{AFX_DATA_INIT(CTabWine)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabWine::PreTranslateMessage(MSG *pMsg)
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

void CTabWine::DoDataExchange(CDataExchange* pDX)
{
	OutTrace("CTabWine::DoDataExchange\n");
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
		
	DDX_Check(pDX, IDC_DIBPALETTE, cTarget->m_DIBPalette);
}

BEGIN_MESSAGE_MAP(CTabWine, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	//ON_BN_CLICKED(IDC_HOOKSDLLIB, &CTabWine::OnBnClickedHooksdllib)
	//ON_BN_CLICKED(IDC_HOOKSDL2LIB, &CTabWine::OnBnClickedHooksdl2lib)
END_MESSAGE_MAP()

BOOL CTabWine::OnInitDialog()
{
	OutTrace("CTabWine::OnInitDialog\n");
	CDialog::OnInitDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers



//void CTabWine::OnBnClickedHooksdllib()
//{
//	// TODO: Add your control notification handler code here
//	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
//	cTarget->m_HookSDLLib = !cTarget->m_HookSDLLib;
//	BOOL enable = cTarget->m_HookSDLLib || cTarget->m_HookSDL2Lib;
//	ShowHideDependencies(enable);
//}
//
//void CTabWine::OnBnClickedHooksdl2lib()
//{
//	// TODO: Add your control notification handler code here
//	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
//	cTarget->m_HookSDL2Lib = !cTarget->m_HookSDL2Lib;
//	BOOL enable = cTarget->m_HookSDLLib || cTarget->m_HookSDL2Lib;
//	ShowHideDependencies(enable);
//}

void CTabWine::ShowHideDependencies(BOOL enable)
{
	//(CButton *)this->GetDlgItem(IDC_SDLFIXMOUSE)->EnableWindow(enable);
}

