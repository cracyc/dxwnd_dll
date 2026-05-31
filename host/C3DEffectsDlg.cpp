// 3DEffectsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "dxwndhost.h"
#include "3DEffectsDialog.h"
#include "math.h"

// 3DEffectsDialog dialog

IMPLEMENT_DYNAMIC(C3DEffectsDialog, CDialog)

C3DEffectsDialog::C3DEffectsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(C3DEffectsDialog::IDD, pParent)
{
}

C3DEffectsDialog::~C3DEffectsDialog()
{
	MessageBoxEx(0, "3D effects destructor", "Warning", MB_OK | MB_ICONEXCLAMATION, NULL);
	//C3DEffectsDialog::OnOK(); // kill timer....
}

void C3DEffectsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTargetDlg)
	DDX_Slider(pDX, IDC_FOGSLIDER, i_FogSlider);
	DDX_Slider(pDX, IDC_LIGHTSLIDER, i_LightSlider);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(C3DEffectsDialog, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()

#define ID3DEffectsTIMER 22

// C3DEffectsDialog message handlers

void C3DEffectsDialog::OnTimer(UINT_PTR nIDEvent)
{
	CSliderCtrl *Slider;
	static BOOL bInitialize = TRUE;

	CDialog::OnTimer(nIDEvent);

	Slider=(CSliderCtrl *)this->GetDlgItem(IDC_FOGSLIDER);
	i_FogSlider=Slider->GetPos();
	//if(GetHookStatus(NULL)!=DXW_RUNNING) {
	if(bInitialize){
		Slider->SetPos(0);
		GetHookInfo()->FogFactor = 1.0F;
	}
	else {
		GetHookInfo()->FogFactor = powf(1.414213F, (float)-i_FogSlider); 
	}

	Slider=(CSliderCtrl *)this->GetDlgItem(IDC_LIGHTSLIDER);
	i_LightSlider=Slider->GetPos();
	//if(GetHookStatus(NULL)!=DXW_RUNNING) {
	if(bInitialize){
		Slider->SetPos(0);
		GetHookInfo()->LightFactor = 1.0F;
	}
	else {
		GetHookInfo()->LightFactor = powf(1.414213F, (float)-i_LightSlider); 
	}

	bInitialize = FALSE;

}

BOOL C3DEffectsDialog::OnInitDialog()
{
	CSliderCtrl *Slider;
	CDialog::OnInitDialog();

	Slider=(CSliderCtrl *)this->GetDlgItem(IDC_FOGSLIDER);
	Slider->SetRange(-8, +8, 0);
	Slider->SetTicFreq(1);
	Slider=(CSliderCtrl *)this->GetDlgItem(IDC_LIGHTSLIDER);
	Slider->SetRange(-8, +8, 0);
	Slider->SetTicFreq(1);
	SetTimer(ID3DEffectsTIMER, 1000, NULL);

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
}

void C3DEffectsDialog::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	// stop timer
	// MessageBoxEx(0, "Stopping Time Slider dialog", "Warning", MB_OK | MB_ICONEXCLAMATION, NULL);
	GetHookInfo()->FogFactor = powf(1.414213F, (float)-i_FogSlider); 
	GetHookInfo()->LightFactor = powf(1.414213F, (float)-i_LightSlider); 
	KillTimer(ID3DEffectsTIMER);
	CDialog::OnOK();
}
