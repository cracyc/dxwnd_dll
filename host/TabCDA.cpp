// TabCDA.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabCDA.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern char AvailableCDDrives[];

/////////////////////////////////////////////////////////////////////////////
// CTabCDA dialog

CTabCDA::CTabCDA(CWnd* pParent /*=NULL*/)
	: CDialog(CTabCDA::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabCDA)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabCDA::PreTranslateMessage(MSG *pMsg)
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

void CTabCDA::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	ShowHideDependencies(cTarget->m_CDEmulationMode != 0);

	// CDA
	DDX_Radio(pDX, IDC_CDA_NONE, cTarget->m_CDEmulationMode);
	DDX_Radio(pDX, IDC_EMULATEWIN9XMCI, cTarget->m_CDEmulationVersion);

	DDX_Check(pDX, IDC_SETCDVOLUME, cTarget->m_SetCDVolume);
	//DDX_Check(pDX, IDC_FORCEMCISYSINFO, cTarget->m_ForceMCISysInfo);
	DDX_Check(pDX, IDC_FORCETRACKREPEAT, cTarget->m_ForceTrackRepeat);
	DDX_Check(pDX, IDC_IGNOREMCIDEVID, cTarget->m_IgnoreMCIDevId);
	DDX_Check(pDX, IDC_CDROMPRESENT, cTarget->m_CDROMPresent);
	DDX_Check(pDX, IDC_HACKMCIFRAMES, cTarget->m_HackMCIFrames);
	DDX_Check(pDX, IDC_CDPAUSECAPABILITY, cTarget->m_CDPauseCapability);
	DDX_Check(pDX, IDC_SUPPRESSCDAUDIO, cTarget->m_SuppressCDAudio);
	DDX_Check(pDX, IDC_EMULATECDMIXER, cTarget->m_EmulateCDMixer);
	DDX_Check(pDX, IDC_EMULATECDAUX, cTarget->m_EmulateCDAux);

	DDX_CBIndex(pDX, IDC_CDA_DRIVE, cTarget->m_CDADriveId);

	//OutTrace("cTarget->m_CDADriveId=%d @%d\n", cTarget->m_CDADriveId, __LINE__);
	if((cTarget->m_CDADriveId > 0) && (cTarget->m_CDADriveId <= ('Z' - 'A')))
		cTarget->m_CDADriveLetter = AvailableCDDrives[cTarget->m_CDADriveId];
	else 
		cTarget->m_CDADriveLetter = '?';
}

BOOL CTabCDA::OnInitDialog()
{
	AfxEnableControlContainer();

	CComboBox *cDrives = (CComboBox *)this->GetDlgItem(IDC_CDA_DRIVE);
	cDrives->ResetContent();
	for(int i=0; AvailableCDDrives[i]; i++){
		if(AvailableCDDrives[i] == '?') cDrives->AddString("def.");
		else {
			char sDrivePath[3];
			sprintf(sDrivePath, "%c:", AvailableCDDrives[i]);
			cDrives->AddString(sDrivePath);
		}
	}

	CDialog::OnInitDialog();
	return TRUE;
}


BEGIN_MESSAGE_MAP(CTabCDA, CDialog)
	//{{AFX_MSG_MAP(CTabCDA)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CDA_NONE, &CTabCDA::OnBnClickedCdaNone)
	ON_BN_CLICKED(IDC_PLAYFROMCD, &CTabCDA::OnBnClickedPlayfromcd)
	ON_BN_CLICKED(IDC_CDAUTOMATICRIP, &CTabCDA::OnBnClickedCdautomaticrip)
	ON_BN_CLICKED(IDC_VIRTUALCDAUDIO, &CTabCDA::OnBnClickedVirtualcdaudio)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCDA message handlers


void CTabCDA::OnBnClickedCdaNone()
{
	// TODO: Add your control notification handler code here
	ShowHideDependencies(FALSE);
}

void CTabCDA::OnBnClickedPlayfromcd()
{
	// TODO: Add your control notification handler code here
	ShowHideDependencies(TRUE);
}

void CTabCDA::OnBnClickedCdautomaticrip()
{
	// TODO: Add your control notification handler code here
	ShowHideDependencies(TRUE);
}

void CTabCDA::OnBnClickedVirtualcdaudio()
{
	// TODO: Add your control notification handler code here
	ShowHideDependencies(TRUE);
}

void CTabCDA::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_EMULATEWIN9XMCI)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_EMULATEWINXPMCI)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_EMULATEVISTAMCI)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FORCETRACKREPEAT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_IGNOREMCIDEVID)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_CDROMPRESENT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_HACKMCIFRAMES)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_CDPAUSECAPABILITY)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SUPPRESSCDAUDIO)->EnableWindow(enable);
}