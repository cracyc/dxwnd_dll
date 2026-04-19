#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE 1
#define _MODULE "dxwnd"
#define X86LDASM

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <psapi.h>
#include <dbghelp.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#ifdef X86LDASM
#include "x86_ldasm_precompiled.h"
#else
#include "disasm.h"
#include "dwdisasm.h"
#endif

#include <DbgHelp.h>
#pragma comment( lib, "dbghelp.lib" )

//#define DXWNDEMULATE3DNOW
#define DXW_MAX_CONSECUTIVE_EXCEPTIONS 20
#define HANDLEVGAPALETTE TRUE
#define HANDLEHWSYNCWAIT TRUE
#define REPLACEDOSROMFONT TRUE

static int WriteCount389 = 0;
static int ReadCount389 = 0;
#define MODE_UNKNOWN 0
#define MODE_READ 1
#define MODE_WRITE 2
static int ModeCount389 = MODE_UNKNOWN;

#if REPLACEDOSROMFONT
// MOVZX DX,BYTE PTR DS:[ECX+EDX+0FFA6E]
static BYTE asmSequence[9]  = { 0x66, 0x0F, 0xB6, 0x94, 0x0A, 0x6E, 0xFA, 0x0F, 0x00 };
static BYTE int1ASequence[2]  = { 0xCD, 0x1A };
static LPBYTE newDOSROM = NULL;
#endif // REPLACEDOSROMFONT
DWORD gVGARegister = 0;

static void SyncPalette(LPBYTE VGAPalette)
{
	extern void mySetPalette(int, int, LPPALETTEENTRY);
	BYTE pal[256*4];
	LPBYTE q = pal;
	LPBYTE p = VGAPalette;
	for (int i=0; i<256; i++){
		*q++ = ((*p++) << 2);
		*q++ = ((*p++) << 2);
		*q++ = ((*p++) << 2);
		*q++ = 0;
	}
	mySetPalette(0, 256, (LPPALETTEENTRY)pal);
}

static LONG ForceNOPs(ApiArg, char *event, PVOID target, int cmdlen)
{
	DWORD oldprot;
	BOOL ret;
	if(!VirtualProtect(target, cmdlen, PAGE_READWRITE, &oldprot)) {
		OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
		return EXCEPTION_CONTINUE_SEARCH; // error condition
	}
	OutTrace("%s: NOP event=%s target=%#x len=%d opcode=%s\n", ApiRef, event, target, cmdlen, hexdump((BYTE *)target, cmdlen));
	memset((BYTE *)target, 0x90, cmdlen); 
	VirtualProtect(target, cmdlen, oldprot, &oldprot);
	ret = FlushInstructionCache(GetCurrentProcess(), target, cmdlen);
	_if(!ret) OutTrace("%s: FlushInstructionCache ERROR target=%#x, err=%#x\n", ApiRef, target, GetLastError());
	return EXCEPTION_CONTINUE_EXECUTION; 
}

#if 0
static void HandleDEP(HANDLE hModule, LPVOID address)
{
   // get the location of the module's IMAGE_NT_HEADERS structure
   IMAGE_NT_HEADERS *pNtHdr = ImageNtHeader(hModule);

   // section table immediately follows the IMAGE_NT_HEADERS
   IMAGE_SECTION_HEADER *pSectionHdr = (IMAGE_SECTION_HEADER *)(pNtHdr + 1);

   const char* imageBase = (const char*)hModule;
   char scnName[sizeof(pSectionHdr->Name) + 1];
   scnName[sizeof(scnName) - 1] = '\0'; // enforce nul-termination for scn names that are the whole length of pSectionHdr->Name[]

   for (int scn = 0; scn < pNtHdr->FileHeader.NumberOfSections; ++scn)
   {
      // Note: pSectionHdr->Name[] is 8 bytes long. If the scn name is 8 bytes long, ->Name[] will
      // not be nul-terminated. For this reason, copy it to a local buffer that's nul-terminated
      // to be sure we only print the real scn name, and no extra garbage beyond it.
      strncpy(scnName, (const char*)pSectionHdr->Name, sizeof(pSectionHdr->Name));

      OutTrace("  Section %3d: %p...%p %-10s (%u bytes)\n",
         scn,
         imageBase + pSectionHdr->VirtualAddress,
         imageBase + pSectionHdr->VirtualAddress + pSectionHdr->Misc.VirtualSize - 1,
         scnName,
         pSectionHdr->Misc.VirtualSize);
      ++pSectionHdr;
   }
}

