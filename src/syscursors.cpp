#define _CRT_SECURE_NO_WARNINGS

#include <windows.h> 
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

#include "TlHelp32.h"

#define OCR_NORMAL          32512UL
#define OCR_IBEAM           32513UL
#define OCR_WAIT            32514UL
#define OCR_CROSS           32515UL
#define OCR_UP              32516UL
#define OCR_SIZE            32640UL   
#define OCR_ICON            32641UL   
#define OCR_SIZENWSE        32642UL
#define OCR_SIZENESW        32643UL
#define OCR_SIZEWE          32644UL
#define OCR_SIZENS          32645UL
#define OCR_SIZEALL         32646UL
#define OCR_ICOCUR          32647UL   
#define OCR_NO              32648UL
#define OCR_HAND            32649UL
#define OCR_APPSTARTING     32650UL

struct CURSOR 
{
    DWORD id;
    HCURSOR hCursor;
};

CURSOR g_arrCursors[] = 
{
    {OCR_NORMAL,        0},
    {OCR_IBEAM,         0},
    {OCR_WAIT,          0},
    {OCR_CROSS,         0},
    {OCR_UP,            0},
    {OCR_SIZE,          0},
    {OCR_ICON,          0},
    {OCR_SIZENWSE,      0},
    {OCR_SIZENESW,      0},
    {OCR_SIZEWE,        0},
    {OCR_SIZENS,        0},
    {OCR_SIZEALL,       0},
    {OCR_ICOCUR,        0},
    {OCR_NO,            0},
    {OCR_HAND,          0},
    {OCR_APPSTARTING,   0}
};

void SaveSystemCursors()
{
    // Backup all the cursors
    for (int i=0; i<sizeof(g_arrCursors)/sizeof(CURSOR);i++){
        HCURSOR hCursorT = LoadCursor(0, MAKEINTRESOURCE(g_arrCursors[i].id));
        
        if (hCursorT){
            g_arrCursors[i].hCursor = CopyCursor(hCursorT);
            DestroyCursor(hCursorT);
        }
        else {
            g_arrCursors[i].hCursor = 0;
        }
    }
}

void RestoreSystemCursors()
{
    // Restore all the cursors
    for (int i=0; i<sizeof(g_arrCursors)/sizeof(CURSOR); i++){
        if (g_arrCursors[i].hCursor){
            SetSystemCursor(g_arrCursors[i].hCursor, g_arrCursors[i].id);
            DestroyCursor(g_arrCursors[i].hCursor);
        }
    }
}