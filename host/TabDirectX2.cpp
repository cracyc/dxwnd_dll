// TabDirectX2.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabDirectX2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabDirectX2 dialog

CTabDirectX2::CTabDirectX2(CWnd* pParent /*=NULL*/)
	: CDialog(CTabDirectX2::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabDirectX2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabDirectX2::PreTranslateMessage(MSG *pMsg)
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

void CTabDirectX2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	// Ddraw tweaks
	// DDX_Check(pDX, IDC_NOSYSMEMPRIMARY, cTarget->m_NoSysMemPrimary);
	//DDX_Check(pDX, IDC_NOSYSMEMBACKBUF, cTarget->m_NoSysMemBackBuf);
	DDX_Check(pDX, IDC_FIXPITCH, cTarget->m_FixPitch);
	DDX_Check(pDX, IDC_POWER2WIDTH, cTarget->m_Power2Width);
	DDX_Check(pDX, IDC_FIXREFCOUNTER, cTarget->m_FixRefCounter);
	DDX_Check(pDX, IDC_MINIMALCAPS, cTarget->m_MinimalCaps);
	DDX_Check(pDX, IDC_SETZBUFFERBITDEPTHS, cTarget->m_SetZBufferBitDepths);
	DDX_Check(pDX, IDC_LIMITDDRAW, cTarget->m_LimitDdraw);
	DDX_Check(pDX, IDC_SUPPRESSOVERLAY, cTarget->m_SuppressOverlay);
	DDX_Check(pDX, IDC_USERGB565, cTarget->m_UseRGB565);
	DDX_CBIndex(pDX, IDC_DDWAWLIMITCOMBO, cTarget->m_MaxDdrawInterface);
	DDX_Check(pDX, IDC_CLEARTEXTUREFOURCC, cTarget->m_ClearTextureFourCC);
	DDX_Check(pDX, IDC_SUPPRESSFOURCCBLT, cTarget->m_SuppressFourCCBlt);
	DDX_Check(pDX, IDC_CREATEDESKTOP, cTarget->m_CreateDesktop);
	DDX_Check(pDX, IDC_SAFEPALETTEUSAGE, cTarget->m_SafePaletteUsage);
	DDX_Check(pDX, IDC_LOADGAMMARAMP, cTarget->m_LoadGammaRamp);
	DDX_Check(pDX, IDC_FULLRECTBLT, cTarget->m_FullRectBlt);
	DDX_Check(pDX, IDC_FIXDCPALETTE, cTarget->m_FixDCPalette);
	DDX_Check(pDX, IDC_MERGEFRONTBUFFERS, cTarget->m_MergeFrontBuffers);
	DDX_Check(pDX, IDC_RECOVEREXCLUSIVEERR, cTarget->m_RecoverExclusiveErr);
	DDX_Check(pDX, IDC_FORCEDX1HOOK, cTarget->m_ForceDx1Hook);
	DDX_Check(pDX, IDC_SURFACEUNLOCK, cTarget->m_SurfaceUnlock);
	DDX_Check(pDX, IDC_EMULATECLIPPER, cTarget->m_EmulateClipper);
	DDX_Check(pDX, IDC_USEBLTFAST, cTarget->m_UseBltFast);
	DDX_Check(pDX, IDC_PLAINBACKBUFFER, cTarget->m_PlainBackBuffer);
	DDX_Radio(pDX, IDC_WAITDEFAULT, cTarget->m_WaitMode);
	DDX_Check(pDX, IDC_DISABLEMAXWINMODE, cTarget->m_DisableMaxWinMode);

	// Clipper
	DDX_Radio(pDX, IDC_CLIPPERNONE, cTarget->m_ClipperMode);

	// FourCC handling
	DDX_Radio(pDX, IDC_NOFOURCC, cTarget->m_FourCCMode);

}

BEGIN_MESSAGE_MAP(CTabDirectX2, CDialog)
	//{{AFX_MSG_MAP(CTabDirectX2)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	//ON_BN_CLICKED(IDC_VSYNCDEFAULT, &CTabDirectX2::OnBnClickedVsyncdefault)
	//ON_BN_CLICKED(IDC_FORCEVSYNC, &CTabDirectX2::OnBnClickedForcevsync)
	//ON_BN_CLICKED(IDC_FORCENOVSYNC, &CTabDirectX2::OnBnClickedForcenovsync)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabDirectX2 message handlers

