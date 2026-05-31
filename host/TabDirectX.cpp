// TabDirectX.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabDirectX.h"
extern void OutTrace(const char *, ...);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern dxw_Filter_Type *GetFilterList(void);
extern dxw_Renderer_Type *GetRendererList(void);

#define FullModeTIMER 24

/////////////////////////////////////////////////////////////////////////////
// CTabDirectX dialog

CTabDirectX::CTabDirectX(CWnd* pParent /*=NULL*/)
	: CDialog(CTabDirectX::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabDirectX)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_FilterPos = 0;
	m_RendererPos = 0;
	m_Renderers = GetRendererList();
	OutTrace("CDialog::CTabDirectX\n");
}

void CTabDirectX::OnTimer(UINT_PTR nIDEvent)
{
	//CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	//CComboBox *cInjectors = (CComboBox *)this->GetDlgItem(IDC_INJECTION_MODE);
	//cInjectors->EnableWindow(cTarget->m_EarlyHook); 
	//OutTrace("timer\n");
	extern BOOL isWindowizeEnabled;
	ShowHideDependencies2(!isWindowizeEnabled);
	//CComboBox *cInjectors = (CComboBox *)this->GetDlgItem(IDC_INJECTION_MODE);
	//cInjectors->EnableWindow(isEarlyHookEnabled); 
}

BOOL CTabDirectX::PreTranslateMessage(MSG *pMsg)
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

void CTabDirectX::DoDataExchange(CDataExchange* pDX)
{
	OutTrace("CTabDirectX::DoDataExchange\n");
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	m_RendererPos = 0;
	for (int i=0; m_Renderers[i].name; i++){
		if(m_Renderers[i].id == cTarget->m_RendererId) {
			m_RendererPos = i;
			break;
		}
	}
	OutTrace("CTabDirectX::DoDataExchange(1) RendererPos=%d\n", m_RendererPos);
	int pos = 0;
	m_FilterPos = 0;
	for (int i=0; m_FilterList[i].name; i++){
		if(m_FilterList[i].mask & m_Renderers[m_RendererPos].mask) {
			if(m_FilterList[i].id == cTarget->m_FilterId) {
				m_FilterPos = pos;
				break;
			}
			pos++;
		}
	}

	ShowHideDependencies(cTarget->m_VSyncMode == 1);
	//ShowHideDependencies2(cTarget->m_Windowize );
	extern BOOL isWindowizeEnabled;
	ShowHideDependencies2(isWindowizeEnabled);

	DDX_CBIndex(pDX, IDC_DX_VERSION, cTarget->m_DXVersion);
	DDX_Check(pDX, IDC_BILINEARFILTER, cTarget->m_BilinearFilter);
	DDX_Check(pDX, IDC_BLACKWHITE, cTarget->m_BlackWhite);
	DDX_Check(pDX, IDC_ASYNCBLITMODE, cTarget->m_AsyncBlitMode);
	DDX_Check(pDX, IDC_AUTOREFRESH, cTarget->m_AutoRefresh);
	DDX_Check(pDX, IDC_INDEPENDENTREFRESH, cTarget->m_IndependentRefresh);
	DDX_Check(pDX, IDC_TEXTUREFORMAT, cTarget->m_TextureFormat);
	DDX_Check(pDX, IDC_SUPPRESSRELEASE, cTarget->m_SuppressRelease);
	DDX_Check(pDX, IDC_VIDEOTOSYSTEMMEM, cTarget->m_VideoToSystemMem);
	DDX_Check(pDX, IDC_SUPPRESSDXERRORS, cTarget->m_SuppressDXErrors);
	DDX_Check(pDX, IDC_NOPALETTEUPDATE, cTarget->m_NoPaletteUpdate);
	DDX_Check(pDX, IDC_NOPIXELFORMAT, cTarget->m_NoPixelFormat);
	DDX_Check(pDX, IDC_NOALPHACHANNEL, cTarget->m_NoAlphaChannel);
	DDX_Check(pDX, IDC_OFFSCREENZBUFFER, cTarget->m_OffscreenZBuffer);
	DDX_Check(pDX, IDC_NOZBUFFERATTACH, cTarget->m_NoZBufferAttach);
	//DDX_Check(pDX, IDC_EMULATEOVERLAY, cTarget->m_EmulateOverlay);
	//DDX_Check(pDX, IDC_TRANSPARENTOVERLAY, cTarget->m_TransparentOverlay);
	//DDX_Check(pDX, IDC_TEXTUREPALETTE, cTarget->m_TexturePalette);
	DDX_Check(pDX, IDC_SETCOMPATIBILITY, cTarget->m_SetCompatibility);
	DDX_Check(pDX, IDC_AEROBOOST, cTarget->m_AEROBoost);
	DDX_CBIndex(pDX, IDC_FILTER_ID, m_FilterPos);
	DDX_CBIndex(pDX, IDC_RENDERER_ID, m_RendererPos);
	DDX_Radio(pDX, IDC_OVERLAY_NONE, cTarget->m_OverlayMode);
	DDX_Radio(pDX, IDC_FLIP_NONE, cTarget->m_FlipMode);

	// Vsync
	DDX_Radio(pDX, IDC_VSYNCDEFAULT, cTarget->m_VSyncMode);
	DDX_Radio(pDX, IDC_VSYNCHW, cTarget->m_VSyncImpl);
	DDX_Text(pDX, IDC_SCANLINE, cTarget->m_ScanLine);

	// coop level
	DDX_Radio(pDX, IDC_COOP_DEFAULT, cTarget->m_CoopMode);

	cTarget->m_RendererId = m_Renderers[m_RendererPos].id;
	OutTrace("CTabDirectX::DoDataExchange(2) RendererPos=%d m_RendererId=%d\n", m_RendererPos, cTarget->m_RendererId);
	pos = 0;
	cTarget->m_FilterId = 0;
	for(int i=0; m_FilterList[i].name; i++){
		if(m_FilterList[i].mask & m_Renderers[m_RendererPos].mask) {	
			OutTrace("CTabDirectX::DoDataExchange: i=%d pos=%d id=%d\n", i, pos, m_FilterList[i].id);
			if(pos == m_FilterPos) {
				cTarget->m_FilterId = m_FilterList[i].id;
				break;
			}
			pos++;
		}
	}
	OutTrace("CTabDirectX::DoDataExchange: m_FilterPos=%d m_FilterID=%d\n", m_FilterPos, cTarget->m_FilterId);
}

