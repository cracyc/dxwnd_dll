typedef struct tagDPNAME {
	DWORD   dwSize;
	DWORD   dwFlags;            /* Not used must be 0 */
	union {/*playerShortName */      /* Player's Handle? */
		LPWSTR  lpszShortName;
		LPSTR   lpszShortNameA;
	} DUMMYUNIONNAME1;
	union {/*playerLongName */       /* Player's formal/real name */
		LPWSTR  lpszLongName;
		LPSTR   lpszLongNameA;
	} DUMMYUNIONNAME2;
} DPNAME, *LPDPNAME;

typedef const DPNAME *LPCDPNAME;

typedef void *LPDPENUMPLAYERSCALLBACK2(int, char *, char *, int, void *);
typedef struct tagDPSESSIONDESC2 {
	DWORD   dwSize;
	DWORD   dwFlags;
	GUID    guidInstance;
	GUID    guidApplication;   /* GUID of the DP application, GUID_NULL if all applications! */
	DWORD   dwMaxPlayers;
	DWORD   dwCurrentPlayers;   /* (read only value) */
	union { /* Session name */
		LPWSTR  lpszSessionName;
		LPSTR   lpszSessionNameA;
	} DUMMYUNIONNAME1;
	union { /* Optional password */
		LPWSTR  lpszPassword;
		LPSTR   lpszPasswordA;
	} DUMMYUNIONNAME2;
	DWORD   dwReserved1;
	DWORD   dwReserved2;
	DWORD   dwUser1;        /* For use by the application */
	DWORD   dwUser2;
	DWORD   dwUser3;
	DWORD   dwUser4;
} DPSESSIONDESC2, *LPDPSESSIONDESC2;

typedef const DPSESSIONDESC2* LPCDPSESSIONDESC2;

typedef BOOL (CALLBACK *LPDPENUMSESSIONSCALLBACK2)(
	LPCDPSESSIONDESC2   lpThisSD,
	LPDWORD             lpdwTimeOut,
	DWORD               dwFlags,
	LPVOID              lpContext );

typedef BOOL (CALLBACK *LPDPENUMCONNECTIONSCALLBACK)(
	LPCGUID     lpguidSP,
	LPVOID      lpConnection,
	DWORD       dwConnectionSize,
	LPCDPNAME   lpName,
	DWORD       dwFlags,
	LPVOID      lpContext);
 
typedef BOOL (CALLBACK *LPDPENUMSESSIONSCALLBACK)(
	LPDPSESSIONDESC lpDPSessionDesc,
	LPVOID      lpContext,
	LPDWORD     lpdwTimeOut,
	DWORD       dwFlags);
