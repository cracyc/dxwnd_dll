// TabDirectX.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabTiming.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern char gInitPath[];

/////////////////////////////////////////////////////////////////////////////
// CTabTiming dialog


CTabTiming::CTabTiming(CWnd* pParent /*=NULL*/)
	: CDialog(CTabTiming::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabTiming)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabTiming::PreTranslateMessage(MSG *pMsg)
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

void CTabTiming::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	ShowHideDependencies(cTarget->m_TimeStretch);
	ShowHideDependencies2(cTarget->m_LimitFPS);

	DDX_Check(pDX, IDC_LIMITFPS, cTarget->m_LimitFPS);
	DDX_Check(pDX, IDC_LIMITDIBOPERATIONS, cTarget->m_LimitDIBOperations);
	DDX_Check(pDX, IDC_SKIPFPS, cTarget->m_SkipFPS);
	DDX_Check(pDX, IDC_LIMITFLIPONLY, cTarget->m_LimitFlipOnly);
	DDX_Check(pDX, IDC_LIMITBEGINSCENE, cTarget->m_LimitBeginScene);
	DDX_Check(pDX, IDC_SHOWTIMESTRETCH, cTarget->m_ShowTimeStretch);
	DDX_Check(pDX, IDC_TIMESTRETCH, cTarget->m_TimeStretch);
	DDX_Check(pDX, IDC_INTERCEPTRDTSC, cTarget->m_InterceptRDTSC);
	DDX_Check(pDX, IDC_STRETCHTIMERS, cTarget->m_StretchTimers);
	DDX_Check(pDX, IDC_NORMALIZEPERFCOUNT, cTarget->m_NormalizePerfCount);
	DDX_Check(pDX, IDC_STRETCHPERFREQUENCY, cTarget->m_StretchPerFrequency);
	DDX_Check(pDX, IDC_CPUSLOWDOWN, cTarget->m_CPUSlowDown);
	DDX_Check(pDX, IDC_CPUMAXUSAGE, cTarget->m_CPUMaxUsage);
	DDX_Check(pDX, IDC_PRECISETIMING, cTarget->m_PreciseTiming);
	DDX_Check(pDX, IDC_SETPRIORITYLOW, cTarget->m_SetPriorityLow);
	DDX_Check(pDX, IDC_FINETIMING, cTarget->m_FineTiming);
	DDX_Check(pDX, IDC_USENANOSLEEP, cTarget->m_UseNanosleep);
	DDX_Check(pDX, IDC_CUSTOMTIMESHIFT, cTarget->m_CustomTimeShift);
	DDX_Check(pDX, IDC_LIMITFREQUENCY, cTarget->m_LimitFrequency);
	DDX_Check(pDX, IDC_TIMEFREEZE, cTarget->m_EnableTimeFreeze);
	DDX_Text(pDX, IDC_MAXFPS, cTarget->m_MaxFPS);
	DDX_Text(pDX, IDC_SLOWRATIO, cTarget->m_SlowRatio);
	DDX_LBIndex(pDX, IDC_LISTTS, cTarget->m_InitTS);
	DDX_Check(pDX, IDC_SLOW, cTarget->m_SlowDown);
	DDX_Check(pDX, IDC_SLOWWINPOLLING, cTarget->m_SlowWinPolling);
	DDX_Check(pDX, IDC_SUSPENDTIMESTRETCH, cTarget->m_SuspendTimeStretch);
	DDX_Check(pDX, IDC_KILLVSYNC, cTarget->m_KillVSync);
	DDX_Check(pDX, IDC_FAKEDATE, cTarget->m_FakeDate);
	DDX_Check(pDX, IDC_UPTIMECLEAR, cTarget->m_UpTimeClear);
	DDX_Check(pDX, IDC_UPTIMESTRESS, cTarget->m_UpTimeStress);

	//{{AFX_DATA_MAP(CTabTiming)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BOOL CTabTiming::OnInitDialog()
{

	AfxEnableControlContainer();

	CListBox *List;
	CDialog::OnInitDialog();
	int i;
	char sFakeDate[80+1];
	extern char *GetTSCaption(int);
	List=(CListBox *)this->GetDlgItem(IDC_LISTTS);
	List->ResetContent();
	for(i=-8; i<=8; i++) List->AddString(GetTSCaption(i));
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	List->SetCurSel(cTarget->m_InitTS);
	GetPrivateProfileString("window", "fakedate", "1/1/1986", sFakeDate, 80, gInitPath);
	CWnd *label = GetDlgItem(IDC_CURRENTFAKEDATE);
	label->SetWindowText(sFakeDate);

	return TRUE;
}


BEGIN_MESSAGE_MAP(CTabTiming, CDialog)
	//{{AFX_MSG_MAP(CTabTiming)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_TIMESTRETCH, &CTabTiming::OnBnClickedTimestretch)
	ON_BN_CLICKED(IDC_LIMITFPS, &CTabTiming::OnBnClickedLimitfps)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabTiming message handlers

void CTabTiming::OnBnClickedTimestretch()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_TimeStretch = !cTarget->m_TimeStretch;
	BOOL enable = cTarget->m_TimeStretch;
	ShowHideDependencies(enable);
}

void CTabTiming::OnBnClickedLimitfps()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_LimitFPS = !cTarget->m_LimitFPS;
	BOOL enable = cTarget->m_LimitFPS;
	ShowHideDependencies2(enable);
}

void CTabTiming::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_SHOWTIMESTRETCH )->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FINETIMING)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_TIMEFREEZE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_STRETCHTIMERS)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_INTERCEPTRDTSC)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_STRETCHPERFREQUENCY)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_NORMALIZEPERFCOUNT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SUSPENDTIMESTRETCH)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_USENANOSLEEP)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_CUSTOMTIMESHIFT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FAKEDATE)->EnableWindow(enable);
}

void CTabTiming::ShowHideDependencies2(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_LIMITFLIPONLY)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_LIMITBEGINSCENE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_LIMITDIBOPERATIONS)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_LIMITFREQUENCY)->EnableWindow(enable);
}
