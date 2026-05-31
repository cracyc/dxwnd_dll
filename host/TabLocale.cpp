// TabLocale.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabLocale.h"
#include "dxwndhost.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef enum {
	MODE_VOID = 0,
	MODE_CODEPAGE,
	MODE_LOCALE
};

typedef int (WINAPI *LCIDToLocaleName_Type)(LCID, LPWSTR, int, DWORD);
LCIDToLocaleName_Type pLCIDToLocaleName = NULL;

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabLocale::CTabLocale(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabLocale::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabLocale)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// do not link to LCIDToLocaleName statically for sake of compatibility
	// with WinXP where the call does not exist
	pLCIDToLocaleName = NULL;
	HMODULE hKernel32 = LoadLibrary("Kernel32.dll");
	if(hKernel32 == NULL) return;
	pLCIDToLocaleName = (LCIDToLocaleName_Type)GetProcAddress(hKernel32, "LCIDToLocaleName");
}

BOOL CTabLocale::OnInitDialog()
{
	CDialog::OnInitDialog();
	sPresets = (char *)malloc(1024+1);
	GetPrivateProfileString(NULL, NULL, NULL, sPresets, 1024, "./presets.ini");
	CComboBox *cPresets = (CComboBox *)this->GetDlgItem(IDC_LOCALE_PRESETS);
	cPresets->AddString((LPCSTR)"-- custom --");
	for(char *p=sPresets; *p; p+=(strlen(p)+1)){
		OutTrace("Presets=%s\n", p);
		cPresets->AddString((LPCSTR)p);
	}
	free(sPresets);
	return TRUE;
}

CTabLocale::~CTabLocale()
{
}

void CTabLocale::UpdateCodePage(UINT CodePage)
{
	CHAR Name[(2*LOCALE_NAME_MAX_LENGTH)+1];
	CPINFOEXA CPInfoEx;
	if(::GetCPInfoEx(CodePage, 0, &CPInfoEx)){
		sprintf(Name, "%s", CPInfoEx.CodePageName);
	}
	else{
		sprintf(Name, "invalid codepage err:%d", GetLastError());
	}
	this->SetDlgItemTextA(IDC_CODEPAGE_TEXT, Name);
}

void CTabLocale::UpdateLocaleID(UINT LocaleID)
{
	// v2.05.18: LCIDToLocaleName dynamic loading to stay compatible with WinXP
	WCHAR WName[LOCALE_NAME_MAX_LENGTH+1];
	CHAR Name[(2*LOCALE_NAME_MAX_LENGTH)+1];
	if(pLCIDToLocaleName == NULL) return;
	if((*pLCIDToLocaleName)(LocaleID, WName, LOCALE_NAME_MAX_LENGTH, 0)){
		sprintf(Name, "%ls", WName);
	}
	else{
		sprintf(Name, "invalid locale err:%d", GetLastError());
	}
	this->SetDlgItemTextA(IDC_LANGUAGE_TEXT, Name);
}

BOOL CTabLocale::PreTranslateMessage(MSG *pMsg)
{
	if((pMsg->message == WM_KEYDOWN) 
		&& (pMsg->wParam == VK_RETURN)) {
			this->UpdateData();
			return TRUE;
	}
    // Added to enable keyboard navigation
    if(IsDialogMessage(pMsg) && (pMsg->wParam != VK_ESCAPE)) {
        return TRUE;
    }
	return CWnd::PreTranslateMessage(pMsg);
}

void CTabLocale::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	BOOL enable = cTarget->m_CustomLocale;
	ShowHideDependencies(enable);

	DDX_Check(pDX, IDC_CUSTOMLOCALE, cTarget->m_CustomLocale);
	DDX_Check(pDX, IDC_CLASSLOCALE, cTarget->m_ClassLocale);
	DDX_Check(pDX, IDC_PATHLOCALE, cTarget->m_PathLocale);
	DDX_Check(pDX, IDC_NOSETTEXT, cTarget->m_NoSetText);
	DDX_Check(pDX, IDC_FIXKEYBOARDTYPE, cTarget->m_FixKeyboardType);
	DDX_Text(pDX, IDC_COUNTRY, cTarget->m_Country);
	DDX_Text(pDX, IDC_LANGUAGE, cTarget->m_Locale);
	DDX_Text(pDX, IDC_CODEPAGE, cTarget->m_CodePage);

	UpdateCodePage(cTarget->m_CodePage);
	UpdateLocaleID(cTarget->m_Locale);
}

