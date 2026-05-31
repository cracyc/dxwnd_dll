// TabDirectX.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabCompat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabCompat::CTabCompat(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabCompat::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabCompat)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabCompat::PreTranslateMessage(MSG *pMsg)
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

void CTabCompat::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	ShowHideDependencies(cTarget->m_HandleExceptions == 1);

	DDX_Check(pDX, IDC_FAKEVERSION, cTarget->m_FakeVersion);
	DDX_LBIndex(pDX, IDC_LISTFAKE, cTarget->m_FakeVersionId);
	DDX_Check(pDX, IDC_SINGLEPROCAFFINITY, cTarget->m_SingleProcAffinity);
	DDX_Check(pDX, IDC_USELASTCORE, cTarget->m_UseLastCore);
	DDX_Check(pDX, IDC_HANDLEEXCEPTIONS, cTarget->m_HandleExceptions);
	DDX_Check(pDX, IDC_FORCENOPS, cTarget->m_ForceNOPs);
	DDX_Check(pDX, IDC_FONTBYPASS, cTarget->m_FontBypass);
	DDX_Check(pDX, IDC_NOPERFCOUNTER, cTarget->m_NoPerfCounter);
	DDX_Check(pDX, IDC_LEGACYALLOC, cTarget->m_LegacyAlloc);
	DDX_Check(pDX, IDC_NOIMAGEHLP, cTarget->m_NoImagehlp);
	DDX_Check(pDX, IDC_BLOCKPRIORITYCLASS, cTarget->m_BlockPriorityClass);
	DDX_Check(pDX, IDC_COLORFIX, cTarget->m_ColorFix);
	DDX_Check(pDX, IDC_FIXFREELIBRARY, cTarget->m_FixFreeLibrary);
	DDX_Check(pDX, IDC_SKIPFREELIBRARY, cTarget->m_SkipFreeLibrary);
	DDX_Check(pDX, IDC_LOADLIBRARYERR, cTarget->m_LoadLibraryErr);
	DDX_Check(pDX, IDC_FIXALTEREDPATH, cTarget->m_FixAlteredPath);
	DDX_Check(pDX, IDC_FIXADJUSTWINRECT, cTarget->m_FixAdjustWinRect);
	DDX_Check(pDX, IDC_PRETENDVISIBLE, cTarget->m_PretendVisible);
	DDX_Check(pDX, IDC_WININSULATION, cTarget->m_WinInsulation);
	DDX_Check(pDX, IDC_DISABLEMMX, cTarget->m_DisableMMX);
	DDX_Check(pDX, IDC_FAKEPENTIUM3, cTarget->m_FakePentium3);
	DDX_Check(pDX, IDC_SAFEALLOCS, cTarget->m_SafeAllocs);
	DDX_Check(pDX, IDC_HOOKLEGACYEVENTS, cTarget->m_HookLegacyEvents);
	DDX_Check(pDX, IDC_HOOKLEGACYWAVE, cTarget->m_HookLegacyWave);
	DDX_Check(pDX, IDC_LIMITFREEDISK, cTarget->m_LimitFreeDisk);
	DDX_Check(pDX, IDC_LIMITFREERAM, cTarget->m_LimitFreeRAM);
	DDX_Check(pDX, IDC_LIMITVIDEORAM, cTarget->m_LimitVideoRAM);
	DDX_Check(pDX, IDC_LIMITTEXTURERAM, cTarget->m_LimitTextureRAM);
	DDX_Check(pDX, IDC_EMULATEWINHELP, cTarget->m_EmulateWinHelp);
	DDX_Check(pDX, IDC_SUPPRESSDEP, cTarget->m_SuppressDEP);
	DDX_Check(pDX, IDC_DISABLEDEP, cTarget->m_DisableDEP);
	// GOG patches
	DDX_Check(pDX, IDC_HOOKGOGLIBS, cTarget->m_HookGOGLibs);
	DDX_Check(pDX, IDC_BYPASSGOGLIBS, cTarget->m_BypassGOGLibs);
}

BEGIN_MESSAGE_MAP(CTabCompat, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_HANDLEEXCEPTIONS, &CTabCompat::OnBnClickedHandleexceptions)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers

static struct {char bMajor; char bMinor; char *sName;} WinVersions[9]=
{
	{4, 0, "Windows 95"},
	{4,10, "Windows 98/SE"},
	{4,90, "Windows ME"},
	{5, 0, "Windows 2000"},
	{5, 1, "Windows XP"},
	{5, 2, "Windows Server 2003"},
	{6, 0, "Windows Vista"},
	{6, 1, "Windows 7"},
	{6, 2, "Windows 8"}
};

BOOL CTabCompat::OnInitDialog()
{
	AfxEnableControlContainer();
	CListBox *List;
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	int i;
	List=(CListBox *)this->GetDlgItem(IDC_LISTFAKE);
	List->ResetContent();
	for(i=0; i<9; i++) List->AddString(WinVersions[i].sName);
	List->SetCurSel(cTarget->m_FakeVersion);
	CDialog::OnInitDialog();
	return TRUE;
}

void CTabCompat::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_SUPPRESSDEP)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FORCENOPS)->EnableWindow(enable);
}

void CTabCompat::OnBnClickedHandleexceptions()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HandleExceptions = !cTarget->m_HandleExceptions;
	BOOL enable = cTarget->m_HandleExceptions;
	ShowHideDependencies(enable);
}
