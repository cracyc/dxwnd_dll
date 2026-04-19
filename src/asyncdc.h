#include <windows.h>
#include "syslibs.h"

class AsyncDC
{
// Construction/destruction
public:
    AsyncDC();
    //virtual ~AsyncDC();

// Operations
public: // methods
	void	AcquireDC();
	HDC		GetDC(HDC);
	HDC		TestDC();
	BOOL	ReleaseDC();
	BOOL	RefreshDC();

private:
	HWND CacheHWnd;
	HDC CacheHDC;
	HANDLE CacheHThread;
	RECT CacheRect;
	HBITMAP CachePic;
	BOOL mainWin;
	UINT lastW;
	UINT lastH;
};

extern AsyncDC *lpADC;