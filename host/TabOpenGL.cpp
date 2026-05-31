// TabOpenGL.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabOpenGL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabOpenGL dialog

CTabOpenGL::CTabOpenGL(CWnd* pParent /*=NULL*/)
	: CDialog(CTabOpenGL::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabOpenGL)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabOpenGL::PreTranslateMessage(MSG *pMsg)
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

void CTabOpenGL::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	BOOL enable = cTarget->m_HookOpenGL;
	ShowHideDependencies(enable);

	// OpenGL
	DDX_Check(pDX, IDC_HOOKOPENGL, cTarget->m_HookOpenGL); 
	DDX_Check(pDX, IDC_FORCEHOOKOPENGL, cTarget->m_ForceHookOpenGL);
	DDX_Check(pDX, IDC_FIXPIXELZOOM, cTarget->m_FixPixelZoom);
	DDX_Check(pDX, IDC_FIXBINDTEXTURE, cTarget->m_FixBindTexture);
	DDX_Check(pDX, IDC_SCALEGLBITMAPS, cTarget->m_ScaleglBitmaps);
	DDX_Check(pDX, IDC_HOOKGLUT32, cTarget->m_HookGlut32);
	DDX_Check(pDX, IDC_HOOKWGLCONTEXT, cTarget->m_HookWGLContext);
	DDX_Check(pDX, IDC_SCALEMAINVIEWPORT, cTarget->m_ScaleMainViewport);
	DDX_Check(pDX, IDC_LOCKGLVIEWPORT, cTarget->m_LockGLViewport);
	DDX_Check(pDX, IDC_GLEXTENSIONSLIE, cTarget->m_GLExtensionsLie);
	DDX_Check(pDX, IDC_GLEXTENSIONSTRIM, cTarget->m_GLExtensionsTrim);
	DDX_Check(pDX, IDC_GLFIXCLAMP, cTarget->m_GLFixClamp);
	DDX_Check(pDX, IDC_SAMPLE2COVERAGE, cTarget->m_Sample2Coverage);
	DDX_Check(pDX, IDC_NOSCISSORTEST, cTarget->m_NoScissorTest);
	DDX_Text (pDX, IDC_OPENGLLIB, cTarget->m_OpenGLLib);
}

BOOL CTabOpenGL::OnInitDialog()
{
	AfxEnableControlContainer();
	CDialog::OnInitDialog();
	return TRUE;
}


BEGIN_MESSAGE_MAP(CTabOpenGL, CDialog)
	//{{AFX_MSG_MAP(CTabOpenGL)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_HOOKOPENGL, &CTabOpenGL::OnBnClickedHookopengl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabOpenGL message handlers

void CTabOpenGL::OnBnClickedHookopengl()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HookOpenGL = !cTarget->m_HookOpenGL;
	BOOL enable = cTarget->m_HookOpenGL;
	ShowHideDependencies(enable);
}

void CTabOpenGL::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_FORCEHOOKOPENGL)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FIXPIXELZOOM)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FIXBINDTEXTURE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SCALEGLBITMAPS)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_HOOKGLUT32)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_HOOKWGLCONTEXT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SCALEMAINVIEWPORT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_LOCKGLVIEWPORT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_GLEXTENSIONSLIE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_GLEXTENSIONSTRIM)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_GLFIXCLAMP)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_NOSCISSORTEST)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SAMPLE2COVERAGE)->EnableWindow(enable);
}