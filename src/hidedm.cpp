#include "dxwnd.h"
#include "dxwcore.hpp"

#define _MODULE "dxwnd"

// Data needed in mode table
typedef struct _MODE
{
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
    DWORD dmDisplayFlags;
    DWORD dmDisplayFrequency;
    DWORD dwActualIndex;
    DWORD bIgnore;
} MODE;

// Permanent mode table
static MODE *g_pModeTable = NULL;

// Number of entries in the mode table
static DWORD g_dwCount = 0;

// Build the mode table on first call
static BOOL g_bInit = FALSE;
static void BuildModeList(void);

// Lookup from the sanitized mode table.
LONG WINAPI hideEnumDisplaySettingsA(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode)
{
	ApiName("hideEnumDisplaySettingsA");
	OutTrace("%s: imodeNum=%d\n", ApiRef, iModeNum);
    BuildModeList();
    
    BOOL bRet = FALSE;

    if (lpszDeviceName || ((LONG)iModeNum < 0) || !g_pModeTable) {
        bRet = (*pEnumDisplaySettingsA)(lpszDeviceName, iModeNum, lpDevMode);
    } 
	else if (iModeNum < g_dwCount) {
        MODE* pmode = g_pModeTable + iModeNum;

        bRet = (*pEnumDisplaySettingsA)(lpszDeviceName, pmode->dwActualIndex, lpDevMode);

        if (bRet) {
			OutTraceDW("%s: Returning shorter list of display modes.\n", ApiRef);
            
            lpDevMode->dmBitsPerPel = pmode->dmBitsPerPel;
            lpDevMode->dmPelsWidth = pmode->dmPelsWidth;
            lpDevMode->dmPelsHeight = pmode->dmPelsHeight;
            lpDevMode->dmDisplayFlags = pmode->dmDisplayFlags;
            lpDevMode->dmDisplayFrequency = pmode->dmDisplayFrequency;
        }
    }

    return bRet;
}

/*++

 Lookup from the sanitized mode table.

--*/
LONG WINAPI hideEnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode)
{
	ApiName("hideEnumDisplaySettingsW");
	OutTrace("%s: imodeNum=%d\n", ApiRef, iModeNum);
    BuildModeList();
    
    BOOL bRet = FALSE;

    if (lpszDeviceName || ((LONG)iModeNum < 0) || !g_pModeTable) {
        bRet = (*pEnumDisplaySettingsW)(lpszDeviceName, iModeNum, lpDevMode);
    } 
	else if (iModeNum < g_dwCount) {
        MODE* pmode = g_pModeTable + iModeNum;

        bRet = (*pEnumDisplaySettingsW)(lpszDeviceName, pmode->dwActualIndex, lpDevMode);

        if (bRet) {
			OutTraceDW("%s: Returning shorter list of display modes.\n", ApiRef);
            
            lpDevMode->dmBitsPerPel = pmode->dmBitsPerPel;
            lpDevMode->dmPelsWidth = pmode->dmPelsWidth;
            lpDevMode->dmPelsHeight = pmode->dmPelsHeight;
            lpDevMode->dmDisplayFlags = pmode->dmDisplayFlags;
            lpDevMode->dmDisplayFrequency = pmode->dmDisplayFrequency;
        }
    }

    return bRet;
}

/*++

 Sort the table by Width+Height+BitsPerPel+Frequency in that order so that 
 they can be easily filtered.

--*/

int _cdecl compare1(const void* a1, const void* a2)
{
    MODE* arg1 = (MODE*)a1;
    MODE* arg2 = (MODE*)a2;
    int d;
    d = arg1->dmPelsWidth - arg2->dmPelsWidth;
    if (d == 0) d = arg1->dmPelsHeight - arg2->dmPelsHeight;
    if (d == 0) d = arg1->dmBitsPerPel - arg2->dmBitsPerPel;
    if (d == 0) d = arg1->dmDisplayFrequency - arg2->dmDisplayFrequency;
    return d;
}

/*++

 Sort the table so it looks like a Win9x mode table, i.e. BitsPerPel is the
 primary sort key.

--*/

