#include <windows.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxhook.h"

/*
Form Screen Size in Pixels

' This is code that I used in VBA, but it should
' just as easily be used in VB. You will need to
' modify the resize code of the form to resize the
' controls to match.

' 
' Use the GetDeviceCaps API call to
' determine the size of the screen in pixels
' and the conversion factor of Pixels per
' inch. Find the total width & height of the
' screen in Points by multiplying the number
' of Points per Inch (1/72) to the total
' number of inches (Pixels/Pixels per Inch).
' 

'Sample:
' (Code is inserted on a module), and Userform1
' is assumed to exist)

Option Explicit

Private Const LOGPIXELSX = 88 ' Logical pixels/inch in X
Private Const LOGPIXELSY = 90 ' Logical pixels/inch in Y
Private Const HORZRES = 8 ' Horizontal width in pixels
Private Const VERTRES = 10 ' Vertical width in pixels
Private Const TWIPSPERINCH = 1440

Private Declare Function GetDeviceCaps Lib "gdi32" ( _
                            ByVal hDc As Long, _
                            ByVal nIndex As Long _
                        ) As Long

Private Declare Function ReleaseDC Lib "user32" ( _
                            ByVal hwnd As Long, _
                            ByVal hDc As Long _
                        ) As Long

Private Declare Function GetDC Lib "user32" ( _
                            ByVal hwnd As Long _
                        ) As Long

Public Sub usbGetFormSize( _
                ByRef x As Variant, _
                ByRef y As Variant _
            )

    Dim hDc As Long, lngRetVal As Long
    Dim varScreenX As Variant, varScreenY As Variant
    Dim varPixToInchX As Variant, varPixToInchY As Variant

    hDc = GetDC(0)

    'Get the Screen size in pixels for X & Y
    varScreenX = GetDeviceCaps(hDc, HORZRES)
    varScreenY = GetDeviceCaps(hDc, VERTRES)

    'Get the conversion of pixels/inch for X & Y
    varPixToInchX = GetDeviceCaps(hDc, LOGPIXELSX)
    varPixToInchY = GetDeviceCaps(hDc, LOGPIXELSY)

    lngRetVal = ReleaseDC(0, hDc)

    'Convert to Points.
    x = (varScreenX / varPixToInchX) * 72
    y = (varScreenY / varPixToInchY) * 72

End Sub
*/

#define TWIPXPERPIXEL 1440

void PatchMSVBVM60()
{
	OutTrace("PatchMSVBVM60: screen size (%d x %d) -> (%d x %d)\n",
		*(DWORD *)0x733B423C, *(DWORD *)0x733B4574,
		dxw.GetScreenWidth(), dxw.GetScreenHeight());
	*(DWORD *)0x733B423C = dxw.GetScreenWidth();
	*(DWORD *)0x733B4574 = dxw.GetScreenHeight();
	if(dxw.dwFlags13 & LEGACYBASEUNITS){
		OutTrace("PatchMSVBVM60: logpixel size (%d x %d) -> (%d x %d)\n",
			*(DWORD *)0x733B4594, *(DWORD *)0x733B43DC, 96, 96);
		*(DWORD *)0x733B4594 = 96; // 96 LOGPIXELX dpi
		*(DWORD *)0x733B43DC = 96; // 96 LOGPIXELY dpi
		*(DWORD *)0x733B41DC = TWIPXPERPIXEL / 96;
		*(DWORD *)0x733B41D8 = TWIPXPERPIXEL / 96;
		OutTrace("PatchMSVBVM60: TextExtent size (%d x %d) -> (%d x %d)\n",
			*(DWORD *)0x733B4288, *(DWORD *)0x733B428C, 0x9, 0x14);
		*(DWORD *)0x733B4288 = 0x9; // GetTextExtent("0") SIZE.cx
		*(DWORD *)0x733B428C = 0x14; // GetTextExtent("0") SIZE.cy
	}
	/*
	return;
	static BOOL DoOnce = FALSE;
	if(DoOnce) return;
	typedef void (WINAPI *DLLMain_Type)(DWORD);
	DLLMain_Type hMain;
	HMODULE vbh = (*pLoadLibraryA)("MSVBVM60.dll");
	if(!vbh) {
		OutTrace("+++ no vbh\n");
		return;
	}
	hMain = (DLLMain_Type)(*pGetProcAddress)(vbh, "ThunRTMain");
	if(!hMain) {
		OutTrace("+++ no ThunRTMain\n");
		return;
	}
	OutTrace("+++ Signature: %4.4s\n", (char *)0x401330);
	//(*hMain)(0x401330); // address of "VB5!" signature
	//(*hMain)(0x4011B0); // address of "VB5!" signature
	DoOnce = TRUE;
	*/
}

typedef struct {
	CHAR *lpDllName;
	CHAR *lpFunctionName;
} DLLFUNCTCALLDESC;

typedef LPVOID (WINAPI *DllFunctionCall_Type)(DLLFUNCTCALLDESC *);
LPVOID WINAPI extDllFunctionCall(DLLFUNCTCALLDESC *);
DllFunctionCall_Type pDllFunctionCall;

static HookEntryEx_Type VBHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "DllFunctionCall", NULL, (FARPROC *)&pDllFunctionCall, (FARPROC)extDllFunctionCall},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookVB6(HMODULE module)
{
	HookLibraryEx(module, VBHooks, "MSVBVM60.dll");
}

LPVOID WINAPI extDllFunctionCall(DLLFUNCTCALLDESC *lpDll)
{
	ApiName("DllFunctionCall");
	LPVOID wrapAddr, origAddr;
	OutTrace("%s: dll=%s function=%s\n", ApiRef, lpDll->lpDllName, lpDll->lpFunctionName);

	wrapAddr = origAddr = (*pDllFunctionCall)(lpDll);
	if(origAddr == pChangeDisplaySettingsA) wrapAddr = extChangeDisplaySettingsA;
	if(origAddr == pEnumDisplaySettingsA) wrapAddr = extEnumDisplaySettingsA;

	if(origAddr != wrapAddr){
		_asm {mov eax, wrapAddr} // necessary???
		OutTrace("%s: addr=%#x -> %#x\n", ApiRef, origAddr, wrapAddr);
	}
	else {
		OutTrace("%s: addr=%#x\n", ApiRef, wrapAddr);
	}
	return wrapAddr;
}
