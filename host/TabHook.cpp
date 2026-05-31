// TabHook.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabHook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabHook dialog

CTabHook::CTabHook(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabHook::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabHook)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

#define HookModeTIMER 23

BOOL CTabHook::OnInitDialog()
{
	typedef struct{
		char *name;
	} injector;
	injector m_Injectors[] = {
		{"Window hook" },
		{"Debugger" },
		{"Inject DLL" },
		{"Proxy DLL" },
		{"Inject APC" },
		{"Detours" },
		//{"Inject APC2" }, - not working so far
		{ 0 } // terminator
	};

	char *SonModes[] = {
		"Default",
		"Suppress",
		"Ext. debug",
		"Ext. inject",
		"Ext. detours",
		0 // terminator
	};

	char *IATScanModes[] = {
		"Word-aligned",
		"Byte-aligned",
		"Scan EXE PE",
		"No IAT scan",
		0 // terminator
	};

	CDialog::OnInitDialog();

	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	CComboBox *cInjectors = (CComboBox *)this->GetDlgItem(IDC_INJECTION_MODE);
	cInjectors->ResetContent();
	for (int i=0; m_Injectors[i].name; i++) {
		cInjectors->AddString(m_Injectors[i].name);
	}
	cInjectors->SetCurSel(cTarget->m_InjectionMode);

	CComboBox *cSonModes = (CComboBox *)this->GetDlgItem(IDC_SONPROCESS_MODE);
	cSonModes->ResetContent();
	for (int i=0; SonModes[i]; i++) {
		cSonModes->AddString(SonModes[i]);
	}
	cSonModes->SetCurSel(cTarget->m_SonProcessMode);

	CComboBox *cHookModes = (CComboBox *)this->GetDlgItem(IDC_HOOKMODE);
	cHookModes->SetCurSel(cTarget->m_HookMode);

	CComboBox *cScanModes = (CComboBox *)this->GetDlgItem(IDC_IATSCAN_MODE);
	cScanModes->ResetContent();
	for (int i=0; IATScanModes[i]; i++) {
		cScanModes->AddString(IATScanModes[i]);
	}
	cScanModes->SetCurSel(cTarget->m_IATAlignedMode);
	SetTimer(HookModeTIMER, 1000, NULL);

	return TRUE;
}

void CTabHook::OnTimer(UINT_PTR nIDEvent)
{
	//CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	//CComboBox *cInjectors = (CComboBox *)this->GetDlgItem(IDC_INJECTION_MODE);
	//cInjectors->EnableWindow(cTarget->m_EarlyHook); 
	extern BOOL isEarlyHookEnabled;
	CComboBox *cInjectors = (CComboBox *)this->GetDlgItem(IDC_INJECTION_MODE);
	cInjectors->EnableWindow(isEarlyHookEnabled); 
}

BOOL CTabHook::PreTranslateMessage(MSG *pMsg)
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

void CTabHook::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	DDX_Text(pDX, IDC_MODULE, cTarget->m_Module);
	DDX_Text(pDX, IDC_CMDLINE, cTarget->m_CmdLine);
	DDX_Text(pDX, IDC_MSSHIMS, cTarget->m_MSShims);
	DDX_Check(pDX, IDC_HOOKENABLED, cTarget->m_HookEnabled);
	DDX_Check(pDX, IDC_RUNBYDXWND, cTarget->m_RunByDxWnd);
	DDX_CBIndex(pDX, IDC_INJECTION_MODE, cTarget->m_InjectionMode);
	DDX_Check(pDX, IDC_HOOKNORUN, cTarget->m_HookNoRun);
	DDX_Check(pDX, IDC_ENABLEPATCHER, cTarget->m_EnablePatcher);
	DDX_Check(pDX, IDC_HOTREGISTRY, cTarget->m_HotRegistry);
	DDX_Check(pDX, IDC_HOOKNOUPDATE, cTarget->m_HookNoUpdate);
	DDX_Check(pDX, IDC_HOOKCHILDWIN, cTarget->m_HookChildWin);
	DDX_Check(pDX, IDC_HOOKDLGWIN, cTarget->m_HookDlgWin);
	DDX_Check(pDX, IDC_SHOWHINTS, cTarget->m_ShowHints);
	DDX_Check(pDX, IDC_HIDEWINDOWCHANGES, cTarget->m_HideWindowChanges);
	DDX_Check(pDX, IDC_FRONTEND, cTarget->m_Frontend);

	// IAT Alignment
	//DDX_Radio(pDX, IDC_IATWORDALIGNED, cTarget->m_IATAlignedMode);
	DDX_CBIndex(pDX, IDC_IATSCAN_MODE, cTarget->m_IATAlignedMode);
	DDX_Check(pDX, IDC_SKIPIATHINT, cTarget->m_SkipIATHint);

	// Kernel32
	//DDX_Radio(pDX, IDC_SONDEFAULT, cTarget->m_SonProcessMode);
	DDX_CBIndex(pDX, IDC_SONPROCESS_MODE, cTarget->m_SonProcessMode);
	DDX_CBIndex(pDX, IDC_HOOKMODE, cTarget->m_HookMode);
	DDX_Check(pDX, IDC_SHAREDHOOK, cTarget->m_SharedHook);

	// Page Commit
	DDX_Check(pDX, IDC_COMMITPAGE, cTarget->m_CommitPage);
	DDX_Text(pDX, IDC_COMMITADDRESS, cTarget->m_CommitAddress);
	DDX_Text(pDX, IDC_COMMITLENGTH, cTarget->m_CommitLength);

	//CComboBox *cInjectors = (CComboBox *)this->GetDlgItem(IDC_INJECTION_MODE);
	//if(cTarget->m_EarlyHook){
	//	cInjectors->EnableWindow(TRUE);
	//	cInjectors->SetCurSel(cTarget->m_InjectionMode);
	//}
	//else 
	//	cInjectors->EnableWindow(FALSE);

}

BEGIN_MESSAGE_MAP(CTabHook, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabHook message handlers
