#define _CRT_SECURE_NO_WARNINGS
#include "windows.h"
#include "stdio.h"
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "dxwlocale.h"

#define BUFSIZE 10

static BOOL bNakedDump = TRUE;
static BOOL bTimedDump = TRUE;

#define timeThreshold 1000
#define MAXTEXTSIZE 1024

typedef struct {
	DWORD time;
	DWORD x;
	DWORD y;
	char *text;
} textDump;

static char *Push(char *text)
{
	static textDump *lastLines = NULL;
	static int lastIndex = 0;
	DWORD now = GetTickCount();
	if (!lastLines) {
		lastLines = (textDump *)malloc(BUFSIZE * sizeof(textDump));
		for(int i=0; i<BUFSIZE; i++) {
			lastLines[i].time = 0;
			lastLines[i].x = 0;
			lastLines[i].y = 0;
			lastLines[i].text = NULL;
		}
		char sInitPath[MAX_PATH+1];
		sprintf(sInitPath, "%sdxwnd.ini", GetDxWndPath()); 
		bNakedDump = GetPrivateProfileInt("window", "nakeddump", 0, sInitPath);
		bTimedDump = GetPrivateProfileInt("window", "timeddump", 0, sInitPath);
	}
	if(bTimedDump){ // t.b.d. how to flush the last line???
		int len;
		if(lastLines[lastIndex].text) len = strlen(lastLines[lastIndex].text);
		else len = 0;
		int targetLen = len + strlen(text) + 1;
		if(((now - lastLines[lastIndex].time) < timeThreshold) && (targetLen < MAXTEXTSIZE)){
			// append
			char *newLine = (char *)malloc(targetLen);
			if (len) strcpy(newLine, lastLines[lastIndex].text);
			else lastLines[lastIndex].text[0] = 0;
			strcat(newLine, text);
			lastLines[lastIndex].text = newLine;
			lastLines[lastIndex].time = now;
			OutTrace("Append: %.80s\n", newLine);
			return NULL;
		}
		else { 
			char *newLine = lastLines[lastIndex].text;
			for(int i=0; i<BUFSIZE; i++)
				if((i != lastIndex) && lastLines[i].text && !strcmp(lastLines[i].text, newLine)) {
					OutTrace("Skip: %.80s\n", newLine);
					if(newLine) free(newLine);
					lastLines[lastIndex].text = NULL;
					lastLines[i].time = now;
					return NULL;
			}
			lastIndex = (lastIndex + 1) % BUFSIZE; // round robin
			if(lastLines[lastIndex].text) free(lastLines[lastIndex].text);
			lastLines[lastIndex].time = now;
			lastLines[lastIndex].text = (char *)malloc((strlen(text)+1) * sizeof(char));
			strcpy(lastLines[lastIndex].text, text);
			OutTrace("Flush: %.80s\n", newLine);
			return newLine;
		}
	}
	else {
		for(int i=0; i<BUFSIZE; i++)
			if(lastLines[i].text && !strcmp(lastLines[i].text, text)) return NULL;
		lastIndex = (lastIndex + 1) % BUFSIZE; // round robin
		if(lastLines[lastIndex].text) free(lastLines[lastIndex].text);
		lastLines[lastIndex].time = now;
		lastLines[lastIndex].text = (char *)malloc((strlen(text)+1) * sizeof(char));
		strcpy(lastLines[lastIndex].text, text);
		return text;
	}
}

void DumpTextA(char *api, int x, int y, LPCSTR text, int c)
{
	char *dump;
	if(text == NULL) return;
	__try {
		int size;
		if(c && (c != -1)) size = c;
		else size = lstrlenA(text); 
		LPSTR str = (LPSTR)malloc((size + 1) * sizeof(char)); // TODO: support UTF-8 3bytes ??? 
		if (str) {
			memcpy(str, text, size);
			str[size] = '\0'; // make tail ! 
		}
		for(char *p=str; *p; p++) if((*p == 0x0D) || (*p == 0x0A)) *p = ' ';
		if(dump=Push(str)){
			FILE *fp = fopen(".\\dxwnd.txt", "a");
			if (fp==NULL) {
				free(str);
				return;
			}
			if(bNakedDump)
				fprintf(fp, "%s\n", dump);
			else
				fprintf(fp, "@%d,%d %s: \"%s\"\n", x, y, api, dump);
			fflush(fp);
			fclose(fp);
		}
		free(str);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		FILE *fp = fopen(".\\dxwnd.txt", "a");
		fprintf(fp, "@%d,%d %s: exception @len=%d text=%#x\n", x, y, api, c, text);
		fclose(fp);
	}
}

void DumpTextW(char *api, int x, int y, LPCWSTR text, int c)
{
	char *dump;
	if(text == NULL) return;
	__try {
		int size;
		if(c && (c != -1)) size = c;
		else size = lstrlenW(text); 
		LPSTR str = (LPSTR)malloc((size + 1) << 1); // TODO: support UTF-8 3bytes ??? 
		if (str) {
			int n = (*pWideCharToMultiByte)(dxw.CodePage, 0, text, size, str, size << 1, NULL, NULL);
			str[n] = '\0'; // make tail ! 
		}
		for(char *p=str; *p; p++) if((*p == 0x0D) || (*p == 0x0A)) *p = ' ';
		if(dump=Push(str)){
			FILE *fp = fopen(".\\dxwnd.txt", "a");
			if (fp==NULL) {
				free(str);
				return;
			}
			if(bNakedDump)
				fprintf(fp, "%s\n", dump);
			else
				fprintf(fp, "@%d,%d %s: \"%s\"\n", x, y, api, dump);
			fflush(fp);
			fclose(fp);
		}
		free(str);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		FILE *fp = fopen(".\\dxwnd.txt", "a");
		fprintf(fp, "@%d,%d %s: exception @len=%d text=%#x\n", x, y, api, c, text);
		fclose(fp);
	}
}
