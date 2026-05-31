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

//typedef struct {
//	char *name;
//	int xfactor;
//	int yfactor;
//} filter_type;

/////////////////////////////////////////////////////////////////////////////
// CTabDirectX dialog

CTabDirectX::CTabDirectX(CWnd* pParent /*=NULL*/)
	: CDialog(CTabDirectX::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabDirectX)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_FilterPos = 0;
	OutTrace("CDialog::CTabDirectX\n");
}

BOOL CTabDirectX::PreTranslateMessage(MSG *pMsg)
{
	//if(((pMsg->message == WM_KEYDOWN) || (pMsg->message == WM_KEYUP))
	if((pMsg->message == WM_KEYDOWN) 
		&& (pMsg->wParam == VK_RETURN)) {
			return TRUE;
	}
	return CWnd::PreTranslateMessage(pMsg);
}

void CTabDirectX::DoDataExchange(CDataExchange* pDX)
{
	OutTrace("CTabDirectX::DoDataExchange\n");
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	//dxw_Filter_Type *filters;
	dxw_Renderer_Type *renderers;
	renderers = GetRendererList();

	int pos = 0;
	m_FilterPos = 0;
	for (int i=0; m_FilterList[i].name; i++){
		if(m_FilterList[i].mask & renderers[cTarget->m_RendererId].mask) {
			//cFilters->AddString(m_FilterList[i].name);
			if(m_FilterList[i].id == cTarget->m_FilterId) m_FilterPos = pos;
			pos++;
		}
	}
	//cFilters->SetCurSel(m_FilterPos);



	DDX_Radio(pDX, IDC_AUTO, cTarget->m_DXVersion);
	//DDX_Radio(pDX, IDC_NOEMULATESURFACE, cTarget->m_DxEmulationMode);
	DDX_Check(pDX, IDC_BILINEARFILTER, cTarget->m_BilinearFilter);
	DDX_Check(pDX, IDC_BLUREFFECT, cTarget->m_BlurEffect);
	DDX_Check(pDX, IDC_BLACKWHITE, cTarget->m_BlackWhite);
	DDX_Check(pDX, IDC_AUTOREFRESH, cTarget->m_AutoRefresh);
	DDX_Check(pDX, IDC_INDEPENDENTREFRESH, cTarget->m_IndependentRefresh);
	DDX_Check(pDX, IDC_TEXTUREFORMAT, cTarget->m_TextureFormat);
	DDX_Check(pDX, IDC_SUPPRESSRELEASE, cTarget->m_SuppressRelease);
	DDX_Check(pDX, IDC_VIDEOTOSYSTEMMEM, cTarget->m_VideoToSystemMem);
	DDX_Check(pDX, IDC_SUPPRESSDXERRORS, cTarget->m_SuppressDXErrors);
	DDX_Check(pDX, IDC_NOPALETTEUPDATE, cTarget->m_NoPaletteUpdate);
	DDX_Check(pDX, IDC_NOPIXELFORMAT, cTarget->m_NoPixelFormat);
	DDX_Check(pDX, IDC_NOALPHACHANNEL, cTarget->m_NoAlphaChannel);
	DDX_Check(pDX, IDC_NOFLIPEMULATION, cTarget->m_NoFlipEmulation);
	DDX_Check(pDX, IDC_OFFSCREENZBUFFER, cTarget->m_OffscreenZBuffer);
	DDX_Check(pDX, IDC_NOZBUFFERATTACH, cTarget->m_NoZBufferAttach);
	DDX_Check(pDX, IDC_TEXTUREPALETTE, cTarget->m_TexturePalette);
	DDX_Check(pDX, IDC_FLIPEMULATION, cTarget->m_FlipEmulation);
	DDX_Check(pDX, IDC_SETCOMPATIBILITY, cTarget->m_SetCompatibility);
	DDX_Check(pDX, IDC_AEROBOOST, cTarget->m_AEROBoost);
	DDX_CBIndex(pDX, IDC_FILTER_ID, m_FilterPos);
	DDX_CBIndex(pDX, IDC_RENDERER_ID, cTarget->m_RendererId);

	////cTarget->m_FilterId = (int)&m_FilterList[m_FilterPos].id;
	//int pos = 0;
	//cTarget->m_FilterId = 0;
	//for (int i=0; m_FilterList[i].name; i++){
	//	if(m_FilterList[i].mask & m_FilterList[cTarget->m_RendererId].mask) {
	//		if(pos++ == m_FilterPos) {
	//			cTarget->m_FilterId = m_FilterList[i].id;
	//			break;
	//		}
	//	}
	//}

	//int pos = 0;
	////m_FilterPos = 0;
	//for (int i=0; m_FilterList[i].name; i++){
	//	if(m_FilterList[i].mask & renderers[RendererId].mask) {
	//		pos++;
	//		//cFilters->AddString(m_FilterList[i].name);
	//		//if(m_FilterList[i].id == cTarget->m_FilterId) m_FilterPos = pos;
	//	}
	//}
	//cFilters->SetCurSel(m_FilterPos);

	pos = 0;
	cTarget->m_FilterId = 0;
	for(int i=0; m_FilterList[i].name; i++){
		if(m_FilterList[i].mask & renderers[cTarget->m_RendererId].mask) {	
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
	m_FilterList = GetFilterList();
	CDialog::OnInitDialog();

	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	CComboBox *cFilters = (CComboBox *)this->GetDlgItem(IDC_FILTER_ID);
	OutTrace("CTabDirectX::OnInitDialog: BEGIN m_FilterPos=%d m_FilterID=%d\n", m_FilterPos, cTarget->m_FilterId);
	dxw_Renderer_Type *renderers;

	//cFilters->ResetContent();
	//filters = GetFilterList();
	//for (int i=0; filters[i].name; i++){
	//	cFilters->AddString(filters[i].name);
	//}
	//cFilters->SetCurSel(cTarget->m_FilterId);
	CComboBox *cRenderers = (CComboBox *)this->GetDlgItem(IDC_RENDERER_ID);
	cRenderers->ResetContent();
	renderers = GetRendererList();
	for (int i=0; renderers[i].name; i++){
		cRenderers->AddString(renderers[i].name);
	}
	cRenderers->SetCurSel(cTarget->m_RendererId);
	cFilters->EnableWindow(renderers[cTarget->m_RendererId].mask ? 1 : 0);

	m_FilterPos = 0;
	int pos = 0;
	cFilters->ResetContent();
	for (int i=0; m_FilterList[i].name; i++){
		if(m_FilterList[i].mask & renderers[cTarget->m_RendererId].mask) {
			OutTrace("CTabDirectX::OnInitDialog: adding filter[%d] name=%s\n", i, m_FilterList[i].name);
			cFilters->AddString(m_FilterList[i].name);
			if(m_FilterList[i].id == cTarget->m_FilterId) m_FilterPos = pos;
			pos ++;
		}
	}
	cFilters->SetCurSel(m_FilterPos);
	//cTarget->m_FilterId = (int)&m_FilterList[m_FilterId].id;

	OutTrace("CTabDirectX::OnInitDialog: END m_FilterPos=%d m_FilterID=%d\n", m_FilterPos, cTarget->m_FilterId);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CTabDirectX, CDialog)
	//{{AFX_MSG_MAP(CTabDirectX)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_RENDERER_ID, &CTabDirectX::OnCbnSelchangeRendererId)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabDirectX message handlers


void CTabDirectX::OnCbnSelchangeRendererId()
{
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	CComboBox *cFilters = (CComboBox *)this->GetDlgItem(IDC_FILTER_ID);
	dxw_Renderer_Type *renderers;
	int RendererId;

	CComboBox *cRenderers = (CComboBox *)this->GetDlgItem(IDC_RENDERER_ID);
	renderers = GetRendererList();
	RendererId = cRenderers->GetCurSel();
	cFilters->EnableWindow(renderers[RendererId].mask ? 1 : 0);

	cFilters->ResetContent();
	int pos = 0;
	m_FilterPos = 0;
	for (int i=0; m_FilterList[i].name; i++){
		if(m_FilterList[i].mask & renderers[RendererId].mask) {
			pos++;
			cFilters->AddString(m_FilterList[i].name);
			if(m_FilterList[i].id == cTarget->m_FilterId) m_FilterPos = pos;
		}
	}
	cFilters->SetCurSel(m_FilterPos);
	//cTarget->m_FilterId = (int)&m_FilterList[m_FilterPos].id;
	OutTrace("CTabDirectX::OnCbnSelchangeRendererId: m_FilterPos=%d m_FilterID=%d\n", m_FilterPos, cTarget->m_FilterId);

}