BOOL CTabDirectX::OnInitDialog()
{
	OutTrace("CTabDirectX::OnInitDialog\n");
	m_FilterList = GetFilterList();
	m_Renderers = GetRendererList();
	CDialog::OnInitDialog();

	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	CComboBox *cRenderers = (CComboBox *)this->GetDlgItem(IDC_RENDERER_ID);
	cRenderers->ResetContent();
	int pos = 0;
	for (int i=0; m_Renderers[i].name; i++) {
		cRenderers->AddString(m_Renderers[i].name);
		if(m_Renderers[i].id == cTarget->m_RendererId) m_RendererPos = i;
	}
	OutTrace("CTabDirectX::OnInitDialog: m_RendererId=%d m_RendererPos=%d\n", cTarget->m_RendererId, m_RendererPos);
	//cRenderers->SetCurSel(cTarget->m_RendererId);
	cRenderers->SetCurSel(m_RendererPos);

	OnCbnSelchangeRendererId();
	SetTimer(FullModeTIMER, 1000, NULL);

	OutTrace("CTabDirectX::OnInitDialog: END m_FilterPos=%d m_FilterID=%d\n", m_FilterPos, cTarget->m_FilterId);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CTabDirectX, CDialog)
	//{{AFX_MSG_MAP(CTabDirectX)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_RENDERER_ID, &CTabDirectX::OnCbnSelchangeRendererId)
	ON_BN_CLICKED(IDC_VSYNCDEFAULT, &CTabDirectX::OnBnClickedVsyncdefault)
	ON_BN_CLICKED(IDC_FORCEVSYNC, &CTabDirectX::OnBnClickedForcevsync)
	ON_BN_CLICKED(IDC_FORCENOVSYNC, &CTabDirectX::OnBnClickedForcenovsync)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabDirectX message handlers


void CTabDirectX::OnCbnSelchangeRendererId()
{
	OutTrace("CTabDirectX::OnCbnSelchangeRendererId\n");
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	CComboBox *cFilters = (CComboBox *)this->GetDlgItem(IDC_FILTER_ID);
	dxw_Renderer_Type *m_Renderers;
	int RendererPos;

	CComboBox *cRenderers = (CComboBox *)this->GetDlgItem(IDC_RENDERER_ID);
	m_Renderers = GetRendererList();
	RendererPos = cRenderers->GetCurSel();
	cFilters->EnableWindow(m_Renderers[RendererPos].mask ? 1 : 0);
	//OutTrace("CTabDirectX::OnCbnSelchangeRendererId: m_FilterID=%d\n", cTarget->m_FilterId);

	cFilters->ResetContent();
	int pos = 0;
	m_FilterPos = 0;
	for (int i=0; m_FilterList[i].name; i++){
		if(m_FilterList[i].mask & m_Renderers[RendererPos].mask) {
			cFilters->AddString(m_FilterList[i].name);
			if(m_FilterList[i].id == cTarget->m_FilterId) m_FilterPos = pos;
			pos++;
		}
	}
	cFilters->SetCurSel(m_FilterPos);

	dxw_Renderer_Type *pRenderer = &m_Renderers[RendererPos];
	CWnd *pAsync = this->GetDlgItem(IDC_ASYNCBLITMODE);
	//pAsync->ShowWindow(pRenderer->pAsyncBlitter && pRenderer->pSyncBlitter);
	pAsync->EnableWindow(pRenderer->pAsyncBlitter && pRenderer->pSyncBlitter);
	//OutTrace("CTabDirectX::OnCbnSelchangeRendererId: m_FilterPos=%d m_FilterID=%d\n", m_FilterPos, cTarget->m_FilterId);
}

void CTabDirectX::OnBnClickedVsyncdefault()
{
	// TODO: Add your control notification handler code here
	ShowHideDependencies(FALSE);
}

void CTabDirectX::OnBnClickedForcevsync()
{
	// TODO: Add your control notification handler code here
	ShowHideDependencies(TRUE);
}

void CTabDirectX::OnBnClickedForcenovsync()
{
	// TODO: Add your control notification handler code here
	ShowHideDependencies(FALSE);
}

void CTabDirectX::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_VSYNCHW)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_VSYNCSCANLINE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_VSYNCSLEEP)->EnableWindow(enable);
}

void CTabDirectX::ShowHideDependencies2(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_COOP_DEFAULT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_COOP_NORMAL)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_COOP_EXCLUSIVE)->EnableWindow(enable);
}