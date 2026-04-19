#define _CRT_SECURE_NO_WARNINGS

#define STRETCHDRAWPIXELS 
#define STRETCHBITMAPS

//#define SCALEDUMP
//#define TRACEALL

#include "stdio.h"
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "gl\gl.h"
#include "gl\glext.h"
#include "dxdds.h"
#include "s3tc.h"
//#include "stb_dxt.h"
extern BOOL Bmp2TextureRGBA(FILE *, BYTE *);
extern BOOL BmpCheckSize(FILE *, int, int);

#ifndef COMPRESSED_RGB_S3TC_DXT1_EXT
#define COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3
#endif

enum
{
	//! Use DXT1 compression.
	kDxt1 = ( 1 << 0 ), 
	//! Use DXT3 compression.
	kDxt3 = ( 1 << 1 ), 
	//! Use DXT5 compression.
	kDxt5 = ( 1 << 2 ), 
	//! Use a very slow but very high quality colour compressor.
	kColourIterativeClusterFit = ( 1 << 8 ),	
	//! Use a slow but high quality colour compressor (the default).
	kColourClusterFit = ( 1 << 3 ),	
	//! Use a fast but low quality colour compressor.
	kColourRangeFit	= ( 1 << 4 ),
	//! Weight the colour by alpha during cluster fit (disabled by default).
	kWeightColourByAlpha = ( 1 << 7 ),
	//! Source is BGRA rather than RGBA
	kSourceBGRA = ( 1 << 9 ),
};

typedef int (WINAPI *GetStorageRequirements_Type)( int width, int height, int flags );
GetStorageRequirements_Type pGetStorageRequirements = 0;
typedef void (WINAPI *CompressImage_Type)(LPBYTE rgba, int width, int height, void *blocks, int flags, float *metrics);
CompressImage_Type pCompressImage = 0;
typedef void (WINAPI *DecompressImage_Type)(LPBYTE rgba, int width, int height, void const *blocks, int flags);
DecompressImage_Type pDecompressImage = 0;

static int mapCompressionType(int internalFormat)
{
	int format;
	switch(internalFormat){
		case COMPRESSED_RGBA_S3TC_DXT1_EXT: format = kDxt1; break;
		case COMPRESSED_RGBA_S3TC_DXT3_EXT: format = kDxt3; break;
		case COMPRESSED_RGBA_S3TC_DXT5_EXT: format = kDxt5; break;
	}
	return format;
}

typedef void (WINAPI *glGetTexImage_Type) (GLenum, GLint, GLenum, GLenum, GLvoid *);
static glGetTexImage_Type pglGetTexImage = NULL;
typedef void (WINAPI *glGetTexLevelParameteriv_Type)(GLenum, GLint, GLenum, GLint *);
static glGetTexLevelParameteriv_Type pglGetTexLevelParameteriv = NULL;

static unsigned int Hash(BYTE *buf, int len)
{
   unsigned int b    = 378551;
   unsigned int a    = 63689;
   DWORD hash = 0;
   for(int i = 0; i < len; i++){
      hash = hash * a + buf[i];
      a    = a * b;
   }
   return hash;
}