BEGIN_MESSAGE_MAP(CTabLocale, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CODEPAGE_BUTTON, &CTabLocale::OnBnClickedCodepageButton)
	ON_LBN_KILLFOCUS(IDC_LISTHELPER, &CTabLocale::OnLbnKillfocusListhelper)
	ON_LBN_DBLCLK(IDC_LISTHELPER, &CTabLocale::OnLbnDblclkListhelper)
	ON_BN_CLICKED(IDC_LOCALE_BUTTON, &CTabLocale::OnBnClickedLocaleButton)
	ON_BN_CLICKED(IDC_CUSTOMLOCALE, &CTabLocale::OnBnClickedCustomlocale)
	ON_CBN_SELCHANGE(IDC_LOCALE_PRESETS, &CTabLocale::OnCbnSelchangeLocalePresets)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers

CListBox *pHelper;

BOOL CALLBACK CodePageEnumProc(LPTSTR lpCodePageString)
{
	if(lpCodePageString == NULL) return FALSE;
	CString line;
	int iCodePage;
	CPINFOEXA CPInfoEx;
	sscanf(lpCodePageString, "%d", &iCodePage);
	if(::GetCPInfoEx(iCodePage, 0, &CPInfoEx)){
		// blank pad to have a proper sorting order
		if(iCodePage < 10) line.SetString("    ");
		else if(iCodePage < 100) line.SetString("   ");
		else if(iCodePage < 1000) line.SetString("  ");
		else if(iCodePage < 10000) line.SetString(" ");
		line.Append(CPInfoEx.CodePageName);
		pHelper->AddString(line);
	}
	return TRUE;
}

BOOL CALLBACK LocaleEnumProc(LPTSTR lpLocaleString)
{
	if(lpLocaleString == NULL) return FALSE;
	CString line;
	UINT iLCID;
	sscanf(lpLocaleString, "%d", &iLCID);
	WCHAR WName[LOCALE_NAME_MAX_LENGTH+1];
	CHAR Name[(2*LOCALE_NAME_MAX_LENGTH)+1];
	if(pLCIDToLocaleName == NULL) return FALSE;
	if((*pLCIDToLocaleName)(iLCID, WName, LOCALE_NAME_MAX_LENGTH, 0)){
		sprintf(Name, "%ls", WName);
	}
	else{
		strcpy(Name, "???");
	}
	// blank pad to have a proper sorting order
	line.Format("%4.4d: ", iLCID);
	line.Append(Name);
	pHelper->AddString(line);
	return TRUE;
}

void CTabLocale::OnBnClickedCodepageButton()
{
	// TODO: Add your control notification handler code here
	pHelper = (CListBox *)this->GetDlgItem(IDC_LISTHELPER);
	pHelper->ResetContent();
	EnumSystemCodePagesA(CodePageEnumProc, CP_SUPPORTED);
	pHelper->ShowWindow(TRUE);
	pHelper->EnableWindow(TRUE);
	pHelper->SetFocus();
	HelperMode = MODE_CODEPAGE;
}

void CTabLocale::OnBnClickedLocaleButton()
{
	// TODO: Add your control notification handler code here
	pHelper = (CListBox *)this->GetDlgItem(IDC_LISTHELPER);
	pHelper->ResetContent();
#ifdef USEENUMERATION
	EnumSystemLocalesA(LocaleEnumProc, LCID_SUPPORTED);
#else
	for(int iLCID=0; iLCID<2000; iLCID++){
		WCHAR WName[LOCALE_NAME_MAX_LENGTH+1];
		CHAR Name[(2*LOCALE_NAME_MAX_LENGTH)+1];
		if(pLCIDToLocaleName == NULL) return;
		if((*pLCIDToLocaleName)(iLCID, WName, LOCALE_NAME_MAX_LENGTH, 0)){
			sprintf(Name, "%.4d: %ls", iLCID, WName);
			pHelper->AddString(Name);
		}
	}
#endif
	pHelper->ShowWindow(TRUE);
	pHelper->EnableWindow(TRUE);
	pHelper->SetFocus();
	HelperMode = MODE_LOCALE;
}

