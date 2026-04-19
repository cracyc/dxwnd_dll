#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE 1
#define _MODULE "dxwnd"

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <psapi.h>
#include <dbghelp.h>
#include <locale.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "shareddc.hpp"
#include "dxhook.h"
#include "glhook.h"
#include "glidehook.h"
#include "msvfwhook.h"
#include "syslibs.h"
#include "dxhelper.h"
#include "Ime.h"
#include "Winnls32.h"
#include "Mmsystem.h"
#include "disasm.h"
#include "dwdisasm.h"
#include "MinHook.h" 

// from WinNls.h:
// #define CTRY_JAPAN 81
// #define CAL_JAPAN                      3      // Japanese Emperor Era calendar
// #define LGRPID_JAPANESE              0x0007   // Japanese
// 	dxw.CodePage = 932 for Shift-JIS japanese (0x3A4);

#if _MSC_VER >= 1900
	//From VS2015.
	typedef struct localerefcount
	{
		char *locale;
		wchar_t *wlocale;
		int *refcount;
		int *wrefcount;
	} locrefcount;
	//From VS2015.
	typedef struct __crt_locale_data
	{
		int refcount;
		unsigned int lc_codepage;
		unsigned int lc_collate_cp;
		unsigned int lc_time_cp;
		locrefcount lc_category[6];
		int lc_clike;
		int mb_cur_max;
		int * lconv_intl_refcount;
		int * lconv_num_refcount;
		int * lconv_mon_refcount;
		struct lconv * lconv;
		int * ctype1_refcount;
		unsigned short * ctype1;
		const unsigned short * pctype;
		const unsigned char * pclmap;
		const unsigned char * pcumap;
		struct __lc_time_data * lc_time_curr;
		wchar_t * locale_name[6];
	} threadlocinfo;
#endif

void HackLocale() 
{
	ApiName("HackLocale");
	HMODULE hDLL; 
	FARPROC symbol; 
	OutTraceLOC("%s: Current codepage: %d\n", ApiRef, _get_current_locale()->locinfo->lc_codepage);
	OutTraceLOC("%s: Current locale: %s\n", ApiRef, setlocale(LC_ALL, NULL));

	OutTraceLOC("%s: custom country=%d lang=%d codepage=%d\n", ApiRef, dxw.Country, dxw.Locale, dxw.CodePage);

	hDLL = (*pLoadLibraryA)("ntdll.dll");
	symbol = (*pGetProcAddress)(hDLL, "NlsAnsiCodePage");
	if (symbol) {
		DWORD oldprot;
		OutTraceLOC("%s: NlsAnsiCodePage %d -> %d\n", ApiRef, *(DWORD*)(DWORD_PTR)symbol, dxw.CodePage);
		if(!VirtualProtect(symbol, 4, PAGE_READWRITE, &oldprot)) {
			OutErrorLOC("%s: VirtualProtect ERROR symbol=%#x err=%d\n", ApiRef, symbol, GetLastError());
		}
		*(DWORD *)(DWORD_PTR)symbol = dxw.CodePage; 
		if(!VirtualProtect(symbol, 4, oldprot, &oldprot)) {
			OutErrorLOC("%s: VirtualProtect ERROR symbol=%#x err=%d\n", ApiRef, symbol, GetLastError());
		}
		if(!FlushInstructionCache(GetCurrentProcess(), symbol, 4)){
			OutErrorLOC("%s: FlushInstructionCache ERROR symbol=%#x, err=%#x\n", ApiRef, symbol, GetLastError());
		}		
		OutTraceLOC("%s: NlsAnsiCodePage %d\n", ApiRef, *(DWORD*)(DWORD_PTR)symbol);
	}
}
