typedef DWORD (WINAPI *GetProcessHeaps_Type)(DWORD, PHANDLE);
DWORD WINAPI extGetProcessHeaps(DWORD, PHANDLE);

typedef BOOL (WINAPI *HeapLock_Type)(HANDLE);
BOOL WINAPI extHeapLock(HANDLE);
BOOL WINAPI extHeapUnlock(HANDLE);

typedef BOOL (WINAPI *HeapQueryInformation_Type)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);
BOOL WINAPI extHeapQueryInformation(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);

typedef BOOL (WINAPI *HeapSetInformation_Type)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
BOOL WINAPI extHeapSetInformation(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);

typedef BOOL (WINAPI *HeapWalk_Type)(HANDLE, LPPROCESS_HEAP_ENTRY);
BOOL WINAPI extHeapWalk(HANDLE, LPPROCESS_HEAP_ENTRY);

DXWEXTERN GetProcessHeaps_Type pGetProcessHeaps;
DXWEXTERN HeapLock_Type pHeapLock, pHeapUnlock;
DXWEXTERN HeapQueryInformation_Type pHeapQueryInformation;
DXWEXTERN HeapSetInformation_Type pHeapSetInformation;
DXWEXTERN HeapWalk_Type pHeapWalk;