static BOOL SetColorMask(char *api, GLint internalFormat, GLenum Format, GLenum type, 
						 WORD *dwColor, DWORD *dwRMask, DWORD *dwGMask, DWORD *dwBMask, DWORD *dwAMask,
						 BOOL *couldModify)
{
	DWORD dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
	char *sType = "unknown";
	WORD dwRGBBitCount = 0;
	switch(internalFormat){
		case GL_LINE_STRIP: // used by "SimGolf", "Daikatana"
		case GL_TRIANGLES: // "Hexen 2 opengl"
		case GL_RGB: // from "Alone in the Dark the new nightmare" .....
		case GL_RGB8: // from "Doom 3"
		case GL_RGB16: 
		case GL_RGBA8: // from "GhostMaster", "Daikatana"
		case GL_RGBA: // ????
		case GL_LUMINANCE: // "Blak Stone"
		case GL_RGB5: // "The Banished"
		case GL_RGBA4: // "The Banished"
		case GL_INTENSITY: 
		case GL_INTENSITY4:
		case GL_INTENSITY8: // "Doom 3"
		case GL_INTENSITY12: 
		case GL_INTENSITY16:
			if(couldModify) *couldModify = TRUE;
			break;
		case COMPRESSED_RGB_S3TC_DXT1_EXT: // "Doom 3"
		case COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case COMPRESSED_RGBA_S3TC_DXT3_EXT: // "Doom 3" ???
		case COMPRESSED_RGBA_S3TC_DXT5_EXT: // "Doom 3" ??? bump mapping ???
			if(couldModify) *couldModify = FALSE;
			break;
		case GL_QUADS: 
		default: 
			OutTraceOGL("%s: unsupported int. format %#x\n", ApiRef, internalFormat);
			return FALSE; 
	}

	// format: GL_RED?, GL_GREEN?, GL_BLUE?, GL_RG?, GL_RGB?, GL_BGR?, GL_RGBA?, GL_BGRA
	// type: GL_UNSIGNED_BYTE?, GL_BYTE?, GL_UNSIGNED_SHORT?, GL_SHORT?, GL_UNSIGNED_INT?, GL_INT?, 
	// GL_FLOAT?, GL_UNSIGNED_BYTE_3_3_2?, GL_UNSIGNED_BYTE_2_3_3_REV?, GL_UNSIGNED_SHORT_5_6_5?, GL_UNSIGNED_SHORT_5_6_5_REV?,
	// GL_UNSIGNED_SHORT_4_4_4_4?, GL_UNSIGNED_SHORT_4_4_4_4_REV?, GL_UNSIGNED_SHORT_5_5_5_1?, GL_UNSIGNED_SHORT_1_5_5_5_REV?, 
	// GL_UNSIGNED_INT_8_8_8_8?, GL_UNSIGNED_INT_8_8_8_8_REV?, GL_UNSIGNED_INT_10_10_10_2?, GL_UNSIGNED_INT_2_10_10_10_REV
	switch(Format){
		case GL_RGB:
			switch(type){
				case GL_BYTE:
				case GL_UNSIGNED_BYTE: // Daikatana, Doom 3
					dwRBitMask = 0x000000FF;
					dwGBitMask = 0x0000FF00;
					dwBBitMask = 0x00FF0000;
					dwABitMask = 0x00000000;
					dwRGBBitCount = 24;
					sType = "RGB888";
					break;
			}
			break;
		case GL_RGBA:
			switch(type){
				case GL_BYTE:
				case GL_UNSIGNED_BYTE: // Daikatana, Doom 3, Hexen II (opengl)
					dwRBitMask = 0x000000FF;
					dwGBitMask = 0x0000FF00;
					dwBBitMask = 0x00FF0000;
					dwABitMask = 0xFF000000;
					dwRGBBitCount = 32;
					sType = "RGBA888";
					break;
			}
			break;
		case GL_BGR:
			switch(type){
				case GL_BYTE:
				case GL_UNSIGNED_BYTE: // ???
					dwRBitMask = 0x00FF0000;
					dwGBitMask = 0x0000FF00;
					dwBBitMask = 0x000000FF;
					dwABitMask = 0x00000000;
					dwRGBBitCount = 24;
					sType = "BGR888";
					break;
			}
			break;		
		case GL_BGRA:
			switch(type){
				case GL_BYTE:
				case GL_UNSIGNED_BYTE: // ???
					dwRBitMask = 0x00FF0000;
					dwGBitMask = 0x0000FF00;
					dwBBitMask = 0x000000FF;
					dwABitMask = 0xFF000000;
					dwRGBBitCount = 32;
					sType = "BGRA888";
					break;
			}
			break;
		case GL_RGB16:
			switch(type){
				case GL_BYTE:
				case GL_UNSIGNED_BYTE: // ???
					dwRBitMask = 0x0000F800;
					dwGBitMask = 0x000007E0;
					dwBBitMask = 0x0000001F;
					dwABitMask = 0x00000000;
					dwRGBBitCount = 16;
					sType = "RGB16";
					break;
			}
			break;
		case GL_RGBA8:
			switch(type){
				case GL_BYTE:
				case GL_UNSIGNED_BYTE: // ???
					dwRBitMask = 0x00000000;
					dwGBitMask = 0x00000000;
					dwBBitMask = 0x00000000;
					dwABitMask = 0x00000000;
					dwRGBBitCount = 8;
					sType = "RGBA8";
					break;
			}
			break;
		case GL_LUMINANCE:
			//switch(type){
			//	default:
					dwRBitMask = 0x0000FFFF;
					dwGBitMask = 0x0000FFFF;
					dwBBitMask = 0x0000FFFF;
					dwABitMask = 0x00000000;
					dwRGBBitCount = 16;
					sType = "LUMINANCE";
			//		break;
			//}
			break;
		default:
			OutTraceOGL("%s: unsupported format=%#x\n", ApiRef, Format);
			break;
	}

	if(!dwRGBBitCount) {
		OutTraceOGL("%s: unsupported 0 RGB count\n", ApiRef);
		return FALSE;
	}

	*dwColor = dwRGBBitCount;
	if(dwRMask) *dwRMask = dwRBitMask;
	if(dwGMask) *dwGMask = dwGBitMask;
	if(dwBMask) *dwBMask = dwBBitMask;
	if(dwAMask) *dwAMask = dwABitMask;
	OutTraceOGL("%s: format=%s bpp=%d RGBA=(%08.8X:%08.8X:%08.8X:%08.8X)\n", 
		ApiRef, sType, dwRGBBitCount, dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask);
	return TRUE;
}

