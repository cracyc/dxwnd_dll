#ifdef XBOX2KEYBOARDTHREAD
#define _WIN32_WINNT 0x0600
#define _MODULE "dxwnd" 

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <XInput.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

#define MAXINPUTS 20
#define MAXBUTTONS 16

class CXBOXController{
private:
	XINPUT_STATE _controllerState;
	int _controllerNum;
public:
	CXBOXController(int playerNumber);
	XINPUT_STATE GetState();
	bool IsConnected();
	void Vibrate(int leftVal = 0, int rightVal = 0);
};

CXBOXController::CXBOXController(int playerNumber)
{
	// Set the Controller Number
	_controllerNum = playerNumber - 1;
}

XINPUT_STATE CXBOXController::GetState()
{
	// Zeroise the state
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));
	// Get the state
	XInputGetState(_controllerNum, &_controllerState);
	return _controllerState;
}

bool CXBOXController::IsConnected()
{
	// Zeroise the state
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));
	// Get the state
	DWORD Result = XInputGetState(_controllerNum, &_controllerState);
	return (Result == ERROR_SUCCESS);
}

void CXBOXController::Vibrate(int leftVal, int rightVal)
{
	// Create a Vibraton State
	XINPUT_VIBRATION Vibration;
	// Zeroise the Vibration
	ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));
	// Set the Vibration Values
	Vibration.wLeftMotorSpeed = leftVal;
	Vibration.wRightMotorSpeed = rightVal;
	// Vibrate the controller
	XInputSetState(_controllerNum, &Vibration);
}

WORD vk[MAXBUTTONS] = {
	VK_UP,
	VK_DOWN,
	VK_LEFT,
	VK_RIGHT, 
	0,
	0,
	VK_UP,
	VK_DOWN,
	VK_LEFT,
	VK_RIGHT, 
	0,		// unused
	0,		// unused
	VK_CONTROL,
	VK_SPACE,
	VK_SPACE,
	VK_SPACE
};

typedef struct {
	char *code;
	WORD val;
} KEYMAP;

KEYMAP kmap[]={
	{"VK_LBUTTON", 0x01},
	{"VK_RBUTTON", 0x02},
	{"VK_CANCEL", 0x03},
	{"VK_MBUTTON", 0x04},
	{"VK_XBUTTON1", 0x05},
	{"VK_XBUTTON2", 0x06},
	{"VK_BACK", 0x08},
	{"VK_TAB", 0x09},
	{"VK_CLEAR", 0x0C},
	{"VK_RETURN", 0x0D},
	{"VK_ENTER", 0x0D}, // added as VK_RETURN alias
	{"VK_SHIFT", 0x10},
	{"VK_CONTROL", 0x11},
	{"VK_MENU", 0x12},
	{"VK_PAUSE", 0x13},
	{"VK_CAPITAL", 0x14},
	{"VK_ESCAPE", 0x1B},
	{"VK_SPACE", 0x20},
	{"VK_PRIOR", 0x21},
	{"VK_NEXT", 0x22},
	{"VK_END", 0x23},
	{"VK_HOME", 0x24},
	{"VK_LEFT", 0x25},
	{"VK_UP", 0x26},
	{"VK_RIGHT", 0x27},
	{"VK_DOWN", 0x28},
	{"VK_SELECT", 0x29},
	{"VK_PRINT", 0x2A},
	{"VK_EXECUTE", 0x2B},
	{"VK_SNAPSHOT", 0x2C},
	{"VK_INSERT", 0x2D},
	{"VK_DELETE", 0x2E},
	{"VK_HELP", 0x2F},
	{"0", 0x30},
	{"1", 0x31},
	{"2", 0x32},
	{"3", 0x33},
	{"4", 0x34},
	{"5", 0x35},
	{"6", 0x36},
	{"7", 0x37},
	{"8", 0x38},
	{"9", 0x39},
	{"A", 0x41},
	{"B", 0x42},
	{"C", 0x43},
	{"D", 0x44},
	{"E", 0x45},
	{"F", 0x46},
	{"G", 0x47},
	{"H", 0x48},
	{"I", 0x49},
	{"J", 0x4A},
	{"K", 0x4B},
	{"L", 0x4C},
	{"M", 0x4D},
	{"N", 0x4E},
	{"O", 0x4F},
	{"P", 0x50},
	{"Q", 0x51},
	{"R", 0x52},
	{"S", 0x53},
	{"T", 0x54},
	{"U", 0x55},
	{"V", 0x56},
	{"W", 0x57},
	{"X", 0x58},
	{"Y", 0x59},
	{"Z", 0x5A},
	{"VK_LWIN", 0x5B},
	{"VK_RWIN", 0x5C},
	{"VK_APPS", 0x5D},
	{"VK_NUMPAD0", 0x60},
	{"VK_NUMPAD1", 0x61},
	{"VK_NUMPAD2", 0x62},
	{"VK_NUMPAD3", 0x63},
	{"VK_NUMPAD4", 0x64},
	{"VK_NUMPAD5", 0x65},
	{"VK_NUMPAD6", 0x66},
	{"VK_NUMPAD7", 0x67},
	{"VK_NUMPAD8", 0x68},
	{"VK_NUMPAD9", 0x69},
	{"VK_MULTIPLY", 0x6A},
	{"VK_ADD", 0x6B},
	{"VK_SEPARATOR", 0x6C},
	{"VK_SUBTRACT", 0x6D},
	{"VK_DECIMAL", 0x6E},
	{"VK_DIVIDE", 0x6F},
	{"VK_F1", 0x70},
	{"VK_F2", 0x71},
	{"VK_F3", 0x72},
	{"VK_F4", 0x73},
	{"VK_F5", 0x74},
	{"VK_F6", 0x75},
	{"VK_F7", 0x76},
	{"VK_F8", 0x77},
	{"VK_F9", 0x78},
	{"VK_F10", 0x79},
	{"VK_F11", 0x7A},
	{"VK_F12", 0x70B},
	{"VK_NUMLOCK", 0x90},
	{"VK_SCROLL", 0x91},
	{"VK_LSHIFT", 0xA0},
	{"VK_RSHIFT", 0xA1},
	{"VK_LCONTROL", 0xA2},
	{"VK_RCONTROL", 0xA3},
	{"VK_LMENU", 0xA4},
	{"VK_RMENU", 0xA5},
	{0, 0}
};

