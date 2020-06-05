// dllmain.cpp : 定义HOOK API DLL的入口点。

// 版权声明：
// Copyright (C) 2013-2019 ETO.                   All Rights Reserved.
// Copyright (C) 2018-2019 FreeSTD Inc.           All Rights Reserved.
// Copyright (C) 2018-2019 Chengdu Zuosi Co.,Ltd. All Rights Reserved.

#include "pch.h"
#include <assert.h>
#include <tchar.h>
#include <stdlib.h>
#include "detours.h"
#pragma comment(lib , "detours.lib")

static HHOOK(WINAPI* poldHook)(
	_In_ int idHook,
	_In_ HOOKPROC lpfn,
	_In_opt_ HINSTANCE hmod,
	_In_ DWORD dwThreadId)
	= SetWindowsHookEx;
HHOOK WINAPI mySetWindowsHookEx(
	_In_ int idHook,
	_In_ HOOKPROC lpfn,
	_In_opt_ HINSTANCE hmod,
	_In_ DWORD dwThreadId)
{
	return 0;
};
int CursorCount = 0;
static int(WINAPI* poldHook2)(
	_In_ BOOL bShow) = ShowCursor;
int WINAPI myShowCursor(
	_In_ BOOL bShow)
{
	if (bShow)
		CursorCount++;
	else
		CursorCount--;
	return CursorCount;
}
void Hook()
{
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&poldHook, mySetWindowsHookEx);
	DetourAttach((void**)&poldHook2, myShowCursor);
	DetourTransactionCommit();
}
void UnHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach((void**)&poldHook, mySetWindowsHookEx);
	DetourDetach((void**)&poldHook2, myShowCursor);
	DetourTransactionCommit();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Hook();
		break;
	case DLL_PROCESS_DETACH:
		UnHook();
		break;
	}
    return TRUE;
}

