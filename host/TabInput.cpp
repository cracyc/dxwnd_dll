// TabInput.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabInput.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabInput dialog

CTabInput::CTabInput(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabInput::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabInput)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
		//if(cTarget->m_HookDI){
}

BOOL CTabInput::PreTranslateMessage(MSG *pMsg)
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

void CTabInput::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	BOOL enable = cTarget->m_HookDI || cTarget->m_HookDI8;
	BOOL enable2 = (cTarget->m_HookDI || cTarget->m_HookDI8) && cTarget->m_VirtualJoystick;
	ShowHideDependencies(enable);
	ShowHideDependencies2(enable2);

	// Control keys
	DDX_Radio(pDX, IDC_ALTTAB_DEFAULT, cTarget->m_AltTabMode);
	// DirectInput
	DDX_Check(pDX, IDC_HOOKDI, cTarget->m_HookDI);
	DDX_Check(pDX, IDC_HOOKDI8, cTarget->m_HookDI8);
	DDX_Check(pDX, IDC_UNACQUIRE, cTarget->m_Unacquire);
	DDX_Check(pDX, IDC_EMULATERELMOUSE, cTarget->m_EmulateRelMouse);
	DDX_Check(pDX, IDC_SCALERELMOUSE, cTarget->m_ScaleRelMouse);
	DDX_Check(pDX, IDC_SKIPDEVTYPEHID, cTarget->m_SkipDevTypeHID);
	DDX_Check(pDX, IDC_SUPPRESSDIERRORS, cTarget->m_SuppressDIErrors);
	DDX_Check(pDX, IDC_SHAREDKEYBOARD, cTarget->m_SharedKeyboard);
	DDX_Check(pDX, IDC_SHAREDMOUSE, cTarget->m_SharedMouse);
	DDX_Check(pDX, IDC_PACKMOUSEDATA, cTarget->m_PackMouseData);
	// Joystick
	DDX_Check(pDX, IDC_VIRTUALJOYSTICK, cTarget->m_VirtualJoystick);
	DDX_Check(pDX, IDC_JOYSTICKEFFECTS, cTarget->m_JoystickEffects);
	DDX_Check(pDX, IDC_HIDEJOYSTICKS, cTarget->m_HideJoysticks);
	// Xinput
	DDX_Check(pDX, IDC_HOOKXINPUT, cTarget->m_HookXinput);
	// Keyboard handling
	DDX_Check(pDX, IDC_XBOX2KEYBOARD, cTarget->m_XBox2Keyboard);
	DDX_Check(pDX, IDC_ENABLEHOTKEYS, cTarget->m_EnableHotKeys);
	DDX_Check(pDX, IDC_HANDLEALTF4, cTarget->m_HandleAltF4);
	DDX_Check(pDX, IDC_NODISABLEPRINT, cTarget->m_NoDisablePrint);
	DDX_Check(pDX, IDC_FIXASYNCKEYSTATE, cTarget->m_FixAsyncKeyState);
	DDX_Check(pDX, IDC_FLUSHKEYSTATE, cTarget->m_FlushKeyState);
}

BEGIN_MESSAGE_MAP(CTabInput, CDialog)
	//{{AFX_MSG_MAP(CTabInput)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_HOOKDI, &CTabInput::OnBnClickedHookdi)
	ON_BN_CLICKED(IDC_HOOKDI8, &CTabInput::OnBnClickedHookdi8)
	ON_BN_CLICKED(IDC_VIRTUALJOYSTICK, &CTabInput::OnBnClickedVirtualjoystick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabInput message handlers

void CTabInput::OnBnClickedHookdi()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HookDI = !cTarget->m_HookDI;
	BOOL enable = cTarget->m_HookDI || cTarget->m_HookDI8;
	BOOL enable2 = (cTarget->m_HookDI || cTarget->m_HookDI8) && cTarget->m_VirtualJoystick;
	ShowHideDependencies(enable);
	ShowHideDependencies2(enable2);
}

void CTabInput::OnBnClickedHookdi8()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HookDI8 = !cTarget->m_HookDI8;
	BOOL enable = cTarget->m_HookDI || cTarget->m_HookDI8;
	BOOL enable2 = (cTarget->m_HookDI || cTarget->m_HookDI8) && cTarget->m_VirtualJoystick;
	ShowHideDependencies(enable);
	ShowHideDependencies2(enable2);
}

void CTabInput::OnBnClickedVirtualjoystick()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_VirtualJoystick = !cTarget->m_VirtualJoystick;
	BOOL enable = cTarget->m_HookDI || cTarget->m_HookDI8;
	BOOL enable2 = (cTarget->m_HookDI || cTarget->m_HookDI8) && cTarget->m_VirtualJoystick;
	ShowHideDependencies(enable);
	ShowHideDependencies2(enable2);
}

void CTabInput::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_UNACQUIRE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_EMULATERELMOUSE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SCALERELMOUSE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SKIPDEVTYPEHID)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SUPPRESSDIERRORS)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SHAREDKEYBOARD)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_SHAREDMOUSE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_PACKMOUSEDATA)->EnableWindow(enable);
}

void CTabInput::ShowHideDependencies2(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_JOYSTICKEFFECTS)->EnableWindow(enable);
}

