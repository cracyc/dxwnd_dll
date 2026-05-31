// TabConfKeys.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabConfKeys.h"
#include "CGlobalSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern Key_Type FKeys[];
extern Key_Type HKeys[];
extern Key_Type SKeys[];
extern KeyCombo_Type FKeyCombo[];
extern KeyCombo_Type HKeyCombo[];
extern KeyCombo_Type SKeyCombo[];

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabConfKeys::CTabConfKeys(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabConfKeys::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabConfKeys)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabConfKeys::PreTranslateMessage(MSG *pMsg)
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

void CTabConfKeys::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CGlobalSettings *cTarget = ((CGlobalSettings *)(this->GetParent()->GetParent()));
	for(int i=0; FKeys[i].iLabelResourceId; i++){
		CComboBox *pCombo = (CComboBox *)GetDlgItem(FKeys[i].iComboResourceId);
		DDX_CBIndex(pDX, FKeys[i].iComboResourceId, FKeys[i].cursor);
	}
	for(int i=0; HKeys[i].iLabelResourceId; i++){
		CComboBox *pCombo = (CComboBox *)GetDlgItem(HKeys[i].iComboResourceId);
		DDX_CBIndex(pDX, HKeys[i].iComboResourceId, HKeys[i].cursor);
	}
	for(int i=0; SKeys[i].iLabelResourceId; i++){
		CComboBox *pCombo = (CComboBox *)GetDlgItem(SKeys[i].iComboResourceId);
		DDX_CBIndex(pDX, SKeys[i].iComboResourceId, SKeys[i].cursor);
	}
}

BOOL CTabConfKeys::OnInitDialog(void)
{
	extern char gInitPath[];
	for(int i=0; FKeys[i].iLabelResourceId; i++){
		int iCursor = 0;
		DWORD dwKey;
		FKeys[i].cursor = 0;
		dwKey = GetPrivateProfileInt("keymapping", FKeys[i].sIniLabel, -1, gInitPath);
		SetDlgItemText(FKeys[i].iLabelResourceId,FKeys[i].sLabel);
		CComboBox *pCombo=(CComboBox *)GetDlgItem(FKeys[i].iComboResourceId);
		pCombo->Clear();
		for(int j=0; FKeyCombo[j].dwVKeyCode; j++) {
			pCombo->AddString(FKeyCombo[j].sVKeyLabel);
			if(dwKey == FKeyCombo[j].dwVKeyCode) iCursor=j;
		}
		FKeys[i].cursor = iCursor;
		pCombo->SetCurSel(iCursor);
	}
	for(int i=0; HKeys[i].iLabelResourceId; i++){
		int iCursor = 0;
		DWORD dwKey;
		dwKey = GetPrivateProfileInt("keymapping", HKeys[i].sIniLabel, -1, gInitPath);
		SetDlgItemText(HKeys[i].iLabelResourceId,HKeys[i].sLabel);
		CComboBox *pCombo=(CComboBox *)GetDlgItem(HKeys[i].iComboResourceId);
		pCombo->Clear();
		for(int j=0; HKeyCombo[j].dwVKeyCode; j++) {
			pCombo->AddString(HKeyCombo[j].sVKeyLabel);
			if(dwKey == HKeyCombo[j].dwVKeyCode) iCursor=j;
		}
		HKeys[i].cursor = iCursor;
		pCombo->SetCurSel(iCursor);
	}
	for(int i=0; SKeys[i].iLabelResourceId; i++){
		int iCursor = 0;
		DWORD dwKey;
		dwKey = GetPrivateProfileInt("keymapping", SKeys[i].sIniLabel, -1, gInitPath);
		SetDlgItemText(SKeys[i].iLabelResourceId,SKeys[i].sLabel);
		CComboBox *pCombo=(CComboBox *)GetDlgItem(SKeys[i].iComboResourceId);
		pCombo->Clear();
		for(int j=0; SKeyCombo[j].dwVKeyCode; j++) {
			pCombo->AddString(SKeyCombo[j].sVKeyLabel);
			if(dwKey == SKeyCombo[j].dwVKeyCode) iCursor=j;
		}
		SKeys[i].cursor = iCursor;
		pCombo->SetCurSel(iCursor);
	}
	return TRUE;
}

BEGIN_MESSAGE_MAP(CTabConfKeys, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers


