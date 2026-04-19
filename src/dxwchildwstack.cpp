/* ------------------------------------------------------------------ */
// DirectDraw Surface Stack implementation
/* ------------------------------------------------------------------ */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

//#define DXW_WINPROC_STACK_TRACING
#define MAXWNDHSTACK 256

dxwChildWStack::dxwChildWStack()
{
	WhndTOS = 0;
	WhndSize = MAXWNDHSTACK;
	WhndStack = (childwinstack_entry *)malloc(WhndSize * sizeof(childwinstack_entry));
}

dxwChildWStack::~dxwChildWStack()
{
}

void dxwChildWStack::Put(HWND hwnd, HWND fatherhwnd, int x, int y, int cx, int cy)
{
	int StackIdx;

	// add extra space when necessary, in chunks of MAXWNDHSTACK entries
	if(WhndTOS == WhndSize){
		WhndSize += MAXWNDHSTACK;
		WhndStack = (childwinstack_entry *)realloc(WhndStack, WhndSize * sizeof(childwinstack_entry));
	}
	// wndproc values of 0xFFFFxxxx type seems to be error codes rather than valid callback addresses ....
	// v2.02.36 using CallWindowProc you can pass WinProc handles, so you don't need to eliminate them!
	//if (((DWORD)wndproc & 0xFFFF0000) == 0xFFFF0000) return;
	//OutTraceDW("DEBUG: WNDPROC STACK push hwnd=%#x, wndproc=%#x\n", hwnd, wndproc);
	// try update first...
	for(StackIdx=0; StackIdx<WhndTOS; StackIdx++) 
		if (WhndStack[StackIdx].hwnd==hwnd) {
			// update only valid fields
			WhndStack[StackIdx].x=x;
			WhndStack[StackIdx].y=y;
			WhndStack[StackIdx].cx=cx;
			WhndStack[StackIdx].cy=cy;
#ifdef DXW_WINPROC_STACK_TRACING
			OutTrace(">>>(Put): updated hwnd=%#x pos=(%d,%d) size=(%dx%d) idx=%d\n", hwnd, x, y, cx, cy, StackIdx);
#endif
			return;
		}
	WhndStack[WhndTOS].hwnd=hwnd;
	// initialize ...
	WhndStack[StackIdx].x=x;
	WhndStack[StackIdx].y=y;
	WhndStack[StackIdx].cx=cx;
	WhndStack[StackIdx].cy=cy;
	// increment TOS.
#ifdef DXW_WINPROC_STACK_TRACING
			OutTrace(">>>(Put): added hwnd=%#x pos=(%d,%d) size=(%dx%d) idx=%d\n", hwnd, x, y, cx, cy, StackIdx);
#endif
	WhndTOS++;
}

BOOL dxwChildWStack::Get(HWND hwnd, int *x, int *y, int *cx, int *cy)
{
	int StackIdx;
	for(StackIdx=0; StackIdx<WhndTOS; StackIdx++) if (WhndStack[StackIdx].hwnd==hwnd) {
		//OutTraceDW("DEBUG: WNDPROC STACK pop hwnd=%#x, wndproc=%#x\n", hwnd, WhndStack[StackIdx].wndproc);
		*x=WhndStack[StackIdx].x;
		*y=WhndStack[StackIdx].y;
		*cx=WhndStack[StackIdx].cx;
		*cy=WhndStack[StackIdx].cy;
#ifdef DXW_WINPROC_STACK_TRACING
		OutTrace(">>>(GetSize): hwnd=%#x, pos=(%s,%d) size=(%dx%d)\n", hwnd, *x, *y, *cx, *cy);
#endif
		return TRUE;
	}
#ifdef DXW_WINPROC_STACK_TRACING
	OutTrace(">>>(GetSize): hwnd=%#x, size=UNKNOWN\n", hwnd);
#endif
	return FALSE;
}
