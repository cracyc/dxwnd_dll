#ifndef DXW_NOTRACES

#define _CRT_SECURE_NO_WARNINGS
#define _MODULE "dxwnd"

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "hddraw.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "shareddc.hpp"

#include "stdio.h"

void DumpDibSection(char *fname, HDC hdc, const BITMAPINFO *pbmi, UINT iUsage, VOID *pvBits)
{
	ApiName("DumpDibSection");
	static int prog=0;
	FILE *fdump;
	char path[81];
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	BITMAPV4HEADER pbi;			// bitmap info-header  
	int iScanLineSize;
	LONG bV4SizeImage;

	if(!dxw.bCustomKeyToggle) return; // dump only when toggle key is active
	if((iUsage != DIB_RGB_COLORS) && (iUsage != DIB_PAL_COLORS)) {
		OutTrace("%s: skip unsupported usage=%d\n", ApiRef, iUsage);
		return; 
	}
	if(!pvBits) {
		OutTrace("%s: skip NULL bits\n", ApiRef);
		return; 
	}
	if((pbmi->bmiHeader.biWidth == 1) && (pbmi->bmiHeader.biHeight == 1)) {
		OutTrace("%s: skip 1x1 bitmaps\n", ApiRef);
		return; // don't dump just built 1x1 DIBs
	}
	if((pbmi->bmiHeader.biCompression != BI_BITFIELDS) && (pbmi->bmiHeader.biCompression != BI_RGB)) {
		OutTrace("%s: skip compressed DIB compression=%d\n", ApiRef, pbmi->bmiHeader.biCompression);
		return; // don't dump compressed DIBs
	}
	if(prog==0) CreateDirectory(".\\bmp.out", NULL);
	sprintf(path,".\\bmp.out\\dib.%08.8d.bmp", prog);
	fdump=fopen(path, "wb");
	// v2.05.57: the program could change its working folder, in case of ERROR_PATH_NOT_FOUND try to repeat
	if(!fdump && (ERROR_PATH_NOT_FOUND == GetLastError())) {
		CreateDirectory(".\\bmp.out", NULL);
		fdump=fopen(path, "wb");
	}
	if(!fdump) {
		OutTrace("%s: fopen(%s) ERROR err=%d\n", ApiRef, path, GetLastError());
		prog++;
		return;
	}
	memset((void *)&pbi, 0, sizeof(BITMAPV4HEADER));
	pbi.bV4Size = sizeof(BITMAPV4HEADER); 
	pbi.bV4Width = pbmi->bmiHeader.biWidth;
	pbi.bV4Height = pbmi->bmiHeader.biHeight;
	pbi.bV4BitCount = pbmi->bmiHeader.biBitCount;
	bV4SizeImage = ((pbi.bV4Width * pbi.bV4BitCount + 0x1F) & ~0x1F)/8 * pbi.bV4Height; 
	if(bV4SizeImage < 0) bV4SizeImage = -bV4SizeImage;
	pbi.bV4SizeImage = bV4SizeImage;
	pbi.bV4Planes = pbmi->bmiHeader.biPlanes;  // must be 1
	//pbi.bV4V4Compression = pbmi->bmiHeader.biCompression;
	pbi.bV4V4Compression = BI_RGB;
	pbi.bV4XPelsPerMeter = 1;
	pbi.bV4YPelsPerMeter = 1;
	pbi.bV4ClrUsed = pbmi->bmiHeader.biClrUsed;
	if(!pbi.bV4ClrUsed && (pbi.bV4BitCount <= 8)) pbi.bV4ClrUsed = 1 << pbi.bV4BitCount;
	pbi.bV4ClrImportant = pbmi->bmiHeader.biClrImportant;
	pbi.bV4AlphaMask = 0;
	switch(pbi.bV4BitCount){
		case 32:
		case 24:
			pbi.bV4RedMask   = 0x00FF0000;
			pbi.bV4GreenMask = 0x0000FF00;
			pbi.bV4BlueMask  = 0x000000FF;
			break;
		case 16:
			if(pbmi->bmiHeader.biCompression == BI_RGB){
				pbi.bV4RedMask   = 0x00007C00;
				pbi.bV4GreenMask = 0x000003E0;
				pbi.bV4BlueMask  = 0x0000001F;
			}
			else {
				// could be either RGB555 or RGB565, how to tell???
				pbi.bV4RedMask   = 0x0000F800;
				pbi.bV4GreenMask = 0x000007E0;
				pbi.bV4BlueMask  = 0x0000001F;
			}
			break;
		case 8:
		default:
			pbi.bV4RedMask = 0;
			pbi.bV4GreenMask = 0;
			pbi.bV4BlueMask = 0;
			break;
	}
	pbi.bV4CSType = LCS_CALIBRATED_RGB;
	iScanLineSize = ((pbi.bV4Width * pbi.bV4BitCount + 0x1F) & ~0x1F)/8;
	OutTrace("%s: fname=%s prog=%08.8d size=%d wxh=(%dx%d) usage=%s bc=%d sizeimg=%d planes=%d comp=%#x ppm=(%dx%d) colors=%d imp=%d\n",
		ApiRef, fname, prog,
		pbi.bV4Size, pbi.bV4Width, pbi.bV4Height, 
		iUsage == DIB_RGB_COLORS ? "RGB" : "PAL",
		pbi.bV4BitCount, pbi.bV4SizeImage,
		pbi.bV4Planes, pbi.bV4V4Compression, pbi.bV4XPelsPerMeter, pbi.bV4YPelsPerMeter,
		pbi.bV4ClrUsed, pbi.bV4ClrImportant);

	prog++;
	hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
	// Compute the size of the entire file.  
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof(RGBQUAD) + pbi.bV4SizeImage); 
	hdr.bfReserved1 = 0; 
	hdr.bfReserved2 = 0; 

	// Compute the offset to the array of color indices.  
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof (RGBQUAD); 

	// Copy the BITMAPFILEHEADER into the .BMP file.  
	fwrite((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, fdump);

	// Copy the BITMAPINFOHEADER array into the file.  
	fwrite((LPVOID)&pbi, sizeof(BITMAPV4HEADER), 1, fdump);

	// Copy the RGBQUAD array into the file.  
	if(iUsage == DIB_RGB_COLORS){
		if(pbi.bV4ClrUsed) fwrite(&pbmi->bmiColors[0], pbi.bV4ClrUsed * sizeof (RGBQUAD), 1, fdump);
	}
	else{
		DWORD RGBColor;
		switch(pbi.bV4BitCount) {
			case 1: // black & white DIBs
				// simulates a 2 color palette WHITE, BLACK
				RGBColor = 0x00FFFFFF;
				fwrite(&RGBColor, sizeof(DWORD), 1, fdump);
				RGBColor = 0x00000000;
				fwrite(&RGBColor, sizeof(DWORD), 1, fdump);
				fwrite((BYTE *)pvBits, pbi.bV4SizeImage, 1, fdump);
				break;
			case 8:
				HPALETTE hpal;
				// hellish trick: to get the DC palette change it twice, but the replacement must be successful,
				// so you must use a valid palette handle to be replaced: GetStockObject(DEFAULT_PALETTE) is ok.
				hpal=(*pGDISelectPalette)(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), 0);
				if(hpal){
					PALETTEENTRY DCPaletteEntries[256];	
					int nEntries;
					(*pGDISelectPalette)(hdc, hpal, 0);
					nEntries=(*pGetPaletteEntries)(hpal, 0, 256, DCPaletteEntries);
					for (int i=0; i<(int)pbi.bV4ClrUsed; i++){
						PALETTEENTRY RGBPalEntry = DCPaletteEntries[i];
						RGBColor = RGBPalEntry.peBlue | (RGBPalEntry.peGreen << 8) | (RGBPalEntry.peRed << 16),
						fwrite(&RGBColor, sizeof(DWORD), 1, fdump);
					}
				}
				break;
			case 24:
				break;
			case 32:
				break;
			case 0:
				MessageBox(0, "NO pbi.bV4ClrUsed", "!", 0);
				break;
		}
	}

	// Copy the array of color indices into the .BMP file.  
	//for(int y=0; y<(int)ddsd.dwHeight; y++)
	//	fwrite((BYTE *)ddsd.lpSurface + (y*ddsd.lPitch), iScanLineSize, 1, fdump);
	fwrite((BYTE *)pvBits, pbi.bV4SizeImage, 1, fdump);

	// Close the .BMP file.  
	fclose(fdump);
}

