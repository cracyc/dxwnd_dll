// TabDirect3D.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabDirect3D.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabDirect3D dialog

CTabDirect3D::CTabDirect3D(CWnd* pParent /*=NULL*/)
	: CDialog(CTabDirect3D::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabDirect3D)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabDirect3D::PreTranslateMessage(MSG *pMsg)
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

void CTabDirect3D::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	// Direct3D tweaks
	DDX_Check(pDX, IDC_ZBUFFERCLEAN, cTarget->m_ZBufferClean);
	DDX_Check(pDX, IDC_ZBUFFER0CLEAN, cTarget->m_ZBuffer0Clean);
	DDX_Check(pDX, IDC_DYNAMICZCLEAN, cTarget->m_DynamicZClean);
	DDX_Check(pDX, IDC_ZBUFFERHARDCLEAN, cTarget->m_ZBufferHardClean);
	DDX_Check(pDX, IDC_NOPOWER2FIX, cTarget->m_NoPower2Fix);
	DDX_Check(pDX, IDC_NOD3DRESET, cTarget->m_NoD3DReset);
	DDX_Check(pDX, IDC_SUPPRESSD3DEXT, cTarget->m_SuppressD3DExt);
	DDX_Check(pDX, IDC_ENUM16BITMODES, cTarget->m_Enum16bitModes);
	DDX_Check(pDX, IDC_TRIMTEXTUREFORMATS, cTarget->m_TrimTextureFormats);
	DDX_Check(pDX, IDC_FORCED3DGAMMARAMP, cTarget->m_ForceD3DGammaRamp);
	DDX_Check(pDX, IDC_LIGHTGAMMARAMP, cTarget->m_LightGammaRamp);
	DDX_Check(pDX, IDC_TRANSFORMANDLIGHT, cTarget->m_TransformAndLight);
	DDX_Check(pDX, IDC_CLEARTARGET, cTarget->m_ClearTarget);
	DDX_Check(pDX, IDC_D3D8BACK16, cTarget->m_D3D8Back16);
	DDX_Check(pDX, IDC_STRETCHFULLRECT, cTarget->m_StretchFullRect);
	DDX_Check(pDX, IDC_COLORKEYTOALPHA, cTarget->m_ColorKeyToAlpha);
	DDX_Check(pDX, IDC_FORCEWBASEDFOG, cTarget->m_ForceWBasedFog);
	DDX_Check(pDX, IDC_NOMULTISAMPLE, cTarget->m_NoMultiSample);
	DDX_Check(pDX, IDC_FORCEVBUFONSYSMEM, cTarget->m_ForceVBufOnSysMem);
	DDX_Check(pDX, IDC_ZBUFONSYSMEMORY, cTarget->m_ZBufOnSysMemory);
	DDX_Check(pDX, IDC_ZBUFONVIDMEMORY, cTarget->m_ZBufOnVidMemory);
	DDX_Check(pDX, IDC_FILTERRGBTEXTURES, cTarget->m_FilterRGBTextures);
	DDX_Check(pDX, IDC_NOD3DOPTIMIZE, cTarget->m_NoD3DOptimize);
	DDX_Check(pDX, IDC_TRIMD3DPOINTS, cTarget->m_TrimD3DPoints);
}

BEGIN_MESSAGE_MAP(CTabDirect3D, CDialog)
	//{{AFX_MSG_MAP(CTabDirect3D)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabDirect3D message handlers

