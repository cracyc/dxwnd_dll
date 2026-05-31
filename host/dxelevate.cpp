#include "stdafx.h"
#include "dxwndhost.h"

#include "MainFrm.h"
#include "dxwndhostDoc.h"
#include "dxwndhostView.h"

extern BOOL IsProcessElevated();
extern BOOL IsUserInAdminGroup();

BOOL DxSelfElevate(CDxwndhostView *view)
{
	BOOL const fInAdminGroup = IsUserInAdminGroup();
	extern BOOL gReadOnlyMode;
	if(!fInAdminGroup) return TRUE;

    // Get and display the process elevation information.
    BOOL const fIsElevated = IsProcessElevated();
	BOOL MustRestart;
	if(fIsElevated) return TRUE;
	MustRestart=MessageBoxLang(DXW_STRING_ADMINCAP, DXW_STRING_WARNING, MB_OKCANCEL | MB_ICONQUESTION);
	if(MustRestart==IDOK){
		extern HANDLE GlobalLocker;
		if(view){
			//save the current position
			CWnd *w = view->GetParent();
			CRect rect;
			w->GetWindowRect(&rect);
			int x = rect.left;
			int y = rect.top;
			int cx = rect.right - rect.left;
			int cy = rect.bottom - rect.top;
			// save window rect
			char val[81];
			sprintf_s(val, sizeof(val), "%i", x);
			WritePrivateProfileString("window", "posx", val, gInitPath);
			sprintf_s(val, sizeof(val), "%i", y);
			WritePrivateProfileString("window", "posy", val, gInitPath);
			sprintf_s(val, sizeof(val), "%i", cx);
			WritePrivateProfileString("window", "sizx", val, gInitPath);
			sprintf_s(val, sizeof(val), "%i", cy);
			WritePrivateProfileString("window", "sizy", val, gInitPath);
		}

		// Autoelevation at startup has no HostView yet, but nothing to save either
		if (view && view->isUpdated && !gReadOnlyMode){
			if (MessageBoxLang(DXW_STRING_LISTUPDATE, DXW_STRING_WARNING, MB_YESNO | MB_ICONQUESTION)==IDYES) {
				view->SaveConfigFile();
			}
		}
		CloseHandle(GlobalLocker);
		char szPath[MAX_PATH];
		if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
		{
			// Launch itself as administrator.
			SHELLEXECUTEINFO sei = { sizeof(sei) };
			CString args;
			sei.lpVerb = "runas";
			sei.lpFile = szPath;
			sei.lpDirectory = gInitialWorkingDir;
			sei.hwnd = (HWND)NULL; // set to NULL to force the confirmation dialog on top of everything...
			sei.nShow = SW_NORMAL;
			args = "";
			for(int i=1; i<=__argc; i++) {
				args += (LPCSTR)(__argv[i]);
				args += " ";
			}
			sei.lpParameters = args;
			if (!ShellExecuteEx(&sei)){
				DWORD dwError = GetLastError();
				if (dwError == ERROR_CANCELLED){
					// The user refused the elevation.
					// Do nothing ...
				}
			}
			else{
				exit(0); // Quit itself
			}
		}
	}
	return TRUE;
}