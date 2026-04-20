#define  _CRT_SECURE_NO_WARNINGS

#define _MODULE "msvcrt"
#define TRACEALL

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <locale.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include <sys\stat.h>

#define ASSERTDIALOGS

typedef LPVOID (CDECL *malloc_Type)(size_t);
typedef LPVOID (CDECL *realloc_Type)(LPVOID, size_t);
typedef VOID (CDECL *free_Type)(LPVOID);
typedef FILE * (CDECL *fopen_Type)(const char *, const char *);
typedef int (CDECL *_open_Type)(const char *, int, int);
typedef void (CDECL *__set_app_type_Type)(int);
typedef void (CDECL *_initterm_Type)(LPVOID, LPVOID);

malloc_Type pmalloc;
realloc_Type prealloc;
free_Type pfree;
fopen_Type pfopen;
_open_Type p_open;
__set_app_type_Type p__set_app_type;
_initterm_Type p_initterm;

LPVOID CDECL extmalloc(size_t);
LPVOID CDECL extrealloc(LPVOID, size_t);
VOID CDECL extfree(LPVOID);
FILE * CDECL extfopen(const char *, const char *);
int CDECL ext_open(const char *, int, int);
void CDECL ext__set_app_type(int);
void CDECL ext_initterm(LPVOID, LPVOID);

typedef UINT (CDECL *__lc_codepage_Type)(void);
__lc_codepage_Type p__lc_codepage;
UINT CDECL ext__lc_codepage(void);

typedef size_t (CDECL *_mbclen_Type)(const unsigned char *);
_mbclen_Type p_mbclen;
size_t CDECL ext_mbclen(const unsigned char *);

typedef size_t (CDECL *_mbclen_l_Type)(const unsigned char *, _locale_t);
_mbclen_l_Type p_mbclen_l = 0;
//size_t CDECL ext_mbclen_l(const unsigned char *, _locale_t);

typedef int (CDECL *mblen_Type)(const char *mbstr, size_t count);
mblen_Type pmblen;
int CDECL extmblen(const char *mbstr, size_t count);

typedef int (CDECL *_mblen_l_Type)(const char *mbstr, size_t count, _locale_t locale);
_mblen_l_Type p_mblen_l;
//int CDECL ext_mblen_l(const char *mbstr, size_t count, _locale_t locale);

typedef _locale_t (CDECL *_create_locale_Type)(int category, const char *locale);
_create_locale_Type p_create_locale;

typedef int (CDECL *_stat_Type)(const char *, struct _stat *);
_stat_Type p_stat;
int CDECL ext_stat(const char *, struct _stat *);

typedef int (CDECL *_access_Type)(const char *, int);
_access_Type p_access;
int CDECL ext_access(const char *, int);

typedef int (CDECL *_waccess_Type)(const wchar_t *, int);
_waccess_Type p_waccess;
int CDECL ext_waccess(const wchar_t *, int);

typedef errno_t (CDECL *_access_s_Type)(const char *, int);
_access_s_Type p_access_s;
errno_t CDECL ext_access_s(const char *, int);

typedef errno_t (CDECL *_waccess_s_Type)(const wchar_t *, int);
_waccess_s_Type p_waccess_s;
errno_t CDECL ext_waccess_s(const wchar_t *, int);

#ifdef TRACEALL
typedef errno_t (CDECL *_chdrive_Type)(int);
_chdrive_Type *p_chdrive;
errno_t CDECL ext_chdrive(int);

typedef int (CDECL *_getdrive_Type)(void);
_getdrive_Type p_getdrive;
int CDECL ext_getdrive(void);

typedef errno_t (CDECL *_chdir_Type)(const char *);
_chdir_Type p_chdir;
errno_t CDECL ext_chdir(const char *);

typedef size_t (CDECL *fread_Type)(void *, size_t, size_t, FILE *);
fread_Type pfread;
size_t CDECL extfread(void *buffer, size_t size, size_t count, FILE *stream);

typedef FILE * (CDECL *_fdopen_Type)(int, const char *);
_fdopen_Type p_fdopen;
FILE * CDECL ext_fdopen(int, const char *);

typedef int (CDECL *_mkdir_Type)(const char *);
_mkdir_Type p_mkdir;
int CDECL ext_mkdir(const char *);