// v2.05.61 fix:
// OpenGL textures can have variable formats and bit color position, so the BI_RGB compression type is not suitable for 
// several formats. Better use BI_BITFIELDS in bV4V4Compression so that the RGBA bitmask is considered.

static void glTextureDump(GLint internalFormat, GLenum Format, GLsizei w, GLsizei h, GLenum type, const GLvoid * data)
{
	ApiName("glTextureDump");
	int iSurfaceSize, iScanLineSize;
	char pszFile[MAX_PATH];
	WORD dwRGBBitCount;
	DWORD dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;

	OutTraceDW("%s: formats=%#x,%#x size=(%dx%d) type=%#x\n", ApiRef, internalFormat, Format, w, h, type);

	if(!dxw.VerifyTextureLimits(w, h)){
		OutTraceOGL("%s: SKIP texture\n", ApiRef);
		return;
	}

	if(!SetColorMask(ApiRef, internalFormat, Format, type, 
		&dwRGBBitCount, &dwRBitMask, &dwGBitMask, &dwBBitMask, &dwABitMask, NULL)) return;

	if(Format == GL_LUMINANCE) w = w / 2;

	iSurfaceSize = w * h * (dwRGBBitCount/8);

	while (TRUE) {
		FILE *hf;
		BITMAPFILEHEADER hdr;       // bitmap file-header 
		BITMAPV4HEADER pbi;			// bitmap info-header
		DWORD hash;

		memset((void *)&pbi, 0, sizeof(BITMAPV4HEADER));
		pbi.bV4Size = sizeof(BITMAPV4HEADER); 
		pbi.bV4Width = w;
		pbi.bV4Height = h;
		pbi.bV4BitCount = dwRGBBitCount;
		pbi.bV4SizeImage = ((w * dwRGBBitCount + 0x1F) & ~0x1F)/8 * h; 
		pbi.bV4Height = - h;
		pbi.bV4Planes = 1;
		pbi.bV4V4Compression = BI_BITFIELDS;
		//pbi.bV4V4Compression = BI_RGB; // more portable
		if(pbi.bV4BitCount == 8) pbi.bV4V4Compression = BI_RGB;
		pbi.bV4XPelsPerMeter = 1;
		pbi.bV4YPelsPerMeter = 1;
		pbi.bV4ClrUsed = 0;
		if(pbi.bV4BitCount == 8) pbi.bV4ClrUsed = 256;
		pbi.bV4ClrImportant = 0;
		pbi.bV4RedMask = dwRBitMask;
		pbi.bV4GreenMask = dwGBitMask;
		pbi.bV4BlueMask = dwBBitMask;
		pbi.bV4AlphaMask = dwABitMask;
		pbi.bV4CSType = LCS_CALIBRATED_RGB;
		iScanLineSize = ((w * dwRGBBitCount + 0x1F) & ~0x1F)/8; 

		// calculate the bitmap hash
		OutDebugOGL("%s: hash linesize=%d h=%d\n", ApiRef, iScanLineSize, h);
		__try {
			hash = Hash((BYTE *)data, iSurfaceSize);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			OutTraceE("%s: hash exception\n", ApiRef);
			return;
		}
		
		// Create the .BMP file. 
		sprintf_s(pszFile, MAX_PATH, "%s\\texture.out\\texture.I%x.F%x.T%x.%03d.%03d.%08X.bmp", 
			GetDxWndPath(), internalFormat, Format, type, w, h, hash);
		OutDebugOGL("%s: writing to %s\n", ApiRef, pszFile);
		hf = fopen(pszFile, "wb");
		if(!hf) break;

		hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
		// Compute the size of the entire file.  
		hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof(RGBQUAD) + pbi.bV4SizeImage); 
		hdr.bfReserved1 = 0; 
		hdr.bfReserved2 = 0; 

		// Compute the offset to the array of color indices.  
		hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof (RGBQUAD); 

		// Copy the BITMAPFILEHEADER into the .BMP file.  
		fwrite((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, hf);

		// Copy the BITMAPINFOHEADER array into the file.  
		fwrite((LPVOID)&pbi, sizeof(BITMAPV4HEADER), 1, hf);

		// Copy the array of color indices into the .BMP file.  
		__try{
			for(int y=0; y<h; y++)
				fwrite((BYTE *)data + (y*iScanLineSize), iScanLineSize, 1, hf);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){}

		//fwrite((BYTE *)data, iScanLineSize, h, hf);

		// Close the .BMP file.  
		fclose(hf);
		break;
	}
}

