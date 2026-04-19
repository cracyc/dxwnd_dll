#define  _CRT_SECURE_NO_WARNINGS
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"

/*
Return value

If the function succeeds, the return value is greater than 31.

If the function fails, the return value is one of the following error values.
Return value
Return code/value 	Description

0						The system is out of memory or resources.
ERROR_BAD_FORMAT		The .exe file is invalid.
ERROR_FILE_NOT_FOUND	The specified file was not found.
ERROR_PATH_NOT_FOUND	The specified path was not found. 
*/

UINT dxwWinExec(LPCSTR arg, UINT flags)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char *cbProcessName;
	DWORD ret = 0;

	cbProcessName = (char *)arg;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	// Start the child process.
	if(!extCreateProcessA( NULL, // No module name (use command line).
		cbProcessName, // Command line.
		NULL,             // Process handle not inheritable.
		NULL,             // Thread handle not inheritable.
		FALSE,            // Set handle inheritance to FALSE.
		0,                // No creation flags.
		NULL,             // Use parent's environment block.
		NULL,             // Use parent's starting directory.
		&si,              // Pointer to STARTUPINFO structure.
		&pi )             // Pointer to PROCESS_INFORMATION structure.
	){
		//Create Process failed
		switch(GetLastError()){
			case ERROR_FILE_NOT_FOUND: ret = ERROR_FILE_NOT_FOUND; break; // 2 -> 2
			case ERROR_PATH_NOT_FOUND: ret = ERROR_PATH_NOT_FOUND; break; // 3 -> 3
			case ERROR_BAD_FORMAT:
			case ERROR_BAD_EXE_FORMAT: ret = ERROR_BAD_FORMAT; break; // 193 -> 11
			default: ret = 0; break;
		}
	}
	else{
		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);
		// Get Return Code.
		DWORD dwExitCode = -1;
		GetExitCodeProcess(pi.hProcess, &dwExitCode);
		ret = 33; // WinExec always returns 33 on successful execution!
		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	return ret;
}