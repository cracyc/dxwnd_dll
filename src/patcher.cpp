#define  _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "dxwnd.h"
#include "dxwcore.hpp"

#define MAXLINESZ 256
#define DEBUGTRACE

typedef struct {
	DWORD address;
	int length;
	BYTE *value;
	LPVOID next;
} lockerType;

lockerType *firstLocker = NULL;

DWORD getHexDWord(char *s)
{
	DWORD val = 0;
	for(int i=0; i<8; i++){
		val <<= 4;
		char c = s[i];
		if((c>='0') && (c<='9')) val += c - '0';
		else
		if((c>='A') && (c<='F')) val += c - 'A' + 0x0A;
		else
		if((c>='a') && (c<='f')) val += c - 'a' + 0x0A;
		else {
			val >>= 4;
			break;
		}
	}
	return val;
}

BYTE getHexByte(char *s)
{
	BYTE val = 0;
	for(int i=0; i<2; i++){
		val <<= 4;
		char c = s[i];
		if((c>='0') && (c<='9')) val += c - '0';
		else
		if((c>='A') && (c<='F')) val += c - 'A' + 0x0A;
		else
		if((c>='a') && (c<='f')) val += c - 'a' + 0x0A;
		else {
			val >>= 4;
			break;
		}
	}
	return val;
}

void addLocker(DWORD address, char *buffer)
{
	char *hex;
	int length = 0;
	lockerType *locker = (lockerType *)malloc(sizeof(lockerType));
	hex = buffer;
	// calculate length
	while((*hex) && (*(hex+1))){
		length++;
		hex += 2;
	}
	//OutTrace("! length=%d\n", length);
	if(length == 0) return;
	BYTE *value = (LPBYTE)malloc(length);
	if(!value) {
		OutTrace("! malloc(%d) failed\n", length);
		return;
	}
	hex = buffer;
	LPBYTE target = value;
	while((*hex) && (*(hex+1))){
		BYTE val = getHexByte(hex);
		*target++ = val;
		hex += 2;
#ifdef DEBUGTRACE
		//OutTrace("! val=%#x\n", val);
#endif

	}
	locker->address = address;
	locker->length = length;
	locker->value = value;
	locker->next = NULL;
	if(firstLocker == NULL){
		firstLocker = locker;
	}
	else {
		lockerType *cur = firstLocker;
		while(cur->next) cur = (lockerType *)(cur->next);
		cur->next = locker;
	}
	OutTrace("adding locker @%#x len=%d val=%s\n", 
		locker->address, locker->length, hexdump(locker->value, locker->length));
}

void patch(DWORD address, char *hex)
{
	BYTE *target = (BYTE *)address;
	while((*hex) && (*(hex+1))){
		BYTE val = getHexByte(hex);
		*target++ = val;
		hex += 2;
	}
}

DWORD WINAPI doLocker(LPVOID lpParameter)
{
	#define DXWREFRESHINTERVAL 500
	while(TRUE){
		Sleep(DXWREFRESHINTERVAL);
		lockerType *cur = firstLocker;
		while(cur) {
#ifdef DEBUGTRACE
			OutTrace("! lock @%#x len=%d val=%s\n", cur->address, cur->length, hexdump(cur->value, cur->length));
#endif
			__try {
				memcpy((LPVOID)(cur->address), cur->value, cur->length);
			}
			__except(EXCEPTION_EXECUTE_HANDLER){
				// do nothing
#ifdef DEBUGTRACE
				OutTrace("! exception @%#x\n", cur->address);
#endif
			}
			cur = (lockerType *)(cur->next);
		}
	}
    return 0;
}

void patcher()
{
	static boolean doOnce = FALSE;
	if(doOnce) return;
	doOnce = TRUE;
	char line[MAXLINESZ+1];
	int len;DWORD address;
	char *patchBuf;
	FILE *fin = fopen(".\\dxwnd.patch.txt", "r");
	if(fin == NULL) return;
	while(fgets(line, MAXLINESZ, fin) != NULL){
		len=strlen(line);
		line[len-1]=0; // strip the newline terminator
		if(len < 1) continue;
		switch(line[0]){
			case '@': 
				address = getHexDWord(&line[1]);
				patchBuf = strchr(line, ':') + 1;
				OutTrace("patcher: @%#x:%s\n", address, patchBuf);
				patch(address, patchBuf);
				break;
			case '!':
				address = getHexDWord(&line[1]);
				patchBuf = strchr(line, ':') + 1;
				OutTrace("patcher: !%#x:%s\n", address, patchBuf);
				addLocker(address, patchBuf);
				break;
			default:
				break;
		}
	}

	if(firstLocker){
		lockerType *cur = firstLocker;
		while(cur){
			OutTrace("! locker @%#x len=%d next=%#x\n", cur->address, cur->length, cur->next);
			cur = (lockerType *)(cur->next);
		}
		CreateThread(NULL, 0, doLocker, NULL, 0, NULL);
	}
}