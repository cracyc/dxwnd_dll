typedef UINT (WINAPI *GetPrivateProfileIntA_Type)(LPCTSTR, LPCTSTR, INT, LPCTSTR);
extern GetPrivateProfileIntA_Type pGetPrivateProfileIntA;
UINT WINAPI extGetPrivateProfileIntA(LPCTSTR, LPCTSTR, INT, LPCTSTR);

typedef UINT (WINAPI *GetPrivateProfileIntW_Type)(LPCWSTR, LPCWSTR, INT, LPCWSTR);
extern GetPrivateProfileIntW_Type pGetPrivateProfileIntW;
UINT WINAPI extGetPrivateProfileIntW(LPCWSTR, LPCWSTR, INT, LPCWSTR);

typedef DWORD (WINAPI *GetPrivateProfileStringA_Type)(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);
extern GetPrivateProfileStringA_Type pGetPrivateProfileStringA;
DWORD WINAPI extGetPrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);

typedef DWORD (WINAPI *GetPrivateProfileStringW_Type)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
extern GetPrivateProfileStringW_Type pGetPrivateProfileStringW;
DWORD WINAPI extGetPrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);

typedef UINT (WINAPI *GetProfileIntA_Type)(LPCTSTR, LPCTSTR, INT);
extern GetProfileIntA_Type pGetProfileIntA;
UINT WINAPI extGetProfileIntA(LPCTSTR, LPCTSTR, INT);

typedef UINT (WINAPI *GetProfileIntW_Type)(LPCWSTR, LPCWSTR, INT);
extern GetProfileIntW_Type pGetProfileIntW;
UINT WINAPI extGetProfileIntW(LPCWSTR, LPCWSTR, INT);

typedef DWORD (WINAPI *GetProfileStringA_Type)(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD);
extern GetProfileStringA_Type pGetProfileStringA;
DWORD WINAPI extGetProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD);

typedef DWORD (WINAPI *GetProfileStringW_Type)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD);
extern GetProfileStringW_Type pGetProfileStringW;
DWORD WINAPI extGetProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD);

typedef DWORD (WINAPI *WritePrivateProfileStringA_Type)(LPCSTR, LPCSTR, LPSTR, LPCSTR);
extern WritePrivateProfileStringA_Type pWritePrivateProfileStringA;
DWORD WINAPI extWritePrivateProfileStringA(LPCSTR, LPCSTR, LPSTR, LPCSTR);

typedef DWORD (WINAPI *WritePrivateProfileStringW_Type)(LPCWSTR, LPCWSTR, LPWSTR, LPCWSTR);
extern WritePrivateProfileStringW_Type pWritePrivateProfileStringW;
DWORD WINAPI extWritePrivateProfileStringW(LPCWSTR, LPCWSTR, LPWSTR, LPCWSTR);

typedef DWORD (WINAPI *WriteProfileStringA_Type)(LPCSTR, LPCSTR, LPSTR);
extern WriteProfileStringA_Type pWriteProfileStringA;
DWORD WINAPI extWriteProfileStringA(LPCSTR, LPCSTR, LPSTR);

typedef DWORD (WINAPI *WriteProfileStringW_Type)(LPCWSTR, LPCWSTR, LPWSTR);
extern WriteProfileStringW_Type pWriteProfileStringW;
DWORD WINAPI extWriteProfileStringW(LPCWSTR, LPCWSTR, LPWSTR);


typedef DWORD (WINAPI *GetCurrentDirectoryA_Type)(DWORD, LPSTR);
extern GetCurrentDirectoryA_Type pGetCurrentDirectoryA;
extern DWORD WINAPI extGetCurrentDirectoryA(DWORD, LPSTR);
typedef DWORD (WINAPI *GetCurrentDirectoryW_Type)(DWORD, LPWSTR);
extern GetCurrentDirectoryW_Type pGetCurrentDirectoryW;
extern DWORD WINAPI extGetCurrentDirectoryW(DWORD, LPWSTR);