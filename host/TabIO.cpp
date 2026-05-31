// TabIO.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabIO.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabIO dialog

CTabIO::CTabIO(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabIO::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabIO)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabIO::PreTranslateMessage(MSG *pMsg)
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

void CTabIO::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	DDX_Check(pDX, IDC_CDROMDRIVETYPE, cTarget->m_CDROMDriveType);
	DDX_Check(pDX, IDC_BUFFEREDIOFIX, cTarget->m_BufferedIOFix);
	DDX_Check(pDX, IDC_HIDECDROMEMPTY, cTarget->m_HideCDROMEmpty);
	DDX_Check(pDX, IDC_HIDECDROMREAL, cTarget->m_HideCDROMReal);
	DDX_Check(pDX, IDC_FAKEHDDRIVE, cTarget->m_FakeHD);
	DDX_Check(pDX, IDC_FAKECDDRIVE, cTarget->m_FakeCD);
	DDX_Check(pDX, IDC_REMAPSYSFOLDERS, cTarget->m_RemapSysFolders);
	DDX_Check(pDX, IDC_RESETMULTIVOLUME, cTarget->m_ResetMultiVolume);
	DDX_Check(pDX, IDC_REMAPROOTFOLDER, cTarget->m_RemapRootFolder);
	DDX_Check(pDX, IDC_HANDLECDLOCK, cTarget->m_HandleCDLock);
	DDX_Check(pDX, IDC_SETCDAUDIOPATH, cTarget->m_SetCDAudioPath);
	DDX_Check(pDX, IDC_CDHACK, cTarget->m_CDHack);
	DDX_Text(pDX, IDC_FAKEHDPATH, cTarget->m_FakeHDPath);
	DDX_Text(pDX, IDC_FAKECDPATH, cTarget->m_FakeCDPath);
	DDX_Text(pDX, IDC_FAKECDLABEL, cTarget->m_FakeCDLabel);
	DDX_Text(pDX, IDC_COMBO_HD, cTarget->m_FakeHDDrive);
	DDX_Text(pDX, IDC_COMBO_CD, cTarget->m_FakeCDDrive);
	DDX_Text(pDX, IDC_STARTFOLDER, cTarget->m_StartFolder);
	DDX_Check(pDX, IDC_USESHORTPATH, cTarget->m_UseShortPath);

}

BEGIN_MESSAGE_MAP(CTabIO, CDialog)
	//{{AFX_MSG_MAP(CTabIO)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabIO message handlers

BOOL IsWinXP()
{
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	return osvi.dwMajorVersion == 5;
}

BOOL CTabIO::OnInitDialog()
{
	CDialog::OnInitDialog();
	CWnd* Pfield = GetDlgItem(IDC_USESHORTPATH);
	if(!IsWinXP()){
		Pfield->EnableWindow(FALSE); 
		Pfield->ShowWindow(SW_HIDE); 
	}
	return TRUE;
}
