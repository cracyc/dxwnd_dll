/* ------------------------------------------------------------------ */
// DirectDraw Surface Stack implementation
/* ------------------------------------------------------------------ */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

//#define DXW_WINPROC_STACK_TRACING
#define MAXWNDHSTACK 256

dxwWStack::dxwWStack() {}
dxwWStack::~dxwWStack() {}

void dxwWStack::PutProc(HWND hwnd, WNDPROC wndproc)
{
	SetPropA(hwnd, "HWndStack", (HWND)wndproc);
}

WNDPROC dxwWStack::GetProc(HWND hwnd)
{
	return (WNDPROC)GetPropA(hwnd, "HWndStack");
}
