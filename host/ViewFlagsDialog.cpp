// ViewFlagsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "dxwndhost.h"
#include "ViewFlagsDialog.h"
#include "TargetDlg.h"

// CViewFlagsDialog dialog

IMPLEMENT_DYNAMIC(CViewFlagsDialog, CDialog)

CViewFlagsDialog::CViewFlagsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CViewFlagsDialog::IDD, pParent)
{
}

CViewFlagsDialog::~CViewFlagsDialog()
{
}

void CViewFlagsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CViewFlagsDialog, CDialog)
END_MESSAGE_MAP()

char *strlow(LPCSTR s, char *buf)
{
	char *p;
	for(p = buf; *s; s++, p++) *p = tolower(*s);
	*p = 0; 
	return buf;
}

// CViewFlagsDialog message handlers
void printFlags(CString *s, DWORD dword, DWORD ref, int idx)
{
	for(int i=0; i<32; i++, dword>>=1, ref >>=1) {
		LPCSTR p = GetFlagCaption(idx,i);
		if(dword & 0x1) {
			if(ref & 0x1) {
				char buf[40];
				s->AppendFormat("%s ", strlow(p, buf));
			}
			else s->AppendFormat("+%s ", p);
		}
		else {
			if(ref & 0x1) s->AppendFormat("-%s ", p);
		}
	}
}

BOOL CViewFlagsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	CString sflags;
	extern TARGETMAP *ViewTarget; // dirty !!!!
	int i;
	TARGETMAP *t;
	TARGETMAP def;
	DWORD dword;
	t = ViewTarget;
	memset((LPVOID)&def, 0, sizeof(TARGETMAP)); // clean up, just in case....
	CTargetDlg *dlg = new(CTargetDlg);
	extern void SetTargetFromDlg(TARGETMAP *, CTargetDlg *);
	SetTargetFromDlg(&def, dlg);
	sflags.Append("Flags1: ");
	printFlags(&sflags, t->flags, def.flags, 0);
	sflags.Append("\nFlags2: ");
	printFlags(&sflags, t->flags2, def.flags2, 1);
	sflags.Append("\nFlags3: ");
	printFlags(&sflags, t->flags3, def.flags3, 2);
	sflags.Append("\nFlags4: ");
	printFlags(&sflags, t->flags4, def.flags4, 3);
	sflags.Append("\nFlags5: ");
	printFlags(&sflags, t->flags5, def.flags5, 4);
	sflags.Append("\nFlags6: ");
	printFlags(&sflags, t->flags6, def.flags6, 5);
	sflags.Append("\nFlags7: ");
	printFlags(&sflags, t->flags7, def.flags7, 6);
	sflags.Append("\nFlags8: ");
	printFlags(&sflags, t->flags8, def.flags8, 7);
	sflags.Append("\nFlags9: ");
	printFlags(&sflags, t->flags9, def.flags9, 8);
	sflags.Append("\nFlags10: ");
	printFlags(&sflags, t->flags10, def.flags10, 9);
	sflags.Append("\nFlags11: ");
	printFlags(&sflags, t->flags11, def.flags11, 10);
	sflags.Append("\nFlags12: ");
	printFlags(&sflags, t->flags12, def.flags12, 11);
	sflags.Append("\nFlags13: ");
	printFlags(&sflags, t->flags13, def.flags13, 12);
	sflags.Append("\nFlags14: ");
	printFlags(&sflags, t->flags14, def.flags14, 13);
	sflags.Append("\nFlags15: ");
	printFlags(&sflags, t->flags15, def.flags15, 14);
	sflags.Append("\nFlags16: ");
	printFlags(&sflags, t->flags16, def.flags16, 15);
	sflags.Append("\nFlags17: ");
	printFlags(&sflags, t->flags17, def.flags17, 16);
	sflags.Append("\nFlags18: ");
	printFlags(&sflags, t->flags18, def.flags18, 17);
	sflags.Append("\nFlags19: ");
	printFlags(&sflags, t->flags19, def.flags19, 18);
	sflags.Append("\nFlags20: ");
	printFlags(&sflags, t->flags20, def.flags20, 19);
	sflags.Append("\nTFlags: ");
	for(i=0, dword = t->tflags; i<32; i++, dword>>=1) if(dword & 0x1) sflags.AppendFormat("%s ", GetFlagCaption(20,i));
	sflags.Append("\nTFlags2: ");
	for(i=0, dword = t->tflags2; i<32; i++, dword>>=1) if(dword & 0x1) sflags.AppendFormat("%s ", GetFlagCaption(21,i));
	sflags.Append("\nDFlags: ");
	for(i=0, dword = t->dflags; i<32; i++, dword>>=1) if(dword & 0x1) sflags.AppendFormat("%s ", GetFlagCaption(22,i));
	sflags.Append("\nDFlags2: ");
	for(i=0, dword = t->dflags2; i<32; i++, dword>>=1) if(dword & 0x1) sflags.AppendFormat("%s ", GetFlagCaption(23,i));

	this->SetDlgItemTextA(IDC_DESKTOPINFO, sflags);
	this->SetWindowTextA(t->path);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CViewFlagsDialog::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialog::OnOK();
}
