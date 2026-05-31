// TabConfDefaults.cpp : implementation file
//

#include "stdafx.h"
#include "TargetDlg.h"
#include "TabConfDefaults.h"
#include "CGlobalSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabCompat dialog

CTabConfDefaults::CTabConfDefaults(CWnd* pParent /*=NULL*/)
	: CDialog(CTabConfDefaults::IDD, pParent)
{
}

BOOL CTabConfDefaults::PreTranslateMessage(MSG *pMsg)
{
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

#define IDDefaultsTIMER 3

BOOL CTabConfDefaults::OnInitDialog()
{
	CDC *myDC;
	CWnd *wColor;
	CGlobalSettings *cTarget = ((CGlobalSettings *)(this->GetParent()->GetParent()));
	if((wColor = this->GetDlgItem(IDC_FOGCOLOR))==NULL) return FALSE;
	if((myDC=wColor->GetDC())==NULL) return FALSE;
	CRect rect;
	wColor->GetClientRect(&rect);
	CBrush br(cTarget->m_FogColor); 
	myDC->FillRect(&rect, &br);
	this->ReleaseDC(myDC);
	CDialog::OnInitDialog();
	KillTimer(IDDefaultsTIMER); // kill previous instance, if any ...
	SetTimer(IDDefaultsTIMER, 200, NULL);
	CDateTimeCtrl *dateTimeCtrl= (CDateTimeCtrl *)this->GetDlgItem(IDC_DATETIMEPICKER);
	dateTimeCtrl->SetFormat("dd/MM/yyyy");
	return TRUE;
}

void CTabConfDefaults::DoDataExchange(CDataExchange* pDX)
{
	//MessageBox("CTabConfDefaults::DoDataExchange","trace",0);
	CString sDefaultPosX, sDefaultPosY;
	CDialog::DoDataExchange(pDX);
	CGlobalSettings *cTarget = ((CGlobalSettings *)(this->GetParent()->GetParent()));
	sDefaultPosX.Format("%d", cTarget->m_DefaultPosX);
	sDefaultPosY.Format("%d", cTarget->m_DefaultPosY);
	DDX_Radio(pDX, IDC_DEFAULTCOORDINATES, cTarget->m_DefaultCoordinates);
	DDX_Text(pDX, IDC_DEFAULTPOSX, sDefaultPosX);
	DDX_Text(pDX, IDC_DEFAULTPOSY, sDefaultPosY);
	DDX_Text(pDX, IDC_DEFAULTSIZX, cTarget->m_DefaultSizX);
	DDX_Text(pDX, IDC_DEFAULTSIZY, cTarget->m_DefaultSizY);
	DDX_Text(pDX, IDC_UPTIMEDAYS, cTarget->m_UpTimeDays);
	DDX_Text(pDX, IDC_CUSTOMPERFCOUNT, cTarget->m_CustomPerfCount);
	cTarget->m_DefaultPosX = atoi(sDefaultPosX);
	cTarget->m_DefaultPosY = atoi(sDefaultPosY);

	DDX_Text(pDX, IDC_TEX_MAXX, cTarget->m_TexMaxX);
	DDX_Text(pDX, IDC_TEX_MAXY, cTarget->m_TexMaxY);
	DDX_Text(pDX, IDC_TEX_MINX, cTarget->m_TexMinX);
	DDX_Text(pDX, IDC_TEX_MINY, cTarget->m_TexMinY);

	DDX_Text(pDX, IDC_LIMITDISK, cTarget->m_FreeDisk);
	DDX_Text(pDX, IDC_LIMITRAM, cTarget->m_FreeRAM);
	DDX_Text(pDX, IDC_LIMITVIDEO, cTarget->m_FreeVideo);
	DDX_Text(pDX, IDC_LIMITTEXTURE, cTarget->m_FreeTexture);

	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER, cTarget->m_fakeDate);
}

BEGIN_MESSAGE_MAP(CTabConfDefaults, CDialog)
	//{{AFX_MSG_MAP(CTabCompat)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_FOGCOLOR, OnFogColor)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCompat message handlers

void CTabConfDefaults::OnTimer(UINT_PTR nIDEvent)
{
	CDC *myDC;
	CWnd *wColor;
	CGlobalSettings *cTarget = ((CGlobalSettings *)(this->GetParent()->GetParent()));
	if((wColor = this->GetDlgItem(IDC_FOGCOLOR))==NULL) return;
	if((myDC=wColor->GetDC())==NULL) return;
	CRect rect;
	wColor->GetClientRect(&rect);
	CBrush br(cTarget->m_FogColor); 
	myDC->FillRect(&rect, &br);
	this->ReleaseDC(myDC);
}

void CTabConfDefaults::OnFogColor() 
{
	CGlobalSettings *cTarget = ((CGlobalSettings *)(this->GetParent()->GetParent()));
	CHOOSECOLORA color;
	int i=0;
	static COLORREF colorRefs[16];
	colorRefs[i++] = RGB(0x00, 0x00, 0x00); 
	colorRefs[i++] = RGB(0xFF, 0x00, 0x00);
	colorRefs[i++] = RGB(0x00, 0xFF, 0x00);
	colorRefs[i++] = RGB(0x00, 0x00, 0xFF);
	colorRefs[i++] = RGB(0xFF, 0xFF, 0x00);
	colorRefs[i++] = RGB(0x00, 0xFF, 0xFF);
	colorRefs[i++] = RGB(0xFF, 0x00, 0xFF);
	colorRefs[i++] = RGB(0xFF, 0xFF, 0xFF); 
	colorRefs[i++] = RGB(0x80, 0x00, 0x00);
	colorRefs[i++] = RGB(0x20, 0x20, 0x20);
	colorRefs[i++] = RGB(0x00, 0x80, 0x00);
	colorRefs[i++] = RGB(0x00, 0x00, 0x80);
	colorRefs[i++] = RGB(0x80, 0x80, 0x00);
	colorRefs[i++] = RGB(0x00, 0x80, 0x80);
	colorRefs[i++] = RGB(0x80, 0x00, 0x80);
	colorRefs[i++] = RGB(0x80, 0x80, 0x80);
	memset(&color, 0, sizeof(CHOOSECOLORA));
	color.lpCustColors = colorRefs;
	color.rgbResult = cTarget->m_FogColor;
	color.Flags = CC_PREVENTFULLOPEN | CC_SOLIDCOLOR;
	color.lStructSize = sizeof(CHOOSECOLORA);
	BOOL res = ChooseColor(&color);
	if(res){
		cTarget->m_FogColor = color.rgbResult;
		/* debug
		char msg[80];
		sprintf(msg, "color=%#x", cTarget->m_FogColor);
		MessageBoxA(msg, "Fog color", 0);
		*/
	}
}
 