static WORD str2key(char *str){
	DWORD key;
	if(strlen(str) == 0) return 0;
	for(UINT i=0; i<strlen(str); i++) str[i] = toupper(str[i]);
	if(!strncmp(str, "0X", 2)) {
		sscanf_s(str, "%x", &key); 
		OutTrace("str2key: s(hex)=\"%s\" ret=%#x\n", str, key);
		return (WORD)key;
	}
	if((str[0]>='0') && (str[0]<='9')){
		sscanf_s(str, "%d", &key); 
		OutTrace("str2key: s(dec)=\"%s\" ret=%#x\n", str, key);
		return (WORD)key;
	}
	for(UINT i=0; kmap[i].code; i++){
		if(!strcmp(kmap[i].code, str)) {
			OutTrace("str2key: s(key)=\"%s\" ret=%#x\n", str, kmap[i].val);
			return kmap[i].val;
		}
	}
	OutTrace("str2key s(key)=\"%s\" UNKNOWN: disabled\n", str);
	return(0);
}

static void LoadConfiguration(WORD *vk)
{
	char *iniPath= ".\\keymap.ini";
	char *appName= "keymap";
	char strBuf[80];
	GetPrivateProfileStringA(appName, "DPAD_UP", "", strBuf, 80, iniPath);
	vk[0]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "DPAD_DOWN", "", strBuf, 80, iniPath);
	vk[1]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "DPAD_LEFT", "", strBuf, 80, iniPath);
	vk[2]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "DPAD_RIGHT", "", strBuf, 80, iniPath);
	vk[3]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "START", "", strBuf, 80, iniPath);
	vk[4]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "BACK", "", strBuf, 80, iniPath);
	vk[5]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "LEFT_THUMB", "", strBuf, 80, iniPath);
	vk[6]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "RIGHT_THUMB", "", strBuf, 80, iniPath);
	vk[7]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "LEFT_SHOULDER", "", strBuf, 80, iniPath);
	vk[8]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "RIGHT_SHOULDER", "", strBuf, 80, iniPath);
	vk[9]=str2key(strBuf);
	vk[10]=0;
	vk[11]=0;
	GetPrivateProfileStringA(appName, "A", "", strBuf, 80, iniPath);
	vk[12]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "B", "", strBuf, 80, iniPath);
	vk[13]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "X", "", strBuf, 80, iniPath);
	vk[14]=str2key(strBuf);
	GetPrivateProfileStringA(appName, "Y", "", strBuf, 80, iniPath);
	vk[15]=str2key(strBuf);
}

DWORD WINAPI XBox2Keyboard(LPVOID lpData)
{
	CXBOXController *Player1 = new CXBOXController(1);
	INPUT Inputs[MAXINPUTS];
	BOOL bPressed[MAXBUTTONS];
	KEYBDINPUT ki;
	for(UINT i=0; i<MAXBUTTONS; i++) bPressed[i] = FALSE;
	LoadConfiguration(vk);
	int keyDelay = GetPrivateProfileIntA("timing", "keydelay", 5, ".\\keymap.ini");
	while(true){
		if(Player1->IsConnected()){
			WORD kInput = Player1->GetState().Gamepad.wButtons;
			for(UINT i=0; i<MAXBUTTONS; i++) {
				if(vk[i]) {
					if(kInput & flag){
						if(!bPressed[i]){
							INPUT *pInput = &Inputs[0];
							ki.wVk = vk[i];
							ki.dwExtraInfo = 0;
							ki.dwFlags = 0;
							ki.wScan = 0;
							ki.time = 0;
							pInput->type = INPUT_KEYBOARD;
							pInput->ki = ki;
							SendInput(1, pInput, sizeof(INPUT));
							Sleep(keyDelay);
							bPressed[i] = TRUE;
						}
					}
					else {
						if(bPressed[i]){
							INPUT *pInput = &Inputs[0];
							ki.wVk = vk[i];
							ki.dwExtraInfo = 0;
							ki.dwFlags = KEYEVENTF_KEYUP;
							ki.wScan = 0;
							ki.time = 0;
							pInput->type = INPUT_KEYBOARD;
							pInput->ki = ki;
							SendInput(1, pInput, sizeof(INPUT));
							Sleep(keyDelay);
							bPressed[i] = FALSE;
						}
					}
				}
			}

			//if(Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_BACK){
			//	break;
			//}
			Sleep(keyDelay);
		}
		else{
			Sleep(1000);
		}
	}

	delete(Player1);
	return( 0 );
}
#endif // XBOX2KEYBOARDTHREAD