void CTabLocale::OnLbnKillfocusListhelper()
{
	// TODO: Add your control notification handler code here
	pHelper = (CListBox *)this->GetDlgItem(IDC_LISTHELPER);
	pHelper->ResetContent();
	pHelper->ShowWindow(FALSE);
	pHelper->EnableWindow(FALSE);
	HelperMode = MODE_VOID;
}

void CTabLocale::OnLbnDblclkListhelper()
{
	// TODO: Add your control notification handler code here
	CString selection;
	UINT iCodePage;
	UINT iLCID;
	pHelper = (CListBox *)this->GetDlgItem(IDC_LISTHELPER);
	pHelper->GetText(pHelper->GetCurSel(), selection);
	//MessageBox(selection, 0, 0);
	if(HelperMode == MODE_CODEPAGE){
		selection.TrimLeft();
		if (sscanf(selection, "%d", &iCodePage) == 1){
			CEdit *cp = (CEdit *)this->GetDlgItem(IDC_CODEPAGE);
			selection.Format("%d", iCodePage);
			cp->SetWindowText(selection);
			this->UpdateCodePage(iCodePage);
			pHelper->ShowWindow(FALSE);
			pHelper->EnableWindow(FALSE);
		}
	}
	else if(HelperMode == MODE_LOCALE){
		selection.TrimLeft();
		if (sscanf(selection, "%d", &iLCID) == 1){
			CEdit *cp = (CEdit *)this->GetDlgItem(IDC_LANGUAGE);
			selection.Format("%d", iLCID);
			cp->SetWindowText(selection);
			this->UpdateLocaleID(iLCID);
			pHelper->ShowWindow(FALSE);
			pHelper->EnableWindow(FALSE);
		}
	}
}

void CTabLocale::OnBnClickedCustomlocale()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_CustomLocale = !cTarget->m_CustomLocale;
	BOOL enable = cTarget->m_CustomLocale;
	ShowHideDependencies(enable);
}

void CTabLocale::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_CLASSLOCALE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_PATHLOCALE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_NOSETTEXT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FIXKEYBOARDTYPE)->EnableWindow(enable);
}
void CTabLocale::OnCbnSelchangeLocalePresets()
{
	char buf[80];
	CComboBox *cPresets = (CComboBox *)this->GetDlgItem(IDC_LOCALE_PRESETS);
	int iSelection = cPresets->GetCurSel();
	if(iSelection == 0) {
		//MessageBox("no preset", "dxwnd", 0);
		return;
	}
	char sSelection[80];
	cPresets->GetLBText(iSelection, sSelection);
	DWORD iLocale = GetPrivateProfileInt(sSelection, "locale", 0, "./presets.ini");
	DWORD iCodePage = GetPrivateProfileInt(sSelection, "codepage", 0, "./presets.ini");
	CEdit *cCountry = (CEdit *)this->GetDlgItem(IDC_COUNTRY);
	CEdit *cLocale = (CEdit *)this->GetDlgItem(IDC_LANGUAGE);
	CEdit *cCodePage = (CEdit *)this->GetDlgItem(IDC_CODEPAGE);
	sprintf(buf, "%d", iLocale);
	cLocale->SetWindowTextA(buf);
	sprintf(buf, "%d", iCodePage);
	cCodePage->SetWindowTextA(buf);
	char debug[80];
	sprintf(debug,"loc=%d cp=%d", iLocale, iCodePage);
	this->UpdateCodePage(iCodePage);
	this->UpdateLocaleID(iLocale);
}
