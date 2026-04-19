#include "dxwnd.h"
#include "dxwcore.hpp"
#include "stdio.h"

BOOL BmpCheckSize(FILE *fp, int w, int h)
{
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	BITMAPV4HEADER pbi;			// bitmap info-header  

	if(fseek(fp, 0, SEEK_SET) != 0) return FALSE;

	// Read the BITMAPFILEHEADER from the .BMP file (and throw away ...).  
	if(fread((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, fp) != 1)return FALSE;

	// Read the BITMAPINFOHEADER (and throw away ...).  
	// If the file contains BITMAPV4HEADER or BITMAPV5HEADER, no problem: next fseek will settle things
	if(fread((LPVOID)&pbi, sizeof(BITMAPINFOHEADER), 1, fp) != 1) return FALSE;

	// check the bitmap size
	OutTrace("wxh=%dx%d bmp wxh=%dx%d\n", w, h, pbi.bV4Width, pbi.bV4Height);
	if(pbi.bV4Width != w) return FALSE;
	if((pbi.bV4Height < 0) && (pbi.bV4Height != -h)) return FALSE;
	if((pbi.bV4Height > 0) && (pbi.bV4Height !=  h)) return FALSE;
	OutTrace("ret=TRUE\n");
	return TRUE;
}

BOOL Bmp2TextureRGBA(FILE *fp, BYTE *pixelsBuf)
{
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	BITMAPV4HEADER pbi;			// bitmap info-header  
	int iLineSize, iPixelsSize;
	int w, h;
	BOOL ret = FALSE;

	while(TRUE){ // fake loop
		if(fseek(fp, 0, SEEK_SET) != 0) break;

		// Read the BITMAPFILEHEADER from the .BMP file (and throw away ...).  
		if(fread((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, fp) != 1)break;

		// Read the BITMAPINFOHEADER (and throw away ...).  
		// If the file contains BITMAPV4HEADER or BITMAPV5HEADER, no problem: next fseek will settle things
		if(fread((LPVOID)&pbi, sizeof(BITMAPINFOHEADER), 1, fp) != 1) break;
		if((pbi.bV4BitCount != 32) && (pbi.bV4BitCount != 24)) break;

		// if the bitmap has a BITMAPV4HEADER or greater, read the masks
		if(pbi.bV4Size >= sizeof(BITMAPV4HEADER)){
			if(fread(&(pbi.bV4RedMask), sizeof(DWORD), 3, fp) != 3) break;
		}

		// skip the RGBQUAD array and go straight to the pixels
		fseek(fp, hdr.bfOffBits, SEEK_SET);

		// allocate space 
		w = pbi.bV4Width;
		h = abs(pbi.bV4Height);
		iLineSize = w * sizeof(DWORD);
		iPixelsSize = h * iLineSize;

		// Read the new texture  from the .BMP file. 
		if(pbi.bV4BitCount == 24) {
			int iScanLineSize = ((w * pbi.bV4BitCount + 0x1F) & ~0x1F)/8;
			BYTE *line = (BYTE *)malloc(iScanLineSize);
			BYTE bkColor;
			DWORD transparentColor = 0;
			if(pbi.bV4Height < 0){
				// biHeight < 0 -> scan lines from top to bottom, same as surface/texture convention
				for(int y=0; y<h; y++){
					BYTE *p = pixelsBuf + (y * iLineSize);
					fseek(fp, hdr.bfOffBits + (iScanLineSize * y), SEEK_SET);
					if(fread((LPVOID)line, iScanLineSize, 1, fp) != 1) break;
					for(int x=0; x<w; x++) {
						memcpy(p, &line[x * 3], 3);
						bkColor = memcmp(p, &transparentColor, 3) ? 0x00 : 0xFF;
						p+=3;
						*p++ = bkColor; // alpha
					}
				}
			}
			else {
				// biHeight > 0 -> scan lines from bottom to top, inverse order as surface/texture convention
				for(int y=0; y<h; y++){
					BYTE *p = pixelsBuf + (((h-1) - y) * iLineSize);
					fseek(fp, hdr.bfOffBits + (iScanLineSize * y), SEEK_SET);
					if(fread((LPVOID)line, iScanLineSize, 1, fp) != 1) break;
					for(int x=0; x<w; x++) {
						memcpy(p, &line[x * 3], 3);
						bkColor = memcmp(p, &transparentColor, 3) ? 0x00 : 0xFF;
						p+=3;
						*p++ = bkColor; // alpha
					}
				}
			}
		}
		else {
			if(pbi.bV4Height < 0){
				// biHeight < 0 -> scan lines from top to bottom, same as surface/texture convention
				for(int y=0; y<h; y++){
					BYTE *p = pixelsBuf + (y * iLineSize);
					fseek(fp, hdr.bfOffBits + (iLineSize * y), SEEK_SET);
					if(fread((LPVOID)p, iLineSize, 1, fp) != 1) break;
				}
			}
			else {
				// biHeight > 0 -> scan lines from bottom to top, inverse order as surface/texture convention
				for(int y=0; y<h; y++){
					BYTE *p = pixelsBuf + (((h-1) - y) * iLineSize);
					fseek(fp, hdr.bfOffBits + (iLineSize * y), SEEK_SET);
					if(fread((LPVOID)p, iLineSize, 1, fp) != 1) break;
				}
			}
		}

		// pixel shifting here
		DWORD *pw = (DWORD *)pixelsBuf;
		BYTE *pb = pixelsBuf;
		int numPixels = w * h;
		switch(pbi.bV4V4Compression){
			case BI_RGB:
				for(int i=0; i<numPixels; i++, pw++, pb+=sizeof(DWORD)){
					*pw = (*pb << 16) | (*(pb+1) << 8) | (*(pb+2) << 0) | (*(pb+3) << 24);
				}
				break;
			case BI_BITFIELDS:
				switch(pbi.bV4RedMask){ 
					case 0x00FF0000:
						// pb+0 = B; pb+1 = G; pb+2 = R; pb+3 = A
						for(int i=0; i<numPixels; i++, pw++, pb+=sizeof(DWORD)){ // tested OK
							*pw = (*pb << 16) | (*(pb+1) << 8) | (*(pb+2) << 0) | (*(pb+3) << 24);
							
						}
						break;
					case 0x000000FF:
					default:
						for(int i=0; i<numPixels; i++, pw++, pb+=sizeof(DWORD)){ // tested OK
							*pw = (*pb << 0) | (*(pb+1) << 8) | (*(pb+2) << 16) | (*(pb+3) << 24);
						}
						break;
				break;
				}
		}
		ret = TRUE;
		break;
	}

	return ret;
}