void glCompressedTextureDump(GLenum target, GLint internalFormat, GLsizei w, GLsizei h, GLsizei imageSize, const GLvoid *data)
{
	ApiName("glCompressedTextureDump");
	char pszFile[MAX_PATH];
	DWORD hash;
	FILE *hf;
	ULONG *decoded;
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	BITMAPV4HEADER pbi;			// bitmap info-header
	DWORD dwRGBBitCount = 32;

	OutTrace("%s: iformat=%#x size=(%dx%d) len=%d\n", ApiRef, internalFormat, w, h, imageSize);

	if(!dxw.VerifyTextureLimits(w, h)){
		OutTraceOGL("%s: SKIP texture\n", ApiRef);
		return;
	}

	__try {
		hash = Hash((BYTE *)data, imageSize);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTraceE("%s: hash exception\n", ApiRef);
		return;
	}

	switch(internalFormat){
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			sprintf_s(pszFile, MAX_PATH, "%s\\texture.out\\texture.I%x.%03d.%03d.%08X.bmp", 
				GetDxWndPath(), internalFormat,w, h, hash);
			OutTrace("%s: writing to %s\n", ApiRef, pszFile);
			hf = fopen(pszFile, "wb");
			if(!hf) {
				OutTrace("%s: fopen ERROR err=%d\n", ApiRef, GetLastError());
				return;
			}
			decoded = (ULONG *)malloc(h * w * sizeof(DWORD));
			(*pDecompressImage)((LPBYTE)decoded, w, h, (void *)data, mapCompressionType(internalFormat));
			memset((void *)&pbi, 0, sizeof(BITMAPV4HEADER));
			pbi.bV4Size = sizeof(BITMAPV4HEADER); 
			pbi.bV4Width = w;
			pbi.bV4Height = h;
			pbi.bV4BitCount = (WORD)dwRGBBitCount;
			pbi.bV4SizeImage = ((w * dwRGBBitCount + 0x1F) & ~0x1F)/8 * h; 
			pbi.bV4Height = - h;
			pbi.bV4Planes = 1;
			pbi.bV4V4Compression = BI_BITFIELDS;
			//pbi.bV4V4Compression = BI_RGB; // more portable
			if(pbi.bV4BitCount == 8) pbi.bV4V4Compression = BI_RGB;
			pbi.bV4XPelsPerMeter = 1;
			pbi.bV4YPelsPerMeter = 1;
			pbi.bV4ClrUsed = 0;
			if(pbi.bV4BitCount == 8) pbi.bV4ClrUsed = 256;
			pbi.bV4ClrImportant = 0;
			switch(dwRGBBitCount){
				case 16:
					//pbi.bV4RedMask = 0x0000F800;
					//pbi.bV4GreenMask = 0x000007E0;
					//pbi.bV4BlueMask = 0x0000001F;
					//pbi.bV4AlphaMask = 0x00000000;
					break;
				case 32:
					pbi.bV4RedMask =	0x000000FF;
					pbi.bV4GreenMask =	0x0000FF00;
					pbi.bV4BlueMask =	0x00FF0000;
					pbi.bV4AlphaMask =	0xFF000000;
					break;
			}
			pbi.bV4CSType = LCS_CALIBRATED_RGB;
			//iScanLineSize = ((w * dwRGBBitCount + 0x1F) & ~0x1F)/8; 

			hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
			// Compute the size of the entire file.  
			hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof(RGBQUAD) + pbi.bV4SizeImage); 
			hdr.bfReserved1 = 0; 
			hdr.bfReserved2 = 0; 

			// Compute the offset to the array of color indices.  
			hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbi.bV4Size + pbi.bV4ClrUsed * sizeof (RGBQUAD); 

			// Copy the BITMAPFILEHEADER into the .BMP file.  
			fwrite((LPVOID)&hdr, sizeof(BITMAPFILEHEADER), 1, hf);

			// Copy the BITMAPINFOHEADER array into the file.  
			fwrite((LPVOID)&pbi, sizeof(BITMAPV4HEADER), 1, hf);

			fwrite(decoded, h * w * sizeof(DWORD), 1, hf);
			free(decoded);
			break;

		default:
			// Create the .RAW file. 
			sprintf_s(pszFile, MAX_PATH, "%s\\texture.out\\texture.I%x.%03d.%03d.%08X.bin", 
				GetDxWndPath(), internalFormat,w, h, hash);
			OutTrace("%s: writing to %s\n", ApiRef, pszFile);
			hf = fopen(pszFile, "wb");
			if(!hf) {
				OutTrace("%s: fopen ERROR err=%d\n", ApiRef, GetLastError());
				return;
			}
			fwrite(data, imageSize, 1, hf);
			break;
	}

	// Close the .BMP file.  
	fclose(hf);
}

