// TabSDL.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabSDL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabSDL::CTabSDL(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabSDL::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabSDL)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabSDL::PreTranslateMessage(MSG *pMsg)
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

void CTabSDL::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
		
	BOOL enable = cTarget->m_HookSDLLib || cTarget->m_HookSDL2Lib;
	ShowHideDependencies(enable);

	DDX_Check(pDX, IDC_HOOKSDLLIB, cTarget->m_HookSDLLib);
	DDX_Check(pDX, IDC_HOOKSDL2LIB, cTarget->m_HookSDL2Lib);
	DDX_Check(pDX, IDC_EXTENDSDLHOOK, cTarget->m_ExtendSDLHook);
	DDX_Check(pDX, IDC_SDLEMULATION, cTarget->m_SDLEmulation);
	DDX_Check(pDX, IDC_SDLFORCESTRETCH, cTarget->m_SDLForceStretch);
	DDX_Check(pDX, IDC_SDLFIXMOUSE, cTarget->m_SDLFixMouse);
}

BEGIN_MESSAGE_MAP(CTabSDL, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_HOOKSDLLIB, &CTabSDL::OnBnClickedHooksdllib)
	ON_BN_CLICKED(IDC_HOOKSDL2LIB, &CTabSDL::OnBnClickedHooksdl2lib)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers



void CTabSDL::OnBnClickedHooksdllib()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HookSDLLib = !cTarget->m_HookSDLLib;
	BOOL enable = cTarget->m_HookSDLLib || cTarget->m_HookSDL2Lib;
	ShowHideDependencies(enable);
}

void CTabSDL::OnBnClickedHooksdl2lib()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HookSDL2Lib = !cTarget->m_HookSDL2Lib;
	BOOL enable = cTarget->m_HookSDLLib || cTarget->m_HookSDL2Lib;
	ShowHideDependencies(enable);
}

void CTabSDL::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_EXTENDSDLHOOK)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SDLEMULATION)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SDLFORCESTRETCH)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SDLFIXMOUSE)->EnableWindow(enable);
}

