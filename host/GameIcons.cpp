#include "stdafx.h"
#include "GameIcons.h"
#include "Shlwapi.h"
#include <pshpack1.h>

//#define ICONSDEBUG

extern BOOL g32BitIcons;
extern void OutTrace(const char *, ...);

bool SaveIconToFile(HICON [], int, const TCHAR *);

void char2hex(unsigned char *src, char *dst, int len)
{
	for (; len; len--){
		sprintf(dst, "%02.2X", *src); 
		src++;
		dst += 2;
	}
	*dst=0; // terminator
}

int hex2char(unsigned char *dst, char *src)
{
	char buf[3];
	int val;
	int len = strlen(src);
	int retlen = 0;
	buf[2]=0; // string terminator
	for (; len>1; len-=2, retlen++){ // v2.05.61 fix
		buf[0]=*src;
		buf[1]=*(src+1);
		sscanf(buf, "%X", &val);
		*dst = (unsigned char)val;
		src += 2;
		dst ++;
	}
	return retlen;
}


#pragma pack(push)
#pragma pack(2)
typedef struct
{
    BYTE        bWidth;          // Width, in pixels, of the image
    BYTE        bHeight;         // Height, in pixels, of the image
    BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;       // Reserved ( must be 0)
    WORD        wPlanes;         // Color Planes
    WORD        wBitCount;       // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    DWORD       dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
    WORD           idReserved;   // Reserved (must be 0)
    WORD           idType;       // Resource Type (1 for icons)
    WORD           idCount;      // How many images?
} ICONDIR, *LPICONDIR;
#pragma pack(pop)

#define WARN OutTrace
#define FIXME OutTrace
#define HANDLE16 WORD

typedef struct
{
    WORD     offset;
    WORD     length;
    WORD     flags;
    WORD     id;
    HANDLE16 handle;
    WORD     usage;
} NE_NAMEINFO;
typedef DWORD FARPROC16;

typedef struct
{
    WORD        type_id;   /* Type identifier */
    WORD        count;     /* Number of resources of this type */
    FARPROC16   resloader; /* SetResourceHandler() */
    /*
     * Name info array.
     */
} NE_TYPEINFO;

#define NE_RSCTYPE_CURSOR             0x8001
#define NE_RSCTYPE_BITMAP             0x8002
#define NE_RSCTYPE_ICON               0x8003
#define NE_RSCTYPE_MENU               0x8004
#define NE_RSCTYPE_DIALOG             0x8005
#define NE_RSCTYPE_STRING             0x8006
#define NE_RSCTYPE_FONTDIR            0x8007
#define NE_RSCTYPE_FONT               0x8008
#define NE_RSCTYPE_ACCELERATOR        0x8009
#define NE_RSCTYPE_RCDATA             0x800a
#define NE_RSCTYPE_GROUP_CURSOR       0x800c
#define NE_RSCTYPE_GROUP_ICON         0x800e
#define NE_RSCTYPE_SCALABLE_FONTPATH  0x80cc   /* Resource found in .fot files */

static BYTE * USER32_LoadResource(LPBYTE peimage, NE_NAMEINFO* pNInfo, WORD sizeShift, ULONG *uSize)
{
    TRACE("%p %p 0x%08x\n", peimage, pNInfo, sizeShift);

    *uSize = (DWORD)pNInfo->length << sizeShift;
    return peimage + ((DWORD)pNInfo->offset << sizeShift);
}

typedef struct
{
    BYTE        bWidth;          /* Width, in pixels, of the image	*/
    BYTE        bHeight;         /* Height, in pixels, of the image	*/
    BYTE        bColorCount;     /* Number of colors in image (0 if >=8bpp) */
    BYTE        bReserved;       /* Reserved ( must be 0)		*/
    WORD        wPlanes;         /* Color Planes			*/
    WORD        wBitCount;       /* Bits per pixel			*/
    DWORD       dwBytesInRes;    /* How many bytes in this resource?	*/
    DWORD       dwImageOffset;   /* Where in the file is this image?	*/
} icoICONDIRENTRY, *LPicoICONDIRENTRY;

typedef struct
{
    WORD            idReserved;   /* Reserved (must be 0) */
    WORD            idType;       /* Resource Type (RES_ICON or RES_CURSOR) */
    WORD            idCount;      /* How many images */
    icoICONDIRENTRY idEntries[1]; /* An entry for each image (idCount of 'em) */
} icoICONDIR, *LPicoICONDIR;