void SearchModules(LPVOID address)
{
	DWORD processID;
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;
    CHAR szModName[MAX_PATH];
 
    // Get a list of all the modules in this process.

	OutTrace("SearchModules addr=%#x\n", address);
	processID = GetCurrentProcessId();
    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID );
	if (hProcess == NULL){
		OutTraceE("OpenProcess ERROR err=%d\n", GetLastError());
		return;
	}

	HMODULE hmod = (*LoadLibrary)("psapi.dll");
	if(!hmod) {
		OutTraceE("LoadLibrary(psapi) ERROR err=%d\n", GetLastError());
		return;
	}
	typedef DWORD (WINAPI *GetModuleFileNameExA_Type)(HANDLE, HMODULE, LPSTR, DWORD);
	typedef BOOL (WINAPI *EnumProcessModules_Type)(HANDLE, HMODULE *, DWORD, LPDWORD);
	GetModuleFileNameExA_Type pGetModuleFileNameExA = (GetModuleFileNameExA_Type)(*GetProcAddress)(hmod, "GetModuleFileNameExA");
	EnumProcessModules_Type pEnumProcessModules = (EnumProcessModules_Type)(*GetProcAddress)(hmod, "EnumProcessModules");
	if(!pGetModuleFileNameExA) {
		OutTraceE("GetModuleFileNameExA MISSING\n");
		return;
	}
	if(!pEnumProcessModules) {
		OutTraceE("EnumProcessModules MISSING\n");
		return;
	}
	//CloseHandle(hmod);
	if((*pEnumProcessModules)(hProcess, hMods, sizeof(hMods), &cbNeeded)){
		for(i = 0; i < (cbNeeded / sizeof(HMODULE)); i++){
            if((*pGetModuleFileNameExA)(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(CHAR))){
                // Print the module name and handle value.
				OutTrace("\t%s (0x%08X)\n", szModName, hMods[i] );
				HandleDEP(hMods[i], address);
            }
        }
    }
	else {
		OutTrace("no modules\n");
	}
    CloseHandle(hProcess);
}
#endif

