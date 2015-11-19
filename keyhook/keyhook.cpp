// keyhook.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
const int MAX_CHAR = 32;
const int MAX_TONE = 5;
const int SLEEP_TIME = 10; //milisecond
const int MAX_BUFF_SIZE = 128;
const char *in_diacritics[MAX_CHAR] = {"A", "AW", "AA", "B", "C", "D", "DD", "E", "EE","F", "G", "H", "I", "K", "L", "M", "N", "O", "OO", "OW", "P", "Q", "R", "S", "T", "U", "UW", "V", "X", "Y", "W", "Z"};
const char *out_diacritics[MAX_CHAR] = {"4", "4", "4", "|3", "c", "|)", "+)", "3", "3", "f", "g", "h", "j", "k", "|_", "m", "n", "0", "0", "0", "|3", "w", "z", "s", "t", "u", "u", "v", "x", "ij", "w", "z"};

const char *in_tone[MAX_TONE] = {"S", "F", "R", "X", "J"};
const char *out_tone[MAX_TONE] = {"'", "`", "?", "~", "."}; 


int lastLen;
char oldKey;
int shiftkey, controlkey, altkey;
int willHalt;
extern HWND currentHwnd;

/*
commit key first -> not in (0x41 -> 0x5A): reset buffer_in, buffer_out
tone second -> if (second_vowel_position) -> update_tone(buffer_in, buffer_out, new tone)
combine key third -> if (can combine with each char in buffer_in) -> update_key(buffer_in, buffer_out, new char)
itself -> search in diracritics, update_key(buffer_in, buffer_out, new char)
	
*/
UINT VK_BACK_SCAN = MapVirtualKey(VK_BACK, 0);
/*
clean everything to make sure input method work correctly
*/
void resetAll()
{
	lastLen = 0;
	oldKey = 0;
}

/*
get index of string in array
input: 
string: string to check
arr: arr to check if string in this
output:
-1: not in array
an integer: index of string in array
*/
int indexInArray(char *string, const char *arr[], int SIZEOF)
{
	for (int i = 0; i < SIZEOF; i++)
	{
		if (!strcmp(string, arr[i]))
			return i;
	}
	return -1;
}
/*
Send backspace key to input method
input:
	count: number of key to send
*/
void sendBackSpace(int count)
{
	for (int i = 0; i < count; i ++)
	{
		INPUT keyEvent = {0};
		/*keyEvent.type = INPUT_KEYBOARD;
		keyEvent.ki.wVk = MapVirtualKeyEx(vkCode, MAPVK_VK_TO_VSC, (HKL)0xf0010413);
		SendInput(1, &keyEvent, sizeof(keyEvent));*/


		keyEvent.type = INPUT_KEYBOARD;
		keyEvent.ki.wVk = VK_BACK;
		//keyEvent.ki.wScan = '|';
		//keyEvent.ki.dwFlags = KEYEVENTF_UNICODE;
		SendInput(1, &keyEvent, sizeof(INPUT));
		Sleep(SLEEP_TIME);
	}
}

/*
Send key to current focus input
input:
	key: key to send
*/
void sendKey(char key)
{
	INPUT keyEvent = {0};
	keyEvent.type = INPUT_KEYBOARD;
	keyEvent.ki.wVk = 0;
	keyEvent.ki.wScan = key;
	keyEvent.ki.dwFlags = KEYEVENTF_UNICODE;
	SendInput(1, &keyEvent, sizeof(INPUT));
	Sleep(SLEEP_TIME);
}

/*
Send an string out
input:
	string: string to be sent
	len: number of characters to be sent
*/
void sendStringOut(const char *string, int len)
{
	for (int i = 0; i < len ; i++)
		sendKey(string[i]);
}

