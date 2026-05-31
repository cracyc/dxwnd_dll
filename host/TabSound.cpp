// TabSound.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabSound.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void OutTrace(const char *, ...);

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabSound::CTabSound(CWnd* pParent /*=NULL*/)
//	: CTargetDlg(pParent)
	: CDialog(CTabSound::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabSound)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CTabSound::PreTranslateMessage(MSG *pMsg)
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

BOOL CTabSound::OnInitDialog()
{
	CDialog::OnInitDialog();
	CSliderCtrl *Slider;

	Slider=(CSliderCtrl *)this->GetDlgItem(IDC_CDSLIDER);
	Slider->SetRange(0, +100, 0);
	Slider->SetTicFreq(10);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CTabSound::DoDataExchange(CDataExchange* pDX)
{
	//OutTrace("CTabSound::DoDataExchange()\n");
	CDialog::DoDataExchange(pDX);
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));

	BOOL enable = cTarget->m_HookDirectSound;
	ShowHideDependencies(enable);

	DDX_Check(pDX, IDC_HOOKDIRECTSOUND, cTarget->m_HookDirectSound);
	DDX_Check(pDX, IDC_SOUNDMUTE, cTarget->m_SoundMute);
	DDX_Check(pDX, IDC_NOSOUNDLOOP, cTarget->m_NoSoundLoop);
	DDX_Check(pDX, IDC_FIXDSOUNDBUFFER, cTarget->m_FixDSoundBuffer);
	DDX_Check(pDX, IDC_BYPASSDSOUND, cTarget->m_BypassDSound);
	DDX_Check(pDX, IDC_NOWAVEOUT, cTarget->m_NoWaveOut);
	DDX_Check(pDX, IDC_MCISINGLETHREADED, cTarget->m_MCISingleThreaded);
	DDX_Check(pDX, IDC_LOCKVOLUME, cTarget->m_LockVolume);
	DDX_Check(pDX, IDC_EMULATEVOLUME, cTarget->m_EmulateVolume);
	DDX_Check(pDX, IDC_SAFEMIDIOUT, cTarget->m_SafeMidiOut);
	DDX_Check(pDX, IDC_MIDIAUTOREPAIR, cTarget->m_MidiAutoRepair);
	DDX_Radio(pDX, IDC_GFOCUSDEFAULT, cTarget->m_GFocusMode);
	DDX_Slider(pDX, IDC_CDSLIDER, cTarget->m_CDAVolume);
	DDX_Slider(pDX, IDC_WAVESLIDER, cTarget->m_WaveVolume);
	DDX_Slider(pDX, IDC_MIDISLIDER, cTarget->m_MidiVolume);
	DDX_Slider(pDX, IDC_GENERALSLIDER, cTarget->m_GeneralVolume);

	DDX_Check(pDX, IDC_HOOKEARSOUND, cTarget->m_HookEARSound);
	DDX_Check(pDX, IDC_PLAYSOUNDFIX, cTarget->m_PlaySoundFix);
	DDX_Check(pDX, IDC_STOPSOUND, cTarget->m_StopSound);
	DDX_Check(pDX, IDC_FIXDEFAULTMCIID, cTarget->m_FixDefaultMCIId);
	DDX_Check(pDX, IDC_DSINITVOLUME, cTarget->m_DSInitVolume);
	DDX_Check(pDX, IDC_HIDEMUTECONTROLS, cTarget->m_HideMuteControls);
	DDX_Check(pDX, IDC_BYPASSWAVEOUTPOS, cTarget->m_BypassWaveOutPos);
	DDX_Check(pDX, IDC_EMULATESOUNDPAN, cTarget->m_EmulateSoundPan);
}

BEGIN_MESSAGE_MAP(CTabSound, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_HOOKDIRECTSOUND, &CTabSound::OnBnClickedHookdirectsound)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers

void CTabSound::OnBnClickedHookdirectsound()
{
	// TODO: Add your control notification handler code here
	CTargetDlg *cTarget = ((CTargetDlg *)(this->GetParent()->GetParent()));
	cTarget->m_HookDirectSound = !cTarget->m_HookDirectSound;
	BOOL enable = cTarget->m_HookDirectSound;
	ShowHideDependencies(enable);
}

void CTabSound::ShowHideDependencies(BOOL enable)
{
	(CButton *)this->GetDlgItem(IDC_SOUNDMUTE)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_DSINITVOLUME)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_NOSOUNDLOOP)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_FIXDSOUNDBUFFER)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_BYPASSDSOUND)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_GFOCUSDEFAULT)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_GFOCUSON)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_GFOCUSOFF)->EnableWindow(enable);
	(CButton *)this->GetDlgItem(IDC_EMULATESOUNDPAN)->EnableWindow(enable);
}