typedef char * (CDECL *_fullpath_Type)(char *, const char *, size_t);
_fullpath_Type p_fullpath;
char * CDECL ext_fullpath(char *, const char *, size_t);

typedef wchar_t * (CDECL *_wfullpath_Type)(wchar_t *, const wchar_t *, size_t);
_wfullpath_Type p_wfullpath;
wchar_t * CDECL ext_wfullpath(wchar_t *, const wchar_t *, size_t);
#endif // TRACEALL

_locale_t curLocale = NULL;

static HookEntryEx_Type HooksAlloc[]={
	{HOOK_IAT_CANDIDATE, 0x0000, "malloc", (FARPROC)NULL, (FARPROC *)&pmalloc, (FARPROC)extmalloc},
	{HOOK_IAT_CANDIDATE, 0x0000, "realloc", (FARPROC)NULL, (FARPROC *)&prealloc, (FARPROC)extrealloc},
	{HOOK_IAT_CANDIDATE, 0x0000, "free", (FARPROC)NULL, (FARPROC *)&pfree, (FARPROC)extfree},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type HooksFiles[]={
	{HOOK_IAT_CANDIDATE, 0x0000, "fopen", (FARPROC)NULL, (FARPROC *)&pfopen, (FARPROC)extfopen},
	{HOOK_IAT_CANDIDATE, 0x0000, "_open", (FARPROC)NULL, (FARPROC *)&p_open, (FARPROC)ext_open},
	{HOOK_IAT_CANDIDATE, 0x0000, "_stat", (FARPROC)NULL, (FARPROC *)&p_stat, (FARPROC)ext_stat},
	{HOOK_IAT_CANDIDATE, 0x0000, "_access", (FARPROC)NULL, (FARPROC *)&p_access, (FARPROC)ext_access},
	{HOOK_IAT_CANDIDATE, 0x0000, "_waccess", (FARPROC)NULL, (FARPROC *)&p_waccess, (FARPROC)ext_waccess},
	{HOOK_IAT_CANDIDATE, 0x0000, "_access_s", (FARPROC)NULL, (FARPROC *)&p_access_s, (FARPROC)ext_access_s},
	{HOOK_IAT_CANDIDATE, 0x0000, "_waccess_s", (FARPROC)NULL, (FARPROC *)&p_waccess_s, (FARPROC)ext_waccess_s},
#ifdef TRACEALL
	{HOOK_IAT_CANDIDATE, 0x0000, "_getdrive", (FARPROC)NULL, (FARPROC *)&p_getdrive, (FARPROC)ext_getdrive},
	{HOOK_IAT_CANDIDATE, 0x0000, "_chdrive", (FARPROC)NULL, (FARPROC *)&p_chdrive, (FARPROC)ext_chdrive},
	{HOOK_IAT_CANDIDATE, 0x0000, "_chdir", (FARPROC)NULL, (FARPROC *)&p_chdir, (FARPROC)ext_chdir},
	{HOOK_IAT_CANDIDATE, 0x0000, "fread", (FARPROC)NULL, (FARPROC *)&pfread, (FARPROC)extfread},
	{HOOK_IAT_CANDIDATE, 0x0000, "_fdopen", (FARPROC)NULL, (FARPROC *)&p_fdopen, (FARPROC)ext_fdopen},
	{HOOK_IAT_CANDIDATE, 0x0000, "_mkdir", (FARPROC)NULL, (FARPROC *)&p_mkdir, (FARPROC)ext_mkdir},
	{HOOK_IAT_CANDIDATE, 0x0000, "_fullpath", (FARPROC)NULL, (FARPROC *)&p_fullpath, (FARPROC)ext_fullpath},
	{HOOK_IAT_CANDIDATE, 0x0000, "_wfullpath", (FARPROC)NULL, (FARPROC *)&p_wfullpath, (FARPROC)ext_wfullpath},
#endif // TRACEALL
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type HooksLocale[]={
	{HOOK_IAT_CANDIDATE, 0x0000, "__lc_codepage", (FARPROC)NULL, (FARPROC *)&p__lc_codepage, (FARPROC)ext__lc_codepage},
	{HOOK_IAT_CANDIDATE, 0x0000, "_mbclen", (FARPROC)NULL, (FARPROC *)&p_mbclen, (FARPROC)ext_mbclen},
	{HOOK_IAT_CANDIDATE, 0x0000, "mblen", (FARPROC)NULL, (FARPROC *)&pmblen, (FARPROC)extmblen},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type HooksRehook[]={
	{HOOK_HOT_REQUIRED,  0x0000, "__set_app_type", (FARPROC)NULL, (FARPROC *)&p__set_app_type, (FARPROC)ext__set_app_type},
	//{HOOK_HOT_REQUIRED,  0x0000, "_initterm", (FARPROC)NULL, (FARPROC *)&p_initterm, (FARPROC)ext_initterm},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static char *libname = "msvcrt.dll";
static HMODULE msvcrthmod;

void HookMSVCRT(HMODULE hModule)
{
	OutTraceSYS("HookMSVCRT\n");
	msvcrthmod = hModule;
	if(dxw.dwFlags11 & SAFEALLOCS) HookLibraryEx(hModule, HooksAlloc, libname);
	if((dxw.dwFlags10 & (FAKECDDRIVE | FAKEHDDRIVE)) || (dxw.dwTFlags2 & OUTFILEIO)) HookLibraryEx(hModule, HooksFiles, libname);
	if(dxw.dwFlags11 & CUSTOMLOCALE) HookLibraryEx(hModule, HooksLocale, libname);
	return;
}

FARPROC Remap_MSVCRT_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;
	if(dxw.dwFlags11& SAFEALLOCS) if(addr=RemapLibraryEx(proc, hModule, HooksAlloc)) return addr;
	if((dxw.dwFlags10 & (FAKECDDRIVE | FAKEHDDRIVE)) || (dxw.dwTFlags2 & OUTFILEIO)) 
		if(addr=RemapLibraryEx(proc, hModule, HooksFiles)) return addr;
	if(dxw.dwFlags11& CUSTOMLOCALE) if(addr=RemapLibraryEx(proc, hModule, HooksLocale)) return addr;
	return NULL;
}

LPVOID CDECL extmalloc(size_t size)
{
	LPVOID ret;
	OutTraceSYS("malloc: size=%d\n", size);
	ret = (*pmalloc)(size+8); // add two extra dwords 
	memcpy(ret, "AA", 2);
	memcpy((LPBYTE)ret+2, &size, sizeof(size_t));
	memcpy((LPBYTE)ret+size+6, "ZZ", 2);
	return (LPBYTE)ret+6;
}

LPVOID CDECL extrealloc(LPVOID addr, size_t size)
{
	LPVOID ret;
	size_t origLen;
	BOOL bCorrupted = FALSE;
	OutTraceSYS("realloc: addr=%#x size=%d\n", addr, size);
	if(!addr){
		// caveat: realloc of NULL address behaves like malloc
		ret = (*prealloc)(NULL, size+8); // add two extra dwords 
		memcpy(ret, "AA", 2);
		memcpy((LPBYTE)ret+2, &size, sizeof(size_t));
		memcpy((LPBYTE)ret+size+6, "ZZ", 2);
		return (LPBYTE)ret+6;
	}
	origLen = (size_t)((LPBYTE)addr-4);
	if(memcmp((LPBYTE)addr-6, "AA", 2)) {
		OutTraceE("realloc: corrupted buffer HEAD @%#x - skip\n", addr);
		HexTrace((LPBYTE)addr-6, 6);
		bCorrupted = TRUE;
	}
	if(memcmp((LPBYTE)addr+origLen, "ZZ", 2)) {
		OutTraceE("realloc: corrupted buffer TAIL @%#x - skip\n", addr);
		HexTrace((LPBYTE)addr+origLen, 2);
		bCorrupted = TRUE;
	}
#ifdef ASSERTDIALOGS
	if(bCorrupted){
		MessageBox(NULL, "Warning: realloc on corrupted buffer", "DxWnd", 0);
	}
#endif // ASSERTDIALOGS
	ret = (*prealloc)((LPBYTE)addr-6, size+8);
	memcpy(ret, "AA", 2);
	memcpy((LPBYTE)ret+2, &size, sizeof(size_t));
	memcpy((LPBYTE)ret+size+6, "ZZ", 2);
	return (LPBYTE)ret+6;
}

VOID CDECL extfree(LPVOID addr)
{
	OutTraceSYS("free: addr=%#x\n", addr);

	size_t origLen = (size_t)((LPBYTE)addr-4);
	if(memcmp((LPBYTE)addr-6, "AA", 2)) {
		if(!memcmp((LPBYTE)addr-6, "FF", 2)) {
			OutTraceE("free: freed twice buffer @%#x - skip\n", addr);
#ifdef ASSERTDIALOGS
			MessageBox(NULL, "Warning: double freed buffer", "DxWnd", 0);
#endif // ASSERTDIALOGS
		}
		else {
			OutTraceE("free: corrupted buffer HEAD @%#x - skip\n", addr);
			HexTrace((LPBYTE)addr-6, 6);
#ifdef ASSERTDIALOGS
			MessageBox(NULL, "Warning: free on corrupted buffer", "DxWnd", 0);
#endif // ASSERTDIALOGS
		}
		return;
	}

	if(memcmp((LPBYTE)addr+origLen, "ZZ", 2)) {
		OutTraceE("free: corrupted buffer TAIL @%#x - skip\n", addr);
		HexTrace((LPBYTE)addr+origLen, 2);
#ifdef ASSERTDIALOGS
		MessageBox(NULL, "Warning: free on corrupted buffer", "DxWnd", 0);
#endif // ASSERTDIALOGS
		return;
	}

	memcpy((LPBYTE)addr-6, "FF", 2);
	(*pfree)((LPBYTE)addr-6);
}

FILE * CDECL extfopen(const char *fname, const char *mode)
{
	ApiName("fopen");
	FILE *ret;

	OutTraceSYS("%s: path=\"%s\" mode=\"%s\"\n", ApiRef, fname, mode);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		fname = dxwTranslatePathA(fname, &mapping);

		// if mapped on virtual CD and write access required, you should fake a no access error code
		if(mapping == DXW_FAKE_CD){
			if(strpbrk(mode, "wa+")){ // write, append or overwrite are all failing on a CD drive
				OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
				// should set lasterror here?
				SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
				return NULL;
			}
		}
	}

	ret = (*pfopen)(fname, mode);
	OutTraceSYS("%s: path=\"%s\" ret(fp)=%#x\n", ApiRef, fname, ret);
	return ret;
}

int CDECL ext_open(const char *fname, int oflag, int mode)
{
	ApiName("_open");
	int ret;
	OutTraceSYS("%s: path=\"%s\" oflag=%#x mode=%#x\n", ApiRef, fname, oflag, mode);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		fname = dxwTranslatePathA(fname, &mapping);

		// if mapped on virtual CD and write access required, you should fake a no access error code
		if(mapping == DXW_FAKE_CD){
			if(oflag & (_O_WRONLY | _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC | _O_EXCL)){ 
				OutTraceDW("%s: simulate ERROR_ACCESS_DENIED on CD\n", ApiRef);
				// should set lasterror here?
				SetLastError(ERROR_ACCESS_DENIED);// assuming the file was there, ERROR_FILE_NOT_FOUND otherwise ?
				return -1;
			}
		}
	}

	ret = (*p_open)(fname, oflag, mode);
	OutTraceSYS("%s: path=\"%s\" ret(fd)=%#x\n", ApiRef, fname, ret);
	return ret;
}

/*
struct _stat64i32 {
        _dev_t     st_dev;
        _ino_t     st_ino;
        unsigned short st_mode;
        short      st_nlink;
        short      st_uid;
        short      st_gid;
        _dev_t     st_rdev;
        _off_t     st_size;
        __time64_t st_atime;
        __time64_t st_mtime;
        __time64_t st_ctime;
        };
*/

int CDECL ext_stat(const char *path,  struct _stat *buffer)
{
	// v2.06.05: added _stat wrapper, needed for @#@ "MechWarrior 3"
	ApiName("_stat");
	int ret;
	DWORD mapping = DXW_NO_FAKE;
	OutTraceSYS("%s: path=\"%s\"\n", ApiRef, path);

	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		path = dxwTranslatePathA(path, &mapping);
	}

	ret = (*p_stat)(path, buffer);
	if(IsDebugSYS){
		OutTrace("> dev: %d\n", buffer->st_dev);
		OutTrace("> ino: %d\n", buffer->st_ino);
		OutTrace("> mode: %#x\n", buffer->st_mode);
		OutTrace("> nlink: %d\n", buffer->st_nlink);
		OutTrace("> uid: %d\n", buffer->st_uid);
		OutTrace("> gid: %d\n", buffer->st_gid);
		OutTrace("> rdev: %d\n", buffer->st_rdev);
		OutTrace("> size: %d\n", buffer->st_size);
	}

	if(mapping == DXW_FAKE_CD){
		// force read only flag
		if(buffer->st_mode & _S_IWRITE) {
			OutTraceSYS("%s: clearing _S_IWRITE flag on fake-CD device\n");
			buffer->st_mode &= ~_S_IWRITE;
		}
	}

	OutTraceSYS("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

// ========== Locale handling ==========

static _locale_t getLocale()
{
	static _locale_t curLocale = NULL;
	if(curLocale) return curLocale; // return the cached value

	if(!p_create_locale){
		HMODULE hmod = (*pLoadLibraryA)("MSVCRT.dll");
		p_create_locale = (_create_locale_Type)(*pGetProcAddress)(hmod, "_create_locale");
	}
	if(p_create_locale){
		char codepage[20];
		sprintf(codepage, ".%d", dxw.CodePage);
		curLocale = (*p_create_locale)(LC_ALL, codepage);
		if(!curLocale) {
			OutTraceSYS("_create_locale: ERROR err=%d\n", GetLastError());
		}
	}
	OutTraceSYS("dxwnd.GetLocale: lc_codepage=%#x lpcollate_cp=%#x\n", 
		curLocale->locinfo->lc_codepage,
		curLocale->locinfo->lc_collate_cp);
	return curLocale;
}

UINT CDECL ext__lc_codepage(void)
{
	OutTraceSYS("__lc_codepage: codepage=%d -> %d\n", (*p__lc_codepage)(), dxw.CodePage);
	return dxw.Locale;
}

size_t CDECL ext_mbclen(const unsigned char *c)
{
	size_t ret;
	OutTraceSYS("_mbclen: c=%s\n", c);
	if(!p_mbclen_l){
		HMODULE hmod = (*pLoadLibraryA)("MSVCRT.dll");
		p_mbclen_l = (_mbclen_l_Type)(*pGetProcAddress)(hmod, "_mbclen_l");
	}
	ret = (*p_mbclen_l)(c, getLocale());
	OutTraceSYS("> len=%d\n", ret);
	return ret;
}

int CDECL extmblen(const char *mbstr, size_t count)
{
	size_t ret;
	OutTraceSYS("mblen: c=%s count=%d\n", mbstr, count);
	if(!p_mblen_l){
		HMODULE hmod = (*pLoadLibraryA)("MSVCRT.dll");
		p_mblen_l = (_mblen_l_Type)(*pGetProcAddress)(hmod, "_mblen_l");
	}
	ret = p_mblen_l(mbstr, count, getLocale());
	OutTraceSYS("> len=%d\n", ret);
	return ret;
}

// to do: _mbbtombc, _mbbtombc_l, _mbscmp, _setmbcp, __lc_codepage, __p___mb_cur_max

void CDECL ext__set_app_type(int appType)
{
	ApiName("__set_app_type");
	OutTraceSYS("%s: type=%d\n", ApiRef, appType);
	(*p__set_app_type)(appType);
	ReHook();
}

void CDECL ext_initterm(LPVOID p1, LPVOID p2)
{
	ApiName("_initterm");
	OutTraceSYS("%s: p1=%#x p2=%#x\n", ApiRef, p1, p2);
	(*p_initterm)(p1, p2);
	ReHook();
}

int CDECL ext_access(const char *path, int mode)
{
	ApiName("_access");
	int ret;
	OutTraceSYS("%s: path=\"%s\" mode=%#x\n", ApiRef, path, mode);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		path = dxwTranslatePathA(path, &mapping);
		if(mapping != DXW_NO_FAKE) OutTraceSYS("%s: REMAP path=\"%s\"\n", ApiRef, path);
	}
	ret=(*p_access)(path, mode);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

int CDECL ext_waccess(const wchar_t *path, int mode)
{
	ApiName("_waccess");
	int ret;
	OutTraceSYS("%s: path=\"%ls\" mode=%#x\n", ApiRef, path, mode);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		path = dxwTranslatePathW(path, &mapping);
		if(mapping != DXW_NO_FAKE) OutTraceSYS("%s: REMAP path=\"%ls\"\n", ApiRef, path);
	}
	ret=(*p_waccess)(path, mode);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

errno_t CDECL ext_access_s(const char *path, int mode)
{
	ApiName("_access_s");
	errno_t ret;
	OutTraceSYS("%s: path=\"%s\" mode=%#x\n", ApiRef, path, mode);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		path = dxwTranslatePathA(path, &mapping);
		if(mapping != DXW_NO_FAKE) OutTraceSYS("%s: REMAP path=\"%s\"\n", ApiRef, path);
	}
	ret=(*p_access_s)(path, mode);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

errno_t CDECL ext_waccess_s(const wchar_t *path, int mode)
{
	ApiName("_waccess_s");
	errno_t ret;
	OutTraceSYS("%s: path=\"%ls\" mode=%#x\n", ApiRef, path, mode);
	if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
		DWORD mapping;
		path = dxwTranslatePathW(path, &mapping);
		if(mapping != DXW_NO_FAKE) OutTraceSYS("%s: REMAP path=\"%ls\"\n", ApiRef, path);
	}
	ret=(*p_waccess_s)(path, mode);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

#ifdef TRACEALL
errno_t CDECL ext_chdrive(int drive)
{
	ApiName("_chdrive");
	errno_t ret;
	OutTraceSYS("%s: drive=%d(%c)\n", ApiRef, drive, 'A'+drive);
	//if(dxw.dwFlags10 & (FAKEHDDRIVE | FAKECDDRIVE)) {
	//	DWORD mapping;
	//	path = dxwTranslatePathW(path, &mapping);
	//	if(mapping != DXW_NO_FAKE) OutTraceSYS("%s: REMAP path=\"%ls\"\n", ApiRef, path);
	//}
	ret=(*p_chdrive)(drive);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

int CDECL ext_getdrive(void)
{
	ApiName("_getdrive");
	int ret;
	ret = (*p_getdrive)();
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

errno_t CDECL ext_chdir(const char *dirname)
{
	ApiName("_chdir");
	errno_t ret;
	OutTraceSYS("%s: dirname=\"%s\"\n", ApiRef, dirname);
	ret = (*p_chdir)(dirname);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

size_t CDECL extfread(void *buffer, size_t size, size_t count, FILE *stream)
{
	ApiName("fread");
	size_t ret;
	OutTraceSYS("%s: buf=%#x size=%d count=%d fp=%#x\n", ApiRef, buffer, size, count, stream);
	ret = (*pfread)(buffer, size, count, stream);
	OutTraceSYS("%s: ret=%d\n", ApiRef, ret);
	return ret;
}

FILE * CDECL ext_fdopen(int fd, const char *mode)
{
	ApiName("_fdopen");
	FILE *ret;
	OutTraceSYS("%s: fd=%#x mode=%s\n", ApiRef, fd, mode);
	ret = (*p_fdopen)(fd, mode);
	OutTraceSYS("%s: ret(fp)=%#x\n", ApiRef, ret);
	return ret;
}

int CDECL ext_mkdir(const char *dirname)
{
	ApiName("_mkdir");
	int ret;
	OutTraceSYS("%s: dirname=%s\n", ApiRef, dirname);
	ret = (*p_mkdir)(dirname);
	OutTraceSYS("%s: ret=%#x\n", ApiRef, ret);
	return ret;
}

char * CDECL ext_fullpath(char *absPath, const char *relPath, size_t maxLength)
{
	ApiName("_fullpath");
	char * ret;
	//OutTraceSYS("%s: abspath=%s relpath=%s maxlen=%d\n", ApiRef, absPath, relPath, maxLength);
	OutTraceSYS("%s: relpath=%s maxlen=%d\n", ApiRef, relPath, maxLength);
	ret = (*p_fullpath)(absPath, relPath, maxLength);
	OutTraceSYS("%s: ret=%s\n", ApiRef, ret);
	return ret;
}

wchar_t * CDECL ext_wfullpath(wchar_t *absPath, const wchar_t *relPath, size_t maxLength)
{
	ApiName("_wfullpath");
	wchar_t * ret;
	//OutTraceSYS("%s: abspath=%s relpath=%s maxlen=%d\n", ApiRef, absPath, relPath, maxLength);
	OutTraceSYS("%s: relpath=%ls maxlen=%d\n", ApiRef, relPath, maxLength);
	ret = (*p_wfullpath)(absPath, relPath, maxLength);
	OutTraceSYS("%s: ret=%ls\n", ApiRef, ret);
	return ret;
}
#endif // TRACEALL