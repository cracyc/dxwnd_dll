#pragma once


// CTimeSlider dialog

class C3DEffectsDialog : public CDialog
{
	DECLARE_DYNAMIC(C3DEffectsDialog)

public:
	C3DEffectsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~C3DEffectsDialog();
	//~C3DEffectsDialog();

// Dialog Data
	enum { IDD = IDD_3DEFFECTS };
	//CSliderCtrl m_TimeSlider;
	int i_FogSlider;
	int i_LightSlider;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
private:
};