/*
Send output out
input:
	string: what to check
	index: check if in this
	output: send this out
	lenSent: number characters is sent
output:
	true: sent
	false: can not send
*/
bool sendOutputOut(char *string, const char *index[], const char *output[], int*lenSent, int SIZEOF)
{
	int i = indexInArray(string, index, SIZEOF);
	if (i != -1)
	{
		sendBackSpace(*lenSent);
		sendStringOut(output[i], strlen(output[i]));
		*lenSent = strlen(output[i]);
		return true;
	}
	return false;
}
/*
hook function
get key input to hwnd and process it to teen code
*/
//enter a key:
//	if not in A - Z
//		-> lastlen = 0, oldkey = "", 
//		-> return;
//
//	if oldkey ="" 
//		-> check in diacritics array and push it out
//		->lastlen = strlen(output)
//	else 
//		-> if is tone key
//			->send tone
//			->lastlen = strlen(output) = 1
//		-> if (old key + new key) in diacritics
//			-> send lastlen backspace
//			-> send out_diacirtics
//			-> lastlen = strlen(output)
//		-> if new key in diacritcis
//			-> send out_diacritics
//			-> lastlen = strlen(output)
__declspec(dllexport) LRESULT  WINAPI keyhook(int nCode, WPARAM wParam, LPARAM lParam) 
{
	if (nCode != 0)
		return CallNextHookEx(NULL, nCode, wParam, lParam); 

	KBDLLHOOKSTRUCT* mystruct = (KBDLLHOOKSTRUCT*) lParam;
	if (wParam == WM_KEYDOWN)
	{		
		if (willHalt || controlkey || altkey || 0x41 > mystruct->vkCode || mystruct->vkCode > 0x5A)
		{
			//get control || altkey
			switch (mystruct->vkCode)
			{
			case VK_LCONTROL:
			case VK_RCONTROL:
				controlkey = 1;
				break;
			case VK_LMENU:
			case VK_RMENU:
				altkey = 1;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
				shiftkey = 1;
				break;
			}
			resetAll();
			return CallNextHookEx(NULL, nCode, wParam, lParam); 
		}
		char tbuffer[MAX_BUFF_SIZE] = {0};
		sprintf_s(tbuffer, "%c", (char)mystruct->vkCode);
		if (!oldKey)
		{
			sendOutputOut(tbuffer, in_diacritics, out_diacritics, &lastLen, MAX_CHAR);
			oldKey = mystruct->vkCode;
		}
		else
		{
			int tempLaslen = 0;
			if (!sendOutputOut(tbuffer, in_tone, out_tone, &tempLaslen, MAX_TONE)) // is tone
			{
				sprintf_s(tbuffer, "%c%c", oldKey, (char)mystruct->vkCode);
				if (!sendOutputOut(tbuffer, in_diacritics, out_diacritics, &lastLen, MAX_CHAR)) //is combie key
				{
					sprintf_s(tbuffer, "%c", mystruct->vkCode);
					lastLen = 0;
					if(sendOutputOut(tbuffer, in_diacritics, out_diacritics, &lastLen, MAX_CHAR)) //single key
						oldKey = mystruct->vkCode;
					else 
						return CallNextHookEx(NULL, nCode, wParam, lParam); 
				}
				else
					resetAll();
			}
			else
				resetAll();
		}
		return 1;
	}
	else if (wParam == WM_KEYUP)
	{
		if (controlkey && shiftkey)
		{
			willHalt = !willHalt;
			SendMessage(currentHwnd, WM_DESTROY, 0, 0);
			return CallNextHookEx(NULL, nCode, wParam, lParam); 
		}
		//controlkey || altkey
		switch (mystruct->vkCode)
			{
			case VK_LCONTROL:
			case VK_RCONTROL:
				controlkey = 0;
				break;
			case VK_LMENU:
			case VK_RMENU:
				altkey = 0;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
				shiftkey = 0;
				break;
			}
		return CallNextHookEx(NULL, nCode, wParam, lParam); 
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam); 
}

__declspec(dllexport) void setHWND(HWND hWnd)
{
	currentHwnd = hWnd;
}