int _cdecl compare2(const void* a1, const void* a2)
{
    MODE* arg1 = (MODE*)a1;
    MODE* arg2 = (MODE*)a2;
    int d;
    d = arg1->dmBitsPerPel - arg2->dmBitsPerPel;
    if (d == 0) d = arg1->dmPelsWidth - arg2->dmPelsWidth;
    if (d == 0) d = arg1->dmPelsHeight - arg2->dmPelsHeight;
    if (d == 0) d = arg1->dmDisplayFrequency - arg2->dmDisplayFrequency;
    return d;
}

/*++

 Create a new mode table based upon the sanitized existing table. To do this, 
 we do the following:
 
    1. Get the entire table
    2. Sort it - to allow efficient removal of duplicates
    3. Remove duplicates and unwanted modes
    4. Build a new table with only the modes that 'pass'

--*/

static void BuildModeList(void)
{
	ApiName("BuildModeList");
    if (g_bInit) return;
	OutTrace("%s: initializing.\n", ApiRef);

    DEVMODEA dm;
    ULONG    i, j;
    dm.dmSize = sizeof(DEVMODEA);

    //
    // Figure out how many modes there are.
    //

    i = 0;
    while ((*pEnumDisplaySettingsA)(NULL, i, &dm)) i++;

    //
    // Allocate the full mode table.
    //
    MODE *pTempTable = (MODE*)malloc(sizeof(MODE) * i);
    
    if (!pTempTable) {
		OutTraceE("%s: Failed to allocate %d bytes.\n", ApiRef, sizeof(MODE) * i);
        return;
    }

    MODE* pmode = pTempTable;

    //
    // Get all the modes.
    //
    i = 0;
    while ((*pEnumDisplaySettingsA)(NULL, i, &dm)) {
        pmode->dmBitsPerPel       = dm.dmBitsPerPel;
        pmode->dmPelsWidth        = dm.dmPelsWidth;
        pmode->dmPelsHeight       = dm.dmPelsHeight;
        pmode->dmDisplayFlags     = dm.dmDisplayFlags;
        pmode->dmDisplayFrequency = 0; // dm.dmDisplayFrequency;
        pmode->dwActualIndex      = i;
        pmode->bIgnore            = FALSE;

        pmode++;
        i++;
    }
    
    //
    // Sort the full table so we can remove duplicates easily.
    //
    qsort((void*)pTempTable, (size_t)i, sizeof(MODE), compare1);

    //
    // Strip away bad modes by setting them as ignored.
    //
    pmode = pTempTable;
    
    MODE* pprev = NULL;

    for (j = 0; j < i; j++) {
        if ((pmode->dmBitsPerPel < 8) || 
            (pmode->dmPelsWidth < 640) ||
            (pmode->dmPelsHeight < 480) ||
            (pmode->dmPelsWidth > 1280) || 
            (pprev &&
            (pprev->dmBitsPerPel == pmode->dmBitsPerPel) &&
            (pprev->dmPelsWidth == pmode->dmPelsWidth) &&
            (pprev->dmPelsHeight == pmode->dmPelsHeight))) {
            
            //
            // Special-case 640x480x4bit.
            //
            if ((pmode->dmBitsPerPel == 4) && 
                (pmode->dmPelsWidth == 640) &&
                (pmode->dmPelsHeight == 480)) {
                
                g_dwCount++;
            } else {
                pmode->bIgnore = TRUE;
            }
        } else {
            g_dwCount++;
        }

        pprev = pmode;
        pmode++;
        
    }

    //
    // Build the new table with only the modes that passed.
    //
    g_pModeTable = (MODE*)malloc(sizeof(MODE) * g_dwCount);
    
    if (!g_pModeTable) {
		OutTraceE("%s: Failed to allocate %d bytes.\n", ApiRef, sizeof(MODE) * g_dwCount);
        free(pTempTable);
        return;
    }

    MODE* pmoden = g_pModeTable;
    
    pmode = pTempTable;

    for (j = 0; j < i; j++) {
        if (!pmode->bIgnore) {
            MoveMemory(pmoden, pmode, sizeof(MODE));
            pmoden++;
        }
        pmode++;
    }

    //
    // Sort the full table so we can remove duplicates easily.
    //
    qsort((void*)g_pModeTable, (size_t)g_dwCount, sizeof(MODE), compare2);
    free(pTempTable);
    g_bInit = TRUE;
}
