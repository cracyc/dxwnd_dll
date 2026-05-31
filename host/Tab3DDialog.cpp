// TabDirect3D.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "Tab3D.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTab3D dialog

CTab3D::CTab3D(CWnd* pParent /*=NULL*/)
	: CDialog(CTab3D::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTab3D)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTab3D::PreTranslateMessage(MSG *pMsg)
{
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

void CTab3D::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	// Texture management
	DDX_Radio(pDX, IDC_TEXTURENONE, cTarget->m_TextureHandling);
	DDX_Radio(pDX, IDC_BMPFORMAT, cTarget->m_TextureFileFormat);

	// 3D Effects
	DDX_Check(pDX, IDC_NOTEXTURES, cTarget->m_NoTextures);
	DDX_Check(pDX, IDC_WIREFRAME, cTarget->m_WireFrame);
	DDX_Check(pDX, IDC_DISABLEFOGGING, cTarget->m_DisableFogging);
	DDX_Check(pDX, IDC_CONTROLFOGGING, cTarget->m_ControlFogging);
	DDX_Check(pDX, IDC_SETFOGCOLOR, cTarget->m_SetFogColor);
}

BEGIN_MESSAGE_MAP(CTab3D, CDialog)
	//{{AFX_MSG_MAP(CTab3D)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTab3D message handlers

