// TabDirect3D.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabDirect3D2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabDirect3D dialog

CTabDirect3D2::CTabDirect3D2(CWnd* pParent /*=NULL*/)
	: CDialog(CTabDirect3D2::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabDirect3D)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabDirect3D2::PreTranslateMessage(MSG *pMsg)
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

void CTabDirect3D2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	BOOL enable = cTarget->m_ForcesSwapEffect;
	ShowHideDependencies(enable);


	// Swap Effect
	DDX_Check(pDX, IDC_FORCESWAPEFFECT, cTarget->m_ForcesSwapEffect);
	DDX_Radio(pDX, IDC_SWAP_DISCARD, cTarget->m_SwapEffect);

	// Force vVertex Processing
	DDX_Radio(pDX, IDC_VERTEX_DEFAULT, cTarget->m_VertexProcessing);

	// Device type
	DDX_Radio(pDX, IDC_FORCEDEVTYPE0, cTarget->m_ForceDevType);

	// D3D tweaks
	DDX_Check(pDX, IDC_PATCHEXECUTEBUFFER, cTarget->m_PatchExecuteBuffer);
	DDX_Radio(pDX, IDC_ZBUFDEFAULT, cTarget->m_ZBufferMode);
	DDX_Radio(pDX, IDC_DITHERDEFAULT, cTarget->m_DitherMode);

	// Capabilities
	DDX_Check(pDX, IDC_NORAMPDEVICE, cTarget->m_NoRampDevice);
	DDX_Check(pDX, IDC_NORGBDEVICE, cTarget->m_NoRGBDevice);
	DDX_Check(pDX, IDC_NOMMXDEVICE, cTarget->m_NoMMXDevice);
	DDX_Check(pDX, IDC_NOHALDEVICE, cTarget->m_NoHALDevice);
	DDX_Check(pDX, IDC_NOTNLDEVICE, cTarget->m_NoTnLDevice);
	DDX_Check(pDX, IDC_FOGVERTEXCAP, cTarget->m_FogVertexCap);
	DDX_Check(pDX, IDC_FOGTABLECAP, cTarget->m_FogTableCap);
}

void CTabDirect3D2::OnBnClickedForcesSwapEffect()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_ForcesSwapEffect = !cTarget->m_ForcesSwapEffect;
	BOOL enable = cTarget->m_ForcesSwapEffect;
	ShowHideDependencies(enable);
}

void CTabDirect3D2::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_SWAP_DISCARD)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SWAP_FLIP)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SWAP_COPY)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SWAP_OVERLAY)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SWAP_FLIPEX)->EnableWindow(enable);
}

BEGIN_MESSAGE_MAP(CTabDirect3D2, CDialog)
	//{{AFX_MSG_MAP(CTabDirect3D)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_FORCESWAPEFFECT, &CTabDirect3D2::OnBnClickedForcesSwapEffect)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabDirect3D message handlers