LONG WINAPI DxWExceptionHandler(LPEXCEPTION_POINTERS ExceptionInfo)
{
	ApiName("DxWExceptionHandler");
	static HMODULE disasmlib = NULL;
	static int iExceptionCounter = 0;
	static VOID *pNext1 = 0;
	static VOID *pNext2 = 0;
	static DWORD iStartPaletteIndex = 0;
	static BYTE VGAPalette[256*3]; // emulates the content of VGA palette data
	BOOL ret;
	PCONTEXT lpContext = ExceptionInfo->ContextRecord;
	PEXCEPTION_RECORD pException = ExceptionInfo->ExceptionRecord;

	OutDebugSYS("%s: exception code=%#x subcode=%#x flags=%#x addr=%#x\n"
		"> eax=%#x ebx=%#x ecx=%#x edx=%#x\n"
		"> esp=%#x ebp=%#x esi=%#x edi=%#x\n"
		"> eip=%#x\n",
		ApiRef, pException->ExceptionCode,
		pException->ExceptionInformation[0],
		pException->ExceptionFlags,
		pException->ExceptionAddress,
		lpContext->Eax, lpContext->Ebx, lpContext->Ecx, lpContext->Edx, 
		lpContext->Esp, lpContext->Ebp, lpContext->Esi, lpContext->Edi,
		lpContext->Eip);

	DWORD oldprot;
	PVOID target = pException->ExceptionAddress;
	ULONG_PTR subcode = pException->ExceptionInformation[0];
	int cmdlen;
#ifndef X86LDASM
	t_disasm da;
	if(!disasmlib){
		if (!(disasmlib=LoadDisasm())) return EXCEPTION_CONTINUE_SEARCH;
		(*pPreparedisasm)();
	}
#endif
	// v2.05.86: ignore and continue search also for a number of non-recoverable exceptons above 0x80000000
	if(pException->ExceptionCode <= 0x80000014){
		// from https://stackoverflow.com/questions/12298406/how-to-treat-0x40010006-exception-in-vectored-exception-handler
		// Exception codes with values less than 0x80000000 are just informal and never an indicator 
		// of real trouble. In general, you should never mess with exceptions that you don't recognize 
		// and don't want to explicitly handle. Let Windows continue searching for a handler by 
		// returning EXCEPTION_CONTINUE_SEARCH, the debugger will probably catch it in this case.
		OutTraceSYS("%s: EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
		return EXCEPTION_CONTINUE_SEARCH;
	}
	cmdlen = 0;
	switch(pException->ExceptionCode){
		/*
		// ignored exceptions
		case 0x80000001: // STATUS_GUARD_PAGE_VIOLATION: Guard Page Exception.
						 // A page of memory that marks the end of a data structure, such as a stack or an array, has been accessed.
		 case 0x80000002: // STATUS_DATATYPE_MISALIGNMENT: Alignment Fault. 
						 // A datatype misalignment was detected in a load or store instruction.
		case 0x80000003: // STATUS_BREAKPOINT: INT3 debugging exception, ignore it
		case 0x80000004: // STATUS_SINGLE_STEP: single step completed, ignore it
		case 0x80000005: // STATUS_BUFFER_OVERFLOW: Buffer Overflow. The data was too large to fit into the specified buffer.
		case 0x80000006: // STATUS_NO_MORE_FILES: No more files were found which match the file specification.
		case 0x80000007: // STATUS_WAKE_SYSTEM_DEBUGGER the system debugger was awakened by an interrupt.
		case 0x8000000A: // STATUS_HANDLES_CLOSED: Handles to objects have been automatically closed as a result of the requested operation.
		case 0x8000000B: // STATUS_NO_INHERITANCE
		case 0x8000000C: // STATUS_GUID_SUBSTITUTION_MADE
		case 0x8000000D: // STATUS_PARTIAL_COPY
		case 0x8000000E: // STATUS_DEVICE_PAPER_EMPTY: The printer is out of paper.
		case 0x8000000F: // STATUS_DEVICE_POWERED_OFF: The printer power has been turned off.
		case 0x80000010: // STATUS_DEVICE_OFF_LINE: The printer has been taken offline.
		case 0x80000011: // STATUS_DEVICE_BUSY: The device is currently busy.
		case 0x80000012: // STATUS_NO_MORE_EAS
		case 0x80000013: // STATUS_INVALID_EA_NAME
		case 0x80000014: // STATUS_EA_LIST_INCONSISTENT
			// follow on https://efmsoft.com/what-is/amp/?code=0x80000015&type=1
		*/
		case 0xc0000008: // invalid handle - Better ignore. Ref. "Dungeon Keeper, Deeper Dungeons D3D" 
		case 0xe06d7363: // v2.05.05: from MSDN: The Visual C++ compiler uses exception code 0xE06D7363 for C++ exceptions. 
						 // https://blogs.msdn.microsoft.com/oldnewthing/20100730-00/?p=13273
						 // fixes "Rage of Mages - Allods" with "Handle exceptions" flag.
		case 0xc000008e: // Floating point division by 0
						 // found in "Jane's Longbow Gold" instant action, returning EXCEPTION_CONTINUE_SEARCH fixes the problem.
			OutTraceSYS("%s: EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
			return EXCEPTION_CONTINUE_SEARCH;
			break;
		case 0xc0000096: // CLI Priviliged instruction exception (Resident Evil), FB (Asterix & Obelix)
						 // also OUT & REP instructions to write to output ports, used by legacy DOS to
						 // write palette data ("Dark Judgement").
						 // To do: possible usage of different registers, handling of D flag set
			if(HANDLEVGAPALETTE){
				// needed by:
				// @#@ "Dark Judgement"
				// @#@ "Club Rapper" (Ko)
				// @#@ "Campus Heroes" (Ko)
				// @#@ "Terracide" (1997)
				//
				//Command:  REP OUTS DX,BYTE PTR DS:[ESI]
				//Hex dump: F3:6E
				//
				//Copies ECX words from the memory to the hardware port.
				//
				//On each iteration, processor reads byte from the memory at address [ESI]
				//and sends it to the port addressed by the contents of the 16-bit register
				//DX. If flag D is cleared, increments ESI by 2, otherwise decrements ESI by
				//2. Register ECX is always decremented by 1. If ECX after decrement is
				//zero, operation stops; otherwise, processor repeats the whole cycle again
				//and again, until count exhausts.

				BOOL bHandled = FALSE;
				if(!VirtualProtect(target, 10, PAGE_READWRITE, &oldprot)) {
					OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
					return EXCEPTION_CONTINUE_SEARCH; // error condition
				}
#ifdef X86LDASM
				cmdlen = x86_ldasm(NULL, 32, (unsigned char *)target);
#else
				cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
#endif

				OutDebugVGA("Got cmdlen=%d assembly=%s EIP=%#x EAX=%#x EDX=%#x\n", 
					cmdlen, hexdump((BYTE *)target, cmdlen), lpContext->Eip, lpContext->Eax, lpContext->Edx); 

				if(cmdlen == 1) {
					BYTE op = *(BYTE *)target;
					switch (op){
						case 0xFA:
						case 0xFB: 
							OutTraceVGA("Detected '%s' @%#x\n", (op == 0XFA) ? "CLI" : "STI", lpContext->Eip); 
							if(dxw.dwFlags16 & FORCENOPS){
								return ForceNOPs(ApiRef, "CLI-STI", target, cmdlen); 
							}
							else {
								if(dxw.dwFlags15 & SLOWDOWNEXCEPTIONS) Sleep(5);
								lpContext->Eip += cmdlen; // skip ahead op-code length
								return EXCEPTION_CONTINUE_EXECUTION; 
							}
							break;
						// default fall through
					}
				}
				if((cmdlen == 2) && (*(BYTE *)target == 0xF3) && (lpContext->Edx == 0x3C9)){
					bHandled = TRUE;
					DWORD iCount = lpContext->Ecx % 0x300; // how many bytes of data to be transferred
					lpContext->Ecx = 0; // clear ECX
					if(iCount == 0) iCount=768; // ref. ...
					LPBYTE lpESI = (LPBYTE)lpContext->Esi;
					lpContext->Esi += iCount; // increment ESI
					OutTraceVGA("%s: DAC WRITE count=%#x(#%d)\n", ApiRef, iCount, iCount);
					OutHexDW(lpESI, iCount);
					if(iCount + iStartPaletteIndex <= 0x300) { // bounds protection
						memcpy(VGAPalette+(iStartPaletteIndex), lpESI, iCount);
					}
					SyncPalette(VGAPalette);
				}
				else
					if((cmdlen == 1) && (*(BYTE *)target == 0xEE)){
						bHandled = TRUE;
						switch (lpContext->Edx & 0x3FF){
							case 0x3C7: {
								//Command:  OUT DX,AL
								//Hex dump: EE
								//
								//Writes contents of 8-bit register AL to the I/O port DX. Flags are not
								//affected. 
								// n.b. AL = EAX & 0xFF

								iStartPaletteIndex = (lpContext->Eax & 0x000000FF) * 3;
								ReadCount389 = iStartPaletteIndex * 3;
								ModeCount389 = MODE_READ;
								OutTraceVGA("%s: DAC WRITE port=0x3C7 addr=%#x(#%d)\n", ApiRef, iStartPaletteIndex, iStartPaletteIndex);
							}
							break;
							case 0x3C8: {
								//Command:  OUT DX,AL
								//Hex dump: EE
								//
								//Writes contents of 8-bit register AL to the I/O port DX. Flags are not
								//affected. 
								// n.b. AL = EAX & 0xFF

								iStartPaletteIndex = (lpContext->Eax & 0x000000FF) * 3;
								WriteCount389 = iStartPaletteIndex;
								ModeCount389 = MODE_WRITE;
								OutTraceVGA("%s: DAC WRITE port=0x3C8 addr=%#x(#%d)\n", ApiRef, iStartPaletteIndex, iStartPaletteIndex);
							}
							break;
							case 0x3C9: {
								//Command:  OUT DX,AL
								//Hex dump: EE
								//
								//Writes contents of 8-bit register AL to the I/O port DX. Flags are not
								//affected. 
								// n.b. AL = EAX & 0xFF

								// v2.05.85: added handling of write to port 3C9.
								// this is peculiar: each operation writes a following byte in the VGA palette
								// following the order RED, GREEN, BLUE, so writes must always be in multiple
								// of the number 3.

								// BEWARE: it must either read or write !!!!
								if(ModeCount389 == MODE_WRITE){
									DWORD color = (lpContext->Eax & 0x000000FF);
									if(WriteCount389 < (256*3)) VGAPalette[WriteCount389] = (BYTE)color;
									WriteCount389++;
									if((WriteCount389 % 3) == 0) SyncPalette(VGAPalette);
									//if(WriteCount389 >= 256*3) WriteCount389 = 0; -- no, causes glitches on "Club Rapper"
									OutTraceVGA("%s: DAC WRITE port=0x3C9 index=%d color=%#x @%d\n", ApiRef, WriteCount389, color, __LINE__);
								}
							}
							break;
							case 0x3D4: {
								// @#@ Terracide - get cursor position?
								gVGARegister = lpContext->Eax & 0xFF;
								OutTraceVGA("%s: DAC WRITE port=0x3D4 eax=%#x AL=%x\n", ApiRef, lpContext->Eax, lpContext->Eax & 0xFF);
							}
							break;
							case 0x3D5:
							case 0x3B4: 
							case 0x3DA: 
							case 0x3C0: 
							case 0x3C1: {
								// @#@ Jane's Apache Longbow Gold - set VGA status?
								OutTraceVGA("%s: DAC WRITE port=%#x eax=%#x\n", ApiRef, lpContext->Edx & 0x3FF, lpContext->Eax);
							}
							break;
							default: {
								OutErrorVGA("%s: DAC WRITE port=%#x UNMANAGED @%d\n", ApiRef, lpContext->Edx, __LINE__);
							}
							break;
						}
					}
				else
				if((cmdlen == 1) && ((*(BYTE *)target == 0xEC) || (*(BYTE *)target == 0xED))){
					// EC = IN AL,DX
					// ED = IN EAX,DX
					bHandled = TRUE;
					gVGARegister = lpContext->Eax & 0xFF;
					//OutTraceVGA("%s: DAC READ port=%#x eax=%#X\n", ApiRef, lpContext->Edx, lpContext->Eax);
					switch (lpContext->Edx & 0x3FF){
							case 0x3D4: {
								gVGARegister = lpContext->Eax & 0xFF;
								OutTraceVGA("%s: DAC READ port=0x3D4 eax=%#x AL=%x\n", ApiRef, lpContext->Eax, lpContext->Eax & 0xFF);
							}
							break;
							case 0x3DA: {
								// @#@ Jane's Apache Longbow Gold - get VGA status?
								OutTraceVGA("%s: DAC READ port=0x3DA eax=%#x\n", ApiRef, lpContext->Eax);
								lpContext->Eax = 0x00000004;
							}
							break;							
							case 0x3D5:
							case 0x3B5: 
							case 0x3C0: 
							case 0x3C1: {
								// @#@ Jane's Apache Longbow Gold - ???
								OutTraceVGA("%s: DAC READ port=%#x eax=%#x\n", ApiRef, lpContext->Edx & 0x3FF, lpContext->Eax);
							}
							break;
							default: {
								OutErrorVGA("%s: DAC READ port=%#x UNMANAGED @%d\n", ApiRef, lpContext->Edx, __LINE__);
							}
							break;
					}
				}
				// recover previous protections, do not NOP assembly
				VirtualProtect(target, 10, oldprot, &oldprot);
				ret = FlushInstructionCache(GetCurrentProcess(), target, cmdlen);
				_if(!ret) OutTraceVGA("%s: FlushInstructionCache ERROR target=%#x, err=%#x\n", ApiRef, target, GetLastError());
				if(bHandled){
					// if handle DAC WRITE operation, skip offending assembly
					lpContext->Eip += cmdlen; // skip ahead op-code length
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if(HANDLEHWSYNCWAIT){
				// this pattern:
				// start: IN AL,DX ; I/O command
				// TEST AL,08
				// JNZ SHORT start or JZ SHORT start
				// is often used to sample the video card status and wait for a vSync status.
				// since the JNZ block is followed by a JZ block removing the IN instruction is not
				// enough to avoid an endless loop. You have to clear all 3 instructions.

				if(!VirtualProtect(target, 10, PAGE_READWRITE, &oldprot)) {
					OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
					return EXCEPTION_CONTINUE_SEARCH; // error condition
				}
#ifdef X86LDASM
				cmdlen = x86_ldasm(NULL, 32, (unsigned char *)target);
#else
				cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
#endif				
				if((cmdlen == 1) && (*(BYTE *)target == 0xEC) && (lpContext->Edx == 0x3DA)){
					LPBYTE p = (LPBYTE)target;
					if( (p[1]==0xA8) &&
						(p[2]==0x08) && // A8 08 => TEST AL, 08
						((p[3]==0x74) || (p[3]==0x75)) && // 74 => JZ; 75 => JNZ
						(p[4]==0xFB)) // FB => SHORT (-4)
					{
						memset((BYTE *)target, 0x90, 5); 
						VirtualProtect(target, 10, oldprot, &oldprot);
						ret = FlushInstructionCache(GetCurrentProcess(), target, cmdlen);
						_if(!ret) OutTrace("%s: FlushInstructionCache ERROR target=%#x, err=%#x\n", ApiRef, target, GetLastError());
						// v2.03.10 skip replaced opcode
						lpContext->Eip += cmdlen; // skip ahead op-code length
						return EXCEPTION_CONTINUE_EXECUTION;
					}
				}
			}
			// else fall-through ....
		case 0xc0000094: // INTEGER_DIV_BY_0 from IDIV reg (@#@ "Ultim@te Race Pro")
		case 0xc0000095: // INTEGER_OVERFLOW from DIV by 0 (divide overflow) exception (@#@ "SonicR")
			// @#@ "Max Force"
			if(cmdlen == 0) {
				// if not falling through, initialize ...
				if(!VirtualProtect(target, 10, PAGE_READWRITE, &oldprot)) {
					OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
					return EXCEPTION_CONTINUE_SEARCH; // error condition
				}
#ifdef X86LDASM
				cmdlen = x86_ldasm(NULL, 32, (unsigned char *)target);
#else
				cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
#endif
			}
			if ((*((LPBYTE)lpContext->Eip) == 0xF7) ||     // Handle idiv
				(*((LPBYTE)lpContext->Eip+1) == 0xF7)) {   // Handle 16 bit idiv
					OutTrace("%s: Detected 'idiv' overflow: validating edx:eax @%#x\n", ApiRef, target);
				if(dxw.dwFlags16 & FORCENOPS){
					return ForceNOPs(ApiRef, "iDiv", target, cmdlen); 
				}
				lpContext->Edx=0;
				if ((LONG)lpContext->Eax < 0){
					lpContext->Eax = (DWORD)(-(LONG)lpContext->Eax);
					OutTrace("%s: fixed 'idiv' overflow: edx:eax=%#x\n", ApiRef, lpContext->Eax);
				}
				return EXCEPTION_CONTINUE_EXECUTION;
			 }
			// else fall through
#ifndef DXWNDEMULATE3DNOW
		case 0xc000001d: // 3DNow! instructions: FEMMS (eXpendable), FPADD etc. (Arthur's Knights I & II, the New Adventures of the Time Machine)
#endif
			//case 0xc0000005: // Memory exception (Tie Fighter) -- moved to SKIP processing
			if(!VirtualProtect(target, 10, PAGE_READWRITE, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
				return EXCEPTION_CONTINUE_SEARCH; // error condition
			}
#ifdef X86LDASM
			cmdlen = x86_ldasm(NULL, 32, (unsigned char *)target);
#else
			cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
#endif			
			return ForceNOPs(ApiRef, "3DNow!", target, cmdlen);
			break;
#ifdef DXWNDEMULATE3DNOW
		case 0xc000001d: // 3DNOW instructions
			if(!VirtualProtect(target, 10, PAGE_READWRITE, &oldprot)) return EXCEPTION_CONTINUE_SEARCH; // error condition
			cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
			if(*(BYTE *)target == 0x0F){
				OutTrace("%s: EMULATE opcode=%s len=%d\n", ApiRef, hexdump((BYTE *)target, cmdlen), cmdlen);
				extern void Emulate3DNow(BYTE *, int, PCONTEXT);
				Emulate3DNow((BYTE *)target, cmdlen, lpContext);
			}
			else {
				OutTrace("%s: NOP opcode=%s len=%d\n", ApiRef, hexdump((BYTE *)target, cmdlen), cmdlen);
				memset((BYTE *)target, 0x90, cmdlen); 
			}
			VirtualProtect(target, 10, oldprot, &oldprot);
			if(!FlushInstructionCache(GetCurrentProcess(), target, cmdlen))
				OutTrace("%s: FlushInstructionCache ERROR target=%#x, err=%#x\n", ApiRef, target, GetLastError());
			// v2.03.10 skip replaced opcode
			lpContext->Eip += cmdlen; // skip ahead op-code length
			return EXCEPTION_CONTINUE_EXECUTION;
			break;
#endif
		case 0xc0000005: 
#if REPLACEDOSROMFONT
			if(!VirtualProtect(target, 10, PAGE_READONLY, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
				return EXCEPTION_CONTINUE_SEARCH; // error condition
			}
#ifdef X86LDASM
				cmdlen = x86_ldasm(NULL, 32, (unsigned char *)target);
#else
				cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
#endif			
				if((cmdlen == 9) && !memcmp(target, asmSequence, 9)){
				// v2.05.88 detect DOSROM access attempts and redirect to an alternate readable memory area
				// needed in "Astro3D II"
				// thanks to https://github.com/spacerace/romfont/blob/master/font-bin/IBM_PC_V3_8x8.bin
				OutTrace("%s: DOSROM access opcode=%s len=%d\n", ApiRef, hexdump((BYTE *)target, cmdlen), cmdlen); 
				DWORD oldProtDummy;
				if (!newDOSROM) {
					char path[MAX_PATH];
					UINT romLen;
					char *romFName = "IBM_PC_8x8.bin";
					sprintf(path, "%s/%s", GetDxWndPath(), romFName);
					FILE *fRom = fopen(path, "rb");
					if(!fRom) OutTraceE("%s: ERROR opening DOSROM file=%s err=%d\n", ApiRef, romFName, GetLastError());
					fseek(fRom, 0, SEEK_END);
					romLen = ftell(fRom);
					fseek(fRom, 0, SEEK_SET);
					OutTraceDW("%s: reading DOSROM file=%s len=%d\n", ApiRef, romFName, romLen);
					newDOSROM = (LPBYTE)malloc(romLen);
					if(fread(newDOSROM, 1, romLen, fRom) != 1) OutTraceE("%s: ERROR reading DOSROM err=%d\n", ApiRef, GetLastError());
					fclose(fRom);
					OutHexDW(newDOSROM, romLen);
				}
				if(!VirtualProtect(target, 10, PAGE_READWRITE, &oldProtDummy)) {
					OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
					return EXCEPTION_CONTINUE_SEARCH; // error condition
				}
				//memcpy((BYTE *)target+5, (LPVOID)newDOSROM, sizeof(DWORD)); 
				*(DWORD *)((LPBYTE)target + 5) = (DWORD)newDOSROM;
				VirtualProtect(target, 10, oldprot, &oldprot);
				ret = FlushInstructionCache(GetCurrentProcess(), target, cmdlen);
				_if(!ret) OutTrace("%s: FlushInstructionCache ERROR target=%#x, err=%#x\n", ApiRef, target, GetLastError());
				// v2.03.10 skip replaced opcode
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else if((cmdlen == 2) && !memcmp(target, int1ASequence, 2)) {
				DWORD dwTick;
				//MessageBox(0, "INT 1A", "DxWnd", 0);
				switch(lpContext->Eax >> 16){
					case 0:
						//MessageBox(0, "INT 1A, 0", "DxWnd", 0);
						//lpContext->Eax |= 0x00000001;
						dwTick = GetTickCount();
						OutTraceSYS("%s: GetTickCount=%#x ecx=%#x edx=%#x\n", ApiRef, dwTick, dwTick >> 16, dwTick & 0x0000FFFF);
						lpContext->Ecx = dwTick >> 16;
						lpContext->Edx = dwTick & 0x0000FFFF;
						lpContext->Eip += cmdlen;
						//Sleep(10);
						break;
					case 1:
						MessageBox(0, "INT 1A, 1", "DxWnd", 0);
						break;
					default:
						break;
				}
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else {
				if((dxw.dwFlags17 & SUPPRESSDEP) && (subcode==0x8)){
					if(!VirtualProtect(target, cmdlen, PAGE_READWRITE, &oldprot)) {
						OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
						return EXCEPTION_CONTINUE_SEARCH; // error condition
					}
					ret = FlushInstructionCache(GetCurrentProcess(), target, cmdlen);
					if(!(oldprot & PAGE_EXECUTE)){
						OutTrace("%s: SUPPRESSDEP adding execution permission @ip=%#x\n", ApiRef, target);
						if(!VirtualProtect(target, cmdlen, PAGE_EXECUTE, &oldprot)) {
							OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
							return EXCEPTION_CONTINUE_SEARCH; // error condition
						}
						ret = FlushInstructionCache(GetCurrentProcess(), target, cmdlen);
						return EXCEPTION_CONTINUE_EXECUTION;
					}
					// else falls through ...
				}
				if(dxw.dwFlags16 & FORCENOPS){
					return ForceNOPs(ApiRef, "AccessViolation", target, cmdlen);
				}
				else {
					OutTrace("%s: SKIP opcode=%s len=%d @%#x\n", ApiRef, hexdump((BYTE *)target, cmdlen), cmdlen, target); // v2.05.28 better logging
					lpContext->Eip += cmdlen; // skip ahead op-code length
					if(dxw.dwFlags15 & SLOWDOWNEXCEPTIONS) Sleep(5);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			break;
#endif // REPLACEDOSROMFONT
		default:
			if(!VirtualProtect(target, 10, PAGE_READONLY, &oldprot)) {
				OutTrace("%s: VirtualProtect ERROR EXCEPTION_CONTINUE_SEARCH @%#x\n", ApiRef, target);
				return EXCEPTION_CONTINUE_SEARCH; // error condition
			}
#ifdef X86LDASM
			cmdlen = x86_ldasm(NULL, 32, (unsigned char *)target);
#else
			cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
#endif			
			OutTrace("%s: SKIP opcode=%s len=%d @%#x\n", ApiRef, hexdump((BYTE *)target, cmdlen), cmdlen, target); // v2.05.28 better logging
			lpContext->Eip += cmdlen; // skip ahead op-code length
			if(dxw.dwFlags15 & SLOWDOWNEXCEPTIONS) Sleep(5);
			return EXCEPTION_CONTINUE_EXECUTION;
			break;
	}
	if((target == pNext1) || (target == pNext2)){
		iExceptionCounter ++;
		if(iExceptionCounter > DXW_MAX_CONSECUTIVE_EXCEPTIONS) {
			OutTrace("%s: MAX consecutive exceptions reached\n", ApiRef);
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}
	else {
		iExceptionCounter = 0;
	}
	pNext1 = target;
	pNext2 = (PBYTE)target + cmdlen;
}
