#pragma once
#ifndef WIN32
#define WIN32
#endif
#include <WinSock2.h>
#include <string>
#include <cstdio>

#include "Util\WindowsConsole.hpp"
#include "Util\KeyboardHook.hpp"
#include "WowStuff\WowConsole.hpp"
#include "EndsceneHandler.hpp"
#include "WowStuff\WowClientDB.hpp"
#include "CommonDetours\ReadChatDetour.hpp"
#include "CommonDetours\NetClientProcessDetour.hpp"
#include "Memory\AntiWarden.hpp"

HINSTANCE instanceDLL = nullptr;
HWND wowWindow = nullptr;

DWORD MainThreadControl(LPVOID /* param */);
LRESULT WINAPI KeyboardHook(int/* nCode*/, WPARAM/* wParam*/, LPARAM/* lParam*/);

// entry point of the DLL
BOOL APIENTRY DllMain(HINSTANCE instDLL, DWORD reason, LPVOID /* reserved */)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		instanceDLL = instDLL;

		// disables thread notifications (DLL_THREAD_ATTACH, DLL_THREAD_DETACH)
		DisableThreadLibraryCalls(instDLL);	

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&MainThreadControl, NULL, 0, NULL);		
	}	
	else if (reason == DLL_PROCESS_DETACH)
	{		
		WindowsConsole::Destroy();		
	}
	return TRUE;
}

int endScenePointer{};
int resetPointer{};
void GetDevicePointers();

void WINAPI MessageLoop(LPVOID lpParm)
{
	hKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, instanceDLL, 0);
	MSG message;
	GetMessage(&message, NULL, 0, 0);	
}

//bool CCommand_TestCommand(char const* cmd, char const* args)
//{
//	ConsoleWriteA("Hello from TestCommand: cmd '%s', args '%s'", INPUT_COLOR, cmd, args);
//	return true;
//}

DWORD MainThreadControl(LPVOID lpParm)
{	
	g_memops["WardenDataHandler"] = new Detour(0x006CA5C0, (int)SMSG_WARDEN_DATA_HandlerDetour);

	WindowsConsole::Create();	
	Sleep(200);

	HWND wowWindow = FindWindow(0, "World of Warcraft");
	SetForegroundWindow(wowWindow);

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&MessageLoop, NULL, 0, NULL);

	GetDevicePointers();
	printf("End Scene Pointer = %X\n", endScenePointer); 
	printf("Reset Pointer = %X\n", resetPointer);

	// Fix InvalidPtrCheck for callbacks outside of .text section
	*(int*)0x884800 = 1;
	*(int*)0x884C00 = 0x7FFFFFFF;

	//g_memops["EndScene"] = new Detour(endScenePointer, (int)EndSceneDetour);
	g_memops["EndSceneCall"] = new Detour(0x005A17A0, (int)EndSceneDetour);
	g_memops["Reset"] = new Detour(resetPointer, (int)ResetDetour);
	
	/*g_memops["NetClientProcess"] = new Detour(0x00631FE0, (int)NetClientProcessDetour);
	g_memops["ReadChat"] = new Detour(0x00966580, (int)ReadChatDetour);	*/

	//InitDBTables();	

	//-----------------------------------  loop here before exit ----------------------------------
	//wait for the numpad 7 to be pressed...
	while (!should_exit) { }

	// exit
	if (hKeyHook)	
		UnhookWindowsHookEx(hKeyHook);

	//tell the enscene hook to finish
	shouldRemoveEndSceneInjection = true;

	printf("Exiting...\n");

	//wait the endscene hook to finish...
	while (!endSceneUnhooked) { }

	// unhooks all detours we created
	for (auto& det : g_memops)
	{
		delete det.second;
	}
	g_memops.clear();
	// shutdowns the bot
	// note: after that DLL's entry point will be called with reason DLL_PROCESS_DETACH
	FreeLibraryAndExitThread((HMODULE)instanceDLL, 0);
	return 0;
}

void GetDevicePointers()
{
	// Device
	int ptr;
	{
		ptr = *(int*)0x00C0ED38;
	}while (ptr == 0);

	{
		ptr = *(int*)(ptr + 0x38A8);
	}while (ptr == 0);

	// Scene	
	{
		ptr = *(int*)ptr;
	}while (ptr == 0);

	{
		endScenePointer = *(int*)(ptr + 0xA8);   //42 * 4	
	}while (endScenePointer == 0);

	{
		resetPointer = *(int*)(ptr + 0x40);   //16 * 4	
	}while (resetPointer == 0);
}








































//hKeyHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardHook, instanceDLL, 0);
//if (hKeyHook)
//	UnhookWindowsHookEx(hKeyHook);


//int countt;
//int __stdcall EndSceneDetour(int device)
//{
//	if (countt++ % 10 == 0)
//	{
//		//MessageBox(HWND_DESKTOP, "Text here", "Text here", MB_OK);
//		printf("count = %d\n", countt);
//		if (countt < 30)
//		{
//			((void(__cdecl*)(const char*, const char*, void*))0x00819210)("print('aaaaaaaaaaa')", "LetsDance", nullptr);
//		}
//	}
//	auto det = g_memops["EndScene"];
//	det->Restore();
//	int res = ((int(__stdcall*)(int))det->GetOrig())(device);
//	det->Apply();
//	return res;
//}