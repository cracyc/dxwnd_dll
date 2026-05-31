#include "stdafx.h"
#include "dxwndhost.h"
#include "CAboutDlg.h"

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_Version = _T("");
	m_DxwPlayVersion = _T("");
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_VERSION, m_Version);
	DDX_Text(pDX, IDC_DXWPLAYVERSION, m_DxwPlayVersion);
	DDX_Text(pDX, IDC_THANKS, m_Thanks);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_ANIMATION, m_Animation);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetTimer(ID_HELP_SCROLL, 600, NULL);
	SetTimer(ID_HELP_SCROLL2, 32, NULL);
	if (m_Animation.Load(MAKEINTRESOURCE(IDR_GHO),_T("GIF")))
	//if (m_Animation.Load("gho.gif"))
		m_Animation.Draw();
	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CAboutDlg::OnDestroy()
{
	KillTimer(ID_HELP_SCROLL);
	return TRUE;
}

CString Thanks[] = {
"",
"",
"DxWnd Copyright(C) 2004-2024 SFB7/GHO",
"",
"This program is free software: you can redistribute it and/or modify ",
"it under the terms of the GNU General Public License as published ",
"by the Free Software Foundation, either version 3 of the License, ",
"or (at your option) any later version.",
"",
"This program is distributed in the hope that it will be useful, ",
"but WITHOUT ANY WARRANTY; without even the implied warranty ",
"of MERCHANTABILITY or FITNESS FOR A PARTICULAR ",
"PURPOSE. See the GNU General Public License for more details.",
"",
"You should have received a copy of the GNU General Public License ",
"along with this program.  ",
"If not, see <http://www.gnu.org/licenses/>.",
"",
"Many thanks to CodeProject and StackOverflow sites.",
"Thank you also to (in alphabetical order):",
"",
"Andrea Mazzoleni (www.scale2x.it) for Scale2X filter",
"Andreas Jönsson for his overlaydemo program",
"Aqrit for proxies, many tweaks && hot patching schema",
"Armen Hakobyan for CFolderDialog source code",
"Arne Bockholdt for DirectSoundControl source code",
"AxXxB and Old-Games.ru teammates for ZBUFFER fix",
"AyuanX for MIDI/wave volume control in ogg-winmm",
"Batteryshark for 9xheap heap emulation",
"Benjamin Dobell for DXTn texture decomp. routines",
"Charles Petzold for MCI_Tester source code",
"Chris Maunder for SystemTray class source code",
"Chris Porter for MW2Hook source code",
"Craig Stuart Sapp for Midifile project",
"Crazyc for fixing 9xheap.dll, clear_shim() and many others",
"David Reid for FLAC, MP3 and WAV audio decoders",
"Dege for creating the excellent dgVoodoo2 wrappers",
"Dippy Dipper for support in MCI wrapping",
"Dixie for support on \"Emperor of the Fading Suns\"",
"DM for contribution in DirectShow patching",
"Elisha Riedlinger for DxWrapper source code",
"Fabian \"ryg\" Giesen && others for DXT1/5 compression",
"Federico Dossena for WineD3D libraries",
"FunkyFr3sh for fixes in proxy DLL",
"Glass Echidna for DXTn texture decomp. routines",
"Gsky916 for chinese translation",
"Huh for his incredible program support",
"Idael Cardoso for his article on a C Sharp Ripper",
"Jari Kommpa for DirectDraw wrapper source and D3D hints",
"Jiri Dvorak for his D3D8 wrapper with 16bpp emulation",
"KuromeSan for his flash timebomb hack sources",
"Leecher1337 for other injection methods from NTVDMx64",
"Luigi Auriemma for injection synchronization",
"Mark Ransom for glbitmap scaling source code", 
"Matt Pietrek for PEDUMP source code",
"Maxim Stepin && Cameron Zemek for hqx filters",
"Michael Chourdakis for wrappit proxy builder",
"Michael Koch for D3D9 proxy DLL",
"Michel Helms for his tutorial on reading audio CDs",
"MrPepka for always proposing new challenges",
"Narzoul for helping && sharing DDrawCompat code",
"NervousHammer for fixing Mechwarrior 2 problems",
"Nikolai Serdiuk for MCI CD Player project",
"OlEG Bykov for GIF animation component",
"Olly (www.ollydbg.de) for OllyDBG && Disasm lib",
"Otya128 for his Icon16bitFix project",
"Reg2s for publishing DxWnd (OG build) source code",
"Riitaoja for shims, tweaks, HTML help and CD audio support",
"RomSteady for his kind encouragement",
"Ryan Geiss for his bilinear filter code",
"Saptadeep Nath (beennath58) for support in help && bug fixes",
"SFB7 who created the original DxWnd project",
"Stefan Röttger for the libsquish source code",
"Swiss Frank for file extension association code",
"TigerhawkT3 for HTML manual pages",
"Toni Spets (toni.spets@iki.fi) for audio CD emulator",
"Tsuda Kageyu for MinHook DLL source code",
"UCyborg for LegacyD3DResolutionHack D3D hack",
"ZeroX4 for the new DxWnd artworks",
"",
"No developers were harmed in the making of this program.",
"*end*"}; // list terminator

void CAboutDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == ID_HELP_SCROLL){
		static int i=0;
		int j;
		int ThanksCount;
		for(ThanksCount=0; ; ThanksCount++) if(!strcmp(Thanks[ThanksCount], "*end*")) break;
		CString RolledThanks;
		for(j=i; j<ThanksCount; j++) RolledThanks.AppendFormat("%s\n", Thanks[j]);
		for(j=0; j<i          ; j++) RolledThanks.AppendFormat("%s\n", Thanks[j]);
		this->SetDlgItemTextA(IDC_THANKS, RolledThanks);
		i=(i+1)%ThanksCount;
	}
	if(nIDEvent == ID_HELP_SCROLL2){
		CWnd *TB = (CWnd *)this->GetDlgItem(IDC_THANKS); 
		TB->ScrollWindow(0, -1, NULL, NULL);
	}
}

void CAboutDlg::OnStnClickedAnimation()
{
	// TODO: Add your control notification handler code here
	KillTimer(ID_HELP_SCROLL);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_TIMER()
	ON_STN_CLICKED(IDC_ANIMATION, &CAboutDlg::OnStnClickedAnimation)
END_MESSAGE_MAP()