static BYTE * ICO_LoadIcon(LPBYTE peimage, LPicoICONDIRENTRY lpiIDE, ULONG *uSize)
{
    TRACE("%p %p\n", peimage, lpiIDE);

    *uSize = lpiIDE->dwBytesInRes;
    return peimage + lpiIDE->dwImageOffset;
}

#define STEP OutTrace("STEP @%d\n", __LINE__)

UINT NE_ExtractIcon(LPCSTR lpszExeFileName,
    HICON * RetPtr,
    INT nIconIndex,
    UINT nIcons,
    UINT cxDesired,
    UINT cyDesired,
    UINT *pIconId,
    UINT flags)
{
    //user32/exticon.c

    UINT		ret = 0;
    UINT		cx1, cx2, cy1, cy2;
    LPBYTE		pData;
    HANDLE		hFile;
    UINT16		iconDirCount = 0, iconCount = 0;
    LPBYTE		image;
    HANDLE		fmapping;
    DWORD		fsizeh, fsizel;

    hFile = CreateFileA(lpszExeFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    fsizel = GetFileSize(hFile, &fsizeh);

    /* Map the file */
    fmapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    CloseHandle(hFile);
    if (!fmapping)
    {
        WARN("CreateFileMapping error %d\n", GetLastError());
        return 0xFFFFFFFF;
    }

    if (!(image = (LPBYTE)MapViewOfFile(fmapping, FILE_MAP_READ, 0, 0, 0)))
    {
        WARN("MapViewOfFile error %d\n", GetLastError());
        CloseHandle(fmapping);
        return 0xFFFFFFFF;
    }
    CloseHandle(fmapping);
    cx1 = LOWORD(cxDesired);
    cx2 = HIWORD(cxDesired);
    cy1 = LOWORD(cyDesired);
    cy2 = HIWORD(cyDesired);

    if (pIconId) /* Invalidate first icon identifier */
        *pIconId = 0xFFFFFFFF;

    if (!pIconId) /* if no icon identifier array present use the icon handle array as intermediate storage */
        pIconId = (UINT*)RetPtr;
    const IMAGE_DOS_HEADER *mz_header = (const IMAGE_DOS_HEADER *)image;
    const IMAGE_OS2_HEADER *ne_header;

    if (fsizel < sizeof(*mz_header)) goto end;
    if (mz_header->e_magic != IMAGE_DOS_SIGNATURE) goto end;
    ne_header = (const IMAGE_OS2_HEADER *)((const char *)image + mz_header->e_lfanew);
    if (mz_header->e_lfanew + sizeof(*ne_header) > fsizel) goto end;
    if (ne_header->ne_magic == IMAGE_NT_SIGNATURE) goto end;  /* win32 exe */
    if (ne_header->ne_magic != IMAGE_OS2_SIGNATURE) goto end;

    pData = image + mz_header->e_lfanew + ne_header->ne_rsrctab;
    /* end ico file */
    if (ne_header->ne_rsrctab < ne_header->ne_restab)
    {
        BYTE		*pCIDir = 0;
        NE_TYPEINFO	*pTInfo = (NE_TYPEINFO*)(pData + 2);
        NE_NAMEINFO	*pIconStorage = NULL;
        NE_NAMEINFO	*pIconDir = NULL;
        LPicoICONDIR	lpiID = NULL;
        ULONG		uSize = 0;

        if (pData == (BYTE*)-1)
        {
            FIXME("ICO_GetIconDirectory\n");
            /*
            pCIDir = ICO_GetIconDirectory(peimage, &lpiID, &uSize);	// check for .ICO file
            if (pCIDir)
            {
                iconDirCount = 1; iconCount = lpiID->idCount;
                TRACE("-- icon found %p 0x%08x 0x%08x 0x%08x\n", pCIDir, uSize, iconDirCount, iconCount);
            }
            */
        }
        else while (pTInfo->type_id && !(pIconStorage && pIconDir))
        {
           if (pTInfo->type_id == NE_RSCTYPE_GROUP_ICON)	/* find icon directory and icon repository */
            {
                iconDirCount = pTInfo->count;
                pIconDir = ((NE_NAMEINFO*)(pTInfo + 1));
                TRACE("\tfound directory - %i icon families\n", iconDirCount);
            }
            if (pTInfo->type_id == NE_RSCTYPE_ICON)
            {
                iconCount = pTInfo->count;
                pIconStorage = ((NE_NAMEINFO*)(pTInfo + 1));
                TRACE("\ttotal icons - %i\n", iconCount);
            }
           pTInfo = (NE_TYPEINFO *)((char*)(pTInfo + 1) + pTInfo->count * sizeof(NE_NAMEINFO));
        }

        if ((pIconStorage && pIconDir) || lpiID)	  /* load resources and create icons */
        {
            if (nIcons == 0)
            {
                ret = iconDirCount;
                if (lpiID)	/* *.ico file, deallocate heap pointer*/
                    HeapFree(GetProcessHeap(), 0, pCIDir);
            }
            else if (nIconIndex < iconDirCount)
            {
               UINT16   i, icon;
                if (nIcons > (UINT)iconDirCount - (UINT)nIconIndex)
                    nIcons = iconDirCount - nIconIndex;

                for (i = 0; i < nIcons; i++)
                {
                  /* .ICO files have only one icon directory */
                    if (lpiID == NULL)	/* not *.ico */
                        pCIDir = USER32_LoadResource(image, pIconDir + i + nIconIndex, *(WORD*)pData, &uSize);
                    pIconId[i] = LookupIconIdFromDirectoryEx(pCIDir, TRUE, cx1, cy1, flags);
                    if (cx2 && cy2) pIconId[++i] = LookupIconIdFromDirectoryEx(pCIDir, TRUE, cx2, cy2, flags);
                }
                if (lpiID)	/* *.ico file, deallocate heap pointer*/
                    HeapFree(GetProcessHeap(), 0, pCIDir);

                for (icon = 0; icon < nIcons; icon++)
                {
                    pCIDir = NULL;
                    if (lpiID)
                        pCIDir = ICO_LoadIcon(image, lpiID->idEntries + (int)pIconId[icon], &uSize);
                    else
                        for (i = 0; i < iconCount; i++)
                            if (pIconStorage[i].id == ((int)pIconId[icon] | 0x8000))
                                pCIDir = USER32_LoadResource(image, pIconStorage + i, *(WORD*)pData, &uSize);

                    if (pCIDir)
                    {
                      RetPtr[icon] = CreateIconFromResourceEx(pCIDir, uSize, TRUE, 0x00030000, cx1, cy1, flags);
                        if (cx2 && cy2)
                            RetPtr[++icon] = CreateIconFromResourceEx(pCIDir, uSize, TRUE, 0x00030000, cx2, cy2, flags);
                    }
                    else
                        RetPtr[icon] = 0;
                }
                ret = icon;	/* return number of retrieved icons */
            }
        }
    }
    else
    {

    }
end:
    UnmapViewOfFile(image);	/* success */
    return ret;
}

#define MAXICONS 20
HICON Icons[MAXICONS];

HICON CGameIcons::Extract(CString path)
{
	HICON Icon;
	// v2.04.81: LoadLibrary fails when trying to load *.ico files, so do not return NULL on failure.
	HINSTANCE Hinst = ::LoadLibrary(path);
	if(Hinst){
		Icon = ::ExtractIcon(Hinst, path.GetBuffer(), 0);
		::FreeLibrary(Hinst); // v2.005.01 fixed
		if(Icon) {
			OutTrace("icon extracted from ExtractIcon path=%s\n", path.GetBuffer());
			return Icon;
		}
	}

	// if not found ...
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char SearchPath[MAX_PATH+1];
	strcpy(SearchPath, path); 
	PathRemoveFileSpec(SearchPath);
	strcat(SearchPath, "\\*.ico");
	hFind = FindFirstFile(SearchPath, &FindFileData);
	if ((hFind != INVALID_HANDLE_VALUE) && (hFind != (HANDLE)ERROR_FILE_NOT_FOUND)){
		strncpy(SearchPath, path, MAX_PATH); 
		PathRemoveFileSpec(SearchPath);
		strcat(SearchPath, "\\");
		strncat(SearchPath, FindFileData.cFileName, MAX_PATH);
		Icon = ::ExtractIcon(NULL, SearchPath, 0);
		FindClose(hFind);
		OutTrace("icon extracted from .ico file=%s\n", SearchPath);
		return Icon;
	}
	else{
		UINT best[2 * MAXICONS] = {0}; 
		//UINT ret = NE_ExtractIcon(path, &Icons[0], 0, 20, MAKELONG(24, 48), MAKELONG(24, 48), &best, LR_DEFAULTSIZE | LR_LOADFROMFILE);
		//UINT ret = NE_ExtractIcon(path, &Icons[0], 0, 20, MAKELONG(32,0), MAKELONG(32,0), best, LR_DEFAULTSIZE | LR_LOADFROMFILE);
		//UINT ret = NE_ExtractIcon(path, &Icons[0], 0, 20, MAKELONG(48, 32), MAKELONG(48, 32), &best[0], LR_DEFAULTSIZE | LR_LOADFROMFILE);
		UINT ret = NE_ExtractIcon(path, &Icons[0], 0, 20, MAKELONG(64, 32), MAKELONG(64, 32), &best[0], LR_DEFAULTSIZE | LR_LOADFROMFILE);
		if(ret && (ret != 0xFFFFFFFF)) {
			OutTrace("icon extracted from NE_ExtractIcon file=%s icons=%d best=[%d,%d]\n", path, ret, best[0], best[1]);
			return Icons[best[0]];
		}
	}

	OutTrace("icon not found\n");
	Icon = (HICON)NULL;
	return Icon;
}

#define TMPICONPATH "extract.ico"
//#define TMPICONPATH "extract::$DATA"

static char *IconSerialize4Bit(HICON Icon)
{
#ifdef ICONSDEBUG
	OutTrace("IconSerialize4Bit hicon=%#x\n", Icon);
#endif
	// save icon to char buffer
	PICTDESC desc = { sizeof(PICTDESC)};
	desc.picType = PICTYPE_ICON;
	desc.icon.hicon = Icon;
	IPicture *pPicture = 0;
	HRESULT hr = OleCreatePictureIndirect(&desc, IID_IPicture, FALSE, (void **)&pPicture);
	if(FAILED(hr)) {
#ifdef ICONSDEBUG
		OutTrace("IconSerialize4Bit OleCreatePictureIndirect ERROR err=%d\n", GetLastError());
#endif
		return NULL;
	}
	IStream *pStream = 0;
	hr = CreateStreamOnHGlobal(0, TRUE, &pStream);
	if(FAILED(hr)) {
#ifdef ICONSDEBUG
		OutTrace("IconSerialize4Bit CreateStreamOnHGlobal ERROR err=%d\n", GetLastError());
#endif
		return NULL;
	}
	LONG cbSize = 0;
	hr = pPicture->SaveAsFile(pStream, TRUE, &cbSize);
	if(FAILED(hr)) {
#ifdef ICONSDEBUG
		OutTrace("IconSerialize4Bit CreateStreamOnHGlobal ERROR err=%d\n", GetLastError());
#endif
		return NULL;
	}

	HGLOBAL hBuf = 0;
	GetHGlobalFromStream(pStream, &hBuf);
	void *buffer = GlobalLock(hBuf);
	char *hexbuf = (char *)malloc(cbSize*2 + 16);
	char2hex((unsigned char *)buffer, hexbuf, cbSize);
#ifdef ICONSDEBUG
	HANDLE hFile = CreateFile("debug.ico", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(hFile){
		DWORD written = 0;
		WriteFile(hFile, buffer, cbSize, &written, 0);
		CloseHandle(hFile);
	}
#endif
	return hexbuf;
}

static char *IconSerialize32Bit(HICON Icon)
{
#ifdef ICONSDEBUG
	OutTrace("IconSerialize32Bit hicon=%#x\n", Icon);
#endif
	_unlink(TMPICONPATH);
	SaveIconToFile(&Icon, 1, TMPICONPATH);
	FILE *f = fopen(TMPICONPATH, "rb");
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	BYTE *buf = (BYTE *)malloc(len);
	fseek(f, 0, SEEK_SET);
	fread(buf, len, 1, f);
	char *hbuf = (char *)malloc(2*len + 16);
	char2hex(buf, hbuf, len);
	free(buf);
	fclose(f);
	_unlink(TMPICONPATH);
	return hbuf;
}

char *CGameIcons::Serialize(HICON Icon)
{
	if(g32BitIcons)
		return IconSerialize32Bit(Icon);
	else
		return IconSerialize4Bit(Icon);
}

HICON CGameIcons::DeSerialize(char *buf)
{
#ifdef ICONSDEBUG
	OutTrace("DeSerialize\n");
#endif
	int len = strlen(buf);
#ifdef ICONSDEBUG
	OutTrace("DeSerialize: len=%d\n", len);
#endif
	if(!len) {
#ifdef ICONSDEBUG
		OutTrace("DeSerialize: len=0\n");
#endif
		return NULL; // no serialized icon
	}
	_unlink(TMPICONPATH);
	HANDLE hFile = CreateFile(
		TMPICONPATH, 
		GENERIC_WRITE, 
		0, // FILE_SHARE_READ | FILE_SHARE_WRITE, 
		0, 
		CREATE_ALWAYS, 
		0, // FILE_ATTRIBUTE_TEMPORARY, // FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 
		0);
	if(!hFile) {
		OutTrace("DeSerialize: CreateFile error path=%s err=%d\n", TMPICONPATH, GetLastError());
		return NULL;
	}
	unsigned char *buffer = (unsigned char *)malloc(len/2 + 16);
	len = hex2char(buffer, buf);
	if(!len){
		OutTrace("DeSerialize: icon hex2char error\n");
		CloseHandle(hFile);
		_unlink(TMPICONPATH);
		return NULL;
	}
	DWORD written = 0;
	WriteFile(hFile, buffer, len, &written, 0);
	CloseHandle(hFile);
	if(len != written){
		OutTrace("DeSerialize: WriteFile failure: error=%d\n", GetLastError());
		CloseHandle(hFile);
		_unlink(TMPICONPATH);
		return NULL;
	}
	HICON hResIcon = (HICON)LoadImage(
		NULL, 
		TMPICONPATH,
		IMAGE_ICON,
		0,
		0,
		LR_DEFAULTSIZE | LR_LOADFROMFILE);
	if(hResIcon == NULL) {
		OutTrace("de-serialize error %d\n", GetLastError());
	}
#ifndef DEBUG
	_unlink(TMPICONPATH);
#endif
	return hResIcon;
}

HICON CGameIcons::GrayIcon(HICON Icon)
{
#ifdef ICONSDEBUG
	OutTrace("GrayIcon: hicon=%#x\n", Icon);
#endif
	// n.b. gets in input a icon handle for any type of icon and returns a gray 4 bits x pixel icon
	BYTE *buf;
	char *hexbuf;
	BYTE *bmpdata, *p;
	ICONDIRENTRY *id;
	HICON GrayIco;
	
	PICTDESC desc = { sizeof(PICTDESC)};
	desc.picType = PICTYPE_ICON;
	desc.icon.hicon = Icon;
	IPicture *pPicture = 0;
	HRESULT hr = OleCreatePictureIndirect(&desc, IID_IPicture, FALSE, (void **)&pPicture);
	if(FAILED(hr)) {
		OutTrace("GrayIcon: OleCreatePictureIndirect error err=%d\n", GetLastError());
		return NULL;
	}

	IStream *pStream = 0;
	CreateStreamOnHGlobal(0, TRUE, &pStream);
	LONG cbSize = 0;
	hr = pPicture->SaveAsFile(pStream, TRUE, &cbSize);
	if(FAILED(hr)) {
		OutTrace("GrayIcon: SaveAsFile error hr=%#x\n", hr);
		return NULL;
	}

	HGLOBAL hBuf = 0;
	hr = GetHGlobalFromStream(pStream, &hBuf);
	if(FAILED(hr)) {
		OutTrace("GrayIcon: GetHGlobalFromStream error hr=%#x\n", hr);
		return NULL;
	}

	buf = (BYTE *)GlobalLock(hBuf);
	if(buf == NULL) {
		OutTrace("GrayIcon: GlobalLock error err=%d\n", GetLastError());
		return NULL;
	}

	// point to bitmap data and replace all pixels (2 pixels x byte) with 0x77 = gray,gray
	id=(ICONDIRENTRY *)&buf[6];
	//OutTrace("GrayIcon: buf=%#x\n", buf);
	//OutTrace("GrayIcon: id=%d\n", id);
	//OutTrace("GrayIcon: imageoffset=%d\n", id->dwImageOffset);
	//OutTrace("GrayIcon: bmpdata=%d\n", id->dwImageOffset+sizeof(BITMAPINFOHEADER)+sizeof(ICONDIR));
	bmpdata=&buf[id->dwImageOffset+sizeof(BITMAPINFOHEADER)+sizeof(ICONDIR)];
	int bmaplen = (32*16) + 58;
	for(p=bmpdata; p<bmpdata+bmaplen; p++){
		// Windows default 16 colors palette:
		// 0: black		1: maroon	2: green	3: olive	4: navy		5: purple	6: teal		7: silver
		// 8: grey		9: red		A: lime		B: yellow	C: blue		D: fuchsia	E: aqua		F: white
		//if(*p) *p= 0x77; -- not all pixels are colored ....
		//*p= 0x77; // silver
		//*p= 0xEE; // clear gray
		//*p= 0x00; // black
		*p= 0x88; // grey
	}

	hexbuf = (char *)malloc(2*cbSize + 16);
	char2hex(buf, hexbuf, cbSize);
	GrayIco = DeSerialize(hexbuf);
	free(hexbuf);
	return GrayIco;
}