static void glTextureHack(GLint internalFormat, GLenum Format, GLsizei w, GLsizei h, GLenum type, const GLvoid *data)
{
	ApiName("glTextureHack");
	int iSurfaceSize, iScanLineSize;
	char pszFile[MAX_PATH];
	WORD dwRGBBitCount;
	DWORD dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
	char *sExt;
	FILE *hf;
#if 0
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	BITMAPV4HEADER pbi;		// bitmap info-header  
	int iSizeImage;
#endif
	int rlen;
	BOOL canUpdate;

	OutTraceDW("%s: formats=%#x,%#x size=(%dx%d) type=%#x\n", ApiRef, internalFormat, Format, w, h, type);

	if(!dxw.VerifyTextureLimits(w, h)){
		OutTraceOGL("%s: SKIP texture\n", ApiRef);
		return;
	}

	if(!SetColorMask(ApiRef, internalFormat, Format, type, &dwRGBBitCount, &dwRBitMask, &dwGBitMask, &dwBBitMask, &dwABitMask, &canUpdate)) return;
	if(!canUpdate) return;

	// calculate the bitmap hash
	iSurfaceSize = w * h * (dwRGBBitCount/8);
	iScanLineSize = ((w * dwRGBBitCount + 0x1F) & ~0x1F)/8; 
	DWORD hash;
	OutDebugOGL("%s: hash linesize=%d h=%d\n", ApiRef, iScanLineSize, h);
	__try {
		hash = Hash((BYTE *)data, iSurfaceSize);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTraceE("%s: hash exception\n", ApiRef);
		return;
	}
	if(!hash) return; // almost certainly, an empty black surface!

	// get the filename. 
	switch (dxw.iTextureFileFormat){
		case FORMAT_BMP: sExt = "bmp"; break; 
		case FORMAT_RAW: sExt = "raw"; break; 
		case FORMAT_DDS: sExt = "dds"; break; 
	}
	sprintf_s(pszFile, MAX_PATH, "%s\\texture.in\\texture.I%x.F%x.T%x.%03d.%03d.%08X.%s", 
		GetDxWndPath(), internalFormat, Format, type, w, h, hash, sExt);

	hf = fopen(pszFile, "rb");
	if(!hf) return; // no updated texture to load

	OutTraceDW("%s: IMPORT path=%s\n", ApiRef, pszFile);

	switch(dxw.iTextureFileFormat){

		case FORMAT_RAW: {

			rlen = fread((BYTE *)data, h * iScanLineSize, 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			}
			break;

		case FORMAT_DDS: {

			BYTE magic[4];
			DDS_HEADER ddsh;
			// assume the file is sane, read and throw away magic and dds header
			rlen = fread(magic, 4, 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			rlen = fread((BYTE *)&ddsh, sizeof(ddsh), 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			memset(&ddsh, 0, sizeof(ddsh));
			rlen = fread((BYTE *)data, h * iScanLineSize, 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
		}
		break;

		case FORMAT_BMP: 

			if (!BmpCheckSize(hf, w, h)) {
				OutTraceE("%s: bitmap size check failed\n", ApiRef);
				break;
			}
			if(!Bmp2TextureRGBA(hf, (BYTE *)data)) break;

		break;
	}
	// Close the .BMP file.  
	fclose(hf);
}

static void glCompressedTextureHack(GLint internalFormat, GLsizei w, GLsizei h, GLsizei imageSize, const GLvoid *data)
{
	ApiName("glCompressedTextureHack");
	char pszFile[MAX_PATH];
	char *sExt;
	FILE *hf;
#if 0
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	BITMAPV4HEADER pbi;		// bitmap info-header  
#endif
	int pixelsSize;
	unsigned char *pixelsBuf;
	int rlen;
	int dwRGBBitCount = 32;
	int iScanLineSize = w * sizeof(DWORD);

	OutTraceDW("%s: iformat=%#x size=(%dx%d)\n", ApiRef, internalFormat, w, h);

	if(!dxw.VerifyTextureLimits(w, h)){
		OutTraceOGL("%s: SKIP texture\n", ApiRef);
		return;
	}

	switch(internalFormat){
		case COMPRESSED_RGBA_S3TC_DXT1_EXT: 
		case COMPRESSED_RGBA_S3TC_DXT3_EXT: 
		case COMPRESSED_RGBA_S3TC_DXT5_EXT: 
			break;
		default:
			OutTrace("%s: unsupported compressed internal format\n", ApiRef);
			return;
	}

	// calculate the bitmap hash
	DWORD hash;
	__try {
		hash = Hash((BYTE *)data, imageSize);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		OutTraceE("%s: hash exception\n", ApiRef);
		return;
	}
	if(!hash) return; // almost certainly, an empty black surface!
	pixelsSize = w * h * sizeof(DWORD);
	pixelsBuf = (unsigned char *)malloc(pixelsSize);

	// get the filename. 
	switch (dxw.iTextureFileFormat){
		case FORMAT_BMP: sExt = "bmp"; break; 
		case FORMAT_RAW: sExt = "raw"; break; 
		case FORMAT_DDS: sExt = "dds"; break; 
	}
	sprintf_s(pszFile, MAX_PATH, "%s\\texture.in\\texture.I%x.%03d.%03d.%08X.%s", 
		GetDxWndPath(), internalFormat, w, h, hash, sExt);

	hf = fopen(pszFile, "rb");
	if(!hf) return; // no updated texture to load

	OutTraceDW("%s: IMPORT path=%s\n", ApiRef, pszFile);

	switch(dxw.iTextureFileFormat){

		case FORMAT_RAW: 
			rlen = fread((BYTE *)data, h * iScanLineSize, 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			break;

		case FORMAT_DDS: {

			BYTE magic[4];
			DDS_HEADER ddsh;
			// assume the file is sane, read and throw away magic and dds header
			rlen = fread(magic, 4, 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			rlen = fread((BYTE *)&ddsh, sizeof(ddsh), 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			memset(&ddsh, 0, sizeof(ddsh));
			rlen = fread((BYTE *)data, h * iScanLineSize, 1, hf);
			_if(rlen != 1) OutTraceE("%s: fread ERROR err=%d\n", ApiRef, GetLastError());
			}
		break;

		case FORMAT_BMP: 

			if (!BmpCheckSize(hf, w, h)) {
				OutTraceE("%s: bitmap size check failed\n", ApiRef);
				break;
			}
			if(!Bmp2TextureRGBA(hf, pixelsBuf)) {
				OutTraceE("%s: texture processing failed\n", ApiRef);
				break;
			}

	}	
	fclose(hf);

	OutTraceDW("%s: TEXTURE LOAD DONE\n", ApiRef);
	(*pCompressImage)(pixelsBuf, w, h, (void *)data, mapCompressionType(internalFormat), NULL);
	free(pixelsBuf);
}

#define GRIDSIZE 16

static void glTextureHighlight(GLint internalFormat, GLenum Format, GLsizei w, GLsizei h, GLenum type, const GLvoid * data)
{
	ApiName("glTextureHighlight");
	WORD dwRGBBitCount;
	DWORD dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
	BOOL canUpdate;

	OutTraceDW("%s: formats=%#x,%#x size=(%dx%d) type=%#x\n", ApiRef, internalFormat, Format, w, h, type);

	if(!dxw.VerifyTextureLimits(w, h)){
		OutTraceOGL("%s: SKIP texture\n", ApiRef);
		return;
	}

	if(!SetColorMask(ApiRef, internalFormat, Format, type, &dwRGBBitCount, 
		&dwRBitMask, &dwGBitMask, &dwBBitMask, &dwABitMask, &canUpdate)) return;
	if(!canUpdate) return;

	int x, y;
	switch (dwRGBBitCount){
		case 8:
			{ 
				BYTE *p;
				BYTE color;
				color=(BYTE)(rand() & 0xFF);
				for(y=0; y<h; y++){
					p = (BYTE *)data + (y * w);
					for(x=0; x<w; x++) *(p+x) = color;
				}
				for(y=0; y<h; y++){
					p = (BYTE *)data + (y * w);
					for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
					if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
				}
			}
			break;
		case 16: 
			{
				SHORT *p;
				SHORT color;
				color=(SHORT)(rand() & (short)~dwABitMask);
				for(y=0; y<h; y++){
					p = (SHORT *)data + (y * w);
					for(x=0; x<w; x++) *(p+x) = color | (*(p+x)&dwABitMask);
				}
				for(y=0; y<h; y++){
					p = (SHORT *)data + (y * w);
					for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
					if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
				}
			}
			break;
		case 24: 
			{
				BYTE *p;
				DWORD color;
				color=(DWORD)(rand() & ~dwABitMask);
				for(y=0; y<h; y++){
					p = (BYTE *)data + (3 * y * w);
					for(x=0; x<w; x++) {
						*(p+x) = color & 0xFF;
						*(p+x+1) = (color >> 8) & 0xFF;
						*(p+x+2) = (color >> 16) & 0xFF;
					}
				}
				//for(y=0; y<h; y++){
				//	p = (SHORT *)data + (y * w);
				//	for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
				//	if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
				//}
			}
			break;
		case 32: 
			{
				DWORD *p;
				DWORD color;
				color=(DWORD)(rand() & ~dwABitMask);
				for(y=0; y<h; y++){
					p = (DWORD *)data + (y * w);
					for(x=0; x<w; x++) *(p+x) = color | (*(p+x)&dwABitMask);
				}
				for(y=0; y<h; y++){
					p = (DWORD *)data + (y * w);
					for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
					if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
				}
			}
			break;
		default:
		OutTraceOGL("%s: SKIP texture RGBBitCount=%d\n", ApiRef, dwRGBBitCount);
		break;
	}
}

void glCompressedTextureHighlight(GLint internalFormat, GLsizei w, GLsizei h, GLsizei imageSize, const GLvoid *data)
{
	ApiName("glCompressedTextureHighlight");
	DWORD *p;
	DWORD color;
	LPBYTE pixelsBuf;
	int x, y;
	DWORD dwABitMask = 0xFF000000;

	OutTraceDW("%s: iformat=%#x size=(%dx%d)\n", ApiRef, internalFormat, w, h);

	if(!dxw.VerifyTextureLimits(w, h)){
		OutTraceOGL("%s: SKIP texture\n", ApiRef);
		return;
	}

	color=(DWORD)(rand() & ~dwABitMask);
	pixelsBuf = (LPBYTE)malloc(w * h * sizeof(DWORD));
	for(y=0; y<h; y++){
		p = (DWORD *)pixelsBuf + (y * w);
		for(x=0; x<w; x++) *(p+x) = color | (*(p+x)&dwABitMask);
	}
	for(y=0; y<h; y++){
		p = (DWORD *)pixelsBuf + (y * w);
		for(x=0; x<w; x+=GRIDSIZE) *(p+x) = 0;
		if((y%GRIDSIZE)==0) for(x=0; x<w; x++) *(p++) = 0;
	}
	
	(*pCompressImage)(pixelsBuf, w, h, (LPVOID)data, mapCompressionType(internalFormat), NULL);
	free(pixelsBuf);
}

static void glTextureTransp(GLint internalFormat, GLenum Format, GLsizei w, GLsizei h, GLenum type, const GLvoid * data)
{
	ApiName("glTextureTransp");
	WORD dwRGBBitCount;
	DWORD dwABitMask;
	BOOL canUpdate;

	OutTraceDW("%s: formats=%#x,%#x size=(%dx%d) type=%#x\n", ApiRef, internalFormat, Format, w, h, type);

	if(!dxw.VerifyTextureLimits(w, h)){
		OutTraceOGL("%s: SKIP texture\n", ApiRef);
		return;
	}

	if(!SetColorMask(ApiRef, internalFormat, Format, type, &dwRGBBitCount, 0, 0, 0, &dwABitMask, &canUpdate)) {
		OutTraceOGL("%s: SKIP 0 RGB count\n", ApiRef);
		return;
	}
	if(!canUpdate) return;

	if(dwABitMask != 0xFF000000) {
		OutTraceOGL("%s: SKIP non 8bit alpha texture\n", ApiRef);
		return;
	}

	DWORD *p;
	DWORD alpha;
	alpha=0x7FFFFFFF;
	//alpha=0x0FFFFFFF;
	p = (DWORD *)data;
	for(int y=0; y<h; y++){
		for(int x=0; x<w; x++){
			*p++ = *p & alpha;
		}
	}
}

#define TRANSPALPHA (SHORT)0x1111
static void glCompressedTextureTransp(GLint internalFormat, GLsizei width, GLsizei height, GLsizei imageSize, const GLvoid *data)
{
	ApiName("glCompressedTextureTransp");
	//SHORT color=(SHORT)rand() & 0xFFFF;
	SHORT *p = (SHORT *)data;
	switch(internalFormat){
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			//for(int i=0; i<imageSize; i+=8){
			//	*p++;
			//	*p++;
			//	*p++ = TRANSPALPHA;
			//	*p++ = TRANSPALPHA;
			//}
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			for(int i=0; i<imageSize; i+=16){
				*p++ = TRANSPALPHA;
				*p++ = TRANSPALPHA;
				*p++ = TRANSPALPHA;
				*p++ = TRANSPALPHA;
				*p++;
				*p++;
				*p++;
				*p++;
			}
			break;
		default:
			//for(int i=0; i<imageSize; i+=4){
			//	*p++ = color;
			//	*p++ = color;
			//}
			//memset((LPVOID)data, color, imageSize);
			break;
	}
}

void glHandleTexture(GLenum target, GLint internalFormat, GLenum format, GLsizei width, GLsizei height, GLenum type, const GLvoid *data)
{
	switch(target){
		//case GL_PROXY_TEXTURE_RECTANGLE:
		//case GL_PROXY_TEXTURE_2D:
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
			switch(dxw.dwFlags5 & TEXTUREMASK){
				case TEXTUREHIGHLIGHT: 
					glTextureHighlight(internalFormat, format, width, height, type, data);
					break;
				case TEXTUREDUMP: 
					glTextureDump(internalFormat, format, width, height, type, data);
					break;
				case TEXTUREHACK:
					glTextureHack(internalFormat, format, width, height, type, data);
					break;
				case TEXTURETRANSP:
					//glTextureTransp(internalFormat, format, width, height, type, data);
					break;
			}
			break;
	}
}

void glHandleCompressedTexture(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLsizei imageSize, const GLvoid *data)
{
	if(!pCompressImage){
		HMODULE h;
		char sSourcePath[MAX_PATH+1];
		sprintf_s(sSourcePath, MAX_PATH, "%s\\libsquish.dll", GetDxWndPath());
		h = (*pLoadLibraryA)(sSourcePath);
		if(!h) return;
		pCompressImage = (CompressImage_Type)(*pGetProcAddress)(h, "CompressImage");
		pDecompressImage = (DecompressImage_Type)(*pGetProcAddress)(h, "DecompressImage");
		pGetStorageRequirements = (GetStorageRequirements_Type)(*pGetProcAddress)(h, "GetStorageRequirements");
		if(!(pCompressImage && pDecompressImage && pGetStorageRequirements)) {
			MessageBox(0, "ERROR: libsquish not found", "DxWnd", 0);
			return;
		}
	}
	switch(target){
		//case GL_PROXY_TEXTURE_RECTANGLE:
		//case GL_PROXY_TEXTURE_2D:
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
			switch(dxw.dwFlags5 & TEXTUREMASK){
				case TEXTUREHIGHLIGHT: 
					glCompressedTextureHighlight(internalFormat, width, height, imageSize, data);
					break;
				case TEXTUREDUMP: 
					glCompressedTextureDump(target, internalFormat, width, height, imageSize, data);
					break;
				case TEXTUREHACK:
					glCompressedTextureHack(internalFormat, width, height, imageSize, data);
					break;
				case TEXTURETRANSP:
					glCompressedTextureTransp(internalFormat, width, height, imageSize, data);
					break;
			}
			break;
	}
}
