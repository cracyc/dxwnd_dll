// TabWDDM.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabWDDM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabWDDM dialog

CTabWDDM::CTabWDDM(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabWDDM::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabWDDM)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabWDDM::PreTranslateMessage(MSG *pMsg)
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

void CTabWDDM::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	BOOL enable = cTarget->m_HookVideoAdapter;
	ShowHideDependencies(enable);

	// D3DDDI interface
	DDX_Check(pDX, IDC_HOOKVIDEOADAPTER, cTarget->m_HookVideoAdapter);
	DDX_Check(pDX, IDC_ALTPIXELCENTER, cTarget->m_AltPixelCenter);
	DDX_Check(pDX, IDC_FIXCOLORKEY, cTarget->m_FixColorKey);
	DDX_Check(pDX, IDC_ENABLEZOOMING, cTarget->m_EnableZooming);
	DDX_Check(pDX, IDC_TRIMVERTEXBUFFER, cTarget->m_TrimVertexBuffer);
	DDX_Check(pDX, IDC_DEPTHBUFZCLEAN, cTarget->m_DepthBufZClean);
	DDX_Check(pDX, IDC_WDDMNOOVERLAY, cTarget->m_WDDMNoOverlay); // this can be set at the app level too !!
	DDX_Check(pDX, IDC_FORCEWBASEDFOGWDDM, cTarget->m_ForceWBasedFogWDDM); // this can be set at the app level too !!
}

void CTabWDDM::OnBnClickedHookWDDM()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HookVideoAdapter = !cTarget->m_HookVideoAdapter;
	BOOL enable = cTarget->m_HookVideoAdapter;
	ShowHideDependencies(enable);
}

void CTabWDDM::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_ALTPIXELCENTER)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FIXCOLORKEY)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_ENABLEZOOMING)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_TRIMVERTEXBUFFER)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_DEPTHBUFZCLEAN)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_WDDMNOOVERLAY)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FORCEWBASEDFOGWDDM)->EnableWindow(enable);
}

BEGIN_MESSAGE_MAP(CTabWDDM, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_HOOKVIDEOADAPTER, &CTabWDDM::OnBnClickedHookWDDM)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers


