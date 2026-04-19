#define _CRT_SECURE_NO_WARNINGS

#include "windows.h"
#include "stdio.h"
#include "dxwnd.h"
#include "dxwcore.hpp"

void DumpMidiMessage(LPBYTE lpData, int Length)
{
	int len = (Length == 0) ? 3 : Length;
	int seq = 1;
	char subString[80+1];
	for (int i = 0; i < len; ) {
		BYTE command = lpData[i] & 0xF0;
		BYTE channel = (lpData[i] & 0x0F) + 1;
		BYTE arg;
		//OutTrace("midi: command=%#x\n", command);
		char *cmdString;
		int bytecount;
		subString[0] = 0;
		switch (command) {
			case 0x80: 
			case 0x90: 
				bytecount = 2; 
				cmdString = (command == 0x80) ? "Note Off" : "Note On"; 
				sprintf(subString, "note=%d pitch=%d", lpData[i+1], lpData[i+2]);
				break;  
			case 0xA0: bytecount = 2; cmdString = "Aftertouch"; break;  
			case 0xB0: bytecount = 2; cmdString = "Continuous Controller"; 
				arg = lpData[i+2];
				switch(lpData[i+1]) {
					case 0:		sprintf(subString, "sound bank selection"); break; // 2 bytes ignored
					case 1:		sprintf(subString, "vibrato=%d", arg); break;
					case 7:		sprintf(subString, "volume=%d", arg); break;
					case 10:	sprintf(subString, "panoramic=%d", arg); break;
					case 11:	sprintf(subString, "expression=%d", arg); break;
					case 32:	sprintf(subString, "sound bank selection (LSB) bank=%d", arg); break;
					case 64:	sprintf(subString, "sustain=%d", arg); break;
					case 121:	sprintf(subString, "all controllers off=%d", arg); break;
					case 123:	sprintf(subString, "all notes off=%d", arg); break;
				}
				break; 
			case 0xC0: bytecount = 1; cmdString = "Patch Change"; break;  
			case 0xD0: bytecount = 1; cmdString = "Channel Pressure"; break;  
			case 0xE0: bytecount = 2; cmdString = "Pitch Bend"; break;  
			case 0xF0:
			default:
				if(command == 0xFF){
					bytecount = 0;
					cmdString = "reset";
				}
				else {
					bytecount = Length; // to break the loop
					cmdString = "Unknown";
				}
				break;
		}
		if(bytecount == 1)
			OutTrace("[%d] %#2x-%#2x   : ch%d %s %s\n", seq, lpData[i], lpData[i+1], channel, cmdString, subString);
		else
			OutTrace("[%d] %#2x-%#2x-%#2x: ch%d %s %s\n", seq, lpData[i], lpData[i+1], lpData[i+2], channel, cmdString, subString);
		i += (bytecount + 1);
		seq ++;
		if(Length == 0) break;
	}
}