void DumpHDC(HDC hDC, int x0, int y0, int w, int h)
{
	ApiName("DumpHDC");
	FILE *fdump;
	static int prog = 0;
	char FilePath[MAX_PATH];
	WORD BitsPerPixel = 24;

	w = (*pGDIGetDeviceCaps)(hDC, HORZSIZE);
	h = (*pGDIGetDeviceCaps)(hDC, VERTSIZE);
	BitsPerPixel = (*pGDIGetDeviceCaps)(hDC, BITSPIXEL);

	if ((w == 0) || (h == 0)) return;
	if ((w == 1) || (h == 1)) return;

	BITMAP structBitmapHeader;
	memset(&structBitmapHeader, 0, sizeof(BITMAP));

	HGDIOBJ hBitmap = GetCurrentObject(hDC, OBJ_BITMAP);
	if(!hBitmap) return;
	if(!GetObject(hBitmap, sizeof(BITMAP), &structBitmapHeader)) {
		OutTrace("%s: hdc=%#x has no bitmap err=%d\n", ApiRef, hDC, GetLastError());
		return;
	}

	DWORD Width = structBitmapHeader.bmWidth;
	DWORD Height = structBitmapHeader.bmHeight;

	if((Width == 0) || (Height == 0)) {
		OutTrace("%s: hdc=%#x has bad size=(%dx%d)\n", ApiRef, hDC, Width, Height);
		return;
	}

	BITMAPINFO Info;
	BITMAPFILEHEADER Header;
	memset(&Info, 0, sizeof(Info));
	memset(&Header, 0, sizeof(Header));
	Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	Info.bmiHeader.biWidth = Width;
	Info.bmiHeader.biHeight = Height;

	Info.bmiHeader.biPlanes = 1;
	Info.bmiHeader.biBitCount = BitsPerPixel;
	Info.bmiHeader.biCompression = BI_RGB;
	Info.bmiHeader.biSizeImage = Width * Height * (BitsPerPixel > 24 ? 4 : 3);

	Header.bfType = 0x4D42;
	Header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	char* Pixels = NULL;
	HDC MemDC = CreateCompatibleDC(hDC);
	HBITMAP Section = CreateDIBSection(hDC, &Info, DIB_RGB_COLORS, (void**)&Pixels, 0, 0);
	DeleteObject(SelectObject(MemDC, Section));
	(*pGDIBitBlt)(MemDC, 0, 0, Info.bmiHeader.biWidth, Info.bmiHeader.biHeight, hDC, 0, 0, SRCCOPY);
	DeleteDC(MemDC);

	if(prog==0) CreateDirectory(".\\bmp.out", NULL);
	sprintf(FilePath,".\\bmp.out\\hdc.%08.8d.bmp", prog);

 	fdump=fopen(FilePath, "wb");
	if(!fdump) {
		OutTrace("%s: err=%d\n", ApiRef, GetLastError());
        DeleteObject(Section);
		return;
	}

	OutTrace("%s: fname=%s hdc=%#x size(wxh)=(%dx%d) bpp=%d\n", 
		ApiRef, FilePath, hDC, Width, Height, BitsPerPixel);

	fwrite((char*)&Header, sizeof(Header), 1, fdump);
	fwrite((char*)&Info.bmiHeader, sizeof(Info.bmiHeader), 1, fdump);
	fwrite(Pixels, (((BitsPerPixel * Info.bmiHeader.biWidth + 31) & ~31) / 8) * Info.bmiHeader.biHeight, 1, fdump);
	fclose(fdump);
    DeleteObject(Section);
	prog++;
    return;
}

void DumpFullHDC(char *caption, HDC hDC)
{
	HBITMAP hBmp = (HBITMAP) GetCurrentObject(hDC, OBJ_BITMAP);
	BITMAP bmp;
	GetObjectA(hBmp, (int)&bmp, (LPVOID)sizeof(bmp));
	OutTrace("%s: ", caption); DumpHDC(hDC, 0, 0, bmp.bmWidth, bmp.bmHeight);
	DeleteObject(hBmp);
}

#endif // DXW_NOTRACES