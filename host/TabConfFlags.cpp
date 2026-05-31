// TabConfFlags.cpp : implementation file
//

#include "stdafx.h"
#include "afxdlgs.h"
#include "TargetDlg.h"
#include "TabConfFlags.h"
#include "CGlobalSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern char gInitPath[];

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabConfFlags::CTabConfFlags(CWnd* pParent /*=NULL*/)
	: CDialog(CTabConfFlags::IDD, pParent)
{
}

BOOL CTabConfFlags::PreTranslateMessage(MSG *pMsg)
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

void CTabConfFlags::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CGlobalSettings *cTarget = ((CGlobalSettings *)(this->GetParent()->GetParent()));
	DDX_Check(pDX, IDC_CONFIG_DEBUGMODE, cTarget->m_DebugMode);
	DDX_Check(pDX, IDC_CONFIG_CHECKADMIN, cTarget->m_CheckAdminRights);
	DDX_Check(pDX, IDC_CONFIG_NAMEFROMFOLDER, cTarget->m_NameFromFolder);
	DDX_Check(pDX, IDC_CONFIG_MULTIHOOKS, cTarget->m_MultiHooks);
	DDX_Check(pDX, IDC_CONFIG_WARNONEXIT, cTarget->m_WarnOnExit);
	DDX_Check(pDX, IDC_CONFIG_SAVEPATHS, cTarget->m_UpdatePaths);
	DDX_Check(pDX, IDC_CONFIG_32BITICONS, cTarget->m_32BitIcons);
	DDX_Check(pDX, IDC_CONFIG_GRAYICONS, cTarget->m_GrayIcons);
	DDX_Check(pDX, IDC_CONFIG_AUTOSAVE, cTarget->m_AutoSave);
	DDX_Check(pDX, IDC_CONFIG_AUTOHIDE, cTarget->m_AutoHideMode);
	DDX_Check(pDX, IDC_CONFIG_HIDEONESCAPE, cTarget->m_HideOnEscape);
	DDX_Check(pDX, IDC_CONFIG_STRIPROOT, cTarget->m_StripRoot);
	DDX_Check(pDX, IDC_CONFIG_CHECKRUNNING, cTarget->m_CheckRunning);
	DDX_Check(pDX, IDC_CONFIG_RELPATH, cTarget->m_RelativePath);
	DDX_Check(pDX, IDC_CONFIG_EXPORTDEBUGFLAGS, cTarget->m_DebugFlags);
	DDX_CBIndex(pDX, IDC_OVERLAY_POS, cTarget->m_OverlayPosition);
	DDX_CBIndex(pDX, IDC_OVERLAY_STYLE, cTarget->m_OverlayStyle);
	DDX_Check(pDX, IDC_CONFIG_NAKEDDUMP, cTarget->m_NakedDump);
	DDX_Check(pDX, IDC_CONFIG_TIMEDDUMP, cTarget->m_TimedDump);
	DDX_Check(pDX, IDC_CONFIG_WINEDEBUG, cTarget->m_WineDebug);
}

BEGIN_MESSAGE_MAP(CTabConfFlags, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CTabConfFlags::OnBnClickedSelectRootFolder)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers


#include "FolderDlg.h"
void CTabConfFlags::OnBnClickedSelectRootFolder()
{
	extern char gProgramRootFolder[];
    CFolderDialog dlg(  _T( "Select a root folder" ), gProgramRootFolder, this );
    if( dlg.DoModal() == IDOK  )
    {    
        strcpy(gProgramRootFolder, dlg.GetFolderPath());
    }    
}
